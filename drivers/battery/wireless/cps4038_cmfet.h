/*
 * mfc_cmfet.h
 * Samsung Mobile MFC CMFET Header
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

#ifndef __MFC_CMFET_H
#define __MFC_CMFET_H __FILE__

#include <linux/err.h>

struct device;
struct mfc_cmfet;

union mfc_cmfet_state {
	unsigned long long	value;

	/*
	 * Maintain order,
	 * New variables should be using resv fields,
	 * Max size is 64 bits
	 */
	struct {
		unsigned	cma : 1,
			cmb : 1,
			chg_done : 1,
			high_swell : 1,
			auth : 1,
			resv : 3,
			tx_id : 8,
			vout : 16,
			bat_cap : 7,
			full : 1,
			mpp_epp_tx_id : 8,
			resv2 : 16;
	};
};

typedef int (*mfc_set_cmfet)(struct device *dev, union mfc_cmfet_state *state, bool cma, bool cmb);

#define MFC_CMFET_DISABLED		(-ESRCH)
#if IS_ENABLED(CONFIG_WIRELESS_CHARGING)
struct mfc_cmfet *mfc_cmfet_init(struct device *dev, mfc_set_cmfet cb_func);

int mfc_cmfet_init_state(struct mfc_cmfet *cmfet);
int mfc_cmfet_refresh(struct mfc_cmfet *cmfet);

int mfc_cmfet_set_tx_id(struct mfc_cmfet *cmfet, int tx_id);
int mfc_cmfet_set_bat_cap(struct mfc_cmfet *cmfet, int bat_cap);
int mfc_cmfet_set_vout(struct mfc_cmfet *cmfet, int vout);
int mfc_cmfet_set_high_swell(struct mfc_cmfet *cmfet, bool state);
int mfc_cmfet_set_full(struct mfc_cmfet *cmfet, bool full);
int mfc_cmfet_set_chg_done(struct mfc_cmfet *cmfet, bool chg_done);
int mfc_cmfet_set_auth(struct mfc_cmfet *cmfet, bool auth);

int mfc_cmfet_get_state(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state);
#else
static inline struct mfc_cmfet *mfc_cmfet_init(struct device *dev, mfc_set_cmfet cb_func)
{ return ERR_PTR(MFC_CMFET_DISABLED); }

static inline int mfc_cmfet_init_state(struct mfc_cmfet *cmfet)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_refresh(struct mfc_cmfet *cmfet)
{ return MFC_CMFET_DISABLED; }

static inline int mfc_cmfet_set_tx_id(struct mfc_cmfet *cmfet, int tx_id)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_set_bat_cap(struct mfc_cmfet *cmfet, int bat_cap)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_set_vout(struct mfc_cmfet *cmfet, int vout)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_set_high_swell(struct mfc_cmfet *cmfet, bool state)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_set_full(struct mfc_cmfet *cmfet, bool full)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_set_chg_done(struct mfc_cmfet *cmfet, bool chg_done)
{ return MFC_CMFET_DISABLED; }
static inline int mfc_cmfet_set_auth(struct mfc_cmfet *cmfet, bool auth)
{ return MFC_CMFET_DISABLED; }

static inline int mfc_cmfet_get_state(struct mfc_cmfet *cmfet, union mfc_cmfet_state *state)
{ return MFC_CMFET_DISABLED; }
#endif

#endif /* __MFC_CMFET_H */
