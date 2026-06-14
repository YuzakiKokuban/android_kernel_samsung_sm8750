#ifndef __SEC_QC_MOCK_QCOM_WDT_CORE_H__
#define __SEC_QC_MOCK_QCOM_WDT_CORE_H__

#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
/* implemented @drivers/cpuidle/governors/qcom-lpm-sec-extra.c */
extern void qcom_lpm_set_sleep_disabled(void);
extern void qcom_lpm_unset_sleep_disabled(void);
#else
static inline void qcom_lpm_set_sleep_disabled(void) {}
static inline void qcom_lpm_unset_sleep_disabled(void) {}
#endif

#endif /* __SEC_QC_MOCK_QCOM_WDT_CORE_H__ */
