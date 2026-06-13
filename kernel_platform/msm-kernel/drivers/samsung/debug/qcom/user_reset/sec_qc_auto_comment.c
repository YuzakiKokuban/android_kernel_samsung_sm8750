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

static int __auto_comment_prepare_reset_header(
		struct qc_user_reset_proc *auto_comment)
{
	struct debug_reset_header *reset_header;

	reset_header = sec_qc_user_reset_get_reset_header();
	if (IS_ERR_OR_NULL(reset_header))
		return -ENOENT;

	if (reset_header->auto_comment_size == 0) {
		kvfree(reset_header);
		return -ENODATA;
	}

	auto_comment->reset_header = reset_header;
	auto_comment->len = reset_header->auto_comment_size;

	return 0;
}

static void __auto_comment_release_reset_header(
		struct qc_user_reset_proc *auto_comment)
{
	struct debug_reset_header *reset_header = auto_comment->reset_header;

	__qc_user_reset_set_reset_header(reset_header);
	kvfree(reset_header);

	auto_comment->reset_header = NULL;
}

static int __auto_comment_prepare_buf(
		struct qc_user_reset_proc *auto_comment)
{
	char *buf;
	int ret = 0;
	ssize_t size;

	size = sec_qc_dbg_part_get_size(debug_index_auto_comment);
	if (size <= 0) {
		ret = -EINVAL;
		goto err_get_size;
	}

	buf = kvmalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto err_nomem;
	}

	if (!sec_qc_dbg_part_read(debug_index_auto_comment, buf)) {
		ret = -ENXIO;
		goto failed_to_read;
	}

	auto_comment->buf = buf;

	return 0;

failed_to_read:
	kvfree(buf);
err_nomem:
err_get_size:
	return ret;
}

static void __auto_comment_release_buf(
		struct qc_user_reset_proc *auto_comment)
{
	kvfree(auto_comment->buf);
	auto_comment->buf = NULL;
}

static int sec_qc_auto_comment_proc_open(struct inode *inode,
		struct file *file)
{
	struct qc_user_reset_proc *auto_comment = pde_data(inode);
	int err = 0;

	mutex_lock(&auto_comment->lock);

	if (auto_comment->ref_cnt) {
		auto_comment->ref_cnt++;
		goto already_cached;
	}

	err = __auto_comment_prepare_reset_header(auto_comment);
	if (err) {
		pr_warn("failed to load reset_header (%d)\n", err);
		goto err_reset_header;
	}

	err = __auto_comment_prepare_buf(auto_comment);
	if (err) {
		pr_warn("failed to load to buffer (%d)\n", err);
		goto err_buf;
	}

	auto_comment->ref_cnt++;

	mutex_unlock(&auto_comment->lock);

	return 0;

err_buf:
	__auto_comment_release_reset_header(auto_comment);
err_reset_header:
already_cached:
	mutex_unlock(&auto_comment->lock);
	return err;
}

static ssize_t sec_qc_auto_comment_proc_read(struct file *file,
		char __user *buf, size_t nbytes, loff_t *ppos)
{
	struct qc_user_reset_proc *auto_comment = pde_data(file_inode(file));
	loff_t pos = *ppos;

	if (pos < 0 || pos > auto_comment->len)
		return 0;

	nbytes = min_t(size_t, nbytes, auto_comment->len - pos);
	if (copy_to_user(buf, &auto_comment->buf[pos], nbytes))
		return -EFAULT;

	*ppos += nbytes;

	return nbytes;
}

static loff_t sec_qc_auto_comment_proc_lseek(struct file *file, loff_t off,
		int whence)
{
	struct qc_user_reset_proc *auto_comment = pde_data(file_inode(file));

	return fixed_size_llseek(file, off, whence, auto_comment->len);
}

static int sec_qc_auto_comment_proc_release(struct inode *inode,
		struct file *file)
{
	struct qc_user_reset_proc *auto_comment = pde_data(inode);

	mutex_lock(&auto_comment->lock);

	auto_comment->ref_cnt--;
	if (auto_comment->ref_cnt)
		goto still_used;

	auto_comment->len = 0;
	__auto_comment_release_buf(auto_comment);
	__auto_comment_release_reset_header(auto_comment);

still_used:
	mutex_unlock(&auto_comment->lock);

	return 0;
}

static const struct proc_ops auto_comment_pops = {
	.proc_open = sec_qc_auto_comment_proc_open,
	.proc_read = sec_qc_auto_comment_proc_read,
	.proc_lseek = sec_qc_auto_comment_proc_lseek,
	.proc_release = sec_qc_auto_comment_proc_release,
};

int __qc_auto_comment_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *auto_comment = &drvdata->auto_comment;
	const char *node_name = "auto_comment";

	mutex_init(&auto_comment->lock);

	auto_comment->proc = proc_create_data(node_name, 0444, NULL,
			&auto_comment_pops, auto_comment);
	if (!auto_comment->proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

void __qc_auto_comment_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_user_reset_proc *auto_comment = &drvdata->auto_comment;

	proc_remove(auto_comment->proc);
	mutex_destroy(&auto_comment->lock);
}
