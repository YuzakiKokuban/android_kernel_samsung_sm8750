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

#ifndef DEK_COMMON_H__
#define DEK_COMMON_H__

#include <linux/limits.h>
#include <linux/ioctl.h>
#include <linux/device.h>

#include <ddar/common.h>
#include "../../fs/crypto/ddar/ddar_crypto.h"

#define DEK_DEBUG		0

#if DEK_DEBUG
#define DEK_LOGD(...) pr_debug("dek: "__VA_ARGS__)
#else
#define DEK_LOGD(...)
#endif /* DEK_DEBUG */
#define DEK_LOGI(...) pr_info("dek: "__VA_ARGS__)
#define DEK_LOGE(...) pr_err("dek: "__VA_ARGS__)

void hex_key_dump(const char *tag, uint8_t *data, size_t data_len);

int dek_create_sysfs_key_dump(struct device *d);
int get_sdp_sysfs_key_dump(void);

#endif
