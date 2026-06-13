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
#include "A36_S6E3FC5_AMS663FS01_panel.h"
#include "A36_S6E3FC5_AMS663FS01_mdnie.h"
#include "ss_panel_power.h"

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
	/*
	 * self mask is enabled from bootloader.
	 * so skip self mask setting during splash booting.
	 */
	if (!vdd->samsung_splash_enabled) {
		if (vdd->self_disp.self_mask_img_write)
			vdd->self_disp.self_mask_img_write(vdd);
	} else {
		LCD_INFO(vdd, "samsung splash enabled.. skip image write\n");
	}

	return 0;
}

static int samsung_display_on_post(struct samsung_display_driver_data *vdd)
{
	/* HAE ddi (B0) only issue */
	/* P220307-02604, P211207-05270 : Do not write AOD 60nit before AOD display on + 1vsync (34ms) */
	if (vdd->panel_lpm.need_br_update) {
		vdd->panel_lpm.need_br_update = false;
		ss_brightness_dcs(vdd, USE_CURRENT_BL_LEVEL, BACKLIGHT_NORMAL);
	}

	return 0;
}

enum VRR_CMD_RR {
	/* 1Hz is PSR mode in LPM (AOD) mode, 10Hz is PSR mode in 120HS mode */
	VRR_60HS = 0,
	VRR_120HS,
	VRR_MAX
};

static int ss_pre_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	vdd->br_info.last_br_is_hbm = false;
	return 0;
}

static int ss_pre_brightness(struct samsung_display_driver_data *vdd)
{
	int normal_max_lv = vdd->br_info.candela_map_table[NORMAL][vdd->panel_revision].max_lv;

	if (vdd->br_info.common_br.bl_level <= normal_max_lv) {
		/* HBM -> Normal Case */
		if (vdd->br_info.last_br_is_hbm) {
			LCD_INFO(vdd, "HBM -> Normal Case, Disable ESD\n");

			/* If there is a pending ESD enable work, cancel that first */
			cancel_delayed_work(&vdd->esd_enable_event_work);

			/* To avoid unexpected ESD detction, Disable ESD irq before cmd tx related with 51h */
			if (vdd->esd_recovery.esd_irq_enable)
				vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_WORK);

			/* Enable ESD after (ESD_WORK_DELAY)ms */
			schedule_delayed_work(&vdd->esd_enable_event_work, msecs_to_jiffies(ESD_WORK_DELAY));
		}
		vdd->br_info.last_br_is_hbm = false;
	} else {
		/* Normal -> HBM Case */
		if (!vdd->br_info.last_br_is_hbm) {
			LCD_INFO(vdd, "Normal -> HBM Case, Disable ESD\n");

			/* If there is a pending ESD enable work, cancel that first */
			cancel_delayed_work(&vdd->esd_enable_event_work);

			/* To avoid unexpected ESD detction, Disable ESD irq before cmd tx related with 51h */
			if (vdd->esd_recovery.esd_irq_enable)
				vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_WORK);

			/* Enable ESD after (ESD_WORK_DELAY)ms */
			schedule_delayed_work(&vdd->esd_enable_event_work, msecs_to_jiffies(ESD_WORK_DELAY));
		}
		vdd->br_info.last_br_is_hbm = true;
	}
	return 0;
}

#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR	0x32
#define RGB_INDEX_SIZE 4
#define COEFFICIENT_DATA_SIZE 8

#define F1(x, y) (((y << 10) - (((x << 10) * 56) / 55) - (102 << 10)) >> 10)
#define F2(x, y) (((y << 10) + (((x << 10) * 5) / 1) - (18483 << 10)) >> 10)

static int coefficient[][COEFFICIENT_DATA_SIZE] = {
	{       0,        0,      0,      0,      0,       0,      0,      0 }, /* dummy */
	{  -52615,   -61905,  21249,  15603,  40775,   80902, -19651, -19618 },
	{ -212096,  -186041,  61987,  65143, -75083,  -27237,  16637,  15737 },
	{   69454,    77493, -27852, -19429, -93856, -133061,  37638,  35353 },
	{  192949,   174780, -56853, -60597,  57592,   13018, -11491, -10757 },
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x, y) > 0) {
		if (F2(x, y) > 0)
			tune_number = 1;
		else
			tune_number = 2;
	} else {
		if (F2(x, y) > 0)
			tune_number = 4;
		else
			tune_number = 3;
	}

	return tune_number;
}

static int mdnie_coordinate_x(int x, int y, int index)
{
	int result = 0;

	result = (coefficient[index][0] * x) + (coefficient[index][1] * y) + (((coefficient[index][2] * x + 512) >> 10) * y) + (coefficient[index][3] * 10000);

	result = (result + 512) >> 10;

	if (result < 0)
		result = 0;
	if (result > 1024)
		result = 1024;

	return result;
}

static int mdnie_coordinate_y(int x, int y, int index)
{
	int result = 0;

	result = (coefficient[index][4] * x) + (coefficient[index][5] * y) + (((coefficient[index][6] * x + 512) >> 10) * y) + (coefficient[index][7] * 10000);

	result = (result + 512) >> 10;

	if (result < 0)
		result = 0;
	if (result > 1024)
		result = 1024;

	return result;
}

#define VIVIDNESS_MAX_IDX 3
#define VIVIDNESS_64h_IDX 4	// vividness table index : 3rd
#define VIVIDNESS_64h_SIZE 18

#define VIVIDNESS_65h_IDX1 114	// vividness table index : 113th
#define VIVIDNESS_65h_SIZE1 1

/* 64h */
static u8 vividness_64h_table[VIVIDNESS_MAX_IDX][VIVIDNESS_64h_SIZE] = {
	{0xC5, 0x09, 0x04, 0x2C, 0xE0, 0x04, 0x00, 0x03, 0xEC, 0x30, 0xEF, 0xEC, 0xD1, 0x0A, 0xE5, 0xF8, 0xEA, 0x0A}, /* default */
	{0xCB, 0x05, 0x02, 0x1B, 0xE6, 0x02, 0x00, 0x02, 0xF6, 0x27, 0xF6, 0xF1, 0xDB, 0x07, 0xEB, 0xF4, 0xE8, 0x05},
	{0xD0, 0x00, 0x00, 0x0A, 0xEC, 0x00, 0x00, 0x00, 0xFF, 0x1E, 0xFC, 0xF5, 0xE5, 0x3, 0xF1, 0xF0, 0xE5, 0x00},
};

/* 65h */
static u8 vividness_65h_table1[VIVIDNESS_MAX_IDX][VIVIDNESS_65h_SIZE1] = {
	{0x00}, /* default */
	{0x33},
	{0x66},
};

static void mdnie_set_vividness(struct samsung_display_driver_data *vdd, struct mdnie_lite_tun_type *tune, struct dsi_cmd_desc *tune_data)
{
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	int idx = tune->vividness_idx;

	/* Do not apply (color lens, natural mode, night_mode) */
	if (tune->color_lens_enable || tune->mdnie_mode == NATURAL_MODE || tune->night_mode_enable) {
		LCD_INFO(vdd, "Return by color_lens_enable or NATURAL_MODE or night_mode_enable\n");
		return;
	}

	if (idx >= 0 && idx < VIVIDNESS_MAX_IDX) {
		memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP1]].ss_txbuf[VIVIDNESS_64h_IDX],
			vividness_64h_table[idx], VIVIDNESS_64h_SIZE);

		memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP2]].ss_txbuf[VIVIDNESS_65h_IDX1],
			vividness_65h_table1[idx], VIVIDNESS_65h_SIZE1);
	}
}

static int dsi_update_mdnie_data(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tune_data *mdnie_data;

	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR(vdd, "fail to allocate mdnie_data memory\n");
		return -ENOMEM;
	}

	/* Update mdnie command */
	mdnie_data->DSI_COLOR_BLIND_MDNIE_1 = COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_1 = RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_2 = RGB_SENSOR_MDNIE_2;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE_2 = UI_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_UI_STANDARD_MDNIE_2 = UI_STANDARD_MDNIE_2;
	mdnie_data->DSI_UI_AUTO_MDNIE_2 = UI_AUTO_MDNIE_2;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE_2 = VIDEO_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE_2 = VIDEO_STANDARD_MDNIE_2;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE_2 = VIDEO_AUTO_MDNIE_2;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE_2 = CAMERA_AUTO_MDNIE_2;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE_2 = GALLERY_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE_2 = GALLERY_STANDARD_MDNIE_2;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE_2 = GALLERY_AUTO_MDNIE_2;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE_2 = BROWSER_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE_2 = BROWSER_STANDARD_MDNIE_2;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE_2 = BROWSER_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE_2 = EBOOK_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE_2 = EBOOK_STANDARD_MDNIE_2;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE_2 = EBOOK_AUTO_MDNIE_2;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE_2 = TDMB_DYNAMIC_MDNIE_2;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE_2 = TDMB_STANDARD_MDNIE_2;
	mdnie_data->DSI_TDMB_AUTO_MDNIE_2 = TDMB_AUTO_MDNIE_2;

	mdnie_data->DSI_BYPASS_MDNIE = BYPASS_MDNIE;
	mdnie_data->DSI_NEGATIVE_MDNIE = NEGATIVE_MDNIE;
	mdnie_data->DSI_COLOR_BLIND_MDNIE = COLOR_BLIND_MDNIE;
	mdnie_data->DSI_HBM_CE_MDNIE = HBM_CE_MDNIE;
	mdnie_data->DSI_HBM_CE_D65_MDNIE = HBM_CE_D65_MDNIE;
	mdnie_data->DSI_RGB_SENSOR_MDNIE = RGB_SENSOR_MDNIE;
	mdnie_data->DSI_UI_DYNAMIC_MDNIE = UI_DYNAMIC_MDNIE;
	mdnie_data->DSI_UI_STANDARD_MDNIE = UI_STANDARD_MDNIE;
	mdnie_data->DSI_UI_NATURAL_MDNIE = UI_NATURAL_MDNIE;
	mdnie_data->DSI_UI_AUTO_MDNIE = UI_AUTO_MDNIE;
	mdnie_data->DSI_VIDEO_DYNAMIC_MDNIE = VIDEO_DYNAMIC_MDNIE;
	mdnie_data->DSI_VIDEO_STANDARD_MDNIE = VIDEO_STANDARD_MDNIE;
	mdnie_data->DSI_VIDEO_NATURAL_MDNIE = VIDEO_NATURAL_MDNIE;
	mdnie_data->DSI_VIDEO_AUTO_MDNIE = VIDEO_AUTO_MDNIE;
	mdnie_data->DSI_CAMERA_AUTO_MDNIE = CAMERA_AUTO_MDNIE;
	mdnie_data->DSI_GALLERY_DYNAMIC_MDNIE = GALLERY_DYNAMIC_MDNIE;
	mdnie_data->DSI_GALLERY_STANDARD_MDNIE = GALLERY_STANDARD_MDNIE;
	mdnie_data->DSI_GALLERY_NATURAL_MDNIE = GALLERY_NATURAL_MDNIE;
	mdnie_data->DSI_GALLERY_AUTO_MDNIE = GALLERY_AUTO_MDNIE;
	mdnie_data->DSI_BROWSER_DYNAMIC_MDNIE = BROWSER_DYNAMIC_MDNIE;
	mdnie_data->DSI_BROWSER_STANDARD_MDNIE = BROWSER_STANDARD_MDNIE;
	mdnie_data->DSI_BROWSER_NATURAL_MDNIE = BROWSER_NATURAL_MDNIE;
	mdnie_data->DSI_BROWSER_AUTO_MDNIE = BROWSER_AUTO_MDNIE;
	mdnie_data->DSI_EBOOK_DYNAMIC_MDNIE = EBOOK_DYNAMIC_MDNIE;
	mdnie_data->DSI_EBOOK_STANDARD_MDNIE = EBOOK_STANDARD_MDNIE;
	mdnie_data->DSI_EBOOK_NATURAL_MDNIE = EBOOK_NATURAL_MDNIE;
	mdnie_data->DSI_EBOOK_AUTO_MDNIE = EBOOK_AUTO_MDNIE;
	mdnie_data->DSI_EMAIL_AUTO_MDNIE = EMAIL_AUTO_MDNIE;
	mdnie_data->DSI_GAME_LOW_MDNIE = BYPASS_MDNIE;
	mdnie_data->DSI_GAME_MID_MDNIE = BYPASS_MDNIE;
	mdnie_data->DSI_GAME_HIGH_MDNIE = BYPASS_MDNIE;
	mdnie_data->DSI_TDMB_DYNAMIC_MDNIE = TDMB_DYNAMIC_MDNIE;
	mdnie_data->DSI_TDMB_STANDARD_MDNIE = TDMB_STANDARD_MDNIE;
	mdnie_data->DSI_TDMB_NATURAL_MDNIE = TDMB_NATURAL_MDNIE;
	mdnie_data->DSI_TDMB_AUTO_MDNIE = TDMB_AUTO_MDNIE;
	mdnie_data->DSI_GRAYSCALE_MDNIE = GRAYSCALE_MDNIE;
	mdnie_data->DSI_GRAYSCALE_NEGATIVE_MDNIE = GRAYSCALE_NEGATIVE_MDNIE;
	mdnie_data->DSI_CURTAIN = SCREEN_CURTAIN_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE = NIGHT_MODE_MDNIE;
	mdnie_data->DSI_NIGHT_MODE_MDNIE_SCR = NIGHT_MODE_MDNIE_1;
	mdnie_data->DSI_COLOR_LENS_MDNIE = COLOR_LENS_MDNIE;
	mdnie_data->DSI_COLOR_LENS_MDNIE_SCR = COLOR_LENS_MDNIE_1;
	mdnie_data->DSI_COLOR_BLIND_MDNIE_SCR = COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_SCR = RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_HBM_CE_MDNIE_SCR_1 = HBM_CE_MDNIE1_1;
	mdnie_data->DSI_HBM_CE_MDNIE_SCR_2 = HBM_CE_MDNIE2_1;
	mdnie_data->DSI_HBM_CE_MDNIE_SCR_3 = HBM_CE_MDNIE3_1;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_1 = HBM_CE_MDNIE1_2;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_2 = HBM_CE_MDNIE2_2;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_3 = HBM_CE_MDNIE3_2;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi0;
	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;
	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;
	mdnie_data->hbm_ce_data = hbm_ce_data;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data->dsi_bypass_mdnie_size = ARRAY_SIZE(BYPASS_MDNIE);
	mdnie_data->mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data->mdnie_scr_cmd_offset = MDNIE_SCR_CMD_OFFSET;
	mdnie_data->mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = RGB_SENSOR_MDNIE_2_SIZE;

	mdnie_data->dsi_adjust_ldu_table = adjust_ldu_data;
	mdnie_data->dsi_max_adjust_ldu = 6;
	mdnie_data->dsi_night_mode_table = night_mode_data;
	mdnie_data->night_mode_vivid_enable = 1;
	mdnie_data->dsi_night_mode_vivid_0_table = night_mode_vivid_0;
	mdnie_data->dsi_night_mode_vivid_1_table = night_mode_vivid_1;
	mdnie_data->dsi_night_mode_vivid_2_table = night_mode_vivid_2;

	mdnie_data->dsi_max_night_mode_index = 102;
	mdnie_data->dsi_hbm_scr_table = hbm_scr_data;
	mdnie_data->dsi_color_lens_table = color_lens_data;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	mdnie_data->dsi_white_balanced_r = 0;
	mdnie_data->dsi_white_balanced_g = 0;
	mdnie_data->dsi_white_balanced_b = 0;
	mdnie_data->dsi_scr_buffer_white_r = SCR_BUFFER_WHITE_RED;
	mdnie_data->dsi_scr_buffer_white_g = SCR_BUFFER_WHITE_GREEN;
	mdnie_data->dsi_scr_buffer_white_b = SCR_BUFFER_WHITE_BLUE;
	mdnie_data->dsi_scr_step_index = MDNIE_STEP1_INDEX;
	mdnie_data->dsi_afc_size = 45;
	mdnie_data->dsi_afc_index = 33;

	vdd->mdnie.mdnie_data = mdnie_data;

	return 0;
}

#define MAX_READ_BUF_SIZE	(20)
static u8 read_buf[MAX_READ_BUF_SIZE];

static int ss_module_info_read(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds;
	int year, month, day;
	int hour, min;
	int x, y;
	int mdnie_tune_index = 0;
	char temp[50];
	int rx_len, len = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx\n", (size_t)vdd);
		return false;
	}

	pcmds = ss_get_cmds(vdd, RX_MODULE_INFO);
	if (pcmds->count <= 0) {
		LCD_ERR(vdd, "no module_info_rx_cmds cmds(%d)\n", vdd->panel_revision);
		return false;
	}

	rx_len = ss_send_cmd_get_rx(vdd, RX_MODULE_INFO, read_buf);
	if (rx_len < 0 || rx_len > MAX_READ_BUF_SIZE) {
		LCD_ERR(vdd, "invalid rx_len(%d)\n", rx_len);
		return false;
	}

	/* Manufacture Date */
	year = read_buf[4] & 0xf0;
	year >>= 4;
	year += 2011; /* 0 = 2011 year */
	month = read_buf[4] & 0x0f;
	day = read_buf[5] & 0x1f;
	hour = read_buf[6] & 0x0f;
	min = read_buf[7] & 0x1f;

	vdd->manufacture_date_dsi = year * 10000 + month * 100 + day;
	vdd->manufacture_time_dsi = hour * 100 + min;

	LCD_INFO(vdd, "manufacture_date (%d%04d), y:m:d=%d:%d:%d, h:m=%d:%d\n",
		vdd->manufacture_date_dsi, vdd->manufacture_time_dsi,
		year, month, day, hour, min);

	/* White Coordinates */
	vdd->mdnie.mdnie_x = read_buf[0] << 8 | read_buf[1];	/* X */
	vdd->mdnie.mdnie_y = read_buf[2] << 8 | read_buf[3];	/* Y */

	mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

	if (((vdd->mdnie.mdnie_x - 3050) * (vdd->mdnie.mdnie_x - 3050) + (vdd->mdnie.mdnie_y - 3210) * (vdd->mdnie.mdnie_y - 3210)) <= 225) {
		x = 0;
		y = 0;
	} else {
		x = mdnie_coordinate_x(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y, mdnie_tune_index);
		y = mdnie_coordinate_y(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y, mdnie_tune_index);
	}

	LCD_INFO(vdd, "X-%d Y-%d \n", vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

	/* CELL ID (manufacture date + white coordinates) */
	/* Manufacture Date */
	len = 0;
	len += sprintf(temp + len, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			read_buf[4], read_buf[5], read_buf[6], read_buf[7],
			read_buf[8], read_buf[9], read_buf[10], read_buf[0],
			read_buf[1], read_buf[2], read_buf[3]);

	vdd->cell_id_dsi = kzalloc(len, GFP_KERNEL);
	if (!vdd->cell_id_dsi) {
		LCD_ERR(vdd, "fail to kzalloc for cell_id_dsi\n");
		return false;
	}

	vdd->cell_id_len = len;
	strlcat(vdd->cell_id_dsi, temp, vdd->cell_id_len);
	LCD_INFO(vdd, "CELL ID: [%d] %s\n", vdd->cell_id_len, vdd->cell_id_dsi);

	pcmds = ss_get_cmds(vdd, RX_LOCAL_HBM_COMPENSATION);
	if (pcmds->count <= 0) {
		LCD_ERR(vdd, "no cmds local_hbm_compensation_rx\n");
		return false;
	}

	rx_len = ss_send_cmd_get_rx(vdd, RX_LOCAL_HBM_COMPENSATION, read_buf);
	if (rx_len < 0 || rx_len > MAX_READ_BUF_SIZE) {
		LCD_ERR(vdd, "invalid rx_len(%d)\n", rx_len);
		return false;
	}

	len = 0;
	len += sprintf(temp + len, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			read_buf[0], read_buf[1], read_buf[2], read_buf[3],
			read_buf[4], read_buf[5], read_buf[6], read_buf[7],
			read_buf[8], read_buf[9]);

	vdd->local_hbm_compensation_value = kzalloc(len, GFP_KERNEL);
	if (!vdd->local_hbm_compensation_value) {
		LCD_ERR(vdd, "fail to kzalloc for local_hbm_compensation\n");
		return false;
	}

	vdd->local_hbm_compensation_len = len;
	strlcat(vdd->local_hbm_compensation_value, temp, vdd->local_hbm_compensation_len);
	LCD_INFO(vdd, "LOCAL HBM COMPENSATION Read Value :[%s]\n", vdd->local_hbm_compensation_value);

	vdd->local_hbm_compensation_len = rx_len;
	for (i = 0; i < vdd->local_hbm_compensation_len; i++)
		vdd->local_hbm_compensation_value[i] = read_buf[i];

	pcmds = ss_get_cmds(vdd, TX_LOCAL_HBM_COMPENSATION);
	for (i = 0; i < vdd->local_hbm_compensation_len; i++)
		pcmds->cmds[2].ss_txbuf[i+1] = read_buf[i];

	ss_send_cmd(vdd, TX_LOCAL_HBM_COMPENSATION);
	LCD_INFO(vdd, "[LOCAL_HBM] WRITE\n");

	return true;
}

static int ss_post_vrr(struct samsung_display_driver_data *vdd,
			int old_rr, bool old_hs, bool old_phs,
			int new_rr, bool new_hs, bool new_phs)
{
	if (new_rr == 120 )
		vdd->dbg_tear_info.early_te_delay_us = 2000; /* 2ms */
	else
		vdd->dbg_tear_info.early_te_delay_us = 0;

	return 0;
}

static int ss_vrr_init(struct vrr_info *vrr)
{
	struct samsung_display_driver_data *vdd =
		container_of(vrr, struct samsung_display_driver_data, vrr);

	LCD_INFO(vdd, "+++\n");

	mutex_init(&vrr->vrr_lock);
	vrr->running_vrr_mdp = false;
	vrr->running_vrr = false;

	/* initial value : Bootloader: 120HS */
	vrr->prev_refresh_rate = vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 120;
	vrr->prev_sot_hs_mode = vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = true;
	vrr->prev_phs_mode = vrr->cur_phs_mode = vrr->adjusted_phs_mode = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	LCD_INFO(vdd, "---\n");
	return 0;
}

static int ss_ccd_read(struct samsung_display_driver_data *vdd, char *buf)
{
	u8 ccd[2] = {0,};
	int ret = ss_send_cmd_get_rx(vdd, TX_CCD_ON, ccd);

	if (ret <= 0) {
		LCD_ERR(vdd, "fail to read ccd state(%d)\n", ret);
		ret = snprintf((char *)buf, 6, "-1\n");
	} else {
		LCD_INFO(vdd, "CCD return (0x%02x)\n", ccd[0]);

		if (ccd[0] == vdd->ccd_pass_val || ccd[0] == 0x08) /* pass val=0x00 or 0x08 */
			ret = scnprintf((char *)buf, 6, "1\n");
		else if (ccd[0] == vdd->ccd_fail_val)
			ret = scnprintf((char *)buf, 6, "0\n");
		else
			ret = scnprintf((char *)buf, 6, "-1\n");
	}
	ss_send_cmd(vdd, TX_CCD_OFF);
	return ret;
}

static int ss_dsc_crc_read(struct samsung_display_driver_data *vdd, char *buf)
{
	struct sde_connector *conn;
	struct ddi_test_mode *test_mode;
	int ret = 0;
	u8 dsc_crc[8] = {0,};

	test_mode = &vdd->ddi_test[DDI_TEST_DSC_CRC];
	if (test_mode->pass_val == NULL) {
		LCD_ERR(vdd, "No pass_val for dsc_crc test..\n");
		return ret;
	}

	/* prevent sw reset to trigger esd recovery */
	LCD_INFO(vdd, "Disable ESD Interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_GCT_TEST);

	LCD_INFO(vdd, "Block Commit\n");
	ss_block_commit(vdd);
	LCD_INFO(vdd, "Kickoff_done!!\n");

	ret = ss_send_cmd_get_rx(vdd, TX_DSC_CRC, dsc_crc);
	if (ret > 0) {
		if (!memcmp(dsc_crc, test_mode->pass_val, test_mode->pass_val_size)) {
			LCD_INFO(vdd, "PASS [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X]\n",
				dsc_crc[0], dsc_crc[1], dsc_crc[2], dsc_crc[3], dsc_crc[4], dsc_crc[5], dsc_crc[6], dsc_crc[7]);
			ret = scnprintf((char *)buf, 40, "1 %02x %02x %02x %02x %02x %02x %02x %02x\n",
				dsc_crc[0], dsc_crc[1], dsc_crc[2], dsc_crc[3], dsc_crc[4], dsc_crc[5], dsc_crc[6], dsc_crc[7]);
		} else {
			LCD_INFO(vdd, "FAIL [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X]\n",
				dsc_crc[0], dsc_crc[1], dsc_crc[2], dsc_crc[3], dsc_crc[4], dsc_crc[5], dsc_crc[6], dsc_crc[7]);
			ret = scnprintf((char *)buf, 40, "-1 %02x %02x %02x %02x %02x %02x %02x %02x\n",
				dsc_crc[0], dsc_crc[1], dsc_crc[2], dsc_crc[3], dsc_crc[4], dsc_crc[5], dsc_crc[6], dsc_crc[7]);
		}
	} else {
		ret = scnprintf((char *)buf, 6, "-1\n");
	}

	LCD_INFO(vdd, "Release Commit\n");
	ss_release_commit(vdd);

	LCD_INFO(vdd, "Tx ss_off_cmd\n");
	ss_send_cmd(vdd, TX_DSI_CMD_SET_OFF);

	/* enable esd interrupt */
	LCD_INFO(vdd, "Enable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(true, true, (void *)vdd, ESD_MASK_GCT_TEST);

	/* hw reset */
	LCD_INFO(vdd, "Panel_dead event to reset panel\n");

	conn = GET_SDE_CONNECTOR(vdd);
	if (!conn)
		LCD_ERR(vdd, "Fail to get valid conn\n");
	else
		schedule_work(&conn->status_work.work);

	do {
		flush_work(&conn->status_work.work);
		msleep(500);
		LCD_INFO(vdd, "Wait for panel on\n");
	} while (!ss_is_panel_on(vdd));

	return ret;
}

void A36_S6E3FC5_AMS663FS01_FHD_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "%s\n", ss_get_panel_name(vdd));
	vdd->panel_state = PANEL_PWR_OFF;

	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;
	vdd->panel_func.samsung_display_on_post = samsung_display_on_post;

	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_module_info_read = ss_module_info_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;

	/* Brightness */
	vdd->panel_func.pre_brightness = ss_pre_brightness;
	vdd->panel_func.pre_lpm_brightness = ss_pre_lpm_brightness;

	vdd->br_info.acl_status = 1;		/* ACL default ON */
	vdd->br_info.gradual_acl_val = 1;	/* ACL default status in acl on */
	vdd->br_info.temperature = 20;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;
	vdd->mdnie.support_trans_dimming = false;
	vdd->mdnie.mdnie_tune_size[0] = sizeof(BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(BYPASS_MDNIE_2);
	vdd->panel_func.set_vividness = mdnie_set_vividness;
	dsi_update_mdnie_data(vdd);

	/* VRR */
	vdd->panel_func.post_vrr = ss_post_vrr;
	ss_vrr_init(&vdd->vrr);

	/* early te*/
	vdd->early_te = false;
	vdd->check_early_te = 0;

	vdd->panel_func.samsung_ccd_read     = ss_ccd_read;     /* ccd */
	vdd->panel_func.samsung_dsc_crc_read = ss_dsc_crc_read; /* dsc_crc */

	/* Below data will be genarated by script in Kbuild file */
	vdd->h_buf = A36_S6E3FC5_AMS663FS01_PDF_DATA;
	vdd->h_size = sizeof(A36_S6E3FC5_AMS663FS01_PDF_DATA);

	/* Get f_buf from header file data to cover recovery mode
	 * Below code should be called before any PDF parsing code such as parsing_glut
	 */
	if (!vdd->file_loading && vdd->h_buf) {
		LCD_ERR(vdd, "Get f_buf from header file data(%zu)\n", vdd->h_size);
		vdd->f_buf = vdd->h_buf;
		vdd->f_size = vdd->h_size;
	}
}
