/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/core/AudioPatch.h>
#include <aidl/android/hardware/audio/core/AudioRoute.h>
#include <aidl/android/media/audio/common/AudioPort.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <aidl/android/media/audio/common/MicrophoneInfo.h>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

// { SEC_AUDIO_COMMON
#include <system/audio.h>
// } SEC_AUDIO_COMMON

namespace qti::audio::core {

#ifdef SEC_AUDIO_COMMON
static const std::string kGsiPrimaryModuleConfigFileName{"audio_module_config_primary_sec_on_gsi.xml"};
#endif
static const std::string kPrimaryModuleConfigFileName{"audio_module_config_primary.xml"};

class ModuleConfig {
  public:
    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> microphones;
    std::vector<::aidl::android::media::audio::common::AudioPort> ports;
    // Exclusive for external device ports and their possible profiles
    std::unordered_map<int32_t, std::vector<::aidl::android::media::audio::common::AudioProfile>>
            mExternalDevicePortProfiles;
    std::vector<::aidl::android::media::audio::common::AudioPortConfig> portConfigs;
    std::vector<::aidl::android::media::audio::common::AudioPortConfig> initialConfigs;
    // Port id -> List of profiles to use when the device port state is set to
    // 'connected' in connection simulation mode.
    std::map<int32_t, std::vector<::aidl::android::media::audio::common::AudioProfile>>
            connectedProfiles;
    std::vector<::aidl::android::hardware::audio::core::AudioRoute> routes;
    std::vector<::aidl::android::hardware::audio::core::AudioPatch> patches;
    std::string toString() const;
    int32_t nextPortId = 1;
    int32_t nextPatchId = 1;
#ifdef SEC_AUDIO_COMMON
    static std::unique_ptr<ModuleConfig> getPrimaryConfiguration(bool secAudioFeatureEnabled = false);
#else // QC
    static std::unique_ptr<ModuleConfig> getPrimaryConfiguration();
#endif
};

} // namespace qti::audio::core
