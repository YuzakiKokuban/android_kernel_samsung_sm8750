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

#define LOG_TAG "PAL: bt_aptx"
//#define LOG_NDEBUG 0

#include <log/log.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "bt_aptx.h"
#include "bt_base.h"
#include <aptx_classic_encoder_api.h>
#include <aptx_hd_encoder_api.h>
#include <aptx_adaptive_encoder_api.h>
#include <aptx_adaptive_swb_encoder_api.h>
#include <aptx_adaptive_swb_decoder_api.h>

typedef enum {
    SWAP_DISABLE,
    SWAP_ENABLE,
} swap_status_t;

#define DEFAULT_FADE_DURATION           255

static int aptx_pack_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_aptx_encoder_config_t *aptx_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    int ret = 0, num_blks = 1, i = 0;
    custom_block_t *blk[1] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    aptx_bt_cfg = (audio_aptx_encoder_config_t *)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }
    enc_payload->bit_format    = aptx_bt_cfg->bits_per_sample;
    enc_payload->sample_rate   = aptx_bt_cfg->sampling_rate;
    enc_payload->channel_count = aptx_bt_cfg->channels;
    enc_payload->num_blks      = num_blks;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_REAL_MODULE_ID */
    ret = bt_base_populate_real_module_id(blk[0], MODULE_ID_APTX_CLASSIC_ENC);
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

static int aptx_dual_mono_pack_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_aptx_dual_mono_config_t *aptx_dm_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    param_id_aptx_classic_sync_mode_payload_t *aptx_sync_mode = NULL;
    int ret = 0, num_blks = 2, i = 0;
    custom_block_t *blk[2] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    aptx_dm_bt_cfg = (audio_aptx_dual_mono_config_t*)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }
    enc_payload->bit_format    = ENCODER_BIT_FORMAT_DEFAULT;
    enc_payload->sample_rate   = aptx_dm_bt_cfg->sampling_rate;
    enc_payload->channel_count = aptx_dm_bt_cfg->channels;
    enc_payload->num_blks      = num_blks;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_REAL_MODULE_ID */
    ret = bt_base_populate_real_module_id(blk[0], MODULE_ID_APTX_CLASSIC_ENC);
    if (ret)
        goto free_payload;

    /* populate payload for PARAM_ID_APTX_CLASSIC_SET_SYNC_MODE */
    aptx_sync_mode = (param_id_aptx_classic_sync_mode_payload_t*)calloc(1,
                         sizeof(param_id_aptx_classic_sync_mode_payload_t));
    if (aptx_sync_mode == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }

    /* Sync mode should be DUAL AUTOSYNC (1) for TWS+ */
    aptx_sync_mode->sync_mode = aptx_dm_bt_cfg->sync_mode;
    if (enc_payload->channel_count == 1)
        aptx_sync_mode->startup_mode = 1; /* (L + R)/2 input */
    else
        aptx_sync_mode->startup_mode = 0; /* L and R separate input */

    aptx_sync_mode->cvr_fade_duration = DEFAULT_FADE_DURATION;

    ret = bt_base_populate_enc_cmn_param(blk[1], PARAM_ID_APTX_CLASSIC_SET_SYNC_MODE,
            aptx_sync_mode, sizeof(param_id_aptx_classic_sync_mode_payload_t));
    free(aptx_sync_mode);
    if (ret)
        goto free_payload;

    enc_payload->blocks[0] = blk[0];
    enc_payload->blocks[1] = blk[1];
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

int bt_apt_ad_populate_enc_audiosrc_info(custom_block_t *blk, uint16_t sample_rate)
{
    struct param_id_aptx_adaptive_enc_original_samplerate_t *param = NULL;

    blk->param_id = PARAM_ID_APTX_ADAPTIVE_ENC_AUDIOSRC_INFO;
    blk->payload_sz = sizeof(struct param_id_aptx_adaptive_enc_original_samplerate_t);
    blk->payload = calloc(1, blk->payload_sz);
    if (!blk->payload) {
        blk->payload_sz = 0;
        return -ENOMEM;
    }

    param = (struct param_id_aptx_adaptive_enc_original_samplerate_t*) blk->payload;
    param->original_sample_rate = sample_rate;
    return 0;

}

static int aptx_ad_pack_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_aptx_ad_encoder_config_t *aptx_ad_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    struct param_id_aptx_adaptive_enc_init_t *enc_init = NULL;
    struct param_id_aptx_adaptive_enc_profile_t *enc_profile = NULL;
    int ret = 0, num_blks = 3, i = 0;
    custom_block_t *blk[3] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    aptx_ad_bt_cfg = (audio_aptx_ad_encoder_config_t*)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }
    /* ABR is always enabled for APTx Adaptive */
    enc_payload->is_abr_enabled = true;
    enc_payload->bit_format  = aptx_ad_bt_cfg->bits_per_sample;
    enc_payload->num_blks    = num_blks;

    switch(aptx_ad_bt_cfg->sampling_rate) {
        case APTX_AD_SR_UNCHANGED:
        case APTX_AD_48:
        default:
            enc_payload->sample_rate = SAMPLING_RATE_48K;
            break;
        case APTX_AD_44_1:
            enc_payload->sample_rate = SAMPLING_RATE_44K;
            break;
        case APTX_AD_96:
            enc_payload->sample_rate = SAMPLING_RATE_96K;
            break;
    }

    switch (aptx_ad_bt_cfg->channel_mode) {
        case APTX_AD_CHANNEL_MONO:
            enc_payload->channel_count = 1;
            break;
        default:
            enc_payload->channel_count = 2;
            break;
    }

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_APTX_ADAPTIVE_ENC_INIT */
    enc_init = (struct param_id_aptx_adaptive_enc_init_t*)calloc(1, sizeof(struct param_id_aptx_adaptive_enc_init_t));
    if (enc_init == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }
    enc_init->sampleRate          = aptx_ad_bt_cfg->sampling_rate;
    enc_init->mtulink0            = aptx_ad_bt_cfg->mtu;
    enc_init->channelMode0        = aptx_ad_bt_cfg->channel_mode;
    enc_init->minsinkBufLL        = aptx_ad_bt_cfg->min_sink_modeA;
    enc_init->maxsinkBufLL        = aptx_ad_bt_cfg->max_sink_modeA;
    enc_init->minsinkBufHQ        = aptx_ad_bt_cfg->min_sink_modeB;
    enc_init->maxsinkBufHQ        = aptx_ad_bt_cfg->max_sink_modeB;
    enc_init->minsinkBufTws       = aptx_ad_bt_cfg->min_sink_modeC;
    enc_init->maxsinkBufTws       = aptx_ad_bt_cfg->max_sink_modeC;
    enc_init->profile             = aptx_ad_bt_cfg->encoder_mode;
    enc_init->twsplusDualMonoMode = aptx_ad_bt_cfg->input_mode;
    enc_init->twsplusFadeDuration = aptx_ad_bt_cfg->fade_duration;
    for (int i = 0; i < sizeof(enc_init->sinkCapability); i++)
        enc_init->sinkCapability[i] = aptx_ad_bt_cfg->sink_cap[i];

    ret = bt_base_populate_enc_cmn_param(blk[0], PARAM_ID_APTX_ADAPTIVE_ENC_INIT,
            enc_init, sizeof(struct param_id_aptx_adaptive_enc_init_t));
    free(enc_init);
    if (ret)
        goto free_payload;

    /* populate payload for PARAM_ID_APTX_ADAPTIVE_ENC_PROFILE */
    enc_profile = (struct param_id_aptx_adaptive_enc_profile_t*)calloc(1, sizeof(struct param_id_aptx_adaptive_enc_profile_t));
    if (enc_profile == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }
    enc_profile->primprofile      = 0;
    enc_profile->secprofile       = 0;
    enc_profile->LLModeLatTarget0 = aptx_ad_bt_cfg->TTP_modeA_low;
    enc_profile->LLModeLatTarget1 = aptx_ad_bt_cfg->TTP_modeA_high;
    enc_profile->hQLatTarget0     = aptx_ad_bt_cfg->TTP_modeB_low;
    enc_profile->hQLatTarget1     = aptx_ad_bt_cfg->TTP_modeB_high;

    ret = bt_base_populate_enc_cmn_param(blk[1], PARAM_ID_APTX_ADAPTIVE_ENC_PROFILE,
            enc_profile, sizeof(struct param_id_aptx_adaptive_enc_profile_t));
    free(enc_profile);
    if (ret)
        goto free_payload;

    /* populate payload for PARAM_ID_APTX_ADAPTIVE_ENC_AUDIOSRC_INFO */
    //TODO: hard coded sample rate as 0 - same as system setting
    ret = bt_apt_ad_populate_enc_audiosrc_info(blk[2], 0);
    if (ret)
        goto free_payload;

    enc_payload->blocks[0] = blk[0];
    enc_payload->blocks[1] = blk[1];
    enc_payload->blocks[2] = blk[2];
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

static int aptx_hd_pack_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    audio_aptx_hd_encoder_config_t *aptx_hd_bt_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    int ret = 0, num_blks = 1, i = 0;
    custom_block_t *blk[1] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    aptx_hd_bt_cfg = (audio_aptx_hd_encoder_config_t*)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }
    enc_payload->bit_format    = aptx_hd_bt_cfg->bits_per_sample;
    enc_payload->sample_rate   = aptx_hd_bt_cfg->sampling_rate;
    enc_payload->channel_count = aptx_hd_bt_cfg->channels;
    enc_payload->num_blks      = num_blks;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_REAL_MODULE_ID */
    ret = bt_base_populate_real_module_id(blk[0], MODULE_ID_APTX_HD_ENC);
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

static int aptx_ad_speech_pack_enc_config(bt_codec_t *codec, void *src, void **dst)
{
    param_id_aptx_adaptive_speech_enc_init_t *aptx_ad_speech_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    uint32_t *mode;
    int ret = 0, num_blks = 1, i = 0;
    custom_block_t *blk[1] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    mode = (uint32_t *)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }
    enc_payload->bit_format     = ENCODER_BIT_FORMAT_DEFAULT;
    enc_payload->sample_rate    = SAMPLING_RATE_32K;
    enc_payload->channel_count  = CH_MONO;
    enc_payload->num_blks       = num_blks;
    enc_payload->is_abr_enabled = true;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_APTX_ADAPTIVE_SPEECH_ENC_INIT */
    aptx_ad_speech_cfg = (param_id_aptx_adaptive_speech_enc_init_t *)calloc(1,
                         sizeof(param_id_aptx_adaptive_speech_enc_init_t));
    if (aptx_ad_speech_cfg == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }
    aptx_ad_speech_cfg->speechMode = *mode;
    aptx_ad_speech_cfg->byteSwap = SWAP_ENABLE;

    ret = bt_base_populate_enc_cmn_param(blk[0], PARAM_ID_APTX_ADAPTIVE_SPEECH_ENC_INIT,
            aptx_ad_speech_cfg, sizeof(param_id_aptx_adaptive_speech_enc_init_t));
    free(aptx_ad_speech_cfg);
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

static int aptx_ad_speech_pack_dec_config(bt_codec_t *codec, void *src, void **dst)
{
    param_id_aptx_adaptive_speech_dec_init_t *aptx_ad_speech_cfg = NULL;
    bt_enc_payload_t *enc_payload = NULL;
    uint32_t *mode;
    int ret = 0, num_blks = 1, i = 0;
    custom_block_t *blk[1] = {NULL};

    ALOGV("%s", __func__);
    if ((src == NULL) || (dst == NULL)) {
        ALOGE("%s: invalid input parameters", __func__);
        return -EINVAL;
    }

    mode = (uint32_t *)src;

    enc_payload = (bt_enc_payload_t *)calloc(1, sizeof(bt_enc_payload_t) +
                   num_blks * sizeof(custom_block_t *));
    if (enc_payload == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        return -ENOMEM;
    }
    enc_payload->bit_format     = ENCODER_BIT_FORMAT_DEFAULT;
    enc_payload->sample_rate    = SAMPLING_RATE_32K;
    enc_payload->channel_count  = CH_MONO;
    enc_payload->num_blks       = num_blks;
    enc_payload->is_abr_enabled = true;

    for (i = 0; i < num_blks; i++) {
        blk[i] = (custom_block_t *)calloc(1, sizeof(custom_block_t));
        if (!blk[i]) {
            ret = -ENOMEM;
            goto free_payload;
        }
    }

    /* populate payload for PARAM_ID_APTX_ADAPTIVE_SPEECH_DEC_INIT */
    aptx_ad_speech_cfg = (param_id_aptx_adaptive_speech_dec_init_t *)calloc(1,
                         sizeof(param_id_aptx_adaptive_speech_dec_init_t));
    if (aptx_ad_speech_cfg == NULL) {
        ALOGE("%s: fail to allocate memory", __func__);
        ret = -ENOMEM;
        goto free_payload;
    }
    aptx_ad_speech_cfg->speechMode = *mode;
    aptx_ad_speech_cfg->byteSwap = SWAP_ENABLE;

    ret = bt_base_populate_enc_cmn_param(blk[0], PARAM_ID_APTX_ADAPTIVE_SPEECH_DEC_INIT,
            aptx_ad_speech_cfg, sizeof(param_id_aptx_adaptive_speech_dec_init_t));
    free(aptx_ad_speech_cfg);
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

static int bt_aptx_populate_payload(bt_codec_t *codec, void *src, void **dst)
{
    config_fn_t config_fn = NULL;

    if (codec->payload)
        free(codec->payload);

    switch (codec->codecFmt) {
        case CODEC_TYPE_APTX:
            config_fn = ((codec->direction == ENC) ? &aptx_pack_enc_config :
                                                     NULL);
            break;
        case CODEC_TYPE_APTX_AD:
            config_fn = ((codec->direction == ENC) ? &aptx_ad_pack_enc_config :
                                                     NULL);
            break;
        case CODEC_TYPE_APTX_HD:
            config_fn = ((codec->direction == ENC) ? &aptx_hd_pack_enc_config :
                                                     NULL);
            break;
        case CODEC_TYPE_APTX_DUAL_MONO:
            config_fn = ((codec->direction == ENC) ? &aptx_dual_mono_pack_enc_config :
                                                     NULL);
            break;
        case CODEC_TYPE_APTX_AD_SPEECH:
            config_fn = ((codec->direction == ENC) ? &aptx_ad_speech_pack_enc_config :
                                                     &aptx_ad_speech_pack_dec_config);
            break;
        default:
            ALOGD("%s unsupported codecFmt %d\n", __func__, codec->codecFmt);
    }

    if (config_fn != NULL) {
        return config_fn(codec, src, dst);
    }

    return -EINVAL;
}

static uint64_t bt_aptx_get_decoder_latency(bt_codec_t *codec)
{
    uint32_t latency = 0;

    switch (codec->codecFmt) {
        case CODEC_TYPE_APTX:
        case CODEC_TYPE_APTX_HD:
        case CODEC_TYPE_APTX_AD:
        case CODEC_TYPE_APTX_DUAL_MONO:
        default:
            latency = 200;
            ALOGD("No valid decoder defined, setting latency to %dms", latency);
            break;
    }
    return (uint64_t)latency;
}

static uint64_t bt_aptx_get_encoder_latency(bt_codec_t *codec)
{
    uint32_t latency = 0;

    switch (codec->codecFmt) {
        case CODEC_TYPE_APTX:
            latency = ENCODER_LATENCY_APTX;
            break;
        case CODEC_TYPE_APTX_HD:
            latency = ENCODER_LATENCY_APTX_HD;
            break;
        case CODEC_TYPE_APTX_AD:
            /* for aptx adaptive the latency depends on the mode (HQ/LL) and
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

static uint64_t bt_aptx_get_codec_latency(bt_codec_t *codec)
{

    if (codec->direction == ENC)
        return bt_aptx_get_encoder_latency(codec);
    else
        return bt_aptx_get_decoder_latency(codec);
}

int bt_aptx_query_num_codecs(bt_codec_t *codec __unused) {
    ALOGV("%s", __func__);
    return NUM_CODEC;
}

void bt_aptx_close(bt_codec_t *codec)
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
    bt_codec->plugin_populate_payload  = bt_aptx_populate_payload;
    bt_codec->plugin_get_codec_latency = bt_aptx_get_codec_latency;
    bt_codec->plugin_query_num_codecs  = bt_aptx_query_num_codecs;
    bt_codec->close_plugin  = bt_aptx_close;

    *codec = bt_codec;
    return 0;
}
