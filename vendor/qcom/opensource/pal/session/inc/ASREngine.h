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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ASRENGINE_H
#define ASRENGINE_H

#include <map>

#include "ASRPlatformInfo.h"
#include "StreamASR.h"
#include "PayloadBuilder.h"
#include "asr_module_calibration_api.h"

typedef enum {
    ASR_ENG_IDLE,
    ASR_ENG_ACTIVE,
    ASR_ENG_TEXT_RECEIVED,
} asr_eng_state_t;

class Session;
class Stream;

class ASREngine
{
public:
    ASREngine(Stream *s, std::shared_ptr<ASRStreamConfig> smCfg);
    ~ASREngine();

    static std::shared_ptr<ASREngine> GetInstance(Stream *s,
                          std::shared_ptr<ASRStreamConfig> smCfg);
    int32_t StartEngine(Stream *s);
    int32_t StopEngine(Stream *s);
    int32_t ConnectSessionDevice(
        Stream* stream_handle,
        pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect);
    int32_t DisconnectSessionDevice(
        Stream* streamHandle,
        pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToDisconnect);
    int32_t SetupSessionDevice(
        Stream* streamHandle,
        pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToDisconnect);
    int32_t setECRef(Stream *s, std::shared_ptr<Device> dev,
                     bool is_enable, bool setECForFirstTime = false);
    int32_t setParameters(Stream *s, asr_param_id_type_t pid, void* paramPayload = nullptr);
    uint32_t GetNumOutput() { return numOutput; }
    uint32_t GetOutputToken() { return outputToken; }
    uint32_t GetPayloadSize() { return payloadSize; }
    void releaseEngine() { eng = nullptr; }
private:
    static void EventProcessingThread(ASREngine *engine);
    static void HandleSessionCallBack(uint64_t hdl, uint32_t event_id, void *data,
                                      uint32_t eventSize);

    int32_t PopulateEventPayload();
    void ParseEventAndNotifyStream();
    void HandleSessionEvent(uint32_t eventId __unused, void *data, uint32_t size);
    bool IsEngineActive();

    bool isCrrDevUsingExtEc;
    bool exitThread;
    uint32_t numOutput;
    uint32_t payloadSize;
    uint32_t outputToken;
    uint32_t moduleTagIds[ASR_MAX_PARAM_IDS];
    uint32_t paramIds[ASR_MAX_PARAM_IDS];
    int32_t ecRefCount;
    int32_t devDisconnectCount;

    std::queue<void *> eventQ;
    static std::shared_ptr<ASREngine> eng;
    param_id_asr_config_t *speechCfg;
    param_id_asr_output_config_t *outputCfg;
    param_id_asr_input_threshold_t *inputCfg;
    std::shared_ptr<Device> rxEcDev;
    std::recursive_mutex ecRefMutex;
    std::shared_ptr<ASRPlatformInfo> asrInfo;
    std::shared_ptr<ASRStreamConfig> smCfg;

    asr_eng_state_t engState;
    std::thread eventThreadHandler;
    std::mutex mutexEngine;
    std::condition_variable cv;

    Session *session;
    Stream *streamHandle;
    PayloadBuilder *builder;
};
#endif  // ASRENGINE_H
