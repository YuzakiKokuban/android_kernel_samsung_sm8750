// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/ctype.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/samsung/debug/qcom/sec_qc_rbcmd.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param.h"

static ssize_t __extra_info_report_reset_reason(unsigned int reset_reason,
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

static ssize_t __extra_info_report_task(const _kern_ex_info_t *kinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	unsigned long long rem_nsec;
	unsigned long long ts_nsec;

	ts_nsec = kinfo->ktime;
	rem_nsec = do_div(ts_nsec, 1000000000ULL);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"KTIME\":\"%llu.%06llu\",", ts_nsec, rem_nsec / 1000ULL);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"CPU\":\"%d\",", kinfo->cpu);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"TASK\":\"%s\",", kinfo->task_name);

	return info_size;
}

static ssize_t __extra_info_report_smmu(const _kern_ex_info_t *kinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const ex_info_smmu_t *smmu = &kinfo->smmu;

	if (!smmu->fsr)
		return info_size;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"SDN\":\"%s\",", smmu->dev_name);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FSR\":\"%x\",", smmu->fsr);

	if (!smmu->fsynr0)
		return info_size;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FSY0\":\"%x\",", smmu->fsynr0);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FSY1\":\"%x\",", smmu->fsynr1);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"IOVA\":\"%08lx\",", smmu->iova);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FAR\":\"%016lx\",", smmu->far);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"SMN\":\"%s\",", smmu->mas_name);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"CB\":\"%d\",", smmu->cbndx);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"SOFT\":\"%016llx\",", smmu->phys_soft);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"ATOS\":\"%016llx\",", smmu->phys_atos);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"SID\":\"%x\",", smmu->sid);

	return info_size;
}

static ssize_t __extra_info_report_badmode(const _kern_ex_info_t *kinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const ex_info_badmode_t *badmode = &kinfo->badmode;

	if (!badmode->esr)
		return info_size;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BDR\":\"%08x\",", badmode->reason);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BDRS\":\"%s\",", badmode->handler_str);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BDE\":\"%08x\",", badmode->esr);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BDES\":\"%s\",", badmode->esr_str);

	return info_size;
}

static ssize_t __extra_info_report_fault(const _kern_ex_info_t *kinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const ex_info_fault_t *fault = &kinfo->fault[0];
	int cpu = kinfo->cpu;

	if ((cpu < 0) || (cpu > num_present_cpus()))
		return info_size;

	if (fault[cpu].esr) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"ESR\":\"%08x\",", fault[cpu].esr);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FNM\":\"%s\",", fault[cpu].str);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FV1\":\"%016llx\",", fault[cpu].var1);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FV2\":\"%016llx\",", fault[cpu].var2);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"FAULT\":\"pgd=%016llx VA=%016llx ", fault[cpu].pte[0], fault[cpu].pte[1]);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "*pgd=%016llx *pud=%016llx ", fault[cpu].pte[2], fault[cpu].pte[3]);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "*pmd=%016llx *pte=%016llx\",", fault[cpu].pte[4], fault[cpu].pte[5]);

	return info_size;
}

static ssize_t __extra_info_report_dying_msg(const rst_exinfo_t *rst_exinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const _kern_ex_info_t *kinfo = &rst_exinfo->kern_ex_info.info;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BUG\":\"%s\",", kinfo->bug_buf);
	if (strlen(kinfo->panic_buf))
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"PANIC\":\"%s\",", kinfo->panic_buf);
	else if (rst_exinfo->uc_ex_info.magic == RESTART_REASON_SEC_DEBUG_MODE)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"PANIC\":\"UCS-%s\",", rst_exinfo->uc_ex_info.str);

	return info_size;
}

static ssize_t __extra_info_report_context(const _kern_ex_info_t *kinfo,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"PC\":\"%s\",", kinfo->pc);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"LR\":\"%s\",", kinfo->lr);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"UFS\":\"%s\",", kinfo->ufs_err);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"ROT\":\"W%dC%d\",", __qc_hw_param_read_param0("wb"), __qc_hw_param_read_param0("ddi"));
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"STACK\":\"%s\"", kinfo->backtrace);

	return info_size;
}

static inline bool __extra_info_kinfo_is_valid(const _kern_ex_info_t *kinfo)
{
	size_t i;

	for (i = 0; i < sizeof(kinfo->bug_buf); i++) {
		if (kinfo->bug_buf[i] == '\0')
			break;
		else if (!isascii(kinfo->bug_buf[i]) || !isprint(kinfo->bug_buf[i]))
			return false;
	}

	for (i = 0; i < sizeof(kinfo->panic_buf); i++) {
		if (kinfo->panic_buf[i] == '\0')
			break;
		else if (!isascii(kinfo->panic_buf[i]) || !isprint(kinfo->panic_buf[i]))
			return false;
	}

	return true;
}

static ssize_t extra_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = EXTRA_LEN_STR;
	unsigned int reset_reason;
	rst_exinfo_t *rst_exinfo;
	_kern_ex_info_t *kinfo;

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

	kinfo = &rst_exinfo->kern_ex_info.info;
	/* FIXME: it seems like, if the 1st upload is not a kernel panic,
	 * 'kinfo' area is not initialized by BL. */
	if (!__extra_info_kinfo_is_valid(kinfo))
		memset(kinfo, 0x0, sizeof(*kinfo));

	info_size = __extra_info_report_reset_reason(reset_reason, buf, sz_buf, info_size);
	info_size = __extra_info_report_task(kinfo, buf, sz_buf, info_size);
	info_size = __extra_info_report_smmu(kinfo, buf, sz_buf, info_size);
	info_size = __extra_info_report_badmode(kinfo, buf, sz_buf, info_size);
	info_size = __extra_info_report_fault(kinfo, buf, sz_buf, info_size);
	info_size = __extra_info_report_dying_msg(rst_exinfo, buf, sz_buf, info_size);
	info_size = __extra_info_report_context(kinfo, buf, sz_buf, info_size);

err_failed_to_read:
	kfree(rst_exinfo);
err_enomem:
	__qc_hw_param_clean_format(buf, &info_size, sz_buf);
err_invalid_reset_reason:
	return info_size;
}
DEVICE_ATTR_RO(extra_info);
