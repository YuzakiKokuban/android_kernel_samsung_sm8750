// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/notifier.h>

int __weak qcom_wdt_pet_register_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_wdt_pet_register_notifier);

int __weak qcom_wdt_pet_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_wdt_pet_unregister_notifier);

int __weak qcom_wdt_bark_register_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_wdt_bark_register_notifier);

int __weak qcom_wdt_bark_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_wdt_bark_unregister_notifier);
