#ifndef __SEC_QC_MOCK_SMEM_H__
#define __SEC_QC_MOCK_SMEM_H__

#include <linux/err.h>
#include <linux/errno.h>

#ifndef QCOM_SMEM_HOST_ANY
#define QCOM_SMEM_HOST_ANY -1
#endif

#if IS_ENABLED(CONFIG_SEC_QC_MOCK)
/* implemented @ drivers/soc/qcom/smsm.c */
extern int qcom_smem_alloc(unsigned host, unsigned item, size_t size);
extern void *qcom_smem_get(unsigned host, unsigned item, size_t *size);
extern phys_addr_t qcom_smem_virt_to_phys(void *p);
#else
static inline int qcom_smem_alloc(unsigned host, unsigned item, size_t size) { return -ENODEV; }
static inline void *qcom_smem_get(unsigned host, unsigned item, size_t *size) { return ERR_PTR(-ENODEV); }
static inline phys_addr_t qcom_smem_virt_to_phys(void *p) { return 0; }
#endif /* CONFIG_SEC_QC_MOCK */

#endif /* __SEC_QC_MOCK_SMEM_H__ */
