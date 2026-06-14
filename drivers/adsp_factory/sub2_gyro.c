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
#define ST_PASS 1
#define ST_FAIL 0
#define STARTUP_BIT_FAIL 2
#define G_ZRL_DELTA_FAIL 4
#define SFLP_FAIL 6
#define SELFTEST_REVISED 1

static ssize_t sub2_gyro_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t sub2_gyro_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t selftest_revised_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", SELFTEST_REVISED);
}

static ssize_t gyro_power_off(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY]: %s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", 1);
}

static ssize_t gyro_power_on(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY]: %s\n", __func__);

	return snprintf(buf, PAGE_SIZE, "%d\n", 1);
}

static ssize_t sub2_gyro_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_GYRO_SUB2_TEMP, 0, MSG_TYPE_GET_RAW_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_RAW_DATA] & 1 << MSG_GYRO_SUB2_TEMP)
		&& cnt++ < TIMEOUT_CNT)
		msleep(20);

	data->ready_flag[MSG_TYPE_GET_RAW_DATA] &= ~(1 << MSG_GYRO_SUB2_TEMP);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-99\n");
	}

	pr_info("[FACTORY] %s: sub2_gyro_temp = %d\n", __func__,
		data->msg_buf[MSG_GYRO_SUB2_TEMP][0]);

	return snprintf(buf, PAGE_SIZE, "%d\n",
		data->msg_buf[MSG_GYRO_SUB2_TEMP][0]);
}

static ssize_t sub2_gyro_selftest_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int st_diff_res = ST_FAIL;
	int st_zro_res = ST_FAIL;
#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	int msg_buf = LSM6DSO_SELFTEST_TRUE;

	adsp_unicast(&msg_buf, sizeof(msg_buf),
		MSG_REF_ANGLE, 0, MSG_TYPE_OPTION_DEFINE);
#endif

	pr_info("[FACTORY] %s - start", __func__);
	adsp_unicast(NULL, 0, MSG_GYRO_SUB2, 0, MSG_TYPE_ST_SHOW_DATA);

	while (!(data->ready_flag[MSG_TYPE_ST_SHOW_DATA] & 1 << MSG_GYRO_SUB2) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(30000, 30100); /* 30 * 200 = 6 sec */

	data->ready_flag[MSG_TYPE_ST_SHOW_DATA] &= ~(1 << MSG_GYRO_SUB2);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
		schedule_delayed_work(&data->lsm6dso_selftest_stop_work, msecs_to_jiffies(300));
#endif
		return snprintf(buf, PAGE_SIZE,
			"0,0,0,0,0,0,0,0,0,0,0,0,%d,%d\n",
			ST_FAIL, ST_FAIL);
	}

	if (data->msg_buf[MSG_GYRO_SUB2][1] != 0) {
		pr_info("[FACTORY] %s - failed(%d, %d)\n", __func__,
			data->msg_buf[MSG_GYRO_SUB2][1],
			data->msg_buf[MSG_GYRO_SUB2][5]);

		pr_info("[FACTORY]: %s - %d,%d,%d\n", __func__,
			data->msg_buf[MSG_GYRO_SUB2][2],
			data->msg_buf[MSG_GYRO_SUB2][3],
			data->msg_buf[MSG_GYRO_SUB2][4]);

#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
		schedule_delayed_work(&data->lsm6dso_selftest_stop_work, msecs_to_jiffies(300));
#endif

		if (data->msg_buf[MSG_GYRO_SUB2][5] == G_ZRL_DELTA_FAIL)
			pr_info("[FACTORY] %s - ZRL Delta fail\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n",
			data->msg_buf[MSG_GYRO_SUB2][2],
			data->msg_buf[MSG_GYRO_SUB2][3],
			data->msg_buf[MSG_GYRO_SUB2][4]);
	} else {
		st_zro_res = ST_PASS;
	}

	if (!data->msg_buf[MSG_GYRO_SUB2][5])
		st_diff_res = ST_PASS;
	else if (data->msg_buf[MSG_GYRO_SUB2][5] == STARTUP_BIT_FAIL)
		pr_info("[FACTORY] %s - Gyro Start Up Bit fail\n", __func__);
	else if (data->msg_buf[MSG_GYRO_SUB2][5] == SFLP_FAIL) {
		pr_info("[FACTORY] %s - SFLP sanity test fail\n", __func__);
		st_diff_res = SFLP_FAIL;
	}

	pr_info("[FACTORY]: %s - %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		__func__,
		data->msg_buf[MSG_GYRO_SUB2][2], data->msg_buf[MSG_GYRO_SUB2][3],
		data->msg_buf[MSG_GYRO_SUB2][4], data->msg_buf[MSG_GYRO_SUB2][6],
		data->msg_buf[MSG_GYRO_SUB2][7], data->msg_buf[MSG_GYRO_SUB2][8],
		data->msg_buf[MSG_GYRO_SUB2][9], data->msg_buf[MSG_GYRO_SUB2][10],
		data->msg_buf[MSG_GYRO_SUB2][11], data->msg_buf[MSG_GYRO_SUB2][12],
		data->msg_buf[MSG_GYRO_SUB2][13], data->msg_buf[MSG_GYRO_SUB2][14],
		st_diff_res, st_zro_res);

#if 0 //IS_ENABLED(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL) || defined(CONFIG_SUPPORT_REF_ANGLE_WITHOUT_DIGITAL_HALL)
	schedule_delayed_work(&data->lsm6dso_selftest_stop_work, msecs_to_jiffies(300));
#endif

	return snprintf(buf, PAGE_SIZE,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		data->msg_buf[MSG_GYRO_SUB2][2], data->msg_buf[MSG_GYRO_SUB2][3],
		data->msg_buf[MSG_GYRO_SUB2][4], data->msg_buf[MSG_GYRO_SUB2][6],
		data->msg_buf[MSG_GYRO_SUB2][7], data->msg_buf[MSG_GYRO_SUB2][8],
		data->msg_buf[MSG_GYRO_SUB2][9], data->msg_buf[MSG_GYRO_SUB2][10],
		data->msg_buf[MSG_GYRO_SUB2][11], data->msg_buf[MSG_GYRO_SUB2][12],
		data->msg_buf[MSG_GYRO_SUB2][13], data->msg_buf[MSG_GYRO_SUB2][14],
		st_diff_res, st_zro_res);
}

static DEVICE_ATTR(name, 0444, sub2_gyro_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, sub2_gyro_vendor_show, NULL);
static DEVICE_ATTR(selftest, 0440, sub2_gyro_selftest_show, NULL);
static DEVICE_ATTR(power_on, 0444, gyro_power_on, NULL);
static DEVICE_ATTR(power_off, 0444, gyro_power_off, NULL);
static DEVICE_ATTR(temperature, 0440, sub2_gyro_temp_show, NULL);
static DEVICE_ATTR(selftest_revised, 0440, selftest_revised_show, NULL);

static struct device_attribute *gyro_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_selftest,
	&dev_attr_power_on,
	&dev_attr_power_off,
	&dev_attr_temperature,
	&dev_attr_selftest_revised,
	NULL,
};

int __init sub2_gyro_factory_init(void)
{
	adsp_factory_register(MSG_GYRO_SUB2, gyro_attrs);

	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

void __exit sub2_gyro_factory_exit(void)
{
	adsp_factory_unregister(MSG_GYRO_SUB2);

	pr_info("[FACTORY] %s\n", __func__);
}
