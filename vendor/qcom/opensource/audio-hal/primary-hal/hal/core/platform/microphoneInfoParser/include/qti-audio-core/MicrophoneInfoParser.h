/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalDefs.h>
#include <aidl/android/media/audio/common/MicrophoneDynamicInfo.h>
#include <aidl/android/media/audio/common/MicrophoneInfo.h>

#include <unordered_map>
#include <vector>

// Forward declare microphone_info classes
namespace qti::audio::microphone_info {
class FrequencyResponseType;
class MicrophoneInfoAndDynamicInfo;
}

using PalDevToMicDynamicInfoMap = std::unordered_map<
        pal_device_id_t, std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>>;

namespace qti::audio::core {
static const std::string kDefaultConfigName{"microphone_characteristics.xml"};

class MicrophoneInfoParser {
  public:
    MicrophoneInfoParser(const std::string &fileName = kDefaultConfigName);
    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> getMicrophoneInfo() {
        return mInfo;
    }
    PalDevToMicDynamicInfoMap getMicrophoneDynamicInfoMap() { return mDynamicInfoMap; }

  private:
    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> mInfo;
    PalDevToMicDynamicInfoMap mDynamicInfoMap;
    void populateMicrophoneInfo(
            const qti::audio::microphone_info::MicrophoneInfoAndDynamicInfo &xsdcConfig);
    void populateMicrophoneDynamicInfo(
            const qti::audio::microphone_info::MicrophoneInfoAndDynamicInfo &xsdcConfig);
    // qti::audio::microphone_info::MicrophoneInfoAndDynamicInfo mXsdcConfig;

    std::vector<::aidl::android::media::audio::common::MicrophoneInfo::FrequencyResponsePoint>
            getFrequencyResponse(
                    const qti::audio::microphone_info::FrequencyResponseType *freqResponse);
};

} // namespace qti::audio::core