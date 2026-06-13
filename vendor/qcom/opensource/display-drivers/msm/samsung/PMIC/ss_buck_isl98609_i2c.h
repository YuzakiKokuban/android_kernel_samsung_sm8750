/*
 * isl98608_hw_i2c.h - Platform data for isl98608 buck driver
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "../ss_dsi_panel_common.h"

struct ss_buck_isl98608_info {
	struct i2c_client			*client;
};

static struct ss_buck_isl98608_info buck_pinfo;

int ss_buck_i2c_read_ex(u8 addr,  u8 *value);
int ss_buck_i2c_write_ex(u8 addr,  u8 value);
int ss_buck_isl98608_control(struct samsung_display_driver_data *vdd);
int ss_buck_isl98608_init(void);
