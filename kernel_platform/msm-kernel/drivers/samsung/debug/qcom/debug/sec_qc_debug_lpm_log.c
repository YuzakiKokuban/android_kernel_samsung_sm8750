// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2021-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>
#include <linux/samsung/debug/sec_debug.h>
#include <linux/samsung/debug/sec_log_buf.h>

#include "sec_qc_debug.h"

static void __qc_lpm_log_pull_log_buf(uint8_t *dst,
		const struct sec_log_buf_head *s_log_buf, ssize_t s_log_buf_sz)
{
	size_t head;

	if (s_log_buf->idx > s_log_buf_sz) {
		head = s_log_buf->idx % s_log_buf_sz;
		memcpy_fromio(dst, &s_log_buf->buf[head], s_log_buf_sz - head);
		if (head != 0)
			memcpy_fromio(&dst[s_log_buf_sz - head],
					s_log_buf->buf, head);
	} else {
		memcpy_fromio(dst, s_log_buf->buf, s_log_buf->idx);
	}
}

static int __qc_lpm_log_store(struct qc_debug_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	uint8_t *buf;
	uint8_t *begin;
	const struct sec_log_buf_head *s_log_buf;
	ssize_t s_log_buf_sz;
	bool valid;

	s_log_buf = sec_log_buf_get_header();
	if (IS_ERR(s_log_buf)) {
		dev_warn(dev, "sec_log_buf is not available. (%ld)\n",
				PTR_ERR(s_log_buf));
		return 0;
	}

	s_log_buf_sz = sec_log_buf_get_buf_size();
	if (s_log_buf_sz <= 0) {
		dev_warn(dev, "sec_log_buf size (%zd) <= 0.\n", s_log_buf_sz);
		return 0;
	}

	buf = vmalloc(max_t(size_t, s_log_buf_sz,
				SEC_DEBUG_RESET_LPM_KLOG_SIZE));
	if (!buf)
		return -ENOMEM;

	__qc_lpm_log_pull_log_buf(buf, s_log_buf, s_log_buf_sz);

	if (SEC_DEBUG_RESET_LPM_KLOG_SIZE >= s_log_buf_sz)
		begin = buf;
	else
		begin = &buf[s_log_buf_sz - SEC_DEBUG_RESET_LPM_KLOG_SIZE];

	valid = sec_qc_dbg_part_write(debug_index_reset_lpm_klog, begin);
	if (!valid)
		dev_err(dev, "failed to write lpm kmsg.\n");

	vfree(buf);

	return 0;
}

static int __qc_lpm_log_init(struct qc_debug_drvdata *drvdata)
{
	struct debug_reset_header __reset_header;
	struct debug_reset_header *reset_header = &__reset_header;
	bool valid;

	drvdata->is_lpm_mode = true;

	valid = sec_qc_dbg_part_read(debug_index_reset_summary_info,
			reset_header);
	if (!valid)
		return -EPROBE_DEFER;

	if (reset_header->magic != DEBUG_PARTITION_MAGIC) {
		dev_warn(drvdata->bd.dev,
				"debug parition is not valid. storing lpm kmsg is skpped.\n");
		return 0;
	}

	return __qc_lpm_log_store(drvdata);
}

static char *boot_mode __ro_after_init;
module_param_named(boot_mode, boot_mode, charp, 0440);

int sec_qc_debug_lpm_log_init(struct builder *bd)
{
	struct qc_debug_drvdata *drvdata =
			container_of(bd, struct qc_debug_drvdata, bd);

	if (!drvdata->use_store_lpm_kmsg)
		return 0;

	if (!boot_mode || strncmp(boot_mode, "charger", strlen("charger")))
		return 0;

	return __qc_lpm_log_init(drvdata);
}
