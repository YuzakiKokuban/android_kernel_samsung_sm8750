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

static ssize_t __extrt_info_report_reset_reason(unsigned int reset_reason,
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

static ssize_t __extrt_info_report_tz_diag_info_v9_3(const struct tzdbg_t *tz_diag_info,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const struct tzdbg_log_v9_2_t *log_v9_2 = (struct tzdbg_log_v9_2_t *)((void *)tz_diag_info
			+ tz_diag_info->ring_off - sizeof(struct tzdbg_log_pos_v9_2_t));
	const struct tzbsp_encr_info_t *encr_info = (struct tzbsp_encr_info_t *)((void *)tz_diag_info
			+ tz_diag_info->v9_3.encr_info_for_log_off);
	const size_t logbuf_size = encr_info->chunks[1].size_to_encr ? : 512;
	size_t i;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"TZDA\":\"");
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		cpu_to_be32(tz_diag_info->magic_num), cpu_to_be32(tz_diag_info->version));
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		cpu_to_be32(tz_diag_info->ring_off), cpu_to_be32(tz_diag_info->ring_len));
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		cpu_to_be32(tz_diag_info->v9_3.encr_info_for_log_off),
		cpu_to_be32(encr_info->chunks[1].size_to_encr));

	for (i = 0; i < TZBSP_AES_256_ENCRYPTED_KEY_SIZE; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", encr_info->key[i]);

	for (i = 0; i < TZBSP_NONCE_LEN; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", encr_info->chunks[1].nonce[i]);

	for (i = 0; i < TZBSP_TAG_LEN; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", encr_info->chunks[1].tag[i]);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		cpu_to_be32(log_v9_2->log_pos.wrap), cpu_to_be32(log_v9_2->log_pos.offset));

	for (i = 0; i < logbuf_size; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", log_v9_2->log_buf[i]);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");

	return info_size;
}

static ssize_t __extrt_info_report_tz_diag_info_v9_2(const struct tzdbg_t *tz_diag_info,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const struct tzdbg_log_v9_2_t *log_v9_2 = (struct tzdbg_log_v9_2_t *)((void *)tz_diag_info
			+ tz_diag_info->ring_off - sizeof(struct tzdbg_log_pos_v9_2_t));
	size_t i;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"TZDA\":\"");
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		tz_diag_info->magic_num, tz_diag_info->version);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X%08X%08X",
		tz_diag_info->cpu_count, tz_diag_info->ring_off,
		tz_diag_info->ring_len, tz_diag_info->v9_2.num_interrupts);

	for (i = 0; i < TZBSP_AES_256_ENCRYPTED_KEY_SIZE; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", tz_diag_info->v9_2.key[i]);

	for (i = 0; i < TZBSP_NONCE_LEN; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", tz_diag_info->v9_2.nonce[i]);

	for (i = 0; i < TZBSP_TAG_LEN; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", tz_diag_info->v9_2.tag[i]);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		log_v9_2->log_pos.wrap, log_v9_2->log_pos.offset);

	for (i = 0; i < tz_diag_info->ring_len; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", log_v9_2->log_buf[i]);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");

	return info_size;
}

static ssize_t __extrt_info_report_tz_diag_info_legacy(const struct tzdbg_t *tz_diag_info,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	const struct tzdbg_log_t *log = (struct tzdbg_log_t *)((void *)tz_diag_info
		+ tz_diag_info->ring_off - sizeof(struct tzdbg_log_pos_t));
	size_t i;

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"TZDA\":\"");
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X",
		tz_diag_info->magic_num, tz_diag_info->version);
	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%08X%08X%08X%08X",
		tz_diag_info->cpu_count, tz_diag_info->ring_off,
		tz_diag_info->ring_len, tz_diag_info->v9_2.num_interrupts);

	for (i = 0; i < TZBSP_AES_256_ENCRYPTED_KEY_SIZE; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", tz_diag_info->v9_2.key[i]);

	for (i = 0; i < TZBSP_NONCE_LEN; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", tz_diag_info->v9_2.nonce[i]);

	for (i = 0; i < TZBSP_TAG_LEN; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", tz_diag_info->v9_2.tag[i]);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%04X%04X",
		log->log_pos.wrap, log->log_pos.offset);

	for (i = 0; i < tz_diag_info->ring_len; i++)
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "%02X", log->log_buf[i]);

	__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"");

	return info_size;
}

static ssize_t __extrt_info_report_tz_diag_info(const struct tzdbg_t *tz_diag_info,
		char *buf, const ssize_t sz_buf, ssize_t info_size)
{
	ssize_t written;

	/* TODO: I'll remove statements when it is not used anymore in future. */
	if (tz_diag_info->version > TZBSP_DIAG_VERSION_V9_2)
		written = __extrt_info_report_tz_diag_info_v9_3(tz_diag_info, buf,
			sz_buf, info_size);
	else if (tz_diag_info->version == TZBSP_DIAG_VERSION_V9_2)
		written = __extrt_info_report_tz_diag_info_v9_2(tz_diag_info,
			buf, sz_buf, info_size);
	else
		written = __extrt_info_report_tz_diag_info_legacy(tz_diag_info,
			buf, sz_buf, info_size);

	return written;
}

static ssize_t extrt_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	const ssize_t sz_buf = SPECIAL_LEN_STR;
	unsigned int reset_reason;
	struct tzdbg_t *tz_diag_info;
	ssize_t size;

	reset_reason = sec_qc_reset_reason_get();
	if (!__qc_hw_param_is_valid_reset_reason(reset_reason))
		goto err_invalid_reset_reason;

	size = sec_qc_dbg_part_get_size(debug_index_reset_tzlog);
	if (size <= 0)
		goto err_get_size;

	tz_diag_info = kmalloc(size, GFP_KERNEL);
	if (!tz_diag_info)
		goto err_enomem;

	if (!sec_qc_dbg_part_read(debug_index_reset_tzlog, tz_diag_info)) {
		dev_warn(dev, "failed to read debug paritition.\n");
		goto err_failed_to_read;
	}

	info_size = __extrt_info_report_reset_reason(reset_reason, buf, sz_buf, info_size);

	if (tz_diag_info->magic_num != TZ_DIAG_LOG_MAGIC) {
		__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"ERR\":\"tzlog magic num error\"");
		goto err_invalid_maic;
	}

	if (tz_diag_info->version > TZBSP_DIAG_VERSION_V9_2) {
		struct tzbsp_encr_info_t *encr_info = (struct tzbsp_encr_info_t *)((void *)tz_diag_info
				+ tz_diag_info->v9_3.encr_info_for_log_off);
		if (encr_info->chunks[1].size_to_encr > 0x200) {/* 512 byte */
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"ERR\":\"tzlog size over\"");
			goto err_size_over;
		}
	} else {
		if (tz_diag_info->ring_len > 0x200)	{/* 512 byte */
			__qc_hw_param_scnprintf(buf, sz_buf, info_size, "\"ERR\":\"tzlog size over\"");
			goto err_size_over;
		}
	}

	info_size = __extrt_info_report_tz_diag_info(tz_diag_info, buf, sz_buf, info_size);

err_size_over:
err_invalid_maic:
err_failed_to_read:
	kfree(tz_diag_info);
err_enomem:
	__qc_hw_param_clean_format(buf, &info_size, sz_buf);
err_get_size:
err_invalid_reset_reason:
	return info_size;
}
DEVICE_ATTR_RO(extrt_info);
