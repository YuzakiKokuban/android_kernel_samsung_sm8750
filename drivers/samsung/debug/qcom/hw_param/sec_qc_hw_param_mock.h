#ifndef __INTERNAL__SEC_QC_HW_PARAM_MOCK_H__
#define __INTERNAL__SEC_QC_HW_PARAM_MOCK_H__

#if IS_ENABLED(CONFIG_QCOM_SOCINFO)
#include <soc/qcom/socinfo.h>
#else
static inline uint32_t socinfo_get_id(void) { return (uint32_t)(-ENODEV); }
#endif

#endif /* __INTERNAL__SEC_QC_HW_PARAM_MOCK_H__ */
