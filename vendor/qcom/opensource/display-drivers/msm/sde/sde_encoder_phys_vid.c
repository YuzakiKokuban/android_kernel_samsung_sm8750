// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__
#include "sde_encoder_phys.h"
#include "sde_hw_interrupts.h"
#include "sde_core_irq.h"
#include "sde_formats.h"
#include "dsi_display.h"
#include "sde_trace.h"
#include "msm_drv.h"
#include <drm/drm_fixed.h>
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
#include "ss_dsi_panel_common.h"
#endif

#define SDE_DEBUG_VIDENC(e, fmt, ...) SDE_DEBUG("enc%d intf%d " fmt, \
		(e) && (e)->base.parent ? \
		(e)->base.parent->base.id : -1, \
		(e) && (e)->base.hw_intf ? \
		(e)->base.hw_intf->idx - INTF_0 : -1, ##__VA_ARGS__)

#define SDE_ERROR_VIDENC(e, fmt, ...) SDE_ERROR("enc%d intf%d " fmt, \
		(e) && (e)->base.parent ? \
		(e)->base.parent->base.id : -1, \
		(e) && (e)->base.hw_intf ? \
		(e)->base.hw_intf->idx - INTF_0 : -1, ##__VA_ARGS__)

#define to_sde_encoder_phys_vid(x) \
	container_of(x, struct sde_encoder_phys_vid, base)

/* Poll time to do recovery during active region */
#define POLL_TIME_USEC_FOR_LN_CNT 500
#define MAX_POLL_CNT 10

static bool sde_encoder_phys_vid_is_master(
		struct sde_encoder_phys *phys_enc)
{
	bool ret = false;

	if (phys_enc->split_role != ENC_ROLE_SLAVE)
		ret = true;

	return ret;
}

static void drm_mode_to_intf_timing_params(
		const struct sde_encoder_phys_vid *vid_enc,
		const struct drm_display_mode *mode,
		struct intf_timing_params *timing)
{
	const struct sde_encoder_phys *phys_enc = &vid_enc->base;
	s64 comp_ratio, width;

	memset(timing, 0, sizeof(*timing));

	if ((mode->htotal < mode->hsync_end)
			|| (mode->hsync_start < mode->hdisplay)
			|| (mode->vtotal < mode->vsync_end)
			|| (mode->vsync_start < mode->vdisplay)
			|| (mode->hsync_end < mode->hsync_start)
			|| (mode->vsync_end < mode->vsync_start)) {
		SDE_ERROR(
		    "invalid params - hstart:%d,hend:%d,htot:%d,hdisplay:%d\n",
				mode->hsync_start, mode->hsync_end,
				mode->htotal, mode->hdisplay);
		SDE_ERROR("vstart:%d,vend:%d,vtot:%d,vdisplay:%d\n",
				mode->vsync_start, mode->vsync_end,
				mode->vtotal, mode->vdisplay);
		return;
	}

	/*
	 * https://www.kernel.org/doc/htmldocs/drm/ch02s05.html
	 *  Active Region      Front Porch   Sync   Back Porch
	 * <-----------------><------------><-----><----------->
	 * <- [hv]display --->
	 * <--------- [hv]sync_start ------>
	 * <----------------- [hv]sync_end ------->
	 * <---------------------------- [hv]total ------------->
	 */
	timing->poms_align_vsync = phys_enc->poms_align_vsync;
	timing->width = mode->hdisplay;	/* active width */
	timing->height = mode->vdisplay;	/* active height */
	timing->xres = timing->width;
	timing->yres = timing->height;
	timing->h_back_porch = mode->htotal - mode->hsync_end;
	timing->h_front_porch = mode->hsync_start - mode->hdisplay;
	timing->v_back_porch = mode->vtotal - mode->vsync_end;
	timing->v_front_porch = mode->vsync_start - mode->vdisplay;
	timing->hsync_pulse_width = mode->hsync_end - mode->hsync_start;
	timing->vsync_pulse_width = mode->vsync_end - mode->vsync_start;
	timing->hsync_polarity = (mode->flags & DRM_MODE_FLAG_NHSYNC) ? 1 : 0;
	timing->vsync_polarity = (mode->flags & DRM_MODE_FLAG_NVSYNC) ? 1 : 0;
	timing->border_clr = 0;
	timing->underflow_clr = 0xff;
	timing->hsync_skew = mode->hskew;
	timing->v_front_porch_fixed = vid_enc->base.vfp_cached;
	timing->vrefresh = drm_mode_vrefresh(&phys_enc->cached_mode);

	if (vid_enc->base.comp_type != MSM_DISPLAY_COMPRESSION_NONE) {
		timing->compression_en = true;
		timing->dce_bytes_per_line = vid_enc->base.dce_bytes_per_line;
	}

	/* DSI controller cannot handle active-low sync signals. */
	if (phys_enc->hw_intf->cap->type == INTF_DSI) {
		timing->hsync_polarity = 0;
		timing->vsync_polarity = 0;
	}

	/* for DP/EDP, Shift timings to align it to bottom right */
	if ((phys_enc->hw_intf->cap->type == INTF_DP) ||
		(phys_enc->hw_intf->cap->type == INTF_EDP)) {
		timing->h_back_porch += timing->h_front_porch;
		timing->h_front_porch = 0;
		timing->v_back_porch += timing->v_front_porch;
		timing->v_front_porch = 0;
	}

	timing->wide_bus_en = sde_encoder_is_widebus_enabled(phys_enc->parent);
	timing->pclk_factor = sde_encoder_get_pclk_factor(phys_enc->parent);

	/*
	 * for DP, divide the horizonal parameters by 2 when
	 * widebus or compression is enabled, irrespective of
	 * compression ratio
	 */
	if (phys_enc->hw_intf->cap->type == INTF_DP &&
			(timing->wide_bus_en ||
			(vid_enc->base.comp_ratio > MSM_DISPLAY_COMPRESSION_RATIO_NONE))) {
		timing->width = timing->width / (timing->pclk_factor);
		timing->xres = timing->xres / (timing->pclk_factor);
		timing->h_back_porch = timing->h_back_porch / (timing->pclk_factor);
		timing->h_front_porch = timing->h_front_porch / (timing->pclk_factor);
		timing->hsync_pulse_width = timing->hsync_pulse_width / (timing->pclk_factor);

		if (vid_enc->base.comp_type == MSM_DISPLAY_COMPRESSION_DSC &&
				(vid_enc->base.comp_ratio > MSM_DISPLAY_COMPRESSION_RATIO_NONE)) {
			timing->extra_dto_cycles =
				vid_enc->base.dsc_extra_pclk_cycle_cnt;
			timing->width += vid_enc->base.dsc_extra_disp_width;
			timing->h_back_porch +=
				vid_enc->base.dsc_extra_disp_width;
		}
	}

	/*
	 * for DSI, if compression is enabled, then divide the horizonal active
	 * timing parameters by compression ratio.
	 */
	if ((phys_enc->hw_intf->cap->type != INTF_DP) &&
			((vid_enc->base.comp_type ==
			MSM_DISPLAY_COMPRESSION_DSC) ||
			(vid_enc->base.comp_type ==
			MSM_DISPLAY_COMPRESSION_VDC))) {
		// adjust active dimensions
		width = drm_fixp_from_fraction(timing->width, 1);
		comp_ratio = drm_fixp_from_fraction(vid_enc->base.comp_ratio, 100);
		width = drm_fixp_div(width, comp_ratio);
		timing->width = drm_fixp2int_ceil(width);
		timing->xres = timing->width;
	}

	/*
	 * For edp only:
	 * DISPLAY_V_START = (VBP * HCYCLE) + HBP
	 * DISPLAY_V_END = (VBP + VACTIVE) * HCYCLE - 1 - HFP
	 */
	/*
	 * if (vid_enc->hw->cap->type == INTF_EDP) {
	 * display_v_start += mode->htotal - mode->hsync_start;
	 * display_v_end -= mode->hsync_start - mode->hdisplay;
	 * }
	 */
}

static inline u32 get_horizontal_total(const struct intf_timing_params *timing)
{
	u32 active = timing->xres;
	u32 inactive =
	    timing->h_back_porch + timing->h_front_porch +
	    timing->hsync_pulse_width;
	return active + inactive;
}

static inline u32 get_vertical_total(const struct intf_timing_params *timing)
{
	u32 active = timing->yres;
	u32 inactive = timing->v_back_porch + timing->v_front_porch +
			    timing->vsync_pulse_width;
	return active + inactive;
}

/*
 * programmable_fetch_get_num_lines:
 *	Number of fetch lines in vertical front porch
 * @timing: Pointer to the intf timing information for the requested mode
 *
 * Returns the number of fetch lines in vertical front porch at which mdp
 * can start fetching the next frame.
 *
 * Number of needed prefetch lines is anything that cannot be absorbed in the
 * start of frame time (back porch + vsync pulse width).
 *
 * Some panels have very large VFP, however we only need a total number of
 * lines based on the chip worst case latencies.
 */
static u32 programmable_fetch_get_num_lines(
		struct sde_encoder_phys_vid *vid_enc,
		const struct intf_timing_params *timing)
{
	struct sde_encoder_phys *phys_enc = &vid_enc->base;
	struct sde_mdss_cfg *m;
	struct sde_connector *sde_conn = to_sde_connector(phys_enc->connector);

	u32 needed_prefill_lines, needed_vfp_lines, actual_vfp_lines;
	const u32 fixed_prefill_fps = DEFAULT_FPS;
	u32 default_prefill_lines =
		phys_enc->hw_intf->cap->prog_fetch_lines_worst_case;
	u32 start_of_frame_lines =
	    timing->v_back_porch + timing->vsync_pulse_width;
	u32 v_front_porch = timing->v_front_porch;
	u32 allowed_vfp = v_front_porch;
	u32 vrefresh, max_fps;

	/*
	 * In case of level TE, MDP will use prog fetch start as the decision point
	 * for checking TE level and determining whether to send out a frame. This
	 * extra line gives time for TE to come up after active.
	 */
	if (sde_conn->vrr_caps.video_psr_support)
		allowed_vfp = v_front_porch - 2;

	m = phys_enc->sde_kms->catalog;
	max_fps = sde_encoder_get_dfps_maxfps(phys_enc->parent);
	vrefresh = (max_fps > timing->vrefresh) ? max_fps : timing->vrefresh;

	/* minimum prefill lines are defined based on 60fps */
	needed_prefill_lines = (vrefresh > fixed_prefill_fps) ?
		((default_prefill_lines * vrefresh) /
			fixed_prefill_fps) : default_prefill_lines;
	needed_vfp_lines = needed_prefill_lines - start_of_frame_lines;

	/* Fetch must be outside active lines, otherwise undefined. */
	if (start_of_frame_lines >= needed_prefill_lines) {
		SDE_DEBUG_VIDENC(vid_enc,
				"prog fetch always enabled case\n");
		actual_vfp_lines = (test_bit(SDE_FEATURE_DELAY_PRG_FETCH, m->features)) ? 2 : 1;
	} else if (allowed_vfp < needed_vfp_lines) {
		/* Warn fetch needed, but not enough porch in panel config */
		pr_warn_once
			("low vbp+vfp may lead to perf issues in some cases\n");
		SDE_DEBUG_VIDENC(vid_enc,
				"less vfp than fetch req, using entire vfp\n");
		actual_vfp_lines = allowed_vfp;
	} else {
		SDE_DEBUG_VIDENC(vid_enc, "room in vfp for needed prefetch\n");
		actual_vfp_lines = needed_vfp_lines;
	}

	SDE_DEBUG_VIDENC(vid_enc,
		"vrefresh:%u v_front_porch:%u v_back_porch:%u vsync_pulse_width:%u\n",
		vrefresh, v_front_porch, timing->v_back_porch,
		timing->vsync_pulse_width);
	SDE_DEBUG_VIDENC(vid_enc,
		"prefill_lines:%u needed_vfp_lines:%u actual_vfp_lines:%u allowed_vfp:%u\n",
		needed_prefill_lines, needed_vfp_lines, actual_vfp_lines, allowed_vfp);

	return actual_vfp_lines;
}

/*
 * programmable_fetch_config: Programs HW to prefetch lines by offsetting
 *	the start of fetch into the vertical front porch for cases where the
 *	vsync pulse width and vertical back porch time is insufficient
 *
 *	Gets # of lines to pre-fetch, then calculate VSYNC counter value.
 *	HW layer requires VSYNC counter of first pixel of tgt VFP line.
 *
 * @timing: Pointer to the intf timing information for the requested mode
 */
static void programmable_fetch_config(struct sde_encoder_phys *phys_enc,
				      const struct intf_timing_params *timing)
{
	struct sde_encoder_phys_vid *vid_enc =
		to_sde_encoder_phys_vid(phys_enc);
	struct intf_prog_fetch f = { 0 };
	u32 vfp_fetch_lines = 0;
	u32 horiz_total = 0;
	u32 vert_total = 0;
	u32 vfp_fetch_start_vsync_counter = 0;
	unsigned long lock_flags;
	struct sde_mdss_cfg *m;

	if (WARN_ON_ONCE(!phys_enc->hw_intf->ops.setup_prg_fetch))
		return;

	m = phys_enc->sde_kms->catalog;

	phys_enc->pf_time_in_us = 0;
	vfp_fetch_lines = programmable_fetch_get_num_lines(vid_enc, timing);
	if (vfp_fetch_lines) {
		vert_total = get_vertical_total(timing);
		horiz_total = get_horizontal_total(timing);
		vfp_fetch_start_vsync_counter =
			(vert_total - vfp_fetch_lines) * horiz_total + 1;

		phys_enc->pf_time_in_us = DIV_ROUND_UP(1000000 * vfp_fetch_lines,
				vert_total * timing->vrefresh);

		/**
		 * Check if we need to throttle the fetch to start
		 * from second line after the active region.
		 */
		if (test_bit(SDE_FEATURE_DELAY_PRG_FETCH, m->features))
			vfp_fetch_start_vsync_counter += horiz_total;

		f.enable = 1;
		f.fetch_start = vfp_fetch_start_vsync_counter;
	}

	SDE_DEBUG_VIDENC(vid_enc,
		"vfp_fetch_lines %u vfp_fetch_start_vsync_counter %u\n",
		vfp_fetch_lines, vfp_fetch_start_vsync_counter);

	spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
	phys_enc->hw_intf->ops.setup_prg_fetch(phys_enc->hw_intf, &f);
	phys_enc->hw_intf->ops.setup_prog_dynref(phys_enc->hw_intf, vert_total - vfp_fetch_lines);
	spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

	/*
	 * In Dual DPU sync mode, prog_intf_offset set in Master DPU
	 * to enable Slave DPU timing engine.
	 */
	if (sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent) &&
		sde_encoder_phys_has_role_master_dpu_master_intf(phys_enc) &&
		phys_enc->hw_intf->ops.setup_dpu_sync_prog_intf_offset)
		phys_enc->hw_intf->ops.setup_dpu_sync_prog_intf_offset(phys_enc->hw_intf, &f);
}

static bool sde_encoder_phys_vid_mode_fixup(
		struct sde_encoder_phys *phys_enc,
		const struct drm_display_mode *mode,
		struct drm_display_mode *adj_mode)
{
	if (phys_enc)
		SDE_DEBUG_VIDENC(to_sde_encoder_phys_vid(phys_enc), "\n");

	/*
	 * Modifying mode has consequences when the mode comes back to us
	 */
	return true;
}
static void _sde_encoder_phys_vid_raw_te_setup(
		struct sde_encoder_phys *phys_enc, bool enable)
{
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);

	vid_enc = to_sde_encoder_phys_vid(phys_enc);

	if (phys_enc->sde_kms->catalog->is_vrr_hw_fence_enable)
		phys_enc->hw_ctl->ops.hw_fence_ctrl(phys_enc->hw_ctl, true, true, 1, true,
			sde_enc->disp_info.vrr_caps.arp_support);
	if (vid_enc->base.hw_intf->ops.raw_te_setup &&
		sde_enc->disp_info.vrr_caps.arp_support)
		vid_enc->base.hw_intf->ops.raw_te_setup(vid_enc->base.hw_intf, enable);

}

/* vid_enc timing_params must be configured before calling this function */
static void _sde_encoder_phys_vid_setup_avr(
		struct sde_encoder_phys *phys_enc, u32 qsync_min_fps)
{
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_connector *sde_conn;

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	if (vid_enc->base.hw_intf->ops.avr_setup) {
		struct intf_avr_params avr_params = {0};
		u32 default_fps = drm_mode_vrefresh(&phys_enc->cached_mode);
		int ret;
		u32 dpu_min_fps = 0;

		if (!default_fps) {
			SDE_ERROR_VIDENC(vid_enc,
					"invalid default fps %d\n",
					default_fps);
			return;
		}
		sde_conn = to_sde_connector(phys_enc->connector);
		if (sde_conn && sde_conn->vrr_caps.video_psr_support) {
			if (sde_conn->freq_pattern &&
				sde_conn->freq_pattern->freq_stepping_seq)
				dpu_min_fps = sde_conn->freq_pattern->freq_stepping_seq[0];
			/* set qsync min fps to dpu_min_fps only in sticky-on-fly case */
			if (dpu_min_fps && ((sde_conn->frame_interval >= dpu_min_fps)
					|| (sde_conn->freq_pattern &&
				sde_conn->freq_pattern->needs_ap_refresh)))
				qsync_min_fps = dpu_min_fps / 1000;

			avr_params.infinite_mode = true;
		}
		if (sde_conn && sde_conn->freq_pattern)
			SDE_EVT32(dpu_min_fps,sde_conn->frame_interval, dpu_min_fps, sde_conn->freq_pattern->needs_ap_refresh);
		if (qsync_min_fps > default_fps) {
			SDE_ERROR_VIDENC(vid_enc,
				"qsync fps %d must be less than default %d\n",
				qsync_min_fps, default_fps);
			return;
		}

		avr_params.default_fps = default_fps;
		avr_params.min_fps = qsync_min_fps;

		ret = vid_enc->base.hw_intf->ops.avr_setup(
				vid_enc->base.hw_intf,
				&vid_enc->timing_params, &avr_params);
		if (ret)
			SDE_ERROR_VIDENC(vid_enc,
				"bad settings, can't configure AVR\n");

		SDE_DEBUG_VIDENC(vid_enc,
				"configure AVR, default_fps =%d,qsync_min_fps=%d\n",
				default_fps, qsync_min_fps);

		SDE_EVT32(DRMID(phys_enc->parent), default_fps,
				qsync_min_fps, ret);
	}
}

static void _sde_encoder_phys_vid_set_num_avr_step(struct sde_encoder_phys *phys_enc)
{
	struct drm_connector *conn = phys_enc->connector;
	struct sde_encoder_phys_vid *vid_enc = to_sde_encoder_phys_vid(phys_enc);
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct msm_mode_info *info = &sde_enc->mode_info;
	struct sde_hw_intf *intf = vid_enc->base.hw_intf;
	unsigned long lock_flags;
	u64 ept, delta_ts = 0, avr_step_time = 0;
	u32 num_avr_step = 0, cur_avr_step = 0;
	ktime_t current_ts, ept_ts;

	spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
	current_ts = ktime_get_ns();
	ept = sde_connector_get_property(conn->state, CONNECTOR_PROP_EPT);
	if (!ept || !info->avr_step_fps) {
		spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);
		return;
	}

	ept_ts = ktime_set(0, ept);

	if (ktime_after(ept_ts, current_ts)) {
		delta_ts = ept_ts - current_ts;
		avr_step_time = DIV_ROUND_UP(NSEC_PER_SEC, info->avr_step_fps);
		num_avr_step = DIV_ROUND_UP(delta_ts, avr_step_time);
	}
	spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

	if (intf->ops.get_cur_num_avr_step)
		cur_avr_step = intf->ops.get_cur_num_avr_step(intf);

	num_avr_step += cur_avr_step;

	if (intf->ops.set_num_avr_step)
		intf->ops.set_num_avr_step(intf, num_avr_step);

	SDE_EVT32(DRMID(phys_enc->parent), ktime_to_us(ept_ts), ktime_to_us(current_ts),
			ktime_to_us(delta_ts), info->avr_step_fps, cur_avr_step, num_avr_step);
}

static void _sde_encoder_phys_flush_snapshot_setup(struct sde_encoder_phys *phys_enc, bool enable)
{
	struct sde_mdss_cfg *m;
	struct intf_timing_params *timing;
	struct sde_encoder_phys_vid *vid_enc;
	u32 snapshot_val = 0, vfp_fetch_lines = 0, snapshot_lines = 0;
	u32 vtotal = 0, htotal = 0;

	m = phys_enc->sde_kms->catalog;
	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	timing = &vid_enc->timing_params;

	if (!phys_enc->hw_intf->ops.setup_flush_snapshot)
		return;

	if (phys_enc->hw_intf->cap->type == INTF_DSI) {
		vfp_fetch_lines = programmable_fetch_get_num_lines(vid_enc, timing);
		if (vfp_fetch_lines && test_bit(SDE_FEATURE_DELAY_PRG_FETCH, m->features))
			vfp_fetch_lines = vfp_fetch_lines - 1;
	}

	vtotal = get_vertical_total(timing);
	htotal = get_horizontal_total(timing);
	snapshot_lines = vtotal - vfp_fetch_lines - phys_enc->hw_intf->cap->hw_flush_sync_val;

	if (snapshot_lines <= 0) {
		SDE_DEBUG("flush snapshot should be set before mdp vsync\n");
		phys_enc->hw_intf->ops.setup_flush_snapshot(phys_enc->hw_intf,
					snapshot_val, false);
	}

	snapshot_val = snapshot_lines * htotal;
	phys_enc->hw_intf->ops.setup_flush_snapshot(phys_enc->hw_intf, snapshot_val, enable);
}

static void _sde_encoder_phys_vid_avr_ctrl(struct sde_encoder_phys *phys_enc)
{
	struct intf_avr_params avr_params;
	struct sde_encoder_phys_vid *vid_enc = to_sde_encoder_phys_vid(phys_enc);
	struct drm_connector *conn = phys_enc->connector;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct msm_mode_info *info = &sde_enc->mode_info;
	struct sde_hw_intf *intf = vid_enc->base.hw_intf;
	u32 avr_step_state;

	if (!conn || !conn->state)
		return;

	avr_step_state = sde_connector_get_property(conn->state, CONNECTOR_PROP_AVR_STEP_STATE);

	memset(&avr_params, 0, sizeof(avr_params));
	avr_params.avr_mode = sde_connector_get_qsync_mode(phys_enc->connector);

	if (sde_enc->disp_info.vrr_caps.arp_support ||
		sde_enc->disp_info.vrr_caps.video_psr_support) {
		avr_step_state = AVR_STEP_ENABLE;
		if (sde_enc->disp_info.vrr_caps.video_psr_support)
			avr_params.avr_mode = SDE_RM_QSYNC_CONTINUOUS_MODE;
		if (avr_params.avr_mode)
			_sde_encoder_phys_vid_raw_te_setup(phys_enc, true);
		else
			_sde_encoder_phys_vid_raw_te_setup(phys_enc, false);
	}

	if (sde_enc->disp_info.vrr_caps.video_psr_support &&
			phys_enc->sde_kms->catalog->is_vrr_hw_fence_enable)
		avr_params.hw_avr_trigger = true;

	if (info->avr_step_fps && (avr_step_state == AVR_STEP_ENABLE))
		avr_params.avr_step_lines = mult_frac(phys_enc->cached_mode.vtotal,
				vid_enc->timing_params.vrefresh, info->avr_step_fps);

	if (intf->ops.avr_ctrl)
		intf->ops.avr_ctrl(intf, &avr_params);

	if (intf->ops.enable_te_level_trigger &&
			!sde_enc->disp_info.is_te_using_watchdog_timer)
		intf->ops.enable_te_level_trigger(intf,
				(avr_step_state == AVR_STEP_ENABLE));

	if (test_bit(SDE_INTF_NUM_AVR_STEP, &intf->cap->features))
		_sde_encoder_phys_vid_set_num_avr_step(phys_enc);

	SDE_EVT32(DRMID(phys_enc->parent), phys_enc->hw_intf->idx - INTF_0, avr_params.avr_mode,
			avr_params.avr_step_lines, info->avr_step_fps, avr_step_state,
			sde_enc->disp_info.is_te_using_watchdog_timer);
}

void _sde_encoder_phys_vid_setup_panic_ctrl(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct sde_encoder_phys_vid *vid_enc = to_sde_encoder_phys_vid(phys_enc);
	struct msm_mode_info *info = &sde_enc->mode_info;
	struct intf_timing_params *timing = &vid_enc->timing_params;
	struct intf_panic_ctrl_cfg cfg = {0, };
	struct intf_status intf_status = {0};
	bool qsync_en = sde_connector_get_qsync_mode(phys_enc->connector);
	u32 bw_update_time_lines, avr_cutoff, fps, min_fps;
	u32 vsync_slow_period, prefill_lines;

	fps = vid_enc->timing_params.vrefresh;
	min_fps = info->qsync_min_fps;

	cfg.enable = true;
	/* set based on fast fps vfp */
	cfg.ext_vfp_start = phys_enc->cached_mode.vdisplay + phys_enc->vfp_cached
				+ timing->v_back_porch + timing->vsync_pulse_width;

	if (phys_enc->hw_intf && phys_enc->hw_intf->ops.get_status)
		phys_enc->hw_intf->ops.get_status(phys_enc->hw_intf, &intf_status);

	bw_update_time_lines = sde_encoder_helper_get_bw_update_time_lines(sde_enc);
	avr_cutoff = intf_status.is_prog_fetch_en ? 1 : 3;

	if ((phys_enc->hw_intf->cap->type == INTF_DSI) && intf_status.is_prog_fetch_en) {
		phys_enc->prog_fetch_start = programmable_fetch_get_num_lines(vid_enc, timing);
		prefill_lines = phys_enc->prog_fetch_start;
	} else {
		phys_enc->prog_fetch_start = 0;
		prefill_lines = info->prefill_lines;
	}

	/* panic level = vsync_period_slow - prog_fetch_start - bw-vote - AVR cutoff */
	vsync_slow_period = (qsync_en && min_fps) ?
			DIV_ROUND_UP(phys_enc->cached_mode.vtotal * fps, min_fps) :
				phys_enc->cached_mode.vtotal;
	cfg.panic_level = vsync_slow_period - prefill_lines - bw_update_time_lines - avr_cutoff;

	if (cfg.ext_vfp_start >= cfg.panic_level)
		cfg.ext_vfp_start = 0xFFFFFFFF;

	SDE_EVT32(phys_enc->hw_intf->idx - INTF_0, cfg.enable, cfg.ext_vfp_start, cfg.panic_level,
				phys_enc->cached_mode.vdisplay, phys_enc->cached_mode.vtotal, fps,
				min_fps, bw_update_time_lines, prefill_lines,
				phys_enc->prog_fetch_start, avr_cutoff, vsync_slow_period, qsync_en,
				phys_enc->vfp_cached);

	phys_enc->hw_intf->ops.setup_intf_panic_ctrl(phys_enc->hw_intf, &cfg);
}

static void sde_encoder_phys_vid_setup_timing_engine(
		struct sde_encoder_phys *phys_enc, bool from_idle)
{
	struct sde_encoder_phys_vid *vid_enc;
	struct msm_display_info *info;
	struct drm_display_mode mode;
	struct sde_encoder_virt *sde_enc;
	struct intf_timing_params timing_params = { 0 };
	const struct sde_format *fmt = NULL;
	u32 fmt_fourcc = DRM_FORMAT_RGB888;
	u32 qsync_min_fps = 0;
	unsigned long lock_flags;
	struct sde_hw_intf_cfg intf_cfg = { 0 };
	bool is_split_link = false;
	bool avr_enabled = sde_connector_get_qsync_mode(phys_enc->connector);

	if (!phys_enc || !phys_enc->sde_kms || !phys_enc->hw_ctl ||
			!phys_enc->hw_intf || !phys_enc->connector) {
		SDE_ERROR("invalid encoder %d\n", !phys_enc);
		return;
	}

	mode = phys_enc->cached_mode;
	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	info = &sde_enc->disp_info;
	if (!phys_enc->hw_intf->ops.setup_timing_gen) {
		SDE_ERROR("timing engine setup is not supported\n");
		return;
	}

	SDE_DEBUG_VIDENC(vid_enc, "enabling mode:\n");
	drm_mode_debug_printmodeline(&mode);

	is_split_link = phys_enc->hw_intf->cfg.split_link_en;
	if ((phys_enc->split_role != ENC_ROLE_SOLO && !sde_encoder_master_in_solo_mode(phys_enc))
				|| is_split_link) {
		mode.hdisplay >>= 1;
		mode.htotal >>= 1;
		mode.hsync_start >>= 1;
		mode.hsync_end >>= 1;

		SDE_DEBUG_VIDENC(vid_enc,
			"split_role %d, halve horizontal %d %d %d %d\n",
			phys_enc->split_role,
			mode.hdisplay, mode.htotal,
			mode.hsync_start, mode.hsync_end);
	}

	if (!phys_enc->vfp_cached) {
		phys_enc->vfp_cached =
			sde_connector_get_panel_vfp(phys_enc->connector, &mode);
		if (phys_enc->vfp_cached <= 0)
			phys_enc->vfp_cached = mode.vsync_start - mode.vdisplay;
	}

	drm_mode_to_intf_timing_params(vid_enc, &mode, &timing_params);

	vid_enc->timing_params = timing_params;

	if (phys_enc->cont_splash_enabled) {
		SDE_DEBUG_VIDENC(vid_enc,
			"skipping intf programming since cont splash is enabled\n");
		goto exit;
	}

	fmt = sde_get_sde_format(fmt_fourcc);
	SDE_DEBUG_VIDENC(vid_enc, "fmt_fourcc 0x%X\n", fmt_fourcc);

	spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
	phys_enc->hw_intf->ops.setup_timing_gen(phys_enc->hw_intf,
			&timing_params, fmt, info->esync_enabled, avr_enabled);

	if (test_bit(SDE_CTL_ACTIVE_CFG,
				&phys_enc->hw_ctl->caps->features)) {
		sde_encoder_helper_update_intf_cfg(phys_enc);
	} else if (phys_enc->hw_ctl->ops.setup_intf_cfg) {
		intf_cfg.intf = phys_enc->hw_intf->idx;
		intf_cfg.intf_mode_sel = SDE_CTL_MODE_SEL_VID;
		intf_cfg.stream_sel = 0; /* Don't care value for video mode */
		intf_cfg.mode_3d =
			sde_encoder_helper_get_3d_blend_mode(phys_enc);

		phys_enc->hw_ctl->ops.setup_intf_cfg(phys_enc->hw_ctl,
				&intf_cfg);
	}
	spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);
	if (phys_enc->hw_intf->cap->type == INTF_DSI)
		programmable_fetch_config(phys_enc, &timing_params);

	if (sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent) &&
		sde_encoder_phys_has_role_master_dpu_master_intf(phys_enc))
		_sde_encoder_phys_flush_snapshot_setup(phys_enc, true);

exit:
	if (sde_encoder_get_cesta_client(phys_enc->parent)) {
		_sde_encoder_phys_vid_setup_panic_ctrl(phys_enc);

		/* update INTF flush for panic_ctrl to take effect with splash enabled */
		if (phys_enc->cont_splash_enabled && phys_enc->hw_ctl
				&& phys_enc->hw_ctl->ops.update_bitmask)
			phys_enc->hw_ctl->ops.update_bitmask(phys_enc->hw_ctl, SDE_HW_FLUSH_INTF,
					phys_enc->hw_intf->idx, 1);
	}

	if (phys_enc->parent_ops.get_qsync_fps)
		phys_enc->parent_ops.get_qsync_fps(
			phys_enc->parent, &qsync_min_fps, phys_enc->connector->state, NULL);

	/* only panels which support qsync will have a non-zero min fps */
	if (qsync_min_fps) {
		sde_enc->mode_info.qsync_min_fps = qsync_min_fps;
		_sde_encoder_phys_vid_setup_avr(phys_enc, qsync_min_fps);
		_sde_encoder_phys_vid_avr_ctrl(phys_enc);
	}
}

static int sde_encoder_phys_vid_setup_esync_engine(
	struct sde_encoder_phys *phys_enc, bool exit_idle)
{
	struct sde_encoder_phys_vid *vid_enc = to_sde_encoder_phys_vid(phys_enc);
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct msm_display_info *info = &sde_enc->disp_info;
	struct intf_esync_params esync_params = {0};
	u32 emsync_fps = info->esync_emsync_fps ? info->esync_emsync_fps : 1;
	u32 hblank;
	u32 active_compressed;
	u32 hsync_period_cycles;

	if (!info->esync_enabled)
		goto exit;

	if (!phys_enc->hw_intf->ops.prepare_esync) {
		SDE_ERROR("esync enabled but hw functions not defined\n");
		return -EINVAL;
	}

	if (phys_enc->prog_fetch_start <= 0) {
		SDE_ERROR("esync enabled but programmable fetch <= 0\n");
		return -EINVAL;
	}

	hblank = phys_enc->cached_mode.htotal - phys_enc->cached_mode.hdisplay;
	active_compressed = mult_frac(phys_enc->cached_mode.hdisplay, 100, phys_enc->comp_ratio);
	hsync_period_cycles = hblank + active_compressed;

	esync_params.avr_step_lines = mult_frac(phys_enc->cached_mode.vtotal,
			vid_enc->timing_params.vrefresh, info->avr_step_fps);
	esync_params.emsync_pulse_width = mult_frac(info->esync_emsync_milli_pulse_width,
			hsync_period_cycles, 1000);
	esync_params.emsync_period_lines = mult_frac(phys_enc->cached_mode.vtotal,
			vid_enc->timing_params.vrefresh, emsync_fps);
	esync_params.hsync_pulse_width = mult_frac(info->esync_hsync_milli_pulse_width,
			hsync_period_cycles, 1000);
	esync_params.hsync_period_cycles = hsync_period_cycles;
	esync_params.skew = mult_frac(phys_enc->cached_mode.htotal, info->esync_milli_skew, 1000);
	esync_params.prog_fetch_start =
			(phys_enc->cached_mode.vtotal - phys_enc->prog_fetch_start + 1)
			% esync_params.avr_step_lines;
	esync_params.hw_fence_enabled = phys_enc->sde_kms->catalog->is_vrr_hw_fence_enable;
	esync_params.align_backup = exit_idle;

	phys_enc->hw_intf->ops.prepare_esync(phys_enc->hw_intf, &esync_params);

exit:
	return 0;
}

static int sde_encoder_phys_vid_setup_backup_esync_engine(
	struct sde_encoder_phys *phys_enc, bool align)
{
	struct sde_encoder_phys_vid *vid_enc = to_sde_encoder_phys_vid(phys_enc);
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct drm_connector *conn = phys_enc->connector;
	struct msm_display_info *info = &sde_enc->disp_info;
	struct intf_esync_params esync_params = {0};
	struct sde_kms *sde_kms;
	u32 hblank;
	u32 active_compressed;
	u32 hsync_period_cycles_pclk;
	u32 hsync_period_cycles_osc;
	u64 esync_freq;
	u64 osc_freq;
	int rc;

	if (!info->esync_enabled)
		goto exit;

	if (!phys_enc->hw_intf->ops.prepare_backup_esync) {
		SDE_ERROR("esync enabled but hw functions not defined\n");
		return -EINVAL;
	}

	sde_kms = phys_enc->sde_kms;
	if (!sde_kms || !sde_kms->catalog) {
		SDE_ERROR("invalid kms catalog\n");
		return -EINVAL;
	}

	hblank = phys_enc->cached_mode.htotal - phys_enc->cached_mode.hdisplay;
	active_compressed = mult_frac(phys_enc->cached_mode.hdisplay, 100, phys_enc->comp_ratio);
	hsync_period_cycles_pclk = hblank + active_compressed;

	rc = sde_connector_clk_get_rate_esync(conn, phys_enc->intf_idx, &esync_freq);
	if (rc)
		SDE_ERROR("invalid esync clock frequency, rc: %d", rc);

	osc_freq = sde_kms->catalog->osc_clk_rate;

	hsync_period_cycles_osc = mult_frac(hsync_period_cycles_pclk, osc_freq, esync_freq);

	esync_params.avr_step_lines = mult_frac(phys_enc->cached_mode.vtotal,
			vid_enc->timing_params.vrefresh, info->avr_step_fps);
	esync_params.emsync_pulse_width = mult_frac(info->esync_emsync_milli_pulse_width,
			hsync_period_cycles_osc, 1000);
	esync_params.emsync_period_lines = mult_frac(phys_enc->cached_mode.vtotal,
			vid_enc->timing_params.vrefresh, info->esync_emsync_fps);
	esync_params.hsync_pulse_width = mult_frac(info->esync_hsync_milli_pulse_width,
			hsync_period_cycles_osc, 1000);
	esync_params.hsync_period_cycles = hsync_period_cycles_osc;
	esync_params.align_backup = align;

	phys_enc->hw_intf->ops.prepare_backup_esync(phys_enc->hw_intf, &esync_params);

exit:
	return 0;
}

static void sde_encoder_phys_vid_connect_te(
		struct sde_encoder_phys *phys_enc, bool enable)
{
	struct sde_encoder_virt *sde_enc;
	struct msm_display_info *info;

	if (!phys_enc || !phys_enc->hw_intf || !phys_enc->parent)
		return;

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	if (!sde_enc->disp_info.vrr_caps.vrr_support)
		return;

	info = &sde_enc->disp_info;
	if (phys_enc->hw_intf->ops.connect_external_te) {
		phys_enc->hw_intf->ops.connect_external_te(phys_enc->hw_intf, enable);

		if (sde_enc->disp_info.vrr_caps.video_psr_support &&
				info->te_gpio_mmio)
			sde_gpio_toggle(info->te_gpio_mmio);
	}

	SDE_EVT32(DRMID(phys_enc->parent), enable);
}

static void sde_encoder_phys_vid_setup_vsync_source(struct sde_encoder_phys *phys_enc,
		u32 vsync_source, struct msm_display_info *disp_info)
{
	struct sde_encoder_virt *sde_enc;
	struct sde_connector *sde_conn;

	if (!phys_enc || !phys_enc->hw_intf || !phys_enc->parent ||
			!phys_enc->connector)
		return;

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	if (!sde_enc || !sde_enc->disp_info.vrr_caps.vrr_support)
		return;

	sde_conn = to_sde_connector(phys_enc->connector);

	if ((disp_info->is_te_using_watchdog_timer || sde_conn->panel_dead) &&
			phys_enc->hw_intf->ops.setup_vsync_source) {
		vsync_source = SDE_VSYNC_SOURCE_WD_TIMER_0;
		phys_enc->hw_intf->ops.setup_vsync_source(phys_enc->hw_intf,
				sde_enc->mode_info.frame_rate);
	} else {
		sde_encoder_helper_vsync_config(phys_enc, vsync_source);
	}

	if (phys_enc->has_intf_te && phys_enc->hw_intf->ops.vsync_sel)
		phys_enc->hw_intf->ops.vsync_sel(phys_enc->hw_intf,
				vsync_source);
}

static void sde_encoder_restore_state(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_virt *sde_enc;
	struct drm_encoder *drm_enc;
	struct drm_device *ddev;
	struct sde_kms *sde_kms;
	struct drm_modeset_acquire_ctx ctx;
	int ret, i;

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	drm_enc = &sde_enc->base;
	sde_kms = sde_encoder_get_kms(drm_enc);
	ddev = sde_kms->dev;

	if (!ddev || !ddev_to_msm_kms(ddev))
		return;

	SDE_EVT32(SDE_EVTLOG_FUNC_ENTRY);
	if (sde_enc->vrr_info.current_state == NULL) {
		SDE_ERROR("No Stored state to restore\n");
		SDE_EVT32(SDE_EVTLOG_ERROR);
	}

	if (sde_enc->vrr_info.current_state)
		drm_mode_config_reset(ddev);

	drm_modeset_acquire_init(&ctx, 0);
retry:
	ret = drm_modeset_lock_all_ctx(ddev, &ctx);
	if (ret == -EDEADLK) {
		drm_modeset_backoff(&ctx);
		goto retry;
	} else if (WARN_ON(ret)) {
		goto end;
	}

	if (sde_enc->vrr_info.current_state) {
		sde_enc->vrr_info.current_state->acquire_ctx = &ctx;
		for (i = 0; i < TEARDOWN_DEADLOCK_RETRY_MAX; i++) {
			ret = drm_atomic_helper_commit_duplicated_state(
				sde_enc->vrr_info.current_state, &ctx);
			pr_err("duplicate commit return value : %d\n", ret);
			SDE_EVT32(ret);
			if (ret != -EDEADLK)
				break;
			drm_modeset_backoff(&ctx);
		}

		if (ret < 0)
			DRM_ERROR("failed to restore state, %d\n", ret);
		drm_atomic_state_put(sde_enc->vrr_info.current_state);
		sde_enc->vrr_info.current_state = NULL;
	}
end:
	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);
}

static void sde_encoder_store_state(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_virt *sde_enc;
	struct drm_encoder *drm_enc;
	struct sde_kms *sde_kms;
	struct drm_device *dev;
	struct drm_modeset_acquire_ctx ctx;
	int ret = 0;

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	drm_enc = &sde_enc->base;
	sde_kms = sde_encoder_get_kms(drm_enc);
	dev = sde_kms->dev;

	drm_modeset_acquire_init(&ctx, 0);

retry:
	ret = drm_modeset_lock_all_ctx(dev, &ctx);
	if (ret)
		goto end;

	if (!sde_enc->vrr_info.current_state) {
		sde_enc->vrr_info.current_state = drm_atomic_helper_duplicate_state(dev, &ctx);
		SDE_EVT32(sde_enc->vrr_info.current_state);
		goto end;

		if (IS_ERR_OR_NULL(sde_enc->vrr_info.current_state)) {
			if (sde_kms->suspend_state) {
				drm_atomic_state_put(sde_kms->suspend_state);
				sde_kms->suspend_state = NULL;
			}
		}
	}

end:
	if (ret == -EDEADLK) {
		drm_modeset_backoff(&ctx);
		goto retry;
	}

	drm_modeset_drop_locks(&ctx);
	drm_modeset_acquire_fini(&ctx);
}

static u32 sde_encoder_phys_vid_freq_step(struct sde_encoder_phys *phys_enc, int commit_in_progress)
{
	u32 fps;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct msm_debugfs_freq_pattern *config;

	config = sde_enc->vrr_info.debugfs_freq_pattern;
	fps = config->freq_stepping_seq[config->index];

	/* commit occurs, reset index to zero */
	if (commit_in_progress) {
		/* reset stepping sequence */
		config->index = 0;
		fps = config->freq_stepping_seq[config->index];
		config->index++;
	} else if (config->index < config->length) {
		/* set the frame rate to current frequency step in
		 * pattcern and increment the index
		 */
		fps = config->freq_stepping_seq[config->index];
		config->index++;
	} else if (config->index >= config->length) {
		/* set frame rate to qsync_min_fps after iterating
		 * through all the freq stepping sequence
		 */
		fps = sde_enc->mode_info.qsync_min_fps;
	}

	return fps;
}

static void sde_encoder_phys_wd_config(struct sde_encoder_phys *phys_enc, int commit_in_progress)
{
	u32 frame_rate, nominal_fps, min_fps, i;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);

	if (!sde_enc->disp_info.vrr_caps.arp_support)
		return;

	nominal_fps = sde_enc->mode_info.frame_rate;
	min_fps = sde_enc->mode_info.qsync_min_fps;
	frame_rate = 1; /* Default qsync min frequency */

	if (sde_enc->vrr_info.sim_arp_panel_mode.mode == ARP_SIM_FIXED) {
		frame_rate = mult_frac(USEC_PER_SEC, 1,
			sde_enc->vrr_info.debugfs_arp_te_in_ms * 1000);
		SDE_EVT32_IRQ(frame_rate, min_fps);
		frame_rate = (frame_rate > min_fps) ? frame_rate : 1;
	}

	if (sde_enc->vrr_info.sim_arp_panel_mode.mode == ARP_SIM_RANDOM_GENERATOR) {
		get_random_bytes(&i, sizeof(int));
		frame_rate = (i % (nominal_fps - min_fps + 1)) + min_fps;
	}

	if (sde_enc->vrr_info.sim_arp_panel_mode.mode == ARP_SIM_FREQ_STEP) {
		if (sde_enc->vrr_info.debugfs_freq_pattern->length == 0)
			return;
		frame_rate = sde_encoder_phys_vid_freq_step(phys_enc, commit_in_progress);
	}
	SDE_EVT32_IRQ(sde_enc->vrr_info.sim_arp_panel_mode.mode, frame_rate,
		sde_enc->vrr_info.debugfs_arp_te_in_ms);
	phys_enc->ops.control_te(phys_enc, false);
	phys_enc->hw_intf->ops.setup_vsync_source(phys_enc->hw_intf, frame_rate);
	phys_enc->hw_intf->ops.vsync_sel(phys_enc->hw_intf, SDE_VSYNC_SOURCE_WD_TIMER_0);
	phys_enc->ops.control_te(phys_enc, true);
}

static enum hrtimer_restart sde_encoder_phys_vid_arp_transition_timer_cb(struct hrtimer *timer)
{
	struct sde_encoder_vrr_cfg *vrr_cfg = container_of(timer,
		struct sde_encoder_vrr_cfg, arp_transition_timer);
	struct sde_encoder_phys *phys_enc = container_of(vrr_cfg,
		struct sde_encoder_phys, sde_vrr_cfg);
	struct sde_encoder_virt *sde_enc;
	struct msm_drm_thread *disp_thread = NULL;
	struct msm_drm_private *priv;

	if (!phys_enc || !phys_enc->connector) {
		SDE_ERROR("invalid encoder %d\n", !phys_enc);
		return HRTIMER_NORESTART;
	}

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	priv = phys_enc->parent->dev->dev_private;
	disp_thread = &priv->disp_thread[sde_enc->crtc->index];

	if (vrr_cfg->arp_transition_state == ARP_MODE3_HW_TE_ON) {
		vrr_cfg->arp_transition_state = ARM_MODE3_TO_MODE1;
		sde_encoder_store_state(phys_enc);
		sde_connector_update_cmd(phys_enc->connector,
			BIT(DSI_CMD_SET_ARP_MODE1_HW_TE_OFF), 0);
	}
	if (vrr_cfg->arp_transition_state == ARP_MODE1_ACTIVE) {
		vrr_cfg->arp_transition_state = ARP_MODE1_IDLE;
			kthread_queue_work(&disp_thread->worker,
				   &sde_enc->self_refresh_work);
	}
	SDE_EVT32_IRQ(1);

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart sde_encoder_phys_vid_freq_step_callback(struct hrtimer *timer)
{
	struct sde_encoder_vrr_cfg *vrr_cfg = container_of(timer,
		struct sde_encoder_vrr_cfg, arp_transition_timer);
	struct sde_encoder_phys *phys_enc = container_of(vrr_cfg,
		struct sde_encoder_phys, sde_vrr_cfg);
	u32 avr_mode = 0;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_encoder_virt *sde_enc;

	if (!phys_enc || !phys_enc->connector) {
		SDE_ERROR("invalid encoder %d\n", !phys_enc);
		return HRTIMER_NORESTART;
	}
	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	if (!vid_enc)
		return HRTIMER_NORESTART;

	sde_enc = to_sde_encoder_virt(phys_enc->parent);

	if (sde_enc->disp_info.vrr_caps.arp_support) {
		if (vrr_cfg->arp_mode_hw_te) {
			if (vrr_cfg->arp_transition_state == ARP_MODE1_IDLE) {
				vrr_cfg->arp_transition_state = ARP_MODE1_ACTIVE;
				sde_encoder_restore_state(phys_enc);
			}
		} else if (vrr_cfg->arp_mode_sw_timer_mode) {
			avr_mode = sde_connector_get_qsync_mode(phys_enc->connector);
			if (avr_mode && vid_enc->base.hw_intf->ops.avr_trigger)
				vid_enc->base.hw_intf->ops.avr_trigger(
					vid_enc->base.hw_intf);
		}
	}

	SDE_EVT32_IRQ(sde_enc->disp_info.vrr_caps.arp_support,
		vrr_cfg->arp_mode_hw_te);
	return HRTIMER_NORESTART;
}

static void sde_encoder_phys_vid_vblank_irq(void *arg, int irq_idx)
{
	struct sde_encoder_phys *phys_enc = arg;
	struct sde_hw_ctl *hw_ctl;
	struct intf_status intf_status = {0};
	struct sde_cesta_scc_status scc_status = {0, };
	struct sde_cesta_client *cesta_client = sde_encoder_get_cesta_client(phys_enc->parent);
	unsigned long lock_flags;
	u32 flush_register = ~0;
	u32 reset_status = 0;
	int new_cnt = -1, old_cnt = -1;
	u32 event = 0;
	int pend_ret_fence_cnt = 0;
	u32 fence_ready = -1;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct drm_connector *conn = phys_enc ? phys_enc->connector : NULL;
	struct sde_connector *sde_conn;
	struct dsi_display *disp;
	struct samsung_display_driver_data *vdd = NULL;

	if (conn) {
		if (phys_enc->parent->encoder_type == DRM_MODE_ENCODER_DSI) {
			sde_conn = to_sde_connector(conn);
			if (sde_conn) {
				disp = sde_conn->display;
				if (disp) {
					vdd = disp->panel->panel_private;
				}
			}
		}
	}
#endif

	if (!phys_enc)
		return;

	hw_ctl = phys_enc->hw_ctl;
	if (!hw_ctl)
		return;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	SDE_DEBUG_VIDENC(to_sde_encoder_phys_vid(phys_enc), "\n");

	if (vdd) {
		if (atomic_add_unless(&vdd->ss_vsync_cnt, -1, 0) &&
			(atomic_read(&vdd->ss_vsync_cnt) == 0)) {
			wake_up_all(&vdd->ss_vsync_wq);
			pr_info("sde_encoder_phys_vid_vblank_irq  ss_vsync_cnt: %d\n", atomic_read(&vdd->ss_vsync_cnt));
		}
		vdd->vblank_irq_time = ktime_to_us(ktime_get());

		/* To check whether last finger mask frame kickoff is started */
		if (vdd->support_optical_fingerprint)
			vdd->panel_hbm_exit_frame_wait = false;

		ss_print_frame(vdd, true);

		/* Incase of DVRR, vsync interrupt is generated by esync(EM pulse) interrupt */
		if (!vdd->dvrr.support_dvrr)
			ss_print_vsync(vdd);
	}
#endif

	SDE_ATRACE_BEGIN("vblank_irq");

	/*
	 * only decrement the pending flush count if we've actually flushed
	 * hardware. due to sw irq latency, vblank may have already happened
	 * so we need to double-check with hw that it accepted the flush bits
	 */
	spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);

	old_cnt = atomic_read(&phys_enc->pending_kickoff_cnt);

	if (hw_ctl->ops.get_flush_register)
		flush_register = hw_ctl->ops.get_flush_register(hw_ctl);

	if (flush_register)
		goto not_flushed;

	new_cnt = atomic_add_unless(&phys_enc->pending_kickoff_cnt, -1, 0);
	pend_ret_fence_cnt = atomic_read(&phys_enc->pending_retire_fence_cnt);

	/* signal only for master, where there is a pending kickoff */
	if (sde_encoder_phys_vid_is_master(phys_enc) &&
	    atomic_add_unless(&phys_enc->pending_retire_fence_cnt, -1, 0)) {
		event = SDE_ENCODER_FRAME_EVENT_DONE |
			SDE_ENCODER_FRAME_EVENT_SIGNAL_RETIRE_FENCE |
			SDE_ENCODER_FRAME_EVENT_SIGNAL_RELEASE_FENCE;
	}

not_flushed:
	if (hw_ctl->ops.get_reset)
		reset_status = hw_ctl->ops.get_reset(hw_ctl);

	spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

	if (event && phys_enc->parent_ops.handle_frame_done)
		phys_enc->parent_ops.handle_frame_done(phys_enc->parent,
			phys_enc, event);

	if (phys_enc->parent_ops.handle_vblank_virt)
		phys_enc->parent_ops.handle_vblank_virt(phys_enc->parent,
				phys_enc);

	if (phys_enc->hw_intf->ops.get_status)
		phys_enc->hw_intf->ops.get_status(phys_enc->hw_intf,
			&intf_status);

	if (flush_register && hw_ctl->ops.get_hw_fence_status)
		fence_ready = hw_ctl->ops.get_hw_fence_status(hw_ctl);

	SDE_EVT32_IRQ(DRMID(phys_enc->parent), phys_enc->hw_intf->idx - INTF_0,
			old_cnt, atomic_read(&phys_enc->pending_kickoff_cnt),
			reset_status ? SDE_EVTLOG_ERROR : 0,
			flush_register, event,
			atomic_read(&phys_enc->pending_retire_fence_cnt),
			intf_status.frame_count, intf_status.line_count,
			fence_ready, DPUID(phys_enc->parent->dev));
	if (cesta_client)
		sde_cesta_get_status(cesta_client, &scc_status);

	sde_encoder_phys_wd_config(phys_enc, old_cnt);
	sde_encoder_handle_frequency_stepping(phys_enc, old_cnt);

	/* Signal any waiting atomic commit thread */
	wake_up_all(&phys_enc->pending_kickoff_wq);
	SDE_ATRACE_END("vblank_irq");
}

static void sde_encoder_phys_vid_te_irq(void *arg, int irq_idx)
{
	struct sde_encoder_phys *phys_enc = arg;
	struct sde_encoder_phys_vid *vid_enc;
	struct drm_connector *drm_conn;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	u32 qsync_mode;

	if (!phys_enc)
		return;

	drm_conn = sde_enc->cur_master->connector;
	qsync_mode = sde_connector_get_property(drm_conn->state, CONNECTOR_PROP_QSYNC_MODE);
	SDE_EVT32(DRMID(phys_enc->parent), irq_idx, qsync_mode,
		sde_enc->disp_info.vrr_caps.arp_support,
		phys_enc->sde_kms->catalog->is_vrr_hw_fence_enable);

	if (qsync_mode && sde_enc->disp_info.vrr_caps.arp_support &&
		!phys_enc->sde_kms->catalog->is_vrr_hw_fence_enable) {
		vid_enc = to_sde_encoder_phys_vid(phys_enc);
		vid_enc->base.hw_intf->ops.avr_trigger(vid_enc->base.hw_intf);
	}
}

static void sde_encoder_phys_vid_wb_irq(void *arg, int irq_idx)
{
	struct sde_encoder_phys *phys_enc = arg;

	if (!phys_enc)
		return;

	SDE_EVT32(DRMID(phys_enc->parent), irq_idx);
	SDE_DEBUG("WD irq!\n");
}

#if defined(CONFIG_SECDP)
extern int secdp_get_hpd_status(struct device *dev);
#endif

static void sde_encoder_phys_vid_underrun_irq(void *arg, int irq_idx)
{
	struct sde_encoder_phys *phys_enc = arg;
	struct sde_cesta_client *cesta_client;
	struct sde_cesta_scc_status scc_status = {0, };
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct drm_connector *conn = phys_enc ? phys_enc->connector : NULL;
	struct sde_connector *sde_conn;
	struct dsi_display *disp;
	struct samsung_display_driver_data *vdd = NULL;

	if (conn) {
		if (phys_enc->parent->encoder_type == DRM_MODE_ENCODER_DSI) {
			sde_conn = to_sde_connector(conn);
			if (sde_conn) {
				disp = sde_conn->display;
				if (disp)
					vdd = disp->panel->panel_private;
			}
		}
	}
#endif

	if (!phys_enc)
		return;

#if defined(CONFIG_SECDP)
	if (secdp_get_hpd_status(NULL)) {
		/* prints underrun log in case of DP connection */
		SDE_ERROR("underrun while DP connection, intf:%d, type:%d\n",
			phys_enc->intf_idx, phys_enc->parent->encoder_type);
	}
#endif

	cesta_client = sde_encoder_get_cesta_client(phys_enc->parent);

	if (cesta_client)
		sde_cesta_get_status(cesta_client, &scc_status);

	if (phys_enc->parent_ops.handle_underrun_virt)
		phys_enc->parent_ops.handle_underrun_virt(phys_enc->parent,
			phys_enc);

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	if (phys_enc->parent->encoder_type == DRM_MODE_ENCODER_DSI){
		SDE_ERROR_VIDENC(to_sde_encoder_phys_vid(phys_enc), "underrun\n");
#if IS_ENABLED(CONFIG_SEC_DEBUG)
		if (vdd && sec_debug_is_enabled() && ss_panel_attach_get(vdd)) {
			SDE_EVT32(DRMID(phys_enc->parent), SDE_EVTLOG_FATAL);
			//SDE_DBG_DUMP_WQ(SDE_DBG_BUILT_IN_ALL, "panic");
		}
#endif
	}
#endif
}

static void sde_encoder_phys_vid_esync_emsync_irq(void *arg, int irq_idx)
{
	struct sde_encoder_phys *phys_enc = arg;

	if (!phys_enc)
		return;

	if (phys_enc->parent_ops.handle_empulse_virt)
		phys_enc->parent_ops.handle_empulse_virt(phys_enc->parent, phys_enc);

	SDE_EVT32_IRQ(DRMID(phys_enc->parent), irq_idx);
}

static enum hrtimer_restart sde_encoder_phys_vid_esync_backup_sim(struct hrtimer *timer)
{
	struct sde_encoder_phys *phys_enc =
			container_of(timer, struct sde_encoder_phys, empulse_backup_timer);
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct msm_display_info *info;

	if (!sde_enc)
		return HRTIMER_NORESTART;

	if (phys_enc->parent_ops.handle_empulse_virt)
		phys_enc->parent_ops.handle_empulse_virt(phys_enc->parent, phys_enc);

	SDE_EVT32(DRMID(phys_enc->parent), SDE_EVTLOG_FUNC_EXIT);

	info = &sde_enc->disp_info;
	hrtimer_forward_now(timer, ns_to_ktime(DIV_ROUND_UP(NSEC_PER_SEC, info->esync_emsync_fps)));
	return HRTIMER_RESTART;
}

static void _sde_encoder_phys_vid_setup_irq_hw_idx(
		struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_irq *irq;

	/*
	 * Initialize irq->hw_idx only when irq is not registered.
	 * Prevent invalidating irq->irq_idx as modeset may be
	 * called many times during dfps.
	 */

	irq = &phys_enc->irq[INTR_IDX_VSYNC];
	if (irq->irq_idx < 0)
		irq->hw_idx = phys_enc->intf_idx;

	irq = &phys_enc->irq[INTR_IDX_UNDERRUN];
	if (irq->irq_idx < 0)
		irq->hw_idx = phys_enc->intf_idx;

	irq = &phys_enc->irq[INTR_IDX_WD_TIMER];
	if (irq->irq_idx < 0)
		irq->hw_idx = phys_enc->intf_idx;

	irq = &phys_enc->irq[INTR_IDX_TE_DEASSERT];
	if (irq->irq_idx < 0)
		irq->hw_idx = phys_enc->intf_idx;

	irq = &phys_enc->irq[INTR_IDX_ESYNC_EMSYNC];
	if (irq->irq_idx < 0)
		irq->hw_idx = phys_enc->intf_idx;
}

static void sde_encoder_phys_vid_cont_splash_mode_set(
		struct sde_encoder_phys *phys_enc,
		struct drm_display_mode *adj_mode)
{
	if (!phys_enc || !adj_mode) {
		SDE_ERROR("invalid args\n");
		return;
	}

	phys_enc->cached_mode = *adj_mode;
	phys_enc->enable_state = SDE_ENC_ENABLED;

	_sde_encoder_phys_vid_setup_irq_hw_idx(phys_enc);
}

static void sde_encoder_phys_vid_mode_set(
		struct sde_encoder_phys *phys_enc,
		struct drm_display_mode *mode,
		struct drm_display_mode *adj_mode, bool *reinit_mixers)
{
	struct sde_rm *rm;
	struct sde_rm_hw_iter iter;
	int i, instance;
	struct sde_encoder_phys_vid *vid_enc;

	if (!phys_enc || !phys_enc->sde_kms) {
		SDE_ERROR("invalid encoder/kms\n");
		return;
	}

	rm = &phys_enc->sde_kms->rm;
	vid_enc = to_sde_encoder_phys_vid(phys_enc);

	if (adj_mode) {
		phys_enc->cached_mode = *adj_mode;
		drm_mode_debug_printmodeline(adj_mode);
		SDE_DEBUG_VIDENC(vid_enc, "caching mode:\n");
	}

	instance = phys_enc->split_role == ENC_ROLE_SLAVE ? 1 : 0;

	/* Retrieve previously allocated HW Resources. Shouldn't fail */
	sde_rm_init_hw_iter(&iter, phys_enc->parent->base.id, SDE_HW_BLK_CTL);
	for (i = 0; i <= instance; i++) {
		if (sde_rm_get_hw(rm, &iter)) {
			if (phys_enc->hw_ctl && phys_enc->hw_ctl != to_sde_hw_ctl(iter.hw)) {
				*reinit_mixers =  true;
				SDE_EVT32(phys_enc->hw_ctl->idx,
						to_sde_hw_ctl(iter.hw)->idx);
			}
			phys_enc->hw_ctl = to_sde_hw_ctl(iter.hw);
		}
	}
	if (IS_ERR_OR_NULL(phys_enc->hw_ctl)) {
		SDE_ERROR_VIDENC(vid_enc, "failed to init ctl, %ld\n",
				PTR_ERR(phys_enc->hw_ctl));
		phys_enc->hw_ctl = NULL;
		return;
	}

	sde_rm_init_hw_iter(&iter, phys_enc->parent->base.id, SDE_HW_BLK_INTF);
	for (i = 0; i <= instance; i++) {
		if (sde_rm_get_hw(rm, &iter))
			phys_enc->hw_intf = to_sde_hw_intf(iter.hw);
	}

	if (IS_ERR_OR_NULL(phys_enc->hw_intf)) {
		SDE_ERROR_VIDENC(vid_enc, "failed to init intf: %ld\n",
				PTR_ERR(phys_enc->hw_intf));
		phys_enc->hw_intf = NULL;
		return;
	}

	_sde_encoder_phys_vid_setup_irq_hw_idx(phys_enc);

	phys_enc->kickoff_timeout_ms =
		sde_encoder_helper_get_kickoff_timeout_ms(phys_enc->parent);
}

static int sde_encoder_phys_vid_control_vblank_irq(
		struct sde_encoder_phys *phys_enc,
		bool enable)
{
	int ret = 0;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_encoder_virt *sde_enc;
	int refcount;

	if (!phys_enc) {
		SDE_ERROR("invalid encoder\n");
		return -EINVAL;
	}

	mutex_lock(phys_enc->vblank_ctl_lock);
	refcount = atomic_read(&phys_enc->vblank_refcount);
	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	sde_enc = to_sde_encoder_virt(phys_enc->parent);

	/* Slave encoders don't report vblank */
	if (!sde_encoder_phys_vid_is_master(phys_enc))
		goto end;

	/* protect against negative */
	if (!enable && refcount == 0) {
		ret = -EINVAL;
		goto end;
	}

	SDE_DEBUG_VIDENC(vid_enc, "[%pS] enable=%d/%d\n",
			__builtin_return_address(0),
			enable, atomic_read(&phys_enc->vblank_refcount));

	SDE_EVT32(DRMID(phys_enc->parent), enable,
			atomic_read(&phys_enc->vblank_refcount));

	if (enable && atomic_inc_return(&phys_enc->vblank_refcount) == 1) {
		ret = sde_encoder_helper_register_irq(phys_enc, INTR_IDX_VSYNC);
		if (ret)
			atomic_dec_return(&phys_enc->vblank_refcount);
		if (sde_enc && sde_enc->disp_info.vrr_caps.vrr_support) {
			sde_encoder_helper_register_irq(phys_enc, INTR_IDX_TE_DEASSERT);
			sde_encoder_helper_register_irq(phys_enc, INTR_IDX_WD_TIMER);
		}
	} else if (!enable &&
			atomic_dec_return(&phys_enc->vblank_refcount) == 0) {
		ret = sde_encoder_helper_unregister_irq(phys_enc,
				INTR_IDX_VSYNC);
		if (ret)
			atomic_inc_return(&phys_enc->vblank_refcount);
		if (sde_enc && sde_enc->disp_info.vrr_caps.vrr_support) {
			sde_encoder_helper_unregister_irq(phys_enc, INTR_IDX_TE_DEASSERT);
			sde_encoder_helper_unregister_irq(phys_enc, INTR_IDX_WD_TIMER);
		}
	}

end:
	if (ret) {
		SDE_ERROR_VIDENC(vid_enc,
				"control vblank irq error %d, enable %d\n",
				ret, enable);
		SDE_EVT32(DRMID(phys_enc->parent),
				phys_enc->hw_intf->idx - INTF_0,
				enable, refcount, SDE_EVTLOG_ERROR);
	}
	mutex_unlock(phys_enc->vblank_ctl_lock);
	return ret;
}

static void sde_encoder_phys_vid_register_esync_backup_sim(
		struct sde_encoder_phys *phys_enc, bool enable)
{
	struct sde_encoder_virt *sde_enc;
	struct msm_display_info *info;
	u64 now_ns;
	u64 last_ns;
	u64 period_ns;
	u64 sleep_duration_ns;

	if (enable) {
		sde_enc = to_sde_encoder_virt(phys_enc->parent);
		info = &sde_enc->disp_info;

		now_ns = ktime_to_ns(ktime_get());
		last_ns = ktime_to_ns(phys_enc->last_vsync_timestamp);
		period_ns = DIV_ROUND_UP(NSEC_PER_SEC, info->esync_emsync_fps);
		sleep_duration_ns = (period_ns - ((now_ns - last_ns) % period_ns));

		hrtimer_start(&phys_enc->empulse_backup_timer,
				ns_to_ktime(sleep_duration_ns), HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&phys_enc->empulse_backup_timer);
	}
}

static int sde_encoder_phys_vid_control_empulse_irq(
		struct sde_encoder_phys *phys_enc, bool enable)
{
	int ret = 0;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_encoder_virt *sde_enc;
	struct msm_display_info *info;
	int refcount;

	if (!phys_enc) {
		SDE_ERROR("invalid encoder\n");
		return -EINVAL;
	}

	mutex_lock(phys_enc->vblank_ctl_lock);
	refcount = atomic_read(&phys_enc->empulse_irq_refcount);
	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	info = &sde_enc->disp_info;

	/* Slave encoders don't report vblank */
	if (!sde_encoder_phys_vid_is_master(phys_enc))
		goto end;

	/* protect against negative */
	if (!enable && refcount <= 0) {
		ret = -EINVAL;
		goto end;
	}

	SDE_DEBUG_VIDENC(vid_enc, "[%pS] enable=%d/%d\n",
			__builtin_return_address(0),
			enable, atomic_read(&phys_enc->empulse_irq_refcount));

	SDE_EVT32(DRMID(phys_enc->parent), enable,
			atomic_read(&phys_enc->empulse_irq_refcount));

	if (enable && atomic_inc_return(&phys_enc->empulse_irq_refcount) == 1) {
		if (sde_enc->rc_state == SDE_ENC_RC_STATE_ON) {
			ret = sde_encoder_helper_register_irq(phys_enc, INTR_IDX_ESYNC_EMSYNC);
			if (ret)
				atomic_dec(&phys_enc->empulse_irq_refcount);

			phys_enc->empulse_notification_sim = false;
		} else {
			sde_encoder_phys_vid_register_esync_backup_sim(phys_enc, true);
			phys_enc->empulse_notification_sim = true;
		}
	} else if (!enable && atomic_dec_return(&phys_enc->empulse_irq_refcount) == 0) {
		if (phys_enc->empulse_notification_sim) {
			sde_encoder_phys_vid_register_esync_backup_sim(phys_enc, false);
		} else {
			ret = sde_encoder_helper_unregister_irq(phys_enc, INTR_IDX_ESYNC_EMSYNC);
			if (ret)
				atomic_inc(&phys_enc->empulse_irq_refcount);
		}
	}

end:
	if (ret) {
		SDE_ERROR_VIDENC(vid_enc,
				"control empulse irq error %d, enable %d\n",
				ret, enable);
		SDE_EVT32(DRMID(phys_enc->parent),
				phys_enc->hw_intf->idx - INTF_0,
				enable, refcount, SDE_EVTLOG_ERROR);
	}
	mutex_unlock(phys_enc->vblank_ctl_lock);
	return ret;
}


static bool sde_encoder_phys_vid_wait_dma_trigger(
		struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_hw_intf *intf;
	struct sde_hw_ctl *ctl;
	struct intf_status status;

	if (!phys_enc) {
		SDE_ERROR("invalid encoder\n");
		return false;
	}

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	intf = phys_enc->hw_intf;
	ctl = phys_enc->hw_ctl;
	if (!phys_enc->hw_intf || !phys_enc->hw_ctl) {
		SDE_ERROR("invalid hw_intf %d hw_ctl %d\n",
			phys_enc->hw_intf != NULL, phys_enc->hw_ctl != NULL);
		return false;
	}

	if (!intf->ops.get_status)
		return false;

	intf->ops.get_status(intf, &status);

	/* if interface is not enabled, return true to wait for dma trigger */
	return status.is_en ? false : true;
}

static void sde_encoder_phys_vid_enable(struct sde_encoder_phys *phys_enc)
{
	struct msm_drm_private *priv;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_hw_intf *intf;
	struct sde_hw_ctl *ctl;

	if (!phys_enc || !phys_enc->parent || !phys_enc->parent->dev ||
			!phys_enc->parent->dev->dev_private ||
			!phys_enc->sde_kms) {
		SDE_ERROR("invalid encoder/device\n");
		return;
	}
	priv = phys_enc->parent->dev->dev_private;

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	intf = phys_enc->hw_intf;
	ctl = phys_enc->hw_ctl;
	if (!phys_enc->hw_intf || !phys_enc->hw_ctl || !phys_enc->hw_pp) {
		SDE_ERROR("invalid hw_intf %d hw_ctl %d hw_pp %d\n",
				!phys_enc->hw_intf, !phys_enc->hw_ctl,
				!phys_enc->hw_pp);
		return;
	}
	if (!ctl->ops.update_bitmask) {
		SDE_ERROR("invalid hw_ctl ops %d\n", ctl->idx);
		return;
	}

	SDE_DEBUG_VIDENC(vid_enc, "\n");

	if (WARN_ON(!phys_enc->hw_intf->ops.enable_timing))
		return;

	if (!phys_enc->cont_splash_enabled)
		sde_encoder_helper_split_config(phys_enc,
				phys_enc->hw_intf->idx);

	sde_encoder_phys_vid_setup_timing_engine(phys_enc, false);

	(void) sde_encoder_phys_vid_setup_esync_engine(phys_enc, false);

	/*
	 * For cases where both the interfaces are connected to same ctl,
	 * set the flush bit for both master and slave.
	 * For single flush cases (dual-ctl or pp-split), skip setting the
	 * flush bit for the slave intf, since both intfs use same ctl
	 * and HW will only flush the master.
	 */
	if (!test_bit(SDE_CTL_ACTIVE_CFG, &ctl->caps->features) &&
			sde_encoder_phys_needs_single_flush(phys_enc) &&
		!sde_encoder_phys_vid_is_master(phys_enc))
		goto skip_flush;

	/**
	 * skip flushing intf during cont. splash handoff since bootloader
	 * has already enabled the hardware and is single buffered.
	 */
	if (phys_enc->cont_splash_enabled) {
		SDE_DEBUG_VIDENC(vid_enc,
		"skipping intf flush bit set as cont. splash is enabled\n");
		goto skip_flush;
	}

	ctl->ops.update_bitmask(ctl, SDE_HW_FLUSH_INTF, intf->idx, 1);

	if (phys_enc->hw_pp->merge_3d)
		ctl->ops.update_bitmask(ctl, SDE_HW_FLUSH_MERGE_3D,
			phys_enc->hw_pp->merge_3d->idx, 1);

	if (phys_enc->hw_intf->cap->type == INTF_DP &&
		phys_enc->comp_type == MSM_DISPLAY_COMPRESSION_DSC &&
		phys_enc->comp_ratio)
		ctl->ops.update_bitmask(ctl, SDE_HW_FLUSH_PERIPH, intf->idx, 1);

skip_flush:
	SDE_DEBUG_VIDENC(vid_enc, "update pending flush ctl %d intf %d role %d\n",
		ctl->idx - CTL_0, intf->idx, phys_enc->split_role);
	SDE_EVT32(DRMID(phys_enc->parent), phys_enc->split_role,
		atomic_read(&phys_enc->pending_retire_fence_cnt), phys_enc->enable_state);

	/* ctl_flush & timing engine enable will be triggered by framework */
	if (phys_enc->enable_state == SDE_ENC_DISABLED)
		phys_enc->enable_state = SDE_ENC_ENABLING;
}

static void sde_encoder_phys_vid_destroy(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_phys_vid *vid_enc;

	if (!phys_enc) {
		SDE_ERROR("invalid encoder\n");
		return;
	}

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	SDE_DEBUG_VIDENC(vid_enc, "\n");
	kfree(vid_enc);
}

static void sde_encoder_phys_vid_get_hw_resources(
		struct sde_encoder_phys *phys_enc,
		struct sde_encoder_hw_resources *hw_res,
		struct drm_connector_state *conn_state)
{
	struct sde_encoder_phys_vid *vid_enc;

	if (!phys_enc || !hw_res) {
		SDE_ERROR("invalid arg(s), enc %d hw_res %d conn_state %d\n",
				!phys_enc, !hw_res, !conn_state);
		return;
	}

	if ((phys_enc->intf_idx - INTF_0) >= INTF_MAX) {
		SDE_ERROR("invalid intf idx:%d\n", phys_enc->intf_idx);
		return;
	}

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	SDE_DEBUG_VIDENC(vid_enc, "\n");
	hw_res->intfs[phys_enc->intf_idx - INTF_0] = INTF_MODE_VIDEO;
}

static int _sde_encoder_handle_flush_sync_timeout(
		struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_wait_info wait_info = {0};
	struct sde_hw_ctl *hw_ctl;
	int ret;
	u32 flush_register;

	if (!phys_enc || !phys_enc->hw_ctl)
		return -EINVAL;
	/*
	 * When flush sync is enabled, flush register will be cleared only once
	 * flush is successful on both the cores. In those cases, where flush is
	 * not cleared and HW is in sync mode, add an additional wait to check
	 * for flush in the second core. If there is still no flush in the
	 * other core, force async mode for this core and wait for vsync.
	 */
	flush_register = sde_encoder_helper_get_ctl_flush(phys_enc);

	if (!flush_register)
		return 0;

	wait_info.wq = &phys_enc->pending_kickoff_wq;
	wait_info.atomic_cnt = &phys_enc->pending_kickoff_cnt;
	wait_info.timeout_ms = phys_enc->kickoff_timeout_ms;
	hw_ctl = phys_enc->hw_ctl;

	SDE_EVT32(flush_register, SDE_EVTLOG_FUNC_CASE1);
	ret = sde_encoder_helper_wait_for_irq(phys_enc, INTR_IDX_VSYNC, &wait_info);
	flush_register = sde_encoder_helper_get_ctl_flush(phys_enc);
	if (!flush_register)
		return 0;

	SDE_EVT32(ret, flush_register, SDE_EVTLOG_FUNC_CASE2);
	if (hw_ctl->ops.enable_sync_mode) {
		hw_ctl->ops.enable_sync_mode(hw_ctl, true);
		ret = sde_encoder_helper_wait_for_irq(phys_enc, INTR_IDX_VSYNC,
			&wait_info);
	}

	return ret;
}

static int _sde_encoder_phys_vid_wait_for_vblank(
		struct sde_encoder_phys *phys_enc, bool notify)
{
	struct sde_encoder_wait_info wait_info = {0};
	int ret = 0, new_cnt;
	u32 event = SDE_ENCODER_FRAME_EVENT_ERROR |
		SDE_ENCODER_FRAME_EVENT_SIGNAL_RELEASE_FENCE |
		SDE_ENCODER_FRAME_EVENT_SIGNAL_RETIRE_FENCE;
	struct drm_connector *conn;
	struct sde_hw_ctl *hw_ctl;
	u32 flush_register = 0xebad;
	bool timeout = false;

	if (!phys_enc || !phys_enc->hw_ctl) {
		pr_err("invalid encoder\n");
		return -EINVAL;
	}

	hw_ctl = phys_enc->hw_ctl;
	conn = phys_enc->connector;

	wait_info.wq = &phys_enc->pending_kickoff_wq;
	wait_info.atomic_cnt = &phys_enc->pending_kickoff_cnt;
	wait_info.timeout_ms = phys_enc->kickoff_timeout_ms;

	/* Wait for kickoff to complete */
	ret = sde_encoder_helper_wait_for_irq(phys_enc, INTR_IDX_VSYNC,
			&wait_info);

	/*
	 * if hwfencing enabled, try again to wait for up to the extended timeout time in
	 * increments as long as fence has not been signaled.
	 */
	if (ret == -ETIMEDOUT && phys_enc->sde_kms->catalog->hw_fence_rev)
		ret = sde_encoder_helper_hw_fence_extended_wait(phys_enc, phys_enc->hw_ctl,
			&wait_info, INTR_IDX_VSYNC);

	if (ret == -ETIMEDOUT && sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent) &&
		sde_encoder_helper_flush_in_sync_mode(phys_enc)) {
		ret = _sde_encoder_handle_flush_sync_timeout(phys_enc);
	}

	if (ret == -ETIMEDOUT) {
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
		LCD_ERR_NOVDD("[SDE] wait_for_vblank timeout\n");
#endif
		new_cnt = atomic_add_unless(&phys_enc->pending_kickoff_cnt, -1, 0);
		timeout = true;

		/*
		 * Reset ret when flush register is consumed. This handles a race condition between
		 * irq wait timeout handler reading the register status and the actual IRQ handler
		 */
		flush_register = sde_encoder_helper_get_ctl_flush(phys_enc);

		if (!flush_register)
			ret = 0;

		/* if we timeout after the extended wait, reset mixers and do sw override */
		if (ret && phys_enc->sde_kms->catalog->hw_fence_rev)
			sde_encoder_helper_hw_fence_sw_override(phys_enc, hw_ctl);

		SDE_EVT32(DRMID(phys_enc->parent), new_cnt, flush_register, ret,
				SDE_EVTLOG_FUNC_CASE1);
	}

	if (notify && timeout && atomic_add_unless(&phys_enc->pending_retire_fence_cnt, -1, 0)
			&& phys_enc->parent_ops.handle_frame_done) {
		phys_enc->parent_ops.handle_frame_done(phys_enc->parent, phys_enc, event);
/* Below Recovery can cause SSD or AD due to panel off without panel off cmd in factory test (GCT & DSC_CRC) */
#if !IS_ENABLED(CONFIG_SEC_FACTORY)
		/* notify only on actual timeout cases */
		if ((ret == -ETIMEDOUT) && sde_encoder_recovery_events_enabled(phys_enc->parent))
			sde_connector_event_notify(conn, DRM_EVENT_SDE_HW_RECOVERY,
				sizeof(uint8_t), SDE_RECOVERY_HARD_RESET);
#endif
	}

	SDE_EVT32(DRMID(phys_enc->parent), event, notify, timeout, ret,
			ret ? SDE_EVTLOG_FATAL : 0, SDE_EVTLOG_FUNC_EXIT);

	if (!ret)
		sde_encoder_clear_fence_error_in_progress(phys_enc);

	return ret;
}

static int sde_encoder_phys_vid_wait_for_vblank(
		struct sde_encoder_phys *phys_enc)
{
	return _sde_encoder_phys_vid_wait_for_vblank(phys_enc, true);
}

static void sde_encoder_phys_vid_update_txq(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_virt *sde_enc;

	if (!phys_enc)
		return;

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	if (!sde_enc)
		return;

	sde_encoder_helper_update_out_fence_txq(sde_enc, true);
}

static int sde_encoder_phys_vid_wait_for_commit_done(
		struct sde_encoder_phys *phys_enc)
{
	int rc = 0;

	/*
	 * With Interface sync, Master DPU will send signal to enable the Slave DPU Timing engine.
	 * Hence, Slave DPU should not wait for vsync during power on commit.
	 * For all other commits, wait_for_vsync is still needed.
	 */
	if (sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent) &&
			phys_enc->enable_state == SDE_ENC_POST_ENABLING) {
		phys_enc->enable_state = SDE_ENC_ENABLED;
		return rc;
	}

	rc =  _sde_encoder_phys_vid_wait_for_vblank(phys_enc, true);
	if (rc)
		sde_encoder_helper_phys_reset(phys_enc);

	/* Update TxQ for the incoming frame */
	sde_encoder_phys_vid_update_txq(phys_enc);

	return rc;
}

static int sde_encoder_phys_vid_wait_for_vblank_no_notify(
		struct sde_encoder_phys *phys_enc)
{
	return _sde_encoder_phys_vid_wait_for_vblank(phys_enc, false);
}

static void _sde_encoder_phys_vid_arp_ctrl(
		struct sde_encoder_phys *phys_enc)
{
	struct sde_connector_state *c_state;
	struct sde_encoder_vrr_cfg *vrr_cfg;
	struct sde_encoder_virt *sde_enc;

	if (!phys_enc || !phys_enc->parent) {
		SDE_ERROR("invalid encoder parameters\n");
		return;
	}
	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	if (!sde_enc->disp_info.vrr_caps.arp_support) {
		SDE_DEBUG("No ARP support\n");
		return;
	}

	vrr_cfg = &phys_enc->sde_vrr_cfg;
	if (phys_enc->connector && phys_enc->connector->state) {
		c_state = to_sde_connector_state(phys_enc->connector->state);
		if (!c_state) {
			SDE_ERROR("invalid connector state\n");
			return;
		}

		switch (vrr_cfg->arp_transition_state) {
		case ARP_MODE3_HW_TE_ON:
			pr_debug("arp_transition\n");
			break;
		case ARM_MODE3_TO_MODE1:
			//not expected scenario , early ept to take care
			SDE_ERROR("Not expected transition\n");
			vrr_cfg->arp_transition_state = ARP_MODE3_HW_TE_ON;
			sde_connector_update_cmd(phys_enc->connector,
				BIT(DSI_CMD_SET_ARP_MODE3_HW_TE_ON), 0);
			break;
		case ARP_MODE1_ACTIVE:
			SDE_ERROR("Not expected transition %d\n",
					vrr_cfg->arp_transition_state);
			vrr_cfg->arp_transition_state = ARP_MODE3_HW_TE_ON;
			hrtimer_cancel(&phys_enc->sde_vrr_cfg.freq_step_timer);
			hrtimer_cancel(&phys_enc->sde_vrr_cfg.arp_transition_timer);
			break;
		case ARP_MODE1_IDLE:
			vrr_cfg->arp_transition_state = ARP_MODE3_HW_TE_ON;
			sde_connector_update_cmd(phys_enc->connector,
				BIT(DSI_CMD_SET_ARP_MODE3_HW_TE_ON), 0);

			break;
		default:
			SDE_ERROR("unknown wait event %d\n",
					vrr_cfg->arp_transition_state);
			return;
		}
	}
}

static void sde_encoder_phys_vid_enact_updated_qsync_state(struct sde_encoder_phys *phys_enc)
{
	struct sde_connector_state *c_state;
	struct sde_encoder_virt *sde_enc;
	struct sde_ctl_cesta_cfg cfg = {0,};
	struct sde_cesta_client *cesta_client;
	u32 qsync_min_fps = 0;

	if (!phys_enc || !phys_enc->parent) {
		SDE_ERROR("invalid encoder parameters\n");
		return;
	}

	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	if (phys_enc->connector && phys_enc->connector->state) {
		c_state = to_sde_connector_state(phys_enc->connector->state);
		if (!c_state) {
			SDE_ERROR("invalid connector state\n");
			return;
		}

		if (sde_enc && sde_enc->disp_info.vrr_caps.arp_support)
			_sde_encoder_phys_vid_arp_ctrl(phys_enc);

		if (!msm_is_mode_seamless_vrr(&c_state->msm_mode)
			&& sde_connector_is_qsync_updated(phys_enc->connector)) {

			cesta_client = sde_encoder_get_cesta_client(phys_enc->parent);
			if (cesta_client) {
				_sde_encoder_phys_vid_setup_panic_ctrl(phys_enc);

				/*
				 * If the vsync line-count is in the ext-vfp region, cesta
				 * already voted for idle and there is no entity to bring it back
				 * to active once the avr_ctrl is disabled.
				 * Add a cesta flush to get the votes active during disable of avr.
				 */
				if (!sde_connector_get_qsync_mode(phys_enc->connector)) {
					cfg.index = cesta_client->scc_index;
					cfg.vote_state = SDE_CESTA_BW_CLK_NOCHANGE;

					if (phys_enc->hw_ctl && phys_enc->hw_ctl->ops.cesta_flush)
						phys_enc->hw_ctl->ops.cesta_flush(
									phys_enc->hw_ctl, &cfg);
					SDE_EVT32(DRMID(phys_enc->parent), cfg.index);
				}
			}
			if (sde_enc->disp_info.vrr_caps.video_psr_support &&
					phys_enc->parent_ops.get_qsync_fps) {
				phys_enc->parent_ops.get_qsync_fps(
					phys_enc->parent, &qsync_min_fps, phys_enc->connector->state, NULL);
				_sde_encoder_phys_vid_setup_avr(phys_enc, qsync_min_fps);
			}
			_sde_encoder_phys_vid_avr_ctrl(phys_enc);
		}
	}
}

static int sde_encoder_phys_vid_prepare_for_kickoff(
		struct sde_encoder_phys *phys_enc,
		struct sde_encoder_kickoff_params *params)
{
	struct sde_encoder_virt *sde_enc;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_hw_ctl *ctl;
	bool recovery_events;
	struct drm_connector *conn;
	int rc;
	int irq_enable;

	if (!phys_enc || !params || !phys_enc->hw_ctl) {
		SDE_ERROR("invalid encoder/parameters\n");
		return -EINVAL;
	}
	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	sde_enc = to_sde_encoder_virt(phys_enc->parent);

	ctl = phys_enc->hw_ctl;
	if (!ctl->ops.wait_reset_status)
		return 0;

	conn = phys_enc->connector;

	if (sde_enc && sde_enc->disp_info.vrr_caps.video_psr_support &&
			phys_enc->cont_splash_enabled)
		sde_encoder_phys_vid_enact_updated_qsync_state(phys_enc);

	recovery_events = sde_encoder_recovery_events_enabled(
			phys_enc->parent);
	/*
	 * hw supports hardware initiated ctl reset, so before we kickoff a new
	 * frame, need to check and wait for hw initiated ctl reset completion
	 */
	rc = ctl->ops.wait_reset_status(ctl);
	if (rc) {
		SDE_ERROR_VIDENC(vid_enc, "ctl %d reset failure: %d\n",
				ctl->idx, rc);

		++vid_enc->error_count;

		/* to avoid flooding, only log first time, and "dead" time */
		if (vid_enc->error_count == 1) {
			SDE_EVT32(DRMID(phys_enc->parent), SDE_EVTLOG_FATAL);
			mutex_lock(phys_enc->vblank_ctl_lock);

			irq_enable = atomic_read(&phys_enc->vblank_refcount);

			if (irq_enable)
				sde_encoder_helper_unregister_irq(
					phys_enc, INTR_IDX_VSYNC);

			SDE_DBG_DUMP(SDE_DBG_BUILT_IN_ALL);

			if (irq_enable)
				sde_encoder_helper_register_irq(
					phys_enc, INTR_IDX_VSYNC);

			mutex_unlock(phys_enc->vblank_ctl_lock);
		}

		/*
		 * if the recovery event is registered by user, don't panic
		 * trigger panic on first timeout if no listener registered
		 */
		if (recovery_events)
			sde_connector_event_notify(conn, DRM_EVENT_SDE_HW_RECOVERY,
					sizeof(uint8_t), SDE_RECOVERY_CAPTURE);
		else
			SDE_DBG_DUMP(0x0, "panic");

		/* request a ctl reset before the next flush */
		phys_enc->enable_state = SDE_ENC_ERR_NEEDS_HW_RESET;
	} else {
		if (recovery_events && vid_enc->error_count)
			sde_connector_event_notify(conn,
					DRM_EVENT_SDE_HW_RECOVERY,
					sizeof(uint8_t),
					SDE_RECOVERY_SUCCESS);
		vid_enc->error_count = 0;
	}

	return rc;
}

static void sde_encoder_phys_vid_single_vblank_wait(
		struct sde_encoder_phys *phys_enc)
{
	int ret;
	struct intf_status intf_status = {0};
	struct sde_encoder_phys_vid *vid_enc
					= to_sde_encoder_phys_vid(phys_enc);

	/*
	 * Wait for a vsync so we know the ENABLE=0 latched before
	 * the (connector) source of the vsync's gets disabled,
	 * otherwise we end up in a funny state if we re-enable
	 * before the disable latches, which results that some of
	 * the settings changes for the new modeset (like new
	 * scanout buffer) don't latch properly..
	 */
	ret = sde_encoder_phys_vid_control_vblank_irq(phys_enc, true);
	if (ret) {
		SDE_ERROR_VIDENC(vid_enc,
				"failed to enable vblank irq: %d\n",
				ret);
		SDE_EVT32(DRMID(phys_enc->parent),
				phys_enc->hw_intf->idx - INTF_0, ret,
				SDE_EVTLOG_FUNC_CASE1,
				SDE_EVTLOG_ERROR);
	} else {
		ret = _sde_encoder_phys_vid_wait_for_vblank(phys_enc, false);
		if (ret) {
			atomic_set(&phys_enc->pending_kickoff_cnt, 0);
			if (phys_enc->hw_intf && phys_enc->hw_intf->ops.get_status) {
				phys_enc->hw_intf->ops.get_status(phys_enc->hw_intf, &intf_status);
				ret = intf_status.is_en ? ret : 0;
			}
			if (ret) {
				SDE_ERROR_VIDENC(vid_enc,
					"failure waiting for disable: %d\n", ret);
				SDE_EVT32(DRMID(phys_enc->parent),
					phys_enc->hw_intf->idx - INTF_0, ret,
					SDE_EVTLOG_FUNC_CASE2, intf_status.is_en,
					SDE_EVTLOG_ERROR);
			}
		}
		sde_encoder_phys_vid_control_vblank_irq(phys_enc, false);
	}
}

static void sde_encoder_phys_vid_timing_engine_disable_wait(struct sde_encoder_phys *phys_enc)
{
	struct intf_status intf_status = {0};
	unsigned long lock_flags;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);

	spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);

	/* Disconnect the sync mux, when suspend is requested on slave dpu */
	if (sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent) &&
			sde_encoder_phys_has_role_slave_dpu_master_intf(phys_enc) &&
			phys_enc->hw_intf->ops.enable_dpu_sync_ctrl)
		phys_enc->hw_intf->ops.enable_dpu_sync_ctrl(phys_enc->hw_intf, 0);

	if (phys_enc->hw_intf->ops.get_status)
		phys_enc->hw_intf->ops.get_status(phys_enc->hw_intf, &intf_status);

	if (!intf_status.is_en) {
	 	SDE_EVT32(DRMID(phys_enc->parent));
		spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);
		return;
	}

	/* Slave DPU timing engine is disabled */
	phys_enc->hw_intf->ops.enable_timing(phys_enc->hw_intf, false);
	sde_encoder_phys_inc_pending(phys_enc);
	if (sde_enc->disp_info.vrr_caps.video_psr_support)
		phys_enc->hw_intf->ops.avr_enable(phys_enc->hw_intf, false);

	spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

	sde_encoder_phys_vid_single_vblank_wait(phys_enc);
	if (phys_enc->hw_intf->ops.get_status)
		phys_enc->hw_intf->ops.get_status(phys_enc->hw_intf,
			&intf_status);

	if (intf_status.is_en) {
		spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
		sde_encoder_phys_inc_pending(phys_enc);
		spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

		sde_encoder_phys_vid_single_vblank_wait(phys_enc);
	}
}

void sde_encoder_phys_vid_idle_pc_enter(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_virt *sde_enc;
	struct sde_hw_intf_cfg_v1 *intf_cfg;
	struct drm_connector *drm_conn = phys_enc->connector;
	struct sde_connector *c_conn;
	int rc;

	if (WARN_ON(!phys_enc->hw_intf->ops.enable_timing
			|| !phys_enc->hw_intf->ops.enable_backup_esync
			|| !phys_enc->hw_intf->ops.wait_for_esync_src_switch
			|| !phys_enc->hw_intf->ops.enable_esync))
		return;

	SDE_EVT32(DRMID(phys_enc->parent));

	/*
	 * Reset the interface count for this display. It will get cleared if we
	 * power collapse, but in the case that a power collapse doesn't happen,
	 * this will make sure the interface count doesn't keep growing.
	 */
	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	c_conn = to_sde_connector(drm_conn);
	intf_cfg = &sde_enc->cur_master->intf_cfg_v1;
	intf_cfg->intf_count = 0;

	phys_enc->hw_intf->ops.enable_infinite_vfp(phys_enc->hw_intf, true);

	if (drm_conn && c_conn->ops.avoid_cmd_transfer)
		c_conn->ops.avoid_cmd_transfer(c_conn->display, true);

	sde_encoder_phys_vid_timing_engine_disable_wait(phys_enc);

	if (drm_conn && c_conn->ops.avoid_cmd_transfer)
		c_conn->ops.avoid_cmd_transfer(c_conn->display, false);

	sde_connector_osc_clk_ctrl(drm_conn, true);

	sde_encoder_phys_vid_setup_backup_esync_engine(phys_enc, true);
	phys_enc->hw_intf->ops.enable_backup_esync(phys_enc->hw_intf, true);
	rc = phys_enc->hw_intf->ops.wait_for_esync_src_switch(phys_enc->hw_intf, true);
	if (rc) {
		SDE_ERROR("esync source switch main->backup timed out");
		phys_enc->hw_intf->ops.enable_esync(phys_enc->hw_intf, false);
		sde_encoder_phys_vid_setup_backup_esync_engine(phys_enc, false);
		phys_enc->hw_intf->ops.enable_backup_esync(phys_enc->hw_intf, true);
	} else {
		phys_enc->hw_intf->ops.enable_esync(phys_enc->hw_intf, false);
	}

	sde_connector_esync_clk_ctrl(drm_conn, false);
}

void sde_encoder_phys_vid_idle_pc_exit(struct sde_encoder_phys *phys_enc)
{
	struct drm_connector *drm_conn = phys_enc->connector;
	struct sde_hw_intf *intf;
	struct sde_hw_ctl *ctl;
	int rc;

	if (WARN_ON(!phys_enc->hw_intf->ops.enable_timing
			|| !phys_enc->hw_intf->ops.enable_backup_esync
			|| !phys_enc->hw_intf->ops.wait_for_esync_src_switch
			|| !phys_enc->hw_intf->ops.enable_esync))
		return;

	intf = phys_enc->hw_intf;
	ctl = phys_enc->hw_ctl;
	phys_enc->esync_pc_exit = true;

	sde_connector_esync_clk_ctrl(drm_conn, true);

	sde_encoder_phys_vid_setup_timing_engine(phys_enc, true);
	sde_encoder_phys_vid_setup_esync_engine(phys_enc, true);
	ctl->ops.update_bitmask(ctl, SDE_HW_FLUSH_INTF, intf->idx, 1);
	if (phys_enc->hw_pp->merge_3d)
		ctl->ops.update_bitmask(ctl, SDE_HW_FLUSH_MERGE_3D,
			phys_enc->hw_pp->merge_3d->idx, 1);

	phys_enc->hw_intf->ops.enable_esync(phys_enc->hw_intf, true);
	rc = phys_enc->hw_intf->ops.wait_for_esync_src_switch(phys_enc->hw_intf, false);
	if (rc) {
		SDE_ERROR("esync source switch backup->main timed out");
		phys_enc->hw_intf->ops.enable_backup_esync(phys_enc->hw_intf, false);
		sde_encoder_phys_vid_setup_backup_esync_engine(phys_enc, false);
		phys_enc->hw_intf->ops.enable_esync(phys_enc->hw_intf, true);
	} else {
		phys_enc->hw_intf->ops.enable_backup_esync(phys_enc->hw_intf, false);
	}

	sde_connector_osc_clk_ctrl(drm_conn, false);
}

static void sde_encoder_phys_vid_disable(struct sde_encoder_phys *phys_enc)
{
	struct msm_drm_private *priv;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_encoder_virt *sde_enc;
	struct msm_display_info *info;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct drm_connector *conn = phys_enc ? phys_enc->connector : NULL;
	struct sde_connector *sde_conn;
	struct dsi_display *disp;
	struct samsung_display_driver_data *vdd = NULL;

	if (conn) {
		if (phys_enc->parent->encoder_type == DRM_MODE_ENCODER_DSI) {
			sde_conn = to_sde_connector(conn);
			if (sde_conn) {
				disp = sde_conn->display;
				if (disp)
					vdd = disp->panel->panel_private;
			}
		}
	}
#endif

	if (!phys_enc || !phys_enc->parent || !phys_enc->parent->dev ||
			!phys_enc->parent->dev->dev_private) {
		SDE_ERROR("invalid encoder/device\n");
		return;
	}
	priv = phys_enc->parent->dev->dev_private;
	sde_enc = to_sde_encoder_virt(phys_enc->parent);
	info = &sde_enc->disp_info;

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	if (!phys_enc->hw_intf || !phys_enc->hw_ctl) {
		SDE_ERROR("invalid hw_intf %d hw_ctl %d\n",
				!phys_enc->hw_intf, !phys_enc->hw_ctl);
		return;
	}

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	/*
	 * To guarantee ss_wait_for_vsync operation on power off & suspend sequence.
	 * Timing generator & dsi clock should be enabled to use vsync irq.
	 */
	if (vdd)
		wait_event_timeout(vdd->ss_vsync_wq,
			atomic_read(&vdd->ss_vsync_cnt) == 0, msecs_to_jiffies(500));
#endif

	SDE_DEBUG_VIDENC(vid_enc, "\n");

	if (WARN_ON(!phys_enc->hw_intf->ops.enable_timing))
		return;
	else if (!sde_encoder_phys_vid_is_master(phys_enc))
		goto exit;

	if (phys_enc->enable_state == SDE_ENC_DISABLED) {
		SDE_ERROR("already disabled\n");
		return;
	}

	if (sde_in_trusted_vm(phys_enc->sde_kms))
		goto exit;

	sde_encoder_phys_vid_timing_engine_disable_wait(phys_enc);

	if (sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent)) {
		if (sde_encoder_phys_has_role_master_dpu_master_intf(phys_enc)) {
			if (phys_enc->hw_ctl &&
					phys_enc->hw_ctl->ops.setup_flush_sync)
				phys_enc->hw_ctl->ops.setup_flush_sync(
					phys_enc->hw_ctl, true, false);
		} else if (sde_encoder_phys_has_role_slave_dpu_master_intf(phys_enc)) {
			if (phys_enc->hw_ctl &&
				phys_enc->hw_ctl->ops.setup_flush_sync)
				phys_enc->hw_ctl->ops.setup_flush_sync(
					phys_enc->hw_ctl, false, false);
		}
	}

	if (info->esync_enabled) {
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
		if (vdd) {
			ss_esync_off_pre(vdd);
			LCD_INFO(vdd, "ESYNC Disabled\n");
		} else {
			pr_err("[SDE] Failed to call ss_esync_off_pre due to vdd is NULL\n");
		}
#endif
		if (phys_enc->hw_intf->ops.enable_esync)
			phys_enc->hw_intf->ops.enable_esync(phys_enc->hw_intf, false);
		if (phys_enc->hw_intf->ops.enable_backup_esync)
			phys_enc->hw_intf->ops.enable_backup_esync(phys_enc->hw_intf, false);

		phys_enc->esync_pc_exit = false;

		if (sde_enc->rc_state == SDE_ENC_RC_STATE_IDLE)
			sde_connector_osc_clk_ctrl(phys_enc->connector, false);
	}

	if (sde_enc && sde_enc->disp_info.vrr_caps.vrr_support) {
		hrtimer_cancel(&phys_enc->sde_vrr_cfg.freq_step_timer);
		hrtimer_cancel(&phys_enc->sde_vrr_cfg.backlight_timer);
	}

	sde_encoder_helper_phys_disable(phys_enc, NULL);
exit:
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	if (!sde_encoder_phys_vid_is_master(phys_enc)) {
		phys_enc->hw_intf->ops.enable_timing(phys_enc->hw_intf, 0);
		SDE_DEBUG_VIDENC(vid_enc, "disable timing gen\n");
	}
#endif
	SDE_EVT32(DRMID(phys_enc->parent),
		atomic_read(&phys_enc->pending_retire_fence_cnt), phys_enc->split_role);
	phys_enc->vfp_cached = 0;
	phys_enc->enable_state = SDE_ENC_DISABLED;
}

static int sde_encoder_phys_vid_poll_for_active_region(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_phys_vid *vid_enc;
	struct intf_timing_params *timing;
	u32 line_cnt, v_inactive, poll_time_us, trial = 0;

	if (!phys_enc || !phys_enc->hw_intf || !phys_enc->hw_intf->ops.get_line_count)
		return -EINVAL;

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	timing = &vid_enc->timing_params;

	/* if programmable fetch is not enabled return early or if it is not a DSI interface*/
	if (!programmable_fetch_get_num_lines(vid_enc, timing) ||
			phys_enc->hw_intf->cap->type != INTF_DSI)
		return 0;

	poll_time_us = DIV_ROUND_UP(1000000, timing->vrefresh) / MAX_POLL_CNT;
	v_inactive = timing->v_front_porch + timing->v_back_porch + timing->vsync_pulse_width;

	do {
		usleep_range(poll_time_us, poll_time_us + 5);
		line_cnt = phys_enc->hw_intf->ops.get_line_count(phys_enc->hw_intf);
		trial++;
	} while ((trial < MAX_POLL_CNT) && (line_cnt < v_inactive));

	return (trial >= MAX_POLL_CNT) ? -ETIMEDOUT : 0;
}

static void sde_encoder_phys_vid_handle_post_kickoff(
		struct sde_encoder_phys *phys_enc)
{
	unsigned long lock_flags;
	struct sde_encoder_phys_vid *vid_enc;
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);
	struct msm_display_info *info = &sde_enc->disp_info;
	struct drm_connector *drm_conn = phys_enc->connector;
	struct sde_connector *sde_conn = to_sde_connector(drm_conn);
	struct drm_connector_state *drm_conn_state = drm_conn->state;
	u32 avr_mode;
	u32 ret;

#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
	struct dsi_display *disp = NULL;
	struct samsung_display_driver_data *vdd = NULL;

	if (sde_conn) {
		disp = sde_conn->display;
		if (disp)
			vdd = disp->panel->panel_private;
	}
#endif

	if (!phys_enc) {
		SDE_ERROR("invalid encoder\n");
		return;
	}

	vid_enc = to_sde_encoder_phys_vid(phys_enc);
	SDE_DEBUG_VIDENC(vid_enc, "enable_state %d\n", phys_enc->enable_state);

	if (phys_enc->enable_state == SDE_ENC_ENABLING)
		SDE_EVT32(DRMID(phys_enc->parent), phys_enc->hw_intf->idx - INTF_0,
			phys_enc->split_role);

	/*
	 * Video mode must flush CTL before enabling timing engine
	 * Video encoders need to turn on their interfaces now
	 */
	if (phys_enc->enable_state == SDE_ENC_ENABLING) {
		if (sde_encoder_phys_vid_is_master(phys_enc) &&
			!sde_encoder_phys_has_role_slave_dpu_master_intf(phys_enc)) {
			if (info->esync_enabled) {
				phys_enc->hw_intf->ops.enable_esync(phys_enc->hw_intf, true);
				if (sde_conn->ops.dcs_cmd_tx)
					sde_conn->ops.dcs_cmd_tx(drm_conn_state,
							DSI_CMD_SET_ESYNC_POST_ON);
#if IS_ENABLED(CONFIG_DISPLAY_SAMSUNG)
				if (vdd) {
					LCD_INFO(vdd, "ESYNC Enabled\n");
					ss_esync_on_post(vdd);
				} else {
					pr_err("[SDE] Failed to call ss_esync_on_post due to vdd is NULL\n");
				}
#endif
			}

			if (sde_conn->ops.avoid_cmd_transfer)
				sde_conn->ops.avoid_cmd_transfer(sde_conn->display, true);

			spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
			phys_enc->hw_intf->ops.enable_timing(phys_enc->hw_intf, 1);
			spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

			ret = sde_encoder_phys_vid_poll_for_active_region(phys_enc);
			if (ret)
				SDE_DEBUG_VIDENC(vid_enc, "poll for active failed ret:%d\n", ret);

			if (sde_conn->ops.avoid_cmd_transfer)
				sde_conn->ops.avoid_cmd_transfer(sde_conn->display, false);
			phys_enc->enable_state = SDE_ENC_ENABLED;
		/* Slave DPU Timing engine mux select from Master DPU */
		} else if (sde_encoder_has_dpu_ctl_op_sync(phys_enc->parent) &&
				sde_encoder_phys_has_role_slave_dpu_master_intf(phys_enc) &&
				phys_enc->hw_intf->ops.enable_dpu_sync_ctrl) {
			spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
			phys_enc->hw_intf->ops.enable_dpu_sync_ctrl(phys_enc->hw_intf, 1);
			spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);
			phys_enc->enable_state = SDE_ENC_POST_ENABLING;
		}
	}

	if (phys_enc->esync_pc_exit) {
		spin_lock_irqsave(phys_enc->enc_spinlock, lock_flags);
		phys_enc->hw_intf->ops.enable_timing(phys_enc->hw_intf, true);
		spin_unlock_irqrestore(phys_enc->enc_spinlock, lock_flags);

		ret = sde_encoder_phys_vid_poll_for_active_region(phys_enc);
		if (ret)
			SDE_DEBUG_VIDENC(vid_enc, "poll for active failed ret:%d\n", ret);

		if (!sde_enc->vrr_info.vhm_cmd_in_progress
				&& sde_conn->ops.avoid_cmd_transfer)
			sde_conn->ops.avoid_cmd_transfer(sde_conn->display, false);

		phys_enc->esync_pc_exit = false;
	}

	avr_mode = sde_connector_get_qsync_mode(phys_enc->connector);

	if (avr_mode && vid_enc->base.hw_intf->ops.avr_trigger &&
			!phys_enc->sde_kms->catalog->is_vrr_hw_fence_enable) {
		vid_enc->base.hw_intf->ops.avr_trigger(vid_enc->base.hw_intf);
		SDE_EVT32(DRMID(phys_enc->parent),
				phys_enc->hw_intf->idx - INTF_0,
				SDE_EVTLOG_FUNC_CASE9);
		if (sde_enc->disp_info.vrr_caps.video_psr_support)
			SDE_ERROR("HW fence is disabled. Overriding TE monitor VHM case.\n");
	}
}

static void sde_encoder_phys_vid_prepare_for_commit(
		struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(phys_enc->parent);

	if (!sde_enc) {
		SDE_ERROR("invalid sde_enc\n");
		return;
	}

	/* during continuous splash, VHM needs this to happen in prepare for kickoff */
	if (!(sde_enc->disp_info.vrr_caps.video_psr_support && phys_enc->cont_splash_enabled))
		sde_encoder_phys_vid_enact_updated_qsync_state(phys_enc);
}

static void sde_encoder_phys_vid_irq_control(struct sde_encoder_phys *phys_enc,
		bool enable)
{
	struct sde_encoder_phys_vid *vid_enc;
	int ret;

	if (!phys_enc)
		return;

	vid_enc = to_sde_encoder_phys_vid(phys_enc);

	SDE_EVT32(DRMID(phys_enc->parent), phys_enc->hw_intf->idx - INTF_0,
			enable, atomic_read(&phys_enc->vblank_refcount));

	if (enable) {
		ret = sde_encoder_phys_vid_control_vblank_irq(phys_enc, true);
		if (ret)
			return;

		sde_encoder_helper_register_irq(phys_enc, INTR_IDX_UNDERRUN);
	} else {
		sde_encoder_phys_vid_control_vblank_irq(phys_enc, false);
		sde_encoder_helper_unregister_irq(phys_enc, INTR_IDX_UNDERRUN);
	}
}

static int sde_encoder_phys_vid_get_line_count(
		struct sde_encoder_phys *phys_enc)
{
	if (!phys_enc)
		return -EINVAL;

	if (!sde_encoder_phys_vid_is_master(phys_enc))
		return -EINVAL;

	if (!phys_enc->hw_intf || !phys_enc->hw_intf->ops.get_line_count)
		return -EINVAL;

	return phys_enc->hw_intf->ops.get_line_count(phys_enc->hw_intf);
}

static u32 sde_encoder_phys_vid_get_underrun_line_count(
		struct sde_encoder_phys *phys_enc)
{
	u32 underrun_linecount = 0xebadebad;
	u32 intf_intr_status = 0xebadebad;
	struct intf_status intf_status = {0};

	if (!phys_enc)
		return -EINVAL;

	if (!sde_encoder_phys_vid_is_master(phys_enc) || !phys_enc->hw_intf)
		return -EINVAL;

	if (phys_enc->hw_intf->ops.get_status)
		phys_enc->hw_intf->ops.get_status(phys_enc->hw_intf,
			&intf_status);

	if (phys_enc->hw_intf->ops.get_underrun_line_count)
		underrun_linecount =
		  phys_enc->hw_intf->ops.get_underrun_line_count(
			phys_enc->hw_intf);

	if (phys_enc->hw_intf->ops.get_intr_status)
		intf_intr_status = phys_enc->hw_intf->ops.get_intr_status(
				phys_enc->hw_intf);

	SDE_EVT32(DRMID(phys_enc->parent), underrun_linecount,
		intf_status.frame_count, intf_status.line_count,
		intf_intr_status, DPUID(phys_enc->parent->dev));

	return underrun_linecount;
}

static int sde_encoder_phys_vid_wait_for_active(
			struct sde_encoder_phys *phys_enc)
{
	struct drm_display_mode mode;
	struct sde_encoder_phys_vid *vid_enc;
	u32 ln_cnt, min_ln_cnt, active_lns_cnt;
	u32 retry = MAX_POLL_CNT;

	vid_enc =  to_sde_encoder_phys_vid(phys_enc);

	if (!phys_enc->hw_intf || !phys_enc->hw_intf->ops.get_line_count) {
		SDE_ERROR_VIDENC(vid_enc, "invalid vid_enc params\n");
		return -EINVAL;
	}

	mode = phys_enc->cached_mode;

	min_ln_cnt = (mode.vtotal - mode.vsync_start) +
		(mode.vsync_end - mode.vsync_start);
	active_lns_cnt = mode.vdisplay;

	while (retry) {
		ln_cnt = phys_enc->hw_intf->ops.get_line_count(
				phys_enc->hw_intf);

		if ((ln_cnt >= min_ln_cnt) &&
			(ln_cnt < (active_lns_cnt + min_ln_cnt))) {
			SDE_DEBUG_VIDENC(vid_enc,
					"Needed lines left line_cnt=%d\n",
					ln_cnt);
			return 0;
		}

		SDE_ERROR_VIDENC(vid_enc, "line count is less. line_cnt = %d\n", ln_cnt);
		udelay(POLL_TIME_USEC_FOR_LN_CNT);
		retry--;
	}

	return -EINVAL;
}

void sde_encoder_phys_vid_add_enc_to_minidump(struct sde_encoder_phys *phys_enc)
{
	struct sde_encoder_phys_vid *vid_enc;
	vid_enc =  to_sde_encoder_phys_vid(phys_enc);

	sde_mini_dump_add_va_region("sde_enc_phys_vid", sizeof(*vid_enc), vid_enc);
}

void sde_encoder_phys_vid_cesta_ctrl_cfg(struct sde_encoder_phys *phys_enc,
		struct sde_cesta_ctrl_cfg *cfg, bool *req_flush, bool *req_scc)
{
	bool qsync_en = sde_connector_get_qsync_mode(phys_enc->connector);

	cfg->enable = true;
	cfg->avr_enable = qsync_en;
	cfg->intf = phys_enc->intf_idx - INTF_0;
	cfg->auto_active_on_panic = true;
	cfg->req_mode = qsync_en ? SDE_CESTA_CTRL_REQ_IMMEDIATE : SDE_CESTA_CTRL_REQ_PANIC_REGION;
	cfg->hw_sleep_enable = !phys_enc->sde_kms->splash_data.num_splash_displays;

	if ((phys_enc->split_role == DPU_MASTER_ENC_ROLE_MASTER)
			|| (phys_enc->split_role == DPU_SLAVE_ENC_ROLE_MASTER))
		cfg->dual_dsi = true;

	*req_flush = qsync_en;
	*req_scc = sde_connector_is_qsync_updated(phys_enc->connector);
}

static void sde_encoder_phys_vid_init_ops(struct sde_encoder_phys_ops *ops)
{
	ops->is_master = sde_encoder_phys_vid_is_master;
	ops->mode_set = sde_encoder_phys_vid_mode_set;
	ops->cont_splash_mode_set = sde_encoder_phys_vid_cont_splash_mode_set;
	ops->mode_fixup = sde_encoder_phys_vid_mode_fixup;
	ops->enable = sde_encoder_phys_vid_enable;
	ops->disable = sde_encoder_phys_vid_disable;
	ops->destroy = sde_encoder_phys_vid_destroy;
	ops->get_hw_resources = sde_encoder_phys_vid_get_hw_resources;
	ops->control_vblank_irq = sde_encoder_phys_vid_control_vblank_irq;
	ops->control_empulse_irq = sde_encoder_phys_vid_control_empulse_irq;
	ops->wait_for_commit_done = sde_encoder_phys_vid_wait_for_commit_done;
	ops->wait_for_vblank = sde_encoder_phys_vid_wait_for_vblank_no_notify;
	ops->wait_for_tx_complete = sde_encoder_phys_vid_wait_for_vblank;
	ops->irq_control = sde_encoder_phys_vid_irq_control;
	ops->prepare_for_kickoff = sde_encoder_phys_vid_prepare_for_kickoff;
	ops->handle_post_kickoff = sde_encoder_phys_vid_handle_post_kickoff;
	ops->needs_single_flush = sde_encoder_phys_needs_single_flush;
	ops->control_te = sde_encoder_phys_vid_connect_te;
	ops->setup_vsync_source = sde_encoder_phys_vid_setup_vsync_source;
	ops->setup_misr = sde_encoder_helper_setup_misr;
	ops->collect_misr = sde_encoder_helper_collect_misr;
	ops->trigger_flush = sde_encoder_helper_trigger_flush;
	ops->hw_reset = sde_encoder_helper_hw_reset;
	ops->get_line_count = sde_encoder_phys_vid_get_line_count;
	ops->wait_dma_trigger = sde_encoder_phys_vid_wait_dma_trigger;
	ops->wait_for_active = sde_encoder_phys_vid_wait_for_active;
	ops->prepare_commit = sde_encoder_phys_vid_prepare_for_commit;
	ops->get_underrun_line_count =
		sde_encoder_phys_vid_get_underrun_line_count;
	ops->add_to_minidump = sde_encoder_phys_vid_add_enc_to_minidump;
	ops->cesta_ctrl_cfg = sde_encoder_phys_vid_cesta_ctrl_cfg;
	ops->idle_pc_enter = sde_encoder_phys_vid_idle_pc_enter;
	ops->idle_pc_exit = sde_encoder_phys_vid_idle_pc_exit;
}

struct sde_encoder_phys *sde_encoder_phys_vid_init(
		struct sde_enc_phys_init_params *p)
{
	struct sde_encoder_phys *phys_enc = NULL;
	struct sde_encoder_phys_vid *vid_enc = NULL;
	struct sde_hw_mdp *hw_mdp;
	struct sde_encoder_irq *irq;
	int i, ret = 0;

	if (!p) {
		ret = -EINVAL;
		goto fail;
	}

	vid_enc = kzalloc(sizeof(*vid_enc), GFP_KERNEL);
	if (!vid_enc) {
		ret = -ENOMEM;
		goto fail;
	}

	phys_enc = &vid_enc->base;

	hw_mdp = sde_rm_get_mdp(&p->sde_kms->rm);
	if (IS_ERR_OR_NULL(hw_mdp)) {
		ret = PTR_ERR(hw_mdp);
		SDE_ERROR("failed to get mdptop\n");
		goto fail;
	}
	phys_enc->hw_mdptop = hw_mdp;
	phys_enc->intf_idx = p->intf_idx;

	SDE_DEBUG_VIDENC(vid_enc, "\n");

	sde_encoder_phys_vid_init_ops(&phys_enc->ops);
	phys_enc->parent = p->parent;
	phys_enc->parent_ops = p->parent_ops;
	phys_enc->sde_kms = p->sde_kms;
	phys_enc->split_role = p->split_role;
	phys_enc->intf_mode = INTF_MODE_VIDEO;
	phys_enc->enc_spinlock = p->enc_spinlock;
	phys_enc->vblank_ctl_lock = p->vblank_ctl_lock;
	phys_enc->comp_type = p->comp_type;
	phys_enc->kickoff_timeout_ms = DEFAULT_KICKOFF_TIMEOUT_MS;
	for (i = 0; i < INTR_IDX_MAX; i++) {
		irq = &phys_enc->irq[i];
		INIT_LIST_HEAD(&irq->cb.list);
		irq->irq_idx = -EINVAL;
		irq->hw_idx = -EINVAL;
		irq->cb.arg = phys_enc;
	}

	irq = &phys_enc->irq[INTR_IDX_VSYNC];
	irq->name = "vsync_irq";
	irq->intr_type = SDE_IRQ_TYPE_INTF_VSYNC;
	irq->intr_idx = INTR_IDX_VSYNC;
	irq->cb.func = sde_encoder_phys_vid_vblank_irq;

	irq = &phys_enc->irq[INTR_IDX_TE_DEASSERT];
	irq->name = "te_deassert";
	irq->intr_type = SDE_IRQ_TYPE_INTF_TEAR_TE_DEASSERT;
	irq->intr_idx = INTR_IDX_TE_DEASSERT;
	irq->cb.func = sde_encoder_phys_vid_te_irq;

	irq = &phys_enc->irq[INTR_IDX_WD_TIMER];
	irq->name = "wd_irq";
	irq->intr_type = SDE_IRQ_TYPE_WD_TIMER_1;
	irq->intr_idx = INTR_IDX_WD_TIMER;
	irq->cb.func = sde_encoder_phys_vid_wb_irq;

	irq = &phys_enc->irq[INTR_IDX_UNDERRUN];
	irq->name = "underrun";
	irq->intr_type = SDE_IRQ_TYPE_INTF_UNDER_RUN;
	irq->intr_idx = INTR_IDX_UNDERRUN;
	irq->cb.func = sde_encoder_phys_vid_underrun_irq;

	irq = &phys_enc->irq[INTR_IDX_ESYNC_EMSYNC];
	irq->name = "esync_irq";
	irq->intr_type = SDE_IRQ_TYPE_INTF_ESYNC_EMSYNC;
	irq->intr_idx = INTR_IDX_ESYNC_EMSYNC;
	irq->cb.func = sde_encoder_phys_vid_esync_emsync_irq;

	atomic_set(&phys_enc->vblank_refcount, 0);
	atomic_set(&phys_enc->pending_kickoff_cnt, 0);
	atomic_set(&phys_enc->pending_retire_fence_cnt, 0);
	init_waitqueue_head(&phys_enc->pending_kickoff_wq);
	phys_enc->enable_state = SDE_ENC_DISABLED;
	hrtimer_init(&phys_enc->sde_vrr_cfg.freq_step_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	phys_enc->sde_vrr_cfg.freq_step_timer.function =
		sde_encoder_phys_vid_freq_step_callback;
	hrtimer_init(&phys_enc->sde_vrr_cfg.arp_transition_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	phys_enc->sde_vrr_cfg.arp_transition_timer.function =
		sde_encoder_phys_vid_arp_transition_timer_cb;
	hrtimer_init(&phys_enc->sde_vrr_cfg.self_refresh_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	phys_enc->sde_vrr_cfg.self_refresh_timer.function =
		sde_encoder_phys_phys_self_refresh_helper;

	hrtimer_init(&phys_enc->sde_vrr_cfg.backlight_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	phys_enc->sde_vrr_cfg.backlight_timer.function =
		sde_encoder_phys_backlight_timer_cb;

	hrtimer_init(&phys_enc->empulse_backup_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	phys_enc->empulse_backup_timer.function =
		sde_encoder_phys_vid_esync_backup_sim;

	SDE_DEBUG_VIDENC(vid_enc, "created intf idx:%d\n", p->intf_idx);

	return phys_enc;

fail:
	SDE_ERROR("failed to create encoder\n");
	if (vid_enc)
		sde_encoder_phys_vid_destroy(phys_enc);

	return ERR_PTR(ret);
}
