/*
 * mfc_fod.h
 * Samsung Mobile MFC FOD Header
 *
 * Copyright (C) 2023 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MFC_FOD_H
#define __MFC_FOD_H __FILE__

#include <linux/err.h>

struct device;
struct mfc_fod;

#define MFC_FOD_MAX_SIZE	256

enum {
	MFC_FOD_BAT_STATE_CC = 0,
	MFC_FOD_BAT_STATE_CV,
	MFC_FOD_BAT_STATE_FULL,
	MFC_FOD_BAT_STATE_MAX
};

union mfc_fod_state {
	unsigned long long	value;

	/*
	 * Maintain order if possible,
	 * Max size is 64 bits
	 */
	struct {
		unsigned	tx_id : 8,
			vendor_id : 8,
			op_mode : 8,
			fake_op_mode : 8,
			bat_state : 7,
			magnet_case_det : 1,
			bat_cap : 7,
			high_swell : 1,
			vout : 16;
	};
};

typedef unsigned int fod_data_t;
typedef int (*mfc_set_fod)(struct device *dev, union mfc_fod_state *state, fod_data_t *data);

enum {
	MFC_FOD_EXT_EPP_REF_QF = 0,
	MFC_FOD_EXT_EPP_REF_RF,

	MFC_FOD_EXT_MAX
};

#define MFC_FOD_DISABLED		(-ESRCH)
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
struct mfc_fod *mfc_fod_init(struct device *dev, unsigned int fod_size, mfc_set_fod cb_func);

int mfc_fod_init_state(struct mfc_fod *fod);
int mfc_fod_refresh(struct mfc_fod *fod);

int mfc_fod_set_op_mode(struct mfc_fod *fod, int op_mode);
int mfc_fod_set_vendor_id(struct mfc_fod *fod, int vendor_id);
int mfc_fod_set_tx_id(struct mfc_fod *fod, int tx_id);
int mfc_fod_set_bat_state(struct mfc_fod *fod, int bat_state);
int mfc_fod_set_magnet_case_det(struct mfc_fod *fod, bool magnet_case_det);
int mfc_fod_set_bat_cap(struct mfc_fod *fod, int bat_cap);
int mfc_fod_set_vout(struct mfc_fod *fod, int vout);
int mfc_fod_set_high_swell(struct mfc_fod *fod, bool state);

int mfc_fod_get_state(struct mfc_fod *fod, union mfc_fod_state *state);
int mfc_fod_get_ext(struct mfc_fod *fod, int ext_type, int *data);
#else
static inline struct mfc_fod *mfc_fod_init(struct device *dev, unsigned int fod_size, mfc_set_fod cb_func)
{ return ERR_PTR(MFC_FOD_DISABLED); }

static inline int mfc_fod_init_state(struct mfc_fod *fod)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_refresh(struct mfc_fod *fod)
{ return MFC_FOD_DISABLED; }

static inline int mfc_fod_set_op_mode(struct mfc_fod *fod, int op_mode)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_magnet_case_det(struct mfc_fod *fod, int magnet_case_det)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_vendor_id(struct mfc_fod *fod, int vendor_id)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_tx_id(struct mfc_fod *fod, int tx_id)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_bat_state(struct mfc_fod *fod, int bat_state)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_bat_cap(struct mfc_fod *fod, int bat_cap)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_vout(struct mfc_fod *fod, int vout)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_set_high_swell(struct mfc_fod *fod, bool state)
{ return MFC_FOD_DISABLED; }

static inline int mfc_fod_get_state(struct mfc_fod *fod, union mfc_fod_state *state)
{ return MFC_FOD_DISABLED; }
static inline int mfc_fod_get_ext(struct mfc_fod *fod, int ext_type, int *data)
{ return MFC_FOD_DISABLED; }
#endif

#endif /* __MFC_FOD_H */
