/*
 * ktz8864b_hw_i2c.h - Platform data for ktz8864b backlight driver
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "../ss_dsi_panel_common.h"
int ss_blic_ktz8864b_init(void);
int ss_blic_ktz8864b_control(bool enable);

struct ss_blic_ktz8864b_info {
	struct i2c_client *client;
};

enum {
	BLIC_ADDR = 0,
	BLIC_VAL,
	BLIC_MAX,
};

u8 ktz8864b_en[][BLIC_MAX] = {
#if defined(CONFIG_PANEL_XC7P_NT36672C_TL066FVMC21_FHD) ||\
	defined(CONFIG_PANEL_XC7P_NT36672C_TL066FVMC2101_FHD)
		/* addr value */
		{0x0C, 0x24},
		{0x0D, 0x1E},
		{0x0E, 0x1E},
		{0x09, 0x99},
		{0x02, 0x6B},
		{0x03, 0x0D},
		{0x11, 0x74},
		{0x04, 0x05},
		{0x05, 0xCA},
		{0x10, 0x66},
		{0x08, 0x13},
#else
		/* addr value */
		{0x0C, 0x24},
		{0x0D, 0x1E},
		{0x0E, 0x1E},
		{0x09, 0x99},
		{0x02, 0x6B},
		{0x03, 0x0D},
		{0x11, 0x74},
		{0x04, 0x05},
		{0x05, 0xCA},
		{0x10, 0x66},
		{0x08, 0x13},
#endif
};

u8 ktz8864b_dis[][BLIC_MAX] = {
#if defined(CONFIG_PANEL_XC7P_NT36672C_TL066FVMC21_FHD) ||\
	defined(CONFIG_PANEL_XC7P_NT36672C_TL066FVMC2101_FHD)
		/* addr value */
		{0x08, 0x00}
#else
		{0x08, 0x00}
#endif
};
