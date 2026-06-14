// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2024 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/blkdev.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/xarray.h>

#include <linux/samsung/sec_kunit.h>

static const inline struct class *__blk_class(void)
{
	struct gendisk *gendisk;
	const struct class *blk_cls;

	gendisk = blk_alloc_disk(NUMA_NO_NODE);
	if (!gendisk) {
		pr_err("blk_alloc_disk failed\n");
		return ERR_PTR(-ENOMEM);
	}

	blk_cls = disk_to_dev(gendisk)->class;

	put_disk(gendisk);

	return blk_cls;
}

__ss_static const struct class *blk_class(void)
{
	static const struct class *blk_cls;

	if (IS_ERR_OR_NULL(blk_cls))
		blk_cls = __blk_class();

	return blk_cls;
}

/* NOTE: these functions are inlined from 'block/genhd.c */
static dev_t __part_devt(struct gendisk *disk, u8 partno)
{
	struct block_device *part;
	dev_t devt = 0;

	rcu_read_lock();
	part = xa_load(&disk->part_tbl, partno);
	if (part)
		devt = part->bd_dev;
	rcu_read_unlock();

	return devt;
}

/* NOTE: these functions are inlined from 'block/early-lookup.c */
struct uuidcmp {
	const char *uuid;
	int len;
};

static int match_dev_by_uuid(struct device *dev, const void *data)
{
	struct block_device *bdev = dev_to_bdev(dev);
	const struct uuidcmp *cmp = data;

	if (!bdev->bd_meta_info ||
	    strncasecmp(cmp->uuid, bdev->bd_meta_info->uuid, cmp->len))
		return 0;
	return 1;
}

static int devt_from_partuuid(const char *uuid_str, dev_t *devt)
{
	struct uuidcmp cmp;
	struct device *dev = NULL;
	int offset = 0;
	char *slash;
#if IS_BUILTIN(CONFIG_SEC_BLK_HELPER)
	const struct class *blk_cls = &block_class;
#else
	const struct class *blk_cls = blk_class();
#endif

	cmp.uuid = uuid_str;

	slash = strchr(uuid_str, '/');
	/* Check for optional partition number offset attributes. */
	if (slash) {
		char c = 0;

		/* Explicitly fail on poor PARTUUID syntax. */
		if (sscanf(slash + 1, "PARTNROFF=%d%c", &offset, &c) != 1)
			goto out_invalid;
		cmp.len = slash - uuid_str;
	} else {
		cmp.len = strlen(uuid_str);
	}

	if (!cmp.len)
		goto out_invalid;

	dev = class_find_device(blk_cls, NULL, &cmp, &match_dev_by_uuid);
	if (!dev)
		return -ENODEV;

	if (offset) {
		/*
		 * Attempt to find the requested partition by adding an offset
		 * to the partition number found by UUID.
		 */
		*devt = __part_devt(dev_to_disk(dev),
				  dev_to_bdev(dev)->bd_partno + offset);
	} else {
		*devt = dev->devt;
	}

	put_device(dev);
	return 0;

out_invalid:
	pr_err("VFS: PARTUUID= is invalid.\n"
	       "Expected PARTUUID=<valid-uuid-id>[/PARTNROFF=%%d]\n");
	return -EINVAL;
}

int sec_devt_from_partuuid(const char *uuid_str, dev_t *devt)
{
	if (strncmp(uuid_str, "PARTUUID=", 9) == 0)
		return devt_from_partuuid(uuid_str + 9, devt);

	pr_warn("only PARTUUID= format is allowed.\n");

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(sec_devt_from_partuuid);

/* NOTE: see fs/pstore/blk.c of linux-5.10.y */
ssize_t sec_blk_read(struct block_device *bdev,
		void *buf, size_t bytes, loff_t pos)
{
	struct file file;
	struct kiocb kiocb;
	struct iov_iter iter;
	struct kvec iov = {.iov_base = buf, .iov_len = bytes};

	memset(&file, 0, sizeof(struct file));
	file.f_mapping = bdev->bd_inode->i_mapping;
	file.f_flags = O_DSYNC | __O_SYNC | O_NOATIME;
	file.f_inode = bdev->bd_inode;
	file_ra_state_init(&file.f_ra, file.f_mapping);

	init_sync_kiocb(&kiocb, &file);
	kiocb.ki_pos = pos;
	iov_iter_kvec(&iter, READ, &iov, 1, bytes);

	return generic_file_read_iter(&kiocb, &iter);
}
EXPORT_SYMBOL_GPL(sec_blk_read);

/* NOTE: this is a copy of 'blkdev_fsync' of 'block/fops.c' */
static int __blkdev_fsync(struct file *filp, loff_t start, loff_t end,
		int datasync)
{
	struct block_device *bdev = I_BDEV(filp->f_mapping->host);
	int error;

	error = file_write_and_wait_range(filp, start, end);
	if (error)
		return error;

	/*
	 * There is no need to serialise calls to blkdev_issue_flush with
	 * i_mutex and doing so causes performance issues with concurrent
	 * O_SYNC writers to a block device.
	 */
	error = blkdev_issue_flush(bdev);
	if (error == -EOPNOTSUPP)
		error = 0;

	return error;
}

/* NOTE: see fs/pstore/blk.c of linux-5.10.y */
ssize_t sec_blk_write(struct block_device *bdev,
		const void *buf, size_t bytes, loff_t pos)
{
	struct iov_iter iter;
	struct kiocb kiocb;
	struct file file;
	ssize_t ret;
	struct kvec iov = {.iov_base = (void *)buf, .iov_len = bytes};

	/* Console/Ftrace backend may handle buffer until flush dirty zones */
	if (in_interrupt() || irqs_disabled())
		return -EBUSY;

	memset(&file, 0, sizeof(struct file));
	file.private_data = bdev;
	file.f_mapping = bdev->bd_inode->i_mapping;
	file.f_flags = O_DSYNC | __O_SYNC | O_NOATIME;
	file.f_inode = bdev->bd_inode;
	file.f_iocb_flags = iocb_flags(&file);

	init_sync_kiocb(&kiocb, &file);
	kiocb.ki_pos = pos;
	iov_iter_kvec(&iter, WRITE, &iov, 1, bytes);

	inode_lock(bdev->bd_inode);
	ret = generic_write_checks(&kiocb, &iter);
	if (ret > 0)
		ret = generic_perform_write(&kiocb, &iter);
	inode_unlock(bdev->bd_inode);

	if (likely(ret > 0)) {
		const struct file_operations f_op = {
			.fsync = __blkdev_fsync,
		};

		file.f_op = &f_op;
		kiocb.ki_pos += ret;
		ret = generic_write_sync(&kiocb, ret);
	}
	sync_blockdev(bdev);
	return ret;
}
EXPORT_SYMBOL_GPL(sec_blk_write);

static int __init sec_blk_helper_init(void)
{
	const struct class *blk_cls;

	if (IS_BUILTIN(CONFIG_SEC_BLK_HELPER))
		return 0;

	blk_cls = blk_class();
	if (IS_ERR_OR_NULL(blk_cls))
		return -EBUSY;

	return 0;
}
module_init(sec_blk_helper_init);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Samsung, Block-Device Helper Driver");
MODULE_LICENSE("GPL v2");
