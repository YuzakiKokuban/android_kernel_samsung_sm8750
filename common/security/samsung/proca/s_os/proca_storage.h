/* SPDX-License-Identifier: GPL-2.0 */
/*
 * PROCA certificate storage API
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

#ifndef _LINUX_PROCA_STORAGE_H
#define _LINUX_PROCA_STORAGE_H

/* Public API for certificate storage */

/*
 * There are two options for certificates storage: xattr or database.
 * According to the selected storage type in config, the corresponding
 * implementation of API will be applied.
 */

/* Copy certificate content in cert_buff and return size of certificate */
int proca_get_certificate(struct file *file, char **cert_buff);

/* Check if certificate exists for current file */
bool proca_is_certificate_present(struct file *file);

/* Init proca Database resources in case of PROCA_CERTIFICATES_DB,
 * in case of PROCA_CERTIFICATES_XATTR init function is empty (no
 * additional initialization is required for xattr).
 */
int init_proca_storage(void);

#endif
