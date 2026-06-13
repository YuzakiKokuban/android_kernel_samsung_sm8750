// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sdp/sdp_debug.h>

int __weak sdp_get_disp_id(void *ctx)
{
	return UNKNOWN_DISP_ID;
}
EXPORT_SYMBOL(sdp_get_disp_id);

MODULE_DESCRIPTION("sdp debug");
MODULE_LICENSE("GPL");
