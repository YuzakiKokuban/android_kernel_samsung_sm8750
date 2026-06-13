/*
 *  Copyright (C) 2024, Samsung Electronics Co. Ltd. All Rights Reserved.
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
#define NAME "Back Tap"
#define VENDOR "Samsung"

/* 1: Double Tap
 * 2: Triple Tap
 * 3: Double and Triple Tap
 */
static int backtap_type = 3;
static int backtap_thd;

static ssize_t backtap_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", NAME);
}

static ssize_t backtap_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t backtap_peak_thd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: Curr THD %d\n", __func__, backtap_thd);

	return snprintf(buf, PAGE_SIZE,	"%d\n", backtap_thd);
}

static ssize_t backtap_peak_thd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int cnt = 0;
	int msg_buf;
	int ret = 0;

	ret = kstrtoint(buf, 10, &msg_buf);
	if (ret < 0) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}
	if (msg_buf < 0 || msg_buf > 2) {
		pr_err("[FACTORY] %s: Invalid value\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf);

	mutex_lock(&data->backtap_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int),
		MSG_BACKTAP, 0, MSG_TYPE_SET_THRESHOLD);

	while (!(data->ready_flag[MSG_TYPE_SET_THRESHOLD] & 1 << MSG_BACKTAP) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_SET_THRESHOLD] &= ~(1 << MSG_BACKTAP);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	else
		backtap_thd = data->msg_buf[MSG_BACKTAP][0];

	pr_info("[FACTORY] %s: New THD %d\n", __func__, backtap_thd);
	mutex_unlock(&data->backtap_factory_mutex);

	return size;
}

static ssize_t backtap_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: Curr Type %d\n", __func__, backtap_type);

	return snprintf(buf, PAGE_SIZE,	"%d\n", backtap_type);
}

static ssize_t backtap_type_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int cnt = 0;
	int msg_buf;
	int ret = 0;

	ret = kstrtoint(buf, 10, &msg_buf);
	if (ret < 0) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return -EINVAL;
	}
	if (msg_buf <= 0 || msg_buf >= 4) {
		pr_err("[FACTORY] %s: Invalid value\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: msg_buf = %d\n", __func__, msg_buf);

	mutex_lock(&data->backtap_factory_mutex);
	adsp_unicast(&msg_buf, sizeof(int),
		MSG_BACKTAP, 0, MSG_TYPE_OPTION_DEFINE);

	while (!(data->ready_flag[MSG_TYPE_OPTION_DEFINE] & 1 << MSG_BACKTAP) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_OPTION_DEFINE] &= ~(1 << MSG_BACKTAP);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	else
		backtap_type = data->msg_buf[MSG_BACKTAP][0];

	pr_info("[FACTORY] %s: New Type %d\n", __func__, backtap_type);
	mutex_unlock(&data->backtap_factory_mutex);

	return size;
}

static DEVICE_ATTR(name, 0444, backtap_name_show, NULL);
static DEVICE_ATTR(vendor, 0444, backtap_vendor_show, NULL);
static DEVICE_ATTR(backtap_peak_thd, 0660, backtap_peak_thd_show, backtap_peak_thd_store);
static DEVICE_ATTR(backtap_type, 0660, backtap_type_show, backtap_type_store);

static struct device_attribute *backtap_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_backtap_peak_thd,
	&dev_attr_backtap_type,
	NULL,
};

int __init backtap_factory_init(void)
{
	adsp_factory_register(MSG_BACKTAP, backtap_attrs);
	pr_info("[FACTORY] %s\n", __func__);
	return 0;
}

void __exit backtap_factory_exit(void)
{
	adsp_factory_unregister(MSG_BACKTAP);
	pr_info("[FACTORY] %s\n", __func__);
}
