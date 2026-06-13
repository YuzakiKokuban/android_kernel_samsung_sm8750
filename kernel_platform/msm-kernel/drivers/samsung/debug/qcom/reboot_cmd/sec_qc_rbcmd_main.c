// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <linux/samsung/sec_kunit.h>

#include "sec_qc_rbcmd.h"

struct qc_rbcmd_drvdata *qc_rbcmd;

__ss_static int __rbcmd_register_pon_rr_writer(
		struct qc_rbcmd_drvdata *drvdata, struct notifier_block *nb)
{
	return raw_notifier_chain_register(&drvdata->pon_rr_writers, nb);
}

int sec_qc_rbcmd_register_pon_rr_writer(struct notifier_block *nb)
{
	if (!__rbcmd_is_probed())
		return -EBUSY;

	return __rbcmd_register_pon_rr_writer(qc_rbcmd, nb);
}
EXPORT_SYMBOL_GPL(sec_qc_rbcmd_register_pon_rr_writer);

__ss_static void __rbcmd_unregister_pon_rr_writer(
		struct qc_rbcmd_drvdata *drvdata, struct notifier_block *nb)
{
	raw_notifier_chain_unregister(&drvdata->pon_rr_writers, nb);
}

void sec_qc_rbcmd_unregister_pon_rr_writer(struct notifier_block *nb)
{
	if (!__rbcmd_is_probed())
		return /* -EBUSY */;

	__rbcmd_unregister_pon_rr_writer(qc_rbcmd, nb);
}
EXPORT_SYMBOL_GPL(sec_qc_rbcmd_unregister_pon_rr_writer);

__ss_static void __rbcmd_write_pon_rr(struct qc_rbcmd_drvdata *drvdata,
		unsigned int pon_rr, struct sec_reboot_param *param)
{
	pr_warn("power on reason: 0x%08X\n", pon_rr);

	raw_notifier_call_chain(&drvdata->pon_rr_writers, pon_rr, param);
}

void __qc_rbcmd_write_pon_rr(unsigned int pon_rr,
		struct sec_reboot_param *param)
{
	if (!__rbcmd_is_probed())
		return /* -EBUSY */;

	__rbcmd_write_pon_rr(qc_rbcmd, pon_rr, param);
}

__ss_static int __rbcmd_register_sec_rr_writer(
		struct qc_rbcmd_drvdata *drvdata, struct notifier_block *nb)
{
	int err;

	err = raw_notifier_chain_register(&drvdata->sec_rr_writers, nb);
	if (err)
		return err;

	/* NOTE: set to 'RESTART_REASON_SEC_DEBUG_MODE' by default to detect
	 * sudden reset.
	 */
	pr_warn("default reboot mode: 0x%08X (for %pS)\n",
			RESTART_REASON_SEC_DEBUG_MODE,
			nb->notifier_call);
	nb->notifier_call(nb, RESTART_REASON_SEC_DEBUG_MODE, NULL);

	return 0;
}

int sec_qc_rbcmd_register_sec_rr_writer(struct notifier_block *nb)
{
	if (!__rbcmd_is_probed())
		return -EBUSY;

	return __rbcmd_register_sec_rr_writer(qc_rbcmd, nb);
}
EXPORT_SYMBOL_GPL(sec_qc_rbcmd_register_sec_rr_writer);

__ss_static void __rbcmd_unregister_sec_rr_writer(
		struct qc_rbcmd_drvdata *drvdata, struct notifier_block *nb)
{
	raw_notifier_chain_unregister(&drvdata->sec_rr_writers, nb);
}

void sec_qc_rbcmd_unregister_sec_rr_writer(struct notifier_block *nb)
{
	if (!__rbcmd_is_probed())
		return /* -EBUSY */;

	__rbcmd_unregister_sec_rr_writer(qc_rbcmd, nb);
}
EXPORT_SYMBOL_GPL(sec_qc_rbcmd_unregister_sec_rr_writer);

__ss_static void __rbcmd_write_sec_rr(struct qc_rbcmd_drvdata *drvdata,
		unsigned int sec_rr, struct sec_reboot_param *param)
{
	pr_warn("reboot mode: 0x%08X\n", sec_rr);

	raw_notifier_call_chain(&drvdata->sec_rr_writers, sec_rr, param);
}

void __qc_rbcmd_write_sec_rr(unsigned int sec_rr,
		struct sec_reboot_param *param)
{
	if (!__rbcmd_is_probed())
		return /* -EBUSY */;

	__rbcmd_write_sec_rr(qc_rbcmd, sec_rr, param);
}

void __qc_rbcmd_set_restart_reason(unsigned int pon_rr, unsigned int sec_rr,
		struct sec_reboot_param *param)
{
	__rbcmd_write_pon_rr(qc_rbcmd, pon_rr, param);
	__rbcmd_write_sec_rr(qc_rbcmd, sec_rr, param);
}

void sec_qc_rbcmd_set_restart_reason(unsigned int pon_rr, unsigned int sec_rr,
		struct sec_reboot_param *param)
{
	if (!__rbcmd_is_probed())
		return /* -EBUSY */;

	__qc_rbcmd_set_restart_reason(pon_rr, sec_rr, param);
}
EXPORT_SYMBOL_GPL(sec_qc_rbcmd_set_restart_reason);

__ss_static noinline int __rbcmd_parse_dt_use_on_reboot(struct builder *bd,
		struct device_node *np)
{
	struct qc_rbcmd_drvdata *drvdata =
			container_of(bd, struct qc_rbcmd_drvdata, bd);

	drvdata->use_on_reboot =
			of_property_read_bool(np, "sec,use-on_reboot");

	return 0;
}

__ss_static noinline int __rbcmd_parse_dt_use_on_restart(struct builder *bd,
		struct device_node *np)
{
	struct qc_rbcmd_drvdata *drvdata =
			container_of(bd, struct qc_rbcmd_drvdata, bd);

	drvdata->use_on_restart =
			of_property_read_bool(np, "sec,use-on_restart");

	return 0;
}

static const struct dt_builder __qc_rbcmd_dt_builder[] = {
	DT_BUILDER(__rbcmd_parse_dt_use_on_reboot),
	DT_BUILDER(__rbcmd_parse_dt_use_on_restart),
};

static noinline int __rbcmd_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __qc_rbcmd_dt_builder,
			ARRAY_SIZE(__qc_rbcmd_dt_builder));
}

static noinline int __rbcmd_probe_prolog(struct builder *bd)
{
	struct qc_rbcmd_drvdata *drvdata =
			container_of(bd, struct qc_rbcmd_drvdata, bd);

	RAW_INIT_NOTIFIER_HEAD(&drvdata->pon_rr_writers);
	RAW_INIT_NOTIFIER_HEAD(&drvdata->sec_rr_writers);

	return 0;
}

static noinline int __rbcmd_set_drvdata(struct builder *bd)
{
	struct qc_rbcmd_drvdata *drvdata =
			container_of(bd, struct qc_rbcmd_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static int __rbcmd_probe_epilog(struct builder *bd)
{
	struct qc_rbcmd_drvdata *drvdata =
			container_of(bd, struct qc_rbcmd_drvdata, bd);

	qc_rbcmd = drvdata;

	return 0;
}

static void __rbcmd_remove_prolog(struct builder *bd)
{
	qc_rbcmd = NULL;
}

static int __rbcmd_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_rbcmd_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __rbcmd_probe_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_rbcmd_drvdata *drvdata = platform_get_drvdata(pdev);

	return sec_director_probe_dev_threaded(&drvdata->bd, builder, n,
			"qc_rbcmd");
}

static int __rbcmd_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_rbcmd_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int __rbcmd_remove_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_rbcmd_drvdata *drvdata = platform_get_drvdata(pdev);
	struct director_threaded *drct = drvdata->bd.drct;

	sec_director_destruct_dev_threaded(drct);

	return 0;
}

static const struct dev_builder __qc_rbcmd_dev_builder[] = {
	DEVICE_BUILDER(__rbcmd_parse_dt, NULL),
	DEVICE_BUILDER(__rbcmd_probe_prolog, NULL),
	DEVICE_BUILDER(__qc_rbcmd_register_panic_handle,
		       __qc_rbcmd_unregister_panic_handle),
	DEVICE_BUILDER(__rbcmd_set_drvdata, NULL),
};

static const struct dev_builder __qc_rbcmd_dev_builder_threaded[] = {
	DEVICE_BUILDER(__qc_rbcmd_init_on_reboot,
		       __qc_rbcmd_exit_on_reboot),
	DEVICE_BUILDER(__qc_rbcmd_init_on_restart,
		       __qc_rbcmd_exit_on_restart),
	DEVICE_BUILDER(__rbcmd_probe_epilog,
		       __rbcmd_remove_prolog),
};

static int sec_qc_rbcmd_probe(struct platform_device *pdev)
{
	int err;

	err = __rbcmd_probe(pdev, __qc_rbcmd_dev_builder,
			ARRAY_SIZE(__qc_rbcmd_dev_builder));
	if (err)
		return err;

	return __rbcmd_probe_threaded(pdev, __qc_rbcmd_dev_builder_threaded,
			ARRAY_SIZE(__qc_rbcmd_dev_builder_threaded));
}

static int sec_qc_rbcmd_remove(struct platform_device *pdev)
{
	__rbcmd_remove_threaded(pdev, __qc_rbcmd_dev_builder_threaded,
			ARRAY_SIZE(__qc_rbcmd_dev_builder_threaded));

	__rbcmd_remove(pdev, __qc_rbcmd_dev_builder,
			ARRAY_SIZE(__qc_rbcmd_dev_builder));

	return 0;
}

static const struct of_device_id sec_qc_rbcmd_match_table[] = {
	{ .compatible = "samsung,qcom-reboot_cmd" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_rbcmd_match_table);

static struct platform_driver sec_qc_rbcmd_driver = {
	.driver = {
		.name = "sec,qc-reboot_cmd",
		.of_match_table = of_match_ptr(sec_qc_rbcmd_match_table),
	},
	.probe = sec_qc_rbcmd_probe,
	.remove = sec_qc_rbcmd_remove,
};

static __init int sec_qc_rbcmd_init(void)
{
	return platform_driver_register(&sec_qc_rbcmd_driver);
}
arch_initcall(sec_qc_rbcmd_init);

static __exit void sec_qc_rbcmd_exit(void)
{
	platform_driver_unregister(&sec_qc_rbcmd_driver);
}
module_exit(sec_qc_rbcmd_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Reboot command handlers for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
