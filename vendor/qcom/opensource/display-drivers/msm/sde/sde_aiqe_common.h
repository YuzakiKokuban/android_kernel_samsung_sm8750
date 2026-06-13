/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _SDE_AIQE_COMMON_H
#define _SDE_AIQE_COMMON_H

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
enum aiqe_copr_roi {
    AIQE_COPR_ROI_1,
    AIQE_COPR_ROI_2,
    AIQE_COPR_ROI_3,
    AIQE_COPR_ROI_4,
    AIQE_COPR_ROI_5,
    MAX_AIQE_COPR_ROI,
};

enum aiqe_reg_copr_wrgb {
    AIQE_COPR_WRGB_W,
    AIQE_COPR_WRGB_R,
    AIQE_COPR_WRGB_G,
    AIQE_COPR_WRGB_B,
    MAX_AIQE_COPR_WRGB,
};

#define COPR_STATUS_LEN          (10)
#define COPR_ROI_G_SHIFT         (16)
#define COPR_ROI_G(_v)           ((_v) << COPR_ROI_G_SHIFT)
#define COPR_ROI_G_MASK          (0x3FF << COPR_ROI_G_SHIFT)
#define COPR_ROI_R_SHIFT         (0)
#define COPR_ROI_R(_v)           ((_v) << COPR_ROI_R_SHIFT)
#define COPR_ROI_R_MASK          (0x3FF << COPR_ROI_R_SHIFT)

#define COPR_ROI_W_SHIFT         (16)
#define COPR_ROI_W(_v)           ((_v) << COPR_ROI_W_SHIFT)
#define COPR_ROI_W_MASK          (0x3FF << COPR_ROI_W_SHIFT)
#define COPR_ROI_B_SHIFT         (0)
#define COPR_ROI_B(_v)           ((_v) << COPR_ROI_B_SHIFT)
#define COPR_ROI_B_MASK          (0x3FF << COPR_ROI_B_SHIFT)
#endif

struct sde_aiqe_top_level {
	atomic_t aiqe_mask;
};

enum aiqe_merge_mode {
	SINGLE_MODE,
	DUAL_MODE,
	QUAD_MODE = 0x3,
	MERGE_MODE_MAX,
};

enum aiqe_features {
	FEATURE_MDNIE = 0x1,
	FEATURE_MDNIE_ART,
	FEATURE_ABC,
	FEATURE_SSRC,
	FEATURE_COPR,
	AIQE_FEATURE_MAX,
};

struct aiqe_reg_common {
	enum aiqe_merge_mode merge;
	u32 config;
	u32 height; // panel
	u32 width; // panel
	u32 irqs;
};

void aiqe_init(u32 aiqe_version, struct sde_aiqe_top_level *aiqe_top);
void aiqe_register_client(enum aiqe_features feature_id, struct sde_aiqe_top_level *aiqe_top);
void aiqe_deregister_client(enum aiqe_features feature_id, struct sde_aiqe_top_level *aiqe_top);
void aiqe_get_common_values(struct sde_hw_cp_cfg *cfg,
			    struct sde_aiqe_top_level *aiqe_top, struct aiqe_reg_common *aiqe_cmn);
bool aiqe_is_client_registered(enum aiqe_features feature_id, struct sde_aiqe_top_level *aiqe_top);
bool mdnie_art_in_progress(struct sde_aiqe_top_level *aiqe_top);
void get_mdnie_art_frame_count(u32 *mdnie_art_frame_count, u32 art_param);
void aiqe_deinit(struct sde_aiqe_top_level *aiqe_top);

#endif /* _SDE_AIQE_COMMON_H */
