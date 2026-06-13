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
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STREAMSOUNDTRIGGER_H_
#define STREAMSOUNDTRIGGER_H_

#include <utility>
#include <map>

#include "Stream.h"
#include "PalRingBuffer.h"
#include "SoundTriggerEngine.h"
#include "VoiceUIPlatformInfo.h"
#include "detection_cmn_api.h"
#include "mma_api.h"

enum {
    ENGINE_IDLE  = 0x0,
    GMM_DETECTED = 0x1,
    KEYWORD_DETECTION_SUCCESS = 0x2,
    KEYWORD_DETECTION_REJECT = 0x4,
    USER_VERIFICATION_SUCCESS = 0x8,
    USER_VERIFICATION_REJECT = 0x10,
    KEYWORD_DETECTION_PENDING = 0x20,
    USER_VERIFICATION_PENDING = 0x40,
    DETECTION_TYPE_SS = 0x1E,
    DETECTION_TYPE_ALL = 0x1F,
};

typedef enum {
    ST_STATE_NONE,
    ST_STATE_IDLE,
    ST_STATE_LOADED,
    ST_STATE_ACTIVE,
    ST_STATE_DETECTED,
    ST_STATE_BUFFERING,
    ST_STATE_SSR,
} st_state_id_t;

static const std::map<int32_t, std::string> stStateNameMap
{
    {ST_STATE_NONE, std::string{"ST_STATE_NONE"}},
    {ST_STATE_IDLE, std::string{"ST_STATE_IDLE"}},
    {ST_STATE_LOADED, std::string{"ST_STATE_LOADED"}},
    {ST_STATE_ACTIVE, std::string{"ST_STATE_ACTIVE"}},
    {ST_STATE_DETECTED, std::string{"ST_STATE_DETECTED"}},
    {ST_STATE_BUFFERING, std::string{"ST_STATE_BUFFERING"}},
    {ST_STATE_SSR, std::string{"ST_STATE_SSR"}}
};

enum {
    ST_EV_LOAD_SOUND_MODEL,
    ST_EV_UNLOAD_SOUND_MODEL,
    ST_EV_RECOGNITION_CONFIG,
    ST_EV_START_RECOGNITION,
    ST_EV_RESTART_RECOGNITION,
    ST_EV_STOP_RECOGNITION,
    ST_EV_DETECTED,
    ST_EV_READ_BUFFER,
    ST_EV_STOP_BUFFERING,
    ST_EV_PAUSE,
    ST_EV_RESUME,
    ST_EV_DEVICE_CONNECTED,
    ST_EV_DEVICE_DISCONNECTED,
    ST_EV_SSR_OFFLINE,
    ST_EV_SSR_ONLINE,
    ST_EV_CONCURRENT_STREAM,
    ST_EV_EC_REF,
    ST_EV_INTERNAL_PAUSE,
    ST_EV_INTERNAL_RESUME,
};

class ResourceManager;
class SoundModelInfo;
class VoiceUIInterface;

class StreamSoundTrigger : public Stream
{
public:
    StreamSoundTrigger(struct pal_stream_attributes *sattr,
                       struct pal_device *dattr,
                       uint32_t no_of_devices,
                       struct modifier_kv *modifiers __unused,
                       uint32_t no_of_modifiers __unused,
                       std::shared_ptr<ResourceManager> rm);
    ~StreamSoundTrigger();

    int32_t open() { return 0; }

    int32_t close() override;
    int32_t start() override;
    int32_t stop() override;

    int32_t prepare() override { return 0; }

    int32_t ssrDownHandler() override;
    int32_t ssrUpHandler() override;
    int32_t setStreamAttributes(struct pal_stream_attributes *sattr __unused) {
        return 0;
    }

    int32_t setVolume(struct pal_volume_data * volume __unused) { return 0; }
    int32_t mute(bool state __unused) override { return 0; }
    int32_t mute_l(bool state __unused) override { return 0; }
    int32_t getDeviceMute(pal_stream_direction_t dir __unused, bool *state __unused) override {return 0;}
    int32_t setDeviceMute(pal_stream_direction_t dir __unused, bool state __unused) override {return 0;}
    int32_t pause() override { return 0; }
    int32_t pause_l() override { return 0; }
    int32_t resume() override { return 0; }
    int32_t resume_l() override { return 0; }

    int32_t read(struct pal_buffer *buf) override;

    int32_t write(struct pal_buffer *buf __unused) { return 0; }

    int32_t registerCallBack(pal_stream_callback cb,  uint64_t cookie) override;
    int32_t getCallBack(pal_stream_callback *cb) override;
    int32_t getParameters(uint32_t param_id, void **payload) override;
    int32_t setParameters(uint32_t param_id, void *payload) override;

    int32_t addRemoveEffect(pal_audio_effect_t effec __unused,
                            bool enable __unused) {
        return -ENOSYS;
    }
    bool isStarted();
    void SetDetectedToEngines(bool detected);
    int32_t SetEngineDetectionState(int32_t state);

    static int32_t isSampleRateSupported(uint32_t sampleRate);
    static int32_t isChannelSupported(uint32_t numChannels);
    static int32_t isBitWidthSupported(uint32_t bitWidth);

    std::shared_ptr<CaptureProfile> GetCurrentCaptureProfile();
    int32_t DisconnectDevice(pal_device_id_t device_id) override;
    int32_t ConnectDevice(pal_device_id_t device_id) override;
    int disconnectStreamDevice_l(Stream* streamHandle,
        pal_device_id_t dev_id) override;
    int connectStreamDevice_l(Stream* streamHandle,
        struct pal_device *dattr) override;
    int32_t Resume(bool is_internal = false) override;
    int32_t Pause(bool is_internal = false) override;
    int32_t GetCurrentStateId();
    int32_t HandleConcurrentStream(bool active);
    int32_t setECRef(std::shared_ptr<Device> dev, bool is_enable) override;
    int32_t setECRef_l(std::shared_ptr<Device> dev, bool is_enable) override;
    bool ConfigSupportLPI() override;
    uint32_t getCallbackEventId() override;
    void TransitTo(int32_t state_id);

    friend class PalRingBufferReader;
    bool IsCaptureRequested() { return capture_requested_; }
    uint32_t GetRecognitionMode() { return recognition_mode_; }
    uint32_t GetHistBufDuration() { return hist_buf_duration_; }
    uint32_t GetPreRollDuration() { return pre_roll_duration_; }
    uint32_t GetModelId(){ return model_id_; }
    void SetModelId(uint32_t model_id) { model_id_ = model_id; }
    uint32_t GetInstanceId();
    bool IsStreamInBuffering() {
       return (capture_requested_ && reader_ &&
              (GetCurrentStateId() == ST_STATE_BUFFERING)) &&
              (reader_->isEnabled() || reader_->isPrepared());
    }
    struct st_uuid GetVendorUuid();
    void *GetGSLEngine() {
        if (gsl_engine_)
            return (void *)gsl_engine_.get();
        else
            return nullptr;
    }

    std::vector<PalRingBufferReader *> GetReaders() { return reader_list_;}

private:
    class EngineCfg {
     public:
        EngineCfg(int32_t id, std::shared_ptr<SoundTriggerEngine> engine,
                  void *data, int32_t size)
            : id_(id), engine_(engine), sm_data_(data), sm_size_(size) {}

        ~EngineCfg() {}

        std::shared_ptr<SoundTriggerEngine> GetEngine() const {
            return engine_;
        }
        int32_t GetEngineId() const { return id_; }

        int32_t id_;
        std::shared_ptr<SoundTriggerEngine> engine_;
        void *sm_data_;
        int32_t sm_size_;
    };

    class StEventConfigData {
     public:
        StEventConfigData() {}
        virtual ~StEventConfigData() {}
    };

    class StEventConfig {
     public:
        explicit StEventConfig(int32_t ev_id)
            : id_(ev_id), data_(nullptr) {}
        virtual ~StEventConfig() {}

        int32_t id_; // event id
        std::shared_ptr<StEventConfigData> data_; // event specific data
    };

    class StLoadEventConfigData : public StEventConfigData {
     public:
        StLoadEventConfigData(void *data): data_(data) {}
        ~StLoadEventConfigData() {}

        void *data_;
    };

    class StLoadEventConfig : public StEventConfig {
     public:
        StLoadEventConfig(void *data)
            : StEventConfig(ST_EV_LOAD_SOUND_MODEL) {
           data_ = std::make_shared<StLoadEventConfigData>(data);
        }
        ~StLoadEventConfig() {}
    };

    class StUnloadEventConfig : public StEventConfig {
     public:
        StUnloadEventConfig() : StEventConfig(ST_EV_UNLOAD_SOUND_MODEL) {}
        ~StUnloadEventConfig() {}
    };

    class StRecognitionCfgEventConfigData : public StEventConfigData {
     public:
        StRecognitionCfgEventConfigData(void *data): data_(data) {}
        ~StRecognitionCfgEventConfigData() {}

        void *data_;
    };

    class StRecognitionCfgEventConfig : public StEventConfig {
     public:
        StRecognitionCfgEventConfig(void *data)
            : StEventConfig(ST_EV_RECOGNITION_CONFIG) {
            data_ = std::make_shared<StRecognitionCfgEventConfigData>(data);
        }
        ~StRecognitionCfgEventConfig() {}
    };

    class StStartRecognitionEventConfigData : public StEventConfigData {
     public:
        StStartRecognitionEventConfigData(int32_t rs) : restart_(rs) {}
        ~StStartRecognitionEventConfigData() {}

        bool restart_;
    };

    class StStartRecognitionEventConfig : public StEventConfig {
     public:
        StStartRecognitionEventConfig(bool restart)
            : StEventConfig(ST_EV_START_RECOGNITION) {
            data_ = std::make_shared<StStartRecognitionEventConfigData>(restart);
        }
        ~StStartRecognitionEventConfig() {}
    };

    class StStopRecognitionEventConfigData : public StEventConfigData {
     public:
        StStopRecognitionEventConfigData(bool deferred) : deferred_(deferred) {}
        ~StStopRecognitionEventConfigData() {}

        bool deferred_;
    };

    class StStopRecognitionEventConfig : public StEventConfig {
     public:
        StStopRecognitionEventConfig(bool deferred)
            : StEventConfig(ST_EV_STOP_RECOGNITION) {
            data_ = std::make_shared<StStopRecognitionEventConfigData>(deferred);
        }
        ~StStopRecognitionEventConfig() {}
    };

    class StDetectedEventConfigData : public StEventConfigData {
     public:
        StDetectedEventConfigData(int32_t type) : det_type_(type) {}
        ~StDetectedEventConfigData() {}

        int32_t det_type_;
    };

    class StDetectedEventConfig : public StEventConfig {
     public:
        StDetectedEventConfig(int32_t type) : StEventConfig(ST_EV_DETECTED) {
            data_ = std::make_shared<StDetectedEventConfigData>(type);
        }
        ~StDetectedEventConfig() {}
    };

    class StReadBufferEventConfigData : public StEventConfigData {
     public:
        StReadBufferEventConfigData(void *data) : data_(data) {}
        ~StReadBufferEventConfigData() {}

        void *data_;
    };

    class StReadBufferEventConfig : public StEventConfig {
     public:
        StReadBufferEventConfig(void *data) : StEventConfig(ST_EV_READ_BUFFER) {
            data_ = std::make_shared<StReadBufferEventConfigData>(data);
        }
        ~StReadBufferEventConfig() {}
    };

    class StStopBufferingEventConfig : public StEventConfig {
     public:
        StStopBufferingEventConfig () : StEventConfig(ST_EV_STOP_BUFFERING) {}
        ~StStopBufferingEventConfig () {}
    };

    class StConcurrentStreamEventConfigData : public StEventConfigData {
     public:
        StConcurrentStreamEventConfigData(bool active)
            : is_active_(active) {}
        ~StConcurrentStreamEventConfigData() {}

        bool is_active_;
    };

    class StConcurrentStreamEventConfig : public StEventConfig {
     public:
        StConcurrentStreamEventConfig (bool active)
            : StEventConfig(ST_EV_CONCURRENT_STREAM) {
            data_ = std::make_shared<StConcurrentStreamEventConfigData>(active);
        }
        ~StConcurrentStreamEventConfig () {}
    };

    class StPauseEventConfig : public StEventConfig {
     public:
        StPauseEventConfig() : StEventConfig(ST_EV_PAUSE) { }
        ~StPauseEventConfig() {}
    };

    class StResumeEventConfig : public StEventConfig {
     public:
        StResumeEventConfig() : StEventConfig(ST_EV_RESUME) { }
        ~StResumeEventConfig() {}
    };

    class StInternalPauseEventConfig : public StEventConfig {
     public:
        StInternalPauseEventConfig() : StEventConfig(ST_EV_INTERNAL_PAUSE) { }
        ~StInternalPauseEventConfig() {}
    };

    class StInternalResumeEventConfig : public StEventConfig {
     public:
        StInternalResumeEventConfig() : StEventConfig(ST_EV_INTERNAL_RESUME) { }
        ~StInternalResumeEventConfig() {}
    };

    class StECRefEventConfigData : public StEventConfigData {
     public:
        StECRefEventConfigData(std::shared_ptr<Device> dev, bool is_enable)
            : dev_(dev), is_enable_(is_enable) {}
        ~StECRefEventConfigData() {}

        std::shared_ptr<Device> dev_;
        bool is_enable_;
    };
    class StECRefEventConfig : public StEventConfig {
     public:
        StECRefEventConfig(std::shared_ptr<Device> dev, bool is_enable)
            : StEventConfig(ST_EV_EC_REF) {
            data_ = std::make_shared<StECRefEventConfigData>(dev, is_enable);
        }
        ~StECRefEventConfig() {}
    };
    class StDeviceConnectedEventConfigData : public StEventConfigData {
     public:
        StDeviceConnectedEventConfigData(pal_device_id_t id)
            : dev_id_(id) {}
        ~StDeviceConnectedEventConfigData() {}

        pal_device_id_t dev_id_;
    };
    class StDeviceConnectedEventConfig : public StEventConfig {
     public:
        StDeviceConnectedEventConfig(pal_device_id_t id)
            : StEventConfig(ST_EV_DEVICE_CONNECTED) {
            data_ = std::make_shared<StDeviceConnectedEventConfigData>(id);
        }
        ~StDeviceConnectedEventConfig() {}
    };

    class StDeviceDisconnectedEventConfigData : public StEventConfigData {
     public:
        StDeviceDisconnectedEventConfigData(pal_device_id_t id)
            : dev_id_(id) {}
        ~StDeviceDisconnectedEventConfigData() {}

        pal_device_id_t dev_id_;
    };
    class StDeviceDisconnectedEventConfig : public StEventConfig {
     public:
        StDeviceDisconnectedEventConfig(pal_device_id_t id)
            : StEventConfig(ST_EV_DEVICE_DISCONNECTED) {
            data_ = std::make_shared<StDeviceDisconnectedEventConfigData>(id);
        }
        ~StDeviceDisconnectedEventConfig() {}
    };

    class StSSROfflineConfig : public StEventConfig {
     public:
        StSSROfflineConfig() : StEventConfig(ST_EV_SSR_OFFLINE) { }
        ~StSSROfflineConfig() {}
    };

    class StSSROnlineConfig : public StEventConfig {
     public:
        StSSROnlineConfig() : StEventConfig(ST_EV_SSR_ONLINE) { }
        ~StSSROnlineConfig() {}
    };

    class StState {
     public:
        StState(StreamSoundTrigger& st_stream, int32_t state_id)
            : st_stream_(st_stream), state_id_(state_id) {}
        virtual ~StState() {}

        int32_t GetStateId() { return state_id_; }

     protected:
        virtual int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) = 0;

        void TransitTo(int32_t state_id) { st_stream_.TransitTo(state_id); }

     private:
        StreamSoundTrigger& st_stream_;
        int32_t state_id_;

        friend class StreamSoundTrigger;
    };

    class StIdle : public StState {
     public:
        StIdle(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_IDLE) {}
        ~StIdle() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StLoaded : public StState {
     public:
        StLoaded(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_LOADED) {}
        ~StLoaded() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StActive : public StState {
     public:
        StActive(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_ACTIVE) {}
        ~StActive() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StDetected : public StState {
     public:
        StDetected(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_DETECTED) {}
        ~StDetected() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StBuffering : public StState {
     public:
        StBuffering(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_BUFFERING) {}
        ~StBuffering() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    class StSSR : public StState {
     public:
        StSSR(StreamSoundTrigger& st_stream)
            : StState(st_stream, ST_STATE_SSR) {}
        ~StSSR() {}
        int32_t ProcessEvent(std::shared_ptr<StEventConfig> ev_cfg) override;
    };

    pal_device_id_t GetAvailCaptureDevice();
    std::shared_ptr<SoundTriggerEngine> HandleEngineLoad(uint8_t *sm_data,
                         int32_t sm_size, listen_model_indicator_enum type,
                         st_module_type_t module_type);
    void AddEngine(std::shared_ptr<EngineCfg> engine_cfg);
    void updateStreamAttributes();
    int32_t UpdateDeviceConfig();
    void UpdateModelId(st_module_type_t type);
    int32_t LoadSoundModel(struct pal_st_sound_model *sm_data);
    int32_t UnloadSoundModel();
    int32_t UpdateSoundModel(struct pal_st_sound_model *sm_data);
    int32_t SendRecognitionConfig(struct pal_st_recognition_config *config);
    int32_t UpdateRecognitionConfig(struct pal_st_recognition_config *config);
    bool compareRecognitionConfig(
       const struct pal_st_recognition_config *current_config,
       struct pal_st_recognition_config *new_config);

    int32_t notifyClient(uint32_t detection);

    static void TimerThread(StreamSoundTrigger& st_stream);
    void PostDelayedStop();
    void CancelDelayedStop();
    void InternalStopRecognition();
    int32_t DisconnectEvent(std::shared_ptr<StEventConfig> ev_cfg,
          bool device_switch_event = false);
    int32_t ConnectEvent(std::shared_ptr<StEventConfig> ev_cfg);
    std::thread timer_thread_;
    std::mutex timer_mutex_;
    std::condition_variable timer_start_cond_;
    std::condition_variable timer_wait_cond_;
    bool timer_stop_waiting_;
    bool exit_timer_thread_;
    bool pending_stop_;
    bool paused_;
    bool device_opened_;
    st_module_type_t model_type_;

    void AddState(StState* state);
    int32_t GetPreviousStateId();
    int32_t ProcessInternalEvent(std::shared_ptr<StEventConfig> ev_cfg);
    void GetUUID(class SoundTriggerUUID *uuid, struct pal_st_sound_model *sound_model);
    void UpdateCaptureHandleInfo(bool start);
    bool IsSameDeviceType(pal_device_id_t dev_id, pal_device_id_t curr_dev_id);
    std::shared_ptr<VoiceUIPlatformInfo> vui_ptfm_info_;
    std::shared_ptr<VUIStreamConfig> sm_cfg_;
    SoundModelInfo* sm_info_;
    std::vector<std::shared_ptr<EngineCfg>> engines_;
    std::shared_ptr<SoundTriggerEngine> gsl_engine_;
    std::shared_ptr<VoiceUIInterface> vui_intf_;
    struct vui_intf_t vui_intf_handle_;

    pal_st_sound_model_type_t sound_model_type_;
    struct pal_st_sound_model *sm_config_;
    struct pal_st_recognition_config *rec_config_;
    uint32_t recognition_mode_;
    uint32_t detection_state_;
    uint32_t notification_state_;
    pal_stream_callback callback_;
    uint64_t cookie_;
    PalRingBufferReader *reader_;
    uint8_t *gsl_engine_model_;
    uint32_t gsl_engine_model_size_;
    uint8_t *gsl_conf_levels_;
    uint32_t gsl_conf_levels_size_;

    StState *st_idle_;
    StState *st_loaded_;
    StState *st_active;
    StState *st_detected_;
    StState *st_buffering_;
    StState *st_ssr_;

    StState *cur_state_;
    StState *prev_state_;
    st_state_id_t state_for_restore_;
    std::map<uint32_t, StState*> st_states_;
    std::shared_ptr<CaptureProfile> cap_prof_;
    std::shared_ptr<Device> ec_rx_dev_;
    uint32_t conf_levels_intf_version_;
    std::vector<PalRingBufferReader *> reader_list_;
    st_confidence_levels_info *st_conf_levels_;
    st_confidence_levels_info_v2 *st_conf_levels_v2_;
    bool capture_requested_;
    uint32_t hist_buf_duration_;
    uint32_t pre_roll_duration_;
    uint32_t model_id_;
    FILE *lab_fd_;
    bool rejection_notified_;
    ChronoSteadyClock_t transit_start_time_;
    ChronoSteadyClock_t transit_end_time_;
    // set to true only when mutex is not locked after callback
    bool mutex_unlocked_after_cb_;
    // flag to indicate whether we should update common capture profile in RM
    bool common_cp_update_disable_;
    bool second_stage_processing_;
    bool is_backend_shared_;
    /*
     * Used for device switch case when client need to switch
     * to specific device attributes instead of capture profile
     * from resourcemanager xml.
     */
    struct pal_device *dattr_specified_;
};
#endif // STREAMSOUNDTRIGGER_H_
