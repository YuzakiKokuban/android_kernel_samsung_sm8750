#ifndef __SEC_QC_MOCK_QCOM_CPUFREQ_HW_H__
#define __SEC_QC_MOCK_QCOM_CPUFREQ_HW_H__

/* implemented @ drivers/cpufreq/qcom-cpufreq-hw.c */
#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
extern int qcom_cpufreq_hw_target_index_register_notifier(struct notifier_block *nb);
extern int qcom_cpufreq_hw_target_index_unregister_notifier(struct notifier_block *nb);
#else
static inline int qcom_cpufreq_hw_target_index_register_notifier(struct notifier_block *nb) { return 0; }
static inline int qcom_cpufreq_hw_target_index_unregister_notifier(struct notifier_block *nb) { return 0; }
#endif

#endif /* __SEC_QC_MOCK_QCOM_CPUFREQ_HW_H__ */
