/*
 * sec_pm_regulator.c
 *
 *  Copyright (c) 2024 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Author: Jonghyeon Cho <jongjaaa.cho@samsung.com>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/of.h>

#define DEFAULT_LATE_CLEANUP_WAKELOCK_TIMEOUT_MS 35000

struct sec_pm_regulator_info {
	struct device *dev;
	struct device_node *np;

	struct wakeup_source *regulator_init_ws;
	unsigned int late_cleanup_wakelock_timeout;
};

static int sec_pm_regulator_parse_dt(struct platform_device *pdev)
{
	struct sec_pm_regulator_info *info = platform_get_drvdata(pdev);

	if (!info || !pdev->dev.of_node)
		return -ENODEV;

	info->np = pdev->dev.of_node;

	if (of_property_read_u32(info->np, "late_cleanup_wakelock",
							&info->late_cleanup_wakelock_timeout))
		info->late_cleanup_wakelock_timeout = DEFAULT_LATE_CLEANUP_WAKELOCK_TIMEOUT_MS;

	return 0;
}

static int sec_pm_regulator_probe(struct platform_device *pdev)
{
	struct sec_pm_regulator_info *info;
	int ret = 0;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "%s: Fail to alloc info\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, info);
	info->dev = &pdev->dev;

	ret = sec_pm_regulator_parse_dt(pdev);
	if (ret) {
		dev_err(info->dev, "%s: Fail to parse device tree\n", __func__);
		goto probe_end;
	}

	info->regulator_init_ws = wakeup_source_register(NULL, "regulator_init_ws");
	if (info->regulator_init_ws) {
		__pm_wakeup_event(info->regulator_init_ws, info->late_cleanup_wakelock_timeout);
		pr_info("%s: acquire wakelock %dms\n", __func__, info->late_cleanup_wakelock_timeout);
	} else {
		dev_err(info->dev, "%s: Fail to register wakeup_source\n", __func__);
		ret = -EPERM;
	}

probe_end:
	return ret;
}

static const struct of_device_id sec_pm_regulator_match[] = {
	{ .compatible = "samsung,sec-pm-regulator", },
	{ },
};

static struct platform_driver sec_pm_regulator_driver = {
	.driver = {
		.name = "sec-pm-regulator",
		.of_match_table = of_match_ptr(sec_pm_regulator_match),
	},
	.probe = sec_pm_regulator_probe,
};

module_platform_driver(sec_pm_regulator_driver);

MODULE_AUTHOR("Jonghyeon Cho <jongjaaa.cho@samsung.com>");
MODULE_DESCRIPTION("System Power Regulator debugging driver");
MODULE_LICENSE("GPL");

