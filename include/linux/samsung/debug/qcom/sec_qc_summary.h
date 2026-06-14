#ifndef __SEC_QC_SUMMARY_H__
#define __SEC_QC_SUMMARY_H__

#include <linux/err.h>
#include <linux/errno.h>

#include "sec_qc_summary_type.h"

#define SEC_DEBUG_SUMMARY_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SUMMARY_MAGIC1 0x05ECDEB6
#define SEC_DEBUG_SUMMARY_MAGIC2 0x14F014F0
/* high word : major version
 * low word : minor version
 * minor version changes should not affect the Boot Loader behavior
 */
#define SEC_DEBUG_SUMMARY_MAGIC3 0x00060000

#if IS_ENABLED(CONFIG_SEC_QC_SUMMARY)
extern struct sec_qc_summary_data_modem *sec_qc_summary_get_modem(void);
#else
static inline struct sec_qc_summary_data_modem *sec_qc_summary_get_modem(void) { return ERR_PTR(-ENODEV); }
#endif

#endif	/* __SEC_QC_SUMMARY_H__ */
