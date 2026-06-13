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
#define VENDOR "STM"

#define CHIP_ID "LSM6DSVW"
#define ACCEL_ST_TRY_CNT 3
#define ACCEL_FACTORY_CAL_CNT 20
#define ACCEL_RAW_DATA_CNT 3
#define MAX_ACCEL_1G 2048
#define PASS 0

#define STM_LSM6DSV_INT_CHECK_RUNNING   4
#define STM_LSM6DSV_INT_CHECK_SKIP_IBI  5

struct sub2_accel_data {
	struct work_struct work_accel;
	struct workqueue_struct *accel_wq;
	struct adsp_data *dev_data;
	bool is_complete_cal;
	bool lpf_onoff;
	bool st_complete;
	int32_t raw_data[ACCEL_RAW_DATA_CNT];
	int32_t avg_data[ACCEL_RAW_DATA_CNT];
};

static struct sub2_accel_data *pdata;

static ssize_t sub2_accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t sub2_accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t sensor_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", "ADSP");
}

int get_sub2_accel_cal_data(struct adsp_data *data, int32_t *cal_data)
{
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL_SUB2, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_ACCEL_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_ACCEL_SUB2);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return 0;
	}

	if (data->msg_buf[MSG_ACCEL_SUB2][3] != ACCEL_RAW_DATA_CNT) {
		pr_err("[FACTORY] %s: Reading Bytes Num %d!!!\n",
			__func__, data->msg_buf[MSG_ACCEL_SUB2][3]);
		return 0;
	}

	cal_data[0] = data->msg_buf[MSG_ACCEL_SUB2][0];
	cal_data[1] = data->msg_buf[MSG_ACCEL_SUB2][1];
	cal_data[2] = data->msg_buf[MSG_ACCEL_SUB2][2];

	pr_info("[FACTORY] %s:  %d, %d, %d, %d\n", __func__,
		cal_data[0], cal_data[1], cal_data[2],
		data->msg_buf[MSG_ACCEL_SUB2][3]);

	return data->msg_buf[MSG_ACCEL_SUB2][3];
}

void set_sub2_accel_cal_data(struct adsp_data *data)
{
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s(1):  %d, %d, %d\n", __func__, pdata->avg_data[0],
		pdata->avg_data[1], pdata->avg_data[2]);
	adsp_unicast(pdata->avg_data, sizeof(pdata->avg_data),
		MSG_ACCEL_SUB2, 0, MSG_TYPE_SET_CAL_DATA);
	pr_info("[FACTORY] %s(2):  %d, %d, %d\n", __func__, pdata->avg_data[0],
		pdata->avg_data[1], pdata->avg_data[2]);

	while (!(data->ready_flag[MSG_TYPE_SET_CAL_DATA] & 1 << MSG_ACCEL_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_CAL_DATA] &= ~(1 << MSG_ACCEL_SUB2);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else if (data->msg_buf[MSG_ACCEL_SUB2][0] != ACCEL_RAW_DATA_CNT) {
		pr_err("[FACTORY] %s: Write Bytes Num %d!!!\n",
			__func__, data->msg_buf[MSG_ACCEL_SUB2][0]);
	}
}

static ssize_t sub2_accel_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cal_data[ACCEL_RAW_DATA_CNT] = {0, };
	int ret = 0;

	mutex_lock(&data->accel_factory_mutex);
	ret = get_sub2_accel_cal_data(data, cal_data);
	mutex_unlock(&data->accel_factory_mutex);
	if (ret > 0) {
		pr_info("[FACTORY] %s:  %d, %d, %d\n", __func__,
			cal_data[0], cal_data[1], cal_data[2]);
		if (cal_data[0] == 0 && cal_data[1] == 0 && cal_data[2] == 0)
			return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
				0, 0, 0, 0);
		else
			return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n",
				true, cal_data[0], cal_data[1], cal_data[2]);
	} else {
		pr_err("[FACTORY] %s: get_sub2_accel_cal_data fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n", 0, 0, 0, 0);
	}
}

static ssize_t sub2_accel_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	pdata->dev_data = data;
	if (sysfs_streq(buf, "0")) {
		mutex_lock(&data->accel_factory_mutex);
		memset(pdata->avg_data, 0, sizeof(pdata->avg_data));
		set_sub2_accel_cal_data(data);
		mutex_unlock(&data->accel_factory_mutex);
	} else {
		pdata->is_complete_cal = false;
		queue_work(pdata->accel_wq, &pdata->work_accel);
		while (pdata->is_complete_cal == false) {
			pr_info("[FACTORY] %s: In factory cal\n", __func__);
			msleep(20);
		}
		mutex_lock(&data->accel_factory_mutex);
		set_sub2_accel_cal_data(data);
		mutex_unlock(&data->accel_factory_mutex);
	}

	return size;
}

static void sub2_accel_work_func(struct work_struct *work)
{
	struct sub2_accel_data *data = container_of((struct work_struct *)work,
		struct sub2_accel_data, work_accel);
	int i;

	mutex_lock(&data->dev_data->accel_factory_mutex);
	memset(pdata->avg_data, 0, sizeof(pdata->avg_data));
	adsp_unicast(pdata->avg_data, sizeof(pdata->avg_data),
		MSG_ACCEL_SUB2, 0, MSG_TYPE_SET_CAL_DATA);
	msleep(30); /* for init of bias */
	for (i = 0; i < ACCEL_FACTORY_CAL_CNT; i++) {
		msleep(20);
		get_sub2_accel_raw_data(pdata->raw_data);
		pdata->avg_data[0] += pdata->raw_data[0];
		pdata->avg_data[1] += pdata->raw_data[1];
		pdata->avg_data[2] += pdata->raw_data[2];
		pr_info("[FACTORY] %s:  %d, %d, %d\n", __func__,
			pdata->raw_data[0], pdata->raw_data[1],
			pdata->raw_data[2]);
	}

	for (i = 0; i < ACCEL_RAW_DATA_CNT; i++) {
		pdata->avg_data[i] /= ACCEL_FACTORY_CAL_CNT;
		pr_info("[FACTORY] %s: avg : %d\n",
			__func__, pdata->avg_data[i]);
	}

	if (pdata->avg_data[2] > 0)
		pdata->avg_data[2] -= MAX_ACCEL_1G;
	else if (pdata->avg_data[2] < 0)
		pdata->avg_data[2] += MAX_ACCEL_1G;

	mutex_unlock(&data->dev_data->accel_factory_mutex);
	pdata->is_complete_cal = true;
}

void sub2_accel_cal_work_func(struct work_struct *work)
{
	struct adsp_data *data = container_of((struct delayed_work *)work,
		struct adsp_data, sub2_accel_cal_work);
	int ret = 0;

	mutex_lock(&data->accel_factory_mutex);
	ret = get_sub2_accel_cal_data(data, pdata->avg_data);
	mutex_unlock(&data->accel_factory_mutex);
	if (ret > 0) {
		pr_info("[FACTORY] %s: ret(%d)  %d, %d, %d\n", __func__, ret,
			pdata->avg_data[0],
			pdata->avg_data[1],
			pdata->avg_data[2]);

		mutex_lock(&data->accel_factory_mutex);
		set_sub2_accel_cal_data(data);
		mutex_unlock(&data->accel_factory_mutex);
	} else {
		pr_err("[FACTORY] %s: get_accel_cal_data fail (%d)\n",
			__func__, ret);
	}
}

static ssize_t sub2_accel_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int retry = 0;
#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	int msg_buf = LSM6DSO_SELFTEST_TRUE;

	adsp_unicast(&msg_buf, sizeof(msg_buf),
		MSG_REF_ANGLE, 0, MSG_TYPE_OPTION_DEFINE);
#endif

	pdata->st_complete = false;
RETRY_ACCEL_SELFTEST:
	adsp_unicast(NULL, 0, MSG_ACCEL_SUB2, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_ACCEL_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		msleep(26);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_ACCEL_SUB2);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		data->msg_buf[MSG_ACCEL_SUB2][1] = -1;
#ifdef CONFIG_SEC_FACTORY
		panic("sensor force crash : sub2 accel selftest timeout\n");
#endif
	}

	pr_info("[FACTORY] %s : init = %d, result = %d, XYZ = %d, %d, %d, nXYZ = %d, %d, %d\n",
		__func__, data->msg_buf[MSG_ACCEL_SUB2][0],
		data->msg_buf[MSG_ACCEL_SUB2][1], data->msg_buf[MSG_ACCEL_SUB2][2],
		data->msg_buf[MSG_ACCEL_SUB2][3], data->msg_buf[MSG_ACCEL_SUB2][4],
		data->msg_buf[MSG_ACCEL_SUB2][5], data->msg_buf[MSG_ACCEL_SUB2][6],
		data->msg_buf[MSG_ACCEL_SUB2][7]);

	pr_info("[FACTORY] %s : pre/postP/postN [%d, %d, %d/%d, %d, %d/%d, %d, %d], comm_err_cnt %d/%d/%d\n",
		__func__, data->msg_buf[MSG_ACCEL_SUB2][8],
		data->msg_buf[MSG_ACCEL_SUB2][9], data->msg_buf[MSG_ACCEL_SUB2][10],
		data->msg_buf[MSG_ACCEL_SUB2][11], data->msg_buf[MSG_ACCEL_SUB2][12],
		data->msg_buf[MSG_ACCEL_SUB2][13], data->msg_buf[MSG_ACCEL_SUB2][14],
		data->msg_buf[MSG_ACCEL_SUB2][15], data->msg_buf[MSG_ACCEL_SUB2][16],
		data->msg_buf[MSG_ACCEL_SUB2][17], data->msg_buf[MSG_ACCEL_SUB2][18],
		data->msg_buf[MSG_ACCEL_SUB2][19]);

	if (data->msg_buf[MSG_ACCEL_SUB2][1] == 1) {
		pr_info("[FACTORY] %s : Pass - result = %d, retry = %d\n",
			__func__, data->msg_buf[MSG_ACCEL_SUB2][1], retry);
	} else {
		data->msg_buf[MSG_ACCEL_SUB2][1] = -5;
		pr_err("[FACTORY] %s : Fail - result = %d, retry = %d\n",
			__func__, data->msg_buf[MSG_ACCEL_SUB2][1], retry);

		if (retry < ACCEL_ST_TRY_CNT) {
			retry++;
			msleep(50);
			cnt = 0;
			pr_info("[FACTORY] %s: retry\n", __func__);
			goto RETRY_ACCEL_SELFTEST;
		}
	}

	pdata->st_complete = true;

#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	schedule_delayed_work(&data->lsm6dso_selftest_stop_work, msecs_to_jiffies(300));
#endif

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d\n",
			data->msg_buf[MSG_ACCEL_SUB2][1],
			(int)abs(data->msg_buf[MSG_ACCEL_SUB2][2]),
			(int)abs(data->msg_buf[MSG_ACCEL_SUB2][3]),
			(int)abs(data->msg_buf[MSG_ACCEL_SUB2][4]),
			(int)abs(data->msg_buf[MSG_ACCEL_SUB2][5]),
			(int)abs(data->msg_buf[MSG_ACCEL_SUB2][6]),
			(int)abs(data->msg_buf[MSG_ACCEL_SUB2][7]));
}

static ssize_t sub2_accel_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t raw_data[ACCEL_RAW_DATA_CNT] = {0, };
	static int32_t prev_raw_data[ACCEL_RAW_DATA_CNT] = {0, };
	int ret = 0;
#ifdef CONFIG_SEC_FACTORY
	static int same_cnt;
#endif

	if (pdata->st_complete == false) {
		pr_info("[FACTORY] %s: selftest is running\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			raw_data[0], raw_data[1], raw_data[2]);
	}

	mutex_lock(&data->accel_factory_mutex);
	ret = get_sub2_accel_raw_data(raw_data);
	mutex_unlock(&data->accel_factory_mutex);

#ifdef CONFIG_SEC_FACTORY
	pr_info("[FACTORY] %s: %d, %d, %d\n", __func__,
		raw_data[0], raw_data[1], raw_data[2]);

	if (prev_raw_data[0] == raw_data[0] &&
		prev_raw_data[1] == raw_data[1] &&
		prev_raw_data[2] == raw_data[2]) {
		same_cnt++;
		pr_info("[FACTORY] %s: same_cnt %d\n", __func__, same_cnt);
		if (same_cnt >= 20)
			panic("sensor force crash : sub2 accel raw_data stuck\n");
	} else
		same_cnt = 0;
#endif

	if (!ret) {
		memcpy(prev_raw_data, raw_data, sizeof(int32_t) * 3);
	} else if (!pdata->lpf_onoff) {
		pr_err("[FACTORY] %s: using prev data!!!\n", __func__);
		memcpy(raw_data, prev_raw_data, sizeof(int32_t) * 3);
	} else {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	}

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
		raw_data[0], raw_data[1], raw_data[2]);
}

static ssize_t sub2_accel_reactive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	bool success = false;
	int32_t msg_buf = 0;

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL_SUB2,
		0, MSG_TYPE_GET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << MSG_ACCEL_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << MSG_ACCEL_SUB2);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d\n", (int)success);
	}

	pr_info("[FACTORY]: %s - %d\n", __func__,
		data->msg_buf[MSG_ACCEL_SUB2][0]);

	if (data->msg_buf[MSG_ACCEL_SUB2][0] == 0)
		success = true;

	if (data->msg_buf[MSG_ACCEL_SUB2][0] == STM_LSM6DSV_INT_CHECK_SKIP_IBI) {
		pr_info("[FACTORY]: %s - IBI mode. Ignore Interrupt Check Test\n", __func__);
		success = true;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", (int)success);
}

static ssize_t sub2_accel_reactive_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t msg_buf;
	uint8_t cnt = 0;

	if (sysfs_streq(buf, "1"))
		pr_info("[FACTORY]: %s - on\n", __func__);
	else if (sysfs_streq(buf, "0"))
		pr_info("[FACTORY]: %s - off\n", __func__);
	else if (sysfs_streq(buf, "2")) {
		pr_info("[FACTORY]: %s - factory\n", __func__);
		msg_buf = 1;

		mutex_lock(&data->accel_factory_mutex);
		adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL_SUB2,
			0, MSG_TYPE_GET_REGISTER);

		while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << MSG_ACCEL_SUB2) &&
			cnt++ < TIMEOUT_CNT)
			usleep_range(500, 550);

		data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << MSG_ACCEL_SUB2);
		mutex_unlock(&data->accel_factory_mutex);

		if (cnt >= TIMEOUT_CNT) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
			return size;
		}

		if (data->msg_buf[MSG_ACCEL_SUB2][0] == STM_LSM6DSV_INT_CHECK_RUNNING)
			pr_info("[FACTORY]: %s - STM_LSM6DSV_INT_CHECK_RUNNING\n", __func__);
		else if (data->msg_buf[MSG_ACCEL_SUB2][0] == STM_LSM6DSV_INT_CHECK_SKIP_IBI)
			pr_info("[FACTORY]: %s - IBI mode. Ignore Interrupt Check Test\n", __func__);
		else
			pr_info("[FACTORY]: %s - Something wrong\n", __func__);
	}

	return size;
}

static ssize_t sub2_accel_lowpassfilter_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t msg_buf;

	if (sysfs_streq(buf, "1")) {
		msg_buf = 1;
	} else if (sysfs_streq(buf, "0")) {
		msg_buf = 0;
#ifdef CONFIG_SEC_FACTORY
	} else if (sysfs_streq(buf, "2")) {
		msg_buf = 2;
		pr_info("[FACTORY] %s: Pretest\n", __func__);
#endif
	} else {
		pr_info("[FACTORY] %s: wrong value\n", __func__);
		return size;
	}

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL_SUB2,
		0, MSG_TYPE_SET_ACCEL_LPF);

	while (!(data->ready_flag[MSG_TYPE_SET_ACCEL_LPF] & 1 << MSG_ACCEL_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_ACCEL_LPF] &= ~(1 << MSG_ACCEL_SUB2);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return size;
	}

	pdata->lpf_onoff = (bool)data->msg_buf[MSG_ACCEL_SUB2][0];

#if IS_ENABLED(CONFIG_LSM6DSV_FACTORY)
	pr_info("[FACTORY] %s: %d, 0x0A:%02x 0x0D:%02x 0x18:%02x\n", __func__,
		data->msg_buf[MSG_ACCEL_SUB2][0], data->msg_buf[MSG_ACCEL_SUB2][1],
		data->msg_buf[MSG_ACCEL_SUB2][2], data->msg_buf[MSG_ACCEL_SUB2][3]);
#else
	pr_info("[FACTORY] %s: %d, 0x0A:%02x 0x0D:%02x 0x10:%02x\n", __func__,
		data->msg_buf[MSG_ACCEL_SUB2][0], data->msg_buf[MSG_ACCEL_SUB2][1],
		data->msg_buf[MSG_ACCEL_SUB2][2], data->msg_buf[MSG_ACCEL_SUB2][3]);
#endif

	return size;
}

static ssize_t sub2_accel_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	char ctrl1_xl = 0;
	uint8_t fullscale = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL_SUB2, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_ACCEL_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_ACCEL_SUB2);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	ctrl1_xl = data->msg_buf[MSG_ACCEL_SUB2][16];

	ctrl1_xl &= 0xC;

	switch (ctrl1_xl) {
	case 0xC:
		fullscale = 8;
		break;
	case 0x8:
		fullscale = 4;
		break;
	case 0x4:
		fullscale = 16;
		break;
	case 0:
		fullscale = 2;
		break;
	default:
		break;
	}

	pr_info("[FACTORY] %s: f/s %u\n", __func__, fullscale);

	return snprintf(buf, PAGE_SIZE, "\"FULL_SCALE\":\"%uG\"\n", fullscale);
}

#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
static ssize_t ref_angle_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t result = PASS;

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(NULL, 0, MSG_REF_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_REF_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_REF_ANGLE);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_REF_ANGLE][0],
		data->msg_buf[MSG_REF_ANGLE][1],
		data->msg_buf[MSG_REF_ANGLE][2],
		data->msg_buf[MSG_REF_ANGLE][3],
		data->msg_buf[MSG_REF_ANGLE][4],
		data->msg_buf[MSG_REF_ANGLE][5],
		data->msg_buf[MSG_REF_ANGLE][6],
		data->msg_buf[MSG_REF_ANGLE][7],
		data->msg_buf[MSG_REF_ANGLE][8]);

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",
		data->msg_buf[MSG_REF_ANGLE][0], result);
}

static ssize_t angle_read_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(NULL, 0, MSG_REF_ANGLE, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_REF_ANGLE) &&
		cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_REF_ANGLE);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	pr_info("[FACTORY] %s - st %d/%d, akm %d/%d, lf %d/%d, hall %d/%d/%d(uT)\n",
		__func__, data->msg_buf[MSG_REF_ANGLE][0],
		data->msg_buf[MSG_REF_ANGLE][1],
		data->msg_buf[MSG_REF_ANGLE][2],
		data->msg_buf[MSG_REF_ANGLE][3],
		data->msg_buf[MSG_REF_ANGLE][4],
		data->msg_buf[MSG_REF_ANGLE][5],
		data->msg_buf[MSG_REF_ANGLE][6],
		data->msg_buf[MSG_REF_ANGLE][7],
		data->msg_buf[MSG_REF_ANGLE][8]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_REF_ANGLE][0],
		data->msg_buf[MSG_REF_ANGLE][1],
		data->msg_buf[MSG_REF_ANGLE][2],
		data->msg_buf[MSG_REF_ANGLE][3],
		data->msg_buf[MSG_REF_ANGLE][4],
		data->msg_buf[MSG_REF_ANGLE][5],
		data->msg_buf[MSG_REF_ANGLE][6],
		data->msg_buf[MSG_REF_ANGLE][7],
		data->msg_buf[MSG_REF_ANGLE][8]);
}
#endif

static DEVICE_ATTR(name, 0444, sub2_accel_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, sub2_accel_vendor_show, NULL);
static DEVICE_ATTR(type, 0444, sensor_type_show, NULL);
static DEVICE_ATTR(calibration, 0664,
	sub2_accel_calibration_show, sub2_accel_calibration_store);
static DEVICE_ATTR(selftest, 0440,
	sub2_accel_selftest_show, NULL);
static DEVICE_ATTR(raw_data, 0444, sub2_accel_raw_data_show, NULL);
static DEVICE_ATTR(reactive_alert, 0664,
	sub2_accel_reactive_show, sub2_accel_reactive_store);
static DEVICE_ATTR(lowpassfilter, 0220,
	NULL, sub2_accel_lowpassfilter_store);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(dhr_sensor_info, 0444,
	sub2_accel_dhr_sensor_info_show, NULL);
#else
static DEVICE_ATTR(dhr_sensor_info, 0440,
	sub2_accel_dhr_sensor_info_show, NULL);
#endif
#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
static DEVICE_ATTR(ref_angle, 0444, ref_angle_show, NULL);
static DEVICE_ATTR(read_angle_data, 0444, angle_read_data_show, NULL);
#endif

static struct device_attribute *acc_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_type,
	&dev_attr_calibration,
	&dev_attr_selftest,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	&dev_attr_lowpassfilter,
	&dev_attr_dhr_sensor_info,
#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	&dev_attr_ref_angle,
	&dev_attr_read_angle_data,
#endif
	NULL,
};

void sub2_accel_factory_init_work(struct adsp_data *data)
{
	schedule_delayed_work(&data->sub2_accel_cal_work, msecs_to_jiffies(8000));
}

int __init sub2_accel_factory_init(void)
{
	adsp_factory_register(MSG_ACCEL_SUB2, acc_attrs);

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	pdata->accel_wq = create_singlethread_workqueue("sub2_accel_wq");
	INIT_WORK(&pdata->work_accel, sub2_accel_work_func);

	pdata->lpf_onoff = true;
	pdata->st_complete = true;
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

void __exit sub2_accel_factory_exit(void)
{
	adsp_factory_unregister(MSG_ACCEL_SUB2);
	pr_info("[FACTORY] %s\n", __func__);
}
