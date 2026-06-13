// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__
#include <linux/iopoll.h>

#include "sde_kms.h"
#include "sde_hw_catalog.h"
#include "sde_hwio.h"
#include "sde_hw_lm.h"
#include "sde_hw_mdss.h"
#include "sde_dbg.h"
#include "sde_kms.h"
#include "sde_hw_util.h"

#define LM_OP_MODE                              0x00
#define LM_OUT_SIZE                             0x04
#define LM_BORDER_COLOR_0                       0x08
#define LM_BORDER_COLOR_1                       0x10

/* These register are offset to mixer base + stage base */
#define LM_BLEND0_OP                            0x00
#define LM_BLEND0_CONST_ALPHA                   0x04
#define LM_BLEND0_FG_COLOR_FILL_COLOR_0         0x08
#define LM_BLEND0_FG_COLOR_FILL_COLOR_1         0x0C
#define LM_BLEND0_FG_COLOR_FILL_SIZE            0x10
#define LM_BLEND0_FG_COLOR_FILL_XY              0x14

#define LM_BLEND0_FG_ALPHA                      0x04
#define LM_BLEND0_BG_ALPHA                      0x08

#define LM_MISR_CTRL                            0x310
#define LM_MISR_SIGNATURE                       0x314
#define LM_NOISE_LAYER                          0x320

/* V1 Register Set */
#define LM_BORDER_COLOR_0_V1                    0x1C
#define LM_BORDER_COLOR_1_V1                    0x20
#define LM_BLEND0_CONST_ALPHA_V1                0x08
#define LM_BG_SRC_SEL_V1                        0x14
#define LM_BLEND0_FG_COLOR_FILL_COLOR_0_V1      0x0C
#define LM_BLEND0_FG_COLOR_FILL_COLOR_1_V1      0x10
#define LM_BLEND0_FG_COLOR_FILL_SIZE_V1         0x14
#define LM_BLEND0_FG_COLOR_FILL_XY_V1           0x18
#define LM_BLEND0_FG_SRC_SEL_V1                 0x04

#define LM_SRC_SEL_RESET_VALUE 0x0000C0C0

static struct sde_lm_cfg *_lm_offset(enum sde_lm mixer,
		struct sde_mdss_cfg *m,
		void __iomem *addr,
		struct sde_hw_blk_reg_map *b)
{
	int i;

	for (i = 0; i < m->mixer_count; i++) {
		if (mixer == m->mixer[i].id) {
			b->base_off = addr;
			b->blk_off = m->mixer[i].base;
			b->length = m->mixer[i].len;
			b->hw_rev = m->hw_rev;
			b->log_mask = SDE_DBG_MASK_LM;
			return &m->mixer[i];
		}
	}

	return ERR_PTR(-ENOMEM);
}

/**
 * _stage_offset(): returns the relative offset of the blend registers
 * for the stage to be setup
 * @c:     mixer ctx contains the mixer to be programmed
 * @stage: stage index to setup
 */
static inline int _stage_offset(struct sde_hw_mixer *ctx, enum sde_stage stage)
{
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;
	int rc;

	if (stage == SDE_STAGE_BASE)
		rc = -EINVAL;
	else if (stage <= sblk->maxblendstages)
		rc = sblk->blendstage_base[stage - SDE_STAGE_0];
	else
		rc = -EINVAL;

	return rc;
}

static void sde_hw_lm_setup_out(struct sde_hw_mixer *ctx,
		struct sde_hw_mixer_cfg *mixer)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 outsize;
	u32 op_mode;

	op_mode = SDE_REG_READ(c, LM_OP_MODE);

	outsize = mixer->out_height << 16 | mixer->out_width;
	SDE_REG_WRITE(c, LM_OUT_SIZE, outsize);

	/* SPLIT_LEFT_RIGHT */
	if (mixer->right_mixer)
		op_mode |= BIT(31);
	else
		op_mode &= ~BIT(31);
	SDE_REG_WRITE(c, LM_OP_MODE, op_mode);
}

static void sde_hw_lm_setup_border_color(struct sde_hw_mixer *ctx,
		struct sde_mdss_color *color,
		u8 border_en)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;

	if (border_en) {
		SDE_REG_WRITE(c, LM_BORDER_COLOR_0,
			(color->color_0 & 0xFFF) |
			((color->color_1 & 0xFFF) << 0x10));
		SDE_REG_WRITE(c, LM_BORDER_COLOR_1,
			(color->color_2 & 0xFFF) |
			((color->color_3 & 0xFFF) << 0x10));
	}
}

static void sde_hw_lm_setup_border_color_v1(struct sde_hw_mixer *ctx,
		struct sde_mdss_color *color,
		u8 border_en)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;

	if (border_en) {
		SDE_REG_WRITE(c, LM_BORDER_COLOR_0_V1,
			(color->color_0 & 0xFFF) |
			((color->color_1 & 0xFFF) << 0x10));
		SDE_REG_WRITE(c, LM_BORDER_COLOR_1_V1,
			(color->color_2 & 0xFFF) |
			((color->color_3 & 0xFFF) << 0x10));
	}
}

static void sde_hw_lm_setup_border_color_10_bits(struct sde_hw_mixer *ctx,
		struct sde_mdss_color *color,
		u8 border_en)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;

	if (!border_en)
		return;

	SDE_REG_WRITE(c, LM_BORDER_COLOR_0_V1,
			(color->color_0 & 0x3FF) |
			((color->color_1 & 0x3FF) << 16));
	SDE_REG_WRITE(c, LM_BORDER_COLOR_1_V1,
			(color->color_2 & 0x3FF) |
			((color->color_3 & 0x3FF) << 16));
}

static void sde_hw_lm_setup_blend_config_combined_alpha(
	struct sde_hw_mixer *ctx, u32 stage,
	u32 fg_alpha, u32 bg_alpha, u32 blend_op)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 const_alpha;

	if (stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, stage);
	if (WARN_ON(stage_off < 0))
		return;

	const_alpha = (bg_alpha & 0xFF) | ((fg_alpha & 0xFF) << 16);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA + stage_off, const_alpha);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, blend_op);
}

static void sde_hw_lm_setup_blend_config_combined_alpha_v1(
	struct sde_hw_mixer *ctx, u32 stage,
	u32 fg_alpha, u32 bg_alpha, u32 blend_op)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 const_alpha;

	if (stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, stage);
	if (WARN_ON(stage_off < 0))
		return;

	const_alpha = (bg_alpha & 0xFF) | ((fg_alpha & 0xFF) << 16);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, const_alpha);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, blend_op);
}

static void sde_hw_lm_setup_blend_config_combined_alpha_10_bits(
	struct sde_hw_mixer *ctx, u32 stage,
	u32 fg_alpha, u32 bg_alpha, u32 blend_op)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 const_alpha;

	if (stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, stage);
	if (WARN_ON(stage_off < 0))
		return;

	const_alpha = (bg_alpha & 0x3FF) | ((fg_alpha & 0x3FF) << 16);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, const_alpha);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, blend_op);
}

static void sde_hw_lm_setup_blend_config(struct sde_hw_mixer *ctx,
	u32 stage, u32 fg_alpha, u32 bg_alpha, u32 blend_op)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;

	if (stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, stage);
	if (WARN_ON(stage_off < 0))
		return;

	SDE_REG_WRITE(c, LM_BLEND0_FG_ALPHA + stage_off, fg_alpha);
	SDE_REG_WRITE(c, LM_BLEND0_BG_ALPHA + stage_off, bg_alpha);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, blend_op);
}

static void sde_hw_lm_setup_color3(struct sde_hw_mixer *ctx,
	u32 mixer_op_mode)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int op_mode;

	/* read the existing op_mode configuration */
	op_mode = SDE_REG_READ(c, LM_OP_MODE);

	op_mode = (op_mode & (BIT(31) | BIT(30))) | mixer_op_mode;

	SDE_REG_WRITE(c, LM_OP_MODE, op_mode);
}

static void sde_hw_lm_setup_color3_v1(struct sde_hw_mixer *ctx, uint32_t mixer_op_mode)
{
	struct sde_hw_blk_reg_map *c;
	int stages, stage_off, i;
	int val;

	if (!ctx)
		return;

	c = &ctx->hw;
	stages = ctx->cap->sblk->maxblendstages;
	if (stages < 0)
		return;

	for (i = SDE_STAGE_0; i <= stages; i++) {
		stage_off = _stage_offset(ctx, i);
		if (WARN_ON(stage_off < 0))
			return;

		/* set color_out3 bit in blend0_op when enabled in mixer_op_mode */
		val = SDE_REG_READ(c, LM_BLEND0_OP + stage_off);
		if (mixer_op_mode & BIT(i))
			val |= BIT(30);
		else
			val &= ~BIT(30);

		SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	}
}

static void sde_hw_lm_gc(struct sde_hw_mixer *mixer,
			void *cfg)
{
}

static void sde_hw_lm_clear_dim_layer(struct sde_hw_mixer *ctx)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;
	int stage_off, i;
	u32 reset = BIT(16), val;

	reset = ~reset;
	for (i = SDE_STAGE_0; i <= sblk->maxblendstages; i++) {
		stage_off = _stage_offset(ctx, i);
		if (WARN_ON(stage_off < 0))
			return;

		/*
		 * read the existing blendn_op register and clear only DIM layer
		 * bit (color_fill bit)
		 */
		val = SDE_REG_READ(c, LM_BLEND0_OP + stage_off);
		val &= reset;
		SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	}
}

static void sde_hw_lm_setup_dim_layer(struct sde_hw_mixer *ctx,
		struct sde_hw_dim_layer *dim_layer)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 val = 0, alpha = 0;

	if (dim_layer->stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, dim_layer->stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for dim layer\n", stage_off);
		return;
	}

	alpha = dim_layer->color_fill.color_3 & 0xFF;
	val = ((dim_layer->color_fill.color_1 << 2) & 0xFFF) << 16 |
			((dim_layer->color_fill.color_0 << 2) & 0xFFF);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_COLOR_0 + stage_off, val);

	val = (alpha << 4) << 16 |
			((dim_layer->color_fill.color_2 << 2) & 0xFFF);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_COLOR_1 + stage_off, val);

	val = dim_layer->rect.h << 16 | dim_layer->rect.w;
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE + stage_off, val);

	val = dim_layer->rect.y << 16 | dim_layer->rect.x;
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY + stage_off, val);

	val = BIT(16); /* enable dim layer */
	val |= SDE_BLEND_FG_ALPHA_FG_CONST | SDE_BLEND_BG_ALPHA_BG_CONST;
	if (dim_layer->flags & SDE_DRM_DIM_LAYER_EXCLUSIVE)
		val |= BIT(17);
	else
		val &= ~BIT(17);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	val = (alpha << 16) | (0xff - alpha);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA + stage_off, val);
}

static void sde_hw_lm_setup_dim_layer_v1(struct sde_hw_mixer *ctx,
		struct sde_hw_dim_layer *dim_layer)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 val = 0, alpha = 0;

	if (dim_layer->stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, dim_layer->stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for dim layer\n", stage_off);
		return;
	}

	alpha = dim_layer->color_fill.color_3 & 0xFF;
	val = ((dim_layer->color_fill.color_1 << 2) & 0xFFF) << 16 |
			((dim_layer->color_fill.color_0 << 2) & 0xFFF);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_COLOR_0_V1 + stage_off, val);

	val = (alpha << 4) << 16 |
			((dim_layer->color_fill.color_2 << 2) & 0xFFF);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_COLOR_1_V1 + stage_off, val);

	val = dim_layer->rect.h << 16 | dim_layer->rect.w;
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE_V1 + stage_off, val);

	val = dim_layer->rect.y << 16 | dim_layer->rect.x;
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY_V1 + stage_off, val);

	val = BIT(16); /* enable dim layer */
	val |= SDE_BLEND_FG_ALPHA_FG_CONST | SDE_BLEND_BG_ALPHA_BG_CONST;
	if (dim_layer->flags & SDE_DRM_DIM_LAYER_EXCLUSIVE)
		val |= BIT(17);
	else
		val &= ~BIT(17);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	val = (alpha << 16) | (0xff - alpha);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, val);
}

static void sde_hw_lm_setup_dim_layer_10_bits(struct sde_hw_mixer *ctx,
		struct sde_hw_dim_layer *dim_layer)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 val = 0, alpha = 0;

	if (dim_layer->stage == SDE_STAGE_BASE)
		return;

	stage_off = _stage_offset(ctx, dim_layer->stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for dim layer\n", stage_off);
		return;
	}

	alpha = dim_layer->color_fill.color_3 & 0x3FF;
	val = (dim_layer->color_fill.color_1 & 0x3FF) << 16 |
			(dim_layer->color_fill.color_0 & 0x3FF);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_COLOR_0_V1 + stage_off, val);

	val = (alpha  << 16) |
			(dim_layer->color_fill.color_2 & 0x3FF);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_COLOR_1_V1 + stage_off, val);

	val = dim_layer->rect.h << 16 | dim_layer->rect.w;
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE_V1 + stage_off, val);

	val = dim_layer->rect.y << 16 | dim_layer->rect.x;
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY_V1 + stage_off, val);

	val = BIT(16); /* enable dim layer */
	val |= SDE_BLEND_FG_ALPHA_FG_CONST | SDE_BLEND_BG_ALPHA_BG_CONST;
	if (dim_layer->flags & SDE_DRM_DIM_LAYER_EXCLUSIVE)
		val |= BIT(17);
	else
		val &= ~BIT(17);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	val = (alpha << 16) | (0x3FF - alpha);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, val);
}

static void sde_hw_lm_setup_misr(struct sde_hw_mixer *ctx,
				bool enable, u32 frame_count)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 config = 0;

	SDE_REG_WRITE(c, LM_MISR_CTRL, MISR_CTRL_STATUS_CLEAR);
	/* clear misr data */
	wmb();

	if (enable)
		config = (frame_count & MISR_FRAME_COUNT_MASK) |
			MISR_CTRL_ENABLE | INTF_MISR_CTRL_FREE_RUN_MASK;

	SDE_REG_WRITE(c, LM_MISR_CTRL, config);
}

static int sde_hw_lm_collect_misr(struct sde_hw_mixer *ctx, bool nonblock,
		u32 *misr_value)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 ctrl = 0;
	int rc = 0;

	if (!misr_value)
		return -EINVAL;

	ctrl = SDE_REG_READ(c, LM_MISR_CTRL);
	if (!nonblock) {
		if (ctrl & MISR_CTRL_ENABLE) {
			rc = read_poll_timeout(sde_reg_read, ctrl, (ctrl & MISR_CTRL_STATUS) > 0,
					500, false, 84000, c, LM_MISR_CTRL);
			if (rc)
				return rc;
		} else {
			return -EINVAL;
		}
	}

	*misr_value  = SDE_REG_READ(c, LM_MISR_SIGNATURE);

	return rc;
}

static void sde_hw_clear_noise_layer(struct sde_hw_mixer *ctx)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;
	int stage_off, i;
	u32 reset = BIT(18) | BIT(31), val;

	reset = ~reset;
	for (i = SDE_STAGE_0; i <= sblk->maxblendstages; i++) {
		stage_off = _stage_offset(ctx, i);
		if (WARN_ON(stage_off < 0))
			return;

		/**
		 * read the blendn_op register and clear only noise layer
		 */
		val = SDE_REG_READ(c, LM_BLEND0_OP + stage_off);
		val &= reset;
		SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	}
	SDE_REG_WRITE(c, LM_NOISE_LAYER, 0);
}

static int sde_hw_lm_setup_noise_layer(struct sde_hw_mixer *ctx,
		struct sde_hw_noise_layer_cfg *cfg)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 val = 0, alpha = 0;
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;
	struct sde_hw_mixer_cfg *mixer = &ctx->cfg;

	sde_hw_clear_noise_layer(ctx);
	if (!cfg)
		return 0;

	if (cfg->noise_blend_stage == SDE_STAGE_BASE ||
		cfg->noise_blend_stage + 1 != cfg->attn_blend_stage ||
		cfg->attn_blend_stage >= sblk->maxblendstages) {
		SDE_ERROR("invalid noise_blend_stage %d attn_blend_stage %d max stage %d\n",
			cfg->noise_blend_stage, cfg->attn_blend_stage, sblk->maxblendstages);
		return -EINVAL;
	}

	stage_off = _stage_offset(ctx, cfg->noise_blend_stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for noise layer blend stage:%d\n",
				stage_off, cfg->noise_blend_stage);
		return -EINVAL;
	}
	val = BIT(18) | BIT(31);
	val |= (1 << 8);
	alpha = 255 | (cfg->alpha_noise << 16);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA + stage_off, alpha);
	val = ctx->cfg.out_width | (ctx->cfg.out_height << 16);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE + stage_off, val);
	/* partial update is not supported in noise layer */
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY + stage_off, 0);
	val = SDE_REG_READ(c, LM_OP_MODE);
	val = (1 << cfg->noise_blend_stage) | val;
	SDE_REG_WRITE(c, LM_OP_MODE, val);

	stage_off = _stage_offset(ctx, cfg->attn_blend_stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for atten layer blend stage:%d\n",
				stage_off, cfg->attn_blend_stage);
		sde_hw_clear_noise_layer(ctx);
		return -EINVAL;
	}
	val = 1 | BIT(31) | BIT(16);
	val |= BIT(2);
	val |= (1 << 8);
	alpha = cfg->attn_factor;
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA + stage_off, alpha);
	val = SDE_REG_READ(c, LM_OP_MODE);
	val = (1 << cfg->attn_blend_stage) | val;
	SDE_REG_WRITE(c, LM_OP_MODE, val);
	val = ctx->cfg.out_width | (ctx->cfg.out_height << 16);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE + stage_off, val);
	/* partial update is not supported in noise layer */
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY + stage_off, 0);

	val = 1;
	if (mixer->right_mixer)
		val |= (((mixer->out_width % 4) & 0x3) << 4);

	if (cfg->flags & DRM_NOISE_TEMPORAL_FLAG)
		val |= BIT(1);
	val |= ((cfg->strength & 0x7) << 8);
	SDE_REG_WRITE(c, LM_NOISE_LAYER, val);
	return 0;
}

static int sde_hw_lm_setup_noise_layer_v1(struct sde_hw_mixer *ctx,
		struct sde_hw_noise_layer_cfg *cfg)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 val = 0, alpha = 0;
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;
	struct sde_hw_mixer_cfg *mixer = &ctx->cfg;

	sde_hw_clear_noise_layer(ctx);
	if (!cfg)
		return 0;

	if (cfg->noise_blend_stage == SDE_STAGE_BASE ||
		cfg->noise_blend_stage + 1 != cfg->attn_blend_stage ||
		cfg->attn_blend_stage >= sblk->maxblendstages) {
		SDE_ERROR("invalid noise_blend_stage %d attn_blend_stage %d max stage %d\n",
			cfg->noise_blend_stage, cfg->attn_blend_stage, sblk->maxblendstages);
		return -EINVAL;
	}

	stage_off = _stage_offset(ctx, cfg->noise_blend_stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for noise layer blend stage:%d\n",
				stage_off, cfg->noise_blend_stage);
		return -EINVAL;
	}
	val = BIT(18) | BIT(31);
	val |= (1 << 8);
	alpha = 255 | (cfg->alpha_noise << 16);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, alpha);
	val = ctx->cfg.out_width | (ctx->cfg.out_height << 16);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE_V1 + stage_off, val);
	/* partial update is not supported in noise layer */
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY_V1 + stage_off, 0);
	val = SDE_REG_READ(c, LM_OP_MODE);
	val = (1 << cfg->noise_blend_stage) | val;
	SDE_REG_WRITE(c, LM_OP_MODE, val);

	stage_off = _stage_offset(ctx, cfg->attn_blend_stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for atten layer blend stage:%d\n",
				stage_off, cfg->attn_blend_stage);
		sde_hw_clear_noise_layer(ctx);
		return -EINVAL;
	}
	val = 1 | BIT(31) | BIT(16);
	val |= BIT(2);
	val |= (1 << 8);
	alpha = cfg->attn_factor;
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, alpha);
	val = SDE_REG_READ(c, LM_OP_MODE);
	val = (1 << cfg->attn_blend_stage) | val;
	SDE_REG_WRITE(c, LM_OP_MODE, val);
	val = ctx->cfg.out_width | (ctx->cfg.out_height << 16);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE_V1 + stage_off, val);
	/* partial update is not supported in noise layer */
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY_V1 + stage_off, 0);

	val = 1;
	if (mixer->right_mixer)
		val |= (((mixer->out_width % 4) & 0x3) << 4);

	if (cfg->flags & DRM_NOISE_TEMPORAL_FLAG)
		val |= BIT(1);
	val |= ((cfg->strength & 0x7) << 8);
	SDE_REG_WRITE(c, LM_NOISE_LAYER, val);
	return 0;
}

static int sde_hw_lm_setup_noise_layer_10_bits(struct sde_hw_mixer *ctx,
		struct sde_hw_noise_layer_cfg *cfg)
{
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off;
	u32 val = 0, alpha = 0;
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;
	struct sde_hw_mixer_cfg *mixer = &ctx->cfg;

	sde_hw_clear_noise_layer(ctx);
	if (!cfg)
		return 0;

	if (cfg->noise_blend_stage == SDE_STAGE_BASE ||
		cfg->noise_blend_stage + 1 != cfg->attn_blend_stage ||
		cfg->attn_blend_stage >= sblk->maxblendstages) {
		SDE_ERROR("invalid noise_blend_stage %d attn_blend_stage %d max stage %d\n",
			cfg->noise_blend_stage, cfg->attn_blend_stage, sblk->maxblendstages);
		return -EINVAL;
	}

	stage_off = _stage_offset(ctx, cfg->noise_blend_stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for noise layer blend stage:%d\n",
				stage_off, cfg->noise_blend_stage);
		return -EINVAL;
	}
	val = BIT(18) | BIT(31);
	val |= (1 << 8);
	alpha = 0x3FF | (cfg->alpha_noise << 16);
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, alpha);
	val = ctx->cfg.out_width | (ctx->cfg.out_height << 16);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE_V1 + stage_off, val);
	/* partial update is not supported in noise layer */
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY_V1 + stage_off, 0);
	val = SDE_REG_READ(c, LM_OP_MODE);
	val = (1 << cfg->noise_blend_stage) | val;
	SDE_REG_WRITE(c, LM_OP_MODE, val);

	stage_off = _stage_offset(ctx, cfg->attn_blend_stage);
	if (stage_off < 0) {
		SDE_ERROR("invalid stage_off:%d for atten layer blend stage:%d\n",
				stage_off, cfg->attn_blend_stage);
		sde_hw_clear_noise_layer(ctx);
		return -EINVAL;
	}
	val = 1 | BIT(31) | BIT(16);
	val |= BIT(2);
	val |= (1 << 8);
	alpha = cfg->attn_factor;
	SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
	SDE_REG_WRITE(c, LM_BLEND0_CONST_ALPHA_V1 + stage_off, alpha);
	val = SDE_REG_READ(c, LM_OP_MODE);
	val = (1 << cfg->attn_blend_stage) | val;
	SDE_REG_WRITE(c, LM_OP_MODE, val);
	val = ctx->cfg.out_width | (ctx->cfg.out_height << 16);
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_SIZE_V1 + stage_off, val);
	/* partial update is not supported in noise layer */
	SDE_REG_WRITE(c, LM_BLEND0_FG_COLOR_FILL_XY_V1 + stage_off, 0);

	val = 1;
	if (mixer->right_mixer)
		val |= (((mixer->out_width % 4) & 0x3) << 4);

	if (cfg->flags & DRM_NOISE_TEMPORAL_FLAG)
		val |= BIT(1);
	val |= ((cfg->strength & 0x7) << 8);
	SDE_REG_WRITE(c, LM_NOISE_LAYER, val);
	return 0;
}

static int _set_staged_sspp(u32 stage,
		struct sde_hw_stage_cfg *stage_cfg, int pipes_per_stage, u32 *value)
{
	int i;
	u32 pipe_type = 0, pipe_id = 0, rec_id = 0, layout = 0;
	u32 src_sel[PIPES_PER_STAGE];

	/* reset mask is 0xc0c0 */
	*value = LM_SRC_SEL_RESET_VALUE;
	if (!stage_cfg || !pipes_per_stage)
		return 0;

	for (i = 0; i < pipes_per_stage; i++) {
		enum sde_sspp pipe = stage_cfg->stage[stage][i];
		enum sde_sspp_multirect_index rect_index = stage_cfg->multirect_index[stage][i];

		src_sel[i] = LM_SRC_SEL_RESET_VALUE;

		if (!pipe || pipe >= SSPP_MAX || rect_index >= SDE_SSPP_RECT_MAX)
			continue;

		/* translate pipe data to SWI pipe_type, pipe_id */
		if (SDE_SSPP_VALID_DMA(pipe)) {
			pipe_type = 0;
			pipe_id = pipe - SSPP_DMA0;
		} else if (SDE_SSPP_VALID_VIG(pipe)) {
			pipe_type = 1;
			pipe_id = pipe - SSPP_VIG0;
		} else {
			SDE_ERROR("invalid rec-%d pipe:%d\n", i, pipe);
			return -EINVAL;
		}

		/* translate rec data to SWI rec_id */
		if (rect_index == SDE_SSPP_RECT_SOLO || rect_index == SDE_SSPP_RECT_0) {
			rec_id = 0;
		} else if (rect_index == SDE_SSPP_RECT_1) {
			rec_id = 1;
		} else {
			SDE_ERROR("invalid rec-%d rect_index:%d\n", i, rect_index);
			rec_id = 0;
		}

		/* calculate SWI value for rec-0 and rec-1 and store it temporary buffer */
		src_sel[i] = (((pipe_type & 0x3) << 6) | ((rec_id & 0x3) << 4) | (pipe_id & 0xf));
	}

	/* calculate final SWI register value for rec-0 and rec-1 */
	for (i = 0; i < pipes_per_stage; i++) {
		if (src_sel[i] == LM_SRC_SEL_RESET_VALUE)
			continue;

		layout = stage_cfg->layout[stage][i];
		*value = *value & ~(0xff << (layout * 8));
		*value |= (src_sel[i] << (layout * 8));
	}

	return 0;
}

static int sde_hw_lm_setup_blendstage(struct sde_hw_mixer *ctx,
		enum sde_lm lm, struct sde_hw_stage_cfg *stage_cfg, bool disable_border)
{
	struct sde_hw_blk_reg_map *c;
	int i, ret, stages, stage_off, pipes_per_stage;
	u32 value;

	if (!ctx)
		return -EINVAL;

	if (lm < LM_0 || lm >= LM_MAX)
		return -EINVAL;

	c = &ctx->hw;
	stages = ctx->cap->sblk->maxblendstages;
	if (stages <= SDE_STAGE_BASE)
		return -EINVAL;

	if (test_bit(SDE_MIXER_SOURCESPLIT, &ctx->cap->features))
		pipes_per_stage = PIPES_PER_STAGE;
	else
		pipes_per_stage = 1;

	/*
	 * When stage configuration is empty, we can enable the
	 * border color by setting the corresponding LAYER_ACTIVE bit
	 * and un-staging all the pipes from the layer mixer.
	 */
	if (!stage_cfg)
		SDE_REG_WRITE(c, LM_BG_SRC_SEL_V1, LM_SRC_SEL_RESET_VALUE);

	for (i = SDE_STAGE_0; i <= stages; i++) {
		stage_off = _stage_offset(ctx, i);
		if (stage_off < 0)
			return stage_off;

		ret = _set_staged_sspp(i, stage_cfg, pipes_per_stage, &value);
		if (ret)
			return ret;

		SDE_REG_WRITE(c, LM_BLEND0_FG_SRC_SEL_V1 + stage_off, value);
	}

	return ret;
}

static int _get_staged_sspp(u32 value, int pipes_per_stage, struct sde_sspp_index_info *info)
{
	int i;
	u32 pipe_type = 0, rec_id = 0, pipe_count = 0;
	unsigned long pipe_id = 0;

	for (i = 0; i < pipes_per_stage; i++) {
		pipe_type = (value >> (i * 8 + 6)) & 0x3;
		rec_id = (value >> (i * 8 + 4)) & 0x3;
		pipe_id = (value >> (i * 8)) & 0xf;

		if (rec_id < 0x2) {
			if (pipe_type == 0x0) {
				set_bit(pipe_id + SSPP_DMA0,
						rec_id ? info->virt_pipes : info->pipes);
				pipe_count++;
			} else if (pipe_type == 0x1) {
				set_bit(pipe_id + SSPP_VIG0,
						rec_id ? info->virt_pipes : info->pipes);
				pipe_count++;
			} else {
				SDE_DEBUG("invalid rec-%d pipe_type %d, value:0x%x\n",
						i, pipe_type, value);
				return 0;
			}
		}
	}

	return pipe_count;
}

static int sde_hw_lm_get_staged_sspp(struct sde_hw_mixer *ctx,
		enum sde_lm lm, struct sde_sspp_index_info *info)
{
	struct sde_hw_blk_reg_map *c;
	int stage_off, pipes_per_stage;
	u32 value, pipe_count = 0;
	int i, stages, ret;

	if (!ctx || !info)
		return 0;

	c = &ctx->hw;
	stages = ctx->cap->sblk->maxblendstages;
	if (stages <= SDE_STAGE_BASE)
		return 0;

	if (test_bit(SDE_MIXER_SOURCESPLIT, &ctx->cap->features))
		pipes_per_stage = PIPES_PER_STAGE;
	else
		pipes_per_stage = 1;

	for (i = SDE_STAGE_0; i <= stages; i++) {
		stage_off = _stage_offset(ctx, i);
		if (stage_off < 0)
			return 0;

		value = SDE_REG_READ(c, LM_BLEND0_FG_SRC_SEL_V1 + stage_off);
		ret = _get_staged_sspp(value, pipes_per_stage, info);
		if (ret > 0)
			pipe_count++;
	}

	return pipe_count;
}

static int sde_hw_lm_clear_all_blendstages(struct sde_hw_mixer *ctx)
{
	struct sde_hw_blk_reg_map *c;
	int i, stages, stage_off;

	if (!ctx)
		return -EINVAL;

	c = &ctx->hw;
	stages = ctx->cap->sblk->maxblendstages;
	if (stages < 0)
		return -EINVAL;

	SDE_REG_WRITE(c, LM_BG_SRC_SEL_V1, LM_SRC_SEL_RESET_VALUE);

	for (i = SDE_STAGE_0; i <= stages; i++) {
		stage_off = _stage_offset(ctx, i);
		if (stage_off < 0)
			return stage_off;

		SDE_REG_WRITE(c, LM_BLEND0_FG_SRC_SEL_V1 + stage_off,
				LM_SRC_SEL_RESET_VALUE);
	}

	return 0;
}

static void _setup_mixer_ops(struct sde_mdss_cfg *m,
		struct sde_hw_lm_ops *ops,
		unsigned long features)
{
	ops->setup_mixer_out = sde_hw_lm_setup_out;
	if (test_bit(SDE_MIXER_COMBINED_ALPHA, &features)) {
		if (test_bit(SDE_MIXER_10_BITS_ALPHA, &features))
			ops->setup_blend_config =
				sde_hw_lm_setup_blend_config_combined_alpha_10_bits;
		else if (test_bit(SDE_FEATURE_MIXER_OP_V1, m->features))
			ops->setup_blend_config =
				sde_hw_lm_setup_blend_config_combined_alpha_v1;
		else
			ops->setup_blend_config =
				sde_hw_lm_setup_blend_config_combined_alpha;
	} else {
		ops->setup_blend_config = sde_hw_lm_setup_blend_config;
	}

	if (test_bit(SDE_MIXER_X_SRC_SEL, &features)) {
		ops->setup_blendstage = sde_hw_lm_setup_blendstage;
		ops->get_staged_sspp = sde_hw_lm_get_staged_sspp;
		ops->clear_all_blendstages = sde_hw_lm_clear_all_blendstages;
		ops->setup_alpha_out = sde_hw_lm_setup_color3_v1;
	} else {
		ops->setup_alpha_out = sde_hw_lm_setup_color3;
	}

	if (test_bit(SDE_MIXER_10_BITS_COLOR, &features))
		ops->setup_border_color = sde_hw_lm_setup_border_color_10_bits;
	else if (test_bit(SDE_FEATURE_MIXER_OP_V1, m->features))
		ops->setup_border_color = sde_hw_lm_setup_border_color_v1;
	else
		ops->setup_border_color = sde_hw_lm_setup_border_color;

	ops->setup_gc = sde_hw_lm_gc;
	ops->setup_misr = sde_hw_lm_setup_misr;
	ops->collect_misr = sde_hw_lm_collect_misr;

	if (test_bit(SDE_DIM_LAYER, &features)) {
		if (test_bit(SDE_MIXER_10_BITS_COLOR, &features))
			ops->setup_dim_layer = sde_hw_lm_setup_dim_layer_10_bits;
		else if (test_bit(SDE_FEATURE_MIXER_OP_V1, m->features))
			ops->setup_dim_layer = sde_hw_lm_setup_dim_layer_v1;
		else
			ops->setup_dim_layer = sde_hw_lm_setup_dim_layer;
		ops->clear_dim_layer = sde_hw_lm_clear_dim_layer;
	}

	if (test_bit(SDE_MIXER_NOISE_LAYER, &features)) {
		if (test_bit(SDE_MIXER_10_BITS_COLOR, &features))
			ops->setup_noise_layer = sde_hw_lm_setup_noise_layer_10_bits;
		else if (test_bit(SDE_FEATURE_MIXER_OP_V1, m->features))
			ops->setup_noise_layer = sde_hw_lm_setup_noise_layer_v1;
		else
			ops->setup_noise_layer = sde_hw_lm_setup_noise_layer;
	}
};

struct sde_hw_blk_reg_map *sde_hw_lm_init(enum sde_lm idx,
		void __iomem *addr,
		struct sde_mdss_cfg *m)
{
	struct sde_hw_mixer *c;
	struct sde_lm_cfg *cfg;

	c = kzalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return ERR_PTR(-ENOMEM);

	cfg = _lm_offset(idx, m, addr, &c->hw);
	if (IS_ERR_OR_NULL(cfg)) {
		kfree(c);
		return ERR_PTR(-EINVAL);
	}

	/* Assign ops */
	c->idx = idx;
	c->cap = cfg;

	/* Dummy mixers should not setup ops nor add to dump ranges */
	if (cfg->dummy_mixer)
		goto done;

	_setup_mixer_ops(m, &c->ops, c->cap->features);

	sde_dbg_reg_register_dump_range(SDE_DBG_NAME, cfg->name, c->hw.blk_off,
			c->hw.blk_off + c->hw.length, c->hw.xin_id);

done:
	return &c->hw;
}

void sde_hw_lm_destroy(struct sde_hw_blk_reg_map *hw)
{
	if (hw)
		kfree(to_sde_hw_mixer(hw));
}
