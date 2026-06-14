/*
 * battery_logger.h
 * Samsung Mobile Charger Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __BATTERY_LOGGER_H
#define __BATTERY_LOGGER_H __FILE__

#if defined(CONFIG_BATTERY_LOGGING)
extern void store_battery_log(const char *fmt, ...);
extern int register_batterylog_proc(void);
extern void unregister_batterylog_proc(void);
#else
static inline void store_battery_log(const char *fmt, ...) {}
static inline int register_batterylog_proc(void)
{ return 0; }
static inline void unregister_batterylog_proc(void) {}
#endif

#endif /* __BATTERY_LOGGER_H */
