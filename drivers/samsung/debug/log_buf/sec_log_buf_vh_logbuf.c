// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2021-2024 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include <printk_ringbuffer.h>
#include <trace/hooks/logbuf.h>

#include "sec_log_buf.h"

static size_t print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec = do_div(ts, 1000000000);

	return sprintf(buf, "\n[%5lu.%06lu]",
		       (unsigned long)ts, rem_nsec / 1000);
}

static size_t print_process(char *buf)
{
	return sprintf(buf, "%c[%1d:%15s:%5d] ",
			in_interrupt() ? 'I' : ' ',
			smp_processor_id(),
			current->comm,
			task_pid_nr(current));
}

static size_t info_print_prefix(const struct printk_info *info, char *buf)
{
	size_t len = 0;

	len += print_time(info->ts_nsec, buf + len);
	len += print_process(buf + len);

	return len;
}

#define PREFIX_MAX		64

static void __trace_android_vh_logbuf(void *unused,
		struct printk_ringbuffer *rb, struct printk_record *r)
{
	size_t text_len = r->info->text_len;
	size_t buf_size = r->text_buf_size;
	char *text = r->text_buf;
	char prefix[PREFIX_MAX];
	size_t prefix_len;

	if (text_len > buf_size)
		text_len = buf_size;

	if (!__log_buf_is_acceptable(text, text_len))
		return;

	prefix_len = info_print_prefix(r->info, prefix);
	__log_buf_write(prefix, prefix_len);
	__log_buf_write(text, text_len);
}

static void __trace_android_vh_logbuf_pr_cont(void *unused,
		struct printk_record *r, size_t text_len)
{
	size_t offset = r->info->text_len - text_len;
	char *text = &r->text_buf[offset];

	if (!__log_buf_is_acceptable(text, text_len))
		return;

	__log_buf_write(text, text_len);
}

static int log_buf_logger_vh_logbuf_probe(struct log_buf_drvdata *drvdata)
{
	int err;

	err = register_trace_android_vh_logbuf(__trace_android_vh_logbuf,
			NULL);
	if (err)
		return err;

	return register_trace_android_vh_logbuf_pr_cont(
			__trace_android_vh_logbuf_pr_cont, NULL);
}

static void log_buf_logger_vh_logbuf_remove(struct log_buf_drvdata *drvdata)
{
	unregister_trace_android_vh_logbuf(__trace_android_vh_logbuf, NULL);
	unregister_trace_android_vh_logbuf_pr_cont(
			__trace_android_vh_logbuf_pr_cont, NULL);
}

static const struct log_buf_logger log_buf_logger_vh_logbuf = {
	.probe = log_buf_logger_vh_logbuf_probe,
	.remove = log_buf_logger_vh_logbuf_remove,
};

const struct log_buf_logger *__log_buf_logger_vh_logbuf_creator(void)
{
	return &log_buf_logger_vh_logbuf;
}
