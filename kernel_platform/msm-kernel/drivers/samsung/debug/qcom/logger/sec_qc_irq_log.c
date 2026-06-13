// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#include <trace/events/irq.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/qcom/sec_qc_irq_log.h>
#include <linux/samsung/debug/sec_debug_region.h>

#include "sec_qc_logger.h"

static struct sec_qc_irq_log_data *irq_log_data __read_mostly;

static __always_inline void __irq_log(unsigned int irq, void *fn,
		const char *name, unsigned int en)
{
	struct sec_qc_irq_buf *irq_buf;
	int cpu = smp_processor_id();
	unsigned int i;

	i = ++(irq_log_data[cpu].idx) & (SEC_QC_IRQ_LOG_MAX - 1);
	irq_buf = &irq_log_data[cpu].buf[i];

	irq_buf->time = __qc_logger_cpu_clock(cpu);
	irq_buf->irq = irq;
	irq_buf->fn = fn;
	irq_buf->name = name;
	irq_buf->en = irqs_disabled();
	irq_buf->preempt_count = preempt_count();
	irq_buf->context = &cpu;
	irq_buf->pid = current->pid;
	irq_buf->entry_exit = en;
}

static int __irq_log_set_irq_log_data(struct builder *bd)
{
	struct qc_logger *logger = container_of(bd, struct qc_logger, bd);
	struct sec_dbg_region_client *client = logger->drvdata->client;
	int cpu;

	irq_log_data = (void *)client->virt;
	if (IS_ERR_OR_NULL(irq_log_data))
		return -EFAULT;

	for_each_possible_cpu(cpu) {
		irq_log_data[cpu].idx = -1;
	}

	return 0;
}

static void __irq_log_unset_irq_log_data(struct builder *bd)
{
	irq_log_data = NULL;
}

static void sec_qc_trace_irq_handler_entry(void *unused,
		int irq, struct irqaction *action)
{
	__irq_log(irq, action->handler, (char *)action->name, IRQ_ENTRY);
}

static int __irq_log_register_trace_irq_handler_entry(struct builder *bd)
{
	return register_trace_irq_handler_entry(sec_qc_trace_irq_handler_entry,
			NULL);
}

static void __irq_log_unregister_trace_irq_handler_entry(struct builder *bd)
{
	unregister_trace_irq_handler_entry(sec_qc_trace_irq_handler_entry,
			NULL);
}

static void sec_qc_trace_irq_handler_exit(void *unused,
		int irq, struct irqaction *action, int res)
{
	__irq_log(irq, action->handler, (char *)action->name, IRQ_EXIT);
}

static int __irq_log_register_trace_irq_handler_exit(struct builder *bd)
{
	return register_trace_irq_handler_exit(sec_qc_trace_irq_handler_exit,
			NULL);
}

static void __irq_log_unregister_trace_irq_handler_exit(struct builder *bd)
{
	unregister_trace_irq_handler_exit(sec_qc_trace_irq_handler_exit, NULL);
}

#if 0
/* NOTE: 'softirq_to_name' is not exported to the kernel moodule.
 * We should use a built-in array of names.
 */
static const char * const __softirq_to_name[NR_SOFTIRQS] = {
	"softirq-HI", "softirq-TIMER", "softirq-NET_TX", "softirq-NET_RX",
	"softirq-BLOCK", "softirq-IRQ_POLL", "softirq-TASKLET",
	"softirq-SCHED", "softirq-HRTIMER", "softirq-RCU"
};

static void sec_qc_trace_softirq_entry(void *unused, unsigned int vec_nr)
{
	__irq_log(-1, NULL, __softirq_to_name[vec_nr], SOFTIRQ_ENTRY);
}

static int __irq_log_register_trace_softirq_entry(struct builder *bd)
{
	return register_trace_softirq_entry(sec_qc_trace_softirq_entry, NULL);
}

static void __irq_log_unregister_trace_softirq_entry(struct builder *bd)
{
	unregister_trace_softirq_entry(sec_qc_trace_softirq_entry, NULL);
}

static void sec_qc_trace_softirq_exit(void *unused, unsigned int vec_nr)
{
	__irq_log(-1, NULL, __softirq_to_name[vec_nr], SOFTIRQ_EXIT);
}

static int __irq_log_register_trace_softirq_exit(struct builder *bd)
{
	return register_trace_softirq_exit(sec_qc_trace_softirq_exit, NULL);
}

static void __irq_log_unregister_trace_softirq_exit(struct builder *bd)
{
	unregister_trace_softirq_exit(sec_qc_trace_softirq_exit, NULL);
}

static void sec_qc_trace_tasklet_entry(void *unused,
		int irq, struct tasklet_struct *t)
{
	__irq_log(-1, t->func, "tasklet_action", SOFTIRQ_ENTRY);
}

static int __irq_log_register_trace_tasklet_entry(struct builder *bd)
{
	return register_trace_tasklet_entry(sec_qc_trace_tasklet_entry, NULL);
}

static void __irq_log_unregister_trace_tasklet_entry(struct builder *bd)
{
	unregister_trace_tasklet_entry(sec_qc_trace_tasklet_entry, NULL);
}

static void sec_qc_trace_tasklet_exit(void *unused,
		int irq, struct tasklet_struct *t)
{
	__irq_log(-1, t->func, "tasklet_action", SOFTIRQ_EXIT);
}

static int __irq_log_register_trace_tasklet_exit(struct builder *bd)
{
	return register_trace_tasklet_exit(sec_qc_trace_tasklet_exit, NULL);
}

static void __irq_log_unregister_trace_tasklet_exit(struct builder *bd)
{
	unregister_trace_tasklet_exit(sec_qc_trace_tasklet_exit, NULL);
}
#endif

static size_t sec_qc_irq_log_get_data_size(struct qc_logger *logger)
{
	return sizeof(struct sec_qc_irq_log_data) * num_possible_cpus();
}

static const struct dev_builder __irq_log_dev_builder[] = {
	DEVICE_BUILDER(__irq_log_set_irq_log_data,
		       __irq_log_unset_irq_log_data),
};

static const struct dev_builder __irq_log_dev_builder_trace[] = {
	DEVICE_BUILDER(__irq_log_register_trace_irq_handler_entry,
		       __irq_log_unregister_trace_irq_handler_entry),
	DEVICE_BUILDER(__irq_log_register_trace_irq_handler_exit,
		       __irq_log_unregister_trace_irq_handler_exit),
#if 0
	DEVICE_BUILDER(__irq_log_register_trace_softirq_entry,
		       __irq_log_unregister_trace_softirq_entry),
	DEVICE_BUILDER(__irq_log_register_trace_softirq_exit,
		       __irq_log_unregister_trace_softirq_exit),
	DEVICE_BUILDER(__irq_log_register_trace_tasklet_entry,
		       __irq_log_unregister_trace_tasklet_entry),
	DEVICE_BUILDER(__irq_log_register_trace_tasklet_exit,
		       __irq_log_unregister_trace_tasklet_exit),
#endif
};

static int sec_qc_irq_log_probe(struct qc_logger *logger)
{
	int err = __qc_logger_sub_module_probe(logger,
			__irq_log_dev_builder,
			ARRAY_SIZE(__irq_log_dev_builder));
	if (err)
		return err;

	return __qc_logger_sub_module_probe(logger,
			__irq_log_dev_builder_trace,
			ARRAY_SIZE(__irq_log_dev_builder_trace));
}

static void sec_qc_irq_log_remove(struct qc_logger *logger)
{
	__qc_logger_sub_module_remove(logger,
			__irq_log_dev_builder,
			ARRAY_SIZE(__irq_log_dev_builder));

	__qc_logger_sub_module_remove(logger,
			__irq_log_dev_builder_trace,
			ARRAY_SIZE(__irq_log_dev_builder_trace));
}

struct qc_logger __qc_logger_irq_log = {
	.get_data_size = sec_qc_irq_log_get_data_size,
	.probe = sec_qc_irq_log_probe,
	.remove = sec_qc_irq_log_remove,
	.unique_id = SEC_QC_IRQ_LOG_MAGIC,
};
