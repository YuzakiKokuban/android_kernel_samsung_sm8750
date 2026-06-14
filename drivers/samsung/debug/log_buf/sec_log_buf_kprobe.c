// SPDX-License-Identifier: GPL-2.0
/*
 * COPYRIGHT(C) 2010-2023 Samsung Electronics Co., Ltd. All Right Reserved.
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s() " fmt, __func__

#include <linux/kernel.h>

#include "sec_log_buf.h"

static void sec_log_buf_post_handle_on_vprintk_emit(struct kprobe *probe,
		struct pt_regs *regs, unsigned long flags)
{
	__log_buf_store_from_kmsg_dumper();
}

static int log_buf_logger_kprobe_probe(struct log_buf_drvdata *drvdata)
{
	struct kprobe *kp = &drvdata->probe;
	int err;

	kp->symbol_name = "vprintk_emit";
	kp->post_handler = sec_log_buf_post_handle_on_vprintk_emit;

	err = register_kprobe(kp);
	if (err)
		goto err_failed_to_register;

	return 0;

err_failed_to_register:
	return err;
}

static void log_buf_logger_kprobe_remove(struct log_buf_drvdata *drvdata)
{
	struct kprobe *kp = &drvdata->probe;

	unregister_kprobe(kp);
}

static const struct log_buf_logger log_buf_logger_kprobe = {
	.probe = log_buf_logger_kprobe_probe,
	.remove = log_buf_logger_kprobe_remove,
};

const struct log_buf_logger *__log_buf_logger_kprobe_creator(void)
{
	return &log_buf_logger_kprobe;
}
