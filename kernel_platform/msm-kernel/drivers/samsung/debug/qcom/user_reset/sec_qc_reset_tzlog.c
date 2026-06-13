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

#include <linux/samsung/debug/qcom/sec_qc_dbg_partition.h>

#include "sec_qc_user_reset.h"

static int __reset_tzlog_prepare_reset_header(
		struct qc_user_reset_proc *reset_tzlog)
{
	struct debug_reset_header *reset_header;
	ssize_t len;
	int ret;

	reset_header = sec_qc_user_reset_get_reset_header();
	if (IS_ERR_OR_NULL(reset_header)) {
		ret = -ENOENT;
		goto err_no_reset_header;
	}

	if (reset_header->stored_tzlog == 0) {
		pr_warn("the target didn't run sdi\n");
		ret = -EINVAL;
		goto err_no_sdi_run;
	}

	len = sec_qc_dbg_part_get_size(debug_index_reset_tzlog);
	if (len <= 0) {
		ret = -ENODEV;
		goto err_invalid_part_size;
	}

	reset_tzlog->reset_header = reset_header;
	reset_tzlog->len = len;

	return 0;

err_invalid_part_size:
err_no_sdi_run:
	kvfree(reset_header);
err_no_reset_header:
	return ret;
}

static void __reset_tzlog_release_reset_header(
		struct qc_user_reset_proc *reset_tzlog)
{
	struct debug_reset_header *reset_header = reset_tzlog->reset_header;

	__qc_user_reset_set_reset_header(reset_header);
	kvfree(reset_header);

	reset_tzlog->reset_header = NULL;
}

static int __reset_tzlog_prepare_buf(
		struct qc_user_reset_proc *reset_tzlog)
{
	char *buf;
	int ret = 0;
	ssize_t size;

	size = sec_qc_dbg_part_get_size(debug_index_reset_tzlog);
	if (size <= 0) {
		ret = -EINVAL;
		goto err_get_size;
	}

	buf = kvmalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto err_nomem;
	}

	if (!sec_qc_dbg_part_read(debug_index_reset_tzlog, buf)) {
		ret = -ENXIO;
		goto failed_to_read;
	}

	reset_tzlog->buf = buf;

	return 0;

failed_to_read:
	kvfree(buf);
err_nomem:
err_get_size:
	return ret;
}

static void __reset_tzlog_release_buf(struct qc_user_reset_proc *reset_tzlog)
{
	kvfree(reset_tzlog->buf);
	reset_tzlog->buf = NULL;
}

static int sec_qc_reset_tzlog_proc_open(struct inode *inode, struct file *file)
{
	struct qc_user_reset_proc *reset_tzlog = pde_data(inode);
	int err = 0;

	mutex_lock(&reset_tzlog->lock);

	if (reset_tzlog->ref_cnt) {
		reset_tzlog->ref_cnt++;
		goto already_cached;
	}

	err = __reset_tzlog_prepare_reset_header(reset_tzlog);
	if (err) {
		pr_warn("failed to load reset_header (%d)\n", err);
		goto err_reset_header;
	}

	err = __reset_tzlog_prepare_buf(reset_tzlog);
	if (err) {
		pr_warn("failed to load to buffer (%d)\n", err);
		goto err_buf;
	}

	reset_tzlog->ref_cnt++;

	mutex_unlock(&reset_tzlog->lock);

	return 0;

err_buf:
	__reset_tzlog_release_reset_header(reset_tzlog);
err_reset_header:
already_cached:
	mutex_unlock(&reset_tzlog->lock);
	return err;
}

static ssize_t sec_qc_reset_tzlog_proc_read(struct file *file,
		char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct qc_user_reset_proc *reset_tzlog = pde_data(file_inode(file));
	loff_t pos = *ppos;

	if (pos < 0 || pos > reset_tzlog->len)
		return 0;

	nbytes = min_t(size_t, nbytes, reset_tzlog->len - pos);
	if (copy_to_user(buf, &reset_tzlog->buf[pos], nbytes))
		return -EFAULT;

	*ppos += nbytes;

	return nbytes;
}

static loff_t sec_qc_reset_tzlog_proc_lseek(struct file *file, loff_t off,
		int whence)
{
	struct qc_user_reset_proc *reset_tzlog = pde_data(file_inode(file));

	return fixed_size_llseek(file, off, whence, reset_tzlog->len);
}

static int sec_qc_reset_tzlog_proc_release(struct inode *inode,
		struct file *file)
{
	struct qc_user_reset_proc *reset_tzlog = pde_data(inode);

	mutex_lock(&reset_tzlog->lock);

	reset_tzlog->ref_cnt--;
	if (reset_tzlog->ref_cnt)
		goto still_used;

	reset_tzlog->len = 0;
	__reset_tzlog_release_buf(reset_tzlog);
	__reset_tzlog_release_reset_header(reset_tzlog);

still_used:
	mutex_unlock(&reset_tzlog->lock);

	return 0;
}

static const struct proc_ops reset_tzlog_pops = {
	.proc_open = sec_qc_reset_tzlog_proc_open,
	.proc_read = sec_qc_reset_tzlog_proc_read,
	.proc_lseek = sec_qc_reset_tzlog_proc_lseek,
	.proc_release = sec_qc_reset_tzlog_proc_release,
};

int __qc_reset_tzlog_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *reset_tzlog = &drvdata->reset_tzlog;
	const char *node_name = "reset_tzlog";

	mutex_init(&reset_tzlog->lock);

	reset_tzlog->proc = proc_create_data(node_name, 0444, NULL,
			&reset_tzlog_pops, reset_tzlog);
	if (!reset_tzlog->proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

void __qc_reset_tzlog_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *reset_tzlog = &drvdata->reset_tzlog;

	proc_remove(reset_tzlog->proc);
	mutex_destroy(&reset_tzlog->lock);
}
