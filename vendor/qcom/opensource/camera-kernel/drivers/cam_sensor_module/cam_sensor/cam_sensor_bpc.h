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

#ifndef _CAM_SENSOR_BPC_H_
#define _CAM_SENSOR_BPC_H_

#define SENSOR_BPC_READ_RETRY_CNT 50
#define BPC_OTP_SIZE_MAX 0x9000
#define BPC_OTP_READ_STATUS_ADDR 0x7422
#define BPC_OTP_READ_STATUS_OK 0x0000
#define BPC_OTP_PAGE_NUM 9
#define SENSOR_REVISION_ADDR 0x0002
#define S5KHP2_SENSOR_REVISION_EVT1 0xB000
#define S5KHP2_SENSOR_SUPPORT_BPC_CRC_SENSOR_REVISION 0xB100
#define BPC_OTP_TERMINATE_CODE_FOR_CRC 0xFFFFFFFF


extern uint8_t *otp_data;

struct cam_sensor_i2c_reg_array bpc_sw_reset_setting[] = {
	{ 0xFCFC, 0x4000, 0x00, 0x00 },
	{ 0x6018, 0x0001, 0x00, 0x00 },
};

struct cam_sensor_i2c_reg_array bpc_dram_init_settting[] = {
	{ 0x6226, 0x0001, 0x00, 0x00 },
	{ 0x6214, 0x0800, 0x00, 0x00 },
	{ 0x6218, 0x0000, 0x00, 0x00 },
	{ 0x7402, 0x0019, 0x00, 0x00 },
	{ 0xB000, 0x0000, 0x00, 0x00 },
	{ 0xB002, 0x0000, 0x00, 0x00 },
};

struct cam_sensor_i2c_reg_array bpc_configure_otp_addr_settting[] = {
	{ 0x7414, 0xBA40, 0x00, 0x00 },
	{ 0x7416, 0x0000, 0x00, 0x00 },
	{ 0x7418, 0x9000, 0x00, 0x00 },
	{ 0x741A, 0x0000, 0x00, 0x00 },
};

struct cam_sensor_i2c_reg_array bpc_configure_dram_settting[] = {
	{ 0x7426, 0x0004, 0x00, 0x00 },
	{ 0x741E, 0x0000, 0x00, 0x00 },
	{ 0x7420, 0x0000, 0x00, 0x00 },
	{ 0x7424, 0x0002, 0x00, 0x00 },
};

struct cam_sensor_i2c_reg_array bpc_otp_read_setting[] = {
	{ 0x7422, 0x0001, 0x00, 0x00 },
};

struct cam_sensor_i2c_reg_array bpc_end_sequence_settting[] = {
	{ 0x6000, 0x0005, 0x00, 0x00 },
	{ 0xFCFC, 0x4150, 0x00, 0x00 },
	{ 0x0004, 0x0030, 0x00, 0x00 },
	{ 0xFCFC, 0x4000, 0x00, 0x00 },
	{ 0xB000, 0x0001, 0x00, 0x00 },
	{ 0x6214, 0x0000, 0x00, 0x00 },
	{ 0x6000, 0x0085, 0x00, 0x00 },
	{ 0xFCFC, 0x2006, 0x00, 0x00 },
};

struct cam_sensor_i2c_reg_setting read_otp_setttings[] =  {
    {
		bpc_sw_reset_setting,
		ARRAY_SIZE(bpc_sw_reset_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		100
	},
	{
		bpc_dram_init_settting,
		ARRAY_SIZE(bpc_dram_init_settting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
	{
		bpc_configure_otp_addr_settting,
		ARRAY_SIZE(bpc_configure_otp_addr_settting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
	{
		bpc_configure_dram_settting,
		ARRAY_SIZE(bpc_configure_dram_settting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		100
	},
	{
		bpc_otp_read_setting,
		ARRAY_SIZE(bpc_otp_read_setting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};

struct cam_sensor_i2c_reg_setting bpc_end_sequence_setttings[] =  {
	{
		bpc_end_sequence_settting,
		ARRAY_SIZE(bpc_end_sequence_settting),
		CAMERA_SENSOR_I2C_TYPE_WORD,
		CAMERA_SENSOR_I2C_TYPE_WORD,
		0
	},
};
#endif /* _CAM_SENSOR_BPC_H_ */
