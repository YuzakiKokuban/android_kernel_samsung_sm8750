#ifndef __SEC_QC_MOCK_DEBUG_SYMBOL_H__
#define __SEC_QC_MOCK_DEBUG_SYMBOL_H__

/* implemented @ drivers/soc/qcom/debug_symbol.c */
#if IS_ENABLED(CONFIG_QCOM_DEBUG_SYMBOL)
#include <debug_symbol.h>
#else
static inline bool debug_symbol_available(void) { return true; }
#endif

#endif /* __SEC_QC_MOCK_DEBUG_SYMBOL_H__ */
