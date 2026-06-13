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
#ifndef _SECDP_LOGGER_H_
#define _SECDP_LOGGER_H_

#ifdef CONFIG_SECDP_LOGGER

void secdp_logger_set_max_count(int count);
void secdp_logger_print(const char *fmt, ...);
void secdp_logger_hex_dump(void *buf, void *pref, size_t len);
int secdp_logger_init(void);
void secdp_logger_deinit(void);

void secdp_logger_set_mode_max_count(unsigned int count);
void secdp_logger_dec_mode_count(void);
void secdp_logger_print_mode(const char *fmt, ...);

#define secdp_pr_debug(fmt, ...) \
	printk(KERN_DEBUG "[msm-dp] %s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)

#define secdp_proc_err(fmt, ...) \
	do { \
		printk(KERN_ERR "[msm-dp] %s: " pr_fmt(fmt), __func__, ##__VA_ARGS__); \
		secdp_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)

#define secdp_proc_info(fmt, ...) \
	do { \
		printk(KERN_INFO "[msm-dp] %s: " pr_fmt(fmt), __func__, ##__VA_ARGS__); \
		secdp_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)

#define secdp_proc_warn(fmt, ...) \
	do { \
		printk(KERN_WARNING "[msm-dp] %s: " pr_fmt(fmt), __func__, ##__VA_ARGS__); \
		secdp_logger_print(fmt, ##__VA_ARGS__); \
	} while (0)

#define secdp_proc_info_mode(fmt, ...) \
	do { \
		printk(KERN_INFO "[msm-dp] %s: " pr_fmt(fmt), __func__, ##__VA_ARGS__); \
		secdp_logger_print_mode(fmt, ##__VA_ARGS__); \
	} while (0)

#define DP_INFO_M(fmt, ...)  secdp_proc_info_mode(fmt, ##__VA_ARGS__)

#ifdef pr_debug
#undef pr_debug
#define pr_debug secdp_pr_debug
#endif

#ifdef pr_err
#undef pr_err
#define pr_err	secdp_proc_err
#endif

#ifdef pr_info
#undef pr_info
#define pr_info	secdp_proc_info
#endif

#ifdef pr_warn
#undef pr_warn
#define pr_warn	secdp_proc_warn
#endif

#else/* !CONFIG_SECDP_LOGGER */

#define secdp_logger_set_max_count(x)		do {} while (0)
#define secdp_logger_print(fmt, ...)		do {} while (0)
#define secdp_logger_hex_dump(buf, pref, len)	do {} while (0)
#define secdp_logger_init(void)			do {} while (0)

#ifdef pr_debug
#undef pr_debug
#define pr_debug	pr_info
#endif

#endif


#endif/*_SECDP_LOGGER_H_*/
