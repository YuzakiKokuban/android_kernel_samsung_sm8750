// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/panic_notifier.h>
#include <linux/platform_device.h>
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_force_err.h>
#include <linux/samsung/debug/qcom/sec_qc_rbcmd.h>
#include <linux/samsung/debug/qcom/sec_qc_upload_cause.h>

#include "sec_qc_qcom_wdt_core.h"

static noinline int __qc_wdt_core_parse_dt_qcom_wdt_core_dev_name(struct builder *bd,
		struct device_node *np)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);
	const char *qcom_wdt_core_dev_name;
	struct device *found;
	int err;

	err = of_property_read_string(np, "sec,qcom_wdt_core_dev_name",
			&qcom_wdt_core_dev_name);
	if (err)
		return err;

	found = bus_find_device_by_name(&platform_bus_type, NULL,
			qcom_wdt_core_dev_name);
	if (!found)
		return -EPROBE_DEFER;

	drvdata->wdog_dd = dev_get_drvdata(found);

	return 0;
}

static noinline int __qc_wdt_core_parse_dt_panic_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,panic_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->nb_panic.priority = (int)priority;

	return 0;
}

static const struct dt_builder __qc_wdt_core_dt_builder[] = {
	DT_BUILDER(__qc_wdt_core_parse_dt_qcom_wdt_core_dev_name),
	DT_BUILDER(__qc_wdt_core_parse_dt_panic_notifier_priority),
};

static noinline int __qc_wdt_core_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __qc_wdt_core_dt_builder,
			ARRAY_SIZE(__qc_wdt_core_dt_builder));
}

static unsigned long long last_emerg_pet __used;

static void __qc_wdt_core_emerg_pet_watchdog(struct qc_wdt_core_drvdata *drvdata)
{
	struct msm_watchdog_data *wdog_dd = drvdata->wdog_dd;

	if (!wdog_dd->enabled)
		return;

	wdog_dd->ops->enable_wdt(1, wdog_dd);
	wdog_dd->ops->reset_wdt(wdog_dd);

	last_emerg_pet = sched_clock();
}

static int sec_qc_wdt_core_panic_notifier_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(this, struct qc_wdt_core_drvdata, nb_panic);

	__qc_wdt_core_emerg_pet_watchdog(drvdata);

	return NOTIFY_OK;
}

static int __qc_wdt_core_register_panic_handler(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);

	drvdata->nb_panic.notifier_call = sec_qc_wdt_core_panic_notifier_call;

	return atomic_notifier_chain_register(&panic_notifier_list,
			&drvdata->nb_panic);
}

static void __qc_wdt_core_unregister_panic_handler(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);

	atomic_notifier_chain_unregister(&panic_notifier_list,
			&drvdata->nb_panic);
}

static int __qc_wdt_core_bark_notifier_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	struct qc_wdt_core_drvdata *drvdata = container_of(this,
			struct qc_wdt_core_drvdata, nb_wdt_bark);
	struct msm_watchdog_data *wdog_dd = drvdata->wdog_dd;

	sec_qc_rbcmd_set_restart_reason(PON_RESTART_REASON_NOT_HANDLE,
			RESTART_REASON_SEC_DEBUG_MODE, NULL);
	sec_qc_upldc_write_cause(UPLOAD_CAUSE_NON_SECURE_WDOG_BARK);
	__qc_wdt_core_emerg_pet_watchdog(drvdata);
	sched_show_task(wdog_dd->watchdog_task);
	if (IS_BUILTIN(CONFIG_SEC_QC_QCOM_WDT_CORE))
		smp_send_stop();

	return NOTIFY_OK;
}

static int sec_qc_wdt_core_bark_notifier_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	static atomic_t cnt = ATOMIC_INIT(1);

	/* NOTE: to ensure one-shot */
	if (atomic_dec_if_positive(&cnt) < 0)
		return NOTIFY_DONE;

	return __qc_wdt_core_bark_notifier_call(this, l, d);
}

static int __qc_wdt_core_register_bark_handler(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);

	drvdata->nb_wdt_bark.notifier_call = sec_qc_wdt_core_bark_notifier_call;

	return qcom_wdt_bark_register_notifier(&drvdata->nb_wdt_bark);
}

static void __qc_wdt_core_unregister_bark_handler(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);

	qcom_wdt_bark_unregister_notifier(&drvdata->nb_wdt_bark);
}

static void __qc_wdt_force_watchdog_bark(struct force_err_handle *h)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(h, struct qc_wdt_core_drvdata, force_err_dp);
	struct msm_watchdog_data *wdog_dd = drvdata->wdog_dd;
	struct device *dev = drvdata->bd.dev;

	qcom_lpm_set_sleep_disabled();

	dev_err(dev, "Causing a QCOM Apps Watchdog bark!\n");
	wdog_dd->ops->show_wdt_status(wdog_dd);
	wdog_dd->ops->set_bark_time(1, wdog_dd);
	wdog_dd->ops->reset_wdt(wdog_dd);
	/* Delay to make sure bark occurs */
	mdelay(10000);
}

static int __qc_wdt_core_add_force_err_dp(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);
	struct force_err_handle *force_err = &drvdata->force_err_dp;
	int err;

	force_err->val = "DP";
	force_err->func = __qc_wdt_force_watchdog_bark;

	err = sec_force_err_add_custom_handle(force_err);
	if (err < 0)
		dev_warn(bd->dev, "DP - force err is disabled. ignored.\n");

	return 0;
}

static void __qc_wdt_core_del_force_err_dp(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);
	struct force_err_handle *force_err = &drvdata->force_err_dp;

	sec_force_err_del_custom_handle(force_err);
}

static void __qc_wdt_force_watchdog_bite(struct force_err_handle *h)
{
	qcom_wdt_trigger_bite();
}

static int __qc_wdt_core_add_force_err_wp(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);
	struct force_err_handle *force_err = &drvdata->force_err_wp;
	int err;

	force_err->val = "WP";
	force_err->func = __qc_wdt_force_watchdog_bite;

	err = sec_force_err_add_custom_handle(force_err);
	if (err < 0)
		dev_warn(bd->dev, "WP - force err is disabled. ignored.\n");

	return 0;
}

static void __qc_wdt_core_del_force_err_wp(struct builder *bd)
{
	struct qc_wdt_core_drvdata *drvdata =
			container_of(bd, struct qc_wdt_core_drvdata, bd);
	struct force_err_handle *force_err = &drvdata->force_err_wp;

	sec_force_err_del_custom_handle(force_err);
}

static int __qc_wdt_core_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_wdt_core_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __qc_wdt_core_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_wdt_core_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __qc_wdt_core_dev_builder[] = {
	DEVICE_BUILDER(__qc_wdt_core_parse_dt, NULL),
	DEVICE_BUILDER(__qc_wdt_core_register_panic_handler,
		       __qc_wdt_core_unregister_panic_handler),
	DEVICE_BUILDER(__qc_wdt_core_register_bark_handler,
		       __qc_wdt_core_unregister_bark_handler),
	DEVICE_BUILDER(__qc_wdt_core_add_force_err_dp,
		       __qc_wdt_core_del_force_err_dp),
	DEVICE_BUILDER(__qc_wdt_core_add_force_err_wp,
		       __qc_wdt_core_del_force_err_wp),
};

static int sec_qc_wdt_core_probe(struct platform_device *pdev)
{
	return __qc_wdt_core_probe(pdev, __qc_wdt_core_dev_builder,
			ARRAY_SIZE(__qc_wdt_core_dev_builder));
}

static int sec_qc_wdt_core_remove(struct platform_device *pdev)
{
	return __qc_wdt_core_remove(pdev, __qc_wdt_core_dev_builder,
			ARRAY_SIZE(__qc_wdt_core_dev_builder));
}

static const struct of_device_id sec_qc_wdt_core_match_table[] = {
	{ .compatible = "samsung,qcom-wdt_core" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_wdt_core_match_table);

static struct platform_driver sec_qc_wdt_core_driver = {
	.driver = {
		.name = "sec,qc-wdt_core",
		.of_match_table = of_match_ptr(sec_qc_wdt_core_match_table),
	},
	.probe = sec_qc_wdt_core_probe,
	.remove = sec_qc_wdt_core_remove,
};

static __init int sec_qc_wdt_core_init(void)
{
	return platform_driver_register(&sec_qc_wdt_core_driver);
}
module_init(sec_qc_wdt_core_init);

static __exit void sec_qc_wdt_core_exit(void)
{
	platform_driver_unregister(&sec_qc_wdt_core_driver);
}
module_exit(sec_qc_wdt_core_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Additional feature for QTI Watchdogr");
MODULE_LICENSE("GPL v2");
