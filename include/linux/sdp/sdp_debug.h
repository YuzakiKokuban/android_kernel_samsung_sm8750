// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SDP_DEBUG_H__
#define __SDP_DEBUG_H__

#define SDP_LOG_PREFIX	"SDP"

enum {
	MAIN_DISP_ID = 0,
	SUB_DISP_ID = 1,
	UNKNOWN_DISP_ID = -1,
};

#define sdp_err(ctx, fmt, ...)						\
	pr_err(pr_fmt("[" SDP_LOG_PREFIX "_%d] %s:%d: error: " fmt),	\
			(ctx) ? sdp_get_disp_id(ctx) : UNKNOWN_DISP_ID,	\
			__func__, __LINE__, ##__VA_ARGS__)

#define sdp_info(ctx, fmt, ...)						\
	pr_info(pr_fmt("[" SDP_LOG_PREFIX "_%d] %s:%d: " fmt),	\
			(ctx) ? sdp_get_disp_id(ctx) : UNKNOWN_DISP_ID,	\
			__func__, __LINE__, ##__VA_ARGS__)

#define sdp_dbg(ctx, fmt, ...)						\
	pr_debug(pr_fmt("[" SDP_LOG_PREFIX "_%d] %s:%d: " fmt),	\
			(ctx) ? sdp_get_disp_id(ctx) : UNKNOWN_DISP_ID,	\
			__func__, __LINE__, ##__VA_ARGS__)

extern int sdp_get_disp_id(void *ctx);

#endif /* __SDP_DEBUG_H__ */
