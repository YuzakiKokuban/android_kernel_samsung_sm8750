// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include "sec_qc_debug.h"

int __weak sec_qc_vendor_hooks_init(struct builder *bd)
{
	return 0;
}
