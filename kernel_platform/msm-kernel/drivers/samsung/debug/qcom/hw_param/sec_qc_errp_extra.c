// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <linux/samsung/debug/qcom/sec_qc_rbcmd.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>
#include <linux/samsung/debug/qcom/sec_qc_upload_cause.h>

#include "sec_qc_hw_param.h"

#define EXTEND_RR_SIZE		150

static ssize_t __errp_extra_report_reset_write_cnt(char *buf,
		const ssize_t sz_buf, ssize_t info_size)
{
	int reset_write_cnt;

	reset_write_cnt = sec_qc_get_reset_write_cnt();

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RWC\":\"%d\",", reset_write_cnt);

	return info_size;
}

static ssize_t __errp_extra_report_reset_reason_smpl(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const _kern_ex_info_t *kinfo = &rst_exinfo->kern_ex_info.info;

	if (strstr(kinfo->panic_buf, "SMPL"))
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, " PANIC:%s", kinfo->panic_buf);

	return info_size;
}

static ssize_t __errp_extra_report_upload_cause(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	if (rst_exinfo->uc_ex_info.magic == RESTART_REASON_SEC_DEBUG_MODE) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, " UPLOAD:%s", rst_exinfo->uc_ex_info.str);
	} else {
		const _kern_ex_info_t *kinfo = &rst_exinfo->kern_ex_info.info;
		char upload_cause_str[80] = {0,};

		sec_qc_upldc_type_to_cause(kinfo->upload_cause,
				upload_cause_str, sizeof(upload_cause_str));

		__qc_hw_param_scnprintf(buf, sz_buf, info_size, " UPLOAD:%s_0x%x", upload_cause_str, kinfo->upload_cause);
	}

	return info_size;
}

static ssize_t __errp_extra_report_tz_reset_reason(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const tz_exinfo_t *tz_ex_info = &rst_exinfo->tz_ex_info;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, " TZ_RR:%s", tz_ex_info->msg);

	return info_size;
}

static ssize_t __errp_extra_report_panic_context(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const _kern_ex_info_t *kinfo = &rst_exinfo->kern_ex_info.info;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, " PANIC:%s", kinfo->panic_buf);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, " PC:%s", kinfo->pc);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, " LR:%s", kinfo->lr);

	return info_size;
}

static int __errp_extra_show(struct seq_file *m, void *v)
{
	struct device *dev = qc_hw_param->bd.dev;
	const ssize_t sz_buf = EXTEND_RR_SIZE;
	ssize_t info_size = 0;
	char buf[EXTEND_RR_SIZE] = {0, };
	unsigned int reset_reason;
	rst_exinfo_t *rst_exinfo;

	reset_reason = sec_qc_reset_reason_get();
	if (!__qc_hw_param_is_valid_reset_reason(reset_reason))
		goto err_invalid_reset_reason;

	rst_exinfo = kmalloc(sizeof(rst_exinfo_t), GFP_KERNEL);
	if (!rst_exinfo)
		goto err_enomem;

	if (!sec_qc_dbg_part_read(debug_index_reset_ex_info, rst_exinfo)) {
		dev_warn(dev, "failed to read debug paritition.\n");
		goto err_failed_to_read;
	}

	info_size = __errp_extra_report_reset_write_cnt(buf, sz_buf, info_size);

	if (reset_reason == USER_UPLOAD_CAUSE_SMPL) {
		info_size = __errp_extra_report_reset_reason_smpl(rst_exinfo, buf, sz_buf, info_size);
		goto early_exit_on_smpl;
	}

	__errp_extra_report_upload_cause(rst_exinfo, buf, sz_buf, info_size);

	if (reset_reason == USER_UPLOAD_CAUSE_WATCHDOG)
		goto early_exit_on_watchdog;

	if (reset_reason == USER_UPLOAD_CAUSE_WTSR ||
	    reset_reason == USER_UPLOAD_CAUSE_POWER_RESET) {
		info_size = __errp_extra_report_tz_reset_reason(rst_exinfo, buf, sz_buf, info_size);
		goto early_exit_on_unknown;
	}

	info_size = __errp_extra_report_panic_context(rst_exinfo, buf, sz_buf, info_size);

early_exit_on_unknown:
early_exit_on_watchdog:
early_exit_on_smpl:
err_failed_to_read:
	kfree(rst_exinfo);
err_enomem:
	seq_puts(m, buf);
err_invalid_reset_reason:
	return 0;
}

static int sec_qc_errp_extra_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, __errp_extra_show, NULL);
}

static const struct proc_ops __errp_extra_pops = {
	.proc_open = sec_qc_errp_extra_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int __qc_errp_extra_init(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct proc_dir_entry *proc;

	proc = proc_create("extra", 0440, NULL, &__errp_extra_pops);
	if (IS_ERR(proc)) {
		dev_err(bd->dev, "failed to create a proc node.");
		return -ENODEV;
	}

	drvdata->errp_extra_proc = proc;

	return 0;
}

void __qc_errp_extra_exit(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);

	proc_remove(drvdata->errp_extra_proc);
}
