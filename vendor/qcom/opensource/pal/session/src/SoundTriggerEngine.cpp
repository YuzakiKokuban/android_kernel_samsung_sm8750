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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define LOG_TAG "PAL: SoundTriggerEngine"

#include "SoundTriggerEngine.h"

#include "SoundTriggerEngineGsl.h"
#include "SoundTriggerEngineCapi.h"
#include "Stream.h"
#include "SoundTriggerPlatformInfo.h"

std::shared_ptr<SoundTriggerEngine> SoundTriggerEngine::Create(
    Stream *s,
    listen_model_indicator_enum type,
    st_module_type_t module_type,
    std::shared_ptr<VUIStreamConfig> sm_cfg)
{
    PAL_VERBOSE(LOG_TAG, "Enter, type %d", type);

    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream handle");
        return nullptr;
    }

    std::shared_ptr<SoundTriggerEngine> st_engine(nullptr);

    switch (type) {
    case ST_SM_ID_SVA_F_STAGE_GMM:
        if (!sm_cfg->GetMergeFirstStageSoundModels() &&
            !IS_MODULE_TYPE_PDK(module_type))
            st_engine = std::make_shared<SoundTriggerEngineGsl>(s, type,
                                          module_type, sm_cfg);
        else
            st_engine = SoundTriggerEngineGsl::GetInstance(s, type,
                                                   module_type, sm_cfg);

        if (!st_engine)
            PAL_ERR(LOG_TAG, "SoundTriggerEngine GSL creation failed");
        break;

    case ST_SM_ID_SVA_S_STAGE_PDK:
    case ST_SM_ID_SVA_S_STAGE_RNN:
    case ST_SM_ID_SVA_S_STAGE_USER:
    case ST_SM_ID_SVA_S_STAGE_UDK:
        st_engine = std::make_shared<SoundTriggerEngineCapi>(s, type,
                                                              sm_cfg);
        if (!st_engine)
            PAL_ERR(LOG_TAG, "SoundTriggerEngine capi creation failed");
        break;

    default:
        PAL_ERR(LOG_TAG, "Invalid model type: %u", type);
        break;
    }

    PAL_VERBOSE(LOG_TAG, "Exit, engine %p", st_engine.get());

    return st_engine;
}

uint32_t SoundTriggerEngine::UsToBytes(uint64_t input_us) {
    uint32_t bytes = 0;

    bytes = sample_rate_ * bit_width_ * channels_ * input_us /
        (BITS_PER_BYTE * US_PER_SEC);

    return bytes;
}

uint32_t SoundTriggerEngine::FrameToBytes(uint32_t frames) {
    uint32_t total_bytes, bytes_per_frame = bit_width_ * channels_ / BITS_PER_BYTE;
    try {
        if ((bytes_per_frame) > (UINT32_MAX / frames))
            throw "multiplication overflow due to frames value";

        total_bytes = frames * bytes_per_frame;
    } catch (const char *e) {
        PAL_ERR(LOG_TAG, "FrametoBytes() failed with error %s . frames %u bit_width %u channels: %u",
                e, frames, bit_width_, channels_);
        return UINT32_MAX;
    }
    return total_bytes;
}

uint32_t SoundTriggerEngine::BytesToFrames(uint32_t bytes) {
    return (bytes * BITS_PER_BYTE) / (bit_width_ * channels_);
}
