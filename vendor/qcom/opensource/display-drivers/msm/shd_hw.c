// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm-shd:%s:%d] " fmt, __func__, __LINE__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/component.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/sched/types.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_crtc.h>
#include <drm/drm_vblank.h>
#include "msm_drv.h"
#include "msm_kms.h"
#include "sde_connector.h"
#include "sde_encoder.h"
#include "sde_crtc.h"
#include "sde_plane.h"
#include "shd_drm.h"
#include "shd_hw.h"
#include "sde_hw_ctl.h"

#define CTL_LAYER(lm)            (((lm) == LM_5) ? (0x024) : (((lm) - LM_0) * 0x004))
#define CTL_LAYER_EXT(lm)        (0x40 + (((lm) - LM_0) * 0x004))
#define CTL_LAYER_EXT2(lm)       (0x70 + (((lm) - LM_0) * 0x004))
#define CTL_LAYER_EXT3(lm)       (0xA0 + (((lm) - LM_0) * 0x004))
#define CTL_LAYER_EXT4(lm)       (0xB8 + (((lm) - LM_0) * 0x004))

#define CTL_MIXER_BORDER_OUT          BIT(24)
#define CTL_FLUSH_MASK                0x090
#define LM_BLEND0_OP                  0x00
#define CTL_NUM_EXT                   5
#define CTL_SSPP_MAX_RECTS            2
#define CTL_MERGE_3D_ACTIVE           0x0E4
#define CTL_WB_ACTIVE                 0x0EC
#define CTL_CWB_ACTIVE                0x0F0
#define CTL_FETCH_PIPE_ACTIVE         0x0FC
#define CTL_INVALID_BIT               0xffff

#define CTL_SSPP_FLUSH_MASK              0x3041807
#define FLUSH_MASK_ALL                   0xfffffff

/* SDE_ROI_MISR_CTL */
#define ROI_MISR_OP_MODE                 0x00
#define ROI_MISR_POSITION(i)            (0x10 + 0x4 * (i))
#define ROI_MISR_SIZE(i)                (0x20 + 0x4 * (i))
#define ROI_MISR_CTRL(i)                (0x30 + 0x4 * (i))
#define ROI_MISR_EXPECTED(i)            (0x50 + 0x4 * (i))

/* ROI_MISR_CTRL register */
#define ROI_MISR_CTRL_ENABLE            BIT(8)
#define ROI_MISR_CTRL_STATUS_CLEAR      BIT(10)
#define ROI_MISR_CTRL_RUN_MODE          BIT(31)

#define ROI_POSITION_VAL(x, y)          ((x) | ((y) << 16))
#define ROI_SIZE_VAL(w, h)              ((w) | ((h) << 16))

static DEFINE_SPINLOCK(hw_ctl_lock);

/**
 * List of SSPP bits in CTL_FETCH_PIPE_ACTIVE
 */
static const u32 fetch_tbl[SSPP_MAX] = {CTL_INVALID_BIT, 16, 17, 18, 19, 0, 1, 2, 3, 4, 5};

/**
 * struct ctl_sspp_stage_reg_map: Describes bit layout for a sspp stage cfg
 * @ext: Index to indicate LAYER_x_EXT id for given sspp
 * @start: Start position of blend stage bits for given sspp
 * @bits: Number of bits from @start assigned for given sspp
 * @sec_bit_mask: Bitmask to add to LAYER_x_EXT1 for missing bit of sspp
 */
struct ctl_sspp_stage_reg_map {
	u32 ext;
	u32 start;
	u32 bits;
	u32 sec_bit_mask;
};

/* list of ctl_sspp_stage_reg_map for all the sppp */
static const struct ctl_sspp_stage_reg_map
sspp_reg_cfg_tbl[SSPP_MAX][CTL_SSPP_MAX_RECTS] = {
	/* SSPP_NONE */{ {0, 0, 0, 0}, {0, 0, 0, 0} },
	/* SSPP_VIG0 */{ {0, 0, 3, BIT(0)}, {3, 0, 4, 0} },
	/* SSPP_VIG1 */{ {0, 3, 3, BIT(2)}, {3, 4, 4, 0} },
	/* SSPP_VIG2 */{ {0, 6, 3, BIT(4)}, {3, 8, 4, 0} },
	/* SSPP_VIG3 */{ {0, 26, 3, BIT(6)}, {3, 12, 4, 0} },
	/* SSPP_DMA0 */{ {0, 18, 3, BIT(16)}, {2, 8, 4, 0} },
	/* SSPP_DMA1 */{ {0, 21, 3, BIT(18)}, {2, 12, 4, 0} },
	/* SSPP_DMA2 */{ {2, 0, 4, 0}, {2, 16, 4, 0} },
	/* SSPP_DMA3 */{ {2, 4, 4, 0}, {2, 20, 4, 0} },
	/* SSPP_DMA4 */{ {4, 0, 4, 0}, {4, 8, 4, 0} },
	/* SSPP_DMA5 */{ {4, 4, 4, 0}, {4, 12, 4, 0} },
};

static void _sde_shd_hw_ctl_clear_blendstages_in_range(struct sde_shd_hw_ctl *hw_ctl,
						       enum sde_lm lm)
{
	struct sde_hw_ctl *ctx = &hw_ctl->base;
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	u32 mixercfg[4];
	u32 mixermask[4] = {0, 0, 0, 0};
	u32 start = hw_ctl->range.start + SDE_STAGE_0;
	u32 end = start + hw_ctl->range.size;
	int i, j;
	u32 value, mask;
	const struct ctl_sspp_stage_reg_map *sspp_cfg;

	mixercfg[0] = SDE_REG_READ(c, CTL_LAYER(lm));
	mixercfg[1] = SDE_REG_READ(c, CTL_LAYER_EXT(lm));
	mixercfg[2] = SDE_REG_READ(c, CTL_LAYER_EXT2(lm));
	mixercfg[3] = SDE_REG_READ(c, CTL_LAYER_EXT3(lm));

	if (!((mixercfg[0] & ~CTL_MIXER_BORDER_OUT) |
		mixercfg[1] | mixercfg[2] | mixercfg[3]))
		goto end;

	for (i = 1; i < SSPP_MAX - 2; i++) {
		for (j = 0 ; j < CTL_SSPP_MAX_RECTS; j++) {
			sspp_cfg = &sspp_reg_cfg_tbl[i][j];

			if (!sspp_cfg->bits)
				continue;

			if (hw_ctl->mixer_cfg[lm].mixercfg_skip_sspp_mask[j] &
					(1 << i))
				continue;

			mask = (1 << sspp_cfg->bits) - 1;
			value = (mixercfg[sspp_cfg->ext] >> sspp_cfg->start);
			value &= mask;
			if (mixercfg[1] & sspp_cfg->sec_bit_mask)
				value |= (1 << sspp_cfg->bits);

			if (value > start && value <= end) {
				mixermask[sspp_cfg->ext] |= (mask << sspp_cfg->start);
				mixermask[1] |= sspp_cfg->sec_bit_mask;
			}
		}
	}

end:
	hw_ctl->mixer_cfg[lm].mixercfg_mask = mixermask[0];

	hw_ctl->mixer_cfg[lm].mixercfg_ext_mask = mixermask[1];
	hw_ctl->mixer_cfg[lm].mixercfg_ext2_mask = mixermask[2];
	hw_ctl->mixer_cfg[lm].mixercfg_ext3_mask = mixermask[3];
}

static inline void _sde_shd_hw_ctl_clear_all_blendstages(struct sde_hw_ctl *ctx)
{
	struct sde_shd_hw_ctl *hw_ctl;
	int i;

	if (!ctx)
		return;

	hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

	for (i = 0; i < ctx->mixer_count; i++)
		_sde_shd_hw_ctl_clear_blendstages_in_range(hw_ctl, ctx->mixer_hw_caps[i].id);
}

static u32 _sde_shd_update_active_pipes(struct sde_hw_ctl *ctx)
{
	struct sde_shd_hw_ctl *shd_hw_ctl;
	u32 fetch_info, other_shd_info,  val = 0;

	shd_hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

	fetch_info = SDE_REG_READ(&ctx->hw, CTL_FETCH_PIPE_ACTIVE);

	/* clean last pipe_active config for this shd and keep for other shds */
	other_shd_info = fetch_info & (~shd_hw_ctl->old_pipe_active);
	val = other_shd_info | shd_hw_ctl->fetch_active;

	shd_hw_ctl->old_pipe_active = shd_hw_ctl->fetch_active;

	return val;
}

static void _sde_shd_setup_active_pipes(struct sde_hw_ctl *ctx, unsigned long *fetch_active)
{
		int i;
		struct sde_shd_hw_ctl *shd_hw_ctl;
		u32 val = 0;

		shd_hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

		if (!fetch_active) {
			shd_hw_ctl->fetch_active = 0;
			return;
		}

		for (i = 0; i < SSPP_MAX; i++) {
			if (test_bit(i, fetch_active) && fetch_tbl[i] != CTL_INVALID_BIT)
				val |= BIT(fetch_tbl[i]);
		}

		shd_hw_ctl->fetch_active = val;
}

static inline int _stage_offset(struct sde_hw_mixer *ctx, enum sde_stage stage)
{
	const struct sde_lm_sub_blks *sblk = ctx->cap->sblk;

	if (stage == SDE_STAGE_BASE || stage > sblk->maxblendstages)
		return -EINVAL;

	return sblk->blendstage_base[stage - SDE_STAGE_0];
}

static void _sde_shd_hw_ctl_setup_blendstage(struct sde_hw_ctl *ctx, enum sde_lm lm,
					     struct sde_hw_stage_cfg *stage_cfg,
					     bool disable_border)
{
	struct sde_shd_hw_ctl *hw_ctl;
	int i, j;
	int pipes_per_stage;
	u32 pipe_idx, rect_idx;
	const struct ctl_sspp_stage_reg_map *sspp_cfg;
	u32 mixercfg[CTL_NUM_EXT] = {CTL_MIXER_BORDER_OUT, 0, 0, 0};
	u32 mixermask[CTL_NUM_EXT] = {0, 0, 0, 0};
	u32 value, mask, stage_value;

	if (!ctx)
		return;

	hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

	if (test_bit(SDE_MIXER_SOURCESPLIT, &ctx->mixer_hw_caps->features))
		pipes_per_stage = PIPES_PER_STAGE;
	else
		pipes_per_stage = 1;

	_sde_shd_hw_ctl_clear_blendstages_in_range(hw_ctl, lm);

	if (!stage_cfg)
		goto exit;

	for (i = SDE_STAGE_0; i <= hw_ctl->range.size; i++) {
		for (j = 0 ; j < pipes_per_stage; j++) {
			pipe_idx = stage_cfg->stage[i][j];
			if (!pipe_idx || pipe_idx >= SSPP_MAX)
				continue;

			rect_idx = (stage_cfg->multirect_index[i][j] == SDE_SSPP_RECT_1);

			sspp_cfg = &sspp_reg_cfg_tbl[pipe_idx][rect_idx];
			if (!sspp_cfg->bits)
				continue;

			stage_value = i + hw_ctl->range.start + 1;
			mask = (1 << sspp_cfg->bits) - 1;
			value = mask & stage_value;
			mixercfg[sspp_cfg->ext] |= (value << sspp_cfg->start);

			if (stage_value > mask)
				mixercfg[1] |= sspp_cfg->sec_bit_mask;

			mixermask[sspp_cfg->ext] |= (mask << sspp_cfg->start);
			mixermask[1] |= sspp_cfg->sec_bit_mask;
		}
	}

	hw_ctl->mixer_cfg[lm].mixercfg_mask |= mixermask[0];
	hw_ctl->mixer_cfg[lm].mixercfg_ext_mask |= mixermask[1];
	hw_ctl->mixer_cfg[lm].mixercfg_ext2_mask |= mixermask[2];
	hw_ctl->mixer_cfg[lm].mixercfg_ext3_mask |= mixermask[3];

exit:
	hw_ctl->mixer_cfg[lm].mixercfg = mixercfg[0];
	hw_ctl->mixer_cfg[lm].mixercfg_ext = mixercfg[1];
	hw_ctl->mixer_cfg[lm].mixercfg_ext2 = mixercfg[2];
	hw_ctl->mixer_cfg[lm].mixercfg_ext3 = mixercfg[3];
	hw_ctl->mixer_cfg[lm].mixercfg_skip_sspp_mask[0] = 0;
	hw_ctl->mixer_cfg[lm].mixercfg_skip_sspp_mask[1] = 0;
}

static int _sde_shd_setup_intf_cfg_v1(struct sde_hw_ctl *ctx, struct sde_hw_intf_cfg_v1 *cfg)
{
	return 0;
}

#ifdef cwb_dsc
static int _sde_shd_update_cwb_dsc_cfg(struct sde_hw_ctl *ctx, struct sde_hw_intf_cfg_v1 *cfg,
				       bool enable)
{
	int i;
	u32 cwb_active = 0;
	u32 merge_3d_active = 0;
	struct sde_hw_blk_reg_map *c;
	struct sde_shd_hw_ctl *hw_ctl;

	if (!ctx || !cfg)
		return -EINVAL;

	if (!cfg->cwb_count)
		goto dsc;

	c = &ctx->hw;
	hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

	if (enable) {
		for (i = 0; i < cfg->cwb_count; i++) {
			if (cfg->cwb[i])
				cwb_active |= BIT(cfg->cwb[i] - CWB_0);
		}

		for (i = 0; i < cfg->merge_3d_count; i++) {
			if (cfg->merge_3d[i])
				merge_3d_active |=
					BIT(cfg->merge_3d[i] - MERGE_3D_0);
		}

		hw_ctl->cwb_active = cwb_active;
		hw_ctl->merge_3d_active = merge_3d_active;
	}

	hw_ctl->cwb_enable = enable;
	hw_ctl->cwb_changed = true;

	return 0;

dsc:
	if (cfg->dsc_count) {
		struct sde_shd_hw_ctl *hw_ctl;

		hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);
		hw_ctl->dsc_cfg = *cfg;
	}

	return 0;
}

static void _sde_shd_flush_cwb_cfg(struct sde_shd_hw_ctl *hw_ctl)
{
	struct sde_hw_blk_reg_map *c;
	u32 tmp;

	if (!hw_ctl->cwb_changed)
		return;

	c = &hw_ctl->base.hw;

	if (hw_ctl->cwb_enable) {
		SDE_REG_WRITE(c, CTL_WB_ACTIVE, BIT(2));

		tmp = SDE_REG_READ(c, CTL_MERGE_3D_ACTIVE);
		tmp |= hw_ctl->merge_3d_active;
		SDE_REG_WRITE(c, CTL_MERGE_3D_ACTIVE, tmp);

		tmp = SDE_REG_READ(c, CTL_CWB_ACTIVE);
		tmp |= hw_ctl->cwb_active;
		SDE_REG_WRITE(c, CTL_CWB_ACTIVE, tmp);
	} else {
		SDE_REG_WRITE(c, CTL_WB_ACTIVE, 0x0);

		tmp = SDE_REG_READ(c, CTL_MERGE_3D_ACTIVE);
		tmp &= ~hw_ctl->merge_3d_active;
		SDE_REG_WRITE(c, CTL_MERGE_3D_ACTIVE, tmp);

		tmp = SDE_REG_READ(c, CTL_CWB_ACTIVE);
		tmp &= ~hw_ctl->cwb_active;
		SDE_REG_WRITE(c, CTL_CWB_ACTIVE, tmp);
	}

	hw_ctl->cwb_changed = false;
}

static void _sde_shd_flush_hw_dsc_config(struct sde_hw_ctl *ctl_ctx)
{
	struct sde_shd_hw_ctl *hw_ctl;

	hw_ctl = container_of(ctl_ctx, struct sde_shd_hw_ctl, base);

	if (hw_ctl->orig && hw_ctl->orig->ops.update_intf_cfg)
		hw_ctl->orig->ops.update_intf_cfg(ctl_ctx, &hw_ctl->dsc_cfg, true);
}
#endif

static void _sde_shd_flush_hw_pipe_active(struct sde_hw_ctl *ctx)
{
	struct sde_shd_hw_ctl *hw_ctl;
	u32 val = 0;

	if (!ctx)
		return;

	hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);
	val = _sde_shd_update_active_pipes(ctx);
	SDE_REG_WRITE(&ctx->hw, CTL_FETCH_PIPE_ACTIVE, val);
}

static void _sde_shd_flush_hw_ctl(struct sde_hw_ctl *ctx)
{
	struct sde_shd_hw_ctl *hw_ctl;
	struct sde_hw_blk_reg_map *c;
	u32 mixercfg, mixercfg_ext;
	u32 mixercfg_ext2, mixercfg_ext3;
	int i;

	hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

	hw_ctl->old_mask = hw_ctl->flush_mask;

	hw_ctl->flush_mask = ctx->flush.pending_flush_mask;

	hw_ctl->flush_mask &= CTL_SSPP_FLUSH_MASK;

	c = &ctx->hw;

	for (i = 0; i < ctx->mixer_count; i++) {
		int lm = ctx->mixer_hw_caps[i].id;

		mixercfg = SDE_REG_READ(c, CTL_LAYER(lm));
		mixercfg_ext = SDE_REG_READ(c, CTL_LAYER_EXT(lm));
		mixercfg_ext2 = SDE_REG_READ(c, CTL_LAYER_EXT2(lm));
		mixercfg_ext3 = SDE_REG_READ(c, CTL_LAYER_EXT3(lm));

		mixercfg &= ~hw_ctl->mixer_cfg[lm].mixercfg_mask;
		mixercfg_ext &= ~hw_ctl->mixer_cfg[lm].mixercfg_ext_mask;
		mixercfg_ext2 &= ~hw_ctl->mixer_cfg[lm].mixercfg_ext2_mask;
		mixercfg_ext3 &= ~hw_ctl->mixer_cfg[lm].mixercfg_ext3_mask;

		mixercfg |= hw_ctl->mixer_cfg[lm].mixercfg;
		mixercfg_ext |= hw_ctl->mixer_cfg[lm].mixercfg_ext;
		mixercfg_ext2 |= hw_ctl->mixer_cfg[lm].mixercfg_ext2;
		mixercfg_ext3 |= hw_ctl->mixer_cfg[lm].mixercfg_ext3;

		SDE_REG_WRITE(c, CTL_LAYER(lm), mixercfg);
		SDE_REG_WRITE(c, CTL_LAYER_EXT(lm), mixercfg_ext);
		SDE_REG_WRITE(c, CTL_LAYER_EXT2(lm), mixercfg_ext2);
		SDE_REG_WRITE(c, CTL_LAYER_EXT3(lm), mixercfg_ext3);
	}
#ifdef cwb_dsc
	_sde_shd_flush_cwb_cfg(hw_ctl);
#endif
}

static void _sde_shd_setup_blend_config(struct sde_hw_mixer *ctx, u32 stage, u32 fg_alpha,
					u32 bg_alpha, u32 blend_op)
{
	struct sde_shd_hw_mixer *hw_lm;
	struct sde_shd_mixer_cfg *cfg;

	if (!ctx)
		return;

	hw_lm = container_of(ctx, struct sde_shd_hw_mixer, base);

	cfg = &hw_lm->cfg[stage + hw_lm->range.start];

	cfg->fg_alpha = fg_alpha;
	cfg->bg_alpha = bg_alpha;
	cfg->blend_op = blend_op;
	cfg->dirty = true;
}

static void _sde_shd_setup_dim_layer(struct sde_hw_mixer *ctx, struct sde_hw_dim_layer *dim_layer)
{
	struct sde_shd_hw_mixer *hw_lm;
	struct sde_hw_dim_layer dim_layer2;

	if (!ctx)
		return;

	hw_lm = container_of(ctx, struct sde_shd_hw_mixer, base);

	dim_layer2 = *dim_layer;
	dim_layer2.stage += hw_lm->range.start;
	dim_layer2.rect.x += hw_lm->roi.x;
	dim_layer2.rect.y += hw_lm->roi.y;

	hw_lm->cfg[dim_layer2.stage].dim_layer = dim_layer2;
	hw_lm->cfg[dim_layer2.stage].dim_layer_enable = true;
}

static void _sde_shd_clear_dim_layer(struct sde_hw_mixer *ctx)
{
	struct sde_shd_hw_mixer *hw_lm;
	int i;

	if (!ctx)
		return;

	hw_lm = container_of(ctx, struct sde_shd_hw_mixer, base);

	for (i = SDE_STAGE_0; i < SDE_STAGE_MAX; i++)
		hw_lm->cfg[i].dim_layer_enable = false;
}

static inline void _sde_shd_setup_mixer_out(struct sde_hw_mixer *ctx, struct sde_hw_mixer_cfg *cfg)
{
	/* do nothing */
}

static void _sde_shd_flush_hw_lm(struct sde_hw_mixer *ctx)
{
	struct sde_shd_hw_mixer *hw_lm;
	struct sde_hw_blk_reg_map *c = &ctx->hw;
	int stage_off, i;
	u32 reset = BIT(16), val;
	int start, end;

	if (!ctx)
		return;

	hw_lm = container_of(ctx, struct sde_shd_hw_mixer, base);

	start = SDE_STAGE_0 + hw_lm->range.start;
	end = start + hw_lm->range.size;
	reset = ~reset;

	for (i = start; i < end; i++) {
		stage_off = _stage_offset(ctx, i);
		if (WARN_ON(stage_off < 0))
			return;

		if (hw_lm->cfg[i].dim_layer_enable) {
			hw_lm->orig->ops.setup_dim_layer(ctx,
				&hw_lm->cfg[i].dim_layer);
		} else {
			val = SDE_REG_READ(c, LM_BLEND0_OP + stage_off);
			val &= reset;
			SDE_REG_WRITE(c, LM_BLEND0_OP + stage_off, val);
		}

		if (hw_lm->cfg[i].dirty) {
			hw_lm->orig->ops.setup_blend_config(ctx, i,
				hw_lm->cfg[i].fg_alpha,
				hw_lm->cfg[i].bg_alpha,
				hw_lm->cfg[i].blend_op);
			hw_lm->cfg[i].dirty = false;
		}
	}
}

void sde_shd_hw_flush(struct sde_hw_ctl *ctl_ctx, struct sde_hw_mixer *lm_ctx[MAX_MIXERS_PER_CRTC],
		      int lm_num)
{
	struct sde_hw_blk_reg_map *c;
	unsigned long lock_flags;
	int i;

	c = &ctl_ctx->hw;

	spin_lock_irqsave(&hw_ctl_lock, lock_flags);
	SDE_REG_WRITE(c, CTL_FLUSH_MASK, FLUSH_MASK_ALL);
	_sde_shd_flush_hw_pipe_active(ctl_ctx);
	_sde_shd_flush_hw_ctl(ctl_ctx);

	for (i = 0; i < lm_num; i++)
		_sde_shd_flush_hw_lm(lm_ctx[i]);

#ifdef cwb_dsc
	_sde_shd_flush_hw_dsc_config(ctl_ctx);
#endif

	if (ctl_ctx->ops.trigger_flush)
		ctl_ctx->ops.trigger_flush(ctl_ctx);

	SDE_REG_WRITE(c, CTL_FLUSH_MASK, 0);
	spin_unlock_irqrestore(&hw_ctl_lock, lock_flags);
}

void sde_shd_hw_ctl_init_op(struct sde_hw_ctl *ctx)
{
	ctx->ops.clear_all_blendstages = _sde_shd_hw_ctl_clear_all_blendstages;
	ctx->ops.setup_blendstage = _sde_shd_hw_ctl_setup_blendstage;
	ctx->ops.setup_intf_cfg_v1 = _sde_shd_setup_intf_cfg_v1;
#ifdef cwb_dsc
	ctx->ops.update_intf_cfg = _sde_shd_update_cwb_dsc_cfg;
#endif
	ctx->ops.set_active_pipes = _sde_shd_setup_active_pipes;
}

void sde_shd_hw_lm_init_op(struct sde_hw_mixer *ctx)
{
	ctx->ops.setup_blend_config = _sde_shd_setup_blend_config;
	ctx->ops.setup_dim_layer = _sde_shd_setup_dim_layer;
	ctx->ops.setup_mixer_out = _sde_shd_setup_mixer_out;
	ctx->ops.clear_dim_layer = _sde_shd_clear_dim_layer;
}

void sde_shd_hw_skip_sspp_clear(struct sde_hw_ctl *ctx, enum sde_sspp sspp, int multirect_idx)
{
	struct sde_shd_hw_ctl *hw_ctl;
	int i;
	int lm;

	if (!ctx)
		return;

	hw_ctl = container_of(ctx, struct sde_shd_hw_ctl, base);

	for (i = 0; i < ctx->mixer_count; i++) {
		lm = ctx->mixer_hw_caps[i].id;
		hw_ctl->mixer_cfg[lm].mixercfg_skip_sspp_mask[multirect_idx] |= (1 << sspp);
	}
}

