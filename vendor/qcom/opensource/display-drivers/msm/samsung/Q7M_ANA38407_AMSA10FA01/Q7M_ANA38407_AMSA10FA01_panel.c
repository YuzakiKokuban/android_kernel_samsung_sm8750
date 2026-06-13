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

#include "Q7M_ANA38407_AMSA10FA01_panel.h"
#include "Q7M_ANA38407_AMSA10FA01_mdnie.h"

#define IRC_MODERATO_MODE_VAL	0x6B
#define IRC_FLAT_GAMMA_MODE_VAL	0x2B
#define get_bit(value, shift, width)	((value >> shift) & (GENMASK(width - 1, 0)))

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, false);
	else
		ss_panel_attach_set(vdd, true);

	switch (ss_panel_rev_get(vdd)) {
	case 0x00:
	case 0x01:
		vdd->panel_revision = 'A';
		break;
	case 0x02:
		vdd->panel_revision = 'B';
		break;
	case 0x03:
		vdd->panel_revision = 'C';
		break;
	case 0x04:
		vdd->panel_revision = 'D';
		break;
	case 0x05:
		vdd->panel_revision = 'E';
		break;
	case 0x06:
		vdd->panel_revision = 'F';
		break;
	case 0x07:
		vdd->panel_revision = 'G';
		break;
	default:
		vdd->panel_revision = 'G';
		LCD_ERR(vdd, "Invalid panel_rev(default rev : %c)\n", vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';
	LCD_INFO_ONCE(vdd, "panel_revision = %c %d\n", vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

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

#if 0
#define MAX_REG_CHECK 20
static int ss_sp_flash_init_check(struct samsung_display_driver_data *vdd)
{
	u8 ip08_check[MAX_REG_CHECK] = {0,};
	int rx_len, loop;

	/* Check IP08 Flash Initialized */
	if (ss_get_cmds(vdd, RX_SP_FLASH_CHECK)) {
		rx_len = ss_send_cmd_get_rx(vdd, RX_SP_FLASH_CHECK, ip08_check);

		for (loop = 0 ; loop < MAX_REG_CHECK ; loop++) {
			if (ip08_check[loop] != 0xFF)
				break;
		}

		if (loop == MAX_REG_CHECK) {
			LCD_INFO(vdd, "Already SP Flash Initialized\n");
			return false;
		}
	} else {
		LCD_ERR(vdd, "No RX_IP08_FLASH_CHECK cmds");
		return -EINVAL;
	}

	return true;
}
#endif

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	int idx = 0;
	struct dsi_panel_cmd_set  *on_cmds = ss_get_cmds(vdd, TX_DSI_CMD_SET_ON);

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, ": Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	idx = ss_get_cmd_idx(on_cmds, 0x00, 0x11);
#if 0
	if (idx >= 0) {
		if (ss_panel_revision(vdd) < 'C')
			on_cmds->cmds[idx].post_wait_ms = 120;
		else
			on_cmds->cmds[idx].post_wait_ms = 0;

		LCD_INFO(vdd, "Delay After Sleep Out = %d, Panel Rev = %c\n",
			on_cmds->cmds[idx].post_wait_ms, ss_panel_revision(vdd));
	}
#endif

	ss_panel_attach_set(vdd, true);
	LCD_INFO(vdd, "ndx=%d\n", vdd->ndx);

	return true;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	//int ret = 0;
	//struct sde_connector *conn;

	LCD_INFO(vdd, "ndx=%d\n", vdd->ndx);

	/*
	 * self mask is enabled from bootloader.
	 * so skip self mask setting during splash booting.
	 */
#if 0
	if (!vdd->samsung_splash_enabled) {
		if (vdd->self_disp.self_mask_img_write)
			vdd->self_disp.self_mask_img_write(vdd);
	} else {
		LCD_INFO(vdd, "samsung splash enabled.. skip image write\n");
	}
	/* self mask checksum */
	if (vdd->self_disp.self_display_debug)
		ret = vdd->self_disp.self_display_debug(vdd);

	/* self mask is turned on only when data checksum matches. */
	if (vdd->self_disp.self_mask_on && !ret)
		vdd->self_disp.self_mask_on(vdd, true);
	else {
		LCD_ERR(vdd, "Selfmask CheckSum Error. Skip Self Mask On!\n");
		if (sec_debug_is_enabled())
			panic("Display Selfmask CheckSum Error\n");
	}
#endif

	LCD_INFO(vdd, "manufacture_id_dsi=%x\n", vdd->manufacture_id_dsi);
#if 0	
	/* Tab S9 FW Update Panel Target LCD_ID = 0x800003 ~ 0x800004 */
	if (sec_debug_is_enabled() && (vdd->manufacture_id_dsi == 0x800003 || vdd->manufacture_id_dsi == 0x800004) && vdd->fw.is_support) {
		int ret = 0;

		if (vdd->fw.fw_id < 0x5 && vdd->fw.fw_id_read)
			vdd->fw.fw_id_read(vdd);

		LCD_INFO(vdd, "1st fw_id=%x, Working:0x%x(0x4:1st, 0xC:2nd)\n", vdd->fw.fw_id, vdd->fw.fw_working);

		ret = vdd->fw.fw_check(vdd, vdd->fw.fw_id);
		/* ret is 1, if needs update */
		if (ret == 1)
			vdd->fw.fw_update(vdd);
	}
#endif
#if 0
	/* SP Flash Initialize */
	if (vdd->gct.on && ss_sp_flash_init_check(vdd)) {
		LCD_INFO(vdd, "SP Flash Init Start\n");
		ss_send_cmd(vdd, TX_SP_FLASH_INIT);
		LCD_INFO(vdd, "SP Flash Init Finish\n");

		conn = GET_SDE_CONNECTOR(vdd);
		if (conn)
			schedule_work(&conn->status_work.work);
		else
			LCD_ERR(vdd, "Failed to get conn\n");
	}
#endif
	return true;
}

#undef COORDINATE_DATA_SIZE
#define COORDINATE_DATA_SIZE 6

#define F1(x, y) ((y)-((39*(x))/38)-95)
#define F2(x, y) ((y)-((36*(x))/35)-56)
#define F3(x, y) ((y)+((7*(x))/1)-24728)
#define F4(x, y) ((y)+((25*(x))/7)-14031)

/* Normal Mode */
static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_9 */
};

/* sRGB/Adobe RGB Mode */
static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* dummy */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfc, 0x00, 0xf6, 0x00}, /* Tune_9 */
};

static char (*coordinate_data_multi[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2, /* DYNAMIC - DCI */
	coordinate_data_2, /* STANDARD - sRGB/Adobe RGB */
	coordinate_data_2, /* NATURAL - sRGB/Adobe RGB */
	coordinate_data_1, /* MOVIE - Normal */
	coordinate_data_1, /* AUTO - Normal */
	coordinate_data_1, /* READING - Normal */
};

static int mdnie_coordinate_index(int x, int y)
{
	int tune_number = 0;

	if (F1(x, y) > 0) {
		if (F3(x, y) > 0) {
			tune_number = 3;
		} else {
			if (F4(x, y) < 0)
				tune_number = 1;
			else
				tune_number = 2;
		}
	} else {
		if (F2(x, y) < 0) {
			if (F3(x, y) > 0) {
				tune_number = 9;
			} else {
				if (F4(x, y) < 0)
					tune_number = 7;
				else
					tune_number = 8;
			}
		} else {
			if (F3(x, y) > 0)
				tune_number = 6;
			else {
				if (F4(x, y) < 0)
					tune_number = 4;
				else
					tune_number = 5;
			}
		}
	}

	return tune_number;
}

static int dsi_update_mdnie_data(struct samsung_display_driver_data *vdd)
{
	struct mdnie_lite_tune_data *mdnie_data;

	if (!vdd->mdnie.support_mdnie)
		return 0;

	mdnie_data = kzalloc(sizeof(struct mdnie_lite_tune_data), GFP_KERNEL);
	if (!mdnie_data) {
		LCD_ERR(vdd, "fail to allocate mdnie_data memory\n");
		return -ENOMEM;
	}

	/* Update mdnie command */
	mdnie_data->DSI_COLOR_BLIND_MDNIE_1 = COLOR_BLIND_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_1 = RGB_SENSOR_MDNIE_1;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_2 = RGB_SENSOR_MDNIE_2;
	mdnie_data->DSI_RGB_SENSOR_MDNIE_3 = RGB_SENSOR_MDNIE_3;
	mdnie_data->DSI_TRANS_DIMMING_MDNIE = RGB_SENSOR_MDNIE_3;
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
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_1 = HBM_CE_MDNIE1_3;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_2 = HBM_CE_MDNIE2_3;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_3 = HBM_CE_MDNIE3_3;
	mdnie_data->hbm_ce_data = hbm_ce_data;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi0;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi0;
	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi0;
	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi0;

	/* Update MDNIE data related with size, offset or index */
	mdnie_data->dsi_bypass_mdnie_size = ARRAY_SIZE(BYPASS_MDNIE);
	mdnie_data->mdnie_color_blinde_cmd_offset = MDNIE_COLOR_BLINDE_CMD_OFFSET;
	mdnie_data->mdnie_scr_cmd_offset = MDNIE_SCR_CMD_OFFSET;
	mdnie_data->mdnie_step_index[MDNIE_STEP1] = MDNIE_STEP1_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP2] = MDNIE_STEP2_INDEX;
	mdnie_data->mdnie_step_index[MDNIE_STEP3] = MDNIE_STEP3_INDEX;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_RED_OFFSET] = ADDRESS_SCR_WHITE_RED;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_GREEN_OFFSET] = ADDRESS_SCR_WHITE_GREEN;
	mdnie_data->address_scr_white[ADDRESS_SCR_WHITE_BLUE_OFFSET] = ADDRESS_SCR_WHITE_BLUE;
	mdnie_data->dsi_rgb_sensor_mdnie_1_size = RGB_SENSOR_MDNIE_1_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_2_size = RGB_SENSOR_MDNIE_2_SIZE;
	mdnie_data->dsi_rgb_sensor_mdnie_3_size = RGB_SENSOR_MDNIE_3_SIZE;

	mdnie_data->dsi_trans_dimming_data_index = MDNIE_TRANS_DIMMING_DATA_INDEX;
	mdnie_data->dsi_trans_dimming_slope_index = MDNIE_TRANS_DIMMING_SLOPE_INDEX;

	mdnie_data->dsi_adjust_ldu_table = adjust_ldu_data;
	mdnie_data->dsi_max_adjust_ldu = 6;
	mdnie_data->dsi_night_mode_table = night_mode_data;
	mdnie_data->dsi_max_night_mode_index = 102;
	mdnie_data->dsi_hbm_scr_table = hbm_scr_data;
	mdnie_data->dsi_color_lens_table = color_lens_data;
	mdnie_data->dsi_white_default_r = 0xff;
	mdnie_data->dsi_white_default_g = 0xff;
	mdnie_data->dsi_white_default_b = 0xff;
	mdnie_data->dsi_white_balanced_r = 0;
	mdnie_data->dsi_white_balanced_g = 0;
	mdnie_data->dsi_white_balanced_b = 0;
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
	int mdnie_tune_index = 0;
	char temp[50];
	int rx_len, len = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	/* A1h 5th~11th para + A1h 1st~4th para */
	pcmds = ss_get_cmds(vdd, RX_MODULE_INFO);
	if (pcmds->count <= 0) {
		LCD_ERR(vdd, "no module_info_rx_cmds cmds(%d)", vdd->panel_revision);
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
	year += 2011; // 0 = 2011 year
	month = read_buf[4] & 0x0f;
	day = read_buf[5] & 0x1f;
	hour = read_buf[6] & 0x0f;
	min = read_buf[7] & 0x1f;

	vdd->manufacture_date_dsi = year * 10000 + month * 100 + day;
	vdd->manufacture_time_dsi = hour * 100 + min;

	LCD_INFO(vdd, "manufacture_date DSI%d = (%d%04d) - year(%d) month(%d) day(%d) hour(%d) min(%d)\n",
		vdd->ndx, vdd->manufacture_date_dsi, vdd->manufacture_time_dsi,
		year, month, day, hour, min);

	/* While Coordinates */
	if (vdd->mdnie.support_mdnie) {
		vdd->mdnie.mdnie_x = read_buf[0] << 8 | read_buf[1];	/* X */
		vdd->mdnie.mdnie_y = read_buf[2] << 8 | read_buf[3];	/* Y */

		mdnie_tune_index = mdnie_coordinate_index(vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);
		coordinate_tunning_multi(vdd, coordinate_data_multi, mdnie_tune_index,	ADDRESS_SCR_WHITE_RED, COORDINATE_DATA_SIZE);
		LCD_INFO(vdd, "X-%d Y-%d \n", vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);
	}

	/* CELL ID (manufacture date + white coordinates) */
	/* Manufacture Date */
	len += sprintf(temp + len, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		read_buf[4], read_buf[5], read_buf[6], read_buf[7], read_buf[8], read_buf[9], read_buf[10],
		read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

	vdd->cell_id_dsi = kzalloc(len, GFP_KERNEL);
	if (!vdd->cell_id_dsi)
		LCD_ERR(vdd, "fail to kzalloc for cell_id_dsi\n");
	else {
		vdd->cell_id_len = len;
		strlcat(vdd->cell_id_dsi, temp, vdd->cell_id_len);
		LCD_INFO(vdd, "CELL ID : [%d] %s\n", vdd->cell_id_len, vdd->cell_id_dsi);
	}

	return true;
}
#if 0
static int ss_self_display_data_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "++\n");

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "Self Display is not supported\n");
		return -EINVAL;
	}

	vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_data;
	vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_data);
	vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_CHECKSUM;
	make_mass_self_display_img_cmds(vdd, FLAG_SELF_MASK, &vdd->self_disp.self_mask_cmd);

	LCD_INFO(vdd, "--\n");
	return 1;
}
#endif

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

static enum VRR_CMD_RR ss_get_vrr_mode(struct samsung_display_driver_data *vdd)
{
	int rr, hs, phs;
	enum VRR_CMD_RR vrr_id;

	rr = vdd->vrr.cur_refresh_rate;
	hs = vdd->vrr.cur_sot_hs_mode;
	phs = vdd->vrr.cur_phs_mode;

	switch (rr) {
	case 120:
		vrr_id = VRR_120HS;
		break;
	case 60:
		if (phs)
			vrr_id = VRR_60PHS;
		else
			vrr_id = VRR_60HS;
		break;
	case 30:
		if (phs)
			vrr_id = VRR_30PHS;
		else
			vrr_id = VRR_30HS;
		break;
	case 24:
		vrr_id = VRR_24PHS;
		break;
	case 10:
		vrr_id = VRR_10PHS;
		break;
	default:
		LCD_ERR(vdd, "invalid refresh rate (%d, %d), set default 120HS..\n", rr, hs);
		vrr_id = VRR_120HS;
		break;
	}

	return vrr_id;
}

static int ss_pre_brightness(struct samsung_display_driver_data *vdd)
{
	enum VRR_CMD_RR vrr_mode = ss_get_vrr_mode(vdd);

	/* For Optical Fingerprint Timing Tuning
	 * Dutration of TE Signal
	 * 120HS(60PHS,30PHS,24PHS,10PHS) : 544us
	 * 60HS : 8.88ms
	 */

	switch (vrr_mode) {
	case VRR_120HS:
		vdd->panel_hbm_entry_after_vsync_pre = 600;
		vdd->panel_hbm_exit_after_vsync_pre = 600;
		break;
	case VRR_60HS:
	case VRR_60PHS:
		vdd->panel_hbm_entry_after_vsync_pre = FRAME_WAIT_120FPS_US + 600; /* 9,000 */
		vdd->panel_hbm_exit_after_vsync_pre = FRAME_WAIT_120FPS_US + 600; /* 9,000 */
		break;
	case VRR_30PHS:
	case VRR_30HS:
		vdd->panel_hbm_entry_after_vsync_pre = FRAME_WAIT_120FPS_US * 3; /* 25,200 */
		vdd->panel_hbm_exit_after_vsync_pre = FRAME_WAIT_120FPS_US * 3; /* 25,200 */;
		break;
	case VRR_24PHS:
		vdd->panel_hbm_entry_after_vsync_pre = FRAME_WAIT_120FPS_US * 4; /* 33,600 */
		vdd->panel_hbm_exit_after_vsync_pre = FRAME_WAIT_120FPS_US * 4; /* 33,600 */
		break;
	case VRR_10PHS:
		vdd->panel_hbm_entry_after_vsync_pre = FRAME_WAIT_120FPS_US * 11 + 1000; /* 93,400 */
		vdd->panel_hbm_exit_after_vsync_pre = FRAME_WAIT_120FPS_US * 11 + 1000; /* 93,400 */
		break;
	default:
		LCD_ERR(vdd, "invalid refresh rate (%d), set default 120HS..\n", vrr_mode);
		vdd->panel_hbm_entry_after_vsync_pre = 0;
		vdd->panel_hbm_exit_after_vsync_pre = 0;
		break;
	}

	LCD_INFO_IF(vdd, "HBM Entry After VSYNC=(%d,%d), HBM Exit After VSYNC=(%d,%d)\n",
				vdd->panel_hbm_entry_after_vsync_pre,
				vdd->panel_hbm_entry_after_vsync,
				vdd->panel_hbm_exit_after_vsync_pre,
				vdd->panel_hbm_exit_after_vsync);

	return 0;
}

static int ss_gct_read(struct samsung_display_driver_data *vdd)
{
	int res;

	if (!vdd->gct.on)
		return GCT_RES_CHECKSUM_OFF;

	if (memcmp(vdd->gct.checksum, vdd->gct.valid_checksum, 2))
		res = GCT_RES_CHECKSUM_PASS;
	else
		res = GCT_RES_CHECKSUM_NG;

	LCD_INFO(vdd, "res=%d Result=(0x%02X 0x%02X), Fail=(0x%02X 0x%02X)\n", res,
		vdd->gct.checksum[0], vdd->gct.checksum[1],
		vdd->gct.valid_checksum[0], vdd->gct.valid_checksum[1]);

	return res;
}

static int ss_gct_write(struct samsung_display_driver_data *vdd)
{
	struct sde_connector *conn;
	int ret = 0;

	LCD_INFO(vdd, "+++\n");

	ss_set_test_mode_state(vdd, PANEL_TEST_GCT);

	vdd->gct.is_running = true;

	/* prevent sw reset to trigger esd recovery */
	LCD_INFO(vdd, "disable esd interrupt\n");
	if (vdd->esd_recovery.esd_irq_enable)
		vdd->esd_recovery.esd_irq_enable(false, true, (void *)vdd, ESD_MASK_GCT_TEST);

	LCD_INFO(vdd, "gct : block commit\n");
	atomic_inc(&vdd->block_commit_cnt);
	ss_wait_for_kickoff_done(vdd);
	LCD_INFO(vdd, "gct : kickoff_done!!\n");

	ret = ss_send_cmd_get_rx(vdd, TX_GCT_ENTER, &vdd->gct.checksum[0]);
	if (ret <= 0)
		goto rx_err;

	LCD_INFO(vdd, "Check Value = {0x%x 0x%x}\n", vdd->gct.checksum[0], vdd->gct.checksum[1]);

	vdd->gct.on = 1;

	LCD_INFO(vdd, "release commit\n");
	atomic_add_unless(&vdd->block_commit_cnt, -1, 0);
	wake_up_all(&vdd->block_commit_wq);

	vdd->gct.is_running = false;

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

	LCD_INFO(vdd, "---\n");

	ss_set_test_mode_state(vdd, PANEL_TEST_NONE);

	return 0;

rx_err:
	LCD_ERR(vdd, "Fail to read checksum via mipi(%d)\n", ret);

	return ret;
}

#if 0
static enum VRR_CMD_RR ss_get_vrr_mode_base(struct samsung_display_driver_data *vdd, int rr, bool hs, bool phs)
{
	enum VRR_CMD_RR vrr_base;

	switch (rr) {
	case 120:
		vrr_base = VRR_120HS;
		break;
	case 60:
		if (phs)
			vrr_base = VRR_120HS;
		else
			vrr_base = VRR_60HS;
		break;
	case 30:
		if (phs)
			vrr_base = VRR_120HS;
		else
			vrr_base = VRR_60HS;
		break;
	case 24:
		vrr_base = VRR_120HS;
		break;
	case 10:
		vrr_base = VRR_120HS;
		break;
	default:
		LCD_ERR(vdd, "invalid refresh rate (%d, %d), set default 120HS..\n", rr, hs);
		vrr_base = VRR_120HS;
		break;
	}

	return vrr_base;
}

static int update_glut(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int cur_rr = state->cur_refresh_rate;
	bool cur_hs = state->sot_hs;
	bool cur_phs = state->sot_phs;
	enum VRR_CMD_RR vrr_base = ss_get_vrr_mode_base(vdd, cur_rr, cur_hs, cur_phs);
	int bl_lvl = state->bl_level;
	struct cmd_legoop_map *glut_map;
	int first_pos = -1, j;

	if (vdd->br_info.glut_skip) {
		LCD_ERR(vdd, "skip GLUT\n");
		goto err_skip;
	}

	/* Find first 0xXX position */
	while (!cmd->pos_0xXX[++first_pos] && first_pos < cmd->tx_len);

	/* cmd length overflow check */
	if (cmd->txbuf[0] == 0xEB) {
		if (first_pos + GLUT_CMD_LEN_0xEB > cmd->tx_len) {
			LCD_ERR(vdd, "Fail to find 0xEB cmd proper 0xXX position(%d, %d)\n",
					first_pos, cmd->tx_len);
			goto err_skip;
		}
	} else if (cmd->txbuf[0] == 0xEC) {
		if (first_pos + GLUT_CMD_LEN_0xEC > cmd->tx_len) {
			LCD_ERR(vdd, "Fail to find 0xEC cmd proper 0xXX position(%d, %d)\n",
					first_pos, cmd->tx_len);
			goto err_skip;
		}
	} else {
		LCD_ERR(vdd, "Invalid Register Address 0x%x\n", cmd->txbuf[0]);
		goto err_skip;
	}

	if (vrr_base == VRR_120HS)
		glut_map = &vdd->br_info.glut_offset_120hs;
	else if (vrr_base == VRR_60HS)
		glut_map = &vdd->br_info.glut_offset_60hs;
	else
		glut_map = NULL;

	if (glut_map) {
		if (vdd->br_info.glut_00_val) {
			if (cmd->txbuf[0] == 0xEB)
				memset(&cmd->txbuf[first_pos], 0x00, GLUT_CMD_LEN_0xEB);
			else
				memset(&cmd->txbuf[first_pos], 0x00, GLUT_CMD_LEN_0xEC);
		} else {
			if (bl_lvl > glut_map->row_size) {
				LCD_ERR(vdd, "invalid bl [%d]/[%d]\n", bl_lvl, glut_map->row_size);
				goto err_skip;
			}

			if (cmd->txbuf[0] == 0xEB)
				for (j = 0; j < GLUT_CMD_LEN_0xEB; j++)
					cmd->txbuf[first_pos + j] = glut_map->cmds[bl_lvl][j];
			else
				for (j = 0; j < GLUT_CMD_LEN_0xEC; j++)
					cmd->txbuf[first_pos + j] = glut_map->cmds[bl_lvl][j + GLUT_CMD_LEN_0xEB];
		}

		LCD_INFO_IF(vdd, "(0x%x) first_pos=%d, bl_lvl=%d, row_size=%d, col_size=%d\n",
			cmd->txbuf[0], first_pos, bl_lvl, glut_map->row_size, glut_map->col_size);
	} else {
		LCD_ERR(vdd, "Failed to get GLUT MAP\n");
		cmd->skip_by_cond = true;
	}

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return -EINVAL;
}

static void glut_map_init(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	struct device_node *np;

	np = ss_get_panel_of(vdd);
	if (!np) {
		LCD_ERR(vdd, "No panel np..\n");
		return;
	}

	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_60hs, sizeof(struct cmd_map),
				"samsung,glut_offset_60HS_table_rev", 0, ss_parse_panel_legoop_table_str);
	if (ret)
		LCD_ERR(vdd, "Failed to parse GLUT_OFFSET_60HS data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_60HS data from panel_data_file\n");

	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_120hs, sizeof(struct cmd_map),
				"samsung,glut_offset_120HS_table_rev", 0, ss_parse_panel_legoop_table_str);
	if (ret)
		LCD_ERR(vdd, "Failed to parse GLUT_OFFSET_120HS data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_120HS data from panel_data_file\n");
}
#endif

static int ss_vrr_init(struct vrr_info *vrr)
{
	struct samsung_display_driver_data *vdd =
		container_of(vrr, struct samsung_display_driver_data, vrr);

	LCD_INFO(vdd, "+++\n");

	mutex_init(&vrr->vrr_lock);

	vrr->running_vrr_mdp = false;
	vrr->running_vrr = false;

	vrr->prev_refresh_rate = vrr->cur_refresh_rate = vrr->adjusted_refresh_rate = 120;
	vrr->prev_sot_hs_mode = vrr->cur_sot_hs_mode = vrr->adjusted_sot_hs_mode = true;
	vrr->prev_phs_mode = vrr->cur_phs_mode = vrr->adjusted_phs_mode = false;

	vrr->vrr_workqueue = create_singlethread_workqueue("vrr_workqueue");
	INIT_WORK(&vrr->vrr_work, ss_panel_vrr_switch_work);

	LCD_INFO(vdd, "---\n");

	return 0;
}

static int wait_tcon_ready(struct samsung_display_driver_data *vdd, int rdy_gpio)
{
	int i;
	int max_wait_cnt = 500; /* max 1000ms */
	static ktime_t elapsed_time;

	LCD_INFO(vdd, "ANAPASS DDI: +: tcon_rdy val: %d\n", gpio_get_value(rdy_gpio));
	elapsed_time = ktime_get();
	for (i = 0; i < max_wait_cnt; i++) {
		if (gpio_get_value(rdy_gpio)) {
			LCD_INFO(vdd, "ANAPASS DDI: tcon_rdy becomes level high!!!\n");
			break;
		}
		usleep_range(2000, 2010);
	}
	elapsed_time = ktime_get() - elapsed_time;
	LCD_INFO(vdd, "ANAPASS DDI: -: tcon_rdy val: %d, elapsed_time: %lld[ms]\n",
		gpio_get_value(rdy_gpio), ktime_to_ms(elapsed_time));

	if (i == max_wait_cnt)
		return false;
	else
		return true;
}

/* Wait ultil gpio TCON_READY goes H */
static int ss_panel_power_ctrl_specific(struct samsung_display_driver_data *vdd, struct pwr_node *pwr, bool enable)
{
	//int num_of_v = 0;
	int rc = -EINVAL;
	int trdy = pwr->gpio;

	LCD_INFO(vdd, "TCON_RDY] +++\n");

	if (vdd->samsung_splash_enabled) {
		LCD_INFO(vdd, "TCON_RDY] splash skip\n");
		return 0;
	}

	//boost here -> move to on_pre?
	//ss_boost_control(vdd);

	if (gpio_is_valid(trdy)) {
		rc = gpio_direction_input(pwr->gpio);
		if (rc) {
			LCD_ERR(vdd, "fail to set gpio(%d) dir, rc=%d, name: [%s]\n",
					pwr->gpio, rc, pwr->name ? pwr->name : "");
			return rc;
		}
		LCD_INFO(vdd, "TCON_RDY] val[%d]: %d, no need if already 1.\n",
			trdy, gpio_get_value(trdy));
		if (gpio_get_value(trdy)) {
			LCD_INFO(vdd, "TCON_RDY] already 1\n");
			return rc;
		}
	} else {
		LCD_ERR(vdd, "TCON_RDY] trdy gpio is not valid!\n");
	}


	if (gpio_is_valid(trdy)) {
		if (!wait_tcon_ready(vdd, trdy)) {
			LCD_INFO(vdd, "ANAPASS DDI tcon_rdy fail \n");
			//panic("ANAPASS DDI tcon_rdy fail");
		}
	} else {
		LCD_INFO(vdd, "ANAPASS DDI tcon_rdy force wait 60ms \n");
		usleep_range(60000, 61000);
	}


	LCD_INFO(vdd, "TCON_RDY] ---\n");

	return 0;
}

static int ss_panel_power_parse_boost(struct samsung_display_driver_data *vdd, struct pwr_node *pwr,
			struct device_node *np)
{
	LCD_INFO(vdd, "\n");

	return 0;
}

static int ss_panel_power_ctrl_boost(struct samsung_display_driver_data *vdd, struct pwr_node *pwr, bool enable)
{
	LCD_INFO(vdd, " +++\n");

	if (vdd->samsung_splash_enabled) {
		LCD_INFO(vdd, "TCON_RDY] splash skip\n");
		return 0;;
	}

	ss_boost_control(vdd, enable);

	LCD_INFO(vdd, " ---\n");

	return 0;
}

void Q7M_ANA38407_AMSA10FA01_WQXGA_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "%s\n", ss_get_panel_name(vdd));

	/* Default Panel Power Status is OFF */
	vdd->panel_state = PANEL_PWR_OFF;

	/* ON/OFF */
	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;
	vdd->panel_func.samsung_panel_off_pre = samsung_panel_off_pre;
	vdd->panel_func.samsung_panel_off_post = samsung_panel_off_post;

	/* DDI RX */
	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_module_info_read = ss_module_info_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;

	/* Brightness */
	vdd->panel_func.pre_brightness = ss_pre_brightness;

	/* default brightness */
	vdd->br_info.common_br.bl_level = 255;

	/* mdnie */
	vdd->mdnie.support_mdnie = false; //

	vdd->mdnie.support_trans_dimming = false; //
	vdd->mdnie.mdnie_tune_size[0] = sizeof(BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[2] = sizeof(BYPASS_MDNIE_3);

	dsi_update_mdnie_data(vdd);

	/* Enable panic on first pingpong timeout */
	//vdd->debug_data->panic_on_pptimeout = true;

	/* ACL default ON */
	vdd->br_info.acl_status = 1;

	/*Default Temperature*/
	vdd->br_info.temperature = 20;

	/* Self display */
	vdd->self_disp.is_support = false;
	vdd->self_disp.factory_support = false;
	//vdd->self_disp.init = self_display_init;
	//vdd->self_disp.data_init = ss_self_display_data_init;

	/* Boost control */
//	vdd->panel_func.samsung_boost_control = ss_boost_control;

	/* Memory Checksum Test */
	vdd->panel_func.samsung_gct_write = ss_gct_write;
	vdd->panel_func.samsung_gct_read = ss_gct_read;

	/* SAMSUNG_FINGERPRINT */
	//vdd->panel_hbm_entry_delay = 0;
	//vdd->panel_hbm_exit_delay = 0;

	/* Below data will be genarated by script in Kbuild file */
	vdd->h_buf = Q7M_ANA38407_AMSA10FA01_PDF_DATA;
	vdd->h_size = sizeof(Q7M_ANA38407_AMSA10FA01_PDF_DATA);

	/* Get f_buf from header file data to cover recovery mode
	 * Below code should be called before any PDF parsing code such as update_glut_map
	 */
	if (!vdd->file_loading && vdd->h_buf) {
		LCD_INFO(vdd, "Get f_buf from header file data(%zu)\n", vdd->h_size);
		vdd->f_buf = vdd->h_buf;
		vdd->f_size = vdd->h_size;
	}

	/* VRR */
	ss_vrr_init(&vdd->vrr);

	/* Disp pmic init */
	ss_boost_max77816_init();

	/* Init GLUT Offset Table */
	//glut_map_init(vdd);
	/* Panel Specific Symbol */
	//register_op_sym_cb(vdd, "GLUT", update_glut, true);

	/* FW Update init: not parsed yet, just call init */
	//fw_update_init_ANA38407(vdd);
	
	vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11].parse_cb = ss_panel_power_parse_boost;
	vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11].ctrl_cb = ss_panel_power_ctrl_boost;
	vdd->panel_powers[PANEL_POWERS_ON_POST_LP11].ctrl_cb = ss_panel_power_ctrl_specific;

	vdd->debug_data->print_cmds = false;

	LCD_INFO(vdd, "Q7M_ANA38407_AMSA10FA01 : --\n");
}
