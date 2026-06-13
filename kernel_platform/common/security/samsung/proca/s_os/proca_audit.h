/* SPDX-License-Identifier: GPL-2.0 */
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

#ifndef __LINUX_PROCA_AUDIT_H
#define __LINUX_PROCA_AUDIT_H

#include <linux/task_integrity.h>

void proca_audit_err(struct task_struct *task, struct file *file,
		const char *op, const char *cause);

#endif
