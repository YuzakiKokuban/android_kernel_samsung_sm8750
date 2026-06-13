#ifndef __SEC_QC_MOCK_QCOM_WDT_CORE_H__
#define __SEC_QC_MOCK_QCOM_WDT_CORE_H__

#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
/* implemented @drivers/soc/qcom/qcom_wdt_core.c */
extern int qcom_wdt_pet_register_notifier(struct notifier_block *nb);
extern int qcom_wdt_pet_unregister_notifier(struct notifier_block *nb);
extern int qcom_wdt_bark_register_notifier(struct notifier_block *nb);
extern int qcom_wdt_bark_unregister_notifier(struct notifier_block *nb);
#else
static inline int qcom_wdt_pet_register_notifier(struct notifier_block *nb) { return 0; }
static inline int qcom_wdt_pet_unregister_notifier(struct notifier_block *nb) { return 0; }
static inline int qcom_wdt_bark_register_notifier(struct notifier_block *nb) { return 0; }
static inline int qcom_wdt_bark_unregister_notifier(struct notifier_block *nb) { return 0; }
#endif

#endif /* __SEC_QC_MOCK_QCOM_WDT_CORE_H__ */
