/*
 *  sec_checklist_app_sysfs.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_battery.h"
#include "sb_checklist_app.h"
#include <linux/battery/sb_sysfs.h>
#include <linux/battery/sb_notify.h>

#define ca_log(str, ...) pr_info("[CHECKLIST-APP]:%s: "str, __func__, ##__VA_ARGS__)

struct sb_ca {
	const char *name;
	struct notifier_block nb;
};

int char_to_int(char *s)
{
	int num = 0;
	int sign = 1;

	do {
		if (*s == '-')
			sign *= -1;

		else if (*s >= '0' && *s <= '9')
			num = (num * 10) + (*s - '0');

		else if (num > 0)
			break;
	} while (*s++);

	return (num * sign);
}

void get_dts_property(const struct device_node *np, const char *name,
					char *buf, unsigned int *p_size, int *i, int value)
{
	bool check = of_property_read_bool(np, name);

	if (check)
		*i += scnprintf(buf + *i, *p_size - *i, "%d ", value);
}

void store_wire_menu(struct sec_battery_info *battery, int tc, int y)
{
	if (tc == OVERHEATLIMIT_THRESH) {
		battery->overheatlimit_threshold = y;
	} else if (tc == OVERHEATLIMIT_RECOVERY) {
		battery->overheatlimit_recovery = y;
	} else if (tc == WIRE_WARM_OVERHEAT_THRESH) {
		battery->pdata->wire_warm_overheat_thresh = y;
	} else if (tc == WIRE_NORMAL_WARM_THRESH) {
		battery->pdata->wire_normal_warm_thresh = y;
	} else if (tc == WIRE_COOL1_NORMAL_THRESH) {
		battery->pdata->wire_cool1_normal_thresh = y;
	} else if (tc == WIRE_COOL2_COOL1_THRESH) {
		battery->pdata->wire_cool2_cool1_thresh = y;
	} else if (tc == WIRE_COOL3_COOL2_THRESH) {
		battery->pdata->wire_cool3_cool2_thresh = y;
	} else if (tc == WIRE_COLD_COOL3_THRESH) {
		battery->pdata->wire_cold_cool3_thresh = y;
	} else if (tc == WIRE_WARM_CURRENT) {
		battery->pdata->wire_warm_current
					= y > battery->pdata->max_charging_current ?
						battery->pdata->max_charging_current : y;
	} else if (tc == WIRE_COOL1_CURRENT) {
		battery->pdata->wire_cool1_current
					= y > battery->pdata->max_charging_current ?
							battery->pdata->max_charging_current : y;
	} else if (tc == WIRE_COOL2_CURRENT) {
		battery->pdata->wire_cool2_current
					= y > battery->pdata->max_charging_current ?
						battery->pdata->max_charging_current : y;
	} else if (tc == WIRE_COOL3_CURRENT) {
		battery->pdata->wire_cool3_current
					= y > battery->pdata->max_charging_current ?
					 battery->pdata->max_charging_current : y;
	}
}

void store_wireless_menu(struct sec_battery_info *battery, int tc, int y)
{
	if (tc == OVERHEATLIMIT_THRESH) {
		battery->overheatlimit_threshold = y;
	} else if (tc == OVERHEATLIMIT_RECOVERY) {
		battery->overheatlimit_recovery = y;
	} else if (tc == WIRELESS_WARM_OVERHEAT_THRESH) {
		battery->pdata->wireless_warm_overheat_thresh = y;
	} else if (tc == WIRELESS_NORMAL_WARM_THRESH) {
		battery->pdata->wireless_normal_warm_thresh = y;
	} else if (tc == WIRELESS_COOL1_NORMAL_THRESH) {
		battery->pdata->wireless_cool1_normal_thresh = y;
	} else if (tc == WIRELESS_COOL2_COOL1_THRESH) {
		battery->pdata->wireless_cool2_cool1_thresh = y;
	} else if (tc == WIRELESS_COOL3_COOL2_THRESH) {
		battery->pdata->wireless_cool3_cool2_thresh = y;
	} else if (tc == WIRELESS_COLD_COOL3_THRESH) {
		battery->pdata->wireless_cold_cool3_thresh = y;
	} else if (tc == WIRELESS_WARM_CURRENT) {
		battery->pdata->wireless_warm_current
					= y > battery->pdata->max_charging_current ?
					battery->pdata->max_charging_current : y;
	} else if (tc == WIRELESS_COOL1_CURRENT) {
		battery->pdata->wireless_cool1_current
					= y > battery->pdata->max_charging_current ?
					battery->pdata->max_charging_current : y;
	} else if (tc == WIRELESS_COOL2_CURRENT) {
		battery->pdata->wireless_cool2_current
					= y > battery->pdata->max_charging_current ?
					  battery->pdata->max_charging_current : y;
	} else if (tc == WIRELESS_COOL3_CURRENT) {
		battery->pdata->wireless_cool3_current
					= y > battery->pdata->max_charging_current
						? battery->pdata->max_charging_current : y;
	} else if (tc == TX_HIGH_THRESHOLD) {
		battery->pdata->tx_high_threshold = y;
	} else if (tc == TX_HIGH_THRESHOLD_RECOVERY) {
		battery->pdata->tx_high_recovery = y;
	} else if (tc == TX_LOW_THRESHOLD) {
		battery->pdata->tx_low_threshold = y;
	} else if (tc == TX_LOW_THRESHOLD_RECOVERY) {
		battery->pdata->tx_low_recovery = y;
	}
}
#if IS_ENABLED(CONFIG_STEP_CHARGING)
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
void store_dc_step_charging_menu(struct sec_battery_info *battery, int tc,
					char val[MAX_STEP_CHG_STEP + 2][MAX_STEP_CHG_STEP + 2])
{
	int res, i;
	unsigned int dc_step_chg_type = 0;
	int dc_op_mode = get_sec_vote_resultf("DCHG_OP");

	if (dc_op_mode < 0 || !is_dc_higher_ratio_support())
		dc_op_mode = DC_MODE_2TO1;

	for (i = 0; i < battery->dc_step_chg_step; i++)
		dc_step_chg_type |= battery->dc_step_chg_type[i];

	if (tc == DCHG_STEP_CHG_COND_VOL) {
		if (dc_step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
			for (i = 0; i < battery->dc_step_chg_step; i++) {
				res = char_to_int(val[i + 2]);
				battery->pdata->dc_step_chg_cond_vol[battery->pdata->age_step][i] = res;
			}
		}
	} else if (tc == DCHG_STEP_CHG_COND_SOC) {
		if (dc_step_chg_type & STEP_CHARGING_CONDITION_SOC ||
			dc_step_chg_type & STEP_CHARGING_CONDITION_SOC_INIT_ONLY) {
			for (i = 0; i < battery->dc_step_chg_step; i++) {
				res = char_to_int(val[i + 2]);
				battery->pdata->dc_step_chg_cond_soc[battery->pdata->age_step][i] = res;
			}
		}
	} else if (tc == DCHG_STEP_CHG_VAL_VFLOAT) {
		if (dc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
			for (i = 0; i < battery->dc_step_chg_step; i++) {
				res = char_to_int(val[i + 2]);
				battery->pdata->dc_step_chg_val_vfloat[battery->pdata->age_step][i] = res;
			}
		}
	} else if (tc == DCHG_STEP_CHG_VAL_IOUT) {
		for (i = 0; i < battery->dc_step_chg_step; i++) {
			res = char_to_int(val[i + 2]);
			battery->pdata->dc_step_chg_val_iout[battery->pdata->age_step][i] = res;
		}
		if (dc_step_chg_type & STEP_CHARGING_CONDITION_INPUT_CURRENT) {
			for (i = 0; i < (battery->dc_step_chg_step - 1); i++) {
				battery->pdata->dc_step_chg_cond_iin[dc_op_mode][i] =
					battery->pdata->dc_step_chg_val_iout[battery->pdata->age_step][i+1] / 2;
				ca_log("Condition Iin [step %d] %dmA",
					i, battery->pdata->dc_step_chg_cond_iin[dc_op_mode][i]);
			}
		}
	}
}
#endif
void store_step_charging_menu(struct sec_battery_info *battery, int tc,
								char val[MAX_STEP_CHG_STEP + 2][MAX_STEP_CHG_STEP + 2])
{
	int res, i;

	if (tc == STEP_CHG_COND) {
		for (i = 0; i < battery->step_chg_step; i++) {
			res = char_to_int(val[i + 2]);
			battery->pdata->step_chg_cond[battery->pdata->age_step][i] = res;
		}
	} else if (tc == STEP_CHG_COND_CURR) {
		for (i = 0; i < battery->step_chg_step; i++) {
			res = char_to_int(val[i + 2]);
			battery->pdata->step_chg_cond_curr[i] = res;
		}
	} else if (tc == STEP_CHG_CURR) {
		for (i = 0; i < battery->step_chg_step; i++) {
			res = char_to_int(val[i + 2]);
			battery->pdata->step_chg_curr[battery->pdata->age_step][i] = res;
		}
	} else if (tc == STEP_CHG_VFLOAT) {
		for (i = 0; i < battery->step_chg_step; i++) {
			res = char_to_int(val[i + 2]);
			battery->pdata->step_chg_vfloat[battery->pdata->age_step][i] = res;
		}
	}
}
#endif

void store_others_menu(struct sec_battery_info *battery, int tc, int y)
{
	if (tc == CHG_HIGH_TEMP)
		battery->pdata->chg_high_temp = y;
	else if (tc == CHG_HIGH_TEMP_RECOVERY)
		battery->pdata->chg_high_temp_recovery = y;
	else if (tc == CHG_INPUT_LIMIT_CURRENT)
		battery->pdata->chg_input_limit_current = y;
	else if (tc == CHG_CHARGING_LIMIT_CURRENT)
		battery->pdata->chg_charging_limit_current = y;
	else if (tc == MIX_HIGH_TEMP)
		battery->pdata->mix_high_temp = y;
	else if (tc == MIX_HIGH_CHG_TEMP)
		battery->pdata->mix_high_chg_temp = y;
	else if (tc == MIX_HIGH_TEMP_RECOVERY)
		battery->pdata->mix_high_temp_recovery = y;
}

int show_battery_checklist_app_values(struct sec_battery_info *battery, char *buf, unsigned int p_size)
{
	int i = 0;
	struct device_node *np;
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
#if IS_ENABLED(CONFIG_STEP_CHARGING)
	int j = 0;
	bool check;
	unsigned int dc_step_chg_type = 0;
#endif
#endif

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s : battery node is NULL\n", __func__);
	} else {
		get_dts_property(np, "battery,overheatlimit_threshold",
			buf, &p_size, &i, battery->overheatlimit_threshold);
		get_dts_property(np, "battery,overheatlimit_recovery",
			buf, &p_size, &i, battery->overheatlimit_recovery);
		get_dts_property(np, "battery,wire_warm_overheat_thresh",
			buf, &p_size, &i, battery->pdata->wire_warm_overheat_thresh);
		get_dts_property(np, "battery,wire_normal_warm_thresh",
			buf, &p_size, &i, battery->pdata->wire_normal_warm_thresh);
		get_dts_property(np, "battery,wire_cool1_normal_thresh",
			buf, &p_size, &i, battery->pdata->wire_cool1_normal_thresh);
		get_dts_property(np, "battery,wire_cool2_cool1_thresh",
			buf, &p_size, &i, battery->pdata->wire_cool2_cool1_thresh);
		get_dts_property(np, "battery,wire_cool3_cool2_thresh",
			buf, &p_size, &i, battery->pdata->wire_cool3_cool2_thresh);
		get_dts_property(np, "battery,wire_cold_cool3_thresh",
			buf, &p_size, &i, battery->pdata->wire_cold_cool3_thresh);
		get_dts_property(np, "battery,wire_warm_current",
			buf, &p_size, &i, battery->pdata->wire_warm_current);
		get_dts_property(np, "battery,wire_cool1_current",
			buf, &p_size, &i, battery->pdata->wire_cool1_current);
		get_dts_property(np, "battery,wire_cool2_current",
			buf, &p_size, &i, battery->pdata->wire_cool2_current);
		get_dts_property(np, "battery,wire_cool3_current",
			buf, &p_size, &i, battery->pdata->wire_cool3_current);
		get_dts_property(np, "battery,wireless_warm_overheat_thresh",
			buf, &p_size, &i, battery->pdata->wireless_warm_overheat_thresh);
		get_dts_property(np, "battery,wireless_normal_warm_thresh",
			buf, &p_size, &i, battery->pdata->wireless_normal_warm_thresh);
		get_dts_property(np, "battery,wireless_cool1_normal_thresh",
			buf, &p_size, &i, battery->pdata->wireless_cool1_normal_thresh);
		get_dts_property(np, "battery,wireless_cool2_cool1_thresh",
			buf, &p_size, &i, battery->pdata->wireless_cool2_cool1_thresh);
		get_dts_property(np, "battery,wireless_cool3_cool2_thresh",
			buf, &p_size, &i, battery->pdata->wireless_cool3_cool2_thresh);
		get_dts_property(np, "battery,wireless_cold_cool3_thresh",
			buf, &p_size, &i, battery->pdata->wireless_cold_cool3_thresh);
		get_dts_property(np, "battery,wireless_warm_current",
			buf, &p_size, &i, battery->pdata->wireless_warm_current);
		get_dts_property(np, "battery,wireless_cool1_current",
			buf, &p_size, &i, battery->pdata->wireless_cool1_current);
		get_dts_property(np, "battery,wireless_cool2_current",
			buf, &p_size, &i, battery->pdata->wireless_cool2_current);
		get_dts_property(np, "battery,wireless_cool3_current",
			buf, &p_size, &i, battery->pdata->wireless_cool3_current);
		get_dts_property(np, "battery,tx_high_threshold",
			buf, &p_size, &i, battery->pdata->tx_high_threshold);
		get_dts_property(np, "battery,tx_high_recovery",
			buf, &p_size, &i, battery->pdata->tx_high_recovery);
		get_dts_property(np, "battery,tx_low_threshold",
			buf, &p_size, &i, battery->pdata->tx_low_threshold);
		get_dts_property(np, "battery,tx_low_recovery",
			buf, &p_size, &i, battery->pdata->tx_low_recovery);
		get_dts_property(np, "battery,chg_high_temp",
			buf, &p_size, &i, battery->pdata->chg_high_temp);
		get_dts_property(np, "battery,chg_high_temp_recovery",
			buf, &p_size, &i, battery->pdata->chg_high_temp_recovery);
		get_dts_property(np, "battery,chg_input_limit_current",
			buf, &p_size, &i, battery->pdata->chg_input_limit_current);
		get_dts_property(np, "battery,chg_charging_limit_current",
			buf, &p_size, &i, battery->pdata->chg_charging_limit_current);
		get_dts_property(np, "battery,mix_high_temp",
			buf, &p_size, &i, battery->pdata->mix_high_temp);
		get_dts_property(np, "battery,mix_high_chg_temp",
			buf, &p_size, &i, battery->pdata->mix_high_chg_temp);
		get_dts_property(np, "battery,mix_high_temp_recovery",
			buf, &p_size, &i, battery->pdata->mix_high_temp_recovery);

#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
#if IS_ENABLED(CONFIG_STEP_CHARGING)
		for (j = 0; j < battery->dc_step_chg_step; j++)
			dc_step_chg_type |= battery->dc_step_chg_type[j];

		if (dc_step_chg_type & STEP_CHARGING_CONDITION_VOLTAGE) {
			check = of_property_read_bool(np, "battery,dc_step_chg_cond_vol");
			if (check) {
				for (j = 0; j < battery->dc_step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ",
					battery->pdata->dc_step_chg_cond_vol[battery->pdata->age_step][j]);
			}
		} else {
			check = of_property_read_bool(np, "battery,dc_step_chg_cond_vol");
			if (check) {
				for (j = 0; j < battery->dc_step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ", -1);
			}
		}

		if (dc_step_chg_type & STEP_CHARGING_CONDITION_SOC ||
			dc_step_chg_type & STEP_CHARGING_CONDITION_SOC_INIT_ONLY) {
			check = of_property_read_bool(np, "battery,dc_step_chg_cond_soc");
			if (check) {
				for (j = 0; j < battery->dc_step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ",
					battery->pdata->dc_step_chg_cond_soc[battery->pdata->age_step][j]);
			}
		} else {
			check = of_property_read_bool(np, "battery,dc_step_chg_cond_soc");
			if (check) {
				for (j = 0; j < battery->dc_step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ", -1);
			}
		}

		if (dc_step_chg_type & STEP_CHARGING_CONDITION_FLOAT_VOLTAGE) {
			check = of_property_read_bool(np, "battery,dc_step_chg_val_vfloat");
			if (check) {
				for (j = 0; j < battery->dc_step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ",
					battery->pdata->dc_step_chg_val_vfloat[battery->pdata->age_step][j]);
			}
		} else {
			check = of_property_read_bool(np, "battery,dc_step_chg_val_vfloat");
			if (check) {
				for (j = 0; j < battery->dc_step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ", -1);
			}
		}

		check = of_property_read_bool(np, "battery,dc_step_chg_val_iout");
		if (check) {
			for (j = 0; j < battery->dc_step_chg_step; j++)
				i += scnprintf(buf + i, p_size - i, "%d ",
				battery->pdata->dc_step_chg_val_iout[battery->pdata->age_step][j]);
		}

		check = of_property_read_bool(np, "battery,step_chg_cond");
		if (check) {
			for (j = 0; j < battery->step_chg_step; j++)
				i += scnprintf(buf + i, p_size - i, "%d ",
				battery->pdata->step_chg_cond[battery->pdata->age_step][j]);
		}


		check = of_property_read_bool(np, "battery,step_chg_cond_curr");
			if (check) {
				for (j = 0; j < battery->step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ",
					battery->pdata->step_chg_cond_curr[j]);
		}

		check = of_property_read_bool(np, "battery,step_chg_curr");
			if (check) {
				for (j = 0; j < battery->step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ",
					battery->pdata->step_chg_curr[battery->pdata->age_step][j]);
		}

		check = of_property_read_bool(np, "battery,step_chg_vfloat");
			if (check) {
				for (j = 0; j < battery->step_chg_step; j++)
					i += scnprintf(buf + i, p_size - i, "%d ",
					battery->pdata->step_chg_vfloat[battery->pdata->age_step][j]);
		}
#endif
#endif
	}

	return i;
}

void store_battery_checklist_app_values(struct sec_battery_info *battery, const char *buf)
{

	char type = buf[0];
	int i, j, cnt;
	int tc, y;
	char val[MAX_STEP_CHG_STEP + 2][MAX_STEP_CHG_STEP + 2];

	cnt = i = j = 0;
	for (i = 0; buf[i]; i++) {
		if (buf[i] == ' ') {
			val[cnt][j] = 0;
			cnt++;
			j = 0;
			continue;
		}
		val[cnt][j] = buf[i];
		j++;
	}
	val[cnt][j] = 0;

	tc = char_to_int(val[1]);
	y = char_to_int(val[2]);

	switch (type) {
	case 'w': //wire menu
		store_wire_menu(battery, tc, y);
		break;
	case 'l': //wireless menu
		store_wireless_menu(battery, tc, y);
		break;
#if defined(CONFIG_STEP_CHARGING)
#if IS_ENABLED(CONFIG_DIRECT_CHARGING)
	case 'd': //dc step charging menu
		store_dc_step_charging_menu(battery, tc, val);
		break;
#endif
	case 's': //step charging menu
		store_step_charging_menu(battery, tc, val);
		break;
#endif
	case 'o': //others (mix temp, chg_temp, dchg temp)
		store_others_menu(battery, tc, y);
		break;
	}
}

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#define CA_SYSFS_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = show_attrs,					\
	.store = store_attrs,					\
}

static struct device_attribute ca_attr[] = {
	CA_SYSFS_ATTR(batt_checklist_app),
};

enum sb_ca_attrs {
	BATT_CHECKLIST_APP = 0,
};

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - ca_attr;
	ssize_t count = 0;

	switch (offset) {
	case BATT_CHECKLIST_APP:
		count = show_battery_checklist_app_values(battery, buf, PAGE_SIZE);
		break;
	default:
		break;
	}

	return count;
}

static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - ca_attr;

	switch (offset) {
	case BATT_CHECKLIST_APP:
		store_battery_checklist_app_values(battery, buf);
		break;
	default:
		break;
	}
	return count;
}

static int sb_noti_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	return 0;
}

void sb_ca_init(struct device *parent)
{
	struct sb_ca *ca;
	int ret = 0;

	ca = kzalloc(sizeof(struct sb_ca), GFP_KERNEL);
	ca->name = "sb-ca";
	ret = sb_sysfs_add_attrs(ca->name, ca, ca_attr, ARRAY_SIZE(ca_attr));
	ca_log("sb_sysfs_add_attrs ret = %s\n", (ret) ? "fail" : "success");

	ret = sb_notify_register(&ca->nb, sb_noti_handler, ca->name, SB_DEV_MODULE);
	ca_log("sb_notify_register ret = %s\n", (ret) ? "fail" : "success");

}
EXPORT_SYMBOL(sb_ca_init);
