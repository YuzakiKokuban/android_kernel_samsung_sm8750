// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param.h"

static ssize_t __extrb_info_report_reset_reason(unsigned int reset_reason,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const char *reset_reason_str;
	int reset_write_cnt;

	reset_reason_str = sec_qc_reset_reason_to_str(reset_reason);
	reset_write_cnt = sec_qc_get_reset_write_cnt();

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RR\":\"%s\",", reset_reason_str);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RWC\":\"%d\",", reset_write_cnt);

	return info_size;
}

static ssize_t __extrb_info_report_rpm_ex_info(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const rpm_exinfo_t *rpm_ex_info = &rst_exinfo->rpm_ex_info;
	int idx, cnt, max_cnt;

	if (rpm_ex_info->info.magic != RPM_EX_INFO_MAGIC ||
	    rpm_ex_info->info.nlog <= 0)
		return info_size;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RPM\":\"");

	if (rpm_ex_info->info.nlog > 5) {
		idx = rpm_ex_info->info.nlog % 5;
		max_cnt = 5;
	} else {
		idx = 0;
		max_cnt = rpm_ex_info->info.nlog;
	}

	for (cnt  = 0; cnt < max_cnt; cnt++, idx++) {
		const __rpm_log_t *rpm_log = &rpm_ex_info->info.log[idx % 5];
		unsigned long ts_nsec = (unsigned long)rpm_log->nsec;
		unsigned long rem_nsec = do_div(ts_nsec, 1000000000);

		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%lu.%06lu ", ts_nsec, rem_nsec / 1000);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%s ", rpm_log->msg);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%x %x %x %x",
				rpm_log->arg[0], rpm_log->arg[1], rpm_log->arg[2], rpm_log->arg[3]);

		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%s", (cnt == max_cnt - 1) ? "\"," : "/");
	}

	return info_size;
}

static ssize_t __extrb_info_report_tz_ex_info(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const tz_exinfo_t *tz_ex_info = &rst_exinfo->tz_ex_info;
	const char *tz_cpu_status[6] = { "NA", "R", "PC", "WB", "IW", "IWT" };
	int cnt, max_cnt;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"TZ_RR\":\"%s\"", tz_ex_info->msg);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"TZBC\":\"");

	max_cnt = sizeof(tz_ex_info->cpu_status);
	max_cnt = max_cnt / sizeof(tz_ex_info->cpu_status[0]);

	for (cnt = 0; cnt < max_cnt; cnt++) {
		if (tz_ex_info->cpu_status[cnt] > INVALID_WARM_TERM_ENTRY_EXIT_COUNT)
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%s", tz_cpu_status[0]);
		else
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%s", tz_cpu_status[tz_ex_info->cpu_status[cnt]]);

		if (cnt != max_cnt - 1)
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",");
	}
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");

	return info_size;
}

static ssize_t __extrb_info_report_pimem_ex_info(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const pimem_exinfo_t *pimem_info = &rst_exinfo->pimem_info;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"PIMEM\":\"0x%08x,0x%08x,0x%08x,0x%08x\"",
			pimem_info->esr, pimem_info->ear0, pimem_info->esr_sdi, pimem_info->ear0_sdi);

	return info_size;
}

static ssize_t __extrb_info_report_cpu_stuck_ex_info(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const cpu_stuck_exinfo_t *cpu_stuck_info = &rst_exinfo->cpu_stuck_info;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size,
			",\"COST\":\"%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\"",
			cpu_stuck_info->cpu[0], cpu_stuck_info->cpu[1],
			cpu_stuck_info->cpu[2], cpu_stuck_info->cpu[3],
			cpu_stuck_info->cpu[4], cpu_stuck_info->cpu[5],
			cpu_stuck_info->cpu[6], cpu_stuck_info->cpu[7]);

	return info_size;
}

static ssize_t __extrb_info_report_cpu_hyp_ex_info(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const hyp_exinfo_t *hyp_ex_info = &rst_exinfo->hyp_ex_info;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"HYC\":\"%d\"", hyp_ex_info->s2_fault_counter);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"HYP\":\"%s\"", hyp_ex_info->msg);

	return info_size;
}

static ssize_t __extrb_info_report_cpu_kern_ex_info(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const _kern_ex_info_t *kern_ex_info = &rst_exinfo->kern_ex_info.info;
	int idx, max_cnt;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"LPM\":\"");
	max_cnt = sizeof(kern_ex_info->lpm_state);
	max_cnt = max_cnt / sizeof(kern_ex_info->lpm_state[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%x", kern_ex_info->lpm_state[idx]);
		if (idx != max_cnt - 1)
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",");
	}
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"PKO\":\"%x\"", kern_ex_info->pko);

	/* NOTE: While integrating ETR* into ETRA, there was a duplication PC,LR.
	 * Since ETRA contains more valueable information, it was decided to delete ETRB 
	 */
#if 0
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"LR\":\"");
	max_cnt = sizeof(kern_ex_info->lr_val);
	max_cnt = max_cnt / sizeof(kern_ex_info->lr_val[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%llx", kern_ex_info->lr_val[idx]);
		if (idx != max_cnt - 1)
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",");
	}
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"PC\":\"");
	max_cnt = sizeof(kern_ex_info->pc_val);
	max_cnt = max_cnt / sizeof(kern_ex_info->pc_val[0]);
	for (idx = 0; idx < max_cnt; idx++) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%llx", kern_ex_info->pc_val[idx]);
		if (idx != max_cnt - 1)
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",");
	}
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");
#endif
	return info_size;
}

static ssize_t extrb_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = SPECIAL_LEN_STR;
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

	info_size = __extrb_info_report_reset_reason(reset_reason, buf, sz_buf, info_size);
	info_size = __extrb_info_report_rpm_ex_info(rst_exinfo, buf, sz_buf, info_size);
	info_size = __extrb_info_report_tz_ex_info(rst_exinfo, buf, sz_buf, info_size);
	info_size = __extrb_info_report_pimem_ex_info(rst_exinfo, buf, sz_buf, info_size);
	info_size = __extrb_info_report_cpu_stuck_ex_info(rst_exinfo, buf, sz_buf, info_size);
	info_size = __extrb_info_report_cpu_hyp_ex_info(rst_exinfo, buf, sz_buf, info_size);
	info_size = __extrb_info_report_cpu_kern_ex_info(rst_exinfo, buf, sz_buf, info_size);

err_failed_to_read:
	kfree(rst_exinfo);
err_enomem:
	__qc_hw_param_clean_format(buf, &info_size, sz_buf);
err_invalid_reset_reason:
	return info_size;
}
DEVICE_ATTR_RO(extrb_info);
