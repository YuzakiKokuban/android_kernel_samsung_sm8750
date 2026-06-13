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

#define S5KHP2_RETENTION_READY_ADDR 	0xF36E
#define S5KHP2_RETENTION_CHECKSUM_PASS	0xF36C
#define S5KHP2_RETENTION_STATUS_OK		0x0100

struct cam_sensor_i2c_reg_array s5khp2_stream_on_setting[] = {
	{ 0x0100,	0x0103, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5khp2_stream_on_settings[] =  {
	{	s5khp2_stream_on_setting,
		ARRAY_SIZE(s5khp2_stream_on_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

struct cam_sensor_i2c_reg_array s5khp2_stream_off_setting[] = {
	{ 0x0100,	0x0003, 0x00,	0x00 },
};

// Case3. Retention / Checksum register reset
struct cam_sensor_i2c_reg_array s5khp2_retention_reset_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 },
	{ 0x6028,	0x1002, 0x00,	0x00 },
	{ 0x602A,	0xF36C, 0x00,	0x00 },
	{ 0x6F12,	0x0000, 0x00,	0x00 },
	{ 0x6F12,	0x0000, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5khp2_stream_off_settings[] =  {
	{	s5khp2_retention_reset_setting,
		ARRAY_SIZE(s5khp2_retention_reset_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
	{	s5khp2_stream_off_setting,
		ARRAY_SIZE(s5khp2_stream_off_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

struct cam_sensor_i2c_reg_setting s5khp2_retention_reset_settings[] =  {
	{	s5khp2_retention_reset_setting,
		ARRAY_SIZE(s5khp2_retention_reset_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

// Case 2. EXIT Retention Setting
struct cam_sensor_i2c_reg_array s5khp2_retention_exit1_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 },
	{ 0x6018,	0x0001, 0x01,	0x00 },
};

struct cam_sensor_i2c_reg_array s5khp2_retention_exit2_setting[] = {
	{ 0x652A,	0x0001, 0x00,	0x00 },
	{ 0x7096,	0x0001, 0x00,	0x00 },
	{ 0x7002,	0x0008, 0x00,	0x00 },
	{ 0x706E,	0x0D13, 0x00,	0x00 },
	{ 0x6028,	0x1001, 0x00,	0x00 },
	{ 0x602A,	0xC990, 0x00,	0x00 },
	{ 0x6F12,	0x1002, 0x00,	0x00 },
	{ 0x6F12,	0xF601, 0x00,	0x00 },
	{ 0x6028,	0x1002, 0x00,	0x00 },
	{ 0x602A,	0xC8C0, 0x00,	0x00 },
	{ 0x6F12,	0xCAFE, 0x00,	0x00 },
	{ 0x6F12,	0x1234, 0x00,	0x00 },
	{ 0x6F12,	0xABBA, 0x00,	0x00 },
	{ 0x6F12,	0x0345, 0x00,	0x00 },
	{ 0x6014,	0x0001, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5khp2_retention_exit_settings[] =  {
	{	s5khp2_retention_exit1_setting,
		ARRAY_SIZE(s5khp2_retention_exit1_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		5
	},
	{	s5khp2_retention_exit2_setting,
		ARRAY_SIZE(s5khp2_retention_exit2_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		10
	},
};

// Case 2. HW Setting Init
struct cam_sensor_i2c_reg_array s5khp2_retention_hw_init_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 },
	{ 0x6214,	0xE949, 0x00,	0x00 },
	{ 0x6218,	0xE940, 0x00,	0x00 },
	{ 0x6222,	0x0000, 0x00,	0x00 },
	{ 0x621E,	0x00F0, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5khp2_retention_hw_init_settings[] =  {
	{	s5khp2_retention_hw_init_setting,
		ARRAY_SIZE(s5khp2_retention_hw_init_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

struct cam_sensor_i2c_reg_array s5khp2_retention_page_setting[] = {
	{ 0xFCFC,	0x1002, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5khp2_retention_page_settings[] = {
	{	s5khp2_retention_page_setting,
		ARRAY_SIZE(s5khp2_retention_page_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

struct cam_sensor_i2c_reg_array s5khp2_normal_page_setting[] = {
	{ 0xFCFC,	0x4000, 0x00,	0x00 },
};

struct cam_sensor_i2c_reg_setting s5khp2_normal_page_settings[] = {
	{	s5khp2_normal_page_setting,
		ARRAY_SIZE(s5khp2_normal_page_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

int s5khp2_stream_on(struct cam_sensor_ctrl_t *s_ctrl) {
	int rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] stream on");
	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_stream_on_settings, ARRAY_SIZE(s5khp2_stream_on_settings));
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

int s5khp2_stream_off(struct cam_sensor_ctrl_t *s_ctrl) {
	int rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] stream off");
	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_stream_off_settings, ARRAY_SIZE(s5khp2_stream_off_settings));
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

int s5khp2_retention_wait_ready(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	if (s_ctrl->streamon_count == 0 ||
		s_ctrl->retention_stream_on == false) {
		rc = s5khp2_stream_on(s_ctrl);
		rc |= s5khp2_stream_off(s_ctrl);
	}

	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_retention_page_settings, ARRAY_SIZE(s5khp2_retention_page_settings));
	rc |= camera_io_dev_poll(&s_ctrl->io_master_info,
		S5KHP2_RETENTION_READY_ADDR, S5KHP2_RETENTION_STATUS_OK, 0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,
		100);
	rc |= cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_normal_page_settings, ARRAY_SIZE(s5khp2_normal_page_settings));

	return rc;
}

int s5khp2_retention_checksum(struct cam_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_retention_page_settings, ARRAY_SIZE(s5khp2_retention_page_settings));
	rc |= camera_io_dev_poll(&s_ctrl->io_master_info,
		S5KHP2_RETENTION_CHECKSUM_PASS, S5KHP2_RETENTION_STATUS_OK, 0,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD,
		100);
	rc |= cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_normal_page_settings, ARRAY_SIZE(s5khp2_normal_page_settings));

	return rc;
}

int s5khp2_retention_init(struct cam_sensor_ctrl_t *s_ctrl)
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

		rc |= s5khp2_retention_wait_ready(s_ctrl);
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

int s5khp2_retention_exit(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] E");

	s_ctrl->retention_checksum = false;

	rc = cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_retention_exit_settings, ARRAY_SIZE(s5khp2_retention_exit_settings));
	rc |= s5khp2_retention_checksum(s_ctrl);
	if (rc != 0)
		CAM_ERR(CAM_SENSOR,	"[RET_DBG] Retention checksum fail, rc = %d", rc);
	else
		rc |= cam_sensor_write_settings(&s_ctrl->io_master_info,
		s5khp2_retention_hw_init_settings, ARRAY_SIZE(s5khp2_retention_hw_init_settings));

	if (rc == 0)
		s_ctrl->retention_checksum = true;

	s_ctrl->retention_stream_on = false;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] X");

	return rc;
}

// Pre-Stream off, Retention/Checksum register reset
int s5khp2_retention_enter(struct cam_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	CAM_INFO(CAM_SENSOR, "[RET_DBG] E");

	rc = s5khp2_retention_wait_ready(s_ctrl);
	if (rc < 0)
		CAM_ERR(CAM_SENSOR, "[RET_DBG] Failed to enter retention mode rc = %d", rc);

	CAM_INFO(CAM_SENSOR, "[RET_DBG] X");

	return rc;
}

struct cam_sensor_retention_info s5khp2_retention_info = {
	.retention_init = s5khp2_retention_init,
	.retention_exit = s5khp2_retention_exit,
	.retention_enter = s5khp2_retention_enter,
	.retention_support = true,
};
