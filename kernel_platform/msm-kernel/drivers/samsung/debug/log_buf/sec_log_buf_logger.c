// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2010-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_log_buf.h"

static const struct log_buf_logger *
		__log_buf_logger_creator(struct log_buf_drvdata *drvdata)
{
	struct device *dev = drvdata->bd.dev;
	const struct log_buf_logger *logger = NULL;
	unsigned int strategy = drvdata->strategy;

	switch (strategy) {
	case SEC_LOG_BUF_STRATEGY_BUILTIN:
		logger = __log_buf_logger_builtin_creator();
		break;
	case SEC_LOG_BUF_STRATEGY_TP_CONSOLE:
		logger = __log_buf_logger_tp_console_creator();
		break;
	case SEC_LOG_BUF_STRATEGY_KPROBE:
		logger = __log_buf_logger_kprobe_creator();
		break;
	case SEC_LOG_BUF_STRATEGY_CONSOLE:
		logger = __log_buf_logger_console_creator();
		break;
	case SEC_LOG_BUF_STRATEGY_VH_LOGBUF:
		logger = __log_buf_logger_vh_logbuf_creator();
		break;
	}

	if (IS_ERR_OR_NULL(logger)) {
		dev_warn(dev, "%u is not supported or deprecated. use default\n",
				strategy);
		logger = __log_buf_logger_console_creator();
	}

	return logger;
}

static void __log_buf_logger_factory(struct log_buf_drvdata *drvdata)
{
	drvdata->logger = __log_buf_logger_creator(drvdata);
}

int __log_buf_logger_init(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);

	__log_buf_logger_factory(drvdata);

	return drvdata->logger->probe(drvdata);
}

void __log_buf_logger_exit(struct builder *bd)
{
	struct log_buf_drvdata *drvdata =
			container_of(bd, struct log_buf_drvdata, bd);

	drvdata->logger->remove(drvdata);
}
