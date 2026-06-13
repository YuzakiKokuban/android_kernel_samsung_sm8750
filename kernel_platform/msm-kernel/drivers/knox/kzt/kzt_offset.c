// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *		http://www.samsung.com
 *
 */
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/mm_types.h>
#include <linux/path.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#include <linux/time64.h>
#include <net/sock.h>

#include "kzt_main.h"
#include "kzt_common.h"

static int __kzt_offset_task_struct_offsets(u32 *offsets);
static int __kzt_offset_cred_offsets(u32 *offsets);
static int __kzt_offset_fs_struct_offsets(u32 *offsets);
static int __kzt_offset_mm_struct_offsets(u32 *offsets);
static int __kzt_offset_file_offsets(u32 *offsets);
static int __kzt_offset_path_offsets(u32 *offsets);
static int __kzt_offset_dentry_offsets(u32 *offsets);
static int __kzt_offset_qstr_offsets(u32 *offsets);
static int __kzt_offset_inode_offsets(u32 *offsets);
static int __kzt_offset_timespec64_offsets(u32 *offsets);
static int __kzt_offset_sock_offsets(u32 *offsets);

static int kzt_offset_task_struct_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_task_struct_offsets(arg->task_struct_.offsets);

	if (!err)
		arg->version = VERSION_OF_TASK_STRUCT_INTERESTS;

	return err;
}

static int __kzt_offset_task_struct_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[MM_OF_TASK_STRUCT] = offsetof(struct task_struct, mm);
		if (offsets[MM_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[EXIT_CODE_OF_TASK_STRUCT] = offsetof(struct task_struct, exit_code);
		if (offsets[EXIT_CODE_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[PID_OF_TASK_STRUCT] = offsetof(struct task_struct, pid);
		if (offsets[PID_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[TGID_OF_TASK_STRUCT] = offsetof(struct task_struct, tgid);
		if (offsets[TGID_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[REAL_PARENT_OF_TASK_STRUCT] = offsetof(struct task_struct, real_parent);
		if (offsets[REAL_PARENT_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[UTIME_OF_TASK_STRUCT] = offsetof(struct task_struct, utime);
		if (offsets[UTIME_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[STIME_OF_TASK_STRUCT] = offsetof(struct task_struct, stime);
		if (offsets[STIME_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[START_TIME_OF_TASK_STRUCT] = offsetof(struct task_struct, start_time);
		if (offsets[START_TIME_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[CRED_OF_TASK_STRUCT] = offsetof(struct task_struct, cred);
		if (offsets[CRED_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

		offsets[FS_OF_TASK_STRUCT] = offsetof(struct task_struct, fs);
		if (offsets[FS_OF_TASK_STRUCT] >= sizeof(struct task_struct))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_cred_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_cred_offsets(arg->cred_.offsets);

	if (!err)
		arg->version = VERSION_OF_CRED_INTERESTS;

	return err;
}

static int __kzt_offset_cred_offsets(u32 *offsets)
{

	int err = -EFAULT;

	do {

		offsets[UID_OF_CRED] = offsetof(struct cred, uid);
		if (offsets[UID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[GID_OF_CRED] = offsetof(struct cred, gid);
		if (offsets[GID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[SUID_OF_CRED] = offsetof(struct cred, suid);
		if (offsets[SUID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[SGID_OF_CRED] = offsetof(struct cred, sgid);
		if (offsets[SGID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[EUID_OF_CRED] = offsetof(struct cred, euid);
		if (offsets[EUID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[EGID_OF_CRED] = offsetof(struct cred, egid);
		if (offsets[EGID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[FSUID_OF_CRED] = offsetof(struct cred, fsuid);
		if (offsets[FSUID_OF_CRED] >= sizeof(struct cred))
			break;

		offsets[FSGID_OF_CRED] = offsetof(struct cred, fsgid);
		if (offsets[FSGID_OF_CRED] >= sizeof(struct cred))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_fs_struct_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_fs_struct_offsets(arg->fs_struct_.offsets);

	if (!err)
		arg->version = VERSION_OF_FS_STRUCT_INTERESTS;

	return err;
}

static int __kzt_offset_fs_struct_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[PWD_OF_FS_STRUCT] = offsetof(struct fs_struct, pwd);
		if (offsets[PWD_OF_FS_STRUCT] >= sizeof(struct fs_struct))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_mm_struct_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_mm_struct_offsets(arg->mm_struct_.offsets);

	if (!err)
		arg->version = VERSION_OF_MM_STRUCT_INTERESTS;

	return err;
}

static int __kzt_offset_mm_struct_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[EXE_FILE_OF_MM_STRUCT] = offsetof(struct mm_struct, exe_file);
		if (offsets[EXE_FILE_OF_MM_STRUCT] >= sizeof(struct mm_struct))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_file_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_file_offsets(arg->file_.offsets);

	if (!err)
		arg->version = VERSION_OF_FILE_INTERESTS;

	return err;
}

static int __kzt_offset_file_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[F_PATH_OF_FILE] = offsetof(struct file, f_path);
		if (offsets[F_PATH_OF_FILE] >= sizeof(struct file))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_path_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_path_offsets(arg->path_.offsets);

	if (!err)
		arg->version = VERSION_OF_PATH_INTERESTS;

	return err;
}

static int __kzt_offset_path_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[DENTRY_OF_PATH] = offsetof(struct path, dentry);
		if (offsets[DENTRY_OF_PATH] >= sizeof(struct path))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_dentry_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_dentry_offsets(arg->dentry_.offsets);

	if (!err)
		arg->version = VERSION_OF_DENTRY_INTERESTS;

	return err;
}

static int __kzt_offset_dentry_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[D_PARENT_OF_DENTRY] = offsetof(struct dentry, d_parent);
		if (offsets[D_PARENT_OF_DENTRY] >= sizeof(struct dentry))
			break;

		offsets[D_NAME_OF_DENTRY] = offsetof(struct dentry, d_name);
		if (offsets[D_NAME_OF_DENTRY] >= sizeof(struct dentry))
			break;

		offsets[D_INODE_OF_DENTRY] = offsetof(struct dentry, d_inode);
		if (offsets[D_INODE_OF_DENTRY] >= sizeof(struct dentry))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_qstr_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_qstr_offsets(arg->qstr_.offsets);

	if (!err)
		arg->version = VERSION_OF_QSTR_INTERESTS;

	return err;
}

static int __kzt_offset_qstr_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[NAME_OF_QSTR] = offsetof(struct qstr, name);
		if (offsets[NAME_OF_QSTR] >= sizeof(struct qstr))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_inode_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_inode_offsets(arg->inode_.offsets);

	if (!err)
		arg->version = VERSION_OF_INODE_INTERESTS;

	return err;
}

static int __kzt_offset_inode_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[I_MODE_OF_INODE] = offsetof(struct inode, i_mode);
		if (offsets[I_MODE_OF_INODE] >= sizeof(struct inode))
			break;

		offsets[I_UID_OF_INODE] = offsetof(struct inode, i_uid);
		if (offsets[I_UID_OF_INODE] >= sizeof(struct inode))
			break;

		offsets[I_GID_OF_INODE] = offsetof(struct inode, i_gid);
		if (offsets[I_GID_OF_INODE] >= sizeof(struct inode))
			break;

		offsets[I_INO_OF_INODE] = offsetof(struct inode, i_ino);
		if (offsets[I_INO_OF_INODE] >= sizeof(struct inode))
			break;

		offsets[I_ATIME_OF_INODE] = offsetof(struct inode, i_atime);
		if (offsets[I_ATIME_OF_INODE] >= sizeof(struct inode))
			break;

		offsets[I_MTIME_OF_INODE] = offsetof(struct inode, i_mtime);
		if (offsets[I_MTIME_OF_INODE] >= sizeof(struct inode))
			break;

		offsets[I_CTIME_OF_INODE] = offsetof(struct inode, __i_ctime);
		if (offsets[I_CTIME_OF_INODE] >= sizeof(struct inode))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_timespec64_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_timespec64_offsets(arg->timespec64_.offsets);

	if (!err)
		arg->version = VERSION_OF_TIMESPEC64_INTERESTS;

	return err;
}

static int __kzt_offset_timespec64_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[TV_SEC_OF_TIMESPEC64] = offsetof(struct timespec64, tv_sec);
		if (offsets[TV_SEC_OF_TIMESPEC64] >= sizeof(struct timespec64))
			break;

	} while ((err = 0));

	return err;
}

static int kzt_offset_sock_offsets(struct kzt_get_offsets_arg *arg)
{
	int err = __kzt_offset_sock_offsets(arg->sock_.offsets);

	if (!err)
		arg->version = VERSION_OF_SOCK_INTERESTS;

	return err;
}

static int __kzt_offset_sock_offsets(u32 *offsets)
{
	int err = -EFAULT;

	do {

		offsets[SK_TYPE_OF_SOCK] = offsetof(struct sock, sk_type);
		if (offsets[SK_TYPE_OF_SOCK] >= sizeof(struct sock))
			break;

		offsets[SK_UID_OF_SOCK] = offsetof(struct sock, sk_uid);
		if (offsets[SK_UID_OF_SOCK] >= sizeof(struct sock))
			break;

	} while ((err = 0));

	return err;
}

int kzt_offset_get_offsets(struct kzt_get_offsets_arg *arg)
{
	switch (arg->type) {
	case STRUCT_TYPE_TASK_STRUCT:
		return kzt_offset_task_struct_offsets(arg);
	case STRUCT_TYPE_CRED:
		return kzt_offset_cred_offsets(arg);
	case STRUCT_TYPE_FS_STRUCT:
		return kzt_offset_fs_struct_offsets(arg);
	case STRUCT_TYPE_MM_STRUCT:
		return kzt_offset_mm_struct_offsets(arg);
	case STRUCT_TYPE_FILE:
		return kzt_offset_file_offsets(arg);
	case STRUCT_TYPE_PATH:
		return kzt_offset_path_offsets(arg);
	case STRUCT_TYPE_DENTRY:
		return kzt_offset_dentry_offsets(arg);
	case STRUCT_TYPE_QSTR:
		return kzt_offset_qstr_offsets(arg);
	case STRUCT_TYPE_INODE:
		return kzt_offset_inode_offsets(arg);
	case STRUCT_TYPE_TIMESPEC64:
		return kzt_offset_timespec64_offsets(arg);
	case STRUCT_TYPE_SOCK:
		return kzt_offset_sock_offsets(arg);
	case STRUCT_TYPE_NONE:
	case STRUCT_TYPE_NONE_END:
		return -EPERM;
	}
}
