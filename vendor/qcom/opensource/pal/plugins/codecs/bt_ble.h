/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _BT_BLE_H_
#define _BT_BLE_H_

#include "bt_intf.h"
#include "bt_base.h"

#define NUM_CODEC          2
#define AUDIO_LOCATION_MAX 28
#define VERSION_IDX        7


/* Information about BT LC3 encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct lc3_cfg_s {
    uint32_t api_version;
    uint32_t sampling_freq;
    uint32_t max_octets_per_frame;
    uint32_t frame_duration;       // 7.5msec, 10msec
    uint32_t bit_depth;
    uint32_t num_blocks;
    uint8_t  default_q_level;
    uint8_t  vendor_specific[16];  // this indicates LC3/LC3Q. 0 => LC3 1.0 , 1 => LC3 1.1
    uint32_t mode;
} lc3_cfg_t;

typedef struct lc3_stream_map_s {
    uint32_t audio_location;
    uint8_t stream_id;
    uint8_t direction;
} lc3_stream_map_t;

typedef struct lc3_encoder_cfg_s {
    lc3_cfg_t toAirConfig;
    uint8_t stream_map_size;
    uint8_t actual_stream_map_size; // SS_BT_LEA - A_041 : Stereo audio for SingleDev_OneChanStereoSnk
    lc3_stream_map_t *streamMapOut;
} lc3_encoder_cfg_t;

typedef struct lc3_decoder_cfg_s {
    lc3_cfg_t fromAirConfig;
    uint32_t decoder_output_channel;
    uint8_t stream_map_size;
    lc3_stream_map_t *streamMapIn;
} lc3_decoder_cfg_t;

typedef struct audio_lc3_codec_cfg_s {
    lc3_encoder_cfg_t enc_cfg;
    lc3_decoder_cfg_t dec_cfg;
    bool is_enc_config_set;
    bool is_dec_config_set;
} audio_lc3_codec_cfg_t;

static uint32_t audio_location_map_array[] = {
    AUDIO_LOCATION_FRONT_LEFT,
    AUDIO_LOCATION_FRONT_RIGHT,
    AUDIO_LOCATION_FRONT_CENTER,
    AUDIO_LOCATION_LOW_FREQUENCY,
    AUDIO_LOCATION_BACK_LEFT,
    AUDIO_LOCATION_BACK_RIGHT,
    AUDIO_LOCATION_FRONT_LEFT_OF_CENTER,
    AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER,
    AUDIO_LOCATION_BACK_CENTER,
    AUDIO_LOCATION_LOW_FREQUENCY_2,
    AUDIO_LOCATION_SIDE_LEFT,
    AUDIO_LOCATION_SIDE_RIGHT,
    AUDIO_LOCATION_TOP_FRONT_LEFT,
    AUDIO_LOCATION_TOP_FRONT_RIGHT,
    AUDIO_LOCATION_TOP_FRONT_CENTER,
    AUDIO_LOCATION_TOP_CENTER,
    AUDIO_LOCATION_TOP_BACK_LEFT,
    AUDIO_LOCATION_TOP_BACK_RIGHT,
    AUDIO_LOCATION_TOP_SIDE_LEFT,
    AUDIO_LOCATION_TOP_SIDE_RIGHT,
    AUDIO_LOCATION_TOP_BACK_CENTER,
    AUDIO_LOCATION_BOTTOM_FRONT_CENTER,
    AUDIO_LOCATION_BOTTOM_FRONT_LEFT,
    AUDIO_LOCATION_BOTTOM_FRONT_RIGHT,
    AUDIO_LOCATION_FRONT_LEFT_WIDE,
    AUDIO_LOCATION_FRONT_RIGHT_WIDE,
    AUDIO_LOCATION_LEFT_SURROUND,
    AUDIO_LOCATION_RIGHT_SURROUND
};

static int channel_map_array[] = {
    PCM_CHANNEL_L,
    PCM_CHANNEL_R,
    PCM_CHANNEL_C,
    PCM_CHANNEL_LFE,
    PCM_CHANNEL_LB,
    PCM_CHANNEL_RB,
    PCM_CHANNEL_FLC,
    PCM_CHANNEL_FRC,
    PCM_CHANNEL_CB,
    PCM_CHANNEL_RS,
    PCM_CHANNEL_SL,
    PCM_CHANNEL_SR,
    PCM_CHANNEL_TFL,
    PCM_CHANNEL_TFR,
    PCM_CHANNEL_TFC,
    PCM_CHANNEL_TC,
    PCM_CHANNEL_TBL,
    PCM_CHANNEL_TBR,
    PCM_CHANNEL_TSL,
    PCM_CHANNEL_TSR,
    PCM_CHANNEL_TBC,
    PCM_CHANNEL_BFC,
    PCM_CHANNEL_BFL,
    PCM_CHANNEL_BFR,
    PCM_CHANNEL_LW,
    PCM_CHANNEL_RW,
    PCM_CHANNEL_LS,
    PCM_CHANNEL_RS
};

__attribute__((unused))
static uint64_t convert_channel_map(uint32_t audio_location)
{
    int i;
    uint64_t channel_mask = (uint64_t) 0x00000000;
    if (!audio_location) {
        channel_mask |= 1ULL << PCM_CHANNEL_C;
        return channel_mask;
    }

    for (i = 0; i < AUDIO_LOCATION_MAX; i++) {
        if (audio_location & audio_location_map_array[i])
            channel_mask |= 1ULL << channel_map_array[i];
    }

    return channel_mask;
}

struct codec_specific_config {
    uint32_t sampling_freq;
    uint32_t frame_duration;
    uint32_t max_octets_per_frame;
    uint32_t bit_depth;
};

#define LC3_CSC_TBL_SIZE 6
static struct codec_specific_config LC3_CSC[LC3_CSC_TBL_SIZE] = {
    {8000,  7500,  26, 24},
    {8000,  10000, 30, 24},
    {16000, 7500,  30, 24},
    {16000, 10000, 40, 24},
    {32000, 7500,  60, 24},
    {32000, 10000, 80, 24},
};

#define DEF_STREAM_MAP_SZ 2
static lc3_stream_map_t def_stream_map_out[DEF_STREAM_MAP_SZ] = {
    {2, 0, 0},
    {1, 1, 0},
};

static lc3_cfg_t def_toair_cfg = {
    33,
    16000,
    40,
    10000,
    24,
    1,
    0,
    {0},
    1,
};

#endif /* _BT_BLE_H_ */
