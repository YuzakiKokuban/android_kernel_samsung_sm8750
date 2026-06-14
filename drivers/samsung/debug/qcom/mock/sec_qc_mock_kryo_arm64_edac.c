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

ap_health_t * __weak qcom_kryo_arm64_edac_error_register_notifier(struct notifier_block *nb)
{
	return ERR_PTR(-ENODEV);
}

int __weak qcom_kryo_arm64_edac_error_unregister_notifier(struct notifier_block *nb)
{
	return -ENODEV;
}
