/* SPDX-License-Identifier: GPL-2.0
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SEC_SECURE_TOUCH_H_
#define _SEC_SECURE_TOUCH_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/input.h>
#include <linux/sysfs.h>
//#if defined(CONFIG_SEC_SYSFS)
//#include <linux/sec_sysfs.h>
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#endif
//#else
//extern struct class *sec_class;
//#endif

#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
#include <linux/hall/hall_ic_notifier.h>
#endif
#if IS_ENABLED(CONFIG_SUPPORT_SENSOR_FOLD)
#include <linux/sensor/sensors_core.h>
#endif

#define SECURE_TOUCH_DEVICE_FIRST	1
#define SECURE_TOUCH_DEVICE_SECOND	2

/*
 * folder open = main screen = 0
 * folder close = sub screen = 1
 */
#define SECURE_TOUCH_MAIN_DEV		0
#define SECURE_TOUCH_SUB_DEV		1

#define SECURE_TOUCH_FOLDER_OPEN	0
#define SECURE_TOUCH_FOLDER_CLOSE	1

#define SECURE_TOUCH_DEV_NAME	"ss_touch"

struct sec_touch_driver {
	struct list_head list;
	struct device *dev;
	int drv_number;
	int (*enable)(void *drv_data);
	int (*disable)(void *drv_data);
	int (*status)(void *drv_data);
	int (*irq_handle)(void *drv_data);
	void *drv_data;
	struct kobject *kobj;
	int enabled;
	int registered;
	int is_running;
};

struct sec_secure_touch {
	struct list_head list_head;
	int secure_enabled;
	void *data;
	struct platform_device *pdev;
	struct device *device;
#if IS_ENABLED(CONFIG_SEC_INPUT_MULTI_DEVICE)
	struct delayed_work folder_work;
#endif
	int hall_ic;
	struct mutex lock;
	struct notifier_block nb;
	struct notifier_block nb_ssh;
	struct sec_touch_driver touch_driver[2];
	int device_number;
	int current_device;
};

int sec_secure_touch_set_device(struct sec_secure_touch *data, int dev_num);
void sec_secure_touch_sysfs_notify(struct sec_secure_touch *data);
struct sec_touch_driver *sec_secure_touch_register(void *drv_data, struct device *dev, int dev_num, struct kobject *kobj);
void sec_secure_touch_unregister(int dev_num);

int sec_secure_touch_init(void);
void sec_secure_touch_exit(void);

extern void hall_ic_register_notify(struct notifier_block *nb);
extern void hall_ic_unregister_notify(struct notifier_block *nb);
#endif
