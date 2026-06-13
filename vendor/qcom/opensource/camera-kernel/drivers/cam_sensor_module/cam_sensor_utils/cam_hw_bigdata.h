/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef _CAM_HW_BIGDATA_H_
#define _CAM_HW_BIGDATA_H_

#include "cam_sensor_dev.h"
#include "cam_actuator_dev.h"
#include "cam_ois_dev.h"
#include "cam_sec_eeprom_core.h"
#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI) && defined(CONFIG_CAMERA_RF_MIPI)
#include "cam_sensor_mipi.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MODULE_ID_INVALID 0
#define MODULE_ID_VALID 1
#define MODULE_ID_ERR_CHAR -1
#define MODULE_ID_ERR_CNT_MAX -2

#define MAX_HW_PARAM_STR_LEN 15

#define CAM_HW_ERR_CNT_FILE_PATH "/data/camera/camera_hw_err_cnt.dat"

typedef enum {
	HW_PARAMS_CREATED = 0,
	HW_PARAMS_NOT_CREATED,
} hw_params_check_type;

enum hw_param_id_type {
	HW_PARAM_REAR = 0,
	HW_PARAM_REAR2,
	HW_PARAM_REAR3,
	HW_PARAM_REAR4,
	HW_PARAM_FRONT,
	HW_PARAM_FRONT2,
	HW_PARAM_FRONT3,
	MAX_HW_PARAM_ID,
};

enum hw_param_error_type {
	I2C_SENSOR_ERROR = 0,
	I2C_COMP_ERROR,
	I2C_OIS_ERROR,
	I2C_AF_ERROR,
	MIPI_SENSOR_ERROR,
	I2C_EEPROM_ERROR,
	CRC_EEPROM_ERROR,
	AF_HALL_ERROR,
	MAX_HW_PARAM_ERROR,
};

enum hw_param_info_type
{
	CAMI_ID,
	I2C_AF,
	I2C_OIS,
	I2C_SEN,
	MIPI_SEN,
	MIPI_INFO,
	I2C_EEPROM,
	CRC_EEPROM,
	CAM_USE_CNT,
	WIFI_INFO,
	AF_HALL,
	AF_INFO,
	MAX_HW_PARAM_INFO,
};

struct cam_hw_param {
	u32 err_cnt[MAX_HW_PARAM_ERROR];
	u16 mipi_chk;
	u16 need_update_to_file;
	u8  rf_rat;
	u32 rf_band;
	u32 rf_channel;
	u8  eeprom_i2c_chk;
	u8  eeprom_crc_chk;
	u32 cam_entrance_cnt;
	u8  cam_available;
	char af_info_value_prev[30];
} __attribute__((__packed__));

struct hw_param_collector {
	struct cam_hw_param hwparam[MAX_HW_PARAM_ID + 1];
	char camera_info[10];
} __attribute__((__packed__));

void hw_bigdata_update_err_cnt(struct cam_hw_param *hw_param, uint32_t modules);
void hw_bigdata_update_eeprom_error_cnt(struct cam_sensor_ctrl_t *s_ctrl);
void hw_bigdata_update_cam_entrance_cnt(struct cam_sensor_ctrl_t *s_ctrl);

int hw_bigdata_get_hw_param_id(uint32_t camera_id);
void hw_bigdata_get_hw_param(struct cam_hw_param **hw_param, uint32_t hw_param_id);
void hw_bigdata_get_hw_param_static(struct cam_hw_param **hw_param, uint32_t hw_param_id);

void hw_bigdata_init_mipi_param(struct cam_hw_param *hw_param);
void hw_bigdata_init_mipi_param_sensor(struct cam_sensor_ctrl_t *s_ctrl);
void hw_bigdata_deinit_mipi_param_sensor(struct cam_sensor_ctrl_t *s_ctrl);
void hw_bigdata_i2c_from_sensor(struct cam_sensor_ctrl_t *s_ctrl);
void hw_bigdata_mipi_from_ife_csid_ver1(uint32_t hw_cam_position);
void hw_bigdata_mipi_from_ife_csid_ver2(int csiphy_num);
void hw_bigdata_i2c_from_actuator(struct cam_actuator_ctrl_t *a_ctrl);
void hw_bigdata_i2c_from_ois_status_reg(uint32_t hw_cam_position);
void hw_bigdata_i2c_from_ois_error_reg(uint32_t err_reg);
void hw_bigdata_i2c_from_eeprom(struct cam_eeprom_ctrl_t *a_ctrl);
void hw_bigdata_crc_from_eeprom(struct cam_eeprom_ctrl_t *a_ctrl);

void hw_bigdata_init_all_cnt(void);
void hw_bigdata_init_err_cnt_file(struct cam_hw_param *hw_param);
void hw_bigdata_copy_err_cnt_from_file(void);
void hw_bigdata_copy_err_cnt_to_file(void);
int hw_bigdata_file_exist(char *filename, hw_params_check_type chktype);

#if defined(CONFIG_CAMERA_ADAPTIVE_MIPI) && defined(CONFIG_CAMERA_RF_MIPI)
void hw_bigdata_get_rfinfo(struct cam_hw_param *hw_param);
#endif
uint32_t hw_bigdata_get_error_cnt(struct cam_hw_param *hw_param, uint32_t modules);
void hw_bigdata_debug_info(void);
ssize_t hw_bigdata_fill(char *buf, struct cam_hw_param *ec_param, char *moduleid,
	char *af_str, uint32_t hw_param_id, char* wifi);
#if defined(CONFIG_SAMSUNG_ACTUATOR_READ_HALL_VALUE)
void hw_bigdata_hall_from_actuator(struct cam_sensor_ctrl_t *s_ctrl);
char* get_af_hall_error_info(char *af_hall_info, struct cam_hw_param *ec_param);
#endif

#endif /* _CAM_HW_BIGDATA_H_ */
