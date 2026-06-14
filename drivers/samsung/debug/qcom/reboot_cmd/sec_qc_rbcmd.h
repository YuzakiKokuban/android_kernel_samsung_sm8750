#ifndef __INTERNAL__SEC_QC_RBCMD_H__
#define __INTERNAL__SEC_QC_RBCMD_H__

#include <linux/notifier.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_reboot_cmd.h>
#include <linux/samsung/debug/qcom/sec_qc_rbcmd.h>

struct qc_rbcmd_stage {
	struct qc_rbcmd_cmd *reboot_cmd;
	struct sec_reboot_cmd *default_cmd;
};

struct qc_rbcmd_drvdata {
	struct builder bd;
	bool use_on_reboot;
	bool use_on_restart;
	struct qc_rbcmd_stage stage[SEC_RBCMD_STAGE_MAX];
	struct raw_notifier_head pon_rr_writers;
	struct raw_notifier_head sec_rr_writers;
};

struct qc_rbcmd_reset_reason {
	unsigned int pon_rr;	/* pon restart reason */
	unsigned int sec_rr;	/* sec restart reason */
};

enum {
	QC_RBCMD_DFLT_ON_NORMAL = 0,
	QC_RBCMD_DFLT_ON_PANIC,
};

#define DUMP_SINK_TO_SDCARD	0x73646364
#define DUMP_SINK_TO_BOOTDEV	0x42544456
#define DUMP_SINK_TO_USB	0x0

#define CDSP_SIGNOFF_BLOCK	0x2377
#define CDSP_SIGNOFF_ON		0x7277

/* sec_qc_rbcmd_main. */
extern struct qc_rbcmd_drvdata *qc_rbcmd;

static __always_inline bool __rbcmd_is_probed(void)
{
	return !!qc_rbcmd;
}

extern void __qc_rbcmd_write_pon_rr(unsigned int pon_rr, struct sec_reboot_param *param);
extern void __qc_rbcmd_write_sec_rr(unsigned int sec_rr, struct sec_reboot_param *param);
extern void __qc_rbcmd_set_restart_reason(unsigned int pon_rr, unsigned int sec_rr, struct sec_reboot_param *param);

/* sec_qc_rbcmd_command.c */
extern int __qc_rbcmd_init_on_reboot(struct builder *bd);
extern void __qc_rbcmd_exit_on_reboot(struct builder *bd);
extern int __qc_rbcmd_init_on_restart(struct builder *bd);
extern void __qc_rbcmd_exit_on_restart(struct builder *bd);
extern int __qc_rbcmd_register_panic_handle(struct builder *bd);
extern void __qc_rbcmd_unregister_panic_handle(struct builder *bd);

#endif /* __INTERNAL__SEC_QC_RBCMD_H__ */
