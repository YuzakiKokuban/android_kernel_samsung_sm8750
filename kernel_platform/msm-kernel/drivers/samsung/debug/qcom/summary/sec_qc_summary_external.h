#ifndef __INTERNAL__SEC_QC_SUMMARY_EXTERNAL_H__
#define __INTERNAL__SEC_QC_SUMMARY_EXTERNAL_H__

/* implemented @ drivers/soc/qcom/memory_dump_v2.c */
#if IS_ENABLED(CONFIG_QCOM_MEMORY_DUMP_V2)
extern void sec_qc_summary_set_msm_memdump_info(struct sec_qc_summary_data_apss *apss);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_memory_dump_v2.h>
#endif

/* implemented @ kernel/sched/walt/walt.c */
#if IS_ENABLED(CONFIG_SCHED_WALT)
extern void sec_qc_summary_set_sched_walt_info(struct sec_qc_summary_data_apss *apss);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_walt.h>
#endif

/* implemented @ kernel/trace/msm_rtb.c */
#if IS_ENABLED(CONFIG_QCOM_RTB)
extern void sec_qc_summary_set_rtb_info(struct sec_qc_summary_data_apss *apss);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_msm_rtb.h>
#endif

/* implemented @ drivers/soc/qcom/smem.c */
#if IS_ENABLED(CONFIG_QCOM_SMEM)
#include <linux/soc/qcom/smem.h>
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_smem.h>
#endif

/* implemented @ drivers/soc/qcom/smem.c */
#include <soc/qcom/watchdog.h>

#if IS_ENABLED(CONFIG_QCOM_WDT_CORE)
extern int qcom_wdt_pet_register_notifier(struct notifier_block *nb);
extern int qcom_wdt_pet_unregister_notifier(struct notifier_block *nb);
extern int qcom_wdt_bark_register_notifier(struct notifier_block *nb);
extern int qcom_wdt_bark_unregister_notifier(struct notifier_block *nb);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_qcom_wdt_core.h>
#endif

#endif /* __INTERNAL__SEC_QC_SUMMARY_EXTERNAL_H__ */
