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
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SESSION_AGM_H
#define SESSION_AGM_H


#include "Session.h"

#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include <algorithm>
#include <queue>
#include <deque>
#include "PalAudioRoute.h"
#include "PalCommon.h"
#include <condition_variable>
#include <agm/agm_api.h>

#define EARLY_EOS_DELAY_MS 150

class Stream;
class Session;

struct agmMetaData {
    uint8_t *buf;
    uint32_t size;
    agmMetaData(uint8_t *b, uint32_t s)
        :buf(b),size(s) {}
};

class SessionAgm : public Session
{
private:

    PayloadBuilder* builder;
    int32_t instanceId;
    std::condition_variable cv_; /* used to wait for incoming requests */
    std::mutex cv_mutex_; /* mutex used in conjunction with above cv */
    uint64_t agmSessHandle;
    bool playback_started;
    bool playback_paused;
    int ioMode;
    std::vector<int> sessionIds;
    pal_audio_fmt_t audio_fmt;
    int fileWrite(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag);
    std::vector <std::pair<int, int>> ckv;
    std::vector <std::pair<int, int>> tkv;
    int getAgmCodecId(pal_audio_fmt_t fmt);
    struct agm_session_config *sess_config;
    struct agm_media_config *in_media_cfg, *out_media_cfg;
    struct agm_buffer_config in_buff_cfg {0,0,0}, out_buff_cfg = in_buff_cfg;
public:
    SessionAgm(std::shared_ptr<ResourceManager> Rm);
    virtual ~SessionAgm();
    int open(Stream * s) override;
    int prepare(Stream * s) override;
    int setConfig(Stream * s __unused, configType type __unused, int tag __unused) {return -EINVAL;}
    int start(Stream * s) override;
    int stop(Stream * s) override;
    int close(Stream * s) override;
    int pause(Stream * s) override;
    int resume(Stream * s) override;
    int readBufferInit(Stream *s __unused, size_t noOfBuf __unused,
                       size_t bufSize __unused, int flag __unused) {return -EINVAL;}
    int writeBufferInit(Stream *s __unused, size_t noOfBuf __unused,
                       size_t bufSize __unused, int flag __unused) {return -EINVAL;}
    int setParameters(Stream *s, int tagId, uint32_t param_id, void *payload);
    int getParameters(Stream *s, int tagId, uint32_t param_id, void **payload);
    int read(Stream *s, int tag, struct pal_buffer *buf, int * size) override;
    int write(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag) override;
    int setECRef(Stream *s __unused, std::shared_ptr<Device> rx_dev __unused, bool is_enable __unused) {return 0;};
    int registerCallBack(session_callback cb, uint64_t cookie);
    int drain(pal_drain_type_t type);
    int flush();
    int suspend(Stream * s);
    int getTagsWithModuleInfo(Stream *s, size_t *size, uint8_t *payload) override;
    int getTimestamp(struct pal_session_time *stime __unused) {return 0;};
    int setupSessionDevice(Stream* streamHandle __unused, pal_stream_type_t streamType __unused,
        std::shared_ptr<Device> deviceToConnect __unused) {return 0;};
    int connectSessionDevice(Stream* streamHandle __unused, pal_stream_type_t streamType __unused,
        std::shared_ptr<Device> deviceToConnect __unused) {return 0;};
    int disconnectSessionDevice(Stream* streamHandle __unused, pal_stream_type_t streamType __unused,
        std::shared_ptr<Device> deviceToDisconnect __unused) {return 0;};
    uint32_t getMIID(const char *backendName __unused, uint32_t tagId __unused, uint32_t *miid __unused)
                     {return 0;};
    struct mixer_ctl* getFEMixerCtl(const char *controlName __unused, int *device __unused, pal_stream_direction_t dir __unused) {return 0;}
    session_callback sessionCb;
    uint64_t cbCookie;
    int32_t sessionId;
    Stream *streamHandle;
};

#endif //SESSION_AGM_H
