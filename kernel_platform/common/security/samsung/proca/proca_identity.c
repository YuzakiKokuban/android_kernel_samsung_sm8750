// SPDX-License-Identifier: GPL-2.0
/*
 * PROCA identity
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Ivan Vorobiov, <i.vorobiov@samsung.com>
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
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

#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>

#include "proca_identity.h"
#include "proca_certificate.h"
#include "proca_log.h"

int init_proca_identity(struct proca_identity *identity, struct file *file,
			char **cert_value, const size_t cert_size,
			struct proca_certificate *parsed_cert)
{
	int rc = 0;

	if (!file)
		return -EINVAL;

	get_file(file);
	identity->file = file;
	identity->certificate_size = cert_size;
	identity->certificate = NULL;
	if (cert_value) {
		identity->certificate = *cert_value;
		*cert_value = NULL;
	}

	if (parsed_cert)
		identity->parsed_cert = *parsed_cert;
	else
		memset(&identity->parsed_cert,
				0, sizeof(identity->parsed_cert));

	return rc;
}

int proca_identity_copy(struct proca_identity *dst, struct proca_identity *src)
{
	int rc = 0;

	PROCA_BUG_ON(!dst || !src);

	memset(dst, 0, sizeof(*dst));

	get_file(src->file);
	dst->file = src->file;

	if (src->certificate) {
		dst->certificate = kmemdup(
				src->certificate,
				src->certificate_size,
				GFP_KERNEL);
		dst->certificate_size =
				src->certificate_size;

		if (unlikely(!dst->certificate)) {
			fput(src->file);
			return -ENOMEM;
		}
	}

	rc = proca_certificate_copy(&dst->parsed_cert, &src->parsed_cert);
	if (rc != 0) {
		kfree(dst->certificate);
		fput(src->file);
	}

	return rc;
}

void deinit_proca_identity(struct proca_identity *identity)
{
	if (unlikely(!identity))
		return;

	deinit_proca_certificate(&identity->parsed_cert);
	if (identity->file)
		fput(identity->file);
	kfree(identity->certificate);
}

#if defined(CONFIG_SEC_KUNIT)
EXPORT_SYMBOL_GPL(proca_identity_copy);
EXPORT_SYMBOL_GPL(init_proca_identity);
#endif
