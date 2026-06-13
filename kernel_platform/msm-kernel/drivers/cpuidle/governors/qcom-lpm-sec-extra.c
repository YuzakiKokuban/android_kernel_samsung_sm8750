// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2020-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/module.h>

#include "qcom-lpm.h"

#if IS_ENABLED(CONFIG_SEC_QC_QCOM_WDT_CORE)
void qcom_lpm_set_sleep_disabled(void)
{
	sleep_disabled = true;
}
EXPORT_SYMBOL(qcom_lpm_set_sleep_disabled);

void qcom_lpm_unset_sleep_disabled(void)
{
	sleep_disabled = false;
}
EXPORT_SYMBOL(qcom_lpm_unset_sleep_disabled);
#endif
