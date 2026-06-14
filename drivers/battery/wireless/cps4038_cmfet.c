/*
 *  mfc_cmfet.c
 *  Samsung Mobile MFC CMFET Module
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

#include "cps4038_charger.h"

#define cmfet_log(str, ...) pr_info("[MFC-CMFET]:%s: "str, __func__, ##__VA_ARGS__)

struct mfc_cmfet {
	struct device			*parent;
	mfc_set_cmfet			cb_func;

	struct mutex			lock;
	union mfc_cmfet_state	state;

	/* threshold */
	bool				unknown_cmb_ctrl;
	bool				p5200_cmb;
	bool				p1100_cmb_off_soc;
	bool				check_p5200_p;
	unsigned int		high_swell_cc_cv_thr;
};

static int mfc_cmfet_parse_dt(struct device_node *np, struct mfc_cmfet *cmfet)
{
	int ret = 0;

	cmfet->unknown_cmb_ctrl = of_property_read_bool(np, "battery,unknown_cmb_ctrl");
	cmfet->p5200_cmb = !of_property_read_bool(np, "battery,p5200_cmb");
	cmfet->check_p5200_p = of_property_read_bool(np, "battery,check_p5200_p");
	cmfet->p1100_cmb_off_soc = of_property_read_bool(np, "battery,p1100_cmb_off_soc");

	ret = of_property_read_u32(np, "high_swell_cc_cv_thr", &cmfet->high_swell_cc_cv_thr);
	if (ret < 0)
		cmfet->high_swell_cc_cv_thr = 70;


	cmfet_log("unknown_cmb_ctrl = %d, high_swell_cc_cv_thr = %d p5200_cmb = %d, p1100_cmb_off_soc = %d\n",
		cmfet->unknown_cmb_ctrl, cmfet->high_swell_cc_cv_thr, cmfet->p5200_cmb, cmfet->p1100_cmb_off_soc);
	return 0;
}

struct mfc_cmfet *mfc_cmfet_init(struct device *dev, mfc_set_cmfet cb_func)
{
	struct mfc_cmfet *cmfet;
	int ret = 0;

	if (IS_ERR_OR_NULL(dev) ||
		(cb_func == NULL))
		return ERR_PTR(-EINVAL);

	cmfet = kzalloc(sizeof(struct mfc_cmfet), GFP_KERNEL);
	if (!cmfet)
		return ERR_PTR(-ENOMEM);

	ret = mfc_cmfet_parse_dt(dev->of_node, cmfet);
	if (ret < 0) {
		kfree(cmfet);
		return ERR_PTR(ret);
	}

	mutex_init(&cmfet->lock);

	cmfet->parent = dev;
	cmfet->cb_func = cb_func;
	cmfet->state.value = 0;

	cmfet_log("DONE!!\n");
	return cmfet;
}
EXPORT_SYMBOL(mfc_cmfet_init);

int mfc_cmfet_init_state(struct mfc_cmfet *cmfet)
{
	if (IS_ERR(cmfet))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.value != 0) {
		cmfet->state.value = 0;

		cmfet->cb_func(cmfet->parent, &cmfet->state, false, false);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_init_state);

int mfc_cmfet_refresh(struct mfc_cmfet *cmfet)
{
	if (IS_ERR(cmfet))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.value != 0)
		cmfet->cb_func(cmfet->parent, &cmfet->state, cmfet->state.cma, cmfet->state.cmb);
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_refresh);

static void set_init_state(union mfc_cmfet_state *state, bool cma, bool cmb)
{
	state->cma = cma;
	state->cmb = cmb;
}

static unsigned long long check_tx_default(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	if (state->high_swell)
		state->cmb = (state->bat_cap > cmfet->high_swell_cc_cv_thr);

	if (state->full)
		state->cmb = true;

	if (state->chg_done)
		state->cmb = false;

	return state->value;
}

static unsigned long long check_tx_unknown(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	set_init_state(state, true, (cmfet->unknown_cmb_ctrl));

	return state->value;
}

static unsigned long long check_tx_p1100(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	set_init_state(state, true, true);

	if (state->vout <= 5500) {
		if (cmfet->p1100_cmb_off_soc) {
			if (state->bat_cap <= 85)
				state->cmb = false;
		} else
			state->cmb = false;
	}

	if (state->chg_done)
		state->cmb = false;

	return state->value;
}

static unsigned long long check_tx_n5200(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	set_init_state(state, true, false);

	return check_tx_default(cmfet, state);
}

static unsigned long long check_tx_p5200(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	set_init_state(state, true, cmfet->p5200_cmb);

	if (state->auth)
		state->cmb = false;

	return check_tx_default(cmfet, state);
}

static unsigned long long check_cmfet_state(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	switch (state->tx_id) {
	case WC_PAD_UNKNOWN:
		return check_tx_unknown(cmfet, state);
	case WC_PAD_P1100:
		return check_tx_p1100(cmfet, state);
	case WC_PAD_N5200_P:
	case WC_PAD_N5200_L:
		return check_tx_n5200(cmfet, state);
	case WC_PAD_P5200_P:
		if (cmfet->check_p5200_p)
			return check_tx_p5200(cmfet, state);
		break;
	case WC_PAD_P5200:
		return check_tx_p5200(cmfet, state);
	default:
		break;
	}

	set_init_state(state, true, true);
	return check_tx_default(cmfet, state);
}

static bool is_changed_state(union mfc_cmfet_state *state1, union mfc_cmfet_state *state2)
{
	return (state1->cma != state2->cma) || (state1->cmb != state2->cmb);
}

static int update_cmfet_state(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	state->value = check_cmfet_state(cmfet, state);

	if (is_changed_state(state, &cmfet->state))
		cmfet->cb_func(cmfet->parent, state, state->cma, state->cmb);

	cmfet->state.value = state->value;
	return 0;
}

int mfc_cmfet_set_tx_id(struct mfc_cmfet *cmfet, int tx_id)
{
	if (IS_ERR(cmfet) ||
		(tx_id < 0) || (tx_id >= 256))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.tx_id != tx_id) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.tx_id = tx_id;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_tx_id);

int mfc_cmfet_set_bat_cap(struct mfc_cmfet *cmfet, int bat_cap)
{
	if (IS_ERR(cmfet) ||
		(bat_cap < 0) || (bat_cap > 100))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.bat_cap != bat_cap) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.bat_cap = bat_cap;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_bat_cap);

int mfc_cmfet_set_vout(struct mfc_cmfet *cmfet, int vout)
{
	if (IS_ERR(cmfet) ||
		(vout <= 0))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.vout != vout) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.vout = vout;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_vout);

int mfc_cmfet_set_high_swell(struct mfc_cmfet *cmfet, bool state)
{
	if (IS_ERR(cmfet))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.high_swell != state) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.high_swell = state;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_high_swell);

int mfc_cmfet_set_full(struct mfc_cmfet *cmfet, bool full)
{
	if (IS_ERR(cmfet))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.full != full) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.full = full;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_full);

int mfc_cmfet_set_chg_done(struct mfc_cmfet *cmfet, bool chg_done)
{
	if (IS_ERR(cmfet))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.chg_done != chg_done) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.chg_done = chg_done;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_chg_done);

int mfc_cmfet_set_auth(struct mfc_cmfet *cmfet, bool auth)
{
	if (IS_ERR(cmfet))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	if (cmfet->state.auth != auth) {
		union mfc_cmfet_state temp = { cmfet->state.value, };

		temp.auth = auth;
		update_cmfet_state(cmfet, &temp);
	}
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_set_auth);

int mfc_cmfet_get_state(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{
	if (IS_ERR(cmfet) ||
		(state == NULL))
		return -EINVAL;

	mutex_lock(&cmfet->lock);
	state->value = cmfet->state.value;
	mutex_unlock(&cmfet->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_cmfet_get_state);
