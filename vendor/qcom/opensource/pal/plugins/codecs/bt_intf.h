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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _BT_PLUGIN_INTF_H_
#define _BT_PLUGIN_INTF_H_

// import data definition expected from Spf
#include <ar_osal_types.h>
#include <common_enc_dec_api.h>
#include <media_fmt_api.h>

#include "SecProductFeature_BLUETOOTH.h"

/*
 * Below enum values are extended from audio_base.h to
 * to keep encoder and decoder type local to bthost_ipc
 * and audio_hal as these are intended only for handshake
 * between IPC lib and Audio HAL.
 */
typedef enum {
    CODEC_TYPE_INVALID        = 0xFFFFFFFFu,
    CODEC_TYPE_AAC            = 0x04000000u,
    CODEC_TYPE_SBC            = 0x1F000000u,
    CODEC_TYPE_APTX           = 0x20000000u,
    CODEC_TYPE_APTX_HD        = 0x21000000u,
    CODEC_TYPE_APTX_DUAL_MONO = 0x22000000u,
    CODEC_TYPE_LDAC           = 0x23000000u,
    CODEC_TYPE_CELT           = 0x24000000u,
    CODEC_TYPE_APTX_AD        = 0x25000000u,
    CODEC_TYPE_APTX_AD_SPEECH = 0x26000000u,
    CODEC_TYPE_RVP            = 0x28000000u, // SS_BT_HFP - H_127 : RVP
    CODEC_TYPE_LC3            = 0x2B000000u,
    CODEC_TYPE_PCM            = 0x1u,
    CODEC_TYPE_APTX_AD_QLEA   = 0x30000000u,
    CODEC_TYPE_APTX_AD_R4     = 0x31000000u,
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    CODEC_TYPE_SSC            = 0x27000000UL,
#endif
} codec_format_t;

/*
 * Update the below lookup table as well when new codec formats get added
 * This will be used by Payload Builder to popualte the Graph KVs
 */
#ifdef __cplusplus
#include <map>
#include <string>
const std::map<uint32_t, std::string> btCodecFormatLUT {
    {CODEC_TYPE_INVALID,          std::string{ "CODEC_TYPE_INVALID"} },
    {CODEC_TYPE_AAC,              std::string{ "CODEC_TYPE_AAC"} },
    {CODEC_TYPE_SBC,              std::string{ "CODEC_TYPE_SBC"} },
    {CODEC_TYPE_APTX,             std::string{ "CODEC_TYPE_APTX"} },
    {CODEC_TYPE_APTX_HD,          std::string{ "CODEC_TYPE_APTX_HD"} },
    {CODEC_TYPE_APTX_DUAL_MONO,   std::string{ "CODEC_TYPE_APTX_DUAL_MONO"} },
    {CODEC_TYPE_LDAC,             std::string{ "CODEC_TYPE_LDAC"} },
    {CODEC_TYPE_CELT,             std::string{ "CODEC_TYPE_CELT"} },
    {CODEC_TYPE_APTX_AD,          std::string{ "CODEC_TYPE_APTX_AD"} },
    {CODEC_TYPE_APTX_AD_SPEECH,   std::string{ "CODEC_TYPE_APTX_AD_SPEECH"} },
    {CODEC_TYPE_RVP,              std::string{ "CODEC_TYPE_RVP"} }, // SS_BT_HFP - H_127 : RVP
    {CODEC_TYPE_LC3,              std::string{ "CODEC_TYPE_LC3"} },
    {CODEC_TYPE_PCM,              std::string{ "CODEC_TYPE_PCM"} },
    {CODEC_TYPE_APTX_AD_QLEA,     std::string{ "CODEC_TYPE_APTX_AD_QLEA"} },
    {CODEC_TYPE_APTX_AD_R4,     std::string{ "CODEC_TYPE_APTX_AD_R4"} },
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    {CODEC_TYPE_SSC,              std::string{ "CODEC_TYPE_SSC"} },
#endif
};
#endif

typedef enum {
    ENC = 1,
    DEC
} codec_type;

typedef enum {
    V1=1,
    V2
}codec_version_t;

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define DEFAULT_SINK_LATENCY_SBC        100
#define DEFAULT_SINK_LATENCY_SSC        160
#else
#define DEFAULT_SINK_LATENCY_SBC        140
#endif
#define DEFAULT_SINK_LATENCY_AAC        180
#define DEFAULT_SINK_LATENCY_CELT       180
#define DEFAULT_SINK_LATENCY_LDAC       180
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
#define DEFAULT_SINK_LATENCY_APTX       110
#else
#define DEFAULT_SINK_LATENCY_APTX       160
#endif
#define DEFAULT_SINK_LATENCY_APTX_HD    180
#define DEFAULT_SINK_LATENCY            200

/* encoder payload sent for BT device */
typedef struct custom_block_s {
    uint32_t param_id;
    uint32_t payload_sz;
    uint8_t  *payload;
} custom_block_t;

typedef struct bt_enc_payload {
    uint32_t channel_count;
    uint32_t bit_format;
    uint32_t sample_rate;
    bool is_abr_enabled;
    uint32_t  num_blks;
    bool is_enc_config_set;
    bool is_dec_config_set;
    codec_version_t codec_version;
    custom_block_t *blocks[];
} bt_enc_payload_t;

typedef struct bt_codec_library_s bt_codec_t;

struct bt_codec_library_s {
    const char *name;
    uint32_t codecFmt;
    codec_type direction;
    bt_enc_payload_t *payload;
    int (*plugin_populate_payload)(bt_codec_t *codec, void *src, void **dst);
    uint64_t (*plugin_get_codec_latency)(bt_codec_t *codec);
    int (*plugin_query_num_codecs)(bt_codec_t *codec);
    void (*close_plugin) (bt_codec_t *codec);
};

typedef int (*config_fn_t) (bt_codec_t *codec, void *src, void **dst);
typedef int (*open_fn_t) (bt_codec_t **codec, uint32_t codecFmt, codec_type direction);

#endif /* _BT_PLUGIN_INTF_H_ */
