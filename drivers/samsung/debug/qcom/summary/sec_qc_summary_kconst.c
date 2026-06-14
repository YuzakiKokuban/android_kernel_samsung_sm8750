// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/cpumask.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/pgtable.h>
#include <linux/percpu.h>

#include <debug_kinfo.h>

#include "sec_qc_summary.h"

static void __summary_kconst_init_common(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary_kconst *kconst =
			&(secdbg_apss(drvdata)->kconst);

	kconst->nr_cpus = num_possible_cpus();
	kconst->page_offset = PAGE_OFFSET;
	kconst->vmap_stack = !!IS_ENABLED(CONFIG_VMAP_STACK);

#if IS_ENABLED(CONFIG_ARM64)
	kconst->phys_offset = PHYS_OFFSET;
	kconst->va_bits = VA_BITS;
	kconst->kimage_vaddr = kimage_vaddr;
	kconst->kimage_voffset = kimage_voffset;
#endif

#if IS_ENABLED(CONFIG_SMP)
	kconst->per_cpu_offset.pa = virt_to_phys(__per_cpu_offset);
	kconst->per_cpu_offset.size = sizeof(__per_cpu_offset[0]);
	kconst->per_cpu_offset.count = ARRAY_SIZE(__per_cpu_offset);
#endif
}

static void __summary_kconst_init_builtin(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary_kconst *kconst =
			&(secdbg_apss(drvdata)->kconst);

	kconst->swapper_pg_dir_paddr = __pa_symbol(swapper_pg_dir);
}

#define __summary_kconst_read_special_reg(x) ({ \
	uint64_t val; \
	asm volatile ("mrs %0, " # x : "=r"(val)); \
	val; \
})

static void __summary_kconst_init_wo_debug_kinfo(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary_kconst *kconst =
			&(secdbg_apss(drvdata)->kconst);
	union {
		struct {
			uint64_t baddr:48;
			uint64_t asid:16;
		};
		uint64_t raw;
	} ttbr1_el1;
	phys_addr_t baddr_phys;

	ttbr1_el1.raw = __summary_kconst_read_special_reg(TTBR1_EL1);
	baddr_phys = ttbr1_el1.baddr & (~(PAGE_SIZE - 1));
	kconst->swapper_pg_dir_paddr = __phys_to_kimg(baddr_phys);
}

static inline void __summary_kconst_init_module(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary_kconst *kconst =
			&(secdbg_apss(drvdata)->kconst);
	const struct kernel_all_info *all_kinfo =
			drvdata->debug_kinfo_rmem->priv;
	const struct kernel_info *kinfo = &(all_kinfo->info);

	kconst->swapper_pg_dir_paddr = kinfo->swapper_pg_dir_pa;
}

int notrace __qc_summary_kconst_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);

	__summary_kconst_init_common(drvdata);

	if (IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		__summary_kconst_init_builtin(drvdata);
	else if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO))
		__summary_kconst_init_wo_debug_kinfo(drvdata);
	else
		__summary_kconst_init_module(drvdata);

	return 0;
}
