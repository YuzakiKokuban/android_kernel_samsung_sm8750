// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include "sec_qc_user_reset.h"

static struct debug_reset_header *__user_reset_alloc_reset_header(void)
{
	struct debug_reset_header *header;
	int err;
	bool valid;

	header = kmalloc(sizeof(*header), GFP_KERNEL);
	if (!header) {
		err = -ENOMEM;
		goto err_nomem;
	}

	valid = sec_qc_dbg_part_read(debug_index_reset_summary_info, header);
	if (!valid) {
		err = -ENXIO;
		goto failed_to_read;
	}

	return header;

failed_to_read:
	kfree(header);
err_nomem:
	return ERR_PTR(err);
}

struct debug_reset_header *sec_qc_user_reset_get_reset_header(void)
{
	static int get_state = DRH_STATE_INIT;
	struct debug_reset_header *header;
	struct device *dev = qc_user_reset->bd.dev;
	struct qc_reset_rwc_proc *reset_rwc = &qc_user_reset->reset_rwc;
	struct mutex *lock = &reset_rwc->lock;

	mutex_lock(lock);

	if (get_state == DRH_STATE_INVALID)
		goto err_drh_state_invalid;

	header = __user_reset_alloc_reset_header();
	if (IS_ERR_OR_NULL(header)) {
		dev_warn(dev, "failed to read debug partition (%ld)",
			PTR_ERR(header));
		goto err_debug_partition;
	}

	if (get_state == DRH_STATE_VALID) {
		mutex_unlock(lock);
		return header;
	}

	if (header->write_times == header->read_times) {
		dev_warn(dev, "untrustworthy debug_rest_header\n");
		get_state = DRH_STATE_INVALID;
		goto err_untrustworthy_header;
	}

	reset_rwc->reset_write_cnt = header->write_times;
	get_state = DRH_STATE_VALID;
	mutex_unlock(lock);
	return header;

err_untrustworthy_header:
	kfree(header);
err_debug_partition:
err_drh_state_invalid:
	mutex_unlock(lock);
	return NULL;
}
EXPORT_SYMBOL_GPL(sec_qc_user_reset_get_reset_header);

int __qc_user_reset_set_reset_header(struct debug_reset_header *header)
{
	static int set_state = DRH_STATE_INIT;
	struct mutex *lock = &qc_user_reset->reset_rwc.lock;
	struct device *dev = qc_user_reset->bd.dev;
	int ret = 0;

	mutex_lock(lock);
	if (set_state == DRH_STATE_VALID) {
		dev_info(dev, "debug_reset_header works well\n");
		mutex_unlock(lock);
		return 0;
	}

	if ((header->write_times - 1) == header->read_times) {
		dev_info(dev, "debug_reset_header works well\n");
		header->read_times++;
	} else {
		dev_info(dev, "debug_reset_header read[%d] and write[%d] has sync error\n",
				header->read_times, header->write_times);
		header->read_times = header->write_times;
	}

	if (!sec_qc_dbg_part_write(debug_index_reset_summary_info, header)) {
		dev_warn(dev, "filed to writhe debug_reset_header\n");
		ret = -ENOENT;
	} else {
		set_state = DRH_STATE_VALID;
	}

	mutex_unlock(lock);

	return ret;
}

static int __qc_get_reset_write_cnt(void)
{
	struct qc_reset_rwc_proc *reset_rwc = &qc_user_reset->reset_rwc;

	return reset_rwc->reset_write_cnt;
}

int sec_qc_get_reset_write_cnt(void)
{
	if (!__qc_user_reset_is_probed())
		return 0;

	return __qc_get_reset_write_cnt();
}
EXPORT_SYMBOL_GPL(sec_qc_get_reset_write_cnt);

static int sec_qc_reset_rwc_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d", sec_qc_get_reset_write_cnt());

	return 0;
}

static int sec_qc_reset_rwc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_qc_reset_rwc_proc_show, NULL);
}

static const struct proc_ops reset_rwc_pops = {
	.proc_open = sec_qc_reset_rwc_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int __qc_reset_rwc_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_reset_rwc_proc *reset_rwc = &drvdata->reset_rwc;
	const char *node_name = "reset_rwc";

	mutex_init(&reset_rwc->lock);

	reset_rwc->proc = proc_create(node_name, 0444, NULL,
			&reset_rwc_pops);
	if (!reset_rwc->proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

void __qc_reset_rwc_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct qc_reset_rwc_proc *reset_rwc = &drvdata->reset_rwc;

	proc_remove(reset_rwc->proc);
	mutex_destroy(&reset_rwc->lock);
}
