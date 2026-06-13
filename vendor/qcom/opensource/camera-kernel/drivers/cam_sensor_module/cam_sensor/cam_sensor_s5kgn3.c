// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <cam_sensor_cmn_header.h>
#include "cam_sensor_core.h"
#include "cam_sensor_util.h"
#include "cam_sensor_retention.h"
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
#include "cam_hw_bigdata.h"
#endif

#define S5KGN3_RETENTION_READY_ADDR 	0x19C4
#define S5KGN3_RETENTION_CHECKSUM_PASS	0x19C2
#define S5KGN3_RETENTION_STATUS_OK		0x0100
#define S5KGN3_RETENTION_MODE_ADDR		0x010E

struct cam_sensor_i2c_reg_array s5kgn3_stream_on_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 }, // set 4000 page
	{ 0x6000,	0x0005, 0x00,	0x00 }, // 16bit READ/WRITE set
	{ 0x19C4,	0x0000, 0x00,	0x00 }, // retention mode enter preparation setting initilaization
	{ 0x6000,	0x0085, 0x00,	0x00 }, // 8bit READ/WRITE set
	{ 0x0100,	0x0103, 0x00,	0x00 }, // streaming on
};

struct cam_sensor_i2c_reg_setting s5kgn3_stream_on_settings[] =  {
	{	s5kgn3_stream_on_setting,
		ARRAY_SIZE(s5kgn3_stream_on_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0,
	},
};

struct cam_sensor_i2c_reg_array s5kgn3_stream_off_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 }, // set 4000 page
	{ 0x6000,	0x0005, 0x00,	0x00 }, // 16bit READ/WRITE set
	{ 0x010E,	0x0100, 0x00,	0x00 }, // Retention checksum check enable
	{ 0x19C2,	0x0000, 0x00,	0x00 }, // retention mode P/F initialization
	{ 0x6000,	0x0085, 0x00,	0x00 }, // 8bit READ/WRITE set
	{ 0x0100,	0x0003, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5kgn3_stream_off_settings[] =  {
	{	s5kgn3_stream_off_setting,
		ARRAY_SIZE(s5kgn3_stream_off_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0,
	},
};

struct cam_sensor_i2c_reg_array s5kgn3_retention_exit_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 }, // set 4000 page
	{ 0x6000,	0x0005, 0x00,	0x00 }, // 16bit READ/WRITE set
};

struct cam_sensor_i2c_reg_setting s5kgn3_retention_exit_settings[] =  {
	{	s5kgn3_retention_exit_setting,
		ARRAY_SIZE(s5kgn3_retention_exit_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0,
	},
};

int s5kgn3_stream_on(struct cam_sensor_ctrl_t *s_ctrl) {
	int rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] stream on");
	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5kgn3_stream_on_settings, ARRAY_SIZE(s5kgn3_stream_on_settings));
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,
			"[RET_DBG] Failed to write stream on rc = %d", rc);
		return rc;
	}

#if defined(CONFIG_CAMERA_FRAME_CNT_CHECK)
	rc = cam_sensor_wait_stream_onoff(s_ctrl, true);
#endif

	return rc;
}

int s5kgn3_stream_off(struct cam_sensor_ctrl_t *s_ctrl) {
	int rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] stream off");
	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5kgn3_stream_off_settings, ARRAY_SIZE(s5kgn3_stream_off_settings));
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR,
			"[RET_DBG] Failed to write stream off rc = %d", rc);
		return rc;
	}

#if defined(CONFIG_CAMERA_FRAME_CNT_CHECK)
	rc = cam_sensor_wait_stream_onoff(s_ctrl, false);
#endif

	return rc;
}

int s5kgn3_retention_wait_ready(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	if (s_ctrl->streamon_count == 0 ||
		s_ctrl->retention_stream_on == false) {
		rc = s5kgn3_stream_on(s_ctrl);
		rc |= s5kgn3_stream_off(s_ctrl);
	}

	// max delay 2ms
	usleep_range(2000, 2100);

	rc |= camera_io_dev_poll(&s_ctrl->io_master_info,
		S5KGN3_RETENTION_READY_ADDR, S5KGN3_RETENTION_STATUS_OK, 0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,
		100);

	return rc;
}

int s5kgn3_retention_init(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] E");

	if (s_ctrl->i2c_data.init_settings.is_settings_valid &&
		(s_ctrl->i2c_data.init_settings.request_id == 0)) {
		rc = cam_sensor_apply_settings(s_ctrl, 0,
			CAM_SENSOR_PACKET_OPCODE_SENSOR_INITIAL_CONFIG);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR,
				"[RET_DBG] Failed to write init rc = %d", rc);
#if defined(CONFIG_USE_CAMERA_HW_BIG_DATA)
			hw_bigdata_i2c_from_sensor(s_ctrl);
#endif
			goto end;
		}

		rc |= s5kgn3_retention_wait_ready(s_ctrl);
		if (rc != 0) {
			CAM_ERR(CAM_SENSOR,
				"[RET_DBG] Failed to wait retention ready rc = %d", rc);
			goto end;
		}
	}
end:
	s_ctrl->retention_stream_on = false;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] X");

	return rc;
}

int s5kgn3_retention_exit(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] E");

	s_ctrl->retention_checksum = false;

	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5kgn3_retention_exit_settings, ARRAY_SIZE(s5kgn3_retention_exit_settings));
	rc |= camera_io_dev_poll(&s_ctrl->io_master_info,
		S5KGN3_RETENTION_MODE_ADDR, 0, 0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,
		15);
	rc |= camera_io_dev_poll(&s_ctrl->io_master_info,
		S5KGN3_RETENTION_CHECKSUM_PASS, S5KGN3_RETENTION_STATUS_OK, 0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,
		100);

	if (rc == 0)
		s_ctrl->retention_checksum = true;

	s_ctrl->retention_stream_on = false;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] X");

	return rc;
}

// Pre-Stream off, Retention/Checksum register reset
int s5kgn3_retention_enter(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] E");

	rc = s5kgn3_retention_wait_ready(s_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "[RET_DBG] Failed to enter retention mode rc = %d", rc);

	CAM_INFO(CAM_SENSOR, "[RET_DBG] X");

	return rc;
}

struct cam_sensor_retention_info s5kgn3_retention_info = {
	.retention_init = s5kgn3_retention_init,
	.retention_exit = s5kgn3_retention_exit,
	.retention_enter = s5kgn3_retention_enter,
	.retention_support = true,
};
