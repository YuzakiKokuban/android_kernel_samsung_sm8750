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
#ifdef CONFIG_SEC_VIB_NOTIFIER
#include <linux/vibrator/sec_vibrator_notifier.h>
#endif
#define VENDOR "STM"
#if IS_ENABLED(CONFIG_LSM6DSV_FACTORY)
#define CHIP_ID "LSM6DSV"
#else
#define CHIP_ID "LSM6DSO"
#endif
#define ACCEL_ST_TRY_CNT 3
#define ACCEL_FACTORY_CAL_CNT 20
#define ACCEL_RAW_DATA_CNT 3
#define MAX_ACCEL_1G 2048

#define STM_LSM6DSO_INT_CHECK_RUNNING   4
#define STM_LSM6DSV_INT_CHECK_SKIP_IBI  5

#define MAX_MOTOR_STOP_TIMEOUT (8 * NSEC_PER_SEC)
/* Haptic Pattern A vibrate during 7ms.
 * touch, touchkey, operation feedback use this.
 * Do not call motor_workfunc when duration is 7ms.
 */
#define DURATION_SKIP	10
#define MOTOR_OFF	0
#define MOTOR_ON	1
#define CALL_VIB_IDX 65534

#ifdef CONFIG_SEC_VIB_NOTIFIER
struct accel_motor_data {
	struct notifier_block motor_nb;
	struct hrtimer motor_stop_timer;
	struct workqueue_struct *slpi_motor_wq;
	struct work_struct work_slpi_motor;
	atomic_t motor_state;
	int idx;
	int timeout;
};

struct accel_motor_data *pdata_motor;
#endif

struct accel_data {
	struct work_struct work_accel;
	struct workqueue_struct *accel_wq;
	struct adsp_data *dev_data;
	bool is_complete_cal;
	bool lpf_onoff;
	bool st_complete;
	atomic_t vib_state;
	int32_t raw_data[ACCEL_RAW_DATA_CNT];
	int32_t avg_data[ACCEL_RAW_DATA_CNT];
};

static struct accel_data *pdata;
static bool is_ignore_crash_factory = false;

static ssize_t accel_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t accel_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t sensor_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", "ADSP");
}

int get_accel_cal_data(struct adsp_data *data, int32_t *cal_data)
{
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return 0;
	}

	if (data->msg_buf[MSG_ACCEL][3] != ACCEL_RAW_DATA_CNT) {
		pr_err("[FACTORY] %s: Reading Bytes Num %d!!!\n",
			__func__, data->msg_buf[MSG_ACCEL][3]);
		return 0;
	}

	cal_data[0] = data->msg_buf[MSG_ACCEL][0];
	cal_data[1] = data->msg_buf[MSG_ACCEL][1];
	cal_data[2] = data->msg_buf[MSG_ACCEL][2];

	pr_info("[FACTORY] %s:  %d, %d, %d, %d\n", __func__,
		cal_data[0], cal_data[1], cal_data[2],
		data->msg_buf[MSG_ACCEL][3]);

	return data->msg_buf[MSG_ACCEL][3];
}

void set_accel_cal_data(struct adsp_data *data)
{
	uint8_t cnt = 0;

	pr_info("[FACTORY] %s:  %d, %d, %d\n", __func__, pdata->avg_data[0],
		pdata->avg_data[1], pdata->avg_data[2]);
	adsp_unicast(pdata->avg_data, sizeof(pdata->avg_data),
		MSG_ACCEL, 0, MSG_TYPE_SET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_SET_CAL_DATA] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_CAL_DATA] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else if (data->msg_buf[MSG_ACCEL][0] != ACCEL_RAW_DATA_CNT) {
		pr_err("[FACTORY] %s: Write Bytes Num %d!!!\n",
			__func__, data->msg_buf[MSG_ACCEL][0]);
	}
}

static ssize_t accel_calibration_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t cal_data[ACCEL_RAW_DATA_CNT] = {0, };
	int ret = 0;

	mutex_lock(&data->accel_factory_mutex);
	ret = get_accel_cal_data(data, cal_data);
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
		pr_err("[FACTORY] %s: get_accel_cal_data fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d\n", 0, 0, 0, 0);
	}
}

static ssize_t accel_calibration_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	pdata->dev_data = data;
	if (sysfs_streq(buf, "0")) {
		mutex_lock(&data->accel_factory_mutex);
		memset(pdata->avg_data, 0, sizeof(pdata->avg_data));
		set_accel_cal_data(data);
		mutex_unlock(&data->accel_factory_mutex);
	} else {
		pdata->is_complete_cal = false;
		queue_work(pdata->accel_wq, &pdata->work_accel);
		while (pdata->is_complete_cal == false) {
			pr_info("[FACTORY] %s: In factory cal\n", __func__);
			msleep(20);
		}
		mutex_lock(&data->accel_factory_mutex);
		set_accel_cal_data(data);
		mutex_unlock(&data->accel_factory_mutex);
	}

	return size;
}

static void accel_work_func(struct work_struct *work)
{
	struct accel_data *data = container_of((struct work_struct *)work,
		struct accel_data, work_accel);
	int i;

	mutex_lock(&data->dev_data->accel_factory_mutex);
	memset(pdata->avg_data, 0, sizeof(pdata->avg_data));
	adsp_unicast(pdata->avg_data, sizeof(pdata->avg_data),
		MSG_ACCEL, 0, MSG_TYPE_SET_CAL_DATA);
	msleep(30); /* for init of bias */
	for (i = 0; i < ACCEL_FACTORY_CAL_CNT; i++) {
		msleep(20);
		get_accel_raw_data(pdata->raw_data);
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

void accel_cal_work_func(struct work_struct *work)
{
	struct adsp_data *data = container_of((struct delayed_work *)work,
		struct adsp_data, accel_cal_work);
	int ret = 0;

	mutex_lock(&data->accel_factory_mutex);
	ret = get_accel_cal_data(data, pdata->avg_data);
	mutex_unlock(&data->accel_factory_mutex);
	if (ret > 0) {
		pr_info("[FACTORY] %s: ret(%d)  %d, %d, %d\n", __func__, ret,
			pdata->avg_data[0],
			pdata->avg_data[1],
			pdata->avg_data[2]);

		mutex_lock(&data->accel_factory_mutex);
		set_accel_cal_data(data);
		mutex_unlock(&data->accel_factory_mutex);
	} else {
		pr_err("[FACTORY] %s: get_accel_cal_data fail (%d)\n",
			__func__, ret);
	}
}

static ssize_t accel_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int retry = 0;
#if IS_ENABLED(CONFIG_SUPPORT_AK09973) || defined(CONFIG_SUPPORT_AK09973)
	int msg_buf = LSM6DSO_SELFTEST_TRUE;

	adsp_unicast(&msg_buf, sizeof(msg_buf),
		MSG_DIGITAL_HALL_ANGLE, 0, MSG_TYPE_OPTION_DEFINE);
#elif IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	int msg_buf = LSM6DSO_SELFTEST_TRUE;

	adsp_unicast(&msg_buf, sizeof(msg_buf),
		MSG_REF_ANGLE, 0, MSG_TYPE_OPTION_DEFINE);
#endif

	pdata->st_complete = false;
RETRY_ACCEL_SELFTEST:
	adsp_unicast(NULL, 0, MSG_ACCEL, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		msleep(26);

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		data->msg_buf[MSG_ACCEL][1] = -1;
#ifdef CONFIG_SEC_FACTORY
		panic("sensor force crash : accel selftest timeout\n");
#endif
	}

	pr_info("[FACTORY] %s : init = %d, result = %d, XYZ = %d, %d, %d, nXYZ = %d, %d, %d\n",
		__func__, data->msg_buf[MSG_ACCEL][0],
		data->msg_buf[MSG_ACCEL][1], data->msg_buf[MSG_ACCEL][2],
		data->msg_buf[MSG_ACCEL][3], data->msg_buf[MSG_ACCEL][4],
		data->msg_buf[MSG_ACCEL][5], data->msg_buf[MSG_ACCEL][6],
		data->msg_buf[MSG_ACCEL][7]);
	pr_info("[FACTORY] %s : pre/postP/postN [%d, %d, %d/%d, %d, %d/%d, %d, %d], comm_err_cnt %d/%d/%d\n",
		__func__, data->msg_buf[MSG_ACCEL][8],
		data->msg_buf[MSG_ACCEL][9], data->msg_buf[MSG_ACCEL][10],
		data->msg_buf[MSG_ACCEL][11], data->msg_buf[MSG_ACCEL][12],
		data->msg_buf[MSG_ACCEL][13], data->msg_buf[MSG_ACCEL][14],
		data->msg_buf[MSG_ACCEL][15], data->msg_buf[MSG_ACCEL][16],
		data->msg_buf[MSG_ACCEL][17], data->msg_buf[MSG_ACCEL][18],
		data->msg_buf[MSG_ACCEL][19]);

	if (data->msg_buf[MSG_ACCEL][1] == 1) {
		pr_info("[FACTORY] %s : Pass - result = %d, retry = %d\n",
			__func__, data->msg_buf[MSG_ACCEL][1], retry);
	} else {
		data->msg_buf[MSG_ACCEL][1] = -5;
		pr_err("[FACTORY] %s : Fail - result = %d, retry = %d\n",
			__func__, data->msg_buf[MSG_ACCEL][1], retry);

		if (retry < ACCEL_ST_TRY_CNT) {
			retry++;
			msleep(50);
			cnt = 0;
			pr_info("[FACTORY] %s: retry\n", __func__);
			goto RETRY_ACCEL_SELFTEST;
		}
	}

	pdata->st_complete = true;

#if IS_ENABLED(CONFIG_SUPPORT_AK09973) || defined(CONFIG_SUPPORT_AK09973) ||\
	IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	schedule_delayed_work(&data->lsm6dso_selftest_stop_work, msecs_to_jiffies(300));
#endif
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d\n",
			data->msg_buf[MSG_ACCEL][1],
			(int)abs(data->msg_buf[MSG_ACCEL][2]),
			(int)abs(data->msg_buf[MSG_ACCEL][3]),
			(int)abs(data->msg_buf[MSG_ACCEL][4]),
			(int)abs(data->msg_buf[MSG_ACCEL][5]),
			(int)abs(data->msg_buf[MSG_ACCEL][6]),
			(int)abs(data->msg_buf[MSG_ACCEL][7]));
}

static ssize_t accel_raw_data_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t raw_data[ACCEL_RAW_DATA_CNT] = {0, };
	static int32_t prev_raw_data[ACCEL_RAW_DATA_CNT] = {0, };
	int ret = 0;
#ifdef CONFIG_SEC_FACTORY
	static int same_cnt = 0;
#endif

	if (pdata->st_complete == false) {
		pr_info("[FACTORY] %s: selftest is running\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			raw_data[0], raw_data[1], raw_data[2]);
	}

	mutex_lock(&data->accel_factory_mutex);
	ret = get_accel_raw_data(raw_data);
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
			panic("sensor force crash : accel raw_data stuck\n");
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

static ssize_t accel_reactive_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	bool success = false;
	int32_t msg_buf = 0;

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_GET_REGISTER);

	while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << MSG_ACCEL);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d\n", (int)success);
	}

	pr_info("[FACTORY]: %s - %d\n", __func__,
		data->msg_buf[MSG_ACCEL][0]);

	if (data->msg_buf[MSG_ACCEL][0] == 0)
		success = true;

	if (data->msg_buf[MSG_ACCEL][0] == STM_LSM6DSV_INT_CHECK_SKIP_IBI) {
		pr_info("[FACTORY]: %s - IBI mode. Ignore Interrupt Check Test\n", __func__);
		success = true;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", (int)success);
}

static ssize_t accel_reactive_store(struct device *dev,
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
		adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
			0, MSG_TYPE_GET_REGISTER);

		while (!(data->ready_flag[MSG_TYPE_GET_REGISTER] & 1 << MSG_ACCEL) &&
			cnt++ < TIMEOUT_CNT)
			usleep_range(500, 550);

		data->ready_flag[MSG_TYPE_GET_REGISTER] &= ~(1 << MSG_ACCEL);
		mutex_unlock(&data->accel_factory_mutex);

		if (cnt >= TIMEOUT_CNT) {
			pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
			return size;
		}

		if (data->msg_buf[MSG_ACCEL][0] == STM_LSM6DSO_INT_CHECK_RUNNING)
			pr_info("[FACTORY]: %s - STM_LSM6DSx_INT_CHECK_RUNNING\n", __func__);
		else if (data->msg_buf[MSG_ACCEL][0] == STM_LSM6DSV_INT_CHECK_SKIP_IBI)
			pr_info("[FACTORY]: %s - IBI mode. Ignore Interrupt Check Test\n", __func__);
		else
			pr_info("[FACTORY]: %s - Something wrong\n", __func__);
	}

	return size;
}

bool sns_check_ignore_crash(void)
{
	return is_ignore_crash_factory;
}
EXPORT_SYMBOL(sns_check_ignore_crash);

static ssize_t accel_lowpassfilter_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int32_t msg_buf;

	if (sysfs_streq(buf, "1")) {
		msg_buf = 1;
		is_ignore_crash_factory = false;
	} else if (sysfs_streq(buf, "0")) {
		msg_buf = 0;
		is_ignore_crash_factory = false;
#ifdef CONFIG_SEC_FACTORY
	} else if (sysfs_streq(buf, "2")) {
		msg_buf = 2;
		is_ignore_crash_factory = true;
		pr_info("[FACTORY] %s: Pretest\n", __func__);
	} else if (sysfs_streq(buf, "3")) {
		msg_buf = 3;
		is_ignore_crash_factory = true;
		pr_info("[FACTORY] %s: Questt\n", __func__);
		return size;
#endif
	} else {
		is_ignore_crash_factory = false;
		pr_info("[FACTORY] %s: wrong value\n", __func__);
		return size;
	}

	mutex_lock(&data->accel_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_SET_ACCEL_LPF);

	while (!(data->ready_flag[MSG_TYPE_SET_ACCEL_LPF] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_ACCEL_LPF] &= ~(1 << MSG_ACCEL);
	mutex_unlock(&data->accel_factory_mutex);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return size;
	}

	pdata->lpf_onoff = (bool)data->msg_buf[MSG_ACCEL][0];

#if IS_ENABLED(CONFIG_LSM6DSV_FACTORY)
	pr_info("[FACTORY] %s: %d, 0x0A:%02x 0x0D:%02x 0x18:%02x\n", __func__,
		data->msg_buf[MSG_ACCEL][0], data->msg_buf[MSG_ACCEL][1],
		data->msg_buf[MSG_ACCEL][2], data->msg_buf[MSG_ACCEL][3]);
#else
	pr_info("[FACTORY] %s: %d, 0x0A:%02x 0x0D:%02x 0x10:%02x\n", __func__,
		data->msg_buf[MSG_ACCEL][0], data->msg_buf[MSG_ACCEL][1],
		data->msg_buf[MSG_ACCEL][2], data->msg_buf[MSG_ACCEL][3]);
#endif
	return size;
}

#ifdef CONFIG_SEC_VIB_NOTIFIER
uint64_t motor_stop_timeout(int timeout_ms)
{
	uint64_t timeout_ns = timeout_ms * NSEC_PER_MSEC;

	if (timeout_ns > MAX_MOTOR_STOP_TIMEOUT)
		return timeout_ns;
	else
		return MAX_MOTOR_STOP_TIMEOUT;
}

int ssc_motor_notify(struct notifier_block *nb,
	unsigned long enable, void *v)
{
	struct vib_notifier_context *vib = (struct vib_notifier_context *)v;
	uint64_t timeout_ns = 0;

	pr_info("[FACTORY] %s: %s, idx: %d timeout: %d\n",
		__func__, enable ? "ON" : "OFF", vib->index, vib->timeout);

	if(enable == 1) {
		pdata_motor->idx = vib->index;
		pdata_motor->timeout = vib->timeout;

		if (pdata_motor->idx == 0)
			timeout_ns = motor_stop_timeout(pdata_motor->timeout);
		else
			timeout_ns = MAX_MOTOR_STOP_TIMEOUT;

		if (atomic_read(&pdata_motor->motor_state) == MOTOR_OFF) {
			atomic_set(&pdata_motor->motor_state, MOTOR_ON);
			queue_work(pdata_motor->slpi_motor_wq,
				&pdata_motor->work_slpi_motor);
		} else {
			hrtimer_cancel(&pdata_motor->motor_stop_timer);
			if (pdata_motor->idx == CALL_VIB_IDX) {
				queue_work(pdata_motor->slpi_motor_wq,
					&pdata_motor->work_slpi_motor);
				return 0;
			}
			hrtimer_start(&pdata_motor->motor_stop_timer,
				ns_to_ktime(timeout_ns),
				HRTIMER_MODE_REL);
		}
	} else {
		if (pdata_motor->idx == CALL_VIB_IDX) {
			atomic_set(&pdata_motor->motor_state, MOTOR_OFF);
			queue_work(pdata_motor->slpi_motor_wq,
				&pdata_motor->work_slpi_motor); 
		} else
			pr_info("[FACTORY] %s: Not support OFF\n", __func__);
	}

	return 0;
}

static enum hrtimer_restart motor_stop_timer_func(struct hrtimer *timer)
{
	pr_info("[FACTORY] %s\n", __func__);
	atomic_set(&pdata_motor->motor_state, MOTOR_OFF);
	queue_work(pdata_motor->slpi_motor_wq, &pdata_motor->work_slpi_motor);

	return HRTIMER_NORESTART;
}

void slpi_motor_work_func(struct work_struct *work)
{
	int32_t msg_buf = 0;
	uint64_t timeout_ns = 0;
	if (pdata_motor->idx == 0)
		timeout_ns = motor_stop_timeout(pdata_motor->timeout);
	else
		timeout_ns = MAX_MOTOR_STOP_TIMEOUT;

	if (atomic_read(&pdata_motor->motor_state) == MOTOR_ON) {
		if (pdata_motor->idx != CALL_VIB_IDX)
			hrtimer_start(&pdata_motor->motor_stop_timer,
				ns_to_ktime(timeout_ns),
				HRTIMER_MODE_REL);
		msg_buf = 1;
	} else if (atomic_read(&pdata_motor->motor_state) == MOTOR_OFF) {
		if (pdata_motor->idx != CALL_VIB_IDX)
			hrtimer_cancel(&pdata_motor->motor_stop_timer);
		msg_buf = 0;
	} else {
		pr_info("[FACTORY] %s: invalid state %d\n",
			__func__, (int)atomic_read(&pdata_motor->motor_state));
	}

	pr_info("[FACTORY] %s: msg_buf = %d, idx/timeout = %d/%d\n",
		__func__, msg_buf, pdata_motor->idx,
		(int)(timeout_ns / NSEC_PER_MSEC));

	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_SET_ACCEL_MOTOR);
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	usleep_range(500, 550);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL_SUB,
		0, MSG_TYPE_SET_ACCEL_MOTOR);
#endif
}
#endif

static ssize_t accel_dhr_sensor_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	char ctrl1_xl = 0;
	uint8_t fullscale = 0;

	adsp_unicast(NULL, 0, MSG_ACCEL, 0, MSG_TYPE_GET_DHR_INFO);
	while (!(data->ready_flag[MSG_TYPE_GET_DHR_INFO] & 1 << MSG_ACCEL) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_DHR_INFO] &= ~(1 << MSG_ACCEL);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		ctrl1_xl = data->msg_buf[MSG_ACCEL][16];

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
	}
	pr_info("[FACTORY] %s: f/s %u\n", __func__, fullscale);

	return snprintf(buf, PAGE_SIZE, "\"FULL_SCALE\":\"%uG\"\n", fullscale);
}

static ssize_t accel_turn_over_crash_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);

	pr_info("[FACTORY] %s: %d, \n", __func__, data->turn_over_crash);
	return snprintf(buf, PAGE_SIZE, "%d\n", data->turn_over_crash);
}

static ssize_t accel_turn_over_crash_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int32_t msg_buf[2] = {0, };

	if (sysfs_streq(buf, "1")) {
		data->turn_over_crash = 1;
		msg_buf[1] = 1;
	} else if (sysfs_streq(buf, "2")) {
		data->turn_over_crash = 2;
		msg_buf[1] = 2;
	} else {
		data->turn_over_crash = 0;
		msg_buf[1] = 0;
	}

	adsp_unicast(msg_buf, sizeof(msg_buf), MSG_ACCEL,
		0, MSG_TYPE_OPTION_DEFINE);
	pr_info("[FACTORY] %s: %d, \n", __func__, msg_buf[1]);

	return size;
}

void vib_onoff(int onoff)
{
	int32_t msg_buf = MOTOR_OFF;
	if (atomic_read(&pdata->vib_state) == MOTOR_ON)
		msg_buf = MOTOR_ON;

	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL,
		0, MSG_TYPE_SET_ACCEL_MOTOR);
#ifdef CONFIG_SUPPORT_DUAL_6AXIS
	usleep_range(500, 550);
	adsp_unicast(&msg_buf, sizeof(int32_t), MSG_ACCEL_SUB,
		0, MSG_TYPE_SET_ACCEL_MOTOR);
#endif
}

static ssize_t vib_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int vib_onoff = (int)atomic_read(&pdata->vib_state);
	pr_info("[FACTORY] %s: %d\n", __func__, vib_onoff);
	return snprintf(buf, PAGE_SIZE, "%d\n", vib_onoff);
}

static ssize_t vib_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int32_t enable = -1;

	if (sysfs_streq(buf, "1"))
		enable = 1;
	else
		enable = 0;

	if (enable == 1) {
		if (atomic_read(&pdata->vib_state) == MOTOR_OFF) {
			atomic_set(&pdata->vib_state, MOTOR_ON);
			vib_onoff(MOTOR_ON);
		} else {
			pr_info("[FACTORY] %s: Current vib_state is MOTOR_ON\n", __func__);
		}
	} else {
		atomic_set(&pdata->vib_state, MOTOR_OFF);
		vib_onoff(MOTOR_OFF);
	}
	pr_info("[FACTORY] %s: %d\n", __func__, enable);
	return size;
}

static DEVICE_ATTR(name, 0444, accel_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, accel_vendor_show, NULL);
static DEVICE_ATTR(type, 0444, sensor_type_show, NULL);
static DEVICE_ATTR(calibration, 0664,
	accel_calibration_show, accel_calibration_store);
static DEVICE_ATTR(selftest, 0440,
	accel_selftest_show, NULL);
static DEVICE_ATTR(raw_data, 0444, accel_raw_data_show, NULL);
static DEVICE_ATTR(reactive_alert, 0664,
	accel_reactive_show, accel_reactive_store);
static DEVICE_ATTR(lowpassfilter, 0220,
	NULL, accel_lowpassfilter_store);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(dhr_sensor_info, 0444,
	accel_dhr_sensor_info_show, NULL);
#else
static DEVICE_ATTR(dhr_sensor_info, 0440,
	accel_dhr_sensor_info_show, NULL);
#endif
static DEVICE_ATTR(turn_over_crash, 0664,
	accel_turn_over_crash_show, accel_turn_over_crash_store);
static DEVICE_ATTR(vib_onoff, 0664,
	vib_onoff_show, vib_onoff_store);

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
	&dev_attr_turn_over_crash,
	&dev_attr_vib_onoff,
	NULL,
};

void accel_factory_init_work(struct adsp_data *data)
{
	schedule_delayed_work(&data->accel_cal_work, msecs_to_jiffies(8000));
}
EXPORT_SYMBOL(accel_factory_init_work);

int __init lsm6dso_accel_factory_init(void)
{
	adsp_factory_register(MSG_ACCEL, acc_attrs);
#ifdef CONFIG_SEC_VIB_NOTIFIER
	pdata_motor = kzalloc(sizeof(*pdata_motor), GFP_KERNEL);

	if (pdata_motor == NULL)
		return -ENOMEM;

	pdata_motor->motor_nb.notifier_call = ssc_motor_notify,
	pdata_motor->motor_nb.priority = 1,
	sec_vib_notifier_register(&pdata_motor->motor_nb);

	hrtimer_init(&pdata_motor->motor_stop_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pdata_motor->motor_stop_timer.function = motor_stop_timer_func;
	pdata_motor->slpi_motor_wq =
		create_singlethread_workqueue("slpi_motor_wq");

	if (pdata_motor->slpi_motor_wq == NULL) {
		pr_err("[FACTORY]: %s - could not create motor wq\n", __func__);
		kfree(pdata_motor);
		return -ENOMEM;
	}

	INIT_WORK(&pdata_motor->work_slpi_motor, slpi_motor_work_func);

	atomic_set(&pdata_motor->motor_state, MOTOR_OFF);
#endif
	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	pdata->accel_wq = create_singlethread_workqueue("accel_wq");
	INIT_WORK(&pdata->work_accel, accel_work_func);

	pdata->lpf_onoff = true;
	pdata->st_complete = true;
	atomic_set(&pdata->vib_state, MOTOR_OFF);
	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

void __exit lsm6dso_accel_factory_exit(void)
{
	adsp_factory_unregister(MSG_ACCEL);
#ifdef CONFIG_SEC_VIB_NOTIFIER
	if (atomic_read(&pdata_motor->motor_state) == MOTOR_ON)
		hrtimer_cancel(&pdata_motor->motor_stop_timer);

	if (pdata_motor != NULL && pdata_motor->slpi_motor_wq != NULL) {
		cancel_work_sync(&pdata_motor->work_slpi_motor);
		destroy_workqueue(pdata_motor->slpi_motor_wq);
		pdata_motor->slpi_motor_wq = NULL;
		kfree(pdata_motor);
	}
#endif
	pr_info("[FACTORY] %s\n", __func__);
}
