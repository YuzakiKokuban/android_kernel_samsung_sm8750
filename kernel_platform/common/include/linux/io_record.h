/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IO_RECORD_H
#define _LINUX_IO_RECORD_H

#define MAX_FILEPATH_LEN 256

ssize_t io_record_read(char __user *buf, size_t count, loff_t *ppos);
bool io_record_write(struct task_struct *task, int type);

extern const struct file_operations proc_pid_filemap_list_ops;
extern const struct file_operations proc_pid_io_record_ops;

#endif /* _LINUX_IO_RECORD_H */
