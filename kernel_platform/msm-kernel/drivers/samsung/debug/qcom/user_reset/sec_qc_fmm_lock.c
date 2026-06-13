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

static ssize_t FMM_lock_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        int lock;
	char str[30];
	const char *status = "UNK";

        if (sec_param_get(param_index_FMM_lock, &lock))
		status = lock ? "ON" : "OFF";

        return scnprintf(buf, sizeof(str),"FMM lock : [%s]\n", status);
}

static ssize_t FMM_lock_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
        int lock;

	sscanf(buf, "%d", &lock);
	if (lock)
		lock = FMMLOCK_MAGIC_NUM;
	else
		lock = 0;

	dev_info(dev, "FMM lock[%s]\n", lock ? "ON" : "OFF");
        sec_param_set(param_index_FMM_lock, &lock);

        return count;
}

static DEVICE_ATTR_RW(FMM_lock);

int __qc_fmm_lock_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *sec_debug_dev = drvdata->sec_debug_dev;
	int err;

	err = sysfs_create_file(&sec_debug_dev->kobj, &dev_attr_FMM_lock.attr);
	if (err)
		dev_err(bd->dev, "failed to create sysfs group\n");

	return err;
}

void __qc_fmm_lock_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *sec_debug_dev = drvdata->sec_debug_dev;

	sysfs_remove_file(&sec_debug_dev->kobj, &dev_attr_FMM_lock.attr);
}
