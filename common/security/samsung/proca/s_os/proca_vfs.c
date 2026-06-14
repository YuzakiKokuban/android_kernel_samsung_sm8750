// SPDX-License-Identifier: GPL-2.0
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

#include <linux/fs.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/uio.h>
#include <linux/fsnotify.h>
#include <linux/sched/xacct.h>

#include <linux/fs_struct.h>
#include <linux/mount.h>

#include "proca_vfs.h"

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
static ssize_t __proca_kernel_read(struct file *file, void *buf, size_t count, loff_t *pos)
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

/*
 * proca_kernel_read - read data from the file
 *
 * This is a function for reading file content instead of kernel_read().
 * It does not perform locking checks to ensure it cannot be blocked.
 * It does not perform security checks because it is irrelevant for IMA.
 *
 * This function is copied from integrity_kernel_read()
 */
int proca_kernel_read(struct file *file, loff_t offset,
			  void *addr, unsigned long count)
{
	return __proca_kernel_read(file, addr, count, &offset);
}

struct file *proca_kernel_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;

	filp = filp_open(path, flags, rights);
	return filp;
}

bool proca_path_is_mounted(const char *str_path)
{
	struct path path;
	int error;

	/* Get path struct for given path name */
	error = kern_path(str_path, LOOKUP_FOLLOW, &path);
	if (error)
		return false;

	/* A struct vfsmount describes a mount.
	 * Field 'mnt' represent the mount point if file is mounted.
	 */
	if (!(path.mnt)) {
		path_put(&path);
		return false;
	}

	path_put(&path);
	return true;
}

const char *proca_d_path(struct file *file, char **pathbuf, char *namebuf)
{
	const struct path *path = &file->f_path;
	char *pathname = NULL;

	*pathbuf = __getname();
	if (*pathbuf) {
		pathname = d_absolute_path(path, *pathbuf, PATH_MAX);
		if (IS_ERR(pathname)) {
			__putname(*pathbuf);
			*pathbuf = NULL;
			pathname = NULL;
		}
	}

	if (!pathname) {
		strscpy(namebuf, path->dentry->d_name.name, NAME_MAX);
		pathname = namebuf;
	}

	return pathname;
}
