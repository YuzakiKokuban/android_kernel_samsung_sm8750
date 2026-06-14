/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *		http://www.samsung.com
 *
 */
#ifndef KZT_COMMON_H
#define KZT_COMMON_H

#include <linux/ioctl.h>
#include <linux/types.h>

#if defined(__ANDROID__)
typedef unsigned int u32;
#endif

#define KZT_KERNEL_VERSION 1

enum kzt_struct_type {
	STRUCT_TYPE_NONE,
	STRUCT_TYPE_TASK_STRUCT,
	STRUCT_TYPE_CRED,
	STRUCT_TYPE_FS_STRUCT,
	STRUCT_TYPE_MM_STRUCT,
	STRUCT_TYPE_FILE,
	STRUCT_TYPE_PATH,
	STRUCT_TYPE_DENTRY,
	STRUCT_TYPE_QSTR,
	STRUCT_TYPE_INODE,
	STRUCT_TYPE_TIMESPEC64,
	STRUCT_TYPE_SOCK,
	STRUCT_TYPE_NONE_END,
};

enum kzt_task_struct_interests {
	MM_OF_TASK_STRUCT,
	EXIT_CODE_OF_TASK_STRUCT,
	PID_OF_TASK_STRUCT,
	TGID_OF_TASK_STRUCT,
	REAL_PARENT_OF_TASK_STRUCT,
	UTIME_OF_TASK_STRUCT,
	STIME_OF_TASK_STRUCT,
	START_TIME_OF_TASK_STRUCT,
	CRED_OF_TASK_STRUCT,
	FS_OF_TASK_STRUCT,
	NUM_OF_TASK_STRUCT_INTERESTS,
	VERSION_OF_TASK_STRUCT_INTERESTS = 1,
};

enum kzt_cred_interests {
	UID_OF_CRED,
	GID_OF_CRED,
	SUID_OF_CRED,
	SGID_OF_CRED,
	EUID_OF_CRED,
	EGID_OF_CRED,
	FSUID_OF_CRED,
	FSGID_OF_CRED,
	NUM_OF_CRED_INTERESTS,
	VERSION_OF_CRED_INTERESTS = 1,
};

enum kzt_fs_struct_interests {
	PWD_OF_FS_STRUCT,
	NUM_OF_FS_STRUCT_INTERESTS,
	VERSION_OF_FS_STRUCT_INTERESTS = 1,
};

enum kzt_mm_struct_interests {
	EXE_FILE_OF_MM_STRUCT,
	NUM_OF_MM_STRUCT_INTERESTS,
	VERSION_OF_MM_STRUCT_INTERESTS = 1,
};

enum kzt_file_interests {
	F_PATH_OF_FILE,
	NUM_OF_FILE_INTERESTS,
	VERSION_OF_FILE_INTERESTS = 1,
};

enum kzt_path_interests {
	DENTRY_OF_PATH,
	NUM_OF_PATH_INTERESTS,
	VERSION_OF_PATH_INTERESTS = 1,
};

enum kzt_dentry_interests {
	D_PARENT_OF_DENTRY,
	D_NAME_OF_DENTRY,
	D_INODE_OF_DENTRY,
	NUM_OF_DENTRY_INTERESTS,
	VERSION_OF_DENTRY_INTERESTS = 1,
};

enum kzt_qstr_interests {
	NAME_OF_QSTR,
	NUM_OF_QSTR_INTERESTS,
	VERSION_OF_QSTR_INTERESTS = 1,
};

enum kzt_inode_interests {
	I_MODE_OF_INODE,
	I_UID_OF_INODE,
	I_GID_OF_INODE,
	I_INO_OF_INODE,
	I_ATIME_OF_INODE,
	I_MTIME_OF_INODE,
	I_CTIME_OF_INODE,
	NUM_OF_INODE_INTERESTS,
	VERSION_OF_INODE_INTERESTS = 1,
};

enum kzt_timespec64_interests {
	TV_SEC_OF_TIMESPEC64,
	NUM_OF_TIMESPEC64_INTERESTS,
	VERSION_OF_TIMESPEC64_INTERESTS = 1,
};

enum kzt_sock_interests {
	SK_TYPE_OF_SOCK,
	SK_UID_OF_SOCK,
	NUM_OF_SOCK_INTERESTS,
	VERSION_OF_SOCK_INTERESTS = 1,
};

struct task_struct_offsets {
	u32 offsets[NUM_OF_TASK_STRUCT_INTERESTS];
};

struct cred_offsets {
	u32 offsets[NUM_OF_CRED_INTERESTS];
};

struct fs_struct_offsets {
	u32 offsets[NUM_OF_FS_STRUCT_INTERESTS];
};

struct mm_struct_offsets {
	u32 offsets[NUM_OF_MM_STRUCT_INTERESTS];
};

struct file_offsets {
	u32 offsets[NUM_OF_FILE_INTERESTS];
};

struct path_offsets {
	u32 offsets[NUM_OF_PATH_INTERESTS];
};

struct dentry_offsets {
	u32 offsets[NUM_OF_DENTRY_INTERESTS];
};

struct qstr_offsets {
	u32 offsets[NUM_OF_QSTR_INTERESTS];
};

struct inode_offsets {
	u32 offsets[NUM_OF_INODE_INTERESTS];
};

struct timespec64_offsets {
	u32 offsets[NUM_OF_TIMESPEC64_INTERESTS];
};

struct sock_offsets {
	u32 offsets[NUM_OF_SOCK_INTERESTS];
};

struct kzt_get_version_arg {
	u32 version;
} __packed;

struct kzt_get_offsets_arg {
	enum kzt_struct_type type;
	u32 version;
	union {
		struct task_struct_offsets task_struct_;
		struct cred_offsets cred_;
		struct fs_struct_offsets fs_struct_;
		struct mm_struct_offsets mm_struct_;
		struct file_offsets file_;
		struct path_offsets path_;
		struct dentry_offsets dentry_;
		struct qstr_offsets qstr_;
		struct inode_offsets inode_;
		struct timespec64_offsets timespec64_;
		struct sock_offsets sock_;
	};
} __packed;

#define KZT_IOCTL_GET_VERSION    _IOR('k', 0x01, struct kzt_get_version_arg)
#define KZT_IOCTL_GET_OFFSETS    _IOWR('k', 0x02, struct kzt_get_offsets_arg)

#endif /* End of KZT_COMMON_H */
