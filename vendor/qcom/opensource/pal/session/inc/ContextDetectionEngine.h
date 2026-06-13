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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef CONTEXT_DETECTION_ENGINE_H
#define CONTEXT_DETECTION_ENGINE_H

#include <condition_variable>
#include <thread>
#include <mutex>
#include <vector>

#include "PalDefs.h"
#include "PalCommon.h"
#include "Device.h"
#include "SoundTriggerUtils.h"
#include "ACDPlatformInfo.h"
#include "PayloadBuilder.h"

typedef enum {
    ENG_IDLE,
    ENG_LOADED,
    ENG_ACTIVE,
    ENG_DETECTED,
} eng_state_t;

class Stream;
class Session;
class ACDPlatformInfo;

class ContextDetectionEngine
{
public:
    static std::shared_ptr<ContextDetectionEngine> Create(Stream *s,
        std::shared_ptr<ACDStreamConfig> sm_cfg);

    ContextDetectionEngine(Stream *s, std::shared_ptr<ACDStreamConfig> sm_cfg);
    virtual ~ContextDetectionEngine();

    virtual int32_t StartEngine(Stream *s) = 0;
    virtual int32_t StopEngine(Stream *s) = 0;
    virtual int32_t SetupEngine(Stream *s, void *config) = 0;
    virtual int32_t TeardownEngine(Stream *s, void *config) = 0;
    virtual int32_t ReconfigureEngine(Stream *s, void *old_config, void *new_config) = 0;
    virtual bool isEngActive() { return eng_state_ == ENG_ACTIVE; }

    virtual int32_t ConnectSessionDevice(
        Stream* stream_handle,
        pal_stream_type_t stream_type,
        std::shared_ptr<Device> device_to_connect);
    virtual int32_t DisconnectSessionDevice(
        Stream* stream_handle,
        pal_stream_type_t stream_type,
        std::shared_ptr<Device> device_to_disconnect);
    virtual int32_t SetupSessionDevice(
        Stream* streamHandle,
        pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect);
    virtual int32_t setECRef(
        Stream *s,
        std::shared_ptr<Device> dev,
        bool is_enable);
    int32_t getTagsWithModuleInfo(Stream *s, size_t *size, uint8_t *payload);

private:
    virtual int32_t LoadSoundModel() = 0;
    virtual int32_t UnloadSoundModel() = 0;

protected:
    std::shared_ptr<ACDPlatformInfo> acd_info_;
    std::shared_ptr<ACDStreamConfig> sm_cfg_;
    std::vector<Stream *> eng_streams_;
    Session *session_;
    Stream *stream_handle_;
    PayloadBuilder *builder_;
    uint32_t sample_rate_;
    uint32_t bit_width_;
    uint32_t channels_;
    int32_t dev_disconnect_count_;

    eng_state_t eng_state_;
    std::thread event_thread_handler_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool exit_thread_;
};

#endif  // CONTEXT_DETECTION_ENGINE_H
