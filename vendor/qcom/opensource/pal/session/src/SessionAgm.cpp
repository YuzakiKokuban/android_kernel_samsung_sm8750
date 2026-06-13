/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: SessionAgm"

#include "SessionAgm.h"
#include "SessionAlsaUtils.h"
#include "Stream.h"
#include "ResourceManager.h"
#include "media_fmt_api.h"
#include <sstream>
#include <mutex>
#include <fstream>
#include <climits>

#define MICRO_SECS_PER_SEC (1000000LL)

void eventCallback(uint32_t session_id, struct agm_event_cb_params *event_params __unused,
                  void *client_data)
{
    struct pal_stream_attributes sAttr;
    SessionAgm *sessAgm = NULL;
    uint32_t event_id = PAL_STREAM_CBK_MAX;
    void *event_data = NULL;
    uint32_t event_size = 0;
    struct pal_event_read_write_done_payload *rw_done_payload = NULL;
    struct agm_event_read_write_done_payload *agm_rw_done_payload;

    if (!client_data) {
        PAL_ERR(LOG_TAG, "invalid client data");
        goto done;
    }

    sessAgm = static_cast<SessionAgm *>(client_data);
    if (session_id != sessAgm->sessionId) {
        PAL_ERR(LOG_TAG, "session Id %d in cb does not match with client data %d",
                         session_id, sessAgm->sessionId);
        goto done;
    }

    PAL_VERBOSE(LOG_TAG, "event_callback session id %d event id %d", session_id, event_params->event_id);
    sessAgm->streamHandle->getStreamAttributes(&sAttr);

    if (event_params->event_id == AGM_EVENT_READ_DONE ||
        event_params->event_id == AGM_EVENT_WRITE_DONE) {

        rw_done_payload = (struct pal_event_read_write_done_payload *) calloc(1,
                                     sizeof(struct pal_event_read_write_done_payload));
        if(!rw_done_payload){
            PAL_ERR(LOG_TAG, "Calloc allocation failed for rw_done_payload");
            goto done;
        }

        agm_rw_done_payload = (struct agm_event_read_write_done_payload *)event_params->event_payload;

        rw_done_payload->tag = agm_rw_done_payload->tag;
        rw_done_payload->status = agm_rw_done_payload->status;
        rw_done_payload->md_status = agm_rw_done_payload->md_status;

        rw_done_payload->buff.flags = agm_rw_done_payload->buff.flags;
        rw_done_payload->buff.size = agm_rw_done_payload->buff.size;
        rw_done_payload->buff.metadata_size = agm_rw_done_payload->buff.metadata_size;
        rw_done_payload->buff.metadata = agm_rw_done_payload->buff.metadata;
        rw_done_payload->buff.alloc_info.alloc_handle =
                                      agm_rw_done_payload->buff.alloc_info.alloc_handle;
        rw_done_payload->buff.alloc_info.alloc_size =
                                      agm_rw_done_payload->buff.alloc_info.alloc_size;
        rw_done_payload->buff.alloc_info.offset =
                                      agm_rw_done_payload->buff.alloc_info.offset;

        if (sAttr.flags & PAL_STREAM_FLAG_TIMESTAMP) {
            rw_done_payload->buff.ts = (struct timespec *)calloc(1, sizeof(struct timespec));

            if(!rw_done_payload->buff.ts){
                PAL_ERR(LOG_TAG, "Calloc allocation failed rw_done_payload buff");
                goto done;
            }
            rw_done_payload->buff.ts->tv_sec = agm_rw_done_payload->buff.timestamp/MICRO_SECS_PER_SEC;
            if (ULONG_MAX/MICRO_SECS_PER_SEC > rw_done_payload->buff.ts->tv_sec) {
                rw_done_payload->buff.ts->tv_nsec = (agm_rw_done_payload->buff.timestamp -
                                             rw_done_payload->buff.ts->tv_sec * MICRO_SECS_PER_SEC)*1000;
            } else {
                rw_done_payload->buff.ts->tv_sec = 0;
                rw_done_payload->buff.ts->tv_nsec = 0;
            }
        }
        PAL_VERBOSE(LOG_TAG, "tv_sec %llu", (unsigned long long)rw_done_payload->buff.ts->tv_sec);
        PAL_VERBOSE(LOG_TAG, "tv_nsec %llu", (unsigned long long)rw_done_payload->buff.ts->tv_nsec);

        if (event_params->event_id == AGM_EVENT_READ_DONE)
            event_id = PAL_STREAM_CBK_EVENT_READ_DONE;
        else
            event_id = PAL_STREAM_CBK_EVENT_WRITE_READY;

        event_data = (void *)rw_done_payload;
        event_size = sizeof(struct pal_event_read_write_done_payload);
    } else if (event_params->event_id == AGM_EVENT_EOS_RENDERED ||
               event_params->event_id == AGM_EVENT_EARLY_EOS) {

        if (event_params->event_id == AGM_EVENT_EOS_RENDERED)
            event_id = PAL_STREAM_CBK_EVENT_DRAIN_READY;
        else
            event_id = PAL_STREAM_CBK_EVENT_PARTIAL_DRAIN_READY;

        event_data = NULL;
    } else if (event_params->event_id == AGM_EVENT_EARLY_EOS_INTERNAL) {
        /* As it is internal event sent by AGM, don't call sessionCb */
        goto done;
    }

    if (sessAgm->sessionCb && event_id !=  PAL_STREAM_CBK_MAX) {
        sessAgm->sessionCb(sessAgm->cbCookie, event_id, event_data, event_size);
    } else {
       PAL_INFO(LOG_TAG, "no session cb registerd or event not valid");
    }

    if (rw_done_payload && rw_done_payload->buff.ts)
        free(rw_done_payload->buff.ts);

    if (rw_done_payload)
        free(rw_done_payload);

done:
    return;
}

int SessionAgm::getAgmCodecId(pal_audio_fmt_t fmt)
{
    int id = -1;

    switch (fmt) {
        case PAL_AUDIO_FMT_MP3:
            id = AGM_FORMAT_MP3;
            break;
        case PAL_AUDIO_FMT_AMR_NB:
            id = AGM_FORMAT_AMR_NB;
            break;
        case PAL_AUDIO_FMT_AMR_WB:
            id = AGM_FORMAT_AMR_WB;
            break;
        case PAL_AUDIO_FMT_AMR_WB_PLUS:
            id = AGM_FORMAT_AMR_WB_PLUS;
            break;
        case PAL_AUDIO_FMT_QCELP:
            id = AGM_FORMAT_QCELP;
            break;
        case PAL_AUDIO_FMT_EVRC:
            id = AGM_FORMAT_EVRC;
            break;
        case PAL_AUDIO_FMT_G711:
            id = AGM_FORMAT_G711;
            break;
        case PAL_AUDIO_FMT_AAC:
        case PAL_AUDIO_FMT_AAC_ADTS:
        case PAL_AUDIO_FMT_AAC_ADIF:
        case PAL_AUDIO_FMT_AAC_LATM:
            id = AGM_FORMAT_AAC;
            break;
        case PAL_AUDIO_FMT_WMA_STD:
            id = AGM_FORMAT_WMASTD;
            break;
        case PAL_AUDIO_FMT_PCM_S8:
            id = AGM_FORMAT_PCM_S8;
            break;
        case PAL_AUDIO_FMT_PCM_S16_LE:
            id = AGM_FORMAT_PCM_S16_LE;
            break;
        case PAL_AUDIO_FMT_PCM_S24_LE:
            id = AGM_FORMAT_PCM_S24_LE;
            break;
        case PAL_AUDIO_FMT_PCM_S24_3LE:
            id = AGM_FORMAT_PCM_S24_3LE;
            break;
        case PAL_AUDIO_FMT_PCM_S32_LE:
            id = AGM_FORMAT_PCM_S32_LE;
            break;
        case PAL_AUDIO_FMT_ALAC:
            id = AGM_FORMAT_ALAC;
            break;
        case PAL_AUDIO_FMT_APE:
            id = AGM_FORMAT_APE;
            break;
        case PAL_AUDIO_FMT_WMA_PRO:
            id = AGM_FORMAT_WMAPRO;
            break;
        case PAL_AUDIO_FMT_FLAC:
        case PAL_AUDIO_FMT_FLAC_OGG:
            id = AGM_FORMAT_FLAC;
            break;
        case PAL_AUDIO_FMT_VORBIS:
            id = AGM_FORMAT_VORBIS;
            break;
        case PAL_AUDIO_FMT_OPUS:
            id = AGM_FORMAT_OPUS;
            break;
        default:
            PAL_ERR(LOG_TAG, "Entered default format %x", fmt);
            break;
    }

    return id;
}

SessionAgm::SessionAgm(std::shared_ptr<ResourceManager> Rm)
{
    rm = Rm;
    builder = new PayloadBuilder();

    customPayload = NULL;
    customPayloadSize = 0;
    agmSessHandle = 0;
    instanceId = 0;
    sessionCb = NULL;
    this->cbCookie = 0;
    playback_started = false;
    playback_paused = false;
}

SessionAgm::~SessionAgm()
{
    delete builder;
}

int SessionAgm::open(Stream * strm)
{
    int status = -EINVAL;
    struct pal_stream_attributes sAttr;
    std::vector <std::pair<int, int>> streamKV;
    std::vector <std::pair<int, int>> emptyKV;
    struct agmMetaData streamMetaData(nullptr, 0);

    status = strm->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        goto exit;
    }

    ioMode = sAttr.flags & PAL_STREAM_FLAG_NON_BLOCKING_MASK;
    if (!ioMode) {
        PAL_ERR(LOG_TAG, "IO mode 0x%x not supported", ioMode);
        goto exit;
    }

    audio_fmt = sAttr.out_media_config.aud_fmt_id;

    sessionIds = rm->allocateFrontEndIds(sAttr, 0);
    if (sessionIds.size() == 0) {
        PAL_ERR(LOG_TAG, "no more FE vailable");
        return -EINVAL;
    }
    sessionId = sessionIds.at(0);

    // get streamKV
    if ((status = builder->populateStreamKV(strm, streamKV)) != 0) {
         PAL_ERR(LOG_TAG, "get stream KV failed %d", status);
         goto exit;
    }
    SessionAlsaUtils::getAgmMetaData(streamKV, emptyKV,
                  NULL, streamMetaData);
    if (!streamMetaData.size) {
        PAL_ERR(LOG_TAG, "stream RX metadata is zero");
        status = -ENOMEM;
        goto exit;
    }
    status = agm_session_set_metadata(sessionId, streamMetaData.size, streamMetaData.buf);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "agm_session_set_metadata failed for session %d", sessionId);
        goto freeMetaData;
    }

    streamHandle = strm;

    status = agm_session_open(sessionId, AGM_SESSION_NON_TUNNEL, &agmSessHandle);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "agm_session_open failed for session %d", sessionId);
        goto freeMetaData;
    }
    status = agm_session_register_cb(sessionId, eventCallback,
                                   AGM_EVENT_DATA_PATH, (void *)this);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "agm_session_register_cb failed for session %d", sessionId);
        goto closeSession;
    }

    status = agm_session_register_cb(sessionId, eventCallback,
                                  AGM_EVENT_MODULE, (void *)this);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "agm_session_register_cb failed for session %d", sessionId);
        goto closeSession;
    }

    sess_config = (struct agm_session_config *)calloc(1, sizeof(struct agm_session_config));
    if (!sess_config) {
        status = -ENOMEM;
        goto closeSession;
    }

    in_media_cfg = (struct agm_media_config *)calloc(1, sizeof(struct agm_media_config));
    if (!in_media_cfg) {
        status = -ENOMEM;
        goto freeSessCfg;
    }

    out_media_cfg = (struct agm_media_config *)calloc(1, sizeof(struct agm_media_config));
    if (!out_media_cfg) {
        status = -ENOMEM;
        goto freeInMediaCfg;
    }

    sess_config->dir = (enum direction)sAttr.direction;
    sess_config->sess_mode = AGM_SESSION_NON_TUNNEL;
    if (sAttr.flags & PAL_STREAM_FLAG_EXTERN_MEM)
        sess_config->data_mode = AGM_DATA_EXTERN_MEM;
    if (sAttr.flags & PAL_STREAM_FLAG_SRCM_INBAND)
        sess_config->sess_flags = AGM_SESSION_FLAG_INBAND_SRCM;

    //read path media config
    in_media_cfg->rate = sAttr.in_media_config.sample_rate;
    in_media_cfg->channels = sAttr.in_media_config.ch_info.channels;
    in_media_cfg->format = (enum agm_media_format)getAgmCodecId(sAttr.in_media_config.aud_fmt_id);

    //write path media config
    out_media_cfg->rate = sAttr.out_media_config.sample_rate;
    out_media_cfg->channels = sAttr.out_media_config.ch_info.channels;
    out_media_cfg->format = (enum agm_media_format)getAgmCodecId(sAttr.out_media_config.aud_fmt_id);

    status = agm_session_set_non_tunnel_mode_config(agmSessHandle, sess_config,
                                                    in_media_cfg, out_media_cfg,
                                                    &in_buff_cfg, &out_buff_cfg);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "agm_session_set_non_tunnel_mode_config failed ret:%d", status);
        goto freeOutMediaCfg;
    }

    goto exit;

freeOutMediaCfg:
    free(out_media_cfg);
freeInMediaCfg:
    free(in_media_cfg);
freeSessCfg:
    free(sess_config);

closeSession:
    agm_session_close(agmSessHandle);
freeMetaData:
    free(streamMetaData.buf);
exit:
    return status;
}

int SessionAgm::close(Stream * s)
{
    struct pal_stream_attributes sAttr;
    s->getStreamAttributes(&sAttr);

    agm_session_register_cb(sessionId, NULL,
                                  AGM_EVENT_DATA_PATH, (void *)this);

    agm_session_register_cb(sessionId, NULL,
                                  AGM_EVENT_MODULE, (void *)this);

    agm_session_close(agmSessHandle);
    PAL_DBG(LOG_TAG, "out of agmSessHandle close");

    rm->freeFrontEndIds(sessionIds, sAttr, 0);

    return 0;
}


int SessionAgm::prepare(Stream * s)
{
   int32_t status = 0;
   size_t in_max_metadata_sz,out_max_metadata_sz = 0;

   s->getMaxMetadataSz(&in_max_metadata_sz, &out_max_metadata_sz);

   /*
    *buffer config is all 0, except for max_metadata_sz in case of EXTERN_MEM mode
    *TODO: Do we need to support heap based NT mode session ?
    */
   in_buff_cfg.max_metadata_size = in_max_metadata_sz;
   out_buff_cfg.max_metadata_size = out_max_metadata_sz;

   status = agm_session_set_non_tunnel_mode_config(agmSessHandle, sess_config,
                                                   in_media_cfg, out_media_cfg,
                                                   &in_buff_cfg, &out_buff_cfg);
   if (status != 0) {
       PAL_ERR(LOG_TAG, "agm_session_set_non_tunnel_mode_config failed ret:%d", status);
       goto exit;
   }

   status = agm_session_prepare(agmSessHandle);
   if (status != 0) {
       PAL_ERR(LOG_TAG, "agm_session_prepare ret:%d", status);
   }

exit:
   return status;
}

int SessionAgm::start(Stream * s __unused)
{
    int32_t status = 0;
    rm->voteSleepMonitor(s, true);
    if (agmSessHandle)
        status = agm_session_start(agmSessHandle);
    if (status != 0) {
        rm->voteSleepMonitor(s, false);
        PAL_ERR(LOG_TAG, "agm_session_start failed %d", status);
    }
    return status;
}

int SessionAgm::pause(Stream * s __unused)
{
    return -EINVAL;
}

int SessionAgm::resume(Stream * s __unused)
{
    return -EINVAL;
}

int SessionAgm::stop(Stream * s __unused)
{
    int32_t status = 0;

    if (agmSessHandle) {
        status = agm_session_stop(agmSessHandle);
        rm->voteSleepMonitor(s, false);
    }
    return status;
}

int SessionAgm::read(Stream *s, int tag __unused, struct pal_buffer *buf, int *size )
{
    uint32_t bytes_read = 0;
    int status;
    struct agm_buff agm_buffer = {0, 0, 0, NULL, 0, NULL, {0, 0, 0}};
    struct pal_stream_attributes sAttr;

    s->getStreamAttributes(&sAttr);
    if (!buf) {
        PAL_VERBOSE(LOG_TAG, "buf: %pK, size: %zu",
                    buf, (buf ? buf->size : 0));
        return -EINVAL;
    }
    if (!agmSessHandle) {
        PAL_ERR(LOG_TAG, "NULL pointer access,agmSessHandle is invalid");
        return -EINVAL;
    }
    agm_buffer.size = buf->size;
    agm_buffer.metadata_size = buf->metadata_size;
    agm_buffer.metadata = buf->metadata;
    if (buf->ts && (sAttr.flags & PAL_STREAM_FLAG_TIMESTAMP)) {
       agm_buffer.flags = AGM_BUFF_FLAG_TS_VALID;
       if (ULONG_MAX/MICRO_SECS_PER_SEC > buf->ts->tv_sec) {
           agm_buffer.timestamp =
               buf->ts->tv_sec * MICRO_SECS_PER_SEC +  (buf->ts->tv_nsec/1000);
       } else {
           PAL_ERR(LOG_TAG, "timestamp tv_sec overflown %lu", buf->ts->tv_sec);
           return -EINVAL;
       }
    }
    agm_buffer.addr = buf->buffer;
    if (sAttr.flags & PAL_STREAM_FLAG_EXTERN_MEM) {
        agm_buffer.alloc_info.alloc_handle = buf->alloc_info.alloc_handle;
        agm_buffer.alloc_info.alloc_size = buf->alloc_info.alloc_size;
        agm_buffer.alloc_info.offset = buf->alloc_info.offset;
    }

    status = agm_session_read_with_metadata(agmSessHandle, &agm_buffer, &bytes_read);

    PAL_VERBOSE(LOG_TAG, "writing buffer (%zu bytes) to agmSessHandle device returned %d",
             buf->size, bytes_read);

    if (size)
        *size = bytes_read;
    return status;
}

int SessionAgm::fileWrite(Stream *s __unused, int tag __unused, struct pal_buffer *buf, int * size, int flag __unused)
{
    std::fstream fs;
    PAL_DBG(LOG_TAG, "Enter.");

    fs.open ("/data/testcompr.wav", std::fstream::binary | std::fstream::out | std::fstream::app);
    PAL_ERR(LOG_TAG, "file open success");
    char *buff= reinterpret_cast<char *>(buf->buffer);
    fs.write (buff,buf->size);
    PAL_ERR(LOG_TAG, "file write success");
    fs.close();
    PAL_ERR(LOG_TAG, "file close success");
    *size = (int)(buf->size);
    PAL_ERR(LOG_TAG,"iExit. size: %d", *size);
    return 0;
}

int SessionAgm::write(Stream *s, int tag __unused, struct pal_buffer *buf, int * size, int flag __unused)
{
    size_t bytes_written = 0;
    int status;
    struct agm_buff agm_buffer = {0, 0, 0, NULL, 0, NULL, {0, 0, 0}};
    struct pal_stream_attributes sAttr;

    s->getStreamAttributes(&sAttr);
    if (!buf) {
        PAL_VERBOSE(LOG_TAG, "buf: %pK, size: %zu",
                    buf, (buf ? buf->size : 0));
        return -EINVAL;
    }
    if (!agmSessHandle) {
        PAL_ERR(LOG_TAG, "NULL pointer access,agmSessHandle is invalid");
        return -EINVAL;
    }
    agm_buffer.size = buf->size;
    agm_buffer.metadata_size = buf->metadata_size;
    agm_buffer.metadata = buf->metadata;
    if (buf->ts && (sAttr.flags & PAL_STREAM_FLAG_TIMESTAMP)) {
       agm_buffer.flags = AGM_BUFF_FLAG_TS_VALID;
       if (ULONG_MAX/MICRO_SECS_PER_SEC > buf->ts->tv_sec) {
           agm_buffer.timestamp =
               buf->ts->tv_sec * MICRO_SECS_PER_SEC +  (buf->ts->tv_nsec/1000);
       } else {
           PAL_ERR(LOG_TAG, "timestamp tv_sec overflown %lu", buf->ts->tv_sec);
           return -EINVAL;
       }
    }
    if (buf->flags & PAL_STREAM_FLAG_EOF)
       agm_buffer.flags |= AGM_BUFF_FLAG_EOF;
    agm_buffer.addr = buf->buffer;
    if (sAttr.flags & PAL_STREAM_FLAG_EXTERN_MEM) {
        agm_buffer.alloc_info.alloc_handle = buf->alloc_info.alloc_handle;
        agm_buffer.alloc_info.alloc_size = buf->alloc_info.alloc_size;
        agm_buffer.alloc_info.offset = buf->alloc_info.offset;
    }

    status = agm_session_write_with_metadata(agmSessHandle, &agm_buffer, &bytes_written);

    PAL_VERBOSE(LOG_TAG, "writing buffer (%zu bytes) to agmSessHandle device returned %d",
             buf->size, bytes_written);

    if (size)
        *size = bytes_written;
    return status;
}

int SessionAgm::setParameters(Stream *s __unused, int tagId __unused, uint32_t param_id, void *payload)
{
    int32_t status = 0;
    pal_param_payload *param_payload = (pal_param_payload *)payload;

    switch (param_id) {
    case PAL_PARAM_ID_MODULE_CONFIG:
        status = agm_session_set_params(sessionId, param_payload->payload, param_payload->payload_size);
        break;
    default:
        PAL_INFO(LOG_TAG, "Unsupported param id %u", param_id);
        break;
    }

    return status;
}

int SessionAgm::registerCallBack(session_callback cb, uint64_t cookie)
{
    sessionCb = cb;
    cbCookie = cookie;
    return 0;
}

int SessionAgm::flush()
{
    int status = 0;

    if (!agmSessHandle) {
        PAL_ERR(LOG_TAG, "Compress is invalid");
        return -EINVAL;
    }

    PAL_VERBOSE(LOG_TAG,"Enter flush\n");
    status = agm_session_flush(agmSessHandle);
    PAL_VERBOSE(LOG_TAG,"flush complete\n");
    return status;
}

int SessionAgm::suspend(Stream *s)
{
    int status = 0;

    if (!agmSessHandle) {
        PAL_ERR(LOG_TAG, "Handle is invalid");
        return -EINVAL;
    }

    PAL_VERBOSE(LOG_TAG,"Enter suspend\n");
    status = agm_session_suspend(agmSessHandle);
    PAL_VERBOSE(LOG_TAG,"suspend complete\n");

    if (!status)
        rm->voteSleepMonitor(s, false);
    else
        PAL_VERBOSE(LOG_TAG, "suspend failed : %d \n", status);

    return status;
}

int SessionAgm::drain(pal_drain_type_t type)
{
    int status = 0;

    if (!agmSessHandle) {
       PAL_ERR(LOG_TAG, "agmSessHandle is invalid");
       return -EINVAL;
    }

    PAL_VERBOSE(LOG_TAG, "drain type = %d", type);

    switch (type) {
        case PAL_DRAIN:
        case PAL_DRAIN_PARTIAL:
            status = agm_session_eos(agmSessHandle);
        break;
        default:
             PAL_ERR(LOG_TAG, "invalid drain type = %d", type);
             return -EINVAL;
    }

    return status;
}

int SessionAgm::getTagsWithModuleInfo(Stream *s __unused, size_t *size, uint8_t *payload)
{
    int status = 0;
    status = agm_session_aif_get_tag_module_info(sessionId, 0, payload, size);
    if (0 != status)
        PAL_ERR(LOG_TAG,"getTagsWithModuleInfo Failed");

    return status;
}

int SessionAgm::getParameters(Stream *s __unused, int tagId __unused, uint32_t param_id __unused, void **payload __unused)
{
    return 0;
}
