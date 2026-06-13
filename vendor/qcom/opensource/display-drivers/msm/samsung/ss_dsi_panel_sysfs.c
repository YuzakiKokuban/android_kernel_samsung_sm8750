/*
 * =================================================================
 *
 *	Description:  samsung display sysfs common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2015, Samsung Electronics. All rights reserved.

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

#include "ss_dsi_panel_sysfs.h"
#include "ss_panel_power.h"

#if IS_ENABLED(CONFIG_SEC_PARAM)
#include <linux/samsung/sec_param.h>
#endif

/***************************************************************************************
 * FACTORY TEST
 ***************************************************************************************/

static ssize_t ss_disp_cell_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->cell_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_INFO(vdd, "no cell_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}
	strlcpy(buf, vdd->cell_id_dsi, vdd->cell_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);

	return strlen(buf);
}

static ssize_t ss_disp_octa_id_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->octa_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_INFO(vdd, "no octa_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}
	strlcpy(buf, vdd->octa_id_dsi, vdd->octa_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);

	return strlen(buf);
}

static ssize_t ss_w_coordinate_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->w_coordinate) {
		strlcpy(buf, "00000000", 9);
		LCD_INFO(vdd, "no w_coordinate set to default buf[%s]\n", buf);
		return strlen(buf);
	}
	strlcpy(buf, vdd->w_coordinate, vdd->w_coordinate_len);
	LCD_INFO(vdd, "[%s]\n", buf);

	return strlen(buf);
}

static ssize_t ss_disp_lcdtype_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	static int string_size = 100;
	char temp[100];
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strnlen(buf, string_size);
	}

	if (vdd->dtsi_data.tft_common_support && vdd->dtsi_data.tft_module_name) {
		if (vdd->dtsi_data.panel_vendor)
			snprintf(temp, 20, "%s_%s\n", vdd->dtsi_data.panel_vendor, vdd->dtsi_data.tft_module_name);
		else
			snprintf(temp, 20, "SDC_%s\n", vdd->dtsi_data.tft_module_name);
	} else if (ss_panel_attached(vdd->ndx)) {
		if (vdd->dtsi_data.panel_vendor)
			snprintf(temp, 20, "%s_%06X\n", vdd->dtsi_data.panel_vendor, vdd->manufacture_id_dsi);
		else
			snprintf(temp, 20, "SDC_%06X\n", vdd->manufacture_id_dsi);
	} else {
		LCD_INFO(vdd,"no manufacture id\n");
		snprintf(temp, 20, "SDC_000000\n");
	}

	strlcpy(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t ss_disp_windowtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int string_size = 15;
	char temp[15];
	int id, id1, id2, id3;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strnlen(buf, string_size);
	}

	/* If LCD_ID is needed before splash done(Multi Color Boot Animation), we should get LCD_ID form LK */
	if (vdd->manufacture_id_dsi == PBA_ID) {
		if (vdd->ndx == SECONDARY_DISPLAY_NDX)
			id = get_lcd_attached_secondary("GET");
		else
			id = get_lcd_attached("GET");
	} else {
		id = vdd->manufacture_id_dsi;
	}

	id1 = (id & 0x00FF0000) >> 16;
	id2 = (id & 0x0000FF00) >> 8;
	id3 = id & 0xFF;

	LCD_INFO(vdd,"%02x %02x %02x\n", id1, id2, id3);
	snprintf(temp, sizeof(temp), "%02x %02x %02x\n", id1, id2, id3);
	strlcpy(buf, temp, string_size);

	return strnlen(buf, string_size);
}

static ssize_t ss_disp_manufacture_date_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static int string_size = 30;
	char temp[30];
	int date;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strnlen(buf, string_size);
	}

	date = vdd->manufacture_date_dsi;
	snprintf((char *)temp, sizeof(temp), "manufacture date : %d\n", date);

	strlcpy(buf, temp, string_size);
	LCD_INFO(vdd, "manufacture date : %d\n", date);

	return strnlen(buf, string_size);
}

static ssize_t ss_disp_manufacture_code_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->ddi_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_ERR(vdd, "no ddi_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->ddi_id_dsi, vdd->ddi_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);

	return strlen(buf);
}

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
static ssize_t ss_disp_brightness_step(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 20, "%d\n", vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].tab_size);

	LCD_INFO(vdd,"brightness_step : %d", vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].tab_size);

	return rc;
}
#endif

static ssize_t ss_mafpc_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int i, len = 0, res = -1;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		res = -ENODEV;
	}

	if (!vdd->mafpc.is_support) {
		LCD_INFO(vdd, "mafpc is not supported..(%d) \n", vdd->mafpc.is_support);
		return -ENODEV;
	}


	if (vdd->mafpc.crc_check) {
		res = vdd->mafpc.crc_check(vdd);
	} else {
		LCD_INFO(vdd, "Do not support mafpc CRC check..\n");
	}

	if(!res) {
		len += scnprintf(buf + len, 60, "1 ");
	} else {
		len += scnprintf(buf + len, 60, "%d ", res);
	}

	if (vdd->mafpc.crc_size) {
		for (i = 0; i < vdd->mafpc.crc_size; i++) {
			len += scnprintf(buf + len, 60, "%02x ", vdd->mafpc.crc_read_data[i]);
			vdd->mafpc.crc_read_data[i] = 0x00;
		}
	}

	len += scnprintf(buf + len, 60, "\n");

	return strlen(buf);
}

/* Dynamic HLPM On/Off Test for Factory */
static ssize_t ss_dynamic_hlpm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int enable = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (!ss_is_panel_lpm(vdd)) {
		LCD_INFO(vdd, "Dynamic HLPM should be tested in LPM state Only. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (!vdd->self_disp.is_support) {
		LCD_INFO(vdd, "self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	if (sscanf(buf, "%d", &enable) != 1)
		return size;

	LCD_INFO(vdd,"Dynamic HLPM %s! (%d)\n", enable ? "Enable" : "Disable", enable);

	if (enable)
		ss_send_cmd(vdd, TX_DYNAMIC_HLPM_ENABLE);
	else
		ss_send_cmd(vdd, TX_DYNAMIC_HLPM_DISABLE);

	return size;
}

static ssize_t ss_self_move_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int pattern; /*pattern 0 => self move off, pattern 1 2 3 4 => self move on with each pattern*/
	struct dsi_display *display = NULL;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	display = GET_DSI_DISPLAY(vdd);
	if (IS_ERR_OR_NULL(display)) {
		LCD_INFO(vdd, "no display");
		return size;
	}

	if (!ss_is_panel_on(vdd)) {
		LCD_INFO(vdd, "Panel is not On state\n");
		return size;
	}

	mutex_lock(&display->display_lock);

	if (sscanf(buf, "%d", &pattern) != 1)
		goto end;

	if (pattern < 0 || pattern > 4) {
		LCD_INFO(vdd, "invalid input");
		goto end;
	}

	if (pattern) {
		LCD_INFO(vdd,"SELF MOVE ON pattern = %d\n", pattern);
		ss_send_cmd(vdd, TX_SELF_IDLE_AOD_ENTER);
		ss_send_cmd(vdd, TX_SELF_IDLE_TIMER_ON);
		ss_send_cmd(vdd, TX_SELF_IDLE_MOVE_ON_PATTERN1 + pattern - 1);
	}
	else {
		LCD_INFO(vdd,"SELF MOVE OFF\n");
		ss_send_cmd(vdd, TX_SELF_IDLE_TIMER_OFF);
		ss_send_cmd(vdd, TX_SELF_IDLE_MOVE_OFF);
		ss_send_cmd(vdd, TX_SELF_IDLE_AOD_EXIT);
	}
end:

	mutex_unlock(&display->display_lock);
	return size;
}

static ssize_t ss_self_mask_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int i, len = 0, res = -1;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		res = -ENODEV;
	}

	if (vdd->self_disp.mask_crc_force_pass) {
		res = 1;
		LCD_INFO(vdd, "Force pass %d..\n", vdd->self_disp.mask_crc_force_pass);
		goto end;
	}

	if (!vdd->self_disp.is_support) {
		LCD_INFO(vdd, "self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}


	if (vdd->self_disp.self_mask_check)
		res = !vdd->self_disp.self_mask_check(vdd);
	else
		LCD_INFO(vdd, "Do not support self mask check..\n");

end:
	len += scnprintf(buf + len, 60, "%d ", res);

	if (vdd->self_disp.mask_crc_size) {
		for (i = 0; i < vdd->self_disp.mask_crc_size; i++) {
			len += scnprintf(buf + len, 60, "%02x ", vdd->self_disp.mask_crc_read_data[i]);
			vdd->self_disp.mask_crc_read_data[i] = 0x00;
		}
	}

	len += scnprintf(buf + len, 60, "\n");

	return strlen(buf);
}

/*
 * Panel LPM related functions
 */
static ssize_t ss_panel_lpm_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
	(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct panel_func *pfunc;
	u8 current_status = 0;

	pfunc = &vdd->panel_func;

	if (IS_ERR_OR_NULL(pfunc)) {
		LCD_INFO(vdd, "no pfunc");
		return rc;
	}

	if (vdd->panel_lpm.is_support)
		current_status = vdd->panel_lpm.mode;

	rc = scnprintf((char *)buf, 30, "%d\n", current_status);
	LCD_INFO(vdd,"[Panel LPM] Current status : %d\n", (int)current_status);

	return rc;
}

#define LPM_MODE_MASK 0xff

static void set_lpm_mode_and_brightness(struct samsung_display_driver_data *vdd)
{
	/*default*/
	vdd->panel_lpm.mode = vdd->panel_lpm.origin_mode;
}

/*
 *	mode: 0x00aa00bb
 *	0xaa: ver 0: old lpm feature, ver 1: new lpm feature
 *	0xbb: lpm mode
 *		ver 0: 0/1/2/3/4
 *			(off/alpm 2nit/hlpm 2bit/alpm 60nit/ hlpm 60nit)
 *		ver 1: 1/2
 *			(alpm/hlpm)
 */
static void ss_panel_set_lpm_mode(
		struct samsung_display_driver_data *vdd, unsigned int mode)
{

	if (!vdd->panel_lpm.is_support) {
		LCD_INFO(vdd,"[Panel LPM][DIPSLAY_%d] LPM(ALPM/HLPM) is not supported\n", vdd->ndx);
		return;
	}

	mutex_lock(&vdd->panel_lpm.lpm_lock);
	vdd->panel_lpm.origin_mode = (u8)(mode & LPM_MODE_MASK);
	set_lpm_mode_and_brightness(vdd);
	mutex_unlock(&vdd->panel_lpm.lpm_lock);

	if (unlikely(vdd->is_factory_mode)) {
		if (vdd->panel_lpm.mode == LPM_MODE_OFF) {
			/* lpm -> normal on */
			ss_panel_power_ctrl(vdd, &vdd->panel_powers[PANEL_POWERS_LPM_OFF], true);
			ss_panel_low_power_config(vdd, false);
		} else {
			/* normal on -> lpm */
			ss_panel_power_ctrl(vdd, &vdd->panel_powers[PANEL_POWERS_LPM_ON], true);
			ss_panel_low_power_config(vdd, true);
		}
	}

	LCD_INFO(vdd, "[Panel LPM][DIPSLAY_%d]: mode(%d) brightness(%dnit)\n",
			vdd->ndx, vdd->panel_lpm.mode, vdd->br_info.common_br.cd_level);
}

static ssize_t ss_panel_lpm_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int mode = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &mode) != 1)
		return size;

	LCD_INFO(vdd,"[Panel LPM][DIPSLAY_%d] Mode : %d(%x) Index(%d)\n", vdd->ndx, mode, mode, vdd->ndx);
	ss_panel_set_lpm_mode(vdd, mode);

	return size;
}

/*
 * New AOD concept : 30HS + HLPM (From Eureka Project)
 * We should know what the panel mode is selected by user
 * Store value : 0 = Normal(30HS1) / 1 = HLPM
 */
static ssize_t ss_panel_hlpm_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &vdd->panel_lpm.hlpm_mode) != 1)
		return size;

	LCD_INFO(vdd,"HLPM Mode : %d\n", vdd->panel_lpm.hlpm_mode);
	return size;
}

static ssize_t ss_panel_hlpm_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
	(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 30, "%d\n", vdd->panel_lpm.hlpm_mode);
	LCD_INFO(vdd,"HLPM Mode : %d\n", vdd->panel_lpm.hlpm_mode);

	return rc;
}


static ssize_t mipi_samsung_mcd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		goto end;
	}

	if (sscanf(buf, "%d", &input) != 1)
		goto end;

	LCD_INFO(vdd,"input: %d, vrr_support_based_bl: %d, cur_rr: %d\n",
			input, vdd->vrr.support_vrr_based_bl,
			vdd->vrr.cur_refresh_rate);

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_MCD)) {
			LCD_INFO(vdd, "invalid mode, skip MCD test\n");
			goto end;
		}
	}

	if (vdd->panel_func.samsung_mcd_pre)
		vdd->panel_func.samsung_mcd_pre(vdd, input);

	if (input)
		ss_send_cmd(vdd, TX_MCD_ON);
	else
		ss_send_cmd(vdd, TX_MCD_OFF);

	if (vdd->panel_func.samsung_mcd_post)
		vdd->panel_func.samsung_mcd_post(vdd, input);

end:
	return size;
}

enum BRIGHTDOT_STATE {
	BRIGHTDOT_STATE_OFF = 0,
	BRIGHTDOT_STATE_ON = 1,
	BRIGHTDOT_LF_STATE_OFF = 2,
	BRIGHTDOT_LF_STATE_ON = 3,
	BRIGHTDOT_LF_STATE_MAX,
};

/* HOP display supports LFD mode using scan mode.
 * But, some pixel dots have high voltage leakage more than expected,
 * and it causes pixel dot blink issue in low LFD frequency and dark image..
 * To detect above "brightdot" issue, add brightdot sysfs.
 *
 * LF BRIGHTDOT test: lowers LFD min/max frequency to 0.5hz, and detect brightdot pixel.
 *
 * During brightdot test, prevent whole brightntess setting,
 * which changes brightdot setting.
 */
static ssize_t ss_brightdot_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	rc = sprintf(buf, "brightdot:%d lf_brightdot:%d\n",
			!!(vdd->brightdot_state & BIT(0)),
			!!(vdd->brightdot_state & BIT(1)));

	LCD_INFO(vdd,"state: %u, brightdot:%d lf_brightdot: %d\n",
			vdd->brightdot_state,
			!!(vdd->brightdot_state & BIT(0)),
			!!(vdd->brightdot_state & BIT(1)));

	return rc;
}

static ssize_t ss_brightdot_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	u32 input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		goto end;
	}

	if (sscanf(buf, "%d", &input) != 1 || input >= BRIGHTDOT_LF_STATE_MAX) {
		LCD_INFO(vdd, "invalid input(%u)\n", input);
		goto end;
	}

	LCD_INFO(vdd,"+ input: %u, state: %u, vrr_support_based_bl: %d, cur_rr: %d\n",
			input, vdd->brightdot_state,
			vdd->vrr.support_vrr_based_bl,
			vdd->vrr.cur_refresh_rate);

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_BRIGHTDOT)) {
			LCD_INFO(vdd, "invalid mode, skip brightdot test\n");
			goto end;
		}
	}

	mutex_lock(&vdd->bl_lock);
	switch (input) {
	case BRIGHTDOT_STATE_OFF:
		vdd->brightdot_state &= ~(u32)BIT(0);
		ss_send_cmd(vdd, TX_BRIGHTDOT_OFF);
		break;
	case BRIGHTDOT_STATE_ON:
		vdd->brightdot_state |= (u32)BIT(0);
		ss_send_cmd(vdd, TX_BRIGHTDOT_ON);
		break;
	case BRIGHTDOT_LF_STATE_OFF:
		vdd->brightdot_state &= ~(u32)BIT(1);
		ss_send_cmd(vdd, TX_BRIGHTDOT_LF_OFF);
		break;
	case BRIGHTDOT_LF_STATE_ON:
		vdd->brightdot_state |= (u32)BIT(1);
		ss_send_cmd(vdd, TX_BRIGHTDOT_LF_ON);
		break;
	default:
		break;
	};
	mutex_unlock(&vdd->bl_lock);

	LCD_INFO(vdd,"- state: %u\n", vdd->brightdot_state);

	/* BIT0: brightdot test, BIT1: brightdot test in LFD 0.5hz
	 * allow brightness update in both brightdot test off case
	 */
	if (!vdd->brightdot_state) {
		LCD_INFO(vdd,"brightdot test is done, update brightness\n");
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	}

end:
	return size;
}

static ssize_t mipi_samsung_mst_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	LCD_INFO(vdd,"(%d)\n", input);

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_MST)) {
			LCD_INFO(vdd, "invalid mode, skip MST test\n");
			goto end;
		}
	}

	if (input)
		ss_send_cmd(vdd, TX_MICRO_SHORT_TEST_ON);
	else
		ss_send_cmd(vdd, TX_MICRO_SHORT_TEST_OFF);
end:
	return size;
}

static ssize_t mipi_samsung_grayspot_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_GRAYSPOT)) {
			LCD_INFO(vdd, "invalid mode, skip GRAYSPOT test\n");
			goto end;
		}
	}

	LCD_INFO(vdd,"(%d)\n", input);

	if (input) {
		if (vdd->panel_func.samsung_gray_spot)
			vdd->panel_func.samsung_gray_spot(vdd, true);
		ss_send_cmd(vdd, TX_GRAY_SPOT_TEST_ON);
		vdd->grayspot = 1;
	} else {
		if (vdd->panel_func.samsung_gray_spot)
			vdd->panel_func.samsung_gray_spot(vdd, false);
		ss_send_cmd(vdd, TX_GRAY_SPOT_TEST_OFF);
		vdd->grayspot = 0;
		/* restore VINT, ELVSS */
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	}

end:
	return size;
}

static ssize_t mipi_samsung_vglhighdot_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	LCD_INFO(vdd,"(%d)\n", input);

	if (input == 1)
		ss_send_cmd(vdd, TX_VGLHIGHDOT_TEST_ON);
	else if (input == 2)
		ss_send_cmd(vdd, TX_VGLHIGHDOT_TEST_2);
	else
		ss_send_cmd(vdd, TX_VGLHIGHDOT_TEST_OFF);

end:
	return size;
}

static ssize_t xtalk_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		goto end;

	LCD_INFO(vdd,"(%d)\n", input);

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_XTALK)) {
			LCD_INFO(vdd, "invalid mode, skip XTALK test\n");
			goto end;
		}
	}

	if (!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_XTALK_ON)) &&
		!SS_IS_CMDS_NULL(ss_get_cmds(vdd, TX_XTALK_OFF))) {
		vdd->xtalk_mode_on = !!(input);
		if (input) {
			LCD_INFO(vdd, "TX_XTALK_ON & do not send whole bl cmd\n");
			ss_send_cmd(vdd, TX_XTALK_ON);
		} else {
			LCD_INFO(vdd, "TX_XTALK_OFF & return to normal bl\n");
			ss_send_cmd(vdd, TX_XTALK_OFF);
			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
		}

		return size;
	}

	if (input) {
		vdd->xtalk_mode = 1;
	} else {
		vdd->xtalk_mode = 0;
	}

	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
end:
	return size;
}

static ssize_t ecc_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int res = -EINVAL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		res = -ENODEV;
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	/* Panel specific processing is required */
	if (vdd->panel_func.ecc_read) {
		res = vdd->panel_func.ecc_read(vdd);
		if (!res)
			LCD_ERR(vdd, "test fail..\n");
	} else {
		u8 *val;
		struct ddi_test_mode *test_mode = &vdd->ddi_test[DDI_TEST_ECC];

		val = kzalloc(test_mode->pass_val_size, GFP_KERNEL);
		if (val) {
			if (ss_send_cmd_get_rx(vdd, RX_GCT_ECC, val) <= 0) {
				LCD_ERR(vdd, "fail to rx_ssr\n");
			} else {
				if (!memcmp(val, test_mode->pass_val, test_mode->pass_val_size))
					res = true;
			}

			kfree(val);
		} else {
			LCD_ERR(vdd, "fail to alloc read buffer ..\n");
		}
	}
end:
	snprintf(buf, 10, "%d", res);
	LCD_INFO(vdd, "[DDI_TEST_ECC] test %s\n", res == true ? "PASS" : "FAIL");

	return strlen(buf);
}

static ssize_t ssr_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int res = -EINVAL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		res = -ENODEV;
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	/* Panel specific processing is required */
	if (vdd->panel_func.ssr_read) {
		res = vdd->panel_func.ssr_read(vdd);
		if (!res)
			LCD_ERR(vdd, "test fail..\n");
	} else {
		u8 *val;
		struct ddi_test_mode *test_mode = &vdd->ddi_test[DDI_TEST_SSR];

		val = kzalloc(test_mode->pass_val_size, GFP_KERNEL);
		if (val) {
			if (ss_send_cmd_get_rx(vdd, RX_SSR, val) <= 0) {
				LCD_ERR(vdd, "fail to rx_ssr\n");
			} else {
				if (!memcmp(val, test_mode->pass_val, test_mode->pass_val_size))
					res = true;
			}

			kfree(val);
		} else {
			LCD_ERR(vdd, "fail to alloc read buffer ..\n");
		}
	}
end:
	snprintf(buf, 10, "%d", res);
	LCD_INFO(vdd, "[DDI_TEST_SSR] test %s\n", res == true ? "PASS" : "FAIL");

	return strlen(buf);
}


static int ss_gct_show(struct samsung_display_driver_data *vdd)
{
	int res;

	if (!vdd->gct.on)
		return GCT_RES_CHECKSUM_OFF;

	if (!memcmp(vdd->gct.checksum, vdd->gct.valid_checksum, 4))
		res = GCT_RES_CHECKSUM_PASS;
	else
		res = GCT_RES_CHECKSUM_NG;

	LCD_INFO(vdd, "res=%d panel= %02X %02X %02X %02X , valid= %02X %02X %02X %02X\n", res,
		vdd->gct.checksum[0], vdd->gct.checksum[1],
		vdd->gct.checksum[2], vdd->gct.checksum[3],
		vdd->gct.valid_checksum[0], vdd->gct.valid_checksum[1],
		vdd->gct.valid_checksum[2], vdd->gct.valid_checksum[3]);

	return res;
}

#define MAX_GCT_RLT_LEN	14
static ssize_t gct_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int res = -EINVAL;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		res = -ENODEV;
		goto end;
	}

	if (!vdd->gct.is_support) {
		LCD_ERR(vdd, "gct is not supported\n");
		res = GCT_RES_CHECKSUM_NOT_SUPPORT;
		goto end;
	}

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_GCT)) {
			LCD_INFO(vdd, "invalid mode, skip GCT test (force PASS)\n");
			res = 1;
			goto end;
		}
	}

	if (vdd->panel_func.samsung_gct_read)
		res = vdd->panel_func.samsung_gct_read(vdd);
	else
		res = ss_gct_show(vdd);
end:
	snprintf(buf, MAX_GCT_RLT_LEN, "%d 0x%02x%02x%02x%02x", res,
			vdd->gct.checksum[3], vdd->gct.checksum[2],
			vdd->gct.checksum[1], vdd->gct.checksum[0]);

	return strlen(buf);
}

static int ss_gct_store(struct samsung_display_driver_data *vdd)
{
	struct samsung_display_driver_data *vddp;
	struct sde_connector *conn;
	int i, ret = 0;

	LCD_INFO(vdd, "+++\n");

	ss_set_test_mode_state(vdd, PANEL_TEST_GCT);

	vdd->gct.is_running = true;

	/* prevent sw reset to trigger esd recovery */
	LCD_INFO(vdd, "disable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_GCT_TEST);

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		vddp = ss_get_vdd(i);
		if (!ss_is_panel_off(vddp)) {
			LCD_INFO(vddp, "gct : block commit\n");
			atomic_inc(&vddp->block_commit_cnt);
			ss_wait_for_kickoff_done(vddp);
			LCD_INFO(vddp, "gct : kickoff_done!!\n");
		}
	}

	ret = ss_send_cmd_get_rx(vdd, TX_GCT_LV, &vdd->gct.checksum[0]);
	if (ret <= 0)
		goto rx_err;

	ret = ss_send_cmd_get_rx(vdd, TX_GCT_HV, &vdd->gct.checksum[2]);
	if (ret <= 0)
		goto rx_err;

	LCD_INFO(vdd, "checksum = {%x %x %x %x}\n",
			vdd->gct.checksum[0], vdd->gct.checksum[1],
			vdd->gct.checksum[2], vdd->gct.checksum[3]);

	vdd->gct.on = 1;

	for (i = PRIMARY_DISPLAY_NDX; i < MAX_DISPLAY_NDX; i++) {
		vddp = ss_get_vdd(i);
		if (!ss_is_panel_off(vddp)) {
			LCD_INFO(vddp, "release commit\n");
			atomic_add_unless(&vddp->block_commit_cnt, -1, 0);
			wake_up_all(&vddp->block_commit_wq);
		}
	}

	vdd->gct.is_running = false;

	LCD_INFO(vdd, "tx ss_off_cmd\n");
	ss_send_cmd(vdd, TX_DSI_CMD_SET_OFF);

	ss_set_panel_state(vdd, PANEL_PWR_OFF);

	/* enable esd interrupt */
	LCD_INFO(vdd, "enable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd, ESD_MASK_GCT_TEST);

	/* hw reset (gct spec.) */
	LCD_INFO(vdd, "panel_dead event to reset panel\n");
	conn = GET_SDE_CONNECTOR(vdd);
	if (!conn)
		LCD_ERR(vdd, "fail to get valid conn\n");
	else
		schedule_work(&conn->status_work.work);

	do {
		flush_work(&conn->status_work.work);
		msleep(500);
		LCD_INFO(vdd, "Wait for panel on\n");
	} while (!ss_is_panel_on(vdd));

	LCD_INFO(vdd, "---\n");

	ss_set_test_mode_state(vdd, PANEL_TEST_NONE);

	return 0;

rx_err:
	LCD_ERR(vdd, "fail to read checksum via mipi(%d)\n", ret);

	return ret;
}

static ssize_t gct_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int ret = -1;

	LCD_INFO(vdd,"+\n");
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!ss_is_panel_on(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	if (!vdd->gct.is_support) {
		LCD_ERR(vdd, "gct is not supported\n");
		return -EPERM;
	}

	if (vdd->panel_func.samsung_gct_write)
		ret = vdd->panel_func.samsung_gct_write(vdd);
	else
		ret = ss_gct_store(vdd);

	LCD_INFO(vdd,"-: ret: %d\n", ret);

	return size;
}

static ssize_t ss_irc_mode_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 50, "%d\n", vdd->br_info.common_br.irc_mode);
	LCD_INFO(vdd,"irc_mode : %d\n", vdd->br_info.common_br.irc_mode);

	return rc;
}

static ssize_t ss_irc_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input_mode;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &input_mode) != 1)
		goto end;

	if (input_mode >= IRC_MAX_MODE) {
		LCD_INFO(vdd,"Invalid arg: %d\n", input_mode);
		goto end;
	}

	if (vdd->br_info.common_br.irc_mode != input_mode) {
		LCD_INFO(vdd,"irc mode: %d -> %d\n", vdd->br_info.common_br.irc_mode, input_mode);
		vdd->br_info.common_br.irc_mode = input_mode;

		if (!vdd->dtsi_data.tft_common_support)
			ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	}
end:
	return size;
}

#define MAX_PASS_VAL_LEN	(128)
static ssize_t ss_disp_flash_gamma_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int i, len = 0;
	bool pass = false;
	u8 val[MAX_PASS_VAL_LEN] = {0, };
	struct ddi_test_mode *test_mode;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_ERR(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return -EBUSY;
	}

	/* Panel specific processing is required */
	if (vdd->panel_func.ddi_flash_test) {
		len = vdd->panel_func.ddi_flash_test(vdd, buf);
		return strlen(buf);
	}

	test_mode = &vdd->ddi_test[DDI_TEST_FLASH_TEST];

	if (!test_mode->pass_val) {
		LCD_ERR(vdd, "pass_val is NULL\n");
		return -EINVAL;
	}

	if (ss_send_cmd_get_rx(vdd, RX_FLASH_LOADING_CHECK, val) <= 0) {
		LCD_ERR(vdd, "fail to rx_ssr\n");
		return -EPERM;
	}

	if (!memcmp(val, test_mode->pass_val, test_mode->pass_val_size))
		pass = true;

	len += sprintf(buf + len, "%d\n", 1);
	len += sprintf(buf + len, "%d ", pass == true ? 1 : 0);
	for (i = 0; i < test_mode->pass_val_size; i++)
		len += sprintf(buf + len, "%02X", val[i]);
	for (; i < 4; i++) /* dummy for eight digits */
		len += sprintf(buf + len, "00");
	len += sprintf(buf + len, "\n");

	LCD_INFO(vdd, "[DDI_TEST_FLASH_TEST] test %s\n", pass == true ? "OK" : "NG");

	return strlen(buf);
}

/**
 * ss_ccd_state_show()
 *
 * This function reads ccd state.
 */
static ssize_t ss_ccd_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int ret = 0;
	u8 ccd[2] = {0,};

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return ret;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return ret;
	}

	if (vdd->panel_func.samsung_ccd_read)
		return vdd->panel_func.samsung_ccd_read(vdd, buf);

	ret = ss_send_cmd_get_rx(vdd, TX_CCD_ON, ccd);
	if (ret <= 0) {
		LCD_ERR(vdd, "fail to read ccd state(%d)\n", ret);
		ret = snprintf((char *)buf, 6, "-1\n");
	} else {
		LCD_INFO(vdd, "CCD return (0x%02x)\n", ccd[0]);

		if (ccd[0] == vdd->ccd_pass_val)
			ret = scnprintf((char *)buf, 6, "1\n");
		else if (ccd[0] == vdd->ccd_fail_val)
			ret = scnprintf((char *)buf, 6, "0\n");
		else
			ret = scnprintf((char *)buf, 6, "-1\n");
	}

	ss_send_cmd(vdd, TX_CCD_OFF);

	return ret;
}

static ssize_t ss_panel_aging_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 40, "%d\n", vdd->panel_againg_status);

	LCD_INFO(vdd, "Panel Aging Status : %s", buf);

	return rc;
}

static ssize_t ss_panel_aging_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	if (input == 1) {
		ss_send_cmd(vdd, TX_PANEL_AGING_ON);
		vdd->panel_againg_status = true;
	} else if (input == 0) {
		ss_send_cmd(vdd, TX_PANEL_AGING_OFF);
		vdd->panel_againg_status = false;
	} else
		LCD_ERR(vdd, "Invalid Input (%d)\n", input);

	LCD_INFO(vdd, "(%d)\n", vdd->panel_againg_status);

end:
	return size;
}

static ssize_t ss_dsc_crc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct sde_connector *conn;
	struct ddi_test_mode *test_mode;
	int ret = 0;
	u8 dsc_crc[2] = {0,};

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return ret;
	}

	if (!ss_is_ready_to_send_cmd(vdd) || !vdd->display_on) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d), display_on(%d)\n",
				vdd->panel_state, vdd->display_on);
		return ret;
	}

	if (vdd->panel_func.samsung_dsc_crc_read)
		return vdd->panel_func.samsung_dsc_crc_read(vdd, buf);

	LCD_INFO(vdd, "+++\n");

	test_mode = &vdd->ddi_test[DDI_TEST_DSC_CRC];
	if (test_mode->pass_val == NULL) {
		LCD_ERR(vdd, "No pass_val for dsc_crc test..\n");
		return ret;
	}

	/* prevent sw reset to trigger esd recovery */
	LCD_INFO(vdd, "disable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_GCT_TEST);

	LCD_INFO(vdd, "block commit\n");
	ss_block_commit(vdd);
	LCD_INFO(vdd, "kickoff_done!!\n");

	ret = ss_send_cmd_get_rx(vdd, TX_DSC_CRC, dsc_crc);
	if (ret > 0) {
		if (!memcmp(dsc_crc, test_mode->pass_val, test_mode->pass_val_size)) {
			LCD_INFO(vdd, "PASS [%02X] [%02X]\n", dsc_crc[0], dsc_crc[1]);
			ret = scnprintf((char *)buf, 20, "1 %02x %02x\n", dsc_crc[0], dsc_crc[1]);
		} else {
			LCD_INFO(vdd, "FAIL [%02X] [%02X]\n", dsc_crc[0], dsc_crc[1]);
			ret = scnprintf((char *)buf, 20, "-1 %02x %02x\n", dsc_crc[0], dsc_crc[1]);
		}
	} else {
		ret = scnprintf((char *)buf, 6, "-1\n");
	}

	LCD_INFO(vdd, "release commit\n");
	ss_release_commit(vdd);

	LCD_INFO(vdd, "tx ss_off_cmd\n");
	ss_send_cmd(vdd, TX_DSI_CMD_SET_OFF);

	ss_set_panel_state(vdd, PANEL_PWR_OFF);

	/* enable esd interrupt */
	LCD_INFO(vdd, "enable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd, ESD_MASK_GCT_TEST);

	/* hw reset */
	LCD_INFO(vdd, "panel_dead event to reset panel\n");
	conn = GET_SDE_CONNECTOR(vdd);
	if (!conn)
		LCD_ERR(vdd, "fail to get valid conn\n");
	else
		schedule_work(&conn->status_work.work);

	do {
		flush_work(&conn->status_work.work);
		msleep(500);
		LCD_INFO(vdd, "Wait for panel on\n");
	} while (!ss_is_panel_on(vdd));

	LCD_INFO(vdd, "---\n");

	return ret;
}

static ssize_t mipi_samsung_hw_cursor_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input[10];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d %d %d %d %d %d %d %x %x %x", &input[0], &input[1],
			&input[2], &input[3], &input[4], &input[5], &input[6],
			&input[7], &input[8], &input[9]) != 10)
			goto end;

	if (!IS_ERR_OR_NULL(vdd->panel_func.ddi_hw_cursor))
		vdd->panel_func.ddi_hw_cursor(vdd, input);
	else
		LCD_INFO(vdd, "ddi_hw_cursor function is NULL\n");

end:
	return size;
}

static ssize_t ss_disp_SVC_OCTA_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->cell_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_INFO(vdd, "no cell_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->cell_id_dsi, vdd->cell_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);
	return strlen(buf);
}

static ssize_t ss_disp_SVC_OCTA2_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(SECONDARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->cell_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_INFO(vdd, "no cell_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->cell_id_dsi, vdd->cell_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);
	return strlen(buf);
}

static ssize_t ss_disp_SVC_OCTA_CHIPID_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->octa_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_INFO(vdd, "no octa_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->octa_id_dsi, vdd->octa_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);
	return strlen(buf);
}

static ssize_t ss_disp_SVC_OCTA2_CHIPID_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(SECONDARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->octa_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_INFO(vdd, "no octa_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->octa_id_dsi, vdd->octa_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);
	return strlen(buf);
}

static ssize_t ss_disp_SVC_OCTA_DDI_CHIPID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->ddi_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_ERR(vdd, "no ddi_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->ddi_id_dsi, vdd->ddi_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);
	return strlen(buf);
}

static ssize_t ss_disp_SVC_OCTA2_DDI_CHIPID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(SECONDARY_DISPLAY_NDX);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return strlen(buf);
	}

	if (!vdd->ddi_id_dsi) {
		strlcpy(buf, "0000000000", 11);
		LCD_ERR(vdd, "no ddi_id_dsi set to default buf[%s]\n", buf);
		return strlen(buf);
	}

	strlcpy(buf, vdd->ddi_id_dsi, vdd->ddi_id_len);
	LCD_INFO(vdd, "[%s]\n", buf);
	return strlen(buf);
}

/***************************************************************************************
 * DPUI/DPCI
 ***************************************************************************************/
static int dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct samsung_display_driver_data *vdd = container_of(self,
			struct samsung_display_driver_data, dpui_notif);
	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	int lcd_id;
	int size;
	u8 *octa_id;

	if (dpui == NULL) {
		LCD_INFO(vdd, "err: dpui is null\n");
		return 0;
	}

	if (vdd == NULL) {
		LCD_INFO(vdd, "err: vdd is null\n");
		return 0;
	}

	LCD_INFO(vdd, "++\n");

	/* MAID DATE */
	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d %04d00", vdd->manufacture_date_dsi, vdd->manufacture_time_dsi);
	set_dpui_field(DPUI_KEY_MAID_DATE, tbuf, size);

	/* lcd id */
	lcd_id = vdd->manufacture_id_dsi;

	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", ((lcd_id  & 0xFF0000) >> 16));
	set_dpui_field(DPUI_KEY_LCDID1, tbuf, size);
	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", ((lcd_id  & 0xFF00) >> 8));
	set_dpui_field(DPUI_KEY_LCDID2, tbuf, size);
	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", (lcd_id  & 0xFF));
	set_dpui_field(DPUI_KEY_LCDID3, tbuf, size);

	/* cell id */
	if (vdd->cell_id_dsi) {
		size = strlcpy(tbuf, vdd->cell_id_dsi, vdd->cell_id_len);
	} else {
		LCD_ERR(vdd, "cell_id_dsi is null\n");
		size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "0\n");
	}
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		set_dpui_field(DPUI_KEY_CELLID, tbuf, size);
	else
		set_dpui_field(DPUI_KEY_SUB_CELLID, tbuf, size);

	/* OCTA ID */
	if (vdd->octa_id_dsi) {
		octa_id = vdd->octa_id_dsi;
		size = strlcpy(tbuf, vdd->octa_id_dsi, vdd->octa_id_len);
	} else {
		LCD_ERR(vdd, "octa_id_dsi is null\n");
		size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "0\n");
	}
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		set_dpui_field(DPUI_KEY_OCTAID, tbuf, size);
	else
		set_dpui_field(DPUI_KEY_SUB_OCTAID, tbuf, size);

	/* Panel Gamma Flash Loading Result */
	/*READ_FAIL_NOT_LOADING = -1..  TODO: check if we can remove this.. */
	size = scnprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", -1);
	set_dpui_field(DPUI_KEY_PNGFLS, tbuf, size);

	/* ub_con cnt */
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		inc_dpui_u32_field(DPUI_KEY_UB_CON, vdd->ub_con_det.ub_con_cnt);
	else
		inc_dpui_u32_field(DPUI_KEY_SUB_UB_CON, vdd->ub_con_det.ub_con_cnt);
	vdd->ub_con_det.ub_con_cnt = 0;

	return 0;
}

static int ss_register_dpui(struct samsung_display_driver_data *vdd)
{
	memset(&vdd->dpui_notif, 0, sizeof(vdd->dpui_notif));
	vdd->dpui_notif.notifier_call = dpui_notifier_callback;

	return dpui_logging_register(&vdd->dpui_notif, DPUI_TYPE_PANEL);
}

/*
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t ss_dpui_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);

	return ret;
}

static ssize_t ss_dpui_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);

	return size;
}

/*
 * [DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t ss_dpui_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);

	return ret;
}

static ssize_t ss_dpui_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);

	return size;
}

/*
 * [AP DEPENDENT ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t ss_dpci_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);

	return ret;
}

static ssize_t ss_dpci_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);

	return size;
}

/*
 * [AP DEPENDENT DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t ss_dpci_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t ss_dpci_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);

	return size;
}

/***************************************************************************************
 * PANEL FUNCTION
 ***************************************************************************************/

static ssize_t ss_disp_gamma_comp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int len = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (vdd->panel_func.samsung_print_gamma_comp)
		vdd->panel_func.samsung_print_gamma_comp(vdd);

end:
	return len;
}

static ssize_t ss_disp_gamma_comp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	if (vdd->panel_func.samsung_gm2_gamma_comp_init)
		vdd->panel_func.samsung_gm2_gamma_comp_init(vdd);

	return size;
}

static ssize_t ss_night_dim_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 40, "%d\n", vdd->night_dim);

	LCD_INFO(vdd, "night_dim : %d\n", vdd->night_dim);

	return rc;
}

static ssize_t ss_night_dim_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int val;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &val) != 1)
		return size;

	LCD_INFO(vdd, "val [%d]\n", val);

	if (val < 0 || val > MAX_NIGHT_DIM_IDX) {
		LCD_ERR(vdd, "val is invalid %d\n", MAX_NIGHT_DIM_IDX);
		return size;
	}

	vdd->night_dim = val;
	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

	return size;
}

/* SAMSUNG_FINGERPRINT */
static ssize_t ss_finger_hbm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int value;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &value) != 1)
		return size;

	LCD_INFO(vdd,"mask_bl_level value : %d\n", value);
	vdd->br_info.common_br.finger_mask_bl_level = value;

	return size;

}

static ssize_t ss_finger_hbm_updated_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (vdd->finger_mask)
		sprintf(buf, "%d\n", vdd->br_info.common_br.finger_mask_bl_level);
	else
		sprintf(buf, "%d\n", vdd->finger_mask);

	LCD_INFO(vdd,"vdd->br_info.common_br.actual_mask_brightness value : %x\n", vdd->finger_mask);

	return strlen(buf);
}

static ssize_t ss_fp_green_circle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int val = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &val) != 1)
		return size;

#if IS_ENABLED(CONFIG_SEC_FACTORY)
	if (val)
		ss_send_cmd(vdd, TX_SELF_MASK_GREEN_CIRCLE_ON_FACTORY);
	else
		ss_send_cmd(vdd, TX_SELF_MASK_GREEN_CIRCLE_OFF_FACTORY);
#else
	if (val)
		ss_send_cmd(vdd, TX_SELF_MASK_GREEN_CIRCLE_ON);
	else
		ss_send_cmd(vdd, TX_SELF_MASK_GREEN_CIRCLE_OFF);
#endif
	LCD_INFO(vdd,"Finger Print Green Circle : %d\n", val);

end:
	return size;
}

static ssize_t ss_local_hbm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int val = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	val = scnprintf((char *)buf, 10, "%d\n", vdd->local_hbm_val);
	LCD_INFO(vdd, "local_hbm_val (%d)\n", vdd->local_hbm_val);

	return val;
}

static ssize_t ss_local_hbm_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int val = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &val) != 1)
		return size;

	LCD_INFO(vdd, "LOCAL_HBM : [%d]\n", val);
	if (val == 1) {
		vdd->local_hbm_val = 1;
		ss_send_cmd(vdd, TX_LOCAL_HBM_ENTER);
		ss_send_cmd(vdd, TX_LOCAL_HBM_ON_CIRCLE_OFF);
	} else if (val == 2) {
		vdd->local_hbm_val = 2;
		ss_send_cmd(vdd, TX_LOCAL_HBM_ON_CIRCLE_ON);
	} else {
		vdd->local_hbm_val = 0;
		ss_send_cmd(vdd, TX_LOCAL_HBM_EXIT);
	}
end:
	return size;
}

static ssize_t ss_ub_con_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int ret = 0;

	if (!ss_gpio_is_valid(vdd->ub_con_det.gpio)) {
		LCD_INFO(vdd, "No ub_con_det gpio..\n");
		ret = scnprintf(buf, 20, "-1\n");
	} else
		ret = scnprintf(buf, 20, ss_gpio_get_value(vdd, vdd->ub_con_det.gpio) ? "disconnected\n" : "connected\n");

	return ret;
}

static ssize_t ss_ub_con_det_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int value;
	int ub_con_gpio;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &value) != 1)
		return size;

	if (!ss_gpio_is_valid(vdd->ub_con_det.gpio)) {
		LCD_INFO(vdd, "ub_con_det gpio is not valid ..\n");
		value = 0;
	}

	if (value) {
		vdd->ub_con_det.enabled = true;
		ub_con_gpio = ss_gpio_get_value(vdd, vdd->ub_con_det.gpio);
		LCD_INFO(vdd, "ub con gpio = %d\n", ub_con_gpio);
		/* Once enable ub con det, check ub status first */
		if (ub_con_gpio)
			ss_send_ub_uevent(vdd);
	} else {
		vdd->ub_con_det.enabled = false;
	}

	LCD_INFO(vdd,"ub_con_det - %s \n", vdd->ub_con_det.enabled ? "[enabled]" : "[disabled]");

	return size;
}

/* controlled by mm service */
static ssize_t ss_dia_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int val = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &val) != 1)
		return size;

	vdd->dia_off = !val;
	ss_send_cmd(vdd, TX_SS_DIA);

	LCD_INFO(vdd,"DIA : %d\n", val);

end:
	return size;
}

static ssize_t ss_disp_acl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, sizeof(vdd->br_info.acl_status), "%d\n", vdd->br_info.acl_status);

	LCD_INFO(vdd, "acl status: %d\n", *buf);

	return rc;
}

static ssize_t ss_disp_acl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int acl_set = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sysfs_streq(buf, "1"))
		acl_set = true;
	else if (sysfs_streq(buf, "0"))
		acl_set = false;
	else
		LCD_INFO(vdd, "Invalid argument!!");

	LCD_INFO(vdd, "(%d)\n", acl_set);

	if ((acl_set && !vdd->br_info.acl_status) ||
			(!acl_set && vdd->br_info.acl_status)) {
		vdd->br_info.acl_status = acl_set;
		if (!ss_is_ready_to_send_cmd(vdd)) {
			LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
			return size;
		}
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	} else {
		vdd->br_info.acl_status = acl_set;
		LCD_INFO(vdd,"skip acl update!! acl %d", vdd->br_info.acl_status);
	}

	return size;
}

static ssize_t ss_disp_siop_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, sizeof(vdd->siop_status), "%d\n", vdd->siop_status);

	LCD_INFO(vdd,"siop status: %d\n", *buf);

	return rc;
}

static ssize_t ss_disp_siop_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int siop_set = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sysfs_streq(buf, "1"))
		siop_set = true;
	else if (sysfs_streq(buf, "0"))
		siop_set = false;
	else
		LCD_INFO(vdd,"Invalid argument!!");

	LCD_INFO(vdd,"(%d)\n", siop_set);

	vdd->siop_status = siop_set;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}
	if (siop_set && !vdd->siop_status) {
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	} else if (!siop_set && vdd->siop_status) {
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	} else {
		LCD_INFO(vdd,"skip siop ss_brightness_dcs!! acl %d", vdd->br_info.acl_status);
	}

	return size;
}

#define read_buf_max (256)
unsigned char readbuf[read_buf_max] = {0x0, };
unsigned int readaddr, readlen, readpos;
static ssize_t ss_read_mtp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i, len = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (readlen && (readlen < read_buf_max)) {
		len += scnprintf(buf + len, 50, "addr[%X] pos[%x] len[%d] : ", readaddr, readpos, readlen);
		for (i = 0; i < readlen; i++)
			len += scnprintf(buf + len, 10, "%02x%s", readbuf[i], (i == readlen - 1) ? "\n" : " ");
	} else {
		len += scnprintf(buf + len, 100, "No read data.. \n");
		len += scnprintf(buf + len, 100, "Please write (addr gpara len) to read_mtp store to read mtp\n");
		len += scnprintf(buf + len, 100, "ex) echo CA 1 20 > read_mtp \n");
		len += scnprintf(buf + len, 100, "- read 20bytes of CAh register from 2nd para. \n");
	}

	return strlen(buf);
}

static ssize_t ss_read_mtp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int temp[3];

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%x %d %d", &temp[0], &temp[1], &temp[2]) != 3)
		return size;

	mutex_lock(&vdd->bl_lock);

	readaddr = temp[0];
	readpos = temp[1];
	readlen = temp[2];

	if (vdd->two_byte_gpara) {
		if (readaddr > 0xFF || readpos > 0xFFFF || readlen > 0xFF) {
			readaddr = readpos = readlen = 0;
			goto err;
		}
	} else {
		if (readaddr > 0xFF || readpos > 0xFF || readlen > 0xFF) {
			readaddr = readpos = readlen = 0;
			goto err;
		}
	}

	LCD_INFO(vdd,"addr 0x(%x) pos(%d) len (%d)\n", readaddr, readpos, readlen);

	memset(readbuf, 0x00, read_buf_max);
	ss_read_mtp_sysfs(vdd, readaddr, readlen, readpos, readbuf);

err:
	mutex_unlock(&vdd->bl_lock);
	return size;
}


static ssize_t ss_write_mtp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	char *p, *arg = (char *)buf;
	u8 *tx_buf = NULL;
	u8 tmp_buf[512] = {0,};
	int len = 0;
	int val = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	while (((p = strsep(&arg, " ")) != NULL) && (len < 512)) {
		if (sscanf(p, "%02x", &val) != 1)
			LCD_INFO(vdd,"fail to sscanf..\n");
		tmp_buf[len++] = val & 0xFF;
		LCD_INFO(vdd,"arg [%02x] \n", val);
	}

	LCD_INFO(vdd,"cmd len : %d\n", len);

	tx_buf = kzalloc(len, GFP_KERNEL);
	if (!tx_buf) {
		LCD_INFO(vdd,"Fail to kmalloc for tx_buf..\n");
		goto err;
	}

	for (i = 0; i < len; i++)
		tx_buf[i] = tmp_buf[i];

	ss_write_mtp(vdd, len, tx_buf);

	kfree(tx_buf);
	tx_buf = NULL;

err:
	return size;
}

static ssize_t ss_temperature_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i, rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	if (vdd->temp_table.size) {
		for (i = 0; i < vdd->temp_table.size; i++)
			rc += scnprintf(buf + rc, 40, "%d ", vdd->temp_table.val[i]);
		rc += scnprintf(buf + rc, 40, "\n");

		LCD_INFO(vdd, "cur temperature : %d, temperature_table : (%d) %s\n",
			vdd->br_info.temperature, vdd->temp_table.size, buf);
	} else {
		LCD_ERR(vdd, "No temperature table..\n");
	}

	return rc;
}

static ssize_t ss_temperature_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct samsung_display_driver_data *vdd_main =
						ss_get_vdd(PRIMARY_DISPLAY_NDX);
	struct samsung_display_driver_data *vdd_sub =
						ss_get_vdd(SECONDARY_DISPLAY_NDX);
	int pre_temp = 0, temp;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "no vdd\n");
		return size;
	}

	pre_temp = vdd->br_info.temperature;

	if (sscanf(buf, "%d", &temp) != 1)
		return size;

	vdd_main->br_info.temperature = temp;
	vdd_sub->br_info.temperature = temp;
	LCD_INFO(vdd, "temperature : %d\n", temp);

	/* When temperature changed, vdd->br_info.is_hbm must setted 0 for EA8061 hbm setting. */
	if (pre_temp != vdd_main->br_info.temperature) {
		if (vdd_main->br_info.is_hbm == 1)
			vdd_main->br_info.is_hbm = 0;
		if (vdd_main->br_info.is_hbm == 1)
			vdd_main->br_info.is_hbm = 0;
	}

	if (ss_is_ready_to_send_cmd(vdd_main)) {
		ss_brightness_dcs(vdd_main, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
		LCD_INFO(vdd_main, "temperature : %d\n", vdd_main->br_info.temperature);
	} else {
		LCD_ERR(vdd_main, "Panel is not ready. Panel State(%d)\n", vdd_main->panel_state);
	}

	if (vdd_sub->ndx == SECONDARY_DISPLAY_NDX) {
		if (ss_is_ready_to_send_cmd(vdd_sub)) {
			ss_brightness_dcs(vdd_sub, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
			LCD_INFO(vdd_sub, "temperature : %d\n", vdd_sub->br_info.temperature);
		} else {
			LCD_ERR(vdd_sub, "Panel is not ready. Panel State(%d)\n", vdd_sub->panel_state);
		}
	}

	return size;
}

static ssize_t ss_lux_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 40, "%d\n", vdd->br_info.lux);

	LCD_INFO(vdd,"lux : %d\n", vdd->br_info.lux);

	return rc;
}

static ssize_t ss_lux_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int pre_lux = 0, temp;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	pre_lux = vdd->br_info.lux;

	if (sscanf(buf, "%d", &temp) != 1)
		return size;

	vdd->br_info.lux = temp;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (vdd->mdnie.support_mdnie && pre_lux != vdd->br_info.lux)
		update_dsi_tcon_mdnie_register(vdd);

	LCD_INFO(vdd,"lux : %d\n", vdd->br_info.lux);

	return size;
}

static ssize_t ss_smooth_dim_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return rc;
	}

	rc = scnprintf((char *)buf, 40, "%d\n", vdd->br_info.common_br.smooth_dim_off ? 0 : 1);

	LCD_INFO(vdd,"Smooth Dimming : %s", buf);

	return rc;
}

/*
 * ss_smooth_dim_store()
 * 0 : Smooth Dimming Off
 * others : Smooth Dimming On (Default)
 */
static ssize_t ss_smooth_dim_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	if (input == 0)
		vdd->br_info.common_br.smooth_dim_off = true;
	else
		vdd->br_info.common_br.smooth_dim_off = false;

	/* Smooth diming change will be applied next brightness change */
	/* Do not call ss_brightness_dcs function here */

	LCD_INFO(vdd, "Smooth Dimming [%s]\n", input ? "On" : "Off");

	return size;
}


/**
 * ss_brt_avg_show()
 *
 * This function shows cd avg (AFC).
 * If not this function returns -1.
 * Please enable 'ss_copr_panel_init' in panel.c file to cal cd_avr for ABC(mafpc).
 */
static ssize_t ss_brt_avg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int ret = 0;
	int idx = COPR_CD_INDEX_1;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return ret;
	}

	if (!ss_is_panel_on(vdd)) {
		LCD_INFO(vdd, "panel stste (%d) \n", vdd->panel_state);
		return ret;
	}

	LCD_DEBUG(vdd, "++ \n");

	mutex_lock(&vdd->copr.copr_lock);

	/* get cd avg */
	ss_set_copr_sum(vdd, idx);
	vdd->copr.copr_cd[idx].cd_avr = vdd->copr.copr_cd[idx].cd_sum / vdd->copr.copr_cd[idx].total_t;
	LCD_INFO(vdd,"[%d] cd_avr (%d) cd_sum (%lld) total_t (%lld)\n",
		idx,
		vdd->copr.copr_cd[idx].cd_avr, vdd->copr.copr_cd[idx].cd_sum,
		vdd->copr.copr_cd[idx].total_t);

	vdd->copr.copr_cd[idx].cd_sum = 0;
	vdd->copr.copr_cd[idx].total_t = 0;

	ret = scnprintf((char *)buf, 10, "%d\n", vdd->copr.copr_cd[idx].cd_avr);

	mutex_unlock(&vdd->copr.copr_lock);

	LCD_DEBUG(vdd, "-- \n");

	return strlen(buf);
}

static ssize_t ss_self_mask_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int enable = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (!vdd->self_disp.is_support) {
		LCD_INFO(vdd, "self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}


	if (sscanf(buf, "%d", &enable) != 1)
		return size;

	LCD_INFO(vdd,"SELF MASK %s! (%d)\n", enable ? "enable" : "disable", enable);
	if (vdd->self_disp.self_mask_on)
		vdd->self_disp.self_mask_on(vdd, enable);
	else
		LCD_INFO(vdd, "Self Mask Function is NULL\n");

	return size;
}

static ssize_t ss_self_mask_udc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int enable = 0;
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (!vdd->self_disp.is_support) {
		LCD_INFO(vdd, "self display is not supported..(%d) \n",
								vdd->self_disp.is_support);
		return -ENODEV;
	}

	if (sscanf(buf, "%d", &enable) != 1)
		return size;

	vdd->self_disp.udc_mask_enable = enable;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d) enable(%d)\n",
			vdd->panel_state, enable);
		return size;
	}

	if (vdd->self_disp.self_mask_udc_on)
		vdd->self_disp.self_mask_udc_on(vdd, vdd->self_disp.udc_mask_enable);
	else
		LCD_INFO(vdd, "Self Mask UDC Function is NULL\n");

	return size;
}

static ssize_t mipi_samsung_hmt_bright_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!vdd->hmt.is_support) {
		LCD_INFO(vdd, "hmt is not supported..\n");
		return -ENODEV;
	}

	rc = scnprintf((char *)buf, 30, "%d\n", vdd->hmt.bl_level);
	LCD_INFO(vdd,"[HMT] hmt bright : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_bright_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;

	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!vdd->hmt.is_support) {
		LCD_INFO(vdd, "hmt is not supported..\n");
		return -ENODEV;
	}

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_HMD)) {
			LCD_INFO(vdd, "invalid mode, fail to turn on HMT\n");
			return -EPERM;
		}
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	LCD_INFO(vdd,"[HMT] input (%d) ++\n", input);

	if (!vdd->hmt.hmt_on) {
		LCD_INFO(vdd,"[HMT] hmt is off!\n");
		goto end;
	}

	if (!ss_is_panel_on(vdd)) {
		LCD_INFO(vdd, "[HMT] panel is not on state (%d) \n", vdd->panel_state);
		vdd->hmt.bl_level = input;
		goto end;
	}

	if (vdd->hmt.bl_level == input) {
		LCD_INFO(vdd, "[HMT] hmt bright already %d!\n", vdd->hmt.bl_level);
		goto end;
	}

	vdd->hmt.bl_level = input;
	hmt_bright_update(vdd);
	LCD_INFO(vdd,"[HMT] input (%d) --\n", input);

end:
	return size;
}

static ssize_t mipi_samsung_hmt_on_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int rc;

	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!vdd->hmt.is_support) {
		LCD_INFO(vdd, "hmt is not supported..\n");
		return -ENODEV;
	}

	rc = scnprintf((char *)buf, 30, "%d\n", vdd->hmt.hmt_on);
	LCD_INFO(vdd,"[HMT] hmt on input : %d\n", *buf);

	return rc;
}

static ssize_t mipi_samsung_hmt_on_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int input;
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (!vdd->hmt.is_support) {
		LCD_INFO(vdd, "hmt is not supported..\n");
		return -ENODEV;
	}

	if (vdd->panel_func.samsung_check_support_mode) {
		if (!vdd->panel_func.samsung_check_support_mode(vdd, CHECK_SUPPORT_HMD)) {
			LCD_INFO(vdd, "invalid mode, fail to turn on HMT\n");
			return -EPERM;
		}
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	LCD_INFO(vdd,"[HMT] input (%d) (VRR: %dHZ%s) ++\n", input,
			vdd->vrr.cur_refresh_rate,
			vdd->vrr.cur_sot_hs_mode ? "HS" : "NS");

	if (!ss_is_panel_on(vdd)) {
		LCD_INFO(vdd, "[HMT] panel is not on state (%d) \n", vdd->panel_state);
		vdd->hmt.hmt_on = !!input;
		return size;
	}

	if (vdd->hmt.hmt_on == !!input) {
		LCD_INFO(vdd,"[HMT] hmt already %s !\n", vdd->hmt.hmt_on?"ON":"OFF");
		return size;
	}

	vdd->hmt.hmt_on = input;

	hmt_enable(vdd);
	hmt_bright_update(vdd);

	LCD_INFO(vdd,"[HMT] input hmt (%d) --\n",
		vdd->hmt.hmt_on);

	return size;
}

static ssize_t ss_adaptive_control_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int value;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &value) != 1)
		return size;

	LCD_INFO(vdd,"ACL value : %x\n", value);
	vdd->br_info.gradual_acl_val = value;

	if (!vdd->br_info.gradual_acl_val)
		vdd->br_info.acl_status = 0;
	else
		vdd->br_info.acl_status = 1;

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (!vdd->dtsi_data.tft_common_support)
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

	return size;
}

static ssize_t mipi_samsung_esd_check_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int value;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &value) != 1)
		return size;

	LCD_INFO(vdd, "value : %d\n", value);

	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(value, true, (void *)vdd, ESD_MASK_DEFAULT);

	return size;
}

static ssize_t mipi_samsung_esd_check_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct irqaction *action = NULL;
	struct irq_desc *desc = NULL;
	int rc = 0;
	int i;
	int irq;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "%s: Invalid data vdd : 0x%zx\n", __func__, (size_t)vdd);
		goto end;
	}

	if (!vdd->esd_recovery.num_of_gpio) {
		LCD_INFO(vdd, "None of gpio registered for esd irq\n");
		goto end;
	}

	LCD_INFO(vdd, "num gpio (%d)\n", vdd->esd_recovery.num_of_gpio);

	for (i = 0; i < vdd->esd_recovery.num_of_gpio; i++) {
		irq = ss_gpio_to_irq(vdd->esd_recovery.esd_gpio[i]);
		desc = irq_to_desc(irq);
		action = desc->action;

		if (action && action->thread_fn) {
			LCD_INFO(vdd, "[%d] gpio (%d) irq (%d) handler (%s)\n",
				i, vdd->esd_recovery.esd_gpio[i], irq, action->name);

			mutex_lock(&vdd->esd_recovery.esd_lock);
			generic_handle_irq(irq);
			mutex_unlock(&vdd->esd_recovery.esd_lock);

		} else {
			LCD_INFO(vdd, "No handler for irq(%d)\n", irq);
		}
	}

end:
	return rc;
}

static ssize_t ss_rf_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	snprintf(buf, 50, "RF INFO: RAT(%d), BAND(%d), ARFCN(%d)\n",
			vdd->dyn_mipi_clk.rf_info.rat,
			vdd->dyn_mipi_clk.rf_info.band,
			vdd->dyn_mipi_clk.rf_info.arfcn);

	return strlen(buf);
}

static ssize_t ss_rf_info_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int temp[6] = {0, };

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->dyn_mipi_clk.is_support) {
		LCD_INFO(vdd, "ndx: %d, dynamic mipi clk is not supported..\n", vdd->ndx);
		return size;
	}

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
#if IS_ENABLED(CONFIG_SDP)
	if (vdd->dyn_mipi_clk.is_adaptive_mipi_v2) {
		struct adaptive_mipi_v2_info *info = &vdd->dyn_mipi_clk.adaptive_mipi_v2_info;
		struct cp_info cp_msg;
		struct band_info binfo;
		struct dev_ril_bridge_msg ril_msg  = {
			.dev_id = IPC_SYSTEM_CP_ADAPTIVE_MIPI_INFO,
			.data = &cp_msg,
		};

		if (sscanf(buf, "%hhu %d %d %hhu %d %d\n", &binfo.rat, &binfo.band,
					&binfo.channel, &binfo.connection_status,
					&binfo.bandwidth, &binfo.sinr) != 6)
			return size;

		cp_msg.cell_count = 1;
		cp_msg.infos[0] = binfo;

		LCD_INFO(vdd, "rat=%d band=%d ch=%d connection_status=%d bandwidth=%d sinr=%d\n",
				binfo.rat, binfo.band, binfo.channel, binfo.connection_status,
				binfo.bandwidth, binfo.sinr);

		info->ril_nb.notifier_call(&info->ril_nb, 0, (void *)&ril_msg);

		return size;
	}
#endif
#endif

	if (sscanf(buf, "%d %d %d\n", &temp[0], &temp[1], &temp[2]) != 3)
		return size;

	mutex_lock(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	vdd->dyn_mipi_clk.rf_info.rat = temp[0];
	vdd->dyn_mipi_clk.rf_info.band = temp[1];
	vdd->dyn_mipi_clk.rf_info.arfcn = temp[2];

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
	if (ss_change_dyn_mipi_clk_timing(vdd) < 0)
		LCD_INFO(vdd, "Failed to update MIPI clock timing\n");
	else
		LCD_INFO(vdd,"RAT(%d), BAND(%d), ARFCN(%d)\n",
			vdd->dyn_mipi_clk.rf_info.rat,
			vdd->dyn_mipi_clk.rf_info.band,
			vdd->dyn_mipi_clk.rf_info.arfcn);
#else
	LCD_INFO(vdd, "mipi clk change not support\n");
#endif
	mutex_unlock(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	return size;
}

static ssize_t ss_dynamic_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int i, len = 0;
#if IS_ENABLED(CONFIG_ARCH_VOLCANO)
	struct clk_timing_table *timing_table;
#endif

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->dyn_mipi_clk.is_support) {
		LCD_INFO(vdd, "ndx: %d, dynamic mipi clk is not supported..\n", vdd->ndx);
		return -EPERM;
	}

	len += scnprintf(buf + len, 100, "idx clk_rate\n");

#if IS_ENABLED(CONFIG_SDP)
	if (vdd->dyn_mipi_clk.is_adaptive_mipi_v2) {
		struct adaptive_mipi_v2_info *info = &vdd->dyn_mipi_clk.adaptive_mipi_v2_info;

		for (i = 0; i < info->mipi_clocks_size; i++)
			len += scnprintf(buf + len, 100, "[%d] %d kbps\n", i, info->mipi_clocks_kbps[i]);
	} else {
#if IS_ENABLED(CONFIG_ARCH_VOLCANO)
		timing_table = &vdd->dyn_mipi_clk.clk_timing_table;
#else
		struct clk_timing_table *timing_table = &vdd->dyn_mipi_clk.clk_timing_table;
#endif

		for (i = 0; i < timing_table->tab_size; i++)
			len += scnprintf(buf + len, 100, "[%d] %d\n", i, timing_table->clk_rate[i]);
	}
#else
#if IS_ENABLED(CONFIG_ARCH_VOLCANO)
	timing_table = &vdd->dyn_mipi_clk.clk_timing_table;
#else
	struct clk_timing_table *timing_table = &vdd->dyn_mipi_clk.clk_timing_table;
#endif

	for (i = 0; i < timing_table->tab_size; i++)
		len += scnprintf(buf + len, 100, "[%d] %d\n", i, timing_table->clk_rate[i]);
#endif

	len += scnprintf(buf + len, 100, "Write [idx] to dynamic_freq node to set clk_rate.\n");
	len += scnprintf(buf + len, 100, "To revert it (use rf info), Write -1 to dynamic_freq node.\n");

	return strlen(buf);
}

/*
 * ss_dynamic_freq_store()
 * -1 : revert fixed idx (use rf_info notifier)
 * others : fix table idx for mipi_clk/ffc to tesst purpose
 */
static ssize_t ss_dynamic_freq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int val = -1;
#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
#if IS_ENABLED(CONFIG_SDP)
	struct sde_connector *sde_conn;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
#endif
#endif

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->dyn_mipi_clk.is_support) {
		LCD_INFO(vdd, "ndx: %d, dynamic mipi clk is not supported..\n", vdd->ndx);
		return size;
	}

	if (sscanf(buf, "%d\n", &val) != 1) {
		LCD_INFO(vdd, "invalid input (%d)\n", val);
		return size;
	}

	mutex_lock(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	vdd->dyn_mipi_clk.force_idx = val;

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
#if IS_ENABLED(CONFIG_SDP)
	if (vdd->dyn_mipi_clk.is_adaptive_mipi_v2) {
		struct adaptive_mipi_v2_info *info = &vdd->dyn_mipi_clk.adaptive_mipi_v2_info;

		if (vdd->dyn_mipi_clk.force_idx < 0) {
			LCD_INFO(vdd, "invalidate force_idx\n");
			vdd->dyn_mipi_clk.force_idx = -1;
		} else if (vdd->dyn_mipi_clk.force_idx >= info->mipi_clocks_size) {
			LCD_ERR(vdd, "invalid mipi_idx: %d >= %d\n",
					vdd->dyn_mipi_clk.force_idx, info->mipi_clocks_size);
			vdd->dyn_mipi_clk.force_idx = -1;
		} else {
			vdd->dyn_mipi_clk.requested_clk_idx = vdd->dyn_mipi_clk.force_idx;
			vdd->dyn_mipi_clk.requested_clk_rate =
				info->mipi_clocks_kbps[vdd->dyn_mipi_clk.force_idx];

			if (dsi_display->panel->panel_mode == DSI_OP_VIDEO_MODE) {
				sde_conn = to_sde_connector(dsi_display->drm_conn);
				if (!sde_conn) {
					LCD_ERR(vdd, "fail to get valid sde_conn\n");
				} else {
					LCD_INFO(vdd, "schedule_work - dyn_clk_work (%d)\n", vdd->dyn_mipi_clk.requested_clk_rate);
					sde_conn->scheduled_dyn_clk_rate = vdd->dyn_mipi_clk.requested_clk_rate;
					schedule_work(&sde_conn->dyn_clk_work.work); /* send drm_event */
				}
			}

			LCD_INFO(vdd, "force_idx: %d, force clk: %d\n",
					vdd->dyn_mipi_clk.force_idx,
					vdd->dyn_mipi_clk.requested_clk_rate);
		}
	} else {
#endif
		if (val >= vdd->dyn_mipi_clk.clk_timing_table.tab_size ||
				ss_change_dyn_mipi_clk_timing(vdd) < 0) {
			LCD_ERR(vdd, "Failed to update MIPI clock timing (val: %d)\n", val);
			vdd->dyn_mipi_clk.force_idx = -1;
		} else {
			LCD_INFO(vdd, "dyn_mipi_clk.force_idx = %d\n", vdd->dyn_mipi_clk.force_idx);
		}
#if IS_ENABLED(CONFIG_SDP)
	}
#endif
#else
	LCD_INFO(vdd, "mipi clk change not support\n");
#endif
	mutex_unlock(&vdd->dyn_mipi_clk.dyn_mipi_lock);

	return size;
}

static ssize_t vrr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int refresh_rate, sot_hs_mode = 0;

	sot_hs_mode = vdd->vrr.cur_sot_hs_mode;
	refresh_rate = vdd->vrr.cur_refresh_rate;
	if(refresh_rate == 96 || refresh_rate == 120)
		sot_hs_mode = 1;

	if (vdd->vrr.cur_phs_mode)
		sot_hs_mode = 2;

	vdd->vrr.check_vsync = CHECK_VSYNC_COUNT;

	LCD_INFO(vdd,"%d %d\n", vdd->vrr.cur_refresh_rate, sot_hs_mode);

	return snprintf(buf, 10, "%d %d\n",
			vdd->vrr.cur_refresh_rate, sot_hs_mode);
}

static ssize_t vrr_lfd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct lfd_div_info div_info;
	ssize_t len;
	struct lfd_mngr *mngr;
	int scope;
	int i;
	struct vrr_info *vrr;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	vrr = &vdd->vrr;

	if (!vrr->lfd.support_lfd) {
		LCD_DEBUG(vdd, "no support lfd\n");
		return sprintf(buf, "no support LFD\n");
	}

	len = 0;

	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		len += sprintf(buf + len, "- client: %s\n", lfd_client_name[i]);
		for (scope = 0; scope < LFD_SCOPE_MAX; scope++) {
			len += sprintf(buf + len, "scope=%s: ", lfd_scope_name[scope]);

			if (mngr->fix[scope] != LFD_FUNC_FIX_OFF)
				len += sprintf(buf + len, "fix=%d ", mngr->fix[scope]);

			if (mngr->scalability[scope] != LFD_FUNC_SCALABILITY0 &&
					mngr->scalability[scope] != LFD_FUNC_SCALABILITY5)
				len += sprintf(buf + len, "scalability=%d ",
						mngr->scalability[scope]);

			if (mngr->min[scope] != LFD_FUNC_MIN_CLEAR)
				len += sprintf(buf + len, "min=%d ", mngr->min[scope]);

			if (mngr->max[scope] != LFD_FUNC_MAX_CLEAR)
				len += sprintf(buf + len, "max=%d ", mngr->max[scope]);

			len += sprintf(buf + len, "\n");
		}
		len += sprintf(buf + len, "\n");
	}

	len += sprintf(buf + len, "[result]\n");

	for (scope = 0; scope < LFD_SCOPE_MAX; scope++) {
		ss_get_lfd_div(vdd, scope, &div_info);
		len += sprintf(buf + len, "scope=%s: LFD freq: %d.%1dhz ~ %d.%1dhz, div: %d ~ %d\n",
				lfd_scope_name[scope],
				GET_LFD_INT_PART(vrr->lfd.base_rr, div_info.min_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, div_info.min_div),
				GET_LFD_INT_PART(vrr->lfd.base_rr, div_info.max_div), GET_LFD_DEC_PART(vrr->lfd.base_rr, div_info.max_div),
				div_info.min_div, div_info.max_div);
	}

	len += sprintf(buf + len, "\n");

	return  len;
}

/*
 * - video detection scenario: graphics HAL will change its panel fps via vrr_lfd sysfs
 * - factory test scenario: turn on/off LFD (to be fixed)
 * - limit LFD scenario: limit LFD frequency range (min and max frequency, to be updated as dlab request)
 */
#define LFD_CLIENT_FIX		"FIX"
#define LFD_CLIENT_SCAN		"SCAN"
#define LFD_CLIENT_FIX_TMP_CLEAR	"0"
#define LFD_CLIENT_FIX_TMP_HIGH	"1"
#define LFD_CLIENT_FIX_TMP_LOW	"2"
#define LFD_CLIENT_TSPLPM	"tsp_lpm"

char *lfd_client_name[LFD_CLIENT_MAX] = {
	[LFD_CLIENT_FAC] = "fac",
	[LFD_CLIENT_DISP] = "disp",
	[LFD_CLIENT_INPUT] = "input",
	[LFD_CLIENT_AOD] = "aod",
	[LFD_CLIENT_VID] = "vid",
	[LFD_CLIENT_HMD] = "hmd",
};

char *lfd_scope_name[LFD_SCOPE_MAX] = {
	[LFD_SCOPE_NORMAL] = "normal",
	[LFD_SCOPE_LPM] = "lpm",
	[LFD_SCOPE_HMD] = "hmd",
};

static ssize_t vrr_lfd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	char *p;
	char *arg = (char *)buf;

	enum LFD_CLIENT_ID client_id = LFD_CLIENT_MAX;
	u32 scope_ids[LFD_SCOPE_MAX];
	bool valid_scope;
	int func_val = -1;
	int i;
	struct lfd_mngr *mngr;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (!vdd->vrr.lfd.support_lfd) {
		LCD_DEBUG(vdd, "no support lfd\n");
		return size;
	}

	/* CLIENT */
	arg = strnstr(arg, "client=", strlen(arg));
	if (!arg) {
		LCD_INFO(vdd, "invalid input: no client info.\n");
		return size;
	}

	arg += strlen("client=");
	for (i = LFD_CLIENT_FAC; i < LFD_CLIENT_MAX; i++) {
		if (!strncmp(arg, lfd_client_name[i], strlen(lfd_client_name[i]))) {
			client_id = i;
			LCD_INFO(vdd,"client: %s\n", lfd_client_name[client_id]);
			break;
		}
	}
	if (client_id == LFD_CLIENT_MAX) {
		LCD_INFO(vdd, "invalid input: client(%d)\n", client_id);
		return size;
	}

	/* SCOPE */
	p = strnstr(arg, "scope=", strlen(arg));
	if (!p) {
		LCD_INFO(vdd, "invalid input: no scope info.\n");
		return size;
	}

	p += strlen("scope=");
	for (i = 0; i < LFD_SCOPE_MAX; i++)
		scope_ids[i] = LFD_SCOPE_MAX;

	valid_scope = false;
	for (i = LFD_SCOPE_NORMAL; i < LFD_SCOPE_MAX; i++) {
		if (strnstr(p, lfd_scope_name[i], strlen(p))) {
			scope_ids[i] = i;
			valid_scope = true;
			LCD_INFO(vdd,"scope: %s\n", lfd_scope_name[i]);
		}
	}
	if (!valid_scope) {
		LCD_INFO(vdd, "fail to get valid scope info.\n");
		return size;
	}

	/* FUNCTION */
	mngr = &vdd->vrr.lfd.lfd_mngr[client_id];

	p = strnstr(arg, "fix=", strlen(arg));
	if (p) {
		if ((sscanf(p + strlen("fix="), "%d", &func_val) != 1) ||
				(func_val < 0) || (func_val >= LFD_FUNC_FIX_MAX)) {
			LCD_INFO(vdd, "invalid fix input(%d)\n", func_val);
			return size;
		}

		if (func_val == LFD_FUNC_FIX_HIGHDOT
			&& vdd->vrr.cur_refresh_rate != 120) {
			LCD_INFO(vdd, "invalid rr(%d) for fix(%d)\n",
				vdd->vrr.cur_refresh_rate, func_val);
			return size;
		}

		for (i = 0; i < LFD_SCOPE_MAX; i++) {
			if (scope_ids[i] < LFD_SCOPE_MAX) {
				LCD_INFO(vdd,"fix[%d]: %d -> %d\n", scope_ids[i],
						mngr->fix[scope_ids[i]], func_val);
				mngr->fix[scope_ids[i]] = func_val;
			}
		}
	}

	p = strnstr(arg, "scalability=", strlen(arg));
	if (p) {
		if ((sscanf(p + strlen("scalability="), "%d", &func_val) != 1) ||
				(func_val < 0) || (func_val >= LFD_FUNC_SCALABILITY_MAX)) {
			LCD_INFO(vdd, "invalid scalability input(%d)\n", func_val);
			return size;
		}
		for (i = 0; i < LFD_SCOPE_MAX; i++) {
			if (scope_ids[i] < LFD_SCOPE_MAX) {
				LCD_INFO(vdd,"scalability[%s]: %d -> %d\n",
					lfd_scope_name[scope_ids[i]],
					mngr->scalability[scope_ids[i]],
					func_val);
				mngr->scalability[scope_ids[i]] = func_val;
			}
		}
	}

	p = strnstr(arg, "min=", strlen(arg));
	if (p) {
		if ((sscanf(p + strlen("min="), "%d", &func_val) != 1) ||
				(func_val < 0) || (func_val > 120)) {
			LCD_INFO(vdd, "invalid min input(%d)\n", func_val);
			return size;
		}
		for (i = 0; i < LFD_SCOPE_MAX; i++) {
			if (scope_ids[i] < LFD_SCOPE_MAX) {
				LCD_INFO(vdd,"min[%s]: %d -> %d\n",
					lfd_scope_name[scope_ids[i]],
					mngr->min[scope_ids[i]], func_val);
				mngr->min[scope_ids[i]] = func_val;
			}
		}
	}

	p = strnstr(arg, "max=", strlen(arg));
	if (p) {
		if ((sscanf(p + strlen("max="), "%d", &func_val) != 1) ||
				(func_val < 0) || (func_val > 120)) {
			LCD_INFO(vdd, "invalid max input(%d)\n", func_val);
			return size;
		}
		for (i = 0; i < LFD_SCOPE_MAX; i++) {
			if (scope_ids[i] < LFD_SCOPE_MAX) {
				LCD_INFO(vdd,"max[%s]: %d -> %d\n",
					lfd_scope_name[scope_ids[i]],
					mngr->max[scope_ids[i]], func_val);
				mngr->max[scope_ids[i]] = func_val;
			}
		}
	}

	vdd->vrr.lfd.running_lfd = true;

	if (!ss_is_ready_to_send_cmd(vdd))
		LCD_INFO(vdd, "Panel is not ready(%d), block LFD change\n", vdd->panel_state);
	else
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);

	vdd->vrr.lfd.running_lfd = false;

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
	ss_notify_queue_work(vdd, PANEL_EVENT_LFD_STATE_CHANGED);
#endif
	return size;
}

/* Motto */
static ssize_t ss_swing_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct msm_dsi_phy *phy;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	int i, ret = 0;
	int len = 0;
	u32 val;

	if (ss_is_panel_off(vdd)) {
		LCD_ERR(vdd, "panel is off\n");
		return len;
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, true);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
		goto end;
	}

	LCD_INFO(vdd, "ctrl_count : %d\n", dsi_display->ctrl_count);

	/* Loop for dual_dsi : need to write at both 0 & 1 ctrls */
	/* Normal model will only have 0 ctrl */
	display_for_each_ctrl(i, dsi_display) {
		phy = dsi_display->ctrl[i].phy;
		if (!phy) {
			LCD_INFO(vdd, "no phy!");
			continue;
		}
		/* Call dsi_phy_hw_v4_0_store_str */
		val = dsi_phy_show_str(phy);

		len += sprintf(buf + len, "[DSI_%d] 0x%02X (0x00 ~ 0xFF)\n", i, val);
		LCD_INFO(vdd, "[CTRL_%d] %02X (0x00 ~ 0xFF)\n", i, val);
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, false);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
	}

end:
	return len;
}

/* Input Value Range = 0x00 ~ 0xFF */
static ssize_t ss_swing_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	u32 val[2];
	int i, ret = 0;
	struct msm_dsi_phy *phy;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	bool dual_dsi = ss_is_dual_dsi(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}
	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (!dsi_display) {
		LCD_INFO(vdd, "cannot extract dsi_display");
		return size;
	}

	if (sscanf(buf, "%x", val) != 1) {
		LCD_INFO(vdd, "size error\n");
		return size;
	}
	if (*val > 0x00FF) {
		LCD_INFO(vdd, "invalid value\n");
		return size;
	}
	val[1] = val[0];/* backup input val */

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, true);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
		goto end;
	}
	if (dual_dsi)
		LCD_INFO(vdd,"dual_dsi!  ndx:%d, ctrl_count:%x\n",
			vdd->ndx, dsi_display->ctrl_count);

	/* Loop for dual_dsi : need to write at both 0 & 1 ctrls */
	/* Normal model will only have 0 ctrl */
	display_for_each_ctrl(i, dsi_display) {
		phy = dsi_display->ctrl[i].phy;
		if (!phy) {
			LCD_INFO(vdd, "no phy!");
			continue;
		}
		/* Call dsi_phy_hw_v4_0_store_str */
		dsi_phy_store_str(phy, val);
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, false);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
	}

	/* save updated value if okay*/
	if (val[1]==val[0])
		vdd->motto_info.motto_swing= val[0];

end:
	return size;
}

static ssize_t ss_phy_vreg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct msm_dsi_phy *phy;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	int i, ret = 0;
	int len = 0;
	u32 val = 0;

	if (ss_is_panel_off(vdd)) {
		LCD_ERR(vdd, "panel is off\n");
		return len;
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, true);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
		goto end;
	}

	LCD_INFO(vdd, "ctrl_count : %d\n", dsi_display->ctrl_count);

	/* Loop for dual_dsi : need to write at both 0 & 1 ctrls */
	/* Normal model will only have 0 ctrl */
	display_for_each_ctrl(i, dsi_display) {
		phy = dsi_display->ctrl[i].phy;
		if (!phy) {
			LCD_INFO(vdd, "no phy!");
			continue;
		}

		val = dsi_phy_show_vreg(phy);

		len += sprintf(buf + len, "[DSI_%d] 0x%02X \n", i, val);
		LCD_INFO(vdd, "[CTRL_%d] 0x%02X \n", i, val);
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, false);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
	}

end:
	return len;
}

/* to test dsi phy vreg ctrol register change */
static ssize_t ss_phy_vreg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	u32 val, read;
	int i, ret = 0;
	struct msm_dsi_phy *phy;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	bool dual_dsi = ss_is_dual_dsi(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}
	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (!dsi_display) {
		LCD_INFO(vdd, "cannot extract dsi_display");
		return size;
	}

	if (sscanf(buf, "%x", &val) != 1) {
		LCD_INFO(vdd, "size error\n");
		return size;
	}
	if (val > 0x00FF) {
		LCD_INFO(vdd, "invalid value\n");
		return size;
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, true);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
		goto end;
	}
	if (dual_dsi)
		LCD_INFO(vdd,"dual_dsi!  ndx:%d, ctrl_count:%x\n",
			vdd->ndx, dsi_display->ctrl_count);

	/* Loop for dual_dsi : need to write at both 0 & 1 ctrls */
	/* Normal model will only have 0 ctrl */
	display_for_each_ctrl(i, dsi_display) {
		phy = dsi_display->ctrl[i].phy;
		if (!phy) {
			LCD_INFO(vdd, "no phy!");
			continue;
		}

		dsi_phy_store_vreg(phy, &val);
		usleep_range(500, 1000);

		read = dsi_phy_show_vreg(phy); // For?
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, false);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
	}

	vdd->motto_info.vreg_ctrl_0 = val;
end:
	return size;
}

static ssize_t ss_emphasis_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int len = 0;

	/* Show: MAX_level MIN_level Current_level */
	len = sprintf(buf, "01 00 %x\n", vdd->motto_info.motto_emphasis);
	LCD_INFO(vdd,"01 00 %x\n", vdd->motto_info.motto_emphasis);
	return len;
}

static ssize_t ss_emphasis_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	u32 val[2];
	int i, ret = 0;
	struct msm_dsi_phy *phy;
	struct dsi_display *dsi_display = GET_DSI_DISPLAY(vdd);
	bool dual_dsi = ss_is_dual_dsi(vdd);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}
	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (!dsi_display) {
		LCD_INFO(vdd, "cannot extract dsi_display");
		return size;
	}

	if (sscanf(buf, "%x", val) != 1) {
		LCD_INFO(vdd, "size error\n");
		return size;
	}
	if (*val > 1) {
		LCD_INFO(vdd, "invalid value\n");
		return size;
	}
	val[1] = val[0];//backup needed?

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, true);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
		goto end;
	}
	if (dual_dsi)
		LCD_INFO(vdd,"dual_dsi! ndx:%d, ctrl_count:%x\n",
				vdd->ndx, dsi_display->ctrl_count);

	/* Loop for dual_dsi : need to write at both 0 & 1 ctrls */
	display_for_each_ctrl(i, dsi_display) {
		phy = dsi_display->ctrl[i].phy;
		if (!phy) {
			LCD_INFO(vdd, "no phy!");
			continue;
		}
		/* Call dsi_phy_hw_v4_0_store_emphasis */
		dsi_phy_store_emphasis(phy, val);
	}

	ret = ss_wrapper_dsi_display_clk_ctrl(vdd, false);
	if (ret) {
		LCD_INFO(vdd, "[%s] failed to disable DSI core clocks, rc=%d\n",
				dsi_display->name, ret);
	}
	vdd->motto_info.motto_emphasis= val[0];

end:
	return size;
}


extern struct msm_file_private *msm_ioctl_power_ctrl_ctx;
extern struct mutex msm_ioctl_power_ctrl_ctx_lock;
static ssize_t ss_ioctl_power_ctrl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	mutex_lock(&msm_ioctl_power_ctrl_ctx_lock);

	if (IS_ERR_OR_NULL(msm_ioctl_power_ctrl_ctx)) {
		snprintf(buf, 30, "enable_refcnt : %d\n", 0);

		LCD_INFO(vdd,"not initialized value %s", buf);
	} else {
		mutex_lock(&msm_ioctl_power_ctrl_ctx->power_lock);
		snprintf(buf, 30, "enable_refcnt : %d\n",msm_ioctl_power_ctrl_ctx->enable_refcnt);
		mutex_unlock(&msm_ioctl_power_ctrl_ctx->power_lock);

		LCD_INFO(vdd,"%s", buf);
	}

	mutex_unlock(&msm_ioctl_power_ctrl_ctx_lock);

	return strlen(buf);
}

static ssize_t ss_te_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	unsigned int disp_te_gpio;
	bool check_success = 0;
	int te_max = 40000; /*sampling 400ms */
	int te_count = 0;
	int rc = 0;
	disp_te_gpio = ss_get_te_gpio(vdd);

	/* 1. Video mode panel
	 * - will return false from ss_get_te_gpio(vdd),
	 *  & will return 0 from this function
	 * 2. Cmd mode panel
	 * - will return 1 when TE detected.
	 *  otherwise return 0 when NO TE detected for 400ms
	 */
	if (ss_gpio_is_valid(disp_te_gpio)) {
		for (te_count = 0 ; te_count < te_max ; te_count++) {
			rc = gpio_get_value(disp_te_gpio);
			if (rc == 1) {
				check_success = 1;
				break;
			}
			/* usleep suspends the calling thread whereas udelay is a
			 * busy wait. Here the value of te_gpio is checked in a loop of
			 * max count = 250. If this loop has to iterate multiple
			 * times before the te_gpio is 1, the calling thread will end
			 * up in suspend/wakeup sequence multiple times if usleep is
			 * used, which is an overhead. So use udelay instead of usleep.
			 */
			udelay(10);
		}
	}
	rc = scnprintf((char *)buf, 6, "%d\n", check_success);

	return rc;
}

static ssize_t ss_udc_data_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (sscanf(buf, "%d", &input) != 1)
		goto end;

	LCD_INFO(vdd,"(%d)\n", input);

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (vdd->panel_func.read_udc_data)
		vdd->panel_func.read_udc_data(vdd);
end:
	return size;
}

static ssize_t ss_udc_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int len = 0;
	int i = 0;
	u8 udc_buf[255];

	if (!vdd->udc.size) {
		LCD_ERR(vdd, "No udc data\n");
		goto end;
	}

	if (!vdd->udc.read_done) {
		LCD_ERR(vdd, "Not read yet\n");
		goto end;
	}

	for (i = 0; i < vdd->udc.size; i++) {
		buf[i] = vdd->udc.data[i];
		len += sprintf(udc_buf + len, "%02X", vdd->udc.data[i]);
	}

	LCD_INFO(vdd, "%s\n", udc_buf);

end:
	return vdd->udc.size;

}

static ssize_t ss_udc_factory_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{

	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int len = 0;
	int i = 0;
	u8 udc_buf[255];

	if (!vdd->udc.size) {
		LCD_ERR(vdd, "No udc data\n");
		goto end;
	}

	if (!vdd->udc.read_done) {
		LCD_ERR(vdd, "Not read yet\n");
		goto end;
	}

	for (i = 0; i < vdd->udc.size; i += 2) {
		if (i == (vdd->udc.size - 2))
			len += sprintf(buf + len, "%d.%d", vdd->udc.data[i], vdd->udc.data[i+1]);
		else
			len += sprintf(buf + len, "%d.%d,", vdd->udc.data[i], vdd->udc.data[i+1]);
	}

	LCD_INFO(vdd, "%s\n", udc_buf);

end:
	return len;
}
static ssize_t ss_set_elvss_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		goto end;
	}

	if (sscanf(buf, "%d", &input) != 1)
		goto end;

	LCD_INFO(vdd,"(%d)\n", input);

	if (input) {
		vdd->set_factory_elvss = 1;
	} else {
		vdd->set_factory_elvss = 0;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
end:
	return size;
}

static char test_list[][60] = {
	"0. NONE",
	"1. BRIGHTNESS",
	"2. mDNIe",
	"3. SELF_DISP",
	"4. MAFPC",
	"5. DEBUG PRINT (0:off, 1:all, 2:cmds, 3:vsync, 4:frame)",
};

static ssize_t ss_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;
	int i;
	int list_size = sizeof(test_list) / sizeof(test_list[0]);

	len += scnprintf(buf + len, 100, "[idx] [test]\n");
	for (i = 1; i < list_size; i++)
		len += scnprintf(buf + len, 100, "%s\n", test_list[i]);
	len += scnprintf(buf + len, 100, "Write [idx] [enable] to control\n");
	len += scnprintf(buf + len, 100, "ex) To disable(0) SELF_DISP(3) write below\n");
	len += scnprintf(buf + len, 100, "ex) echo 3 0 > test\n");

	return len;
}

static ssize_t ss_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	unsigned int input[2];
	int list_size = sizeof(test_list) / sizeof(test_list[0]);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (sscanf(buf, "%d %d", &input[0], &input[1]) != 2)
		return size;

	if ((input[0] < 0) || (input[0] >= list_size)) {
		LCD_ERR(vdd, "invalid input\n");
		return size;
	}

	switch(input[0]) {
	case 1: /* BRIGHTNESS*/
		if (input[1])
			vdd->br_info.no_brightness = false;
		else
			vdd->br_info.no_brightness = true;
		break;
	case 2: /* mDNIe */
		if (input[1])
			vdd->mdnie.support_mdnie = true;
		else
			vdd->mdnie.support_mdnie = false;
		break;
	case 3: /* SELF_DISP */
		if (input[1]) {
			vdd->self_disp.is_support = true;
			if (!vdd->self_disp.is_initialized) {
				if (vdd->self_disp.init)
					vdd->self_disp.init(vdd);
				if (vdd->self_disp.data_init)
					vdd->self_disp.data_init(vdd);
			}
		} else {
			vdd->self_disp.is_support = false;
		}
		break;
	case 4: /* MAFPC(ABC) */
		switch (input[1]) {
		case 0:
			vdd->mafpc.en = false;
			if (vdd->mafpc.enable)
				vdd->mafpc.enable(vdd, false);
			break;
		case 1:
			vdd->mafpc.en = true;
			break;
		case 2:
			vdd->mafpc.en = true;
			if (vdd->mafpc.img_write)
				vdd->mafpc.img_write(vdd, true);
			if (vdd->mafpc.enable)
				vdd->mafpc.enable(vdd, true);
			break;
		default:
			LCD_ERR(vdd, "Invalid Input\n");
			break;
		}
		break;
	case 5: /* DEBUG PRINT */
		switch (input[1]) {
		case 0:
			vdd->debug_data->print_all = false;
			vdd->debug_data->print_cmds = false;
			vdd->debug_data->print_vsync = false;
			vdd->debug_data->print_frame = false;
			break;
		case 1:
			vdd->debug_data->print_all = true;
			break;
		case 2:
			vdd->debug_data->print_cmds = true;
			break;
		case 3:
			vdd->debug_data->print_vsync = true;
			break;
		case 4:
			vdd->debug_data->print_frame = true;
			break;
		default:
			LCD_ERR(vdd, "Invalid Input\n");
			break;
		}
		break;
	default:
		break;
	}

	LCD_INFO(vdd, "TEST [%s] : [%d]\n", test_list[input[0]], input[1]);
	return size;
}

static ssize_t ss_glut_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;

	len += scnprintf(buf + len, 100, "[val] [test]\n");
	len += scnprintf(buf + len, 100, "0 : default\n");
	len += scnprintf(buf + len, 100, "1 : use 0x00 val for all GLUT\n");
	len += scnprintf(buf + len, 100, "2 : skip GLUT val\n");

	return len;
}

static ssize_t ss_glut_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return size;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	switch(input) {
	case 0:
		vdd->br_info.glut_00_val = false;
		vdd->br_info.glut_skip = false;
		break;
	case 1:
		vdd->br_info.glut_00_val = true;
		break;
	case 2:
		vdd->br_info.glut_skip = true;
		break;
	default:
		break;
	}

	return size;
}

/* for debugging lego op tables (candela table, glut table, so on..) */
static ssize_t ss_table_legop_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct ss_brightness_info *info = &vdd->br_info;
	struct cmd_legoop_map *table;
	struct candela_map_table *candela_table;
	struct candela_map_data *map;
	int i, j, tbl;
	int len = 0;

	/* print tables */
	for (tbl = NORMAL; tbl < CD_MAP_TABLE_MAX; tbl++) {
		candela_table = &info->candela_map_table[tbl][vdd->panel_revision];
		map = candela_table->cd_map;
		LCD_INFO(vdd, "[candela table (%s): row=%d] min_lv:%d, max_lv:%d\n",
				tbl == NORMAL ? "NORMAL" : tbl == HBM ? "HBM" :
				tbl == AOD ? "AOD " : tbl == HMT ? "HMT " : "INVALID",
				candela_table->tab_size,
				candela_table->min_lv, candela_table->max_lv);
		LCD_INFO(vdd, "bl_level  wrdisbv  cd\n");
		LCD_INFO(vdd, "---------------------------\n");
		for (i = 0 ; i < candela_table->tab_size; i++)
			LCD_INFO(vdd, "%4d %4d %4d\n", map[i].bl_level, map[i].wrdisbv, map[i].cd);
	}

	table = &info->glut_offset_48hs;
	LCD_INFO(vdd, "[glut_offset_48hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->glut_offset_60hs;
	LCD_INFO(vdd, "[glut_offset_60hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->glut_offset_96hs;
	LCD_INFO(vdd, "[glut_offset_96hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->glut_offset_night_dim[0];
	LCD_INFO(vdd, "[glut_offset_night_dim: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->analog_offset_48hs[vdd->panel_revision];
	LCD_INFO(vdd, "[analog_offset_48hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%d ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->analog_offset_60hs[vdd->panel_revision];
	LCD_INFO(vdd, "[analog_offset_60hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%d ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->analog_offset_96hs[vdd->panel_revision];
	LCD_INFO(vdd, "[analog_offset_96hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%d ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->analog_offset_120hs[vdd->panel_revision];
	LCD_INFO(vdd, "[analog_offset_120hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%d ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->manual_aor_48hs[vdd->panel_revision];
	LCD_INFO(vdd, "[manual_aor_48hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->manual_aor_60hs[vdd->panel_revision];
	LCD_INFO(vdd, "[manual_aor_60hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->manual_aor_96hs[vdd->panel_revision];
	LCD_INFO(vdd, "[manual_aor_96hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}

	table = &info->manual_aor_120hs[vdd->panel_revision];
	LCD_INFO(vdd, "[manual_aor_120hs: col=%d, row=%d]\n", table->col_size, table->row_size);
	for (i = 0 ; i < table->row_size; i++) {
		LCD_INFO(vdd, "[%3d] ", i);
		for (j = 0 ; j < table->col_size; j++)
			pr_cont("%02X ", table->cmds[i][j]);
		pr_cont("\n");
	}
	return len;
}

static ssize_t ss_pdf_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int input = 0;
	struct samsung_display_driver_data *vdd =
			(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (sscanf(buf, "%d", &input) != 1)
		return size;

	LCD_INFO(vdd, "PDF Sysfs Call Start\n");

	mutex_lock(&vdd->vdd_lock);

	/* Read PDF File Again */
	ss_wrapper_read_from_file(vdd, true);

	/* Parsing All Data Again */
	ss_panel_parse_dt(vdd);

	/* self display */
	if (vdd->self_disp.is_support) {
		if (vdd->self_disp.init)
			vdd->self_disp.init(vdd);

		if (vdd->self_disp.data_init)
			vdd->self_disp.data_init(vdd);
		else
			LCD_ERR(vdd, "no self display data init function\n");
	}

	/* mafpc */
	if (vdd->mafpc.is_support) {
		if (vdd->mafpc.init)
			vdd->mafpc.init(vdd);

		if (vdd->mafpc.data_init)
			vdd->mafpc.data_init(vdd);
		else
			LCD_ERR(vdd, "no mafpc data init function\n");
	}

	mutex_unlock(&vdd->vdd_lock);

	LCD_INFO(vdd, "PDF Sysfs Call Done\n");

	return size;
}

/* For Opcode Validation */
#if IS_ENABLED(CONFIG_OPCODE_PARSER)
#define MAX_PATH_LEN 100
static ssize_t parse_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd;
	char *data;
	int ret;
	size_t length;
	char firmware_path[MAX_PATH_LEN] = "Display_OPcode.txt";
	const struct firmware *OPfile = NULL;

	if (buf == NULL || strchr(buf, '.') || strchr(buf, '/'))
		return size;

	vdd = (struct samsung_display_driver_data *)dev_get_drvdata(dev);
	LCD_INFO(vdd, "Entered Store Function");

	/* Allow 2 methods of input ie:
	 * From file (/vendor/firmware/Display_OPcode.txt).
	 * From adb buffer ( echo "opcodes" > ).
	 */
	if (size <= 3 && strnstr(buf, "ok", 2)) {
		/* Read from file */
		LCD_INFO(vdd, "Loading file name : [%s]\n", firmware_path);
		ret = request_firmware(&OPfile, firmware_path, &vdd->lcd_dev->dev);
		if (ret < 0) {
			LCD_INFO(vdd, "Failed to Load file\n");
			return -EPERM;
		}

		LCD_INFO(vdd, "OPcode loading success. Size : %ld(bytes)", OPfile->size);
		length = OPfile->size;
		data = kvzalloc((length + 10)*sizeof(char), GFP_KERNEL);
		if (length < 1) {
			LCD_INFO(vdd, "Nothing to read\n");
			release_firmware(OPfile);
			goto exit;
		}
		if (data == NULL) {
			LCD_INFO(vdd, "Could not assign free space for data\n");
			release_firmware(OPfile);
			return -ENOMEM;
		}
		memcpy(data, OPfile->data, length);

		release_firmware(OPfile);
	} else {
		/* Read from buffer */
		data = kvzalloc(size * sizeof(char), GFP_KERNEL);
		if (!data) {
			LCD_INFO(vdd, "Could not assign free space for data\n");
			return -ENOMEM;
		}
		strcpy(data, buf);
		length = size;
	}
	LCD_INFO(vdd, "Calling Parse\n");
	ret = ss_wrapper_parse_cmd_sets_sysfs(vdd, data, length, "Dummy_opcode");
	if (!ret) {
		LCD_INFO(vdd, "Could not parse Commands\n");
		ret = -EPERM;
	}
exit:
	if (data != NULL)
		kvfree(data);
	return size;
}
#else
static ssize_t parse_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}
#endif

static ssize_t ss_vlin1_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	int enable;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd");
		goto end;
	}

	if (!ss_is_ready_to_send_cmd(vdd)) {
		LCD_INFO(vdd, "Panel is not ready. Panel State(%d)\n", vdd->panel_state);
		return size;
	}

	if (sscanf(buf, "%d", &enable) != 1)
		goto end;

	LCD_INFO(vdd,"(%d)\n", enable);

	if (enable)
		ss_send_cmd(vdd, TX_VLIN1_TEST_ENTER);
	else
		ss_send_cmd(vdd, TX_VLIN1_TEST_EXIT);

end:
	return size;
}

static ssize_t ss_fac_pretest_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);
	struct dsi_panel *panel = GET_DSI_PANEL(vdd);
	int input;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_INFO(vdd, "no vdd\n");
		return -ENODEV;
	}

	if (sscanf(buf, "%d", &input) != 1)
		goto end;

	if (input)
		vdd->is_factory_pretest = true;
	else
		vdd->is_factory_pretest = false;

	dsi_panel_set_pinctrl_state(panel, true);

	LCD_INFO(vdd, "factory pretest %d\n", vdd->is_factory_pretest);

end:
	return size;
}

static ssize_t ss_display_on_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct samsung_display_driver_data *vdd =
		(struct samsung_display_driver_data *)dev_get_drvdata(dev);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	snprintf(buf, 20, "%lld", vdd->display_on_time);
	LCD_INFO(vdd,"latest frame done time(%lld)\n", vdd->display_on_time);
	return strlen(buf);
}

/* FACTORY TEST */
static DEVICE_ATTR(lcd_type, S_IRUGO, ss_disp_lcdtype_show, NULL);
static DEVICE_ATTR(cell_id, S_IRUGO, ss_disp_cell_id_show, NULL);
static DEVICE_ATTR(octa_id, S_IRUGO, ss_disp_octa_id_show, NULL);
static DEVICE_ATTR(w_coordinate, S_IRUGO, ss_w_coordinate_show, NULL);
static DEVICE_ATTR(window_type, S_IRUGO, ss_disp_windowtype_show, NULL);
static DEVICE_ATTR(manufacture_date, S_IRUGO, ss_disp_manufacture_date_show, NULL);
static DEVICE_ATTR(manufacture_code, S_IRUGO, ss_disp_manufacture_code_show, NULL);
static DEVICE_ATTR(power_reduce, S_IRUGO | S_IWUSR | S_IWGRP, ss_disp_acl_show, ss_disp_acl_store);
static DEVICE_ATTR(siop_enable, S_IRUGO | S_IWUSR | S_IWGRP, ss_disp_siop_show, ss_disp_siop_store);
static DEVICE_ATTR(mafpc_check, S_IRUGO | S_IWUSR | S_IWGRP, ss_mafpc_check_show, NULL);
static DEVICE_ATTR(dynamic_hlpm, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_dynamic_hlpm_store);
static DEVICE_ATTR(self_move, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_self_move_store);
static DEVICE_ATTR(self_mask_check, S_IRUGO | S_IWUSR | S_IWGRP, ss_self_mask_check_show, NULL);
static DEVICE_ATTR(alpm, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, ss_panel_lpm_mode_show, ss_panel_lpm_mode_store);
static DEVICE_ATTR(hlpm_mode, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, ss_panel_hlpm_mode_show, ss_panel_hlpm_mode_store);
static DEVICE_ATTR(mcd_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_mcd_store);
static DEVICE_ATTR(brightdot, S_IRUGO | S_IWUSR | S_IWGRP, ss_brightdot_show, ss_brightdot_store);
static DEVICE_ATTR(mst, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_mst_store);
static DEVICE_ATTR(xtalk_mode, S_IRUGO | S_IWUSR | S_IWGRP, NULL, xtalk_store);
static DEVICE_ATTR(irc_mode, S_IRUGO | S_IWUSR | S_IWGRP, ss_irc_mode_show, ss_irc_mode_store);
static DEVICE_ATTR(ecc, S_IRUGO | S_IWUSR | S_IWGRP, ecc_show, NULL);
static DEVICE_ATTR(ssr, S_IRUGO | S_IWUSR | S_IWGRP, ssr_show, NULL);
static DEVICE_ATTR(gct, S_IRUGO | S_IWUSR | S_IWGRP, gct_show, gct_store);
static DEVICE_ATTR(grayspot, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_grayspot_store);
static DEVICE_ATTR(vglhighdot, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_vglhighdot_store);
static DEVICE_ATTR(gamma_flash, S_IRUGO | S_IWUSR | S_IWGRP, ss_disp_flash_gamma_show, NULL);
static DEVICE_ATTR(ccd_state, S_IRUGO | S_IWUSR | S_IWGRP, ss_ccd_state_show, NULL);
static DEVICE_ATTR(dsc_crc, S_IRUGO | S_IWUSR | S_IWGRP, ss_dsc_crc_show, NULL);
static DEVICE_ATTR(panel_aging, S_IRUGO | S_IWUSR | S_IWGRP, ss_panel_aging_show, ss_panel_aging_store);
static DEVICE_ATTR(adaptive_control, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_adaptive_control_store);
static DEVICE_ATTR(hw_cursor, S_IRUGO | S_IWUSR | S_IWGRP, NULL, mipi_samsung_hw_cursor_store);

/* SVC */
static DEVICE_ATTR(SVC_OCTA, S_IRUGO, ss_disp_SVC_OCTA_show, NULL);
static DEVICE_ATTR(SVC_OCTA2, S_IRUGO, ss_disp_SVC_OCTA2_show, NULL);
static DEVICE_ATTR(SVC_OCTA_CHIPID, S_IRUGO, ss_disp_SVC_OCTA_CHIPID_show, NULL);
static DEVICE_ATTR(SVC_OCTA2_CHIPID, S_IRUGO, ss_disp_SVC_OCTA2_CHIPID_show, NULL);
static DEVICE_ATTR(SVC_OCTA_DDI_CHIPID, S_IRUGO, ss_disp_SVC_OCTA_DDI_CHIPID_show, NULL);
static DEVICE_ATTR(SVC_OCTA2_DDI_CHIPID, S_IRUGO, ss_disp_SVC_OCTA2_DDI_CHIPID_show, NULL);

/* DPUI/DPCI */
static DEVICE_ATTR(dpui, S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP, ss_dpui_show, ss_dpui_store);
static DEVICE_ATTR(dpui_dbg, S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP, ss_dpui_dbg_show, ss_dpui_dbg_store);
static DEVICE_ATTR(dpci, S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP, ss_dpci_show, ss_dpci_store);
static DEVICE_ATTR(dpci_dbg, S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP, ss_dpci_dbg_show, ss_dpci_dbg_store);

/* FINGERPRINT */
static DEVICE_ATTR(mask_brightness, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_finger_hbm_store);
static DEVICE_ATTR(actual_mask_brightness, S_IRUGO | S_IWUSR | S_IWGRP, ss_finger_hbm_updated_show, NULL);
static DEVICE_ATTR(fp_green_circle, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_fp_green_circle_store);
static DEVICE_ATTR(local_hbm, S_IRUGO | S_IWUSR | S_IWGRP, ss_local_hbm_show, ss_local_hbm_store);

/* PANEL FUNCTION */
static DEVICE_ATTR(read_mtp, S_IRUGO | S_IWUSR | S_IWGRP, ss_read_mtp_show, ss_read_mtp_store);
static DEVICE_ATTR(write_mtp, S_IRUGO | S_IWUSR | S_IWGRP, ss_read_mtp_show, ss_write_mtp_store);
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR | S_IWGRP, ss_temperature_show, ss_temperature_store);
static DEVICE_ATTR(lux, S_IRUGO | S_IWUSR | S_IWGRP, ss_lux_show, ss_lux_store);
static DEVICE_ATTR(smooth_dim, S_IRUGO | S_IWUSR | S_IWGRP, ss_smooth_dim_show, ss_smooth_dim_store);
static DEVICE_ATTR(brt_avg, S_IRUGO | S_IWUSR | S_IWGRP, ss_brt_avg_show, NULL);
static DEVICE_ATTR(self_mask_udc, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_self_mask_udc_store);
static DEVICE_ATTR(self_mask, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_self_mask_store);
static DEVICE_ATTR(esd_check, S_IRUGO | S_IWUSR | S_IWGRP, mipi_samsung_esd_check_show, mipi_samsung_esd_check_store);
static DEVICE_ATTR(rf_info, S_IRUGO | S_IWUSR | S_IWGRP, ss_rf_info_show, ss_rf_info_store);
static DEVICE_ATTR(dynamic_freq, S_IRUGO | S_IWUSR | S_IWGRP, ss_dynamic_freq_show, ss_dynamic_freq_store);
static DEVICE_ATTR(dia, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_dia_store);
static DEVICE_ATTR(gamma_comp, S_IRUGO | S_IWUSR | S_IWGRP, ss_disp_gamma_comp_show, ss_disp_gamma_comp_store);
static DEVICE_ATTR(night_dim, S_IRUGO | S_IWUSR | S_IWGRP, ss_night_dim_show, ss_night_dim_store);
static DEVICE_ATTR(conn_det, S_IRUGO | S_IWUSR | S_IWGRP, ss_ub_con_det_show, ss_ub_con_det_store);
static DEVICE_ATTR(vrr, S_IRUGO|S_IWUSR|S_IWGRP, vrr_show, NULL);
static DEVICE_ATTR(vrr_lfd, S_IRUGO|S_IWUSR|S_IWGRP, vrr_lfd_show, vrr_lfd_store);
static DEVICE_ATTR(swing, S_IRUGO|S_IWUSR|S_IWGRP, ss_swing_show, ss_swing_store);
static DEVICE_ATTR(phy_vreg, S_IRUGO|S_IWUSR|S_IWGRP, ss_phy_vreg_show, ss_phy_vreg_store);
static DEVICE_ATTR(emphasis, S_IRUGO|S_IWUSR|S_IWGRP, ss_emphasis_show, ss_emphasis_store);
static DEVICE_ATTR(ioctl_power_ctrl, S_IRUGO|S_IWUSR|S_IWGRP, ss_ioctl_power_ctrl_show, NULL);
static DEVICE_ATTR(te_check, S_IRUGO | S_IWUSR | S_IWGRP, ss_te_check_show, NULL);
static DEVICE_ATTR(udc_data, S_IRUGO | S_IWUSR | S_IWGRP, ss_udc_data_show, ss_udc_data_store);
static DEVICE_ATTR(udc_fac, S_IRUGO | S_IWUSR | S_IWGRP, ss_udc_factory_show, NULL);
static DEVICE_ATTR(set_elvss, S_IRUGO | S_IWUSR | S_IWGRP, NULL, ss_set_elvss_store);
static DEVICE_ATTR(test, 0644, ss_test_show, ss_test_store);
static DEVICE_ATTR(glut_test, S_IRUGO | S_IWUSR | S_IWGRP, ss_glut_test_show, ss_glut_test_store);
static DEVICE_ATTR(table_legop, 0644, ss_table_legop_show, NULL);

/* HMT */
static DEVICE_ATTR(hmt_bright, S_IRUGO | S_IWUSR | S_IWGRP, mipi_samsung_hmt_bright_show, mipi_samsung_hmt_bright_store);
static DEVICE_ATTR(hmt_on, S_IRUGO | S_IWUSR | S_IWGRP,	mipi_samsung_hmt_on_show, mipi_samsung_hmt_on_store);

/* For PDF with Sysfs */
static DEVICE_ATTR(pdf, 0664, NULL, ss_pdf_store);

/* For Opcode Validation */
static DEVICE_ATTR(parse_cmd, 0664, NULL, parse_cmd_store);
static DEVICE_ATTR(vlin1_test, 0644, NULL, ss_vlin1_test_store);

static DEVICE_ATTR(fac_pretest, 0644, NULL, ss_fac_pretest_store);

/* Check for display on time */
static DEVICE_ATTR(display_on, 0644, ss_display_on_show, NULL);

static struct attribute *panel_sysfs_attributes[] = {
	&dev_attr_lcd_type.attr,
	&dev_attr_cell_id.attr,
	&dev_attr_octa_id.attr,
	&dev_attr_w_coordinate.attr,
	&dev_attr_window_type.attr,
	&dev_attr_manufacture_date.attr,
	&dev_attr_manufacture_code.attr,
	&dev_attr_power_reduce.attr,
	&dev_attr_siop_enable.attr,
	&dev_attr_read_mtp.attr,
	&dev_attr_write_mtp.attr,
	&dev_attr_brt_avg.attr,
	&dev_attr_self_mask.attr,
	&dev_attr_self_mask_udc.attr,
	&dev_attr_dynamic_hlpm.attr,
	&dev_attr_self_move.attr,
	&dev_attr_self_mask_check.attr,
	&dev_attr_mafpc_check.attr,
	&dev_attr_temperature.attr,
	&dev_attr_lux.attr,
	&dev_attr_smooth_dim.attr,
	&dev_attr_alpm.attr,
	&dev_attr_hlpm_mode.attr,
	&dev_attr_hmt_bright.attr,
	&dev_attr_hmt_on.attr,
	&dev_attr_mcd_mode.attr,
	&dev_attr_brightdot.attr,
	&dev_attr_irc_mode.attr,
	&dev_attr_adaptive_control.attr,
	&dev_attr_hw_cursor.attr,
	&dev_attr_SVC_OCTA.attr,
	&dev_attr_SVC_OCTA2.attr,
	&dev_attr_SVC_OCTA_CHIPID.attr,
	&dev_attr_SVC_OCTA2_CHIPID.attr,
	&dev_attr_SVC_OCTA_DDI_CHIPID.attr,
	&dev_attr_SVC_OCTA2_DDI_CHIPID.attr,
	&dev_attr_esd_check.attr,
	&dev_attr_rf_info.attr,
	&dev_attr_dynamic_freq.attr,
	&dev_attr_xtalk_mode.attr,
	&dev_attr_ecc.attr,
	&dev_attr_ssr.attr,
	&dev_attr_gct.attr,
	&dev_attr_mst.attr,
	&dev_attr_grayspot.attr,
	&dev_attr_vglhighdot.attr,
	&dev_attr_dpui.attr,
	&dev_attr_dpui_dbg.attr,
	&dev_attr_dpci.attr,
	&dev_attr_dpci_dbg.attr,
	&dev_attr_gamma_flash.attr,
	&dev_attr_gamma_comp.attr,
	&dev_attr_night_dim.attr,
	&dev_attr_glut_test.attr,
	&dev_attr_ccd_state.attr,
	&dev_attr_mask_brightness.attr,
	&dev_attr_actual_mask_brightness.attr,
	&dev_attr_fp_green_circle.attr,
	&dev_attr_conn_det.attr,
	&dev_attr_dia.attr,
	&dev_attr_vrr.attr,
	&dev_attr_vrr_lfd.attr,
	&dev_attr_ioctl_power_ctrl.attr,
	&dev_attr_te_check.attr,
	&dev_attr_udc_data.attr,
	&dev_attr_udc_fac.attr,
	&dev_attr_set_elvss.attr,
	&dev_attr_dsc_crc.attr,
	&dev_attr_panel_aging.attr,
	&dev_attr_test.attr,
	&dev_attr_table_legop.attr,
	&dev_attr_pdf.attr,
	&dev_attr_parse_cmd.attr,
	&dev_attr_vlin1_test.attr,
	&dev_attr_fac_pretest.attr,
	&dev_attr_display_on.attr,
	&dev_attr_local_hbm.attr,
	NULL
};
static const struct attribute_group panel_sysfs_group = {
	.attrs = panel_sysfs_attributes,
};

static struct attribute *motto_tune_attrs[] = {
	&dev_attr_swing.attr,
	&dev_attr_phy_vreg.attr,
	&dev_attr_emphasis.attr,
	NULL,
};
static const struct attribute_group motto_tune_group = {
	.attrs = motto_tune_attrs
};

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
static DEVICE_ATTR(brightness_step, S_IRUGO | S_IWUSR | S_IWGRP,
			ss_disp_brightness_step,
			NULL);
static struct attribute *bl_sysfs_attributes[] = {
	&dev_attr_brightness_step.attr,
	NULL
};

static const struct attribute_group bl_sysfs_group = {
	.attrs = bl_sysfs_attributes,
};
#endif /* END CONFIG_LCD_CLASS_DEVICE*/

int __mockable ss_create_sysfs(struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	struct lcd_device *lcd_device;
#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
	struct backlight_device *bd = NULL;
#endif
	char dirname[10];
	struct class *motto_class;

	if (vdd->lcd_dev) {
		LCD_ERR(vdd, "already have sysfs\n");
		return rc;
	}

	LCD_INFO(vdd, "++\n");

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		sprintf(dirname, "panel");
	else
		sprintf(dirname, "panel%d", vdd->ndx);

	lcd_device = lcd_device_register(dirname, NULL, vdd, NULL);
	vdd->lcd_dev = lcd_device;

	if (IS_ERR_OR_NULL(lcd_device)) {
		rc = PTR_ERR(lcd_device);
		LCD_INFO(vdd, "Failed to register lcd device..\n");
		return rc;
	}

	rc = sysfs_create_group(&lcd_device->dev.kobj, &panel_sysfs_group);
	if (rc) {
		LCD_INFO(vdd, "Failed to create panel sysfs group..\n");
		sysfs_remove_group(&lcd_device->dev.kobj, &panel_sysfs_group);
		return rc;
	}

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		sprintf(dirname, "panel");
	else
		sprintf(dirname, "panel%d", vdd->ndx);

	bd = backlight_device_register(dirname, &lcd_device->dev,
						vdd, NULL, NULL);
	if (IS_ERR(bd)) {
		rc = PTR_ERR(bd);
		LCD_INFO(vdd, "backlight : failed to register device\n");
		return rc;
	}

	rc = sysfs_create_group(&bd->dev.kobj, &bl_sysfs_group);
	if (rc) {
		LCD_INFO(vdd, "Failed to create backlight sysfs group..\n");
		sysfs_remove_group(&bd->dev.kobj, &bl_sysfs_group);
		return rc;
	}
#endif

	/* Creat mottoN folder and sysfs under mottoN */
	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
#if IS_ENABLED(CONFIG_ARCH_VOLCANO)
		motto_class = class_create(THIS_MODULE, "motto");
#else
		motto_class = class_create("motto");
#endif
	else
#if IS_ENABLED(CONFIG_ARCH_VOLCANO)
		motto_class = class_create(THIS_MODULE, "motto1");
#else
		motto_class = class_create("motto1");
#endif
	if (IS_ERR_OR_NULL(motto_class)) {
		LCD_INFO(vdd, "failed to create %s motto class\n", dirname);
	}

	if (vdd->ndx == PRIMARY_DISPLAY_NDX)
		sprintf(dirname, "motto");
	else
		sprintf(dirname, "motto%d", vdd->ndx);
	vdd->motto_device = device_create(motto_class, &lcd_device->dev, 0, vdd, "%s", dirname);
	if (IS_ERR_OR_NULL(vdd->motto_device)) {
		LCD_INFO(vdd, "failed to create motto%d device\n", vdd->ndx);
	}

	rc = sysfs_create_group(&vdd->motto_device->kobj, &motto_tune_group);
	if (rc)
		LCD_INFO(vdd, "faield to create motto's nodes\n");
	/* Motto swing init value moved to panel file, if needed */

	LCD_INFO(vdd, "done!!\n");

	return rc;
}

int __mockable ss_create_sysfs_svc(struct samsung_display_driver_data *vdd)
{
	int rc = 0;
	struct lcd_device *lcd_device;
	char dirname[10];
	struct dsi_display *display = NULL;
	struct device *dev = NULL;
	struct kernfs_node *SVC_sd = NULL;
	struct kobject *SVC = NULL;

	LCD_INFO(vdd, "++\n");

	lcd_device = vdd->lcd_dev;
	if (IS_ERR_OR_NULL(lcd_device)) {
		rc = PTR_ERR(lcd_device);
		LCD_INFO(vdd, "Failed to get lcd device..\n");
		return rc;
	}

	display = GET_DSI_DISPLAY(vdd);
	dev = &display->pdev->dev;

	/* To find SVC kobject */
	SVC_sd = sysfs_get_dirent(dev->kobj.kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(SVC_sd)) {
		/* try to create SVC kobject */
		SVC = kobject_create_and_add("svc", &dev->kobj.kset->kobj);
		if (IS_ERR_OR_NULL(SVC))
			LCD_INFO(vdd, "Failed to create sys/devices/svc already exist");
		else
			LCD_INFO(vdd,"Success to create sys/devices/svc");
	} else {
		SVC = (struct kobject *)SVC_sd->priv;
		LCD_INFO(vdd,"Success to find SVC\n");
	}

	if (!IS_ERR_OR_NULL(SVC)) {
		if (vdd->ndx == PRIMARY_DISPLAY_NDX)
			sprintf(dirname, "OCTA");
		else
			sprintf(dirname, "OCTA%d", vdd->ndx);

		rc = sysfs_create_link(SVC, &lcd_device->dev.kobj, dirname);
		if (rc)
			LCD_INFO(vdd, "Failed to create panel sysfs svc/%s\n", dirname);
		else
			LCD_INFO(vdd,"Success to create panel sysfs svc/%s\n", dirname);
	} else
		LCD_INFO(vdd, "Failed to find svc kobject\n");

	ss_register_dpui(vdd);

	LCD_INFO(vdd, "done!!\n");

	return rc;
}
