/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2021, Samsung Electronics. All rights reserved.

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "ss_wrapper_common.h"

#define MAX_LEN_DUMP_BUF	(256)

#define kfree_and_null(x)	{ \
	if (x) { \
		kfree(x); \
		x = NULL; \
	} \
}

/*
 * Check CONFIG_DPM_WATCHDOG_TIMEOUT time.
 * Device should not take resume/suspend over CONFIG_DPM_WATCHDOG_TIMEOUT time.
 */
#define WAIT_PM_RESUME_TIMEOUT_MS	(3000)	/* 3 seconds */

int __mockable ss_wait_for_pm_resume(
			struct samsung_display_driver_data *vdd)
{
	struct drm_device *ddev = GET_DRM_DEV(vdd);
	int timeout = WAIT_PM_RESUME_TIMEOUT_MS;

	while (!pm_runtime_enabled(ddev->dev) && --timeout)
		usleep_range(1000, 1000); /* 1ms */

	if (timeout < WAIT_PM_RESUME_TIMEOUT_MS)
		LCD_INFO(vdd, "wait for pm resume (timeout: %d)", timeout);

	if (!timeout) {
		LCD_ERR(vdd, "pm resume timeout\n");

		/* Should not reach here... Add panic to debug further */
		ss_display_panic(vdd, "timeout wait for pm resume");
		return -ETIMEDOUT;
	}

	return 0;
}

int __mockable ss_gpio_get_value(struct samsung_display_driver_data *vdd, unsigned int gpio)
{
	struct gpio_desc *desc;

	desc = gpio_to_desc(gpio);
	if (unlikely(gpiod_cansleep(desc)))
		return gpio_get_value_cansleep(gpio);
	else
		return gpio_get_value(gpio);
}

int __mockable ss_gpio_set_value(struct samsung_display_driver_data *vdd, unsigned int gpio, int value)
{
	struct gpio_desc *desc;

	desc = gpio_to_desc(gpio);
	if (unlikely(gpiod_cansleep(desc)))
		gpio_set_value_cansleep(gpio, value);
	else
		gpio_set_value(gpio, value);

	return 0;
}

#if IS_ENABLED(CONFIG_DRM_SDE_VM)
int __mockable ss_sde_vm_owns_hw(struct samsung_display_driver_data *vdd)
{
	struct dsi_display *display = NULL;
	struct msm_drm_private *priv = NULL;
	struct sde_kms *sde_kms = NULL;

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display))
		return -ENODEV;

	priv = display->drm_dev->dev_private;
	if (!priv)
		return -ENODEV;

	sde_kms = to_sde_kms(priv->kms);
	if (!sde_kms)
		return -ENODEV;

	return sde_vm_owns_hw(sde_kms);
}

void __mockable ss_sde_vm_lock(struct samsung_display_driver_data *vdd, bool lock)
{
	struct dsi_display *display = NULL;
	struct msm_drm_private *priv = NULL;
	struct sde_kms *sde_kms = NULL;

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display))
		return;

	priv = display->drm_dev->dev_private;
	if (!priv)
		return;

	sde_kms = to_sde_kms(priv->kms);

	if (sde_kms && sde_kms->vm) {
		if (lock)
			sde_vm_lock(sde_kms);
		else
			sde_vm_unlock(sde_kms);
	}
}
#else
int ss_sde_vm_owns_hw(struct samsung_display_driver_data *vdd)
{
	return 1;
}

void ss_sde_vm_lock(struct samsung_display_driver_data *vdd, bool lock)
{
}
#endif

int __mockable ss_dsi_display_suspend(struct device *dev)
{
	struct dsi_display *display = dev_get_drvdata(dev);
	struct samsung_display_driver_data *vdd = NULL;

	if (!display) {
		pr_err("%s: Failed to get display\n", __func__);
		return 0;
	}

	if (!display->panel) {
		pr_err("%s: No panel\n", __func__);
		return 0;
	}

	vdd = display->panel->panel_private;
	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: fail to get vdd\n", __func__);
		return 0;
	}

	/*
	 * Flush sde_normal_clk_work does not mean clock is off.
	 * This function should be called before msm_pm_suspend -> sde_kms_pm_suspend.
	 * sde_kms_pm_suspend can gaurantee all the SDE resource off.
	 */
	flush_delayed_work(&vdd->sde_normal_clk_work);
	cancel_delayed_work_sync(&vdd->smooth_br_done_work);

	if (ss_dct_is_clk_on(vdd)) {
		LCD_ERR(vdd, "display clock is on in suspend\n");
		ss_dct_dump_all_info(vdd);
	}

	LCD_INFO(vdd, "Done\n");

	return 0;
}

bool __mockable ss_gpio_is_valid(int number)
{
	return gpio_is_valid(number);
}

int __mockable ss_gpio_to_irq(unsigned gpio)
{
	return gpio_to_irq(gpio);
}

void __weak sde_encoder_early_wakeup(struct drm_encoder *drm_enc)
{
}

bool is_ss_cmd_op_skip(struct samsung_display_driver_data *vdd,
			struct ss_cmd_desc *cmd)
{
	u32 delay;

	if (!cmd)
		return false;

	if (cmd->skip_by_cond)
		return true;

	if (cmd->skip_lvl_key) {
		/* delay post_wait_ms and skip level key command transmission */
		if (cmd->post_wait_ms)
			delay = cmd->post_wait_ms;
		else if (cmd->post_wait_frame)
			delay = ss_frame_to_ms(vdd, cmd->post_wait_frame);
		else
			delay = 0;

		if (delay)
			usleep_range(delay * 1000, ((delay * 1000) + 10));

		return true;
	}

	return false;
}

int register_op_sym_cb(struct samsung_display_driver_data *vdd,
			char *tmp_symbol,
			int (*cb)(struct samsung_display_driver_data *,
					char *, struct ss_cmd_desc *),
			bool op_update)
{
	char *symbol;
	int i;

	if (!tmp_symbol) {
		pr_err("%s: SDE: null tmp_symbol\n", __func__);
		return -EINVAL;
	}

	if (strnlen(tmp_symbol, 1024) == 1024) {
		pr_err("%s: SDE: not null terminated tmp_symbol\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < vdd->sym_cb_tbl_size; i++) {
		if (op_update == vdd->sym_cb_tbl[i].op_update &&
			strlen(tmp_symbol) == strlen(vdd->sym_cb_tbl[i].symbol) &&
			!strncasecmp(tmp_symbol, vdd->sym_cb_tbl[i].symbol, strlen(tmp_symbol))) {
			/* replace symbol callback with new one */
			pr_info("%s: SDE: replace symbol cb(%s)\n",
					__func__, tmp_symbol);
			vdd->sym_cb_tbl[i].cb = cb;
			return 0;
		}
	}

	symbol = kzalloc(strlen(tmp_symbol) + 1, GFP_KERNEL);
	if (!symbol) {
		pr_err("%s: SDE: fail to alloc symbol(%s)\n",
				__func__, tmp_symbol);
		return -ENOMEM;
	}

	strcpy(symbol, tmp_symbol);
	vdd->sym_cb_tbl[vdd->sym_cb_tbl_size].symbol = symbol;
	vdd->sym_cb_tbl[vdd->sym_cb_tbl_size].cb = cb;
	vdd->sym_cb_tbl[vdd->sym_cb_tbl_size++].op_update = op_update;

	return 0;
}

enum {
	VRR_SPEED_MODE_NS = 0,
	VRR_SPEED_MODE_HS,
	VRR_SPEED_MODE_PHS,
	VRR_SPEED_MODE_DONT_CARE,
};

static int is_matched_build(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	if (!strncasecmp(val, "FACTORY", strlen(val))) {
		if (vdd->is_factory_mode)
			return 1;
	} else if (!strncasecmp(val, "USER", strlen(val))) {
		if (!vdd->is_factory_mode)
			return 1;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return 0;
}

static int is_matched_prev_vrr(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	int prev_vrr_val;
	char *endp;
	struct cmd_ref_state *state = &vdd->cmd_ref_state;

	prev_vrr_val = simple_strtol(val, &endp, 10);

	if (prev_vrr_val > 0 && prev_vrr_val != state->prev_refresh_rate)
		return 0;

	if (endp && strlen(endp) > 0) {
		if (!strncasecmp(endp, "NS", strlen(endp))) {
			if (state->prev_sot_hs || state->prev_sot_phs)
				return 0;
		} else if (!strncasecmp(endp, "HS", strlen(endp))) {
			if (!state->prev_sot_hs || state->prev_sot_phs)
				return 0;
		} else if (!strncasecmp(endp, "PHS", strlen(endp))) {
			if (!state->prev_sot_phs)
				return 0;
		} else {
			LCD_ERR(vdd, "invalid VRR input[%s]\n", val);
			return -EINVAL;
		}
	}

	return 1;
}

static int is_matched_vrr(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	int vrr_val;
	char *endp;
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	struct vrr_info *vrr = &vdd->vrr;

	vrr_val = simple_strtol(val, &endp, 10);

	if (vrr_val > 0 && vrr_val != state->cur_refresh_rate)
		return 0;

	if (endp && strlen(endp) > 0) {
		if (!strncasecmp(endp, "NS", strlen(endp))) {
			if (state->sot_hs || state->sot_phs)
				return 0;
		} else if (!strncasecmp(endp, "HS", strlen(endp))) {
			if (!state->sot_hs || state->sot_phs)
				return 0;
		} else if (!strncasecmp(endp, "PHS", strlen(endp))) {
			if (!state->sot_phs)
				return 0;
		} else if (!strncasecmp(endp, "CHANGING", strlen(endp))) {
			if (!state->running_vrr)
				return 0;
		} else if (!strncasecmp(endp, "UPDATE", strlen(endp))) {
			if (!(state->running_lfd || state->running_vrr || state->need_vrr_update)) {
				return 0;
			} else {
				/* Once vrr/lfd is updated with the need_vrr_update flag, this flag is disabled again. */
				if (state->need_vrr_update)
					vrr->need_vrr_update = false;
			}
		} else {
			LCD_ERR(vdd, "invalid VRR input[%s]\n", val);
			return -EINVAL;
		}
	}

	return 1;
}

static bool is_matched_lfd_hz(struct samsung_display_driver_data *vdd,
				char *val, int lfd_fps)
{
	char *endp, *endp2;
	int val1, val2, parse_val;

	val1 = val2 = parse_val = 0;

	val1 = simple_strtol(val, &endp, 10); /* Integer part */
	if (endp && !strncasecmp(endp, ".", 1)) { /* Decimal part */
		val2 = simple_strtol(endp + 1, &endp2, 10);
	}

	parse_val = val1 * 10 + val2;

	LCD_DEBUG(vdd, "lfd_in (%d) , lfd (%d)\n", parse_val, lfd_fps);

	if (parse_val == lfd_fps)
		return 1;

	return 0;
}

static int is_matched_lfdmax(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool ret;

	if (!vdd->vrr.lfd.support_lfd) {
		LCD_ERR(vdd, "LFD not supported\n");
		return 0;
	}

	ret = is_matched_lfd_hz(vdd, val, state->lfd_max_fps);

	return ret;
}

static int is_matched_lfdmin(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool ret;

	if (!vdd->vrr.lfd.support_lfd) {
		LCD_ERR(vdd, "LFD not supported\n");
		return 0;
	}

	ret = is_matched_lfd_hz(vdd, val, state->lfd_min_fps);

	return ret;
}

static int is_matched_lfdfix(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int lfd_val;
	bool ret;

	if (!vdd->vrr.lfd.support_lfd) {
		LCD_ERR(vdd, "LFD not supported\n");
		return 0;
	}

	if (kstrtoint(val, 10, &lfd_val)) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	ret = (lfd_val == state->lfd_fix) ? true : false;

	return ret;
}

static int is_matched_clock(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int clk_rate_MHz;

	if (kstrtoint(val, 10, &clk_rate_MHz)) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	if (state->clk_rate_MHz == clk_rate_MHz)
		return 1;
	else
		return 0;
}

static int is_matched_clock_id(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int clk_idx;

	if (kstrtoint(val, 10, &clk_idx)) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	if (clk_idx == state->clk_idx)
		return 1;
	else
		return 0;
}

static int is_matched_osc_id(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int osc_idx;

	if (kstrtoint(val, 10, &osc_idx)) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	if (osc_idx == state->osc_idx)
		return 1;
	else
		return 0;
}

static int is_matched_temperature(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int temperature = 0;

	if (kstrtoint(val, 10, &temperature)) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	if (temperature == state->temperature)
		return 1;
	else
		return 0;
}

static int is_matched_resolution(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int i;

	for (i = 0; i < vdd->res_count; i++)
		if (!strncasecmp(val, vdd->res[i].name, strlen(val)) &&
				vdd->res[i].width == state->h_active &&
				vdd->res[i].height == state->v_active)
			return 1;

	return 0;
}

static int is_matched_irc(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;

	if (!strncasecmp(val, "MODERATO", strlen(val))) {
		if (state->irc_mode == IRC_MODERATO_MODE)
			return 1;
	} else if (!strncasecmp(val, "FLAT", strlen(val))) {
		if (state->irc_mode == IRC_FLAT_GAMMA_MODE)
			return 1;
	} else if (!strncasecmp(val, "FLAT_Z", strlen(val))) {
		if (state->irc_mode == IRC_FLAT_Z_GAMMA_MODE)
			return 1;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return 0;
}

static int is_matched_mode(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool is_hlpm = state->lpm_ongoing;

	if (!strncasecmp(val, "HLPM", strlen(val))) {
		if (is_hlpm)
			return 1;
	} else if (!strncasecmp(val, "HMT", strlen(val))) {
		if (state->hmt_on)
			return 1;
	} else if (!strncasecmp(val, "HBM", strlen(val))) {
		if (state->is_hbm && !state->hmt_on && !is_hlpm)
			return 1;
	} else if (!strncasecmp(val, "HBM+", strlen(val))) {
		if (state->is_hbm_plus && !state->hmt_on && !is_hlpm)
			return 1;
	} else if (!strncasecmp(val, "NORMAL", strlen(val))) {
		if (!state->is_hbm && !state->hmt_on && !is_hlpm)
			return 1;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return 0;
}

/* if gradual_acl_val = 2, always 30% 0x03
 * else,
 * 	normal BL && gradual_acl_val = 3: 15%, 0x03
 * 	normal BL && gradual_acl_val = other: 8%, 0x01
 * 	hbm: 8% 0x01
 * input val: "8%", "15%", "30%", "ON", or "OFF".
 */
static int is_matched_acl(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int acl_per;
	int cur_acl_per;
	char *endp;

	if (state->gradual_acl_val  == 2) {
		cur_acl_per = 30; /* 30% */
	} else {
		if (state->is_hbm) {
			cur_acl_per = 8; /* HBM 8% */
		} else {
			if (state->gradual_acl_val   == 3)
				cur_acl_per = 15;
			else
				cur_acl_per = 8;
		}
	}

	acl_per = simple_strtol(val, &endp, 10);

	/* input val: "8%", "15%", or "30%". */
	if (acl_per > 0) {
		if (acl_per == cur_acl_per)
			return 1;
		else
			return 0;
	}

	/* input val: "ON", or "OFF" */
	if (!strncasecmp(val, "ON", strlen(val))) {
		if (state->acl_on)
			return 1;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		if (!state->acl_on)
			return 1;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return 0;
}

static int is_matched_acl_mode(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int req_acl_mode = 0;

	if (kstrtoint(val, 10, &req_acl_mode)) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (req_acl_mode == state->gradual_acl_val);
}

static int is_matched_acl_changed(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int ret = 0;
	bool req = false;

	if (!strncasecmp(val, "TRUE", strlen(val))) {
		req = true;
		if (state->gradual_acl_changed)
			ret = 1;
		else
			ret = 0;
	} else if (!strncasecmp(val, "FALSE", strlen(val))) {
		req = false;
		if (state->gradual_acl_changed)
			ret = 0;
		else
			ret = 1;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	LCD_DEBUG(vdd, "ret = %d\n", !(req == state->gradual_acl_changed));

	return ret;
}

static int is_matched_display_on(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool req_display_on;

	if (!strncasecmp(val, "ON", strlen(val)))
		req_display_on = true;
	else
		req_display_on = false;

	if (state->display_on == req_display_on)
		return 1;

	return 0;
}

static int is_matched_smooth_dim(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool req, dim_off = false;

	if (!strncasecmp(val, "ON", strlen(val))) {
		req = false;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		req = true;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	/* If user brightness comes down from the userlevel (fw) after DISPLAY_ON,
	 * apply SMOOTH_DIM OFF for that brightness setting.
	 */

	LCD_DEBUG(vdd, "%d %d \n", vdd->br_info.common_br.prev_bl_level, vdd->br_info.common_br.bl_level);

	/* SMOOTH DIM OFF before display on */
	if (!vdd->display_on)
		dim_off = true;

	if (state->first_high_level)
		dim_off = true;

	/* SMOOTH DIM OFF control by sysfs */
	if (state->smooth_dim_off)
		dim_off = true;

	/* SMOOTH DIM ON */
	return (req == dim_off);
}

static int is_matched_dia(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool req_dia_off;

	/* input val: "ON", or "OFF" */
	if (!strncasecmp(val, "ON", strlen(val))) {
		req_dia_off = false;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		req_dia_off = true;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (req_dia_off == state->dia_off);
}

static int is_matched_mafpc(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool req;

	/* input val: "ON", or "OFF" */
	if (!strncasecmp(val, "ON", strlen(val))) {
		req = false;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		req = true;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (req == state->mafpc_en);
}

static int is_matched_night_dim(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool req_night_dim;

	/* input val: "ON", or "OFF" */
	if (!strncasecmp(val, "ON", strlen(val))) {
		req_night_dim = true;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		req_night_dim = false;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (req_night_dim == state->night_dim);
}

static int is_matched_early_te(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool early_te;

	/* input val: "ON", or "OFF" */
	if (!strncasecmp(val, "ON", strlen(val))) {
		early_te = true;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		early_te = false;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (early_te == state->early_te);
}

static int is_matched_finger_mask(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool finger_mask_updated;

	/* input val: "UPDATED" */
	if (!strncasecmp(val, "UPDATED", strlen(val))) {
		finger_mask_updated = true;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (finger_mask_updated == state->finger_mask_updated);
}

static int is_matched_local_hbm_using(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	bool req_local_hbm_using;

	if (!strncasecmp(val, "ON", strlen(val))) {
		req_local_hbm_using = true;
	} else if (!strncasecmp(val, "OFF", strlen(val))) {
		req_local_hbm_using = false;
	} else {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	return (req_local_hbm_using == state->local_hbm_using);
}

/* HLPM brightness: 60nit 30nit 10nit 2nit */
static int is_matched_hlpmbrightness(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int cd;
	char *endp;

	cd = simple_strtol(val, &endp, 10);
	if (!(endp && strlen(endp) > 0 && !strncasecmp(endp, "nit", strlen(endp)))) {
		LCD_ERR(vdd, "invalid val(%s)\n", val);
		return -EINVAL;
	}

	if (cd == state->cd_level)
		return 1;

	return 0;
}

/* Check DDI reivision range.
 * ex) IF REV AtoB THEN APPLY , IF REV CtoZ THEN 0x01 0x02, ...
 */
static int is_matched_ddi_rev(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int panel_revision = state->panel_revision;
	char rev_from, rev_to;

	if (strlen(val) != 4 || strncasecmp(val + 1, "to", 2))
		goto input_err;

	rev_from = val[0];
	rev_to = val[3];

	if (rev_from < 'A' || rev_from > 'Z' || rev_to < 'A' || rev_to > 'Z')
		goto input_err;

	if (rev_from - 'A' <= panel_revision && panel_revision <= rev_to - 'A')
		return 1;

	return 0;

input_err:
	LCD_ERR(vdd, "invalid input, len: %lu, val: [%s]\n", strlen(val), val);

	return -EINVAL;
}

static int update_tset(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int temperature = state->temperature;
	int i = -1;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	cmd->txbuf[i] = temperature > 0 ?  temperature : BIT(7) | (-temperature);

	return 0;
}

int save_otp(struct samsung_display_driver_data *vdd)
{
	struct otp_info *otp = vdd->otp;
	int i;

	if (vdd->otp_init_done || vdd->otp_len == 0) {
		LCD_DEBUG(vdd, "No need otp read (len:%d, done:%d)\n",
				vdd->otp_len, vdd->otp_init_done);
		return 0;
	}

	/* read from ddi and save */
	for (i = 0; i < vdd->otp_len; i++) {
		ss_read_mtp(vdd, otp[i].addr, 1, otp[i].offset, &otp[i].val);
		LCD_INFO(vdd, "[%d] addr: %x, off: %x, val: %x\n",
				i, otp[i].addr, otp[i].offset, otp[i].val);
	}

	vdd->otp_init_done = true;

	return 0;
}

static void prep_otp(struct samsung_display_driver_data *vdd,
			u8 addr, u16 offset)
{
	int i;

	for (i = 0; i < vdd->otp_len; i++)
		if (vdd->otp[i].addr == addr && vdd->otp[i].offset == offset)
			return;

	if (vdd->otp_len >= MAX_OTP_LEN) {
		LCD_ERR(vdd, "over MAX_OTP_LEN, len: %d\n", vdd->otp_len);
		return;
	}

	vdd->otp[vdd->otp_len].addr = addr;
	vdd->otp[vdd->otp_len].offset = offset;
	vdd->otp_len++;

	LCD_INFO(vdd, "otp_len: %d, addr: %x, off: %x\n", vdd->otp_len, addr, offset);
}

int ss_prepare_otp(struct samsung_display_driver_data *vdd)
{
	struct ss_cmd_set *set;
	struct ss_cmd_op_str *op_root;
	int type, i, j;

	for (type = SS_DSI_CMD_SET_START; type <= SS_DSI_CMD_SET_MAX ; type++) {
		/* To find UPDATE OTP, check only op_root */
		set = ss_get_ss_cmds(vdd, type);
		for (i = 0; i < set->count; i++) {
			list_for_each_entry(op_root, &set->cmds[i].op_list, list) {
				if (op_root->op == SS_CMD_OP_UPDATE &&
						strlen(op_root->symbol) == 3 &&
						!strncasecmp(op_root->symbol, "OTP", 3)) {
					u8 addr = set->cmds[i].txbuf[0];
					u16 offset = 0;
					struct ss_cmd_desc *cmd_gpara = i > 0 ? &set->cmds[i - 1] : NULL;

					if (cmd_gpara && cmd_gpara->txbuf[0] == 0xB0 &&
							cmd_gpara->txbuf[cmd_gpara->tx_len - 1] == addr) {
						if (cmd_gpara->tx_len == 4) // two point gpara
							offset = (cmd_gpara->txbuf[1] << 8) | cmd_gpara->txbuf[2];
						else
							offset = cmd_gpara->txbuf[1];
					}

					for (j = 0; j < set->cmds[i].tx_len; j++)
						if (set->cmds[i].pos_0xXX[j])
							prep_otp(vdd, addr, offset + j - 1);
				}
			}
		}
	}

	return 0;
}

u8 get_otp(struct samsung_display_driver_data *vdd, u8 addr, u16 offset)
{
	int i;

	for (i = 0; i < vdd->otp_len; i++)
		if (vdd->otp[i].addr == addr && vdd->otp[i].offset == offset)
			return vdd->otp[i].val;

	LCD_ERR(vdd, "fail to find otp for addr %x off %x\n", addr, offset);
	return 0;
}

static int update_otp(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	/* TODO how to check cmd_gpara is valid??? check cmd's idx in set?? */
	struct ss_cmd_desc *cmd_gpara = cmd - 1;
	u8 addr = cmd->txbuf[0];
	u16 offset = 0;
	int i;

	for (i = 1; i < cmd->tx_len; i++) {
		if (!cmd->pos_0xXX[i])
			continue;

		if (cmd_gpara->txbuf[0] == 0xB0 &&
				cmd_gpara->txbuf[cmd_gpara->tx_len - 1] == addr) {
			if (cmd_gpara->tx_len == 4) /* two point gpara */
				offset = (cmd_gpara->txbuf[1] << 8) | cmd_gpara->txbuf[2];
			else
				offset = cmd_gpara->txbuf[1];
		}

		cmd->txbuf[i] = get_otp(vdd, addr, offset + i - 1);
	}

	return 0;
}

static int update_brightness(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	int wrdisbv = vdd->br_info.common_br.gm2_wrdisbv;
	int i = -1;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	if (i + 1 >= cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position\n");
		return -EINVAL;
	}

	LCD_DEBUG(vdd, "gm2_wrdisbv : 0x%x\n", wrdisbv);

	cmd->txbuf[i] = (wrdisbv >> 8) & 0xFF;
	cmd->txbuf[i + 1] = wrdisbv & 0xFF;

	return 0;
}

static int update_acl_offset(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_legoop_map *acl_offset_map = NULL;
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int bl_lvl = state->bl_level;
	int i = -1;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	acl_offset_map = &vdd->br_info.acl_offset_map_table[vdd->panel_revision];

	if (bl_lvl >= acl_offset_map->row_size) {
		LCD_ERR(vdd, "invalid bl_lvl (%d) >= row_size(%d)\n",
				bl_lvl, acl_offset_map->row_size);
		return -EINVAL;
	}

	if (acl_offset_map->cmds) {
		LCD_DEBUG(vdd, "acl_offset : 0x%x\n", acl_offset_map->cmds[bl_lvl][0]);
		cmd->txbuf[i] = acl_offset_map->cmds[bl_lvl][0];
	}

	return 0;
}

static int update_irc_offset(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_legoop_map *irc_offset_map = NULL;
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int bl_lvl = state->bl_level;
	int i = -1;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	irc_offset_map = &vdd->br_info.irc_offset_map_table[vdd->panel_revision];

	if (bl_lvl >= irc_offset_map->row_size) {
		LCD_ERR(vdd, "invalid bl_lvl (%d) >= row_size(%d)\n",
				bl_lvl, irc_offset_map->row_size);
		return -EINVAL;
	}

	if (irc_offset_map->cmds) {
		LCD_DEBUG(vdd, "irc_offset : 0x%x\n", irc_offset_map->cmds[bl_lvl][0]);
		cmd->txbuf[i] = irc_offset_map->cmds[bl_lvl][0];
	}

	return 0;
}

static int update_local_hbm_compensation(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	int i = 0;

	for (i = 0; i < vdd->local_hbm_compensation_len; i++)
		cmd->txbuf[i+1] = vdd->local_hbm_compensation_value[i];

	return 0;
}

static int is_matched_analog_offset_on(struct samsung_display_driver_data *vdd,
				char *val, struct ss_cmd_desc *cmd)
{
	bool ret = false;

	if (vdd->panel_func.analog_offset_on)
		ret = vdd->panel_func.analog_offset_on(vdd);

	return ret;
}

int register_common_op_sym_cb(struct samsung_display_driver_data *vdd)
{
	register_op_sym_cb(vdd, "BUILD", is_matched_build, false);
	register_op_sym_cb(vdd, "VRR", is_matched_vrr, false);
	register_op_sym_cb(vdd, "PREV_VRR", is_matched_prev_vrr, false);
	register_op_sym_cb(vdd, "LFDMAX", is_matched_lfdmax, false);
	register_op_sym_cb(vdd, "LFDMIN", is_matched_lfdmin, false);
	register_op_sym_cb(vdd, "LFDFIX", is_matched_lfdfix, false);
	register_op_sym_cb(vdd, "CLOCK", is_matched_clock, false);
	register_op_sym_cb(vdd, "CLOCK_ID", is_matched_clock_id, false);
	register_op_sym_cb(vdd, "OSC_ID", is_matched_osc_id, false);
	register_op_sym_cb(vdd, "TEMPERATURE", is_matched_temperature, false);
	register_op_sym_cb(vdd, "RESOLUTION", is_matched_resolution, false);
	register_op_sym_cb(vdd, "IRC", is_matched_irc, false);
	register_op_sym_cb(vdd, "MODE", is_matched_mode, false);
	register_op_sym_cb(vdd, "ACL", is_matched_acl, false);
	register_op_sym_cb(vdd, "ACL_MODE", is_matched_acl_mode, false);
	register_op_sym_cb(vdd, "ACL_CHANGED", is_matched_acl_changed, false);
	register_op_sym_cb(vdd, "BRIGHTNESS", is_matched_hlpmbrightness, false);
	register_op_sym_cb(vdd, "DISPLAY_ON", is_matched_display_on, false);
	register_op_sym_cb(vdd, "SMOOTH_DIM", is_matched_smooth_dim, false);
	register_op_sym_cb(vdd, "DIA", is_matched_dia, false);
	register_op_sym_cb(vdd, "NIGHT_DIM", is_matched_night_dim, false);
	register_op_sym_cb(vdd, "EARLY_TE", is_matched_early_te, false);
	register_op_sym_cb(vdd, "FINGER_MASK", is_matched_finger_mask, false);
	register_op_sym_cb(vdd, "REVISION", is_matched_ddi_rev, false);
	register_op_sym_cb(vdd, "ANALOG_OFFSET", is_matched_analog_offset_on, false);
	register_op_sym_cb(vdd, "MAFPC", is_matched_mafpc, false);
	register_op_sym_cb(vdd, "LOCAL_HBM_USING", is_matched_local_hbm_using, false);

	/* common UPDATE symbol */
	register_op_sym_cb(vdd, "BRIGHTNESS", update_brightness, true);
	register_op_sym_cb(vdd, "TSET", update_tset, true);
	register_op_sym_cb(vdd, "OTP", update_otp, true);
	register_op_sym_cb(vdd, "ACL_OFFSET", update_acl_offset, true);
	register_op_sym_cb(vdd, "IRC_OFFSET", update_irc_offset, true);
	register_op_sym_cb(vdd, "LOCAL_HBM_COMPENSATION", update_local_hbm_compensation, true);

	return 0;
}

/* search symbol lookup table and get callback function,
 * then call it.
 */
static int ss_wrapper_op_process(struct samsung_display_driver_data *vdd,
				char *symbol, char *val,
				struct ss_cmd_desc *cmd, bool op_update)
{
	int i;

	for (i = 0; i < vdd->sym_cb_tbl_size; i++) {
		if (op_update == vdd->sym_cb_tbl[i].op_update &&
			strlen(symbol) == strlen(vdd->sym_cb_tbl[i].symbol) &&
			!strncasecmp(symbol, vdd->sym_cb_tbl[i].symbol, strlen(symbol)))
			return vdd->sym_cb_tbl[i].cb(vdd, val, cmd);
	}

	LCD_ERR(vdd, "fail to find callback for symbol(%s), update: %d\n",
			symbol, op_update);

	for (i = 0; i < vdd->sym_cb_tbl_size; i++)
		LCD_INFO(vdd, "[%d] symbol: [%s], update: %d\n", i,
				vdd->sym_cb_tbl[i].symbol, vdd->sym_cb_tbl[i].op_update);

	return -EINVAL;
}

/* return true if 'IF_BLOCK' condition is true */
static bool ss_cmd_op_process_ifblock(struct samsung_display_driver_data *vdd,
			struct ss_cmd_op_str *op_root, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_op_str *op_sibling;
	int is_true;


	/* check op_root */
	is_true = ss_wrapper_op_process(vdd,
			op_root->symbol, op_root->val, cmd, false);
	if (is_true < 0) /* error case */
		goto err;

	if (op_root->cond == SS_CMD_OP_COND_AND && !is_true)
		goto end_false;
	else if (op_root->cond == SS_CMD_OP_COND_OR && is_true)
		return true;

	/* check op_sibling */
	list_for_each_entry(op_sibling, &op_root->sibling, sibling) {
		is_true = ss_wrapper_op_process(vdd,
				op_sibling->symbol, op_sibling->val, cmd, false);
		if (is_true < 0) /* error case */
			goto err;

		if (op_root->cond == SS_CMD_OP_COND_AND && !is_true)
			goto end_false;
		else if (op_root->cond == SS_CMD_OP_COND_OR && is_true)
			return true;
	}

	if (op_root->cond == SS_CMD_OP_COND_AND)
		return true;

end_false:
err:
	cmd->skip_by_cond = true;
	return false;
}

/* return true if 'IF' condition is true */
static bool ss_cmd_op_process_if(struct samsung_display_driver_data *vdd,
		struct ss_cmd_op_str *op_root, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_op_str *op_sibling;
	int is_true;

	/* check op_root */
	is_true = ss_wrapper_op_process(vdd,
			op_root->symbol, op_root->val, cmd, false);
	if (is_true < 0) /* error case */
		goto err;

	if (op_root->cond == SS_CMD_OP_COND_AND && !is_true)
		return false;
	else if (op_root->cond == SS_CMD_OP_COND_OR && is_true)
		goto end_true;

	/* check op_sibling */
	list_for_each_entry(op_sibling, &op_root->sibling, sibling) {
		is_true = ss_wrapper_op_process(vdd,
				op_sibling->symbol, op_sibling->val, cmd, false);
		if (is_true < 0) /* error case */
			goto err;

		if (op_root->cond == SS_CMD_OP_COND_AND && !is_true)
			return false;
		else if (op_root->cond == SS_CMD_OP_COND_OR && is_true)
			goto end_true;
	}

	if (op_root->cond == SS_CMD_OP_COND_OR)
		return false;

end_true:
	/* replace with candidate cmd */
	cmd->txbuf = op_root->candidate_buf;
	cmd->qc_cmd->msg.tx_buf = cmd->txbuf;

	return true;

err:
	cmd->skip_by_cond = true;
	return false;
}

static void ss_cmd_op_process_else(struct samsung_display_driver_data *vdd,
		struct ss_cmd_op_str *op_root, struct ss_cmd_desc *cmd)
{
	/* replace with candidate cmd */
	cmd->txbuf = op_root->candidate_buf;
	cmd->qc_cmd->msg.tx_buf = cmd->txbuf;
}

static void ss_cmd_op_process_update(struct samsung_display_driver_data *vdd,
		struct ss_cmd_op_str *op, struct ss_cmd_desc *cmd)
{
	int ret;

	ret = ss_wrapper_op_process(vdd, op->symbol, op->val, cmd, true);
	if (ret < 0)
		cmd->skip_by_cond = true;
}

static int ss_cmd_op_process(struct samsung_display_driver_data *vdd,
				struct ss_cmd_desc *cmd)
{
	struct ss_cmd_op_str *op_root;
	bool flag_if_no_matched_cond = false;

	if (list_empty(&cmd->op_list))
		return 0;

	list_for_each_entry(op_root, &cmd->op_list, list) {
		switch (op_root->op) {
		case SS_CMD_OP_IF_BLOCK:
			if (!ss_cmd_op_process_ifblock(vdd, op_root, cmd))
				goto end;
			break;
		case SS_CMD_OP_IF:
			flag_if_no_matched_cond = true;
			if (ss_cmd_op_process_if(vdd, op_root, cmd))
				goto end;
			break;
		case SS_CMD_OP_ELSE:
			ss_cmd_op_process_else(vdd, op_root, cmd);
			goto end;
		case SS_CMD_OP_UPDATE:
			ss_cmd_op_process_update(vdd, op_root, cmd);
			goto end;
		default:
			break;
		}
	}

	if (flag_if_no_matched_cond)
		LCD_ERR(vdd, "no IF matched cmd: tx_len: %zd, txbuf[0]: 0x%X\n",
				cmd->tx_len, cmd->txbuf[0]);
end:
	return 0;
}


static void ss_op_copy_state(struct samsung_display_driver_data *vdd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	struct vrr_info *vrr = &vdd->vrr;
	struct dvrr_info *dvrr = &vdd->dvrr;
	struct brightness_info *br = &vdd->br_info.common_br;
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);
	enum LFD_SCOPE_ID scope;
	struct lfd_div_info div_info = {0,};
	int normal_max_lv = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].max_lv;
	int normal_tab_size = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].tab_size;
	int hbm_plus_lv = vdd->br_info.hbm_plus_level;

	if (!display)
		LCD_ERR(vdd, "display null\n");

	if (!panel)
		LCD_ERR(vdd, "panel null\n");

	state->temperature = vdd->br_info.temperature;

	state->cur_refresh_rate = vrr->cur_refresh_rate;
	state->sot_hs = vrr->cur_sot_hs_mode;
	state->sot_phs = vrr->cur_phs_mode;
	state->prev_refresh_rate = vrr->prev_refresh_rate;
	state->prev_sot_hs = vrr->prev_sot_hs_mode;
	state->prev_sot_phs = vrr->prev_phs_mode;
	state->running_vrr = vrr->running_vrr;
	state->running_lfd  = vrr->lfd.running_lfd;
	state->need_vrr_update = vrr->need_vrr_update;

	state->lpm_ongoing = ss_is_panel_lpm_ongoing(vdd);
	state->hmt_on = vdd->hmt.hmt_on;
	state->is_hbm = br->bl_level > normal_max_lv;
	if (hbm_plus_lv)
		state->is_hbm_plus = br->bl_level >= hbm_plus_lv;

	state->bl_level = br->bl_level;
	state->cd_level = br->cd_level;
	state->bl_idx = br->bl_idx;
	if (state->is_hbm)
		state->bl_idx += normal_tab_size;

	if (vrr->lfd.support_lfd) {
		if (state->lpm_ongoing)
			scope = LFD_SCOPE_LPM;
		else if (vdd->hmt.hmt_on)
			scope = LFD_SCOPE_HMD;
		else
			scope = LFD_SCOPE_NORMAL;

		ss_get_lfd_div(vdd, scope, &div_info);

		/* It is expressed by x10 to support decimal points */
		state->lfd_max_fps = div_info.max_div ? DIV_ROUND_UP(vrr->lfd.base_rr * 10, div_info.max_div) : 0;
		state->lfd_min_fps = div_info.min_div ? DIV_ROUND_UP(vrr->lfd.base_rr * 10, div_info.min_div) : 0;
		state->lfd_fix = div_info.fix;
	} else {
		state->lfd_max_fps = 0;
		state->lfd_min_fps = 0;
	}

	state->clk_rate_MHz = display ?  display->config.bit_clk_rate_hz / 1000000 : 0;
	state->clk_idx = vdd->dyn_mipi_clk.requested_clk_idx;
	state->osc_idx = vdd->dyn_mipi_clk.requested_osc_idx;

	/* cannot use cur_h = vrr->cur_h_active;.
	 * it transmits switch command before update vrr->cur_h_active.
	 */
	if (panel && panel->cur_mode) {
		state->h_active = panel->cur_mode->timing.h_active;
		state->v_active = panel->cur_mode->timing.v_active;
	} else {
		state->h_active = 0;
		state->v_active = 0;
	}

	state->irc_mode = vdd->br_info.common_br.irc_mode;
	if (vdd->br_info.gradual_acl_val != state->gradual_acl_val)
		state->gradual_acl_changed = true;
	else
		state->gradual_acl_changed = false;
	state->gradual_acl_val = vdd->br_info.gradual_acl_val;
	state->acl_on = is_acl_on(vdd);
	state->smooth_dim_off = vdd->br_info.common_br.smooth_dim_off;
	state->display_on = vdd->display_on;
	state->first_high_level = (!vdd->br_info.common_br.prev_bl_level && br->bl_level && vdd->display_on);
	state->dia_off = vdd->dia_off;
	state->night_dim = vdd->night_dim;
	state->early_te = vdd->early_te;
	state->finger_mask_updated = vdd->finger_mask_updated;
	state->panel_revision = vdd->panel_revision;
	state->is_factory_mode = vdd->is_factory_mode;
	state->mafpc_en = vdd->mafpc.en;
	state->local_hbm_using = vdd->local_hbm_val;

	/* DVRR Related */
	state->cur_frame_interval = dvrr->cur_frame_interval;
	state->prev_frame_interval = dvrr->prev_frame_interval;
	state->dvrr_update = dvrr->dvrr_update;

	if (dvrr->support_dvrr) {
		LCD_INFO(vdd, "%dx%d@%d, bl_level:%d, cd:%dnit, smooth_dim:%s, " \
			"LFD: %u.%.1uhz(%u)~%u.%.1dhz(%u), fix(%d)" \
			"dsi clk: %lluMHZ, clkid: %d, oscid: %d, irc: %s, mode: %s, " \
			"acl: %s(%d)(%d), temp: %d, " \
			"night_dim: %d, first_high: %d, " \
			"mafpc: %s, dia_off: %d, finger_mask_updated=%d, display_on: %d, rev: %c\n",
			state->h_active, state->v_active,
			state->cur_frame_interval,
			br->bl_level, state->cd_level, state->smooth_dim_off ?
"OFF" : "ON",
			GET_LFD_INT_PART(vrr->lfd.base_rr, div_info.min_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, div_info.min_div), div_info.min_div,
			GET_LFD_INT_PART(vrr->lfd.base_rr, div_info.max_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, div_info.max_div), div_info.max_div,
			div_info.fix,
			state->clk_rate_MHz, state->clk_idx, state->osc_idx,
			state->irc_mode == IRC_MODERATO_MODE ?
				"MODERATO" : "FLAT",
			state->lpm_ongoing ? "LPM" :
				state->hmt_on ? "HMT" :
				state->is_hbm_plus ? "HBM+" :
				state->is_hbm ? "HBM" : "NORMAL",
			state->acl_on ? "ON" : "OFF", state->gradual_acl_val, state->gradual_acl_changed,
			vdd->br_info.temperature, vdd->night_dim, state->first_high_level,
			state->mafpc_en ? "ON" : "OFF",
			state->dia_off, state->finger_mask_updated, state->display_on, state->panel_revision + 'A');
	} else {
		LCD_INFO(vdd, "cur info: %dx%d(%dx%d)@%d%s, bl_idx: %d, bl_level: %d, cd: %dnit, " \
			"LFD: %u.%.1uhz(%u)~%u.%.1dhz(%u), fix(%d), base_rr: %uhz, " \
			"vrr: %d, lfd: %d, " \
			"dsi clk: %lluMHZ, clkid: %d, oscid: %d, irc: %s, mode: %s, " \
			"acl: %s(%d), display_on: %d, smooth_dim_off: %d, temp: %d, " \
			"night_dim: %d, running_lpm: %d, first_high: %d, " \
			"dia_off: %d, night_dim: %d early_te: %d finger_mask_updated=%d, rev: %c\n",
			state->h_active, state->v_active,
			vrr->cur_h_active, vrr->cur_v_active,
			state->cur_refresh_rate,
			state->sot_phs ? "PHS" : state->sot_hs ? "HS" : "NS",
			state->bl_idx, br->bl_level, state->cd_level,
			GET_LFD_INT_PART(vrr->lfd.base_rr, div_info.min_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, div_info.min_div), div_info.min_div,
			GET_LFD_INT_PART(vrr->lfd.base_rr, div_info.max_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, div_info.max_div), div_info.max_div,
			div_info.fix, vrr->lfd.base_rr, state->running_vrr, state->running_lfd,
			state->clk_rate_MHz, state->clk_idx, state->osc_idx,
			state->irc_mode == IRC_MODERATO_MODE ?
				"MODERATO" : "FLAT",
			state->lpm_ongoing ? "LPM" :
				state->hmt_on ? "HMT" :
				state->is_hbm_plus ? "HBM+" :
				state->is_hbm ? "HBM" : "NORMAL",
			state->acl_on ? "ON" : "OFF", state->gradual_acl_val,
			state->display_on, state->smooth_dim_off, vdd->br_info.temperature,
			vdd->night_dim, vdd->panel_lpm.during_ctrl, state->first_high_level,
			state->dia_off, state->night_dim, state->early_te,
			state->finger_mask_updated, state->panel_revision + 'A');
	}
}

__visible_for_testing int ss_op_process(struct samsung_display_driver_data *vdd,
					struct ss_cmd_set *set)
{
	struct ss_cmd_desc *cmds;
	int i;

	/* copy states referred by cmd condition */
	ss_op_copy_state(vdd);

	for (i = 0, cmds = set->cmds; i < set->count; i++, cmds++) {
		cmds->skip_by_cond = false;
		ss_cmd_op_process(vdd, cmds);

		/* translate frame delay to ms, and update to qc cmd */
		if (cmds->post_wait_frame)
			cmds->qc_cmd->post_wait_ms =
				ss_frame_to_ms(vdd, cmds->post_wait_frame);

		/* if skip command with gpara, skip also gpara command */
		if (cmds->skip_by_cond && i > 0) {
			struct ss_cmd_desc *gpara = cmds - 1;

			if (!gpara->skip_by_cond && gpara->txbuf[0] == 0xB0 &&
					(!vdd->pointing_gpara ||
					 (vdd->pointing_gpara &&
					  gpara->txbuf[gpara->tx_len - 1] == cmds->txbuf[0])))
				gpara->skip_by_cond = true;
		}
	}

	return 0;
}

bool is_level0_key(struct samsung_display_driver_data *vdd, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_set *set_on = ss_get_ss_cmds(vdd, TX_LEVEL0_KEY_ENABLE);
	struct ss_cmd_set *set_off = ss_get_ss_cmds(vdd, TX_LEVEL0_KEY_DISABLE);

	if (SS_IS_SS_CMDS_NULL(set_on) || SS_IS_SS_CMDS_NULL(set_off)) {
		LCD_INFO_ONCE(vdd, "ref level cmds is NULL\n");
		return false;
	}

	if (cmd->tx_len == set_on->cmds[0].tx_len &&
			!memcmp(cmd->txbuf, set_on->cmds[0].txbuf, cmd->tx_len))
		return true;

	if (cmd->tx_len == set_off->cmds[0].tx_len &&
			!memcmp(cmd->txbuf, set_off->cmds[0].txbuf, cmd->tx_len))
		return true;

	return false;
}

bool is_level1_key(struct samsung_display_driver_data *vdd, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_set *set_on = ss_get_ss_cmds(vdd, TX_LEVEL1_KEY_ENABLE);
	struct ss_cmd_set *set_off = ss_get_ss_cmds(vdd, TX_LEVEL1_KEY_DISABLE);

	if (SS_IS_SS_CMDS_NULL(set_on) || SS_IS_SS_CMDS_NULL(set_off)) {
		LCD_INFO_ONCE(vdd, "ref level cmds is NULL\n");
		return false;
	}

	if (cmd->tx_len == set_on->cmds[0].tx_len &&
			!memcmp(cmd->txbuf, set_on->cmds[0].txbuf, cmd->tx_len))
		return true;

	if (cmd->tx_len == set_off->cmds[0].tx_len &&
			!memcmp(cmd->txbuf, set_off->cmds[0].txbuf, cmd->tx_len))
		return true;

	return false;
}


bool is_level2_key(struct samsung_display_driver_data *vdd, struct ss_cmd_desc *cmd)
{
	struct ss_cmd_set *set_on = ss_get_ss_cmds(vdd, TX_LEVEL2_KEY_ENABLE);
	struct ss_cmd_set *set_off = ss_get_ss_cmds(vdd, TX_LEVEL2_KEY_DISABLE);

	if (SS_IS_SS_CMDS_NULL(set_on) || SS_IS_SS_CMDS_NULL(set_off)) {
		LCD_INFO_ONCE(vdd, "ref level cmds is NULL\n");
		return false;
	}

	if (cmd->tx_len == set_on->cmds[0].tx_len &&
			!memcmp(cmd->txbuf, set_on->cmds[0].txbuf, cmd->tx_len))
		return true;

	if (cmd->tx_len == set_off->cmds[0].tx_len &&
			!memcmp(cmd->txbuf, set_off->cmds[0].txbuf, cmd->tx_len))
		return true;

	return false;
}

int get_level_key(struct samsung_display_driver_data *vdd, struct ss_cmd_desc *cmd)
{
	int level_key = 0;

	if (is_level0_key(vdd, cmd))
		level_key |= LEVEL0_KEY;

	if (is_level1_key(vdd, cmd))
		level_key |= LEVEL1_KEY;

	if (is_level2_key(vdd, cmd))
		level_key |= LEVEL2_KEY;

	return level_key;
}

/* Skip level key commands in middle of commands.
 * Level key commands will be send only once prior to other commands.
 */
static int ss_tot_level_key(struct samsung_display_driver_data *vdd,
					struct ss_cmd_set *set)
{
	struct ss_cmd_desc *cmds;
	int i;
	int tot_level_key = 0;

	if (!vdd->allow_level_key_once || set->count < 2 || set->is_skip_tot_lvl) {
		LCD_DEBUG(vdd, "tot level key is not supported");
		return 0;
	}

	for (i = 0, cmds = set->cmds; i < set->count; i++, cmds++) {
		/* assume that level key commands sent without gpara */
		int level_key = get_level_key(vdd, cmds);
		if (level_key)
			cmds->skip_lvl_key = true;

		tot_level_key |= level_key;
	}

	return tot_level_key;
}

int ss_is_queue_cmd_waits_type(int type)
{

	if (type == TX_SS_SMOOTH_BR_DONE || type == TX_SS_BRIGHT_CTRL)
		return true;

	return 0;
}

int __mockable ss_wrapper_dsi_queue_cmd_waits(struct samsung_display_driver_data *vdd, bool enable)
{
	struct dsi_display *display = NULL;
	struct sde_connector *conn = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: fail to get vdd\n", __func__);
		return -ENODEV;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_ERR(vdd, "display is NULL\n");
		return -ENODEV;
	}

	conn = GET_SDE_CONNECTOR(vdd);
	if (IS_ERR_OR_NULL(conn)) {
		LCD_ERR(vdd, "conn is NULL\n");
		return -ENODEV;
	}

	if (enable)
		display->queue_cmd_waits = true;
	else {
		if (!conn->pre_kickoff_progress)
			display->queue_cmd_waits = false;
	}

	return 0;
}

int __mockable ss_wrapper_dsi_display_clk_ctrl(struct samsung_display_driver_data *vdd, int enable)
{
	int ret = 0;
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: fail to get vdd\n", __func__);
		return -ENODEV;
	}

	if (enable) {
		if (dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_CORE_CLK | DSI_LINK_CLK, DSI_CLK_ON))
			return -EBUSY;
	} else {
		if (dsi_display_clk_ctrl(display->dsi_clk_handle, DSI_CORE_CLK | DSI_LINK_CLK, DSI_CLK_OFF))
			return -EBUSY;
	}

	return ret;

}

int __mockable ss_wrapper_dsi_display_lock(struct samsung_display_driver_data *vdd, bool enable)
{
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);

	if (enable)
		mutex_lock(&display->display_lock);
	else
		mutex_unlock(&display->display_lock);

	return 0;
}

#if IS_ENABLED(CONFIG_ARCH_SUN) /* SM8750 */
int __mockable ss_wrapper_dsi_ctrl_setup_avr(struct samsung_display_driver_data *vdd, bool enable)
{
	int i;
	struct dsi_display *display = GET_DSI_DISPLAY(vdd);

	display_for_each_ctrl(i, display)
		dsi_ctrl_setup_avr(display->ctrl[i].ctrl, enable);

	LCD_INFO(vdd, "dsi_ctrl_setup_avr %s\n", enable ? "Enable" : "Disable");

	return 0;
}

int __mockable ss_wrapper_trigger_self_refresh(struct samsung_display_driver_data *vdd)
{
	struct drm_connector *connector = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: fail to get vdd\n", __func__);
		return -ENODEV;
	}

	connector = GET_DRM_CONNECTORS(vdd);
	if (IS_ERR_OR_NULL(connector)) {
		pr_err("%s: fail to get drm_connector\n", __func__);
		return -ENODEV;
	}

	return sde_connector_trigger_cmd_self_refresh(connector);
}

int __mockable ss_wrapper_sde_encoder_enter_idle(struct samsung_display_driver_data *vdd, int enable)
{
	struct drm_encoder *drm_enc;
	struct drm_connector *connector = NULL;

	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: fail to get vdd\n", __func__);
		return -ENODEV;
	}

	connector = GET_DRM_CONNECTORS(vdd);
	if (IS_ERR_OR_NULL(connector)) {
		pr_err("%s: fail to get drm_connector\n", __func__);
		return -ENODEV;
	}

	if (connector->state && connector->state->best_encoder)
		drm_enc = connector->state->best_encoder;
	else
		drm_enc = connector->encoder;

	if (IS_ERR_OR_NULL(connector)) {
		pr_err("%s: fail to get drm_enc\n", __func__);
		return -ENODEV;
	}

	return sde_encoder_enter_idle(drm_enc, enable);
}
#else
int __mockable ss_wrapper_trigger_self_refresh(struct samsung_display_driver_data *vdd)
{
	return 0;
}

int __mockable ss_wrapper_dsi_ctrl_setup_avr(struct samsung_display_driver_data *vdd, bool enable)
{
	return 0;
}
int __mockable ss_wrapper_sde_encoder_enter_idle(struct samsung_display_driver_data *vdd, int enable)
{
	return 0;
}
#endif

int __mockable ss_wrapper_dsi_panel_tx_cmd_set(struct dsi_panel *panel, int type)
{
	int rc = 0;
	struct dsi_display *display = container_of(panel->host, struct dsi_display, host);
	struct samsung_display_driver_data *vdd = display->panel->panel_private;
	struct drm_encoder *drm_enc = GET_DRM_ENCODER(vdd);

	ss_op_process(vdd, ss_get_ss_cmds(vdd, type));

	if (!vdd->not_support_single_tx) {
		if (drm_enc) {
			struct sde_encoder_virt *sde_enc = to_sde_encoder_virt(drm_enc);
			if (sde_enc && sde_enc->rc_state == SDE_ENC_RC_STATE_IDLE)
				sde_encoder_early_wakeup(drm_enc);
		}
		else
			LCD_ERR(vdd, "drm_enc is NULL\n");
	}

	if (ss_wrapper_dsi_display_clk_ctrl(vdd, true)) {
		LCD_ERR(vdd, "[%s] fail to enable clocks\n", display->name);
		return -EBUSY;
	}

	SDE_ATRACE_BEGIN("ss_send_cmd");

	if (ss_is_queue_cmd_waits_type(type))
		ss_wrapper_dsi_queue_cmd_waits(vdd, true);

#if IS_ENABLED(CONFIG_ARCH_PARROT) || IS_ENABLED(CONFIG_ARCH_VOLCANO)/* SM6475,SM7635 */
	rc = dsi_panel_tx_cmd_set(panel, type);
#else
	rc = dsi_panel_tx_cmd_set(panel, type, false);
#endif
	if (rc)
		LCD_ERR(vdd, "[%s] failed to send cmd(%s), rc=%d\n",
				display->name, ss_get_cmd_name(type), rc);

	if (ss_is_queue_cmd_waits_type(type))
		ss_wrapper_dsi_queue_cmd_waits(vdd, false);

	SDE_ATRACE_END("ss_send_cmd");

	if (ss_wrapper_dsi_display_clk_ctrl(vdd, false)) {
		LCD_ERR(vdd, "[%s] fail to disable clocks\n", display->name);
		return -EBUSY;
	}

	return rc;
}

int __mockable ss_wrapper_regulator_get_voltage(struct samsung_display_driver_data *vdd,
						struct regulator *regulator)
{
	if (!regulator) {
		LCD_ERR(vdd, "null regulator\n");
		return -ENODEV;
	}

	return regulator_get_voltage(regulator);
}

int __mockable ss_wrapper_regulator_set_voltage(struct samsung_display_driver_data *vdd,
						struct regulator *regulator, int min_uV, int max_uV)
{
	if (!regulator) {
		LCD_ERR(vdd, "null regulator\n");
		return -ENODEV;
	}

	return regulator_set_voltage(regulator, min_uV, max_uV);
}

int __mockable ss_wrapper_regulator_set_short_detection(struct samsung_display_driver_data *vdd,
						struct regulator *regulator, bool enable, int lv_uA)
{
	if (!regulator) {
		LCD_ERR(vdd, "null regulator\n");
		return -ENODEV;
	}

#if IS_ENABLED(CONFIG_SEC_PM)
	return regulator_set_current_limit(regulator, lv_uA, lv_uA);
#else
	LCD_ERR(vdd, "CONFG_SEC_PM is not Enabled\n");
	return -1;
#endif
}

int __mockable ss_wrapper_misc_register(struct samsung_display_driver_data *vdd,
					struct miscdevice *misc)
{
	if (!misc) {
		LCD_ERR(vdd, "null misc\n");
		return -ENODEV;
	}

	return misc_register(misc);
}

int ss_wrapper_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	if (!adap) {
		pr_debug("err: null adap\n");
		return -ENODEV;
	}

	return  i2c_transfer(adap, msgs, num);
}

/* Read file from vendor/firmware/ */
#define MAX_FIRMWARE_PATH_LEN 100
int ss_wrapper_read_from_file(struct samsung_display_driver_data *vdd, bool force_read)
{
	int ret = 0;
	const struct firmware *f_image = NULL;
	char firmware_path[MAX_FIRMWARE_PATH_LEN];

	if (vdd->file_loading && !force_read) {
		LCD_INFO(vdd, "Panel data loading is already done\n");
		return -EMFILE;
	}

	snprintf(firmware_path, MAX_FIRMWARE_PATH_LEN, "%s.dat", vdd->panel_name);

	LCD_INFO(vdd, "Loading Panel Data File name : [%s]\n", firmware_path);

	ret = request_firmware(&f_image, firmware_path, &vdd->lcd_dev->dev);
	if (ret < 0) {
		LCD_INFO(vdd, "Skip panel data firmware Loading\n");
		goto err;
	} else
		LCD_INFO(vdd, "Panel data firmware loading success. Size : %ld(bytes)\n", f_image->size);

	vdd->file_loading = true;
	vdd->f_size = f_image->size;
	vdd->f_buf = kvzalloc(f_image->size + 10, GFP_KERNEL);
	if (!vdd->f_buf) {
		LCD_ERR(vdd, "Buffer for firmware alloc failed\n");
		goto err;
	}

	memcpy(vdd->f_buf, f_image->data, f_image->size);

	LCD_INFO_IF(vdd, "%s\n", vdd->f_buf);
err:
	release_firmware(f_image);

	LCD_INFO(vdd, "File loading is done\n");

	return ret;
}

int __ss_parse_ss_cmd_sets_revision(struct samsung_display_driver_data *vdd,
					struct ss_cmd_set *set,
					int type, struct device_node *np, const char *buf,
					char *cmd_name, bool from_node)
{
	int rc = 0;
	int rev;
	char map[SS_CMD_PROP_STR_LEN];
	struct ss_cmd_set *set_rev[SUPPORT_PANEL_REVISION];
	int len;

	if (!set) {
		LCD_ERR(vdd, "set_rev: set is null\n");
		return -EINVAL;
	}

	/* For revA */
	set->cmd_set_rev[0] = set;

	/* For revB ~ revT */
	strlcpy(map, cmd_name, SS_CMD_PROP_STR_LEN);
	len = strlen(cmd_name);

	if (len >= SS_CMD_PROP_STR_LEN) {
		LCD_ERR(vdd, "set_rev: too long cmd name\n");
		return -EINVAL;
	}

	for (rev = 1; rev < SUPPORT_PANEL_REVISION; rev++) {
		const char *data;
		u32 length = 0;

		if (map[len - 1] >= 'A' && map[len - 1] <= 'Z')
			map[len - 1] = 'A' + rev;

		data = ss_wrapper_of_get_property(np, buf, map, &length, from_node);
		if (!data || !length) {
			/* If there is no data for the panel rev,
			 * copy previous panel rev data pointer.
			 */
			set->cmd_set_rev[rev] = set->cmd_set_rev[rev - 1];
			continue;
		}

		/* revA is ss style but sub revision is not ss style cmd */
		if (!ss_is_prop_string_style(np, buf, map, from_node)) {
			LCD_ERR(vdd, "none ss cmd[%s]\n", map);
			rc = -EINVAL;
			goto err_free_mem;
		}

		set_rev[rev] = kvzalloc(sizeof(struct ss_cmd_set), GFP_KERNEL);
		rc = ss_wrapper_parse_cmd_sets(vdd, set_rev[rev], type, np, buf, map, from_node);
		if (rc) {
			LCD_ERR(vdd, "set_rev: fail to parse rev(%d) set\n", rev);
			goto err_free_mem;
		}

		set->cmd_set_rev[rev] = set_rev[rev];
	}

	return 0;

err_free_mem:
	LCD_ERR(vdd, "err, set_rev: free set(rc: %d, type: %d, rev: %d)\n",
			rc, type, rev);
	for (rev = rev - 1; rev >= 1; --rev) {
		if (set_rev[rev])
			kfree(set_rev[rev]);
	}

	return rc;
}

#define TEMP_BUF_SIZE SZ_4M
char *ss_wrapper_parse_symbol(struct samsung_display_driver_data *vdd,
				const struct device_node *np,
				const char *data, u32 *length, char *key_string)
{
	char *final_data;
	char sym_name[50] = {CMD_STR_NUL,};
	int sym_name_len = 0;
	char *t_buf = NULL;
	u32 t_buf_cnt = 0;
	char *sym_data = NULL;
	int sym_data_len = 0;

	if (!strnchr(data, *length, '$')) {
		/* If there is not any symbol in data, return original data */
		return (char *)data;
	}

	t_buf = kvzalloc(TEMP_BUF_SIZE, GFP_KERNEL);
	if (!t_buf) {
		LCD_ERR(vdd, "Failed to t_buf memory alloc for [%s]\n", key_string);
		return (char *)data;
	}

	LCD_INFO_IF(vdd, "Parsing symbol for (%s) ++\n", key_string);

	/* 1. Copy data string to temp_buf with parsing symbol */
	while (data && *data != CMD_STR_NUL) {
		if (!strncasecmp(data, CMDSTR_COMMENT, strlen(CMDSTR_COMMENT)))
			data = strpbrk(data, "\r\n");

		if (!strncasecmp(data, CMDSTR_COMMENT2, strlen(CMDSTR_COMMENT2))) {
			/*Range Comment : go to behind end-comment */
			data = strstr(data, CMDSTR_COMMENT3) + 2;
		}

		if (*data == '$') {
			/* Symbol Parsing */
			data++;

			if (*data == '{')
				data++;

			while (IS_CMD_DELIMITER(*data))
				data++;

			sym_name_len = 0;
			while (data && !IS_CMD_DELIMITER(*data) && *data != CMD_STR_NUL
					&& *data != CMD_STR_QUOT && *data != '}') {
				sym_name[sym_name_len] = *data;
				sym_name_len++;
				data++;
			}
			sym_name[sym_name_len] = CMD_STR_NUL;

			if (*data == '}')
				data++;

			if (sym_name_len < 1) {
				LCD_ERR(vdd, "Failed to parse symbol name\n");
				goto err;
			}

			/* Search from f_buf first */
			sym_data = ss_wrapper_of_get_property(np, vdd->f_buf, sym_name, &sym_data_len, false);
			if (!sym_data) {
				/* If not, search from dt node */
				sym_data = ss_wrapper_of_get_property(np, NULL, sym_name, &sym_data_len, true);
				if (!sym_data) {
					LCD_ERR(vdd, "Failed to parse [%s]symbol data\n", sym_name);
					goto err;
				}
				LCD_INFO(vdd, "Parsing Symbol(%s) from DT node\n", sym_name);

				if (sym_data_len < TEMP_BUF_SIZE)
					strcpy(t_buf + t_buf_cnt, sym_data);
				else {
					LCD_ERR(vdd, "Symbol data size(%d) is bigger than temp buffer size(%d)\n",
									sym_data_len, TEMP_BUF_SIZE);
					goto err;
				}
			} else {
				LCD_INFO(vdd, "Parsing Symbol(%s) from f_buf\n", sym_name);
				if (sym_data_len < TEMP_BUF_SIZE) {
					strcpy(t_buf + t_buf_cnt, sym_data);
					kvfree(sym_data);
				} else {
					kvfree(sym_data);
					LCD_ERR(vdd, "Symbol data size(%d) is bigger than temp buffer size(%d)\n",
									sym_data_len, TEMP_BUF_SIZE);
					goto err;
				}
			}

			t_buf_cnt += (sym_data_len - 1); /* Ignore last NULL('\0') char */
		}

		t_buf[t_buf_cnt++] = *(data++);
	}
	t_buf[t_buf_cnt] = CMD_STR_NUL;

	/* 2. Alloc mem to match the size with parsed data len */
	final_data = kvzalloc(strlen(t_buf) + 1, GFP_KERNEL);
	if (!final_data) {
		LCD_ERR(vdd, "Failed to backup memory alloc\n");
		goto err;
	}

	/* 3. Copy data from t_buf(parsed data with 16K) to final_buf(parsed data with calculated size) */
	strcpy(final_data, t_buf);
	*length = t_buf_cnt;

	if (strnchr(final_data, *length, '$')) {
		LCD_INFO_IF(vdd, "Recursive Symbol Parsing len=[%d] final_data=[%s]\n", *length, final_data);
		final_data = ss_wrapper_parse_symbol(vdd, np, final_data, length, key_string);
	}

	LCD_INFO_IF(vdd, "Parsing symbol for (%s) --\n", key_string);

	kvfree(t_buf);
	return final_data;
err:
	kvfree(t_buf);
	*length = 1; /* handle it as empty data now.. TBR: return error code instead of data pointer.. */
	return (char *)data;
}


__mockable u8 *ss_wrapper_of_get_property(const struct device_node *np, const char *buf,
					const char *name, int *lenp, bool from_node)
{
	if (from_node)
		return (u8 *)of_get_property(np, name, lenp);
	else
		return (u8 *)ss_get_str_property(buf, name, lenp);
}

__mockable int ss_wrapper_of_get_named_gpio(struct device_node *np,
					const char *name, int index)
{
	return of_get_named_gpio(np, name, index);
}

static int ss_get_cmd_pkt_count(const char *data, u32 length, u32 *cnt)
{
	u32 count = 0;

	char *data_block, *data_block_backup;
	char *data_line;
	char *data_val;

	/* strsep modifies original data.
	 * To keep original data, copy to temprorary data block
	 */
	data_block = kvzalloc(length, GFP_KERNEL);
	if (!data_block)
		return -ENOMEM;

	strlcpy(data_block, data, length);
	data_block_backup = data_block;

	while ((data_line = strsep(&data_block, "\r\n")) != NULL) {
		if (!*data_line)
			continue;

		while (IS_CMD_DELIMITER(*data_line))
			++data_line;

		data_val = strsep(&data_line, " \t");
		if ((strlen(data_val) == strlen(CMDSTR_WRITE) &&
				!strncasecmp(data_val, CMDSTR_WRITE, strlen(CMDSTR_WRITE))) ||
			(strlen(data_val) == strlen(CMDSTR_WRITE_TYPE) &&
				 !strncasecmp(data_val, CMDSTR_WRITE_TYPE, strlen(CMDSTR_WRITE_TYPE))) ||
			(strlen(data_val) == strlen(CMDSTR_READ) &&
				!strncasecmp(data_val, CMDSTR_READ, strlen(CMDSTR_READ))) ||
			(strlen(data_val) == strlen(CMDSTR_READ_TYPE) &&
				!strncasecmp(data_val, CMDSTR_READ_TYPE, strlen(CMDSTR_READ_TYPE))))
			count++;
	}

	*cnt = count;

	kvfree(data_block_backup);

	return 0;
}

static int ss_alloc_cmd_packets(struct ss_cmd_set *set, u32 packet_count)
{
	int i;

	set->cmds = kvzalloc(packet_count * sizeof(*set->cmds), GFP_KERNEL);
	if (!set->cmds) {
		pr_err("%s: fail to allocate cmds\n", __func__);
		return -ENOMEM;
	}

	set->count = packet_count;

	for (i = 0; i < set->count; i++)
		INIT_LIST_HEAD(&set->cmds[i].op_list);

	return 0;
}

/* Count 4 consecutive charactors without delimiters(space or tab).
 * If "0xAA 0xBB", count is 2.
 */
static int ss_cmd_val_count(char *input)
{
	int cnt_val = 0;
	char c;
	char *data = input;

	/*data format: 0x01 0x02 0x03 ... */
	while ((c = *data)) {
		if (IS_CMD_DELIMITER(c)) {
			++data;
			continue;
		}

		if (!(!IS_CMD_DELIMITER(data[1]) &&
				!IS_CMD_DELIMITER(data[2]) &&
				!IS_CMD_DELIMITER(data[3]) &&
				(IS_CMD_DELIMITER(data[4]) || (!data[4])))) {
			if (!strncasecmp(data, CMDSTR_COMMENT, strlen(CMDSTR_COMMENT)) ||
				!strncasecmp(data, CMDSTR_COMMENT2, strlen(CMDSTR_COMMENT2))) {
				/* Skip comment */
				pr_debug("%s: Skip comment[%s] cnt_val:%d\n", __func__, data, cnt_val);
				break;
			} else {
				pr_err("%s: invalid data input[%s] cnt_val:%d\n", __func__, input, cnt_val);
				return -1;
			}
		}

		data += 4;
		++cnt_val;
	};

	return cnt_val;
}


static u8 ss_cmd_get_type(struct samsung_display_driver_data *vdd,
			enum SS_CMD_CTRL cmd_ctrl, int cnt_val)
{
	u8 type;

	if (cmd_ctrl == CMD_CTRL_READ) {
		type = MIPI_DSI_DCS_READ; /* 06h */
	} else if (cmd_ctrl == CMD_CTRL_WRITE) {
		if (cnt_val == 1)
			type = vdd->ss_cmd_dsc_short_write_param ?
				MIPI_DSI_DCS_SHORT_WRITE_PARAM : /* 15h */
				MIPI_DSI_DCS_SHORT_WRITE; /* default 05h */
		else
			type = vdd->ss_cmd_dsc_long_write ?
				MIPI_DSI_DCS_LONG_WRITE : /* 39h */
				MIPI_DSI_GENERIC_LONG_WRITE; /* default 29h */
		/* type = MIPI_DSI_DCS_LONG_WRITE;
		 * some ddi or some condition requires to set 39h type..
		 * TODO: find that condition and use it..
		 */
	} else {
		LCD_ERR(vdd, "invalid cmd_ctrl(%d)\n", cmd_ctrl);
		type = -EINVAL;
	}

	return type;
}


/* copy data from old, except op_root and sibling's list connection */
static struct ss_cmd_op_str *ss_create_op(enum ss_cmd_op op,
					char *symbol, char *val,
					enum ss_cmd_op_cond cond,
					size_t candidate_len, u8 *candidate_buf)
{
	struct ss_cmd_op_str *new;

	new = kzalloc(sizeof(*new), GFP_KERNEL);
	if (!new) {
		pr_err("%s: SDE: fail to allocate new\n", __func__);
		return NULL;
	}

	INIT_LIST_HEAD(&new->list);
	INIT_LIST_HEAD(&new->sibling);

	new->op = op;
	if (symbol) {
		new->symbol = kzalloc(strlen(symbol) + 1, GFP_KERNEL);
		if (!new->symbol) {
			pr_err("%s: SDE: fail to allocate op_symbol\n", __func__);
			return NULL;
		}
		strcpy(new->symbol, symbol);
	}

	if (val) {
		new->val = kzalloc(strlen(val) + 1, GFP_KERNEL);
		if (!new->val) {
			pr_err("%s: SDE: fail to allocate op_val\n", __func__);
			return NULL;
		}
		strcpy(new->val, val);
	}

	new->cond = cond;

	if (candidate_len > 0 && candidate_buf) {
		new->candidate_len = candidate_len;
		new->candidate_buf = kvzalloc(new->candidate_len, GFP_KERNEL);
		if (!new->candidate_buf) {
			pr_err("%s: SDE: fail to allocate candidate buf\n", __func__);
			return NULL;
		}

		memcpy(new->candidate_buf, candidate_buf, new->candidate_len);
	} else if (candidate_len > 0 || candidate_buf) {
		/* invalid: !(candidate_len == 0 && candidate_buf == NULL) case */
		pr_err("%s: SDE: invalid candidate(len: %zd, buf: %d)\n",
				__func__, candidate_len, !!candidate_buf);
		return NULL;
	}

	return new;
}

/* copy data from src, except op_root and sibling's list connection */
static struct ss_cmd_op_str *ss_create_copy_op(struct ss_cmd_op_str *src)
{
	return ss_create_op(src->op, src->symbol, src->val, src->cond,
				src->candidate_len, src->candidate_buf);
}

static void ss_destroy_op(struct ss_cmd_op_str *old)
{
	if (!old) {
		pr_err("SDE: fail to destroy NULL op\n");
		return;
	}

	list_del(&old->sibling);
	list_del(&old->list);

	kfree(old->candidate_buf);
	kfree(old->val);
	kfree(old->symbol);
	kfree(old);
}

static int ss_copy_op_list(struct list_head *dst_lh, struct list_head *src_lh)
{
	struct ss_cmd_op_str *op_src_root, *op_src_sibling;
	struct ss_cmd_op_str *op_new_root, *op_new_sibling;

	list_for_each_entry(op_src_root, src_lh, list) {
		op_new_root = ss_create_copy_op(op_src_root);
		if (!op_new_root) {
			pr_err("%s: SDE: fail to get op_new_root\n", __func__);
			return -ENOMEM;
		}

		list_add_tail(&op_new_root->list, dst_lh);

		list_for_each_entry(op_src_sibling, &op_src_root->sibling, sibling) {
			op_new_sibling = ss_create_copy_op(op_src_sibling);
			if (!op_new_sibling) {
				pr_err("%s: SDE: fail to get op_new_sibling\n", __func__);
				return -ENOMEM;
			}

			list_add_tail(&op_new_sibling->sibling, &op_new_root->sibling);
		}
	}

	return 0;
}

static int ss_free_op_list(struct list_head *lh)
{
	struct ss_cmd_op_str *op_root, *op_sibling, *op_root_next, *op_sibling_next;

	list_for_each_entry_safe(op_root, op_root_next, lh, list) {
		list_for_each_entry_safe(op_sibling, op_sibling_next, &op_root->sibling, sibling)
			ss_destroy_op(op_sibling);
		ss_destroy_op(op_root);
	}

	return 0;
}


/* Parse command from dtsi data.
 * example)
 * W 0xF7 0x0F
 * delay 100ms
 * R 0xC8 0x01
 */
#define MAX_UPDATE_0xXX_LEN	(256) /* temporary value */

static int ss_create_cmd_packets(struct samsung_display_driver_data *vdd,
				const char *dtsi_data, u32 length, u32 count,
				struct ss_cmd_desc *cmds, struct ss_cmd_set *set,
				bool is_lp)
{
	int rc = 0;
	int cmd_idx = 0;
	char *data_block, *data_block_backup;
	char *data_line;
	char *data_val;
	int cnt_val = 0;
	u8 *payload;
	u8 pos_0xXX[MAX_UPDATE_0xXX_LEN];
	int i;

	char *pre;

	char *op_str;
	struct ss_cmd_op_str *op = NULL;
	struct ss_cmd_op_str *op_root;
	char *op_symbol;
	char *op_val;
	char *op_instr;
	enum ss_cmd_op_cond cond;
	int gpara_offset = 0;
	int gpara_addr = 0;
	int gpara_len;

	LIST_HEAD(op_blk_list_head);

	/* strsep modifies original data.
	 * To keep original data, copy to temprorary data block
	 */
	data_block = kvzalloc(length, GFP_KERNEL);
	if (!data_block) {
		LCD_ERR(vdd, "fail to allocate data_block\n");
		return -ENOMEM;
	}
	strlcpy(data_block, dtsi_data, length);
	data_block_backup = data_block;

	/* concatenate cmd data split by '\n', like below.
	 * "W 0x9E 0x11 0x00 \n 0x05 0xA0" -> "W 0x9E 0x11 0x00   0x05 0xA0"
	 */
	pre = data_block;
	while ((pre = strpbrk(pre, "\r\n"))) {
		char *next;

		next = pre + 1;
		while (IS_CMD_DELIMITER(*next))
			++next;

		/* reach to end of string, do not access further memory */
		if (!next[0] || !next[1])
			break;

		if (!strncasecmp(next, "0x", 2)) {
			if (pre && IS_CMD_NEWLINE(*pre))
				*pre = CMD_STR_SPACE;
			if ((pre + 1) && IS_CMD_NEWLINE(*(pre + 1)))
				*(pre + 1) = CMD_STR_SPACE;
		}

		do {
			++pre;
		} while (IS_CMD_DELIMITER(*pre));
	}

	while ((data_line = strsep(&data_block, "\r\n")) != NULL) {
		if (!*data_line)
			continue;

		while (IS_CMD_DELIMITER(*data_line))
			++data_line;

		LCD_INFO_IF(vdd, "data_line: [%s]\n", data_line);

		if (!strncasecmp(data_line, CMDSTR_WRITE, strlen(CMDSTR_WRITE)) ||
				!strncasecmp(data_line, CMDSTR_WRITE_TYPE, strlen(CMDSTR_WRITE_TYPE))) {
			enum SS_CMD_CTRL cmd_ctrl;
			u8 hdr_type;

			if (!strncasecmp(data_line, CMDSTR_WRITE_TYPE, strlen(CMDSTR_WRITE_TYPE))) {
				data_line += strlen(CMDSTR_WRITE_TYPE);
				cmd_ctrl = CMD_CTRL_WRITE_TYPE;
			} else {
				data_line += strlen(CMDSTR_WRITE);
				cmd_ctrl = CMD_CTRL_WRITE;
			}

			cnt_val = ss_cmd_val_count(data_line);

			if (cmd_ctrl == CMD_CTRL_WRITE_TYPE) {
				while ((data_val = strsep(&data_line, " \t")) != NULL && !*data_val);

				if (!data_val || kstrtou8(data_val, 16, &hdr_type)) {
					LCD_ERR(vdd, "fail to parse WT header[%s], line [%s]\n",
							data_val ? data_val : "NULL", data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				cmds[cmd_idx].type = hdr_type;
				cnt_val--;
			} else {
				cmds[cmd_idx].type = ss_cmd_get_type(vdd, CMD_CTRL_WRITE, cnt_val);
			}

			if (cnt_val < 1) {
				LCD_ERR(vdd, "no data for WRITE, line [%s]\n", data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			payload = kvcalloc(cnt_val, sizeof(u8), GFP_KERNEL);
			if (!payload) {
				rc = -ENOMEM;
				goto error_free_payloads;
			}

			memset(pos_0xXX, 0, MAX_UPDATE_0xXX_LEN);

			i = 0;
			while ((data_val = strsep(&data_line, " \t")) != NULL) {
				if (!*data_val)
					continue;

				if (kstrtou8(data_val, 16, &payload[i])) {
					if (!strncasecmp(data_val, "0xXX" ,4)) {
						cmds[cmd_idx].updatable = SS_CMD_UPDATABLE;
						if (unlikely(i >= MAX_UPDATE_0xXX_LEN)) {
							/* Temporary MAX_UPDATE_0xXX_LEN is not enough..
							 * Increase MAX_UPDATE_0xXX_LEN...
							 */
							LCD_ERR(vdd, "fail to update 0xXX pos(%d)\n", i);
							goto error_free_payloads;
						} else {
							pos_0xXX[i] = 1;
						}
					} else {
						if (!strncasecmp(data_val, CMDSTR_COMMENT, strlen(CMDSTR_COMMENT)) ||
							!strncasecmp(data_val, CMDSTR_COMMENT2, strlen(CMDSTR_COMMENT2))) {
							LCD_INFO_IF(vdd, "skip comment[%s] i:%d\n", data_val, i);
							break;
						} else {
							LCD_ERR(vdd, "fail to parse [%s], line [%s]\n",
									data_val, data_line);
							rc = -EINVAL;
							kvfree(payload);
							goto error_free_payloads;
						}
					}
				}
				i++;
			}

			/* TODO: get some hint to set last_command */
			cmds[cmd_idx].last_command = vdd->ss_cmd_always_last_command_set ? true : false;
			/* will be updated with following DELAY command */
			cmds[cmd_idx].post_wait_ms = 0;
			cmds[cmd_idx].post_wait_frame = 0;
			cmds[cmd_idx].tx_len = cnt_val;
			cmds[cmd_idx].txbuf = payload; /* TODO: for IF XX, free original txbuf to save buffer */
			cmds[cmd_idx].rx_len = 0;
			cmds[cmd_idx].rx_offset = 0;
			cmds[cmd_idx].rxbuf = NULL;

			if (vdd->gpara) {
			/* keep the offset of the current gpara to set the rx_offset of the next rx cmd. */
				gpara_len = 2;
				if (vdd->two_byte_gpara)
					gpara_len++;
				if (vdd->pointing_gpara)
					gpara_len++;

				if (cmds[cmd_idx].txbuf[0] == 0xB0 && cmds[cmd_idx].tx_len == gpara_len) {
					if (vdd->two_byte_gpara) { /* two byte gpara */
						gpara_offset = cmds[cmd_idx].txbuf[2];
						gpara_offset |= (cmds[cmd_idx].txbuf[1] << 8);
						gpara_addr = cmds[cmd_idx].txbuf[3];
					} else {	/* one byte gpara */
						gpara_offset = cmds[cmd_idx].txbuf[1];
						gpara_addr = cmds[cmd_idx].txbuf[2];
					}

					LCD_DEBUG(vdd, "save B0 info : rx_addr %X / offset %X\n", gpara_addr, gpara_offset);
				}
			}

			ss_copy_op_list(&cmds[cmd_idx].op_list, &op_blk_list_head);

			++cmd_idx;
		} else if (!strncasecmp(data_line, CMDSTR_READ, strlen(CMDSTR_READ)) ||
				!strncasecmp(data_line, CMDSTR_READ_TYPE, strlen(CMDSTR_READ_TYPE))) {
			u32 rx_addr;
			u32 rx_len;
			u8 hdr_type;
			enum SS_CMD_CTRL cmd_ctrl;

			if (!strncasecmp(data_line, CMDSTR_READ_TYPE, strlen(CMDSTR_READ_TYPE))) {
				data_line += strlen(CMDSTR_READ_TYPE);
				cmd_ctrl = CMD_CTRL_READ_TYPE;
			} else {
				data_line += strlen(CMDSTR_READ);
				cmd_ctrl = CMD_CTRL_READ;
			}

			cnt_val = ss_cmd_val_count(data_line);

			if (cmd_ctrl == CMD_CTRL_READ_TYPE) {
				if (cnt_val != 3) {
					LCD_ERR(vdd, "invalid read_type cnt_val(%d)\n", cnt_val);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				if (sscanf(data_line, "%s %x %x", &hdr_type, &rx_addr, &rx_len) != 3) {
					LCD_ERR(vdd, "fail to parse, line [%s]\n", data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				cmds[cmd_idx].type = hdr_type;
			} else {
				if (cnt_val != 2) {
					LCD_ERR(vdd, "invalid read cnt_val(%d), line [%s]\n",
							cnt_val, data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				if (sscanf(data_line, "%x %x", &rx_addr, &rx_len) != 2) {
					LCD_ERR(vdd, "fail to parse, line [%s]\n", data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				cmds[cmd_idx].type = ss_cmd_get_type(vdd, CMD_CTRL_READ, cnt_val);
			}

			if (rx_addr > 0xFF || rx_len > 0xFF) {
				LCD_ERR(vdd, "invalid RX len: rx_addr: %X, rx_len: %X, line [%s]\n",
						rx_addr, rx_len, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			cmds[cmd_idx].last_command = true;
			cmds[cmd_idx].post_wait_ms = 0; /* will be updated with later DELAY command */
			cmds[cmd_idx].post_wait_frame = 0; /* will be updated with later DELAY command */

			cmds[cmd_idx].tx_len = 1;
			cmds[cmd_idx].txbuf = kvcalloc(cmds[cmd_idx].tx_len, sizeof(u8), GFP_KERNEL);
			if (!cmds[cmd_idx].txbuf) {
				rc = -ENOMEM;
				goto error_free_payloads;
			}
			cmds[cmd_idx].txbuf[0] = (u8)rx_addr;
			cmds[cmd_idx].rx_len = (u8)rx_len;

			/* set rx_offset from previous grapa cmd (B0h) */
			if (gpara_offset && (gpara_addr == cmds[cmd_idx].txbuf[0])) {
				cmds[cmd_idx].rx_offset = gpara_offset;
				LCD_INFO(vdd, "set gpara offset : rx_addr %X / offset %X\n", rx_addr, cmds[cmd_idx].rx_offset);
				gpara_offset = 0;
			} else {
				cmds[cmd_idx].rx_offset = 0;
			}

			ss_copy_op_list(&cmds[cmd_idx].op_list, &op_blk_list_head);

			/* rxbuf will be allocated in ss_pack_rx_gpara() and ss_pack_rx_no_gpara() */
			++cmd_idx;
		} else if (!strncasecmp(data_line, CMDSTR_DELAY, strlen(CMDSTR_DELAY))) {
			int delay;
			char *endp;

			data_line += strlen(CMDSTR_DELAY);
			while (IS_CMD_DELIMITER(*data_line))
				++data_line;

			if (cmd_idx < 1) {
				LCD_ERR(vdd, "DELAY w/o cmd(%d), cnt: %d, line [%s]\n",
						cmd_idx, cnt_val, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			delay = simple_strtol(data_line, &endp, 10);
			if (!endp || strlen(endp) <= 0) {
				LCD_ERR(vdd, "invalid delay, wait: %d, line [%s]\n",
						delay, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			if (!strncasecmp(endp, "ms", 2)) {
				cmds[cmd_idx - 1].post_wait_ms = delay;
			} else if (!strncasecmp(endp, "frame", 5)) {
				cmds[cmd_idx - 1].post_wait_frame = delay;
			} else {
				LCD_ERR(vdd, "invalid delay unit(%s), delay: %d, line [%s]\n",
						endp, delay, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}
		} else if (!strncasecmp(data_line, CMDSTR_IF, strlen(CMDSTR_IF)) &&
				strnstr(data_line, CMDSTR_APPLY, strlen(data_line))) {
			/* IF BLOCK */
			op_root = NULL;
			cond = SS_CMD_OP_COND_AND;

			data_line += strlen(CMDSTR_IF);

			while ((op_str = strsep(&data_line, " \t")) != NULL) {
				/* symbol */
				op_symbol = op_str;
				while (op_symbol && !*op_symbol)
					op_symbol = strsep(&data_line, " \t");

				if (!op_symbol) {
					rc = -EINVAL;
					LCD_ERR(vdd, "Invalid IF: Condition Symbol missing before Line: [%s]\n", data_line);
					goto error_free_payloads;
				}

				/* value */
				op_val = strsep(&data_line, " \t");
				while (op_val && !*op_val)
					op_val = strsep(&data_line, " \t");

				if (!op_val) {
					rc = -EINVAL;
					LCD_ERR(vdd, "Invalid IF: Condition Value missing before Line: [%s]\n", data_line);
					goto error_free_payloads;
				}

				op = ss_create_op(SS_CMD_OP_IF_BLOCK, op_symbol, op_val, cond, 0, NULL);
				if (!op) {
					LCD_ERR(vdd, "fail to create op\n");
					rc = -ENOMEM;
					goto error_free_payloads;
				}

				if (!op_root) { /* first op */
					op_root = op;
					INIT_LIST_HEAD(&op_root->sibling);
				} else {
					list_add_tail(&op->sibling, &op_root->sibling);
				}

				/* next instructor like AND, OR, APPLY or THEN */
				op_instr = strsep(&data_line, " \t");
				while (op_instr && !*op_instr)
					op_instr = strsep(&data_line, " \t");

				if (!op_instr) {
					LCD_ERR(vdd, "invalid instructor [%s], line [%s]\n",
							op_instr, data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				if (!strncasecmp(op_instr, CMDSTR_AND, strlen(CMDSTR_AND))) {
					cond = SS_CMD_OP_COND_AND;
				} else if (!strncasecmp(op_instr, CMDSTR_OR, strlen(CMDSTR_OR))) {
					/* default condition is AND.. if OR, upadte it to op_root */
					cond = SS_CMD_OP_COND_OR;
					op_root->cond = cond;
				} else if (!strncasecmp(op_instr, CMDSTR_THEN, strlen(CMDSTR_THEN))) {
					/* do not have to check "APPLY" */
					break;
				} else {
					LCD_ERR(vdd, "invalid instructor [%s], line [%s]\n",
							op_instr, data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}
			}

			if (!op_root) {
				rc = -EINVAL;
				LCD_ERR(vdd, "invalid if block input, line [%s]\n", data_line);
				goto error_free_payloads;
			}

			list_add_tail(&op_root->list, &op_blk_list_head);
		} else if (!strncasecmp(data_line, CMDSTR_END, strlen(CMDSTR_END))) {
			list_del(op_blk_list_head.prev);
		} else if (!strncasecmp(data_line, CMDSTR_IF, strlen(CMDSTR_IF))) {
			op_root = NULL;
			cond = SS_CMD_OP_COND_AND;

			data_line += strlen(CMDSTR_IF);

			while ((op_str = strsep(&data_line, " \t")) != NULL) {
				/* symbol */
				op_symbol = op_str;
				while (op_symbol && !*op_symbol)
					op_symbol = strsep(&data_line, " \t");

				if (!op_symbol) {
					rc = -EINVAL;
					LCD_ERR(vdd, "Invalid IF: Condition Symbol missing before Line: [%s]\n", data_line);
					goto error_free_payloads;
				}

				/* value */
				op_val = strsep(&data_line, " \t");
				while (op_val && !*op_val)
					op_val = strsep(&data_line, " \t");

				if (!op_val) {
					rc = -EINVAL;
					LCD_ERR(vdd, "Invalid IF: Condition Value missing before Line: [%s]\n", data_line);
					goto error_free_payloads;
				}

				op = ss_create_op(SS_CMD_OP_IF, op_symbol, op_val, cond, 0, NULL);
				if (!op) {
					LCD_ERR(vdd, "fail to create op\n");
					rc = -ENOMEM;
					goto error_free_payloads;
				}

				if (!op_root) { /* first op */
					op_root = op;
					INIT_LIST_HEAD(&op_root->sibling);
				} else {
					list_add_tail(&op->sibling, &op_root->sibling);
				}

				/* next instructor like AND or OR */
				op_instr = strsep(&data_line, " \t");
				while (op_instr && !*op_instr)
					op_instr = strsep(&data_line, " \t");

				if (!op_instr) {
					LCD_ERR(vdd, "Invalid IF Block: Empty payload Data, Before Line :[%s]\n", data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				if (!strncasecmp(op_instr, CMDSTR_AND, strlen(CMDSTR_AND))) {
					cond = SS_CMD_OP_COND_AND;
				} else if (!strncasecmp(op_instr, CMDSTR_OR, strlen(CMDSTR_OR))) {
					/* default condition is AND.. if OR, upadte it to op_root */
					cond = SS_CMD_OP_COND_OR;
					op_root->cond = cond;
				} else if (!strncasecmp(op_instr, CMDSTR_THEN, strlen(CMDSTR_THEN))) {
					break;
				} else {
					LCD_ERR(vdd, "invalid instructor [%s]\n", op_instr);
					rc = -EINVAL;
					goto error_free_payloads;
				}
			}

			if (!op_root) {
				rc = -EINVAL;
				LCD_ERR(vdd, "invalid if block input\n");
				goto error_free_payloads;
			}

			if (cmd_idx < 1 || cmds[cmd_idx - 1].updatable != SS_CMD_UPDATABLE) {
				LCD_ERR(vdd, "Invalid IF: No updatable WRITE Command , w/o cmd(%d), cnt: %d, line [%s]\n",
						cmd_idx, cnt_val, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			/* parse and save candidate commands */
			cnt_val = cmds[cmd_idx - 1].tx_len;
			payload = kvcalloc(cnt_val, sizeof(u8), GFP_KERNEL);
			if (!payload) {
				LCD_ERR(vdd, "fail to alloc candidate buf(%d)\n", cnt_val);
				rc = -ENOMEM;
				goto error_free_payloads;
			}

			memcpy(payload, cmds[cmd_idx - 1].txbuf, cnt_val);

			i = 0;
			while ((data_val = strsep(&data_line, " \t")) != NULL) {
				if (!*data_val)
					continue;

				/* find next updatable 0xXX pos */
				while (!pos_0xXX[i] && i < cnt_val)
					i++;
				if (i >= cnt_val) {
					LCD_ERR(vdd, "not matched 0xXX count(%d), line [%s]\n",
							cnt_val, data_line);
					kvfree(payload);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				if (kstrtou8(data_val, 16, &payload[i])) {
					LCD_ERR(vdd, "fail to parse [%s], line [%s]\n",
							data_val, data_line);
					kvfree(payload);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				i++;
			}

			op_root->candidate_buf = payload;
			op_root->candidate_len = cnt_val;

			list_add_tail(&op_root->list, &cmds[cmd_idx - 1].op_list);
		} else if (!strncasecmp(data_line, CMDSTR_ELSE, strlen(CMDSTR_ELSE))) {
			data_line += strlen(CMDSTR_ELSE);
			if (cmd_idx < 1 || cmds[cmd_idx - 1].updatable != SS_CMD_UPDATABLE) {
				LCD_ERR(vdd, "Invalid ELSE: Missing Updatable Write Block\nw/o cmd(%d), cnt: %d, line [%s]\n",
						cmd_idx, cnt_val, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			/* ELSE: do not refer to cond, but just save with default SS_CMD_OP_COND_AND */
			op = ss_create_op(SS_CMD_OP_ELSE, NULL, NULL, SS_CMD_OP_COND_AND, 0, NULL);
			if (!op) {
				LCD_ERR(vdd, "fail to create op\n");
				rc = -ENOMEM;
				goto error_free_payloads;
			}

			/* parse and save candidate commands */
			cnt_val = cmds[cmd_idx - 1].tx_len;
			payload = kvcalloc(cnt_val, sizeof(u8), GFP_KERNEL);
			if (!payload) {
				LCD_ERR(vdd, "fail to alloc candidate buf(%d)\n", cnt_val);
				rc = -ENOMEM;
				goto error_free_payloads;
			}

			memcpy(payload, cmds[cmd_idx - 1].txbuf, cnt_val);

			i = 0;
			while ((data_val = strsep(&data_line, " \t")) != NULL) {
				if (!*data_val)
					continue;

				/* find next updatable 0xXX pos */
				while (!pos_0xXX[i] && i < cnt_val)
					i++;
				if (i >= cnt_val) {
					LCD_ERR(vdd, "not matched 0xXX count(%d), line [%s]\n",
							cnt_val, data_line);
					kvfree(payload);
					rc = -EINVAL;
					goto error_free_payloads;
				}

				if (kstrtou8(data_val, 16, &payload[i])) {
					LCD_ERR(vdd, "fail to parse [%s], line [%s]\n", data_val, data_line);
					rc = -EINVAL;
					goto error_free_payloads;
				}
				i++;
			}

			op->candidate_buf = payload;
			op->candidate_len = cnt_val;

			list_add_tail(&op->list, &cmds[cmd_idx - 1].op_list);
		} else if (!strncasecmp(data_line, CMDSTR_UPDATE, strlen(CMDSTR_UPDATE))) {
			data_line += strlen(CMDSTR_UPDATE);

			op_symbol = strsep(&data_line, " \t");
			while (op_symbol && !*op_symbol)
				op_symbol = strsep(&data_line, " \t");

			if (!op_symbol) {
				LCD_ERR(vdd, "UPDATE : Missing Symbol Name\nw/o cmd(%d), cnt: %d, line [%s]\n",
						cmd_idx, cnt_val, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			if (cmd_idx < 1) {
				LCD_ERR(vdd, "UPDATE w/o cmd(%d), cnt: %d, line [%s]\n",
						cmd_idx, cnt_val, data_line);
				rc = -EINVAL;
				goto error_free_payloads;
			}

			/* UPDATE : do not refer to cond, but just save with default SS_CMD_OP_COND_AND */
			op = ss_create_op(SS_CMD_OP_UPDATE, op_symbol, NULL, SS_CMD_OP_COND_AND, 0, NULL);
			if (!op) {
				LCD_ERR(vdd, "fail to create op\n");
				rc = -ENOMEM;
				goto error_free_payloads;
			}

			cmds[cmd_idx - 1].pos_0xXX = kvcalloc(cnt_val, sizeof(u8), GFP_KERNEL);
			if (!cmds[cmd_idx - 1].pos_0xXX) {
				LCD_ERR(vdd, "fail to alloc pos_0xXX buf(%d)\n", cnt_val);
				ss_destroy_op(op);
				rc = -ENOMEM;
				goto error_free_payloads;
			}
			memcpy(cmds[cmd_idx - 1].pos_0xXX, pos_0xXX, cnt_val);

			list_add_tail(&op->list, &cmds[cmd_idx - 1].op_list);
		} else if (!strncasecmp(data_line, CMDSTR_COMMENT, strlen(CMDSTR_COMMENT)) ||
			!strncasecmp(data_line, CMDSTR_COMMENT2, strlen(CMDSTR_COMMENT2))) {
			op_str = strsep(&data_line, "\r\n");
			LCD_INFO_IF(vdd, "Skip Line Comment(%s)\n", op_str);
		} else if (*data_line) { /* not empty line, without valid ctrl cmd */
			LCD_ERR(vdd, "invalid ctrl cmd, line [%s]\n", data_line);
			rc = -EINVAL;
			goto error_free_payloads;
		}
	}

	set->state = SS_CMD_SET_STATE_HS;

	/* send mipi rx packets in LP mode to prevent SoT error.
	 * case 03377897
	 * In combinaiton of tx/rx commands, use only HS mode...
	 */
	if ((set->count == 1 && ss_is_read_ss_cmd(&cmds[0])) || is_lp)
		set->state = SS_CMD_SET_STATE_LP;

	/*
	 * For Video Hybrid Mode, All TX/RX cmd's state shuld be HS.
	 * QC Official recommend from Case 07302146
	 */
	if (vdd->esync_support)
		set->state = SS_CMD_SET_STATE_HS;

	kvfree(data_block_backup);

	return 0;

error_free_payloads:
	LCD_ERR(vdd, "free: cmd_idx: %d\n", cmd_idx);
	for (cmd_idx = cmd_idx - 1; cmd_idx >= 0; cmd_idx--) {
		kfree_and_null(cmds[cmd_idx].txbuf);
		kfree_and_null(cmds[cmd_idx].rxbuf);
	}

	kvfree(data_block_backup);

	return rc;
}

int ss_bridge_qc_cmd_alloc(struct samsung_display_driver_data *vdd,
				struct ss_cmd_set *ss_set, bool no_gpara)
{
	struct dsi_panel_cmd_set *qc_set = &ss_set->base;
	struct ss_cmd_desc *cmd = ss_set->cmds;
	int rx_split_num;
	u32 size;
	int i;

	qc_set->count = ss_set->count;

	if (no_gpara)
		goto alloc;

	for (i = 0; i < ss_set->count; i++, cmd++) {
		if (!ss_is_read_ss_cmd(cmd))
			continue;

		rx_split_num = DIV_ROUND_UP(cmd->rx_len, RX_SIZE_LIMIT);

		/* For ss style command, it always have B0 gpara command
		 * prior to RX command.  It means that first RX comamnd (i=0)
		 * with gpara already have B0 gpara command, and don't
		 * have to add B0 gpara command here.
		 */
		rx_split_num = rx_split_num  * 2 - 1; /* gparam(B0h) and rx cmd,  */
		qc_set->count += rx_split_num  - 1; /* one ss cmd --> rx_split_num */
	}

alloc:
	size = qc_set->count * sizeof(*qc_set->cmds);
	qc_set->cmds = kvzalloc(size, GFP_KERNEL);
	if (!qc_set->cmds)
		return -ENOMEM;

	return 0;
}

static struct dsi_cmd_desc *ss_pack_rx_gpara(struct samsung_display_driver_data *vdd,
		struct ss_cmd_desc *ss_cmd, struct dsi_cmd_desc *qc_cmd)
{
	int tot_rx_len;
	u8 rx_addr;
	int i;
	int rx_split_num;
	int rx_offset;

	ss_cmd->rxbuf = kvzalloc(ss_cmd->rx_len, GFP_KERNEL);
	if (!ss_cmd->rxbuf) {
		LCD_ERR(vdd, "fail to alloc ss rxbuf(%zd)\n", ss_cmd->rx_len);
		return NULL;
	}

	rx_addr = ss_cmd->txbuf[0];
	rx_offset = ss_cmd->rx_offset;
	tot_rx_len = ss_cmd->rx_len;
	rx_split_num = DIV_ROUND_UP(tot_rx_len, RX_SIZE_LIMIT);

	for (i = 0; i < rx_split_num; i++) {
		int pos = 0;
		u8 *payload;
		int rx_len;

		/* For ss style command, it always have B0 gpara command
		 * prior to RX command.  It means that first RX comamnd (i=0)
		 * with gpara already have B0 gpara command, and don't
		 * have to add B0 gpara command here.
		 */
		if (i > 0) {
			/* 1. Send gpara first with new offset */
			/* if samsung,two_byte_gpara is true,
			 * that means DDI uses two_byte_gpara
			 * to figure it, check cmd length
			 * 06 01 00 00 00 00 01 DA 01 00 : use 1byte gpara, length is 10
			 * 06 01 00 00 00 00 01 DA 01 00 00 : use 2byte gpara, length is 11
			 */
			qc_cmd->msg.tx_len = 2;

			if (vdd->two_byte_gpara)
				qc_cmd->msg.tx_len++;
			if (vdd->pointing_gpara)
				qc_cmd->msg.tx_len++;

			payload = kvzalloc(qc_cmd->msg.tx_len, GFP_KERNEL);
			if (!payload) {
				LCD_ERR(vdd, "fail to alloc tx_buf\n");
				return NULL;
			}

			payload[pos++] = 0xB0;

			if (vdd->two_byte_gpara) {
				payload[pos++] = (rx_offset >> 8) & 0xff;
				payload[pos++] =  rx_offset & 0xff;
			} else {
				payload[pos++] = rx_offset & 0xff;
			}

			/* set pointing gpara */
			if (vdd->pointing_gpara)
				payload[pos] = rx_addr;

			qc_cmd->msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
			qc_cmd->last_command = true; /* transmit B0h before rx cmd */
			qc_cmd->msg.channel = 0;
			qc_cmd->msg.flags = 0;
			qc_cmd->post_wait_ms = 0;
			qc_cmd->msg.tx_buf = payload;
			qc_cmd->ss_txbuf = payload; /* ss_txbuf: deprecated... */
			qc_cmd->msg.rx_len = 0;
			qc_cmd->msg.rx_buf = NULL;

			qc_cmd->ss_cmd = ss_cmd;

			qc_cmd++;
		}

		/* 2. Set new read length */
		if (i == (rx_split_num - 1))
			rx_len = (tot_rx_len - RX_SIZE_LIMIT * i);
		else
			rx_len = RX_SIZE_LIMIT;

		/* 3. RX */
		qc_cmd->msg.rx_len = rx_len;
		qc_cmd->msg.type = ss_cmd->type;
		qc_cmd->last_command = true;
		qc_cmd->msg.channel = 0;
		qc_cmd->msg.flags = 0;

		/* delay only after last recv cmd */
		if (i == (rx_split_num - 1)) {
			if (ss_cmd->post_wait_ms)
				qc_cmd->post_wait_ms = ss_cmd->post_wait_ms;
			else if (ss_cmd->post_wait_frame)
				qc_cmd->post_wait_ms = ss_frame_to_ms(vdd,
							ss_cmd->post_wait_frame);
			else
				qc_cmd->post_wait_ms = 0;
		} else {
			qc_cmd->post_wait_ms = 0;
		}

		qc_cmd->msg.tx_len = ss_cmd->tx_len;
		qc_cmd->msg.tx_buf = ss_cmd->txbuf;
		qc_cmd->ss_txbuf = ss_cmd->txbuf; /* ss_txbuf: deprecated... */
		qc_cmd->msg.rx_buf = ss_cmd->rxbuf + (i * RX_SIZE_LIMIT);
		qc_cmd->ss_cmd = ss_cmd;

		qc_cmd++;
		rx_offset += RX_SIZE_LIMIT;
	}

	return --qc_cmd;
}

static struct dsi_cmd_desc *ss_pack_rx_no_gpara(struct samsung_display_driver_data *vdd,
		struct ss_cmd_desc *ss_cmd, struct dsi_cmd_desc *qc_cmd)
{
	qc_cmd->msg.type = ss_cmd->type;
	qc_cmd->last_command = true;
	qc_cmd->msg.channel = 0;
	qc_cmd->msg.flags = 0;

	if (ss_cmd->post_wait_ms)
		qc_cmd->post_wait_ms = ss_cmd->post_wait_ms;
	else if (ss_cmd->post_wait_frame)
		qc_cmd->post_wait_ms = ss_frame_to_ms(vdd,
					ss_cmd->post_wait_frame);
	else
		qc_cmd->post_wait_ms = 0;


	qc_cmd->msg.tx_len = ss_cmd->tx_len;
	qc_cmd->msg.tx_buf = ss_cmd->txbuf;
	qc_cmd->ss_txbuf = ss_cmd->txbuf; /* ss_txbuf: deprecated... */

	qc_cmd->msg.rx_len = ss_cmd->rx_len + ss_cmd->rx_offset;
	qc_cmd->msg.rx_buf = kvzalloc(qc_cmd->msg.rx_len, GFP_KERNEL);
	if (!qc_cmd->msg.rx_buf) {
		LCD_ERR(vdd, "fail to alloc qc rx_buf(%zd)\n", qc_cmd->msg.rx_len);
		return NULL;
	}

	qc_cmd->ss_cmd = ss_cmd;

	ss_cmd->rxbuf = qc_cmd->msg.rx_buf + ss_cmd->rx_offset;

	return qc_cmd;
}

int ss_bridge_qc_cmd_update(struct samsung_display_driver_data *vdd,
			struct ss_cmd_set *ss_set,
			bool no_gpara)
{
	struct dsi_panel_cmd_set *qc_set = &ss_set->base;
	struct dsi_cmd_desc *qc_cmd;
	struct ss_cmd_desc *ss_cmd = ss_set->cmds;
	int i;

	if (unlikely(!qc_set->cmds)) {
		LCD_ERR(vdd, "no qc cmd(%d)\n", ss_set->ss_type);
		return -ENOMEM;
	}

	qc_cmd = qc_set->cmds;

	for (i = 0; i < ss_set->count; ++i, ++ss_cmd, ++qc_cmd) {
		/* don't handle test key, should put test key in dtsi cmd */
		if (ss_is_read_ss_cmd(ss_cmd)) {
			if (!no_gpara)
				qc_cmd = ss_pack_rx_gpara(vdd, ss_cmd, qc_cmd);
			else
				qc_cmd = ss_pack_rx_no_gpara(vdd, ss_cmd, qc_cmd);

			if (!qc_cmd) {
				LCD_ERR(vdd, "fail to handle rx cmd(i: %d)\n", i);
				return -EFAULT;
			}
		} else {
			qc_cmd->msg.type = ss_cmd->type;

			qc_cmd->last_command = ss_cmd->last_command;
			qc_cmd->msg.channel = 0;
			qc_cmd->msg.flags = 0;

			/* post_wait_frame will be updated on runtime */
			qc_cmd->post_wait_ms = ss_cmd->post_wait_ms;

			qc_cmd->msg.tx_len = ss_cmd->tx_len;
			qc_cmd->msg.tx_buf = ss_cmd->txbuf;
			qc_cmd->ss_txbuf = ss_cmd->txbuf; /* ss_txbuf: deprecated... */
			qc_cmd->msg.rx_len = 0;
			qc_cmd->msg.rx_buf = NULL;

			/* TODO: add this connection to RX cmds... */
			qc_cmd->ss_cmd = ss_cmd;

			ss_cmd->qc_cmd = qc_cmd;
		}
	}

	switch (ss_set->state) {
	case SS_CMD_SET_STATE_LP:
		qc_set->state = DSI_CMD_SET_STATE_LP;
		break;
	case SS_CMD_SET_STATE_HS:
		qc_set->state = DSI_CMD_SET_STATE_HS;
		break;
	default:
		LCD_ERR(vdd, "invalid ss state(%d)\n", ss_set->state);
		return -EINVAL;
		break;
	}

	if (--qc_cmd != &qc_set->cmds[qc_set->count - 1] ||
			--ss_cmd != &ss_set->cmds[ss_set->count - 1]) {
		LCD_ERR(vdd, "unmatched countcritical count err(%d,%d)\n",
				qc_set->count, ss_set->count);
		return -EFAULT;
	}

	return 0;
}

static int ss_free_cmd_set(struct samsung_display_driver_data *vdd, struct ss_cmd_set *ss_set)
{
	struct dsi_panel_cmd_set *qc_set;
	void *prev_set_rev = NULL;
	int i;

	if (!ss_set)
		return -EINVAL;

	LCD_INFO(vdd, "free [%s]\n", ss_set->name ? ss_set->name : "no cmd name");

	/* free ss cmds */
	for (i = 0; i < ss_set->count; i++) {
		kfree_and_null(ss_set->cmds[i].txbuf);
		if (vdd->gpara)
			kfree_and_null(ss_set->cmds[i].rxbuf);
	}

	ss_set->count = 0;
	kfree_and_null(ss_set->cmds);
	kfree_and_null(ss_set->self_disp_cmd_set_rev);

	prev_set_rev = ss_set->cmd_set_rev[0];
	for (i = 1; i < SUPPORT_PANEL_REVISION; i++) {
		if (prev_set_rev == ss_set->cmd_set_rev[i]) {
			ss_set->cmd_set_rev[i] = NULL;
			continue;
		}

		prev_set_rev = ss_set->cmd_set_rev[i];
		kfree_and_null(ss_set->cmd_set_rev[i]);
	}

	/* free qc cmds */
	qc_set = &ss_set->base;
	for (i = 0; i < qc_set->count; i++) {
		if (!vdd->gpara)
			kfree_and_null(qc_set->cmds[i].msg.rx_buf);
	}

	qc_set->count = 0;
	kfree_and_null(qc_set->cmds);

	return 0;
}

int ss_is_prop_string_style(struct device_node *np, const char *buf, char *cmd_name, bool from_node)
{
	const char *data;
	u32 length = 0;
	int is_str;

	data = ss_wrapper_of_get_property(np, buf, cmd_name, &length, from_node);

	/* qc command style is
	 * samsung,mcd_on_tx_cmds_revA = [ 29 01 00 ...];,
	 * and it parses as integer value.
	 * It always '00' among the values,
	 * strlen is always smaller than  (prop->length - 1).
	 */

	if (data && strnlen(data, length) == (length - 1))
		is_str = 1;
	else
		is_str = 0;

	return is_str;
}

static int ss_copy_level_key_cmd(struct samsung_display_driver_data *vdd,
				struct ss_cmd_desc *cmd_dst, int type)
{
	struct ss_cmd_set *set_src = ss_get_ss_cmds(vdd, type);
	struct ss_cmd_desc *cmd_src = &set_src->cmds[0];

	*cmd_dst = *cmd_src;

	if (cmd_dst->tx_len <= 0) {
		LCD_ERR(vdd, "invalid tx_len(%lu), type: %d\n",
				cmd_dst->tx_len, type);
		return -EINVAL;
	}

	cmd_dst->qc_cmd = kvcalloc(1, sizeof(struct dsi_cmd_desc), GFP_KERNEL);
	if (!cmd_dst->qc_cmd) {
		LCD_ERR(vdd, "fail to allocate qc_cmd, type: %d\n", type);
		return -ENOMEM;
	}

	cmd_dst->txbuf = kvcalloc(cmd_dst->tx_len, sizeof(u8), GFP_KERNEL);
	if (!cmd_dst->txbuf) {
		LCD_ERR(vdd, "fail to allocate txbuf(%lu), type: %d\n",
				cmd_dst->tx_len, type);
		return -ENOMEM;
	}

	memcpy(cmd_dst->txbuf, cmd_src->txbuf, cmd_dst->tx_len * sizeof(u8));

	*cmd_dst->qc_cmd = *cmd_src->qc_cmd;
	cmd_dst->qc_cmd->ss_cmd = cmd_dst;
	cmd_dst->qc_cmd->ss_txbuf = cmd_dst->txbuf;
	cmd_dst->qc_cmd->msg.tx_buf = cmd_dst->txbuf;

	INIT_LIST_HEAD(&cmd_dst->op_list);

	return 0;
}

int ss_remove_duplicated_level_key(struct samsung_display_driver_data *vdd,
					struct ss_cmd_set *set)
{
	int tot_level_key = 0;
	int tot_level_key_cnt = 0;
	int level_key_cnt = 0;
	struct ss_cmd_desc *tmp_cmds;
	u32 org_count;

	int i, pos;

	tmp_cmds = kvzalloc(sizeof(struct ss_cmd_desc) * set->count, GFP_KERNEL);
	if (!tmp_cmds) {
		LCD_ERR(vdd, "allocation failed\n");
		return -ENOMEM;
	}

	tot_level_key = ss_tot_level_key(vdd, set);

	for (i = 0; i < set->count; i++) {
		tmp_cmds[i] = set->cmds[i];
		INIT_LIST_HEAD(&tmp_cmds[i].op_list);
		ss_copy_op_list(&tmp_cmds[i].op_list, &set->cmds[i].op_list);
	}

	if (tot_level_key & LEVEL0_KEY)
		tot_level_key_cnt++;
	if (tot_level_key & LEVEL1_KEY)
		tot_level_key_cnt++;
	if (tot_level_key & LEVEL2_KEY)
		tot_level_key_cnt++;

	for (i = 0; i < set->count; i++)
		level_key_cnt += set->cmds[i].skip_lvl_key;

	for (i = 0; i < set->count; i++)
		ss_free_op_list(&set->cmds[i].op_list);

	org_count = set->count;
	set->count += tot_level_key_cnt * 2 - level_key_cnt;

	kvfree(set->cmds);

	set->cmds = kvzalloc(set->count * sizeof(*set->cmds), GFP_KERNEL);
	if (!set->cmds) {
		LCD_ERR(vdd, "fail to allocate cmds\n");
		return -ENOMEM;
	}

	/* copy tot level keys on first */
	pos = 0;
	if (tot_level_key & LEVEL0_KEY)
		ss_copy_level_key_cmd(vdd, &set->cmds[pos++], TX_LEVEL0_KEY_ENABLE);

	if (tot_level_key & LEVEL1_KEY)
		ss_copy_level_key_cmd(vdd, &set->cmds[pos++], TX_LEVEL1_KEY_ENABLE);

	if (tot_level_key & LEVEL2_KEY)
		ss_copy_level_key_cmd(vdd, &set->cmds[pos++], TX_LEVEL2_KEY_ENABLE);

	/* copy original data except level keys */
	for (i = 0; i < org_count; i++) {
		if (tmp_cmds[i].skip_lvl_key) {
			/* move delay to previous command */
			set->cmds[pos - 1].post_wait_ms += tmp_cmds[i].post_wait_ms;
			set->cmds[pos - 1].post_wait_frame += tmp_cmds[i].post_wait_frame;

			continue;
		}

		set->cmds[pos] = tmp_cmds[i];
		INIT_LIST_HEAD(&set->cmds[pos].op_list);
		ss_copy_op_list(&set->cmds[pos].op_list, &tmp_cmds[i].op_list);
		ss_free_op_list(&tmp_cmds[i].op_list);
		pos++;
	}


	/* copy tot level key off later */
	if (tot_level_key & LEVEL2_KEY)
		ss_copy_level_key_cmd(vdd, &set->cmds[pos++], TX_LEVEL2_KEY_DISABLE);

	if (tot_level_key & LEVEL1_KEY)
		ss_copy_level_key_cmd(vdd, &set->cmds[pos++], TX_LEVEL1_KEY_DISABLE);

	if (tot_level_key & LEVEL0_KEY)
		ss_copy_level_key_cmd(vdd, &set->cmds[pos++], TX_LEVEL0_KEY_DISABLE);

	return 0;
}

int ss_wrapper_parse_cmd_sets(struct samsung_display_driver_data *vdd,
				struct ss_cmd_set *set, int type,
				struct device_node *np, const char *buf, char *cmd_name, bool from_node)
{
	int rc = 0;
	const char *data;
	u32 length = 0;
	u32 packet_count = 0;
	bool is_lp = false;
	bool no_gpara = false;
	char map_option[SS_CMD_PROP_STR_LEN + 3] = {0, };

	LCD_INFO_IF(vdd, "+ name: [%s]\n", cmd_name);

	set->name = cmd_name;

	/* set initial gpara option */
	no_gpara = !vdd->gpara;

	/* Samsung RX cmd is splited to multiple QCT cmds, with RX cmds and
	 * B0h grapam cmds with RX_LIMIT(10 bytets)
	 */
	data = ss_wrapper_of_get_property(np, buf, cmd_name, &length, from_node);
	if (!data) {
		LCD_DEBUG(vdd, "no matched data [%s]\n", cmd_name);
		return -ENOPARAM;
	}

	if (length == 1) {
		LCD_INFO(vdd, "dt: empty data [%s], len: %d\n", cmd_name, length);
		return -ENOPARAM;
	}

	/* To support LEGO symbol, check data to find LEGO symbol */
	data = ss_wrapper_parse_symbol(vdd, np, data, &length, cmd_name);

	if (length == 1) {
		LCD_INFO(vdd, "f_buf: empty data [%s], len: %d\n", cmd_name, length);
		return -ENOPARAM;
	}

	rc = ss_get_cmd_pkt_count(data, length, &packet_count);
	if (rc) {
		LCD_ERR(vdd, "fail to count pkt [%s](%d)\n", cmd_name, rc);
		return rc;
	}

	rc = ss_alloc_cmd_packets(set, packet_count);
	if (rc) {
		LCD_ERR(vdd, "fail to allocate cmd [%s](%d)\n", cmd_name, rc);
		return rc;
	}

	/* If samsung,mdss_dsi_on_tx_cmds_revA command need to be send on LP mode,
	 * Declare samsung,mdss_dsi_on_tx_cmds_revA_lp in panel dtsi.
	 * Default setting is HS mode for all ss commands.
	 */
	strcpy(map_option, cmd_name);
	strcat(map_option, "_lp");
	is_lp = of_property_read_bool(np, map_option);
	LCD_INFO_IF(vdd, "%s mode\n", is_lp ? "LP" : "HS");

	strcpy(map_option, cmd_name);
	strcat(map_option, "_skip_tot_lvl");
	set->is_skip_tot_lvl = of_property_read_bool(np, map_option);
	if (set->is_skip_tot_lvl)
		LCD_INFO_IF(vdd, "skip tot lvl: [%s]\n", ss_get_cmd_name(type));

	if (vdd->gpara) {
		strcpy(map_option, cmd_name);
		strcat(map_option, "_no_gpara");
		no_gpara = of_property_read_bool(np, map_option);
		if (no_gpara)
			LCD_INFO_IF(vdd, "skip gpara: [%s]\n", ss_get_cmd_name(type));
	}

	rc = ss_create_cmd_packets(vdd, data, length, packet_count, set->cmds, set, is_lp);
	if (rc) {
		LCD_ERR(vdd, "fail to create cmd(%d)\n", rc);
		goto err_free_cmds;
	}

	if (vdd->allow_level_key_once && !set->is_skip_tot_lvl &&
			type > TX_LEVEL_KEY_END) {
		/* check level key and remove those, and send level key only first and last */
		ss_remove_duplicated_level_key(vdd, set);
	}

	/* instead of calling ss_bridge_qc_cmd_alloc, alloc cmd here...???
	 * todo: care of qc_set for sub revisions...
	 */
	rc = ss_bridge_qc_cmd_alloc(vdd, set, no_gpara);
	if (rc) {
		LCD_ERR(vdd, "fail to alloc qc cmd(%d)\n", rc);
		goto err_free_cmds;
	}

	rc = ss_bridge_qc_cmd_update(vdd, set, no_gpara);
	if (rc) {
		LCD_ERR(vdd, "fail to update qc cmd(%d)\n", rc);
		goto err_free_cmds;
	}

	LCD_INFO_IF(vdd, "- name: [%s]\n", cmd_name);

	return 0;

err_free_cmds:
	ss_free_cmd_set(vdd, set);

	return rc;

}

/* Parse data from Sysfs node call */
int ss_wrapper_parse_cmd_sets_sysfs(struct samsung_display_driver_data *vdd, const char *buf, u32 length, const char *cmd_name)
{
	int rc = 0;
	u32 packet_count = 0;
	const char *data;
	struct ss_cmd_set *dummy_set;
	struct dsi_panel_cmd_set *qc_set;
	struct dsi_cmd_desc *qc_cmd;
	int i;

	data = buf;
	if (!data) {
		LCD_ERR(vdd, "no matched data\n");
		return -EINVAL;
	}

	if (length == 1) {
		LCD_INFO(vdd, "empty data [%s]\n", cmd_name);
		return 0;
	}

	/* Use Existing opcode storage data structure.
	 * To update Opcodes without re-flash,
	 * dummy_set = ss_get_ss_cmds() may help
	 */
	dummy_set = kvzalloc(sizeof(*dummy_set), GFP_KERNEL);
	if (!dummy_set) {
		LCD_INFO(vdd, "set allocation failed\n");
		return 0;
	}
	rc = ss_get_cmd_pkt_count(data, length, &packet_count);
	if (rc) {
		LCD_ERR(vdd, "fail to count pkt (%d)\n", rc);
		goto error;
	}
	LCD_INFO(vdd, "Packets : %d\n", packet_count);

	rc = ss_alloc_cmd_packets(dummy_set, packet_count);

	if (rc) {
		LCD_ERR(vdd, "fail to allocate cmd(%d)\n", rc);
		goto error;
	}

	/* parse the data into LEGO style Commands
	 * Eg:
	 * W 0xF0 0x5A 0x5A
	 * W 0x94 0xXX 0xXX
	 * IF VRR 120 THEN 0x5A 0x94
	 * ELSE 0xF0 0xF0
	 */
	rc = ss_create_cmd_packets(vdd, data, length, packet_count, dummy_set->cmds, dummy_set, 0);
	if (rc) {
		LCD_ERR(vdd, "fail to create cmd(%d)\n", rc);
		goto err_free_cmds;
	}

	/* check level key and remove those, and send level key only first and last */
	ss_remove_duplicated_level_key(vdd, dummy_set);

	rc = ss_bridge_qc_cmd_alloc(vdd, dummy_set, 0);
	if (rc) {
		LCD_ERR(vdd, "fail to alloc qc cmd(%d)\n", rc);
		goto err_free_cmds;
	}

	/* Create DSI Commands from extracted LEGO Commands
	 * Eg:
	 * 29 00 00 00 00 03 F0 5A 5A
	 * 29 00 00 00 00 03 94 5A 94
	 */
	rc = ss_bridge_qc_cmd_update(vdd, dummy_set, 0);
	if (rc) {
		LCD_ERR(vdd, "fail to update qc cmd(%d)\n", rc);
		goto err_free_cmds;
	}
	LCD_INFO(vdd, "Parsing completed\n");

	qc_set = &dummy_set->base;
	if (unlikely(!qc_set->cmds)) {
		LCD_ERR(vdd, "no qc cmd()\n");
		goto err_free_cmds;
	}

	/*
	 * To Process IF , ELSE and Delay Tags,
	 * Initially all conditional branch opcodes are generated,
	 * So Now we evaluate conditions to choose the correct one.
	 */
	ss_op_process(vdd, dummy_set);

	/* print all final commands.*/
	vdd->debug_data->print_cmds = true;
	LCD_INFO(vdd, "Command Packet Length : %d\n", qc_set->count);
	for (i = 0; i < qc_set->count; i++) {
		qc_cmd = &qc_set->cmds[i];
		ss_print_cmd_desc(qc_cmd, vdd);
	}
	vdd->debug_data->print_cmds = false;

/*clean and Exit */
err_free_cmds:
	if (!dummy_set)
		return -EINVAL;
	/* free ss cmds */
	for (i = 0; i < dummy_set->count; i++) {
		kfree_and_null(dummy_set->cmds[i].txbuf);
		if (vdd->gpara)
			kfree_and_null(dummy_set->cmds[i].rxbuf);
	}
	dummy_set->count = 0;
	kfree_and_null(dummy_set->cmds);
	/* free qc cmds */
	qc_set = &dummy_set->base;
	for (i = 0; i < qc_set->count; i++) {
		if (!vdd->gpara)
			kfree_and_null(qc_set->cmds[i].msg.rx_buf);
	}
	qc_set->count = 0;
	kfree_and_null(qc_set->cmds);
error:
	kvfree(dummy_set);
	return packet_count;
}

/* Return SS cmd format */
struct ss_cmd_set *ss_get_ss_cmds(struct samsung_display_driver_data *vdd, int type)
{
	struct ss_cmd_set *set;
	int rev = vdd->panel_revision;

	/* save the mipi cmd type */
	vdd->cmd_type = type;

	if (type < SS_DSI_CMD_SET_START) {
		LCD_DEBUG(vdd, "type(%d) is not ss cmd\n", type);
		return NULL;
	}

	/* SS cmds */
	set = &vdd->dtsi_data.ss_cmd_sets[type - SS_DSI_CMD_SET_START];

	if (set && set->cmd_set_rev[rev])
		set = set->cmd_set_rev[rev];

	return set;
}

int ss_print_rx_buf(struct samsung_display_driver_data *vdd, int type)
{
	struct ss_cmd_set *set;
	struct ss_cmd_desc *cmds;
	char dump_buffer[MAX_LEN_DUMP_BUF] = {0,};
	int pos;
	int i, j;

	if (type < SS_DSI_CMD_SET_START)
		return 0;

	set = ss_get_ss_cmds(vdd, type);
	cmds = set->cmds;

	for (i = 0; i < set->count; i++, cmds++) {
		if (!cmds->rx_len || !cmds->rxbuf)
			continue;

		pos = 0;
		for (j = 0; j < cmds->rx_len && pos < MAX_LEN_DUMP_BUF; j++)
			pos += snprintf(dump_buffer + pos, MAX_LEN_DUMP_BUF - pos,
					"%02x ", cmds->rxbuf[j]);

		LCD_INFO(vdd, "%s: [cmdid %d] addr: 0x%02X, off: 0x%02X, len: %zd, data: %s\n",
				ss_get_cmd_name(type), i, cmds->txbuf[0],
				cmds->rx_offset, cmds->rx_len, dump_buffer);
	}

	return 0;
}

/* reg: target rx address
 * cmd_order: Nth rx address value which we try to get
 */
int ss_get_rx_buf_addr(struct samsung_display_driver_data *vdd, int type,
			u8 addr, int cmd_order, u8 *out_buf, int out_len)
{
	struct ss_cmd_set *set;
	struct ss_cmd_desc *cmds;
	int order = 0;
	int i;

	if (type < SS_DSI_CMD_SET_START)
		return 0;

	set = ss_get_ss_cmds(vdd, type);
	cmds = set->cmds;

	for (i = 0; i < set->count; i++, cmds++) {
		if (!cmds->rx_len || !cmds->rxbuf)
			continue;

		if (cmds->txbuf[0] == addr && ++order == cmd_order) {
			if (out_len > cmds->rx_len) {
				LCD_ERR(vdd, "out_len(%d) > rx_len(%zd), addr: %02X, order: %d\n",
						out_len, cmds->rx_len, addr, cmd_order);
				return -EINVAL;
			}

			memcpy(out_buf, cmds->rxbuf, out_len);

			return 0;
		}
	}

	LCD_ERR(vdd, "fail to find %dth rx(%02x) buf\n", cmd_order, addr);

	return -EINVAL;
}

/* cmd_order: Nth rx value which we try to get
 */
int ss_get_rx_buf(struct samsung_display_driver_data *vdd, int type,
			int cmd_order, u8 *out_buf)
{
	struct ss_cmd_set *set;
	struct ss_cmd_desc *cmds;
	int order = 0;
	int i;

	if (type < SS_DSI_CMD_SET_START)
		return 0;

	set = ss_get_ss_cmds(vdd, type);
	cmds = set->cmds;

	for (i = 0; i < set->count; i++, cmds++) {
		if (!cmds->rx_len || !cmds->rxbuf)
			continue;

		if (++order == cmd_order) {
			memcpy(out_buf, cmds->rxbuf, cmds->rx_len);

			return 0;
		}
	}

	LCD_ERR(vdd, "fail to find %dth rx buf\n", cmd_order);

	return -EINVAL;
}

/* return total rx_len */
int ss_get_all_rx_buf(struct samsung_display_driver_data *vdd, int type,
			u8 *out_buf)
{
	struct ss_cmd_set *set;
	struct ss_cmd_desc *cmds;
	int i;
	int tot_len = 0;

	if (!out_buf) {
		LCD_ERR(vdd, "out buf is null\n");
		return -ENOMEM;
	}

	if (type < SS_DSI_CMD_SET_START)
		return -EINVAL;

	set = ss_get_ss_cmds(vdd, type);
	cmds = set->cmds;

	for (i = 0; i < set->count; i++, cmds++) {
		if (!cmds->rx_len || !cmds->rxbuf)
			continue;

		memcpy(out_buf, cmds->rxbuf, cmds->rx_len);
		out_buf += cmds->rx_len;
		tot_len += cmds->rx_len;
	}

	return tot_len;
}

/* return positive value: total rx_len
 * return negative value: error case
 * return zero: no received data.. error case
 */
int ss_send_cmd_get_rx(struct samsung_display_driver_data *vdd, int type,
				u8 *out_buf)
{
	int ret;

	/* To block read operation at esd-recovery */
	if (vdd->panel_dead) {
		LCD_ERR(vdd, "esd recovery, skip %s\n", ss_get_cmd_name(type));
		return -EPERM;
	}

	ret = ss_send_cmd(vdd, type);
	if (ret) {
		LCD_ERR(vdd, "fail to tx cmd(%s)\n", ss_get_cmd_name(type));
		return ret;
	}

	return ss_get_all_rx_buf(vdd, type, out_buf);
}
