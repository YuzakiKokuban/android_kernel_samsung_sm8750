// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched/cputime.h>
#include <linux/sched.h>
#include <linux/timekeeper_internal.h>

#include <asm/stacktrace.h>
#include <asm/page.h>
#include <asm/sections.h>

#include <kernel/module/internal.h>
#include <kernel/sched/sched.h>

#include "sec_qc_summary.h"
#include "sec_qc_summary_coreinfo.h"
#include "sec_qc_summary_external.h"

static char *summary_coreinfo_data;
static int32_t summary_coreinfo_size;
static void *summary_coreinfo_note;

void sec_qc_summary_coreinfo_append_str(const char *fmt, ...)
{
	va_list args;
	char *buf = &summary_coreinfo_data[summary_coreinfo_size];
	int32_t remained;
	int32_t expected;

	remained = (int32_t)QC_SUMMARY_COREINFO_BYTES - summary_coreinfo_size;
	if (remained < 0) {
		pr_warn("No space left\n");
		return;
	}

	va_start(args, fmt);
	expected = (int32_t)vsnprintf(buf, remained, fmt, args);
	va_end(args);

	if (expected >= remained || expected < 0) {
		pr_warn("Not enough space to append %s\n", buf);
		memset(buf, 0x0, remained);
		return;
	}

	summary_coreinfo_size += expected;
}

static int __summary_coreinfo_alloc(void)
{
	summary_coreinfo_data = (char *)get_zeroed_page(GFP_KERNEL);
	if (!summary_coreinfo_data) {
		pr_err("Memory allocation for summary_coreinfo_data failed\n");
		return -ENOMEM;
	}

	/* TODO: will be used in future? */
	summary_coreinfo_note = alloc_pages_exact(QC_SUMMARY_COREINFO_NOTE_SIZE,
						  GFP_KERNEL | __GFP_ZERO);
	if (!summary_coreinfo_note) {
		pr_warn("Memory allocation for summary_coreinfo_note failed\n");
		return -ENOMEM;
	}

	return 0;
}

static void __summary_coreinfo_free(void)
{
	if (!summary_coreinfo_data) {
		free_page((unsigned long)summary_coreinfo_data);
		summary_coreinfo_data = NULL;
	}

	if (!summary_coreinfo_note) {
		free_pages_exact(summary_coreinfo_note,
				QC_SUMMARY_COREINFO_NOTE_SIZE);
		summary_coreinfo_note = NULL;
	}
}

static void __summary_coreinfo_init_dbg_apps(
		struct sec_qc_summary_data_apss *dbg_apss)
{
	dbg_apss->coreinfo.coreinfo_data = (uintptr_t)summary_coreinfo_data;
	dbg_apss->coreinfo.coreinfo_size = (uintptr_t)&summary_coreinfo_size;
	dbg_apss->coreinfo.coreinfo_note = (uintptr_t)summary_coreinfo_note;

	pr_info("coreinfo_data=0x%pK\n", (void *)dbg_apss->coreinfo.coreinfo_data);
}


#define QC_SUMMARY_COREINFO_KALLSYMS(__drvdata, __name) \
		sec_qc_summary_coreinfo_append_str("SYMBOL(%s)=0x%llx\n", #__name, \
				__qc_summary_kallsyms_lookup_name(__drvdata, #__name))

static void __summary_coreinfo_append_data(struct qc_summary_drvdata *drvdata)
{
	QC_SUMMARY_COREINFO_SYMBOL(runqueues);
	QC_SUMMARY_COREINFO_STRUCT_SIZE(rq);
	QC_SUMMARY_COREINFO_OFFSET(rq, clock);
	QC_SUMMARY_COREINFO_OFFSET(rq, nr_running);

	QC_SUMMARY_COREINFO_OFFSET(task_struct, prio);
	QC_SUMMARY_COREINFO_OFFSET(task_struct, mm);
	QC_SUMMARY_COREINFO_OFFSET(task_struct, cred);
	QC_SUMMARY_COREINFO_OFFSET(cred, uid);

	QC_SUMMARY_COREINFO_KALLSYMS(drvdata, __sched_text_start);
	QC_SUMMARY_COREINFO_KALLSYMS(drvdata, __sched_text_end);
	/* TODO: kernel/time/timekeeping.c
	 * static struct {
	 *	seqcount_t		seq;
	 *	struct timekeeper	timekeeper;
	 * } tk_core ____cacheline_aligned = {
	 *	.seq = SEQCNT_ZERO(tk_core.seq),
	 * };
	 */
	sec_qc_summary_coreinfo_append_str("OFFSET(tk_core.timekeeper)=%zu\n",
			8);

	QC_SUMMARY_COREINFO_OFFSET(timekeeper, xtime_sec);
}

static void __summary_coreinfo_append_module_data(
		struct qc_summary_drvdata *drvdata)
{
	QC_SUMMARY_COREINFO_KALLSYMS(drvdata, mod_tree);

	QC_SUMMARY_COREINFO_OFFSET(mod_tree_root, root);
	QC_SUMMARY_COREINFO_OFFSET(mod_tree_root, addr_min);
	QC_SUMMARY_COREINFO_OFFSET(mod_tree_root, addr_max);

	QC_SUMMARY_COREINFO_STRUCT_SIZE(module_memory);
	QC_SUMMARY_COREINFO_OFFSET(module_memory, base);
	QC_SUMMARY_COREINFO_OFFSET(module_memory, size);
	QC_SUMMARY_COREINFO_OFFSET(module_memory, mtn);

	QC_SUMMARY_COREINFO_OFFSET(mod_tree_node, node);

	QC_SUMMARY_COREINFO_OFFSET(module, mem);
	QC_SUMMARY_COREINFO_OFFSET(module, state);
	QC_SUMMARY_COREINFO_OFFSET(module, name);
	QC_SUMMARY_COREINFO_OFFSET(module, kallsyms);

	QC_SUMMARY_COREINFO_OFFSET(mod_kallsyms, symtab);
	QC_SUMMARY_COREINFO_OFFSET(mod_kallsyms, num_symtab);
	QC_SUMMARY_COREINFO_OFFSET(mod_kallsyms, strtab);

	QC_SUMMARY_COREINFO_OFFSET(latch_tree_root, seq);
	QC_SUMMARY_COREINFO_OFFSET(latch_tree_root, tree);
	QC_SUMMARY_COREINFO_OFFSET(seqcount, sequence);
}

int __qc_summary_coreinfo_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct sec_qc_summary_data_apss *dbg_apss = secdbg_apss(drvdata);
	int err;

	err = __summary_coreinfo_alloc();
	if (err)
		goto err_coreinfo_alloc_failed;

	__summary_coreinfo_init_dbg_apps(dbg_apss);
	__summary_coreinfo_append_data(drvdata);
	__summary_coreinfo_append_module_data(drvdata);

	return 0;

err_coreinfo_alloc_failed:
	__summary_coreinfo_free();
	return err;
}

void __qc_summary_coreinfo_exit(struct builder *bd)
{
	__summary_coreinfo_free();
}
