// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2019-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/samsung/builder_pattern.h>
#include <linux/samsung/debug/sec_debug.h>

#include "sec_debug.h"

static unsigned int enable_user = 1;
module_param_named(enable_user, enable_user, uint, 0644);

static void sec_user_fault_dump(void)
{
	if (sec_debug_is_enabled() && enable_user)
		panic("User Fault");
}

static ssize_t sec_user_fault_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;

	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';
	if (!strncmp(buf, "dump_user_fault", strlen("dump_user_fault")))
		sec_user_fault_dump();

	return count;
}

static const struct proc_ops sec_user_fault_proc_fops = {
	.proc_write = sec_user_fault_write,
};

static struct proc_dir_entry *proc_user_fault;

int sec_user_fault_init(struct builder *bd)
{
	proc_user_fault = proc_create("user_fault", 0220, NULL,
			&sec_user_fault_proc_fops);
	if (!proc_user_fault)
		return -ENOMEM;

	return 0;
}

void sec_user_fault_exit(struct builder *bd)
{
	proc_remove(proc_user_fault);
}
