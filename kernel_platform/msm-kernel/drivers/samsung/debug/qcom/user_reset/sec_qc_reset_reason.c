// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */
#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

#include "sec_qc_user_reset.h"

unsigned int sec_qc_reset_reason_get(void)
{
	struct qc_reset_reason_proc *rr_proc;

	if (!__qc_user_reset_is_probed())
		return 0xFFEEFFEE;

	rr_proc = &qc_user_reset->reset_reason;

	return rr_proc->reset_reason;
}
EXPORT_SYMBOL_GPL(sec_qc_reset_reason_get);

static const char *reset_reason_str[] = {
	[USER_UPLOAD_CAUSE_SMPL]		= "SP",
	[USER_UPLOAD_CAUSE_WTSR]		= "WP",
	[USER_UPLOAD_CAUSE_WATCHDOG]		= "DP",
	[USER_UPLOAD_CAUSE_PANIC]		= "KP",
	[USER_UPLOAD_CAUSE_MANUAL_RESET]	= "MP",
	[USER_UPLOAD_CAUSE_POWER_RESET]		= "PP",
	[USER_UPLOAD_CAUSE_REBOOT]		= "RP",
	[USER_UPLOAD_CAUSE_BOOTLOADER_REBOOT]	= "BP",
	[USER_UPLOAD_CAUSE_POWER_ON]		= "NP",
	[USER_UPLOAD_CAUSE_THERMAL]		= "TP",
	[USER_UPLOAD_CAUSE_CP_CRASH]		= "CP",
	[USER_UPLOAD_CAUSE_UNKNOWN]		= "NP",
};

const char *sec_qc_reset_reason_to_str(unsigned int reason)
{
	if (reason < USER_UPLOAD_CAUSE_MIN || reason > USER_UPLOAD_CAUSE_MAX)
		reason = USER_UPLOAD_CAUSE_UNKNOWN;

	return reset_reason_str[reason];
}
EXPORT_SYMBOL_GPL(sec_qc_reset_reason_to_str);

static void ____reset_reason_update_and_clear(
		struct qc_reset_reason_proc *rr_proc,
		ap_health_t *health)
{
	switch (rr_proc->reset_reason) {
	case USER_UPLOAD_CAUSE_SMPL:
		health->daily_rr.sp++;
		health->rr.sp++;
		break;
	case USER_UPLOAD_CAUSE_WTSR:
		health->daily_rr.wp++;
		health->rr.wp++;
		break;
	case USER_UPLOAD_CAUSE_WATCHDOG:
		health->daily_rr.dp++;
		health->rr.dp++;
		break;
	case USER_UPLOAD_CAUSE_PANIC:
		health->daily_rr.kp++;
		health->rr.kp++;
		break;
	case USER_UPLOAD_CAUSE_MANUAL_RESET:
		health->daily_rr.mp++;
		health->rr.mp++;
		break;
	case USER_UPLOAD_CAUSE_POWER_RESET:
		health->daily_rr.pp++;
		health->rr.pp++;
		break;
	case USER_UPLOAD_CAUSE_REBOOT:
		health->daily_rr.rp++;
		health->rr.rp++;
		break;
	case USER_UPLOAD_CAUSE_THERMAL:
		health->daily_rr.tp++;
		health->rr.tp++;
		break;
	case USER_UPLOAD_CAUSE_CP_CRASH:
		health->daily_rr.cp++;
		health->rr.cp++;
		break;
	default:
		health->daily_rr.np++;
		health->rr.np++;
	}

	health->last_rst_reason = 0;
}

static int __reset_reason_update_and_clear(struct qc_reset_reason_proc *rr_proc)
{
	struct qc_user_reset_drvdata *drvdata = container_of(rr_proc,
			struct qc_user_reset_drvdata, reset_reason);
	ap_health_t *health;
	bool valid;

	if (rr_proc->lpm_mode)
		return 0;

	if (atomic_dec_if_positive(&rr_proc->first_run) < 0)
		return 0;

	health = __qc_ap_health_data_read(drvdata);
	if (IS_ERR_OR_NULL(health))
		return -ENODEV;

	____reset_reason_update_and_clear(rr_proc, health);

	valid = __qc_ap_health_data_write(drvdata, health);
	if (!valid)
		return -EINVAL;

	return 0;
}

static int sec_qc_reset_reason_proc_show(struct seq_file *m, void *v)
{
	struct qc_reset_reason_proc *rr_proc = m->private;
	unsigned int reset_reason = rr_proc->reset_reason;
	int err;

	seq_printf(m, "%sON\n", sec_qc_reset_reason_to_str(reset_reason));

	err = __reset_reason_update_and_clear(rr_proc);
	if (err)
		pr_warn("failed to update and clear reset reason.");

	return 0;
}

static int sec_qc_reset_reason_proc_open(struct inode *inode, struct file *file)
{
	struct qc_reset_reason_proc *rr_proc = pde_data(inode);

	return single_open(file, sec_qc_reset_reason_proc_show, rr_proc);
}

static const struct proc_ops reset_reason_pops = {
	.proc_open = sec_qc_reset_reason_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __reset_reason_read(struct qc_reset_reason_proc *rr_proc)
{
	struct qc_user_reset_drvdata *drvdata = container_of(rr_proc,
			struct qc_user_reset_drvdata, reset_reason);
	ap_health_t *health;
	int ret = 0;

	health = __qc_ap_health_data_read(drvdata);
	if (IS_ERR_OR_NULL(health)) {
		ret = -ENODEV;
		goto err_read_ap_health_data;
	}

	rr_proc->reset_reason = health->last_rst_reason;

	return 0;

err_read_ap_health_data:
	return ret;
}

static inline int __rest_reasson_proc_create(struct qc_reset_reason_proc *rr_proc)
{
	struct qc_user_reset_drvdata *drvdata = container_of(rr_proc,
			struct qc_user_reset_drvdata, reset_reason);
	const char *node_name = "reset_reason";

	rr_proc->proc = proc_create_data(node_name, 0444, NULL,
			&reset_reason_pops, rr_proc);
	if (!rr_proc->proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

static char *boot_mode __ro_after_init;
module_param_named(boot_mode, boot_mode, charp, 0444);

static inline void __reset_reason_chec_lpm_mode(struct qc_reset_reason_proc *rr_proc)
{
	if (!boot_mode || strncmp(boot_mode, "charger", strlen("charger")))
		rr_proc->lpm_mode = false;
	else
		rr_proc->lpm_mode = true;
}

int __qc_reset_reason_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_reset_reason_proc *rr_proc = &drvdata->reset_reason;
	int err;

	atomic_set(&rr_proc->first_run, 1);

	err = __reset_reason_read(rr_proc);
	if (err)
		return err;
	err = __rest_reasson_proc_create(rr_proc);
	if (err)
		return err;

	__reset_reason_chec_lpm_mode(rr_proc);
	__reset_reason_update_and_clear(rr_proc);

	return 0;
}

void __qc_reset_reason_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_reset_reason_proc *rr_proc = &drvdata->reset_reason;

	proc_remove(rr_proc->proc);
}
