/*
 * Copyright (c) 2018-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include "ngksm_netlink.h"
#include "ngksm_rate_limit.h"
#include "ngksm_common.h"
#include "ngk_hypervisor_detector.h"

static int is_ngksm_initialized;

int ngksm_log_level = NGKSM_LOG_LEVEL;

void ngksm_printk(int level, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (ngksm_log_level < level)
		return;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	printk(KERN_INFO "[%s] %pV", TAG, &vaf);

	va_end(args);
}

int ngksm_is_initialized(void)
{
	return (is_ngksm_initialized == true) ? true : false;
}

static int __init ngksm_init(void)
{
	int ret = NGKSM_SUCCESS;

	NGKSM_LOG_DEBUG("Init");


	ret = ngk_hyp_detector_init();
	if (ret != NGKSM_SUCCESS)
		NGKSM_LOG_ERROR("hypervisor_detector_init failed");

	ret = ngksm_netlink_init();
	if (ret != NGKSM_SUCCESS)
		goto exit_ret;
	is_ngksm_initialized = true;
exit_ret:
	if (ret == NGKSM_SUCCESS)
		NGKSM_LOG_INFO("Init done");
	else
		NGKSM_LOG_ERROR("Failed to Init.");
	return ret;
}

static void __exit ngksm_exit(void)
{
	if (ngksm_is_initialized()) {
		is_ngksm_initialized = false;
		ngksm_netlink_exit();
		ngk_hyp_detector_exit();
	}
	NGKSM_LOG_INFO("Exited.");
}

module_init(ngksm_init);
module_exit(ngksm_exit);

MODULE_AUTHOR("Taeho Kim <taeho81.kim@samsung.com>");
MODULE_AUTHOR("Shinjae Lee <shinjae1.lee@samsung.com>");
MODULE_DESCRIPTION("NGKSM driver");
MODULE_LICENSE("GPL");
