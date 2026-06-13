/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <vector>

/* AIDL types */
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceAddress.h>
#include <aidl/android/media/audio/common/AudioDeviceDescription.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/AudioOutputFlags.h>
#include <aidl/android/media/audio/common/PcmType.h>
/* PAL types */
#include <PalDefs.h>

namespace qti::audio {

/**
 * monostate singleton class
 * APIs are designed to convert any AIDL Audio type to PAL type or vice versa
 **/
class PlatformConverter {
  private:
    PlatformConverter() = delete;

    PlatformConverter(const PlatformConverter&) = delete;
    PlatformConverter& operator=(const PlatformConverter& x) = delete;

    PlatformConverter(PlatformConverter&& other) = delete;
    PlatformConverter& operator=(PlatformConverter&& other) = delete;

  public:
    static pal_audio_fmt_t getPalFormatId(
            const ::aidl::android::media::audio::common::AudioFormatDescription&
                    formatDescription) noexcept;
    static pal_device_id_t getPalDeviceId(
            const ::aidl::android::media::audio::common::AudioDeviceDescription&
                    deviceDescription) noexcept;
    static pal_stream_type_t getPalStreamTypeId(int32_t outputFlag) noexcept;

    static uint16_t getBitWidthForAidlPCM(
            const ::aidl::android::media::audio::common::AudioFormatDescription&) noexcept;
#ifdef SEC_AUDIO_SAMSUNGRECORD
    static uint32_t getAudioFormatForAidlPCM(
            const ::aidl::android::media::audio::common::AudioFormatDescription&) noexcept;
#endif
    static std::unique_ptr<pal_channel_info> getPalChannelInfoForChannelCount(int count) noexcept;
    static std::string toString() noexcept;
};
} // namespace qti::audio
