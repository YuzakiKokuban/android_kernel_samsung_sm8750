/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */

/*
 * fat_common interface:
 *
 * NOTE: This is a out of tree kernel module, so no one can really guarantee
 * that this header file will always exist. Therefore, it is recommended to
 * remove the build dependency by using its own macro as shown below.
 *
 * #ifdef CONFIG_FAT_COMMON
 * #include <linux/fat_common.h>
 * #else
 * #endif
 */

#ifndef _LINUX_FAT_COMMON_H
#define  _LINUX_FAT_COMMON_H

struct super_block;

#ifdef CONFIG_FS_RO_UEVENT
/* ro_uevent.c */
extern void fs_ro_uevent(struct super_block *sb, const char *prefix);
#else
#define fs_ro_uevent(sb, prefix)
#endif

#ifdef CONFIG_FS_COMMON_STLOG
__printf(3, 4) __cold
extern void fs_common_stlog(struct super_block *sb,
			    const char *prefix, const char *fmt, ...);

extern void fs_common_stlog_bs(struct super_block *sb, const char *prefix,
			       void *bs);
#else
#define fs_common_stlog(sb, prefix, fmt, args...)
#define fs_common_stlog_bs(sb, prefix, bs)
#endif

/* Helper function */
#define fat_stlog(sb, fmt, args...)	\
	fs_common_stlog(sb, "FAT-fs", fmt, ##args)

#define exfat_stlog(sb, fmt, args...)	\
	fs_common_stlog(sb, "exFAT-fs", fmt, ##args)

#define fat_stlog_bs(sb, bs)	\
	fs_common_stlog_bs(sb, "FAT-fs", bs)

#define exfat_stlog_bs(sb, bs)	\
	fs_common_stlog_bs(sb, "exFAT-fs", bs)

static inline void fat_uevent_ro_remount(struct super_block *sb)
{
	fs_ro_uevent(sb, "FAT-fs");
}

static inline void exfat_uevent_ro_remount(struct super_block *sb)
{
	fs_ro_uevent(sb, "exFAT-fs");
}

#endif /* _LINUX_FAT_COMMON_H */

