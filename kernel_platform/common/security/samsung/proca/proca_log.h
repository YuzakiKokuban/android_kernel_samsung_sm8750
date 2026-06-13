/* SPDX-License-Identifier: GPL-2.0 */
/*
 * PROCA logging definitions
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Hryhorii Tur, <hryhorii.tur@partner.samsung.com>
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

#ifndef _LINUX_PROCA_LOG_H
#define _LINUX_PROCA_LOG_H

#ifdef CONFIG_PROCA_DEBUG
#define PROCA_DEBUG_LOG(msg, ...) pr_info("PROCA: "msg, ##__VA_ARGS__)
#else
#define PROCA_DEBUG_LOG(msg, ...)
#endif

#define PROCA_ERROR_LOG(msg, ...) pr_err("PROCA: "msg, ##__VA_ARGS__)

#define PROCA_INFO_LOG(msg, ...) pr_info("PROCA: "msg, ##__VA_ARGS__)

#define PROCA_WARN_LOG(msg, ...) pr_warn("PROCA: "msg, ##__VA_ARGS__)

#ifdef CONFIG_PROCA_DEBUG
#define PROCA_BUG() \
	do { \
		BUG(); \
	} while (0)
#else
#define PROCA_BUG() \
	do { \
		PROCA_ERROR_LOG("BUG detected in function %s at %s:%d\n", \
				__func__, __FILE__, __LINE__); \
		return -EINVAL; \
	} while (0)
#endif

#define PROCA_BUG_ON(cond) \
	do { \
		if (unlikely(cond)) \
			PROCA_BUG(); \
	} while (0)

#endif /* _LINUX_PROCA_LOG_H */

