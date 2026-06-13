/* SPDX-License-Identifier: GPL-2.0-only */
/*
* Copyright (c) 2024, The Linux Foundation. All rights reserved.
*/
/*
*  sec_audio_debug.c
*
*  Copyright (c) 2024 Samsung Electronics
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*
*/
#include <sound/soc.h>

#define LOG_COUNT 15
#define STRING_COUNT 8
#define REGDUMP_PRINT_STRIDE 4
#define STRING_BUFFER_SIZE STRING_COUNT * LOG_COUNT
#define MAX_CHANNEL_NUM 4
#define TX_INFO_PARAM_NUM 7
#define RX_INFO_PARAM_NUM 2

enum {
	OFF_STATUS,
	NORMAL_STATUS,
	ABNORMAL_STATUS,
	NA_STATUS
};

struct _tx_debug_info {
	int num_mic;
	bool bias_enable;
	int config_status;
	bool dmic_clk_enable[MAX_CHANNEL_NUM];
	int dmic_status[MAX_CHANNEL_NUM];
};

struct _rx_debug_info {
	int num_amp;
	bool rx_playback_mute;
	bool rx_feedback_mute;
	int amp_status[MAX_CHANNEL_NUM];
};

void sec_audio_debug_codec_micbias_enable(bool enable);
void sec_audio_debug_codec_mic_clk_enable(bool enable, int dmic);
int sec_audio_debug_get_tx_component(struct snd_soc_component *component);
int sec_audio_debug_get_rx_component(struct snd_soc_component *component);
void sec_audio_debug_rx_mute_control_info(int stream, int mute);
int sec_audio_debug_rx_status_check(struct _rx_debug_info *debug_info);
int sec_audio_debug_rx_reg_print(struct _rx_debug_info *debug_info);