/*
 * Samsung Mobile VE Group.
 *
 * drivers/gpio/secgpio_dvs.c
 *
 * Drivers for samsung gpio debugging & verification.
 *
 * Copyright (C) 2013, Samsung Electronics.
 *
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>

#include "secgpio_dvs.h"
#include <trace/events/power.h>

/*sys fs*/
struct class *secgpio_dvs_class;
EXPORT_SYMBOL(secgpio_dvs_class);

struct device *secgpio_dotest;
EXPORT_SYMBOL(secgpio_dotest);

/* extern GPIOMAP_RESULT GpioMap_result; */
static struct gpio_dvs_t *gdvs_info;

static ssize_t checked_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_sleep_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_secgpio_init_read_details(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_secgpio_sleep_read_details(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t secgpio_checked_sleepgpio_read(
	struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t checked_secgpio_init_call(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t len);

static DEVICE_ATTR(gpioinit_check, 0664,
	checked_secgpio_file_read, NULL);
static DEVICE_ATTR(gpiosleep_check, 0664,
	checked_sleep_secgpio_file_read, NULL);
static DEVICE_ATTR(check_init_detail, 0664,
	checked_secgpio_init_read_details, NULL);
static DEVICE_ATTR(check_sleep_detail, 0664,
	checked_secgpio_sleep_read_details, NULL);
static DEVICE_ATTR(checked_sleepGPIO, 0664,
	secgpio_checked_sleepgpio_read, NULL);
static DEVICE_ATTR(gpioinit_call, 0664,
	NULL, checked_secgpio_init_call);

static struct attribute *secgpio_dvs_attributes[] = {
		&dev_attr_gpioinit_check.attr,
		&dev_attr_gpiosleep_check.attr,
		&dev_attr_check_init_detail.attr,
		&dev_attr_check_sleep_detail.attr,
		&dev_attr_checked_sleepGPIO.attr,
		&dev_attr_gpioinit_call.attr,
		NULL,
};

static struct attribute_group secgpio_dvs_attr_group = {
		.attrs = secgpio_dvs_attributes,
};

static ssize_t checked_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "%x ", gdvs->result->init[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);
}

static ssize_t checked_sleep_secgpio_file_read(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "%x ", gdvs->result->sleep[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);
}

static ssize_t checked_secgpio_init_read_details(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "GI[%d] - %x\n ",
			i, gdvs->result->init[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);
}
static ssize_t checked_secgpio_sleep_read_details(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	char temp_buf[20];
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	for (i = 0; i < gdvs->count; i++) {
		memset(temp_buf, 0, sizeof(char)*20);
		snprintf(temp_buf, 20, "GS[%d] - %x\n ",
			i, gdvs->result->sleep[i]);
		strlcat(buf, temp_buf, PAGE_SIZE);
	}

	return strlen(buf);

}


static ssize_t secgpio_checked_sleepgpio_read(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct gpio_dvs_t *gdvs = dev_get_drvdata(dev);

	if (gdvs->check_sleep)
		return snprintf(buf, PAGE_SIZE, "1");
	else
		return snprintf(buf, PAGE_SIZE, "0");
}

static ssize_t checked_secgpio_init_call(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	/* Check init gpio status */
	gpio_dvs_check_initgpio();

	return len;
}

void gpio_dvs_check_initgpio(void)
{
	if (gdvs_info && gdvs_info->check_gpio_status)
		gdvs_info->check_gpio_status(PHONE_INIT);
}
EXPORT_SYMBOL(gpio_dvs_check_initgpio);

void gpio_dvs_check_sleepgpio(void)
{
	if (unlikely(gdvs_info && !gdvs_info->check_sleep)) {
		gdvs_info->check_gpio_status(PHONE_SLEEP);
		gdvs_info->check_sleep = true;
	}
}

#ifdef CONFIG_OF
static const struct of_device_id secgpio_dvs_dt_match[] = {
	{ .compatible = "samsung,secgpio-dvs",
		.data = (void *)&msm_gpio_dvs_data },
	{ },
};
MODULE_DEVICE_TABLE(of, secgpio_dvs_dt_match);

static struct secgpio_dvs_data *secgpio_dvs_get_soc_data(
		struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct device_node *node = pdev->dev.of_node;
	struct secgpio_dvs_data *data;

	match = of_match_node(secgpio_dvs_dt_match, node);
	if (!match) {
		dev_err(&pdev->dev, "failed to get SoC node\n");
		return NULL;
	}

	data = (struct secgpio_dvs_data *)match->data;
	if (!data) {
		dev_err(&pdev->dev, "failed to get SoC data\n");
		return NULL;
	}

	return data;
}
#else
static struct gpio_dvs_t *secgpio_dvs_get_soc_data(struct platform_device *pdev)
{
	return dev_get_platdata(&pdev->dev);
}
#endif

static int secgpio_dvs_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct class *secgpio_dvs_class;
	struct device *secgpio_dotest;
	struct secgpio_dvs_data *data = secgpio_dvs_get_soc_data(pdev);
	struct gpio_dvs_t *gdvs;

	pr_info("[secgpio_dvs] %s has been created!!!\n", __func__);

	if (!data)
		return -ENODEV;

	gdvs = data->gpio_dvs;

	secgpio_dvs_class = class_create("secgpio_check");
	if (IS_ERR(secgpio_dvs_class)) {
		ret = PTR_ERR(secgpio_dvs_class);
		pr_err("Failed to create class(secgpio_check_all)");
		goto fail_out;
	}

	secgpio_dotest = device_create(secgpio_dvs_class,
				NULL, 0, NULL, "secgpio_check_all");
	if (IS_ERR(secgpio_dotest)) {
		ret = PTR_ERR(secgpio_dotest);
		pr_err("Failed to create device(secgpio_check_all)");
		goto fail1;
	}
	dev_set_drvdata(secgpio_dotest, gdvs);
	gdvs_info = gdvs;

	ret = sysfs_create_group(&secgpio_dotest->kobj,
			&secgpio_dvs_attr_group);
	if (ret) {
		pr_err("Failed to create sysfs group");
		goto fail2;
	}

	return ret;

fail2:
	device_destroy(secgpio_dvs_class, 0);
fail1:
	class_destroy(secgpio_dvs_class);
fail_out:
	if (ret)
		pr_err(" (err = %d)!\n", ret);
	return ret;

}

static int secgpio_dvs_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver secgpio_dvs = {
	.probe = secgpio_dvs_probe,
	.remove = secgpio_dvs_remove,
	.driver = {
		.name = "secgpio_dvs",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(secgpio_dvs_dt_match),
#endif
	},
};

static void gpio_debug_suspend_trace_probe(void *unused,
					const char *action, int val, bool start)
{
	if (start && val > 0 && !strcmp("machine_suspend", action)) {
		gpio_dvs_check_sleepgpio();
	}
}

static int __init secgpio_dvs_init(void)
{
	int ret;

	ret = platform_driver_register(&secgpio_dvs);
	pr_info("[secgpio_dvs] %s has been initialized!!!, ret=%d\n", __func__, ret);

	/* Register callback for cheking sleep gpio status */
	ret = register_trace_suspend_resume(
		gpio_debug_suspend_trace_probe, NULL);
	if (ret) {
		pr_err("%s: Failed to register suspend trace callback, ret=%d\n",
			__func__, ret);
		return ret;
	}

	return ret;
}

static void __exit secgpio_dvs_exit(void)
{
	unregister_trace_suspend_resume(
			gpio_debug_suspend_trace_probe, NULL);

	platform_driver_unregister(&secgpio_dvs);
}

module_init(secgpio_dvs_init);
module_exit(secgpio_dvs_exit);

MODULE_DESCRIPTION("Samsung GPIO debugging and verification");
MODULE_LICENSE("GPL v2");
