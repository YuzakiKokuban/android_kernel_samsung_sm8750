#ifndef __INTERNAL__SEC_QC_DEBUG_H__
#define __INTERNAL__SEC_QC_DEBUG_H__

#include <linux/kdebug.h>
#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_boot_stat.h>

#include "sec_qc_debug_mock.h"

struct cp_dump_encrypt_entry {
	const char *compatible;
	unsigned int low;
	unsigned int mid_high;
};

struct cp_dump_encrypt {
	int nr_entries;
	struct cp_dump_encrypt_entry *entry;
};

struct qc_debug_drvdata {
	struct builder bd;
	struct notifier_block reboot_nb;
	struct notifier_block die_nb;
	bool use_cp_dump_encrypt;
	struct cp_dump_encrypt cp_dump_encrypt;
	bool use_store_last_kmsg;
	bool use_store_lpm_kmsg;
	bool use_store_onoff_history;
	bool is_lpm_mode;
	struct sec_boot_stat_soc_operations *boot_stat_ops;
};

/* sec_qc_lpm_log.c */
extern int sec_qc_debug_lpm_log_init(struct builder *bd);

/* sec_qc_debug_reboot.c */
extern int sec_qc_debug_reboot_init(struct builder *bd);
extern void sec_qc_debug_reboot_exit(struct builder *bd);

/* sec_qc_force_err.c */
extern int sec_qc_force_err_init(struct builder *bd);
extern void sec_qc_force_err_exit(struct builder *bd);

/* sec_qc_boot_stat.c */
extern int sec_qc_boot_stat_init(struct builder *bd);
extern void sec_qc_boot_stat_exit(struct builder *bd);

/* sec_qc_cp_dump_encrypt.c */
extern int sec_qc_cp_dump_encrypt_init(struct builder *bd);

/* sec_qc_vendor_hooks.c */
extern int sec_qc_vendor_hooks_init(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_DEBUG_H__ */
