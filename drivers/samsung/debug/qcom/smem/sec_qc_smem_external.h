#ifndef __INTERNAL__SEC_QC_SMEM_EXTERNAL_H__
#define __INTERNAL__SEC_QC_SMEM_EXTERNAL_H__

/* implemented @ drivers/cpufreq/qcom-cpufreq-hw.c */
#if IS_ENABLED(CONFIG_ARM_QCOM_CPUFREQ_HW)
extern int qcom_cpufreq_hw_target_index_register_notifier(struct notifier_block *nb);
extern int qcom_cpufreq_hw_target_index_unregister_notifier(struct notifier_block *nb);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_qcom-cpufreq-hw.h>
#endif

/* implemented @ drivers/interconnect/qcom/epss-l3.c */
#if IS_ENABLED(CONFIG_INTERCONNECT_QCOM_EPSS_L3)
extern int qcom_icc_epss_l3_cpu_set_register_notifier(struct notifier_block *nb);
extern int qcom_icc_epss_l3_cpu_set_unregister_notifier(struct notifier_block *nb);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_epss-l3.h>
#endif

#endif /* __INTERNAL__SEC_QC_SMEM_EXTERNAL_H__ */
