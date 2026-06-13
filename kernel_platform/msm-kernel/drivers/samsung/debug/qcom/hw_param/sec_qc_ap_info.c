// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched/clock.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param.h"

static ssize_t ap_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct qc_ap_info *ap_info = &qc_hw_param->ap_info;
	ssize_t info_size = 0;
	const ssize_t sz_buf = DEFAULT_LEN_STR;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"HW_REV\":\"%d\"", ap_info->system_rev);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, ",\"SoC_ID\":\"%u\"", socinfo_get_id());

	__qc_hw_param_clean_format(buf, &info_size, sz_buf);

	return info_size;
}
DEVICE_ATTR_RO(ap_info);

static unsigned int system_rev __ro_after_init;
module_param_named(revision, system_rev, uint, 0440);

int __qc_ap_info_init(struct builder *bd)
{
	struct qc_hw_param_drvdata *drvdata =
			container_of(bd, struct qc_hw_param_drvdata, bd);
	struct qc_ap_info *ap_info = &drvdata->ap_info;

	ap_info->system_rev = system_rev;

	return 0;
}
