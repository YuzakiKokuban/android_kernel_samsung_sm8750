/*
 *  sec_adc.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include "sec_adc.h"

#define DEBUG
#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#endif

struct adc_list {
	const char *name;
	struct iio_channel *channel;
	bool is_used;
	int prev_value;
};
static DEFINE_MUTEX(adclock);

static struct adc_list batt_adc_list[SEC_BAT_ADC_CHANNEL_NUM] = {
	{.name = "adc-cable"},
	{.name = "adc-bat-id"},
	{.name = "adc-temp"},
	{.name = "adc-temp-amb"},
	{.name = "adc-full"},
	{.name = "adc-volt"},
	{.name = "adc-chg-temp"},
	{.name = "adc-in-bat"},
	{.name = "adc-dischg"},
	{.name = "adc-dischg-ntc"},
	{.name = "adc-wpc-temp"},
	{.name = "adc-sub-chg-temp"},
	{.name = "adc-usb-temp"},
	{.name = "adc-sub-bat"},
	{.name = "adc-blkt-temp"},
	{.name = "adc-dc-temp"},
};

static int adc_init_count;

#if defined(CONFIG_SEC_KUNIT)
int __mockable adc_read_type(struct device *dev, int channel, int batt_adc_type)
#else
int adc_read_type(struct device *dev, int channel, int batt_adc_type)
#endif
{
	int adc = -1;
	int ret = 0;
	int retry_cnt = RETRY_CNT;

	/* adc init retry because adc init was failed when probe time */
	if (!adc_init_count) {
		int i = 0;
		struct iio_channel *temp_adc;

		pr_err("%s: ADC init retry!!\n", __func__);
		for (i = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++) {
			temp_adc = iio_channel_get(dev, batt_adc_list[i].name);
			batt_adc_list[i].channel = temp_adc;
			batt_adc_list[i].is_used = !IS_ERR_OR_NULL(temp_adc);
			if (batt_adc_list[i].is_used)
				adc_init_count++;
		}
	}

	if (batt_adc_list[channel].is_used) {
		do {
			switch (batt_adc_type) {
			case SEC_BATTERY_ADC_RAW:
				ret = iio_read_channel_raw(batt_adc_list[channel].channel, &adc);
				break;
			default:
				/* SEC_BATTERY_ADC_PROCESSED */
				ret = iio_read_channel_processed(batt_adc_list[channel].channel, &adc);
				break;
			}

			retry_cnt--;
		} while ((retry_cnt > 0) && (adc < 0));
	} else {
		ret = 0;
	}

	if (retry_cnt <= 0) {
		pr_err("%s: Error in ADC\n", __func__);
		adc = batt_adc_list[channel].prev_value;
	} else {
		batt_adc_list[channel].prev_value = adc;
	}

	pr_debug("%s: [%d] ADC (type:%s) = %d\n", __func__, channel,
		(batt_adc_type ? "raw" : "proc."), adc);

	return adc;
}

int sec_bat_get_adc_data(struct device *dev, int adc_ch, int count, int batt_adc_type)
{
	int adc_data = 0;
	int adc_max = 0;
	int adc_min = 0xFFFF;
	int adc_total = 0;
	int i = 0;

	if (count < 3)
		count = 3;

	for (i = 0; i < count; i++) {
		mutex_lock(&adclock);
		adc_data = adc_read_type(dev, adc_ch, batt_adc_type);
		mutex_unlock(&adclock);

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}
		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (count - 2);
}
EXPORT_SYMBOL(sec_bat_get_adc_data);

int sec_bat_get_charger_type_adc(struct sec_battery_info *battery)
{
	/* It is true something valid is connected to the device for charging.
	 * By default this something is considered to be USB.
	 */
	int result = SEC_BATTERY_CABLE_USB;

	int adc = 0;
	int i = 0;

	/* Do NOT check cable type when cable_switch_check() returns false
	 * and keep current cable type
	 */
	if (battery->pdata->cable_switch_check && !battery->pdata->cable_switch_check())
		return battery->cable_type;

	adc = sec_bat_get_adc_data(battery->dev, SEC_BAT_ADC_CHANNEL_CABLE_CHECK,
			battery->pdata->adc_check_count, battery->pdata->adc_read_type);

	/* Do NOT check cable type when cable_switch_normal() returns false
	 * and keep current cable type
	 */
	if (battery->pdata->cable_switch_normal && !battery->pdata->cable_switch_normal())
		return battery->cable_type;

	for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++)
		if ((adc > battery->pdata->cable_adc_value[i].min) && (adc < battery->pdata->cable_adc_value[i].max))
			break;

	if (i >= SEC_BATTERY_CABLE_MAX)
		dev_err(battery->dev, "%s: default USB\n", __func__);
	else
		result = i;

	dev_dbg(battery->dev, "%s: result(%d), adc(%d)\n", __func__, result, adc);

	return result;
}
EXPORT_SYMBOL(sec_bat_get_charger_type_adc);

bool sec_bat_convert_adc_to_val(int adc, int offset, sec_bat_adc_table_data_t *adc_table, int size, int *value)
{
	int temp = 0;
	int low = 0;
	int high = 0;
	int mid = 0;

	if (size <= 0)
		return false;

	adc = (offset) ? (offset - adc) : (adc);

	if (adc_table[0].adc >= adc) {
		temp = adc_table[0].data;
		goto temp_by_adc_goto;
	} else if (adc_table[size-1].adc <= adc) {
		temp = adc_table[size-1].data;
		goto temp_by_adc_goto;
	}

	high = size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (adc_table[mid].adc > adc)
			high = mid - 1;
		else if (adc_table[mid].adc < adc)
			low = mid + 1;
		else {
			temp = adc_table[mid].data;
			goto temp_by_adc_goto;
		}
	}

	temp = adc_table[high].data;
	temp += ((adc_table[low].data - adc_table[high].data) *
			(adc - adc_table[high].adc)) /
		(adc_table[low].adc - adc_table[high].adc);

temp_by_adc_goto:
	*value = temp;
	pr_debug("%s: Temp(%d), Temp-ADC(%d)\n", __func__, temp, adc);

	return true;
}
EXPORT_SYMBOL(sec_bat_convert_adc_to_val);

int sec_bat_get_inbat_vol_by_adc(struct sec_battery_info *battery)
{
	int inbat = 0;
	int inbat_adc;
	int low = 0;
	int high = 0;
	int mid = 0;
	const sec_bat_adc_table_data_t *inbat_adc_table;
	unsigned int inbat_adc_table_size;

	if (!battery->pdata->inbat_adc_table) {
		dev_err(battery->dev, "%s: not designed to read in-bat voltage\n", __func__);
		return -1;
	}

	inbat_adc_table = battery->pdata->inbat_adc_table;
	inbat_adc_table_size = battery->pdata->inbat_adc_table_size;

	inbat_adc = sec_bat_get_adc_data(battery->dev, SEC_BAT_ADC_CHANNEL_INBAT_VOLTAGE,
			battery->pdata->adc_check_count, battery->pdata->adc_read_type);
	if (inbat_adc <= 0)
		return inbat_adc;

	battery->inbat_adc = inbat_adc;

	if (inbat_adc_table[0].adc <= inbat_adc) {
		inbat = inbat_adc_table[0].data;
		goto inbat_by_adc_goto;
	} else if (inbat_adc_table[inbat_adc_table_size-1].adc >= inbat_adc) {
		inbat = inbat_adc_table[inbat_adc_table_size-1].data;
		goto inbat_by_adc_goto;
	}

	high = inbat_adc_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (inbat_adc_table[mid].adc < inbat_adc)
			high = mid - 1;
		else if (inbat_adc_table[mid].adc > inbat_adc)
			low = mid + 1;
		else {
			inbat = inbat_adc_table[mid].data;
			goto inbat_by_adc_goto;
		}
	}

	inbat = inbat_adc_table[high].data;
	inbat +=
		((inbat_adc_table[low].data - inbat_adc_table[high].data) *
		 (inbat_adc - inbat_adc_table[high].adc)) /
		(inbat_adc_table[low].adc - inbat_adc_table[high].adc);

	if (inbat < 0)
		inbat = 0;

inbat_by_adc_goto:
	dev_info(battery->dev, "%s: inbat(%d), inbat-ADC(%d)\n", __func__, inbat, inbat_adc);

	return inbat;
}
EXPORT_SYMBOL(sec_bat_get_inbat_vol_by_adc);

bool sec_bat_check_vf_adc(struct sec_battery_info *battery)
{
	int adc = 0;

	adc = sec_bat_get_adc_data(battery->dev,
		SEC_BAT_ADC_CHANNEL_BATID_CHECK,
		battery->pdata->adc_check_count,
		battery->pdata->adc_read_type);

	if (adc < 0) {
		dev_err(battery->dev, "%s: VF ADC error\n", __func__);
		adc = battery->check_adc_value;
	} else
		battery->check_adc_value = adc;

	if ((battery->check_adc_value <= battery->pdata->check_adc_max) &&
		(battery->check_adc_value >= battery->pdata->check_adc_min)) {
		return true;
	} else {
		dev_info(battery->dev, "%s: VF_ADC(%d) is out of range(min:%d, max:%d)\n",
			__func__, battery->check_adc_value, battery->pdata->check_adc_min, battery->pdata->check_adc_max);
		return false;
	}
}
EXPORT_SYMBOL(sec_bat_check_vf_adc);

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
int sec_bat_get_direct_chg_temp_adc(
	struct sec_battery_info *battery, int adc_data, int count, int check_type)
{
	int temp = 0;
	int temp_adc;
	int low = 0;
	int high = 0;
	int mid = 0;
	const sec_bat_adc_table_data_t *temp_adc_table = {0 , };
	unsigned int temp_adc_table_size = 0;
	int offset = battery->pdata->dchg_thm_info.offset;

	if (check_type == SEC_BATTERY_TEMP_CHECK_FAKE)
		return FAKE_TEMP;

	temp_adc = (offset) ? (offset - adc_data) : (adc_data);
	if (temp_adc < 0)
		return 0;

	temp_adc_table = battery->pdata->dchg_thm_info.adc_table;
	temp_adc_table_size = battery->pdata->dchg_thm_info.adc_table_size;
	battery->pdata->dchg_thm_info.adc = temp_adc;

	if (temp_adc_table[0].adc >= temp_adc) {
		temp = temp_adc_table[0].data;
		goto direct_chg_temp_goto;
	} else if (temp_adc_table[temp_adc_table_size - 1].adc <= temp_adc) {
		temp = temp_adc_table[temp_adc_table_size - 1].data;
		goto direct_chg_temp_goto;
	}

	high = temp_adc_table_size - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if (temp_adc_table[mid].adc > temp_adc)
			high = mid - 1;
		else if (temp_adc_table[mid].adc < temp_adc)
			low = mid + 1;
		else {
			temp = temp_adc_table[mid].data;
			goto direct_chg_temp_goto;
		}
	}

	temp = temp_adc_table[high].data;
	temp += ((temp_adc_table[low].data - temp_adc_table[high].data) *
		 (temp_adc - temp_adc_table[high].adc)) /
		(temp_adc_table[low].adc - temp_adc_table[high].adc);

direct_chg_temp_goto:
	dev_info(battery->dev, "%s: temp(%d), direct-chg-temp-ADC(%d)\n", __func__, temp, adc_data);

	return temp;
}
EXPORT_SYMBOL(sec_bat_get_direct_chg_temp_adc);
#endif

void adc_init(struct platform_device *pdev, struct sec_battery_info *battery)
{
	int i = 0;
	struct iio_channel *temp_adc;

	for (i = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++) {
		temp_adc = iio_channel_get(&pdev->dev, batt_adc_list[i].name);
		batt_adc_list[i].channel = temp_adc;
		batt_adc_list[i].is_used = !IS_ERR_OR_NULL(temp_adc);
		if (batt_adc_list[i].is_used)
			battery->adc_init_count++;
	}

	for (i  = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++)
		pr_info("%s: %s - %s\n", __func__,
				batt_adc_list[i].name, batt_adc_list[i].is_used ? "used" : "not used");
}
EXPORT_SYMBOL(adc_init);

void adc_exit(struct sec_battery_info *battery)
{
	int i = 0;

	for (i = 0; i < SEC_BAT_ADC_CHANNEL_NUM; i++) {
		if (batt_adc_list[i].is_used)
			iio_channel_release(batt_adc_list[i].channel);
	}
}
EXPORT_SYMBOL(adc_exit);

