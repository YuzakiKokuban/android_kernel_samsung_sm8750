// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ADAPTIVE_MIPI_V2_CP_INFO_H__
#define __ADAPTIVE_MIPI_V2_CP_INFO_H__

#if IS_ENABLED(CONFIG_DEV_RIL_BRIDGE)
#include <linux/dev_ril_bridge.h>
#else
#define IPC_SYSTEM_CP_ADAPTIVE_MIPI_INFO	(0x05)

struct dev_ril_bridge_msg {
	unsigned int dev_id;
	unsigned int data_len;
	void *data;
};

static inline int register_dev_ril_bridge_event_notifier(struct notifier_block *nb) { return 0; }
static inline int unregister_dev_ril_bridge_event_notifier(struct notifier_block *nb) { return 0; }
#endif /* CONFIG_DEV_RIL_BRIDGE */

/* IPC structure from CP */
#define MAX_BAND	(16)
#define STATUS_PRIMARY_SERVING		(0x01)
#define STATUS_SECONDARY_SERVING	(0x02)

struct __packed band_info {
	u8 rat;
	u32 band;
	u32 channel;
	u8 connection_status;
	u32 bandwidth; /* khz */
	s32 sinr;
	s32 rsrp;
	s32 rsrq;
	u8 cqi;
	u8 dl_mcs;
	s32 pusch_power;
};

struct __packed cp_info {
	u32 cell_count;
	struct band_info infos[MAX_BAND];
};

/* Tunable value */
#define WEIGHT_PRIMARY_SERVING		(10)
#define WEIGHT_SECONDARY_SERVING	(1)

#define DEFAULT_WEAK_SIGNAL		(0x7FFFFFFF)
#define JUDGE_STRONG_SIGNAL		(20)
#define WEIGHT_STRONG_SIGNAL		(0)
#define WEIGHT_WEAK_SIGNAL		(1)

#endif /* __ADAPTIVE_MIPI_V2_CP_INFO_H__ */
