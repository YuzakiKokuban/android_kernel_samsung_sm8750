// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug_region.h>

#include "sec_qc_logger.h"

u64 notrace __qc_logger_cpu_clock(int cpu)
{
	static DEFINE_PER_CPU(u64, __cpu_clock);
	u64 *last_clk = &per_cpu(__cpu_clock, cpu);
	u64 curr_clk = cpu_clock(cpu);

	if (unlikely(*last_clk >= curr_clk))
		curr_clk = ++(*last_clk);
	else
		*last_clk = curr_clk;

	return curr_clk;
}

int __qc_logger_sub_module_probe(struct qc_logger *logger,
		const struct dev_builder *builder, ssize_t n)
{
	return sec_director_probe_dev(&logger->bd, builder, n);
}

void __qc_logger_sub_module_remove(struct qc_logger *logger,
		const struct dev_builder *builder, ssize_t n)
{
	sec_director_destruct_dev(&logger->bd, builder, n, n);
}

static noinline int __qc_logger_parse_dt_name(struct builder *bd, struct device_node *np)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);

	return of_property_read_string(np, "sec,name", &drvdata->name);
}

static noinline int __qc_logger_parse_dt_unique_id(struct builder *bd,
		struct device_node *np)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	u32 unique_id;
	int err;

	err = of_property_read_u32(np, "sec,unique_id", &unique_id);
	if (err)
		return -EINVAL;

	drvdata->unique_id = (uint32_t)unique_id;

	return 0;
}

static const struct dt_builder __qc_logger_dt_builder[] = {
	DT_BUILDER(__qc_logger_parse_dt_name),
	DT_BUILDER(__qc_logger_parse_dt_unique_id),
};

static noinline int __qc_logger_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __qc_logger_dt_builder,
			ARRAY_SIZE(__qc_logger_dt_builder));
}

static DEFINE_MUTEX(__concrete_logger_lock);

static struct qc_logger *__concrete_loggers[] = {
	&__qc_logger_sched_log,
	&__qc_logger_irq_log,
	&__qc_logger_irq_exit_log,
};

static int __qc_logger_pick_concrete_logger(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	struct qc_logger *logger = NULL;
	int err = -ENODEV;
	size_t i;

	mutex_lock(&__concrete_logger_lock);
	for (i = 0; i < ARRAY_SIZE(__concrete_loggers); i++) {
		logger = __concrete_loggers[i];

		if (drvdata->unique_id == logger->unique_id) {
			if (logger->drvdata) {	/* already used */
				err = -EBUSY;
				break;
			}

			logger->drvdata = drvdata;
			drvdata->logger = logger;
			err = 0;
			break;
		}
	}
	mutex_unlock(&__concrete_logger_lock);

	return err;
}

static void __qc_logger_unpick_concrete_logger(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	struct qc_logger *logger = drvdata->logger;

	mutex_lock(&__concrete_logger_lock);

	logger->drvdata = NULL;
	drvdata->logger = NULL;

	mutex_unlock(&__concrete_logger_lock);
}

static int __qc_logger_alloc_client(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	struct qc_logger *logger = drvdata->logger;
	size_t size;
	struct sec_dbg_region_client *client;

	size = logger->get_data_size(logger);

	client = sec_dbg_region_alloc(drvdata->unique_id, size);
	if (PTR_ERR(client) == -EBUSY)
		return -EPROBE_DEFER;
	else if (IS_ERR_OR_NULL(client))
		return -ENOMEM;

	client->name = drvdata->name;
	drvdata->client = client;

	return 0;
}

static void __qc_logger_free_client(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);

	sec_dbg_region_free(drvdata->client);
}

static int __qc_logger_call_concrete_logger_probe(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	struct qc_logger *logger = drvdata->logger;

	return logger->probe(logger);
}

static void __qc_logger_call_concrete_logger_remove(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	struct qc_logger *logger = drvdata->logger;

	logger->remove(logger);
}

static noinline int __qc_logger_probe_epilog(struct builder *bd)
{
	struct qc_logger_drvdata *drvdata =
			container_of(bd, struct qc_logger_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static int __qc_logger_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_logger_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __qc_logger_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_logger_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __qc_logger_dev_builder[] = {
	DEVICE_BUILDER(__qc_logger_parse_dt, NULL),
	DEVICE_BUILDER(__qc_logger_pick_concrete_logger,
		       __qc_logger_unpick_concrete_logger),
	DEVICE_BUILDER(__qc_logger_alloc_client,
		       __qc_logger_free_client),
	DEVICE_BUILDER(__qc_logger_call_concrete_logger_probe,
		       __qc_logger_call_concrete_logger_remove),
	DEVICE_BUILDER(__qc_logger_probe_epilog, NULL),
};

static int sec_qc_logger_probe(struct platform_device *pdev)
{
	return __qc_logger_probe(pdev, __qc_logger_dev_builder,
			ARRAY_SIZE(__qc_logger_dev_builder));
}

static int sec_qc_logger_remove(struct platform_device *pdev)
{
	return __qc_logger_remove(pdev, __qc_logger_dev_builder,
			ARRAY_SIZE(__qc_logger_dev_builder));
}

static const struct of_device_id sec_qc_logger_match_table[] = {
	{ .compatible = "samsung,qcom-logger" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_logger_match_table);

static struct platform_driver sec_qc_logger_driver = {
	.driver = {
		.name = "sec,qc-logger",
		.of_match_table = of_match_ptr(sec_qc_logger_match_table),
	},
	.probe = sec_qc_logger_probe,
	.remove = sec_qc_logger_remove,
};

static int __init sec_qc_logger_init(void)
{
	return platform_driver_register(&sec_qc_logger_driver);
}
arch_initcall(sec_qc_logger_init);

static void __exit sec_qc_logger_exit(void)
{
	platform_driver_unregister(&sec_qc_logger_driver);
}
module_exit(sec_qc_logger_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Logger for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
