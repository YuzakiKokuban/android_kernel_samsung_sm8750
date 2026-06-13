/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _SECDP_LOGGER_EX_H_
#define _SECDP_LOGGER_EX_H_

/*
 * if .c has its own pr_fmt, then use this instead of secdp_logger.h
 */
#ifdef CONFIG_SECDP_LOGGER
void secdp_logger_print(const char *fmt, ...);

#define secdp_proc_info_ex(fmt, ...) \
	do { \
		printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__); \
		secdp_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)

#define secdp_pr_info_ex(fmt, ...) \
		printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)

#ifdef pr_debug
#undef pr_debug
#define pr_debug secdp_pr_info_ex
#endif

#ifdef pr_err
#undef pr_err
#define pr_err	secdp_proc_info_ex
#endif

#ifdef pr_info
#undef pr_info
#define pr_info	secdp_proc_info_ex
#endif

#ifdef pr_warn
#undef pr_warn
#define pr_warn	secdp_proc_info_ex
#endif

#endif/*CONFIG_SECDP_LOGGER*/
#endif/*_SECDP_LOGGER_EX_H_*/
