/* kunit_manager.c
 *
 * Driver to manage kunit
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 * Sangsu Ha <sangsu.ha@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <kunit/kunit_manager.h>

extern int test_executor_init(void);

static ssize_t kunit_manager_run_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (!strncmp(buf, "1", 1))
		test_executor_init();
	return count;
}
static DEVICE_ATTR(run, 0200, NULL, kunit_manager_run_store);

static struct attribute *kunit_manager_attributes[] = {
	&dev_attr_run.attr,
	NULL,
};

static struct attribute_group kunit_manager_attr_group = {
	.attrs = kunit_manager_attributes,
};

static int __init kunit_manager_init(void)
{
	struct kunit_manager_data *data;
	int ret;

	data = kzalloc(sizeof(struct kunit_manager_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;


	data->dev = (struct device *)sec_device_create(data, "sec_kunit");
	if (IS_ERR(data->dev)) {
		pr_err("%s: Failed to create device!\n", __func__);
		ret = PTR_ERR(data->dev);
		goto err_sec_device_create;
	}

	dev_set_drvdata(data->dev, data);

	ret = sysfs_create_group(&data->dev->kobj, &kunit_manager_attr_group);
	if (ret < 0) {
		pr_err("%s: Failed to create sysfs group\n", __func__);
		goto err_sysfs_create_group;
	}

	pr_info("%s: Success\n", __func__);

	return 0;

err_sysfs_create_group:
	sec_device_destroy(data->dev->devt);
err_sec_device_create:
	kfree(data);

	return ret;
}

static void __exit kunit_manager_exit(void)
{
}

module_init(kunit_manager_init);
module_exit(kunit_manager_exit);

MODULE_DESCRIPTION("Samsung kunit manager driver");
MODULE_AUTHOR("Sangsu Ha <sangsu.ha@samsung.com>");
MODULE_LICENSE("GPL");
