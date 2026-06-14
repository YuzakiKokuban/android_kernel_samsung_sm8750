// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/io.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/memremap.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/panic_notifier.h>
#include <linux/platform_device.h>

#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/debug/qcom/sec_qc_smem.h>
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_debug_symbol.h>

#include "sec_qc_summary.h"
#include "sec_qc_summary_external.h"

struct qc_summary_drvdata *qc_summary;

static __always_inline bool __summary_is_probed(void)
{
	return !!qc_summary;
}

struct sec_qc_summary_data_modem *sec_qc_summary_get_modem(void)
{
	if (!__summary_is_probed())
		return ERR_PTR(-EBUSY);

	return secdbg_modem(qc_summary);
}
EXPORT_SYMBOL_GPL(sec_qc_summary_get_modem);

static noinline int __summary_parse_dt_die_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,die_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->nb_die.priority = (int)priority;

	return 0;
}

static noinline int __summary_parse_dt_panic_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,panic_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->nb_panic.priority = (int)priority;

	return 0;
}

static noinline int __summary_parse_dt_smem_offset(struct builder *bd, struct device_node *np)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	u32 smem_offset;
	int err;

	/* NOTE:
	 * android13-5.15.y and before : optional. if not found assign '0'.
	 * android14-6.1.y and after : mandatory.
	 */
	err = of_property_read_u32(np, "sec,smem_offset", &smem_offset);
	if (err)
		return -EINVAL;

	drvdata->smem_offset = (size_t)smem_offset;

	return 0;
}

/* NOTE: this fucntion doe not return error codes.
 * When this function fails in this function,
 * then, just, 'google,debug_kinfo' is not used by this module.
 */
static noinline int __summary_parse_dt_memory_region_google_debug_kinfo(
		struct builder *bd, struct device_node *np)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct device *dev = bd->dev;
	int idx;
	struct device_node *mem_np;
	struct reserved_mem *rmem;

	if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO) || IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		return 0;

	if (IS_ENABLED(CONFIG_QCOM_DEBUG_SYMBOL) && !debug_symbol_available())
		return -EPROBE_DEFER;

	idx = of_property_match_string(np, "memory-region-names",
			"google,debug_kinfo");
	mem_np = of_parse_phandle(np, "memory-region", idx);
	if (!mem_np) {
		dev_warn(dev, "failed to parse memory-region\n");
		return 0;
	}

	rmem = of_reserved_mem_lookup(mem_np);
	if (!rmem) {
		dev_warn(dev, "failed to get a reserved memory (%s)\n",
				mem_np->name);
		return 0;
	}

	drvdata->debug_kinfo_rmem = rmem;
	if (!rmem->priv) {
		/* NOTE: ioreamp permanently to prevent 'iounmap'
		 * by -EPROBE_DEFER
		 */
		void __iomem *kinfo_va = memremap(rmem->base, rmem->size, MEMREMAP_WB);

		memset(kinfo_va, 0x0, rmem->size);
		rmem->priv = kinfo_va;
		return -EPROBE_DEFER;
	}

	return 0;
}

static const struct dt_builder __summary_dt_builder[] = {
	DT_BUILDER(__summary_parse_dt_memory_region_google_debug_kinfo),
	/* concrete builders which can return -EPROBE_DEFER should be
	 * placed before here.
	 */
	DT_BUILDER(__summary_parse_dt_die_notifier_priority),
	DT_BUILDER(__summary_parse_dt_panic_notifier_priority),
	DT_BUILDER(__summary_parse_dt_smem_offset),
};

static noinline int __summary_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __summary_dt_builder,
			ARRAY_SIZE(__summary_dt_builder));
}

static noinline int __summary_alloc_summary_from_smem(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct device *dev = bd->dev;
	uint8_t *smem_region;
	struct sec_qc_summary *dbg_summary;
	const size_t sz_summary = sizeof(struct sec_qc_summary);
	const size_t sz_requested = sz_summary + drvdata->smem_offset;
	size_t sz_alloc = sz_requested;
	int err;

	/* set summary address in smem for other subsystems to see */
	err = qcom_smem_alloc(QCOM_SMEM_HOST_ANY, SMEM_ID_VENDOR2, sz_alloc);
	if (err && err != -EEXIST) {
		dev_err(dev, "smem alloc failed! (%d)\n", err);
		return err;
	}

	smem_region = qcom_smem_get(QCOM_SMEM_HOST_ANY, SMEM_ID_VENDOR2, &sz_alloc);
	if (!smem_region) {
		dev_err(dev, "smem get failed!\n");
		return -ENOENT;
	} else if (sz_alloc < sz_requested) {
		dev_err(dev, "smem is too small (%zu < %zu)!\n", sz_alloc, sz_requested);
		return -ENOMEM;
	}

	dbg_summary = (struct sec_qc_summary *)&smem_region[drvdata->smem_offset];
	drvdata->summary = dbg_summary;

	return 0;
}

static noinline int __summary_probe_prolog(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct device *dev = bd->dev;
	struct sec_qc_summary *dbg_summary = drvdata->summary;
	struct sec_qc_summary_data_apss *dbg_apss = secdbg_apss(drvdata);

	memset_io(dbg_summary, 0x0, sizeof(*dbg_summary));

	dbg_summary->_apss = qcom_smem_virt_to_phys(&dbg_summary->priv.apss);
	dbg_summary->_rpm = qcom_smem_virt_to_phys(&dbg_summary->priv.rpm);
	dbg_summary->_modem = qcom_smem_virt_to_phys(&dbg_summary->priv.modem);
	dbg_summary->_dsps = qcom_smem_virt_to_phys(&dbg_summary->priv.dsps);

	dev_info(dev, "apss(%llx) rpm(%llx) modem(%llx) dsps(%llx)\n",
			dbg_summary->_apss, dbg_summary->_rpm,
			dbg_summary->_modem, dbg_summary->_dsps);

	strlcpy(dbg_apss->name, "APSS", sizeof(dbg_apss->name));
	strlcpy(dbg_apss->state, "Init", sizeof(dbg_apss->state));
	dbg_apss->nr_cpus = num_present_cpus();

	sec_qc_summary_set_msm_memdump_info(dbg_apss);
	sec_qc_summary_set_rtb_info(dbg_apss);

	return 0;
}

static int sec_qc_summary_die_handler(struct notifier_block *this,
		unsigned long val, void *data)
{
	struct qc_summary_drvdata *drvdata =
			container_of(this, struct qc_summary_drvdata, nb_die);
	struct sec_qc_summary_data_apss *dbg_apss = secdbg_apss(drvdata);
	struct die_args *args = data;
	struct pt_regs *regs = args->regs;

	snprintf(dbg_apss->excp.pc_sym, sizeof(dbg_apss->excp.pc_sym),
			"%pS", (void *)instruction_pointer(regs));
#if IS_ENABLED(CONFIG_ARM64)
	snprintf(dbg_apss->excp.lr_sym, sizeof(dbg_apss->excp.lr_sym),
			"%pS", (void *)regs->regs[30]);
#endif

	return NOTIFY_OK;
}

static int __summary_register_die_notifier(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	drvdata->nb_die.notifier_call = sec_qc_summary_die_handler;

	return register_die_notifier(&drvdata->nb_die);
}

static void __summary_unregister_die_notifier(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	unregister_die_notifier(&drvdata->nb_die);
}

static int sec_qc_summary_panic_handler(struct notifier_block *this,
		unsigned long val, void *data)
{
	struct qc_summary_drvdata *drvdata =
			container_of(this, struct qc_summary_drvdata, nb_panic);
	struct sec_qc_summary_data_apss *dbg_apss = secdbg_apss(drvdata);
	const void *caller;
	const char *str;

	caller = __builtin_return_address(2);
	str = data;

	snprintf(dbg_apss->excp.panic_caller,
			sizeof(dbg_apss->excp.panic_caller),
			"%pS", (void *)caller);
	snprintf(dbg_apss->excp.panic_msg,
			sizeof(dbg_apss->excp.panic_msg),
			"%s", str);
	snprintf(dbg_apss->excp.thread,
			sizeof(dbg_apss->excp.thread),
			"%s:%d", current->comm, task_pid_nr(current));

	return NOTIFY_OK;
}

static int __summary_register_panic_notifier(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	drvdata->nb_panic.notifier_call = sec_qc_summary_panic_handler;

	return atomic_notifier_chain_register(&panic_notifier_list,
			&drvdata->nb_panic);
}

static void __summary_unregister_panic_notifier(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	atomic_notifier_chain_unregister(&panic_notifier_list,
			&drvdata->nb_panic);
}

static noinline int __summary_set_drvdata(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);

	return 0;
}

static void __summary_iounmap_debug_kinfo(struct qc_summary_drvdata *drvdata)
{
	if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO) || IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		return;

	if (IS_ENABLED(CONFIG_QCOM_DEBUG_SYMBOL))
		return;		/* kinfo region is managed by QC's debug_symbol driver */

	if (!sec_debug_is_enabled())
		memunmap(drvdata->debug_kinfo_rmem->priv);
}

static noinline int __summary_probe_epilog(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct sec_qc_summary *dbg_summary = drvdata->summary;

	__summary_iounmap_debug_kinfo(drvdata);

	/* fill magic nubmer last to ensure data integrity when the magic
	 * numbers are written
	 */
	dbg_summary->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	dbg_summary->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	dbg_summary->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	dbg_summary->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;

	qc_summary = drvdata;

	return 0;
}

static noinline void __summary_remove_prolog(struct builder *bd)
{
	qc_summary = NULL;
}

static const struct dev_builder __summary_dev_builder[] = {
	DEVICE_BUILDER(__summary_parse_dt, NULL),
	DEVICE_BUILDER(__summary_alloc_summary_from_smem, NULL),
	DEVICE_BUILDER(__qc_summary_debug_kinfo_init, NULL),
	DEVICE_BUILDER(__summary_probe_prolog, NULL),
	DEVICE_BUILDER(__qc_summary_arm64_ap_context_init,
		       __qc_summary_arm64_ap_context_exit),
	/* concrete builders which can return -EPROBE_DEFER should be
	 * placed before here.
	 */
	DEVICE_BUILDER(__summary_register_die_notifier,
		       __summary_unregister_die_notifier),
	DEVICE_BUILDER(__summary_register_panic_notifier,
		       __summary_unregister_panic_notifier),
	DEVICE_BUILDER(__summary_set_drvdata, NULL),
};

static const struct dev_builder __summary_dev_builder_threaded[] = {
	DEVICE_BUILDER(__qc_summary_kallsyms_init, NULL),
	DEVICE_BUILDER(__qc_summary_info_mon_init, NULL),
	DEVICE_BUILDER(__qc_summary_var_mon_init_last_pet,
		       __qc_summary_var_mon_exit_last_pet),
	DEVICE_BUILDER(__qc_summary_var_mon_init_last_ns, NULL),
	DEVICE_BUILDER(__qc_summary_task_init, NULL),
	DEVICE_BUILDER(__qc_summary_kconst_init, NULL),
	DEVICE_BUILDER(__qc_summary_ksyms_init, NULL),
	DEVICE_BUILDER(__qc_summary_sched_log_init, NULL),
	DEVICE_BUILDER(__qc_summary_aplpm_init, NULL),
	DEVICE_BUILDER(__qc_summary_coreinfo_init,
		       __qc_summary_coreinfo_exit),
	DEVICE_BUILDER(__qc_summary_dump_sink_init, NULL),
	DEVICE_BUILDER(__summary_probe_epilog, __summary_remove_prolog),
};

static int __summary_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct qc_summary_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __summary_probe_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_summary_drvdata *drvdata = platform_get_drvdata(pdev);

	return sec_director_probe_dev_threaded(&drvdata->bd, builder, n,
			"qc_summary");
}

static int __summary_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_summary_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int __summary_remove_threaded(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct qc_summary_drvdata *drvdata = platform_get_drvdata(pdev);
	struct director_threaded *drct = drvdata->bd.drct;

	sec_director_destruct_dev_threaded(drct);

	return 0;
}

static int sec_qc_summary_probe(struct platform_device *pdev)
{
	int err;

	err = __summary_probe(pdev, __summary_dev_builder,
			ARRAY_SIZE(__summary_dev_builder));
	if (err)
		return err;

	return __summary_probe_threaded(pdev, __summary_dev_builder_threaded,
			ARRAY_SIZE(__summary_dev_builder_threaded));
}

static int sec_qc_summary_remove(struct platform_device *pdev)
{
	__summary_remove_threaded(pdev, __summary_dev_builder_threaded,
			ARRAY_SIZE(__summary_dev_builder_threaded));

	__summary_remove(pdev, __summary_dev_builder,
			ARRAY_SIZE(__summary_dev_builder));

	return 0;
}

static const struct of_device_id sec_qc_summary_match_table[] = {
	{ .compatible = "samsung,qcom-summary" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_summary_match_table);

static struct platform_driver sec_qc_summary_driver = {
	.driver = {
		.name = "sec,qc-summary",
		.of_match_table = of_match_ptr(sec_qc_summary_match_table),
	},
	.probe = sec_qc_summary_probe,
	.remove = sec_qc_summary_remove,
};

static int __init sec_qc_summary_init(void)
{
	return platform_driver_register(&sec_qc_summary_driver);
}
module_init(sec_qc_summary_init);

static void __exit sec_qc_summary_exit(void)
{
	platform_driver_unregister(&sec_qc_summary_driver);
}
module_exit(sec_qc_summary_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Summary for Qualcomm based devices");
MODULE_LICENSE("GPL v2");

MODULE_SOFTDEP("pre: sec_arm64_ap_context");
MODULE_SOFTDEP("pre: sec_qc_logger");
