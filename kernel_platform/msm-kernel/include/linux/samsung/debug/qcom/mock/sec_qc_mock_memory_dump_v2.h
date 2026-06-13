#ifndef __SEC_QC_MOCK_MEMORY_DUMP_V2_H__
#define __SEC_QC_MOCK_MEMORY_DUMP_V2_H__

/* implemented @ drivers/soc/qcom/memory_dump_v2.c */
#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
extern void sec_qc_summary_set_msm_memdump_info(struct sec_qc_summary_data_apss *apss);
#else
static inline void sec_qc_summary_set_msm_memdump_info(struct sec_qc_summary_data_apss *apss) {}
#endif

#endif /* __SEC_QC_MOCK_MEMORY_DUMP_V2_H__ */
