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

#ifndef SOUNDTRIGGERENGINEGSL_H
#define SOUNDTRIGGERENGINEGSL_H

#include <map>
#include <queue>
#include "SoundTriggerEngine.h"
#include "SoundTriggerUtils.h"
#include "StreamSoundTrigger.h"
#include "PalRingBuffer.h"
#include "PayloadBuilder.h"

typedef enum {
    ENG_IDLE,
    ENG_LOADED,
    ENG_ACTIVE,
    ENG_BUFFERING,
    ENG_DETECTED,
} eng_state_t;

class Session;
class Stream;

class SoundTriggerEngineGsl : public SoundTriggerEngine {
 public:
    SoundTriggerEngineGsl(Stream *s,
               listen_model_indicator_enum type,
               st_module_type_t module_type,
               std::shared_ptr<VUIStreamConfig> sm_cfg);
    ~SoundTriggerEngineGsl();
    static std::shared_ptr<SoundTriggerEngineGsl> GetInstance(Stream *s,
                          listen_model_indicator_enum type,
                          st_module_type_t module_type,
                          std::shared_ptr<VUIStreamConfig> sm_cfg);
    int32_t LoadSoundModel(Stream *s, uint8_t *data,
                           uint32_t data_size) override;
    int32_t UnloadSoundModel(Stream *s) override;
    int32_t StartRecognition(Stream *s) override;
    int32_t RestartRecognition(Stream *s) override;
    int32_t StopRecognition(Stream *s) override;
    int32_t UpdateBufConfig(Stream *s, uint32_t hist_buffer_duration,
                            uint32_t pre_roll_duration) override;
    void GetUpdatedBufConfig(uint32_t *hist_buffer_duration,
                            uint32_t *pre_roll_duration) override;
     void SetDetected(bool detected __unused) {};
    int32_t GetParameters(uint32_t param_id, void **payload) override;
    int32_t ConnectSessionDevice(
        Stream* stream_handle,
        pal_stream_type_t stream_type,
        std::shared_ptr<Device> device_to_connect) override;
    int32_t DisconnectSessionDevice(
        Stream* stream_handle,
        pal_stream_type_t stream_type,
        std::shared_ptr<Device> device_to_disconnect,
        bool device_switch_event = false) override;
    int32_t SetupSessionDevice(
        Stream* stream_handle,
        pal_stream_type_t stream_type,
        std::shared_ptr<Device> device_to_disconnect) override;
    void SetCaptureRequested(bool is_requested) override;
    int32_t ReconfigureDetectionGraph(Stream *s) override;
    int32_t setECRef(
        Stream *s,
        std::shared_ptr<Device> dev,
        bool is_enable,
        bool setECForFirstTime = false) override;
    ChronoSteadyClock_t GetDetectedTime(Stream* s) {
        return detection_time_map_[s];
    }
    void UpdateStateToActive() override;
    void SetVoiceUIInterface(Stream *s,
        std::shared_ptr<VoiceUIInterface> intf) override;
    int32_t CreateBuffer(uint32_t buffer_size, uint32_t engine_size,
        std::vector<PalRingBufferReader *> &reader_list) override;
    int32_t SetBufferReader(PalRingBufferReader *reader) { return -ENOSYS;}
    int32_t ResetBufferReaders(std::vector<PalRingBufferReader *> &reader_list) override;
    bool CheckForStartRecognition() override;

 private:
    static void EventProcessingThread(SoundTriggerEngineGsl *gsl_engine);
    static void HandleSessionCallBack(uint64_t hdl, uint32_t event_id, void *data,
                                      uint32_t event_size);

    void ProcessEventTask();
    void HandleSessionEvent(uint32_t event_id __unused, void *data, uint32_t size);
    int32_t StartBuffering(Stream *s);
    int32_t RestartRecognition_l(Stream *s);
    int32_t UpdateSessionPayload(Stream *s, st_param_id_type_t param);

    bool CheckIfOtherStreamsAttached(Stream *s);
    Stream* GetOtherActiveStream(Stream *s);
    bool CheckIfOtherStreamsBuffering(Stream *s);
    int32_t HandleMultiStreamLoad(Stream *s, uint8_t *data, uint32_t data_size);
    int32_t HandleMultiStreamUnload(Stream *s);
    int32_t ProcessStartRecognition(Stream *s);
    int32_t ProcessStopRecognition(Stream *s);
    int32_t UpdateEngineConfigOnStop(Stream *s);
    int32_t UpdateEngineConfigOnStart(Stream *s);
    int32_t UpdateConfigPDK(uint32_t model_id);
    int32_t UpdateConfigsToSession(Stream *s);
    void UpdateState(eng_state_t state);
    bool IsEngineActive();
    std::vector<Stream *> GetBufferingStreams();
    void DetachStream(Stream *s, bool erase_engine);
    Session *session_;
    PayloadBuilder *builder_;
    st_module_type_t module_type_;
    static std::map<st_module_type_t,std::vector<std::shared_ptr<SoundTriggerEngineGsl>>>
                                                                      eng_;
    static std::map<Stream*, std::shared_ptr<SoundTriggerEngineGsl>> str_eng_map_;
    std::vector<Stream *> eng_streams_;
    std::queue<Stream *> det_streams_q_;
    struct keyword_stats kw1_stats_;

    int32_t dev_disconnect_count_;
    eng_state_t eng_state_;

    struct detection_engine_multi_model_buffering_config buffer_config_;

    /*
     * Used to indicate if VA engine supports
     * loading multi models separately.
     *
     * For PDK models: True
     * For GMM/HW models: False
     * For Custom models: Depends on Custom VA engine
     */
    bool is_multi_model_supported_;
    bool is_qc_wakeup_config_;
    bool is_crr_dev_using_ext_ec_;
    uint32_t lpi_miid_;
    uint32_t nlpi_miid_;
    bool use_lpi_;
    uint32_t module_tag_ids_[MAX_PARAM_IDS];
    uint32_t param_ids_[MAX_PARAM_IDS];
    struct pal_mmap_buffer mmap_buffer_;
    size_t mmap_buffer_size_;
    uint32_t mmap_write_position_;
    uint64_t kw_transfer_latency_;
    int32_t ec_ref_count_;
    std::map<Stream*, ChronoSteadyClock_t> detection_time_map_;
    std::mutex state_mutex_;
    std::mutex eos_mutex_;
    static std::mutex eng_create_mutex_;
    static int32_t engine_count_;
    std::shared_ptr<Device> rx_ec_dev_;
    std::recursive_mutex ec_ref_mutex_;
    Stream* device_switch_stream_;
};
#endif  // SOUNDTRIGGERENGINEGSL_H
