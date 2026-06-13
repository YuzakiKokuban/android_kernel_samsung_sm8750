// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sec_actuator_core.h"
#include "cam_sensor_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"

#if defined(CONFIG_SAMSUNG_ACTUATOR_READ_HALL_VALUE)

#define ACTUATOR_STATUS_REGISTER_ADDR 0x2
#define ACTUATOR_HALL_REGISTER_ADDR 0x84

static int32_t cam_sec_actuator_i2c_read(struct cam_actuator_ctrl_t *a_ctrl, uint32_t addr,
		uint32_t *data,
        enum camera_sensor_i2c_type addr_type,
        enum camera_sensor_i2c_type data_type)
{
	int rc = 0;

	if (a_ctrl == NULL) {
		CAM_ERR(CAM_ACTUATOR, "failed. a_ctrl is NULL");
		return -EINVAL;
	}

	rc = camera_io_dev_read(&a_ctrl->io_master_info, addr,
		(uint32_t *)data, addr_type, data_type, false);
	if (rc < 0) {
		CAM_ERR(CAM_ACTUATOR, "Failed to read 0x%x", addr);
	}

	return rc;
}

static int32_t cam_sec_actuator_get_status_for_hall_value(struct cam_actuator_ctrl_t *a_ctrl, uint16_t *info)
{
	int32_t rc = 0;
	uint32_t val = 0;

	rc = cam_sec_actuator_i2c_read(a_ctrl, ACTUATOR_STATUS_REGISTER_ADDR, &val, CAMERA_SENSOR_I2C_TYPE_BYTE, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0) {
		CAM_ERR(CAM_ACTUATOR, "get status i2c read fail:%d", rc);
		return -EINVAL;
	}

	CAM_INFO(CAM_ACTUATOR, "[AF] val = 0x%x", val);

	*info = (val & 0x60);

	return rc;
}

static void cam_sec_actuator_busywait_for_hall_value(struct cam_actuator_ctrl_t *a_ctrl)
{
	uint16_t info = 0, status_check_count = 0;
	int32_t rc = 0;

	do {
		rc = cam_sec_actuator_get_status_for_hall_value(a_ctrl, &info);
		if (rc < 0) {
			CAM_ERR(CAM_ACTUATOR, "cam_actuator_get_status failed:%d", rc);
		}
		if  (info) {
			CAM_INFO(CAM_ACTUATOR, "[AF] Not Active");
			msleep(10);
		}

		status_check_count++;
	} while (info && status_check_count < 8);

	if (status_check_count == 8)
		CAM_ERR(CAM_ACTUATOR, "[AF] status check failed");
	else
		CAM_INFO(CAM_ACTUATOR, "[AF] Active");
}

int32_t cam_sec_actuator_read_hall_value(struct cam_actuator_ctrl_t *a_ctrl, uint16_t* af_hall_value)
{
	int32_t rc = 0;
	uint8_t value[2];
	uint16_t hallValue = 0;

	if (a_ctrl == NULL) {
		CAM_ERR(CAM_ACTUATOR, "failed. a_ctrl is NULL");
		return -EINVAL;
	}

	cam_sec_actuator_busywait_for_hall_value(a_ctrl);
#if defined(CONFIG_SEC_FACTORY)
	msleep(50);
#endif	

	rc = camera_io_dev_read_seq(&a_ctrl->io_master_info, ACTUATOR_HALL_REGISTER_ADDR, value, CAMERA_SENSOR_I2C_TYPE_BYTE, CAMERA_SENSOR_I2C_TYPE_BYTE, 2);

	hallValue = (((uint16_t)value[0]) << 4) | ((uint16_t)value[1]) >> 4;

	if (rc < 0) {
		CAM_ERR(CAM_ACTUATOR, "get status i2c read fail:%d", rc);
		return -EINVAL;
	}

	CAM_INFO(CAM_ACTUATOR, "[AF] RAW data = %u", hallValue);

	*af_hall_value = hallValue;

	return rc;
}
#endif
