// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#define pr_fmt(fmt) "[VIB] " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/version.h>
#if defined(CONFIG_SEC_KUNIT)
#include "kunit_test/vibrator_vib_info_test.h"
#else
#define __visible_for_testing static
#endif
#define MAX_INTENSITY_VALUE 10000
#define MAX_FUNCTION_NUM 64

struct vib_info_platform_data {
	char **functions_s;
	int functions_count;
	u32 *intensities;
	u32 *haptic_intensities;
	int intensities_steps;
	int haptic_intensities_steps;
};

struct vib_info_driver_data {
	struct class *vib_info_class;
	struct device *virtual_dev;
	char **functions_s;
	int functions_count;
	u32 *intensities;
	u32 *haptic_intensities;
	int intensities_steps;
	int haptic_intensities_steps;
};

static const char *str_delimiter = ",";
static const char *str_newline = "\n";

__visible_for_testing ssize_t array2str(char *buf, u32 *arr_intensity, int size)
{
	ssize_t ret = 0;
	int i = 0;
	int data_limit = 0;

	data_limit = PAGE_SIZE - sizeof(str_newline);

	if (!arr_intensity)
		return -EINVAL;

	for (i = 0; i < size; i++) {
		ret += snprintf(buf + ret, data_limit - ret, "%u",
				arr_intensity[i]);
		if (i < (size - 1))
			ret += snprintf(buf + ret, data_limit - ret, "%s",
					str_delimiter);
	}

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s", str_newline);

	pr_info("%s %s\n", __func__, buf);

	return ret;
}

__visible_for_testing ssize_t names2str(char *buf, char **names, int size)
{
	ssize_t ret = 0;
	int i = 0;
	int data_limit = 0;

	data_limit = PAGE_SIZE - sizeof(str_newline);

	if (!names)
		return -EINVAL;

	for (i = 0; i < size; i++) {
		ret += snprintf(buf + ret, data_limit - ret, "%s",
				*(names + i));
		if (i < (size - 1))
			ret += snprintf(buf + ret, data_limit - ret, " ");
	}

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s", str_newline);

	pr_info("%s %s\n", __func__, buf);

	return ret;
}

static ssize_t functions_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct vib_info_driver_data *ddata = dev_get_drvdata(dev);
	ssize_t ret = 0;

	pr_info("%s\n", __func__);

	ret = names2str(buf, ddata->functions_s,
			ddata->functions_count);

	return ret;
}

static ssize_t intensities_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct vib_info_driver_data *ddata = dev_get_drvdata(dev);
	ssize_t ret = 0;

	pr_info("%s\n", __func__);

	ret = array2str(buf, ddata->intensities,
			ddata->intensities_steps);

	return ret;
}

static ssize_t haptic_intensities_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct vib_info_driver_data *ddata = dev_get_drvdata(dev);
	ssize_t ret = 0;

	pr_info("%s\n", __func__);

	ret = array2str(buf, ddata->haptic_intensities,
			ddata->haptic_intensities_steps);

	return ret;
}

static DEVICE_ATTR_RO(functions);
static DEVICE_ATTR_RO(intensities);
static DEVICE_ATTR_RO(haptic_intensities);

static struct attribute *vib_info_attributes[] = {
	&dev_attr_functions.attr,
	&dev_attr_intensities.attr,
	&dev_attr_haptic_intensities.attr,
	NULL,
};

static struct attribute_group vib_info_attr_group = {
	.attrs = vib_info_attributes,
};

static struct vib_info_platform_data *vib_info_get_pdata(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct vib_info_platform_data *pdata;
	int count = 0;
	int ret = 0, i = 0;

	if (!IS_ENABLED(CONFIG_OF) || !pdev->dev.of_node)
		return dev_get_platdata(&pdev->dev);

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		goto err;

	count = of_property_count_strings(dn, "functions");
	if (count < 0 || count > MAX_FUNCTION_NUM)
		goto err;

	dev_info(dev, "%s functions count %d\n", __func__, count);

	pdata->functions_s = devm_kcalloc(&pdev->dev, count,
		sizeof(*(pdata->functions_s)), GFP_KERNEL);
	if (!pdata->functions_s)
		goto err;

	count = of_property_read_string_array(dn, "functions",
			(const char **)pdata->functions_s, count);
	if (count < 0) {
		dev_err(dev, "%s error.of_property_read_string_array\n", __func__);
		goto err;
	}

	pdata->functions_count = count;

	for (i = 0; i < count; i++)
		dev_info(dev, "%s %s\n", __func__, *(pdata->functions_s+i));

	pdata->intensities_steps = of_property_count_elems_of_size(dn,
			"samsung,intensities", sizeof(u32));
	if (pdata->intensities_steps < 0) {
		dev_err(dev, "%s error.intensities.of_property_count_elems_of_size\n",
			__func__);
		goto err;
	}

	dev_info(dev, "%s intensity size %d\n",
			__func__, pdata->intensities_steps);

	pdata->intensities = kmalloc_array(pdata->intensities_steps,
			sizeof(u32), GFP_KERNEL);
	if (!pdata->intensities)
		goto err;

	ret = of_property_read_u32_array(dn, "samsung,intensities",
		pdata->intensities, pdata->intensities_steps);
	if (ret) {
		dev_err(dev, "%s error.intensities.of_property_read_u32_array\n",
			__func__);
		goto err1;
	}

	for (i = 0; i < pdata->intensities_steps; i++) {
		if (pdata->intensities[i] > MAX_INTENSITY_VALUE) {
			dev_err(dev, "%s error. i=%d pdata->intensities=%u\n",
				__func__, i, pdata->intensities[i]);
			goto err1;
		} else {
			dev_info(dev, "%s i=%d %u\n", __func__,
					i, pdata->intensities[i]);
		}
	}

	pdata->haptic_intensities_steps = of_property_count_elems_of_size(dn,
			"samsung,haptic_intensities", sizeof(u32));
	if (pdata->intensities_steps < 0) {
		dev_err(dev, "%s error.haptic_intensities.of_property_count_elems_of_size\n",
			__func__);
		goto err1;
	}

	dev_info(dev, "%s haptic_intensity size %d\n",
			__func__, pdata->haptic_intensities_steps);

	pdata->haptic_intensities = kmalloc_array(pdata->haptic_intensities_steps,
			sizeof(u32), GFP_KERNEL);
	if (!pdata->haptic_intensities)
		goto err1;

	ret = of_property_read_u32_array(dn, "samsung,haptic_intensities",
		pdata->haptic_intensities, pdata->haptic_intensities_steps);
	if (ret) {
		dev_err(dev, "%s error.haptic_intensities.of_property_read_u32_array\n",
			__func__);
		goto err2;
	}

	for (i = 0; i < pdata->haptic_intensities_steps; i++) {
		if (pdata->haptic_intensities[i] > MAX_INTENSITY_VALUE) {
			dev_err(dev, "%s error. i=%d pdata->haptic_intensities=%u\n",
				__func__, i, pdata->haptic_intensities[i]);
			goto err2;
		} else {
			dev_info(dev, "%s i=%d %u\n", __func__,
					i, pdata->haptic_intensities[i]);
		}
	}

	return pdata;
err2:
	kfree(pdata->haptic_intensities);
err1:
	kfree(pdata->intensities);
err:
	return NULL;
}

static int vib_info_probe(struct platform_device *pdev)
{
	struct vib_info_platform_data *pdata;
	struct vib_info_driver_data *ddata;
	struct device *dev = &pdev->dev;

	int ret = 0;

	pdata = vib_info_get_pdata(pdev);
	if (!pdata) {
		dev_err(dev, "No platform data found\n");
		ret = -EINVAL;
		goto err1;
	}

	pdev->dev.platform_data = pdata;

	ddata = devm_kzalloc(dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata) {
		ret = -ENOMEM;
		goto err1;
	}

	ddata->functions_count = pdata->functions_count;
	ddata->functions_s = pdata->functions_s;
	ddata->intensities_steps = pdata->intensities_steps;
	ddata->intensities = pdata->intensities;
	ddata->haptic_intensities_steps
			= pdata->haptic_intensities_steps;
	ddata->haptic_intensities = pdata->haptic_intensities;

	platform_set_drvdata(pdev, ddata);
	dev_set_drvdata(dev, ddata);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	ddata->vib_info_class = class_create(THIS_MODULE, "vib_info_class");
#else
	ddata->vib_info_class = class_create("vib_info_class");
#endif
	if (IS_ERR(ddata->vib_info_class)) {
		ret = PTR_ERR(ddata->vib_info_class);
		goto err1;
	}
	ddata->virtual_dev = device_create(ddata->vib_info_class,
			NULL, MKDEV(0, 0), ddata, "vib_support_info");
	if (IS_ERR(ddata->virtual_dev)) {
		ret = PTR_ERR(ddata->virtual_dev);
		goto err2;
	}

	ret = sysfs_create_group(&ddata->virtual_dev->kobj,
			&vib_info_attr_group);
	if (ret) {
		ret = -ENODEV;
		dev_err(dev, "Failed to create sysfs %d\n", ret);
		goto err3;
	}

	dev_info(dev, "%s\n", __func__);
	return 0;
err3:
	device_destroy(ddata->vib_info_class, MKDEV(0, 0));
err2:
	class_destroy(ddata->vib_info_class);
err1:
	return ret;
}

static int vib_info_remove(struct platform_device *pdev)
{
	struct vib_info_driver_data *ddata = dev_get_drvdata(&pdev->dev);
	struct vib_info_platform_data *pdata;

	pdata = pdev->dev.platform_data;

	sysfs_remove_group(&pdev->dev.kobj, &vib_info_attr_group);
	device_destroy(ddata->vib_info_class, MKDEV(0, 0));
	class_destroy(ddata->vib_info_class);
	if (pdata) {
		kfree(pdata->haptic_intensities);
		kfree(pdata->intensities);
	}
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id vib_info_dt_ids[] = {
	{ .compatible = "samsung,vib-info",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, vib_info_dt_ids);
#endif

static struct platform_driver vib_info_driver = {
	.probe		= vib_info_probe,
	.remove		= vib_info_remove,
	.driver		= {
		.name	= "vib_info",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_match_ptr(vib_info_dt_ids),
#endif
	},
};

static int __init vib_info_init(void)
{
	return platform_driver_register(&vib_info_driver);
}

static void __exit vib_info_exit(void)
{
	platform_driver_unregister(&vib_info_driver);
}

module_init(vib_info_init);
module_exit(vib_info_exit);

MODULE_AUTHOR("Samsung USB Team");
MODULE_DESCRIPTION("vib_info");
MODULE_LICENSE("GPL");
