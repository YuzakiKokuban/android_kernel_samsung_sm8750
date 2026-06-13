/*
 * include/linux/battery/sb_wireless.h
 *
 * header file supporting samsung battery wireless information
 *
 * Copyright (C) 2023 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __SB_WIRELESS_H__
#define __SB_WIRELESS_H__

enum wpc_op_mode {
	WPC_OP_MODE_NONE = 0, /* AC MISSING */
	WPC_OP_MODE_BPP,
	WPC_OP_MODE_PPDE,
	WPC_OP_MODE_EPP,
	WPC_OP_MODE_MPP,
	WPC_OP_MODE_MAX
};

enum wpc_auth_mode {
	WPC_AUTH_MODE_NONE = 0,
	WPC_AUTH_MODE_BPP,
	WPC_AUTH_MODE_PPDE,
	WPC_AUTH_MODE_EPP,
	WPC_AUTH_MODE_MPP,
	WPC_AUTH_MODE_MAX
};

struct sb_wireless_op {
	int (*get_op_mode)(void *pdata);
	int (*get_qi_ver)(void *pdata);
	int (*get_auth_mode)(void *pdata);
};

const char *sb_wrl_op_mode_str(int op_mode);

int sb_wireless_set_op(void *pdata, const struct sb_wireless_op *op);

#endif /* __SB_WIRELESS_H__ */
