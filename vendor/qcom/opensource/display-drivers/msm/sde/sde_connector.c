// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__
#include "msm_drv.h"
#include "sde_dbg.h"

#include "sde_kms.h"
#include "sde_connector.h"
#include "sde_encoder.h"
#include "msm_cooling_device.h"
#include <linux/backlight.h>
#include <linux/string.h>
#include <linux/file.h>
#include "dsi_drm.h"
#include "sde_crtc.h"
#include "sde_rm.h"
#include "sde_vm.h"
#include <drm/drm_probe_helper.h>
#include <linux/version.h>
#include <shd_drm.h>
#include "sde_trace.h"

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
#include "ss_dsi_panel_common.h"
#endif

#define BL_NODE_NAME_SIZE 32
#define HDR10_PLUS_VSIF_TYPE_CODE      0x81

/* Autorefresh will occur after FRAME_CNT frames. Large values are unlikely */
#define AUTOREFRESH_MAX_FRAME_CNT 6

#define SDE_DEBUG_CONN(c, fmt, ...) SDE_DEBUG("conn%d " fmt,\
		(c) ? (c)->base.base.id : -1, ##__VA_ARGS__)

#define SDE_ERROR_CONN(c, fmt, ...) SDE_ERROR("conn%d " fmt,\
		(c) ? (c)->base.base.id : -1, ##__VA_ARGS__)

static const struct drm_prop_enum_list e_topology_name[] = {
	{SDE_RM_TOPOLOGY_NONE,	"sde_none"},
	{SDE_RM_TOPOLOGY_SINGLEPIPE,	"sde_singlepipe"},
	{SDE_RM_TOPOLOGY_SINGLEPIPE_DSC,	"sde_singlepipe_dsc"},
	{SDE_RM_TOPOLOGY_SINGLEPIPE_VDC,	"sde_singlepipe_vdc"},
	{SDE_RM_TOPOLOGY_DUALPIPE,	"sde_dualpipe"},
	{SDE_RM_TOPOLOGY_DUALPIPE_DSC,	"sde_dualpipe_dsc"},
	{SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE,	"sde_dualpipemerge"},
	{SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE_DSC,	"sde_dualpipemerge_dsc"},
	{SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE_VDC,	"sde_dualpipemerge_vdc"},
	{SDE_RM_TOPOLOGY_DUALPIPE_DSCMERGE,	"sde_dualpipe_dscmerge"},
	{SDE_RM_TOPOLOGY_PPSPLIT,	"sde_ppsplit"},
	{SDE_RM_TOPOLOGY_QUADPIPE_3DMERGE,	"sde_quadpipemerge"},
	{SDE_RM_TOPOLOGY_QUADPIPE_3DMERGE_DSC,	"sde_quadpipe_3dmerge_dsc"},
	{SDE_RM_TOPOLOGY_QUADPIPE_DSCMERGE,	"sde_quadpipe_dscmerge"},
	{SDE_RM_TOPOLOGY_QUADPIPE_DSC4HSMERGE,	"sde_quadpipe_dsc4hsmerge"},
};
static const struct drm_prop_enum_list e_topology_control[] = {
	{SDE_RM_TOPCTL_RESERVE_LOCK,	"reserve_lock"},
	{SDE_RM_TOPCTL_RESERVE_CLEAR,	"reserve_clear"},
	{SDE_RM_TOPCTL_DSPP,		"dspp"},
	{SDE_RM_TOPCTL_DS,		"ds"},
	{SDE_RM_TOPCTL_DNSC_BLUR,	"dnsc_blur"},
	{SDE_RM_TOPCTL_CDM,		"cdm"},
};
static const struct drm_prop_enum_list e_power_mode[] = {
	{SDE_MODE_DPMS_ON,	"ON"},
	{SDE_MODE_DPMS_LP1,	"LP1"},
	{SDE_MODE_DPMS_LP2,	"LP2"},
	{SDE_MODE_DPMS_OFF,	"OFF"},
};
static const struct drm_prop_enum_list e_qsync_mode[] = {
	{SDE_RM_QSYNC_DISABLED,	"none"},
	{SDE_RM_QSYNC_CONTINUOUS_MODE,	"continuous"},
	{SDE_RM_QSYNC_ONE_SHOT_MODE,	"one_shot"},
};
static const struct drm_prop_enum_list e_dsc_mode[] = {
	{MSM_DISPLAY_DSC_MODE_NONE, "none"},
	{MSM_DISPLAY_DSC_MODE_ENABLED, "dsc_enabled"},
	{MSM_DISPLAY_DSC_MODE_DISABLED, "dsc_disabled"},
};
static const struct drm_prop_enum_list e_frame_trigger_mode[] = {
	{FRAME_DONE_WAIT_DEFAULT, "default"},
	{FRAME_DONE_WAIT_SERIALIZE, "serialize_frame_trigger"},
	{FRAME_DONE_WAIT_POSTED_START, "posted_start"},
};
static const struct drm_prop_enum_list e_panel_mode[] = {
	{MSM_DISPLAY_VIDEO_MODE, "video_mode"},
	{MSM_DISPLAY_CMD_MODE, "command_mode"},
	{MSM_DISPLAY_MODE_MAX, "none"},
};
static const struct drm_prop_enum_list e_bpp_mode[] = {
	{MSM_DISPLAY_PIXEL_FORMAT_NONE, "none"},
	{MSM_DISPLAY_PIXEL_FORMAT_RGB888, "dsi_24bpp"},
	{MSM_DISPLAY_PIXEL_FORMAT_RGB101010, "dsi_30bpp"},
};

struct dsi_display *_sde_connector_get_display(struct sde_connector *c_conn)
{
	struct dsi_display *display = NULL;
	struct shd_display *shd_display;

	if (!c_conn)
		return 0;

	if (c_conn->shared) {
		shd_display = c_conn->display;
		display = shd_display->dsi_base;
	} else {
		display = (struct dsi_display *)c_conn->display;
	}

	return display;
}

static void sde_dimming_bl_notify(struct sde_connector *conn, struct dsi_backlight_config *config)
{
	struct drm_event event;
	struct drm_msm_backlight_info bl_info;

	if (!conn || !config)
		return;

	SDE_DEBUG("bl_config.dimming_status 0x%x user_disable_notify %d\n",
		  config->dimming_status, config->user_disable_notification);
	if (!conn->dimming_bl_notify_enabled || config->user_disable_notification)
		return;

	bl_info.brightness_max = config->brightness_max_level;
	bl_info.brightness = config->brightness;
	bl_info.bl_level_max = config->bl_max_level;
	bl_info.bl_level = config->bl_level;
	bl_info.bl_scale = config->bl_scale;
	bl_info.bl_scale_sv = config->bl_scale_sv;
	bl_info.status = config->dimming_status;
	bl_info.min_bl = config->dimming_min_bl;
	bl_info.bl_scale_max = MAX_BL_SCALE_LEVEL;
	bl_info.bl_scale_sv_max = SV_BL_SCALE_CAP;
	event.type = DRM_EVENT_DIMMING_BL;
	event.length = sizeof(bl_info);
	SDE_DEBUG("dimming BL event bl_level %d bl_scale %d, bl_scale_sv = %d "
		  "min_bl %d status 0x%x\n", bl_info.bl_level, bl_info.bl_scale,
		  bl_info.bl_scale_sv, bl_info.min_bl, bl_info.status);
	msm_mode_object_event_notify(&conn->base.base, conn->base.dev, &event, (u8 *)&bl_info);
}

static int sde_backlight_set_notify(struct sde_connector *c_conn, int brightness, u32 bl_lvl)
{
	struct drm_event event;
	struct dsi_display *display;
	int rc = 0;

	if (!c_conn) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	} else if (brightness != 0) {
		event.type = DRM_EVENT_SYS_BACKLIGHT;
		event.length = sizeof(u32);
		msm_mode_object_event_notify(&c_conn->base.base,
			c_conn->base.dev, &event, (u8 *)&brightness);
	}

	display = (struct dsi_display *) c_conn->display;
	rc = c_conn->ops.set_backlight(&c_conn->base, c_conn->display, bl_lvl);
	if (!rc) {
		SDE_EVT32(DRMID(&c_conn->base), bl_lvl);
		if (c_conn->num_bl_frames)
			display->panel->bl_config.brightness = brightness;
		sde_dimming_bl_notify(c_conn, &display->panel->bl_config);
	}

	return rc;
}

static int sde_connector_apply_incremental_bl(struct sde_connector *c_conn)
{
	int diff_brighness = 0, prev_brightness, updated_brightness, new_brightness;
	int diff_bl_lvl, prev_bl_lvl, updated_bl_lvl, new_bl_lvl;
	struct sde_kms *sde_kms;
	struct drm_encoder *drm_enc;
	struct drm_connector *connector;
	struct sde_encoder_virt *sde_enc;
	int rc = 0;

	if (!c_conn) {
		pr_err("invalid params\n");
		return -EINVAL;
	} else if (!c_conn->bl_vrr.bl_increment_in_progress) {
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE1);
		return 0;
	}

	sde_kms = sde_connector_get_kms(&c_conn->base);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	}
	connector = &c_conn->base;
	if (connector->state && connector->state->best_encoder)
		drm_enc = connector->state->best_encoder;
	else
		drm_enc = connector->encoder;

	sde_enc = to_sde_encoder_virt(drm_enc);
	/* If peripheral flush in progress update in next interval*/
	if (sde_enc && (sde_enc->vrr_info.vhm_cmd_in_progress == SDE_CMD_SCHEDULED)) {
		sde_encoder_handle_next_backlight_update(drm_enc);
		SDE_EVT32(sde_enc->vrr_info.vhm_cmd_in_progress);
		return 0;
	}

	c_conn->bl_vrr.bl_frame_idx++;

	new_brightness = c_conn->bl_vrr.new_brightness;
	prev_brightness = c_conn->bl_vrr.prev_brightness;
	new_bl_lvl = c_conn->bl_vrr.new_bl_lvl;
	prev_bl_lvl = c_conn->bl_vrr.prev_bl_lvl;

	if (c_conn->bl_vrr.bl_frame_idx == c_conn->num_bl_frames) {
		updated_brightness = new_brightness;
		updated_bl_lvl = new_bl_lvl;
	} else if (new_brightness > prev_brightness) {
		diff_brighness = mult_frac((u32)(new_brightness - prev_brightness),
			c_conn->bl_vrr.bl_frame_idx, c_conn->num_bl_frames);
		updated_brightness = prev_brightness + diff_brighness;
		diff_bl_lvl = mult_frac((u32)(new_bl_lvl - prev_bl_lvl),
			c_conn->bl_vrr.bl_frame_idx, c_conn->num_bl_frames);
		updated_bl_lvl = prev_bl_lvl + diff_bl_lvl;
	} else {
		diff_brighness = mult_frac((u32)(prev_brightness - new_brightness),
			c_conn->bl_vrr.bl_frame_idx, c_conn->num_bl_frames);
		updated_brightness = prev_brightness - diff_brighness;
		diff_bl_lvl = mult_frac((u32)(prev_bl_lvl - new_bl_lvl),
			c_conn->bl_vrr.bl_frame_idx, c_conn->num_bl_frames);
		updated_bl_lvl = prev_bl_lvl - diff_bl_lvl;
	}

	SDE_EVT32(DRMID(&c_conn->base), new_brightness, prev_brightness, new_bl_lvl, prev_bl_lvl,
		c_conn->bl_vrr.bl_frame_idx, updated_brightness, updated_bl_lvl, diff_brighness,
		c_conn->num_bl_frames);

	SDE_ATRACE_BEGIN("smooth_dimming_backlight_update");
	rc = sde_backlight_set_notify(c_conn, updated_brightness, updated_bl_lvl);
	SDE_ATRACE_END("smooth_dimming_backlight_update");

	if (rc) {
		SDE_ERROR("Backlight set notify failed\n");
		return rc;
	}

	c_conn->bl_vrr.curr_brightness = updated_brightness;
	c_conn->bl_vrr.curr_bl_lvl = updated_bl_lvl;

	if (c_conn->bl_vrr.bl_frame_idx >= c_conn->num_bl_frames) {
		c_conn->bl_vrr.bl_frame_idx = 0;
		c_conn->bl_vrr.bl_increment_in_progress = false;
		c_conn->bl_vrr.prev_brightness = new_brightness;
		c_conn->bl_vrr.prev_bl_lvl = new_bl_lvl;
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE2);
	} else if (c_conn->bl_vrr.bl_increment_in_progress) {
		sde_encoder_handle_next_backlight_update(drm_enc);
	}

	return rc;
}

static bool sde_connector_is_cont_bl_updates(struct sde_connector *c_conn)
{
	bool rc = false;
	u64 curr_time, nominal_vsync_ns;
	struct sde_encoder_virt *sde_enc;
	struct sde_kms *sde_kms;

	sde_enc = to_sde_encoder_virt(c_conn->encoder);
	curr_time = ktime_get();
	if (!c_conn->encoder || !sde_enc->mode_info.frame_rate || !c_conn->bl_vrr.prev_bl_time_ns) {
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE1, !c_conn->encoder, !sde_enc->mode_info.frame_rate,
			!c_conn->bl_vrr.prev_bl_time_ns);
		c_conn->bl_vrr.prev_bl_time_ns = curr_time;
		return true;
	}

	sde_kms = sde_connector_get_kms(&c_conn->base);
	nominal_vsync_ns = SEC_TO_NS / sde_enc->mode_info.frame_rate;
	if (ktime_sub(curr_time, c_conn->bl_vrr.prev_bl_time_ns) <
			(c_conn->num_bl_frames * nominal_vsync_ns)) {
		SDE_EVT32(ktime_to_us(curr_time), ktime_to_us(c_conn->bl_vrr.prev_bl_time_ns),
			ktime_to_us(nominal_vsync_ns));
		rc = true;
	}

	c_conn->bl_vrr.prev_bl_time_ns = curr_time;
	return rc;
}

static int sde_connector_begin_incremental_bl(struct sde_connector *c_conn, int brightness,
		u32 bl_lvl)
{
	int rc = 0;
	struct drm_encoder *drm_enc;
	struct drm_connector *connector;
	struct sde_encoder_virt *sde_enc;

	connector = &c_conn->base;
	if (connector->state && connector->state->best_encoder)
		drm_enc = connector->state->best_encoder;
	else
		drm_enc = connector->encoder;

	c_conn->bl_vrr.new_incremental_bl_update = true;
	mutex_lock(&c_conn->bl_vrr.bl_lock);
	/* first frame after suspend/init */
	if (c_conn->bl_vrr.prev_brightness == 0 ||
			c_conn->last_panel_power_mode == SDE_MODE_DPMS_OFF) {
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE1, c_conn->bl_vrr.prev_brightness,
			c_conn->last_panel_power_mode);
		goto skip_incremental_update;
	} else if (c_conn->disable_cont_dimming &&
		sde_connector_is_cont_bl_updates(c_conn)) {
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE2, c_conn->bl_vrr.curr_brightness,
			c_conn->bl_vrr.curr_bl_lvl);
		goto skip_incremental_update;
	}

	if (c_conn->bl_vrr.bl_increment_in_progress) {
		sde_encoder_phys_cancel_backlight_timer(drm_enc);
		c_conn->bl_vrr.prev_brightness = c_conn->bl_vrr.curr_brightness;
		c_conn->bl_vrr.prev_bl_lvl = c_conn->bl_vrr.curr_bl_lvl;
	}

	c_conn->bl_vrr.new_brightness = brightness;
	c_conn->bl_vrr.new_bl_lvl = bl_lvl;
	c_conn->bl_vrr.bl_increment_in_progress = true;
	c_conn->bl_vrr.bl_frame_idx = 0;

	SDE_EVT32(bl_lvl, brightness, c_conn->bl_vrr.curr_bl_lvl,
		c_conn->bl_vrr.curr_brightness, c_conn->bl_vrr.bl_increment_in_progress,
		c_conn->bl_vrr.prev_bl_lvl, c_conn->bl_vrr.prev_bl_lvl);

	sde_enc = to_sde_encoder_virt(drm_enc);
	if (sde_enc && kthread_cancel_delayed_work_sync(&sde_enc->delayed_off_work)) {
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE3);
		sde_encoder_rc_restart_delayed(sde_enc, 1);
	}

	sde_encoder_handle_next_backlight_update(drm_enc);
	mutex_unlock(&c_conn->bl_vrr.bl_lock);
	c_conn->bl_vrr.new_incremental_bl_update = false;
	return rc;

skip_incremental_update:
	sde_encoder_phys_cancel_backlight_timer(drm_enc);
	c_conn->bl_vrr.bl_increment_in_progress = false;

	rc = sde_backlight_set_notify(c_conn, brightness, bl_lvl);
	if (!rc) {
		c_conn->bl_vrr.bl_frame_idx = 0;
		c_conn->bl_vrr.prev_brightness = brightness;
		c_conn->bl_vrr.prev_bl_lvl = bl_lvl;
		c_conn->bl_vrr.curr_brightness = brightness;
		c_conn->bl_vrr.curr_bl_lvl = bl_lvl;
		SDE_EVT32(brightness, bl_lvl);
	}
	mutex_unlock(&c_conn->bl_vrr.bl_lock);
	return rc;
}

static int sde_backlight_device_update_status(struct backlight_device *bd)
{
	int brightness;
	struct dsi_display *display;
	struct sde_connector *c_conn = bl_get_data(bd);
	int bl_lvl;
	int rc = 0;
	struct sde_kms *sde_kms;

	sde_kms = sde_connector_get_kms(&c_conn->base);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	}

	brightness = bd->props.brightness;
#if defined(CONFIG_DISPLAY_SAMSUNG)
	SDE_INFO("bl update %d\n", brightness);
#endif

	if ((bd->props.power != FB_BLANK_UNBLANK) ||
			(bd->props.state & BL_CORE_FBBLANK) ||
			(bd->props.state & BL_CORE_SUSPENDED))
		brightness = 0;

	display = _sde_connector_get_display(c_conn);
	if (!display)
		return 0;

	if (brightness > display->panel->bl_config.brightness_max_level)
		brightness = display->panel->bl_config.brightness_max_level;
	if (brightness > c_conn->thermal_max_brightness)
		brightness = c_conn->thermal_max_brightness;

	display->panel->bl_config.brightness = brightness;
	/* map UI brightness into driver backlight level with rounding */
	bl_lvl = mult_frac(brightness, display->panel->bl_config.bl_max_level,
			display->panel->bl_config.brightness_max_level);

	if (!bl_lvl && brightness)
		bl_lvl = 1;

	if (!c_conn->allow_bl_update) {
		c_conn->unset_bl_level = bl_lvl;
#if defined(CONFIG_DISPLAY_SAMSUNG)
		SDE_ERROR("set unset_bl_level (%d)\n", c_conn->unset_bl_level);
#endif
		return 0;
	}
	SDE_ATRACE_INT("dimming", c_conn->bl_vrr.bl_increment_in_progress);
	SDE_ATRACE_BEGIN("backlight_update");
	SDE_EVT32(bl_lvl, brightness, c_conn->num_bl_frames);

	sde_vm_lock(sde_kms);

	if (!sde_vm_owns_hw(sde_kms)) {
		SDE_DEBUG("skipping bl update due to HW unavailablity\n");
#if defined(CONFIG_DISPLAY_SAMSUNG)
		/*************************************************
		 SS is using ss_send_cmd
		 Hold vm_lock in ss_send_cmd when send a command.
		**************************************************/
		sde_vm_unlock(sde_kms);
#endif
		goto done;
	}
#if defined(CONFIG_DISPLAY_SAMSUNG)
	sde_vm_unlock(sde_kms);
#endif

	if (c_conn->ops.set_backlight) {
		/* skip notifying user space if bl is 0 */
		if (c_conn->num_bl_frames && c_conn->frame_interval)
			sde_connector_begin_incremental_bl(c_conn, brightness, bl_lvl);
		else
			sde_backlight_set_notify(c_conn, brightness, bl_lvl);

		c_conn->unset_bl_level = 0;
	}

done:
#if !defined(CONFIG_DISPLAY_SAMSUNG)
	sde_vm_unlock(sde_kms);
#endif
	SDE_ATRACE_END("backlight_update");

	return rc;
}

static int sde_backlight_device_get_brightness(struct backlight_device *bd)
{
	return 0;
}

static const struct backlight_ops sde_backlight_device_ops = {
	.update_status = sde_backlight_device_update_status,
	.get_brightness = sde_backlight_device_get_brightness,
};

static int sde_backlight_cooling_cb(struct notifier_block *nb,
					unsigned long val, void *data)
{
	struct sde_connector *c_conn;
	struct backlight_device *bd = (struct backlight_device *)data;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	// Do not control brightness from termal sensor
	SDE_INFO("bl: thermal max brightness cap:%lu : Do not support\n", val);
	return 0;
#endif

	c_conn = bl_get_data(bd);
	SDE_DEBUG("bl: thermal max brightness cap:%lu\n", val);
	c_conn->thermal_max_brightness = val;

	sde_backlight_device_update_status(bd);
	return 0;
}

static int sde_backlight_setup(struct sde_connector *c_conn,
					struct drm_device *dev)
{
	struct backlight_properties props;
	struct dsi_display *display;
	struct dsi_backlight_config *bl_config;
	struct sde_kms *sde_kms;
	static int display_count;

	char bl_node_name[BL_NODE_NAME_SIZE];

	sde_kms = sde_connector_get_kms(&c_conn->base);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	} else if (!c_conn->ops.set_backlight) {
		return 0;
	}

	display = _sde_connector_get_display(c_conn);
	if (!display)
		return 0;

	bl_config = &display->panel->bl_config;

	if (bl_config->type != DSI_BACKLIGHT_DCS &&
		sde_in_trusted_vm(sde_kms))
		return 0;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.power = FB_BLANK_UNBLANK;
	props.max_brightness = bl_config->brightness_max_level;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	props.brightness = bl_config->bl_level;
#else
	props.brightness = bl_config->brightness_max_level;
#endif
	snprintf(bl_node_name, BL_NODE_NAME_SIZE, "panel%u-backlight",
							display_count);
	c_conn->bl_device = backlight_device_register(bl_node_name, dev->dev, c_conn,
			&sde_backlight_device_ops, &props);
	if (IS_ERR_OR_NULL(c_conn->bl_device)) {
		SDE_ERROR("Failed to register backlight: %ld\n",
				    PTR_ERR(c_conn->bl_device));
		c_conn->bl_device = NULL;
		return -ENODEV;
	}
	c_conn->thermal_max_brightness = bl_config->brightness_max_level;

	/**
	 * In TVM, thermal cooling device is not enabled. Registering with dummy
	 * thermal device will return a NULL leading to a failure. So skip it.
	 */
	if (sde_in_trusted_vm(sde_kms))
		goto done;

	c_conn->n.notifier_call = sde_backlight_cooling_cb;
	c_conn->cdev = backlight_cdev_register(dev->dev, c_conn->bl_device,
							&c_conn->n);
	if (IS_ERR_OR_NULL(c_conn->cdev)) {
		SDE_INFO("Failed to register backlight cdev: %ld\n",
				    PTR_ERR(c_conn->cdev));
		c_conn->cdev = NULL;
	}
done:
	display_count++;

	return 0;
}

int sde_connector_trigger_event(void *drm_connector,
		uint32_t event_idx, uint32_t instance_idx,
		uint32_t data0, uint32_t data1,
		uint32_t data2, uint32_t data3)
{
	struct sde_connector *c_conn;
	unsigned long irq_flags;
	int (*cb_func)(uint32_t event_idx,
			uint32_t instance_idx, void *usr,
			uint32_t data0, uint32_t data1,
			uint32_t data2, uint32_t data3);
	void *usr;
	int rc = 0;

	/*
	 * This function may potentially be called from an ISR context, so
	 * avoid excessive logging/etc.
	 */
	if (!drm_connector)
		return -EINVAL;
	else if (event_idx >= SDE_CONN_EVENT_COUNT)
		return -EINVAL;
	c_conn = to_sde_connector(drm_connector);

	spin_lock_irqsave(&c_conn->event_lock, irq_flags);
	cb_func = c_conn->event_table[event_idx].cb_func;
	usr = c_conn->event_table[event_idx].usr;
	spin_unlock_irqrestore(&c_conn->event_lock, irq_flags);

	if (cb_func)
		rc = cb_func(event_idx, instance_idx, usr,
			data0, data1, data2, data3);
	else
		rc = -EAGAIN;

	return rc;
}

int sde_connector_register_event(struct drm_connector *connector,
		uint32_t event_idx,
		int (*cb_func)(uint32_t event_idx,
			uint32_t instance_idx, void *usr,
			uint32_t data0, uint32_t data1,
			uint32_t data2, uint32_t data3),
		void *usr)
{
	struct sde_connector *c_conn;
	unsigned long irq_flags;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	} else if (event_idx >= SDE_CONN_EVENT_COUNT) {
		SDE_ERROR("conn%d, invalid event %d\n",
				connector->base.id, event_idx);
		return -EINVAL;
	}
	c_conn = to_sde_connector(connector);

	spin_lock_irqsave(&c_conn->event_lock, irq_flags);
	c_conn->event_table[event_idx].cb_func = cb_func;
	c_conn->event_table[event_idx].usr = usr;
	spin_unlock_irqrestore(&c_conn->event_lock, irq_flags);

	/* optionally notify display of event registration */
	if (c_conn->ops.enable_event && c_conn->display)
		c_conn->ops.enable_event(connector, event_idx, cb_func != NULL,
					_sde_connector_get_display(c_conn));
	return 0;
}

void sde_connector_unregister_event(struct drm_connector *connector,
		uint32_t event_idx)
{
	(void)sde_connector_register_event(connector, event_idx, 0, 0);
}

static void _sde_connector_install_dither_property(struct drm_device *dev,
		struct sde_kms *sde_kms, struct sde_connector *c_conn)
{
	char prop_name[DRM_PROP_NAME_LEN];
	struct sde_mdss_cfg *catalog = NULL;
	u32 version = 0;

	if (!dev || !sde_kms || !c_conn) {
		SDE_ERROR("invld args (s), dev %pK, sde_kms %pK, c_conn %pK\n",
				dev, sde_kms, c_conn);
		return;
	}

	catalog = sde_kms->catalog;
	version = SDE_COLOR_PROCESS_MAJOR(
			catalog->pingpong[0].sblk->dither.version);
	snprintf(prop_name, ARRAY_SIZE(prop_name), "%s%d",
			"SDE_PP_DITHER_V", version);
	switch (version) {
	case 1:
	case 2:
		msm_property_install_blob(&c_conn->property_info, prop_name,
			DRM_MODE_PROP_BLOB,
			CONNECTOR_PROP_PP_DITHER);
		break;
	default:
		SDE_ERROR("unsupported dither version %d\n", version);
		return;
	}
}

int sde_connector_get_dither_cfg(struct drm_connector *conn,
			struct drm_connector_state *state, void **cfg,
			size_t *len, bool idle_pc)
{
	struct sde_connector *c_conn = NULL;
	struct sde_connector_state *c_state = NULL;
	size_t dither_sz = 0;
	bool is_dirty;
	u32 *p = (u32 *)cfg;

	if (!conn || !state || !p) {
		SDE_ERROR("invalid arguments\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(conn);
	c_state = to_sde_connector_state(state);

	is_dirty = msm_property_is_dirty(&c_conn->property_info,
			&c_state->property_state,
			CONNECTOR_PROP_PP_DITHER);

	if (!is_dirty && !idle_pc) {
		return -ENODATA;
	} else if (is_dirty || idle_pc) {
		*cfg = msm_property_get_blob(&c_conn->property_info,
				&c_state->property_state,
				&dither_sz,
				CONNECTOR_PROP_PP_DITHER);
		/*
		 * in idle_pc use case return early,
		 * when dither is already disabled.
		 */
		if (idle_pc && *cfg == NULL)
			return -ENODATA;
		/* disable dither based on user config data */
		else if (*cfg == NULL)
			return 0;
	}
	*len = dither_sz;
	return 0;
}

static void sde_connector_get_avail_res_info(struct drm_connector *conn,
		struct msm_resource_caps_info *avail_res)
{
	struct sde_kms *sde_kms;
	struct drm_encoder *drm_enc = NULL;
	struct sde_connector *sde_conn;

	sde_kms = sde_connector_get_kms(conn);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return;
	}

	sde_conn = to_sde_connector(conn);
	if (conn->state && conn->state->best_encoder)
		drm_enc = conn->state->best_encoder;
	else
		drm_enc = conn->encoder;

	sde_rm_get_resource_info(&sde_kms->rm, drm_enc, avail_res);
	if ((sde_kms->catalog->allowed_dsc_reservation_switch &
		SDE_DP_DSC_RESERVATION_SWITCH) &&
		sde_conn->connector_type == DRM_MODE_CONNECTOR_DisplayPort)
		avail_res->num_dsc = sde_kms->catalog->dsc_count;

	avail_res->max_mixer_width = sde_kms->catalog->max_mixer_width;
}

void sde_connector_get_avail_res_info_shd(struct drm_connector *conn,
					  struct msm_resource_caps_info *avail_res)
{
	sde_connector_get_avail_res_info(conn, avail_res);
}

int sde_connector_set_msm_mode(struct drm_connector_state *conn_state,
				struct drm_display_mode *adj_mode)
{
	struct sde_connector_state *c_state;

	c_state = to_sde_connector_state(conn_state);
	if (!c_state || !adj_mode) {
		SDE_ERROR("invalid params c_state: %d, adj_mode %d\n",
				c_state == NULL, adj_mode == NULL);
		return -EINVAL;
	}

	c_state->msm_mode.base = adj_mode;
	return 0;
}

int sde_connector_get_lm_cnt_from_topology(struct drm_connector *conn,
		const struct drm_display_mode *drm_mode)
{
	struct sde_connector *c_conn;

	c_conn = to_sde_connector(conn);

	if (!c_conn || c_conn->connector_type != DRM_MODE_CONNECTOR_DSI ||
		!c_conn->ops.get_num_lm_from_mode)
		return -EINVAL;

	return c_conn->ops.get_num_lm_from_mode(c_conn->display, drm_mode);
}

int sde_connector_get_mode_info(struct drm_connector *conn,
		const struct drm_display_mode *drm_mode,
		struct msm_sub_mode *sub_mode,
		struct msm_mode_info *mode_info)
{
	struct sde_connector *sde_conn;
	struct msm_resource_caps_info avail_res;

	memset(&avail_res, 0, sizeof(avail_res));

	sde_conn = to_sde_connector(conn);

	if (!sde_conn)
		return -EINVAL;

	sde_connector_get_avail_res_info(conn, &avail_res);

	return sde_conn->ops.get_mode_info(conn, drm_mode, sub_mode,
			mode_info, sde_conn->display, &avail_res);
}

int sde_connector_state_get_mode_info(struct drm_connector_state *conn_state,
	struct msm_mode_info *mode_info)
{
	struct sde_connector_state *sde_conn_state = NULL;

	if (!conn_state || !mode_info) {
		SDE_ERROR("Invalid arguments\n");
		return -EINVAL;
	}

	sde_conn_state = to_sde_connector_state(conn_state);
	memcpy(mode_info, &sde_conn_state->mode_info,
		sizeof(sde_conn_state->mode_info));

	return 0;
}

static int sde_connector_handle_panel_id(uint32_t event_idx,
			uint32_t instance_idx, void *usr,
			uint32_t data0, uint32_t data1,
			uint32_t data2, uint32_t data3)
{
	struct sde_connector *c_conn = usr;
	int i;
	u64 panel_id;
	u8 msb_arr[8];

	if (!c_conn)
		return -EINVAL;

	panel_id = (((u64)data0) << 32) | data1;
	if (panel_id == ~0x0)
		return 0;

	for (i = 0; i < 8; i++)
		msb_arr[i] = (panel_id >> (8 * (7 - i)));

	/* update the panel id */
	msm_property_set_blob(&c_conn->property_info,
		  &c_conn->blob_panel_id, &msb_arr,  sizeof(msb_arr),
		  CONNECTOR_PROP_DEMURA_PANEL_ID);
	sde_connector_register_event(&c_conn->base,
			SDE_CONN_EVENT_PANEL_ID, NULL, c_conn);
	return 0;
}

static int sde_connector_handle_disp_recovery(uint32_t event_idx,
			uint32_t instance_idx, void *usr,
			uint32_t data0, uint32_t data1,
			uint32_t data2, uint32_t data3)
{
	struct sde_connector *c_conn = usr;
	int rc = 0;

	if (!c_conn)
		return -EINVAL;

	rc = sde_kms_handle_recovery(c_conn->encoder);

	return rc;
}

int sde_connector_get_info(struct drm_connector *connector,
		struct msm_display_info *info)
{
	struct sde_connector *c_conn;

	if (!connector || !info) {
		SDE_ERROR("invalid argument(s), conn %pK, info %pK\n",
				connector, info);
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);

	if (!c_conn->display || !c_conn->ops.get_info) {
		SDE_ERROR("display info not supported for %pK\n",
				c_conn->display);
		return -EINVAL;
	}

	return c_conn->ops.get_info(&c_conn->base, info, c_conn->display);
}

void sde_connector_schedule_status_work(struct drm_connector *connector,
		bool en)
{
	struct sde_connector *c_conn;
	struct msm_display_info info;

	c_conn = to_sde_connector(connector);
	if (!c_conn)
		return;

	/* Return if there is no change in ESD status check condition */
	if (en == c_conn->esd_status_check)
		return;

	sde_connector_get_info(connector, &info);
	if (c_conn->ops.check_status &&
		(info.capabilities & MSM_DISPLAY_ESD_ENABLED)) {
		if (en) {
			u32 interval;

			/*
			 * If debugfs property is not set then take
			 * default value
			 */
			interval = c_conn->esd_status_interval ?
				c_conn->esd_status_interval :
					STATUS_CHECK_INTERVAL_MS;
#if !IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
			/* Schedule ESD status check */
			schedule_delayed_work(&c_conn->status_work,
				msecs_to_jiffies(interval));
#endif
			c_conn->esd_status_check = true;
		} else {
			/* Cancel any pending ESD status check */
			cancel_delayed_work_sync(&c_conn->status_work);
			c_conn->esd_status_check = false;
		}
	}
}

static int _sde_connector_update_power_locked(struct sde_connector *c_conn)
{
	struct drm_connector *connector;
	void *display;
	int (*set_power)(struct drm_connector *conn, int status, void *disp);
	int mode, rc = 0;

	if (!c_conn)
		return -EINVAL;
	connector = &c_conn->base;

	switch (c_conn->dpms_mode) {
	case DRM_MODE_DPMS_ON:
		mode = c_conn->lp_mode;
		break;
	case DRM_MODE_DPMS_STANDBY:
		mode = SDE_MODE_DPMS_STANDBY;
		break;
	case DRM_MODE_DPMS_SUSPEND:
		mode = SDE_MODE_DPMS_SUSPEND;
		break;
	case DRM_MODE_DPMS_OFF:
		mode = SDE_MODE_DPMS_OFF;
		break;
	default:
		mode = c_conn->lp_mode;
		SDE_ERROR("conn %d dpms set to unrecognized mode %d\n",
				connector->base.id, mode);
		break;
	}

	SDE_EVT32(connector->base.id, c_conn->dpms_mode, c_conn->lp_mode, mode,
		c_conn->last_panel_power_mode);
	SDE_DEBUG("conn %d - dpms %d, lp %d, panel %d\n", connector->base.id,
			c_conn->dpms_mode, c_conn->lp_mode, mode);

	if (mode == SDE_MODE_DPMS_ON)
		c_conn->vrr_cmd_state = VRR_CMD_POWER_ON;
	else if (mode == SDE_MODE_DPMS_OFF)
		c_conn->vrr_cmd_state = VRR_CMD_POWER_OFF;
	else if ((c_conn->last_panel_power_mode == SDE_MODE_DPMS_OFF) &&
		(mode == SDE_MODE_DPMS_LP1))
		c_conn->vrr_cmd_state = VRR_CMD_POWER_ON;

#if !IS_ENABLED(CONFIG_DISPLAY_SAMSUNG) /* QC Original */
	if (mode != c_conn->last_panel_power_mode && c_conn->ops.set_power) {
#else /* SS Modify */
	if (mode != c_conn->last_panel_power_mode && c_conn->ops.set_power
		&& !(mode == SDE_MODE_DPMS_OFF && c_conn->last_panel_power_mode == SDE_MODE_DPMS_ON)
		&& !(mode == SDE_MODE_DPMS_ON && c_conn->last_panel_power_mode == SDE_MODE_DPMS_OFF)) {
#endif
		display = c_conn->display;
		set_power = c_conn->ops.set_power;

		mutex_unlock(&c_conn->lock);
		rc = set_power(connector, mode, display);
		mutex_lock(&c_conn->lock);
	}
	c_conn->last_panel_power_mode = mode;

	mutex_unlock(&c_conn->lock);
	if (mode != SDE_MODE_DPMS_ON)
		sde_connector_schedule_status_work(connector, false);
	else
		sde_connector_schedule_status_work(connector, true);
	mutex_lock(&c_conn->lock);

	return rc;
}

static int _sde_connector_update_dimming_bl_lut(struct sde_connector *c_conn,
		struct sde_connector_state *c_state)
{
	bool is_dirty;
	size_t sz = 0;
	struct dsi_display *dsi_display;
	struct dsi_backlight_config *bl_config;
	int rc = 0;

	if (!c_conn || !c_state) {
		SDE_ERROR("invalid arguments\n");
		return -EINVAL;
	}

	dsi_display = _sde_connector_get_display(c_conn);
	if (!dsi_display)
		return 0;

	if (!dsi_display || !dsi_display->panel) {
		SDE_ERROR("Invalid params(s) dsi_display %pK, panel %pK\n",
			dsi_display,
			((dsi_display) ? dsi_display->panel : NULL));
		return -EINVAL;
	}

	is_dirty = msm_property_is_dirty(&c_conn->property_info,
			&c_state->property_state,
			CONNECTOR_PROP_DIMMING_BL_LUT);
	if (!is_dirty)
		return -ENODATA;

	bl_config = &dsi_display->panel->bl_config;
	bl_config->dimming_bl_lut = msm_property_get_blob(&c_conn->property_info,
			&c_state->property_state, &sz, CONNECTOR_PROP_DIMMING_BL_LUT);
	rc = c_conn->ops.set_backlight(&c_conn->base,
			dsi_display, bl_config->bl_level);
	if (!rc)
		c_conn->unset_bl_level = 0;

	return 0;
}

static int _sde_connector_update_dimming_ctrl(struct sde_connector *c_conn,
		struct sde_connector_state *c_state, uint64_t val)
{
	struct dsi_display *dsi_display;
	struct dsi_backlight_config *bl_config;
	bool prev, curr = (bool)val;

	if (!c_conn || !c_state) {
		SDE_ERROR("invalid arguments\n");
		return -EINVAL;
	}

	dsi_display = _sde_connector_get_display(c_conn);
	if (!dsi_display)
		return 0;

	if (!dsi_display || !dsi_display->panel) {
		SDE_ERROR("Invalid params(s) dsi_display %pK, panel %pK\n",
			dsi_display,
			((dsi_display) ? dsi_display->panel : NULL));
		return -EINVAL;
	}

	bl_config = &dsi_display->panel->bl_config;
	prev = (bool)(bl_config->dimming_status & DIMMING_ENABLE);
	if (curr == prev)
		return 0;

	if(val) {
		bl_config->dimming_status |= DIMMING_ENABLE;
	} else {
		bl_config->dimming_status &= ~DIMMING_ENABLE;
	}
	bl_config->user_disable_notification = false;
	sde_dimming_bl_notify(c_conn, bl_config);
	if (!val)
		bl_config->user_disable_notification = true;

	return 0;
}

static int _sde_connector_update_dimming_min_bl(struct sde_connector *c_conn,
		struct sde_connector_state *c_state, uint64_t val)
{
	struct dsi_display *dsi_display;
	struct dsi_backlight_config *bl_config;
	uint32_t tmp = 0;

	if (!c_conn || !c_state) {
		SDE_ERROR("invalid arguments\n");
		return -EINVAL;
	}

	dsi_display = _sde_connector_get_display(c_conn);
	if (!dsi_display)
		return 0;

	if (!dsi_display || !dsi_display->panel) {
		SDE_ERROR("Invalid params(s) dsi_display %pK, panel %pK\n",
			dsi_display,
			((dsi_display) ? dsi_display->panel : NULL));
		return -EINVAL;
	}

	bl_config = &dsi_display->panel->bl_config;
	tmp = (uint32_t)val;
	if (tmp == bl_config->dimming_min_bl)
		return 0;
	bl_config->dimming_min_bl = tmp;
	bl_config->dimming_status |= DIMMING_MIN_BL_VALID;
	sde_dimming_bl_notify(c_conn, bl_config);
	bl_config->dimming_status &= ~DIMMING_MIN_BL_VALID;

	return 0;
}

#if !IS_ENABLED(CONFIG_DISPLAY_SAMSUNG) /* to AVOID unexpectable brightness control */
static int _sde_connector_update_bl_scale(struct sde_connector *c_conn)
{
	struct dsi_display *dsi_display;
	struct dsi_backlight_config *bl_config;
	int rc = 0;

	if (!c_conn) {
		SDE_ERROR("Invalid params sde_connector null\n");
		return -EINVAL;
	}

	dsi_display = _sde_connector_get_display(c_conn);
	if (!dsi_display)
		return 0;

	if (!dsi_display || !dsi_display->panel) {
		SDE_ERROR("Invalid params(s) dsi_display %pK, panel %pK\n",
			dsi_display,
			((dsi_display) ? dsi_display->panel : NULL));
		return -EINVAL;
	}

	bl_config = &dsi_display->panel->bl_config;
	bl_config->bl_scale = c_conn->bl_scale > MAX_BL_SCALE_LEVEL ?
			MAX_BL_SCALE_LEVEL : c_conn->bl_scale;
	bl_config->bl_scale_sv = c_conn->bl_scale_sv > SV_BL_SCALE_CAP ?
			SV_BL_SCALE_CAP : c_conn->bl_scale_sv;

	if (!c_conn->allow_bl_update) {
		c_conn->unset_bl_level = bl_config->bl_level;
		return 0;
	}

	if (c_conn->unset_bl_level)
		bl_config->bl_level = c_conn->unset_bl_level;

	SDE_DEBUG("bl_scale = %u, bl_scale_sv = %u, bl_level = %u\n",
		bl_config->bl_scale, bl_config->bl_scale_sv,
		bl_config->bl_level);
	rc = c_conn->ops.set_backlight(&c_conn->base,
			dsi_display, bl_config->bl_level);

	if (!rc)
		sde_dimming_bl_notify(c_conn, bl_config);
	c_conn->unset_bl_level = 0;

	return rc;
}
#endif

void sde_connector_set_colorspace(struct sde_connector *c_conn)
{
	int rc = 0;

	if (c_conn->ops.set_colorspace)
		rc = c_conn->ops.set_colorspace(&c_conn->base,
			c_conn->display);

	if (rc)
		SDE_ERROR_CONN(c_conn, "cannot apply new colorspace %d\n", rc);

}

void sde_connector_set_vrr_params(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	struct drm_encoder *drm_enc;
	u32 frame_interval = 0, usecase_idx = 0;
	u64 frame_interval_ns = 0;
	bool frame_interval_updated = false, usecase_idx_updated = false;
	struct msm_freq_step_pattern *new_freq_pattern, *old_freq_pattern;

	if (!connector)
		return;

	c_conn = to_sde_connector(connector);
	if (!c_conn->vrr_caps.vrr_support)
		return;

	c_state = to_sde_connector_state(connector->state);
	if (connector->state && connector->state->best_encoder)
		drm_enc = connector->state->best_encoder;
	else
		drm_enc = connector->encoder;

	frame_interval_ns = sde_connector_get_property(c_conn->base.state,
			CONNECTOR_PROP_FRAME_INTERVAL);

	if (!c_conn->apply_vrr && frame_interval_ns) {
		c_conn->apply_vrr = true;
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE1, c_conn->apply_vrr);
	}

	if (!frame_interval_ns) {
		SDE_DEBUG("VRR not supported\n");
		return;
	}

	frame_interval = (NSEC_PER_SEC/frame_interval_ns)*1000;
	SDE_EVT32(frame_interval);
	if (frame_interval != c_conn->frame_interval) {
		frame_interval_updated = true;
		c_conn->frame_interval = frame_interval;
		SDE_DEBUG("frame_interval%d\n", c_conn->frame_interval);
	}

	usecase_idx = sde_connector_get_property(c_conn->base.state,
			CONNECTOR_PROP_USECASE_IDX);
	if (usecase_idx != c_conn->usecase_idx) {
		usecase_idx_updated = true;
		c_conn->usecase_idx = usecase_idx;
	}

	c_conn->freq_pattern_updated = false;
	c_conn->freq_pattern_type_changed = false;
	if (usecase_idx_updated || frame_interval_updated || !c_conn->freq_pattern) {
		new_freq_pattern = sde_encoder_get_freq_pattern(drm_enc, c_conn->frame_interval,
				c_conn->usecase_idx);
		if (!new_freq_pattern) {
			SDE_EVT32(SDE_EVTLOG_ERROR, frame_interval_ns, frame_interval_ns>>32);
			return;
		}
		old_freq_pattern = c_conn->freq_pattern;
		if (!old_freq_pattern) {
			c_conn->freq_pattern = new_freq_pattern;
			c_conn->freq_pattern_updated = true;
			c_conn->freq_pattern_type_changed = true;
		} else if (new_freq_pattern->frame_interval != old_freq_pattern->frame_interval ||
			new_freq_pattern->usecase_idx != old_freq_pattern->usecase_idx) {
			c_conn->freq_pattern = new_freq_pattern;
			c_conn->freq_pattern_updated = true;

			if (new_freq_pattern->needs_ap_refresh !=
					old_freq_pattern->needs_ap_refresh)
				c_conn->freq_pattern_type_changed = true;

			SDE_EVT32(new_freq_pattern->frame_interval,
				old_freq_pattern->frame_interval, new_freq_pattern->usecase_idx,
				old_freq_pattern->usecase_idx);
		}

		if (new_freq_pattern && old_freq_pattern &&
			(new_freq_pattern->freq_stepping_seq[0] != old_freq_pattern->freq_stepping_seq[0])) {
			c_conn->qsync_updated = true;
			SDE_EVT32(new_freq_pattern->freq_stepping_seq[0], old_freq_pattern->freq_stepping_seq[0]);
		}
	}

	SDE_EVT32(connector->base.id, frame_interval_ns,
		frame_interval_ns>>32, usecase_idx_updated,
		frame_interval_updated, c_conn->freq_pattern_updated,
		c_conn->freq_pattern_type_changed,  c_conn->apply_vrr);
	SDE_DEBUG("usecase_update:%d FI_updated:%d usecase:%d FI:%d pattern_updated %d %d\n",
			usecase_idx_updated, frame_interval_updated, usecase_idx,
			frame_interval, c_conn->freq_pattern_updated,
			c_conn->freq_pattern_type_changed);
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	{
		struct samsung_display_driver_data *vdd;
		struct dsi_display *display = _sde_connector_get_display(c_conn);;

		if (display && display->panel) {
			vdd = display->panel->panel_private;
			ss_panel_dvrr_switch(vdd, c_conn->frame_interval);
		}
	}
#endif
}

void sde_connector_set_qsync_params(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	u32 qsync_propval = 0, ept_fps = 0;
	bool prop_dirty;

	if (!connector)
		return;

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(connector->state);
	c_conn->qsync_updated = false;

	prop_dirty = msm_property_is_dirty(&c_conn->property_info,
					&c_state->property_state,
					CONNECTOR_PROP_QSYNC_MODE);
	if (prop_dirty) {
		qsync_propval = sde_connector_get_property(c_conn->base.state,
						CONNECTOR_PROP_QSYNC_MODE);
		if (qsync_propval != c_conn->qsync_mode) {
			SDE_DEBUG("updated qsync mode %d -> %d\n",
					c_conn->qsync_mode, qsync_propval);
			c_conn->qsync_updated = true;
			c_conn->qsync_mode = qsync_propval;
		}
	}

	prop_dirty = msm_property_is_dirty(&c_conn->property_info, &c_state->property_state,
				CONNECTOR_PROP_AVR_STEP_STATE);
	if (prop_dirty)
		c_conn->qsync_updated = true;

	prop_dirty = msm_property_is_dirty(&c_conn->property_info, &c_state->property_state,
				CONNECTOR_PROP_EPT_FPS);
	if (prop_dirty) {
		ept_fps = sde_connector_get_property(c_conn->base.state, CONNECTOR_PROP_EPT_FPS);
		if (ept_fps != c_conn->ept_fps) {
			c_conn->qsync_updated = true;
			c_conn->ept_fps = ept_fps;
		}
	}

	if ((c_conn->vrr_caps.arp_support || c_conn->vrr_caps.video_psr_support) &&
			(c_conn->qsync_mode != SDE_RM_QSYNC_CONTINUOUS_MODE)) {
		c_conn->qsync_mode = SDE_RM_QSYNC_CONTINUOUS_MODE;
		c_conn->qsync_updated = true;
	}

}

void sde_connector_complete_qsync_commit(struct drm_connector *conn,
				struct msm_display_conn_params *params)
{
	struct sde_connector *c_conn;

	if (!conn || !params) {
		SDE_ERROR("invalid params\n");
		return;
	}

	c_conn = to_sde_connector(conn);

	if (c_conn && c_conn->qsync_updated &&
		(c_conn->qsync_mode == SDE_RM_QSYNC_ONE_SHOT_MODE)) {
		/* Reset qsync states if mode is one shot */
		params->qsync_mode = c_conn->qsync_mode = 0;
		params->qsync_update = true;
		SDE_EVT32(conn->base.id, c_conn->qsync_mode);
	}
}

static int _sde_connector_update_hdr_metadata(struct sde_connector *c_conn,
		struct sde_connector_state *c_state)
{
	int rc = 0;

	if (c_conn->ops.config_hdr)
		rc = c_conn->ops.config_hdr(&c_conn->base, c_conn->display,
				c_state);

	if (rc)
		SDE_ERROR_CONN(c_conn, "cannot apply hdr metadata %d\n", rc);

	SDE_DEBUG_CONN(c_conn, "updated hdr metadata: %d\n", rc);
	return rc;
}

static int _sde_connector_update_dirty_properties(
				struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	int idx;
	u32 b_lvl;

	if (!connector) {
		SDE_ERROR("invalid argument\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(connector->state);

	mutex_lock(&c_conn->property_info.property_lock);
	while ((idx = msm_property_pop_dirty(&c_conn->property_info,
					&c_state->property_state)) >= 0) {
		switch (idx) {
		case CONNECTOR_PROP_LP:
			mutex_lock(&c_conn->lock);
			c_conn->lp_mode = sde_connector_get_property(
					connector->state, CONNECTOR_PROP_LP);
			_sde_connector_update_power_locked(c_conn);
			mutex_unlock(&c_conn->lock);
			break;
		case CONNECTOR_PROP_HDR_METADATA:
			_sde_connector_update_hdr_metadata(c_conn, c_state);
			break;
		case CONNECTOR_PROP_BRIGHTNESS:
			SDE_ATRACE_INT("dimming", c_conn->bl_vrr.bl_increment_in_progress);
			SDE_ATRACE_BEGIN("backlight_device_set_brightness");
			b_lvl = sde_connector_get_property(connector->state,
						CONNECTOR_PROP_BRIGHTNESS);
			backlight_device_set_brightness(c_conn->bl_device, b_lvl);
			SDE_ATRACE_END("backlight_device_set_brightness");
			break;
		default:
			/* nothing to do for most properties */
			break;
		}
	}
	mutex_unlock(&c_conn->property_info.property_lock);

	/* if colorspace needs to be updated do it first */
	if (c_conn->colorspace_updated) {
		c_conn->colorspace_updated = false;
		sde_connector_set_colorspace(c_conn);
	}

#if !IS_ENABLED(CONFIG_DISPLAY_SAMSUNG) /* to AVOID unexpectable brightness control */
	/*
	 * Special handling for postproc properties and
	 * for updating backlight if any unset backlight level is present
	 */
	if (c_conn->bl_scale_dirty || c_conn->unset_bl_level) {
		_sde_connector_update_bl_scale(c_conn);
		c_conn->bl_scale_dirty = false;
	}
#endif

	return 0;
}

struct sde_connector_dyn_hdr_metadata *sde_connector_get_dyn_hdr_meta(
		struct drm_connector *connector)
{
	struct sde_connector_state *c_state;

	if (!connector)
		return NULL;

	c_state = to_sde_connector_state(connector->state);
	return &c_state->dyn_hdr_meta;
}

static bool sde_connector_power_on_off_frame(struct drm_connector *connector)
{
	int lp_mode;

	lp_mode = sde_connector_get_property(connector->state, CONNECTOR_PROP_LP);
	if (lp_mode == SDE_MODE_DPMS_OFF)
		return true;

	if (connector->state && connector->state->crtc &&
			sde_crtc_is_power_on_frame(connector->state->crtc))
		return true;

	return false;
}

int sde_connector_check_update_vhm_cmd(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct msm_freq_step_pattern *freq_pattern;
	struct sde_encoder_virt *sde_enc;
	u64 cmd_bit_mask = 0;
	int rc = 0;

	if (!connector) {
		SDE_ERROR("invalid argument, conn %d\n", connector != NULL);
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	sde_enc = to_sde_encoder_virt(c_conn->encoder);

	if (sde_enc)
		sde_enc->vrr_info.vhm_cmd_in_progress = SDE_NO_CMD_SCHEDULED;

	if (sde_encoder_in_cont_splash(connector->encoder))
		return 0;

	if (!c_conn->freq_pattern) {
		SDE_ERROR("frequency pattern is NULL but update is true\n");
		return -EINVAL;
	}

	SDE_EVT32(c_conn->vrr_cmd_state, c_conn->freq_pattern_updated,
		SDE_EVTLOG_FUNC_CASE1);
	freq_pattern = c_conn->freq_pattern;

	if (c_conn->vrr_cmd_state == VRR_CMD_POWER_ON ||
			c_conn->vrr_cmd_state == VRR_CMD_IDLE_EXIT) {
		if (!sde_connector_power_on_off_frame(connector)) {
			c_conn->freq_pattern_updated = true;
			c_conn->freq_pattern_type_changed = true;
			c_conn->vrr_cmd_state = VRR_CMD_STATE_NONE;
		}
	}

	if  (c_conn->freq_pattern_updated)
		cmd_bit_mask |= BIT(DSI_CMD_SET_FI_PATTAREN1_CHANGE +
			freq_pattern->frame_pattern_seq_idx);

	if (c_conn->freq_pattern_type_changed && freq_pattern->needs_ap_refresh)
		cmd_bit_mask |= BIT(DSI_CMD_SET_STICKY_STILL_DISABLE);
	else if (c_conn->freq_pattern_type_changed && !freq_pattern->needs_ap_refresh)
		cmd_bit_mask |= BIT(DSI_CMD_SET_STICKY_STILL_EN);

	if (cmd_bit_mask) {
		SDE_ATRACE_BEGIN("update_vhm_cmd");
		mutex_lock(&c_conn->bl_vrr.bl_lock);
		rc = sde_connector_update_cmd(connector, cmd_bit_mask, true);
		if (sde_enc)
			sde_enc->vrr_info.vhm_cmd_in_progress = SDE_CMD_SCHEDULED;
		mutex_unlock(&c_conn->bl_vrr.bl_lock);
		SDE_ATRACE_END("update_vhm_cmd");
	}

	SDE_EVT32(SDE_EVTLOG_FUNC_CASE2, rc, cmd_bit_mask>>32, cmd_bit_mask,
		freq_pattern->frame_pattern_seq_idx, freq_pattern->frame_interval,
		c_conn->freq_pattern_type_changed, freq_pattern->needs_ap_refresh,
		c_conn->vrr_cmd_state, c_conn->freq_pattern_updated);

	c_conn->freq_pattern_updated = false;
	c_conn->freq_pattern_type_changed = false;


	return rc;
}

int sde_connector_pre_kickoff(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	struct msm_display_kickoff_params params;
	struct dsi_display *display;
	int rc;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct samsung_display_driver_data *vdd;
	u32 finger_mask_state;
#endif

	if (!connector) {
		SDE_ERROR("invalid argument\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(connector->state);
	if (!c_conn->display) {
		SDE_ERROR("invalid connector display\n");
		return -EINVAL;
	}

	display = _sde_connector_get_display(c_conn);
	if (!display)
		return 0;
	/*
	 * During pre kickoff DCS commands have to have an
	 * asynchronous wait to avoid an unnecessary stall
	 * in pre-kickoff. This flag must be reset at the
	 * end of display pre-kickoff.
	 */
	SDE_ATRACE_BEGIN("connector_pre_kickoff");
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	c_conn->pre_kickoff_progress = true;
#endif
	if (c_conn->connector_type == DRM_MODE_CONNECTOR_DSI)
		display->queue_cmd_waits = true;

	rc = _sde_connector_update_dirty_properties(connector);
	if (rc) {
		SDE_EVT32(connector->base.id, SDE_EVTLOG_ERROR);
		goto end;
	}

	/* Send VHM commands post BRIGHTNESS updates */
	if (c_conn->vrr_caps.video_psr_support) {
		rc = sde_connector_check_update_vhm_cmd(connector);
		if (rc)
			SDE_EVT32(connector->base.id, SDE_EVTLOG_ERROR);
	}

	if (!c_conn->ops.pre_kickoff)
		return 0;

	params.rois = &c_state->rois;
	params.hdr_meta = &c_state->hdr_meta;

	SDE_EVT32_VERBOSE(connector->base.id);

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	if (c_conn->connector_type == DRM_MODE_CONNECTOR_DSI) {
		/* SAMSUNG_FINGERPRINT */
		vdd = display->panel->panel_private;
		if (vdd->support_optical_fingerprint) {
			finger_mask_state = sde_connector_get_property(c_conn->base.state,
					CONNECTOR_PROP_FINGERPRINT_MASK);
			/* To prevent race condition between Finger Mask Commit & VRR Commit
			 * P230622-05552 : Race condition beteen Finger Mask & VRR(HS - PHS Switching)
			 * Incase of VRR switching, VRR shulod be changed even though finger mask is being changed.
			 * To protect link between vdd->finger_mask_updated & finger_mask brightness update, vrr_lock should be added.
			 *
			 * Simply, please refer below NG/OK cases.
			 * vdd->finger_mask_updated=true -> vrr brightness_update -> finger_mask brightness_update (NG)
			 * vdd->finger_mask_updated=true -> finger_mask brightness_update -> vdd->finger_mask_updated=false -> vrr brightness_update (OK)
			 */

			if (finger_mask_state != vdd->finger_mask) {
				mutex_lock(&vdd->vrr.vrr_lock);
				SDE_INFO("[FINGER_MASK]updated finger mask mode %d\n", finger_mask_state);
				vdd->finger_mask_updated = true;
				vdd->finger_mask = finger_mask_state;
				ss_send_hbm_fingermask_image_tx(vdd, vdd->finger_mask);
				vdd->finger_mask_updated = false;
				mutex_unlock(&vdd->vrr.vrr_lock);
			}
		}
	}
#endif

	rc = c_conn->ops.pre_kickoff(connector, c_conn->display, &params);

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	c_conn->pre_kickoff_progress = false;
#endif
	if (c_conn->connector_type == DRM_MODE_CONNECTOR_DSI)
		display->queue_cmd_waits = false;
end:
	SDE_ATRACE_END("connector_pre_kickoff");
	return rc;
}

int sde_connector_prepare_commit(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	struct msm_display_conn_params params;
	int rc;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct sde_encoder_virt *sde_enc;
#endif

	if (!connector) {
		SDE_ERROR("invalid argument\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(connector->state);
	if (!c_conn->display) {
		SDE_ERROR("invalid connector display\n");
		return -EINVAL;
	}

	if (!c_conn->ops.prepare_commit)
		return 0;

	/* In LPM case (poweroff charging), vrr is not applied. c_conn->frame_interval is 0 */
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	sde_enc = to_sde_encoder_virt(c_conn->encoder);
	if (!sde_enc) {
		SDE_ERROR("invalid encoder, sde_enc %d\n", sde_enc != NULL);
		return -EINVAL;
	}

	if (sde_enc->disp_info.vrr_caps.vrr_support && !c_conn->apply_vrr) {
		SDE_ERROR("vrr_support (%d) apply_vrr (%d), Do not set qsync mode\n",
			sde_enc->disp_info.vrr_caps.vrr_support, c_conn->apply_vrr);
		c_conn->qsync_updated = false;
	}
#endif

	memset(&params, 0, sizeof(params));

	if (c_conn->qsync_updated) {
		params.qsync_mode = c_conn->qsync_mode;
		params.qsync_update = true;
	}

	rc = c_conn->ops.prepare_commit(c_conn->display, &params);

	SDE_EVT32(connector->base.id, params.qsync_mode,
		  params.qsync_update, rc);

	return rc;
}

int sde_connector_trigger_cmd_self_refresh(struct drm_connector *connector)
{
	int rc = 0;
	struct sde_connector *c_conn;
	struct sde_encoder_virt *sde_enc;

	if (!connector) {
		SDE_ERROR("invalid argument, conn %d\n", connector != NULL);
		return -EINVAL;
	}
	c_conn = to_sde_connector(connector);
	sde_enc = to_sde_encoder_virt(c_conn->encoder);

	if (!sde_enc) {
		SDE_ERROR("invalid encoder, sde_enc %d\n", sde_enc != NULL);
		return -EINVAL;
	}

	SDE_EVT32(connector->base.id);
	SDE_ATRACE_BEGIN("cmd_self_refresh");

	if (c_conn->vrr_caps.video_psr_support &&
			!(c_conn->ops.check_cmd_defined(c_conn->display,
			DSI_CMD_SET_TRIGGER_SELF_REFRESH))) {
		if (sde_enc->rc_state != SDE_ENC_RC_STATE_IDLE)
			sde_encoder_handle_video_psr_self_refresh(sde_enc, false);
	} else {
		rc = sde_connector_update_cmd(connector,
			BIT(DSI_CMD_SET_TRIGGER_SELF_REFRESH), true);
	}
	SDE_ATRACE_END("cmd_self_refresh");

	return rc;
}

int sde_connector_trigger_cmd_backlight_update(struct drm_connector *connector)
{
	struct sde_connector *c_conn = NULL;
	struct sde_encoder_virt *sde_enc;
	int rc = 0;

	SDE_EVT32(SDE_EVTLOG_FUNC_ENTRY);
	if (!connector) {
		SDE_ERROR("invalid argument, conn %d\n", connector != NULL);
		return -EINVAL;
	}
	c_conn = to_sde_connector(connector);

	if (c_conn->bl_vrr.new_incremental_bl_update) {
		SDE_EVT32(DRMID(&c_conn->base), SDE_EVTLOG_FUNC_CASE1);
		return 0;
	}

	sde_enc = to_sde_encoder_virt(c_conn->encoder);
	if (c_conn->encoder && sde_enc->off_work_in_progress) {
		SDE_EVT32(DRMID(&c_conn->base), SDE_EVTLOG_ERROR);
		sde_encoder_handle_next_backlight_update(c_conn->encoder);
		return 0;
	}

	mutex_lock(&c_conn->bl_vrr.bl_lock);
	/* apply the incremental backlight */
	rc = sde_connector_apply_incremental_bl(c_conn);
	if (rc) {
		SDE_ERROR("Incremental backlight apply failed\n");
		mutex_unlock(&c_conn->bl_vrr.bl_lock);
		return rc;
	}

	SDE_EVT32(SDE_EVTLOG_FUNC_EXIT, DRMID(&c_conn->base));
	mutex_unlock(&c_conn->bl_vrr.bl_lock);

	return rc;
}

int sde_connector_trigger_cmd_backlight_sr(struct drm_connector *connector)
{
	struct sde_connector *c_conn = NULL;
	int rc = 0;

	SDE_EVT32(SDE_EVTLOG_FUNC_ENTRY);
	if (!connector) {
		SDE_ERROR("invalid argument, conn %d\n", connector != NULL);
		return -EINVAL;
	}
	c_conn = to_sde_connector(connector);

	/* trigger self refresh if no frame scheduled */
	if (connector && connector->state && connector->state->crtc &&
			sde_crtc_no_frame_in_progress(connector->state->crtc)) {
		rc = sde_connector_trigger_cmd_self_refresh(connector);
		SDE_EVT32(SDE_EVTLOG_FUNC_CASE1, rc);
	}

	SDE_EVT32(SDE_EVTLOG_FUNC_EXIT, DRMID(&c_conn->base));

	return rc;
}

int sde_connector_update_cmd(struct drm_connector *connector,
	u64 cmd_bit_mask, bool peripheral_flush)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	struct msm_display_conn_params params;
	struct drm_encoder *drm_enc;
	int rc;

	if (!connector) {
		SDE_ERROR("invalid argument\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(connector->state);
	drm_enc = c_conn->encoder;

	if (!c_conn->display) {
		SDE_ERROR("invalid connector display\n");
		return -EINVAL;
	}

	if (!c_conn->ops.prepare_commit)
		return 0;

	memset(&params, 0, sizeof(params));

	if (peripheral_flush)
		sde_encoder_update_periph_flush(drm_enc);

	params.cmd_bit_mask = cmd_bit_mask;
	params.peripheral_flush = peripheral_flush;

	rc = c_conn->ops.prepare_commit(c_conn->display, &params);

	c_conn->last_vhm_cmd = cmd_bit_mask;
	SDE_EVT32(connector->base.id, params.cmd_bit_mask >> 32,
		params.cmd_bit_mask, params.peripheral_flush, rc);

	return rc;
}

void sde_connector_set_private_flag(struct sde_connector_state *conn_state, u32 flag, bool set)
{
	if (set)
		conn_state->msm_mode.private_flags |= flag;
	else
		conn_state->msm_mode.private_flags &= ~flag;
}

void sde_connector_helper_bridge_disable(struct drm_connector *connector)
{
	int rc;
	struct sde_connector *c_conn = NULL;
	struct dsi_display *display;
	bool poms_pending = false;
	struct sde_kms *sde_kms;

	sde_kms = sde_connector_get_kms(connector);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return;
	}

	c_conn = to_sde_connector(connector);
	if (c_conn->connector_type == DRM_MODE_CONNECTOR_DSI) {
		display = _sde_connector_get_display(c_conn);
		if (!display)
			return;
		poms_pending = display->poms_pending;
	}

	if (!poms_pending) {
		rc = _sde_connector_update_dirty_properties(connector);
		if (rc) {
			SDE_ERROR("conn %d final pre kickoff failed %d\n",
					connector->base.id, rc);
			SDE_EVT32(connector->base.id, SDE_EVTLOG_ERROR);
		}
	}
	/* Disable ESD thread */
	sde_connector_schedule_status_work(connector, false);

	if (!sde_in_trusted_vm(sde_kms) && c_conn->bl_device && !poms_pending) {
		c_conn->bl_device->props.power = FB_BLANK_POWERDOWN;
		c_conn->bl_device->props.state |= BL_CORE_FBBLANK;
		backlight_update_status(c_conn->bl_device);
	}

	c_conn->allow_bl_update = false;
	if (c_conn->vrr_caps.video_psr_support || c_conn->vrr_caps.arp_support)
		c_conn->qsync_mode = SDE_RM_QSYNC_DISABLED;
}

void sde_connector_helper_bridge_post_disable(struct drm_connector *connector)
{
	struct sde_connector *c_conn = NULL;
	c_conn = to_sde_connector(connector);

	c_conn->panel_dead = false;
}

void sde_connector_helper_bridge_enable(struct drm_connector *connector)
{
	struct sde_connector *c_conn = NULL;
	struct dsi_display *display;
	struct sde_kms *sde_kms;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct samsung_display_driver_data *vdd;
#endif

	sde_kms = sde_connector_get_kms(connector);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return;
	}

	c_conn = to_sde_connector(connector);
	display = _sde_connector_get_display(c_conn);
	if (!display)
		return;
	/*
	 * Special handling for some panels which need atleast
	 * one frame to be transferred to GRAM before enabling backlight.
	 * So delay backlight update to these panels until the
	 * first frame commit is received from the HW.
	 */
	if (display->panel->bl_config.bl_update ==
				BL_UPDATE_DELAY_UNTIL_FIRST_FRAME)
		sde_encoder_wait_for_event(c_conn->encoder,
				MSM_ENC_TX_COMPLETE);
	c_conn->allow_bl_update = true;

	if (!sde_in_trusted_vm(sde_kms) && c_conn->bl_device && !display->poms_pending) {
		c_conn->bl_device->props.power = FB_BLANK_UNBLANK;
		c_conn->bl_device->props.state &= ~BL_CORE_FBBLANK;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
		vdd = display->panel->panel_private;

		if (vdd->vrr.support_vrr_based_bl &&
				(vdd->vrr.running_vrr_mdp || vdd->vrr.running_vrr)) {
			LCD_DEBUG(vdd, "skip & backup props.brightness = %d during VRR\n",
					c_conn->bl_device->props.brightness);
			vdd->br_info.common_br.bl_level = c_conn->bl_device->props.brightness;
		} else {
			backlight_update_status(c_conn->bl_device);
		}
#else
		backlight_update_status(c_conn->bl_device);
#endif
	}
}

int sde_connector_clk_ctrl(struct drm_connector *connector, bool enable, bool idle_pc)
{
	struct sde_connector *c_conn;
	struct dsi_display *display;
	u32 state = enable ? DSI_CLK_ON : DSI_CLK_OFF;
	int rc = 0;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	display = _sde_connector_get_display(c_conn);
	if (!display) {
		SDE_ERROR("null display\n");
		return -EINVAL;
	}

	if (c_conn->ops.clk_ctrl)
		rc = c_conn->ops.clk_ctrl(display, DSI_CORE_CLK | DSI_LINK_CLK, state);
	if (c_conn->ops.idle_pc_ctrl)
		c_conn->ops.idle_pc_ctrl(display, idle_pc);

	return rc;
}

int sde_connector_esync_clk_ctrl(struct drm_connector *connector, bool enable)
{
	struct sde_connector *c_conn;
	struct dsi_display *display;
	u32 state = enable ? DSI_CLK_ON : DSI_CLK_OFF;
	int rc = 0;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	display = (struct dsi_display *) c_conn->display;

	if (display && c_conn->ops.clk_ctrl)
		rc = c_conn->ops.clk_ctrl(display, DSI_ESYNC_CLK, state);

	return rc;
}

int sde_connector_osc_clk_ctrl(struct drm_connector *connector, bool enable)
{
	struct sde_connector *c_conn;
	struct dsi_display *display;
	u32 state = enable ? DSI_CLK_ON : DSI_CLK_OFF;
	int rc = 0;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	display = (struct dsi_display *) c_conn->display;

	if (display && c_conn->ops.clk_ctrl)
		rc = c_conn->ops.clk_ctrl(display, DSI_OSC_CLK, state);

	return rc;
}

int sde_connector_clk_get_rate_esync(struct drm_connector *connector,
		enum sde_intf intf_idx, u64 *rate)
{
	struct sde_connector *c_conn;
	struct dsi_display *display;
	u32 dsi_idx = INTF_MAX;
	int rc = 0;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	display = (struct dsi_display *) c_conn->display;

	if (intf_idx == INTF_1) {
		dsi_idx = 0;
	} else if (intf_idx == INTF_2) {
		dsi_idx = 1;
	} else {
		SDE_ERROR("invalid interface index %d", intf_idx-INTF_0);
		return -EINVAL;
	}

	if (display && c_conn->ops.clk_get_rate)
		rc = c_conn->ops.clk_get_rate(display, dsi_idx, DSI_ESYNC_CLK, rate);

	return rc;
}

void sde_connector_destroy(struct drm_connector *connector)
{
	struct sde_connector *c_conn;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return;
	}

	c_conn = to_sde_connector(connector);

	if (c_conn->sysfs_dev)
		device_unregister(c_conn->sysfs_dev);

	/* cancel if any pending esd work */
	sde_connector_schedule_status_work(connector, false);

	if (c_conn->ops.pre_destroy)
		c_conn->ops.pre_destroy(connector, c_conn->display);

	if (c_conn->blob_caps)
		drm_property_blob_put(c_conn->blob_caps);
	if (c_conn->blob_hdr)
		drm_property_blob_put(c_conn->blob_hdr);
	if (c_conn->blob_dither)
		drm_property_blob_put(c_conn->blob_dither);
	if (c_conn->blob_mode_info)
		drm_property_blob_put(c_conn->blob_mode_info);
	if (c_conn->blob_ext_hdr)
		drm_property_blob_put(c_conn->blob_ext_hdr);

	if (c_conn->cdev)
		backlight_cdev_unregister(c_conn->cdev);
	if (c_conn->bl_device)
		backlight_device_unregister(c_conn->bl_device);
	drm_connector_unregister(connector);
	mutex_destroy(&c_conn->lock);
	sde_fence_deinit(c_conn->retire_fence);
	drm_connector_cleanup(connector);
	msm_property_destroy(&c_conn->property_info);
	kfree(c_conn);
}

/**
 * _sde_connector_destroy_fb - clean up connector state's out_fb buffer
 * @c_conn: Pointer to sde connector structure
 * @c_state: Pointer to sde connector state structure
 */
static void _sde_connector_destroy_fb(struct sde_connector *c_conn,
		struct sde_connector_state *c_state)
{
	if (!c_state || !c_state->out_fb) {
		SDE_ERROR("invalid state %pK\n", c_state);
		return;
	}

	drm_framebuffer_put(c_state->out_fb);
	c_state->out_fb = NULL;

	if (c_conn)
		c_state->property_values[CONNECTOR_PROP_OUT_FB].value =
			msm_property_get_default(&c_conn->property_info,
					CONNECTOR_PROP_OUT_FB);
	else
		c_state->property_values[CONNECTOR_PROP_OUT_FB].value = ~0;
}

static void sde_connector_atomic_destroy_state(struct drm_connector *connector,
		struct drm_connector_state *state)
{
	struct sde_connector *c_conn = NULL;
	struct sde_connector_state *c_state = NULL;

	if (!state) {
		SDE_ERROR("invalid state\n");
		return;
	}

	/*
	 * The base DRM framework currently always passes in a NULL
	 * connector pointer. This is not correct, but attempt to
	 * handle that case as much as possible.
	 */
	if (connector)
		c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(state);

	if (c_state->out_fb)
		_sde_connector_destroy_fb(c_conn, c_state);

	__drm_atomic_helper_connector_destroy_state(&c_state->base);

	if (!c_conn) {
		kfree(c_state);
	} else {
		/* destroy value helper */
		msm_property_destroy_state(&c_conn->property_info, c_state,
				&c_state->property_state);
	}
}

static void sde_connector_atomic_reset(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return;
	}

	c_conn = to_sde_connector(connector);

	if (connector->state &&
			!sde_crtc_is_reset_required(connector->state->crtc)) {
		SDE_DEBUG_CONN(c_conn, "avoid reset for connector\n");
		return;
	}

	if (connector->state) {
		sde_connector_atomic_destroy_state(connector, connector->state);
		connector->state = 0;
	}

	c_state = msm_property_alloc_state(&c_conn->property_info);
	if (!c_state) {
		SDE_ERROR("state alloc failed\n");
		return;
	}

	/* reset value helper, zero out state structure and reset properties */
	msm_property_reset_state(&c_conn->property_info, c_state,
			&c_state->property_state,
			c_state->property_values);

	__drm_atomic_helper_connector_reset(connector, &c_state->base);
}

static struct drm_connector_state *
sde_connector_atomic_duplicate_state(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state, *c_oldstate;

	if (!connector || !connector->state) {
		SDE_ERROR("invalid connector %pK\n", connector);
		return NULL;
	}

	c_conn = to_sde_connector(connector);
	c_oldstate = to_sde_connector_state(connector->state);
	if (c_oldstate->cont_splash_populated) {
		connector->state->crtc = NULL;
		c_oldstate->cont_splash_populated = false;
	}

	c_state = msm_property_alloc_state(&c_conn->property_info);
	if (!c_state) {
		SDE_ERROR("state alloc failed\n");
		return NULL;
	}

	/* duplicate value helper */
	msm_property_duplicate_state(&c_conn->property_info,
			c_oldstate, c_state,
			&c_state->property_state, c_state->property_values);

	__drm_atomic_helper_connector_duplicate_state(connector,
			&c_state->base);

	/* additional handling for drm framebuffer objects */
	if (c_state->out_fb)
		drm_framebuffer_get(c_state->out_fb);

	/* clear dynamic HDR metadata from prev state */
	if (c_state->dyn_hdr_meta.dynamic_hdr_update) {
		c_state->dyn_hdr_meta.dynamic_hdr_update = false;
		c_state->dyn_hdr_meta.dynamic_hdr_payload_size = 0;
	}

	return &c_state->base;
}

int sde_connector_roi_v1_check_roi(struct drm_connector_state *conn_state)
{
	const struct msm_roi_alignment *align = NULL;
	struct sde_connector *c_conn = NULL;
	struct msm_mode_info mode_info;
	struct sde_connector_state *c_state;
	int i, w, h;

	if (!conn_state)
		return -EINVAL;

	memset(&mode_info, 0, sizeof(mode_info));

	c_state = to_sde_connector_state(conn_state);
	c_conn = to_sde_connector(conn_state->connector);

	memcpy(&mode_info, &c_state->mode_info, sizeof(c_state->mode_info));

	if (!mode_info.roi_caps.enabled)
		return 0;

	if (c_state->rois.num_rects > mode_info.roi_caps.num_roi) {
		SDE_ERROR_CONN(c_conn, "too many rects specified: %d > %d\n",
				c_state->rois.num_rects,
				mode_info.roi_caps.num_roi);
		return -E2BIG;
	}

	align = &mode_info.roi_caps.align;
	for (i = 0; i < c_state->rois.num_rects; ++i) {
		struct drm_clip_rect *roi_conn;

		roi_conn = &c_state->rois.roi[i];
		w = roi_conn->x2 - roi_conn->x1;
		h = roi_conn->y2 - roi_conn->y1;

		SDE_EVT32_VERBOSE(DRMID(&c_conn->base),
				roi_conn->x1, roi_conn->y1,
				roi_conn->x2, roi_conn->y2);

		if (w <= 0 || h <= 0) {
			SDE_ERROR_CONN(c_conn, "invalid conn roi w %d h %d\n",
					w, h);
			return -EINVAL;
		}

		if (w < align->min_width || w % align->width_pix_align) {
			SDE_ERROR_CONN(c_conn,
					"invalid conn roi width %d min %d align %d\n",
					w, align->min_width,
					align->width_pix_align);
			return -EINVAL;
		}

		if (h < align->min_height || h % align->height_pix_align) {
			SDE_ERROR_CONN(c_conn,
					"invalid conn roi height %d min %d align %d\n",
					h, align->min_height,
					align->height_pix_align);
			return -EINVAL;
		}

		if (roi_conn->x1 % align->xstart_pix_align) {
			SDE_ERROR_CONN(c_conn,
					"invalid conn roi x1 %d align %d\n",
					roi_conn->x1, align->xstart_pix_align);
			return -EINVAL;
		}

		if (roi_conn->y1 % align->ystart_pix_align) {
			SDE_ERROR_CONN(c_conn,
					"invalid conn roi y1 %d align %d\n",
					roi_conn->y1, align->ystart_pix_align);
			return -EINVAL;
		}
	}

	return 0;
}

static int _sde_connector_set_roi_v1(
		struct sde_connector *c_conn,
		struct sde_connector_state *c_state,
		void __user *usr_ptr)
{
	struct sde_drm_roi_v1 roi_v1;
	int i;

	if (!c_conn || !c_state) {
		SDE_ERROR("invalid args\n");
		return -EINVAL;
	}

	memset(&c_state->rois, 0, sizeof(c_state->rois));

	if (!usr_ptr) {
		SDE_DEBUG_CONN(c_conn, "rois cleared\n");
		return 0;
	}

	if (copy_from_user(&roi_v1, usr_ptr, sizeof(roi_v1))) {
		SDE_ERROR_CONN(c_conn, "failed to copy roi_v1 data\n");
		return -EINVAL;
	}

	SDE_DEBUG_CONN(c_conn, "num_rects %d\n", roi_v1.num_rects);

	if (roi_v1.num_rects == 0) {
		SDE_DEBUG_CONN(c_conn, "rois cleared\n");
		return 0;
	}

	if (roi_v1.num_rects > SDE_MAX_ROI_V1) {
		SDE_ERROR_CONN(c_conn, "num roi rects more than supported: %d",
				roi_v1.num_rects);
		return -EINVAL;
	}

	c_state->rois.num_rects = roi_v1.num_rects;
	for (i = 0; i < roi_v1.num_rects; ++i) {
		c_state->rois.roi[i] = roi_v1.roi[i];
		SDE_DEBUG_CONN(c_conn, "roi%d: roi (%d,%d) (%d,%d)\n", i,
				c_state->rois.roi[i].x1,
				c_state->rois.roi[i].y1,
				c_state->rois.roi[i].x2,
				c_state->rois.roi[i].y2);
	}

	return 0;
}

static int _sde_connector_set_ext_hdr_info(
	struct sde_connector *c_conn,
	struct sde_connector_state *c_state,
	void __user *usr_ptr)
{
	int rc = 0;
	struct drm_msm_ext_hdr_metadata *hdr_meta;
	size_t payload_size = 0;
	u8 *payload = NULL;
	int i;

	if (!c_conn || !c_state) {
		SDE_ERROR_CONN(c_conn, "invalid args\n");
		rc = -EINVAL;
		goto end;
	}

	memset(&c_state->hdr_meta, 0, sizeof(c_state->hdr_meta));

	if (!usr_ptr) {
		SDE_DEBUG_CONN(c_conn, "hdr metadata cleared\n");
		goto end;
	}

	if (!c_conn->hdr_supported) {
		SDE_ERROR_CONN(c_conn, "sink doesn't support HDR\n");
		rc = -ENOTSUPP;
		goto end;
	}

	if (copy_from_user(&c_state->hdr_meta,
		(void __user *)usr_ptr,
			sizeof(*hdr_meta))) {
		SDE_ERROR_CONN(c_conn, "failed to copy hdr metadata\n");
		rc = -EFAULT;
		goto end;
	}

	hdr_meta = &c_state->hdr_meta;

	/* dynamic metadata support */
	if (!hdr_meta->hdr_plus_payload_size || !hdr_meta->hdr_plus_payload)
		goto skip_dhdr;

	if (!c_conn->hdr_plus_app_ver) {
		SDE_ERROR_CONN(c_conn, "sink doesn't support dynamic HDR\n");
		rc = -ENOTSUPP;
		goto end;
	}

	payload_size = hdr_meta->hdr_plus_payload_size;
	if (payload_size > sizeof(c_state->dyn_hdr_meta.dynamic_hdr_payload)) {
		SDE_ERROR_CONN(c_conn, "payload size exceeds limit\n");
		rc = -EINVAL;
		goto end;
	}

	payload = c_state->dyn_hdr_meta.dynamic_hdr_payload;
	if (copy_from_user(payload,
			(void __user *)c_state->hdr_meta.hdr_plus_payload,
			payload_size)) {
		SDE_ERROR_CONN(c_conn, "failed to copy dhdr metadata\n");
		rc = -EFAULT;
		goto end;
	}

	/* verify 1st header byte, programmed in DP Infoframe SDP header */
	if (payload_size < 1 || (payload[0] != HDR10_PLUS_VSIF_TYPE_CODE)) {
		SDE_ERROR_CONN(c_conn, "invalid payload detected, size: %zd\n",
				payload_size);
		rc = -EINVAL;
		goto end;
	}

	c_state->dyn_hdr_meta.dynamic_hdr_update = true;

skip_dhdr:
	c_state->dyn_hdr_meta.dynamic_hdr_payload_size = payload_size;

	SDE_DEBUG_CONN(c_conn, "hdr_state %d\n", hdr_meta->hdr_state);
	SDE_DEBUG_CONN(c_conn, "hdr_supported %d\n", hdr_meta->hdr_supported);
	SDE_DEBUG_CONN(c_conn, "eotf %d\n", hdr_meta->eotf);
	SDE_DEBUG_CONN(c_conn, "white_point_x %d\n", hdr_meta->white_point_x);
	SDE_DEBUG_CONN(c_conn, "white_point_y %d\n", hdr_meta->white_point_y);
	SDE_DEBUG_CONN(c_conn, "max_luminance %d\n", hdr_meta->max_luminance);
	SDE_DEBUG_CONN(c_conn, "max_content_light_level %d\n",
				hdr_meta->max_content_light_level);
	SDE_DEBUG_CONN(c_conn, "max_average_light_level %d\n",
				hdr_meta->max_average_light_level);

	for (i = 0; i < HDR_PRIMARIES_COUNT; i++) {
		SDE_DEBUG_CONN(c_conn, "display_primaries_x [%d]\n",
				   hdr_meta->display_primaries_x[i]);
		SDE_DEBUG_CONN(c_conn, "display_primaries_y [%d]\n",
				   hdr_meta->display_primaries_y[i]);
	}
	SDE_DEBUG_CONN(c_conn, "hdr_plus payload%s updated, size %d\n",
			c_state->dyn_hdr_meta.dynamic_hdr_update ? "" : " NOT",
			c_state->dyn_hdr_meta.dynamic_hdr_payload_size);

end:
	return rc;
}

static int _sde_connector_set_prop_out_fb(struct drm_connector *connector,
		struct drm_connector_state *state,
		uint64_t val)
{
	int rc = 0;
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(state);

	/* clear old fb, if present */
	if (c_state->out_fb)
		_sde_connector_destroy_fb(c_conn, c_state);

	/* convert fb val to drm framebuffer and prepare it */
	c_state->out_fb =
		drm_framebuffer_lookup(connector->dev, NULL, val);
	if (!c_state->out_fb && val) {
		SDE_ERROR("failed to look up fb %lld\n", val);
		rc = -EFAULT;
	} else if (!c_state->out_fb && !val) {
		SDE_DEBUG("cleared fb_id\n");
		rc = 0;
	}

	return rc;
}

static struct drm_encoder *
sde_connector_best_encoder(struct drm_connector *connector)
{
	struct sde_connector *c_conn = to_sde_connector(connector);

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return NULL;
	}

	/*
	 * This is true for now, revisit this code when multiple encoders are
	 * supported.
	 */
	return c_conn->encoder;
}

static int _sde_connector_set_prop_retire_fence(struct drm_connector *connector,
		struct drm_connector_state *state,
		uint64_t val)
{
	int rc = 0;
	struct sde_connector *c_conn;
	uint64_t fence_user_fd;
	uint64_t __user prev_user_fd;
	struct sde_hw_ctl *hw_ctl = NULL;

	c_conn = to_sde_connector(connector);

	rc = copy_from_user(&prev_user_fd, (void __user *)val,
			sizeof(uint64_t));
	if (rc) {
		SDE_ERROR("copy from user failed rc:%d\n", rc);
		rc = -EFAULT;
		goto end;
	}

	/*
	 * client is expected to reset the property to -1 before
	 * requesting for the retire fence
	 */
	if (prev_user_fd == -1) {
		uint32_t offset;

		offset = sde_connector_get_property(state,
				CONN_PROP_RETIRE_FENCE_OFFSET);
		/*
		 * update the offset to a timeline for
		 * commit completion
		 */
		offset++;

		/* get hw_ctl for a wb connector not in cwb mode */
		if (c_conn->connector_type == DRM_MODE_CONNECTOR_VIRTUAL) {
			struct drm_encoder *drm_enc = sde_connector_best_encoder(connector);

			if (drm_enc && !sde_encoder_in_clone_mode(drm_enc))
				hw_ctl = sde_encoder_get_hw_ctl(c_conn);
		}

		rc = sde_fence_create(c_conn->retire_fence,
					&fence_user_fd, offset, hw_ctl);
		if (rc) {
			SDE_ERROR("fence create failed rc:%d\n", rc);
			goto end;
		}

		rc = copy_to_user((uint64_t __user *)(uintptr_t)val,
				&fence_user_fd, sizeof(uint64_t));
		if (rc) {
			SDE_ERROR("copy to user failed rc:%d\n", rc);
			/*
			 * fence will be released with timeline
			 * update
			 */
			put_unused_fd(fence_user_fd);
			rc = -EFAULT;
			goto end;
		}
	}
end:
	return rc;
}

static int _sde_connector_set_prop_dyn_transfer_time(struct sde_connector *c_conn, uint64_t val)
{
	int rc = 0;

	if (!c_conn->ops.update_transfer_time)
		return rc;

	rc = c_conn->ops.update_transfer_time(c_conn->display, val);
	if (rc)
		SDE_ERROR_CONN(c_conn, "transfer time update failed, val: %llu, rc %d\n", val, rc);

	return rc;
}

static int sde_connector_atomic_set_property(struct drm_connector *connector,
		struct drm_connector_state *state,
		struct drm_property *property,
		uint64_t val)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	int idx, rc;

	if (!connector || !state || !property) {
		SDE_ERROR("invalid argument(s), conn %pK, state %pK, prp %pK\n",
				connector, state, property);
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(state);

	/* generic property handling */
	rc = msm_property_atomic_set(&c_conn->property_info,
			&c_state->property_state, property, val);
	if (rc)
		goto end;

	/* connector-specific property handling */
	idx = msm_property_index(&c_conn->property_info, property);
	switch (idx) {
	case CONNECTOR_PROP_OUT_FB:
		rc = _sde_connector_set_prop_out_fb(connector, state, val);
		break;
	case CONNECTOR_PROP_RETIRE_FENCE:
		if (!val)
			goto end;

		rc = _sde_connector_set_prop_retire_fence(connector, state, val);
		break;
	case CONNECTOR_PROP_ROI_V1:
		rc = _sde_connector_set_roi_v1(c_conn, c_state,
				(void *)(uintptr_t)val);
		if (rc)
			SDE_ERROR_CONN(c_conn, "invalid roi_v1, rc: %d\n", rc);
		break;
	/* CONNECTOR_PROP_BL_SCALE and CONNECTOR_PROP_SV_BL_SCALE are
	 * color-processing properties. These two properties require
	 * special handling since they don't quite fit the current standard
	 * atomic set property framework.
	 */
	case CONNECTOR_PROP_BL_SCALE:
		c_conn->bl_scale = val;
		c_conn->bl_scale_dirty = true;
		break;
	case CONNECTOR_PROP_SV_BL_SCALE:
		c_conn->bl_scale_sv = val;
		c_conn->bl_scale_dirty = true;
		break;
	case CONNECTOR_PROP_DIMMING_BL_LUT:
		rc = _sde_connector_update_dimming_bl_lut(c_conn, c_state);
		break;
	case CONNECTOR_PROP_DIMMING_CTRL:
		rc = _sde_connector_update_dimming_ctrl(c_conn, c_state, val);
		break;
	case CONNECTOR_PROP_DIMMING_MIN_BL:
		rc = _sde_connector_update_dimming_min_bl(c_conn, c_state, val);
		break;
	case CONNECTOR_PROP_HDR_METADATA:
		rc = _sde_connector_set_ext_hdr_info(c_conn,
			c_state, (void *)(uintptr_t)val);
		if (rc)
			SDE_ERROR_CONN(c_conn, "cannot set hdr info %d\n", rc);
		break;
	case CONNECTOR_PROP_QSYNC_MODE:
	case CONNECTOR_PROP_AVR_STEP_STATE:
	case CONNECTOR_PROP_EPT_FPS:
		msm_property_set_dirty(&c_conn->property_info,
				&c_state->property_state, idx);
		break;
	case CONNECTOR_PROP_SET_PANEL_MODE:
		if (val == DRM_MODE_FLAG_VID_MODE_PANEL)
			c_conn->expected_panel_mode =
				MSM_DISPLAY_VIDEO_MODE;
		else if (val == DRM_MODE_FLAG_CMD_MODE_PANEL)
			c_conn->expected_panel_mode =
				MSM_DISPLAY_CMD_MODE;
		break;
	case CONNECTOR_PROP_DYN_BIT_CLK:
		if (!c_conn->ops.set_dyn_bit_clk)
			break;

		rc = c_conn->ops.set_dyn_bit_clk(connector, val);
		if (rc)
			SDE_ERROR_CONN(c_conn, "dynamic bit clock set failed, rc: %d", rc);

		break;
	case CONNECTOR_PROP_DYN_TRANSFER_TIME:
		_sde_connector_set_prop_dyn_transfer_time(c_conn, val);
		break;
	case CONNECTOR_PROP_LP:
		/* suspend case: clear stale MISR */
		if (val == SDE_MODE_DPMS_OFF) {
			memset(&c_conn->previous_misr_sign, 0, sizeof(struct sde_misr_sign));
			/* reset backlight scale of LTM */
			if (c_conn->bl_scale_sv != MAX_SV_BL_SCALE_LEVEL) {
				c_conn->bl_scale_sv = MAX_SV_BL_SCALE_LEVEL;
				c_conn->bl_scale_dirty = true;
			}
		}
		break;
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	case CONNECTOR_PROP_BRIGHTNESS:
		SDE_INFO("Brightness updated to %llu\n", val);
		break;
#endif
	default:
		break;
	}

	/* check for custom property handling */
	if (!rc && c_conn->ops.set_property) {
		rc = c_conn->ops.set_property(connector,
				state,
				idx,
				val,
				c_conn->display);

		/* potentially clean up out_fb if rc != 0 */
		if ((idx == CONNECTOR_PROP_OUT_FB) && rc)
			_sde_connector_destroy_fb(c_conn, c_state);
	}
end:
	return rc;
}

static int sde_connector_atomic_get_property(struct drm_connector *connector,
		const struct drm_connector_state *state,
		struct drm_property *property,
		uint64_t *val)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	int idx, rc = -EINVAL;

	if (!connector || !state) {
		SDE_ERROR("invalid argument(s), conn %pK, state %pK\n",
				connector, state);
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(state);

	idx = msm_property_index(&c_conn->property_info, property);
	if (idx == CONNECTOR_PROP_RETIRE_FENCE) {
		*val = ~0;
		rc = 0;
	} else {
		/* get cached property value */
		rc = msm_property_atomic_get(&c_conn->property_info,
				&c_state->property_state, property, val);
	}

	/* allow for custom override */
	if (c_conn->ops.get_property)
		rc = c_conn->ops.get_property(connector,
				(struct drm_connector_state *)state,
				idx,
				val,
				c_conn->display);
	return rc;
}

void sde_conn_timeline_status(struct drm_connector *conn)
{
	struct sde_connector *c_conn;

	if (!conn) {
		SDE_ERROR("invalid connector\n");
		return;
	}

	c_conn = to_sde_connector(conn);
	sde_fence_timeline_status(c_conn->retire_fence, &conn->base);
}

void sde_connector_fence_error_ctx_signal(struct drm_connector *conn, int input_fence_status,
	bool is_vid)
{
	struct sde_connector *sde_conn;
	struct sde_fence_context *ctx;
	ktime_t time_stamp;

	sde_conn = to_sde_connector(conn);
	if (!sde_conn)
		return;

	ctx = sde_conn->retire_fence;
	sde_fence_error_ctx_update(ctx, input_fence_status,
		is_vid ? SET_ERROR_ONLY_VID : SET_ERROR_ONLY_CMD_RETIRE);
	time_stamp = ktime_get();

	sde_fence_signal(ctx, time_stamp, SDE_FENCE_SIGNAL, NULL);
	sde_fence_error_ctx_update(ctx, 0, NO_ERROR);
}

void sde_connector_prepare_fence(struct drm_connector *connector)
{
	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return;
	}

	sde_fence_prepare(to_sde_connector(connector)->retire_fence);
}

void sde_connector_complete_commit(struct drm_connector *connector,
		ktime_t ts, enum sde_fence_event fence_event)
{
	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return;
	}

	/* signal connector's retire fence */
	sde_fence_signal(to_sde_connector(connector)->retire_fence,
			ts, fence_event, NULL);
}

void sde_connector_commit_reset(struct drm_connector *connector, ktime_t ts)
{
	struct sde_hw_ctl *hw_ctl = NULL;
	struct sde_connector *c_conn;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return;
	}
	c_conn = to_sde_connector(connector);

	/* get hw_ctl for a wb connector */
	if (c_conn->connector_type == DRM_MODE_CONNECTOR_VIRTUAL)
		hw_ctl = sde_encoder_get_hw_ctl(c_conn);

	/* signal connector's retire fence */
	sde_fence_signal(c_conn->retire_fence, ts, SDE_FENCE_RESET_TIMELINE, hw_ctl);
}

static void sde_connector_update_hdr_props(struct drm_connector *connector)
{
	struct sde_connector *c_conn = to_sde_connector(connector);
	struct drm_msm_ext_hdr_properties hdr = {0};

	hdr.hdr_metadata_type_one = c_conn->hdr_metadata_type_one ? 1 : 0;
	hdr.hdr_supported = c_conn->hdr_supported ? 1 : 0;
	hdr.hdr_eotf = c_conn->hdr_eotf;
	hdr.hdr_max_luminance = c_conn->hdr_max_luminance;
	hdr.hdr_avg_luminance = c_conn->hdr_avg_luminance;
	hdr.hdr_min_luminance = c_conn->hdr_min_luminance;
	hdr.hdr_plus_supported = c_conn->hdr_plus_app_ver;

	msm_property_set_blob(&c_conn->property_info, &c_conn->blob_ext_hdr,
			&hdr, sizeof(hdr), CONNECTOR_PROP_EXT_HDR_INFO);
}

static void sde_connector_update_colorspace(struct drm_connector *connector)
{
	int ret;
	struct sde_connector *c_conn = to_sde_connector(connector);

	ret = msm_property_set_property(
			sde_connector_get_propinfo(connector),
			sde_connector_get_property_state(connector->state),
			CONNECTOR_PROP_SUPPORTED_COLORSPACES,
				c_conn->color_enc_fmt);

	if (ret)
		SDE_ERROR("failed to set colorspace property for connector\n");
}

static int
sde_connector_detect_ctx(struct drm_connector *connector,
		struct drm_modeset_acquire_ctx *ctx,
		bool force)
{
	enum drm_connector_status status = connector_status_unknown;
	struct sde_connector *c_conn;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return status;
	}

	c_conn = to_sde_connector(connector);

	if (c_conn->ops.detect_ctx)
                status = c_conn->ops.detect_ctx(connector, ctx, force, c_conn->display);
        else if (c_conn->ops.detect)
                status = c_conn->ops.detect(connector, force, c_conn->display);

	SDE_DEBUG("connector id: %d, connection status: %d\n", connector->base.id, status);

	return (int)status;
}

int sde_connector_get_dpms(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	int rc;

	if (!connector) {
		SDE_DEBUG("invalid connector\n");
		return DRM_MODE_DPMS_OFF;
	}

	c_conn = to_sde_connector(connector);

	mutex_lock(&c_conn->lock);
	rc = c_conn->dpms_mode;
	mutex_unlock(&c_conn->lock);

	return rc;
}

int sde_connector_set_property_for_commit(struct drm_connector *connector,
		struct drm_atomic_state *atomic_state,
		uint32_t property_idx, uint64_t value)
{
	struct drm_connector_state *state;
	struct drm_property *property;
	struct sde_connector *c_conn;

	if (!connector || !atomic_state) {
		SDE_ERROR("invalid argument(s), conn %d, state %d\n",
				connector != NULL, atomic_state != NULL);
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	property = msm_property_index_to_drm_property(
			&c_conn->property_info, property_idx);
	if (!property) {
		SDE_ERROR("invalid property index %d\n", property_idx);
		return -EINVAL;
	}

	state = drm_atomic_get_connector_state(atomic_state, connector);
	if (IS_ERR_OR_NULL(state)) {
		SDE_ERROR("failed to get conn %d state\n",
				connector->base.id);
		return -EINVAL;
	}

	return sde_connector_atomic_set_property(
			connector, state, property, value);
}

int sde_connector_helper_reset_custom_properties(
		struct drm_connector *connector,
		struct drm_connector_state *connector_state)
{
	struct sde_connector *c_conn;
	struct sde_connector_state *c_state;
	struct drm_property *drm_prop;
	enum msm_mdp_conn_property prop_idx;

	if (!connector || !connector_state) {
		SDE_ERROR("invalid params\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	c_state = to_sde_connector_state(connector_state);

	for (prop_idx = 0; prop_idx < CONNECTOR_PROP_COUNT; prop_idx++) {
		uint64_t val = c_state->property_values[prop_idx].value;
		uint64_t def;
		int ret;

		drm_prop = msm_property_index_to_drm_property(
				&c_conn->property_info, prop_idx);
		if (!drm_prop) {
			/* not all props will be installed, based on caps */
			SDE_DEBUG_CONN(c_conn, "invalid property index %d\n",
					prop_idx);
			continue;
		}

		def = msm_property_get_default(&c_conn->property_info,
				prop_idx);
		if (val == def)
			continue;

		SDE_DEBUG_CONN(c_conn, "set prop %s idx %d from %llu to %llu\n",
				drm_prop->name, prop_idx, val, def);

		ret = sde_connector_atomic_set_property(connector,
				connector_state, drm_prop, def);
		if (ret) {
			SDE_ERROR_CONN(c_conn,
					"set property failed, idx %d ret %d\n",
					prop_idx, ret);
			continue;
		}
	}

	return 0;
}

static int _sde_connector_lm_preference(struct sde_connector *sde_conn,
		 struct sde_kms *sde_kms, uint32_t disp_type)
{
	int ret = 0;
	u32 num_lm = 0;

	if (!sde_conn || !sde_kms || !sde_conn->ops.get_default_lms) {
		SDE_DEBUG("invalid input params");
		return -EINVAL;
	}

	if (!disp_type || disp_type >= SDE_CONNECTOR_MAX) {
		SDE_DEBUG("invalid display_type");
		return -EINVAL;
	}

	ret = sde_conn->ops.get_default_lms(sde_conn->display, &num_lm);
	if (ret || !num_lm) {
		SDE_DEBUG("failed to get default lm count");
		return ret;
	}

	if (num_lm > sde_kms->catalog->mixer_count) {
		SDE_DEBUG(
				"topology requesting more lms [%d] than hw exists [%d]",
				num_lm, sde_kms->catalog->mixer_count);
		return -EINVAL;
	}

	sde_conn->lm_mask = sde_hw_mixer_set_preference(sde_kms->catalog,
							num_lm, disp_type);

	return ret;
}

static void _sde_connector_init_hw_fence(struct sde_connector *c_conn,
		struct msm_display_info *display_info, struct sde_kms *sde_kms)
{
	/* enable hw-fence override if hw-fencing is disabled but vrr is supported */
	if (display_info->vrr_caps.video_psr_support || display_info->vrr_caps.arp_support ||
			sde_kms->catalog->hw_fence_rev)
		sde_kms->catalog->is_vrr_hw_fence_enable = true;

	/* Enable hw-fences for wb retire-fence */
	if (c_conn->connector_type == DRM_MODE_CONNECTOR_VIRTUAL && sde_kms->catalog->hw_fence_rev)
		c_conn->hwfence_wb_retire_fences_enable = true;
}

int sde_connector_get_panel_vfp(struct drm_connector *connector,
	struct drm_display_mode *mode)
{
	struct sde_connector *c_conn;
	int vfp = -EINVAL;

	if (!connector || !mode) {
		SDE_ERROR("invalid connector\n");
		return vfp;
	}
	c_conn = to_sde_connector(connector);
	if (!c_conn->ops.get_panel_vfp)
		return vfp;

	vfp = c_conn->ops.get_panel_vfp(c_conn->display,
		mode->hdisplay, mode->vdisplay);
	if (vfp <= 0)
		SDE_ERROR("Failed get_panel_vfp %d\n", vfp);

	return vfp;
}

static int _sde_debugfs_conn_cmd_tx_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->private_data = inode->i_private;
	return nonseekable_open(inode, file);
}

static ssize_t _sde_debugfs_conn_cmd_tx_sts_read(struct file *file,
		char __user *buf, size_t count, loff_t *ppos)
{
	struct drm_connector *connector = file->private_data;
	struct sde_connector *c_conn = NULL;
	char buffer[MAX_CMD_PAYLOAD_SIZE] = {0};
	int blen = 0;

	if (*ppos)
		return 0;

	if (!connector) {
		SDE_ERROR("invalid argument, conn is NULL\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);

	mutex_lock(&c_conn->lock);
	blen = snprintf(buffer, MAX_CMD_PAYLOAD_SIZE,
		"last_cmd_tx_sts:0x%x",
		c_conn->last_cmd_tx_sts);
	mutex_unlock(&c_conn->lock);

	SDE_DEBUG("output: %s\n", buffer);
	if (blen <= 0) {
		SDE_ERROR("snprintf failed, blen %d\n", blen);
		return -EINVAL;
	}

	if (blen > count)
		blen = count;

	blen = min_t(size_t, blen, MAX_CMD_PAYLOAD_SIZE);
	if (copy_to_user(buf, buffer, blen)) {
		SDE_ERROR("copy to user buffer failed\n");
		return -EFAULT;
	}

	*ppos += blen;
	return blen;
}

static ssize_t _sde_debugfs_conn_cmd_tx_write(struct file *file,
			const char __user *p, size_t count, loff_t *ppos)
{
	struct drm_connector *connector = file->private_data;
	struct sde_connector *c_conn = NULL;
	struct sde_kms *sde_kms;
	char *input, *token, *input_copy, *input_dup = NULL;
	const char *delim = " ";
	char buffer[MAX_CMD_PAYLOAD_SIZE] = {0};
	int rc = 0, strtoint = 0;
	u32 buf_size = 0;

	if (*ppos || !connector) {
		SDE_ERROR("invalid argument(s), conn %d\n", connector != NULL);
		return -EINVAL;
	}
	c_conn = to_sde_connector(connector);

	sde_kms = sde_connector_get_kms(&c_conn->base);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	}

	if (!c_conn->ops.cmd_transfer) {
		SDE_ERROR("no cmd transfer support for connector name %s\n",
				c_conn->name);
		return -EINVAL;
	}

	input = kzalloc(count + 1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	sde_vm_lock(sde_kms);
	if (!sde_vm_owns_hw(sde_kms)) {
		SDE_DEBUG("op not supported due to HW unavailablity\n");
		rc = -EOPNOTSUPP;
		goto end;
	}

	if (copy_from_user(input, p, count)) {
		SDE_ERROR("copy from user failed\n");
		rc = -EFAULT;
		goto end;
	}
	input[count] = '\0';

	SDE_INFO("Command requested for transfer to panel: %s\n", input);

	input_copy = kstrdup(input, GFP_KERNEL);
	if (!input_copy) {
		rc = -ENOMEM;
		goto end;
	}

	input_dup = input_copy;
	token = strsep(&input_copy, delim);
	while (token) {
		rc = kstrtoint(token, 0, &strtoint);
		if (rc) {
			SDE_ERROR("input buffer conversion failed\n");
			goto end1;
		}

		buffer[buf_size++] = (strtoint & 0xff);
		if (buf_size >= MAX_CMD_PAYLOAD_SIZE) {
			SDE_ERROR("buffer size exceeding the limit %d\n",
					MAX_CMD_PAYLOAD_SIZE);
			rc = -EFAULT;
			goto end1;
		}
		token = strsep(&input_copy, delim);
	}
	SDE_DEBUG("command packet size in bytes: %u\n", buf_size);
	if (!buf_size) {
		rc = -EFAULT;
		goto end1;
	}

	mutex_lock(&c_conn->lock);
	rc = c_conn->ops.cmd_transfer(&c_conn->base, c_conn->display, buffer,
			buf_size, false);
	c_conn->last_cmd_tx_sts = !rc ? true : false;
	mutex_unlock(&c_conn->lock);

	rc = count;
end1:
	kfree(input_dup);
end:
	sde_vm_unlock(sde_kms);
	kfree(input);
	return rc;
}

static const struct file_operations conn_cmd_tx_fops = {
	.open =		_sde_debugfs_conn_cmd_tx_open,
	.read =		_sde_debugfs_conn_cmd_tx_sts_read,
	.write =	_sde_debugfs_conn_cmd_tx_write,
};

static int _sde_debugfs_conn_cmd_rx_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->private_data = inode->i_private;
	return nonseekable_open(inode, file);
}

static ssize_t _sde_debugfs_conn_cmd_rx_read(struct file *file,
		char __user *buf, size_t count, loff_t *ppos)
{
	struct drm_connector *connector = file->private_data;
	struct sde_connector *c_conn = NULL;
	char *strs = NULL;
	char *strs_temp = NULL;
	int blen = 0, i = 0, n = 0, left_size = 0;

	if (*ppos)
		return 0;

	if (!connector) {
		SDE_ERROR("invalid argument, conn is NULL\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	if (c_conn->rx_len <= 0 || c_conn->rx_len > MAX_CMD_RECEIVE_SIZE) {
		SDE_ERROR("no valid data from panel\n");
		return -EINVAL;
	}

	/*
	 * Rx data was stored as HEX value in rx buffer,
	 * convert 1 HEX value to strings for display, need 5 bytes.
	 * for example: HEX value 0xFF, converted to strings, should be '0',
	 * 'x','F','F' and 1 space.
	 */
	left_size = c_conn->rx_len * 5 + 1;
	strs = kzalloc(left_size, GFP_KERNEL);
	if (!strs)
		return -ENOMEM;
	strs_temp = strs;

	mutex_lock(&c_conn->lock);
	for (i = 0; i < c_conn->rx_len; i++) {
		n = scnprintf(strs_temp, left_size, "0x%.2x ",
			     c_conn->cmd_rx_buf[i]);
		strs_temp += n;
		left_size -= n;
	}
	mutex_unlock(&c_conn->lock);

	blen = strlen(strs);
	if (blen <= 0) {
		SDE_ERROR("snprintf failed, blen %d\n", blen);
		blen = -EFAULT;
		goto err;
	}

	if (copy_to_user(buf, strs, blen)) {
		SDE_ERROR("copy to user buffer failed\n");
		blen = -EFAULT;
		goto err;
	}

	*ppos += blen;

err:
	kfree(strs);
	return blen;
}


static ssize_t _sde_debugfs_conn_cmd_rx_write(struct file *file,
			const char __user *p, size_t count, loff_t *ppos)
{
	struct drm_connector *connector = file->private_data;
	struct sde_connector *c_conn = NULL;
	char *input, *token, *input_copy, *input_dup = NULL;
	const char *delim = " ";
	unsigned char buffer[MAX_CMD_PAYLOAD_SIZE] = {0};
	int rc = 0, strtoint = 0;
	u32 buf_size = 0;

	if (*ppos || !connector) {
		SDE_ERROR("invalid argument(s), conn %d\n", connector != NULL);
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	if (!c_conn->ops.cmd_receive) {
		SDE_ERROR("no cmd receive support for connector name %s\n",
				c_conn->name);
		return -EINVAL;
	}

	memset(c_conn->cmd_rx_buf, 0x0, MAX_CMD_RECEIVE_SIZE);
	c_conn->rx_len = 0;

	input = kzalloc(count + 1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	if (copy_from_user(input, p, count)) {
		SDE_ERROR("copy from user failed\n");
		rc  = -EFAULT;
		goto end;
	}
	input[count] = '\0';

	SDE_INFO("Command requested for rx from panel: %s\n", input);

	input_copy = kstrdup(input, GFP_KERNEL);
	if (!input_copy) {
		rc = -ENOMEM;
		goto end;
	}

	input_dup = input_copy;
	token = strsep(&input_copy, delim);
	while (token) {
		rc = kstrtoint(token, 0, &strtoint);
		if (rc) {
			SDE_ERROR("input buffer conversion failed\n");
			goto end1;
		}

		buffer[buf_size++] = (strtoint & 0xff);
		if (buf_size >= MAX_CMD_PAYLOAD_SIZE) {
			SDE_ERROR("buffer size = %d exceeding the limit %d\n",
					buf_size, MAX_CMD_PAYLOAD_SIZE);
			rc = -EFAULT;
			goto end1;
		}
		token = strsep(&input_copy, delim);
	}

	if (!buffer[0] || buffer[0] > MAX_CMD_RECEIVE_SIZE) {
		SDE_ERROR("invalid rx length\n");
		rc = -EFAULT;
		goto end1;
	}

	SDE_DEBUG("command packet size in bytes: %u, rx len: %u\n",
			buf_size, buffer[0]);
	if (!buf_size) {
		rc = -EFAULT;
		goto end1;
	}

	mutex_lock(&c_conn->lock);
	c_conn->rx_len = c_conn->ops.cmd_receive(c_conn->display, buffer + 1,
			buf_size - 1, c_conn->cmd_rx_buf, buffer[0], NULL);
	mutex_unlock(&c_conn->lock);

	if (c_conn->rx_len <= 0)
		rc = -EINVAL;
	else
		rc = count;
end1:
	kfree(input_dup);
end:
	kfree(input);
	return rc;
}

static const struct file_operations conn_cmd_rx_fops = {
	.open =         _sde_debugfs_conn_cmd_rx_open,
	.read =         _sde_debugfs_conn_cmd_rx_read,
	.write =        _sde_debugfs_conn_cmd_rx_write,
};

#if IS_ENABLED(CONFIG_DEBUG_FS)
/**
 * sde_connector_init_debugfs - initialize connector debugfs
 * @connector: Pointer to drm connector
 */
static int sde_connector_init_debugfs(struct drm_connector *connector)
{
	struct sde_connector *sde_connector;
	struct msm_display_info info;
	struct sde_kms *sde_kms;

	if (!connector || !connector->debugfs_entry) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	sde_kms = sde_connector_get_kms(connector);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	}

	sde_connector = to_sde_connector(connector);

	sde_connector_get_info(connector, &info);
	if (sde_connector->ops.check_status &&
		(info.capabilities & MSM_DISPLAY_ESD_ENABLED)) {
		debugfs_create_u32("esd_status_interval", 0600,
				connector->debugfs_entry,
				&sde_connector->esd_status_interval);
	}

	if (sde_connector->ops.cmd_transfer) {
		if (!debugfs_create_file("tx_cmd", 0600,
			connector->debugfs_entry,
			connector, &conn_cmd_tx_fops)) {
			SDE_ERROR("failed to create connector cmd_tx\n");
			return -ENOMEM;
		}
	}

	if (sde_connector->ops.cmd_receive) {
		if (!debugfs_create_file("rx_cmd", 0600,
			connector->debugfs_entry,
			connector, &conn_cmd_rx_fops)) {
			SDE_ERROR("failed to create connector cmd_rx\n");
			return -ENOMEM;
		}
	}

	if (sde_connector->connector_type == DRM_MODE_CONNECTOR_VIRTUAL &&
			sde_kms->catalog->hw_fence_rev)
		debugfs_create_bool("wb_hw_fence_enable", 0600, connector->debugfs_entry,
			&sde_connector->hwfence_wb_retire_fences_enable);

	if (sde_connector->connector_type == DRM_MODE_CONNECTOR_DSI) {
		debugfs_create_u32("num_bl_frames", 0600, connector->debugfs_entry,
				&sde_connector->num_bl_frames);
		debugfs_create_bool("disable_cont_dimming", 0600, connector->debugfs_entry,
				&sde_connector->disable_cont_dimming);
	}

	return 0;
}
#else
static int sde_connector_init_debugfs(struct drm_connector *connector)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

static int sde_connector_late_register(struct drm_connector *connector)
{
	return sde_connector_init_debugfs(connector);
}

static void sde_connector_early_unregister(struct drm_connector *connector)
{
	/* debugfs under connector->debugfs are deleted by drm_debugfs */
}

static int sde_connector_fill_modes(struct drm_connector *connector,
		uint32_t max_width, uint32_t max_height)
{
	int rc, mode_count = 0;
	struct sde_connector *sde_conn = NULL;

	sde_conn = to_sde_connector(connector);
	if (!sde_conn) {
		SDE_ERROR("invalid arguments\n");
		return 0;
	}

	mode_count = drm_helper_probe_single_connector_modes(connector,
			max_width, max_height);

	sde_conn->max_mode_width = sde_conn_get_max_mode_width(connector);

	if (sde_conn->ops.set_allowed_mode_switch)
		sde_conn->ops.set_allowed_mode_switch(connector,
				sde_conn->display);

	rc = sde_connector_set_blob_data(connector,
				connector->state,
				CONNECTOR_PROP_MODE_INFO);
	if (rc) {
		SDE_ERROR_CONN(sde_conn,
			"failed to setup mode info prop, rc = %d\n", rc);
		return 0;
	}

	return mode_count;
}

static const struct drm_connector_funcs sde_connector_ops = {
	.reset =                  sde_connector_atomic_reset,
	.destroy =                sde_connector_destroy,
	.fill_modes =             sde_connector_fill_modes,
	.atomic_duplicate_state = sde_connector_atomic_duplicate_state,
	.atomic_destroy_state =   sde_connector_atomic_destroy_state,
	.atomic_set_property =    sde_connector_atomic_set_property,
	.atomic_get_property =    sde_connector_atomic_get_property,
	.late_register =          sde_connector_late_register,
	.early_unregister =       sde_connector_early_unregister,
};

static int sde_connector_get_modes(struct drm_connector *connector)
{
	struct sde_connector *c_conn;
	struct msm_resource_caps_info avail_res;
	int mode_count = 0;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return 0;
	}

	c_conn = to_sde_connector(connector);
	if (!c_conn->ops.get_modes) {
		SDE_DEBUG("missing get_modes callback\n");
		return 0;
	}

	memset(&avail_res, 0, sizeof(avail_res));
	sde_connector_get_avail_res_info(connector, &avail_res);

	mode_count = c_conn->ops.get_modes(connector, c_conn->display,
			&avail_res);
	if (!mode_count) {
		SDE_ERROR_CONN(c_conn, "failed to get modes\n");
		return 0;
	}

	if (c_conn->hdr_capable)
		sde_connector_update_hdr_props(connector);

	if (c_conn->connector_type == DRM_MODE_CONNECTOR_DisplayPort)
		sde_connector_update_colorspace(connector);

	return mode_count;
}

static enum drm_mode_status
sde_connector_mode_valid(struct drm_connector *connector,
		struct drm_display_mode *mode)
{
	struct sde_connector *c_conn;
	struct msm_resource_caps_info avail_res;

	if (!connector || !mode) {
		SDE_ERROR("invalid argument(s), conn %pK, mode %pK\n",
				connector, mode);
		return MODE_ERROR;
	}

	c_conn = to_sde_connector(connector);

	memset(&avail_res, 0, sizeof(avail_res));
	sde_connector_get_avail_res_info(connector, &avail_res);

	if (c_conn->ops.mode_valid)
		return c_conn->ops.mode_valid(connector, mode, c_conn->display,
				&avail_res);

	/* assume all modes okay by default */
	return MODE_OK;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static struct drm_encoder *
sde_connector_atomic_best_encoder(struct drm_connector *connector,
		struct drm_atomic_state *state)
{
	struct sde_connector *c_conn;
	struct drm_encoder *encoder = NULL;
	struct drm_connector_state *connector_state = NULL;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return NULL;
	}

	connector_state = drm_atomic_get_new_connector_state(state, connector);
	c_conn = to_sde_connector(connector);

	if (c_conn->ops.atomic_best_encoder)
		encoder = c_conn->ops.atomic_best_encoder(connector,
				c_conn->display, connector_state);

	return encoder;
}
#else
static struct drm_encoder *
sde_connector_atomic_best_encoder(struct drm_connector *connector,
		struct drm_connector_state *connector_state)
{
	struct sde_connector *c_conn;
	struct drm_encoder *encoder = NULL;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return NULL;
	}

	c_conn = to_sde_connector(connector);

	if (c_conn->ops.atomic_best_encoder)
		encoder = c_conn->ops.atomic_best_encoder(connector,
				c_conn->display, connector_state);

	return encoder;
}
#endif

static int sde_connector_atomic_check(struct drm_connector *connector,
		struct drm_atomic_state *state)
{
	struct sde_connector *c_conn;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(connector);
	if (c_conn->ops.atomic_check)
		return c_conn->ops.atomic_check(connector,
				c_conn->display, state);

	return 0;
}

static void _sde_connector_report_panel_dead(struct sde_connector *conn,
	bool skip_pre_kickoff)
{
	struct drm_event event;

	if (!conn)
		return;

	/* Panel dead notification can come:
	 * 1) ESD thread
	 * 2) Commit thread (if TE stops coming)
	 * So such case, avoid failure notification twice.
	 */
	if (conn->panel_dead)
		return;

	SDE_EVT32(SDE_EVTLOG_ERROR);
	conn->panel_dead = true;
	sde_encoder_display_failure_notification(conn->encoder,
		skip_pre_kickoff);

	event.type = DRM_EVENT_PANEL_DEAD;
	event.length = sizeof(bool);
	msm_mode_object_event_notify(&conn->base.base,
		conn->base.dev, &event, (u8 *)&conn->panel_dead);
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	{
		struct dsi_display *display = conn->display;
		struct samsung_display_driver_data *vdd = display->panel->panel_private;

		vdd->panel_dead = true;
		LCD_ERR(vdd, "Report Panel Dead\n");
	}
#else
	SDE_ERROR("esd check failed report PANEL_DEAD conn_id: %d enc_id: %d\n",
			conn->base.base.id, conn->encoder->base.id);
#endif
}

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
static void _sde_connector_report_dyn_mipi_clock(struct sde_connector *conn,
	u32 bit_clk)
{
	struct drm_event event;
	struct drm_msm_dyn_mipi_clk_info clk_info;

	if (!conn) {
		SDE_ERROR("No conn..\n");
		return;
	}

	clk_info.dyn_bit_clk = bit_clk;

	event.type = DRM_EVENT_DYN_CLK;
	event.length = sizeof(clk_info);

	msm_mode_object_event_notify(&conn->base.base, conn->base.dev, &event, (u8 *)&clk_info);

	SDE_INFO("report DRM_EVENT_DYN_CLK conn_id: %d enc_id: %d bit_clk: %d\n",
			conn->base.base.id, conn->encoder->base.id, bit_clk);
}
#endif

const char *sde_conn_get_topology_name(struct drm_connector *conn,
		struct msm_display_topology topology)
{
	struct sde_kms *sde_kms;
	int topology_idx = 0;

	sde_kms = sde_connector_get_kms(conn);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return NULL;
	}

	topology_idx = (int)sde_rm_get_topology_name(&sde_kms->rm,
				topology);

	if (topology_idx >= SDE_RM_TOPOLOGY_MAX) {
		SDE_ERROR("invalid topology\n");
		return NULL;
	}

	return e_topology_name[topology_idx].name;
}

int sde_connector_esd_status(struct drm_connector *conn)
{
	struct sde_connector *sde_conn = NULL;
	struct dsi_display *display;
	int ret = 0;

	if (!conn)
		return ret;

	sde_conn = to_sde_connector(conn);
	if (!sde_conn || !sde_conn->ops.check_status)
		return ret;

	display = _sde_connector_get_display(sde_conn);
	if (!display)
		return 0;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	/* Do not panel recovery when ub is disconnected. */
	{
		struct samsung_display_driver_data *vdd = display->panel->panel_private;
		if (!ss_is_ub_connected(vdd)) {
			LCD_ERR(vdd, "Panel is Disconnected\n");
			return 0;
		}
	}
#endif

	/* protect this call with ESD status check call */
	mutex_lock(&sde_conn->lock);
	if (atomic_read(&(display->panel->esd_recovery_pending))) {
		SDE_ERROR("ESD recovery already pending\n");
		mutex_unlock(&sde_conn->lock);
		return -ETIMEDOUT;
	}
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	ret = sde_conn->ops.check_status(&sde_conn->base,
					 sde_conn->display, false);
#else
	ret = sde_conn->ops.check_status(&sde_conn->base,
					 sde_conn->display, true);
#endif
	mutex_unlock(&sde_conn->lock);

	if (ret <= 0) {
		/* cancel if any pending esd work */
		sde_connector_schedule_status_work(conn, false);
		_sde_connector_report_panel_dead(sde_conn, true);
		ret = -ETIMEDOUT;
	} else {
		SDE_DEBUG("Successfully received TE from panel\n");
		ret = 0;
	}
	SDE_EVT32(ret);

	return ret;
}

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
static void sde_connector_dyn_clk_work(struct work_struct *work)
{
	struct sde_connector *conn;
	struct dsi_display *display;
	struct samsung_display_driver_data *vdd;

	conn = container_of(to_delayed_work(work),
			struct sde_connector, dyn_clk_work);
	if (!conn) {
		SDE_ERROR("not able to get connector object\n");
		return;
	}

	display = _sde_connector_get_display(conn);
	if (!display) {
		SDE_ERROR("No display..\n");
		return;
	}

	vdd = display->panel->panel_private;

	_sde_connector_report_dyn_mipi_clock(conn, vdd->dyn_mipi_clk.requested_clk_rate);

	return;
}
#endif

static void sde_connector_check_status_work(struct work_struct *work)
{
	struct sde_connector *conn;
	int rc = 0;
	struct device *dev;

	conn = container_of(to_delayed_work(work),
			struct sde_connector, status_work);
	if (!conn) {
		SDE_ERROR("not able to get connector object\n");
		return;
	}

	mutex_lock(&conn->lock);
	dev = conn->base.dev->dev;

	if (!conn->ops.check_status || dev->power.is_suspended ||
			(conn->lp_mode == SDE_MODE_DPMS_OFF)) {
		SDE_DEBUG("dpms mode: %d\n", conn->dpms_mode);
		mutex_unlock(&conn->lock);
		return;
	}

	rc = conn->ops.check_status(&conn->base, conn->display, false);
	mutex_unlock(&conn->lock);

	if (rc > 0) {
		u32 interval;

		SDE_DEBUG("esd check status success conn_id: %d enc_id: %d\n",
				conn->base.base.id, conn->encoder->base.id);

		/* If debugfs property is not set then take default value */
		interval = conn->esd_status_interval ?
			conn->esd_status_interval : STATUS_CHECK_INTERVAL_MS;
#if !IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
		schedule_delayed_work(&conn->status_work,
			msecs_to_jiffies(interval));
#endif
		return;
	}

	_sde_connector_report_panel_dead(conn, false);
}

static const struct drm_connector_helper_funcs sde_connector_helper_ops = {
	.get_modes =    sde_connector_get_modes,
	.detect_ctx =   sde_connector_detect_ctx,
	.mode_valid =   sde_connector_mode_valid,
	.best_encoder = sde_connector_best_encoder,
	.atomic_check = sde_connector_atomic_check,
};

static const struct drm_connector_helper_funcs sde_connector_helper_ops_v2 = {
	.get_modes =    sde_connector_get_modes,
	.detect_ctx =   sde_connector_detect_ctx,
	.mode_valid =   sde_connector_mode_valid,
	.best_encoder = sde_connector_best_encoder,
	.atomic_best_encoder = sde_connector_atomic_best_encoder,
	.atomic_check = sde_connector_atomic_check,
};

static int sde_connector_populate_mode_info(struct drm_connector *conn,
	struct sde_kms_info *info)
{
	struct sde_kms *sde_kms;
	struct sde_connector *c_conn = NULL;
	struct drm_display_mode *mode;
	struct msm_mode_info mode_info;
	struct msm_freq_step_list *freq_step_list;
	const char *topo_name = NULL;
	int rc = 0, i;

	sde_kms = sde_connector_get_kms(conn);
	if (!sde_kms) {
		SDE_ERROR("invalid kms\n");
		return -EINVAL;
	}

	c_conn = to_sde_connector(conn);
	if (!c_conn->ops.get_mode_info) {
		SDE_ERROR_CONN(c_conn, "get_mode_info not defined\n");
		return -EINVAL;
	}

	list_for_each_entry(mode, &conn->modes, head) {

		memset(&mode_info, 0, sizeof(mode_info));

		rc = sde_connector_get_mode_info(&c_conn->base, mode, NULL,
				&mode_info);
		if (rc) {
			SDE_ERROR_CONN(c_conn,
				"failed to get mode info for mode %s\n",
				mode->name);
			continue;
		}

		sde_kms_info_add_keystr(info, "mode_name", mode->name);

		sde_kms_info_add_keyint(info, "bit_clk_rate",
					mode_info.clk_rate);

		if (c_conn->ops.set_submode_info) {
			c_conn->ops.set_submode_info(conn, info, c_conn->display, mode);
		} else {
			topo_name = sde_conn_get_topology_name(conn, mode_info.topology);
			if (topo_name)
				sde_kms_info_add_keystr(info, "topology", topo_name);
		}

		sde_kms_info_add_keyint(info, "qsync_min_fps", mode_info.qsync_min_fps);
		sde_kms_info_add_keyint(info, "avr_step_fps", mode_info.avr_step_fps);

		if (mode_info.freq_step_list) {
			freq_step_list = mode_info.freq_step_list;
			for (i = 0; i < freq_step_list->count; i++) {
				sde_kms_info_add_keyint(info, "Frame_interval:",
					freq_step_list->freq_pattern[i].frame_interval);
				sde_kms_info_add_keyint(info, "usecase:",
					freq_step_list->freq_pattern[i].usecase_idx);
				sde_kms_info_add_list(info, "Pattern:",
						freq_step_list->freq_pattern[i].freq_stepping_seq,
						freq_step_list->freq_pattern[i].length);
			}
		}

		if (c_conn->vrr_caps.video_psr_support)
			sde_kms_info_add_keyint(info, "has_vhm_support", 1);

		if (c_conn->vrr_caps.vrr_support)
			sde_kms_info_add_keyint(info, "early_ept_timeout",
				IDLE_POWERCOLLAPSE_DURATION * NSEC_PER_MSEC);

		sde_kms_info_add_keyint(info, "has_cwb_crop", test_bit(SDE_FEATURE_CWB_CROP,
								       sde_kms->catalog->features));
		sde_kms_info_add_keyint(info, "has_dedicated_cwb_support",
				test_bit(SDE_FEATURE_DEDICATED_CWB, sde_kms->catalog->features));
		if (test_bit(SDE_FEATURE_DUAL_DEDICATED_CWB, sde_kms->catalog->features))
			sde_kms_info_add_keyint(info, "max_cwb", 2);
		else
			sde_kms_info_add_keyint(info, "max_cwb", 1);

		sde_kms_info_add_keyint(info, "mdp_transfer_time_us",
			mode_info.mdp_transfer_time_us);

		if (mode_info.mdp_transfer_time_us_min && mode_info.mdp_transfer_time_us_max) {
			sde_kms_info_add_keyint(info, "mdp_transfer_time_us_min",
					mode_info.mdp_transfer_time_us_min);
			sde_kms_info_add_keyint(info, "mdp_transfer_time_us_max",
					mode_info.mdp_transfer_time_us_max);
		}

		sde_kms_info_add_keyint(info, "allowed_mode_switch",
			mode_info.allowed_mode_switches);

		if (!mode_info.roi_caps.num_roi)
			continue;

		sde_kms_info_add_keyint(info, "partial_update_num_roi",
			mode_info.roi_caps.num_roi);
		sde_kms_info_add_keyint(info, "partial_update_xstart",
			mode_info.roi_caps.align.xstart_pix_align);
		sde_kms_info_add_keyint(info, "partial_update_walign",
			mode_info.roi_caps.align.width_pix_align);
		sde_kms_info_add_keyint(info, "partial_update_wmin",
			mode_info.roi_caps.align.min_width);
		sde_kms_info_add_keyint(info, "partial_update_ystart",
			mode_info.roi_caps.align.ystart_pix_align);
		sde_kms_info_add_keyint(info, "partial_update_halign",
			mode_info.roi_caps.align.height_pix_align);
		sde_kms_info_add_keyint(info, "partial_update_hmin",
			mode_info.roi_caps.align.min_height);
		sde_kms_info_add_keyint(info, "partial_update_roimerge",
			mode_info.roi_caps.merge_rois);
	}

	return rc;
}

int sde_connector_set_blob_data(struct drm_connector *conn,
		struct drm_connector_state *state,
		enum msm_mdp_conn_property prop_id)
{
	struct sde_kms_info *info;
	struct sde_connector *c_conn = NULL;
	struct sde_connector_state *sde_conn_state = NULL;
	struct msm_mode_info mode_info;
	struct drm_property_blob **blob = NULL;
	int rc = 0;

	c_conn = to_sde_connector(conn);
	if (!c_conn) {
		SDE_ERROR("invalid argument\n");
		return -EINVAL;
	}

	info = vzalloc(sizeof(*info));
	if (!info)
		return -ENOMEM;

	sde_kms_info_reset(info);

	switch (prop_id) {
	case CONNECTOR_PROP_SDE_INFO:
		memset(&mode_info, 0, sizeof(mode_info));

		if (state) {
			sde_conn_state = to_sde_connector_state(state);
			memcpy(&mode_info, &sde_conn_state->mode_info,
					sizeof(sde_conn_state->mode_info));
		} else {
			/**
			 * connector state is assigned only on first
			 * atomic_commit. But this function is allowed to be
			 * invoked during probe/init sequence. So not throwing
			 * an error.
			 */
			SDE_DEBUG_CONN(c_conn, "invalid connector state\n");
		}

		if (c_conn->ops.set_info_blob) {
			rc = c_conn->ops.set_info_blob(conn, info,
					c_conn->display, &mode_info);
			if (rc) {
				SDE_ERROR_CONN(c_conn,
						"set_info_blob failed, %d\n",
						rc);
				goto exit;
			}
		}

		blob = &c_conn->blob_caps;
	break;
	case CONNECTOR_PROP_MODE_INFO:
		rc = sde_connector_populate_mode_info(conn, info);
		if (rc) {
			SDE_ERROR_CONN(c_conn,
					"mode info population failed, %d\n",
					rc);
			goto exit;
		}
		blob = &c_conn->blob_mode_info;
	break;
	default:
		SDE_ERROR_CONN(c_conn, "invalid prop_id: %d\n", prop_id);
		goto exit;
	}

	msm_property_set_blob(&c_conn->property_info,
			blob,
			SDE_KMS_INFO_DATA(info),
			SDE_KMS_INFO_DATALEN(info),
			prop_id);
exit:
	vfree(info);

	return rc;
}

static void _sde_connector_install_qsync_properties(struct sde_kms *sde_kms,
		struct sde_connector *c_conn, struct dsi_display *dsi_display,
		struct msm_display_info *display_info)
{
	static const struct drm_prop_enum_list e_avr_step_state[] = {
		{AVR_STEP_NONE, "avr_step_none"},
		{AVR_STEP_ENABLE, "avr_step_enable"},
		{AVR_STEP_DISABLE, "avr_step_disable"},
	};

	if (test_bit(SDE_FEATURE_QSYNC, sde_kms->catalog->features) && dsi_display &&
			dsi_display->panel && dsi_display->panel->qsync_caps.qsync_support) {
		msm_property_install_enum(&c_conn->property_info, "qsync_mode", 0, 0, e_qsync_mode,
				ARRAY_SIZE(e_qsync_mode), 0, CONNECTOR_PROP_QSYNC_MODE);

		if (test_bit(SDE_FEATURE_EPT_FPS, sde_kms->catalog->features) &&
				(display_info->capabilities & MSM_DISPLAY_CAP_CMD_MODE))
			msm_property_install_range(&c_conn->property_info,
					"EPT_FPS", 0x0, 0, U32_MAX, 0, CONNECTOR_PROP_EPT_FPS);
	}

	if (test_bit(SDE_FEATURE_AVR_STEP, sde_kms->catalog->features))
		msm_property_install_enum(&c_conn->property_info, "avr_step_state",
				0, 0, e_avr_step_state, ARRAY_SIZE(e_avr_step_state), 0,
				CONNECTOR_PROP_AVR_STEP_STATE);
}

static int _sde_connector_install_properties(struct drm_device *dev,
	struct sde_kms *sde_kms, struct sde_connector *c_conn,
	int connector_type, void *display,
	struct msm_display_info *display_info)
{
	struct dsi_display *dsi_display;
	int rc;
	struct drm_connector *connector;
	u64 panel_id = ~0x0;

	msm_property_install_blob(&c_conn->property_info, "capabilities",
			DRM_MODE_PROP_IMMUTABLE, CONNECTOR_PROP_SDE_INFO);

	rc = sde_connector_set_blob_data(&c_conn->base,
			NULL, CONNECTOR_PROP_SDE_INFO);
	if (rc) {
		SDE_ERROR_CONN(c_conn,
			"failed to setup connector info, rc = %d\n", rc);
		return rc;
	}

	connector = &c_conn->base;

	msm_property_install_blob(&c_conn->property_info, "mode_properties",
			DRM_MODE_PROP_IMMUTABLE, CONNECTOR_PROP_MODE_INFO);

	if (connector_type == DRM_MODE_CONNECTOR_DSI) {
		dsi_display = _sde_connector_get_display(c_conn);
		if (dsi_display && dsi_display->panel) {
			msm_property_install_blob(&c_conn->property_info,
				"dimming_bl_lut", DRM_MODE_PROP_BLOB,
				CONNECTOR_PROP_DIMMING_BL_LUT);
			msm_property_install_range(&c_conn->property_info, "dimming_dyn_ctrl",
					0x0, 0, ~0, 0, CONNECTOR_PROP_DIMMING_CTRL);
			msm_property_install_range(&c_conn->property_info, "dimming_min_bl",
					0x0, 0, dsi_display->panel->bl_config.brightness_max_level, 0,
					CONNECTOR_PROP_DIMMING_MIN_BL);
		}

		if (dsi_display && dsi_display->panel &&
			dsi_display->panel->hdr_props.hdr_enabled == true) {
			msm_property_install_blob(&c_conn->property_info,
				"hdr_properties",
				DRM_MODE_PROP_IMMUTABLE,
				CONNECTOR_PROP_HDR_INFO);

			msm_property_set_blob(&c_conn->property_info,
				&c_conn->blob_hdr,
				&dsi_display->panel->hdr_props,
				sizeof(dsi_display->panel->hdr_props),
				CONNECTOR_PROP_HDR_INFO);
		}

		if (dsi_display && dsi_display->panel &&
				dsi_display->panel->dyn_clk_caps.dyn_clk_support)
			msm_property_install_range(&c_conn->property_info, "dyn_bit_clk",
					0x0, 0, ~0, 0, CONNECTOR_PROP_DYN_BIT_CLK);

		msm_property_install_range(&c_conn->property_info, "dyn_transfer_time",
				 0x0, 0, 1000000, 0, CONNECTOR_PROP_DYN_TRANSFER_TIME);

		mutex_lock(&c_conn->base.dev->mode_config.mutex);
		sde_connector_fill_modes(&c_conn->base,
						dev->mode_config.max_width,
						dev->mode_config.max_height);
		mutex_unlock(&c_conn->base.dev->mode_config.mutex);
	}

	msm_property_install_volatile_range(
			&c_conn->property_info, "sde_drm_roi_v1", 0x0,
			0, ~0, 0, CONNECTOR_PROP_ROI_V1);

	/* install PP_DITHER properties */
	_sde_connector_install_dither_property(dev, sde_kms, c_conn);

	if (connector_type == DRM_MODE_CONNECTOR_DisplayPort) {
		struct drm_msm_ext_hdr_properties hdr = {0};

		c_conn->hdr_capable = true;

		msm_property_install_blob(&c_conn->property_info,
				"ext_hdr_properties",
				DRM_MODE_PROP_IMMUTABLE,
				CONNECTOR_PROP_EXT_HDR_INFO);

		/* set default values to avoid reading uninitialized data */
		msm_property_set_blob(&c_conn->property_info,
			      &c_conn->blob_ext_hdr,
			      &hdr,
			      sizeof(hdr),
			      CONNECTOR_PROP_EXT_HDR_INFO);

		if (c_conn->ops.install_properties)
			c_conn->ops.install_properties(display, connector);
	}

	msm_property_install_volatile_range(&c_conn->property_info,
		"hdr_metadata", 0x0, 0, ~0, 0, CONNECTOR_PROP_HDR_METADATA);

	msm_property_install_volatile_range(&c_conn->property_info,
		"RETIRE_FENCE", 0x0, 0, ~0, 0, CONNECTOR_PROP_RETIRE_FENCE);

	msm_property_install_volatile_range(&c_conn->property_info,
		"RETIRE_FENCE_OFFSET", 0x0, 0, ~0, 0,
		 CONN_PROP_RETIRE_FENCE_OFFSET);

	msm_property_install_range(&c_conn->property_info, "autorefresh",
			0x0, 0, AUTOREFRESH_MAX_FRAME_CNT, 0,
			CONNECTOR_PROP_AUTOREFRESH);

	c_conn->vrr_caps = display_info->vrr_caps;

	if (c_conn->vrr_caps.vrr_support) {
		msm_property_install_range(&c_conn->property_info, "frame_interval", 0x0,
			0, U64_MAX, 0, CONNECTOR_PROP_FRAME_INTERVAL);

		msm_property_install_range(&c_conn->property_info, "usecase_idx", 0x0,
			0, U64_MAX, 0, CONNECTOR_PROP_USECASE_IDX);
	}

	if (connector_type == DRM_MODE_CONNECTOR_DSI) {
		_sde_connector_install_qsync_properties(sde_kms, c_conn, dsi_display, display_info);

		if (test_bit(SDE_FEATURE_EPT, sde_kms->catalog->features))
			msm_property_install_range(&c_conn->property_info, "EPT", 0x0, 0, U64_MAX,
				0, CONNECTOR_PROP_EPT);

		msm_property_install_enum(&c_conn->property_info, "dsc_mode", 0,
			0, e_dsc_mode, ARRAY_SIZE(e_dsc_mode), 0, CONNECTOR_PROP_DSC_MODE);

		if (dsi_display && dsi_display->panel &&
			display_info->capabilities & MSM_DISPLAY_CAP_CMD_MODE &&
			display_info->capabilities & MSM_DISPLAY_CAP_VID_MODE)
			msm_property_install_enum(&c_conn->property_info,
			"panel_mode", 0, 0,
			e_panel_mode,
			ARRAY_SIZE(e_panel_mode),
			(dsi_display->panel->panel_mode == DSI_OP_VIDEO_MODE) ? 0 : 1,
			CONNECTOR_PROP_SET_PANEL_MODE);

		msm_property_install_enum(&c_conn->property_info, "bpp_mode", 0,
			0, e_bpp_mode, ARRAY_SIZE(e_bpp_mode), 0, CONNECTOR_PROP_BPP_MODE);

		if (test_bit(SDE_FEATURE_DEMURA, sde_kms->catalog->features)) {
			msm_property_install_blob(&c_conn->property_info,
				"DEMURA_PANEL_ID", DRM_MODE_PROP_IMMUTABLE,
				CONNECTOR_PROP_DEMURA_PANEL_ID);
			msm_property_set_blob(&c_conn->property_info,
			      &c_conn->blob_panel_id,
			      &panel_id,
			      sizeof(panel_id),
			      CONNECTOR_PROP_DEMURA_PANEL_ID);
		}
	}

	if ((display_info->capabilities & MSM_DISPLAY_CAP_CMD_MODE)
			|| (connector_type == DRM_MODE_CONNECTOR_VIRTUAL))
		msm_property_install_enum(&c_conn->property_info, "frame_trigger_mode",
			0, 0, e_frame_trigger_mode, ARRAY_SIZE(e_frame_trigger_mode), 0,
			CONNECTOR_PROP_CMD_FRAME_TRIGGER_MODE);

	msm_property_install_range(&c_conn->property_info, "bl_scale",
		0x0, 0, MAX_BL_SCALE_LEVEL, MAX_BL_SCALE_LEVEL,
		CONNECTOR_PROP_BL_SCALE);

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	/* SAMSUNG_FINGERPRINT */
	msm_property_install_range(&c_conn->property_info, "fingerprint_mask",
		0x0, 0, 100, 0,
		CONNECTOR_PROP_FINGERPRINT_MASK);
#endif

	msm_property_install_range(&c_conn->property_info, "sv_bl_scale",
		0x0, 0, U32_MAX, MAX_SV_BL_SCALE_LEVEL,
		CONNECTOR_PROP_SV_BL_SCALE);

	c_conn->bl_scale_dirty = false;
	c_conn->bl_scale = MAX_BL_SCALE_LEVEL;
	c_conn->bl_scale_sv = MAX_SV_BL_SCALE_LEVEL;

	if (connector_type == DRM_MODE_CONNECTOR_DisplayPort)
		msm_property_install_range(&c_conn->property_info,
			"supported_colorspaces",
			DRM_MODE_PROP_IMMUTABLE, 0, 0xffff, 0,
			CONNECTOR_PROP_SUPPORTED_COLORSPACES);

	/* enum/bitmask properties */
	msm_property_install_enum(&c_conn->property_info, "topology_name",
			DRM_MODE_PROP_IMMUTABLE, 0, e_topology_name,
			ARRAY_SIZE(e_topology_name), 0,
			CONNECTOR_PROP_TOPOLOGY_NAME);
	msm_property_install_enum(&c_conn->property_info, "topology_control",
			0, 1, e_topology_control,
			ARRAY_SIZE(e_topology_control), 0,
			CONNECTOR_PROP_TOPOLOGY_CONTROL);
	msm_property_install_enum(&c_conn->property_info, "LP",
			0, 0, e_power_mode,
			ARRAY_SIZE(e_power_mode), 0,
			CONNECTOR_PROP_LP);

	if (connector_type == DRM_MODE_CONNECTOR_DSI) {
		dsi_display = (struct dsi_display *)(display);
		if (dsi_display && dsi_display->panel) {
			msm_property_install_range(&c_conn->property_info, "brightness",
			0x0, 0, dsi_display->panel->bl_config.brightness_max_level, 0,
			CONNECTOR_PROP_BRIGHTNESS);
		}
	}

	return 0;
}

static ssize_t panel_power_state_show(struct device *device,
	struct device_attribute *attr, char *buf)
{
	struct drm_connector *conn;
	struct sde_connector *sde_conn;

	conn = dev_get_drvdata(device);
	sde_conn = to_sde_connector(conn);

	return scnprintf(buf, PAGE_SIZE, "%d\n", sde_conn->last_panel_power_mode);
}

static ssize_t twm_enable_store(struct device *device,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct drm_connector *conn;
	struct sde_connector *sde_conn;
	struct dsi_display *dsi_display;
	int rc;
	int data;

	conn = dev_get_drvdata(device);
	sde_conn = to_sde_connector(conn);
	dsi_display = (struct dsi_display *) sde_conn->display;
	rc = kstrtoint(buf, 10, &data);
	if (rc) {
		SDE_ERROR("kstrtoint failed, rc = %d\n", rc);
		return -EINVAL;
	}
	sde_conn->twm_en = data ? true : false;
	dsi_display->twm_enabled = sde_conn->twm_en;
	sde_conn->allow_bl_update = data ? false : true;

	SDE_DEBUG("TWM: %s\n", sde_conn->twm_en ? "ENABLED" : "DISABLED");
	return count;
}

static ssize_t twm_enable_show(struct device *device,
	struct device_attribute *attr, char *buf)
{
	struct drm_connector *conn;
	struct sde_connector *sde_conn;

	conn = dev_get_drvdata(device);
	sde_conn = to_sde_connector(conn);

	SDE_DEBUG("TWM: %s\n", sde_conn->twm_en ? "ENABLED" : "DISABLED");
	return scnprintf(buf, PAGE_SIZE, "%d\n", sde_conn->twm_en);
}

static DEVICE_ATTR_RO(panel_power_state);
static DEVICE_ATTR_RW(twm_enable);

static struct attribute *sde_connector_dev_attrs[] = {
	&dev_attr_panel_power_state.attr,
	&dev_attr_twm_enable.attr,
	NULL
};

static const struct attribute_group sde_connector_attr_group = {
	.attrs = sde_connector_dev_attrs,
};
static const struct attribute_group *sde_connector_attr_groups[] = {
	&sde_connector_attr_group,
	NULL,
};

int sde_connector_post_init(struct drm_device *dev, struct drm_connector *conn)
{
	struct sde_connector *c_conn;
	int rc = 0;

	if (!dev || !dev->primary || !dev->primary->kdev || !conn) {
		SDE_ERROR("invalid input param(s)\n");
		rc = -EINVAL;
		return rc;
	}

	c_conn =  to_sde_connector(conn);

	if (conn->connector_type != DRM_MODE_CONNECTOR_DSI)
		return rc;

	c_conn->sysfs_dev =
		device_create_with_groups(dev->primary->kdev->class,
			dev->primary->kdev, 0, conn, sde_connector_attr_groups,
			"sde-conn-%d-%s", conn->index, conn->name);
	if (IS_ERR_OR_NULL(c_conn->sysfs_dev)) {
		SDE_ERROR("connector:%u sysfs create failed rc:%ld\n",
			c_conn->base.index, PTR_ERR(c_conn->sysfs_dev));
		if (!c_conn->sysfs_dev)
			rc = -EINVAL;
		else
			rc = PTR_ERR(c_conn->sysfs_dev);
	}

	return rc;
}

struct drm_connector *sde_connector_init(struct drm_device *dev,
		struct drm_encoder *encoder,
		struct drm_panel *panel,
		void *display,
		const struct sde_connector_ops *ops,
		int connector_poll,
		int connector_type, bool shared)
{
	struct msm_drm_private *priv;
	struct sde_kms *sde_kms;
	struct sde_connector *c_conn = NULL;
	struct msm_display_info display_info;
	int rc;

	if (!dev || !dev->dev_private || !encoder) {
		SDE_ERROR("invalid argument(s), dev %pK, enc %pK\n",
				dev, encoder);
		return ERR_PTR(-EINVAL);
	}

	priv = dev->dev_private;
	if (!priv->kms) {
		SDE_ERROR("invalid kms reference\n");
		return ERR_PTR(-EINVAL);
	}

	c_conn = kzalloc(sizeof(*c_conn), GFP_KERNEL);
	if (!c_conn) {
		SDE_ERROR("failed to alloc sde connector\n");
		return ERR_PTR(-ENOMEM);
	}

	memset(&display_info, 0, sizeof(display_info));

	rc = drm_connector_init(dev,
			&c_conn->base,
			&sde_connector_ops,
			connector_type);
	if (rc)
		goto error_free_conn;

	spin_lock_init(&c_conn->event_lock);

	c_conn->shared = shared;
	c_conn->panel = panel;
	c_conn->connector_type = connector_type;
	c_conn->encoder = encoder;
	c_conn->display = display;

	c_conn->dpms_mode = DRM_MODE_DPMS_ON;
	c_conn->lp_mode = 0;
	c_conn->last_panel_power_mode = SDE_MODE_DPMS_ON;
	c_conn->twm_en = false;

	sde_kms = to_sde_kms(priv->kms);
	if (sde_kms->vbif[VBIF_NRT]) {
		c_conn->aspace[SDE_IOMMU_DOMAIN_UNSECURE] =
			sde_kms->aspace[MSM_SMMU_DOMAIN_NRT_UNSECURE];
		c_conn->aspace[SDE_IOMMU_DOMAIN_SECURE] =
			sde_kms->aspace[MSM_SMMU_DOMAIN_NRT_SECURE];
	} else {
		c_conn->aspace[SDE_IOMMU_DOMAIN_UNSECURE] =
			sde_kms->aspace[MSM_SMMU_DOMAIN_UNSECURE];
		c_conn->aspace[SDE_IOMMU_DOMAIN_SECURE] =
			sde_kms->aspace[MSM_SMMU_DOMAIN_SECURE];
	}

	if (ops)
		c_conn->ops = *ops;

	if (ops && ops->atomic_best_encoder && ops->atomic_check)
		c_conn->base.helper_private = &sde_connector_helper_ops_v2;
	else
		c_conn->base.helper_private = &sde_connector_helper_ops;

	c_conn->base.polled = connector_poll;
	c_conn->base.interlace_allowed = 0;
	c_conn->base.doublescan_allowed = 0;

	snprintf(c_conn->name,
			SDE_CONNECTOR_NAME_SIZE,
			"conn%u",
			c_conn->base.base.id);

	c_conn->retire_fence = sde_fence_init(c_conn->name,
			c_conn->base.base.id);
	if (IS_ERR(c_conn->retire_fence)) {
		rc = PTR_ERR(c_conn->retire_fence);
		SDE_ERROR("failed to init fence, %d\n", rc);
		goto error_cleanup_conn;
	}

	mutex_init(&c_conn->lock);
	mutex_init(&c_conn->bl_vrr.bl_lock);

	rc = drm_connector_attach_encoder(&c_conn->base, encoder);
	if (rc) {
		SDE_ERROR("failed to attach encoder to connector, %d\n", rc);
		goto error_cleanup_fence;
	}

	rc = sde_backlight_setup(c_conn, dev);
	if (rc) {
		SDE_ERROR("failed to setup backlight, rc=%d\n", rc);
		goto error_cleanup_fence;
	}

	/* create properties */
	msm_property_init(&c_conn->property_info, &c_conn->base.base, dev,
			priv->conn_property, c_conn->property_data,
			CONNECTOR_PROP_COUNT, CONNECTOR_PROP_BLOBCOUNT,
			sizeof(struct sde_connector_state));

	if (c_conn->ops.post_init) {
		rc = c_conn->ops.post_init(&c_conn->base, display);
		if (rc) {
			SDE_ERROR("post-init failed, %d\n", rc);
			goto error_cleanup_fence;
		}
	}

	rc = sde_connector_get_info(&c_conn->base, &display_info);
	if (!rc && (connector_type == DRM_MODE_CONNECTOR_DSI) &&
			(display_info.capabilities & MSM_DISPLAY_CAP_VID_MODE))
		sde_connector_register_event(&c_conn->base,
			SDE_CONN_EVENT_VID_FIFO_OVERFLOW,
			sde_connector_handle_disp_recovery,
			c_conn);

	rc = _sde_connector_install_properties(dev, sde_kms, c_conn,
		connector_type, display, &display_info);
	if (rc)
		goto error_cleanup_fence;

	if (connector_type == DRM_MODE_CONNECTOR_DSI &&
			test_bit(SDE_FEATURE_DEMURA, sde_kms->catalog->features)) {
		rc = sde_connector_register_event(&c_conn->base,
			SDE_CONN_EVENT_PANEL_ID,
			sde_connector_handle_panel_id, c_conn);
		if (rc)
			SDE_ERROR("register panel id event err %d\n", rc);
	}

	rc = msm_property_install_get_status(&c_conn->property_info);
	if (rc) {
		SDE_ERROR("failed to create one or more properties\n");
		goto error_destroy_property;
	}

	_sde_connector_lm_preference(c_conn, sde_kms,
			display_info.display_type);

	_sde_connector_init_hw_fence(c_conn, &display_info, sde_kms);

	SDE_DEBUG("connector %d attach encoder %d, wb hwfences:%d\n",
			DRMID(&c_conn->base), DRMID(encoder),
			c_conn->hwfence_wb_retire_fences_enable);

	INIT_DELAYED_WORK(&c_conn->status_work,
			sde_connector_check_status_work);

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	INIT_DELAYED_WORK(&c_conn->dyn_clk_work,
			sde_connector_dyn_clk_work);
#endif

	return &c_conn->base;

error_destroy_property:
	if (c_conn->blob_caps)
		drm_property_blob_put(c_conn->blob_caps);
	if (c_conn->blob_hdr)
		drm_property_blob_put(c_conn->blob_hdr);
	if (c_conn->blob_dither)
		drm_property_blob_put(c_conn->blob_dither);
	if (c_conn->blob_mode_info)
		drm_property_blob_put(c_conn->blob_mode_info);
	if (c_conn->blob_ext_hdr)
		drm_property_blob_put(c_conn->blob_ext_hdr);

	msm_property_destroy(&c_conn->property_info);
error_cleanup_fence:
	mutex_destroy(&c_conn->lock);
	sde_fence_deinit(c_conn->retire_fence);
error_cleanup_conn:
	drm_connector_cleanup(&c_conn->base);
error_free_conn:
	kfree(c_conn);

	return ERR_PTR(rc);
}

static int _sde_conn_enable_hw_recovery(struct drm_connector *connector)
{
	struct sde_connector *c_conn;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}
	c_conn = to_sde_connector(connector);

	if (c_conn->encoder)
		sde_encoder_enable_recovery_event(c_conn->encoder);

	return 0;
}

int sde_connector_register_custom_event(struct sde_kms *kms,
		struct drm_connector *conn_drm, u32 event, bool val)
{
	int ret = -EINVAL;
	struct sde_connector *c_conn;

	switch (event) {
	case DRM_EVENT_SYS_BACKLIGHT:
		ret = 0;
		break;
	case DRM_EVENT_DIMMING_BL:
		if (!conn_drm) {
			SDE_ERROR("invalid connector\n");
			return -EINVAL;
		}
		c_conn = to_sde_connector(conn_drm);
		c_conn->dimming_bl_notify_enabled = val;
		ret = 0;
		break;
	case DRM_EVENT_MISR_SIGN:
		if (!conn_drm) {
			SDE_ERROR("invalid connector\n");
			return -EINVAL;
		}
		c_conn = to_sde_connector(conn_drm);
		c_conn->misr_event_notify_enabled = val;
		ret = sde_encoder_register_misr_event(c_conn->encoder, val);
		break;
	case DRM_EVENT_PANEL_DEAD:
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	case DRM_EVENT_DYN_CLK:
#endif
		ret = 0;
		break;
	case DRM_EVENT_SDE_HW_RECOVERY:
		ret = _sde_conn_enable_hw_recovery(conn_drm);
		if (SDE_DBG_DEFAULT_DUMP_MODE != SDE_DBG_DUMP_IN_LOG_LIMITED)
			sde_dbg_update_dump_mode(val);
		break;
	default:
		break;
	}
	return ret;
}

int sde_connector_event_notify(struct drm_connector *connector, uint32_t type,
		uint32_t len, uint32_t val)
{
	struct drm_event event;
	int ret;

	if (!connector) {
		SDE_ERROR("invalid connector\n");
		return -EINVAL;
	}

	switch (type) {
	case DRM_EVENT_SYS_BACKLIGHT:
	case DRM_EVENT_DIMMING_BL:
	case DRM_EVENT_PANEL_DEAD:
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	case DRM_EVENT_DYN_CLK:
#endif
	case DRM_EVENT_SDE_HW_RECOVERY:
	case DRM_EVENT_MISR_SIGN:
		ret = 0;
		break;
	default:
		SDE_ERROR("connector %d, Unsupported event %d\n",
				connector->base.id, type);
		return -EINVAL;
	}

	event.type = type;
	event.length = len;
	msm_mode_object_event_notify(&connector->base, connector->dev, &event,
			(u8 *)&val);

	SDE_EVT32(connector->base.id, type, len, val);
	SDE_DEBUG("connector:%d hw recovery event(%d) value (%d) notified\n",
			connector->base.id, type, val);

	return ret;
}
