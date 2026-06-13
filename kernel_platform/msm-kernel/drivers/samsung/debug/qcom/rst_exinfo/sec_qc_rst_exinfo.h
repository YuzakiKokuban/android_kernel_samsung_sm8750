#ifndef __INTERNAL__SEC_QC_RST_EXINFO_H__
#define __INTERNAL__SEC_QC_RST_EXINFO_H__

#include <linux/hashtable.h>
#include <linux/ioport.h>
#include <linux/kdebug.h>
#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset_type.h>

#include "sec_qc_rst_exinfo_fault.h"

struct rst_exinfo_drvdata {
	struct builder bd;
	struct reserved_mem *rmem;
	phys_addr_t paddr;
	size_t size;
	rst_exinfo_t *rst_exinfo;
	struct notifier_block nb_panic;
	struct notifier_block nb_die;
	DECLARE_HASHTABLE(die_handler_htbl, 3);
};

extern struct rst_exinfo_drvdata *qc_rst_exinfo;

static __always_inline bool __qc_rst_exinfo_is_probed(void)
{
	return !!qc_rst_exinfo;
}

/* sec_qc_rst_exinfo_main.c */
extern void sec_qc_rst_exinfo_store_extc_idx(bool prefix);
extern void __qc_rst_exinfo_store_extc_idx(struct rst_exinfo_drvdata *drvdata, bool prefix);
extern void __qc_rst_exinfo_save_dying_msg(struct rst_exinfo_drvdata *drvdata, const char *str, const void *pc, const void *lr);
extern void __qc_arch_rst_exinfo_die_handler(struct rst_exinfo_drvdata *drvdata, enum die_val val, const struct die_args *args);

/* sec_qc_arch_rst_exinfo_vh.c */
extern int __qc_arch_rst_exinfo_vh_init(struct builder *bd);
extern int __qc_arch_rst_exinfo_register_rvh_die_kernel_fault(struct builder *bd);
extern int __qc_arch_rst_exinfo_register_rvh_do_sp_pc_abort(struct builder *bd);
extern int __qc_arch_rst_exinfo_register_rvh_report_bug(struct builder *bd);

/* sec_qc_rst_exinfo_fault.c */
extern void __qc_rst_exinfo_fault_print_handler(struct rst_exinfo_drvdata *drvdata, const char *msg);
extern int __qc_rst_exinfo_fault_init(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_RST_EXINFO_H__ */
