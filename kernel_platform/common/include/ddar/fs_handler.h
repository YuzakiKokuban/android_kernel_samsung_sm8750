/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DDAR_FS_HANDLER_H
#define _DDAR_FS_HANDLER_H

#include <ddar/fs_request.h>

#include <linux/list.h>
#include <linux/spinlock.h>

#define SDP_FS_HANDLER_NETLINK 28
#define SDP_FS_HANDLER_PID_SET 3001
#define SDP_FS_HANDLER_RESULT 3002

struct result_t {
	u32 request_id;
	u8 opcode;
	s16 ret;
};

/** The request state */
enum req_state {
	SDP_FS_HANDLER_REQ_INIT = 0,
	SDP_FS_HANDLER_REQ_PENDING,
	SDP_FS_HANDLER_REQ_FINISHED
};

struct sdp_fs_handler_control_t {
	struct list_head pending_list;
	spinlock_t lock;

	/** The next unique request id */
	u32 reqctr;
};

struct sdp_fs_handler_request_t {
	u32 id;
	u8 opcode;

	struct list_head list;
	/** refcount */
	atomic_t count;

	enum req_state state;

	struct sdp_fs_command_t command;
	struct result_t result;

	fs_request_cb_t callback;

	/** The request was aborted */
	u8 aborted;
};
#endif
