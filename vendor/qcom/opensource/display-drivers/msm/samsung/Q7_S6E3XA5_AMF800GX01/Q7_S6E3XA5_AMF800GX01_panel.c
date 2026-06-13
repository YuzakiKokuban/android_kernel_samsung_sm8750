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
#include "Q7_S6E3XA5_AMF800GX01_panel.h"
#include "Q7_S6E3XA5_AMF800GX01_mdnie.h"

#define MAX_READ_BUF_SIZE	(20)
static u8 read_buf[MAX_READ_BUF_SIZE];

static int samsung_panel_on_pre(struct samsung_display_driver_data *vdd)
{
	ss_panel_attach_set(vdd, true);

	LCD_INFO(vdd, "-: ndx=%d\n", vdd->ndx);

	return 0;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

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

	/* self mask checksum */
	if (vdd->self_disp.self_display_debug)
		ret = vdd->self_disp.self_display_debug(vdd);

	/* self mask is turned on only when data checksum matches. */
	if (vdd->self_disp.self_mask_on && !ret)
		vdd->self_disp.self_mask_on(vdd, true);

	/* mafpc */
	//if (vdd->mafpc.is_support) {
	//	vdd->mafpc.need_to_write = true;
	//	LCD_INFO(vdd, "Need to write mafpc image data to DDI\n");
	//}

	return 0;
}

static char ss_panel_revision(struct samsung_display_driver_data *vdd)
{
	if (vdd->manufacture_id_dsi == PBA_ID)
		ss_panel_attach_set(vdd, false);
	else
		ss_panel_attach_set(vdd, true);

	switch (ss_panel_rev_get(vdd)) {
	case 0x00:
		vdd->panel_revision = 'A';
		break;
	case 0x01:
	case 0x02:
		vdd->panel_revision = 'B';
		break;
	case 0x03:
		vdd->panel_revision = 'C';
		break;
	case 0x04:
	case 0x05:
		vdd->panel_revision = 'D';
		break;
	default:
		vdd->panel_revision = 'D';
		LCD_ERR(vdd, "Invalid panel_rev(default rev : %c)\n", vdd->panel_revision);
		break;
	}

	vdd->panel_revision -= 'A';
	LCD_INFO_ONCE(vdd, "panel_revision = %c %d\n", vdd->panel_revision + 'A', vdd->panel_revision);

	return (vdd->panel_revision + 'A');
}

static int ss_update_base_lfd_val(struct vrr_info *vrr,
			enum LFD_SCOPE_ID scope, struct lfd_base_str *lfd_base)
{
	u32 base_rr, max_div_def, min_div_def, min_div_lowest;
	struct samsung_display_driver_data *vdd =
		container_of(vrr, struct samsung_display_driver_data, vrr);
	struct lfd_mngr *mngr;
	int cur_rr = vdd->vrr.cur_refresh_rate;

	if (scope == LFD_SCOPE_LPM) {
		base_rr = 30;
		max_div_def = 1;	/* default 1HZ */
		min_div_def = 30;
		min_div_lowest = 30;
		goto done;
	}

	mngr = &vrr->lfd.lfd_mngr[LFD_CLIENT_DISP];

	switch (cur_rr) {
	case 10:
		base_rr = 120;
		max_div_def = 12; // 10hz
		min_div_def = 12;   // 10hz for scalability 5
		min_div_lowest = 120; // 1hz for scalability 6
		break;
	case 24:
		base_rr = 120;
		max_div_def = 5; // 24hz
		min_div_def = 12; // 10hz
		min_div_lowest = 120; // 1hz for scalability 6
		break;
	case 30:
		base_rr = 120;
		max_div_def = 4; // 30hz
		min_div_def = 12; // 10hz
		min_div_lowest = 120; // 1hz for scalability 6
		break;
	case 48:
		base_rr = 96;
		max_div_def = 2; // 48hz
		min_div_def = 9; // 10.67hz
		min_div_lowest = 96; // 1hz for scalability 6
		break;
	case 60:
		base_rr = 120;
		max_div_def = 2; // 60hz
		min_div_def = 12; // 10hz
		min_div_lowest = 120; // 1hz for scalability 6
		break;
	case 96:
		base_rr = 96;
		max_div_def = 1; // 96hz
		min_div_def = 9; // 10.67hz
		min_div_lowest = 96; // 1hz for scalability 6
		break;
	case 120:
		base_rr = 120;
		max_div_def = 1; // 120hz
		min_div_def = 12; // 10hz
		min_div_lowest = 120; // 1hz for scalability 6
		break;
	default:
		LCD_ERR(vdd, "invalid refresh_rate\n");
		base_rr = 120;
		max_div_def = 1; // 120hz
		min_div_def = min_div_lowest = 120; // 1hz
		break;
	}

	/* HBM 1Hz */
	if (mngr->scalability[LFD_SCOPE_NORMAL] == LFD_FUNC_SCALABILITY6) {
		min_div_def = min_div_lowest;
	}

done:
	lfd_base->base_rr = base_rr;
	lfd_base->max_div_def = max_div_def;
	lfd_base->min_div_def = min_div_def;
	lfd_base->min_div_lowest = min_div_lowest;
	lfd_base->fix_div_def = 1; // LFD MAX/MIN 120hz fix
	lfd_base->highdot_div_def = 120 * 2; // 120hz % 240 = 0.5hz for highdot test (120hz base)

	vrr->lfd.base_rr = base_rr;

	LCD_DEBUG(vdd, "LFD(%s): base_rr: %uhz, def: %uhz(%u)~%uhz(%u), lowest: %uhz(%u) hightdot_div: %u\n",
			lfd_scope_name[scope], base_rr,
			DIV_ROUND_UP(base_rr, min_div_def), min_div_def,
			DIV_ROUND_UP(base_rr, max_div_def), max_div_def,
			DIV_ROUND_UP(base_rr, min_div_lowest), min_div_lowest,
			lfd_base->highdot_div_def);

	return 0;
}

#if 0
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
			schedule_delayed_work(&vdd->esd_enable_event_work,
				msecs_to_jiffies(ESD_WORK_DELAY));
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
			schedule_delayed_work(&vdd->esd_enable_event_work,
				msecs_to_jiffies(ESD_WORK_DELAY));
		}

		vdd->br_info.last_br_is_hbm = true;
	}

	vdd->vrr.need_vrr_update = false;

	return 0;
}
#endif

#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR	0x32
#define RGB_INDEX_SIZE 4
#define COEFFICIENT_DATA_SIZE 8

#define F1(x, y) (((y << 10) - (((x << 10) * 56) / 55) - (102 << 10)) >> 10)
#define F2(x, y) (((y << 10) + (((x << 10) * 5) / 1) - (18483 << 10)) >> 10)

static char coordinate_data_1[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfa, 0x00, 0xf9, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfc, 0x00, 0xff, 0x00}, /* Tune_2 */
	{0xf8, 0x00, 0xf7, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfd, 0x00, 0xf9, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xf8, 0x00, 0xfa, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfd, 0x00, 0xff, 0x00, 0xf8, 0x00}, /* Tune_7 */
	{0xfb, 0x00, 0xff, 0x00, 0xfc, 0x00}, /* Tune_8 */
	{0xf8, 0x00, 0xfd, 0x00, 0xff, 0x00}, /* Tune_9 */
};

static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_8 */
	{0xfc, 0x00, 0xfb, 0x00, 0xf5, 0x00}, /* Tune_9 */
};

static char (*coordinate_data[MAX_MODE])[COORDINATE_DATA_SIZE] = {
	coordinate_data_2,
	coordinate_data_2,
	coordinate_data_2,
	coordinate_data_1,
	coordinate_data_1,
	coordinate_data_1
};

static int rgb_index[][RGB_INDEX_SIZE] = {
	{ 5, 5, 5, 5 }, /* dummy */
	{ 5, 2, 6, 3 },
	{ 5, 2, 4, 1 },
	{ 5, 8, 4, 7 },
	{ 5, 8, 6, 9 },
};

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
		if (F2(x, y) > 0) {
			tune_number = 1;
		} else {
			tune_number = 2;
		}
	} else {
		if (F2(x, y) > 0) {
			tune_number = 4;
		} else {
			tune_number = 3;
		}
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

/* DFh */
static u8 vividness_skin_tone_table[VIVIDNESS_MAX_IDX][VIVIDNESS_SKIN_TONE_SIZE] = {
	{0xE1, 0x40, 0x40, 0xFC, 0xEA, 0x20, 0xFF, 0x10, 0xF0, 0xFF, 0xFF, 0xF6}, /* default */
	{0xDD, 0x3F, 0x20, 0xFA, 0xF2, 0x20, 0xF5, 0x34, 0xF2, 0xFF, 0xFF, 0xFF},
	{0xD4, 0x43, 0x50, 0xEF, 0xED, 0x00, 0xF5, 0x34, 0xF2, 0xFF, 0xFF, 0xFF},
};

/* DEh */
static u8 vividness_3x3_matrix_table[VIVIDNESS_MAX_IDX][VIVIDNESS_3x3_MATRIX_SIZE] = {
	{0x04, 0x3F, 0x1F, 0xCB, 0x1F, 0xF6, 0x1F, 0xFF, 0x04, 0x11, 0x1F, 0xF6, 0x1F, 0xFF, 0x1F, 0xCB, 0x04, 0x36}, /* default */
	{0x04, 0x71, 0x1F, 0x9C, 0x1F, 0xF3, 0x1F, 0xE9, 0x04, 0x24, 0x1F, 0xF3, 0x1F, 0xE9, 0x1F, 0x9C, 0x04, 0x7B},
	{0x04, 0xA2, 0x1F, 0x6D, 0x1F, 0xF1, 0x1F, 0xD4, 0x04, 0x3B, 0x1F, 0xF1, 0x1F, 0xD4, 0x1F, 0x6D, 0x04, 0xBF},
};

/* DEh for night mode */
static u8 vividness_3x3_matrix_night_mode_table[VIVIDNESS_MAX_IDX][VIVIDNESS_3x3_MATRIX_SIZE] = {
	{0x04, 0x3F, 0x1F, 0xCB, 0x1F, 0xF6, 0x1F, 0xFF, 0x04, 0x11, 0x1F, 0xF6, 0x1F, 0xFF, 0x1F, 0xCB, 0x04, 0x36}, /* default */
	{0x04, 0x71, 0x1F, 0x9C, 0x1F, 0xF3, 0x1F, 0xE9, 0x04, 0x24, 0x1F, 0xF3, 0x1F, 0xE9, 0x1F, 0x9C, 0x04, 0x7B},
	{0x04, 0xA2, 0x1F, 0x6D, 0x1F, 0xF1, 0x1F, 0xD4, 0x04, 0x3B, 0x1F, 0xF1, 0x1F, 0xD4, 0x1F, 0x6D, 0x04, 0xBF},
};

static void mdnie_set_vividness(struct samsung_display_driver_data *vdd, struct mdnie_lite_tun_type *tune, struct dsi_cmd_desc *tune_data)
{
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	int idx = tune->vividness_idx;

	/* Do not apply (color lens, natural mode) */
	if (tune->color_lens_enable || tune->mdnie_mode == DYNAMIC_MODE)
		return;

	if (idx >= 0 && idx < VIVIDNESS_MAX_IDX) {
		if (tune->night_mode_enable) {
			memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP2]].ss_txbuf[VIVIDNESS_3x3_MATRIX_IDX],
				vividness_3x3_matrix_night_mode_table[idx], VIVIDNESS_3x3_MATRIX_SIZE);
		} else {
			memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP2]].ss_txbuf[VIVIDNESS_3x3_MATRIX_IDX],
				vividness_3x3_matrix_table[idx], VIVIDNESS_3x3_MATRIX_SIZE);

			memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP1]].ss_txbuf[VIVIDNESS_SKIN_TONE_IDX],
				vividness_skin_tone_table[idx], VIVIDNESS_SKIN_TONE_SIZE);
		}
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
	mdnie_data->DSI_AFC = DSI_AFC;
	mdnie_data->DSI_AFC_ON = DSI_AFC_ON;
	mdnie_data->DSI_AFC_OFF = DSI_AFC_OFF;
	mdnie_data->DSI_HBM_CE_MDNIE_SCR_1 = HBM_CE_MDNIE1_1;
	mdnie_data->DSI_HBM_CE_MDNIE_SCR_2 = HBM_CE_MDNIE2_1;
	mdnie_data->DSI_HBM_CE_MDNIE_SCR_3 = HBM_CE_MDNIE3_1;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_1 = HBM_CE_MDNIE1_3;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_2 = HBM_CE_MDNIE2_3;
	mdnie_data->DSI_HBM_CE_MDNIE_DIMMING_3 = HBM_CE_MDNIE3_3;

	mdnie_data->mdnie_tune_value_dsi = mdnie_tune_value_dsi;
	mdnie_data->hmt_color_temperature_tune_value_dsi = hmt_color_temperature_tune_value_dsi;

	mdnie_data->hdr_tune_value_dsi = hdr_tune_value_dsi;

	mdnie_data->light_notification_tune_value_dsi = light_notification_tune_value_dsi;

	mdnie_data->hbm_ce_data = hbm_ce_data;

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
	mdnie_data->dsi_max_night_mode_index = 306;
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
	mdnie_data->dsi_afc_size = 71;
	mdnie_data->dsi_afc_index = 56;

	vdd->mdnie.mdnie_data = mdnie_data;

	return 0;
}

static int ss_module_info_read(struct samsung_display_driver_data *vdd)
{
	struct dsi_panel_cmd_set *pcmds;
	int year, month, day;
	int hour, min;
	int x, y;
	int mdnie_tune_index = 0;
	char temp[50];
	int rx_len, len = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

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

	/* While Coordinates */

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

	coordinate_tunning_calculate(vdd, x, y, coordinate_data,
			rgb_index[mdnie_tune_index],
			MDNIE_SCR_WR_ADDR, COORDINATE_DATA_SIZE);

	LCD_INFO(vdd, "X-%d Y-%d \n", vdd->mdnie.mdnie_x, vdd->mdnie.mdnie_y);

	/* CELL ID (manufacture date + white coordinates) */
	/* Manufacture Date */
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

	return true;
}

static int ss_pre_lpm_brightness(struct samsung_display_driver_data *vdd)
{
	vdd->br_info.last_br_is_hbm = false;

	return 0;
}

#if 0
static int ss_self_display_data_init(struct samsung_display_driver_data *vdd)
{
	u32 panel_type = 0;
	u32 panel_color = 0;

	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "Self Display is not supported\n");
		return -EINVAL;
	}

	LCD_INFO(vdd, "Self Display Panel Data init\n");

	panel_type = (ss_panel_id0_get(vdd) & 0x30) >> 4;
	panel_color = ss_panel_id0_get(vdd) & 0xF;

	LCD_INFO(vdd, "Panel Type=0x%x, Panel Color=0x%x\n", panel_type, panel_color);

	vdd->self_disp.operation[FLAG_SELF_MASK].img_buf = self_mask_img_data;
	vdd->self_disp.operation[FLAG_SELF_MASK].img_size = ARRAY_SIZE(self_mask_img_data);
	vdd->self_disp.operation[FLAG_SELF_MASK].img_checksum = SELF_MASK_IMG_CHECKSUM;
	make_mass_self_display_img_cmds(vdd, FLAG_SELF_MASK, &vdd->self_disp.self_mask_cmd);

	if (vdd->is_factory_mode) {
		vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_buf = self_mask_img_crc_data;
		vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_size = ARRAY_SIZE(self_mask_img_crc_data);
		vdd->self_disp.operation[FLAG_SELF_MASK_CRC].img_checksum = SELF_MASK_IMG_CHECKSUM;
		make_mass_self_display_img_cmds(vdd, FLAG_SELF_MASK_CRC, &vdd->self_disp.self_mask_crc_cmd);
	}

	return 1;
}

static int ss_mafpc_data_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	LCD_INFO(vdd, "mAFPC Panel Data init\n");

	vdd->mafpc.img_buf = mafpc_img_data;
	vdd->mafpc.img_size = ARRAY_SIZE(mafpc_img_data);

	if (!vdd->mafpc.make_img_mass_cmds) {
		LCD_ERR(vdd, "no make_img_mass_cmds callback\n");
		return -EINVAL;
	}

	vdd->mafpc.make_img_mass_cmds(vdd, vdd->mafpc.img_buf,
			vdd->mafpc.img_size, &vdd->mafpc_img_cmd);

	if (vdd->is_factory_mode) {
		/* CRC Check For Factory Mode */
		vdd->mafpc.crc_img_buf = mafpc_img_data_crc_check;
		vdd->mafpc.crc_img_size = ARRAY_SIZE(mafpc_img_data_crc_check);

		if (!vdd->mafpc.make_img_mass_cmds) {
			LCD_ERR(vdd, "Can not make mafpc image commands\n");
			return -EINVAL;
		}

		vdd->mafpc.make_img_mass_cmds(vdd, vdd->mafpc.crc_img_buf,
				vdd->mafpc.crc_img_size, &vdd->mafpc_crc_img_cmd);
	}

	return 0;
}
#endif

static int ss_vrr_init(struct vrr_info *vrr)
{
	struct lfd_mngr *mngr;
	int i, scope;
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

	/* LFD mode */
	for (i = 0, mngr = &vrr->lfd.lfd_mngr[i]; i < LFD_CLIENT_MAX; i++, mngr++) {
		for (scope = 0; scope < LFD_SCOPE_MAX; scope++) {
			mngr->fix[scope] = LFD_FUNC_FIX_OFF;
			mngr->scalability[scope] = LFD_FUNC_SCALABILITY0;
			mngr->min[scope] = LFD_FUNC_MIN_CLEAR;
			mngr->max[scope] = LFD_FUNC_MAX_CLEAR;
		}
	}

#if IS_ENABLED(CONFIG_SEC_FACTORY)
	mngr = &vrr->lfd.lfd_mngr[LFD_CLIENT_FAC];
	mngr->fix[LFD_SCOPE_NORMAL] = LFD_FUNC_FIX_HIGH;
	mngr->fix[LFD_SCOPE_LPM] = LFD_FUNC_FIX_HIGH;
	mngr->fix[LFD_SCOPE_HMD] = LFD_FUNC_FIX_HIGH;
#endif

	LCD_INFO(vdd, "---\n");
	return 0;
}

static bool ss_check_support_mode(struct samsung_display_driver_data *vdd, enum CHECK_SUPPORT_MODE mode)
{
	bool is_support = true;
	int cur_rr = vdd->vrr.cur_refresh_rate;
	bool cur_hs = vdd->vrr.cur_sot_hs_mode;

	switch (mode) {

	case CHECK_SUPPORT_BRIGHTDOT:
		if (!(cur_rr == 120 && cur_hs)) {
			is_support = false;
			LCD_ERR(vdd, "BRIGHT DOT fail: supported on 120HS(cur: %d%s)\n",
					cur_rr, cur_hs ? "HS" : "NS");
		}
		break;

	default:
		break;
	}

	return is_support;
}

#if 0
static void ss_read_flash(struct samsung_display_driver_data *vdd, u32 raddr, u32 rsize, u8 *rbuf)
{
	char showbuf[256];
	int i, pos = 0;

	if (rsize > 0x100) {
		LCD_ERR(vdd, "rsize(%x) is not supported..\n", rsize);
		return;
	}

	ss_send_cmd(vdd, TX_POC_ENABLE);
	spsram_read_bytes(vdd, raddr, rsize, rbuf);
	ss_send_cmd(vdd, TX_POC_DISABLE);

	for (i = 0; i < rsize; i++)
		pos += scnprintf(showbuf + pos, 256 - pos, "%02x ", rbuf[i]);
	LCD_INFO(vdd, "buf : %s\n", showbuf);

	return;
}
#endif

static int ss_read_udc_transmittance_data(struct samsung_display_driver_data *vdd)
{
	int ret, i = 0;
	u8 *read_buf;

	read_buf = kzalloc(vdd->udc.size, GFP_KERNEL);
	if (!read_buf) {
		LCD_ERR(vdd, "fail to kzalloc for udc read_buf\n");
		return 0;
	}

	ret = ss_send_cmd_get_rx(vdd, RX_UDC_DATA, read_buf);
	if (ret <= 0) {
		LCD_ERR(vdd, "fail to read udc data..\n");
		goto err;
	}

	memcpy(vdd->udc.data, read_buf, vdd->udc.size);

	for (i = 0; i < vdd->udc.size/2; i++)
		LCD_INFO(vdd, "[%d] %X.%X\n", i, vdd->udc.data[(i*2)+0], vdd->udc.data[(i*2)+1]);

	vdd->udc.read_done = true;
err:
	kfree(read_buf);
	return 0;
}


#if 0 // Glut related
static bool ss_check_night_dim_enable(struct samsung_display_driver_data *vdd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int bl_idx = state->bl_idx;

	if (vdd->night_dim && (bl_idx < vdd->night_dim_max_lv))
		return true;
	else
		return false;
}

static int update_glut_enable(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int cur_rr = state->cur_refresh_rate;
	int bl_idx = state->bl_idx;
	int bl_lvl = state->bl_level;
	int i = -1;
	bool glut_enable = true;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	if (i + 1 > cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position(%d, %d)\n",
				i, cmd->tx_len);
		goto err_skip;
	}

	if (vdd->br_info.glut_skip) {
		LCD_ERR(vdd, "skip GLUT\n");
		goto err_skip;
	}

	if ((cur_rr == 48 || cur_rr == 96) || ss_check_night_dim_enable(vdd))
		glut_enable = true;
	else
		glut_enable = false;

	cmd->txbuf[i] = glut_enable ? 0x01 : 0x00;

	LCD_DEBUG(vdd, "enable[%d], bl_idx: %d, bl_lvl: %d, cur_rr: %d, i: %d, night_dim : %d / night_dim_max_lv %d\n",
		glut_enable, bl_idx, bl_lvl, cur_rr, i, vdd->night_dim, vdd->night_dim_max_lv);

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return -EINVAL;
}

static int update_glut_offset(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int cur_rr = state->cur_refresh_rate;
	int bl_idx = state->bl_idx;
	int bl_lvl = state->bl_level;
	struct cmd_legoop_map *glut_map = NULL;
	int i = -1, j;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	if (ss_check_night_dim_enable(vdd)) {
		if (cur_rr == 96 || cur_rr == 48)
			glut_map = &vdd->br_info.glut_offset_night_dim_96hs;
		else
			glut_map = &vdd->br_info.glut_offset_night_dim_120hs;
	} else if (cur_rr == 96 || cur_rr == 48) {
		glut_map = &vdd->br_info.glut_offset_96hs;
	} else {
		LCD_DEBUG(vdd, "Skip glut offset cmd\n");
		goto err_skip;
	}

	if (i + glut_map->col_size > cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position(%d, %d, %d)\n",
				i, glut_map->col_size, cmd->tx_len);
		goto err_skip;
	}

	if (vdd->br_info.glut_skip) {
		LCD_ERR(vdd, "skip GLUT\n");
		goto err_skip;
	}

	if (bl_idx > glut_map->row_size) {
		LCD_DEBUG(vdd, "invalid bl_idx [%d] / [%d]\n", bl_idx, glut_map->row_size);
		goto err_skip;
	}

	if (vdd->br_info.glut_00_val) {
		memset(&cmd->txbuf[i], 0x00, glut_map->col_size);
	} else {
		for (j = 0; j < glut_map->col_size; j++)
			cmd->txbuf[i + j] = glut_map->cmds[bl_idx][j];
	}

	LCD_DEBUG(vdd, "bl_idx: %d, bl_lvl: %d, cur_rr: %d, i: %d col_size : %d, night_dim : %d\n",
		bl_idx, bl_lvl, cur_rr, i, glut_map->col_size, vdd->night_dim);

	return 0;

err_skip:
	cmd->skip_by_cond = true;
	return -EINVAL;
}

static void parse_glut(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	struct device_node *np;

	np = ss_get_panel_of(vdd);
	if (!np) {
		LCD_ERR(vdd, "No panel np..\n");
		return;
	}

	/* GLUT table for 48/96HS */
	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_96hs, sizeof(struct cmd_legoop_map),
				"samsung,glut_offset_96hs_table_rev", 0, ss_parse_panel_legoop_table);
	if (ret)
		LCD_ERR(vdd, "Failed to parse GLUT_OFFSET_96HS data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_NIGHT_DIM data from panel_data_file [%d][%d]\n",
			vdd->br_info.glut_offset_96hs.row_size, vdd->br_info.glut_offset_96hs.col_size);

	/* GLUT table for night_dim 96/120 */
	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_night_dim_96hs, sizeof(struct cmd_legoop_map),
				"samsung,glut_offset_night_dim_96hs_table_rev", 0, ss_parse_panel_legoop_table);
	if (ret)
		LCD_ERR(vdd, "Failed to parse glut_offset_night_dim_96hs data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_NIGHT_DIM_96HS data from panel_data_file [%d][%d]\n",
			vdd->br_info.glut_offset_night_dim_96hs.row_size, vdd->br_info.glut_offset_night_dim_96hs.col_size);

	vdd->night_dim_max_lv = vdd->br_info.glut_offset_night_dim_96hs.row_size;

	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_night_dim_120hs, sizeof(struct cmd_legoop_map),
				"samsung,glut_offset_night_dim_120hs_table_rev", 0, ss_parse_panel_legoop_table);
	if (ret)
		LCD_ERR(vdd, "Failed to parse glut_offset_night_dim_120hs data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_NIGHT_DIM_120HS data from panel_data_file [%d][%d]\n",
			vdd->br_info.glut_offset_night_dim_120hs.row_size, vdd->br_info.glut_offset_night_dim_120hs.col_size);

	vdd->night_dim_max_lv = vdd->br_info.glut_offset_night_dim_120hs.row_size;
}

static int update_manual_aor(struct samsung_display_driver_data *vdd,
			char *val, struct ss_cmd_desc *cmd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int cur_rr = state->cur_refresh_rate;
	int bl_idx = state->bl_idx;
	int bl_lvl = state->bl_level;
	struct cmd_legoop_map *manual_aor_map = NULL;
	int i = -1;

	while (!cmd->pos_0xXX[++i] && i < cmd->tx_len);

	if (i + 1 >= cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position\n");
		return -EINVAL;
	}

	if (cur_rr == 96 || cur_rr == 48) {
		manual_aor_map = &vdd->br_info.manual_aor_96hs[vdd->panel_revision];
	} else {
		// Don't need to be updated now.
		//manual_aor_map = &vdd->br_info.manual_aor_120hs[vdd->panel_revision];
	}

	if (manual_aor_map) {
		if (bl_idx > manual_aor_map->row_size) {
			LCD_ERR(vdd, "invalid bl [%d]/[%d]\n", bl_idx, manual_aor_map->row_size);
			return 0;
		}

		cmd->txbuf[i] = manual_aor_map->cmds[bl_idx][0];
		cmd->txbuf[i + 1] = manual_aor_map->cmds[bl_idx][1];

		LCD_DEBUG(vdd, "bl_idx: %d, bl_lvl: %d, i: %d, aor: 0x%X%X, tx_len: %d\n",
				bl_idx, bl_lvl, i,
				manual_aor_map->cmds[bl_idx][0], manual_aor_map->cmds[bl_idx][1], cmd->tx_len);

		return 0;
	} else {
		LCD_INFO(vdd, "No manual_aor_map\n");
		goto err_skip;
	}

err_skip:
	cmd->skip_by_cond = true;
	return -EINVAL;
}
#endif

void Q7_S6E3XA5_AMF800GX01_QXGA_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "%s\n", ss_get_panel_name(vdd));

	vdd->panel_state = PANEL_PWR_OFF;	/* default OFF */

	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;

	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_module_info_read = ss_module_info_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;

	/* Brightness */
//	vdd->panel_func.pre_brightness = ss_pre_brightness;
	vdd->panel_func.pre_lpm_brightness = ss_pre_lpm_brightness;

	vdd->br_info.acl_status = 1; /* ACL default ON */
	vdd->br_info.gradual_acl_val = 1; /* ACL default status in acl on */
	vdd->br_info.temperature = 20;

	/* mdnie */
	vdd->mdnie.support_mdnie = false;
	vdd->mdnie.support_trans_dimming = false;
	vdd->mdnie.mdnie_tune_size[MDNIE_STEP1] = sizeof(BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[MDNIE_STEP2] = sizeof(BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[MDNIE_STEP3] = sizeof(BYPASS_MDNIE_3);
	vdd->panel_func.set_vividness = mdnie_set_vividness;
	dsi_update_mdnie_data(vdd);

	vdd->self_disp.factory_support = false;
	//vdd->self_disp.init = self_display_init;
	//vdd->self_disp.data_init = ss_self_display_data_init;

	//vdd->mafpc.init = ss_mafpc_init;
	//vdd->mafpc.data_init = ss_mafpc_data_init;

	/* UDC */
	vdd->panel_func.read_udc_data = ss_read_udc_transmittance_data;

	/* VRR */
	vdd->panel_func.samsung_lfd_get_base_val = ss_update_base_lfd_val;
	ss_vrr_init(&vdd->vrr);

	/* early te */
	vdd->early_te = false;
	vdd->check_early_te = 0;

	vdd->panel_func.samsung_check_support_mode = ss_check_support_mode;

	/* change  MIPI Drive strength values for this panel - request by HW group */
	vdd->motto_info.motto_swing = 0xEE;
	vdd->motto_info.vreg_ctrl_0 = 0x46;

	/* Below data will be genarated by script in Kbuild file */
	vdd->h_buf = Q7_S6E3XA5_AMF800GX01_PDF_DATA;
	vdd->h_size = sizeof(Q7_S6E3XA5_AMF800GX01_PDF_DATA);

	/* Get f_buf from header file data to cover recovery mode
	 * Below code should be called before any PDF parsing code such as update_glut_map
	 */
	if (!vdd->file_loading && vdd->h_buf) {
		LCD_INFO(vdd, "Get f_buf from header file data(%lu)\n", vdd->h_size);
		vdd->f_buf = vdd->h_buf;
		vdd->f_size = vdd->h_size;
	}

	//register_op_sym_cb(vdd, "GLUT", update_glut_offset, true);
	//register_op_sym_cb(vdd, "GLUT_ENABLE", update_glut_enable, true);
	//register_op_sym_cb(vdd, "AOR", update_manual_aor, true);
	//parse_glut(vdd);
}
