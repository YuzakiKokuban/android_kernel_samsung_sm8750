// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2014-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_hw_param.h"

static ssize_t __extrm_info_report_reset_reason(unsigned int reset_reason,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const char *reset_reason_str;
	int reset_write_cnt;

	reset_reason_str = sec_qc_reset_reason_to_str(reset_reason);
	reset_write_cnt = sec_qc_get_reset_write_cnt();

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RR\":\"%s\",", reset_reason_str);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RWC\":\"%d\",", reset_write_cnt);

	return info_size;
}

static ssize_t extrm_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = SPECIAL_LEN_STR;
	unsigned int reset_reason;
	char *extrm_buf;

	reset_reason = sec_qc_reset_reason_get();
	if (!__qc_hw_param_is_valid_reset_reason(reset_reason))
		goto err_invalid_reset_reason;

	extrm_buf = kmalloc(SEC_DEBUG_RESET_ETRM_SIZE, GFP_KERNEL);
	if (!extrm_buf)
		goto err_enomem;

	if (!sec_qc_dbg_part_read(debug_index_reset_rkplog, extrm_buf)) {
		dev_warn(dev, "failed to read debug paritition.\n");
		goto err_failed_to_read;
	}

	info_size = __extrm_info_report_reset_reason(reset_reason, buf, sz_buf, info_size);

	extrm_buf[SEC_DEBUG_RESET_ETRM_SIZE - 1] = '\0';

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"RKP\":\"%s\"", &extrm_buf[info_size]);

err_failed_to_read:
	kfree(extrm_buf);
err_enomem:
	__qc_hw_param_clean_format(buf, &info_size, sz_buf);
err_invalid_reset_reason:
	return info_size;
}
DEVICE_ATTR_RO(extrm_info);
