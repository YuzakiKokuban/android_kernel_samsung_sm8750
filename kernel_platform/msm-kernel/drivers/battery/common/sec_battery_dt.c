/*
 *  sec_battery_dt.c
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
#include "sec_battery_dt.h"
#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#else
#define __visible_for_testing static
#endif

#ifdef CONFIG_OF
#define PROPERTY_NAME_SIZE 128
int sec_bat_parse_dt_siop(
	struct sec_battery_info *battery, struct device_node *np)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0, len = 0;
	const u32 *p;

	ret = of_property_read_u32(np, "battery,siop_icl",
			&pdata->siop_icl);
	if (ret)
		pdata->siop_icl = SIOP_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_fcc",
			&pdata->siop_fcc);
	if (ret)
		pdata->siop_fcc = SIOP_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_12v_icl",
			&pdata->siop_hv_12v_icl);
	if (ret)
		pdata->siop_hv_12v_icl = SIOP_HV_12V_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_12v_fcc",
			&pdata->siop_hv_12v_fcc);
	if (ret)
		pdata->siop_hv_12v_fcc = SIOP_HV_12V_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_icl",
			&pdata->siop_hv_icl);
	if (ret)
		pdata->siop_hv_icl = SIOP_HV_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_icl_2nd",
			&pdata->siop_hv_icl_2nd);
	if (ret)
		pdata->siop_hv_icl_2nd = SIOP_HV_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_hv_fcc",
			&pdata->siop_hv_fcc);
	if (ret)
		pdata->siop_hv_fcc = SIOP_HV_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_apdo_icl",
			&pdata->siop_apdo_icl);
	if (ret)
		pdata->siop_apdo_icl = SIOP_APDO_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_apdo_fcc",
			&pdata->siop_apdo_fcc);
	if (ret)
		pdata->siop_apdo_fcc = SIOP_APDO_CHARGING_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,siop_wpc_icl",
			&pdata->siop_wpc_icl);
	if (ret)
		pdata->siop_wpc_icl = SIOP_WIRELESS_INPUT_LIMIT_CURRENT;

	p = of_get_property(np, "battery,siop_wpc_fcc", &len);
	if (!p) {
		pr_info("%s : battery,siop_wpc_fcc is Empty\n", __func__);
	} else {
		len = len / sizeof(u32);
		pdata->siop_wpc_fcc =
			kzalloc(sizeof(*pdata->siop_wpc_fcc) * len, GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,siop_wpc_fcc",
				pdata->siop_wpc_fcc, len);

		pr_info("%s: parse siop_wpc_fcc, ret = %d, len = %d\n", __func__, ret, len);
	}

	ret = of_property_read_u32(np, "battery,siop_hv_wpc_icl",
			&pdata->siop_hv_wpc_icl);
	if (ret)
		pdata->siop_hv_wpc_icl = SIOP_HV_WIRELESS_INPUT_LIMIT_CURRENT;

	ret = of_property_read_u32(np, "battery,rechg_hv_wpc_icl",
			&pdata->rechg_hv_wpc_icl);
	if (ret)
		pdata->rechg_hv_wpc_icl = pdata->siop_hv_wpc_icl;

	p = of_get_property(np, "battery,siop_hv_wpc_fcc", &len);
	if (!p) {
		pr_info("%s : battery,siop_hv_wpc_fcc is Empty\n", __func__);
	} else {
		len = len / sizeof(u32);
		pdata->siop_hv_wpc_fcc =
			kzalloc(sizeof(*pdata->siop_hv_wpc_fcc) * len, GFP_KERNEL);
		ret = of_property_read_u32_array(np, "battery,siop_hv_wpc_fcc",
				pdata->siop_hv_wpc_fcc, len);

		pr_info("%s: parse siop_hv_wpc_fcc, ret = %d, len = %d\n", __func__, ret, len);
	}

	len = of_property_count_u32_elems(np, "battery,siop_scenarios");
	if (len > 0) {
		int siop_scenarios[SIOP_SCENARIO_NUM_MAX];

		pdata->siop_scenarios_num = (len > SIOP_SCENARIO_NUM_MAX) ? SIOP_SCENARIO_NUM_MAX : len;
		ret = of_property_read_u32_array(np, "battery,siop_scenarios",
					(u32 *)siop_scenarios, pdata->siop_scenarios_num);

		ret = of_property_read_u32(np, "battery,siop_curr_type_num",
				&pdata->siop_curr_type_num);
		if (ret) {
			pdata->siop_scenarios_num = 0;
			pdata->siop_curr_type_num = 0;
			goto parse_siop_next;
		}

		pdata->siop_curr_type_num =
			(pdata->siop_curr_type_num > SIOP_CURR_TYPE_MAX) ? SIOP_CURR_TYPE_MAX : pdata->siop_curr_type_num;

		if (pdata->siop_curr_type_num > 0) {
			int i, j, siop_level;
			char prop_name[PROPERTY_NAME_SIZE];

			for (i = 0; i < pdata->siop_scenarios_num; ++i) {
				siop_level = siop_scenarios[i];
				pdata->siop_table[i].level = siop_level;

				snprintf(prop_name, PROPERTY_NAME_SIZE, "battery,siop_icl_%d", siop_level);
				ret = of_property_read_u32_array(np, prop_name,
							(u32 *)pdata->siop_table[i].icl, pdata->siop_curr_type_num);

				snprintf(prop_name, PROPERTY_NAME_SIZE, "battery,siop_fcc_%d", siop_level);
				ret = of_property_read_u32_array(np, prop_name,
							(u32 *)pdata->siop_table[i].fcc, pdata->siop_curr_type_num);

				for (j = 0; j < pdata->siop_curr_type_num; ++j)
					pr_info("%s: level=%d, [%d].siop_icl[%d]=%d, [%d].siop_fcc[%d]=%d\n",
						__func__, pdata->siop_table[i].level,
						i, j, pdata->siop_table[i].icl[j],
						i, j, pdata->siop_table[i].fcc[j]);
			}
		}
	}

parse_siop_next:

	return ret;
}

int sec_bat_parse_dt_lrp(
	struct sec_battery_info *battery, struct device_node *np, int type)
{
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0, len = 0;
	char prop_name[PROPERTY_NAME_SIZE];
	int lrp_table[LRP_PROPS];

	snprintf(prop_name, PROPERTY_NAME_SIZE,
			"battery,temp_table_%s", LRP_TYPE_STRING[type]);
	len = of_property_count_u32_elems(np, prop_name);
	if (len != LRP_PROPS)
		return -1;

	ret = of_property_read_u32_array(np, prop_name,
		(u32 *)lrp_table, LRP_PROPS);
	if (ret) {
		pr_info("%s: failed to parse %s!!, ret = %d\n",
			__func__, LRP_TYPE_STRING[type], ret);
		return ret;
	}

	pdata->lrp_temp[type].trig[ST2][LCD_OFF] = lrp_table[0];
	pdata->lrp_temp[type].recov[ST2][LCD_OFF] = lrp_table[1];
	pdata->lrp_temp[type].trig[ST1][LCD_OFF] = lrp_table[2];
	pdata->lrp_temp[type].recov[ST1][LCD_OFF] = lrp_table[3];
	pdata->lrp_temp[type].trig[ST2][LCD_ON] = lrp_table[4];
	pdata->lrp_temp[type].recov[ST2][LCD_ON] = lrp_table[5];
	pdata->lrp_temp[type].trig[ST1][LCD_ON] = lrp_table[6];
	pdata->lrp_temp[type].recov[ST1][LCD_ON] = lrp_table[7];
	pdata->lrp_curr[type].st_icl[ST1] = lrp_table[8];
	pdata->lrp_curr[type].st_fcc[ST1] = lrp_table[9];
	pdata->lrp_curr[type].st_icl[ST2] = lrp_table[10];
	pdata->lrp_curr[type].st_fcc[ST2] = lrp_table[11];

	pr_info("%s: lrp_temp[%s].trig_st1=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].trig[ST1][LCD_OFF]);
	pr_info("%s: lrp_temp[%s].trig_st2=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].trig[ST2][LCD_OFF]);
	pr_info("%s: lrp_temp[%s].recov_st1=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].recov[ST1][LCD_OFF]);
	pr_info("%s: lrp_temp[%s].recov_st2=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].recov[ST2][LCD_OFF]);
	pr_info("%s: lrp_temp[%s].trig_st1_lcdon=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].trig[ST1][LCD_ON]);
	pr_info("%s: lrp_temp[%s].trig_st2_lcdon=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].trig[ST2][LCD_ON]);
	pr_info("%s: lrp_temp[%s].recov_st1_lcdon=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].recov[ST1][LCD_ON]);
	pr_info("%s: lrp_temp[%s].recov_st2_lcdon=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_temp[type].recov[ST2][LCD_ON]);
	pr_info("%s: lrp_temp[%s].st1_icl=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_curr[type].st_icl[ST1]);
	pr_info("%s: lrp_temp[%s].st1_fcc=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_curr[type].st_fcc[ST1]);
	pr_info("%s: lrp_temp[%s].st2_icl=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_curr[type].st_icl[ST2]);
	pr_info("%s: lrp_temp[%s].st2_fcc=%d\n",
		__func__, LRP_TYPE_STRING[type], pdata->lrp_curr[type].st_fcc[ST2]);

	return 0;
}

/* ret: table size */
static int sec_bat_parse_adc_table(struct device_node *np, char *name, sec_bat_adc_table_data_t **adc_table, int offset)
{
	char adc_str[64], data_str[64];
	sec_bat_adc_table_data_t *table;
	const u32 *p;
	int i, ret, len = 0;
	int n_len = strlen(name);

	strcpy(adc_str, name);
	strcpy(adc_str+n_len, "temp_table_adc");

	p = of_get_property(np, adc_str, &len);
	if (!p)
		return 0;

	len = len / sizeof(u32);
	table = kcalloc(len, sizeof(sec_bat_adc_table_data_t), GFP_KERNEL);

	strcpy(data_str, name);
	strcpy(data_str+n_len, "temp_table_data");
	for (i = 0; i < len; i++) {
		u32 temp;

		ret = of_property_read_u32_index(np, adc_str, i, &temp);
		table[i].adc = (offset) ? (offset - (int)temp) : ((int)temp);
		if (ret)
			pr_info("%s : %s is Empty\n", __func__, adc_str);
		ret = of_property_read_u32_index(np, data_str, i, &temp);
		table[i].data = (int)temp;
		if (ret)
			pr_info("%s : %s is Empty\n", __func__, data_str);
	}
	*adc_table = table;
	return len;
}

__visible_for_testing void sec_bat_parse_thm_info(struct device_node *np, char *name, struct sec_bat_thm_info *info)
{
	char buf[64];
	int ret;
	int n_len = strlen(name);

	strcpy(buf, name);
	strcpy(buf+n_len, "thermal_source");
	ret = of_property_read_u32(np, buf, &info->source);
	if (ret) {
		info->source = SEC_BATTERY_THERMAL_SOURCE_NONE;
		pr_info("%s : %s is Empty, %d\n", __func__, buf, ret);
	}

	strcpy(buf+n_len, "temp_offset");
	ret = of_property_read_u32(np, buf, &info->offset);
	if (ret)
		pr_info("%s : %s is Empty\n", __func__, buf);
	info->adc_table_size = sec_bat_parse_adc_table(np, name,
			&info->adc_table, info->offset);

	strcpy(buf+n_len, "temp_check_type");
	ret = of_property_read_u32(np, buf, &info->check_type);
	if (ret)
		pr_info("%s : %s is Empty %d\n", __func__, buf, ret);

	strcpy(buf+n_len, "temp_adc_rsense");
	ret = of_property_read_u32(np, buf, &info->adc_rsense);
	if (ret)
		pr_info("%s : %s is Empty %d\n", __func__, buf, ret);

	if (info->source == SEC_BATTERY_THERMAL_SOURCE_ADC) {
		ret = of_property_read_u32(np, "battery,adc_check_count",
				&info->check_count);
		if (ret)
			pr_info("%s : Adc check count is Empty, %d\n", __func__, ret);
	}
	info->test = 0x7FFF;
}

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
static void sec_bat_parse_dc_thm(struct device_node *np, sec_battery_platform_data_t *pdata)
{
	int ret = 0, len = 0, i = 0;
	const u32 *p;
	int len_step = 4;
	char str[256] = {0, };

	sec_bat_parse_thm_info(np, "battery,dchg_", &pdata->dchg_thm_info);

	pdata->dctp_by_cgtp = of_property_read_bool(np, "battery,dctp_by_cgtp");
	pdata->dctp_bootmode_en = of_property_read_bool(np, "battery,dctp_bootmode_en");
	pdata->lrpts_by_batts = of_property_read_bool(np, "battery,lrpts_by_batts");

	/* dchg_high_temp */
	p = of_get_property(np, "battery,dchg_high_temp", &len);
	if (!p) {
		pr_info("%s: failed to parse dchg_high_temp!\n", __func__);
		for (i = 0; i < len_step; i++)
			pdata->dchg_high_temp[i] = 690;
		return;
	}
	len = len / sizeof(u32);
	ret = of_property_read_u32_array(np, "battery,dchg_high_temp",
			pdata->dchg_high_temp, len);
	if (len != len_step) {
		pr_err("%s not match size of dchg_high_temp: %d\n", __func__, len);
		for (i = 1; i < len_step; i++)
			pdata->dchg_high_temp[i] = pdata->dchg_high_temp[0];
	}

	/* dchg_high_temp_recovery */
	p = of_get_property(np, "battery,dchg_high_temp_recovery", &len);
	if (!p) {
		pr_info("%s: failed to parse dchg_high_temp_recovery!\n", __func__);
		for (i = 0; i < len_step; i++)
			pdata->dchg_high_temp_recovery[i] = 630;
	}
	len = len / sizeof(u32);
	ret = of_property_read_u32_array(np, "battery,dchg_high_temp_recovery",
			pdata->dchg_high_temp_recovery, len);
	if (len != len_step) {
		pr_err("%s not match size of dchg_high_temp_recovery: %d\n", __func__, len);
		for (i = 1; i < len_step; i++)
			pdata->dchg_high_temp_recovery[i] = pdata->dchg_high_temp_recovery[0];
	}

	/* dchg_high_batt_temp */
	p = of_get_property(np, "battery,dchg_high_batt_temp", &len);
	if (!p) {
		pr_info("%s: failed to parse dchg_high_batt_temp!\n", __func__);
		for (i = 0; i < len_step; i++)
			pdata->dchg_high_batt_temp[i] = 400;
	}
	len = len / sizeof(u32);
	ret = of_property_read_u32_array(np, "battery,dchg_high_batt_temp",
			pdata->dchg_high_batt_temp, len);
	if (len != len_step) {
		pr_err("%s not match size of dchg_high_batt_temp: %d\n", __func__, len);
		for (i = 1; i < len_step; i++)
			pdata->dchg_high_batt_temp[i] = pdata->dchg_high_batt_temp[0];
	}

	/* dchg_high_batt_temp_recovery */
	p = of_get_property(np, "battery,dchg_high_batt_temp_recovery", &len);
	if (!p) {
		pr_info("%s: failed to parse dchg_high_batt_temp_recovery!\n", __func__);
		for (i = 0; i < len_step; i++)
			pdata->dchg_high_batt_temp_recovery[i] = 380;
	}
	len = len / sizeof(u32);
	ret = of_property_read_u32_array(np, "battery,dchg_high_batt_temp_recovery",
			pdata->dchg_high_batt_temp_recovery, len);
	if (len != len_step) {
		pr_err("%s not match size of dchg_high_batt_temp_recovery: %d\n", __func__, len);
		for (i = 1; i < len_step; i++)
			pdata->dchg_high_batt_temp_recovery[i] = pdata->dchg_high_batt_temp_recovery[0];
	}

	sprintf(str, "%s: dchg_htemp: ", __func__);
	for (i = 0; i < len_step; i++)
		sprintf(str + strlen(str), "%d ", pdata->dchg_high_temp[i]);
	sprintf(str + strlen(str), ",dchg_htemp_rec: ");
	for (i = 0; i < len_step; i++)
		sprintf(str + strlen(str), "%d ", pdata->dchg_high_temp_recovery[i]);
	sprintf(str + strlen(str), ",dchg_batt_htemp: ");
	for (i = 0; i < len_step; i++)
		sprintf(str + strlen(str), "%d ", pdata->dchg_high_batt_temp[i]);
	sprintf(str + strlen(str), ",dchg_batt_htemp_rec: ");
	for (i = 0; i < len_step; i++)
		sprintf(str + strlen(str), "%d ", pdata->dchg_high_batt_temp_recovery[i]);
	sprintf(str + strlen(str), "\n");
	pr_info("%s", str);

	return;
}
#else
static void sec_bat_parse_dc_thm(struct device_node *np, sec_battery_platform_data_t *pdata)
{
	pr_info("%s: direct charging is not set\n", __func__);
}
#endif

static void sec_bat_parse_health_condition(struct device_node *np, sec_battery_platform_data_t *pdata)
{
	int ret = 0, len = 0;
	unsigned int i = 0;

	const u32 *p = of_get_property(np, "battery,health_condition_cycle", &len);

	len /= sizeof(u32);
	if (!p || len != BATTERY_HEALTH_MAX) {
		pdata->health_condition = NULL;
		pr_err("%s there is not health_condition, len(%d)\n", __func__, len);
		return;
	}

	pdata->health_condition = kzalloc(len*sizeof(battery_health_condition), GFP_KERNEL);

	for (i = 0; i < len; i++) {
		ret = of_property_read_u32_index(np, "battery,health_condition_cycle",
			i, &(pdata->health_condition[i].cycle));
		if (ret)
			break;
		ret = of_property_read_u32_index(np, "battery,health_condition_asoc",
			i, &(pdata->health_condition[i].asoc));
		if (ret)
			break;
		pr_err("%s: [BATTERY_HEALTH] %d: Cycle(~ %d), ASoC(~ %d)\n",
			__func__, i, pdata->health_condition[i].cycle, pdata->health_condition[i].asoc);
	}

	if (ret) {
		pr_err("%s failed to read battery->pdata->health_condition: %d\n", __func__, ret);
		kfree(pdata->health_condition);
		pdata->health_condition = NULL;
	}
}

static int sec_bat_parse_age_data_by_common_offset(struct device_node *np, sec_battery_platform_data_t *pdata)
{
	int ret = 0, len = 0;
	u32 temp = 0, i = 0, n_len = 0;
	char *age_data_prefix = "battery,age_data_";
	char cycle_str[64];
	char full_condition_soc_str[64];
	char common_offset_str[64];
#if defined(CONFIG_BATTERY_AGE_FORECAST_B2B)
	char max_charging_current_offset_str[64];
#endif

	n_len = strlen(age_data_prefix);
	strcpy(cycle_str, age_data_prefix);
	strcpy(cycle_str + n_len, "cycle");
	strcpy(full_condition_soc_str, age_data_prefix);
	strcpy(full_condition_soc_str + n_len, "full_condition_soc");
	strcpy(common_offset_str, age_data_prefix);
	strcpy(common_offset_str + n_len, "common_offset");
#if defined(CONFIG_BATTERY_AGE_FORECAST_B2B)
	strcpy(max_charging_current_offset_str, age_data_prefix);
	strcpy(max_charging_current_offset_str + n_len, "max_charging_current_offset");
#endif

	len = of_property_count_u32_elems(np, cycle_str);
	if (len > 0) {
		pdata->num_age_step = len;

		len = of_property_count_u32_elems(np, full_condition_soc_str);
		if (len != pdata->num_age_step) {
			pr_info("%s : %s has %d elements - expected %d\n",
				__func__, full_condition_soc_str, len, pdata->num_age_step);
			return -EINVAL;
		}
		len = of_property_count_u32_elems(np, common_offset_str);
		if (len != pdata->num_age_step) {
			pr_info("%s : %s has %d elements - expected %d\n",
				__func__, common_offset_str, len, pdata->num_age_step);
			return -EINVAL;
		}
#if defined(CONFIG_BATTERY_AGE_FORECAST_B2B)
		len = of_property_count_u32_elems(np, max_charging_current_offset_str);
		if (len != pdata->num_age_step) {
			pr_info("%s : %s has %d elements - expected %d\n",
				__func__, max_charging_current_offset_str, len, pdata->num_age_step);
			return -EINVAL;
		}
#endif
	} else {
		pr_info("%s : Error in Calculating Age Data Table Size - %d\n", __func__, len);
		return -EINVAL;
	}

	pdata->age_data = kzalloc(len * sizeof(sec_age_data_t), GFP_KERNEL);
	pr_info("%s : Read Age Data Using Offsets - Table Size %d\n", __func__, len);
	for (i = 0; i < len; i++) {
		ret = of_property_read_u32_index(np, cycle_str, i, &temp);
		if (ret) {
			pr_info("%s : %s is Empty\n", __func__, cycle_str);
			break;
		}
		pdata->age_data[i].cycle = temp;

		ret = of_property_read_u32_index(np, full_condition_soc_str, i, &temp);
		if (ret) {
			pr_info("%s : %s is Empty\n", __func__, full_condition_soc_str);
			break;
		}
		pdata->age_data[i].full_condition_soc = temp;

		ret = of_property_read_u32_index(np, common_offset_str, i, &temp);
		if (ret) {
			pr_info("%s : %s is Empty\n", __func__, common_offset_str);
			break;
		}
		pdata->age_data[i].float_voltage = pdata->chg_float_voltage - temp;
		pdata->age_data[i].full_condition_vcell = pdata->full_condition_vcell - temp;
		pdata->age_data[i].recharge_condition_vcell = pdata->recharge_condition_vcell - temp;

#if defined(CONFIG_BATTERY_AGE_FORECAST_B2B)
		ret = of_property_read_u32_index(np, max_charging_current_offset_str, i, &temp);
		if (ret) {
			pr_info("%s : %s is Empty\n", __func__, max_charging_current_offset_str);
			break;
		}
		pdata->age_data[i].max_charging_current = pdata->max_charging_current - temp;

#endif
	}

	return ret;
}

static void sec_bat_parse_age_data(struct device_node *np, sec_battery_platform_data_t *pdata)
{
	int ret = 0, len = 0;
	unsigned int i = 0;
	const u32 *p;
	char str[256] = {0, };
	bool age_data_by_common_offset = of_property_read_bool(np, "battery,age_data_by_common_offset");

	pr_info("%s: age_data_by_common_offset is %s", __func__, (age_data_by_common_offset ? "true" : "false"));

	if (age_data_by_common_offset) {

		ret = sec_bat_parse_age_data_by_common_offset(np, pdata);
	} else {
		p = of_get_property(np, "battery,age_data", &len);
		if (p) {
			pdata->num_age_step = len / sizeof(sec_age_data_t);
			pdata->age_data = kzalloc(len, GFP_KERNEL);
			ret = of_property_read_u32_array(np, "battery,age_data",
				(u32 *)pdata->age_data, len/sizeof(u32));
		}
	}
	if (ret) {
		pr_err("%s failed to read battery->pdata->age_data: %d\n", __func__, ret);
		kfree(pdata->age_data);
		pdata->age_data = NULL;
		pdata->num_age_step = 0;
	} else {
		pr_err("%s num_age_step : %d\n", __func__, pdata->num_age_step);
		for (i = 0; i < pdata->num_age_step; ++i) {
			memset(str, 0x0, sizeof(str));
			sprintf(str + strlen(str), "[%d/%d]cycle:%d, float:%d, full_v:%d, recharge_v:%d, soc:%d",
				i, pdata->num_age_step-1,
				pdata->age_data[i].cycle,
				pdata->age_data[i].float_voltage,
				pdata->age_data[i].full_condition_vcell,
				pdata->age_data[i].recharge_condition_vcell,
				pdata->age_data[i].full_condition_soc);
#if defined(CONFIG_BATTERY_AGE_FORECAST_B2B)
			sprintf(str + strlen(str), ", max_c:%d",
				pdata->age_data[i].max_charging_current);
#endif
			pr_err("%s: %s", __func__, str);
		}
	}
}

int sec_bat_parse_dt(struct device *dev,
		struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0, len = 0;
	unsigned int i = 0;
	int eur_det_gpio = 0;
	const u32 *p;
	u32 temp = 0;

	np = of_find_node_by_name(NULL, "cable-info");
	if (!np) {
		pr_err ("%s : np NULL\n", __func__);
	} else {
		struct device_node *child;
		u32 input_current = 0, charging_current = 0;

		ret = of_property_read_u32(np, "default_input_current", &input_current);
		ret = of_property_read_u32(np, "default_charging_current", &charging_current);
		ret = of_property_read_u32(np, "full_check_current_1st", &pdata->full_check_current_1st);
		ret = of_property_read_u32(np, "full_check_current_2nd", &pdata->full_check_current_2nd);
		if (!pdata->full_check_current_2nd)
			pdata->full_check_current_2nd = pdata->full_check_current_1st;

		pdata->default_input_current = input_current;
		pdata->default_charging_current = charging_current;

		for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
			pdata->charging_current[i].input_current_limit = (unsigned int)input_current;
			pdata->charging_current[i].fast_charging_current = (unsigned int)charging_current;
		}

		for_each_child_of_node(np, child) {
			ret = of_property_read_u32(child, "input_current", &input_current);
			ret = of_property_read_u32(child, "charging_current", &charging_current);
			p = of_get_property(child, "cable_number", &len);
			if (!p)
				return 1;

			len = len / sizeof(u32);

			for (i = 0; i <= len; i++) {
				ret = of_property_read_u32_index(child, "cable_number", i, &temp);
				pdata->charging_current[temp].input_current_limit = (unsigned int)input_current;
				pdata->charging_current[temp].fast_charging_current = (unsigned int)charging_current;
			}
		}
	}

	for (i = 0; i < SEC_BATTERY_CABLE_MAX; i++) {
		pr_info("%s : CABLE_NUM(%d) INPUT(%d) CHARGING(%d)\n",
			__func__, i,
			pdata->charging_current[i].input_current_limit,
			pdata->charging_current[i].fast_charging_current);
	}

	pr_info("%s : TOPOFF_1ST(%d), TOPOFF_2ND(%d)\n",
		__func__, pdata->full_check_current_1st, pdata->full_check_current_2nd);

	pdata->default_usb_input_current =
		pdata->charging_current[SEC_BATTERY_CABLE_USB].input_current_limit;
	pdata->default_usb_charging_current =
		pdata->charging_current[SEC_BATTERY_CABLE_USB].fast_charging_current;
	pdata->default_wc20_input_current =
		pdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].input_current_limit;
	pdata->default_wc20_charging_current =
		pdata->charging_current[SEC_BATTERY_CABLE_HV_WIRELESS_20].fast_charging_current;
	pdata->default_mpp_input_current =
		pdata->charging_current[SEC_BATTERY_CABLE_WIRELESS_MPP].input_current_limit;
	pdata->default_mpp_charging_current =
		pdata->charging_current[SEC_BATTERY_CABLE_WIRELESS_MPP].fast_charging_current;
#ifdef CONFIG_SEC_FACTORY
	pdata->default_charging_current = 1500;
	pdata->charging_current[SEC_BATTERY_CABLE_TA].fast_charging_current = 1500;
#endif
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
		return 1;
	}

	ret = of_property_read_u32(np, "battery,battery_full_capacity",
			&pdata->battery_full_capacity);
	if (ret)
		pr_info("%s : battery_full_capacity is Empty\n", __func__);

	pdata->soc_by_repcap_en = of_property_read_bool(np, "battery,soc_by_repcap_en");

#ifdef CONFIG_SEC_FACTORY
	ret = of_property_read_u32(np, "battery,factory_chg_limit_max",
		&pdata->store_mode_charging_max);
	if (ret) {
		pr_info("%s :factory_chg_limit_max is Empty\n", __func__);
		pdata->store_mode_charging_max = 80;
	}

	ret = of_property_read_u32(np, "battery,factory_chg_limit_min",
		&pdata->store_mode_charging_min);
	if (ret) {
		pr_info("%s :factory_chg_limit_min is Empty\n", __func__);
		pdata->store_mode_charging_min = 70;
	}
#else
	ret = of_property_read_u32(np, "battery,store_mode_charging_max",
		&pdata->store_mode_charging_max);
	if (ret) {
		pr_info("%s :factory_chg_limit_max is Empty\n", __func__);
		pdata->store_mode_charging_max = 70;
	}

	ret = of_property_read_u32(np, "battery,store_mode_charging_min",
		&pdata->store_mode_charging_min);
	if (ret) {
		pr_info("%s :factory_chg_limit_min is Empty\n", __func__);
		pdata->store_mode_charging_min = 60;
	}
	/* VZW's prepaid devices has "VPP" as sales_code, not "VZW" */
	if (sales_code_is("VZW") || sales_code_is("VPP")) {
		pr_info("%s: Sales is VZW or VPP\n", __func__);

		pdata->store_mode_charging_max = 35;
		pdata->store_mode_charging_min = 30;
	}
#endif /*CONFIG_SEC_FACTORY */

	else {
		pr_info("%s : battery_full_capacity : %d\n", __func__, pdata->battery_full_capacity);
		pdata->cisd_cap_high_thr = pdata->battery_full_capacity + 1000; /* battery_full_capacity + 1000 */
		pdata->cisd_cap_low_thr = pdata->battery_full_capacity + 500; /* battery_full_capacity + 500 */
		pdata->cisd_cap_limit = (pdata->battery_full_capacity * 11) / 10; /* battery_full_capacity + 10% */
	}

	ret = of_property_read_u32(np, "battery,cisd_max_voltage_thr",
		&pdata->max_voltage_thr);
	if (ret) {
		pr_info("%s : cisd_max_voltage_thr is Empty\n", __func__);
		pdata->max_voltage_thr = 4400;
	}

	ret = of_property_read_u32(np, "battery,cisd_alg_index",
			&pdata->cisd_alg_index);
	if (ret) {
		pr_info("%s : cisd_alg_index is Empty. Defalut set to six\n", __func__);
		pdata->cisd_alg_index = 6;
	} else {
		pr_info("%s : set cisd_alg_index : %d\n", __func__, pdata->cisd_alg_index);
	}

	ret = of_property_read_u32(np,
				   "battery,expired_time", &temp);
	if (ret) {
		pr_info("expired time is empty\n");
		pdata->expired_time = 3 * 60 * 60;
	} else {
		pdata->expired_time = (unsigned int) temp;
	}
	pdata->expired_time *= 1000;
	battery->safety_timer.remaining_time = pdata->expired_time;

	ret = of_property_read_u32(np,
				   "battery,recharging_expired_time", &temp);
	if (ret) {
		pr_info("expired time is empty\n");
		pdata->recharging_expired_time = 90 * 60;
	} else {
		pdata->recharging_expired_time = (unsigned int) temp;
	}
	pdata->recharging_expired_time *= 1000;

	ret = of_property_read_u32(np,
				   "battery,standard_curr", &pdata->standard_curr);
	if (ret) {
		pr_info("standard_curr is empty\n");
		pdata->standard_curr = 2150;
	}

	ret = of_property_read_string(np,
		"battery,vendor", (char const **)&pdata->vendor);
	if (ret)
		pr_info("%s: Vendor is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,charger_name", (char const **)&pdata->charger_name);
	if (ret)
		pr_info("%s: Charger name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,otg_name", (char const **)&pdata->otg_name);
	if (ret)
		pr_info("%s: otg_name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,fuelgauge_name", (char const **)&pdata->fuelgauge_name);
	if (ret)
		pr_info("%s: Fuelgauge name is Empty\n", __func__);

	ret = of_property_read_string(np,
		"battery,wireless_charger_name", (char const **)&pdata->wireless_charger_name);
	if (ret)
		pr_info("%s: Wireless charger name is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,inbat_ocv_type",
			&pdata->inbat_ocv_type);
	if (ret) {
		pr_info("%s : inbat_ocv_type is Empty\n", __func__);
		pdata->inbat_ocv_type = SEC_BATTERY_OCV_NONE;
	}

	ret = of_property_read_string(np,
		"battery,fgsrc_switch_name", (char const **)&pdata->fgsrc_switch_name);
	if (ret) {
		pdata->support_fgsrc_change = false;
		pr_info("%s: fgsrc_switch_name is Empty\n", __func__);
	} else {
		pdata->support_fgsrc_change = true;
		pdata->inbat_ocv_type = SEC_BATTERY_OCV_FG_SRC_CHANGE;
	}

	pdata->dynamic_cv_factor = of_property_read_bool(np,
						     "battery,dynamic_cv_factor");

	pdata->slowcharging_usb_bootcomplete = of_property_read_bool(np,
						     "battery,slowcharging_usb_bootcomplete");

	ret = of_property_read_string(np,
		"battery,chip_vendor", (char const **)&pdata->chip_vendor);
	if (ret)
		pr_info("%s: Chip vendor is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,technology",
		&pdata->technology);
	if (ret)
		pr_info("%s : technology is Empty\n", __func__);

	ret = of_property_read_u32(np,
		"battery,wireless_cc_cv", &pdata->wireless_cc_cv);

	pdata->p2p_cv_headroom = of_property_read_bool(np,
						     "battery,p2p_cv_headroom");

	pdata->fake_capacity = of_property_read_bool(np,
						     "battery,fake_capacity");

	pdata->bc12_ifcon_wa = of_property_read_bool(np,
						     "battery,bc12_ifcon_wa");

	ret = of_property_read_u32(np, "battery,power_value",
		&pdata->power_value);
	if (ret) {
		pdata->power_value = 6000;
		pr_info("%s : power_value is Empty\n", __func__);
	}

	pdata->en_batt_full_status_usage = of_property_read_bool(np,
						     "battery,en_batt_full_status_usage");
	pr_info("%s: batt_full_status_usage is %s.\n", __func__,
		pdata->en_batt_full_status_usage ? "Enabled" : "Disabled");

	pdata->en_auto_shipmode_temp_ctrl = of_property_read_bool(np,
						     "battery,en_auto_shipmode_temp_ctrl");

	pdata->boosting_voltage_aicl = of_property_read_bool(np,
						     "battery,boosting_voltage_aicl");

	battery->ta_alert_wa = of_property_read_bool(np, "battery,ta_alert_wa");

#if !defined(CONFIG_SEC_FACTORY)
	pdata->mass_with_usb_thm = of_property_read_bool(np,
						     "battery,mass_with_usb_thm");
	pdata->usb_protection = of_property_read_bool(np,
						     "battery,usb_protection");
#endif

	/* PMIC gpio
	 * Pull down i.e. value 0 -> EUR
	 * Pull up i.e. value 1 -> Non EUR
	 */
	eur_det_gpio = of_get_named_gpio(np, "battery,eur_detection", 0);
	if (eur_det_gpio < 0) {
		pr_info("%s: 'battery,eur_detection' not found\n", __func__);
		pdata->auth_ic_support = 0;
	} else {
		ret = gpio_get_value(eur_det_gpio);
		if (ret != 0) {
			pr_err("%s: not valid auth IC device, gpio_val(%d)\n", __func__, ret);
			pdata->auth_ic_support = 0;
		} else
			pdata->auth_ic_support = 1;
	}

	p = of_get_property(np, "battery,polling_time", &len);
	if (!p)
		return 1;

	len = len / sizeof(u32);
	pdata->polling_time = kzalloc(sizeof(*pdata->polling_time) * len, GFP_KERNEL);
	ret = of_property_read_u32_array(np, "battery,polling_time",
					 pdata->polling_time, len);
	if (ret)
		pr_info("%s : battery,polling_time is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,temp_check_count",
		&pdata->temp_check_count);
	if (ret)
		pr_info("%s : Temp check count is Empty\n", __func__);

	sec_bat_parse_thm_info(np, "battery,", &pdata->bat_thm_info);
	pdata->bat_thm_info.channel = SEC_BAT_ADC_CHANNEL_TEMP;
	sec_bat_parse_thm_info(np, "battery,usb_", &pdata->usb_thm_info);
	pdata->usb_thm_info.channel = SEC_BAT_ADC_CHANNEL_USB_TEMP;
	sec_bat_parse_thm_info(np, "battery,chg_", &pdata->chg_thm_info);
	pdata->chg_thm_info.channel = SEC_BAT_ADC_CHANNEL_CHG_TEMP;
	sec_bat_parse_thm_info(np, "battery,wpc_", &pdata->wpc_thm_info);
	pdata->wpc_thm_info.channel = SEC_BAT_ADC_CHANNEL_WPC_TEMP;
	sec_bat_parse_thm_info(np, "battery,sub_bat_", &pdata->sub_bat_thm_info);
	pdata->sub_bat_thm_info.channel = SEC_BAT_ADC_CHANNEL_SUB_BAT_TEMP;
	sec_bat_parse_thm_info(np, "battery,blkt_", &pdata->blk_thm_info);
	pdata->blk_thm_info.channel = SEC_BAT_ADC_CHANNEL_BLKT_TEMP;
	sec_bat_parse_thm_info(np, "battery,dchg_", &pdata->dchg_thm_info);
	pdata->dchg_thm_info.channel = SEC_BAT_ADC_CHANNEL_DC_TEMP;

	ret = of_property_read_u32(np, "battery,adc_read_type",
		&pdata->adc_read_type);
	if (ret ||
		(pdata->adc_read_type != SEC_BATTERY_ADC_PROCESSED &&
		pdata->adc_read_type != SEC_BATTERY_ADC_RAW)) {
		pdata->adc_read_type = SEC_BATTERY_ADC_PROCESSED;
		pr_info("%s : adc_read_type is default (processed)\n", __func__);
	} else {
		pr_info("%s : adc_read_type is %s\n",
			__func__, (pdata->adc_read_type ? "raw" : "proc."));
	}

	/* parse dc thm info */
	sec_bat_parse_dc_thm(np, pdata);

	ret = of_property_read_u32(np, "battery,d2d_check_type",
		&pdata->d2d_check_type);
	if (ret) {
		pdata->d2d_check_type = SB_D2D_NONE;
		pr_info("%s : d2d_check_type is Empty\n", __func__);
	}

	pdata->support_vpdo = of_property_read_bool(np,
					"battery,support_vpdo");

	ret = of_property_read_u32(np, "battery,lrp_temp_check_type",
		&pdata->lrp_temp_check_type);
	if (ret)
		pr_info("%s : lrp_temp_check_type is Empty\n", __func__);

	if (pdata->lrp_temp_check_type) {
		for (i = 0; i < LRP_MAX; i++) {
			if (sec_bat_parse_dt_lrp(battery, np, i) < 0) {
				pdata->lrp_temp[i].trig[ST1][LCD_OFF] = 375;
				pdata->lrp_temp[i].trig[ST2][LCD_OFF] = 375;
				pdata->lrp_temp[i].recov[ST1][LCD_OFF] = 365;
				pdata->lrp_temp[i].recov[ST2][LCD_OFF] = 365;
				pdata->lrp_temp[i].trig[ST1][LCD_ON] = 375;
				pdata->lrp_temp[i].trig[ST2][LCD_ON] = 375;
				pdata->lrp_temp[i].recov[ST1][LCD_ON] = 365;
				pdata->lrp_temp[i].recov[ST2][LCD_ON] = 365;
				pdata->lrp_curr[i].st_icl[0] = pdata->default_input_current;
				pdata->lrp_curr[i].st_fcc[0] = pdata->default_charging_current;
				pdata->lrp_curr[i].st_icl[1] = pdata->default_input_current;
				pdata->lrp_curr[i].st_fcc[1] = pdata->default_charging_current;
			}
		}
		pdata->sc_LRP_25W = of_property_read_bool(np,
			"battery,sc_LRP_25W");
	}
/* mix temp v2 */
	pdata->enable_mix_v2 = of_property_read_bool(np, "battery,enable_mix_v2");

	ret = of_property_read_u32(np, "battery,mix_v2_lrp_recov", &pdata->mix_v2_lrp_recov);
	if (ret) {
		pr_info("%s : mix_v2_lrp_recov is Empty\n", __func__);
		pdata->mix_v2_lrp_recov = 0;
	}
	ret = of_property_read_u32(np, "battery,mix_v2_lrp_cond", &pdata->mix_v2_lrp_cond);
	if (ret) {
		pr_info("%s : mix_v2_lrp_cond is Empty\n", __func__);
		pdata->mix_v2_lrp_cond = 0;
	}
	ret = of_property_read_u32(np, "battery,mix_v2_bat_cond", &pdata->mix_v2_bat_cond);
	if (ret) {
		pr_info("%s : mix_v2_bat_cond is Empty\n", __func__);
		pdata->mix_v2_bat_cond = 0;
	}
	ret = of_property_read_u32(np, "battery,mix_v2_chg_cond", &pdata->mix_v2_chg_cond);
	if (ret) {
		pr_info("%s : mix_v2_chg_cond is Empty\n", __func__);
		pdata->mix_v2_chg_cond = 0;
	}
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	ret = of_property_read_u32(np, "battery,mix_v2_dchg_cond", &pdata->mix_v2_dchg_cond);
	if (ret) {
		pr_info("%s : mix_v2_dchg_cond is Empty\n", __func__);
		pdata->mix_v2_dchg_cond = 0;
	}
#else
	pr_info("%s : mix_v2_dchg_cond is not supported\n", __func__);
	pdata->mix_v2_dchg_cond = 0;
#endif
/* mix temp v2 */

	if (pdata->chg_thm_info.check_type) {
		ret = of_property_read_u32(np, "battery,chg_12v_high_temp",
					   &temp);
		pdata->chg_12v_high_temp = (int)temp;
		if (ret)
			pr_info("%s : chg_12v_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_high_temp",
					   &temp);
		pdata->chg_high_temp = (int)temp;
		if (ret)
			pr_info("%s : chg_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_high_temp_recovery",
					   &temp);
		pdata->chg_high_temp_recovery = (int)temp;
		if (ret)
			pr_info("%s : chg_temp_recovery is Empty\n", __func__);

		if (sec_bat_get_lpmode()) {
			ret = of_property_read_u32(np, "battery,chg_high_temp_lpm", &temp);
			if (!ret)
				pdata->chg_high_temp = (int)temp;

			ret = of_property_read_u32(np, "battery,chg_high_temp_recovery_lpm", &temp);
			if (!ret)
				pdata->chg_high_temp_recovery = (int)temp;

		}
		pr_info("%s : chg_temp_high(%d,%d)\n", __func__,
				pdata->chg_high_temp, pdata->chg_high_temp_recovery);

		ret = of_property_read_u32(np, "battery,chg_charging_limit_current",
					   &pdata->chg_charging_limit_current);
		if (ret)
			pr_info("%s : chg_charging_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,chg_input_limit_current",
					   &pdata->chg_input_limit_current);
		if (ret)
			pr_info("%s : chg_input_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,dchg_charging_limit_current",
					   &pdata->dchg_charging_limit_current);
		if (ret) {
			pr_info("%s : dchg_charging_limit_current is Empty\n", __func__);
			pdata->dchg_charging_limit_current = pdata->chg_charging_limit_current;
		}

		ret = of_property_read_u32(np, "battery,dchg_input_limit_current",
					   &pdata->dchg_input_limit_current);
		if (ret) {
			pr_info("%s : dchg_input_limit_current is Empty\n", __func__);
			pdata->dchg_input_limit_current = pdata->chg_input_limit_current;
		}

		ret = of_property_read_u32(np, "battery,mix_high_temp",
					   &temp);
		pdata->mix_high_temp = (int)temp;
		if (ret)
			pr_info("%s : mix_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,mix_high_chg_temp",
					   &temp);
		pdata->mix_high_chg_temp = (int)temp;
		if (ret)
			pr_info("%s : mix_high_chg_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,mix_high_temp_recovery",
					   &temp);
		pdata->mix_high_temp_recovery = (int)temp;
		if (ret)
			pr_info("%s : mix_high_temp_recovery is Empty\n", __func__);
	}

	if (pdata->wpc_thm_info.check_type) {
		ret = of_property_read_u32(np, "battery,sub_temp_control_source",
				&pdata->sub_temp_control_source);
		if (ret) {
			pr_info("%s : sub_temp_control_source is Empty\n", __func__);
			pdata->sub_temp_control_source = TEMP_CONTROL_SOURCE_BAT_THM;
		}

		ret = of_property_read_u32(np, "battery,wpc_temp_control_source",
				&pdata->wpc_temp_control_source);
		if (ret) {
			pr_info("%s : wpc_temp_control_source is Empty\n", __func__);
			pdata->wpc_temp_control_source = TEMP_CONTROL_SOURCE_CHG_THM;
		}

		ret = of_property_read_u32(np, "battery,wpc_temp_lcd_on_control_source",
				&pdata->wpc_temp_lcd_on_control_source);
		if (ret) {
			pr_info("%s : wpc_temp_lcd_on_control_source is Empty\n", __func__);
			pdata->wpc_temp_lcd_on_control_source = TEMP_CONTROL_SOURCE_CHG_THM;
		}

		ret = of_property_read_u32(np, "battery,wpc_high_temp",
				&pdata->wpc_high_temp);
		if (ret)
			pr_info("%s : wpc_high_temp is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_high_temp_recovery",
				&pdata->wpc_high_temp_recovery);
		if (ret)
			pr_info("%s : wpc_high_temp_recovery is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_high_temp_12w",
			&pdata->wpc_high_temp_12w);
		if (ret) {
			pr_info("%s : wpc_high_temp_12w is Empty\n", __func__);
			pdata->wpc_high_temp_12w = pdata->wpc_high_temp;
		}

		ret = of_property_read_u32(np, "battery,wpc_high_temp_recovery_12w",
			&pdata->wpc_high_temp_recovery_12w);
		if (ret) {
			pr_info("%s : wpc_high_temp_recovery_12w is Empty\n", __func__);
			pdata->wpc_high_temp_recovery_12w = pdata->wpc_high_temp_recovery;
		}

		ret = of_property_read_u32(np, "battery,wpc_high_temp_15w",
			&pdata->wpc_high_temp_15w);
		if (ret) {
			pr_info("%s : wpc_high_temp_15w is Empty\n", __func__);
			pdata->wpc_high_temp_15w = pdata->wpc_high_temp;
		}

		ret = of_property_read_u32(np, "battery,wpc_high_temp_recovery_15w",
			&pdata->wpc_high_temp_recovery_15w);
		if (ret) {
			pr_info("%s : wpc_high_temp_recovery_15w is Empty\n", __func__);
			pdata->wpc_high_temp_recovery_15w = pdata->wpc_high_temp_recovery;
		}

		ret = of_property_read_u32(np, "battery,wpc_temp_offset_step",
			&pdata->wpc_temp_offset_step);
		if (ret)
			pdata->wpc_temp_offset_step = 50;

		pdata->wpc_high_check_using_lrp = of_property_read_bool(np, "battery,wpc_high_check_using_lrp");

		if (pdata->wpc_high_check_using_lrp) {
			ret = of_property_read_u32(np, "battery,wpc_lrp_high_temp",
					&pdata->wpc_lrp_high_temp);
			if (ret)
				pr_info("%s : wpc_lrp_high_temp is Empty\n", __func__);

			ret = of_property_read_u32(np, "battery,wpc_lrp_high_temp_recovery",
					&pdata->wpc_lrp_high_temp_recovery);
			if (ret)
				pr_info("%s : wpc_lrp_high_temp_recovery is Empty\n", __func__);

			ret = of_property_read_u32(np, "battery,wpc_lrp_high_temp_12w",
					&pdata->wpc_lrp_high_temp_12w);
			if (ret) {
				pr_info("%s : wpc_lrp_high_temp_12w is Empty\n", __func__);
				pdata->wpc_lrp_high_temp_12w = pdata->wpc_lrp_high_temp;
			}

			ret = of_property_read_u32(np, "battery,wpc_lrp_high_temp_recovery_12w",
					&pdata->wpc_lrp_high_temp_recovery_12w);
			if (ret) {
				pr_info("%s : wpc_lrp_high_temp_recovery_12w is Empty\n", __func__);
				pdata->wpc_lrp_high_temp_recovery_12w = pdata->wpc_lrp_high_temp_recovery;
			}

			ret = of_property_read_u32(np, "battery,wpc_lrp_high_temp_15w",
					&pdata->wpc_lrp_high_temp_15w);
			if (ret) {
				pr_info("%s : wpc_lrp_high_temp_15w is Empty\n", __func__);
				pdata->wpc_lrp_high_temp_15w = pdata->wpc_lrp_high_temp;
			}

			ret = of_property_read_u32(np, "battery,wpc_lrp_high_temp_recovery_15w",
					&pdata->wpc_lrp_high_temp_recovery_15w);
			if (ret) {
				pr_info("%s : wpc_lrp_high_temp_recovery_15w is Empty\n", __func__);
				pdata->wpc_lrp_high_temp_recovery_15w = pdata->wpc_lrp_high_temp_recovery;
			}
		}

		pdata->enable_check_wpc_temp_v2 =
			of_property_read_bool(np, "battery,enable_check_wpc_temp_v2");

		if (pdata->enable_check_wpc_temp_v2) {
			pdata->wpc_high_check_with_nv = of_property_read_bool(np, "battery,wpc_high_check_with_nv");

			ret = of_property_read_u32(np, "battery,wpc_temp_v2_cond", &pdata->wpc_temp_v2_cond);
			if (ret) {
				pr_info("%s : wpc_temp_v2_cond is Empty\n", __func__);
				pdata->wpc_temp_v2_cond = 0;
			}
			ret = of_property_read_u32(np, "battery,wpc_temp_v2_cond_12w", &pdata->wpc_temp_v2_cond_12w);
			if (ret) {
				pr_info("%s : wpc_temp_v2_cond_12w is Empty\n", __func__);
				pdata->wpc_temp_v2_cond_12w = pdata->wpc_temp_v2_cond;
			}
			ret = of_property_read_u32(np, "battery,wpc_temp_v2_cond_15w", &pdata->wpc_temp_v2_cond_15w);
			if (ret) {
				pr_info("%s : wpc_temp_v2_cond_15w is Empty\n", __func__);
				pdata->wpc_temp_v2_cond_15w = pdata->wpc_temp_v2_cond;
			}
			if (pdata->wpc_high_check_using_lrp) {
				ret = of_property_read_u32(np, "battery,wpc_lrp_temp_v2_cond",
						&pdata->wpc_lrp_temp_v2_cond);
				if (ret) {
					pr_info("%s : wpc_lrp_temp_v2_cond is Empty\n", __func__);
					pdata->wpc_lrp_temp_v2_cond = 0;
				}
				ret = of_property_read_u32(np, "battery,wpc_lrp_temp_v2_cond_12w",
						&pdata->wpc_lrp_temp_v2_cond_12w);
				if (ret) {
					pr_info("%s : wpc_lrp_temp_v2_cond_12w is Empty\n", __func__);
					pdata->wpc_lrp_temp_v2_cond_12w = pdata->wpc_lrp_temp_v2_cond;
				}
				ret = of_property_read_u32(np, "battery,wpc_lrp_temp_v2_cond_15w",
						&pdata->wpc_lrp_temp_v2_cond_15w);
				if (ret) {
					pr_info("%s : wpc_lrp_temp_v2_cond_15w is Empty\n", __func__);
					pdata->wpc_lrp_temp_v2_cond_15w = pdata->wpc_lrp_temp_v2_cond;
				}
			}
		}

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp",
				&pdata->wpc_lcd_on_high_temp);
		if (ret) {
			pr_info("%s : wpc_lcd_on_high_temp is Empty\n", __func__);
			pdata->wpc_lcd_on_high_temp = pdata->wpc_high_temp;
		}

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp_rec",
				&pdata->wpc_lcd_on_high_temp_rec);
		if (ret) {
			pr_info("%s : wpc_lcd_on_high_temp_rec is Empty\n", __func__);
			pdata->wpc_lcd_on_high_temp_rec = pdata->wpc_high_temp_recovery;
		}

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp_12w",
				&pdata->wpc_lcd_on_high_temp_12w);
		if (ret) {
			pr_info("%s : wpc_lcd_on_high_temp_12w is Empty\n", __func__);
			pdata->wpc_lcd_on_high_temp_12w = pdata->wpc_high_temp_12w;
		}

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp_rec_12w",
				&pdata->wpc_lcd_on_high_temp_rec_12w);
		if (ret) {
			pr_info("%s : wpc_lcd_on_high_temp_rec_12w is Empty\n", __func__);
			pdata->wpc_lcd_on_high_temp_rec_12w = pdata->wpc_high_temp_recovery_12w;
		}

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp_15w",
				&pdata->wpc_lcd_on_high_temp_15w);
		if (ret) {
			pr_info("%s : wpc_lcd_on_high_temp_15w is Empty\n", __func__);
			pdata->wpc_lcd_on_high_temp_15w = pdata->wpc_high_temp_15w;
		}

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_high_temp_rec_15w",
				&pdata->wpc_lcd_on_high_temp_rec_15w);
		if (ret) {
			pr_info("%s : wpc_lcd_on_high_temp_rec_15w is Empty\n", __func__);
			pdata->wpc_lcd_on_high_temp_rec_15w = pdata->wpc_high_temp_recovery_15w;
		}

		ret = of_property_read_u32(np, "battery,wpc_input_limit_current",
				&pdata->wpc_input_limit_current);
		if (ret)
			pr_info("%s : wpc_input_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_charging_limit_current",
				&pdata->wpc_charging_limit_current);
		if (ret)
			pr_info("%s : wpc_charging_limit_current is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,wpc_lcd_on_input_limit_current",
				&pdata->wpc_lcd_on_input_limit_current);
		if (ret) {
			pr_info("%s : wpc_lcd_on_input_limit_current is Empty\n", __func__);
			pdata->wpc_lcd_on_input_limit_current =
				pdata->wpc_input_limit_current;
		}

		pdata->wpc_vout_ctrl_lcd_on = of_property_read_bool(np,
				"battery,wpc_vout_ctrl_lcd_on");
		if (pdata->wpc_vout_ctrl_lcd_on) {
			ret = of_property_read_u32(np, "battery,wpc_flicker_wa_input_limit_current",
				&pdata->wpc_flicker_wa_input_limit_current);
			if (ret)
				pr_info("%s : wpc_flicker_wa_input_limit_current is Empty\n", __func__);
		}

		len = of_property_count_u32_elems(np, "battery,wpc_step_limit_temp");
		if (len > 0) {
			pdata->wpc_step_limit_size = len;
			len = of_property_count_u32_elems(np, "battery,wpc_step_limit_fcc");
			if (pdata->wpc_step_limit_size != len) {
				pr_err("%s: not matched, wpc_step_limit_temp is %d, wpc_step_limit_fcc is %d\n",
						 __func__, pdata->wpc_step_limit_size, len);
				pdata->wpc_step_limit_size = 0;
			} else {
				pdata->wpc_step_limit_temp =
					kcalloc(pdata->wpc_step_limit_size, sizeof(unsigned int), GFP_KERNEL);
				ret = of_property_read_u32_array(np, "battery,wpc_step_limit_temp",
							(u32 *)pdata->wpc_step_limit_temp, pdata->wpc_step_limit_size);
				if (ret < 0) {
					pr_err("%s failed to read battery,wpc_step_limit_temp: %d\n",
							 __func__, ret);

					kfree(pdata->wpc_step_limit_temp);
					pdata->wpc_step_limit_temp = NULL;
				}

				pdata->wpc_step_limit_fcc =
					kcalloc(pdata->wpc_step_limit_size, sizeof(unsigned int), GFP_KERNEL);
				ret = of_property_read_u32_array(np, "battery,wpc_step_limit_fcc",
							(u32 *)pdata->wpc_step_limit_fcc, pdata->wpc_step_limit_size);
				if (ret < 0) {
					pr_err("%s failed to read battery,wpc_step_limit_fcc: %d\n",
							 __func__, ret);

					kfree(pdata->wpc_step_limit_fcc);
					pdata->wpc_step_limit_fcc = NULL;
				}

				if (!pdata->wpc_step_limit_temp || !pdata->wpc_step_limit_fcc) {
					pdata->wpc_step_limit_size = 0;
				} else {
					for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
						pr_info("%s: wpc_step temp:%d, fcc:%d\n", __func__,
							pdata->wpc_step_limit_temp[i], pdata->wpc_step_limit_fcc[i]);
					}
				}
			}

			len = of_property_count_u32_elems(np, "battery,wpc_step_limit_fcc_12w");
			if (pdata->wpc_step_limit_size != len) {
				pr_err("%s: not matched, wpc_step_limit_temp is %d, wpc_step_limit_fcc_12w is %d\n",
						 __func__, pdata->wpc_step_limit_size, len);
				pdata->wpc_step_limit_fcc_12w =
					kcalloc(pdata->wpc_step_limit_size, sizeof(unsigned int), GFP_KERNEL);
				for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
					pdata->wpc_step_limit_fcc_12w[i] = pdata->wpc_step_limit_fcc[i];
					pr_info("%s: wpc_step temp:%d, fcc_12w:%d\n", __func__,
						pdata->wpc_step_limit_temp[i], pdata->wpc_step_limit_fcc_12w[i]);
				}
			} else {
				pdata->wpc_step_limit_fcc_12w =
					kcalloc(pdata->wpc_step_limit_size, sizeof(unsigned int), GFP_KERNEL);
				ret = of_property_read_u32_array(np, "battery,wpc_step_limit_fcc_12w",
							(u32 *)pdata->wpc_step_limit_fcc_12w,
							pdata->wpc_step_limit_size);
				if (ret < 0) {
					pr_err("%s failed to read battery,wpc_step_limit_fcc_12w: %d\n",
							 __func__, ret);

					for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
						pdata->wpc_step_limit_fcc_12w[i] = pdata->wpc_step_limit_fcc[i];
						pr_info("%s: wpc_step temp:%d, fcc_12w:%d\n", __func__,
							pdata->wpc_step_limit_temp[i],
							pdata->wpc_step_limit_fcc_12w[i]);
					}
				} else {
					for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
						pr_info("%s: wpc_step temp:%d, fcc_12w:%d\n", __func__,
							pdata->wpc_step_limit_temp[i],
							pdata->wpc_step_limit_fcc_12w[i]);
					}
				}
			}

			len = of_property_count_u32_elems(np, "battery,wpc_step_limit_fcc_15w");
			if (pdata->wpc_step_limit_size != len) {
				pr_err("%s: not matched, wpc_step_limit_temp is %d, wpc_step_limit_fcc_15w is %d\n",
						 __func__, pdata->wpc_step_limit_size, len);
				pdata->wpc_step_limit_fcc_15w =
					kcalloc(pdata->wpc_step_limit_size, sizeof(unsigned int), GFP_KERNEL);
				for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
					pdata->wpc_step_limit_fcc_15w[i] = pdata->wpc_step_limit_fcc[i];
					pr_info("%s: wpc_step temp:%d, fcc_15w:%d\n", __func__,
						pdata->wpc_step_limit_temp[i], pdata->wpc_step_limit_fcc_15w[i]);
				}
			} else {
				pdata->wpc_step_limit_fcc_15w =
					kcalloc(pdata->wpc_step_limit_size, sizeof(unsigned int), GFP_KERNEL);
				ret = of_property_read_u32_array(np, "battery,wpc_step_limit_fcc_15w",
							(u32 *)pdata->wpc_step_limit_fcc_15w,
							pdata->wpc_step_limit_size);
				if (ret < 0) {
					pr_err("%s failed to read battery,wpc_step_limit_fcc_15w: %d\n",
							 __func__, ret);

					for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
						pdata->wpc_step_limit_fcc_15w[i] = pdata->wpc_step_limit_fcc[i];
						pr_info("%s: wpc_step temp:%d, fcc_15w:%d\n", __func__,
							pdata->wpc_step_limit_temp[i],
							pdata->wpc_step_limit_fcc_15w[i]);
					}
				} else {
					for (i = 0; i < pdata->wpc_step_limit_size; ++i) {
						pr_info("%s: wpc_step temp:%d, fcc_15w:%d\n", __func__,
							pdata->wpc_step_limit_temp[i],
							pdata->wpc_step_limit_fcc_15w[i]);
					}
				}
			}
		} else {
			pdata->wpc_step_limit_size = 0;
			pr_info("%s : wpc_step_limit_temp is Empty. len(%d), wpc_step_limit_size(%d)\n",
				__func__, len, pdata->wpc_step_limit_size);
		}

	}

	ret = of_property_read_u32(np, "battery,wc_full_input_limit_current",
		&pdata->wc_full_input_limit_current);
	if (ret)
		pr_info("%s : wc_full_input_limit_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,wc_hero_stand_cc_cv",
		&pdata->wc_hero_stand_cc_cv);
	if (ret) {
		pr_info("%s : wc_hero_stand_cc_cv is Empty\n", __func__);
		pdata->wc_hero_stand_cc_cv = 70;
	}
	ret = of_property_read_u32(np, "battery,wc_hero_stand_cv_current",
		&pdata->wc_hero_stand_cv_current);
	if (ret) {
		pr_info("%s : wc_hero_stand_cv_current is Empty\n", __func__);
		pdata->wc_hero_stand_cv_current = 600;
	}
	ret = of_property_read_u32(np, "battery,wc_hero_stand_hv_cv_current",
		&pdata->wc_hero_stand_hv_cv_current);
	if (ret) {
		pr_info("%s : wc_hero_stand_hv_cv_current is Empty\n", __func__);
		pdata->wc_hero_stand_hv_cv_current = 450;
	}

	ret = of_property_read_u32(np, "battery,sleep_mode_limit_current",
			&pdata->sleep_mode_limit_current);
	if (ret)
		pr_info("%s : sleep_mode_limit_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,inbat_voltage",
			&pdata->inbat_voltage);
	if (ret)
		pr_info("%s : inbat_voltage is Empty\n", __func__);

	if (pdata->inbat_voltage) {
		p = of_get_property(np, "battery,inbat_voltage_table_adc", &len);
		if (!p)
			return 1;

		len = len / sizeof(u32);

		pdata->inbat_adc_table_size = len;

		pdata->inbat_adc_table =
			kzalloc(sizeof(sec_bat_adc_table_data_t) *
					pdata->inbat_adc_table_size, GFP_KERNEL);

		for (i = 0; i < pdata->inbat_adc_table_size; i++) {
			ret = of_property_read_u32_index(np,
							 "battery,inbat_voltage_table_adc", i, &temp);
			pdata->inbat_adc_table[i].adc = (int)temp;
			if (ret)
				pr_info("%s : inbat_adc_table(adc) is Empty\n",
						__func__);

			ret = of_property_read_u32_index(np,
							 "battery,inbat_voltage_table_data", i, &temp);
			pdata->inbat_adc_table[i].data = (int)temp;
			if (ret)
				pr_info("%s : inbat_adc_table(data) is Empty\n",
						__func__);
		}
	}

	ret = of_property_read_u32(np, "battery,pre_afc_input_current",
		&pdata->pre_afc_input_current);
	if (ret) {
		pr_info("%s : pre_afc_input_current is Empty\n", __func__);
		pdata->pre_afc_input_current = 1000;
	}

	ret = of_property_read_u32(np, "battery,select_pd_input_current",
		&pdata->select_pd_input_current);
	if (ret) {
		pr_info("%s : select_pd_input_current is Empty\n", __func__);
		pdata->select_pd_input_current = 1000;
	}

	ret = of_property_read_u32(np, "battery,pre_afc_work_delay",
			&pdata->pre_afc_work_delay);
	if (ret) {
		pr_info("%s : pre_afc_work_delay is Empty\n", __func__);
		pdata->pre_afc_work_delay = 2000;
	}

	ret = of_property_read_u32(np, "battery,pre_wc_afc_input_current",
		&pdata->pre_wc_afc_input_current);
	if (ret) {
		pr_info("%s : pre_wc_afc_input_current is Empty\n", __func__);
		pdata->pre_wc_afc_input_current = 500; /* wc input default */
	}

	ret = of_property_read_u32(np, "battery,pre_wc_afc_work_delay",
			&pdata->pre_wc_afc_work_delay);
	if (ret) {
		pr_info("%s : pre_wc_afc_work_delay is Empty\n", __func__);
		pdata->pre_wc_afc_work_delay = 4000;
	}

	ret = of_property_read_u32(np, "battery,select_pd_input_current",
		&pdata->select_pd_input_current);
	if (ret) {
		pr_info("%s : select_pd_input_current is Empty\n", __func__);
		pdata->select_pd_input_current = 1000;
	}

	ret = of_property_read_u32(np, "battery,tx_stop_capacity",
		&pdata->tx_stop_capacity);
	if (ret) {
		pr_info("%s : tx_stop_capacity is Empty\n", __func__);
		pdata->tx_stop_capacity = 30;
	}

	ret = of_property_read_u32(np, "battery,adc_check_count",
		&pdata->adc_check_count);
	if (ret)
		pr_info("%s : Adc check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,cable_check_type",
		&pdata->cable_check_type);
	if (ret)
		pr_info("%s : Cable check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,cable_source_type",
		&pdata->cable_source_type);
	if (ret)
		pr_info("%s: Cable_source_type is Empty\n", __func__);
#if defined(CONFIG_CHARGING_VZWCONCEPT)
	pdata->cable_check_type &= ~SEC_BATTERY_CABLE_CHECK_NOUSBCHARGE;
	pdata->cable_check_type |= SEC_BATTERY_CABLE_CHECK_NOINCOMPATIBLECHARGE;
#endif
	ret = of_property_read_u32(np, "battery,polling_type",
		&pdata->polling_type);
	if (ret)
		pr_info("%s : Polling type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,monitor_initial_count",
		&pdata->monitor_initial_count);
	if (ret)
		pr_info("%s : Monitor initial count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,battery_check_type",
		&pdata->battery_check_type);
	if (ret)
		pr_info("%s : Battery check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_count",
		&pdata->check_count);
	if (ret)
		pr_info("%s : Check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_adc_max",
		&pdata->check_adc_max);
	if (ret)
		pr_info("%s : Check adc max is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,check_adc_min",
		&pdata->check_adc_min);
	if (ret)
		pr_info("%s : Check adc min is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
		&pdata->ovp_uvlo_check_type);
	if (ret)
		pr_info("%s : Ovp Uvlo check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,overheatlimit_threshold", &temp);
	battery->overheatlimit_threshold = (int)temp;
	if (ret) {
		pr_info("%s : overheatlimit_threshold is Empty\n", __func__);
		battery->overheatlimit_threshold = 700;
	}

	ret = of_property_read_u32(np, "battery,overheatlimit_recovery", &temp);
	battery->overheatlimit_recovery = (int)temp;
	if (ret) {
		pr_info("%s : overheatlimit_recovery is Empty\n", __func__);
		battery->overheatlimit_recovery = 680;
	}

	ret = of_property_read_u32(np, "battery,usb_protection_temp", &temp);
	battery->usb_protection_temp = (int)temp;
	if (ret) {
		pr_info("%s : usb protection temp value is Empty\n", __func__);
		battery->usb_protection_temp = 610;
	}

	ret = of_property_read_u32(np, "battery,temp_gap_bat_usb", &temp);
	battery->temp_gap_bat_usb = (int)temp;
	if (ret) {
		pr_info("%s : temp gap value is Empty\n", __func__);
		battery->temp_gap_bat_usb = 200;
	}

	ret = of_property_read_u32(np, "battery,wire_warm_overheat_thresh", &temp);
	pdata->wire_warm_overheat_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_warm_overheat_thresh is Empty\n", __func__);
		pdata->wire_warm_overheat_thresh = 500;
	}

	ret = of_property_read_u32(np, "battery,wire_normal_warm_thresh", &temp);
	pdata->wire_normal_warm_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_normal_warm_thresh is Empty\n", __func__);
		pdata->wire_normal_warm_thresh = 420;
	}

	ret = of_property_read_u32(np, "battery,wire_cool1_normal_thresh", &temp);
	pdata->wire_cool1_normal_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool1_normal_thresh is Empty\n", __func__);
		pdata->wire_cool1_normal_thresh = 180;
	}

	ret = of_property_read_u32(np, "battery,wire_cool2_cool1_thresh", &temp);
	pdata->wire_cool2_cool1_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool2_cool1_thresh is Empty\n", __func__);
		pdata->wire_cool2_cool1_thresh = 150;
	}

	ret = of_property_read_u32(np, "battery,wire_cool3_cool2_thresh", &temp);
	pdata->wire_cool3_cool2_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool3_cool2_thresh is Empty\n", __func__);
		pdata->wire_cool3_cool2_thresh = 50;
	}

	ret = of_property_read_u32(np, "battery,wire_cold_cool3_thresh", &temp);
	pdata->wire_cold_cool3_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wire_cold_cool3_thresh is Empty\n", __func__);
		pdata->wire_cold_cool3_thresh = 0;
	}

	ret = of_property_read_u32(np, "battery,wireless_warm_overheat_thresh", &temp);
	pdata->wireless_warm_overheat_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_warm_overheat_thresh is Empty\n", __func__);
		pdata->wireless_warm_overheat_thresh = 450;
	}

	ret = of_property_read_u32(np, "battery,wireless_normal_warm_thresh", &temp);
	pdata->wireless_normal_warm_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_normal_warm_thresh is Empty\n", __func__);
		pdata->wireless_normal_warm_thresh = 410;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool1_normal_thresh", &temp);
	pdata->wireless_cool1_normal_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool1_normal_thresh is Empty\n", __func__);
		pdata->wireless_cool1_normal_thresh = 180;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool2_cool1_thresh", &temp);
	pdata->wireless_cool2_cool1_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool2_cool1_thresh is Empty\n", __func__);
		pdata->wireless_cool2_cool1_thresh = 150;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool3_cool2_thresh", &temp);
	pdata->wireless_cool3_cool2_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool3_cool2_thresh is Empty\n", __func__);
		pdata->wireless_cool3_cool2_thresh = 50;
	}

	ret = of_property_read_u32(np, "battery,wireless_cold_cool3_thresh", &temp);
	pdata->wireless_cold_cool3_thresh = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cold_cool3_thresh is Empty\n", __func__);
		pdata->wireless_cold_cool3_thresh = 0;
	}

	ret = of_property_read_u32(np, "battery,wire_warm_current", &temp);
	pdata->wire_warm_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_warm_current is Empty\n", __func__);
		pdata->wire_warm_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wire_cool1_current", &temp);
	pdata->wire_cool1_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool1_current is Empty\n", __func__);
		pdata->wire_cool1_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wire_cool2_current", &temp);
	pdata->wire_cool2_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool2_current is Empty\n", __func__);
		pdata->wire_cool2_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wire_cool3_current", &temp);
	pdata->wire_cool3_current = (int)temp;
	if (ret) {
		pr_info("%s : wire_cool3_current is Empty\n", __func__);
		pdata->wire_cool3_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_warm_current", &temp);
	pdata->wireless_warm_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_warm_current is Empty\n", __func__);
		pdata->wireless_warm_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool1_current", &temp);
	pdata->wireless_cool1_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool1_current is Empty\n", __func__);
		pdata->wireless_cool1_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool2_current", &temp);
	pdata->wireless_cool2_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool2_current is Empty\n", __func__);
		pdata->wireless_cool2_current = 500;
	}

	ret = of_property_read_u32(np, "battery,wireless_cool3_current", &temp);
	pdata->wireless_cool3_current = (int)temp;
	if (ret) {
		pr_info("%s : wireless_cool3_current is Empty\n", __func__);
		pdata->wireless_cool3_current = 500;
	}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	ret = of_property_read_u32(np, "battery,limiter_main_warm_current",
					&pdata->limiter_main_warm_current);
	if (ret)
		pr_info("%s: limiter_main_warm_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_sub_warm_current",
					&pdata->limiter_sub_warm_current);
	if (ret)
		pr_info("%s: limiter_sub_warm_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_main_wireless_warm_current",
					&pdata->limiter_main_wireless_warm_current);
	if (ret)
		pr_info("%s: limiter_main_wireless_warm_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_sub_wireless_warm_current",
					&pdata->limiter_sub_wireless_warm_current);
	if (ret)
		pr_info("%s: limiter_sub_wireless_warm_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_main_cool1_current",
					&pdata->limiter_main_cool1_current);
	if (ret)
		pr_info("%s: limiter_main_cool1_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_sub_cool1_current",
					&pdata->limiter_sub_cool1_current);
	if (ret)
		pr_info("%s: limiter_sub_cool1_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_main_cool2_current",
					&pdata->limiter_main_cool2_current);
	if (ret)
		pr_info("%s: limiter_main_cool2_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_sub_cool2_current",
					&pdata->limiter_sub_cool2_current);
	if (ret)
		pr_info("%s: limiter_sub_cool2_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_main_cool3_current",
					&pdata->limiter_main_cool3_current);
	if (ret)
		pr_info("%s: limiter_main_cool3_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_sub_cool3_current",
					&pdata->limiter_sub_cool3_current);
	if (ret)
		pr_info("%s: limiter_sub_cool3_current is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,limiter_aging_float_offset",
					&pdata->limiter_aging_float_offset);
	if (ret)
		pr_info("%s: limiter_aging_float_offset is Empty\n", __func__);

#if IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	pdata->dual_battery_individual_topoff =	of_property_read_bool(np, "battery,dual_battery_individual_topoff");
	pr_info("%s: dual_battery_individual_topoff is %s.\n", __func__,
		pdata->dual_battery_individual_topoff ? "Enabled" : "Disabled");

	pdata->dual_battery_individual_fv = of_property_read_bool(np, "battery,dual_battery_individual_fv");
	pr_info("%s: dual_battery_individual_fv is %s.\n", __func__,
		pdata->dual_battery_individual_fv ? "Enabled" : "Disabled");

	ret = of_property_read_u32(np, "battery,dual_battery_balance_threshold", &temp);
	pdata->dual_battery_balance_threshold = (int)temp;
	if (ret) {
		pr_info("%s : dual_battery_balance_threshold is Empty\n", __func__);
		pdata->dual_battery_balance_threshold = 4050;
	}
#endif
#endif

	ret = of_property_read_u32(np, "battery,high_temp_float", &temp);
	pdata->high_temp_float = (int)temp;
	if (ret) {
		pr_info("%s : high_temp_float is Empty\n", __func__);
		pdata->high_temp_float = 4150;
	}

	ret = of_property_read_u32(np, "battery,low_temp_float", &temp);
	pdata->low_temp_float = (int)temp;
	if (ret) {
		pr_info("%s : low_temp_float is Empty\n", __func__);
		pdata->low_temp_float = 4350;
	}

	ret = of_property_read_u32(np, "battery,low_temp_cool3_float", &temp);
	pdata->low_temp_cool3_float = (int)temp;
	if (ret) {
		pr_info("%s : low_temp_cool3_float is Empty\n", __func__);
		pdata->low_temp_cool3_float = pdata->low_temp_float;
	}

	ret = of_property_read_u32(np, "battery,buck_recovery_margin", &temp);
	pdata->buck_recovery_margin = (int)temp;
	if (ret) {
		pr_info("%s : buck_recovery_margin is Empty\n", __func__);
		pdata->buck_recovery_margin = 50;
	}

	ret = of_property_read_u32(np, "battery,swelling_high_rechg_voltage", &temp);
	pdata->swelling_high_rechg_voltage = (int)temp;
	if (ret) {
		pr_info("%s : swelling_high_rechg_voltage is Empty\n", __func__);
		pdata->swelling_high_rechg_voltage = 4000;
	}

	pdata->chgen_over_swell_rechg_vol = of_property_read_bool(np, "battery,chgen_over_swell_rechg_vol");
	pr_info("%s: chgen_over_swell_rechg_vol %s.\n", __func__,
		pdata->chgen_over_swell_rechg_vol ? "Enabled" : "Disabled");

	ret = of_property_read_u32(np, "battery,tx_high_threshold",
				   &temp);
	pdata->tx_high_threshold = (int)temp;
	if (ret) {
		pr_info("%s : tx_high_threshold is Empty\n", __func__);
		pdata->tx_high_threshold = 450;
	}

	ret = of_property_read_u32(np, "battery,tx_high_recovery",
				   &temp);
	pdata->tx_high_recovery = (int)temp;
	if (ret) {
		pr_info("%s : tx_high_recovery is Empty\n", __func__);
		pdata->tx_high_recovery = 400;
	}

	ret = of_property_read_u32(np, "battery,tx_low_threshold",
				   &temp);
	pdata->tx_low_threshold = (int)temp;
	if (ret) {
		pr_info("%s : tx_low_threshold is Empty\n", __func__);
		pdata->tx_low_recovery = 0;
	}
	ret = of_property_read_u32(np, "battery,tx_low_recovery",
				   &temp);
	pdata->tx_low_recovery = (int)temp;
	if (ret) {
		pr_info("%s : tx_low_recovery is Empty\n", __func__);
		pdata->tx_low_recovery = 50;
	}

	ret = of_property_read_u32(np, "battery,icl_by_tx_gear",
					   &pdata->icl_by_tx_gear);
	if (ret)
		pr_info("%s : icl_by_tx_gear is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,fcc_by_tx",
				   &pdata->fcc_by_tx);
	if (ret)
		pr_info("%s : fcc_by_tx is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,fcc_by_tx_gear",
				   &pdata->fcc_by_tx_gear);
	if (ret)
		pr_info("%s : fcc_by_tx_gear is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,wpc_input_limit_by_tx_check",
				   &pdata->wpc_input_limit_by_tx_check);
	if (ret)
		pr_info("%s : wpc_input_limit_by_tx_check is Empty\n", __func__);

	if (pdata->wpc_input_limit_by_tx_check) {
		ret = of_property_read_u32(np, "battery,wpc_input_limit_current_by_tx",
				&pdata->wpc_input_limit_current_by_tx);
		if (ret) {
			pr_info("%s : wpc_input_limit_current_by_tx is Empty\n", __func__);
			pdata->wpc_input_limit_current_by_tx =
				pdata->wpc_input_limit_current;
		}
	}

	ret = of_property_read_u32(np, "battery,full_check_type",
		&pdata->full_check_type);
	if (ret)
		pr_info("%s : Full check type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_type_2nd",
		&pdata->full_check_type_2nd);
	if (ret)
		pr_info("%s : Full check type 2nd is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_check_count",
		&pdata->full_check_count);
	if (ret)
		pr_info("%s : Full check count is Empty\n", __func__);

        ret = of_property_read_u32(np, "battery,chg_gpio_full_check",
                &pdata->chg_gpio_full_check);
	if (ret)
		pr_info("%s : Chg gpio full check is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_polarity_full_check",
		&pdata->chg_polarity_full_check);
	if (ret)
		pr_info("%s : Chg polarity full check is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_type",
		&pdata->full_condition_type);
	if (ret)
		pr_info("%s : Full condition type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,full_condition_soc",
		&pdata->full_condition_soc);
	if (ret)
		pr_info("%s : Full condition soc is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_check_count",
		&pdata->recharge_check_count);
	if (ret)
		pr_info("%s : Recharge check count is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_type",
		&pdata->recharge_condition_type);
	if (ret)
		pr_info("%s : Recharge condition type is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,recharge_condition_soc",
		&pdata->recharge_condition_soc);
	if (ret)
		pr_info("%s : Recharge condition soc is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,charging_reset_time",
		(unsigned int *)&pdata->charging_reset_time);
	if (ret)
		pr_info("%s : Charging reset time is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,chg_float_voltage",
		(unsigned int *)&pdata->chg_float_voltage);
	if (ret) {
		pr_info("%s: chg_float_voltage is Empty\n", __func__);
		pdata->chg_float_voltage = 4350;
	}

	ret = of_property_read_u32(np, "battery,chg_float_voltage_conv",
				   &pdata->chg_float_voltage_conv);
	if (ret) {
		pr_info("%s: chg_float_voltage_conv is Empty\n", __func__);
		pdata->chg_float_voltage_conv = 1;
	}

	ret = of_property_read_u32(np, "battery,full_condition_vcell", &pdata->full_condition_vcell);
	if (ret) {
		if (pdata->chg_float_voltage > 0)
			pdata->full_condition_vcell = pdata->chg_float_voltage - DEFAULT_FULL_MARGIN;
		else
			pdata->full_condition_vcell = 4300;
		pr_info("%s : full_condition_vcell is Empty, set it to: %d\n",
			__func__, pdata->full_condition_vcell);
	}

	ret = of_property_read_u32(np, "battery,recharge_condition_vcell", &pdata->recharge_condition_vcell);
	if (ret) {
		if (pdata->chg_float_voltage > 0)
			pdata->recharge_condition_vcell = pdata->chg_float_voltage - DEFAULT_RCHG_MARGIN;
		else
			pdata->recharge_condition_vcell = 4280;
		pr_info("%s : recharge_condition_vcell is Empty, set it to: %d\n",
			__func__, pdata->recharge_condition_vcell);
	}
#if IS_ENABLED(CONFIG_DUAL_BATTERY) && IS_ENABLED(CONFIG_BATTERY_HOTSWAP)
	battery->recharging_voltage_main = pdata->recharge_condition_vcell;
	battery->recharging_voltage_sub = pdata->recharge_condition_vcell;
#endif

	ret = of_property_read_u32(np, "battery,swelling_low_rechg_voltage", &temp);
	pdata->swelling_low_rechg_voltage = (int)temp;
	if (ret) {
		if (pdata->chg_float_voltage > 0)
			pdata->swelling_low_rechg_voltage = pdata->chg_float_voltage - DEFAULT_LOW_SWELLING_RCHG_MARGIN;
		else
			pdata->swelling_low_rechg_voltage = 4200;

		pr_info("%s : swelling_low_rechg_voltage is Empty, set it to: %d\n",
			__func__, pdata->swelling_low_rechg_voltage);
	}

	ret = of_property_read_u32(np, "battery,swelling_low_cool3_rechg_voltage", &temp);
	pdata->swelling_low_cool3_rechg_voltage = (int)temp;
	if (ret) {
		pr_info("%s : swelling_low_cool3_rechg_voltage is Empty\n", __func__);
		pdata->swelling_low_cool3_rechg_voltage = pdata->swelling_low_rechg_voltage;
	}

	ret = of_property_read_u32(np, "battery,max_charging_current",
			&pdata->max_charging_current);
	if (ret) {
		pr_err("%s: max_charging_current is Empty\n", __func__);
		pdata->max_charging_current = 3000;
	}

	sec_bat_parse_age_data(np, battery->pdata);

	sec_bat_parse_health_condition(np, battery->pdata);

	sec_bat_parse_dt_siop(battery, np);

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	for (i = 0; i < DC_MODE_MAX; i++) {
		if (i <= DC_MODE_2TO1)
			pdata->apdo_max_volt_arr[i] = 11000;
		else
			pdata->apdo_max_volt_arr[i] = 16000;
	}
#endif

	ret = of_property_read_u32(np, "battery,wireless_otg_input_current",
			&pdata->wireless_otg_input_current);
	if (ret)
		pdata->wireless_otg_input_current = WIRELESS_OTG_INPUT_CURRENT;

	ret = of_property_read_u32(np, "battery,max_input_voltage",
			&pdata->max_input_voltage);
	if (ret)
		pdata->max_input_voltage = 20000;

	ret = of_property_read_u32(np, "battery,max_input_current",
			&pdata->max_input_current);
	if (ret)
		pdata->max_input_current = 3000;

	ret = of_property_read_u32(np, "battery,pd_charging_charge_power",
			&pdata->pd_charging_charge_power);
	if (ret) {
		pr_err("%s: pd_charging_charge_power is Empty\n", __func__);
		pdata->pd_charging_charge_power = 15000;
	}

	pdata->support_fpdo_dc = of_property_read_bool(np, "battery,support_fpdo_dc");
	if (pdata->support_fpdo_dc) {
		ret = of_property_read_u32(np, "battery,fpdo_dc_charge_power",
				&pdata->fpdo_dc_charge_power);
		if (ret) {
			pr_err("%s: fpdo_dc_charge_power is Empty\n", __func__);
			pdata->fpdo_dc_charge_power = 15000;
		}
	}

	ret = of_property_read_u32(np, "battery,rp_current_rp1",
			&pdata->rp_current_rp1);
	if (ret) {
		pr_err("%s: rp_current_rp1 is Empty\n", __func__);
		pdata->rp_current_rp1 = 500;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rp2",
			&pdata->rp_current_rp2);
	if (ret) {
		pr_err("%s: rp_current_rp2 is Empty\n", __func__);
		pdata->rp_current_rp2 = 1500;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rp3",
			&pdata->rp_current_rp3);
	if (ret) {
		pr_err("%s: rp_current_rp3 is Empty\n", __func__);
		pdata->rp_current_rp3 = 3000;
	}

	ret = of_property_read_u32(np, "battery,rp_current_rdu_rp3",
			&pdata->rp_current_rdu_rp3);
	if (ret) {
		pr_err("%s: rp_current_rdu_rp3 is Empty\n", __func__);
		pdata->rp_current_rdu_rp3 = 2100;
	}

	ret = of_property_read_u32(np, "battery,rp_current_abnormal_rp3",
			&pdata->rp_current_abnormal_rp3);
	if (ret) {
		pr_err("%s: rp_current_abnormal_rp3 is Empty\n", __func__);
		pdata->rp_current_abnormal_rp3 = 1800;
	}

	ret = of_property_read_u32(np, "battery,nv_charge_power",
			&pdata->nv_charge_power);
	if (ret) {
		pr_err("%s: nv_charge_power is Empty\n", __func__);
		pdata->nv_charge_power = mW_by_mVmA(SEC_INPUT_VOLTAGE_5V, pdata->default_input_current);
	}

	ret = of_property_read_u32(np, "battery,tx_minduty_default",
			&pdata->tx_minduty_default);
	if (ret) {
		pdata->tx_minduty_default = 20;
		pr_err("%s: tx minduty is Empty. set %d\n", __func__, pdata->tx_minduty_default);
	}

	ret = of_property_read_u32(np, "battery,tx_minduty_5V",
			&pdata->tx_minduty_5V);
	if (ret) {
		pdata->tx_minduty_5V = 50;
		pr_err("%s: tx minduty 5V is Empty. set %d\n", __func__, pdata->tx_minduty_5V);
	}

	ret = of_property_read_u32(np, "battery,tx_ping_duty_default",
			&pdata->tx_ping_duty_default);
	if (ret) {
		pdata->tx_ping_duty_default = 0;
		pr_err("%s: tx ping duty default is not changed (disabled) %d\n",
			__func__, pdata->tx_ping_duty_default);
	}

	ret = of_property_read_u32(np, "battery,tx_ping_duty_no_ta",
			&pdata->tx_ping_duty_no_ta);
	if (ret) {
		pdata->tx_ping_duty_no_ta = pdata->tx_ping_duty_default;
		pr_err("%s: tx ping duty no TA is default %d\n", __func__, pdata->tx_ping_duty_no_ta);
	}
	if (pdata->tx_ping_duty_default)
		pr_info("%s : tx_ping_duty_default: %d, tx_ping_duty_no_ta: %d\n",
			__func__, pdata->tx_ping_duty_default, pdata->tx_ping_duty_no_ta);

	ret = of_property_read_u32(np, "battery,tx_uno_vout",
			&pdata->tx_uno_vout);
	if (ret) {
		pdata->tx_uno_vout = WC_TX_VOUT_7500MV;
		pr_err("%s: tx uno vout is Empty. set %d\n", __func__, pdata->tx_uno_vout);
	}

	ret = of_property_read_u32(np, "battery,tx_ping_vout",
			&pdata->tx_ping_vout);
	if (ret) {
		pdata->tx_ping_vout = WC_TX_VOUT_5000MV;
		pr_err("%s: tx ping vout is Empty. set %d\n", __func__, pdata->tx_ping_vout);
	}

	ret = of_property_read_u32(np, "battery,tx_gear_vout",
			&pdata->tx_gear_vout);
	if (ret) {
		pdata->tx_gear_vout = WC_TX_VOUT_5000MV;
		pr_info("%s : tx gear vout is Empty. set %d\n", __func__, pdata->tx_gear_vout);
	}

	ret = of_property_read_u32(np, "battery,tx_buds_vout",
			&pdata->tx_buds_vout);
	if (ret) {
		pdata->tx_buds_vout = pdata->tx_uno_vout; // battery,tx_buds_vout in dt is not mandatory
		pr_info("%s : tx buds vout is Empty. set %d\n", __func__, pdata->tx_buds_vout);
	}

	ret = of_property_read_u32(np, "battery,tx_uno_iout",
			&pdata->tx_uno_iout);
	if (ret) {
		pdata->tx_uno_iout = 1500;
		pr_err("%s: tx uno iout is Empty. set %d\n", __func__, pdata->tx_uno_iout);
	}

	ret = of_property_read_u32(np, "battery,tx_uno_iout_gear",
			&pdata->tx_uno_iout_gear);
	if (ret) {
		pdata->tx_uno_iout_gear = pdata->tx_uno_iout;
		pr_err("%s: tx_uno_iout_gear is Empty. set %d\n", __func__, pdata->tx_uno_iout);
	}

	ret = of_property_read_u32(np, "battery,tx_uno_iout_aov_gear",
			&pdata->tx_uno_iout_aov_gear);
	if (ret) {
		pdata->tx_uno_iout_aov_gear = pdata->tx_uno_iout_gear;
		pr_err("%s: tx_uno_iout_aov_gear is Empty. set %d\n",
			__func__, pdata->tx_uno_iout_gear);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_gear",
			&pdata->tx_mfc_iout_gear);
	if (ret) {
		pdata->tx_mfc_iout_gear = 1500;
		pr_err("%s: tx mfc iout gear is Empty. set %d\n", __func__, pdata->tx_mfc_iout_gear);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_aov_gear",
			&pdata->tx_mfc_iout_aov_gear);
	if (ret) {
		pdata->tx_mfc_iout_aov_gear = pdata->tx_mfc_iout_gear;
		pr_err("%s: tx_mfc_iout_aov_gear is Empty. set %d\n",
			__func__, pdata->tx_mfc_iout_gear);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_phone",
			&pdata->tx_mfc_iout_phone);
	if (ret) {
		pdata->tx_mfc_iout_phone = 1100;
		pr_err("%s: tx mfc iout phone is Empty. set %d\n", __func__, pdata->tx_mfc_iout_phone);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_phone_5v",
			&pdata->tx_mfc_iout_phone_5v);
	if (ret) {
		pdata->tx_mfc_iout_phone_5v = 300;
		pr_err("%s: tx mfc iout phone 5v is Empty. set %d\n", __func__, pdata->tx_mfc_iout_phone_5v);
	}

	ret = of_property_read_u32(np, "battery,tx_mfc_iout_lcd_on",
			&pdata->tx_mfc_iout_lcd_on);
	if (ret) {
		pdata->tx_mfc_iout_lcd_on = 900;
		pr_err("%s: tx mfc iout lcd on is Empty. set %d\n", __func__, pdata->tx_mfc_iout_lcd_on);
	}

	pdata->tx_5v_disable = of_property_read_bool(np, "battery,tx_5v_disable");
	pr_info("%s: 5V TA power sharing is %s.\n", __func__,
		pdata->tx_5v_disable ? "Disabled" : "Enabled");

	ret = of_property_read_u32(np, "battery,phm_vout_ctrl_dev",
					&pdata->phm_vout_ctrl_dev);
	if (ret < 0) {
		pr_info("%s: fail to read phm_vout_ctrl_dev\n", __func__);
		pdata->phm_vout_ctrl_dev = 0;
	}
	pr_info("%s: phm_vout_ctrl_dev = %d\n", __func__, pdata->phm_vout_ctrl_dev);

	ret = of_property_read_u32(np, "battery,tx_aov_start_vout",
			&pdata->tx_aov_start_vout);
	if (ret) {
		pdata->tx_aov_start_vout = WC_TX_VOUT_6000MV;
		pr_err("%s: tx aov start vout is Empty. set %d\n", __func__, pdata->tx_aov_start_vout);
	}

	ret = of_property_read_u32(np, "battery,tx_aov_freq_low",
			&pdata->tx_aov_freq_low);
	if (ret) {
		pdata->tx_aov_freq_low = 125;
		pr_err("%s: tx aov freq low is Empty. set %d\n", __func__, pdata->tx_aov_freq_low);
	}

	ret = of_property_read_u32(np, "battery,tx_aov_freq_high",
			&pdata->tx_aov_freq_high);
	if (ret) {
		pdata->tx_aov_freq_high = 147;
		pr_err("%s: tx aov freq high is Empty. set %d\n", __func__, pdata->tx_aov_freq_high);
	}

	ret = of_property_read_u32(np, "battery,tx_aov_delay",
			&pdata->tx_aov_delay);
	if (ret) {
		pdata->tx_aov_delay = 3000;
		pr_err("%s: tx aov dealy is Empty. set %d\n", __func__, pdata->tx_aov_delay);
	}

	ret = of_property_read_u32(np, "battery,tx_aov_delay_phm_escape",
			&pdata->tx_aov_delay_phm_escape);
	if (ret) {
		pdata->tx_aov_delay_phm_escape = 4000;
		pr_err("%s: tx aov dealy phm escape is Empty. set %d\n", __func__, pdata->tx_aov_delay_phm_escape);
	}

	pdata->tx_lrp_temp_compensation = of_property_read_bool(np, "battery,tx_lrp_temp_compensation");
	if (pdata->tx_lrp_temp_compensation) {
		ret = of_property_read_u32(np, "battery,tx_lrp_temp_trig",
				&pdata->tx_lrp_temp_trig);
		if (ret) {
			pr_err("%s: tx_lrp_temp_trig is Empty\n", __func__);
			pdata->tx_lrp_temp_trig = 430;
		}
		ret = of_property_read_u32(np, "battery,tx_lrp_temp_recov",
				&pdata->tx_lrp_temp_recov);
		if (ret) {
			pr_err("%s: tx_lrp_temp_recov is Empty\n", __func__);
			pdata->tx_lrp_temp_recov = 420;
		}
	}

	pdata->wpc_warm_fod = of_property_read_bool(np, "battery,wpc_warm_fod");
	pr_info("%s: WPC Warm FOD %s.\n", __func__,
		pdata->wpc_warm_fod ? "Enabled" : "Disabled");

	/* Default setting 100mA */
	if (pdata->wpc_warm_fod) {
		ret = of_property_read_u32(np, "battery,wpc_warm_fod_icc",
				&pdata->wpc_warm_fod_icc);
		if (ret)
			pdata->wpc_warm_fod_icc = 100;
	}

	ret = of_property_read_u32(np, "battery,max_wlc_icl_15w", &pdata->max_wlc_icl_15w);
	if (ret)
		pr_err("%s: max_wlc_icl_15w is Empty\n", __func__);

	ret = of_property_read_u32(np, "battery,max_wlc_icl_12w", &pdata->max_wlc_icl_12w);
	if (ret)
		pr_err("%s: max_wlc_icl_12w is Empty\n", __func__);

	pdata->lr_enable = of_property_read_bool(np, "battery,lr_enable");
	if (pdata->lr_enable) {
		ret = of_property_read_u32(np, "battery,lr_param_bat_thm",
				&pdata->lr_param_bat_thm);
		if (ret)
			pdata->lr_param_bat_thm = 420;

		ret = of_property_read_u32(np, "battery,lr_param_sub_bat_thm",
				&pdata->lr_param_sub_bat_thm);
		if (ret)
			pdata->lr_param_sub_bat_thm = 580;

		ret = of_property_read_u32(np, "battery,lr_delta",
				&pdata->lr_delta);
		if (ret)
			pdata->lr_delta = 16;

		ret = of_property_read_u32(np, "battery,lr_param_init_bat_thm",
				&pdata->lr_param_init_bat_thm);
		if (ret)
			pdata->lr_param_init_bat_thm = 70;

		ret = of_property_read_u32(np, "battery,lr_param_init_sub_bat_thm",
				&pdata->lr_param_init_sub_bat_thm);
		if (ret)
			pdata->lr_param_init_sub_bat_thm = 30;

		ret = of_property_read_u32(np, "battery,lr_round_off",
				&pdata->lr_round_off);
		if (ret)
			pdata->lr_round_off = 500;
	}

	pr_info("%s: vendor : %s, technology : %d, cable_check_type : %d\n"
		"cable_source_type : %d, polling_type: %d\n"
		"initial_count : %d, check_count : %d\n"
		"battery_check_type : %d, check_adc_max : %d, check_adc_min : %d\n"
		"ovp_uvlo_check_type : %d, thermal_source : %d\n"
		"temp_check_type : %d, temp_check_count : %d, temp_adc_rsense : %d\n"
		"nv_charge_power : %d, full_condition_type : %d, recharge_condition_type : %d\n"
		"full_check_type : %d\n",
		__func__,
		pdata->vendor, pdata->technology,pdata->cable_check_type,
		pdata->cable_source_type, pdata->polling_type,
		pdata->monitor_initial_count, pdata->check_count,
		pdata->battery_check_type, pdata->check_adc_max, pdata->check_adc_min,
		pdata->ovp_uvlo_check_type, pdata->bat_thm_info.source,
		pdata->bat_thm_info.check_type, pdata->temp_check_count, pdata->bat_thm_info.adc_rsense,
		pdata->nv_charge_power, pdata->full_condition_type, pdata->recharge_condition_type,
		pdata->full_check_type
		);

	ret = of_property_read_u32(np, "battery,batt_temp_adj_gap_inc",
		&pdata->batt_temp_adj_gap_inc);
	if (ret) {
		pr_err("%s: batt_temp_adj_gap_inc is Empty\n", __func__);
		pdata->batt_temp_adj_gap_inc = 0;
	}

	ret = of_property_read_u32(np, "battery,change_FV_after_full",
		&pdata->change_FV_after_full);
	if (ret) {
		pr_err("%s: change_FV_after_full is Empty\n", __func__);
		pdata->change_FV_after_full = 0;
	}

	pdata->loosened_unknown_temp = of_property_read_bool(np, "battery,loosened_unknown_temp");

	pdata->pogo_chgin = of_property_read_bool(np, "battery,pogo_chgin");

#if defined(CONFIG_STEP_CHARGING)
	sec_step_charging_init(battery, dev);
#endif

	ret = of_property_read_u32(np, "battery,max_charging_charge_power",
			&pdata->max_charging_charge_power);
	if (ret) {
		pr_err("%s: max_charging_charge_power is Empty\n", __func__);
		pdata->max_charging_charge_power = 25000;
	}

	ret = of_property_read_u32(np, "battery,apdo_max_volt",
			&pdata->apdo_max_volt);
	if (ret) {
		pr_err("%s: apdo_max_volt is Empty\n", __func__);
		pdata->apdo_max_volt = 11000; /* 11v */
	}

#if IS_ENABLED(CONFIG_DUAL_BATTERY)
	ret = of_property_read_string(np,
		"battery,dual_battery_name", (char const **)&pdata->dual_battery_name);
	if (ret)
		pr_info("%s: Dual battery name is Empty\n", __func__);

	np = of_find_node_by_name(NULL, "sec-dual-battery");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
	} else {
		/* zone1 current ratio, 0C ~ 0.4C */
		ret = of_property_read_u32(np, "battery,zone1_limiter_current",
				&pdata->zone1_limiter_current);
		if (ret) {
			pr_err("%s: zone1_limiter_current is Empty\n", __func__);
			pdata->zone1_limiter_current = 100;
		}
		ret = of_property_read_u32(np, "battery,main_zone1_current_rate",
				&pdata->main_zone1_current_rate);
		if (ret) {
			pr_err("%s: main_zone1_current_rate is Empty\n", __func__);
			pdata->main_zone1_current_rate = 50;
		}
		ret = of_property_read_u32(np, "battery,sub_zone1_current_rate",
				&pdata->sub_zone1_current_rate);
		if (ret) {
			pr_err("%s: sub_zone1_current_rate is Empty\n", __func__);
			pdata->sub_zone1_current_rate = 60;
		}
		/* zone2 current ratio, 0.4C ~ 1.1C */
		ret = of_property_read_u32(np, "battery,zone2_limiter_current",
				&pdata->zone2_limiter_current);
		if (ret) {
			pr_err("%s: zone2_limiter_current is Empty\n", __func__);
			pdata->zone2_limiter_current = 1200;
		}
		ret = of_property_read_u32(np, "battery,main_zone2_current_rate",
				&pdata->main_zone2_current_rate);
		if (ret) {
			pr_err("%s: main_zone2_current_rate is Empty\n", __func__);
			pdata->main_zone2_current_rate = 50;
		}
		ret = of_property_read_u32(np, "battery,sub_zone2_current_rate",
				&pdata->sub_zone2_current_rate);
		if (ret) {
			pr_err("%s: sub_zone2_current_rate is Empty\n", __func__);
			pdata->sub_zone2_current_rate = 60;
		}
		/* zone3 current ratio, 1.1C ~ MAX */
		ret = of_property_read_u32(np, "battery,zone3_limiter_current",
				&pdata->zone3_limiter_current);
		if (ret) {
			pr_err("%s: zone3_limiter_current is Empty\n", __func__);
			pdata->zone3_limiter_current = 3000;
		}
		ret = of_property_read_u32(np, "battery,main_zone3_current_rate",
				&pdata->main_zone3_current_rate);
		if (ret) {
			pr_err("%s: main_zone3_current_rate is Empty\n", __func__);
			pdata->main_zone3_current_rate = pdata->main_zone2_current_rate;
		}
		ret = of_property_read_u32(np, "battery,sub_zone3_current_rate",
				&pdata->sub_zone3_current_rate);
		if (ret) {
			pr_err("%s: sub_zone3_current_rate is Empty\n", __func__);
			pdata->sub_zone3_current_rate = pdata->sub_zone2_current_rate;
		}
		ret = of_property_read_u32(np, "battery,force_recharge_margin",
				&pdata->force_recharge_margin);
		if (ret) {
			pr_err("%s: force_recharge_margin is Empty\n", __func__);
			pdata->force_recharge_margin = 150;
		}
		ret = of_property_read_u32(np, "battery,max_main_limiter_current",
				&pdata->max_main_limiter_current);
		if (ret) {
			pr_err("%s: max_main_limiter_current is Empty\n", __func__);
			pdata->max_main_limiter_current = 1550;
		}
		ret = of_property_read_u32(np, "battery,min_main_limiter_current",
				&pdata->min_main_limiter_current);
		if (ret) {
			pr_err("%s: min_main_limiter_current is Empty\n", __func__);
			pdata->min_main_limiter_current = 450;
		}
		ret = of_property_read_u32(np, "battery,max_sub_limiter_current",
				&pdata->max_sub_limiter_current);
		if (ret) {
			pr_err("%s: max_sub_limiter_current is Empty\n", __func__);
			pdata->max_sub_limiter_current = 1300;
		}
		ret = of_property_read_u32(np, "battery,min_sub_limiter_current",
				&pdata->min_sub_limiter_current);
		if (ret) {
			pr_err("%s: min_sub_limiter_current is Empty\n", __func__);
			pdata->min_sub_limiter_current = 450;
		}
		pdata->main_fto = of_property_read_bool(np, "battery,main_fto");
		pdata->sub_fto = of_property_read_bool(np, "battery,sub_fto");

		if (pdata->main_fto) {
			ret = of_property_read_u32(np, "battery,main_fto_current_thresh",
					&pdata->main_fto_current_thresh);
			if (ret) {
				pr_err("%s: main_fto_current_thresh is Empty\n", __func__);
				pdata->main_fto_current_thresh = pdata->zone3_limiter_current;
			}
		}
		if (pdata->sub_fto) {
			ret = of_property_read_u32(np, "battery,sub_fto_current_thresh",
					&pdata->sub_fto_current_thresh);
			if (ret) {
				pr_err("%s: sub_fto_current_thresh is Empty\n", __func__);
				pdata->sub_fto_current_thresh = pdata->zone3_limiter_current;
			}
		}

		pr_info("%s : main ratio:%d(zn1) %d(zn2) %d(zn3), sub ratio:%d(zn1) %d(zn2) %d(zn3), recharge marging:%d, "
				"max main curr:%d, min main curr:%d, max sub curr:%d, min sub curr:%d, main_fto:%d, sub_fto:%d, "
				"main_fto_curr:%d, sub_fto_curr:%d\n",
				__func__, pdata->main_zone1_current_rate, pdata->main_zone2_current_rate, pdata->main_zone3_current_rate,
				pdata->sub_zone1_current_rate, pdata->sub_zone2_current_rate, pdata->sub_zone3_current_rate,
				pdata->force_recharge_margin, pdata->max_main_limiter_current, pdata->min_main_limiter_current,
				pdata->max_sub_limiter_current, pdata->min_sub_limiter_current, pdata->main_fto, pdata->sub_fto,
				pdata->main_fto_current_thresh, pdata->sub_fto_current_thresh);

		ret = of_property_read_string(np, "battery,main_current_limiter",
				(char const **)&battery->pdata->main_limiter_name);
		if (ret)
			pr_err("%s: main_current_limiter is Empty\n", __func__);
		else {
			np = of_find_node_by_name(NULL, battery->pdata->main_limiter_name);
			if (!np) {
				pr_info("%s: main_limiter_name is Empty\n", __func__);
			} else {
				/* MAIN_BATTERY_SW_EN */
				ret = pdata->main_bat_enb_gpio = of_get_named_gpio(np, "limiter,main_bat_enb_gpio", 0);
				if (ret < 0)
					pr_info("%s : can't get main_bat_enb_gpio\n", __func__);

				/* MAIN_BATTERY_SW_EN2 */
				ret = pdata->main_bat_enb2_gpio = of_get_named_gpio(np, "limiter,main_bat_enb2_gpio", 0);
				if (ret < 0)
					pr_info("%s : can't get main_bat_enb2_gpio\n", __func__);
			}
		}
		np = of_find_node_by_name(NULL, "sec-dual-battery");

		ret = of_property_read_string(np, "battery,sub_current_limiter",
				(char const **)&battery->pdata->sub_limiter_name);
		if (ret)
			pr_err("%s: sub_current_limiter is Empty\n", __func__);
		else {
			np = of_find_node_by_name(NULL, battery->pdata->sub_limiter_name);
			if (!np) {
				pr_info("%s: sub_limiter_name is Empty\n", __func__);
			} else {
				/* SUB_BATTERY_SW_EN */
				ret = pdata->sub_bat_enb_gpio = of_get_named_gpio(np, "limiter,sub_bat_enb_gpio", 0);
				if (ret < 0)
					pr_info("%s : can't get sub_bat_enb_gpio\n", __func__);
			}
		}
	}
	np = of_find_node_by_name(NULL, "battery");
#if IS_ENABLED(CONFIG_DUAL_FUELGAUGE)
	ret = of_property_read_string(np,
		"battery,dual_fuelgauge_name", (char const **)&pdata->dual_fuelgauge_name);
	if (ret)
		pr_info("%s: Dual fuelgauge name is Empty\n", __func__);

	np = of_find_node_by_name(NULL, "sec-dual-fuelgauge");
	if (!np) {
		pr_info("%s: np NULL\n", __func__);
	} else {
		ret = of_property_read_u32(np, "battery,main_design_capacity",
				&pdata->main_design_capacity);
		if (ret)
			pr_err("%s: main_design_capacity is Empty\n", __func__);

		ret = of_property_read_u32(np, "battery,sub_design_capacity",
				&pdata->sub_design_capacity);
		if (ret)
			pr_err("%s: sub_design_capacity is Empty\n", __func__);
	}
	ret = of_property_read_string(np, "battery,main_fuelgauge_name",
			(char const **)&battery->pdata->main_fuelgauge_name);
	if (ret)
		pr_err("%s: main_fuelgauge_name is Empty\n", __func__);

	ret = of_property_read_string(np, "battery,sub_fuelgauge_name",
			(char const **)&battery->pdata->sub_fuelgauge_name);
	if (ret)
		pr_err("%s: sub_fuelgauge_name is Empty\n", __func__);

	np = of_find_node_by_name(NULL, "battery");
#endif
#endif

	p = of_get_property(np, "battery,ignore_cisd_index", &len);
	pdata->ignore_cisd_index = kzalloc(sizeof(*pdata->ignore_cisd_index) * 2, GFP_KERNEL);
	if (p) {
		len = len / sizeof(u32);
		ret = of_property_read_u32_array(np, "battery,ignore_cisd_index",
	                     pdata->ignore_cisd_index, len);
		if (ret)
			pr_err("%s failed to read ignore_cisd_index: %d\n",
					__func__, ret);
	} else {
		pr_info("%s : battery,ignore_cisd_index is Empty\n", __func__);
	}

	p = of_get_property(np, "battery,ignore_cisd_index_d", &len);
	pdata->ignore_cisd_index_d = kzalloc(sizeof(*pdata->ignore_cisd_index_d) * 2, GFP_KERNEL);
	if (p) {
		len = len / sizeof(u32);
		ret = of_property_read_u32_array(np, "battery,ignore_cisd_index_d",
	                     pdata->ignore_cisd_index_d, len);
		if (ret)
			pr_err("%s failed to read ignore_cisd_index_d: %d\n",
					__func__, ret);
	} else {
		pr_info("%s : battery,ignore_cisd_index_d is Empty\n", __func__);
	}

	pdata->support_usb_conn_check = of_property_read_bool(np,
		"battery,support_usb_conn_check");
	pr_info("%s: support_usb_conn_check(%d)\n", __func__, pdata->support_usb_conn_check);

	ret = of_property_read_u32(np, "battery,usb_conn_slope_avg",
				&pdata->usb_conn_slope_avg);
	if (ret) {
		pdata->usb_conn_slope_avg = 9; /* 0.9 degrees */
		pr_err("%s: usb_conn_slope_avg is default: %d\n", __func__, pdata->usb_conn_slope_avg);
	}

	pdata->support_spsn_ctrl = of_property_read_bool(np,
		"battery,support_spsn_ctrl");
	pr_info("%s: support_spsn_ctrl(%d)\n", __func__, pdata->support_spsn_ctrl);

#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
	battery->disable_mfc = of_property_read_bool(np,
						     "battery,disable_mfc");
	pr_info("%s: disable_mfc(%d)\n", __func__, battery->disable_mfc);
#endif

	pdata->hall_ic_notifier = of_property_read_bool(np,
						     "battery,hall_ic_notifier");
	pr_info("%s: support hall_ic_notifier(%d)\n", __func__, pdata->hall_ic_notifier);

	pdata->support_mpla_thr_recov = of_property_read_bool(np,
						     "battery,support_mpla_thr_recov");
	pr_info("%s: support support_mpla_thr_recov(%d)\n", __func__, pdata->support_mpla_thr_recov);

	if (pdata->support_mpla_thr_recov) {
		ret = of_property_read_u32(np, "battery,mpla_thr_recov_cnt", &pdata->mpla_thr_recov_cnt);
		if (ret) {
			pdata->mpla_thr_recov_cnt = 15;
			pr_err("%s: mpla_thr_recov_cnt is Empty, use default: %d\n",
				__func__, pdata->mpla_thr_recov_cnt);
		} else
			pr_info("%s: mpla_thr_recov_cnt is: %d\n",
				__func__, pdata->mpla_thr_recov_cnt);
	}

	pdata->multi_batt_id_chg_support = of_property_read_bool(np,
						     "battery,multi_batt_id_chg_support");
	pr_info("%s: support multi_batt_id_chg_support(%d)\n", __func__, pdata->multi_batt_id_chg_support);

	return 0;
}
EXPORT_SYMBOL(sec_bat_parse_dt);

void sec_bat_parse_mode_dt(struct sec_battery_info *battery)
{
	struct device_node *np;
	sec_battery_platform_data_t *pdata = battery->pdata;
	int ret = 0;
	u32 temp = 0;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
		return;
	}

	if (battery->store_mode) {
		ret = of_property_read_u32(np, "battery,store_mode_max_input_power",
			&pdata->store_mode_max_input_power);
		if (ret) {
			pr_info("%s : store_mode_max_input_power is Empty\n", __func__);
			pdata->store_mode_max_input_power = 4000;
		}

		if (pdata->wpc_thm_info.check_type) {
			ret = of_property_read_u32(np, "battery,wpc_store_high_temp",
			   &temp);
			if (!ret) {
				pdata->wpc_high_temp = temp;
				pdata->wpc_high_temp_12w = temp;
				pdata->wpc_high_temp_15w = temp;
			}

			ret = of_property_read_u32(np, "battery,wpc_store_high_temp_recovery",
			   &temp);
			if (!ret) {
				pdata->wpc_high_temp_recovery = temp;
				pdata->wpc_high_temp_recovery_12w = temp;
				pdata->wpc_high_temp_recovery_15w = temp;
			}

			ret = of_property_read_u32(np, "battery,wpc_store_charging_limit_current",
			   &temp);
			if (!ret)
				pdata->wpc_input_limit_current = temp;

			ret = of_property_read_u32(np, "battery,wpc_store_lcd_on_high_temp",
			   &temp);
			if (!ret) {
				pdata->wpc_lcd_on_high_temp = (int)temp;
				pdata->wpc_lcd_on_high_temp_12w = (int)temp;
				pdata->wpc_lcd_on_high_temp_15w = (int)temp;
			}

			ret = of_property_read_u32(np, "battery,wpc_store_lcd_on_high_temp_rec",
			   &temp);
			if (!ret) {
				pdata->wpc_lcd_on_high_temp_rec = (int)temp;
				pdata->wpc_lcd_on_high_temp_rec_12w = (int)temp;
				pdata->wpc_lcd_on_high_temp_rec_15w = (int)temp;
			}

			ret = of_property_read_u32(np, "battery,wpc_store_lcd_on_charging_limit_current",
				&temp);
			if (!ret)
				pdata->wpc_lcd_on_input_limit_current = (int)temp;

			pr_info("%s: update store_mode - wpc high_temp(t:%d/%d/%d, r:%d/%d/%d), "
					"lcd_on_high_temp(t:%d/%d%d, r:%d/%d/%d), curr(%d, %d)\n", __func__,
				pdata->wpc_high_temp, pdata->wpc_high_temp_12w, pdata->wpc_high_temp_15w,
				pdata->wpc_high_temp_recovery, pdata->wpc_high_temp_recovery_12w, pdata->wpc_high_temp_recovery_15w,
				pdata->wpc_lcd_on_high_temp, pdata->wpc_lcd_on_high_temp_12w, pdata->wpc_lcd_on_high_temp_12w,
				pdata->wpc_lcd_on_high_temp_rec, pdata->wpc_lcd_on_high_temp_rec_12w, pdata->wpc_lcd_on_high_temp_rec_12w,
				pdata->wpc_input_limit_current, pdata->wpc_lcd_on_input_limit_current);
		}

		ret = of_property_read_u32(np, "battery,siop_store_hv_wpc_icl",
			&temp);
		if (!ret)
			pdata->siop_hv_wpc_icl = temp;
		else
			pdata->siop_hv_wpc_icl = SIOP_STORE_HV_WIRELESS_CHARGING_LIMIT_CURRENT;
		pr_info("%s: update siop_hv_wpc_icl(%d)\n",
			__func__, pdata->siop_hv_wpc_icl);
		pdata->store_mode_buckoff = of_property_read_bool(np, "battery,store_mode_buckoff");
		pr_info("%s : battery,store_mode_buckoff: %d\n", __func__, pdata->store_mode_buckoff);
	}
}
EXPORT_SYMBOL_KUNIT(sec_bat_parse_mode_dt);

void sec_bat_parse_mode_dt_work(struct work_struct *work)
{
	struct sec_battery_info *battery = container_of(work,
		struct sec_battery_info, parse_mode_dt_work.work);

	sec_bat_parse_mode_dt(battery);

	if (is_hv_wire_type(battery->cable_type) ||
		(is_hv_wireless_type(battery->cable_type) &&
		(battery->cable_type != SEC_BATTERY_CABLE_WIRELESS_MPP))) {
		sec_bat_set_charging_current(battery);
	}

	__pm_relax(battery->parse_mode_dt_ws);
}
EXPORT_SYMBOL(sec_bat_parse_mode_dt_work);
#endif
