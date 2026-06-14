// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/arch_topology.h>
#include <linux/cpufreq.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/sched/clock.h>

#include <clocksource/arm_arch_timer.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

#include "sec_qc_smem.h"

int __qc_smem_clk_osm_probe(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata = container_of(bd,
			struct qc_smem_drvdata, bd);
	struct device *dev = bd->dev;
	smem_apps_stat_t *apps_stat;
	cpuclk_log_t *cpuclk_log = drvdata->cpuclk_log;
	size_t nr_cpuclk_log = ARRAY_SIZE(drvdata->cpuclk_log);
	size_t i;

	for (i = 0; i < nr_cpuclk_log; i++)
		cpuclk_log[i].max_cnt = MAX_CLK_LOG_CNT;

	apps_stat = __qc_smem_id_vendor1_get_apps_stat(drvdata);
	if (IS_ERR_OR_NULL(apps_stat)) {
		dev_warn(dev, "apps_stat is not available for vendor1 v%u\n",
				drvdata->vendor1_ver);
		return 0;
	}

	apps_stat->clk = (void *)virt_to_phys(cpuclk_log);
	dev_info(dev, "vendor1->apps_stat.clk = %p\n", apps_stat->clk);

	return 0;
}

static void __smem_cpuclk_log(cpuclk_log_t *cpuclk_log,
		size_t slot, unsigned long rate)
{
	cpuclk_log_t *clk = &cpuclk_log[slot];
	uint64_t idx = clk->index;
	apps_clk_log_t *log = &clk->log[idx];

	log->ktime = local_clock();
	log->qtime = arch_timer_read_counter();
	log->rate = rate;
	clk->index = (clk->index + 1) % clk->max_cnt;
}

static void __smem_clk_osm_add_log_cpufreq(cpuclk_log_t *cpuclk_log,
		unsigned int cpu, unsigned long rate, const char *name)
{
	unsigned int domain = cpu_topology[cpu].package_id;

	if (!WARN((domain + 1) < PWR_CLUSTER || (domain + 1) > PRIME_CLUSTER,
			"%s : invalid cluster_num(%u), dbg_name(%s)\n",
			__func__, domain + 1, name)) {
		__smem_cpuclk_log(cpuclk_log, domain + 1, rate);
	}
}

static int sec_qc_smem_cpufreq_target_index_handler(struct notifier_block *this,
		unsigned long index, void *d)
{
	struct qc_smem_drvdata *drvdata = container_of(this,
			struct qc_smem_drvdata, nb_cpuclk_log);
	struct cpufreq_policy *policy = d;

	__smem_clk_osm_add_log_cpufreq(drvdata->cpuclk_log,
			policy->cpu,
			policy->freq_table[index].frequency,
			policy->kobj.name);

	return NOTIFY_OK;
}

static __always_inline int ____qc_smem_register_nb_cpuclk_log(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata = container_of(bd,
			struct qc_smem_drvdata, bd);

	drvdata->nb_cpuclk_log.notifier_call =
			sec_qc_smem_cpufreq_target_index_handler;

	return qcom_cpufreq_hw_target_index_register_notifier(
			&drvdata->nb_cpuclk_log);
}

int __qc_smem_register_nb_cpuclk_log(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_ARM_QCOM_CPUFREQ_HW))
		return 0;

	return ____qc_smem_register_nb_cpuclk_log(bd);
}

static __always_inline void ____qc_smem_unregister_nb_cpuclk_log(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata = container_of(bd,
			struct qc_smem_drvdata, bd);

	qcom_cpufreq_hw_target_index_unregister_notifier(
			&drvdata->nb_cpuclk_log);
}

void __qc_smem_unregister_nb_cpuclk_log(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_ARM_QCOM_CPUFREQ_HW))
		return;

	____qc_smem_unregister_nb_cpuclk_log(bd);
}

static void __smem_clk_osm_add_log_l3(cpuclk_log_t *cpuclk_log,
		unsigned long rate)
{
	__smem_cpuclk_log(cpuclk_log, L3, rate / 1000);
}

static int sec_qc_smem_l3_cpu_set_handler(struct notifier_block *this,
		unsigned long index, void *__lut_freqs)
{
	struct qc_smem_drvdata *drvdata = container_of(this,
			struct qc_smem_drvdata, nb_l3clk_log);
	unsigned long *lut_freqs = __lut_freqs;

	__smem_clk_osm_add_log_l3(drvdata->cpuclk_log, lut_freqs[index]);

	return NOTIFY_OK;
}

static __always_inline int ____qc_smem_register_nb_l3clk_log(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata = container_of(bd,
			struct qc_smem_drvdata, bd);

	drvdata->nb_l3clk_log.notifier_call =
			sec_qc_smem_l3_cpu_set_handler;

	return qcom_icc_epss_l3_cpu_set_register_notifier(&drvdata->nb_l3clk_log);
}

int __qc_smem_register_nb_l3clk_log(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_INTERCONNECT_QCOM_EPSS_L3))
		return 0;

	return ____qc_smem_register_nb_l3clk_log(bd);
}

static __always_inline void ____qc_smem_unregister_nb_l3clk_log(struct builder *bd)
{
	struct qc_smem_drvdata *drvdata = container_of(bd,
			struct qc_smem_drvdata, bd);

	qcom_icc_epss_l3_cpu_set_unregister_notifier(&drvdata->nb_l3clk_log);
}

void __qc_smem_unregister_nb_l3clk_log(struct builder *bd)
{
	if (!IS_ENABLED(CONFIG_INTERCONNECT_QCOM_EPSS_L3))
		return;

	____qc_smem_unregister_nb_l3clk_log(bd);
}

