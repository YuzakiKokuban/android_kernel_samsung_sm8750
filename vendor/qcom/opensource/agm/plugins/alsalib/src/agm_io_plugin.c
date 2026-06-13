/*
** Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
**/
#define LOG_TAG "PLUGIN: AGMIO"
#include <stdio.h>
#include <sys/poll.h>

#include <sys/eventfd.h>
#include <alsa/asoundlib.h>
#include <alsa/pcm_external.h>

#include <agm/agm_api.h>
#include <agm/agm_list.h>
#include <snd-card-def.h>
#include "utils.h"

#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(a[0]))

enum {
    AGM_IO_STATE_OPEN = 1,
    AGM_IO_STATE_SETUP,
    AGM_IO_STATE_PREPARED,
    AGM_IO_STATE_RUNNING,
};

struct agmio_priv {
    snd_pcm_ioplug_t io;

    int card;
    int device;

    void *card_node;
    void *pcm_node;
    uint64_t handle;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;
    struct agm_session_config *session_config;
    unsigned int period_size;
    size_t frame_size;
    unsigned int state;
    snd_pcm_uframes_t hw_pointer;
    snd_pcm_uframes_t boundary;
    int event_fd;
/* add private variables here */
};

static int agm_get_session_handle(struct agmio_priv *priv,
                                  uint64_t *handle)
{
    if (!priv)
        return -EINVAL;

    *handle = priv->handle;
    if (!*handle)
        return -EINVAL;

    return 0;
}

static int agm_io_start(snd_pcm_ioplug_t * io)
{
    struct agmio_priv *pcm = io->private_data;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(pcm, &handle);
    if (ret)
        return ret;

    if (pcm->state < AGM_IO_STATE_PREPARED) {
        ret = agm_session_prepare(handle);
        errno = ret;
        if (ret)
            return ret;
        pcm->state = AGM_IO_STATE_PREPARED;
    }

    if (pcm->state != AGM_IO_STATE_RUNNING) {
        ret = agm_session_start(handle);
        if (!ret)
            pcm->state = AGM_IO_STATE_RUNNING;
    }

    AGM_LOGD("%s: exit\n", __func__);
    return ret;
}

static int agm_io_stop(snd_pcm_ioplug_t * io)
{
    struct agmio_priv *pcm = io->private_data;
    uint64_t handle;
    int ret;

    ret = agm_get_session_handle(pcm, &handle);
    if (ret)
        return ret;
    ret = agm_session_stop(handle);

    AGM_LOGD("%s: exit\n", __func__);
    return ret;
}

static int agm_io_drain(snd_pcm_ioplug_t * io)
{
    AGM_LOGD("%s: exit\n", __func__);
    return 0;
}

static snd_pcm_sframes_t agm_io_pointer(snd_pcm_ioplug_t * io)
{
    struct agmio_priv *pcm = io->private_data;
    snd_pcm_sframes_t new_hw_ptr;

    new_hw_ptr = pcm->hw_pointer;
    if (io->stream == SND_PCM_STREAM_CAPTURE) {
        if (pcm->hw_pointer == 0)
             new_hw_ptr = io->period_size * pcm->frame_size;
    }


    AGM_LOGD("%s %d\n", __func__, io->state);
    return new_hw_ptr;
}

static snd_pcm_sframes_t agm_io_transfer(snd_pcm_ioplug_t * io,
                                     const snd_pcm_channel_area_t * areas,
                                     snd_pcm_uframes_t offset,
                                     snd_pcm_uframes_t size)
{
    struct agmio_priv *pcm = io->private_data;
    uint64_t handle;
    uint8_t *buf = (uint8_t *) areas->addr + (areas->first + areas->step * offset) / 8;
    size_t count;
    int ret = 0;

    ret = agm_get_session_handle(pcm, &handle);
    if (ret)
        return ret;

    if (pcm->state != AGM_IO_STATE_RUNNING) {
        ret = agm_io_start(io);
        if (ret)
            return ret;
    }

    count = size * pcm->frame_size;

    if (io->stream == SND_PCM_STREAM_PLAYBACK)
        ret = agm_session_write(handle, buf, &count);
    else
        ret = agm_session_read(handle, buf, &count);

    if (ret == 0) {
        ret = snd_pcm_bytes_to_frames(io->pcm, count);
        pcm->hw_pointer += ret;
    }

    if (pcm->hw_pointer > pcm->boundary)
         pcm->hw_pointer -= pcm->boundary;

    AGM_LOGD("%s: exit\n", __func__);
    return ret;
}

static int agm_io_prepare(snd_pcm_ioplug_t * io)
{
    uint64_t handle;
    struct agmio_priv *pcm = io->private_data;
    int ret = 0;

    ret = agm_get_session_handle(pcm, &handle);
    if (ret)
        return ret;

    ret = agm_session_prepare(handle);

    AGM_LOGD("%s: exit\n", __func__);
    return ret;
}

static int agm_io_hw_params(snd_pcm_ioplug_t * io,
                           snd_pcm_hw_params_t * params)
{
    struct agmio_priv *pcm = io->private_data;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;
    struct agm_session_config *session_config = NULL;
    uint64_t handle;
    int ret = 0, sess_mode = 0;

    ret = agm_get_session_handle(pcm, &handle);

    pcm->frame_size = (snd_pcm_format_physical_width(io->format) * io->channels) / 8;

    media_config = pcm->media_config;
    buffer_config = pcm->buffer_config;
    session_config = pcm->session_config;

    media_config->rate =  io->rate;
    media_config->channels = io->channels;
    media_config->format = io->format;

    buffer_config->count = io->buffer_size / io->period_size;
    pcm->period_size = io->period_size;
    buffer_config->size = io->period_size * pcm->frame_size;
    pcm->hw_pointer = 0;

    snd_card_def_get_int(pcm->pcm_node, "session_mode", &sess_mode);

    session_config->dir = (io->stream == SND_PCM_STREAM_PLAYBACK) ? RX : TX;
    session_config->sess_mode = sess_mode;
    ret = agm_session_set_config(pcm->handle, session_config,
                                 pcm->media_config, pcm->buffer_config);
    if (!ret)
        pcm->state = AGM_IO_STATE_SETUP;

    AGM_LOGD("%s: exit\n", __func__);
    return ret;
}

static int agm_io_sw_params(snd_pcm_ioplug_t *io, snd_pcm_sw_params_t *params)
{
    struct agmio_priv *pcm = io->private_data;
    struct agm_session_config *session_config = NULL;
    uint64_t handle = 0;
    int ret = 0, sess_mode = 0;
    snd_pcm_uframes_t start_threshold;
    snd_pcm_uframes_t stop_threshold;

    ret = agm_get_session_handle(pcm, &handle);
    if (ret)
        return ret;
    session_config = pcm->session_config;

    snd_card_def_get_int(pcm->pcm_node, "session_mode", &sess_mode);

    session_config->dir = (io->stream == SND_PCM_STREAM_PLAYBACK) ? RX : TX;
    session_config->sess_mode = sess_mode;
    snd_pcm_sw_params_get_start_threshold(params, &start_threshold);
    snd_pcm_sw_params_get_stop_threshold(params, &stop_threshold);
    snd_pcm_sw_params_get_boundary(params, &pcm->boundary);
    session_config->start_threshold = (uint32_t)start_threshold;
    session_config->stop_threshold = (uint32_t)stop_threshold;
    ret = agm_session_set_config(pcm->handle, session_config,
                                 pcm->media_config, pcm->buffer_config);

    AGM_LOGD("%s: exit\n", __func__);
    return ret;
}

static int agm_io_close(snd_pcm_ioplug_t * io)
{
    struct agmio_priv *pcm = io->private_data;
    uint64_t handle;
    int ret = 0;

    ret = agm_get_session_handle(pcm, &handle);
    if (ret)
        return ret;

    ret = agm_session_close(handle);

    snd_card_def_put_card(pcm->card_node);
    free(pcm->buffer_config);
    free(pcm->media_config);
    free(pcm->session_config);
    free(io->private_data);

    AGM_LOGD("%s: exit\n", __func__);
    return 0;
}

static int agm_io_pause(snd_pcm_ioplug_t * io, int enable)
{
    struct agmio_priv *pcm = io->private_data;
    uint64_t handle;
    int ret = 0;

     ret = agm_get_session_handle(pcm, &handle);
     if (ret)
         return ret;

     if (enable)
         ret = agm_session_pause(handle);
     else
         ret = agm_session_resume(handle);

     AGM_LOGD("%s: exit\n", __func__);
     return ret;
}

static int agm_io_poll_desc_count(snd_pcm_ioplug_t *io) {
    (void)io;
    /* TODO : Needed for ULL usecases */
    AGM_LOGD("%s: exit\n", __func__);
    return 1;
}

static int agm_io_poll_desc(snd_pcm_ioplug_t *io, struct pollfd *pfd,
                            unsigned int space)
{
    struct agmio_priv *pcm = io->private_data;

    /* TODO : Needed for ULL usecases, Need update */
    if (space != 1) {
        AGM_LOGE("%s space %u is not correct!\n", __func__, space);
        return -EINVAL;
    }
    if (io->stream == SND_PCM_STREAM_PLAYBACK) {
        pfd[0].fd = pcm->event_fd;
        pfd[0].events = POLLOUT;
    } else {
        pfd[0].fd = pcm->event_fd;
        pfd[0].events = POLLIN;
    }

    AGM_LOGD("%s: exit\n", __func__);
    return space;
}

static int agm_io_poll_revents(snd_pcm_ioplug_t *io, struct pollfd *pfd,
                               unsigned int nfds, unsigned short *revents)
{
    struct agmio_priv *pcm = io->private_data;

    /* TODO : Needed for ULL usecases, Need update */
    if (nfds != 1) {
        AGM_LOGE("%s nfds %u is not correct!\n", __func__, nfds);
        return -EINVAL;
    }

    if (pfd[0].revents & POLLIN) {
        *revents = POLLIN;
    } else if (pfd[0].revents & POLLOUT) {
        *revents = POLLOUT;
    }
    return 0;
}

static const snd_pcm_ioplug_callback_t agm_io_callback = {
    .start = agm_io_start,
    .stop = agm_io_stop,
    .pointer = agm_io_pointer,
    .drain = agm_io_drain,
    .transfer = agm_io_transfer,
    .prepare = agm_io_prepare,
    .hw_params = agm_io_hw_params,
    .sw_params = agm_io_sw_params,
    .close = agm_io_close,
    .pause = agm_io_pause,
    .poll_descriptors_count = agm_io_poll_desc_count,
    .poll_descriptors = agm_io_poll_desc,
    .poll_revents = agm_io_poll_revents,
};

static int agm_hw_constraint(struct agmio_priv* priv)
{
    snd_pcm_ioplug_t *io = &priv->io;
    int ret;

    static const snd_pcm_access_t access_list[] = {
        SND_PCM_ACCESS_RW_INTERLEAVED,
        SND_PCM_ACCESS_MMAP_INTERLEAVED
    };
    static const unsigned int formats[] = {
        SND_PCM_FORMAT_U8,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_FORMAT_S32_LE,
        SND_PCM_FORMAT_S24_3LE,
        SND_PCM_FORMAT_S24_LE,
    };

    ret = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_ACCESS,
                                        ARRAY_SIZE(access_list),
                                        access_list);
    if (ret < 0)
        return ret;

    ret = snd_pcm_ioplug_set_param_list(io, SND_PCM_IOPLUG_HW_FORMAT,
                                        ARRAY_SIZE(formats), formats);
    if (ret < 0)
        return ret;

    ret = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_CHANNELS,
                                          1, 8);
    if (ret < 0)
            return ret;

    ret = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_RATE,
                                          8000, 384000);
    if (ret < 0)
            return ret;

    ret = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIOD_BYTES,
                                          64, 122880);
    if (ret < 0)
            return ret;

    ret = snd_pcm_ioplug_set_param_minmax(io, SND_PCM_IOPLUG_HW_PERIODS,
                                          1, 8);
    if (ret < 0)
            return ret;

    return 0;
}

SND_PCM_PLUGIN_DEFINE_FUNC(agm)
{
    snd_config_iterator_t it, next;
    struct agmio_priv *priv = NULL;
    long card = 0, device = 100;
    struct agm_session_config *session_config;
    struct agm_media_config *media_config;
    struct agm_buffer_config *buffer_config;
    void *card_node, *pcm_node;
    enum agm_session_mode sess_mode = AGM_SESSION_DEFAULT;
    uint64_t handle;
    int ret = 0, session_id = device;

    priv = calloc(1, sizeof(*priv));
    if (!priv)
        return -ENOMEM;

    media_config = calloc(1, sizeof(struct agm_media_config));
    if (!media_config)
        return -ENOMEM;

    buffer_config = calloc(1, sizeof(struct agm_buffer_config));
    if (!buffer_config)
        return -ENOMEM;

    session_config = calloc(1, sizeof(struct agm_session_config));
    if (!session_config)
        return -ENOMEM;

    snd_config_for_each(it, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(it);
        const char *id;

        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0)
            continue;
        if (strcmp(id, "card") == 0) {
            if (snd_config_get_integer(n, &card) < 0) {
                AGM_LOGE("Invalid type for %s", id);
                ret = -EINVAL;
                goto err_free_priv;
            }
            AGM_LOGD("card id is %d\n", card);
            priv->card = card;
            continue;
        }
        if (strcmp(id, "device") == 0) {
            if (snd_config_get_integer(n, &device) < 0) {
                AGM_LOGE("Invalid type for %s", id);
                ret = -EINVAL;
                goto err_free_priv;
            }
            AGM_LOGD("device id is %d\n", device);
            priv->device = device;
            continue;
        }
    }

    card_node = snd_card_def_get_card(card);
    if (!card_node) {
        AGM_LOGE("card node is NULL\n");
        ret = -EINVAL;
        goto err_free_priv;
    }
    priv->card_node = card_node;

    pcm_node = snd_card_def_get_node(card_node, device, SND_NODE_TYPE_PCM);
    if (!pcm_node) {
        AGM_LOGE("pcm node is NULL\n");
        ret = -EINVAL;
        goto err_free_priv;
    }
    priv->pcm_node = pcm_node;

    snd_card_def_get_int(pcm_node, "session_mode", &sess_mode);

    session_id = priv->device;
    ret = agm_session_open(session_id, sess_mode, &handle);
    if (ret) {
        AGM_LOGE("handle is NULL\n");
        ret = -EINVAL;
        goto err_free_priv;
    }

    priv->media_config = media_config;
    priv->buffer_config = buffer_config;
    priv->session_config = session_config;
    priv->handle = handle;
    priv->event_fd = -1;
    priv->state = AGM_IO_STATE_OPEN;
    priv->io.version = SND_PCM_IOPLUG_VERSION;
    priv->io.name = "AGM PCM I/O Plugin";
    priv->io.mmap_rw = 0;
    priv->io.callback = &agm_io_callback;
    priv->io.private_data = priv;

    ret = snd_pcm_ioplug_create(&priv->io, name, stream, mode);
    if (ret < 0) {
        AGM_LOGE("IO plugin create failed\n");
        goto err_free_priv;
    }

    if ((priv->event_fd = eventfd(0, EFD_CLOEXEC)) == -1) {
        AGM_LOGE("failed to create event_fd\n");
        ret = -EINVAL;
        goto err_free_priv;
    }

    ret = agm_hw_constraint(priv);
    if (ret < 0) {
        snd_pcm_ioplug_delete(&priv->io);
        goto err_free_priv;
    }

    *pcmp = priv->io.pcm;
    return 0;
err_free_priv:
    free(priv);
    return ret;
}

SND_PCM_PLUGIN_SYMBOL(agm);
