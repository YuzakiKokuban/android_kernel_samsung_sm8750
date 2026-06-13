// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/task.h>


#include "sec_qc_summary.h"

#define __type_member(__ptr, __type, __member) \
{ \
	(__ptr)->size = sizeof(((__type *)0)->__member); \
	(__ptr)->offset = offsetof(__type, __member); \
}

#define task_struct_member(__ptr, __member) \
	__type_member(__ptr, struct task_struct, __member)

#define thread_info_member(__ptr, __member) \
	__type_member(__ptr, struct thread_info, __member)

static inline void __summary_task_struct_init(
		struct qc_summary_drvdata *drvdata,
		struct sec_qc_summary_task *task)
{
	task->stack_size = THREAD_SIZE;
	task->start_sp = THREAD_SIZE;

	task->ts.struct_size = sizeof(struct task_struct);
#if IS_ENABLED(CONFIG_THREAD_INFO_IN_TASK)
	task_struct_member(&task->ts.cpu, thread_info.cpu);
#endif
	task_struct_member(&task->ts.state, __state);
	task_struct_member(&task->ts.exit_state, exit_state);
	task_struct_member(&task->ts.stack, stack);
	task_struct_member(&task->ts.flags, flags);
#if IS_ENABLED(CONFIG_SMP)
	task_struct_member(&task->ts.on_cpu, on_cpu);
#endif
	task_struct_member(&task->ts.pid, pid);
	task_struct_member(&task->ts.comm, comm);
	task_struct_member(&task->ts.tasks_next, tasks.next);
	task_struct_member(&task->ts.thread_group_next, thread_group.next);
#if IS_ENABLED(CONFIG_SCHED_INFO)
	/* sched_info */
	task_struct_member(&task->ts.sched_info__pcount, sched_info.pcount);
	task_struct_member(&task->ts.sched_info__run_delay, sched_info.run_delay);
	task_struct_member(&task->ts.sched_info__last_arrival, sched_info.last_arrival);
	task_struct_member(&task->ts.sched_info__last_queued, sched_info.last_queued);
#endif

	task->init_task = (uint64_t)&init_task;
	task->ropp.magic = 0x0;		/* FIXME: deprecated feature. */
}

#if IS_ENABLED(CONFIG_ARM64)
static inline void __summary_task_thread_info_init(
		struct qc_summary_drvdata *drvdata,
		struct sec_qc_summary_task *task)
{
	task->ti.struct_size = sizeof(struct thread_info);
	thread_info_member(&task->ti.flags, flags);

	task_struct_member(&task->ts.fp, thread.cpu_context.fp);
	task_struct_member(&task->ts.sp, thread.cpu_context.sp);
	task_struct_member(&task->ts.pc, thread.cpu_context.pc);

#if defined (CONFIG_CFP_ROPP) || defined(CONFIG_RKP_CFP_ROPP)
	thread_info_member(&task->ti.rrk, rrk);
#endif
}

/* arch/arm64/kernel/irq.c */
DECLARE_PER_CPU(unsigned long *, irq_stack_ptr);
DECLARE_PER_CPU_ALIGNED(unsigned long [IRQ_STACK_SIZE/sizeof(long)], irq_stack);

static inline void __summary_task_irq_init_builtin(
		struct qc_summary_drvdata *drvdata,
		struct sec_qc_summary_task *task)
{
#if IS_ENABLED(CONFIG_VMAP_STACK)
	task->irq_stack.pcpu_stack = (uint64_t)&irq_stack_ptr;
#else
	task->irq_stack.pcpu_stack = (uint64_t)&irq_stack;
#endif
	task->irq_stack.size = IRQ_STACK_SIZE;
	task->irq_stack.start_sp = IRQ_STACK_SIZE;
}

static inline void __summary_task_irq_init_module(
		struct qc_summary_drvdata *drvdata,
		struct sec_qc_summary_task *task)
{
#if IS_ENABLED(CONFIG_VMAP_STACK)
	task->irq_stack.pcpu_stack = (uint64_t)
			__qc_summary_kallsyms_lookup_name(drvdata, "irq_stack_ptr");
#else
	task->irq_stack.pcpu_stack = (uint64_t)
			__qc_summary_kallsyms_lookup_name(drvdata, "irq_stack");
#endif

	task->irq_stack.size = IRQ_STACK_SIZE;
	task->irq_stack.start_sp = IRQ_STACK_SIZE;
}

static inline void __summary_task_irq_init(struct qc_summary_drvdata *drvdata,
		struct sec_qc_summary_task *task)
{
	if (IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		__summary_task_irq_init_builtin(drvdata, task);
	else
		__summary_task_irq_init_module(drvdata, task);
}
#else
static inline void __summary_task_thread_info_init(struct qc_summary_drvdata *drvdata, struct sec_qc_summary_task *task) {}
static inline void __summary_task_irq_init(struct qc_summary_drvdata *drvdata, struct sec_qc_summary_task *task) {}
#endif

int notrace __qc_summary_task_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct sec_qc_summary_task *task =
			&(secdbg_apss(drvdata)->task);

	__summary_task_struct_init(drvdata, task);
	__summary_task_thread_info_init(drvdata, task);
	__summary_task_irq_init(drvdata, task);

	return 0;
}
