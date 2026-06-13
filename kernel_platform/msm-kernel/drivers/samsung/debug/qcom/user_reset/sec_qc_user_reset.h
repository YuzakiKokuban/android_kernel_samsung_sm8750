#ifndef __INTERNAL__SEC_QC_USER_RESET_H__
#define __INTERNAL__SEC_QC_USER_RESET_H__

#include <linux/notifier.h>
#include <linux/workqueue.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_user_reset_external.h"

struct qc_reset_rwc_proc {
	struct mutex lock;
	uint32_t reset_write_cnt;
	struct proc_dir_entry *proc;
};

struct qc_user_reset_proc {
	struct mutex lock;
	unsigned int ref_cnt;
	struct debug_reset_header *reset_header;
	const char *buf;
	size_t len;
	struct proc_dir_entry *proc;
};

struct qc_reset_reason_proc {
	unsigned int reset_reason;
	bool lpm_mode;
	atomic_t first_run;
	struct proc_dir_entry *proc;
};

struct qc_user_reset_drvdata {
	struct builder bd;
	ap_health_t *health;
	atomic_t health_work_offline;
	struct delayed_work health_work;
	struct device *sec_debug_dev;
	struct qc_reset_rwc_proc reset_rwc;
	struct qc_user_reset_proc reset_summary;
	struct qc_user_reset_proc reset_klog;
	struct qc_user_reset_proc reset_tzlog;
	struct qc_user_reset_proc auto_comment;
	struct qc_user_reset_proc reset_history;
	struct qc_reset_reason_proc reset_reason;
	struct notifier_block nb_kryo_arm64_edac;
	struct proc_dir_entry *store_lastkmsg_proc;
};

extern struct qc_user_reset_drvdata *qc_user_reset;

static __always_inline bool __qc_user_reset_is_probed(void)
{
	return !!qc_user_reset;
}

/* sec_qc_dbg_part_ap_health.c */
extern int __qc_ap_health_init(struct builder *bd);
extern void __qc_ap_health_exit(struct builder *bd);
/* TODO: internal use only */
extern ap_health_t *__qc_ap_health_data_read(struct qc_user_reset_drvdata *drvdata);
extern int __qc_ap_health_data_write(struct qc_user_reset_drvdata *drvdata, ap_health_t *data);
extern void __qc_ap_health_data_write_delayed(struct qc_user_reset_drvdata *drvdata, ap_health_t *data);

/* sec_qc_reset_rwc.c */
extern int __qc_reset_rwc_init(struct builder *bd);
extern void __qc_reset_rwc_exit(struct builder *bd);
/* TODO: internal use only */
extern int __qc_user_reset_set_reset_header(struct debug_reset_header *header);

/* sec_qc_reset_reason.c */
extern int __qc_reset_reason_init(struct builder *bd);
extern void __qc_reset_reason_exit(struct builder *bd);

/* sec_qc_reset_summary.c */
extern int __qc_reset_summary_init(struct builder *bd);
extern void __qc_reset_summary_exit(struct builder *bd);

/* sec_qc_reset_klog.c */
extern int __qc_reset_klog_init(struct builder *bd);
extern void __qc_reset_klog_exit(struct builder *bd);

/* sec_qc_reset_tzlog.c */
extern int __qc_reset_tzlog_init(struct builder *bd);
extern void __qc_reset_tzlog_exit(struct builder *bd);

/* sec_qc_auto_comment.c */
extern int __qc_auto_comment_init(struct builder *bd);
extern void __qc_auto_comment_exit(struct builder *bd);

/* sec_qc_reset_history.c */
extern int __qc_reset_history_init(struct builder *bd);
extern void __qc_reset_history_exit(struct builder *bd);

/* sec_qc_fmm_lock.c */
extern int __qc_fmm_lock_init(struct builder *bd);
extern void __qc_fmm_lock_exit(struct builder *bd);

/* sec_qc_recovery_cause.c */
extern int __qc_recovery_cause_init(struct builder *bd);
extern void __qc_recovery_cause_exit(struct builder *bd);

/* sec_qc_kryo_arm64_edac.c */
extern int __qc_kryo_arm64_edac_init(struct builder *bd);
extern void __qc_kryo_arm64_edac_exit(struct builder *bd);

/* sec_qc_store_lastkmsg.c */
extern int __qc_store_lastkmsg_init(struct builder *bd);
extern void __qc_store_lastkmsg_exit(struct builder *bd);

/* sec_qc_modem_reset_data.c */
extern int __qc_modem_reset_data_init(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_USER_RESET_H__ */
