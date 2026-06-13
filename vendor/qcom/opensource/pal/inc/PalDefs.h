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
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** \file pal_defs.h
 *  \brief struture, enum constant defintions of the
 *         PAL(Platform Audio Layer).
 *
 *  This file contains macros, constants, or global variables
 *  exposed to the client of PAL(Platform Audio Layer).
 */

#ifndef PAL_DEFS_H
#define PAL_DEFS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// { SEC_AUDIO_COMMON
#include <system/audio.h>
#include "SecProductFeature_BLUETOOTH.h"
// } SEC_AUDIO_COMMON

#ifdef __cplusplus

#include <map>
#include <string>

extern "C" {
#endif

#define MIXER_PATH_MAX_LENGTH 100
#define PAL_MAX_CHANNELS_SUPPORTED 64
#define MAX_KEYWORD_SUPPORTED 8
#define PAL_MAX_LATENCY_MODES 8

#define PAL_VERSION "1.0"

/** Audio stream handle */
typedef uint64_t pal_stream_handle_t;

/** Sound Trigger handle */
typedef uint64_t pal_st_handle_t;

/** PAL Audio format enumeration */
typedef enum {
    PAL_AUDIO_FMT_DEFAULT_PCM = 0x1,                         /**< PCM*/
    PAL_AUDIO_FMT_PCM_S16_LE = PAL_AUDIO_FMT_DEFAULT_PCM,   /**< 16 bit little endian PCM*/
    PAL_AUDIO_FMT_DEFAULT_COMPRESSED = 0x2,                  /**< Default Compressed*/
    PAL_AUDIO_FMT_MP3 = PAL_AUDIO_FMT_DEFAULT_COMPRESSED,
    PAL_AUDIO_FMT_AAC = 0x3,
    PAL_AUDIO_FMT_AAC_ADTS = 0x4,
    PAL_AUDIO_FMT_AAC_ADIF = 0x5,
    PAL_AUDIO_FMT_AAC_LATM = 0x6,
    PAL_AUDIO_FMT_WMA_STD = 0x7,
    PAL_AUDIO_FMT_ALAC = 0x8,
    PAL_AUDIO_FMT_APE = 0x9,
    PAL_AUDIO_FMT_WMA_PRO = 0xA,
    PAL_AUDIO_FMT_FLAC = 0xB,
    PAL_AUDIO_FMT_FLAC_OGG = 0xC,
    PAL_AUDIO_FMT_VORBIS = 0xD,
    PAL_AUDIO_FMT_AMR_NB = 0xE,
    PAL_AUDIO_FMT_AMR_WB = 0xF,
    PAL_AUDIO_FMT_AMR_WB_PLUS = 0x10,
    PAL_AUDIO_FMT_EVRC = 0x11,
    PAL_AUDIO_FMT_G711 = 0x12,
    PAL_AUDIO_FMT_QCELP = 0x13,
    PAL_AUDIO_FMT_PCM_S8 = 0x14,            /**< 8 Bit PCM*/
    PAL_AUDIO_FMT_PCM_S24_3LE = 0x15,       /**<24 bit packed little endian PCM*/
    PAL_AUDIO_FMT_PCM_S24_LE = 0x16,        /**<24bit in 32bit word (LSB aligned) little endian PCM*/
    PAL_AUDIO_FMT_PCM_S32_LE = 0x17,        /**< 32bit little endian PCM*/
    PAL_AUDIO_FMT_OPUS = 0x18,
    PAL_AUDIO_FMT_NON_PCM = 0xE0000000,     /* Internal Constant used for Non PCM format identification */
    PAL_AUDIO_FMT_COMPRESSED_RANGE_BEGIN = 0xF0000000,  /* Reserved for beginning of compressed codecs */
    PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_BEGIN   = 0xF0000F00,  /* Reserved for beginning of 3rd party codecs */
    PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_END     = 0xF0000FFF,  /* Reserved for beginning of 3rd party codecs */
    PAL_AUDIO_FMT_COMPRESSED_RANGE_END   = PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_END /* Reserved for beginning of 3rd party codecs */
} pal_audio_fmt_t;

typedef enum {
    PAL_NOTIFY_START = 1,
    PAL_NOTIFY_STOP,
    PAL_NOTIFY_DEVICESWITCH
} pal_notification_t;

#define PCM_24_BIT_PACKED (0x6u)
#define PCM_32_BIT (0x3u)
#define PCM_16_BIT (0x1u)

#define SAMPLE_RATE_192000 192000

#ifdef __cplusplus
static const std::map<std::string, pal_audio_fmt_t> PalAudioFormatMap
{
    { "PCM",  PAL_AUDIO_FMT_PCM_S16_LE},
    { "PCM_S8",  PAL_AUDIO_FMT_PCM_S8},
    { "PCM_S16_LE",  PAL_AUDIO_FMT_PCM_S16_LE},
    { "PCM_S24_3LE",  PAL_AUDIO_FMT_PCM_S24_3LE},
    { "PCM_S24_LE",  PAL_AUDIO_FMT_PCM_S24_LE},
    { "PCM_S32_LE",  PAL_AUDIO_FMT_PCM_S32_LE},
    { "MP3",  PAL_AUDIO_FMT_MP3},
    { "AAC",  PAL_AUDIO_FMT_AAC},
    { "AAC_ADTS",  PAL_AUDIO_FMT_AAC_ADTS},
    { "AAC_ADIF",  PAL_AUDIO_FMT_AAC_ADIF},
    { "AAC_LATM",  PAL_AUDIO_FMT_AAC_LATM},
    { "WMA_STD",  PAL_AUDIO_FMT_WMA_STD},
    { "ALAC", PAL_AUDIO_FMT_ALAC},
    { "APE", PAL_AUDIO_FMT_APE},
    { "WMA_PRO", PAL_AUDIO_FMT_WMA_PRO},
    { "FLAC", PAL_AUDIO_FMT_FLAC},
    { "FLAC_OGG", PAL_AUDIO_FMT_FLAC_OGG},
    { "VORBIS", PAL_AUDIO_FMT_VORBIS},
    { "AMR_NB", PAL_AUDIO_FMT_AMR_NB},
    { "AMR_WB", PAL_AUDIO_FMT_AMR_WB},
    { "AMR_WB_PLUS", PAL_AUDIO_FMT_AMR_WB_PLUS},
    { "EVRC", PAL_AUDIO_FMT_EVRC},
    { "G711", PAL_AUDIO_FMT_G711},
    { "QCELP", PAL_AUDIO_FMT_QCELP},
    { "OPUS", PAL_AUDIO_FMT_OPUS}

};
#endif

struct aac_enc_cfg {
    uint16_t aac_enc_mode; /**< AAC encoder mode */
    uint16_t aac_fmt_flag; /**< AAC format flag */
};

struct pal_snd_enc_aac {
    uint32_t aac_bit_rate;
    uint32_t global_cutoff_freq;
    struct aac_enc_cfg enc_cfg;
};

struct pal_snd_dec_aac {
    uint16_t audio_obj_type;
    uint16_t pce_bits_size;
};

struct pal_snd_dec_wma {
    uint32_t fmt_tag;
    uint32_t super_block_align;
    uint32_t bits_per_sample;
    uint32_t channelmask;
    uint32_t encodeopt;
    uint32_t encodeopt1;
    uint32_t encodeopt2;
    uint32_t avg_bit_rate;
};

struct pal_snd_dec_alac {
    uint32_t frame_length;
    uint8_t compatible_version;
    uint8_t bit_depth;
    uint8_t pb;
    uint8_t mb;
    uint8_t kb;
    uint8_t num_channels;
    uint16_t max_run;
    uint32_t max_frame_bytes;
    uint32_t avg_bit_rate;
    uint32_t sample_rate;
    uint32_t channel_layout_tag;
};

struct pal_snd_dec_ape {
    uint16_t compatible_version;
    uint16_t compression_level;
    uint32_t format_flags;
    uint32_t blocks_per_frame;
    uint32_t final_frame_blocks;
    uint32_t total_frames;
    uint16_t bits_per_sample;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t seek_table_present;
};

struct pal_snd_dec_flac {
    uint16_t sample_size;
    uint16_t min_blk_size;
    uint16_t max_blk_size;
    uint16_t min_frame_size;
    uint16_t max_frame_size;
};

struct pal_snd_dec_vorbis {
    uint32_t bit_stream_fmt;
};

struct pal_snd_dec_opus {
    uint16_t bitstream_format;
    uint16_t payload_type;
    uint8_t version;
    uint8_t num_channels;
    uint16_t pre_skip;
    uint32_t sample_rate;
    uint16_t output_gain;
    uint8_t mapping_family;
    uint8_t stream_count;
    uint8_t coupled_count;
    uint8_t channel_map[8];
};

typedef struct pal_key_value_pair_s {
    uint32_t key; /**< key */
    uint32_t value; /**< value */
} pal_key_value_pair_t;

typedef struct pal_key_vector_s {
    size_t num_tkvs;  /**< number of key value pairs */
    pal_key_value_pair_t kvp[];  /**< vector of key value pairs */
} pal_key_vector_t;

typedef enum {
    PARAM_NONTKV,
    PARAM_TKV,
} pal_param_type_t;

typedef struct pal_effect_custom_payload_s {
    uint32_t paramId;
    uint32_t data[];
} pal_effect_custom_payload_t;

typedef struct effect_pal_payload_s {
    pal_param_type_t isTKV;      /* payload type: 0->non-tkv 1->tkv*/
    uint32_t tag;
    uint32_t  payloadSize;
    uint32_t  payload[]; /* TKV uses pal_key_vector_t, while nonTKV uses pal_effect_custom_payload_t */
} effect_pal_payload_t;

/* Type of Modes for Haptics Device Protection */
typedef enum {
    PAL_HAP_MODE_DYNAMIC_CAL = 1,
    PAL_HAP_MODE_FACTORY_TEST,
} pal_haptics_mode;

/* Payload For ID: PAL_PARAM_ID_HAPTICS_MODE
 * Description   : Values for haptics modes
 */
typedef struct pal_haptics_payload {
    pal_haptics_mode operationMode;/* Type of mode for which request is raised */
} pal_haptics_payload;

/* Type of Modes for Speaker Protection */
typedef enum {
    PAL_SP_MODE_DYNAMIC_CAL = 1,
    PAL_SP_MODE_FACTORY_TEST,
    PAL_SP_MODE_V_VALIDATION,
} pal_spkr_prot_mode;

/* Payload For ID: PAL_PARAM_ID_SP_MODE
 * Description   : Values for Speaker protection modes
 */
typedef struct pal_spkr_prot_payload {
    uint32_t spkrHeatupTime;         /* Wait time in mili seconds to heat up the
                                      * speaker before collecting statistics  */

    uint32_t operationModeRunTime;   /* Time period in  milli seconds when
                                      * statistics are collected */

    pal_spkr_prot_mode operationMode;/* Type of mode for which request is raised */
} pal_spkr_prot_payload;

typedef enum {
    GEF_PARAM_READ = 0,
    GEF_PARAM_WRITE,
} gef_param_rw_t;

/** Audio parameter data*/
typedef union {
    struct pal_snd_dec_aac aac_dec;
    struct pal_snd_dec_wma wma_dec;
    struct pal_snd_dec_alac alac_dec;
    struct pal_snd_dec_ape ape_dec;
    struct pal_snd_dec_flac flac_dec;
    struct pal_snd_dec_vorbis vorbis_dec;
    struct pal_snd_dec_opus opus_dec;
} pal_snd_dec_t;

/** Audio encoder parameter data*/
typedef union {
    struct pal_snd_enc_aac aac_enc;
} pal_snd_enc_t;

/** Audio parameter data*/
typedef struct pal_param_payload_s {
    uint32_t payload_size;
    uint8_t payload[];
} pal_param_payload;

typedef struct gef_payload_s {
    pal_key_vector_t *graph;
    bool persist;
    effect_pal_payload_t data;
} gef_payload_t;

typedef enum {
    LPI_VOTE,
    NLPI_VOTE,
    AVOID_VOTE,
} vote_type_t;

/** Audio channel map enumeration*/
typedef enum {
    PAL_CHMAP_CHANNEL_FL = 1,                      /**< Front right channel. */
    PAL_CHMAP_CHANNEL_FR = 2,                      /**< Front center channel. */
    PAL_CHMAP_CHANNEL_C = 3,                       /**< Left surround channel. */
    PAL_CHMAP_CHANNEL_LS = 4,                      /** Right surround channel. */
    PAL_CHMAP_CHANNEL_RS = 5,                      /** Low frequency effect channel. */
    PAL_CHMAP_CHANNEL_LFE = 6,                     /** Center surround channel; */
    PAL_CHMAP_CHANNEL_RC = 7,                      /**< rear center channel. */
    PAL_CHMAP_CHANNEL_CB = PAL_CHMAP_CHANNEL_RC,   /**< center back channel. */
    PAL_CHMAP_CHANNEL_LB = 8,                      /**< rear left channel. */
    PAL_CHMAP_CHANNEL_RB = 9,                      /**<  rear right channel. */
    PAL_CHMAP_CHANNEL_TS = 10,                     /**< Top surround channel. */
    PAL_CHMAP_CHANNEL_CVH = 11,                    /**< Center vertical height channel.*/
    PAL_CHMAP_CHANNEL_TFC = PAL_CHMAP_CHANNEL_CVH, /**< Top front center channel. */
    PAL_CHMAP_CHANNEL_MS = 12,                     /**< Mono surround channel. */
    PAL_CHMAP_CHANNEL_FLC = 13,                    /**< Front left of center channel. */
    PAL_CHMAP_CHANNEL_FRC = 14,                    /**< Front right of center channel. */
    PAL_CHMAP_CHANNEL_RLC = 15,                    /**< Rear left of center channel. */
    PAL_CHMAP_CHANNEL_RRC = 16,                    /**< Rear right of center channel. */
    PAL_CHMAP_CHANNEL_LFE2 = 17,                   /**< Secondary low frequency effect channel. */
    PAL_CHMAP_CHANNEL_SL = 18,                     /**< Side left channel. */
    PAL_CHMAP_CHANNEL_SR = 19,                     /**< Side right channel. */
    PAL_CHMAP_CHANNEL_TFL = 20,                    /**< Top front left channel. */
    PAL_CHMAP_CHANNEL_LVH = PAL_CHMAP_CHANNEL_TFL, /**< Right vertical height channel */
    PAL_CHMAP_CHANNEL_TFR = 21,                    /**< Top front right channel. */
    PAL_CHMAP_CHANNEL_RVH = PAL_CHMAP_CHANNEL_TFR, /**< Right vertical height channel. */
    PAL_CHMAP_CHANNEL_TC = 22,                     /**< Top center channel. */
    PAL_CHMAP_CHANNEL_TBL = 23,                    /**< Top back left channel. */
    PAL_CHMAP_CHANNEL_TBR = 24,                    /**< Top back right channel. */
    PAL_CHMAP_CHANNEL_TSL = 25,                    /**< Top side left channel. */
    PAL_CHMAP_CHANNEL_TSR = 26,                    /**< Top side right channel. */
    PAL_CHMAP_CHANNEL_TBC = 27,                    /**< Top back center channel. */
    PAL_CHMAP_CHANNEL_BFC = 28,                    /**< Bottom front center channel. */
    PAL_CHMAP_CHANNEL_BFL = 29,                    /**< Bottom front left channel. */
    PAL_CHMAP_CHANNEL_BFR = 30,                    /**< Bottom front right channel. */
    PAL_CHMAP_CHANNEL_LW = 31,                     /**< Left wide channel. */
    PAL_CHMAP_CHANNEL_RW = 32,                     /**< Right wide channel. */
    PAL_CHMAP_CHANNEL_LSD = 33,                    /**< Left side direct channel. */
    PAL_CHMAP_CHANNEL_RSD = 34,                    /**< Left side direct channel. */
} pal_channel_map;

/** Audio channel info data structure */
struct pal_channel_info {
    uint16_t channels;      /**< number of channels*/
    uint8_t  ch_map[PAL_MAX_CHANNELS_SUPPORTED]; /**< ch_map value per channel. */
};

/** Audio stream direction enumeration */
typedef enum {
    PAL_AUDIO_OUTPUT        = 0x1, /**< playback usecases*/
    PAL_AUDIO_INPUT         = 0x2, /**< capture/voice activation usecases*/
    PAL_AUDIO_INPUT_OUTPUT  = 0x3, /**< transcode usecases*/
} pal_stream_direction_t;

/** Audio Voip TX Effect enumeration */
typedef enum {
    PAL_AUDIO_EFFECT_NONE      = 0x0, /**< No audio effect ie., EC_OFF_NS_OFF*/
    PAL_AUDIO_EFFECT_EC        = 0x1, /**< Echo Cancellation ie., EC_ON_NS_OFF*/
    PAL_AUDIO_EFFECT_NS        = 0x2, /**< Noise Suppression ie., NS_ON_EC_OFF*/
    PAL_AUDIO_EFFECT_ECNS      = 0x3, /**< EC + NS*/
} pal_audio_effect_t;

/** Audio stream types */
typedef enum {
    PAL_STREAM_LOW_LATENCY = 1,           /**< :low latency, higher power*/
    PAL_STREAM_DEEP_BUFFER = 2,           /**< :low power, higher latency*/
    PAL_STREAM_COMPRESSED = 3,            /**< :compresssed audio*/
    PAL_STREAM_VOIP = 4,                  /**< :pcm voip audio*/
    PAL_STREAM_VOIP_RX = 5,               /**< :pcm voip audio downlink*/
    PAL_STREAM_VOIP_TX = 6,               /**< :pcm voip audio uplink*/
    PAL_STREAM_VOICE_CALL_MUSIC = 7,      /**< :incall music */
    PAL_STREAM_GENERIC = 8,               /**< :generic playback audio*/
    PAL_STREAM_RAW = 9,                   /**< pcm no post processing*/
    PAL_STREAM_VOICE_RECOGNITION = 10,     /**< voice recognition*/
    PAL_STREAM_VOICE_CALL_RECORD = 11,    /**< incall record */
    PAL_STREAM_VOICE_CALL_TX = 12,        /**< incall record, uplink */
    PAL_STREAM_VOICE_CALL_RX_TX = 13,     /**< incall record, uplink & Downlink */
    PAL_STREAM_VOICE_CALL = 14,           /**< voice call */
    PAL_STREAM_LOOPBACK = 15,             /**< loopback */
    PAL_STREAM_TRANSCODE = 16,            /**< audio transcode */
    PAL_STREAM_VOICE_UI = 17,             /**< voice ui */
    PAL_STREAM_PCM_OFFLOAD = 18,          /**< pcm offload audio */
    PAL_STREAM_ULTRA_LOW_LATENCY = 19,    /**< pcm ULL audio */
    PAL_STREAM_PROXY = 20,                /**< pcm proxy audio */
    PAL_STREAM_NON_TUNNEL = 21,           /**< NT Mode session */
    PAL_STREAM_HAPTICS = 22,              /**< Haptics Stream */
    PAL_STREAM_ACD = 23,                  /**< ACD Stream */
    PAL_STREAM_CONTEXT_PROXY = 24,        /**< Context Proxy Stream */
    PAL_STREAM_SENSOR_PCM_DATA = 25,      /**< Sensor Pcm Data Stream */
    PAL_STREAM_ULTRASOUND = 26,           /**< Ultrasound Proximity detection */
    PAL_STREAM_SPATIAL_AUDIO = 27,        /**< Spatial audio playback */
    PAL_STREAM_COMMON_PROXY = 28,         /**< AFS's WakeUp Algo library detection */
    PAL_STREAM_SENSOR_PCM_RENDERER = 29,  /**< Sensor Pcm Rendering Stream */
    PAL_STREAM_ASR = 30,                  /**< ASR Stream */
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    PAL_STREAM_HOTWORD = 31,
#endif
    PAL_STREAM_MAX,                       /**< max stream types - add new ones above */
} pal_stream_type_t;

/** Audio devices available for enabling streams */
typedef enum {
    //OUTPUT DEVICES
    PAL_DEVICE_OUT_MIN = 0,
    PAL_DEVICE_NONE = 1, /**< for transcode usecases*/
    PAL_DEVICE_OUT_HANDSET = 2,
    PAL_DEVICE_OUT_SPEAKER = 3,
    PAL_DEVICE_OUT_WIRED_HEADSET = 4,
    PAL_DEVICE_OUT_WIRED_HEADPHONE = 5, /**< Wired headphones without mic*/
    PAL_DEVICE_OUT_LINE = 6,
    PAL_DEVICE_OUT_BLUETOOTH_SCO = 7,
    PAL_DEVICE_OUT_BLUETOOTH_A2DP = 8,
    PAL_DEVICE_OUT_AUX_DIGITAL = 9,
    PAL_DEVICE_OUT_HDMI = 10,
    PAL_DEVICE_OUT_USB_DEVICE = 11,
    PAL_DEVICE_OUT_USB_HEADSET = 12,
    PAL_DEVICE_OUT_SPDIF = 13,
    PAL_DEVICE_OUT_FM = 14,
    PAL_DEVICE_OUT_AUX_LINE = 15,
    PAL_DEVICE_OUT_PROXY = 16,
    PAL_DEVICE_OUT_AUX_DIGITAL_1 = 17,
    PAL_DEVICE_OUT_HEARING_AID = 18,
    PAL_DEVICE_OUT_HAPTICS_DEVICE = 19,
    PAL_DEVICE_OUT_ULTRASOUND = 20,
    PAL_DEVICE_OUT_ULTRASOUND_DEDICATED = 21,
    PAL_DEVICE_OUT_BLUETOOTH_BLE = 22,
    PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST = 23,
    PAL_DEVICE_OUT_DUMMY = 24,
    PAL_DEVICE_OUT_RECORD_PROXY = 25,
// { SEC_AUDIO_COMMON
    PAL_DEVICE_OUT_SEC_DEVICE,
    PAL_DEVICE_OUT_EXT_MODEM = PAL_DEVICE_OUT_SEC_DEVICE, // SEC_AUDIO_CALL_SATELLITE
    // Add new sec OUT devices here, increment MAX and MIN below when you do so
    PAL_DEVICE_OUT_MAX = PAL_DEVICE_OUT_SEC_DEVICE + 1,
// } SEC_AUDIO_COMMON
/* { Qcom original code
    // Add new OUT devices here, increment MAX and MIN below when you do so
    PAL_DEVICE_OUT_MAX = 26,
   } Qcom original code */
    //INPUT DEVICES
    PAL_DEVICE_IN_MIN = PAL_DEVICE_OUT_MAX,
    PAL_DEVICE_IN_HANDSET_MIC = PAL_DEVICE_IN_MIN +1,
    PAL_DEVICE_IN_SPEAKER_MIC = PAL_DEVICE_IN_MIN + 2,
    PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET = PAL_DEVICE_IN_MIN + 3,
    PAL_DEVICE_IN_WIRED_HEADSET = PAL_DEVICE_IN_MIN + 4,
    PAL_DEVICE_IN_AUX_DIGITAL = PAL_DEVICE_IN_MIN + 5,
    PAL_DEVICE_IN_HDMI = PAL_DEVICE_IN_MIN + 6,
    PAL_DEVICE_IN_USB_ACCESSORY = PAL_DEVICE_IN_MIN + 7,
    PAL_DEVICE_IN_USB_DEVICE = PAL_DEVICE_IN_MIN + 8,
    PAL_DEVICE_IN_USB_HEADSET = PAL_DEVICE_IN_MIN + 9,
    PAL_DEVICE_IN_FM_TUNER = PAL_DEVICE_IN_MIN + 10,
    PAL_DEVICE_IN_LINE = PAL_DEVICE_IN_MIN + 11,
    PAL_DEVICE_IN_SPDIF = PAL_DEVICE_IN_MIN + 12,
    PAL_DEVICE_IN_PROXY = PAL_DEVICE_IN_MIN + 13,
    PAL_DEVICE_IN_HANDSET_VA_MIC = PAL_DEVICE_IN_MIN + 14,
    PAL_DEVICE_IN_BLUETOOTH_A2DP = PAL_DEVICE_IN_MIN + 15,
    PAL_DEVICE_IN_HEADSET_VA_MIC = PAL_DEVICE_IN_MIN + 16,
    PAL_DEVICE_IN_VI_FEEDBACK = PAL_DEVICE_IN_MIN + 17,
    PAL_DEVICE_IN_TELEPHONY_RX = PAL_DEVICE_IN_MIN + 18,
    PAL_DEVICE_IN_ULTRASOUND_MIC = PAL_DEVICE_IN_MIN +19,
    PAL_DEVICE_IN_EXT_EC_REF = PAL_DEVICE_IN_MIN + 20,
    PAL_DEVICE_IN_ECHO_REF = PAL_DEVICE_IN_MIN + 21,
    PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK = PAL_DEVICE_IN_MIN + 22,
    PAL_DEVICE_IN_BLUETOOTH_BLE = PAL_DEVICE_IN_MIN + 23,
    PAL_DEVICE_IN_CPS_FEEDBACK = PAL_DEVICE_IN_MIN + 24,
    PAL_DEVICE_IN_DUMMY = PAL_DEVICE_IN_MIN + 25,
    PAL_DEVICE_IN_CPS2_FEEDBACK = PAL_DEVICE_IN_MIN + 26,
    PAL_DEVICE_IN_RECORD_PROXY = PAL_DEVICE_IN_MIN + 27,
// { SEC_AUDIO_COMMON
    PAL_DEVICE_IN_SEC_DEVICE,
    PAL_DEVICE_IN_EXT_MODEM_MIC = PAL_DEVICE_IN_SEC_DEVICE, // SEC_AUDIO_CALL_SATELLITE
    // Add new sec OUT devices here, increment MAX and MIN below when you do so
    PAL_DEVICE_IN_MAX = PAL_DEVICE_IN_SEC_DEVICE + 1,
// } SEC_AUDIO_COMMON
/* { Qcom original code
    // Add new IN devices here, increment MAX and MIN below when you do so
    PAL_DEVICE_IN_MAX = PAL_DEVICE_IN_MIN + 28,
   } Qcom original code */
} pal_device_id_t;

typedef enum {
    VOICEMMODE1 = 0x11C05000,
    VOICEMMODE2 = 0x11DC5000,
    VOICELBMMODE1 = 0x12006000,
    VOICELBMMODE2 = 0x121C6000,
} pal_VSID_t;

typedef enum {
    PAL_STREAM_LOOPBACK_PCM,
    PAL_STREAM_LOOPBACK_HFP_RX,
    PAL_STREAM_LOOPBACK_HFP_TX,
    PAL_STREAM_LOOPBACK_COMPRESS,
    PAL_STREAM_LOOPBACK_FM,
    PAL_STREAM_LOOPBACK_KARAOKE,
    PAL_STREAM_LOOPBACK_PLAYBACK_ONLY,
    PAL_STREAM_LOOPBACK_CAPTURE_ONLY,
// { SEC_AUDIO_CALL_SATELLITE
    PAL_STREAM_LOOPBACK_EXT_MODEM_RX,
    PAL_STREAM_LOOPBACK_EXT_MODEM_TX,
// } SEC_AUDIO_CALL_SATELLITE
} pal_stream_loopback_type_t;

typedef enum {
    PAL_STREAM_PROXY_TX_VISUALIZER,
    PAL_STREAM_PROXY_TX_WFD,
    PAL_STREAM_PROXY_TX_TELEPHONY_RX,
} pal_stream_proxy_tx_type_t;

typedef enum {
    PAL_STREAM_PROXY_RX_WFD = 1,
} pal_stream_proxy_rx_type_t;

typedef enum {
    PAL_STREAM_HAPTICS_RINGTONE,
    PAL_STREAM_HAPTICS_TOUCH = 1,
    PAL_STREAM_HAPTICS_PCM = 2,
} pal_stream_haptics_type_t;

#ifdef __cplusplus
static const std::map<std::string, pal_device_id_t> deviceIdLUT {
    {std::string{ "PAL_DEVICE_OUT_MIN" },                  PAL_DEVICE_OUT_MIN},
    {std::string{ "PAL_DEVICE_NONE" },                     PAL_DEVICE_NONE},
    {std::string{ "PAL_DEVICE_OUT_HANDSET" },              PAL_DEVICE_OUT_HANDSET},
    {std::string{ "PAL_DEVICE_OUT_SPEAKER" },              PAL_DEVICE_OUT_SPEAKER},
    {std::string{ "PAL_DEVICE_OUT_WIRED_HEADSET" },        PAL_DEVICE_OUT_WIRED_HEADSET},
    {std::string{ "PAL_DEVICE_OUT_WIRED_HEADPHONE" },      PAL_DEVICE_OUT_WIRED_HEADPHONE},
    {std::string{ "PAL_DEVICE_OUT_LINE" },                 PAL_DEVICE_OUT_LINE},
    {std::string{ "PAL_DEVICE_OUT_BLUETOOTH_SCO" },        PAL_DEVICE_OUT_BLUETOOTH_SCO},
    {std::string{ "PAL_DEVICE_OUT_BLUETOOTH_A2DP" },       PAL_DEVICE_OUT_BLUETOOTH_A2DP},
    {std::string{ "PAL_DEVICE_OUT_BLUETOOTH_BLE" },        PAL_DEVICE_OUT_BLUETOOTH_BLE},
    {std::string{ "PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST" }, PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST},
    {std::string{ "PAL_DEVICE_OUT_AUX_DIGITAL" },          PAL_DEVICE_OUT_AUX_DIGITAL},
    {std::string{ "PAL_DEVICE_OUT_HDMI" },                 PAL_DEVICE_OUT_HDMI},
    {std::string{ "PAL_DEVICE_OUT_USB_DEVICE" },           PAL_DEVICE_OUT_USB_DEVICE},
    {std::string{ "PAL_DEVICE_OUT_USB_HEADSET" },          PAL_DEVICE_OUT_USB_HEADSET},
    {std::string{ "PAL_DEVICE_OUT_SPDIF" },                PAL_DEVICE_OUT_SPDIF},
    {std::string{ "PAL_DEVICE_OUT_FM" },                   PAL_DEVICE_OUT_FM},
    {std::string{ "PAL_DEVICE_OUT_AUX_LINE" },             PAL_DEVICE_OUT_AUX_LINE},
    {std::string{ "PAL_DEVICE_OUT_PROXY" },                PAL_DEVICE_OUT_PROXY},
    {std::string{ "PAL_DEVICE_OUT_RECORD_PROXY" },         PAL_DEVICE_OUT_RECORD_PROXY},
    {std::string{ "PAL_DEVICE_OUT_AUX_DIGITAL_1" },        PAL_DEVICE_OUT_AUX_DIGITAL_1},
    {std::string{ "PAL_DEVICE_OUT_HEARING_AID" },          PAL_DEVICE_OUT_HEARING_AID},
    {std::string{ "PAL_DEVICE_OUT_HAPTICS_DEVICE" },       PAL_DEVICE_OUT_HAPTICS_DEVICE},
    {std::string{ "PAL_DEVICE_OUT_ULTRASOUND" },           PAL_DEVICE_OUT_ULTRASOUND},
    {std::string{ "PAL_DEVICE_OUT_ULTRASOUND_DEDICATED" }, PAL_DEVICE_OUT_ULTRASOUND_DEDICATED},
    {std::string{ "PAL_DEVICE_OUT_DUMMY" },                PAL_DEVICE_OUT_DUMMY},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {std::string{ "PAL_DEVICE_OUT_EXT_MODEM" },            PAL_DEVICE_OUT_EXT_MODEM},
#endif
    {std::string{ "PAL_DEVICE_OUT_MAX" },                  PAL_DEVICE_OUT_MAX},
    {std::string{ "PAL_DEVICE_IN_HANDSET_MIC" },           PAL_DEVICE_IN_HANDSET_MIC},
    {std::string{ "PAL_DEVICE_IN_SPEAKER_MIC" },           PAL_DEVICE_IN_SPEAKER_MIC},
    {std::string{ "PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET" }, PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET},
    {std::string{ "PAL_DEVICE_IN_WIRED_HEADSET" },         PAL_DEVICE_IN_WIRED_HEADSET},
    {std::string{ "PAL_DEVICE_IN_AUX_DIGITAL" },           PAL_DEVICE_IN_AUX_DIGITAL},
    {std::string{ "PAL_DEVICE_IN_HDMI" },                  PAL_DEVICE_IN_HDMI},
    {std::string{ "PAL_DEVICE_IN_USB_ACCESSORY" },         PAL_DEVICE_IN_USB_ACCESSORY},
    {std::string{ "PAL_DEVICE_IN_USB_DEVICE" },            PAL_DEVICE_IN_USB_DEVICE},
    {std::string{ "PAL_DEVICE_IN_USB_HEADSET" },           PAL_DEVICE_IN_USB_HEADSET},
    {std::string{ "PAL_DEVICE_IN_FM_TUNER" },              PAL_DEVICE_IN_FM_TUNER},
    {std::string{ "PAL_DEVICE_IN_LINE" },                  PAL_DEVICE_IN_LINE},
    {std::string{ "PAL_DEVICE_IN_SPDIF" },                 PAL_DEVICE_IN_SPDIF},
    {std::string{ "PAL_DEVICE_IN_PROXY" },                 PAL_DEVICE_IN_PROXY},
    {std::string{ "PAL_DEVICE_IN_RECORD_PROXY" },          PAL_DEVICE_IN_RECORD_PROXY},
    {std::string{ "PAL_DEVICE_IN_HANDSET_VA_MIC" },        PAL_DEVICE_IN_HANDSET_VA_MIC},
    {std::string{ "PAL_DEVICE_IN_BLUETOOTH_A2DP" },        PAL_DEVICE_IN_BLUETOOTH_A2DP},
    {std::string{ "PAL_DEVICE_IN_BLUETOOTH_BLE" },         PAL_DEVICE_IN_BLUETOOTH_BLE},
    {std::string{ "PAL_DEVICE_IN_HEADSET_VA_MIC" },        PAL_DEVICE_IN_HEADSET_VA_MIC},
    {std::string{ "PAL_DEVICE_IN_VI_FEEDBACK" },           PAL_DEVICE_IN_VI_FEEDBACK},
    {std::string{ "PAL_DEVICE_IN_TELEPHONY_RX" },          PAL_DEVICE_IN_TELEPHONY_RX},
    {std::string{ "PAL_DEVICE_IN_ULTRASOUND_MIC" },        PAL_DEVICE_IN_ULTRASOUND_MIC},
    {std::string{ "PAL_DEVICE_IN_EXT_EC_REF" },            PAL_DEVICE_IN_EXT_EC_REF},
    {std::string{ "PAL_DEVICE_IN_ECHO_REF" },              PAL_DEVICE_IN_ECHO_REF},
    {std::string{ "PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK" },   PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK},
    {std::string{ "PAL_DEVICE_IN_CPS_FEEDBACK" },          PAL_DEVICE_IN_CPS_FEEDBACK},
    {std::string{ "PAL_DEVICE_IN_DUMMY" },                 PAL_DEVICE_IN_DUMMY},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {std::string{ "PAL_DEVICE_IN_EXT_MODEM_MIC" },         PAL_DEVICE_IN_EXT_MODEM_MIC},
#endif
};

//reverse mapping
static const std::map<uint32_t, std::string> deviceNameLUT {
    {PAL_DEVICE_OUT_MIN,                  std::string{"PAL_DEVICE_OUT_MIN"}},
    {PAL_DEVICE_NONE,                     std::string{"PAL_DEVICE_NONE"}},
    {PAL_DEVICE_OUT_HANDSET,              std::string{"PAL_DEVICE_OUT_HANDSET"}},
    {PAL_DEVICE_OUT_SPEAKER,              std::string{"PAL_DEVICE_OUT_SPEAKER"}},
    {PAL_DEVICE_OUT_WIRED_HEADSET,        std::string{"PAL_DEVICE_OUT_WIRED_HEADSET"}},
    {PAL_DEVICE_OUT_WIRED_HEADPHONE,      std::string{"PAL_DEVICE_OUT_WIRED_HEADPHONE"}},
    {PAL_DEVICE_OUT_LINE,                 std::string{"PAL_DEVICE_OUT_LINE"}},
    {PAL_DEVICE_OUT_BLUETOOTH_SCO,        std::string{"PAL_DEVICE_OUT_BLUETOOTH_SCO"}},
    {PAL_DEVICE_OUT_BLUETOOTH_A2DP,       std::string{"PAL_DEVICE_OUT_BLUETOOTH_A2DP"}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE,        std::string{"PAL_DEVICE_OUT_BLUETOOTH_BLE"}},
    {PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST, std::string{"PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST"}},
    {PAL_DEVICE_OUT_AUX_DIGITAL,          std::string{"PAL_DEVICE_OUT_AUX_DIGITAL"}},
    {PAL_DEVICE_OUT_HDMI,                 std::string{"PAL_DEVICE_OUT_HDMI"}},
    {PAL_DEVICE_OUT_USB_DEVICE,           std::string{"PAL_DEVICE_OUT_USB_DEVICE"}},
    {PAL_DEVICE_OUT_USB_HEADSET,          std::string{"PAL_DEVICE_OUT_USB_HEADSET"}},
    {PAL_DEVICE_OUT_SPDIF,                std::string{"PAL_DEVICE_OUT_SPDIF"}},
    {PAL_DEVICE_OUT_FM,                   std::string{"PAL_DEVICE_OUT_FM"}},
    {PAL_DEVICE_OUT_AUX_LINE,             std::string{"PAL_DEVICE_OUT_AUX_LINE"}},
    {PAL_DEVICE_OUT_PROXY,                std::string{"PAL_DEVICE_OUT_PROXY"}},
    {PAL_DEVICE_OUT_RECORD_PROXY,         std::string{"PAL_DEVICE_OUT_RECORD_PROXY"}},
    {PAL_DEVICE_OUT_AUX_DIGITAL_1,        std::string{"PAL_DEVICE_OUT_AUX_DIGITAL_1"}},
    {PAL_DEVICE_OUT_HEARING_AID,          std::string{"PAL_DEVICE_OUT_HEARING_AID"}},
    {PAL_DEVICE_OUT_HAPTICS_DEVICE,       std::string{"PAL_DEVICE_OUT_HAPTICS_DEVICE"}},
    {PAL_DEVICE_OUT_ULTRASOUND,           std::string{"PAL_DEVICE_OUT_ULTRASOUND"}},
    {PAL_DEVICE_OUT_ULTRASOUND_DEDICATED, std::string{"PAL_DEVICE_OUT_ULTRASOUND_DEDICATED"}},
    {PAL_DEVICE_OUT_DUMMY,                std::string{"PAL_DEVICE_OUT_DUMMY"}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_OUT_EXT_MODEM,            std::string{"PAL_DEVICE_OUT_EXT_MODEM"}},
#endif
    {PAL_DEVICE_OUT_MAX,                  std::string{"PAL_DEVICE_OUT_MAX"}},
    {PAL_DEVICE_IN_HANDSET_MIC,           std::string{"PAL_DEVICE_IN_HANDSET_MIC"}},
    {PAL_DEVICE_IN_SPEAKER_MIC,           std::string{"PAL_DEVICE_IN_SPEAKER_MIC"}},
    {PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET, std::string{"PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET"}},
    {PAL_DEVICE_IN_WIRED_HEADSET,         std::string{"PAL_DEVICE_IN_WIRED_HEADSET"}},
    {PAL_DEVICE_IN_AUX_DIGITAL,           std::string{"PAL_DEVICE_IN_AUX_DIGITAL"}},
    {PAL_DEVICE_IN_HDMI,                  std::string{"PAL_DEVICE_IN_HDMI"}},
    {PAL_DEVICE_IN_USB_ACCESSORY,         std::string{"PAL_DEVICE_IN_USB_ACCESSORY"}},
    {PAL_DEVICE_IN_USB_DEVICE,            std::string{"PAL_DEVICE_IN_USB_DEVICE"}},
    {PAL_DEVICE_IN_USB_HEADSET,           std::string{"PAL_DEVICE_IN_USB_HEADSET"}},
    {PAL_DEVICE_IN_FM_TUNER,              std::string{"PAL_DEVICE_IN_FM_TUNER"}},
    {PAL_DEVICE_IN_LINE,                  std::string{"PAL_DEVICE_IN_LINE"}},
    {PAL_DEVICE_IN_SPDIF,                 std::string{"PAL_DEVICE_IN_SPDIF"}},
    {PAL_DEVICE_IN_PROXY,                 std::string{"PAL_DEVICE_IN_PROXY"}},
    {PAL_DEVICE_IN_RECORD_PROXY,          std::string{"PAL_DEVICE_IN_RECORD_PROXY"}},
    {PAL_DEVICE_IN_HANDSET_VA_MIC,        std::string{"PAL_DEVICE_IN_HANDSET_VA_MIC"}},
    {PAL_DEVICE_IN_BLUETOOTH_A2DP,        std::string{"PAL_DEVICE_IN_BLUETOOTH_A2DP"}},
    {PAL_DEVICE_IN_BLUETOOTH_BLE,         std::string{"PAL_DEVICE_IN_BLUETOOTH_BLE"}},
    {PAL_DEVICE_IN_HEADSET_VA_MIC,        std::string{"PAL_DEVICE_IN_HEADSET_VA_MIC"}},
    {PAL_DEVICE_IN_VI_FEEDBACK,           std::string{"PAL_DEVICE_IN_VI_FEEDBACK"}},
    {PAL_DEVICE_IN_TELEPHONY_RX,          std::string{"PAL_DEVICE_IN_TELEPHONY_RX"}},
    {PAL_DEVICE_IN_ULTRASOUND_MIC,        std::string{"PAL_DEVICE_IN_ULTRASOUND_MIC"}},
    {PAL_DEVICE_IN_EXT_EC_REF,            std::string{"PAL_DEVICE_IN_EXT_EC_REF"}},
    {PAL_DEVICE_IN_ECHO_REF,              std::string{"PAL_DEVICE_IN_ECHO_REF"}},
    {PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK,   std::string{"PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK"}},
    {PAL_DEVICE_IN_CPS_FEEDBACK,          std::string{"PAL_DEVICE_IN_CPS_FEEDBACK"}},
    {PAL_DEVICE_IN_DUMMY,                 std::string{"PAL_DEVICE_IN_DUMMY"}},
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_DEVICE_IN_EXT_MODEM_MIC,         std::string{"PAL_DEVICE_IN_EXT_MODEM_MIC"}},
#endif
};

const std::map<std::string, uint32_t> usecaseIdLUT {
    {std::string{ "PAL_STREAM_LOW_LATENCY" },              PAL_STREAM_LOW_LATENCY},
    {std::string{ "PAL_STREAM_DEEP_BUFFER" },              PAL_STREAM_DEEP_BUFFER},
    {std::string{ "PAL_STREAM_COMPRESSED" },               PAL_STREAM_COMPRESSED},
    {std::string{ "PAL_STREAM_VOIP" },                     PAL_STREAM_VOIP},
    {std::string{ "PAL_STREAM_VOIP_RX" },                  PAL_STREAM_VOIP_RX},
    {std::string{ "PAL_STREAM_VOIP_TX" },                  PAL_STREAM_VOIP_TX},
    {std::string{ "PAL_STREAM_VOICE_CALL_MUSIC" },         PAL_STREAM_VOICE_CALL_MUSIC},
    {std::string{ "PAL_STREAM_GENERIC" },                  PAL_STREAM_GENERIC},
    {std::string{ "PAL_STREAM_RAW" },                      PAL_STREAM_RAW},
    {std::string{ "PAL_STREAM_VOICE_RECOGNITION" },        PAL_STREAM_VOICE_RECOGNITION},
    {std::string{ "PAL_STREAM_VOICE_CALL_RECORD" },        PAL_STREAM_VOICE_CALL_RECORD},
    {std::string{ "PAL_STREAM_VOICE_CALL_TX" },            PAL_STREAM_VOICE_CALL_TX},
    {std::string{ "PAL_STREAM_VOICE_CALL_RX_TX" },         PAL_STREAM_VOICE_CALL_RX_TX},
    {std::string{ "PAL_STREAM_VOICE_CALL" },               PAL_STREAM_VOICE_CALL},
    {std::string{ "PAL_STREAM_LOOPBACK" },                 PAL_STREAM_LOOPBACK},
    {std::string{ "PAL_STREAM_TRANSCODE" },                PAL_STREAM_TRANSCODE},
    {std::string{ "PAL_STREAM_VOICE_UI" },                 PAL_STREAM_VOICE_UI},
    {std::string{ "PAL_STREAM_PCM_OFFLOAD" },              PAL_STREAM_PCM_OFFLOAD},
    {std::string{ "PAL_STREAM_ULTRA_LOW_LATENCY" },        PAL_STREAM_ULTRA_LOW_LATENCY},
    {std::string{ "PAL_STREAM_PROXY" },                    PAL_STREAM_PROXY},
    {std::string{ "PAL_STREAM_NON_TUNNEL" },               PAL_STREAM_NON_TUNNEL},
    {std::string{ "PAL_STREAM_HAPTICS" },                  PAL_STREAM_HAPTICS},
    {std::string{ "PAL_STREAM_ACD" },                      PAL_STREAM_ACD},
    {std::string{ "PAL_STREAM_ASR" },                      PAL_STREAM_ASR},
    {std::string{ "PAL_STREAM_ULTRASOUND" },               PAL_STREAM_ULTRASOUND},
    {std::string{ "PAL_STREAM_SENSOR_PCM_DATA" },          PAL_STREAM_SENSOR_PCM_DATA},
    {std::string{ "PAL_STREAM_SPATIAL_AUDIO" },            PAL_STREAM_SPATIAL_AUDIO},
    {std::string{ "PAL_STREAM_CONTEXT_PROXY" },            PAL_STREAM_CONTEXT_PROXY},
    {std::string{ "PAL_STREAM_COMMON_PROXY" },             PAL_STREAM_COMMON_PROXY},
    {std::string{ "PAL_STREAM_SENSOR_PCM_RENDERER" },      PAL_STREAM_SENSOR_PCM_RENDERER},
};

/* Update the reverse mapping as well when new stream is added */
const std::map<uint32_t, std::string> streamNameLUT {
    {PAL_STREAM_LOW_LATENCY,        std::string{ "PAL_STREAM_LOW_LATENCY" } },
    {PAL_STREAM_DEEP_BUFFER,        std::string{ "PAL_STREAM_DEEP_BUFFER" } },
    {PAL_STREAM_COMPRESSED,         std::string{ "PAL_STREAM_COMPRESSED" } },
    {PAL_STREAM_VOIP,               std::string{ "PAL_STREAM_VOIP" } },
    {PAL_STREAM_VOIP_RX,            std::string{ "PAL_STREAM_VOIP_RX" } },
    {PAL_STREAM_VOIP_TX,            std::string{ "PAL_STREAM_VOIP_TX" } },
    {PAL_STREAM_VOICE_CALL_MUSIC,   std::string{ "PAL_STREAM_VOICE_CALL_MUSIC" } },
    {PAL_STREAM_GENERIC,            std::string{ "PAL_STREAM_GENERIC" } },
    {PAL_STREAM_RAW,                std::string{ "PAL_STREAM_RAW" } },
    {PAL_STREAM_VOICE_RECOGNITION,  std::string{ "PAL_STREAM_VOICE_RECOGNITION" } },
    {PAL_STREAM_VOICE_CALL_RECORD,  std::string{ "PAL_STREAM_VOICE_CALL_RECORD" } },
    {PAL_STREAM_VOICE_CALL_TX,      std::string{ "PAL_STREAM_VOICE_CALL_TX" } },
    {PAL_STREAM_VOICE_CALL_RX_TX,   std::string{ "PAL_STREAM_VOICE_CALL_RX_TX" } },
    {PAL_STREAM_VOICE_CALL,         std::string{ "PAL_STREAM_VOICE_CALL" } },
    {PAL_STREAM_LOOPBACK,           std::string{ "PAL_STREAM_LOOPBACK" } },
    {PAL_STREAM_TRANSCODE,          std::string{ "PAL_STREAM_TRANSCODE" } },
    {PAL_STREAM_VOICE_UI,           std::string{ "PAL_STREAM_VOICE_UI" } },
    {PAL_STREAM_PCM_OFFLOAD,        std::string{ "PAL_STREAM_PCM_OFFLOAD" } },
    {PAL_STREAM_ULTRA_LOW_LATENCY,  std::string{ "PAL_STREAM_ULTRA_LOW_LATENCY" } },
    {PAL_STREAM_PROXY,              std::string{ "PAL_STREAM_PROXY" } },
    {PAL_STREAM_NON_TUNNEL,         std::string{ "PAL_STREAM_NON_TUNNEL" } },
    {PAL_STREAM_HAPTICS,            std::string{ "PAL_STREAM_HAPTICS" } },
    {PAL_STREAM_CONTEXT_PROXY,      std::string{ "PAL_STREAM_CONTEXT_PROXY" } },
    {PAL_STREAM_ACD,                std::string{ "PAL_STREAM_ACD" } },
    {PAL_STREAM_ASR,                std::string{ "PAL_STREAM_ASR" } },
    {PAL_STREAM_ULTRASOUND,         std::string{ "PAL_STREAM_ULTRASOUND" } },
    {PAL_STREAM_SENSOR_PCM_DATA,    std::string{ "PAL_STREAM_SENSOR_PCM_DATA" } },
    {PAL_STREAM_SPATIAL_AUDIO,      std::string{ "PAL_STREAM_SPATIAL_AUDIO" } },
    {PAL_STREAM_COMMON_PROXY,       std::string{ "PAL_STREAM_COMMON_PROXY" } },
    {PAL_STREAM_SENSOR_PCM_RENDERER,std::string{ "PAL_STREAM_SENSOR_PCM_RENDERER" } },
};

const std::map<uint32_t, std::string> vsidLUT {
    {VOICEMMODE1,    std::string{ "VOICEMMODE1" } },
    {VOICEMMODE2,    std::string{ "VOICEMMODE2" } },
    {VOICELBMMODE1,  std::string{ "VOICELBMMODE1" } },
    {VOICELBMMODE2,  std::string{ "VOICELBMMODE2" } },
};

const std::map<uint32_t, std::string> loopbackLUT {
    {PAL_STREAM_LOOPBACK_PCM,           std::string{ "PAL_STREAM_LOOPBACK_PCM" } },
    {PAL_STREAM_LOOPBACK_HFP_RX,        std::string{ "PAL_STREAM_LOOPBACK_HFP_RX" } },
    {PAL_STREAM_LOOPBACK_HFP_TX,        std::string{ "PAL_STREAM_LOOPBACK_HFP_TX" } },
    {PAL_STREAM_LOOPBACK_COMPRESS,      std::string{ "PAL_STREAM_LOOPBACK_COMPRESS" } },
    {PAL_STREAM_LOOPBACK_FM,            std::string{ "PAL_STREAM_LOOPBACK_FM" } },
    {PAL_STREAM_LOOPBACK_KARAOKE,       std::string{ "PAL_STREAM_LOOPBACK_KARAOKE" }},
    {PAL_STREAM_LOOPBACK_PLAYBACK_ONLY, std::string{ "PAL_STREAM_LOOPBACK_PLAYBACK_ONLY" } },
    {PAL_STREAM_LOOPBACK_CAPTURE_ONLY,  std::string{ "PAL_STREAM_LOOPBACK_CAPTURE_ONLY" } },
#ifdef SEC_AUDIO_CALL_SATELLITE
    {PAL_STREAM_LOOPBACK_EXT_MODEM_RX,  std::string{ "PAL_STREAM_LOOPBACK_EXT_MODEM_RX" } },
    {PAL_STREAM_LOOPBACK_EXT_MODEM_TX,  std::string{ "PAL_STREAM_LOOPBACK_EXT_MODEM_TX" } },
#endif
};

const std::map<uint32_t, std::string> hapticsLUT {
    {PAL_STREAM_HAPTICS_TOUCH,        std::string{ "PAL_STREAM_HAPTICS_TOUCH" } },
    {PAL_STREAM_HAPTICS_RINGTONE,     std::string{ "PAL_STREAM_HAPTICS_RINGTONE" } },
};

#endif


/* type of asynchronous write callback events. Mutually exclusive */
typedef enum {
    PAL_STREAM_CBK_EVENT_WRITE_READY, /* non blocking write completed */
    PAL_STREAM_CBK_EVENT_DRAIN_READY,  /* drain completed */
    PAL_STREAM_CBK_EVENT_PARTIAL_DRAIN_READY, /* partial drain completed */
    PAL_STREAM_CBK_EVENT_READ_DONE, /* non blocking read completed */
    PAL_STREAM_CBK_EVENT_ERROR, /* stream hit some error, let AF take action */
    PAL_STREAM_CBK_MAX = 0xFFFF,
} pal_stream_callback_event_t;

/* type of global callback events. */
typedef enum {
    PAL_SND_CARD_STATE,
} pal_global_callback_event_t;

struct pal_stream_info {
    int64_t version;                    /** version of structure*/
    int64_t size;                       /** size of structure*/
    int64_t duration_us;                /** duration in microseconds, -1 if unknown */
    bool has_video;                     /** optional, true if stream is tied to a video stream */
    bool is_streaming;                  /** true if streaming, false if local playback */
    int32_t loopback_type;              /** used only if stream_type is LOOPBACK. One of the */
                                        /** enums defined in enum pal_stream_loopback_type */
    int32_t tx_proxy_type;   /** enums defined in enum pal_stream_proxy_tx_types */
    int32_t rx_proxy_type;   /** enums defined in enum pal_stream_proxy_rx_types */
    int32_t haptics_type;    /** enums defined in enum pal_sream_haptics_types */
    //pal_audio_attributes_t usage;       /** Not sure if we make use of this */
};

typedef enum {
    INCALL_RECORD_VOICE_UPLINK = 1,
    INCALL_RECORD_VOICE_DOWNLINK,
    INCALL_RECORD_VOICE_UPLINK_DOWNLINK,
} pal_incall_record_direction;

struct pal_voice_record_info {
    int64_t version;                    /** version of structure*/
    int64_t size;                       /** size of structure*/
    pal_incall_record_direction record_direction;         /** use direction enum to indicate content to be record */
};

struct pal_voice_call_info {
     uint32_t VSID;
     uint32_t tty_mode;
};

typedef enum {
    PAL_TTY_OFF = 0,
    PAL_TTY_HCO = 1,
    PAL_TTY_VCO = 2,
    PAL_TTY_FULL = 3,
} pal_tty_t;

struct pal_incall_music_info {
    bool local_playback;
};

#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA
struct pal_lowlatency_media {
    bool mode;
};
#endif

typedef union {
    struct pal_stream_info opt_stream_info; /* optional */
    struct pal_voice_record_info voice_rec_info; /* mandatory */
    struct pal_voice_call_info voice_call_info; /* manatory for voice call*/
    struct pal_incall_music_info incall_music_info;
#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA
    struct pal_lowlatency_media lowlatency_media;
#endif
} pal_stream_info_t;

/** Media configuraiton */
struct pal_media_config {
    uint32_t sample_rate;                /**< sample rate */
    uint32_t bit_width;                  /**< bit width */
    pal_audio_fmt_t aud_fmt_id;          /**< audio format id*/
    struct pal_channel_info ch_info;    /**< channel info */
};

/** Android Media configuraiton  */
/* dynamic media config for plugin devices, +1 so that the last entry is always 0 */
#define MAX_SUPPORTED_CHANNEL_MASKS (2 * 8)
#define MAX_SUPPORTED_FORMATS 15
#define MAX_SUPPORTED_SAMPLE_RATES 7
typedef struct dynamic_media_config {
    uint32_t sample_rate[MAX_SUPPORTED_SAMPLE_RATES+1];   /**< sample rate */
    uint32_t format[MAX_SUPPORTED_FORMATS+1];             /**< format */
    uint32_t mask[MAX_SUPPORTED_CHANNEL_MASKS + 1];       /**< channel mask */
    bool jack_status;                                     /**< input/output jack status*/
} dynamic_media_config_t;

/**  Available stream flags of an audio session*/
typedef enum {
    PAL_STREAM_FLAG_TIMESTAMP       = 0x1,  /**< Enable time stamps associated to audio buffers  */
    PAL_STREAM_FLAG_NON_BLOCKING    = 0x2,  /**< Stream IO operations are non blocking */
    PAL_STREAM_FLAG_MMAP            = 0x4,  /**< Stream Mode should be in MMAP*/
    PAL_STREAM_FLAG_MMAP_NO_IRQ     = 0x8,  /**< Stream Mode should be No IRQ */
    PAL_STREAM_FLAG_EXTERN_MEM      = 0x10, /**< Shared memory buffers allocated by client*/
    PAL_STREAM_FLAG_SRCM_INBAND     = 0x20, /**< MediaFormat change event inband with data buffers*/
    PAL_STREAM_FLAG_EOF             = 0x40, /**< MediaFormat change event inband with data buffers*/
} pal_stream_flags_t;

#define PAL_STREAM_FLAG_NON_BLOCKING_MASK 0x2
#define PAL_STREAM_FLAG_MMAP_MASK 0x4
#define PAL_STREAM_FLAG_MMAP_NO_IRQ_MASK 0x8

/**< PAL stream attributes to be specified, used in pal_stream_open cmd */
struct pal_stream_attributes {
    pal_stream_type_t type;                      /**<  stream type */
    pal_stream_info_t info;                      /**<  stream info */
    pal_stream_flags_t flags;                    /**<  stream flags */
    pal_stream_direction_t direction;            /**<  direction of the streams */
    struct pal_media_config in_media_config;     /**<  media config of the input audio samples */
    struct pal_media_config out_media_config;    /**<  media config of the output audio samples */
};

typedef struct pal_callback_config {
    int32_t noOfPrevDevices, noOfCurrentDevices;
    pal_device_id_t *prevDevices, *currentDevices;
    struct pal_stream_attributes streamAttributes;
} pal_callback_config_t;

/**< Key value pair to identify the topology of a usecase from default  */
struct modifier_kv  {
    uint32_t key;
    uint32_t value;
};

/** Metadata flags */
enum {
    PAL_META_DATA_FLAGS_NONE = 0,
};

/** metadata flags, can be OR'able */
typedef uint32_t pal_meta_data_flags_t;

typedef struct pal_extern_alloc_buff_info {
    int      alloc_handle;/**< unique memory handle identifying extern mem allocation */
    uint32_t alloc_size;  /**< size of external allocation */
    uint32_t offset;      /**< offset of buffer within extern allocation */
} pal_extern_alloc_buff_info_t;

/** PAL buffer structure used for reading/writing buffers from/to the stream */
struct pal_buffer {
    uint8_t *buffer;               /**<  buffer pointer */
    size_t size;                   /**< number of bytes */
    size_t offset;                 /**< offset in buffer from where valid byte starts */
    struct timespec *ts;           /**< timestmap */
    uint32_t flags;                /**< flags */
    size_t metadata_size;          /**< size of metadata buffer in bytes */
    uint8_t *metadata;             /**< metadata buffer. Can contain multiple metadata*/
    pal_extern_alloc_buff_info_t alloc_info; /**< holds info for extern buff */
    uint64_t frame_index;          /**< frame index of the buffer */
};

struct pal_clbk_buffer_info {
    uint64_t frame_index;       /**< frame index of the buffer */
    uint32_t sample_rate;       /**< updated sample rate */
    uint32_t bit_width;         /**< updated bit width */
    uint16_t channel_count;     /**< updated channel count */
};

struct pal_callback_buffer {
    uint8_t *buffer;               /**<  buffer pointer */
    size_t size;                   /**< filled length of the buffer */
    struct timespec *ts;           /**< timestamp */
    uint32_t status;               /**< status of callback payload */
    struct pal_clbk_buffer_info cb_buf_info;   /**< callback buffer info */
};

/** pal_mmap_buffer flags */
enum {
    PAL_MMMAP_BUFF_FLAGS_NONE = 0,
    /**
    * Only set this flag if applications can access the audio buffer memory
    * shared with the backend (usually DSP) _without_ security issue.
    *
    * Setting this flag also implies that Binder will allow passing the shared memory FD
    * to applications.
    *
    * That usually implies that the kernel will prevent any access to the
    * memory surrounding the audio buffer as it could lead to a security breach.
    *
    * For example, a "/dev/snd/" file descriptor generally is not shareable,
    * but an "anon_inode:dmabuffer" file descriptor is shareable.
    * See also Linux kernel's dma_buf.
    *
    */
    PAL_MMMAP_BUFF_FLAGS_APP_SHAREABLE = 1,
};

/** pal_mmap_buffer flags, can be OR'able */
typedef uint32_t pal_mmap_buffer_flags_t;

/** PAL buffer structure used for reading/writing buffers from/to the stream */
struct pal_mmap_buffer {
    void*    buffer;                /**< base address of mmap memory buffer,
                                         for use by local proces only */
    int32_t  fd;                    /**< fd for mmap memory buffer */
    uint32_t buffer_size_frames;    /**< total buffer size in frames */
    uint32_t burst_size_frames;     /**< transfer size granularity in frames */
    pal_mmap_buffer_flags_t flags;  /**< Attributes describing the buffer. */
};

 /**
 * Mmap buffer read/write position returned by GetMmapPosition.
 * note\ Used by streams opened in mmap mode.
 */
struct pal_mmap_position {
    int64_t  time_nanoseconds; /**< timestamp in ns, CLOCK_MONOTONIC */
    int32_t  position_frames;  /**< increasing 32 bit frame count reset when stop()
                                    is called */
};

/** channel mask and volume pair */
struct pal_channel_vol_kv {
    uint32_t channel_mask;       /**< channel mask */
    float vol;                   /**< gain of the channel mask */
};

/** Volume data strucutre defintion used as argument for volume command */
struct pal_volume_data {
    uint32_t no_of_volpair;                       /**< no of volume pairs*/
    struct pal_channel_vol_kv volume_pair[];     /**< channel mask and volume pair */
};

/** Gain data strucutre defintion used as argument for gain command */
struct pal_gain_data {
    uint16_t gain;                       /** Gain value in q13 format **/
    uint16_t reserved;
};

struct pal_time_us {
    uint32_t value_lsw;   /** Lower 32 bits of 64 bit time value in microseconds */
    uint32_t value_msw;   /** Upper 32 bits of 64 bit time value in microseconds */
};

/** Timestamp strucutre defintion used as argument for
 *  gettimestamp api */
struct pal_session_time {
    struct pal_time_us session_time;   /** Value of the current session time in microseconds */
    struct pal_time_us absolute_time;  /** Value of the absolute time in microseconds */
    struct pal_time_us timestamp;      /** Value of the last processed time stamp in microseconds */
};

/** EVENT configurations data strucutre defintion used as
 *  argument for mute command */
//typedef union {
//} pal_event_cfg_t;

/** event id of the event generated*/
typedef uint32_t pal_event_id;

typedef enum {
    /** request notification when all accumlated data has be
     *  drained.*/
    PAL_DRAIN,
    /** request notification when drain completes shortly before all
     *  accumlated data of the current track has been played out */
    PAL_DRAIN_PARTIAL,
} pal_drain_type_t;

typedef enum {
    PAL_PARAM_ID_LOAD_SOUND_MODEL = 0,
    PAL_PARAM_ID_RECOGNITION_CONFIG = 1,
    PAL_PARAM_ID_ECNS_ON_OFF = 2,
    PAL_PARAM_ID_DIRECTION_OF_ARRIVAL = 3,
    PAL_PARAM_ID_UIEFFECT = 4,
    PAL_PARAM_ID_STOP_BUFFERING = 5,
    PAL_PARAM_ID_CODEC_CONFIGURATION = 6,
    /* Non-Stream Specific Parameters*/
    PAL_PARAM_ID_DEVICE_CONNECTION = 7,
    PAL_PARAM_ID_SCREEN_STATE = 8,
    PAL_PARAM_ID_CHARGING_STATE = 9,
    PAL_PARAM_ID_DEVICE_ROTATION = 10,
    PAL_PARAM_ID_BT_SCO = 11,
    PAL_PARAM_ID_BT_SCO_WB = 12,
    PAL_PARAM_ID_BT_SCO_SWB = 13,
    PAL_PARAM_ID_BT_A2DP_RECONFIG = 14,
    PAL_PARAM_ID_BT_A2DP_RECONFIG_SUPPORTED = 15,
    PAL_PARAM_ID_BT_A2DP_SUSPENDED = 16,
    PAL_PARAM_ID_BT_A2DP_TWS_CONFIG = 17,
    PAL_PARAM_ID_BT_A2DP_ENCODER_LATENCY = 18,
    PAL_PARAM_ID_DEVICE_CAPABILITY = 19,
    PAL_PARAM_ID_GET_SOUND_TRIGGER_PROPERTIES = 20,
    PAL_PARAM_ID_TTY_MODE = 21,
    PAL_PARAM_ID_VOLUME_BOOST = 22,
    PAL_PARAM_ID_SLOW_TALK = 23,
    PAL_PARAM_ID_SPEAKER_RAS = 24,
    PAL_PARAM_ID_SP_MODE = 25,
    PAL_PARAM_ID_GAIN_LVL_MAP = 26,
    PAL_PARAM_ID_GAIN_LVL_CAL = 27,
    PAL_PARAM_ID_GAPLESS_MDATA = 28,
    PAL_PARAM_ID_HD_VOICE = 29,
    PAL_PARAM_ID_WAKEUP_ENGINE_CONFIG = 30,
    PAL_PARAM_ID_WAKEUP_BUFFERING_CONFIG = 31,
    PAL_PARAM_ID_WAKEUP_ENGINE_RESET = 32,
    PAL_PARAM_ID_WAKEUP_MODULE_VERSION = 33,
    PAL_PARAM_ID_WAKEUP_CUSTOM_CONFIG = 34,
    PAL_PARAM_ID_UNLOAD_SOUND_MODEL = 35,
    PAL_PARAM_ID_MODULE_CONFIG = 36, /*Clients directly configure DSP modules*/
    PAL_PARAM_ID_BT_A2DP_LC3_CONFIG = 37,
    PAL_PARAM_ID_PROXY_CHANNEL_CONFIG = 38,
    PAL_PARAM_ID_CONTEXT_LIST = 39,
    PAL_PARAM_ID_HAPTICS_INTENSITY = 40,
    PAL_PARAM_ID_HAPTICS_VOLUME = 41,
    PAL_PARAM_ID_BT_A2DP_DECODER_LATENCY = 42,
    PAL_PARAM_ID_CUSTOM_CONFIGURATION = 43,
    PAL_PARAM_ID_KW_TRANSFER_LATENCY = 44,
    PAL_PARAM_ID_BT_A2DP_FORCE_SWITCH = 45,
    PAL_PARAM_ID_BT_SCO_LC3 = 46,
    PAL_PARAM_ID_DEVICE_MUTE = 47,
    PAL_PARAM_ID_UPD_REGISTER_FOR_EVENTS = 48,
    PAL_PARAM_ID_SP_GET_CAL = 49,
    PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED = 50,
    PAL_PARAM_ID_SNDCARD_STATE = 51,
    PAL_PARAM_ID_HIFI_PCM_FILTER = 52,
    PAL_PARAM_ID_CHARGER_STATE = 53,
    PAL_PARAM_ID_BT_SCO_NREC = 54,
    PAL_PARAM_ID_VOLUME_USING_SET_PARAM = 55,
    PAL_PARAM_ID_UHQA_FLAG = 56,
    PAL_PARAM_ID_STREAM_ATTRIBUTES = 57,
    PAL_PARAM_ID_SET_UPD_DUTY_CYCLE = 58,
    PAL_PARAM_ID_MSPP_LINEAR_GAIN = 59,
    PAL_PARAM_ID_SET_SOURCE_METADATA = 60,
    PAL_PARAM_ID_SET_SINK_METADATA = 61,
    PAL_PARAM_ID_ULTRASOUND_RAMPDOWN = 62,
    PAL_PARAM_ID_VOLUME_CTRL_RAMP = 63,
    PAL_PARAM_ID_SVA_WAKEUP_MODULE_VERSION = 64,
    PAL_PARAM_ID_GAIN_USING_SET_PARAM = 65,
    PAL_PARAM_ID_HAPTICS_CNFG = 66,
    PAL_PARAM_ID_WAKEUP_ENGINE_PER_MODEL_RESET = 67,
    PAL_PARAM_ID_RECONFIG_ENCODER = 68,
    PAL_PARAM_ID_VUI_SET_META_DATA = 69,
    PAL_PARAM_ID_VUI_GET_META_DATA = 70,
    PAL_PARAM_ID_VUI_CAPTURE_META_DATA = 71,
    PAL_PARAM_ID_TIMESTRETCH_PARAMS = 72,
    PAL_PARAM_ID_LATENCY_MODE = 73,
    PAL_PARAM_ID_ST_CAPTURE_INFO = 74,
    PAL_PARAM_ID_RESOURCES_AVAILABLE = 75,
    PAL_PARAM_ID_PROXY_RECORD_SESSION = 76,
    PAL_PARAM_ID_ASR_MODEL = 77,
    PAL_PARAM_ID_ASR_CONFIG = 78,
    PAL_PARAM_ID_ASR_CUSTOM = 79,
    PAL_PARAM_ID_ASR_FORCE_OUTPUT = 80,
    PAL_PARAM_ID_ASR_OUTPUT = 81,
    PAL_PARAM_ID_ASR_SET_PARAM = 82,
    PAL_PARAM_ID_HAPTICS_MODE = 83,

#ifdef SEC_AUDIO_LOOPBACK_TEST
    PAL_PARAM_ID_LOOPBACK_MODE = 1001,
#endif
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
    PAL_PARAM_ID_SPEAKER_STATUS = 1002,
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_FMRADIO
    PAL_PARAM_ID_FMRADIO_USB_GAIN = 1004,
#endif
#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK
    PAL_PARAM_ID_HAPTIC_SOURCE = 1005,
#endif
#ifdef SEC_AUDIO_CALL_VOIP
    PAL_PARAM_ID_MIC_MODE = 1006,
    PAL_PARAM_ID_VOIP_MIC_MODE = 1007,
#endif
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    PAL_PARAM_ID_BT_A2DP_DYN_BITRATE = 2001,
    PAL_PARAM_ID_BT_A2DP_SBM_CONFIG = 2002,
    PAL_PARAM_ID_BT_A2DP_DELAY_REPORT = 2003,
#endif
#ifdef SEC_AUDIO_BLE_OFFLOAD
    PAL_PARAM_ID_BT_A2DP_SUSPENDED_FOR_BLE = 2004,
#endif
#ifdef SEC_AUDIO_BLUETOOTH
    PAL_PARAM_ID_BT_SCO_CODEC_TYPE = 2005,
#endif
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
    PAL_PARAM_ID_SEPAKER_AMP_RUN_CAL = 3000,
#endif
#ifdef SEC_AUDIO_AMP_SDHMS
    PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV = 3001,
    PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK = 3002,
#endif
#ifdef SEC_AUDIO_AMP_BIGDATA
    PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET = 3003,
    PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE = 3004,
    PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_START = 3005,
    PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_STOP = 3006,
#endif
#ifdef SEC_AUDIO_BARGEIN_MODE
    PAL_PARAM_ID_BARGEIN_MODE = 3007,
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    PAL_PARAM_ID_INTERPRETER_MODE = 3008,
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
    PAL_PARAM_ID_BTMIX_RECORD = 3009,
#endif
} pal_param_id_type_t;

/** HDMI/DP */
// START: MST ==================================================
#define MAX_CONTROLLERS 1
#define MAX_STREAMS_PER_CONTROLLER 2
// END: MST ==================================================

/** Audio parameter data*/

typedef struct pal_param_proxy_channel_config {
    uint32_t num_proxy_channels;
} pal_param_proxy_channel_config_t;

struct pal_param_context_list {
    uint32_t num_contexts;
    uint32_t context_id[]; /* list of num_contexts context_id */
};

struct pal_param_disp_port_config_params {
    int controller;
    int stream;
};

struct pal_usb_device_address {
    int card_id;
    int device_num;
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    int vid;
    int pid;
    struct audio_route *usb_ar;
#endif
};

typedef union {
    struct pal_param_disp_port_config_params dp_config;
    struct pal_usb_device_address usb_addr;
} pal_device_config_t;

struct pal_amp_db_and_gain_table {
    float    amp;
    float    db;
    uint32_t level;
};

struct pal_vol_ctrl_ramp_param {
   uint32_t ramp_period_ms;
};

/* Payload For ID: PAL_PARAM_ID_DEVICE_CONNECTION
 * Description   : Device Connection
*/
typedef struct pal_param_device_connection {
    pal_device_id_t   id;
    bool              connection_state;
    pal_device_config_t device_config;
#ifdef SEC_AUDIO_BT_OFFLOAD
    bool              is_bt_offload_enabled;
#endif
} pal_param_device_connection_t;

/* Payload For ID: PAL_PARAM_ID_GAIN_LVL_MAP
 * Description   : get gain level mapping
*/
typedef struct pal_param_gain_lvl_map {
    struct pal_amp_db_and_gain_table *mapping_tbl;
    int                              table_size;
    int                              filled_size;
} pal_param_gain_lvl_map_t;

/* Payload For ID: PAL_PARAM_ID_GAIN_LVL_CAL
 * Description   : set gain level calibration
*/
typedef struct pal_param_gain_lvl_cal {
    int level;
} pal_param_gain_lvl_cal_t;

/* Payload For ID: PAL_PARAM_ID_MSPP_LINEAR_GAIN
 * Description   : set linear gain for MSPP
*/
typedef struct pal_param_mspp_linear_gain {
    int32_t gain;
} pal_param_mspp_linear_gain_t;

/*
* payload for playspeed and pitch
*/
typedef struct pal_param_playback_rate {
     float speed;
     float pitch;
} pal_param_playback_rate_t;

/* Payload For ID: PAL_PARAM_ID_DEVICE_CAPABILITY
 * Description   : get Device Capability
*/
 typedef struct pal_param_device_capability {
  pal_device_id_t   id;
  struct pal_usb_device_address addr;
  bool              is_playback;
  struct dynamic_media_config *config;
} pal_param_device_capability_t;

/* Payload For ID: PAL_PARAM_ID_SCREEN_STATE
 * Description   : Screen State
*/
typedef struct pal_param_screen_state {
    bool              screen_state;
} pal_param_screen_state_t;

/* Payload For ID: PAL_PARAM_ID_CHARGING_STATE
 * Description   : Charging State
*/
typedef struct pal_param_charging_state {
    bool              charging_state;
} pal_param_charging_state_t;

/* Payload For ID: PAL_PARAM_ID_CHARGER_STATE
 * Description   : Charger State
*/
typedef struct pal_param_charger_state {
    bool              is_charger_online;
    bool              is_concurrent_boost_enable;
} pal_param_charger_state_t;

/* Payload For ID: PAL_PARAM_ID_ST_CAPTURE_INFO
 * Description   : Capture pal_handle in resource_manager
 */
typedef struct pal_param_st_capture_info {
    int                   capture_handle;
    pal_stream_handle_t   *pal_handle;
} pal_param_st_capture_info_t;

/* Payload For ID: PAL_PARAM_ID_RESOURCES_AVAILABLE
 * Description   : capture callback and cookie in resource_manager
 */
typedef struct pal_param_resources_available {
    void*             callback;
    uint64_t          cookie;
} pal_param_resources_available_t;

/*
 * Used to identify the swapping type
 */
typedef enum {
    PAL_SPEAKER_ROTATION_LR,  /* Default position. It will be set when device
                               * is at angle 0, 90 or 180 degree.
                               */

    PAL_SPEAKER_ROTATION_RL   /* This will be set if device is rotated by 270
                               * degree
                               */
} pal_speaker_rotation_type;

#ifdef SEC_AUDIO_CALL_RECORD
enum {
    PAL_CALL_RECORD_RX_TX_NON_MIX = PAL_SPEAKER_ROTATION_RL
};
#endif

/* Payload For ID: PAL_PARAM_ID_DEVICE_ROTATION
 * Description   : Device Rotation
 */
typedef struct pal_param_device_rotation {
    pal_speaker_rotation_type    rotation_type;
} pal_param_device_rotation_t;

#ifndef SEC_AUDIO_SUPPORT_UHQ // use pal_param_uhqa_state define on SecPalDefs.h
/* Payload For ID: PAL_PARAM_ID_UHQA_FLAG
 * Description   : use to enable/disable USB high quality audio from userend
*/
typedef struct pal_param_uhqa_state {
    bool uhqa_state;
} pal_param_uhqa_t;
#endif

/* Payload For ID: PAL_PARAM_ID_HAPTICS_CNFG
 * Description   : Store the haptics param and use while extracting info from
                   xml
*/
typedef struct  pal_param_haptics_cnfg_t {
    pal_stream_haptics_type_t mode;
    int16_t  effect_id;
    float    amplitude;
    int16_t strength;
    int32_t time;
    int16_t ch_mask;
    bool isCompose;
    int32_t buffer_size;
    uint8_t *buffer_ptr;
} pal_param_haptics_cnfg_t;

/* Payload For ID: PAL_PARAM_ID_BT_SCO*
 * Description   : BT SCO related device parameters
*/
static const char* lc3_reserved_params[] = {
    "StreamMap",
    "Codec",
    "FrameDuration",
    "rxconfig_index",
    "txconfig_index",
    "version",
    "Blocks_forSDU",
    "vendor",
};

enum {
    LC3_STREAM_MAP_BIT     = 0x1,
    LC3_CODEC_BIT          = 0x1 << 1,
    LC3_FRAME_DURATION_BIT = 0x1 << 2,
    LC3_RXCFG_IDX_BIT      = 0x1 << 3,
    LC3_TXCFG_IDX_BIT      = 0x1 << 4,
    LC3_VERSION_BIT        = 0x1 << 5,
    LC3_BLOCKS_FORSDU_BIT  = 0x1 << 6,
    LC3_VENDOR_BIT         = 0x1 << 7,
    LC3_BIT_ALL            = LC3_STREAM_MAP_BIT |
                             LC3_CODEC_BIT |
                             LC3_FRAME_DURATION_BIT |
                             LC3_RXCFG_IDX_BIT |
                             LC3_TXCFG_IDX_BIT |
                             LC3_VERSION_BIT |
                             LC3_BLOCKS_FORSDU_BIT |
                             LC3_VENDOR_BIT,
    LC3_BIT_MASK           = LC3_BIT_ALL & ~LC3_FRAME_DURATION_BIT, // frame duration is optional
    LC3_BIT_VALID          = LC3_BIT_MASK,
};

/* max length of streamMap string, up to 16 stream id supported */
#define PAL_LC3_MAX_STRING_LEN 200
typedef struct btsco_lc3_cfg {
    uint32_t fields_map;
    uint32_t rxconfig_index;
    uint32_t txconfig_index;
    uint32_t api_version;
    uint32_t frame_duration;
    uint32_t num_blocks;
    uint32_t mode;
    char     streamMap[PAL_LC3_MAX_STRING_LEN];
    char     vendor[PAL_LC3_MAX_STRING_LEN];
} btsco_lc3_cfg_t;

#ifdef SEC_AUDIO_BLUETOOTH
typedef enum {
    PAL_BT_SCO_CODEC_TYPE_NONE,
    PAL_BT_SCO_CODEC_TYPE_RVP,
} pal_bt_sco_codec_t;
#endif

typedef struct pal_param_btsco {
    bool   bt_sco_on;
    bool   bt_wb_speech_enabled;
    bool   bt_sco_nrec;
    int    bt_swb_speech_mode;
    bool   bt_lc3_speech_enabled;
    btsco_lc3_cfg_t lc3_cfg;
    bool   is_bt_hfp;
#ifdef SEC_AUDIO_BLUETOOTH
    pal_bt_sco_codec_t bt_sco_codec_type;
#endif
} pal_param_btsco_t;

/* Payload For ID: PAL_PARAM_ID_BT_A2DP*
 * Description   : A2DP related device setParameters
*/
typedef struct pal_param_bta2dp {
    int32_t  reconfig_supported;
    bool     reconfig;
    bool     a2dp_suspended;
    bool     a2dp_capture_suspended;
    bool     is_tws_mono_mode_on;
    bool     is_lc3_mono_mode_on;
    bool     is_force_switch;
    uint32_t latency;
    pal_device_id_t   dev_id;
    bool     is_suspend_setparam;
    bool     is_in_call;
#ifdef SEC_AUDIO_BT_OFFLOAD
    bool     is_bt_offload_enabled;
#endif
#ifdef SEC_AUDIO_BLE_OFFLOAD
    bool     a2dp_suspended_for_ble;
    uint32_t bt_mix_latency;
#endif
} pal_param_bta2dp_t;

/* Payload For ID: PAL_PARAM_ID_LATENCY_MODE
 * Description   : Get supported or set latency modes
*/
typedef struct pal_param_latency_mode {
    pal_device_id_t dev_id;
    size_t          num_modes; /* number of supported modes */
    uint32_t        modes[PAL_MAX_LATENCY_MODES]; /* list of supported modes or use mode[0] for set latency mode */
} pal_param_latency_mode_t;

typedef struct pal_param_upd_event_detection {
    bool     register_status;
} pal_param_upd_event_detection_t;

typedef struct pal_bt_tws_payload_s {
    bool isTwsMonoModeOn;
    uint32_t codecFormat;
} pal_bt_tws_payload;

/* Payload For Custom Config
 * Description : Used by PAL client to customize
 *               the device related information.
*/
#define PAL_MAX_CUSTOM_KEY_SIZE 128
typedef struct pal_device_custom_config {
    char custom_key[PAL_MAX_CUSTOM_KEY_SIZE];
} pal_device_custom_config_t;

#ifdef SEC_AUDIO_LOOPBACK_TEST
/*
 * Used to identify the factory loopback mode type
 */
typedef enum {
    PAL_LOOPBACK_PACKET_DELAY_MODE,
    PAL_LOOPBACK_PACKET_NO_DELAY_MODE,
    PAL_LOOPBACK_MODE_MAX
} pal_loopback_mode;
/* Payload For ID: PAL_PARAM_ID_LOOPBACK_MODE
 * Description   : loopback mode
 */
typedef struct pal_param_loopback {
    pal_loopback_mode   loopback_mode;
} pal_param_loopback_t;
#endif

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
typedef struct pal_param_ss_pack_a2dp_suspend {
   uint16_t a2dp_suspend;
} pal_param_ss_pack_a2dp_suspend_t;

typedef struct pal_param_bta2dp_dynbitrate {
    uint32_t bitrate;
} pal_param_bta2dp_dynbitrate_t;

typedef struct pal_param_bta2dp_delay_report {
    uint32_t delay_report;
} pal_param_bta2dp_delay_report_t;

typedef struct pal_param_bta2dp_sbm {
    int firstParam;
    int secondParam;
} pal_param_bta2dp_sbm_t;
#endif

typedef struct pal_bt_lc3_payload_s {
    bool isLC3MonoModeOn;
} pal_bt_lc3_payload;

typedef struct pal_param_haptics_intensity {
    int intensity;
} pal_param_haptics_intensity_t;

#ifdef SEC_AUDIO_BARGEIN_MODE
typedef struct pal_param_bargein_mode {
    int mode;
} pal_param_bargein_mode_t;
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
typedef struct pal_param_interpreter_mode {
    int mode;
} pal_param_interpreter_mode_t;
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
typedef struct pal_param_btmix_record {
    bool enable;
} pal_param_btmix_record_t;
#endif

/**< PAL device */
#define DEVICE_NAME_MAX_SIZE 128
struct pal_device {
    pal_device_id_t id;                     /**<  device id */
    struct pal_media_config config;         /**<  media config of the device */
    struct pal_usb_device_address address;
    char sndDevName[DEVICE_NAME_MAX_SIZE];
    pal_device_custom_config_t custom_config;        /**<  Optional */
};

/**
 * Maps the modules instance id to module id for a single module
 */
struct module_info {
    uint32_t module_id; /**< module id */
    uint32_t module_iid; /**< globally unique module instance id */
};

/**
 * Structure mapping the tag_id to module info (mid and miid)
 */
struct pal_tag_module_mapping {
    uint32_t tag_id; /**< tag id of the module */
    uint32_t num_modules; /**< number of modules matching the tag_id */
    struct module_info mod_list[]; /**< module list */
};

/**
 * Used to return tags and module info data to client given a graph key vector
 */
struct pal_tag_module_info {
    /**< number of tags */
    uint32_t num_tags;
    /**< variable payload of type struct pal_tag_module_mapping*/
    uint8_t pal_tag_module_list[];
};

#define PAL_SOUND_TRIGGER_MAX_STRING_LEN 64 /* max length of strings in properties or descriptor structs */
#define PAL_SOUND_TRIGGER_MAX_LOCALE_LEN 6  /* max length of locale string. e.g en_US */
#define PAL_SOUND_TRIGGER_MAX_USERS 10      /* max number of concurrent users */
#define PAL_SOUND_TRIGGER_MAX_PHRASES 10    /* max number of concurrent phrases */

#define PAL_RECOGNITION_MODE_VOICE_TRIGGER 0x1       /* simple voice trigger */
#define PAL_RECOGNITION_MODE_USER_IDENTIFICATION 0x2 /* trigger only if one user in model identified */
#define PAL_RECOGNITION_MODE_USER_AUTHENTICATION 0x4 /* trigger only if one user in mode authenticated */
#define PAL_RECOGNITION_MODE_GENERIC_TRIGGER 0x8     /* generic sound trigger */

#define PAL_RECOGNITION_STATUS_SUCCESS 0
#define PAL_RECOGNITION_STATUS_ABORT 1
#define PAL_RECOGNITION_STATUS_FAILURE 2
#define PAL_RECOGNITION_STATUS_GET_STATE_RESPONSE 3  /* Indicates that the recognition event is in
                                                        response to a state request and was not
                                                        triggered by a real DSP recognition */

/** used to identify the sound model type for the session */
typedef enum {
    PAL_SOUND_MODEL_TYPE_UNKNOWN = -1,        /* use for unspecified sound model type */
    PAL_SOUND_MODEL_TYPE_KEYPHRASE = 0,       /* use for key phrase sound models */
    PAL_SOUND_MODEL_TYPE_GENERIC = 1          /* use for all models other than keyphrase */
} pal_st_sound_model_type_t;

struct st_uuid {
    uint32_t timeLow;
    uint16_t timeMid;
    uint16_t timeHiAndVersion;
    uint16_t clockSeq;
    uint8_t node[6];
};

/**
 * sound trigger implementation descriptor read by the framework via get_properties().
 * Used by SoundTrigger service to report to applications and manage concurrency and policy.
 */
struct pal_st_properties {
    int8_t           implementor[PAL_SOUND_TRIGGER_MAX_STRING_LEN]; /* implementor name */
    int8_t           description[PAL_SOUND_TRIGGER_MAX_STRING_LEN]; /* implementation description */
    uint32_t         version;               /* implementation version */
    struct st_uuid   uuid;                             /* unique implementation ID.
                                                   Must change with version each version */
    uint32_t         max_sound_models;      /* maximum number of concurrent sound models
                                                   loaded */
    uint32_t         max_key_phrases;       /* maximum number of key phrases */
    uint32_t         max_users;             /* maximum number of concurrent users detected */
    uint32_t         recognition_modes;     /* all supported modes.
                                                   e.g PAL_RECOGNITION_MODE_VOICE_TRIGGER */
    bool             capture_transition;    /* supports seamless transition from detection
                                                   to capture */
    uint32_t         max_buffer_ms;         /* maximum buffering capacity in ms if
                                                   capture_transition is true*/
    bool             concurrent_capture;    /* supports capture by other use cases while
                                                   detection is active */
    bool             trigger_in_event;      /* returns the trigger capture in event */
    uint32_t         power_consumption_mw;  /* Rated power consumption when detection is active
                                                   with TDB silence/sound/speech ratio */
};

/** sound model structure passed in by ST Client during pal_st_load_sound_model() */
struct pal_st_sound_model {
    pal_st_sound_model_type_t type;           /* model type. e.g. PAL_SOUND_MODEL_TYPE_KEYPHRASE */
    struct st_uuid            uuid;           /* unique sound model ID. */
    struct st_uuid            vendor_uuid;    /* unique vendor ID. Identifies the engine the
                                                  sound model was build for */
    uint32_t                  data_size;      /* size of opaque model data */
    uint32_t                  data_offset;    /* offset of opaque data start from head of struct
                                                  e.g sizeof struct pal_st_sound_model) */
};

/** key phrase descriptor */
struct pal_st_phrase {
    uint32_t    id;                                 /**< keyphrase ID */
    uint32_t    recognition_mode;                   /**< recognition modes supported by this key phrase */
    uint32_t    num_users;                          /**< number of users in the key phrase */
    uint32_t    users[PAL_SOUND_TRIGGER_MAX_USERS]; /**< users ids: (not uid_t but sound trigger
                                                        specific IDs */
    char        locale[PAL_SOUND_TRIGGER_MAX_LOCALE_LEN]; /**< locale - JAVA Locale style (e.g. en_US) */
    char        text[PAL_SOUND_TRIGGER_MAX_STRING_LEN];   /**< phrase text in UTF-8 format. */
};

/**
 * Specialized sound model for key phrase detection.
 * Proprietary representation of key phrases in binary data must match information indicated
 * by phrases field use this when not sending
 */
struct pal_st_phrase_sound_model {
    struct pal_st_sound_model   common;         /** common sound model */
    uint32_t                    num_phrases;    /** number of key phrases in model */
    struct pal_st_phrase        phrases[PAL_SOUND_TRIGGER_MAX_PHRASES];
};

struct pal_st_confidence_level {
    uint32_t user_id;   /* user ID */
    uint32_t level;     /* confidence level in percent (0 - 100).
                               - min level for recognition configuration
                               - detected level for recognition event */
};

/** Specialized recognition event for key phrase detection */
struct pal_st_phrase_recognition_extra {
    uint32_t id;                /* keyphrase ID */
    uint32_t recognition_modes; /* recognition modes used for this keyphrase */
    uint32_t confidence_level;  /* confidence level for mode RECOGNITION_MODE_VOICE_TRIGGER */
    uint32_t num_levels;        /* number of user confidence levels */
    struct pal_st_confidence_level levels[PAL_SOUND_TRIGGER_MAX_USERS];
};

struct pal_st_recognition_event {
    int32_t                          status;              /**< recognition status e.g.
                                                              RECOGNITION_STATUS_SUCCESS */
    pal_st_sound_model_type_t        type;                /**< event type, same as sound model type.
                                                              e.g. SOUND_MODEL_TYPE_KEYPHRASE */
    pal_st_handle_t                  *st_handle;           /**< handle of sound trigger session */
    bool                             capture_available;   /**< it is possible to capture audio from this
                                                              utterance buffered by the
                                                              implementation */
    int32_t                          capture_session;     /**< audio session ID. framework use */
    int32_t                          capture_delay_ms;    /**< delay in ms between end of model
                                                              detection and start of audio available
                                                              for capture. A negative value is possible
                                                              (e.g. if key phrase is also available for
                                                              capture */
    int32_t                          capture_preamble_ms; /**< duration in ms of audio captured
                                                              before the start of the trigger.
                                                              0 if none. */
    bool                             trigger_in_data;     /**< the opaque data is the capture of
                                                              the trigger sound */
    struct pal_media_config          media_config;        /**< media format of either the trigger in
                                                              event data or to use for capture of the
                                                              rest of the utterance */
    uint32_t                         data_size;           /**< size of opaque event data */
    uint32_t                         data_offset;         /**< offset of opaque data start from start of
                                                              this struct (e.g sizeof struct
                                                              sound_trigger_phrase_recognition_event) */
};

typedef void(*pal_st_recognition_callback_t)(struct pal_st_recognition_event *event,
                                             uint8_t *cookie);

/* Payload for pal_st_start_recognition() */
struct pal_st_recognition_config {
    int32_t       capture_handle;             /**< IO handle that will be used for capture.
                                                N/A if capture_requested is false */
    uint32_t      capture_device;             /**< input device requested for detection capture */
    bool          capture_requested;          /**< capture and buffer audio for this recognition
                                                instance */
    uint32_t      num_phrases;                /**< number of key phrases recognition extras */
    struct pal_st_phrase_recognition_extra phrases[PAL_SOUND_TRIGGER_MAX_PHRASES];
                                              /**< configuration for each key phrase */
    pal_st_recognition_callback_t callback;   /**< callback for recognition events */
    uint8_t *        cookie;                     /**< cookie set from client*/
    uint32_t      data_size;                  /**< size of opaque capture configuration data */
    uint32_t      data_offset;                /**< offset of opaque data start from start of this struct
                                              (e.g sizeof struct sound_trigger_recognition_config) */
};

struct pal_st_phrase_recognition_event {
    struct pal_st_recognition_event common;
    uint32_t num_phrases;
    struct pal_st_phrase_recognition_extra phrase_extras[PAL_SOUND_TRIGGER_MAX_PHRASES];
};

struct pal_st_generic_recognition_event {
    struct pal_st_recognition_event common;
};

struct detection_engine_config_voice_wakeup {
    uint16_t mode;
    uint16_t custom_payload_size;
    uint8_t num_active_models;
    uint8_t reserved;
    uint8_t confidence_levels[PAL_SOUND_TRIGGER_MAX_USERS];
    uint8_t keyword_user_enables[PAL_SOUND_TRIGGER_MAX_USERS];
};

struct detection_engine_config_stage1_pdk {
    uint16_t mode;
    uint16_t custom_payload_size;
    uint32_t model_id;
    uint32_t num_keywords;
    uint32_t confidence_levels[MAX_KEYWORD_SUPPORTED];
};

struct detection_engine_multi_model_buffering_config {
    uint32_t model_id;
    uint32_t hist_buffer_duration_in_ms;
    uint32_t pre_roll_duration_in_ms;
};

struct ffv_doa_tracking_monitor_t
{
    int16_t target_angle_L16[2];
    int16_t interf_angle_L16[2];
    int8_t polarActivityGUI[360];
};

struct __attribute__((__packed__)) version_arch_payload {
    unsigned int version;
    char arch[64];
};

typedef enum {
    PAL_ASR_EVENT_STATUS_SUCCESS = 0,
    PAL_ASR_EVENT_STATUS_ABORTED = 1,
} pal_asr_event_status_t;

typedef enum {
    PAL_ASR_STATUS_SUCCESS = 0,
    PAL_ASR_STATUS_INVALID = -1,
    PAL_ASR_STATUS_RESOURCE_CONTENTION = -2,
    PAL_ASR_STATUS_OPERATION_NOT_SUPPORTED = -3,
    PAL_ASR_STATUS_FAILURE = -4,
} pal_asr_status_t;

/*  Payload for ID : PAL_PARAM_ID_ASR_MODEL.
 *  Description: To be used to pass ASR model.
 */
struct pal_asr_model {
    struct st_uuid vendor_uuid;  /**< Unique vendor ID. Identifies the engine the model was build for */
    int32_t fd;                  /**< ASR model */
    uint32_t size;               /**< ASR model size */
};

/* Payload for ID : PAL_PARAM_ID_ASR_CONFIG.
 * Description: To be used to pass ASR input configuration.
 */
struct pal_asr_config {
    int32_t input_language_code;            /**< input language code */
    int32_t output_language_code;           /**< output language code */
    bool enable_language_detection;         /**< language detection switch enable/disable flag */
    bool enable_translation;                /**< translation switch enable/disable flag */
    bool enable_continuous_mode;            /**< continuous mode enable/disable flag */
    bool enable_partial_transcription;      /**< partial transcription switch enable/disable flag */
    uint32_t threshold;                     /**< Confidence threshold for ASR transcription */
    uint32_t timeout_duration;              /**< ASR processing timeout, in milliseconds, if silence is not detected
                                                 after the speech processing is started*/
    uint32_t silence_detection_duration;    /**< "No speech" duration needed before determining speech has ended (in ms) */

    bool outputBufferMode;                  /**< Buffer mode enable/disable */
    uint32_t data_size;                     /**< Additional Engine specific custom data size */
    uint8_t data[];                         /**< custom data offset from the start of this structure */
};

#define MAX_TRANSCRIPTION_CHAR_SIZE 1024
#define MAX_JSON_CHAR_SIZE 4096
struct pal_asr_engine_event {
    bool is_final;                          /**< Final transcription after end of speech is detected. */
    uint32_t confidence;                    /**< Confidence for the entire transcription */
    uint32_t text_size;                     /**< Size of text to sent to the client */
    char text[MAX_TRANSCRIPTION_CHAR_SIZE]; /**< Text to be sent to the client */
    uint32_t json_size;                     /**< size of JSON output, to be sent to client */
    char result_json[MAX_JSON_CHAR_SIZE];   /**< JSON result */
    uint32_t data_size;                     /**< event payload size */
    uint8_t data[];                         /**< event payload offset from the start of this structure */
};

/* Payload to be used for callback for ASR event through pal_stream_callback
 */
struct pal_asr_event {
    pal_asr_event_status_t status;
    uint32_t num_events;
    struct pal_asr_engine_event event[];
};

struct pal_compr_gapless_mdata {
       uint32_t encoderDelay;
       uint32_t encoderPadding;
};

typedef struct pal_device_mute_t {
    pal_stream_direction_t dir;
    bool mute;
}pal_device_mute_t;

/**
 * Event payload passed to client with PAL_STREAM_CBK_EVENT_READ_DONE and
  * PAL_STREAM_CBK_EVENT_WRITE_READY events
  */
struct pal_event_read_write_done_payload {
    uint32_t tag; /**< tag that was used to read/write this buffer */
    uint32_t status; /**< data buffer status as defined in ar_osal_error.h */
    uint32_t md_status; /**< meta-data status as defined in ar_osal_error.h */
    struct pal_buffer buff; /**< buffer that was passed to pal_stream_read/pal_stream_write */
};

/** @brief Callback function prototype to be given for
 *         pal_open_stream.
 *
 * \param[in] stream_handle - stream handle associated with the
 * callback event.
 * \param[in] event_id - event id of the event raised on the
 *       stream.
 * \param[in] event_data - event_data specific to the event
 *       raised.
 * \param[in] cookie - cookie speificied in the
 *       pal_stream_open()
 */
typedef int32_t (*pal_stream_callback)(pal_stream_handle_t *stream_handle,
                                       uint32_t event_id, uint32_t *event_data,
                                       uint32_t event_data_size,
                                       uint64_t cookie);

/** @brief Callback function prototype to be given for
 *         pal_audio_event_callback.
 *
 * \param[in] config - configuration data related to
 *       stream and device.
 * \param[in] event - event raised on the stream.
 * \param[in] isregister - specifies if it is called
 *       during register of callback.
 */
typedef int32_t (*pal_audio_event_callback)(pal_callback_config_t *config, uint32_t event, bool isregister);

/** @brief Callback function prototype to be given for
 *         pal_register_callback.
 *
 * \param[in] event_id - event id of the event raised on the
 *       stream.
 * \param[in] event_data - event_data specific to the event
 *       raised.
 * \param[in] cookie - cookie specified in the
 *       pal_register_global_callback.
 */
typedef int32_t (*pal_global_callback)(uint32_t event_id, uint32_t *event_data, uint64_t cookie);

/** Sound card state */
typedef enum card_status_t {
    CARD_STATUS_OFFLINE = 0,
    CARD_STATUS_ONLINE,
    CARD_STATUS_STANDBY,
    CARD_STATUS_NONE,
} card_status_t;

#define PAL_CARD_STATUS_DOWN(n)     (n == CARD_STATUS_OFFLINE || n == CARD_STATUS_STANDBY)
#define PAL_CARD_STATUS_UP(n)       (n == CARD_STATUS_ONLINE)

typedef struct pal_buffer_config {
    size_t buf_count; /**< number of buffers*/
    size_t buf_size; /**< This would be the size of each buffer*/
    size_t max_metadata_size; /** < max metadata size associated with each buffer*/
} pal_buffer_config_t;

#define PAL_GENERIC_PLATFORM_DELAY     (29*1000LL)
#define PAL_DEEP_BUFFER_PLATFORM_DELAY (29*1000LL)
#define PAL_SPATIAL_AUDIO_PLATFORM_DELAY (13*1000LL)
#define PAL_PCM_OFFLOAD_PLATFORM_DELAY (30*1000LL)
#define PAL_LOW_LATENCY_PLATFORM_DELAY (13*1000LL)
#define PAL_MMAP_PLATFORM_DELAY        (3*1000LL)
#define PAL_ULL_PLATFORM_DELAY         (4*1000LL)
#define PAL_VOIP_PLATFORM_DELAY        (29*1000LL)

#define PAL_GENERIC_OUTPUT_PERIOD_DURATION 40
#define PAL_DEEP_BUFFER_OUTPUT_PERIOD_DURATION 40
#define PAL_PCM_OFFLOAD_OUTPUT_PERIOD_DURATION 80
#define PAL_LOW_LATENCY_OUTPUT_PERIOD_DURATION 5
#define PAL_SPATIAL_AUDIO_PERIOD_DURATION 10
#define PAL_VOIP_OUTPUT_PERIOD_DURATION 20
#define PAL_ULL_OUTPUT_PERIOD_DURATION 10

#define PAL_GENERIC_PLAYBACK_PERIOD_COUNT 2
#define PAL_DEEP_BUFFER_PLAYBACK_PERIOD_COUNT 2
#define PAL_PCM_OFFLOAD_PLAYBACK_PERIOD_COUNT 2
#define PAL_LOW_LATENCY_PLAYBACK_PERIOD_COUNT 2
#define PAL_SPATIAL_AUDIO_PLAYBACK_PERIOD_COUNT 2
#define PAL_VOIP_PLAYBACK_PERIOD_COUNT 2
#define PAL_ULL_PLAYBACK_PERIOD_COUNT 2

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /*PAL_DEFS_H*/
