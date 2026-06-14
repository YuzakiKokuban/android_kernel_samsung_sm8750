#ifndef __INTERNAL__SEC_QC_SUMMARY_H__
#define __INTERNAL__SEC_QC_SUMMARY_H__

#include <linux/io.h>
#include <linux/of_reserved_mem.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/qcom/sec_qc_summary.h>
#include <linux/samsung/debug/sec_arm64_ap_context.h>

struct qc_summary_kallsyms {
	const unsigned long *addresses;
	const int *offsets;
	const u8 *names;
	unsigned int num_syms;
	unsigned long relative_base;
	const char *token_table;
	const u16 *token_index;
	const unsigned int *markers;
};

struct qc_summary_ap_context {
	struct sec_arm64_ap_context *ap_context;
	struct notifier_block nb;
};

struct qc_summary_drvdata {
	struct builder bd;
	size_t smem_offset;
	struct sec_qc_summary *summary;
	struct notifier_block nb_die;
	struct notifier_block nb_panic;
	struct reserved_mem *debug_kinfo_rmem;
	struct qc_summary_kallsyms kallsyms;
	struct qc_summary_ap_context ap_context;
};

#if !IS_ENABLED(CONFIG_ARM64)
static inline u64 __phys_to_kimg(unsigned long physaddr)
{
	return (uintptr_t)phys_to_virt(physaddr);
}
#endif

static inline struct sec_qc_summary_data_apss *secdbg_apss(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary *dbg_summary = drvdata->summary;

	return &dbg_summary->priv.apss;
}


static inline struct sec_qc_summary_data_modem *secdbg_modem(
		struct qc_summary_drvdata *drvdata)
{
	struct sec_qc_summary *dbg_summary = drvdata->summary;

	return &dbg_summary->priv.modem;
}

/* sec_qc_summary_var_mon.c */
extern int __qc_summary_info_mon_init(struct builder *bd);
extern int __qc_summary_var_mon_init_last_pet(struct builder *bd);
extern int __qc_summary_var_mon_init_last_ns(struct builder *bd);
extern void __qc_summary_var_mon_exit_last_pet(struct builder *bd);

/* sec_qc_summary_task.c */
extern int __qc_summary_task_init(struct builder *bd);

/* sec_qc_summary_kconst.c */
extern int __qc_summary_kconst_init(struct builder *bd);

/* sec_qc_summary_ksyms.c */
/* implemented @ kernel/kallsyms.c */
extern const unsigned long kallsyms_addresses[] __weak;
extern const int kallsyms_offsets[] __weak;
extern const u8 kallsyms_names[] __weak;
extern const unsigned long kallsyms_num_syms
__attribute__((weak, section(".rodata")));
extern const unsigned long kallsyms_relative_base
__attribute__((weak, section(".rodata")));
extern const u8 kallsyms_token_table[] __weak;
extern const u16 kallsyms_token_index[] __weak;
extern const unsigned long kallsyms_markers[] __weak;

extern int __qc_summary_ksyms_init(struct builder *bd);

/* sec_qc_summary_sched_log.c */
extern int __qc_summary_sched_log_init(struct builder *bd);

/* sec_qc_summary_aplmp.c */
/* implemented @ kernel/sched/core.c */
DECLARE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);

extern int __qc_summary_aplpm_init(struct builder *bd);

/* sec_qc_summary_arm64_ap_context.c */
extern int __qc_summary_arm64_ap_context_init(struct builder *bd);
extern void __qc_summary_arm64_ap_context_exit(struct builder *bd);

/* sec_qc_summary_coreinfo.c */
extern int __qc_summary_coreinfo_init(struct builder *bd);
extern void __qc_summary_coreinfo_exit(struct builder *bd);

/* sec_qc_summary_dump_sink.c */
extern int __qc_summary_dump_sink_init(struct builder *bd);

/* sec_qc_summary_debug_kinfo.c */
extern int __qc_summary_debug_kinfo_init(struct builder *bd);

/* sec_qc_summary_kallsyms.c */
extern int __qc_summary_kallsyms_init(struct builder *bd);
extern unsigned long __qc_summary_kallsyms_lookup_name(struct qc_summary_drvdata *drvdata, const char *name);

#endif /* __INTERNAL__SEC_QC_SUMMARY_H__ */
