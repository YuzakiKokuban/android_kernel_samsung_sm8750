// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2010-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "sec_log_buf.h"

int __last_kmsg_alloc_buffer(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;
	const size_t log_buf_size = __log_buf_get_buf_size();

	last_kmsg->buf = vmalloc(log_buf_size);
	if (!last_kmsg->buf)
		return -ENOMEM;

	return 0;
}

void __last_kmsg_free_buffer(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	vfree(last_kmsg->buf);
}

int __last_kmsg_pull_last_log(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;
	char *buf = last_kmsg->buf;

	last_kmsg->size = __log_buf_copy_to_buffer(buf);

	return 0;
}

static int __last_kmsg_decompress_buf(struct last_kmsg_data *last_kmsg)
{
	void *buf;
	unsigned int size = last_kmsg->size;
	unsigned int size_comp = last_kmsg->size_comp;
	int err;

	buf = vmalloc(size);
	if (!buf) {
		pr_warn("failed to alloc buf\n");
		return -ENOMEM;
	}

	err = crypto_comp_decompress(last_kmsg->tfm,
			last_kmsg->buf_comp, size_comp, buf, &size);
	if (err) {
		pr_warn("failed to decompress (%d)\n", err);
		vfree(buf);
		return err;
	}

	last_kmsg->buf = buf;

	return 0;
}

static void __last_kmsg_release_buf(struct last_kmsg_data *last_kmsg)
{
	vfree(last_kmsg->buf);
	last_kmsg->buf = NULL;
}

static int sec_last_kmsg_buf_open(struct inode *inode, struct file *file)
{
	struct last_kmsg_data *last_kmsg = pde_data(inode);
	int err = 0;

	if (!last_kmsg->use_compression || !last_kmsg->size)
		return 0;

	mutex_lock(&last_kmsg->lock);

	if (last_kmsg->ref_cnt) {
		last_kmsg->ref_cnt++;
		goto already_decompressed;
	}

	err = __last_kmsg_decompress_buf(last_kmsg);
	if (err) {
		pr_warn("failed to decompress last_kmsg (%d)\n", err);
		goto err_decompress;
	}

	last_kmsg->ref_cnt++;

	mutex_unlock(&last_kmsg->lock);

	return 0;

err_decompress:
already_decompressed:
	mutex_unlock(&last_kmsg->lock);
	return err;
}

static ssize_t sec_last_kmsg_buf_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	struct last_kmsg_data *last_kmsg = pde_data(file_inode(file));
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_kmsg->size || !last_kmsg->buf) {
		pr_warn("pos %lld, size %zu\n", pos, last_kmsg->size);
		return 0;
	}

	count = min(len, (size_t)(last_kmsg->size - pos));
	if (copy_to_user(buf, last_kmsg->buf + pos, count))
		return -EFAULT;

	*offset += count;

	return count;
}

static loff_t sec_last_kmsg_buf_lseek(struct file *file, loff_t off,
		int whence)
{
	struct last_kmsg_data *last_kmsg = pde_data(file_inode(file));

	return fixed_size_llseek(file, off, whence, last_kmsg->size);
}

static int sec_last_kmsg_buf_release(struct inode *inode, struct file *file)
{
	struct last_kmsg_data *last_kmsg = pde_data(inode);

	if (!last_kmsg->use_compression)
		return 0;

	mutex_lock(&last_kmsg->lock);

	last_kmsg->ref_cnt--;
	if (last_kmsg->ref_cnt)
		goto still_used;

	__last_kmsg_release_buf(last_kmsg);

still_used:
	mutex_unlock(&last_kmsg->lock);

	return 0;
}

static const struct proc_ops last_kmsg_buf_pops = {
	.proc_open = sec_last_kmsg_buf_open,
	.proc_read = sec_last_kmsg_buf_read,
	.proc_lseek = sec_last_kmsg_buf_lseek,
	.proc_release = sec_last_kmsg_buf_release,
};

#define LAST_LOG_BUF_NODE		"last_kmsg"

int __last_kmsg_procfs_create(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	last_kmsg->proc = proc_create_data(LAST_LOG_BUF_NODE, 0444,
			NULL, &last_kmsg_buf_pops, last_kmsg);
	if (!last_kmsg->proc) {
		dev_warn(dev, "failed to create proc entry. ram console may be present\n");
		return -ENODEV;
	}

	mutex_init(&last_kmsg->lock);
	proc_set_size(last_kmsg->proc, last_kmsg->size);

	return 0;
}

void __last_kmsg_procfs_remove(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	proc_remove(last_kmsg->proc);
	mutex_destroy(&last_kmsg->lock);
}

static void *__last_kmsg_vmalloc_compressed(struct last_kmsg_data *last_kmsg,
		struct device *dev, struct crypto_comp *tfm, size_t *size_comp)
{
	unsigned int size_decomp = last_kmsg->size;
	unsigned int size = last_kmsg->size * 2;
	void *buf_tmp;
	void *buf_comp;
	int err;

	buf_tmp = vmalloc(size);
	if (!buf_tmp)
		return ERR_PTR(-ENOMEM);

	err = crypto_comp_compress(tfm, last_kmsg->buf, size_decomp,
			buf_tmp, &size);
	if (err || size >= size_decomp) {
		vfree(buf_tmp);
		return ERR_PTR(-EINVAL);
	}

	buf_comp = vmalloc(size);
	if (!buf_comp) {
		vfree(buf_tmp);
		return ERR_PTR(-ENOMEM);
	}

	memcpy(buf_comp, buf_tmp, size);
	vfree(buf_tmp);

	*size_comp = size;

	return buf_comp;
}

static int ____last_kmsg_init_compression(struct last_kmsg_data *last_kmsg,
		struct device *dev)
{
	struct crypto_comp *tfm;
	void *buf_comp;
	size_t size_comp;
	int err;

	tfm = crypto_alloc_comp(last_kmsg->compressor, 0, 0);
	if (IS_ERR(tfm)) {
		err = PTR_ERR(tfm);
		goto err_alloc_comp;
	}

	buf_comp = __last_kmsg_vmalloc_compressed(last_kmsg, dev, tfm, &size_comp);
	if (IS_ERR_OR_NULL(buf_comp)) {
		err = PTR_ERR(buf_comp);
		goto err_alloc_buf_comp;
	}

	vfree(last_kmsg->buf);
	last_kmsg->buf = NULL;
	last_kmsg->buf_comp = buf_comp;
	last_kmsg->size_comp = size_comp;
	last_kmsg->tfm = tfm;

	return 0;

err_alloc_buf_comp:
	crypto_free_comp(tfm);
err_alloc_comp:
	last_kmsg->use_compression = false;
	return err;
}

int __last_kmsg_init_compression(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	if (!last_kmsg->use_compression || !last_kmsg->size)
		return 0;

	return ____last_kmsg_init_compression(last_kmsg, dev);
}

static void ____last_kmsg_exit_compression(struct last_kmsg_data *last_kmsg,
		struct device *dev)
{
	vfree(last_kmsg->buf_comp);
	last_kmsg->buf_comp = NULL;

	crypto_free_comp(last_kmsg->tfm);
}

void __last_kmsg_exit_compression(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	if (!last_kmsg->use_compression)
		return;

	____last_kmsg_exit_compression(last_kmsg, dev);
}
