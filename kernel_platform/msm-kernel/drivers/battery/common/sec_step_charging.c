/*
 *  sec_step_charging.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2018 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "sec_battery.h"

#define STEP_CHARGING_CONDITION_VOLTAGE			0x01
#define STEP_CHARGING_CONDITION_SOC				0x02
#define STEP_CHARGING_CONDITION_CHARGE_POWER 	0x04
#define STEP_CHARGING_CONDITION_ONLINE 			0x08
#define STEP_CHARGING_CONDITION_CURRENT_NOW		0x10
#define STEP_CHARGING_CONDITION_FLOAT_VOLTAGE	0x20
#define STEP_CHARGING_CONDITION_INPUT_CURRENT		0x40
#define STEP_CHARGING_CONDITION_SOC_INIT_ONLY		0x80 /* use this to consider SOC to decide starting step only */
#define STEP_CHARGING_CONDITION_FORCE_SOC		0x100
#define STEP_CHARGING_CONDITION_FG_CURRENT		0x200

#define STEP_CHARGING_CONDITION_DC_INIT		(STEP_CHARGING_CONDITION_VOLTAGE | STEP_CHARGING_CONDITION_SOC | STEP_CHARGING_CONDITION_SOC_INIT_ONLY)

#define DIRECT_CHARGING_FLOAT_VOLTAGE_MARGIN		20
#define DIRECT_CHARGING_FORCE_SOC_MARGIN			10

static void print_log_for_step_charging_dt_2darr(unsigned int **arr, int rows, int cols, const char *arr_name)
{
	char str[256] = {0,};
	int i = 0, j = 0;

	for (i = 0; i < rows; i++) {
		memset(str, 0x0, sizeof(str));
		snprintf(str, sizeof(str), "%s arr[%d]:", arr_name, i);
		for (j = 0; j < cols; j++)
			snprintf(str + strlen(str), sizeof(str) - strlen(str), " %d", arr[i][j]);

		pr_info("%s\n", str);
	}
}

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
static void print_log_for_step_charging_dt_1darr(unsigned int *arr, int size, const char *arr_name)
{
	char str[256] = {0,};
	int i = 0;

	snprintf(str, sizeof(str), "%s arr:", arr_name);
	for (i = 0; i < size; i++)
		snprintf(str + strlen(str), sizeof(str) - strlen(str), " %d", arr[i]);

	pr_info("%s\n", str);
}
#endif

static unsigned int **alloc_2darr(int rows, int cols)
{
	int i = 0, j = 0;
	unsigned int **arr;

	arr = kcalloc(rows, sizeof(u32 *), GFP_KERNEL);
	if (!arr)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < rows; i++) {
		arr[i] = kcalloc(cols, sizeof(u32), GFP_KERNEL);
		if (!arr[i]) {
			for (j = 0; j < i; j++)
				kfree(arr[j]);
			kfree(arr);
			return ERR_PTR(-ENOMEM);
		}
	}

	return arr;
}

static int init_2darr_with_dt(struct device_node *np, const char *node_name, unsigned int **arr, int rows, int cols, bool transform_1darr_to_2darr)
{
	int i = 0, j = 0;
	int ret = 0;
	u32 *buffer;

	if (transform_1darr_to_2darr) {
		pr_info("%s: len of %s is not matched.\n", __func__, node_name);

		ret = of_property_read_u32_array(np, node_name, *arr, cols);
		if (ret != 0)
			return ret;

		for (i = 1; i < rows; i++) {
			for (j = 0; j < cols; j++)
				arr[i][j] = arr[0][j];
		}
	} else {
		buffer = kcalloc(rows * cols, sizeof(u32), GFP_KERNEL);
		if (!buffer)
			return -ENOMEM;

		ret = of_property_read_u32_array(np, node_name, buffer, rows * cols);
		if (ret != 0) {
			kfree(buffer);
			return ret;
		}

		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++)
				arr[i][j] = buffer[i * cols + j];
		}
		kfree(buffer);
	}

	return 0;
}

void sec_bat_reset_step_charging(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	sec_vote(battery->vlim_vote, VOTER_STEP_CHARGE, false, 0);
	sec_vote(battery->vlim_vote, VOTER_DC_STEP_CHARGE, false, 0);
#endif
	pr_info("%s\n", __func__);
	battery->step_chg_status = -1;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	battery->wpc_step_chg_status = -1;
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	battery->dc_float_voltage_set = false;
#endif
}
EXPORT_SYMBOL(sec_bat_reset_step_charging);

void sec_bat_exit_step_charging(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	sec_vote(battery->vlim_vote, VOTER_STEP_CHARGE, false, 0);
	sec_vote(battery->vlim_vote, VOTER_DC_STEP_CHARGE, false, 0);
#endif
	sec_vote(battery->fcc_vote, VOTER_STEP_CHARGE, false, 0);
	if (battery->step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
		sec_vote(battery->fv_vote, VOTER_STEP_CHARGE, false, 0);
	sec_bat_reset_step_charging(battery);
}
EXPORT_SYMBOL(sec_bat_exit_step_charging);

void sec_bat_exit_wpc_step_charging(struct sec_battery_info *battery)
{
	sec_vote(battery->fcc_vote, VOTER_WPC_STEP_CHARGE, false, 0);
	if (battery->step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
		sec_vote(battery->fv_vote, VOTER_WPC_STEP_CHARGE, false, 0);
	sec_bat_reset_step_charging(battery);
}
EXPORT_SYMBOL(sec_bat_exit_wpc_step_charging);

bool skip_check_step(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };
	int fpdo_sc = 0;

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	if (is_wireless_all_type(battery->cable_type))
		goto end_skip_check_step;
#endif

	if (battery->cable_type == SEC_BATTERY_CABLE_FPDO_DC) {
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, val);
		fpdo_sc = val.intval;
		pr_info("%s: SC for FPDO_DC(%d)\n", __func__, fpdo_sc);

		if (!fpdo_sc && battery->step_chg_status >= 0)
			sec_bat_reset_step_charging(battery);
	}

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	if (is_pd_apdo_wire_type(battery->cable_type)) {
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, val);
		if (val.intval == SEC_CHARGING_SOURCE_SWITCHING) {
			pr_info("%s: SC for test_mode_source\n", __func__);
			return false;
		} else if (val.intval == SEC_CHARGING_SOURCE_DIRECT) {
			pr_info("%s: DC for test_mode_source\n", __func__);
			goto end_skip_check_step;
		}
		if (!fpdo_sc && !((battery->current_event & SEC_BAT_CURRENT_EVENT_DC_ERR) &&
			(battery->ta_alert_mode == OCP_NONE)))
			goto end_skip_check_step;
	}
	if ((is_pd_apdo_wire_type(battery->cable_type) || is_pd_apdo_wire_type(battery->wire_status)) &&
		!fpdo_sc &&
		(battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL3)) {
		pr_info("%s: This cable type should be checked in dc step check\n", __func__);
		goto end_skip_check_step;
	}
#endif

	if (!is_hv_wire_type(battery->cable_type) && !is_pd_wire_type(battery->cable_type) &&
		(battery->sink_status.rp_currentlvl != RP_CURRENT_LEVEL3))
		goto end_skip_check_step;

	return false;

end_skip_check_step:
	sec_vote(battery->fv_vote, VOTER_STEP_CHARGE, false, 0);
	sec_vote(battery->fcc_vote, VOTER_STEP_CHARGE, false, 0);
	return true;
}

/*
 * true: step is changed
 * false: not changed
 */
bool sec_bat_check_step_charging(struct sec_battery_info *battery)
{
	int i = 0, value = 0, step_condition = 0, lcd_status = 0;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	int value_sub = 0, step_condition_sub = 0;
#endif
	static int curr_cnt;
	static bool skip_lcd_on_changed;
	int age_step = battery->pdata->age_step;

#if defined(CONFIG_SEC_FACTORY)
	if (!battery->step_chg_en_in_factory)
		return false;
#endif
	if (!battery->step_chg_type)
		return false;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if (battery->test_charge_current)
		return false;
	if (battery->test_step_condition <= 4500)
		battery->pdata->step_chg_cond[0][0] = battery->test_step_condition;
#endif

	if (battery->step_chg_type & STEP_CHARGING_CONDITION_ONLINE) {
		if (skip_check_step(battery))
			return false;
	}

	pr_info("%s\n", __func__);

	if (battery->siop_level < 100 || battery->lcd_status)
		lcd_status = 1;
	else
		lcd_status = 0;

	if (battery->step_chg_type & STEP_CHARGING_CONDITION_CHARGE_POWER) {
		if (battery->max_charge_power < battery->step_chg_charge_power) {
			/* In case of max_charge_power falling by AICL during step-charging ongoing */
			sec_bat_exit_step_charging(battery);
			return false;
		}
	}

	if (battery->step_charging_skip_lcd_on && lcd_status) {
		if (!skip_lcd_on_changed) {
			if (battery->step_chg_status != (battery->step_chg_step - 1)) {
				sec_vote(battery->fcc_vote, VOTER_STEP_CHARGE, true,
					battery->pdata->step_chg_curr[age_step][battery->step_chg_step - 1]);

				if (battery->step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
					pr_info("%s : float voltage = %d\n", __func__,
						battery->pdata->step_chg_vfloat[age_step][battery->step_chg_step - 1]);
					sec_vote(battery->fv_vote, VOTER_STEP_CHARGE, true,
						battery->pdata->step_chg_vfloat[age_step][battery->step_chg_step - 1]);
				}
				pr_info("%s : skip step charging because lcd on\n", __func__);
				skip_lcd_on_changed = true;
				return true;
			}
		}
		return false;
	}

	if (battery->step_chg_status < 0) {
		i = 0;

		/* this is only for step enter condition and do not use STEP_CHARGING_CONDITION_SOC at the same time */
		if (battery->step_chg_type & STEP_CHARGING_CONDITION_SOC_INIT_ONLY) {
			int soc_condition;

			value = battery->capacity;
			while (i < battery->step_chg_step - 1) {
				soc_condition = battery->pdata->step_chg_cond_soc[age_step][i];
				if (value < soc_condition)
					break;
				i++;
			}

			pr_info("%s : set initial step(%d) by soc\n", __func__, i);
			goto check_step_change;
		}
	} else
		i = battery->step_chg_status;

	step_condition = battery->pdata->step_chg_cond[age_step][i];

	if (battery->step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		step_condition_sub = battery->pdata->step_chg_cond_sub[age_step][i];
		if (battery->pdata->step_chg_use_vnow) {
			value = battery->voltage_now_main;
			value_sub = battery->voltage_now_sub;
		} else {
			value = battery->voltage_avg_main;
			value_sub = battery->voltage_avg_sub;
		}
#else
		value = battery->voltage_avg;
#endif
	} else if (battery->step_chg_type & STEP_CHARGING_CONDITION_SOC) {
		value = battery->capacity;
		if (lcd_status) {
			step_condition = battery->pdata->step_chg_cond[age_step][i] + 15;
			curr_cnt = 0;
		}
	} else {
		return false;
	}

	while (i < battery->step_chg_step - 1) {
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
			if ((value < step_condition) && (value_sub < step_condition_sub))
				break;
		} else {
			if (value < step_condition)
				break;
		}
#else
		if (value < step_condition)
			break;
#endif
		i++;

		if ((battery->step_chg_type & STEP_CHARGING_CONDITION_SOC) &&
			lcd_status)
			step_condition = battery->pdata->step_chg_cond[age_step][i] + 15;
		else {
			step_condition = battery->pdata->step_chg_cond[age_step][i];
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
			if (battery->step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE)
				step_condition_sub = battery->pdata->step_chg_cond_sub[age_step][i];
#endif
		}
		if (battery->step_chg_status != -1)
			break;
	}

check_step_change:
	if ((i != battery->step_chg_status) || skip_lcd_on_changed) {
		/* this is only for no consuming current */
		if ((battery->step_chg_type & STEP_CHARGING_CONDITION_CURRENT_NOW) &&
			!lcd_status &&
			battery->step_chg_status >= 0) {
			int condition_curr;
			condition_curr = max(battery->current_avg, battery->current_now);
			if (condition_curr < battery->pdata->step_chg_cond_curr[battery->step_chg_status]) {
				curr_cnt++;
				pr_info("%s : cnt = %d, curr(%d)mA < curr cond(%d)mA\n",
					__func__, curr_cnt, condition_curr,
					battery->pdata->step_chg_cond_curr[battery->step_chg_status]);
				if (curr_cnt < 3)
					return false;
			} else {
				pr_info("%s : clear cnt, curr(%d)mA >= curr cond(%d)mA or < 0mA\n",
					__func__, condition_curr,
					battery->pdata->step_chg_cond_curr[battery->step_chg_status]);
				curr_cnt = 0;
				return false;
			}
		}

		pr_info("%s : prev=%d, new=%d, value=%d, current=%d, curr_cnt=%d\n", __func__,
			battery->step_chg_status, i, value,
			battery->pdata->step_chg_curr[age_step][i], curr_cnt);
		battery->step_chg_status = i;
		skip_lcd_on_changed = false;
		sec_vote(battery->fcc_vote, VOTER_STEP_CHARGE, true,
			battery->pdata->step_chg_curr[age_step][i]);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->pdata->step_chg_vsublim) {
			unsigned int vlim =
				battery->pdata->chg_float_voltage - battery->pdata->limiter_aging_float_offset;

			if (battery->step_chg_status != battery->step_chg_step - 1)
				vlim = battery->pdata->step_chg_vsublim;

			sec_vote(battery->vlim_vote, VOTER_STEP_CHARGE, true, vlim);
		}
#endif

		if (battery->step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
			pr_info("%s: float voltage = %d\n", __func__,
				battery->pdata->step_chg_vfloat[age_step][i]);
			sec_vote(battery->fv_vote, VOTER_STEP_CHARGE, true,
				battery->pdata->step_chg_vfloat[age_step][i]);
		}
		return true;
	}
	return false;
}
EXPORT_SYMBOL(sec_bat_check_step_charging);

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
bool sec_bat_check_wpc_step_charging(struct sec_battery_info *battery)
{
	int i = 0, value = 0, step_condition = 0, lcd_status = 0;
	static int curr_cnt;
	static bool skip_lcd_on_changed;
	int age_step = battery->pdata->age_step;

#if defined(CONFIG_SEC_FACTORY)
	if (!battery->step_chg_en_in_factory)
		return false;
#endif
	if (!battery->wpc_step_chg_type)
		return false;
	if (is_not_wireless_type(battery->cable_type)) {
		sec_vote(battery->fv_vote, VOTER_WPC_STEP_CHARGE, false, 0);
		sec_vote(battery->fcc_vote, VOTER_WPC_STEP_CHARGE, false, 0);

		return false;
	}
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if (battery->test_charge_current)
		return false;
	if (battery->test_step_condition <= 4500)
		battery->pdata->wpc_step_chg_cond[0][0] = battery->test_step_condition;
#endif

	if (battery->siop_level < 100 || battery->lcd_status)
		lcd_status = 1;
	else
		lcd_status = 0;

	if (battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_CHARGE_POWER) {
		if (battery->max_charge_power < battery->wpc_step_chg_charge_power) {
			/* In case of max_charge_power falling by AICL during step-charging ongoing */
			sec_bat_exit_wpc_step_charging(battery);
			return false;
		}
	}

	if (battery->step_charging_skip_lcd_on && lcd_status) {
		if (!skip_lcd_on_changed) {
			if (battery->wpc_step_chg_status != (battery->wpc_step_chg_step - 1)) {
				sec_vote(battery->fcc_vote, VOTER_WPC_STEP_CHARGE, true,
					battery->pdata->wpc_step_chg_curr[age_step][battery->wpc_step_chg_step - 1]);

				if (battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
					pr_info("%s : float voltage = %d\n", __func__,
						battery->pdata->wpc_step_chg_vfloat[age_step][battery->wpc_step_chg_step - 1]);
					sec_vote(battery->fv_vote, VOTER_WPC_STEP_CHARGE, true,
						battery->pdata->wpc_step_chg_vfloat[age_step][battery->wpc_step_chg_step - 1]);
				}
				pr_info("%s : skip step charging because lcd on\n", __func__);
				skip_lcd_on_changed = true;
				return true;
			}
		}
		return false;
	}

	if (battery->wpc_step_chg_status < 0)
		i = 0;
	else
		i = battery->wpc_step_chg_status;

	step_condition = battery->pdata->wpc_step_chg_cond[age_step][i];

	if (battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
		value = battery->voltage_avg;
	} else if (battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_SOC) {
		value = battery->capacity;
		if (lcd_status) {
			step_condition = battery->pdata->wpc_step_chg_cond[age_step][i] + 15;
			curr_cnt = 0;
		}
	} else {
		return false;
	}

	while (i < battery->wpc_step_chg_step - 1) {
		if (value < step_condition)
			break;
		i++;

		if ((battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_SOC) &&
			lcd_status)
			step_condition = battery->pdata->wpc_step_chg_cond[age_step][i] + 15;
		else {
			step_condition = battery->pdata->wpc_step_chg_cond[age_step][i];
		}
		if (battery->wpc_step_chg_status != -1)
			break;
	}

	/* this is only for no consuming current */
	if ((battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_CURRENT_NOW) &&
		!lcd_status &&
		battery->wpc_step_chg_status >= 0) {
		int condition_curr;
		condition_curr = max(battery->current_avg, battery->current_now);
		if (condition_curr < battery->pdata->wpc_step_chg_cond_curr[battery->wpc_step_chg_status]) {
			curr_cnt++;
			pr_info("%s : cnt = %d, curr(%d)mA < curr cond(%d)mA\n",
				__func__, curr_cnt, condition_curr,
				battery->pdata->wpc_step_chg_cond_curr[battery->wpc_step_chg_status]);
			if (curr_cnt < 3)
				return false;
		} else {
			pr_info("%s : clear cnt, curr(%d)mA >= curr cond(%d)mA or < 0mA\n",
				__func__, condition_curr,
				battery->pdata->wpc_step_chg_cond_curr[battery->wpc_step_chg_status]);
			curr_cnt = 0;
			return false;
		}
	}

	pr_info("%s : prev=%d, new=%d, value=%d, current=%d, curr_cnt=%d\n", __func__,
		battery->wpc_step_chg_status, i, value,
		battery->pdata->wpc_step_chg_curr[age_step][i], curr_cnt);
	battery->wpc_step_chg_status = i;
	skip_lcd_on_changed = false;
	sec_vote(battery->fcc_vote, VOTER_WPC_STEP_CHARGE, true,
		battery->pdata->wpc_step_chg_curr[age_step][i]);

	if (battery->wpc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
		pr_info("%s : float voltage = %d\n", __func__,
			battery->pdata->wpc_step_chg_vfloat[age_step][i]);
		sec_vote(battery->fv_vote, VOTER_WPC_STEP_CHARGE, true,
			battery->pdata->wpc_step_chg_vfloat[age_step][i]);
	}

	return true;
}
EXPORT_SYMBOL(sec_bat_check_wpc_step_charging);
#endif

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
bool skip_check_dc_step(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };

	if (battery->dchg_dc_in_swelling) {
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE)
			goto end_skip_check_dc_step;
	} else {
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE)
			goto end_skip_check_dc_step;
	}

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ||
		   ((battery->current_event & SEC_BAT_CURRENT_EVENT_DC_ERR) &&
		   (battery->ta_alert_mode == OCP_NONE)) ||
		   battery->current_event & SEC_BAT_CURRENT_EVENT_SIOP_LIMIT ||
		   battery->wc_tx_enable ||
		   battery->uno_en ||
		   battery->mix_limit ||
		   battery->lrp_chg_src == SEC_CHARGING_SOURCE_SWITCHING)
		goto end_skip_check_dc_step;

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, val);
	if (val.intval == SEC_CHARGING_SOURCE_DIRECT) {
		pr_info("%s: DC for test_mode_source\n", __func__);
		return false;
	} else if (val.intval == SEC_CHARGING_SOURCE_SWITCHING) {
		pr_info("%s: SC for test_mode_source\n", __func__);
		return true;
	}

	return false;

end_skip_check_dc_step:
	if (battery->step_chg_status >= 0)
		sec_bat_reset_step_charging(battery);

	return true;
}

bool sec_bat_check_dc_step_charging(struct sec_battery_info *battery)
{
	int i, value;
	int step = -1, step_vol = -1, step_input = -1, step_soc = -1, soc_condition = 0;
	int force_step_soc = 0, step_fg_current = -1;
	bool force_change_step = false;
	union power_supply_propval val = {0, };
	int age_step = battery->pdata->age_step;
	unsigned int dc_step_chg_type;
	int dc_op_mode = get_sec_vote_resultf("DCHG_OP");

	if (battery->dc_step_chg_step == 0 || battery->dc_step_chg_type == NULL) {
		pr_info("%s : Skip dc_step_charging. Please check dc_step_chg_* properties in .dtsi\n", __func__);
		return false;
	}

	if (dc_op_mode < 0 || !is_dc_higher_ratio_support())
		dc_op_mode = DC_MODE_2TO1;

	if (battery->cable_type == SEC_BATTERY_CABLE_FPDO_DC) {
		sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, false, 0);
		sec_vote(battery->fcc_vote, VOTER_CABLE, true,
			battery->pdata->charging_current[SEC_BATTERY_CABLE_FPDO_DC].fast_charging_current);
		sec_vote_refresh(battery->fcc_vote);

		return false;
	}

	i = (battery->step_chg_status < 0 ? 0 : battery->step_chg_status);
	dc_step_chg_type = battery->dc_step_chg_type[i];

	if (!dc_step_chg_type) {
		sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, false, 0);
		return false;
	}

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_CHARGE_POWER)
		if (battery->charge_power < battery->dc_step_chg_charge_power) {
			sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, false, 0);
			return false;
		}

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_ONLINE) {
		if (!is_pd_apdo_wire_type(battery->cable_type)) {
			sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, false, 0);
			return false;
		}
	}
	if (skip_check_dc_step(battery)) {
		sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, false, 0);
		return false;
	}

	if (!(dc_step_chg_type & STEP_CHARGING_CONDITION_DC_INIT)) {
		pr_info("%s : cond_vol and cond_soc are both empty\n", __func__);
		sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, false, 0);
		return false;
	}

	/* this is only for step enter condition and do not use STEP_CHARGING_CONDITION_SOC at the same time */
	if (dc_step_chg_type & STEP_CHARGING_CONDITION_SOC_INIT_ONLY) {
		if (battery->step_chg_status < 0) {
			step_soc = i;
			value = battery->capacity;
			while (step_soc < battery->dc_step_chg_step - 1) {
				soc_condition = battery->pdata->dc_step_chg_cond_soc[age_step][step_soc];
				if (value < soc_condition)
					break;
				step_soc++;
			}

			if ((step_soc < step) || (step < 0))
				step = step_soc;

			pr_info("%s : set initial step(%d) by soc\n", __func__, step_soc);
			goto check_dc_step_change;
		} else
			step_soc = battery->dc_step_chg_step - 1;
	}

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_SOC) {
		step_soc = i;
		value = battery->capacity;
		while (step_soc < battery->dc_step_chg_step - 1) {
			soc_condition = battery->pdata->dc_step_chg_cond_soc[age_step][step_soc];
			if (battery->step_chg_status >= 0 &&
				(battery->siop_level < 100 || battery->lcd_status)) {
				soc_condition += DIRECT_CHARGING_FORCE_SOC_MARGIN;
				force_change_step = true;
			}
			if (value < soc_condition)
				break;
			step_soc++;
			if (battery->step_chg_status >= 0)
				break;
		}

		if ((step_soc < step) || (step < 0))
			step = step_soc;

		if (battery->step_chg_status < 0) {
			pr_info("%s : set initial step(%d) by soc\n", __func__, step_soc);
			goto check_dc_step_change;
		}
		if (force_change_step) {
			pr_info("%s : force check step(%d) by soc\n", __func__, step_soc);
			step_vol = step_input = step_soc;
			battery->dc_step_chg_iin_cnt = battery->pdata->dc_step_chg_iin_check_cnt;
			goto check_dc_step_change;
		}
	} else
		step_soc = battery->dc_step_chg_step - 1;

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		int value_main = 0;
		int value_sub = 0;
		int step_cond_main = 0;
		int step_cond_sub = 0;
#endif
		step_vol = i;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->pdata->dc_step_chg_use_vnow) {
			value_main = battery->voltage_now_main;
			value_sub = battery->voltage_now_sub;
		} else {
			value_main = battery->voltage_avg_main;
			value_sub = battery->voltage_avg_sub;
		}
		step_cond_main = battery->pdata->dc_step_chg_cond_vol[age_step][step_vol];
		step_cond_sub = battery->pdata->dc_step_chg_cond_vol_sub[age_step][step_vol];

		/* (charging current)step down when main or sub voltage condition meets */
		while (step_vol < battery->dc_step_chg_step - 1) {
			if (value_main < step_cond_main && value_sub < step_cond_sub)
				break;
			step_vol++;
			if (battery->step_chg_status >= 0)
				break;
		}
#else
		if (dc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
			value = battery->voltage_now + battery->pdata->dc_step_chg_cond_v_margin;
		else
			value = battery->voltage_avg;
		while (step_vol < battery->dc_step_chg_step - 1) {
			if (value < battery->pdata->dc_step_chg_cond_vol[age_step][step_vol])
				break;
			step_vol++;
			if (battery->step_chg_status >= 0)
				break;
		}
#endif
		if ((step_vol < step) || (step < 0))
			step = step_vol;

		if (battery->step_chg_status < 0) {
			pr_info("%s : set initial step(%d) by vol\n", __func__, step_vol);
			goto check_dc_step_change;
		}
	} else
		step_vol = battery->dc_step_chg_step - 1;

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_INPUT_CURRENT) {
		step_input = i;
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE, val);
		if (val.intval != SEC_DIRECT_CHG_MODE_DIRECT_ON) {
			pr_info("%s : dc no charging status = %d\n", __func__, val.intval);
			battery->dc_step_chg_iin_cnt = 0;
			return false;
		} else if (battery->siop_level >= 100 && !battery->lcd_status) {
			val.intval = SEC_BATTERY_IIN_MA;
			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_EXT_PROP_MEASURE_INPUT, val);
			value = val.intval;

			while (step_input < battery->dc_step_chg_step - 1) {
				if (value > battery->pdata->dc_step_chg_cond_iin[dc_op_mode][step_input])
					break;
				step_input++;

				if (battery->step_chg_status >= 0) {
					battery->dc_step_chg_iin_cnt++;
					break;
				} else {
					battery->dc_step_chg_iin_cnt = 0;
				}
			}
		} else {
			/*
			 * Do not check input current when lcd is on or siop is not 100
			 * since there might be quite big system current
			 */
			step_input = battery->dc_step_chg_step - 1;
		}

		if ((step_input < step) || (step < 0))
			step = step_input;
	} else
		step_input = battery->dc_step_chg_step - 1;

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_FG_CURRENT) {
		step_fg_current = i;
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE, val);
		if (val.intval != SEC_DIRECT_CHG_MODE_DIRECT_ON) {
			pr_info("%s : dc no charging status = %d\n", __func__, val.intval);
			battery->dc_step_chg_iin_cnt = 0;
			return false;
		} else if (battery->siop_level >= 100 && !battery->lcd_status) {
			int current_now, current_avg;

			val.intval = SEC_BATTERY_CURRENT_MA;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, val);
			current_now = val.intval;
			val.intval = SEC_BATTERY_CURRENT_MA;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CURRENT_AVG, val);
			current_avg = val.intval;
			value = max(current_now, current_avg) / dc_op_mode;

			while (step_fg_current < battery->dc_step_chg_step - 1) {
				if (value > battery->pdata->dc_step_chg_cond_iin[dc_op_mode][step_fg_current])
					break;
				step_fg_current++;

				if (battery->step_chg_status >= 0) {
					battery->dc_step_chg_iin_cnt++;
					break;
				}
				battery->dc_step_chg_iin_cnt = 0;
			}
		} else {
			/*
			 * Do not check input current when lcd is on or siop is not 100
			 * since there might be quite big system current
			 */
			step_fg_current = battery->dc_step_chg_step - 1;
		}

		if ((step_fg_current < step) || (step < 0))
			step = step_fg_current;
	} else
		step_fg_current = battery->dc_step_chg_step - 1;


	if (dc_step_chg_type & STEP_CHARGING_CONDITION_FORCE_SOC) {
		force_step_soc = i;
		if (battery->capacity >= battery->pdata->dc_step_chg_cond_soc[age_step][i]) {
			if (++force_step_soc > step)
				step = force_step_soc;
			pr_info("%s : SOC(%d) cond_soc(%d) step(%d) force_step_soc(%d)\n", __func__,
				battery->capacity, battery->pdata->dc_step_chg_cond_soc[age_step][i],
				step, force_step_soc);
		} else
			force_step_soc = 0;
	} else
		force_step_soc = 0;


check_dc_step_change:
	pr_info("%s : curr_step(%d), step_vol(%d), step_soc(%d), step_input(%d, %d), curr_cnt(%d/%d) force_step_soc(%d)\n",
		__func__, step, step_vol, step_soc, step_input, step_fg_current,
		battery->dc_step_chg_iin_cnt, battery->pdata->dc_step_chg_iin_check_cnt, force_step_soc);

	if (battery->step_chg_status < 0 || force_step_soc ||
		(step != battery->step_chg_status &&
		step == min(min(step_vol, step_soc), min(step_input, step_fg_current)))) {
		if ((dc_step_chg_type &
			(STEP_CHARGING_CONDITION_INPUT_CURRENT | STEP_CHARGING_CONDITION_FG_CURRENT)) &&
			(battery->step_chg_status >= 0)) {
			if ((battery->dc_step_chg_iin_cnt < battery->pdata->dc_step_chg_iin_check_cnt) &&
				(battery->siop_level >= 100 && !battery->lcd_status) && !force_step_soc) {
				pr_info("%s : keep step(%d), curr_cnt(%d/%d)\n",
					__func__, battery->step_chg_status,
					battery->dc_step_chg_iin_cnt, battery->pdata->dc_step_chg_iin_check_cnt);
				return false;
			}
		}

		pr_info("%s : cable(%d), soc(%d), step changed(%d->%d), current(%dmA) force_step_soc(%d)\n",
			__func__, battery->cable_type, battery->capacity, battery->step_chg_status, step,
			battery->pdata->dc_step_chg_val_iout[age_step][step], force_step_soc);
		/* set charging current */
		battery->pdata->charging_current[battery->cable_type].fast_charging_current =
			battery->pdata->dc_step_chg_val_iout[age_step][step];

		if (dc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
			if (battery->step_chg_status < 0) {
				pr_info("%s : step float voltage = %d\n", __func__,
					battery->pdata->dc_step_chg_val_vfloat[age_step][step]);
				sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, true,
					battery->pdata->dc_step_chg_val_vfloat[age_step][step]);
			}
			battery->dc_float_voltage_set = true;
		}

		if (battery->step_chg_status < 0) {
			pr_info("%s : step input current = %d\n", __func__,
				battery->pdata->dc_step_chg_val_iout[age_step][step] / dc_op_mode);
			val.intval = battery->pdata->dc_step_chg_val_iout[age_step][step] / dc_op_mode;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX, val);
		}

		battery->step_chg_status = step;
		battery->dc_step_chg_iin_cnt = 0;

		sec_vote(battery->fcc_vote, VOTER_CABLE, true,
			battery->pdata->dc_step_chg_val_iout[age_step][step]);
		sec_vote_refresh(battery->fcc_vote);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->pdata->dc_step_chg_vsublim) {
			sec_vote(battery->vlim_vote, VOTER_DC_STEP_CHARGE, true,
					battery->pdata->dc_step_chg_vsublim[battery->step_chg_status]);
		}
#endif

		return true;
	} else {
		battery->dc_step_chg_iin_cnt = 0;
	}

	return false;
}
EXPORT_SYMBOL(sec_bat_check_dc_step_charging);

static int sec_bat_parse_cond_iin(struct device_node *np, char *name,
	int age_step, int dc_step_chg_step, sec_battery_platform_data_t *pdata)
{
	const u32 *p;
	int i = 0, j = 0, len = 0, ratio = 0, ret = 0;
	u32 *iin_temp;
	char str[128] = {0,};
	int dc_step_chg_ratio = is_dc_higher_ratio_support();

	if (dc_step_chg_ratio < DC_MODE_2TO1)
		dc_step_chg_ratio = DC_MODE_2TO1;

	if (dc_step_chg_step < 2) {
		pr_err("%s: dc_step_chg_cond_iin is empty\n", __func__);
		return -1;
	}

	pdata->dc_step_chg_cond_iin = kcalloc(dc_step_chg_ratio + 1, sizeof(u32 *), GFP_KERNEL);
	for (i = 0; i <= dc_step_chg_ratio; i++) {
		pdata->dc_step_chg_cond_iin[i] =
			kcalloc(dc_step_chg_step, sizeof(u32), GFP_KERNEL);
	}

	p = of_get_property(np, name, &len);
	if (!p) {
		for (j = 0; j <= dc_step_chg_ratio; j++) {
			ratio = j;
			pr_info("%s: %s is Empty, set default (Iout / %d)\n", __func__, name, ratio);

			if (j <= DC_MODE_2TO1)
				ratio = DC_MODE_2TO1;
			for (i = 0; i < (dc_step_chg_step - 1); i++) {
				pdata->dc_step_chg_cond_iin[j][i] =
					pdata->dc_step_chg_val_iout[age_step][i + 1] / ratio;
				pr_info("%s: Condition Iin Ratio[%d:1][step %d] %dmA\n",
					__func__, j, i, pdata->dc_step_chg_cond_iin[j][i]);
			}
			pdata->dc_step_chg_cond_iin[j][i] = 0;
		}
	} else {
		len = len / sizeof(u32);

		pr_info("%s: step(%d) * dc_ratio(%d), dc_step_chg_cond_iin len(%d)\n",
			__func__, dc_step_chg_step, dc_step_chg_ratio, len);

		/* if there are only 1 dimentional array of value, get the same value */
		if ((dc_step_chg_step * (dc_step_chg_ratio + 1)) != len) {
			pr_err("%s: len of dc_step_chg_cond_iin is not matched\n", __func__);

			ret = of_property_read_u32_array(np, name,
					pdata->dc_step_chg_cond_iin[0], dc_step_chg_step);
			if (ret) {
				pr_info("%s : dc_step_chg_cond_iin read fail\n", __func__);
				return -1;
			}

			for (i = 1; i <= dc_step_chg_ratio; i++) {
				for (j = 0; j < dc_step_chg_step; j++)
					pdata->dc_step_chg_cond_iin[i][j] = pdata->dc_step_chg_cond_iin[0][j];
			}
		} else {
			iin_temp = kcalloc(dc_step_chg_step * (dc_step_chg_ratio + 1), sizeof(u32), GFP_KERNEL);

			ret = of_property_read_u32_array(np, name,
						iin_temp, dc_step_chg_step * (dc_step_chg_ratio + 1));
			if (ret) {
				pr_err("%s: Failed to read iin_temp property\n", __func__);
				kfree(iin_temp);
				return -1;
			}

			/* copy buff to 2d arr */
			for (i = 0; i <= dc_step_chg_ratio; i++) {
				for (j = 0; j < dc_step_chg_step; j++)
					pdata->dc_step_chg_cond_iin[i][j] = iin_temp[i * dc_step_chg_step + j];
			}
			kfree(iin_temp);
		}

		/* debug log */
		for (i = 0; i <= dc_step_chg_ratio; i++) {
			memset(str, 0x0, sizeof(str));
			sprintf(str + strlen(str), "iin arr[%d:1]:", i);
			for (j = 0; j < dc_step_chg_step; j++)
				sprintf(str + strlen(str), " %d", pdata->dc_step_chg_cond_iin[i][j]);
			pr_info("%s: %s\n", __func__, str);
		}
	}

	return 0;
} /* sec_bat_parse_cond_iin */

void parse_step_chg_condition(struct sec_battery_info *battery, struct device_node *np,
	const char *prop_name, u32 ***dest, unsigned int cond_type, unsigned int dc_step_chg_type)
{
	int ret = 0, len = 0, i = 0;
	const u32 *p;
	int num_age_step = battery->pdata->num_age_step;

	p = of_get_property(np, prop_name, &len);
	if (!p) {
		pr_err("%s: %s is Empty, type(0x%X->0x%X)\n",
			__func__, prop_name, dc_step_chg_type,
			dc_step_chg_type & ~cond_type);
		for (i = 0; i < battery->dc_step_chg_step; i++)
			battery->dc_step_chg_type[i] &= ~cond_type;
	} else {
		len = len / sizeof(u32);
		pr_info("%s: step(%d) * age_step(%d), %s len(%d)\n",
			__func__, battery->dc_step_chg_step, num_age_step, prop_name, len);

		*dest = alloc_2darr(num_age_step, battery->dc_step_chg_step);
		if (IS_ERR(*dest)) {
			ret = PTR_ERR(*dest);
		} else {
			ret = init_2darr_with_dt(np, prop_name,
				*dest, num_age_step, battery->dc_step_chg_step,
				(num_age_step * battery->dc_step_chg_step) != len);

			print_log_for_step_charging_dt_2darr(*dest,
				num_age_step, battery->dc_step_chg_step, prop_name);
		}

		if (ret) {
			pr_info("%s : dc_step_chg_cond_vol read fail\n", __func__);
			for (i = 0; i < battery->dc_step_chg_step; i++)
				battery->dc_step_chg_type[i] &= ~cond_type;
		}
	}
}

void parse_volt_condition(struct sec_battery_info *battery, struct device_node *np, const char *prop_name,
	unsigned int dc_step_chg_type)
{
	parse_step_chg_condition(battery, np, prop_name,
		&battery->pdata->dc_step_chg_cond_vol, STEP_CHARGING_CONDITION_VOLTAGE, dc_step_chg_type);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	parse_step_chg_condition(battery, np, "battery,dc_step_chg_cond_vol_sub",
		&battery->pdata->dc_step_chg_cond_vol_sub, STEP_CHARGING_CONDITION_VOLTAGE, dc_step_chg_type);

	sb_of_parse_u32_dt(np, "battery,dc_step_cond_v_margin_main",
		battery->pdata, dc_step_cond_v_margin_main, 0);
	sb_of_parse_u32_dt(np, "battery,dc_step_cond_v_margin_sub",
		battery->pdata, dc_step_cond_v_margin_sub, 0);
	sb_of_parse_u32_dt(np, "battery,sc_vbat_thresh_main",
		battery->pdata, sc_vbat_thresh_main, 4420);
	sb_of_parse_u32_dt(np, "battery,sc_vbat_thresh_sub",
		battery->pdata, sc_vbat_thresh_sub, battery->pdata->sc_vbat_thresh_main);
	sb_of_parse_bool_dt(np, "battery,dc_step_chg_use_vnow", battery->pdata, dc_step_chg_use_vnow);
#endif
}

void parse_soc_condition(struct sec_battery_info *battery, struct device_node *np,
	unsigned int dc_step_chg_type)
{
	unsigned int i = 0;

	parse_step_chg_condition(battery, np, "battery,dc_step_chg_cond_soc",
		&battery->pdata->dc_step_chg_cond_soc,
		(STEP_CHARGING_CONDITION_SOC | STEP_CHARGING_CONDITION_SOC_INIT_ONLY),
		dc_step_chg_type);

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_SOC &&
		dc_step_chg_type & STEP_CHARGING_CONDITION_SOC_INIT_ONLY) {
		pr_info("%s : do not set SOC and SOC_INIT_ONLY at the same time\n", __func__);
		for (i = 0; i < battery->dc_step_chg_step; i++)
			battery->dc_step_chg_type[i] &= ~STEP_CHARGING_CONDITION_SOC;
	}
}

void parse_fv_condition(struct sec_battery_info *battery, struct device_node *np, const char *prop_name,
	unsigned int dc_step_chg_type)
{
	unsigned int i = 0;
	long ret = 0;

	parse_step_chg_condition(battery, np, prop_name,
		&battery->pdata->dc_step_chg_val_vfloat,
		STEP_CHARGING_CONDITION_FLOAT_VOLTAGE, dc_step_chg_type);

	sb_of_parse_u32_dt(np, "battery,dc_step_chg_cond_v_margin",
		battery->pdata, dc_step_chg_cond_v_margin, DIRECT_CHARGING_FLOAT_VOLTAGE_MARGIN);

	battery->pdata->dc_step_chg_vol_offset = kcalloc(battery->dc_step_chg_step, sizeof(u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, "battery,dc_step_chg_vol_offset",
			battery->pdata->dc_step_chg_vol_offset, battery->dc_step_chg_step);
	if (ret) {
		pr_info("%s: dc_step_chg_vol_offset is empty\n", __func__);
		/* Fill-up use one-dimensional offset table */
		for (i = 0; i < battery->dc_step_chg_step; i++) {
			if (battery->pdata->dc_step_chg_val_vfloat[0][i] > battery->pdata->chg_float_voltage)
				battery->pdata->dc_step_chg_vol_offset[i] =
					battery->pdata->dc_step_chg_val_vfloat[0][i]
						- battery->pdata->chg_float_voltage;
		}
	}

	print_log_for_step_charging_dt_1darr(battery->pdata->dc_step_chg_vol_offset,
		battery->dc_step_chg_step, "dc_step_chg_vol_offset");
}

void parse_iout_condition(struct sec_battery_info *battery, struct device_node *np)
{
	int num_age_step = battery->pdata->num_age_step;
	unsigned int i = 0;
	int len = 0;
	long ret = 0;
	const u32 *p;

	p = of_get_property(np, "battery,dc_step_chg_val_iout", &len);
	if (!p) {
		pr_err("%s: dc_step_chg_val_iout is Empty\n", __func__);
		for (i = 0; i < battery->dc_step_chg_step; i++)
			battery->dc_step_chg_type[i] = 0;
		return;
	} else {
		len = len / sizeof(u32);
		pr_info("%s: step(%d) * age_step(%d), dc_step_chg_val_iout len(%d)\n",
			__func__, battery->dc_step_chg_step, num_age_step, len);

		battery->pdata->dc_step_chg_val_iout = alloc_2darr(num_age_step, battery->dc_step_chg_step);
		if (IS_ERR(battery->pdata->dc_step_chg_val_iout)) {
			ret = PTR_ERR(battery->pdata->dc_step_chg_val_iout);
		} else {
			ret = init_2darr_with_dt(np, "battery,dc_step_chg_val_iout",
					battery->pdata->dc_step_chg_val_iout, num_age_step, battery->dc_step_chg_step,
					(num_age_step * battery->dc_step_chg_step) != len);

			print_log_for_step_charging_dt_2darr(battery->pdata->dc_step_chg_val_iout,
				num_age_step, battery->dc_step_chg_step, "dc_step_chg_val_iout");
		}

		if (ret) {
			pr_info("%s : dc_step_chg_val_iout read fail\n", __func__);
		}
	}
}

void parse_current_condition(struct sec_battery_info *battery, struct device_node *np)
{
	int age_step = battery->pdata->age_step;
	unsigned int i = 0;
	long ret = 0;

	if (sec_bat_parse_cond_iin(np, "battery,dc_step_chg_cond_iin", age_step,
		battery->dc_step_chg_step, battery->pdata) < 0) {
		pr_err("%s: clear INPUT_CURRENT and FG_CURRENT conditions\n", __func__);
		for (i = 0; i < battery->dc_step_chg_step; i++) {
			battery->dc_step_chg_type[i] &= ~STEP_CHARGING_CONDITION_INPUT_CURRENT;
			battery->dc_step_chg_type[i] &= ~STEP_CHARGING_CONDITION_FG_CURRENT;
		}
	}

	ret = of_property_read_u32(np, "battery,dc_step_chg_iin_check_cnt",
			&battery->pdata->dc_step_chg_iin_check_cnt);
	if (ret) {
		pr_err("%s: dc_step_chg_iin_check_cnt is Empty\n", __func__);
		battery->pdata->dc_step_chg_iin_check_cnt = 2;
	} else {
		pr_err("%s: dc_step_chg_iin_check_cnt is %d\n",
			__func__, battery->pdata->dc_step_chg_iin_check_cnt);
	}
}

void print_dc_step_chg_info(struct sec_battery_info *battery, int dc_step_chg_ratio)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	char str[128] = {0,};
	int i = 0, j = 0;
	int age_step = battery->pdata->age_step;

	for (i = 0; i < battery->dc_step_chg_step; i++) {
		memset(str, 0x0, sizeof(str));
		if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_VOLTAGE)
			sprintf(str + strlen(str), "cond_vol: %dmV, ", pdata->dc_step_chg_cond_vol[age_step][i]);
		if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_SOC)
			sprintf(str + strlen(str), "cond_soc: %d%%, ", pdata->dc_step_chg_cond_soc[age_step][i]);
		if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
			sprintf(str + strlen(str), "vfloat: %dmV, ", pdata->dc_step_chg_val_vfloat[age_step][i]);

		sprintf(str + strlen(str), "iout: %dmA,", pdata->dc_step_chg_val_iout[age_step][i]);
		pr_info("%s : step [%d] %s\n", __func__, i, str);
	}

	for (j = DC_MODE_2TO1; j <= dc_step_chg_ratio; j++) {
		for (i = 0; i < battery->dc_step_chg_step; i++) {
			memset(str, 0x0, sizeof(str));
			if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_INPUT_CURRENT ||
				battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_FG_CURRENT) {
				sprintf(str + strlen(str), "cond_iin: %dmA, ", pdata->dc_step_chg_cond_iin[j][i]);
				pr_info("%s : ratio[%d:1] step[%d] %s\n", __func__, j, i, str);
			}
		}
	}
}

static bool get_dc_step_chg_step(struct sec_battery_info *battery, struct device_node *np)
{
	int ret = 0;

	ret = of_property_read_u32(np, "battery,dc_step_chg_step", &battery->dc_step_chg_step);
	if (ret) {
		battery->dc_step_chg_step = 0;
		pr_info("%s: dc_step_chg_step is Empty\n", __func__);
		return false;
	}
	pr_err("%s: dc_step_chg_step is %d\n", __func__, battery->dc_step_chg_step);
	return true;
}

void print_dc_step_chg_type(struct sec_battery_info *battery)
{
	char str[128] = {0,};
	unsigned int i = 0;

	memset(str, 0x0, sizeof(str));
	sprintf(str + strlen(str), "dc_step_chg_type arr :");
	for (i = 0; i < battery->dc_step_chg_step; i++)
		sprintf(str + strlen(str), " 0x%x", battery->dc_step_chg_type[i]);

	pr_info("%s: %s\n", __func__, str);
}

static bool get_dc_step_chg_type(struct sec_battery_info *battery, struct device_node *np)
{
	int ret = 0, len = 0, i;
	const u32 *p;

	p = of_get_property(np, "battery,dc_step_chg_type", &len);
	if (!p) {
		pr_info("%s: dc_step_chg_type is Empty\n", __func__);
		return false;
	}

	len = len / sizeof(u32);
	battery->dc_step_chg_type = kcalloc(battery->dc_step_chg_step, sizeof(u32), GFP_KERNEL);
	ret = of_property_read_u32_array(np, "battery,dc_step_chg_type", battery->dc_step_chg_type, len);
	if (len != battery->dc_step_chg_step) {
		pr_err("%s not match size of dc_step_chg_type: %d\n", __func__, len);
		for (i = 1; i < battery->dc_step_chg_step; i++)
			battery->dc_step_chg_type[i] = battery->dc_step_chg_type[0];
	}
	print_dc_step_chg_type(battery);

	return true;
}

void sec_update_dc_step_chg_dt(struct sec_battery_info *battery, int battery_id, int type)
{
	struct device_node *np;
	char prop_name[64] = {0,};
	int i = 0, age_step = battery->pdata->age_step;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: battery is NULL!\n", __func__);
		return;
	}
	//  clear the data
	if (type & STEP_CHARGING_CONDITION_VOLTAGE) {
		for (i = 0; i < age_step; i++)
			kfree(battery->pdata->dc_step_chg_cond_vol[i]);
		kfree(battery->pdata->dc_step_chg_cond_vol);
	}
	if (type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
		for (i = 0; i < age_step; i++)
			kfree(battery->pdata->dc_step_chg_val_vfloat[i]);
		kfree(battery->pdata->dc_step_chg_val_vfloat);
		kfree(battery->pdata->dc_step_chg_vol_offset);
	}

	pr_info("%s: battery_id(%d) type(0x%x)\n", __func__, battery_id, type);

	// update the data
	if (type & STEP_CHARGING_CONDITION_VOLTAGE) {
		memset(prop_name, 0x0, sizeof(prop_name));
		sprintf(prop_name, "battery%d,dc_step_chg_cond_vol", battery_id);
		parse_volt_condition(battery, np, prop_name, type);
	}
	if (type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
		memset(prop_name, 0x0, sizeof(prop_name));
		sprintf(prop_name, "battery%d,dc_step_chg_val_vfloat", battery_id);
		parse_fv_condition(battery, np, prop_name, type);
	}
}
EXPORT_SYMBOL(sec_update_dc_step_chg_dt);

int sec_dc_step_charging_dt(struct sec_battery_info *battery, struct device *dev)
{
	struct device_node *np = dev->of_node;
	unsigned int dc_step_chg_type = 0;
	unsigned int i = 0;
	int dc_step_chg_ratio = is_dc_higher_ratio_support();

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	long ret = 0;
#endif

	if (dc_step_chg_ratio < DC_MODE_2TO1)
		dc_step_chg_ratio = DC_MODE_2TO1;

	battery->dchg_dc_in_swelling = of_property_read_bool(np,
						     "battery,dchg_dc_in_swelling");
	pr_info("%s: dchg_dc_in_swelling(%d)\n", __func__, battery->dchg_dc_in_swelling);

	if (!get_dc_step_chg_step(battery, np))
		return -1;
	if (!get_dc_step_chg_type(battery, np))
		return -1;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	battery->pdata->dc_step_chg_vsublim = kcalloc(battery->dc_step_chg_step, sizeof(u32), GFP_KERNEL);

	ret = of_property_read_u32_array(np, "battery,dc_step_chg_vsublim",
			battery->pdata->dc_step_chg_vsublim, battery->dc_step_chg_step);
	if (ret) {
		pr_info("%s: dc_step_chg_vsublim is empty\n", __func__);
		kfree(battery->pdata->dc_step_chg_vsublim);
		battery->pdata->dc_step_chg_vsublim = NULL;
	} else {
		print_log_for_step_charging_dt_1darr(battery->pdata->dc_step_chg_vsublim,
			battery->dc_step_chg_step, "dc_step_chg_vsublim");
	}
#endif
	sb_of_parse_u32_dt(np, "battery,dc_step_chg_charge_power",
		battery, dc_step_chg_charge_power, 20000);

	for (i = 0; i < battery->dc_step_chg_step; i++)
		dc_step_chg_type |= battery->dc_step_chg_type[i];

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE)
		parse_volt_condition(battery, np, "battery,dc_step_chg_cond_vol", dc_step_chg_type);

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_SOC ||
		dc_step_chg_type & STEP_CHARGING_CONDITION_SOC_INIT_ONLY)
		parse_soc_condition(battery, np, dc_step_chg_type);

	if (dc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
		parse_fv_condition(battery, np, "battery,dc_step_chg_val_vfloat", dc_step_chg_type);

	parse_iout_condition(battery, np);

	if ((dc_step_chg_type & STEP_CHARGING_CONDITION_INPUT_CURRENT) ||
		(dc_step_chg_type & STEP_CHARGING_CONDITION_FG_CURRENT))
		parse_current_condition(battery, np);

	print_dc_step_chg_info(battery, dc_step_chg_ratio);

	return 0;
} /* sec_dc_step_charging_dt */
#endif

void sec_bat_set_aging_info_step_charging(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	union power_supply_propval val;
	int i = 0;
	unsigned int max_fv = 0;
	int float_volt;
#endif
	int age_step = battery->pdata->age_step;
	int dc_op_mode = get_sec_vote_resultf("DCHG_OP");

#if !IS_ENABLED(CONFIG_DIRECT_CHARGING)
	dc_op_mode = DC_MODE_2TO1;
#else
	if (dc_op_mode < 0 || !is_dc_higher_ratio_support())
		dc_op_mode = DC_MODE_2TO1;
#endif

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	if (battery->dc_step_chg_step == 0 || battery->dc_step_chg_type == NULL) {
		pr_info("%s : Skip dc_step_charging. Please check dc_step_chg_* properties in .dtsi\n", __func__);
		return;
	}

	i = (battery->step_chg_status < 0 ? 0 : battery->step_chg_status);
	if (!battery->dc_step_chg_type[i]) {
		pr_info("%s : invalid dc step chg type\n", __func__);
		return;
	}
#endif

	if (battery->step_chg_type) {
		if (battery->step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
			battery->pdata->step_chg_vfloat[age_step][battery->step_chg_step-1] =
				battery->pdata->chg_float_voltage;

		dev_info(battery->dev, "%s: float_v(%d)\n",
			__func__, battery->pdata->step_chg_vfloat[age_step][battery->step_chg_step-1]);
	}
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	for (i = 0; i < battery->dc_step_chg_step; i++) {
		float_volt = battery->pdata->dc_step_chg_vol_offset[i] + battery->pdata->chg_float_voltage;

		if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE)
			if (battery->pdata->dc_step_chg_val_vfloat[age_step][i] > float_volt)
				battery->pdata->dc_step_chg_val_vfloat[age_step][i] = float_volt;
		max_fv = max(max_fv, battery->pdata->dc_step_chg_val_vfloat[age_step][i]);

		if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_VOLTAGE)
			if (battery->pdata->dc_step_chg_cond_vol[age_step][i] > float_volt)
				battery->pdata->dc_step_chg_cond_vol[age_step][i] = float_volt;
	}

	for (i = 0; i < battery->dc_step_chg_step; i++) {
		dev_info(battery->dev, "%s: cond_vol: %dmV, vfloat: %dmV, cond_iin: %dmA, iout: %dmA\n", __func__,
			battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_VOLTAGE ?
				battery->pdata->dc_step_chg_cond_vol[age_step][i] : 0,
			battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE ?
				battery->pdata->dc_step_chg_val_vfloat[age_step][i] : 0,
			battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_INPUT_CURRENT ?
				battery->pdata->dc_step_chg_cond_iin[dc_op_mode][i] : 0,
			battery->pdata->dc_step_chg_val_iout[age_step][i]);
	}

	i = (battery->step_chg_status < 0 ? 0 : battery->step_chg_status);
	if (battery->dc_step_chg_type[i] & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
		val.intval = battery->pdata->dc_step_chg_val_vfloat[age_step][battery->dc_step_chg_step-1];
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_DIRECT_CONSTANT_CHARGE_VOLTAGE_MAX, val);
	}

	sec_vote(battery->dc_fv_vote, VOTER_AGING_STEP, true, max_fv);

	sec_bat_reset_step_charging(battery);
	sec_bat_check_dc_step_charging(battery);
#endif
}
EXPORT_SYMBOL(sec_bat_set_aging_info_step_charging);

void sec_step_charging_dt(struct sec_battery_info *battery, struct device *dev)
{
	struct device_node *np = dev->of_node;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int num_age_step = battery->pdata->num_age_step;

	int len;
	long ret;
	const u32 *p;

	battery->step_charging_skip_lcd_on = of_property_read_bool(np,
						     "battery,step_charging_skip_lcd_on");
	battery->step_chg_en_in_factory = of_property_read_bool(np,
						     "battery,step_chg_en_in_factory");

	ret = of_property_read_u32(np, "battery,step_chg_step",
			&battery->step_chg_step);
	if (ret) {
		pr_err("%s: step_chg_step is Empty\n", __func__);
		battery->step_chg_step = 0;
	} else {
		pr_err("%s: step_chg_step is %d\n",
			__func__, battery->step_chg_step);
	}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	ret = of_property_read_u32(np, "battery,step_chg_vsublim", &pdata->step_chg_vsublim);
	if (ret)
		pdata->step_chg_vsublim = 0;

	pr_err("%s: step_chg_vsublim is %d\n", __func__, pdata->step_chg_vsublim);
#endif

	ret = of_property_read_u32(np, "battery,step_chg_charge_power",
			&battery->step_chg_charge_power);
	if (ret) {
		pr_err("%s: step_chg_charge_power is Empty\n", __func__);
		battery->step_chg_charge_power = 20000;
	}

	p = of_get_property(np, "battery,step_chg_cond", &len);
	if (!p) {
		battery->step_chg_step = 0;
	} else {
		len = len / sizeof(u32);
		pr_info("%s: step(%d) * age_step(%d), step_chg_cond len(%d)\n",
			__func__, battery->step_chg_step, num_age_step, len);

		pdata->step_chg_cond = alloc_2darr(num_age_step, battery->step_chg_step);
		if (IS_ERR(pdata->step_chg_cond)) {
			ret = PTR_ERR(pdata->step_chg_cond);
		} else {
			ret = init_2darr_with_dt(np, "battery,step_chg_cond",
							pdata->step_chg_cond, num_age_step, battery->step_chg_step,
							(num_age_step * battery->step_chg_step) != len);

			print_log_for_step_charging_dt_2darr(pdata->step_chg_cond,
				num_age_step, battery->step_chg_step, "step_chg_cond");
		}

		if (ret) {
			pr_info("%s : step_chg_cond read fail\n", __func__);
			battery->step_chg_step = 0;
		}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
			unsigned int i = 0, j = 0;

			pdata->step_chg_cond_sub = alloc_2darr(num_age_step, battery->step_chg_step);
			if (IS_ERR(pdata->step_chg_cond_sub)) {
				ret = PTR_ERR(pdata->step_chg_cond_sub);
			} else {
				p = of_get_property(np, "battery,step_chg_cond_sub", &len);
				if (!p) {
					pr_info("%s: step_chg_cond_sub is Empty\n", __func__);
					for (i = 0; i < num_age_step; i++) {
						for (j = 0; j < battery->step_chg_step; j++)
							pdata->step_chg_cond_sub[i][j] = pdata->step_chg_cond[i][j];
					}
					ret = 0;
				} else {
					len = len / sizeof(u32);
					pr_info("%s: step(%d) * age_step(%d), step_chg_cond_sub len(%d)\n",
						__func__, battery->step_chg_step, num_age_step, len);

					ret = init_2darr_with_dt(np, "battery,step_chg_cond_sub",
									pdata->step_chg_cond_sub, num_age_step, battery->step_chg_step,
									(num_age_step * battery->step_chg_step) != len);
				}
				print_log_for_step_charging_dt_2darr(pdata->step_chg_cond_sub,
					num_age_step, battery->step_chg_step, "step_chg_cond_sub");
			}

			if (ret)
				pr_info("%s : step_chg_cond_sub read fail\n", __func__);

			battery->pdata->step_chg_use_vnow = of_property_read_bool(np, "battery,step_chg_use_vnow");
		}
#endif

		p = of_get_property(np, "battery,step_chg_cond_curr", &len);
		if (!p) {
			pr_err("%s: step_chg_cond_curr is Empty\n", __func__);
		} else {
			len = len / sizeof(u32);
			pdata->step_chg_cond_curr = kcalloc(len, sizeof(u32), GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,step_chg_cond_curr",
					pdata->step_chg_cond_curr, len);
			if (ret) {
				pr_info("%s : step_chg_cond_curr read fail\n", __func__);
				battery->step_chg_step = 0;
			}
		}

		p = of_get_property(np, "battery,step_chg_vfloat", &len);
		if (!p) {
			pr_err("%s: step_chg_vfloat is Empty\n", __func__);
		} else {
			len = len / sizeof(u32);
			pr_info("%s: step(%d) * age_step(%d), step_chg_vfloat len(%d)\n",
				__func__, battery->step_chg_step, num_age_step, len);

			pdata->step_chg_vfloat = alloc_2darr(num_age_step, battery->step_chg_step);
			if (IS_ERR(pdata->step_chg_vfloat)) {
				ret = PTR_ERR(pdata->step_chg_vfloat);
			} else {
				ret = init_2darr_with_dt(np, "battery,step_chg_vfloat",
								pdata->step_chg_vfloat, num_age_step, battery->step_chg_step,
								(num_age_step * battery->step_chg_step) != len);

				print_log_for_step_charging_dt_2darr(pdata->step_chg_vfloat,
					num_age_step, battery->step_chg_step, "step_chg_vfloat");
			}

			if (ret)
				pr_info("%s : step_chg_vfloat read fail\n", __func__);
		}

		p = of_get_property(np, "battery,step_chg_curr", &len);
		if (!p) {
			pr_err("%s: step_chg_curr is Empty\n", __func__);
		} else {
			len = len / sizeof(u32);
			pr_info("%s: step(%d) * age_step(%d), step_chg_curr len(%d)\n",
				__func__, battery->step_chg_step, num_age_step, len);

			pdata->step_chg_curr = alloc_2darr(num_age_step, battery->step_chg_step);
			if (IS_ERR(pdata->step_chg_curr)) {
				ret = PTR_ERR(pdata->step_chg_curr);
			} else {
				ret = init_2darr_with_dt(np, "battery,step_chg_curr",
								pdata->step_chg_curr, num_age_step, battery->step_chg_step,
								(num_age_step * battery->step_chg_step) != len);

				print_log_for_step_charging_dt_2darr(pdata->step_chg_curr,
					num_age_step, battery->step_chg_step, "step_chg_curr");
			}

			if (ret)
				pr_info("%s : step_chg_curr read fail\n", __func__);
		}

		p = of_get_property(np, "battery,step_chg_cond_soc", &len);
		if (!p) {
			pr_err("%s: step_chg_cond_soc is Empty\n", __func__);
			battery->step_chg_type =
				(battery->step_chg_type) & (~STEP_CHARGING_CONDITION_SOC_INIT_ONLY);
		} else {
			len = len / sizeof(u32);
			pr_info("%s: step(%d) * age_step(%d), step_chg_soc len(%d)\n",
				__func__, battery->step_chg_step, num_age_step, len);

			if (battery->step_chg_step * num_age_step != len) {
				pr_err("%s: mis-match len!!\n", __func__);
				battery->step_chg_type =
					(battery->step_chg_type) & (~STEP_CHARGING_CONDITION_SOC_INIT_ONLY);
				goto err_soc;
			}

			pdata->step_chg_cond_soc = alloc_2darr(num_age_step, battery->step_chg_step);
			if (IS_ERR(pdata->step_chg_cond_soc)) {
				ret = PTR_ERR(pdata->step_chg_cond_soc);
			} else {
				ret = init_2darr_with_dt(np, "battery,step_chg_cond_soc",
									pdata->step_chg_cond_soc, num_age_step, battery->step_chg_step,
									false);

				print_log_for_step_charging_dt_2darr(pdata->step_chg_cond_soc,
					num_age_step, battery->step_chg_step, "step_chg_cond_soc");
			}

			if (ret) {
				pr_err("%s: step_chg_cond_soc read fail\n", __func__);
				battery->step_chg_type =
					(battery->step_chg_type) & (~STEP_CHARGING_CONDITION_SOC_INIT_ONLY);
				goto err_soc;
			}

err_soc:
			pr_info("%s: step_chg_soc end\n", __func__);
		}
	}
}

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
void sec_wpc_step_charging_dt(struct sec_battery_info *battery, struct device *dev)
{
	struct device_node *np = dev->of_node;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int num_age_step = battery->pdata->num_age_step;

	int len;
	long ret;
	const u32 *p;

	ret = of_property_read_u32(np, "battery,wpc_step_chg_step",
			&battery->wpc_step_chg_step);
	if (ret) {
		pr_err("%s: wpc_step_chg_step is Empty\n", __func__);
		battery->wpc_step_chg_step = 0;
	} else {
		pr_err("%s: wpc_step_chg_step is %d\n",
			__func__, battery->wpc_step_chg_step);
	}

	ret = of_property_read_u32(np, "battery,wpc_step_chg_charge_power",
			&battery->wpc_step_chg_charge_power);
	if (ret) {
		pr_err("%s: wpc_step_chg_charge_power is Empty\n", __func__);
		battery->wpc_step_chg_charge_power = 7500;
	}

	p = of_get_property(np, "battery,wpc_step_chg_cond", &len);
	if (!p) {
		battery->wpc_step_chg_step = 0;
	} else {
		len = len / sizeof(u32);
		pr_info("%s: step(%d) * age_step(%d), step_chg_cond len(%d)\n",
			__func__, battery->wpc_step_chg_step, num_age_step, len);

		pdata->wpc_step_chg_cond = alloc_2darr(num_age_step, battery->wpc_step_chg_step);
		if (IS_ERR(pdata->wpc_step_chg_cond)) {
			ret = PTR_ERR(pdata->wpc_step_chg_cond);
		} else {
			ret = init_2darr_with_dt(np, "battery,wpc_step_chg_cond",
								pdata->wpc_step_chg_cond, num_age_step, battery->wpc_step_chg_step,
								(num_age_step * battery->wpc_step_chg_step) != len);

			print_log_for_step_charging_dt_2darr(pdata->wpc_step_chg_cond,
				num_age_step, battery->wpc_step_chg_step, "wpc_step_chg_cond");
		}

		if (ret) {
			pr_info("%s : wpc_step_chg_cond read fail\n", __func__);
			battery->wpc_step_chg_step = 0;
		}

		p = of_get_property(np, "battery,wpc_step_chg_cond_curr", &len);
		if (!p) {
			pr_err("%s: wpc_step_chg_cond_curr is Empty\n", __func__);
		} else {
			len = len / sizeof(u32);
			pdata->wpc_step_chg_cond_curr = kcalloc(len, sizeof(u32), GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,wpc_step_chg_cond_curr",
					pdata->wpc_step_chg_cond_curr, len);
			if (ret) {
				pr_info("%s : wpc_step_chg_cond_curr read fail\n", __func__);
				battery->wpc_step_chg_step = 0;
			}
		}

		p = of_get_property(np, "battery,wpc_step_chg_vfloat", &len);
		if (!p) {
			pr_err("%s: wpc_step_chg_vfloat is Empty\n", __func__);
		} else {
			len = len / sizeof(u32);
			pr_info("%s: step(%d) * age_step(%d), wpc_step_chg_vfloat len(%d)\n",
				__func__, battery->wpc_step_chg_step, num_age_step, len);

			pdata->wpc_step_chg_vfloat = alloc_2darr(num_age_step, battery->wpc_step_chg_step);
			if (IS_ERR(pdata->wpc_step_chg_vfloat)) {
				ret = PTR_ERR(pdata->wpc_step_chg_vfloat);
			} else {
				ret = init_2darr_with_dt(np, "battery,wpc_step_chg_vfloat",
								pdata->wpc_step_chg_vfloat, num_age_step, battery->wpc_step_chg_step,
								(num_age_step * battery->wpc_step_chg_step) != len);

				print_log_for_step_charging_dt_2darr(pdata->wpc_step_chg_vfloat,
					num_age_step, battery->wpc_step_chg_step, "wpc_step_chg_vfloat");
			}

			if (ret)
				pr_info("%s : wpc_step_chg_vfloat read fail\n", __func__);
		}

		p = of_get_property(np, "battery,wpc_step_chg_curr", &len);
		if (!p) {
			pr_err("%s: wpc_step_chg_curr is Empty\n", __func__);
		} else {
			len = len / sizeof(u32);
			pr_info("%s: step(%d) * age_step(%d), wpc_step_chg_curr len(%d)\n",
				__func__, battery->wpc_step_chg_step, num_age_step, len);

			pdata->wpc_step_chg_curr = alloc_2darr(num_age_step, battery->wpc_step_chg_step);
			if (IS_ERR(pdata->wpc_step_chg_curr)) {
				ret = PTR_ERR(pdata->wpc_step_chg_curr);
			} else {
				ret = init_2darr_with_dt(np, "battery,wpc_step_chg_curr",
								pdata->wpc_step_chg_curr, num_age_step, battery->wpc_step_chg_step,
								(num_age_step * battery->wpc_step_chg_step) != len);

				print_log_for_step_charging_dt_2darr(pdata->wpc_step_chg_curr,
					num_age_step, battery->wpc_step_chg_step, "wpc_step_chg_curr");
			}

			if (ret)
				pr_info("%s : wpc_step_chg_curr read fail\n", __func__);
		}
	}
}
#endif

void sec_step_charging_init(struct sec_battery_info *battery, struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret;

	battery->step_chg_status = -1;

	ret = of_property_read_u32(np, "battery,step_chg_type",
			&battery->step_chg_type);
	pr_err("%s: step_chg_type 0x%x\n", __func__, battery->step_chg_type);
	if (ret) {
		pr_err("%s: step_chg_type is Empty\n", __func__);
		battery->step_chg_type = 0;
	}

	if (battery->step_chg_type)
		sec_step_charging_dt(battery, dev);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	ret = of_property_read_u32(np, "battery,wpc_step_chg_type",
			&battery->wpc_step_chg_type);
	pr_err("%s: wpc_step_chg_type 0x%x\n", __func__, battery->wpc_step_chg_type);
	if (ret) {
		pr_err("%s: wpc_step_chg_type is Empty\n", __func__);
		battery->wpc_step_chg_type = 0;
	}

	if (battery->wpc_step_chg_type)
		sec_wpc_step_charging_dt(battery, dev);
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	sec_dc_step_charging_dt(battery, dev);
#endif
}
EXPORT_SYMBOL(sec_step_charging_init);
