/*
 *  sb_full_soc.c
 *  Samsung Mobile Battery Full SoC Module
 *
 *  Copyright (C) 2023 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "sb_full_soc.h"
#include "sec_battery.h"
#include "battery_logger.h"

struct sb_full_soc {
	int full_capacity;
	unsigned int full_cap_event;

	bool is_eu_eco_rechg;
	bool eu_eco_rechg_state;
	sec_battery_recharge_condition_t old_recharge_condition_type;
	unsigned int old_recharge_condition_soc;

	bool eu_eco_chg_done;

	struct mutex lock;
	struct wakeup_source *ws;
	struct delayed_work eu_eco_work;

	struct sec_battery_info *battery;
};

enum {
	SB_FULL_CAP_EVENT_NONE = 0,
	SB_FULL_CAP_EVENT_HIGHSOC,
	SB_FULL_CAP_EVENT_SLEEP,
	SB_FULL_CAP_EVENT_OPTION,
};

#define MAX_CAP_EVENT_STR	16
#define DEF_ECO_RECHG_DIF	5
#define DEF_RECHG_SOC_DIF	2

static int conv_full_cap_event_value(const char *str)
{
	if (str == NULL)
		return SB_FULL_CAP_EVENT_NONE;

	if (!strncmp(str, "HIGHSOC", MAX_CAP_EVENT_STR))
		return SB_FULL_CAP_EVENT_HIGHSOC;

	if (!strncmp(str, "SLEEP", MAX_CAP_EVENT_STR))
		return SB_FULL_CAP_EVENT_SLEEP;

	if (!strncmp(str, "OPTION", MAX_CAP_EVENT_STR))
		return SB_FULL_CAP_EVENT_OPTION;

	return SB_FULL_CAP_EVENT_NONE;
}

static const char *conv_full_cap_str(unsigned int val)
{
	switch (val) {
	case SB_FULL_CAP_EVENT_HIGHSOC:
		return "HIGHSOC";
	case SB_FULL_CAP_EVENT_SLEEP:
		return "SLEEP";
	case SB_FULL_CAP_EVENT_OPTION:
		return "OPTION";
	}

	return "NONE";
}

int get_full_capacity(struct sb_full_soc *fs)
{
	if (fs == NULL)
		return 100;

	return fs->full_capacity;
}
EXPORT_SYMBOL(get_full_capacity);

static void set_full_capacity(struct sb_full_soc *fs, int new_cap)
{
	if (fs == NULL)
		return;

	fs->full_capacity = new_cap;
}

bool is_full_capacity(struct sb_full_soc *fs)
{
	if (fs == NULL)
		return false;

	return ((fs->full_capacity > 0) && (fs->full_capacity < 100));
}
EXPORT_SYMBOL(is_full_capacity);

static void set_full_cap_event(struct sb_full_soc *fs, unsigned int new_cap_event)
{
	if (fs == NULL)
		return;

	fs->full_cap_event = new_cap_event;
}

static unsigned int get_full_cap_event(struct sb_full_soc *fs)
{
	if (fs == NULL)
		return 0;

	return fs->full_cap_event;
}

static bool is_full_cap_event_highsoc(struct sb_full_soc *fs)
{
	if (fs == NULL)
		return false;

	return (fs->full_cap_event == SB_FULL_CAP_EVENT_HIGHSOC);
}

static void set_eu_eco_rechg(struct sb_full_soc *fs, bool enable)
{
	if (fs == NULL)
		return;

	fs->eu_eco_rechg_state = enable;
}

bool is_eu_eco_rechg(struct sb_full_soc *fs)
{
	if (fs == NULL)
		return false;

	return fs->is_eu_eco_rechg;
}
EXPORT_SYMBOL(is_eu_eco_rechg);

bool check_eu_eco_full_status(struct sec_battery_info *battery)
{
	struct sb_full_soc *fs = battery->fs;

	if (fs == NULL)
		return false;

	if ((!fs->is_eu_eco_rechg) ||
		(battery->status != POWER_SUPPLY_STATUS_FULL)) {
		fs->eu_eco_chg_done = false;
		return false;
	}

	if (battery->capacity >= 100) {
		fs->eu_eco_chg_done = true;
	} else if (fs->eu_eco_chg_done) {
		fs->eu_eco_chg_done = false;

		if ((battery->is_recharging) ||
			(battery->charging_mode == SEC_BATTERY_CHARGING_2ND)) {
			pr_info("%s: fixed the 2nd fullcharged!!!(%d, %d)\n",
				__func__, battery->is_recharging, battery->charging_mode);
			return true;
		}
	}

	return false;
}
EXPORT_SYMBOL(check_eu_eco_full_status);

static void enable_eu_eco_rechg(struct sec_battery_info *battery)
{
	battery->pdata->recharge_condition_type = SEC_BATTERY_RECHARGE_CONDITION_SOC;
	battery->pdata->recharge_condition_soc = 100 - DEF_ECO_RECHG_DIF;
}

static void disable_eu_eco_rechg(struct sec_battery_info *battery)
{
	struct sb_full_soc *fs = battery->fs;

	battery->pdata->recharge_condition_type = fs->old_recharge_condition_type;
	battery->pdata->recharge_condition_soc = fs->old_recharge_condition_soc;
}

bool check_eu_eco_rechg_ui_condition(struct sec_battery_info *battery)
{
	if (battery->pdata->recharge_condition_type & SEC_BATTERY_RECHARGE_CONDITION_SOC)
		return (battery->capacity <= battery->pdata->recharge_condition_soc);

	return false;
}
EXPORT_SYMBOL(check_eu_eco_rechg_ui_condition);

static void eu_eco_work(struct work_struct *work)
{
	struct sb_full_soc *fs = container_of(work, struct sb_full_soc, eu_eco_work.work);
	struct sec_battery_info *battery = fs->battery;
	union power_supply_propval value = {0, };

	pr_info("%s: start (%d, %d, %d, %d)\n",
		__func__,
		fs->eu_eco_rechg_state, fs->is_eu_eco_rechg,
		battery->status, battery->capacity);

	mutex_lock(&fs->lock);
	if (fs->is_eu_eco_rechg == fs->eu_eco_rechg_state)
		goto end_work;

	if ((battery->status != POWER_SUPPLY_STATUS_FULL) ||
		(battery->capacity >= 100))
		goto update_state;

	if (fs->eu_eco_rechg_state) {
		pr_info("%s : Update fg scale to %d%%\n", __func__, battery->capacity);
		value.intval = 99;
		psy_do_property(battery->pdata->fuelgauge_name, set,
			POWER_SUPPLY_PROP_CHARGE_FULL, value);
	} else {
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_CHARGING);
		battery->is_recharging = false;
		battery->charging_mode = SEC_BATTERY_CHARGING_1ST;
		if (battery->pdata->change_FV_after_full)
			sec_vote(battery->fv_vote, VOTER_FULL_CHARGE, false, battery->pdata->chg_float_voltage);
		sec_vote(battery->chgen_vote, VOTER_CABLE, true, SEC_BAT_CHG_MODE_CHARGING);
		sec_vote(battery->topoff_vote, VOTER_FULL_CHARGE, false, 0);
		sec_vote(battery->chgen_vote, VOTER_FULL_CHARGE, false, 0);
		pr_info("%s: battery status full -> charging, Cap(%d)\n",
			__func__, battery->capacity);
		value.intval = POWER_SUPPLY_STATUS_CHARGING;
		psy_do_property(battery->pdata->wireless_charger_name, set,
			POWER_SUPPLY_PROP_STATUS, value);
	}

	/* start polling work */
	__pm_stay_awake(battery->monitor_ws);
	queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);

update_state:
	pr_info("%s: update eu eco rechg(%d --> %d)\n",
		__func__, fs->is_eu_eco_rechg, fs->eu_eco_rechg_state);
	store_battery_log("EUECO:%d->%d,%s,%d%%",
		fs->is_eu_eco_rechg, fs->eu_eco_rechg_state,
		sb_get_bst_str(battery->status), battery->capacity);
	fs->is_eu_eco_rechg = fs->eu_eco_rechg_state;
end_work:
	mutex_unlock(&fs->lock);
	__pm_relax(fs->ws);
}

static ssize_t
sb_full_soc_show_attrs(struct device *, struct device_attribute *, char *);
static ssize_t
sb_full_soc_store_attrs(struct device *, struct device_attribute *, const char *, size_t);

#define SB_FULL_SOC_ATTR(_name)	\
{	\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = sb_full_soc_show_attrs,	\
	.store = sb_full_soc_store_attrs,	\
}

enum sec_bat_attrs {
	BATT_FULL_CAPACITY = 0,
	BATT_SOC_RECHG,
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	BATT_FULL_CAP_EVENT,
	BATT_FULL_SOC_TEST,
#endif
};

static struct device_attribute sb_full_soc_attrs[] = {
	SB_FULL_SOC_ATTR(batt_full_capacity),
	SB_FULL_SOC_ATTR(batt_soc_rechg),
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	SB_FULL_SOC_ATTR(batt_full_cap_event),
	SB_FULL_SOC_ATTR(batt_full_soc_test),
#endif
};


static ssize_t sb_full_soc_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sb_full_soc_attrs;
	int i = 0;

	switch (offset) {
	case BATT_FULL_CAPACITY:
		i += scnprintf(buf, PAGE_SIZE, "%d\n", get_full_capacity(battery->fs));
		break;
	case BATT_SOC_RECHG:
		i += scnprintf(buf, PAGE_SIZE, "%d\n", is_eu_eco_rechg(battery->fs));
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_FULL_CAP_EVENT:
		i += scnprintf(buf, PAGE_SIZE, "%s\n",
			conv_full_cap_str(get_full_cap_event(battery->fs)));
		break;
	case BATT_FULL_SOC_TEST:
		i += scnprintf(buf, PAGE_SIZE, "%d, 0x%x, %d\n",
			is_eu_eco_rechg(battery->fs),
			battery->pdata->recharge_condition_type,
			battery->pdata->recharge_condition_soc);
		break;
#endif
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t sb_full_soc_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_battery_info *battery = power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - sb_full_soc_attrs;

	switch (offset) {
	case BATT_FULL_CAPACITY:
	{
		unsigned int full_cap_event = SB_FULL_CAP_EVENT_NONE;
		int x = 0, n = 0;
		bool is_changed = false;

		if (sscanf(buf, "%10d%n", &x, &n) <= 0) {
			pr_info("%s: invalid arguments\n", __func__);
			return -EINVAL;
		} else if (x < 0 || x > 100) {
			pr_info("%s: out of range(%d)\n", __func__, x);
			break;
		}

		if (n > 0) {
			char cap_event[MAX_CAP_EVENT_STR] = { 0, };

			if ((count - n) > MAX_CAP_EVENT_STR)
				pr_info("%s: out of range\n", __func__);
			else if (sscanf(buf + n, "%s\n", cap_event) > 0)
				full_cap_event = conv_full_cap_event_value(cap_event);
		}

		if ((get_full_capacity(battery->fs) != x) ||
			(get_full_cap_event(battery->fs) != full_cap_event)) {
			is_changed = true;

			store_battery_log("FCAP:%d%%->%d%%,%s->%s",
				get_full_capacity(battery->fs), x,
				conv_full_cap_str(get_full_cap_event(battery->fs)), conv_full_cap_str(full_cap_event));

			set_full_capacity(battery->fs, x);
			set_full_cap_event(battery->fs, full_cap_event);

			/* recov full cap */
			sec_bat_recov_full_capacity(battery);

			__pm_stay_awake(battery->monitor_ws);
			queue_delayed_work(battery->monitor_wqueue, &battery->monitor_work, 0);
		}
		pr_info("%s: %s full cap(%d, %s)\n",
			__func__, (is_changed ? "set" : "same"), x, conv_full_cap_str(full_cap_event));
	}
		break;
	case BATT_SOC_RECHG:
	{
		int x = 0;

		if (sscanf(buf, "%10d\n", &x) != 1) {
			pr_info("%s: invalid arguments\n", __func__);
			return -EINVAL;
		}

		mutex_lock(&battery->fs->lock);
		set_eu_eco_rechg(battery->fs, !!x);
		if (x)
			enable_eu_eco_rechg(battery);
		else
			disable_eu_eco_rechg(battery);

		/* start eu eco work */
		__pm_stay_awake(battery->fs->ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->fs->eu_eco_work, 0);
		mutex_unlock(&battery->fs->lock);

		pr_info("%s: set eu eco rechg(%d)\n",
			__func__, is_eu_eco_rechg(battery->fs));
	}
		break;
#if defined(CONFIG_ENG_BATTERY_CONCEPT)
	case BATT_FULL_CAP_EVENT:
		break;
	case BATT_FULL_SOC_TEST:
	{
		int x;

		if (sscanf(buf, "%10d\n", &x) != 1) {
			pr_info("%s: invalid arguments\n", __func__);
			return -EINVAL;
		}

		battery->pdata->recharge_condition_soc = x;
	}
		break;
#endif
	default:
		return -EINVAL;
	}

	return count;
}

static int sb_full_soc_create_attrs(struct device *dev)
{
	unsigned long i = 0;
	int rc = 0;

	for (i = 0; i < ARRAY_SIZE(sb_full_soc_attrs); i++) {
		rc = device_create_file(dev, &sb_full_soc_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	while (i--)
		device_remove_file(dev, &sb_full_soc_attrs[i]);
create_attrs_succeed:
	return rc;
}

static void sb_full_soc_remove_attrs(struct device *dev)
{
	int i = 0;

	for (; i < ARRAY_SIZE(sb_full_soc_attrs); i++)
		device_remove_file(dev, &sb_full_soc_attrs[i]);
}

void sec_bat_recov_full_capacity(struct sec_battery_info *battery)
{
	sec_bat_set_misc_event(battery, 0, BATT_MISC_EVENT_FULL_CAPACITY);
	if (battery->status == POWER_SUPPLY_STATUS_NOT_CHARGING
		&& battery->health == POWER_SUPPLY_HEALTH_GOOD) {
#if defined(CONFIG_ENABLE_FULL_BY_SOC)
		if (battery->capacity >= 100)
			sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_FULL);
		else
#endif
			sec_bat_set_charging_status(battery,
				POWER_SUPPLY_STATUS_CHARGING);
	}

	if (!is_full_cap_event_highsoc(battery->fs))
		sec_vote(battery->chgen_vote, VOTER_FULL_CAPACITY, false, 0);
}
EXPORT_SYMBOL(sec_bat_recov_full_capacity);

void sec_bat_check_full_capacity(struct sec_battery_info *battery)
{
	int now_full_capacity = get_full_capacity(battery->fs);
	int rechg_capacity = now_full_capacity - DEF_RECHG_SOC_DIF;

	if (is_full_cap_event_highsoc(battery->fs) &&
		(battery->capacity <= now_full_capacity)) {
		set_full_cap_event(battery->fs, SB_FULL_CAP_EVENT_NONE);
		sec_vote(battery->chgen_vote, VOTER_FULL_CAPACITY,
			(battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY), SEC_BAT_CHG_MODE_CHARGING_OFF);
	}

	if (!is_full_capacity(battery->fs) ||
		battery->status == POWER_SUPPLY_STATUS_DISCHARGING) {
		if (battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY) {
			pr_info("%s: full_capacity(%d) status(%d)\n",
				__func__, now_full_capacity, battery->status);
			sec_bat_recov_full_capacity(battery);
		}
		return;
	}

	if (battery->misc_event & BATT_MISC_EVENT_FULL_CAPACITY) {
		if (battery->capacity <= rechg_capacity ||
			battery->status == POWER_SUPPLY_STATUS_CHARGING) {
			pr_info("%s : start re-charging(%d, %d) status(%d)\n",
				__func__, battery->capacity, rechg_capacity, battery->status);
			set_full_cap_event(battery->fs, SB_FULL_CAP_EVENT_NONE);
			sec_bat_recov_full_capacity(battery);
		}
	} else if (battery->capacity >= now_full_capacity) {
		union power_supply_propval value = {0, };

		pr_info("%s : stop charging(%d, %d, %s)\n", __func__,
			battery->capacity, now_full_capacity,
			conv_full_cap_str(get_full_cap_event(battery->fs)));
		sec_bat_set_misc_event(battery, BATT_MISC_EVENT_FULL_CAPACITY,
			BATT_MISC_EVENT_FULL_CAPACITY);
		sec_bat_set_charging_status(battery, POWER_SUPPLY_STATUS_NOT_CHARGING);
		sec_vote(battery->chgen_vote, VOTER_FULL_CAPACITY, true,
			(is_full_cap_event_highsoc(battery->fs) ?
				SEC_BAT_CHG_MODE_BUCK_OFF : SEC_BAT_CHG_MODE_CHARGING_OFF));

		if (is_wireless_all_type(battery->cable_type)) {
			value.intval = POWER_SUPPLY_STATUS_FULL;
			psy_do_property(battery->pdata->wireless_charger_name, set,
				POWER_SUPPLY_PROP_STATUS, value);
		}
	}
}
EXPORT_SYMBOL(sec_bat_check_full_capacity);

int sb_full_soc_init(struct sec_battery_info *battery)
{
	struct sb_full_soc *fs;
	int ret = 0;

	fs = kzalloc(sizeof(struct sb_full_soc), GFP_KERNEL);
	if (!fs)
		return -ENOMEM;

	ret = sb_full_soc_create_attrs(&battery->psy_bat->dev);
	if (ret) {
		pr_err("%s: failed to create attrs(%d)\n", __func__, ret);
		goto err_attrs;
	}

	fs->ws = wakeup_source_register(NULL, "full-soc");
	if (!fs->ws) {
		pr_err("%s: failed to register wakeup\n", __func__);
		goto err_ws;
	}

	mutex_init(&fs->lock);
	INIT_DELAYED_WORK(&fs->eu_eco_work, eu_eco_work);

	fs->battery = battery;

	fs->full_capacity = 0;
	fs->full_cap_event = SB_FULL_CAP_EVENT_NONE;

	fs->is_eu_eco_rechg = false;
	fs->eu_eco_rechg_state = false;
	fs->old_recharge_condition_type = battery->pdata->recharge_condition_type;
	fs->old_recharge_condition_soc = battery->pdata->recharge_condition_soc;

	battery->fs = fs;
	return 0;

err_ws:
	sb_full_soc_remove_attrs(&battery->psy_bat->dev);
err_attrs:
	kfree(fs);
	return ret;
}
EXPORT_SYMBOL(sb_full_soc_init);
