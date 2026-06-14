/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef __LINUX_FIVE_FILE_H
#define __LINUX_FIVE_FILE_H

struct file *test_open_file(const char *filename);
void test_close_file(struct file *file);
ssize_t test_write_file(struct file *file, const char __user *buf,
	size_t count, loff_t *pos);

#endif // __LINUX_FIVE_FILE_H
