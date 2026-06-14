// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/samsung/bsp/sec_class.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

#include "sec_qc_user_reset.h"

struct qc_user_reset_drvdata *qc_user_reset;

static noinline int __user_reset_test_dbg_partition(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct debug_reset_header __reset_header;
	struct debug_reset_header *reset_header = &__reset_header;
	bool valid;

	valid = sec_qc_dbg_part_read(debug_index_reset_summary_info,
			reset_header);
	if (!valid)
		return -EPROBE_DEFER;

	if (reset_header->magic != DEBUG_PARTITION_MAGIC) {
		dev_warn(drvdata->bd.dev, "debug parition is not valid.\n");
		return -ENODEV;
	}

	return 0;
}

static int __user_reset_init_sec_class(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *sec_debug_dev;

	sec_debug_dev = sec_device_create(NULL, "sec_debug");
	if (IS_ERR(sec_debug_dev)) {
		dev_err(bd->dev, "failed to create device for sec_debug\n");
		return PTR_ERR(sec_debug_dev);
	}

	drvdata->sec_debug_dev = sec_debug_dev;

	return 0;
}

static void __user_reset_exit_sec_class(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *sec_debug_dev = drvdata->sec_debug_dev;

	sec_device_destroy(sec_debug_dev->devt);
}

static noinline int __user_reset_probe_epilog(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	qc_user_reset = drvdata;

	return 0;
}

static noinline void __user_reset_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	qc_user_reset = NULL;
}

static const struct dev_builder __user_reset_dev_builder[] = {
	DEVICE_BUILDER(__user_reset_test_dbg_partition, NULL),
	/* TODO: deferrable concrete builders should be before here. */
	DEVICE_BUILDER(__qc_ap_health_init, __qc_ap_health_exit),
	DEVICE_BUILDER(__qc_modem_reset_data_init, NULL),
	DEVICE_BUILDER(__qc_kryo_arm64_edac_init,
		       __qc_kryo_arm64_edac_exit),
	DEVICE_BUILDER(__user_reset_init_sec_class,
		       __user_reset_exit_sec_class),
	DEVICE_BUILDER(__qc_reset_rwc_init, __qc_reset_rwc_exit),
	DEVICE_BUILDER(__qc_reset_reason_init, __qc_reset_reason_exit),
	DEVICE_BUILDER(__qc_reset_summary_init, __qc_reset_summary_exit),
	DEVICE_BUILDER(__qc_reset_klog_init, __qc_reset_klog_exit),
	DEVICE_BUILDER(__qc_reset_tzlog_init, __qc_reset_tzlog_exit),
	DEVICE_BUILDER(__qc_auto_comment_init, __qc_auto_comment_exit),
	DEVICE_BUILDER(__qc_reset_history_init, __qc_reset_history_exit),
	DEVICE_BUILDER(__qc_fmm_lock_init, __qc_fmm_lock_exit),
	DEVICE_BUILDER(__qc_recovery_cause_init, __qc_recovery_cause_exit),
	DEVICE_BUILDER(__qc_store_lastkmsg_init, __qc_store_lastkmsg_exit),
	DEVICE_BUILDER(__user_reset_probe_epilog, __user_reset_remove_prolog),
};

static int __user_reset_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_user_reset_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __user_reset_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_user_reset_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int sec_qc_user_reset_probe(struct platform_device *pdev)
{
	return __user_reset_probe(pdev, __user_reset_dev_builder,
			ARRAY_SIZE(__user_reset_dev_builder));
}

static int sec_qc_user_reset_remove(struct platform_device *pdev)
{
	return __user_reset_remove(pdev, __user_reset_dev_builder,
			ARRAY_SIZE(__user_reset_dev_builder));
}

static const struct of_device_id sec_qc_user_reset_match_table[] = {
	{ .compatible = "samsung,qcom-user_reset" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_user_reset_match_table);

static struct platform_driver sec_qc_user_reset_driver = {
	.driver = {
		.name = "sec,qc-user_reset",
		.of_match_table = of_match_ptr(sec_qc_user_reset_match_table),
	},
	.probe = sec_qc_user_reset_probe,
	.remove = sec_qc_user_reset_remove,
};

static int __init sec_qc_user_reset_init(void)
{
	return platform_driver_register(&sec_qc_user_reset_driver);
}
module_init(sec_qc_user_reset_init);

static void __exit sec_qc_user_reset_exit(void)
{
	platform_driver_unregister(&sec_qc_user_reset_driver);
}
module_exit(sec_qc_user_reset_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("User Reset Debug for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
