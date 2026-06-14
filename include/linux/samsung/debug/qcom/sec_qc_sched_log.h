#ifndef __SEC_QC_SCHED_LOG_H__
#define __SEC_QC_SCHED_LOG_H__

#include <dt-bindings/samsung/debug/qcom/sec_qc_sched_log.h>

#define SEC_QC_SCHED_LOG_MAX	512

struct sec_qc_sched_buf {
	u64 time;
	union {
		char comm[TASK_COMM_LEN];
		u64 addr;
	};
	pid_t pid;
	struct task_struct *task;
	char prev_comm[TASK_COMM_LEN];
	int prio;
	pid_t prev_pid;
	int prev_prio;
	int prev_state;
};

struct sec_qc_sched_log_data {
	unsigned int idx;
	struct sec_qc_sched_buf buf[SEC_QC_SCHED_LOG_MAX];
} ____cacheline_aligned_in_smp;

#if IS_BUILTIN(CONFIG_SEC_QC_LOGGER)
/* called @ kernel/sched/core.c */
extern void sec_debug_task_sched_log(int cpu, bool preempt, struct task_struct *task, struct task_struct *prev);

/* called @ drivers/cpuidle/lpm-levels.c */
/* called @ kernel/workqueue.c */
extern int sec_debug_sched_msg(char *fmt, ...);
#else
static inline void sec_debug_task_sched_log(int cpu, bool preempt, struct task_struct *task, struct task_struct *prev) {}
static inline int sec_debug_sched_msg(char *fmt, ...) { return -ENODEV; }
#endif

#endif /* __SEC_QC_SCHED_LOG_H__ */
