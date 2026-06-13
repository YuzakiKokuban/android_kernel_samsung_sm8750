// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2010-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_log_buf.h"

void sec_log_buf_store_on_vprintk_emit(void)
{
	if (!__log_buf_is_probed())
		return;

	if (sec_log_buf->strategy != SEC_LOG_BUF_STRATEGY_BUILTIN)
		return;

	__log_buf_store_from_kmsg_dumper();
}

static int log_buf_logger_builtin_probe(struct log_buf_drvdata *drvdata)
{
	return 0;
}

static void log_buf_logger_builtin_remove(struct log_buf_drvdata *drvdata)
{
}

static const struct log_buf_logger log_buf_logger_builtin = {
	.probe = log_buf_logger_builtin_probe,
	.remove = log_buf_logger_builtin_remove,
};

const struct log_buf_logger *__log_buf_logger_builtin_creator(void)
{
	if (IS_BUILTIN(CONFIG_SEC_LOG_BUF))
		return &log_buf_logger_builtin;
	else
		return ERR_PTR(-ENODEV);
}
