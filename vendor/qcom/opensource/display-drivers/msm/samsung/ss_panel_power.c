/*
 * =================================================================
 *
 *
 *	Description:  samsung display common file
 *	Company:  Samsung Electronics
 *
 * ================================================================
 *
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

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include "ss_panel_power.h"

char *panel_power_str[MAX_PANEL_POWERS] = {
	[PANEL_POWERS_ON_PRE_LP11] = "power_on_pre_lp11_seq",
	[PANEL_POWERS_ON_POST_LP11] = "power_on_post_lp11_seq",
	[PANEL_POWERS_OFF_PRE_LP11] = "power_off_pre_lp11_seq",
	[PANEL_POWERS_OFF_POST_LP11] = "power_off_post_lp11_seq",
	[PANEL_POWERS_LPM_ON] = "lpm_on_seq",
	[PANEL_POWERS_LPM_OFF] = "lpm_off_seq",
	[PANEL_POWERS_ON_MIDDLE_OF_LP_HS_CLK] = "power_on_middle_of_lp_hs_clk_seq",
	[PANEL_POWERS_OFF_MIDDLE_OF_LP_HS_CLK] = "power_off_middle_of_lp_hs_clk_seq",
	[PANEL_POWERS_AOLCE_ON] = "aolce_on_seq",
	[PANEL_POWERS_AOLCE_OFF] = "aolce_off_seq",
	[PANEL_POWERS_EVLDD_ELVSS] = "elvdd_elvdd_seq",
};

struct regulator_voltage {
	int min_uV;
	int max_uV;
};

#define REGULATOR_STATES_NUM    (PM_SUSPEND_MAX + 1)

struct regulator_tmp {
        struct device *dev;
        struct list_head list;
        unsigned int always_on:1;
        unsigned int bypass:1;
        unsigned int device_link:1;
        int uA_load;
        unsigned int enable_count;
        unsigned int deferred_disables;
        struct regulator_voltage voltage[REGULATOR_STATES_NUM];
        const char *supply_name;
        struct device_attribute dev_attr;
        struct regulator_dev *rdev;
        struct dentry *debugfs;
};

static bool check_regulator_dev(struct pwr_node *pwr)
{
	struct regulator_tmp *reg = pwr ? (struct regulator_tmp *)pwr->reg : NULL;
	struct regulator_dev *rdev = reg ? reg->rdev : NULL;
	
	if (rdev == NULL)
		return 0;
	else
		return 1;
}

static int ss_panel_parse_pwr_seq(struct samsung_display_driver_data *vdd,
		struct pwr_node *pwr, struct device_node *np)
{
	int rc = 0;
	int i;
	u32 len = 0;
	u32 arr_32[32];
	const u32 *arr;

	arr = of_get_property(np, "pwr,seq", &len);
	if (!arr) {
		LCD_INFO(vdd, "no pwr,seq\n");
		return 0;
	}

	if (len & 0x1) {
		LCD_ERR(vdd, "syntax error (len: %d)\n", len);
		return -EINVAL;
	}

	len = len / sizeof(u32);
	if (len > 32) {
		LCD_ERR(vdd, "too large len(%d)\n", len);
		return -EINVAL;
	}

	pwr->pwr_seq_count = len / 2;
	pwr->pwr_seq = kcalloc(pwr->pwr_seq_count,
			sizeof(struct panel_pwr_seq), GFP_KERNEL);
	if (!pwr->pwr_seq) {
		LCD_ERR(vdd, "fail to allocate pwr,seq, len: %d\n", len);
		return -ENOMEM;
	}

	rc = of_property_read_u32_array(np, "pwr,seq", arr_32, len);
	if (rc) {
		LCD_ERR(vdd, "fail to parse pwr,seq, rc: %d\n", rc);
		pwr->pwr_seq_count = 0;
		kfree(pwr->pwr_seq);
		return -EINVAL;
	}

	for (i = 0; i < len; i += 2) {
		pwr->pwr_seq[i / 2].onoff = arr_32[i];
		pwr->pwr_seq[i / 2].post_delay = arr_32[i + 1];
	}

	return 0;
}

/* devm_regulator_get returns dummy regulator even it cannot get regulator resource. */
static int ss_panel_check_reg(struct samsung_display_driver_data *vdd, char *reg_name)
{
	struct regulator *reg;
	int rc;
	static int reg_fail_cnt;

	reg = regulator_get_optional(NULL, reg_name);
	rc = PTR_ERR_OR_ZERO(reg);
	if (rc) {
		if (++reg_fail_cnt < 30) {
			LCD_INFO(vdd, "regulator(%s) is not allowed yet, (%d)\n", reg_name, rc);
			usleep_range(20000, 20000);
			return -EPROBE_DEFER;
		}

		LCD_ERR(vdd, "need to check reg_fail (%d), only dummy regulator is returned..\n",
				reg_fail_cnt);
		ss_display_panic(vdd, "dummy regulator");
	} else {
		LCD_INFO(vdd, "pass Panel Reg check, name=%s\n", reg_name);
		regulator_put(reg);
	}

	return 0;
}


static void ss_panel_remove_powers_pmic(struct samsung_display_driver_data *vdd)
{
	int i, j;
	struct panel_power *powers;
	struct pwr_node *pwr;

	for (i = 0; i < MAX_PANEL_POWERS; i++) {
		powers = &vdd->panel_powers[i];

		for (j = 0, pwr = powers->pwrs; j < powers->pwr_count; j++, pwr++) {
			if (ss_is_pmic_type(pwr->type)) {
				if (pwr->reg) {
					LCD_INFO(vdd, "[%s] remove [%s] [%d]\n", panel_power_str[i], pwr->name, pwr->use_shared);
					if (!pwr->use_shared)
						devm_regulator_put(pwr->reg);
					pwr->reg = NULL;
				}
			}
		}
	}
}

static int ss_panel_get_power_pmic(struct samsung_display_driver_data *vdd,
				struct pwr_node *pwr, struct device *dev)
{
	struct panel_power *powers;
	struct pwr_node *other_pwr;
	struct regulator *reg;
	int rc, i, j;

	for (i = 0; i < MAX_PANEL_POWERS; i++) {
		powers = &vdd->panel_powers[i];

		for (j = 0, other_pwr = powers->pwrs; j < powers->pwr_count; j++, other_pwr++) {
			if (ss_is_pmic_type(other_pwr->type) &&
					other_pwr->name &&
					!strcmp(other_pwr->name, pwr->name) &&
					other_pwr->reg) {
				/* To prevent regulator->enable_count underflow,
				 * prevent duplciated regulator_get.
				 */
				LCD_INFO(vdd, "%s share regulator handler\n", pwr->name);
				pwr->reg = other_pwr->reg;
				pwr->use_shared = true;
				return 0;
			}
		}
	}

	rc = ss_panel_check_reg(vdd, pwr->name);
	if (rc)
		return rc;

	reg = devm_regulator_get(dev, pwr->name);
	rc = PTR_ERR_OR_ZERO(reg);
	if (rc) {
		LCD_ERR(vdd, "fail to get regulator[%s], rc: %d\n", pwr->name, rc);
		return rc;
	}
	pwr->reg = reg;

	if (!check_regulator_dev(pwr)) {
		LCD_ERR(vdd, "No regulator device for [%s]\n", pwr->name);
		devm_regulator_put(pwr->reg);
		pwr->reg = NULL;
		return -EPROBE_DEFER;
	}

	return 0;
}

static int ss_panel_parse_power_pmic(struct samsung_display_driver_data *vdd,
		struct pwr_node *pwr, struct device_node *np, struct device *dev)
{
	const char *data;
	int tmp;
	int rc;

	rc = of_property_read_string(np, "pwr,name", &data);
	if (rc) {
		LCD_ERR(vdd, "fail to read name, rc=%d\n", rc);
		return -EINVAL;
	}
	pwr->name = kzalloc(strlen(data) + 1, GFP_KERNEL);
	if (!pwr->name) {
		LCD_ERR(vdd, "fail to allocate name(%lu)\n", strlen(data) + 1);
		return -ENOMEM;
	}
	sprintf(pwr->name, "%s", data);

	rc = of_property_read_u32(np, "pwr,voltage", &tmp);
	if (rc) {
		LCD_INFO(vdd, "no voltage [%s], set 0\n", pwr->name);
		tmp = 0;
	}
	pwr->voltage = tmp;

	rc = of_property_read_u32(np, "pwr,current", &tmp);
	if (rc) {
		LCD_INFO(vdd, "no current [%s]\n", pwr->name);
		tmp = -1;
	}
	pwr->current_uA = tmp;

	rc = of_property_read_u32(np, "pwr,load", &tmp);
	if (rc) {
		LCD_INFO(vdd, "no load [%s]\n", pwr->name);
		tmp = -1;
	}
	pwr->load = tmp;

	rc = ss_panel_parse_pwr_seq(vdd, pwr, np);
	if (rc)
		LCD_INFO(vdd, "no seq [%s]\n", pwr->name);

	/* TBR: move getting power resource code to other position? */
	rc = ss_panel_get_power_pmic(vdd, pwr, dev);
	if (rc) {
		LCD_ERR(vdd, "fail to get pmic [%s]\n", pwr->name);
		return rc;
	}

	LCD_INFO(vdd, "parsed power pmic(%s)\n", pwr->name);

	return 0;
}

static int ss_panel_parse_power_gpio(struct samsung_display_driver_data *vdd,
		struct pwr_node *pwr, struct device_node *np)
{
	const char *data;

	if (!of_property_read_string(np, "pwr,name", &data)) {
		pwr->name = kzalloc(strlen(data) + 1, GFP_KERNEL);
		if (!pwr->name) {
			LCD_ERR(vdd, "fail to allocate name(%lu)\n", strlen(data) + 1);
			return -ENOMEM;
		}

		sprintf(pwr->name, "%s", data);
	}

	pwr->gpio = ss_wrapper_of_get_named_gpio(np, "pwr,gpio", 0);
	if (!ss_gpio_is_valid(pwr->gpio)) {
		LCD_ERR(vdd, "fail to parse gpio\n");
		return -EINVAL;
	}

	return ss_panel_parse_pwr_seq(vdd, pwr, np);
}

static int ss_panel_parse_power_panel_specific(struct samsung_display_driver_data *vdd,
		struct panel_power *power, struct pwr_node *pwr, struct device_node *np)
{
	if (!power->parse_cb) {
		LCD_INFO(vdd, "Caution: no panel specific parse callback\n");
		return 0;
	}

	return power->parse_cb(vdd, pwr, np);
}

char *panel_pwr_type_name[PANEL_PWR_MAX] = {
	[PANEL_PWR_PMIC] = "pmic",
	[PANEL_PWR_PMIC_SPECIFIC] = "pmic_specific",
	[PANEL_PWR_GPIO] = "gpio",
	[PANEL_PWR_GPIO_SPECIFIC] = "gpio_specific",
	[PANEL_PWR_PANEL_SPECIFIC] = "panel_specific",
};

/* TBR : merge with ss_panel_parse_regulator*/
static int ss_panel_parse_power(struct samsung_display_driver_data *vdd,
				struct panel_power *power,
				struct device_node *panel_node,
				struct device *dev, char *keystring)
{
	struct device_node *child_np, *np;
	int rc;
	const char *data;
	int count = 0;
	struct pwr_node *pwr;
	int type;
	int i;

	np = of_get_child_by_name(panel_node, keystring);
	if (!np) {
		LCD_INFO(vdd, "no %s node to parse\n", keystring);
		return 0; /* return 0 for PBA booting */
	}

	for_each_child_of_node(np, child_np)
		count++;

	if (count == 0) {
		LCD_ERR(vdd, "No child node of %s\n", keystring);
		return -ENODEV;
	}
	LCD_INFO(vdd, "%s count is %d\n", keystring, count);

	power->pwrs = kcalloc(count, sizeof(struct pwr_node), GFP_KERNEL);
	if (!power->pwrs) {
		LCD_ERR(vdd, "fail to kcalloc for panel regulators [%s]\n",
				keystring);
		return -ENOMEM;
	}
	power->pwr_count = count;

	count = 0;
	for_each_child_of_node(np, child_np) {
		pwr = &power->pwrs[count++];

		rc = of_property_read_string(child_np, "pwr,type", &data);
		if (rc) {
			LCD_ERR(vdd, "fail to read type. rc=%d\n", rc);
			continue;
		}

		for (type = 0; type < PANEL_PWR_MAX; type++) {
			if (!strcmp(data, panel_pwr_type_name[type])) {
				pwr->type = type;
				break;
			}
		}

		if (type == PANEL_PWR_MAX) {
			LCD_ERR(vdd, "invalid type (%s) for [%s]\n",
					data, keystring);
			return -EINVAL;
		}

		if (ss_is_pmic_type(pwr->type))
			rc = ss_panel_parse_power_pmic(vdd, pwr, child_np, dev);
		else if (pwr->type == PANEL_PWR_GPIO)
			rc = ss_panel_parse_power_gpio(vdd, pwr, child_np);
		else if (pwr->type == PANEL_PWR_GPIO_SPECIFIC)
			rc = ss_panel_parse_power_gpio(vdd, pwr, child_np);
		else if (pwr->type == PANEL_PWR_PANEL_SPECIFIC)
			rc = ss_panel_parse_power_panel_specific(vdd, power, pwr, child_np);
		if (rc)
			goto err_get_resource;
	}

	return 0;

err_get_resource:
	LCD_ERR(vdd, "fail to get resource, type: %d, rc: %d, count: %d for [%s]\n",
			pwr->type, rc, count, keystring);

	for (i = 0; i < count - 1; i++) {
		pwr = &power->pwrs[i];

		if (ss_is_pmic_type(pwr->type)) {
			devm_regulator_put(pwr->reg);
			pwr->reg = NULL;
		}
	}

	return rc;
}

/* Should be called in dsi_panel_vreg_get(), part of qc display probe sequence,
 * to return -EPROBE_DEFER in case.
 * But panel->panel_private is not connected to vdd yet at that timing,
 * so get proper vdd pointer from panel name.
 * TBR: separate parsing powers and getting power resource parts.
 */
int ss_panel_parse_powers(struct samsung_display_driver_data *vdd,
		struct device_node *panel_node, struct device *dev)
{
	int i;
	int rc;

	if (!vdd || !panel_node || !dev) {
		LCD_ERR_NOVDD("invalid input: vdd: %s, np: %s, dev: %s\n",
				vdd ? "ok" : NULL,
				panel_node ? "ok" : NULL,
				dev ? "ok" : NULL);
		return -ENODEV;
	}

	for (i = 0; i < MAX_PANEL_POWERS; i++) {
		rc = ss_panel_parse_power(vdd, &vdd->panel_powers[i],
					panel_node, dev, panel_power_str[i]);
		if (rc) {
			ss_panel_remove_powers_pmic(vdd);
			return rc;
		}
	}

	/* TBR: move below vote upt and getting regulator code to other parts,
	 * This function should only parse data from panel dtsi.
	 */

	/*
	 * Vote on panel regulator is added to make sure panel regulators
	 * are ON for cont-splash enabled usecase.
	 * This panel regulator vote will be removed only in:
	 *	1) device suspend when cont-splash is enabled.
	 *	2) cont_splash_res_disable() when cont-splash is disabled.
	 * For GKI, adding this vote will make sure that sync_state
	 * kernel driver doesn't disable the panel regulators after
	 * dsi probe is complete.
	 */
	ss_panel_power_pmic_vote(vdd, true);

	return 0;
}

/* TBR: refactoring that
 * - check necessaty of min_voltage, enable, and etc variables.
 * - split functions: set votage, set load, set current, and enable/disable
 * - merge pmic specific type for eureka to pmic type.
 */
static int ss_panel_power_ctrl_pmic(struct samsung_display_driver_data *vdd,
			struct pwr_node *pwr, bool enable)
{
	int num_of_v = 0;
	int i, rc = 0;
	int min_voltage, len = 0;
	char pBuf[256];

	memset(pBuf, 0x00, 256);

	len += snprintf(pBuf + len, 256 - len, "[%s]", pwr->name);

	if (enable) {
		if (pwr->load >= 0) {
			len += snprintf(pBuf + len, 256 - len, " / load: %d", pwr->load);
			rc = regulator_set_load(pwr->reg, pwr->load);
			if (rc) {
				LCD_ERR(vdd, "fail to set optimum mode(%s), rc: %d\n",
				       pwr->name, rc);
				return rc;
			}
		}

		min_voltage = enable ? pwr->voltage : 0;
		len += snprintf(pBuf + len, 256 - len, " / set voltage: %duV - %duV", min_voltage, pwr->voltage);
		num_of_v = regulator_count_voltages(pwr->reg);
		if (num_of_v > 0) {
			rc = regulator_set_voltage(pwr->reg, min_voltage, pwr->voltage);
			if (rc) {
				LCD_ERR(vdd, "fail to set voltage(%s), rc=%d\n",
						pwr->name, rc);
				return rc;
			}
		} else {
			LCD_INFO(vdd, "[%s] num_of_v is %d, do not set_voltage..\n", pwr->name, num_of_v);
		}
	}

	for (i = 0; i < pwr->pwr_seq_count; i++) {
		if (pwr->pwr_seq[i].onoff)
			rc = regulator_enable(pwr->reg);
		else
			rc = regulator_disable(pwr->reg);

		if (rc) {
			LCD_ERR(vdd, "[%s] fail to %s, rc=%d\n", pwr->name,
					pwr->pwr_seq[i].onoff ? "enable" : "disable",
					rc);
			return rc;
		}

		len += snprintf(pBuf + len, 256 - len, " / %s (delay: %dms)",
				pwr->pwr_seq[i].onoff ? "enable" : "disable",
				pwr->pwr_seq[i].post_delay);

		if (pwr->pwr_seq[i].post_delay)
			usleep_range(pwr->pwr_seq[i].post_delay * 1000,
					pwr->pwr_seq[i].post_delay * 1100);
	}

	if (!enable) {
		if (pwr->load >= 0) {
			len += snprintf(pBuf + len, 256 - len, " / load: %d", pwr->load);
			rc = regulator_set_load(pwr->reg, pwr->load);
			if (rc) {
				LCD_ERR(vdd, "fail to set optimum mode(%s), rc: %d\n",
				       pwr->name, rc);
				return rc;
			}
		}

		min_voltage = enable ? pwr->voltage : 0;
		len += snprintf(pBuf + len, 256 - len, " / set voltage: %duV - %duV", min_voltage, pwr->voltage);
		num_of_v = regulator_count_voltages(pwr->reg);
		if (num_of_v > 0) {
			rc = regulator_set_voltage(pwr->reg, min_voltage, pwr->voltage);
			if (rc) {
				LCD_ERR(vdd, "fail to set voltage(%s), rc=%d\n",
						pwr->name, rc);
				return rc;
			}
		} else {
			LCD_INFO(vdd, "[%s] num_of_v is %d, do not set_voltage..\n", pwr->name, num_of_v);
		}
	}

	if (pwr->current_uA >= 0) {
		len += snprintf(pBuf + len, 256 - len, " / current limit: %d", pwr->current_uA);

#if IS_ENABLED(CONFIG_SEC_PM)
		rc = regulator_set_current_limit(pwr->reg, pwr->current_uA, pwr->current_uA);
		if (rc) {
			LCD_ERR(vdd, "[%s] fail to set current limit(%duA)\n",
					pwr->name, pwr->current_uA);
			return rc;
		}
#else
		LCD_ERR(vdd, "CONFG_SEC_PM is not Enabled\n");
#endif
	}



	LCD_INFO(vdd, "%s\n", pBuf);

	return 0;
}

static int ss_panel_power_ctrl_gpio(struct samsung_display_driver_data *vdd,
			struct pwr_node *pwr)
{
	int rc;
	int i;

	if (!vdd || !pwr) {
		LCD_ERR(vdd, "invalid val(vdd:%s, pwr: %s)\n",
				vdd ? "ok" : "null",
				pwr ? "ok" : "null");
		return -ENODEV;
	}

	if (!gpio_is_valid(pwr->gpio)) {
		LCD_ERR(vdd, "invalid gpio(%d), name: [%s]\n",
				pwr->gpio, pwr->name ? pwr->name : "");
		return -EINVAL;
	}

	if (pwr->pwr_seq_count == 0) {
		LCD_ERR(vdd, "pwr_seq_count is zero (gpio(%d), name: [%s])\n",
				pwr->gpio, pwr->name ? pwr->name : "");
		return -EINVAL;
	}

	rc = gpio_direction_output(pwr->gpio, pwr->pwr_seq[0].onoff);
	if (rc) {
		LCD_ERR(vdd, "fail to set gpio(%d) dir, rc=%d, name: [%s]\n",
				pwr->gpio, rc, pwr->name ? pwr->name : "");
		return rc;
	}

	for (i = 0; i < pwr->pwr_seq_count; i++) {
		LCD_INFO(vdd, "[%s] set gpio(%d) %s (post delay: %dms)\n",
				pwr->name, pwr->gpio,
				pwr->pwr_seq[i].onoff ? "high" : "low",
				pwr->pwr_seq[i].post_delay);
		gpio_set_value(pwr->gpio, pwr->pwr_seq[i].onoff);

		if (pwr->pwr_seq[i].post_delay)
			usleep_range(pwr->pwr_seq[i].post_delay * 1000,
					pwr->pwr_seq[i].post_delay * 1100);
	}

	return 0;
}

static int ss_panel_power_ctrl_gpio_specific(struct samsung_display_driver_data *vdd,
			struct panel_power *power, struct pwr_node *pwr, bool enable)
{

	if (!vdd || !pwr || !power) {
		LCD_ERR(vdd, "invalid val(vdd:%s, pwr: %s, power: %s)\n",
				vdd ? "ok" : "null",
				pwr ? "ok" : "null",
				power ? "ok" : "null");
		return -ENODEV;
	}

	if (!power->ctrl_cb) {
		LCD_ERR(vdd, "no panel specific pmic ctrl callback\n");
		return -EINVAL;
	}

	return power->ctrl_cb(vdd, pwr, enable);
}

static int ss_panel_power_ctrl_pmic_specific(struct samsung_display_driver_data *vdd,
			struct panel_power *power, struct pwr_node *pwr, bool enable)
{

	if (!vdd || !pwr || !power) {
		LCD_ERR(vdd, "invalid val(vdd:%s, pwr: %s, power: %s)\n",
				vdd ? "ok" : "null",
				pwr ? "ok" : "null",
				power ? "ok" : "null");
		return -ENODEV;
	}

	if (!power->ctrl_cb) {
		LCD_ERR(vdd, "no panel specific pmic ctrl callback\n");
		return -EINVAL;
	}

	return power->ctrl_cb(vdd, pwr, enable);
}

static int ss_panel_power_ctrl_panel_specific(struct samsung_display_driver_data *vdd,
			struct panel_power *power, struct pwr_node *pwr, bool enable)
{

	if (!vdd || !pwr || !power) {
		LCD_ERR(vdd, "invalid val(vdd:%s, pwr: %s, power: %s)\n",
				vdd ? "ok" : "null",
				pwr ? "ok" : "null",
				power ? "ok" : "null");
		return -ENODEV;
	}

	if (!power->ctrl_cb) {
		LCD_ERR(vdd, "no panel specific power ctrl callback\n");
		return -EINVAL;
	}

	return power->ctrl_cb(vdd, pwr, enable);
}

int ss_panel_power_force_disable(struct samsung_display_driver_data *vdd,
			struct panel_power *power)
{
	struct pwr_node *pwr;
	int i;
	int rc = 0;

	if (!power->pwrs) {
		LCD_ERR(vdd, "No pwrs for force_disable \n");
		return rc;
	}

	for (i = 0, pwr = power->pwrs; i < power->pwr_count; i++, pwr++) {
		LCD_INFO(vdd, "[%s] force disable..\n", pwr->name);
		rc = regulator_force_disable(pwr->reg);
		if (rc)
			LCD_ERR(vdd, "force_disable failed, ret=%d\n", rc);
	}

	return rc;
}

int ss_panel_power_ctrl(struct samsung_display_driver_data *vdd,
			struct panel_power *power, bool enable)
{
	struct pwr_node *pwr;
	int i;

	if (!ss_panel_attach_get(vdd)) {
		LCD_INFO(vdd, "PBA booting (%pS)\n", __builtin_return_address(0));

		if (ss_is_ub_connected(vdd) == UB_CONNECTED)
			LCD_ERR(vdd, "PBA booting but UB connected (mipi error)\n");

		return -EPERM;
	}

	if (enable && vdd->is_factory_mode && !ss_is_ub_connected(vdd)) {
		LCD_INFO(vdd, "ub disconnected (%pS)\n", __builtin_return_address(0));
		return -EPERM;
	}

	for (i = 0, pwr = power->pwrs; i < power->pwr_count; i++, pwr++) {
		if (pwr->type == PANEL_PWR_PMIC)
			ss_panel_power_ctrl_pmic(vdd, pwr, enable);
		else if (pwr->type == PANEL_PWR_PMIC_SPECIFIC)
			ss_panel_power_ctrl_pmic_specific(vdd, power, pwr, enable);
		else if (pwr->type == PANEL_PWR_GPIO)
			ss_panel_power_ctrl_gpio(vdd, pwr);
		else if (pwr->type == PANEL_PWR_GPIO_SPECIFIC)
			ss_panel_power_ctrl_gpio_specific(vdd, power, pwr, enable);
		else if (pwr->type == PANEL_PWR_PANEL_SPECIFIC)
			ss_panel_power_ctrl_panel_specific(vdd, power, pwr, enable);
	}

	return 0;
}

int ss_panel_power_on(struct samsung_display_driver_data *vdd,
			struct panel_power *power)
{
	struct pwr_node *pwr;
	int i;
	int rc = 0;

	/* vdd->panel_dead flag is set when esd interrupt occurs,
	 * and should be released when display connection is confirmed and
	 * before start to read DDI MTP during dipslay on sequence.
	 */
	if (vdd->panel_dead) {
		if (ss_is_ub_connected(vdd)) {
			LCD_INFO(vdd, "panel recovery is done, release panel_dead\n");
			vdd->panel_dead = false;
		} else {
			LCD_INFO(vdd, "ub is disconnected yet, panel is dead (%d)\n", vdd->panel_dead);
		}
	}

	// TBR: modify below qc code as ss code
	rc = dsi_panel_set_pinctrl_state(GET_DSI_PANEL(vdd), true);
	if (rc)
		LCD_ERR(vdd, "failed to set pinctrl, rc=%d\n", rc);

	ss_panel_power_ctrl(vdd, power, true);

	for (i = 0, pwr = power->pwrs; i < power->pwr_count; i++, pwr++)
		if (pwr->name && !strcmp(pwr->name, "panel_reset"))
			vdd->reset_time_64 = ktime_get();

	return rc;

}

// TBR: need to put ref count??? refer to dsi_pwr_enable_regulator
/* refer to dsi_panel_power_on */
int ss_panel_power_on_pre_lp11(struct samsung_display_driver_data *vdd)
{
	return ss_panel_power_on(vdd, &vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11]);
}

// TBR: need to put ref count??? refer to dsi_pwr_enable_regulator
/* refer to dsi_panel_power_on */
int ss_panel_power_on_post_lp11(struct samsung_display_driver_data *vdd)
{
	return ss_panel_power_on(vdd, &vdd->panel_powers[PANEL_POWERS_ON_POST_LP11]);
}

int ss_panel_power_on_middle_lp_hs_clk(void)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	return ss_panel_power_on(vdd, &vdd->panel_powers[PANEL_POWERS_ON_MIDDLE_OF_LP_HS_CLK]);
}

int ss_panel_power_off_middle_lp_hs_clk(void)
{
	struct samsung_display_driver_data *vdd = ss_get_vdd(PRIMARY_DISPLAY_NDX);

	return ss_panel_power_on(vdd, &vdd->panel_powers[PANEL_POWERS_OFF_MIDDLE_OF_LP_HS_CLK]);
}

/* for continuous splash mode, vote up in booting, then vote down in splash_disable */
int ss_panel_power_pmic_vote(struct samsung_display_driver_data *vdd, bool vote_up)
{
	struct panel_power *powers[2] = {
		&vdd->panel_powers[PANEL_POWERS_ON_PRE_LP11],
		&vdd->panel_powers[PANEL_POWERS_ON_POST_LP11]
	};
	struct pwr_node *pwr;
	int num_of_v = 0;
	int i, j;
	int rc = 0;

	for (i = 0; i < 2; i++) {
		for (j = 0, pwr = powers[i]->pwrs; j < powers[i]->pwr_count; j++, pwr++) {
			if (!ss_is_pmic_type(pwr->type))
				continue;

			if (!pwr->pwr_seq)
				continue;

			if (vote_up) {
				rc = regulator_set_load(pwr->reg, pwr->load);
				if (rc < 0)
					LCD_ERR(vdd, "fail to set optimum mode(%s), rc: %d\n",
							pwr->name, rc);

				num_of_v = regulator_count_voltages(pwr->reg);
				if (num_of_v > 0) {
					rc = regulator_set_voltage(pwr->reg,
							pwr->voltage, pwr->voltage);
					if (rc)
						LCD_ERR(vdd, "fail to set voltage(%s), rc: %d\n",
								pwr->name, rc);
				} else {
					LCD_WARN(vdd, "[%s] num_of_v is %d, do not set_voltage..\n",
							pwr->name, num_of_v);
				}

				rc = regulator_enable(pwr->reg);
				if (rc)
					LCD_ERR(vdd, "fail to enable %s, rc=%d\n",
							pwr->name, rc);
			} else {
				regulator_disable(pwr->reg);
				regulator_set_load(pwr->reg, pwr->load);
			}
			if (rc) {
				LCD_ERR(vdd, "fail to %s %s, rc=%d\n",
						vote_up ? "enable" : "disable",
						pwr->name, rc);
				return -EINVAL;
			} else {
				LCD_INFO(vdd, "[%d] Success to vote %s\n", i, pwr->name);
			}
		}
	}

	return 0;
}

int ss_panel_power_off(struct samsung_display_driver_data *vdd,
			struct panel_power *power)
{
	int rc = 0;

	// TBR: modify below qc code as ss code
	rc = dsi_panel_set_pinctrl_state(GET_DSI_PANEL(vdd), false);
	if (rc)
		LCD_ERR(vdd, "failed to set pinctrl, rc=%d\n", rc);

	ss_panel_power_ctrl(vdd, power, false);

	return rc;
}
/* refer to  dsi_panel_power_off */
int ss_panel_power_off_pre_lp11(struct samsung_display_driver_data *vdd)
{
	return ss_panel_power_off(vdd, &vdd->panel_powers[PANEL_POWERS_OFF_PRE_LP11]);
}

/* refer to  dsi_panel_power_off */
int ss_panel_power_off_post_lp11(struct samsung_display_driver_data *vdd)
{
	return ss_panel_power_off(vdd, &vdd->panel_powers[PANEL_POWERS_OFF_POST_LP11]);
}
