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
#include "sb_pass_through.h"

#define pt_log(str, ...) pr_info("[PASS-THROUGH]:%s: "str, __func__, ##__VA_ARGS__)

#define PT_MODULE_NAME	"pass-through"
#define IV_VOTE_NAME	"IV"
#define ICL_VOTE_NAME	"ICL"
#define FCC_VOTE_NAME	"FCC"
#define CHGEN_VOTE_NAME	"CHGEN"
#define DCFV_VOTE_NAME	"DCFV"

struct sb_pt {
	struct notifier_block nb;
	struct mutex mlock;

	struct wakeup_source *ws;

	struct workqueue_struct *wq;
	struct delayed_work start_work;
	struct delayed_work adjust_work;
	struct delayed_work step_work;

	/* state flags */
	int user_mode;
	int chg_src;
	int step;
	int ref_cap;

	int adj_state;
	unsigned int adj_cnt;
	unsigned int adj_op_cnt;

	/* battery status */
	int cable_type;
	int batt_status;

	int dc_status;

	/* dt data */
	bool is_enabled;

	unsigned int start_delay;
	unsigned int init_delay;
	unsigned int adj_delay;
	unsigned int adj_max_cnt;
	unsigned int min_cap;
	unsigned int fixed_sc_cap;
	unsigned int max_icl;
	unsigned int vfloat;

	char *sc_name;
	char *dc_name;
	char *fg_name;
};

enum pt_step {
	PT_STEP_NONE = 0,
	PT_STEP_INIT,
	PT_STEP_PRESET,
	PT_STEP_ADJUST,
	PT_STEP_MONITOR,
	PT_STEP_RESET,
};

static const char *get_step_str(int step)
{
	switch (step) {
	case PT_STEP_NONE:
		return "None";
	case PT_STEP_INIT:
		return "Init";
	case PT_STEP_PRESET:
		return "Preset";
	case PT_STEP_ADJUST:
		return "Adjust";
	case PT_STEP_MONITOR:
		return "Monitor";
	case PT_STEP_RESET:
		return "Reset";
	}

	return "Unknown";
}

static void set_misc_event(bool state)
{
	struct sbn_bit_event misc;

	misc.value = (state) ? BATT_MISC_EVENT_PASS_THROUGH : 0;
	misc.mask = BATT_MISC_EVENT_PASS_THROUGH;

	sb_notify_call(SB_NOTIFY_EVENT_MISC, cast_to_sb_pdata(&misc));
}

static int set_dc_ta_volt(struct sb_pt *pt, int value)
{
	union power_supply_propval val = { value, };

	return psy_do_property(pt->dc_name, set,
		POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE_TA_VOL, val);
}

/* don't call the mutex lock in static functions */
static bool check_state(struct sb_pt *pt)
{
	if (!pt)
		return false;

	if (!pt->is_enabled)
		return false;

	if (pt->user_mode == PTM_NONE)
		return false;

	if (!is_pd_apdo_wire_type(pt->cable_type))
		return false;

	if (pt->batt_status != POWER_SUPPLY_STATUS_CHARGING)
		return false;

	if (pt->step == PT_STEP_NONE) {
		union power_supply_propval value = { 0, };

		value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
		psy_do_property(pt->fg_name, get,
			POWER_SUPPLY_PROP_CAPACITY, value);
		if (pt->min_cap >= value.intval)
			return false;
	}

	return true;
}

static bool check_preset_state(int dc_status)
{
	return (dc_status == SEC_DIRECT_CHG_MODE_DIRECT_ON) ||
		(dc_status == SEC_DIRECT_CHG_MODE_DIRECT_DONE) ||
		(dc_status == SEC_DIRECT_CHG_MODE_DIRECT_BYPASS);
}

#define CAP_HIGH (1)
#define CAP_NORMAL (0)
#define CAP_LOW (-1)
static int check_cap(struct sb_pt *pt)
{
	union power_supply_propval value = {0, };
	int ncap1, ncap2, rcap1, rcap2;
	int delta_cap = 0;

	value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
	psy_do_property(pt->fg_name, get,
		POWER_SUPPLY_PROP_CAPACITY, value);

	ncap1 = (value.intval / 10);
	ncap2 = (value.intval % 10);

	rcap1 = (pt->ref_cap / 10);
	rcap2 = (pt->ref_cap % 10);

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(pt->fg_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

	if (ncap1 == rcap1)
		delta_cap = (((ncap2 >= 6) && (value.intval > 0)) ? CAP_HIGH :
			(((ncap2 <= 4) && (value.intval < 0)) ? CAP_LOW : CAP_NORMAL));
	else if (ncap1 > rcap1)
		delta_cap = (value.intval > 0) ? CAP_HIGH : CAP_NORMAL;
	else
		delta_cap = (value.intval < 0) ? CAP_LOW : CAP_NORMAL;

	pt_log("Now Cap(%03d.%d%%), Ref Cap(%03d.%d%%), Current(%04dmA), delta(%d)\n",
			ncap1, ncap2, rcap1, rcap2, value.intval, delta_cap);
	return delta_cap;
}

static void clear_state(struct sb_pt *pt, int init_step)
{
	if (pt->step == PT_STEP_NONE)
		return;

	pt_log("latest step = %d, init_step = %d\n", pt->step, init_step);

	sec_votef(IV_VOTE_NAME, VOTER_PASS_THROUGH, false, 0);
	sec_votef(ICL_VOTE_NAME, VOTER_PASS_THROUGH, false, 0);
	sec_votef(FCC_VOTE_NAME, VOTER_PASS_THROUGH, false, 0);
	sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, false, 0);

	pt->chg_src = SEC_CHARGING_SOURCE_SWITCHING;
	pt->adj_state = CAP_NORMAL;
	pt->adj_cnt = 0;
	pt->adj_op_cnt = 0;

	if (init_step == PT_STEP_NONE) {
		int prev_dcfv = get_sec_vote_resultf(DCFV_VOTE_NAME);

		pt->ref_cap = 0;

		/* set charging off before re-starting dc */
		sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, true, SEC_BAT_CHG_MODE_CHARGING_OFF);

		sec_votef(DCFV_VOTE_NAME, VOTER_PASS_THROUGH, false, 0);
		if (get_sec_vote_resultf(DCFV_VOTE_NAME) == prev_dcfv)
			sec_vote_refreshf(DCFV_VOTE_NAME);

		sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, false, 0);

		/* clear event */
		set_misc_event(false);
		sec_pd_detach_with_cc(0);
	}
	pt->step = init_step;
}

static void cb_start_work(struct work_struct *work)
{
	struct sb_pt *pt = container_of(work,
		struct sb_pt, start_work.work);

	mutex_lock(&pt->mlock);

	pt_log("now step = %s\n", get_step_str(pt->step));

	if (!check_state(pt))
		goto end_work;

	if (pt->step == PT_STEP_NONE)
		pt->step = PT_STEP_INIT;

	if (pt->step != PT_STEP_INIT)
		goto end_work;

	__pm_wakeup_event(pt->ws, msecs_to_jiffies(1000));
	queue_delayed_work(pt->wq, &pt->step_work, 0);

end_work:
	mutex_unlock(&pt->mlock);
}

#define PT_ADJ_OP_MAX_CNT	100
#define PT_ADJ_CURR			500
static void cb_adjust_work(struct work_struct *work)
{
	struct sb_pt *pt = container_of(work, struct sb_pt, adjust_work.work);
	union power_supply_propval value = {0, };
	int now_state = CAP_NORMAL;

	mutex_lock(&pt->mlock);

	if (!check_state(pt))
		goto end_work;

	if (pt->step != PT_STEP_ADJUST)
		goto end_work;

	if (pt->chg_src == SEC_CHARGING_SOURCE_SWITCHING) {
		pt->step = PT_STEP_MONITOR;
		goto end_work;
	}

	if (pt->adj_op_cnt++ >= PT_ADJ_OP_MAX_CNT) {
		pt_log("over working(%d) !!\n", pt->adj_op_cnt);
		pt->step = PT_STEP_MONITOR;
		goto end_work;
	}

	now_state = check_cap(pt);
	if (now_state != pt->adj_state) {
		pt->adj_cnt = 0;
		pt->adj_state = now_state;
		goto re_work;
	}

	value.intval = SEC_BATTERY_CURRENT_MA;
	psy_do_property(pt->fg_name, get,
		POWER_SUPPLY_PROP_CURRENT_NOW, value);

	switch (pt->adj_state) {
	case CAP_LOW:
		if (value.intval < (-PT_ADJ_CURR)) {
			pt->adj_cnt = 0;
			set_dc_ta_volt(pt, CAP_LOW);
		} else if (value.intval > (PT_ADJ_CURR)) {
			pt->adj_cnt = 0;
		} else {
			pt->adj_cnt++;
		}
		break;
	case CAP_HIGH:
		if (value.intval > (PT_ADJ_CURR)) {
			pt->adj_cnt = 0;
			set_dc_ta_volt(pt, CAP_HIGH);
		} else if (value.intval < (-PT_ADJ_CURR)) {
			pt->adj_cnt = 0;
		} else {
			pt->adj_cnt++;
		}
		break;
	case CAP_NORMAL:
		if (value.intval < (-PT_ADJ_CURR)) {
			pt->adj_cnt = 0;
			set_dc_ta_volt(pt, CAP_LOW);
		} else if (value.intval > (PT_ADJ_CURR)) {
			pt->adj_cnt = 0;
			set_dc_ta_volt(pt, CAP_HIGH);
		} else {
			pt->adj_cnt++;
		}
		break;
	default:
		break;
	}
	pt_log("ADJ STATE(%d, %d), CURR(%d), CNT(%d)\n",
		pt->adj_state, now_state, value.intval, pt->adj_cnt);

	if (pt->adj_cnt >= pt->adj_max_cnt) {
		pt->step = PT_STEP_MONITOR;
		goto end_work;
	}

re_work:
	mutex_unlock(&pt->mlock);

	queue_delayed_work(pt->wq, &pt->adjust_work, msecs_to_jiffies(pt->adj_delay));
	return;

end_work:
	pt->adj_state = CAP_NORMAL;
	pt->adj_cnt = 0;
	pt->adj_op_cnt = 0;

	mutex_unlock(&pt->mlock);
	__pm_relax(pt->ws);
}

static void cb_step_work(struct work_struct *work)
{
	struct sb_pt *pt = container_of(work, struct sb_pt, step_work.work);

	sb_pt_monitor(pt, pt->chg_src);
}

static void push_start_work(struct sb_pt *pt, unsigned int delay)
{
	unsigned int work_state;

	if (!check_state(pt))
		return;

	work_state = work_busy(&pt->start_work.work);
	pt_log("work_state = 0x%x, delay = %d\n", work_state, delay);
	if (!(work_state & (WORK_BUSY_PENDING | WORK_BUSY_RUNNING))) {
		__pm_wakeup_event(pt->ws, msecs_to_jiffies(delay + 1000));
		queue_delayed_work(pt->wq, &pt->start_work, msecs_to_jiffies(delay));
	}
}

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf);
static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#define PT_SYSFS_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = show_attrs,					\
	.store = store_attrs,					\
}

static struct device_attribute pt_attr[] = {
	PT_SYSFS_ATTR(pass_through),
};

enum sb_pt_attrs {
	PASS_THROUGH = 0,
};

static ssize_t show_attrs(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sb_pt *pt = sb_sysfs_get_pdata(PT_MODULE_NAME);
	const ptrdiff_t offset = attr - pt_attr;
	ssize_t count = 0;

	switch (offset) {
	case PASS_THROUGH:
		count += scnprintf(buf + count, PAGE_SIZE - count, "%d\n", pt->user_mode);
		break;
	default:
		break;
	}

	return count;
}

static ssize_t store_attrs(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sb_pt *pt = sb_sysfs_get_pdata(PT_MODULE_NAME);
	const ptrdiff_t offset = attr - pt_attr;

	switch (offset) {
	case PASS_THROUGH:
	{
		int x = 0;

		if (sscanf(buf, "%10d\n", &x) == 1) {
			mutex_lock(&pt->mlock);

			pt_log("user_mode = %d <-> %d, %s\n",
				x, pt->user_mode, ((x != PTM_NONE) ? "enabled" : "disabled"));

			x = (x) ? PTM_2TO1 : PTM_NONE;
			if (pt->user_mode != x) {
				pt->user_mode = x;

				if (pt->step != PT_STEP_NONE)
					clear_state(pt, PT_STEP_NONE);

				if (pt->user_mode)
					push_start_work(pt, pt->start_delay);
			}

			mutex_unlock(&pt->mlock);
		}
	}
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

static int parse_dt(struct sb_pt *pt, struct device *parent)
{
#if defined(CONFIG_OF)
	struct device_node *np;
	int ret = 0;

	if (!parent)
		return -EINVAL;

	np = of_find_node_by_name(NULL, PT_MODULE_NAME);
	if (!np) {
		pt_log("failed to find root node\n");
		return -ENODEV;
	}

	pt->is_enabled = true;

	sb_of_parse_u32(np, pt, start_delay, 5000);
	sb_of_parse_u32(np, pt, init_delay, 5000);
	sb_of_parse_u32(np, pt, adj_delay, 500);
	sb_of_parse_u32(np, pt, adj_max_cnt, 3);
	sb_of_parse_u32(np, pt, min_cap, 200);
	sb_of_parse_u32(np, pt, fixed_sc_cap, 900);
	sb_of_parse_u32(np, pt, max_icl, 3000);
	sb_of_parse_u32(np, pt, vfloat, 0);

	np = of_find_node_by_name(NULL, "battery");
	if (np) {
		if (pt->vfloat == 0) {
			ret = of_property_read_u32(np, "battery,chg_float_voltage",
				(unsigned int *)&pt->vfloat);
			if (ret) {
				pt_log("vfloat is Empty\n");
				pt->vfloat = 4400;
			}
		}

		ret = of_property_read_string(np,
			"battery,fuelgauge_name", (const char **)&pt->fg_name);
		if (ret)
			pt_log("failed to get fg name in battery dt (ret = %d)\n", ret);
	}

	ret = of_property_read_string(parent->of_node,
		"charger,main_charger", (const char **)&pt->sc_name);
	if (ret)
		pt_log("failed to get sc name in dc drv (ret = %d)\n", ret);

	ret = of_property_read_string(parent->of_node,
		"charger,direct_charger", (const char **)&pt->dc_name);
	if (ret)
		pt_log("failed to get dc name in dc drv (ret = %d)\n", ret);
#endif
	return 0;
}

struct sb_pt *sb_pt_init(struct device *parent)
{
	struct sb_pt *pt;
	int ret = 0;

	pt = kzalloc(sizeof(struct sb_pt), GFP_KERNEL);
	if (!pt)
		return ERR_PTR(-ENOMEM);

	ret = parse_dt(pt, parent);
	pt_log("parse_dt ret = %s\n", (ret) ? "fail" : "success");
	if (ret)
		goto failed_dt;

	pt->wq = create_singlethread_workqueue(PT_MODULE_NAME);
	if (!pt->wq) {
		ret = -ENOMEM;
		goto failed_wq;
	}

	pt->ws = wakeup_source_register(parent, PT_MODULE_NAME);

	INIT_DELAYED_WORK(&pt->start_work, cb_start_work);
	INIT_DELAYED_WORK(&pt->adjust_work, cb_adjust_work);
	INIT_DELAYED_WORK(&pt->step_work, cb_step_work);

	mutex_init(&pt->mlock);

	pt->chg_src = SEC_CHARGING_SOURCE_SWITCHING;
	pt->step = PT_STEP_NONE;
	pt->ref_cap = 0;
	pt->user_mode = PTM_NONE;
	pt->adj_state = CAP_NORMAL;
	pt->adj_cnt = 0;
	pt->adj_op_cnt = 0;

	ret = sb_sysfs_add_attrs(PT_MODULE_NAME, pt, pt_attr, ARRAY_SIZE(pt_attr));
	pt_log("sb_sysfs_add_attrs ret = %s\n", (ret) ? "fail" : "success");

	ret = sb_notify_register(&pt->nb, sb_noti_handler, PT_MODULE_NAME, SB_DEV_MODULE);
	pt_log("sb_notify_register ret = %s\n", (ret) ? "fail" : "success");

	return pt;

failed_wq:
failed_dt:
	kfree(pt);

	return ERR_PTR(ret);
}
EXPORT_SYMBOL(sb_pt_init);

#if defined(CONFIG_MULTI_DIRECT_CHARGER)
void sb_pt_change_dc_ic_src(struct sb_pt *pt, char *direct_charger_name)
{
	int len = 0;

	mutex_lock(&pt->mlock);
	len = strlen(direct_charger_name);
	pt->dc_name = kzalloc(len + 1, GFP_KERNEL);

	if (pt->dc_name) {
		strscpy(pt->dc_name, direct_charger_name, len + 1);
		pt_log("change dc name to %s\n", pt->dc_name);
	} else
		pt_log("failed to alloc memory for dc_name\n");

	mutex_unlock(&pt->mlock);
}
EXPORT_SYMBOL(sb_pt_change_dc_ic_src);
#endif

int sb_pt_psy_set_property(struct sb_pt *pt, enum power_supply_property psp, const union power_supply_propval *value)
{
	if (!pt)
		return 0;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_STATUS:
		pt->batt_status = value->intval;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		pt->cable_type = value->intval;

		mutex_lock(&pt->mlock);
		if (!is_pd_apdo_wire_type(pt->cable_type) &&
			(pt->step != PT_STEP_NONE))
			clear_state(pt, PT_STEP_NONE);
		mutex_unlock(&pt->mlock);
		break;
	case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE:
		/* Caution : dead lock */
		pt->dc_status = value->intval;
		break;
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(sb_pt_psy_set_property);

int sb_pt_psy_get_property(struct sb_pt *pt, enum power_supply_property psp, union power_supply_propval *value)
{
	int ret = 0;

	if (!pt)
		return 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		mutex_lock(&pt->mlock);
		if ((pt->step != PT_STEP_NONE) &&
			(pt->chg_src == SEC_CHARGING_SOURCE_SWITCHING)) {
			union power_supply_propval val;

			val.intval = 0;
			psy_do_property(pt->sc_name, get, psp, val);
			if (val.intval == POWER_SUPPLY_STATUS_FULL) {
				ret = -EBUSY;
				pt_log("prevent charging status\n");
				value->intval = POWER_SUPPLY_STATUS_CHARGING;
			}
		}
		mutex_unlock(&pt->mlock);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		mutex_lock(&pt->mlock);
		if (pt->step != PT_STEP_NONE) {
			union power_supply_propval val;

			val.intval = 0;
			psy_do_property(pt->dc_name, get, psp, val);

			if (val.intval == POWER_SUPPLY_EXT_HEALTH_DC_ERR) {
				pt_log("clear pt state because of DC err(%d)\n", val.intval);
				clear_state(pt, PT_STEP_NONE);
			}
		}
		mutex_unlock(&pt->mlock);
		break;
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL(sb_pt_psy_get_property);

int sb_pt_monitor(struct sb_pt *pt, int chg_src)
{
	if (!pt)
		return -EINVAL;

	mutex_lock(&pt->mlock);

	if (!check_state(pt)) {
		clear_state(pt, PT_STEP_NONE);
		goto end_monitor;
	}

	pt_log("start - step = %s, chg_src = %d, dc_status = %d\n", get_step_str(pt->step), chg_src, pt->dc_status);

	switch (pt->step) {
	case PT_STEP_NONE:
		push_start_work(pt, pt->start_delay);
		pt->chg_src = chg_src;
		break;
	case PT_STEP_INIT:
		if (pt->ref_cap <= 0) {
			union power_supply_propval value = { 0, };

			value.intval = SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE;
			psy_do_property(pt->fg_name, get,
				POWER_SUPPLY_PROP_CAPACITY, value);

			pt->ref_cap = value.intval;
			pt_log("update ref_cap = %d\n", pt->ref_cap);

			if ((chg_src == SEC_CHARGING_SOURCE_DIRECT) &&
				(pt->ref_cap > pt->fixed_sc_cap)) {
				/* reset chg src to switching charger */
				sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, true, SEC_BAT_CHG_MODE_CHARGING_OFF);
				pt->step = PT_STEP_PRESET;
				break;
			}
		}

		if (chg_src == SEC_CHARGING_SOURCE_SWITCHING) {
			pt->step = PT_STEP_PRESET;
		} else {
			if (check_preset_state(pt->dc_status))
				pt->step = PT_STEP_PRESET;
			else
				push_start_work(pt, pt->init_delay);
		}
		pt->chg_src = chg_src;
		break;
	case PT_STEP_PRESET:
	{
		union power_supply_propval value = { 0, };
		int iv, icl, fcc, chgen;

		if ((chg_src == SEC_CHARGING_SOURCE_DIRECT) &&
			(pt->ref_cap > pt->fixed_sc_cap))
			chg_src = SEC_CHARGING_SOURCE_SWITCHING;

		pt->chg_src = chg_src;
		pt->step = PT_STEP_ADJUST;

		if (chg_src == SEC_CHARGING_SOURCE_SWITCHING) {
			iv = SEC_INPUT_VOLTAGE_5V;
			icl = fcc = pt->max_icl;
		} else {
			iv = SEC_INPUT_VOLTAGE_APDO;
			icl = fcc = pt->max_icl * pt->user_mode;
		}
		chgen = SEC_BAT_CHG_MODE_PASS_THROUGH;

		sec_votef(IV_VOTE_NAME, VOTER_PASS_THROUGH, true, iv);
		sec_votef(ICL_VOTE_NAME, VOTER_PASS_THROUGH, true, icl);
		sec_votef(FCC_VOTE_NAME, VOTER_PASS_THROUGH, true, fcc);
		sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, true, chgen);

		if (chg_src == SEC_CHARGING_SOURCE_SWITCHING) {
			value.intval = pt->user_mode;
			psy_do_property(pt->sc_name, set,
				POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE, value);

			/* skip adjust work */
			pt->step = PT_STEP_MONITOR;
		} else {
			value.intval = pt->user_mode;
			psy_do_property(pt->dc_name, set,
				POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE, value);
			sec_pd_detach_with_cc(1);

			sec_votef(DCFV_VOTE_NAME, VOTER_PASS_THROUGH, true, pt->vfloat);

			/* set adj state */
			pt->adj_state = check_cap(pt);

			/* start adj work */
			__pm_stay_awake(pt->ws);
			queue_delayed_work(pt->wq, &pt->adjust_work, msecs_to_jiffies(pt->adj_delay));
		}

		set_misc_event(true);
	}
		break;
	case PT_STEP_ADJUST:
		/* not working */
		break;
	case PT_STEP_MONITOR:
	{
		int cap_state = CAP_NORMAL;

		if (pt->chg_src != chg_src) {
			clear_state(pt, PT_STEP_INIT);
			break;
		}

		cap_state = check_cap(pt);
		if (pt->chg_src == SEC_CHARGING_SOURCE_SWITCHING) {
			if (cap_state == CAP_LOW)
				sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, true, SEC_BAT_CHG_MODE_CHARGING);
			else
				sec_votef(CHGEN_VOTE_NAME, VOTER_PASS_THROUGH, true, SEC_BAT_CHG_MODE_PASS_THROUGH);
		} else {
			set_dc_ta_volt(pt, cap_state);
		}
	}
		break;
	case PT_STEP_RESET:
		break;
	default:
		break;
	}

end_monitor:
	pt_log("end - step = %s\n", get_step_str(pt->step));
	mutex_unlock(&pt->mlock);

	return 0;
}
EXPORT_SYMBOL(sb_pt_monitor);

int sb_pt_check_chg_src(struct sb_pt *pt, int chg_src)
{
	if (!pt)
		return chg_src;

	if ((pt->step != PT_STEP_NONE) &&
		(pt->ref_cap > pt->fixed_sc_cap))
		return SEC_CHARGING_SOURCE_SWITCHING;

	return chg_src;
}
EXPORT_SYMBOL(sb_pt_check_chg_src);

