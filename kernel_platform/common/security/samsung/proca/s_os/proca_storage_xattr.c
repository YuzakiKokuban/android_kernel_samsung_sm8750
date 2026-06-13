// SPDX-License-Identifier: GPL-2.0
/*
 * PROCA xattr operation
 *
 * Copyright (C) 2023 Samsung Electronics, Inc.
 * Oleksandr Stanislavskyi, <o.stanislavs@samsung.com>
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

#include <linux/file.h>
#include <linux/xattr.h>

#include "proca_porting.h"
#include "proca_storage.h"

#define XATTR_PA_SUFFIX "pa"
#define XATTR_NAME_PA (XATTR_USER_PREFIX XATTR_PA_SUFFIX)

static int read_xattr(struct dentry *dentry, const char *name,
			char **cert_buff)
{
	ssize_t ret;
	void *buffer = NULL;

	dentry = d_real_comp(dentry);
	*cert_buff = NULL;
	ret = __vfs_getxattr(dentry, dentry->d_inode, name,
				NULL, 0, XATTR_NOSECURITY);
	if (ret <= 0)
		return 0;

	buffer = kmalloc(ret + 1, GFP_NOFS);
	if (!buffer)
		return 0;

	ret = __vfs_getxattr(dentry, dentry->d_inode, name,
				buffer, ret + 1, XATTR_NOSECURITY);

	if (ret <= 0) {
		ret = 0;
		kfree(buffer);
	} else {
		*cert_buff = buffer;
	}

	return ret;
}

int proca_get_certificate(struct file *file, char **cert_buff)
{
	int ret = 0;

	ret = read_xattr(file->f_path.dentry, XATTR_NAME_PA, cert_buff);
	return ret;
}

bool proca_is_certificate_present(struct file *file)
{
	struct dentry *dentry;

	dentry = file->f_path.dentry;
	if (__vfs_getxattr(dentry, dentry->d_inode, XATTR_NAME_PA,
			NULL, 0, XATTR_NOSECURITY) > 0)
		return true;

	return false;
}

int init_proca_storage(void)
{
	return 0;
}
