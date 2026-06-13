#ifndef __SEC_BLK_HELPER_H__
#define __SEC_BLK_HELPER_H__

#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/device.h>

#if IS_ENABLED(CONFIG_SEC_BLK_HELPER)
extern int sec_devt_from_partuuid(const char *uuid_str, dev_t *devt);
extern ssize_t sec_blk_read(struct block_device *bdev, void *buf, size_t bytes, loff_t pos);
extern ssize_t sec_blk_write(struct block_device *bdev, const void *buf, size_t bytes, loff_t pos);
#else
static inline int sec_devt_from_partuuid(const char *uuid_str, dev_t *devt) { return -ENODEV; }
static inline ssize_t sec_blk_read(struct block_device *bdev, void *buf, size_t bytes, loff_t pos) { return -ENODEV; }
static inline ssize_t sec_blk_write(struct block_device *bdev, const void *buf, size_t bytes, loff_t pos) { return -ENODEV; }
#endif

#endif /* __SEC_BLK_HELPER_H__ */
