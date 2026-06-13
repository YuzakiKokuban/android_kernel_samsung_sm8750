/* SPDX-License-Identifier: GPL-2.0 */
/*
 * PROCA fcntl implementation
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 * Anton Voloshchuk, <a.voloshchuk@samsung.com>
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

#ifndef __LINUX_FIVE_TESTING_H
#define __LINUX_FIVE_TESTING_H

#if defined(FIVE_KUNIT_ENABLED) || defined(PROCA_KUNIT_ENABLED)
#define KUNIT_UML // this define should be used for adding UML-specific modifications
#define __mockable __weak
#define __visible_for_testing
/* <for five_dsms.c> dsms_send_message stub. Never called.
 * To isolate FIVE from DSMS during KUnit testing
 */
static inline int dsms_send_message(const char *feature_code,
					const char *detail,
					int64_t value)
{ return 1; }
// <for five_main.c: five_ptrace(...)>
#ifdef KUNIT_UML
#define COMPAT_PTRACE_GETREGS		12
#define COMPAT_PTRACE_GET_THREAD_AREA	22
#define COMPAT_PTRACE_GETVFPREGS	27
#define COMPAT_PTRACE_GETHBPREGS	29
#endif
#else
#define __mockable
#define __visible_for_testing static
#endif // FIVE_KUNIT_ENABLED || PROCA_KUNIT_ENABLED

#endif // __LINUX_FIVE_TESTING_H
