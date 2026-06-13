// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2022-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/seq_file.h>

#include "sec_log_buf.h"

static void __log_buf_dbgfs_show_basic(struct seq_file *m,
		struct log_buf_drvdata *drvdata)
{
	seq_puts(m, "- Basic Information:\n");
	seq_printf(m, "  + VA:0x%p (PA:%pa) / %zu bytes\n", __log_buf_get_header(),
			&drvdata->paddr, drvdata->size);
	seq_puts(m, "\n");
}

static void __log_buf_dbgfs_show_logger(struct seq_file *m,
		struct log_buf_drvdata *drvdata)
{
	seq_puts(m, "- Logger Information:\n");
	seq_printf(m, "  + logger : [%u] %ps\n", drvdata->strategy, drvdata->logger);
	seq_puts(m, "\n");
}

static void __log_buf_dbgfs_show_last_kmsg(struct seq_file *m,
		struct log_buf_drvdata *drvdata)
{
	struct last_kmsg_data *last_kmsg = &drvdata->last_kmsg;

	seq_puts(m, "- Last-KMSG Information:\n");
	seq_printf(m, "  + compressor : %s\n", last_kmsg->use_compression ?
			last_kmsg->compressor : "none");

	if (!last_kmsg->use_compression)
		seq_printf(m, "  + VA:0x%p / %zu bytes\n",
				last_kmsg->buf, last_kmsg->size);
	else {
		size_t ratio = (last_kmsg->size_comp * 100000) / last_kmsg->size;

		seq_printf(m, "  + VA:0x%p / %zu (%zu) bytes (%zu.%03zu%%)\n",
				last_kmsg->buf_comp, last_kmsg->size_comp, last_kmsg->size,
				ratio / 1000, ratio % 1000);
	}

	seq_puts(m, "\n");
}

static int sec_log_buf_dbgfs_show_all(struct seq_file *m, void *unsed)
{
	struct log_buf_drvdata *drvdata = m->private;

	__log_buf_dbgfs_show_basic(m, drvdata);
	__log_buf_dbgfs_show_logger(m, drvdata);
	__log_buf_dbgfs_show_last_kmsg(m, drvdata);

	return 0;
}

static int sec_log_buf_dbgfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_log_buf_dbgfs_show_all, inode->i_private);
}

static const struct file_operations sec_log_buf_dgbfs_fops = {
	.open = sec_log_buf_dbgfs_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int __log_buf_debugfs_create(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);

	drvdata->dbgfs = debugfs_create_file("sec_log_buf", 0440,
			NULL, drvdata, &sec_log_buf_dgbfs_fops);

	return 0;
}

void __log_buf_debugfs_remove(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);

	debugfs_remove(drvdata->dbgfs);
}
