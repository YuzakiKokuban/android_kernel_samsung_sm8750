/*
 * lp8558_hw_i2c.h - Platform data for lp8558 backlight driver
 *
 * Copyright (C) 2020 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "../ss_dsi_panel_common.h"

struct ss_blic_lp8558_info {
	struct i2c_client			*client;
};

static struct ss_blic_lp8558_info backlight_pinfo;

int ss_blic_lp8558_control(struct samsung_display_driver_data *vdd);
int ss_blic_lp8558_init(void);
