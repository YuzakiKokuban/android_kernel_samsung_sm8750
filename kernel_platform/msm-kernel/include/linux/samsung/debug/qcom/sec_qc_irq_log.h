#ifndef __SEC_QC_IRQ_LOG_H__
#define __SEC_QC_IRQ_LOG_H__

#include <dt-bindings/samsung/debug/qcom/sec_qc_irq_log.h>

#define SEC_QC_IRQ_LOG_MAX	512

#define IRQ_ENTRY		0x4945
#define IRQ_EXIT		0x4958

#define SOFTIRQ_ENTRY		0x5345
#define SOFTIRQ_EXIT		0x5358

struct sec_qc_irq_buf {
	u64 time;
	unsigned int irq;
	void *fn;
	const char *name;
	int en;
	int preempt_count;
	void *context;
	pid_t pid;
	unsigned int entry_exit;
};

struct sec_qc_irq_log_data {
	unsigned int idx;
	struct sec_qc_irq_buf buf[SEC_QC_IRQ_LOG_MAX];
} ____cacheline_aligned_in_smp;

#if IS_BUILTIN(CONFIG_SEC_QC_LOGGER)
/* called @ kernel/irq/chip.c */
/* called @ kernel/irq/handle.c */
/* called @ kernel/softirq.c */
extern void sec_debug_irq_sched_log(unsigned int irq, void *fn, char *name, unsigned int en);
#else
static inline void sec_debug_irq_sched_log(unsigned int irq, void *fn, char *name, unsigned int en) {}
#endif

#endif /* __SEC_QC_IRQ_LOG_H__ */
