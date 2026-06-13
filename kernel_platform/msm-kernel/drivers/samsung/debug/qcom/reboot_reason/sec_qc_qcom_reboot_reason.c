// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

/* NOTE: This file is a counter part of QC's
 * 'drivers/power/reset/qcom-dload-mode.c' and
 * 'dirvers/power/reset/qcom-reboot-reason.c'.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/panic_notifier.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/slab.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_reboot_cmd.h>
#include <linux/samsung/debug/qcom/sec_qc_rbcmd.h>

/* TODO: implemented by SAMSUNG
 * Location : drivers/power/reset/qcom-dload-mode.c
 */
extern void qcom_set_dload_mode(int on);

struct qc_reboot_reason_drvdata {
	struct builder bd;
	struct nvmem_cell *nv_restart_reason;
	struct nvmem_cell *nv_pon_reason;
	/* NOTE: This address is used in 'msm-poweroff' module which is
	 * located in the imem.
	 */
	u32 __iomem *qcom_restart_reason;
	struct notifier_block nb_pon_rr;
	struct notifier_block nb_sec_rr;
	struct notifier_block nb_reboot;
	struct notifier_block nb_panic;
	bool in_panic;
};

static noinline int __qc_reboot_reason_parse_dt_reboot_notifier_priority(
		struct builder *bd, struct device_node *np)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,reboot_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->nb_reboot.priority = (int)priority;

	return 0;
}

static const struct dt_builder __qc_reboot_reason_dt_builder[] = {
	DT_BUILDER(__qc_reboot_reason_parse_dt_reboot_notifier_priority),
};

static noinline int __qc_reboot_reason_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __qc_reboot_reason_dt_builder,
			ARRAY_SIZE(__qc_reboot_reason_dt_builder));
}

static int __qc_reboot_reason_probe_prolog(struct builder *bd)
{
	/* NOTE: enable upload mode to debug sudden reset */
	qcom_set_dload_mode(1);

	return 0;
}

static void __qc_reboot_reason_remove_epilog(struct builder *bd)
{
	qcom_set_dload_mode(0);
}

static noinline int __qc_reboot_reason_get_nv_restart_reason(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);
	struct device *dev = bd->dev;
	struct nvmem_cell *nvmem_cell;

	nvmem_cell = devm_nvmem_cell_get(dev, "restart_reason");
	if (IS_ERR(nvmem_cell))
		return PTR_ERR(nvmem_cell);

	drvdata->nv_restart_reason = nvmem_cell;

	return 0;
}

static noinline int __qc_reboot_reason_get_nv_pon_reason(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);
	struct device *dev = bd->dev;
	struct nvmem_cell *nvmem_cell;

	nvmem_cell = devm_nvmem_cell_get(dev, "pon_reason");
	if (IS_ERR(nvmem_cell)) {
		dev_warn(dev, "pon_reason nvmem_cell is not ready in the devcie-tree\n");
		/* NOTE: this is an optional feature to check the power on
		 * reason for debuging for certaion conditions like SMPL.
		 */
		return 0;
	}

	/* TODO: implement code checking power on reasons at here */

	drvdata->nv_pon_reason = nvmem_cell;

	return 0;
}

static int __qc_reboot_reason_ioremap_qcom_restart_reason(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);
	struct device_node *mem_np;
	struct device *dev = bd->dev;
	u32 __iomem *qcom_restart_reason;

	mem_np = of_find_compatible_node(NULL, NULL,
			"qcom,msm-imem-restart_reason");
	if (!mem_np) {
		dev_err(dev, "unable to find DT imem restart reason node\n");
		return -ENODEV;
	}

	qcom_restart_reason = of_iomap(mem_np, 0);
	if (unlikely(!qcom_restart_reason)) {
		dev_err(dev, "unable to map imem restart reason offset\n");
		return -ENOMEM;
	}

	drvdata->qcom_restart_reason = qcom_restart_reason;

	return 0;
}

static void __qc_reboot_reason_iounmap_qcom_restart_reason(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	iounmap(drvdata->qcom_restart_reason);
}

static void __reboot_reason_write_pon_rr(struct qc_reboot_reason_drvdata *drvdata,
		unsigned char pon_reason)
{
	struct device *dev = drvdata->bd.dev;
	unsigned char pon_read;
	const size_t max_retry = 5;
	size_t retry;
	void *buf;
	int err;

	for (retry = 0; retry < max_retry; retry++) {
		err = nvmem_cell_write(drvdata->nv_restart_reason,
				&pon_reason, sizeof(pon_reason));
		if (err <= 0)
			continue;

		buf = nvmem_cell_read(drvdata->nv_restart_reason,
				NULL);
		if (IS_ERR(buf)) {
			kfree(buf);
			continue;
		}

		pon_read = *(unsigned char *)buf;
		kfree(buf);
		if (pon_read == pon_reason) {
			dev_info(dev, "0x%02hhX is written successfully. (retry = %zu)\n",
					pon_reason, retry);
			return;
		}
	}

	dev_warn(dev, "pon reason was not written properly!\n");
}

static int sec_qc_reboot_reason_write_pon_rr(struct notifier_block *this,
		unsigned long pon_rr, void *data)
{
	struct qc_reboot_reason_drvdata *drvdata = container_of(this,
			struct qc_reboot_reason_drvdata, nb_pon_rr);
	struct sec_reboot_param *param = data;

	if (param && param->mode == SYS_POWER_OFF)
		return NOTIFY_DONE;

	if (pon_rr == PON_RESTART_REASON_NOT_HANDLE)
		return NOTIFY_DONE;

	__reboot_reason_write_pon_rr(drvdata, (unsigned char)pon_rr);

	nvmem_cell_put(drvdata->nv_restart_reason);
	drvdata->nv_restart_reason = NULL;

	return NOTIFY_OK;
}

static int __qc_reboot_reason_register_pon_rr_writer(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);
	int err;

	drvdata->nb_pon_rr.notifier_call = sec_qc_reboot_reason_write_pon_rr;

	err = sec_qc_rbcmd_register_pon_rr_writer(&drvdata->nb_pon_rr);
	if (err == -EBUSY)
		return -EPROBE_DEFER;

	return err;
}

static void __qc_reboot_reason_unregister_pon_rr_writer(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	return sec_qc_rbcmd_unregister_pon_rr_writer(&drvdata->nb_pon_rr);
}

static int sec_qc_reboot_reason_write_sec_rr(struct notifier_block *this,
		unsigned long sec_rr, void *d)
{
	struct qc_reboot_reason_drvdata *drvdata = container_of(this,
			struct qc_reboot_reason_drvdata, nb_sec_rr);

	if (sec_rr == RESTART_REASON_NOT_HANDLE)
		return NOTIFY_DONE;

	__raw_writel((u32)sec_rr, drvdata->qcom_restart_reason);

	return NOTIFY_OK;
}

static int __qc_reboot_reason_register_sec_rr_writer(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	drvdata->nb_sec_rr.notifier_call = sec_qc_reboot_reason_write_sec_rr;

	return sec_qc_rbcmd_register_sec_rr_writer(&drvdata->nb_sec_rr);
}

static void __qc_reboot_reason_unregister_sec_rr_writer(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	return sec_qc_rbcmd_unregister_sec_rr_writer(&drvdata->nb_sec_rr);
}

static int sec_qc_reboot_reason_reboot_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	struct qc_reboot_reason_drvdata *drvdata = container_of(this,
			struct qc_reboot_reason_drvdata, nb_reboot);

	if (drvdata->in_panic)
		qcom_set_dload_mode(1);
	else
		qcom_set_dload_mode(0);

	return NOTIFY_OK;
}

static int __qc_reboot_reason_register_reboot_notifier(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);
	int err;

	drvdata->nb_reboot.notifier_call = sec_qc_reboot_reason_reboot_call;

	err = register_reboot_notifier(&drvdata->nb_reboot);
	if (err == -EBUSY)
		return -EPROBE_DEFER;

	return err;
}

static void __qc_reboot_reason_unregister_reboot_notifier(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	unregister_reboot_notifier(&drvdata->nb_reboot);
}

static int sec_qc_reboot_reason_panic_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	struct qc_reboot_reason_drvdata *drvdata =
		container_of(this, struct qc_reboot_reason_drvdata, nb_panic);
	struct device *dev = drvdata->bd.dev;

	qcom_set_dload_mode(1);

	dev_warn(dev, "reboot mode: 0x%08X\n", RESTART_REASON_SEC_DEBUG_MODE);
	__raw_writel(RESTART_REASON_SEC_DEBUG_MODE, drvdata->qcom_restart_reason);

	drvdata->in_panic = true;
	reboot_mode = REBOOT_WARM;

	return NOTIFY_OK;
}

static int __qc_reboot_reason_register_panic_notifier(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	drvdata->nb_panic.notifier_call = sec_qc_reboot_reason_panic_call;

	return atomic_notifier_chain_register(&panic_notifier_list,
			&drvdata->nb_panic);
}

static void __qc_reboot_reason_unregister_panic_notifier(struct builder *bd)
{
	struct qc_reboot_reason_drvdata *drvdata =
			container_of(bd, struct qc_reboot_reason_drvdata, bd);

	atomic_notifier_chain_unregister(&panic_notifier_list,
			&drvdata->nb_panic);
}

static int __qc_reboot_reason_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_reboot_reason_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __qc_reboot_reason_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_reboot_reason_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static const struct dev_builder __qc_reboot_reason_dev_builder[] = {
	DEVICE_BUILDER(__qc_reboot_reason_parse_dt, NULL),
	DEVICE_BUILDER(__qc_reboot_reason_probe_prolog,
		       __qc_reboot_reason_remove_epilog),
	DEVICE_BUILDER(__qc_reboot_reason_get_nv_restart_reason, NULL),
	DEVICE_BUILDER(__qc_reboot_reason_get_nv_pon_reason, NULL),
	DEVICE_BUILDER(__qc_reboot_reason_ioremap_qcom_restart_reason,
		       __qc_reboot_reason_iounmap_qcom_restart_reason),
	DEVICE_BUILDER(__qc_reboot_reason_register_pon_rr_writer,
		       __qc_reboot_reason_unregister_pon_rr_writer),
	DEVICE_BUILDER(__qc_reboot_reason_register_sec_rr_writer,
		       __qc_reboot_reason_unregister_sec_rr_writer),
	DEVICE_BUILDER(__qc_reboot_reason_register_reboot_notifier,
		       __qc_reboot_reason_unregister_reboot_notifier),
	DEVICE_BUILDER(__qc_reboot_reason_register_panic_notifier,
		       __qc_reboot_reason_unregister_panic_notifier),
};

static int sec_qc_reboot_reason_probe(struct platform_device *pdev)
{
	return __qc_reboot_reason_probe(pdev, __qc_reboot_reason_dev_builder,
			ARRAY_SIZE(__qc_reboot_reason_dev_builder));
}

static int sec_qc_reboot_reason_remove(struct platform_device *pdev)
{
	return __qc_reboot_reason_remove(pdev, __qc_reboot_reason_dev_builder,
			ARRAY_SIZE(__qc_reboot_reason_dev_builder));
}

static const struct of_device_id sec_qc_reboot_reason_match_table[] = {
	{ .compatible = "samsung,qcom-qcom_reboot_reason" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_reboot_reason_match_table);

static struct platform_driver sec_qc_reboot_reason_driver = {
	.driver = {
		.name = "sec,qc-qcom_reboot_reason",
		.of_match_table = of_match_ptr(sec_qc_reboot_reason_match_table),
	},
	.probe = sec_qc_reboot_reason_probe,
	.remove = sec_qc_reboot_reason_remove,
};

static int __init sec_qc_reboot_reason_init(void)
{
	return platform_driver_register(&sec_qc_reboot_reason_driver);
}
arch_initcall(sec_qc_reboot_reason_init);

static void __exit sec_qc_reboot_reason_exit(void)
{
	platform_driver_unregister(&sec_qc_reboot_reason_driver);
}
module_exit(sec_qc_reboot_reason_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Counter-part of Qualcomm's qcom-reboot-reason and qcom-dload-mode.");
MODULE_LICENSE("GPL v2");

