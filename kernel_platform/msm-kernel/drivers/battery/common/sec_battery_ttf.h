/*
 * sec_battery.h
 * Samsung Mobile Battery Header
 *
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __SEC_BATTERY_TTF_H
#define __SEC_BATTERY_TTF_H __FILE__

struct sec_cv_slope {
		int fg_current;
		int soc;
		int time;
};

struct sec_battery_info;

struct ttf_charge_current {
    unsigned int hv_12v;
    unsigned int hv;
    unsigned int hv_wireless;
    unsigned int wireless;
    unsigned int dc25;
    unsigned int dc45;
    unsigned int wc20_wireless;
    unsigned int wc21_wireless;
    unsigned int fpdo_dc;
};

struct sec_ttf_data {
	void *pdev;
	int timetofull;
	int old_timetofull;

	struct ttf_charge_current currents;

	struct sec_cv_slope *cv_data;
	int cv_data_length;
	unsigned int ttf_capacity;

	struct delayed_work timetofull_work;
};

int sec_calc_ttf(struct sec_battery_info * battery, unsigned int ttf_curr);
extern void sec_bat_calc_time_to_full(struct sec_battery_info * battery);
int sec_get_ttf_standard_curr(struct sec_battery_info *battery);
extern void sec_bat_time_to_full_work(struct work_struct *work);
extern void ttf_init(struct sec_battery_info *battery);
extern void ttf_work_start(struct sec_battery_info *battery);
extern int ttf_display(unsigned int capacity, int bat_sts, int thermal_zone, int time);
#ifdef CONFIG_OF
int sec_ttf_parse_dt(struct sec_battery_info *battery);
#endif

#endif /* __SEC_BATTERY_H */
