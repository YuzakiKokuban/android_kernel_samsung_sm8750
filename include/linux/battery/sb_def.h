/*
 * sb_def.h
 * Samsung Mobile Battery DEF Header
 *
 * Copyright (C) 2012 Samsung Electronics, Inc.
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

#ifndef __SB_DEF_H
#define __SB_DEF_H __FILE__

enum sb_dev_type {
	SB_DEV_UNKNOWN = 0,

	SB_DEV_BATTERY,
	SB_DEV_DIRECT_CHARGER,
	SB_DEV_DUAL_BATTERY,

	SB_DEV_CHARGER,
	SB_DEV_FUEL_GAUGE,
	SB_DEV_WIRELESS_CHARGER,
	SB_DEV_LIMITTER,
	SB_DEV_DIVIDER,

	SB_DEV_MODULE,
	SB_DEV_MAX,
};

typedef unsigned long long	sb_data;

#define cast_to_sb_data(data)			((sb_data)(data))
#define cast_to_sb_pdata(data)			((sb_data *)(data))

#define cast_sb_data(type, data)		((type)(data))
#define cast_sb_data_ptr(type, data)	((type *)(data))
#define cast_sb_pdata(type, pdata)		((type)(*pdata))
#define cast_sb_pdata_ptr(type, pdata)	((type *)(*pdata))

#endif /* __SB_DEF_H */

