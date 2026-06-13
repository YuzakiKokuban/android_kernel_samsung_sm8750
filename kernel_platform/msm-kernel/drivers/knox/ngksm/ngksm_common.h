/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _NGKSM_COMMON_H
#define _NGKSM_COMMON_H

#include <linux/errno.h>
#include <linux/printk.h>


#define TAG				"ngksm"

void ngksm_printk(int level, const char *fmt, ...);

#define DEV_ERR				(1)
#define DEV_WARN			(2)
#define DEV_NOTI			(3)
#define DEV_INFO			(4)
#define DEV_DEBUG			(5)
#ifdef NGK_DEBUG
#define NGKSM_LOG_LEVEL		DEV_DEBUG
#else
#define NGKSM_LOG_LEVEL		DEV_INFO
#endif

#define NGKSM_LOG_ERROR(fmt, ...)	ngksm_printk(DEV_ERR, fmt, ## __VA_ARGS__)
#define NGKSM_LOG_WARN(fmt, ...)	ngksm_printk(DEV_WARN, fmt, ## __VA_ARGS__)
#define NGKSM_LOG_NOTI(fmt, ...)	ngksm_printk(DEV_NOTI, fmt, ## __VA_ARGS__)
#define NGKSM_LOG_INFO(fmt, ...)	ngksm_printk(DEV_INFO, fmt, ## __VA_ARGS__)
#define NGKSM_LOG_DEBUG(fmt, ...)	ngksm_printk(DEV_DEBUG, fmt, ## __VA_ARGS__)

#define NGKSM_SUCCESS (0)

extern int ngksm_is_initialized(void);

#endif /* _NGKSM_COMMON_H */
