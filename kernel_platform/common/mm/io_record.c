// SPDX-License-Identifier: GPL-2.0
/*
 * io_record/
 *
 * Copyright (C) 2024 Samsung Electronics
 *
 */

#include <linux/module.h>
#include <linux/file.h>
#include <linux/rmap.h>
#include <linux/sort.h>
#include <linux/vmalloc.h>
#include <linux/io_record.h>
#include <trace/hooks/fs.h>
#include <trace/hooks/mm.h>

#define NUM_IO_INFO_IN_BUF (64 * 1024) /* # of struct io_info */
#define RESULT_BUF_SIZE_IN_BYTES (512 * 1024) /* 512 KB */
#define RESULT_BUF_END_MAGIC (~0) /* -1 */

struct io_info {
	struct file *file;
	struct inode *inode;
	int offset;
	int nr_pages;
};

enum io_record_cmd_types {
	IO_RECORD_INIT = 1,
	IO_RECORD_START = 2,
	IO_RECORD_STOP = 3,
	IO_RECORD_POST_PROCESSING = 4,
	IO_RECORD_POST_PROCESSING_DONE = 5,
};

static struct io_info *record_buf; /* array of struct io_info */
static void *result_buf; /* buffer used for post processing result */
static void *result_buf_cursor; /* this is touched by post processing only */
static atomic_t record_buf_cursor = ATOMIC_INIT(-1); /* record_buf array idx */
static int record_target; /* pid # of group leader */
static bool record_enable;
static DEFINE_RWLOCK(record_rwlock);
static DEFINE_MUTEX(status_lock);
static enum io_record_cmd_types current_status = IO_RECORD_INIT;

/*
 * format in result buf per file:
 * <A = length of "path", (size = sizeof(int))>
 * <"path" string, (size = A)>
 * <tuple array, (size = B * sizeof(int) * 2>
 * <end MAGIC, (val = -1, size = sizeof(int) * 2>
 */

static void write_to_result_buf(void *src, int size)
{
	memcpy(result_buf_cursor, src, size);
	result_buf_cursor = result_buf_cursor + size;
}

/* this assumes that start_idx~end_idx belong to the same inode */
static int fill_result_buf(int start_idx, int end_idx)
{
	struct file *file;
	char *path, strbuf[MAX_FILEPATH_LEN];
	int i, size_expected, pathsize, result_buf_used;
	int prev_offset = -1;
	int max_size = 0;
	void *buf_start;

	if (start_idx >= end_idx)
		BUG_ON(1); /* this case is not in consideration */

	file = record_buf[start_idx].file;
	path = d_path(&file->f_path, strbuf, MAX_FILEPATH_LEN);
	if (!path || IS_ERR(path))
		return 0;

	/* max size check (not strict) */
	result_buf_used = result_buf_cursor - result_buf;
	size_expected = sizeof(int) * 2 +                         /* end magic of this attempt */
			sizeof(int) + strlen(path) +              /* for path string */
			sizeof(int) * 2 * (end_idx - start_idx) + /* data */
			sizeof(int);                              /* end magic of post-processing */
	if (size_expected > RESULT_BUF_SIZE_IN_BYTES - result_buf_used)
		return -EINVAL;

	buf_start = result_buf_cursor;
	pathsize = strlen(path);
	write_to_result_buf(&pathsize, sizeof(int));
	write_to_result_buf(path, pathsize);

	/* fill the result buf using the record buf */
	for (i = start_idx; i < end_idx; i++) {
		if (prev_offset == -1) {
			prev_offset = record_buf[i].offset;
			max_size = record_buf[i].nr_pages;
			continue;
		}
		/* in the last range */
		if (prev_offset + max_size >=
		    record_buf[i].offset + record_buf[i].nr_pages)
			continue;

		if (prev_offset + max_size >= record_buf[i].offset) {
			max_size = record_buf[i].offset +
				record_buf[i].nr_pages - prev_offset;
		} else {
			write_to_result_buf(&prev_offset, sizeof(int));
			write_to_result_buf(&max_size, sizeof(int));
			prev_offset = record_buf[i].offset;
			max_size = record_buf[i].nr_pages;
		}
	}
	/* fill the record buf */
	write_to_result_buf(&prev_offset, sizeof(int));
	write_to_result_buf(&max_size, sizeof(int));

	/* fill the record buf with final magic */
	prev_offset = RESULT_BUF_END_MAGIC;
	max_size = RESULT_BUF_END_MAGIC;
	write_to_result_buf(&prev_offset, sizeof(int));
	write_to_result_buf(&max_size, sizeof(int));

	/* return # of bytes written to result buf */
	return result_buf_cursor - buf_start;
}

static inline void set_record_enable(bool enable)
{
	write_lock(&record_rwlock);
	record_enable = enable;
	write_unlock(&record_rwlock);
}

/* assume caller has read lock of record_rwlock */
static inline bool __get_record_status(void)
{
	return record_enable;
}

static inline void set_record_target(int pid)
{
	write_lock(&record_rwlock);
	record_target = pid;
	write_unlock(&record_rwlock);
}

static void release_records(void)
{
	struct io_info *info;
	int i;

	for (i = 0; i < atomic_read(&record_buf_cursor); i++) {
		info = record_buf + i;
		fput(info->file);
	}
}

/* change the current status, and do the init jobs for the status */
static void change_current_status(enum io_record_cmd_types status)
{
	switch (status) {
	case IO_RECORD_INIT:
		set_record_enable(false);
		set_record_target(-1);
		release_records();
		atomic_set(&record_buf_cursor, 0);
		result_buf_cursor = result_buf;
		break;
	case IO_RECORD_START:
		set_record_enable(true);
		break;
	case IO_RECORD_STOP:
		set_record_enable(false);
		break;
	default:
		break;
	}
	current_status = status;
}

/*
 * Only this function contains the status change rules.
 * Caller should hold status lock
 */
static inline bool change_status_if_valid(enum io_record_cmd_types next_status)
{
	bool ret = false;

	if (!record_buf)
		return ret;

	if (next_status == IO_RECORD_INIT &&
	    current_status != IO_RECORD_POST_PROCESSING)
		ret = true;
	else if (next_status == current_status + 1)
		ret = true;
	if (ret)
		change_current_status(next_status);

	return ret;
}

static bool set_record_status(enum io_record_cmd_types status, int pid)
{
	bool ret;

	mutex_lock(&status_lock);
	ret = change_status_if_valid(status);
	if (ret && status == IO_RECORD_START)
		set_record_target(pid);
	mutex_unlock(&status_lock);
	return ret;
}

static void io_info_swap(void *lhs, void *rhs, int size)
{
	struct io_info tmp;
	struct io_info *linfo = (struct io_info *)lhs;
	struct io_info *rinfo = (struct io_info *)rhs;

	memcpy(&tmp, linfo, sizeof(struct io_info));
	memcpy(linfo, rinfo, sizeof(struct io_info));
	memcpy(rinfo, &tmp, sizeof(struct io_info));
}

static int io_info_compare(const void *lhs, const void *rhs)
{
	struct io_info *linfo = (struct io_info *)lhs;
	struct io_info *rinfo = (struct io_info *)rhs;

	if ((unsigned long)linfo->inode > (unsigned long)rinfo->inode)
		return 1;
	if ((unsigned long)linfo->inode < (unsigned long)rinfo->inode)
		return -1;
	if ((unsigned long)linfo->offset > (unsigned long)rinfo->offset)
		return 1;
	if ((unsigned long)linfo->offset < (unsigned long)rinfo->offset)
		return -1;
	return 0;
}

static bool post_processing_records(void)
{
	struct inode *prev = NULL;
	int start_idx = -1, end_idx = -1;
	int last_magic = RESULT_BUF_END_MAGIC;
	int i;
	bool ret = false;

	mutex_lock(&status_lock);
	if (!change_status_if_valid(IO_RECORD_POST_PROCESSING))
		goto out;

	/* From this point, we assume that no one touches record buf */
	/* sort based on inode pointer address */
	sort(record_buf, atomic_read(&record_buf_cursor),
	     sizeof(struct io_info), &io_info_compare, &io_info_swap);

	/* fill the result buf per inode */
	for (i = 0; i < atomic_read(&record_buf_cursor); i++) {
		if (prev != record_buf[i].inode) {
			end_idx = i;
			/* if result buf full, break without write */
			if (prev && fill_result_buf(start_idx, end_idx) < 0)
				break;
			prev = record_buf[i].inode;
			start_idx = i;
		}
	}

	if (start_idx != -1)
		fill_result_buf(start_idx, i);

	/* fill the last magic to indicate end of result */
	write_to_result_buf(&last_magic, sizeof(int));

	if (!change_status_if_valid(IO_RECORD_POST_PROCESSING_DONE))
		BUG_ON(1); /* this is the case not in consideration */

	ret = true;
out:
	mutex_unlock(&status_lock);
	return ret;
}

static void io_record_store(struct file *file, pgoff_t offset, int nr_pages)
{
	struct io_info *info;
	int cnt;

	if (!file || !nr_pages)
		return;

	/* check without lock */
	if (task_tgid_nr(current) != record_target)
		return;

	cnt = atomic_read(&record_buf_cursor);
	if (cnt < 0 || cnt >= NUM_IO_INFO_IN_BUF)
		return;

	if (!read_trylock(&record_rwlock))
		return;

	if (!__get_record_status())
		goto out;

	/* strict check */
	if (task_tgid_nr(current) != record_target)
		goto out;

	cnt = atomic_inc_return(&record_buf_cursor) - 1;

	/* buffer is full */
	if (cnt >= NUM_IO_INFO_IN_BUF) {
		atomic_dec(&record_buf_cursor);
		goto out;
	}

	info = record_buf + cnt;

	get_file(file); /* will be put in release_records */
	info->file = file;
	info->inode = file_inode(file);
	info->offset = (int)offset;
	info->nr_pages = nr_pages;
out:
	read_unlock(&record_rwlock);
}

ssize_t io_record_read(char __user *buf, size_t count, loff_t *ppos)
{
	int result_buf_size;
	int ret;

	mutex_lock(&status_lock);
	if (current_status != IO_RECORD_POST_PROCESSING_DONE) {
		ret = -EFAULT;
		goto out;
	}

	result_buf_size = result_buf_cursor - result_buf;
	if (*ppos >= result_buf_size) {
		ret = 0;
		goto out;
	}

	ret = *ppos + count < result_buf_size ?
			count : result_buf_size - *ppos;
	if (copy_to_user(buf, result_buf + *ppos, ret)) {
		ret = -EFAULT;
		goto out;
	}

	*ppos = *ppos + ret;
out:
	mutex_unlock(&status_lock);
	return ret;
}

bool io_record_write(struct task_struct *task, int type)
{
	switch (type) {
	case IO_RECORD_INIT:
	case IO_RECORD_START:
	case IO_RECORD_STOP:
		return set_record_status(type, task_pid_nr(task));
	case IO_RECORD_POST_PROCESSING:
		return post_processing_records();
	}
	return false;
}

static void io_record_ksys_umount(void *data, char __user *name, int flags)
{
	int loopnum = 1;

	if (!record_buf)
		return;

	while (!set_record_status(IO_RECORD_INIT, -1))
		loopnum++;

	if (loopnum > 1)
		pr_err("%s,%d: loopnum %d\n", __func__, __LINE__, loopnum);
}

static void io_record_do_read_fault(void *data, struct vm_fault *vmf,
		unsigned long fault_around_pages)
{
	if (vmf->vma->vm_ops->map_pages && fault_around_pages == 1)
		io_record_store(vmf->vma->vm_file, vmf->pgoff, 1);
}

static void io_record_filemap_map_pages(void *data, struct file *file,
		pgoff_t first_pgoff, pgoff_t last_pgoff, vm_fault_t ret)
{
	if (ret == VM_FAULT_NOPAGE)
		io_record_store(file, first_pgoff, last_pgoff - first_pgoff + 1);
}

static void io_record_filemap_read(void *data, struct file *file,
		loff_t pos, size_t size)
{
	io_record_store(file, pos >> PAGE_SHIFT,
			(size + PAGE_SIZE - 1) >> PAGE_SHIFT);
}

static int __init io_record_init(void)
{
	record_buf = vzalloc(sizeof(struct io_info) * NUM_IO_INFO_IN_BUF);
	if (!record_buf)
		return -ENOMEM;

	result_buf = vzalloc(RESULT_BUF_SIZE_IN_BYTES);
	if (!result_buf) {
		vfree(record_buf);
		return -ENOMEM;
	}

	mutex_lock(&status_lock);
	change_status_if_valid(IO_RECORD_INIT);
	mutex_unlock(&status_lock);

	register_trace_android_vh_do_read_fault(
			io_record_do_read_fault, NULL);
	register_trace_android_vh_filemap_map_pages(
			io_record_filemap_map_pages, NULL);
	register_trace_android_vh_filemap_read(
			io_record_filemap_read, NULL);
	register_trace_android_rvh_ksys_umount(
			io_record_ksys_umount, NULL);
	return 0;
}

static void __exit io_record_exit(void)
{
	vfree(record_buf);
	vfree(result_buf);

	unregister_trace_android_vh_do_read_fault(
			io_record_do_read_fault, NULL);
	unregister_trace_android_vh_filemap_map_pages(
			io_record_filemap_map_pages, NULL);
	unregister_trace_android_vh_filemap_read(
			io_record_filemap_read, NULL);
}

module_init(io_record_init);
module_exit(io_record_exit);
MODULE_LICENSE("GPL");
