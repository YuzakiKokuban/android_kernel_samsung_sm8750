/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 */

#include <drm/sde_drm.h>
#include "sde_hw_top.h"
#include "shd_drm.h"

#ifndef SHD_HW_H
#define SHD_HW_H

struct sde_shd_ctl_mixer_cfg {
	u32 mixercfg;
	u32 mixercfg_ext;
	u32 mixercfg_ext2;
	u32 mixercfg_ext3;

	u32 mixercfg_mask;
	u32 mixercfg_ext_mask;
	u32 mixercfg_ext2_mask;
	u32 mixercfg_ext3_mask;

	u32 mixercfg_skip_sspp_mask[2];
};

struct sde_shd_hw_ctl {
	struct sde_hw_ctl base;
	struct shd_stage_range range;
	struct sde_hw_ctl *orig;
	u32 flush_mask;
	u32 old_mask;
	struct sde_shd_ctl_mixer_cfg mixer_cfg[MAX_BLOCKS];

	bool cwb_enable;
	bool cwb_changed;
	u32 cwb_active;
	u32 merge_3d_active;
	struct sde_hw_intf_cfg_v1 dsc_cfg;
	u32 fetch_active;
	u32 old_pipe_active;
};

struct sde_shd_mixer_cfg {
	u32 fg_alpha;
	u32 bg_alpha;
	u32 blend_op;
	bool dirty;

	struct sde_hw_dim_layer dim_layer;
	bool dim_layer_enable;
};

struct sde_shd_hw_mixer {
	struct sde_hw_mixer base;
	struct shd_stage_range range;
	struct sde_rect roi;
	struct sde_hw_mixer *orig;
	struct sde_shd_mixer_cfg cfg[SDE_STAGE_MAX];
};

void sde_shd_hw_ctl_init_op(struct sde_hw_ctl *ctx);

void sde_shd_hw_lm_init_op(struct sde_hw_mixer *ctx);

void sde_shd_hw_skip_sspp_clear(struct sde_hw_ctl *ctx, enum sde_sspp sspp, int multirect_idx);

#endif
