// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/sched/clock.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param.h"

static inline void __last_dcvs_update(struct qc_hw_param_drvdata *drvdata,
		int cap, int volt, int temp, int curr)
{
	ap_health_t *health = drvdata->ap_health;
	uint32_t tail;

	if ((health->battery.tail & 0xf) >= MAX_BATT_DCVS)
		health->battery.tail = 0x10;

	tail = health->battery.tail & 0xf;

	health->battery.batt[tail].ktime = local_clock();
	health->battery.batt[tail].cap = cap;
	health->battery.batt[tail].volt = volt;
	health->battery.batt[tail].temp = temp;
	health->battery.batt[tail].curr = curr;

	health->battery.tail++;
}

void battery_last_dcvs(int cap, int volt, int temp, int curr)
{
	if (!__qc_hw_param_is_probed()) {
		pr_warn("driver is not proved!\n");
		return;
	}

	__last_dcvs_update(qc_hw_param, cap, volt, temp, curr);
}
EXPORT_SYMBOL_GPL(battery_last_dcvs);

static ssize_t __last_dcvs_report_reset_reason(unsigned int reset_reason,
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

static ssize_t __last_dcvs_report_apps(const dcvs_info_t *last_dcvs,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const apps_dcvs_t *apps = &last_dcvs->apps[0];
	const char *prefix[MAX_CLUSTER_NUM] = { "L3", "SC", "GC", "GP" };
	size_t i;

	for (i = 0; i < MAX_CLUSTER_NUM; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"%sKHz\":\"%u\",", prefix[i], apps[i].cpu_KHz);

	return info_size;
}

static ssize_t __last_dcvs_report_rpm(const dcvs_info_t *last_dcvs,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const rpm_dcvs_t *rpm = &last_dcvs->rpm;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"DDRKHz\":\"%u\",", rpm->ddr_KHz);

	return info_size;
}

static ssize_t __last_dcvs_report_batt(const dcvs_info_t *last_dcvs,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const batt_dcvs_t *batt = &last_dcvs->batt;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BCAP\":\"%d\",", batt->cap);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BVOL\":\"%d\",", batt->volt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BTEM\":\"%d\",", batt->temp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"BCUR\":\"%d\",", batt->curr);

	return info_size;
}

static ssize_t __last_dcvs_report_pon(const dcvs_info_t *last_dcvs,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const pon_dcvs_t *pon = &last_dcvs->pon;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"PON\":\"%llx\",", pon->pon_reason);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"PONF\":\"%llx\",", pon->fault_reason);

	return info_size;
}

static ssize_t last_dcvs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int reset_reason;
	dcvs_info_t *last_dcvs;
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;

	reset_reason = sec_qc_reset_reason_get();
	if (!__qc_hw_param_is_valid_reset_reason(reset_reason))
		goto err_invalid_reset_reason;

	last_dcvs = &qc_hw_param->ap_health->last_dcvs;

	info_size = __last_dcvs_report_reset_reason(reset_reason, buf, sz_buf, info_size);
	info_size = __last_dcvs_report_apps(last_dcvs, buf, sz_buf, info_size);
	info_size = __last_dcvs_report_rpm(last_dcvs, buf, sz_buf, info_size);
	info_size = __last_dcvs_report_batt(last_dcvs, buf, sz_buf, info_size);
	info_size = __last_dcvs_report_pon(last_dcvs, buf, sz_buf, info_size);

	/* remove the last ',' character */
	info_size--;

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

err_invalid_reset_reason:
	return info_size;
}
DEVICE_ATTR_RO(last_dcvs);

static void __last_dcvs_clear_batt_info(struct qc_hw_param_drvdata *drvdata)
{
	ap_health_t *health = drvdata->ap_health;

	memset(&health->battery, 0x00, sizeof(battery_health_t));
}

int __qc_last_dcvs_init(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);

	__last_dcvs_clear_batt_info(drvdata);

	return 0;
}
