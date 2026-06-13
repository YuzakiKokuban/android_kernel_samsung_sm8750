/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
 */

#ifndef _CAM_SYSFS_LPAI_OIS_H_
#define _CAM_SYSFS_LPAI_OIS_H_

#include <linux/sysfs.h>
#include "cam_sensor_cmn_header.h"
#include "cam_eeprom_dev.h"

extern const struct device_attribute *ois_attrs[];
extern struct cam_ois_ctrl_t *g_o_ctrls[SEC_SENSOR_ID_MAX];
extern struct cam_actuator_ctrl_t *g_a_ctrls[SEC_SENSOR_ID_MAX];

int ois_handle_factory_test_result(struct cam_factory_test_cmd* dev_config);
void ois_handle_ois_autotest_result(uint8_t* payload);
void ois_handle_gyro_calibration_result(uint8_t* payload);
void ois_handle_gyro_noise_stdev_result(uint8_t* payload);
void ois_handle_gyro_selftest_result(uint8_t* payload);
void ois_handle_ois_valid_check_result(uint8_t* payload);
void ois_handle_ois_get_exif_result(uint8_t* payload);
void ois_handle_ois_get_fw_result(uint8_t* payload);
void ois_handle_ois_hall_cal_result(uint8_t* payload);
void ois_handle_ois_cross_talk_result(uint8_t* payload);
void ois_handle_ois_get_mgless_result(uint8_t* payload);
void ois_handle_ois_hall_position_result(uint8_t* payload);
void ois_handle_ois_reset_check_result(uint8_t* payload);
void ois_handle_ois_error(uint8_t* payload);
#endif /* _CAM_SYSFS_LPAI_OIS_H_ */
