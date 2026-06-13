// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kdebug.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/sched.h>
#include <linux/panic_notifier.h>
#include <linux/workqueue.h>

#include <trace/events/sched.h>
#include <trace/events/workqueue.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug_region.h>
#include <linux/samsung/debug/qcom/sec_qc_sched_log.h>

#include "sec_qc_logger.h"

static struct sec_qc_sched_log_data *sched_log_data __read_mostly;

static __always_inline void strcpy_task_comm(char *dst, char *src)
{
	if (TASK_COMM_LEN == 16) {
		struct __task_comm {
			uint64_t l;
			uint64_t h;
		} __attribute__((aligned(8))) *__dst, *__src;
		__dst = (void *)dst;
		__src = (void *)src;
		__dst->l = __src->l;
		__dst->h = __src->h;
	} else
		memcpy(dst, src, TASK_COMM_LEN);
}

static __always_inline long get_switch_state(bool preempt,
		struct task_struct *p)
{
	return preempt ? TASK_RUNNING | TASK_STATE_MAX : p->__state;
}

static void sec_qc_trace_sched_switch(void *unused, bool preempt,
		struct task_struct *prev, struct task_struct *next,
		unsigned prev_state)
{
	struct sec_qc_sched_buf *sched_buf;
	int cpu = smp_processor_id();
	unsigned int i;

	i = ++(sched_log_data[cpu].idx) & (SEC_QC_SCHED_LOG_MAX - 1);
	sched_buf = &sched_log_data[cpu].buf[i];

	sched_buf->time = __qc_logger_cpu_clock(cpu);

	strcpy_task_comm(sched_buf->comm, next->comm);
	sched_buf->pid = next->pid;
	sched_buf->task = next;
	strcpy_task_comm(sched_buf->prev_comm, prev->comm);
	sched_buf->prio = next->prio;
	sched_buf->prev_pid = prev->pid;
	sched_buf->prev_state = get_switch_state(preempt, prev);
	sched_buf->prev_prio = prev->prio;
}

static int __sched_log_set_sched_log_data(struct builder *bd)
{
	struct qc_logger *logger = container_of(bd, struct qc_logger, bd);
	struct sec_dbg_region_client *client = logger->drvdata->client;
	int cpu;

	sched_log_data = (void *)client->virt;
	if (IS_ERR_OR_NULL(sched_log_data))
		return -EFAULT;

	for_each_possible_cpu(cpu) {
		sched_log_data[cpu].idx = -1;
	}

	return 0;
}

static void __sched_log_unset_sched_log_data(struct builder *bd)
{
	sched_log_data = NULL;
}

static int notrace __sched_log_msg(char *fmt, ...)
{
	struct sec_qc_sched_buf *sched_buf;
	int cpu = smp_processor_id();
	int r;
	int i;
	va_list args;

	i = ++(sched_log_data[cpu].idx) & (SEC_QC_SCHED_LOG_MAX - 1);
	sched_buf = &sched_log_data[cpu].buf[i];

	va_start(args, fmt);
	r = vsnprintf(sched_buf->comm, sizeof(sched_buf->comm), fmt, args);
	va_end(args);

	sched_buf->time = __qc_logger_cpu_clock(cpu);
	sched_buf->task = NULL;
	sched_buf->pid = current->pid;

	return r;
}

static int __sched_log_register_trace_sched_switch(struct builder *bd)
{
	return register_trace_sched_switch(sec_qc_trace_sched_switch, NULL);
}

static void __sched_log_unregister_trace_sched_switch(struct builder *bd)
{
	unregister_trace_sched_switch(sec_qc_trace_sched_switch, NULL);
}

static void sec_qc_trace_workqueue_execute_start(void *unsed,
		struct work_struct *work)
{
	struct sec_qc_sched_buf *sched_buf;
	int cpu = smp_processor_id();
	int i;

	i = ++(sched_log_data[cpu].idx) & (SEC_QC_SCHED_LOG_MAX - 1);
	sched_buf = &sched_log_data[cpu].buf[i];

	sched_buf->addr = (uintptr_t)work->func;
	sched_buf->time = __qc_logger_cpu_clock(cpu);
	sched_buf->task = NULL;
	sched_buf->pid = current->pid;
}

static int __sched_log_register_trace_workqueue_execute_start(
		struct builder *bd)
{
	return register_trace_workqueue_execute_start(
			sec_qc_trace_workqueue_execute_start, NULL);
}

static void __sched_log_unregister_trace_workqueue_execute_start(
		struct builder *bd)
{
	unregister_trace_workqueue_execute_start(
			sec_qc_trace_workqueue_execute_start, NULL);
}

static size_t sec_qc_shed_log_get_data_size(struct qc_logger *logger)
{
	return sizeof(struct sec_qc_sched_log_data) * num_possible_cpus();
}

static int sec_qc_sched_log_die_notifier_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	__sched_log_msg("!!die!!");
	__sched_log_msg("!!die!!");

	return NOTIFY_OK;
}

static struct notifier_block sec_qc_sched_log_die_handle = {
	.notifier_call = sec_qc_sched_log_die_notifier_call,
	.priority = 0x7FFFFFFF,		/* most high priority */
};

static int __sched_log_register_die_handle(struct builder *bd)
{
	return register_die_notifier(&sec_qc_sched_log_die_handle);
}

static void __sched_log_unregister_die_handle(struct builder *bd)
{
	unregister_die_notifier(&sec_qc_sched_log_die_handle);
}

static int sec_qc_sched_log_panic_notifier_call(struct notifier_block *this,
		unsigned long l, void *d)
{
	__sched_log_msg("!!panic!!");
	__sched_log_msg("!!panic!!");

	return NOTIFY_OK;
}

static struct notifier_block sec_qc_sched_log_panic_handle = {
	.notifier_call = sec_qc_sched_log_panic_notifier_call,
	.priority = 0x7FFFFFFF,		/* most high priority */
};

static int __sched_log_register_panic_handle(struct builder *bd)
{
	return atomic_notifier_chain_register(&panic_notifier_list,
			&sec_qc_sched_log_panic_handle);
}

static void __sched_log_unregister_panic_handle(struct builder *bd)
{
	atomic_notifier_chain_unregister(&panic_notifier_list,
			&sec_qc_sched_log_panic_handle);
}

static const struct dev_builder __sched_log_dev_builder[] = {
	DEVICE_BUILDER(__sched_log_set_sched_log_data,
		       __sched_log_unset_sched_log_data),
};

static const struct dev_builder __sched_log_dev_builder_trace[] = {
	DEVICE_BUILDER(__sched_log_register_trace_sched_switch,
		       __sched_log_unregister_trace_sched_switch),
	DEVICE_BUILDER(__sched_log_register_trace_workqueue_execute_start,
		       __sched_log_unregister_trace_workqueue_execute_start),
	DEVICE_BUILDER(__sched_log_register_die_handle,
		       __sched_log_unregister_die_handle),
	DEVICE_BUILDER(__sched_log_register_panic_handle,
		       __sched_log_unregister_panic_handle),
};

static int sec_qc_sched_log_probe(struct qc_logger *logger)
{
	int err = __qc_logger_sub_module_probe(logger,
			__sched_log_dev_builder,
			ARRAY_SIZE(__sched_log_dev_builder));
	if (err)
		return err;

	return __qc_logger_sub_module_probe(logger,
			__sched_log_dev_builder_trace,
			ARRAY_SIZE(__sched_log_dev_builder_trace));
}

static void sec_qc_sched_log_remove(struct qc_logger *logger)
{
	__qc_logger_sub_module_remove(logger,
			__sched_log_dev_builder_trace,
			ARRAY_SIZE(__sched_log_dev_builder_trace));

	__qc_logger_sub_module_remove(logger,
			__sched_log_dev_builder,
			ARRAY_SIZE(__sched_log_dev_builder));
}

struct qc_logger __qc_logger_sched_log = {
	.get_data_size = sec_qc_shed_log_get_data_size,
	.probe = sec_qc_sched_log_probe,
	.remove = sec_qc_sched_log_remove,
	.unique_id = SEC_QC_SCHED_LOG_MAGIC,
};
