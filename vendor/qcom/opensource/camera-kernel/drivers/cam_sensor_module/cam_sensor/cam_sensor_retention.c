// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_sensor_dev.h"
#include "cam_sensor_retention.h"

int default_retention_init(struct cam_sensor_ctrl_t *s_ctrl)
{
	return 0;
}

int default_retention_exit(struct cam_sensor_ctrl_t *s_ctrl)
{
	return 0;
}

int default_retention_enter(struct cam_sensor_ctrl_t *s_ctrl)
{
	return 0;
}

struct cam_sensor_retention_info default_retention_info = {
	.retention_init = default_retention_init,
	.retention_exit = default_retention_exit,
	.retention_enter = default_retention_enter,
	.retention_support = false,
};

extern struct cam_sensor_retention_info s5kgn3_retention_info;
extern struct cam_sensor_retention_info s5khp2_retention_info;
extern struct cam_sensor_retention_info imx854_retention_info;
extern struct cam_sensor_retention_info s5kjn3_retention_info;

void cam_sensor_get_retention_info (struct cam_sensor_ctrl_t *s_ctrl)
{
	uint16_t sensor_id = s_ctrl->sensordata->slave_info.sensor_id;
	CAM_INFO(CAM_SENSOR, "[RET_DBG] sensor_id 0x%x", sensor_id);
	s_ctrl->retention_info = default_retention_info;
	if (sensor_id == S5KHP2_SENSOR_ID)
		s_ctrl->retention_info = s5khp2_retention_info;
	if (sensor_id == IMX854_SENSOR_ID)
		s_ctrl->retention_info = imx854_retention_info;
	if (sensor_id == S5KGN3_SENSOR_ID)
		s_ctrl->retention_info = s5kgn3_retention_info;
	if (sensor_id == S5KJN3_SENSOR_ID)
		s_ctrl->retention_info = s5kjn3_retention_info;

};
