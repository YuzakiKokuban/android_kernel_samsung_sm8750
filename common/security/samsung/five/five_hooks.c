// SPDX-License-Identifier: GPL-2.0
/*
 * Five Event interface
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Ivan Vorobiov, <i.vorobiov@samsung.com>
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

#include "five_hooks.h"
#include "five_porting.h"
#include "five_testing.h"

#include <linux/fs.h>
#include <linux/slab.h>

#define call_void_hook(FUNC, ...)				\
	do {							\
		struct five_hook_list *P;			\
								\
		list_for_each_entry(P, &five_hook_heads.FUNC, list)	\
			P->hook.FUNC(__VA_ARGS__);		\
	} while (0)

struct five_hook_heads five_hook_heads = {
	.file_processed =
		LIST_HEAD_INIT(five_hook_heads.file_processed),
	.file_skipped =
		LIST_HEAD_INIT(five_hook_heads.file_skipped),
	.file_signed =
		LIST_HEAD_INIT(five_hook_heads.file_signed),
	.task_forked =
		LIST_HEAD_INIT(five_hook_heads.task_forked),
	.integrity_reset =
		LIST_HEAD_INIT(five_hook_heads.integrity_reset),
	.integrity_reset2 =
		LIST_HEAD_INIT(five_hook_heads.integrity_reset2),
};
EXPORT_SYMBOL_GPL(five_hook_heads);

void five_hook_file_processed(struct task_struct *task,
				struct file *file, void *xattr,
				size_t xattr_size, int result)
{
	call_void_hook(file_processed,
		task,
		task_integrity_read(TASK_INTEGRITY(task)),
		file,
		xattr,
		xattr_size,
		result);
}

void five_hook_file_signed(struct task_struct *task,
				struct file *file, void *xattr,
				size_t xattr_size, int result)
{
	call_void_hook(file_signed,
		task,
		task_integrity_read(TASK_INTEGRITY(task)),
		file,
		xattr,
		xattr_size,
		result);
}

void five_hook_file_skipped(struct task_struct *task, struct file *file)
{
	call_void_hook(file_skipped,
		task,
		task_integrity_read(TASK_INTEGRITY(task)),
		file);
}

void five_hook_task_forked(struct task_struct *parent,
				struct task_struct *child,
				enum task_integrity_value parent_tint_value,
				enum task_integrity_value child_tint_value)
{
	call_void_hook(task_forked,
		parent,
		parent_tint_value,
		child,
		child_tint_value);
}

__mockable
void five_hook_integrity_reset(struct task_struct *task,
			       struct file *file,
			       enum task_integrity_reset_cause cause)
{
	if (task == NULL)
		return;

	call_void_hook(integrity_reset, task);
	call_void_hook(integrity_reset2, task, file, cause);
}
