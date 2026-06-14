/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 Samsung Electronics Co., Ltd
 *		http://www.samsung.com
 *
 */
#ifndef KZT_H
#define KZT_H

#include <linux/fs.h>

#define KZT_ERR     1
#define KZT_INFO    2
#define KZT_DEBUG   3
#define KZT_VERBOSE 4

#define DEFAULT_KZT_DEBUG_LEVEL KZT_DEBUG

extern unsigned int kzt_debug_level;
void kzt_msg(const int level, const char *fmt, ...);

#define kzt_err(fmt, ...)		\
	kzt_msg(KZT_ERR, fmt, ##__VA_ARGS__)

#define kzt_info(fmt, ...)		\
	kzt_msg(KZT_INFO, fmt, ##__VA_ARGS__)

#define kzt_debug(fmt, ...)		\
	kzt_msg(KZT_DEBUG, fmt, ##__VA_ARGS__)

struct kzt_get_offsets_arg;

long kzt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

int kzt_offset_get_offsets(struct kzt_get_offsets_arg *arg);

#endif /* End of KZT_H */
