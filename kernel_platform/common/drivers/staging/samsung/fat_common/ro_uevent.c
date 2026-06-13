// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */

#include <linux/fat_common.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>

#ifdef CONFIG_PROC_STLOG
#include <linux/fslog.h>
#define ST_LOG(fmt, ...) fslog_stlog(fmt, ##__VA_ARGS__)
#else
#define ST_LOG(fmt, ...)
#endif

#define STLOG_PREFIX "%s (%s[%d:%d]): "

/* /sys/fs/fat/uevent is used for all external filesystems */
static struct kset *fat_kset;
static struct kobject fs_uevent_kobj;

void fs_ro_uevent(struct super_block *sb, const char *prefix)
{
	dev_t bd_dev = sb->s_dev;

	char major[16], minor[16];
	char *envp[] = { major, minor, NULL };

	snprintf(major, sizeof(major), "MAJOR=%d", MAJOR(bd_dev));
	snprintf(minor, sizeof(minor), "MINOR=%d", MINOR(bd_dev));

	kobject_uevent_env(&fs_uevent_kobj, KOBJ_CHANGE, envp);

	ST_LOG(STLOG_PREFIX "Filesystem has been set read-only (uevent triggered)\n",
	       prefix, sb->s_id, MAJOR(bd_dev), MINOR(bd_dev));
}

static int __init fs_uevent_init(struct kset *fat_kset)
{
	const struct kobj_type *ktype = get_ktype(&fat_kset->kobj);
	int err;

	fs_uevent_kobj.kset = fat_kset;
	err = kobject_init_and_add(&fs_uevent_kobj, ktype, NULL, "uevent");
	if (err)
		fs_uevent_kobj.kset = NULL;
	return err;
}

static void __exit fs_uevent_uninit(void)
{
	if (fs_uevent_kobj.kset)
		kobject_del(&fs_uevent_kobj);

	memset(&fs_uevent_kobj, 0, sizeof(struct kobject));
}

int __init fs_ro_uevent_init(void)
{
	int err;

	fat_kset = kset_create_and_add("fat", NULL, fs_kobj);
	if (!fat_kset) {
		pr_err("Failed to create fat_kset for fs_ro_uevent\n");
		return -ENOMEM;
	}

	err = fs_uevent_init(fat_kset);
	if (err) {
		pr_err("Unable to create fs-ro-uevent kobj\n");
		kset_unregister(fat_kset);
		fat_kset = NULL;
		return err;
	}

	return 0;
}

void __exit fs_ro_uevent_exit(void)
{
	fs_uevent_uninit();

	if (fat_kset) {
		kset_unregister(fat_kset);
		fat_kset = NULL;
	}
}
