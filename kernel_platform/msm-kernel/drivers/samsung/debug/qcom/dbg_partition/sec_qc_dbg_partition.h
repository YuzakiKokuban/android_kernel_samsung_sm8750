#ifndef __INTERNAL__SEC_QC_DBG_PARTITION_H__
#define __INTERNAL__SEC_QC_DBG_PARTITION_H__

#include <linux/blkdev.h>
#include <linux/debugfs.h>

#include <linux/samsung/builder_pattern.h>

struct qc_dbg_part_drvdata {
	struct builder bd;
	const char *bdev_path;
	struct block_device *bdev;
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *dbgfs;
#endif
};

#define QC_DBG_PART_INFO(__index, __offset, __size, __flags) \
	[__index] = { \
		.name = #__index, \
		.offset = __offset, \
		.size = __size, \
		.flags = __flags, \
	}

struct qc_dbg_part_info {
	const char *name;
	loff_t offset;
	size_t size;
	int flags;
};

#define DEBUG_PART_OFFSET_FROM_DT SEC_DEBUG_PARTITION_SIZE
#define DEBUG_PART_SIZE_FROM_DT SEC_DEBUG_PARTITION_SIZE

#endif /* __INTERNAL__SEC_QC_DBG_PARTITION_H__ */
