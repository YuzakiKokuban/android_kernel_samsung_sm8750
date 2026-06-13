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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SESSION_ALSAPCM_H
#define SESSION_ALSAPCM_H

#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include "Session.h"
#include "PalAudioRoute.h"
#include "PalCommon.h"
#include <tinyalsa/asoundlib.h>
#include <thread>
#include <mutex>

#define PARAM_ID_DETECTION_ENGINE_CONFIG_VOICE_WAKEUP 0x08001049
#define PARAM_ID_VOICE_WAKEUP_BUFFERING_CONFIG 0x08001044
#define PADDING_8BYTE_ALIGN(x) ((((x) + 7) & 7) ^ 7)

class Stream;
class Session;

class SessionAlsaPcm : public Session
{
private:
    uint32_t spr_miid = 0;
    PayloadBuilder* builder;
    struct pcm *pcm;
    struct pcm *pcmRx;
    struct pcm *pcmTx;
    std::shared_ptr<ResourceManager> rm;
    size_t in_buf_size, in_buf_count, out_buf_size, out_buf_count;
    std::vector<int> pcmDevIds;
    std::vector<int> pcmDevRxIds;
    std::vector<int> pcmDevTxIds;
    std::vector<std::pair<std::string, int>> freeDeviceMetadata;
    std::vector <std::pair<int, int>> gkv;
    std::vector <std::pair<int, int>> ckv;
    std::vector <std::pair<int, int>> tkv;
    std::thread threadHandler;
    sessionState mState;
    session_callback sessionCb;
    uint64_t cbCookie;
    uint32_t svaMiid;
    uint32_t asrMiid;
    uint32_t vaMicChannels;
    static std::mutex pcmLpmRefCntMtx;
    static int pcmLpmRefCnt;
    int32_t configureInCallRxMFC();
    static bool silenceEventRegistered;
public:

    SessionAlsaPcm(std::shared_ptr<ResourceManager> Rm);
    ~SessionAlsaPcm();
    int open(Stream * s) override;
    int prepare(Stream * s) override;
    int setTKV(Stream * s, configType type, effect_pal_payload_t *payload) override;
    int setConfig(Stream * s, configType type, int tag = 0) override;
    int setConfig(Stream * s, configType type, uint32_t tag1,
            uint32_t tag2, uint32_t tag3) override;
    //int getConfig(Stream * s) override;
    int start(Stream * s) override;
    int stop(Stream * s) override;
    int close(Stream * s) override;
    int readBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int writeBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int read(Stream *s, int tag, struct pal_buffer *buf, int * size) override;
    int write(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag) override;
    int setParameters(Stream *s, int tagId, uint32_t param_id, void *payload) override;
    int getParameters(Stream *s, int tagId, uint32_t param_id, void **payload) override;
    int setECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable) override;
    int getTimestamp(struct pal_session_time *stime) override;
    int registerCallBack(session_callback cb, uint64_t cookie) override;
    int drain(pal_drain_type_t type) override;
    int flush();
    int setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int connectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToDisconnect) override;
    bool isActive();
    int32_t getFrontEndId(uint32_t ldir) override;
    uint32_t getMIID(const char *backendName, uint32_t tagId, uint32_t *miid) override;
    struct mixer_ctl* getFEMixerCtl(const char *controlName, int *device, pal_stream_direction_t dir) override;
    int createMmapBuffer(Stream *s, int32_t min_size_frames,
                                   struct pal_mmap_buffer *info) override;
    int GetMmapPosition(Stream *s, struct pal_mmap_position *position) override;
    int ResetMmapBuffer(Stream *s) override;
    int openGraph(Stream *s) override;
    void adjustMmapPeriodCount(struct pcm_config *config, int32_t min_size_frames);
    void registerAdmStream(Stream *s, pal_stream_direction_t dir,
            pal_stream_flags_t flags, struct pcm *, struct pcm_config *cfg);
    void deRegisterAdmStream(Stream *s);
    void requestAdmFocus(Stream *s, long ns);
    void AdmRoutingChange(Stream *s);
    void releaseAdmFocus(Stream *s);
    void setEventPayload(uint32_t event_id, void *payload, size_t payload_size);
    int register_asps_event(uint32_t reg);
    int getTagsWithModuleInfo(Stream *s, size_t *size __unused, uint8_t *payload);
    void retryOpenWithoutEC(Stream *s, unsigned int pcm_flags, struct pcm_config *config);
    int reconfigureModule(uint32_t tagID, const char* BE, struct sessionToPayloadParam *data);
    int notifyUPDToneRendererFmtChng(struct pal_device *dAttr,
            us_tone_renderer_ep_media_format_status_t event);
};

#endif //SESSION_ALSAPCM_H
