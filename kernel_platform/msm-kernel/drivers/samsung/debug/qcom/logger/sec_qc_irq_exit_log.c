// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug_region.h>
#include <linux/samsung/debug/qcom/sec_qc_irq_exit_log.h>

#include "sec_qc_logger.h"

static struct sec_qc_irq_exit_log_data *irq_exit_log_data __read_mostly;

static __always_inline void __irq_exit_log(unsigned int irq, u64 start_time)
{
	struct sec_qc_irq_exit_buf *irq_exit_buf;
	int cpu = smp_processor_id();
	unsigned int i;

	i = ++(irq_exit_log_data[cpu].idx) & (SEC_QC_IRQ_EXIT_LOG_MAX - 1);
	irq_exit_buf = &irq_exit_log_data[cpu].buf[i];

	irq_exit_buf->irq = irq;
	irq_exit_buf->time = start_time;
	irq_exit_buf->end_time = __qc_logger_cpu_clock(cpu);
	irq_exit_buf->elapsed_time = irq_exit_buf->end_time - start_time;
	irq_exit_buf->pid = current->pid;
}

static int __irq_exit_log_set_irq_exit_log_data(struct builder *bd)
{
	struct qc_logger *logger = container_of(bd, struct qc_logger, bd);
	struct sec_dbg_region_client *client = logger->drvdata->client;
	int cpu;

	irq_exit_log_data = (void *)client->virt;
	if (IS_ERR_OR_NULL(irq_exit_log_data))
		return -EFAULT;

	for_each_possible_cpu(cpu) {
		irq_exit_log_data[cpu].idx = -1;
	}

	return 0;
}

static void __irq_exit_log_unset_irq_exit_log_data(struct builder *bd)
{
	irq_exit_log_data = NULL;
}

static size_t sec_qc_irq_exit_log_get_data_size(struct qc_logger *logger)
{
	return sizeof(struct sec_qc_irq_exit_log_data) * num_possible_cpus();
}

static const struct dev_builder __irq_exit_log_dev_builder[] = {
	DEVICE_BUILDER(__irq_exit_log_set_irq_exit_log_data,
		       __irq_exit_log_unset_irq_exit_log_data),
};

static int sec_qc_irq_exit_log_probe(struct qc_logger *logger)
{
	return __qc_logger_sub_module_probe(logger,
			__irq_exit_log_dev_builder,
			ARRAY_SIZE(__irq_exit_log_dev_builder));
}

static void sec_qc_irq_exit_log_remove(struct qc_logger *logger)
{
	__qc_logger_sub_module_remove(logger,
			__irq_exit_log_dev_builder,
			ARRAY_SIZE(__irq_exit_log_dev_builder));
}

struct qc_logger __qc_logger_irq_exit_log = {
	.get_data_size = sec_qc_irq_exit_log_get_data_size,
	.probe = sec_qc_irq_exit_log_probe,
	.remove = sec_qc_irq_exit_log_remove,
	.unique_id = SEC_QC_IRQ_EXIT_LOG_MAGIC,
};
