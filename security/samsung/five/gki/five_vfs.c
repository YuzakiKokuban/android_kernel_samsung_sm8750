// SPDX-License-Identifier: GPL-2.0
/*
 * FIVE vfs functions
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

#include <linux/fs.h>
#include <linux/xattr.h>
#include <linux/uio.h>
#include <linux/fsnotify.h>
#include <linux/sched/xacct.h>
#include "five_vfs.h"
#include "five_log.h"
#include "five_porting.h"

/* This function is an alternative implementation of vfs_getxattr_alloc() */
ssize_t five_getxattr_alloc(struct dentry *dentry, const char *name,
			    char **xattr_value, size_t xattr_size, gfp_t flags)
{
	struct inode *inode = dentry->d_inode;
	char *value = *xattr_value;
	ssize_t error;

	error = __vfs_getxattr(dentry, inode, name, NULL,
							 0, XATTR_NOSECURITY);
	if (error < 0)
		return error;

	if (!value || (error > xattr_size)) {
		value = krealloc(*xattr_value, error + 1, flags);
		if (!value)
			return -ENOMEM;
		memset(value, 0, error + 1);
	}

	error = __vfs_getxattr(dentry, inode, name, value,
						error, XATTR_NOSECURITY);
	*xattr_value = value;
	return error;
}

/* This function is copied from __vfs_setxattr_noperm() */
int five_setxattr_noperm(struct dentry *dentry, const char *name,
			 const void *value, size_t size, int flags)
{
	struct inode *inode = dentry->d_inode;
	int error = -EAGAIN;
	int issec = !strncmp(name, XATTR_SECURITY_PREFIX,
			     XATTR_SECURITY_PREFIX_LEN);

	if (issec)
		inode->i_flags &= ~S_NOSEC;
	if (inode->i_opflags & IOP_XATTR) {
		error = __vfs_setxattr(dentry, inode, name, value, size, flags);
	} else {
		if (unlikely(is_bad_inode(inode)))
			return -EIO;
	}

	return error;
}

static int warn_unsupported(struct file *file, const char *op)
{
	pr_warn_ratelimited(
		"kernel %s not supported for file %pD4 (pid: %d comm: %.20s)\n",
		op, file, current->pid, current->comm);
	return -EINVAL;
}

/*
 * This function is copied from __kernel_read()
 */
static ssize_t __five_kernel_read(struct file *file, void *buf,
						size_t count, loff_t *pos)
{
	struct kvec iov = {
		.iov_base    = buf,
		.iov_len    = min_t(size_t, count, MAX_RW_COUNT),
	};
	struct kiocb kiocb;
	struct iov_iter iter;
	ssize_t ret;

	if (WARN_ON_ONCE(!(file->f_mode & FMODE_READ)))
		return -EINVAL;
	if (!(file->f_mode & FMODE_CAN_READ))
		return -EINVAL;
	/*
	 * Also fail if ->read_iter and ->read are both wired up as that
	 * implies very convoluted semantics.
	 */
	if (unlikely(!file->f_op->read_iter || file->f_op->read))
		return warn_unsupported(file, "read");

	init_sync_kiocb(&kiocb, file);
	kiocb.ki_pos = pos ? *pos : 0;
	iov_iter_kvec(&iter, READ, &iov, 1, iov.iov_len);
	ret = file->f_op->read_iter(&kiocb, &iter);
	if (ret > 0) {
		if (pos)
			*pos = kiocb.ki_pos;
		fsnotify_access(file);
		add_rchar(current, ret);
	}
	inc_syscr(current);
	return ret;
}

int five_kernel_read(struct file *file, loff_t offset,
			  void *addr, unsigned long count)
{
	return __five_kernel_read(file, addr, count, &offset);
}
