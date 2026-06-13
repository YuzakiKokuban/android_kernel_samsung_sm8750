/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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
#ifndef DISPLAYPORT_BIGDATA_H
#define DISPLAYPORT_BIGDATA_H
#include <linux/platform_device.h>
#include <linux/stdarg.h>

enum DP_BD_ITEM_LIST {
	ERR_AUX,
	ERR_EDID,
	ERR_HDCP_AUTH,
	ERR_LINK_TRAIN,
	ERR_INF_IRQHPD,

	BD_LINK_CONFIGURE,
	BD_ADAPTER_HWID,
	BD_ADAPTER_FWVER,
	BD_ADAPTER_TYPE,
	BD_MAX_LANE_COUNT,
	BD_MAX_LINK_RATE,
	BD_CUR_LANE_COUNT,
	BD_CUR_LINK_RATE,
	BD_HDCP_VER,
	BD_ORIENTATION,
	BD_RESOLUTION,
	BD_EDID,
	BD_ADT_VID,
	BD_ADT_PID,
	BD_DP_MODE,
	BD_SINK_NAME,
	BD_AUD_CH,
	BD_AUD_FREQ,
	BD_AUD_BIT,
	BD_ITEM_MAX,
};

void secdp_bigdata_save_item(enum DP_BD_ITEM_LIST item, ...);
void secdp_bigdata_inc_error_cnt(enum DP_BD_ITEM_LIST err);
void secdp_bigdata_clr_error_cnt(enum DP_BD_ITEM_LIST err);
void secdp_bigdata_connection(void);
void secdp_bigdata_disconnection(void);
void secdp_bigdata_init(struct class *dp_class);

ssize_t _secdp_bigdata_show(const struct class *class,
		const struct class_attribute *attr, char *buf);
ssize_t _secdp_bigdata_store(const struct class *class,
		const struct class_attribute *attr, const char *buf, size_t size);

#endif /* DISPLAYPORT_BIGDATA_H */
