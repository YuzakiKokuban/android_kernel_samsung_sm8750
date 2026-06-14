// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#include "sec_log_buf.h"

static int sec_ap_klog_open(struct inode *inode, struct file *file)
{
	struct ap_klog_proc *ap_klog = pde_data(inode);
	struct log_buf_drvdata *drvdata = container_of(ap_klog,
			struct log_buf_drvdata, ap_klog);
	const size_t sz_buf = drvdata->size;
	int err = 0;

	mutex_lock(&ap_klog->lock);

	if (ap_klog->ref_cnt) {
		ap_klog->ref_cnt++;
		goto already_cached;
	}

	ap_klog->buf = vmalloc(sz_buf);
	if (!ap_klog->buf) {
		err = -ENOMEM;
		goto err_vmalloc;
	}

	ap_klog->size = __log_buf_copy_to_buffer(ap_klog->buf);
	ap_klog->ref_cnt++;

	mutex_unlock(&ap_klog->lock);

	return 0;

err_vmalloc:
already_cached:
	mutex_unlock(&ap_klog->lock);
	return err;
}

static ssize_t sec_ap_klog_read(struct file *file, char __user * buf,
		size_t len, loff_t * offset)
{
	struct ap_klog_proc *ap_klog = pde_data(file_inode(file));
	loff_t pos = *offset;
	ssize_t count;

	if (pos < 0 || pos > ap_klog->size)
		return 0;

	count = min(len, (size_t) (ap_klog->size - pos));
	if (copy_to_user(buf, ap_klog->buf + pos, count))
		return -EFAULT;

	*offset += count;

	return count;
}

static loff_t sec_ap_klog_lseek(struct file *file, loff_t off, int whence)
{
	struct ap_klog_proc *ap_klog = pde_data(file_inode(file));

	return fixed_size_llseek(file, off, whence, ap_klog->size);
}

static int sec_ap_klog_release(struct inode *inode, struct file *file)
{
	struct ap_klog_proc *ap_klog = pde_data(inode);

	mutex_lock(&ap_klog->lock);

	ap_klog->ref_cnt--;
	if (ap_klog->ref_cnt)
		goto still_used;

	vfree(ap_klog->buf);
	ap_klog->buf = NULL;
	ap_klog->size = 0;

still_used:
	mutex_unlock(&ap_klog->lock);

	return 0;
}

static const struct proc_ops ap_klog_pops = {
	.proc_open = sec_ap_klog_open,
	.proc_read = sec_ap_klog_read,
	.proc_lseek = sec_ap_klog_lseek,
	.proc_release = sec_ap_klog_release,
};

#define AP_KLOG_PROC_NODE		"sec_log"

int __ap_klog_proc_init(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	struct ap_klog_proc *ap_klog = &drvdata->ap_klog;

	ap_klog->proc = proc_create_data(AP_KLOG_PROC_NODE, 0444,
			NULL, &ap_klog_pops, ap_klog);
	if (!ap_klog->proc) {
		dev_warn(dev, "failed to create proc entry\n");
		return -ENODEV;
	}

	mutex_init(&ap_klog->lock);

	return 0;
}

void __ap_klog_proc_exit(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct ap_klog_proc *ap_klog = &drvdata->ap_klog;

	proc_remove(ap_klog->proc);
	mutex_destroy(&ap_klog->lock);
}

#define AP_KLOG_PROC_SYMLINK		"ap_klog"

int __ap_klog_proc_create_symlink(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct device *dev = bd->dev;
	struct ap_klog_proc *ap_klog = &drvdata->ap_klog;

	ap_klog->symlink = proc_symlink(AP_KLOG_PROC_SYMLINK, NULL,
		AP_KLOG_PROC_NODE);
	if (!ap_klog->symlink) {
		dev_warn(dev, "failed to create proc entry\n");
		return -ENODEV;
	}

	return 0;
}

void __ap_klog_proc_remove_symlink(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);
	struct ap_klog_proc *ap_klog = &drvdata->ap_klog;

	proc_remove(ap_klog->symlink);
}
