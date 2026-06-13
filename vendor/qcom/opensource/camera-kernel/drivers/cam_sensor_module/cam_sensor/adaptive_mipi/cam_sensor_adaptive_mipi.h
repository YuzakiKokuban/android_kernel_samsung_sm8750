/*
 * Samsung SDM845 CAM MIPI driver
 *
 * SDM845 mipi channel definition
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CAM_SENSOR_ADAPTIVE_MIPI_H
#define CAM_SENSOR_ADAPTIVE_MIPI_H

#include "cam_sensor_dev.h"

extern struct cam_sensor_adaptive_mipi_info s5kgn3_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info s5khp2_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info imx754_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info imx854_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info s5k3lu_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info imx564_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info s5k3k1_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info s5kjn3_adaptive_mipi_info;
extern struct cam_sensor_adaptive_mipi_info imx874_adaptive_mipi_info;

/*************************************/
/* ========== DUMMY ================ */
/*************************************/
static struct cam_sensor_i2c_reg_array MIPI_DUMMY_REG_ARRAY[] = {
};

static const struct cam_sensor_i2c_reg_setting sensor_setfile_dummy[] = {
    { MIPI_DUMMY_REG_ARRAY, ARRAY_SIZE(MIPI_DUMMY_REG_ARRAY),
	  CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD, 0 }
};

static const struct cam_mipi_setting sensor_setfile_dummy_mipi_setting[] = {
	{ "DUMMY Msps", 0,
	  sensor_setfile_dummy, ARRAY_SIZE(sensor_setfile_dummy) },
};

static const struct cam_mipi_cell_ratings sensor_setfile_dummy_channel[] = {
	{ CAM_RAT_BAND(CAM_RAT_1_GSM, CAM_BAND_001_GSM_GSM850), 0, 0, {0} },
};

static const struct cam_mipi_sensor_mode sensor_mipi_dummy_mode[] = {
	{
		sensor_setfile_dummy_channel,	ARRAY_SIZE(sensor_setfile_dummy_channel),
		sensor_setfile_dummy_mipi_setting,	ARRAY_SIZE(sensor_setfile_dummy_mipi_setting)
	},
};

#endif /* CAM_SENSOR_ADAPTIVE_MIPI_H */
