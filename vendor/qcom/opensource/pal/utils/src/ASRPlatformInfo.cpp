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

#include "ASRPlatformInfo.h"
#include "asr_module_calibration_api.h"

#define LOG_TAG "PAL: ASRPlatformInfo"


void ASRCommonConfig::HandleStartTag(const char* tag, const char** attribs __unused)
{
    PAL_INFO(LOG_TAG, "Start tag %s", tag);

    if (!strcmp(tag, "param")) {
        uint32_t i = 0;
        while (attribs[i]) {
            if (!strcmp(attribs[i], "asr_input_buffer_size")) {
                input_buffer_size_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "asr_input_buffer_size_partial_mode")) {
                partial_mode_input_buffer_size_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "buffering_mode_out_buf_size")) {
                buffering_mode_out_buffer_size_ = std::stoi(attribs[++i]);
            } else if (!strcmp(attribs[i], "command_mode_timeout")) {
                command_mode_timeout_ = std::stoi(attribs[++i]);
            } else {
                PAL_ERR(LOG_TAG, "Invalid attribute %s", attribs[++i]);
            }
        }
    }
}

void ASRCommonConfig::HandleEndTag(struct xml_userdata *data, const char* tag_name)
{
    PAL_INFO(LOG_TAG, "Got end tag %s, nothing to handle here.", tag_name);

    return;
}

ASRCommonConfig::ASRCommonConfig():
    input_buffer_size_(0),
    partial_mode_input_buffer_size_(0),
    buffering_mode_out_buffer_size_(0),
    command_mode_timeout_(0)
{
}

uint32_t ASRCommonConfig::GetOutputBufferSize(int mode) {

    if (mode == BUFFERED)
        return GetBufferingModeOutBufferSize();

    return OUT_BUF_SIZE_DEFAULT;
}

uint32_t ASRCommonConfig::GetInputBufferSize(int mode) {

    if (mode == BUFFERED)
        return GetInputBufferSize();

    return GetPartialModeInputBufferSize();

}

void ASRStreamConfig::HandleStartTag(const char* tag, const char** attribs)
{
    PAL_INFO(LOG_TAG, "Got start tag %s", tag);

    if (!strcmp(tag, "operating_modes") || !strcmp(tag, "module_Info")
                                        || !strcmp(tag, "name")) {
        PAL_DBG(LOG_TAG, "tag:%s appeared, nothing to do", tag);
        return;
    }

    std::shared_ptr<SoundTriggerPlatformInfo> st_info = SoundTriggerPlatformInfo::GetInstance();
    if (!strcmp(tag, "param")) {
        uint32_t i = 0;
        while (attribs[i]) {
            uint32_t index = 0;
            if (!strcmp(attribs[i], "vendor_uuid")) {
                UUID::StringToUUID(attribs[++i], vendor_uuid_);
            } else {
                if (!strcmp(attribs[i], "asr_input_config_id")) {
                    index = ASR_INPUT_CONFIG;
                } else if (!strcmp(attribs[i], "asr_output_config_id")) {
                    index = ASR_OUTPUT_CONFIG;
                } else if (!strcmp(attribs[i], "asr_input_buffer_duration_id")) {
                    index = ASR_INPUT_BUF_DURATON;
                } else if (!strcmp(attribs[i], "asr_output_id")) {
                    index = ASR_OUTPUT;
                } else if (!strcmp(attribs[i], "asr_force_output_id")) {
                    index = ASR_FORCE_OUTPUT;
                } else {
                    PAL_ERR(LOG_TAG, "Invalid attribute %s", attribs[i++]);
                }
                sscanf(attribs[++i], "%x, %x", &module_tag_ids_[index], &param_ids_[index]);
                PAL_DBG(LOG_TAG, "index : %u, module_id : %x, param : %x",
                            index, module_tag_ids_[index], param_ids_[index]);
                ++i; /* move to next attribute */
            }
        }
    } else {
        if (!strcmp(tag, "low_power")) {
            st_info->ReadCapProfileNames(ST_OPERATING_MODE_LOW_POWER, attribs, asr_op_modes_);
        } else if (!strcmp(tag, "high_performance")) {
            st_info->ReadCapProfileNames(ST_OPERATING_MODE_HIGH_PERF, attribs, asr_op_modes_);
        }
    }
}

void ASRStreamConfig::HandleEndTag(struct xml_userdata *data, const char* tag)
{
    PAL_INFO(LOG_TAG, "Got end tag %s", tag);

    if (!strcmp(tag, "module_Info") || !strcmp(tag, "operating_modes")) {
        PAL_INFO(LOG_TAG, "Exit, Nothing to do for this %s tag.", tag);
    }

    if (!strcmp(tag, "name")) {
        if (data->offs <= 0)
            return;
        data->data_buf[data->offs] = '\0';

        std::string name(data->data_buf);
        name_ = name;
    }

    PAL_INFO(LOG_TAG, "Exit, for %s tag.", tag);

    return;
}

std::shared_ptr<ASRPlatformInfo> ASRPlatformInfo::me_ = nullptr;

ASRStreamConfig::ASRStreamConfig() :
    curr_child_(nullptr)
{
    for (int i = 0; i < ASR_MAX_PARAM_IDS; i++) {
        module_tag_ids_[i] = 0;
        param_ids_[i] = 0;
    }
}

ASRPlatformInfo::ASRPlatformInfo() :
    curr_child_(nullptr),
    cm_cfg_(nullptr)
{
}

std::shared_ptr<ASRPlatformInfo> ASRPlatformInfo::GetInstance()
{
    if (!me_)
        me_ = std::shared_ptr<ASRPlatformInfo> (new ASRPlatformInfo);

    return me_;
}

std::shared_ptr<ASRStreamConfig> ASRPlatformInfo::GetStreamConfig(const UUID& uuid) const
{
    auto smCfg = stream_cfg_list_.find(uuid);

    if (smCfg != stream_cfg_list_.end())
        return smCfg->second;
    else
        return nullptr;
}

void ASRPlatformInfo::HandleStartTag(const char* tag, const char** attribs)
{
    PAL_INFO(LOG_TAG, "Got start tag %s", tag);

    /* Delegate to child element if currently active */
    if (curr_child_) {
        curr_child_->HandleStartTag(tag, attribs);
        return;
    }

    if (!strcmp(tag, "stream_config")) {
        curr_child_ = std::static_pointer_cast<SoundTriggerXml>(
            std::make_shared<ASRStreamConfig>());
        return;
    } else if (!strcmp(tag, "common_config")) {
        curr_child_ = std::static_pointer_cast<SoundTriggerXml>(
                           std::make_shared<ASRCommonConfig>());
        return;
    } else {
        PAL_ERR(LOG_TAG, "Invalid tag %s", tag);
    }

    PAL_INFO(LOG_TAG, "Exit for tag %s.", tag);
}

void ASRPlatformInfo::HandleEndTag(struct xml_userdata *data, const char* tag)
{
    PAL_INFO(LOG_TAG, "Got end tag %s", tag);

    if (!strcmp(tag, "stream_config")) {
        std::shared_ptr<ASRStreamConfig> sm_cfg(
            std::static_pointer_cast<ASRStreamConfig>(curr_child_));
        const auto res = stream_cfg_list_.insert(
            std::make_pair(sm_cfg->GetUUID(), sm_cfg));
        if (!res.second)
            PAL_ERR(LOG_TAG, "Failed to insert to map");
        curr_child_ = nullptr;
    } else if (!strcmp(tag, "common_config")) {
        std::shared_ptr<ASRCommonConfig> cm_cfg(
             std::static_pointer_cast<ASRCommonConfig>(curr_child_));
        cm_cfg_ = cm_cfg;
        curr_child_ = nullptr;
    }

    if (curr_child_)
        curr_child_->HandleEndTag(data, tag);

    PAL_DBG(LOG_TAG, "Exit for tag %s.", tag);

    return;
}
