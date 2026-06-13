// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2016-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <linux/samsung/debug/sec_log_buf.h>
#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

#include "sec_qc_user_reset.h"

static int __reset_klog_prepare_reset_header(
		struct qc_user_reset_proc *reset_klog)
{
	struct debug_reset_header *reset_header;

	reset_header = sec_qc_user_reset_get_reset_header();
	if (IS_ERR_OR_NULL(reset_header))
		return -ENOENT;

	reset_klog->reset_header = reset_header;

	return 0;
}

static void __reset_klog_release_reset_header(
		struct qc_user_reset_proc *reset_klog)
{
	struct debug_reset_header *reset_header = reset_klog->reset_header;

	__qc_user_reset_set_reset_header(reset_header);
	kvfree(reset_header);

	reset_klog->reset_header = NULL;
}

static size_t __reset_klog_copy(const struct debug_reset_header *reset_header,
		char *dst, const char *src)
{
	const struct sec_log_buf_head *s_log_buf =
			(struct sec_log_buf_head *)src;
	size_t last_idx, idx, len;
	const char *log_src;
	size_t klog_buf_max_size;

	if (s_log_buf->magic == SEC_LOG_MAGIC) {
		last_idx = max_t(size_t, s_log_buf->idx,
				reset_header->ap_klog_idx);
		log_src = &s_log_buf->buf[0];
	} else {
		last_idx = reset_header->ap_klog_idx;
		log_src = src;
	}

	klog_buf_max_size = SEC_DEBUG_RESET_KLOG_SIZE -
			offsetof(struct sec_log_buf_head, buf);

	idx = last_idx % klog_buf_max_size;
	len = 0;
	if (last_idx > klog_buf_max_size) {
		len = klog_buf_max_size - idx;
		memcpy(dst, &log_src[idx], len);
	}

	memcpy(&dst[len], log_src, idx);

	return klog_buf_max_size;
}

static int __reset_klog_prepare_buf(struct qc_user_reset_proc *reset_klog)
{
	const struct debug_reset_header *reset_header =
			reset_klog->reset_header;
	char *buf_raw;
	char *buf;
	int ret = 0;
	ssize_t size;

	size = sec_qc_dbg_part_get_size(debug_index_reset_klog);
	if (size <= 0) {
		ret = -EINVAL;
		goto err_get_size;
	}

	buf_raw = kvmalloc(size, GFP_KERNEL);
	buf = kvmalloc(size, GFP_KERNEL);
	if (!buf_raw || !buf) {
		ret = -ENOMEM;
		goto err_nomem;
	}

	if (!sec_qc_dbg_part_read(debug_index_reset_klog, buf_raw)) {
		ret = -ENXIO;
		goto failed_to_read;
	}

	reset_klog->len = __reset_klog_copy(reset_header, buf, buf_raw);
	reset_klog->buf = buf;

	kvfree(buf_raw);

	return 0;

failed_to_read:
err_nomem:
	kvfree(buf_raw);
	kvfree(buf);
err_get_size:
	return ret;
}

static void __reset_klog_release_buf(
		struct qc_user_reset_proc *reset_klog)
{
	kvfree(reset_klog->buf);
	reset_klog->buf = NULL;
}

static int sec_qc_reset_klog_proc_open(struct inode *inode, struct file *file)
{
	struct qc_user_reset_proc *reset_klog = pde_data(inode);
	int err = 0;

	mutex_lock(&reset_klog->lock);

	if (reset_klog->ref_cnt) {
		reset_klog->ref_cnt++;
		goto already_cached;
	}

	err = __reset_klog_prepare_reset_header(reset_klog);
	if (err) {
		pr_warn("failed to load reset_header (%d)\n", err);
		goto err_reset_header;
	}

	err = __reset_klog_prepare_buf(reset_klog);
	if (err) {
		pr_warn("failed to load to buffer (%d)\n", err);
		goto err_buf;
	}

	reset_klog->ref_cnt++;

	mutex_unlock(&reset_klog->lock);

	return 0;

err_buf:
	__reset_klog_release_reset_header(reset_klog);
err_reset_header:
already_cached:
	mutex_unlock(&reset_klog->lock);
	return err;
}

static ssize_t sec_qc_reset_klog_proc_read(struct file *file,
		char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct qc_user_reset_proc *reset_klog = pde_data(file_inode(file));
	loff_t pos = *ppos;

	if (pos < 0 || pos > reset_klog->len)
		return 0;

	nbytes = min_t(size_t, nbytes, reset_klog->len - pos);
	if (copy_to_user(buf, &reset_klog->buf[pos], nbytes))
		return -EFAULT;

	*ppos += nbytes;

	return nbytes;
}

static loff_t sec_qc_reset_klog_proc_lseek(struct file *file, loff_t off,
		int whence)
{
	struct qc_user_reset_proc *reset_klog = pde_data(file_inode(file));

	return fixed_size_llseek(file, off, whence, reset_klog->len);
}

static int sec_qc_reset_klog_proc_release(struct inode *inode,
		struct file *file)
{
	struct qc_user_reset_proc *reset_klog = pde_data(inode);

	mutex_lock(&reset_klog->lock);

	reset_klog->ref_cnt--;
	if (reset_klog->ref_cnt)
		goto still_used;

	reset_klog->len = 0;
	__reset_klog_release_buf(reset_klog);
	__reset_klog_release_reset_header(reset_klog);

still_used:
	mutex_unlock(&reset_klog->lock);

	return 0;
}

static const struct proc_ops reset_klog_pops = {
	.proc_open = sec_qc_reset_klog_proc_open,
	.proc_read = sec_qc_reset_klog_proc_read,
	.proc_lseek = sec_qc_reset_klog_proc_lseek,
	.proc_release = sec_qc_reset_klog_proc_release,
};

int __qc_reset_klog_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *reset_klog = &drvdata->reset_klog;
	const char *node_name = "reset_klog";

	mutex_init(&reset_klog->lock);

	reset_klog->proc = proc_create_data(node_name, 0444, NULL,
			&reset_klog_pops, reset_klog);
	if (!reset_klog->proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

void __qc_reset_klog_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *reset_klog = &drvdata->reset_klog;

	proc_remove(reset_klog->proc);
	mutex_destroy(&reset_klog->lock);
}
