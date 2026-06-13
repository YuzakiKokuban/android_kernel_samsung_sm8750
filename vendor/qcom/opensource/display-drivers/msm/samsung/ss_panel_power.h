/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 * Copyright (C) 2023, Samsung Electronics. All rights reserved.
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

#ifndef SS_PANEL_POWER_H
#define SS_PANEL_POWER_H

#include "ss_dsi_panel_common.h"

static inline bool ss_is_pmic_type(enum panel_pwr_type type)
{
	if ((type == PANEL_PWR_PMIC_SPECIFIC) || (type == PANEL_PWR_PMIC))
		return true;
	else
		return false;
}

int ss_panel_power_force_disable(struct samsung_display_driver_data *vdd,
			struct panel_power *power);
int ss_panel_power_ctrl(struct samsung_display_driver_data *vdd,
			struct panel_power *power, bool enable);
int ss_panel_power_on(struct samsung_display_driver_data *vdd,
			struct panel_power *power);
int ss_panel_power_on_pre_lp11(struct samsung_display_driver_data *vdd);
int ss_panel_power_on_post_lp11(struct samsung_display_driver_data *vdd);
int ss_panel_power_on_middle_lp_hs_clk(void);

int ss_panel_power_off_pre_lp11(struct samsung_display_driver_data *vdd);
int ss_panel_power_off_post_lp11(struct samsung_display_driver_data *vdd);
int ss_panel_power_off(struct samsung_display_driver_data *vdd,
			struct panel_power *power);
int ss_panel_power_off_middle_lp_hs_clk(void);

int ss_panel_parse_powers(struct samsung_display_driver_data *vdd,
		struct device_node *panel_node, struct device *dev);
int ss_panel_power_pmic_vote(struct samsung_display_driver_data *vdd, bool vote_up);
#endif /* SS_PANEL_POWER_H */
