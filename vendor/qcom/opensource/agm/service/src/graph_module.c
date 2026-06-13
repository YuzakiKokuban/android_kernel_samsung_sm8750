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
 * Copyright (c) 2022-2023, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AGM: graph_module"

#include <errno.h>
#include <pthread.h>
#include "gsl_intf.h"
#include <agm/graph.h>
#include <agm/graph_module.h>
#include <agm/metadata.h>
#include <agm/utils.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_GRAPH_MODULE
#include <log_utils.h>
#endif

#define MONO 1
#define GET_BITS_PER_SAMPLE(format, bit_width) \
                           (format == AGM_FORMAT_PCM_S24_LE? 32 : bit_width)

/*qfactor should be set to 23 only for 24_3LE and 24_LE formats*/
#define GET_Q_FACTOR(format, bit_width) (bit_width - 1)

static void get_default_channel_map(uint8_t *channel_map, int channels)
{
    switch (channels) {
    case CHANNEL_1:
         channel_map[0] = PCM_CHANNEL_C;
         break;
    case CHANNEL_2:
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         break;
    case CHANNEL_3:
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         channel_map[2] = PCM_CHANNEL_C;
         break;
    case CHANNEL_4:
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         channel_map[2] = PCM_CHANNEL_LB;
         channel_map[3] = PCM_CHANNEL_RB;
         break;
    case CHANNEL_5:
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         channel_map[2] = PCM_CHANNEL_C;
         channel_map[3] = PCM_CHANNEL_LB;
         channel_map[4] = PCM_CHANNEL_RB;
         break;
    case CHANNEL_6:
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         channel_map[2] = PCM_CHANNEL_C;
         channel_map[3] = PCM_CHANNEL_LFE;
         channel_map[4] = PCM_CHANNEL_LB;
         channel_map[5] = PCM_CHANNEL_RB;
         break;
    case CHANNEL_7:
         /*
          * Configured for 5.1 channel mapping + 1 channel for debug
          * Can be customized based on DSP.
          */
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         channel_map[2] = PCM_CHANNEL_C;
         channel_map[3] = PCM_CHANNEL_LFE;
         channel_map[4] = PCM_CHANNEL_LB;
         channel_map[5] = PCM_CHANNEL_RB;
         channel_map[6] = PCM_CHANNEL_CS;
         break;
    case CHANNEL_8:
         channel_map[0] = PCM_CHANNEL_L;
         channel_map[1] = PCM_CHANNEL_R;
         channel_map[2] = PCM_CHANNEL_C;
         channel_map[3] = PCM_CHANNEL_LFE;
         channel_map[4] = PCM_CHANNEL_LB;
         channel_map[5] = PCM_CHANNEL_RB;
         channel_map[6] = PCM_CHANNEL_LS;
         channel_map[7] = PCM_CHANNEL_RS;
    }
}

static bool is_format_pcm(enum agm_media_format fmt_id)
{
    if (fmt_id >= AGM_FORMAT_PCM_S8 && fmt_id <= AGM_FORMAT_PCM_S32_LE)
        return true;

    return false;
}

static bool is_format_bypassed(enum agm_media_format fmt_id)
{
    if (fmt_id == AGM_FORMAT_VORBIS) {
        return true;
    }

    return false;
}

int get_pcm_bit_width(enum agm_media_format fmt_id)
{
    int bit_width = 16;

    switch (fmt_id) {
    case AGM_FORMAT_PCM_S24_3LE:
    /*
     *This api returns the number of audio data bit width specific to the format
     *e.g. In S24_LE, even if the number of bytes is 4, the audio data is only in 3 bytes
     *Hence we return 24 as the bit_width, whereas the bitspersample for this format would
     *return 32
     */
    case AGM_FORMAT_PCM_S24_LE:
         bit_width = 24;
         break;
    case AGM_FORMAT_PCM_S32_LE:
         bit_width = 32;
         break;
    case AGM_FORMAT_PCM_S16_LE:
    default:
         break;
    }

    return bit_width;
}

static int get_media_bit_width(struct session_obj *sess_obj,
                               struct agm_media_config *media_config)
{
    int bit_width = 16;

    if (is_format_pcm(media_config->format))
        return get_pcm_bit_width(media_config->format);

    switch (media_config->format) {
    case AGM_FORMAT_FLAC:
        bit_width = sess_obj->stream_config.codec.flac_dec.sample_size;
        break;
    case AGM_FORMAT_ALAC:
        bit_width = sess_obj->stream_config.codec.alac_dec.bit_depth;
        break;
    case AGM_FORMAT_APE:
        bit_width = sess_obj->stream_config.codec.ape_dec.bit_width;
        break;
    case AGM_FORMAT_WMASTD:
        bit_width = sess_obj->stream_config.codec.wma_dec.bits_per_sample;
        break;
    case AGM_FORMAT_WMAPRO:
        bit_width = sess_obj->stream_config.codec.wmapro_dec.bits_per_sample;
        break;
    case AGM_FORMAT_MP3:
    case AGM_FORMAT_AAC:
    case AGM_FORMAT_OPUS:
    default:
        break;
    }
    AGM_LOGD("%s: bit width: %d", __func__, bit_width);

    return bit_width;
}

static int configure_codec_dma_ep(struct module_info *mod,
                           struct graph_obj *graph_obj)
{
    int ret = 0;
    struct device_obj *dev_obj = mod->dev_obj;
    hw_ep_info_t hw_ep_info = dev_obj->hw_ep_info;
    struct apm_module_param_data_t *header;
    struct param_id_codec_dma_intf_cfg_t* codec_config;
    size_t payload_sz;
    uint8_t *payload = NULL;
    uint32_t *chmap = NULL;
    struct agm_media_config media_config = (dev_obj->group_data) ?
                          dev_obj->group_data->media_config.config :dev_obj->media_config;

    AGM_LOGV("entry mod tag %x miid %x mid %x\n", mod->tag, mod->miid, mod->mid);

    payload_sz = sizeof(struct apm_module_param_data_t) +
        sizeof(struct param_id_codec_dma_intf_cfg_t);

    ALIGN_PAYLOAD(payload_sz, 8);
    payload = (uint8_t*)calloc(1, (size_t)payload_sz);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;
    codec_config = (struct param_id_codec_dma_intf_cfg_t*)
                     (payload + sizeof(struct apm_module_param_data_t));

    ret = device_get_channel_map(dev_obj, &chmap);
    if (ret || chmap == NULL) {
        AGM_LOGE("get channel map failed");
        goto done;
    }

    if (chmap[0] < media_config.channels) {
        AGM_LOGE("Mismatch in num channels, expected %d, received %d",
                  media_config.channels, chmap[0]);
        ret = -EINVAL;
        goto done;
    }

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_CODEC_DMA_INTF_CFG;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_codec_dma_intf_cfg_t);

    codec_config->lpaif_type = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.lpaif_type;
    codec_config->intf_indx = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.intf_idx;
    /* chmap[0] contains num_ch and chmap[1] contains channelmap */
    codec_config->active_channels_mask = chmap[1];

    AGM_LOGD("cdc_dma intf cfg lpaif %d indx %d ch_msk %x",
              codec_config->lpaif_type, codec_config->intf_indx,
              codec_config->active_channels_mask);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_sz);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config for module %d failed with error %d",
                      mod->tag, ret);
    }
done:
    if (chmap)
        free(chmap);

    if (payload)
        free(payload);

    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

static int configure_i2s_ep(struct module_info *mod,
                           struct graph_obj *graph_obj)
{
    int ret = 0;
    struct device_obj *dev_obj = mod->dev_obj;
    hw_ep_info_t hw_ep_info = dev_obj->hw_ep_info;
    struct gsl_key_vector tag_key_vect;
    struct apm_module_param_data_t *header;
    struct  param_id_i2s_intf_cfg_t* i2s_config;
    size_t payload_sz, ret_payload_sz = 0;
    uint8_t *payload = NULL;
    struct agm_media_config media_config = (dev_obj->group_data) ?
                          dev_obj->group_data->media_config.config :dev_obj->media_config;

    AGM_LOGV("entry mod tag %x miid %x mid %x", mod->tag, mod->miid, mod->mid);

    payload_sz = sizeof(struct apm_module_param_data_t) +
        sizeof(struct  param_id_i2s_intf_cfg_t);

    ALIGN_PAYLOAD(payload_sz, 8);
    ret_payload_sz = payload_sz;
    payload = (uint8_t*)calloc(1, (size_t)payload_sz);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;
    i2s_config = (struct  param_id_i2s_intf_cfg_t*)
                     (payload + sizeof(struct apm_module_param_data_t));

    /*
     * For Codec dma we need to configure the following tags
     * 1.Channels  - Channels are reused to derive the active channel mask
     */
    tag_key_vect.num_kvps = 1;
    tag_key_vect.kvp = calloc(tag_key_vect.num_kvps,
                                sizeof(struct gsl_key_value_pair));

    if (!tag_key_vect.kvp) {
        AGM_LOGE("Not enough memory for KVP");
        ret = -ENOMEM;
        goto free_payload;
    }

    tag_key_vect.kvp[0].key = CHANNELS;
    tag_key_vect.kvp[0].value = media_config.channels;

    ret = gsl_get_tagged_data((struct gsl_key_vector *)mod->gkv,
                               mod->tag, &tag_key_vect, (uint8_t *)payload,
                               &ret_payload_sz);

    if (ret != 0) {
        if (ret == AR_ENEEDMORE)
           AGM_LOGE("payload buffer sz %zu smaller than expected size %zu",
                     payload_sz, ret_payload_sz);
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGD("get_tagged_data for module %d failed with error %d",
                      mod->tag, ret);
        goto free_kvp;
    }

    AGM_LOGV("hdr mid %x pid %x er_cd %x param_sz %d",
             header->module_instance_id, header->param_id, header->error_code,
             header->param_size);

    i2s_config->lpaif_type = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.lpaif_type;
    i2s_config->intf_idx = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.intf_idx;

    AGM_LOGV("i2s intf cfg lpaif %d indx %d sd_ln_idx %x ws_src %d",
              i2s_config->lpaif_type, i2s_config->intf_idx,
              i2s_config->sd_line_idx, i2s_config->ws_src);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_sz);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config for module %d failed with error %d",
                      mod->tag, ret);
    }
free_kvp:
    free(tag_key_vect.kvp);
free_payload:
    free(payload);
done:
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}


static int configure_tdm_ep(struct module_info *mod,
                           struct graph_obj *graph_obj)
{
    int ret = 0;
    struct device_obj *dev_obj = mod->dev_obj;
    hw_ep_info_t hw_ep_info = dev_obj->hw_ep_info;
    struct gsl_key_vector tag_key_vect;
    struct apm_module_param_data_t *header;
    struct param_id_tdm_intf_cfg_t* tdm_config;
    size_t payload_sz, ret_payload_sz = 0;
    uint8_t *payload = NULL;
    struct agm_media_config media_config = (dev_obj->group_data) ?
                          dev_obj->group_data->media_config.config :dev_obj->media_config;

    AGM_LOGV("entry mod tag %x miid %x mid %x", mod->tag, mod->miid, mod->mid);

    payload_sz = sizeof(struct apm_module_param_data_t) +
        sizeof(struct param_id_tdm_intf_cfg_t);

    ALIGN_PAYLOAD(payload_sz, 8);
    ret_payload_sz = payload_sz;
    payload = (uint8_t*)calloc(1, (size_t)payload_sz);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;
    tdm_config = (struct  param_id_tdm_intf_cfg_t*)
                     (payload + sizeof(struct apm_module_param_data_t));

    /*
     * For Codec dma we need to configure the following tags
     * 1.Channels  - Channels are reused to derive the active channel mask
     */
    tag_key_vect.num_kvps = 1;
    tag_key_vect.kvp = calloc(tag_key_vect.num_kvps,
                                sizeof(struct gsl_key_value_pair));

    if (!tag_key_vect.kvp) {
        AGM_LOGE("Not enough memory for KVP");
        ret = -ENOMEM;
        goto free_payload;
    }

    tag_key_vect.kvp[0].key = CHANNELS;
    tag_key_vect.kvp[0].value = media_config.channels;

    ret = gsl_get_tagged_data((struct gsl_key_vector *)mod->gkv,
                               mod->tag, &tag_key_vect, (uint8_t *)payload,
                               &ret_payload_sz);

    if (ret != 0) {
        if (ret == AR_ENEEDMORE)
           AGM_LOGE("payload buffer sz %zu smaller than expected size %zu",
                     payload_sz, ret_payload_sz);
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGD("get_tagged_data for module %d failed with error %d",
                      mod->tag, ret);
        goto free_kvp;
    }

    tdm_config->lpaif_type = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.lpaif_type;
    tdm_config->intf_idx = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.intf_idx;

    /* Update slot_mask from AGM only in case of group TDM */
    if (dev_obj->group_data)
        tdm_config->slot_mask = dev_obj->group_data->media_config.slot_mask;

    AGM_LOGV("tdm intf cfg lpaif %d idx %d sync_src %d ctrl_dt_ot_enb %d",
             tdm_config->lpaif_type, tdm_config->intf_idx, tdm_config->sync_src,
             tdm_config->ctrl_data_out_enable);
    AGM_LOGV("slt_msk %x nslts_per_frm %x slt_wdth %d sync_mode %d",
             tdm_config->slot_mask, tdm_config->nslots_per_frame,
             tdm_config->slot_width, tdm_config->sync_mode);
    AGM_LOGV("inv_sync_pulse %d sync_data_delay %d",
             tdm_config->ctrl_invert_sync_pulse, tdm_config->ctrl_sync_data_delay);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_sz);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config for module %d failed with error %d",
                      mod->tag, ret);
    }
free_kvp:
    free(tag_key_vect.kvp);
free_payload:
    free(payload);
done:
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}


static int configure_aux_pcm_ep(struct module_info *mod,
                           struct graph_obj *graph_obj)
{
    int ret = 0;
    struct device_obj *dev_obj = mod->dev_obj;
    hw_ep_info_t hw_ep_info = dev_obj->hw_ep_info;
    struct gsl_key_vector tag_key_vect;
    struct apm_module_param_data_t *header;

    struct param_id_hw_pcm_intf_cfg_t* aux_pcm_cfg;
    size_t payload_sz ,ret_payload_sz = 0;
    uint8_t *payload = NULL;

    AGM_LOGV("entry mod tag %x miid %x mid %x", mod->tag, mod->miid, mod->mid);

    payload_sz = sizeof(struct apm_module_param_data_t) +
        sizeof(struct param_id_hw_pcm_intf_cfg_t);

    ALIGN_PAYLOAD(payload_sz, 8);
    ret_payload_sz = payload_sz;
    payload = (uint8_t*)calloc(1, (size_t)payload_sz);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;
    aux_pcm_cfg = (struct  param_id_hw_pcm_intf_cfg_t*)
                     (payload + sizeof(struct apm_module_param_data_t));

    /*
     * For Codec dma we need to configure the following tags
     * 1.Channels  - Channels are reused to derive the active channel mask
     */
    tag_key_vect.num_kvps = 1;
    tag_key_vect.kvp = calloc(tag_key_vect.num_kvps,
                                sizeof(struct gsl_key_value_pair));

    if (!tag_key_vect.kvp) {
        AGM_LOGE("Not enough memory for KVP");
        ret = -ENOMEM;
        goto free_payload;
    }

    tag_key_vect.kvp[0].key = CHANNELS;
    tag_key_vect.kvp[0].value = dev_obj->media_config.channels;

    ret = gsl_get_tagged_data((struct gsl_key_vector *)mod->gkv,
                               mod->tag, &tag_key_vect, (uint8_t *)payload,
                               &ret_payload_sz);

    if (ret != 0) {
       if (ret == AR_ENEEDMORE)
           AGM_LOGE("payload buffer sz %zu smaller than expected size %zu",
                     payload_sz, ret_payload_sz);
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGD("get_tagged_data for module %d failed with error %d",
                      mod->tag, ret);
        goto free_kvp;
    }

    aux_pcm_cfg->lpaif_type = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.lpaif_type;
    aux_pcm_cfg->intf_idx = hw_ep_info.ep_config.cdc_dma_i2s_tdm_config.intf_idx;

    AGM_LOGV("aux intf cfg lpaif %d idx %d sync_src %d ctrl_dt_ot_enb %d",
             aux_pcm_cfg->lpaif_type, aux_pcm_cfg->intf_idx, aux_pcm_cfg->sync_src,
             aux_pcm_cfg->ctrl_data_out_enable);
    AGM_LOGV("slt_msk %x frm_setting %x aux_mode %d",
             aux_pcm_cfg->slot_mask, aux_pcm_cfg->frame_setting,
             aux_pcm_cfg->aux_mode);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_sz);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config for module %d failed with error %d",
                      mod->tag, ret);
    }
free_kvp:
    free(tag_key_vect.kvp);
free_payload:
    free(payload);
done:
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

static int configure_slimbus_ep(struct module_info *mod,
                           struct graph_obj *graph_obj)
{
    int ret = 0;
    struct device_obj *dev_obj = mod->dev_obj;
    hw_ep_info_t hw_ep_info = dev_obj->hw_ep_info;
    struct apm_module_param_data_t *header;
    struct param_id_slimbus_cfg_t* slimbus_cfg;
    size_t payload_sz;
    uint8_t *payload = NULL;
    int i = 0;
    uint32_t *chmap = NULL;

    AGM_LOGD("entry mod tag %x miid %x mid %x", mod->tag, mod->miid, mod->mid);

    if (dev_obj->media_config.channels > SB_MAX_CHAN_CNT) {
        AGM_LOGE("device channels %d exceed max supported ch %d for Slimbus",
                  dev_obj->media_config.channels, SB_MAX_CHAN_CNT);
        ret = -EINVAL;
        goto done;
   }

    payload_sz = sizeof(struct apm_module_param_data_t) +
        sizeof(struct param_id_slimbus_cfg_t);

    ALIGN_PAYLOAD(payload_sz, 8);
    payload = (uint8_t*)calloc(1, (size_t)payload_sz);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;
    slimbus_cfg = (struct  param_id_slimbus_cfg_t*)
                     (payload + sizeof(struct apm_module_param_data_t));

    ret = device_get_channel_map(dev_obj, &chmap);
    if (ret || chmap == NULL) {
        AGM_LOGE("get channel map failed");
        goto done;
    }

    if (chmap[0] < dev_obj->media_config.channels) {
        AGM_LOGE("Mismatch in num channels, expected %d, received %d",
                 dev_obj->media_config.channels, chmap[0]);
        ret = -EINVAL;
        goto done;
    }

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_SLIMBUS_CONFIG;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_slimbus_cfg_t);
    slimbus_cfg->slimbus_dev_id = hw_ep_info.ep_config.slim_config.dev_id;

    AGM_LOGD("slimbus intf cfg dev id %d ch %d", slimbus_cfg->slimbus_dev_id,
             dev_obj->media_config.channels);
    for (i = 0; i < dev_obj->media_config.channels; i++) {
        slimbus_cfg->shared_channel_mapping[i] = chmap[i + 1];
        AGM_LOGV("shared_chnl_mapping[%d] = 0x%x\n", i, slimbus_cfg->shared_channel_mapping[i]);
    }

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_sz);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config for module %d failed with error %d",
                      mod->tag, ret);
    }
done:
    if (payload)
        free(payload);

    if (chmap)
        free(chmap);

    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

int configure_hw_ep_media_config(struct module_info *mod,
                                struct graph_obj *graph_obj)
{
    int ret = 0;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    struct device_obj *dev_obj = mod->dev_obj;
    struct apm_module_param_data_t* header;
    struct param_id_hw_ep_mf_t* hw_ep_media_conf;
    struct agm_media_config media_config = (dev_obj->group_data) ?
                          dev_obj->group_data->media_config.config :dev_obj->media_config;

    AGM_LOGD("entry mod tag %x miid %x mid %x",mod->tag, mod->miid, mod->mid);
    payload_size = sizeof(struct apm_module_param_data_t) +
                   sizeof(struct param_id_hw_ep_mf_t);

    /*ensure that the payloadszie is byte multiple atleast*/
    ALIGN_PAYLOAD(payload_size, 8);
    payload = calloc(1, (size_t)payload_size);
    if (!payload) {
        AGM_LOGE("No memory to allocate for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;
    hw_ep_media_conf = (struct param_id_hw_ep_mf_t*)
                         (payload + sizeof(struct apm_module_param_data_t));

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_HW_EP_MF_CFG;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_hw_ep_mf_t);

    hw_ep_media_conf->sample_rate = media_config.rate;
    hw_ep_media_conf->bit_width = get_pcm_bit_width(media_config.format);

    hw_ep_media_conf->num_channels = media_config.channels;
    hw_ep_media_conf->data_format = media_config.data_format;

    AGM_LOGD("rate %d bw %d ch %d, data_fmt %d", media_config.rate,
                    hw_ep_media_conf->bit_width, media_config.channels,
                    media_config.data_format);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config command for module %d failed with error %d",
                      mod->tag, ret);
    }
    free(payload);
done:
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

int configure_hw_ep(struct module_info *mod,
                    struct graph_obj *graph_obj)
{
    int ret = 0;
    struct device_obj *dev_obj = mod->dev_obj;

    if ((dev_obj->hw_ep_info.intf == PCM_RT_PROXY) ||
        (dev_obj->hw_ep_info.intf == PCM_DUMMY) ||
        (dev_obj->hw_ep_info.intf == BTFM_PROXY)) {
        AGM_LOGD("no ep media config for %d\n",  dev_obj->hw_ep_info.intf);
    } else {
        ret = configure_hw_ep_media_config(mod, graph_obj);
        if (ret) {
            AGM_LOGE("hw_ep_media_config failed %d", ret);
            return ret;
        }
    }
    switch (dev_obj->hw_ep_info.intf) {
    case CODEC_DMA:
         ret = configure_codec_dma_ep(mod, graph_obj);
         break;
    case MI2S:
         ret = configure_i2s_ep(mod, graph_obj);
         break;
    case AUXPCM:
         ret = configure_aux_pcm_ep(mod, graph_obj);
         break;
    case TDM:
         ret = configure_tdm_ep(mod, graph_obj);
         break;
    case SLIMBUS:
         ret = configure_slimbus_ep(mod, graph_obj);
         break;
    case DISPLAY_PORT:
    case USB_AUDIO:
    case PCM_RT_PROXY:
    case AUDIOSS_DMA:
    case PCM_DUMMY:
    case BTFM_PROXY:
        AGM_LOGD("no ep configuration for %d\n",  dev_obj->hw_ep_info.intf);
        break;
    default:
         AGM_LOGE("hw intf %d not enabled yet", dev_obj->hw_ep_info.intf);
         ret = -EINVAL;
         break;
    }
    return ret;
}

/**
 *PCM DECODER/ENCODER and PCM CONVERTER are configured with the
 *same PCM_FORMAT_CFG hence reuse the implementation
*/
int configure_output_media_format(struct module_info *mod,
                               struct graph_obj *graph_obj)
{
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct media_format_t *media_fmt_hdr;
    struct payload_pcm_output_format_cfg_t *pcm_output_fmt_payload;
    struct apm_module_param_data_t *header;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    uint8_t *channel_map;
    int ret = 0;
    int num_channels = MONO;
    struct agm_media_config media_config = {0};

    /*We use in_media_config for Record usecases or
     *if it is NON_TUNNEL_MODE decode usecase otherwise
     *out_media_config is used to configure PCM_Convertor
     */
    if ((sess_obj->stream_config.dir == TX) ||
        ((sess_obj->stream_config.sess_mode == AGM_SESSION_NON_TUNNEL) &&
        (is_format_pcm(sess_obj->in_media_config.format)))) {
         media_config = sess_obj->in_media_config;
    }
    else
        media_config = sess_obj->out_media_config;

    AGM_LOGD("entry mod tag %x miid %x mid %x",mod->tag, mod->miid, mod->mid);
    num_channels = media_config.channels;

    payload_size = sizeof(struct apm_module_param_data_t) +
                   sizeof(struct media_format_t) +
                   sizeof(struct payload_pcm_output_format_cfg_t) +
                   sizeof(uint8_t)*num_channels;

    /*ensure that the payloadszie is byte multiple atleast*/
    ALIGN_PAYLOAD(payload_size, 8);

    payload = calloc(1, (size_t)payload_size);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }


    header = (struct apm_module_param_data_t*)payload;

    media_fmt_hdr = (struct media_format_t*)(payload +
                         sizeof(struct apm_module_param_data_t));
    pcm_output_fmt_payload = (struct payload_pcm_output_format_cfg_t*)(payload +
                             sizeof(struct apm_module_param_data_t) +
                             sizeof(struct media_format_t));

    channel_map = (uint8_t*)(payload + sizeof(struct apm_module_param_data_t) +
                                sizeof(struct media_format_t) +
                                sizeof(struct payload_pcm_output_format_cfg_t));

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_PCM_OUTPUT_FORMAT_CFG;
    header->error_code = 0x0;
    header->param_size = sizeof(struct media_format_t) +
                         sizeof(struct payload_pcm_output_format_cfg_t) +
                         sizeof(uint8_t)*num_channels;

    media_fmt_hdr->data_format = AGM_DATA_FORMAT_FIXED_POINT;
    media_fmt_hdr->fmt_id = MEDIA_FMT_ID_PCM;
    media_fmt_hdr->payload_size =
                      (uint32_t)(sizeof(payload_pcm_output_format_cfg_t) +
                                    sizeof(uint8_t) * num_channels);

    pcm_output_fmt_payload->endianness = PCM_LITTLE_ENDIAN;

    pcm_output_fmt_payload->bit_width = get_media_bit_width(sess_obj, &media_config);

    /**
     *alignment field is referred to only in case where bit width is
     *24 and bits per sample is 32, tiny alsa only supports 24 bit
     *in 32 word size in LSB aligned mode(AGM_FORMAT_PCM_S24_LE).
     *Hence we hardcode this to PCM_LSB_ALIGNED;
     */
    pcm_output_fmt_payload->alignment = PCM_LSB_ALIGNED;
    pcm_output_fmt_payload->num_channels = num_channels;

    if (((sess_obj->stream_config.dir == TX) ||
         ((sess_obj->stream_config.sess_mode == AGM_SESSION_NON_TUNNEL) && mod->module == MODULE_PCM_CONVERTER)) &&
         is_format_pcm(media_config.format)) {
        /*for PCM capture usecase, we want native data to be captured hence
         *configure pcm convertor accordingly
         *Also in case of Non Tunnel Mode decode, we configure PCM_CONVERTOR with
         *media config same as what is configured by client on the read path.
         *We have added the module check above to ensure that the decoder
         *PCM_OUTPUT_FORMAT_CFG is configured in the else part.
         */
        pcm_output_fmt_payload->bits_per_sample =
                             GET_BITS_PER_SAMPLE(media_config.format,
                                                 pcm_output_fmt_payload->bit_width);

        pcm_output_fmt_payload->q_factor =
                             GET_Q_FACTOR(media_config.format,
                                          pcm_output_fmt_payload->bit_width);
         if (sess_obj->stream_config.sess_mode == AGM_SESSION_NON_TUNNEL)
            /*
             * Setting num channels to native mode for PCM convetter
             * so that channels always match upstream decoder module
             * channel configuration
             */
             pcm_output_fmt_payload->num_channels = PARAM_VAL_NATIVE;
    } else {
        switch (pcm_output_fmt_payload->bit_width) {
        case 16:
        case 32:
             pcm_output_fmt_payload->bits_per_sample =
                                     pcm_output_fmt_payload->bit_width;
             pcm_output_fmt_payload->q_factor =
                                     pcm_output_fmt_payload->bit_width - 1;
             break;
        case 24:
             /*
              *modules after pcm convertor only work on 16 or 32bit samples hence
              *even for 24 bit input data configure pcm convertor output with
              *32 bits per sample also to accomodate post processing, SPF
              *tean recommends to set the q_factor as 27 even for 24_LE/24_3LE formats
              *This is to be done only for pcm_convertor on the playback path
              */
             pcm_output_fmt_payload->bits_per_sample = 32;
             pcm_output_fmt_payload->q_factor = 27;
             break;
        default:
             AGM_LOGE("wrong bitwidth %d", pcm_output_fmt_payload->bit_width);
             ret = -EINVAL;
             goto done;
        }
    }

    if (sess_obj->stream_config.dir == RX)
        pcm_output_fmt_payload->interleaved = PCM_DEINTERLEAVED_UNPACKED;
    else
        pcm_output_fmt_payload->interleaved = PCM_INTERLEAVED;

    /**
     *#TODO:As of now channel_map is not part of media_config
     *ADD channel map part as part of the session/device media config
     *structure and use that channel map if set by client otherwise
     * use the default channel map
     */
    get_default_channel_map(channel_map, num_channels);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config command for module %d failed with error %d",
                      mod->tag, ret);
    }
done:
    if (payload) {
        free(payload);
    }
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

static int configure_pcm_encoder_frame_size(struct module_info *mod,
                                    struct graph_obj *graph_obj,
                                    uint32_t frame_size_samples)
{
    struct apm_module_param_data_t *header;
    struct param_id_pcm_encoder_frame_size_t *frame_size_payload;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    int ret = 0;

    payload_size = sizeof(struct apm_module_param_data_t) +
                   sizeof(struct param_id_pcm_encoder_frame_size_t);
    payload = calloc(1, (size_t)payload_size);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        return -ENOMEM;
    }
    header = (struct apm_module_param_data_t*)payload;
    frame_size_payload = (struct param_id_pcm_encoder_frame_size_t*)(payload +
                            sizeof(struct apm_module_param_data_t));
    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_PCM_ENCODER_FRAME_SIZE;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_pcm_encoder_frame_size_t);

    frame_size_payload->frame_size_type = 1; /* frame_size_in_samples */
    frame_size_payload->frame_size_in_samples = frame_size_samples;

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("pcm encoder frame size config for module %d failed with error %d",
                      mod->tag, ret);
    }
    free(payload);
    return ret;
}

int configure_pcm_encoder_params(struct module_info *mod,
                                struct graph_obj *graph_obj)
{
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    uint32_t samples_per_msec = 0, frame_size = 0;
    uint32_t channels = MONO, bits = 16;

    /* configure output media format */
    ret = configure_output_media_format(mod, graph_obj);
    if (ret)
        return ret;

    /* configure pcm encoder frame size */
    if (sess_obj->stream_config.sess_mode != AGM_SESSION_NON_TUNNEL) {
        samples_per_msec = sess_obj->in_media_config.rate/1000;
        channels = sess_obj->in_media_config.channels;
        bits = get_pcm_bit_width(sess_obj->in_media_config.format);
        bits = GET_BITS_PER_SAMPLE(sess_obj->in_media_config.format, bits);
        channels = (channels == 0) ? MONO : channels;
        frame_size = (sess_obj->in_buffer_config.size * 8) /
                        (channels * bits);

        if (samples_per_msec &&
            (((frame_size * 1000) % sess_obj->in_media_config.rate) != 0)) {
            AGM_LOGD("pcm encoder: frame_size %d\n", frame_size);
            ret = configure_pcm_encoder_frame_size(mod, graph_obj, frame_size);
        }
    }

    return ret;
}

static int get_media_fmt_id_and_size(enum agm_media_format fmt_id,
                              size_t *payload_size, size_t *real_fmt_id)
{
    int ret = 0;
    size_t format_size = 0;

    if (is_format_pcm(fmt_id)) {
        format_size = sizeof(struct payload_media_fmt_pcm_t);;
        *real_fmt_id = MEDIA_FMT_PCM;
        goto pl_size;
    }

    switch (fmt_id) {
    case AGM_FORMAT_MP3:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_MP3;
        break;
    case AGM_FORMAT_AAC:
        format_size = sizeof(struct payload_media_fmt_aac_t);
        *real_fmt_id = MEDIA_FMT_AAC;
        break;
    case AGM_FORMAT_FLAC:
        format_size = sizeof(struct payload_media_fmt_flac_t);
        *real_fmt_id = MEDIA_FMT_FLAC;
        break;
    case AGM_FORMAT_ALAC:
        format_size = sizeof(struct payload_media_fmt_alac_t);
        *real_fmt_id = MEDIA_FMT_ALAC;
        break;
    case AGM_FORMAT_APE:
        format_size = sizeof(struct payload_media_fmt_ape_t);
        *real_fmt_id = MEDIA_FMT_APE;
        break;
    case AGM_FORMAT_WMASTD:
        format_size = sizeof(struct payload_media_fmt_wmastd_t);
        *real_fmt_id = MEDIA_FMT_WMASTD;
        break;
    case AGM_FORMAT_WMAPRO:
        format_size = sizeof(struct payload_media_fmt_wmapro_t);
        *real_fmt_id = MEDIA_FMT_WMAPRO;
        break;
    case AGM_FORMAT_OPUS:
        format_size = sizeof(struct payload_media_fmt_opus_t);
        *real_fmt_id = MEDIA_FMT_OPUS;
        break;
    case AGM_FORMAT_VORBIS:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_VORBIS;
        break;
    case AGM_FORMAT_AMR_NB:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_AMRNB;
        break;
    case AGM_FORMAT_AMR_WB:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_AMRWB;
        break;
    case AGM_FORMAT_AMR_WB_PLUS:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_AMRWBPLUS;
        break;
    case AGM_FORMAT_EVRC:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_EVRC;
        break;
    case AGM_FORMAT_G711:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_G711;
        break;
    case AGM_FORMAT_QCELP:
        format_size = 0;
        *real_fmt_id = MEDIA_FMT_QCELP;
        break;
    default:
        ret = -EINVAL;
        break;
    }

pl_size:
    *payload_size = sizeof(struct apm_module_param_data_t) +
                   sizeof(struct media_format_t) + format_size;

    return ret;
}


int  set_compressed_media_format(enum agm_media_format fmt_id,
                       struct media_format_t *media_fmt_hdr,
                       struct session_obj *sess_obj)
{
    int ret = 0;
    size_t fmt_size = 0;

    switch (fmt_id) {
    case AGM_FORMAT_MP3:
    {
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_MP3;
        media_fmt_hdr->payload_size = 0;
        break;
    }
    case AGM_FORMAT_AAC:
    {
        struct payload_media_fmt_aac_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_aac_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_AAC;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl =
        (struct payload_media_fmt_aac_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));

        memcpy(fmt_pl, &sess_obj->stream_config.codec.aac_dec,
               fmt_size);
        AGM_LOGD("AAC payload: fmt:%d, Obj_type:%d, ch:%d, SR:%d",
                 fmt_pl->aac_fmt_flag, fmt_pl->audio_obj_type,
                 fmt_pl->num_channels, fmt_pl->sample_rate);
        break;
    }
    case AGM_FORMAT_FLAC:
    {
        struct payload_media_fmt_flac_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_flac_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_FLAC;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl = (struct payload_media_fmt_flac_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));
        memcpy(fmt_pl, &sess_obj->stream_config.codec.flac_dec,
               fmt_size);
        AGM_LOGD("FLAC payload: ch:%d, sample_size:%d, SR:%d",
                 fmt_pl->num_channels, fmt_pl->sample_size, fmt_pl->sample_rate);
        break;
    }
    case AGM_FORMAT_ALAC:
    {
        struct payload_media_fmt_alac_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_alac_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_ALAC;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl = (struct payload_media_fmt_alac_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));
        memcpy(fmt_pl, &sess_obj->stream_config.codec.alac_dec,
               fmt_size);
        AGM_LOGD("ALAC payload: bit_depth:%d, ch:%d, SR:%d",
                 fmt_pl->bit_depth, fmt_pl->num_channels,
                 fmt_pl->sample_rate);
        break;
    }
    case AGM_FORMAT_APE:
    {
        struct payload_media_fmt_ape_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_ape_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_APE;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl = (struct payload_media_fmt_ape_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));
        memcpy(fmt_pl, &sess_obj->stream_config.codec.ape_dec,
               fmt_size);
        AGM_LOGD("APE payload: bit_width:%d, ch:%d, SR:%d",
                 fmt_pl->bit_width, fmt_pl->num_channels,
                 fmt_pl->sample_rate);
        break;
    }
    case AGM_FORMAT_WMASTD:
    {
        struct payload_media_fmt_wmastd_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_wmastd_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_WMASTD;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl = (struct payload_media_fmt_wmastd_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));
        memcpy(fmt_pl, &sess_obj->stream_config.codec.wma_dec,
               fmt_size);
        AGM_LOGD("WMA payload: fmt:%d, ch:%d, SR:%d, bit_width:%d",
                  fmt_pl->fmt_tag, fmt_pl->num_channels,
                 fmt_pl->sample_rate, fmt_pl->bits_per_sample);
        break;
    }
    case AGM_FORMAT_WMAPRO:
    {
        struct payload_media_fmt_wmapro_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_wmapro_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_WMAPRO;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl = (struct payload_media_fmt_wmapro_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));
        memcpy(fmt_pl, &sess_obj->stream_config.codec.wmapro_dec,
               fmt_size);
        AGM_LOGD("WMAPro payload: fmt:%d, ch:%d, SR:%d, bit_width:%d",
                  fmt_pl->fmt_tag, fmt_pl->num_channels,
                 fmt_pl->sample_rate, fmt_pl->bits_per_sample);
        break;
    }
    case AGM_FORMAT_OPUS:
    {
        struct payload_media_fmt_opus_t *fmt_pl;
        fmt_size = sizeof(struct payload_media_fmt_opus_t);
        media_fmt_hdr->data_format = AGM_DATA_FORMAT_RAW_COMPRESSED;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_OPUS;
        media_fmt_hdr->payload_size = fmt_size;

        fmt_pl = (struct payload_media_fmt_opus_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));
        memcpy(fmt_pl, &sess_obj->stream_config.codec.opus_dec,
               fmt_size);
        AGM_LOGD("OPUS payload: bitstream_format:%d payload_type:%d version:%d, ch:%d, "
                 "pre_skip:%d, SR:%d, output_gain:%d, mapping_family:%d, stream_count:%d, "
                 "coupled_count:%d",
                 fmt_pl->bitstream_format, fmt_pl->payload_type, fmt_pl->version,
                 fmt_pl->num_channels, fmt_pl->pre_skip,
                 fmt_pl->sample_rate, fmt_pl->output_gain,
                 fmt_pl->mapping_family, fmt_pl->stream_count,
                 fmt_pl->coupled_count);
        AGM_LOGD("channel map:");
        for (int i = 0; i < fmt_pl->num_channels; i++) {
            AGM_LOGD("%d", fmt_pl->channel_map[i]);
        }
        break;
    }
    default:
        return -EINVAL;
    }

    return ret;
}

/**
 *Configure placeholder decoder
*/
int configure_placeholder_dec(struct module_info *mod,
                              struct graph_obj *graph_obj)
{
    int ret = 0;
    struct gsl_key_vector tkv;
    struct session_obj *sess_obj = NULL;
    size_t payload_size = 0, real_fmt_id = 0;

// { SEC_AUDIO_ADD_FOR_DEBUG        
    AGM_LOGI("enter");
// } SEC_AUDIO_ADD_FOR_DEBUG
    memset(&tkv, 0, sizeof(struct gsl_key_vector));
    if (graph_obj == NULL) {
        AGM_LOGE("invalid graph object");
        goto done;
    }
    sess_obj = graph_obj->sess_obj;
    /* 1. Configure placeholder decoder with Real ID */
    ret = get_media_fmt_id_and_size(sess_obj->out_media_config.format,
                                    &payload_size, &real_fmt_id);
    if (ret) {
        AGM_LOGD("module is not configured for format: %d\n",
                 sess_obj->out_media_config.format);
        /* If ret is non-zero then placeholder module would be
         * configured by client so return from here.
         */
        ret = 0;
        goto done;
    }

    tkv.num_kvps = 1;
    tkv.kvp = calloc(tkv.num_kvps, sizeof(struct gsl_key_value_pair));
    if (!tkv.kvp) {
        AGM_LOGE("Not enough memory for tkv.kvp\n");
        ret = -ENOMEM;
        goto done;
    }
    tkv.kvp->key = MEDIA_FMT_ID;
    tkv.kvp->value = real_fmt_id;

    AGM_LOGD("Placeholder mod TKV key:%0x value: %0x", tkv.kvp->key,
             tkv.kvp->value);
    ret = gsl_set_config(graph_obj->graph_handle, (struct gsl_key_vector *)mod->gkv,
                         TAG_STREAM_PLACEHOLDER_DECODER, &tkv);

    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("set_config command failed with error: %d", ret);
        goto done;
    }

    /* 2. Set output media format cfg for placeholder decoder */
    ret = configure_output_media_format(mod, graph_obj);
    if (ret != 0)
        AGM_LOGE("output_media_format cfg failed: %d", ret);

done:
    if (tkv.kvp)
        free(tkv.kvp);
// { SEC_AUDIO_ADD_FOR_DEBUG        
    AGM_LOGI("exit, ret %d", ret);
// } SEC_AUDIO_ADD_FOR_DEBUG
    return ret;
}

int configure_aac_encoder(struct module_info *mod,
                                          struct graph_obj *graph_obj){
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct apm_module_param_data_t *header;
    struct param_id_aac_enc_cutoff_freq_config_t *cut_off_frequency;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    size_t size_apm_module = sizeof(struct apm_module_param_data_t);
    size_t size_cutoff_freq_config =
        sizeof(struct param_id_aac_enc_cutoff_freq_config_t);
    size_t size_apm_and_size_cutoff_freq =
        size_apm_module + size_cutoff_freq_config;

    if (sess_obj->stream_config.codec.aac_enc.global_cutoff_freq <= 0){
        AGM_LOGD("AAC cutoff frequency ignored or not configured");
        return 0;
    }

    /**
     * For Encoder module, PARAM_ID_AAC_CUTOFF_FREQ_CONFIG
     * configuration: optional
     * */
    payload_size = size_apm_and_size_cutoff_freq;
    /*ensure that the payloadszie is byte multiple atleast*/
    ALIGN_PAYLOAD(payload_size, 8);
    payload = (uint8_t *)calloc(1, payload_size);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto exit;
    }
    header = (struct apm_module_param_data_t *)(payload);
    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_AAC_CUTOFF_FREQ_CONFIG;
    header->error_code = 0x0;
    header->param_size = size_cutoff_freq_config;
    cut_off_frequency =
        (struct param_id_aac_enc_cutoff_freq_config_t *)(payload +
                                                         size_apm_module);
    cut_off_frequency->freq_cutoff_mode = 1;  // global cutoff frquency
    cut_off_frequency->global_cutoff_freq =
        sess_obj->stream_config.codec.aac_enc.global_cutoff_freq;

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE(
            "custom_config command for module %d with "
            "PARAM_ID_AAC_CUTOFF_FREQ_CONFIG failed with error %d",
            mod->tag, ret);
        goto exit;
    } else {
        AGM_LOGD("AAC cutoff frequency requested: %d",
                 cut_off_frequency->global_cutoff_freq);
        free(payload);
        payload = NULL;
    }

exit:
    if (payload) {
        free(payload);
        payload = NULL;
    }
    AGM_LOGV("Exit: %d", ret);
    return ret;
}

/**
 *  codec specific encoder config only for TX type
 */
int configure_encoder_output_media_format(struct module_info *mod,
                                          struct graph_obj *graph_obj) {
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct apm_module_param_data_t *header;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    size_t size_apm_module = sizeof(struct apm_module_param_data_t);
    size_t size_encoder_config =
        sizeof(struct param_id_encoder_output_config_t);
    size_t size_apm_and_encoder_config = size_apm_module + size_encoder_config;
    AGM_LOGV("Enter");

    /**
     * For Encoder module, PARAM_ID_ENCODER_OUTPUT_CONFIG
     * */

    /**
     * only TX session, hence in media config
     * */
    switch (sess_obj->in_media_config.format) {
        case AGM_FORMAT_AAC: {
            size_t size_aac_cfg = sizeof(struct aac_enc_cfg_t);
            payload_size = size_apm_and_encoder_config + size_aac_cfg;

            /*ensure that the payloadszie is byte multiple atleast*/
            ALIGN_PAYLOAD(payload_size, 8);
            payload = (uint8_t *)calloc(1, payload_size);
            if (!payload) {
                AGM_LOGE("Not enough memory for payload");
                ret = -ENOMEM;
                goto err;
            }

            struct param_id_encoder_output_config_t enc_out_conf_param;
            enc_out_conf_param.data_format = AGM_DATA_FORMAT_FIXED_POINT;
            enc_out_conf_param.fmt_id = MEDIA_FMT_AAC;
            enc_out_conf_param.payload_size = size_aac_cfg;
            memcpy(payload + size_apm_module, &enc_out_conf_param,
                   size_encoder_config);

            memcpy(payload + size_apm_and_encoder_config,
                   &(sess_obj->stream_config.codec.aac_enc.enc_cfg),
                   size_aac_cfg);
            break;
        }

        default:
            ret = -EINVAL;
            goto err;
            break;
    }

    header = (struct apm_module_param_data_t *)(payload);
    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_ENCODER_OUTPUT_CONFIG;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_encoder_output_config_t);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE(
            "custom_config command for module %d with "
            "PARAM_ID_ENCODER_OUTPUT_CONFIG failed with error %d",
            mod->tag, ret);
        goto err;
    } else {
        struct aac_enc_cfg_t *aac_cfg =
            (struct aac_enc_cfg_t *)(payload + size_apm_and_encoder_config);
        AGM_LOGD("AAC encode mode: 0x%x, fmt_flag: 0x%x", aac_cfg->enc_mode,
                 aac_cfg->aac_fmt_flag);
        free(payload);
        payload = NULL;
    }

    /**
     * For Encoder Module, PARAM_ID_ENC_BITRATE
     * */
    switch (sess_obj->in_media_config.format) {
        case AGM_FORMAT_AAC: {
            struct param_id_enc_bitrate_param_t *bitrate_param = NULL;
            size_t bitrate_param_size =
                sizeof(struct param_id_enc_bitrate_param_t);
            payload_size = size_apm_module + bitrate_param_size;
            /*ensure that the payloadszie is byte multiple atleast*/
            ALIGN_PAYLOAD(payload_size, 8);
            payload = (uint8_t *)calloc(1, payload_size);
            if (!payload) {
                AGM_LOGE("Not enough memory for payload");
                ret = -ENOMEM;
                goto err;
            }

            bitrate_param =
                (struct param_id_enc_bitrate_param_t *)(payload +
                                                        size_apm_module);
            bitrate_param->bitrate =
                sess_obj->stream_config.codec.aac_enc.aac_bit_rate;
            break;
        }

        default:
            ret = -EINVAL;
            goto err;
            break;
    }
    header = (struct apm_module_param_data_t *)(payload);
    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_ENC_BITRATE;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_enc_bitrate_param_t);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE(
            "custom_config command for module %d with "
            "PARAM_ID_ENC_BITRATE failed with error %d",
            mod->tag, ret);
        goto err;
    } else {
        struct param_id_enc_bitrate_param_t *bitrate_param =
            (struct param_id_enc_bitrate_param_t *)(payload + size_apm_module);
        AGM_LOGD("AAC bitrate: %d", bitrate_param->bitrate);
        free(payload);
        payload = NULL;
    }

    if (sess_obj->in_media_config.format == AGM_FORMAT_AAC) {
        ret = configure_aac_encoder(mod, graph_obj);
    }

err:
done:
    if (payload) {
        free(payload);
        payload = NULL;
    }
    AGM_LOGV("Exit: %d", ret);
    return ret;
}

/**
 *Configure placeholder encoder
*/
int configure_placeholder_enc(struct module_info *mod,
                              struct graph_obj *graph_obj)
{
    int ret = 0;
    struct gsl_key_vector tkv;
    struct session_obj *sess_obj = NULL;

    size_t payload_size = 0, real_fmt_id = 0;

    AGM_LOGE("enter");
    if (graph_obj == NULL) {
        AGM_LOGE("invalid graph object");
        return -EINVAL;
    }
    sess_obj = graph_obj->sess_obj;

    /* 1. Configure placeholder encoder with Real ID */
    ret = get_media_fmt_id_and_size(sess_obj->in_media_config.format,
                                    &payload_size, &real_fmt_id);
    if (ret) {
        AGM_LOGD("module is not configured for format: %d\n",
                 sess_obj->in_media_config.format);
        /* If ret is non-zero then placeholder module would be
         * configured by client so return from here.
         */
        return 0;
    }

    tkv.num_kvps = 1;
    tkv.kvp = calloc(tkv.num_kvps, sizeof(struct gsl_key_value_pair));
    if (!tkv.kvp) {
        AGM_LOGE("Not enough memory for tkv.kvp\n");
        return -ENOMEM;
    }
    tkv.kvp->key = MEDIA_FMT_ID;
    tkv.kvp->value = real_fmt_id;

    AGM_LOGD("Placeholder mod TKV key:%0x value: %0x", tkv.kvp->key,
             tkv.kvp->value);
    ret = gsl_set_config(graph_obj->graph_handle, (struct gsl_key_vector *)mod->gkv,
                         TAG_STREAM_PLACEHOLDER_ENCODER, &tkv);

    if (tkv.kvp)
        free(tkv.kvp);

    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("set_config command failed with error: %d", ret);
        return ret;
    }

    /* configure only in case of compress capture */
    if (sess_obj->stream_config.sess_mode == AGM_SESSION_COMPRESS &&
        sess_obj->stream_config.dir == TX) {
        ret = configure_encoder_output_media_format(mod, graph_obj);
        if (ret != 0)
            AGM_LOGE("configure_encoder_output_media_format failed: %d", ret);
    }

    return ret;
}


int configure_compress_shared_mem_ep(struct module_info *mod,
                            struct graph_obj *graph_obj)
{
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct media_format_t *media_fmt_hdr;
    struct apm_module_param_data_t *header;
    uint8_t *payload = NULL;
    size_t payload_size = 0, real_fmt_id = 0;
    size_t actual_param_sz = 0;

    if (is_format_bypassed(sess_obj->out_media_config.format) ||
        sess_obj->stream_config.sess_mode == AGM_SESSION_NON_TUNNEL) {
        AGM_LOGI("bypass shared mem ep config for format %x or sess_mode %d",
                 sess_obj->out_media_config.format, sess_obj->stream_config.sess_mode);
        return 0;
    }

    ret = get_media_fmt_id_and_size(sess_obj->out_media_config.format,
                                    &payload_size, &real_fmt_id);
    if (ret) {
        AGM_LOGD("module is not configured for format: %d\n",
                 sess_obj->out_media_config.format);
        /* If ret is non-zero then shared memory module would be
         * configured by client so return from here.
         */
        return 0;
    }
    /*
     *We get the actual size of the PID being sent before the payload_size
     *is updated for 8 byte alignment.
     */
    actual_param_sz = payload_size - sizeof(struct apm_module_param_data_t);

    /*ensure that the payloadsize is byte multiple atleast*/
    ALIGN_PAYLOAD(payload_size, 8);

    payload = calloc(1, (size_t)payload_size);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload\n");
        return -ENOMEM;
    }

    header = (struct apm_module_param_data_t*)payload;

    media_fmt_hdr = (struct media_format_t*)(payload +
                         sizeof(struct apm_module_param_data_t));

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = actual_param_sz;

    ret =  set_compressed_media_format(sess_obj->out_media_config.format,
                             media_fmt_hdr, sess_obj);
    if (ret) {
        AGM_LOGD("Shared mem EP is not configured for format: %d\n",
                 sess_obj->out_media_config.format);
        /* If ret is non-zero then shared memory module would be
         * configured by client so return from here.
         */
        goto free_payload;
    }

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config command for module %d failed with error %d",
                      mod->tag, ret);
    }

free_payload:
    free(payload);
    return ret;
}

static int configure_compress_shared_mem_ep_datapath(struct module_info *mod,
                            struct graph_obj *graph_obj)
{
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct media_format_t *media_fmt_hdr;
    uint8_t *payload = NULL;
    size_t payload_size = 0, real_fmt_id = 0;
    struct agm_buff buffer = {0};
    size_t consumed_size = 0;

    if (is_format_bypassed(sess_obj->out_media_config.format) ||
        sess_obj->stream_config.sess_mode == AGM_SESSION_NON_TUNNEL) {
        AGM_LOGI("bypass shared mem ep config for format %x or sess_mode %d",
                 sess_obj->out_media_config.format, sess_obj->stream_config.sess_mode);
        return 0;
    }

    ret = get_media_fmt_id_and_size(sess_obj->out_media_config.format,
                                    &payload_size, &real_fmt_id);
    if (ret) {
        AGM_LOGD("module is not configured for format: %d\n",
                 sess_obj->out_media_config.format);
        /* If ret is non-zero then shared memory module would be
         * configured by client so return from here.
         */
        return 0;
    }

    payload_size = payload_size - sizeof(struct apm_module_param_data_t);
    media_fmt_hdr = (struct media_format_t *) calloc(1, (size_t)payload_size);
    if (!media_fmt_hdr) {
        AGM_LOGE("Not enough memory for payload\n");
        return -ENOMEM;
    }


    buffer.timestamp = 0x0;
    buffer.flags = AGM_BUFF_FLAG_MEDIA_FORMAT;
    buffer.size = payload_size;
    buffer.addr = (uint8_t *)media_fmt_hdr;

    ret =  set_compressed_media_format(sess_obj->out_media_config.format,
                             media_fmt_hdr, sess_obj);
    if (ret) {
        AGM_LOGD("Shared mem EP is not configured for format: %d\n",
                 sess_obj->out_media_config.format);
        /* If ret is non-zero then shared memory module would be
         * configured by client so return from here.
         */
        goto free_payload;
    }

    ret = graph_write(graph_obj, &buffer, &consumed_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config command for module %d failed with error %d",
                      mod->tag, ret);
    }

free_payload:
    free(media_fmt_hdr);
    return ret;
}

int configure_pcm_shared_mem_ep(struct module_info *mod,
                            struct graph_obj *graph_obj)
{
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct media_format_t *media_fmt_hdr;
    struct apm_module_param_data_t *header;
    struct payload_media_fmt_pcm_t *media_fmt_payload;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    int num_channels = MONO;
    uint8_t *channel_map;

    AGM_LOGD("entry mod tag %x miid %x mid %x",mod->tag, mod->miid, mod->mid);
    num_channels = sess_obj->out_media_config.channels;

    payload_size = sizeof(struct apm_module_param_data_t) +
                   sizeof(struct media_format_t) +
                   sizeof(struct payload_media_fmt_pcm_t) +
                   sizeof(uint8_t)*num_channels;

    /*ensure that the payloadszie is byte multiple atleast*/
    ALIGN_PAYLOAD(payload_size, 8);

    payload = calloc(1,(size_t)payload_size);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;

    media_fmt_hdr = (struct media_format_t*)(payload +
                         sizeof(struct apm_module_param_data_t));
    media_fmt_payload = (struct payload_media_fmt_pcm_t*)(payload +
                             sizeof(struct apm_module_param_data_t) +
                             sizeof(struct media_format_t));

    channel_map = (uint8_t*)(payload + sizeof(struct apm_module_param_data_t) +
                                sizeof(struct media_format_t) +
                                sizeof(struct payload_media_fmt_pcm_t));

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = sizeof(struct media_format_t) +
                         sizeof(struct payload_media_fmt_pcm_t) +
                         sizeof(uint8_t)*num_channels;

    media_fmt_hdr->data_format = AGM_DATA_FORMAT_FIXED_POINT;
    media_fmt_hdr->fmt_id = MEDIA_FMT_ID_PCM;
    media_fmt_hdr->payload_size = (uint32_t)(sizeof(payload_media_fmt_pcm_t) +
                                     sizeof(uint8_t) * num_channels);

    media_fmt_payload->endianness = PCM_LITTLE_ENDIAN;
    media_fmt_payload->bit_width = get_pcm_bit_width(sess_obj->out_media_config.format);
    media_fmt_payload->sample_rate = sess_obj->out_media_config.rate;
    /**
     *alignment field is referred to only in case where bit width is
     *24 and bits per sample is 32, tiny alsa only supports 24 bit
     *in 32 word size in LSB aligned mode(AGM_FORMAT_PCM_S24_LE).
     *Hence we hardcode this to PCM_LSB_ALIGNED;
     */
    media_fmt_payload->alignment = PCM_LSB_ALIGNED;
    media_fmt_payload->num_channels = num_channels;
    media_fmt_payload->bits_per_sample =
                             GET_BITS_PER_SAMPLE(sess_obj->out_media_config.format,
                                                 media_fmt_payload->bit_width);
    media_fmt_payload->q_factor = GET_Q_FACTOR(sess_obj->out_media_config.format,
                                               media_fmt_payload->bit_width);
    /**
     *#TODO:As of now channel_map is not part of media_config
     *ADD channel map part as part of the session/device media config
     *structure and use that channel map if set by client otherwise
     * use the default channel map
     */
    get_default_channel_map(channel_map, num_channels);

    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config command for module %d failed with error %d",
                      mod->tag, ret);
    }
done:
    if (payload) {
        free(payload);
    }
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}

int configure_wr_shared_mem_ep(struct module_info *mod,
                            struct graph_obj *graph_obj)
{
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;

    /*
     *This is for the write shared mem endpoint,
     *hence we use out media config.
     */
    if (is_format_pcm(sess_obj->out_media_config.format))
        ret = configure_pcm_shared_mem_ep(mod, graph_obj);
    else {
        if (graph_obj->state != STARTED)
            ret = configure_compress_shared_mem_ep(mod, graph_obj);
        else
            ret = configure_compress_shared_mem_ep_datapath(mod, graph_obj);
    }

    if (ret)
        return ret;

    return 0;
}

int configure_rd_shared_mem_ep(struct module_info *mod,
                            struct graph_obj *graph_obj)
{
    int ret = 0;
    struct session_obj *sess_obj = graph_obj->sess_obj;
    struct apm_module_param_data_t *header;
    struct param_id_rd_sh_mem_cfg_t *rd_sh_mem_cfg;
    uint8_t *payload = NULL;
    size_t payload_size = 0;

    AGM_LOGD("Enter");

    /*
     *Note: read shared mem ep is configured in case of non-tunnel
     *decode sessions where the client has configured the session with
     *AGM_SESSION_FLAG_INBAND_SRCM flag
     *In case of non-tunnel encode sessions, we set the num_frames_per_buff cfg
     *as a part of calibration itself.
     *In case of compress capture case also , read shared mem ep is configured.
     */
    if (!(sess_obj->stream_config.sess_flags & AGM_SESSION_FLAG_INBAND_SRCM) &&
        !(sess_obj->stream_config.sess_mode == AGM_SESSION_COMPRESS))
        goto done;

    AGM_LOGD("entry mod tag %x miid %x mid %x sess_flags %x",mod->tag, mod->miid, mod->mid,
              sess_obj->stream_config.sess_flags);

    payload_size = sizeof(struct apm_module_param_data_t) +
                    sizeof(struct param_id_rd_sh_mem_cfg_t);

    /*ensure that the payloadszie is byte multiple atleast*/
    ALIGN_PAYLOAD(payload_size, 8);

    payload = calloc(1,(size_t)payload_size);
    if (!payload) {
        AGM_LOGE("Not enough memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    header = (struct apm_module_param_data_t*)payload;

    rd_sh_mem_cfg = (struct param_id_rd_sh_mem_cfg_t *)(payload
                       + sizeof(struct apm_module_param_data_t));

    header->module_instance_id = mod->miid;
    header->param_id = PARAM_ID_RD_SH_MEM_CFG;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_rd_sh_mem_cfg_t);

    /*
     *In NT mode session in_media config represents config for data being captured
     *Hence for NT Mode decode it would mean PCM data.
     */
    if (sess_obj->stream_config.sess_mode == AGM_SESSION_NON_TUNNEL) {
        rd_sh_mem_cfg->metadata_control_flags = 0x2; /*ENABLE_MEDIA_FORMAT_MD*/
        if (is_format_pcm(sess_obj->in_media_config.format))
            rd_sh_mem_cfg->num_frames_per_buffer = 0x0; /*As many frames as possible*/
        else
        /*TODO:This is encode usecase hence ideally client wont enable SRCM event
            *Even if the client wants it enabled, then we configure 1 frame every for
            *every read call;
            */
            rd_sh_mem_cfg->num_frames_per_buffer = 0x1;
    }

    /* For compress capture */
    if (sess_obj->stream_config.sess_mode == AGM_SESSION_COMPRESS &&
        sess_obj->stream_config.dir == TX) {
        rd_sh_mem_cfg->num_frames_per_buffer = 0x1;
        AGM_LOGD("compress capture uses 1 frame per buffer");
    }


    ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
    if (ret != 0) {
        ret = ar_err_get_lnx_err_code(ret);
        AGM_LOGE("custom_config command for module %d failed with error %d",
                      mod->tag, ret);
    }
    free(payload);
done:
    AGM_LOGD("exit, ret %d", ret);
    return ret;
}


int configure_spr(struct module_info *spr_mod,
                            struct graph_obj *graph_obj)
{
    int ret = 0;

    struct listnode *node = NULL;
    struct module_info *mod;
    struct apm_module_param_data_t *header;
    struct param_id_spr_delay_path_end_t *spr_hwep_delay;
    uint8_t *payload = NULL;
    size_t payload_size = 0;

    AGM_LOGD("SPR module IID %x", spr_mod->miid);
    graph_obj->spr_miid = spr_mod->miid;
    payload_size = sizeof(struct apm_module_param_data_t) +
                    sizeof(struct param_id_spr_delay_path_end_t);
    ALIGN_PAYLOAD(payload_size, 8);

    payload = calloc(1, (size_t)payload_size);
    if (!payload) {
        AGM_LOGE("No memory to allocate for payload");
        ret = -ENOMEM;
        goto done;
    }
    header = (struct apm_module_param_data_t*)payload;
    spr_hwep_delay = (struct param_id_spr_delay_path_end_t *)(payload
                          + sizeof(struct apm_module_param_data_t));
    header->module_instance_id = spr_mod->miid;
    header->param_id = PARAM_ID_SPR_DELAY_PATH_END;
    header->error_code = 0x0;
    header->param_size = sizeof(struct param_id_spr_delay_path_end_t);

    list_for_each(node, &graph_obj->tagged_mod_list) {
        mod = node_to_item(node, module_info_t, list);
        if (mod->tag == DEVICE_HW_ENDPOINT_RX) {
            AGM_LOGD("HW EP module IID %x", mod->miid);
            spr_hwep_delay->module_instance_id = mod->miid;
            ret = gsl_set_custom_config(graph_obj->graph_handle, payload, payload_size);
            if (ret !=0) {
                ret = ar_err_get_lnx_err_code(ret);
                AGM_LOGE("graph_set_custom_config failed %d", ret);
            }
        }
    }
done:
    if (payload)
        free(payload);
    return ret;
}

int configure_gapless(struct module_info *gapless_mod,
                            struct graph_obj *gph_obj)
{
    int ret = 0;
    struct gsl_cmd_register_custom_event *reg_ev_payload = NULL;
    size_t payload_size = 0;

    AGM_LOGD("GAPLESS module \n");

    if (gph_obj->graph_handle == NULL) {
        pthread_mutex_unlock(&gph_obj->lock);
        AGM_LOGE("invalid graph handle\n");
        ret = -EINVAL;
        goto done;
    }
    payload_size = sizeof(struct gsl_cmd_register_custom_event);

    reg_ev_payload = calloc(1, payload_size);
    if (reg_ev_payload == NULL) {
        pthread_mutex_unlock(&gph_obj->lock);
        AGM_LOGE("calloc failed for reg_ev_payload\n");
        ret = -ENOMEM;
        goto done;
    }

    AGM_LOGD("GAPLESS module IID = %d\n", gapless_mod->miid);

    reg_ev_payload->event_id = EVENT_ID_EARLY_EOS;
    // Write shared memory end point module IID
    reg_ev_payload->module_instance_id = gapless_mod->miid;
    // No payload for early eos registration
    reg_ev_payload->event_config_payload_size = 0;
    reg_ev_payload->is_register = 1;

    ret = gsl_ioctl(gph_obj->graph_handle, GSL_CMD_REGISTER_CUSTOM_EVENT,
                                           reg_ev_payload, payload_size);
    if (ret != 0) {
       ret = ar_err_get_lnx_err_code(ret);
       AGM_LOGE("Early EOS event registration failed with error %d\n", ret);
    }

done:
    if (reg_ev_payload)
        free(reg_ev_payload);
    return ret;
}


module_info_t stream_module_list[] = {
    {
        .module = MODULE_PCM_ENCODER,
        .tag = STREAM_PCM_ENCODER,
        .configure = configure_pcm_encoder_params,
    },
    {
        .module = MODULE_PCM_DECODER,
        .tag = STREAM_PCM_DECODER,
        .configure = configure_output_media_format,
    },
    {
        .module = MODULE_PLACEHOLDER_ENCODER,
        .tag = TAG_STREAM_PLACEHOLDER_ENCODER,
        .configure = configure_placeholder_enc,
    },
    {
        .module = MODULE_PLACEHOLDER_DECODER,
        .tag = TAG_STREAM_PLACEHOLDER_DECODER,
        .configure = configure_placeholder_dec,
    },
    {
        .module = MODULE_PCM_CONVERTER,
        .tag = STREAM_PCM_CONVERTER,
        .configure = configure_output_media_format,
    },
    {
        .module = MODULE_WR_SHARED_MEM,
        .tag = STREAM_INPUT_MEDIA_FORMAT,
        .configure = configure_wr_shared_mem_ep,
    },
    {
        .module = MODULE_STREAM_PAUSE,
        .tag = TAG_PAUSE,
        .configure = NULL,
    },
    {
        .module = MODULE_STREAM_SPR,
        .tag = TAG_STREAM_SPR,
        .configure = configure_spr,
    },
    {
        .module = MODULE_STREAM_GAPLESS,
        .tag = MODULE_GAPLESS,
        .configure = configure_gapless,
    },
    {
        .module = MODULE_RD_SHARED_MEM,
        .tag = RD_SHMEM_ENDPOINT,
        .configure = configure_rd_shared_mem_ep,
    },
};

module_info_t hw_ep_module[] = {
    {
        .module = MODULE_HW_EP_RX,
        .tag = DEVICE_HW_ENDPOINT_RX,
        .configure = configure_hw_ep,
    },
    {
        .module = MODULE_HW_EP_TX,
        .tag = DEVICE_HW_ENDPOINT_TX,
        .configure = configure_hw_ep,
    }
};

void get_stream_module_list_array(module_info_t **info, size_t *size)
{
    *size = sizeof(stream_module_list);
    *info = stream_module_list;
}

void get_hw_ep_module_list_array(module_info_t **info, size_t *size)
{
    *size = sizeof(hw_ep_module);
    *info =  hw_ep_module;
}
