/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDAR_COMMON_H__
#define DDAR_COMMON_H__

#include <linux/printk.h>
#include <linux/types.h>

#define AID_USER_OFFSET     100000 /* offset for uid ranges for each user */
#define AID_APP_START        10000 /* first app user */
#define AID_APP_END          19999 /* last app user */
#define AID_VENDOR_DDAR_DE_ACCESS (5300)
static inline bool uid_is_app(uid_t uid)
{
	uid_t appid = uid % AID_USER_OFFSET;

	return appid >= AID_APP_START && appid <= AID_APP_END;
}

void dek_add_to_log(int engine_id, char *buffer);

static inline void secure_zeroout(const char *msg, unsigned char *raw, unsigned int size)
{
	int i, verified = 1;
	unsigned char *p = raw;

	for (i = 0; i < size; i++)
		p[i] = 0;

	for (i = 0; i < size; i++)
		if (p[i] != 0)
			verified = 0;

	if (!verified)
		pr_err("%s - %s verified:%d\n", __func__, msg, verified);
}
#endif
