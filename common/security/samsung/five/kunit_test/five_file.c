// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/shmem_fs.h>
#include <linux/buffer_head.h>

struct file *test_open_file(const char *filename)
{
	return shmem_kernel_file_setup(filename, 0, VM_NORESERVE);
}
EXPORT_SYMBOL_GPL(test_open_file);

void test_close_file(struct file *file)
{
	fput(file);
}
EXPORT_SYMBOL_GPL(test_close_file);

ssize_t test_write_file(struct file *file, const char __user *buf,
	size_t count, loff_t *pos)
{
	return vfs_write(file, buf, count, pos);
}
EXPORT_SYMBOL_GPL(test_write_file);
