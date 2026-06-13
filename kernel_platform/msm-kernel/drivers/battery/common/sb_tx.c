/*
 *  sb_tx.c
 *  Samsung Mobile Wireless TX Driver
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mutex.h>

#include <linux/battery/sb_sysfs.h>
#include <linux/battery/sb_notify.h>

#include "sec_battery.h"
#include "sec_charging_common.h"
#include "sb_tx.h"

#define tx_log(str, ...) pr_info("[SB-TX]:%s: "str, __func__, ##__VA_ARGS__)

#define AOV_VOUT_STEP	500
#define AOV_VOUT_MAX	7500

enum sb_tx_aov_state {
	AOV_STATE_NONE = 0,
	AOV_STATE_PRESET,
	AOV_STATE_MONITOR,
	AOV_STATE_PHM,
	AOV_STATE_ERR,
};

enum sb_tx_chg_state {
	AOV_WITH_NONE = 0,
	AOV_WITH_NOCHG,
	AOV_WITH_CHG,
};

static const char *get_aov_state_str(int state)
{
	switch (state) {
	case AOV_STATE_NONE:
		return "None";
	case AOV_STATE_PRESET:
		return "Preset";
	case AOV_STATE_MONITOR:
		return "Monitor";
	case AOV_STATE_PHM:
		return "PHM";
	case AOV_STATE_ERR:
		return "Error";
	}

	return "Unknown";
}

struct sb_tx_aov {
	bool enable;
	int state;
	int tx_chg_state;

	/* dt data */
	unsigned int start_vout;
	unsigned int low_freq;
	unsigned int high_freq;
	unsigned int delay;
	unsigned int preset_delay;
	unsigned int phm_icl;
	unsigned int phm_icl_full;
	unsigned int vout_min;
};

struct sb_tx {
	/* temporary attributes */
	struct sec_battery_info *battery;

	struct notifier_block nb;

	struct wakeup_source *ws;

	struct workqueue_struct *wq;
	struct delayed_work tx_err_work;

	bool enable;
	unsigned int event;

	struct mutex event_lock;

	/* option */
	struct sb_tx_aov aov;

	char *wrl_name;
	char *fg_name;
};
struct sb_tx *gtx;
static DEFINE_MUTEX(tx_lock);

static struct sb_tx *get_sb_tx(void)
{
	struct sb_tx *tx = NULL;

	mutex_lock(&tx_lock);
	tx = gtx;
	mutex_unlock(&tx_lock);

	return tx;
}

static bool check_full_state(struct sb_tx *tx)
{
	union power_supply_propval value = {0, };

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
	psy_do_property(tx->fg_name, get,
		POWER_SUPPLY_PROP_CAPACITY, value);

	return (value.intval >= 970);
}

int sb_tx_init_aov(void)
{
	struct sb_tx *tx = get_sb_tx();
	struct sb_tx_aov *aov;

	if (!tx)
		return -ENODEV;

	aov = &tx->aov;

	mutex_lock(&tx_lock);

	aov->state = AOV_STATE_NONE;
	aov->tx_chg_state = AOV_WITH_NONE;
	sec_vote(tx->battery->input_vote, VOTER_WC_TX, false, 0);

	mutex_unlock(&tx_lock);

	return 0;
}

bool sb_tx_is_aov_enabled(int cable_type)
{
	struct sb_tx *tx = get_sb_tx();
	struct sb_tx_aov *aov = &tx->aov;

	if (!tx)
		return false;

	if (!tx->aov.enable)
		return false;

	if (tx->aov.state == AOV_STATE_ERR)
		return false;

	if (is_pd_apdo_wire_type(cable_type)) {
		if (tx->aov.state == AOV_STATE_NONE) {
			unsigned int min_iv = aov->vout_min, max_iv = AOV_VOUT_MAX;

			if (sec_pd_get_pdo_power(NULL, &min_iv, &max_iv, NULL) <= 0)
				return false;
		}
		aov->tx_chg_state = AOV_WITH_CHG;
	} else if (!is_nocharge_type(cable_type)) {
		if (tx->aov.state != AOV_STATE_NONE)
			sb_tx_init_aov();
		return false;
	} else {
		aov->tx_chg_state = AOV_WITH_NOCHG;
	}

	return true;
}

int sb_tx_monitor_aov(int vout, bool phm)
{
	struct sb_tx *tx = get_sb_tx();
	struct sec_battery_info *battery;
	struct sb_tx_aov *aov;
	int prev_aov_state;
	static int prev_tx_chg_state;

	if (!tx)
		return -ENODEV;

	aov = &tx->aov;
	battery = tx->battery;
	mutex_lock(&tx_lock);
	prev_aov_state = aov->state;

	switch (aov->state) {
	case AOV_STATE_NONE:
		prev_tx_chg_state = AOV_WITH_NONE;
		aov->state = AOV_STATE_PRESET;
		fallthrough;
	case AOV_STATE_PRESET:
		if (vout < aov->start_vout) {
			if (prev_aov_state == AOV_STATE_NONE) {
				sec_bat_run_wpc_tx_work(battery, (aov->preset_delay + 1000));
			} else {
				vout = vout + AOV_VOUT_STEP;
				sec_vote(battery->iv_vote, VOTER_WC_TX, true, vout);
				sec_bat_wireless_vout_cntl(tx->battery, vout);
				sec_bat_run_wpc_tx_work(battery,
					((vout == aov->start_vout) ? (aov->preset_delay * 2) : 500));
			}
			break;
		}

		aov->state = AOV_STATE_MONITOR;
		fallthrough;
	case AOV_STATE_MONITOR:
		if (phm) {
			int phm_icl = (check_full_state(tx)) ?
				aov->phm_icl_full : aov->phm_icl;

			sec_vote(battery->iv_vote, VOTER_WC_TX, true, SEC_INPUT_VOLTAGE_5V);
			sec_vote(battery->input_vote, VOTER_WC_TX, true, phm_icl);
			sec_bat_wireless_vout_cntl(battery, WC_TX_VOUT_5000MV);
			sec_bat_wireless_iout_cntl(battery, battery->pdata->tx_uno_iout, 1000);

			aov->state = AOV_STATE_PHM;
		} else {
			union power_supply_propval freq = {0, };
			int prev_vout = vout;

			psy_do_property(tx->wrl_name, get, POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ, freq);
			if ((freq.intval <= aov->low_freq) && (vout < AOV_VOUT_MAX))
				vout = vout + AOV_VOUT_STEP;
			else if ((freq.intval >= aov->high_freq) && (vout > aov->vout_min))
				vout = vout - AOV_VOUT_STEP;

			if ((prev_vout != vout) ||
				((prev_tx_chg_state == AOV_WITH_NOCHG) && (aov->tx_chg_state == AOV_WITH_CHG))) {
				sec_vote(battery->iv_vote, VOTER_WC_TX, true, vout);
				sec_bat_wireless_vout_cntl(battery, vout);
				sec_bat_run_wpc_tx_work(battery, aov->delay);
			} else {
				sec_vote_refresh(battery->iv_vote);
			}
		}
		break;
	case AOV_STATE_PHM:
		if (!phm) {
			vout = aov->start_vout + AOV_VOUT_STEP;
			sec_vote(battery->iv_vote, VOTER_WC_TX, true, vout);
			if (battery->pdata->icl_by_tx_gear)
				sec_vote(battery->input_vote, VOTER_WC_TX, true, battery->pdata->icl_by_tx_gear);
			else
				sec_vote(battery->input_vote, VOTER_WC_TX, false, 0);
			sec_bat_wireless_vout_cntl(battery, vout);
			sec_bat_run_wpc_tx_work(battery, aov->delay);

			aov->state = AOV_STATE_MONITOR;
		} else {
			int phm_icl = (check_full_state(tx)) ?
				aov->phm_icl_full : aov->phm_icl;

			sec_vote(battery->input_vote, VOTER_WC_TX, true, phm_icl);
		}
		break;
	default:
		break;
	}
	prev_tx_chg_state = aov->tx_chg_state;
	sec_vote(battery->chgen_vote, VOTER_WC_TX, false, 0);
	tx_log("aov state = %s, tx_chg_state = %d, vout = %d\n",
		get_aov_state_str(aov->state), aov->tx_chg_state, vout);

	mutex_unlock(&tx_lock);

	return aov->state;
}

static bool check_tx_err_state(struct sb_tx *tx)
{
	union power_supply_propval value = { 0, };
	int vout, iout, freq;

	psy_do_property(tx->wrl_name, get, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_VIN, value);
	vout = value.intval;

	psy_do_property(tx->wrl_name, get, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_UNO_IIN, value);
	iout = value.intval;

	psy_do_property(tx->wrl_name, get, POWER_SUPPLY_EXT_PROP_WIRELESS_OP_FREQ, value);
	freq = value.intval;

	return (vout <= 0) && (iout <= 0) && (freq <= 0);
}

static void cb_tx_err_work(struct work_struct *work)
{
	struct sb_tx *tx = container_of(work,
		struct sb_tx, tx_err_work.work);

	tx_log("start!\n");

	if (check_tx_err_state(tx)) {
		union power_supply_propval value = { 0, };

		tx_log("set tx retry!!!\n");
		value.intval = BATT_TX_EVENT_WIRELESS_TX_ETC;
		psy_do_property("wireless", set, POWER_SUPPLY_EXT_PROP_WIRELESS_TX_ERR, value);
	}

	__pm_relax(tx->ws);
}

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#define TX_SYSFS_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = show_attrs,					\
	.store = store_attrs,					\
}

static struct device_attribute tx_attr[] = {
	TX_SYSFS_ATTR(tx_test),
};

enum tx_attrs {
	TX_TEST = 0,
};

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static int sb_noti_handler(struct notifier_block *nb, unsigned long action, void *data)
{
	return 0;
}

#ifdef CONFIG_OF
static int sb_tx_parse_aov_dt(struct device_node *np, struct sb_tx_aov *aov)
{
	if (of_property_read_bool(np, "disable"))
		return -1;

	sb_of_parse_u32(np, aov, start_vout, WC_TX_VOUT_5500MV);
	sb_of_parse_u32(np, aov, phm_icl, 800);
	sb_of_parse_u32(np, aov, phm_icl_full, 100);
	sb_of_parse_u32(np, aov, low_freq, 131);
	sb_of_parse_u32(np, aov, high_freq, 147);
	sb_of_parse_u32(np, aov, delay, 3000);
	sb_of_parse_u32(np, aov, preset_delay, 3000);
	sb_of_parse_u32(np, aov, vout_min, 5000);
	return 0;
}

static int sb_tx_parse_dt(struct sb_tx *tx)
{
	struct device_node *np, *child;
	int ret = 0;

	np = of_find_node_by_name(NULL, TX_MODULE_NAME);
	if (!np)
		return -ENODEV;

	for_each_child_of_node(np, child) {
		if (!strcmp(child->name, "aov")) {
			ret = sb_tx_parse_aov_dt(child, &tx->aov);

			tx->aov.enable = !(ret);
			tx_log("AOV = %s\n", tx->aov.enable ? "Enable" : "Disable");
		}
	}

	np = of_find_node_by_name(NULL, "battery");
	if (np) {
		ret = of_property_read_string(np,
			"battery,fuelgauge_name", (const char **)&tx->fg_name);
		if (ret)
			tx_log("failed to get fg name in battery dt (ret = %d)\n", ret);
	}

	return ret;
}
#else
static int sb_tx_parse_dt(struct sb_tx *tx)
{
	return 0;
}
#endif

int sb_tx_init(struct sec_battery_info *battery, char *wrl_name)
{
	struct sb_tx *tx;
	int ret = 0;

	if ((battery == NULL) || (wrl_name == NULL))
		return -EINVAL;

	/* temporary code */
	if (get_sb_tx() != NULL)
		return 0;

	mutex_lock(&tx_lock);

	tx = kzalloc(sizeof(struct sb_tx), GFP_KERNEL);
	if (!tx) {
		mutex_unlock(&tx_lock);
		return -ENOMEM;
	}

	ret = sb_tx_parse_dt(tx);
	if (ret)
		goto err_parse_dt;

	tx->wq = create_singlethread_workqueue(TX_MODULE_NAME);
	if (!tx->wq) {
		ret = -ENOMEM;
		goto err_parse_dt;
	}

	tx->ws = wakeup_source_register(battery->dev, TX_MODULE_NAME);

	INIT_DELAYED_WORK(&tx->tx_err_work, cb_tx_err_work);

	mutex_init(&tx->event_lock);

	tx->battery = battery;
	tx->wrl_name = wrl_name;
	tx->enable = false;

	ret = sb_sysfs_add_attrs(TX_MODULE_NAME, tx, tx_attr, ARRAY_SIZE(tx_attr));
	tx_log("sb_sysfs_add_attrs ret = %s\n", (ret) ? "fail" : "success");

	ret = sb_notify_register(&tx->nb, sb_noti_handler, TX_MODULE_NAME, SB_DEV_MODULE);
	tx_log("sb_notify_register ret = %s\n", (ret) ? "fail" : "success");

	gtx = tx;

	mutex_unlock(&tx_lock);
	return 0;

err_parse_dt:
	kfree(tx);

	mutex_unlock(&tx_lock);
	return ret;
}
EXPORT_SYMBOL(sb_tx_init);

int sb_tx_set_enable(bool tx_enable, int cable_type)
{
	struct sb_tx *tx = get_sb_tx();

	if (tx_enable) {
		if (check_tx_err_state(tx)) {
			tx_log("abnormal case - run tx err work!\n");

			__pm_stay_awake(tx->ws);
			queue_delayed_work(tx->wq, &tx->tx_err_work, msecs_to_jiffies(1000));
		}
	} else {
		cancel_delayed_work(&tx->tx_err_work);
		__pm_relax(tx->ws);
	}

	return 0;
}
EXPORT_SYMBOL(sb_tx_set_enable);

bool sb_tx_get_enable(void)
{
	struct sb_tx *tx = get_sb_tx();

	return (tx != NULL) ? tx->enable : false;
}
EXPORT_SYMBOL(sb_tx_get_enable);

int sb_tx_set_event(int value, int mask)
{
	return 0;
}
EXPORT_SYMBOL(sb_tx_set_event);

int sb_tx_psy_set_property(enum power_supply_property psp, const union power_supply_propval *value)
{
	return 0;
}
EXPORT_SYMBOL(sb_tx_psy_set_property);

int sb_tx_psy_get_property(enum power_supply_property psp, union power_supply_propval *value)
{
	return 0;
}
EXPORT_SYMBOL(sb_tx_psy_get_property);

int sb_tx_monitor(int cable_type, int capacity, int lcd_state)
{
	return 0;
}
EXPORT_SYMBOL(sb_tx_monitor);

