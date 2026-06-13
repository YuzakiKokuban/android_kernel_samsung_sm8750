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

#define LOG_TAG "PAL: bt_ble"
//#define LOG_NDEBUG 0

#include <log/log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "bt_ble.h"
#include <lc3_encoder_api.h>
#include <lc3_decoder_api.h>

static int ble_pack_def_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_lc3_codec_cfg_t *ble_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    struct param_id_lc3_encoder_config_payload_t *enc_init = NULL;
    int ret = 0, num_blks = 1, payload_sz = 0, i = 0;
    custom_block_t *blk[1] = {NULL};

    ble_bt_cfg = (audio_lc3_codec_cfg_t *)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }

    enc_payload->bit_format     = def_toair_cfg.bit_depth;
    enc_payload->sample_rate    = def_toair_cfg.sampling_freq;
    enc_payload->num_blks       = def_toair_cfg.num_blocks;
    enc_payload->channel_count  = CH_STEREO;

    enc_payload->is_abr_enabled    = true;
    enc_payload->is_enc_config_set = ble_bt_cfg->is_enc_config_set;
    enc_payload->is_dec_config_set = ble_bt_cfg->is_dec_config_set;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_LC3_ENC_INIT */
    payload_sz = sizeof(struct param_id_lc3_encoder_config_payload_t) +
                 sizeof(stream_map_t) * DEF_STREAM_MAP_SZ;
    enc_init = (struct param_id_lc3_encoder_config_payload_t *)calloc(1, payload_sz);
    if (enc_init == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }

    enc_init->stream_map_size                  = DEF_STREAM_MAP_SZ;
    enc_init->toAirConfig.num_blocks           = def_toair_cfg.num_blocks;
    enc_init->toAirConfig.api_version          = def_toair_cfg.api_version;
    enc_init->toAirConfig.sampling_Frequency   = def_toair_cfg.sampling_freq;
    enc_init->toAirConfig.max_octets_per_frame = def_toair_cfg.max_octets_per_frame;
    enc_init->toAirConfig.frame_duration       = def_toair_cfg.frame_duration;
    enc_init->toAirConfig.bit_depth            = def_toair_cfg.bit_depth;
    enc_init->toAirConfig.mode                 = def_toair_cfg.mode;
    for (i = 0; i < 16; i++) {
        enc_init->toAirConfig.vendor_specific[i] =
            ble_bt_cfg->enc_cfg.toAirConfig.vendor_specific[i];
    }

    for (i = 0; i < enc_init->stream_map_size; i++) {
        enc_init->streamMapOut[i].audio_location = def_stream_map_out[i].audio_location;
        enc_init->streamMapOut[i].stream_id = def_stream_map_out[i].stream_id;
        enc_init->streamMapOut[i].direction = def_stream_map_out[i].direction;
    }
    ret = bt_base_populate_enc_cmn_param(blk[0], PARAM_ID_LC3_ENC_INIT,
            enc_init, payload_sz);
    free(enc_init);
    if (ret)
        goto free_payload;

    enc_payload->blocks[0] = blk[0];
    *dst = enc_payload;
    codec->payload = enc_payload;

    return ret;
free_payload:
    for (i = 0; i < num_blks; i++) {
        if (blk[i]) {
            if (blk[i]->payload)
                free(blk[i]->payload);
            free(blk[i]);
        }
    }
    if (enc_payload)
        free(enc_payload);
    return ret;
}

static int ble_pack_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_lc3_codec_cfg_t *ble_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    struct param_id_lc3_encoder_config_payload_t *enc_init = NULL;
    int ret = 0, num_blks = 1, payload_sz = 0, i = 0;
    custom_block_t *blk[1] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    ble_bt_cfg = (audio_lc3_codec_cfg_t *)src;

    if (!ble_bt_cfg->is_enc_config_set) {
        ALOGI("%s: is_enc_config_set is false; setting default enc params",
                __func__);
        return ble_pack_def_enc_config(codec, src, dst);
    }

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }

    enc_payload->bit_format     = ble_bt_cfg->enc_cfg.toAirConfig.bit_depth;
    enc_payload->sample_rate    = ble_bt_cfg->enc_cfg.toAirConfig.sampling_freq;
    enc_payload->num_blks       = num_blks;
    if (ble_bt_cfg->enc_cfg.stream_map_size) {
        if ((1 == ble_bt_cfg->enc_cfg.stream_map_size) &&
            (ble_bt_cfg->enc_cfg.streamMapOut[0].audio_location != 3))
            enc_payload->channel_count = CH_MONO;
        else
            enc_payload->channel_count = CH_STEREO;
    }

    enc_payload->is_abr_enabled    = true;
    enc_payload->is_enc_config_set = ble_bt_cfg->is_enc_config_set;
    enc_payload->is_dec_config_set = ble_bt_cfg->is_dec_config_set;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_LC3_ENC_INIT */
    payload_sz = sizeof(struct param_id_lc3_encoder_config_payload_t) +
                 sizeof(stream_map_t) * ble_bt_cfg->enc_cfg.stream_map_size;
    enc_init = (struct param_id_lc3_encoder_config_payload_t *)calloc(1, payload_sz);
    if (enc_init == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }

    enc_init->stream_map_size                  = ble_bt_cfg->enc_cfg.stream_map_size;
    enc_init->toAirConfig.api_version          = ble_bt_cfg->enc_cfg.toAirConfig.api_version;
    enc_init->toAirConfig.sampling_Frequency   = ble_bt_cfg->enc_cfg.toAirConfig.sampling_freq;
    enc_init->toAirConfig.max_octets_per_frame = ble_bt_cfg->enc_cfg.toAirConfig.max_octets_per_frame;
    enc_init->toAirConfig.frame_duration       = ble_bt_cfg->enc_cfg.toAirConfig.frame_duration;
    enc_init->toAirConfig.bit_depth            = ble_bt_cfg->enc_cfg.toAirConfig.bit_depth;
    enc_init->toAirConfig.num_blocks           = ble_bt_cfg->enc_cfg.toAirConfig.num_blocks;
    enc_init->toAirConfig.default_q_level      = ble_bt_cfg->enc_cfg.toAirConfig.default_q_level;
    enc_init->toAirConfig.mode                 = ble_bt_cfg->enc_cfg.toAirConfig.mode;
    for (i = 0; i < 16; i++) {
        enc_init->toAirConfig.vendor_specific[i] =
            ble_bt_cfg->enc_cfg.toAirConfig.vendor_specific[i];
        if (i == VERSION_IDX)
            enc_payload->codec_version = enc_init->toAirConfig.vendor_specific[i];
    }

    for (i = 0; i < enc_init->stream_map_size; i++) {
        enc_init->streamMapOut[i].audio_location = ble_bt_cfg->enc_cfg.streamMapOut[i].audio_location;
        enc_init->streamMapOut[i].stream_id = ble_bt_cfg->enc_cfg.streamMapOut[i].stream_id;
        enc_init->streamMapOut[i].direction = ble_bt_cfg->enc_cfg.streamMapOut[i].direction;
    }

    ret = bt_base_populate_enc_cmn_param(blk[0], PARAM_ID_LC3_ENC_INIT,
            enc_init, payload_sz);
    free(enc_init);
    if (ret)
        goto free_payload;

    enc_payload->blocks[0] = blk[0];
    *dst = enc_payload;
    codec->payload = enc_payload;

    return ret;
free_payload:
    for (i = 0; i < num_blks; i++) {
        if (blk[i]) {
            if (blk[i]->payload)
                free(blk[i]->payload);
            free(blk[i]);
        }
    }
    if (enc_payload)
        free(enc_payload);
    return ret;
}

static int ble_pack_dec_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_lc3_codec_cfg_t *ble_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    struct param_id_lc3_decoder_config_payload_t *dec_init = NULL;
    int ret = 0, num_blks = 1, payload_sz = 0, i = 0;
    custom_block_t *blk[1] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    ble_bt_cfg = (audio_lc3_codec_cfg_t *)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }

    enc_payload->bit_format        = ble_bt_cfg->dec_cfg.fromAirConfig.bit_depth;
    enc_payload->sample_rate       = ble_bt_cfg->dec_cfg.fromAirConfig.sampling_freq;
    enc_payload->channel_count     = ble_bt_cfg->dec_cfg.decoder_output_channel;
    enc_payload->num_blks          = num_blks;
    enc_payload->is_abr_enabled    = true;
    enc_payload->is_enc_config_set = ble_bt_cfg->is_enc_config_set;
    enc_payload->is_dec_config_set = ble_bt_cfg->is_dec_config_set;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_LC3_DEC_INIT */
    payload_sz = sizeof(struct param_id_lc3_decoder_config_payload_t) +
                 sizeof(stream_map_t) * ble_bt_cfg->dec_cfg.stream_map_size;
    dec_init = (struct param_id_lc3_decoder_config_payload_t *)calloc(1, payload_sz);
    if (dec_init == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }

    dec_init->decoder_output_channel             = ble_bt_cfg->dec_cfg.decoder_output_channel;
    dec_init->stream_map_size                    = ble_bt_cfg->dec_cfg.stream_map_size;
    dec_init->fromAirConfig.api_version          = ble_bt_cfg->dec_cfg.fromAirConfig.api_version;
    dec_init->fromAirConfig.sampling_Frequency   = ble_bt_cfg->dec_cfg.fromAirConfig.sampling_freq;
    dec_init->fromAirConfig.max_octets_per_frame = ble_bt_cfg->dec_cfg.fromAirConfig.max_octets_per_frame;
    dec_init->fromAirConfig.frame_duration       = ble_bt_cfg->dec_cfg.fromAirConfig.frame_duration;
    dec_init->fromAirConfig.bit_depth            = ble_bt_cfg->dec_cfg.fromAirConfig.bit_depth;
    dec_init->fromAirConfig.num_blocks           = ble_bt_cfg->dec_cfg.fromAirConfig.num_blocks;
    dec_init->fromAirConfig.default_q_level      = ble_bt_cfg->dec_cfg.fromAirConfig.default_q_level;
    dec_init->fromAirConfig.mode                 = ble_bt_cfg->dec_cfg.fromAirConfig.mode;
    for (i = 0; i < 16; i++) {
        dec_init->fromAirConfig.vendor_specific[i] =
            ble_bt_cfg->dec_cfg.fromAirConfig.vendor_specific[i];
        if (i == VERSION_IDX)
            enc_payload->codec_version = dec_init->fromAirConfig.vendor_specific[i];
    }

    for (i = 0; i < dec_init->stream_map_size; i++) {
        dec_init->streamMapIn[i].audio_location = ble_bt_cfg->dec_cfg.streamMapIn[i].audio_location;
        dec_init->streamMapIn[i].stream_id      = ble_bt_cfg->dec_cfg.streamMapIn[i].stream_id;
        dec_init->streamMapIn[i].direction      = ble_bt_cfg->dec_cfg.streamMapIn[i].direction;
    }

    ret = bt_base_populate_enc_cmn_param(blk[0], PARAM_ID_LC3_DEC_INIT,
            dec_init, payload_sz);
    free(dec_init);
    if (ret)
        goto free_payload;

    enc_payload->blocks[0] = blk[0];
    *dst = enc_payload;
    codec->payload = enc_payload;

    return ret;
free_payload:
    for (i = 0; i < num_blks; i++) {
        if (blk[i]) {
            if (blk[i]->payload)
                free(blk[i]->payload);
            free(blk[i]);
        }
    }
    if (enc_payload)
        free(enc_payload);
    return ret;
}

static int bt_ble_populate_payload(bt_codec_t *codec, void *src, void **dst)
{
    config_fn_t config_fn = NULL;

    if (codec->payload)
        free(codec->payload);

    switch (codec->codecFmt) {
        case CODEC_TYPE_LC3:
        case CODEC_TYPE_APTX_AD_R4:
            config_fn = ((codec->direction == ENC) ? &ble_pack_enc_config :
                                                     &ble_pack_dec_config);
            break;
        case CODEC_TYPE_APTX_AD_QLEA:
            config_fn = ((codec->direction == ENC) ? &ble_pack_enc_config :
                                                     NULL);
            break;
        default:
            ALOGD("%s unsupported codecFmt %d\n", __func__, codec->codecFmt);
    }

    if (config_fn != NULL) {
        return config_fn(codec, src, dst);
    }

    return -EINVAL;
}

static uint64_t bt_ble_get_decoder_latency(bt_codec_t *codec)
{
    uint32_t latency = 0;

    switch (codec->codecFmt) {
        case CODEC_TYPE_LC3:
            latency = 0;
            break;
        default:
            latency = 200;
            ALOGD("No valid decoder defined, setting latency to %dms", latency);
            break;
    }
    return (uint64_t)latency;
}

static uint64_t bt_ble_get_encoder_latency(bt_codec_t *codec)
{
    uint32_t latency = 0;

    switch (codec->codecFmt) {
        case CODEC_TYPE_LC3:
        case CODEC_TYPE_APTX_AD_QLEA:
        case CODEC_TYPE_APTX_AD_R4:
            /* for BLE, the latency depends on the mode (HQ/LL) and
             * BT IPC will take care of accomodating the mode factor and return latency
             */
            latency = 0;
            break;
        default:
            latency = 200;
            break;
    }
    ALOGV("%s: codecFmt %u, direction %d, latency %u",
            __func__, codec->codecFmt, codec->direction, latency);
    return latency;
}

static uint64_t bt_ble_get_codec_latency(bt_codec_t *codec)
{
    if (codec->direction == ENC)
        return bt_ble_get_encoder_latency(codec);
    else
        return bt_ble_get_decoder_latency(codec);
}

int bt_ble_query_num_codecs(bt_codec_t *codec __unused) {
    ALOGV("%s", __func__);
    return NUM_CODEC;
}

void bt_ble_close(bt_codec_t *codec)
{
    bt_enc_payload_t *payload = codec->payload;
    int num_blks, i;
    custom_block_t *blk;

    if (payload) {
        num_blks = payload->num_blks;
        for (i = 0; i < num_blks; i++) {
            blk = payload->blocks[i];
            if (blk) {
                if (blk->payload)
                    free(blk->payload);
                free(blk);
                blk = NULL;
            }
        }
        free(payload);
    }
    free(codec);
}

__attribute__ ((visibility ("default")))
int plugin_open(bt_codec_t **codec, uint32_t codecFmt, codec_type direction)
{
    bt_codec_t *bt_codec = NULL;


    bt_codec = calloc(1, sizeof(bt_codec_t));
    if (!bt_codec) {
        ALOGE("%s: Memory allocation failed\n", __func__);
        return -ENOMEM;
    }

    bt_codec->codecFmt = codecFmt;
    bt_codec->direction = direction;
    bt_codec->plugin_populate_payload  = bt_ble_populate_payload;
    bt_codec->plugin_get_codec_latency = bt_ble_get_codec_latency;
    bt_codec->plugin_query_num_codecs  = bt_ble_query_num_codecs;
    bt_codec->close_plugin  = bt_ble_close;

    *codec = bt_codec;
    return 0;
}
