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
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VOICE_UI_PLATFORM_INFO_H
#define VOICE_UI_PLATFORM_INFO_H

#include "SoundTriggerPlatformInfo.h"

#define MAX_MODULE_CHANNELS 4

class VUISecondStageConfig : public SoundTriggerXml
{
public:
    VUISecondStageConfig();

    void HandleStartTag(const char *tag, const char **attribs) override;

    st_sound_model_type_t GetDetectionType() const { return detection_type_; }
    std::string GetLibName() const { return module_lib_; }
    uint32_t GetSoundModelID() const { return sm_id_; }
    uint32_t GetSampleRate() const { return sample_rate_; }
    uint32_t GetBitWidth() const { return bit_width_; }
    uint32_t GetChannels() const { return channels_; }

private:
    st_sound_model_type_t detection_type_;
    uint32_t sm_id_;
    std::string module_lib_;
    uint32_t sample_rate_;
    uint32_t bit_width_;
    uint32_t channels_;
};

class VUIFirstStageConfig : public SoundTriggerXml
{
public:
    VUIFirstStageConfig();

    void HandleStartTag(const char *tag, const char **attribs) override;

    st_module_type_t GetModuleType() const { return module_type_; }
    std::string GetModuleName() const { return module_name_; }
    bool IsLpiSupported() const { return lpi_supported_; }
    uint32_t GetModuleTagId(st_param_id_type_t param_id) const {
        return module_tag_ids_[param_id];
    }
    uint32_t GetParamId(st_param_id_type_t param_id) const {
        return param_ids_[param_id];
    }

private:
    bool lpi_supported_;
    st_module_type_t module_type_;
    std::string module_name_;
    uint32_t module_tag_ids_[MAX_PARAM_IDS];
    uint32_t param_ids_[MAX_PARAM_IDS];
};

class VUIStreamConfig : public SoundTriggerXml
{
public:
    VUIStreamConfig();
    VUIStreamConfig(VUIStreamConfig &rhs) = delete;
    VUIStreamConfig & operator=(VUIStreamConfig &rhs) = delete;

    void HandleStartTag(const char *tag, const char **attribs) override;
    void HandleEndTag(struct xml_userdata *data, const char *tag) override;

    UUID GetUUID() const { return vendor_uuid_; }
    std::string GetVUIIntfPluginLib() const { return vui_intf_plugin_lib_name_; }
    bool GetModuleVersionSupported() const {
        return get_module_version_supported_;
    }
    bool GetMergeFirstStageSoundModels() const {
        return merge_first_stage_sound_models_;
    }
    bool isSingleInstanceStage1() const { return supported_first_stage_engine_count_ == 1; }
    bool isQCVAUUID() const { return is_qcva_uuid_; }
    uint32_t GetKwDuration() const { return capture_keyword_; }
    uint32_t GetCaptureReadDelay() const { return client_capture_read_delay_; }
    uint32_t GetPreRollDuration() const { return pre_roll_duration_; }
    uint32_t GetKwStartTolerance() const { return kw_start_tolerance_; }
    uint32_t GetKwEndTolerance() const { return kw_end_tolerance_; }
    uint32_t GetDataBeforeKwStart() const { return data_before_kw_start_; }
    uint32_t GetDataAfterKwEnd() const { return data_after_kw_end_; }
    uint32_t GetSampleRate() const { return sample_rate_; }
    uint32_t GetBitWidth() const { return bit_width_; }
    uint32_t GetOutChannels() const { return out_channels_; }
    uint32_t GetSupportedEngineCount() const {
                        return supported_first_stage_engine_count_; }
    bool GetConcurrentEventCapture() const { return enable_concurrent_event_capture_; }
    st_module_type_t GetVUIModuleType();
    std::shared_ptr<VUISecondStageConfig> GetVUISecondStageConfig(
        const listen_model_indicator_enum& sm_type) const;
    std::shared_ptr<VUIFirstStageConfig> GetVUIFirstStageConfig(const uint32_t type) const;
    std::shared_ptr<VUIFirstStageConfig> GetVUIFirstStageConfig();
    std::string GetVUIModuleName();
    std::string GetVUIModuleName(st_module_type_t type) const {
        return GetVUIFirstStageConfig(type)->GetModuleName();
    }
    std::shared_ptr<CaptureProfile> GetCaptureProfile(
        std::pair<StOperatingModes, StInputModes> mode_pair) const {
        return vui_op_modes_.at(mode_pair);
    }
    void GetDetectionPropertyList(std::vector<uint32_t> &list);
    void ReadDetectionPropertyList(const char *prop_string);
    bool IsDetPropSupported(uint32_t prop) const;

private:
    std::string name_;
    UUID vendor_uuid_;
    std::string vui_intf_plugin_lib_name_;
    bool is_qcva_uuid_;
    bool get_module_version_supported_;
    bool merge_first_stage_sound_models_;
    uint32_t capture_keyword_;
    uint32_t client_capture_read_delay_;
    uint32_t pre_roll_duration_;
    uint32_t kw_start_tolerance_;
    uint32_t kw_end_tolerance_;
    uint32_t data_before_kw_start_;
    uint32_t data_after_kw_end_;
    uint32_t sample_rate_;
    uint32_t bit_width_;
    uint32_t out_channels_;
    uint32_t supported_first_stage_engine_count_;
    bool enable_concurrent_event_capture_;
    st_op_modes_t vui_op_modes_;
    std::shared_ptr<SoundTriggerXml> curr_child_;
    std::map<uint32_t, std::shared_ptr<VUISecondStageConfig>> vui_2nd_stage_cfg_list_;
    std::map<uint32_t, std::shared_ptr<VUIFirstStageConfig>> vui_1st_stage_cfg_list_;
    std::map<UUID, std::shared_ptr<VUIFirstStageConfig>> vui_uuid_1st_stage_cfg_list_;
    std::vector<uint32_t> ext_det_prop_list_;
};

class VoiceUIPlatformInfo : public SoundTriggerPlatformInfo
{
public:
    VoiceUIPlatformInfo();
    VoiceUIPlatformInfo(VUIStreamConfig &rhs) = delete;
    VoiceUIPlatformInfo & operator=(VoiceUIPlatformInfo &rhs) = delete;

    void HandleStartTag(const char *tag, const char **attribs) override;
    void HandleEndTag(struct xml_userdata *data, const char *tag) override;

    static std::shared_ptr<VoiceUIPlatformInfo> GetInstance();
    std::string GetSoundModelLib() const { return sound_model_lib_; }
    bool GetEnableFailureDetection() const { return enable_failure_detection_; }
    bool GetTransitToNonLpiOnCharging() const {
        return transit_to_non_lpi_on_charging_;
    }
    bool GetMmapEnable() const { return mmap_enable_; }
    bool GetNotifySecondStageFailure() { return notify_second_stage_failure_; }
    uint32_t GetVersion() const { return vui_version_; }
    uint32_t GetMmapBufferDuration() const { return mmap_buffer_duration_; }
    uint32_t GetMmapFrameLength() const { return mmap_frame_length_; }
    std::shared_ptr<VUIStreamConfig> GetStreamConfig(const UUID& uuid) const;
    void GetStreamConfigForVersionQuery(std::vector<std::shared_ptr<VUIStreamConfig>> &sm_cfg_list) const;

private:
    static std::shared_ptr<VoiceUIPlatformInfo> me_;
    uint32_t vui_version_;
    bool enable_failure_detection_;
    bool transit_to_non_lpi_on_charging_;
    bool notify_second_stage_failure_;
    bool mmap_enable_;
    uint32_t mmap_buffer_duration_;
    uint32_t mmap_frame_length_;
    std::string sound_model_lib_;
    std::map<UUID, std::shared_ptr<VUIStreamConfig>> stream_cfg_list_;
    std::shared_ptr<SoundTriggerXml> curr_child_;
};
#endif
