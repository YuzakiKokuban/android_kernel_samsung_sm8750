/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <soc/soundwire.h>
#ifndef _QMP_AGGREGATOR_H_
#define _QMP_AGGREGATOR_H_

int stream_agg_add_channel(void *substream, uint32_t channels,
		uint32_t channel_rate, u8 mport_type, u8 slv_port_id, u8 dev_num,
		void *qmp_private_data, struct swr_device *sdev,
		void (*update_offset1)(void *private_data, u8 slv_port_id, u8 offset1));
int stream_agg_remove_channel(void *substream, u8 mport_type,
		u8 slv_port_id, u8 dev_num);
int stream_agg_prepare_channel(void *substream, u8 mport_type,
		u8 slv_port_id, u8 dev_num);
void update_ch_per_substream(int ch, void *substream);
#endif

