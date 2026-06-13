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

static ssize_t __extrc_info_report_reset_reason(unsigned int reset_reason,
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

static ssize_t extrc_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = SPECIAL_LEN_STR;
	unsigned int reset_reason;
	char *extrc_buf;
	size_t i;

	reset_reason = sec_qc_reset_reason_get();
	if (!__qc_hw_param_is_valid_reset_reason(reset_reason))
		goto err_invalid_reset_reason;

	extrc_buf = kmalloc(SEC_DEBUG_RESET_EXTRC_SIZE, GFP_KERNEL);
	if (!extrc_buf)
		goto err_enomem;

	if (!sec_qc_dbg_part_read(debug_index_reset_extrc_info, extrc_buf)) {
		dev_warn(dev, "failed to read debug paritition.\n");
		goto err_failed_to_read;
	}

	info_size = __extrc_info_report_reset_reason(reset_reason, buf, sz_buf, info_size);

	/* check " character and then change ' character */
	for (i = 0; i < SEC_DEBUG_RESET_EXTRC_SIZE; i++) {
		if (extrc_buf[i] == '"')
			extrc_buf[i] = '\'';
		if (extrc_buf[i] == '\0')
			break;
	}
	extrc_buf[SEC_DEBUG_RESET_EXTRC_SIZE - 1] = '\0';

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"LKL\":\"%s\"", &extrc_buf[info_size]);

err_failed_to_read:
	kfree(extrc_buf);
err_enomem:
	__qc_hw_param_clean_format(buf, &info_size, sz_buf);
err_invalid_reset_reason:
	return info_size;
}
DEVICE_ATTR_RO(extrc_info);
