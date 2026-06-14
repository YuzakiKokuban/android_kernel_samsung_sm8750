#ifndef __SEC_QC_DBG_PARTITION_H__
#define __SEC_QC_DBG_PARTITION_H__

#include "sec_qc_dbg_partition_type.h"

#if IS_ENABLED(CONFIG_SEC_QC_DEBUG_PARTITION)
extern ssize_t sec_qc_dbg_part_get_size(size_t index);
extern bool sec_qc_dbg_part_read(size_t index, void *value);
extern bool sec_qc_dbg_part_write(size_t index, const void *value);
#else
static inline ssize_t sec_qc_dbg_part_get_size(size_t index) { return -ENODEV; }
static inline bool sec_qc_dbg_part_read(size_t index, void *value) { return false; }
static inline bool sec_qc_dbg_part_write(size_t index, const void *value) { return false; }
#endif

#endif /* __SEC_QC_DBG_PARTITION_H__ */
