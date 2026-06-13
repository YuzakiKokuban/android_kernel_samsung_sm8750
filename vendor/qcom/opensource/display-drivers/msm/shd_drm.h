/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 */

#ifndef _SHD_DRM_H_
#define _SHD_DRM_H_
#include <linux/types.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include "msm_drv.h"
#include "dsi_display.h"

struct shd_mode_info {
	int x_offset;
	int y_offset;
	int width;
	int height;
};

struct shd_stage_range {
	u32 start;
	u32 size;
};

struct shd_display_base {
	struct drm_display_mode mode;
	struct drm_display_info info;
	struct drm_crtc *crtc;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct list_head head;
	struct list_head disp_list;
	struct device_node *of_node;
	struct sde_connector_ops ops;

	int intf_idx;
	int hw_dev_id;
	bool mst_port;
	bool dynamic_mode;
	bool fill_ops;
};

struct shd_display {
	struct dsi_display *dsi_base;
	struct drm_device *drm_dev;
	const char *name;
	const char *display_type;
	struct drm_display_info info;

	struct shd_display_base *base;
	struct drm_bridge *bridge;

	struct device_node *base_of;
	struct sde_rect src;
	struct sde_rect roi;
	struct shd_stage_range stage_range;

	bool full_screen;

	struct platform_device *pdev;
	struct list_head head;
	struct notifier_block notifier;
	struct drm_crtc *crtc;
};

void *sde_encoder_phys_shd_init(enum sde_intf_type type, u32 controller_id,
				void *phys_init_params);

void sde_shd_hw_flush(struct sde_hw_ctl *ctl_ctx,
		      struct sde_hw_mixer *lm_ctx[MAX_MIXERS_PER_CRTC], int lm_num);

/* helper for seamless plane handoff */

u32 shd_get_shared_crtc_mask(struct drm_crtc *crtc);
void shd_skip_shared_plane_update(struct drm_plane *plane, struct drm_crtc *crtc);
void sde_connector_get_avail_res_info_shd(struct drm_connector *conn,
					  struct msm_resource_caps_info *avail_res);
#endif /* _SHD_DRM_H_ */
