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

static int __reset_summary_prepare_reset_header(
		struct qc_user_reset_proc *reset_summary)
{
	struct debug_reset_header *reset_header;
	ssize_t size;

	reset_header = sec_qc_user_reset_get_reset_header();
	if (IS_ERR_OR_NULL(reset_header))
		return -ENOENT;

	size = sec_qc_dbg_part_get_size(debug_index_reset_summary);
	if (size <= 0 || reset_header->summary_size > size ) {
		kvfree(reset_header);
		return -EINVAL;
	}

	reset_summary->reset_header = reset_header;
	reset_summary->len = reset_header->summary_size;

	return 0;
}

static void __reset_summary_release_reset_header(
		struct qc_user_reset_proc *reset_summary)
{
	struct debug_reset_header *reset_header = reset_summary->reset_header;

	__qc_user_reset_set_reset_header(reset_header);
	kvfree(reset_header);

	reset_summary->reset_header = NULL;
}

static int __reset_summary_prepare_buf(
		struct qc_user_reset_proc *reset_summary)
{
	char *buf;
	int ret = 0;
	ssize_t size;

	size = sec_qc_dbg_part_get_size(debug_index_reset_summary);
	if (size <= 0) {
		ret = -EINVAL;
		goto err_get_size;
	}

	buf = kvmalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto err_nomem;
	}

	if (!sec_qc_dbg_part_read(debug_index_reset_summary, buf)) {
		ret = -ENXIO;
		goto failed_to_read;
	}

	reset_summary->buf = buf;

	return 0;

failed_to_read:
	kvfree(buf);
err_nomem:
err_get_size:
	return ret;
}

static void __reset_summary_release_buf(
		struct qc_user_reset_proc *reset_summary)
{
	kvfree(reset_summary->buf);
	reset_summary->buf = NULL;
}

static int sec_qc_reset_summary_proc_open(struct inode *inode,
		struct file *file)
{
	struct qc_user_reset_proc *reset_summary = pde_data(inode);
	int err = 0;

	mutex_lock(&reset_summary->lock);

	if (reset_summary->ref_cnt) {
		reset_summary->ref_cnt++;
		goto already_cached;
	}

	err = __reset_summary_prepare_reset_header(reset_summary);
	if (err) {
		pr_warn("failed to load reset_header (%d)\n", err);
		goto err_reset_header;
	}

	err = __reset_summary_prepare_buf(reset_summary);
	if (err) {
		pr_warn("failed to load to buffer (%d)\n", err);
		goto err_buf;
	}

	reset_summary->ref_cnt++;

	mutex_unlock(&reset_summary->lock);

	return 0;

err_buf:
	__reset_summary_release_reset_header(reset_summary);
err_reset_header:
already_cached:
	mutex_unlock(&reset_summary->lock);
	return err;
}

static ssize_t sec_qc_reset_summary_proc_read(struct file *file,
		char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct qc_user_reset_proc *reset_summary = pde_data(file_inode(file));
	loff_t pos = *ppos;

	if (pos < 0 || pos > reset_summary->len)
		return 0;

	nbytes = min_t(size_t, nbytes, reset_summary->len - pos);
	if (copy_to_user(buf, &reset_summary->buf[pos], nbytes))
		return -EFAULT;

	*ppos += nbytes;

	return nbytes;
}

static loff_t sec_qc_reset_summary_proc_lseek(struct file *file, loff_t off,
		int whence)
{
	struct qc_user_reset_proc *reset_summary = pde_data(file_inode(file));

	return fixed_size_llseek(file, off, whence, reset_summary->len);
}

static int sec_qc_reset_summary_proc_release(struct inode *inode,
		struct file *file)
{
	struct qc_user_reset_proc *reset_summary = pde_data(inode);

	mutex_lock(&reset_summary->lock);

	reset_summary->ref_cnt--;
	if (reset_summary->ref_cnt)
		goto still_used;

	reset_summary->len = 0;
	__reset_summary_release_buf(reset_summary);
	__reset_summary_release_reset_header(reset_summary);

still_used:
	mutex_unlock(&reset_summary->lock);

	return 0;
}

static const struct proc_ops reset_summary_pops = {
	.proc_open = sec_qc_reset_summary_proc_open,
	.proc_read = sec_qc_reset_summary_proc_read,
	.proc_lseek = sec_qc_reset_summary_proc_lseek,
	.proc_release = sec_qc_reset_summary_proc_release,
};

int __qc_reset_summary_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *reset_summary = &drvdata->reset_summary;
	const char *node_name = "reset_summary";

	mutex_init(&reset_summary->lock);

	reset_summary->proc = proc_create_data(node_name, 0444, NULL,
			&reset_summary_pops, reset_summary);
	if (!reset_summary->proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

void __qc_reset_summary_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *reset_summary = &drvdata->reset_summary;

	proc_remove(reset_summary->proc);
	mutex_destroy(&reset_summary->lock);
}
