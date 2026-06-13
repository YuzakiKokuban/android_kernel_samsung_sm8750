// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>

void __weak qcom_set_dload_mode(int on)
{
	pr_info("on = %d\n", on);
}
EXPORT_SYMBOL_GPL(qcom_set_dload_mode);
