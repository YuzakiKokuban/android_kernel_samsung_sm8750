/*
 * sec_battery.c
 * Samsung Mobile Battery Driver
 *
 * Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/battery/sb_sysfs.h>
#include <linux/battery/sb_notify.h>

#include "sec_battery.h"
#include "sec_battery_sysfs.h"
#include "sec_battery_dt.h"
#include "sec_battery_ttf.h"
#if defined(CONFIG_SEC_COMMON)
#include <linux/sec_common.h>
#endif

#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

#if IS_ENABLED(CONFIG_SEC_PARAM)
#if defined(CONFIG_ARCH_QCOM) && !(defined(CONFIG_ARCH_EXYNOS) || defined(CONFIG_ARCH_MEDIATEK))
#include <linux/samsung/sec_param.h>
#endif
#endif
#if IS_ENABLED(CONFIG_SEC_DEBUG)
#include <linux/sec_debug.h>
#endif

#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#else
#define __visible_for_testing static
#endif

#if defined(CONFIG_UML)
#include "kunit_test/uml_dummy.h"
#endif

#include "battery_logger.h"
#include "sb_tx.h"
#include "sb_batt_dump.h"
#if IS_ENABLED(CONFIG_VBUS_NOTIFIER) && IS_ENABLED(CONFIG_LSI_IFPMIC)
#include <linux/vbus_notifier.h>
#endif

#define BUFF_SIZE 1024

static unsigned int __read_mostly lpcharge;
module_param(lpcharge, uint, 0444);
static int __read_mostly fg_reset;
module_param(fg_reset, int, 0444);
static int factory_mode;
module_param(factory_mode, int, 0444);
static unsigned int __read_mostly charging_mode;
module_param(charging_mode, uint, 0444);
static unsigned int __read_mostly pd_disable;
module_param(pd_disable, uint, 0444);
static char * __read_mostly sales_code;
module_param(sales_code, charp, 0444);
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
static char __read_mostly *f_mode;
module_param(f_mode, charp, 0444);
#endif

static const char *sec_voter_name[] = {
	FOREACH_VOTER(GENERATE_STRING)
};

static enum power_supply_property sec_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
#if IS_ENABLED(CONFIG_FUELGAUGE_MAX77705) || IS_ENABLED(CONFIG_FUELGAUGE_MAX77775)
	POWER_SUPPLY_PROP_POWER_NOW,
	POWER_SUPPLY_PROP_POWER_AVG,
#endif
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
};

static enum power_supply_property sec_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property sec_wireless_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
};

static enum power_supply_property sec_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property sec_otg_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
};

static char *supply_list[] = {
	"battery",
};

const char *sb_get_ct_str(int ct)
{
	switch (ct) {
	case SEC_BATTERY_CABLE_UNKNOWN:
		return "UNKNOWN";
	case SEC_BATTERY_CABLE_NONE:
		return "NONE";
	case SEC_BATTERY_CABLE_PREPARE_TA:
		return "PREPARE_TA";
	case SEC_BATTERY_CABLE_TA:
		return "TA";
	case SEC_BATTERY_CABLE_USB:
		return "USB";
	case SEC_BATTERY_CABLE_USB_CDP:
		return "USB_CDP";
	case SEC_BATTERY_CABLE_9V_TA:
		return "9V_TA";
	case SEC_BATTERY_CABLE_9V_ERR:
		return "9V_ERR";
	case SEC_BATTERY_CABLE_9V_UNKNOWN:
		return "9V_UNKNOWN";
	case SEC_BATTERY_CABLE_12V_TA:
		return "12V_TA";
	case SEC_BATTERY_CABLE_WIRELESS:
		return "WC";
	case SEC_BATTERY_CABLE_HV_WIRELESS:
		return "HV_WC";
	case SEC_BATTERY_CABLE_PMA_WIRELESS:
		return "PMA_WC";
	case SEC_BATTERY_CABLE_WIRELESS_PACK:
		return "WC_PACK";
	case SEC_BATTERY_CABLE_WIRELESS_HV_PACK:
		return "WC_HV_PACK";
	case SEC_BATTERY_CABLE_WIRELESS_STAND:
		return "WC_STAND";
	case SEC_BATTERY_CABLE_WIRELESS_HV_STAND:
		return "WC_HV_STAND";
	case SEC_BATTERY_CABLE_QC20:
		return "QC20";
	case SEC_BATTERY_CABLE_QC30:
		return "QC30";
	case SEC_BATTERY_CABLE_PDIC:
		return "PDIC";
	case SEC_BATTERY_CABLE_UARTOFF:
		return "UARTOFF";
	case SEC_BATTERY_CABLE_OTG:
		return "OTG";
	case SEC_BATTERY_CABLE_LAN_HUB:
		return "LAN_HUB";
	case SEC_BATTERY_CABLE_LO_TA:
		return "LO_TA";
	case SEC_BATTERY_CABLE_POWER_SHARING:
		return "POWER_SHARING";
	case SEC_BATTERY_CABLE_HMT_CONNECTED:
		return "HMT_CONNECTED";
	case SEC_BATTERY_CABLE_HMT_CHARGE:
		return "HMT_CHARGE";
	case SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT:
		return "HV_TA_CHG_LIMIT";
	case SEC_BATTERY_CABLE_WIRELESS_VEHICLE:
		return "WC_VEHICLE";
	case SEC_BATTERY_CABLE_WIRELESS_HV_VEHICLE:
		return "WC_HV_VEHICLE";
	case SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV:
		return "WC_HV_PREPARE";
	case SEC_BATTERY_CABLE_TIMEOUT:
		return "TIMEOUT";
	case SEC_BATTERY_CABLE_SMART_OTG:
		return "SMART_OTG";
	case SEC_BATTERY_CABLE_SMART_NOTG:
		return "SMART_NOTG";
	case SEC_BATTERY_CABLE_WIRELESS_TX:
		return "WC_TX";
	case SEC_BATTERY_CABLE_HV_WIRELESS_20:
		return "HV_WC_20";
	case SEC_BATTERY_CABLE_HV_WIRELESS_20_LIMIT:
		return "HV_WC_20_LIMIT";
	case SEC_BATTERY_CABLE_WIRELESS_FAKE:
		return "WC_FAKE";
	case SEC_BATTERY_CABLE_PREPARE_WIRELESS_20:
		return "HV_WC_20_PREPARE";
	case SEC_BATTERY_CABLE_PDIC_APDO:
		return "PDIC_APDO";
	case SEC_BATTERY_CABLE_POGO:
		return "POGO";
	case SEC_BATTERY_CABLE_POGO_9V:
		return "POGO_9V";
	case SEC_BATTERY_CABLE_FPDO_DC:
		return "FPDO_DC";
	case SEC_BATTERY_CABLE_WIRELESS_EPP:
		return "WC_EPP";
	case SEC_BATTERY_CABLE_WIRELESS_MPP:
		return "WC_MPP";
	case SEC_BATTERY_CABLE_WIRELESS_MPP_FAKE:
		return "WC_MPP_FAKE";
	case SEC_BATTERY_CABLE_WIRELESS_EPP_NV:
		return "WC_EPP_NV";
	case SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE:
		return "WC_EPP_FAKE";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_get_ct_str);

const char *sb_get_cm_str(int charging_mode)
{
	switch (charging_mode) {
	case SEC_BATTERY_CHARGING_NONE:
		return "None";
	case SEC_BATTERY_CHARGING_1ST:
		return "Normal";
	case SEC_BATTERY_CHARGING_2ND:
		return "Additional";
	case SEC_BATTERY_CHARGING_RECHARGING:
		return "Re-Charging";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_get_cm_str);

const char *sb_get_bst_str(int status)
{
	switch (status) {
	case POWER_SUPPLY_STATUS_UNKNOWN:
		return "Unknown";
	case POWER_SUPPLY_STATUS_CHARGING:
		return "Charging";
	case POWER_SUPPLY_STATUS_DISCHARGING:
		return "Discharging";
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		return "Not-charging";
	case POWER_SUPPLY_STATUS_FULL:
		return "Full";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_get_bst_str);

const char *sb_get_hl_str(int health)
{
	switch (health) {
	case POWER_SUPPLY_HEALTH_GOOD:
		return "Good";
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		return "Overheat";
	case POWER_SUPPLY_HEALTH_DEAD:
		return "Dead";
	case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		return "Over voltage";
	case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
		return "Unspecified failure";
	case POWER_SUPPLY_HEALTH_COLD:
		return "Cold";
	case POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE:
		return "Watchdog timer expire";
	case POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE:
		return "Safety timer expire";
	case POWER_SUPPLY_HEALTH_OVERCURRENT:
		return "Over current";
	case POWER_SUPPLY_HEALTH_CALIBRATION_REQUIRED:
		return "Cal required";
	case POWER_SUPPLY_HEALTH_WARM:
		return "Warm";
	case POWER_SUPPLY_HEALTH_COOL:
		return "Cool";
	case POWER_SUPPLY_HEALTH_HOT:
		return "Hot";
	case POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE:
		return "UnderVoltage";
	case POWER_SUPPLY_EXT_HEALTH_OVERHEATLIMIT:
		return "OverheatLimit";
	case POWER_SUPPLY_EXT_HEALTH_VSYS_OVP:
		return "VsysOVP";
	case POWER_SUPPLY_EXT_HEALTH_VBAT_OVP:
		return "VbatOVP";
	case POWER_SUPPLY_EXT_HEALTH_DC_ERR:
		return "DCErr";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_get_hl_str);

const char *sb_get_tz_str(int tz)
{
	switch (tz) {
	case BAT_THERMAL_NORMAL:
		return "Normal";
	case BAT_THERMAL_COLD:
		return "COLD";
	case BAT_THERMAL_COOL3:
		return "COOL3";
	case BAT_THERMAL_COOL2:
		return "COOL2";
	case BAT_THERMAL_COOL1:
		return "COOL1";
	case BAT_THERMAL_WARM:
		return "WARM";
	case BAT_THERMAL_OVERHEAT:
		return "OVERHEAT";
	case BAT_THERMAL_OVERHEATLIMIT:
		return "OVERHEATLIM";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_get_tz_str);

const char *sb_charge_mode_str(int charge_mode)
{
	switch (charge_mode) {
	case SEC_BAT_CHG_MODE_BUCK_OFF:
		return "Buck-Off";
	case SEC_BAT_CHG_MODE_BUCK_OFF_LINEAR_CHARGING:
		return "Buck-Off/Linear-On";
	case SEC_BAT_CHG_MODE_CHARGING_OFF:
		return "Charging-Off";
	case SEC_BAT_CHG_MODE_PASS_THROUGH:
		return "Pass-Through";
	case SEC_BAT_CHG_MODE_CHARGING:
		return "Charging-On";
	case SEC_BAT_CHG_MODE_OTG_ON:
		return "OTG-On";
	case SEC_BAT_CHG_MODE_OTG_OFF:
		return "OTG-Off";
	case SEC_BAT_CHG_MODE_UNO_ON:
		return "UNO-On";
	case SEC_BAT_CHG_MODE_UNO_OFF:
		return "UNO-Off";
	case SEC_BAT_CHG_MODE_UNO_ONLY:
		return "UNO-Only";
	case SEC_BAT_CHG_MODE_NOT_SET:
		return "Not-Set";
	case SEC_BAT_CHG_MODE_MAX:
		return "Max";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_charge_mode_str);

const char *sb_rx_type_str(int type)
{
	switch (type) {
	case NO_DEV:
		return "No Dev";
	case OTHER_DEV:
		return "Other Dev";
	case SS_GEAR:
		return "Gear";
	case SS_PHONE:
		return "Phone";
	case SS_BUDS:
		return "Buds";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_rx_type_str);

const char *sb_vout_ctr_mode_str(int vout_mode)
{
	switch (vout_mode) {
	case WIRELESS_VOUT_OFF:
		return "Set VOUT Off";
	case WIRELESS_VOUT_NORMAL_VOLTAGE:
		return "Set VOUT NV";
	case WIRELESS_VOUT_RESERVED:
		return "Set VOUT Rsv";
	case WIRELESS_VOUT_HIGH_VOLTAGE:
		return "Set VOUT HV";
	case WIRELESS_VOUT_CC_CV_VOUT:
		return "Set VOUT CV";
	case WIRELESS_VOUT_CALL:
		return "Set VOUT Call";
	case WIRELESS_VOUT_5V:
		return "Set VOUT 5V";
	case WIRELESS_VOUT_9V:
		return "Set VOUT 9V";
	case WIRELESS_VOUT_10V:
		return "Set VOUT 10V";
	case WIRELESS_VOUT_11V:
		return "Set VOUT 11V";
	case WIRELESS_VOUT_12V:
		return "Set VOUT 12V";
	case WIRELESS_VOUT_12_5V:
		return "Set VOUT 12.5V";
	case WIRELESS_VOUT_13V:
		return "Set VOUT 13V";
	case WIRELESS_VOUT_4_5V_STEP:
		return "Set VOUT 4.5V Step";
	case WIRELESS_VOUT_5V_STEP:
		return "Set VOUT 5V Step";
	case WIRELESS_VOUT_5_5V_STEP:
		return "Set VOUT 5.5V Step";
	case WIRELESS_VOUT_9V_STEP:
		return "Set VOUT 9V Step";
	case WIRELESS_VOUT_10V_STEP:
		return "Set VOUT 10V Step";
	case WIRELESS_VOUT_OTG:
		return "Set VOUT OTG";
	case WIRELESS_VOUT_5_5V:
		return "Set VOUT 5.5V";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_vout_ctr_mode_str);

const char *sb_rx_vout_str(int vout)
{
	switch (vout) {
	case MFC_VOUT_4_5V:
		return "VOUT 4.5V";
	case MFC_VOUT_4_7V:
		return "VOUT 4.7V";
	case MFC_VOUT_4_8V:
		return "VOUT 4.8V";
	case MFC_VOUT_4_9V:
		return "VOUT 4.9V";
	case MFC_VOUT_5V:
		return "VOUT 5V";
	case MFC_VOUT_5_5V:
		return "VOUT 5.5V";
	case MFC_VOUT_6V:
		return "VOUT 6V";
	case MFC_VOUT_7V:
		return "VOUT 7V";
	case MFC_VOUT_8V:
		return "VOUT 8V";
	case MFC_VOUT_9V:
		return "VOUT 9V";
	case MFC_VOUT_10V:
		return "VOUT 10V";
	case MFC_VOUT_11V:
		return "VOUT 11V";
	case MFC_VOUT_12V:
		return "VOUT 12V";
	case MFC_VOUT_12_5V:
		return "VOUT 12.5V";
	case MFC_VOUT_OTG:
		return "VOUT OTG";
	case MFC_VOUT_13V:
		return "VOUT 13V";
	default:
		return "UNDEFINED";
	}
}
EXPORT_SYMBOL(sb_rx_vout_str);

__visible_for_testing int sec_bat_check_afc_input_current(struct sec_battery_info *battery);
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
__visible_for_testing void sec_bat_set_rp_current(struct sec_battery_info *battery, int cable_type);
#endif

unsigned int sec_bat_get_lpmode(void) {	return lpcharge; }
void sec_bat_set_lpmode(unsigned int value) { lpcharge = value; }
EXPORT_SYMBOL_KUNIT(sec_bat_set_lpmode);
int sec_bat_get_fgreset(void) { return fg_reset; }
int sec_bat_get_facmode(void) { return factory_mode; }
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
void sec_bat_set_facmode(int value) { factory_mode = value; }
#endif
unsigned int sec_bat_get_chgmode(void) { return charging_mode; }
EXPORT_SYMBOL_KUNIT(sec_bat_get_chgmode);
void sec_bat_set_chgmode(unsigned int value) { charging_mode = value; }
EXPORT_SYMBOL_KUNIT(sec_bat_set_chgmode);
unsigned int sec_bat_get_dispd(void) { return pd_disable; }
EXPORT_SYMBOL_KUNIT(sec_bat_get_dispd);
void sec_bat_set_dispd(unsigned int value) { pd_disable = value; }
EXPORT_SYMBOL_KUNIT(sec_bat_set_dispd);
char *sec_bat_get_sales_code(void) {return sales_code; }

#if !defined(CONFIG_SEC_FACTORY)
#define SALE_CODE_STR_LEN		3
bool sales_code_is(char *str)
{
	if (sec_bat_get_sales_code() == NULL)
		return false;

	pr_info("%s: %s\n", __func__, sec_bat_get_sales_code());

	return !strncmp(sec_bat_get_sales_code(), str, SALE_CODE_STR_LEN + 1);
}
#endif

static bool check_silent_type(int ct)
{
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	if (ct == ATTACHED_DEV_RETRY_TIMEOUT_OPEN_MUIC ||
			ct == ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC ||
			ct == ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC)
		return true;
#endif
	return false;
}
static bool check_afc_disabled_type(int ct)
{
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	if (ct == ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC)
		return true;
#endif
	return false;
}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
__visible_for_testing void sec_bat_divide_limiter_current(struct sec_battery_info *battery, int limiter_current)
{
	unsigned int main_current = 0, sub_current = 0, main_current_rate = 0, sub_current_rate = 0;
	union power_supply_propval value = {0, };

	if (is_pd_apdo_wire_type(battery->cable_type) && battery->pd_list.now_isApdo) {
		if (limiter_current < battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current)
			limiter_current = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current;
	}

	if (battery->pdata->sub_fto && limiter_current >= battery->pdata->sub_fto_current_thresh) {
		value.intval = 3;
		psy_do_property(battery->pdata->sub_limiter_name, set,
		POWER_SUPPLY_EXT_PROP_CHG_MODE, value);
	} else if (battery->pdata->sub_fto && limiter_current < battery->pdata->sub_fto_current_thresh) {
		value.intval = 0;
		psy_do_property(battery->pdata->sub_limiter_name, set,
		POWER_SUPPLY_EXT_PROP_CHG_MODE, value);
	}

	if (limiter_current >= battery->pdata->zone3_limiter_current) {
		main_current_rate = battery->pdata->main_zone3_current_rate;
		sub_current_rate = battery->pdata->sub_zone3_current_rate;
	} else if (limiter_current >= battery->pdata->zone2_limiter_current) {
		main_current_rate = battery->pdata->main_zone2_current_rate;
		sub_current_rate = battery->pdata->sub_zone2_current_rate;
	} else if (limiter_current > battery->pdata->zone1_limiter_current) {
		main_current_rate = battery->pdata->main_zone1_current_rate;
		sub_current_rate = battery->pdata->sub_zone1_current_rate;
	} else { /* like discharge current 100mA */
		main_current_rate = battery->pdata->min_main_limiter_current;
		sub_current_rate = battery->pdata->min_sub_limiter_current;
	}

	/* divide setting has ratio value(percent value, max 99%) and current value(ex 1500mA) */
	main_current = (main_current_rate > 100) ? main_current_rate : ((main_current_rate * limiter_current) / 100);
	sub_current = (sub_current_rate > 100) ? sub_current_rate : ((sub_current_rate * limiter_current) / 100);

	pr_info("%s: (%d) -> main_current(%d), sub_current(%d)\n", __func__,
		limiter_current, main_current, sub_current); // debug

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3) {
		if (main_current > battery->pdata->limiter_main_cool3_current)
			main_current = battery->pdata->limiter_main_cool3_current;
		if (sub_current > battery->pdata->limiter_sub_cool3_current)
			sub_current = battery->pdata->limiter_sub_cool3_current;
	} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2) {
		if (main_current > battery->pdata->limiter_main_cool2_current)
			main_current = battery->pdata->limiter_main_cool2_current;
		if (sub_current > battery->pdata->limiter_sub_cool2_current)
			sub_current = battery->pdata->limiter_sub_cool2_current;
	} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING) {
		if (main_current > battery->pdata->limiter_main_warm_current)
			main_current = battery->pdata->limiter_main_warm_current;
		if (sub_current > battery->pdata->limiter_sub_warm_current)
			sub_current = battery->pdata->limiter_sub_warm_current;
	} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1) {
		if (main_current > battery->pdata->limiter_main_cool1_current)
			main_current = battery->pdata->limiter_main_cool1_current;
		if (sub_current > battery->pdata->limiter_sub_cool1_current)
			sub_current = battery->pdata->limiter_sub_cool1_current;
	}

	/* calculate main battery current */
	if (main_current > battery->pdata->max_main_limiter_current)
		main_current = battery->pdata->max_main_limiter_current;

	/* calculate sub battery current */
	if (sub_current > battery->pdata->max_sub_limiter_current)
		sub_current = battery->pdata->max_sub_limiter_current;

	pr_info("%s: main_current(%d), sub_current(%d)\n", __func__,
		main_current, sub_current);

	battery->main_current = main_current;
	battery->sub_current = sub_current;
}
EXPORT_SYMBOL_KUNIT(sec_bat_divide_limiter_current);

__visible_for_testing void sec_bat_set_limiter_current(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	pr_info("%s: charge_m(%d), charge_s(%d)\n", __func__,
		battery->main_current, battery->sub_current);

	value.intval = battery->main_current;
	psy_do_property(battery->pdata->main_limiter_name, set,
		POWER_SUPPLY_EXT_PROP_FASTCHG_LIMIT_CURRENT, value);

	value.intval = battery->sub_current;
	psy_do_property(battery->pdata->sub_limiter_name, set,
		POWER_SUPPLY_EXT_PROP_FASTCHG_LIMIT_CURRENT, value);
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_limiter_current);
#endif

__visible_for_testing int set_charging_current(void *data, int v)
{
	union power_supply_propval value = {0, };
	struct sec_battery_info *battery = data;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	sec_bat_divide_limiter_current(battery, v);
	/* to set higher current, ex) 500mA -> 1000mA */
	if (battery->charging_current < v) {
		pr_info("%s: set limiter current right away\n", __func__);
		sec_bat_set_limiter_current(battery);
	}
#endif

	value.intval = v;
	if (is_wireless_type(battery->cable_type)) {
		if (battery->charging_current < v) {
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, value);
		} else {
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_CONSTANT_CHARGE_CURRENT_WRL, value);
		}
	} else {
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, value);
	}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	/* to set lower current, ex) 1000mA -> 500mA */
	if (battery->charging_current > v) {
		if (!is_wireless_type(battery->cable_type)) {
			if (is_pd_apdo_wire_type(battery->cable_type) && battery->pd_list.now_isApdo) {
				battery->set_lower_curr = true;
				pr_info("%s: set limiter current in next polling\n", __func__);
			} else {
				pr_info("%s: set limiter current right away\n", __func__);
				sec_bat_set_limiter_current(battery);
			}
		}
	}
#endif
	battery->charging_current = v;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	pr_info("%s: power(%d), input(%d), charge(%d), charge_m(%d), charge_s(%d)\n", __func__,
			battery->charge_power, battery->input_current, battery->charging_current, battery->main_current, battery->sub_current);
#else
	pr_info("%s: power(%d), input(%d), charge(%d)\n", __func__,
			battery->charge_power, battery->input_current, battery->charging_current);
#endif
	return v;
}
EXPORT_SYMBOL_KUNIT(set_charging_current);

__visible_for_testing int set_input_current(void *data, int v)
{
	union power_supply_propval value = {0, };
	struct sec_battery_info *battery = data;

	battery->input_current = v;
	battery->charge_power = mW_by_mVmA(battery->input_voltage, v);
	if (battery->charge_power > battery->max_charge_power)
		battery->max_charge_power = battery->charge_power;
	value.intval = v;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);

	if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP) {
		value.intval = v;
		psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);
	}
	return v;
}
EXPORT_SYMBOL_KUNIT(set_input_current);

__visible_for_testing int set_float_voltage(void *data, int voltage)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };

	value.intval = voltage;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
	return voltage;
}
EXPORT_SYMBOL_KUNIT(set_float_voltage);

__visible_for_testing int set_dc_float_voltage(void *data, int voltage)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };

	value.intval = voltage;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_DIRECT_CONSTANT_CHARGE_VOLTAGE, value);
	return voltage;
}
EXPORT_SYMBOL_KUNIT(set_dc_float_voltage);

__visible_for_testing int set_dc_op_mode(void *data, int dc_op_mode)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };

	store_battery_log(
	"SWITCH DC RATIO: %d:1 to %d:1,%d%%,%dmV,ct(%d,%d,%d),cev(0x%x),mev(0x%x)",
	battery->dc_op_mode,
	dc_op_mode,
	battery->capacity,
	battery->voltage_now,
	battery->cable_type,
	battery->wire_status,
	battery->pd_usb_attached,
	battery->current_event,
	battery->misc_event
	);

	value.intval = dc_op_mode;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_DC_OP_MODE, value);
	battery->dc_op_mode = dc_op_mode;

	return dc_op_mode;
}
EXPORT_SYMBOL_KUNIT(set_dc_op_mode);

int is_dc_higher_ratio_support(void)
{
	static int support = -1;
	int ret = 0;

	if (support == -1) {
		struct device_node *np;

		np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_info("%s: np NULL\n", __func__);
			support = 0;
		} else {
			ret = of_property_read_u32(np, "battery,dc_step_chg_ratio",
					&support);
			if (ret || !support) {
				/* default doesn't support other ratios */
				support = DC_MODE_NONE;
				pr_info("%s : doesn't support DC ratio controls\n", __func__);
			}
		}
		if (support)
			pr_info("%s : supports up to[%d : 1] DC MODE\n", __func__, support);
	}
	return support;
}
EXPORT_SYMBOL(is_dc_higher_ratio_support);

__visible_for_testing int set_apdo_max_volt(void *data, int apdo_max_volt)
{
	struct sec_battery_info *battery = data;

	store_battery_log(
	"APDO MAX VOLTAGE: %d to %d,%d%%,%dmV,ct(%d,%d,%d),cev(0x%x),mev(0x%x)",
	battery->pdata->apdo_max_volt,
	apdo_max_volt,
	battery->capacity,
	battery->voltage_now,
	battery->cable_type,
	battery->wire_status,
	battery->pd_usb_attached,
	battery->current_event,
	battery->misc_event
	);
	battery->pdata->apdo_max_volt = apdo_max_volt;

	return apdo_max_volt;
}
EXPORT_SYMBOL_KUNIT(set_apdo_max_volt);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
__visible_for_testing int set_vlim_voltage(void *data, int voltage)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };

	value.intval = voltage;
	psy_do_property(battery->pdata->sub_limiter_name, set,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
	return voltage;
}
EXPORT_SYMBOL_KUNIT(set_dc_float_voltage);
#endif

__visible_for_testing int set_topoff_current(void *data, int v)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval value = {0, };
	bool do_chgen_vote = false;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_2ND ||
		battery->pdata->full_check_type == SEC_BATTERY_FULLCHARGED_CHGPSY ||
		battery->pdata->full_check_type == SEC_BATTERY_FULLCHARGED_CHGINT)
		do_chgen_vote = true;

#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	if (battery->pdata->dual_battery_individual_topoff && do_chgen_vote) {
		battery->topoff_condition = v;
		pr_info("%s: skip! dual_battery_individual_topoff!\n", __func__);

		return v;
	}
#endif

	if (do_chgen_vote)
		sec_vote(battery->chgen_vote, VOTER_TOPOFF_CHANGE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

	value.intval = v;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT, value);

	if (do_chgen_vote)
		sec_vote(battery->chgen_vote, VOTER_TOPOFF_CHANGE, false, 0);
	battery->topoff_condition = v;

	return v;
}
EXPORT_SYMBOL_KUNIT(set_topoff_current);

int get_chg_power_type(int ct, int ws, int pd_max_pw, int max_pw)
{
	if (!is_wireless_type(ct)) {
		if (is_pd_wire_type(ct) &&
			pd_max_pw >= HV_CHARGER_STATUS_STANDARD4)
			return SFC_45W;
		else if (is_pd_wire_type(ct) &&
			pd_max_pw >= HV_CHARGER_STATUS_STANDARD3)
			return SFC_25W;
		else if (is_hv_wire_12v_type(ct) ||
			max_pw >= HV_CHARGER_STATUS_STANDARD2) /* 20000mW */
			return AFC_12V_OR_20W;
		else if (is_hv_wire_type(ct) ||
			(is_pd_wire_type(ct) &&
			pd_max_pw >= HV_CHARGER_STATUS_STANDARD1) ||
#if !defined(CONFIG_BC12_DEVICE)
			ws == SEC_BATTERY_CABLE_PREPARE_TA ||
#endif
			max_pw >= HV_CHARGER_STATUS_STANDARD1) /* 12000mW */
			return AFC_9V_OR_15W;
	}

	return NORMAL_TA;
}
EXPORT_SYMBOL_KUNIT(get_chg_power_type);

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
static void sec_bat_mpla_thr_cnt_check_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, mpla_thr_cnt_check_work.work);
	union power_supply_propval value = {0, };

	pr_info("%s: mpl_thr_counter: %d\n", __func__, battery->pdata->mpla_thr_cnt);

	if (battery->pdata->mpla_thr_cnt >= battery->pdata->mpla_thr_recov_cnt) {
		// sent EPT-RST
		pr_info("@mpp: %s: send ept reset for mpla throttling recovery\n", __func__);
		value.intval = EPT_RESTART;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WC_EPT_UNKNOWN, value);
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_MPLA_THR_RECOV, value);
		battery->pdata->mpla_thr_cnt = 999;
	} else {
		battery->pdata->mpla_thr_cnt = 0;
	}
	__pm_relax(battery->mpla_thr_cnt_check_ws);
}

static void sb_check_mpla_thr_cnt(struct sec_battery_info *battery, int prev_icl)
{
	int prev_cnt = battery->pdata->mpla_thr_cnt;
	int new_cnt = 0;

	if (!battery->pdata->support_mpla_thr_recov)
		return;

	if (prev_icl > 100)
		return;

	if (battery->pdata->mpla_thr_cnt == 0) {
		__pm_stay_awake(battery->mpla_thr_cnt_check_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->mpla_thr_cnt_check_work, msecs_to_jiffies(4000));
		pr_info("%s: start mpla thr recov work\n", __func__);
	}

	battery->pdata->mpla_thr_cnt++;
	new_cnt = battery->pdata->mpla_thr_cnt;

	pr_info("%s: mpl_thr_counter: from %d -> to %d\n",
		__func__, prev_cnt, new_cnt);
}
#endif

static void sec_bat_run_input_check_work(struct sec_battery_info *battery, int work_delay)
{
	unsigned int ws_duration = 0;
	unsigned int offset = 3; /* 3 seconds */

	pr_info("%s: for %s after %d msec\n", __func__, sb_get_ct_str(battery->cable_type), work_delay);

	cancel_delayed_work(&battery->input_check_work);
	ws_duration = offset + (work_delay / 1000);
	__pm_wakeup_event(battery->input_ws, jiffies_to_msecs(HZ * ws_duration));
	queue_delayed_work(battery->monitor_wqueue,
		&battery->input_check_work, msecs_to_jiffies(work_delay));
}

static void sec_bat_cancel_input_check_work(struct sec_battery_info *battery)
{
	cancel_delayed_work(&battery->input_check_work);
	battery->input_check_cnt = 0;
}

static bool sec_bat_recheck_input_work(struct sec_battery_info *battery)
{
	return !delayed_work_pending(&battery->input_check_work) &&
		(battery->current_event & SEC_BAT_CURRENT_EVENT_SELECT_PDO);
}

__visible_for_testing int sec_bat_change_iv(void *data, int voltage)
{
	struct sec_battery_info *battery = data;

	if ((is_hv_wire_type(battery->cable_type) || is_hv_wire_type(battery->wire_status)) &&
		(battery->cable_type != SEC_BATTERY_CABLE_QC30)) {
		/* no need to set afc prepare current in already 9v cable status */
		if (!is_hv_wire_type(battery->wire_status) ||
			voltage != SEC_INPUT_VOLTAGE_9V)
			sec_bat_check_afc_input_current(battery);
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if IS_ENABLED(CONFIG_MTK_CHARGER) && IS_ENABLED(CONFIG_AFC_CHARGER)
		afc_set_voltage(voltage);
#else
		muic_afc_request_voltage(AFC_REQUEST_CHARGER, voltage/1000);
#endif
#if defined(CONFIG_BC12_DEVICE)
		battery->input_voltage = voltage;
#endif
#endif
	} else if (is_pd_wire_type(battery->cable_type) ||
			(is_pd_wire_type(battery->wire_status) && is_slate_mode(battery))) {
		int curr_pdo = 0, pdo = 0, iv = 0, icl = 0;

		if (voltage == SEC_INPUT_VOLTAGE_APDO) {
			pr_info("%s : Doesn't control input voltage during Direct Charging\n", __func__);
			return voltage;
		}

		iv = voltage;
		if (sec_pd_get_pdo_power(&pdo, &iv, &iv, &icl) <= 0) {
			pr_err("%s: failed to get pdo\n", __func__);
			return -1;
		}
		pr_info("%s: target pdo = %d, iv = %d, icl = %d\n", __func__, pdo, iv, icl);

		if (sec_pd_get_current_pdo(&curr_pdo) < 0) {
			pr_err("%s: failed to get current pdo\n", __func__);
			return -1;
		}

		if (curr_pdo != pdo) {
			/* change input current before request new pdo if new pdo's input current is less than now */
			sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SELECT_PDO,
				SEC_BAT_CURRENT_EVENT_SELECT_PDO);
			sec_vote(battery->input_vote, VOTER_SELECT_PDO, true,
				min(battery->sink_status.power_list[pdo].max_current,
				battery->sink_status.power_list[curr_pdo].max_current));
			sec_bat_run_input_check_work(battery, 1000);
#if !defined(CONFIG_BC12_DEVICE)
			battery->pdic_ps_rdy = false;
#endif
		}

		if (sec_pd_is_apdo(pdo))
			sec_pd_select_pps(pdo, voltage, icl);
		else
			sec_pd_select_pdo(pdo);
	}

	return voltage;
}
EXPORT_SYMBOL_KUNIT(sec_bat_change_iv);

void sec_bat_set_misc_event(struct sec_battery_info *battery,
	unsigned int misc_event_val, unsigned int misc_event_mask)
{

	unsigned int temp = battery->misc_event;

	mutex_lock(&battery->misclock);

	battery->misc_event &= ~misc_event_mask;
	battery->misc_event |= misc_event_val;

	pr_info("%s: misc event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->misc_event);

	mutex_unlock(&battery->misclock);

	if (battery->prev_misc_event != battery->misc_event) {
		cancel_delayed_work(&battery->misc_event_work);
		__pm_stay_awake(battery->misc_event_ws);
		queue_delayed_work(battery->monitor_wqueue,
				&battery->misc_event_work, 0);
	}
}
EXPORT_SYMBOL(sec_bat_set_misc_event);

void sec_bat_set_tx_event(struct sec_battery_info *battery,
	unsigned int tx_event_val, unsigned int tx_event_mask)
{

	unsigned int temp = battery->tx_event;

	mutex_lock(&battery->txeventlock);

	battery->tx_event &= ~tx_event_mask;
	battery->tx_event |= tx_event_val;

	pr_info("@Tx_Mode %s: tx event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->tx_event);

	if (temp != battery->tx_event) {
		/* Assure receiving tx_event to App for sleep case */
		__pm_wakeup_event(battery->tx_event_ws, jiffies_to_msecs(HZ * 2));
		power_supply_changed(battery->psy_bat);
	}
	mutex_unlock(&battery->txeventlock);
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_tx_event);

void sec_bat_set_current_event(struct sec_battery_info *battery,
				unsigned int current_event_val, unsigned int current_event_mask)
{
	unsigned int temp = battery->current_event;

	mutex_lock(&battery->current_eventlock);

	battery->current_event &= ~current_event_mask;
	battery->current_event |= current_event_val;

	pr_info("%s: current event before(0x%x), after(0x%x)\n",
		__func__, temp, battery->current_event);

	mutex_unlock(&battery->current_eventlock);
}
EXPORT_SYMBOL(sec_bat_set_current_event);

void sec_bat_set_temp_control_test(struct sec_battery_info *battery, bool temp_enable)
{
	if (temp_enable) {
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST) {
			pr_info("%s : BATT_TEMP_CONTROL_TEST already ENABLED\n", __func__);
			return;
		}
		pr_info("%s : BATT_TEMP_CONTROL_TEST ENABLE\n", __func__);
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST,
			SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST);
		battery->pdata->usb_temp_check_type_backup = battery->pdata->usb_thm_info.check_type;
		battery->pdata->usb_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
		battery->overheatlimit_threshold_backup = battery->overheatlimit_threshold;
		battery->overheatlimit_threshold = 990;
		battery->overheatlimit_recovery_backup = battery->overheatlimit_recovery;
		battery->overheatlimit_recovery = 980;
	} else {
		if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST)) {
			pr_info("%s : BATT_TEMP_CONTROL_TEST already END\n", __func__);
			return;
		}
		pr_info("%s : BATT_TEMP_CONTROL_TEST END\n", __func__);
		sec_bat_set_current_event(battery, 0,
			SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST);
		battery->pdata->usb_thm_info.check_type = battery->pdata->usb_temp_check_type_backup;
		battery->overheatlimit_threshold = battery->overheatlimit_threshold_backup;
		battery->overheatlimit_recovery = battery->overheatlimit_recovery_backup;
	}
}
EXPORT_SYMBOL(sec_bat_set_temp_control_test);

void sec_bat_change_default_current(struct sec_battery_info *battery,
					int cable_type, int input, int output)
{
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if (!battery->test_max_current)
#endif
		battery->pdata->charging_current[cable_type].input_current_limit = input;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	if (!battery->test_charge_current)
#endif
		battery->pdata->charging_current[cable_type].fast_charging_current = output;
	pr_info("%s: cable_type: %d(%d,%d), input: %d, output: %d\n",
			__func__, cable_type, battery->cable_type, battery->wire_status,
			battery->pdata->charging_current[cable_type].input_current_limit,
			battery->pdata->charging_current[cable_type].fast_charging_current);
}
EXPORT_SYMBOL_KUNIT(sec_bat_change_default_current);

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
#if defined(CONFIG_SEC_FACTORY)
static bool sec_bat_usb_factory_set_vote(struct sec_battery_info *battery, bool vote_en)
{
	if (vote_en) {
		if ((battery->cable_type == SEC_BATTERY_CABLE_USB) &&
				!(battery->batt_f_mode == IB_MODE) &&
				!sec_bat_get_lpmode())
			if (battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL_NONE) {
				sec_vote(battery->fcc_vote, VOTER_USB_FAC_100MA, true, 100);
				sec_vote(battery->input_vote, VOTER_USB_FAC_100MA, true, 100);
				dev_info(battery->dev, "%s: usb factory 100mA\n", __func__);
				return true;
			}
	} else {
		if ((battery->cable_type == SEC_BATTERY_CABLE_USB) &&
			!(battery->batt_f_mode == IB_MODE) && !sec_bat_get_lpmode())
			if (battery->sink_status.rp_currentlvl != RP_CURRENT_LEVEL_NONE) {
				sec_vote(battery->fcc_vote, VOTER_USB_FAC_100MA, false, 100);
				sec_vote(battery->input_vote, VOTER_USB_FAC_100MA, false, 100);
				dev_info(battery->dev, "%s: recover usb factory 100mA\n", __func__);
			}
	}
	return false;
}
#endif

static void sec_bat_usb_factory_clear(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };

#if defined(CONFIG_SEC_FACTORY)
	if (battery->usb_factory_slate_mode || (battery->batt_f_mode == IB_MODE)) {
		if (is_slate_mode(battery)) {
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SLATE);
			sec_vote(battery->chgen_vote, VOTER_SLATE, false, 0);
			sec_vote(battery->chgen_vote, VOTER_SMART_SLATE, false, 0);
			sec_bat_set_mfc_on(battery, WPC_EN_SLATE);
			dev_info(battery->dev, "%s: disable slate mode\n", __func__);
		}
		battery->usb_factory_slate_mode = false;
	}
#endif
	if (battery->batt_f_mode == IB_MODE) {
		val.intval = 0;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_IB_MODE, val);
	} else if (battery->batt_f_mode == OB_MODE) {
		val.intval = 0;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_OB_MODE_CABLE_REMOVED, val);
	}
}
#endif

#if !defined(CONFIG_SEC_FACTORY)
#if defined(CONFIG_SUPPORT_HV_CTRL)
static int sec_bat_chk_siop_scenario_idx(struct sec_battery_info *battery,
	int siop_level);
static bool sec_bat_chk_siop_skip_scenario(struct sec_battery_info *battery,
	int ct, int ws, int scenario_idx);

__visible_for_testing bool sec_bat_change_vbus_condition(int ct, unsigned int evt)
{
	if (!(is_hv_wire_type(ct) || is_pd_wire_type(ct)) ||
			(ct == SEC_BATTERY_CABLE_QC30))
		return false;

	if ((evt & SEC_BAT_CURRENT_EVENT_AFC) ||
			(evt & SEC_BAT_CURRENT_EVENT_SELECT_PDO))
		return false;
	return true;
}
EXPORT_SYMBOL_KUNIT(sec_bat_change_vbus_condition);

__visible_for_testing bool sec_bat_change_vbus(struct sec_battery_info *battery,
		int ct, unsigned int evt, int siop_level)
{
	int s_idx = -1;

	if (battery->pdata->chg_thm_info.check_type == SEC_BATTERY_TEMP_CHECK_NONE ||
			battery->store_mode ||
			((battery->siop_level == 80) && is_wired_type(battery->cable_type)))
		return false;
	if (!sec_bat_change_vbus_condition(ct, evt))
		return false;

	s_idx = sec_bat_chk_siop_scenario_idx(battery, battery->siop_level);

	if ((siop_level >= 100) ||
		sec_bat_chk_siop_skip_scenario(battery,
			battery->cable_type, battery->wire_status, s_idx))
		sec_vote(battery->iv_vote, VOTER_SIOP, false, 0);
	else {
		sec_vote(battery->iv_vote, VOTER_SIOP, true, SEC_INPUT_VOLTAGE_5V);
		pr_info("%s: vbus set 5V by level(%d), Cable(%s, %s, %d, %d)\n",
				__func__, battery->siop_level,
				sb_get_ct_str(ct), sb_get_ct_str(battery->wire_status),
				battery->muic_cable_type, battery->pd_usb_attached);
		return true;
	}
	return false;
}
EXPORT_SYMBOL_KUNIT(sec_bat_change_vbus);
#else
__visible_for_testing bool sec_bat_change_vbus(struct sec_battery_info *battery,
		int ct, unsigned int evt, int siop_level)
{
		return false;
}
EXPORT_SYMBOL_KUNIT(sec_bat_change_vbus);
#endif
#endif

__visible_for_testing int sec_bat_check_afc_input_current(struct sec_battery_info *battery)
{
	int work_delay = 0;
	int input_current;

	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AFC,
			(SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));
	if (!is_wireless_type(battery->cable_type)) {
		input_current = battery->pdata->pre_afc_input_current; // 1000mA
		work_delay = battery->pdata->pre_afc_work_delay;
	} else {
		input_current = battery->pdata->pre_wc_afc_input_current;
		/* do not reduce this time, this is for noble pad */
		work_delay = battery->pdata->pre_wc_afc_work_delay;
	}
	sec_vote(battery->input_vote, VOTER_VBUS_CHANGE, true, input_current);

	if (!delayed_work_pending(&battery->input_check_work))
		sec_bat_run_input_check_work(battery, work_delay);

	pr_info("%s: change input_current(%d), cable_type(%d)\n", __func__, input_current, battery->cable_type);

	return input_current;
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_afc_input_current);

__visible_for_testing void sec_bat_get_input_current_in_power_list(struct sec_battery_info *battery)
{
	int pdo_num = battery->sink_status.current_pdo_num;
	int max_input_current = 0;

	if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo)
		pdo_num = 1;

	max_input_current = battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].input_current_limit =
		battery->sink_status.power_list[pdo_num].max_current;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].input_current_limit =
		battery->sink_status.power_list[pdo_num].max_current;

	if (battery->is_fpdo_dc && !(battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE)) {
		max_input_current =
			battery->pdata->charging_current[SEC_BATTERY_CABLE_FPDO_DC].input_current_limit =
			mA_by_mWmV(battery->pdata->pd_charging_charge_power,
					battery->sink_status.power_list[pdo_num].max_voltage);
		battery->pdata->charging_current[SEC_BATTERY_CABLE_FPDO_DC].fast_charging_current =
			battery->sink_status.power_list[pdo_num].max_current * 2;
	}

	pr_info("%s:max_input_current : %dmA, pdo_num : %d\n", __func__, max_input_current, pdo_num);
	sec_vote(battery->input_vote, VOTER_CABLE, true, max_input_current);
}
EXPORT_SYMBOL_KUNIT(sec_bat_get_input_current_in_power_list);

__visible_for_testing void sec_bat_get_charging_current_in_power_list(struct sec_battery_info *battery)
{
	int max_charging_current = 0, pd_power = 0;
	int pdo_num = battery->sink_status.current_pdo_num;

	if (is_pd_apdo_wire_type(battery->wire_status) && battery->pd_list.now_isApdo)
		pdo_num = 1;

	pd_power = mW_by_mVmA(battery->sink_status.power_list[pdo_num].max_voltage,
		battery->sink_status.power_list[pdo_num].max_current);

	/* We assume that output voltage to float voltage */
	max_charging_current = mA_by_mWmV(pd_power,
			(battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv));
	max_charging_current = max_charging_current > battery->pdata->max_charging_current ?
		battery->pdata->max_charging_current : max_charging_current;
	battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC].fast_charging_current = max_charging_current;

#if defined(CONFIG_STEP_CHARGING)
	if (battery->dchg_dc_in_swelling && (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)) {
		pr_info("%s: max cc(%d) warm_cc(%d)\n", __func__,
			max_charging_current, battery->pdata->wire_warm_current);
		max_charging_current = max(max_charging_current, battery->pdata->wire_warm_current);
	}
	if (is_pd_apdo_wire_type(battery->wire_status) && !battery->pd_list.now_isApdo &&
		battery->step_chg_status < 0)
#else
	if (is_pd_apdo_wire_type(battery->wire_status) && !battery->pd_list.now_isApdo)
#endif
		battery->pdata->charging_current[SEC_BATTERY_CABLE_PDIC_APDO].fast_charging_current =
			max_charging_current;
	battery->charge_power = pd_power;

	pr_info("%s:pd_charge_power : %dmW, max_charging_current : %dmA\n", __func__,
		battery->charge_power, max_charging_current);
	sec_vote(battery->fcc_vote, VOTER_CABLE, true,
			battery->pdata->charging_current[battery->wire_status].fast_charging_current);
}
EXPORT_SYMBOL_KUNIT(sec_bat_get_charging_current_in_power_list);

#if !defined(CONFIG_SEC_FACTORY)
void sec_bat_check_temp_ctrl_by_cable(struct sec_battery_info *battery)
{
	int ct = battery->cable_type;
	int siop_lvl = battery->siop_level;
	bool is_apdo = false;

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	is_apdo = (is_pd_apdo_wire_type(ct) && battery->pd_list.now_isApdo) ? 1 : 0;
#endif

	if (battery->pdata->enable_mix_v2)
		sec_bat_check_mix_temp_v2(battery);
	else
		sec_bat_check_mix_temp(battery, ct, siop_lvl, is_apdo);

	if (is_apdo) {
		sec_bat_check_direct_chg_temp(battery, siop_lvl);
	} else if (is_wireless_type(ct)) {
		if (battery->pdata->enable_check_wpc_temp_v2)
			sec_bat_check_wpc_temp_v2(battery);
		else
			sec_bat_check_wpc_temp(battery, ct, siop_lvl);
	} else {
		if (!sec_bat_change_vbus(battery, ct, battery->current_event, siop_lvl)) {
			if (is_pd_wire_type(ct))
				sec_bat_check_pdic_temp(battery, siop_lvl);
			else {
				sec_bat_check_afc_temp(battery, siop_lvl);
			}
		} else if (battery->chg_limit) {
			sec_vote(battery->fcc_vote, VOTER_CHG_TEMP, false, 0);
			sec_vote(battery->input_vote, VOTER_CHG_TEMP, false, 0);
			battery->chg_limit = false;
		}
	}
	if (!is_wireless_all_type(ct))
		sec_bat_check_lrp_temp(battery,
			ct, battery->wire_status, siop_lvl, battery->lcd_status);

	pr_info("%s: ct : %s\n", __func__, sb_get_ct_str(ct));
}
#endif
int sec_bat_set_charging_current(struct sec_battery_info *battery)
{
	mutex_lock(&battery->iolock);

	if (!is_nocharge_type(battery->cable_type)) {
#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_check_temp_ctrl_by_cable(battery);
#endif

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		/* Calculate wireless input current under the specific conditions (wpc_sleep_mode, chg_limit)*/
		/* VOTER_WPC_CUR */
		if (battery->wc_status != SEC_BATTERY_CABLE_NONE) {
			sec_bat_get_wireless_current(battery);
		}
#endif

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		if (battery->dc_float_voltage_set) {
			int age_step = battery->pdata->age_step;
			int chg_step = battery->step_chg_status;

			pr_info("%s : step float voltage = %d\n", __func__,
					battery->pdata->dc_step_chg_val_vfloat[age_step][chg_step]);
			sec_vote(battery->dc_fv_vote, VOTER_DC_STEP_CHARGE, true,
				battery->pdata->dc_step_chg_val_vfloat[age_step][chg_step]);
			battery->dc_float_voltage_set = false;
		}
#endif

		/* check topoff current */
		if (battery->charging_mode == SEC_BATTERY_CHARGING_2ND &&
				(battery->pdata->full_check_type_2nd == SEC_BATTERY_FULLCHARGED_CHGPSY ||
				battery->pdata->full_check_type_2nd == SEC_BATTERY_FULLCHARGED_LIMITER)) {
			sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, true, battery->pdata->full_check_current_2nd);
		}

	}/* !is_nocharge_type(battery->cable_type) */
	mutex_unlock(&battery->iolock);
	return 0;
}
EXPORT_SYMBOL(sec_bat_set_charging_current);

int sec_bat_set_charge(void * data, int chg_mode)
{
	struct sec_battery_info *battery = data;
	union power_supply_propval val = {0, };
	struct timespec64 ts = {0, };

	if (chg_mode == SEC_BAT_CHG_MODE_NOT_SET) {
		pr_info("%s: temp mode for decrease 5v\n", __func__);
		return chg_mode;
	}

	if ((battery->current_event & SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE) &&
		(chg_mode == SEC_BAT_CHG_MODE_CHARGING)) {
		dev_info(battery->dev, "%s: charge disable by HMT\n", __func__);
		chg_mode = SEC_BAT_CHG_MODE_CHARGING_OFF;
	}

	battery->charger_mode = chg_mode;
	pr_info("%s set %s mode\n", __func__, sb_charge_mode_str(chg_mode));

	val.intval = battery->status;
	psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_STATUS, val);
	ts = ktime_to_timespec64(ktime_get_boottime());

	if (chg_mode == SEC_BAT_CHG_MODE_CHARGING) {
		/*Reset charging start time only in initial charging start */
		if (battery->charging_start_time == 0) {
			if (ts.tv_sec < 1)
				ts.tv_sec = 1;
			battery->charging_start_time = ts.tv_sec;
			battery->charging_next_time =
				battery->pdata->charging_reset_time;
		}
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		if (is_pd_apdo_wire_type(battery->cable_type)) {
			sec_bat_reset_step_charging(battery);
			sec_bat_check_dc_step_charging(battery);
		}
#endif
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
		if (is_wireless_type(battery->cable_type)) {
			val.intval = EXIT_PHM;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_RX_PHM, val);
		}
#endif
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_EXT_PROP_WIRELESS_OP_MODE, val);
		pr_info("%s: @mpp : op_mode %d\n", __func__, val.intval);
		if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP &&
			val.intval == MFC_RX_MODE_WPC_MPP_CLOAK) {
			val.intval = CLOAK_EXIT_CMD;
			psy_do_property("wireless", set,
				POWER_SUPPLY_EXT_PROP_MPP_CLOAK, val);
		}
	} else {
		battery->charging_start_time = 0;
		battery->charging_passed_time = 0;
		battery->charging_next_time = 0;
		battery->charging_fullcharged_time = 0;
		battery->full_check_cnt = 0;
#if defined(CONFIG_STEP_CHARGING)
		sec_bat_reset_step_charging(battery);
#endif
		battery->usb_overheat_check = false;
		battery->cisd.ab_vbat_check_count = 0;
	}

	val.intval = chg_mode;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED, val);
	val.intval = chg_mode;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED, val);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	if ((battery->pdata->full_check_type_2nd == SEC_BATTERY_FULLCHARGED_FG_CURRENT) &&
		(battery->charging_mode == SEC_BATTERY_CHARGING_NONE && battery->status == POWER_SUPPLY_STATUS_FULL)) {
		/*
		 * Enable supplement mode for 2nd charging done, should cut off charger then limiter sequence,
		 * this case only for full_check_type_2nd is
		 * SEC_BATTERY_FULLCHARGED_FG_CURRENT not SEC_BATTERY_FULLCHARGED_LIMITER
		 */
		val.intval = SEC_DUAL_BATTERY_CHG_OFF;
		psy_do_property(battery->pdata->dual_battery_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED, val);
	} else if (!(battery->charging_mode == SEC_BATTERY_CHARGING_NONE && battery->status == POWER_SUPPLY_STATUS_FULL) &&
				!(battery->charging_mode == SEC_BATTERY_CHARGING_NONE && battery->thermal_zone == BAT_THERMAL_WARM)) {
		/*
		 * Limiter should disable supplement mode to do battery balancing properly in case of
		 * charging, discharging and buck off. But needs to disable supplement mode except
		 * 2nd full charge done and swelling charging.
		 */
#ifdef CONFIG_IFPMIC_LIMITER
		if (chg_mode == SEC_BAT_CHG_MODE_BUCK_OFF)
			val.intval = SEC_DUAL_BATTERY_BUCK_OFF;
		else if (chg_mode == SEC_BAT_CHG_MODE_CHARGING)
			val.intval = SEC_DUAL_BATTERY_CHG_ON;
		else if (chg_mode == SEC_BAT_CHG_MODE_PASS_THROUGH) {
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_EXT_PROP_CHARGER_MODE_DIRECT, val);
			if (val.intval == SEC_BAT_CHG_MODE_PASS_THROUGH) // dc pass_through
				val.intval = SEC_DUAL_BATTERY_CHG_ON;
			else // sc pass_through
				val.intval = SEC_DUAL_BATTERY_CHG_OFF;
		} else {
			if (battery->status == POWER_SUPPLY_STATUS_FULL)
				return chg_mode;
			val.intval = SEC_DUAL_BATTERY_CHG_OFF;
		}
#else
		val.intval = SEC_DUAL_BATTERY_CHG_ON;
#endif
		psy_do_property(battery->pdata->dual_battery_name, set,
			POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED, val);
	}
#endif
	return chg_mode;
}
EXPORT_SYMBOL(sec_bat_set_charge);

__visible_for_testing bool sec_bat_check_by_psy(struct sec_battery_info *battery)
{
	char *psy_name = NULL;
	union power_supply_propval value = {0, };
	bool ret = true;

	switch (battery->pdata->battery_check_type) {
	case SEC_BATTERY_CHECK_PMIC:
		psy_name = battery->pdata->pmic_name;
		break;
	case SEC_BATTERY_CHECK_FUELGAUGE:
		psy_name = battery->pdata->fuelgauge_name;
		break;
	case SEC_BATTERY_CHECK_CHARGER:
		psy_name = battery->pdata->charger_name;
		break;
	default:
		dev_err(battery->dev, "%s: Invalid Battery Check Type\n", __func__);
		ret = false;
		goto battery_check_error;
		break;
	}

	psy_do_property(psy_name, get, POWER_SUPPLY_PROP_PRESENT, value);
	ret = (bool)value.intval;

battery_check_error:
	return ret;
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_by_psy);

#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_LIMITER_S2ASL01)
static bool sec_bat_check_by_gpio(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	bool ret = true;
	int main_det = -1, sub_det = -1;

	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->dual_battery_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_BAT_DET, value);
	main_det = value.intval;

	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->dual_battery_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_BAT_DET, value);
	sub_det = value.intval;

	ret = (bool)(main_det & sub_det);
	if (!ret)
		pr_info("%s : main det = %d, sub det = %d\n", __func__, main_det, sub_det);

	return ret;
}

#if !defined(CONFIG_SEC_FACTORY)
static void sec_bat_powerpath_check(struct sec_battery_info *battery, int m_health, int s_health)
{
	static int m_abnormal_cnt, s_abnormal_cnt;
	bool m_supplement_status = false, s_supplement_status = false;
	union power_supply_propval value = {0, };

	/* do not check battery power path when limiter is not ok */
	if (m_health != POWER_SUPPLY_HEALTH_GOOD ||
		s_health != POWER_SUPPLY_HEALTH_GOOD) {
		pr_info("%s : do not check current status\n", __func__);
		return;
	}

	/* get main limiter's supplement status */
	psy_do_property(battery->pdata->main_limiter_name, get,
		POWER_SUPPLY_EXT_PROP_SUPLLEMENT_MODE, value);
	m_supplement_status = value.intval;

	/* get sub limiter's supplement status */
	psy_do_property(battery->pdata->sub_limiter_name, get,
		POWER_SUPPLY_EXT_PROP_SUPLLEMENT_MODE, value);
	s_supplement_status = value.intval;

	if ((battery->current_avg_main == 0) &&
		!m_supplement_status &&
		!(battery->misc_event & BATT_MISC_EVENT_MAIN_POWERPATH)) {
		m_abnormal_cnt++;
	} else if (battery->current_avg_main != 0) {
		if (battery->misc_event & BATT_MISC_EVENT_MAIN_POWERPATH) {
			pr_info("%s : main power path get back to normal\n", __func__);
			sec_bat_set_misc_event(battery,
				0, BATT_MISC_EVENT_MAIN_POWERPATH);
		}
		m_abnormal_cnt = 0;
	}

	if ((battery->current_avg_sub == 0) &&
		!s_supplement_status &&
		!(battery->misc_event & BATT_MISC_EVENT_SUB_POWERPATH)) {
		s_abnormal_cnt++;
	} else if (battery->current_avg_sub != 0) {
		if (battery->misc_event & BATT_MISC_EVENT_SUB_POWERPATH) {
			pr_info("%s : sub power path get back to normal\n", __func__);
			sec_bat_set_misc_event(battery,
				0, BATT_MISC_EVENT_SUB_POWERPATH);
		}
		s_abnormal_cnt = 0;
	}

	if (m_abnormal_cnt > 5) {
		pr_info("%s : main power path seems to have problem\n", __func__);
		sec_bat_set_misc_event(battery,
			BATT_MISC_EVENT_MAIN_POWERPATH, BATT_MISC_EVENT_MAIN_POWERPATH);
		m_abnormal_cnt = 0;
	}

	if (s_abnormal_cnt > 5) {
		pr_info("%s : sub power path seems to have problem\n", __func__);
		sec_bat_set_misc_event(battery,
			BATT_MISC_EVENT_SUB_POWERPATH, BATT_MISC_EVENT_SUB_POWERPATH);
		s_abnormal_cnt = 0;
	}
}

static void sec_bat_limiter_check(struct sec_battery_info *battery)
{
	union power_supply_propval m_value = {0, }, s_value = {0, };
	int main_enb, main_enb2, sub_enb;

	pr_info("%s: Start\n", __func__);

	/* do not check limiter status when enb is not active status since it is certain test mode using enb pin */
	main_enb = gpio_get_value(battery->pdata->main_bat_enb_gpio);
	main_enb2 = gpio_get_value(battery->pdata->main_bat_enb2_gpio);
	sub_enb = gpio_get_value(battery->pdata->sub_bat_enb_gpio);

	if (main_enb2 || sub_enb) {
		pr_info("%s : main_enb = %d, main_enb2 = %d, sub_enb = %d\n", __func__, main_enb, main_enb2, sub_enb);
		return;
	}

	/* check powermeter and vchg, vbat value of main limiter */
	psy_do_property(battery->pdata->main_limiter_name, get,
		POWER_SUPPLY_PROP_HEALTH, m_value);

	/* check powermeter and vchg, vbat value of sub limiter */
	psy_do_property(battery->pdata->sub_limiter_name, get,
		POWER_SUPPLY_PROP_HEALTH, s_value);

	/* discharging case with non sleep current since it has low powermeter resolution */
	if (is_nocharge_type(battery->cable_type) &&
		is_nocharge_type(battery->wire_status) &&
		battery->current_avg < (-500) &&
		!is_slate_mode(battery))
		sec_bat_powerpath_check(battery, m_value.intval, s_value.intval);

	/* do not check limiter status input curruent is not set fully */
	if (is_nocharge_type(battery->cable_type) ||
		is_wireless_all_type(battery->cable_type) ||
		battery->health != POWER_SUPPLY_HEALTH_GOOD ||
		battery->status != POWER_SUPPLY_STATUS_CHARGING ||
		battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE ||
		battery->current_event & SEC_BAT_CURRENT_EVENT_AICL ||
		battery->charge_power < 9000 ||
		battery->siop_level < 100 ||
		battery->lcd_status ||
		battery->limiter_check) {
		return;
	}

	if (m_value.intval != POWER_SUPPLY_HEALTH_GOOD) {
		pr_info("%s : main limiter wa will work\n", __func__);
		battery->cisd.event_data[EVENT_MAIN_BAT_ERR]++;
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

		m_value.intval = 1000;
		psy_do_property(battery->pdata->main_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_DISCHG_LIMIT_CURRENT, m_value);

		m_value.intval = 0;
		psy_do_property(battery->pdata->main_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_DISCHG_MODE, m_value);

		/* deactivate main limiter */
		gpio_direction_output(battery->pdata->main_bat_enb2_gpio, 1);
		usleep_range(1000, 2000);
		/* activate main limiter */
		gpio_direction_output(battery->pdata->main_bat_enb2_gpio, 0);
		msleep(50);

		/* needs to init limiter setting again */
		m_value.intval = 1;
		psy_do_property(battery->pdata->main_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_POWERMETER_ENABLE, m_value);
		msleep(100);

		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING);
		battery->limiter_check = true;
		pr_info("%s : main limiter wa done\n", __func__);

		/* re-check powermeter and vchg, vbat value of main limiter */
		psy_do_property(battery->pdata->main_limiter_name, get,
			POWER_SUPPLY_PROP_HEALTH, m_value);

		if (m_value.intval != POWER_SUPPLY_HEALTH_GOOD) {
			pr_info("%s : main limiter wa did not work\n", __func__);
			battery->cisd.event_data[EVENT_BAT_WA_ERR]++;
		}
#if IS_ENABLED(CONFIG_SEC_ABC)
		sec_abc_send_event("MODULE=battery@WARN=lim_stuck");
#endif
	}

	if (s_value.intval != POWER_SUPPLY_HEALTH_GOOD) {
		pr_info("%s : sub limiter wa will work\n", __func__);
		battery->cisd.event_data[EVENT_SUB_BAT_ERR]++;
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

		s_value.intval = 1000;
		psy_do_property(battery->pdata->sub_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_DISCHG_LIMIT_CURRENT, s_value);

		s_value.intval = 0;
		psy_do_property(battery->pdata->sub_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_DISCHG_MODE, s_value);

		/* deactivate sub limiter */
		gpio_direction_output(battery->pdata->sub_bat_enb_gpio, 1);
		usleep_range(1000, 2000);
		/* activate sub limiter */
		gpio_direction_output(battery->pdata->sub_bat_enb_gpio, 0);
		msleep(50);

		/* needs to init limiter setting again */
		s_value.intval = 1;
		psy_do_property(battery->pdata->sub_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_POWERMETER_ENABLE, s_value);
		msleep(100);

		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING);

		battery->limiter_check = true;
		pr_info("%s : sub limiter wa done\n", __func__);

		/* re-check powermeter and vchg, vbat value of sub limiter */
		psy_do_property(battery->pdata->sub_limiter_name, get,
			POWER_SUPPLY_PROP_HEALTH, s_value);

		if (s_value.intval != POWER_SUPPLY_HEALTH_GOOD) {
			pr_info("%s : main limiter wa did not work\n", __func__);
			battery->cisd.event_data[EVENT_BAT_WA_ERR]++;
		}
#if IS_ENABLED(CONFIG_SEC_ABC)
		sec_abc_send_event("MODULE=battery@WARN=lim_stuck");
#endif
	}
}
#endif
#endif

static bool sec_bat_check(struct sec_battery_info *battery)
{
	bool ret = true;

	if (battery->factory_mode || battery->is_jig_on || sec_bat_get_facmode()) {
		dev_dbg(battery->dev, "%s: No need to check in factory mode\n",
			__func__);
		return ret;
	}

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return ret;
	}

	switch (battery->pdata->battery_check_type) {
	case SEC_BATTERY_CHECK_ADC:
		if (is_nocharge_type(battery->cable_type))
			ret = battery->present;
		else
			ret = sec_bat_check_vf_adc(battery);
		break;
	case SEC_BATTERY_CHECK_INT:
	case SEC_BATTERY_CHECK_CALLBACK:
		if (is_nocharge_type(battery->cable_type)) {
			ret = battery->present;
		} else {
			if (battery->pdata->check_battery_callback)
				ret = battery->pdata->check_battery_callback();
		}
		break;
	case SEC_BATTERY_CHECK_PMIC:
	case SEC_BATTERY_CHECK_FUELGAUGE:
	case SEC_BATTERY_CHECK_CHARGER:
		ret = sec_bat_check_by_psy(battery);
		break;
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_LIMITER_S2ASL01)
	case SEC_BATTERY_CHECK_DUAL_BAT_GPIO:
		ret = sec_bat_check_by_gpio(battery);
		break;
#endif
	case SEC_BATTERY_CHECK_NONE:
		dev_dbg(battery->dev, "%s: No Check\n", __func__);
		break;
	default:
		break;
	}

	return ret;
}

static void sec_bat_send_cs100(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (is_wireless_all_type(battery->cable_type)) {
		value.intval = POWER_SUPPLY_STATUS_FULL;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_STATUS, value);
	}
}

__visible_for_testing bool sec_bat_get_cable_type(struct sec_battery_info *battery, int cable_source_type)
{
	bool ret = false;
	int cable_type = battery->cable_type;

	if (cable_source_type & SEC_BATTERY_CABLE_SOURCE_CALLBACK) {
		if (battery->pdata->check_cable_callback)
			cable_type = battery->pdata->check_cable_callback();
	}

	if (cable_source_type & SEC_BATTERY_CABLE_SOURCE_ADC) {
		if (gpio_get_value_cansleep(
			battery->pdata->bat_gpio_ta_nconnected) ^
			battery->pdata->bat_polarity_ta_nconnected)
			cable_type = SEC_BATTERY_CABLE_NONE;
		else
			cable_type = sec_bat_get_charger_type_adc(battery);
	}

	if (battery->cable_type == cable_type) {
		dev_dbg(battery->dev, "%s: No need to change cable status\n", __func__);
	} else {
		if (cable_type < SEC_BATTERY_CABLE_NONE || cable_type >= SEC_BATTERY_CABLE_MAX) {
			dev_err(battery->dev, "%s: Invalid cable type\n", __func__);
		} else {
			battery->cable_type = cable_type;
			if (battery->pdata->check_cable_result_callback)
				battery->pdata->check_cable_result_callback(battery->cable_type);

			ret = true;

			dev_dbg(battery->dev, "%s: Cable Changed (%d)\n", __func__, battery->cable_type);
		}
	}

	return ret;
}
EXPORT_SYMBOL_KUNIT(sec_bat_get_cable_type);

int sec_bat_get_batt_capacity(struct sec_battery_info *battery, int old_capacity)
{
	union power_supply_propval value = {0, };
	int ret = 0;

	ret = psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_CAPACITY, value);
	if (ret < 0)
		return old_capacity;

	return value.intval;
}
EXPORT_SYMBOL(sec_bat_get_batt_capacity);

void sec_bat_set_charging_status(struct sec_battery_info *battery, int status)
{
	union power_supply_propval value = {0, };

#if defined(CONFIG_NO_BATTERY)
	status = POWER_SUPPLY_STATUS_DISCHARGING;
#endif
	switch (status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		if (battery->siop_level < 100 || battery->lcd_status || battery->wc_tx_enable)
			battery->safety_timer.stop_timer = true;
		if ((battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) && is_wireless_all_type(battery->cable_type)) {
			value.intval = POWER_SUPPLY_STATUS_CHARGING;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_STATUS, value);
		}
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if (battery->status == POWER_SUPPLY_STATUS_FULL &&
			battery->capacity < 100 && is_eu_eco_rechg(battery->fs)) {

			pr_info("%s : EU eco case do not Fg scale, capacity(%d)\n", __func__, battery->capacity);

		} else if ((battery->status == POWER_SUPPLY_STATUS_FULL ||
			(battery->capacity == 100 && !is_slate_mode(battery))) &&
			!battery->store_mode) {

			pr_info("%s : Update fg scale to 101%%\n", __func__);
			value.intval = 100;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_PROP_CHARGE_FULL, value);

			/* To get SOC value (NOT raw SOC), need to reset value */
			battery->capacity = sec_bat_get_batt_capacity(battery, battery->capacity);
		}
		sec_bat_reset_safety_timer(battery);
		break;
	case POWER_SUPPLY_STATUS_FULL:
		sec_bat_send_cs100(battery);
		break;
	default:
		break;
	}
	battery->status = status;
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_charging_status);

void sec_bat_set_health(struct sec_battery_info *battery, int health)
{
	if (battery->health != health) {
		if (health == POWER_SUPPLY_EXT_HEALTH_OVERHEATLIMIT)
			sec_bat_set_misc_event(battery,
				BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT, BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT);
		else
			sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT);

		if (is_wireless_all_type(battery->cable_type)) {
			union power_supply_propval val = {0, };

			val.intval = health;
			psy_do_property(battery->pdata->wireless_charger_name, set, POWER_SUPPLY_PROP_HEALTH, val);
		}
	}

	battery->health = health;

	if (health != POWER_SUPPLY_HEALTH_GOOD)
		store_battery_log(
			"Health:%d%%,%dmV,%s,ct(%s,%s,%d,%d),%s",
			battery->capacity,
			battery->voltage_now,
			sb_get_bst_str(battery->status),
			sb_get_ct_str(battery->cable_type),
			sb_get_ct_str(battery->wire_status),
			battery->muic_cable_type,
			battery->pd_usb_attached,
			sb_get_hl_str(battery->health)
			);
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_health);

static bool sec_bat_battery_cable_check(struct sec_battery_info *battery)
{
	if (!sec_bat_check(battery)) {
		if (battery->check_count < battery->pdata->check_count)
			battery->check_count++;
		else {
			dev_err(battery->dev,
				"%s: Battery Disconnected\n", __func__);
			battery->present = false;
			sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);

			if (battery->status !=
				POWER_SUPPLY_STATUS_DISCHARGING) {
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_NOT_CHARGING);
				sec_vote(battery->chgen_vote, VOTER_BATTERY, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			}

			if (battery->pdata->check_battery_result_callback)
				battery->pdata->
					check_battery_result_callback();
			return false;
		}
	} else
		battery->check_count = 0;

	battery->present = true;

	if (battery->health == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);

		if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_CHARGING);
			sec_vote(battery->chgen_vote, VOTER_BATTERY, false, 0);
		}
	}

	dev_dbg(battery->dev, "%s: Battery Connected\n", __func__);

	if (battery->pdata->cable_check_type &
		SEC_BATTERY_CABLE_CHECK_POLLING) {
		if (sec_bat_get_cable_type(battery,
			battery->pdata->cable_source_type)) {
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
						&battery->cable_work, 0);
		}
	}
	return true;
}

static int sec_bat_ovp_uvlo_by_psy(struct sec_battery_info *battery)
{
	char *psy_name = NULL;
	union power_supply_propval value = {0, };
	int ret = 0;

	value.intval = POWER_SUPPLY_HEALTH_GOOD;

	switch (battery->pdata->ovp_uvlo_check_type) {
	case SEC_BATTERY_OVP_UVLO_PMICPOLLING:
		psy_name = battery->pdata->pmic_name;
		break;
	case SEC_BATTERY_OVP_UVLO_CHGPOLLING:
		psy_name = battery->pdata->charger_name;
		break;
	default:
		dev_err(battery->dev,
			"%s: Invalid OVP/UVLO Check Type\n", __func__);
		goto ovp_uvlo_check_error;
		break;
	}

	ret = psy_do_property(psy_name, get,
		POWER_SUPPLY_PROP_HEALTH, value);

ovp_uvlo_check_error:
	/* Need to not display HEALTH UNKNOWN if driver cannot be read */
	return ((ret == 0) ? value.intval : POWER_SUPPLY_HEALTH_GOOD);
}
#if defined(CONFIG_SEC_KUNIT)
int __mockable chk_ap_wake_chg(void)
#else
static int chk_ap_wake_chg(void)
#endif
{
	static int en = -1;

	if (en == -1) {
		struct device_node *np;

		np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_info("%s: np NULL\n", __func__);
			en = 0;
		} else {
			en = of_property_read_bool(np, "battery,ap_wake_chg");
		}
		pr_info("%s: en: %d\n", __func__, en);
	}
	return en;
}

__visible_for_testing void sb_set_vbus_wake(struct wakeup_source *vbus_ws, int health, int cable_type)
{
	if (!chk_ap_wake_chg() || chg_can_sleep_type(cable_type) ||
		(health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE) || (health == POWER_SUPPLY_HEALTH_OVERVOLTAGE))
		__pm_wakeup_event(vbus_ws, jiffies_to_msecs(HZ * 10));
	else
		__pm_stay_awake(vbus_ws);
}
EXPORT_SYMBOL_KUNIT(sb_set_vbus_wake);

static bool sec_bat_ovp_uvlo_result(struct sec_battery_info *battery, int health)
{
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	union power_supply_propval value = {0, };
#endif

	if (health == POWER_SUPPLY_EXT_HEALTH_DC_ERR) {
		dev_info(battery->dev,
			"%s: DC err (%d)\n",
			__func__, health);
		battery->is_recharging = false;
		battery->health_check_count = DEFAULT_HEALTH_CHECK_COUNT;
		/* Enable charging anyway to check actual DC's health */
		sec_vote(battery->chgen_vote, VOTER_DC_ERR, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
		sec_vote(battery->chgen_vote, VOTER_DC_ERR, false, 0);
	}

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	if (health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE) {
		psy_do_property(battery->pdata->wireless_charger_name, get,
			POWER_SUPPLY_PROP_CHARGE_EMPTY, value);
		if (value.intval == 0) {
			dev_info(battery->dev, "%s: skip undervoltage for WL %d %d\n",
				__func__, health, battery->health);
			health = battery->health;
		}
	}
#endif

	if (battery->health != health) {
		sec_bat_set_health(battery, health);
		sb_set_vbus_wake(battery->vbus_ws, health, battery->cable_type);
		switch (health) {
		case POWER_SUPPLY_HEALTH_GOOD:
			dev_info(battery->dev, "%s: Safe voltage\n", __func__);
			dev_info(battery->dev, "%s: is_recharging : %d\n", __func__, battery->is_recharging);
			if (!(battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY))
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
			sec_vote(battery->chgen_vote, VOTER_VBUS_OVP, false, 0);
			battery->health_check_count = 0;
			break;
		case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
		case POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE:
			dev_info(battery->dev,
				"%s: Unsafe voltage (%d)\n",
				__func__, health);
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_NOT_CHARGING);
			sec_vote(battery->chgen_vote, VOTER_VBUS_OVP, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->is_recharging = false;
			battery->health_check_count = DEFAULT_HEALTH_CHECK_COUNT;
			battery->cisd.data[CISD_DATA_UNSAFETY_VOLTAGE]++;
			battery->cisd.data[CISD_DATA_UNSAFE_VOLTAGE_PER_DAY]++;
			break;
		case POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE:
			dev_info(battery->dev,
				"%s: watchdog timer expired (%d)\n",
				__func__, health);
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_NOT_CHARGING);
			sec_vote(battery->chgen_vote, VOTER_WDT_EXPIRE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			battery->is_recharging = false;
			battery->health_check_count = DEFAULT_HEALTH_CHECK_COUNT;
			break;
		}
		power_supply_changed(battery->psy_bat);
		if (health != POWER_SUPPLY_HEALTH_GOOD)
			return true;
	}

	return false;
}

static bool sec_bat_ovp_uvlo(struct sec_battery_info *battery)
{
	int health = POWER_SUPPLY_HEALTH_GOOD;

#ifdef CONFIG_IFPMIC_LIMITER
	if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
			(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_dbg(battery->dev, "%s: No need to check in Full status", __func__);
		return false;
	}
#else
	if (battery->wdt_kick_disable) {
		dev_dbg(battery->dev,
			"%s: No need to check in wdt test\n",
			__func__);
		return false;
	} else if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
			(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_dbg(battery->dev, "%s: No need to check in Full status", __func__);
		return false;
	}
#endif

	if (battery->health != POWER_SUPPLY_HEALTH_GOOD &&
		battery->health != POWER_SUPPLY_HEALTH_OVERVOLTAGE &&
		battery->health != POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE &&
		battery->health != POWER_SUPPLY_EXT_HEALTH_DC_ERR &&
		battery->health != POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE) {
		dev_dbg(battery->dev, "%s: No need to check\n", __func__);
		return false;
	}
	health = battery->health;

	switch (battery->pdata->ovp_uvlo_check_type) {
	case SEC_BATTERY_OVP_UVLO_CALLBACK:
		if (battery->pdata->ovp_uvlo_callback)
			health = battery->pdata->ovp_uvlo_callback();
		break;
	case SEC_BATTERY_OVP_UVLO_PMICPOLLING:
	case SEC_BATTERY_OVP_UVLO_CHGPOLLING:
		health = sec_bat_ovp_uvlo_by_psy(battery);
		break;
	case SEC_BATTERY_OVP_UVLO_PMICINT:
	case SEC_BATTERY_OVP_UVLO_CHGINT:
		/* nothing for interrupt check */
	default:
		break;
	}

	if (battery->factory_mode) {
		dev_dbg(battery->dev,
			"%s: No need to check in factory mode\n",
			__func__);
		return false;
	}

	return sec_bat_ovp_uvlo_result(battery, health);
}

__visible_for_testing bool sec_bat_check_recharge(struct sec_battery_info *battery)
{
	if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)
		return false;

	if ((battery->status == POWER_SUPPLY_STATUS_CHARGING) &&
		(battery->pdata->full_condition_type &
		SEC_BATTERY_FULL_CONDITION_NOTIMEFULL) &&
		(battery->charging_mode == SEC_BATTERY_CHARGING_NONE)) {
		dev_info(battery->dev, "%s: Re-charging by NOTIMEFULL (%d)\n",
			__func__, battery->capacity);
		goto check_recharge_check_count;
	}

	if (battery->status == POWER_SUPPLY_STATUS_FULL &&
		battery->charging_mode == SEC_BATTERY_CHARGING_NONE) {
		int recharging_voltage = battery->pdata->recharge_condition_vcell,
			recharging_soc = battery->pdata->recharge_condition_soc;

		if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
				recharging_voltage = battery->pdata->swelling_low_cool3_rechg_voltage;
			else
				recharging_voltage = battery->pdata->swelling_low_rechg_voltage;
		}
		dev_info(battery->dev, "%s: recharging voltage(%d) soc(%d) %s\n",
			__func__, recharging_voltage, recharging_soc,
			(battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE ?
				"changed by low temp" : "normal"));

		if ((battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_SOC) &&
			(battery->capacity <= recharging_soc)) {
			dev_info(battery->dev, "%s: Re-charging by SOC (%d)\n",
				__func__, battery->capacity);
			goto check_recharge_check_count;
		}

		if ((battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_AVGVCELL) &&
			(battery->voltage_avg <= recharging_voltage)) {
			dev_info(battery->dev, "%s: Re-charging by average VCELL (%d)\n",
				__func__, battery->voltage_avg);
			goto check_recharge_check_count;
		}

		if ((battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_VCELL) &&
			(battery->voltage_now <= recharging_voltage)) {
			dev_info(battery->dev, "%s: Re-charging by VCELL (%d)\n",
				__func__, battery->voltage_now);
			goto check_recharge_check_count;
		}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_LIMITER) {
			int voltage = max(battery->voltage_avg_main, battery->voltage_avg_sub);
			if (voltage <= recharging_voltage) {
				dev_info(battery->dev, "%s: Re-charging by VPACK (%d)mV\n",
					__func__, voltage);
				goto check_recharge_check_count;
			} else if (abs(battery->voltage_avg_main - battery->voltage_avg_sub) >
				battery->pdata->force_recharge_margin) {
				dev_info(battery->dev, "%s: Force Re-charging by VPACK diff(%d, %d)mV\n",
					__func__, battery->voltage_avg_main, battery->voltage_avg_sub);
				goto check_recharge_check_count;
			}
		}

#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
		if (battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_DUAL_BATTERY_INDIVIDUAL_FV) {
			int rechg_voltage_main = battery->recharging_voltage_main;
			int rechg_voltage_sub = battery->recharging_voltage_sub;

			if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE) {
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3) {
					rechg_voltage_main = battery->pdata->swelling_low_cool3_rechg_voltage;
					rechg_voltage_sub = battery->pdata->swelling_low_cool3_rechg_voltage;
				} else {
					rechg_voltage_main = battery->pdata->swelling_low_rechg_voltage;
					rechg_voltage_sub = battery->pdata->swelling_low_rechg_voltage;
				}
			}

			if (battery->voltage_avg_main <= rechg_voltage_main ||
				battery->voltage_avg_sub <= rechg_voltage_sub) {
				pr_info("%s: Re-charging by Voltage m(%d/%d)mV, s(%d/%d)mV\n",
					__func__, battery->voltage_avg_main, rechg_voltage_main,
					battery->voltage_avg_sub, rechg_voltage_sub);
				goto check_recharge_check_count;
			}
		}
#endif
#endif
	}

	battery->recharge_check_cnt = 0;
	return false;

check_recharge_check_count:
	battery->safety_timer.remaining_time = battery->pdata->recharging_expired_time;
	battery->safety_timer.prev_time = 0;

	if (battery->recharge_check_cnt <
		battery->pdata->recharge_check_count)
		battery->recharge_check_cnt++;
	dev_dbg(battery->dev,
		"%s: recharge count = %d\n",
		__func__, battery->recharge_check_cnt);

	if (battery->recharge_check_cnt >=
		battery->pdata->recharge_check_count)
		return true;
	else
		return false;
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_recharge);

static bool sec_bat_voltage_check(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING ||
		is_nocharge_type(battery->cable_type) ||
		is_wireless_fake_type(battery->cable_type)) {
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	/* OVP/UVLO check */
	if (sec_bat_ovp_uvlo(battery)) {
		if (battery->pdata->ovp_uvlo_result_callback)
			battery->pdata->
				ovp_uvlo_result_callback(battery->health);
		return false;
	}

	if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
			((battery->charging_mode != SEC_BATTERY_CHARGING_NONE &&
			battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING) ||
			(battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING))) {
		int voltage_now = battery->voltage_now;
		int voltage_ref = battery->pdata->recharge_condition_vcell - 50;
#if defined(CONFIG_ENABLE_FULL_BY_SOC)
		int soc_ref = 98;
#else
		int soc_ref = battery->pdata->full_condition_soc;
#endif

		if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3)
				voltage_ref = battery->pdata->swelling_low_cool3_rechg_voltage - 50;
			else
				voltage_ref = battery->pdata->swelling_low_rechg_voltage - 50;
		}

		if (is_eu_eco_rechg(battery->fs))
			soc_ref = (soc_ref > battery->pdata->recharge_condition_soc) ?
				battery->pdata->recharge_condition_soc : soc_ref;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		if (battery->pdata->recharge_condition_type &
			SEC_BATTERY_RECHARGE_CONDITION_LIMITER) {
			voltage_now = max(battery->voltage_avg_main, battery->voltage_avg_sub);
		}
#ifdef CONFIG_BATTERY_HOTSWAP
		if (battery->pdata->dual_battery_individual_topoff && soc_ref == 100)
			voltage_now = 0; /* if soc_ref is 100, ignore voltage condition */
#endif
#endif

		pr_info("%s: charging_mode(%d), voltage_ref(%d), voltage_now(%d), soc_ref(%d), soc(%d)\n",
			__func__, battery->charging_mode, voltage_ref, voltage_now, soc_ref, battery->capacity);

		if (battery->capacity < soc_ref && voltage_now < voltage_ref) {
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
			battery->safety_timer.remaining_time = battery->pdata->expired_time;
			battery->safety_timer.prev_time = 0;
#endif
			battery->is_recharging = false;
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			if (is_wireless_type(battery->cable_type)) {
				value.intval = EXIT_PHM;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_RX_PHM, value);
			}
#endif
			if (battery->pdata->change_FV_after_full)
				sec_vote(battery->fv_vote, VOTER_FULL_CHARGE, false, battery->pdata->chg_float_voltage);
			sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_CHARGING);
			sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
			sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
			pr_info("%s: battery status full -> charging, soc(%d)\n", __func__, battery->capacity);
			value.intval = POWER_SUPPLY_STATUS_CHARGING;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_STATUS, value);
			return false;
		}
	}

	/* Re-Charging check */
	if (sec_bat_check_recharge(battery)) {
		if (battery->pdata->full_check_type !=
			SEC_BATTERY_FULLCHARGED_NONE)
			battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
		else
			battery->charging_mode = SEC_BATTERY_CHARGING_2ND;

		battery->is_recharging = true;
		battery->cisd.data[CISD_DATA_RECHARGING_COUNT]++;
		battery->cisd.data[CISD_DATA_RECHARGING_COUNT_PER_DAY]++;
		if (is_wireless_type(battery->cable_type)) {
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			value.intval = EXIT_PHM;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_RX_PHM, value);
#endif
			if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP) {
				value.intval = CLOAK_EXIT_CMD;
				psy_do_property("wireless", set,
					POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
			}
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_1ST_DONE, value);

			value.intval = WIRELESS_VRECT_ADJ_OFF;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL, value);
		}

		if (battery->pdata->change_FV_after_full)
			sec_vote(battery->fv_vote, VOTER_FULL_CHARGE, false, battery->pdata->chg_float_voltage);
		sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_CHARGING);
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);

		return false;
	}

	return true;
}

__visible_for_testing bool sec_bat_set_aging_step(struct sec_battery_info *battery, int step)
{
	union power_supply_propval value = {0, };

	if (battery->pdata->num_age_step <= 0 || step < 0 || step >= battery->pdata->num_age_step) {
		pr_info("%s: [AGE] abnormal age step : %d/%d\n",
			__func__, step, battery->pdata->num_age_step-1);
		return false;
	}

	battery->pdata->age_step = step;

	/* float voltage */
	battery->pdata->chg_float_voltage =
		battery->pdata->age_data[battery->pdata->age_step].float_voltage;
	sec_vote(battery->fv_vote, VOTER_AGING_STEP, true, battery->pdata->chg_float_voltage);

	/* full/recharge condition */
	battery->pdata->recharge_condition_vcell =
		battery->pdata->age_data[battery->pdata->age_step].recharge_condition_vcell;
	battery->pdata->full_condition_soc =
		battery->pdata->age_data[battery->pdata->age_step].full_condition_soc;
	battery->pdata->full_condition_vcell =
		battery->pdata->age_data[battery->pdata->age_step].full_condition_vcell;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	value.intval = battery->pdata->age_step;
	psy_do_property(battery->pdata->dual_battery_name, set,
		POWER_SUPPLY_EXT_PROP_FULL_CONDITION, value);

	if (battery->pdata->limiter_aging_float_offset > 0) {
#ifdef CONFIG_IFPMIC_LIMITER
		sec_vote(battery->vlim_vote, VOTER_AGING_STEP, true,
			battery->pdata->chg_float_voltage - battery->pdata->limiter_aging_float_offset);
#else
		value.intval = battery->pdata->chg_float_voltage -
			battery->pdata->limiter_aging_float_offset;
		psy_do_property(battery->pdata->dual_battery_name, set,
			POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		if (battery->pdata->dc_step_chg_vsublim) {
			battery->pdata->dc_step_chg_vsublim[battery->dc_step_chg_step-1] =
				min(battery->pdata->dc_step_chg_vsublim[battery->dc_step_chg_step-1],
				battery->pdata->chg_float_voltage - battery->pdata->limiter_aging_float_offset);
		}
#endif
	}
#endif
#if defined(CONFIG_LSI_IFPMIC)
	value.intval = battery->pdata->age_step;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_EXT_PROP_UPDATE_BATTERY_DATA, value);
#else
	value.intval = battery->pdata->full_condition_soc;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_CAPACITY_LEVEL, value);
#endif
#if defined(CONFIG_STEP_CHARGING)
	sec_bat_set_aging_info_step_charging(battery);
#endif
#if IS_ENABLED(CONFIG_MTK_CHARGER)
	if (battery->pdata->dynamic_cv_factor) {
		value.intval = (battery->pdata->chg_float_voltage) * 1000;
		psy_do_property(battery->pdata->fuelgauge_name, set,
						POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, value);
	}
#endif

	dev_info(battery->dev,
		"%s: Step(%d/%d), Cycle(%d), float_v(%d), r_v(%d), f_s(%d), f_vl(%d)\n",
		__func__,
		battery->pdata->age_step, battery->pdata->num_age_step-1, max(battery->batt_cycle, battery->batt_full_status_usage),
		battery->pdata->chg_float_voltage,
		battery->pdata->recharge_condition_vcell,
		battery->pdata->full_condition_soc,
		battery->pdata->full_condition_vcell);

#if defined(CONFIG_BATTERY_AGE_FORECAST_B2B)
	{
		int i;
		bool bChanged = false;

		battery->pdata->max_charging_current =
			battery->pdata->age_data[battery->pdata->age_step].max_charging_current;

		for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
			if (battery->pdata->charging_current[i].fast_charging_current >
				battery->pdata->max_charging_current) {

				dev_info(battery->dev, "%s: cable(%d) charging current(%d->%d)\n",
					__func__, i,
					battery->pdata->charging_current[i].fast_charging_current,
					battery->pdata->max_charging_current);
				battery->pdata->charging_current[i].fast_charging_current =
					battery->pdata->max_charging_current;
				if (battery->cable_type == i)
					bChanged = true;
			}
		}

		if (bChanged)
			sec_bat_set_charging_current(battery);
	}
#endif
	return true;
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_aging_step);

#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
void sec_bat_aging_check_dual_battery_individual_fv(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	int offset = 0;
	int fv = battery->pdata->chg_float_voltage - battery->pdata->limiter_aging_float_offset;

	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_CYCLES, value);

	if (value.intval > 500)
		offset += 30;
	if (value.intval > 1000)
		offset += 30;

	battery->recharging_voltage_main = battery->pdata->recharge_condition_vcell - offset;

	pr_info("%s: [MAIN] cycle(%d), offset(%d), fv(%d), m_rechg(%d)\n",
		__func__, value.intval, offset, fv - offset,
		battery->recharging_voltage_main);

	value.intval = fv - offset;
	psy_do_property(battery->pdata->dual_battery_name, set,
		POWER_SUPPLY_EXT_PROP_MAIN_LIMITER_CHG_VOLTAGE, value);

	offset = 0;
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_CYCLES, value);

	if (value.intval > 500)
		offset += 30;
	if (value.intval > 1000)
		offset += 30;

	battery->recharging_voltage_sub = battery->pdata->recharge_condition_vcell - offset;

	pr_info("%s: [SUB] cycle(%d), offset(%d), fv(%d), s_rechg(%d)\n",
		__func__, value.intval, offset, fv - offset,
		battery->recharging_voltage_sub);

	value.intval = fv - offset;
	psy_do_property(battery->pdata->dual_battery_name, set,
		POWER_SUPPLY_EXT_PROP_SUB_LIMITER_CHG_VOLTAGE, value);
}
EXPORT_SYMBOL(sec_bat_aging_check_dual_battery_individual_fv);
#endif

void sec_bat_aging_check(struct sec_battery_info *battery)
{
	int prev_step = battery->pdata->age_step;
	int calc_step;
	bool ret = 0;
	static bool init; /* false */

	if ((battery->pdata->num_age_step <= 0) || (battery->batt_cycle < 0))
		return;

	if (battery->temperature < 50) {
		pr_info("%s: [AGE] skip (temperature:%d)\n", __func__, battery->temperature);
		return;
	}

	for (calc_step = battery->pdata->num_age_step - 1; calc_step >= 0; calc_step--) {
		if (battery->pdata->age_data[calc_step].cycle <= max(battery->batt_cycle, battery->batt_full_status_usage))
			break;
	}

	if ((calc_step == prev_step) && init)
		return;

	init = true;
	ret = sec_bat_set_aging_step(battery, calc_step);
	dev_info(battery->dev,
		"%s: %s change step (%d->%d), Cycle(%d)\n",
		__func__, ret ? "Succeed in" : "Fail to",
		prev_step, battery->pdata->age_step, max(battery->batt_cycle, battery->batt_full_status_usage));
}
EXPORT_SYMBOL(sec_bat_aging_check);

void sec_bat_check_battery_health(struct sec_battery_info *battery)
{
	static battery_health_condition default_table[3] =
		{{.cycle = 900, .asoc = 75}, {.cycle = 1200, .asoc = 65}, {.cycle = 1500, .asoc = 55}};

	battery_health_condition *ptable = default_table;
	battery_health_condition state;
	int i, battery_health, size = BATTERY_HEALTH_MAX;

	if (battery->pdata->health_condition == NULL) {
		/*
		 * If a new type is added to misc_battery_health, default table cannot verify the actual state except "bad".
		 * If you want to modify to return the correct values for all states,
		 * add a table that matches the state added to the dt file.
		*/
		pr_info("%s: does not set health_condition_table, use default table\n", __func__);
		size = 3;
	} else {
		ptable = battery->pdata->health_condition;
	}

	/* Checking Cycle and ASoC */
	state.cycle = state.asoc = BATTERY_HEALTH_BAD;
	for (i = size - 1; i >= 0; i--) {
		if (ptable[i].cycle >= (battery->batt_cycle % 10000))
			state.cycle = i + BATTERY_HEALTH_GOOD;
		if (ptable[i].asoc <= battery->batt_asoc)
			state.asoc = i + BATTERY_HEALTH_GOOD;
	}
	battery_health = max(state.cycle, state.asoc);
	pr_info("%s: update battery_health(%d), (%d - %d)\n",
		__func__, battery_health, state.cycle, state.asoc);
	/* Update battery health */
	sec_bat_set_misc_event(battery,
		(battery_health << BATTERY_HEALTH_SHIFT), BATT_MISC_EVENT_BATTERY_HEALTH);
}
EXPORT_SYMBOL(sec_bat_check_battery_health);

__visible_for_testing bool sec_bat_check_fullcharged_condition(struct sec_battery_info *battery)
{
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;
	int voltage_now, voltage_avg;

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
		full_check_type = battery->pdata->full_check_type;
	else
		full_check_type = battery->pdata->full_check_type_2nd;

	switch (full_check_type) {
	/*
	 * If these is NOT full check type or NONE full check type,
	 * it is full-charged
	 */
	case SEC_BATTERY_FULLCHARGED_CHGINT:
	case SEC_BATTERY_FULLCHARGED_TIME:
	case SEC_BATTERY_FULLCHARGED_NONE:
		return true;
	default:
		break;
	}

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100 && !battery->is_recharging) {
		dev_info(battery->dev, "%s: enough SOC (%d%%), skip!\n", __func__, battery->capacity);
		return true;
	}
#endif

	if (battery->pdata->full_condition_type & SEC_BATTERY_FULL_CONDITION_SOC) {
		if (battery->capacity < battery->pdata->full_condition_soc) {
			dev_dbg(battery->dev, "%s: Not enough SOC (%d%%)\n", __func__, battery->capacity);
			return false;
		}
	}

	if (battery->pdata->full_condition_type & SEC_BATTERY_FULL_CONDITION_VCELL) {
		int full_condition_vcell = battery->pdata->full_condition_vcell;

		if (battery->thermal_zone == BAT_THERMAL_WARM)	/* high temp swelling full */
			full_condition_vcell = battery->pdata->high_temp_float - 100;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		voltage_now = max(battery->voltage_now_main, battery->voltage_now_sub);
#else
		voltage_now = battery->voltage_now;
#endif
		if (voltage_now < full_condition_vcell) {
			dev_dbg(battery->dev, "%s: Not enough VCELL (%dmV)\n", __func__, voltage_now);
			return false;
		}
	}

	if (battery->pdata->full_condition_type & SEC_BATTERY_FULL_CONDITION_AVGVCELL) {
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		voltage_avg = max(battery->voltage_avg_main, battery->voltage_avg_sub);
#else
		voltage_avg = battery->voltage_avg;
#endif
		if (voltage_avg < battery->pdata->full_condition_avgvcell) {
			dev_dbg(battery->dev, "%s: Not enough AVGVCELL (%dmV)\n", __func__, voltage_avg);
			return false;
		}
	}

	if (battery->pdata->full_condition_type & SEC_BATTERY_FULL_CONDITION_OCV) {
		if (battery->voltage_ocv < battery->pdata->full_condition_ocv) {
			dev_dbg(battery->dev, "%s: Not enough OCV (%dmV)\n", __func__, battery->voltage_ocv);
			return false;
		}
	}

	return true;
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_fullcharged_condition);

__visible_for_testing int sec_bat_do_test_function(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (battery->test_mode == 0)
		return battery->test_mode;

	pr_info("%s: Test Mode\n", __func__);
	switch (battery->test_mode) {
	case 1:
		if (battery->status == POWER_SUPPLY_STATUS_CHARGING) {
			sec_vote(battery->chgen_vote, VOTER_TEST_MODE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			sec_bat_set_charging_status(battery,
					POWER_SUPPLY_STATUS_DISCHARGING);
		}
		break;
	case 2:
		if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
			sec_vote(battery->chgen_vote, VOTER_TEST_MODE, true, SEC_BAT_CHG_MODE_CHARGING);
			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_PROP_STATUS, value);
			sec_bat_set_charging_status(battery, value.intval);
		}
		battery->test_mode = 0;
		break;
	case 3: // clear temp block
		sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);
		sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_DISCHARGING);
		break;
	case 4:
		if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
			sec_vote(battery->chgen_vote, VOTER_TEST_MODE, true, SEC_BAT_CHG_MODE_CHARGING);
			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_PROP_STATUS, value);
			sec_bat_set_charging_status(battery, value.intval);
		}
		break;
	default:
		pr_info("%s: error test: unknown state\n", __func__);
		break;
	}
	return battery->test_mode;
}
EXPORT_SYMBOL_KUNIT(sec_bat_do_test_function);

static bool sec_bat_time_management(struct sec_battery_info *battery)
{
	struct timespec64 ts = {0, };
	unsigned long charging_time;

	if (battery->charging_start_time == 0 || !battery->safety_timer_set) {
		dev_dbg(battery->dev,
			"%s: Charging Disabled\n", __func__);
		return true;
	}

	ts = ktime_to_timespec64(ktime_get_boottime());

	if (ts.tv_sec >= battery->charging_start_time) {
		charging_time = ts.tv_sec - battery->charging_start_time;
	} else {
		charging_time = 0xFFFFFFFF - battery->charging_start_time
			+ ts.tv_sec;
	}

	battery->charging_passed_time = charging_time;

	switch (battery->status) {
	case POWER_SUPPLY_STATUS_FULL:
		if (battery->safety_timer.remaining_time == 0) {
			dev_info(battery->dev,
				"%s: Recharging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE);
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->is_recharging = false;
			sec_vote(battery->chgen_vote, VOTER_TIME_EXPIRED, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			return false;
		}
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		if ((battery->pdata->full_condition_type &
			SEC_BATTERY_FULL_CONDITION_NOTIMEFULL) &&
			(battery->is_recharging && (battery->safety_timer.remaining_time == 0))) {
			dev_info(battery->dev,
			"%s: Recharging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE);
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->is_recharging = false;
			sec_vote(battery->chgen_vote, VOTER_TIME_EXPIRED, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			return false;
		} else if (!battery->is_recharging &&
				(battery->safety_timer.remaining_time == 0)) {
			dev_info(battery->dev,
				"%s: Charging Timer Expired\n", __func__);
			battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
			sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE);
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
			battery->cisd.data[CISD_DATA_SAFETY_TIMER]++;
			battery->cisd.data[CISD_DATA_SAFETY_TIMER_PER_DAY]++;
#if IS_ENABLED(CONFIG_SEC_ABC)
			sec_abc_send_event("MODULE=battery@WARN=safety_timer");
#endif
			sec_vote(battery->chgen_vote, VOTER_TIME_EXPIRED, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			return false;
		}
		break;
	default:
		dev_err(battery->dev,
			"%s: Undefine Battery Status\n", __func__);
		return true;
	}

	return true;
}

bool sec_bat_check_full(struct sec_battery_info *battery, int full_check_type)
{
	union power_supply_propval value = {0, };
	int current_adc = 0;
	bool ret = false;
	int err = 0;

	switch (full_check_type) {
	case SEC_BATTERY_FULLCHARGED_ADC:
		current_adc =
			sec_bat_get_adc_data(battery->dev,
			SEC_BAT_ADC_CHANNEL_FULL_CHECK,
			battery->pdata->adc_check_count,
			battery->pdata->adc_read_type);

		dev_dbg(battery->dev, "%s: Current ADC (%d)\n", __func__, current_adc);

#if !defined(CONFIG_SEC_KUNIT) /* In Kunit test, battery->current_adc is changed by KUNIT TC */
		if (current_adc < 0)
			break;

		battery->current_adc = current_adc;
#endif

		if (battery->current_adc < battery->topoff_condition) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev, "%s: Full Check ADC (%d)\n",
					__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		break;
	case SEC_BATTERY_FULLCHARGED_FG_CURRENT:
		if ((battery->current_now > 0 && battery->current_now <
			battery->topoff_condition) &&
			(battery->current_avg > 0 && battery->current_avg < battery->topoff_condition)) {
				battery->full_check_cnt++;
				dev_dbg(battery->dev, "%s: Full Check Current (%d)\n",
				__func__, battery->full_check_cnt);
		} else if (battery->current_now <= 0 &&
			(battery->voltage_now > battery->pdata->high_temp_float) &&
			(battery->misc_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev, "%s: Full Check Current (%d)\n",
				__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		break;
	case SEC_BATTERY_FULLCHARGED_TIME:
		if ((battery->charging_mode ==
			SEC_BATTERY_CHARGING_2ND ?
			(battery->charging_passed_time -
			battery->charging_fullcharged_time) :
			battery->charging_passed_time) >
			battery->topoff_condition) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev, "%s: Full Check Time (%d)\n",
				__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		break;
	case SEC_BATTERY_FULLCHARGED_SOC:
		if (battery->capacity <=
			battery->topoff_condition) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev, "%s: Full Check SOC (%d)\n",
				__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		break;

	case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		err = gpio_request(
			battery->pdata->chg_gpio_full_check,
			"GPIO_CHG_FULL");
		if (err) {
			dev_err(battery->dev,
				"%s: Error in Request of GPIO\n", __func__);
			break;
		}
		if (!(gpio_get_value_cansleep(
			battery->pdata->chg_gpio_full_check) ^
			!battery->pdata->chg_polarity_full_check)) {
			battery->full_check_cnt++;
			dev_dbg(battery->dev, "%s: Full Check GPIO (%d)\n",
				__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		gpio_free(battery->pdata->chg_gpio_full_check);
		break;
	case SEC_BATTERY_FULLCHARGED_CHGINT:
	case SEC_BATTERY_FULLCHARGED_CHGPSY:
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_PROP_STATUS, value);

		if (value.intval == POWER_SUPPLY_STATUS_FULL) {
			battery->full_check_cnt++;
			dev_info(battery->dev, "%s: Full Check Charger (%d)\n",
				__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		break;

	/*
	 * If these is NOT full check type or NONE full check type,
	 * it is full-charged
	 */
	case SEC_BATTERY_FULLCHARGED_NONE:
		battery->full_check_cnt = 0;
		ret = true;
		break;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	case SEC_BATTERY_FULLCHARGED_LIMITER:
		value.intval = 1;
		psy_do_property(battery->pdata->dual_battery_name, get,
			POWER_SUPPLY_PROP_STATUS, value);

		if (value.intval == POWER_SUPPLY_STATUS_FULL) {
			battery->full_check_cnt++;
			dev_info(battery->dev, "%s: Full Check Limiter (%d)\n",
				__func__, battery->full_check_cnt);
		} else {
			battery->full_check_cnt = 0;
		}
		break;
#endif
	default:
		dev_err(battery->dev,
			"%s: Invalid Full Check\n", __func__);
		break;
	}

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100 &&
		battery->charging_mode == SEC_BATTERY_CHARGING_1ST &&
		!battery->is_recharging) {
		battery->full_check_cnt = battery->pdata->full_check_count;
		dev_info(battery->dev,
			"%s: enough SOC to make FULL(%d%%)\n",
			__func__, battery->capacity);
	}
#endif

	if (battery->full_check_cnt >=
		battery->pdata->full_check_count) {
		battery->full_check_cnt = 0;
		ret = true;
	}

	if (ret && (battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE)) {
		battery->cisd.data[CISD_DATA_SWELLING_FULL_CNT]++;
		battery->cisd.data[CISD_DATA_SWELLING_FULL_CNT_PER_DAY]++;
	}
	return ret;
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_full);

static void sec_bat_check_eoc_for_repcap(struct sec_battery_info *battery, int full_check_type)
{
	union power_supply_propval value = {0, };

	if (battery->pdata->soc_by_repcap_en && battery->eoc_d->eoc_check) {
		pr_info("%s: check eoc for repcap\n", __func__);
		switch (full_check_type) {
		case SEC_BATTERY_FULLCHARGED_FG_CURRENT:
			if ((battery->current_now > 0 &&
				battery->current_now < battery->pdata->full_check_current_1st) &&
				(battery->current_avg > 0 &&
				battery->current_avg < battery->pdata->full_check_current_1st)) {
				battery->eoc_d->eoc_cnt++;
				pr_info("%s : full cnt  = %d FG current full check cnt = %d\n", __func__,
					battery->pdata->full_check_count, battery->eoc_d->eoc_cnt);
				if (battery->eoc_d->eoc_cnt >= battery->pdata->full_check_count) {
					battery->eoc_d->eoc_check = false;
					value.intval = battery->capacity;
					/*update 1st EOC repcap value */
					psy_do_property(battery->pdata->fuelgauge_name, set,
							POWER_SUPPLY_EXT_PROP_STATUS_FULL_REPCAP, value);
					pr_info("%s:first EOC\n", __func__);
				}
			} else {
				pr_info("%s: not first EOC\n", __func__);
				battery->eoc_d->eoc_cnt = 0; // check by default value
			}
			break;
		default:
			dev_err(battery->dev,
				"%s: Invalid Full Check\n", __func__);
			break;
		}
	}
}

static void sec_bat_enable_eoc_check(struct sec_battery_info *battery)
{
	if (battery->pdata->soc_by_repcap_en && battery->capacity < 100 &&
		battery->charging_mode == SEC_BATTERY_CHARGING_1ST) {
		battery->eoc_d->eoc_check = true; // check for default value, check charging mode value

		pr_info("%s : eoc_check(%d) charging mode int(%d) , battery_capacity: %d\n",
				__func__, battery->eoc_d->eoc_check, battery->charging_mode, battery->capacity);
	}
}

bool sec_bat_check_fullcharged(struct sec_battery_info *battery)
{
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;

	sec_bat_enable_eoc_check(battery);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	/* do not check 1st full condition during 2nd charging  */
	if (battery->charging_mode != SEC_BATTERY_CHARGING_2ND &&
		!sec_bat_check_fullcharged_condition(battery))
		return false;
#else
	if (!sec_bat_check_fullcharged_condition(battery))
		return false;
#endif

	sec_bat_check_eoc_for_repcap(battery, battery->pdata->full_check_type);

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
		full_check_type = battery->pdata->full_check_type;
	else
		full_check_type = battery->pdata->full_check_type_2nd;

	return sec_bat_check_full(battery, full_check_type);
}

#ifdef CONFIG_IFPMIC_LIMITER
void sec_bat_trace_vtrack(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };

	if (!is_nocharge_type(battery->cable_type)) {
		value.intval = 1;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_TRACE_VTRACK, value);
	}
}
#endif

__visible_for_testing void sec_bat_do_fullcharged(struct sec_battery_info *battery, bool force_fullcharged)
{
	union power_supply_propval value = {0, };

	/*
	 * To let charger/fuel gauge know the full status,
	 * set status before calling sec_bat_set_charge()
	 */
	if (battery->status != POWER_SUPPLY_STATUS_FULL) {
		battery->cisd.data[CISD_DATA_FULL_COUNT]++;
		battery->cisd.data[CISD_DATA_FULL_COUNT_PER_DAY]++;
	}
	sec_bat_set_charging_status(battery,
			POWER_SUPPLY_STATUS_FULL);

	if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST &&
		battery->pdata->full_check_type_2nd != SEC_BATTERY_FULLCHARGED_NONE && !force_fullcharged) {
		battery->charging_mode = SEC_BATTERY_CHARGING_2ND;
		battery->charging_fullcharged_time = battery->charging_passed_time;
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, true, battery->pdata->full_check_current_2nd);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING);
		pr_info("%s: 1st charging is done\n", __func__);
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_1ST_DONE, value);
	} else {
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		battery->is_recharging = false;

		if (!battery->wdt_kick_disable) {
			pr_info("%s: wdt kick enable -> Charger Off, %d\n",
					__func__, battery->wdt_kick_disable);
			sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
			if (battery->pdata->change_FV_after_full)
				sec_vote(battery->fv_vote, VOTER_FULL_CHARGE, true, battery->pdata->change_FV_after_full);
			pr_info("%s: 2nd charging is done\n", __func__);
			if (is_wireless_type(battery->cable_type)) {
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_2ND_DONE, value);
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
				value.intval = ENTER_PHM;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_RX_PHM, value);
#endif
				if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_MPP) {
					value.intval = CLOAK_GENERIC;
					psy_do_property("wireless", set,
						POWER_SUPPLY_EXT_PROP_MPP_CLOAK, value);
				}
			}
		} else {
			pr_info("%s: wdt kick disabled -> skip charger off, %d\n",
					__func__, battery->wdt_kick_disable);
		}

#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
		if (battery->pdata->dual_battery_individual_fv)
			sec_bat_aging_check_dual_battery_individual_fv(battery);
		else
			sec_bat_aging_check(battery);
#else
		sec_bat_aging_check(battery);
#endif

		/* this concept is only for power-off charging mode*/
		if (!battery->store_mode && sec_bat_get_lpmode()) {
			/* vbus level : 9V --> 5V */
			sec_vote(battery->iv_vote, VOTER_FULL_CHARGE, true, SEC_INPUT_VOLTAGE_5V);
			pr_info("%s: vbus is set 5V by 2nd full\n", __func__);
		}

		value.intval = POWER_SUPPLY_STATUS_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_STATUS, value);
	}

	/*
	 * platform can NOT get information of battery
	 * because wakeup time is too short to check uevent
	 * To make sure that target is wakeup if full-charged,
	 * activated wake lock in a few seconds
	 */
	if (battery->pdata->polling_type == SEC_BATTERY_MONITOR_ALARM)
		sb_set_vbus_wake(battery->vbus_ws, battery->health, battery->cable_type);
}
EXPORT_SYMBOL_KUNIT(sec_bat_do_fullcharged);

static bool sec_bat_fullcharged_check(struct sec_battery_info *battery)
{
	if ((battery->charging_mode == SEC_BATTERY_CHARGING_NONE) ||
		(battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
		dev_dbg(battery->dev,
			"%s: No Need to Check Full-Charged\n", __func__);
		return true;
	}

	if (sec_bat_check_fullcharged(battery)) {
		union power_supply_propval value = {0, };
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		/* this forced 100% is only for 1st full charging not for 2nd full charging */
		if ((battery->status != POWER_SUPPLY_STATUS_FULL) && battery->capacity < 100) {
#else
		if (battery->capacity < 100) {
#endif
			/* update capacity max */
			value.intval = battery->capacity;
			psy_do_property(battery->pdata->fuelgauge_name, set,
					POWER_SUPPLY_PROP_CHARGE_FULL, value);
			pr_info("%s : forced full-charged sequence for the capacity(%d)\n",
					__func__, battery->capacity);
			battery->full_check_cnt = battery->pdata->full_check_count;
		} else {
			sec_bat_do_fullcharged(battery, false);
		}
	}

	dev_info(battery->dev,
		"%s: Charging Mode : %s\n", __func__,
		battery->is_recharging ?
		sb_get_cm_str(SEC_BATTERY_CHARGING_RECHARGING) :
		sb_get_cm_str(battery->charging_mode));

	return true;
}

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
static bool sec_bat_check_wireless_overheat(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };
	int temp_offset = 0;

	if (battery->misc_event & BATT_MISC_EVENT_WIRELESS_OVERHEAT)
		temp_offset = 20;

	if (battery->temperature >= battery->pdata->wireless_warm_overheat_thresh - temp_offset) {
		dev_info(battery->dev, "%s: overheat with FAKE cable type!\n", __func__);
		/* Do NOT activate cable work for AA which is overheat condition */

		val.intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		psy_do_property(battery->pdata->wireless_charger_name, set, POWER_SUPPLY_PROP_HEALTH, val);

		if (!(battery->misc_event & BATT_MISC_EVENT_WIRELESS_OVERHEAT)) {
			sec_bat_set_misc_event(battery,
					BATT_MISC_EVENT_WIRELESS_OVERHEAT, BATT_MISC_EVENT_WIRELESS_OVERHEAT);
			power_supply_changed(battery->psy_bat);
		}
		return true;
	}
	return false;
}
#endif

int sec_bat_get_inbat_vol_ocv(struct sec_battery_info *battery)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	union power_supply_propval value = {0, };
	int j, k, ocv, jig_on = 0, ocv_data[10];
	int ret = 0;

	switch (pdata->inbat_ocv_type) {
	case SEC_BATTERY_OCV_FG_SRC_CHANGE:
		pr_info("%s: FGSRC_SWITCHING_VBAT\n", __func__);
		value.intval = SEC_BAT_INBAT_FGSRC_SWITCHING_VBAT;
		psy_do_property(pdata->fgsrc_switch_name, set,
				POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);

#if IS_ENABLED(CONFIG_FUELGAUGE_MAX77775)
			msleep(400);
#endif

		for (j = 0; j < 10; j++) {
#if IS_ENABLED(CONFIG_FUELGAUGE_MAX77775)
			msleep(200);
#else
			msleep(175);
#endif
			value.intval = SEC_BATTERY_VOLTAGE_MV;
			psy_do_property(pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			ocv_data[j] = value.intval;
		}

		ret = psy_do_property(pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_JIG_GPIO, value);
		if (value.intval < 0 || ret < 0)
			jig_on = 0;
		else if (value.intval == 0)
			jig_on = 1;

		if (battery->is_jig_on || sec_bat_get_facmode() || battery->factory_mode || jig_on) {
			pr_info("%s: FGSRC_SWITCHING_VSYS\n", __func__);
			value.intval = SEC_BAT_INBAT_FGSRC_SWITCHING_VSYS;
			psy_do_property(pdata->fgsrc_switch_name, set,
					POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);
		}

		for (j = 1; j < 10; j++) {
			ocv = ocv_data[j];
			k = j;
			while (k > 0 && ocv_data[k-1] > ocv) {
				ocv_data[k] = ocv_data[k-1];
				k--;
			}
			ocv_data[k] = ocv;
		}

		for (j = 0; j < 10; j++)
			pr_info("%s: [%d] %d\n", __func__, j, ocv_data[j]);

		ocv = 0;
		for (j = 2; j < 8; j++)
			ocv += ocv_data[j];

		ret = ocv / 6;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		/* just for debug */
		value.intval = SEC_DUAL_BATTERY_MAIN;
		psy_do_property(pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
		value.intval = SEC_DUAL_BATTERY_SUB;
		psy_do_property(pdata->dual_battery_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
#endif
		break;
	case SEC_BATTERY_OCV_FG_NOSRC_CHANGE:
		value.intval = SEC_BATTERY_VOLTAGE_MV;
		psy_do_property(pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		ret = value.intval;
		break;
	case SEC_BATTERY_OCV_ADC:
		/* run twice */
		ret = (sec_bat_get_inbat_vol_by_adc(battery) +
				sec_bat_get_inbat_vol_by_adc(battery)) / 2;
		break;
	case SEC_BATTERY_OCV_VOLT_FROM_PMIC:
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_EXT_PROP_PMIC_BAT_VOLTAGE,
				value);
		ret = value.intval;
		break;
	case SEC_BATTERY_OCV_NONE:
	default:
		break;
	};

	pr_info("%s: [%d] in-battery voltage ocv(%d)\n",
		__func__, pdata->inbat_ocv_type, ret);

	return ret;
}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
void sec_bat_dual_battery_check_eoc(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DUAL_SBP)
	union power_supply_propval value = {0, };
	int topoff_soc_ref = 1000;

	if (!battery->pdata->dual_battery_individual_topoff)
		return;

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING)
		topoff_soc_ref = 700;

	if (battery->main_capacity >= topoff_soc_ref &&
		battery->current_avg_main <= battery->pdata->full_check_current_2nd / 2) {
		value.intval = SEC_DUAL_BATTERY_MAIN;
		psy_do_property(battery->pdata->dual_battery_name, set,
			POWER_SUPPLY_EXT_PROP_CHECK_EOC, value);
	}

	if (battery->sub_capacity >= topoff_soc_ref &&
		battery->current_avg_sub <= battery->pdata->full_check_current_2nd / 2) {
		value.intval = SEC_DUAL_BATTERY_SUB;
		psy_do_property(battery->pdata->dual_battery_name, set,
			POWER_SUPPLY_EXT_PROP_CHECK_EOC, value);
	}

	pr_info("%s: m_cap: %d, m_iavg: %d, s_cap: %d, s_iavg: %d, topoff: %d, topoff_soc_ref: %d\n", __func__,
		battery->main_capacity, battery->current_avg_main, battery->sub_capacity, battery->current_avg_sub,
		battery->pdata->full_check_current_2nd / 2, topoff_soc_ref);
#else
	pr_info("%s: Bring up of the DUAL SBP function is required.\n", __func__);
#endif
}

static void sec_bat_check_dual_battery_balance(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DUAL_SBP)
	union power_supply_propval value = {0, };

	if (battery->status == POWER_SUPPLY_STATUS_CHARGING ||
		battery->status == POWER_SUPPLY_STATUS_FULL) {
		return;
	}

	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_SBP_VOLTAGE_NOW, value);
	battery->voltage_now_main = value.intval;

	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_DUAL_SBP_VOLTAGE_NOW, value);
	battery->voltage_now_sub = value.intval;

	pr_info("%s: voltage_now_main(%d), voltage_now_sub(%d), balance_threshold(%d)\n",
		__func__, battery->voltage_now_main, battery->voltage_now_sub,
		battery->pdata->dual_battery_balance_threshold);
	if (battery->voltage_now_main > battery->pdata->dual_battery_balance_threshold &&
		battery->voltage_now_sub > battery->pdata->dual_battery_balance_threshold) {
		value.intval = 1;
		psy_do_property(battery->pdata->dual_battery_name, set,
			POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED, value);
	} else {
		value.intval = 0;
		psy_do_property(battery->pdata->dual_battery_name, set,
			POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED, value);
	}
#else
	pr_info("%s: Bring up of the DUAL SBP function is required.\n", __func__);
#endif
}
#endif

int sec_bat_dual_battery_vbat(struct sec_battery_info *battery, int battery_type)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	union power_supply_propval value = {0, };
	int ret = 0;

	switch(battery_type) {
	case SEC_DUAL_BATTERY_MAIN:
		pr_info("%s: MAIN FGSRC_SWITCHING_VBAT\n", __func__);
		value.intval = SEC_BAT_INBAT_FGSRC_SWITCHING_VBAT;
		psy_do_property(pdata->main_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);

		msleep(250);

		value.intval = battery_type;
		psy_do_property(pdata->dual_battery_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		ret = value.intval;

		if (battery->is_jig_on || sec_bat_get_facmode() || battery->factory_mode) {
			pr_info("%s: MAIN FGSRC_SWITCHING_VSYS\n", __func__);
			value.intval = SEC_BAT_INBAT_FGSRC_SWITCHING_VSYS;
			psy_do_property(pdata->main_limiter_name, set,
					POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);
		}
		break;
	case SEC_DUAL_BATTERY_SUB:
		pr_info("%s:SUB FGSRC_SWITCHING_VBAT\n", __func__);
		value.intval = SEC_BAT_INBAT_FGSRC_SWITCHING_VBAT;
		psy_do_property(pdata->sub_limiter_name, set,
				POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);

		msleep(350);

		value.intval = battery_type;
		psy_do_property(pdata->dual_battery_name, get,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		ret = value.intval;

		if (battery->is_jig_on || sec_bat_get_facmode() || battery->factory_mode) {
			pr_info("%s: SUB FGSRC_SWITCHING_VSYS\n", __func__);
			value.intval = SEC_BAT_INBAT_FGSRC_SWITCHING_VSYS;
			psy_do_property(pdata->sub_limiter_name, set,
					POWER_SUPPLY_EXT_PROP_FGSRC_SWITCHING, value);
		}
		break;
	default:
		break;
	};

	pr_info("%s: [%d] Vbat(%d)\n", __func__, battery_type, ret);

	return ret;

}
#endif

#if !defined(CONFIG_SEC_FACTORY)
bool sec_bat_is_unknown_wpc_temp(
	int wpc_temp, int usb_temp, bool loosened_unknown_temp)
{
	if (loosened_unknown_temp) {
		if (usb_temp - wpc_temp >= 250)
			return true;
	} else {
		if (wpc_temp <= (-200))
			return true;
	}
	return false;
}
EXPORT_SYMBOL(sec_bat_is_unknown_wpc_temp);

void sec_bat_calc_unknown_wpc_temp(
	struct sec_battery_info *battery, int *batt_temp, int wpc_temp, int usb_temp)
{
	if ((battery->pdata->wpc_thm_info.source != SEC_BATTERY_THERMAL_SOURCE_NONE) &&
		!is_wireless_all_type(battery->cable_type)) {
		if (sec_bat_is_unknown_wpc_temp(wpc_temp, usb_temp,
								battery->pdata->loosened_unknown_temp)) {
			if (usb_temp >= 270)
				*batt_temp = (usb_temp + 60) > 900 ? 900 : (usb_temp + 60);
			else if (usb_temp <= 210)
				*batt_temp = (usb_temp - 50) < (-200) ? (-200) : (usb_temp - 50);
			else
				*batt_temp = (170 * usb_temp - 26100) / 60;
			pr_info("%s : Tbat(wpc_temp:%d) to (%d)\n", __func__, wpc_temp, *batt_temp);
		}
	}
}
EXPORT_SYMBOL(sec_bat_calc_unknown_wpc_temp);

/* Linear Regression for Predicting Battery Temperature */
int lr_predict_bat_temp(struct sec_battery_info *battery, int batt_temp, int sub_bat_temp)
{
	int bat_t = 0, bat_t_1 = 0; /* bat_t = bat_v(t), bat_t_1 = bat_v(t_1) */
	int lr_delta = battery->pdata->lr_delta;
	struct timespec64 ts = {0, };

	ts = ktime_to_timespec64(ktime_get_boottime());

	/* this temperature prediction formula has high accuracy at room temperature */
	if (batt_temp <= 250 || sub_bat_temp <= 0) {
		battery->lr_bat_temp = 0x7FFF;
		battery->lr_start_time = 0;
		battery->lr_time_span = 0;

		return batt_temp;
	}

	battery->lr_time_span = ts.tv_sec - battery->lr_start_time;
	battery->lr_start_time = battery->lr_start_time + battery->lr_time_span;

	if (battery->lr_time_span > 60)
		battery->lr_time_span = 60;
	if (battery->lr_time_span < 1)
		battery->lr_time_span = 1;

	if (battery->lr_bat_temp == 0x7FFF) {
		/* init bat_t_1 as bat(0) */
		bat_t_1 = (sub_bat_temp*battery->pdata->lr_param_init_sub_bat_thm + batt_temp*battery->pdata->lr_param_init_bat_thm)/100;
	} else
		bat_t_1 = battery->lr_bat_t_1; /* bat_v(t) as previous bat_v(t_1) */

	/*
	 * calculate bat_v(t)
	 * bat_v(0) = bat_v
	 * bat_v(t) = bat_v(t_1) + ((raw - bat_v(t_1))*0.016*time_span+0.5)
	 * lrp_temp = A*bat_v(t) + B*bat
	 */
	bat_t = ((bat_t_1*1000) + ((batt_temp - bat_t_1)*lr_delta*battery->lr_time_span+battery->pdata->lr_round_off))/1000;
	battery->lr_bat_temp = (battery->pdata->lr_param_bat_thm*bat_t + battery->pdata->lr_param_sub_bat_thm*sub_bat_temp)/1000;

	if ((is_wireless_all_type(battery->cable_type) ||
		battery->misc_event & BATT_MISC_EVENT_WIRELESS_DET_LEVEL) &&
		battery->lr_bat_temp >= 350)
		battery->lr_bat_temp = battery->lr_bat_temp - 10;

	pr_info("%s : LRP: %d, lrp_temp_raw: %d wpc_temp: %d lrp_bat_t_1: %d lrp_bat_v: %d time_span: %lds\n",
		__func__, battery->lr_bat_temp, batt_temp, sub_bat_temp, bat_t_1, bat_t, battery->lr_time_span);

	battery->lr_bat_t_1 = bat_t;

	return battery->lr_bat_temp;
}
EXPORT_SYMBOL(lr_predict_bat_temp);
#endif

int sec_bat_get_temperature(struct device *dev, struct sec_bat_thm_info *info, int old_val,
		char *chg_name, char *fg_name, int adc_read_type)
{
	struct sec_battery_info *battery
			= dev_get_drvdata(dev);
	union power_supply_propval value = {0, };
	int temp = old_val;
	int adc;

	if (info->check_type == SEC_BATTERY_TEMP_CHECK_FAKE)
		return FAKE_TEMP;

	if (info->test > -300 && info->test < 3000) {
		pr_info("%s : temperature test %d\n", __func__, info->test);
		return info->test;
	}
	/* get battery thm info */
	switch (info->source) {
	case SEC_BATTERY_THERMAL_SOURCE_FG:
		psy_do_property(fg_name, get, POWER_SUPPLY_PROP_TEMP, value);
		temp = value.intval;
		break;
	case SEC_BATTERY_THERMAL_SOURCE_CALLBACK:
		pr_info("%s : need to implement\n", __func__);
		break;
	case SEC_BATTERY_THERMAL_SOURCE_ADC:
		adc = sec_bat_get_adc_data(dev, info->channel, info->check_count, adc_read_type);
		if (adc >= 0) {
			if (info->adc_rsense > 0) {
				if (!psy_do_property(chg_name, get, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value)) {
					if (is_wired_type(battery->cable_type) && battery->charging_passed_time >= 5) {
						adc += (value.intval * info->adc_rsense) / 1000;
					}
				}
			}
			info->adc = adc;
			sec_bat_convert_adc_to_val(adc, info->offset,
					info->adc_table, info->adc_table_size, &temp);
		}
		break;
	case SEC_BATTERY_THERMAL_SOURCE_CHG_ADC:
		if (!psy_do_property(chg_name, get, POWER_SUPPLY_PROP_TEMP, value)) {
			info->adc = value.intval;
			sec_bat_convert_adc_to_val(value.intval, info->offset,
					info->adc_table, info->adc_table_size, &temp);
		}
		break;
	case SEC_BATTERY_THERMAL_SOURCE_FG_ADC:
#if IS_ENABLED(CONFIG_MTK_CHARGER)
		value.intval = SEC_BATTERY_TEMP_ADC;
#endif
		if (!psy_do_property(fg_name, get, POWER_SUPPLY_PROP_TEMP, value)) {
			info->adc = value.intval;
			sec_bat_convert_adc_to_val(value.intval, info->offset,
					info->adc_table, info->adc_table_size, &temp);
		}
		break;
	default:
		break;
	}

	return temp;
}
EXPORT_SYMBOL(sec_bat_get_temperature);


int sec_bat_adjust_temperature(struct sec_battery_info *battery, int read_temp, int prev_temp)
{
	int ret = read_temp;
	static bool temp_init;

	if (temp_init == false) {
		temp_init = true;

		return ret;
	}

	if ((read_temp - prev_temp) > battery->pdata->batt_temp_adj_gap_inc)
		ret = prev_temp + battery->pdata->batt_temp_adj_gap_inc;

	pr_info("%s: read: %d, prev: %d, now: %d\n",
			__func__, read_temp, prev_temp, ret);
	return ret;
}

__visible_for_testing void sec_bat_get_temperature_info(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	static bool shipmode_en = false;
	int batt_temp = battery->temperature;
	int usb_temp = battery->usb_temp;
	int chg_temp = battery->chg_temp;
	int dchg_temp = battery->dchg_temp;
	int wpc_temp = battery->wpc_temp;
	int sub_bat_temp = battery->sub_bat_temp;
	int blkt_temp = battery->blkt_temp;
	char *fg_name = battery->pdata->fuelgauge_name;
	char *chg_name = battery->pdata->charger_name;

	/* get battery thm info */
	batt_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->bat_thm_info, batt_temp,
			chg_name, fg_name, battery->pdata->adc_read_type);
	/* get usb thm info */
	usb_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->usb_thm_info, usb_temp,
			chg_name, fg_name, battery->pdata->adc_read_type);
	/* get chg thm info */
	chg_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->chg_thm_info, chg_temp,
			chg_name, fg_name, battery->pdata->adc_read_type);
	/* get wpc thm info */
	wpc_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->wpc_thm_info, wpc_temp,
			chg_name, fg_name, battery->pdata->adc_read_type);
	/* get sub_bat thm info */
	sub_bat_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->sub_bat_thm_info, sub_bat_temp,
			chg_name, fg_name, battery->pdata->adc_read_type);
#if !defined(CONFIG_SEC_FACTORY)
	if (battery->pdata->lr_enable)
		batt_temp = lr_predict_bat_temp(battery, batt_temp, sub_bat_temp);
#endif
	/* get blkt thm info */
	blkt_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->blk_thm_info, blkt_temp,
			chg_name, fg_name, battery->pdata->adc_read_type);

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	if (battery->pdata->dctp_by_cgtp)
		dchg_temp = chg_temp;
	else if (is_pd_apdo_wire_type(battery->wire_status))
		dchg_temp = sec_bat_get_temperature(battery->dev, &battery->pdata->dchg_thm_info,
				dchg_temp, chg_name, fg_name, battery->pdata->adc_read_type);
#endif

#if !defined(CONFIG_SEC_FACTORY)
	if (battery->pdata->sub_temp_control_source == TEMP_CONTROL_SOURCE_WPC_THM)
		sec_bat_calc_unknown_wpc_temp(battery, &sub_bat_temp, wpc_temp, usb_temp);
	else
		sec_bat_calc_unknown_wpc_temp(battery, &batt_temp, wpc_temp, usb_temp);
#endif

	if (battery->pdata->batt_temp_adj_gap_inc)
		battery->temperature = sec_bat_adjust_temperature(battery,
			batt_temp, battery->temperature);
	else
		battery->temperature = batt_temp;

	if (battery->pdata->lrpts_by_batts)
		battery->lrp = battery->temperature;

	battery->temper_amb = batt_temp;
	battery->usb_temp = usb_temp;
	battery->chg_temp = chg_temp;
	battery->dchg_temp = dchg_temp;
	battery->wpc_temp = wpc_temp;
	battery->sub_bat_temp = sub_bat_temp;
	battery->blkt_temp = blkt_temp;

	value.intval = battery->temperature;
#if defined(CONFIG_SEC_FACTORY)
	if (battery->pdata->usb_thm_info.check_type &&
		(battery->temperature <= (-200))) {
		value.intval = (battery->usb_temp <= (-200) ? battery->chg_temp : battery->usb_temp);
	}
#endif
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_TEMP, value);

	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_PROP_TEMP_AMBIENT, value);

#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	value.intval = sub_bat_temp;
	psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_EXT_PROP_SUB_BAT_TEMP, value);
#endif

	if (battery->pdata->en_auto_shipmode_temp_ctrl) {
		if (battery->temperature < 0 && !shipmode_en) {
			value.intval = 0;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_AUTO_SHIPMODE_CONTROL, value);
			shipmode_en = true;
		} else if (battery->temperature >= 50 && shipmode_en) {
			value.intval = 1;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_AUTO_SHIPMODE_CONTROL, value);
			shipmode_en = false;
		}
	}
}
EXPORT_SYMBOL_KUNIT(sec_bat_get_temperature_info);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
void sec_bat_get_dual_battery_voltage_info(struct sec_battery_info *battery, char *str)
{
	union power_supply_propval value = {0, };

#if IS_ENABLED(CONFIG_DUAL_SBP)
	/* get main voltage */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_SBP_VOLTAGE_NOW, value);
	battery->voltage_now_main = value.intval;

	/* get sub voltage */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_DUAL_SBP_VOLTAGE_NOW, value);
	battery->voltage_now_sub = value.intval;
#else
	/* get main voltage */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
	battery->voltage_now_main = value.intval;

	/* get sub voltage */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
	battery->voltage_now_sub = value.intval;
#endif
	/* get main voltage */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
	battery->voltage_avg_main = value.intval;

	/* get sub voltage */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
	battery->voltage_avg_sub = value.intval;

	snprintf(str, BUFF_SIZE, "Vm(%dmV),Vavgm(%dmV),Vs(%dmV),Vavgs(%dmV),",
		battery->voltage_now_main, battery->voltage_avg_main,
		battery->voltage_now_sub, battery->voltage_avg_sub
	);

#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	/* get sub chgin voltage */
	psy_do_property(battery->pdata->sub_limiter_name, get, POWER_SUPPLY_EXT_PROP_CHG_VOLTAGE, value);
	battery->chgin_sub = value.intval;
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), "Chgin_s(%dmV),", battery->chgin_sub);
#endif
}

void sec_bat_get_dual_battery_current_info(struct sec_battery_info *battery, char *str)
{
	union power_supply_propval value = {0, };

#if IS_ENABLED(CONFIG_DUAL_SBP)
	/* get main current */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_SBP_CURRENT_NOW, value);
	battery->current_now_main = value.intval;

	/* get sub current */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_SBP_CURRENT_NOW, value);
	battery->current_now_sub = value.intval;

	/* get main current */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_SBP_CURRENT_AVG, value);
	battery->current_avg_main = value.intval;

	/* get sub current */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_DUAL_SBP_CURRENT_AVG, value);
	battery->current_avg_sub = value.intval;
#else
	/* get main current */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now_main = value.intval;

	/* get sub current */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now_sub = value.intval;

	/* get main current */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg_main = value.intval;

	/* get sub current */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg_sub = value.intval;
#endif
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str),
		"Inow(%dmA),Iavg(%dmA),Isysavg(%dmA),Inow_m(%dmA),Iavg_m(%dmA),Inow_s(%dmA),Iavg_s(%dmA),Imax(%dmA),Ichg(%dmA),Ichg_m(%dmA),Ichg_s(%dmA)",
		battery->current_now, battery->current_avg, battery->current_sys_avg,
		battery->current_now_main, battery->current_avg_main,
		battery->current_now_sub, battery->current_avg_sub,
		battery->current_max, battery->charging_current,
		battery->main_current, battery->sub_current
	);
}
#if (IS_ENABLED(CONFIG_DUAL_FUELGAUGE) || IS_ENABLED(CONFIG_DUAL_SBP))
void sec_bat_get_dual_battery_soc_info(struct sec_battery_info *battery, char *str)
{
	union power_supply_propval value = {0, };

	/* get main soc */
	value.intval = SEC_DUAL_BATTERY_MAIN;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_EXT_PROP_FG_SOC, value);
	battery->main_capacity = value.intval;

	/* get sub soc */
	value.intval = SEC_DUAL_BATTERY_SUB;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_EXT_PROP_FG_SOC, value);
	battery->sub_capacity = value.intval;

	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), "MSOC(%d.%d%%),SSOC(%d.%d%%),",
		battery->main_capacity/10, battery->main_capacity%10,
		battery->sub_capacity/10, battery->sub_capacity%10
	);
}
#endif
#endif

void sec_bat_get_battery_voltage_info(struct sec_battery_info *battery, char *str)
{
	union power_supply_propval value = {0, };

	value.intval = SEC_BATTERY_VOLTAGE_MV;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
	battery->voltage_now = value.intval;

	value.intval = SEC_BATTERY_VOLTAGE_MV;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
	battery->voltage_avg = value.intval;

	/* Do not call it to reduce time after cable_work, this function call FG full log */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL)) {
		value.intval = SEC_BATTERY_VOLTAGE_MV;
		psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_VOLTAGE_OCV, value);
		battery->voltage_ocv = value.intval;
	}

	snprintf(str, BUFF_SIZE, "Vnow(%dmV),Vavg(%dmV),", battery->voltage_now, battery->voltage_avg);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	sec_bat_get_dual_battery_voltage_info(battery, str);
#endif
}

void sec_bat_get_battery_current_info(struct sec_battery_info *battery, char *str)
{
	union power_supply_propval value = {0, };

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_CURRENT_NOW, value);
	battery->current_now = value.intval;

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_CURRENT_AVG, value);
	battery->current_avg = value.intval;

#if IS_ENABLED(CONFIG_FUELGAUGE_MAX77705) || IS_ENABLED(CONFIG_FUELGAUGE_MAX77775)
	value.intval = SEC_BATTERY_ISYS_AVG_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
	battery->current_sys_avg = value.intval;

	value.intval = SEC_BATTERY_ISYS_MA;
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
	battery->current_sys = value.intval;
#endif

	/* input current limit in charger */
	psy_do_property(battery->pdata->charger_name, get, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);
	battery->current_max = value.intval;

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	sec_bat_get_dual_battery_current_info(battery, str);
#else
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str),
		"Inow(%dmA),Iavg(%dmA),Isysavg(%dmA),Imax(%dmA),Ichg(%dmA),",
		battery->current_now, battery->current_avg, battery->current_sys_avg,
		battery->current_max, battery->charging_current
	);
#endif
}

void sec_bat_get_battery_soc_info(struct sec_battery_info *battery, char *str)
{
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_CHARGE_COUNTER, value);
	battery->charge_counter = value.intval;

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	value.intval = (battery->status == POWER_SUPPLY_STATUS_FULL) ? 100 : battery->capacity;

	/* check abnormal status for wireless charging */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL) &&
		(is_wireless_type(battery->cable_type) || battery->wc_tx_enable)) {
		psy_do_property(battery->pdata->wireless_charger_name, set, POWER_SUPPLY_PROP_ENERGY_NOW, value);
	}
	psy_do_property(battery->pdata->wireless_charger_name, set, POWER_SUPPLY_PROP_CAPACITY, value);
#endif

	/* To get SOC value (NOT raw SOC), need to reset value */
	/* if the battery status was full, and SOC wasn't 100% yet,
		then ignore FG SOC, and report (previous SOC +1)% */
	battery->capacity = sec_bat_get_batt_capacity(battery, battery->capacity);
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), "SOC(%d%%),", battery->capacity);

#if (IS_ENABLED(CONFIG_DUAL_FUELGAUGE) || IS_ENABLED(CONFIG_DUAL_SBP))
	sec_bat_get_dual_battery_soc_info(battery, str);
#endif
}

void sec_bat_get_battery_temperature_info(struct sec_battery_info *battery, char *str)
{
	sec_bat_get_temperature_info(battery);
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	sec_bat_check_dual_battery_balance(battery);
#endif

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), "Tsub(%d),", battery->sub_bat_temp);
#endif
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), "Tbat(%d),Tusb(%d),Tchg(%d),Twpc(%d),Tblkt(%d),Tlrp(%d)",
		battery->temperature, battery->usb_temp, battery->chg_temp,
		battery->wpc_temp, battery->blkt_temp, battery->lrp
	);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), ",Tdchg(%d)\n", battery->dchg_temp);
#else
	snprintf(str + strlen(str), BUFF_SIZE - strlen(str), "\n");
#endif
}

void sec_bat_get_battery_info(struct sec_battery_info *battery)
{
	char str[BUFF_SIZE] = {0, };

	sec_bat_get_battery_voltage_info(battery, str);
	sec_bat_get_battery_current_info(battery, str);
	sec_bat_get_battery_soc_info(battery, str);
	sec_bat_get_battery_temperature_info(battery, str);

	pr_info("%s:%s", __func__, str);

#if IS_ENABLED(CONFIG_SEC_DEBUG) && defined(CONFIG_ARCH_QCOM) && !(defined(CONFIG_ARCH_EXYNOS) || defined(CONFIG_ARCH_MEDIATEK))
	if (!strcmp(battery->pdata->chip_vendor, "QCOM"))
		battery_last_dcvs(battery->capacity, battery->voltage_avg,
				battery->temperature, battery->current_avg);
#endif
#if IS_ENABLED(CONFIG_SEC_DEBUG) && IS_ENABLED(CONFIG_SEC_DEBUG_EXTRA_INFO) && !defined(CONFIG_ARCH_MTK_PROJECT)
	if (!strcmp(battery->pdata->chip_vendor, "LSI"))
		secdbg_exin_set_batt(battery->capacity, battery->voltage_avg,
				battery->temperature, battery->current_avg);
#endif
}
EXPORT_SYMBOL(sec_bat_get_battery_info);

__visible_for_testing void sec_bat_polling_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(
		work, struct sec_battery_info, polling_work.work);

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	dev_dbg(battery->dev, "%s: Activated\n", __func__);
}
EXPORT_SYMBOL_KUNIT(sec_bat_polling_work);

static void sec_bat_program_alarm(
				struct sec_battery_info *battery, int seconds)
{
	alarm_start(&battery->polling_alarm,
			ktime_add(battery->last_poll_time, ktime_set(seconds, 0)));
}

static unsigned int sec_bat_get_polling_time(
	struct sec_battery_info *battery)
{
	if (battery->status == POWER_SUPPLY_STATUS_FULL)
		battery->polling_time = battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_CHARGING];
	else
		battery->polling_time =	battery->pdata->polling_time[battery->status];

	battery->polling_short = true;

	switch (battery->status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		if (battery->polling_in_sleep)
			battery->polling_short = false;
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		if (battery->polling_in_sleep)
			battery->polling_time =	battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_SLEEP];
		else
			battery->polling_time =	battery->pdata->polling_time[battery->status];

		mutex_lock(&battery->wclock);
		if (!battery->wc_enable) {
			battery->polling_time = battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_CHARGING];
			pr_info("%s: wc_enable is false, polling time is 30sec\n", __func__);
		}
		mutex_unlock(&battery->wclock);
		if (battery->d2d_auth == D2D_AUTH_SRC) {
			battery->polling_time = battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_CHARGING];
			pr_info("%s: hp d2d, polling time is 30sec\n", __func__);
		}

		battery->polling_short = false;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		if (battery->polling_in_sleep) {
			if (!(battery->pdata->full_condition_type & SEC_BATTERY_FULL_CONDITION_NOSLEEPINFULL) &&
				battery->charging_mode == SEC_BATTERY_CHARGING_NONE)
				battery->polling_time =	battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_SLEEP];
			battery->polling_short = false;
		} else {
			if (battery->charging_mode == SEC_BATTERY_CHARGING_NONE)
				battery->polling_short = false;
		}
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE ||
			(battery->health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE)) &&
			(battery->health_check_count > 0)) {
			battery->health_check_count--;
			battery->polling_time = 1;
			battery->polling_short = false;
		}
		break;
	}

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->wc_tx_enable) {
		battery->polling_time = 10;
		battery->polling_short = false;
		pr_info("%s: Tx mode enable polling time is 10sec\n", __func__);
	}
#endif

	if (is_pd_apdo_wire_type(battery->cable_type) &&
		(battery->pd_list.now_isApdo || battery->ta_alert_mode != OCP_NONE)) {
		battery->polling_time = 10;
		battery->polling_short = false;
		pr_info("%s: DC mode enable polling time is 10sec\n", __func__);
	}

	if (battery->polling_short)
		return battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_BASIC];

	return battery->polling_time;
}

__visible_for_testing bool sec_bat_is_short_polling(struct sec_battery_info *battery)
{
	/*
	 * Change the full and short monitoring sequence
	 * Originally, full monitoring was the last time of polling_count
	 * But change full monitoring to first time
	 * because temperature check is too late
	 */
	if (!battery->polling_short || battery->polling_count == 1)
		return false;
	else
		return true;
}
EXPORT_SYMBOL_KUNIT(sec_bat_is_short_polling);

static void sec_bat_update_polling_count(struct sec_battery_info *battery)
{
	/*
	 * do NOT change polling count in sleep
	 * even though it is short polling
	 * to keep polling count along sleep/wakeup
	 */
	if (battery->polling_short && battery->polling_in_sleep)
		return;

	if (battery->polling_short &&
			((battery->polling_time / battery->pdata->polling_time[SEC_BATTERY_POLLING_TIME_BASIC])	>
			battery->polling_count))
		battery->polling_count++;
	else
		battery->polling_count = 1;	/* initial value = 1 */
}

__visible_for_testing int sec_bat_set_polling(struct sec_battery_info *battery)
{
	unsigned int polling_time_temp = 0;

	dev_dbg(battery->dev, "%s: Start\n", __func__);

	polling_time_temp = sec_bat_get_polling_time(battery);

	dev_dbg(battery->dev, "%s: Status:%s, Sleep:%s, Charging:%s, Short Poll:%s\n",
		__func__, sb_get_bst_str(battery->status),
		battery->polling_in_sleep ? "Yes" : "No",
		(battery->charging_mode ==
		SEC_BATTERY_CHARGING_NONE) ? "No" : "Yes",
		battery->polling_short ? "Yes" : "No");
	dev_info(battery->dev, "%s: Polling time %d/%d sec.\n", __func__,
		battery->polling_short ?
		(polling_time_temp * battery->polling_count) :
		polling_time_temp, battery->polling_time);

	/*
	 * To sync with log above,
	 * change polling count after log is displayed
	 * Do NOT update polling count in initial monitor
	 */
	if (!battery->pdata->monitor_initial_count)
		sec_bat_update_polling_count(battery);
	else
		dev_dbg(battery->dev,
			"%s: Initial monitor %d times left.\n", __func__,
			battery->pdata->monitor_initial_count);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		if (battery->pdata->monitor_initial_count) {
			battery->pdata->monitor_initial_count--;
			schedule_delayed_work(&battery->polling_work, HZ);
		} else
			schedule_delayed_work(&battery->polling_work,
				polling_time_temp * HZ);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		battery->last_poll_time = ktime_get_boottime();

		if (battery->pdata->monitor_initial_count) {
			battery->pdata->monitor_initial_count--;
			sec_bat_program_alarm(battery, 1);
		} else
			sec_bat_program_alarm(battery, polling_time_temp);
		break;
	case SEC_BATTERY_MONITOR_TIMER:
		break;
	default:
		break;
	}
	dev_dbg(battery->dev, "%s: End\n", __func__);

	return polling_time_temp;
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_polling);

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
extern bool get_usb_enumeration_state(void);
#else
static bool get_usb_enumeration_state(void)
{
	return true;
}
#endif
/* To display slow charging when usb charging 100MA*/
__visible_for_testing void sec_bat_check_slowcharging_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, slowcharging_work.work);

	if (battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT &&
		battery->cable_type == SEC_BATTERY_CABLE_USB) {
		if (!get_usb_enumeration_state() &&
			(battery->current_event & SEC_BAT_CURRENT_EVENT_USB_100MA)) {
			battery->usb_slow_chg = true;
			battery->max_charge_power = mW_by_mVmA(battery->input_voltage, battery->current_max);
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

	pr_info("%s:\n", __func__);
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_slowcharging_work);

static int sec_bat_chk_siop_scenario_idx(struct sec_battery_info *battery,
	int siop_level)
{
	int scenario_idx = -1;
	int i = 0;

	if (battery->pdata->siop_scenarios_num > 0) {
		for (i = 0; i < battery->pdata->siop_scenarios_num; ++i)
			if (siop_level == battery->pdata->siop_table[i].level) {
				scenario_idx = i;
				pr_info("%s: siop level(%d), scenario_idx(%d)\n",
					__func__, siop_level, scenario_idx);
				break;
			}
	}

	return scenario_idx;
}

static bool sec_bat_chk_siop_skip_scenario(struct sec_battery_info *battery,
	int ct, int ws, int scenario_idx)
{
	struct sec_siop_table *siop_table;
	int type = SIOP_CURR_TYPE_NV;

	if (scenario_idx < 0)
		return false;

	if (is_hv_wireless_type(ct))
		type = SIOP_CURR_TYPE_WPC_HV;
	else if (is_nv_wireless_type(ct))
		type = SIOP_CURR_TYPE_WPC_NV;
	else if (is_hv_wire_type(ct) && is_hv_wire_type(ws))
		type = SIOP_CURR_TYPE_HV;
	else if (is_pd_apdo_wire_type(ct) && battery->pd_list.now_isApdo)
		type = SIOP_CURR_TYPE_APDO;
	else if (is_pd_wire_type(ct))
		type = SIOP_CURR_TYPE_FPDO;

	if (type >= battery->pdata->siop_curr_type_num)
		return false;

	siop_table = &battery->pdata->siop_table[scenario_idx];
	if ((siop_table->icl[type] == SIOP_SKIP) && (siop_table->fcc[type] == SIOP_SKIP)) {
		pr_info("%s: scenario_idx(%d), type(%d), siop_icl(%d), siop_fcc(%d)\n",
		__func__, scenario_idx, type, siop_table->icl[type], siop_table->fcc[type]);
		return true;
	}

	return false;
}

static int sec_bat_get_siop_scenario_icl(struct sec_battery_info *battery, int icl, int scenario_idx, int type)
{
	int siop_icl = icl;

	if (scenario_idx < 0)
		return icl;

	if (type >= SIOP_CURR_TYPE_MAX || type >= battery->pdata->siop_curr_type_num)
		return icl;

	siop_icl = battery->pdata->siop_table[scenario_idx].icl[type];
	if (siop_icl == SIOP_DEFAULT || siop_icl > icl || siop_icl <= 0 ||
		siop_icl == SIOP_SKIP)
		return icl;
	pr_info("%s: scenario_idx(%d), type(%d), siop_icl(%d)\n",
		__func__, scenario_idx, type, siop_icl);

	return siop_icl;
}

static int sec_bat_get_siop_scenario_fcc(struct sec_battery_info *battery, int fcc, int scenario_idx, int type)
{
	int siop_fcc = fcc;

	if (scenario_idx < 0)
		return fcc;

	if (type >= SIOP_CURR_TYPE_MAX || type >= battery->pdata->siop_curr_type_num)
		return fcc;

	siop_fcc = battery->pdata->siop_table[scenario_idx].fcc[type];
	if (siop_fcc == SIOP_DEFAULT || siop_fcc > fcc || siop_fcc <= 0 ||
		siop_fcc == SIOP_SKIP)
		return fcc;
	pr_info("%s: scenario_idx(%d), type(%d), siop_fcc(%d)\n",
		__func__, scenario_idx, type, siop_fcc);

	return siop_fcc;
}

static void sec_bat_siop_level_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
			struct sec_battery_info, siop_level_work.work);
	sec_battery_platform_data_t *pdata = battery->pdata;

	int icl = INT_MAX;	/* Input Current Limit */
	int fcc = INT_MAX;	/* Fast Charging Current */
	int scenario_idx = -1;
	int skip_vote = 0;

	enum {
		SIOP_STEP1,	/* siop level 70 */
		SIOP_STEP2,	/* siop level 10 */
		SIOP_STEP3,	/* siop level 0 */
	};
	int siop_step = (battery->siop_level == 0) ?
		SIOP_STEP3 : ((battery->siop_level == 10) ? SIOP_STEP2 : SIOP_STEP1);

	pr_info("%s : set current by siop level(%d), siop_step(%d)\n", __func__, battery->siop_level, siop_step + 1);

	scenario_idx = sec_bat_chk_siop_scenario_idx(battery, battery->siop_level);

	if (sec_bat_chk_siop_skip_scenario(battery,
			battery->cable_type, battery->wire_status, scenario_idx) ||
			(battery->siop_level >= 80)) {
#if defined(CONFIG_SUPPORT_HV_CTRL)
		sec_vote(battery->iv_vote, VOTER_SIOP, false, 0);
#endif
		sec_vote(battery->fcc_vote, VOTER_SIOP, false, 0);
		sec_vote(battery->input_vote, VOTER_SIOP, false, 0);

		__pm_relax(battery->siop_level_ws);
		return;
	}

	if (is_hv_wireless_type(battery->cable_type)) {
		icl = pdata->siop_hv_wpc_icl;
		fcc = pdata->siop_hv_wpc_fcc[siop_step];
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_WPC_HV);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_WPC_HV);
		}
	} else if (is_nv_wireless_type(battery->cable_type)) {
		icl = pdata->siop_wpc_icl;
		fcc = pdata->siop_wpc_fcc[siop_step];
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_WPC_NV);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_WPC_NV);
		}
	} else if (is_hv_wire_12v_type(battery->cable_type) && is_hv_wire_type(battery->wire_status)) {
		icl = pdata->siop_hv_12v_icl;
		fcc = pdata->siop_hv_12v_fcc;
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_HV);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_HV);
		}
	} else if (is_hv_wire_type(battery->cable_type) && is_hv_wire_type(battery->wire_status)) {
#if defined(CONFIG_SUPPORT_HV_CTRL) && !defined(CONFIG_SEC_FACTORY)
		if (sec_bat_change_vbus(battery, battery->cable_type, battery->current_event, battery->siop_level)) {
			if (battery->misc_event & BATT_MISC_EVENT_HV_BY_AICL) {
				icl = pdata->siop_hv_icl;
				fcc = pdata->siop_hv_fcc;
			} else {
				icl = pdata->siop_icl;
				fcc = pdata->siop_fcc;
			}
		} else {
			skip_vote = 1;
		}
#else
		icl = (siop_step == SIOP_STEP3) ? pdata->siop_hv_icl_2nd : pdata->siop_hv_icl;
		fcc = pdata->siop_hv_fcc;
#endif
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_HV);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_HV);
		}
	} else if (is_pd_apdo_wire_type(battery->cable_type) && battery->pd_list.now_isApdo) {
		icl = pdata->siop_apdo_icl;
		fcc = pdata->siop_apdo_fcc;
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_APDO);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_APDO);
		}
	} else if (is_pd_wire_type(battery->cable_type)) {
#if defined(CONFIG_SUPPORT_HV_CTRL) && !defined(CONFIG_SEC_FACTORY)
		if (sec_bat_change_vbus(battery, battery->cable_type, battery->current_event, battery->siop_level)) {
			if (battery->misc_event & BATT_MISC_EVENT_HV_BY_AICL) {
				icl = mA_by_mWmV(pdata->power_value, SEC_INPUT_VOLTAGE_9V);
				fcc = pdata->siop_hv_fcc;
			} else {
				icl = mA_by_mWmV(pdata->power_value, SEC_INPUT_VOLTAGE_5V);
				fcc = pdata->siop_fcc;
			}
		} else {
			skip_vote = 1;
		}
#else
		icl = mA_by_mWmV(pdata->power_value, battery->input_voltage);
		fcc = pdata->siop_fcc;
#endif
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_FPDO);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_FPDO);
		}
	} else {
#if defined(CONFIG_SUPPORT_HV_CTRL)
		if (battery->wire_status != SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)
			sec_vote(battery->iv_vote, VOTER_SIOP, false, 0);
#endif
		icl = pdata->siop_icl;
		fcc = pdata->siop_fcc;
		if (battery->pdata->siop_scenarios_num > 0) {
			icl = sec_bat_get_siop_scenario_icl(battery, icl, scenario_idx, SIOP_CURR_TYPE_NV);
			fcc = sec_bat_get_siop_scenario_fcc(battery, fcc, scenario_idx, SIOP_CURR_TYPE_NV);
		}
	}

	if (!skip_vote) {
		pr_info("%s: icl(%d), fcc(%d)\n", __func__, icl, fcc);
		sec_vote(battery->input_vote, VOTER_SIOP, true, icl);
		sec_vote(battery->fcc_vote, VOTER_SIOP, true, fcc);
	}

	__pm_relax(battery->siop_level_ws);
}

#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
#define MFC_FW_UPDATE_MIN_CAP 20
__visible_for_testing void sec_bat_fw_init_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, fw_init_work.work);

#if defined(CONFIG_WIRELESS_IC_PARAM)
	union power_supply_propval value = {0, };

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_CHECK_FW_VER, value);
	if (value.intval) {
		pr_info("%s: wireless firmware is already updated.\n", __func__);
		return;
	}
#endif

	/* auto mfc firmware update when only no otg, mst, wpc, factory_mode */
	if (sec_bat_check_boost_mfc_condition(battery, SEC_WIRELESS_FW_UPDATE_AUTO_MODE) &&
		(battery->capacity >= MFC_FW_UPDATE_MIN_CAP) && !sec_bat_get_lpmode() && !sec_bat_get_facmode()) {
		sec_bat_fw_update(battery, SEC_WIRELESS_FW_UPDATE_AUTO_MODE);
	}
}
EXPORT_SYMBOL_KUNIT(sec_bat_fw_init_work);
#endif

#if defined(CONFIG_UPDATE_BATTERY_DATA)
static void sec_bat_update_data_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, batt_data_work.work);

	sec_battery_update_data(battery->data_path);
	__pm_relax(battery->batt_data_ws);
}
#endif

static void sec_bat_misc_event_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, misc_event_work.work);
	int xor_misc_event = battery->prev_misc_event ^ battery->misc_event;

	if (xor_misc_event & BATT_MISC_EVENT_MUIC_ABNORMAL) {
		if (battery->misc_event & BATT_MISC_EVENT_MUIC_ABNORMAL)
			sec_vote(battery->chgen_vote, VOTER_MUIC_ABNORMAL, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		else if (battery->prev_misc_event & BATT_MISC_EVENT_MUIC_ABNORMAL)
			sec_vote(battery->chgen_vote, VOTER_MUIC_ABNORMAL, false, 0);
	}

	pr_info("%s: change misc event(0x%x --> 0x%x)\n",
		__func__, battery->prev_misc_event, battery->misc_event);
	battery->prev_misc_event = battery->misc_event;
	__pm_relax(battery->misc_event_ws);

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
}

static int sec_bat_get_input_power(struct sec_battery_info *battery)
{
	int input_power = 0;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	int direct_chg_done = 0;
	union power_supply_propval value = {0,};
#endif

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_EXT_PROP_DIRECT_DONE, value);
	direct_chg_done = value.intval;

	if (is_pd_apdo_wire_type(battery->cable_type) &&
		!battery->chg_limit && !battery->lrp_limit &&
		(battery->pd_list.now_isApdo || direct_chg_done))
		input_power = battery->pd_max_charge_power;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	else if (is_wireless_type(battery->cable_type))
		input_power = mW_by_mVmA(get_wc_vout_mW(battery->wpc_vout_level), battery->current_max);
#endif
	else
		input_power = mW_by_mVmA(battery->input_voltage, battery->current_max);
#else
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	if (is_wireless_type(battery->cable_type))
		input_power = mW_by_mVmA(get_wc_vout_mW(battery->wpc_vout_level), battery->current_max);
	else
		input_power = mW_by_mVmA(battery->input_voltage, battery->current_max);
#else
		input_power = mW_by_mVmA(battery->input_voltage, battery->current_max);
#endif
#endif

	return input_power;
}

static int sec_bat_get_charging_power(struct sec_battery_info *battery)
{
	return mW_by_mVmA(battery->charging_current,
		(battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv));
}

#if defined(CONFIG_SEC_KUNIT)
int __mockable sec_bat_get_charging_curr_considering_power(struct sec_battery_info *battery)
#else
static int sec_bat_get_charging_curr_considering_power(struct sec_battery_info *battery)
#endif
{
	int input_power = 0, charging_power = 0, charging_curr = 0;

	input_power = sec_bat_get_input_power(battery);
	charging_power = sec_bat_get_charging_power(battery);

	if (input_power > charging_power) {
		charging_curr = battery->charging_current;
	} else {
		charging_curr = mA_by_mWmV(input_power, (battery->pdata->chg_float_voltage / battery->pdata->chg_float_voltage_conv));
		charging_curr = (charging_curr * 9) / 10;
	}

	if (charging_curr > battery->pdata->standard_curr) {
		charging_curr = battery->pdata->standard_curr;
	}

	pr_info("%s : EXPIRED_TIME(%ld)msec, IP(%d)mW, CP(%d)mW, CURR(%d)mA, STANDARD(%d)mA\n",
		__func__, battery->safety_timer.remaining_time, input_power, charging_power, charging_curr, battery->pdata->standard_curr);

	return charging_curr;
}

void sec_bat_reset_safety_timer(struct sec_battery_info *battery)
{
	pr_info("%s : SAFETY TIME RESET!\n", __func__);
	battery->safety_timer.remaining_time = battery->pdata->expired_time;
	battery->safety_timer.prev_time = 0;
}

#if defined(CONFIG_SEC_KUNIT)
bool __mockable sec_bat_check_discharged_with_charging(struct sec_battery_info *battery)
#else
static bool sec_bat_check_discharged_with_charging(struct sec_battery_info *battery)
#endif
{
	static int discharging_cnt;

	if (battery->current_avg < 0) {
		discharging_cnt++;
	} else {
		discharging_cnt = 0;
	}

	if (discharging_cnt >= 5) {
		discharging_cnt = 0;
		return true;
	}
	return false;
}

#if defined(CONFIG_SEC_KUNIT)
unsigned long __mockable sec_bat_get_current_time_sec(struct sec_battery_info *battery)
#else
static unsigned long sec_bat_get_current_time_sec(struct sec_battery_info *battery)
#endif
{
	struct timespec64 ts = {0, };
	ts = ktime_to_timespec64(ktime_get_boottime());

	return ts.tv_sec;
}

static void sec_bat_determin_pause_safety_timer(struct sec_battery_info *battery)
{
	if ((battery->lcd_status || battery->wc_tx_enable) && !battery->safety_timer.stop_timer) {
		battery->safety_timer.stop_timer = true;
	} else if (!(battery->lcd_status || battery->wc_tx_enable) && battery->safety_timer.stop_timer) {
		battery->safety_timer.stop_timer = false;
	}
}

#if defined(CONFIG_SEC_KUNIT)
__visible_for_testing void sec_bat_calculate_safety_time(struct sec_battery_info *battery)
#else
static void sec_bat_calculate_safety_time(struct sec_battery_info *battery)
#endif
{
	struct sec_safety_timer *safety_t = &battery->safety_timer;
	unsigned long long remaining_time = safety_t->remaining_time;
	unsigned long current_time = 0, elapsed_time = 0;
	int charging_curr = 0;

	if (sec_bat_check_discharged_with_charging(battery)) {
		sec_bat_reset_safety_timer(battery); /* reset safety timer */
		return;
	}

	if ((battery->lcd_status || battery->wc_tx_enable) && safety_t->stop_timer) {
		safety_t->prev_time = 0; /* pause safety timer */
		return;
	}

	current_time = sec_bat_get_current_time_sec(battery);
	if (safety_t->prev_time == 0) {
		safety_t->prev_time = current_time;
	}

	sec_bat_determin_pause_safety_timer(battery);
	charging_curr = sec_bat_get_charging_curr_considering_power(battery);
	if (charging_curr == 0) {
		return;
	}

	elapsed_time = current_time - safety_t->prev_time;
	remaining_time = (remaining_time * battery->pdata->standard_curr) / charging_curr;
	if (remaining_time <= (elapsed_time * 1000)) {
		remaining_time = 0; /* 0 means safety timer is expired */
	} else {
		remaining_time -= (elapsed_time * 1000);
	}
	safety_t->rescaled_remaining_time = remaining_time;
	safety_t->remaining_time = (remaining_time * charging_curr) / battery->pdata->standard_curr;
	safety_t->prev_time = current_time;

	pr_info("%s : REMAIN_TIME(%ld)msec, RESCALED_REMAIN_TIME(%ld)msec, ELAPSED_TIME(%ld)sec\n",
		__func__, safety_t->remaining_time,
		safety_t->rescaled_remaining_time, elapsed_time);
}

static bool sb_check_skip_monitor(struct sec_battery_info *battery)
{
	static struct timespec64 old_ts = {0, };
	struct timespec64 c_ts = {0, };
	union power_supply_propval val = {0, };

	c_ts = ktime_to_timespec64(ktime_get_boottime());

	/* monitor once after wakeup */
	if (battery->polling_in_sleep) {
		battery->polling_in_sleep = false;
		if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING &&
			!battery->wc_tx_enable &&
			(battery->d2d_auth != D2D_AUTH_SRC) &&
			((unsigned long)(c_ts.tv_sec - old_ts.tv_sec) < 10 * 60)
		) {
			val.intval = SEC_BATTERY_VOLTAGE_MV;
			psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_PROP_VOLTAGE_NOW, val);
			battery->voltage_now = val.intval;

			battery->capacity = sec_bat_get_batt_capacity(battery, battery->capacity);

			sec_bat_get_temperature_info(battery);
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
			sec_bat_check_dual_battery_balance(battery);
#endif
			sec_bat_cisd_check(battery);
			power_supply_changed(battery->psy_bat);
			pr_info("Skip monitor work(%llu, Vnow:%d(mV), SoC:%d(%%), Tbat:%d(0.1'C))\n",
				c_ts.tv_sec - old_ts.tv_sec, battery->voltage_now,
				battery->capacity, battery->temperature);
			return true;
		}
	}
	/* update last monitor time */
	old_ts = c_ts;
	return false;
}

static void sec_bat_check_store_mode(struct sec_battery_info *battery)
{

	if (sec_bat_get_facmode())
		return;
#if defined(CONFIG_SEC_FACTORY)
	if (!is_nocharge_type(battery->cable_type)) {
#else
	if (!is_nocharge_type(battery->cable_type) && battery->store_mode) {
#endif
		pr_info("%s: capacity(%d), status(%d), store_mode(%d)\n",
			__func__, battery->capacity, battery->status, battery->store_mode);
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		pr_info("%s: @battery->batt_f_mode: %s\n", __func__,
			BOOT_MODE_STRING[battery->batt_f_mode]);
#endif

		/*
		 * VOTER_STORE_MODE
		 * Set limited max power when store mode is set and LDU
		 * Limited max power should be set with over 5% capacity
		 * since target could be turned off during boot up
		 * display test requirement : do not decrease fcc in store mode condition
		 */
		if (!battery->display_test && battery->store_mode && battery->capacity >= 5) {
			sec_vote(battery->input_vote, VOTER_STORE_MODE, true,
				mA_by_mWmV(battery->pdata->store_mode_max_input_power, battery->input_voltage));
		}

		if (battery->capacity >= battery->pdata->store_mode_charging_max) {
			int chg_mode = SEC_BAT_CHG_MODE_CHARGING_OFF;
			/* to discharge the battery, off buck */
			if (battery->capacity > battery->pdata->store_mode_charging_max
					|| battery->pdata->store_mode_buckoff)
				chg_mode = SEC_BAT_CHG_MODE_BUCK_OFF;

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
			if ((sec_bat_get_facmode() || battery->batt_f_mode != NO_MODE) &&
				chg_mode == SEC_BAT_CHG_MODE_BUCK_OFF)
#else
			if (sec_bat_get_facmode() &&
				chg_mode == SEC_BAT_CHG_MODE_BUCK_OFF)
#endif
				chg_mode = SEC_BAT_CHG_MODE_CHARGING_OFF;

			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
			sec_vote(battery->chgen_vote, VOTER_STORE_MODE, true, chg_mode);
		}

		if (battery->capacity <= battery->pdata->store_mode_charging_min &&
				battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
			sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
			sec_vote(battery->chgen_vote, VOTER_STORE_MODE, false, 0);
		}
	}
}

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
static void sec_bat_check_boottime(void *data,
	bool dctp_en)
{
	struct sec_battery_info *battery = data;

	if (!dctp_en)
		return;

	if (battery->capacity > 10) {
		pr_info("%s: %d\n", __func__, battery->pdata->dctp_bootmode_en);
		battery->pdata->dctp_bootmode_en = false;
	}
}

static void sec_bat_d2d_check(struct sec_battery_info *battery,
	unsigned int capacity, int batt_t, int lrp_t)
{
	union power_supply_propval value = {0, };
	int auth = AUTH_LOW_PWR;

	if ((battery->pdata->d2d_check_type == SB_D2D_NONE) ||
		(battery->d2d_auth != D2D_AUTH_SRC) ||
		battery->vpdo_ocp) {
		return;
	}

	if (battery->pdata->d2d_check_type == SB_D2D_SRCSNK) {
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VOLTAGE, value);
		battery->vpdo_src_boost = value.intval ? true : false;
	}

	if (battery->vpdo_src_boost) {
		value.intval = SEC_BATTERY_VIN_MA;
		psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_D2D_REVERSE_OCP, value);

		battery->vpdo_ocp = value.intval ? true : false;
		pr_info("%s : reverse ocp(%d)\n", __func__, value.intval);
		if (battery->vpdo_ocp)
			battery->hp_d2d = HP_D2D_OCP;
	}

	if ((batt_t >= 500) || (batt_t <= 0)) {
		battery->hp_d2d = HP_D2D_BATT_TMP;
	} else if (lrp_t > 370) {
		battery->hp_d2d = HP_D2D_LRP_TMP;
	} else if (battery->vpdo_ocp) {
		battery->hp_d2d = HP_D2D_OCP;
	} else if (capacity < 30) {
		battery->hp_d2d = HP_D2D_SOC;
	} else {
		if (battery->hp_d2d == HP_D2D_BATT_TMP) {
			if ((batt_t <= 480) && (batt_t >= 20)) {
				auth = AUTH_HIGH_PWR;
				battery->hp_d2d = HP_D2D_ON;
			} else
				battery->vpdo_auth_stat = auth;
		} else {
			if (battery->lcd_status) {
				battery->hp_d2d = HP_D2D_LCD;
			} else {
				auth = AUTH_HIGH_PWR;
				battery->hp_d2d = HP_D2D_ON;
			}
		}
	}

	pr_info("%s : auth %s, hp d2d (%d)\n", __func__,
			(auth == AUTH_LOW_PWR) ? "LOW PWR" : "HIGH PWR", battery->hp_d2d);

	if (battery->vpdo_auth_stat != auth) {
		sec_pd_vpdo_auth(auth, battery->pdata->d2d_check_type);

		pr_info("%s : vpdo auth changed\n", __func__);
		battery->vpdo_auth_stat = auth;
	}
}

static void sec_bat_check_direct_charger(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };
	int ret = 0;

	if (battery->status != POWER_SUPPLY_STATUS_CHARGING ||
		battery->health != POWER_SUPPLY_HEALTH_GOOD) {
		battery->dc_check_cnt = 0;
		return;
	}

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_CHARGER_MODE_DIRECT, val);
	if (val.intval != SEC_BAT_CHG_MODE_CHARGING) {
		battery->dc_check_cnt = 0;
		return;
	}

	val.strval = "GETCHARGING";
	ret = psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_CHG_STATUS, val);
	if (ret < 0)
		return;

	if (strncmp(val.strval, "NO_CHARGING", 12) == 0) {
		pr_info("%s: cnt(%d)\n", __func__, battery->dc_check_cnt);
		if (++battery->dc_check_cnt < 3)
			return;
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_DC_ERR, SEC_BAT_CURRENT_EVENT_DC_ERR);
		sec_vote(battery->chgen_vote, VOTER_DC_ERR, true,
			SEC_BAT_CHG_MODE_CHARGING_OFF);
		msleep(100);
		sec_bat_set_current_event(battery,
			0, SEC_BAT_CURRENT_EVENT_DC_ERR);
		sec_vote(battery->chgen_vote, VOTER_DC_ERR, false, 0);
#if IS_ENABLED(CONFIG_SEC_ABC)
		sec_abc_send_event("MODULE=battery@WARN=dc_error");
#endif
	}
	battery->dc_check_cnt = 0;
}
#endif

static void sb_monitor_preliminary_checks(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	if (battery->set_lower_curr) {
		pr_info("%s: set lower limiter current\n", __func__);
		battery->set_lower_curr = false;
		sec_bat_set_limiter_current(battery);
	}
#endif
	sec_bat_get_battery_info(battery);
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	sec_bat_dual_battery_check_eoc(battery);
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	sec_bat_d2d_check(battery, battery->capacity, battery->temperature, battery->lrp);
#endif
	sec_bat_cisd_check(battery);
#if defined(CONFIG_STEP_CHARGING)
	sec_bat_check_step_charging(battery);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	sec_bat_check_wpc_step_charging(battery);
#endif
#endif
	/* time to full check */
	sec_bat_calc_time_to_full(battery);
	sec_bat_check_full_capacity(battery);
#if defined(CONFIG_WIRELESS_TX_MODE)
	/* tx mode check */
	sec_bat_check_tx_mode(battery);
#endif
#ifdef CONFIG_IFPMIC_LIMITER
	sec_bat_trace_vtrack(battery);
#endif
}

static bool sb_monitor_critical_checks(struct sec_battery_info *battery)
{
	bool checks_pass =
		!sec_bat_do_test_function(battery) &&	/* 0. test mode */
		sec_bat_battery_cable_check(battery) &&	/* 1. battery check */
		sec_bat_voltage_check(battery);			/* 2. voltage check */

	return checks_pass;
}

static void sb_monitor_nonurgent_checks(struct sec_battery_info *battery)
{
	/* 4. bat thm check */
	sec_bat_thermal_check(battery);

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	/* 4-1. wireless overheat check */
	if (battery->misc_event & BATT_MISC_EVENT_WIRELESS_OVERHEAT)
		if (!sec_bat_check_wireless_overheat(battery))
			sec_bat_set_misc_event(battery,	0, BATT_MISC_EVENT_WIRELESS_OVERHEAT);
#endif

	/* 5. full charging check */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING))
		sec_bat_fullcharged_check(battery);

	/* 5-1. eu eco check */
	if (check_eu_eco_full_status(battery))
		sec_bat_do_fullcharged(battery, true);

	/* 6. additional check */
	if (battery->pdata->monitor_additional_check)
		battery->pdata->monitor_additional_check();

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	if (is_wireless_type(battery->cable_type) && !battery->wc_cv_mode && battery->charging_passed_time > 10)
		sec_bat_wc_cv_mode_check(battery);
#endif

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	sec_bat_check_boottime(battery, battery->pdata->dctp_bootmode_en);
#if defined(CONFIG_STEP_CHARGING)
	if (is_pd_apdo_wire_type(battery->cable_type))
		sec_bat_check_dc_step_charging(battery);
#endif
#endif

#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_LIMITER_S2ASL01) && !defined(CONFIG_SEC_FACTORY)
	sec_bat_limiter_check(battery);
#endif
}

static bool sb_monitor_checks(struct sec_battery_info *battery)
{
	bool update_battery_info = true;

	sb_monitor_preliminary_checks(battery);	/* Preliminary Checks */
	if (sb_monitor_critical_checks(battery)) { /* Do Critical Checks*/
		/* CheckShort Polling & Monitor Count */
		if (battery->pdata->monitor_initial_count || sec_bat_is_short_polling(battery))
			update_battery_info = false; // Don't Update Battery Info
		else if (sec_bat_time_management(battery)) /* 3. time management */
			sb_monitor_nonurgent_checks(battery);
	}
	return update_battery_info;
}

static void sb_monitor_update(struct sec_battery_info *battery, bool update_battery_info)
{
	union power_supply_propval val = {0, };

	// Update Battery Info
	if (update_battery_info) {
		/* clear HEATING_CONTROL*/
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

		/* calculate safety time */
		if (battery->charger_mode == SEC_BAT_CHG_MODE_CHARGING)
			sec_bat_calculate_safety_time(battery);

		/* set charging current */
		sec_bat_set_charging_current(battery);

		if (sec_bat_recheck_input_work(battery))
			sec_bat_run_input_check_work(battery, 0);
	}

	// Update Charger
	psy_do_property(battery->pdata->charger_name, get, POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	if (is_pd_apdo_wire_type(battery->cable_type) && (val.intval == LOW_VBAT_SET))
		sec_vote_refresh(battery->fcc_vote);
	if (is_pd_apdo_wire_type(battery->wire_status))
		sec_bat_check_direct_charger(battery);
#endif

	// Update Fuelgauge
	psy_do_property(battery->pdata->fuelgauge_name, get, POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);
	if (battery->pdata->wireless_charger_name)
		psy_do_property(battery->pdata->wireless_charger_name, get, POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	psy_do_property(battery->pdata->dual_battery_name, get, POWER_SUPPLY_EXT_PROP_MONITOR_WORK, val);
#endif
}

static void sb_monitor_internal(struct sec_battery_info *battery)
{
	bool update_battery_info = true;
	bool bypass_checks = false;

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	sec_bat_check_wc_available(battery);
	if (is_wireless_type(battery->cable_type) && !battery->wc_auth_retried && !sec_bat_get_lpmode())
		sec_bat_check_wc_re_auth(battery);
#endif

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
	if ((battery->cable_type != SEC_BATTERY_CABLE_NONE) && (battery->batt_f_mode == OB_MODE)) {
		battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
		battery->cable_type = SEC_BATTERY_CABLE_NONE;
		bypass_checks = true; // Skip Checks
	}
#endif
	if (!bypass_checks)
		update_battery_info = sb_monitor_checks(battery);

	sb_monitor_update(battery, update_battery_info);
}

static void sb_monitor_print(struct sec_battery_info *battery, const char *funcname)
{
	char str[512] = {0, };
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	int dc_op_mode = get_sec_vote_result(battery->dc_op_mode_vote);
	int apdo_max_volt = get_sec_vote_result(battery->apdo_max_volt_vote);
#endif

	sprintf(str, "%s: Status(%s), mode(%s), Health(%s), Cable(%s, %s, %d, %d), rp(%d), HV(%s), flash(%d), mst(%d)",
		funcname,
		sb_get_bst_str(battery->status),
		sb_get_cm_str(battery->charging_mode),
		sb_get_hl_str(battery->health),
		sb_get_ct_str(battery->cable_type),
		sb_get_ct_str(battery->wire_status),
		battery->muic_cable_type,
		battery->pd_usb_attached,
		battery->sink_status.rp_currentlvl,
		battery->hv_chg_name,
		battery->flash_state,
		battery->mst_en
	);

#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
	sprintf(str + strlen(str), ", phm(%d)",
		battery->wc_rx_pdetb_mode
	);
#endif

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	if (is_dc_higher_ratio_support()) {
		sprintf(str + strlen(str), ", dc_op(%d:1), apdo_max_volt(%d)",
			(dc_op_mode < 0) ? 0 : dc_op_mode,
			(apdo_max_volt < 0) ? 0 : apdo_max_volt
		);
	}
#endif

	sprintf(str + strlen(str), ", lcd(%d), slate(%d), store(%d), siop_level(%d), sleep_mode(%d)",
		battery->lcd_status,
		is_slate_mode(battery),
		battery->store_mode,
		battery->siop_level,
		battery->sleep_mode
	);

#if defined(CONFIG_BATTERY_AGE_FORECAST_DETACHABLE)
	sprintf(str + strlen(str), ", Cycle(%dw)",
		battery->batt_cycle
	);
#else
	sprintf(str + strlen(str), ", Cycle(%d, %d)",
		battery->batt_cycle, battery->batt_full_status_usage
	);
#endif

#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	sprintf(str + strlen(str), ", battery->stability_test(%d), battery->eng_not_full_status(%d)",
		battery->stability_test,
		battery->eng_not_full_status);
#endif

	sprintf(str + strlen(str), "\n");
	pr_info("%s", str);

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->wc_tx_enable) {
		pr_info("@Tx_Mode %s: Rx(%s), WC_TX_VOUT(%dmV), UNO_IOUT(%d), MFC_IOUT(%d) AFC_DISABLE(%d)\n",
			funcname, sb_rx_type_str(battery->wc_rx_type), battery->wc_tx_vout,
			battery->tx_uno_iout, battery->tx_mfc_iout, battery->afc_disable);
	}
#endif
}

static void sec_bat_monitor_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work, struct sec_battery_info, monitor_work.work);

	dev_dbg(battery->dev, "%s: Start\n", __func__);

	if (!sb_check_skip_monitor(battery)) {
		sb_monitor_internal(battery); // Internal Function Which Performs the Checks
		sb_monitor_print(battery, __func__); // Print Monitor

		/* store mode & fac bin */
		sec_bat_check_store_mode(battery);
		power_supply_changed(battery->psy_bat);
	}

	sec_bat_set_polling(battery);

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->tx_switch_mode_change)
		sec_bat_run_wpc_tx_work(battery, 0);
#endif

	if (battery->capacity <= 0 || battery->health_change)
		__pm_wakeup_event(battery->monitor_ws, jiffies_to_msecs(HZ * 5));
	else
		__pm_relax(battery->monitor_ws);

	dev_dbg(battery->dev, "%s: End\n", __func__);
}

static enum alarmtimer_restart sec_bat_alarm(struct alarm *alarm, ktime_t now)
{
	struct sec_battery_info *battery = container_of(alarm,
				struct sec_battery_info, polling_alarm);

	dev_dbg(battery->dev, "%s\n", __func__);

	/*
	 * In wake up, monitor work will be queued in complete function
	 * To avoid duplicated queuing of monitor work,
	 * do NOT queue monitor work in wake up by polling alarm
	 */
	if (!battery->polling_in_sleep) {
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_dbg(battery->dev, "%s: Activated\n", __func__);
	}

	return ALARMTIMER_NORESTART;
}

__visible_for_testing void sec_bat_check_input_voltage(struct sec_battery_info *battery, int cable_type)
{
	unsigned int voltage = 0;
	int input_current = battery->pdata->charging_current[cable_type].input_current_limit;

#if !defined(CONFIG_NO_BATTERY)
	if (battery->status == POWER_SUPPLY_STATUS_DISCHARGING)
		return;
#endif

	if (is_pd_wire_type(cable_type)) {
		battery->max_charge_power = battery->pd_max_charge_power;
		return;
	} else if (is_hv_wire_12v_type(cable_type))
		voltage = SEC_INPUT_VOLTAGE_12V;
	else if (is_hv_wire_9v_type(cable_type))
		voltage = SEC_INPUT_VOLTAGE_9V;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	else if (cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 || is_pwr_nego_wireless_type(cable_type))
		voltage = battery->wc20_vout;
	else if (is_hv_wireless_type(cable_type) || cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)
		voltage = SEC_INPUT_VOLTAGE_10V;
	else if (is_nv_wireless_type(cable_type))
		voltage = SEC_INPUT_VOLTAGE_5_5V;
#endif
	else
		voltage = SEC_INPUT_VOLTAGE_5V;

	battery->input_voltage = voltage;
	battery->charge_power = mW_by_mVmA(voltage, input_current);
#if !defined(CONFIG_SEC_FACTORY)
	if (battery->charge_power > battery->max_charge_power)
#endif
		battery->max_charge_power = battery->charge_power;

	pr_info("%s: input_voltage:%dmV, charge_power:%dmW, max_charge_power:%dmW)\n", __func__,
		battery->input_voltage, battery->charge_power, battery->max_charge_power);
}
EXPORT_SYMBOL_KUNIT(sec_bat_check_input_voltage);

static void sec_bat_set_usb_configure(struct sec_battery_info *battery, int usb_status)
{
	int cable_work_delay = 0;

	pr_info("%s: usb configured %d -> %d\n", __func__, battery->prev_usb_conf, usb_status);

	if (usb_status == USB_CURRENT_UNCONFIGURED) {
		sec_bat_set_current_event(battery,
				SEC_BAT_CURRENT_EVENT_USB_100MA, SEC_BAT_CURRENT_EVENT_USB_STATE);
		if (battery->cable_type == SEC_BATTERY_CABLE_USB && !sec_bat_get_lpmode()) {
			sec_vote(battery->fcc_vote, VOTER_USB_100MA, true, 100);
			sec_vote(battery->input_vote, VOTER_USB_100MA, true, 100);
		}
	} else if (usb_status == USB_CURRENT_HIGH_SPEED || usb_status == USB_CURRENT_SUPER_SPEED) {
		battery->usb_slow_chg = false;
		sec_vote(battery->fcc_vote, VOTER_USB_100MA, false, 0);
		sec_vote(battery->input_vote, VOTER_USB_100MA, false, 0);
		if (usb_status == USB_CURRENT_HIGH_SPEED) {
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_USB_STATE);
			if (battery->cable_type == SEC_BATTERY_CABLE_USB) {
				sec_vote(battery->fcc_vote, VOTER_CABLE, true,
						battery->pdata->default_usb_charging_current);
				sec_vote(battery->input_vote, VOTER_CABLE, true,
						battery->pdata->default_usb_input_current);
			}
		} else {
			sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_USB_SUPER, SEC_BAT_CURRENT_EVENT_USB_STATE);
			if (battery->cable_type == SEC_BATTERY_CABLE_USB) {
				sec_vote(battery->fcc_vote, VOTER_CABLE, true, USB_CURRENT_SUPER_SPEED);
				sec_vote(battery->input_vote, VOTER_CABLE, true, USB_CURRENT_SUPER_SPEED);
			}
		}
		if (battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL3) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE) {
				sec_vote(battery->fcc_vote, VOTER_CABLE, true,
						battery->pdata->default_charging_current);
				sec_vote(battery->input_vote, VOTER_CABLE, true,
						battery->pdata->default_input_current);
			} else {
				if (battery->store_mode) {
					sec_vote(battery->fcc_vote, VOTER_CABLE, true,
							battery->pdata->max_charging_current);
					sec_vote(battery->input_vote, VOTER_CABLE, true,
							battery->pdata->rp_current_rdu_rp3);
				} else {
					if (!(is_pd_wire_type(battery->wire_status) ||
						is_pd_wire_type(battery->cable_type))) {
						sec_vote(battery->fcc_vote, VOTER_CABLE, true,
							battery->pdata->max_charging_current);
						sec_vote(battery->input_vote, VOTER_CABLE, true,
							battery->pdata->rp_current_rp3);
					}
				}
			}
		} else if (battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL2) {
			if (!(is_pd_wire_type(battery->wire_status) ||
				is_pd_wire_type(battery->cable_type))) {
				sec_vote(battery->fcc_vote, VOTER_CABLE, true, battery->pdata->rp_current_rp2);
				sec_vote(battery->input_vote, VOTER_CABLE, true, battery->pdata->rp_current_rp2);
			}
		}
	} else if (usb_status == USB_CURRENT_SUSPENDED) {
		battery->usb_slow_chg = false;
		sec_bat_set_current_event(battery,
				SEC_BAT_CURRENT_EVENT_USB_SUSPENDED, SEC_BAT_CURRENT_EVENT_USB_STATE);
		sec_vote(battery->chgen_vote, VOTER_SUSPEND, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		if (battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT) {
			sec_vote(battery->fcc_vote, VOTER_USB_100MA, true, 100);
			sec_vote(battery->input_vote, VOTER_USB_100MA, true, 100);
		}
		cable_work_delay = 500;
		store_battery_log(
				"USB suspend:%d%%,%dmV,%s,ct(%d,%d,%d),cev(0x%x),mev(0x%x)",
				battery->capacity,
				battery->voltage_now,
				sb_get_bst_str(battery->status),
				battery->cable_type,
				battery->wire_status,
				battery->muic_cable_type,
				battery->current_event,
				battery->misc_event
				);
	} else if (usb_status == USB_CURRENT_CLEAR) {
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_USB_STATE);
		sec_vote(battery->chgen_vote, VOTER_SUSPEND, false, 0);
		sec_vote(battery->fcc_vote, VOTER_USB_100MA, false, 0);
		sec_vote(battery->input_vote, VOTER_USB_100MA, false, 0);
	}

	if (usb_status != USB_CURRENT_SUSPENDED)
		sec_vote(battery->chgen_vote, VOTER_SUSPEND, false, 0);

	battery->prev_usb_conf = usb_status;

	cancel_delayed_work(&battery->cable_work);
	__pm_stay_awake(battery->cable_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->cable_work,
			msecs_to_jiffies(cable_work_delay));
}

#define REDUCE_STEP 500
#define MIN_FCC_VALUE 3500
static void sec_bat_set_abnormal_ta_fcc(struct sec_battery_info *battery, bool enable)
{
	union power_supply_propval value = {0, };
	int fcc;

	if (!enable) {
		battery->abnormal_ta = false;
		pr_info("%s: enable(%d)", __func__, enable);
		sec_vote(battery->fcc_vote, VOTER_ABNORMAL_TA, false, 0);
		return;
	}

	fcc = get_sec_vote_result(battery->fcc_vote) - REDUCE_STEP;
	if (fcc > MIN_FCC_VALUE) {
		pr_info("%s: reduce chg current(%d)", __func__, fcc);
		sec_vote(battery->fcc_vote, VOTER_ABNORMAL_TA, true, fcc);
	} else {
		battery->abnormal_ta = true;
		pr_info("%s: update charging source(%d)", __func__, enable);
		sec_vote(battery->fcc_vote, VOTER_ABNORMAL_TA, true, MIN_FCC_VALUE);
		if (is_pd_apdo_wire_type(battery->cable_type))
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);
	}
}

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
__visible_for_testing void sec_bat_ext_event_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
			struct sec_battery_info, ext_event_work.work);

	sec_bat_ext_event_work_content(battery);
}
EXPORT_SYMBOL_KUNIT(sec_bat_ext_event_work);

static void sec_bat_wpc_tx_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wpc_tx_work.work);

	sec_bat_wpc_tx_work_content(battery);
}

__visible_for_testing void sec_bat_wpc_tx_en_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wpc_tx_en_work.work);

	sec_bat_wpc_tx_en_work_content(battery);
}
EXPORT_SYMBOL_KUNIT(sec_bat_wpc_tx_en_work);

__visible_for_testing void sec_bat_txpower_calc_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wpc_txpower_calc_work.work);

	sec_bat_txpower_calc(battery);
}
EXPORT_SYMBOL_KUNIT(sec_bat_txpower_calc_work);

static void sec_bat_wc20_current_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, wc20_current_work.work);

	sec_bat_set_wc20_current(battery);
	__pm_relax(battery->wc20_current_ws);
}
#endif

static void cw_check_pd_wire_type(struct sec_battery_info *battery)
{
	if (is_pd_wire_type(battery->wire_status)) {
		int pdo_num = battery->sink_status.current_pdo_num;

		sec_bat_get_input_current_in_power_list(battery);
		sec_bat_get_charging_current_in_power_list(battery);

#if defined(CONFIG_STEP_CHARGING)
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		if (!is_pd_apdo_wire_type(battery->cable_type)) {
			sec_bat_reset_step_charging(battery);
		} else if (is_pd_apdo_wire_type(battery->cable_type) && (battery->ta_alert_mode != OCP_NONE)) {
			battery->ta_alert_mode = OCP_WA_ACTIVE;
			sec_bat_reset_step_charging(battery);
		}
#else
		sec_bat_reset_step_charging(battery);
#endif
#endif
		if (!battery->sink_status.power_list[pdo_num].comm_capable
			|| !battery->sink_status.power_list[pdo_num].suspend) {
			pr_info("%s : clear suspend event pdo_num:%d, comm:%d, suspend:%d\n", __func__,
				pdo_num,
				battery->sink_status.power_list[pdo_num].comm_capable,
				battery->sink_status.power_list[pdo_num].suspend);
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_USB_SUSPENDED);
			sec_vote(battery->chgen_vote, VOTER_SUSPEND, false, 0);
			sec_vote(battery->fcc_vote, VOTER_USB_100MA, false, 0);
			sec_vote(battery->input_vote, VOTER_USB_100MA, false, 0);
		}
	}

}

static bool cw_check_skip_condition(struct sec_battery_info *battery, int current_cable_type)
{
	static bool first_run = true;

	if (first_run) {
		first_run = false;

		if (current_cable_type == SEC_BATTERY_CABLE_NONE) {
			dev_info(battery->dev, "%s: do not skip! first cable work\n", __func__);

			return false;
		}
	}

	if ((current_cable_type == battery->cable_type) && !is_slate_mode(battery)
			&& !(battery->current_event & SEC_BAT_CURRENT_EVENT_USB_SUSPENDED)) {
		if (is_pd_wire_type(current_cable_type) && is_pd_wire_type(battery->cable_type)) {
			sec_bat_set_current_event(battery, 0,
				SEC_BAT_CURRENT_EVENT_AFC | SEC_BAT_CURRENT_EVENT_AICL);
			sec_vote(battery->input_vote, VOTER_AICL, false, 0);
			sec_vote(battery->input_vote, VOTER_VBUS_CHANGE, false, 0);
			power_supply_changed(battery->psy_bat);
		} else if (battery->prev_usb_conf != USB_CURRENT_NONE) {
			dev_info(battery->dev, "%s: set usb charging current to %d mA\n",
				__func__, battery->prev_usb_conf);
			sec_bat_set_charging_current(battery);
			battery->prev_usb_conf = USB_CURRENT_NONE;
		}

		dev_info(battery->dev, "%s: Cable is NOT Changed(%d)\n", __func__, battery->cable_type);
		/* Do NOT activate cable work for NOT changed */

		return true;
	}

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	if (is_wireless_fake_type(current_cable_type))
		if (sec_bat_check_wireless_overheat(battery))
			return true;
#endif

	return false;
}

__visible_for_testing int cw_check_cable_switch(struct sec_battery_info *battery, int prev_ct,
		int cur_ct, bool afc_disabled)
{
	if ((is_wired_type(prev_ct) && is_wireless_all_type(cur_ct))
		|| (is_wireless_all_type(prev_ct) && is_wired_type(cur_ct))
		|| afc_disabled) {
		battery->max_charge_power = 0;
		sec_bat_set_threshold(battery, cur_ct);
	}

	if (cur_ct == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)
		cur_ct = SEC_BATTERY_CABLE_9V_TA;

	if (!can_usb_suspend_type(cur_ct) &&
			battery->current_event & SEC_BAT_CURRENT_EVENT_USB_SUSPENDED) {
		pr_info("%s: clear suspend event prev_cable_type:%s -> %s\n", __func__,
				sb_get_ct_str(prev_ct), sb_get_ct_str(cur_ct));
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_USB_SUSPENDED);
		sec_vote(battery->chgen_vote, VOTER_SUSPEND, false, 0);
		sec_vote(battery->fcc_vote, VOTER_USB_100MA, false, 0);
		sec_vote(battery->input_vote, VOTER_USB_100MA, false, 0);
	}

	return cur_ct;
}

#if defined(CONFIG_USE_POGO)
static int cw_check_pogo(struct sec_battery_info *battery)
{
	int pogo_current, wire_current;

	if (battery->pogo_status) {
		if (battery->wire_status != SEC_BATTERY_CABLE_NONE) {
			if (battery->pogo_9v) {
				pogo_current = battery->pdata->charging_current[SEC_BATTERY_CABLE_POGO_9V].input_current_limit;
				pogo_current = pogo_current * SEC_INPUT_VOLTAGE_9V;
			} else {
				pogo_current = battery->pdata->charging_current[SEC_BATTERY_CABLE_POGO].input_current_limit;
				pogo_current = pogo_current * SEC_INPUT_VOLTAGE_5V;
			}
			if (battery->wire_status == SEC_BATTERY_CABLE_PDIC) {
				if (pogo_current < battery->pd_max_charge_power)
					return battery->wire_status;
			} else {
				wire_current = (battery->wire_status == SEC_BATTERY_CABLE_PREPARE_TA ?
						battery->pdata->charging_current[SEC_BATTERY_CABLE_TA].input_current_limit :
						battery->pdata->charging_current[battery->wire_status].input_current_limit);

				wire_current = wire_current * (is_hv_wire_type(battery->wire_status) ?
						(battery->wire_status == SEC_BATTERY_CABLE_12V_TA ? SEC_INPUT_VOLTAGE_12V : SEC_INPUT_VOLTAGE_9V)
						: SEC_INPUT_VOLTAGE_5V);
				pr_info("%s: pogo_cur(%d), wr_cur(%d), wire_cable_type(%d)\n",
						__func__, pogo_current, wire_current, battery->wire_status);

				if (pogo_current < wire_current)
					return battery->wire_status;
				else
					return (battery->pogo_9v) ? SEC_BATTERY_CABLE_POGO_9V : SEC_BATTERY_CABLE_POGO;
			}
		}
		return (battery->pogo_9v) ? SEC_BATTERY_CABLE_POGO_9V : SEC_BATTERY_CABLE_POGO;
	}
	return battery->wire_status;
}
#endif

static void cw_set_psp_online2drivers(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };

	if (is_slate_mode(battery))
		val.intval = SEC_BATTERY_CABLE_NONE;
	else
		val.intval = battery->cable_type;
	psy_do_property(battery->pdata->charger_name, set, POWER_SUPPLY_PROP_ONLINE, val);

	val.intval = battery->cable_type;
	psy_do_property(battery->pdata->fuelgauge_name, set, POWER_SUPPLY_PROP_ONLINE, val);

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	val.intval = battery->cable_type;
	psy_do_property(battery->pdata->dual_battery_name, set, POWER_SUPPLY_PROP_ONLINE, val);
#endif

	if (battery->wc_tx_enable) {
		val.intval = is_wired_type(battery->wire_status) ? 1 : 0;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_WR_CONNECTED, val);
	}

	if (!is_wireless_fake_type(battery->cable_type))
		sec_vote_refresh(battery->chgen_vote);
}

static void cw_check_wc_condition(struct sec_battery_info *battery, int prev_cable_type)
{
	union power_supply_propval value = {0, };

	/* need to move to wireless set property */
	if (is_wireless_all_type(battery->cable_type)) {
		power_supply_changed(battery->psy_bat);
	}

	/* For wire + wireless case */
	if (!is_wireless_type(prev_cable_type) && is_wireless_type(battery->cable_type)) {
		pr_info("%s: non-wl -> wl: prev_cable(%s) , current_cable(%s)\n",
			__func__, sb_get_ct_str(prev_cable_type), sb_get_ct_str(battery->cable_type));

		if (is_all_hv_wire_type(prev_cable_type) &&
			battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_MPP) {
			pr_info("@mpp: %s: send ept reset\n", __func__);
			value.intval = CEP_TIMEOUT;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_WC_EPT_UNKNOWN, value);
		}
		battery->wc_cv_mode = false;
		battery->charging_passed_time = 0;
	}
}

static void chg_retention_time_check(struct sec_battery_info *battery)
{
	struct timespec64 ts = {0, };

	ts = ktime_to_timespec64(ktime_get_boottime());
	pr_info("%s: end time = %lld\n", __func__, ts.tv_sec);

	if (ts.tv_sec <= battery->charging_retention_time || !battery->charging_retention_time)
		return;

	battery->charging_retention_time = ts.tv_sec - battery->charging_retention_time;
	/* get only minutes part */
	battery->charging_retention_time /= 60;
	pr_info("%s: retention time = %ld mins\n", __func__, battery->charging_retention_time);
	battery->cisd.data[CISD_DATA_TOTAL_CHG_RETENTION_TIME_PER_DAY] += battery->charging_retention_time;
	battery->cisd.data[CISD_DATA_CHG_RETENTION_TIME_PER_DAY] =
		max(battery->cisd.data[CISD_DATA_CHG_RETENTION_TIME_PER_DAY], (int)battery->charging_retention_time);
	battery->charging_retention_time = 0;
}

static void chg_start_time_check(struct sec_battery_info *battery)
{
	struct timespec64 ts = {0, };

	ts = ktime_to_timespec64(ktime_get_boottime());
	battery->charging_retention_time = ts.tv_sec;
	pr_info("%s: start time = %lu\n", __func__, battery->charging_retention_time);
}

void sec_bat_smart_sw_src(struct sec_battery_info *battery, bool enable, int curr)
{
	if (enable)
		battery->smart_sw_src = true;
	else if (battery->smart_sw_src)
		battery->smart_sw_src = false;
	else
		return;

	sec_pd_change_src(curr);
	dev_info(battery->dev,
		"%s: smart switch src, %s src cap max current\n",
		__func__, enable ? "reduce" : "recover");
}

static void cw_nocharge_type(struct sec_battery_info *battery)
{
	int i;

	/* initialize all status */
	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	battery->is_recharging = false;
	battery->cisd.ab_vbat_check_count = 0;
	battery->cisd.state &= ~CISD_STATE_OVER_VOLTAGE;
	battery->d2d_check = false;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	battery->wc20_power_class = 0;
	battery->wc20_rx_power = 0;
	battery->wc20_vout = 0;
#endif
	battery->input_voltage = 0;
	battery->charge_power = 0;
	battery->max_charge_power = 0;
	battery->pd_max_charge_power = 0;
	battery->pd_rated_power = 0;
	sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
	battery->thermal_zone = BAT_THERMAL_NORMAL;
	battery->chg_limit = false;
	battery->lrp_limit = false;
	battery->lrp_step = LRP_NONE;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	battery->lrp_chg_src = SEC_CHARGING_SOURCE_DIRECT;
#endif
	battery->mix_limit = false;
	battery->chg_limit_recovery_cable = SEC_BATTERY_CABLE_NONE;
	battery->wc_heating_start_time = 0;
	sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);
	battery->prev_usb_conf = USB_CURRENT_NONE;
	battery->ta_alert_mode = OCP_NONE;
	battery->prev_tx_phm_mode = false;
	battery->wpc_temp_v2_offset = 0;
	battery->is_fpdo_dc = false;
	battery->dc_ta_op_max_mode = DC_MODE_NONE;
	battery->dc_ta_forced_op_max_mode = DC_MODE_NONE;
	battery->pdata->mpla_thr_cnt = 0;

	sec_bat_cancel_input_check_work(battery);
	sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_USB,
			battery->pdata->default_usb_input_current,
			battery->pdata->default_usb_charging_current);
	sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_TA,
			battery->pdata->default_input_current,
			battery->pdata->default_charging_current);
	sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_HV_WIRELESS_20,
			battery->pdata->default_wc20_input_current,
			battery->pdata->default_wc20_charging_current);
	sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_WIRELESS_EPP,
			battery->pdata->default_wc20_input_current,
			battery->pdata->default_wc20_charging_current);
	sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_WIRELESS_MPP,
			battery->pdata->default_mpp_input_current,
			battery->pdata->default_mpp_charging_current);
	/* usb default current is 100mA before configured*/
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA,
			(SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE |
			SEC_BAT_CURRENT_EVENT_AFC |
			SEC_BAT_CURRENT_EVENT_VBAT_OVP |
			SEC_BAT_CURRENT_EVENT_VSYS_OVP |
			SEC_BAT_CURRENT_EVENT_CHG_LIMIT |
			SEC_BAT_CURRENT_EVENT_AICL |
			SEC_BAT_CURRENT_EVENT_SELECT_PDO |
			SEC_BAT_CURRENT_EVENT_WDT_EXPIRED |
			SEC_BAT_CURRENT_EVENT_25W_OCP |
			SEC_BAT_CURRENT_EVENT_DC_ERR |
			SEC_BAT_CURRENT_EVENT_USB_STATE |
			SEC_BAT_CURRENT_EVENT_SEND_UVDM));
	sec_bat_set_misc_event(battery, 0,
		(BATT_MISC_EVENT_WIRELESS_MISALIGN | BATT_MISC_EVENT_HV_BY_AICL));
	sec_bat_recov_full_capacity(battery); /* should call this after setting discharging */

	/* slate_mode needs to be clear manually since smart switch does not disable slate_mode sometimes */
	if (is_slate_mode(battery)) {
		int voter_status = SEC_BAT_CHG_MODE_CHARGING;
		if (get_sec_voter_status(battery->chgen_vote, VOTER_SMART_SLATE, &voter_status) < 0)
			pr_err("%s: INVALID VOTER ID\n", __func__);
		pr_info("%s: voter_status: %d\n", __func__, voter_status); // debug
		if (voter_status == SEC_BAT_CHG_MODE_BUCK_OFF) {
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SLATE);
			sec_bat_set_mfc_on(battery, WPC_EN_SLATE);
			dev_info(battery->dev,
					"%s: disable slate mode(smart switch) manually\n", __func__);
		}
	}
	sec_bat_smart_sw_src(battery, false, 500);

	chg_retention_time_check(battery);
	battery->wc_cv_mode = false;
	battery->is_sysovlo = false;
	battery->is_vbatovlo = false;
	battery->is_abnormal_temp = false;
	battery->auto_mode = false;
#if !defined(CONFIG_SEC_FACTORY)
	if (sec_bat_get_lpmode())
		battery->usb_conn_status = USB_CONN_NORMAL;
#endif
	for (i = 0; i < VOTER_MAX; i++) {
		if (i != VOTER_FLASH &&
			i != VOTER_MST &&
			i != VOTER_FW)
			sec_vote(battery->iv_vote, i, false, 0);
		if (i == VOTER_SIOP ||
			i == VOTER_SLATE ||
			i == VOTER_AGING_STEP ||
			i == VOTER_WC_TX ||
			i == VOTER_MUIC_ABNORMAL ||
			i == VOTER_NO_BATTERY ||
			i == VOTER_FULL_CAPACITY ||
			i == VOTER_MULTI_BATT_CHG)
			continue;
		sec_vote(battery->topoff_vote, i, false, 0);
		if (i != VOTER_FW)
			sec_vote(battery->chgen_vote, i, false, 0);
		sec_vote(battery->input_vote, i, false, 0);
		sec_vote(battery->fcc_vote, i, false, 0);
		sec_vote(battery->fv_vote, i, false, 0);
		sec_vote(battery->dc_fv_vote, i, false, 0);
		sec_vote(battery->dc_op_mode_vote, i, false, 0);
		sec_vote(battery->apdo_max_volt_vote, i, false, 0);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		sec_vote(battery->vlim_vote, i, false, 0);
#endif
	}
	cancel_delayed_work(&battery->slowcharging_work);
#if !defined(CONFIG_NO_BATTERY)
	/* Discharging has 100mA current unlike non LEGO model */
	sec_vote(battery->fcc_vote, VOTER_USB_100MA, true, 100);
	sec_vote(battery->input_vote, VOTER_USB_100MA, true, 100);
#endif
	battery->usb_slow_chg = false;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	battery->limiter_check = false;
#endif
	sec_bat_set_abnormal_ta_fcc(battery, false);
}

static void cw_slate_mode(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	union power_supply_propval val = {0, };
#endif
	int j = 0;

	/* Some charger ic's buck is enabled after vbus off, So disable buck again*/
	sec_vote_refresh(battery->chgen_vote);
	battery->is_recharging = false;
	battery->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);
	battery->is_sysovlo = false;
	battery->is_vbatovlo = false;
	battery->is_abnormal_temp = false;
	battery->lrp_limit = false;
	battery->lrp_step = LRP_NONE;
	battery->pdata->mpla_thr_cnt = 0;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	battery->lrp_chg_src = SEC_CHARGING_SOURCE_DIRECT;
#endif

	for (j = 0; j < VOTER_MAX; j++) {
		sec_vote(battery->iv_vote, j, false, 0);
		if (j == VOTER_SIOP ||
			j == VOTER_SLATE ||
			j == VOTER_SMART_SLATE ||
			j == VOTER_AGING_STEP ||
			j == VOTER_WC_TX ||
			j == VOTER_FULL_CAPACITY ||
			j == VOTER_MULTI_BATT_CHG)
			continue;
		sec_vote(battery->topoff_vote, j, false, 0);
		sec_vote(battery->chgen_vote, j, false, 0);
		sec_vote(battery->input_vote, j, false, 0);
		sec_vote(battery->fcc_vote, j, false, 0);
		sec_vote(battery->fv_vote, j, false, 0);
	}

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	/* No need val data */
	psy_do_property(battery->pdata->charger_name, set, POWER_SUPPLY_EXT_PROP_DC_INITIALIZE, val);
#endif
	battery->thermal_zone = BAT_THERMAL_NORMAL;
	sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
}

static void cw_usb_suspend(struct sec_battery_info *battery)
{
	/* Some charger ic's buck is enabled after vbus off, So disable buck again*/
	sec_vote_refresh(battery->chgen_vote);
	battery->is_recharging = false;
	battery->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);
	battery->is_sysovlo = false;
	battery->is_vbatovlo = false;
	battery->is_abnormal_temp = false;
	battery->thermal_zone = BAT_THERMAL_NORMAL;
	if (!(battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY))
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
}

static bool is_afc_evt_clear(int cable_type, int wire_status, int wc_status, unsigned int rp_currentlvl)
{
	if ((cable_type == SEC_BATTERY_CABLE_TA && (rp_currentlvl == RP_CURRENT_LEVEL_DEFAULT ||
			rp_currentlvl == RP_CURRENT_LEVEL_NONE)) ||
			cable_type == SEC_BATTERY_CABLE_WIRELESS ||
			cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS ||
			(is_hv_wire_type(cable_type) &&
			(wc_status == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20 ||
				is_pwr_nego_wireless_type(wc_status) ||
				wire_status == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT)))
		return false;
	else
		return true;
}

static void cw_prev_cable_is_nocharge(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_MTK_CHARGER)
	union power_supply_propval value = {0, };
#endif

	chg_start_time_check(battery);

	if ((battery->cable_type == SEC_BATTERY_CABLE_WIRELESS ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE) &&
		(battery->health > POWER_SUPPLY_HEALTH_GOOD)) {
		pr_info("%s: prev cable type was WC_FAKE and health is not good\n", __func__);
		return;
	}
#if defined(CONFIG_ARCH_MTK_PROJECT) || IS_ENABLED(CONFIG_SEC_MTK_CHARGER)
	if (battery->current_event & SEC_BAT_CURRENT_EVENT_USB_100MA) {
		if ((battery->cable_type == SEC_BATTERY_CABLE_USB) && !lpcharge) {
			pr_info("%s: usb unconfigured\n", __func__);
			sec_vote(battery->fcc_vote, VOTER_USB_100MA, true, 100);
			sec_vote(battery->input_vote, VOTER_USB_100MA, true, 100);
		}
	}
#endif
	if (battery->pdata->full_check_type != SEC_BATTERY_FULLCHARGED_NONE)
		battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
	else
		battery->charging_mode = SEC_BATTERY_CHARGING_2ND;

#if defined(CONFIG_ENABLE_FULL_BY_SOC)
	if (battery->capacity >= 100) {
		sec_bat_do_fullcharged(battery, true);
		pr_info("%s: charging start at full, do not turn on charging\n", __func__);
#if IS_ENABLED(CONFIG_MTK_CHARGER)
		/*
		 * MTK check the AFC when charging-on.
		 *  Even though do not charging-on, need to check AFC still.
		 */
		value.intval = battery->cable_type;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_AFC_INIT, value);
#endif
	} else if (!(battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY)) {
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
	}
#else
	if (!(battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY)) {
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
	}
#endif
	sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);

	/* bat thm check to prevent charging current spike */
	sec_bat_thermal_check(battery);
	/* lrp temp check to prevent another current spike and set correct ratio */
#if !defined(CONFIG_SEC_FACTORY)
	if (!is_wireless_all_type(battery->cable_type))
		sec_bat_check_lrp_temp(battery,
			battery->cable_type, battery->wire_status, battery->siop_level, battery->lcd_status);
#endif
	/* disable charging immediately if battery protection */
	sec_bat_check_full_capacity(battery);
	sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_CHARGING);

	if (battery->cable_type == SEC_BATTERY_CABLE_USB && !sec_bat_get_lpmode())
		queue_delayed_work(battery->monitor_wqueue, &battery->slowcharging_work, msecs_to_jiffies(3000));
	if (is_hv_wireless_type(battery->cable_type) && battery->sleep_mode)
		sec_vote(battery->input_vote, VOTER_SLEEP_MODE, true, battery->pdata->sleep_mode_limit_current);

	ttf_work_start(battery);
}

static int sec_bat_check_pd_iv(SEC_PD_SINK_STATUS *sink_status)
{
	int i;
	int max_voltage = SEC_INPUT_VOLTAGE_5V;

	for (i = 1; i <= sink_status->available_pdo_num; i++) {
		if ((sink_status->power_list[i].pdo_type == APDO_TYPE)
			|| !sink_status->power_list[i].accept)
			break;

		if (sink_status->power_list[i].max_voltage == SEC_INPUT_VOLTAGE_9V) {
			max_voltage = SEC_INPUT_VOLTAGE_9V;
			break;
		}
	}

	return max_voltage;
}

#if defined(CONFIG_SUPPORT_HV_CTRL) && !defined(CONFIG_SEC_FACTORY)
static bool sb_ct_has_9v(struct sec_battery_info *battery, int ct)
{
	if (is_hv_wire_type(ct))
		return true;
	else if (is_pd_wire_type(ct)) {
		if (sec_bat_check_pd_iv(&battery->sink_status) == SEC_INPUT_VOLTAGE_9V)
			return true;
	}
	return false;
}
#endif

static void cw_set_iv(struct sec_battery_info *battery)
{
	if (is_nocharge_type(battery->cable_type) || is_wireless_all_type(battery->cable_type))
		return;

	if (is_hv_wire_type(battery->cable_type))
		sec_vote(battery->iv_vote, VOTER_CABLE, true, SEC_INPUT_VOLTAGE_9V);
	else if (is_pd_wire_type(battery->cable_type))
		sec_vote(battery->iv_vote, VOTER_CABLE, true, sec_bat_check_pd_iv(&battery->sink_status));
#if defined(CONFIG_USE_POGO)
	else if (battery->cable_type == SEC_BATTERY_CABLE_POGO_9V)
		sec_vote(battery->iv_vote, VOTER_CABLE, true, SEC_INPUT_VOLTAGE_9V);
#endif
	else
		sec_vote(battery->iv_vote, VOTER_CABLE, true, SEC_INPUT_VOLTAGE_5V);

}

__visible_for_testing void sec_bat_cable_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, cable_work.work);
	int current_cable_type = SEC_BATTERY_CABLE_NONE;
	unsigned int input_current;
	unsigned int charging_current;
	bool clear_afc_evt = false;
	int prev_cable_type = battery->cable_type;
	int monitor_work_delay = 0;

	dev_info(battery->dev, "%s: Start\n", __func__);
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL,
			SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL);

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if !IS_ENABLED(CONFIG_MTK_CHARGER) || IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	/*
	 * showing charging icon and noti(no sound, vi, haptic) only
	 * if slow insertion is detected by MUIC
	 */
	if (!(battery->pogo_status && battery->pdata->pogo_chgin))
		sec_bat_set_misc_event(battery,
			(battery->muic_cable_type == ATTACHED_DEV_TIMEOUT_OPEN_MUIC ? BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE : 0),
			BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);
#endif
#endif

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	current_cable_type = sec_bat_choose_cable_type(battery);
#elif defined(CONFIG_USE_POGO)
	current_cable_type = cw_check_pogo(battery);
#else
	current_cable_type = battery->wire_status;
#endif

	/* check wire type PD charger case */
	cw_check_pd_wire_type(battery);

	/* check cable work skip condition */
	if (cw_check_skip_condition(battery, current_cable_type))
		goto end_of_cable_work;

	/* to clear this value when cable type switched without detach */
	prev_cable_type = battery->cable_type;
	battery->cable_type = cw_check_cable_switch(battery, prev_cable_type,
			current_cable_type, check_afc_disabled_type(battery->muic_cable_type));

	/* set online(cable type) */
	cw_set_psp_online2drivers(battery);

	/* check wireless charging condition */
	cw_check_wc_condition(battery, prev_cable_type);

	if (battery->pdata->check_cable_result_callback)
		battery->pdata->check_cable_result_callback(battery->cable_type);

	if (is_nocharge_type(battery->cable_type) ||
		((battery->pdata->cable_check_type & SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE) &&
		battery->cable_type == SEC_BATTERY_CABLE_UNKNOWN)) {
		pr_info("%s: prev_cable_type(%d)\n", __func__, prev_cable_type);
		cw_nocharge_type(battery);
	} else if (is_slate_mode(battery)) {
		pr_info("%s: slate mode on\n", __func__);
		cw_slate_mode(battery);
	} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_USB_SUSPENDED) {
		pr_info("%s: usb suspend\n", __func__);
		cw_usb_suspend(battery);
		battery->prev_usb_conf = USB_CURRENT_NONE;
		monitor_work_delay = 3000;
		goto run_monitor_work;
	} else if (!battery->is_recharging &&
		(is_nocharge_type(prev_cable_type) || is_wireless_fake_type(prev_cable_type))) {
		pr_info("%s: c: %d, ov: %d, at: %d, cm: %d, tz: %d\n", __func__,
				battery->cable_type, battery->is_vbatovlo, battery->is_abnormal_temp,
				battery->charger_mode, battery->thermal_zone);
		clear_afc_evt =
			is_afc_evt_clear(battery->cable_type, battery->wire_status, battery->wc_status, battery->sink_status.rp_currentlvl);
		if (!clear_afc_evt)
			sec_bat_check_afc_input_current(battery);

		cw_prev_cable_is_nocharge(battery);

		sec_vote_refresh(battery->iv_vote);
	} else if (is_hv_wire_type(battery->cable_type) && (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC)) {
		clear_afc_evt =
			is_afc_evt_clear(battery->cable_type, battery->wire_status, battery->wc_status, battery->sink_status.rp_currentlvl);
	}

#if defined(CONFIG_STEP_CHARGING)
	if (!is_hv_wire_type(battery->cable_type) && !is_pd_wire_type(battery->cable_type)
		&& (battery->sink_status.rp_currentlvl != RP_CURRENT_LEVEL3))
		sec_bat_reset_step_charging(battery);
#endif

	/* set input voltage by cable type */
	cw_set_iv(battery);

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE) && defined(CONFIG_SEC_FACTORY)
	sec_bat_usb_factory_set_vote(battery, true);
#endif

	/* Check VOTER_SIOP to set up current based on cable_type */
	__pm_stay_awake(battery->siop_level_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);

	if (battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_FAKE) {
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AICL);
		sec_vote(battery->input_vote, VOTER_AICL, false, 0);
		sec_bat_check_input_voltage(battery, battery->cable_type);
		/* to init battery type current when wireless charging -> battery case */
		sec_vote_refresh(battery->input_vote);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		sec_wireless_otg_icl_control(battery);
#endif
		input_current = battery->pdata->charging_current[current_cable_type].input_current_limit;
		charging_current = battery->pdata->charging_current[current_cable_type].fast_charging_current;
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, charging_current);
		sec_vote(battery->input_vote, VOTER_CABLE, true, input_current);
	}

	if ((!is_nocharge_type(battery->cable_type) && battery->cable_type != SEC_BATTERY_CABLE_USB) || sec_bat_get_lpmode()) {
		sec_vote(battery->fcc_vote, VOTER_USB_100MA, false, 0);
		sec_vote(battery->input_vote, VOTER_USB_100MA, false, 0);
	}

	if (clear_afc_evt) {
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		sec_vote(battery->input_vote, VOTER_VBUS_CHANGE, false, 0);
	}
	store_battery_log(
		"CW:%d%%,%dmV,%s,ct(%s,%s,%d,%d),slate(%d),cev(0x%x),mev(0x%x)",
		battery->capacity,
		battery->voltage_now,
		sb_get_bst_str(battery->status),
		sb_get_ct_str(battery->cable_type),
		sb_get_ct_str(battery->wire_status),
		battery->muic_cable_type,
		battery->pd_usb_attached,
		is_slate_mode(battery),
		battery->current_event,
		battery->misc_event
		);

	/*
	 * polling time should be reset when cable is changed
	 * polling_in_sleep should be reset also
	 * before polling time is re-calculated
	 * to prevent from counting 1 for events
	 * right after cable is connected
	 */
	battery->polling_in_sleep = false;
	sec_bat_get_polling_time(battery);

	pr_info("%s: Status:%s, Sleep:%s, Charging:%s, Short Poll:%s\n",
		__func__, sb_get_bst_str(battery->status),
		battery->polling_in_sleep ? "Yes" : "No",
		(battery->charging_mode == SEC_BATTERY_CHARGING_NONE) ? "No" : "Yes",
		battery->polling_short ? "Yes" : "No");
	pr_info("%s: Polling time is reset to %d sec.\n", __func__, battery->polling_time);

	battery->polling_count = 1;	/* initial value = 1 */

run_monitor_work:
	__pm_stay_awake(battery->monitor_ws);
	/* run monitor_work immediately if SEC_BAT_CURRENT_EVENT_USB_SUSPENDED is cleared for timing issue */
	if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_USB_SUSPENDED))
		cancel_delayed_work(&battery->monitor_work);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, msecs_to_jiffies(monitor_work_delay));

	sb_set_vbus_wake(battery->vbus_ws, battery->health, battery->cable_type);
end_of_cable_work:
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	if ((battery->bc12_cable != SEC_BATTERY_CABLE_TIMEOUT) &&
				(battery->misc_event & BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE))
				sec_bat_set_misc_event(battery, 0,
					BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);
#endif

	__pm_relax(battery->cable_ws);
	dev_info(battery->dev, "%s: End\n", __func__);
}
EXPORT_SYMBOL_KUNIT(sec_bat_cable_work);

#define MAX_INPUT_CHECK_COUNT	3
__visible_for_testing void sec_bat_input_check_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, input_check_work.work);
	union power_supply_propval value = {0, };

	dev_info(battery->dev, "%s for %s start\n", __func__, sb_get_ct_str(battery->cable_type));
	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, value);
	battery->current_max = value.intval;

	if (battery->current_event & SEC_BAT_CURRENT_EVENT_AFC) {
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		sec_vote(battery->input_vote, VOTER_VBUS_CHANGE, false, 0);
#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_change_vbus(battery, battery->cable_type, battery->current_event, battery->siop_level);
#endif

		if (battery->cable_type == SEC_BATTERY_CABLE_TA)
			battery->cisd.cable_data[CISD_CABLE_TA]++;

#if defined(CONFIG_WIRELESS_TX_MODE)
		if (battery->wc_tx_enable)
			sec_bat_run_wpc_tx_work(battery, 0);
#endif
	} else if (battery->current_event & SEC_BAT_CURRENT_EVENT_SELECT_PDO) {
		int curr_pdo = 0, pdo = 0, iv = 0, icl = 0;

		iv = get_sec_vote_result(battery->iv_vote);
		if ((iv == SEC_INPUT_VOLTAGE_APDO) ||
			(sec_pd_get_pdo_power(&pdo, &iv, &iv, &icl) <= 0) ||
			(sec_pd_get_current_pdo(&curr_pdo) < 0) ||
			(pdo == curr_pdo)) {
			pr_info("%s: clear select_pdo event(%d, %d, %d)\n",
				__func__, iv, pdo, curr_pdo);

#if IS_ENABLED(CONFIG_MTK_CHARGER)
			if (sec_pd_get_current_pdo(&curr_pdo) >= 0) {
				pr_info("%s: sink_status.current_pdo_num(%d), curr_pdo(%d)\n",
					__func__, battery->sink_status.current_pdo_num, curr_pdo);
				battery->sink_status.current_pdo_num = curr_pdo;
				sec_bat_get_input_current_in_power_list(battery);
				sec_bat_get_charging_current_in_power_list(battery);
			}
#endif
			sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SELECT_PDO);
			sec_vote(battery->input_vote, VOTER_SELECT_PDO, false, 0);
			battery->input_check_cnt = 0;

			/* Check VOTER_SIOP to set up current after select pdo. */
			__pm_stay_awake(battery->siop_level_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);
		} else if ((++battery->input_check_cnt) % MAX_INPUT_CHECK_COUNT) {
			pr_info("%s: refresh(%d)!! pdo: %d, curr_pdo: %d\n",
				__func__, battery->input_check_cnt, pdo, curr_pdo);
			sec_vote_refresh(battery->iv_vote);
			return;
		}
	} else {
		dev_info(battery->dev, "%s: Nothing to do\n", __func__);
	}

	dev_info(battery->dev, "%s: End\n", __func__);
}
EXPORT_SYMBOL_KUNIT(sec_bat_input_check_work);

static void sec_bat_transit_clear_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, transit_clear_work.work);

	sec_vote(battery->chgen_vote, VOTER_SRCCAP_TRANSIT, false, 0);
	battery->srccap_transit = false;
	pr_info("%s: disable VOTER_SRCCAP_TRANSIT manually\n", __func__);
	__pm_relax(battery->transit_clear_ws);
}

static void sec_bat_usb_conn_check_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, usb_conn_check_work.work);

	pr_info("%s: usb_conn_check_cnt(%d)\n", __func__, ++battery->usb_conn_check_cnt);

	if (battery->usb_conn_check_cnt > MAX_USB_CONN_CHECK_CNT ||
		battery->usb_conn_status != USB_CONN_NORMAL ||
		battery->current_event & SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST ||
		battery->pdata->bat_thm_info.check_type == SEC_BATTERY_TEMP_CHECK_NONE) {
		battery->run_usb_conn_check = false;
		battery->usb_conn_check_cnt = 0;
		__pm_relax(battery->usb_conn_check_ws);
		return;
	}

	if (sec_usb_conn_check(battery) != USB_CONN_NORMAL) {
		pr_info("%s: Set BATT_MISC_EVENT_TEMP_HICCUP_TYPE in usb_conn_check_work\n", __func__);
		battery->run_usb_conn_check = false;
		battery->usb_conn_check_cnt = 0;
		battery->polling_count = 1;
		__pm_stay_awake(battery->monitor_ws);
		__pm_relax(battery->usb_conn_check_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	} else {
		queue_delayed_work(battery->monitor_wqueue,
			&battery->usb_conn_check_work, msecs_to_jiffies(1000));
	}
}

#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
static void sec_bat_handle_bc12_connection(struct sec_battery_info *battery)
{
	mutex_lock(&battery->bc12_notylock);

	dev_info(battery->dev,
		"%s: prev_cable_type(%d), bc12_cable(%d), wire_status(%d)\n",
		__func__, battery->cable_type, battery->bc12_cable, battery->wire_status);
#if defined(CONFIG_SUPPORT_HV_CTRL)
	if (battery->cable_type == SEC_BATTERY_CABLE_9V_TA &&
		battery->bc12_cable == SEC_BATTERY_CABLE_9V_TA) {
		sec_vote(battery->input_vote, VOTER_CABLE, true, battery->pdata->charging_current[battery->bc12_cable].input_current_limit);
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, battery->pdata->charging_current[battery->bc12_cable].fast_charging_current);
	}
#endif

	battery->wire_status = battery->bc12_cable;

	if (is_hv_wire_type(battery->bc12_cable)) {
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AICL);
		sec_vote(battery->input_vote, VOTER_AICL, false, 0);
	}
	if (battery->bc12_cable == SEC_BATTERY_CABLE_NONE) {
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		sec_bat_usb_factory_clear(battery);
#endif
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
#endif
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
		battery->init_src_cap = false;
		battery->sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
#endif
	}
#if IS_ENABLED(CONFIG_PDIC_NOTIFIER)
	else if (battery->cable_type != battery->bc12_cable &&
		battery->sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
		(battery->bc12_cable == SEC_BATTERY_CABLE_USB ||
			battery->bc12_cable == SEC_BATTERY_CABLE_TA)) {
		sec_bat_set_rp_current(battery, battery->bc12_cable);
	}
#endif
	else if ((battery->bc12_cable == SEC_BATTERY_CABLE_TIMEOUT) &&
			(!battery->init_src_cap))
		/*
		 * showing charging icon and noti(no sound, vi, haptic) only
		 * if slow insertion is detected by BC1.2
		 */
		sec_bat_set_misc_event(battery, BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE,
						BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);

	/* cable is attached or detached
	 * if battery->bc12_cable is minus value,
	 * check cable by sec_bat_get_cable_type()
	 * although SEC_BATTERY_CABLE_SOURCE_EXTERNAL is set
	 * (0 is SEC_BATTERY_CABLE_UNKNOWN)
	 */
	if ((battery->bc12_cable >= 0) &&
		(battery->bc12_cable < SEC_BATTERY_CABLE_MAX) &&
		(battery->pdata->cable_source_type &
		SEC_BATTERY_CABLE_SOURCE_EXTERNAL)) {

		__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
	} else {
		if (sec_bat_get_cable_type(battery,
					battery->pdata->cable_source_type)) {
			__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
		}
	}
	mutex_unlock(&battery->bc12_notylock);
} /* sec_bat_handle_bc12_connection */
#endif

#define TRANSIT_CNT 5
#define MAX_TRANSIT_CNT 100
static void sec_bat_check_srccap_transit(struct sec_battery_info *battery, int enable)
{
	int voter_status;

	pr_info("%s: set init_src_cap(%d->%d)\n",
		__func__, battery->init_src_cap, enable);

	if (enable) {
		battery->init_src_cap = true;
		return;
	}

	if (++battery->srccap_transit_cnt < TRANSIT_CNT) {
		sec_vote(battery->chgen_vote, VOTER_SRCCAP_TRANSIT, true, SEC_BAT_CHG_MODE_BUCK_OFF);
		battery->srccap_transit = true;
		battery->dc_ta_op_max_mode = DC_MODE_NONE;
		battery->dc_ta_forced_op_max_mode = DC_MODE_NONE;
		__pm_stay_awake(battery->transit_clear_ws);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->transit_clear_work, msecs_to_jiffies(10000));
		return;
	}

	if (battery->srccap_transit_cnt > MAX_TRANSIT_CNT)
		battery->srccap_transit_cnt = TRANSIT_CNT;
	voter_status = SEC_BAT_CHG_MODE_CHARGING;
	if (get_sec_voter_status(battery->chgen_vote, VOTER_SRCCAP_TRANSIT, &voter_status) < 0) {
		return;
	}
	pr_info("%s: voter_status: %d cnt: %d\n", __func__, voter_status, battery->srccap_transit_cnt);

	if (voter_status == SEC_BAT_CHG_MODE_BUCK_OFF) {
		sec_vote(battery->chgen_vote, VOTER_SRCCAP_TRANSIT, false, 0);
		battery->srccap_transit = false;
		pr_info("%s: disable VOTER_SRCCAP_TRANSIT manually\n", __func__);
	}
#if IS_ENABLED(CONFIG_SEC_ABC)
	sec_abc_send_event("MODULE=battery@WARN=dc_error");
#endif
}

static int sec_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	int current_cable_type = SEC_BATTERY_CABLE_NONE;
	int full_check_type = SEC_BATTERY_FULLCHARGED_NONE;
	union power_supply_propval value = {0, };
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	dev_dbg(battery->dev,
		"%s: (%d,%d)\n", __func__, psp, val->intval);

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (battery->charging_mode == SEC_BATTERY_CHARGING_1ST)
			full_check_type = battery->pdata->full_check_type;
		else
			full_check_type = battery->pdata->full_check_type_2nd;
		if ((full_check_type == SEC_BATTERY_FULLCHARGED_CHGINT) &&
			(val->intval == POWER_SUPPLY_STATUS_FULL))
			sec_bat_do_fullcharged(battery, false);
		sec_bat_set_charging_status(battery, val->intval);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (val->intval == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: unspec failure !!\n", __func__);
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_UNSPEC_FAILURE);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
				sec_vote(battery->chgen_vote, VOTER_VBAT_OVP, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
							&battery->monitor_work, 0);
			}
		} else if (val->intval == POWER_SUPPLY_EXT_HEALTH_VSYS_OVP) {
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: Vsys is ovlo !!\n", __func__);
				battery->is_sysovlo = true;
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				sec_bat_set_health(battery, POWER_SUPPLY_EXT_HEALTH_VSYS_OVP);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_VSYS_OVP, SEC_BAT_CURRENT_EVENT_VSYS_OVP);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
				battery->cisd.data[CISD_DATA_VSYS_OVP]++;
				battery->cisd.data[CISD_DATA_VSYS_OVP_PER_DAY]++;
#if IS_ENABLED(CONFIG_SEC_ABC)
				sec_abc_send_event("MODULE=battery@WARN=vsys_ovp");
#endif
				sec_vote(battery->chgen_vote, VOTER_SYSOVLO, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
							&battery->monitor_work, 0);
			}
		} else if (val->intval == POWER_SUPPLY_EXT_HEALTH_VBAT_OVP) {
			if (battery->status != POWER_SUPPLY_STATUS_DISCHARGING) {
				pr_info("%s: Vbat is ovlo !!\n", __func__);
				battery->is_vbatovlo = true;
				battery->is_recharging = false;
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				sec_bat_set_health(battery, POWER_SUPPLY_EXT_HEALTH_VBAT_OVP);
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_VBAT_OVP, SEC_BAT_CURRENT_EVENT_VBAT_OVP);
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);

				sec_vote(battery->chgen_vote, VOTER_VBAT_OVP, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
							&battery->monitor_work, 0);
			}
		}
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		current_cable_type = val->intval;
		if (current_cable_type < 0) {
			dev_info(battery->dev,
					"%s: ignore event(%d)\n",
					__func__, current_cable_type);
		} else {
			if (current_cable_type == SEC_BATTERY_CABLE_OTG) {
				battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
				battery->is_recharging = false;
				sec_bat_set_charging_status(battery,
						POWER_SUPPLY_STATUS_DISCHARGING);
				battery->cable_type = current_cable_type;
				__pm_stay_awake(battery->monitor_ws);
				queue_delayed_work(battery->monitor_wqueue,
						&battery->monitor_work, 0);
				break;
			} else {
				battery->wire_status = current_cable_type;
				if (is_nocharge_type(battery->wire_status) &&
						(battery->wc_status != SEC_BATTERY_CABLE_NONE))
					current_cable_type = SEC_BATTERY_CABLE_WIRELESS;
			}
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
			battery->bc12_cable = current_cable_type;

/* Skip notify from BC1.2 if PDIC is attached already */
			if ((is_pd_wire_type(battery->wire_status) || battery->init_src_cap) &&
				(battery->bc12_cable != SEC_BATTERY_CABLE_NONE)) {
				if (lpcharge)
					break;
				else if (battery->usb_conn_status == USB_CONN_NORMAL &&
					!(battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE))
					break;
			}
			sec_bat_handle_bc12_connection(battery);
#endif
		}
#if !IS_ENABLED(CONFIG_MTK_CHARGER) || IS_ENABLED(CONFIG_VIRTUAL_MUIC)
		dev_info(battery->dev,
				"%s: current_cable(%d), wc_status(%d), wire_status(%d)\n",
				__func__, current_cable_type, battery->wc_status,
				battery->wire_status);

		/*
		 * cable is attached or detached
		 * if current_cable_type is minus value,
		 * check cable by sec_bat_get_cable_type()
		 * although SEC_BATTERY_CABLE_SOURCE_EXTERNAL is set
		 * (0 is SEC_BATTERY_CABLE_UNKNOWN)
		 */
		if ((current_cable_type >= 0) &&
				(current_cable_type < SEC_BATTERY_CABLE_MAX) &&
				(battery->pdata->cable_source_type &
				 SEC_BATTERY_CABLE_SOURCE_EXTERNAL)) {

			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work,0);
		} else {
			if (sec_bat_get_cable_type(battery,
						battery->pdata->cable_source_type)) {
				__pm_stay_awake(battery->cable_ws);
				queue_delayed_work(battery->monitor_wqueue,
						&battery->cable_work,0);
			}
		}
#endif
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		battery->capacity = val->intval;
		pr_info("%s: set POWER_SUPPLY_PROP_CAPACITY(%d)\n", __func__, val->intval);
		power_supply_changed(battery->psy_bat);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		battery->present = val->intval;

		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		pr_info("%s: Valert was occurred! run monitor work for updating cisd data!\n", __func__);
		battery->cisd.data[CISD_DATA_VALERT_COUNT]++;
		battery->cisd.data[CISD_DATA_VALERT_COUNT_PER_DAY]++;
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work_on(0, battery->monitor_wqueue,
			&battery->monitor_work, 0);
		break;
	case POWER_SUPPLY_PROP_ENERGY_NOW:
		if (!battery->factory_mode_boot_on)
			factory_mode = 1;
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_AICL_CURRENT:
			battery->max_charge_power = battery->charge_power =
				mW_by_mVmA(battery->input_voltage, val->intval);
			/* Voter should be removed after all chargers is fixed */
			sec_vote(battery->input_vote, VOTER_AICL, true, val->intval);
#if defined(CONFIG_SUPPORT_HV_CTRL) && !defined(CONFIG_SEC_FACTORY)
			if (battery->pdata->boosting_voltage_aicl) {
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_HV_BY_AICL,
					BATT_MISC_EVENT_HV_BY_AICL);
				if (sb_ct_has_9v(battery, battery->cable_type)) {
					sec_vote(battery->input_vote, VOTER_AICL, false, 0);
					sec_vote(battery->iv_vote, VOTER_AICL, true, SEC_INPUT_VOLTAGE_9V);

					/* Check siop level to set current */
					__pm_stay_awake(battery->siop_level_ws);
					queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);
				}
			}
#endif

			pr_info("%s: aicl : %dmA, %dmW)\n", __func__,
				val->intval, battery->charge_power);
			if (is_wired_type(battery->cable_type)) {
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_AICL,
					SEC_BAT_CURRENT_EVENT_AICL);
				store_battery_log(
					"AICL:%d%%,curr(%dmA),%dmV,%s,ct(%d,%d,%d,%d),cev(0x%x)",
					battery->capacity,
					val->intval,
					battery->voltage_now,
					sb_get_bst_str(battery->status),
					battery->cable_type,
					battery->wire_status,
					battery->muic_cable_type,
					battery->pd_usb_attached,
					battery->current_event
					);
			}
			battery->cisd.data[CISD_DATA_AICL_COUNT]++;
			battery->cisd.data[CISD_DATA_AICL_COUNT_PER_DAY]++;
			break;
		case POWER_SUPPLY_EXT_PROP_USB_CONFIGURE:
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE) && defined(CONFIG_SEC_FACTORY)
			pr_info("%s: usb configured %d\n", __func__, val->intval);

			if (sec_bat_usb_factory_set_vote(battery, true))
				break;
#endif
			if (val->intval == USB_CURRENT_CLEAR || val->intval != battery->prev_usb_conf)
				sec_bat_set_usb_configure(battery, val->intval);

			break;
		case POWER_SUPPLY_EXT_PROP_OVERHEAT_NOTIFY:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_OVERHEAT_NOTIFY!\n", __func__);
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue,
						&battery->monitor_work, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_HV_DISABLE:
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if defined(CONFIG_PD_CHARGER_HV_DISABLE)
			pr_info("None PD wired charging mode is %s\n", (val->intval == CH_MODE_AFC_DISABLE_VAL ? "Disabled" : "Enabled"));
			if (val->intval == CH_MODE_AFC_DISABLE_VAL) {
				sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE, SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE);
			} else {
				sec_bat_set_current_event(battery,
					0, SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE);
			}
#else
			pr_info("HV wired charging mode is %s\n", (val->intval == CH_MODE_AFC_DISABLE_VAL ? "Disabled" : "Enabled"));
			if (val->intval == CH_MODE_AFC_DISABLE_VAL) {
				sec_bat_set_current_event(battery,
					SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
			} else {
				sec_bat_set_current_event(battery,
					0, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
			}

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
#endif
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
			if ((battery->cable_type == SEC_BATTERY_CABLE_TA) &&
				(battery->sink_status.rp_currentlvl == RP_CURRENT_LEVEL3)) {
				sec_bat_set_rp_current(battery, battery->cable_type);
				sec_vote(battery->fcc_vote, VOTER_CABLE, true,
					battery->pdata->charging_current[battery->cable_type].fast_charging_current);
				sec_vote(battery->input_vote, VOTER_CABLE, true,
					battery->pdata->charging_current[battery->cable_type].input_current_limit);
			}
#endif
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_WC_CONTROL:
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
			mutex_lock(&battery->wclock);
			pr_info("%s: Recover MFC IC (wc_enable: %d)\n",
				__func__, battery->wc_enable);

			if (battery->wc_enable) {
				sec_bat_set_mfc_off(battery, WPC_EN_CHARGING, false);
				msleep(500);

				sec_bat_set_mfc_on(battery, WPC_EN_CHARGING);
			}
			mutex_unlock(&battery->wclock);
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_WDT_STATUS:
			if (val->intval)
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_WDT_EXPIRED,
					SEC_BAT_CURRENT_EVENT_WDT_EXPIRED);
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_EVENT:
			if (!(battery->current_event & val->intval)) {
				pr_info("%s: set new current_event %d\n", __func__, val->intval);
				sec_bat_set_current_event(battery, val->intval, val->intval);
				if (val->intval == SEC_BAT_CURRENT_EVENT_DC_ERR) {
					battery->cisd.event_data[EVENT_DC_ERR]++;
					psy_do_property(battery->pdata->charger_name, get,
							POWER_SUPPLY_EXT_PROP_DC_ERROR_CAUSE, value);
					count_cisd_dcerr_data(&battery->cisd, (0xFF & value.intval));
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_EVENT_CLEAR:
			pr_info("%s: new current_event clear %d\n", __func__, val->intval);
			sec_bat_set_current_event(battery, 0, val->intval);
			break;
#if defined(CONFIG_WIRELESS_TX_MODE)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_AVG_CURR:
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE:
			sec_wireless_set_tx_enable(battery, val->intval);
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_SRCCAP:
			sec_bat_check_srccap_transit(battery, val->intval);
			break;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		case POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT:
			if (battery->ta_alert_wa) {
				pr_info("@TA_ALERT: %s: TA OCP DETECT\n", __func__);
				battery->cisd.event_data[EVENT_TA_OCP_DET]++;
				if (battery->ta_alert_mode == OCP_NONE)
					battery->cisd.event_data[EVENT_TA_OCP_ON]++;
				battery->ta_alert_mode = OCP_DETECT;
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_25W_OCP,
							SEC_BAT_CURRENT_EVENT_25W_OCP);
				store_battery_log(
				"TA OCP:%d%%,%dmV,%s,ct(%d,%d,%d,%d),cev(0x%x),mev(0x%x)",
				battery->capacity,
				battery->voltage_now,
				sb_get_bst_str(battery->status),
				battery->cable_type,
				battery->wire_status,
				battery->muic_cable_type,
				battery->pd_usb_attached,
				battery->current_event,
				battery->misc_event
				);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_SEND_UVDM:
			if (is_pd_apdo_wire_type(battery->cable_type)) {
				char direct_charging_source_status[2] = {0, };

				pr_info("@SEND_UVDM: Request Change Charging Source : %s\n",
					val->intval == 0 ? "Switch Charger" : "Direct Charger" );

				sec_bat_reset_step_charging(battery);
				direct_charging_source_status[0] = SEC_SEND_UVDM;
				direct_charging_source_status[1] = val->intval;

				sec_bat_set_current_event(battery, val->intval == 0 ?
					SEC_BAT_CURRENT_EVENT_SEND_UVDM : 0, SEC_BAT_CURRENT_EVENT_SEND_UVDM);
				value.strval = direct_charging_source_status;
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_FIXED_PDO:
			if (!is_slate_mode(battery) && !is_wireless_all_type(battery->cable_type)) {
				sec_vote(battery->iv_vote, VOTER_DC_MODE, true, val->intval);
				if (val->intval == SEC_INPUT_VOLTAGE_APDO) {
					sec_vote(battery->iv_vote, VOTER_AICL, false, 0);
					sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_HV_BY_AICL);
				}
			}
			break;
#endif
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		case POWER_SUPPLY_EXT_PROP_FASTCHG_LIMIT_CURRENT:
			if (is_wireless_type(battery->cable_type))
				sec_bat_set_limiter_current(battery);
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_WPC_EN:
			sec_bat_set_current_event(battery,
				val->intval ? 0 : SEC_BAT_CURRENT_EVENT_WPC_EN, SEC_BAT_CURRENT_EVENT_WPC_EN);
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL:
			value.intval = val->intval;
			pr_info("%s: WCIN-UNO %s\n", __func__, value.intval > 0 ? "on" : "off");
			psy_do_property("otg", set,
					POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
			break;
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		case POWER_SUPPLY_EXT_PROP_BATT_F_MODE:
			battery->batt_f_mode = val->intval;
			if (battery->batt_f_mode == OB_MODE) {
				sec_bat_set_facmode(true);
				battery->factory_mode = true;
			}  else if (((battery->cable_type == SEC_BATTERY_CABLE_NONE) &&
					(battery->wire_status != SEC_BATTERY_CABLE_NONE)) &&
					(battery->batt_f_mode == IB_MODE)) {
				battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
				sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
				battery->cable_type = battery->wire_status;
				sec_bat_set_facmode(false);
				battery->factory_mode = false;
			} else {
				sec_bat_set_facmode(false);
				battery->factory_mode = false;
			}
			value.intval = battery->batt_f_mode;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				POWER_SUPPLY_EXT_PROP_BATT_F_MODE, value);
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_INPUT_VOLTAGE_REGULATION:
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONTROL:
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_COUNTER_SHADOW:
			break;
#if defined(CONFIG_UPDATE_BATTERY_DATA)
		case POWER_SUPPLY_EXT_PROP_POWER_DESIGN:
			sec_bat_parse_dt(battery->dev, battery);
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_MFC_FW_UPDATE:
			battery->mfc_fw_update = val->intval;
			if (!battery->mfc_fw_update) {
				pr_info("%s: fw update done: (5V -> 9V).\n", __func__);
				sec_vote(battery->chgen_vote, VOTER_FW, false, 0);
				sec_vote(battery->iv_vote, VOTER_FW, false, 0);
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
#if !IS_ENABLED(CONFIG_MTK_CHARGER) || !IS_ENABLED(CONFIG_AFC_CHARGER)
				muic_afc_request_voltage(AFC_REQUEST_MFC, SEC_INPUT_VOLTAGE_9V / 1000);
#endif
#endif
			}
			break;
		case POWER_SUPPLY_EXT_PROP_THERMAL_ZONE:
			pr_info("%s : bat_thm_info.check_type set to %s\n", __func__, val->intval ? "NONE" : "TEMP");
			if (val->intval) {
				battery->skip_swelling = true;	/* restore thermal_zone to NORMAL */
				battery->pdata->bat_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				battery->pdata->lrp_temp_check_type = SEC_BATTERY_TEMP_CHECK_NONE;
				sec_vote(battery->iv_vote, VOTER_LRP_TEMP, false, 0);
				sec_vote(battery->fcc_vote, VOTER_LRP_TEMP, false, 0);
				sec_vote(battery->input_vote, VOTER_LRP_TEMP, false, 0);
				battery->lrp_limit = false;
				battery->lrp_step = LRP_NONE;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
				battery->lrp_chg_src = SEC_CHARGING_SOURCE_DIRECT;
#endif
			} else {
				battery->skip_swelling = false;
				battery->pdata->bat_thm_info.check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				battery->pdata->lrp_temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP;
				/* Check VOTER_SIOP to set up current based on cable_type */
				__pm_stay_awake(battery->siop_level_ws);
				queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);

#if !defined(CONFIG_SEC_FACTORY)
				sec_bat_check_lrp_temp(battery,
					battery->cable_type, battery->wire_status,
					battery->siop_level, battery->lcd_status);
#endif
			}
			break;
		case POWER_SUPPLY_EXT_PROP_TEMP_CHECK_TYPE:
			break;
		case POWER_SUPPLY_EXT_PROP_SUB_TEMP:
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_FREQ_STRENGTH:
			pr_info("%s : wpc vout strength(%d)\n", __func__, val->intval);
			sec_bat_set_misc_event(battery,
				((val->intval <= 0) ? BATT_MISC_EVENT_WIRELESS_MISALIGN : 0),
				BATT_MISC_EVENT_WIRELESS_MISALIGN);
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL:
			value.intval = val->intval;
			pr_info("%s: d2d reverse boost : %d\n", __func__, val->intval);
			if (val->intval) {
				if (!battery->d2d_check) {
					battery->cisd.event_data[EVENT_D2D]++;
					battery->d2d_check = true;
				}
				sec_vote(battery->chgen_vote, VOTER_D2D_WIRE, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			}
			psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VOLTAGE, value);
			if (!val->intval) {
				battery->d2d_check = false;
				sec_vote(battery->chgen_vote, VOTER_D2D_WIRE, false, 0);
				battery->vpdo_auth_stat = AUTH_NONE;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_LCD_FLICKER:
			battery->wpc_vout_ctrl_mode = (val->intval && battery->pdata->wpc_vout_ctrl_lcd_on) ? 1 : 0;
			pr_info("%s: %s display flicker wa\n", __func__,
				(battery->wpc_vout_ctrl_mode) ? "enable" : "disable");
			break;
		case POWER_SUPPLY_EXT_PROP_FLASH_STATE: /* check only for MTK */
			battery->flash_state = val->intval;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
			if (is_pd_apdo_wire_type(battery->cable_type))
				psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);
#endif
			if (val->intval) {
				pr_info("%s: Flash on: (9V -> 5V).\n", __func__);
				sec_vote(battery->iv_vote, VOTER_FLASH, true, SEC_INPUT_VOLTAGE_5V);
			} else {
				pr_info("%s: Flash off: (5V -> 9V).\n", __func__);
				sec_vote(battery->iv_vote, VOTER_FLASH, false, 0);
			}
			break;
#if IS_ENABLED(CONFIG_MTK_CHARGER)
		case POWER_SUPPLY_EXT_PROP_MTK_FG_INIT: /* check only for MTK */
			battery->mtk_fg_init = val->intval;
			pr_info("%s: mtk_fg_init (%d)\n", __func__, battery->mtk_fg_init);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
			if (is_pd_apdo_wire_type(battery->cable_type))
				psy_do_property(battery->pdata->charger_name, set,
						POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);
#endif
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_USB_BOOTCOMPLETE:
			battery->usb_bootcomplete = val->intval;
			pr_info("%s: usb_bootcomplete (%d)\n", __func__, battery->usb_bootcomplete);
			break;
		case POWER_SUPPLY_EXT_PROP_MISC_EVENT:
			if (!(battery->misc_event & val->intval)) {
				pr_info("%s: set new misc_event %d\n", __func__, val->intval);
				sec_bat_set_misc_event(battery, val->intval, val->intval);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_MISC_EVENT_CLEAR:
			pr_info("%s: new misc_event clear %d\n", __func__, val->intval);
			sec_bat_set_misc_event(battery, 0, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_MST_EN:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_MST_EN(%d)\n", __func__, val->intval);
			battery->mst_en = val->intval;
			if (val->intval)
				sec_vote(battery->iv_vote, VOTER_MST, true, SEC_INPUT_VOLTAGE_5V);
			else
				sec_vote(battery->iv_vote, VOTER_MST, false, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_ABNORMAL_SRCCAP:
			store_battery_log(
				"ABNORMAL_SRCCAP:%d%%,%dmV,%s,PDO(0x%X)",
				battery->capacity,
				battery->voltage_now,
				sb_get_bst_str(battery->status),
				val->intval
			);
			break;
		case POWER_SUPPLY_EXT_PROP_HARDRESET_OCCUR:
			if (is_pd_wire_type(battery->cable_type)) {
				battery->sink_status.selected_pdo_num = -1;
				store_battery_log(
					"HARDRESET_%s:%d%%,%dmV,%s,SELPDO(%d),CURPDO(%d)",
					val->intval ? "SENT" : "RECEIVED",
					battery->capacity,
					battery->voltage_now,
					sb_get_bst_str(battery->status),
					battery->sink_status.selected_pdo_num,
					battery->sink_status.current_pdo_num
				);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_ABNORMAL_TA:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_ABNORMAL_TA(%d)\n", __func__, val->intval);
			sec_bat_set_abnormal_ta_fcc(battery, true);
			break;
		case POWER_SUPPLY_EXT_PROP_RX_PHM:
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			pr_info("%s : PHM set %d\n", __func__, val->intval);
			if (val->intval)
				sec_vote(battery->chgen_vote, VOTER_PHM, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			else
				sec_vote(battery->chgen_vote, VOTER_PHM, false, 0);
#endif
			break;
#ifdef CONFIG_BATTERY_HOTSWAP
		case POWER_SUPPLY_EXT_PROP_BATTERY_HOT_SWAP_STATUS:
			pr_info("%s: POWER_SUPPLY_EXT_PROP_BATTERY_HOT_SWAP_STATUS(%d)\n", __func__, val->intval);
			value.intval = val->intval;
			psy_do_property(battery->pdata->fuelgauge_name, set,
				(enum power_supply_property)POWER_SUPPLY_EXT_PROP_BATTERY_HOT_SWAP_STATUS, value);

			power_supply_changed(battery->psy_bat);

			if (!is_nocharge_type(battery->cable_type) && !val->intval) { // removal
				sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_SLATE, SEC_BAT_CURRENT_EVENT_SLATE);
				sec_vote(battery->chgen_vote, VOTER_SLATE, true, SEC_BAT_CHG_MODE_BUCK_OFF);

				value.intval = 1;
				psy_do_property(battery->pdata->dual_battery_name, set,
					POWER_SUPPLY_EXT_PROP_CLEAR_FULL_STATUS, value);
				sec_bat_get_battery_info(battery);
				battery->skip_swelling = true;
				sec_bat_thermal_check(battery);
			} else if (val->intval) { // insertion
				sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_SLATE);
				sec_bat_check_dual_battery_balance(battery);
				sec_vote(battery->chgen_vote, VOTER_SLATE, false, 0);
				sec_bat_get_battery_info(battery);
				sec_bat_dual_battery_check_eoc(battery);
				battery->skip_swelling = false;
			}
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue,
					   &battery->cable_work, 0);
			break;
#endif
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_bat_check_status(struct sec_battery_info *battery)
{
	/* need to update for DPM UI */
	if (battery->misc_event & BATT_MISC_EVENT_DIRECT_POWER_MODE)
		return POWER_SUPPLY_STATUS_NOT_CHARGING;

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
		(battery->health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE))
		return POWER_SUPPLY_STATUS_DISCHARGING;

	if ((battery->pdata->cable_check_type &
		SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE) &&
		!sec_bat_get_lpmode()) {
		switch (battery->cable_type) {
		case SEC_BATTERY_CABLE_USB:
		case SEC_BATTERY_CABLE_USB_CDP:
			return POWER_SUPPLY_STATUS_DISCHARGING;
		}
	}

#if defined(CONFIG_STORE_MODE)
	if (battery->store_mode && !sec_bat_get_lpmode() &&
		!is_nocharge_type(battery->cable_type) &&
		battery->status == POWER_SUPPLY_STATUS_DISCHARGING)
		return POWER_SUPPLY_STATUS_CHARGING;
#endif

	if (is_eu_eco_rechg(battery->fs) &&
		(battery->status == POWER_SUPPLY_STATUS_FULL)) {
		if (battery->is_recharging) {
#if defined(CONFIG_ENABLE_FULL_BY_SOC)
			if (battery->capacity < 100)
				return POWER_SUPPLY_STATUS_CHARGING;
#else
			return POWER_SUPPLY_STATUS_CHARGING;
#endif
		}

		if (check_eu_eco_rechg_ui_condition(battery))
			return POWER_SUPPLY_STATUS_CHARGING;
	}

	return battery->status;
}

static int sec_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0, };
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = sec_bat_check_status(battery);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	{
		unsigned int input_current = battery->pdata->charging_current[battery->cable_type].input_current_limit;
		if (is_nocharge_type(battery->cable_type)) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		} else if (is_hv_wire_type(battery->cable_type) || is_pd_wire_type(battery->cable_type) || is_wireless_all_type(battery->cable_type)) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		} else if (!battery->usb_bootcomplete && !lpcharge && battery->pdata->slowcharging_usb_bootcomplete) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		} else if (input_current <= SLOW_CHARGING_CURRENT_STANDARD || battery->usb_slow_chg) {
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		} else {
			psy_do_property(battery->pdata->charger_name, get,
				POWER_SUPPLY_PROP_CHARGE_TYPE, value);
			if (value.intval == SEC_BATTERY_CABLE_UNKNOWN)
				/*
				 * if error in CHARGE_TYPE of charger
				 * set CHARGE_TYPE as NONE
				 */
				val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			else
				val->intval = value.intval;
		}
	}
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (sec_bat_get_lpmode() &&
			(battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE ||
			battery->health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE ||
			battery->health == POWER_SUPPLY_EXT_HEALTH_DC_ERR)) {
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		} else if (battery->health >= POWER_SUPPLY_EXT_HEALTH_MIN) {
			if (battery->health == POWER_SUPPLY_EXT_HEALTH_OVERHEATLIMIT)
				val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			else
				val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		} else {
			val->intval = battery->health;
		}
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = battery->present;

		/* BATT_MISC_EVENT_DIRECT_POWER_MODE event is activated only in the DPM application model. */
		/* need to update for SSRM cooldown mode */
		if (battery->misc_event & BATT_MISC_EVENT_DIRECT_POWER_MODE)
			val->intval = 0;

		break;
	case POWER_SUPPLY_PROP_ONLINE:
		/* SEC_BATTERY_CABLE_SILENT_TYPE, defines with PMS team for avoid charger connection sound */
		if (check_silent_type(battery->muic_cable_type))
			val->intval = SEC_BATTERY_CABLE_SILENT_TYPE;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		else if (is_hv_wireless_type(battery->cable_type) ||
			(battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_HV)) {
			if (sec_bat_hv_wc_normal_mode_check(battery))
				val->intval = SEC_BATTERY_CABLE_WIRELESS;
			else
				val->intval = SEC_BATTERY_CABLE_HV_WIRELESS_ETX;
		} else if ((battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20) ||
			(battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_EPP_FAKE)) {
			if (sec_bat_hv_wc_normal_mode_check(battery) ||
				(battery->wc20_rx_power < WFC10_WIRELESS_POWER))
				val->intval = SEC_BATTERY_CABLE_WIRELESS;
			else
				val->intval = SEC_BATTERY_CABLE_HV_WIRELESS_ETX;
		} else if (battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_PACK ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_STAND ||
			battery->cable_type == SEC_BATTERY_CABLE_PMA_WIRELESS ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_VEHICLE ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_TX ||
			battery->cable_type == SEC_BATTERY_CABLE_WIRELESS_FAKE) {
			val->intval = SEC_BATTERY_CABLE_WIRELESS;
		}
#endif
		else
			val->intval = battery->cable_type;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = battery->pdata->technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
#ifdef CONFIG_SEC_FACTORY
		value.intval = SEC_BATTERY_VOLTAGE_MV;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		battery->voltage_now = value.intval;
		dev_err(battery->dev,
			"%s: voltage now(%d)\n", __func__, battery->voltage_now);
#endif
		/* voltage value should be in uV */
		val->intval = battery->voltage_now * 1000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_AVG:
#ifdef CONFIG_SEC_FACTORY
		value.intval = SEC_BATTERY_VOLTAGE_MV;
		psy_do_property(battery->pdata->fuelgauge_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
		battery->voltage_avg = value.intval;
		dev_err(battery->dev,
			"%s: voltage avg(%d)\n", __func__, battery->voltage_avg);
#endif
		/* voltage value should be in uV */
		val->intval = battery->voltage_avg * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		value.intval = SEC_BATTERY_CURRENT_UA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CURRENT_NOW, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		value.intval = SEC_BATTERY_CURRENT_UA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_CURRENT_AVG, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = battery->pdata->battery_full_capacity * 1000;
		break;
	/* charging mode (differ from power supply) */
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = battery->charging_mode;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		if (battery->pdata->fake_capacity) {
			val->intval = 70;
			pr_info("%s : capacity(%d)\n", __func__, val->intval);
		} else {
			val->intval = battery->capacity;
			if ((battery->status == POWER_SUPPLY_STATUS_FULL) &&
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
				!battery->eng_not_full_status &&
#endif
				!is_eu_eco_rechg(battery->fs))
				val->intval = 100;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = battery->temperature;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = battery->temper_amb;
		break;
#if IS_ENABLED(CONFIG_FUELGAUGE_MAX77705) || IS_ENABLED(CONFIG_FUELGAUGE_MAX77775)
	case POWER_SUPPLY_PROP_POWER_NOW:
		value.intval = SEC_BATTERY_ISYS_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
		val->intval = value.intval;
		break;
	case POWER_SUPPLY_PROP_POWER_AVG:
		value.intval = SEC_BATTERY_ISYS_AVG_MA;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_MEASURE_SYS, value);
		val->intval = value.intval;
		break;
#endif
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = ttf_display(battery->capacity, battery->status,
				battery->thermal_zone, battery->ttf_d->timetofull);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_SWELLING_MODE)
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		val->intval = battery->charge_counter;
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_CHARGE_POWER:
			val->intval = battery->charge_power;
			break;
		case POWER_SUPPLY_EXT_PROP_CURRENT_EVENT:
			val->intval = battery->current_event;
			break;
#if defined(CONFIG_WIRELESS_TX_MODE)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_AVG_CURR:
			val->intval = battery->tx_avg_curr;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ENABLE:
			val->intval = battery->wc_tx_enable;
			break;
#endif
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE:
			val->intval = battery->pd_list.now_isApdo;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_HAS_APDO:
			val->intval = battery->sink_status.has_apdo;
			break;
		case POWER_SUPPLY_EXT_PROP_PAD_VOLT_CTRL:
			if (battery->pdata->wpc_vout_ctrl_lcd_on)
				val->intval = battery->lcd_status;
			else
				val->intval = false;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_TA_ALERT:
			if (battery->ta_alert_wa) {
				val->intval = battery->ta_alert_mode;
			} else
				val->intval = OCP_NONE;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_SEND_UVDM:
			break;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
		case POWER_SUPPLY_EXT_PROP_DIRECT_VBAT_CHECK:
		{
			int main_voltage = battery->voltage_avg_main;
			int sub_voltage = battery->voltage_avg_sub;
			if (battery->pdata->dc_step_chg_use_vnow) {
				main_voltage = battery->voltage_now_main;
				sub_voltage = battery->voltage_now_sub;
			}
			pr_info("%s : step(%d/%d), vmp:%dmV, vsp:%dmV\n", __func__,
				battery->step_chg_status, battery->dc_step_chg_step - 1,
				main_voltage, sub_voltage);
			if ((battery->step_chg_status == battery->dc_step_chg_step - 1) &&
				((main_voltage >= battery->pdata->sc_vbat_thresh_main) ||
				(sub_voltage >= battery->pdata->sc_vbat_thresh_sub)))
				val->intval = 1;
			else
				val->intval = 0;
		}
			break;
		case POWER_SUPPLY_EXT_PROP_VOLTAGE_PACK_MAIN:
			val->intval = battery->voltage_avg_main;
			break;
		case POWER_SUPPLY_EXT_PROP_VOLTAGE_PACK_SUB:
			val->intval = battery->voltage_avg_sub;
			break;
#endif
#endif
		case POWER_SUPPLY_EXT_PROP_HV_PDO:
			val->intval = battery->hv_pdo;
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL:
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_COUNTER_SHADOW:
			val->intval = battery->wire_status;
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL:
			break;
		case POWER_SUPPLY_EXT_PROP_HEALTH:
			val->intval = battery->health;
			break;
		case POWER_SUPPLY_EXT_PROP_MFC_FW_UPDATE:
			val->intval = battery->mfc_fw_update;
			break;
		case POWER_SUPPLY_EXT_PROP_THERMAL_ZONE:
			val->intval = battery->thermal_zone;
			break;
		case POWER_SUPPLY_EXT_PROP_TEMP_CHECK_TYPE:
			switch (val->intval) {
			case THM_INFO_BAT:
				val->intval = battery->pdata->bat_thm_info.check_type;
				break;
			case THM_INFO_USB:
				val->intval = battery->pdata->usb_thm_info.check_type;
				break;
			case THM_INFO_CHG:
				val->intval = battery->pdata->chg_thm_info.check_type;
				break;
			case THM_INFO_WPC:
				val->intval = battery->pdata->wpc_thm_info.check_type;
				break;
			case THM_INFO_SUB_BAT:
				val->intval = battery->pdata->sub_bat_thm_info.check_type;
				break;
			case THM_INFO_BLK:
				val->intval = battery->pdata->blk_thm_info.check_type;
				break;
			case THM_INFO_DCHG:
				val->intval = battery->pdata->dchg_thm_info.check_type;
				break;
			default:
				val->intval = SEC_BATTERY_TEMP_CHECK_NONE;
				break;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_SUB_TEMP:
			val->intval = battery->sub_bat_temp;
			break;
		case POWER_SUPPLY_EXT_PROP_MIX_LIMIT:
			val->intval = battery->mix_limit;
			break;
		case POWER_SUPPLY_EXT_PROP_WPC_FREQ_STRENGTH:
			break;
		case POWER_SUPPLY_EXT_PROP_LCD_FLICKER:
			val->intval = (battery->lcd_status && battery->wpc_vout_ctrl_mode) ? 1 : 0;
			break;
		case POWER_SUPPLY_EXT_PROP_FLASH_STATE:  /* check only for MTK */
			val->intval = battery->flash_state;
			break;
#if IS_ENABLED(CONFIG_MTK_CHARGER)
		case POWER_SUPPLY_EXT_PROP_MTK_FG_INIT:	 /* check only for MTK */
			val->intval = battery->mtk_fg_init;
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_SRCCAP:
			val->intval = battery->init_src_cap;
			break;
#if IS_ENABLED(CONFIG_MTK_CHARGER)
		case POWER_SUPPLY_EXT_PROP_RP_LEVEL:
			val->intval = battery->sink_status.rp_currentlvl;
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_LRP_CHG_SRC:
			val->intval = battery->lrp_chg_src;
			break;
		case POWER_SUPPLY_EXT_PROP_MISC_EVENT:
			val->intval = battery->misc_event;
			break;
		case POWER_SUPPLY_EXT_PROP_MST_EN:
			val->intval = battery->mst_en;
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_DONE:
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
			psy_do_property(battery->pdata->charger_name, get,
					POWER_SUPPLY_EXT_PROP_DIRECT_DONE, value);
			val->intval = value.intval;
#else
			val->intval = 0;
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_FPDO_DC_THERMAL_CHECK:
			pr_info("%s:  FPDO_DC, Tbat(%d), chg_limit(%d), lrp_limit(%d), siop(%d), tz(%d), pdo(%d)\n",
				__func__, battery->temperature, battery->chg_limit, battery->lrp_limit,
				battery->siop_level, battery->thermal_zone, battery->sink_status.current_pdo_num);
			if (battery->chg_limit || battery->lrp_limit || battery->siop_level < 80 ||
					battery->thermal_zone != BAT_THERMAL_NORMAL ||
					battery->sink_status.current_pdo_num < 2 ||
					battery->temperature <= battery->pdata->wire_cool1_normal_thresh ||
					battery->temperature >= battery->pdata->wire_normal_warm_thresh)
				val->intval = 1;
			else
				val->intval = 0;
			break;
		case POWER_SUPPLY_EXT_PROP_ABNORMAL_TA:
			if (battery->abnormal_ta && battery->charging_current <= MIN_FCC_VALUE)
				val->intval = 1;
			else
				val->intval = 0;
			break;
		case POWER_SUPPLY_EXT_PROP_DC_TA_MAX_OP:
			val->intval = battery->dc_ta_op_max_mode;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sec_usb_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
		(battery->health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE)) {
		val->intval = 0;
		return 0;
	}
	/* Set enable=1 only if the USB charger is connected */
	switch (battery->wire_status) {
	case SEC_BATTERY_CABLE_USB:
	case SEC_BATTERY_CABLE_USB_CDP:
		val->intval = 1;
		break;
	case SEC_BATTERY_CABLE_PDIC:
	case SEC_BATTERY_CABLE_FPDO_DC:
		val->intval = (battery->pd_usb_attached) ? 1:0;
		break;
	default:
		val->intval = 0;
		break;
	}

	if (is_slate_mode(battery))
		val->intval = 0;
	return 0;
}

static int sec_ac_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if ((battery->health == POWER_SUPPLY_HEALTH_OVERVOLTAGE) ||
				(battery->health == POWER_SUPPLY_EXT_HEALTH_UNDERVOLTAGE)) {
			val->intval = 0;
			return 0;
		}

		/* Set enable=1 only if the AC charger is connected */
		switch (battery->cable_type) {
		case SEC_BATTERY_CABLE_TA:
		case SEC_BATTERY_CABLE_UARTOFF:
		case SEC_BATTERY_CABLE_LAN_HUB:
		case SEC_BATTERY_CABLE_LO_TA:
		case SEC_BATTERY_CABLE_UNKNOWN:
		case SEC_BATTERY_CABLE_PREPARE_TA:
		case SEC_BATTERY_CABLE_9V_ERR:
		case SEC_BATTERY_CABLE_9V_UNKNOWN:
		case SEC_BATTERY_CABLE_9V_TA:
		case SEC_BATTERY_CABLE_12V_TA:
		case SEC_BATTERY_CABLE_HMT_CONNECTED:
		case SEC_BATTERY_CABLE_HMT_CHARGE:
		case SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT:
		case SEC_BATTERY_CABLE_QC20:
		case SEC_BATTERY_CABLE_QC30:
		case SEC_BATTERY_CABLE_TIMEOUT:
		case SEC_BATTERY_CABLE_SMART_OTG:
		case SEC_BATTERY_CABLE_SMART_NOTG:
		case SEC_BATTERY_CABLE_POGO:
		case SEC_BATTERY_CABLE_POGO_9V:
		case SEC_BATTERY_CABLE_PDIC_APDO:
			val->intval = 1;
			break;
		case SEC_BATTERY_CABLE_PDIC:
		case SEC_BATTERY_CABLE_FPDO_DC:
			val->intval = (battery->pd_usb_attached) ? 0:1;
			break;
		default:
			val->intval = 0;
			break;
		}
		if (sec_bat_get_lpmode() && (battery->misc_event & BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE))
			val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = battery->chg_temp;
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
			case POWER_SUPPLY_EXT_PROP_WATER_DETECT:
				if (battery->misc_event & (BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE |
					BATT_MISC_EVENT_WATER_HICCUP_TYPE)) {
					val->intval = 1;
					pr_info("%s: Water Detect\n", __func__);
				} else {
					val->intval = 0;
				}
				break;
			default:
				return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_wireless_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = is_wireless_all_type(battery->cable_type) ? 1 : 0;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = (battery->pdata->wireless_charger_name) ?
			1 : 0;
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_POWER_DESIGN:
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
			if (battery->cable_type == SEC_BATTERY_CABLE_PREPARE_WIRELESS_20)
				val->intval = battery->wc20_power_class;
			else
#endif
				val->intval = 0;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR:
			val->intval = battery->tx_event;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_RETRY_CASE:
			val->intval = battery->tx_retry_case;
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_WC_STATUS:
			val->intval = battery->wc_status;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_wireless_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	union power_supply_propval value = {0, };
#endif

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (val->intval != SEC_BATTERY_CABLE_NONE && battery->wc_status == SEC_BATTERY_CABLE_NONE) {
			battery->cisd.data[CISD_DATA_WIRELESS_COUNT]++;
			battery->cisd.data[CISD_DATA_WIRELESS_COUNT_PER_DAY]++;
		}
		pr_info("%s : wireless_type(%s)\n", __func__, sb_get_ct_str(val->intval));

		/* Clear the FOD , AUTH State */
		sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_WIRELESS_FOD);

		battery->wc_status = val->intval;

#if !defined(CONFIG_WIRELESS_RX_PHM_CTRL)
		if ((battery->ext_event & BATT_EXT_EVENT_CALL) &&
			(battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_PACK ||
			battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_HV_PACK ||
			battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_TX)) {
				battery->wc_rx_phm_mode = true;
		}
#endif

		if (battery->wc_status == SEC_BATTERY_CABLE_NONE) {
			if (!battery->is_otg_on)
				battery->wpc_vout_level = WIRELESS_VOUT_5_5V;
			battery->wpc_max_vout_level = WIRELESS_VOUT_12_5V;
			battery->auto_mode = false;
			sec_bat_set_misc_event(battery, 0,
			(BATT_MISC_EVENT_WIRELESS_DET_LEVEL | /* clear wpc_det level status */
			BATT_MISC_EVENT_WIRELESS_AUTH_START |
			BATT_MISC_EVENT_WIRELESS_AUTH_RECVED |
			BATT_MISC_EVENT_WIRELESS_AUTH_FAIL |
			BATT_MISC_EVENT_WIRELESS_AUTH_PASS |
			BATT_MISC_EVENT_WC_JIG_PAD));
			change_sec_voter_pri(battery->input_vote, VOTER_WPC_CUR, VOTE_PRI_0);
		} else if (!is_wireless_fake_type(battery->wc_status)) {
			sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_DET_LEVEL, /* set wpc_det level status */
			BATT_MISC_EVENT_WIRELESS_DET_LEVEL);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
			if (battery->wc_status == SEC_BATTERY_CABLE_HV_WIRELESS_20) {
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_PASS,
					BATT_MISC_EVENT_WIRELESS_AUTH_PASS);
				if (battery->wc_status == SEC_BATTERY_CABLE_HV_WIRELESS_20)
					battery->cisd.cable_data[CISD_CABLE_HV_WC_20]++;
			} else if (battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_MPP) {
				change_sec_voter_pri(battery->input_vote, VOTER_WPC_CUR, VOTE_PRI_1);
				sec_vote(battery->input_vote, VOTER_WPC_CUR, true,
					battery->pdata->charging_current[battery->wc_status].input_current_limit);
				sec_vote(battery->input_vote, VOTER_CABLE, false, 0);
				value.intval = battery->pdata->charging_current[battery->wc_status].input_current_limit;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					POWER_SUPPLY_EXT_PROP_WIRELESS_MPP_PWR, value);
			}
#endif
		}

		__pm_stay_awake(battery->cable_ws);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->cable_work, 0);

		if (battery->wc_status == SEC_BATTERY_CABLE_NONE ||
			battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_PACK ||
			battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_HV_PACK ||
			battery->wc_status == SEC_BATTERY_CABLE_WIRELESS_VEHICLE) {
			sec_bat_set_misc_event(battery,
				(battery->wc_status == SEC_BATTERY_CABLE_NONE ?
				0 : BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE),
				BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE);
		}
		break;
	case POWER_SUPPLY_PROP_AUTHENTIC:
		pr_info("%s : tx_type(0x%x)\n", __func__, val->intval);
		count_cisd_pad_data(&battery->cisd, val->intval);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		sec_vote(battery->input_vote, VOTER_AICL, false, 0);
		pr_info("%s: reset aicl\n", __func__);
		break;
	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
		case POWER_SUPPLY_EXT_PROP_WIRELESS_ERR:
			if (is_wireless_type(battery->cable_type))
				sec_bat_set_misc_event(battery, val->intval ? BATT_MISC_EVENT_WIRELESS_FOD : 0,
					BATT_MISC_EVENT_WIRELESS_FOD);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR:
			if (val->intval & BATT_TX_EVENT_WIRELESS_TX_MISALIGN) {
				if (battery->tx_event & BATT_TX_EVENT_WIRELESS_TX_RETRY)
					sec_bat_handle_tx_misalign(battery, false);
				else
					sec_bat_handle_tx_misalign(battery, true);
			} else if (val->intval & BATT_TX_EVENT_WIRELESS_TX_OCP) {
				if (battery->tx_event & BATT_TX_EVENT_WIRELESS_TX_RETRY)
					sec_bat_handle_tx_ocp(battery, false);
				else
					sec_bat_handle_tx_ocp(battery, true);
			} else if (val->intval & BATT_TX_EVENT_WIRELESS_TX_AC_MISSING) {
				if (battery->wc_tx_enable)
					sec_wireless_set_tx_enable(battery, false);
				battery->tx_retry_case |= SEC_BAT_TX_RETRY_AC_MISSING;
				/* clear tx all event */
				sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
				sec_bat_set_tx_event(battery,
						BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
			} else if (val->intval & BATT_TX_EVENT_WIRELESS_TX_RETRY) {
				sec_wireless_set_tx_enable(battery, false);
				/* clear tx all event */
				sec_bat_set_tx_event(battery, 0, BATT_TX_EVENT_WIRELESS_ALL_MASK);
				sec_bat_set_tx_event(battery,
						BATT_TX_EVENT_WIRELESS_TX_RETRY, BATT_TX_EVENT_WIRELESS_TX_RETRY);
			} else {
				sec_wireless_set_tx_enable(battery, false);
				sec_bat_set_tx_event(battery, val->intval, val->intval);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_CONNECTED:
			sec_bat_set_tx_event(battery, val->intval ? BATT_TX_EVENT_WIRELESS_RX_CONNECT : 0,
				BATT_TX_EVENT_WIRELESS_RX_CONNECT);
			battery->wc_rx_connected = val->intval;
			battery->wc_tx_phm_mode = false;
			battery->prev_tx_phm_mode = false;

			if (!val->intval) {
				battery->wc_rx_type = NO_DEV;

				battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
				battery->tx_switch_mode_change = false;
				battery->tx_switch_start_soc = 0;

				sec_bat_run_wpc_tx_work(battery, 0);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_TYPE:
			if (val->intval &&
				battery->wc_rx_type != val->intval) {
				if (val->intval == SS_BUDS)
					battery->cisd.tx_data[SS_PHONE]--;
				battery->cisd.tx_data[val->intval]++;
			}
			battery->wc_rx_type = val->intval;
			pr_info("@Tx_Mode %s : RX_TYPE=%d\n", __func__, battery->wc_rx_type);
			sec_bat_run_wpc_tx_work(battery, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_AUTH_ADT_STATUS:
			if (val->intval == WIRELESS_AUTH_START)
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_START, BATT_MISC_EVENT_WIRELESS_AUTH_START);
			else if (val->intval == WIRELESS_AUTH_RECEIVED)
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_RECVED, BATT_MISC_EVENT_WIRELESS_AUTH_RECVED);
			else if (val->intval == WIRELESS_AUTH_SENT) /* need to be clear this value when data is sent */
				sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_WIRELESS_AUTH_START | BATT_MISC_EVENT_WIRELESS_AUTH_RECVED);
			else if (val->intval == WIRELESS_AUTH_FAIL)
				sec_bat_set_misc_event(battery, BATT_MISC_EVENT_WIRELESS_AUTH_FAIL, BATT_MISC_EVENT_WIRELESS_AUTH_FAIL);
			break;
		case POWER_SUPPLY_EXT_PROP_CALL_EVENT:
#if !defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			if (val->intval == 1) {
				pr_info("%s : PHM enabled\n", __func__);
				battery->wc_rx_phm_mode = true;
			}
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_RX_PHM:
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
			pr_info("%s : PHM %d\n", __func__, val->intval);
			battery->wc_rx_pdetb_mode = val->intval;
#endif
			break;
		case POWER_SUPPLY_EXT_PROP_GEAR_PHM_EVENT:
			battery->wc_tx_phm_mode = val->intval;
			pr_info("@Tx_Mode %s : tx phm status(%d)\n", __func__, val->intval);
			sec_bat_run_wpc_tx_work(battery, 0);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_RX_POWER:
			pr_info("@MPP @EPP %s : rx power %d\n", __func__, val->intval);
			battery->wc20_rx_power = val->intval;

			__pm_stay_awake(battery->wc20_current_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->wc20_current_work,
				msecs_to_jiffies(0));
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_MAX_VOUT:
		{
			unsigned int vout;

			pr_info("@MPP @EPP %s: max vout(%s)\n", __func__, sb_vout_ctr_mode_str(val->intval));

			vout = get_wc_vout_mW(val->intval);
			if (vout) {
				if (battery->current_event & SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK)
					battery->wpc_vout_level = WIRELESS_VOUT_5V;
				else
					battery->wpc_vout_level = val->intval;
				battery->wpc_max_vout_level = val->intval;
				battery->wc20_vout = vout;
			}
		}
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_OTG_CONTROL:
			sec_wireless_otg_vout_control(battery, val->intval);
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGE_OTG_ICL_CONTROL:
			sec_wireless_otg_icl_control(battery);
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_JIG_PAD:
			if (sec_bat_get_lpmode()) {
				sec_bat_set_misc_event(battery,
					BATT_MISC_EVENT_WC_JIG_PAD, BATT_MISC_EVENT_WC_JIG_PAD);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_WIRELESS_MPP_PWR:
			pr_info("@MPP %s: update default current(%d)\n", __func__, val->intval);
			sec_bat_change_default_current(battery, SEC_BATTERY_CABLE_WIRELESS_MPP,
				val->intval, battery->pdata->default_mpp_charging_current);
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_CLOAK:
			pr_info("@MPP %s: set MPP_CLOAK(%d)\n", __func__, val->intval);
			value.intval = val->intval;

			if (val->intval > CLOAK_EXIT_CMD) { // Cloak Enable.
				sec_vote(battery->input_vote, VOTER_WPC_CUR, true, 400);
				msleep(100);
				sec_bat_mfc_ldo_cntl(battery, MFC_LDO_OFF);
				value.intval = SEC_BAT_CHG_MODE_UNO_ON;
				psy_do_property("otg", set,
						POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
				value.intval = val->intval;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					psp, value);
			} else if (val->intval == CLOAK_EXIT_CMD) {
				pr_info("@MPP %s: send cmd to exit CLOAK\n", __func__);
				sec_vote(battery->input_vote, VOTER_WPC_CUR, true,
						battery->pdata->default_mpp_input_current);
				value.intval = CLOAK_EXIT_CMD;
				psy_do_property(battery->pdata->wireless_charger_name, set,
					psp, value);
			} else { // Cloak Disalbe - off
				sec_vote(battery->input_vote, VOTER_WPC_CUR, true,
						battery->pdata->default_mpp_input_current);
				psy_do_property(battery->pdata->wireless_charger_name, set,
					psp, value);
				value.intval = 0;
				psy_do_property("otg", set,
						POWER_SUPPLY_EXT_PROP_CHARGE_UNO_CONTROL, value);
				sec_bat_mfc_ldo_cntl(battery, MFC_LDO_ON);
			}
			break;
		case POWER_SUPPLY_EXT_PROP_MPP_ICL_CTRL:
		{
			int icl = get_sec_vote_result(battery->input_vote);
			int step = 50;

			if (val->intval) {
#if defined(CONFIG_SEC_FACTORY)
				step = 210;
#endif
				int max_icl =
					(battery->pdata->charging_current[battery->cable_type].input_current_limit - battery->wpc_temp_v2_offset);

				icl = (icl + step) > max_icl ? max_icl : (icl + step);
				pr_info("@MPP %s: Increase ICL(%d)\n", __func__, icl);
			} else {
				int prev_icl = icl;

				sb_check_mpla_thr_cnt(battery, prev_icl);

				icl = (icl - step) < 100 ? 100 : (icl - step);
				if ((prev_icl >= 100) && (icl <= 100)) {
					icl = 100;
				}
				pr_info("@MPP %s: Decrease ICL(%d)\n", __func__, icl);
			}
			sec_vote(battery->input_vote, VOTER_WPC_CUR, true, icl);
		}
			break;
#endif
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void sec_wireless_test_print_sgf_data(const char *buf, int count)
{
	char temp_buf[1024] = {0,};
	int i, size = 0;

	snprintf(temp_buf, sizeof(temp_buf), "0x%x", buf[0]);
	size = sizeof(temp_buf) - strlen(temp_buf);

	for (i = 1; i < count; i++) {
		snprintf(temp_buf+strlen(temp_buf), size, " 0x%x", buf[i]);
		size = sizeof(temp_buf) - strlen(temp_buf);
	}
	pr_info("%s: %s\n", __func__, temp_buf);
}

static ssize_t sec_wireless_sgf_show_attr(struct device *dev,
	struct device_attribute *attr, char *buf)
{
/*
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
*/
	return -EINVAL;
}

static ssize_t sec_wireless_sgf_store_attr(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0, };

	if (count < 4) {
		pr_err("%s: invalid data\n", __func__);
		return -EINVAL;
	}

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ID, value);
	pr_info("%s!!!(cable_type = %d, tx_id = %d, count = %ld)\n",
		__func__, battery->cable_type, value.intval, count);
	sec_wireless_test_print_sgf_data(buf, (int)count);

	if (is_wireless_type(battery->cable_type) && value.intval == 0xEF) {
		value.strval = buf;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_EXT_PROP_WIRELESS_SGF, value);
	}

	return count;
}

static DEVICE_ATTR(sgf, 0664, sec_wireless_sgf_show_attr, sec_wireless_sgf_store_attr);

static int sec_pogo_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
#if defined(CONFIG_USE_POGO)
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
#endif

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	val->intval = 0;
#if defined(CONFIG_USE_POGO)
	val->intval = battery->pogo_status;
	pr_info("%s: POGO online : %d\n", __func__, val->intval);
#endif

	return 0;
}

static int sec_pogo_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
#if defined(CONFIG_USE_POGO)
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
#endif

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
#if defined(CONFIG_USE_POGO)
		battery->pogo_status = (val->intval) ? 1 : 0;
		battery->pogo_9v = (val->intval > 1) ? true : false;

		if (battery->pogo_status && battery->pdata->pogo_chgin) {
			battery->muic_cable_type = ATTACHED_DEV_POGO_DOCK_MUIC;
			sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE);
		}

		__pm_stay_awake(battery->cable_ws);
		queue_delayed_work(battery->monitor_wqueue,
			&battery->cable_work, 0);
		pr_info("%s: pogo_status : %d\n", __func__, battery->pogo_status);
#endif
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sec_otg_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0,};
	int ret = 0;

	value.intval = val->intval;
	ret = psy_do_property(battery->pdata->otg_name, get, psp, value);
	val->intval = value.intval;

	return ret;
}

static int sec_otg_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	union power_supply_propval value = {0,};
	int ret = 0;

	value.intval = val->intval;

	if ((int)psp == POWER_SUPPLY_PROP_ONLINE) {
		battery->is_otg_on = val->intval;
		if (value.intval && !battery->otg_check) {
			battery->cisd.event_data[EVENT_OTG]++;
			battery->otg_check = true;
		} else if (!value.intval)
			battery->otg_check = false;
	}

	ret = psy_do_property(battery->pdata->otg_name, set, psp, value);
	return ret;
}

static void sec_otg_external_power_changed(struct power_supply *psy)
{
	power_supply_changed(psy);
}

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER) || IS_ENABLED(CONFIG_MUIC_NOTIFIER)
__visible_for_testing int sec_bat_cable_check(struct sec_battery_info *battery,
				muic_attached_dev_t attached_dev)
{
	int current_cable_type = -1;
	union power_supply_propval val = {0, };
#if IS_ENABLED(CONFIG_SBP_FG)
	int batt_present = 1;
	int ret;

	ret = psy_do_property(battery->pdata->charger_name, get, POWER_SUPPLY_PROP_PRESENT, val);
	if (ret >= 0) {
		batt_present = val.intval;
		pr_info("%s: batt_present(%d)\n", __func__, batt_present);
	}
#endif

	pr_info("[%s]ATTACHED(%d)\n", __func__, attached_dev);

	switch (attached_dev)
	{
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		battery->is_jig_on = true;
		battery->skip_cisd = true;
		current_cable_type = SEC_BATTERY_CABLE_NONE;
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
		val.intval = attached_dev;
		psy_do_property(battery->pdata->fuelgauge_name, set,
		POWER_SUPPLY_EXT_PROP_523K_JIG, val);
#endif
		break;
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
		battery->is_jig_on = true;
		battery->skip_cisd = true;
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_SMARTDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_MUIC:
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_UNDEFINED_CHARGING_MUIC:
	case ATTACHED_DEV_UNDEFINED_RANGE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_HICCUP_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_HMT_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_OTG;
		break;
	case ATTACHED_DEV_TIMEOUT_OPEN_MUIC:
	case ATTACHED_DEV_RETRY_TIMEOUT_OPEN_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_TIMEOUT;
		break;
	case ATTACHED_DEV_USB_MUIC:
	case ATTACHED_DEV_SMARTDOCK_USB_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_USB_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_USB;
		break;
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
#if defined(CONFIG_LSI_IFPMIC)
		current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
#endif
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_UARTOFF;
		if (sec_bat_get_facmode())
			current_cable_type = SEC_BATTERY_CABLE_NONE;
		break;
	case ATTACHED_DEV_RDU_TA_MUIC:
		battery->store_mode = true;
		__pm_stay_awake(battery->parse_mode_dt_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->parse_mode_dt_work, 0);
		current_cable_type = SEC_BATTERY_CABLE_TA;
		break;
	case ATTACHED_DEV_TA_MUIC:
#if defined(CONFIG_BC12_DEVICE)
		if (battery->pdata->bc12_ifcon_wa && !battery->sink_status.rp_currentlvl) {
			pr_err("%s: IFCON_WA, wait rp_currentlvl\n", __func__);
			sec_vote(battery->input_vote, VOTER_IFCON_WA, true,
					battery->pdata->default_usb_input_current);
		}
#endif
	case ATTACHED_DEV_CARDOCK_MUIC:
	case ATTACHED_DEV_DESKDOCK_VB_MUIC:
	case ATTACHED_DEV_SMARTDOCK_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_TA_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_ANY_MUIC:
	case ATTACHED_DEV_UNSUPPORTED_ID_VB_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_DISABLED_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_TA;
		break;
	case ATTACHED_DEV_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_RETRY_AFC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_QC_CHARGER_5V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_5V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT;
		break;
	case ATTACHED_DEV_CDP_MUIC:
	case ATTACHED_DEV_UNOFFICIAL_ID_CDP_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_USB_CDP;
		break;
	case ATTACHED_DEV_USB_LANHUB_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_LAN_HUB;
		break;
	case ATTACHED_DEV_LO_TA_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_LO_TA;
		break;
	case ATTACHED_DEV_CHARGING_CABLE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_POWER_SHARING;
		break;
	case ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC:
	case ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_PREPARE_TA;
		break;
	case ATTACHED_DEV_QC_CHARGER_9V_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_TA;
		if ((battery->cable_type == SEC_BATTERY_CABLE_TA) ||
				(battery->cable_type == SEC_BATTERY_CABLE_NONE))
			battery->cisd.cable_data[CISD_CABLE_QC]++;
		break;
	case ATTACHED_DEV_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_RETRY_AFC_CHARGER_9V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_TA;
		if ((battery->cable_type == SEC_BATTERY_CABLE_TA) ||
				(battery->cable_type == SEC_BATTERY_CABLE_NONE))
			battery->cisd.cable_data[CISD_CABLE_AFC]++;
		break;
#if defined(CONFIG_MUIC_HV_12V)
	case ATTACHED_DEV_AFC_CHARGER_12V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_12V_TA;
		break;
#endif
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC:
	case ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC:
		battery->cisd.cable_data[CISD_CABLE_AFC_FAIL]++;
		break;
	case ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC:
		battery->cisd.cable_data[CISD_CABLE_QC_FAIL]++;
		break;
	case ATTACHED_DEV_HV_ID_ERR_UNDEFINED_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_UNSUPPORTED_MUIC:
	case ATTACHED_DEV_HV_ID_ERR_SUPPORTED_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_9V_UNKNOWN;
		break;
	case ATTACHED_DEV_VZW_INCOMPATIBLE_MUIC:
		current_cable_type = SEC_BATTERY_CABLE_UNKNOWN;
		break;
	default:
		pr_err("%s: invalid type for charger:%d\n",
			__func__, attached_dev);
		break;
	}

#if IS_ENABLED(CONFIG_SBP_FG)
	switch (attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
		if (batt_present == 0)
			psy_do_property(battery->pdata->fuelgauge_name, set, POWER_SUPPLY_EXT_PROP_FAKE_SOC, val);
		break;
	default:
		break;
	}
#endif

#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_LIMITER_S2ASL01) && defined(CONFIG_SEC_FACTORY)
	if (!sec_bat_check_by_gpio(battery)) {
		if (attached_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC ||
			attached_dev == ATTACHED_DEV_JIG_USB_ON_MUIC) {
			pr_info("%s No Main or Sub Battery, 301k or 523k with FACTORY\n", __func__);
			gpio_direction_output(battery->pdata->sub_bat_enb_gpio, 1);
		}
	} else {
		if (attached_dev == ATTACHED_DEV_JIG_UART_OFF_MUIC ||
			attached_dev == ATTACHED_DEV_JIG_UART_ON_MUIC ||
			attached_dev == ATTACHED_DEV_JIG_USB_ON_MUIC) {
			pr_info("%s 301k or 523k or 619k with FACTORY\n", __func__);
			gpio_direction_output(battery->pdata->sub_bat_enb_gpio, 0);
		}
	}
#endif

	if (battery->is_jig_on && !battery->pdata->support_fgsrc_change)
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);

#if defined(CONFIG_LSI_IFPMIC)
	switch (attached_dev) {
	case ATTACHED_DEV_JIG_USB_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
		val.intval = attached_dev;
		if (!battery->factory_mode_boot_on)
			factory_mode = 1;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
		pr_err("%s : FACTORY MODE TEST! (%d, %d)\n", __func__, val.intval,
			battery->factory_mode_boot_on);
		break;
#if IS_ENABLED(CONFIG_CHARGER_S2MF301)
	case ATTACHED_DEV_JIG_USB_OFF_MUIC:
		val.intval = attached_dev;
		if (!battery->factory_mode_boot_on)
			factory_mode = 0;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
		pr_err("%s : FACTORY MODE TEST! (%d, %d)\n", __func__, val.intval,
			battery->factory_mode_boot_on);
		break;
#endif
#if defined(CONFIG_SIDO_OVP)
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
#endif
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		val.intval = 0;
		if (!battery->factory_mode_boot_on)
			factory_mode = 0;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
		pr_err("%s : FACTORY MODE TEST! (%d, %d)\n", __func__, val.intval,
			battery->factory_mode_boot_on);
		break;
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_ENABLE_HW_FACTORY_MODE, val);
		pr_err("%s : HW FACTORY MODE ENABLE TEST! (%d)\n", __func__, val.intval);
		break;
	default:
		break;
	}
#endif

	return current_cable_type;
}
EXPORT_SYMBOL_KUNIT(sec_bat_cable_check);
#endif

#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
__visible_for_testing void sec_bat_set_rp_current(struct sec_battery_info *battery, int cable_type)
{
	int icl = 0;
	int fcc = 0;

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE) && defined(CONFIG_SEC_FACTORY)
	sec_bat_usb_factory_set_vote(battery, false);
#endif

	switch (battery->sink_status.rp_currentlvl) {
	case RP_CURRENT_ABNORMAL:
		icl = battery->pdata->rp_current_abnormal_rp3;
		fcc = battery->pdata->rp_current_abnormal_rp3;
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		break;
	case RP_CURRENT_LEVEL3:
#if defined(CONFIG_PD_CHARGER_HV_DISABLE)
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE) {
#else
		if (battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE) {
#endif
			icl = battery->pdata->default_input_current;
			fcc = battery->pdata->default_charging_current;
		} else {
			if (battery->store_mode) {
				icl = battery->pdata->rp_current_rdu_rp3;
				fcc = battery->pdata->max_charging_current;
			} else {
				icl = battery->pdata->rp_current_rp3;
				fcc = battery->pdata->max_charging_current;
			}
		}
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		break;
	case RP_CURRENT_LEVEL2:
		icl = battery->pdata->rp_current_rp2;
		fcc = battery->pdata->rp_current_rp2;
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		break;
	case RP_CURRENT_LEVEL_DEFAULT:
		if (cable_type == SEC_BATTERY_CABLE_USB) {
			if (battery->current_event & SEC_BAT_CURRENT_EVENT_USB_SUPER) {
				icl = USB_CURRENT_SUPER_SPEED;
				fcc = USB_CURRENT_SUPER_SPEED;
			} else {
				icl = battery->pdata->default_usb_input_current;
				fcc = battery->pdata->default_usb_charging_current;
			}
		} else if (cable_type == SEC_BATTERY_CABLE_TA) {
			icl = battery->pdata->default_input_current;
			fcc = battery->pdata->default_charging_current;
		} else {
			icl = battery->pdata->default_usb_input_current;
			fcc = battery->pdata->default_usb_charging_current;
			pr_err("%s: wrong cable type(%d)\n", __func__, cable_type);
		}
		break;
	default:
		icl = battery->pdata->default_usb_input_current;
		fcc = battery->pdata->default_usb_charging_current;
		pr_err("%s: undefined rp_currentlvl(%d)\n", __func__, battery->sink_status.rp_currentlvl);
		break;
	}

	sec_bat_change_default_current(battery, cable_type, icl, fcc);

	pr_info("%s:(%d)\n", __func__, battery->sink_status.rp_currentlvl);
	battery->max_charge_power = 0;
	sec_bat_check_input_voltage(battery, cable_type);

#if defined(CONFIG_BC12_DEVICE)
	if (battery->pdata->bc12_ifcon_wa) {
		if (battery->sink_status.rp_currentlvl)
			sec_vote(battery->input_vote, VOTER_IFCON_WA, false, 0);
		else
			pr_err("%s: IFCON_WA keeps 500mA ICL, rp_currentlvl none\n", __func__);
	}
#endif
	sec_vote(battery->input_vote, VOTER_AICL, false, 0);
}
EXPORT_SYMBOL_KUNIT(sec_bat_set_rp_current);

static int usb_typec_handle_id_attach(struct sec_battery_info *battery, PD_NOTI_ATTACH_TYPEDEF *pdata, int *cable_type, const char **cmd)
{
	struct pdic_notifier_struct *pd_noti = pdata->pd;
	SEC_PD_SINK_STATUS *psink_status = NULL;

	if (pd_noti)
		psink_status = &pd_noti->sink_status;

	switch (pdata->attach) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		*cmd = "DETACH";
		battery->is_jig_on = false;
		battery->pd_usb_attached = false;
		*cable_type = SEC_BATTERY_CABLE_NONE;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		battery->sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
		sec_bat_usb_factory_clear(battery);
#endif
		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		/* Skip notify from MUIC if PDIC is attached already */
		if (is_pd_wire_type(battery->wire_status) || battery->init_src_cap) {
			if (sec_bat_get_lpmode() ||
				(battery->usb_conn_status == USB_CONN_NORMAL &&
				!(battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE))) {
				return -1; /* skip usb_typec_handle_after_id() */
			}
		}
		*cmd = "ATTACH";
		battery->muic_cable_type = pdata->cable_type;
		*cable_type = sec_bat_cable_check(battery, battery->muic_cable_type);
		if (battery->cable_type != *cable_type &&
			battery->sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
			(*cable_type == SEC_BATTERY_CABLE_USB || *cable_type == SEC_BATTERY_CABLE_TA)) {
			sec_bat_set_rp_current(battery, *cable_type);
		} else if (psink_status &&
			pd_noti->event == PDIC_NOTIFY_EVENT_PDIC_ATTACH &&
			psink_status->rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
			(*cable_type == SEC_BATTERY_CABLE_USB || *cable_type == SEC_BATTERY_CABLE_TA)) {
			battery->sink_status.rp_currentlvl = psink_status->rp_currentlvl;
			sec_bat_set_rp_current(battery, *cable_type);
		}
		break;
	default:
		*cmd = "ERROR";
		*cable_type = -1;
		battery->muic_cable_type = pdata->cable_type;
		break;
	}
	battery->pdic_attach = false;
	battery->pdic_ps_rdy = false;
	battery->init_src_cap = false;
	if (battery->muic_cable_type == ATTACHED_DEV_QC_CHARGER_9V_MUIC ||
		battery->muic_cable_type == ATTACHED_DEV_QC_CHARGER_ERR_V_MUIC)
		battery->hv_chg_name = "QC";
	else if (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_MUIC ||
		battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_9V_DUPLI_MUIC ||
		battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_MUIC ||
		battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)
		battery->hv_chg_name = "AFC";
#if defined(CONFIG_MUIC_HV_12V)
	else if (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_MUIC ||
		battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC)
		battery->hv_chg_name = "12V";
#endif
	else
		battery->hv_chg_name = "NONE";

	dev_info(battery->dev, "%s: cable_type:%d, muic_cable_type:%d\n",
		__func__, *cable_type, battery->muic_cable_type);

	return 0;
}

static void sb_disable_reverse_boost(struct sec_battery_info *battery)
{
	union power_supply_propval value = { 0, };

	if (battery->pdata->d2d_check_type == SB_D2D_SNKONLY)
		return;

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VOLTAGE, value);
	battery->vpdo_src_boost = value.intval ? true : false;

	if (battery->vpdo_src_boost) {
		/* turn off dc reverse boost */
		value.intval = 0;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VOLTAGE, value);
	}
}

static int sb_set_dc_ta_op_max_mode(int pdo_type, int max_volt, int max_curr)
{
#if !IS_ENABLED(CONFIG_DIRECT_CHARGING)
	return DC_MODE_NONE;
#endif
	if (pdo_type != APDO_TYPE)
		return DC_MODE_NONE;

	max_volt /= 1000;

	if (max_volt >= DC_CHARGER_3TO1_MIN_VOLTAGE &&
		max_curr >= DC_CHARGER_MIN_CURRENT) {
		pr_info("%s: pdo_type:%d, max_volt:%d, max_curr:%d\n",
			__func__, pdo_type, max_volt, max_curr);
		return DC_MODE_3TO1;
	} else
		return DC_MODE_2TO1; /* Default ratio will be 2:1 */
}

static int sb_set_dc_ta_forced_op_max_mode(int pdo_type, int max_curr, int max_volt, unsigned short vid)
{
	if (!is_valid_2_1_forced_ta(vid))
		return DC_MODE_NONE;

	if (pdo_type == APDO_TYPE &&
		max_curr >= DC_CHARGER_3TO1_SKIP_CABLE_CURRENT &&
		max_volt >= DC_CHARGER_3TO1_SKIP_CABLE_VOLT) {
		pr_info("%s: pdo_type:%d, max_volt:%d, max_curr:%d, vid:%04x\n",
			__func__, pdo_type, max_volt, max_curr, vid);
		return DC_MODE_2TO1;
	}

	return DC_MODE_NONE;
}

static int usb_typec_handle_id_power_status(struct sec_battery_info *battery,
		PD_NOTI_POWER_STATUS_TYPEDEF *pdata, int *cable_type, const char **cmd)
{
	int pdata_fpdo_max_power = battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ?
		battery->pdata->nv_charge_power : battery->pdata->pd_charging_charge_power;
	int pdata_apdo_max_power = battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE ?
		battery->pdata->nv_charge_power : battery->pdata->max_charging_charge_power;
	int max_power = 0, apdo_max_power = 0, fpdo_max_power = 0;
	int i = 0, current_pdo = 0, num_pd_list = 0, dc_ta_op_max_mode = 0, voter_status = 0;
	bool bPdIndexChanged = false, bPrintPDlog = true;
	unsigned short vid = 0;
	union power_supply_propval value = {0, };
	struct pdic_notifier_struct *pd_noti = pdata->pd;
	SEC_PD_SINK_STATUS *psink_status = NULL;

	if (pd_noti)
		psink_status = &pd_noti->sink_status;

	if (!psink_status) {
		dev_err(battery->dev, "%s: sink_status is NULL\n", __func__);
		return -1; /* skip usb_typec_handle_after_id() */
	}

	dev_info(battery->dev, "%s: pd_event(%d)\n", __func__, pd_noti->event);

#if IS_ENABLED(CONFIG_HICCUP_CHARGER)
	if (pd_noti->event != PDIC_NOTIFY_EVENT_DETACH &&
		pd_noti->event != PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC) {
		if (!sec_bat_get_lpmode() && (battery->usb_conn_status ||
					(battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE))) {
			return 0;
		}
	}
#endif

	switch (pd_noti->event) {
	case PDIC_NOTIFY_EVENT_DETACH:
		dev_info(battery->dev, "%s: skip pd operation - attach(%d)\n", __func__, pdata->attach);
		battery->pdic_attach = false;
		battery->pdic_ps_rdy = false;
		battery->init_src_cap = false;
		battery->hv_pdo = false;
		battery->pd_list.now_isApdo = false;
		return -1; /* usb_typec_handle_after_id() */
		break;
	case PDIC_NOTIFY_EVENT_PDIC_ATTACH:
		battery->sink_status.rp_currentlvl = psink_status->rp_currentlvl;
		dev_info(battery->dev, "%s: battery->rp_currentlvl(%d)\n",
				__func__, battery->sink_status.rp_currentlvl);
		if (battery->pdata->support_usb_conn_check &&
			battery->wire_status == SEC_BATTERY_CABLE_NONE && !delayed_work_pending(&battery->usb_conn_check_work) &&
			!battery->run_usb_conn_check) {
#if !defined(CONFIG_SEC_FACTORY)
			battery->run_usb_conn_check = true;
			battery->usb_conn_check_cnt = 0;
			__pm_stay_awake(battery->usb_conn_check_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->usb_conn_check_work, msecs_to_jiffies(1000));
#endif
		} else if (battery->wire_status == SEC_BATTERY_CABLE_USB || battery->wire_status == SEC_BATTERY_CABLE_TA) {
			*cable_type = battery->wire_status;
			battery->chg_limit = false;
			battery->lrp_limit = false;
			battery->lrp_step = LRP_NONE;
			sec_bat_set_rp_current(battery, *cable_type);
			return 0;
		}
		return -1; /* skip usb_typec_handle_after_id() */
		break;
	case PDIC_NOTIFY_EVENT_PD_SOURCE:
	case PDIC_NOTIFY_EVENT_PD_SINK:
		break;
	case PDIC_NOTIFY_EVENT_PD_SINK_CAP:
		battery->update_pd_list = true;
		break;
	case PDIC_NOTIFY_EVENT_PD_PRSWAP_SNKTOSRC:
		*cmd = "PD_PRWAP";
		dev_info(battery->dev, "%s: PRSWAP_SNKTOSRC(%d)\n", __func__, pdata->attach);
		*cable_type = SEC_BATTERY_CABLE_NONE;

		battery->pdic_attach = false;
		battery->pdic_ps_rdy = false;
		battery->init_src_cap = false;
		battery->hv_pdo = false;

		if (battery->pdata->d2d_check_type == SB_D2D_NONE)
			return 0;

		/* for 15w d2d snk to src prswap */
		if (battery->pdata->d2d_check_type == SB_D2D_SNKONLY) {
			value.intval = 1;
			psy_do_property(battery->pdata->otg_name, set,
				POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		}

		if (battery->d2d_auth == D2D_AUTH_SNK)
			battery->d2d_auth = D2D_AUTH_SRC;
		return 0;
	case PDIC_NOTIFY_EVENT_PD_PRSWAP_SRCTOSNK:
		*cmd = "PD_PRWAP";
		dev_info(battery->dev, "%s: PRSWAP_SRCTOSNK(%d)\n", __func__, pdata->attach);

		if (battery->d2d_auth == D2D_AUTH_SRC) {
			if (battery->pdata->d2d_check_type == SB_D2D_SRCSNK) {
				sb_disable_reverse_boost(battery);
				battery->vpdo_ocp = false;
			}
			battery->d2d_auth = D2D_AUTH_SNK;

			value.intval = 0;
			psy_do_property(battery->pdata->otg_name, set,
				POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		}
		battery->vpdo_auth_stat = AUTH_NONE;
		battery->hp_d2d = HP_D2D_NONE;
		sec_vote(battery->chgen_vote, VOTER_D2D_WIRE, false, 0);
		return 0;
	default:
		break;
	}

	*cmd = "PD_ATTACH";
	battery->init_src_cap = false;

	if (battery->update_pd_list) {
		pr_info("%s : update_pd_list(%d)\n", __func__, battery->update_pd_list);
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
#if defined(CONFIG_STEP_CHARGING)
		sec_bat_reset_step_charging(battery);
		if (is_pd_apdo_wire_type(battery->cable_type))
			sec_bat_check_dc_step_charging(battery);
#endif
		value.intval = 0;
		psy_do_property(battery->pdata->charger_name, set,
				POWER_SUPPLY_EXT_PROP_DIRECT_CLEAR_ERR, value);
#endif
		battery->pdic_attach = false;
	}

	if (!battery->pdic_attach) {
		battery->sink_status = *psink_status;
		bPdIndexChanged = true;
	} else {
		dev_info(battery->dev, "%s: sel_pdo(%d -> %d), cur_pdo(%d -> %d)\n",
			__func__, battery->sink_status.selected_pdo_num, psink_status->selected_pdo_num,
			battery->sink_status.current_pdo_num, psink_status->current_pdo_num);

		if (battery->sink_status.current_pdo_num != psink_status->current_pdo_num)
			bPdIndexChanged = true;

		battery->sink_status.selected_pdo_num = psink_status->selected_pdo_num;
		battery->sink_status.current_pdo_num = psink_status->current_pdo_num;
		battery->pdic_ps_rdy = true;
		sec_bat_get_input_current_in_power_list(battery);
		sec_bat_get_charging_current_in_power_list(battery);
		sec_vote(battery->chgen_vote, VOTER_SRCCAP_TRANSIT, false, 0);
		if (delayed_work_pending(&battery->transit_clear_work)) {
			cancel_delayed_work(&battery->transit_clear_work);
			__pm_relax(battery->transit_clear_ws);
		}
		battery->srccap_transit = false;
		battery->srccap_transit_cnt = 0;
	}
	current_pdo = battery->sink_status.current_pdo_num;
	if (battery->sink_status.power_list[current_pdo].max_voltage > SEC_INPUT_VOLTAGE_5V)
		battery->hv_pdo = true;
	else
		battery->hv_pdo = false;
	dev_info(battery->dev, "%s: battery->pdic_ps_rdy(%d), hv_pdo(%d)\n",
			__func__, battery->pdic_ps_rdy, battery->hv_pdo);

	if (battery->sink_status.has_apdo) {
		*cable_type = SEC_BATTERY_CABLE_PDIC_APDO;
		if (battery->sink_status.power_list[current_pdo].pdo_type == APDO_TYPE) {
			battery->hv_chg_name = "PDIC_APDO";
			battery->pd_list.now_isApdo = true;
		} else {
			battery->hv_chg_name = "PDIC_FIXED";
			battery->pd_list.now_isApdo = false;
		}

		if (battery->pdic_attach)
			bPrintPDlog = false;
	} else {
		*cable_type = SEC_BATTERY_CABLE_PDIC;
		if (battery->sink_status.power_list[current_pdo].pdo_type == VPDO_TYPE) {
			battery->hv_chg_name = "PDIC_VPDO";
			if ((battery->pdata->d2d_check_type == SB_D2D_SRCSNK) &&
				(battery->d2d_auth == D2D_AUTH_SNK)) {
				/* preset auth vpdo for pr swap case (snk to src) */
				sec_pd_vpdo_auth(AUTH_HIGH_PWR, SB_D2D_SRCSNK);
			}
		} else
			battery->hv_chg_name = "PDIC_FIXED";
		battery->pd_list.now_isApdo = false;
	}
	battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
	battery->input_voltage = battery->sink_status.power_list[current_pdo].max_voltage;
	dev_info(battery->dev, "%s: available pdo : %d, current pdo : %d\n", __func__,
		battery->sink_status.available_pdo_num, current_pdo);

	for (i = 1; i <= battery->sink_status.available_pdo_num; i++) {
		bool isUpdated = false;
		int temp_power = 0;
		int temp_dc_ta_op_mode = 0;
		int temp_apdo_max_volt = 0;
		int pdo_type = battery->sink_status.power_list[i].pdo_type;
		int max_volt = battery->sink_status.power_list[i].max_voltage;
		int min_volt = battery->sink_status.power_list[i].min_voltage;
		int max_curr = battery->sink_status.power_list[i].max_current;
		bool isAccept = battery->sink_status.power_list[i].accept;

		if (get_sec_vote_result(battery->apdo_max_volt_vote) < 0) {
			dev_info(battery->dev, "%s: init apdo_max_volt_vote: %d\n",
				__func__, battery->pdata->apdo_max_volt);
			temp_apdo_max_volt = battery->pdata->apdo_max_volt;
			sec_vote(battery->apdo_max_volt_vote, VOTER_CABLE, true, temp_apdo_max_volt);
		} else
			temp_apdo_max_volt = get_sec_vote_result(battery->apdo_max_volt_vote);

		if ((pdo_type == APDO_TYPE) &&
			(sec_pd_get_apdo_prog_volt(pdo_type, max_volt) > temp_apdo_max_volt))
			temp_power = temp_apdo_max_volt * max_curr; /* Protect to show sfc2.0 with 45w ta + 3a cable */
		else
			temp_power = sec_pd_get_max_power(pdo_type, min_volt, max_volt, max_curr);

		if ((pdo_type != APDO_TYPE) && isAccept &&
			(max_volt > battery->pdata->max_input_voltage)) {
			battery->sink_status.power_list[i].accept = false;
			isAccept = battery->sink_status.power_list[i].accept;
		}

		if (!(battery->current_event & SEC_BAT_CURRENT_EVENT_HV_DISABLE) &&
				!battery->sink_status.has_apdo &&
				pdo_type == FPDO_TYPE &&
				battery->pdata->support_fpdo_dc &&
				max_volt == 9000 &&
				(max_curr >= 3000 || battery->is_fpdo_dc)) {
			dev_info(battery->dev, "%s: cable_type update to FPDO_DC\n", __func__);
			if (!battery->is_fpdo_dc)
				battery->cisd.cable_data[CISD_CABLE_FPDO_DC]++;
			battery->is_fpdo_dc = true;
			*cable_type = SEC_BATTERY_CABLE_FPDO_DC;
			pdata_fpdo_max_power = battery->pdata->fpdo_dc_charge_power;

			pr_info("%s: FPDO_DC, refresh charging source.\n", __func__);
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_REFRESH_CHARGING_SOURCE, value);
#if defined(CONFIG_STEP_CHARGING)
			sec_bat_check_step_charging(battery);
#endif
		}

		if (bPrintPDlog)
			pr_info("%s:%spower_list[%d,%s,%s], maxVol:%d, minVol:%d, maxCur:%d, power:%d\n",
				__func__, i == current_pdo ? "**" : "  ",
				i, sec_pd_pdo_type_str(pdo_type), isAccept ? "O" : "X",
				max_volt, min_volt, max_curr, temp_power);

		if (!battery->pdic_attach && isAccept) {
			if (pdo_type == APDO_TYPE) {
				if (temp_power > apdo_max_power) {
					max_power = temp_power;
					apdo_max_power = max_power;
				}
			} else {
				if (temp_power > fpdo_max_power) {
					max_power = temp_power;
					fpdo_max_power = max_power;
				}
			}
		}

		if (!isAccept)
			continue;

		if (is_dc_higher_ratio_support()) {
			sec_pd_get_vid(&vid);
			temp_dc_ta_op_mode = battery->dc_ta_op_max_mode;
			battery->dc_ta_forced_op_max_mode =
				max(sb_set_dc_ta_forced_op_max_mode(pdo_type, max_curr, max_volt, vid),
					battery->dc_ta_forced_op_max_mode);
			battery->dc_ta_op_max_mode =
				max(sb_set_dc_ta_op_max_mode(pdo_type, max_volt, max_curr), temp_dc_ta_op_mode);
			if (battery->dc_ta_forced_op_max_mode &&
				battery->dc_ta_op_max_mode > battery->dc_ta_forced_op_max_mode) {
				dc_ta_op_max_mode = battery->dc_ta_forced_op_max_mode;
			}
		}

		/* no change apdo */
		if (pdo_type == APDO_TYPE) {
			num_pd_list++;
			continue;
		}

		if (temp_power > (pdata_fpdo_max_power * 1000)) {
			fpdo_max_power = pdata_fpdo_max_power * 1000;
			max_curr = mA_by_mWmV(pdata_fpdo_max_power, max_volt);
			isUpdated = true;
		}
		if (max_curr > battery->pdata->max_input_current) {
			max_curr = battery->pdata->max_input_current;
			isUpdated = true;
		}

		if (isUpdated) {
			battery->sink_status.power_list[i].max_current = max_curr;
			if (bPrintPDlog)
				pr_info("%s:->updated [%d,%s,%s], maxVol:%d, minVol:%d, maxCur:%d, power:%d\n",
					__func__, i, sec_pd_pdo_type_str(pdo_type),
					isAccept ? "O" : "X", battery->sink_status.power_list[i].max_voltage,
					battery->sink_status.power_list[i].min_voltage,
					battery->sink_status.power_list[i].max_current,
					sec_pd_get_max_power(pdo_type, min_volt, max_volt, max_curr));
		}
		num_pd_list++;
	}

	if (!battery->pdic_attach) {
		if (battery->sink_status.has_apdo)
			battery->max_charge_power = (apdo_max_power / 1000) > pdata_apdo_max_power ?
					pdata_apdo_max_power : (apdo_max_power / 1000);
		else
			battery->max_charge_power = (fpdo_max_power / 1000) > pdata_fpdo_max_power ?
					pdata_fpdo_max_power : (fpdo_max_power / 1000);
		battery->pd_max_charge_power = battery->max_charge_power;
		pr_info("%s: pd_max_charge_power(%d,%d,%d) = %d\n",
			__func__, max_power, apdo_max_power, fpdo_max_power, battery->pd_max_charge_power * 1000);
		battery->pd_rated_power = max_power / 1000 / 1000; /* save as W */
		count_cisd_power_data(&battery->cisd, max_power / 1000);
		if (is_dc_higher_ratio_support()) {
			pr_info("%s: TA supports up to %d:1 mode\n", __func__, battery->dc_ta_op_max_mode);
			if (battery->sink_status.has_apdo) {
				sec_vote(battery->apdo_max_volt_vote, VOTER_CABLE, true,
					battery->pdata->apdo_max_volt_arr[battery->dc_ta_op_max_mode]);
				sec_vote(battery->dc_op_mode_vote, VOTER_CABLE, true, battery->dc_ta_op_max_mode);
			}
		}

		if (battery->cable_type == SEC_BATTERY_CABLE_NONE) {
			if (battery->pd_max_charge_power > HV_CHARGER_STATUS_STANDARD1)
				battery->cisd.cable_data[CISD_CABLE_PD_HIGH]++;
			else
				battery->cisd.cable_data[CISD_CABLE_PD]++;
		}
#if defined(CONFIG_SUPPORT_HV_CTRL) && !defined(CONFIG_SEC_FACTORY)
		if (battery->pdata->boosting_voltage_aicl &&
			(battery->misc_event & BATT_MISC_EVENT_HV_BY_AICL)) {
			if (sb_ct_has_9v(battery, battery->cable_type)) {
				sec_vote(battery->input_vote, VOTER_AICL, false, 0);
				sec_vote(battery->iv_vote, VOTER_AICL, true, SEC_INPUT_VOLTAGE_9V);
			} else {
				sec_vote(battery->iv_vote, VOTER_AICL, false, 0);
			}
		}
#endif
		battery->pd_list.max_pd_count = num_pd_list;
		pr_info("%s: total num_pd_list: %d\n", __func__, battery->pd_list.max_pd_count);
		if (battery->pd_list.max_pd_count <= 0
			|| battery->pd_list.max_pd_count > MAX_PDO_NUM)
			return -1; /* skip usb_typec_handle_after_id() */
	} else if (is_dc_higher_ratio_support() && battery->dc_ta_forced_op_max_mode &&
			dc_ta_op_max_mode != get_sec_vote_result(battery->dc_op_mode_vote) &&
			get_sec_vote_result(battery->dc_op_mode_vote) > 0 &&
			get_sec_voter_status(battery->dc_op_mode_vote, VOTER_DC_OP_MODE_SYSFS, &voter_status) < 0) {
		pr_info("%s: Ratio updated to %d:1 due to VID: %04x\n",
			__func__, battery->dc_ta_forced_op_max_mode, vid);
		/* To prevent current jump during ratio change */
		sec_vote(battery->fcc_vote, VOTER_DC_OP_RATIO_CHG, true, 3001);
		sec_vote(battery->apdo_max_volt_vote, VOTER_CABLE, true,
						battery->pdata->apdo_max_volt_arr[battery->dc_ta_forced_op_max_mode]);
		sec_vote(battery->dc_op_mode_vote, VOTER_CABLE, true, battery->dc_ta_forced_op_max_mode);
		sec_vote(battery->fcc_vote, VOTER_DC_OP_RATIO_CHG, false, 0);
	}
	battery->pdic_attach = true;
#if defined(CONFIG_BC12_DEVICE)
	battery->pdic_ps_rdy = true;
#endif
	if (is_pd_wire_type(battery->cable_type) && bPdIndexChanged) {
		if (battery->update_pd_list || ((battery->d2d_auth == D2D_AUTH_SNK) &&
			(battery->sink_status.power_list[current_pdo].pdo_type == FPDO_TYPE)) ||
			(battery->sink_status.has_apdo && (battery->current_event & SEC_BAT_CURRENT_EVENT_DC_ERR)) ||
			(!battery->sink_status.has_apdo && (battery->sink_status.selected_pdo_num == -1))) {
			int curr_volt = battery->sink_status.power_list[current_pdo].max_voltage;
			int vote_volt = get_sec_vote_result(battery->iv_vote);

			/* request vpdo for 15w d2d */
			sec_vote(battery->iv_vote, VOTER_CABLE, true,
				sec_bat_check_pd_iv(&battery->sink_status));

			/* current pdo voltage and voter voltage are different, 9v should be select */
			if (battery->update_pd_list || ((curr_volt != vote_volt) &&
				(sec_bat_check_pd_iv(&battery->sink_status) == vote_volt))) {
				pr_info("%s: update_pd_list: %d\n", __func__, battery->update_pd_list);
				sec_vote_refresh(battery->iv_vote);
			}

			battery->update_pd_list = false;
		}

		/* Check VOTER_SIOP to set up current based on chagned index */
		__pm_stay_awake(battery->siop_level_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);

#if !defined(CONFIG_SEC_FACTORY)
		sec_bat_check_lrp_temp(battery,
			battery->cable_type, battery->wire_status, battery->siop_level, battery->lcd_status);
#endif
	}

	if (is_pd_apdo_wire_type(battery->wire_status) && !bPdIndexChanged &&
		(battery->sink_status.power_list[current_pdo].pdo_type == APDO_TYPE)) {
		battery->wire_status = *cable_type;
		return -1; /* skip usb_typec_handle_after_id() */
	}

	store_battery_log("CURR_PDO:PDO(%d/%d),Max_V(%d),Min_V(%d),Max_C(%d),PD_MCP(%d),PD_RP(%d),",
				battery->sink_status.current_pdo_num,
				battery->sink_status.available_pdo_num,
				battery->sink_status.power_list[current_pdo].max_voltage,
				battery->sink_status.power_list[current_pdo].min_voltage,
				battery->sink_status.power_list[current_pdo].max_current,
				battery->pd_max_charge_power,
				battery->pd_rated_power);

	return 0;
}

static int usb_typec_handle_id_usb(struct sec_battery_info *battery, PD_NOTI_ATTACH_TYPEDEF *pdata)
{
	if (pdata->cable_type == PD_USB_TYPE)
		battery->pd_usb_attached = true;
	else if (pdata->cable_type == PD_NONE_TYPE)
		battery->pd_usb_attached = false;
	dev_info(battery->dev, "%s: PDIC_NOTIFY_ID_USB: %d\n", __func__, battery->pd_usb_attached);
	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);

	return -1; /* skip usb_typec_handle_after_id() */
}

static void sb_auth_detach(struct sec_battery_info *battery)
{
	union power_supply_propval value = { 0, };

	battery->d2d_auth = D2D_AUTH_NONE;
	battery->vpdo_ocp = false;
	battery->vpdo_auth_stat = AUTH_NONE;
	battery->hp_d2d = HP_D2D_NONE;
	/* clear vpdo auth for snk to src(pr swap) with detach case */
	sec_pd_vpdo_auth(AUTH_NONE, battery->pdata->d2d_check_type);

	sb_disable_reverse_boost(battery);

	/* set otg ocp limit 0.9A */
	value.intval = 0;
	psy_do_property(battery->pdata->otg_name, set,
			POWER_SUPPLY_PROP_VOLTAGE_MAX, value);

	/* clear buck off vote */
	sec_vote(battery->chgen_vote, VOTER_D2D_WIRE, false, 0);
}

static int usb_typec_handle_id_device_info(struct sec_battery_info *battery,
	PD_NOTI_DEVICE_INFO_TYPEDEF *pdata)
{
	union power_supply_propval value = { 0, };

	if ((pdata->vendor_id != AUTH_VENDOR_ID) ||
		(pdata->product_id != AUTH_PRODUCT_ID) ||
		(pdata->version <= 0)) {
		pr_info("%s : skip id_device_info\n", __func__);
		return -1;
	}

	if (battery->pdata->d2d_check_type != SB_D2D_NONE) {
		if (battery->pdata->d2d_check_type == SB_D2D_SNKONLY) {
			/* set otg ocp limit 1.5A */
			value.intval = 1;
			psy_do_property(battery->pdata->otg_name, set,
				POWER_SUPPLY_PROP_VOLTAGE_MAX, value);
		}

		battery->d2d_auth = D2D_AUTH_SRC;

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
		sec_bat_d2d_check(battery,
			battery->capacity, battery->temperature, battery->lrp);
#endif
	} else
		battery->d2d_auth = D2D_AUTH_SNK;

	pr_info("%s set d2d auth %s : (0x%04x, 0x%04x, %d)\n", __func__,
		((battery->d2d_auth == D2D_AUTH_SRC) ? "src" : "snk"),
		pdata->vendor_id, pdata->product_id, pdata->version);

	return -1;
}

static int usb_typec_handle_id_svid_info(struct sec_battery_info *battery,
	PD_NOTI_SVID_INFO_TYPEDEF *pdata)
{
	if (pdata->standard_vendor_id == AUTH_VENDOR_ID)
		battery->d2d_auth = D2D_AUTH_SNK;

	pr_info("%s set vpdo snk auth : %s (0x%04x)\n", __func__,
		(battery->d2d_auth == D2D_AUTH_SNK) ? "enable" : "disable", pdata->standard_vendor_id);

	return -1;
}

static int usb_typec_handle_id_clear_info(struct sec_battery_info *battery,
	PD_NOTI_CLEAR_INFO_TYPEDEF *pdata)
{
	if ((pdata->clear_id == PDIC_NOTIFY_ID_DEVICE_INFO) ||
		(pdata->clear_id == PDIC_NOTIFY_ID_SVID_INFO)) {
		sb_auth_detach(battery);
	}

	pr_info("%s clear vpdo auth : (%d)\n", __func__,
		pdata->clear_id);

	return -1;
}

static int usb_typec_handle_after_id(struct sec_battery_info *battery, int cable_type, const char *cmd)
{
#if defined(CONFIG_WIRELESS_TX_MODE)
	if (!is_hv_wire_type(cable_type) &&
		!is_hv_pdo_wire_type(cable_type, battery->hv_pdo)) {
		sec_vote(battery->chgen_vote, VOTER_CHANGE_CHGMODE, false, 0);
		sec_vote(battery->iv_vote, VOTER_CHANGE_CHGMODE, false, 0);
	}
#endif

#if defined(CONFIG_PD_CHARGER_HV_DISABLE) && !defined(CONFIG_SEC_FACTORY)
	if (check_afc_disabled_type(battery->muic_cable_type)) {
		pr_info("%s set SEC_BAT_CURRENT_EVENT_AFC_DISABLE\n", __func__);
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_AFC_DISABLE, SEC_BAT_CURRENT_EVENT_AFC_DISABLE);
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue,
					&battery->monitor_work, 0);
	} else {
		sec_bat_set_current_event(battery,
			0, SEC_BAT_CURRENT_EVENT_AFC_DISABLE);
	}
#endif
	sec_bat_set_misc_event(battery,
		(battery->muic_cable_type == ATTACHED_DEV_UNDEFINED_CHARGING_MUIC ?
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) |
		(battery->muic_cable_type == ATTACHED_DEV_UNDEFINED_RANGE_MUIC ?
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0),
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE);

	if (battery->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		if (battery->usb_conn_status || (battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			pr_info("%s: Hiccup Set because of USB Temp\n", __func__);
			sec_bat_set_misc_event(battery,
					BATT_MISC_EVENT_TEMP_HICCUP_TYPE, BATT_MISC_EVENT_TEMP_HICCUP_TYPE);
			battery->usb_conn_status = USB_CONN_NORMAL;
		} else {
			pr_info("%s: Hiccup Set because of Water detect\n", __func__);
			sec_bat_set_misc_event(battery,
					BATT_MISC_EVENT_WATER_HICCUP_TYPE, BATT_MISC_EVENT_WATER_HICCUP_TYPE);
		}
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->hiccup_clear) {
			sec_bat_set_misc_event(battery, 0,
				(BATT_MISC_EVENT_WATER_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE));
			battery->hiccup_clear = false;
			pr_info("%s : Hiccup event clear! hiccup clear bit set (%d)\n",
					__func__, battery->hiccup_clear);
		} else if (battery->misc_event &
				(BATT_MISC_EVENT_WATER_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

	if (cable_type < 0 || cable_type > SEC_BATTERY_CABLE_MAX) {
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, battery->muic_cable_type);
		return -1; /* skip usb_typec_handle_after_id() */
	} else if ((cable_type == SEC_BATTERY_CABLE_UNKNOWN) &&
			(battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->cable_type = cable_type;
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev, "%s: UNKNOWN cable plugin\n", __func__);
		return -1; /* skip usb_typec_handle_after_id() */
	}
	battery->wire_status = cable_type;

#if defined(CONFIG_WIRELESS_TX_MODE)
	if (battery->wc_tx_enable && battery->uno_en) {
		int work_delay = 0;

		if (battery->wire_status == SEC_BATTERY_CABLE_NONE) {
			battery->buck_cntl_by_tx = true;
			sec_vote(battery->chgen_vote, VOTER_WC_TX, true, SEC_BAT_CHG_MODE_BUCK_OFF);
			if (battery->tx_switch_mode != TX_SWITCH_GEAR_PPS) {
				battery->tx_switch_mode = TX_SWITCH_MODE_OFF;
				battery->tx_switch_mode_change = false;
			}
			battery->tx_switch_start_soc = 0;
		}

		if (battery->pdata->tx_5v_disable && battery->wire_status == SEC_BATTERY_CABLE_TA)
			work_delay = battery->pdata->pre_afc_work_delay + 500;	//add delay more afc check

		if (battery->tx_switch_mode != TX_SWITCH_GEAR_PPS)
			sec_bat_run_wpc_tx_work(battery, work_delay);
	}
#endif

	cancel_delayed_work(&battery->cable_work);
	__pm_relax(battery->cable_ws);

	if (cable_type == SEC_BATTERY_CABLE_HV_TA_CHG_LIMIT) {
		/* set current event */
		sec_bat_cancel_input_check_work(battery);
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_CHG_LIMIT,
					(SEC_BAT_CURRENT_EVENT_CHG_LIMIT | SEC_BAT_CURRENT_EVENT_AFC));
		sec_vote(battery->input_vote, VOTER_VBUS_CHANGE, false, 0);

		/* Check VOTER_SIOP to set up current based on cable_type */
		__pm_stay_awake(battery->siop_level_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->siop_level_work, 0);

		__pm_stay_awake(battery->monitor_ws);
		battery->polling_count = 1;	/* initial value = 1 */
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	} else if ((battery->wire_status == battery->cable_type) &&
		(((battery->wire_status == SEC_BATTERY_CABLE_USB || battery->wire_status == SEC_BATTERY_CABLE_TA) &&
		battery->sink_status.rp_currentlvl >= RP_CURRENT_LEVEL_DEFAULT &&
		!(battery->current_event & SEC_BAT_CURRENT_EVENT_AFC)) ||
		is_hv_wire_type(battery->wire_status))) {
		sec_bat_cancel_input_check_work(battery);
		sec_bat_set_current_event(battery, 0, SEC_BAT_CURRENT_EVENT_AFC);
		sec_vote(battery->input_vote, VOTER_CABLE, true, battery->pdata->charging_current[cable_type].input_current_limit);
		sec_vote(battery->fcc_vote, VOTER_CABLE, true, battery->pdata->charging_current[cable_type].fast_charging_current);
		sec_vote(battery->input_vote, VOTER_VBUS_CHANGE, false, 0);

		__pm_stay_awake(battery->monitor_ws);
		battery->polling_count = 1;	/* initial value = 1 */
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	} else if (cable_type == SEC_BATTERY_CABLE_PREPARE_TA) {
		sec_bat_check_afc_input_current(battery);
	} else {
		__pm_stay_awake(battery->cable_ws);
		if (battery->ta_alert_wa && battery->ta_alert_mode != OCP_NONE) {
			if (!strcmp(cmd, "DETACH")) {
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, msecs_to_jiffies(3000));
			} else {
				queue_delayed_work(battery->monitor_wqueue,
					&battery->cable_work, 0);
			}
		} else {
			queue_delayed_work(battery->monitor_wqueue,
				&battery->cable_work, 0);
		}
	}

	return 0;
}

static int usb_typec_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd = "NONE";
	struct sec_battery_info *battery =
			container_of(nb, struct sec_battery_info, usb_typec_nb);
	int cable_type = SEC_BATTERY_CABLE_NONE;
	PD_NOTI_TYPEDEF *pdata = (PD_NOTI_TYPEDEF *)data;
	struct pdic_notifier_struct *pd_noti = pdata->pd;
	SEC_PD_SINK_STATUS *psink_status = NULL;
	int ret_handle = 0;

	dev_info(battery->dev, "%s: action:%ld src:%d, dest:%d, id:%d, sub1:%d, sub2:%d, sub3:%d\n",
		__func__, action, pdata->src, pdata->dest, pdata->id, pdata->sub1, pdata->sub2, pdata->sub3);

	if ((pdata->dest != PDIC_NOTIFY_DEV_BATT) && (pdata->dest != PDIC_NOTIFY_DEV_ALL)) {
		dev_info(battery->dev, "%s: skip handler dest(%d)\n",
			__func__, pdata->dest);
		return 0;
	}

	if (!pd_noti) {
		dev_info(battery->dev, "%s: pd_noti(pdata->pd) is NULL\n", __func__);
	} else {
		psink_status = &pd_noti->sink_status;

		if (!battery->psink_status) {
			battery->psink_status = psink_status;
			sec_pd_init_data(battery->psink_status);
			sec_pd_register_chg_info_cb(count_cisd_pd_data);
		}
	}

	mutex_lock(&battery->typec_notylock);
	switch (pdata->id) {
	case PDIC_NOTIFY_ID_WATER:
	case PDIC_NOTIFY_ID_ATTACH:
		ret_handle = usb_typec_handle_id_attach(battery, (PD_NOTI_ATTACH_TYPEDEF *)pdata, &cable_type, &cmd);
		break;
	case PDIC_NOTIFY_ID_POWER_STATUS:
		ret_handle = usb_typec_handle_id_power_status(battery, (PD_NOTI_POWER_STATUS_TYPEDEF *)pdata, &cable_type, &cmd);
		break;
	case PDIC_NOTIFY_ID_USB:
		ret_handle = usb_typec_handle_id_usb(battery, (PD_NOTI_ATTACH_TYPEDEF *)pdata);
		break;
	case PDIC_NOTIFY_ID_DEVICE_INFO:
		ret_handle = usb_typec_handle_id_device_info(battery, (PD_NOTI_DEVICE_INFO_TYPEDEF *)pdata);
		break;
	case PDIC_NOTIFY_ID_SVID_INFO:
		ret_handle = usb_typec_handle_id_svid_info(battery, (PD_NOTI_SVID_INFO_TYPEDEF *)pdata);
		break;
	case PDIC_NOTIFY_ID_CLEAR_INFO:
		ret_handle = usb_typec_handle_id_clear_info(battery, (PD_NOTI_CLEAR_INFO_TYPEDEF *)pdata);
		break;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		battery->hv_chg_name = "NONE";
		break;
	}

	if (ret_handle < 0)
		goto skip_handle_after_id;

	usb_typec_handle_after_id(battery, cable_type, cmd);

skip_handle_after_id:
	dev_info(battery->dev, "%s: CMD[%s], CABLE_TYPE[%d]\n", __func__, cmd, cable_type);
	mutex_unlock(&battery->typec_notylock);
	return 0;
}
#else
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
static int batt_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	const char *cmd;
	int cable_type = SEC_BATTERY_CABLE_NONE;
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info, batt_nb);
	union power_supply_propval value = {0, };

#if IS_ENABLED(CONFIG_PDIC_NOTIFIER) || IS_ENABLED(CONFIG_CCIC_NOTIFIER)
	PD_NOTI_ATTACH_TYPEDEF *p_noti = (PD_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = p_noti->cable_type;
#else
	muic_attached_dev_t attached_dev = *(muic_attached_dev_t *)data;
#endif

	mutex_lock(&battery->batt_handlelock);
	switch (action) {
	case MUIC_NOTIFY_CMD_DETACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_DETACH:
		cmd = "DETACH";
		battery->is_jig_on = false;
		cable_type = SEC_BATTERY_CABLE_NONE;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		break;
	case MUIC_NOTIFY_CMD_ATTACH:
	case MUIC_NOTIFY_CMD_LOGICALLY_ATTACH:
		cmd = "ATTACH";
		cable_type = sec_bat_cable_check(battery, attached_dev);
		battery->muic_cable_type = attached_dev;
		break;
	default:
		cmd = "ERROR";
		cable_type = -1;
		battery->muic_cable_type = ATTACHED_DEV_NONE_MUIC;
		break;
	}

	sec_bat_set_misc_event(battery,
#if !defined(CONFIG_ENG_BATTERY_CONCEPT) && !defined(CONFIG_SEC_FACTORY)
		(battery->muic_cable_type == ATTACHED_DEV_JIG_UART_ON_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) |
		(battery->muic_cable_type == ATTACHED_DEV_JIG_USB_ON_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0) |
#endif
		(battery->muic_cable_type == ATTACHED_DEV_UNDEFINED_RANGE_MUIC ? BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE : 0),
		BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE);

	if (battery->muic_cable_type == ATTACHED_DEV_HICCUP_MUIC) {
		if (battery->usb_conn_status || (battery->misc_event & BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			pr_info("%s: Hiccup Set because of USB Temp\n", __func__);
			sec_bat_set_misc_event(battery,
					BATT_MISC_EVENT_TEMP_HICCUP_TYPE, BATT_MISC_EVENT_TEMP_HICCUP_TYPE);
			battery->usb_conn_status = USB_CONN_NORMAL;
		} else {
			pr_info("%s: Hiccup Set because of Water detect\n", __func__);
			sec_bat_set_misc_event(battery,
					BATT_MISC_EVENT_WATER_HICCUP_TYPE, BATT_MISC_EVENT_WATER_HICCUP_TYPE);
		}
		battery->hiccup_status = 1;
	} else {
		battery->hiccup_status = 0;
		if (battery->misc_event &
				(BATT_MISC_EVENT_WATER_HICCUP_TYPE | BATT_MISC_EVENT_TEMP_HICCUP_TYPE)) {
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
	}

	/* If PD cable is already attached, return this function */
	if (battery->pdic_attach) {
		dev_info(battery->dev, "%s: ignore event pdic attached(%d)\n",
			__func__, battery->pdic_attach);
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	}

	if (attached_dev == ATTACHED_DEV_MHL_MUIC) {
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	}

	if (cable_type < 0) {
		dev_info(battery->dev, "%s: ignore event(%d)\n",
			__func__, cable_type);
	} else if ((cable_type == SEC_BATTERY_CABLE_UNKNOWN) &&
		(battery->status != POWER_SUPPLY_STATUS_DISCHARGING)) {
		battery->cable_type = cable_type;
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		dev_info(battery->dev,
			"%s: UNKNOWN cable plugin\n", __func__);
		mutex_unlock(&battery->batt_handlelock);
		return 0;
	} else {
		battery->wire_status = cable_type;
		if (is_nocharge_type(battery->wire_status) && (battery->wc_status != SEC_BATTERY_CABLE_NONE))
			cable_type = SEC_BATTERY_CABLE_WIRELESS;
	}
	dev_info(battery->dev,
			"%s: current_cable(%d), wc_status(%d), wire_status(%d)\n",
			__func__, cable_type, battery->wc_status,
			battery->wire_status);

	mutex_unlock(&battery->batt_handlelock);
	if (attached_dev == ATTACHED_DEV_USB_LANHUB_MUIC) {
		if (!strcmp(cmd, "ATTACH")) {
			value.intval = true;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHARGE_POWERED_OTG_CONTROL,
					value);
			dev_info(battery->dev,
				"%s: Powered OTG cable attached\n", __func__);
		} else {
			value.intval = false;
			psy_do_property(battery->pdata->charger_name, set,
					POWER_SUPPLY_EXT_PROP_CHARGE_POWERED_OTG_CONTROL,
					value);
			dev_info(battery->dev,
				"%s: Powered OTG cable detached\n", __func__);
		}
	}

	if (!strcmp(cmd, "ATTACH")) {
		if ((battery->muic_cable_type >= ATTACHED_DEV_QC_CHARGER_PREPARE_MUIC) &&
			(battery->muic_cable_type <= ATTACHED_DEV_QC_CHARGER_9V_MUIC)) {
			battery->hv_chg_name = "QC";
		} else if ((battery->muic_cable_type >= ATTACHED_DEV_AFC_CHARGER_PREPARE_MUIC) &&
			(battery->muic_cable_type <= ATTACHED_DEV_AFC_CHARGER_ERR_V_DUPLI_MUIC)) {
			battery->hv_chg_name = "AFC";
#if defined(CONFIG_MUIC_HV_12V)
		} else if (battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_MUIC ||
			battery->muic_cable_type == ATTACHED_DEV_AFC_CHARGER_12V_DUPLI_MUIC) {
			battery->hv_chg_name = "12V";
#endif
		} else
			battery->hv_chg_name = "NONE";
	} else {
			battery->hv_chg_name = "NONE";
	}

	pr_info("%s : HV_CHARGER_NAME(%s)\n",
		__func__, battery->hv_chg_name);

	if ((cable_type >= 0) &&
		cable_type <= SEC_BATTERY_CABLE_MAX) {
		if (cable_type == SEC_BATTERY_CABLE_NONE) {
			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		} else if (cable_type != battery->cable_type) {
			__pm_stay_awake(battery->cable_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->cable_work, 0);
		} else {
			dev_info(battery->dev,
				"%s: Cable is Not Changed(%d)\n",
				__func__, battery->cable_type);
		}
	}

	pr_info("%s: CMD=%s, attached_dev=%d\n", __func__, cmd, attached_dev);

	return 0;
}
#endif /* CONFIG_MUIC_NOTIFIER */
#endif

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
static int vbus_handle_notification(struct notifier_block *nb,
		unsigned long action, void *data)
{
	vbus_status_t vbus_status = *(vbus_status_t *)data;
	struct sec_battery_info *battery =
		container_of(nb, struct sec_battery_info, vbus_nb);

	mutex_lock(&battery->batt_handlelock);
	pr_info("battery: %s: action=%d, vbus_status=%s, otg=%s\n",
		__func__, (int)action, vbus_status == STATUS_VBUS_HIGH ? "HIGH" : "LOW",
		battery->is_otg_on ? "ON" : "OFF");

#if IS_ENABLED(CONFIG_LSI_IFPMIC)
	if (battery->pdata->support_vpdo && battery->cable_type == SEC_BATTERY_CABLE_PDIC) {
		sec_vote_refresh(battery->input_vote);
		sec_vote_refresh(battery->fcc_vote);
		sec_vote_refresh(battery->chgen_vote);
	}
#endif
	if (battery->pdata->support_usb_conn_check &&
		!battery->is_otg_on && vbus_status == STATUS_VBUS_HIGH && !delayed_work_pending(&battery->usb_conn_check_work) &&
		!battery->run_usb_conn_check) {
#if !defined(CONFIG_SEC_FACTORY)
		battery->run_usb_conn_check = true;
		battery->usb_conn_check_cnt = 0;
		__pm_stay_awake(battery->usb_conn_check_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->usb_conn_check_work, msecs_to_jiffies(1000));
#endif
	}
	mutex_unlock(&battery->batt_handlelock);

	return 0;
}
#endif

__visible_for_testing void sec_bat_parse_param_value(struct sec_battery_info *battery)
{
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	union power_supply_propval value = {0, };
#endif
	int chg_mode = 0;
	int pd_hv_disable = 0;
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	int afc_mode = 0;
#endif

	chg_mode = sec_bat_get_chgmode() & 0x000000FF;
	pr_info("%s: charging_mode: 0x%x (charging_night_mode:0x%x)\n",
		__func__, sec_bat_get_chgmode(), chg_mode);

	pd_hv_disable = sec_bat_get_dispd() & 0x000000FF;
	pr_info("%s: pd_disable: 0x%x (pd_hv_disable:0x%x)\n",
		__func__, sec_bat_get_dispd(), pd_hv_disable);

#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	afc_mode = get_afc_mode();
	pr_info("%s: afc_mode: 0x%x\n", __func__, afc_mode);
#endif

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	battery->charging_night_mode = chg_mode;

	/* Check High Voltage charging option for wireless charging */
	/* '1' means disabling High Voltage charging */
	if (battery->charging_night_mode == '1') /* 0x31 */
		battery->sleep_mode = true;
	else
		battery->sleep_mode = false;
	value.intval = battery->sleep_mode;
	psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_EXT_PROP_SLEEP_MODE, value);
#endif
#if defined(CONFIG_PD_CHARGER_HV_DISABLE)
	if (pd_hv_disable == '1') { /* 0x31 */
		battery->pd_disable = true;
		pr_info("PD wired charging mode is disabled\n");
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
	}
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	/* Check High Voltage charging option for wired charging */
	if (afc_mode == CH_MODE_AFC_DISABLE_VAL) {
		pr_info("None PD wired charging mode is disabled\n");
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE, SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE);
	}
#endif
#else
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	/* Check High Voltage charging option for wired charging */
	if (afc_mode == CH_MODE_AFC_DISABLE_VAL) {
		pr_info("HV wired charging mode is disabled\n");
		sec_bat_set_current_event(battery,
			SEC_BAT_CURRENT_EVENT_HV_DISABLE, SEC_BAT_CURRENT_EVENT_HV_DISABLE);
	}
#endif
#endif

#if IS_ENABLED(CONFIG_USB_FACTORY_MODE)
	pr_info("%s: f_mode: %s\n", __func__, f_mode);

	if (!f_mode) {
		battery->batt_f_mode = NO_MODE;
	} else if ((strncmp(f_mode, "OB", 2) == 0) || (strncmp(f_mode, "DL", 2) == 0)) {
		/* Set factory mode variables in OB mode */
		sec_bat_set_facmode(true);
		battery->factory_mode = true;
		battery->batt_f_mode = OB_MODE;
	} else if (strncmp(f_mode, "IB", 2) == 0) {
		battery->batt_f_mode = IB_MODE;
	} else {
		battery->batt_f_mode = NO_MODE;
	}
	pr_info("[BAT] %s: f_mode: %s\n", __func__, BOOT_MODE_STRING[battery->batt_f_mode]);

	battery->usb_factory_init = false;
#if defined(CONFIG_SEC_FACTORY)
	battery->usb_factory_slate_mode = false;
#endif
#endif

	battery->factory_mode_boot_on = sec_bat_get_facmode();
}
EXPORT_SYMBOL_KUNIT(sec_bat_parse_param_value);

static void sec_bat_afc_init_work(struct work_struct *work)
{
	int ret = 0;

#if defined(CONFIG_AFC_CHARGER_MODE)
	ret = muic_hv_charger_init();
#endif
	pr_info("%s, ret(%d)\n", __func__, ret);
}

static void sec_bat_check_spsn_open(struct sec_battery_info *battery)
{
#if !defined(CONFIG_SEC_FACTORY)
	union power_supply_propval value = {0, };

	if (sec_bat_get_facmode() || battery->factory_mode ||
		!battery->pdata->support_spsn_ctrl) {
		pr_info("%s: skip factory mode or not support spsn ctrl\n", __func__);
		return;
	}

	if (battery->voltage_now < 3000) {
		pr_info("%s: voltage = %d\n", __func__, battery->voltage_now);
		return;
	}

	psy_do_property(battery->pdata->charger_name, get,
		POWER_SUPPLY_EXT_PROP_SPSN_TEST, value);
	if (!value.intval) {
		value.intval = 100;
		psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_SPSN_TEST, value);
	}
#endif
}

void sb_update_fv(struct sec_battery_info *battery, int new_fv, int current_bat_id)
{
	sec_battery_platform_data_t *pdata = battery->pdata;

	pr_info("%s: changed chg_float_voltage for batt_id(%d): %d->%d\n",
		__func__, current_bat_id, battery->pdata->chg_float_voltage, new_fv);

	pdata->chg_float_voltage = new_fv;
	pdata->low_temp_float = new_fv;
	pdata->full_condition_vcell = pdata->chg_float_voltage - DEFAULT_FULL_MARGIN;
	pdata->recharge_condition_vcell = pdata->chg_float_voltage - DEFAULT_RCHG_MARGIN;
	pdata->swelling_low_rechg_voltage = pdata->chg_float_voltage - DEFAULT_LOW_SWELLING_RCHG_MARGIN;

	sec_vote(battery->fv_vote, VOTER_MULTI_BATT_CHG, true, battery->pdata->chg_float_voltage);
}

static void sb_check_multi_batt_id_charging(struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	char prop_name[64] = {0,};
	u32 temp = 0;
	int ret = 0;
	int current_bat_id = battery->pdata->current_bat_id;
	union power_supply_propval value = {0, };

	if (!pdata->multi_batt_id_chg_support)
		return;

	if (!pdata->batt_id_changed)
		return;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return;
	}

	memset(prop_name, 0x0, sizeof(prop_name));
	sprintf(prop_name, "battery%d,chg_float_voltage", current_bat_id);
	ret = of_property_read_u32(np, prop_name, &temp);
	if (ret || (temp == pdata->chg_float_voltage))
		pr_info("%s: chg_float_voltage is unchanged\n", __func__);
	else
		sb_update_fv(battery, temp, current_bat_id);

	// Force disable DC
	ret = psy_do_property(pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_FORCE_SWC, value);
	if (ret < 0)
		dev_err(battery->dev,
			"%s : Failed to disable DC\n", __func__);

	memset(prop_name, 0x0, sizeof(prop_name));
	sprintf(prop_name, "battery%d,max_charging_current", current_bat_id);

	ret = of_property_read_u32(np, prop_name, &temp);
	if (ret) {
		pr_info("%s: max_charging_current is unchanged\n", __func__);
	} else {
		pr_info("%s: changed max_charging_current for batt_id(%d): %d->%d\n",
			__func__, current_bat_id, battery->pdata->max_charging_current, temp);
		battery->pdata->max_charging_current = temp;
		sec_vote(battery->fcc_vote, VOTER_MULTI_BATT_CHG,
			true, battery->pdata->max_charging_current);
	}
} /* sb_check_multi_batt_id_charging */

static void sec_bat_get_multi_step_chg_param(struct sec_battery_info *battery)
{
	union power_supply_propval value = {0, };
	struct device_node *np;
	int ret, type, updated_bat_id, current_bat_id;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return;
	}

	ret = of_property_read_u32(np, "battery,dc_multi_step_chg", &updated_bat_id);
	if (ret) {
		pr_err("%s: dc_multi_step_chg is Empty\n", __func__);
		return;
	}

	ret = of_property_read_u32(np, "battery,dc_multi_step_chg_type", &type);
	if (ret) {
		pr_err("%s: dc_multi_step_chg_type is Empty\n", __func__);
		return;
	}

	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_EXT_PROP_BATTERY_ID, value);
	current_bat_id = value.intval;
	pr_info("%s: updated battery_id(%d), current battery_id(%d) type(0x%x)\n",
		__func__, updated_bat_id, current_bat_id, type);

	if (current_bat_id != updated_bat_id)
		return;

	battery->pdata->batt_id_changed = true;
	battery->pdata->current_bat_id = current_bat_id;

#if IS_ENABLED(CONFIG_STEP_CHARGING)
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	sec_update_dc_step_chg_dt(battery, current_bat_id, type);
#endif
#endif
}

#if IS_ENABLED(CONFIG_HALL_NOTIFIER) && !defined(CONFIG_ARCH_MEDIATEK)
static int sb_hall_ic_notifier(struct notifier_block *nb, unsigned long hall_ic, void *ptr)
{
	struct sec_battery_info *battery = container_of(nb, struct sec_battery_info, hallic_nb);
	union power_supply_propval value = {0, };

	pr_info("%s: hall_ic %s\n", __func__, (hall_ic ? "CLOSE":"OPEN"));

	value.intval = hall_ic ? 1 : 0;
	battery->mag_cover = value.intval;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_EXT_PROP_MPP_COVER, value);

	return 0;
}
#endif

static void sec_batt_dev_init_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
				struct sec_battery_info, dev_init_work.work);
	union power_supply_propval value = {0, };
#if defined(CONFIG_STORE_MODE) && !defined(CONFIG_SEC_FACTORY) && IS_ENABLED(CONFIG_DIRECT_CHARGING)
	char direct_charging_source_status[2] = {0, };
#endif
#if defined(CONFIG_MULTI_DIRECT_CHARGER)
	int ret = 0;
#endif

	pr_info("%s: Start\n", __func__);

	sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_DISCHARGING);
	sec_bat_set_health(battery, POWER_SUPPLY_HEALTH_GOOD);
	battery->capacity = 50;

	sec_chg_check_modprobe();

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
#if defined(CONFIG_MULTI_DIRECT_CHARGER)
	ret = psy_do_property(battery->pdata->charger_name, set,
			POWER_SUPPLY_EXT_PROP_CHECK_VALID_DC_IC, value);
	if (ret < 0)
		dev_err(battery->dev,
			"%s : Failed to sb_get_dc_ic_name\n", __func__);
#endif
#endif
	sec_bat_get_multi_step_chg_param(battery);
	sb_check_multi_batt_id_charging(battery);
	/* updates temperatures on boot */
	sec_bat_get_temperature_info(battery);
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	sec_bat_check_dual_battery_balance(battery);
#endif

	/* initialize battery voltage*/
	value.intval = SEC_BATTERY_VOLTAGE_MV;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
	battery->voltage_now = value.intval;

	/* initialize average battery voltage*/
	value.intval = SEC_BATTERY_VOLTAGE_MV;
	psy_do_property(battery->pdata->fuelgauge_name, get,
		POWER_SUPPLY_PROP_VOLTAGE_AVG, value);
	battery->voltage_avg = value.intval;

	/* initialize battery level*/
	battery->capacity = sec_bat_get_batt_capacity(battery, battery->capacity);

	sec_bat_check_spsn_open(battery);

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	sec_bat_check_boottime(battery, battery->pdata->dctp_bootmode_en);
#endif

	sec_bat_parse_param_value(battery);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	queue_delayed_work(battery->monitor_wqueue, &battery->fw_init_work, msecs_to_jiffies(2000));
#endif

	psy_do_property(battery->pdata->wireless_charger_name, get,
		POWER_SUPPLY_EXT_PROP_WPC_EN, value);
	sec_bat_set_current_event(battery,
		value.intval ? SEC_BAT_CURRENT_EVENT_WPC_EN : 0, SEC_BAT_CURRENT_EVENT_WPC_EN);

	/*
	 * notify wireless charger driver when sec_battery probe is done,
	 * if wireless charging is possible, POWER_SUPPLY_PROP_ONLINE of wireless property will be called.
	 */
	value.intval = 0;
	psy_do_property(battery->pdata->wireless_charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_TYPE, value);
#endif
#if defined(CONFIG_USE_POGO)
	/*
	 * notify pogo charger driver when sec_battery probe is done,
	 * if pogo charging is possible, POWER_SUPPLY_PROP_ONLINE of pogo property will be called.
	 */
	value.intval = 0;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_TYPE, value);
#endif

#if defined(CONFIG_STORE_MODE) && !defined(CONFIG_SEC_FACTORY)
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	direct_charging_source_status[0] = SEC_STORE_MODE;
	direct_charging_source_status[1] = SEC_CHARGING_SOURCE_SWITCHING;
	value.strval = direct_charging_source_status;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_EXT_PROP_CHANGE_CHARGING_SOURCE, value);
#endif
#endif
	register_batterylog_proc();

#if IS_ENABLED(CONFIG_HALL_NOTIFIER) && !defined(CONFIG_ARCH_MEDIATEK)
	if (battery->pdata->hall_ic_notifier) {
		pr_info("%s: hall_ic_notifier(%d)\n", __func__, battery->pdata->hall_ic_notifier);
		battery->hallic_nb.notifier_call = sb_hall_ic_notifier;
		battery->hallic_nb.priority = 1;
		hall_notifier_register(&battery->hallic_nb);
	}
#endif
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	battery->sink_status.rp_currentlvl = RP_CURRENT_LEVEL_NONE;
#if IS_ENABLED(CONFIG_SEC_MTK_CHARGER)
	psy_do_property("primary_chg", get,
		POWER_SUPPLY_EXT_PROP_RP_LEVEL, value);
	battery->sink_status.rp_currentlvl = value.intval;
	pr_info("%s: battery->rp_currentlvl(%d)\n", __func__, battery->sink_status.rp_currentlvl);
#endif
	manager_notifier_register(&battery->usb_typec_nb,
		usb_typec_handle_notification, MANAGER_NOTIFY_PDIC_BATTERY);
#else
#if IS_ENABLED(CONFIG_MUIC_NOTIFIER)
	muic_notifier_register(&battery->batt_nb,
		batt_handle_notification, MUIC_NOTIFY_DEV_CHARGER);
#endif
#endif
	value.intval = true;
	psy_do_property(battery->pdata->charger_name, set,
		POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX, value);

#if defined(CONFIG_SEC_COMMON)
	/* make fg_reset true again for actual normal booting after recovery kernel is done */
	if (sec_bat_get_fgreset() && seccmn_recv_is_boot_recovery()) {
		pr_info("%s: fg_reset(%d) boot_recov(%d)\n",
			__func__, sec_bat_get_fgreset(), seccmn_recv_is_boot_recovery());
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_ENERGY_NOW, value);
		pr_info("%s: make fg_reset true again for actual normal booting\n", __func__);
	}
#endif
#if defined(CONFIG_NO_BATTERY)
	sec_vote(battery->chgen_vote, VOTER_NO_BATTERY, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
#else
	sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_USB_100MA, SEC_BAT_CURRENT_EVENT_USB_100MA);
	sec_vote(battery->input_vote, VOTER_USB_100MA, true, 100);
	sec_vote(battery->fcc_vote, VOTER_USB_100MA, true, 100);
	sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, true, battery->pdata->full_check_current_1st);
	sec_vote(battery->fv_vote, VOTER_FULL_CHARGE, true, battery->pdata->chg_float_voltage);
#endif

#if defined(CONFIG_BC12_DEVICE)
#if defined(CONFIG_SEC_FACTORY)
	value.intval = 0;
	psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_EXT_PROP_CHECK_INIT, value);
	battery->vbat_adc_open = (value.intval == 1) ? true : false;
#endif
	value.intval = 0;
	psy_do_property(battery->pdata->charger_name, get,
			POWER_SUPPLY_EXT_PROP_CHECK_INIT, value);
#endif

#if IS_ENABLED(CONFIG_VBUS_NOTIFIER)
	vbus_notifier_register(&battery->vbus_nb,
		vbus_handle_notification, VBUS_NOTIFY_DEV_BATTERY);
#endif

	queue_delayed_work(battery->monitor_wqueue, &battery->afc_init_work, 0);
	if ((battery->wire_status == SEC_BATTERY_CABLE_NONE) ||
		(battery->wire_status == SEC_BATTERY_CABLE_PREPARE_TA)) {
		__pm_stay_awake(battery->monitor_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
	}
	pr_info("%s: End\n", __func__);

	__pm_relax(battery->dev_init_ws);
}

static int sb_handle_notification(struct notifier_block *nb, unsigned long action, void *data)
{
	struct sec_battery_info *battery = container_of(nb, struct sec_battery_info, sb_nb);
	sb_data *sbd = data;
	int ret = 0;

	switch (action) {
	case SB_NOTIFY_DEV_PROBE:
		ret = sb_sysfs_create_attrs(&battery->psy_bat->dev);
		pr_info("%s: create sysfs node (name = %s, count = %d)\n",
			__func__, cast_sb_data_ptr(const char, sbd), ret);
		break;
	case SB_NOTIFY_DEV_LIST:
	{
		struct sbn_dev_list *tmp_list = cast_sb_data_ptr(struct sbn_dev_list, sbd);

		ret = sb_sysfs_create_attrs(&battery->psy_bat->dev);
		pr_info("%s: create sysfs node (dev_count = %d, sysfs_count = %d)\n",
			__func__, tmp_list->count, ret);
	}
		break;
	case SB_NOTIFY_EVENT_MISC:
	{
		struct sbn_bit_event *misc = cast_sb_data_ptr(struct sbn_bit_event, sbd);

		sec_bat_set_misc_event(battery, misc->value, misc->mask);
	}
		break;
	case SB_NOTIFY_DEV_SHUTDOWN:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct power_supply_desc battery_power_supply_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = sec_battery_props,
	.num_properties = ARRAY_SIZE(sec_battery_props),
	.get_property = sec_bat_get_property,
	.set_property = sec_bat_set_property,
};

static const struct power_supply_desc usb_power_supply_desc = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = sec_power_props,
	.num_properties = ARRAY_SIZE(sec_power_props),
	.get_property = sec_usb_get_property,
};

static const struct power_supply_desc ac_power_supply_desc = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = sec_ac_props,
	.num_properties = ARRAY_SIZE(sec_ac_props),
	.get_property = sec_ac_get_property,
};

static const struct power_supply_desc wireless_power_supply_desc = {
	.name = "wireless",
	.type = POWER_SUPPLY_TYPE_WIRELESS,
	.properties = sec_wireless_props,
	.num_properties = ARRAY_SIZE(sec_wireless_props),
	.get_property = sec_wireless_get_property,
	.set_property = sec_wireless_set_property,
};

static const struct power_supply_desc pogo_power_supply_desc = {
	.name = "pogo",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = sec_power_props,
	.num_properties = ARRAY_SIZE(sec_power_props),
	.get_property = sec_pogo_get_property,
	.set_property = sec_pogo_set_property,
};

static const struct power_supply_desc otg_power_supply_desc = {
	.name = "otg",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.properties = sec_otg_props,
	.num_properties = ARRAY_SIZE(sec_otg_props),
	.get_property = sec_otg_get_property,
	.set_property = sec_otg_set_property,
	.external_power_changed = sec_otg_external_power_changed,
};

static int sec_battery_probe(struct platform_device *pdev)
{
	sec_battery_platform_data_t *pdata = NULL;
	struct sec_battery_info *battery;
	struct power_supply_config battery_cfg = {};
	int ret = 0;

	dev_info(&pdev->dev,
		"%s: SEC Battery Driver Loading\n", __func__);

	battery = kzalloc(sizeof(*battery), GFP_KERNEL);
	if (!battery)
		return -ENOMEM;

	if (pdev->dev.of_node) {
		pdata = devm_kzalloc(&pdev->dev,
				sizeof(sec_battery_platform_data_t),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&pdev->dev, "Failed to allocate memory\n");
			ret = -ENOMEM;
			goto err_bat_free;
		}

		battery->pdata = pdata;

		if (sec_bat_parse_dt(&pdev->dev, battery)) {
			dev_err(&pdev->dev,
				"%s: Failed to get battery dt\n", __func__);
			ret = -EINVAL;
			goto err_bat_free;
		}
	} else {
		pdata = dev_get_platdata(&pdev->dev);
		battery->pdata = pdata;
	}

	if (battery->pdata->soc_by_repcap_en) {
		battery->eoc_d = kzalloc(sizeof(struct sec_eoc_info),
			GFP_KERNEL);
		if (!battery->eoc_d) {
			ret = -EINVAL;
			goto err_pdata_free;
		}
	}
	platform_set_drvdata(pdev, battery);

	battery->dev = &pdev->dev;

	mutex_init(&battery->iolock);
	mutex_init(&battery->misclock);
	mutex_init(&battery->txeventlock);
	mutex_init(&battery->batt_handlelock);
	mutex_init(&battery->current_eventlock);
	mutex_init(&battery->typec_notylock);
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	mutex_init(&battery->bc12_notylock);
#endif
	mutex_init(&battery->wclock);
	mutex_init(&battery->voutlock);

	dev_dbg(battery->dev, "%s: ADC init\n", __func__);

	adc_init(pdev, battery);

	battery->monitor_ws = wakeup_source_register(&pdev->dev, "sec-battery-monitor");
	battery->cable_ws = wakeup_source_register(&pdev->dev, "sec-battery-cable");
	battery->vbus_ws = wakeup_source_register(&pdev->dev, "sec-battery-vbus");
	battery->input_ws = wakeup_source_register(&pdev->dev, "sec-battery-input");
	battery->siop_level_ws = wakeup_source_register(&pdev->dev, "sec-battery-siop_level");
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	battery->ext_event_ws = wakeup_source_register(&pdev->dev, "sec-battery-ext_event");
	battery->wpc_tx_ws = wakeup_source_register(&pdev->dev, "sec-battery-wcp-tx");
	battery->wpc_tx_en_ws = wakeup_source_register(&pdev->dev, "sec-battery-wpc_tx_en");
	battery->tx_event_ws = wakeup_source_register(&pdev->dev, "sec-battery-tx-event");
	battery->wc20_current_ws = wakeup_source_register(&pdev->dev, "sec-battery-wc20-current");
	battery->mpla_thr_cnt_check_ws = wakeup_source_register(&pdev->dev, "sec-battery-mpla-thr-cnt-check");
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	battery->batt_data_ws = wakeup_source_register(&pdev->dev, "sec-battery-update-data");
#endif
	battery->misc_event_ws = wakeup_source_register(&pdev->dev, "sec-battery-misc-event");
	battery->parse_mode_dt_ws = wakeup_source_register(&pdev->dev, "sec-battery-parse_mode_dt");
	battery->dev_init_ws = wakeup_source_register(&pdev->dev, "sec-battery-dev-init");
	battery->usb_conn_check_ws = wakeup_source_register(&pdev->dev, "sec-battery-usb-thm-check");
	battery->transit_clear_ws = wakeup_source_register(&pdev->dev, "sec-battery-transit-clear");

	/* create work queue */
	battery->monitor_wqueue =
		create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!battery->monitor_wqueue) {
		dev_err(battery->dev,
			"%s: Fail to Create Workqueue\n", __func__);
		goto err_irq;
	}

	INIT_DELAYED_WORK(&battery->monitor_work, sec_bat_monitor_work);
	INIT_DELAYED_WORK(&battery->cable_work, sec_bat_cable_work);
	INIT_DELAYED_WORK(&battery->slowcharging_work, sec_bat_check_slowcharging_work);
	INIT_DELAYED_WORK(&battery->input_check_work, sec_bat_input_check_work);
	INIT_DELAYED_WORK(&battery->siop_level_work, sec_bat_siop_level_work);
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	INIT_DELAYED_WORK(&battery->fw_init_work, sec_bat_fw_init_work);
#endif
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	INIT_DELAYED_WORK(&battery->wpc_tx_work, sec_bat_wpc_tx_work);
	INIT_DELAYED_WORK(&battery->wpc_tx_en_work, sec_bat_wpc_tx_en_work);
	INIT_DELAYED_WORK(&battery->wpc_txpower_calc_work, sec_bat_txpower_calc_work);
	INIT_DELAYED_WORK(&battery->ext_event_work, sec_bat_ext_event_work);
	INIT_DELAYED_WORK(&battery->wc20_current_work, sec_bat_wc20_current_work);
	INIT_DELAYED_WORK(&battery->mpla_thr_cnt_check_work, sec_bat_mpla_thr_cnt_check_work);
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	INIT_DELAYED_WORK(&battery->batt_data_work, sec_bat_update_data_work);
#endif
	INIT_DELAYED_WORK(&battery->misc_event_work, sec_bat_misc_event_work);
	INIT_DELAYED_WORK(&battery->parse_mode_dt_work, sec_bat_parse_mode_dt_work);
	INIT_DELAYED_WORK(&battery->dev_init_work, sec_batt_dev_init_work);
	INIT_DELAYED_WORK(&battery->afc_init_work, sec_bat_afc_init_work);
	INIT_DELAYED_WORK(&battery->usb_conn_check_work, sec_bat_usb_conn_check_work);
	INIT_DELAYED_WORK(&battery->transit_clear_work, sec_bat_transit_clear_work);

	battery->fcc_vote = sec_vote_init("FCC", SEC_VOTE_MIN, VOTER_MAX,
			500, sec_voter_name, set_charging_current, battery);
	battery->input_vote = sec_vote_init("ICL", SEC_VOTE_MIN, VOTER_MAX,
			500, sec_voter_name, set_input_current, battery);
	battery->fv_vote = sec_vote_init("FV", SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->chg_float_voltage, sec_voter_name, set_float_voltage, battery);
	battery->dc_fv_vote = sec_vote_init("DCFV", SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->chg_float_voltage, sec_voter_name, set_dc_float_voltage, battery);
	battery->chgen_vote = sec_vote_init("CHGEN", SEC_VOTE_MIN, VOTER_MAX,
			SEC_BAT_CHG_MODE_CHARGING_OFF, sec_voter_name, sec_bat_set_charge, battery);
	battery->topoff_vote = sec_vote_init("TOPOFF", SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->full_check_current_1st, sec_voter_name, set_topoff_current, battery);
	battery->iv_vote = sec_vote_init("IV", SEC_VOTE_MIN, VOTER_MAX,
			SEC_INPUT_VOLTAGE_5V, sec_voter_name, sec_bat_change_iv, battery);
	battery->dc_op_mode_vote = sec_vote_init("DCHG_OP", SEC_VOTE_MIN, VOTER_MAX,
			DC_MODE_NONE, sec_voter_name, set_dc_op_mode, battery);
	battery->apdo_max_volt_vote = sec_vote_init("MAX_APDO_VOLT", SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->apdo_max_volt, sec_voter_name, set_apdo_max_volt, battery);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	battery->vlim_vote = sec_vote_init("VLIM", SEC_VOTE_MIN, VOTER_MAX,
			battery->pdata->chg_float_voltage - battery->pdata->limiter_aging_float_offset,
			sec_voter_name, set_vlim_voltage, battery);
#endif

	/* set vote priority */
	change_sec_voter_pri(battery->iv_vote, VOTER_MUIC_ABNORMAL, VOTE_PRI_10);
	change_sec_voter_pri(battery->iv_vote, VOTER_MST, VOTE_PRI_10);
	change_sec_voter_pri(battery->iv_vote, VOTER_CHANGE_CHGMODE, VOTE_PRI_9);
	change_sec_voter_pri(battery->iv_vote, VOTER_WC_TX, VOTE_PRI_8);
	change_sec_voter_pri(battery->iv_vote, VOTER_AICL, VOTE_PRI_1);
	change_sec_voter_pri(battery->iv_vote, VOTER_FLASH, VOTE_PRI_3);
	change_sec_voter_pri(battery->chgen_vote, VOTER_NO_BATTERY, VOTE_PRI_10);
	change_sec_voter_pri(battery->chgen_vote, VOTER_CHANGE_CHGMODE, VOTE_PRI_9);
	change_sec_voter_pri(battery->fv_vote, VOTER_SWELLING, VOTE_PRI_10);
	change_sec_voter_pri(battery->dc_fv_vote, VOTER_SWELLING, VOTE_PRI_10);
	change_sec_voter_pri(battery->dc_op_mode_vote, VOTER_DC_OP_MODE_SYSFS, VOTE_PRI_9);
	change_sec_voter_pri(battery->apdo_max_volt_vote, VOTER_DC_OP_MODE_SYSFS, VOTE_PRI_9);
	change_sec_voter_pri(battery->dc_op_mode_vote, VOTER_DC_OP_MODE_F, VOTE_PRI_10);
	change_sec_voter_pri(battery->apdo_max_volt_vote, VOTER_DC_OP_MODE_F, VOTE_PRI_10);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	change_sec_voter_pri(battery->vlim_vote, VOTER_SWELLING, VOTE_PRI_10);
	change_sec_voter_pri(battery->vlim_vote, VOTER_LRP_TEMP, VOTE_PRI_9);
	change_sec_voter_pri(battery->vlim_vote, VOTER_STEP_CHARGE, VOTE_PRI_8);
	change_sec_voter_pri(battery->vlim_vote, VOTER_DC_STEP_CHARGE, VOTE_PRI_8);
#endif

	switch (pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		INIT_DELAYED_WORK(&battery->polling_work,
			sec_bat_polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		battery->last_poll_time = ktime_get_boottime();
		alarm_init(&battery->polling_alarm, ALARM_BOOTTIME,
			sec_bat_alarm);
		break;
	default:
		break;
	}

	battery->pogo_status = 0;
	battery->pogo_9v = false;

	battery->is_fpdo_dc = false;

	battery->ta_alert_mode = OCP_NONE;
	battery->present = true;
	battery->is_jig_on = false;
	battery->wdt_kick_disable = 0;
	battery->polling_count = 1;	/* initial value = 1 */
	battery->polling_time = pdata->polling_time[
		SEC_BATTERY_POLLING_TIME_DISCHARGING];
	battery->polling_in_sleep = false;
	battery->polling_short = false;
	battery->check_count = 0;
	battery->check_adc_count = 0;
	battery->check_adc_value = 0;
	battery->input_current = 0;
	battery->charging_current = 0;
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	battery->main_current = 0;
	battery->sub_current = 0;
	battery->limiter_check = false;
	battery->set_lower_curr = false;
#endif
	battery->topoff_condition = 0;
	battery->wpc_vout_level = WIRELESS_VOUT_5_5V;
	battery->wpc_max_vout_level = WIRELESS_VOUT_12_5V;
	battery->charging_retention_time = 0;
	battery->charging_start_time = 0;
	battery->charging_passed_time = 0;
	battery->wc_heating_start_time = 0;
	battery->wc_heating_passed_time = 0;
	battery->charging_next_time = 0;
	battery->charging_fullcharged_time = 0;
	battery->siop_level = 100;
	battery->wc_enable = true;
	battery->wc_enable_cnt = 0;
	battery->wc_enable_cnt_value = 3;
	battery->stability_test = 0;
	battery->eng_not_full_status = 0;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
#if defined(CONFIG_STEP_CHARGING)
	battery->test_step_condition = 0x7FFF;
#endif
	battery->test_max_current = false;
	battery->test_charge_current = false;
#endif
	battery->wc_status = SEC_BATTERY_CABLE_NONE;
	battery->wc_cv_mode = false;
	battery->wire_status = SEC_BATTERY_CABLE_NONE;
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	battery->bc12_cable = SEC_BATTERY_CABLE_NONE;
#endif
#if defined(CONFIG_WIRELESS_RX_PHM_CTRL)
	battery->wc_rx_pdetb_mode = false;
#else
	battery->wc_rx_phm_mode = false;
#endif
	battery->wc_tx_phm_mode = false;
	battery->wc_tx_enable = false;
	battery->uno_en = false;
	battery->afc_disable = false;
	battery->pd_disable = false;
	battery->buck_cntl_by_tx = false;
	battery->wc_tx_vout = WC_TX_VOUT_5000MV;
	battery->wc_rx_type = NO_DEV;
	battery->tx_mfc_iout = 0;
	battery->tx_uno_iout = 0;
	battery->wc_need_ldo_on = false;
	battery->tx_minduty = battery->pdata->tx_minduty_default;
	battery->chg_limit = false;
	battery->lrp_limit = false;
	battery->lrp_step = LRP_NONE;
	battery->mix_limit = false;
	battery->usb_temp = 0;
	battery->dchg_temp = 0;
	battery->blkt_temp = 0;
	battery->lrp = 0;
	battery->lrp_test = 0;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	battery->lrp_chg_src = SEC_CHARGING_SOURCE_DIRECT;
#endif
#if IS_ENABLED(CONFIG_BATTERY_AUTH_SLE956681)
	battery->vk_key_status = -1;
#endif
	battery->skip_swelling = false;
	battery->wpc_temp_v2_offset = 0;
	battery->bat_thm_count = 0;
	battery->adc_init_count = 0;
	battery->led_cover = 0;
	battery->mag_cover = 0;
	battery->hiccup_status = 0;
	battery->hiccup_clear = false;
	battery->ext_event = BATT_EXT_EVENT_NONE;
	battery->tx_retry_case = SEC_BAT_TX_RETRY_NONE;
	battery->tx_misalign_cnt = 0;
	battery->tx_ocp_cnt = 0;
	battery->auto_mode = false;
	battery->update_pd_list = false;
	battery->charging_mode = SEC_BATTERY_CHARGING_NONE;
	battery->is_recharging = false;
	battery->cable_type = SEC_BATTERY_CABLE_NONE;
	battery->test_mode = 0;
	battery->factory_mode = false;
	battery->display_test = false;
	battery->store_mode = false;
	battery->prev_usb_conf = USB_CURRENT_NONE;
	battery->is_hc_usb = false;
	battery->is_sysovlo = false;
	battery->is_vbatovlo = false;
	battery->is_abnormal_temp = false;
	battery->hv_pdo = false;
	battery->safety_timer_set = true;
	battery->safety_timer.stop_timer = false;
	battery->safety_timer.prev_time = 0;
	battery->lcd_status = true;
	battery->wc_auth_retried = false;
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	battery->wc20_power_class = 0;
	battery->wc20_vout = 0;
	battery->wc20_rx_power = 0;
#endif
	battery->thermal_zone = BAT_THERMAL_NORMAL;
	sec_bat_set_threshold(battery, battery->cable_type);
	battery->usb_overheat_check = false;
	battery->skip_cisd = false;
	battery->batt_cycle = -1;
	battery->batt_full_status_usage = -1;
	battery->pdata->age_step = 0;
	battery->batt_asoc = 100;
	battery->health_change = false;
	battery->usb_conn_status = USB_CONN_NORMAL;
	battery->lr_bat_temp = 0x7FFF;
	battery->lr_start_time = 0;
	battery->lr_time_span = 0;
	battery->usb_slow_chg = false;
	battery->usb_bootcomplete = false;
#if IS_ENABLED(CONFIG_MTK_CHARGER)
	battery->mtk_fg_init = false;
#endif
	battery->d2d_auth = D2D_AUTH_NONE;
	battery->vpdo_src_boost = false;
	battery->vpdo_ocp = false;
	battery->vpdo_auth_stat = AUTH_NONE;
	battery->hp_d2d = HP_D2D_NONE;
	if (battery->pdata->soc_by_repcap_en) {
		battery->eoc_d->eoc_check = false;
		battery->eoc_d->eoc_cnt = 0;
	}
	battery->dc_check_cnt = 0;
	battery->srccap_transit_cnt = 0;
	battery->abnormal_ta = false;
	battery->smart_sw_src = false;
	battery->dc_ta_op_max_mode = DC_MODE_NONE;
	battery->dc_ta_forced_op_max_mode = DC_MODE_NONE;

	ttf_init(battery);
	sec_battery_cisd_init(battery);
#if defined(CONFIG_STORE_MODE) && !defined(CONFIG_SEC_FACTORY)
	battery->store_mode = true;
	sec_bat_parse_mode_dt(battery);
#endif
#if defined(CONFIG_WIRELESS_AUTH)
	sec_bat_misc_init(battery);
#endif
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	if ((battery->pdata->charging_current[SEC_BATTERY_CABLE_WIRELESS].input_current_limit <= battery->pdata->wpc_input_limit_current) ||
		(battery->pdata->charging_current[SEC_BATTERY_CABLE_WIRELESS].input_current_limit <= battery->pdata->wpc_lcd_on_input_limit_current))
		battery->nv_wc_temp_ctrl_skip = true;
	else
		battery->nv_wc_temp_ctrl_skip = false;
#endif
	if (battery->pdata->charger_name == NULL)
		battery->pdata->charger_name = "sec-charger";
	if (battery->pdata->fuelgauge_name == NULL)
		battery->pdata->fuelgauge_name = "sec-fuelgauge";
	if (battery->pdata->check_battery_callback)
		battery->present = battery->pdata->check_battery_callback();

	if (sec_bat_get_fgreset())
		sec_bat_set_current_event(battery, SEC_BAT_CURRENT_EVENT_FG_RESET,
			SEC_BAT_CURRENT_EVENT_FG_RESET);

	battery_cfg.drv_data = battery;

	/* init power supplier framework */
	battery->psy_usb = power_supply_register(&pdev->dev, &usb_power_supply_desc, &battery_cfg);
	if (IS_ERR(battery->psy_usb)) {
		ret = PTR_ERR(battery->psy_usb);
		dev_err(battery->dev,
			"%s: Failed to Register psy_usb(%d)\n", __func__, ret);
		goto err_workqueue;
	}
	battery->psy_usb->supplied_to = supply_list;
	battery->psy_usb->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_ac = power_supply_register(&pdev->dev, &ac_power_supply_desc, &battery_cfg);
	if (IS_ERR(battery->psy_ac)) {
		ret = PTR_ERR(battery->psy_ac);
		dev_err(battery->dev,
			"%s: Failed to Register psy_ac(%d)\n", __func__, ret);
		goto err_supply_unreg_usb;
	}
	battery->psy_ac->supplied_to = supply_list;
	battery->psy_ac->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_bat = power_supply_register(&pdev->dev, &battery_power_supply_desc, &battery_cfg);
	if (IS_ERR(battery->psy_bat)) {
		ret = PTR_ERR(battery->psy_bat);
		dev_err(battery->dev,
			"%s: Failed to Register psy_bat(%d)\n", __func__, ret);
		goto err_supply_unreg_ac;
	}

	battery->psy_pogo = power_supply_register(&pdev->dev, &pogo_power_supply_desc, &battery_cfg);
	if (IS_ERR(battery->psy_pogo)) {
		ret = PTR_ERR(battery->psy_pogo);
		dev_err(battery->dev,
			"%s: Failed to Register psy_pogo(%d)\n", __func__, ret);
		goto err_supply_unreg_bat;
	}

	battery->psy_wireless = power_supply_register(&pdev->dev, &wireless_power_supply_desc, &battery_cfg);
	if (IS_ERR(battery->psy_wireless)) {
		ret = PTR_ERR(battery->psy_wireless);
		dev_err(battery->dev,
			"%s: Failed to Register psy_wireless(%d)\n", __func__, ret);
		goto err_supply_unreg_pogo;
	}
	battery->psy_wireless->supplied_to = supply_list;
	battery->psy_wireless->num_supplicants = ARRAY_SIZE(supply_list);

	battery->psy_otg = power_supply_register(&pdev->dev, &otg_power_supply_desc, &battery_cfg);
	if (IS_ERR(battery->psy_otg)) {
		ret = PTR_ERR(battery->psy_otg);
		dev_err(battery->dev,
			"%s: Failed to Register psy_otg(%d)\n", __func__, ret);
		goto err_supply_unreg_wireless;
	}

	if (device_create_file(&battery->psy_wireless->dev, &dev_attr_sgf))
		dev_err(battery->dev,
			"%s: failed to create sgf attr\n", __func__);

	ret = sec_bat_create_attrs(&battery->psy_bat->dev);
	if (ret) {
		dev_err(battery->dev,
			"%s : Failed to sec_bat_create_attrs\n", __func__);
		goto err_req_irq;
	}

	ret = sec_pogo_create_attrs(&battery->psy_pogo->dev);
	if (ret) {
		dev_err(battery->dev,
			"%s : Failed to sec_pogo_create_attrs\n", __func__);
		goto err_req_irq;
	}

	ret = sec_otg_create_attrs(&battery->psy_otg->dev);
	if (ret) {
		dev_err(battery->dev,
			"%s : Failed to sec_otg_create_attrs\n", __func__);
		goto err_req_irq;
	}

	ret = sb_full_soc_init(battery);
	dev_info(battery->dev, "%s: sb_full_soc (%s)\n", __func__, (ret) ? "fail" : "success");

	ret = sb_notify_register(&battery->sb_nb, sb_handle_notification, "battery", SB_DEV_BATTERY);
	dev_info(battery->dev, "%s: sb_notify_register (%s)\n", __func__, (ret) ? "fail" : "success");

	sb_ca_init(battery->dev);
	sb_bd_init();

	__pm_stay_awake(battery->dev_init_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->dev_init_work, 0);

	pr_info("%s: SEC Battery Driver Loaded\n", __func__);
	return 0;

err_req_irq:
	power_supply_unregister(battery->psy_otg);
err_supply_unreg_wireless:
	power_supply_unregister(battery->psy_wireless);
err_supply_unreg_pogo:
	power_supply_unregister(battery->psy_pogo);
err_supply_unreg_bat:
	power_supply_unregister(battery->psy_bat);
err_supply_unreg_ac:
	power_supply_unregister(battery->psy_ac);
err_supply_unreg_usb:
	power_supply_unregister(battery->psy_usb);
err_workqueue:
	destroy_workqueue(battery->monitor_wqueue);
err_irq:
	wakeup_source_unregister(battery->monitor_ws);
	wakeup_source_unregister(battery->cable_ws);
	wakeup_source_unregister(battery->vbus_ws);
	wakeup_source_unregister(battery->input_ws);
	wakeup_source_unregister(battery->siop_level_ws);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	wakeup_source_unregister(battery->ext_event_ws);
	wakeup_source_unregister(battery->wpc_tx_ws);
	wakeup_source_unregister(battery->wpc_tx_en_ws);
	wakeup_source_unregister(battery->tx_event_ws);
	wakeup_source_unregister(battery->wc20_current_ws);
	wakeup_source_unregister(battery->mpla_thr_cnt_check_ws);
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wakeup_source_unregister(battery->batt_data_ws);
#endif
	wakeup_source_unregister(battery->misc_event_ws);
	wakeup_source_unregister(battery->parse_mode_dt_ws);
	wakeup_source_unregister(battery->dev_init_ws);
	wakeup_source_unregister(battery->usb_conn_check_ws);
	wakeup_source_unregister(battery->transit_clear_ws);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->txeventlock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);
	mutex_destroy(&battery->typec_notylock);
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	mutex_destroy(&battery->bc12_notylock);
#endif
	mutex_destroy(&battery->wclock);
	mutex_destroy(&battery->voutlock);
err_pdata_free:
	if (battery->pdata->soc_by_repcap_en)
		kfree(pdata);
err_bat_free:
	kfree(battery);

	return ret;
}

static int sec_battery_remove(struct platform_device *pdev)
{
	struct sec_battery_info *battery = platform_get_drvdata(pdev);

	pr_info("%s: ++\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}
	unregister_batterylog_proc();
	flush_workqueue(battery->monitor_wqueue);
	destroy_workqueue(battery->monitor_wqueue);
	wakeup_source_unregister(battery->monitor_ws);
	wakeup_source_unregister(battery->cable_ws);
	wakeup_source_unregister(battery->vbus_ws);
	wakeup_source_unregister(battery->input_ws);
	wakeup_source_unregister(battery->siop_level_ws);
	wakeup_source_unregister(battery->misc_event_ws);
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	wakeup_source_unregister(battery->ext_event_ws);
	wakeup_source_unregister(battery->wpc_tx_ws);
	wakeup_source_unregister(battery->wpc_tx_en_ws);
	wakeup_source_unregister(battery->tx_event_ws);
	wakeup_source_unregister(battery->wc20_current_ws);
	wakeup_source_unregister(battery->mpla_thr_cnt_check_ws);
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	wakeup_source_unregister(battery->batt_data_ws);
#endif
	wakeup_source_unregister(battery->parse_mode_dt_ws);
	wakeup_source_unregister(battery->dev_init_ws);
	wakeup_source_unregister(battery->usb_conn_check_ws);
	wakeup_source_unregister(battery->transit_clear_ws);
	mutex_destroy(&battery->iolock);
	mutex_destroy(&battery->misclock);
	mutex_destroy(&battery->txeventlock);
	mutex_destroy(&battery->batt_handlelock);
	mutex_destroy(&battery->current_eventlock);
	mutex_destroy(&battery->typec_notylock);
#if IS_ENABLED(CONFIG_MTK_CHARGER)  && !IS_ENABLED(CONFIG_VIRTUAL_MUIC)
	mutex_destroy(&battery->bc12_notylock);
#endif
	mutex_destroy(&battery->wclock);
	mutex_destroy(&battery->voutlock);

	adc_exit(battery);

	power_supply_unregister(battery->psy_otg);
	power_supply_unregister(battery->psy_wireless);
	power_supply_unregister(battery->psy_pogo);
	power_supply_unregister(battery->psy_ac);
	power_supply_unregister(battery->psy_usb);
	power_supply_unregister(battery->psy_bat);

	kfree(battery);

	pr_info("%s: --\n", __func__);

	return 0;
}

static int sec_battery_prepare(struct device *dev)
{
	struct sec_battery_info *battery
		= dev_get_drvdata(dev);

	dev_info(battery->dev, "%s: Start\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	/* monitor_ws should be unlocked before cancel monitor_work */
	__pm_relax(battery->monitor_ws);
	cancel_delayed_work_sync(&battery->monitor_work);

	battery->polling_in_sleep = true;

	sec_bat_set_polling(battery);

	/*
	 * cancel work for polling
	 * that is set in sec_bat_set_polling()
	 * no need for polling in sleep
	 */
	if (battery->pdata->polling_type ==
		SEC_BATTERY_MONITOR_WORKQUEUE)
		cancel_delayed_work(&battery->polling_work);

	dev_info(battery->dev, "%s: End\n", __func__);

	return 0;
}

static int sec_battery_suspend(struct device *dev)
{
	return 0;
}

static int sec_battery_resume(struct device *dev)
{
	return 0;
}

static void sec_battery_complete(struct device *dev)
{
	struct sec_battery_info *battery
		= dev_get_drvdata(dev);

	dev_info(battery->dev, "%s: Start\n", __func__);

	/* cancel current alarm and reset after monitor work */
	if (battery->pdata->polling_type == SEC_BATTERY_MONITOR_ALARM)
		alarm_cancel(&battery->polling_alarm);

	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue,
		&battery->monitor_work, 0);

	dev_info(battery->dev, "%s: End\n", __func__);

	return;
}

static void sec_battery_shutdown(struct platform_device *pdev)
{
	struct sec_battery_info *battery = platform_get_drvdata(pdev);

	pr_info("%s: ++\n", __func__);

	switch (battery->pdata->polling_type) {
	case SEC_BATTERY_MONITOR_WORKQUEUE:
		cancel_delayed_work(&battery->polling_work);
		break;
	case SEC_BATTERY_MONITOR_ALARM:
		alarm_cancel(&battery->polling_alarm);
		break;
	default:
		break;
	}

	cancel_delayed_work(&battery->monitor_work);
	cancel_delayed_work(&battery->cable_work);
	cancel_delayed_work(&battery->slowcharging_work);
	cancel_delayed_work(&battery->input_check_work);
	cancel_delayed_work(&battery->siop_level_work);
#if defined(CONFIG_WIRELESS_FIRMWARE_UPDATE)
	cancel_delayed_work(&battery->fw_init_work);
#endif
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	cancel_delayed_work(&battery->wpc_tx_work);
	cancel_delayed_work(&battery->wpc_tx_en_work);
	cancel_delayed_work(&battery->wpc_txpower_calc_work);
	cancel_delayed_work(&battery->ext_event_work);
	cancel_delayed_work(&battery->wc20_current_work);
	cancel_delayed_work(&battery->mpla_thr_cnt_check_work);
#endif
#if defined(CONFIG_UPDATE_BATTERY_DATA)
	cancel_delayed_work(&battery->batt_data_work);
#endif
	cancel_delayed_work(&battery->misc_event_work);
	cancel_delayed_work(&battery->parse_mode_dt_work);
	cancel_delayed_work(&battery->dev_init_work);
	cancel_delayed_work(&battery->afc_init_work);
	cancel_delayed_work(&battery->usb_conn_check_work);
	cancel_delayed_work(&battery->transit_clear_work);

	pr_info("%s: --\n", __func__);
}

#ifdef CONFIG_OF
static struct of_device_id sec_battery_dt_ids[] = {
	{ .compatible = "samsung,sec-battery" },
	{ }
};
MODULE_DEVICE_TABLE(of, sec_battery_dt_ids);
#endif /* CONFIG_OF */

static const struct dev_pm_ops sec_battery_pm_ops = {
	.prepare = sec_battery_prepare,
	.suspend = sec_battery_suspend,
	.resume = sec_battery_resume,
	.complete = sec_battery_complete,
};

static struct platform_driver sec_battery_driver = {
	.driver = {
		.name = "sec-battery",
		.owner = THIS_MODULE,
		.pm = &sec_battery_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = sec_battery_dt_ids,
#endif
	},
	.probe = sec_battery_probe,
	.remove = sec_battery_remove,
	.shutdown = sec_battery_shutdown,
};

static int __init sec_battery_init(void)
{
	sec_chg_init_gdev();
	return platform_driver_register(&sec_battery_driver);
}

static void __exit sec_battery_exit(void)
{
	platform_driver_unregister(&sec_battery_driver);
}

late_initcall(sec_battery_init);
module_exit(sec_battery_exit);

MODULE_DESCRIPTION("Samsung Battery Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
