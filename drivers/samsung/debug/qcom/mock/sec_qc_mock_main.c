// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "sec_qc_mock.h"

int __init sec_qc_mock_init(void)
{
	__qc_mock_smem_init();

	return 0;
}
#if IS_BUILTIN(CONFIG_SEC_QC_MOCK)
pure_initcall(sec_qc_mock_init);
#else
module_init(sec_qc_mock_init);
#endif

void __exit sec_qc_mock_exit(void)
{
	__qc_mock_smem_exit();
}
module_exit(sec_qc_mock_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("SEC Qualcomm Mock driver under QEMU based testing");
MODULE_LICENSE("GPL v2");
