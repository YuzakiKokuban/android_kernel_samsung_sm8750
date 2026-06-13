// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct sec_secure_touch *g_ss_touch;

#include "sec_secure_touch.h"
#include <linux/notifier.h>
#include "sec_input.h"

int sec_secure_touch_set_device(struct sec_secure_touch *data, int dev_num)
{
	int number = dev_num - 1;
	int ret;

	if (data->touch_driver[number].registered == 0)
		return -ENODEV;

	if (data->device_number > 1)
		return -EBUSY;

	mutex_lock(&data->lock);
	ret = sysfs_create_link(&data->device->kobj, data->touch_driver[number].kobj, "secure");
	if (ret < 0) {
		mutex_unlock(&data->lock);
		return -EINVAL;
	}

	pr_info("%s: %s: create link ret:%d, %s\n", SECLOG, __func__, ret, data->touch_driver[number].kobj->name);

	data->touch_driver[number].enabled = 1;

	mutex_unlock(&data->lock);

	return ret;
}

struct sec_touch_driver *sec_secure_touch_register(void *drv_data, struct device *dev, int dev_num, struct kobject *kobj)
{
	struct sec_secure_touch *data = g_ss_touch;
	int number = dev_num - 1;

	if (!data) {
		pr_info("%s %s: null\n", SECLOG, __func__);
		return NULL;
	}

	pr_info("%s %s\n", SECLOG, __func__);

	if (dev_num < 1) {
		dev_err(&data->pdev->dev, "%s: invalid parameter:%d\n", __func__, dev_num);
		return NULL;
	}

	if (data->touch_driver[number].registered) {
		dev_info(&data->pdev->dev, "%s: already registered device number\n", __func__);
		return NULL;
	}

	pr_info("%s %s: name is %s\n", SECLOG, __func__, kobj->name);
	data->touch_driver[number].drv_number = dev_num;
	data->touch_driver[number].drv_data = drv_data;
	data->touch_driver[number].kobj = kobj;
	data->touch_driver[number].dev = dev;
	data->touch_driver[number].registered = 1;

	data->device_number++;

	sec_secure_touch_set_device(data, dev_num);

	return &data->touch_driver[number];
}
EXPORT_SYMBOL(sec_secure_touch_register);


void sec_secure_touch_unregister(int dev_num)
{
	struct sec_secure_touch *data = g_ss_touch;
	int number = dev_num - 1;

	pr_info("%s: %s\n", SECLOG, __func__);

	data->touch_driver[number].drv_number = 0;
	data->touch_driver[number].drv_data = NULL;
	data->touch_driver[number].kobj = NULL;
	data->touch_driver[number].registered = 0;

	data->device_number--;

}
EXPORT_SYMBOL(sec_secure_touch_unregister);

void sec_secure_touch_sysfs_notify(struct sec_secure_touch *data)
{
	if (!data)
		sysfs_notify(&g_ss_touch->device->kobj, NULL, "secure_touch");
	else
		sysfs_notify(&data->device->kobj, NULL, "secure_touch");

	dev_info(&g_ss_touch->pdev->dev, "%s\n", __func__);
}

static ssize_t dev_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_secure_touch *data = dev_get_drvdata(dev);

	if (!data)
		return -ENOMEM;

	return snprintf(buf, PAGE_SIZE, "%d", data->device_number);
}

static DEVICE_ATTR_RO(dev_count);

static struct attribute *sec_secure_touch_attrs[] = {
	&dev_attr_dev_count.attr,
	NULL,
};

static struct attribute_group sec_secure_touch_attr_group = {
	.attrs = sec_secure_touch_attrs,
};

#if IS_ENABLED(CONFIG_SEC_INPUT_MULTI_DEVICE)
static void sec_secure_touch_hall_ic_work(struct work_struct *work)
{
	struct sec_secure_touch *data = container_of(work, struct sec_secure_touch, folder_work.work);
	int ret;

	mutex_lock(&data->lock);

	if (data->hall_ic == SECURE_TOUCH_FOLDER_OPEN) {
		if (data->touch_driver[SECURE_TOUCH_SUB_DEV].enabled) {
			if (data->touch_driver[SECURE_TOUCH_SUB_DEV].is_running) {
				schedule_delayed_work(&data->folder_work, msecs_to_jiffies(10));
				mutex_unlock(&data->lock);
				return;
			}

			if (!IS_ERR_OR_NULL(sysfs_get_dirent(data->device->kobj.sd, "secure"))) {
				sysfs_remove_link(&data->device->kobj, "secure");
				data->touch_driver[SECURE_TOUCH_SUB_DEV].enabled = 0;
			}
		} else {
			pr_info("%s: %s: sub is not enabled: %d\n", SECLOG, __func__, __LINE__);
		}

		if (data->touch_driver[SECURE_TOUCH_MAIN_DEV].registered) {
			if (data->touch_driver[SECURE_TOUCH_MAIN_DEV].enabled == 1) {
				mutex_unlock(&data->lock);
				return;
			}

			if (!IS_ERR_OR_NULL(data->touch_driver[SECURE_TOUCH_MAIN_DEV].kobj)) {
				ret = sysfs_create_link(&data->device->kobj, data->touch_driver[SECURE_TOUCH_MAIN_DEV].kobj, "secure");
				if (ret < 0) {
					pr_info("%s: %s: ret:%d, line:%d\n", SECLOG, __func__, ret, __LINE__);
					mutex_unlock(&data->lock);
					return;
				}

				data->touch_driver[SECURE_TOUCH_MAIN_DEV].enabled = 1;
			}
		} else {
			pr_info("%s: %s: main is not enabled: %d\n", SECLOG, __func__, __LINE__);
		}
	} else if (data->hall_ic == SECURE_TOUCH_FOLDER_CLOSE) {
		if (data->touch_driver[SECURE_TOUCH_MAIN_DEV].enabled) {
			if (data->touch_driver[SECURE_TOUCH_MAIN_DEV].is_running) {
				schedule_delayed_work(&data->folder_work, msecs_to_jiffies(10));
				mutex_unlock(&data->lock);
				return;
			}

			if (!IS_ERR_OR_NULL(sysfs_get_dirent(data->device->kobj.sd, "secure"))) {
				sysfs_remove_link(&data->device->kobj, "secure");
				data->touch_driver[SECURE_TOUCH_MAIN_DEV].enabled = 0;
			}
		} else {
			pr_info("%s: %s: main is not enabled: %d\n", SECLOG, __func__, __LINE__);
		}

		if (data->touch_driver[SECURE_TOUCH_SUB_DEV].registered) {
			if (data->touch_driver[SECURE_TOUCH_SUB_DEV].enabled == 1) {
				mutex_unlock(&data->lock);
				return;
			}

			if (!IS_ERR_OR_NULL(data->touch_driver[SECURE_TOUCH_SUB_DEV].kobj)) {
				ret = sysfs_create_link(&data->device->kobj, data->touch_driver[SECURE_TOUCH_SUB_DEV].kobj, "secure");
				if (ret < 0) {
					pr_info("%s: %s: ret:%d, line:%d\n", SECLOG, __func__, ret, __LINE__);
					mutex_unlock(&data->lock);
					return;
				}

				data->touch_driver[SECURE_TOUCH_SUB_DEV].enabled = 1;
			}
		} else {
			pr_info("%s: %s: sub is not enabled: %d\n", SECLOG, __func__, __LINE__);
		}
	} else {
		mutex_unlock(&data->lock);
		return;
	}

	mutex_unlock(&data->lock);
}

#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
static int sec_secure_touch_hall_ic_notifier(struct notifier_block *nb, unsigned long hall_ic, void *ptr)
{
	struct sec_secure_touch *data = container_of(nb, struct sec_secure_touch, nb);
	struct hall_notifier_context *hall_notifier;

	if (!data)
		return NOTIFY_DONE;

	if (data->device_number < 1)
		return NOTIFY_DONE;

	hall_notifier = ptr;

	if (strncmp(hall_notifier->name, "flip", 4) != 0) {
		pr_info("%s: %s\n", __func__, hall_notifier->name);
		return NOTIFY_DONE;
	}

	data->hall_ic = hall_ic;

	pr_info("%s %s: device number:%d,%s %s%s\n", SECLOG, __func__, data->device_number,
			data->hall_ic ? "CLOSE" : "OPEN",
			data->touch_driver[SECURE_TOUCH_MAIN_DEV].is_running ? "tsp1" : "",
			data->touch_driver[SECURE_TOUCH_SUB_DEV].is_running ? "tsp2" : "");

	if (data->device_number < 2)
		return NOTIFY_DONE;

	if (data->hall_ic == SECURE_TOUCH_FOLDER_OPEN) {
		if (data->touch_driver[SECURE_TOUCH_SUB_DEV].registered) {
			if (data->touch_driver[SECURE_TOUCH_SUB_DEV].enabled) {
				struct sec_ts_plat_data *pdata;
				struct sec_trusted_touch *pvm;

				if (!data->touch_driver[SECURE_TOUCH_SUB_DEV].dev)
					goto out;
				pdata = data->touch_driver[SECURE_TOUCH_SUB_DEV].dev->platform_data;
				pvm = pdata->pvm;

				if (atomic_read(&pvm->trusted_touch_enabled) == 1) {
					pr_info("[sec_input] %s wait for disabling trusted touch(sub)\n", __func__);
					wait_for_completion_interruptible(&pvm->trusted_touch_powerdown);
					pr_info("[sec_input] %s complete disabling trusted touch(sub)\n", __func__);
				}
			}
		}
	} else if (data->hall_ic == SECURE_TOUCH_FOLDER_CLOSE) {
		if (data->touch_driver[SECURE_TOUCH_MAIN_DEV].registered) {
			if (data->touch_driver[SECURE_TOUCH_MAIN_DEV].enabled) {
				struct sec_ts_plat_data *pdata;
				struct sec_trusted_touch *pvm;

				if (!data->touch_driver[SECURE_TOUCH_MAIN_DEV].dev)
					goto out;
				pdata = data->touch_driver[SECURE_TOUCH_MAIN_DEV].dev->platform_data;
				pvm = pdata->pvm;

				if (atomic_read(&pvm->trusted_touch_enabled) == 1) {
					pr_info("[sec_input] %s wait for disabling trusted touch(main)\n", __func__);
					wait_for_completion_interruptible(&pvm->trusted_touch_powerdown);
					pr_info("[sec_input] %s complete disabling trusted touch(main)\n", __func__);
				}
			}
		}
	}
out:
	schedule_work(&data->folder_work.work);

	return NOTIFY_DONE;
}
#endif

#if IS_ENABLED(CONFIG_SUPPORT_SENSOR_FOLD)
static int sec_secure_touch_hall_ic_ssh_notifier(struct notifier_block *nb, unsigned long hall_ic, void *ptr)
{
	struct sec_secure_touch *data = container_of(nb, struct sec_secure_touch, nb_ssh);

	if (!data)
		return NOTIFY_DONE;

	if (data->device_number < 1)
		return NOTIFY_DONE;

	data->hall_ic = hall_ic;

	pr_info("%s %s: device number:%d,%s %s%s\n", SECLOG, __func__, data->device_number,
			data->hall_ic ? "CLOSE" : "OPEN",
			data->touch_driver[SECURE_TOUCH_MAIN_DEV].is_running ? "tsp1" : "",
			data->touch_driver[SECURE_TOUCH_SUB_DEV].is_running ? "tsp2" : "");

	schedule_work(&data->folder_work.work);

	return NOTIFY_DONE;
}
#endif
#endif

static int sec_secure_touch_probe(struct platform_device *pdev)
{
	struct sec_secure_touch *data;
	int ret;

#if !IS_ENABLED(CONFIG_DRV_SAMSUNG)
	pr_info("%s %s: sec_class is not support\n", SECLOG, __func__);
	return -ENOENT;
#endif
	data = kzalloc(sizeof(struct sec_secure_touch), GFP_KERNEL);
	if (!data) {
		pr_info("%s %s: failed probe: mem\n", SECLOG, __func__);
		return -ENOMEM;
	}
	data->pdev = pdev;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	data->device = sec_device_create(data, SECURE_TOUCH_DEV_NAME);
	if (IS_ERR(data->device)) {
		pr_info("%s %s: failed probe: create\n", SECLOG, __func__);
		kfree(data);
		return -ENODEV;
	}
#endif
	g_ss_touch = data;

	dev_set_drvdata(data->device, data);

	platform_set_drvdata(pdev, data);

	mutex_init(&data->lock);

	ret = sysfs_create_group(&data->device->kobj, &sec_secure_touch_attr_group);
	if (ret < 0) {
		pr_info("%s %s: failed probe: create sysfs\n", SECLOG, __func__);
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
		sec_device_destroy(data->device->devt);
#endif
		g_ss_touch = NULL;
		kfree(data);
		return -ENODEV;
	}

#if IS_ENABLED(CONFIG_SEC_INPUT_MULTI_DEVICE)
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	data->nb.notifier_call = sec_secure_touch_hall_ic_notifier;
	data->nb.priority = 1;
	hall_notifier_register(&data->nb);
#endif
#if IS_ENABLED(CONFIG_SUPPORT_SENSOR_FOLD)
	data->nb_ssh.notifier_call = sec_secure_touch_hall_ic_ssh_notifier;
	data->nb_ssh.priority = 1;
	sensorfold_notifier_register(&data->nb_ssh);
#endif
	INIT_DELAYED_WORK(&data->folder_work, sec_secure_touch_hall_ic_work);
#else
	sec_secure_touch_set_device(data, 1);
#endif
	pr_info("%s: %s\n", SECLOG, __func__);

	return 0;
}

static int sec_secure_touch_remove(struct platform_device *pdev)
{
	struct sec_secure_touch *data = platform_get_drvdata(pdev);
	int ii;

	pr_info("%s: %s\n", SECLOG, __func__);
#if IS_ENABLED(CONFIG_SEC_INPUT_MULTI_DEVICE)
	mutex_lock(&data->lock);
#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
	hall_notifier_unregister(&data->nb);
#endif
	mutex_unlock(&data->lock);
#endif
	for (ii = 0; ii < data->device_number; ii++) {
		if (data->touch_driver[ii].enabled)
			sysfs_remove_link(&data->device->kobj, "secure");

		sysfs_remove_group(&data->device->kobj, &sec_secure_touch_attr_group);
	}

	mutex_destroy(&data->lock);
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sec_device_destroy(data->device->devt);
#endif
	g_ss_touch = NULL;
	kfree(data);
	return 0;
}

#if CONFIG_OF
static const struct of_device_id sec_secure_touch_dt_match[] = {
	{ .compatible = "samsung,ss_touch" },
	{}
};
#endif

struct platform_driver sec_secure_touch_driver = {
	.probe = sec_secure_touch_probe,
	.remove = sec_secure_touch_remove,
	.driver = {
		.name = "sec_secure_touch",
		.owner = THIS_MODULE,
#if CONFIG_OF
		.of_match_table = of_match_ptr(sec_secure_touch_dt_match),
#endif
	},
};

int sec_secure_touch_init(void)
{
	pr_info("%s: %s\n", SECLOG, __func__);

	platform_driver_register(&sec_secure_touch_driver);
	return 0;
}
EXPORT_SYMBOL(sec_secure_touch_init);

void sec_secure_touch_exit(void)
{
	pr_info("%s; %s\n", SECLOG, __func__);

};
EXPORT_SYMBOL(sec_secure_touch_exit);

MODULE_DESCRIPTION("Samsung Secure Touch Driver");
MODULE_LICENSE("GPL");

