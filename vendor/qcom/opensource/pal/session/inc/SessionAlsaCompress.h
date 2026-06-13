/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

#ifndef SESSION_ALSACOMPRESS_H
#define SESSION_ALSACOMPRESS_H


#include "Session.h"

#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include <algorithm>
#include <queue>
#include <deque>
#include "PalAudioRoute.h"
#include "PalCommon.h"
#include <tinyalsa/asoundlib.h>
#include <condition_variable>
#include <sound/compress_params.h>
#include <tinycompress/tinycompress.h>

#define EARLY_EOS_DELAY_MS 150

class Stream;
class Session;

enum {
    OFFLOAD_CMD_EXIT,               /* exit compress offload thread loop*/
    OFFLOAD_CMD_DRAIN,              /* send a full drain request to DSP */
    OFFLOAD_CMD_PARTIAL_DRAIN,      /* send a partial drain request to DSP */
    OFFLOAD_CMD_WAIT_FOR_BUFFER,    /* wait for buffer released by DSP */
    OFFLOAD_CMD_ERROR               /* offload playback hit some error */
};

#ifdef SND_AUDIOPROFILE_WMA9_PRO
#define PAL_SND_PROFILE_WMA9_PRO SND_AUDIOPROFILE_WMA9_PRO
#else
#define PAL_SND_PROFILE_WMA9_PRO SND_AUDIOMODE_WMAPRO_LEVELM0
#endif

#ifdef SND_AUDIOPROFILE_WMA9_LOSSLESS
#define PAL_SND_PROFILE_WMA9_LOSSLESS SND_AUDIOPROFILE_WMA9_LOSSLESS
#else
#define PAL_SND_PROFILE_WMA9_LOSSLESS SND_AUDIOMODE_WMAPRO_LEVELM1
#endif

#ifdef SND_AUDIOPROFILE_WMA10_LOSSLESS
#define PAL_SND_PROFILE_WMA10_LOSSLESS SND_AUDIOPROFILE_WMA10_LOSSLESS
#else
#define PAL_SND_PROFILE_WMA10_LOSSLESS SND_AUDIOMODE_WMAPRO_LEVELM2
#endif

struct offload_msg {
    offload_msg(int c)
        :cmd(c) {}
    int cmd; /**< command */
};

class SessionAlsaCompress : public Session
{
private:

    struct compress *compress;
    uint32_t spr_miid = 0;
    PayloadBuilder* builder;
    struct snd_codec codec;
    //  unsigned int compressDevId;
    std::vector<int> compressDevIds;
    std::unique_ptr<std::thread> worker_thread;
    std::queue<std::shared_ptr<offload_msg>> msg_queue_;
    size_t compress_cap_buf_size;
    std::vector<std::pair<std::string, int>> freeDeviceMetadata;

    std::condition_variable cv_; /* used to wait for incoming requests */
    std::mutex cv_mutex_; /* mutex used in conjunction with above cv */
    void getSndCodecParam(struct snd_codec &codec, struct pal_stream_attributes &sAttr);
    int getSndCodecId(pal_audio_fmt_t fmt);
    int setCustomFormatParam(pal_audio_fmt_t audio_fmt);
    bool playback_started;
    bool capture_started;
    bool playback_paused;
    bool capture_paused;
    int ioMode;
    session_callback sessionCb;
    uint64_t cbCookie;
    pal_audio_fmt_t audio_fmt;
    int fileWrite(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag);
    std::vector <std::pair<int, int>> ckv;
    std::vector <std::pair<int, int>> tkv;
    bool isGaplessFmt = false;
    bool sendNextTrackParams = false;
    bool isGaplessFormat(pal_audio_fmt_t fmt);
    bool isCodecConfigNeeded(pal_audio_fmt_t audio_fmt,
                             pal_stream_direction_t stream_direction);
    int configureEarlyEOSDelay(void);
    void updateCodecOptions(pal_param_payload *param_payload,
                            pal_stream_direction_t stream_direction);
    int command = OFFLOAD_CMD_EXIT;
#ifdef SEC_AUDIO_MUTE_DETECTION
    bool isCompressStart = false;
#endif
public:
    SessionAlsaCompress(std::shared_ptr<ResourceManager> Rm);
    virtual ~SessionAlsaCompress();
    int open(Stream * s) override;
    int prepare(Stream * s) override;
    int setConfig(Stream * s, configType type, int tag = 0) override;
    int setConfig(Stream * s, configType type, uint32_t tag1,
            uint32_t tag2, uint32_t tag3) override;
    int setTKV(Stream * s, configType type, effect_pal_payload_t *payload) override;
    //int getConfig(Stream * s) override;
    int start(Stream * s) override;
    int stop(Stream * s) override;
    int close(Stream * s) override;
    int pause(Stream * s) override;
    int resume(Stream * s) override;
    int readBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int writeBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int setParameters(Stream *s, int tagId, uint32_t param_id, void *payload);
    int getParameters(Stream *s, int tagId, uint32_t param_id, void **payload);
    int read(Stream *s, int tag, struct pal_buffer *buf, int * size) override;
    int write(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag) override;
    int setECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable) override;
    static void offloadThreadLoop(SessionAlsaCompress *ob);
    int registerCallBack(session_callback cb, uint64_t cookie);
    int drain(pal_drain_type_t type);
    int flush();
    int getTimestamp(struct pal_session_time *stime) override;
    int setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int connectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToDisconnect) override;
    int32_t getFrontEndId(uint32_t ldir) override;
    uint32_t getMIID(const char *backendName, uint32_t tagId, uint32_t *miid) override;
    struct mixer_ctl* getFEMixerCtl(const char *controlName, int *device, pal_stream_direction_t dir __unused) override;
};

#endif //SESSION_ALSACOMPRESS_H
