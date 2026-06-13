// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2021-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug.h>

#include "sec_qc_debug.h"

static noinline int __qc_debug_parse_dt_reboot_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,reboot_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->reboot_nb.priority = (int)priority;

	return 0;
}

static noinline int __qc_debug_parse_dt_use_cp_dump_encrypt(struct builder *bd,
		struct device_node *np)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	drvdata->use_cp_dump_encrypt =
			of_property_read_bool(np, "sec,use-cp_dump_encrypt");

	return 0;
}

static int __debug_parse_dt_cp_encrypt_nr_entries(struct device_node *np)
{
	int nr_entries;

	nr_entries = of_property_count_elems_of_size(np,
			"sec,cp_dump_encrypt_magic", sizeof(u32)) / 2;

	if (nr_entries < 0)
		return 0;

	return nr_entries;
}

static int __debug_parse_dt_cp_encrypt_each_entry(struct cp_dump_encrypt_entry *entry,
		struct device *dev, struct device_node *np, int i)
{
	const char *cp_dump_encrypt_reg = "sec,cp_dump_encrypt_reg";
	const char *cp_dump_encrypt_magic = "sec,cp_dump_encrypt_magic";
	const char *compatible;
	u32 low, mid_high;
	int err;

	err = of_property_read_string_helper(np, cp_dump_encrypt_reg, &compatible, 1, i);
	if (err < 0) {
		dev_err(dev, "can't read %s [%d]\n", compatible, i);
		return err;
	}

	err = of_property_read_u32_index(np, cp_dump_encrypt_magic, i * 2, &low);
	if (err) {
		dev_err(dev, "%s %d 'low' read error - %d\n",
				cp_dump_encrypt_magic, i, err);
		return -EINVAL;
	}

	err = of_property_read_u32_index(np, cp_dump_encrypt_magic, i * 2 + 1, &mid_high);
	if (err) {
		dev_err(dev, "%s %d 'mid/high' read error - %d\n",
				cp_dump_encrypt_magic, i, err);
		return -EINVAL;
	}

	entry->compatible = compatible;
	entry->low = low;
	entry->mid_high = mid_high;

	return 0;
}

static noinline int __qc_debug_parse_dt_cp_dump_encrypt_magic(struct builder *bd,
		struct device_node *np)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);
	struct device *dev = bd->dev;
	struct cp_dump_encrypt *encrypt;
	int nr_entries;
	struct cp_dump_encrypt_entry *entry;
	int i;

	if (!drvdata->use_cp_dump_encrypt)
		return 0;

	nr_entries = __debug_parse_dt_cp_encrypt_nr_entries(np);
	if (!nr_entries) {
		drvdata->use_cp_dump_encrypt = false;
		return 0;
	}

	entry = devm_kcalloc(dev, nr_entries, sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	for (i = 0; i < nr_entries; i++) {
		int err = __debug_parse_dt_cp_encrypt_each_entry(&entry[i], dev, np, i);

		if (err)
			return err;
	}

	encrypt = &drvdata->cp_dump_encrypt;

	encrypt->nr_entries = nr_entries;
	encrypt->entry = entry;

	return 0;
}

static noinline int __qc_debug_parse_dt_use_store_last_kmsg(struct builder *bd,
		struct device_node *np)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	drvdata->use_store_last_kmsg =
			of_property_read_bool(np, "sec,use-store_last_kmsg");

	return 0;
}

static noinline int __qc_debug_parse_dt_use_store_lpm_kmsg(struct builder *bd,
		struct device_node *np)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	drvdata->use_store_lpm_kmsg =
			of_property_read_bool(np, "sec,use-store_lpm_kmsg");

	return 0;
}

static noinline int __qc_debug_parse_dt_use_store_onoff_history(struct builder *bd,
		struct device_node *np)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	drvdata->use_store_onoff_history =
			of_property_read_bool(np, "sec,use-store_onoff_history");

	return 0;
}

static const struct dt_builder __qc_debug_dt_builder[] = {
	DT_BUILDER(__qc_debug_parse_dt_reboot_notifier_priority),
	DT_BUILDER(__qc_debug_parse_dt_use_cp_dump_encrypt),
	DT_BUILDER(__qc_debug_parse_dt_cp_dump_encrypt_magic),
	DT_BUILDER(__qc_debug_parse_dt_use_store_last_kmsg),
	DT_BUILDER(__qc_debug_parse_dt_use_store_lpm_kmsg),
	DT_BUILDER(__qc_debug_parse_dt_use_store_onoff_history)
};

static noinline int __qc_debug_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __qc_debug_dt_builder,
			ARRAY_SIZE(__qc_debug_dt_builder));
}

static noinline int __qc_debug_probe_epilog(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static const struct dev_builder __qc_debug_dev_builder[] = {
	DEVICE_BUILDER(__qc_debug_parse_dt, NULL),
	DEVICE_BUILDER(sec_qc_cp_dump_encrypt_init, NULL),
	DEVICE_BUILDER(sec_qc_debug_lpm_log_init, NULL),
	DEVICE_BUILDER(sec_qc_debug_reboot_init, sec_qc_debug_reboot_exit),
	DEVICE_BUILDER(sec_qc_force_err_init, sec_qc_force_err_exit),
	DEVICE_BUILDER(sec_qc_boot_stat_init, sec_qc_boot_stat_exit),
	DEVICE_BUILDER(sec_qc_vendor_hooks_init, NULL),
	DEVICE_BUILDER(__qc_debug_probe_epilog, NULL),
};

static int __qc_debug_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_debug_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __qc_debug_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_debug_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int sec_qc_debug_probe(struct platform_device *pdev)
{
	return __qc_debug_probe(pdev, __qc_debug_dev_builder,
			ARRAY_SIZE(__qc_debug_dev_builder));
}

static int sec_qc_debug_remove(struct platform_device *pdev)
{
	return __qc_debug_remove(pdev, __qc_debug_dev_builder,
			ARRAY_SIZE(__qc_debug_dev_builder));
}

static const struct of_device_id sec_qc_debug_match_table[] = {
	{ .compatible = "samsung,qcom-debug" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_debug_match_table);

static struct platform_driver sec_qc_debug_driver = {
	.driver = {
		.name = "sec,qc-debug",
		.of_match_table = of_match_ptr(sec_qc_debug_match_table),
	},
	.probe = sec_qc_debug_probe,
	.remove = sec_qc_debug_remove,
};

static int __init sec_qc_debug_init(void)
{
	return platform_driver_register(&sec_qc_debug_driver);
}
module_init(sec_qc_debug_init);

static void __exit sec_qc_debug_exit(void)
{
	platform_driver_unregister(&sec_qc_debug_driver);
}
module_exit(sec_qc_debug_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Common debugging feature for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
