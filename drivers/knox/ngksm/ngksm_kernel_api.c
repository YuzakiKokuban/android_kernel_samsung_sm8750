/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/ngksm.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "ngksm_netlink.h"
#include "ngksm_rate_limit.h"
#include "ngksm_common.h"

noinline int ngksm_send_message(const char *feature_code,
		const char *detail,
		int64_t value)
{
	int ret;
	size_t len;

	if (!ngksm_daemon_ready()) {
		ret = -EPERM;
		goto exit_send;
	}

	if (!ngksm_is_initialized()) {
		NGKSM_LOG_ERROR("NGKSM not initialized.");
		ret = -EACCES;
		goto exit_send;
	}

	if (!feature_code) {
		NGKSM_LOG_ERROR("Invalid feature code.");
		ret = -EINVAL;
		goto exit_send;
	}

	if (!detail)
		detail = "";

	len = strnlen(detail, MAX_ALLOWED_DETAIL_LENGTH);

	ret = ngksm_check_message_rate_limit();
	if (ret != NGKSM_SUCCESS)
		goto exit_send;

	ret = ngksm_send_netlink_message(feature_code, detail, value);
	if (ret == NGKSM_SUCCESS) {
			NGKSM_LOG_DEBUG("send netlink message success - {'%s', '%s' (%zu bytes), %lld}",
				feature_code, detail, len, value);
	}

exit_send:
	return ret;
}
