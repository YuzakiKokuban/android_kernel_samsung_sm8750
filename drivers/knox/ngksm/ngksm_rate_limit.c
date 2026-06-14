/*
 * Copyright (c) 2018-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/ngksm.h>
#include <linux/ratelimit.h>
#include "ngksm_rate_limit.h"
#include "ngksm_common.h"

#define MAX_MESSAGES_PER_SEC (30)


int ngksm_check_message_rate_limit(void)
{
	static DEFINE_RATELIMIT_STATE(ngksm_ratelimit_state, 1 * HZ, MAX_MESSAGES_PER_SEC);

	if (__ratelimit(&ngksm_ratelimit_state))
		return NGKSM_SUCCESS;

	return -EBUSY;

}
