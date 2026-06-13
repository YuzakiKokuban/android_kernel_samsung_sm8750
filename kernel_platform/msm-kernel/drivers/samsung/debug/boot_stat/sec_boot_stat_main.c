// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <linux/samsung/bsp/sec_class.h>
#include <linux/samsung/sec_kunit.h>

#include "sec_boot_stat.h"

static struct boot_stat_drvdata *sec_boot_stat;

static __always_inline bool __boot_stat_is_probed(void)
{
	return !!sec_boot_stat;
}

__ss_static __ss_always_inline bool __is_for_enh_boot_time(const char *log)
{
	const struct {
		char pmsg_mark[2];	/* !@ */
		uint64_t boot_prefix;
		char colon;
	} __packed *log_head = (const void *)log;
	const union {
		uint64_t raw;
		char text[8];
	} boot_prefix = {
		.text = { 'B', 'o', 'o', 't', '_', 'E', 'B', 'S', },
	};

	if (log_head->boot_prefix == boot_prefix.raw)
		return true;

	return false;
}

static void __boot_stat_add(struct boot_stat_drvdata *drvdata, const char *log)
{
	if (__is_for_enh_boot_time(log))
		sec_enh_boot_time_add_boot_event(drvdata, log);
	else
		sec_boot_stat_add_boot_event(drvdata, log);
}

void sec_boot_stat_add(const char *log)
{
	if (!__boot_stat_is_probed())
		return;

	__boot_stat_add(sec_boot_stat, log);
}
EXPORT_SYMBOL_GPL(sec_boot_stat_add);

int sec_boot_stat_register_soc_ops(struct sec_boot_stat_soc_operations *soc_ops)
{
	int ret = 0;

	if (!__boot_stat_is_probed())
		return -EBUSY;

	mutex_lock(&sec_boot_stat->soc_ops_lock);

	if (sec_boot_stat->soc_ops) {
		pr_warn("soc specific operations already registered\n");
		ret = -ENOENT;
		goto __arleady_registered;
	}

	sec_boot_stat->soc_ops = soc_ops;

__arleady_registered:
	mutex_unlock(&sec_boot_stat->soc_ops_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sec_boot_stat_register_soc_ops);

int sec_boot_stat_unregister_soc_ops(struct sec_boot_stat_soc_operations *soc_ops)
{
	int ret = 0;

	if (!__boot_stat_is_probed())
		return -EBUSY;

	mutex_lock(&sec_boot_stat->soc_ops_lock);

	if (sec_boot_stat->soc_ops != soc_ops) {
		pr_warn("already unregistered or wrong soc specific operation\n");
		ret = -EINVAL;
		goto __invalid_soc_ops;
	}

__invalid_soc_ops:
	mutex_unlock(&sec_boot_stat->soc_ops_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sec_boot_stat_unregister_soc_ops);

unsigned long long sec_boot_stat_ktime_to_time(unsigned long long ktime)
{
	struct sec_boot_stat_soc_operations *soc_ops;
	unsigned long long time;

	mutex_lock(&sec_boot_stat->soc_ops_lock);

	soc_ops = sec_boot_stat->soc_ops;
	if (!soc_ops || !soc_ops->ktime_to_time) {
		time = ktime;
		goto __without_adjust;
	}

	time = soc_ops->ktime_to_time(ktime);

__without_adjust:
	mutex_unlock(&sec_boot_stat->soc_ops_lock);
	return time;
}

void sec_boot_stat_bootloader_stat(struct seq_file *m)
{
	struct sec_boot_stat_soc_operations *soc_ops;

	mutex_lock(&sec_boot_stat->soc_ops_lock);

	soc_ops = sec_boot_stat->soc_ops;
	if (!soc_ops || !soc_ops->show_on_enh_boot_stat) {
		pr_warn("Wrong soc show_on_enh_boot_stat operation\n");
		goto __without_adjust;
	}

	soc_ops->show_on_enh_boot_stat(m);

__without_adjust:
	mutex_unlock(&sec_boot_stat->soc_ops_lock);
}

static noinline int __boot_stat_probe_prolog(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);

	mutex_init(&drvdata->soc_ops_lock);

	return 0;
}

static noinline void __boot_stat_remove_epilog(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);

	mutex_destroy(&drvdata->soc_ops_lock);
}

static int __boot_stat_sec_class_create(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *bsp_dev;

	bsp_dev = sec_device_create(NULL, "bsp");
	if (IS_ERR(bsp_dev))
		return PTR_ERR(bsp_dev);

	dev_set_drvdata(bsp_dev, drvdata);

	drvdata->bsp_dev = bsp_dev;

	return 0;
}

static void __boot_stat_sec_class_remove(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *bsp_dev = drvdata->bsp_dev;

	if (!bsp_dev)
		return;

	sec_device_destroy(bsp_dev->devt);
}

static ssize_t boot_stat_store(struct device *sec_class_dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct boot_stat_drvdata *drvdata = dev_get_drvdata(sec_class_dev);

	__boot_stat_add(drvdata, buf);

	return count;
}
DEVICE_ATTR_WO(boot_stat);

static struct attribute *sec_bsp_attrs[] = {
	&dev_attr_boot_stat.attr,
	NULL,
};

static const struct attribute_group sec_bsp_attr_group = {
	.attrs = sec_bsp_attrs,
};

static noinline int __boot_stat_sysfs_create(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = drvdata->bsp_dev;
	int err;

	err = sysfs_create_group(&dev->kobj, &sec_bsp_attr_group);
	if (err)
		return err;

	return 0;
}

static noinline void __boot_stat_sysfs_remove(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = drvdata->bsp_dev;

	sysfs_remove_group(&dev->kobj, &sec_bsp_attr_group);
}

static noinline int __boot_stat_probe_epilog(struct builder *bd)
{
	struct boot_stat_drvdata *drvdata =
			container_of(bd, struct boot_stat_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static  int __boot_stat_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct boot_stat_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;
	sec_boot_stat = drvdata;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __boot_stat_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct boot_stat_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __boot_stat_dev_builder[] = {
	DEVICE_BUILDER(__boot_stat_probe_prolog, __boot_stat_remove_epilog),
	DEVICE_BUILDER(__boot_stat_sec_class_create,
		       __boot_stat_sec_class_remove),
	DEVICE_BUILDER(sec_boot_stat_proc_init, sec_boot_stat_proc_exit),
	DEVICE_BUILDER(sec_enh_boot_time_init, sec_enh_boot_time_exit),
	DEVICE_BUILDER(__boot_stat_sysfs_create, __boot_stat_sysfs_remove),
	DEVICE_BUILDER(__boot_stat_probe_epilog, NULL),
};

static int sec_boot_stat_probe(struct platform_device *pdev)
{
	return __boot_stat_probe(pdev, __boot_stat_dev_builder,
			ARRAY_SIZE(__boot_stat_dev_builder));
}

static int sec_boot_stat_remove(struct platform_device *pdev)
{
	return __boot_stat_remove(pdev, __boot_stat_dev_builder,
			ARRAY_SIZE(__boot_stat_dev_builder));
}

static const struct of_device_id sec_boot_stat_match_table[] = {
	{ .compatible = "samsung,boot_stat" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_boot_stat_match_table);

static struct platform_driver sec_boot_stat_driver = {
	.driver = {
		.name = "sec,boot_stat",
		.of_match_table = of_match_ptr(sec_boot_stat_match_table),
	},
	.probe = sec_boot_stat_probe,
	.remove = sec_boot_stat_remove,
};

static int __init sec_boot_stat_init(void)
{
	return platform_driver_register(&sec_boot_stat_driver);
}
module_init(sec_boot_stat_init);

static void __exit sec_boot_stat_exit(void)
{
	platform_driver_unregister(&sec_boot_stat_driver);
}
module_exit(sec_boot_stat_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Boot-stat driver");
MODULE_LICENSE("GPL v2");
