#ifndef __SEC_QC_MOCK_EPSS_L3_H__
#define __SEC_QC_MOCK_EPSS_L3_H__

#include <linux/notifier.h>

/* implemented @ drivers/interconnect/qcom/epss-l3.c */
#if IS_ENABLED(CONFIG_INTERCONNECT_QCOM_EPSS_L3)
extern int qcom_icc_epss_l3_cpu_set_register_notifier(struct notifier_block *nb);
extern int qcom_icc_epss_l3_cpu_set_unregister_notifier(struct notifier_block *nb);
#else
static inline int qcom_icc_epss_l3_cpu_set_register_notifier(struct notifier_block *nb) { return 0; }
static inline int qcom_icc_epss_l3_cpu_set_unregister_notifier(struct notifier_block *nb) { return 0; }
#endif

#endif /* */
