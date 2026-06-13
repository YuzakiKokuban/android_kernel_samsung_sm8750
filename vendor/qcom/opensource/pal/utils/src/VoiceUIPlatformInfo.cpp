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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "VoiceUIPlatformInfo.h"
#include "PalCommon.h"
#include "SoundTriggerUtils.h"

#define LOG_TAG "PAL: VoiceUIPlatformInfo"

static const struct st_uuid qcva_uuid =
    { 0x68ab2d40, 0xe860, 0x11e3, 0x95ef, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } };

VUISecondStageConfig::VUISecondStageConfig() :
    detection_type_(ST_SM_TYPE_NONE),
    sm_id_(0),
    module_lib_(""),
    sample_rate_(16000),
    bit_width_(16),
    channels_(1)
{
}

void VUISecondStageConfig::HandleStartTag(const char *tag, const char **attribs)
{
    PAL_DBG(LOG_TAG, "Got start tag %s", tag);

    if (!strcmp(tag, "param")) {
        uint32_t i = 0;
        while (attribs[i]) {
            if (!strcmp(attribs[i], "sm_detection_type")) {
                i++;
                if (!strcmp(attribs[i], "KEYWORD_DETECTION")) {
                    detection_type_ = ST_SM_TYPE_KEYWORD_DETECTION;
                } else if (!strcmp(attribs[i], "USER_VERIFICATION")) {
                    detection_type_ = ST_SM_TYPE_USER_VERIFICATION;
                } else if (!strcmp(attribs[i], "CUSTOM_DETECTION")) {
                    detection_type_ = ST_SM_TYPE_CUSTOM_DETECTION;
                }
            } else if (!strcmp(attribs[i], "sm_id")) {
                sm_id_ = std::strtoul(attribs[++i], nullptr, 16);
            } else if (!strcmp(attribs[i], "module_lib")) {
                module_lib_ = attribs[++i];
            } else if (!strcmp(attribs[i], "sample_rate")) {
                sample_rate_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "bit_width")) {
                bit_width_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "channel_count")) {
                channels_ = std::stoi(attribs[++i]);
            }
            ++i;
        }
    } else {
        PAL_ERR(LOG_TAG, "Invalid tag %s", tag);
    }
}

VUIFirstStageConfig::VUIFirstStageConfig() :
    module_type_(ST_MODULE_TYPE_GMM),
    module_name_("GMM"),
    lpi_supported_(true)
{
    for (int i = 0; i < MAX_PARAM_IDS; i++) {
        module_tag_ids_[i] = 0;
        param_ids_[i] = 0;
    }
}

void VUIFirstStageConfig::HandleStartTag(const char *tag, const char **attribs)
{
    PAL_DBG(LOG_TAG, "Got start tag %s", tag);

    if (!strcmp(tag, "param")) {
        uint32_t i = 0;

        while (attribs[i]) {
            if (!strcmp(attribs[i], "module_type")) {
                i++;
                module_name_ = attribs[i];
                if (!strcmp(attribs[i], "GMM")) {
                    module_type_ = ST_MODULE_TYPE_GMM;
                } else if (!strcmp(attribs[i], "PDK")) {
                    module_type_ = ST_MODULE_TYPE_PDK;
                } else if (!strcmp(attribs[i], "HOTWORD")) {
                    module_type_ = ST_MODULE_TYPE_HW;
                } else if (!strcmp(attribs[i], "CUSTOM1")) {
                    module_type_ = ST_MODULE_TYPE_CUSTOM_1;
                } else if (!strcmp(attribs[i], "CUSTOM2")) {
                    module_type_ = ST_MODULE_TYPE_CUSTOM_2;
                } else if (!strcmp(attribs[i], "MMA")) {
                    module_type_ = ST_MODULE_TYPE_MMA;
                }
                PAL_DBG(LOG_TAG, "Module name:%s, type:%d",
                        module_name_.c_str(), module_type_);
            } else if (!strcmp(attribs[i], "lpi_supported")) {
                lpi_supported_ = !strcmp(attribs[++i], "true");
            } else {
                uint32_t index = 0;

                if (!strcmp(attribs[i], "load_sound_model_ids")) {
                    index = LOAD_SOUND_MODEL;
                } else if (!strcmp(attribs[i], "unload_sound_model_ids")) {
                    index = UNLOAD_SOUND_MODEL;
                } else if (!strcmp(attribs[i], "wakeup_config_ids")) {
                    index = WAKEUP_CONFIG;
                } else if (!strcmp(attribs[i], "buffering_config_ids")) {
                    index = BUFFERING_CONFIG;
                } else if (!strcmp(attribs[i], "engine_reset_ids")) {
                    index = ENGINE_RESET;
                } else if (!strcmp(attribs[i], "custom_config_ids")) {
                    index = CUSTOM_CONFIG;
                } else if (!strcmp(attribs[i], "version_ids")) {
                    index = MODULE_VERSION;
                } else if (!strcmp(attribs[i], "engine_per_model_reset_ids")) {
                    index = ENGINE_PER_MODEL_RESET;
                }
                sscanf(attribs[++i], "%x, %x", &module_tag_ids_[index],
                       &param_ids_[index]);
                PAL_DBG(LOG_TAG, "index : %u, module_id : %x, param : %x",
                        index, module_tag_ids_[index], param_ids_[index]);
            }
            ++i;
        }
    } else {
        PAL_ERR(LOG_TAG, "Invalid tag %s", tag);
    }
}

VUIStreamConfig::VUIStreamConfig() :
    vui_intf_plugin_lib_name_(""),
    is_qcva_uuid_(false),
    merge_first_stage_sound_models_(false),
    capture_keyword_(2000),
    client_capture_read_delay_(2000),
    pre_roll_duration_(0),
    supported_first_stage_engine_count_(1),
    enable_concurrent_event_capture_(false),
    curr_child_(nullptr)
{
    ext_det_prop_list_.clear();
}

/*
 * Below functions GetVUIFirstStageConfig(), GetVUIModuleType(),
 * and GetVUIModuleName() are to be used only for getting module
 * info and module type/name for third party or custom sound model
 * engines. It assumes only one module type per vendor UUID.
 */
std::shared_ptr<VUIFirstStageConfig> VUIStreamConfig::GetVUIFirstStageConfig()
{
    auto smCfg = vui_uuid_1st_stage_cfg_list_.find(vendor_uuid_);

    if(smCfg != vui_uuid_1st_stage_cfg_list_.end())
         return smCfg->second;
    else
        return nullptr;
}

st_module_type_t VUIStreamConfig::GetVUIModuleType()
{
    std::shared_ptr<VUIFirstStageConfig> sTModuleInfo = GetVUIFirstStageConfig();

    if (sTModuleInfo != nullptr)
        return sTModuleInfo->GetModuleType();
    else
        return ST_MODULE_TYPE_NONE;
}

std::string VUIStreamConfig::GetVUIModuleName()
{
    std::shared_ptr<VUIFirstStageConfig> sTModuleInfo = GetVUIFirstStageConfig();

    if (sTModuleInfo != nullptr)
        return sTModuleInfo->GetModuleName();
    else
        return std::string();
}

std::shared_ptr<VUISecondStageConfig> VUIStreamConfig::GetVUISecondStageConfig(
    const listen_model_indicator_enum& sm_type) const
{
    uint32_t sm_id = static_cast<uint32_t>(sm_type);
    auto ss_config = vui_2nd_stage_cfg_list_.find(sm_id);

    if (ss_config != vui_2nd_stage_cfg_list_.end())
        return ss_config->second;
    else
        return nullptr;
}

std::shared_ptr<VUIFirstStageConfig> VUIStreamConfig::GetVUIFirstStageConfig(const uint32_t type) const
{
    uint32_t module_type = type;

    PAL_DBG(LOG_TAG, "search module for model type %u", type);
    if (IS_MODULE_TYPE_PDK(type)) {
        PAL_DBG(LOG_TAG, "PDK module");
        module_type = ST_MODULE_TYPE_PDK;
    }

    auto module_config = vui_1st_stage_cfg_list_.find(module_type);
    if (module_config != vui_1st_stage_cfg_list_.end())
        return module_config->second;
    else
        return nullptr;
}

void VUIStreamConfig::GetDetectionPropertyList(
    std::vector<uint32_t> &list) {

    for (int i = 0; i < ext_det_prop_list_.size(); i++)
        list.push_back(ext_det_prop_list_[i]);
}

void VUIStreamConfig::ReadDetectionPropertyList(const char *prop_string)
{
    int ret = 0;
    char *token = nullptr;
    char *delims = ",";
    char *save = nullptr;

    PAL_VERBOSE(LOG_TAG, "Detection property list %s", prop_string);

    token = strtok_r(const_cast<char *>(prop_string), delims, &save);
    while (token) {
        ext_det_prop_list_.push_back(std::strtoul(token, nullptr, 16));
        token = strtok_r(NULL, delims, &save);
    }

    for (int i = 0; i < ext_det_prop_list_.size(); i++) {
        PAL_INFO(LOG_TAG, "Found extension detection property 0x%x",
            ext_det_prop_list_[i]);
    }
}

bool VUIStreamConfig::IsDetPropSupported(uint32_t prop) const {

    auto iter =
        std::find(ext_det_prop_list_.begin(), ext_det_prop_list_.end(), prop);

    return iter != ext_det_prop_list_.end();
}

void VUIStreamConfig::HandleStartTag(const char* tag, const char** attribs)
{
    /* Delegate to child element if currently active */
    if (curr_child_) {
        curr_child_->HandleStartTag(tag, attribs);
        return;
    }

    PAL_DBG(LOG_TAG, "Got start tag %s", tag);

    if (!strcmp(tag, "first_stage_module_params")) {
        auto st_module_info_ = std::make_shared<VUIFirstStageConfig>();
        vui_uuid_1st_stage_cfg_list_.insert(std::make_pair(vendor_uuid_, st_module_info_));
        curr_child_ = std::static_pointer_cast<SoundTriggerXml>(st_module_info_);
        return;
    }

    if (!strcmp(tag, "arm_ss_module_params")) {
        curr_child_ = std::static_pointer_cast<SoundTriggerXml>(
            std::make_shared<VUISecondStageConfig>());
        return;
    }

    if (!strcmp(tag, "operating_modes") || !strcmp(tag, "sound_model_info")
                                        || !strcmp(tag, "name")) {
        PAL_DBG(LOG_TAG, "tag:%s appeared, nothing to do", tag);
        return;
    }

    std::shared_ptr<SoundTriggerPlatformInfo> st_info = SoundTriggerPlatformInfo::GetInstance();
    if (!strcmp(tag, "param")) {
        uint32_t i = 0;
        while (attribs[i]) {
            if (!strcmp(attribs[i], "vendor_uuid")) {
                UUID::StringToUUID(attribs[++i], vendor_uuid_);
                if (vendor_uuid_.CompareUUID(qcva_uuid))
                    is_qcva_uuid_ = true;
            } else if (!strcmp(attribs[i], "interface_plugin_lib")) {
                vui_intf_plugin_lib_name_ = attribs[++i];
            } else if (!strcmp(attribs[i], "get_module_version")) {
                get_module_version_supported_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "merge_first_stage_sound_models")) {
                merge_first_stage_sound_models_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "pdk_first_stage_max_engine_count")) {
                supported_first_stage_engine_count_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "enable_concurrent_event_capture")) {
                enable_concurrent_event_capture_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "capture_keyword")) {
                capture_keyword_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "client_capture_read_delay")) {
                client_capture_read_delay_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "pre_roll_duration")) {
                pre_roll_duration_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "kw_start_tolerance")) {
                kw_start_tolerance_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "kw_end_tolerance")) {
                kw_end_tolerance_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "data_before_kw_start")) {
                data_before_kw_start_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "data_after_kw_end")) {
                data_after_kw_end_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "sample_rate")) {
                sample_rate_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "bit_width")) {
                bit_width_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "out_channels")) {
                if (std::stoi(attribs[++i]) <= MAX_MODULE_CHANNELS)
                    out_channels_ = std::stoi(attribs[i]);
            } else if (!strcmp(attribs[i], "detection_property_list")) {
                ReadDetectionPropertyList(attribs[++i]);
            } else {
                PAL_ERR(LOG_TAG, "Invalid attribute %s", attribs[i++]);
            }
            ++i; /* move to next attribute */
        }
    } else if (!strcmp(tag, "low_power")) {
        st_info->ReadCapProfileNames(ST_OPERATING_MODE_LOW_POWER, attribs, vui_op_modes_);
    } else if (!strcmp(tag, "high_performance")) {
        st_info->ReadCapProfileNames(ST_OPERATING_MODE_HIGH_PERF, attribs, vui_op_modes_);
    } else if (!strcmp(tag, "high_performance_and_charging")) {
        st_info->ReadCapProfileNames(ST_OPERATING_MODE_HIGH_PERF_AND_CHARGING, attribs, vui_op_modes_);
    } else {
        PAL_ERR(LOG_TAG, "Invalid tag %s", (char *)tag);
    }
}

void VUIStreamConfig::HandleEndTag(struct xml_userdata *data, const char* tag)
{
    PAL_DBG(LOG_TAG, "Got end tag %s", tag);

    if (!strcmp(tag, "first_stage_module_params")) {
        std::shared_ptr<VUIFirstStageConfig> st_module_info(
            std::static_pointer_cast<VUIFirstStageConfig>(curr_child_));
        const auto res = vui_1st_stage_cfg_list_.insert(
            std::make_pair(st_module_info->GetModuleType(), st_module_info));

        if (!res.second)
            PAL_ERR(LOG_TAG, "Failed to insert to map");
        curr_child_ = nullptr;
    } else if (!strcmp(tag, "arm_ss_module_params")) {
        std::shared_ptr<VUISecondStageConfig> ss_cfg(
            std::static_pointer_cast<VUISecondStageConfig>(curr_child_));
        const auto res = vui_2nd_stage_cfg_list_.insert(
            std::make_pair(ss_cfg->GetSoundModelID(), ss_cfg));

        if (!res.second)
            PAL_ERR(LOG_TAG, "Failed to insert to map");
        curr_child_ = nullptr;
    }

    if (curr_child_)
        curr_child_->HandleEndTag(data, tag);

    return;
}

std::shared_ptr<VoiceUIPlatformInfo> VoiceUIPlatformInfo::me_ = nullptr;

VoiceUIPlatformInfo::VoiceUIPlatformInfo() :
    enable_failure_detection_(false),
    transit_to_non_lpi_on_charging_(false),
    notify_second_stage_failure_(false),
    mmap_enable_(false),
    mmap_buffer_duration_(0),
    mmap_frame_length_(0),
    sound_model_lib_("liblistensoundmodel2vendor.so"),
    curr_child_(nullptr)
{
}

std::shared_ptr<VUIStreamConfig> VoiceUIPlatformInfo::GetStreamConfig(const UUID& uuid) const
{
    auto smCfg = stream_cfg_list_.find(uuid);

    if (smCfg != stream_cfg_list_.end())
        return smCfg->second;
    else
        return nullptr;
}

// We can assume only Hotword sm config supports version api
void VoiceUIPlatformInfo::GetStreamConfigForVersionQuery(
    std::vector<std::shared_ptr<VUIStreamConfig>> &sm_cfg_list) const
{
    std::shared_ptr<VUIStreamConfig> sm_cfg = nullptr;

    for (auto iter = stream_cfg_list_.begin();
         iter != stream_cfg_list_.end(); iter++) {
        sm_cfg = iter->second;
        if (sm_cfg && sm_cfg->GetModuleVersionSupported())
            sm_cfg_list.push_back(sm_cfg);
    }
}

std::shared_ptr<VoiceUIPlatformInfo> VoiceUIPlatformInfo::GetInstance()
{
    if (!me_)
        me_ = std::shared_ptr<VoiceUIPlatformInfo> (new VoiceUIPlatformInfo);

    return me_;
}

void VoiceUIPlatformInfo::HandleStartTag(const char* tag, const char** attribs)
{
    /* Delegate to child element if currently active */
    if (curr_child_) {
        curr_child_->HandleStartTag(tag, attribs);
        return;
    }

    PAL_DBG(LOG_TAG, "Got start tag %s", tag);

    if (!strcmp(tag, "stream_config")) {
        curr_child_ = std::static_pointer_cast<SoundTriggerXml>(
            std::make_shared<VUIStreamConfig>());
        return;
    }

    if (!strcmp(tag, "config")) {
        PAL_DBG(LOG_TAG, "tag:%s appeared, nothing to do", tag);
        return;
    }

    if (!strcmp(tag, "param")) {
        uint32_t i = 0;
        while (attribs[i]) {
            if (!attribs[i]) {
                PAL_ERR(LOG_TAG,"missing attrib value for tag %s", tag);
            } else if (!strcmp(attribs[i], "version")) {
                vui_version_ = std::strtoul(attribs[++i], nullptr, 16);
            } else if (!strcmp(attribs[i], "enable_failure_detection")) {
                enable_failure_detection_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "transit_to_non_lpi_on_charging")) {
                transit_to_non_lpi_on_charging_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "notify_second_stage_failure")) {
                notify_second_stage_failure_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "mmap_enable")) {
                mmap_enable_ =
                    !strncasecmp(attribs[++i], "true", 4) ? true : false;
            } else if (!strcmp(attribs[i], "mmap_buffer_duration")) {
                mmap_buffer_duration_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "mmap_frame_length")) {
                mmap_frame_length_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "sound_model_lib")) {
                sound_model_lib_ = std::string(attribs[++i]);
            } else {
                PAL_ERR(LOG_TAG, "Invalid attribute %s", attribs[i++]);
            }
            ++i; /* move to next attribute */
        }
    } else {
        PAL_ERR(LOG_TAG, "Invalid tag %s", tag);
    }
}

void VoiceUIPlatformInfo::HandleEndTag(struct xml_userdata *data, const char* tag)
{
    PAL_DBG(LOG_TAG, "Got end tag %s", tag);

    if (!strcmp(tag, "stream_config")) {
        std::shared_ptr<VUIStreamConfig> sm_cfg(
            std::static_pointer_cast<VUIStreamConfig>(curr_child_));
        const auto res = stream_cfg_list_.insert(
            std::make_pair(sm_cfg->GetUUID(), sm_cfg));

        if (!res.second)
            PAL_ERR(LOG_TAG, "Failed to insert to map");
        curr_child_ = nullptr;
    }

    if (curr_child_)
        curr_child_->HandleEndTag(data, tag);

    return;
}
