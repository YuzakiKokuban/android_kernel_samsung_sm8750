/*
 * max77816_i2c.h - Platform data for max77816 buck booster hw i2c driver
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "../ss_dsi_panel_common.h"
int ss_boost_max77816_init(void);
int ss_boost_max77816_control(bool enable);

struct ss_boost_max77816_info {
	struct i2c_client *client;
};

enum {
	BOOSTIC_ADDR = 0,
	BOOSTIC_VAL,
	BOOSTIC_MAX,
};

u8 max77816_en[][BOOSTIC_MAX] = {
#if defined(CONFIG_PANEL_GTA5P_HX83123A_TL101VVMS01_WUXGA) ||\
	defined(CONFIG_PANEL_GTA5P_NT31536E_TL101VVMS01_WUXGA)
		/* addr value */
		{0x03, 0x30},
		{0x02, 0x8E},
		{0x03, 0x70},
		{0x04, 0x78},
#elif defined(CONFIG_PANEL_Q7M_ANA38407_AMSA10FA01_WQXGA)
		/* addr value */
		{0x03, 0x70},
		{0x02, 0x8E},
#else
		/* addr value */
		{0x03, 0x30},
		{0x02, 0x8E},
		{0x03, 0x70},
		{0x04, 0x78},
#endif
};
/* Below disable_data is not used */
u8 max77816_dis[][BOOSTIC_MAX] = {
#if defined(CONFIG_PANEL_GTA5P_HX83123A_TL101VVMS01_WUXGA) ||\
	defined(CONFIG_PANEL_GTA5P_NT31536E_TL101VVMS01_WUXGA)
		/* addr value */
		{0x03, 0x30},
#else
		/* addr value */
		{0x03, 0x30},
#endif
};
