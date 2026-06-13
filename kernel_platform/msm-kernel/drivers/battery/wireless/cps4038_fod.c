/*
 *  mfc_fod.c
 *  Samsung Mobile MFC FOD Module
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

#include <linux/battery/sb_wireless.h>
#include <dt-bindings/battery/sec-battery.h>

#include "cps4038_fod.h"

#define fod_log(str, ...) pr_info("[MFC-FOD]:%s: "str, __func__, ##__VA_ARGS__)
#define DEFAULT_TX_IDX		0
#define DEFAULT_OP_MODE		WPC_OP_MODE_BPP
#define DEFAULT_VENDOR_ID	0x42

struct mfc_fod_op {
	unsigned int flag;
	fod_data_t	*data[MFC_FOD_BAT_STATE_MAX];
};

struct mfc_fod_tx {
	unsigned int id;
	struct mfc_fod_op op[WPC_OP_MODE_MAX]; /* Regular FOD Opmode Data*/
	struct mfc_fod_op op_mc[WPC_OP_MODE_MAX]; /* Magnet Case FOD Opmode Data*/
};

struct mfc_fod {
	struct device	*parent;
	mfc_set_fod		cb_func;

	struct mutex		lock;
	union mfc_fod_state	state;

	/* fod data */
	struct mfc_fod_tx	*list;
	unsigned int		count;
	unsigned int		ext[MFC_FOD_EXT_MAX];

	/* threshold */
	unsigned int		bpp_vout;
	unsigned int		cc_cv_thr;
	unsigned int		high_swell_cc_cv_thr;
	unsigned int		vendor_id;
};

static int get_op_mode_by_str(const char *str)
{
	if (str == NULL)
		return WPC_OP_MODE_NONE;

	if (!strncmp(str, "ppde", 4))
		return WPC_OP_MODE_PPDE;

	if (!strncmp(str, "epp", 3))
		return WPC_OP_MODE_EPP;

	if (!strncmp(str, "mpp", 3))
		return WPC_OP_MODE_MPP;

	return WPC_OP_MODE_BPP;
}

static const char *get_ext_str(unsigned int ext_type)
{
	switch (ext_type) {
	case MFC_FOD_EXT_EPP_REF_QF:
		return "epp_ref_qf";
	case MFC_FOD_EXT_EPP_REF_RF:
		return "epp_ref_rf";
	}

	return "none";
}

static const char *get_bat_state_str(unsigned int bat_state)
{
	switch (bat_state) {
	case MFC_FOD_BAT_STATE_CC:
		return "cc";
	case MFC_FOD_BAT_STATE_CV:
		return "cv";
	case MFC_FOD_BAT_STATE_FULL:
		return "full";
	}

	return "none";
}

static fod_data_t *mfc_fod_parse_data(struct device_node *np, int bat_state, unsigned int fod_size)
{
	fod_data_t *data;
	const u32 *p;
	int len = 0, t_size = 0, ret;

	p = of_get_property(np, get_bat_state_str(bat_state), &len);
	if (!p)
		return NULL;

	t_size = sizeof(fod_data_t);
	data = kcalloc(fod_size, t_size, GFP_KERNEL);
	if (!data)
		return NULL;

	if (fod_size != (len / t_size)) {
		fod_log("not match the size(%d, %d, %d)\n", fod_size, len, t_size);
		goto err_size;
	}

	switch (t_size) {
	case sizeof(u64):
		ret = of_property_read_u64_array(np, get_bat_state_str(bat_state),
			(u64 *)data, fod_size);
		break;
	case sizeof(u32):
		ret = of_property_read_u32_array(np, get_bat_state_str(bat_state),
			(u32 *)data, fod_size);
		break;
	case sizeof(u16):
		ret = of_property_read_u16_array(np, get_bat_state_str(bat_state),
			(u16 *)data, fod_size);
		break;
	case sizeof(u8):
		ret = of_property_read_u8_array(np, get_bat_state_str(bat_state),
			(u8 *)data, fod_size);
		break;
	default:
		fod_log("invalid t size(%d)\n", t_size);
		goto err_size;
	}

	if (ret < 0) {
		fod_log("%s, failed to parse data (ret = %d)\n", get_bat_state_str(bat_state), ret);
		goto err_size;
	}

	return data;

err_size:
	kfree(data);
	return NULL;
}

static int mfc_fod_parse_op_node(struct device_node *np, struct mfc_fod *fod,
		struct mfc_fod_tx *fod_tx, int op_mode, unsigned int fod_size, bool magnet_case)
{
	struct mfc_fod_op *fod_op = magnet_case ? &fod_tx->op_mc[op_mode] : &fod_tx->op[op_mode];
	unsigned int flag = 0;
	int ret = 0, i;

	ret = of_property_read_u32(np, "flag", &flag);
	if (ret < 0) {
		pr_err("%s: failed to get flag of %s\n", __func__, np->name);
		return ret;
	}

	for (i = MFC_FOD_BAT_STATE_CC; i < MFC_FOD_BAT_STATE_MAX; i++) {
		switch ((flag >> (i * 4)) & 0xF) {
		case FOD_FLAG_ADD:
			fod_op->data[i] = mfc_fod_parse_data(np, i, fod_size);
			if (fod_op->data[i] == NULL) {
				ret = -1;
				goto err_data;
			}
			break;
		case FOD_FLAG_USE_CC:
			if (fod_op->data[MFC_FOD_BAT_STATE_CC] == NULL) {
				ret = -2;
				goto err_data;
			}

			fod_op->data[i] = fod_op->data[MFC_FOD_BAT_STATE_CC];
			break;
		case FOD_FLAG_USE_CV:
			if (fod_op->data[MFC_FOD_BAT_STATE_CV] == NULL) {
				ret = -3;
				goto err_data;
			}

			fod_op->data[i] = fod_op->data[MFC_FOD_BAT_STATE_CV];
			break;
		case FOD_FLAG_USE_FULL:
			if (fod_op->data[MFC_FOD_BAT_STATE_FULL] == NULL) {
				ret = -4;
				goto err_data;
			}

			fod_op->data[i] = fod_op->data[MFC_FOD_BAT_STATE_FULL];
			break;
		case FOD_FLAG_USE_DEF_PAD:
		{
			struct mfc_fod_tx *def_tx = &fod->list[DEFAULT_TX_IDX];
			struct mfc_fod_op *def_op = magnet_case ? &def_tx->op_mc[op_mode] : &def_tx->op[op_mode];

			if (def_op->data[i] == NULL) {
				ret = -5;
				goto err_data;
			}

			fod_op->data[i] = def_tx->op[op_mode].data[i];
		}
			break;
		case FOD_FLAG_USE_DEF_OP:
		{
			struct mfc_fod_op *def_op = magnet_case ? &fod_tx->op_mc[DEFAULT_OP_MODE] : &fod_tx->op[DEFAULT_OP_MODE];

			if (def_op->data[i] == NULL) {
				ret = -6;
				goto err_data;
			}

			fod_op->data[i] = def_op->data[i];
		}
			break;
		case FOD_FLAG_NONE:
		default:
			fod_log("%s - %s is not set\n", np->name, get_bat_state_str(i));
			break;
		}
	}

	fod_op->flag = flag;
	return 0;

err_data:
	for (; i >= 0; i--) {
		if (((flag >> (i * 4)) & 0xF) == FOD_FLAG_ADD)
			kfree(fod_op->data[i]);
	}

	return ret;
}

static int mfc_fod_init_ext_pad(struct mfc_fod_tx *fod_tx, struct mfc_fod_tx *def_tx)
{
	int i, j;

	if (fod_tx->id == DEFAULT_TX_IDX)
		return 0;

	for (j = WPC_OP_MODE_NONE; j < WPC_OP_MODE_MAX; j++) {
		if (def_tx->op[j].flag == 0)
			continue;

		for (i = MFC_FOD_BAT_STATE_CC; i < MFC_FOD_BAT_STATE_MAX; i++)
			fod_tx->op[j].data[i] = def_tx->op[j].data[i];

		fod_tx->op[j].flag = (SET_FOD_CC(USE_DEF_PAD) | SET_FOD_CV(USE_DEF_PAD) | SET_FOD_FULL(USE_DEF_PAD));

		if (def_tx->op_mc[j].flag == 0)
			continue;

		for (i = MFC_FOD_BAT_STATE_CC; i < MFC_FOD_BAT_STATE_MAX; i++)
			fod_tx->op_mc[j].data[i] = def_tx->op_mc[j].data[i];

		fod_tx->op_mc[j].flag = (SET_FOD_CC(USE_DEF_PAD) | SET_FOD_CV(USE_DEF_PAD) | SET_FOD_FULL(USE_DEF_PAD));
	}

	return 0;
}

static int mfc_fod_parse_tx_node(struct device_node *np, struct mfc_fod *fod, unsigned int fod_size)
{
	struct device_node *tx_node = NULL;
	int ret = 0, tx_idx = 0;

	for_each_child_of_node(np, tx_node) {
		struct mfc_fod_tx *fod_tx = NULL;
		struct device_node *op_node = NULL;

		if (tx_idx >= fod->count) {
			fod_log("out of range(%d <--> %d)\n", tx_idx, fod->count);
			break;
		}

		fod_tx = &fod->list[tx_idx++];
		if (sscanf(tx_node->name, "pad_0x%X", &fod_tx->id) < 0) {
			fod_log("failed to get tx id(%s)\n", tx_node->name);
			continue;
		}

		mfc_fod_init_ext_pad(fod_tx, &fod->list[DEFAULT_TX_IDX]);

		for_each_child_of_node(tx_node, op_node) {
			int op_mode;
			bool magnet_case_data = false;

			op_mode = get_op_mode_by_str(op_node->name);
			if (op_mode == WPC_OP_MODE_NONE) {
				fod_log("%s, invalid op name\n", op_node->name);
				continue;
			}
			/*Checks if the op_node contains Magnet Case Data*/
			if (strstr(op_node->name, "magnet_case") != NULL)
				magnet_case_data = true;

			ret = mfc_fod_parse_op_node(op_node, fod, fod_tx, op_mode, fod_size, magnet_case_data);
			if (ret < 0)
				fod_log("%s, failed to parse data(ret = %d)\n", op_node->name, ret);
		}
	}

	return 0;
}

static void mfc_fod_print_op_cc_cv_full(struct mfc_fod_op fod_op, int tx_id, unsigned int fod_size, char *op_mode_str)
{
	int z, k;

	for (z = MFC_FOD_BAT_STATE_CC; z < MFC_FOD_BAT_STATE_MAX; z++) {
		char temp_buf[1024] = {0, };
		int size = 1024;

		if (fod_op.flag == 0 || fod_op.data[z] == NULL) {
			fod_log("PAD_0x%02X:%s:%s is null!!\n", tx_id, op_mode_str, get_bat_state_str(z));
			continue;
		}

		for (k = 0; k < fod_size; k++) {
			snprintf(temp_buf + strlen(temp_buf), size, "0x%02X ", fod_op.data[z][k]);
			size = sizeof(temp_buf) - strlen(temp_buf);
		}

		fod_log("PAD_0x%02X:%s:%s - %s\n", tx_id, op_mode_str, get_bat_state_str(z), temp_buf);
	}
}

static void mfc_fod_print_data(struct mfc_fod *fod, unsigned int fod_size)
{
	int x, y;
	struct mfc_fod_tx *fod_tx;

	for (x = 0; x < fod->count; x++) {
		fod_tx = &fod->list[x];

		for (y = WPC_OP_MODE_NONE + 1; y < WPC_OP_MODE_MAX; y++) {
			char op_mode_str[32] = {0, };
			int size = 32;

			snprintf(op_mode_str, size, "%s", sb_wrl_op_mode_str(y));
			size = sizeof(op_mode_str) - strlen(op_mode_str);

			/*Print Regular FOD Params*/
			mfc_fod_print_op_cc_cv_full(fod_tx->op[y], fod_tx->id, fod_size, op_mode_str);

			snprintf(op_mode_str + strlen(op_mode_str), size, "_MC");
			/*Print Magnet FOD Params*/
			mfc_fod_print_op_cc_cv_full(fod_tx->op_mc[y], fod_tx->id, fod_size, op_mode_str);
		}
	}
}

static void mfc_fod_print_ext(struct mfc_fod *fod)
{
	char ext_buf[1024] = {0, };
	int x, ext_size = 1024;

	for (x = 0; x < MFC_FOD_EXT_MAX; x++) {
		snprintf(ext_buf + strlen(ext_buf), ext_size, "%02d:0x%X ", x, fod->ext[x]);
		ext_size = sizeof(ext_buf) - strlen(ext_buf);
	}
	fod_log("EXT - %s\n", ext_buf);
}

static int mfc_fod_parse_dt(struct device_node *np, struct mfc_fod *fod, unsigned int fod_size)
{
	int ret = 0, i;

	np = of_find_node_by_name(np, "fod_list");
	if (!np) {
		fod_log("fod list is null!!!!\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "count", &fod->count);
	if (ret < 0) {
		fod_log("count is null(ret = %d)\n", ret);
		return ret;
	}

	fod->list = kcalloc(fod->count, sizeof(struct mfc_fod_tx), GFP_KERNEL);
	if (!fod->list) {
		fod_log("failed to alloc fod list\n");
		return -ENOMEM;
	}

	ret = mfc_fod_parse_tx_node(np, fod, fod_size);
	if (ret < 0) {
		kfree(fod->list);
		return ret;
	}

	/* parse ext */
	for (i = 0; i < MFC_FOD_EXT_MAX; i++) {
		ret = of_property_read_u32(np, get_ext_str(i), (unsigned int *)&fod->ext[i]);
		if (ret < 0)
			fod_log("%s is null(ret = %d)!!\n", get_ext_str(i), ret);
	}

	mfc_fod_print_data(fod, fod_size);
	mfc_fod_print_ext(fod);

	return 0;
}

static int mfc_fod_thr_parse_dt(struct device_node *np, struct mfc_fod *fod)
{
	int ret = 0;

	ret = of_property_read_u32(np, "bpp_vout", &fod->bpp_vout);
	if (ret < 0)
		fod->bpp_vout = 7000;

	ret = of_property_read_u32(np, "cc_cv_thr", &fod->cc_cv_thr);
	if (ret < 0)
		fod->cc_cv_thr = 85;

	ret = of_property_read_u32(np, "high_swell_cc_cv_thr", &fod->high_swell_cc_cv_thr);
	if (ret < 0)
		fod->high_swell_cc_cv_thr = 70;

	ret = of_property_read_u32(np, "vendor_id", (unsigned int *)&fod->vendor_id);
	if (ret < 0) {
		fod_log("vendor_id is null(ret = %d)!!\n", ret);
		fod->vendor_id = DEFAULT_VENDOR_ID;
	}

	fod_log("bpp_vout = %d, cc_cv_thr = %d, high_swell_cc_cv_thr = %d, vendor_id = 0x%x\n",
		fod->bpp_vout, fod->cc_cv_thr, fod->high_swell_cc_cv_thr, fod->vendor_id);
	return 0;
}

static struct mfc_fod_tx *get_fod_tx(struct mfc_fod *fod, unsigned int tx_id)
{
	int i;

	for (i = 0; i < fod->count; i++) {
		if (fod->list[i].id == tx_id)
			return &fod->list[i];
	}

	return &fod->list[DEFAULT_TX_IDX];
}

static bool check_vendor_id_to_set_op_mode_by_vout(union mfc_fod_state *state, int vendor_id)
{
	return (state->vendor_id == vendor_id);
}

static int check_op_mode_vout(struct mfc_fod *fod, union mfc_fod_state *state)
{
	if (!check_vendor_id_to_set_op_mode_by_vout(state, fod->vendor_id))
		return state->op_mode;

	switch (state->op_mode) {
	case WPC_OP_MODE_EPP:
	case WPC_OP_MODE_PPDE:
		if (state->vout <= fod->bpp_vout)
			return WPC_OP_MODE_BPP;
		break;
	case WPC_OP_MODE_MPP:
		break;
	default:
		/* default op mode */
		return WPC_OP_MODE_BPP;
	}

	return state->op_mode;
}

static fod_data_t *mfc_fod_get_data(struct mfc_fod *fod)
{
	union mfc_fod_state *fod_state = &fod->state;
	struct mfc_fod_tx *fod_tx;
	struct mfc_fod_op fod_op;
	int op_mode = 0;

	if (fod->count <= 0)
		return NULL;

	fod_tx = get_fod_tx(fod, fod_state->tx_id);
	op_mode = check_op_mode_vout(fod, fod_state);
	fod_state->fake_op_mode = op_mode;

	/* Only Use MC Op Data if Magnet Case Det is True and MC Op Data Exists (i.e. Flag isnt 0)*/
	fod_log("Opmode(%s) Magnet Case(%d) Flag(%d) Flag_MC(%d)\n",
		sb_wrl_op_mode_str(op_mode), fod_state->magnet_case_det, fod_tx->op[op_mode].flag, fod_tx->op_mc[op_mode].flag);

	if (fod_state->magnet_case_det && fod_tx->op_mc[op_mode].flag) {
		fod_op = fod_tx->op_mc[op_mode];
		fod_log("Magnet Case FOD Data Used\n");
	} else {
		fod_op = fod_tx->op[op_mode];
		fod_log("Regular FOD Data Used\n");
	}
	return fod_op.data[fod_state->bat_state];
}

struct mfc_fod *mfc_fod_init(struct device *dev, unsigned int fod_size, mfc_set_fod cb_func)
{
	struct mfc_fod	*fod;
	int ret = 0;

	if (IS_ERR_OR_NULL(dev) ||
		(fod_size <= 0) ||
		(fod_size > MFC_FOD_MAX_SIZE) ||
		(cb_func == NULL))
		return ERR_PTR(-EINVAL);

	fod = kzalloc(sizeof(struct mfc_fod), GFP_KERNEL);
	if (!fod)
		return ERR_PTR(-ENOMEM);

	ret = mfc_fod_parse_dt(dev->of_node, fod, fod_size);
	if (ret < 0) {
		kfree(fod);
		return ERR_PTR(ret);
	}
	mfc_fod_thr_parse_dt(dev->of_node, fod);

	mutex_init(&fod->lock);

	fod->parent = dev;
	fod->cb_func = cb_func;
	fod->state.value = 0;

	fod_log("DONE!!\n");
	return fod;
}
EXPORT_SYMBOL(mfc_fod_init);

int mfc_fod_init_state(struct mfc_fod *fod)
{
	if (IS_ERR(fod))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.value != 0) {
		fod->state.value = 0;

		fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_init_state);

int mfc_fod_refresh(struct mfc_fod *fod)
{
	if (IS_ERR(fod))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.value != 0)
		fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_refresh);

int mfc_fod_set_op_mode(struct mfc_fod *fod, int op_mode)
{
	if (IS_ERR(fod) ||
		(op_mode < WPC_OP_MODE_NONE) ||
		(op_mode >= WPC_OP_MODE_MAX))
		return -EINVAL;

	mutex_lock(&fod->lock);
	switch (fod->state.op_mode) {
	case WPC_OP_MODE_EPP:
	case WPC_OP_MODE_MPP:
	case WPC_OP_MODE_PPDE:
		fod_log("prevent op mode(%d)!!\n", op_mode);
		break;
	case WPC_OP_MODE_BPP:
	case WPC_OP_MODE_NONE:
	default:
		if (fod->state.op_mode != op_mode) {
			fod->state.op_mode = op_mode;

			fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
		}
		break;
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_op_mode);


int mfc_fod_set_magnet_case_det(struct mfc_fod *fod, bool magnet_case_det)
{
	if (IS_ERR(fod))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.magnet_case_det != magnet_case_det) {
		fod->state.magnet_case_det = magnet_case_det;

		fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_magnet_case_det);

int mfc_fod_set_vendor_id(struct mfc_fod *fod, int vendor_id)
{
	if (IS_ERR(fod) ||
		(vendor_id < 0) ||
		(vendor_id > 0xFF))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (vendor_id != fod->state.vendor_id) {
		fod->state.vendor_id = vendor_id;

		fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_vendor_id);

int mfc_fod_set_tx_id(struct mfc_fod *fod, int tx_id)
{
	if (IS_ERR(fod) ||
		(tx_id < 0) || (tx_id >= 256))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.tx_id != tx_id) {
		fod->state.tx_id = tx_id;

		fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_tx_id);

static int check_bat_state(struct mfc_fod *fod)
{
	if (fod->state.high_swell)
		return (fod->state.bat_cap > fod->high_swell_cc_cv_thr) ?
			MFC_FOD_BAT_STATE_CV : MFC_FOD_BAT_STATE_CC;

	return (fod->state.bat_cap > fod->cc_cv_thr) ?
		MFC_FOD_BAT_STATE_CV : MFC_FOD_BAT_STATE_CC;
}

static int set_bat_state(struct mfc_fod *fod, int bat_state)
{
	switch (fod->state.bat_state) {
	case MFC_FOD_BAT_STATE_FULL:
		fod_log("prevent bat state(%d)!!\n", bat_state);
		break;
	case MFC_FOD_BAT_STATE_CC:
	case MFC_FOD_BAT_STATE_CV:
	default:
		if (fod->state.bat_state != bat_state) {
			fod->state.bat_state = bat_state;

			fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
		}
		break;
	}

	return 0;
}

int mfc_fod_set_bat_state(struct mfc_fod *fod, int bat_state)
{
	if (IS_ERR(fod) ||
		(bat_state < MFC_FOD_BAT_STATE_CC) ||
		(bat_state >= MFC_FOD_BAT_STATE_MAX))
		return -EINVAL;

	mutex_lock(&fod->lock);
	set_bat_state(fod, bat_state);
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_bat_state);

int mfc_fod_set_bat_cap(struct mfc_fod *fod, int bat_cap)
{
	if (IS_ERR(fod) ||
		(bat_cap < 0) || (bat_cap > 100))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.bat_cap != bat_cap) {
		fod->state.bat_cap = bat_cap;

		set_bat_state(fod, check_bat_state(fod));
	}
	mutex_unlock(&fod->lock);

	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_bat_cap);

int mfc_fod_set_vout(struct mfc_fod *fod, int vout)
{
	if (IS_ERR(fod) ||
		(vout <= 0))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.vout != vout) {
		int new_op_mode, old_op_mode;

		old_op_mode = check_op_mode_vout(fod, &fod->state);

		fod->state.vout = vout;
		new_op_mode = check_op_mode_vout(fod, &fod->state);

		if (new_op_mode != old_op_mode)
			fod->cb_func(fod->parent, &fod->state, mfc_fod_get_data(fod));
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_vout);

int mfc_fod_set_high_swell(struct mfc_fod *fod, bool state)
{
	if (IS_ERR(fod))
		return -EINVAL;

	mutex_lock(&fod->lock);
	if (fod->state.high_swell != state) {
		fod->state.high_swell = state;

		set_bat_state(fod, check_bat_state(fod));
	}
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_set_high_swell);

int mfc_fod_get_state(struct mfc_fod *fod, union mfc_fod_state *state)
{
	if (IS_ERR(fod) ||
		(state == NULL))
		return -EINVAL;

	mutex_lock(&fod->lock);
	state->value = fod->state.value;
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_get_state);

int mfc_fod_get_ext(struct mfc_fod *fod, int ext_type, int *data)
{
	if (IS_ERR(fod) ||
		(ext_type < MFC_FOD_EXT_EPP_REF_QF) ||
		(ext_type >= MFC_FOD_EXT_MAX) ||
		(data == NULL))
		return -EINVAL;

	mutex_lock(&fod->lock);
	*data = fod->ext[ext_type];
	mutex_unlock(&fod->lock);
	return 0;
}
EXPORT_SYMBOL(mfc_fod_get_ext);
