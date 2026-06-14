// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>
#include <linux/sched.h>

#include <linux/samsung/debug/sec_debug_region.h>
#include <linux/samsung/debug/qcom/sec_qc_irq_log.h>
#include <linux/samsung/debug/qcom/sec_qc_irq_exit_log.h>
#include <linux/samsung/debug/qcom/sec_qc_sched_log.h>

#include "sec_qc_summary.h"

static inline void __summary_set_sched_log_info(
		struct sec_qc_summary_sched_log *sched_log)
{
	const struct sec_dbg_region_client *client;

	client = sec_dbg_region_find(SEC_QC_SCHED_LOG_MAGIC);
	if (IS_ERR(client))
		return;

	sched_log->sched_idx_paddr = client->phys +
			offsetof(struct sec_qc_sched_log_data, idx);
	sched_log->sched_buf_paddr = client->phys +
			offsetof(struct sec_qc_sched_log_data, buf);
	sched_log->sched_struct_buf_sz = sizeof(struct sec_qc_sched_buf);
	sched_log->sched_struct_log_sz = sizeof(struct sec_qc_sched_log_data);
	sched_log->sched_array_cnt = SEC_QC_SCHED_LOG_MAX;
}

static inline void __summary_set_irq_log_info(
		struct sec_qc_summary_sched_log *sched_log)
{
	const struct sec_dbg_region_client *client;

	client = sec_dbg_region_find(SEC_QC_IRQ_LOG_MAGIC);
	if (IS_ERR(client))
		return;

	sched_log->irq_idx_paddr = client->phys +
			offsetof(struct sec_qc_irq_log_data, idx);
	sched_log->irq_buf_paddr = client->phys +
			offsetof(struct sec_qc_irq_log_data, buf);
	sched_log->irq_struct_buf_sz = sizeof(struct sec_qc_irq_buf);
	sched_log->irq_struct_log_sz = sizeof(struct sec_qc_irq_log_data);
	sched_log->irq_array_cnt = SEC_QC_IRQ_LOG_MAX;
}

static inline void __summary_set_irq_exit_log_info(
		struct sec_qc_summary_sched_log *sched_log)
{
	const struct sec_dbg_region_client *client;

	client = sec_dbg_region_find(SEC_QC_IRQ_EXIT_LOG_MAGIC);
	if (IS_ERR(client))
		return;

	sched_log->irq_exit_idx_paddr = client->phys +
			offsetof(struct sec_qc_irq_exit_log_data, idx);
	sched_log->irq_exit_buf_paddr = client->phys +
			offsetof(struct sec_qc_irq_exit_log_data, buf);
	sched_log->irq_exit_struct_buf_sz = sizeof(struct sec_qc_irq_exit_buf);
	sched_log->irq_exit_struct_log_sz = sizeof(struct sec_qc_irq_exit_log_data);
	sched_log->irq_exit_array_cnt = SEC_QC_IRQ_EXIT_LOG_MAX;
}

int notrace __qc_summary_sched_log_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct sec_qc_summary_sched_log *sched_log =
			&(secdbg_apss(drvdata)->sched_log);

	__summary_set_sched_log_info(sched_log);
	__summary_set_irq_log_info(sched_log);
	__summary_set_irq_exit_log_info(sched_log);

	return 0;
}
