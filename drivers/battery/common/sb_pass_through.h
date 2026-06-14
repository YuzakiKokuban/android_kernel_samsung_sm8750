/*
 * sb_tx.h
 * Samsung Mobile Wireless TX Header
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

#ifndef __SB_PASS_THROUGH_H
#define __SB_PASS_THROUGH_H __FILE__

#include <linux/err.h>

enum pass_through_mode {
	PTM_NONE = 0,
	PTM_1TO1,
	PTM_2TO1,
	PTM_3TO1,
};

struct sb_pt;
struct device;
enum power_supply_property;
union power_supply_propval;

struct sb_pt *sb_pt_init(struct device *parent);
int sb_pt_psy_set_property(struct sb_pt *pt, enum power_supply_property psp, const union power_supply_propval *value);
int sb_pt_psy_get_property(struct sb_pt *pt, enum power_supply_property psp, union power_supply_propval *value);
int sb_pt_monitor(struct sb_pt *pt, int chg_src);
void sb_pt_change_dc_ic_src(struct sb_pt *pt, char *direct_charger_name);

int sb_pt_check_chg_src(struct sb_pt *pt, int chg_src);

#endif /* __SB_PASS_THROUGH_H */

