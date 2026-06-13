#ifndef __SEC_QC_IRQ_EXIT_LOG_H__
#define __SEC_QC_IRQ_EXIT_LOG_H__

#include <dt-bindings/samsung/debug/qcom/sec_qc_irq_exit_log.h>

#define SEC_QC_IRQ_EXIT_LOG_MAX		512

struct sec_qc_irq_exit_buf {
	unsigned int irq;
	u64 time;
	u64 end_time;
	u64 elapsed_time;
	pid_t pid;
};

struct sec_qc_irq_exit_log_data {
	unsigned int idx;
	struct sec_qc_irq_exit_buf buf[SEC_QC_IRQ_EXIT_LOG_MAX];
} ____cacheline_aligned_in_smp;

#if IS_BUILTIN(CONFIG_SEC_QC_LOGGER)
extern void __deprecated sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time);
#else
static inline void sec_debug_irq_enterexit_log(unsigned int irq, u64 start_time) {}
#endif

#endif /* __SEC_QC_IRQ_EXIT_LOG_H__ */
