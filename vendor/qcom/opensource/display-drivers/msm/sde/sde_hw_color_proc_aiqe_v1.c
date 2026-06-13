// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <drm/msm_drm_aiqe.h>
#include "sde_hw_util.h"
#include "sde_reg_dma.h"
#include "sde_dbg.h"
#include "sde_hw_reg_dma_v1_color_proc.h"
#include "sde_hw_color_proc_aiqe_v1.h"
#include "sde_aiqe_common.h"
#include "sde_crtc.h"


static void sde_setup_aiqe_common_v1(struct sde_hw_dspp *ctx, void *cfg,
					struct sde_aiqe_top_level *aiqe_top)
{
	struct aiqe_reg_common aiqe_common;
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	u32 aiqe_base = ctx->cap->sblk->aiqe.base;

	aiqe_get_common_values(hw_cfg, aiqe_top, &aiqe_common);

	SDE_REG_WRITE(&ctx->hw, aiqe_base, aiqe_common.config);
	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x4, aiqe_common.merge);
	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x14,
			((aiqe_common.width & 0xFFF) << 16) | (aiqe_common.height & 0xFFF));
	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x3EC, 0);
	SDE_EVT32(aiqe_common.config, aiqe_common.merge,
			 ((aiqe_common.width & 0xFFF) << 16), (aiqe_common.height & 0xFFF));
}

static int _reg_dmav1_aiqe_write_top_level_v1(struct sde_reg_dma_setup_ops_cfg *dma_cfg,
		struct sde_hw_dspp *ctx, struct sde_hw_cp_cfg *hw_cfg,
		struct sde_hw_reg_dma_ops *dma_ops,
		struct sde_aiqe_top_level *aiqe_top)
{
	struct aiqe_reg_common aiqe_common;
	u32 values[4];
	u32 base = ctx->hw.blk_off + ctx->cap->sblk->aiqe.base;
	int rc = 0;

	aiqe_get_common_values(hw_cfg, aiqe_top, &aiqe_common);

	values[0] = aiqe_common.config;
	values[1] = aiqe_common.merge;
	values[2] = ((aiqe_common.width & 0xFFF) << 16) | (aiqe_common.height & 0xFFF);
	values[3] = 0;
	REG_DMA_SETUP_OPS(*dma_cfg, base,
			&values[0], 2 * sizeof(u32), REG_BLK_WRITE_SINGLE, 0, 0, 0);
	rc = dma_ops->setup_payload(dma_cfg);
	if (rc) {
		DRM_ERROR("write top part 1 failed ret %d\n", rc);
		return rc;
	}

	REG_DMA_SETUP_OPS(*dma_cfg, base + 0x14,
			&values[2], sizeof(u32), REG_SINGLE_WRITE, 0, 0, 0);
	rc = dma_ops->setup_payload(dma_cfg);
	if (rc) {
		DRM_ERROR("write top part 2 failed ret %d\n", rc);
		return rc;
	}

	REG_DMA_SETUP_OPS(*dma_cfg, base + 0x3EC,
			&values[3], sizeof(u32), REG_SINGLE_WRITE, 0, 0, 0);
	rc = dma_ops->setup_payload(dma_cfg);
	if (rc)
		DRM_ERROR("write top part 3 failed ret %d\n", rc);

	return rc;
}

void sde_reset_mdnie_art(struct sde_hw_dspp *ctx)
{
	u32 aiqe_base = 0;

	if (!ctx) {
		DRM_ERROR("invalid parameters ctx %pK\n", ctx);
		return;
	}

	aiqe_base = ctx->cap->sblk->aiqe.base;
	if (!aiqe_base) {
		DRM_DEBUG_DRIVER("AIQE not supported on DSPP idx %d", ctx->idx);
		return;
	}

	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x3dc, 0x10);
	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x100, 0);
}

int sde_read_copr_status(struct sde_hw_dspp *ctx, struct drm_msm_copr_status *copr_status)
{
	uint32_t status_off;
	int i;

	if (!ctx || !copr_status)
		return -EINVAL;

	status_off = ctx->cap->sblk->aiqe.base + 0x344;
	for (i = 0; i < AIQE_COPR_STATUS_LEN; i++)
		copr_status->status[i] = SDE_REG_READ(&ctx->hw, status_off + 4 * i);

	return 0;
}

void sde_setup_mdnie_art_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct drm_msm_mdnie_art *mdnie_art = NULL;
	u32 art_value, art_id, aiqe_base = 0;

	if (!ctx || !cfg) {
		DRM_ERROR("invalid parameters ctx %pK cfg %pK\n", ctx, cfg);
		return;
	}

	aiqe_base = ctx->cap->sblk->aiqe.base;
	if (!aiqe_base) {
		DRM_ERROR("AIQE not supported on DSPP idx %d", ctx->idx);
		return;
	}

	mdnie_art = (struct drm_msm_mdnie_art *)(hw_cfg->payload);
	if (mdnie_art && hw_cfg->len != sizeof(struct drm_msm_mdnie_art)) {
		DRM_ERROR("invalid size of payload len %d exp %zd\n",
				hw_cfg->len, sizeof(struct drm_msm_mdnie_art));
		return;
	}

	if (!mdnie_art || !(mdnie_art->param & BIT(0))) {
		DRM_DEBUG_DRIVER("Disable MDNIE ART feature\n");
		sde_setup_aiqe_common_v1(ctx, hw_cfg, aiqe_top);
		SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x100, 0);
		LOG_FEATURE_OFF;
		return;
	}

	sde_setup_aiqe_common_v1(ctx, hw_cfg, aiqe_top);
	art_id = ~((SDE_REG_READ(&ctx->hw, aiqe_base + 0x100) & BIT(1)) >> 1) & BIT(0);
	art_value = (mdnie_art->param & 0xFFFFFF01) | (art_id << 1);
	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x100, art_value);
	LOG_FEATURE_ON;
}

void sde_setup_copr_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct drm_msm_copr *copr_data = NULL;
	u32 data, i, aiqe_base = 0;

	if (!ctx || !cfg) {
		DRM_ERROR("invalid parameters ctx %pK cfg %pK\n", ctx, cfg);
		return;
	}

	aiqe_base = ctx->cap->sblk->aiqe.base;
	if (!aiqe_base) {
		DRM_DEBUG_DRIVER("AIQE not supported on DSPP idx %d", ctx->idx);
		return;
	}

	copr_data = (struct drm_msm_copr *)(hw_cfg->payload);
	if (copr_data && hw_cfg->len != sizeof(struct drm_msm_copr)) {
		DRM_ERROR("invalid size of payload len %d exp %zd\n",
				hw_cfg->len, sizeof(struct drm_msm_copr));
		return;
	}

	if (!copr_data || !(copr_data->param[0] & BIT(0))) {
		DRM_DEBUG_DRIVER("Disable COPR feature\n");
		sde_setup_aiqe_common_v1(ctx, hw_cfg, aiqe_top);
		SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x300, 0);
		LOG_FEATURE_OFF;
		return;
	}

	sde_setup_aiqe_common_v1(ctx, hw_cfg, aiqe_top);
	for (i = 0; i < AIQE_COPR_PARAM_LEN; i++) {
		data = copr_data->param[i];
		SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x300 + (i * 4), data);
	}
	LOG_FEATURE_ON;
}

void sde_setup_mdnie_psr(struct sde_hw_dspp *ctx)
{
	u32 aiqe_base = 0;

	if (!ctx) {
		DRM_ERROR("invalid parameters ctx %pK\n", ctx);
		return;
	}

	aiqe_base = ctx->cap->sblk->aiqe.base;
	if (!aiqe_base) {
		DRM_DEBUG_DRIVER("AIQE not supported on DSPP idx %d", ctx->idx);
		return;
	}

	SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x3f4, 1);
}

static void _mdnie_disable_v1(struct sde_reg_dma_setup_ops_cfg *dma_cfg,
		struct sde_hw_dspp *ctx, struct sde_hw_cp_cfg *hw_cfg,
		struct sde_hw_reg_dma_ops *dma_ops,
		struct sde_aiqe_top_level *aiqe_top)
{
	int rc = 0;
	u32 value = 0;
	u32 base = ctx->hw.blk_off + ctx->cap->sblk->aiqe.base;
	struct sde_reg_dma_kickoff_cfg dma_kickoff;

	rc = _reg_dmav1_aiqe_write_top_level_v1(dma_cfg, ctx, hw_cfg, dma_ops, aiqe_top);
	if (rc)
		return;

	REG_DMA_SETUP_OPS(*dma_cfg, base + 0x104,
		&value, sizeof(u32), REG_SINGLE_WRITE, 0, 0, 0);
	rc = dma_ops->setup_payload(dma_cfg);
	if (rc) {
		DRM_ERROR("write decode select failed ret %d\n", rc);
		return;
	}

	REG_DMA_SETUP_KICKOFF(dma_kickoff, hw_cfg->ctl, dma_cfg->dma_buf,
			REG_DMA_WRITE, DMA_CTL_QUEUE0,
			WRITE_IMMEDIATE, AIQE_MDNIE);
	rc = dma_ops->kick_off(&dma_kickoff, ctx->dpu_idx);
	if (rc)
		DRM_ERROR("failed to kick off ret %d\n", rc);
	else
		LOG_FEATURE_OFF;
}

void reg_dmav1_setup_mdnie_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	struct drm_msm_mdnie *mdnie_data;
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct sde_hw_reg_dma_ops *dma_ops;
	struct sde_reg_dma_setup_ops_cfg dma_write_cfg;
	struct sde_reg_dma_kickoff_cfg kick_off;
	int rc = 0;
	u32 aiqe_base = 0;

	if (!ctx || !cfg) {
		DRM_ERROR("invalid parameters ctx %pK cfg %pK\n", ctx, cfg);
		return;
	}

	if (!ctx->cap->sblk->aiqe.base) {
		DRM_DEBUG_DRIVER("AIQE not supported on DSPP idx %d", ctx->idx);
		return;
	}

	rc = reg_dma_dspp_check(ctx, cfg, AIQE_MDNIE);
	if (rc)
		return;

	aiqe_base = ctx->hw.blk_off + ctx->cap->sblk->aiqe.base;
	dma_ops = sde_reg_dma_get_ops(ctx->dpu_idx);
	dma_ops->reset_reg_dma_buf(dspp_buf[AIQE_MDNIE][ctx->idx][ctx->dpu_idx]);
	REG_DMA_INIT_OPS(dma_write_cfg, MDSS, AIQE_MDNIE,
		dspp_buf[AIQE_MDNIE][ctx->idx][ctx->dpu_idx]);
	REG_DMA_SETUP_OPS(dma_write_cfg, 0, NULL, 0, HW_BLK_SELECT, 0, 0, 0);
	rc = dma_ops->setup_payload(&dma_write_cfg);
	if (rc) {
		DRM_ERROR("write decode select failed ret %d\n", rc);
		return;
	}

	mdnie_data = hw_cfg->payload;
	if (mdnie_data && hw_cfg->len != sizeof(struct drm_msm_mdnie)) {
		DRM_ERROR("invalid sz of payload len %d exp %zd\n",
				hw_cfg->len, sizeof(struct drm_msm_mdnie));
		return;
	}

	if (!mdnie_data || !(mdnie_data->param[0] & BIT(0))) {
		DRM_DEBUG_DRIVER("Disable MDNIE feature\n");
		_mdnie_disable_v1(&dma_write_cfg, ctx, hw_cfg, dma_ops, aiqe_top);
		return;
	}

	rc = _reg_dmav1_aiqe_write_top_level_v1(&dma_write_cfg, ctx, hw_cfg, dma_ops, aiqe_top);
	if (rc)
		return;

	REG_DMA_SETUP_OPS(dma_write_cfg, aiqe_base + 0x104, mdnie_data->param,
			AIQE_MDNIE_PARAM_LEN * sizeof(u32), REG_BLK_WRITE_SINGLE, 0, 0, 0);
	rc = dma_ops->setup_payload(&dma_write_cfg);
	if (rc) {
		SDE_ERROR("mdnie dma write failed ret %d\n", rc);
		return;
	}

	REG_DMA_SETUP_KICKOFF(kick_off, hw_cfg->ctl,
			dspp_buf[AIQE_MDNIE][ctx->idx][ctx->dpu_idx],
			REG_DMA_WRITE, DMA_CTL_QUEUE0, WRITE_IMMEDIATE,
			AIQE_MDNIE);

	rc = dma_ops->kick_off(&kick_off, ctx->dpu_idx);
	if (rc) {
		DRM_ERROR("failed to kick off ret %d\n", rc);
		return;
	}

	LOG_FEATURE_ON;
}

int sde_validate_aiqe_ssrc_data_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	int rc = 0;
	struct drm_msm_ssrc_data *data;
	struct sde_aiqe_top_level *aiqe_tl = aiqe_top;
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct aiqe_reg_common aiqe_common;
	size_t region_count = 0, index = 0;
	size_t max_regions = 0;

	if (!hw_cfg || !ctx || !aiqe_tl)
		return -EINVAL;
	else if (!hw_cfg->payload || ctx->idx != hw_cfg->dspp[0]->idx)
		return 0;

	data = hw_cfg->payload;
	if (data->data_size == 0) {
		DRM_ERROR("Data size must be greater than 0\n");
		return -EINVAL;
	} else if (data->data_size > AIQE_SSRC_DATA_LEN) {
		DRM_ERROR("Data size exceeds max data size. Max - %d\t Actual - %u\n",
				AIQE_SSRC_DATA_LEN, data->data_size);
		return -EINVAL;
	}

	aiqe_get_common_values(hw_cfg, aiqe_tl, &aiqe_common);
	if (aiqe_common.config & BIT(1))
		max_regions = 2;
	else
		max_regions = 4;

	while (index < data->data_size) {
		region_count++;
		index += data->data[index] + 1;
		if (region_count > max_regions) {

			DRM_ERROR("Region count exceeds max. Max - %zu\t Actual - %zu\n",
					max_regions, region_count);
			rc = -EINVAL;
			break;
		}
	}

	if (index > data->data_size) {
		DRM_ERROR("Region data exceeds reported size. Reported - %u\t Actual - %zu\n",
				data->data_size, index);
		rc = -EINVAL;
	}

	return rc;
}

static void _aiqe_ssrc_config_off_v1(struct sde_reg_dma_setup_ops_cfg *dma_cfg,
		struct sde_hw_dspp *ctx, struct sde_hw_cp_cfg *hw_cfg,
		struct sde_hw_reg_dma_ops *dma_ops,
		struct sde_aiqe_top_level *aiqe_top)
{
	int rc = 0;
	u32 value = 0;
	u32 base = ctx->hw.blk_off + ctx->cap->sblk->aiqe.base;
	struct sde_reg_dma_kickoff_cfg dma_kickoff;

	// Error message handled in function
	rc = _reg_dmav1_aiqe_write_top_level_v1(dma_cfg, ctx, hw_cfg, dma_ops, aiqe_top);
	if (rc)
		return;

	REG_DMA_SETUP_OPS(*dma_cfg, base + 0x380,
		&value, sizeof(u32), REG_SINGLE_WRITE, 0, 0, 0);
	rc = dma_ops->setup_payload(dma_cfg);
	if (rc) {
		DRM_ERROR("write SSRC off failed ret %d\n", rc);
		return;
	}

	REG_DMA_SETUP_KICKOFF(dma_kickoff, hw_cfg->ctl, dma_cfg->dma_buf,
			REG_DMA_WRITE, DMA_CTL_QUEUE0,
			WRITE_IMMEDIATE, AIQE_SSRC_CONFIG);
	rc = dma_ops->kick_off(&dma_kickoff, ctx->dpu_idx);
	if (rc)
		DRM_ERROR("failed to kick off ret %d\n", rc);
	else
		LOG_FEATURE_OFF;
}

void reg_dmav1_setup_aiqe_ssrc_config_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	struct drm_msm_ssrc_config *ssrc_config;
	struct sde_aiqe_top_level *aiqe_tl = aiqe_top;
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct sde_hw_reg_dma_ops *dma_ops;
	struct sde_reg_dma_buffer *buf;
	struct sde_reg_dma_setup_ops_cfg dma_cfg;
	struct sde_reg_dma_kickoff_cfg dma_kickoff;
	int rc = -EINVAL;
	u32 base;

	rc = reg_dma_dspp_check(ctx, cfg, AIQE_SSRC_CONFIG);
	if (rc)
		return;

	if (!ctx->cap->sblk->aiqe.base) {
		DRM_DEBUG_DRIVER("AIQE not present on DSPP idx %d", ctx->idx);
		return;
	}

	base = ctx->hw.blk_off + ctx->cap->sblk->aiqe.base;
	dma_ops = sde_reg_dma_get_ops(ctx->dpu_idx);
	buf = dspp_buf[AIQE_SSRC_CONFIG][ctx->idx][ctx->dpu_idx];
	dma_ops->reset_reg_dma_buf(buf);
	REG_DMA_INIT_OPS(dma_cfg, MDSS, AIQE_SSRC_CONFIG, buf);
	REG_DMA_SETUP_OPS(dma_cfg, 0, NULL, 0, HW_BLK_SELECT, 0, 0, 0);
	rc = dma_ops->setup_payload(&dma_cfg);
	if (rc) {
		DRM_ERROR("write decode select failed ret %d\n", rc);
		return;
	}

	ssrc_config = hw_cfg->payload;
	if (!ssrc_config || (ssrc_config->config[0] & BIT(0)) == 0) {
		_aiqe_ssrc_config_off_v1(&dma_cfg, ctx, hw_cfg, dma_ops, aiqe_tl);
		return;
	}

	// Error message handled in function
	rc = _reg_dmav1_aiqe_write_top_level_v1(&dma_cfg, ctx, hw_cfg, dma_ops, aiqe_tl);
	if (rc)
		return;

	REG_DMA_SETUP_OPS(dma_cfg, base + 0x380,
		ssrc_config->config, AIQE_SSRC_PARAM_LEN * sizeof(u32),
		REG_BLK_WRITE_SINGLE, 0, 0, 0);
	rc = dma_ops->setup_payload(&dma_cfg);
	if (rc) {
		DRM_ERROR("write config failed ret %d\n", rc);
		return;
	}

	REG_DMA_SETUP_KICKOFF(dma_kickoff, hw_cfg->ctl, dma_cfg.dma_buf,
			REG_DMA_WRITE, DMA_CTL_QUEUE0,
			WRITE_IMMEDIATE, AIQE_SSRC_CONFIG);
	rc = dma_ops->kick_off(&dma_kickoff, ctx->dpu_idx);
	if (rc)
		DRM_ERROR("failed to kick off ret %d\n", rc);
	else
		LOG_FEATURE_ON;
}

void reg_dmav1_setup_aiqe_ssrc_data_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	struct drm_msm_ssrc_data *ssrc_data;
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct sde_hw_reg_dma_ops *dma_ops;
	struct sde_reg_dma_buffer *buf;
	struct sde_reg_dma_setup_ops_cfg dma_cfg;
	struct sde_reg_dma_kickoff_cfg dma_kickoff;
	int rc = -EINVAL;
	size_t index = 0;
	u32 base;

	rc = reg_dma_dspp_check(ctx, cfg, AIQE_SSRC_DATA);
	if (rc)
		return;

	if (!ctx->cap->sblk->aiqe.base) {
		DRM_DEBUG_DRIVER("MDNIE not present on DSPP idx %d", ctx->idx);
		return;
	}

	base = ctx->hw.blk_off + ctx->cap->sblk->aiqe.base;
	dma_ops = sde_reg_dma_get_ops(ctx->dpu_idx);
	buf = dspp_buf[AIQE_SSRC_DATA][ctx->idx][ctx->dpu_idx];
	dma_ops->reset_reg_dma_buf(buf);
	REG_DMA_INIT_OPS(dma_cfg, MDSS, AIQE_SSRC_DATA, buf);
	REG_DMA_SETUP_OPS(dma_cfg, 0, NULL, 0, HW_BLK_SELECT, 0, 0, 0);
	rc = dma_ops->setup_payload(&dma_cfg);
	if (rc) {
		DRM_ERROR("write decode select failed ret %d\n", rc);
		return;
	}

	ssrc_data = hw_cfg->payload;
	if (!ssrc_data) {
		LOG_FEATURE_OFF;
		return;
	}

	while (index < ssrc_data->data_size) {
		size_t region_size = ssrc_data->data[index++];

		REG_DMA_SETUP_OPS(dma_cfg, base + 0x0C,
				&ssrc_data->data[index], sizeof(u32), REG_SINGLE_WRITE, 0, 0, 0);
		rc = dma_ops->setup_payload(&dma_cfg);
		if (rc) {
			DRM_ERROR("write data phase 1 failed ret %d\n", rc);
			return;
		}

		REG_DMA_SETUP_OPS(dma_cfg, base + 0x10,
				&ssrc_data->data[index + 1], (region_size - 1) * sizeof(u32),
				REG_BLK_WRITE_INC, 0, 0, 0);
		rc = dma_ops->setup_payload(&dma_cfg);
		if (rc) {
			DRM_ERROR("write data phase 2 failed ret %d\n", rc);
			return;
		}

		index += region_size;
	}

	REG_DMA_SETUP_KICKOFF(dma_kickoff, hw_cfg->ctl, buf,
			REG_DMA_WRITE, DMA_CTL_QUEUE0, WRITE_IMMEDIATE, AIQE_SSRC_DATA);
	rc = dma_ops->kick_off(&dma_kickoff, ctx->dpu_idx);
	if (rc)
		DRM_ERROR("failed to kick off ret %d\n", rc);
	else
		LOG_FEATURE_ON;
}

int sde_check_ai_scaler_v1(struct sde_hw_dspp *ctx, void *cfg)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct drm_msm_ai_scaler *ai_scaler_cfg = NULL;

	if (!ctx || !cfg) {
		DRM_ERROR("invalid parameters ctx %pK cfg %pK\n", ctx, cfg);
		return -EINVAL;
	}

	if (hw_cfg->payload || ctx->idx != hw_cfg->dspp[0]->idx)
		return 0;

	/* check for number of mixers */
	if (hw_cfg->num_of_mixers > 2) {
		DRM_ERROR("invalid mixer count %d\n", hw_cfg->num_of_mixers);
		return -EINVAL;
	}

	if (!hw_cfg->payload)
		return 0;

	if (hw_cfg->len != sizeof(struct drm_msm_ai_scaler)) {
		DRM_ERROR("invalid size of payload len %d exp %zd\n",
				hw_cfg->len, sizeof(struct drm_msm_ai_scaler));
		return -EINVAL;
	}

	ai_scaler_cfg = (struct drm_msm_ai_scaler *)(hw_cfg->payload);

	/* check for scaler input resolution */
	if (ai_scaler_cfg->src_w != hw_cfg->displayh ||
		ai_scaler_cfg->src_h != hw_cfg->displayv) {
		DRM_ERROR("invalid scaler input resolution %d %d\n",
				ai_scaler_cfg->src_w, ai_scaler_cfg->src_h);
		return -EINVAL;
	}

	/* check for scaler output resolution */
	if (ai_scaler_cfg->dst_w != hw_cfg->panel_width ||
		ai_scaler_cfg->dst_h != hw_cfg->panel_height) {
		DRM_ERROR("invalid scaler output resolution %d %d\n",
			ai_scaler_cfg->dst_w, ai_scaler_cfg->dst_h);
		return -EINVAL;
	}

	return 0;
}

int sde_setup_ai_scaler_v1(struct sde_hw_dspp *ctx, void *cfg)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	u32 ai_scaler_base, width, height, size, i, offset = 0;
	struct drm_msm_ai_scaler *ai_scaler_cfg = NULL;

	if (!ctx || !cfg) {
		DRM_ERROR("invalid parameters ctx %pK cfg %pK\n", ctx, cfg);
		return -EINVAL;
	}

	ai_scaler_base = ctx->cap->sblk->ai_scaler.base;
	if (!ai_scaler_base) {
		DRM_DEBUG_DRIVER("AI Scaler not supported on DSPP idx %d", ctx->idx);
		return -EINVAL;
	}

	/* program ai scaler for single dspp instance */
	if (ctx->idx != hw_cfg->dspp[0]->idx) {
		DRM_DEBUG_DRIVER("AI Scaler need not be programmed on %d", ctx->idx);
		return 0;
	}

	if (!hw_cfg->payload) {
		LOG_FEATURE_OFF;
		DRM_DEBUG_DRIVER("Disable AI SCALER feature\n");
		SDE_REG_WRITE(&ctx->hw, ai_scaler_base + 0x4, 0);
		return 0;
	}

	if (hw_cfg->len != sizeof(struct drm_msm_ai_scaler)) {
		DRM_ERROR("invalid size of payload len %d exp %zd\n",
				hw_cfg->len, sizeof(struct drm_msm_ai_scaler));
		return -EINVAL;
	}

	ai_scaler_cfg = (struct drm_msm_ai_scaler *)(hw_cfg->payload);

	/* program config register */
	SDE_REG_WRITE(&ctx->hw, ai_scaler_base + 0x4, ai_scaler_cfg->config);

	/* program merge control config register */
	if (hw_cfg->num_of_mixers == 1)
		SDE_REG_WRITE(&ctx->hw, ai_scaler_base + 0x8, 0);
	else if (hw_cfg->num_of_mixers == 2)
		SDE_REG_WRITE(&ctx->hw, ai_scaler_base + 0x8, 1);
	else {
		DRM_ERROR("invalid number of mixers %d\n", hw_cfg->num_of_mixers);
		return -EINVAL;
	}

	/* program input size register */
	width = (ai_scaler_cfg->src_w) & 0xFFF;
	height = (ai_scaler_cfg->src_h) & 0xFFF;
	size = width | height << 0x10;
	SDE_REG_WRITE(&ctx->hw, ai_scaler_base + 0xC, size);

	/* program output size register */
	width = (ai_scaler_cfg->dst_w) & 0xFFF;
	height = (ai_scaler_cfg->dst_h) & 0xFFF;
	size = width | height << 0x10;
	SDE_REG_WRITE(&ctx->hw, ai_scaler_base + 0x10, size);

	/* program parameter registers only when weight selection bit is set */
	if (!(ai_scaler_cfg->config & BIT(18)))
		goto end;

	offset = 0x3C;
	for (i = 0; i < AIQE_AI_SCALER_PARAM_LEN; i++) {
		SDE_DEBUG("param[%d] = 0x%X\n", i, ai_scaler_cfg->param[i]);
		SDE_REG_WRITE(&ctx->hw, ai_scaler_base + offset, ai_scaler_cfg->param[i]);
		offset += 0x4;
	}

end:
	LOG_FEATURE_ON;
	SDE_DEBUG("Enable AI Scaler: src_w:0x%X src_h:0x%X dst_w:0x%X dst_h:0x%X\n",
			ai_scaler_cfg->src_w, ai_scaler_cfg->src_h, ai_scaler_cfg->dst_w,
			ai_scaler_cfg->dst_h);
	SDE_EVT32(hw_cfg->num_of_mixers, ai_scaler_cfg->config, ai_scaler_cfg->src_w,
			ai_scaler_cfg->src_h, ai_scaler_cfg->dst_w, ai_scaler_cfg->dst_h);
	return 0;
}

static bool valid_abc_main_layer_cfg_v1(struct drm_msm_abc *aiqe_abc,
	struct sde_hw_cp_cfg *hw_cfg)
{
	u32 h, w, div = 1, tempw, temph;

	if ((aiqe_abc->param[0] & 0x1) == 0 || !hw_cfg->skip_planes[SB_PLANE_REAL].valid) {
		DRM_INFO("aiqe_abc state %d valid blend plane %d\n", (aiqe_abc->param[0] & 0x1),
				hw_cfg->skip_planes[SB_PLANE_REAL].valid);
		return false;
	}

	if (hw_cfg->panel_width == 1080 && (hw_cfg->panel_height % 3 == 0))
		div = 3;
	else if (hw_cfg->panel_width % 4 == 0 && hw_cfg->panel_height % 4 == 0)
		div = 4;
	else
		return false;

	h = hw_cfg->panel_height / div;
	w = hw_cfg->panel_width / div;

	temph = (aiqe_abc->param[1] >> 16) & (((1 << 12) - 1));
	tempw = aiqe_abc->param[1] & ((1 << 12) - 1);

	if (w != tempw || h != temph) {
		DRM_ERROR("invalid plane param h %d w %d exp h %d exp w %d\n", temph,
		temph, h, w);
		return false;
	}

	w = (w * 3) / 4;
	if (h != hw_cfg->skip_planes[SB_PLANE_REAL].plane_h ||
		w != hw_cfg->skip_planes[SB_PLANE_REAL].plane_w) {
		DRM_ERROR("real plane invalid plane h %d w %d exp h %d exp w %d\n",
			hw_cfg->skip_planes[SB_PLANE_REAL].plane_h,
			hw_cfg->skip_planes[SB_PLANE_REAL].plane_w, h, w);
		return false;
	}
	return true;
}

static bool valid_abc_udc_layer_cfg_v1(struct drm_msm_abc *aiqe_abc,
					struct sde_hw_cp_cfg *hw_cfg)
{
	u32 h, w;

	/* UDC is disabled early return */
	if (!(aiqe_abc->param[0] & 0x2))
		return true;

	if ((aiqe_abc->param[0] & 0x2) && !hw_cfg->skip_planes[SB_PLANE_VIRT].valid) {
		DRM_INFO("aiqe_abc state %d valid blend plane %d\n", (aiqe_abc->param[0] & 0x2),
				hw_cfg->skip_planes[SB_PLANE_VIRT].valid);
		return false;
	}

	h = (aiqe_abc->param[24] >> 16) & (((1 << 12) - 1));
	w = aiqe_abc->param[24] & ((1 << 12) - 1);

	w = (w * 3) / 4;

	if (h != hw_cfg->skip_planes[SB_PLANE_VIRT].plane_h ||
		w != hw_cfg->skip_planes[SB_PLANE_VIRT].plane_w) {
		DRM_ERROR("virt plane invalid plane h %d w %d exp h %d exp w %d\n",
			hw_cfg->skip_planes[SB_PLANE_VIRT].plane_h,
			hw_cfg->skip_planes[SB_PLANE_VIRT].plane_w, h, w);
		return false;
	}
	return true;
}

static bool valid_abc_v1_en_cfg(struct drm_msm_abc *aiqe_abc,
				struct sde_hw_cp_cfg *hw_cfg)
{
	if (!valid_abc_main_layer_cfg_v1(aiqe_abc, hw_cfg))
		return false;

	if (!valid_abc_udc_layer_cfg_v1(aiqe_abc, hw_cfg))
		return false;

	return true;
}

void sde_setup_aiqe_abc_v1(struct sde_hw_dspp *ctx, void *cfg, void *aiqe_top)
{
	struct sde_hw_cp_cfg *hw_cfg = cfg;
	struct drm_msm_abc *aiqe_abc = NULL;
	u32 i, aiqe_base;

	if (!ctx || !cfg || !aiqe_top) {
		DRM_ERROR("invalid parameters ctx %pK cfg %pK aiqe top %pK\n",
			ctx, cfg, aiqe_top);
		return;
	}
	aiqe_base = ctx->cap->sblk->aiqe.base;
	if (!aiqe_base) {
		DRM_DEBUG_DRIVER("AIQE not supported on DSPP idx %d", ctx->idx);
		return;
	}
	aiqe_abc = (struct drm_msm_abc *)(hw_cfg->payload);

	if (!hw_cfg->payload || !valid_abc_v1_en_cfg(aiqe_abc, hw_cfg)) {
		DRM_DEBUG_DRIVER("Disable ABC feature\n");
		/* Setup common registers in disable case, error case skip */
		if (!hw_cfg->payload)
			sde_setup_aiqe_common_v1(ctx, hw_cfg, aiqe_top);
		SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x020, 0);
		return;
	}

	if (hw_cfg->len != sizeof(struct drm_msm_abc)) {
		DRM_ERROR("invalid size of payload len %d exp %zd\n",
			hw_cfg->len, sizeof(struct drm_msm_abc));
		return;
	}

	for (i = 0; i < AIQE_ABC_PARAM_LEN; i++)
		SDE_REG_WRITE(&ctx->hw, aiqe_base + 0x020 + (i * sizeof(u32)), aiqe_abc->param[i]);

	sde_setup_aiqe_common_v1(ctx, hw_cfg, aiqe_top);
}
