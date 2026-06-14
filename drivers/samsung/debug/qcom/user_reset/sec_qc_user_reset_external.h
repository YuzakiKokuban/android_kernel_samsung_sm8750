#ifndef __INTERNAL__SEC_QC_USER_RESET_EXTERNAL_H__
#define __INTERNAL__SEC_QC_USER_RESET_EXTERNAL_H__

#if IS_ENABLED(CONFIG_EDAC_KRYO_ARM64)
/* implemented @ drivers/edac/kryo_arm64_edac.c */
extern ap_health_t *qcom_kryo_arm64_edac_error_register_notifier(struct notifier_block *nb);
extern int qcom_kryo_arm64_edac_error_unregister_notifier(struct notifier_block *nb);
#else
#include <linux/samsung/debug/qcom/mock/sec_qc_mock_kryo_arm64_edac.h>
#endif

#endif /* __INTERNAL__SEC_QC_USER_RESET_EXTERNAL_H__ */
