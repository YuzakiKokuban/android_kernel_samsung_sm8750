// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2010-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_log_buf.h"

static void sec_log_buf_write_console(struct console *console, const char *s,
		unsigned int count)
{
	if (!__log_buf_is_acceptable(s, count))
		return;

	__log_buf_write(s, count);
}

static int log_buf_logger_console_probe(struct log_buf_drvdata *drvdata)
{
	struct console *con = &drvdata->con;

	strlcpy(con->name, "sec_log_buf", sizeof(con->name));
	con->write = sec_log_buf_write_console;
	/* NOTE: CON_PRINTBUFFER is ommitted.
	 * I use __log_buf_pull_early_buffer instead of it.
	 */
	con->flags = CON_ENABLED | CON_ANYTIME;
	con->index = -1;

	register_console(con);

	return 0;
}

static void log_buf_logger_console_remove(struct log_buf_drvdata *drvdata)
{
	struct console *con = &drvdata->con;

	unregister_console(con);
}

static const struct log_buf_logger log_buf_logger_console = {
	.probe = log_buf_logger_console_probe,
	.remove = log_buf_logger_console_remove,
};

const struct log_buf_logger *__log_buf_logger_console_creator(void)
{
	return &log_buf_logger_console;
}
