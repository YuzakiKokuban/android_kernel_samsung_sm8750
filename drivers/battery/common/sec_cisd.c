/*
 *  sec_cisd.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "sec_battery.h"
#include "sec_cisd.h"

#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

const char *cisd_data_str[] = {
	"RESET_ALG", "ALG_INDEX", "FULL_CNT", "CAP_MAX", "CAP_MIN", "RECHARGING_CNT", "VALERT_CNT",
	"BATT_CYCLE", "WIRE_CNT", "WIRELESS_CNT", "HIGH_SWELLING_CNT", "LOW_SWELLING_CNT",
	"WC_HIGH_SWELLING_CNT", "SWELLING_FULL_CNT", "SWELLING_RECOVERY_CNT", "AICL_CNT", "BATT_THM_MAX",
	"BATT_THM_MIN", "CHG_THM_MAX", "CHG_THM_MIN", "WPC_THM_MAX", "WPC_THM_MIN", "USB_THM_MAX", "USB_THM_MIN",
	"CHG_BATT_THM_MAX", "CHG_BATT_THM_MIN", "CHG_CHG_THM_MAX", "CHG_CHG_THM_MIN", "CHG_WPC_THM_MAX",
	"CHG_WPC_THM_MIN", "CHG_USB_THM_MAX", "CHG_USB_THM_MIN", "USB_OVERHEAT_CHARGING", "UNSAFETY_VOLT",
	"UNSAFETY_TEMP", "SAFETY_TIMER", "VSYS_OVP", "VBAT_OVP", "USB_OVERHEAT_RAPID_CHANGE", "ASOC",
	"USB_OVERHEAT_ALONE", "CAP_NOM", "RC0"
};
EXPORT_SYMBOL(cisd_data_str);

const char *cisd_data_str_d[] = {
	"FULL_CNT_D", "CAP_MAX_D", "CAP_MIN_D", "RECHARGING_CNT_D", "VALERT_CNT_D", "WIRE_CNT_D", "WIRELESS_CNT_D",
	"HIGH_SWELLING_CNT_D", "LOW_SWELLING_CNT_D", "WC_HIGH_SWELLING_CNT_D", "SWELLING_FULL_CNT_D",
	"SWELLING_RECOVERY_CNT_D", "AICL_CNT_D", "BATT_THM_MAX_D", "BATT_THM_MIN_D", "SUB_BATT_THM_MAX_D",
	"SUB_BATT_THM_MIN_D", "CHG_THM_MAX_D", "CHG_THM_MIN_D", "USB_THM_MAX_D", "USB_THM_MIN_D",
	"CHG_BATT_THM_MAX_D", "CHG_BATT_THM_MIN_D", "CHG_SUB_BATT_THM_MAX_D", "CHG_SUB_BATT_THM_MIN_D",
	"CHG_CHG_THM_MAX_D", "CHG_CHG_THM_MIN_D", "CHG_USB_THM_MAX_D", "CHG_USB_THM_MIN_D",
	"USB_OVERHEAT_CHARGING_D", "UNSAFETY_VOLT_D", "UNSAFETY_TEMP_D",
	"SAFETY_TIMER_D", "VSYS_OVP_D", "VBAT_OVP_D", "USB_OVERHEAT_RAPID_CHANGE_D", "BUCK_OFF_D",
	"USB_OVERHEAT_ALONE_D", "DROP_SENSOR_D", "CHG_TIME_D", "TOTAL_CHG_TIME_D"
};
EXPORT_SYMBOL(cisd_data_str_d);

const char *cisd_cable_data_str[] = {"TA", "AFC", "AFC_FAIL", "QC", "QC_FAIL", "PD", "PD_HIGH", "HV_WC_20", "FPDO_DC"};
EXPORT_SYMBOL(cisd_cable_data_str);
const char *cisd_tx_data_str[] = {"ON", "OTHER", "GEAR", "PHONE", "BUDS"};
EXPORT_SYMBOL(cisd_tx_data_str);
#if IS_ENABLED(CONFIG_DUAL_BATTERY)
const char *cisd_event_data_str[] = {"DC_ERR", "TA_OCP_DET", "TA_OCP_ON", "OVP_EVENT_POWER", "OVP_EVENT_SIGNAL", "OTG", "D2D", "MAIN_BAT_ERR", "SUB_BAT_ERR", "WA_ERR"};
#else
const char *cisd_event_data_str[] = {"DC_ERR", "TA_OCP_DET", "TA_OCP_ON", "OVP_EVENT_POWER", "OVP_EVENT_SIGNAL", "OTG", "D2D"};
#endif
EXPORT_SYMBOL(cisd_event_data_str);

bool sec_bat_cisd_check(struct sec_battery_info *battery)
{
	union power_supply_propval val = {0, };
	struct cisd *pcisd = &battery->cisd;
	bool ret = false;
	int voltage = battery->voltage_now;

	if (battery->factory_mode || battery->is_jig_on || battery->skip_cisd) {
		dev_info(battery->dev, "%s: No need to check in factory mode\n",
			__func__);
		return ret;
	}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	voltage = max(battery->voltage_now_main, battery->voltage_now_sub);
#endif

	if ((battery->status == POWER_SUPPLY_STATUS_CHARGING) ||
		(battery->status == POWER_SUPPLY_STATUS_FULL)) {

		/* check abnormal vbat */
		pcisd->ab_vbat_check_count = voltage > pcisd->max_voltage_thr ?
				pcisd->ab_vbat_check_count + 1 : 0;

		if ((pcisd->ab_vbat_check_count >= pcisd->ab_vbat_max_count) &&
			!(pcisd->state & CISD_STATE_OVER_VOLTAGE)) {
			dev_info(battery->dev, "%s : [CISD] Battery Over Voltage Protection !! vbat(%d)mV\n",
				__func__, voltage);
			val.intval = POWER_SUPPLY_EXT_HEALTH_VBAT_OVP;
			psy_do_property("battery", set, POWER_SUPPLY_PROP_HEALTH,
					val);
			pcisd->data[CISD_DATA_VBAT_OVP]++;
			pcisd->data[CISD_DATA_VBAT_OVP_PER_DAY]++;
			pcisd->state |= CISD_STATE_OVER_VOLTAGE;
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
			sec_abc_send_event("MODULE=battery@INFO=over_voltage");
#else
			sec_abc_send_event("MODULE=battery@WARN=over_voltage");
#endif
#endif
		}

		if (battery->temperature > pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX] = battery->temperature;
		if (battery->temperature < pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN] = battery->temperature;

		if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX] = battery->chg_temp;
		if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN] = battery->chg_temp;

		if (battery->wpc_temp > pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MAX] = battery->wpc_temp;
		if (battery->wpc_temp < pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_WPC_TEMP_MIN] = battery->wpc_temp;

		if (battery->usb_temp > pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX] = battery->usb_temp;
		if (battery->usb_temp < pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN] = battery->usb_temp;

		if (battery->temperature > pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = battery->temperature;
		if (battery->temperature < pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = battery->temperature;

		if (battery->sub_bat_temp > pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MAX_PER_DAY] = battery->sub_bat_temp;
		if (battery->sub_bat_temp < pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_SUB_BATT_TEMP_MIN_PER_DAY] = battery->sub_bat_temp;

		if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = battery->chg_temp;
		if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = battery->chg_temp;

		if (battery->usb_temp > pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = battery->usb_temp;
		if (battery->usb_temp < pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = battery->usb_temp;

		if (battery->usb_temp > 800 && !battery->usb_overheat_check) {
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_CHARGING]++;
			battery->cisd.data[CISD_DATA_USB_OVERHEAT_CHARGING_PER_DAY]++;
			battery->usb_overheat_check = true;
		}
	} else  {
		/* discharging */
		if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING) {
			/* check abnormal vbat */
			pcisd->ab_vbat_check_count = voltage > pcisd->max_voltage_thr ?
				pcisd->ab_vbat_check_count + 1 : 0;

			if ((pcisd->ab_vbat_check_count >= pcisd->ab_vbat_max_count) &&
				!(pcisd->state & CISD_STATE_OVER_VOLTAGE)) {
				pcisd->data[CISD_DATA_VBAT_OVP]++;
				pcisd->data[CISD_DATA_VBAT_OVP_PER_DAY]++;
				pcisd->state |= CISD_STATE_OVER_VOLTAGE;
#if IS_ENABLED(CONFIG_SEC_ABC)
#if IS_ENABLED(CONFIG_SEC_FACTORY)
				sec_abc_send_event("MODULE=battery@INFO=over_voltage");
#else
				sec_abc_send_event("MODULE=battery@WARN=over_voltage");
#endif
#endif
			}
		}

		val.intval = SEC_BATTERY_CAPACITY_FULL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
		if (val.intval > pcisd->data[CISD_DATA_CAP_MAX])
			pcisd->data[CISD_DATA_CAP_MAX] = val.intval;
		if (val.intval < pcisd->data[CISD_DATA_CAP_MIN])
			pcisd->data[CISD_DATA_CAP_MIN] = val.intval;

		if (val.intval > pcisd->data[CISD_DATA_CAP_MAX_PER_DAY])
			pcisd->data[CISD_DATA_CAP_MAX_PER_DAY] = val.intval;
		if (val.intval < pcisd->data[CISD_DATA_CAP_MIN_PER_DAY])
			pcisd->data[CISD_DATA_CAP_MIN_PER_DAY] = val.intval;

		val.intval = SEC_BATTERY_CAPACITY_AGEDCELL;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
		pcisd->data[CISD_DATA_CAP_NOM] = val.intval;
		dev_info(battery->dev, "%s: [CISD] CAP_NOM %dmAh\n", __func__, pcisd->data[CISD_DATA_CAP_NOM]);

		val.intval = SEC_BATTERY_CAPACITY_RC0;
		psy_do_property(battery->pdata->fuelgauge_name, get,
			POWER_SUPPLY_PROP_ENERGY_NOW, val);
		pcisd->data[CISD_DATA_RC0] = val.intval;
		dev_info(battery->dev, "%s: [CISD] RC0 0x%x\n", __func__, pcisd->data[CISD_DATA_RC0]);
	}

	if (battery->temperature > pcisd->data[CISD_DATA_BATT_TEMP_MAX])
		pcisd->data[CISD_DATA_BATT_TEMP_MAX] = battery->temperature;
	if (battery->temperature < battery->cisd.data[CISD_DATA_BATT_TEMP_MIN])
		pcisd->data[CISD_DATA_BATT_TEMP_MIN] = battery->temperature;

	if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_TEMP_MAX])
		pcisd->data[CISD_DATA_CHG_TEMP_MAX] = battery->chg_temp;
	if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_TEMP_MIN])
		pcisd->data[CISD_DATA_CHG_TEMP_MIN] = battery->chg_temp;

	if (battery->wpc_temp > pcisd->data[CISD_DATA_WPC_TEMP_MAX])
		pcisd->data[CISD_DATA_WPC_TEMP_MAX] = battery->wpc_temp;
	if (battery->wpc_temp < battery->cisd.data[CISD_DATA_WPC_TEMP_MIN])
		pcisd->data[CISD_DATA_WPC_TEMP_MIN] = battery->wpc_temp;

	if (battery->usb_temp > pcisd->data[CISD_DATA_USB_TEMP_MAX])
		pcisd->data[CISD_DATA_USB_TEMP_MAX] = battery->usb_temp;
	if (battery->usb_temp < pcisd->data[CISD_DATA_USB_TEMP_MIN])
		pcisd->data[CISD_DATA_USB_TEMP_MIN] = battery->usb_temp;

	if (battery->temperature > pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = battery->temperature;
	if (battery->temperature < pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = battery->temperature;

	if (battery->sub_bat_temp > pcisd->data[CISD_DATA_SUB_BATT_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_SUB_BATT_TEMP_MAX_PER_DAY] = battery->sub_bat_temp;
	if (battery->sub_bat_temp < pcisd->data[CISD_DATA_SUB_BATT_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_SUB_BATT_TEMP_MIN_PER_DAY] = battery->sub_bat_temp;

	if (battery->chg_temp > pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = battery->chg_temp;
	if (battery->chg_temp < pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = battery->chg_temp;

	if (battery->usb_temp > pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY])
		pcisd->data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = battery->usb_temp;
	if (battery->usb_temp < pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY])
		pcisd->data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = battery->usb_temp;

	return ret;
}
EXPORT_SYMBOL(sec_bat_cisd_check);

static irqreturn_t cisd_irq_thread(int irq, void *data)
{
	struct cisd *pcisd = data;

	pr_info("%s: irq(%d)\n", __func__, irq);
	if (irq == pcisd->irq_ovp_power &&
		!gpio_get_value(pcisd->gpio_ovp_power))
		pcisd->event_data[EVENT_OVP_POWER]++;

	if (irq == pcisd->irq_ovp_signal &&
		!gpio_get_value(pcisd->gpio_ovp_signal))
		pcisd->event_data[EVENT_OVP_SIGNAL]++;

	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static void sec_cisd_parse_dt(struct cisd *pcisd)
{
	struct device_node *np;
	int ret = 0;

	np = of_find_node_by_name(NULL, "sec-cisd");
	if (!np) {
		pr_err("%s: np NULL\n", __func__);
		return;
	}

	ret = of_get_named_gpio(np, "ovp_power", 0);
	if (ret >= 0) {
		pcisd->gpio_ovp_power = ret;
		pr_info("%s: set ovp_power gpio(%d)\n", __func__, pcisd->gpio_ovp_power);
		pcisd->irq_ovp_power = gpio_to_irq(pcisd->gpio_ovp_power);
		ret = request_threaded_irq(pcisd->irq_ovp_power, NULL,
			cisd_irq_thread, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"cisd-ovp-power", pcisd);
		if (ret < 0) {
			pr_err("%s: failed to request ovp_power irq(ret = %d)\n",
				__func__, ret);
			pcisd->irq_ovp_power = 0;
		} else
			pr_info("%s: set irq_ovp_power(%d)\n", __func__, pcisd->irq_ovp_power);
	} else
		pr_err("%s: failed to get ovp_power\n", __func__);

	ret = of_get_named_gpio(np, "ovp_signal", 0);
	if (ret >= 0) {
		pcisd->gpio_ovp_signal = ret;
		pr_info("%s: set ovp_signal gpio(%d)\n", __func__, pcisd->gpio_ovp_signal);
		pcisd->irq_ovp_signal = gpio_to_irq(pcisd->gpio_ovp_signal);
		ret = request_threaded_irq(pcisd->irq_ovp_signal, NULL,
			cisd_irq_thread, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"cisd-ovp-signal", pcisd);
		if (ret < 0) {
			pr_err("%s: failed to request ovp_signal irq(ret = %d)\n",
				__func__, ret);
			pcisd->irq_ovp_signal = 0;
		} else
			pr_info("%s: set irq_ovp_signal(%d)\n", __func__, pcisd->irq_ovp_signal);
	} else
		pr_err("%s: failed to get ovp_signal\n", __func__);
}
#else
static void sec_cisd_parse_dt(struct cisd *pcisd)
{
}
#endif

struct cisd *gcisd;
EXPORT_SYMBOL(gcisd);
void sec_battery_cisd_init(struct sec_battery_info *battery)
{
	/* parse dt */
	sec_cisd_parse_dt(&battery->cisd);

	/* init cisd data */
	battery->cisd.state = CISD_STATE_NONE;

	battery->cisd.data[CISD_DATA_ALG_INDEX] = battery->pdata->cisd_alg_index;
	battery->cisd.data[CISD_DATA_FULL_COUNT] = 1;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_WPC_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_USB_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_WPC_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_USB_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_CHG_CHG_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_CHG_WPC_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_CHG_USB_TEMP_MAX] = -300;
	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CHG_CHG_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CHG_WPC_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CHG_USB_TEMP_MIN] = 1000;
	battery->cisd.data[CISD_DATA_CAP_MIN] = 0xFFFF;
	battery->cisd.data[CISD_DATA_ASOC] = 100;

	battery->cisd.data[CISD_DATA_FULL_COUNT_PER_DAY] = 1;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_USB_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_USB_TEMP_MIN_PER_DAY] = 1000;

	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_SUB_BATT_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_CHG_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_USB_TEMP_MAX_PER_DAY] = -300;
	battery->cisd.data[CISD_DATA_CHG_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_SUB_BATT_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_CHG_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_USB_TEMP_MIN_PER_DAY] = 1000;
	battery->cisd.data[CISD_DATA_CHG_RETENTION_TIME_PER_DAY] = 0;
	battery->cisd.data[CISD_DATA_TOTAL_CHG_RETENTION_TIME_PER_DAY] = 0;

	battery->cisd.ab_vbat_max_count = 2; /* should be 2 */
	battery->cisd.ab_vbat_check_count = 0;
	battery->cisd.max_voltage_thr = battery->pdata->max_voltage_thr;

	/* set cisd pointer */
	gcisd = &battery->cisd;

	/* initialize pad data */
	mutex_init(&battery->cisd.padlock);
	mutex_init(&battery->cisd.powerlock);
	mutex_init(&battery->cisd.pdlock);
	mutex_init(&battery->cisd.dcerrlock);
	init_cisd_pad_data(&battery->cisd);
	init_cisd_power_data(&battery->cisd);
	init_cisd_pd_data(&battery->cisd);
	init_cisd_dcerr_data(&battery->cisd);
}
EXPORT_SYMBOL(sec_battery_cisd_init);

static struct pad_data *create_pad_data(unsigned int pad_id, unsigned int pad_count)
{
	struct pad_data *temp_data;

	temp_data = kzalloc(sizeof(struct pad_data), GFP_KERNEL);
	if (temp_data == NULL)
		return NULL;

	temp_data->id = pad_id;
	temp_data->count = pad_count;
	temp_data->prev = temp_data->next = NULL;

	return temp_data;
}

static struct pad_data *find_pad_data_by_id(struct cisd *cisd, unsigned int pad_id)
{
	struct pad_data *temp_data = cisd->pad_array->next;

	if (cisd->pad_count <= 0 || temp_data == NULL)
		return NULL;

	while ((temp_data->id != pad_id) &&
		((temp_data = temp_data->next) != NULL));

	return temp_data;
}

static void add_pad_data(struct cisd *cisd, unsigned int pad_id, unsigned int pad_count)
{
	struct pad_data *temp_data = cisd->pad_array->next;
	struct pad_data *pad_data;

	if (pad_id >= MAX_PAD_ID)
		return;

	pad_data = create_pad_data(pad_id, pad_count);
	if (pad_data == NULL)
		return;

	pr_info("%s: id(0x%x), count(%d)\n", __func__, pad_id, pad_count);
	while (temp_data) {
		if (temp_data->id > pad_id) {
			temp_data->prev->next = pad_data;
			pad_data->prev = temp_data->prev;
			pad_data->next = temp_data;
			temp_data->prev = pad_data;
			cisd->pad_count++;
			return;
		}
		temp_data = temp_data->next;
	}

	pr_info("%s: failed to add pad_data(%d, %d)\n",
		__func__, pad_id, pad_count);
	kfree(pad_data);
}

void init_cisd_pad_data(struct cisd *cisd)
{
	struct pad_data *temp_data = NULL;

	mutex_lock(&cisd->padlock);
	temp_data = cisd->pad_array;
	while (temp_data) {
		struct pad_data *next_data = temp_data->next;

		kfree(temp_data);
		temp_data = next_data;
	}

	/* create dummy data */
	cisd->pad_array = create_pad_data(0, 0);
	if (cisd->pad_array == NULL)
		goto err_create_dummy_data;
	temp_data = create_pad_data(MAX_PAD_ID, 0);
	if (temp_data == NULL) {
		kfree(cisd->pad_array);
		cisd->pad_array = NULL;
		goto err_create_dummy_data;
	}
	cisd->pad_count = 0;
	cisd->pad_array->next = temp_data;
	temp_data->prev = cisd->pad_array;

err_create_dummy_data:
	mutex_unlock(&cisd->padlock);
}
EXPORT_SYMBOL(init_cisd_pad_data);

void count_cisd_pad_data(struct cisd *cisd, unsigned int pad_id)
{
	struct pad_data *pad_data;

	if (cisd->pad_array == NULL) {
		pr_info("%s: can't update the connected count of pad_id(0x%x) because of null\n",
			__func__, pad_id);
		return;
	}

	mutex_lock(&cisd->padlock);
	pad_data = find_pad_data_by_id(cisd, pad_id);
	if (pad_data != NULL)
		pad_data->count++;
	else
		add_pad_data(cisd, pad_id, 1);
	mutex_unlock(&cisd->padlock);
}
EXPORT_SYMBOL(count_cisd_pad_data);

void set_cisd_pad_data(struct sec_battery_info *battery, const char *buf)
{
	struct cisd *pcisd = &battery->cisd;
	unsigned int pad_total_count, pad_id, pad_count;
	struct pad_data *pad_data;
	int i, x;

	pr_info("%s: %s\n", __func__, buf);
	if (pcisd->pad_count > 0)
		init_cisd_pad_data(pcisd);

	if (pcisd->pad_array == NULL) {
		pr_info("%s: can't set the pad data because of null\n", __func__);
		return;
	}

	if (sscanf(buf, "%10u %n", &pad_total_count, &x) <= 0) {
		pr_info("%s: failed to read pad index\n", __func__);
		return;
	}
	buf += (size_t)x;
	pr_info("%s: add pad data(count: %d)\n", __func__, pad_total_count);
	for (i = 0; i < pad_total_count; i++) {
		if (sscanf(buf, "0x%02x:%10u %n", &pad_id, &pad_count, &x) != 2) {
			pr_info("%s: failed to read pad data(0x%x, %d, %d)!!!re-init pad data\n",
				__func__, pad_id, pad_count, x);
			init_cisd_pad_data(pcisd);
			break;
		}
		buf += (size_t)x;
		mutex_lock(&pcisd->padlock);
		pad_data = find_pad_data_by_id(pcisd, pad_id);
		if (pad_data != NULL)
			pad_data->count = pad_count;
		else
			add_pad_data(pcisd, pad_id, pad_count);
		mutex_unlock(&pcisd->padlock);
	}
}
EXPORT_SYMBOL(set_cisd_pad_data);

static struct power_data *create_power_data(unsigned int power, unsigned int power_count)
{
	struct power_data *temp_data;

	temp_data = kzalloc(sizeof(struct power_data), GFP_KERNEL);
	if (temp_data == NULL)
		return NULL;

	temp_data->power = power;
	temp_data->count = power_count;
	temp_data->prev = temp_data->next = NULL;

	return temp_data;
}

static struct power_data *find_data_by_power(struct cisd *cisd, unsigned int power)
{
	struct power_data *temp_data = cisd->power_array->next;

	if (cisd->power_count <= 0 || temp_data == NULL)
		return NULL;

	while ((temp_data->power != power) &&
		((temp_data = temp_data->next) != NULL));

	return temp_data;
}

static void add_power_data(struct cisd *cisd, unsigned int power, unsigned int power_count)
{
	struct power_data *temp_data = cisd->power_array->next;
	struct power_data *power_data;

	power_data = create_power_data(power, power_count);
	if (power_data == NULL)
		return;

	pr_info("%s: power(%d), count(%d)\n", __func__, power, power_count);
	while (temp_data) {
		if (temp_data->power > power) {
			temp_data->prev->next = power_data;
			power_data->prev = temp_data->prev;
			power_data->next = temp_data;
			temp_data->prev = power_data;
			cisd->power_count++;
			return;
		}
		temp_data = temp_data->next;
	}

	pr_info("%s: failed to add pad_data(%d, %d)\n",
		__func__, power, power_count);
	kfree(power_data);
}

void init_cisd_power_data(struct cisd *cisd)
{
	struct power_data *temp_data = NULL;

	mutex_lock(&cisd->powerlock);
	temp_data = cisd->power_array;
	while (temp_data) {
		struct power_data *next_data = temp_data->next;

		kfree(temp_data);
		temp_data = next_data;
	}

	/* create dummy data */
	cisd->power_array = create_power_data(0, 0);
	if (cisd->power_array == NULL)
		goto err_create_dummy_data;
	temp_data = create_power_data(MAX_CHARGER_POWER, 0);
	if (temp_data == NULL) {
		kfree(cisd->power_array);
		cisd->power_array = NULL;
		goto err_create_dummy_data;
	}
	cisd->power_count = 0;
	cisd->power_array->next = temp_data;
	temp_data->prev = cisd->power_array;

err_create_dummy_data:
	mutex_unlock(&cisd->powerlock);
}
EXPORT_SYMBOL(init_cisd_power_data);

#define FIND_MAX_POWER 45000
#define FIND_POWER_STEP 10000
#define POWER_MARGIN 1000
void count_cisd_power_data(struct cisd *cisd, int power)
{
	struct power_data *power_data;
	int power_index = 0;

	pr_info("%s: power value : %d\n", __func__, power);
	if (cisd->power_array == NULL || power < 15000) {
		pr_info("%s: can't update the connected count of power(%d) because of null\n",
			__func__, power);
		return;
	}

	power_index = FIND_MAX_POWER;
	while (power_index >= 14000) {
		if (power + POWER_MARGIN - power_index >= 0) {
			power_index /= 1000;
			break;
		}

		power_index -= FIND_POWER_STEP;
	}

	mutex_lock(&cisd->powerlock);
	power_data = find_data_by_power(cisd, power_index);
	if (power_data != NULL)
		power_data->count++;
	else
		add_power_data(cisd, power_index, 1);
	mutex_unlock(&cisd->powerlock);
}
EXPORT_SYMBOL(count_cisd_power_data);

void set_cisd_power_data(struct sec_battery_info *battery, const char *buf)
{
	struct cisd *pcisd = &battery->cisd;
	unsigned int power_total_count, power_id, power_count;
	struct power_data *power_data;
	int i, x;

	pr_info("%s: %s\n", __func__, buf);
	if (pcisd->power_count > 0)
		init_cisd_power_data(pcisd);

	if (pcisd->power_array == NULL) {
		pr_info("%s: can't set the power data because of null\n", __func__);
		return;
	}

	if (sscanf(buf, "%10u %n", &power_total_count, &x) <= 0)
		return;

	buf += (size_t)x;
	pr_info("%s: add power data(count: %d)\n", __func__, power_total_count);
	for (i = 0; i < power_total_count; i++) {
		if (sscanf(buf, "%10u:%10u %n", &power_id, &power_count, &x) != 2) {
			pr_info("%s: failed to read power data(%d, %d, %d)!!!re-init power data\n",
				__func__, power_id, power_count, x);
			init_cisd_power_data(pcisd);
			break;
		}
		buf += (size_t)x;
		mutex_lock(&pcisd->powerlock);
		power_data = find_data_by_power(pcisd, power_id);
		if (power_data != NULL)
			power_data->count = power_count;
		else
			add_power_data(pcisd, power_id, power_count);
		mutex_unlock(&pcisd->powerlock);
	}
}
EXPORT_SYMBOL(set_cisd_power_data);

static struct pd_data *create_pd_data(unsigned short pid, unsigned int pd_count)
{
	struct pd_data *temp_data;

	temp_data = kzalloc(sizeof(struct pd_data), GFP_KERNEL);
	if (temp_data == NULL)
		return NULL;

	temp_data->pid = pid;
	temp_data->count = pd_count;
	temp_data->prev = temp_data->next = NULL;

	return temp_data;
}

static struct pd_data *find_data_by_pd(struct cisd *cisd, unsigned short pid)
{
	struct pd_data *temp_data = cisd->pd_array->next;

	if (cisd->pd_count <= 0 || temp_data == NULL)
		return NULL;

	while ((temp_data->pid != pid) &&
		((temp_data = temp_data->next) != NULL))
		;

	return temp_data;
}

static void add_pd_data(struct cisd *cisd, unsigned short pid, unsigned int pd_count)
{
	struct pd_data *temp_data = cisd->pd_array->next;
	struct pd_data *pd_data;

	pd_data = create_pd_data(pid, pd_count);
	if (pd_data == NULL)
		return;

	pr_info("%s: pid(0x%04x), count(%d)\n", __func__, pid, pd_count);
	while (temp_data) {
		if (temp_data->pid > pid) {
			temp_data->prev->next = pd_data;
			pd_data->prev = temp_data->prev;
			pd_data->next = temp_data;
			temp_data->prev = pd_data;
			cisd->pd_count++;
			return;
		}
		temp_data = temp_data->next;
	}

	pr_info("%s: failed to add pd_data(0x%04x, %d)\n",
		__func__, pid, pd_count);
	kfree(pd_data);
}

void init_cisd_pd_data(struct cisd *cisd)
{
	struct pd_data *temp_data = NULL;

	mutex_lock(&cisd->pdlock);
	temp_data = cisd->pd_array;
	while (temp_data) {
		struct pd_data *next_data = temp_data->next;

		kfree(temp_data);
		temp_data = next_data;
	}

	/* create dummy data */
	cisd->pd_array = create_pd_data(0, 0);
	if (cisd->pd_array == NULL)
		goto err_create_dummy_data;
	temp_data = create_pd_data(MAX_SS_PD_PID, 0);
	if (temp_data == NULL) {
		kfree(cisd->pd_array);
		cisd->pd_array = NULL;
		goto err_create_dummy_data;
	}
	cisd->pd_count = 0;
	cisd->pd_array->next = temp_data;
	temp_data->prev = cisd->pd_array;

err_create_dummy_data:
	mutex_unlock(&cisd->pdlock);
}
EXPORT_SYMBOL(init_cisd_pd_data);

void count_cisd_pd_data(unsigned short vid, unsigned short pid)
{
	struct power_supply *psy = power_supply_get_by_name("battery");
	struct sec_battery_info *battery = NULL;
	struct cisd *cisd = NULL;
	struct pd_data *pd_data;

	if (psy == NULL) {
		pr_info("%s: can't update, psy null\n", __func__);
		return;
	}

	battery = power_supply_get_drvdata(psy);	
	cisd = &battery->cisd;

	pr_info("%s: vid : 0x%04x, pid : 0x%04x\n", __func__, vid, pid);
	if (cisd->pd_array == NULL) {
		pr_info("%s: can't update count of pid(0x%04x) because of null\n",
			__func__, pid);
		return;
	}

	if ((vid != SS_PD_VID) || ((vid == SS_PD_VID)
		&& (pid < MIN_SS_PD_PID || pid > MAX_SS_PD_PID))) {
		pr_info("%s: other pd ta(vid_0x%04x, pid_0x%04x). change pid to 0x0000\n",
			__func__, vid, pid);
		pid = 0x0;
	}

	mutex_lock(&cisd->pdlock);
	pd_data = find_data_by_pd(cisd, pid);
	if (pd_data != NULL)
		pd_data->count++;
	else
		add_pd_data(cisd, pid, 1);
	mutex_unlock(&cisd->pdlock);
}
EXPORT_SYMBOL(count_cisd_pd_data);

void set_cisd_pd_data(struct sec_battery_info *battery, const char *buf)
{
	struct cisd *pcisd = &battery->cisd;
	unsigned int pd_total_count, pid, pd_count;
	struct pd_data *pd_data;
	int i, x;

	pr_info("%s: %s\n", __func__, buf);
	if (pcisd->pd_count > 0)
		init_cisd_pd_data(pcisd);

	if (pcisd->pd_array == NULL) {
		pr_info("%s: can't set the pd data because of null\n", __func__);
		return;
	}

	if (sscanf(buf, "%10u %n", &pd_total_count, &x) <= 0)
		return;

	buf += (size_t)x;
	pr_info("%s: add pd data(count: %d)\n", __func__, pd_total_count);
	for (i = 0; i < pd_total_count; i++) {
		if (sscanf(buf, "0x%04x:%10u %n", &pid, &pd_count, &x) != 2) {
			pr_info("%s: failed to read pd data(0x%04x, %d, %d)!!!re-init pd data\n",
				__func__, pid, pd_count, x);
			init_cisd_pd_data(pcisd);
			break;
		}
		buf += (size_t)x;
		mutex_lock(&pcisd->pdlock);
		pd_data = find_data_by_pd(pcisd, pid);
		if (pd_data != NULL)
			pd_data->count = pd_count;
		else
			add_pd_data(pcisd, pid, pd_count);
		mutex_unlock(&pcisd->pdlock);
	}
}
EXPORT_SYMBOL(set_cisd_pd_data);

static struct dcerr_data *create_dcerr_data(unsigned int dcerr_cause, unsigned int dcerr_count)
{
	struct dcerr_data *temp_data;

	temp_data = kzalloc(sizeof(struct dcerr_data), GFP_KERNEL);
	if (temp_data == NULL)
		return NULL;

	temp_data->cause = dcerr_cause;
	temp_data->count = dcerr_count;
	temp_data->prev = temp_data->next = NULL;

	return temp_data;
}

static struct dcerr_data *find_dcerr_data_by_cause(struct cisd *cisd, unsigned int dcerr_cause)
{
	struct dcerr_data *temp_data = cisd->dcerr_array->next;

	if (cisd->dcerr_count <= 0 || temp_data == NULL)
		return NULL;

	while ((temp_data->cause != dcerr_cause) &&
		((temp_data = temp_data->next) != NULL));

	return temp_data;
}

static void add_dcerr_data(struct cisd *cisd, unsigned int dcerr_cause, unsigned int dcerr_count)
{
	struct dcerr_data *temp_data = cisd->dcerr_array->next;
	struct dcerr_data *dcerr_data;

	if (dcerr_cause >= MAX_DCERR_CAUSE)
		return;

	dcerr_data = create_dcerr_data(dcerr_cause, dcerr_count);
	if (dcerr_data == NULL)
		return;

	pr_info("%s: cause(0x%x), count(%d)\n", __func__, dcerr_cause, dcerr_count);
	while (temp_data) {
		if (temp_data->cause > dcerr_cause) {
			temp_data->prev->next = dcerr_data;
			dcerr_data->prev = temp_data->prev;
			dcerr_data->next = temp_data;
			temp_data->prev = dcerr_data;
			cisd->dcerr_count++;
			return;
		}
		temp_data = temp_data->next;
	}

	pr_info("%s: failed to add dcerr_data(%d, %d)\n",
		__func__, dcerr_cause, dcerr_count);
	kfree(dcerr_data);
}

void init_cisd_dcerr_data(struct cisd *cisd)
{
	struct dcerr_data *temp_data = NULL;

	mutex_lock(&cisd->dcerrlock);
	temp_data = cisd->dcerr_array;
	while (temp_data) {
		struct dcerr_data *next_data = temp_data->next;

		kfree(temp_data);
		temp_data = next_data;
	}

	/* create dummy data */
	cisd->dcerr_array = create_dcerr_data(0, 0);
	if (cisd->dcerr_array == NULL)
		goto err_create_dummy_data;
	temp_data = create_dcerr_data(MAX_DCERR_CAUSE, 0);
	if (temp_data == NULL) {
		kfree(cisd->dcerr_array);
		cisd->dcerr_array = NULL;
		goto err_create_dummy_data;
	}
	cisd->dcerr_count = 0;
	cisd->dcerr_array->next = temp_data;
	temp_data->prev = cisd->dcerr_array;

err_create_dummy_data:
	mutex_unlock(&cisd->dcerrlock);
}
EXPORT_SYMBOL(init_cisd_dcerr_data);

void count_cisd_dcerr_data(struct cisd *cisd, unsigned int dcerr_cause)
{
	struct dcerr_data *dcerr_data;

	if (cisd->dcerr_array == NULL) {
		pr_info("%s: can't update the count of dcerr_cause(0x%x) because of null\n",
			__func__, dcerr_cause);
		return;
	}

	mutex_lock(&cisd->dcerrlock);
	dcerr_data = find_dcerr_data_by_cause(cisd, dcerr_cause);
	if (dcerr_data != NULL)
		dcerr_data->count++;
	else
		add_dcerr_data(cisd, dcerr_cause, 1);
	mutex_unlock(&cisd->dcerrlock);
}
EXPORT_SYMBOL(count_cisd_dcerr_data);

void set_cisd_dcerr_data(struct sec_battery_info *battery, const char *buf)
{
	struct cisd *pcisd = &battery->cisd;
	unsigned int dcerr_total_count, dcerr_cause, dcerr_count;
	struct dcerr_data *dcerr_data;
	int i, x;

	pr_info("%s: %s\n", __func__, buf);
	if (pcisd->dcerr_count > 0)
		init_cisd_dcerr_data(pcisd);

	if (pcisd->dcerr_array == NULL) {
		pr_info("%s: can't set the dcerr data because of null\n", __func__);
		return;
	}

	if (sscanf(buf, "%10u %n", &dcerr_total_count, &x) <= 0) {
		pr_info("%s: failed to read dcerr index\n", __func__);
		return;
	}
	buf += (size_t)x;
	pr_info("%s: add dcerr data(count: %d)\n", __func__, dcerr_total_count);
	for (i = 0; i < dcerr_total_count; i++) {
		if (sscanf(buf, "0x%02x:%10u %n", &dcerr_cause, &dcerr_count, &x) != 2) {
			pr_info("%s: failed to read dcerr data(0x%x, %d, %d)!!!re-init dcerr data\n",
				__func__, dcerr_cause, dcerr_count, x);
			init_cisd_dcerr_data(pcisd);
			break;
		}
		buf += (size_t)x;
		mutex_lock(&pcisd->dcerrlock);
		dcerr_data = find_dcerr_data_by_cause(pcisd, dcerr_cause);
		if (dcerr_data != NULL)
			dcerr_data->count = dcerr_count;
		else
			add_dcerr_data(pcisd, dcerr_cause, dcerr_count);
		mutex_unlock(&pcisd->dcerrlock);
	}
}
EXPORT_SYMBOL(set_cisd_dcerr_data);