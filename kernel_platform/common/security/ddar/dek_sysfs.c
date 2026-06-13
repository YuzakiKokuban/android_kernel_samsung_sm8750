// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <ddar/dek_common.h>
#include <linux/string.h>

#ifdef CONFIG_DDAR_KEY_DUMP
static int kek_dump;

static ssize_t key_dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", kek_dump);
}

static ssize_t key_dump_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long flag;
	int err = kstrtoul(buf, 10, &flag);

	if (err)
		return 0;

	kek_dump = flag;

	return strlen(buf);
}

//static DEVICE_ATTR(key_dump, 0644, key_dump_show, key_dump_store);
static DEVICE_ATTR_RW(key_dump);

int dek_create_sysfs_key_dump(struct device *d)
{
	int error = device_create_file(d, &dev_attr_key_dump);

	if (error)
		return error;

	return 0;
}

int get_sdp_sysfs_key_dump(void)
{
	return kek_dump;
}
#else
int dek_create_sysfs_key_dump(struct device *d)
{
	pr_info("key_dump feature not available");

	return 0;
}

int get_sdp_sysfs_key_dump(void)
{
	pr_info("key_dump feature not available");

	return 0;
}
#endif
