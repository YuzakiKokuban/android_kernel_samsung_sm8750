#ifndef __SEC_QC_MOCK_WALT_H__
#define __SEC_QC_MOCK_WALT_H__

/* implemented @ kernel/sched/walt/walt.c */
#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
extern void sec_qc_summary_set_sched_walt_info(struct sec_qc_summary_data_apss *apss);
#else
static inline void sec_qc_summary_set_sched_walt_info(struct sec_qc_summary_data_apss *apss) {}
#endif

#endif /* __SEC_QC_MOCK_WALT_H__ */
