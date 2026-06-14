/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#ifndef __ADSP_SENSOR_H__
#define __ADSP_SENSOR_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
//#include <linux/sensors.h>
#include <linux/adsp/adsp_ft_common.h>

#define TIMEOUT_CNT 200
#define TIMEOUT_DHR_CNT 50

#define PATH_LEN                 50
#define FILE_BUF_LEN             110
#define ID_INDEX_NUMS            2
#define RETRY_MAX                3
#define VERSION_FILE_NAME_LEN    20
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
/* To avoid wrong folder close */
#define LSM6DSO_SELFTEST_TRUE 3
#define LSM6DSO_SELFTEST_FALSE 4
#endif
#define AUTO_CAL_X_START	1
#define AUTO_CAL_Y_START	20
#define AUTO_CAL_Z_START	39
#define UNKNOWN_INDEX      0
#define DEVICE_INFO_LENGTH 10

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
	#define __mockable __weak
#define __visible_for_testing
#else
#define __mockable
#define __visible_for_testing static
#endif

enum {
	D_FACTOR,
	R_COEF,
	G_COEF,
	B_COEF,
	C_COEF,
	CT_COEF,
	CT_OFFSET,
	THD_HIGH,
	THD_LOW,
	IRIS_PROX_THD,
	SUM_CRC,
	EFS_SAVE_NUMS,
};

enum {
	ID_UTYPE,
	ID_BLACK,
	ID_WHITE,
	ID_GOLD,
	ID_SILVER,
	ID_GREEN,
	ID_BLUE,
	ID_PINKGOLD,
	ID_MAX,
};

#if IS_ENABLED(CONFIG_SUPPORT_SENSOR_FOLD)
struct sensor_fold_state {
	int64_t ts;
	int state; // 0: unfold, 1: close(fold)
};
#endif

/* Main struct containing all the data */
struct adsp_data {
	struct device *adsp;
	struct device *sensor_device[MSG_SENSOR_MAX];
	struct device_attribute **sensor_attr[MSG_SENSOR_MAX];
	struct sock *adsp_skt;
	int32_t *msg_buf[MSG_SENSOR_MAX];
	unsigned int ready_flag[MSG_TYPE_MAX];
	bool sysfs_created[MSG_SENSOR_MAX];
	struct mutex prox_factory_mutex;
	struct mutex light_factory_mutex;
	struct mutex accel_factory_mutex;
	struct mutex remove_sysfs_mutex;
	struct mutex sensors_dump_mutex;
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
	struct mutex vir_optic_factory_mutex;
#endif
#if IS_ENABLED(CONFIG_FLIP_COVER_DETECTOR_FACTORY)
	struct mutex flip_cover_factory_mutex;
#endif
#if IS_ENABLED(CONFIG_BACKTAP_FACTORY)
	struct mutex backtap_factory_mutex;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
	struct mutex digital_hall_mutex;
#endif
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	struct notifier_block adsp_nb;
#endif
#ifdef CONFIG_VBUS_NOTIFIER
	struct notifier_block vbus_nb;
#endif
	int32_t fac_fstate;
#if IS_ENABLED(CONFIG_LIGHT_FACTORY)
	struct delayed_work light_init_work;
	bool light_factory_is_ready;
	char light_device_vendor[2][10];
	char light_device_name[2][10];
#endif
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_CALIBRATION)
	struct delayed_work light_cal_work;
	int32_t light_cal_result;
	int32_t light_cal1;
	int32_t light_cal2;
	int32_t copr_w;
	int32_t sub_light_cal_result;
	int32_t sub_light_cal1;
	int32_t sub_light_cal2;
	int32_t sub_copr_w;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_DDI_COPR_FOR_LIGHT_SENSOR)
	struct delayed_work light_copr_debug_work;
	int light_copr_debug_count;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AP_COPR_FOR_LIGHT_SENSOR)
	struct delayed_work light_copr_work;
	int32_t copr_data[15];
	bool has_pending_copr_data;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR)
	struct work_struct light_br_work;
#endif
#if IS_ENABLED(CONFIG_LIGHT_FACTORY)
	int32_t light_temp_reg;
	int32_t brightness_info[6];
	int32_t pre_bl_level[2];
	int32_t pre_panel_state[2];
	int32_t pre_screen_mode[2];
	int32_t pre_panel_idx;
	int32_t pre_display_idx;
	int32_t light_debug_info_cmd;
	int32_t hyst[4];
	int32_t brightness_resolution[2];
#endif
#if IS_ENABLED(CONFIG_SUPPORT_PROX_CALIBRATION)
	int32_t prox_cal;
	int32_t prox_sub_cal;
#endif
	struct delayed_work accel_cal_work;
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
	struct delayed_work sub_accel_cal_work;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_TRIPLE_6AXIS)
	struct delayed_work sub2_accel_cal_work;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_ACCEL_HIGHG) || defined(CONFIG_SUPPORT_ACCEL_HIGHG)
	struct delayed_work accel_highg_cal_work;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_SEAMLESS)
	struct delayed_work light_seamless_work;
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AK09973) || defined(CONFIG_SUPPORT_AK09973)
	struct delayed_work lsm6dso_selftest_stop_work;
	struct delayed_work dhall_cal_work;
#elif IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	struct delayed_work lsm6dso_selftest_stop_work;
#endif
	struct delayed_work pressure_cal_work;
	char press_device_vendor[DEVICE_INFO_LENGTH];
	char press_device_name[DEVICE_INFO_LENGTH];
#if IS_ENABLED(CONFIG_SUPPORT_SENSOR_FOLD)
	struct sensor_fold_state fold_state;
#endif
	uint32_t support_algo;
	bool restrict_mode;
	int turn_over_crash;
	bool send_probe_fail_msg;
	bool fssr_ignore;
	bool fssr_dump;
};

struct device_id_t {
	uint8_t device_id;
	char device_vendor[DEVICE_INFO_LENGTH];
	char device_name[DEVICE_INFO_LENGTH];
};

#ifdef CONFIG_SEC_FACTORY
int get_mag_raw_data(int32_t *raw_data);
#endif
int get_accel_raw_data(int32_t *raw_data);
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
int get_sub_accel_raw_data(int32_t *raw_data);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_TRIPLE_6AXIS)
int get_sub2_accel_raw_data(int32_t *raw_data);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_ACCEL_HIGHG)
int get_accel_highg_raw_data(int32_t *raw_data);
#endif
int adsp_get_sensor_data(int sensor_type);
int adsp_factory_register(unsigned int type,
	struct device_attribute *attributes[]);
int adsp_factory_unregister(unsigned int type);
#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE) || defined(CONFIG_VBUS_NOTIFIER) || IS_ENABLED(CONFIG_HALL_NOTIFIER)
struct adsp_data* adsp_ssc_core_register(unsigned int type,
	struct device_attribute *attributes[]);
struct adsp_data* adsp_ssc_core_unregister(unsigned int type);
#endif
int adsp_unicast(void *param, int param_size, u16 sensor_type,
	u32 portid, u16 msg_type);
int sensors_register(struct device **dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
void sensors_unregister(struct device *dev,
	struct device_attribute *attributes[]);
int core_factory_init(void);
void core_factory_exit(void);
#if IS_ENABLED(CONFIG_LSM6DSO_FACTORY)
int lsm6dso_accel_factory_init(void);
void lsm6dso_accel_factory_exit(void);
int lsm6dso_gyro_factory_init(void);
void lsm6dso_gyro_factory_exit(void);
#endif
#if IS_ENABLED(CONFIG_LSM6DSL_FACTORY)
int lsm6dsl_accel_factory_init(void);
void lsm6dsl_accel_factory_exit(void);
int lsm6dsl_gyro_factory_init(void);
void lsm6dsl_gyro_factory_exit(void);
#endif
void accel_factory_init_work(struct adsp_data *data);
void accel_cal_work_func(struct work_struct *work);
#if IS_ENABLED(CONFIG_AK09918_FACTORY)
int ak09918_factory_init(void);
void ak09918_factory_exit(void);
#endif
#if IS_ENABLED(CONFIG_LPS22HH_FACTORY)
int lps22hh_pressure_factory_init(void);
void lps22hh_pressure_factory_exit(void);
#elif IS_ENABLED(CONFIG_PRESSURE_FACTORY)
int pressure_factory_init(void);
void pressure_factory_exit(void);
#endif
#if IS_ENABLED(CONFIG_LIGHT_FACTORY)
int light_factory_init(void);
void light_factory_exit(void);
void light_init_work(struct adsp_data *data);
void light_init_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_PROX_FACTORY)
int prox_factory_init(void);
void prox_factory_exit(void);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_6AXIS)
int lsm6dso_sub_accel_factory_init(void);
void lsm6dso_sub_accel_factory_exit(void);
int lsm6dso_sub_gyro_factory_init(void);
void lsm6dso_sub_gyro_factory_exit(void);
void sub_accel_factory_init_work(struct adsp_data *data);
void sub_accel_cal_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_TRIPLE_6AXIS)
int sub2_accel_factory_init(void);
void sub2_accel_factory_exit(void);
int sub2_gyro_factory_init(void);
void sub2_gyro_factory_exit(void);
void sub2_accel_factory_init_work(struct adsp_data *data);
void sub2_accel_cal_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_ACCEL_HIGHG)
int accel_highg_factory_init(void);
void accel_highg_factory_exit(void);
void accel_highg_factory_init_work(struct adsp_data *data);
void accel_highg_cal_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_MAG)
int mag_sub_factory_init(void);
void mag_sub_factory_exit(void);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
int ak09970_factory_init(void);
void ak09970_factory_exit(void);
void lsm6dso_selftest_stop_work_func(struct work_struct *work);
bool lf_stream_read_cal_data(int axis, int *cal_data);
bool lf_stream_read_auto_cal_x_data(int *cal_data);
bool lf_stream_read_auto_cal_y_data(int *cal_data);
bool lf_stream_read_auto_cal_z_data(int *cal_data);	
int lf_stream_write_auto_cal_data(bool first_booting);
int lf_stream_write_cal_data(int axis, bool first_booting);
void lf_stream_set_mode_flag(bool is_first_boot, int *flag, umode_t *mode);
bool lf_stream_get_info_from_axis(int axis, int *index, int *nums);
int lf_stream_get_index_cal_data(int axis, int index);
bool lf_stream_get_cal_path(int axis, char *path);
bool lf_stream_get_data_buf(int axis, int32_t *data_buf);
#elif IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
void lsm6dso_selftest_stop_work_func(struct work_struct *work);
#endif

#if IS_ENABLED(CONFIG_SUPPORT_DEVICE_MODE)
void sns_device_mode_init_work(void);
void sns_flip_init_work(void);
#endif
#ifdef CONFIG_VBUS_NOTIFIER
void sns_vbus_init_work(void);
#endif
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
void sns_mpp_cover_init_work(void);
#endif

#if IS_ENABLED(CONFIG_BACKTAP_FACTORY)
int backtap_factory_init(void);
void backtap_factory_exit(void);
#endif

#if IS_ENABLED(CONFIG_FLIP_COVER_DETECTOR_FACTORY)
int flip_cover_detector_factory_init(void);
void flip_cover_detector_factory_exit(void);
void flip_cover_detector_cal_init_work(void);
#endif

#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_CALIBRATION)
void light_cal_init_work(struct adsp_data *data);
void light_cal_read_work_func(struct work_struct *work);
#endif /* CONFIG_SUPPORT_LIGHT_CALIBRATION */
#if IS_ENABLED(CONFIG_SUPPORT_DDI_COPR_FOR_LIGHT_SENSOR)
void light_copr_debug_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AP_COPR_FOR_LIGHT_SENSOR)
void light_copr_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_PROX_CALIBRATION)
void prox_cal_init_work(struct adsp_data *data);
void prox_send_cal_data(struct adsp_data *data, uint16_t prox_idx, bool fac_cal);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_PROX_POWER_ON_CAL)
void prox_factory_init_work(void);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
void digital_hall_factory_auto_cal_init_work(struct adsp_data *data);
int get_hall_angle_data(int32_t *raw_data);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_LIGHT_SEAMLESS)
void light_seamless_work_func(struct work_struct *work);
void light_seamless_init_work(struct adsp_data *data);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_DUAL_OPTIC)
int get_light_sidx(struct adsp_data *data);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_BRIGHTNESS_NOTIFY_FOR_LIGHT_SENSOR) && \
	IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2)
void light_brightness_work_func(struct work_struct *work);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_AK09973)
void dhall_cal_work_func(struct work_struct *work);
#endif
void pressure_factory_init_work(struct adsp_data *data);
void pressure_cal_work_func(struct work_struct *work);
struct adsp_data* adsp_get_struct_data(void);
bool sns_check_ignore_crash(void);
#endif /* __ADSP_SENSOR_H__ */
