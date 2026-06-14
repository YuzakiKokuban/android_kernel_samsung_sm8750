/*
 * sb_checklist_app.h
 * Samsung Mobile
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
#ifndef __SB_CHECLIST_APP_H
#define __SB_CHECLIST_APP_H __FILE__

#define STEP_CHARGING_CONDITION_VOLTAGE			0x01
#define STEP_CHARGING_CONDITION_SOC				0x02
#define STEP_CHARGING_CONDITION_CHARGE_POWER	0x04
#define STEP_CHARGING_CONDITION_ONLINE			0x08
#define STEP_CHARGING_CONDITION_CURRENT_NOW		0x10
#define STEP_CHARGING_CONDITION_FLOAT_VOLTAGE	0x20
#define STEP_CHARGING_CONDITION_INPUT_CURRENT		0x40
#define STEP_CHARGING_CONDITION_SOC_INIT_ONLY		0x80 /* use this to consider SOC to decide starting step only */

#define STEP_CHARGING_CONDITION_DC_INIT		(STEP_CHARGING_CONDITION_VOLTAGE |\
									STEP_CHARGING_CONDITION_SOC |\
									STEP_CHARGING_CONDITION_SOC_INIT_ONLY)
//need to update above values if sec_step_charging.c file update
#define MAX_STEP_CHG_STEP			3
//wire menu
#define OVERHEATLIMIT_THRESH        0
#define OVERHEATLIMIT_RECOVERY      1
#define WIRE_WARM_OVERHEAT_THRESH   2
#define WIRE_NORMAL_WARM_THRESH     3
#define WIRE_COOL1_NORMAL_THRESH    4
#define WIRE_COOL2_COOL1_THRESH     5
#define WIRE_COOL3_COOL2_THRESH     6
#define WIRE_COLD_COOL3_THRESH      7
#define WIRE_WARM_CURRENT           8
#define WIRE_COOL1_CURRENT          9
#define WIRE_COOL2_CURRENT          10
#define WIRE_COOL3_CURRENT          11

//wireless menu
#define OVERHEATLIMIT_THRESH                    0
#define OVERHEATLIMIT_RECOVERY                  1
#define WIRELESS_WARM_OVERHEAT_THRESH           2
#define WIRELESS_NORMAL_WARM_THRESH             3
#define WIRELESS_COOL1_NORMAL_THRESH            4
#define WIRELESS_COOL2_COOL1_THRESH             5
#define WIRELESS_COOL3_COOL2_THRESH             6
#define WIRELESS_COLD_COOL3_THRESH              7
#define WIRELESS_WARM_CURRENT                   8
#define WIRELESS_COOL1_CURRENT                  9
#define WIRELESS_COOL2_CURRENT                  10
#define WIRELESS_COOL3_CURRENT                  11
#define TX_HIGH_THRESHOLD                       12
#define TX_HIGH_THRESHOLD_RECOVERY              13
#define TX_LOW_THRESHOLD                        14
#define TX_LOW_THRESHOLD_RECOVERY               15

//dchg step charging
#define DCHG_STEP_CHG_COND_VOL              1
#define DCHG_STEP_CHG_COND_SOC              2
#define DCHG_STEP_CHG_VAL_VFLOAT            3
#define DCHG_STEP_CHG_VAL_IOUT              4
//step charging menu
#define STEP_CHG_COND                       6
#define STEP_CHG_COND_CURR                  7
#define STEP_CHG_CURR                       8
#define STEP_CHG_VFLOAT                     9

//others menu: mix temp, chg temp, dchg temp
#define CHG_HIGH_TEMP                       0
#define CHG_HIGH_TEMP_RECOVERY              1
#define CHG_INPUT_LIMIT_CURRENT             2
#define CHG_CHARGING_LIMIT_CURRENT          3
#define MIX_HIGH_TEMP                       4
#define MIX_HIGH_CHG_TEMP                   5
#define MIX_HIGH_TEMP_RECOVERY              6
#define DCHG_HIGH_TEMP                      7
#define DCHG_HIGH_TEMP_RECOVERY             8
#define DCHG_HIGH_BATT_TEMP                 9
#define DCHG_HIGH_BATT_TEMP_RECOVERY        10
#define DCHG_INPUT_LIMIT_CURRENT            11
#define DCHG_CHARGING_BATT_TEMP_RECOVERY    12

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
extern void sb_ca_init(struct device *parent);
#else
static inline void sb_ca_init(struct device *parent) {}
#endif
#endif	/* __SB_CHECLIST_APP_H */
