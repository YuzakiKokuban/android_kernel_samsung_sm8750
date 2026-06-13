// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>

#include <linux/samsung/bsp/sec_class.h>
#include <linux/samsung/bsp/sec_param.h>

#include "sec_qc_user_reset.h"

static ssize_t recovery_cause_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char recovery_cause[256];
	bool valid;

	valid = sec_param_get(param_index_reboot_recovery_cause,
			recovery_cause);
	if (valid)
		dev_info(dev, "%s\n", recovery_cause);
	else {
		strlcpy(recovery_cause, "invalid recovery cause",
				sizeof(recovery_cause));
		dev_warn(dev, "%s\n", recovery_cause);
	}

	return scnprintf(buf, PAGE_SIZE, "%s", recovery_cause);
}

static ssize_t recovery_cause_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char recovery_cause[256];
	bool valid;

	if (count > sizeof(recovery_cause)) {
		dev_err(dev, "input buffer length is out of range.\n");
		return -EINVAL;
	}

	snprintf(recovery_cause, sizeof(recovery_cause), "%s:%d ",
			current->comm, task_pid_nr(current));
	if (strlen(recovery_cause) + strlen(buf) >= sizeof(recovery_cause)) {
		dev_err(dev, "input buffer length is out of range.\n");
		return -EINVAL;
	}

	strlcat(recovery_cause, buf, sizeof(recovery_cause));
	valid = sec_param_set(param_index_reboot_recovery_cause,
			recovery_cause);
	if (valid)
		dev_info(dev, "%s\n", recovery_cause);
	else
		dev_warn(dev, "failed to write recovery cause - %s",
				recovery_cause);

	return count;
}

static DEVICE_ATTR_RW(recovery_cause);

int __qc_recovery_cause_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *sec_debug_dev = drvdata->sec_debug_dev;
	int err;

	err = sysfs_create_file(&sec_debug_dev->kobj,
			&dev_attr_recovery_cause.attr);
	if (err)
		dev_err(bd->dev, "failed to create sysfs group\n");

	return err;
}

void __qc_recovery_cause_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *sec_debug_dev = drvdata->sec_debug_dev;

	sysfs_remove_file(&sec_debug_dev->kobj, &dev_attr_recovery_cause.attr);
}
