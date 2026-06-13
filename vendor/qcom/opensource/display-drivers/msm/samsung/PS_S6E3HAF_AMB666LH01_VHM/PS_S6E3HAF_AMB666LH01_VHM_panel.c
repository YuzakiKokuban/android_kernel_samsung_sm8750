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
#include "PS_S6E3HAF_AMB666LH01_VHM_panel.h"
#include "PS_S6E3HAF_AMB666LH01_VHM_mdnie.h"
#include "ss_panel_power.h"

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
		vdd->panel_revision = 'B';
		break;
	case 0x02:
		vdd->panel_revision = 'C';
		break;
	case 0x03:
		vdd->panel_revision = 'D';
		break;
	case 0x04:
		vdd->panel_revision = 'E';
		break;
	case 0x05:
		vdd->panel_revision = 'F';
		break;
	case 0x06:
		vdd->panel_revision = 'G';
		break;
	case 0x07:
		vdd->panel_revision = 'H';
		break;
	case 0x08:
		vdd->panel_revision = 'I';
		break;
	case 0x09:
		vdd->panel_revision = 'J';
		break;
	default:
		vdd->panel_revision = 'J';
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
		LCD_ERR(vdd, ": Invalid data vdd : 0x%zx", (size_t)vdd);
		return false;
	}

	LCD_INFO(vdd, "+: ndx=%d\n", vdd->ndx);

	ss_panel_attach_set(vdd, true);

	return 0;
}

static int samsung_panel_on_post(struct samsung_display_driver_data *vdd)
{
	int ret = 0;

	if (vdd->self_disp.is_support) {
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
		else
			LCD_ERR(vdd, "Self Mask On Fail\n");
	}
#if 0
	/* mafpc */
	if (vdd->mafpc.is_support && vdd->samsung_splash_enabled) {
		vdd->mafpc.need_to_write = true;
		LCD_INFO(vdd, "Need to write mafpc image data to DDI\n");
	}
#endif
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
	VRR_10HS = 0,
	VRR_24HS,
	VRR_30HS,
	VRR_48HS,
	VRR_60HS,
	VRR_80HS,
	VRR_120HS,
	VRR_MAX
};

static enum VRR_CMD_RR ss_get_vrr_id(struct samsung_display_driver_data *vdd)
{
	enum VRR_CMD_RR vrr_id;
	int cur_rr = vdd->vrr.cur_refresh_rate;
	int cur_hs = vdd->vrr.cur_sot_hs_mode;

	switch (cur_rr) {
	case 10:
		vrr_id = VRR_10HS;
		break;
	case 24:
		vrr_id = VRR_24HS;
		break;
	case 30:
		vrr_id = VRR_30HS;
		break;
	case 48:
		vrr_id = VRR_48HS;
		break;
	case 60:
		vrr_id = VRR_60HS;
		break;
	case 80:
		vrr_id = VRR_80HS;
		break;
	case 120:
		vrr_id = VRR_120HS;
		break;
	default:
		LCD_ERR(vdd, "invalid refresh rate (%d, %d), set default 120HS..\n", cur_rr, cur_hs);
		vrr_id = VRR_120HS;
		break;
	}

	return vrr_id;
}

static int ss_update_base_lfd_val(struct vrr_info *vrr,
			enum LFD_SCOPE_ID scope, struct lfd_base_str *lfd_base)
{
	u32 base_rr, max_div_def, min_div_def, min_div_lowest;
	enum VRR_CMD_RR vrr_id;
	struct samsung_display_driver_data *vdd =
		container_of(vrr, struct samsung_display_driver_data, vrr);
	struct lfd_mngr *mngr;

	/* AOD : Max 30Hz / Min 1Hz */
	if (scope == LFD_SCOPE_LPM) {
		base_rr = 30;
		max_div_def = 1;	/* 30/1 = 30Hz */
		min_div_def = 30;	/* 30/30 = 1Hz */
		min_div_lowest = 30;	/* 30/30 = 1Hz */
		goto done;
	}

	mngr = &vrr->lfd.lfd_mngr[LFD_CLIENT_DISP];

	vrr_id = ss_get_vrr_id(vdd);

	switch (vrr_id) {
	case VRR_10HS:
		base_rr = 240;
		max_div_def = 24; /* 240/24 = 10hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	case VRR_24HS:
		base_rr = 240;
		max_div_def = 10; /* 240/10 = 24hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	case VRR_30HS:
		base_rr = 240;
		max_div_def = 8; /* 240/8 = 30hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	case VRR_48HS:
		base_rr = 240;
		max_div_def = 5; /* 240/5 = 48hz */
		min_div_def = 10; /* 24hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	case VRR_60HS:
		base_rr = 240;
		max_div_def = 4; /* 240/4 = 60hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	case VRR_80HS:
		base_rr = 240;
		max_div_def = 3; /* 240/3 = 80hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	case VRR_120HS:
		base_rr = 240;
		max_div_def = 2; /* 240/2 = 120hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	default:
		LCD_ERR(vdd, "invalid vrr_id\n");
		base_rr = 240;
		max_div_def = 2; /* 120hz */
		min_div_def = 24; /* 10hz */
		min_div_lowest = 240; /* 1Hz for Scalability 6 */
		break;
	}

	/* 1Hz for scalability 6 */
	if (mngr->scalability[LFD_SCOPE_NORMAL] == LFD_FUNC_SCALABILITY6 ||
		mngr->scalability[LFD_SCOPE_LPM] == LFD_FUNC_SCALABILITY6)
		min_div_def = min_div_lowest;

done:
	lfd_base->base_rr = base_rr;
	lfd_base->max_div_def = max_div_def;
	lfd_base->min_div_def = min_div_def;
	lfd_base->min_div_lowest = min_div_lowest;
	lfd_base->fix_div_def = 2; // LFD MAX/MIN 120hz fix
	lfd_base->highdot_div_def = 240 * 2; // 240 / 480 = 0.5hz for highdot test (240EM base)
	lfd_base->fix_low_div_def = base_rr; // 1Hz for LFD_FUNC_FIX_LOW(2)

	vrr->lfd.base_rr = base_rr;

	LCD_DEBUG(vdd, "LFD(%s): base_rr: %uhz, def: %uhz(%u)~%uhz(%u), lowest: %uhz(%u), highdot_div: %u\n",
			lfd_scope_name[scope], base_rr,
			DIV_ROUND_UP(base_rr, min_div_def), min_div_def,
			DIV_ROUND_UP(base_rr, max_div_def), max_div_def,
			DIV_ROUND_UP(base_rr, min_div_lowest), min_div_lowest,
			lfd_base->highdot_div_def);

	return 0;
}

#define COORDINATE_DATA_SIZE 6
#define MDNIE_SCR_WR_ADDR	0x32
#define RGB_INDEX_SIZE 4
#define COEFFICIENT_DATA_SIZE 8

#define F1(x, y) (((y << 10) - (((x << 10) * 56) / 55) - (102 << 10)) >> 10)
#define F2(x, y) (((y << 10) + (((x << 10) * 5) / 1) - (18483 << 10)) >> 10)

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

static char coordinate_data_2[][COORDINATE_DATA_SIZE] = {
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* dummy */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_2 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_5 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_6 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_7 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_8 */
	{0xff, 0x00, 0xfb, 0x00, 0xf4, 0x00}, /* Tune_9 */
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

	result = (coefficient[index][0] * x) + (coefficient[index][1] * y) +
			(((coefficient[index][2] * x + 512) >> 10) * y) +
			(coefficient[index][3] * 10000);

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

	result = (coefficient[index][4] * x) + (coefficient[index][5] * y) +
			(((coefficient[index][6] * x + 512) >> 10) * y) +
			(coefficient[index][7] * 10000);

	result = (result + 512) >> 10;

	if (result < 0)
		result = 0;
	if (result > 1024)
		result = 1024;

	return result;
}

/* DFh */
static u8 vividness_skin_tone_table[VIVIDNESS_MAX_IDX][VIVIDNESS_SKIN_TONE_SIZE] = {
	{0xFF, 0x20, 0x40, 0xFF, 0xFD, 0x20, 0xFF, 0x00, 0xF5, 0xFF, 0xFF, 0xFB}, /* default */
	{0xF7, 0x20, 0x40, 0xF7, 0xFC, 0x30, 0xFF, 0x08, 0xFA, 0xFF, 0xFF, 0xFD},
	{0xEF, 0x20, 0x40, 0xF0, 0xFC, 0x40, 0xFF, 0x10, 0xFF, 0xFF, 0xFF, 0xFF},
};

/* DEh */
static u8 vividness_3x3_matrix_table[VIVIDNESS_MAX_IDX][VIVIDNESS_3x3_MATRIX_SIZE] = {
	{0x04, 0x3A, 0x1F, 0xCE, 0x1F, 0xF7, 0x1F, 0xFF, 0x04, 0x08, 0x1F, 0xF7, 0x1F, 0xFF, 0x1F, 0xCE, 0x04, 0x31}, /* default */
	{0x04, 0x6D, 0x1F, 0x9D, 0x1F, 0xF4, 0x1F, 0xE9, 0x04, 0x21, 0x1F, 0xF4, 0x1F, 0xE9, 0x1F, 0x9D, 0x04, 0x77},
	{0x04, 0xA1, 0x1F, 0x6D, 0x1F, 0xF1, 0x1F, 0xD4, 0x04, 0x3A, 0x1F, 0xF1, 0x1F, 0xD4, 0x1F, 0x6D, 0x04, 0xBE},
};

/* DEh for night mode */
static u8 vividness_3x3_matrix_night_mode_table[VIVIDNESS_MAX_IDX][VIVIDNESS_3x3_MATRIX_SIZE] = {
	{0x04, 0x3A, 0x1F, 0xCE, 0x1F, 0xF7, 0x1F, 0xFF, 0x04, 0x08, 0x1F, 0xF7, 0x1F, 0xFF, 0x1F, 0xCE, 0x04, 0x31},
	{0x04, 0x6D, 0x1F, 0x9D, 0x1F, 0xF4, 0x1F, 0xE9, 0x04, 0x21, 0x1F, 0xF4, 0x1F, 0xE9, 0x1F, 0x9D, 0x04, 0x77},
	{0x04, 0xA1, 0x1F, 0x6D, 0x1F, 0xF1, 0x1F, 0xD4, 0x04, 0x3A, 0x1F, 0xF1, 0x1F, 0xD4, 0x1F, 0x6D, 0x04, 0xBE},
};

static void mdnie_set_vividness(struct samsung_display_driver_data *vdd, struct mdnie_lite_tun_type *tune, struct dsi_cmd_desc *tune_data)
{
	struct mdnie_lite_tune_data *mdnie_data = vdd->mdnie.mdnie_data;
	int idx = tune->vividness_idx;

	/* Do not apply (color lens, natural mode) */
	if (tune->color_lens_enable || tune->mdnie_mode == DYNAMIC_MODE)
		return;

	if (idx >= 0 && idx < VIVIDNESS_MAX_IDX) {
		/* DFh */
		if (tune->night_mode_enable == false) {
			memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP1]].ss_txbuf[VIVIDNESS_SKIN_TONE_IDX],
				vividness_skin_tone_table[idx], VIVIDNESS_SKIN_TONE_SIZE);
		}

		/* DEh */
		if (tune->night_mode_enable) {
			memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP2]].ss_txbuf[VIVIDNESS_3x3_MATRIX_IDX],
				vividness_3x3_matrix_night_mode_table[idx], VIVIDNESS_3x3_MATRIX_SIZE);
		} else {
			memcpy(&tune_data[mdnie_data->mdnie_step_index[MDNIE_STEP2]].ss_txbuf[VIVIDNESS_3x3_MATRIX_IDX],
				vividness_3x3_matrix_table[idx], VIVIDNESS_3x3_MATRIX_SIZE);
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
	int ret = -1;

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
	len += sprintf(temp + len, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
			read_buf[4], read_buf[5], read_buf[6], read_buf[7],
			read_buf[8], read_buf[9], read_buf[10], read_buf[0],
			read_buf[1], read_buf[2], read_buf[3]);

	/* compare cell_id from cmdline and read here */
	if (vdd->cell_id_len > 0) {
		ret = strncmp(temp, vdd->cell_id_dsi, len-1);
		if (!ret) {
			LCD_INFO(vdd, "cell_id both bootloader & kernel match\n");
		} else {
			LCD_INFO(vdd, "cell_id is not match with bootloader's\n");
		}
	}

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

static int ss_self_display_data_init(struct samsung_display_driver_data *vdd)
{
	if (IS_ERR_OR_NULL(vdd)) {
		LCD_ERR(vdd, "vdd is null or error\n");
		return -ENODEV;
	}

	if (!vdd->self_disp.is_support) {
		LCD_ERR(vdd, "Self Display is not supported\n");
		return -EINVAL;
	}

	LCD_INFO(vdd, "Self Display Panel Data init\n");

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

	return 0;
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

static int ss_post_vrr(struct samsung_display_driver_data *vdd,
			int old_rr, bool old_hs, bool old_phs,
			int new_rr, bool new_hs, bool new_phs)
{
	if (new_rr == 120 || new_rr == 96)
		vdd->dbg_tear_info.early_te_delay_us = 2000; /* 2ms */
	else
		vdd->dbg_tear_info.early_te_delay_us = 0;

	return 0;
}

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

/* night_dim 0 - Vivid + E/C Off (glut disable)
 * night_dim 1 - Natural + E/C Off (120 glut offset)
 * night_dim 2 - Vivid + E/C On (glut offset[0] for <= 30nit, glut disable for > 30nit)
 * night_dim 3 - Natural + E/C On (glut offset[0] for <= 30nit, glut offset[1] for > 30nit)
 */
static bool ss_check_night_dim_enable(struct samsung_display_driver_data *vdd)
{
	struct cmd_ref_state *state = &vdd->cmd_ref_state;
	int bl_idx = state->bl_idx;
	bool ret = true;

	if (vdd->night_dim == 0) {
		ret = false;
	} else if (vdd->night_dim == 1) {
		if (bl_idx >= vdd->night_dim_max_lv)
			ret = false;
	}

	return ret;
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
		LCD_ERR(vdd, "fail to find proper 0xXX position(%d, %lu)\n",
				i, cmd->tx_len);
		goto err_skip;
	}

	if (vdd->br_info.glut_skip) {
		LCD_ERR(vdd, "skip GLUT\n");
		goto err_skip;
	}

	if (ss_check_night_dim_enable(vdd)) /* night_dim */
		glut_enable = true;
	else
		glut_enable = false;

	cmd->txbuf[i] = glut_enable ? 0x01 : 0x00;

	LCD_ERR(vdd, "enable[%d], bl_idx: %d, bl_lvl: %d, cur_rr: %d, i: %d, night_dim : %d / night_dim_max_lv %d\n",
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

	if (!vdd->night_dim)
		goto err_skip;

	if (vdd->night_dim == 2) {
		glut_map = &vdd->br_info.glut_offset_120hs;
	} else if (vdd->night_dim == 1) {
		if (bl_idx < vdd->night_dim_max_lv)
			glut_map = &vdd->br_info.glut_offset_night_dim[1];
		else
			glut_map = NULL;
	} else if (vdd->night_dim == 3) {
		if (bl_idx < vdd->night_dim_max_lv)
			glut_map = &vdd->br_info.glut_offset_night_dim[0];
		else
			glut_map = &vdd->br_info.glut_offset_120hs;
	} else {
		LCD_ERR(vdd, "invalid night_dim option.. %d, %d\n", vdd->night_dim, bl_idx);
		glut_map = NULL;
	}

	if (glut_map == NULL) {
		LCD_ERR(vdd, "No glut_map %d, %d\n", vdd->night_dim, bl_idx);
		goto err_skip;
	}

	if (i + glut_map->col_size > (int)cmd->tx_len) {
		LCD_ERR(vdd, "fail to find proper 0xXX position(%d, %d, %zu)\n",
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

	LCD_ERR(vdd, "bl_idx: %d, bl_lvl: %d, cur_rr: %d, i: %d col_size : %d, night_dim : %d\n",
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

	/* GLUT table for normal */
	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_120hs, sizeof(struct cmd_legoop_map),
				"samsung,glut_offset_120hs_table_rev", 0, ss_parse_panel_legoop_table);
	if (ret)
		LCD_ERR(vdd, "Failed to parse GLUT_OFFSET_120HS data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_NIGHT_DIM data from panel_data_file [%d][%d]\n",
			vdd->br_info.glut_offset_120hs.row_size, vdd->br_info.glut_offset_120hs.col_size);

	/* GLUT table for night_dim */
	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_night_dim[0], sizeof(struct cmd_legoop_map),
				"samsung,glut_offset_night_dim_table1_rev", 0, ss_parse_panel_legoop_table);
	if (ret)
		LCD_ERR(vdd, "Failed to parse GLUT_OFFSET_night_dim 1 data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_NIGHT_DIM1 data from panel_data_file [%d][%d]\n",
			vdd->br_info.glut_offset_night_dim[0].row_size, vdd->br_info.glut_offset_night_dim[0].col_size);

	vdd->night_dim_max_lv = vdd->br_info.glut_offset_night_dim[0].row_size;

	ret = parse_dt_data(vdd, np, &vdd->br_info.glut_offset_night_dim[1], sizeof(struct cmd_legoop_map),
				"samsung,glut_offset_night_dim_table2_rev", 0, ss_parse_panel_legoop_table);
	if (ret)
		LCD_ERR(vdd, "Failed to parse GLUT_OFFSET_night_dim 2 data\n");
	else
		LCD_INFO(vdd, "Parsing GLUT_OFFSET_NIGHT_DIM2 data from panel_data_file [%d][%d]\n",
			vdd->br_info.glut_offset_night_dim[1].row_size, vdd->br_info.glut_offset_night_dim[1].col_size);

	vdd->night_dim_max_lv = vdd->br_info.glut_offset_night_dim[1].row_size;
}

static int ss_panel_power_ctrl_specific(struct samsung_display_driver_data *vdd, struct pwr_node *pwr, bool enable)
{
	int num_of_v = 0;
	int rc = -EINVAL;
	int i;
	int min_voltage, len = 0;
	char pBuf[256];

	memset(pBuf, 0x00, 256);

	len += snprintf(pBuf + len, 256 - len, "[%s]", pwr->name);

	for (i = 0; i < pwr->pwr_seq_count; i++) {
		if (pwr->pwr_seq[i].onoff)
			rc = regulator_enable(pwr->reg);
		else
			rc = regulator_disable(pwr->reg);

		if (rc) {
			LCD_ERR(vdd, "[%s] fail to %s, rc=%d\n", pwr->name,
					pwr->pwr_seq[i].onoff ? "enable" : "disable",
					rc);
			return rc;
		} else {
			LCD_INFO(vdd, "[%s] %s \n", pwr->name,
					pwr->pwr_seq[i].onoff ? "enable" : "disable");
		}

		len += snprintf(pBuf + len, 256 - len, " / %s (delay: %dms)",
				pwr->pwr_seq[i].onoff ? "enable" : "disable",
				pwr->pwr_seq[i].post_delay);

		/* regulator on -> wait 2ms : need 470us more to DUMP OTP of S2DOS07 */
		usleep_range(2000, 2100);
	}

	if (pwr->load >= 0) {
		len += snprintf(pBuf + len, 256 - len, " / load: %d", pwr->load);
		rc = regulator_set_load(pwr->reg, pwr->load);
		if (rc) {
			LCD_ERR(vdd, "fail to set optimum mode(%s), rc: %d\n",
				   pwr->name, rc);
			return rc;
		}
	}

	if (pwr->current_uA >= 0) {
		len += snprintf(pBuf + len, 256 - len, " / current limit: %d", pwr->current_uA);

#if IS_ENABLED(CONFIG_SEC_PM)
		rc = regulator_set_current_limit(pwr->reg, pwr->current_uA, pwr->current_uA);
		if (rc) {
			LCD_ERR(vdd, "[%s] fail to set current limit(%duA)\n",
					pwr->name, pwr->current_uA);
			return rc;
		}
#else
		LCD_ERR(vdd, "CONFG_SEC_PM is not Enabled\n");
#endif
	}

	min_voltage = enable ? pwr->voltage : 0;
	len += snprintf(pBuf + len, 256 - len, " / set voltage: %duV - %duV", min_voltage, pwr->voltage);

	num_of_v = regulator_count_voltages(pwr->reg);
	if (num_of_v > 0) {
		rc = regulator_set_voltage(pwr->reg, min_voltage, pwr->voltage);
		if (rc) {
			LCD_ERR(vdd, "fail to set voltage(%s), rc=%d\n",
					pwr->name, rc);
			return rc;
		} else {
			LCD_INFO(vdd, "[%s] set voltage: %duV - %duV \n", pwr->name,
					min_voltage, pwr->voltage);
		}
	} else {
		LCD_ERR(vdd, "[%s] num_of_v is %d, do not set_voltage..\n", pwr->name, num_of_v);
	}

	LCD_INFO(vdd, "%s\n", pBuf);

	return 0;
}

static int ss_ddi_flash_test(struct samsung_display_driver_data *vdd, char* buf)
{
	u8 *val;
	int i, len = 0;
	int res = 0;
	struct ddi_test_mode *test_mode = &vdd->ddi_test[DDI_TEST_FLASH_TEST];

	if (test_mode->pass_val) {
		val = kzalloc(test_mode->pass_val_size, GFP_KERNEL);
		if (val) {
			if (ss_send_cmd_get_rx(vdd, RX_FLASH_LOADING_CHECK, val) <= 0) {
				LCD_ERR(vdd, "fail to rx_ssr\n");
			} else {
				if (val[0] == test_mode->pass_val[0] &&
					(val[1] & ~(test_mode->pass_val[1])) == 0)
					res = true;

				len += sprintf(buf + len, "%d\n", 1);
				len += sprintf(buf + len, "%d ", res == true ? 1 : 0);
				for (i = 0; i < test_mode->pass_val_size; i++) {
					len += sprintf(buf + len, "%02X", val[i]);
				}
				len += sprintf(buf + len, "\n");
			}

			kfree(val);
		} else {
			LCD_ERR(vdd, "fail to alloc read buffer ..\n");
		}
	} else {
		LCD_ERR(vdd, "pass_val is NULL\n");
	}

	LCD_INFO(vdd, "Flash Test is done, result=%s\n", res ? "OK" : "NG");

	return strlen(buf);
}

static int ss_check_flash_done(struct samsung_display_driver_data *vdd)
{
	int ret = 0;
	int read_len = 0;
	u8 val[2] = {0x00, };
	struct ddi_test_mode *test_mode = &vdd->ddi_test[DDI_TEST_FLASH_TEST];

	read_len = ss_send_cmd_get_rx(vdd, RX_FLASH_LOADING_CHECK, val);
	if (read_len <= 0) {
		LCD_ERR(vdd, "fail to read flash loading check(read_len=%d)\n", read_len);
		return -EFAULT;
	}

	if (val[0] == test_mode->pass_val[0] &&
		(val[1] & ~(test_mode->pass_val[1])) == 0)
		LCD_INFO(vdd, "FLASH GOOD\n");
	else {
		LCD_INFO(vdd, "FLASH NOT GOOD [0x%x 0x%x]\n", val[0], val[1]);
		//vdd->need_panel_recovery = true;
		//inc_dpui_u32_field(DPUI_KEY_FLASH_LOAD, 1);
	}

	return ret;
}

static void ss_pre_mdnie(struct samsung_display_driver_data *vdd, struct dsi_cmd_desc *tune_data)
{
	if ((tune_data[MDNIE_STEP3_INDEX].ss_txbuf[3] & 0x01)
		|| (tune_data[MDNIE_STEP3_INDEX].ss_txbuf[4] & 0x04)) {
		if (!vdd->mdnie.aolce_on) {
			LCD_INFO(vdd, "AOLCE ON! / 0x%02X 0x%02X \n" ,
				tune_data[MDNIE_STEP3_INDEX].ss_txbuf[3], tune_data[MDNIE_STEP3_INDEX].ss_txbuf[4]);
			ss_panel_power_on(vdd, &vdd->panel_powers[PANEL_POWERS_AOLCE_ON]);
			vdd->mdnie.aolce_on = true;
		}
	} else {
		if (vdd->mdnie.aolce_on) {
			LCD_INFO(vdd, "AOLCE OFF! / 0x%02X 0x%02X \n" ,
				tune_data[MDNIE_STEP3_INDEX].ss_txbuf[3], tune_data[MDNIE_STEP3_INDEX].ss_txbuf[4]);
			ss_panel_power_on(vdd, &vdd->panel_powers[PANEL_POWERS_AOLCE_OFF]);
			vdd->mdnie.aolce_on = false;
		}
	}
}

void PS_S6E3HAF_AMB666LH01_VHM_WQHD_init(struct samsung_display_driver_data *vdd)
{
	LCD_INFO(vdd, "%s\n", ss_get_panel_name(vdd));

	/* vdd->debug_data->print_cmds = true; */
	/* vdd->debug_data->print_frame = true; */

	vdd->panel_state = PANEL_PWR_OFF;	/* default OFF */

	/* MIPI Phy Tune Value for PA2 project */
	vdd->motto_info.motto_swing = 0xff;
	vdd->motto_info.vreg_ctrl_0 = 0x56;

	vdd->panel_func.samsung_panel_on_pre = samsung_panel_on_pre;
	vdd->panel_func.samsung_panel_on_post = samsung_panel_on_post;
	vdd->panel_func.samsung_display_on_post = samsung_display_on_post;

	vdd->panel_func.samsung_panel_revision = ss_panel_revision;
	vdd->panel_func.samsung_module_info_read = ss_module_info_read;
	vdd->panel_func.samsung_ddi_id_read = ss_ddi_id_read;
	vdd->panel_func.samsung_octa_id_read = ss_octa_id_read;

	vdd->panel_func.ddi_flash_test = ss_ddi_flash_test;

	/* Brightness */
	vdd->panel_func.pre_brightness = NULL;
	vdd->panel_func.pre_hmt_brightness = NULL;
	vdd->panel_func.pre_lpm_brightness = NULL;

	vdd->br_info.acl_status = 1; /* ACL default ON */
	vdd->br_info.gradual_acl_val = 1; /* ACL default status in acl on */
	vdd->br_info.temperature = 20;

	/* mdnie */
	vdd->mdnie.support_mdnie = true;
	vdd->mdnie.support_trans_dimming = true;
	vdd->mdnie.mdnie_tune_size[0] = sizeof(BYPASS_MDNIE_1);
	vdd->mdnie.mdnie_tune_size[1] = sizeof(BYPASS_MDNIE_2);
	vdd->mdnie.mdnie_tune_size[2] = sizeof(BYPASS_MDNIE_3);
	vdd->panel_func.pre_mdnie = ss_pre_mdnie;
	vdd->panel_func.set_vividness = mdnie_set_vividness;
	dsi_update_mdnie_data(vdd);

	vdd->panel_func.check_flash_done = ss_check_flash_done;

	vdd->self_disp.factory_support = true;
	vdd->self_disp.init = self_display_init;
	vdd->self_disp.data_init = ss_self_display_data_init;

	vdd->mafpc.init = ss_mafpc_init;
	vdd->mafpc.data_init = ss_mafpc_data_init;

	/* VRR */
	vdd->panel_func.samsung_lfd_get_base_val = ss_update_base_lfd_val;
	vdd->panel_func.post_vrr = ss_post_vrr;
	ss_vrr_init(&vdd->vrr);

	/* early te*/
	vdd->early_te = false;
	vdd->check_early_te = 0;

	vdd->panel_func.samsung_check_support_mode = ss_check_support_mode;

	/* Below data will be genarated by script in Kbuild file */
	vdd->h_buf = PS_S6E3HAF_AMB666LH01_VHM_PDF_DATA;
	vdd->h_size = sizeof(PS_S6E3HAF_AMB666LH01_VHM_PDF_DATA);

	/* Get f_buf from header file data to cover recovery mode
	 * Below code should be called before any PDF parsing code such as parsing_glut
	 */
	if (!vdd->file_loading && vdd->h_buf) {
		LCD_INFO(vdd, "Get f_buf from header file data(%lu)\n", vdd->h_size);
		vdd->f_buf = vdd->h_buf;
		vdd->f_size = vdd->h_size;
	}

	register_op_sym_cb(vdd, "GLUT", update_glut_offset, true);
	register_op_sym_cb(vdd, "GLUT_ENABLE", update_glut_enable, true);

	parse_glut(vdd);

	/* callback for PANEL_PWR_PMIC_SPECIFIC type */
	vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11].ctrl_cb = ss_panel_power_ctrl_specific;
	vdd->panel_powers[PANEL_POWERS_OFF_POST_LP11].ctrl_cb = ss_panel_power_ctrl_specific;
}
