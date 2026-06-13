// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */

#include <linux/fat_common.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include "internal.h"

#ifdef CONFIG_PROC_STLOG
#include <linux/fslog.h>
#define ST_LOG(fmt, ...) fslog_stlog(fmt, ##__VA_ARGS__)
#else
#define ST_LOG(fmt, ...)
#endif

#ifdef CONFIG_FS_COMMON_STLOG
#define STLOG_PREFIX "%s (%s[%d:%d]): "

void fs_common_stlog(struct super_block *sb,
		     const char *prefix, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;

	ST_LOG(STLOG_PREFIX "%pV\n", prefix, sb->s_id,
	       MAJOR(sb->s_dev), MINOR(sb->s_dev), &vaf);
	va_end(args);
}
#endif

static int __init fat_common_init(void)
{
	int err;

	err = fs_ro_uevent_init();

	return err;
}

static void __exit fat_common_exit(void)
{
	fs_ro_uevent_exit();
}

fs_initcall(fat_common_init);
module_exit(fat_common_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("External filesystem common interface");
MODULE_AUTHOR("Samsung Electronics Co., Ltd.");
