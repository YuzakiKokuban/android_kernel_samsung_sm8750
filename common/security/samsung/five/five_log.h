/* SPDX-License-Identifier: GPL-2.0 */
/*
 * FIVE logging definitions
 *
 * Copyright (C) 2024 Samsung Electronics, Inc.
 * Oleksandr Stanislavskyi, <o.stanislavs@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LINUX_FIVE_LOG_H
#define _LINUX_FIVE_LOG_H

#ifdef CONFIG_FIVE_DEBUG
#define FIVE_DEBUG_LOG(msg, ...) pr_info("FIVE: "msg, ##__VA_ARGS__)
#else
#define FIVE_DEBUG_LOG(msg, ...)
#endif

#define FIVE_ERROR_LOG(msg, ...) pr_err("FIVE: "msg, ##__VA_ARGS__)

#define FIVE_INFO_LOG(msg, ...) pr_info("FIVE: "msg, ##__VA_ARGS__)

#define FIVE_WARN_LOG(msg, ...) pr_warn("FIVE: "msg, ##__VA_ARGS__)

#ifdef CONFIG_FIVE_DEBUG
#define FIVE_BUG() \
	do { \
		BUG(); \
	} while (0)
#else
#define FIVE_BUG() \
	do { \
		FIVE_ERROR_LOG("BUG detected in function %s at %s:%d\n", \
				__func__, __FILE__, __LINE__); \
		return -EINVAL; \
	} while (0)
#endif

#define FIVE_BUG_ON(cond) \
	do { \
		if (unlikely(cond)) \
			FIVE_BUG(); \
	} while (0)

#endif /* _LINUX_FIVE_LOG_H */

