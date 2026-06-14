#ifndef __SEC_QC_MOCK_MSM_RTB_H__
#define __SEC_QC_MOCK_MSM_RTB_H__

/* implemented @ kernel/trace/msm_rtb.c */
#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
extern void sec_qc_summary_set_rtb_info(struct sec_qc_summary_data_apss *apss);
#else
static inline void sec_qc_summary_set_rtb_info(struct sec_qc_summary_data_apss *apss) {}
#endif

#endif /* __SEC_QC_MOCK_MSM_RTB_H__ */
