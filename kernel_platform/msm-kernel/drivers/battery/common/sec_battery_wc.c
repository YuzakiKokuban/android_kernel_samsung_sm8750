/*
 *  sec_battery_wc.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2020 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "sec_battery.h"
#include "sb_tx.h"

#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#else
#define __visible_for_testing static
#endif

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
bool sec_bat_check_boost_mfc_condition(struct sec_battery_info *battery, int mode)
{
	union power_supply_propval value = {0, };
	int boost_status = 0, wpc_det = 0, mst_pwr_en = 0;

	pr_info("%s\n", __func__);

	if (mode == SEC_WIRELESS_FW_UPDATE_AUTO_MODE) {
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_INITIAL_WC_CHECK, value);
		wpc_det = value.intval;
	}

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_MST_PWR_EN, value);
	mst_pwr_en = value.intval;

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_CHARGE_BOOST, value);
	boost_status = value.intval;

	pr_info("%s wpc_det(%d), mst_pwr_en(%d), boost_status(%d)\n",
		__func__, wpc_det, mst_pwr_en, boost_status);

	if (!boost_status && !wpc_det && !mst_pwr_en)
		return true;
	return false;
}

void sec_bat_fw_update(struct sec_battery_info *battery, int mode)
{
	union power_supply_propval value = {0, };
	int ret = 0;

	pr_info("%s\n", __func__);

	__pm_wakeup_event(battery->vbus_ws, jiffies_to_msecs(HZ * 10));

	switch (mode) {
	case SEC_WIRELESS_FW_UPDATE_SDCARD_MODE:
	case SEC_WIRELESS_FW_UPDATE_BUILTIN_MODE:
	case SEC_WIRELESS_FW_UPDATE_AUTO_MODE:
	case SEC_WIRELESS_FW_UPDATE_SPU_MODE:
	case SEC_WIRELESS_FW_UPDATE_SPU_VERIFY_MODE:
		battery->mfc_fw_update = true;
		sec_vote(battery->chgen_vote, VOTER_FW, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		msleep(500);
		sec_vote(battery->iv_vote, VOTER_FW, true, SEC_INPUT_VOLTAGE_5V);
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if !IS_ENABLED(CONFIG_MTK_CHARGER) || !IS_ENABLED(CONFIG_AFC_CHARGER)
		muic_afc_request_voltage(AFC_REQUEST_MFC, SEC_INPUT_VOLTAGE_5V / 1000);
#endif
#endif
		msleep(2000);
		value.intval = mode;
		ret = psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_CHARGE_POWERED_OTG_CONTROL, value);
		if (ret < 0) {
			battery->mfc_fw_update = false;
			sec_vote(battery->chgen_vote, VOTER_FW, false, 0);
			sec_vote(battery->iv_vote, VOTER_FW, false, 0);
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if !IS_ENABLED(CONFIG_MTK_CHARGER) || !IS_ENABLED(CONFIG_AFC_CHARGER)
			muic_afc_request_voltage(AFC_REQUEST_MFC, SEC_INPUT_VOLTAGE_9V / 1000);
#endif
#endif
		}
		break;
	default:
		break;
	}
}
#endif

int sec_bat_check_wpc_vout(struct sec_battery_info *battery, int ct, unsigned int chg_limit,
		int pre_vout, unsigned int evt)
{
	union power_supply_propval value = {0, };
	int vout = 0;
	bool check_flicker_wa = false;

	if (!is_hv_wireless_type(ct)) {
		pr_info("%s:\n", __func__);
		return WIRELESS_VOUT_5V;
	}
	if (ct == SEC_BATTERY_CABLE_WIRELESS_MPP)
		vout = MFC_VOUT_13V;
	else if ((ct == SEC_BATTERY_CABLE_HV_WIRELESS_20) || (ct == SEC_BATTERY_CABLE_WIRELESS_EPP))
		vout = battery->wpc_max_vout_level;
	else
		vout = WIRELESS_VOUT_10V;

	mutex_lock(&battery->voutlock);
	if (battery->pdata->wpc_vout_ctrl_lcd_on) {
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
		if ((value.intval != WC_PAD_UNKNOWN) &&
			volt_ctrl_pad(value.intval)) {
			if (battery->wpc_vout_ctrl_mode && battery->lcd_status) {
				pr_info("%s: trigger flicker wa\n", __func__);
				check_flicker_wa = true;
			} else {
				value.intval = 0;
				psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);
				if (!value.intval) {
					pr_info("%s: recover flicker wa\n", __func__);
					value.intval = battery->lcd_status;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL, value);
				}
			}
		}
	}

	/* get vout level */
	psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_RX_VOUT, value);

	if (value.intval == WIRELESS_VOUT_5_5V_STEP)
		pre_vout = WIRELESS_VOUT_5_5V_STEP;

	if ((evt & (SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING | SEC_BAT_CURRENT_EVENT_ISDB)) ||
			battery->sleep_mode || chg_limit || check_flicker_wa)
		vout = WIRELESS_VOUT_5_5V_STEP;

	pr_info("%s: prev_vout(%d) => vout(%d)\n", __func__, pre_vout, vout);
	if (vout != pre_vout) {
		if (evt & SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK) {
			vout = pre_vout;
			pr_info("%s: block to set wpc vout level(%d) because otg on\n",
					__func__, vout);
		} else {
			value.intval = vout;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
			pr_info("%s: change vout level(%d)", __func__, vout);
			sec_vote(battery->input_vote, VOTER_AICL, false, 0);
		}
	} else if ((vout == WIRELESS_VOUT_10V ||
				vout == battery->wpc_max_vout_level)) {
		/* reset aicl current to recover current for unexpected aicl during */
		/* before vout boosting completion */
		sec_vote(battery->input_vote, VOTER_AICL, false, 0);
	}
	mutex_unlock(&battery->voutlock);
	return vout;
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_wpc_vout);

void sec_wireless_otg_vout_control(struct sec_battery_info *battery, int enable)
{
	union power_supply_propval value = {0, };

	if (enable) {
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK,
			SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK);
	} else {
		sec_bat_set_current_event(battery, 0,
			SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK);
	}

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_OP_MODE, value);
	if (value.intval == 0x4) { // MFC_RX_MODE_WPC_MPP_FULL
		pr_info("%s: @mpp : WIRELESS_MPP %d\n", __func__, value.intval);
		value.intval = enable;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
		return;
	}

	value.intval = enable;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);

	if (enable) {
		if (battery->wc_tx_enable) {
			/* TX power should turn off during otg on */
			pr_info("@Tx_Mode %s: OTG is going to work, TX power should off\n", __func__);
			/* set tx event */
			sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_OTG_ON, BATT_TX_EVENT_WIRELESS_TX_OTG_ON);
			sec_wireless_set_tx_enable(battery, false);
		} else {
			battery->wpc_vout_level = WIRELESS_VOUT_5V;
		}
	} else if (is_wireless_all_type(battery->cable_type)) {
		if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
			((battery->charging_mode == SEC_BATTERY_CHARGING_2ND) || battery->is_recharging)) {
			psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);

			if (value.intval >= MFC_VOUT_5_5V)
				battery->wpc_vout_level = WIRELESS_VOUT_5_5V_STEP;
			else
				battery->wpc_vout_level = WIRELESS_VOUT_CC_CV_VOUT;
			value.intval = WIRELESS_VOUT_CC_CV_VOUT;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_1ST_DONE, value);
		} else {
			battery->wpc_vout_level = sec_bat_check_wpc_vout(battery, battery->cable_type, battery->chg_limit,
				battery->wpc_vout_level, battery->current_event);
		}
		sec_wireless_otg_icl_control(battery);
	}
}

unsigned int get_wc_vout_mW(unsigned int vout)
{
	unsigned int ret = 5000;

	switch (vout) {
	case WIRELESS_VOUT_5V:
		ret = 5000;
		break;
	case WIRELESS_VOUT_5_5V:
	case WIRELESS_VOUT_5_5V_STEP:
	case WIRELESS_VOUT_CC_CV_VOUT:
		ret = 5500;
		break;
	case WIRELESS_VOUT_9V:
		ret = 9000;
		break;
	case WIRELESS_VOUT_10V:
		ret = 10000;
		break;
	case WIRELESS_VOUT_11V:
		ret = 11000;
		break;
	case WIRELESS_VOUT_12V:
		ret = 12000;
		break;
	case WIRELESS_VOUT_12_5V:
		ret = 12500;
		break;
	case WIRELESS_VOUT_13V:
		ret = 13000;
		break;
	default:
		pr_err("%s vout is not supported\n", __func__);
		break;
	}

	pr_info("%s vout(%d) -> vout(%dmV)\n", __func__, vout, ret);
	return ret;
}

void sec_bat_set_wc20_current(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	int icl = 0, fcc = 0;

	pr_info("%s: wc_status(%d), rx_power(%d), vout(%d)\n", __func__,
		battery->wc_status, battery->wc20_rx_power, battery->wc20_vout);

	if (is_pwr_nego_wireless_type(battery->wc_status)) {
		icl = (battery->wc20_rx_power / battery->wc20_vout);
		fcc = battery->pdata->charging_current[battery->wc_status].fast_charging_current;

		if (battery->wc20_rx_power <= SEC_WIRELESS_RX_POWER_5W) {
			battery->wc20_power_class = 0;
		} else if (battery->wc20_rx_power <= SEC_WIRELESS_RX_POWER_7_5W) {
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_1;
		} else if (battery->wc20_rx_power <= SEC_WIRELESS_RX_POWER_12W) {
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_2;
			if (battery->pdata->max_wlc_icl_12w > 0 && battery->pdata->max_wlc_icl_12w < icl)
				icl = battery->pdata->max_wlc_icl_12w;
		} else if (battery->wc20_rx_power <= SEC_WIRELESS_RX_POWER_20W) {
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_3;
			if (battery->pdata->max_wlc_icl_15w > 0 && battery->pdata->max_wlc_icl_15w < icl)
				icl = battery->pdata->max_wlc_icl_15w;
		} else {
			battery->wc20_power_class = SEC_WIRELESS_RX_POWER_CLASS_4;
		}

		sec_bat_change_default_current(battery, battery->wc_status, icl, fcc);
		if (battery->wc_status != SEC_BATTERY_CABLE_WIRELESS_MPP) {
			sec_vote(battery->input_vote, VOTER_CABLE, true, icl);
			sec_vote(battery->fcc_vote, VOTER_CABLE, true, fcc);
		}
		if (is_wired_type(battery->cable_type)) {
			int wl_power = battery->wc20_rx_power;

			pr_info("%s: check power(%d <--> %d)\n",
				__func__, battery->max_charge_power, wl_power);
			if (battery->max_charge_power < wl_power) {
				__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			}
		} else {
			if (battery->wc_status != SEC_BATTERY_CABLE_WIRELESS_MPP)
				sec_bat_set_charging_current(battery);
			else {
				value.intval = 1;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_MPP_INC_INT_CTRL, value);
				value.intval = icl;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_MPP_PWR, value);
			}
		}
	}
}

void sec_wireless_otg_icl_control(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	bool need_vote_refresh = false;

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_OP_MODE, value);
	if (value.intval == 0x4 && battery->is_otg_on) { // MFC_RX_MODE_WPC_MPP_FULL
		pr_info("%s: @mpp : WIRELESS_MPP %d, otg_on %d\n", __func__, value.intval, battery->is_otg_on);
		sec_vote(battery->input_vote, VOTER_OTG, true, 600);
		msleep(50);
		sec_vote(battery->input_vote, VOTER_OTG, true, 400);
		return;
	}

	pr_info("%s: is_otg_on=%s\n", __func__, battery->is_otg_on ? "ON" : "OFF");

	if (battery->is_otg_on) {
		if (get_sec_vote_result(battery->input_vote) <= battery->pdata->wireless_otg_input_current)
			need_vote_refresh = true;
		sec_vote(battery->input_vote, VOTER_OTG, true, battery->pdata->wireless_otg_input_current);
		if (need_vote_refresh)
			sec_vote_refresh(battery->input_vote);
	} else {
		sec_vote(battery->input_vote, VOTER_OTG, false, 0);
	}
	sec_vote(battery->input_vote, VOTER_AICL, false, 0);
}

void sec_bat_set_mfc_off(struct sec_battery_info *battery, char flag, bool need_ept)
{
	union power_supply_propval value = {0, };
	char wpc_en_status[2];

	if (need_ept) {
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_ONLINE, value);

		msleep(300);
	}

	wpc_en_status[0] = flag;
	wpc_en_status[1] = false;
	value.strval = wpc_en_status;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_WPC_EN, value);

	pr_info("@DIS_MFC %s: WC CONTROL: Disable %d\n", __func__, flag);
}

void sec_bat_set_mfc_on(struct sec_battery_info *battery, char flag)
{
	union power_supply_propval value = {0, };
	char wpc_en_status[2];

	wpc_en_status[0] = flag;
	wpc_en_status[1] = true;
	value.strval = wpc_en_status;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_WPC_EN, value);

	pr_info("%s: WC CONTROL: Enable %d\n", __func__, flag);
}

void sec_bat_mfc_ldo_cntl(struct sec_battery_info *battery, bool en)
{
	union power_supply_propval value = {0, };

	battery->wc_need_ldo_on = !en;
	value.intval = en;

	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_EMPTY, value);
	if (en) {
		battery->wpc_vout_level = WIRELESS_VOUT_5V;
	} else {
		battery->wpc_vout_level = sec_bat_check_wpc_vout(battery, battery->cable_type, battery->chg_limit,
			battery->wpc_vout_level, battery->current_event);
	}

	if (battery->disable_mfc) {
		pr_info("%s : set mfc %s\n", __func__, (en ? "on" : "off"));
		if (en)
			sec_bat_set_mfc_on(battery, WPC_EN_CHARGING);
		else
			sec_bat_set_mfc_off(battery, WPC_EN_CHARGING, false);
	}
}

__visible_for_testing int sec_bat_get_wire_power(struct sec_battery_info *battery, int wr_sts)
{
	int wr_pwr = 0;
	int wr_icl = 0, wr_vol = 0;

	if (!is_wired_type(wr_sts))
		return 0;
	if (is_pd_wire_type(wr_sts))
		return battery->pd_max_charge_power;

	wr_icl = (wr_sts == SEC_BATTERY_CABLE_PREPARE_TA ?
			battery->pdata->charging_current[SEC_BATTERY_CABLE_TA].input_current_limit :
			battery->pdata->charging_current[wr_sts].input_current_limit);
	wr_vol = is_hv_wire_type(wr_sts) ?
		(wr_sts == SEC_BATTERY_CABLE_12V_TA ? SEC_INPUT_VOLTAGE_12V : SEC_INPUT_VOLTAGE_9V)
		: SEC_INPUT_VOLTAGE_5V;

	wr_pwr = mW_by_mVmA(wr_vol, wr_icl);
	pr_info("%s: wr_power(%d), wire_cable_type(%d)\n", __func__, wr_pwr, wr_sts);

	return wr_pwr;
}

__visible_for_testing int sec_bat_get_wireless_power(struct sec_battery_info *battery, int wrl_sts)
{
	int wrl_icl = 0, wrl_pwr = 0;

	if (wrl_sts == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)
		wrl_sts = SEC_BATTERY_CABLE_HV_WIRELESS;
	else if (wrl_sts == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)
		wrl_sts = SEC_BATTERY_CABLE_HV_WIRELESS_20;

	wrl_icl = battery->pdata->charging_current[wrl_sts].input_current_limit;

	if (battery->sleep_mode)
		wrl_pwr = mW_by_mVmA(SEC_INPUT_VOLTAGE_5_5V, battery->pdata->sleep_mode_limit_current);
	else if (is_nv_wireless_type(wrl_sts) || (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_FAKE))
		wrl_pwr = mW_by_mVmA(SEC_INPUT_VOLTAGE_5_5V, wrl_icl);
	else if (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_MPP || wrl_sts == SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE)
		wrl_pwr = max(mW_by_mVmA(battery->wc20_vout, wrl_icl), mW_by_mVmA(SEC_INPUT_VOLTAGE_13V, 800));
	else if (is_pwr_nego_wireless_type(wrl_sts) || (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE))
		wrl_pwr = mW_by_mVmA(battery->wc20_vout, wrl_icl);
	else
		wrl_pwr = mW_by_mVmA(SEC_INPUT_VOLTAGE_10V, wrl_icl);

	return wrl_pwr;
}

__visible_for_testing void sec_bat_switch_to_wr(struct sec_battery_info *battery, int wrl_sts, int prev_ct)
{
	union power_supply_propval val = {0, };

	pr_info("%s wrl_sts(%d) prev_ct(%d)\n", __func__, wrl_sts, prev_ct);

	if (wrl_sts == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)
		wrl_sts = SEC_BATTERY_CABLE_HV_WIRELESS_20;
	/* limit charging current before change path between chgin and wcin */
	if (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_MPP) {
		sec_vote(battery->input_vote, VOTER_WPC_CUR, true, 400);
		msleep(200);
	} else if (is_pwr_nego_wireless_type(prev_ct) &&
			is_pwr_nego_wireless_type(wrl_sts)) {
		/* limit charging current before change path between chgin and wcin */
		pr_info("%s: set charging current %dmA for a moment in case of TA OCP\n",
				__func__, battery->pdata->wpc_charging_limit_current);
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, battery->pdata->wpc_charging_limit_current);
		msleep(100);
	}
	if (wrl_sts != SEC_BATTERY_CABLE_WIRELESS_MPP)
		sec_bat_mfc_ldo_cntl(battery, MFC_LDO_OFF);

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_OP_MODE, val);
	pr_info("%s: @mpp : op_mode %d\n", __func__, val.intval);
	if (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_MPP &&
		val.intval != MFC_RX_MODE_WPC_MPP_CLOAK && val.intval) {
		val.intval = CLOAK_GENERIC;
		psy_do_property("wireless", set,
			POWER_SUPPLY_EXT_PROP_MPP_CLOAK, val);
		pr_info("@MPP %s: MFC LDO OFF toggle --> cloak(%d)\n", __func__, val.intval);
	}

	/* Turn off TX to charge by cable charging having more power */
	if (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_TX) {
		pr_info("@Tx_Mode %s: RX device with TA, notify TX device of this info\n",
				__func__);
		val.intval = true;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_SWITCH, val);
	}
	if (wrl_sts != SEC_BATTERY_CABLE_WIRELESS_MPP) {
		pr_info("%s: wrl_sts(%d)\n", __func__, wrl_sts);
		sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_WIRELESS_MPP,
				battery->pdata->default_mpp_input_current,
				battery->pdata->default_mpp_charging_current);
	}
}

__visible_for_testing void sec_bat_switch_to_wrl(struct sec_battery_info *battery, int wr_sts, int prev_ct)
{
	pr_info("%s prev_ct=%d wr_sts=%d\n", __func__, prev_ct, wr_sts);

	if (!is_wireless_type(prev_ct)) {
		/* limit charging current before change path between chgin and wcin */
		pr_info("%s: set charging current %dmA for a moment in case of TA OCP\n",
				__func__, battery->pdata->wpc_charging_limit_current);
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, battery->pdata->wpc_charging_limit_current);
		msleep(100);
	}
	/* turn on ldo when ldo was off because of TA, */
	/* ldo is supposed to turn on automatically except force off by sw. */
	/* do not turn on ldo every wireless connection just in case ldo re-toggle by ic */
	if (!is_nocharge_type(wr_sts)) {
		sec_bat_mfc_ldo_cntl(battery, MFC_LDO_ON);
	}
}

int sec_bat_choose_cable_type(struct sec_battery_info *battery)
{
	int wr_sts = battery->wire_status;
	int cur_ct = wr_sts;
	int prev_ct = battery->cable_type;
	int wrl_sts = battery->wc_status;
	union power_supply_propval value = {0, };

	if (wrl_sts != SEC_BATTERY_CABLE_NONE) {
		cur_ct = wrl_sts;
		if (!is_nocharge_type(wr_sts)) {
			int wrl_pwr, wr_pwr;

			wr_pwr = sec_bat_get_wire_power(battery, wr_sts);
			wrl_pwr = sec_bat_get_wireless_power(battery, wrl_sts);
			if (wrl_pwr <= wr_pwr)
				cur_ct = wr_sts;
			pr_info("%s: wrl_pwr(%d), wr_pwr(%d), wc_sts(%d), wr_sts(%d), cur_ct(%d)\n",
					__func__, wrl_pwr, wr_pwr, wrl_sts, wr_sts, cur_ct);
			if (is_wireless_type(cur_ct))
				sec_bat_switch_to_wrl(battery, wr_sts, prev_ct);
			else {
				if (is_hv_wireless_type(wrl_sts) && (wrl_sts != SEC_BATTERY_CABLE_WIRELESS_MPP)) {
					value.intval = WIRELESS_VOUT_FORCE_9V;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
					msleep(200);
					value.intval = WIRELESS_VOUT_5V;
					psy_do_property(battery->pdata->wireless_charger_name, set,
						POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
				} else if (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_MPP) {
					if (!is_nocharge_type(wr_sts)) {
						pr_info("@MPP %s : send ept rst for MPP\n", __func__);
						value.intval = EPT_RESTART;
						psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_WC_EPT_UNKNOWN, value);
					}
				}
				if (is_wireless_all_type(prev_ct)) {
					msleep(200);
					sec_vote(battery->fcc_vote, VOTER_WL_TO_W, true, 300);
					msleep(200);
					sec_vote(battery->fcc_vote, VOTER_WL_TO_W, true, 100);
					msleep(200);
					sec_vote(battery->chgen_vote, VOTER_WL_TO_W, true, SEC_BAT_CHG_MODE_BUCK_OFF);
					value.intval = WL_TO_W;
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_EXT_PROP_CHGINSEL, value);
				}
				sec_bat_switch_to_wr(battery, wrl_sts, prev_ct);
				sec_vote(battery->input_vote, VOTER_WPC_CUR, false, 0);
				sec_vote(battery->fcc_vote, VOTER_WL_TO_W, false, 0);
				sec_vote(battery->chgen_vote, VOTER_WL_TO_W, false, 0);
			}
		} else {
			if (battery->wc_need_ldo_on) {
				pr_info("%s : wc_need_ldo_on %d\n", __func__, battery->wc_need_ldo_on);

				psy_do_property(battery->pdata->wireless_charger_name, get,
					POWER_SUPPLY_EXT_PROP_WIRELESS_OP_MODE, value);
				pr_info("%s: @mpp : op_mode %d\n", __func__, value.intval);
				if (wrl_sts == SEC_BATTERY_CABLE_WIRELESS_MPP &&
					value.intval == MFC_RX_MODE_WPC_MPP_CLOAK) {
					value.intval = CLOAK_EXIT_CMD;
					psy_do_property("wireless", set,
						POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
					pr_info("@MPP %s: MFC LDO ON toggle --> cloak(%d)\n", __func__, value.intval);
				} else if (wrl_sts != SEC_BATTERY_CABLE_WIRELESS_FAKE)
					sec_bat_mfc_ldo_cntl(battery, MFC_LDO_ON);
			}
		}
	} else if (is_nocharge_type(wr_sts) && battery->disable_mfc) {
		pr_info("%s : sec_bat_set_mfc_on because of CABLE_NONE\n", __func__);
		sec_bat_set_mfc_on(battery, WPC_EN_CHARGING);
	}

	return cur_ct;
}

void sec_bat_get_wireless_current(struct sec_battery_info *battery)
{
	int incurr = INT_MAX;
	union power_supply_propval value = {0, };

	if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP)
		return;

	/* WPC_SLEEP_MODE */
	if (is_hv_wireless_type(battery->cable_type) && battery->sleep_mode) {
		if (incurr > battery->pdata->sleep_mode_limit_current)
			incurr = battery->pdata->sleep_mode_limit_current;
		pr_info("%s: sleep_mode = %d, chg_limit = %d, in_curr = %d\n",
				__func__, battery->sleep_mode, battery->chg_limit, incurr);

		if (!battery->auto_mode) {
			/* send cmd once */
			battery->auto_mode = true;
			value.intval = WIRELESS_SLEEP_MODE_ENABLE;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);
		}
	}

	/* WPC_TEMP_MODE */
	if (is_wireless_type(battery->cable_type) && battery->chg_limit) {
		if ((battery->siop_level >= 100 && !battery->lcd_status) &&
				(incurr > battery->pdata->wpc_input_limit_current)) {
			if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX &&
				battery->pdata->wpc_input_limit_by_tx_check)
				incurr = battery->pdata->wpc_input_limit_current_by_tx;
			else
				incurr = battery->pdata->wpc_input_limit_current;
		} else if ((battery->siop_level < 100 || battery->lcd_status) &&
				(incurr > battery->pdata->wpc_lcd_on_input_limit_current))
			incurr = battery->pdata->wpc_lcd_on_input_limit_current;
	}

	/* Display flicker W/A */
	if (battery->pdata->wpc_vout_ctrl_lcd_on && battery->wpc_vout_ctrl_mode && battery->lcd_status) {
		if (is_wireless_type(battery->cable_type) && battery->pdata->wpc_flicker_wa_input_limit_current) {
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
			if ((value.intval != WC_PAD_UNKNOWN) &&
				volt_ctrl_pad(value.intval)) {
				pr_info("%s: trigger flicker wa\n", __func__);
				if (incurr > battery->pdata->wpc_flicker_wa_input_limit_current)
					incurr = battery->pdata->wpc_flicker_wa_input_limit_current;
			}
		}
	}

	/* Full-Additional state */
	if (battery->status == POWER_SUPPLY_STATUS_FULL) {
		if ((incurr > battery->pdata->siop_hv_wpc_icl) &&
			(battery->charging_mode == SEC_BATTERY_CHARGING_2ND))
			incurr = battery->pdata->siop_hv_wpc_icl;
		if ((incurr > battery->pdata->rechg_hv_wpc_icl) && battery->is_recharging)
			incurr = battery->pdata->rechg_hv_wpc_icl;
	}

	/* Hero Stand Pad CV */
	if (battery->capacity >= battery->pdata->wc_hero_stand_cc_cv) {
		if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND) {
			if (incurr > battery->pdata->wc_hero_stand_cv_current)
				incurr = battery->pdata->wc_hero_stand_cv_current;
		} else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_STAND) {
			if (battery->chg_limit &&
					incurr > battery->pdata->wc_hero_stand_cv_current) {
				incurr = battery->pdata->wc_hero_stand_cv_current;
			} else if (!battery->chg_limit &&
					incurr > battery->pdata->wc_hero_stand_hv_cv_current) {
				incurr = battery->pdata->wc_hero_stand_hv_cv_current;
			}
		}
	}

	/* Full-None state && SIOP_LEVEL 100 */
	if ((battery->siop_level >= 100 && !battery->lcd_status) &&
		battery->status == POWER_SUPPLY_STATUS_FULL && battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
		incurr = battery->pdata->wc_full_input_limit_current;
	}

	if (incurr != INT_MAX)
		sec_vote(battery->input_vote, VOTER_WPC_CUR, true, incurr);
	else
		sec_vote(battery->input_vote, VOTER_WPC_CUR, false, incurr);
}

int sec_bat_check_wc_available(struct sec_battery_info *battery)
{
	mutex_lock(&battery->wclock);
	if (!battery->wc_enable) {
		pr_info("%s: wc_enable(%d), cnt(%d)\n", __func__, battery->wc_enable, battery->wc_enable_cnt);
		if (battery->wc_enable_cnt > battery->wc_enable_cnt_value) {
			union power_supply_propval val = {0, };
			char wpc_en_status[2];

			battery->wc_enable = true;
			battery->wc_enable_cnt = 0;
			wpc_en_status[0] = WPC_EN_SYSFS;
			wpc_en_status[1] = true;
			val.strval = wpc_en_status;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WPC_EN, val);
			pr_info("%s: WC CONTROL: Enable\n", __func__);
		}
		battery->wc_enable_cnt++;
	}
	mutex_unlock(&battery->wclock);

	return 0;
}

/* OTG during HV wireless charging or sleep mode have 4.5W normal wireless charging UI */
bool sec_bat_hv_wc_normal_mode_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
	if (value.intval || battery->sleep_mode) {
		pr_info("%s: otg(%d), sleep_mode(%d)\n", __func__, value.intval, battery->sleep_mode);
		return true;
	}
	return false;
}
EXPORT_SYMBOL_KUNIT(sec_bat_hv_wc_normal_mode_check);

void sec_bat_ext_event_work_content(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (battery->wc_tx_enable) { /* TX ON state */
		if (battery->ext_event & BATT_EXT_EVENT_CAMERA) {
			pr_info("@Tx_Mode %s: Camera ON, TX OFF\n", __func__);
			sec_bat_set_tx_event(battery,
					BATT_TX_EVENT_WIRELESS_TX_CAMERA_ON, BATT_TX_EVENT_WIRELESS_TX_CAMERA_ON);
			sec_wireless_set_tx_enable(battery, false);
		} else if (battery->ext_event & BATT_EXT_EVENT_DEX) {
			pr_info("@Tx_Mode %s: Dex ON, TX OFF\n", __func__);
			sec_bat_set_tx_event(battery,
					BATT_TX_EVENT_WIRELESS_TX_OTG_ON, BATT_TX_EVENT_WIRELESS_TX_OTG_ON);
			sec_wireless_set_tx_enable(battery, false);
		} else if (battery->ext_event & BATT_EXT_EVENT_CALL) {
			pr_info("@Tx_Mode %s: Call ON, TX OFF\n", __func__);
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_CALL;
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
			sec_wireless_set_tx_enable(battery, false);
		}
	} else { /* TX OFF state, it has only call scenario */
		if (battery->ext_event & BATT_EXT_EVENT_CALL) {
			pr_info("@Tx_Mode %s: Call ON\n", __func__);

			value.intval = BATT_EXT_EVENT_CALL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_CALL_EVENT, value);

#if !defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_HV_PACK ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
				pr_info("%s: Call is on during Wireless Pack or TX\n", __func__);
				battery->wc_rx_phm_mode = true;
			}
#endif
			if (battery->tx_retry_case != SEC_BAT_TX_RETRY_NONE) {
				pr_info("@Tx_Mode %s: TX OFF because of other reason(retry:0x%x), save call retry case\n",
					__func__, battery->tx_retry_case);
				battery->tx_retry_case |= SEC_BAT_TX_RETRY_CALL;
			}
		} else if (!(battery->ext_event & BATT_EXT_EVENT_CALL)) {
			pr_info("@Tx_Mode %s: Call OFF\n", __func__);

			value.intval = BATT_EXT_EVENT_NONE;
			psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_CALL_EVENT, value);

			/* check the diff between current and previous ext_event state */
			if (battery->tx_retry_case & SEC_BAT_TX_RETRY_CALL) {
				battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_CALL;
				if (!battery->tx_retry_case) {
					pr_info("@Tx_Mode %s: Call OFF, TX Retry\n", __func__);
					sec_bat_set_tx_event(battery,
							BATT_TX_EVENT_WIRELESS_TX_RETRY,
							BATT_TX_EVENT_WIRELESS_TX_RETRY);
				}
			}
#if !defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			/* process escape phm */
			if (battery->wc_rx_phm_mode) {
				pr_info("%s: ESCAPE PHM STEP 1\n", __func__);
				sec_bat_set_mfc_on(battery, WPC_EN_CHARGING);
				msleep(100);

				pr_info("%s: ESCAPE PHM STEP 2\n", __func__);
				sec_bat_set_mfc_off(battery, WPC_EN_CHARGING, false);
				msleep(510);

				pr_info("%s: ESCAPE PHM STEP 3\n", __func__);
				sec_bat_set_mfc_on(battery, WPC_EN_CHARGING);
			}
			battery->wc_rx_phm_mode = false;
#endif
		}
	}

	__pm_relax(battery->ext_event_ws);
}

void sec_bat_wireless_minduty_cntl(struct sec_battery_info *battery, unsigned int duty_val)
{
	union power_supply_propval value = {0, };

	if (duty_val != battery->tx_minduty) {
		value.intval = duty_val;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_MIN_DUTY, value);

		pr_info("@Tx_Mode %s: Min duty changed (%d -> %d)\n", __func__, battery->tx_minduty, duty_val);
		battery->tx_minduty = duty_val;
	}
}

void sec_bat_wireless_set_ping_duty(struct sec_battery_info *battery, unsigned int ping_duty_val)
{
	union power_supply_propval value = {0, };

	if (ping_duty_val != battery->tx_ping_duty) {
		value.intval = ping_duty_val;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_PING_DUTY, value);

		pr_info("@Tx_Mode %s: Ping duty changed (%d -> %d)\n",
			__func__, battery->tx_ping_duty, ping_duty_val);
		battery->tx_ping_duty = ping_duty_val;
	}
}

void sec_bat_wireless_uno_cntl(struct sec_battery_info *battery, bool en)
{
	union power_supply_propval value = {0, };

	value.intval = en;
	battery->uno_en = en;
	pr_info("@Tx_Mode %s: Uno control %d\n", __func__, en);

	psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE, value);

	sb_tx_set_enable(en, 0);
}

void sec_bat_wireless_iout_cntl(struct sec_battery_info *battery, int uno_iout, int mfc_iout)
{
	union power_supply_propval value = {0, };

	if (battery->tx_uno_iout != uno_iout) {
		pr_info("@Tx_Mode %s: set uno iout(%d) -> (%d)\n", __func__, battery->tx_uno_iout, uno_iout);
		value.intval = battery->tx_uno_iout = uno_iout;
		psy_do_property("otg", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);
	} else {
		pr_info("@Tx_Mode %s: Already set Uno Iout(%d == %d)\n", __func__, battery->tx_uno_iout, uno_iout);
	}

#if !defined(CONFIG_SEC_FACTORY)
	if (battery->lcd_status && (mfc_iout == battery->pdata->tx_mfc_iout_phone)) {
		pr_info("@Tx_Mode %s: Reduce Tx MFC Iout. LCD ON\n", __func__);
		mfc_iout = battery->pdata->tx_mfc_iout_lcd_on;
	}
#endif

	if (battery->tx_mfc_iout != mfc_iout) {
		pr_info("@Tx_Mode %s: set mfc iout(%d) -> (%d)\n", __func__, battery->tx_mfc_iout, mfc_iout);
		value.intval = battery->tx_mfc_iout = mfc_iout;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_IOUT, value);
	} else {
		pr_info("@Tx_Mode %s: Already set MFC Iout(%d == %d)\n", __func__, battery->tx_mfc_iout, mfc_iout);
	}
}

void sec_bat_wireless_vout_cntl(struct sec_battery_info *battery, int vout_now)
{
	union power_supply_propval value = {0, };

	pr_info("@Tx_Mode %s: set uno & mfc vout (%dmV -> %dmV)\n", __func__, battery->wc_tx_vout, vout_now);

	if (battery->wc_tx_vout >= vout_now) {
		battery->wc_tx_vout = value.intval = vout_now;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
		psy_do_property("otg", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
	} else if (vout_now > battery->wc_tx_vout) {
		battery->wc_tx_vout = value.intval = vout_now;
		psy_do_property("otg", set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
	}

}

#if defined(CONFIG_WIRELESS_TX_MODE)
#if !defined(CONFIG_SEC_FACTORY)
static void sec_bat_check_tx_battery_drain(struct sec_battery_info *battery)
{
	if (battery->capacity <= battery->pdata->tx_stop_capacity &&
		is_nocharge_type(battery->cable_type)) {
		pr_info("@Tx_Mode %s: battery level is drained, TX mode should turn off\n", __func__);
		/* set tx event */
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_SOC_DRAIN, BATT_TX_EVENT_WIRELESS_TX_SOC_DRAIN);
		sec_wireless_set_tx_enable(battery, false);
	}
}

static void sec_bat_check_tx_current(struct sec_battery_info *battery)
{
	if (battery->lcd_status && (battery->tx_mfc_iout > battery->pdata->tx_mfc_iout_lcd_on)) {
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_lcd_on);
		pr_info("@Tx_Mode %s: Reduce Tx MFC Iout. LCD ON\n", __func__);
	} else if (!battery->lcd_status && (battery->tx_mfc_iout == battery->pdata->tx_mfc_iout_lcd_on)) {
		union power_supply_propval value = {0, };

		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
		pr_info("@Tx_Mode %s: Recovery Tx MFC Iout. LCD OFF\n", __func__);

		value.intval = true;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK, value);
	}
}
#endif

static void sec_bat_check_tx_switch_mode(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	/* temporary mode */
	if (battery->tx_switch_mode == TX_SWITCH_GEAR_PPS) {
		pr_info("@Tx_mode %s: skip routine in gear pps mode.\n", __func__);
		return;
	}

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC)	{
		pr_info("@Tx_mode %s: Do not switch switch mode! AFC Event set\n", __func__);
		return;
	}

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_CAPACITY_POINT;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);

	if ((battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) && (!battery->buck_cntl_by_tx)) {
		battery->buck_cntl_by_tx = true;
		sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);

		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
		sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_uno_vout);
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
	} else if ((battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) && (battery->buck_cntl_by_tx)) {
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone_5v);
		sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_ping_vout);
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_5V);

		battery->buck_cntl_by_tx = false;
		sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
	}

	if (battery->status == POWER_SUPPLY_STATUS_FULL) {
		if (battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
			if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY)
				battery->tx_switch_mode_change = true;
		} else {
			if (battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) {
				if (battery->tx_switch_start_soc >= 100) {
					if (battery->capacity < 99 || (battery->capacity == 99 && value.intval <= 1))
						battery->tx_switch_mode_change = true;
				} else {
					if ((battery->capacity == battery->tx_switch_start_soc && value.intval <= 1) ||
					(battery->capacity < battery->tx_switch_start_soc))
						battery->tx_switch_mode_change = true;
				}
			} else if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) {
				if (battery->capacity >= 100)
					battery->tx_switch_mode_change = true;
			}
		}
	} else {
		if (battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) {
			if (((battery->capacity == battery->tx_switch_start_soc) && (value.intval <= 1)) ||
				(battery->capacity < battery->tx_switch_start_soc))
				battery->tx_switch_mode_change = true;

		} else if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) {
			if (((battery->capacity == (battery->tx_switch_start_soc + 1)) && (value.intval >= 8)) ||
				(battery->capacity > (battery->tx_switch_start_soc + 1)))
				battery->tx_switch_mode_change = true;
		}
	}
	pr_info("@Tx_mode Tx mode(%d) tx_switch_mode_change(%d) start soc(%d) now soc(%d.%d)\n",
		battery->tx_switch_mode, battery->tx_switch_mode_change,
		battery->tx_switch_start_soc, battery->capacity, value.intval);
}
#endif

void sec_bat_txpower_calc(struct sec_battery_info *battery)
{
	if (delayed_work_pending(&battery->wpc_txpower_calc_work)) {
		pr_info("%s: keep average tx power(%5d mA)\n", __func__, battery->tx_avg_curr);
	} else if (battery->wc_tx_enable) {
		int tx_vout = 0, tx_iout = 0, vbatt = 0;
		union power_supply_propval value = {0, };

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN, value);
		tx_vout = value.intval;

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		tx_iout = value.intval;

		value.intval = SEC_BATTERY_VOLTAGE_MV;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		vbatt = value.intval;

		battery->tx_time_cnt++;

		/* AVG curr will be calculated only when the battery is discharged */
		if (battery->current_avg <= 0 && vbatt > 0 && tx_vout > 0 && tx_iout > 0)
			tx_iout = (tx_vout / vbatt) * tx_iout;
		else
			tx_iout = 0;

		/* monitor work will be scheduled every 10s when wc_tx_enable is true */
		battery->tx_avg_curr =
			((battery->tx_avg_curr * battery->tx_time_cnt) + tx_iout) / (battery->tx_time_cnt + 1);
		battery->tx_total_power =
			(battery->tx_avg_curr * battery->tx_time_cnt) / (60 * 60 / 10);

		if (battery->tx_total_power > ((battery->pdata->battery_full_capacity * 7) / 10)) {
			pr_info("%s: tx_total_power(%dmAh) is wrong\n", __func__, battery->tx_total_power);
			battery->tx_total_power = (battery->pdata->battery_full_capacity * 7) / 10;
		}

		pr_info("%s: tx_time_cnt(%ds), UNO_Vin(%dV), UNO_Iin(%dmA), tx_avg_curr(%dmA), tx_total_power(%dmAh)\n",
				__func__, battery->tx_time_cnt * 10, tx_vout, tx_iout, battery->tx_avg_curr, battery->tx_total_power);
	}
}

void sec_bat_tx_off_by_misalign_ocp(struct sec_battery_info *battery)
{
	battery->tx_retry_case &= ~(SEC_BAT_TX_RETRY_MISALIGN | SEC_BAT_TX_RETRY_OCP);
	battery->tx_misalign_start_time = 0;
	battery->tx_misalign_cnt = 0;
	battery->tx_ocp_start_time = 0;
	battery->tx_ocp_cnt = 0;
}

void sec_bat_handle_tx_misalign(struct sec_battery_info *battery, bool trigger_misalign)
{
	struct timespec64 ts = {0, };

	if (trigger_misalign && battery->wc_tx_enable) {
		if (battery->tx_misalign_start_time == 0) {
			ts = ktime_to_timespec64(ktime_get_boottime());
			battery->tx_misalign_start_time = ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: misalign is triggered!!(%d)\n", __func__, ++battery->tx_misalign_cnt);
		/* Attention!! in this case, 0x00(TX_OFF)  is sent first */
		/* and then 0x8000(RETRY) is sent */
		if (battery->tx_misalign_cnt < MISALIGN_TX_TRY_CNT) {
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_MISALIGN;
			sec_wireless_set_tx_enable(battery, false);
			msleep(1000);
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
			sec_bat_set_tx_event(battery,
				BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
			msleep(1000);
		} else {
			pr_info("@Tx_Mode %s: Misalign over %d times, TX OFF (cancel misalign)\n",
				__func__, MISALIGN_TX_TRY_CNT);
			sec_bat_tx_off_by_misalign_ocp(battery);
			sec_bat_set_tx_event(battery,
				BATT_TX_EVENT_WIRELESS_TX_MISALIGN, BATT_TX_EVENT_WIRELESS_TX_MISALIGN);
			sec_wireless_set_tx_enable(battery, false);
		}
	} else if (battery->tx_retry_case & SEC_BAT_TX_RETRY_MISALIGN) {
		ts = ktime_to_timespec64(ktime_get_boottime());
		if (ts.tv_sec >= battery->tx_misalign_start_time) {
			battery->tx_misalign_passed_time = ts.tv_sec - battery->tx_misalign_start_time;
		} else {
			battery->tx_misalign_passed_time = 0xFFFFFFFF - battery->tx_misalign_start_time
				+ ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: already misaligned, passed time(%ld)\n",
				__func__, battery->tx_misalign_passed_time);

		if (battery->tx_misalign_passed_time >= 60) {
			pr_info("@Tx_Mode %s: after 1min\n", __func__);
			if (battery->wc_tx_enable) {
				if (battery->wc_rx_connected) {
					pr_info("@Tx_Mode %s: RX Dev, Keep TX ON status (cancel misalign)\n", __func__);
				} else {
					pr_info("@Tx_Mode %s: NO RX Dev, TX OFF (cancel misalign)\n", __func__);
					sec_bat_tx_off_by_misalign_ocp(battery);
					sec_bat_set_tx_event(battery,
						BATT_TX_EVENT_WIRELESS_TX_MISALIGN, BATT_TX_EVENT_WIRELESS_TX_MISALIGN);
					sec_wireless_set_tx_enable(battery, false);
				}
			} else {
				pr_info("@Tx_Mode %s: Keep TX OFF status (cancel misalign)\n", __func__);
			}
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_MISALIGN;
			battery->tx_misalign_start_time = 0;
			battery->tx_misalign_cnt = 0;
		}
	}
}
EXPORT_SYMBOL_KUNIT(sec_bat_handle_tx_misalign);

void sec_bat_handle_tx_ocp(struct sec_battery_info *battery, bool trigger_ocp)
{
	struct timespec64 ts = {0, };

	if (trigger_ocp && battery->wc_tx_enable) {
		if (battery->tx_ocp_start_time == 0) {
			ts = ktime_to_timespec64(ktime_get_boottime());
			battery->tx_ocp_start_time = ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: ocp is triggered!!(%d)\n", __func__, ++battery->tx_ocp_cnt);
		/* Attention!! in this case, 0x00(TX_OFF)  is sent first */
		/* and then 0x8000(RETRY) is sent */
		if (battery->tx_ocp_cnt < 3) {
			battery->tx_retry_case |= SEC_BAT_TX_RETRY_OCP;
			sec_wireless_set_tx_enable(battery, false);
			msleep(1000);
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
			sec_bat_set_tx_event(battery,
				BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
			msleep(1000);
		} else {
			pr_info("@Tx_Mode %s: ocp over 3 times, TX OFF (cancel ocp)\n", __func__);
			sec_bat_tx_off_by_misalign_ocp(battery);
			sec_bat_set_tx_event(battery,
				BATT_TX_EVENT_WIRELESS_TX_OCP, BATT_TX_EVENT_WIRELESS_TX_OCP);
			sec_wireless_set_tx_enable(battery, false);
		}
	} else if (battery->tx_retry_case & SEC_BAT_TX_RETRY_OCP) {
		ts = ktime_to_timespec64(ktime_get_boottime());
		if (ts.tv_sec >= battery->tx_ocp_start_time) {
			battery->tx_ocp_passed_time = ts.tv_sec - battery->tx_ocp_start_time;
		} else {
			battery->tx_ocp_passed_time = 0xFFFFFFFF - battery->tx_ocp_start_time
				+ ts.tv_sec;
		}
		pr_info("@Tx_Mode %s: already ocp, passed time(%ld)\n",
				__func__, battery->tx_ocp_passed_time);

		if (battery->tx_ocp_passed_time >= 60) {
			pr_info("@Tx_Mode %s: after 1min\n", __func__);
			if (battery->wc_tx_enable) {
				if (battery->wc_rx_connected) {
					pr_info("@Tx_Mode %s: RX Dev, Keep TX ON status (cancel ocp)\n", __func__);
				} else {
					pr_info("@Tx_Mode %s: NO RX Dev, TX OFF (cancel ocp)\n", __func__);
					sec_bat_tx_off_by_misalign_ocp(battery);
					sec_bat_set_tx_event(battery,
						BATT_TX_EVENT_WIRELESS_TX_OCP, BATT_TX_EVENT_WIRELESS_TX_OCP);
					sec_wireless_set_tx_enable(battery, false);
				}
			} else {
				pr_info("@Tx_Mode %s: Keep TX OFF status (cancel ocp)\n", __func__);
			}
			battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_OCP;
			battery->tx_ocp_start_time = 0;
			battery->tx_ocp_cnt = 0;
		}
	}
}

void sec_bat_check_wc_re_auth(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	int tx_id = 0, auth_stat = 0;

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
	tx_id = value.intval;

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
	auth_stat = value.intval;

	pr_info("%s %s: tx_id(0x%x), cable(%d), soc(%d), auth_stat(%d)\n", WC_AUTH_MSG, __func__,
		tx_id, battery->cable_type, battery->capacity, auth_stat);

	if ((auth_stat == WIRELESS_AUTH_FAIL) && (battery->capacity >= 5)) {
		pr_info("%s %s: EPT Unknown for re-auth\n", WC_AUTH_MSG, __func__);
		value.intval = EPT_UNKNOWN;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WC_EPT_UNKNOWN, value);
		battery->wc_auth_retried = true;
	} else if (auth_stat == WIRELESS_AUTH_PASS) {
		pr_info("%s %s: auth success\n", WC_AUTH_MSG, __func__);
		battery->wc_auth_retried = true;
	} else if (((tx_id != WC_PAD_UNKNOWN) && (tx_id < WC_PAD_AUTH_PAD_START))
		|| (tx_id > WC_PAD_AUTH_PAD_END)
		|| ((tx_id == WC_PAD_UNKNOWN) && is_hv_wireless_type(battery->cable_type))) {
		pr_info("%s %s: re-auth is unnecessary\n", WC_AUTH_MSG, __func__);
		battery->wc_auth_retried = true;
	}
}

#if defined(CONFIG_WIRELESS_TX_MODE)
void sec_bat_check_tx_mode(struct sec_battery_info *battery)
{

	if (battery->wc_tx_enable) {
		pr_info("@Tx_Mode %s: tx_retry(0x%x), tx_switch(0x%x)",
			__func__, battery->tx_retry_case, battery->tx_switch_mode);
#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_check_tx_battery_drain(battery);
		sec_bat_check_tx_temperature(battery);

		if ((battery->wc_rx_type == SS_PHONE) ||
				(battery->wc_rx_type == OTHER_DEV) ||
				(battery->wc_rx_type == SS_BUDS))
			sec_bat_check_tx_current(battery);
#endif
		sec_bat_txpower_calc(battery);
		sec_bat_handle_tx_misalign(battery, false);
		sec_bat_handle_tx_ocp(battery, false);
		battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_AC_MISSING;

		if (battery->tx_switch_mode != TX_SWITCH_MODE_OFF && battery->tx_switch_start_soc != 0)
			sec_bat_check_tx_switch_mode(battery);

	} else if (battery->tx_retry_case != SEC_BAT_TX_RETRY_NONE) {
		pr_info("@Tx_Mode %s: tx_retry(0x%x)", __func__, battery->tx_retry_case);
#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_check_tx_temperature(battery);
#endif
		sec_bat_handle_tx_misalign(battery, false);
		sec_bat_handle_tx_ocp(battery, false);
		battery->tx_retry_case &= ~SEC_BAT_TX_RETRY_AC_MISSING;
	}
}
#endif

void sec_bat_wc_cv_mode_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	int is_otg_on = 0;

	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
	is_otg_on = value.intval;

	pr_info("%s: battery->wc_cv_mode = %d, otg(%d), cable_type(%d)\n", __func__,
		battery->wc_cv_mode, is_otg_on, battery->cable_type);

	if (battery->capacity >= battery->pdata->wireless_cc_cv && !is_otg_on) {
		battery->wc_cv_mode = true;
		if (is_nv_wireless_type(battery->cable_type)) {
			if ((battery->cable_type == SEC_BATTERY_CABLE_WIRELESS ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND ||
				battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX)) {
				value.intval = WIRELESS_CLAMP_ENABLE;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);
			}
		}
		/* Change FOD values for CV mode */
		value.intval = POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_STATUS, value);

		/* Change Vrect headroom for CV mode */
		if (battery->pdata->p2p_cv_headroom && battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX) {
			value.intval = WIRELESS_VRECT_ADJ_ROOM_2;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);
		}
	}
}
EXPORT_SYMBOL_KUNIT(sec_bat_wc_cv_mode_check);

static int is_5v_charger(struct sec_battery_info *battery, int wr_sts)
{
	if ((is_pd_wire_type(wr_sts) && battery->pd_list.max_pd_count > 1)
			|| is_hv_wire_12v_type(wr_sts)
			|| is_hv_wire_type(wr_sts)
			|| (wr_sts == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)
			|| (wr_sts == SEC_BATTERY_CABLE_PREPARE_TA)
			|| (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC))
		return false;
	else if (is_wired_type(wr_sts))
		return true;

	return false;
}

void sec_bat_run_wpc_tx_work(struct sec_battery_info *battery, int work_delay)
{
	if (delayed_work_pending(&battery->wpc_tx_work)) {
		pr_info("%s: did not push the tx_work because of already in queue.\n", __func__);
		return;
	}

	cancel_delayed_work(&battery->wpc_tx_work);
	__pm_stay_awake(battery->wpc_tx_ws);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->wpc_tx_work, msecs_to_jiffies(work_delay));
}

static void sec_bat_wpc_tx_iv(struct sec_vote *target_vote, int target_voltage, bool need_refresh)
{
	int voter_val = get_sec_vote_result(target_vote);

	pr_info("@Tx_mode %s : present iv = %d. target iv = %d\n", __func__, voter_val, target_voltage);

	sec_vote(target_vote, VOTER_WC_TX, true, target_voltage);

	if (need_refresh && (voter_val == target_voltage))
		sec_vote_refresh(target_vote);
}

static void sec_bat_tx_work_nodev(struct sec_battery_info *battery)
{
	if (battery->pdata->fcc_by_tx)
		sec_vote(battery->fcc_vote, VOTER_WC_TX, true,
				battery->pdata->fcc_by_tx);

	if (is_hv_wire_type(battery->wire_status) ||
		(is_pd_wire_type(battery->wire_status) && battery->hv_pdo)) {
		sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, true);
		return;
	}
	battery->buck_cntl_by_tx = true;
	sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
	if (!battery->uno_en)
		sec_bat_wireless_uno_cntl(battery, true);

	sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_ping_vout);
	sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout_aov_gear, battery->pdata->tx_mfc_iout_aov_gear);
	sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
}

static void sec_bat_tx_work_with_nv_charger(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (battery->pdata->tx_5v_disable)
		return;

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
		if (!battery->buck_cntl_by_tx) {
			battery->buck_cntl_by_tx = true;
			sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		}
		battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
		battery->tx_switch_start_soc = 0;
		battery->tx_switch_mode_change = false;

		sec_bat_wireless_iout_cntl(battery,
				battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
		sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_uno_vout);
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
	} else if (battery->tx_switch_mode == TX_SWITCH_MODE_OFF) {
		battery->tx_switch_mode = TX_SWITCH_UNO_ONLY;
		battery->tx_switch_start_soc = battery->capacity;
		if (!battery->buck_cntl_by_tx) {
			battery->buck_cntl_by_tx = true;
			sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		}
		sec_bat_wireless_iout_cntl(battery,
				battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
		sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_uno_vout);
		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);

	} else if (battery->tx_switch_mode_change == true) {
		battery->tx_switch_start_soc = battery->capacity;

		pr_info("@Tx_mode: Switch Mode Change(%d -> %d)\n",
				battery->tx_switch_mode,
				battery->tx_switch_mode == TX_SWITCH_UNO_ONLY ?
				TX_SWITCH_CHG_ONLY : TX_SWITCH_UNO_ONLY);

		if (battery->tx_switch_mode == TX_SWITCH_UNO_ONLY) {
			battery->tx_switch_mode = TX_SWITCH_CHG_ONLY;

			sec_bat_wireless_iout_cntl(battery,
					battery->pdata->tx_uno_iout,
					battery->pdata->tx_mfc_iout_phone_5v);
			sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_ping_vout);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_5V);

			if (battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = false;
				sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
			}

		} else if (battery->tx_switch_mode == TX_SWITCH_CHG_ONLY) {
			battery->tx_switch_mode = TX_SWITCH_UNO_ONLY;

			if (!battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = true;
				sec_vote(battery->chgen_vote,
						VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			}
			sec_bat_wireless_iout_cntl(battery,
					battery->pdata->tx_uno_iout,
					battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_vout_cntl(battery, battery->pdata->tx_uno_vout);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);

			value.intval = true;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_SEND_FSK, value);
		}
		battery->tx_switch_mode_change = false;
	}
}

void sec_bat_wireless_proc_ping_duty(struct sec_battery_info *battery, int wr_sts)
{
	if (wr_sts != SEC_BATTERY_CABLE_NONE)
		sec_bat_wireless_set_ping_duty(battery, battery->pdata->tx_ping_duty_default);
	else
		sec_bat_wireless_set_ping_duty(battery, battery->pdata->tx_ping_duty_no_ta);
}

void sec_bat_wpc_tx_work_content(struct sec_battery_info *battery)
{
	int wr_sts = battery->wire_status;
	unsigned int tx_uno_vout = battery->pdata->tx_uno_vout;

	if (battery->wc_rx_type == SS_GEAR)
		tx_uno_vout = battery->pdata->tx_gear_vout;
	else if (battery->wc_rx_type == SS_BUDS)
		tx_uno_vout = battery->pdata->tx_buds_vout;

	dev_info(battery->dev, "@Tx_Mode %s: Start: %d\n", __func__, battery->wc_rx_type);
	if (!battery->wc_tx_enable) {
		pr_info("@Tx_Mode %s : exit wpc_tx_work. Because Tx is already off\n", __func__);
		goto end_of_tx_work;
	}
	if (battery->pdata->tx_5v_disable && is_5v_charger(battery, wr_sts)) {
		pr_info("@Tx_Mode %s : 5V charger(%d) connected, disable TX\n", __func__, battery->cable_type);
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_5V_TA, BATT_TX_EVENT_WIRELESS_TX_5V_TA);
		sec_wireless_set_tx_enable(battery, false);
		goto end_of_tx_work;
	}

	if (battery->uno_en) {
		sec_bat_wireless_proc_ping_duty(battery, wr_sts);
	}

	switch (battery->wc_rx_type) {
	case NO_DEV:
		sec_bat_tx_work_nodev(battery);
		sec_bat_wireless_proc_ping_duty(battery, wr_sts);
		sb_tx_init_aov();
		break;
	case SS_GEAR:
	{
		if (battery->pdata->icl_by_tx_gear)
			sec_vote(battery->input_vote, VOTER_WC_TX, true,
				battery->pdata->icl_by_tx_gear);
		if (battery->pdata->fcc_by_tx_gear)
			sec_vote(battery->fcc_vote, VOTER_WC_TX, true,
				battery->pdata->fcc_by_tx_gear);

		if (sb_tx_is_aov_enabled(battery->wire_status)) {
			sb_tx_monitor_aov(battery->wc_tx_vout, battery->wc_tx_phm_mode);
			battery->buck_cntl_by_tx = false;

			battery->tx_switch_mode = TX_SWITCH_GEAR_PPS;
			battery->tx_switch_mode_change = true;

			sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout_aov_gear,
				battery->pdata->tx_mfc_iout_aov_gear);

			if (delayed_work_pending(&battery->wpc_tx_work))
				return;
			break;
		} else {
			if (battery->pdata->phm_vout_ctrl_dev & SEC_WIRELESS_PHM_VOUT_CTRL_GEAR) {
				if (battery->wc_tx_phm_mode) {
					if (is_hv_wire_type(battery->wire_status) ||
					(is_pd_wire_type(battery->wire_status) && battery->hv_pdo)) {
						pr_info("@Tx_Mode %s : change iv(9V -> 5V).\n", __func__);
						sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5000MV);
						sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, true);
						break;
					}
				} else {
					if ((battery->wire_status == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) ||
					(is_pd_wire_type(battery->wire_status) && !battery->hv_pdo)) {
						pr_info("@Tx_Mode %s : charging voltage change(5V -> 9V)\n", __func__);
						/* prevent ocp */
						if (!battery->buck_cntl_by_tx) {
							sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5000MV);
							sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout_gear, 1000);
							battery->buck_cntl_by_tx = true;
							sec_vote(battery->chgen_vote, VOTER_WC_TX,
								true, SEC_BAT_CHG_MODE_BUCK_OFF);
							sec_bat_run_wpc_tx_work(battery, 500);
							return;
						} else if (battery->wc_tx_vout < WC_TX_VOUT_8500MV) {
							sec_bat_wireless_vout_cntl(battery,
								battery->wc_tx_vout + WC_TX_VOUT_STEP_AOV);
							sec_bat_run_wpc_tx_work(battery, 500);
							return;
						}
						sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_9V, true);
						break;
					}
				}
			} else {
				if (is_hv_wire_type(battery->wire_status) ||
					(is_pd_wire_type(battery->wire_status) && battery->hv_pdo)) {
					pr_info("@Tx_Mode %s : change iv(9V -> 5V).\n", __func__);
					sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5000MV);
					sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, true);
					break;
				}
			}
		}

		if (is_wired_type(battery->wire_status) && battery->buck_cntl_by_tx) {
			battery->buck_cntl_by_tx = false;
			sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
		} else if ((battery->wire_status == SEC_BATTERY_CABLE_NONE) && (!battery->buck_cntl_by_tx)) {
			battery->buck_cntl_by_tx = true;
			sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		}

		sec_bat_wireless_vout_cntl(battery, tx_uno_vout);
		sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout_gear,
			battery->pdata->tx_mfc_iout_gear);
	}
		break;
	default: /* SS_BUDS, SS_PHONE, OTHER_DEV */
		if (battery->pdata->phm_vout_ctrl_dev & SEC_WIRELESS_PHM_VOUT_CTRL_BUDS) {
			if (battery->wc_tx_phm_mode) {
				pr_info("@Tx_Mode %s : phm on status.\n", __func__);

				sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5000MV);
				if (is_hv_wire_type(battery->wire_status) ||
					(is_pd_wire_type(battery->wire_status) && battery->hv_pdo)) {
					pr_info("@Tx_Mode %s : change iv(9V -> 5V).\n", __func__);
					sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, true);
				} else {
					pr_info("@Tx_Mode %s : change iv(%dV -> 5V).\n",
						__func__, battery->pdata->tx_uno_vout);
				}
				battery->prev_tx_phm_mode = battery->wc_tx_phm_mode;
				break;
			} else {
				if (battery->prev_tx_phm_mode) {
					pr_info("@Tx_Mode %s : keep phm off status concept before tx off\n", __func__);
					break;
				}
			}
		}

		if ((battery->wire_status == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) ||
			(is_pd_wire_type(battery->wire_status) && !battery->hv_pdo)) {
			if (battery->wc_tx_vout < tx_uno_vout) {
				sec_bat_wireless_vout_cntl(battery, tx_uno_vout);
				if (!battery->buck_cntl_by_tx) {
					battery->buck_cntl_by_tx = true;
					sec_vote(battery->chgen_vote, VOTER_WC_TX,
						true, SEC_BAT_CHG_MODE_BUCK_OFF);
				}
				sec_bat_run_wpc_tx_work(battery, 500);
				return;
			}
			pr_info("@Tx_Mode %s : change iv(5V -> 9V)\n", __func__);
			sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_9V, true);
			break;
		}

		if (battery->wire_status == SEC_BATTERY_CABLE_NONE) {
			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_start_soc = 0;
			battery->tx_switch_mode_change = false;

			if (!battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = true;
				sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			}
			sec_bat_wireless_vout_cntl(battery, tx_uno_vout);
			sec_bat_wireless_iout_cntl(battery,
					battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		} else if (is_hv_wire_type(battery->wire_status) ||
				(is_pd_wire_type(battery->wire_status) && battery->hv_pdo)) {
			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_start_soc = 0;
			battery->tx_switch_mode_change = false;

			if (battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = false;
				sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
			}
			sec_bat_wireless_iout_cntl(battery,
					battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		} else if (is_pd_wire_type(battery->wire_status) && battery->hv_pdo) {
			pr_info("@Tx_Mode %s: PD cable attached. HV PDO(%d)\n", __func__, battery->hv_pdo);

			battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
			battery->tx_switch_start_soc = 0;
			battery->tx_switch_mode_change = false;

			if (battery->buck_cntl_by_tx) {
				battery->buck_cntl_by_tx = false;
				sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
			}
			sec_bat_wireless_iout_cntl(battery,
					battery->pdata->tx_uno_iout, battery->pdata->tx_mfc_iout_phone);
			sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		} else if (is_wired_type(battery->wire_status) && !is_hv_wire_type(battery->wire_status) &&
				(battery->wire_status != SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)) {
			sec_bat_tx_work_with_nv_charger(battery);
		}
		break;
	}
end_of_tx_work:
	dev_info(battery->dev, "@Tx_Mode %s End\n", __func__);
	__pm_relax(battery->wpc_tx_ws);
}

#define EN_RETRY_DELAY 100
#define EN_RETRY_CNT 20
void sec_bat_wpc_tx_en_work_content(struct sec_battery_info *battery)
{
	int wr_sts = battery->wire_status;
	int selected_pdo;
	static unsigned int cnt;

	pr_info("@Tx_Mode %s: tx %s\n", __func__,
		battery->wc_tx_enable ? "on" : "off");

	battery->tx_minduty = battery->pdata->tx_minduty_default;
	battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
	battery->tx_switch_start_soc = 0;
	battery->tx_switch_mode_change = false;
	battery->tx_ping_duty = 0;

	if (battery->wc_tx_enable) {
		/* set tx event */
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_STATUS,
			(BATT_TX_EVENT_WIRELESS_TX_STATUS | BATT_TX_EVENT_WIRELESS_TX_RETRY));

		if (is_pd_wire_type(wr_sts)) {
			sec_pd_get_selected_pdo(&selected_pdo);
			if (selected_pdo == battery->sink_status.current_pdo_num) {
				if (battery->sink_status.power_list[battery->sink_status.current_pdo_num].max_voltage
					== SEC_INPUT_VOLTAGE_5V) {
					pr_info("@Tx_Mode %s: 5V PDO. Tx Start\n", __func__);
					sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, false);
					sec_bat_run_wpc_tx_work(battery, 0);
				} else {
					pr_info("@Tx_Mode %s: Wait to decrease 5V (%d / %d)\n",
						__func__, ++cnt, EN_RETRY_CNT);
					sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, false);
					__pm_stay_awake(battery->wpc_tx_en_ws);
					queue_delayed_work(battery->monitor_wqueue,
						&battery->wpc_tx_en_work, msecs_to_jiffies(EN_RETRY_DELAY));
					return;
				}
			} else {
				if (cnt >= EN_RETRY_CNT) {
					pr_info("@Tx_Mode %s: No response for 5V. Error case\n", __func__);
					cnt = 0;
				} else {
					pr_info("@Tx_Mode %s: Wait to decrease 5V (%d / %d)\n",
						__func__, ++cnt, EN_RETRY_CNT);
					__pm_stay_awake(battery->wpc_tx_en_ws);
					queue_delayed_work(battery->monitor_wqueue,
						&battery->wpc_tx_en_work, msecs_to_jiffies(EN_RETRY_DELAY));
					return;
				}
			}
		} else if (is_hv_wire_type(wr_sts)) {
			if (cnt >= EN_RETRY_CNT) {
				pr_info("@Tx_Mode %s: No response for 5V. Error case\n", __func__);
				cnt = 0;
			} else {
				pr_info("@Tx_Mode %s: Wait to decrease 5V (%d / %d)\n",
					__func__, ++cnt, EN_RETRY_CNT);
				sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, false);
				__pm_stay_awake(battery->wpc_tx_en_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->wpc_tx_en_work, msecs_to_jiffies(EN_RETRY_DELAY));
				return;
			}
		} else {
			pr_info("@Tx_Mode %s: 5V AFC. Tx Start\n", __func__);
			sec_bat_wpc_tx_iv(battery->iv_vote, SEC_INPUT_VOLTAGE_5V, false);
			sec_bat_run_wpc_tx_work(battery, 0);
		}

		pr_info("@Tx_Mode %s: TX Power Calculation start.\n", __func__);
		queue_delayed_work(battery->monitor_wqueue,
				&battery->wpc_txpower_calc_work, 0);
	} else {
		cancel_delayed_work(&battery->wpc_tx_work);
		cancel_delayed_work(&battery->wpc_txpower_calc_work);
		__pm_relax(battery->wpc_tx_ws);

		sec_bat_wireless_minduty_cntl(battery, battery->pdata->tx_minduty_default);
		battery->wc_rx_type = NO_DEV;
		battery->wc_rx_connected = false;
		battery->tx_uno_iout = 0;
		battery->tx_mfc_iout = 0;
		battery->buck_cntl_by_tx = false;
		sec_bat_wireless_uno_cntl(battery, false);
		sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5000MV);
		sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
		sec_vote(battery->fcc_vote, VOTER_WC_TX, false, 0);
		sec_vote(battery->input_vote, VOTER_WC_TX, false, 0);
		/* for 1) not supporting DC and charging bia PD20/DC on Tx */
		/*     2) supporting DC and charging bia PD20 on Tx */
		/*	   3) During retrying, maintain iv_vote */
		if (battery->tx_retry_case == SEC_BAT_TX_RETRY_NONE)
			sec_vote(battery->iv_vote, VOTER_WC_TX, false, 0);
		cnt = 0;
	}

	pr_info("@Tx_Mode %s Done\n", __func__);
	__pm_relax(battery->wpc_tx_en_ws);
}

void sec_wireless_set_tx_enable(struct sec_battery_info *battery, bool wc_tx_enable)
{
	int wr_sts = battery->wire_status;
	pr_info("@Tx_Mode %s: TX Power enable ? (%d)\n", __func__, wc_tx_enable);

	if (battery->pdata->tx_5v_disable && wc_tx_enable && is_5v_charger(battery, wr_sts)) {
		pr_info("@Tx_Mode %s : 5V charger(%d) connected, do not turn on TX\n", __func__, wr_sts);
		sec_bat_set_tx_event(battery, BATT_TX_EVENT_WIRELESS_TX_5V_TA, BATT_TX_EVENT_WIRELESS_TX_5V_TA);
		return;
	}

	/* temporary code */
	sb_tx_init(battery, battery->pdata->wireless_charger_name);
	sb_tx_init_aov();

	battery->wc_tx_enable = wc_tx_enable;
	battery->wc_tx_phm_mode = false;
	battery->prev_tx_phm_mode = false;

	/* FPDO_DC & APDO concept */
	if (is_pd_apdo_wire_type(wr_sts) && battery->wc_tx_enable) {
		union power_supply_propval value = {0, };

		value.intval = 0;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);

	}

	cancel_delayed_work(&battery->wpc_tx_en_work);
	__pm_stay_awake(battery->wpc_tx_en_ws);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->wpc_tx_en_work, 0);
}
EXPORT_SYMBOL(sec_wireless_set_tx_enable);
