/*
** Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
**/
#define LOG_TAG "PLUGIN: compress"

#include <agm/agm_api.h>
#include <errno.h>
#include <limits.h>
#include <linux/ioctl.h>
#include <time.h>
#include <sound/asound.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <tinycompress/compress_plugin.h>
#include <tinycompress/tinycompress.h>
#include <snd-card-def.h>
#include <tinyalsa/asoundlib.h>
#include <sound/compress_params.h>
#include <sound/compress_offload.h>
#include <agm/utils.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_COMPRESS_PLUGIN
#include <log_utils.h>
#endif

/* Default values */
#define COMPR_PLAYBACK_MIN_FRAGMENT_SIZE (8 * 1024)
#define COMPR_PLAYBACK_MAX_FRAGMENT_SIZE (128 * 1024)
#define COMPR_PLAYBACK_MIN_NUM_FRAGMENTS (4)
#define COMPR_PLAYBACK_MAX_NUM_FRAGMENTS (16)

struct agm_compress_priv {
    struct agm_media_config media_config;
    struct agm_buffer_config buffer_config;
    struct agm_session_config session_config;
    struct snd_compr_caps compr_cap;
    uint64_t handle;
    bool prepared;
    uint64_t bytes_copied; /* Copied to DSP buffer */
    uint64_t total_buf_size; /* Total buffer size */

    int64_t bytes_avail; /* avail size to write/read */

    uint64_t bytes_received;  /* from DSP */
    uint64_t bytes_read;  /* Consumed by client */
    bool start_drain;
    bool eos;
    bool early_eos;
    bool eos_received;
    bool internal_unblock_eos;
    bool internal_unblock_early_eos;
    bool internal_unblock_write;

    enum agm_gapless_silence_type type;   /* Silence Type (Initial/Trailing) */
    uint32_t silence;  /* Samples to remove */

    void *client_data;
    void *card_node;
    int session_id;
    pthread_cond_t drain_cond;
    pthread_mutex_t drain_lock;
    pthread_cond_t eos_cond;
    pthread_mutex_t eos_lock;
    pthread_cond_t early_eos_cond;
    pthread_mutex_t early_eos_lock;
    pthread_mutex_t lock;
    pthread_cond_t poll_cond;
    pthread_mutex_t poll_lock;
};

void agm_session_update_codec_options(struct agm_session_config*, struct snd_compr_params *);

static int agm_get_session_handle(struct agm_compress_priv *priv,
                                  uint64_t *handle)
{
    if (!priv)
        return -EINVAL;

    *handle = priv->handle;
    if (!*handle)
        return -EINVAL;

    return 0;
}

void agm_compress_event_cb(uint32_t session_id __unused,
                           struct agm_event_cb_params *event_params,
                           void *client_data)
{
    struct compress_plugin *agm_compress_plugin = client_data;
    struct agm_compress_priv *priv;

    if (!agm_compress_plugin) {
        AGM_LOGE("%s: client_data is NULL\n", __func__);
        return;
    }
    priv = agm_compress_plugin->priv;
    if (!priv) {
        AGM_LOGE("%s: Private data is NULL\n", __func__);
        return;
    }
    if (!event_params) {
        AGM_LOGE("%s: event params is NULL\n", __func__);
        return;
    }

    pthread_mutex_lock(&priv->lock);
    AGM_LOGV("%s: enter: bytes_avail = %lld, event_id = %d\n", __func__,
             (long long) priv->bytes_avail, event_params->event_id);
    if (event_params->event_id == AGM_EVENT_WRITE_DONE) {
        /*
         * Write done cb is expected for every DSP write with
         * fragment size even for partial buffers
         */
        priv->bytes_avail += priv->buffer_config.size;
        if (priv->bytes_avail > priv->total_buf_size) {
            AGM_LOGE("%s: Error: bytes_avail %lld, total size = %llu\n",
                   __func__, priv->bytes_avail, (unsigned long long) priv->total_buf_size);
            pthread_mutex_unlock(&priv->lock);
            return;
        }

        pthread_mutex_lock(&priv->drain_lock);
        if (priv->start_drain &&
            (priv->bytes_avail == priv->total_buf_size))
            pthread_cond_signal(&priv->drain_cond);
        pthread_mutex_unlock(&priv->drain_lock);
    } else if (event_params->event_id == AGM_EVENT_READ_DONE) {
        /* Read done cb expected for every DSP read with Fragment size */
        priv->bytes_avail += priv->buffer_config.size;
        priv->bytes_received += priv->buffer_config.size;
    } else if (event_params->event_id == AGM_EVENT_EOS_RENDERED) {
        AGM_LOGD("%s: EOS event received \n", __func__);
        /* Unblock eos wait if all the buffers are rendered */
        pthread_mutex_lock(&priv->eos_lock);
        if (priv->eos) {
            pthread_cond_signal(&priv->eos_cond);
            priv->eos = false;
        } else {
            AGM_LOGD("%s: EOS received before drain called\n", __func__);
            priv->eos_received = true;
        }
        pthread_mutex_unlock(&priv->eos_lock);
    } else if (event_params->event_id == AGM_EVENT_EARLY_EOS) {
        AGM_LOGD("%s: Early EOS event received \n", __func__);
        /* Unblock early eos wait */
        pthread_mutex_lock(&priv->early_eos_lock);
        if (priv->early_eos) {
            priv->early_eos = false;
            pthread_cond_signal(&priv->early_eos_cond);
        }
        pthread_mutex_unlock(&priv->early_eos_lock);
    } else if (event_params->event_id == AGM_EVENT_EARLY_EOS_INTERNAL) {
        AGM_LOGD("%s: Early EOS event received from internal unblock\n", __func__);
        /* Unblock early eos wait */
        pthread_mutex_lock(&priv->early_eos_lock);
        if (priv->early_eos) {
            priv->early_eos = false;
            pthread_cond_signal(&priv->early_eos_cond);
            priv->internal_unblock_early_eos = true;
        }
        pthread_mutex_unlock(&priv->early_eos_lock);
    } else {
        AGM_LOGE("%s: error: Invalid event params id: %d\n", __func__,
           event_params->event_id);
    }
    pthread_mutex_unlock(&priv->lock);
    /* Signal Poll */
    pthread_mutex_lock(&priv->poll_lock);
    pthread_cond_signal(&priv->poll_cond);
    pthread_mutex_unlock(&priv->poll_lock);
}

int agm_compress_write(struct compress_plugin *plugin, const void *buff,
                            size_t count)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;
    int64_t size = count, buf_cnt;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (priv->eos_received)
        priv->eos_received = false;

    if (count > priv->total_buf_size) {
        AGM_LOGE("%s: Size %zu is greater than total buf size %llu\n",
               __func__, count, (unsigned long long) priv->total_buf_size);
        return -EINVAL;
    }

    /* Call prepare in the first write as write()
        will be called before start() */
    if (!priv->prepared) {
        ret = agm_session_prepare(handle);
        if (ret) {
            errno = ret;
            return ret;
        }
        priv->prepared = true;
    }

    ret = agm_session_write(handle, (void *)buff, (size_t*)&size);
    if (ret) {
        errno = ret;
        return ret;
    }

    pthread_mutex_lock(&priv->lock);
    buf_cnt = size / priv->buffer_config.size;
    if (size % priv->buffer_config.size != 0)
        buf_cnt +=1;

    /* Avalible buffer size is always multiple of fragment size */
    priv->bytes_avail -= (buf_cnt * priv->buffer_config.size);
    if (priv->bytes_avail < 0) {
        AGM_LOGE("%s: err: bytes_avail = %lld", __func__, (long long) priv->bytes_avail);
        ret = -EINVAL;
        goto err;
    }
    AGM_LOGV("%s: count = %zu, priv->bytes_avail: %lld\n",
                     __func__, count, (long long) priv->bytes_avail);
    priv->bytes_copied += size;
    ret = size;
err:
    pthread_mutex_unlock(&priv->lock);

    return ret;
}

int agm_compress_read(struct compress_plugin *plugin, void *buff, size_t count)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0, buf_cnt = 0;
    AGM_LOGV("Enter");

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (count > priv->bytes_avail) {
        AGM_LOGE("%s: Invalid requested size %zu", __func__, count);
        return -EINVAL;
    }

    ret = agm_session_read(handle, buff, &count);
    if (ret < 0) {
        errno = ret;
        return ret;
    }
    pthread_mutex_lock(&priv->lock);

    priv->bytes_read += count;

    pthread_mutex_unlock(&priv->lock);
    AGM_LOGV("Exit: read bytes: %d",count);
    return count;
}

int agm_compress_tstamp(struct compress_plugin *plugin,
                       struct snd_compr_tstamp *tstamp)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;
    uint64_t timestamp = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    tstamp->sampling_rate = priv->media_config.rate;
    tstamp->copied_total = priv->bytes_copied;

    /*
     * for compress capture get latest buffer timestamp
     * otherwise in case of playback use get session time
     * which inturn queries SPR module in playback graph.
     */
    if (priv->session_config.dir == TX) {
        ret = agm_get_buffer_timestamp(priv->session_id, &timestamp);
    } else {
        ret = agm_get_session_time(handle, &timestamp);
    }

    if (ret) {
        errno = ret;
        return ret;
    }
    timestamp *= tstamp->sampling_rate;
    tstamp->pcm_io_frames = timestamp/1000000;

    return 0;
}

int agm_compress_avail(struct compress_plugin *plugin,
                        struct snd_compr_avail *avail)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    agm_compress_tstamp(plugin, &avail->tstamp);

    pthread_mutex_lock(&priv->lock);
    /* Avail size is always in multiples of fragment size */
    avail->avail = priv->bytes_avail;
    AGM_LOGV("%s: size = %zu, *avail = %llu, pcm_io_frames: %d \
             sampling_rate: %u\n", __func__,
             sizeof(struct snd_compr_avail), avail->avail,
             avail->tstamp.pcm_io_frames,
             avail->tstamp.sampling_rate);
    pthread_mutex_unlock(&priv->lock);

    return ret;
}

int agm_compress_get_caps(struct compress_plugin *plugin,
                             struct snd_compr_caps *caps)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (caps) {
        memcpy(caps, &priv->compr_cap, sizeof(struct snd_compr_caps));
    } else {
        ret = -EINVAL;
        return ret;
    }

    return 0;
}

int agm_session_update_codec_config(struct agm_compress_priv *priv,
                                    struct snd_compr_params *params)
{
    struct agm_media_config *media_cfg;
    struct agm_session_config *sess_cfg;
    union snd_codec_options *copt;

    media_cfg = &priv->media_config;
    sess_cfg = &priv->session_config;
    copt = &params->codec.options;

    media_cfg->rate =  params->codec.sample_rate;
    media_cfg->channels = params->codec.ch_out;

    if (sess_cfg->dir == RX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_MP3:
            media_cfg->format = AGM_FORMAT_MP3;
            break;
        case SND_AUDIOCODEC_AAC:
            media_cfg->format = AGM_FORMAT_AAC;
            if (params->codec.format == SND_AUDIOSTREAMFORMAT_MP4LATM)
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x04;
            else if (params->codec.format == SND_AUDIOSTREAMFORMAT_ADIF)
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x02;
            else if (params->codec.format == SND_AUDIOSTREAMFORMAT_MP4ADTS)
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x00;
            else
                sess_cfg->codec.aac_dec.aac_fmt_flag = 0x03;
            sess_cfg->codec.aac_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.aac_dec.sample_rate = media_cfg->rate;
            break;
        case SND_AUDIOCODEC_FLAC:
            media_cfg->format = AGM_FORMAT_FLAC;
            sess_cfg->codec.flac_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.flac_dec.sample_rate = media_cfg->rate;
            break;
    #ifdef SND_AUDIOCODEC_ALAC
        case SND_AUDIOCODEC_ALAC:
            media_cfg->format = AGM_FORMAT_ALAC;
            sess_cfg->codec.alac_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.alac_dec.sample_rate = media_cfg->rate;
            break;
    #endif
    #ifdef SND_AUDIOCODEC_APE
        case SND_AUDIOCODEC_APE:
            media_cfg->format = AGM_FORMAT_APE;
            sess_cfg->codec.ape_dec.num_channels = params->codec.ch_in;
            sess_cfg->codec.ape_dec.sample_rate = media_cfg->rate;
            break;
    #endif
        case SND_AUDIOCODEC_WMA:
    #ifdef SND_AUDIOPROFILE_WMA9_LOSSLESS
            if ((params->codec.profile == SND_AUDIOPROFILE_WMA9_PRO) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA9_LOSSLESS) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA10_LOSSLESS)) {
    #else
            if ((params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM0) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM1) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM2)) {
    #endif
                media_cfg->format = AGM_FORMAT_WMAPRO;
                sess_cfg->codec.wmapro_dec.fmt_tag = params->codec.format;
                sess_cfg->codec.wmapro_dec.num_channels = params->codec.ch_in;
                sess_cfg->codec.wmapro_dec.sample_rate = media_cfg->rate;
            } else {
                media_cfg->format = AGM_FORMAT_WMASTD;
                sess_cfg->codec.wma_dec.fmt_tag = params->codec.format;
                sess_cfg->codec.wma_dec.num_channels = params->codec.ch_in;
                sess_cfg->codec.wma_dec.sample_rate = media_cfg->rate;
            }
            break;
        case SND_AUDIOCODEC_VORBIS:
            media_cfg->format = AGM_FORMAT_VORBIS;
            break;
        case SND_AUDIOCODEC_BESPOKE:
            if (copt->generic.reserved[0] == AGM_FORMAT_OPUS) {
                media_cfg->format = AGM_FORMAT_OPUS;
                sess_cfg->codec.opus_dec.num_channels = params->codec.ch_in;
                sess_cfg->codec.opus_dec.sample_rate = media_cfg->rate;
            }
            break;
        default:
            break;
        }
    }

    // capture path
    if (sess_cfg->dir == TX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_AAC:
            media_cfg->format = AGM_FORMAT_AAC;

            sess_cfg->codec.aac_enc.aac_bit_rate = params->codec.bit_rate;
            sess_cfg->codec.aac_enc.global_cutoff_freq =
                params->codec.rate_control;
            sess_cfg->codec.aac_enc.enc_cfg.aac_enc_mode =
                params->codec.profile;
            sess_cfg->codec.aac_enc.enc_cfg.aac_fmt_flag = params->codec.format;

            AGM_LOGD(
                "%s: requested configuration, AAC encode mode: %x, AAC format "
                "flag: %x, AAC bit rate: %d, global_cutoff_freq: %d",
                __func__, sess_cfg->codec.aac_enc.enc_cfg.aac_enc_mode,
                sess_cfg->codec.aac_enc.enc_cfg.aac_fmt_flag,
                sess_cfg->codec.aac_enc.aac_bit_rate,
                sess_cfg->codec.aac_enc.global_cutoff_freq);
            break;

        default:
            break;
        }
    }
    agm_session_update_codec_options(sess_cfg, params);

    AGM_LOGD("%s: format = %d rate = %d, channels = %d\n", __func__,
           media_cfg->format, media_cfg->rate, media_cfg->channels);
    return 0;
}

int agm_compress_set_params(struct compress_plugin *plugin,
                                    struct snd_compr_params *params)
{
    struct agm_compress_priv *priv = plugin->priv;
    struct agm_buffer_config *buf_cfg;
    struct agm_session_config *sess_cfg;
    uint64_t handle;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    buf_cfg = &priv->buffer_config;
    buf_cfg->count = params->buffer.fragments;
    buf_cfg->size = params->buffer.fragment_size;
    /*explicitly set the metadata size to 0 */
    buf_cfg->max_metadata_size = 0;
    priv->total_buf_size = buf_cfg->size * buf_cfg->count;

    sess_cfg = &priv->session_config;

    if (sess_cfg->dir == RX)
        priv->bytes_avail = priv->total_buf_size;
    else
        priv->bytes_avail = priv->total_buf_size;

    sess_cfg->start_threshold = 0;
    sess_cfg->stop_threshold = 0;

    if (sess_cfg->dir == RX)
        sess_cfg->data_mode = AGM_DATA_NON_BLOCKING;
    else
        sess_cfg->data_mode = AGM_DATA_BLOCKING;

    /* Populate each codec format specific params */
    ret = agm_session_update_codec_config(priv, params);
    if (ret)
        return ret;

    ret = agm_session_set_config(priv->handle, sess_cfg,
                                 &priv->media_config, buf_cfg);
    if (ret)
        return ret;

    AGM_LOGD("%s: exit fragments cnt = %d size = %zu\n", __func__,
           buf_cfg->count, buf_cfg->size);
    return ret;
}

static int agm_compress_set_metadata(struct compress_plugin *plugin,
                                     struct snd_compr_metadata *metadata)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (metadata->key == SNDRV_COMPRESS_ENCODER_PADDING) {
        priv->type = TRAILING_SILENCE;
        priv->silence = metadata->value[0];
    }
    if (metadata->key == SNDRV_COMPRESS_ENCODER_DELAY) {
        priv->type = INITIAL_SILENCE;
        priv->silence = metadata->value[0];
    }

    ret = agm_set_gapless_session_metadata(handle, priv->type,
                                           priv->silence);
    if (ret)
       ALOGE("%s: failed to send gapless metadata ret = %d\n", __func__, ret);

    return ret;
}

static int agm_compress_start(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    /**
     * Unlike playback, for capture case, call
     * agm_session_prepare it in start.
     * For playback it is called in write.
     * */
    if (!priv->prepared) {
        ret = agm_session_prepare(handle);
        if (ret) {
            errno = ret;
            return ret;
        }
        priv->prepared = true;
    }

    ret = agm_session_start(handle);
    if (ret)
        errno = ret;
    return ret;
}

static int agm_compress_stop(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    /* Unlock drain if its waiting for EOS rendered */
    pthread_mutex_lock(&priv->eos_lock);
    if (priv->eos) {
        pthread_cond_signal(&priv->eos_cond);
        priv->internal_unblock_eos = true;
        priv->eos = false;
    }
    priv->eos_received = true;
    pthread_mutex_unlock(&priv->eos_lock);

    /* Unblock eos wait if early eos event cb has not been called */
    pthread_mutex_lock(&priv->early_eos_lock);
    if (priv->early_eos) {
        pthread_cond_signal(&priv->early_eos_cond);
        priv->internal_unblock_early_eos = true;
        priv->early_eos = false;
    }
    pthread_mutex_unlock(&priv->early_eos_lock);

    /* Signal Poll */
    pthread_mutex_lock(&priv->poll_lock);
    pthread_cond_signal(&priv->poll_cond);
    priv->internal_unblock_write = true;
    pthread_mutex_unlock(&priv->poll_lock);

    ret = agm_session_stop(handle);
    if (ret) {
        errno = ret;
        return ret;
    }
    /* stop will reset all the buffers and it called during seek also */
    priv->bytes_avail = priv->total_buf_size;
    priv->bytes_copied = 0;

    return ret;
}

static int agm_compress_pause(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_pause(handle);
    return ret;
}

static int agm_compress_resume(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_resume(handle);
    return ret;
}

static int agm_compress_drain(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    AGM_LOGV("%s: priv->bytes_avail = %lld,  priv->total_buf_size = %llu\n",
           __func__, (long long) priv->bytes_avail, (unsigned long long) priv->total_buf_size);
    /* No need to wait for all buffers to be consumed to issue EOS as
     * write and EOS cmds are sequential
     */
    /* TODO: how to handle wake up in SSR scenario */
    pthread_mutex_lock(&priv->eos_lock);
    if (!priv->eos_received) {
        priv->eos = true;
        ret = agm_session_eos(handle);
        if (ret) {
            AGM_LOGE("%s: EOS fail\n", __func__);
            errno = ret;
            pthread_mutex_unlock(&priv->eos_lock);
            return ret;
        }
        pthread_cond_wait(&priv->eos_cond, &priv->eos_lock);
        AGM_LOGD("%s: out of eos wait\n", __func__);
        if (priv->internal_unblock_eos) {
            AGM_LOGD("%s: out of eos wait, internally unblocked\n", __func__);
            priv->internal_unblock_eos = false;
            ret = -ECANCELED;
            errno = ret;
        }
    }
    priv->eos_received = false;
    pthread_mutex_unlock(&priv->eos_lock);

    return ret;
}

static int agm_compress_partial_drain(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    // Send EOS command and wait for EARLY EOS event
    ret = agm_session_eos(handle);
    pthread_mutex_lock(&priv->early_eos_lock);
    if (!priv->eos_received) {
        priv->early_eos = true;
        if (ret) {
            AGM_LOGE("%s: EOS fail\n", __func__);
            pthread_mutex_unlock(&priv->early_eos_lock);
            return ret;
        }
        pthread_cond_wait(&priv->early_eos_cond, &priv->early_eos_lock);
        AGM_LOGD("%s: out of early eos wait\n", __func__);
        if (priv->internal_unblock_early_eos) {
            AGM_LOGD("%s: out of early eos wait, internally unblocked\n", __func__);
            priv->internal_unblock_early_eos = false;
            ret = -ECANCELED;
            errno = ret;
        }
    }
    pthread_mutex_unlock(&priv->early_eos_lock);

    AGM_LOGV("%s: exit\n", __func__);
    return ret;
}

static int agm_compress_next_track(struct compress_plugin *plugin)
{

    AGM_LOGE("%s: next track \n", __func__);

    return 0;
}

static int agm_compress_ioctl(struct compress_plugin *plugin, int cmd, ...)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;
    va_list ap;
    void *arg;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case SNDRV_COMPRESS_SET_METADATA:
        ret = agm_compress_set_metadata(plugin, arg);
        break;
    default:
        break;
    }

    return ret;
}

static int agm_compress_poll(struct compress_plugin *plugin,
                             struct pollfd *fds, nfds_t nfds __unused,
                             int timeout)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    struct timespec poll_ts;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    clock_gettime(CLOCK_MONOTONIC, &poll_ts);
    poll_ts.tv_sec += timeout/1000;
    /* Unblock poll wait if avail bytes to write/read is more than one fragment */
    pthread_mutex_lock(&priv->poll_lock);
    /* If timeout is -1 then its infinite wait */
    if (timeout < 0)
        ret = pthread_cond_wait(&priv->poll_cond, &priv->poll_lock);
    else
        ret = pthread_cond_timedwait(&priv->poll_cond, &priv->poll_lock, &poll_ts);

    if (priv->internal_unblock_write) {
        AGM_LOGD("%s: out of early eos wait, internally unblocked\n", __func__);
        priv->internal_unblock_write = false;
        ret = -ECANCELED;
        errno = ret;
        pthread_mutex_unlock(&priv->poll_lock);
        goto exit;
    }

    pthread_mutex_unlock(&priv->poll_lock);

    if (ret == ETIMEDOUT) {
        /* Poll() expects 0 return value in case of timeout */
        ret = 0;
    } else {
        fds->revents |= POLLOUT;
        ret = POLLOUT;
    }
exit :
    return ret;
}

void agm_compress_close(struct compress_plugin *plugin)
{
    struct agm_compress_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;

    AGM_LOGV("%s:%d\n", __func__, __LINE__);
    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return;

    if (priv->session_config.dir == RX) {
        ret = agm_session_register_cb(priv->session_id, NULL,
                                    AGM_EVENT_DATA_PATH, plugin);
        ret = agm_session_register_cb(priv->session_id, NULL,
                                    AGM_EVENT_MODULE, plugin);
    }

    ret = agm_session_close(handle);
    if (ret)
        AGM_LOGE("%s: agm_session_close failed \n", __func__);

    snd_card_def_put_card(priv->card_node);
    /* Unblock eos wait if eos-rendered event cb has not been called */
    pthread_mutex_lock(&priv->eos_lock);
    if (priv->eos) {
        pthread_cond_signal(&priv->eos_cond);
        priv->internal_unblock_eos = true;
        priv->eos = false;
    }
    pthread_mutex_unlock(&priv->eos_lock);

    /* Unblock eos wait if early eos event cb has not been called */
    pthread_mutex_lock(&priv->early_eos_lock);
    if (priv->early_eos) {
        pthread_cond_signal(&priv->early_eos_cond);
        priv->internal_unblock_early_eos = true;
        priv->early_eos = false;
    }
    pthread_mutex_unlock(&priv->early_eos_lock);

    /* Signal Poll */
    pthread_mutex_lock(&priv->poll_lock);
    pthread_cond_signal(&priv->poll_cond);
    priv->internal_unblock_write = true;
    pthread_mutex_unlock(&priv->poll_lock);

    /* Make sure callbacks are not running at this point */
    free(plugin->priv);
    free(plugin);

    return;
}

struct compress_plugin_ops agm_compress_ops = {
    .close = agm_compress_close,
    .get_caps = agm_compress_get_caps,
    .set_params = agm_compress_set_params,
    .avail = agm_compress_avail,
    .tstamp = agm_compress_tstamp,
    .write = agm_compress_write,
    .read = agm_compress_read,
    .start = agm_compress_start,
    .stop = agm_compress_stop,
    .pause = agm_compress_pause,
    .resume = agm_compress_resume,
    .drain = agm_compress_drain,
    .partial_drain = agm_compress_partial_drain,
    .next_track = agm_compress_next_track,
    .ioctl = agm_compress_ioctl,
    .poll = agm_compress_poll,
};

static int agm_populate_codec_caps(struct agm_compress_priv *priv)
{
    int codec_count = 0;

    if (priv->session_config.dir == RX)
        priv->compr_cap.direction = SND_COMPRESS_PLAYBACK;
    else
        priv->compr_cap.direction = SND_COMPRESS_CAPTURE;

    priv->compr_cap.min_fragment_size =
                    COMPR_PLAYBACK_MIN_FRAGMENT_SIZE;
    priv->compr_cap.max_fragment_size =
                    COMPR_PLAYBACK_MAX_FRAGMENT_SIZE;
    priv->compr_cap.min_fragments =
                    COMPR_PLAYBACK_MIN_NUM_FRAGMENTS;
    priv->compr_cap.max_fragments =
                    COMPR_PLAYBACK_MAX_NUM_FRAGMENTS;

    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_MP3;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_AAC;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_WMA;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_FLAC;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_VORBIS;
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_BESPOKE;
#ifdef SND_AUDIOCODEC_ALAC
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_ALAC;
#endif
#ifdef SND_AUDIOCODEC_APE
    priv->compr_cap.codecs[codec_count++] = SND_AUDIOCODEC_APE;
#endif
    priv->compr_cap.num_codecs = codec_count;

    return 0;
};

COMPRESS_PLUGIN_OPEN_FN(agm_compress_plugin)
{
    struct compress_plugin *agm_compress_plugin;
    struct agm_compress_priv *priv;
    uint64_t handle;
    int ret = 0, session_id = device;
    int is_playback = 0, is_capture = 0, sess_mode = 0;
    void *card_node, *compr_node;

    AGM_LOGV("%s: session_id: %d \n", __func__, device);
    agm_compress_plugin = calloc(1, sizeof(struct compress_plugin));
    if (!agm_compress_plugin)
        return -ENOMEM;

    priv = calloc(1, sizeof(struct agm_compress_priv));
    if (!priv) {
        ret = -ENOMEM;
        goto err_plugin_free;
    }

    card_node = snd_card_def_get_card(card);
    if (!card_node) {
        ret = -EINVAL;
        goto err_priv_free;
    }

    compr_node = snd_card_def_get_node(card_node, device, SND_NODE_TYPE_COMPR);
    if (!compr_node) {
        ret = -EINVAL;
        goto err_card_put;
    }

    agm_compress_plugin->card = card;
    agm_compress_plugin->ops = &agm_compress_ops;
    agm_compress_plugin->node = compr_node;
    agm_compress_plugin->priv = priv;
    priv->card_node = card_node;

    ret = snd_card_def_get_int(agm_compress_plugin->node, "playback",
                                                       &is_playback);
    if (ret)
       goto err_card_put;

    ret = snd_card_def_get_int(agm_compress_plugin->node, "capture",
                                                       &is_capture);
    if (ret)
       goto err_card_put;

    ret = snd_card_def_get_int(agm_compress_plugin->node, "session_mode",
                                                       &sess_mode);
    if (ret)
       goto err_card_put;

    priv->session_config.sess_mode = sess_mode;
    priv->session_config.dir = (flags & COMPRESS_IN) ? RX : TX;
    priv->session_id = session_id;
    AGM_LOGD("%s: requested agm session mode: %zu", __func__,
             priv->session_config.sess_mode);

    if ((priv->session_config.dir == RX) && !is_playback) {
        AGM_LOGE("%s: Playback is supported for device %d \n",
                                           __func__, device);
        goto err_card_put;
    }
    if ((priv->session_config.dir == TX) && !is_capture) {
        AGM_LOGE("%s: Capture is supported for device %d \n",
                                           __func__, device);
        goto err_card_put;
    }

    ret = agm_session_open(session_id, sess_mode, &handle);
    if (ret) {
        errno = ret;
        goto err_card_put;
    }

    // TODO introduce nonblock flag here
    // instead of checking with direction and then registering callback
    // use nonblock flag and then register call back
    /**
     * the agm call back aren't required for capture usecase. since
     * the read calls to agm are data blocking.
     * */
    if (priv->session_config.dir == RX) {
        ret = agm_session_register_cb(session_id, &agm_compress_event_cb,
                                  AGM_EVENT_DATA_PATH, agm_compress_plugin);
        if (ret)
            goto err_sess_cls;

        ret = agm_session_register_cb(session_id, &agm_compress_event_cb,
                                  AGM_EVENT_MODULE, agm_compress_plugin);
        if (ret)
            goto err_sess_cls;
    }

    agm_populate_codec_caps(priv);
    priv->handle = handle;
    *plugin = agm_compress_plugin;
    pthread_mutex_init(&priv->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->eos_lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->drain_lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->poll_lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&priv->early_eos_lock, (const pthread_mutexattr_t *) NULL);

    return 0;

err_sess_cls:
    agm_session_close(handle);
err_card_put:
    snd_card_def_put_card(card_node);
err_priv_free:
    free(priv);
err_plugin_free:
    free(agm_compress_plugin);
    if (ret < 0)
        return ret;
    else
        return -ret;
}

void agm_session_update_codec_options(struct agm_session_config *sess_cfg,
                                      struct snd_compr_params *params)
{

    union snd_codec_options *copt = &params->codec.options;

    // playback
    if (sess_cfg->dir == RX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_AAC:
            sess_cfg->codec.aac_dec.audio_obj_type = copt->generic.reserved[0];
            sess_cfg->codec.aac_dec.total_size_of_PCE_bits = copt->generic.reserved[1];
            break;
        case SND_AUDIOCODEC_FLAC:
            sess_cfg->codec.flac_dec.sample_size = copt->generic.reserved[0];
            sess_cfg->codec.flac_dec.min_blk_size = copt->generic.reserved[1];
            sess_cfg->codec.flac_dec.max_blk_size = copt->generic.reserved[2];
            sess_cfg->codec.flac_dec.min_frame_size = copt->generic.reserved[3];
            sess_cfg->codec.flac_dec.max_frame_size = copt->generic.reserved[4];
            break;
    #ifdef SND_AUDIOCODEC_ALAC
        case SND_AUDIOCODEC_ALAC:
            sess_cfg->codec.alac_dec.frame_length = copt->generic.reserved[0];
            sess_cfg->codec.alac_dec.compatible_version = copt->generic.reserved[1];
            sess_cfg->codec.alac_dec.bit_depth = copt->generic.reserved[2];
            sess_cfg->codec.alac_dec.pb = copt->generic.reserved[3];
            sess_cfg->codec.alac_dec.mb = copt->generic.reserved[4];
            sess_cfg->codec.alac_dec.kb = copt->generic.reserved[5];
            sess_cfg->codec.alac_dec.max_run = copt->generic.reserved[6];
            sess_cfg->codec.alac_dec.max_frame_bytes = copt->generic.reserved[7];
            sess_cfg->codec.alac_dec.avg_bit_rate = copt->generic.reserved[8];
            sess_cfg->codec.alac_dec.channel_layout_tag = copt->generic.reserved[9];
            break;
        case SND_AUDIOCODEC_BESPOKE:
            if (copt->generic.reserved[0] == (uint8_t)AGM_FORMAT_OPUS) {
                sess_cfg->codec.opus_dec.bitstream_format = copt->generic.reserved[1];
                sess_cfg->codec.opus_dec.payload_type = copt->generic.reserved[2];
                sess_cfg->codec.opus_dec.version = copt->generic.reserved[3];
                sess_cfg->codec.opus_dec.num_channels = copt->generic.reserved[4];
                sess_cfg->codec.opus_dec.pre_skip = copt->generic.reserved[5];
                sess_cfg->codec.opus_dec.sample_rate = copt->generic.reserved[6];
                sess_cfg->codec.opus_dec.output_gain = copt->generic.reserved[7];
                sess_cfg->codec.opus_dec.mapping_family = copt->generic.reserved[8];
                sess_cfg->codec.opus_dec.stream_count = copt->generic.reserved[9];
                sess_cfg->codec.opus_dec.coupled_count = copt->generic.reserved[10];
                memcpy(&sess_cfg->codec.opus_dec.channel_map[0], &copt->generic.reserved[11], 4);
                memcpy(&sess_cfg->codec.opus_dec.channel_map[4], &copt->generic.reserved[12], 4);
            }
            break;
    #endif
    #ifdef SND_AUDIOCODEC_APE
        case SND_AUDIOCODEC_APE:
            sess_cfg->codec.ape_dec.bit_width = copt->generic.reserved[0];
            sess_cfg->codec.ape_dec.compatible_version = copt->generic.reserved[1];
            sess_cfg->codec.ape_dec.compression_level = copt->generic.reserved[2];
            sess_cfg->codec.ape_dec.format_flags = copt->generic.reserved[3];
            sess_cfg->codec.ape_dec.blocks_per_frame = copt->generic.reserved[4];
            sess_cfg->codec.ape_dec.final_frame_blocks = copt->generic.reserved[5];
            sess_cfg->codec.ape_dec.total_frames = copt->generic.reserved[6];
            sess_cfg->codec.ape_dec.seek_table_present = copt->generic.reserved[7];
            break;
    #endif
        case SND_AUDIOCODEC_WMA:
    #ifdef SND_AUDIOPROFILE_WMA9_LOSSLESS
            if ((params->codec.profile == SND_AUDIOPROFILE_WMA9_PRO) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA9_LOSSLESS) ||
                (params->codec.profile == SND_AUDIOPROFILE_WMA10_LOSSLESS)) {
    #else
            if ((params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM0) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM1) ||
                (params->codec.profile == SND_AUDIOMODE_WMAPRO_LEVELM2)) {
    #endif
                sess_cfg->codec.wmapro_dec.avg_bytes_per_sec = copt->generic.reserved[0];
                sess_cfg->codec.wmapro_dec.blk_align = copt->generic.reserved[1];
                sess_cfg->codec.wmapro_dec.bits_per_sample = copt->generic.reserved[2];
                sess_cfg->codec.wmapro_dec.channel_mask = copt->generic.reserved[3];
                sess_cfg->codec.wmapro_dec.enc_options = copt->generic.reserved[4];
                sess_cfg->codec.wmapro_dec.advanced_enc_option = copt->generic.reserved[5];
                sess_cfg->codec.wmapro_dec.advanced_enc_option2 = copt->generic.reserved[6];
            } else {
                sess_cfg->codec.wma_dec.avg_bytes_per_sec = copt->generic.reserved[0];
                sess_cfg->codec.wma_dec.blk_align = copt->generic.reserved[1];
                sess_cfg->codec.wma_dec.bits_per_sample = copt->generic.reserved[2];
                sess_cfg->codec.wma_dec.channel_mask = copt->generic.reserved[3];
                sess_cfg->codec.wma_dec.enc_options = copt->generic.reserved[4];
            }
            break;
        default:
            break;
        }
    }

    // capture
    if (sess_cfg->dir == TX) {
        switch (params->codec.id) {
        case SND_AUDIOCODEC_AAC:
            break;

        default:
            break;
        }
    }
}
