#ifndef __SEC_QC_MOCK_KRYO_ARM64_EDAC_H__
#define __SEC_QC_MOCK_KRYO_ARM64_EDAC_H__

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/notifier.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
/* implemented @ drivers/edac/kryo_arm64_edac.c */
extern ap_health_t *qcom_kryo_arm64_edac_error_register_notifier(struct notifier_block *nb);
extern int qcom_kryo_arm64_edac_error_unregister_notifier(struct notifier_block *nb);
#else
static inline ap_health_t *qcom_kryo_arm64_edac_error_register_notifier(struct notifier_block *nb) { return ERR_PTR(-ENODEV); }
static inline int qcom_kryo_arm64_edac_error_unregister_notifier(struct notifier_block *nb) { return -ENODEV; }
#endif

#endif /* __SEC_QC_MOCK_KRYO_ARM64_EDAC_H__ */
