// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>

static bool mock_sleep_disabled = true;

void __weak qcom_lpm_set_sleep_disabled(void)
{
	mock_sleep_disabled = true;
}
EXPORT_SYMBOL_GPL(qcom_lpm_set_sleep_disabled);

void __weak qcom_lpm_unset_sleep_disabled(void)
{
	mock_sleep_disabled = false;
}
EXPORT_SYMBOL_GPL(qcom_lpm_unset_sleep_disabled);