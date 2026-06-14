// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/panic_notifier.h>
#include <linux/platform_device.h>
#include <linux/sched/clock.h>

#include <linux/samsung/sec_kunit.h>
#include <linux/samsung/debug/sec_crashkey.h>
#include <linux/samsung/debug/sec_log_buf.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/sec_of.h>

#include "sec_qc_rst_exinfo.h"

struct rst_exinfo_drvdata *qc_rst_exinfo;

static noinline int __rst_exinfo_parse_dt_memory_region(struct builder *bd,
		struct device_node *np)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	struct device *dev = bd->dev;
	struct device_node *mem_np;
	struct reserved_mem *rmem;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	rmem = of_reserved_mem_lookup(mem_np);
	if (!rmem) {
		dev_warn(dev, "failed to get a reserved memory (%s)\n",
				mem_np->name);
		return -EFAULT;
	}

	drvdata->rmem = rmem;

	return 0;
}

static bool __rst_exinfo_is_in_reserved_mem_bound(
		const struct reserved_mem *rmem,
		phys_addr_t base, phys_addr_t size)
{
	phys_addr_t rmem_base = rmem->base;
	phys_addr_t rmem_end = rmem_base + rmem->size - 1;
	phys_addr_t end = base + size - 1;

	if ((base >= rmem_base) && (end <= rmem_end))
		return true;

	return false;
}

static int __rst_exinfo_use_partial_reserved_mem(
	struct rst_exinfo_drvdata *drvdata, struct device_node *np)
{
	struct reserved_mem *rmem = drvdata->rmem;
	phys_addr_t base;
	phys_addr_t size;
	int err;

	err = sec_of_parse_reg_prop(np, &base, &size);
	if (err)
		return err;

	if (!__rst_exinfo_is_in_reserved_mem_bound(rmem, base, size))
		return -ERANGE;

	drvdata->paddr = base;
	drvdata->size = size;

	return 0;
}

static int __rst_exinfo_use_entire_reserved_mem(
	struct rst_exinfo_drvdata *drvdata)
{
	struct reserved_mem *rmem = drvdata->rmem;

	drvdata->paddr = rmem->base;
	drvdata->size = rmem->size;

	return 0;
}

static noinline int __rst_exinfo_parse_dt_partial_reserved_mem(struct builder *bd,
		struct device_node *np)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	int err;

	if (of_property_read_bool(np, "sec,use-partial_reserved_mem"))
		err = __rst_exinfo_use_partial_reserved_mem(drvdata, np);
	else
		err = __rst_exinfo_use_entire_reserved_mem(drvdata);

	if (err)
		return -EFAULT;

	return 0;
}

static void *__rst_exinfo_ioremap(struct rst_exinfo_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;

#if IS_ENABLED(CONFIG_HAS_IOMEM)
	return devm_ioremap_wc(dev, drvdata->paddr, drvdata->size);
#else
	dev = dev;
	return ioremap(drvdata->paddr, drvdata->size);
#endif
}

static noinline int __rst_exinfo_parse_dt_test_no_map(struct builder *bd,
		struct device_node *np)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	struct device *dev = bd->dev;
	struct device_node *mem_np;
	rst_exinfo_t *rst_exinfo;

	mem_np = of_parse_phandle(np, "memory-region", 0);
	if (!mem_np)
		return -EINVAL;

	if (!of_property_read_bool(mem_np, "no-map"))
		rst_exinfo = phys_to_virt(drvdata->paddr);
	else
		rst_exinfo = __rst_exinfo_ioremap(drvdata);

	drvdata->rst_exinfo = rst_exinfo;

	dev_info(dev, "ex info phy=%pa, size=0x%zx\n",
			&drvdata->paddr, drvdata->size);

	return 0;
}

__ss_static noinline int __rst_exinfo_dt_die_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,die_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->nb_die.priority = (int)priority;

	return 0;
}

__ss_static noinline int __rst_exinfo_dt_panic_notifier_priority(struct builder *bd,
		struct device_node *np)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	s32 priority;
	int err;

	err = of_property_read_s32(np, "sec,panic_notifier-priority",
			&priority);
	if (err)
		return -EINVAL;

	drvdata->nb_panic.priority = (int)priority;

	return 0;
}

static const struct dt_builder __rst_exinfo_dt_builder[] = {
	DT_BUILDER(__rst_exinfo_parse_dt_memory_region),
	DT_BUILDER(__rst_exinfo_parse_dt_partial_reserved_mem),
	DT_BUILDER(__rst_exinfo_dt_die_notifier_priority),
	DT_BUILDER(__rst_exinfo_dt_panic_notifier_priority),
	DT_BUILDER(__rst_exinfo_parse_dt_test_no_map),
};

static noinline int __rst_exinfo_parse_dt(struct builder *bd)
{
	return sec_director_parse_dt(bd, __rst_exinfo_dt_builder,
			ARRAY_SIZE(__rst_exinfo_dt_builder));
}

__ss_static noinline int __rst_exinfo_init_panic_extra_info(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	struct device *dev = bd->dev;
	rst_exinfo_t *rst_exinfo = drvdata->rst_exinfo;
	_kern_ex_info_t *kern_ex_info = &rst_exinfo->kern_ex_info.info;

	memset(&rst_exinfo->kern_ex_info, 0x0,
			sizeof(rst_exinfo->kern_ex_info));
	kern_ex_info->cpu = -1;

	dev_info(dev, "ex_info memory initialized size[%ld]\n",
			sizeof(kern_exinfo_t));

	return 0;
}

void __qc_rst_exinfo_store_extc_idx(struct rst_exinfo_drvdata *drvdata,
		bool prefix)
{
	rst_exinfo_t *rst_exinfo = drvdata->rst_exinfo;
	_kern_ex_info_t *kern_ex_info = &rst_exinfo->kern_ex_info.info;
	const struct sec_log_buf_head *s_log_buf;

	if (kern_ex_info->extc_idx != 0)
		return;

	s_log_buf = sec_log_buf_get_header();
	if (IS_ERR(s_log_buf))
		return;

	kern_ex_info->extc_idx = s_log_buf->idx;

	if (prefix)
		kern_ex_info->extc_idx += SEC_DEBUG_RESET_EXTRC_SIZE;
}

void sec_qc_rst_exinfo_store_extc_idx(bool prefix)
{
	if (!__qc_rst_exinfo_is_probed())
		return;

	__qc_rst_exinfo_store_extc_idx(qc_rst_exinfo, prefix);
}

void __qc_rst_exinfo_save_dying_msg(struct rst_exinfo_drvdata *drvdata,
		const char *str, const void *pc, const void *lr)
{
	rst_exinfo_t *rst_exinfo = drvdata->rst_exinfo;
	_kern_ex_info_t *kern_ex_info = &rst_exinfo->kern_ex_info.info;
	ssize_t len;
	char *msg;

	if (kern_ex_info->cpu != -1)
		return;

	kern_ex_info->cpu = smp_processor_id();
	snprintf(kern_ex_info->task_name, sizeof(kern_ex_info->task_name),
			"%s", current->comm);
	kern_ex_info->ktime = local_clock();
	snprintf(kern_ex_info->pc, sizeof(kern_ex_info->pc), "%pS", pc);
	snprintf(kern_ex_info->lr, sizeof(kern_ex_info->lr), "%pS", lr);

	msg = kern_ex_info->panic_buf;
	len = scnprintf(msg, sizeof(kern_ex_info->panic_buf), "%s", str);
	if ((len >= 1) && (msg[len - 1] == '\n'))
		msg[len - 1] = '\0';
}

void __weak __qc_arch_rst_exinfo_die_handler(struct rst_exinfo_drvdata *drvdata,
		enum die_val val, const struct die_args *args)
{
}

static int sec_qc_rst_exinfo_die_handler(struct notifier_block *this,
		unsigned long l, void *data)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(this, struct rst_exinfo_drvdata, nb_die);
	enum die_val val = l;
	struct die_args *args = data;

	__qc_arch_rst_exinfo_die_handler(drvdata, val, args);

	return NOTIFY_OK;
}

static int __rst_exinfo_register_die_notifier(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);

	drvdata->nb_die.notifier_call = sec_qc_rst_exinfo_die_handler;

	return register_die_notifier(&drvdata->nb_die);
}

static void __rst_exinfo_unregister_die_notifier(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);

	unregister_die_notifier(&drvdata->nb_die);
}

static int sec_qc_rst_exinfo_panic_handler(struct notifier_block *this,
		unsigned long val, void *data)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(this, struct rst_exinfo_drvdata, nb_panic);
	const void *pc;
	const void *lr;
	const char *str;

	pc = __builtin_return_address(2);
	lr = __builtin_return_address(3);
	str = data;

	__qc_rst_exinfo_store_extc_idx(drvdata, false);
	__qc_rst_exinfo_save_dying_msg(drvdata, str, pc, lr);

	return NOTIFY_OK;
}

static int __rst_exinfo_register_panic_notifier(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);

	drvdata->nb_panic.notifier_call = sec_qc_rst_exinfo_panic_handler;

	return atomic_notifier_chain_register(&panic_notifier_list,
			&drvdata->nb_panic);
}

static void __rst_exinfo_unregister_panic_notifier(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);

	atomic_notifier_chain_unregister(&panic_notifier_list,
			&drvdata->nb_panic);
}

static noinline int __rst_exinfo_probe_epilog(struct builder *bd)
{
	struct rst_exinfo_drvdata *drvdata =
			container_of(bd, struct rst_exinfo_drvdata, bd);
	struct device *dev = bd->dev;

	dev_set_drvdata(dev, drvdata);
	qc_rst_exinfo = drvdata;

	return 0;
}

static noinline void __rst_exinfo_remove_prolog(struct builder *bd)
{
	/* FIXME: This is not a graceful exit. */
	qc_rst_exinfo = NULL;
}

static const struct dev_builder __rst_exinfo_dev_builder[] = {
	DEVICE_BUILDER(__rst_exinfo_parse_dt, NULL),
	DEVICE_BUILDER(__rst_exinfo_init_panic_extra_info, NULL),
	DEVICE_BUILDER(__rst_exinfo_register_die_notifier,
		       __rst_exinfo_unregister_die_notifier),
	DEVICE_BUILDER(__rst_exinfo_register_panic_notifier,
		       __rst_exinfo_unregister_panic_notifier),
	DEVICE_BUILDER(__qc_arch_rst_exinfo_vh_init, NULL),
	DEVICE_BUILDER(__qc_arch_rst_exinfo_register_rvh_do_sp_pc_abort, NULL),
	DEVICE_BUILDER(__qc_arch_rst_exinfo_register_rvh_report_bug, NULL),
	DEVICE_BUILDER(__qc_arch_rst_exinfo_register_rvh_die_kernel_fault, NULL),
	DEVICE_BUILDER(__qc_rst_exinfo_fault_init, NULL),
	DEVICE_BUILDER(__rst_exinfo_probe_epilog, __rst_exinfo_remove_prolog),
};

static int __rst_exinfo_probe(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct device *dev = &pdev->dev;
	struct rst_exinfo_drvdata *drvdata;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->bd.dev = dev;

	return sec_director_probe_dev(&drvdata->bd, builder, n);
}

static int __rst_exinfo_remove(struct platform_device *pdev,
		const struct dev_builder *builder, ssize_t n)
{
	struct rst_exinfo_drvdata *drvdata = platform_get_drvdata(pdev);

	sec_director_destruct_dev(&drvdata->bd, builder, n, n);

	return 0;
}

static int sec_qc_rst_exinfo_probe(struct platform_device *pdev)
{
	return __rst_exinfo_probe(pdev, __rst_exinfo_dev_builder,
			ARRAY_SIZE(__rst_exinfo_dev_builder));
}

static int sec_qc_rst_exinfo_remove(struct platform_device *pdev)
{
	return __rst_exinfo_remove(pdev, __rst_exinfo_dev_builder,
			ARRAY_SIZE(__rst_exinfo_dev_builder));
}

static const struct of_device_id sec_qc_rst_exinfo_match_table[] = {
	{ .compatible = "samsung,qcom-rst_exinfo" },
	{},
};
MODULE_DEVICE_TABLE(of, sec_qc_rst_exinfo_match_table);

static struct platform_driver sec_qc_rst_exinfo = {
	.driver = {
		.name = "sec,qc-rst_exinfo",
		.of_match_table = of_match_ptr(sec_qc_rst_exinfo_match_table),
	},
	.probe = sec_qc_rst_exinfo_probe,
	.remove = sec_qc_rst_exinfo_remove,
};

static int sec_qc_rst_exinfo_init(void)
{
	return platform_driver_register(&sec_qc_rst_exinfo);
}
module_init(sec_qc_rst_exinfo_init);

static void sec_qc_rst_exinfo_exit(void)
{
	platform_driver_unregister(&sec_qc_rst_exinfo);
}
module_exit(sec_qc_rst_exinfo_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Reset extra info for Qualcomm based devices");
MODULE_LICENSE("GPL v2");
