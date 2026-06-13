/*
 * ktz8868_hw_i2c.h - Platform data for ktz8868 backlight driver
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "../ss_dsi_panel_common.h"
int ss_blic_ktz8868_init(void);
int ss_blic_ktz8868_control(bool enable);

struct ss_blic_ktz8868_info {
	struct i2c_client *client;
};

enum {
	BLIC_ADDR = 0,
	BLIC_VAL,
	BLIC_MAX,
};

u8 ktz8868_en[][BLIC_MAX] = {
#if defined(CONFIG_PANEL_GTA5P_HX83123A_TL101VVMS01_WUXGA) ||\
	defined(CONFIG_PANEL_GTA5P_NT31536E_TL101VVMS01_WUXGA)
		/* addr value */
		{0x0C, 0x28},
		{0x0D, 0x24},
		{0x0E, 0x24},
		{0x09, 0x99},
		{0x02, 0x6B},
		{0x03, 0x8D},
		{0x11, 0x37},
		{0x04, 0x07},
		{0x05, 0x9B},
		{0x10, 0x00},
		{0x08, 0xFF},
		{0x01, 0x01},
#else
		/* addr value */
		{0x0C, 0x28},
		{0x0D, 0x24},
		{0x0E, 0x24},
		{0x09, 0x99},
		{0x02, 0x6B},
		{0x03, 0x8D},
		{0x11, 0x37},
		{0x04, 0x03},
		{0x05, 0xC2},
		{0x10, 0x00},
		{0x08, 0xFF},
		{0x01, 0x01},
#endif
};

u8 ktz8868_dis[][BLIC_MAX] = {
#if defined(CONFIG_PANEL_GTA5P_HX83123A_TL101VVMS01_WUXGA) ||\
	defined(CONFIG_PANEL_GTA5P_NT31536E_TL101VVMS01_WUXGA)
		/* addr value */
		{0x01, 0x00}
#else
		{0x01, 0x00}
#endif
};
