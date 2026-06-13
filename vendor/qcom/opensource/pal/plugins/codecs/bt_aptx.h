/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 */

#ifndef _BT_APTX_H_
#define _BT_APTX_H_

#include "bt_intf.h"

#define ENCODER_LATENCY_APTX            40
#define ENCODER_LATENCY_APTX_HD         20
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define DEFAULT_SINK_LATENCY_APTX       110
#else
#define DEFAULT_SINK_LATENCY_APTX       160
#endif
#define DEFAULT_SINK_LATENCY_APTX_HD    180
#define MODULE_ID_APTX_CLASSIC_ENC      0x0700107E
#define MODULE_ID_APTX_CLASSIC_DEC      0x0700107D
#define MODULE_ID_APTX_HD_DEC           0x0700107F
#define MODULE_ID_APTX_HD_ENC           0x07001080
#define MODULE_ID_APTX_ADAPTIVE_ENC     0x07001082
#define MODULE_ID_APTX_ADAPTIVE_SWB_DEC 0x07001083
#define MODULE_ID_APTX_ADAPTIVE_SWB_ENC 0x07001084
#define NUM_CODEC                       4

/*
 * enums which describes the APTX Adaptive
 * channel mode, these values are used by encoder
 */
typedef enum {
    APTX_AD_CHANNEL_UNCHANGED = -1,
    APTX_AD_CHANNEL_JOINT_STEREO = 0, // default
    APTX_AD_CHANNEL_MONO = 1,
    APTX_AD_CHANNEL_DUAL_MONO = 2,
    APTX_AD_CHANNEL_STEREO_TWS = 4,
    APTX_AD_CHANNEL_EARBUD = 8,
} enc_aptx_ad_channel_mode;

/*
 * enums which describes the APTX Adaptive
 * sampling frequency, these values are used
 * by encoder
 */
typedef enum {
    APTX_AD_SR_UNCHANGED = 0x0,
    APTX_AD_48 = 0x1,  // 48 KHz default
    APTX_AD_44_1 = 0x2, // 44.1kHz
    APTX_AD_96 = 0x3,  // 96KHz
} enc_aptx_ad_s_rate;

/* Information about BT APTX encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct audio_aptx_encoder_config_s {
    uint16_t sampling_rate;
    uint8_t  channels;
    uint32_t bitrate;
    uint32_t bits_per_sample;
} audio_aptx_encoder_config_t;

typedef struct audio_aptx_hd_encoder_config_s {
    uint16_t sampling_rate;
    uint8_t  channels;
    uint32_t bitrate;
    uint32_t bits_per_sample;
} audio_aptx_hd_encoder_config_t;

typedef struct audio_aptx_dual_mono_config_s {
    uint16_t sampling_rate;
    uint8_t  channels;
    uint32_t bitrate;
    uint8_t sync_mode;
} audio_aptx_dual_mono_config_t;

typedef struct audio_aptx_ad_encoder_config_s {
    uint32_t sampling_rate;
    uint32_t mtu;
    int32_t  channel_mode;
    uint32_t min_sink_modeA;
    uint32_t max_sink_modeA;
    uint32_t min_sink_modeB;
    uint32_t max_sink_modeB;
    uint32_t min_sink_modeC;
    uint32_t max_sink_modeC;
    uint32_t encoder_mode;
    uint8_t  TTP_modeA_low;
    uint8_t  TTP_modeA_high;
    uint8_t  TTP_modeB_low;
    uint8_t  TTP_modeB_high;
    uint8_t  TTP_TWS_low;
    uint8_t  TTP_TWS_high;
    uint32_t bits_per_sample;
    uint32_t input_mode;
    uint32_t fade_duration;
    uint8_t  sink_cap[11];
} audio_aptx_ad_encoder_config_t;

#endif /* _BT_APTX_H_ */
