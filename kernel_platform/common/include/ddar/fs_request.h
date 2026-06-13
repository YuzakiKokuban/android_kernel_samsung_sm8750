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

#ifndef FS_REQUEST_H_
#define FS_REQUEST_H_

#include <linux/slab.h>

#define FSOP_AUDIT_FAIL_ENCRYPT		51
#define FSOP_AUDIT_FAIL_DECRYPT		52
#define FSOP_AUDIT_FAIL_ACCESS		53
#define FSOP_AUDIT_FAIL_DE_ACCESS	54

// opcode, ret, inode
typedef void (*fs_request_cb_t)(int, int, unsigned long);

struct sdp_fs_command_t {
	int req_id;

	int opcode;
	int user_id;
	int part_id;
	unsigned long ino;
	int pid;
	int err;
};

int sdp_fs_request(struct sdp_fs_command_t *sdp_req, fs_request_cb_t callback);

static inline struct sdp_fs_command_t *sdp_fs_command_alloc(int opcode, int pid,
		int userid, int partid, unsigned long ino, int err, gfp_t gfp) {
	struct sdp_fs_command_t *cmd;

	cmd = kmalloc(sizeof(struct sdp_fs_command_t), gfp);
	if (cmd == NULL)
		return NULL;

	cmd->opcode = opcode;
	cmd->pid = pid;
	cmd->user_id = userid;
	cmd->part_id = partid;
	cmd->ino = ino;
	cmd->err = err;

	return cmd;
}

static inline void sdp_fs_command_free(struct sdp_fs_command_t *cmd)
{
	kfree_sensitive(cmd);
}

#endif /* FS_REQUEST_H_ */
