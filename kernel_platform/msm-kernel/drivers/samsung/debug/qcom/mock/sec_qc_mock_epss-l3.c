// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

int __weak qcom_icc_epss_l3_cpu_set_register_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL(qcom_icc_epss_l3_cpu_set_register_notifier);

int __weak qcom_icc_epss_l3_cpu_set_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL(qcom_icc_epss_l3_cpu_set_unregister_notifier);
