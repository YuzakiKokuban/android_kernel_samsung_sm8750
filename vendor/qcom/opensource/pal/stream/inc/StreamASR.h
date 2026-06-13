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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAMASR_H_
#define STREAMASR_H_

#include <utility>
#include <map>

#include "Stream.h"
#include "ASRPlatformInfo.h"
#include "ResourceManager.h"
#include "ASREngine.h"
#include "asr_module_calibration_api.h"


typedef enum {
    ASR_STATE_NONE,
    ASR_STATE_IDLE,
    ASR_STATE_LOADED,
    ASR_STATE_ACTIVE,
    ASR_STATE_SSR,
} asrStateIdT;

static const std::map<int32_t, std::string> asrStateNameMap
{
    {ASR_STATE_NONE, std::string{"ASR_STATE_NONE"}},
    {ASR_STATE_IDLE, std::string{"ASR_STATE_IDLE"}},
    {ASR_STATE_LOADED, std::string{"ASR_STATE_LOADED"}},
    {ASR_STATE_ACTIVE, std::string{"ASR_STATE_ACTIVE"}},
    {ASR_STATE_SSR, std::string{"ASR_STATE_SSR"}},
};

enum {
    ASR_EV_LOAD_SOUND_MODEL,
    ASR_EV_UNLOAD_SOUND_MODEL,
    ASR_EV_SPEECH_CONFIG,
    ASR_EV_START_SPEECH_RECOGNITION,
    ASR_EV_STOP_SPEECH_RECOGNITION,
    ASR_EV_FORCE_OUTPUT,
    ASR_EV_PAUSE,
    ASR_EV_RESUME,
    ASR_EV_DEVICE_CONNECTED,
    ASR_EV_DEVICE_DISCONNECTED,
    ASR_EV_SSR_OFFLINE,
    ASR_EV_SSR_ONLINE,
    ASR_EV_CONCURRENT_STREAM,
    ASR_EV_EC_REF,
};

class ASREngine;

class StreamASR : public Stream {
 public:
    StreamASR(struct pal_stream_attributes *sattr,
                       struct pal_device *dattr,
                       uint32_t noOfDevices,
                       struct modifier_kv *modifiers __unused,
                       uint32_t no_of_modifiers __unused,
                       std::shared_ptr<ResourceManager> rm);
    ~StreamASR();
    int32_t open() { return 0; }
    int32_t close() override;
    int32_t prepare() override { return 0; }
    int32_t start() override;
    int32_t stop() override;

    static int32_t isSampleRateSupported(uint32_t sampleRate);
    static int32_t isChannelSupported(uint32_t numChannels);
    static int32_t isBitWidthSupported(uint32_t bitWidth);

    int32_t setVolume(struct pal_volume_data * volume __unused) { return 0; }
    int32_t mute(bool state __unused) override { return 0; }
    int32_t mute_l(bool state __unused) override { return 0; }
    int32_t pause() override { return 0; }
    int32_t pause_l() override { return 0; }
    int32_t resume() override { return 0; }
    int32_t resume_l() override { return 0; }

    int32_t read(struct pal_buffer *buf __unused) {return 0; }
    int32_t write(struct pal_buffer *buf __unused) { return 0; }

    int32_t DisconnectDevice(pal_device_id_t deviceId) override;
    int32_t ConnectDevice(pal_device_id_t deviceId) override;
    int32_t setECRef(std::shared_ptr<Device> dev, bool isEnable);
    int32_t setECRef_l(std::shared_ptr<Device> dev, bool isEnable);
    int32_t ssrDownHandler() override;
    int32_t ssrUpHandler() override;
    int32_t registerCallBack(pal_stream_callback cb,  uint64_t cookie) override;
    int32_t getCallBack(pal_stream_callback *cb) override { return 0; }
    int32_t getParameters(uint32_t paramId, void **payload) override { return 0; }
    int32_t setParameters(uint32_t paramId, void *payload) override;

    int32_t Resume(bool isInternal = false) override;
    int32_t Pause(bool isInternal = false) override;
    int32_t HandleConcurrentStream(bool active) override;
    int32_t addRemoveEffect(pal_audio_effect_t effec __unused,
                            bool enable __unused) {
        return -ENOSYS;
    }
    int32_t setStreamAttributes(struct pal_stream_attributes *sattr __unused) {
        return 0;
    }
    uint32_t GetNumEvents() override;
    uint32_t GetOutputToken() override;
    uint32_t GetPayloadSize() override;
    int32_t SetupStreamConfig(const struct st_uuid *vendorUuid);
    int32_t SetupDetectionEngine();

    pal_device_id_t GetAvailCaptureDevice();
    std::shared_ptr<CaptureProfile> GetCurrentCaptureProfile();

    int32_t GetCurrentStateId();
    void TransitTo(int32_t stateId);
    void GetUUID(class SoundTriggerUUID *uuid, const struct st_uuid *vendorUuid);
    void HandleEventData(struct pal_asr_event *event, size_t eventSize);
    void sendAbort();
    bool compareConfig(struct pal_asr_config *oldConfig, struct pal_asr_config *newConfig);
    struct st_uuid GetVendorUuid() { return qcAsrUuid; }
    param_id_asr_config_t* GetSpeechConfig() { return recConfig;}
    param_id_asr_output_config_t* GetOutputConfig() { return outputConfig; }
    param_id_asr_input_threshold_t* GetInputBufConfig() { return inputConfig; }

 private:
    class ASREventData {
     public:
        ASREventData() {}
        virtual ~ASREventData() {}
    };

    class ASREventConfig {
     public:
        explicit ASREventConfig(int32_t evId) : id(evId) {}

        virtual ~ASREventConfig() {}

        int32_t id; // event id
        std::shared_ptr<ASREventData> data; // event specific data
    };

    class ASRLoadEventData : public ASREventData {
     public:
        ASRLoadEventData(void *data) : model(data){};
        ~ASRLoadEventData() {}

        void *model;
    };

    class ASRLoadEventConfig : public ASREventConfig {
     public:
        ASRLoadEventConfig(void *dataParam)
            : ASREventConfig(ASR_EV_LOAD_SOUND_MODEL) {

            data = std::make_shared<ASRLoadEventData>(dataParam);
        }
        ~ASRLoadEventConfig() {}
    };

    class ASRUnloadEventConfig : public ASREventConfig {
     public:
        ASRUnloadEventConfig() : ASREventConfig(ASR_EV_UNLOAD_SOUND_MODEL) {}
        ~ASRUnloadEventConfig() {}
    };

    class ASRSpeechCfgEventData : public ASREventData {
     public:
        ASRSpeechCfgEventData(void *data) : config(data){}
        ~ASRSpeechCfgEventData() {}

        void *config;
    };

    class ASRSpeechCfgEventConfig : public ASREventConfig {
     public:
        ASRSpeechCfgEventConfig(void *dataParam) : ASREventConfig(ASR_EV_SPEECH_CONFIG) {
            data = std::make_shared<ASRSpeechCfgEventData>(dataParam);
        }
        ~ASRSpeechCfgEventConfig() {}
    };


    class ASRStartRecognitionEventConfig : public ASREventConfig {
     public:
        ASRStartRecognitionEventConfig()
            : ASREventConfig(ASR_EV_START_SPEECH_RECOGNITION) {
        }
        ~ASRStartRecognitionEventConfig() {}
    };

    class ASRStopRecognitionEventConfig : public ASREventConfig {
     public:
        ASRStopRecognitionEventConfig()
            : ASREventConfig(ASR_EV_STOP_SPEECH_RECOGNITION) {
        }
        ~ASRStopRecognitionEventConfig() {}
    };

    class ASRForceOutputConfig : public ASREventConfig {
     public:
        ASRForceOutputConfig()
            : ASREventConfig(ASR_EV_FORCE_OUTPUT) {
        }
        ~ASRForceOutputConfig() {}
    };

    class ASRConcurrentStreamEventData : public ASREventData {
     public:
        ASRConcurrentStreamEventData(bool data) : active(data){};
        ~ASRConcurrentStreamEventData() {}

        bool active;
    };

    class ASRConcurrentStreamEventConfig : public ASREventConfig {
     public:
        ASRConcurrentStreamEventConfig (bool active)
            : ASREventConfig(ASR_EV_CONCURRENT_STREAM) {
            data = std::make_shared<ASRConcurrentStreamEventData>(active);
        }
        ~ASRConcurrentStreamEventConfig () {}
    };

    class ASRPauseEventConfig : public ASREventConfig {
     public:
        ASRPauseEventConfig() : ASREventConfig(ASR_EV_PAUSE) { }
        ~ASRPauseEventConfig() {}
    };

    class ASRResumeEventConfig : public ASREventConfig {
     public:
        ASRResumeEventConfig() : ASREventConfig(ASR_EV_RESUME) { }
        ~ASRResumeEventConfig() {}
    };

    class ASRDeviceConnectedEventData : public ASREventData {
     public:
        ASRDeviceConnectedEventData(pal_device_id_t dId) : devId(dId){};
        ~ASRDeviceConnectedEventData() {}

        pal_device_id_t devId;
    };

    class ASRDeviceConnectedEventConfig : public ASREventConfig {
     public:
        ASRDeviceConnectedEventConfig(pal_device_id_t id)
            : ASREventConfig(ASR_EV_DEVICE_CONNECTED) {
            data = std::make_shared<ASRDeviceConnectedEventData>(id);
        }
        ~ASRDeviceConnectedEventConfig() {}
    };

    class ASRDeviceDisconnectedEventData : public ASREventData {
     public:
        ASRDeviceDisconnectedEventData(pal_device_id_t dId) : devId(dId){};
        ~ASRDeviceDisconnectedEventData() {}

        pal_device_id_t devId;
    };

    class ASRDeviceDisconnectedEventConfig : public ASREventConfig {
     public:
        ASRDeviceDisconnectedEventConfig(pal_device_id_t id)
            : ASREventConfig(ASR_EV_DEVICE_DISCONNECTED) {
            data = std::make_shared<ASRDeviceDisconnectedEventData>(id);
        }
        ~ASRDeviceDisconnectedEventConfig() {}
    };

    class ASRECRefEventData : public ASREventData {
     public:
        ASRECRefEventData(std::shared_ptr<Device> dev, bool isEnable)
            : dev(dev), isEnable(isEnable) {}
        ~ASRECRefEventData() {}

        std::shared_ptr<Device> dev;
        bool isEnable;
    };

    class ASRECRefEventConfig : public ASREventConfig {
     public:
        ASRECRefEventConfig(std::shared_ptr<Device> dev, bool isEnable)
            : ASREventConfig(ASR_EV_EC_REF) {
            data = std::make_shared<ASRECRefEventData>(dev, isEnable);
        }
        ~ASRECRefEventConfig() {}
    };

    class ASRSSROfflineConfig : public ASREventConfig {
     public:
        ASRSSROfflineConfig() : ASREventConfig(ASR_EV_SSR_OFFLINE) { }
        ~ASRSSROfflineConfig() {}
    };

    class ASRSSROnlineConfig : public ASREventConfig {
     public:
        ASRSSROnlineConfig() : ASREventConfig(ASR_EV_SSR_ONLINE) { }
        ~ASRSSROnlineConfig() {}
    };

    class ASRState {
     public:
        ASRState(StreamASR& asrStream, int32_t stateId)
            : asrStream(asrStream), stateId(stateId) {}
        virtual ~ASRState() {}

        int32_t GetStateId() { return stateId; }

     protected:
        virtual int32_t ProcessEvent(std::shared_ptr<ASREventConfig> evCfg) = 0;

        void TransitTo(int32_t stateId) { asrStream.TransitTo(stateId); }

     private:
        StreamASR& asrStream;
        int32_t stateId;

        friend class StreamASR;

    };

    class ASRIdle : public ASRState {
     public:
        ASRIdle(StreamASR& asrStream)
            : ASRState(asrStream, ASR_STATE_IDLE) {}
        ~ASRIdle() {}
        int32_t ProcessEvent(std::shared_ptr<ASREventConfig> evCfg) override;
    };

    class ASRLoaded : public ASRState {
     public:
        ASRLoaded(StreamASR& asrStream)
            : ASRState(asrStream, ASR_STATE_LOADED) {}
        ~ASRLoaded() {}
        int32_t ProcessEvent(std::shared_ptr<ASREventConfig> evCfg) override;
    };
    class ASRActive : public ASRState {
     public:
        ASRActive(StreamASR& asrStream)
            : ASRState(asrStream, ASR_STATE_ACTIVE) {}
        ~ASRActive() {}
        int32_t ProcessEvent(std::shared_ptr<ASREventConfig> evCfg) override;
    };

    class ASRSSR : public ASRState {
     public:
        ASRSSR(StreamASR& asrStream)
            : ASRState(asrStream, ASR_STATE_SSR) {}
        ~ASRSSR() {}
        int32_t ProcessEvent(std::shared_ptr<ASREventConfig> evCfg) override;
    };

    static void EventNotificationThread(StreamASR *stream);
    void AddState(ASRState* state);
    bool UseLpiCaptureProfile();
    int32_t GetPreviousStateId();
    int32_t ProcessInternalEvent(std::shared_ptr<ASREventConfig> evCfg);
    int32_t SetRecognitionConfig(struct pal_asr_config *config);
    int32_t GenerateCallbackEvent(struct pal_asr_event **event,
                                  uint32_t *eventSize);
    /* Currently model is not loaded from HLOS, hence using this hardcoded UUID,
     * Later when loading is supported, we need to remove it from here, and
     * get it from model, to check which sm_config it supports to
     */
    static constexpr struct st_uuid qcAsrUuid =
     { 0x018ebfb8, 0x1364, 0x7417, 0xb92e, {0xf6, 0xab, 0x16, 0xb5, 0x54, 0x31} };
    std::shared_ptr<ASRStreamConfig> smCfg;
    std::shared_ptr<ASRCommonConfig> cmCfg;
    std::shared_ptr<ASRPlatformInfo> asrInfo;
    std::shared_ptr<CaptureProfile> capProf;
    std::shared_ptr<ASREngine> engine;

    struct pal_asr_config *palRecConfig;
    param_id_asr_config_t *recConfig;
    param_id_asr_output_config_t *outputConfig;
    param_id_asr_input_threshold_t *inputConfig;
    bool                  paused;
    bool                  deviceOpened;
    pal_stream_callback callback;
    uint64_t            cookie;

    ASRState *asrIdle;
    ASRState *asrLoaded;
    ASRState *asrActive;
    ASRState *asrSsr;

    ASRState *curState;
    ASRState *prevState;
    asrStateIdT stateToRestore;

    std::map<uint32_t, ASRState*> asrStates;
    std::condition_variable cv;
};
#endif // STREAMASR_H_
