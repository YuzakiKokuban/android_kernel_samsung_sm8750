/*
 * Copyright (c) 2020-2021 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _NGKSM_NETLINK_H
#define _NGKSM_NETLINK_H

#define NGKSM_FAMILY "NGKSM Family"
#define NGKSM_GROUP "NGKSM Group"

#define FEATURE_CODE_LENGTH        (9)
#define MAX_ALLOWED_DETAIL_LENGTH  (1024)



// Creation of ngksm operation for the generic netlink communication
enum ngksm_operations {
	NGKSM_MSG_CMD,
};

// Creation of ngksm attributes ids for the ngksm netlink policy
enum ngksm_attribute_ids {
	/* Numbering must start from 1 */
	NGKSM_VALUE = 1,
	NGKSM_FEATURE_CODE,
	NGKSM_DETAIL,
	NGKSM_DAEMON_READY,
	NGKSM_ATTR_COUNT,
#define NGKSM_ATTR_MAX (NGKSM_ATTR_COUNT - 1)
};

extern int __init ngksm_netlink_init(void);
extern void __exit ngksm_netlink_exit(void);
extern int ngksm_daemon_ready(void);
extern int ngksm_send_netlink_message(const char *feature_code,
				     const char *detail,
				     int64_t value);

#endif /* _NGKSM_NETLINK_H */
