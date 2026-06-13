/*
 * sb_batt_dump.h
 * Samsung Mobile Battery Dump Header
 *
 * Copyright (C) 2021 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SB_BATT_DUMP_H
#define __SB_BATT_DUMP_H __FILE__

#include <linux/err.h>

struct bd_pt;
struct device;
enum power_supply_property;
union power_supply_propval;

#if defined(CONFIG_BATTERY_LOGGING)
int sb_bd_init(void);
#else
static inline int sb_bd_init(void)
{ return 0; }
#endif

#endif /* __SB_BATT_DUMP_H */

