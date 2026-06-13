#ifndef __SEC_PARAM_H__
#define __SEC_PARAM_H__

/* TODO: currently, we use a different defenition for param index for each
 * SoCs.
 * This should be improved in future.
 */
#include "qcom/sec_qc_param.h"

typedef bool (*sec_param_read_t)(size_t, void *);
typedef bool (*sec_param_write_t)(size_t, const void *);

struct sec_param_operations {
	sec_param_read_t read;
	sec_param_write_t write;
};

#if IS_ENABLED(CONFIG_SEC_PARAM)
extern bool sec_param_get(size_t index, void *value);
extern bool sec_param_set(size_t index, const void *value);
extern int sec_param_register_operations(struct sec_param_operations *ops);
extern void sec_param_unregister_operations(struct sec_param_operations *ops);
#else
static inline bool sec_param_get(size_t index, void *value) { return false; }
static inline bool sec_param_set(size_t index, const void *value) { return false; }
static inline int sec_param_register_operations(struct sec_param_operations *ops) { return 0; }
static inline void sec_param_unregister_operations(struct sec_param_operations *ops) {}
#endif

#endif /* __SEC_PARAM_H__ */
