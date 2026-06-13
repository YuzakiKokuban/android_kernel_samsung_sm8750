/*
 *  Copyright (C) 2020, Samsung Electronics Co. Ltd. All Rights Reserved.
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
#include <linux/of.h>
#include <linux/notifier.h>
#include "adsp.h"

enum {
	OFF = 0,
	ON = 1
};

enum {
	X = 0,
	Y = 1,
	Z = 2,
	AXIS_MAX
};

#define ATTACH "ATTACH"
#define DETACH "DETACH"
#define PASS "PASS"
#define FAIL "FAIL"

#define AXIS_SELECT X
#define THRESHOLD -500
#define DETACH_MARGIN 100
#define SATURATION_VALUE 4900
#define MAG_DELAY_MS 10

#define COVER_DETACH 0 // OPEN
#define COVER_ATTACH 1 // CLOSE
#define COVER_ATTACH_NFC_ACTIVE 2 // CLOSE
#define COVER_ATTACH_NFC_TAG_PRESENT 7 // CLOSE

static bool fcd_available;
static int fcd_dual_cal_matrix;

struct factory_cover_status_data {
	char cover_status[10];
	uint32_t axis_select;
	int32_t threshold;
	int32_t init[AXIS_MAX];
	int32_t attach[AXIS_MAX];
	int32_t attach_extremum[AXIS_MAX];
	int32_t detach[AXIS_MAX];
	char attach_result[10];
	char detach_result[10];
	char final_result[10];

	uint8_t factory_test_status;
	int32_t attach_diff;
	int32_t detach_diff;
	int32_t failed_attach_max;
	int32_t failed_attach_min;
	uint8_t saturation;
};

struct flip_cover_detector_data {
	struct hrtimer fcd_timer;
	struct workqueue_struct *fcd_wq;
	struct work_struct work_fcd;
	ktime_t poll_delay;
	struct delayed_work notifier_work;
	struct adsp_data *data;
};

struct factory_cover_status_data *factory_data;
struct flip_cover_detector_data *fcd_data;

static int nfc_cover_status = -1;
static uint32_t axis_update = AXIS_SELECT;
static int32_t threshold_update = THRESHOLD;

char sysfs_cover_status[10];

#if IS_ENABLED(CONFIG_FLIP_COVER_DETECTOR_NOTIFIER)
static BLOCKING_NOTIFIER_HEAD(fcd_nb_head);

int fcd_notifier_register(struct notifier_block *nb)
{
	if (!fcd_available) {
		pr_info("%s: fcd not available\n", __func__);
		return -1;
	} else {
		pr_info("%s\n", __func__);
		return blocking_notifier_chain_register(&fcd_nb_head, nb);		
	}
}
EXPORT_SYMBOL_GPL(fcd_notifier_register);

int fcd_notifier_unregister(struct notifier_block *nb)
{
	if (!fcd_available) {
		pr_info("%s: fcd not available\n", __func__);
		return -1;
	} else {
		pr_info("%s\n", __func__);
		return blocking_notifier_chain_unregister(&fcd_nb_head, nb);
	}
}
EXPORT_SYMBOL(fcd_notifier_unregister);

int fcd_notifier_call_chain(unsigned long val, void *v)
{
	if (!fcd_available) {
		pr_info("%s: fcd not available\n", __func__);
		return -1;
	} else {
		pr_info("%s - %lu\n", __func__, val);
		return blocking_notifier_call_chain(&fcd_nb_head, val, v);
	}
}
#endif

static void send_axis_threshold_settings(struct adsp_data *data, int axis, int threshold)
{
	uint8_t cnt = 0;
	uint16_t flip_cover_detector_idx = MSG_FLIP_COVER_DETECTOR;
	int32_t msg_buf[2];

	msg_buf[0] = axis;
	msg_buf[1] = threshold;

	mutex_lock(&data->flip_cover_factory_mutex);

	adsp_unicast(msg_buf, sizeof(msg_buf),
		flip_cover_detector_idx, 0, MSG_TYPE_SET_THRESHOLD);

	while (!(data->ready_flag[MSG_TYPE_SET_THRESHOLD] & 1 << flip_cover_detector_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(20000, 20000);

	data->ready_flag[MSG_TYPE_SET_THRESHOLD] &= ~(1 << flip_cover_detector_idx);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		axis_update = axis;
		threshold_update = threshold;
	}

	pr_info("[FACTORY] %s: axis=%d, threshold=%d\n", __func__, axis_update, threshold_update);

	mutex_unlock(&data->flip_cover_factory_mutex);
}

static void send_flip_cover_status_to_mag_sensor(struct adsp_data *data, int val)
{
	uint8_t cnt = 0;
	int32_t msg_buf[1];

	msg_buf[0] = val;

	mutex_lock(&data->flip_cover_factory_mutex);

	adsp_unicast(msg_buf, sizeof(msg_buf),
		MSG_MAG, 0, MSG_TYPE_SET_TEMPORARY_MSG);

	while (!(data->ready_flag[MSG_TYPE_SET_TEMPORARY_MSG] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(20000, 20000);

	data->ready_flag[MSG_TYPE_SET_TEMPORARY_MSG] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);

	pr_info("[FACTORY] %s: nfc_cover_status=%d set in mag sensor\n", __func__, nfc_cover_status);

	mutex_unlock(&data->flip_cover_factory_mutex);	
}

static void notify_fcd_status(struct work_struct *work)
{
#if IS_ENABLED(CONFIG_FLIP_COVER_DETECTOR_NOTIFIER)
	fcd_notifier_call_chain(nfc_cover_status, NULL);
#endif
}

static void send_nfc_cover_status(struct adsp_data *data, int val)
{
	uint8_t cnt = 0;
	uint16_t flip_cover_detector_idx = MSG_FLIP_COVER_DETECTOR;
	int32_t msg_buf[1];

	msg_buf[0] = val;

	mutex_lock(&data->flip_cover_factory_mutex);

	adsp_unicast(msg_buf, sizeof(msg_buf),
		flip_cover_detector_idx, 0, MSG_TYPE_OPTION_DEFINE);

	while (!(data->ready_flag[MSG_TYPE_OPTION_DEFINE] & 1 << flip_cover_detector_idx) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(20000, 20000);

	data->ready_flag[MSG_TYPE_OPTION_DEFINE] &= ~(1 << flip_cover_detector_idx);

	if (cnt >= TIMEOUT_CNT)
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	else
		nfc_cover_status = data->msg_buf[flip_cover_detector_idx][0];

	pr_info("[FACTORY] %s: nfc_cover_status=%d\n", __func__, nfc_cover_status);

	mutex_unlock(&data->flip_cover_factory_mutex);

	if (fcd_dual_cal_matrix)
		send_flip_cover_status_to_mag_sensor(data, val);

	schedule_delayed_work(&fcd_data->notifier_work, msecs_to_jiffies(0));
}

static void get_mag_cal_data_with_saturation(struct adsp_data *data, int *mag_data)
{
	uint8_t cnt = 0;

	adsp_unicast(NULL, 0, MSG_MAG, 0, MSG_TYPE_GET_CAL_DATA);

	while (!(data->ready_flag[MSG_TYPE_GET_CAL_DATA] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 500);

	data->ready_flag[MSG_TYPE_GET_CAL_DATA] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
	} else {
		mag_data[X] = data->msg_buf[MSG_MAG][0];
		mag_data[Y] = data->msg_buf[MSG_MAG][1];
		mag_data[Z] = data->msg_buf[MSG_MAG][2];
		factory_data->saturation = data->msg_buf[MSG_MAG][3];
	}

	pr_info("[FACTORY] %s: %d, %d, %d, %d\n", __func__,
		mag_data[0], mag_data[1], mag_data[2], factory_data->saturation);
}

void check_cover_detection_factory(int *mag_data, int axis_select)
{
	int axis = 0;

	if (!strcmp(factory_data->cover_status, DETACH)) {
		if (mag_data[axis_select] > factory_data->failed_attach_max) {
			factory_data->failed_attach_max = mag_data[axis_select];

			if (abs(factory_data->failed_attach_max - factory_data->init[axis_select])
						> abs(factory_data->failed_attach_min - factory_data->init[axis_select])) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					factory_data->attach[axis] = mag_data[axis];
				}
			}
		} else if (mag_data[axis_select] < factory_data->failed_attach_min) {
			factory_data->failed_attach_min = mag_data[axis_select];

			if (abs(factory_data->failed_attach_max - factory_data->init[axis_select])
						< abs(factory_data->failed_attach_min - factory_data->init[axis_select])) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					factory_data->attach[axis] = mag_data[axis];
				}
			}
		}

		pr_info("[FACTORY] %s: failed_attach_max=%d, failed_attach_min=%d\n", __func__,
						factory_data->failed_attach_max, factory_data->failed_attach_min);

		factory_data->attach_diff = mag_data[axis_select] - factory_data->init[axis_select];

		if (abs(factory_data->attach_diff) > factory_data->threshold) {
			snprintf(factory_data->cover_status, 10, ATTACH);
			snprintf(factory_data->attach_result, 10, PASS);
			for (axis = X; axis < AXIS_MAX; axis++) {
				factory_data->attach[axis] = mag_data[axis];
				factory_data->attach_extremum[axis] = mag_data[axis];
			}
			pr_info("[FACTORY] %s: COVER ATTACHED\n", __func__);
		}
	} else {
		if (factory_data->attach_diff > 0) {
			if (factory_data->saturation) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					mag_data[axis] = SATURATION_VALUE;
				}
			}

			if (mag_data[axis_select] > factory_data->attach_extremum[axis_select]) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					factory_data->attach_extremum[axis] = mag_data[axis];
					factory_data->detach[axis] = 0;
				}
			}
		} else {
			if (factory_data->saturation) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					mag_data[axis] = -SATURATION_VALUE;
				}
			}

			if (mag_data[axis_select] < factory_data->attach_extremum[axis_select]) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					factory_data->attach_extremum[axis] = mag_data[axis];
					factory_data->detach[axis] = 0;
				}
			}
		}

		factory_data->detach_diff = mag_data[axis_select] - factory_data->attach_extremum[axis_select];

		if (factory_data->attach_diff > 0) {
			if (mag_data[axis_select] < (factory_data->attach_extremum[axis_select] - DETACH_MARGIN)) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					factory_data->detach[axis] = mag_data[axis];
				}
			}

			if (factory_data->detach_diff < -factory_data->threshold) {
				snprintf(factory_data->cover_status, 10, DETACH);
				snprintf(factory_data->detach_result, 10, PASS);
				snprintf(factory_data->final_result, 10, PASS);
				factory_data->factory_test_status = OFF;
				pr_info("[FACTORY] %s: COVER_DETACHED\n", __func__);
			}
		} else {
			if (mag_data[axis_select] > (factory_data->attach_extremum[axis_select] + DETACH_MARGIN)) {
				for (axis = X; axis < AXIS_MAX; axis++) {
					factory_data->detach[axis] = mag_data[axis];
				}
			}

			if (factory_data->detach_diff > factory_data->threshold) {
				snprintf(factory_data->cover_status, 10, DETACH);
				snprintf(factory_data->detach_result, 10, PASS);
				snprintf(factory_data->final_result, 10, PASS);
				factory_data->factory_test_status = OFF;
				pr_info("[FACTORY] %s: COVER_DETACHED\n", __func__);
			}
		}
	}

	pr_info("[FACTORY1] %s: cover_status=%s, axis_select=%d, thd=%d, \
		x_init=%d, x_attach=%d, x_min_max=%d, x_detach=%d, \
		y_init=%d, y_attach=%d, y_min_max=%d, y_detach=%d, \
		z_init=%d, z_attach=%d, z_min_max=%d, z_detach=%d, \
		attach_result=%s, detach_result=%s, final_result=%s\n",
		__func__, factory_data->cover_status, factory_data->axis_select, factory_data->threshold,
		factory_data->init[X], factory_data->attach[X], factory_data->attach_extremum[X], factory_data->detach[X],
		factory_data->init[Y], factory_data->attach[Y], factory_data->attach_extremum[Y], factory_data->detach[Y],
		factory_data->init[Z], factory_data->attach[Z], factory_data->attach_extremum[Z], factory_data->detach[Z],
		factory_data->attach_result, factory_data->detach_result, factory_data->final_result);
}

static void fcd_work_func(struct work_struct *work)
{
	int mag_data[AXIS_MAX];

	if (factory_data->factory_test_status == ON) {
		get_mag_cal_data_with_saturation(fcd_data->data, mag_data);
		check_cover_detection_factory(mag_data, factory_data->axis_select);
	}
}

static enum hrtimer_restart fcd_timer_func(struct hrtimer *timer)
{
	queue_work(fcd_data->fcd_wq, &fcd_data->work_fcd);
	hrtimer_forward_now(&fcd_data->fcd_timer, fcd_data->poll_delay);

	if (factory_data->factory_test_status == ON)
		return HRTIMER_RESTART;
	else
		return HRTIMER_NORESTART;
}

static void factory_data_init(void)
{
	int mag_data[AXIS_MAX];
	int axis = 0;

	memset(factory_data, 0, sizeof(struct factory_cover_status_data));

	get_mag_cal_data_with_saturation(fcd_data->data, mag_data);

	factory_data->axis_select = axis_update;
	factory_data->threshold = (threshold_update > 0) ? threshold_update : threshold_update * (-1);

	for (axis = X; axis < AXIS_MAX; axis++) {
		factory_data->init[axis] = mag_data[axis];
		factory_data->attach[axis] = mag_data[axis];
	}

	factory_data->failed_attach_max = mag_data[factory_data->axis_select];
	factory_data->failed_attach_min = mag_data[factory_data->axis_select];

	snprintf(factory_data->cover_status, 10, DETACH);
	snprintf(factory_data->attach_result, 10, FAIL);
	snprintf(factory_data->detach_result, 10, FAIL);
	snprintf(factory_data->final_result, 10, FAIL);
}

static void enable_factory_test(int request)
{
	if (request == ON) {
		factory_data_init();
		factory_data->factory_test_status = ON;
		hrtimer_start(&fcd_data->fcd_timer, fcd_data->poll_delay, HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&fcd_data->fcd_timer);
		cancel_work_sync(&fcd_data->work_fcd);
		factory_data->factory_test_status = OFF;
	}
}

static ssize_t nfc_cover_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if (nfc_cover_status == COVER_ATTACH || nfc_cover_status == COVER_ATTACH_NFC_ACTIVE || nfc_cover_status == COVER_ATTACH_NFC_TAG_PRESENT) {
		snprintf(sysfs_cover_status, 10, "CLOSE");
	} else if (nfc_cover_status == COVER_DETACH)  {
		snprintf(sysfs_cover_status, 10, "OPEN");
	}

	pr_info("[FACTORY] %s: sysfs_cover_status=%s\n", __func__, sysfs_cover_status);

	return snprintf(buf, PAGE_SIZE, "%s\n",	sysfs_cover_status);
}

static ssize_t nfc_cover_status_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int status = 0;

	if (kstrtoint(buf, 10, &status)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	send_nfc_cover_status(data, status);

	pr_info("[FACTORY] %s: status=%d\n", __func__, status);

	return size;
}

static ssize_t factory_cover_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: status=%s, axis=%d, thd=%d, \
		x_init=%d, x_attach=%d, x_min_max=%d, x_detach=%d, \
		y_init=%d, y_attach=%d, y_min_max=%d, y_detach=%d, \
		z_init=%d, z_attach=%d, z_min_max=%d, z_detach=%d, \
		attach_result=%s, detach_result=%s, final_result=%s\n",
		__func__, factory_data->cover_status, factory_data->axis_select, factory_data->threshold,
		factory_data->init[X], factory_data->attach[X], factory_data->attach_extremum[X], factory_data->detach[X],
		factory_data->init[Y], factory_data->attach[Y], factory_data->attach_extremum[Y], factory_data->detach[Y],
		factory_data->init[Z], factory_data->attach[Z], factory_data->attach_extremum[Z], factory_data->detach[Z],
		factory_data->attach_result, factory_data->detach_result, factory_data->final_result);

	return snprintf(buf, PAGE_SIZE, "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s,%s\n",
		factory_data->cover_status, factory_data->axis_select, factory_data->threshold,
		factory_data->init[X], factory_data->attach[X], factory_data->attach_extremum[X], factory_data->detach[X],
		factory_data->init[Y], factory_data->attach[Y], factory_data->attach_extremum[Y], factory_data->detach[Y],
		factory_data->init[Z], factory_data->attach[Z], factory_data->attach_extremum[Z], factory_data->detach[Z],
		factory_data->attach_result, factory_data->detach_result, factory_data->final_result);
}

static ssize_t factory_cover_status_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int factory_test_request = -1;

	fcd_data->data = data;

	if (kstrtoint(buf, 10, &factory_test_request)) {
		pr_err("[FACTORY] %s: kstrtoint fail\n", __func__);
		return size;
	}

	pr_info("[FACTORY] %s: factory_test_request=%d\n", __func__, factory_test_request);

	if (factory_test_request == ON && factory_data->factory_test_status == OFF)
		enable_factory_test(ON);
	else if (factory_test_request == OFF && factory_data->factory_test_status == ON)
		enable_factory_test(OFF);

	return size;
}

static ssize_t axis_threshold_setting_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("[FACTORY] %s: axis=%d, threshold=%d\n", __func__, axis_update, threshold_update);

	return snprintf(buf, PAGE_SIZE, "%d,%d\n",	axis_update, threshold_update);
}

static ssize_t axis_threshold_setting_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	int ret;
	int axis, threshold;

	ret = sscanf(buf, "%d,%d", &axis, &threshold);

	if (ret != 2) {
		pr_err("[FACTORY] %s: Invalid values received, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	if (axis < 0 || axis >= AXIS_MAX) {
		pr_err("[FACTORY] %s: Invalid axis value received\n", __func__);
		return -EINVAL;
	}

	pr_info("[FACTORY] %s: axis=%d, threshold=%d\n", __func__, axis, threshold);

	send_axis_threshold_settings(data, axis, threshold);

	return size;
}

static ssize_t mag_cal_matrix_num_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adsp_data *data = dev_get_drvdata(dev);
	uint8_t cnt = 0;
	int cal_matrix_num;

	adsp_unicast(NULL, 0, MSG_MAG, 0, MSG_TYPE_GET_THRESHOLD);
	while (!(data->ready_flag[MSG_TYPE_GET_THRESHOLD] & 1 << MSG_MAG) &&
		cnt++ < TIMEOUT_CNT)
		usleep_range(500, 550);

	data->ready_flag[MSG_TYPE_GET_THRESHOLD] &= ~(1 << MSG_MAG);

	if (cnt >= TIMEOUT_CNT) {
		pr_err("[FACTORY] %s: Timeout!!!\n", __func__);
		cal_matrix_num = -1;
	} else {
		pr_info("[FACTORY] %s cal_matix_num = %d\n", __func__, data->msg_buf[MSG_MAG][0]);
		cal_matrix_num = data->msg_buf[MSG_MAG][0];
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", cal_matrix_num);
}

void flip_cover_detector_cal_init_work(void)
{
	if (fcd_dual_cal_matrix && nfc_cover_status > 0) {
		int32_t msg_buf[1];

		msg_buf[0] = nfc_cover_status;
		adsp_unicast(msg_buf, sizeof(msg_buf), MSG_MAG, 0, MSG_TYPE_SET_TEMPORARY_MSG);
		pr_info("[FACTORY] %s: nfc_cover_status=%d set in mag sensor after factory init\n",
			__func__, msg_buf[0]);
	}
}

static DEVICE_ATTR(nfc_cover_status, 0664, nfc_cover_status_show, nfc_cover_status_store);
static DEVICE_ATTR(factory_cover_status, 0664, factory_cover_status_show, factory_cover_status_store);
static DEVICE_ATTR(axis_threshold_setting, 0664, axis_threshold_setting_show, axis_threshold_setting_store);
static DEVICE_ATTR(cal_matrix_num, 0444, mag_cal_matrix_num_show, NULL);

static struct device_attribute *flip_cover_detector_attrs[] = {
	&dev_attr_nfc_cover_status,
	&dev_attr_factory_cover_status,
	&dev_attr_axis_threshold_setting,
	&dev_attr_cal_matrix_num,
	NULL,
};

void check_device_default_thd(struct device_node *np)
{
	int ret = of_property_read_u32(np, "fcd,attach_thd", &threshold_update);
	if (ret < 0) {
		pr_info("[FACTORY] %s: attach_thd not found, ret=%d\n", __func__, ret);
		threshold_update = THRESHOLD;
	}
	pr_info("[FACTORY] %s: %d\n", __func__, threshold_update);
}

void check_device_axis_update(struct device_node *np)
{
	int ret = of_property_read_u32(np, "fcd,axis", &axis_update);
	if (ret < 0) {
		pr_info("[FACTORY] %s: fcd,axis not found, ret=%d\n", __func__, ret);
		axis_update = AXIS_SELECT;
	}

	pr_info("[FACTORY] %s: %d\n", __func__, axis_update);
}

void check_fcd_dual_cal_matrix_support(struct device_node *np)
{
	int ret = of_property_read_u32(np, "fcd,dual_cal_matrix", &fcd_dual_cal_matrix);
	if (ret < 0) {
		pr_info("[FACTORY] %s: fcd,dual_cal_matrix not found, ret=%d\n", __func__, ret);
		fcd_dual_cal_matrix = 0;
	}

	pr_info("[FACTORY] %s: %d\n", __func__, fcd_dual_cal_matrix);
}

static bool check_flip_cover_detector_availability(void)
{
	struct device_node *np = of_find_node_by_name(NULL, "flip_cover_detector_sensor");

	if (np == NULL) {
		pr_info("[FACTORY] %s: false\n", __func__);
		fcd_available = false;
	} else {
		pr_info("[FACTORY] %s: true\n", __func__);
		fcd_available = true;

		check_fcd_dual_cal_matrix_support(np);
		check_device_default_thd(np);
		check_device_axis_update(np);
	}

	return fcd_available;
}

int flip_cover_detector_factory_init(void)
{
	fcd_available = check_flip_cover_detector_availability();

	if (!fcd_available)
		return 0;

	adsp_factory_register(MSG_FLIP_COVER_DETECTOR, flip_cover_detector_attrs);

	fcd_data = kzalloc(sizeof(*fcd_data), GFP_KERNEL);
	if (fcd_data == NULL) {
		pr_err("[FACTORY] %s: Memory allocation failed for fcd_data\n", __func__);
		return -ENOMEM;
	}

	factory_data = kzalloc(sizeof(*factory_data), GFP_KERNEL);
	if (factory_data == NULL) {
		pr_err("[FACTORY] %s: Memory allocation failed for factory_data\n", __func__);
		kfree(fcd_data);
		return -ENOMEM;
	}

	hrtimer_init(&fcd_data->fcd_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	fcd_data->fcd_timer.function = fcd_timer_func;

	fcd_data->fcd_wq = create_singlethread_workqueue("flip_cover_detector_wq");
	if (fcd_data->fcd_wq == NULL) {
		pr_err("[FACTORY] %s: could not create flip cover detector wq\n", __func__);
		kfree(fcd_data);
		kfree(factory_data);
		return -ENOMEM;
	}

	INIT_WORK(&fcd_data->work_fcd, fcd_work_func);
	fcd_data->poll_delay = ns_to_ktime(MAG_DELAY_MS * NSEC_PER_MSEC);

	INIT_DELAYED_WORK(&fcd_data->notifier_work, notify_fcd_status);

	snprintf(sysfs_cover_status, 10, "OPEN");

	pr_info("[FACTORY] %s\n", __func__);

	return 0;
}

void flip_cover_detector_factory_exit(void)
{
	if (!fcd_available)
		return;

	cancel_delayed_work(&fcd_data->notifier_work);

	adsp_factory_unregister(MSG_FLIP_COVER_DETECTOR);

	if (fcd_data != NULL) {
		if (factory_data->factory_test_status == ON)
			enable_factory_test(OFF);

		if (fcd_data->fcd_wq != NULL) {
			destroy_workqueue(fcd_data->fcd_wq);
			fcd_data->fcd_wq = NULL;
		}

		kfree(fcd_data);
		kfree(factory_data);
	}

	pr_info("[FACTORY] %s\n", __func__);
}
