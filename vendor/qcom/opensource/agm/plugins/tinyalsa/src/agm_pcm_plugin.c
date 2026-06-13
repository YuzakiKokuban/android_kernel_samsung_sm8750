/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * Copyright (c) 2022-2024, Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */

#define LOG_TAG "PLUGIN: pcm"

#include <agm/agm_api.h>
#include <errno.h>
#include <limits.h>
#include <linux/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sound/asound.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <tinyalsa/pcm_plugin.h>
#include <snd-card-def.h>
#include <tinyalsa/asoundlib.h>
#include <agm/utils.h>
#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_PCM_PLUGIN
#include <log_utils.h>
#endif

/* 2 words of uint32_t = 64 bits of mask */
#define PCM_MASK_SIZE (2)
#define PCM_FORMAT_BIT(x) ((uint64_t)1 << x)

/* pull-push mode macros */
#define AGM_PULL_PUSH_IDX_RETRY_COUNT 2
#define AGM_PULL_PUSH_FRAME_CNT_RETRY_COUNT 5

/* multiplier of timeout for wating for mmap buffers */
#define MMAP_TOUT_MULTI 4

struct agm_shared_pos_buffer {
    volatile uint32_t frame_counter;
    volatile uint32_t read_index;
    volatile uint32_t wall_clock_us_lsw;
    volatile uint32_t wall_clock_us_msw;
};

struct pcm_plugin_pos_buf_info {
    void *pos_buf_addr;
    unsigned int boundary;       /* pcm boundary */
    snd_pcm_uframes_t hw_ptr;    /* RO: hw ptr (0...boundary-1) */
    snd_pcm_uframes_t hw_ptr_base;
    unsigned int crossed_boundary_cnt;
    struct timespec tstamp;
    snd_pcm_uframes_t appl_ptr;  /* RW: appl ptr (0...boundary-1) */
    snd_pcm_uframes_t avail_min; /* RW: min available frames for wakeup */
    uint32_t wall_clk_msw;
    uint32_t wall_clk_lsw;
    uint32_t frame_counter;
};

struct agm_mmap_buffer_port {
    void* mmap_buffer_addr;
    snd_pcm_uframes_t mmap_buffer_length;
};

struct agm_pcm_priv {
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;
    struct agm_session_config *session_config;
    struct agm_buf_info *buf_info;
    struct pcm_plugin_pos_buf_info *pos_buf;
    uint64_t handle;
    void *card_node;
    int session_id;
    unsigned int period_size;
    snd_pcm_uframes_t total_size_frames;
    /* idx: 0: out port, 1: in port */
    struct agm_mmap_buffer_port mmap_buffer_port[2];
    bool mmap_status;
    uint32_t mmap_buf_tout;
};

struct pcm_plugin_hw_constraints agm_pcm_constrs = {
    .access = 0,
    .format = 0,
    .bit_width = {
        .min = 16,
        .max = 32,
    },
    .channels = {
        .min = 1,
        .max = 8,
    },
    .rate = {
        .min = 8000,
        .max = 384000,
    },
    .periods = {
        .min = 1,
        .max = 8,
    },
    .period_bytes = {
        .min = 96,
        .max = 122880,
    },
};

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p,
                                                  int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static unsigned int param_get_int(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        if (i->integer)
            return i->max;
    }
    return 0;
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int snd_mask_val(const struct snd_mask *mask)
{
    int i;
    for (i = 0; i < PCM_MASK_SIZE; i++) {
        if (mask->bits[i])
            return ffs(mask->bits[i]) + (i << 5) - 1;
    }
    return 0;
}

static unsigned int agm_format_to_bits(enum pcm_format format)
{
    switch (format) {
    case AGM_FORMAT_PCM_S32_LE:
    case AGM_FORMAT_PCM_S24_LE:
        return 32;
    case AGM_FORMAT_PCM_S24_3LE:
        return 24;
    default:
    case AGM_FORMAT_PCM_S16_LE:
        return 16;
    };
}

static enum agm_media_format alsa_to_agm_format(int format)
{
    switch (format) {
    case SNDRV_PCM_FORMAT_S32_LE:
        return AGM_FORMAT_PCM_S32_LE;
    case SNDRV_PCM_FORMAT_S8:
        return AGM_FORMAT_PCM_S8;
    case SNDRV_PCM_FORMAT_S24_3LE:
        return AGM_FORMAT_PCM_S24_3LE;
    case SNDRV_PCM_FORMAT_S24_LE:
        return AGM_FORMAT_PCM_S24_LE;
    default:
    case SNDRV_PCM_FORMAT_S16_LE:
        return AGM_FORMAT_PCM_S16_LE;
    };
}

static enum agm_media_format param_get_mask_val(struct snd_pcm_hw_params *p,
                                        int n)
{
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        int val = snd_mask_val(m);

        return alsa_to_agm_format(val);
    }
    return 0;
}

static unsigned int agm_pcm_frames_to_bytes(struct agm_media_config *config,
        unsigned int frames)
{
    return frames * config->channels *
        (agm_format_to_bits(config->format) >> 3);
}

static unsigned int agm_pcm_bytes_to_frames(unsigned int bytes,
        struct agm_media_config *config)
{
    unsigned int frame_bits = config->channels *
        agm_format_to_bits(config->format);

    return bytes * 8 / frame_bits;
}

static int agm_get_session_handle(struct agm_pcm_priv *priv,
                                  uint64_t *handle)
{
    if (!priv)
        return -EINVAL;

    *handle = priv->handle;
    if (!*handle)
        return -EINVAL;

    return 0;
}

static void agm_pcm_plugin_apply_appl_ptr(struct agm_pcm_priv *priv,
        snd_pcm_uframes_t appl_ptr)
{
    struct pcm_plugin_pos_buf_info *pos = priv->pos_buf;

    pos->appl_ptr = appl_ptr;
}

static void agm_pcm_plugin_apply_avail_min(struct agm_pcm_priv *priv,
        snd_pcm_uframes_t avail_min)
{
    struct pcm_plugin_pos_buf_info *pos = priv->pos_buf;

    pos->avail_min = avail_min;
}

static snd_pcm_uframes_t agm_pcm_plugin_get_avail_min(struct agm_pcm_priv *priv)
{
    struct pcm_plugin_pos_buf_info *pos = priv->pos_buf;

    return pos->avail_min;
}

static snd_pcm_uframes_t agm_pcm_plugin_get_appl_ptr(struct agm_pcm_priv *priv)
{
    struct pcm_plugin_pos_buf_info *pos = priv->pos_buf;

    return pos->appl_ptr;
}

static snd_pcm_uframes_t agm_pcm_plugin_get_hw_ptr(struct agm_pcm_priv *priv)
{
    struct pcm_plugin_pos_buf_info *pos = priv->pos_buf;

    return pos->hw_ptr;
}

static int agm_pcm_plugin_get_shared_pos(struct pcm_plugin_pos_buf_info *pos_buf,
        uint32_t *read_index, uint32_t *wall_clk_msw,
        uint32_t *wall_clk_lsw)
{
    struct agm_shared_pos_buffer *buf;
    int i, j;
    uint32_t frame_cnt1, frame_cnt2;

    buf = (struct agm_shared_pos_buffer *)pos_buf->pos_buf_addr;
    for (i = 0; i < AGM_PULL_PUSH_IDX_RETRY_COUNT; ++i) {
        for (j = 0; j < AGM_PULL_PUSH_FRAME_CNT_RETRY_COUNT; ++j) {
            frame_cnt1 = buf->frame_counter;
            if (frame_cnt1 != 0)
                break;
        }
        *wall_clk_msw = buf->wall_clock_us_msw;
        *wall_clk_lsw = buf->wall_clock_us_lsw;
        *read_index = buf->read_index; /* 0,.... Circ_buf_size-1 */
        frame_cnt2 = buf->frame_counter;

        if (frame_cnt1 != frame_cnt2)
            continue;

        pos_buf->frame_counter = frame_cnt1;
        return 0;
    }

    return -EAGAIN;
}

static int agm_pcm_plugin_update_hw_ptr(struct agm_pcm_priv *priv)
{
    int retries = 10;
    snd_pcm_sframes_t circ_buf_pos;
    snd_pcm_uframes_t pos, old_hw_ptr, new_hw_ptr, hw_base;
    uint32_t read_index, wall_clk_msw, wall_clk_lsw;
    int64_t delta_wall_clk_us = 0;
    uint32_t delta_wall_clk_frames = 0;
    uint64_t sub_res = 0;
    int ret = 0;
    uint32_t period_size = priv->period_size; /** in frames */
    uint32_t crossed_boundary = 0;
    snd_pcm_uframes_t boundary = 0;
    uint32_t old_frame_counter = priv->pos_buf->frame_counter;

    do {
        ret = agm_pcm_plugin_get_shared_pos(priv->pos_buf,
                &read_index, &wall_clk_msw, &wall_clk_lsw);
    } while (ret == -EAGAIN && --retries);

    if (ret == 0) {
        circ_buf_pos = agm_pcm_bytes_to_frames(read_index, priv->media_config);
        pos = (circ_buf_pos / period_size) * period_size;
        old_hw_ptr = agm_pcm_plugin_get_hw_ptr(priv);
        hw_base = priv->pos_buf->hw_ptr_base;

        // Update new_hw_ptr
        __builtin_uaddl_overflow(hw_base, pos, &new_hw_ptr);
        __builtin_umull_overflow(priv->pos_buf->boundary,
                                 priv->pos_buf->crossed_boundary_cnt, &boundary);
        __builtin_uaddl_overflow(new_hw_ptr, boundary, &new_hw_ptr);

        // Set delta_wall_clk_us only if cached wall clk is non-zero
        if (priv->pos_buf->wall_clk_msw || priv->pos_buf->wall_clk_lsw) {
            uint64_t dsp_wall_clk =  (((uint64_t)wall_clk_msw) << 32 | wall_clk_lsw);
            uint64_t cached_wall_clk = (((uint64_t)priv->pos_buf->wall_clk_msw) << 32 |
                                         priv->pos_buf->wall_clk_lsw);
            // Compute delta only if diff is greater than zero
            if (dsp_wall_clk > cached_wall_clk) {
                __builtin_usubl_overflow(dsp_wall_clk,cached_wall_clk,&sub_res);
                delta_wall_clk_us = (int64_t)sub_res;
            }
        }
        // Identify the number of times of shared buffer length that the
        // hw ptr has jumped through by checking wall clock time delta
        // and assuming read ptr moved at a constant rate
        if (delta_wall_clk_us > 0 ) {
            __builtin_mul_overflow(delta_wall_clk_us / 1000000,
                (priv->media_config->rate * priv->media_config->channels),
                 &delta_wall_clk_frames);
            crossed_boundary = delta_wall_clk_frames / priv->total_size_frames;
        }

        // More than 1 loop of shared buffer completed by hw_ptr
        // No need to check if new_hw_ptr < old_hw_ptr, as new_hw_ptr
        // has crossed old_hw_ptr atleast once if not more
        if (crossed_boundary > 0) {
            hw_base += (crossed_boundary * priv->total_size_frames);
            if (hw_base >= priv->pos_buf->boundary) {
                priv->pos_buf->crossed_boundary_cnt += hw_base / priv->pos_buf->boundary;
                hw_base = 0;
            }
            __builtin_uaddl_overflow(hw_base, pos, &new_hw_ptr);
            __builtin_uaddl_overflow(new_hw_ptr,
                priv->pos_buf->boundary * priv->pos_buf->crossed_boundary_cnt, &new_hw_ptr);
            priv->pos_buf->hw_ptr_base = hw_base;
            AGM_LOGD("%s: crossed_boundary = %u, new_hw_ptr=%ld \n",
                                                __func__, crossed_boundary, new_hw_ptr);
            AGM_LOGD("%s: delta_wall_clk_frames = %lx, delta_wall_clk_us=%ld \n",
                                                __func__, delta_wall_clk_frames, delta_wall_clk_us);
            AGM_LOGD("%s: shared buffer length = %lx \n",
                                                __func__, priv->total_size_frames);
        } else {
            if (new_hw_ptr < old_hw_ptr) {
                hw_base += priv->total_size_frames;
                if (hw_base >= priv->pos_buf->boundary) {
                    hw_base = 0;
                    priv->pos_buf->crossed_boundary_cnt += 1;
                }
                __builtin_uaddl_overflow(hw_base, pos, &new_hw_ptr);
                __builtin_umull_overflow(priv->pos_buf->boundary,
                                         priv->pos_buf->crossed_boundary_cnt, &boundary);
                __builtin_uaddl_overflow(new_hw_ptr, boundary, &new_hw_ptr);
                priv->pos_buf->hw_ptr_base = hw_base;
            }
        }

        priv->pos_buf->hw_ptr = new_hw_ptr;
        // cache wall clk only when there's data update in shared buffer
        if (priv->pos_buf->frame_counter != old_frame_counter) {
            priv->pos_buf->wall_clk_lsw = wall_clk_lsw;
            priv->pos_buf->wall_clk_msw = wall_clk_msw;
        }
        clock_gettime(CLOCK_MONOTONIC, &priv->pos_buf->tstamp);
    }

    return ret;
}

static void agm_get_shared_buffer_addr(struct pcm_plugin *plugin,
                                       void **addr, snd_pcm_uframes_t *length)
{
    struct agm_pcm_priv *priv = plugin->priv;
    enum direction dir;

    dir = (plugin->mode & PCM_IN) ? TX : RX;
    *addr = priv->mmap_buffer_port[dir-1].mmap_buffer_addr;
    *length = priv->mmap_buffer_port[dir-1].mmap_buffer_length;
}

static int agm_pcm_plugin_reset(struct pcm_plugin *plugin)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;
    void *mmap_addr = NULL;
    snd_pcm_uframes_t mmap_length = 0;

    if (!(plugin->mode & PCM_NOIRQ))
        return -EOPNOTSUPP;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (!priv->buf_info)
        return -EINVAL;

    agm_get_shared_buffer_addr(plugin,&mmap_addr,&mmap_length);
    if (mmap_addr && mmap_length != 0) {
        memset(mmap_addr,0,mmap_length);
    } else {
        AGM_LOGE("%s: failed to clear shared buffer\n", __func__);
        ret = -EINVAL;
    }
    agm_pcm_plugin_update_hw_ptr(priv);
    priv->pos_buf->hw_ptr = (snd_pcm_uframes_t)(priv->pos_buf->hw_ptr % priv->total_size_frames);
    priv->pos_buf->hw_ptr_base = 0;
    priv->pos_buf->wall_clk_msw = 0;
    priv->pos_buf->wall_clk_lsw = 0;
    priv->pos_buf->crossed_boundary_cnt = 0;
    AGM_LOGD("%s: reset hw_ptr to %d \n", __func__, priv->pos_buf->hw_ptr);
    return ret;
}

static int agm_pcm_hw_params(struct pcm_plugin *plugin,
                             struct snd_pcm_hw_params *params)
{
    struct agm_pcm_priv *priv = plugin->priv;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;
    struct agm_session_config *session_config = NULL;
    uint64_t handle;
    int ret = 0, sess_mode = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    media_config = priv->media_config;
    buffer_config = priv->buffer_config;
    session_config = priv->session_config;

    media_config->rate =  param_get_int(params, SNDRV_PCM_HW_PARAM_RATE);
    media_config->channels = param_get_int(params, SNDRV_PCM_HW_PARAM_CHANNELS);
    media_config->format = param_get_mask_val(params, SNDRV_PCM_HW_PARAM_FORMAT);

    buffer_config->count = param_get_int(params, SNDRV_PCM_HW_PARAM_PERIODS);
    buffer_config->max_metadata_size = 0;
    priv->period_size = param_get_int(params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
    /* period size in bytes */
    buffer_config->size = agm_pcm_frames_to_bytes(media_config,
            priv->period_size);
    priv->total_size_frames = buffer_config->count *
            priv->period_size; /* in frames */

    snd_card_def_get_int(plugin->node, "session_mode", &sess_mode);
    session_config->dir = (plugin->mode & PCM_IN) ? TX : RX;
    session_config->sess_mode = sess_mode;
    AGM_LOGD("%s: mode: %d\n", __func__, plugin->mode);
    if ((plugin->mode & PCM_MMAP) && (plugin->mode & PCM_NOIRQ))
        session_config->data_mode = AGM_DATA_PUSH_PULL;

    ret = agm_session_set_config(priv->handle, session_config,
                                 priv->media_config, priv->buffer_config);
    return ret;
}

static int agm_pcm_sw_params(struct pcm_plugin *plugin,
                             struct snd_pcm_sw_params *sparams)
{
    struct agm_pcm_priv *priv = plugin->priv;
    struct agm_session_config *session_config = NULL;
    uint64_t handle = 0;
    int ret = 0, sess_mode = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    session_config = priv->session_config;

    snd_card_def_get_int(plugin->node, "session_mode", &sess_mode);

    session_config->dir = (plugin->mode & PCM_IN) ? TX : RX;
    session_config->sess_mode = sess_mode;
    session_config->start_threshold = (uint32_t)sparams->start_threshold;
    session_config->stop_threshold = (uint32_t)sparams->stop_threshold;

    ret = agm_session_set_config(priv->handle, session_config,
                                 priv->media_config, priv->buffer_config);
    return ret;
}

static int agm_pcm_sync_ptr(struct pcm_plugin *plugin,
                            struct snd_pcm_sync_ptr *sync_ptr)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;

    if (!(plugin->mode & PCM_NOIRQ))
        return -EOPNOTSUPP;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    if (!priv->buf_info)
        return -EINVAL;

    if (sync_ptr->flags & SNDRV_PCM_SYNC_PTR_HWSYNC) {
        /** sync hw ptr */
        ret = agm_pcm_plugin_update_hw_ptr(priv);
        if (ret < 0)
            return ret;
    }

    if (!(sync_ptr->flags & SNDRV_PCM_SYNC_PTR_APPL)) {
        agm_pcm_plugin_apply_appl_ptr(priv, sync_ptr->c.control.appl_ptr);
    } else {
        sync_ptr->c.control.appl_ptr = agm_pcm_plugin_get_appl_ptr(priv);
    }

    if (!(sync_ptr->flags & SNDRV_PCM_SYNC_PTR_AVAIL_MIN)) {
        agm_pcm_plugin_apply_avail_min(priv, sync_ptr->c.control.avail_min);
    } else {
        sync_ptr->c.control.avail_min = agm_pcm_plugin_get_avail_min(priv);
    }

    sync_ptr->s.status.hw_ptr = agm_pcm_plugin_get_hw_ptr(priv);
    sync_ptr->s.status.tstamp = priv->pos_buf->tstamp;

    return ret;
}

static int agm_pcm_writei_frames(struct pcm_plugin *plugin, struct snd_xferi *x)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    void *buff;
    size_t count;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    buff = x->buf;
    count = x->frames * (priv->media_config->channels *
            agm_format_to_bits(priv->media_config->format) / 8);

    ret = agm_session_write(handle, buff, &count);
    errno = ret;

    return ret;
}

static int agm_pcm_readi_frames(struct pcm_plugin *plugin, struct snd_xferi *x)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    void *buff;
    size_t count;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    buff = x->buf;
    count = x->frames * (priv->media_config->channels *
            agm_format_to_bits(priv->media_config->format) / 8);
    ret = agm_session_read(handle, buff, &count);
    errno = ret;

    return ret;
}

static int agm_pcm_ttstamp(struct pcm_plugin *plugin, int *tstamp __unused)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    /* TODO : Add AGM API call */
    return 0;
}

static int agm_pcm_prepare(struct pcm_plugin *plugin)
{
    uint64_t handle;
    struct agm_pcm_priv *priv = plugin->priv;
    int ret = 0;

    if (priv->pos_buf) {
        priv->pos_buf->wall_clk_msw = 0;
        priv->pos_buf->wall_clk_lsw = 0;
    }

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_prepare(handle);
    errno = ret;

    return ret;
}

static int agm_pcm_start(struct pcm_plugin *plugin)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_start(handle);
    errno = ret;

    return ret;
}

static int agm_pcm_drop(struct pcm_plugin *plugin)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_stop(handle);
    errno = ret;

    return ret;
}

static int agm_pcm_close(struct pcm_plugin *plugin)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    enum direction dir;
    int ret = 0;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    ret = agm_session_close(handle);
    errno = ret;

    snd_card_def_put_card(priv->card_node);
    free(priv->buffer_config);
    free(priv->media_config);
    free(priv->session_config);
    // unmap memory in case agm_pcm_munmap not called before close
    if (priv->mmap_status) {
        if (plugin->mode & PCM_NOIRQ) {
            if (priv->pos_buf) {
                munmap(priv->pos_buf->pos_buf_addr,
                        priv->buf_info->pos_buf_size);
                free(priv->pos_buf);
                priv->pos_buf = NULL;
            }
        }
        dir = (plugin->mode & PCM_IN) ? TX : RX;
        munmap(priv->mmap_buffer_port[dir-1].mmap_buffer_addr,
            priv->mmap_buffer_port[dir-1].mmap_buffer_length);
        priv->mmap_status = false;
    }
    if (priv->buf_info) {
        if (priv->buf_info->data_buf_fd != -1)
            close(priv->buf_info->data_buf_fd);
        if (priv->buf_info->pos_buf_fd != -1)
            close(priv->buf_info->pos_buf_fd);
        free(priv->buf_info);
    }
    free(plugin->priv);
    free(plugin);

    return ret;
}

static snd_pcm_sframes_t agm_pcm_get_avail(struct pcm_plugin *plugin)
{
    struct agm_pcm_priv *priv = plugin->priv;
    snd_pcm_sframes_t avail = 0;
    enum direction dir;

    dir = (plugin->mode & PCM_IN) ? TX : RX;

    if (dir == RX) {
        snd_pcm_uframes_t temp_value = 0;
        // In first condition, if there is no overflow, temp_value holds the addition result;
        // if there is overflow, __builtin_add_overflow returns true, then exit if clause.
        // In second condition, if there is overflow, __builtin_sub_overflow returns true and
        // we reset avail to 0. If there is no overflow, avail holds the final result and exit.
        if (!__builtin_add_overflow(priv->pos_buf->hw_ptr, priv->total_size_frames,
                &temp_value) &&
            __builtin_sub_overflow(temp_value, priv->pos_buf->appl_ptr, &avail)) {
            avail = 0;
        }

        if (avail < 0)
            avail += priv->pos_buf->boundary;
        else if ((snd_pcm_uframes_t)avail >= priv->pos_buf->boundary)
            avail -= priv->pos_buf->boundary;
    } else if (dir == TX) {
        __builtin_sub_overflow(priv->pos_buf->hw_ptr, priv->pos_buf->appl_ptr, &avail);
        if (avail < 0)
            avail += priv->pos_buf->boundary;
    }

    return avail;
}

static int agm_pcm_poll(struct pcm_plugin *plugin, struct pollfd *pfd,
        nfds_t nfds __attribute__ ((unused)), int timeout)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint32_t period_size = priv->period_size;
    snd_pcm_sframes_t avail;
    int ret = 0;
    uint32_t period_to_msec = period_size / (priv->media_config->rate / 1000);

    avail = agm_pcm_get_avail(plugin);

    if (avail < period_size) {
        if (timeout == 0) //wait for 1msec
            timeout = 1;
        usleep(timeout * 1000);
        ret = agm_pcm_plugin_update_hw_ptr(priv);
        if (ret == 0)
            avail = agm_pcm_get_avail(plugin);
    }

    if (avail >= period_size) {
        if (plugin->mode & PCM_IN) {
            pfd->revents = POLLIN | POLLOUT;
            ret = POLLIN;
        } else {
            pfd->revents = POLLOUT;
            ret = POLLOUT;
        }
        priv->mmap_buf_tout = 0;
    } else {
        ret = 0; /* TIMEOUT */
        priv->mmap_buf_tout += timeout;
        if (priv->mmap_buf_tout > (period_to_msec * MMAP_TOUT_MULTI)) {
            AGM_LOGE("timeout in waiting for mmap buffer");
            priv->mmap_buf_tout = 0;
            errno = ETIMEDOUT;
            return -ETIMEDOUT;
        }
    }

    return ret;
}

static void* agm_pcm_mmap(struct pcm_plugin *plugin, void *addr __unused, size_t length, int prot __unused,
                               int flags __unused, off_t offset)
{
    struct agm_pcm_priv *priv = plugin->priv;
    struct agm_buf_info *buf_info = NULL;
    struct pcm_plugin_pos_buf_info *pos = NULL;
    uint64_t handle;
    int flag = DATA_BUF;
    int ret = 0;
    unsigned int boundary;
    void *mmap_addr = NULL;
    enum direction dir;
    if (offset != 0)
        return MAP_FAILED;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return MAP_FAILED;

    if (!priv->buf_info) {
        buf_info = calloc(1, sizeof(struct agm_buf_info));
        if (!buf_info)
            return MAP_FAILED;

        if (plugin->mode & PCM_NOIRQ)
            flag |= POS_BUF;

        ret = agm_session_get_buf_info(priv->session_id, buf_info, flag);
        if (ret) {
            free(buf_info);
            return MAP_FAILED;
        }
        priv->buf_info = buf_info;
    }
    if (plugin->mode & PCM_NOIRQ) {
        if (!priv->pos_buf) {
            pos = calloc(1, sizeof(struct pcm_plugin_pos_buf_info));
            if (!pos)
                return MAP_FAILED;

            pos->pos_buf_addr = mmap(0, priv->buf_info->pos_buf_size,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    priv->buf_info->pos_buf_fd, 0);
            if (pos->pos_buf_addr == MAP_FAILED)
                return MAP_FAILED;

            priv->pos_buf = pos;

            boundary = priv->total_size_frames;
            while (boundary * 2 <= 0x7fffffffUL - priv->total_size_frames)
                boundary *= 2;

            priv->pos_buf->boundary = boundary;
            AGM_LOGE("%s: boundary: 0x%x, size_frames: 0x%lx\n",
                    __func__, boundary, priv->total_size_frames);
        }
    }

    if (length > priv->buf_info->data_buf_size)
        return MAP_FAILED;

    dir = (plugin->mode & PCM_IN) ? TX : RX;
    mmap_addr = mmap(0, length,
             PROT_READ | PROT_WRITE, MAP_SHARED,
             priv->buf_info->data_buf_fd, 0);
    if (mmap_addr != MAP_FAILED) {
        priv->mmap_buffer_port[dir-1].mmap_buffer_addr = mmap_addr;
        priv->mmap_buffer_port[dir-1].mmap_buffer_length = length;
    }
    priv->mmap_status = true;
    return mmap_addr;
}

static int agm_pcm_munmap(struct pcm_plugin *plugin, void *addr, size_t length)
{
    struct agm_pcm_priv *priv = plugin->priv;

    priv->mmap_status = false;
    if (plugin->mode & PCM_NOIRQ) {
        if (priv->pos_buf) {
            munmap(priv->pos_buf->pos_buf_addr,
                    priv->buf_info->pos_buf_size);
            free(priv->pos_buf);
            priv->pos_buf = NULL;
        }
    }
    return munmap(addr, length);
}

static int agm_pcm_ioctl(struct pcm_plugin *plugin, int cmd, ...)
{
    struct agm_pcm_priv *priv = plugin->priv;
    uint64_t handle;
    int ret = 0;
    va_list ap;
    void *arg;

    ret = agm_get_session_handle(priv, &handle);
    if (ret)
        return ret;

    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    switch (cmd) {
    case SNDRV_PCM_IOCTL_RESET:
        ret = agm_pcm_plugin_reset(plugin);
        break;
    default:
        break;
    }

    return ret;
}

struct pcm_plugin_ops agm_pcm_ops = {
    .close = agm_pcm_close,
    .hw_params = agm_pcm_hw_params,
    .sw_params = agm_pcm_sw_params,
    .sync_ptr = agm_pcm_sync_ptr,
    .writei_frames = agm_pcm_writei_frames,
    .readi_frames = agm_pcm_readi_frames,
    .ttstamp = agm_pcm_ttstamp,
    .prepare = agm_pcm_prepare,
    .start = agm_pcm_start,
    .drop = agm_pcm_drop,
    .mmap = agm_pcm_mmap,
    .munmap = agm_pcm_munmap,
    .poll = agm_pcm_poll,
    .ioctl = agm_pcm_ioctl,
};

PCM_PLUGIN_OPEN_FN(agm_pcm_plugin)
{
    struct pcm_plugin *agm_pcm_plugin;
    struct agm_pcm_priv *priv;
    struct agm_session_config *session_config;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;
    uint64_t handle;
    enum agm_session_mode sess_mode = AGM_SESSION_DEFAULT;
    int ret = 0, session_id = device;
    void *card_node, *pcm_node;

    agm_pcm_plugin = calloc(1, sizeof(struct pcm_plugin));
    if (!agm_pcm_plugin)
        return -ENOMEM;

    priv = calloc(1, sizeof(struct agm_pcm_priv));
    if (!priv) {
        ret = -ENOMEM;
        goto err_plugin_free;
    }

    media_config = calloc(1, sizeof(struct agm_media_config));
    if (!media_config) {
        ret = -ENOMEM;
        goto err_priv_free;
    }

    buffer_config = calloc(1, sizeof(struct agm_buffer_config));
    if (!buffer_config) {
        ret = -ENOMEM;
        goto err_media_free;
    }

    session_config = calloc(1, sizeof(struct agm_session_config));
    if (!session_config) {
        ret = -ENOMEM;
        goto err_buf_free;
    }

    card_node = snd_card_def_get_card(card);
    if (!card_node) {
        ret = -EINVAL;
        goto err_session_free;
    }

    pcm_node = snd_card_def_get_node(card_node, device, SND_NODE_TYPE_PCM);
    if (!pcm_node) {
        ret = -EINVAL;
        goto err_card_put;
    }

    agm_pcm_constrs.access = (PCM_FORMAT_BIT(SNDRV_PCM_ACCESS_RW_INTERLEAVED) |
                              PCM_FORMAT_BIT(SNDRV_PCM_ACCESS_RW_NONINTERLEAVED));
    agm_pcm_constrs.format = (PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S16_LE) |
                              PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S24_LE) |
                              PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S24_3LE) |
                              PCM_FORMAT_BIT(SNDRV_PCM_FORMAT_S32_LE));

    agm_pcm_plugin->card = card;
    agm_pcm_plugin->ops = &agm_pcm_ops;
    agm_pcm_plugin->node = pcm_node;
    agm_pcm_plugin->mode = mode;
    agm_pcm_plugin->constraints = &agm_pcm_constrs;
    agm_pcm_plugin->priv = priv;

    priv->media_config = media_config;
    priv->buffer_config = buffer_config;
    priv->session_config = session_config;
    priv->card_node = card_node;
    priv->session_id = session_id;
    priv->mmap_status = false;
    snd_card_def_get_int(pcm_node, "session_mode", &sess_mode);

    ret = agm_session_open(session_id, sess_mode, &handle);
    if (ret) {
        errno = ret;
        goto err_card_put;
    }
    priv->handle = handle;
    *plugin = agm_pcm_plugin;

    return 0;

err_card_put:
    snd_card_def_put_card(card_node);
err_session_free:
    free(session_config);
err_buf_free:
    free(buffer_config);
err_media_free:
    free(media_config);
err_priv_free:
    free(priv);
err_plugin_free:
    free(agm_pcm_plugin);
    if (ret < 0)
       return ret;
    else
       return -ret;
}
