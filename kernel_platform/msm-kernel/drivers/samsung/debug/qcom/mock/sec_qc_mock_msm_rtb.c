// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/samsung/debug/qcom/sec_qc_summary.h>

void __weak sec_qc_summary_set_rtb_info(struct sec_qc_summary_data_apss *apss)
{
}
EXPORT_SYMBOL_GPL(sec_qc_summary_set_rtb_info);
