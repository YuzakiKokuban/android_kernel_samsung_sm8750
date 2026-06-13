/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef _CAM_SENSOR_RETENTION_H_
#define _CAM_SENSOR_RETENTION_H_

#define S5KHP2_SENSOR_ID	0x1B72
#define S5KGN3_SENSOR_ID	0x08E3
#define IMX854_SENSOR_ID	0x0854
#define S5KJN3_SENSOR_ID	0x38E3

struct cam_sensor_ctrl_t;
struct cam_sensor_retention_info {
	int (*retention_init) (struct cam_sensor_ctrl_t *s_ctrl);
	int (*retention_exit) (struct cam_sensor_ctrl_t *s_ctrl);
	int (*retention_enter) (struct cam_sensor_ctrl_t *s_ctrl);
	bool retention_support;
};

void cam_sensor_get_retention_info (struct cam_sensor_ctrl_t *s_ctrl);

#endif /* _CAM_SENSOR_RETENTION_H_ */
