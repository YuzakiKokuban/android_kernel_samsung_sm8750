// SPDX-License-Identifier: GPL-2.0
/*
 *  Samsung specific module in block layer
 *
 *  Copyright (C) 2021 Manjong Lee <mj0123.lee@samsung.com>
 *  Copyright (C) 2021 Junho Kim <junho89.kim@samsung.com>
 *  Copyright (C) 2021 Changheun Lee <nanich.lee@samsung.com>
 *  Copyright (C) 2021 Seunghwan Hyun <seunghwan.hyun@samsung.com>
 *  Copyright (C) 2021 Tran Xuan Nam <nam.tx2@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/part_stat.h>

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#else
static struct class *blk_sec_class;
#endif

#include "blk-sec.h"
#include "../drivers/ufs/host/ufs-sec-feature.h"

struct disk_info {
	/* fields related with target device itself */
	struct gendisk *gd;
	struct request_queue *queue;
};

struct device *blk_sec_dev;
EXPORT_SYMBOL(blk_sec_dev);

struct workqueue_struct *blk_sec_common_wq;
EXPORT_SYMBOL(blk_sec_common_wq);

static struct disk_info internal_disk;
static unsigned int internal_min_size_mb = 10 * 1024; /* 10GB */

static char manual_hcgc_status[32] = "off";

#define SECTORS2MB(x) ((x) / 2 / 1024)

#define SCSI_DISK0_MAJOR 8
#define MMC_BLOCK_MAJOR 179
#define MAJOR8_DEV_NUM 16	/* maximum number of minor devices in scsi disk0 */
#define SCSI_MINORS 16		/* first minor number of scsi disk0 */
#define MMC_TARGET_DEV 16	/* number of mmc devices set of target (maximum 256) */
#define MMC_MINORS 8		/* first minor number of mmc disk */

static bool is_internal_bdev(struct block_device *dev)
{
	int size_mb;

	if (bdev_is_partition(dev))
		return false;

	if (dev->bd_disk->flags & GENHD_FL_REMOVABLE)
		return false;

	size_mb = SECTORS2MB(get_capacity(dev->bd_disk));
	if (size_mb >= internal_min_size_mb)
		return true;

	return false;
}

static struct gendisk *find_internal_disk(void)
{
	struct block_device *bdev;
	struct gendisk *gd = NULL;
	int idx;
	dev_t devno = MKDEV(0, 0);

	for (idx = 0; idx < MAJOR8_DEV_NUM; idx++) {
		devno = MKDEV(SCSI_DISK0_MAJOR, SCSI_MINORS * idx);
		bdev = blkdev_get_by_dev(devno, BLK_OPEN_READ, NULL, NULL);
		if (IS_ERR(bdev))
			continue;

		if (bdev->bd_disk && is_internal_bdev(bdev))
			gd = bdev->bd_disk;

		blkdev_put(bdev, NULL);

		if (gd)
			return gd;
	}

	for (idx = 0; idx < MMC_TARGET_DEV; idx++) {
		devno = MKDEV(MMC_BLOCK_MAJOR, MMC_MINORS * idx);
		bdev = blkdev_get_by_dev(devno, BLK_OPEN_READ, NULL, NULL);
		if (IS_ERR(bdev))
			continue;

		if (bdev->bd_disk && is_internal_bdev(bdev))
			gd = bdev->bd_disk;

		blkdev_put(bdev, NULL);

		if (gd)
			return gd;
	}

	return NULL;
}

static inline int init_internal_disk_info(void)
{
	if (!internal_disk.gd) {
		internal_disk.gd = find_internal_disk();
		if (unlikely(!internal_disk.gd)) {
			pr_err("%s: can't find internal disk\n", __func__);
			return -ENODEV;
		}
		internal_disk.queue = internal_disk.gd->queue;
	}

	return 0;
}

static inline void clear_internal_disk_info(void)
{
	internal_disk.gd = NULL;
	internal_disk.queue = NULL;
}

struct gendisk *blk_sec_internal_disk(void)
{
	if (unlikely(!internal_disk.gd))
		init_internal_disk_info();

	return internal_disk.gd;
}
EXPORT_SYMBOL(blk_sec_internal_disk);

static int blk_sec_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	return add_uevent_var(env, "DEVNAME=%s", dev->kobj.name);
}

static struct device_type blk_sec_type = {
	.uevent = blk_sec_uevent,
};

static ssize_t manual_hcgc_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", manual_hcgc_status);
}

static ssize_t manual_hcgc_store(struct kobject *kobj, struct kobj_attribute *attr,
		const char *buf, size_t count)
{
#define BUF_SIZE 32
	char hcgc_str[BUF_SIZE];
	char *envp[] = { "NAME=HCGC_BKL_SEC", hcgc_str, NULL, };

	if (!ufs_sec_is_hcgc_allowed())
		return -EOPNOTSUPP;

	if (strncmp(buf, "on", 2) && strncmp(buf, "off", 3) &&
			strncmp(buf, "done", 4) && strncmp(buf, "disable", 7) && strncmp(buf, "enable", 6))
		return -EINVAL;

	if (!strncmp(manual_hcgc_status, "disable", 7) && strncmp(buf, "enable", 6))
		return -EINVAL;

	memset(manual_hcgc_status, 0, BUF_SIZE);

	if (!strncmp(buf, "done", 4)) {
		strncpy(manual_hcgc_status, buf, BUF_SIZE - 1);
		return count;
	}

	snprintf(hcgc_str, BUF_SIZE, "MANUAL_HCGC=%s", buf);
	kobject_uevent_env(&blk_sec_dev->kobj, KOBJ_CHANGE, envp);

	if (!strncmp(buf, "enable", 6))
		strncpy(manual_hcgc_status, "off", BUF_SIZE - 1);
	else
		strncpy(manual_hcgc_status, buf, BUF_SIZE - 1);

	return count;
}

static struct kobj_attribute manual_hcgc_attr = __ATTR(manual_hcgc, 0600, manual_hcgc_show, manual_hcgc_store);

static const struct attribute *blk_sec_attrs[] = {
	&manual_hcgc_attr.attr,
	NULL,
};

static struct kobject *blk_sec_kobj;

static int __init blk_sec_common_init(void)
{
	int retval;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	blk_sec_dev = sec_device_create(NULL, "blk_sec");
	if (IS_ERR(blk_sec_dev)) {
		pr_err("%s: Failed to create blk_sec device\n", __func__);
		return PTR_ERR(blk_sec_dev);
	}
#else
	blk_sec_class = class_create("blk_sec");
	if (IS_ERR(blk_sec_class)) {
		pr_err("%s: couldn't create blk_sec class\n", __func__);
		return PTR_ERR(blk_sec_class);
	}

	blk_sec_dev = device_create(blk_sec_class, NULL, MKDEV(0, 0), NULL, "blk_sec");
	if (IS_ERR(blk_sec_dev)) {
		pr_err("%s: Failed to create blk_sec device\n", __func__);
		class_destroy(blk_sec_class);
		return PTR_ERR(blk_sec_dev);
	}
#endif

	blk_sec_dev->type = &blk_sec_type;

	blk_sec_kobj = kobject_create_and_add("blk_sec", kernel_kobj);
	if (!blk_sec_kobj)
		goto destroy_device;
	if (sysfs_create_files(blk_sec_kobj, blk_sec_attrs)) {
		kobject_put(blk_sec_kobj);
		goto destroy_device;
	}

	blk_sec_common_wq = create_freezable_workqueue("blk_sec_common");

	retval = init_internal_disk_info();
	if (retval) {
		clear_internal_disk_info();
		pr_err("%s: Can't find internal disk info!", __func__);
	}

	return 0;

destroy_device:
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sec_device_destroy(blk_sec_dev->devt);
#else
	device_destroy(blk_sec_class, MKDEV(0, 0));
	class_destroy(blk_sec_class);
#endif
	return -ENOMEM;
}

static void __exit blk_sec_common_exit(void)
{
#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	sec_device_destroy(blk_sec_dev->devt);
#else
	device_destroy(blk_sec_class, MKDEV(0, 0));
	class_destroy(blk_sec_class);
#endif
	sysfs_remove_files(blk_sec_kobj, blk_sec_attrs);
	kobject_put(blk_sec_kobj);

	clear_internal_disk_info();
}

module_init(blk_sec_common_init);
module_exit(blk_sec_common_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Changheun Lee <nanich.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung specific module in block layer");
MODULE_VERSION("1.0");
