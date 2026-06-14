/*
 *  sec_battery_sysfs.c
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
#include "sec_battery_sysfs.h"
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

static struct device_attribute sec_battery_attrs[] = {
	SEC_BATTERY_ATTR(batt_reset_soc),
	SEC_BATTERY_ATTR(batt_read_raw_soc),
	SEC_BATTERY_ATTR(batt_read_adj_soc),
	SEC_BATTERY_ATTR(batt_type),
	SEC_BATTERY_ATTR(batt_vfocv),
	SEC_BATTERY_ATTR(batt_vol_adc),
	SEC_BATTERY_ATTR(batt_vol_adc_cal),
	SEC_BATTERY_ATTR(batt_vol_aver),
	SEC_BATTERY_ATTR(batt_vol_adc_aver),
	SEC_BATTERY_ATTR(batt_voltage_now),
	SEC_BATTERY_ATTR(batt_current_ua_now),
	SEC_BATTERY_ATTR(batt_current_ua_avg),
	SEC_BATTERY_ATTR(batt_filter_cfg),
	SEC_BATTERY_ATTR(batt_temp),
	SEC_BATTERY_ATTR(batt_temp_raw),
	SEC_BATTERY_ATTR(batt_temp_adc),
	SEC_BATTERY_ATTR(batt_temp_aver),
	SEC_BATTERY_ATTR(batt_temp_adc_aver),
	SEC_BATTERY_ATTR(usb_temp),
	SEC_BATTERY_ATTR(usb_temp_adc),
	SEC_BATTERY_ATTR(chg_temp),
	SEC_BATTERY_ATTR(chg_temp_adc),
	SEC_BATTERY_ATTR(sub_bat_temp),
	SEC_BATTERY_ATTR(sub_bat_temp_adc),
	SEC_BATTERY_ATTR(sub_chg_temp),
	SEC_BATTERY_ATTR(sub_chg_temp_adc),
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	SEC_BATTERY_ATTR(dchg_adc_mode_ctrl),
	SEC_BATTERY_ATTR(dchg_temp),
	SEC_BATTERY_ATTR(dchg_temp_adc),
	SEC_BATTERY_ATTR(dchg_read_batp_batn),
#endif
	SEC_BATTERY_ATTR(blkt_temp),
	SEC_BATTERY_ATTR(blkt_temp_adc),
	SEC_BATTERY_ATTR(batt_vf_adc),
	SEC_BATTERY_ATTR(batt_slate_mode),

	SEC_BATTERY_ATTR(batt_lp_charging),
	SEC_BATTERY_ATTR(siop_activated),
	SEC_BATTERY_ATTR(siop_level),
	SEC_BATTERY_ATTR(siop_event),
	SEC_BATTERY_ATTR(batt_charging_source),
	SEC_BATTERY_ATTR(fg_reg_dump),
	SEC_BATTERY_ATTR(fg_reset_cap),
	SEC_BATTERY_ATTR(fg_capacity),
	SEC_BATTERY_ATTR(fg_asoc),
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	SEC_BATTERY_ATTR(fg_main_asoc),
	SEC_BATTERY_ATTR(fg_sub_asoc),
#endif
	SEC_BATTERY_ATTR(auth),
	SEC_BATTERY_ATTR(chg_current_adc),
	SEC_BATTERY_ATTR(wc_adc),
	SEC_BATTERY_ATTR(wc_status),
	SEC_BATTERY_ATTR(wc_enable),
	SEC_BATTERY_ATTR(wc_control),
	SEC_BATTERY_ATTR(wc_control_cnt),
	SEC_BATTERY_ATTR(led_cover),
	SEC_BATTERY_ATTR(hv_charger_status),
	SEC_BATTERY_ATTR(hv_wc_charger_status),
	SEC_BATTERY_ATTR(hv_charger_set),
	SEC_BATTERY_ATTR(factory_mode),
	SEC_BATTERY_ATTR(store_mode),
	SEC_BATTERY_ATTR(update),
	SEC_BATTERY_ATTR(test_mode),

	SEC_BATTERY_ATTR(call),
	SEC_BATTERY_ATTR(2g_call),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(3g_call),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(music),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(browser),
	SEC_BATTERY_ATTR(hotspot),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(camcorder),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(wifi),
	SEC_BATTERY_ATTR(wibro),
	SEC_BATTERY_ATTR(lte),
	SEC_BATTERY_ATTR(lcd),
#if defined(CONFIG_ISDB_CHARGING_CONTROL)
	SEC_BATTERY_ATTR(batt_event_isdb),
#endif
	SEC_BATTERY_ATTR(gps),
	SEC_BATTERY_ATTR(event),
	SEC_BATTERY_ATTR(batt_temp_table),
	SEC_BATTERY_ATTR(batt_high_current_usb),
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	SEC_BATTERY_ATTR(test_charge_current),
#if defined(CONFIG_STEP_CHARGING)
	SEC_BATTERY_ATTR(test_step_condition),
#endif
#endif
	SEC_BATTERY_ATTR(set_stability_test),
	SEC_BATTERY_ATTR(batt_capacity_max),
	SEC_BATTERY_ATTR(batt_repcap_1st),
	SEC_BATTERY_ATTR(batt_inbat_voltage),
	SEC_BATTERY_ATTR(batt_inbat_voltage_ocv),
	SEC_BATTERY_ATTR(batt_inbat_voltage_adc),
	SEC_BATTERY_ATTR(vbyp_voltage),
	SEC_BATTERY_ATTR(check_sub_chg),
	SEC_BATTERY_ATTR(batt_inbat_wireless_cs100),
	SEC_BATTERY_ATTR(hmt_ta_connected),
	SEC_BATTERY_ATTR(hmt_ta_charge),
#if defined(CONFIG_SEC_FACTORY)
	SEC_BATTERY_ATTR(afc_test_fg_mode),
	SEC_BATTERY_ATTR(nozx_ctrl),
#endif
	SEC_BATTERY_ATTR(fg_cycle), /* this value is from fuelgauge, 100% has 1 value */
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	SEC_BATTERY_ATTR(fg_sub_cycle), /* this value is from fuelgauge, 100% has 1 value */
#endif
	SEC_BATTERY_ATTR(fg_full_voltage),
	SEC_BATTERY_ATTR(fg_fullcapnom),
	SEC_BATTERY_ATTR(battery_cycle), /* this value is calculated by PMS and saved in efs, 100% has 1 value */
#if defined(CONFIG_BATTERY_AGE_FORECAST_DETACHABLE)
	SEC_BATTERY_ATTR(batt_after_manufactured),
#endif
	SEC_BATTERY_ATTR(battery_cycle_test),
	SEC_BATTERY_ATTR(batt_wpc_temp),
	SEC_BATTERY_ATTR(batt_wpc_temp_adc),
	SEC_BATTERY_ATTR(mst_switch_test), /* MFC MST switch test */
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	SEC_BATTERY_ATTR(batt_wireless_firmware_update),
	SEC_BATTERY_ATTR(otp_firmware_result),
	SEC_BATTERY_ATTR(wc_ic_grade),
	SEC_BATTERY_ATTR(wc_ic_chip_id),
	SEC_BATTERY_ATTR(otp_firmware_ver_bin),
	SEC_BATTERY_ATTR(otp_firmware_ver),
#endif
	SEC_BATTERY_ATTR(wc_phm_ctrl),
	SEC_BATTERY_ATTR(wc_vout),
	SEC_BATTERY_ATTR(wc_vrect),
	SEC_BATTERY_ATTR(wc_tx_en),
	SEC_BATTERY_ATTR(wc_tx_vout),
	SEC_BATTERY_ATTR(batt_hv_wireless_status),
	SEC_BATTERY_ATTR(batt_hv_wireless_pad_ctrl),
	SEC_BATTERY_ATTR(wc_tx_id),
	SEC_BATTERY_ATTR(wc_op_freq),
	SEC_BATTERY_ATTR(wc_cmd_info),
	SEC_BATTERY_ATTR(wc_rx_connected),
	SEC_BATTERY_ATTR(wc_rx_connected_dev),
	SEC_BATTERY_ATTR(wc_tx_mfc_vin_from_uno),
	SEC_BATTERY_ATTR(wc_tx_mfc_iin_from_uno),
#if defined(CONFIG_WIRELESS_TX_MODE)
	SEC_BATTERY_ATTR(wc_tx_avg_curr),
	SEC_BATTERY_ATTR(wc_tx_total_pwr),
#endif
	SEC_BATTERY_ATTR(wc_tx_stop_capacity),
	SEC_BATTERY_ATTR(wc_tx_timer_en),
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	SEC_BATTERY_ATTR(batt_tune_float_voltage),
	SEC_BATTERY_ATTR(batt_tune_input_charge_current),
	SEC_BATTERY_ATTR(batt_tune_fast_charge_current),
	SEC_BATTERY_ATTR(batt_tune_wireless_vout_current),
	SEC_BATTERY_ATTR(batt_tune_ui_term_cur_1st),
	SEC_BATTERY_ATTR(batt_tune_ui_term_cur_2nd),
	SEC_BATTERY_ATTR(batt_tune_temp_high_normal),
	SEC_BATTERY_ATTR(batt_tune_temp_high_rec_normal),
	SEC_BATTERY_ATTR(batt_tune_temp_low_normal),
	SEC_BATTERY_ATTR(batt_tune_temp_low_rec_normal),
	SEC_BATTERY_ATTR(batt_tune_chg_temp_high),
	SEC_BATTERY_ATTR(batt_tune_chg_temp_rec),
	SEC_BATTERY_ATTR(batt_tune_chg_limit_cur),
	SEC_BATTERY_ATTR(batt_tune_lrp_temp_high_lcdon),
	SEC_BATTERY_ATTR(batt_tune_lrp_temp_high_lcdoff),
	SEC_BATTERY_ATTR(batt_tune_coil_temp_high),
	SEC_BATTERY_ATTR(batt_tune_coil_temp_rec),
	SEC_BATTERY_ATTR(batt_tune_coil_limit_cur),
	SEC_BATTERY_ATTR(batt_tune_wpc_temp_high),
	SEC_BATTERY_ATTR(batt_tune_wpc_temp_high_rec),
	SEC_BATTERY_ATTR(batt_tune_dchg_temp_high),
	SEC_BATTERY_ATTR(batt_tune_dchg_temp_high_rec),
	SEC_BATTERY_ATTR(batt_tune_dchg_batt_temp_high),
	SEC_BATTERY_ATTR(batt_tune_dchg_batt_temp_high_rec),
	SEC_BATTERY_ATTR(batt_tune_dchg_limit_input_cur),
	SEC_BATTERY_ATTR(batt_tune_dchg_limit_chg_cur),
#if defined(CONFIG_WIRELESS_TX_MODE)
	SEC_BATTERY_ATTR(batt_tune_tx_mfc_iout_gear),
	SEC_BATTERY_ATTR(batt_tune_tx_mfc_iout_phone),
#endif
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	SEC_BATTERY_ATTR(batt_update_data),
#endif
	SEC_BATTERY_ATTR(batt_misc_event),
	SEC_BATTERY_ATTR(batt_tx_event),
	SEC_BATTERY_ATTR(batt_ext_dev_chg),
	SEC_BATTERY_ATTR(batt_wdt_control),
	SEC_BATTERY_ATTR(mode),
	SEC_BATTERY_ATTR(check_ps_ready),
	SEC_BATTERY_ATTR(batt_chip_id),
	SEC_BATTERY_ATTR(error_cause),
	SEC_BATTERY_ATTR(cisd_fullcaprep_max),
	SEC_BATTERY_ATTR(cisd_data),
	SEC_BATTERY_ATTR(cisd_data_json),
	SEC_BATTERY_ATTR(cisd_data_d_json),
	SEC_BATTERY_ATTR(cisd_wire_count),
	SEC_BATTERY_ATTR(cisd_wc_data),
	SEC_BATTERY_ATTR(cisd_wc_data_json),
	SEC_BATTERY_ATTR(cisd_power_data),
	SEC_BATTERY_ATTR(cisd_power_data_json),
	SEC_BATTERY_ATTR(cisd_pd_data),
	SEC_BATTERY_ATTR(cisd_pd_data_json),
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	SEC_BATTERY_ATTR(cisd_dcerr_data),
	SEC_BATTERY_ATTR(cisd_dcerr_data_json),
#endif
	SEC_BATTERY_ATTR(cisd_cable_data),
	SEC_BATTERY_ATTR(cisd_cable_data_json),
	SEC_BATTERY_ATTR(cisd_tx_data),
	SEC_BATTERY_ATTR(cisd_tx_data_json),
	SEC_BATTERY_ATTR(cisd_event_data),
	SEC_BATTERY_ATTR(cisd_event_data_json),
	SEC_BATTERY_ATTR(prev_battery_data),
	SEC_BATTERY_ATTR(prev_battery_info),
	SEC_BATTERY_ATTR(safety_timer_set),
	SEC_BATTERY_ATTR(batt_swelling_control),
	SEC_BATTERY_ATTR(batt_battery_id),
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	SEC_BATTERY_ATTR(batt_sub_battery_id),
#endif
	SEC_BATTERY_ATTR(batt_temp_control_test),
	SEC_BATTERY_ATTR(safety_timer_info),
	SEC_BATTERY_ATTR(batt_shipmode_test),
	SEC_BATTERY_ATTR(batt_misc_test),
	SEC_BATTERY_ATTR(batt_temp_test),
	SEC_BATTERY_ATTR(batt_volt_test),
	SEC_BATTERY_ATTR(batt_current_event),
	SEC_BATTERY_ATTR(batt_jig_gpio),
	SEC_BATTERY_ATTR(cc_info),
#if defined(CONFIG_WIRELESS_AUTH)
	SEC_BATTERY_ATTR(wc_auth_adt_sent),
#endif
	SEC_BATTERY_ATTR(wc_duo_rx_power),
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	SEC_BATTERY_ATTR(batt_main_voltage),
	SEC_BATTERY_ATTR(batt_sub_voltage),
	SEC_BATTERY_ATTR(batt_main_vcell),
	SEC_BATTERY_ATTR(batt_sub_vcell),
	SEC_BATTERY_ATTR(batt_main_current_ma),
	SEC_BATTERY_ATTR(batt_sub_current_ma),
	SEC_BATTERY_ATTR(batt_main_con_det),
	SEC_BATTERY_ATTR(batt_sub_con_det),
	SEC_BATTERY_ATTR(batt_main_vchg),
	SEC_BATTERY_ATTR(batt_sub_vchg),
	SEC_BATTERY_ATTR(batt_main_chg_voltage),
	SEC_BATTERY_ATTR(batt_sub_chg_voltage),
	SEC_BATTERY_ATTR(batt_main_rechg_voltage),
	SEC_BATTERY_ATTR(batt_sub_rechg_voltage),
#if IS_ENABLED(CONFIG_LIMITER_S2ASL01)
	SEC_BATTERY_ATTR(batt_main_enb),
	SEC_BATTERY_ATTR(batt_main_enb2),
	SEC_BATTERY_ATTR(batt_sub_enb),
	SEC_BATTERY_ATTR(batt_sub_pwr_mode2),
#else
	SEC_BATTERY_ATTR(batt_main_shipmode),
	SEC_BATTERY_ATTR(batt_sub_shipmode),
	SEC_BATTERY_ATTR(batt_main_vbat),
	SEC_BATTERY_ATTR(batt_sub_vbat),
#endif
#if (IS_ENABLED(CONFIG_DUAL_FUELGAUGE) || IS_ENABLED(CONFIG_DUAL_SBP))
	SEC_BATTERY_ATTR(batt_main_soc),
	SEC_BATTERY_ATTR(batt_sub_soc),
#endif
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	SEC_BATTERY_ATTR(batt_main_raw_soc),
	SEC_BATTERY_ATTR(batt_sub_raw_soc),
	SEC_BATTERY_ATTR(batt_main_repcap),
	SEC_BATTERY_ATTR(batt_sub_repcap),
	SEC_BATTERY_ATTR(batt_main_fullcaprep),
	SEC_BATTERY_ATTR(batt_sub_fullcaprep),
#endif
#endif
	SEC_BATTERY_ATTR(ext_event),
	SEC_BATTERY_ATTR(direct_charging_status),
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	SEC_BATTERY_ATTR(direct_charging_step),
	SEC_BATTERY_ATTR(direct_charging_iin),
	SEC_BATTERY_ATTR(direct_charging_chg_status),
	SEC_BATTERY_ATTR(direct_charging_ratio),
	SEC_BATTERY_ATTR(switch_charging_source),
	SEC_BATTERY_ATTR(dc_error_cause),
#endif
	SEC_BATTERY_ATTR(charging_type),
	SEC_BATTERY_ATTR(batt_factory_mode),
	SEC_BATTERY_ATTR(boot_completed),
	SEC_BATTERY_ATTR(pd_disable),
	SEC_BATTERY_ATTR(factory_mode_relieve),
	SEC_BATTERY_ATTR(factory_mode_bypass),
	SEC_BATTERY_ATTR(normal_mode_bypass),
	SEC_BATTERY_ATTR(factory_voltage_regulation),
	SEC_BATTERY_ATTR(factory_mode_disable),
	SEC_BATTERY_ATTR(usb_conf_test),
	SEC_BATTERY_ATTR(charge_otg_control),
	SEC_BATTERY_ATTR(charge_uno_control),
	SEC_BATTERY_ATTR(charge_counter_shadow),
	SEC_BATTERY_ATTR(voter_status),
#if defined(CONFIG_WIRELESS_IC_PARAM)
	SEC_BATTERY_ATTR(wc_param_info),
#endif
	SEC_BATTERY_ATTR(chg_info),
	SEC_BATTERY_ATTR(lrp),
	SEC_BATTERY_ATTR(hp_d2d),
	SEC_BATTERY_ATTR(charger_ic_name),
	SEC_BATTERY_ATTR(dc_rb_en),
	SEC_BATTERY_ATTR(dc_op_mode),
	SEC_BATTERY_ATTR(dc_adc_mode),
	SEC_BATTERY_ATTR(dc_vbus),
	SEC_BATTERY_ATTR(chg_type),
	SEC_BATTERY_ATTR(mst_en),
	SEC_BATTERY_ATTR(spsn_test),
	SEC_BATTERY_ATTR(chg_soc_lim),
	SEC_BATTERY_ATTR(mag_cover),
	SEC_BATTERY_ATTR(mag_cloak),
	SEC_BATTERY_ATTR(mag_value),
	SEC_BATTERY_ATTR(ari_cnt),
#if IS_ENABLED(CONFIG_SBP_FG)
	SEC_BATTERY_ATTR(state_of_health),
#endif
#if IS_ENABLED(CONFIG_BATTERY_AUTH_SLE956681)
	SEC_BATTERY_ATTR(vk_key_status),   /* For infineon IC , status = 1 => key is verified in VK by sbauthd */
#endif
	SEC_BATTERY_ATTR(adc_rsense), /* for tuning adc_rsense of bat_thm only now */
	SEC_BATTERY_ATTR(support_functions),
#if IS_ENABLED(CONFIG_BATTERY_AUTH_EUR_DET_GPIO_SUPPORT)
	SEC_BATTERY_ATTR(batt_auth_support),
#endif
	SEC_BATTERY_ATTR(ocpwarn),
};

static struct device_attribute sec_pogo_attrs[] = {
	SEC_POGO_ATTR(sec_type),
};

static struct device_attribute sec_otg_attrs[] = {
	SEC_OTG_ATTR(sec_type),
};

ssize_t sec_bat_show_attrs(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sec_battery_attrs;
	union power_supply_propval value = {0, };
	int i = 0;
	int ret = 0;

	switch (offset) {
	case BATT_RESET_SOC:
		break;
	case BATT_READ_RAW_SOC:
		{
			value.intval =
				SEC_FUELGAUGE_CAPACITY_TYPE_RAW;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CAPACITY, value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_READ_ADJ_SOC:
		break;
	case BATT_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			battery->batt_type);
		break;
	case BATT_VFOCV:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->voltage_ocv);
		break;
	case BATT_VOL_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->inbat_adc);
		break;
	case BATT_VOL_ADC_CAL:
		break;
	case BATT_VOL_AVER:
		break;
	case BATT_VOL_ADC_AVER:
		break;
	case BATT_VOLTAGE_NOW:
		{
			value.intval = SEC_BATTERY_VOLTAGE_MV;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			        value.intval * 1000);
		}
		break;
	case BATT_CURRENT_UA_NOW:
		{
			value.intval = SEC_BATTERY_CURRENT_UA;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
#if defined(CONFIG_SEC_FACTORY)
			pr_err("%s: batt_current_ua_now (%d)\n",
					__func__, value.intval);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_CURRENT_UA_AVG:
		{
			value.intval = SEC_BATTERY_CURRENT_UA;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_CURRENT_AVG, value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_FILTER_CFG:
		{
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_FILTER_CFG, value);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
				value.intval);
		}
		break;
	case BATT_TEMP:
		value.intval = sec_bat_get_temperature(battery->dev,
				&battery->pdata->bat_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
#if !defined(CONFIG_SEC_FACTORY)
		if (battery->pdata->lr_enable) {
			int sub_bat_temp = sec_bat_get_temperature(battery->dev,
					&battery->pdata->sub_bat_thm_info, 0,
					battery->pdata->charger_name, battery->pdata->fuelgauge_name,
					battery->pdata->adc_read_type);
			value.intval = lr_predict_bat_temp(battery, value.intval, sub_bat_temp);
		}
#endif
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case BATT_TEMP_RAW:
		value.intval = sec_bat_get_temperature(battery->dev,
				&battery->pdata->bat_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case BATT_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->pdata->bat_thm_info.adc);
		break;
	case BATT_TEMP_AVER:
		break;
	case BATT_TEMP_ADC_AVER:
		break;
	case USB_TEMP:
		value.intval = sec_bat_get_temperature(battery->dev, &battery->pdata->usb_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case USB_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				       battery->pdata->usb_thm_info.adc);
		break;
	case BATT_CHG_TEMP:
		value.intval = sec_bat_get_temperature(battery->dev,
				&battery->pdata->chg_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case BATT_CHG_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->pdata->chg_thm_info.adc);
		break;
	case SUB_BAT_TEMP:
		value.intval = sec_bat_get_temperature(battery->dev, &battery->pdata->sub_bat_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
#if !defined(CONFIG_SEC_FACTORY)
		if (battery->pdata->sub_temp_control_source == TEMP_CONTROL_SOURCE_WPC_THM)
			sec_bat_calc_unknown_wpc_temp(battery, &(value.intval), battery->wpc_temp, battery->usb_temp);
#endif
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case SUB_BAT_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->pdata->sub_bat_thm_info.adc);
		break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case DCHG_ADC_MODE_CTRL:
		break;
	case DCHG_TEMP:
		{
			if (battery->pdata->dctp_by_cgtp)
				battery->dchg_temp = battery->chg_temp;
			else
				battery->dchg_temp = sec_bat_get_temperature(battery->dev,
						&battery->pdata->dchg_thm_info, 0,
						battery->pdata->charger_name, battery->pdata->fuelgauge_name,
						battery->pdata->adc_read_type);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->dchg_temp);
		}
		break;
	case DCHG_TEMP_ADC:
		{
			switch (battery->pdata->dchg_thm_info.source) {
				case SEC_BATTERY_THERMAL_SOURCE_CHG_ADC:
					psy_do_property(battery->pdata->charger_name, get,
						POWER_SUPPLY_PROP_TEMP, value);
					break;
				case SEC_BATTERY_THERMAL_SOURCE_ADC:
					value.intval = battery->pdata->dchg_thm_info.adc;
					break;
				default:
					value.intval = -1;
					break;
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case DCHG_READ_BATP_BATN:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_DCHG_READ_BATP_BATN, value);
		pr_info("%s: DCHG_READ_BATP_BATN(%dmV) ", __func__, value.intval / 1000);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval / 1000);
		break;
#endif
	case BLKT_TEMP:
		value.intval = sec_bat_get_temperature(battery->dev, &battery->pdata->blk_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case BLKT_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->pdata->blk_thm_info.adc);
		break;
	case BATT_VF_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->check_adc_value);
		break;
	case BATT_SLATE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			is_slate_mode(battery));
		break;

	case BATT_LP_CHARGING:
		if (sec_bat_get_lpmode())
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", sec_bat_get_lpmode());
		break;
	case SIOP_ACTIVATED:
		break;
	case SIOP_LEVEL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->siop_level);
		break;
	case SIOP_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			0);
		break;
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->cable_type);
		break;
	case FG_REG_DUMP:
		break;
	case FG_RESET_CAP:
		break;
	case FG_CAPACITY:
	{
		value.intval =
			SEC_BATTERY_CAPACITY_DESIGNED;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ",
			value.intval);

		value.intval =
			SEC_BATTERY_CAPACITY_ABSOLUTE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ",
			value.intval);

		value.intval =
			SEC_BATTERY_CAPACITY_TEMPERARY;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x ",
			value.intval);

		value.intval =
			SEC_BATTERY_CAPACITY_CURRENT;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%04x\n",
			value.intval);
	}
		break;
	case FG_ASOC:
		value.intval = -1;
		{
			struct power_supply *psy_fg = NULL;
			psy_fg = get_power_supply_by_name(battery->pdata->fuelgauge_name);
			if (!psy_fg) {
				pr_err("%s: Fail to get psy (%s)\n",
						__func__, battery->pdata->fuelgauge_name);
			} else {
				if (psy_fg->desc->get_property != NULL) {
					ret = psy_fg->desc->get_property(psy_fg,
							POWER_SUPPLY_PROP_ENERGY_FULL, &value);
					if (ret < 0) {
						pr_err("%s: Fail to %s get (%d=>%d)\n",
								__func__, battery->pdata->fuelgauge_name,
								POWER_SUPPLY_PROP_ENERGY_FULL, ret);
					}
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
					if (!value.intval)
						sec_abc_send_event("MODULE=battery@WARN=show_fg_asoc0");
#endif
				}
			}
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       value.intval);
		break;
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	case FG_MAIN_ASOC:
		value.intval = -1;
		{
			struct power_supply *psy_fg = NULL;

			psy_fg = get_power_supply_by_name(battery->pdata->fuelgauge_name);

			if (!psy_fg) {
				pr_err("%s: Fail to get psy (%s)\n",
						__func__, battery->pdata->fuelgauge_name);
			} else {
				if (psy_fg->desc->get_property != NULL) {
					ret = psy_fg->desc->get_property(psy_fg,
							(enum power_supply_property)POWER_SUPPLY_EXT_PROP_MAIN_ENERGY_FULL, &value);
					if (ret < 0) {
						pr_err("%s: Fail to %s get (%d=>%d)\n",
								__func__, battery->pdata->fuelgauge_name,
								(enum power_supply_property)POWER_SUPPLY_EXT_PROP_MAIN_ENERGY_FULL, ret);
					}
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
					if (!value.intval)
						sec_abc_send_event("MODULE=battery@WARN=show_fg_asoc0");
#endif
				}
			}
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       value.intval);
		break;
	case FG_SUB_ASOC:
		value.intval = -1;
		{
			struct power_supply *psy_fg = NULL;

			psy_fg = get_power_supply_by_name(battery->pdata->fuelgauge_name);

			if (!psy_fg) {
				pr_err("%s: Fail to get psy (%s)\n",
						__func__, battery->pdata->fuelgauge_name);
			} else {
				if (psy_fg->desc->get_property != NULL) {
					ret = psy_fg->desc->get_property(psy_fg,
							(enum power_supply_property)POWER_SUPPLY_EXT_PROP_SUB_ENERGY_FULL, &value);
					if (ret < 0) {
						pr_err("%s: Fail to %s get (%d=>%d)\n",
								__func__, battery->pdata->fuelgauge_name,
								(enum power_supply_property)POWER_SUPPLY_EXT_PROP_SUB_ENERGY_FULL, ret);
					}
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
					if (!value.intval)
						sec_abc_send_event("MODULE=battery@WARN=show_fg_asoc0");
#endif
				}
			}
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       value.intval);
		break;
#endif
	case AUTH:
		break;
	case CHG_CURRENT_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->current_adc);
		break;
	case WC_ADC:
		break;
	case WC_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			is_wireless_type(battery->cable_type) ? 1: 0);
		break;
	case WC_ENABLE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->wc_enable);
		break;
	case WC_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->wc_enable);
		break;
	case WC_CONTROL_CNT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->wc_enable_cnt_value);
		break;
	case LED_COVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->led_cover);
		break;
	case HV_CHARGER_STATUS:
		{
			int check_val = 0;

			check_val = get_chg_power_type(
					battery->cable_type, battery->wire_status,
					battery->pd_max_charge_power,
					battery->max_charge_power);
			pr_info("%s : HV_CHARGER_STATUS(%d), max_charger_power(%d), pd_max charge power(%d)\n",
					__func__, check_val, battery->max_charge_power, battery->pd_max_charge_power);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", check_val);
		}
		break;
	case HV_WC_CHARGER_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			is_hv_wireless_type(battery->cable_type) ? 1 : 0);
		break;
	case HV_CHARGER_SET:
		break;
	case FACTORY_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->factory_mode);
		break;
	case STORE_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->store_mode);
		break;
	case UPDATE:
		break;
	case TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->test_mode);
		break;
	case BATT_EVENT_CALL:
		break;
	case BATT_EVENT_2G_CALL:
		break;
	case BATT_EVENT_TALK_GSM:
		break;
	case BATT_EVENT_3G_CALL:
		break;
	case BATT_EVENT_TALK_WCDMA:
		break;
	case BATT_EVENT_MUSIC:
		break;
	case BATT_EVENT_VIDEO:
		break;
	case BATT_EVENT_BROWSER:
		break;
	case BATT_EVENT_HOTSPOT:
		break;
	case BATT_EVENT_CAMERA:
		break;
	case BATT_EVENT_CAMCORDER:
		break;
	case BATT_EVENT_DATA_CALL:
		break;
	case BATT_EVENT_WIFI:
		break;
	case BATT_EVENT_WIBRO:
		break;
	case BATT_EVENT_LTE:
		break;
	case BATT_EVENT_LCD:
		break;
#if defined(CONFIG_ISDB_CHARGING_CONTROL)
	case BATT_EVENT_ISDB:
		break;
#endif
	case BATT_EVENT_GPS:
		break;
	case BATT_EVENT:
		break;
	case BATT_HIGH_CURRENT_USB:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->is_hc_usb);
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case TEST_CHARGE_CURRENT:
		{
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					value.intval);
		}
		break;
#if defined(CONFIG_STEP_CHARGING)
	case TEST_STEP_CONDITION:
		{
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					battery->test_step_condition);
		}
		break;
#endif
#endif
	case SET_STABILITY_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->stability_test);
		break;
	case BATT_CAPACITY_MAX:
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATT_REPCAP_1ST:
		if (battery->pdata->soc_by_repcap_en) {
			psy_do_property(battery->pdata->fuelgauge_name, get,
					POWER_SUPPLY_EXT_PROP_CHARGE_FULL_REPCAP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		} else
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", -1);
		break;
	case BATT_INBAT_VOLTAGE:
	case BATT_INBAT_VOLTAGE_OCV:
		ret = sec_bat_get_inbat_vol_ocv(battery);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_INBAT_VOLTAGE_ADC:
		/* run twice */
		ret = (sec_bat_get_inbat_vol_by_adc(battery) +\
			sec_bat_get_inbat_vol_by_adc(battery)) / 2;
		dev_info(battery->dev, "in-battery voltage adc(%d)\n", ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_VBYP_VOLTAGE:
		value.intval = SEC_BATTERY_VBYP;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_INPUT, value);
		pr_info("%s: vbyp(%d)mV\n", __func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case CHECK_SUB_CHG:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHECK_SUB_CHG_I2C, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		pr_info("%s : CHECK_SUB_CHG=%d\n",__func__,value.intval);
		break;
	case BATT_INBAT_WIRELESS_CS100:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_STATUS, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case HMT_TA_CONNECTED:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(battery->cable_type == SEC_BATTERY_CABLE_HMT_CONNECTED) ? 1 : 0);
		break;
	case HMT_TA_CHARGE:
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER) || IS_ENABLED(CONFIG_CCIC_NOTIFIER)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(battery->current_event & SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE) ? 0 : 1);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			(battery->cable_type == SEC_BATTERY_CABLE_HMT_CHARGE) ? 1 : 0);
#endif
		break;
#if defined(CONFIG_SEC_FACTORY)
	case AFC_TEST_FG_MODE:
		break;
	case NOZX_CTRL:
		break;
#endif
	case FG_CYCLE:
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
		value.intval = SEC_DUAL_BATTERY_MAIN;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_CYCLES, value);
#else
		value.intval = SEC_BATTERY_CAPACITY_CYCLE;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
#endif
		value.intval = value.intval / 100;
		dev_info(battery->dev, "fg cycle(%d)\n", value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	case FG_SUB_CYCLE:
		value.intval = SEC_DUAL_BATTERY_SUB;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_CYCLES, value);

		//value.intval = value.intval / 100;
		dev_info(battery->dev, "fg cycle(%d)\n", value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
#endif
	case FG_FULL_VOLTAGE:
		{
			int recharging_voltage = battery->pdata->recharge_condition_vcell;

			if (battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE) {
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)
					recharging_voltage = battery->pdata->swelling_high_rechg_voltage;
				else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
					recharging_voltage = battery->pdata->swelling_low_cool3_rechg_voltage;
				else /* cool1 cool2 */
					recharging_voltage = battery->pdata->swelling_low_rechg_voltage;
			}

			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d\n",
				value.intval, recharging_voltage);

#ifdef CONFIG_IFPMIC_LIMITER
			psy_do_property(battery->pdata->sub_limiter_name, get,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d\n",
				value.intval, recharging_voltage);
#endif
			break;
		}
	case FG_FULLCAPNOM:
		value.intval =
			SEC_BATTERY_CAPACITY_AGEDCELL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST_DETACHABLE)
	case BATT_AFTER_MANUFACTURED:
#endif
	case BATTERY_CYCLE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->batt_cycle);
		break;
	case BATTERY_CYCLE_TEST:
		break;
	case BATT_WPC_TEMP:
		value.intval = sec_bat_get_temperature(battery->dev, &battery->pdata->wpc_thm_info, 0,
				battery->pdata->charger_name, battery->pdata->fuelgauge_name,
				battery->pdata->adc_read_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case BATT_WPC_TEMP_ADC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->pdata->wpc_thm_info.adc);
		break;
	case BATT_WIRELESS_MST_SWITCH_TEST:
		value.intval = SEC_WIRELESS_MST_SWITCH_VERIFY;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		pr_info("%s MST switch verify. result: %x\n", __func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	case BATT_WIRELESS_FIRMWARE_UPDATE:
		value.intval = SEC_WIRELESS_OTP_FIRM_VERIFY;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		pr_info("%s RX firmware verify. result: %d\n", __func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case OTP_FIRMWARE_RESULT:
		value.intval = SEC_WIRELESS_OTP_FIRM_RESULT;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_IC_GRADE:
		value.intval = SEC_WIRELESS_IC_GRADE;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x ", value.intval);

		value.intval = SEC_WIRELESS_IC_REVISION;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", value.intval);
		break;
	case WC_IC_CHIP_ID:
		value.intval = SEC_WIRELESS_IC_CHIP_ID;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case OTP_FIRMWARE_VER_BIN:
		value.intval = SEC_WIRELESS_OTP_FIRM_VER_BIN;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case OTP_FIRMWARE_VER:
		value.intval = SEC_WIRELESS_OTP_FIRM_VER;
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_MANUFACTURER, value);

		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
#endif
	case WC_PHM_CTRL:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_RX_PHM, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_VOUT:
		/* wc_vout and wc_vrect are supposed to be read with wireless connection or uno on because of so much i2c fails.
		   But wired + wireless charging case makes ldo toggle which only have vrect turning on, in this case needs this node for test app. */
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_VRECT:
		/* wc_vout and wc_vrect are supposed to be read with wireless connection or uno on because of so much i2c fails.
		   But wired + wireless charging case makes ldo toggle which only have vrect turning on, in this case needs this node for test app. */
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_ENERGY_AVG, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_TX_EN:
		pr_info("%s wc tx enable(%d)",__func__, battery->wc_tx_enable);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wc_tx_enable);
		break;
	case WC_TX_VOUT:
		pr_info("%s wc tx vout(%d)",__func__, battery->wc_tx_vout);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wc_tx_vout);
		break;
	case BATT_HV_WIRELESS_STATUS:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATT_HV_WIRELESS_PAD_CTRL:
		break;
	case WC_TX_ID:
		if (battery->wc_tx_enable || is_wireless_all_type(battery->cable_type)) {
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
			pr_info("%s TX ID (0x%x)",__func__, value.intval);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_OP_FREQ:
		if (battery->wc_tx_enable || is_wireless_all_type(battery->cable_type)) {
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ, value);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case WC_CMD_INFO:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_CMD, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x ",
			value.intval);

		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_TRX_VAL, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%02x ",
			value.intval);
		break;
	case WC_RX_CONNECTED:
		pr_info("%s RX Connected (%d)",__func__, battery->wc_rx_connected);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wc_rx_connected);
		break;
	case WC_RX_CONNECTED_DEV:
		pr_info("%s RX Type (%d)",__func__, battery->wc_rx_type);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->wc_rx_type);
		break;
	case WC_TX_MFC_VIN_FROM_UNO:
		if (battery->wc_tx_enable || is_wireless_all_type(battery->cable_type)) {
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN, value);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case WC_TX_MFC_IIN_FROM_UNO:
		if (battery->wc_tx_enable || is_wireless_all_type(battery->cable_type)) {
			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
#if defined(CONFIG_WIRELESS_TX_MODE)
	case WC_TX_AVG_CURR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		battery->tx_avg_curr);
	break;
	case WC_TX_TOTAL_PWR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
		battery->tx_total_power);
		/* If PMS read this value, average Tx current will be reset */
		battery->tx_time_cnt = 0;
		battery->tx_avg_curr = 0;
		battery->tx_total_power = 0;
	break;
#endif
	case WC_TX_STOP_CAPACITY:
		ret = battery->pdata->tx_stop_capacity;
		pr_info("%s tx stop capacity = %d%%", __func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			ret);
		break;
	case WC_TX_TIMER_EN:
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_TUNE_FLOAT_VOLTAGE:
		ret = get_sec_vote_result(battery->fv_vote);
		pr_info("%s float voltage = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_INPUT_CHARGE_CURRENT:
		ret = battery->pdata->charging_current[i].input_current_limit;
		pr_info("%s input charge current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_FAST_CHARGE_CURRENT:
		ret = battery->pdata->charging_current[i].fast_charging_current;
		pr_info("%s fast charge current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_WIRELESS_VOUT_CURRENT:
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		ret = battery->wc20_vout;
		pr_info("%s vout(%d) input_current(%d)",__func__, ret,
			(battery->wc20_vout == 0) ? -1 : (battery->wc20_rx_power / battery->wc20_vout));

		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
#endif
		break;
	case BATT_TUNE_UI_TERM_CURRENT_1ST:
		ret = battery->pdata->full_check_current_1st;
		pr_info("%s ui term current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_UI_TERM_CURRENT_2ND:
		ret = battery->pdata->full_check_current_2nd;
		pr_info("%s ui term current = %d mA",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_CHG_TEMP_HIGH:
		ret = battery->pdata->chg_high_temp;
		pr_info("%s chg_high_temp = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_CHG_TEMP_REC:
		ret = battery->pdata->chg_high_temp_recovery;
		pr_info("%s chg_high_temp_recovery	= %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_CHG_LIMIT_CUR:
		ret = battery->pdata->chg_charging_limit_current;
		pr_info("%s chg_charging_limit_current = %d ",__func__, ret);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_LRP_TEMP_HIGH_LCDON:
		{
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d %d %d %d\n",
				battery->pdata->lrp_temp[LRP_NORMAL].trig[ST2][LCD_ON],
				battery->pdata->lrp_temp[LRP_NORMAL].recov[ST2][LCD_ON],
				battery->pdata->lrp_temp[LRP_NORMAL].trig[ST1][LCD_ON],
				battery->pdata->lrp_temp[LRP_NORMAL].recov[ST1][LCD_ON]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = LRP_NORMAL + 1; j < LRP_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf),
					size, "%d %d %d %d\n",
								battery->pdata->lrp_temp[j].trig[ST2][LCD_ON],
								battery->pdata->lrp_temp[j].recov[ST2][LCD_ON],
								battery->pdata->lrp_temp[j].trig[ST1][LCD_ON],
								battery->pdata->lrp_temp[j].recov[ST1][LCD_ON]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case BATT_TUNE_LRP_TEMP_HIGH_LCDOFF:
		{
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d %d %d %d\n",
				battery->pdata->lrp_temp[LRP_NORMAL].trig[ST2][LCD_OFF],
				battery->pdata->lrp_temp[LRP_NORMAL].recov[ST2][LCD_OFF],
				battery->pdata->lrp_temp[LRP_NORMAL].trig[ST1][LCD_OFF],
				battery->pdata->lrp_temp[LRP_NORMAL].recov[ST1][LCD_OFF]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = LRP_NORMAL + 1; j < LRP_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf),
					size, "%d %d %d %d\n",
								battery->pdata->lrp_temp[j].trig[ST2][LCD_OFF],
								battery->pdata->lrp_temp[j].recov[ST2][LCD_OFF],
								battery->pdata->lrp_temp[j].trig[ST1][LCD_OFF],
								battery->pdata->lrp_temp[j].recov[ST1][LCD_OFF]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case BATT_TUNE_COIL_TEMP_HIGH:
		break;
	case BATT_TUNE_COIL_TEMP_REC:
		break;
	case BATT_TUNE_COIL_LIMIT_CUR:
		break;
	case BATT_TUNE_WPC_TEMP_HIGH:
		ret = battery->pdata->wpc_high_temp;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_WPC_TEMP_HIGH_REC:
		ret = battery->pdata->wpc_high_temp_recovery;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d\n",
			battery->pdata->dchg_high_temp[0],
			battery->pdata->dchg_high_temp[1],
			battery->pdata->dchg_high_temp[2],
			battery->pdata->dchg_high_temp[3]);
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH_REC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d\n",
			battery->pdata->dchg_high_temp_recovery[0],
			battery->pdata->dchg_high_temp_recovery[1],
			battery->pdata->dchg_high_temp_recovery[2],
			battery->pdata->dchg_high_temp_recovery[3]);
		break;
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d\n",
			battery->pdata->dchg_high_batt_temp[0],
			battery->pdata->dchg_high_batt_temp[1],
			battery->pdata->dchg_high_batt_temp[2],
			battery->pdata->dchg_high_batt_temp[3]);
		break;
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH_REC:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d\n",
			battery->pdata->dchg_high_batt_temp_recovery[0],
			battery->pdata->dchg_high_batt_temp_recovery[1],
			battery->pdata->dchg_high_batt_temp_recovery[2],
			battery->pdata->dchg_high_batt_temp_recovery[3]);
		break;
	case BATT_TUNE_DCHG_LIMIT_INPUT_CUR:
		ret = battery->pdata->dchg_input_limit_current;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_DCHG_LIMIT_CHG_CUR:
		ret = battery->pdata->dchg_charging_limit_current;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
#if defined(CONFIG_WIRELESS_TX_MODE)
	case BATT_TUNE_TX_MFC_IOUT_GEAR:
		ret = battery->pdata->tx_mfc_iout_gear;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
	case BATT_TUNE_TX_MFC_IOUT_PHONE:
		ret = battery->pdata->tx_mfc_iout_phone;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		break;
#endif
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case BATT_UPDATE_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
				battery->data_path ? "OK" : "NOK");
		break;
#endif
	case BATT_MISC_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->misc_event);
		break;
	case BATT_TX_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->tx_event);
		if (battery->tx_event & BATT_TX_EVENT_WIRELESS_TX_ERR) {
			/* clear tx all event */
			sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
		}
		break;
	case BATT_EXT_DEV_CHG:
		break;
	case BATT_WDT_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				battery->wdt_kick_disable);
		break;
	case MODE:
		value.strval = NULL;
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			(value.strval) ? value.strval : "main");
		break;
	case CHECK_PS_READY:
		value.intval = battery->pdic_ps_rdy;
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		pr_info("%s : CHECK_PS_READY=%d\n",__func__,value.intval);
		break;
	case BATT_CHIP_ID:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHIP_ID, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
	case ERROR_CAUSE:
		{
			int error_cause = SEC_BAT_ERROR_CAUSE_NONE;

			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_ERROR_CAUSE, value);
			error_cause |= value.intval;
			pr_info("%s: ERROR_CAUSE = 0x%X ",__func__, error_cause);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				error_cause);
		}
		break;
	case CISD_FULLCAPREP_MAX:
		{
			union power_supply_propval fullcaprep_val;

			fullcaprep_val.intval = SEC_BATTERY_CAPACITY_FULL;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_NOW, fullcaprep_val);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					fullcaprep_val.intval);
		}
		break;
	case CISD_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->data[CISD_DATA_RESET_ALG]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_DATA_RESET_ALG + 1; j < CISD_DATA_MAX_PER_DAY; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
					cisd_data_str[CISD_DATA_RESET_ALG], pcisd->data[CISD_DATA_RESET_ALG]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_DATA_RESET_ALG + 1; j < CISD_DATA_MAX; j++) {
				if (battery->pdata->ignore_cisd_index[j / 32] & (0x1 << (j % 32)))
					continue;
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"", cisd_data_str[j], pcisd->data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_DATA_D_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_data_str_d[CISD_DATA_FULL_COUNT_PER_DAY-CISD_DATA_MAX],
				pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_DATA_FULL_COUNT_PER_DAY + 1; j < CISD_DATA_MAX_PER_DAY; j++) {
				if (battery->pdata->ignore_cisd_index_d[(j - CISD_DATA_FULL_COUNT_PER_DAY) / 32] & (0x1 << ((j - CISD_DATA_FULL_COUNT_PER_DAY) % 32)))
					continue;
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
				cisd_data_str_d[j-CISD_DATA_MAX], pcisd->data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Data */
			for (j = CISD_DATA_FULL_COUNT_PER_DAY; j < CISD_DATA_MAX_PER_DAY; j++)
				pcisd->data[j] = 0;

			pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
			pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;
			pcisd->data[CISD_DATA_CHG_RETENTION_TIME_PER_DAY] = 0;
			pcisd->data[CISD_DATA_TOTAL_CHG_RETENTION_TIME_PER_DAY] = 0;

			pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = 0xFFFF;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_WIRE_COUNT:
		{
			struct cisd *pcisd = &battery->cisd;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				pcisd->data[CISD_DATA_WIRE_COUNT]);
		}
		break;
	case CISD_WC_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			struct pad_data *pad_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->padlock);
			pad_data = pcisd->pad_array;
			snprintf(temp_buf, size, "%d", pcisd->pad_count);
			while ((pad_data != NULL) && ((pad_data = pad_data->next) != NULL) &&
					(pad_data->id < MAX_PAD_ID) && (j++ < pcisd->pad_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, " 0x%02x:%d", pad_data->id, pad_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->padlock);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_WC_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			struct pad_data *pad_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->padlock);
			pad_data = pcisd->pad_array;
			snprintf(temp_buf+strlen(temp_buf), size, "\"%s\":\"%d\"",
				PAD_INDEX_STRING, pcisd->pad_count);
			while ((pad_data != NULL) && ((pad_data = pad_data->next) != NULL) &&
					(pad_data->id < MAX_PAD_ID) && (j++ < pcisd->pad_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s%02x\":\"%d\"",
					PAD_JSON_STRING, pad_data->id, pad_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->padlock);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_POWER_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			struct power_data *power_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->powerlock);
			power_data = pcisd->power_array;
			snprintf(temp_buf+strlen(temp_buf), size, "%d", pcisd->power_count);
			while ((power_data != NULL) && ((power_data = power_data->next) != NULL) &&
					(power_data->power < MAX_CHARGER_POWER) && (j++ < pcisd->power_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d:%d", power_data->power, power_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->powerlock);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_POWER_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			struct power_data *power_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->powerlock);
			power_data = pcisd->power_array;
			snprintf(temp_buf+strlen(temp_buf), size, "\"%s\":\"%d\"",
				POWER_COUNT_JSON_STRING, pcisd->power_count);
			while ((power_data != NULL) && ((power_data = power_data->next) != NULL) &&
					(power_data->power < MAX_CHARGER_POWER) && (j++ < pcisd->power_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s%d\":\"%d\"",
					POWER_JSON_STRING, power_data->power, power_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->powerlock);

			/* clear daily power data */
			init_cisd_power_data(&battery->cisd);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_PD_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			struct pd_data *pd_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->pdlock);
			pd_data = pcisd->pd_array;
			snprintf(temp_buf+strlen(temp_buf), size, "%d", pcisd->pd_count);
			while ((pd_data != NULL) && ((pd_data = pd_data->next) != NULL) &&
					(pd_data->pid < MAX_SS_PD_PID) && (j++ < pcisd->pd_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, " 0x%04x:%d", pd_data->pid, pd_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->pdlock);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_PD_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			struct pd_data *pd_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->pdlock);
			pd_data = pcisd->pd_array;
			snprintf(temp_buf+strlen(temp_buf), size, "\"%s\":\"%d\"",
				PD_COUNT_JSON_STRING, pcisd->pd_count);
			while ((pd_data != NULL) && ((pd_data = pd_data->next) != NULL) &&
					(pd_data->pid < MAX_SS_PD_PID) && (j++ < pcisd->pd_count)) {
				if (pd_data->pid == 0x0)
					snprintf(temp_buf+strlen(temp_buf), size, ",\"PID_OTHER\":\"%d\"",
						pd_data->count);
				else
					snprintf(temp_buf+strlen(temp_buf), size, ",\"%s%04x\":\"%d\"",
						PD_JSON_STRING, pd_data->pid, pd_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->pdlock);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case CISD_DCERR_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			struct dcerr_data *dcerr_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->dcerrlock);
			dcerr_data = pcisd->dcerr_array;
			snprintf(temp_buf, size, "%d", pcisd->dcerr_count);
			while ((dcerr_data != NULL) && ((dcerr_data = dcerr_data->next) != NULL) &&
					(dcerr_data->cause < MAX_DCERR_CAUSE) && (j++ < pcisd->dcerr_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, " 0x%02x:%d", dcerr_data->cause, dcerr_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->dcerrlock);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_DCERR_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			struct dcerr_data *dcerr_data = NULL;
			char temp_buf[1024] = {0,};
			int j = 0, size = 1024;

			mutex_lock(&pcisd->dcerrlock);
			dcerr_data = pcisd->dcerr_array;
			snprintf(temp_buf+strlen(temp_buf), size, "\"%s\":\"%d\"",
					DCERR_CAUSE_JSON_STRING, pcisd->dcerr_count);
			while ((dcerr_data != NULL) && ((dcerr_data = dcerr_data->next) != NULL) &&
					(dcerr_data->cause < MAX_DCERR_CAUSE) && (j++ < pcisd->dcerr_count)) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s%02x\":\"%d\"",
						DCERR_JSON_STRING, dcerr_data->cause, dcerr_data->count);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			mutex_unlock(&pcisd->dcerrlock);

			/* clear daily dcerr data */
			init_cisd_dcerr_data(&battery->cisd);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
#endif
	case CISD_CABLE_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->cable_data[CISD_CABLE_TA]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_CABLE_TA + 1; j < CISD_CABLE_TYPE_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->cable_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);

		}
		break;

	case CISD_CABLE_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_cable_data_str[CISD_CABLE_TA], pcisd->cable_data[CISD_CABLE_TA]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = CISD_CABLE_TA + 1; j < CISD_CABLE_TYPE_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
					cisd_cable_data_str[j], pcisd->cable_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Cable Data */
			for (j = CISD_CABLE_TA; j < CISD_CABLE_TYPE_MAX; j++)
				pcisd->cable_data[j] = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_TX_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->tx_data[TX_ON]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = TX_ON + 1; j < TX_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->tx_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_TX_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_tx_data_str[TX_ON], pcisd->tx_data[TX_ON]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = TX_ON + 1; j < TX_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
					cisd_tx_data_str[j], pcisd->tx_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Tx Data */
			for (j = TX_ON; j < TX_DATA_MAX; j++)
				pcisd->tx_data[j] = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_EVENT_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "%d", pcisd->event_data[EVENT_DC_ERR]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = EVENT_DC_ERR + 1; j < EVENT_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, " %d", pcisd->event_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case CISD_EVENT_DATA_JSON:
		{
			struct cisd *pcisd = &battery->cisd;
			char temp_buf[1024] = {0,};
			int j = 0;
			int size = 0;

			snprintf(temp_buf, sizeof(temp_buf), "\"%s\":\"%d\"",
				cisd_event_data_str[EVENT_DC_ERR], pcisd->event_data[EVENT_DC_ERR]);
			size = sizeof(temp_buf) - strlen(temp_buf);

			for (j = EVENT_DC_ERR + 1; j < EVENT_DATA_MAX; j++) {
				snprintf(temp_buf+strlen(temp_buf), size, ",\"%s\":\"%d\"",
					cisd_event_data_str[j], pcisd->event_data[j]);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			/* Clear Daily Event Data */
			for (j = EVENT_DC_ERR; j < EVENT_DATA_MAX; j++)
				pcisd->event_data[j] = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case PREV_BATTERY_DATA:
		{
			if (battery->enable_update_data)
				i += scnprintf(buf + i, PAGE_SIZE - i, "%d, %d, %d, %d\n",
					battery->voltage_now, battery->temperature, battery->is_jig_on,
					(battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING) ? 1 : 0);
		}
		break;
	case PREV_BATTERY_INFO:
		{
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d,%d,%d,%d\n",
				battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);
			pr_info("%s: Read Prev Battery Info : %d, %d, %d, %d\n", __func__,
				battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);
		}
		break;
	case SAFETY_TIMER_SET:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->safety_timer_set);
		break;
	case BATT_SWELLING_CONTROL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->skip_swelling);
		break;
	case BATT_BATTERY_ID:
		value.intval = 0;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_BATTERY_ID, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	case BATT_SUB_BATTERY_ID:
		value.intval = SEC_DUAL_BATTERY_SUB;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_BATTERY_ID, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		break;
#endif
	case BATT_TEMP_CONTROL_TEST:
		{
			int temp_ctrl_t = 0;

			if (battery->current_event & SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST)
				temp_ctrl_t = 1;
			else
				temp_ctrl_t = 0;

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				temp_ctrl_t);
		}
		break;
	case SAFETY_TIMER_INFO:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%ld\n",
			       battery->safety_timer.rescaled_remaining_time);
		break;
	case BATT_SHIPMODE_TEST:
		value.intval = 0;
		ret = psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_SHIPMODE_TEST, value);
		if (ret < 0) {
			pr_info("%s: not support BATT_SHIPMODE_TEST\n", __func__);
			value.intval = 0;
		} else {
			pr_info("%s: show BATT_SHIPMODE_TEST(%d)\n", __func__, value.intval);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case BATT_MISC_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->display_test);
		break;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	case BATT_TEMP_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d %d %d\n",
			battery->pdata->bat_thm_info.test,
			battery->pdata->usb_thm_info.test,
			battery->pdata->wpc_thm_info.test,
			battery->pdata->chg_thm_info.test,
			battery->pdata->dchg_thm_info.test,
			battery->pdata->sub_bat_thm_info.test);
		break;
#else
	case BATT_TEMP_TEST:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d %d %d %d %d %d\n",
			battery->pdata->bat_thm_info.test,
			battery->pdata->usb_thm_info.test,
			battery->pdata->wpc_thm_info.test,
			battery->pdata->chg_thm_info.test,
			battery->pdata->dchg_thm_info.test,
			battery->pdata->blk_thm_info.test,
			battery->lrp_test);
		break;
#endif
	case BATT_VOLT_TEST:
		break;
	case BATT_CURRENT_EVENT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->current_event);
		break;
	case BATT_JIG_GPIO:
 		value.intval = 0;
		ret = psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_JIG_GPIO, value);
		if (value.intval < 0 || ret < 0) {
			value.intval = -1;
			pr_info("%s: does not support JIG GPIO PIN READ\n", __func__);
		}
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
	case CC_INFO:
		{
			union power_supply_propval cc_val;

			cc_val.intval = SEC_BATTERY_CAPACITY_QH;
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_ENERGY_NOW, cc_val);

			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					cc_val.intval);
		}
		break;
#if defined(CONFIG_WIRELESS_AUTH)
	case WC_AUTH_ADT_SENT:
		{
			//union power_supply_propval val = {0, };
			u8 auth_mode;

			psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS, value);
			auth_mode = value.intval;
			if (auth_mode == WIRELESS_AUTH_WAIT)
				value.strval = "None";
			else if (auth_mode == WIRELESS_AUTH_START)
				value.strval = "Start";
			else if (auth_mode == WIRELESS_AUTH_SENT)
				value.strval = "Sent";
			else if (auth_mode == WIRELESS_AUTH_RECEIVED)
				value.strval = "Received";
			else if (auth_mode == WIRELESS_AUTH_FAIL)
				value.strval = "Fail";
			else if (auth_mode == WIRELESS_AUTH_PASS)
				value.strval = "Pass";
			else
				value.strval = "None";
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", value.strval);
		}
		break;
#endif
	case WC_DUO_RX_POWER:
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			value.intval);
		break;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	case BATT_MAIN_VOLTAGE:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
#if IS_ENABLED(CONFIG_DUAL_SBP)
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_DUAL_SBP_VOLTAGE_NOW, value);
#else
			psy_do_property(battery->pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_VOLTAGE:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
#if IS_ENABLED(CONFIG_DUAL_SBP)
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_DUAL_SBP_VOLTAGE_NOW, value);
#else
			psy_do_property(battery->pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_VCELL:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
			psy_do_property(battery->pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_VCELL:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
			psy_do_property(battery->pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_CURRENT_MA:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
#if IS_ENABLED(CONFIG_DUAL_SBP)
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_DUAL_SBP_CURRENT_NOW, value);
#else
			psy_do_property(battery->pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_CURRENT_MA:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
#if IS_ENABLED(CONFIG_DUAL_SBP)
			psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_DUAL_SBP_CURRENT_NOW, value);
#else
			psy_do_property(battery->pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
#endif
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_CON_DET:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
			psy_do_property(battery->pdata->dual_battery_name, get,
			POWER_SUPPLY_EXT_PROP_DUAL_BAT_DET, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_CON_DET:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
			psy_do_property(battery->pdata->dual_battery_name, get,
			POWER_SUPPLY_EXT_PROP_DUAL_BAT_DET, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_VCHG:
		{
			value.intval = SEC_BATTERY_VOLTAGE_MV;
			psy_do_property(battery->pdata->main_limiter_name, get,
				POWER_SUPPLY_EXT_PROP_CHG_VOLTAGE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_VCHG:
		{
			value.intval = SEC_BATTERY_VOLTAGE_MV;
			psy_do_property(battery->pdata->sub_limiter_name, get,
				POWER_SUPPLY_EXT_PROP_CHG_VOLTAGE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_CHG_VOLTAGE:
		{
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
			psy_do_property(battery->pdata->main_limiter_name, get,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
#else
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
#endif
		}
		break;
	case BATT_SUB_CHG_VOLTAGE:
		{
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
			psy_do_property(battery->pdata->sub_limiter_name, get,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
#else
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
#endif
		}
		break;
	case BATT_MAIN_RECHG_VOLTAGE:
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->recharging_voltage_main);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
#endif
		break;
	case BATT_SUB_RECHG_VOLTAGE:
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			       battery->recharging_voltage_sub);
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
#endif
		break;
#if IS_ENABLED(CONFIG_LIMITER_S2ASL01)
	case BATT_MAIN_ENB: /* This pin is reversed by FET */
		{
			if (battery->pdata->main_bat_enb_gpio)
				value.intval = !gpio_get_value(battery->pdata->main_bat_enb_gpio);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_ENB2:
		{
			if (battery->pdata->main_bat_enb2_gpio)
				value.intval = gpio_get_value(battery->pdata->main_bat_enb2_gpio);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_ENB:
		{
			if (battery->pdata->sub_bat_enb_gpio)
				value.intval = gpio_get_value(battery->pdata->sub_bat_enb_gpio);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_PWR_MODE2:
		{
			psy_do_property(battery->pdata->sub_limiter_name, get,
				POWER_SUPPLY_EXT_PROP_POWER_MODE2, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
#else /* max17333 */
	case BATT_MAIN_SHIPMODE:
		{
			value.intval = 0;
			ret = psy_do_property(battery->pdata->main_limiter_name, get,
					POWER_SUPPLY_EXT_PROP_LIMITER_SHIPMODE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_SHIPMODE:
		{
			value.intval = 0;
			ret = psy_do_property(battery->pdata->sub_limiter_name, get,
					POWER_SUPPLY_EXT_PROP_LIMITER_SHIPMODE, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_VBAT:
		{
			ret = sec_bat_dual_battery_vbat(battery, SEC_DUAL_BATTERY_MAIN);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		}
		break;
	case BATT_SUB_VBAT:
		{
			ret = sec_bat_dual_battery_vbat(battery, SEC_DUAL_BATTERY_SUB);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				ret);
		}
		break;

#endif
#if (IS_ENABLED(CONFIG_DUAL_FUELGAUGE) || IS_ENABLED(CONFIG_DUAL_SBP))
	case BATT_MAIN_SOC:
		{
			value.intval = battery->main_capacity/10;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_SOC:
		{
			value.intval = battery->sub_capacity/10;
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
#endif
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	case BATT_MAIN_RAW_SOC:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
			psy_do_property(battery->pdata->dual_fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_RAWSOC, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_RAW_SOC:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
			psy_do_property(battery->pdata->dual_fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_RAWSOC, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_REPCAP:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
			psy_do_property(battery->pdata->dual_fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_REPCAP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_REPCAP:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
			psy_do_property(battery->pdata->dual_fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_REPCAP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_MAIN_FULLCAPREP:
		{
			value.intval = SEC_DUAL_BATTERY_MAIN;
			psy_do_property(battery->pdata->dual_fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_FULLCAPREP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
	case BATT_SUB_FULLCAPREP:
		{
			value.intval = SEC_DUAL_BATTERY_SUB;
			psy_do_property(battery->pdata->dual_fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_FULLCAPREP, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		}
		break;
#endif
#endif
	case EXT_EVENT:
		break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case DIRECT_CHARGING_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->pd_list.now_isApdo);
		break;
	case DIRECT_CHARGING_STEP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->step_chg_status);
		break;
	case DIRECT_CHARGING_IIN:
		if (is_pd_apdo_wire_type(battery->wire_status)) {
			value.intval = SEC_BATTERY_IIN_UA;
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_MEASURE_INPUT, value);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				value.intval);
		} else {
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
					0);
		}
		break;
	case DIRECT_CHARGING_CHG_STATUS:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_CHG_STATUS, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			value.strval);
		break;
	case DIRECT_CHARGING_RATIO:
		if (is_dc_higher_ratio_support())
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
				get_sec_vote_result(battery->dc_op_mode_vote));
		else
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", 0);
		break;
	case SWITCH_CHARGING_SOURCE:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
		pr_info("%s Test Charging Source(%d) ",__func__, value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case DC_ERROR_CAUSE:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_DC_ERROR_CAUSE, value);
		pr_info("%s dc err cause(0x%x) ",__func__, 0xFF & value.intval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", value.intval);
		break;
#else
	case DIRECT_CHARGING_STATUS:
		ret = -1; /* DC not supported model returns -1 */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", ret);
		break;
#endif
	case CHARGING_TYPE:
		{
			if (battery->cable_type > 0 && battery->cable_type < SEC_BATTERY_CABLE_MAX) {
				value.strval = sb_get_ct_str(battery->cable_type);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
				if (is_pd_apdo_wire_type(battery->cable_type) &&
					battery->current_event & SEC_BAT_CURRENT_EVENT_DC_ERR)
					value.strval = "PDIC";
#endif
			} else
				value.strval = "UNKNOWN";
			pr_info("%s: CHARGING_TYPE = %s\n",__func__, value.strval);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", value.strval);
		}
		break;
	case BATT_FACTORY_MODE:
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->usb_factory_init ? battery->usb_factory_mode : sec_bat_get_facmode());
#else
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", sec_bat_get_facmode());
#endif
		break;
	case PD_DISABLE:
		if (battery->pd_disable)
			value.strval = "PD Disabled";
		else
			value.strval = "PD Enabled";
		pr_info("%s: PD = %s\n",__func__, value.strval);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->pd_disable);
		break;
	case FACTORY_MODE_RELIEVE:
		break;
	case FACTORY_MODE_BYPASS:
		break;
	case NORMAL_MODE_BYPASS:
		break;
	case FACTORY_VOLTAGE_REGULATION:
		break;
	case FACTORY_MODE_DISABLE:
		break;
	case CHARGE_OTG_CONTROL:
		break;
	case CHARGE_UNO_CONTROL:
		break;
	case CHARGE_COUNTER_SHADOW:
		psy_do_property("battery", get,
			POWER_SUPPLY_EXT_PROP_CHARGE_COUNTER_SHADOW, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case VOTER_STATUS:
		i = show_sec_vote_status(buf, PAGE_SIZE);
		break;
#if defined(CONFIG_WIRELESS_IC_PARAM)
	case WC_PARAM_INFO:
		ret = psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_WIRELESS_PARAM_INFO, value);
		if (ret < 0) {
			i = -EINVAL;
		} else {
			pr_info("%s: WC_PARAM_INFO(0x%08x)\n", __func__, value.intval);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		}
	break;
#endif
	case CHG_INFO:
	{
		unsigned short vid = 0, pid = 0;
		unsigned int xid = 0;

		sec_pd_get_vid_pid(&vid, &pid, &xid);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%04x %04x %08x\n", vid, pid, xid);
	}
		break;
	case LRP:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->lrp);
		break;
	case HP_D2D:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", battery->hp_d2d);
		break;
	case CHARGER_IC_NAME:
		ret = psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_CHARGER_IC_NAME, value);
		if (ret < 0) {
			pr_info("%s: read fail\n", __func__);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "NONAME");
		} else {
			pr_info("%s: CHARGER_IC_NAME: %s\n", __func__, value.strval);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", value.strval);
		}
		break;
	case DC_RB_EN:
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VOLTAGE, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case DC_OP_MODE:
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_DC_OP_MODE, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		break;
	case DC_ADC_MODE:
		break;
	case DC_VBUS:
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VBUS, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case CHG_TYPE:
		{
			char temp_buf[20] = {0,};

			value.strval = sb_get_ct_str(battery->cable_type);
			strncpy(temp_buf, value.strval, sizeof(temp_buf) - 1);
			if (is_pd_apdo_wire_type(battery->cable_type))
				snprintf(temp_buf+strlen(temp_buf), sizeof(temp_buf), "_%d", battery->pd_rated_power);
			pr_info("%s: CHG_TYPE = %s\n", __func__, temp_buf);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
	case MST_EN:
		psy_do_property("battery", get,
			POWER_SUPPLY_EXT_PROP_MST_EN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case SPSN_TEST:
		/* Only MD15 supports this function (2022y 10m 12d) */
		ret = psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_SPSN_TEST, value);
		if (ret < 0) {
			pr_info("%s: Does not support SPSN_TEST(%d)\n", __func__, ret);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", ret);
		} else {
			pr_info("%s: SPSN_DTLS: 0x%x\n", __func__, value.intval);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n", value.intval);
		}
		break;
	case CHG_SOC_LIM:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d %d\n",
			battery->pdata->store_mode_charging_min,
			battery->pdata->store_mode_charging_max);
		break;
	case MAG_COVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->mag_cover);
		break;
	case MAG_CLOAK:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "None");
		break;
	case MAG_VALUE:
		psy_do_property(battery->pdata->wireless_charger_name, get,
				POWER_SUPPLY_EXT_PROP_MPP_VALUE, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	case ARI_CNT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", "None");
		break;
#if IS_ENABLED(CONFIG_SBP_FG)
	case STATE_OF_HEALTH:
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_HEALTH, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
#endif
#if IS_ENABLED(CONFIG_BATTERY_AUTH_SLE956681)
	case VK_KEY_STATUS:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->vk_key_status);
		break;
#endif
	case ADC_RSENSE: /* for tuning adc_rsense of bat_thm only now */
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			battery->pdata->bat_thm_info.adc_rsense);
		break;
	case SUPPORT_FUNCTIONS:
		{
			char temp_buf[1024] = {0,};
			int size = 0;

			value.strval = "SB";
			snprintf(temp_buf, sizeof(temp_buf), "%s", value.strval);
			size = sizeof(temp_buf) - strlen(temp_buf);

			if (battery->pdata->en_batt_full_status_usage) {
				value.strval = "BFSU";
				snprintf(temp_buf+strlen(temp_buf), size, " %s", value.strval);
				size = sizeof(temp_buf) - strlen(temp_buf);
			}

			pr_info("%s: SUPPORT_FUNCTIONS = %s\n", __func__, temp_buf);
			i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n", temp_buf);
		}
		break;
#if IS_ENABLED(CONFIG_BATTERY_AUTH_EUR_DET_GPIO_SUPPORT)
	case BATT_AUTH_SUPPORT:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%u\n", battery->pdata->auth_ic_support);
		break;
#endif
	case OCPWARN:
		psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_OCPWARN, value);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", value.intval);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t sec_bat_store_attrs(
					struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sec_battery_attrs;
	int ret = -EINVAL;
	int x = 0, y = 0;
	int i = 0;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	char direct_charging_source_status[2] = {0, };
#endif

	union power_supply_propval value = {0, };

	switch (offset) {
	case BATT_RESET_SOC:
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
		if (sscanf(buf, "%10d\n", &x) == 1) {
		/* Do NOT reset fuel gauge in charging mode */
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
			if (battery->is_jig_on || battery->batt_f_mode != NO_MODE) {
#else
			if (battery->is_jig_on) {
#endif
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_BATT_RESET_SOC,
					BATT_MISC_EVENT_BATT_RESET_SOC);
				if (x == 2)
					value.intval =
						SEC_FUELGAUGE_CAPACITY_TYPE_RESET_SUB;
				else if (x == 3)
					value.intval =
						SEC_FUELGAUGE_CAPACITY_TYPE_RESET_SUB_PKCP;
				else if (x == 4)
					value.intval =
						SEC_FUELGAUGE_CAPACITY_TYPE_RESET_DUAL;
				else
					value.intval =
						SEC_FUELGAUGE_CAPACITY_TYPE_RESET;
				psy_do_property(battery->pdata->fuelgauge_name, set,
						POWER_SUPPLY_PROP_CAPACITY, value);
				dev_info(battery->dev, "do reset SOC\n");
				/* update battery info */
				sec_bat_get_battery_info(battery);
			}
		ret = count;
		}
#else
		/* Do NOT reset fuel gauge in charging mode */
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		if (battery->is_jig_on || battery->batt_f_mode != NO_MODE) {
#else
		if (battery->is_jig_on) {
#endif
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_BATT_RESET_SOC, BATT_MISC_EVENT_BATT_RESET_SOC);

			value.intval =
				SEC_FUELGAUGE_CAPACITY_TYPE_RESET;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CAPACITY, value);
			dev_info(battery->dev,"do reset SOC\n");
			/* update battery info */
			sec_bat_get_battery_info(battery);
		}
		ret = count;
#endif
		break;
	case BATT_READ_RAW_SOC:
		break;
	case BATT_READ_ADJ_SOC:
		break;
	case BATT_TYPE:
		strncpy(battery->batt_type, buf, sizeof(battery->batt_type) - 1);
		battery->batt_type[sizeof(battery->batt_type)-1] = '\0';
		ret = count;
		break;
	case BATT_VFOCV:
		break;
	case BATT_VOL_ADC:
		break;
	case BATT_VOL_ADC_CAL:
		break;
	case BATT_VOL_AVER:
		break;
	case BATT_VOL_ADC_AVER:
		break;
	case BATT_VOLTAGE_NOW:
		break;
	case BATT_CURRENT_UA_NOW:
		break;
	case BATT_CURRENT_UA_AVG:
		break;
	case BATT_FILTER_CFG:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_EXT_PROP_FILTER_CFG, value);
			ret = count;
		}
		break;
	case BATT_TEMP:
#if defined(CONFIG_ENG_BATTERY_CONCEPT) || defined(CONFIG_SEC_FACTORY)
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev, "%s: skip check thermal mode %s\n",
				__func__, (x ? "enable" : "disable"));
			if (x == 0)
				battery->skip_swelling = true;
			else
				battery->skip_swelling = false;
			ret = count;
		}
#endif
		break;
	case BATT_TEMP_RAW:
		break;
	case BATT_TEMP_ADC:
		break;
	case BATT_TEMP_AVER:
		break;
	case BATT_TEMP_ADC_AVER:
		break;
	case USB_TEMP:
		break;
	case USB_TEMP_ADC:
		break;
	case BATT_CHG_TEMP:
		break;
	case BATT_CHG_TEMP_ADC:
		break;
	case SUB_BAT_TEMP:
		break;
	case SUB_BAT_TEMP_ADC:
		break;
	case SUB_CHG_TEMP:
		break;
	case SUB_CHG_TEMP_ADC:
		break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case DCHG_ADC_MODE_CTRL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				 "%s : direct charger adc mode cntl : %d\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_ADC_CTRL, value);
			ret = count;
		}
		break;
	case DCHG_TEMP:
	case DCHG_TEMP_ADC:
	case DCHG_READ_BATP_BATN:
		break;
#endif
	case BLKT_TEMP:
		break;
	case BLKT_TEMP_ADC:
		break;
	case BATT_VF_ADC:
		break;
	case BATT_SLATE_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == SEC_SMART_SWITCH_SLATE) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SLATE, SEC_BAT_CURRENT_EVENT_SLATE);
				sec_vote(battery->chgen_vote, VOTER_SMART_SLATE, true, SEC_BAT_CHG_MODE_BUCK_OFF);
				sec_bat_set_mfc_off(battery, WPC_EN_SLATE, false);
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE) && defined(CONFIG_SEC_FACTORY)
				battery->usb_factory_slate_mode = true;
#endif
				dev_info(battery->dev,
					"%s: enable smart switch slate mode : %d\n", __func__, x);
			} else if (x == SEC_SLATE_MODE) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SLATE, SEC_BAT_CURRENT_EVENT_SLATE);
				sec_vote(battery->chgen_vote, VOTER_SLATE, true, SEC_BAT_CHG_MODE_BUCK_OFF);
				dev_info(battery->dev,
					"%s: enable slate mode : %d\n", __func__, x);
			} else if (x == SEC_SLATE_OFF) {
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SLATE);
				sec_vote(battery->chgen_vote, VOTER_SLATE, false, 0);
				sec_vote(battery->chgen_vote, VOTER_SMART_SLATE, false, 0);
				sec_bat_set_mfc_on(battery, WPC_EN_SLATE);
				/* recover smart switch src cap max current to 500mA */
				sec_bat_smart_sw_src(battery, false, 500);
				dev_info(battery->dev,
					"%s: disable slate mode : %d\n", __func__, x);
			} else if (x == SEC_SMART_SWITCH_SRC) {
				/* reduce smart switch src cap max current */
				sec_bat_smart_sw_src(battery, true, 0);
			} else {
				dev_info(battery->dev,
					"%s: SLATE MODE unknown command\n", __func__);
				return -EINVAL;
			}
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->cable_work, 0);
			ret = count;
		}
		break;
	case BATT_LP_CHARGING:
		break;
	case SIOP_ACTIVATED:
		break;
	case SIOP_LEVEL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
					"%s: siop level: %d\n", __func__, x);

			battery->wc_heating_start_time = 0;
			if (x == battery->siop_level) {
				dev_info(battery->dev,
					"%s: skip same siop level: %d\n", __func__, x);
				return count;
			} else if (x >= 0 && x <= 100 && battery->pdata->bat_thm_info.check_type) {
				battery->siop_level = x;
				if (battery->siop_level == 0)
					sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SIOP_LIMIT,
						SEC_BAT_CURRENT_EVENT_SIOP_LIMIT);
				else
					sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SIOP_LIMIT);
			} else {
				battery->siop_level = 100;
			}

			__pm_stay_awake(battery->siop_level_ws);
#if !defined(CONFIG_SEC_FACTORY)
			if (battery->siop_level == 100)
				if (!is_nocharge_type(battery->cable_type))
					sec_bat_check_temp_ctrl_by_cable(battery);
#endif
			queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);

			ret = count;
		}
		break;
	case SIOP_EVENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_CHARGING_SOURCE:
		break;
	case FG_REG_DUMP:
		break;
	case FG_RESET_CAP:
		break;
	case FG_CAPACITY:
		break;
	case FG_ASOC:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if (x >= 0 && x <= 100) {
				dev_info(battery->dev, "%s: batt_asoc(%d)\n", __func__, x);
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
				if (!x)
					sec_abc_send_event("MODULE=battery@WARN=store_fg_asoc0");
#endif
				battery->batt_asoc = x;
				battery->cisd.data[CISD_DATA_ASOC] = x;
				sec_bat_check_battery_health(battery);
			}
			ret = count;
		}
		break;
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	case FG_MAIN_ASOC:
	case FG_SUB_ASOC:
		break;
#endif
	case AUTH:
		break;
	case CHG_CURRENT_ADC:
		break;
	case WC_ADC:
		break;
	case WC_STATUS:
		break;
	case WC_ENABLE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 0) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = false;
				battery->wc_enable_cnt = 0;
				mutex_unlock(&battery->wclock);
			} else if (x == 1) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = true;
				battery->wc_enable_cnt = 0;
				mutex_unlock(&battery->wclock);
			} else {
				dev_info(battery->dev,
					"%s: WPC ENABLE unknown command\n",
					__func__);
				return -EINVAL;
			}
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			ret = count;
		}
		break;
	case WC_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			char wpc_en_status[2];

			wpc_en_status[0] = WPC_EN_SYSFS;
			if (x == 0) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = false;
				battery->wc_enable_cnt = 0;
				value.intval = 0;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WC_CONTROL, value);

				wpc_en_status[1] = false;
				value.strval= wpc_en_status;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WPC_EN, value);
				pr_info("@DIS_MFC %s: WC CONTROL: Disable\n", __func__);
				mutex_unlock(&battery->wclock);
			} else if (x == 1) {
				mutex_lock(&battery->wclock);
				battery->wc_enable = true;
				battery->wc_enable_cnt = 0;
				value.intval = 1;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WC_CONTROL, value);
				wpc_en_status[1] = true;
				value.strval= wpc_en_status;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WPC_EN, value);

				pr_info("@DIS_MFC %s: WC CONTROL: Enable\n", __func__);
				mutex_unlock(&battery->wclock);
			} else {
				dev_info(battery->dev,
					"%s: WC CONTROL unknown command\n",
					__func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case WC_CONTROL_CNT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->wc_enable_cnt_value = x;
			ret = count;
		}
		break;
	case LED_COVER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: MFC, LED_COVER(%d)\n", __func__, x);
			battery->led_cover = x;
			value.intval = battery->led_cover;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_FILTER_CFG, value);
			ret = count;
		}
		break;
	case HV_CHARGER_STATUS:
		break;
	case HV_WC_CHARGER_STATUS:
		break;
	case HV_CHARGER_SET:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				"%s: HV_CHARGER_SET(%d)\n", __func__, x);
			if (x == 1) {
				battery->wire_status = SEC_BATTERY_CABLE_9V_TA;
				__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			} else {
				battery->wire_status = SEC_BATTERY_CABLE_NONE;
				__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			}
			ret = count;
		}
		break;
	case FACTORY_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->factory_mode = x ? true : false;
			ret = count;
		}
		break;
	case STORE_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_SEC_FACTORY)
			if (x) {
				battery->store_mode = true;
				__pm_stay_awake(battery->parse_mode_dt_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->parse_mode_dt_work, 0);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
				sec_bat_reset_step_charging(battery);
				direct_charging_source_status[0] = SEC_STORE_MODE;
				direct_charging_source_status[1] = SEC_CHARGING_SOURCE_SWITCHING;
				value.strval = direct_charging_source_status;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
#endif
			}
#endif
			ret = count;
		}
		break;
	case UPDATE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			/* update battery info */
			sec_bat_get_battery_info(battery);
			ret = count;
		}
		break;
	case TEST_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->test_mode = x;
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue,
				&battery->monitor_work, 0);
			ret = count;
		}
		break;

	case BATT_EVENT_CALL:
	case BATT_EVENT_2G_CALL:
	case BATT_EVENT_TALK_GSM:
	case BATT_EVENT_3G_CALL:
	case BATT_EVENT_TALK_WCDMA:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if defined(CONFIG_LIMIT_CHARGING_DURING_CALL)
			if (x)
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CALL, SEC_BAT_CURRENT_EVENT_CALL);
			else
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_CALL);
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
#endif
			ret = count;
		}
		break;
	case BATT_EVENT_MUSIC:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_VIDEO:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_BROWSER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_HOTSPOT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_CAMERA:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_CAMCORDER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_DATA_CALL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_WIFI:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_WIBRO:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_LTE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_EVENT_LCD:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !defined(CONFIG_SEC_FACTORY)
			struct timespec64 ts;
			ts = ktime_to_timespec64(ktime_get_boottime());
			if (x) {
				battery->lcd_status = true;
			} else {
				battery->lcd_status = false;
			}
			pr_info("%s : lcd_status (%d)\n", __func__, battery->lcd_status);

			if (battery->wc_tx_enable ||
				(battery->pdata->wpc_vout_ctrl_lcd_on && is_wireless_all_type(battery->cable_type)) ||
				(battery->d2d_auth == D2D_AUTH_SRC)) {
				battery->polling_short = false;
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->monitor_work, 0);
			}
#endif
			ret = count;
		}
		break;
#if defined(CONFIG_ISDB_CHARGING_CONTROL)
	case BATT_EVENT_ISDB:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				"%s: ISDB EVENT %d\n", __func__, x);
			if (x) {
				pr_info("%s: ISDB ON\n", __func__);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_ISDB,
					SEC_BAT_CURRENT_EVENT_ISDB);
				if (is_hv_wireless_type(battery->cable_type)) {
					pr_info("%s: set vout 5.5V with ISDB\n", __func__);
					value.intval = WIRELESS_VOUT_5_5V_STEP; // 5.5V
					psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
					sec_bat_set_charging_current(battery);
				} else if (is_hv_wire_type(battery->cable_type) ||
					(is_pd_wire_type(battery->cable_type) &&
					battery->pd_max_charge_power >= HV_CHARGER_STATUS_STANDARD1 &&
					battery->pdic_info.sink_status.available_pdo_num > 1) ||
					battery->max_charge_power >= HV_CHARGER_STATUS_STANDARD1)
					sec_bat_set_charging_current(battery);
			} else {
				pr_info("%s: ISDB OFF\n", __func__);
				sec_bat_set_current_event(battery, 0,
					SEC_BAT_CURRENT_EVENT_ISDB);
				if (is_hv_wireless_type(battery->cable_type)) {
					pr_info("%s: recover vout 10V with ISDB\n", __func__);
					value.intval = WIRELESS_VOUT_10V; // 10V
					psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
					sec_bat_set_charging_current(battery);
				} else if (is_hv_wire_type(battery->cable_type))
					sec_bat_set_charging_current(battery);
			}
			ret = count;
		}
		break;
#endif
	case BATT_EVENT_GPS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			ret = count;
		}
		break;
	case BATT_HIGH_CURRENT_USB:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			battery->is_hc_usb = x ? true : false;
			value.intval = battery->is_hc_usb;

			pr_info("%s: is_hc_usb (%d)\n", __func__, battery->is_hc_usb);
			ret = count;
		}
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case TEST_CHARGE_CURRENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x >= 0 && x <= 2000) {
				dev_err(battery->dev,
					"%s: BATT_TEST_CHARGE_CURRENT(%d)\n", __func__, x);
				battery->pdata->charging_current[
					SEC_BATTERY_CABLE_USB].input_current_limit = x;
				battery->pdata->charging_current[
					SEC_BATTERY_CABLE_USB].fast_charging_current = x;
				if (x > 500) {
					battery->eng_not_full_status = true;
					battery->pdata->bat_thm_info.check_type =
						SEC_BATTERY_TEMP_CHECK_NONE;
				}
				if (battery->cable_type == SEC_BATTERY_CABLE_USB) {
					value.intval = x;
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
						value);
				}
			}
			ret = count;
		}
		break;
#if defined(CONFIG_STEP_CHARGING)
	case TEST_STEP_CONDITION:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x >= 0 && x <= 100) {
				dev_err(battery->dev,
					"%s: TEST_STEP_CONDITION(%d)\n", __func__, x);
				battery->test_step_condition = x;
			}
			ret = count;
		}
		break;
#endif
#endif
	case SET_STABILITY_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_err(battery->dev,
				"%s: BATT_STABILITY_TEST(%d)\n", __func__, x);
			if (x) {
				battery->stability_test = true;
				battery->eng_not_full_status = true;
			}
			else {
				battery->stability_test = false;
				battery->eng_not_full_status = false;
			}
			ret = count;
		}
		break;
	case BATT_CAPACITY_MAX:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_err(battery->dev,
					"%s: BATT_CAPACITY_MAX(%d), fg_reset(%d)\n", __func__, x, sec_bat_get_fgreset());
			if (!sec_bat_get_fgreset() && !battery->store_mode) {
				value.intval = x;
				psy_do_property(battery->pdata->fuelgauge_name, set,
						POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN, value);

				/* update soc */
				battery->capacity = sec_bat_get_batt_capacity(battery, battery->capacity);
			} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				battery->fg_reset = 1;
#endif
			}
			ret = count;
		}
		break;
	case BATT_REPCAP_1ST:
		if ((sscanf(buf, "%10d\n", &x) == 1) && (battery->pdata->soc_by_repcap_en)) {
			dev_info(battery->dev,
					"%s: BATT_REPCAP(%d), fg_reset(%d)\n", __func__, x, sec_bat_get_fgreset());
			/* Maximum value check should be added in FG driver file */
			if (!sec_bat_get_fgreset() && !battery->store_mode && x >= 0) {
				value.intval = x;
				psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_EXT_PROP_CHARGE_FULL_REPCAP, value);

				/* update soc */
				battery->capacity = sec_bat_get_batt_capacity(battery, battery->capacity);
			} else {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				battery->fg_reset = 1;
#endif
			}
			ret = count;
		}
		break;
	case BATT_INBAT_VOLTAGE:
		break;
	case BATT_INBAT_VOLTAGE_OCV:
		break;
	case BATT_VBYP_VOLTAGE:
		break;
	case CHECK_SUB_CHG:
		break;
	case BATT_INBAT_WIRELESS_CS100:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s send cs100 command\n", __func__);
			value.intval = POWER_SUPPLY_STATUS_FULL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_PROP_STATUS, value);
			ret = count;
		}
		break;
	case HMT_TA_CONNECTED:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if !IS_ENABLED(CONFIG_PDIC_NOTIFIER) && !IS_ENABLED(CONFIG_CCIC_NOTIFIER)
			dev_info(battery->dev,
					"%s: HMT_TA_CONNECTED(%d)\n", __func__, x);
			if (x) {
				value.intval = false;
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL,
						value);
				dev_info(battery->dev,
						"%s: changed to OTG cable detached\n", __func__);

				battery->wire_status = SEC_BATTERY_CABLE_HMT_CONNECTED;
				__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			} else {
				value.intval = true;
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL,
						value);
				dev_info(battery->dev,
						"%s: changed to OTG cable attached\n", __func__);

				battery->wire_status = SEC_BATTERY_CABLE_OTG;
				__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
			}
#endif
			ret = count;
		}
		break;
	case HMT_TA_CHARGE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER) || IS_ENABLED(CONFIG_CCIC_NOTIFIER)
			dev_info(battery->dev,
					"%s: HMT_TA_CHARGE(%d)\n", __func__, x);

			/* do not charge off without cable type, since wdt could be expired */
			if (x) {
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE);
				sec_vote(battery->chgen_vote, VOTER_HMT, false, 0);
			} else if (!x && !is_nocharge_type(battery->cable_type)) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE,
						SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE);
				sec_vote(battery->chgen_vote, VOTER_HMT, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			} else
				dev_info(battery->dev, "%s: Wrong HMT control\n", __func__);

			ret = count;
#else
			dev_info(battery->dev,
					"%s: HMT_TA_CHARGE(%d)\n", __func__, x);
			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
			if (value.intval) {
				dev_info(battery->dev,
					"%s: ignore HMT_TA_CHARGE(%d)\n", __func__, x);
			} else {
				if (x) {
					value.intval = false;
					psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL,
							value);
					dev_info(battery->dev,
						"%s: changed to OTG cable detached\n", __func__);
					battery->wire_status = SEC_BATTERY_CABLE_HMT_CHARGE;
					__pm_stay_awake(battery->cable_ws);
					queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
				} else {
					value.intval = false;
					psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL,
							value);
					dev_info(battery->dev,
							"%s: changed to OTG cable detached\n", __func__);
					battery->wire_status = SEC_BATTERY_CABLE_HMT_CONNECTED;
					__pm_stay_awake(battery->cable_ws);
					queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
				}
			}
			ret = count;
#endif
		}
		break;
#if defined(CONFIG_SEC_FACTORY)
	case AFC_TEST_FG_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_EXT_PROP_AFC_TEST_FG_MODE, value);
			ret = count;
		}
	break;
	case NOZX_CTRL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: NOZX set to %s\n", __func__,
				(!x) ? "Enable" : "Disable");	// 0: NOZX enable, 1: NOZX disable
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_NOZX_CTRL, value);
			ret = count;
		}
		break;
#endif
	case FG_CYCLE:
		break;
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	case FG_SUB_CYCLE:
		break;
#endif
	case FG_FULL_VOLTAGE:
		break;
	case FG_FULLCAPNOM:
		break;
#if defined(CONFIG_BATTERY_AGE_FORECAST_DETACHABLE)
	case BATT_AFTER_MANUFACTURED:
#else
	case BATTERY_CYCLE:
#endif
		if (sscanf(buf, "%10d %10d\n", &x, &y) > 0) {
			dev_info(battery->dev, "%s: %s(%d), BATT_FULL_STATUS(%d)\n", __func__,
				(offset == BATTERY_CYCLE) ? "BATTERY_CYCLE" : "BATTERY_CYCLE(W)", x, y);
			if (x >= 0) {
				int prev_battery_cycle = battery->batt_cycle;
				battery->batt_cycle = x;
				if(y >= 0)
					battery->batt_full_status_usage = y;
				battery->cisd.data[CISD_DATA_CYCLE] = x;
				if (prev_battery_cycle < 0) {
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
					if (battery->pdata->dual_battery_individual_fv)
						sec_bat_aging_check_dual_battery_individual_fv(battery);
					else
						sec_bat_aging_check(battery);
#else
					sec_bat_aging_check(battery);
#endif
				}
				sec_bat_check_battery_health(battery);
			}
			ret = count;
		}
		break;
	case BATTERY_CYCLE_TEST:
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
		if (battery->pdata->dual_battery_individual_fv)
			sec_bat_aging_check_dual_battery_individual_fv(battery);
		else
			sec_bat_aging_check(battery);
#else
		sec_bat_aging_check(battery);
#endif
	break;
	case BATT_WPC_TEMP:
		break;
	case BATT_WPC_TEMP_ADC:
		break;
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	case BATT_WIRELESS_FIRMWARE_UPDATE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (sec_bat_check_boost_mfc_condition(battery, x)) {
				if (x == SEC_WIRELESS_FW_UPDATE_SDCARD_MODE) {
					pr_info("%s fw mode is SDCARD\n", __func__);
					sec_bat_fw_update(battery, x);
				} else if (x == SEC_WIRELESS_FW_UPDATE_BUILTIN_MODE) {
					pr_info("%s fw mode is BUILD IN\n", __func__);
					sec_bat_fw_update(battery, x);
				} else if (x == SEC_WIRELESS_FW_UPDATE_SPU_MODE) {
					pr_info("%s fw mode is SPU\n", __func__);
					sec_bat_fw_update(battery, x);
				} else if (x == SEC_WIRELESS_FW_UPDATE_SPU_MODE) {
					pr_info("%s fw mode is SPU VERIFY\n", __func__);
					sec_bat_fw_update(battery, x);
				} else {
					dev_info(battery->dev, "%s: wireless firmware unknown command\n", __func__);
					return -EINVAL;
				}
			} else
				pr_info("%s: skip fw update at this time\n", __func__);
			ret = count;
		}
		break;
	case OTP_FIRMWARE_RESULT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 2) {
				value.intval = x;
				pr_info("%s RX firmware update ready!\n", __func__);
				psy_do_property(battery->pdata->wireless_charger_name, set,
								POWER_SUPPLY_PROP_MANUFACTURER, value);
			} else {
				dev_info(battery->dev, "%s: firmware unknown command\n", __func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case WC_IC_GRADE:
		break;
	case WC_IC_CHIP_ID:
		break;
	case OTP_FIRMWARE_VER_BIN:
		break;
	case OTP_FIRMWARE_VER:
		break;
#endif
	case WC_PHM_CTRL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s : phm ctrl %d\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_RX_PHM, value);
			ret = count;
		}
		break;
	case WC_VOUT:
		break;
	case WC_VRECT:
		break;
	case WC_RX_CONNECTED:
		break;
	case WC_RX_CONNECTED_DEV:
		break;
	case WC_TX_MFC_VIN_FROM_UNO:
		break;
	case WC_TX_MFC_IIN_FROM_UNO:
		break;
#if defined(CONFIG_WIRELESS_TX_MODE)
	case WC_TX_AVG_CURR:
		break;
	case WC_TX_TOTAL_PWR:
		break;
#endif
	case WC_TX_STOP_CAPACITY:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s : tx stop capacity (%d)%%\n", __func__, x);
			if (x >= 0 && x <= 100)
				battery->pdata->tx_stop_capacity = x;
			ret = count;
		}
		break;
	case WC_TX_TIMER_EN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s : tx receiver detecting timer (%d)%%\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_TIMER_ON, value);
			ret = count;
		}
		break;
	case WC_TX_EN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (battery->mfc_fw_update) {
				pr_info("@Tx_Mode %s : skip Tx by mfc_fw_update\n", __func__);
				return count;
			}

#if defined(CONFIG_WIRELESS_TX_MODE)
			/* x value is written by ONEUI 2.5 PMS when tx_event is changed */
			if (x && is_wireless_all_type(battery->cable_type)) {
				pr_info("@Tx_Mode %s : Can't enable Tx mode during wireless charging\n", __func__);
				return count;
			} else {
				pr_info("@Tx_Mode %s: Set TX Enable (%d)\n", __func__, x);
				sec_wireless_set_tx_enable(battery, x);
				if (!x) {
					/* clear tx all event */
					sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
				}
				if (x)
					battery->cisd.tx_data[TX_ON]++;
			}
#endif
			ret = count;
		}
		break;
	case WC_TX_VOUT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("@Tx_Mode %s: Set TX Vout (%d)\n", __func__, x);
			battery->wc_tx_vout = value.intval = x;
			if (battery->wc_tx_enable) {
				pr_info("@Tx_Mode %s: set TX Vout (%d)\n", __func__, value.intval);
				psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_TX_VOUT, value);
			} else {
				pr_info("@Tx_Mode %s: TX mode turned off now\n", __func__);
			}
			ret = count;
		}
		break;
	case BATT_HV_WIRELESS_STATUS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 1 && is_hv_wireless_type(battery->cable_type)) {
#ifdef CONFIG_SEC_FACTORY
				int input_current, charging_current;
				pr_info("%s change cable type HV WIRELESS -> WIRELESS\n", __func__);
				battery->wc_status = battery->cable_type = SEC_BATTERY_CABLE_WIRELESS;
				input_current =  battery->pdata->charging_current[battery->cable_type].input_current_limit;
				charging_current = battery->pdata->charging_current[battery->cable_type].fast_charging_current;
				sec_vote(battery->fcc_vote, VOTER_SLEEP_MODE, true, charging_current);
				sec_vote(battery->input_vote, VOTER_SLEEP_MODE, true, input_current);
#endif
				pr_info("%s HV_WIRELESS_STATUS set to 1. Vout set to 5V.\n", __func__);
				value.intval = WIRELESS_VOUT_5V;
				__pm_stay_awake(battery->cable_ws);
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
				__pm_relax(battery->cable_ws);
			} else if (x == 3 && is_hv_wireless_type(battery->cable_type)) {
				pr_info("%s HV_WIRELESS_STATUS set to 3. Vout set to 10V.\n", __func__);
				value.intval = WIRELESS_VOUT_10V;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
			} else {
				dev_info(battery->dev, "%s: HV_WIRELESS_STATUS unknown command\n", __func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case BATT_HV_WIRELESS_PAD_CTRL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_err("%s: x : %d\n", __func__, x);

			if (x == 1) {
				pr_info("%s: hv wireless charging is disabled\n", __func__);
				battery->sleep_mode = true;
				value.intval = battery->sleep_mode;
				psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_SLEEP_MODE, value);

				if (is_hv_wireless_type(battery->cable_type))
					sec_vote(battery->input_vote, VOTER_SLEEP_MODE, true, battery->pdata->sleep_mode_limit_current);
			} else if (x == 2) {
				pr_info("%s: hv wireless charging is enabled\n", __func__);
				battery->auto_mode = false;
				battery->sleep_mode = false;
				value.intval = battery->sleep_mode;
				psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_SLEEP_MODE, value);

				value.intval = WIRELESS_SLEEP_MODE_DISABLE;
				psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);

				sec_vote(battery->input_vote, VOTER_SLEEP_MODE, false, 0);
			} else if (x == 3) {
				pr_info("%s led off\n", __func__);
				value.intval = WIRELESS_PAD_LED_OFF;
				psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);
			} else if (x == 4) {
				pr_info("%s led on\n", __func__);
				value.intval = WIRELESS_PAD_LED_ON;
				psy_do_property(battery->pdata->wireless_charger_name, set,
							POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);
			} else if (((x == 5) || (x == 6)) && is_wireless_all_type(battery->cable_type)) {
				if (battery->pdata->wpc_vout_ctrl_lcd_on) {
					battery->wpc_vout_ctrl_mode = (x == 5) ? true : false;
					pr_info("%s: %s display flicker wa\n",
						__func__, (x == 5) ? "enable" : "disable");
				}
				battery->polling_short = false;
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->monitor_work, 0);
			} else {
				dev_info(battery->dev, "%s: BATT_HV_WIRELESS_PAD_CTRL unknown command\n", __func__);
				return -EINVAL;
			}
			ret = count;
		}
		break;
	case WC_TX_ID:
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_TUNE_FLOAT_VOLTAGE:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s float voltage = %d mV",__func__, x);
		sec_vote(battery->fv_vote, VOTER_CABLE, true, x);
		break;
	case BATT_TUNE_INPUT_CHARGE_CURRENT:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s input charge current = %d mA",__func__, x);
		if (x >= 0 && x <= 4000 ){
			battery->test_max_current = true;
			for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
				if (i != SEC_BATTERY_CABLE_USB)
					battery->pdata->charging_current[i].input_current_limit = x;
				pr_info("%s [%d] = %d mA",__func__, i, battery->pdata->charging_current[i].input_current_limit);
			}

			if (battery->cable_type != SEC_BATTERY_CABLE_USB) {
				value.intval = x;
				psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);
			}
		}
		break;
	case BATT_TUNE_FAST_CHARGE_CURRENT:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s fast charge current = %d mA",__func__, x);
		if (x >= 0 && x <= 4000 ){
			battery->test_charge_current = true;
			for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
				if (i != SEC_BATTERY_CABLE_USB)
					battery->pdata->charging_current[i].fast_charging_current = x;
				pr_info("%s [%d] = %d mA",__func__, i, battery->pdata->charging_current[i].fast_charging_current);
			}

			if (battery->cable_type != SEC_BATTERY_CABLE_USB) {
				value.intval = x;
					psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, value);
			}
		}
		break;
	case BATT_TUNE_WIRELESS_VOUT_CURRENT:
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		{
			int vout, input_current, offset;

			sscanf(buf, "%10d %10d\n", &offset, &input_current);
			switch (offset) {
			case 5500:
				vout = WIRELESS_VOUT_5V;
				break;
			case 9000:
				vout = WIRELESS_VOUT_9V;
				break;
			case 10000:
				vout = WIRELESS_VOUT_10V;
				break;
			case 11000:
				vout = WIRELESS_VOUT_11V;
				break;
			case 12000:
				vout = WIRELESS_VOUT_12V;
				break;
			case 12500:
				vout = WIRELESS_VOUT_12_5V;
				break;
			default:
				pr_info("%s vout(%d) you entered is not supported\n", __func__, offset);
				vout = 0;
				break;
			}

			pr_info("%s vout(%d, %d) input_current(%d)",__func__, offset, vout, input_current);
			battery->wc20_vout = offset;
			battery->wc20_rx_power = offset * input_current;
			__pm_stay_awake(battery->wc20_current_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->wc20_current_work,
				msecs_to_jiffies(0));
		}
#endif
		break;
	case BATT_TUNE_UI_TERM_CURRENT_1ST:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s ui term current = %d mA",__func__, x);

		if (x > 0 && x < 1000 ){
			battery->pdata->full_check_current_1st = x;
		}
		break;
	case BATT_TUNE_UI_TERM_CURRENT_2ND:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s ui term current = %d mA",__func__, x);

		if (x > 0 && x < 1000 ){
			battery->pdata->full_check_current_2nd = x;
		}
		break;
	case BATT_TUNE_CHG_TEMP_HIGH:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s chg_high_temp  = %d ",__func__, x);
		if (x < 1000 && x >= -200)
			battery->pdata->chg_high_temp = x;
		break;
	case BATT_TUNE_CHG_TEMP_REC:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s chg_high_temp_recovery	= %d ",__func__, x);
		if (x < 1000 && x >= -200)
			battery->pdata->chg_high_temp_recovery = x;
		break;
	case BATT_TUNE_CHG_LIMIT_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s chg_charging_limit_current	= %d ",__func__, x);
		if (x < 3000 && x > 0)
		{
			battery->pdata->chg_charging_limit_current = x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_ERR].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_UNKNOWN].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_TA].input_current_limit= x;
		}
		break;
	case BATT_TUNE_LRP_TEMP_HIGH_LCDON:
	{
		int lrp_m = 0, lrp_t[4] = {0, };
		int lrp_pt = LRP_NORMAL;

		if (sscanf(buf, "%10d %10d %10d %10d %10d\n",
				&lrp_m, &lrp_t[0], &lrp_t[1], &lrp_t[2], &lrp_t[3]) == 5) {
			pr_info("%s : lrp_high_temp_lcd on lrp_m: %c, temp: %d %d %d %d\n",
				__func__, lrp_m, lrp_t[0], lrp_t[1], lrp_t[2], lrp_t[3]);

			if (lrp_m == 45)
				lrp_pt = LRP_45W;
			else if (lrp_m == 25)
				lrp_pt = LRP_25W;

			if (x < 1000 && x >= -200) {
				battery->pdata->lrp_temp[lrp_pt].trig[ST2][LCD_ON] = lrp_t[0];
				battery->pdata->lrp_temp[lrp_pt].recov[ST2][LCD_ON] = lrp_t[1];
				battery->pdata->lrp_temp[lrp_pt].trig[ST1][LCD_ON] = lrp_t[2];
				battery->pdata->lrp_temp[lrp_pt].recov[ST1][LCD_ON] = lrp_t[3];
			}
		}
		break;
	}
	case BATT_TUNE_LRP_TEMP_HIGH_LCDOFF:
	{
		int lrp_m = 0, lrp_t[4] = {0, };
		int lrp_pt = LRP_NORMAL;

		if (sscanf(buf, "%10d %10d %10d %10d %10d\n",
				&lrp_m, &lrp_t[0], &lrp_t[1], &lrp_t[2], &lrp_t[3]) == 5) {
			pr_info("%s : lrp_high_temp_lcd off lrp_m: %dW, temp: %d %d %d %d\n",
				__func__, lrp_m, lrp_t[0], lrp_t[1], lrp_t[2], lrp_t[3]);

			if (lrp_m == 45)
				lrp_pt = LRP_45W;
			else if (lrp_m == 25)
				lrp_pt = LRP_25W;

			if (x < 1000 && x >= -200) {
				battery->pdata->lrp_temp[lrp_pt].trig[ST2][LCD_OFF] = lrp_t[0];
				battery->pdata->lrp_temp[lrp_pt].recov[ST2][LCD_OFF] = lrp_t[1];
				battery->pdata->lrp_temp[lrp_pt].trig[ST1][LCD_OFF] = lrp_t[2];
				battery->pdata->lrp_temp[lrp_pt].recov[ST1][LCD_OFF] = lrp_t[3];
			}
		}
		break;
	}
	case BATT_TUNE_COIL_TEMP_HIGH:
		break;
	case BATT_TUNE_COIL_TEMP_REC:
		break;
	case BATT_TUNE_COIL_LIMIT_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s wpc_input_limit_current	= %d ",__func__, x);
		if (x < 3000 && x > 0)
		{
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_ERR].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_UNKNOWN].input_current_limit= x;
			battery->pdata->charging_current[SEC_BATTERY_CABLE_9V_TA].input_current_limit= x;
		}
		break;
	case BATT_TUNE_WPC_TEMP_HIGH:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s wpc_high_temp = %d ",__func__, x);
		battery->pdata->wpc_high_temp = x;
		break;
	case BATT_TUNE_WPC_TEMP_HIGH_REC:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s wpc_high_temp_recovery = %d ",__func__, x);
		battery->pdata->wpc_high_temp_recovery = x;
		break;
	case BATT_TUNE_DCHG_TEMP_HIGH:
	{
		int dchg_t[4] = {0, };

		sscanf(buf, "%10d %10d %10d %10d\n",
			&dchg_t[0], &dchg_t[1], &dchg_t[2], &dchg_t[3]);
		pr_info("%s dchg_high_temp = %d %d %d %d", __func__,
			dchg_t[0], dchg_t[1], dchg_t[2], dchg_t[3]);
		for (i = 0; i < 4; i++) {
			battery->pdata->dchg_high_temp[i] = dchg_t[i];
		}
		break;
	}
	case BATT_TUNE_DCHG_TEMP_HIGH_REC:
	{
		int dchg_t[4] = {0, };

		sscanf(buf, "%10d %10d %10d %10d\n",
			&dchg_t[0], &dchg_t[1], &dchg_t[2], &dchg_t[3]);
		pr_info("%s dchg_high_temp_recovery = %d %d %d %d", __func__,
			dchg_t[0], dchg_t[1], dchg_t[2], dchg_t[3]);
		for (i = 0; i < 4; i++) {
			battery->pdata->dchg_high_temp_recovery[i] = dchg_t[i];
		}
		break;
	}
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH:
	{
		int dchg_t[4] = {0, };

		sscanf(buf, "%10d %10d %10d %10d\n",
			&dchg_t[0], &dchg_t[1], &dchg_t[2], &dchg_t[3]);
		pr_info("%s dchg_high_batt_temp = %d %d %d %d", __func__,
			dchg_t[0], dchg_t[1], dchg_t[2], dchg_t[3]);
		for (i = 0; i < 4; i++) {
			battery->pdata->dchg_high_batt_temp[i] = dchg_t[i];
		}
		break;
	}
	case BATT_TUNE_DCHG_BATT_TEMP_HIGH_REC:
	{
		int dchg_t[4] = {0, };

		sscanf(buf, "%10d %10d %10d %10d\n",
			&dchg_t[0], &dchg_t[1], &dchg_t[2], &dchg_t[3]);
		pr_info("%s dchg_high_batt_temp_recovery = %d %d %d %d", __func__,
			dchg_t[0], dchg_t[1], dchg_t[2], dchg_t[3]);
		for (i = 0; i < 4; i++) {
			battery->pdata->dchg_high_batt_temp_recovery[i] = dchg_t[i];
		}
		break;
	}
	case BATT_TUNE_DCHG_LIMIT_INPUT_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_input_limit_current = %d ",__func__, x);
		battery->pdata->dchg_input_limit_current = x;
		break;
	case BATT_TUNE_DCHG_LIMIT_CHG_CUR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s dchg_charging_limit_current = %d ",__func__, x);
		battery->pdata->dchg_charging_limit_current = x;
		break;
#if defined(CONFIG_WIRELESS_TX_MODE)
	case BATT_TUNE_TX_MFC_IOUT_GEAR:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s tx_mfc_iout_gear = %d", __func__, x);
		battery->pdata->tx_mfc_iout_gear = x;
		break;
	case BATT_TUNE_TX_MFC_IOUT_PHONE:
		sscanf(buf, "%10d\n", &x);
		pr_info("%s tx_mfc_iout_phone = %d", __func__, x);
		battery->pdata->tx_mfc_iout_phone = x;
		break;
#endif
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	case BATT_UPDATE_DATA:
		if (!battery->data_path && (count * sizeof(char)) < 256) {
			battery->data_path = kzalloc((count * sizeof(char) + 1), GFP_KERNEL);
			if (battery->data_path) {
				sscanf(buf, "%s\n", battery->data_path);
				cancel_delayed_work(&battery->batt_data_work);
				__pm_stay_awake(battery->batt_data_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->batt_data_work, msecs_to_jiffies(100));
			} else {
				pr_info("%s: failed to alloc data_path buffer\n", __func__);
			}
		}
		ret = count;
		break;
#endif
	case BATT_MISC_EVENT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: PMS sevice hiccup read done : %d ", __func__, x);
			if (battery->misc_event &
					(BATT_MISC_EVENT_WATER_HICCUP_TYPE |
					 BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
				if (!battery->hiccup_status) {
					sec_bat_set_misc_event(battery,
						0, (BATT_MISC_EVENT_WATER_HICCUP_TYPE |
							BATT_MISC_EVENT_TEMP_HICCUP_TYPE));
				} else {
					battery->hiccup_clear = true;
					pr_info("%s : Hiccup event doesn't clear. Hiccup clear bit set (%d)\n",
							__func__, battery->hiccup_clear);
				}
			}
		}
		ret = count;
		break;
	case BATT_EXT_DEV_CHG:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: Connect Ext Device : %d ",__func__, x);

			switch (x) {
				case EXT_DEV_NONE:
					battery->wire_status = SEC_BATTERY_CABLE_NONE;
					value.intval = 0;
					break;
				case EXT_DEV_GAMEPAD_CHG:
					battery->wire_status = SEC_BATTERY_CABLE_TA;
					value.intval = 0;
					break;
				case EXT_DEV_GAMEPAD_OTG:
					battery->wire_status = SEC_BATTERY_CABLE_OTG;
					value.intval = 1;
					break;
				default:
					break;
			}

			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL,
					value);

			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			ret = count;
		}
		break;
	case BATT_WDT_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: Charger WDT Set : %d\n", __func__, x);
			battery->wdt_kick_disable = x;
#ifdef CONFIG_IFPMIC_LIMITER
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_WDT_KICK_TEST, value);
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL, value);
#endif
		}
		ret = count;
		break;
	case MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_MULTI_CHARGER_MODE, value);
			ret = count;
		}
		break;
	case CHECK_PS_READY:
		break;
	case BATT_CHIP_ID:
		break;
	case ERROR_CAUSE:
		break;
	case CISD_FULLCAPREP_MAX:
		break;
	case CISD_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			int temp_data[CISD_DATA_MAX_PER_DAY] = {0,};

			sscanf(buf, "%10d\n", &temp_data[0]);

			if (temp_data[CISD_DATA_RESET_ALG] > 0) {
				if (sscanf(buf, "%10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d, %10d\n",
					&temp_data[0], &temp_data[1], &temp_data[2],
					&temp_data[3], &temp_data[4], &temp_data[5],
					&temp_data[6], &temp_data[7], &temp_data[8],
					&temp_data[9], &temp_data[10], &temp_data[11],
					&temp_data[12], &temp_data[13], &temp_data[14],
					&temp_data[15], &temp_data[16], &temp_data[17],
					&temp_data[18], &temp_data[19], &temp_data[20],
					&temp_data[21], &temp_data[22], &temp_data[23],
					&temp_data[24], &temp_data[25], &temp_data[26],
					&temp_data[27], &temp_data[28], &temp_data[29],
					&temp_data[30], &temp_data[31], &temp_data[32],
					&temp_data[33], &temp_data[34], &temp_data[35],
					&temp_data[36], &temp_data[37], &temp_data[38],
					&temp_data[39], &temp_data[40], &temp_data[41],
					&temp_data[42], &temp_data[43], &temp_data[44],
					&temp_data[45], &temp_data[46], &temp_data[47],
					&temp_data[48], &temp_data[49], &temp_data[50],
					&temp_data[51], &temp_data[52], &temp_data[53],
					&temp_data[54], &temp_data[55], &temp_data[56],
					&temp_data[57], &temp_data[58], &temp_data[59],
					&temp_data[60], &temp_data[61], &temp_data[62],
					&temp_data[63], &temp_data[64], &temp_data[65],
					&temp_data[66], &temp_data[67], &temp_data[68],
					&temp_data[69], &temp_data[70], &temp_data[71],
					&temp_data[72], &temp_data[73], &temp_data[74],
					&temp_data[75], &temp_data[76]) <= CISD_DATA_MAX_PER_DAY) {
					for (i = 0; i < CISD_DATA_MAX_PER_DAY; i++)
						pcisd->data[i] = 0;

					pcisd->data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;
					pcisd->data[CISD_DATA_FULL_COUNT] = temp_data[0];
					pcisd->data[CISD_DATA_CAP_MAX] = temp_data[1];
					pcisd->data[CISD_DATA_CAP_MIN] = temp_data[2];
					pcisd->data[CISD_DATA_VALERT_COUNT] = temp_data[16];
					pcisd->data[CISD_DATA_CYCLE] = temp_data[17];
					pcisd->data[CISD_DATA_WIRE_COUNT] = temp_data[18];
					pcisd->data[CISD_DATA_WIRELESS_COUNT] = temp_data[19];
					pcisd->data[CISD_DATA_HIGH_TEMP_SWELLING] = temp_data[20];
					pcisd->data[CISD_DATA_LOW_TEMP_SWELLING] = temp_data[21];
					pcisd->data[CISD_DATA_WC_HIGH_TEMP_SWELLING] = temp_data[22];
					pcisd->data[CISD_DATA_AICL_COUNT] = temp_data[26];
					pcisd->data[CISD_DATA_BATT_TEMP_MAX] = temp_data[27];
					pcisd->data[CISD_DATA_BATT_TEMP_MIN] = temp_data[28];
					pcisd->data[CISD_DATA_CHG_TEMP_MAX] = temp_data[29];
					pcisd->data[CISD_DATA_CHG_TEMP_MIN] = temp_data[30];
					pcisd->data[CISD_DATA_WPC_TEMP_MAX] = temp_data[31];
					pcisd->data[CISD_DATA_WPC_TEMP_MIN] = temp_data[32];
					pcisd->data[CISD_DATA_UNSAFETY_VOLTAGE] = temp_data[33];
					pcisd->data[CISD_DATA_UNSAFETY_TEMPERATURE] = temp_data[34];
					pcisd->data[CISD_DATA_SAFETY_TIMER] = temp_data[35];
					pcisd->data[CISD_DATA_VSYS_OVP] = temp_data[36];
					pcisd->data[CISD_DATA_VBAT_OVP] = temp_data[37];
				}
			} else {
				const char *p = buf;

				pr_info("%s: %s\n", __func__, buf);
				for (i = CISD_DATA_RESET_ALG; i < CISD_DATA_MAX_PER_DAY; i++) {
					if (sscanf(p, "%10d%n", &pcisd->data[i], &x) > 0) {
						p += (size_t)x;
						if (pcisd->data[CISD_DATA_ALG_INDEX] != battery->pdata->cisd_alg_index) {
							pr_info("%s: ALG_INDEX is changed %d -> %d\n", __func__,
								pcisd->data[CISD_DATA_ALG_INDEX], battery->pdata->cisd_alg_index);
							temp_data[CISD_DATA_RESET_ALG] = -1;
							break;
						}
					} else {
						pr_info("%s: NO DATA (cisd_data)\n", __func__);
						temp_data[CISD_DATA_RESET_ALG] = -1;
						break;
					}
				}

				pr_info("%s: %s cisd data\n", __func__,
					((temp_data[CISD_DATA_RESET_ALG] < 0 || battery->fg_reset) ?
					"init" : "update"));

				if (temp_data[CISD_DATA_RESET_ALG] < 0 || battery->fg_reset) {
					/* initialize data */
					for (i = CISD_DATA_RESET_ALG; i < CISD_DATA_MAX_PER_DAY; i++)
						pcisd->data[i] = 0;

					battery->fg_reset = 0;

					pcisd->data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;

					pcisd->data[CISD_DATA_FULL_COUNT] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CAP_MIN] = 0xFFFF;

					pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = 0xFFFF;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE_PER_DAY] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_ALONE] = 0;

					/* initialize pad data */
					init_cisd_pad_data(&battery->cisd);

					/* initialize power data */
					init_cisd_power_data(&battery->cisd);

					/* initialize pd data */
					init_cisd_pd_data(&battery->cisd);

					/* initialize dcerr data */
					init_cisd_dcerr_data(&battery->cisd);
				}
			}
			ret = count;
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
		break;
	case CISD_DATA_JSON:
		{
			char tc;
			struct cisd *pcisd = &battery->cisd;

			if (sscanf(buf, "%1c\n", &tc) == 1) {
				if (tc == 'c') {
					for (i = 0; i < CISD_DATA_MAX; i++)
						pcisd->data[i] = 0;

					pcisd->data[CISD_DATA_FULL_COUNT] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CAP_MIN] = 0xFFFF;

					pcisd->data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
					pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN] = 1000;

					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
					pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
					pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;

					pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = 0xFFFF;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_RAPID_CHANGE_PER_DAY] = 0;
					pcisd->data[CISD_DATA_USB_OVERHEAT_ALONE] = 0;
				}
			}
			ret = count;
		}
		break;
	case CISD_DATA_D_JSON:
		break;
	case CISD_WIRE_COUNT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			struct cisd *pcisd = &battery->cisd;
			pr_info("%s: Wire Count : %d\n", __func__, x);
			pcisd->data[CISD_DATA_WIRE_COUNT] = x;
			pcisd->data[CISD_DATA_WIRE_COUNT_PER_DAY]++;
		}
		ret = count;
		break;
	case CISD_WC_DATA:
		set_cisd_pad_data(battery, buf);
		ret = count;
		break;
	case CISD_WC_DATA_JSON:
		break;
	case CISD_POWER_DATA:
		set_cisd_power_data(battery, buf);
		ret = count;
		break;
	case CISD_POWER_DATA_JSON:
		break;
	case CISD_PD_DATA:
		set_cisd_pd_data(battery, buf);
		ret = count;
		break;
	case CISD_PD_DATA_JSON:
		break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case CISD_DCERR_DATA:
		set_cisd_dcerr_data(battery, buf);
		ret = count;
		break;
	case CISD_DCERR_DATA_JSON:
		break;
#endif
	case CISD_CABLE_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = CISD_CABLE_TA; i < CISD_CABLE_TYPE_MAX; i++) {
				if (sscanf(p, "%10d%n", &pcisd->cable_data[i], &x) > 0) {
					p += (size_t)x;
				} else {
					pr_info("%s: NO DATA (CISD_CABLE_TYPE)\n", __func__);
					pcisd->cable_data[i] = 0;
					break;
				}
			}
		}
		ret = count;
		break;

	case CISD_CABLE_DATA_JSON:
		break;
	case CISD_TX_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = TX_ON; i < TX_DATA_MAX; i++) {
				if (sscanf(p, "%10d%n", &pcisd->tx_data[i], &x) > 0) {
					p += (size_t)x;
				} else {
					pr_info("%s: NO DATA (CISD_TX_DATA)\n", __func__);
					pcisd->tx_data[i] = 0;
					break;
				}
			}
		}
		ret = count;
		break;
	case CISD_TX_DATA_JSON:
		break;
	case CISD_EVENT_DATA:
		{
			struct cisd *pcisd = &battery->cisd;
			const char *p = buf;

			pr_info("%s: %s\n", __func__, buf);
			for (i = EVENT_DC_ERR; i < EVENT_DATA_MAX; i++) {
				if (sscanf(p, "%10d%n", &pcisd->event_data[i], &x) > 0) {
					p += (size_t)x;
				} else {
					pr_info("%s: NO DATA (CISD_EVENT_DATA)\n", __func__);
					pcisd->event_data[i] = 0;
					break;
				}
			}
		}
		ret = count;
		break;
	case CISD_EVENT_DATA_JSON:
		break;
	case PREV_BATTERY_DATA:
		if (sscanf(buf, "%10d, %10d, %10d, %10d\n",
				&battery->prev_volt, &battery->prev_temp,
				&battery->prev_jig_on, &battery->prev_chg_on) >= 4) {
			pr_info("%s: prev voltage : %d, prev_temp : %d, prev_jig_on : %d, prev_chg_on : %d\n",
				__func__, battery->prev_volt, battery->prev_temp,
				battery->prev_jig_on, battery->prev_chg_on);

			if (battery->prev_volt >= 3700 && battery->prev_temp >= 150 &&
					!battery->prev_jig_on && battery->fg_reset)
				pr_info("%s: Battery have been Removed\n", __func__);

			ret = count;
		}
		battery->enable_update_data = 1;
		break;
	case PREV_BATTERY_INFO:
		break;
	case SAFETY_TIMER_SET:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x) {
				battery->safety_timer_set = true;
			} else {
				battery->safety_timer_set = false;
			}
			ret = count;
		}
		break;
	case BATT_SWELLING_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x) {
				pr_info("%s : 15TEST START!! SWELLING MODE DISABLE\n", __func__);
				battery->skip_swelling = true;
			} else {
				pr_info("%s : 15TEST END!! SWELLING MODE END\n", __func__);
				battery->skip_swelling = false;
			}
			ret = count;
		}
		break;
	case BATT_BATTERY_ID:
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	case BATT_SUB_BATTERY_ID:
#endif
		break;
	case BATT_TEMP_CONTROL_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x)
				sec_bat_set_temp_control_test(battery, true);
			else
				sec_bat_set_temp_control_test(battery, false);

			ret = count;
		}
		break;
	case SAFETY_TIMER_INFO:
		break;
	case BATT_SHIPMODE_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s ship mode test %d\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_SHIPMODE_TEST, value);
			ret = count;
		}
		break;
	case BATT_MISC_TEST:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s batt_misc_test %d\n", __func__, x);
			switch (x) {
			case MISC_TEST_RESET:
				pr_info("%s RESET MISC_TEST command\n", __func__);
				battery->display_test = false;
				battery->store_mode = false;
				battery->skip_swelling = false;
				battery->pdata->bat_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				battery->pdata->usb_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				battery->pdata->chg_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				battery->pdata->wpc_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				battery->pdata->sub_bat_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				battery->pdata->blk_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
				battery->pdata->dchg_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
#endif
				break;
			case MISC_TEST_DISPLAY:
				pr_info("%s START DISPLAY_TEST command\n", __func__);
				battery->display_test = true;	/* block for display test */
				battery->store_mode = true;	/* enter store mode for prevent 100% full charge */
				battery->skip_swelling = true;	/* restore thermal_zone to NORMAL */
				battery->pdata->bat_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				battery->pdata->usb_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				battery->pdata->chg_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				battery->pdata->wpc_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				battery->pdata->sub_bat_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				battery->pdata->blk_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
				battery->pdata->dchg_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
#endif
				break;
			case MISC_TEST_EPT_UNKNOWN:
#if defined(CONFIG_SEC_FACTORY) && IS_ENABLED(CONFIG_WIRELESS_CHARGING)
				pr_info("%s START EPT_UNKNOWN command\n", __func__);
				value.intval = EPT_UNKNOWN;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WC_EPT_UNKNOWN, value);
#else
				pr_info("%s not support EPT_UNKNOWN command\n", __func__);
#endif
				break;
			case MISC_TEST_TRACE_VTRACK:
#if defined(CONFIG_SEC_FACTORY) && IS_ENABLED(CONFIG_IFPMIC_LIMITER)
				pr_info("%s START TRACE VTRACK command\n", __func__);
				value.intval = 1;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_TRACE_VTRACK, value);
#else
				pr_info("%s not support TRACE VTRACK command\n", __func__);
#endif
				break;
			case MISC_TEST_DC_ERR:
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
				pr_info("%s: START : DC_ERR command\n", __func__);
				value.intval = 1;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_DC_ERR_TEST, value);
#else
				pr_info("%s not support DC_ERR command\n", __func__);
#endif
				break;
			case MISC_TEST_MAX:
			default:
				pr_info("%s Wrong MISC_TEST command\n", __func__);
				break;
			}
			ret = count;
		}
		break;
	case BATT_TEMP_TEST:
	{
		char tc;
		if (sscanf(buf, "%c %10d\n", &tc, &x) == 2) {
			pr_info("%s : temperature t: %c, temp: %d\n", __func__, tc, x);
			if (tc == 'u') {
				if (x > 900)
					battery->pdata->usb_thm_info.check_type = 0;
				else
					battery->pdata->usb_thm_info.test = x;
			} else if (tc == 'w') {
				if (x > 900)
					battery->pdata->wpc_thm_info.check_type = 0;
				else
					battery->pdata->wpc_thm_info.test = x;
			} else if (tc == 'b') {
				if (x > 900)
					battery->pdata->bat_thm_info.check_type = 0;
				else
					battery->pdata->bat_thm_info.test = x;
			} else if (tc == 'c') {
				if (x > 900)
					battery->pdata->chg_thm_info.check_type = 0;
				else
					battery->pdata->chg_thm_info.test = x;
			} else if (tc == 's') {
				if (x > 900)
					battery->pdata->sub_bat_thm_info.check_type = 0;
				else
					battery->pdata->sub_bat_thm_info.test = x;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
			} else if (tc == 'd') {
				if (x > 900)
					battery->pdata->dchg_thm_info.check_type = 0;
				else
					battery->pdata->dchg_thm_info.test = x;
#endif
			} else if (tc == 'k') {
				battery->pdata->blk_thm_info.test = x;
			} else if (tc == 'r') {
				battery->lrp_test = x;
				battery->lrp = x;
			}
			ret = count;
		}
		break;
	}
	case BATT_VOLT_TEST:
	{
		char tc;

		if (sscanf(buf, "%c %10d\n", &tc, &x) == 2) {
			if (tc == 'm') {
				value.intval = x;
				psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_EXT_PROP_VOLTAGE_NOW_TEST, value);
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
			} else if (tc == 's') {
				value.intval = x;
				psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_EXT_PROP_SUB_VOLTAGE_NOW_TEST, value);
#endif
			}
			ret = count;
		}
		break;
	}
	case BATT_CURRENT_EVENT:
		break;
	case BATT_JIG_GPIO:
		break;
	case CC_INFO:
		break;
#if defined(CONFIG_WIRELESS_AUTH)
	case WC_AUTH_ADT_SENT:
		break;
#endif
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	case BATT_MAIN_VOLTAGE:
	case BATT_SUB_VOLTAGE:
	case BATT_MAIN_VCELL:
	case BATT_SUB_VCELL:
	case BATT_MAIN_CURRENT_MA:
	case BATT_SUB_CURRENT_MA:
	case BATT_MAIN_CON_DET:
	case BATT_SUB_CON_DET:
	case BATT_MAIN_VCHG:
	case BATT_SUB_VCHG:
	case BATT_MAIN_CHG_VOLTAGE:
	case BATT_SUB_CHG_VOLTAGE:
		break;
#if IS_ENABLED(CONFIG_LIMITER_S2ASL01)
	case BATT_MAIN_ENB: /* Can control This pin with 523k jig only, high active pin because it is reversed */
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (battery->pdata->main_bat_enb_gpio) {
				pr_info("%s main battery enb = %d\n", __func__, x);
				if (x == 0) {
					union power_supply_propval value = {0, };
					/* activate main limiter */
					gpio_direction_output(battery->pdata->main_bat_enb_gpio, 1);
					msleep(100);
					value.intval = 1;
					psy_do_property(battery->pdata->main_limiter_name, set,
						POWER_SUPPLY_EXT_PROP_POWERMETER_ENABLE, value);
				} else if (x == 1) {
					/* deactivate main limiter */
					gpio_direction_output(battery->pdata->main_bat_enb_gpio, 0);
				}
				pr_info("%s main enb = %d, main enb2 = %d, sub enb = %d\n",
					__func__,
					gpio_get_value(battery->pdata->main_bat_enb_gpio),
					gpio_get_value(battery->pdata->main_bat_enb2_gpio),
					gpio_get_value(battery->pdata->sub_bat_enb_gpio));
			}
			ret = count;
		}
		break;
	case BATT_MAIN_ENB2: /* Low active pin */
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (battery->pdata->main_bat_enb2_gpio) {
				pr_info("%s main battery enb2 = %d\n", __func__, x);
				if (x == 0) {
					union power_supply_propval value = {0, };
					/* activate main limiter */
					gpio_direction_output(battery->pdata->main_bat_enb2_gpio, 0);
					msleep(100);
					value.intval = 1;
					psy_do_property(battery->pdata->main_limiter_name, set,
						POWER_SUPPLY_EXT_PROP_POWERMETER_ENABLE, value);
				} else if (x == 1) {
					/* deactivate main limiter */
					gpio_direction_output(battery->pdata->main_bat_enb2_gpio, 1);
				}
				pr_info("%s main enb = %d, main enb2 = %d, sub enb = %d\n",
					__func__,
					gpio_get_value(battery->pdata->main_bat_enb_gpio),
					gpio_get_value(battery->pdata->main_bat_enb2_gpio),
					gpio_get_value(battery->pdata->sub_bat_enb_gpio));
			}
			ret = count;
		}
		break;
	case BATT_SUB_ENB: /* Low active pin */
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (battery->pdata->sub_bat_enb_gpio) {
				pr_info("%s sub battery enb = %d\n", __func__, x);
				if (x == 0) {
					union power_supply_propval value = {0, };
					/* activate sub limiter */
					gpio_direction_output(battery->pdata->sub_bat_enb_gpio, 0);
					msleep(100);
					value.intval = 1;
					psy_do_property(battery->pdata->sub_limiter_name, set,
						POWER_SUPPLY_EXT_PROP_POWERMETER_ENABLE, value);
				} else if (x == 1) {
					/* deactivate sub limiter */
					gpio_direction_output(battery->pdata->sub_bat_enb_gpio, 1);
				}
				pr_info("%s main enb = %d, sub enb = %d\n",
					__func__,
					gpio_get_value(battery->pdata->main_bat_enb_gpio),
					gpio_get_value(battery->pdata->sub_bat_enb_gpio));
			}
			ret = count;
		}
		break;
	case BATT_SUB_PWR_MODE2:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			union power_supply_propval value = {0, };
			pr_info("%s sub pwr mode2 = %d\n", __func__, x);
			if (x == 0) {
				value.intval = 0;
				psy_do_property(battery->pdata->sub_limiter_name, set,
					POWER_SUPPLY_EXT_PROP_POWER_MODE2, value);
			} else if (x == 1) {
				value.intval = 1;
				psy_do_property(battery->pdata->sub_limiter_name, set,
					POWER_SUPPLY_EXT_PROP_POWER_MODE2, value);
			}
			ret = count;
		}
		break;
#else /* max17333 */
	case BATT_MAIN_SHIPMODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			union power_supply_propval value = {0, };
			pr_info("%s main limiter shipmode = %d\n", __func__, x);
			if (x == 1) {
				value.intval = 1;
				psy_do_property(battery->pdata->main_limiter_name, set,
					POWER_SUPPLY_EXT_PROP_LIMITER_SHIPMODE, value);
			} else {
				pr_info("%s wrong option for main limiter shipmode\n", __func__);
			}
			ret = count;
		}
		break;
	case BATT_SUB_SHIPMODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			union power_supply_propval value = {0, };
			pr_info("%s sub limiter shipmode = %d\n", __func__, x);
			if (x == 1) {
				value.intval = 1;
				psy_do_property(battery->pdata->sub_limiter_name, set,
					POWER_SUPPLY_EXT_PROP_LIMITER_SHIPMODE, value);
			} else {
				pr_info("%s wrong option for sub limiter shipmode\n", __func__);
			}
			ret = count;
		}
		break;
#endif
#if (IS_ENABLED(CONFIG_DUAL_FUELGAUGE) || IS_ENABLED(CONFIG_DUAL_SBP))
	case BATT_MAIN_SOC:
	case BATT_SUB_SOC:
#endif
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	case BATT_MAIN_RAW_SOC:
	case BATT_SUB_RAW_SOC:
	case BATT_MAIN_REPCAP:
	case BATT_SUB_REPCAP:
	case BATT_MAIN_FULLCAPREP:
	case BATT_SUB_FULLCAPREP:
		break;
#endif
#endif /* CONFIG_DUAL_BATTERY */
	case EXT_EVENT:
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: ext event 0x%x\n", __func__, x);
			battery->ext_event = x;
			__pm_stay_awake(battery->ext_event_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->ext_event_work, 0);
			ret = count;
		}
#endif
		break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case DIRECT_CHARGING_STATUS:
		break;
	case DIRECT_CHARGING_STEP:
		break;
	case DIRECT_CHARGING_IIN:
		break;
	case DIRECT_CHARGING_CHG_STATUS:
		break;
	case DIRECT_CHARGING_RATIO:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (is_pd_apdo_wire_type(battery->cable_type) &&
				is_dc_higher_ratio_support() &&
				x <= battery->dc_ta_op_max_mode &&
				x != get_sec_vote_result(battery->dc_op_mode_vote)) {
				dev_info(battery->dev, "%s: Request Change Charging Ratio : %d:1\n",
						__func__, x);

				/* To prevent current jump during ratio change */
				sec_vote(battery->fcc_vote, VOTER_DC_OP_MODE_SYSFS, true, 3001);
				if (x < DC_MODE_2TO1) {
					dev_info(battery->dev, "%s: clear force ratio setting\n", __func__);
					sec_vote(battery->apdo_max_volt_vote, VOTER_DC_OP_MODE_SYSFS, false, 0);
					sec_vote(battery->apdo_max_volt_vote, VOTER_DC_OP_MODE_SYSFS, false, 0);
				} else {
					sec_vote(battery->apdo_max_volt_vote, VOTER_DC_OP_MODE_SYSFS, true,
						battery->pdata->apdo_max_volt_arr[x]);
					sec_vote(battery->dc_op_mode_vote, VOTER_DC_OP_MODE_SYSFS, true, x);
				}
				sec_vote(battery->fcc_vote, VOTER_DC_OP_MODE_SYSFS, false, 0);
			}
			ret = count;
		}
		break;
	case SWITCH_CHARGING_SOURCE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (is_pd_apdo_wire_type(battery->cable_type)) {
				dev_info(battery->dev, "%s: Request Change Charging Source : %s\n",
						__func__, x == 0 ? "Switch Charger" : "Direct Charger");
				sec_bat_reset_step_charging(battery);
				direct_charging_source_status[0] = SEC_TEST_MODE;
				direct_charging_source_status[1] =
					(x == 0) ? SEC_CHARGING_SOURCE_SWITCHING : SEC_CHARGING_SOURCE_DIRECT;
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) {
					direct_charging_source_status[1] = SEC_CHARGING_SOURCE_SWITCHING;
					pr_info("%s : Change Charging Source to S/C because of Swelling\n", __func__);
				}
				value.strval = direct_charging_source_status;
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
			}
			ret = count;
		}
		break;
	case DC_ERROR_CAUSE:
		break;
#endif
	case CHARGING_TYPE:
		break;
	case BATT_FACTORY_MODE:
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			battery->usb_factory_mode = value.intval;
			battery->usb_factory_init = true;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_BATT_F_MODE, value);
			ret = count;
		}
#endif
		break;
	case BOOT_COMPLETED:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev,
				"%s: boot completed(%d)\n", __func__, x);
			ret = count;
		}
		break;
	case PD_DISABLE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("%s: PD wired charging mode is %s\n",
				__func__, ((x == 1) ? "disabled" : "enabled"));

			if (x == 1) {
				battery->pd_disable = true;
				sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);

				if (is_pd_wire_type(battery->cable_type)) {
					battery->update_pd_list = true;
					pr_info("%s: update pd list\n", __func__);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
					if (is_pd_apdo_wire_type(battery->cable_type))
						psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);
#endif
					sec_vote_refresh(battery->iv_vote);
				}
			} else {
				battery->pd_disable = false;
				sec_bat_set_current_event(battery,
					0, SEC_BAT_CURRENT_EVENT_HV_DISABLE);

				if (is_pd_wire_type(battery->cable_type)) {
					battery->update_pd_list = true;
					pr_info("%s: update pd list\n", __func__);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
					if (is_pd_apdo_wire_type(battery->cable_type))
						psy_do_property(battery->pdata->charger_name, set,
							POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);
#endif
					sec_vote_refresh(battery->iv_vote);
				}
			}
			ret = count;
		}
		break;
	case FACTORY_MODE_RELIEVE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION, value);
			ret = count;
		}
		break;
	case FACTORY_MODE_BYPASS:
		pr_info("%s: factory mode bypass\n", __func__);
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_AUTHENTIC, value);
			ret = count;
		}
		break;
	case NORMAL_MODE_BYPASS:
		pr_info("%s: normal mode bypass for current measure\n", __func__);
		if (sscanf(buf, "%10d\n", &x) == 1) {
//			if (battery->pdata->detect_moisture && x) {
//				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
//				sec_bat_set_charge(battery, SEC_BAT_CHG_MODE_BUCK_OFF);
//			}

			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_CURRENT_MEASURE, value);
			ret = count;
		}
		break;
	case FACTORY_VOLTAGE_REGULATION:
		{
			sscanf(buf, "%10d\n", &x);
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_FACTORY_VOLTAGE_REGULATION, value);

			value.intval =
				SEC_FUELGAUGE_CAPACITY_TYPE_RESET;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CAPACITY, value);
			dev_info(battery->dev,"do reset SOC\n");
			/* update battery info */
			sec_bat_get_battery_info(battery);
		}
		ret = count;
		break;
	case FACTORY_MODE_DISABLE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DISABLE_FACTORY_MODE, value);
			ret = count;
		}
		break;
	case USB_CONF:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_USB_CONFIGURE, value);
			ret = count;
		}
		break;
	case CHARGE_OTG_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
			ret = count;
		}
		break;
	case CHARGE_UNO_CONTROL:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x && (is_hv_wire_type(battery->cable_type) ||
				is_hv_pdo_wire_type(battery->cable_type, battery->hv_pdo))) {
				pr_info("%s: Skip uno control during HV wired charging\n", __func__);
				break;
			}
			value.intval = x;
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
			ret = count;
		}
		break;
	case CHARGE_COUNTER_SHADOW:
		break;
	case VOTER_STATUS:
		break;
#if defined(CONFIG_WIRELESS_IC_PARAM)
	case WC_PARAM_INFO:
		break;
#endif
	case LRP:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev, "%s: LRP(%d)\n", __func__, x);
			if ((x >= -200 && x <= 900) && (battery->lrp_test == 0))
				battery->lrp = x;
			ret = count;
		}
		break;
	case HP_D2D:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			dev_info(battery->dev, "%s: set high power d2d(%d)\n", __func__, x);
			battery->hp_d2d = x;
			ret = count;
		}
		break;
	case DC_RB_EN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			pr_info("%s: en reverse boost(%d)\n", __func__, value.intval);
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL, value);
			ret = count;
		}
		break;
	case DC_OP_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			pr_info("%s: en dc op mode(%d)\n", __func__, value.intval);
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DC_OP_MODE, value);
			ret = count;
		}

		break;
	case DC_ADC_MODE:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			pr_info("%s: en adc mode(%d)\n", __func__, value.intval);
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_ADC_MODE, value);
			ret = count;
		}
		break;
	case DC_VBUS:
		break;
	case CHG_TYPE:
		break;
	case MST_EN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			value.intval = x;
			pr_info("%s: mst en(%d)\n", __func__, value.intval);
			psy_do_property("battery", set,
				POWER_SUPPLY_EXT_PROP_MST_EN, value);
			ret = count;
		}
		break;
	case SPSN_TEST:
		break;
	case CHG_SOC_LIM:
	{
#if defined(CONFIG_SEC_FACTORY)
		int y = 0;

		if (sscanf(buf, "%10d %10d\n", &x, &y) == 2) {
			if (x >= y) {
				pr_info("%s: min SOC (%d) higher/equal to max SOC (%d)\n",
					__func__, x, y);
			} else if (x >= 0 && y >= 0 && x <= 100 && y <= 100) {
				battery->pdata->store_mode_charging_min = x;
				battery->pdata->store_mode_charging_max = y;
			} else {
				pr_info("%s: Invalid min/max SOC (%d/%d)\n", __func__, x, y);
			}
			ret = count;
		}
#endif
		break;
	}
	case MAG_COVER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("@MPP %s: update mag_cover(%d)\n", __func__, x);
			battery->mag_cover = x;
			value.intval = battery->mag_cover;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_MPP_COVER, value);
			ret = count;
		}
		break;
	case MAG_CLOAK:
		if (sscanf(buf, "%10d %10d\n", &x, &y) == 2) {
			pr_info("@MPP %s: update mag_cloak(%d) - reason(%d)\n", __func__, x, y);

			value.intval = x;
			if (x == 0)
				value.intval = CLOAK_EXIT_CMD;
			else if (x != 0 && y == 5) /* MFC_TRX_MPP_CLOAK_END_OF_CHARGE*/
				value.intval = CLOAK_END_OF_CHARGE;
			else if (x != 0 && y == 6) /* MFC_TRX_MPP_CLOAK_PTX_INITIATED*/
				value.intval = CLOAK_PTX_INITIATED;
			psy_do_property("wireless", set,
				POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
			ret = count;
		}
		break;
	case ARI_CNT:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("@ARI %s: (%d)\n", __func__, x);
			value.intval = x;
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_ARI_CNT, value);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
			psy_do_property(battery->pdata->dual_battery_name, set,
				POWER_SUPPLY_EXT_PROP_ARI_CNT, value);
#endif
		}
		ret = count;
		break;
#if IS_ENABLED(CONFIG_BATTERY_AUTH_SLE956681)
	case VK_KEY_STATUS:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("@vk_key_status %s: (%d)\n", __func__, x);
			battery->vk_key_status = x;
		}
		ret = count;
		break;
#endif
	case ADC_RSENSE: /* for tuning adc_rsense of bat_thm only now */
		if (sscanf(buf, "%10d\n", &x) == 1) {
			pr_info("@adc_rsense %s: (%d)\n", __func__, x);
			battery->pdata->bat_thm_info.adc_rsense = x;
		}
		ret = count;
		break;
	case OCPWARN:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			if (x == 0 || (x >= 2800 && x <= 13000)) {
				pr_info("@OCPWARN %s: (%d)\n", __func__, x);
				value.intval = x;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_OCPWARN, value);
			} else
				pr_info("@OCPWARN %s: Invalid value %d min/max range (2800 ~ 13000)\n", __func__, x);
		}
		ret = count;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int sec_bat_create_attrs(struct device *dev)
{
	unsigned long i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(sec_battery_attrs); i++) {
		rc = device_create_file(dev, &sec_battery_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_battery_attrs[i]);
create_attrs_succeed:
	return rc;
}
EXPORT_SYMBOL(sec_bat_create_attrs);

ssize_t sec_pogo_show_attrs(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sec_pogo_attrs;
	int i = 0;

	switch (offset) {
	case POGO_SEC_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "POGO\n");
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

int sec_pogo_create_attrs(struct device *dev)
{
	unsigned long i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(sec_pogo_attrs); i++) {
		rc = device_create_file(dev, &sec_pogo_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_pogo_attrs[i]);
create_attrs_succeed:
	return rc;
}
EXPORT_SYMBOL(sec_pogo_create_attrs);

ssize_t sec_otg_show_attrs(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - sec_otg_attrs;
	int i = 0;

	switch (offset) {
	case OTG_SEC_TYPE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "OTG\n");
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

int sec_otg_create_attrs(struct device *dev)
{
	unsigned long i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(sec_otg_attrs); i++) {
		rc = device_create_file(dev, &sec_otg_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &sec_otg_attrs[i]);
create_attrs_succeed:
	return rc;
}
EXPORT_SYMBOL(sec_otg_create_attrs);
