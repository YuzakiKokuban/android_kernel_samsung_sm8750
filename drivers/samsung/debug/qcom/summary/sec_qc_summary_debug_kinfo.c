// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/timer.h>

#include <debug_kinfo.h>

#include "sec_qc_summary.h"

int __qc_summary_debug_kinfo_init(struct builder *bd)
{
	struct qc_summary_drvdata *drvdata =
			container_of(bd, struct qc_summary_drvdata, bd);
	struct kernel_all_info *all_kinfo;

	if (!IS_ENABLED(CONFIG_ANDROID_DEBUG_KINFO) ||
			IS_BUILTIN(CONFIG_SEC_QC_SUMMARY))
		return 0;

	if (!drvdata->debug_kinfo_rmem)
		return 0;

	all_kinfo = drvdata->debug_kinfo_rmem->priv;
	if (all_kinfo->magic_number != DEBUG_KINFO_MAGIC)
		return -EPROBE_DEFER;

	return 0;
}
