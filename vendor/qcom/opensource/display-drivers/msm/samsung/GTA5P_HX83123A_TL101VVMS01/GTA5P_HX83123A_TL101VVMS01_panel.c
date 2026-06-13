/*
 * =================================================================
 *
 *
 *	Description:  samsung display panel file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2024, Samsung Electronics. All rights reserved.

*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/
#include "GTA5P_HX83123A_TL101VVMS01_panel.h"

/* For LCD_BL_5P0 voltage generation
 * required by KTZ8868 to produce VSP/VSN
 */
static int ss_boost_control(struct samsung_display_driver_data *vdd, bool enable)
{
	int ret;

	if (!enable)
		return 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, ": Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return false;
	}
	LCD_INFO(vdd, "+: %s\n", enable ? "enable" : "disable");

	ret = ss_boost_max77816_control(enable);
	if (ret < 0)
		LCD_ERR(vdd, "boost ic control failed\n");

	return 0;
}

/* For 5P8_P,5P8_N & BLIC configuration */
static int ss_blic_control(struct samsung_display_driver_data *vdd, bool enable)
{
	int ret;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, ": Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return false;
	}
	LCD_INFO(vdd, "+: %s\n", enable ? "enable" : "disable");

	ret = ss_blic_ktz8868_control(enable);
	if (ret < 0)
		LCD_ERR(vdd, "blic control failed\n");

	return 0;
}

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, ": Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return false;
	}

	LCD_INFO(vdd, "+: ndx=%d\n", vdd->ndx);
	ss_panel_attach_set(vdd, true);

	return 0;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		pr_err("%s: Invalid data vdd 0x%zx", __func__, (size_t)vdd);
		return -EINVAL;
	}

	return true;
}

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, false);
	else
		ss_panel_attach_set(vdd, true);

	switch (ss_panel_rev_get(vdd)) {
	case 0x06:
		vdd->panel_revision = 'A';
		break;
	default:
		vdd->panel_revision = 'A';
		LCD_ERR(vdd, "Invalid panel_rev(default rev : %c)\n", vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';
	LCD_INFO_ONCE(vdd, "panel_revision = %c %d \n", vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

static int samsung_panel_off_pre(struct samsung_display_driver_data *vdd)
{
	int rc = 0;

	return rc;
}

static int samsung_panel_off_post(struct samsung_display_driver_data *vdd)
{
	int rc = 0;

	return rc;
}

static int ss_panel_power_parse_boost(struct samsung_display_driver_data *vdd, struct pwr_node *pwr,
			struct device_node *np)
{
	LCD_INFO(vdd, "\n");

	return 0;
}

static int ss_panel_power_ctrl_boost(struct samsung_display_driver_data *vdd, struct pwr_node *pwr, bool enable)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, ": Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return false;
	}
	LCD_INFO(vdd, "+: %s\n", enable ? "enable" : "disable");

	ss_boost_control(vdd, enable);
	ss_blic_control(vdd, enable);

	return 0;
}

void GTA5P_HX83123A_TL101VVMS01_WUXGA_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "%s\n", ss_get_panel_name(vdd));
	vdd->panel_state = PANEL_PWR_OFF;

	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = samsung_panel_off_pre;
	vdd->panel_func.samsung_panel_off_post = samsung_panel_off_post;

	vdd->panel_func.samsung_panel_revision = ss_panel_revision;

	/* Brightness */
	vdd->panel_func.pre_brightness = NULL;
	vdd->panel_func.pre_lpm_brightness = NULL;

	/* Below data will be genarated by script in Kbuild file */
	vdd->h_buf = GTA5P_HX83123A_TL101VVMS01_PDF_DATA;
	vdd->h_size = sizeof(GTA5P_HX83123A_TL101VVMS01_PDF_DATA);

	/* Get f_buf from header file data to cover recovery mode
	 * Below code should be called before any PDF parsing code such as parsing_glut
	 */
	if (!vdd->file_loading && vdd->h_buf) {
		LCD_ERR(vdd, "Get f_buf from header file data(%zu)\n", vdd->h_size);
		vdd->f_buf = vdd->h_buf;
		vdd->f_size = vdd->h_size;
	}

	ss_boost_max77816_init();
	ss_blic_ktz8868_init();
	/* callback for PANEL_PWR_PANEL_SPECIFIC type */

	vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11].parse_cb = ss_panel_power_parse_boost;
	vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11].ctrl_cb = ss_panel_power_ctrl_boost;

	vdd->panel_powers[PANEL_POWERS_OFF_POST_LP11].parse_cb = ss_panel_power_parse_boost;
	vdd->panel_powers[PANEL_POWERS_OFF_POST_LP11].ctrl_cb = ss_panel_power_ctrl_boost;

	vdd->debug_data->print_cmds = true;
}
