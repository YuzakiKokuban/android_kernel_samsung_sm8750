/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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

#ifndef _BT_BUNDLE_H_
#define _BT_BUNDLE_H_

#include "bt_intf.h"

#define ENCODER_LATENCY_SBC             10
#define ENCODER_LATENCY_AAC             70
#define ENCODER_LATENCY_CELT            40
#define ENCODER_LATENCY_LDAC            40
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define ENCODER_LATENCY_SSC             10
#define DEFAULT_SINK_LATENCY_SBC        100
#define DEFAULT_SINK_LATENCY_SSC        160
#else
#define DEFAULT_SINK_LATENCY_SBC        140
#endif
#define DEFAULT_SINK_LATENCY_AAC        180
#define DEFAULT_SINK_LATENCY_CELT       180
#define DEFAULT_SINK_LATENCY_LDAC       180
#define MODULE_ID_AAC_ENC               0x0700101E
#define MODULE_ID_AAC_DEC               0x0700101F
#define MODULE_ID_SBC_DEC               0x07001039
#define MODULE_ID_SBC_ENC               0x0700103A
#define MODULE_ID_LDAC_ENC              0x0700107A
#define MODULE_ID_CELT_ENC              0x07001090
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define MODULE_ID_SS_SBC_ENC            0x10010BF5
#define MODULE_ID_SSC_ENC               0x10010BF3
#define NUM_CODEC                       5
#define PCM_CHANNEL_L                   1
#define PCM_CHANNEL_R                   2
#define PCM_CHANNEL_C                   3
#else
#define NUM_CODEC                       4
#endif

/* Information about BT AAC encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */

#define MAX_ABR_QUALITY_LEVELS 5
typedef struct bit_rate_level_map_s {
    uint32_t link_quality_level;
    uint32_t bitrate;
} bit_rate_level_map_t;

struct quality_level_to_bitrate_info {
    uint32_t num_levels;
    bit_rate_level_map_t bit_rate_level_map[MAX_ABR_QUALITY_LEVELS];
};

/* Structure to control frame size of AAC encoded frames. */
enum {
    MTU_SIZE = 0,
    PEAK_BIT_RATE,
    BIT_RATE_MODE,
};

struct aac_frame_size_control_t {
    /* Type of frame size control: MTU_SIZE / PEAK_BIT_RATE*/
    uint32_t ctl_type;
    /* Control value
     * MTU_SIZE: MTU size in bytes
     * PEAK_BIT_RATE: Peak bitrate in bits per second.
     * BIT_RATE_MODE: Bit rate mode such as CBR or VBR
     */
    uint32_t ctl_value;
};

struct aac_abr_control_t {
    bool is_abr_enabled;
    struct quality_level_to_bitrate_info level_to_bitrate_map;
};

typedef struct audio_aac_encoder_config_s {
    uint32_t enc_mode;       /* LC, SBR, PS */
    uint16_t format_flag;    /* RAW, ADTS */
    uint16_t channels;       /* 1-Mono, 2-Stereo */
    uint32_t sampling_rate;
    uint32_t bitrate;
    uint32_t bits_per_sample;
    struct aac_frame_size_control_t frame_ctl;
    uint8_t size_control_struct;
    struct aac_frame_size_control_t* frame_ctl_ptr;
    uint8_t abr_size_control_struct;
    struct aac_abr_control_t* abr_ctl_ptr;
} audio_aac_encoder_config_t;

/* Information about BT SBC encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct audio_sbc_encoder_config_s {
    uint32_t subband;        /* 4, 8 */
    uint32_t blk_len;        /* 4, 8, 12, 16 */
    uint16_t sampling_rate;  /* 44.1khz,48khz */
    uint8_t  channels;       /* 0(Mono),1(Dual_mono),2(Stereo),3(JS) */
    uint8_t  alloc;          /* 0(Loudness),1(SNR) */
    uint8_t  min_bitpool;    /* 2 */
    uint8_t  max_bitpool;    /* 53(44.1khz),51 (48khz) */
    uint32_t bitrate;        /* 320kbps to 512kbps */
    uint32_t bits_per_sample;
} audio_sbc_encoder_config_t;

/* Information about BT CELT encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct audio_celt_encoder_config_s {
    uint32_t sampling_rate;   /* 32000 - 48000, 48000 */
    uint16_t channels;        /* 1-Mono, 2-Stereo, 2 */
    uint16_t frame_size;      /* 64-128-256-512, 512 */
    uint16_t complexity;      /* 0-10, 1 */
    uint16_t prediction_mode; /* 0-1-2, 0 */
    uint16_t vbr_flag;        /* 0-1, 0 */
    uint32_t bitrate;         /* 32000 - 1536000, 139500 */
    uint32_t bits_per_sample;
} audio_celt_encoder_config_t;

/* Information about BT LDAC encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct audio_ldac_encoder_config_s {
    uint32_t sampling_rate;  /* 44100,48000,88200,96000 */
    uint32_t bit_rate;       /* 303000,606000,909000(in bits per second) */
    uint16_t channel_mode;   /* 0, 4, 2, 1 */
    uint16_t mtu;            /* 679 */
    uint32_t bits_per_sample;
    bool     is_abr_enabled;
    struct quality_level_to_bitrate_info level_to_bitrate_map;
} audio_ldac_encoder_config_t;

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
/* Information about BT SS SBC encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct audio_ss_sbc_encoder_config_s {
    uint32_t subband;        /* 4, 8 */
    uint32_t blk_len;        /* 4, 8, 12, 16 */
    uint16_t sampling_rate;  /* 44.1khz,48khz */
    uint8_t  channels;       /* 0(Mono),1(Dual_mono),2(Stereo),3(JS) */
    uint8_t  alloc;          /* 0(Loudness),1(SNR) */
    uint8_t  min_bitpool;    /* 2 */
    uint8_t  max_bitpool;    /* 53(44.1khz),51 (48khz) */
    uint32_t bitrate;        /* 320kbps to 512kbps */
    uint32_t bits_per_sample;
	bool is_abr_enabled;
#ifdef QCA_OFFLOAD
    struct quality_level_to_bitrate_info level_to_bitrate_map;
#endif
} audio_ss_sbc_encoder_config_t;

// [ADD_FOR_SAMSUNG - For samsung bt codec
/* Information about BT SCALABLE encoder configuration
 * This data is used between audio HAL module and
 * BT IPC library to configure DSP encoder
 */
typedef struct audio_ssc_encoder_config_s {
    uint32_t sampling_rate;
    uint8_t channels;
    uint32_t bitrate;
    uint32_t bits_per_sample;
    bool is_abr_enabled;
#ifdef QCA_OFFLOAD
    struct quality_level_to_bitrate_info level_to_bitrate_map;
#endif
} audio_ssc_encoder_config_t;

enum SS_CodecType : uint32_t {
    SSC = 0x20,
};
#endif

#endif /* _BT_PLUGIN_BUNDLE_H_ */
