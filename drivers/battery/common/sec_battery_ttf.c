/*
 * sec_battery_ttf.c
 * Samsung Mobile Battery Driver
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "sec_battery.h"
#include "sec_battery_ttf.h"

#define is_ttf_thermal_zone(thermal_zone) ( \
	thermal_zone == BAT_THERMAL_NORMAL || \
	thermal_zone == BAT_THERMAL_COOL1 || \
	thermal_zone == BAT_THERMAL_COOL2)

static bool skip_ttf_event(unsigned int misc_event)
{
	return (misc_event & BATT_MISC_EVENT_PASS_THROUGH);
}

static bool check_ttf_state(unsigned int capacity, int bat_sts)
{
	return ((bat_sts == POWER_SUPPLY_STATUS_CHARGING) ||
		(bat_sts == POWER_SUPPLY_STATUS_FULL && capacity != 100));
}

static int get_cc_cv_time(struct sec_battery_info * battery, int ttf_curr, int soc, bool minimum)
{
	struct sec_cv_slope *cv_data = battery->ttf_d->cv_data;
	int i, design_cap = battery->ttf_d->ttf_capacity;
	int cc_time = 0, cv_time = 0;
	int minimum_time = 0;

	for (i = 0; i < battery->ttf_d->cv_data_length; i++) {
		if (ttf_curr >= cv_data[i].fg_current)
			break;
	}
	i = i >= battery->ttf_d->cv_data_length ? battery->ttf_d->cv_data_length - 1 : i;
	if (cv_data[i].soc < soc) {
		for (i = 0; i < battery->ttf_d->cv_data_length; i++) {
			if (soc <= cv_data[i].soc)
				break;
		}
		cv_time =
		    ((cv_data[i - 1].time - cv_data[i].time) * (cv_data[i].soc - soc)
		     / (cv_data[i].soc - cv_data[i - 1].soc)) + cv_data[i].time;
	} else {		/* CC mode || NONE */
		cv_time = cv_data[i].time;
		cc_time =
			design_cap * (cv_data[i].soc - soc) / ttf_curr * 3600 / 1000;
		pr_debug("%s: cc_time: %d\n", __func__, cc_time);
		if (cc_time < 0)
			cc_time = 0;
	}

	pr_info("%s: cap: %d, soc: %4d, T: %6d, avg: %4d, cv soc: %4d, i: %4d, val: %d, minimum:%d\n",
	     __func__, design_cap, soc, cv_time + cc_time,
	     battery->current_avg, cv_data[i].soc, i, ttf_curr, minimum);

	if (minimum)
		minimum_time = 60;

	return ((cc_time + cv_time >= 0) ? (cc_time + cv_time + minimum_time) : minimum_time);
}

static int get_current_soc( char *name)
{
	union power_supply_propval value = {0, };

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
	psy_do_property(name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);

	return value.intval;
}

int sec_calc_ttf(struct sec_battery_info * battery, unsigned int ttf_curr)
{
	struct sec_cv_slope *cv_data = battery->ttf_d->cv_data;
	int total_time;

	if (!cv_data || (ttf_curr <= 0)) {
		pr_info("%s: no cv_data or val: %d\n", __func__, ttf_curr);
		return -1;
	}

	total_time = get_cc_cv_time(battery, ttf_curr, get_current_soc(battery->pdata->fuelgauge_name), true);
	if (is_full_capacity(battery->fs)) {
		int now_full_cap = get_full_capacity(battery->fs);

		pr_info("%s: time to %d percent\n", __func__, now_full_cap);
		total_time -= get_cc_cv_time(battery, ttf_curr, (now_full_cap * 10), false);
	}

	return total_time;
}

int check_hv_wireless_current(struct ttf_charge_current currents, unsigned int wc20_rx_power, int cable_type, unsigned int wc_budg_pwr)
{
	int charge = 0;

	if ((cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 && !sec_bat_get_lpmode()) ||
		cable_type == SEC_BATTERY_CABLE_HV_WIRELESS_20) {
		pr_info("%s : POWER_SUPPLY_EXT_PROP_TX_PWR_BUDG(%d)\n",
				__func__, wc_budg_pwr);
		if (wc20_rx_power >= WFC21_WIRELESS_POWER || wc_budg_pwr >= RX_POWER_15W)
			charge = currents.wc21_wireless;
		else if (wc20_rx_power >= WFC20_WIRELESS_POWER || wc_budg_pwr >= RX_POWER_12W)
			charge = currents.wc20_wireless;
		else if (wc20_rx_power >= WFC10_WIRELESS_POWER || wc_budg_pwr >= RX_POWER_7_5W)
			charge = currents.hv_wireless;
		else
			charge = currents.wireless;
	} else {
		charge = currents.hv_wireless;
	}

	return charge;
}

int check_epp_mpp_current(struct ttf_charge_current currents, unsigned int wc20_rx_power)
{
	int charge = 0;

	if (wc20_rx_power >= WFC21_WIRELESS_POWER)
		charge = currents.wc21_wireless;
	else if (wc20_rx_power >= WFC20_WIRELESS_POWER)
		charge = currents.wc20_wireless;
	else if (wc20_rx_power >= WFC10_WIRELESS_POWER)
		charge = currents.hv_wireless;
	else
		charge = currents.wireless;

	return charge;
}

int sec_get_ttf_standard_curr(struct sec_battery_info *battery)
{
	int charge = 0;

	if (is_hv_wire_12v_type(battery->cable_type)) {
		charge = battery->ttf_d->currents.hv_12v;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	} else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP) {
		union power_supply_propval value = {0, };

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_NEGO_DONE_PWR, value);
		charge = check_epp_mpp_current(battery->ttf_d->currents,
				(value.intval > battery->wc20_rx_power ? value.intval : battery->wc20_rx_power));
	} else if (is_epp_wireless_type(battery->cable_type)) {
		charge = check_epp_mpp_current(battery->ttf_d->currents, battery->wc20_rx_power);
	} else if (is_hv_wireless_type(battery->cable_type) ||
		battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV ||
		battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20) {
		if (sec_bat_hv_wc_normal_mode_check(battery)) {
			charge = battery->ttf_d->currents.wireless;
		} else {
			union power_supply_propval value = {0, };

			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_TX_PWR_BUDG, value);
			charge = check_hv_wireless_current(battery->ttf_d->currents, battery->wc20_rx_power, battery->cable_type, value.intval);
		}
	} else if (is_nv_wireless_type(battery->cable_type)) {
		charge = battery->ttf_d->currents.wireless;
#endif
	} else if (is_hv_wire_type(battery->cable_type)) {
		charge = battery->ttf_d->currents.hv;
	} else if (is_pd_apdo_wire_type(battery->cable_type) ||
		(is_pd_fpdo_wire_type(battery->cable_type) && battery->hv_pdo)) {
		if (battery->pd_max_charge_power > HV_CHARGER_STATUS_STANDARD4) {
			charge = battery->ttf_d->currents.dc45;
		} else if (battery->pd_max_charge_power > HV_CHARGER_STATUS_STANDARD3) {
			charge = battery->ttf_d->currents.dc25;
		} else if (battery->pd_max_charge_power <= battery->pdata->pd_charging_charge_power &&
			battery->pdata->charging_current[battery->cable_type].fast_charging_current >= \
			battery->pdata->max_charging_current) { /* same PD power with AFC */
			charge = battery->ttf_d->currents.hv;
		} else { /* other PD charging */
			charge = (battery->pd_max_charge_power / 5) > battery->pdata->charging_current[battery->cable_type].fast_charging_current ?
				battery->pdata->charging_current[battery->cable_type].fast_charging_current : (battery->pd_max_charge_power / 5);
		}
	} else {
		charge = (battery->max_charge_power / 5) > battery->pdata->charging_current[battery->cable_type].fast_charging_current ?
				battery->pdata->charging_current[battery->cable_type].fast_charging_current : (battery->max_charge_power / 5);
	}

	if (battery->cable_type == SEC_BATTERY_CABLE_FPDO_DC)
		charge = battery->ttf_d->currents.fpdo_dc;

	return charge;
}
EXPORT_SYMBOL_KUNIT(sec_get_ttf_standard_curr);

static int check_ttf_valid(struct sec_ttf_data *ttf_d, int ttf)
{
	if (ttf_d->timetofull < 0) {
		ttf_d->old_timetofull = ttf;
		return ttf;
	}

	if (ttf > ttf_d->timetofull) {
		if (ttf > ttf_d->old_timetofull) {
			int ret = ttf_d->old_timetofull;

			ttf_d->old_timetofull = ttf;
			return ret;
		}
	}

	ttf_d->old_timetofull = ttf;
	return ttf;
}

static void init_ttf(struct sec_ttf_data *ttf_d)
{
	ttf_d->timetofull = -1;
	ttf_d->old_timetofull = -1;
}

void sec_bat_calc_time_to_full(struct sec_battery_info * battery)
{
	if (delayed_work_pending(&battery->ttf_d->timetofull_work)) {
		pr_info("%s: keep time_to_full(%5d sec)\n", __func__, battery->ttf_d->timetofull);
	} else if (check_ttf_state(battery->capacity, battery->status) &&
		!battery->wc_tx_enable && !skip_ttf_event(battery->misc_event)) {
		int charge = 0, ttf = 0;

		charge = sec_get_ttf_standard_curr(battery);
		ttf = sec_calc_ttf(battery, charge);
		battery->ttf_d->timetofull = check_ttf_valid(battery->ttf_d, ttf);
		dev_info(battery->dev, "%s: T: %5d|%5d sec, passed time: %5ld, current: %d\n",
				__func__, battery->ttf_d->timetofull, ttf, battery->charging_passed_time, charge);
	} else {
		init_ttf(battery->ttf_d);
	}
}

int sec_ttf_parse_dt(struct sec_battery_info *battery)
{
	struct device_node *np;
	struct sec_ttf_data *pdata = battery->ttf_d;
	sec_battery_platform_data_t *bpdata = battery->pdata;
	int ret = 0, len = 0;
	const u32 *p;

	pdata->pdev = battery;
	np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_info("%s: np NULL\n", __func__);
			return 1;
	}

	ret = of_property_read_u32(np, "battery,ttf_hv_12v_charge_current",
					&pdata->currents.hv_12v);
	if (ret) {
		pdata->currents.hv_12v =
			bpdata->charging_current[SEC_BATTERY_CABLE_12V_TA].fast_charging_current;
		pr_info("%s: ttf_hv_12v_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.hv_12v);
	}
	ret = of_property_read_u32(np, "battery,ttf_hv_charge_current",
					&pdata->currents.hv);
	if (ret) {
		pdata->currents.hv =
			bpdata->charging_current[SEC_BATTERY_CABLE_9V_TA].fast_charging_current;
		pr_info("%s: ttf_hv_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.hv);
	}

	ret = of_property_read_u32(np, "battery,ttf_hv_wireless_charge_current",
					&pdata->currents.hv_wireless);
	if (ret) {
		pdata->currents.hv_wireless =
			bpdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS].fast_charging_current - 300;
		pr_info("%s: ttf_hv_wireless_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.hv_wireless);
	}

	ret = of_property_read_u32(np, "battery,ttf_wc20_wireless_charge_current",
					&pdata->currents.wc20_wireless);
	if (ret) {
		pdata->currents.wc20_wireless =
			bpdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].fast_charging_current - 300;
		pr_info("%s: ttf_wc20_wireless_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.wc20_wireless);
	}

	ret = of_property_read_u32(np, "battery,ttf_wc21_wireless_charge_current",
					&pdata->currents.wc21_wireless);
	if (ret) {
		pdata->currents.wc21_wireless =
			bpdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].fast_charging_current - 300;
		pr_info("%s: ttf_wc21_wireless_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.wc21_wireless);
	}

	ret = of_property_read_u32(np, "battery,ttf_wireless_charge_current",
					&pdata->currents.wireless);
	if (ret) {
		pdata->currents.wireless =
			bpdata->charging_current[SEC_BATTERY_CABLE_WIRELESS].input_current_limit;
		pr_info("%s: ttf_wireless_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.wireless);
	}

	ret = of_property_read_u32(np, "battery,ttf_dc25_charge_current",
					&pdata->currents.dc25);
	if (ret) {
		pdata->currents.dc25 =
			bpdata->charging_current[SEC_BATTERY_CABLE_9V_TA].fast_charging_current;
		pr_info("%s: ttf_dc25_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.dc25);
	}

	ret = of_property_read_u32(np, "battery,ttf_dc45_charge_current",
					&pdata->currents.dc45);
	if (ret) {
		pdata->currents.dc45 = pdata->currents.dc25;
		pr_info("%s: ttf_dc45_charge_current is Empty, Default value %d \n",
			__func__, pdata->currents.dc45);
	}

	ret = of_property_read_u32(np, "battery,ttf_fpdo_dc_charge_current",
					&pdata->currents.fpdo_dc);
	if (ret) {
		pdata->currents.fpdo_dc = pdata->currents.hv;
		pr_info("%s: ttf_fpdo_dc_charge_current is Empty, Default value %d\n",
			__func__, pdata->currents.fpdo_dc);
	}

	ret = of_property_read_u32(np, "battery,ttf_capacity",
					   &pdata->ttf_capacity);
	if (ret < 0) {
		pr_err("%s error reading capacity_calculation_type %d\n", __func__, ret);
		pdata->ttf_capacity = bpdata->battery_full_capacity;
	}

	p = of_get_property(np, "battery,cv_data", &len);
	if (p) {
		pdata->cv_data = kzalloc(len, GFP_KERNEL);
		pdata->cv_data_length = len / sizeof(struct sec_cv_slope);
		pr_err("%s: len= %ld, length= %d, %d\n", __func__,
		       sizeof(int) * len, len, pdata->cv_data_length);
		ret = of_property_read_u32_array(np, "battery,cv_data",
				(u32 *)pdata->cv_data, len / sizeof(u32));
		if (ret) {
			pr_err("%s: failed to read battery->cv_data: %d\n",
				__func__, ret);
			kfree(pdata->cv_data);
			pdata->cv_data = NULL;
		}
	} else {
		pr_err("%s: there is not cv_data\n", __func__);
	}
	return 0;
}

void sec_bat_time_to_full_work(struct work_struct *work)
{
	struct sec_ttf_data *dev = container_of(work,
				struct sec_ttf_data, timetofull_work.work);
	struct sec_battery_info *battery = dev->pdev;
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);
	battery->current_max = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg = value.intval;

	sec_bat_calc_time_to_full(battery);
	dev_info(battery->dev, "%s:\n",__func__);
	if (battery->voltage_now > 0)
		battery->voltage_now--;

	power_supply_changed(battery->psy_bat);
}

void ttf_work_start(struct sec_battery_info *battery)
{
	if (sec_bat_get_lpmode()) {
		cancel_delayed_work(&battery->ttf_d->timetofull_work);
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
			int work_delay = 0;

			if (!is_wireless_type(battery->cable_type)) {
				work_delay = battery->pdata->pre_afc_work_delay;
			} else {
				work_delay = battery->pdata->pre_wc_afc_work_delay;
			}
			queue_delayed_work(battery->monitor_wqueue,
				&battery->ttf_d->timetofull_work, msecs_to_jiffies(work_delay));
		}
	}
}

int ttf_display(unsigned int capacity, int bat_sts, int thermal_zone, int time)
{
	if (capacity == 100)
		return 0;

	if (check_ttf_state(capacity, bat_sts) &&
		is_ttf_thermal_zone(thermal_zone))
		return time;

	return 0;
}
EXPORT_SYMBOL_KUNIT(ttf_display);

void ttf_init(struct sec_battery_info *battery)
{
	battery->ttf_d = kzalloc(sizeof(struct sec_ttf_data),
		GFP_KERNEL);
	if (!battery->ttf_d) {
		pr_err("Failed to allocate memory\n");
	}
	sec_ttf_parse_dt(battery);
	init_ttf(battery->ttf_d);

	INIT_DELAYED_WORK(&battery->ttf_d->timetofull_work, sec_bat_time_to_full_work);
}
EXPORT_SYMBOL_KUNIT(ttf_init);
