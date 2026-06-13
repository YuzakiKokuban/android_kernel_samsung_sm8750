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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef STREAMACD_H_
#define STREAMACD_H_

#include <utility>
#include <map>

#include "Stream.h"
#include "ACDPlatformInfo.h"
#include "SoundTriggerUtils.h"
#include "ContextDetectionEngine.h"

class ContextDetectionEngine;

typedef enum {
    ACD_STATE_NONE,
    ACD_STATE_SSR,
    ACD_STATE_IDLE,
    ACD_STATE_LOADED,
    ACD_STATE_ACTIVE,
    ACD_STATE_DETECTED,
} acd_state_id_t;

static const std::map<int32_t, std::string> acdStateNameMap
{
    {ACD_STATE_NONE, std::string{"ACD_STATE_NONE"}},
    {ACD_STATE_SSR, std::string{"ACD_STATE_SSR"}},
    {ACD_STATE_IDLE, std::string{"ACD_STATE_IDLE"}},
    {ACD_STATE_LOADED, std::string{"ACD_STATE_LOADED"}},
    {ACD_STATE_ACTIVE, std::string{"ACD_STATE_ACTIVE"}},
    {ACD_STATE_DETECTED, std::string{"ACD_STATE_DETECTED"}}
};

enum {
    ACD_EV_LOAD_SOUND_MODEL,
    ACD_EV_UNLOAD_SOUND_MODEL,
    ACD_EV_RECOGNITION_CONFIG,
    ACD_EV_CONTEXT_CONFIG,
    ACD_EV_START_RECOGNITION,
    ACD_EV_STOP_RECOGNITION,
    ACD_EV_DETECTED,
    ACD_EV_PAUSE,
    ACD_EV_RESUME,
    ACD_EV_DEVICE_CONNECTED,
    ACD_EV_DEVICE_DISCONNECTED,
    ACD_EV_SSR_OFFLINE,
    ACD_EV_SSR_ONLINE,
    ACD_EV_CONCURRENT_STREAM,
    ACD_EV_EC_REF,
};

class ResourceManager;

class StreamACD : public Stream {
 public:
    StreamACD(struct pal_stream_attributes *sattr,
                       struct pal_device *dattr,
                       uint32_t no_of_devices,
                       struct modifier_kv *modifiers __unused,
                       uint32_t no_of_modifiers __unused,
                       std::shared_ptr<ResourceManager> rm);
    ~StreamACD();
    int32_t open() { palStateEnqueue(this, PAL_STATE_OPENED, 0); return 0; }
    int32_t close() override;
    int32_t prepare() override { return 0; }
    int32_t start() override;
    int32_t stop() override;
    int32_t getTagsWithModuleInfo(size_t *size, uint8_t *payload) override;
    int32_t setStreamAttributes(struct pal_stream_attributes *sattr __unused) {
        return 0;
    }
    int32_t setVolume(struct pal_volume_data * volume __unused) { return 0; }
    int32_t mute(bool state __unused) override { return 0; }
    int32_t mute_l(bool state __unused) override { return 0; }
    int32_t pause() override { return 0; }
    int32_t pause_l() override { return 0; }
    int32_t resume() override { return 0; }
    int32_t resume_l() override { return 0; }

    int32_t read(struct pal_buffer *buf __unused) {return 0; }
    int32_t write(struct pal_buffer *buf __unused) { return 0; }

    int32_t DisconnectDevice(pal_device_id_t device_id) override;
    int32_t ConnectDevice(pal_device_id_t device_id) override;
    int32_t setECRef(std::shared_ptr<Device> dev, bool is_enable);
    int32_t setECRef_l(std::shared_ptr<Device> dev, bool is_enable);
    int32_t ssrDownHandler();
    int32_t ssrUpHandler();
    int32_t registerCallBack(pal_stream_callback cb,  uint64_t cookie) override;
    int32_t getCallBack(pal_stream_callback *cb) override;
    int32_t getParameters(uint32_t param_id, void **payload) override;
    int32_t setParameters(uint32_t param_id, void *payload) override;
    int32_t addRemoveEffect(pal_audio_effect_t effec __unused,
                            bool enable __unused) {
        return -ENOSYS;
    }

    int32_t Resume(bool is_internal __unused) override;
    int32_t Pause(bool is_internal __unused) override;
    int32_t HandleConcurrentStream(bool active) override;

    pal_device_id_t GetAvailCaptureDevice();
    std::shared_ptr<CaptureProfile> GetCurrentCaptureProfile();
    void CacheEventData(struct acd_context_event *event);
    void SendCachedEventData();
    void PopulateCallbackPayload(struct acd_context_event *event, void *payload);

    int32_t GetCurrentStateId();
    void TransitTo(int32_t state_id);
    void GetUUID(class SoundTriggerUUID *uuid,
                 const struct st_uuid *vendor_uuid);
    void SetEngineDetectionData(struct acd_context_event *event);
    struct acd_recognition_cfg *GetRecognitionConfig();
    struct st_uuid GetVendorUuid();

 private:
    class ACDEventConfigData {
     public:
        ACDEventConfigData() {}
        virtual ~ACDEventConfigData() {}
    };

    class ACDEventConfig {
     public:
        explicit ACDEventConfig(int32_t ev_id)
            : id_(ev_id), data_(nullptr) {}
        virtual ~ACDEventConfig() {}

        int32_t id_; // event id
        std::shared_ptr<ACDEventConfigData> data_; // event specific data
    };

    class ACDLoadEventConfigData : public ACDEventConfigData {
     public:
        ACDLoadEventConfigData(void *data): data_(data) {}
        ~ACDLoadEventConfigData() {}

        void *data_;
    };

    class ACDLoadEventConfig : public ACDEventConfig {
     public:
        ACDLoadEventConfig(void *data)
            : ACDEventConfig(ACD_EV_LOAD_SOUND_MODEL) {
           data_ = std::make_shared<ACDLoadEventConfigData>(data);
        }
        ~ACDLoadEventConfig() {}
    };

    class ACDUnloadEventConfig : public ACDEventConfig {
     public:
        ACDUnloadEventConfig() : ACDEventConfig(ACD_EV_UNLOAD_SOUND_MODEL) {}
        ~ACDUnloadEventConfig() {}
    };

    class ACDRecognitionCfgEventConfigData : public ACDEventConfigData {
     public:
        ACDRecognitionCfgEventConfigData(void *data): data_(data) {}
        ~ACDRecognitionCfgEventConfigData() {}

        void *data_;
    };

    class ACDRecognitionCfgEventConfig : public ACDEventConfig {
     public:
        ACDRecognitionCfgEventConfig(void *data)
            : ACDEventConfig(ACD_EV_RECOGNITION_CONFIG) {
            data_ = std::make_shared<ACDRecognitionCfgEventConfigData>(data);
        }
        ~ACDRecognitionCfgEventConfig() {}
    };

    class ACDContextCfgEventConfigData : public ACDEventConfigData {
     public:
        ACDContextCfgEventConfigData(void *data): data_(data) {}
        ~ACDContextCfgEventConfigData() {}

        void *data_;
    };

    class ACDContextCfgEventConfig : public ACDEventConfig {
     public:
        ACDContextCfgEventConfig(void *data)
            : ACDEventConfig(ACD_EV_CONTEXT_CONFIG) {
            data_ = std::make_shared<ACDContextCfgEventConfigData>(data);
        }
        ~ACDContextCfgEventConfig() {}
    };

    class ACDStartRecognitionEventConfigData : public ACDEventConfigData {
     public:
        ACDStartRecognitionEventConfigData(int32_t rs) : restart_(rs) {}
        ~ACDStartRecognitionEventConfigData() {}

        bool restart_;
    };

    class ACDStartRecognitionEventConfig : public ACDEventConfig {
     public:
        ACDStartRecognitionEventConfig(bool restart)
            : ACDEventConfig(ACD_EV_START_RECOGNITION) {
            data_ = std::make_shared<ACDStartRecognitionEventConfigData>(restart);
        }
        ~ACDStartRecognitionEventConfig() {}
    };

    class ACDStopRecognitionEventConfigData : public ACDEventConfigData {
     public:
        ACDStopRecognitionEventConfigData(bool deferred) : deferred_(deferred) {}
        ~ACDStopRecognitionEventConfigData() {}

        bool deferred_;
    };

    class ACDStopRecognitionEventConfig : public ACDEventConfig {
     public:
        ACDStopRecognitionEventConfig(bool deferred)
            : ACDEventConfig(ACD_EV_STOP_RECOGNITION) {
            data_ = std::make_shared<ACDStopRecognitionEventConfigData>(deferred);
        }
        ~ACDStopRecognitionEventConfig() {}
    };

    class ACDDetectedEventConfigData : public ACDEventConfigData {
     public:
        ACDDetectedEventConfigData(void *data) : data_(data) {}
        ~ACDDetectedEventConfigData() {}

        void *data_;
    };

    class ACDDetectedEventConfig : public ACDEventConfig {
     public:
        ACDDetectedEventConfig(void *data) : ACDEventConfig(ACD_EV_DETECTED) {
            data_ = std::make_shared<ACDDetectedEventConfigData>(data);
        }
        ~ACDDetectedEventConfig() {}
    };

    class ACDConcurrentStreamEventConfigData : public ACDEventConfigData {
     public:
        ACDConcurrentStreamEventConfigData(bool active)
            : is_active_(active) {}
        ~ACDConcurrentStreamEventConfigData() {}

        bool is_active_;
    };

    class ACDConcurrentStreamEventConfig : public ACDEventConfig {
     public:
        ACDConcurrentStreamEventConfig (bool active)
            : ACDEventConfig(ACD_EV_CONCURRENT_STREAM) {
            data_ = std::make_shared<ACDConcurrentStreamEventConfigData>(active);
        }
        ~ACDConcurrentStreamEventConfig () {}
    };

    class ACDPauseEventConfig : public ACDEventConfig {
     public:
        ACDPauseEventConfig() : ACDEventConfig(ACD_EV_PAUSE) { }
        ~ACDPauseEventConfig() {}
    };

    class ACDResumeEventConfig : public ACDEventConfig {
     public:
        ACDResumeEventConfig() : ACDEventConfig(ACD_EV_RESUME) { }
        ~ACDResumeEventConfig() {}
    };

    class ACDDeviceConnectedEventConfigData : public ACDEventConfigData {
     public:
        ACDDeviceConnectedEventConfigData(pal_device_id_t id)
            : dev_id_(id) {}
        ~ACDDeviceConnectedEventConfigData() {}

        pal_device_id_t dev_id_;
    };

    class ACDDeviceConnectedEventConfig : public ACDEventConfig {
     public:
        ACDDeviceConnectedEventConfig(pal_device_id_t id)
            : ACDEventConfig(ACD_EV_DEVICE_CONNECTED) {
            data_ = std::make_shared<ACDDeviceConnectedEventConfigData>(id);
        }
        ~ACDDeviceConnectedEventConfig() {}
    };

    class ACDDeviceDisconnectedEventConfigData : public ACDEventConfigData {
     public:
        ACDDeviceDisconnectedEventConfigData(pal_device_id_t id)
            : dev_id_(id) {}
        ~ACDDeviceDisconnectedEventConfigData() {}

        pal_device_id_t dev_id_;
    };
    class ACDDeviceDisconnectedEventConfig : public ACDEventConfig {
     public:
        ACDDeviceDisconnectedEventConfig(pal_device_id_t id)
            : ACDEventConfig(ACD_EV_DEVICE_DISCONNECTED) {
            data_ = std::make_shared<ACDDeviceDisconnectedEventConfigData>(id);
        }
        ~ACDDeviceDisconnectedEventConfig() {}
    };

    class ACDECRefEventConfigData : public ACDEventConfigData {
     public:
        ACDECRefEventConfigData(std::shared_ptr<Device> dev, bool is_enable)
            : dev_(dev), is_enable_(is_enable) {}
        ~ACDECRefEventConfigData() {}

        std::shared_ptr<Device> dev_;
        bool is_enable_;
    };
    class ACDECRefEventConfig : public ACDEventConfig {
     public:
        ACDECRefEventConfig(std::shared_ptr<Device> dev, bool is_enable)
            : ACDEventConfig(ACD_EV_EC_REF) {
            data_ = std::make_shared<ACDECRefEventConfigData>(dev, is_enable);
        }
        ~ACDECRefEventConfig() {}
    };

    class ACDSSROfflineConfig : public ACDEventConfig {
     public:
        ACDSSROfflineConfig() : ACDEventConfig(ACD_EV_SSR_OFFLINE) { }
        ~ACDSSROfflineConfig() {}
    };

    class ACDSSROnlineConfig : public ACDEventConfig {
     public:
        ACDSSROnlineConfig() : ACDEventConfig(ACD_EV_SSR_ONLINE) { }
        ~ACDSSROnlineConfig() {}
    };

    class ACDState {
     public:
        ACDState(StreamACD& acd_stream, int32_t state_id)
            : acd_stream_(acd_stream), state_id_(state_id) {}
        virtual ~ACDState() {}

        int32_t GetStateId() { return state_id_; }

     protected:
        virtual int32_t ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg) = 0;

        void TransitTo(int32_t state_id) { acd_stream_.TransitTo(state_id); }

     private:
        StreamACD& acd_stream_;
        int32_t state_id_;

        friend class StreamACD;
    };

    class ACDIdle : public ACDState {
     public:
        ACDIdle(StreamACD& acd_stream)
            : ACDState(acd_stream, ACD_STATE_IDLE) {}
        ~ACDIdle() {}
        int32_t ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg) override;
    };

    class ACDLoaded : public ACDState {
     public:
        ACDLoaded(StreamACD& acd_stream)
            : ACDState(acd_stream, ACD_STATE_LOADED) {}
        ~ACDLoaded() {}
        int32_t ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg) override;
    };
    class ACDActive : public ACDState {
     public:
        ACDActive(StreamACD& acd_stream)
            : ACDState(acd_stream, ACD_STATE_ACTIVE) {}
        ~ACDActive() {}
        int32_t ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg) override;
    };

    class ACDDetected : public ACDState {
     public:
        ACDDetected(StreamACD& acd_stream)
            : ACDState(acd_stream, ACD_STATE_DETECTED) {}
        ~ACDDetected() {}
        int32_t ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg) override;
    };

    class ACDSSR : public ACDState {
     public:
        ACDSSR(StreamACD& acd_stream)
            : ACDState(acd_stream, ACD_STATE_SSR) {}
        ~ACDSSR() {}
        int32_t ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg) override;
    };

    void AddState(ACDState* state);
    int32_t GetPreviousStateId();
    int32_t ProcessInternalEvent(std::shared_ptr<ACDEventConfig> ev_cfg);

    int32_t SetupStreamConfig(const struct st_uuid *vendor_uuid);
    int32_t UpdateRecognitionConfig(struct acd_recognition_cfg *config);
    int32_t UpdateContextConfig(struct pal_param_context_list *config);
    int32_t SendRecognitionConfig(struct acd_recognition_cfg *config);
    int32_t SendContextConfig(struct pal_param_context_list *config);
    int32_t SetupDetectionEngine();

    int32_t GenerateCallbackEvent(struct pal_acd_recognition_event **event,
                                  uint32_t *event_size);
    static void EventNotificationThread(StreamACD *stream);

    std::shared_ptr<ACDStreamConfig> sm_cfg_;
    std::shared_ptr<ACDPlatformInfo> acd_info_;
    std::shared_ptr<CaptureProfile> cap_prof_;
    std::shared_ptr<ContextDetectionEngine> engine_;

    struct acd_recognition_cfg    *rec_config_;
    struct pal_param_context_list *context_config_;
    struct pal_st_recognition_event *cached_event_data_;
    bool                          paused_;
    bool                          device_opened_;

    pal_stream_callback callback_;
    uint64_t            cookie_;

    ACDState *acd_idle_;
    ACDState *acd_loaded_;
    ACDState *acd_active;
    ACDState *acd_detected_;
    ACDState *acd_ssr_;

    ACDState *cur_state_;
    ACDState *prev_state_;
    acd_state_id_t state_for_restore_;
    bool notificationInProgress = false;
    bool deferredNotification = false;

    std::map<uint32_t, ACDState*> acd_states_;
 protected:
    std::thread notification_thread_handler_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool exit_thread_;
};
#endif // STREAMACD_H_
