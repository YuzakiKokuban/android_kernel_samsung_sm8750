/*
 * sb_full_soc.h
 * Samsung Mobile Battery Full SoC Header
 *
 * Copyright (C) 2023 Samsung Electronics, Inc.
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

#ifndef __SB_FULL_SOC_H
#define __SB_FULL_SOC_H __FILE__

struct sec_battery_info;
struct sb_full_soc;

int get_full_capacity(struct sb_full_soc *fs);
bool is_full_capacity(struct sb_full_soc *fs);
bool is_eu_eco_rechg(struct sb_full_soc *fs);

bool check_eu_eco_full_status(struct sec_battery_info *battery);
bool check_eu_eco_rechg_ui_condition(struct sec_battery_info *battery);
void sec_bat_recov_full_capacity(struct sec_battery_info *battery);
void sec_bat_check_full_capacity(struct sec_battery_info *battery);

int sb_full_soc_init(struct sec_battery_info *battery);

#endif /* __SB_FULL_SOC_H */
