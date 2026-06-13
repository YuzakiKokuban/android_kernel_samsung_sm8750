// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm-shd] %s: " fmt, __func__

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/component.h>
#include <linux/of_irq.h>
#include <linux/kthread.h>
#include <linux/sched/types.h>
#include <linux/backlight.h>
#include "sde_connector.h"
#include <drm/drm_atomic_helper.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_atomic.h>
#include <drm/drm_crtc.h>
#include <drm/drm_vblank.h>
#include <drm/drm_connector.h>
#include <drm/drm_modes.h>
#include "msm_drv.h"
#include "msm_kms.h"
#include "sde_connector.h"
#include "sde_encoder.h"
#include "sde_crtc.h"
#include "sde_plane.h"
#include "shd_drm.h"
#include "shd_hw.h"
#include <drm/drm_edid.h>

#define DRM_CONNECTOR_MAX_ENCODER 3
static LIST_HEAD(g_base_list);

struct shd_crtc {
	struct drm_crtc_helper_funcs helper_funcs;
	const struct drm_crtc_helper_funcs *orig_helper_funcs;
	struct drm_crtc_funcs funcs;
	const struct drm_crtc_funcs *orig_funcs;
	struct shd_display *display;
};

struct shd_kms {
	struct msm_kms_funcs funcs;
	const struct msm_kms_funcs *orig_funcs;
};

struct sde_cp_node_dummy {
	u32 property_id;
	u32 prop_flags;
	u32 feature;
	void *blob_ptr;
	u64 prop_val;
	const struct sde_pp_blk *pp_blk;
	struct list_head cp_feature_list;
	struct list_head active_list;
	struct list_head dirty_list;
};

static struct shd_kms *g_shd_kms;

static enum drm_connector_status shd_display_base_detect(struct drm_connector *connector,
							 bool force, void *disp)
{
	return connector_status_disconnected;
}

static inline bool shd_display_check_enc_intf(struct sde_encoder_hw_resources *hw_res,
					      int intf_idx)
{
	if (intf_idx >= INTF_MAX - INTF_0)
		return false;

	return hw_res->intfs[intf_idx] != INTF_MODE_NONE;
}

static inline bool shd_display_check_enc_intf_dsi(int intf_idx)
{
	if (intf_idx >= INTF_MAX - INTF_0)
		return false;

	return (intf_idx == INTF_1 || intf_idx == INTF_0);
}

static inline int shd_display_get_enc_intf(struct sde_encoder_hw_resources *hw_res)
{
	int i;

	for (i = INTF_0; i < INTF_MAX; i++)
		if (hw_res->intfs[i - INTF_0] != INTF_MODE_NONE)
			return i - INTF_0;

	return INTF_MAX;
}

static int shd_display_init_base_connector(struct drm_device *dev, struct shd_display_base *base)
{
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct sde_connector *sde_conn;
	struct drm_connector_list_iter conn_iter;
	int rc = 0;

	if (base->connector)
		goto next;

	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
		sde_conn = to_sde_connector(connector);
		encoder = sde_conn->encoder;
		if (encoder == base->encoder) {
			base->connector = connector;
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	if (!base->connector) {
		SDE_ERROR("failed to find connector\n");
		return -ENOENT;
	}

next:
	SDE_DEBUG("found base connector %d\n", base->connector->base.id);

	return rc;
}

static int shd_display_init_base_encoder(struct drm_device *dev, struct shd_display_base *base)
{
	struct drm_encoder *encoder = 0;
	struct sde_connector *sde_conn = 0;
	struct sde_encoder_hw_resources *hw_res;
	struct sde_connector_state *conn_state;
	struct msm_compression_info *comp_info;
	int rc = 0;

	hw_res = kzalloc(sizeof(*hw_res), GFP_KERNEL);
	conn_state = kzalloc(sizeof(*conn_state), GFP_KERNEL);
	comp_info = kzalloc(sizeof(*comp_info), GFP_KERNEL);
	if (!hw_res || !conn_state || !comp_info) {
		rc = -ENOMEM;
		goto end;
	}
	hw_res->comp_info = comp_info;
	if (base->connector) {
		sde_conn = to_sde_connector(base->connector);
		encoder = sde_conn->encoder;
		sde_encoder_get_hw_resources(encoder, hw_res, &conn_state->base);
		base->encoder = encoder;

		base->intf_idx = shd_display_get_enc_intf(hw_res);
		goto next;
	}
	drm_for_each_encoder(encoder, dev) {
		if (encoder->encoder_type == DRM_MODE_ENCODER_DSI &&
		    shd_display_check_enc_intf_dsi(base->intf_idx)) {
			sde_encoder_get_hw_resources(encoder, hw_res, &conn_state->base);
			if (shd_display_check_enc_intf(hw_res, base->intf_idx)) {
				base->encoder = encoder;
				break;
			}
		} else if (encoder->encoder_type == DRM_MODE_ENCODER_TMDS) {
			sde_encoder_get_hw_resources(encoder, hw_res, &conn_state->base);
			if (shd_display_check_enc_intf(hw_res, base->intf_idx)) {
				base->encoder = encoder;
				break;
			}
		}
	}
next:
	if (!base->encoder) {
		SDE_ERROR("can't find base encoder for intf %d\n", base->intf_idx);
		rc = -ENOENT;
		goto end;
	}

	SDE_DEBUG("found base encoder %d\n", base->encoder->base.id);
end:
	kfree(hw_res);
	kfree(comp_info);
	kfree(conn_state);
	return rc;
}

static int shd_display_init_base_crtc(struct drm_device *dev, struct shd_display_base *base)
{
	struct drm_crtc *crtc = NULL;
	struct msm_drm_private *priv;
	struct drm_plane *primary;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct drm_connector_list_iter conn_iter;
	int crtc_idx;
	int i;

	priv = dev->dev_private;

	if (base->encoder->crtc) {
		/* if cont splash is enabled on crtc */
		crtc = base->encoder->crtc;
		crtc_idx = drm_crtc_index(crtc);
	} else {
		/* find last crtc for base encoder */
		for (i = priv->num_crtcs - 1; i >= 0; i--) {
			if (base->encoder->possible_crtcs & (1 << i)) {
				crtc = priv->crtcs[i];
				crtc_idx = i;
				break;
			}
		}

		if (!crtc)
			return -ENOENT;
	}

	if (priv->num_planes >= MAX_PLANES)
		return -ENOENT;

	/*create dummy primary plane for base crtc*/
	primary = sde_plane_init(dev, SSPP_DMA0, true, 0, 0);

	if (IS_ERR(primary))
		return -ENOMEM;

	priv->planes[priv->num_planes++] = primary;
	list_del(&primary->head);
	if (primary->funcs->reset)
		primary->funcs->reset(primary);

	SDE_DEBUG("create dummy plane%d free plane%d\n", DRMID(primary), DRMID(crtc->primary));

	crtc->primary = primary;

	/* disable crtc from other connectors */
	drm_connector_list_iter_begin(dev, &conn_iter);
	drm_for_each_connector_iter(connector, &conn_iter) {
		if (connector == base->connector)
			continue;

		for (i = 0; i < connector->possible_encoders; i++) {
			encoder = connector->encoder;
			if (encoder)
				encoder->possible_crtcs &= ~(1 << crtc_idx);
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	base->crtc = crtc;
	SDE_DEBUG("found base crtc %d\n", crtc->base.id);

	/* limit base encoder to base crtc */
	base->encoder->possible_crtcs = (1 << crtc_idx);
	return 0;
}

static int shd_crtc_validate_shared_display(struct drm_crtc *crtc,
					    struct drm_atomic_state *atomic_state)
{
	struct sde_crtc *sde_crtc;
	struct shd_crtc *shd_crtc;
	struct sde_crtc_state *sde_crtc_state;
	struct drm_plane *plane;
	const struct drm_plane_state *pstate;
	struct sde_plane_state *sde_pstate;
	int i;

	struct drm_crtc_state *state = drm_atomic_get_new_crtc_state(atomic_state, crtc);

	sde_crtc = to_sde_crtc(crtc);
	shd_crtc = sde_crtc->priv_handle;
	sde_crtc_state = to_sde_crtc_state(state);

	/* check shared display roi */
	if (state->mode_changed && state->active) {
		struct shd_display *display = shd_crtc->display;
		struct drm_crtc_state *base_crtc_state;

		base_crtc_state = drm_atomic_get_existing_crtc_state(state->state,
								     display->base->crtc);
		if (!base_crtc_state)
			base_crtc_state = display->base->crtc->state;

		if (state->mode.hdisplay + display->roi.x >
			base_crtc_state->mode.hdisplay ||
			state->mode.vdisplay + display->roi.y >
			base_crtc_state->mode.vdisplay) {
			SDE_ERROR("roi %d,%d,%dx%d exceeds base mode %dx%d\n", display->roi.x,
				  display->roi.y, state->mode.hdisplay, state->mode.vdisplay,
				  base_crtc_state->mode.hdisplay, base_crtc_state->mode.vdisplay);
			return -EINVAL;
		}
	}

	/* check z-pos for all planes */
	drm_atomic_crtc_state_for_each_plane_state(plane, pstate, state) {
		sde_pstate = to_sde_plane_state(pstate);
		if (sde_pstate->stage >=
			shd_crtc->display->stage_range.size + SDE_STAGE_0) {
			SDE_ERROR("plane stage %d is larger than maximum %d\n",
				  sde_pstate->stage, shd_crtc->display->stage_range.size);
			return -EINVAL;
		}
	}

	/* check z-pos for all dim layers */
	for (i = 0; i < sde_crtc_state->num_dim_layers; i++) {
		if (sde_crtc_state->dim_layer[i].stage >=
			shd_crtc->display->stage_range.size + SDE_STAGE_0) {
			SDE_ERROR("dim stage %d is larger than maximum %d\n",
				  sde_crtc_state->dim_layer[i].stage,
				  shd_crtc->display->stage_range.size);
			return -EINVAL;
		}
	}

	/* update crtc_roi */
	sde_crtc_state->crtc_roi.x = -shd_crtc->display->roi.x;
	sde_crtc_state->crtc_roi.y = -shd_crtc->display->roi.y;
	sde_crtc_state->crtc_roi.w = 0;
	sde_crtc_state->crtc_roi.h = 0;

	return 0;
}

static int shd_crtc_atomic_check(struct drm_crtc *crtc, struct drm_atomic_state *atomic_state)
{
	struct drm_crtc_state *state = drm_atomic_get_new_crtc_state(atomic_state, crtc);
	struct sde_crtc *sde_crtc = to_sde_crtc(crtc);
	struct shd_crtc *shd_crtc = sde_crtc->priv_handle;
	int rc;
	struct drm_crtc_state *base_drm_crtc_state = NULL;
	struct sde_crtc_state *base_sde_cstate;
	struct sde_crtc *base_sde_crtc;

	base_drm_crtc_state =
		drm_atomic_get_existing_crtc_state(state->state,
						   shd_crtc->display->base->crtc);
	if (!base_drm_crtc_state)
		base_drm_crtc_state = shd_crtc->display->base->crtc->state;

	base_sde_cstate = to_sde_crtc_state(base_drm_crtc_state);
	base_sde_crtc = to_sde_crtc(base_drm_crtc_state->crtc);

	/* update topology name */
	if (sde_crtc->num_mixers == SDE_RM_TOPOLOGY_NONE || base_sde_crtc->num_mixers == 0)
		sde_crtc->num_mixers = base_sde_crtc->num_mixers;

	rc = shd_crtc->orig_helper_funcs->atomic_check(crtc, atomic_state);
	if (rc)
		return rc;

	return shd_crtc_validate_shared_display(crtc, atomic_state);
}

static int shd_crtc_atomic_set_property(struct drm_crtc *crtc, struct drm_crtc_state *state,
					struct drm_property *property, uint64_t val)
{
	struct sde_crtc *sde_crtc = to_sde_crtc(crtc);
	struct shd_crtc *shd_crtc = sde_crtc->priv_handle;
	struct sde_cp_node_dummy *prop_node;

	if (!crtc || !state || !property) {
		SDE_ERROR("invalid argument(s)\n");
		return -EINVAL;
	}

	/* ignore all the dspp properties */
	list_for_each_entry(prop_node, &sde_crtc->cp_feature_list, cp_feature_list) {
		if (property->base.id == prop_node->property_id)
			return 0;
	}

	return shd_crtc->orig_funcs->atomic_set_property(crtc, state, property, val);
}

u32 shd_get_shared_crtc_mask(struct drm_crtc *src_crtc)
{
	struct shd_crtc *shd_src_crtc, *shd_crtc;
	struct drm_crtc *crtc;
	u32 crtc_mask = 0;

	if (!src_crtc)
		return 0;

	if (src_crtc->helper_private->atomic_check != shd_crtc_atomic_check)
		return drm_crtc_mask(src_crtc);

	shd_src_crtc = to_sde_crtc(src_crtc)->priv_handle;

	drm_for_each_crtc(crtc, src_crtc->dev) {
		if (crtc->helper_private->atomic_check != shd_crtc_atomic_check)
			continue;

		shd_crtc = to_sde_crtc(crtc)->priv_handle;

		if (shd_src_crtc->display->base == shd_crtc->display->base)
			crtc_mask |= drm_crtc_mask(crtc);
	}

	return crtc_mask;
}

void shd_skip_shared_plane_update(struct drm_plane *plane, struct drm_crtc *crtc)
{
	struct sde_crtc *sde_crtc;
	struct shd_crtc *shd_crtc;
	enum sde_sspp sspp;
	bool is_virtual;
	int i;

	if (!plane || !crtc) {
		SDE_ERROR("invalid plane or crtc\n");
		return;
	}

	if (crtc->funcs->atomic_set_property != shd_crtc_atomic_set_property) {
		SDE_ERROR("not shared crtc\n");
		return;
	}

	sde_crtc = to_sde_crtc(crtc);
	shd_crtc = sde_crtc->priv_handle;
	sspp = sde_plane_pipe(plane);
	is_virtual = is_sde_plane_virtual(plane);

	for (i = 0; i < sde_crtc->num_mixers; i++)
		sde_shd_hw_skip_sspp_clear(sde_crtc->mixers[i].hw_ctl, sspp, is_virtual);
}

static int shd_display_set_default_clock(struct drm_crtc_state *crtc_state,
					 struct drm_connector_state *conn_state,
					 struct drm_display_mode *mode)
{
	struct msm_drm_private *priv;
	struct sde_kms *sde_kms;
	struct sde_crtc *sde_crtc;
	struct sde_crtc_state *cstate;
	struct sde_connector *sde_conn;
	struct msm_mode_info mode_info;
	struct drm_property *drm_prop;
	u64 core_clk;
	int ret;
	struct msm_resource_caps_info avail_res;

	priv = crtc_state->crtc->dev->dev_private;
	sde_kms = to_sde_kms(priv->kms);
	sde_conn = to_sde_connector(conn_state->connector);

	if (!crtc_state->active || !sde_conn->ops.get_mode_info)
		return 0;

	memset(&avail_res, 0, sizeof(avail_res));
	sde_connector_get_avail_res_info_shd(conn_state->connector, &avail_res);

	ret = sde_conn->ops.get_mode_info(&sde_conn->base, mode, NULL, &mode_info,
			sde_conn->display, &avail_res);
	if (ret)
		return ret;

	if (!mode_info.topology.num_lm) {
		mode_info.topology.num_lm = 1;
		pr_info("fixup base topology to 1 lm\n");
	}

	/* calculate clock based on layer mixer */
	core_clk = crtc_state->mode.clock / mode_info.topology.num_lm;
	core_clk *= 1050ULL;

	/* 3dmerge + dsc we need to double the clock */
	if (mode_info.topology.num_enc && mode_info.topology.num_lm > mode_info.topology.num_enc)
		core_clk *= 2;

	cstate = to_sde_crtc_state(crtc_state);
	sde_crtc = to_sde_crtc(crtc_state->crtc);

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_CORE_CLK];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, core_clk);
	if (ret)
		return ret;

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_CORE_AB];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, 0);
	if (ret)
		return ret;

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_CORE_IB];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, 0);
	if (ret)
		return ret;

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_LLCC_AB];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, 0);
	if (ret)
		return ret;

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_LLCC_IB];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, 0);
	if (ret)
		return ret;

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_DRAM_AB];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, 0);
	if (ret)
		return ret;

	drm_prop = sde_crtc->property_info.property_array[CRTC_PROP_DRAM_IB];
	ret = msm_property_atomic_set(&sde_crtc->property_info, &cstate->property_state,
				      drm_prop, 0);
	if (ret)
		return ret;

	cstate->bw_control = true;
	cstate->bw_split_vote = true;

	SDE_DEBUG("set base core clock %llu\n", core_clk);

	return 0;
}

static int shd_display_atomic_check(struct msm_kms *kms, struct drm_atomic_state *state)
{
	struct msm_drm_private *priv;
	struct drm_crtc *crtc;
	struct drm_crtc_state *old_crtc_state, *new_crtc_state;
	struct drm_connector_state *conn_state;
	struct sde_crtc *sde_crtc;
	struct shd_crtc *shd_crtc;
	struct shd_display *display;
	struct shd_display_base *base;
	u32 base_mask = 0, enable_mask = 0, disable_mask = 0, change_mask = 0;
	u32 crtc_mask, active_mask;
	bool active;
	int i, rc;

	for_each_oldnew_crtc_in_state(state, crtc, old_crtc_state, new_crtc_state, i) {
		if (new_crtc_state->mode_changed && new_crtc_state->active)
			change_mask |= drm_crtc_mask(crtc);

		if (crtc->helper_private->atomic_check != shd_crtc_atomic_check)
			continue;

		if (old_crtc_state->active == new_crtc_state->active)
			continue;

		sde_crtc = to_sde_crtc(crtc);
		shd_crtc = sde_crtc->priv_handle;
		base = shd_crtc->display->base;
		base_mask |= drm_crtc_mask(base->crtc);

		if (new_crtc_state->active)
			enable_mask |= drm_crtc_mask(crtc);
		else
			disable_mask |= drm_crtc_mask(crtc);
	}

	/*
	 * when base display has mode change set, all shared displays should
	 * also set mode change flag.
	 */
	if (change_mask) {
		list_for_each_entry(base, &g_base_list, head) {
			if (!(drm_crtc_mask(base->crtc) & change_mask))
				continue;

			list_for_each_entry(display, &base->disp_list, head) {
				new_crtc_state = drm_atomic_get_crtc_state(state, display->crtc);
				if (IS_ERR(new_crtc_state))
					return PTR_ERR(new_crtc_state);

				if (new_crtc_state->active)
					new_crtc_state->mode_changed = true;
			}

			base_mask |= drm_crtc_mask(base->crtc);
		}
	}

	if (!base_mask)
		return g_shd_kms->orig_funcs->atomic_check(kms, state);

	/*
	 * If base display need to be enabled/disabled, add state
	 * changes to the same atomic state. As base crtc is always
	 * ahead of shared crtc in the crtc list, base crtc is
	 * enabled/disabled before shared crtcs.
	 */
	list_for_each_entry(base, &g_base_list, head) {
		if (!(drm_crtc_mask(base->crtc) & base_mask))
			continue;

		/* always add base crtc's lock into state */
		rc = drm_modeset_lock(&base->crtc->mutex, state->acquire_ctx);
		if (rc)
			return rc;

		/* read old crtc state from all shared displays */
		crtc_mask = 0;
		active_mask = 0;
		list_for_each_entry(display, &base->disp_list, head) {
			crtc_mask |= drm_crtc_mask(display->crtc);
			if (display->crtc->state->active)
				active_mask |= drm_crtc_mask(display->crtc);
		}

		/* apply changes in state */
		active_mask |= (enable_mask & crtc_mask);
		active_mask &= ~disable_mask;
		active = !!active_mask;

		/* skip if there is no change */
		if (base->crtc->state->active == active &&
		    (!active || (base->crtc->state->connector_mask &
						(1 << drm_connector_index(base->connector)))))
			continue;

		new_crtc_state = drm_atomic_get_crtc_state(state, base->crtc);
		if (IS_ERR(new_crtc_state))
			return PTR_ERR(new_crtc_state);

		/* if base display is in state, no need to wait */
		base_mask &= ~drm_crtc_mask(base->crtc);

		new_crtc_state->active = active;

		conn_state = drm_atomic_get_connector_state(state, base->connector);
		if (IS_ERR(conn_state))
			return PTR_ERR(conn_state);

		rc = drm_atomic_set_mode_for_crtc(new_crtc_state, active ? &base->mode : NULL);
		if (rc) {
			SDE_ERROR("failed to set mode for crtc\n");
			return rc;
		}

		rc = shd_display_set_default_clock(new_crtc_state, conn_state, &base->mode);
		if (rc) {
			SDE_ERROR("failed to set default clock\n");
			return rc;
		}

		rc = drm_atomic_set_crtc_for_connector(conn_state, active ? base->crtc : NULL);
		if (rc) {
			SDE_ERROR("failed to set crtc for connector\n");
			return rc;
		}

		SDE_DEBUG("set base crtc%d mode=%s active=%d\n", base->crtc->base.id,
			  base->mode.name, active);
	}

	rc = g_shd_kms->orig_funcs->atomic_check(kms, state);
	if (rc)
		return rc;

	/* wait if there is base thread running */
	if (base_mask) {
		priv = state->dev->dev_private;
		spin_lock(&priv->pending_crtcs_event.lock);
		wait_event_interruptible_locked(priv->pending_crtcs_event,
						!(priv->pending_crtcs & base_mask));
		spin_unlock(&priv->pending_crtcs_event.lock);
	}

	return 0;
}

static int shd_connector_get_info(struct drm_connector *connector, struct msm_display_info *info,
				  void *data)
{
	struct shd_display *display = data;

	if (!info || !data || !display->base || !display->drm_dev) {
		SDE_ERROR("invalid params\n");
		return -EINVAL;
	}

	info->intf_type = display->base->connector->connector_type;
	info->capabilities = MSM_DISPLAY_CAP_VID_MODE | MSM_DISPLAY_CAP_MST_MODE;
	info->is_connected = true;
	info->num_of_h_tiles = 1;
	info->h_tile_instance[0] = display->base->intf_idx;

	return 0;
}

static int shd_connector_get_mode_info(struct drm_connector *connector,
				       const struct drm_display_mode *drm_mode,
				       struct msm_sub_mode *sub_mode,
				       struct msm_mode_info *mode_info, void *display,
				       const struct msm_resource_caps_info *avail_res)
{
	struct shd_display *shd_display = display;
	struct sde_connector *base_conn;
	struct msm_mode_info base_mode_info;

	if (!drm_mode || !mode_info || !display) {
		SDE_ERROR("invalid params\n");
		return -EINVAL;
	}

	memset(mode_info, 0, sizeof(*mode_info));

	mode_info->frame_rate = drm_mode_vrefresh(drm_mode);
	mode_info->vtotal = drm_mode->vtotal;
	mode_info->comp_info.comp_type = MSM_DISPLAY_COMPRESSION_NONE;

	base_conn = to_sde_connector(shd_display->base->connector);
	base_conn->ops.get_mode_info(shd_display->base->connector, &shd_display->base->mode,
			sub_mode, &base_mode_info, base_conn->display, avail_res);

	mode_info->topology = base_mode_info.topology;

	if (shd_display->src.h != shd_display->roi.h)
		mode_info->vpadding = shd_display->roi.h;

	return 0;
}

static enum drm_connector_status shd_connector_detect(struct drm_connector *conn, bool force,
						      void *display)
{
	struct shd_display *disp = display;
	struct sde_connector *sde_conn;
	struct drm_connector *b_conn;
	enum drm_connector_status status = connector_status_disconnected;

	if (!conn || !display || !disp->base) {
		SDE_ERROR("invalid params\n");
		goto end;
	}

	b_conn =  disp->base->connector;
	if (b_conn) {
		sde_conn = to_sde_connector(b_conn);

		if (disp->base->ops.detect)
			status = disp->base->ops.detect(b_conn, force, sde_conn->display);
	}

end:
	return status;
}

static int shd_drm_update_edid_name(struct edid *edid, const char *name)
{
	u8 *dtd = (u8 *)&edid->detailed_timings[3];
	u8 standard_header[] = {0x00, 0x00, 0x00, 0xFC, 0x00};
	u32 dtd_size = 18;
	u32 header_size = sizeof(standard_header);

	if (!name)
		return -EINVAL;

	/* Fill standard header */
	memcpy(dtd, standard_header, header_size);

	dtd_size -= header_size;
	dtd_size = min_t(u32, dtd_size, strlen(name));

	memcpy(dtd + header_size, name, dtd_size);

	return 0;
}

static void shd_drm_update_checksum(struct edid *edid)
{
	u8 *data = (u8 *)edid;
	u32 i, sum = 0;

	for (i = 0; i < EDID_LENGTH - 1; i++)
		sum += data[i];

	edid->checksum = 0x100 - (sum & 0xFF);
}

static int shd_connector_get_modes(struct drm_connector *connector, void *data,
				   const struct msm_resource_caps_info *avail_res)
{
	struct shd_display *disp = data;
	struct drm_display_mode *m, *base_mode = NULL;
	struct sde_connector *sde_conn;
	int count;
	int rc;
	u32 edid_size;
	struct edid edid;
	const u8 edid_buf[EDID_LENGTH] = {
		0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x44, 0x6D,
		0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1B, 0x10, 0x01, 0x03,
		0x80, 0x50, 0x2D, 0x78, 0x0A, 0x0D, 0xC9, 0xA0, 0x57, 0x47,
		0x98, 0x27, 0x12, 0x48, 0x4C, 0x00, 0x00, 0x00, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
		0x01, 0x01, 0x01, 0x01,
	};

	sde_conn = to_sde_connector(disp->base->connector);

	/* get base probed modes */
	if (!sde_conn->ops.get_modes)
		return 0;

	count = sde_conn->ops.get_modes(disp->base->connector,
			sde_conn->display, NULL);

	if (!count) {
		SDE_DEBUG("no base mode probed\n");
		return 0;
	}

	/* set all old modes to the stale state */
	list_for_each_entry(m, &connector->modes, head)
		m->status = MODE_STALE;

	/* update base modes */
	drm_connector_list_update(disp->base->connector);

	/* validate modes */
	list_for_each_entry(m, &disp->base->connector->modes, head) {
		if (sde_conn->ops.mode_valid)
			m->status = sde_conn->ops.mode_valid(disp->base->connector, m,
							     sde_conn->display, avail_res);
	}

	/* prune invalid modes */
	drm_mode_prune_invalid(disp->drm_dev, &disp->base->connector->modes, false);

	if (list_empty(&disp->base->connector->modes)) {
		SDE_DEBUG("no valid base mode\n");
		return 0;
	}

	/* sort base mode */
	drm_mode_sort(&disp->base->connector->modes);

	/* search for base mode */
	if (disp->base->dynamic_mode) {
		list_for_each_entry(m, &disp->base->connector->modes, head) {
			drm_mode_copy(&disp->base->mode, m);
			base_mode = m;
			break;
		}
	} else {
		list_for_each_entry(m, &disp->base->connector->modes, head) {
			if (disp->base->mode.hdisplay == m->hdisplay &&
			    disp->base->mode.hsync_start == m->hsync_start &&
			    disp->base->mode.hsync_end == m->hsync_end &&
			    disp->base->mode.htotal == m->htotal &&
			    disp->base->mode.vdisplay == m->vdisplay &&
			    disp->base->mode.vsync_start == m->vsync_start &&
			    disp->base->mode.vsync_end == m->vsync_end &&
			    disp->base->mode.vtotal == m->vtotal &&
			    disp->base->mode.clock == m->clock) {
				drm_mode_copy(&disp->base->mode, m);
				base_mode = m;
				break;
			}
		}
		if (!base_mode) {
			SDE_INFO("directly use base mode in DT\n");
			base_mode = &disp->base->mode;
		}

		/* check display info override */
		if (disp->base->info.width_mm)
			disp->base->connector->display_info.width_mm =
					disp->base->info.width_mm;
		if (disp->base->info.height_mm)
			disp->base->connector->display_info.height_mm =
					disp->base->info.height_mm;
	}

	if (!base_mode) {
		SDE_ERROR("can't find base mode\n");
		return 0;
	}

	/* check shared display roi */
	if (!disp->full_screen) {
		if (disp->roi.x + disp->roi.w > base_mode->hdisplay ||
		    disp->roi.y + disp->roi.h > base_mode->vdisplay) {
			SDE_INFO("roi exceeds base display mode\n");
			return 0;
		}
	}

	/* update edid name */
	edid_size = min_t(u32, sizeof(edid), EDID_LENGTH);
	memcpy(&edid, edid_buf, edid_size);

	rc = shd_drm_update_edid_name(&edid, connector->name);
	if (rc)
		return 0;

	shd_drm_update_checksum(&edid);
	rc = drm_connector_update_edid_property(connector, &edid);

	if (rc)
		return 0;

	/* duplicate mode from base */
	m = drm_mode_duplicate(disp->drm_dev, base_mode);
	if (!m)
		return 0;

	/* update roi size */
	if (disp->full_screen) {
		disp->src.w = base_mode->hdisplay;
		disp->src.h = base_mode->vdisplay;
		disp->roi.w = base_mode->hdisplay;
		disp->roi.h = base_mode->vdisplay;
	} else {
		m->hdisplay = disp->src.w;
		m->hsync_start = m->hdisplay;
		m->hsync_end = m->hsync_start;
		m->htotal = m->hsync_end;
		m->vdisplay = disp->src.h;
		m->vsync_start = m->vdisplay;
		m->vsync_end = m->vsync_start;
		m->vtotal = m->vsync_end;
		drm_mode_set_name(m);
	}

	drm_mode_probed_add(connector, m);

	if (disp->info.width_mm)
		connector->display_info.width_mm = disp->info.width_mm;
	else
		connector->display_info.width_mm =
				disp->base->connector->display_info.width_mm;

	if (disp->info.height_mm)
		connector->display_info.height_mm = disp->info.height_mm;
	else
		connector->display_info.height_mm =
				disp->base->connector->display_info.height_mm;
	return 1;
}

static enum drm_mode_status shd_connector_mode_valid(struct drm_connector *connector,
						     struct drm_display_mode *mode, void *display,
						     const struct msm_resource_caps_info *avail_res)
{
	return MODE_OK;
}

static int shd_conn_set_info_blob(struct drm_connector *connector, void *info, void *display,
				  struct msm_mode_info *mode_info)
{
	struct shd_display *shd_display = display;

	if (!info || !shd_display)
		return -EINVAL;

	sde_kms_info_add_keyint(info, "max_blendstages", shd_display->stage_range.size);

	sde_kms_info_add_keystr(info, "display type", shd_display->display_type);

	sde_kms_info_add_keystr(info, "display type", shd_display->display_type);

	return 0;
}

static int shd_conn_set_property(struct drm_connector *connector,
				 struct drm_connector_state *state,
				 int property_index, uint64_t value, void *display)
{
	struct sde_connector *c_conn;

	c_conn = to_sde_connector(connector);

	/* overwrite properties that are not supported */
	switch (property_index) {
	case CONNECTOR_PROP_BL_SCALE:
		c_conn->bl_scale_dirty = false;
		c_conn->unset_bl_level = 0;
		break;
	case CONNECTOR_PROP_SV_BL_SCALE:
		c_conn->bl_scale_dirty = false;
		c_conn->unset_bl_level = 0;
		break;
	default:
		break;
	}

	return 0;
}

static int shd_backlight_device_update_status(struct backlight_device *bd)
{
	return 0;
}

static int shd_backlight_device_get_brightness(struct backlight_device *bd)
{
	return 0;
}

static const struct backlight_ops shd_backlight_device_ops = {
	.update_status = shd_backlight_device_update_status,
	.get_brightness = shd_backlight_device_get_brightness,
};

static int shd_display_create_backlight(struct drm_connector *connector)
{
	struct sde_connector *c_conn = to_sde_connector(connector);
	struct backlight_properties props;
	char bl_node_name[32];

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.power = FB_BLANK_UNBLANK;
	props.max_brightness = 255;
	props.brightness = 255;
	snprintf(bl_node_name, sizeof(bl_node_name), "panel%u-backlight",
		 connector->connector_type_id - 1);
	c_conn->bl_device = backlight_device_register(bl_node_name, connector->dev->dev, c_conn,
						      &shd_backlight_device_ops, &props);

	return 0;
}

static int shd_drm_obj_init(struct shd_display *display)
{
	struct msm_drm_private *priv;
	struct drm_device *dev;
	struct drm_crtc *crtc;
	struct drm_plane *primary;
	struct drm_encoder *encoder;
	struct drm_connector *connector;
	struct sde_crtc *sde_crtc;
	struct shd_crtc *shd_crtc;
	struct sde_connector *sde_conn;
	struct msm_display_info info;
	struct shd_display_base *base;
	struct sde_kms *sde_kms;
	int rc = 0;
	u32 i;

	static const struct sde_connector_ops shd_ops = {
		.set_info_blob = shd_conn_set_info_blob,
		.detect = shd_connector_detect,
		.get_modes = shd_connector_get_modes,
		.mode_valid = shd_connector_mode_valid,
		.get_info = shd_connector_get_info,
		.get_mode_info = shd_connector_get_mode_info,
		.set_property = shd_conn_set_property,
	};

	static const struct sde_encoder_ops enc_ops = {
		.phys_init =    sde_encoder_phys_shd_init,
	};

	dev = display->drm_dev;
	priv = dev->dev_private;

	base = display->base;

	list_for_each_entry(base, &g_base_list, head) {
		sde_conn = to_sde_connector(base->connector);

		if (!base->fill_ops) {
			base->ops = sde_conn->ops;
			sde_conn->ops.detect = shd_display_base_detect;
			base->fill_ops = true;
		}
	}

	if (priv->num_crtcs >= MAX_CRTCS) {
		SDE_ERROR("crtc reaches the maximum %d\n", priv->num_crtcs);
		rc = -ENOENT;
		goto end;
	}

	memset(&info, 0x0, sizeof(info));
	rc = shd_connector_get_info(NULL, &info, display);
	if (rc) {
		SDE_ERROR("shd get_info failed\n");
		goto end;
	}

	encoder = sde_encoder_init_with_ops(dev, &info, &enc_ops, NULL);
	if (IS_ERR_OR_NULL(encoder)) {
		SDE_ERROR("shd encoder init failed\n");
		rc = -ENOENT;
		goto end;
	}

	SDE_DEBUG("create encoder %d\n", DRMID(encoder));

	sde_kms = to_sde_kms(priv->kms);
	if (sde_kms->dsi_display_count > 0)
		display->dsi_base =  (struct dsi_display *)sde_kms->dsi_displays[0];

	connector = sde_connector_init(dev, encoder, dsi_display_get_drm_panel(display->dsi_base),
				       display, &shd_ops, DRM_CONNECTOR_POLL_HPD,
				       info.intf_type, true);
	if (connector) {
		priv->encoders[priv->num_encoders++] = encoder;
		priv->connectors[priv->num_connectors++] = connector;
	} else {
		SDE_ERROR("shd connector init failed\n");
		sde_encoder_destroy(encoder);
		rc = -ENOENT;
		goto end;
	}

	sde_conn = to_sde_connector(connector);
	sde_conn->shared = true;

	if (display->name)
		connector->name = kasprintf(GFP_KERNEL, "%s", display->name);

	if (info.intf_type == DRM_MODE_CONNECTOR_DSI)
		shd_display_create_backlight(connector);

	SDE_DEBUG("create connector %d\n", DRMID(connector));

	/* create primary plane for crtc */
	primary = sde_plane_init(dev, SSPP_DMA0, true, 0, 0);

	if (IS_ERR(primary))
		return -ENOMEM;

	SDE_DEBUG("created primary plane %d\n", DRMID(primary));
	priv->planes[priv->num_planes++] = primary;

	crtc = sde_crtc_init(dev, primary);
	if (IS_ERR(crtc)) {
		rc = PTR_ERR(crtc);
		goto end;
	}
	priv->crtcs[priv->num_crtcs++] = crtc;

	SDE_DEBUG("create crtc %d index %d\n", DRMID(crtc), drm_crtc_index(crtc));

	/* update encoder's possible crtcs */
	encoder->possible_crtcs = 1 << (priv->num_crtcs - 1);

	/* update plane's possible crtcs */
	for (i = 0; i < priv->num_planes; i++)
		priv->planes[i]->possible_crtcs |= 1 << (priv->num_crtcs - 1);

	/* update crtc's check function */
	shd_crtc = kzalloc(sizeof(*shd_crtc), GFP_KERNEL);
	if (!shd_crtc) {
		rc = -ENOMEM;
		goto end;
	}

	shd_crtc->helper_funcs = *crtc->helper_private;
	shd_crtc->orig_helper_funcs = crtc->helper_private;
	shd_crtc->helper_funcs.atomic_check = shd_crtc_atomic_check;
	shd_crtc->funcs = *crtc->funcs;
	shd_crtc->orig_funcs = crtc->funcs;
	shd_crtc->funcs.atomic_set_property = shd_crtc_atomic_set_property;
	shd_crtc->display = display;
	sde_crtc = to_sde_crtc(crtc);
	sde_crtc->priv_handle = shd_crtc;
	crtc->helper_private = &shd_crtc->helper_funcs;
	crtc->funcs = &shd_crtc->funcs;
	display->crtc = crtc;

end:
	return rc;
}

static int shd_drm_postinit(struct msm_kms *kms)
{
	struct shd_display_base *base;
	struct sde_connector *sde_conn;

	/* set base connector disconnected*/
	list_for_each_entry(base, &g_base_list, head) {
		sde_conn = to_sde_connector(base->connector);

		sde_conn->ops.set_info_blob = NULL;
		sde_connector_set_blob_data(&sde_conn->base, NULL, CONNECTOR_PROP_SDE_INFO);
	}

	return g_shd_kms->orig_funcs->postinit(kms);
}

static int shd_drm_base_init(struct drm_device *ddev, struct shd_display_base *base)
{
	struct msm_drm_private *priv;
	int rc;

	rc = shd_display_init_base_encoder(ddev, base);
	if (rc) {
		SDE_ERROR("failed to find base encoder\n");
		return rc;
	}

	rc = shd_display_init_base_connector(ddev, base);
	if (rc) {
		SDE_ERROR("failed to find base connector\n");
		return rc;
	}

	rc = shd_display_init_base_crtc(ddev, base);
	if (rc) {
		SDE_ERROR("failed to find base crtc\n");
		return rc;
	}

	if (!g_shd_kms) {
		priv = ddev->dev_private;
		g_shd_kms = kzalloc(sizeof(*g_shd_kms), GFP_KERNEL);
		if (!g_shd_kms)
			return -ENOMEM;

		g_shd_kms->funcs = *priv->kms->funcs;
		g_shd_kms->orig_funcs = priv->kms->funcs;
		g_shd_kms->funcs.atomic_check = shd_display_atomic_check;
		g_shd_kms->funcs.postinit = shd_drm_postinit;
		priv->kms->funcs = &g_shd_kms->funcs;
	}
	return rc;
}

static int shd_parse_display(struct shd_display *display)
{
	struct device_node *of_node = display->pdev->dev.of_node;
	struct device_node *of_src, *of_roi;
	u32 src_w, src_h, dst_x, dst_y, dst_w, dst_h;
	u32 range[2];
	int rc;

	display->base_of = of_parse_phandle(of_node, "qcom,shared-display-base", 0);
	if (!display->base_of) {
		SDE_ERROR("No base device present\n");
		rc = -ENODEV;
		goto error;
	}

	of_src = of_get_child_by_name(of_node, "qcom,shared-display-src-mode");
	if (!of_src) {
		SDE_DEBUG("full screen mode\n");

		display->full_screen = true;
		goto next;
	}

	rc = of_property_read_u32(of_src, "qcom,mode-h-active", &src_w);
	if (rc) {
		SDE_ERROR("Failed to parse h active\n");
		goto error;
	}

	rc = of_property_read_u32(of_src, "qcom,mode-v-active", &src_h);
	if (rc) {
		SDE_ERROR("Failed to parse v active\n");
		goto error;
	}
	of_roi = of_get_child_by_name(of_node, "qcom,shared-display-dst-mode");
	if (!of_roi) {
		SDE_ERROR("No roi mode present\n");
		rc = -ENODEV;
		goto error;
	}

	rc = of_property_read_u32(of_roi, "qcom,mode-x-offset", &dst_x);
	if (rc) {
		SDE_ERROR("Failed to parse x offset\n");
		goto error;
	}

	rc = of_property_read_u32(of_roi, "qcom,mode-y-offset", &dst_y);
	if (rc) {
		SDE_ERROR("Failed to parse y offset\n");
		goto error;
	}

	rc = of_property_read_u32(of_roi, "qcom,mode-width", &dst_w);
	if (rc) {
		SDE_ERROR("Failed to parse roi width\n");
		goto error;
	}

	rc = of_property_read_u32(of_roi, "qcom,mode-height", &dst_h);
	if (rc) {
		SDE_ERROR("Failed to parse roi height\n");
		goto error;
	}

	if (src_w != dst_w) {
		SDE_ERROR("horizontal scaling is not supported\n");
		goto error;
	}

	if (src_h > dst_h) {
		SDE_ERROR("downscale is not supported\n");
		goto error;
	}

	display->src.w = src_w;
	display->src.h = src_h;
	display->roi.x = dst_x;
	display->roi.y = dst_y;
	display->roi.w = dst_w;
	display->roi.h = dst_h;

	of_property_read_u32(of_node, "qcom,mode-width-mm", &display->info.width_mm);
	of_property_read_u32(of_node, "qcom,mode-height-mm", &display->info.height_mm);

next:
	rc = of_property_read_u32_array(of_node, "qcom,blend-stage-range", range, 2);
	if (rc)
		SDE_ERROR("Failed to parse blend stage range\n");

	display->stage_range.start = range[0];
	display->stage_range.size = range[1];

	display->name = of_get_property(of_node, "qcom,shared-display-name", NULL);

	SDE_DEBUG("%s src %dx%d dst %d,%d %dx%d range %d-%d\n", display->name,
		  display->src.w, display->src.h, display->roi.x, display->roi.y,
		  display->roi.w, display->roi.h, display->stage_range.start,
		  display->stage_range.size);

	display->display_type = of_get_property(of_node, "qcom,display-type", NULL);
	if (!display->display_type)
		display->display_type = "unknown";

error:
	return rc;
}

static int shd_parse_base(struct drm_device *drm_dev, struct shd_display_base *base)
{
	struct device_node *of_node = base->of_node;
	struct device_node *node = NULL;
	struct drm_display_mode *mode = &base->mode;
	u32 h_front_porch, h_pulse_width, h_back_porch;
	u32 v_front_porch, v_pulse_width, v_back_porch;
	bool h_active_high, v_active_high;
	bool tile_mode;
	struct drm_connector *connector;
	struct drm_connector_list_iter conn_iter;
	const char *name;
	u32 flags = 0;
	int rc;

	rc = of_property_read_u32(of_node, "qcom,shared-display-base-intf", &base->intf_idx);
	if (!rc) {
		base->mst_port = of_property_read_bool(of_node, "qcom,shared-display-base-mst");
	} else {
		rc = of_property_read_string(of_node, "qcom,shared-display-base-connector", &name);
		if (rc) {
			SDE_ERROR("failed to read base connector (%d)\n", rc);
			goto fail;
		}

		drm_connector_list_iter_begin(drm_dev, &conn_iter);
		drm_for_each_connector_iter(connector, &conn_iter) {
			if (!strcmp(connector->name, name)) {
				base->connector = connector;
				break;
			}
		}

		drm_connector_list_iter_end(&conn_iter);
		if (!base->connector) {
			SDE_ERROR("failed to find base connector %s\n", name);
			rc = -ENOENT;
			goto fail;
		}
	}

	node = of_get_child_by_name(of_node, "qcom,shared-display-base-mode");
	if (!node) {
		SDE_DEBUG("full screen mode\n");

		base->dynamic_mode = true;
		return 0;
	}

	rc = of_property_read_u16(node, "qcom,mode-h-active", &mode->hdisplay);
	if (rc) {
		SDE_ERROR("failed to read h-active, rc=%d\n", rc);
		goto fail;
	}

	rc = of_property_read_u32(node, "qcom,mode-h-front-porch", &h_front_porch);
	if (rc) {
		SDE_ERROR("failed to read h-front-porch, rc=%d\n", rc);
		goto fail;
	}

	rc = of_property_read_u32(node, "qcom,mode-h-pulse-width", &h_pulse_width);
	if (rc) {
		SDE_ERROR("failed to read h-pulse-width, rc=%d\n", rc);
		goto fail;
	}

	rc = of_property_read_u32(node, "qcom,mode-h-back-porch", &h_back_porch);
	if (rc) {
		SDE_ERROR("failed to read h-back-porch, rc=%d\n", rc);
		goto fail;
	}

	h_active_high = of_property_read_bool(node, "qcom,mode-h-active-high");

	rc = of_property_read_u16(node, "qcom,mode-v-active", &mode->vdisplay);
	if (rc) {
		SDE_ERROR("failed to read v-active, rc=%d\n", rc);
		goto fail;
	}

	rc = of_property_read_u32(node, "qcom,mode-v-front-porch", &v_front_porch);
	if (rc) {
		SDE_ERROR("failed to read v-front-porch, rc=%d\n", rc);
		goto fail;
	}

	rc = of_property_read_u32(node, "qcom,mode-v-pulse-width", &v_pulse_width);
	if (rc) {
		SDE_ERROR("failed to read v-pulse-width, rc=%d\n", rc);
		goto fail;
	}

	rc = of_property_read_u32(node, "qcom,mode-v-back-porch", &v_back_porch);
	if (rc) {
		SDE_ERROR("failed to read v-back-porch, rc=%d\n", rc);
		goto fail;
	}

	v_active_high = of_property_read_bool(node, "qcom,mode-v-active-high");

	rc = of_property_read_u32(node, "qcom,mode-clock-in-khz", &mode->clock);
	if (rc) {
		SDE_ERROR("failed to read clock, rc=%d\n", rc);
		goto fail;
	}

	tile_mode = of_property_read_bool(of_node, "qcom,mode-tile");
	of_property_read_u32(node, "qcom,mode-width-mm", &base->info.width_mm);
	of_property_read_u32(node, "qcom,mode-height-mm", &base->info.height_mm);

	mode->hsync_start = mode->hdisplay + h_front_porch;
	mode->hsync_end = mode->hsync_start + h_pulse_width;
	mode->htotal = mode->hsync_end + h_back_porch;
	mode->vsync_start = mode->vdisplay + v_front_porch;
	mode->vsync_end = mode->vsync_start + v_pulse_width;
	mode->vtotal = mode->vsync_end + v_back_porch;

	if (h_active_high)
		flags |= DRM_MODE_FLAG_PHSYNC;
	else
		flags |= DRM_MODE_FLAG_NHSYNC;

	if (v_active_high)
		flags |= DRM_MODE_FLAG_PVSYNC;
	else
		flags |= DRM_MODE_FLAG_NVSYNC;

	if (tile_mode)
		flags |= DRM_MODE_FLAG_CLKDIV2;

	mode->flags = flags;
	drm_mode_set_name(mode);

	SDE_DEBUG("base mode h[%d,%d,%d,%d] v[%d,%d,%d,%d] %x %d\n",
		  mode->hdisplay, mode->hsync_start,
		  mode->hsync_end, mode->htotal, mode->vdisplay,
		  mode->vsync_start, mode->vsync_end, mode->vtotal,
		  mode->flags, mode->clock);

fail:
	return rc;
}

static int shd_display_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
	struct shd_display *shd_dev;
	struct shd_display_base *base;
	int rc = 0;

	if (action != MSM_COMP_OBJECT_CREATED)
		return 0;

	shd_dev = container_of(nb, struct shd_display, notifier);

	list_for_each_entry(base, &g_base_list, head) {
		if (base->of_node == shd_dev->base_of)
			goto next;
	}

	base = devm_kzalloc(&shd_dev->pdev->dev, sizeof(*base), GFP_KERNEL);
	if (!base) {
		rc = -ENOMEM;
		goto error;
	}

	INIT_LIST_HEAD(&base->disp_list);
	base->of_node = shd_dev->base_of;

	rc = shd_parse_base(shd_dev->drm_dev, base);
	if (rc) {
		SDE_ERROR("failed to parse shared display base\n");
		goto error;
	}
	rc = shd_drm_base_init(shd_dev->drm_dev, base);
	if (rc) {
		SDE_ERROR("failed to init crtc for shared display base\n");
		goto error;
	}

	list_add_tail(&base->head, &g_base_list);

next:
	shd_dev->base = base;
	rc = shd_drm_obj_init(shd_dev);
	if (rc) {
		SDE_ERROR("failed to init shared drm objects\n");
		goto error;
	}

	list_add_tail(&shd_dev->head, &base->disp_list);
	SDE_DEBUG("add shd to intf %d\n", base->intf_idx);

error:
	return rc;
}

static int shd_display_bind(struct device *dev, struct device *master, void *data)
{
	int rc = 0;
	struct shd_display *shd_dev;
	struct drm_device *drm;
	struct platform_device *pdev = to_platform_device(dev);

	if (!dev || !pdev || !master) {
		pr_err("invalid param(s), dev %pK, pdev %pK, master %pK\n", dev, pdev, master);
		rc = -EINVAL;
		goto end;
	}

	drm = dev_get_drvdata(master);
	shd_dev = platform_get_drvdata(pdev);
	if (!drm || !shd_dev) {
		pr_err("invalid param(s), drm %pK, shd_dev %pK\n", drm, shd_dev);
		rc = -EINVAL;
		goto end;
	}

	shd_dev->drm_dev = drm;
	shd_dev->notifier.notifier_call = shd_display_notifier;

	rc = msm_drm_register_component(drm, &shd_dev->notifier);
	if (rc) {
		pr_err("failed to register component notifier\n");
		goto end;
	}

	SDE_DEBUG("register component\n");
end:
	return rc;
}

static void shd_display_unbind(struct device *dev, struct device *master,   void *data)
{
	struct shd_display *shd_dev;
	struct platform_device *pdev = to_platform_device(dev);

	if (!dev || !pdev) {
		pr_err("invalid param\n");
		return;
	}

	shd_dev = platform_get_drvdata(pdev);
	if (!shd_dev) {
		pr_err("invalid param\n");
		return;
	}

	msm_drm_unregister_component(shd_dev->drm_dev, &shd_dev->notifier);

	list_del_init(&shd_dev->head);
	if (list_empty(&shd_dev->base->disp_list))
		list_del_init(&shd_dev->base->head);
}

static const struct component_ops shd_display_comp_ops = {
	.bind = shd_display_bind,
	.unbind = shd_display_unbind,
};

/**
 * sde_shd_probe - load shared display module
 * @pdev:	Pointer to platform device
 */
static int sde_shd_probe(struct platform_device *pdev)
{
	struct shd_display *shd_dev;
	int ret;

	shd_dev = devm_kzalloc(&pdev->dev, sizeof(*shd_dev), GFP_KERNEL);
	if (!shd_dev)
		return -ENOMEM;

	shd_dev->pdev = pdev;

	ret = shd_parse_display(shd_dev);
	if (ret) {
		SDE_ERROR("failed to parse shared display\n");
		goto error;
	}

	platform_set_drvdata(pdev, shd_dev);

	ret = component_add(&pdev->dev, &shd_display_comp_ops);
	if (ret) {
		pr_err("component add failed, rc=%d\n", ret);
		goto error;
	}

error:
	return ret;
}

/**
 * sde_shd_remove - unload shared display module
 * @pdev:	Pointer to platform device
 */
static int sde_shd_remove(struct platform_device *pdev)
{
	struct shd_display *shd_dev;

	shd_dev = platform_get_drvdata(pdev);
	if (!shd_dev)
		return 0;

	list_del_init(&shd_dev->head);
	if (list_empty(&shd_dev->base->disp_list))
		list_del_init(&shd_dev->base->head);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "qcom,shared-display"},
	{},
};

static struct platform_driver sde_shd_driver = {
	.probe = sde_shd_probe,
	.remove = sde_shd_remove,
	.driver = {
		.name = "sde_shd",
		.of_match_table = dt_match,
		.suppress_bind_attrs = true,
	},
};

void __init sde_shd_register(void)
{
	platform_driver_register(&sde_shd_driver);
}

void __exit sde_shd_unregister(void)
{
	platform_driver_unregister(&sde_shd_driver);
}
