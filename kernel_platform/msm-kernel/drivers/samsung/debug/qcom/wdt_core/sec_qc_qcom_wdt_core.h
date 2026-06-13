#ifndef __INTERNAL__SEC_QC_QCOM_WDT_CORE_H__
#define __INTERNAL__SEC_QC_QCOM_WDT_CORE_H__

struct msm_watchdog_data;

struct qc_wdt_core_drvdata {
	struct builder bd;
	struct msm_watchdog_data* wdog_dd;
	struct notifier_block nb_panic;
	struct notifier_block nb_wdt_bark;
	struct force_err_handle force_err_dp;
	struct force_err_handle force_err_wp;
};

/* implemented @ drivers/soc/qcom/smem.c */
#if IS_ENABLED(CONFIG_QCOM_WDT_CORE)
#include <soc/qcom/watchdog.h>

extern int qcom_wdt_pet_register_notifier(struct notifier_block *nb);
extern int qcom_wdt_pet_unregister_notifier(struct notifier_block *nb);
extern int qcom_wdt_bark_register_notifier(struct notifier_block *nb);
extern int qcom_wdt_bark_unregister_notifier(struct notifier_block *nb);

extern void qcom_lpm_set_sleep_disabled(void);
extern void qcom_lpm_unset_sleep_disabled(void);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_qcom_lpm.h>
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_qcom_wdt_core.h>
#endif

#endif /* __INTERNAL__SEC_QC_QCOM_WDT_CORE_H__ */
