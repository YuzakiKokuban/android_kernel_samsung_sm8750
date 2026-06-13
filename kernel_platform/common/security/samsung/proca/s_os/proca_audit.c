// SPDX-License-Identifier: GPL-2.0
/*
 * Audit calls for PROCA audit subsystem
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

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/audit.h>
#include <linux/task_integrity.h>
#include <linux/version.h>
#include <linux/iversion.h>

#include "proca_log.h"
#include "proca_vfs.h"

static void proca_audit_msg(struct task_struct *task, struct file *file,
		const char *op, const char *cause);

void proca_audit_err(struct task_struct *task, struct file *file,
		const char *op, const char *cause)
{
	proca_audit_msg(task, file, op, cause);
}

static void proca_audit_msg(struct task_struct *task, struct file *file,
		const char *op, const char *cause)
{
	struct audit_buffer *ab;
	struct inode *inode = NULL;
	const char *fname = NULL;
	char *pathbuf = NULL;
	char filename[NAME_MAX];
	char comm[TASK_COMM_LEN];
	const char *name = "";
	struct task_struct *tsk = task ? task : current;

	if (file) {
		inode = file_inode(file);
		fname = proca_d_path(file, &pathbuf, filename);
	}

	ab = audit_log_start(current->audit_context, GFP_KERNEL,
			AUDIT_INTEGRITY_DATA);
	if (unlikely(!ab)) {
		PROCA_ERROR_LOG("Can't get a context of audit logs\n");
		goto exit;
	}

	audit_log_format(ab, " pid=%d", task_pid_nr(tsk));
	audit_log_format(ab, " tgid=%d", task_tgid_nr(tsk));
	audit_log_task_context(ab);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	audit_log_format(ab, " op=%s", op);
#else
	audit_log_format(ab, " op=");
	audit_log_string(ab, op);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	audit_log_format(ab, " cause=%s", cause);
#else
	audit_log_format(ab, " cause=");
	audit_log_string(ab, cause);
#endif
	audit_log_format(ab, " comm=");
	audit_log_untrustedstring(ab, get_task_comm(comm, tsk));
	if (fname) {
		audit_log_format(ab, " name=");
		audit_log_untrustedstring(ab, fname);
		name = fname;
	}
	if (inode) {
		audit_log_format(ab, " dev=");
		audit_log_untrustedstring(ab, inode->i_sb->s_id);
		audit_log_format(ab, " ino=%lu", inode->i_ino);
		audit_log_format(ab, " i_version=%llu ",
				inode_query_iversion(inode));
	}
	audit_log_end(ab);

exit:
	if (pathbuf)
		__putname(pathbuf);
}
