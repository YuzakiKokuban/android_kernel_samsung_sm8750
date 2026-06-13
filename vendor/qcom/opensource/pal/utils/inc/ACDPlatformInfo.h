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
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ACD_PLATFORM_INFO_H
#define ACD_PLATFORM_INFO_H

#include "ResourceManager.h"
#include "SoundTriggerPlatformInfo.h"

class ACDStreamConfig;

class ACDContextInfo
{
public:
    ACDContextInfo(uint32_t context_id, uint32_t type);
    uint32_t GetContextId() const { return context_id_; }
    uint32_t GetContextType() const { return context_type_; }

private:
    uint32_t context_id_;
    uint32_t context_type_;
};

class ACDSoundModelInfo : public SoundTriggerXml
{
public:
    ACDSoundModelInfo(ACDStreamConfig *sm_cfg);

    void HandleStartTag(const char *tag, const char **attribs) override;
    void HandleEndTag(struct xml_userdata *data, const char *tag) override;

    std::string GetModelType() const { return model_type_; }
    std::string GetModelBinName() const { return model_bin_name_; }
    uint32_t GetModelUUID() const { return model_uuid_; }
    uint32_t GetModelId() const { return model_id_; }
    size_t GetNumContexts() const { return acd_context_info_list_.size(); }
    std::vector<std::shared_ptr<ACDContextInfo>> GetSupportedContextList()const {
        return acd_context_info_list_;
    }

private:
    std::string  model_type_;
    std::string  model_bin_name_;
    uint32_t     model_id_;
    uint32_t     model_uuid_;
    bool         is_parsing_contexts;
    ACDStreamConfig *sm_cfg_;
    std::vector<std::shared_ptr<ACDContextInfo>> acd_context_info_list_;
};

class ACDStreamConfig : public SoundTriggerXml
{
public:
    ACDStreamConfig();
    ACDStreamConfig(ACDStreamConfig &rhs) = delete;
    ACDStreamConfig & operator=(ACDStreamConfig &rhs) = delete;

    void HandleStartTag(const char *tag, const char **attribs) override;
    void HandleEndTag(struct xml_userdata *data, const char *tag) override;

    UUID GetUUID() const { return vendor_uuid_; }
    std::string GetStreamConfigName() const { return name_; }
    std::shared_ptr<ACDSoundModelInfo> GetSoundModelInfoByModelId(uint32_t model_id);
    std::shared_ptr<ACDSoundModelInfo> GetSoundModelInfoByContextId(uint32_t context_id);
    void UpdateContextModelMap(uint32_t context_id);
    uint32_t GetSampleRate() const { return sample_rate_; }
    uint32_t GetBitWidth() const { return bit_width_; }
    uint32_t GetOutChannels() const { return out_channels_; }
    std::vector<std::shared_ptr<ACDSoundModelInfo>> GetSoundModelList() const {
        return acd_soundmodel_info_list_;
    }
    std::shared_ptr<CaptureProfile> GetCaptureProfile(
        std::pair<StOperatingModes, StInputModes> mode_pair) const {
        return acd_op_modes_.at(mode_pair);
    }
    uint32_t GetAndUpdateSndMdlCnt() { return sound_model_cnt++; }

private:
    std::string name_;
    UUID vendor_uuid_;
    uint32_t sample_rate_;
    uint32_t bit_width_;
    uint32_t out_channels_;
    st_op_modes_t acd_op_modes_;
    std::shared_ptr<SoundTriggerXml> curr_child_;
    std::vector<std::shared_ptr<ACDSoundModelInfo>> acd_soundmodel_info_list_;
    std::map<uint32_t, std::shared_ptr<ACDSoundModelInfo>> context_model_map_;
    std::map<uint32_t, std::shared_ptr<ACDSoundModelInfo>> acd_modelinfo_map_;
    uint32_t sound_model_cnt;
};

class ACDPlatformInfo : public SoundTriggerPlatformInfo
{
public:
    ACDPlatformInfo();
    ACDPlatformInfo(ACDStreamConfig &rhs) = delete;
    ACDPlatformInfo & operator=(ACDPlatformInfo &rhs) = delete;

    void HandleStartTag(const char *tag, const char **attribs) override;
    void HandleEndTag(struct xml_userdata *data, const char *tag) override;

    static std::shared_ptr<ACDPlatformInfo> GetInstance();
    bool IsACDEnabled() const { return acd_enable_; }
    std::shared_ptr<ACDStreamConfig> GetStreamConfig(const UUID& uuid) const;

private:
    bool acd_enable_;
    static std::shared_ptr<ACDPlatformInfo> me_;
    std::shared_ptr<SoundTriggerXml> curr_child_;
    std::map<UUID, std::shared_ptr<ACDStreamConfig>> acd_cfg_list_;
};
#endif
