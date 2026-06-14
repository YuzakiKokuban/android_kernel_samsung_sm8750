/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 */

/*
 * fat_common internal interface:
 */

#ifndef _LINUX_FAT_COMMON_INTERNAL_H
#define  _LINUX_FAT_COMMON_INTERNAL_H

struct super_block;

/* ro_uevent.c */
int fs_ro_uevent_init(void);
void fs_ro_uevent_exit(void);

#endif /* _LINUX_FAT_COMMON_INTERNAL_H */

