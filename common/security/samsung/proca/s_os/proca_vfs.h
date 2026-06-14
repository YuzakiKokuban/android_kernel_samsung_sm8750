/* SPDX-License-Identifier: GPL-2.0 */
/*
 * PROCA vfs functions
 *
 * Copyright (C) 2020 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LINUX_PROCA_VFS_H
#define _LINUX_PROCA_VFS_H

struct file *proca_kernel_open(const char *path, int flags, int rights);
int proca_kernel_read(struct file *file, loff_t offset,
			  void *addr, unsigned long count);
bool proca_path_is_mounted(const char *path);
const char *proca_d_path(struct file *file, char **pathbuf, char *namebuf);

#endif /* _LINUX_PROCA_VFS_H */
