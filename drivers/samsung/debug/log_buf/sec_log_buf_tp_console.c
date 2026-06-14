// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>
#include <linux/sched/clock.h>
#include <linux/spinlock.h>

#include <trace/events/printk.h>

#include "sec_log_buf.h"

static __always_inline size_t __trace_console_print_prefix(char *buf, size_t sz_buf)
{
	size_t len = 0;
	u64 ts_nsec = local_clock();	/* NOTE: may have a skew... */

	len += __log_buf_print_time(ts_nsec, buf + len);
	len += __log_buf_print_process(smp_processor_id(), buf + len, sz_buf - len);

	return len;
}

#define PREFIX_MAX		64

static __always_inline void __trace_console_locked(void *unused,
		const char *text, size_t len)
{
	static char last_char = '\n';
	size_t text_len;

	if (!__log_buf_is_acceptable(text, len))
		return;

	if (unlikely(len == 0 || text[0] == '\0'))
		goto skip;

	if (likely(last_char == '\n')) {
		char prefix[PREFIX_MAX];
		size_t prefix_len;

		prefix_len = __trace_console_print_prefix(prefix, sizeof(prefix));
		__log_buf_write(prefix, prefix_len);
	}
	__log_buf_write(text, len);

skip:
	text_len = len + strnlen(&text[len], 16);
	last_char = text[text_len - 1];
}

static void __trace_console(void *unused, const char *text, size_t len)
{
	static DEFINE_SPINLOCK(lock);
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	__trace_console_locked(unused, text, len);
	spin_unlock_irqrestore(&lock, flags);
}

static int log_buf_logger_tp_console_probe(struct log_buf_drvdata *drvdata)
{
	return register_trace_console(__trace_console, NULL);
}

static void log_buf_logger_tp_console_remove(struct log_buf_drvdata *drvdata)
{
	unregister_trace_console(__trace_console, NULL);
}

static const struct log_buf_logger log_buf_tp_console = {
	.probe = log_buf_logger_tp_console_probe,
	.remove = log_buf_logger_tp_console_remove,
};

const struct log_buf_logger *__log_buf_logger_tp_console_creator(void)
{
	return &log_buf_tp_console;
}
