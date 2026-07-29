/* SPDX-License-Identifier: GPL-2.0 */

/* Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _SEA_NETLINK_H
#define _SEA_NETLINK_H

#define SEA_FAMILY "SEA Family"
#define SEA_GROUP "SEA Group"

#define FEATURE_CODE_LENGTH (9)
#define MAX_ALLOWED_DETAIL_LENGTH (1024)

// Creation of sea operation for the generic netlink communication
enum sea_operations {
	SEA_MSG_CMD,
};

// Creation of sea attributes ids for the sea netlink policy
enum sea_attribute_ids {
	/* Numbering must start from 1 */
	SEA_VALUE = 1,
	SEA_FEATURE_CODE,
	SEA_DETAIL,
	SEA_DAEMON_READY,
	SEA_ATTR_COUNT,
#define SEA_ATTR_MAX (SEA_ATTR_COUNT - 1)
};

int __init sea_netlink_init(void);
void __exit sea_netlink_exit(void);
int sea_daemon_ready(void);
int sea_send_netlink_message(const char *feature_code, const char *detail,
			     int64_t value);
int sea_check_message_rate_limit(void);
noinline int sea_send_message(const char *feature_code, const char *detail,
			      int64_t value);

#endif /* _SEA_NETLINK_H */
