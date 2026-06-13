// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2022 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * SECDP logger
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
#include <linux/sched/clock.h>
#else
#include <linux/sched.h>
#endif
#include <linux/secdp_logger.h>

#include "secdp_unit_test.h"

#define BUF_SIZE	SZ_64K
#define MAX_STR_LEN	160
#define PROC_FILE_NAME	"dplog"
#define LOG_PREFIX	"secdp"

static char log_buf[BUF_SIZE];
static unsigned int g_curpos;
static int is_secdp_logger_init;
static int is_buf_full;
static int log_max_count = -1;
static unsigned int max_mode_count;
static struct mutex dplog_lock;
static struct proc_dir_entry *g_entry;

static void dp_logger_print_date_time(void)
{
	char tmp[64] = {0x0, };
	struct tm tm;
	struct timespec64 ts;
	unsigned long sec;

	ktime_get_real_ts64(&ts);
	sec = ts.tv_sec - (sys_tz.tz_minuteswest * 60);
	time64_to_tm(sec, 0, &tm);
	snprintf(tmp, sizeof(tmp), "!@[%02d-%02d %02d:%02d:%02d.%03lu]",
		tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		ts.tv_nsec / 1000000);

	secdp_logger_print("%s\n", tmp);
}

/* set max log count, if count is -1, no limit */
void secdp_logger_set_max_count(int count)
{
	log_max_count = count;
	dp_logger_print_date_time();
}

static void _secdp_logger_print(const char *fmt, va_list args)
{
	int len;
	char buf[MAX_STR_LEN] = {0, };
	u64 time;
	unsigned long nsec;
	volatile unsigned int curpos;

	mutex_lock(&dplog_lock);

	time = local_clock();
	nsec = do_div(time, 1000000000);
	len = snprintf(buf, sizeof(buf), "[%5lu.%06ld] ",
			(unsigned long)time, nsec / 1000);

	len += vsnprintf(buf + len, MAX_STR_LEN - len, fmt, args);
	if (len > MAX_STR_LEN)
		len = MAX_STR_LEN;

	curpos = g_curpos;
	if (curpos + len >= BUF_SIZE) {
		g_curpos = curpos = 0;
		is_buf_full = 1;
	}
	memcpy(log_buf + curpos, buf, len);
	g_curpos += len;

	mutex_unlock(&dplog_lock);
}

void secdp_logger_print(const char *fmt, ...)
{
	va_list args;

	if (!is_secdp_logger_init)
		return;

	if (!log_max_count)
		return;

	if (log_max_count > 0)
		log_max_count--;

	va_start(args, fmt);
	_secdp_logger_print(fmt, args);
	va_end(args);
}

/* set max num of modes print */
void secdp_logger_set_mode_max_count(unsigned int num)
{
	max_mode_count = num + 1;
}

void secdp_logger_dec_mode_count(void)
{
	if (!is_secdp_logger_init)
		return;

	if (max_mode_count > 0)
		max_mode_count--;
}

void secdp_logger_print_mode(const char *fmt, ...)
{
	va_list args;

	if (!is_secdp_logger_init)
		return;

	if (!max_mode_count)
		return;

	va_start(args, fmt);
	_secdp_logger_print(fmt, args);
	va_end(args);
}

void secdp_logger_hex_dump(void *buf, void *pref, size_t size)
{
	uint8_t *ptr = buf;
	size_t i;
	char tmp[128] = {0x0, };
	char *ptmp = tmp;
	int len;

	if (!is_secdp_logger_init)
		return;

	if (!log_max_count)
		return;

	if (log_max_count > 0)
		log_max_count--;

	for (i = 0; i < size; i++) {
		len = snprintf(ptmp, 4, "%02x ", *ptr++);
		ptmp = ptmp + len;
		if (((i+1)%16) == 0) {
			secdp_logger_print("%s%s\n", (char *)pref, tmp);
			ptmp = tmp;
		}
	}

	if (i % 16) {
		len = ptmp - tmp;
		tmp[len] = 0x0;
		secdp_logger_print("%s%s\n", (char *)pref, tmp);
	}
}

static ssize_t secdp_logger_read(struct file *file, char __user *buf,
		size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	size_t size;
	volatile unsigned int curpos;

	mutex_lock(&dplog_lock);

	curpos = g_curpos;

	if (is_buf_full || BUF_SIZE <= curpos)
		size = BUF_SIZE;
	else
		size = (size_t)curpos;

	if (pos >= size) {
		count = 0;
		goto end;
	}

	count = min(len, size);

	if ((pos + count) > size)
		count = size - pos;

	if (copy_to_user(buf, log_buf + pos, count)) {
		count = -EFAULT;
		goto end;
	}

	*offset += count;
end:
	mutex_unlock(&dplog_lock);
	return count;
}

#if KERNEL_VERSION(5, 6, 0) <= LINUX_VERSION_CODE
static const struct proc_ops secdp_logger_ops = {
	.proc_read = secdp_logger_read,
	.proc_lseek = default_llseek,
};
#else
static const struct file_operations secdp_logger_ops = {
	.owner = THIS_MODULE,
	.read = secdp_logger_read,
	.llseek = default_llseek,
};
#endif

int secdp_logger_init(void)
{
	struct proc_dir_entry *entry;

	if (is_secdp_logger_init)
		return 0;

	entry = proc_create(PROC_FILE_NAME, 0444, NULL, &secdp_logger_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, BUF_SIZE);
	is_secdp_logger_init = 1;
	g_entry = entry;

	mutex_init(&dplog_lock);
	secdp_logger_print("dp logger init ok\n");
	return 0;
}

void secdp_logger_deinit(void)
{
	if (!g_entry)
		return;

	proc_remove(g_entry);
	is_secdp_logger_init = 0;
	g_entry = NULL;

	mutex_destroy(&dplog_lock);
	secdp_logger_print("dp logger deinit ok\n");
}
