// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "sec_arm64_debug.h"

static const struct dev_builder __arm64_debug_dev_builder[] = {
	DEVICE_BUILDER(sec_fsimd_debug_init_random_pi_work, NULL),
	DEVICE_BUILDER(sec_arm64_force_err_init, sec_arm64_force_err_exit),
};

static int __arm64_debug_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct arm64_debug_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __arm64_debug_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct arm64_debug_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int sec_arm64_debug_probe(struct platform_device *pdev)
{
	return __arm64_debug_probe(pdev, __arm64_debug_dev_builder,
			ARRAY_SIZE(__arm64_debug_dev_builder));
}

static int sec_arm64_debug_remove(struct platform_device *pdev)
{
	return __arm64_debug_remove(pdev, __arm64_debug_dev_builder,
			ARRAY_SIZE(__arm64_debug_dev_builder));
}

static const struct of_device_id sec_arm64_debug_match_table[] = {
	{ .compatible = "samsung,arm64-debug" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_arm64_debug_match_table);

static struct platform_driver sec_arm64_debug_driver = {
	.driver = {
		.name = "sec,arm64-debug",
		.of_match_table = of_match_ptr(sec_arm64_debug_match_table),
	},
	.probe = sec_arm64_debug_probe,
	.remove = sec_arm64_debug_remove,
};

static int __init sec_arm64_debug_init(void)
{
	return platform_driver_register(&sec_arm64_debug_driver);
}
module_init(sec_arm64_debug_init);

static void __exit sec_arm64_debug_exit(void)
{
	platform_driver_unregister(&sec_arm64_debug_driver);
}
module_exit(sec_arm64_debug_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Common debugging feature for ARM64 based devices");
MODULE_LICENSE("GPL v2");
