// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2006-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <linux/samsung/debug/qcom/sec_qc_user_reset.h>

#include "sec_qc_user_reset.h"

static int sec_qc_store_lastkmsg_proc_show(struct seq_file *m, void *v)
{
	struct debug_reset_header *is_stored =
			sec_qc_user_reset_get_reset_header();

	if (IS_ERR_OR_NULL(is_stored))
		seq_puts(m, "0\n");
	else {
		seq_puts(m, "1\n");
		kfree(is_stored);
	}

	return 0;
}

static int sec_qc_store_lastkmsg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_qc_store_lastkmsg_proc_show, NULL);
}

static const struct proc_ops store_lastkmsg_pops = {
	.proc_open = sec_qc_store_lastkmsg_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int __qc_store_lastkmsg_init(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct proc_dir_entry *proc = drvdata->store_lastkmsg_proc;
	const char *node_name = "store_lastkmsg";

	proc = proc_create(node_name, 0444, NULL, &store_lastkmsg_pops);
	if (!proc) {
		dev_err(drvdata->bd.dev, "failed create procfs node (%s)\n",
				node_name);
		return -ENODEV;
	}

	return 0;
}

void __qc_store_lastkmsg_exit(struct builder *bd)
{
	struct qc_user_reset_drvdata *drvdata =
			container_of(bd, struct qc_user_reset_drvdata, bd);
	struct proc_dir_entry *proc = drvdata->store_lastkmsg_proc;

	proc_remove(proc);
}
