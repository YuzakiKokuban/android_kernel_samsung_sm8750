// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/samsung/builder_pattern.h>

#include "sec_debug.h"

static unsigned long long ap_serial __ro_after_init;
module_param_named(ap_serial, ap_serial, ullong, 0440);

static ssize_t SVC_AP_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%016llX\n", ap_serial);
}

static DEVICE_ATTR_RO(SVC_AP);

static struct attribute *SVC_AP_attrs[] = {
	&dev_attr_SVC_AP.attr,
	NULL,
};

static struct attribute_group SVC_AP_group = {
	.attrs = SVC_AP_attrs,
};

static const struct attribute_group *SVC_AP_groups[] = {
	&SVC_AP_group,
	NULL,
};

static int __ap_serial_svc_ap_init(struct device *dev,
		struct svc_ap_node *svc_ap)
{
	struct kernfs_node *kn;
	struct kset *dev_ks;
	struct kobject *svc_kobj;
	struct device *ap_dev;
	int err;

	dev_ks = dev->kobj.kset;

	svc_kobj = kobject_create_and_add("svc", &dev_ks->kobj);
	if (IS_ERR_OR_NULL(svc_kobj)) {
		kn = sysfs_get_dirent(dev_ks->kobj.sd, "svc");
		if (!kn) {
			dev_err(dev, "failed to create sys/devices/svc\n");
			return -ENODEV;
		}
		svc_kobj = (struct kobject *)kn->priv;
	}

	ap_dev = devm_kzalloc(dev, sizeof(struct device), GFP_KERNEL);
	if (!ap_dev) {
		err = -ENOMEM;
		goto err_alloc_ap_dev;
	}

	err = dev_set_name(ap_dev, "AP");
	if (err < 0) {
		err = -ENOENT;
		goto err_set_name_ap_dev;
	}

	ap_dev->kobj.parent = svc_kobj;
	ap_dev->groups = SVC_AP_groups;

	err = device_register(ap_dev);
	if (err < 0) {
		err = -EINVAL;
		goto err_register_ap_dev;
	}

	svc_ap->ap_dev = ap_dev;
	svc_ap->svc_kobj = svc_kobj;

	return 0;

err_register_ap_dev:
err_set_name_ap_dev:
err_alloc_ap_dev:
	kobject_put(svc_kobj);

	return err;
}

static void __ap_serial_svc_ap_exit(struct device *dev,
		struct svc_ap_node *svc_ap)
{
	device_unregister(svc_ap->ap_dev);
	kobject_put(svc_ap->svc_kobj);
}

int sec_ap_serial_sysfs_init(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata = container_of(bd,
			struct sec_debug_drvdata, bd);
	struct device *dev = bd->dev;

	return __ap_serial_svc_ap_init(dev, &drvdata->svc_ap);
}

void sec_ap_serial_sysfs_exit(struct builder *bd)
{
	struct sec_debug_drvdata *drvdata = container_of(bd,
			struct sec_debug_drvdata, bd);
	struct device *dev = bd->dev;

	__ap_serial_svc_ap_exit(dev, &drvdata->svc_ap);
}
