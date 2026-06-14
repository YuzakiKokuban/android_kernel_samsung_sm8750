#ifndef __INTERNAL__SEC_QC_DEBUG_MOCK_H__
#define __INTERNAL__SEC_QC_DEBUG_MOCK_H__

#if IS_ENABLED(CONFIG_QCOM_SCM)
#include <linux/firmware/qcom/qcom_scm.h>
#else
static inline int qcom_scm_sec_wdog_trigger(void) { return -ENODEV; }
static inline bool qcom_scm_is_secure_wdog_trigger_available(void) { return false; }
#endif

#if IS_ENABLED(CONFIG_QCOM_WDT_CORE)
#include <soc/qcom/watchdog.h>
#else
static inline void qcom_wdt_trigger_bite(void) {}
#endif

#endif /* __INTERNAL__SEC_QC_DEBUG_MOCK_H__ */
