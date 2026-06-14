#ifndef __SEC_QC_HW_PARAM_H__
#define __SEC_QC_HW_PARAM_H__

#if IS_ENABLED(CONFIG_SEC_QC_HW_PARAM)
extern void battery_last_dcvs(int cap, int volt, int temp, int curr);
#else
static inline void battery_last_dcvs(int cap, int volt, int temp, int curr) {}
#endif

#endif /* __SEC_QC_HW_PARAM_H__ */
