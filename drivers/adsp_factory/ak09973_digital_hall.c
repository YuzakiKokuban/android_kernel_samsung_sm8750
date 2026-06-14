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

#include <linux/init.h>
#include <linux/module.h>
#include "adsp.h"
#define VENDOR "AKM"
#define CHIP_ID "AK09973"
#define IDX_180_X 0
#define IDX_180_Y 1
#define IDX_180_Z 2
#define IDX_90_X 3
#define IDX_90_Y 4
#define IDX_90_Z 5
#define IDX_0_X 6
#define IDX_0_Y 7
#define IDX_0_Z 8
#define SPEC_180_X_IDX 3
#define SPEC_90_X_IDX 4
#define SPEC_0_X_IDX 5
#define SPEC_180_Y_IDX 6
#define SPEC_90_Y_IDX 7
#define SPEC_0_Y_IDX 8
#define SPEC_180_Z_IDX 9
#define SPEC_90_Z_IDX 10
#define SPEC_0_Z_IDX 11
#define SPEC_MIN_IDX 0
#define SPEC_MAX_IDX 1
#define PASS 0
#define FAIL (-1)
#define FAIL_SPECOUT (-2)
#define FAIL_STUCK (-3)
#define ST_SPEC_MASK 0xE000
#define DEGREE_180 180
#define DEGREE_90 90
#define DEGREE_0 0
#define FAC_CAL_DATA_NUM 9
#define HALL_ANGLE_SPEC 15
#define FLEX_LO 80
#define FLEX_COVER_HI 70

#if ENABLE_LF_STREAM 1

#ifdef CONFIG_AK09973_DIGITAL_HALL_TEST_FOR_ONLY_UML
#define ENABLE_MM_SEGMENT	0
#else
#define ENABLE_MM_SEGMENT	1
#endif
#if ENABLE_LF_STREAM
#define LF_STREAM_AUTO_CAL_X_PATH "/data/digital_hall_auto_cal_x"
#define LF_STREAM_AUTO_CAL_Y_PATH "/data/digital_hall_auto_cal_y"
#define LF_STREAM_AUTO_CAL_Z_PATH "/data/digital_hall_auto_cal_z"
#endif

static int32_t spec[12][2] = {
	{170, 180}, {80, 110}, {0, 10}, //ref angle 180, 90, 0
	{-39640, 39640}, {-39640, 39640}, {-39640, 39640}, // X 180, 90, 0
	{-39640, 39640}, {-39640, 39640}, {-39640, 39640}, // Y 180, 90, 0
	{-39640, 39640}, {-39640, 39640}, {-39640, 39640} // Z 180, 90, 0
};

static int32_t test_spec[10][2] = {
	{150, 170}, {40, 60}, //ref angle 160, 50
	{-39640, 39640}, {-39640, 39640}, // X 160, 50
	{-39640, 39640}, {-39640, 39640}, // Y 160, 50
	{-39640, 39640}, {-39640, 39640}, // Z 160, 50
	{145, 175}, {35, 65} //hall angle 160, 50
};

int32_t curr_angle;

#if ENABLE_LF_STREAM
struct lf_stream_data {
	int32_t ref_x[AUTO_CAL_DATA_NUM];
	int32_t ref_y[AUTO_CAL_DATA_NUM];
	int32_t ref_z[AUTO_CAL_DATA_NUM];
	int32_t flg_update;
};
static struct lf_stream_data *pdata;
#endif

struct autocal_data_force_update {
#ifdef CONFIG_SEC_FACTORY
	struct workqueue_struct *autocal_debug_wq;
	struct work_struct work_autocal_debug;
#endif
	struct hrtimer rftest_timer;
	struct work_struct work_rftest;
	struct workqueue_struct *rftest_wq;
	int32_t ref_x[AUTO_CAL_DATA_NUM];
	int32_t ref_y[AUTO_CAL_DATA_NUM];
	int32_t ref_z[AUTO_CAL_DATA_NUM];
	int32_t flg_update;
	int32_t min_angle;
	int32_t max_angle;
	int32_t init_angle;
	short rftest_timer_enabled;
	int32_t flex_low;
	int32_t flex_cover_hi;
#ifdef CONFIG_SEC_FACTORY
	uint8_t block_autocal;
#endif
};
static struct autocal_data_force_update *auto_cal_data;

#ifdef CONFIG_SEC_FACTORY
void autocal_debug_work_func(struct work_struct *work)
{
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_CAL_DATA);
}
#endif

static enum hrtimer_restart rftest_timer_func(struct hrtimer *timer)
{
	queue_work(auto_cal_data->rftest_wq, &auto_cal_data->work_rftest);
	hrtimer_forward_now(&auto_cal_data->rftest_timer,
		ns_to_ktime(2000 * NSEC_PER_MSEC));
	return HRTIMER_RESTART;
}

static void rftest_work_func(struct work_struct *work)
{
	int32_t hall_angle = 0;

	get_hall_angle_data(&hall_angle);

	if (hall_angle < auto_cal_data->min_angle)
		auto_cal_data->min_angle = hall_angle;
	if (hall_angle > auto_cal_data->max_angle)
		auto_cal_data->max_angle = hall_angle;

	pr_info("[FACTORY] %s - curr/min/max = %d/%d/%d\n",
		__func__, hall_angle,
		auto_cal_data->min_angle, auto_cal_data->max_angle);
}

static ssize_t digital_hall_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t digital_hall_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t digital_hall_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s - start", __func__);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_DIGITAL_HALL) &&
		cnt++ < TIMEOUT_CNT)
		msleep(26);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_DIGITAL_HALL);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0,0\n");
	}
/*
 * Meaning of data->msg_buf[MSG_DIGITAL_HALL][14] value. Refer sns_ak0997x_hal.h
 *
 * TLIMIT_NO_INVALID_ID                        (1 << 1)
 * TLIMIT_NO_RESET                             (1 << 2)
 * TLIMIT_NO_SET_SELFTEST                      (1 << 3)
 * TLIMIT_NO_CNT_CNTL2                         (1 << 4)
 * TLIMIT_NO_CNT_WAIT                          (1 << 5)
 * TLIMIT_NO_CNT_READ                          (1 << 6)
 * TLIMIT_NO_DATA_CHANGED                      (1 << 7)
 * TLIMIT_NO_SLF_RVHX                          (1 << 13)
 * TLIMIT_NO_SLF_RVHY                          (1 << 14)
 * TLIMIT_NO_SLF_RVHZ                          (1 << 15)
 * TLIMIT_NO_SLF_ST2                           (1 << 16)
 */
	pr_info("[FACTORY] I2C_ERROR: %d, ST_RES: %d, min: %d/%d/%d, max: %d/%d/%d, avg: %d/%d/%d, st: %d/%d/%d, err: %d\n",
		data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
		data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
		data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
		data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
		data->msg_buf[MSG_DIGITAL_HALL][8], data->msg_buf[MSG_DIGITAL_HALL][9],
		data->msg_buf[MSG_DIGITAL_HALL][10], data->msg_buf[MSG_DIGITAL_HALL][11],
		data->msg_buf[MSG_DIGITAL_HALL][12], data->msg_buf[MSG_DIGITAL_HALL][13],
		data->msg_buf[MSG_DIGITAL_HALL][14]);

	if (data->msg_buf[MSG_DIGITAL_HALL][1] == FAIL_STUCK)
		data->msg_buf[MSG_DIGITAL_HALL][1] = FAIL;

	if ((data->msg_buf[MSG_DIGITAL_HALL][14] & ST_SPEC_MASK) != 0)
		data->msg_buf[MSG_DIGITAL_HALL][1] = FAIL_SPECOUT;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
		data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
		data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
		data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
		data->msg_buf[MSG_DIGITAL_HALL][11], data->msg_buf[MSG_DIGITAL_HALL][12],
		data->msg_buf[MSG_DIGITAL_HALL][13]);
}

static ssize_t digital_hall_spec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		spec[0][0], spec[0][1], spec[1][0], spec[1][1], spec[2][0], spec[2][1],
		spec[3][0], spec[3][1], spec[4][0], spec[4][1], spec[5][0], spec[5][1],
		spec[6][0], spec[6][1], spec[7][0], spec[7][1], spec[8][0], spec[8][1],
		spec[9][0], spec[9][1], spec[10][0], spec[10][1], spec[11][0], spec[11][1]);
}

static ssize_t digital_hall_test_spec_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		test_spec[0][0], test_spec[0][1], test_spec[1][0], test_spec[1][1],
		test_spec[2][0], test_spec[2][1], test_spec[3][0], test_spec[3][1],
		test_spec[4][0], test_spec[4][1], test_spec[5][0], test_spec[5][1],
		test_spec[6][0], test_spec[6][1], test_spec[7][0], test_spec[7][1],
		test_spec[8][0], test_spec[8][1], test_spec[9][0], test_spec[9][1]);
}

static ssize_t digital_hall_ref_angle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t min, max, result;

	min = curr_angle - 10;
	max = curr_angle + 10;
	result = PASS;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	if (data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0] < min ||
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0] > max)
		result = FAIL;

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0], result);
}

static ssize_t digital_hall_read_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);
}

static ssize_t digital_hall_test_read_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int result = PASS;
	int hall_angle = 0;
	int min = curr_angle - HALL_ANGLE_SPEC;
	int max = curr_angle + HALL_ANGLE_SPEC;

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
                return snprintf(buf, PAGE_SIZE, "-1,-1,-1,-1,-1,-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][3],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][4],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][5],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8]);

	hall_angle = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2];

	if (hall_angle < min || hall_angle > max) {
		pr_info("[FACTORY] %s - %d (%d, %d)\n", __func__,
			hall_angle, min, max);
		result = FAIL;
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][6],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][7],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][8],
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][2],
		result);
}

static ssize_t digital_hall_test_read_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (sysfs_streq(buf, "50")) {
		curr_angle = 50;
	} else if (sysfs_streq(buf, "160")) {
		curr_angle = 160;
	} else {
		pr_err("[FACTORY] %s - wrong degree !!!\n", __func__);
		return size;
	}

	pr_info("[FACTORY] %s - Test read at degree %d\n",
		__func__, curr_angle);

	return size;
}

static ssize_t reset_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int reset_data[58] = { 0, };
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	/* reset */
	adsp_unicast(reset_data, sizeof(reset_data),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_REGISTER);

	/* read */
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);
}

static ssize_t check_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s\n", __func__);

	mutex_lock(&data->digital_hall_mutex);
	/* Try to backup auto cal table */
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < 10)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 10) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}


	auto_cal_data->flg_update = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, auto_cal_data->flg_update);


	if (auto_cal_data->flg_update) {
		memcpy(auto_cal_data->ref_x, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_y, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][20], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_z, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][39], sizeof(int32_t) * 19);
	}

#ifdef CONFIG_SEC_FACTORY
	/* Print mx, my, mz buffer in SSC_DAEMON log */
	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_CAL_DATA);
#endif
	return snprintf(buf, PAGE_SIZE, "%d\n", auto_cal_data->flg_update);
}

static ssize_t backup_restore_auto_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int new_value;
	int32_t auto_cal_buf[58] = { 0, };
	uint8_t cnt = 0;

	if (sysfs_streq(buf, "0"))
		new_value = 0;
	else if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		return size;

	pr_info("[FACTORY] %s: new_value %d\n", __func__, new_value);

	if (new_value) {
		mutex_lock(&data->digital_hall_mutex);
		adsp_unicast(NULL, 0, MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_GET_CAL_DATA);

		while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
			cnt++ < 3)
			msleep(30);

		data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);
		mutex_unlock(&data->digital_hall_mutex);

		if (cnt >= 3) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
			return size;
		}

		pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0]);

		if (!data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0])
			return size;

		auto_cal_data->flg_update = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
		memcpy(auto_cal_data->ref_x, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_y, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][20], sizeof(int32_t) * 19);
		memcpy(auto_cal_data->ref_z, &data->msg_buf[MSG_DIGITAL_HALL_ANGLE][39], sizeof(int32_t) * 19);

		pr_info("[FACTORY] %s: backup auto_cal\n", __func__);
		pr_info("[FACTORY] %s: %d/%d/%d/%d\n", __func__,
			auto_cal_data->flg_update, auto_cal_data->ref_x[18],
			auto_cal_data->ref_y[18], auto_cal_data->ref_z[18]);

#if IS_ENABLED(CONFIG_SUPPORT_DHALL_SWITCH)
		cnt = 0;
		pr_info("[FACTORY] %s: Saving bop/brp to registry\n", __func__);
		mutex_lock(&data->digital_hall_mutex);
		adsp_unicast(NULL, 0, MSG_DIGITAL_HALL, 0, MSG_TYPE_GET_CAL_DATA);

		while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_DIGITAL_HALL) &&
			cnt++ < 3)
			msleep(30);

		data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_DIGITAL_HALL);
		mutex_unlock(&data->digital_hall_mutex);

		if (cnt >= 3) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		} else {
			pr_info("[FACTORY] %s: bop/brp %d/%d\n", __func__,
				data->msg_buf[MSG_DIGITAL_HALL][0],
				data->msg_buf[MSG_DIGITAL_HALL][1]);
		}
#endif
#ifdef CONFIG_SEC_FACTORY
		queue_work(auto_cal_data->autocal_debug_wq,
			&auto_cal_data->work_autocal_debug);
#endif
	} else {
		if (auto_cal_data->flg_update == 0) {
			pr_info("[FACTORY] %s: flg_update is zero\n", __func__);
			return size;
		}
		auto_cal_buf[0] = auto_cal_data->flg_update;
		memcpy(&auto_cal_buf[1], auto_cal_data->ref_x, sizeof(int32_t) * 19);
		memcpy(&auto_cal_buf[20], auto_cal_data->ref_y, sizeof(int32_t) * 19);
		memcpy(&auto_cal_buf[39], auto_cal_data->ref_z, sizeof(int32_t) * 19);

		pr_info("[FACTORY] %s: restore auto_cal\n", __func__);
		pr_info("[FACTORY] %s: %d/%d/%d/%d\n", __func__,
			auto_cal_buf[0], auto_cal_buf[1],
			auto_cal_buf[20], auto_cal_buf[39]);
		adsp_unicast(auto_cal_buf, sizeof(auto_cal_buf),
			MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_REGISTER);
	}

	return size;
}

static ssize_t rf_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s - init %d, min %d, max %d\n", __func__,
		auto_cal_data->init_angle,
		auto_cal_data->min_angle,
		auto_cal_data->max_angle);

	return snprintf(buf, PAGE_SIZE,	"%d,%d,%d\n",
		auto_cal_data->init_angle,
		auto_cal_data->min_angle,
		auto_cal_data->max_angle);
}

static ssize_t rf_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int new_value;
	int32_t angle = 0;

	if (sysfs_streq(buf, "1"))
		new_value = 1;
	else
		new_value = 0;

	if (new_value == auto_cal_data->rftest_timer_enabled)
		return size;

	if (new_value == 1) {
		auto_cal_data->rftest_timer_enabled = 1;
		get_hall_angle_data(&angle);
		auto_cal_data->init_angle = angle;
		hrtimer_start(&auto_cal_data->rftest_timer,
			ns_to_ktime(2000 * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	} else {
		auto_cal_data->rftest_timer_enabled = 0;
		hrtimer_cancel(&auto_cal_data->rftest_timer);
		cancel_work_sync(&auto_cal_data->work_rftest);
		auto_cal_data->init_angle = 0;
		auto_cal_data->min_angle = 180;
		auto_cal_data->max_angle = 0;
	}

	pr_info("[FACTORY] %s - %d. init_angle %d\n", __func__,
		auto_cal_data->rftest_timer_enabled,
		auto_cal_data->init_angle);

	return size;
}

#if ENABLE_LF_STREAM
int lf_stream_get_index_cal_data(int axis, int index)
{
	int ret;
	if (axis == AUTO_CAL_AXIS_X)
		ret = pdata->ref_x[index];
	else if (axis == AUTO_CAL_AXIS_Y)
		ret = pdata->ref_y[index];
	else
		ret = pdata->ref_z[index];

	return ret;
}

static ssize_t lf_stream_reset_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int reset_data[58] = { 0, };
	uint8_t cnt = 0;

	mutex_lock(&data->digital_hall_mutex);
	/* reset */
	adsp_unicast(reset_data, sizeof(reset_data),
		MSG_LF_STREAM, 0, MSG_TYPE_SET_REGISTER);

	/* read */
	adsp_unicast(NULL, 0, MSG_LF_STREAM, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_LF_STREAM) &&
		cnt++ < 3)
		msleep(30);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_LF_STREAM);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, data->msg_buf[MSG_LF_STREAM][0]);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->msg_buf[MSG_LF_STREAM][0]);
}

void lf_stream_set_mode_flag(bool is_first_boot, int *flag, umode_t *mode)
{
	if (is_first_boot) {
		*flag = O_TRUNC | O_RDWR | O_CREAT;
		*mode = 0600;
	} else {
		*flag = O_RDWR;
		*mode = 0660;
	}
}

bool lf_stream_get_cal_path(int axis, char *path)
{
	bool ret = true;

	if (axis == AUTO_CAL_AXIS_X) {
		memcpy(path, LF_STREAM_AUTO_CAL_X_PATH, sizeof(LF_STREAM_AUTO_CAL_X_PATH));
	} else if (axis == AUTO_CAL_AXIS_Y) {
		memcpy(path, LF_STREAM_AUTO_CAL_Y_PATH, sizeof(LF_STREAM_AUTO_CAL_Y_PATH));
	} else if (axis == AUTO_CAL_AXIS_Z) {
		memcpy(path, LF_STREAM_AUTO_CAL_Z_PATH, sizeof(LF_STREAM_AUTO_CAL_Z_PATH));
	} else {
		ret = false;
	}
	return ret;
}

bool lf_stream_get_data_buf(int axis, int32_t *data_buf)
{
	bool ret = true;

	if (axis == AUTO_CAL_AXIS_X) {
		data_buf = pdata->ref_x;
	} else if (axis == AUTO_CAL_AXIS_Y) {
		data_buf = pdata->ref_y;
	} else if (axis == AUTO_CAL_AXIS_Z) {
		data_buf = pdata->ref_z;		
	} else {
		ret = false;
	}
	return ret;
}

int lf_stream_write_data(struct file *auto_cal_filp, int axis, char *write_buf, int32_t *data_buf)
{
	int ret;
	int data_len = AUTO_CAL_DATA_NUM;

	if (axis == AUTO_CAL_AXIS_X) {
		snprintf(write_buf, 1, "%d,", pdata->flg_update);
		data_len++;
	}
	snprintf(write_buf, AUTO_CAL_DATA_NUM,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4],
		data_buf[5], data_buf[6], data_buf[7], data_buf[8], data_buf[9],
		data_buf[10], data_buf[11], data_buf[12], data_buf[13],
		data_buf[14], data_buf[15], data_buf[16], data_buf[17],
		data_buf[18]);

	ret = vfs_write(auto_cal_filp, (char *)write_buf,
		data_len * sizeof(char), &auto_cal_filp->f_pos);

	if (ret < 0)
		pr_err("[FACTORY] %s: lf_stream auto_cal_x fd write:%d\n",
			__func__, ret);
	return ret;
}

int __mockable lf_stream_write_cal_data(int axis, bool first_booting)
{
	struct file *auto_cal_filp = NULL;
#if ENABLE_MM_SEGMENT
	mm_segment_t old_fs;
#endif
	int flag, ret = 0;
	umode_t mode = 0;
	int32_t *data_buf;
	char *auto_cal_path = NULL;
	char *write_buf = kzalloc(AUTO_CAL_FILE_BUF_LEN, GFP_KERNEL);
	
	lf_stream_set_mode_flag(first_booting, &flag, &mode);

#if ENABLE_MM_SEGMENT	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	if (!lf_stream_get_cal_path(axis, auto_cal_path)) {
		goto exit_fs;
	}

	if (!lf_stream_get_data_buf(axis, data_buf)) {
		goto exit_fs;
	}

	auto_cal_filp = filp_open(auto_cal_path, flag, mode);
	if (IS_ERR(auto_cal_filp)) {
		ret = PTR_ERR(auto_cal_filp);
		pr_err("[FACTORY] %s: open fail lf_stream auto_cal_x_filp:%d\n",
			__func__, ret);
		goto exit_fs;
	}

	ret = lf_stream_write_data(auto_cal_filp, axis, write_buf, data_buf);
	filp_close(auto_cal_filp, current->files);
exit_fs:
#if ENABLE_MM_SEGMENT
	set_fs(old_fs);
#endif
	kfree(write_buf);
	return ret;
}

int lf_stream_write_auto_cal_data(bool first_booting)
{
	int ret;
	
	ret = lf_stream_write_cal_data(AUTO_CAL_AXIS_X, first_booting);
	ret = lf_stream_write_cal_data(AUTO_CAL_AXIS_Y, first_booting);
	ret = lf_stream_write_cal_data(AUTO_CAL_AXIS_Z, first_booting);

	pr_info("[FACTORY] %s: saved", __func__);
	return ret;
}

static ssize_t lf_stream_auto_cal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s\n", __func__);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(NULL, 0, MSG_LF_STREAM, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_LF_STREAM) &&
		cnt++ < 3)
		msleep(30);
	
	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_LF_STREAM);
	mutex_unlock(&data->digital_hall_mutex);

	if (cnt >= 3) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE,
		"lf_stream autocal save failed\n");
	}

	pdata->flg_update = data->msg_buf[MSG_LF_STREAM][0];
	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, pdata->flg_update);

	if (!pdata->flg_update)
		return snprintf(buf, PAGE_SIZE, "flg_update is not true\n");

	memcpy(pdata->ref_x, &data->msg_buf[MSG_LF_STREAM][1], sizeof(int32_t) * 19);
	memcpy(pdata->ref_y, &data->msg_buf[MSG_LF_STREAM][20], sizeof(int32_t) * 19);
	memcpy(pdata->ref_z, &data->msg_buf[MSG_LF_STREAM][39], sizeof(int32_t) * 19);

	lf_stream_write_auto_cal_data(false);

	return snprintf(buf, PAGE_SIZE,
		"lf_stream autocal was saved in file system\n");
}

bool lf_stream_get_info_from_axis(int axis, int *index, int *nums)
{
	int ret = true;
	if (axis == AUTO_CAL_AXIS_X) {
		*index = AUTO_CAL_X_START;
		*nums = AUTO_CAL_DATA_NUM + 1;
	} else if (axis == AUTO_CAL_AXIS_Y) {
		*index = AUTO_CAL_Y_START;
		*nums = AUTO_CAL_DATA_NUM;
	} else if (axis == AUTO_CAL_AXIS_Z) {
		*index = AUTO_CAL_Z_START;
		*nums = AUTO_CAL_DATA_NUM;
	} else {
		ret = false;
	}
	return ret;
}

bool lf_stream_read_data(struct file *cal_filp, int axis, char *auto_cal_buf, int start_index, int data_nums, int *cal_data)
{
	int ret;

	ret = vfs_read(cal_filp, (char *)auto_cal_buf,
		AUTO_CAL_FILE_BUF_LEN * sizeof(char), &cal_filp->f_pos);
	if (ret < 0) {
		pr_err("[FACTORY] %s - read fail:%d\n", __func__, ret);
		return false;
	}

	if (axis == AUTO_CAL_AXIS_X)
		ret = sscanf(auto_cal_buf, "%9d,", &cal_data[0]);

	ret += sscanf(auto_cal_buf,
		"%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d,%9d",
		&cal_data[start_index + 1], &cal_data[start_index + 2],
		&cal_data[start_index + 3], &cal_data[start_index + 4],
		&cal_data[start_index + 5], &cal_data[start_index + 6],
		&cal_data[start_index + 7], &cal_data[start_index + 8],
		&cal_data[start_index + 9], &cal_data[start_index + 10],
		&cal_data[start_index + 11], &cal_data[start_index + 12],
		&cal_data[start_index + 13], &cal_data[start_index + 14],
		&cal_data[start_index + 15], &cal_data[start_index + 16],
		&cal_data[start_index + 17], &cal_data[start_index + 18],
		&cal_data[start_index + 19]);

	if (ret != data_nums) {
		pr_err("[FACTORY] %s - lf_stream_auto_cal_%d: sscanf fail %d\n",
			__func__, axis, ret);
		return false;
	}
	return true;
}

bool __mockable lf_stream_read_cal_data(int axis, int *cal_data)
{
	struct file *cal_filp = NULL;
#if ENABLE_MM_SEGMENT
	mm_segment_t old_fs;
#endif
	int ret = 0, start_index = 0;
	int data_nums;
	char auto_cal_path[35];
	char *auto_cal_buf = kzalloc(AUTO_CAL_FILE_BUF_LEN * sizeof(char),
		GFP_KERNEL);

	/* auto_cal X */
#if ENABLE_MM_SEGMENT
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif
	if(!lf_stream_get_cal_path(axis, auto_cal_path))
		goto exit_fs;

	if (!lf_stream_get_info_from_axis(axis, &start_index, &data_nums))
		goto exit_fs;

	cal_filp = filp_open(auto_cal_path, O_RDONLY, 0440);
	if (PTR_ERR(cal_filp) == -ENOENT || PTR_ERR(cal_filp) == -ENXIO) {
		pr_info("[FACTORY] %s - no lf_stream_auto_cal_x file\n",
			__func__);
#if ENABLE_MM_SEGMENT
		set_fs(old_fs);
#endif
		if (axis == AUTO_CAL_AXIS_X)
			lf_stream_write_auto_cal_data(true);
		ret = false;
		goto exit_free;
	} else if (IS_ERR(cal_filp)) {
		pr_err("[FACTORY]: %s - filp_open error: lf_stream_auto_cal_x\n",
			__func__);
		ret = false;
		goto exit_fs;
	} else {
		pr_info("[FACTORY] %s - already exist: lf_stream_auto_cal_x\n", __func__);
		ret = lf_stream_read_data(cal_filp, axis, auto_cal_buf, start_index, data_nums, cal_data);
	}
	filp_close(cal_filp, current->files);
exit_fs:
#if ENABLE_MM_SEGMENT
	set_fs(old_fs);
#endif
exit_free:
	kfree(auto_cal_buf);
	return ret;
}

bool lf_stream_read_auto_cal_x_data(int *cal_data)
{
	bool ret = false;

	ret = lf_stream_read_cal_data(AUTO_CAL_AXIS_X, cal_data);
	pdata->flg_update = cal_data[0];
	memcpy(pdata->ref_x, &cal_data[1], sizeof(int32_t) * AUTO_CAL_DATA_NUM);
	pr_info("[FACTORY] %s: flg_update=%d\n", __func__, pdata->flg_update);

	return ret;
}

bool lf_stream_read_auto_cal_y_data(int *cal_data)
{
	bool ret = false;

	ret = lf_stream_read_cal_data(AUTO_CAL_AXIS_Y, cal_data);
	memcpy(pdata->ref_y, &cal_data[AUTO_CAL_Y_START],
		sizeof(int32_t) * AUTO_CAL_DATA_NUM);

	return ret;
}

bool lf_stream_read_auto_cal_z_data(int *cal_data)
{
	bool ret = false;

	ret = lf_stream_read_cal_data(AUTO_CAL_AXIS_Z, cal_data);
	memcpy(pdata->ref_z, &cal_data[AUTO_CAL_Z_START],
		sizeof(int32_t) * AUTO_CAL_DATA_NUM);

	return ret;
}

static ssize_t lf_stream_auto_cal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	int cal_data[58] = { 0, };

	pr_info("[FACTORY] %s - lf_stream autocal restor start!!!\n", __func__);
	ret = lf_stream_read_auto_cal_x_data(cal_data);
	ret = lf_stream_read_auto_cal_y_data(cal_data);
	ret = lf_stream_read_auto_cal_z_data(cal_data);

	adsp_unicast(cal_data, sizeof(cal_data),
		MSG_LF_STREAM, 0, MSG_TYPE_SET_REGISTER);

	pr_info("[FACTORY] %s - lf_stream autocal was restored!!!\n", __func__);
	return size;
}
#endif

static ssize_t flexcover_thd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: Curr THD %d/%d\n", __func__,
		auto_cal_data->flex_low, auto_cal_data->flex_cover_hi);

	return snprintf(buf, PAGE_SIZE,	"%d,%d\n",
		auto_cal_data->flex_low, auto_cal_data->flex_cover_hi);
}

static ssize_t flexcover_thd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int cnt = 0;
	int32_t msg_buf[1];

	if (sscanf(buf, "%2d", &msg_buf[0]) != 1) {
		pr_err("[FACTORY]: %s - The number of data are wrong\n",
			__func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: msg_buf[0] = %d\n", __func__, msg_buf[0]);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_SET_THRESHOLD);

	while (!(data->ready_flag[MSG_TYPE_SET_THRESHOLD] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_THRESHOLD] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		auto_cal_data->flex_low = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
		auto_cal_data->flex_cover_hi = data->msg_buf[MSG_DIGITAL_HALL_ANGLE][1];
	}

	pr_info("[FACTORY] %s: New THD %d/%d\n", __func__,
		auto_cal_data->flex_low, auto_cal_data->flex_cover_hi);

	mutex_unlock(&data->digital_hall_mutex);

	return size;
}

#ifdef CONFIG_SEC_FACTORY
static ssize_t block_autocal_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: block_autocal = %u\n",
		__func__, auto_cal_data->block_autocal);

	return snprintf(buf, PAGE_SIZE,	"%u\n", auto_cal_data->block_autocal);
}

static ssize_t block_autocal_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int cnt = 0;
	int32_t msg_buf[1];

	if (sysfs_streq(buf, "0"))
		msg_buf[0] = 0;
	else if (sysfs_streq(buf, "1"))
		msg_buf[0] = 1;
	else
		return size;

	pr_info("[FACTORY] %s: msg_buf[0] = %d\n", __func__, msg_buf[0]);

	mutex_lock(&data->digital_hall_mutex);
	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_OPTION_DEFINE);

	while (!(data->ready_flag[MSG_TYPE_OPTION_DEFINE] & 1 << MSG_DIGITAL_HALL_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_OPTION_DEFINE] &= ~(1 << MSG_DIGITAL_HALL_ANGLE);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		auto_cal_data->block_autocal =
			(uint8_t)data->msg_buf[MSG_DIGITAL_HALL_ANGLE][0];
		pr_info("[FACTORY] %s: block_autocal = %u\n",
			__func__, auto_cal_data->block_autocal);
	}
	mutex_unlock(&data->digital_hall_mutex);

	return size;
}
#endif

static ssize_t digital_hall_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_DIGITAL_HALL, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_DIGITAL_HALL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_DIGITAL_HALL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		pr_info("[FACTORY] %s - ST: %02x, HX/HY/HZ: %d/%d/%d, CNTL: %02x/%02x/%02x, BOP/BRP: %d/%d\n",
			__func__,
			data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
			data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
			data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
			data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
			data->msg_buf[MSG_DIGITAL_HALL][8]);
	}

	return snprintf(buf, PAGE_SIZE, "ST: %02x, HX/HY/HZ: %d/%d/%d, CNTL: %02x/%02x/%02x, BOP/BRP: %d/%d\n",
		data->msg_buf[MSG_DIGITAL_HALL][0], data->msg_buf[MSG_DIGITAL_HALL][1],
		data->msg_buf[MSG_DIGITAL_HALL][2], data->msg_buf[MSG_DIGITAL_HALL][3],
		data->msg_buf[MSG_DIGITAL_HALL][4], data->msg_buf[MSG_DIGITAL_HALL][5],
		data->msg_buf[MSG_DIGITAL_HALL][6], data->msg_buf[MSG_DIGITAL_HALL][7],
		data->msg_buf[MSG_DIGITAL_HALL][8]);
}


static DEVICE_ATTR(name, 0444, digital_hall_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, digital_hall_vendor_show, NULL);
static DEVICE_ATTR(selftest, 0440, digital_hall_selftest_show, NULL);
static DEVICE_ATTR(spec, 0440, digital_hall_spec_show, NULL);
static DEVICE_ATTR(test_spec, 0440, digital_hall_test_spec_show, NULL);
static DEVICE_ATTR(ref_angle, 0440, digital_hall_ref_angle_show, NULL);
static DEVICE_ATTR(read_data, 0440, digital_hall_read_data_show, NULL);
static DEVICE_ATTR(test_read, 0660,
	digital_hall_test_read_show, digital_hall_test_read_store);
static DEVICE_ATTR(reset_auto_cal, 0440, reset_auto_cal_show, NULL);
static DEVICE_ATTR(check_auto_cal, 0440, check_auto_cal_show, NULL);
static DEVICE_ATTR(backup_restore_auto_cal, 0220,
	NULL, backup_restore_auto_cal_store);
#if ENABLE_LF_STREAM
static DEVICE_ATTR(lf_stream_reset_auto_cal, 0440,
	lf_stream_reset_auto_cal_show, NULL);
static DEVICE_ATTR(lf_stream_auto_cal, 0660,
	lf_stream_auto_cal_show, lf_stream_auto_cal_store);
#endif
static DEVICE_ATTR(rf_test, 0660, rf_test_show, rf_test_store);
static DEVICE_ATTR(flexcover_thd, 0660, flexcover_thd_show, flexcover_thd_store);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(block_autocal, 0660, block_autocal_show, block_autocal_store);
#endif
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(dhr_sensor_info, 0444, digital_hall_dhr_sensor_info_show, NULL);
#else
static DEVICE_ATTR(dhr_sensor_info, 0440, digital_hall_dhr_sensor_info_show, NULL);
#endif

static struct device_attribute *digital_hall_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_spec,
	&dev_attr_test_spec,
	&dev_attr_ref_angle,
	&dev_attr_read_data,
	&dev_attr_test_read,
	&dev_attr_reset_auto_cal,
	&dev_attr_check_auto_cal,
	&dev_attr_backup_restore_auto_cal,
#if ENABLE_LF_STREAM
	&dev_attr_lf_stream_auto_cal,
	&dev_attr_lf_stream_reset_auto_cal,
#endif
	&dev_attr_rf_test,
	&dev_attr_flexcover_thd,
#ifdef CONFIG_SEC_FACTORY
	&dev_attr_block_autocal,
#endif
	&dev_attr_dhr_sensor_info,
	NULL,
};

int __init ak09970_factory_init(void)
{
	adsp_factory_register(MSG_DIGITAL_HALL, digital_hall_attrs);
#if ENABLE_LF_STREAM
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
#endif
	auto_cal_data = kzalloc(sizeof(*auto_cal_data), GFP_KERNEL);

	hrtimer_init(&auto_cal_data->rftest_timer,
		CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	auto_cal_data->rftest_timer.function = rftest_timer_func;
	auto_cal_data->rftest_wq =
		create_singlethread_workqueue("hall_angle_rftest_wq");
	INIT_WORK(&auto_cal_data->work_rftest, rftest_work_func);

	auto_cal_data->init_angle = 0;
	auto_cal_data->min_angle = 180;
	auto_cal_data->max_angle = 0;
	auto_cal_data->rftest_timer_enabled = 0;
	auto_cal_data->flex_low = FLEX_LO;
	auto_cal_data->flex_cover_hi = FLEX_COVER_HI;

#ifdef CONFIG_SEC_FACTORY
	auto_cal_data->block_autocal = 0;
	auto_cal_data->autocal_debug_wq =
		create_singlethread_workqueue("autocal_dbg_wq");
	if (auto_cal_data->autocal_debug_wq == NULL) {
		pr_err("[FACTORY]: %s - could not create autocal_dbg_wq",
			__func__);
	}
	INIT_WORK(&auto_cal_data->work_autocal_debug, autocal_debug_work_func);
#endif
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

void __exit ak09970_factory_exit(void)
{
	if (auto_cal_data->rftest_timer_enabled == 1) {
		hrtimer_cancel(&auto_cal_data->rftest_timer);
		cancel_work_sync(&auto_cal_data->work_rftest);
	}
	destroy_workqueue(auto_cal_data->rftest_wq);

#ifdef CONFIG_SEC_FACTORY
	if (auto_cal_data->autocal_debug_wq)
		destroy_workqueue(auto_cal_data->autocal_debug_wq);
#endif
	adsp_factory_unregister(MSG_DIGITAL_HALL);
#if ENABLE_LF_STREAM
	if (pdata)
		kfree(pdata);
#endif
	if (auto_cal_data)
		kfree(auto_cal_data);

	pr_info("[FACTORY] %s\n", __func__);
}
