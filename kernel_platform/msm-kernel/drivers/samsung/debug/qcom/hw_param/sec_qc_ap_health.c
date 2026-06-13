// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/cpumask.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param.h"

static ssize_t ap_health_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ap_health_t *health = qc_hw_param->ap_health;
	char clear;
	int err;

	err = sscanf(buf, "%1c", &clear);
	if ((err <= 0) || (clear != 'c' && clear != 'C')) {
		dev_warn(dev, "command error\n");
		return count;
	}

	dev_info(dev, "clear ap_health_data by HQM %zu\n",
			sizeof(ap_health_t));

	/*++ add here need init data by HQM ++*/
	memset(&(health->daily_cache), 0, sizeof(cache_health_t));
	memset(&(health->daily_pcie), 0, sizeof(pcie_health_t) * MAX_PCIE_NUM);
	memset(&(health->daily_rr), 0, sizeof(reset_reason_t));

	sec_qc_ap_health_data_write(health);

	return count;
}

static inline bool __ap_health_is_last_cpu(int cpu)
{
	return cpu == (num_present_cpus() - 1);
}

static inline ssize_t __ap_health_report_cache(const ap_health_t *health,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const cache_health_t *cache = &health->cache;
	int cpu;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"L1c\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", cache->edac[cpu][0].ce_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"L1u\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", cache->edac[cpu][0].ue_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"L2c\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", cache->edac[cpu][1].ce_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"L2u\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", cache->edac[cpu][1].ue_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"L3c\":\"%d\",", cache->edac_l3.ce_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"L3u\":\"%d\",", cache->edac_l3.ue_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"EDB\":\"%d\",", cache->edac_bus_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"LDc\":\"%d\",", cache->edac_llcc_data_ram.ce_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"LDu\":\"%d\",", cache->edac_llcc_data_ram.ue_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"LTc\":\"%d\",", cache->edac_llcc_tag_ram.ce_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"LTu\":\"%d\",", cache->edac_llcc_tag_ram.ue_cnt);

	return info_size;
}

static inline ssize_t __ap_health_report_daily_cache(const ap_health_t *health,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const cache_health_t *daily_cache = &health->daily_cache;
	int cpu;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dL1c\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", daily_cache->edac[cpu][0].ce_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dL1u\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", daily_cache->edac[cpu][0].ue_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dL2c\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", daily_cache->edac[cpu][1].ce_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dL2u\":\"");
	for_each_cpu(cpu, cpu_present_mask) {
		const char *sep = __ap_health_is_last_cpu(cpu) ? "\"," : ",";
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%d%s", daily_cache->edac[cpu][1].ue_cnt, sep);
	}

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dL3c\":\"%d\",", daily_cache->edac_l3.ce_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dL3u\":\"%d\",", daily_cache->edac_l3.ue_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dEDB\":\"%d\",", daily_cache->edac_bus_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dLDc\":\"%d\",", daily_cache->edac_llcc_data_ram.ce_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dLDu\":\"%d\",", daily_cache->edac_llcc_data_ram.ue_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dLTc\":\"%d\",", daily_cache->edac_llcc_tag_ram.ce_cnt);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dLTu\":\"%d\",", daily_cache->edac_llcc_tag_ram.ue_cnt);

	return info_size;
}

static inline ssize_t __ap_health_report_pcie(const ap_health_t *health,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const pcie_health_t *pcie = &health->pcie[0];
	const pcie_health_t *daily_pcie = &health->daily_pcie[0];
	size_t i;

	for (i = 0; i < MAX_PCIE_NUM; i++) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"P%zuPF\":\"%d\",", i, pcie[i].phy_init_fail_cnt);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"P%zuLD\":\"%d\",", i, pcie[i].link_down_cnt);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"P%zuLF\":\"%d\",", i, pcie[i].link_up_fail_cnt);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"P%zuLT\":\"%x\",", i, pcie[i].link_up_fail_ltssm);

		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dP%zuPF\":\"%d\",", i, daily_pcie[i].phy_init_fail_cnt);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dP%zuLD\":\"%d\",", i, daily_pcie[i].link_down_cnt);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dP%zuLF\":\"%d\",", i, daily_pcie[i].link_up_fail_cnt);
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dP%zuLT\":\"%x\",", i, daily_pcie[i].link_up_fail_ltssm);
	}

	return info_size;
}

static inline ssize_t __ap_health_report_daily_rr(const ap_health_t *health,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const reset_reason_t *daily_rr = &health->daily_rr;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dNP\":\"%d\",", daily_rr->np);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dRP\":\"%d\",", daily_rr->rp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dMP\":\"%d\",", daily_rr->mp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dKP\":\"%d\",", daily_rr->kp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dDP\":\"%d\",", daily_rr->dp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dWP\":\"%d\",", daily_rr->wp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dTP\":\"%d\",", daily_rr->tp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dSP\":\"%d\",", daily_rr->sp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dPP\":\"%d\",", daily_rr->pp);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"dCP\":\"%d\"", daily_rr->cp);

	return info_size;
}

static ssize_t ap_health_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	const ap_health_t *health = qc_hw_param->ap_health;
	const ssize_t sz_buf = DEFAULT_LEN_STR;
	ssize_t info_size = 0;

	info_size = __ap_health_report_cache(health, buf, sz_buf, info_size);
	info_size = __ap_health_report_daily_cache(health, buf, sz_buf, info_size);
	info_size = __ap_health_report_pcie(health, buf, sz_buf, info_size);
	info_size = __ap_health_report_daily_rr(health, buf, sz_buf, info_size);

	__qc_hw_param_clean_format(buf, &info_size, DEFAULT_LEN_STR);

	return info_size;
}

DEVICE_ATTR_RW(ap_health);
