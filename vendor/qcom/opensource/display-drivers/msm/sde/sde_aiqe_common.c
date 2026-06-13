// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "sde_kms.h"
#include "sde_aiqe_common.h"
#include "sde_hw_mdss.h"

#define AIQE_VER_1_0 0x00010000

void (*aiqe_get_common_values_func)(struct sde_hw_cp_cfg *cfg,
		struct sde_aiqe_top_level *aiqe_top, struct aiqe_reg_common *aiqe_cmn);
static void aiqe_get_common_values_v1(struct sde_hw_cp_cfg *cfg,
					struct sde_aiqe_top_level *aiqe_top,
					struct aiqe_reg_common *aiqe_cmn);

void aiqe_init(u32 aiqe_version, struct sde_aiqe_top_level *aiqe_top)
{
	if (!aiqe_top)
		return;

	switch (aiqe_version) {
	case AIQE_VER_1_0:
		aiqe_get_common_values_func = &aiqe_get_common_values_v1;
		break;
	default:
		break;
	}
}

void aiqe_register_client(enum aiqe_features feature_id, struct sde_aiqe_top_level *aiqe_top)
{
	if (!aiqe_top || feature_id >= AIQE_FEATURE_MAX)
		return;

	SDE_EVT32(feature_id);
	atomic_or(1 << feature_id, &aiqe_top->aiqe_mask);
}

void aiqe_deregister_client(enum aiqe_features feature_id, struct sde_aiqe_top_level *aiqe_top)
{
	if (!aiqe_top || feature_id >= AIQE_FEATURE_MAX)
		return;

	SDE_EVT32(feature_id);
	atomic_andnot(1 << feature_id, &aiqe_top->aiqe_mask);
}

void aiqe_get_common_values(struct sde_hw_cp_cfg *cfg, struct sde_aiqe_top_level *aiqe_top,
				struct aiqe_reg_common *aiqe_cmn)
{
	if (aiqe_get_common_values_func == NULL) {
		DRM_ERROR("Get common values function is invalid!");
		return;
	}

	if (!cfg || !aiqe_top || !aiqe_cmn) {
		DRM_ERROR("Invalid params!\n");
		return;
	}

	(*aiqe_get_common_values_func)(cfg, aiqe_top, aiqe_cmn);
}

bool aiqe_is_client_registered(enum aiqe_features feature_id, struct sde_aiqe_top_level *aiqe_top)
{
	if (!aiqe_top || feature_id >= AIQE_FEATURE_MAX)
		return false;

	return (atomic_read(&aiqe_top->aiqe_mask) & (1 << feature_id));
}

static void aiqe_get_common_values_v1(struct sde_hw_cp_cfg *cfg,
					struct sde_aiqe_top_level *aiqe_top,
					struct aiqe_reg_common *aiqe_cmn)
{
	struct sde_hw_mixer *hw_lm = NULL;
	u32 mask_value = 0;

	hw_lm = cfg->mixer_info;
	mask_value = atomic_read(&aiqe_top->aiqe_mask);
	if (mask_value == 0)
		aiqe_cmn->config &= ~BIT(0);
	else
		aiqe_cmn->config |= BIT(0);

	if (hw_lm->idx == LM_0) {
		aiqe_cmn->config &= ~BIT(1);
	} else if (hw_lm->idx == LM_2) {
		aiqe_cmn->config |= BIT(1);
	} else {
		DRM_DEBUG_DRIVER("AIQE not supported on LM idx %d", hw_lm->idx);
		return;
	}

	if (cfg->num_of_mixers == 1) {
		aiqe_cmn->merge = SINGLE_MODE;
	} else if (cfg->num_of_mixers == 2) {
		aiqe_cmn->merge = DUAL_MODE;
	} else if (cfg->num_of_mixers == 4) {
		aiqe_cmn->merge = QUAD_MODE;
	} else {
		DRM_ERROR("Invalid number of mixers %d", cfg->num_of_mixers);
		return;
	}

	aiqe_cmn->height = cfg->panel_height;
	aiqe_cmn->width = cfg->panel_width;
	aiqe_cmn->irqs = 0;
}

bool mdnie_art_in_progress(struct sde_aiqe_top_level *aiqe_top)
{
	if (!aiqe_top)
		return false;

	return aiqe_is_client_registered(FEATURE_MDNIE_ART, aiqe_top);
}

void get_mdnie_art_frame_count(u32 *mdnie_art_frame_count, u32 art_param)
{
	u32 art_slope = 0;
	if (!art_param)
		return;

	art_slope = (art_param & 0xF0000) >> 16;
	*mdnie_art_frame_count = 1 << art_slope;
	++(*mdnie_art_frame_count);
}

void aiqe_deinit(struct sde_aiqe_top_level *aiqe_top)
{
}
