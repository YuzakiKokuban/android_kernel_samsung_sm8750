// SPDX-License-Identifier: GPL-2.0
/*
 * PROCA Database operation
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

#include "proca_certificate_db.h"
#include "proca_storage.h"
#include "proca_log.h"

int proca_get_certificate(struct file *file, char **cert_buff)
{
	int ret = 0;

	ret = proca_get_certificate_db(file, cert_buff);
	return ret;
}

bool proca_is_certificate_present(struct file *file)
{
	return proca_is_certificate_present_db(file);
}

int __init init_proca_storage(void)
{
	int ret = 0;

	ret = proca_keyring_init();
	if (ret)
		return ret;

	PROCA_INFO_LOG("Proca keyring was initialized\n");
	ret = proca_load_built_x509();
	if (ret)
		return ret;
	PROCA_INFO_LOG("Proca x509 certificate was initialized\n");

	ret = proca_certificate_db_init();
	if (ret)
		return ret;
	PROCA_INFO_LOG("Proca certificate db was initialized\n");

#ifdef CONFIG_PROCA_CERT_DEVICE
	ret = init_proca_cert_device();
#endif

	return ret;
}
