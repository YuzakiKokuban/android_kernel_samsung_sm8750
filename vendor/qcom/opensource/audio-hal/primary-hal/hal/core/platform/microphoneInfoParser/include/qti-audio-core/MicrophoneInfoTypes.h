/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <qti_audio_microphone_info.h>

#include <unordered_map>

using ::aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace xsd = ::qti::audio::microphone_info;
using FrequencyResponseType = xsd::FrequencyResponseType;
using MicrophoneInfoAndDynamicInfo = xsd::MicrophoneInfoAndDynamicInfo;

namespace qti::audio::core {

const static std::unordered_map<xsd::AudioDeviceType, AudioDeviceType> XsdToAudioDeviceType = {
        {xsd::AudioDeviceType::IN_AFE_PROXY, AudioDeviceType::IN_AFE_PROXY},
        {xsd::AudioDeviceType::IN_DEVICE, AudioDeviceType::IN_DEVICE},
        {xsd::AudioDeviceType::IN_ECHO_REFERENCE, AudioDeviceType::IN_ECHO_REFERENCE},
        {xsd::AudioDeviceType::IN_FM_TUNER, AudioDeviceType::IN_FM_TUNER},
        {xsd::AudioDeviceType::IN_HEADSET, AudioDeviceType::IN_HEADSET},
        {xsd::AudioDeviceType::IN_LOOPBACK, AudioDeviceType::IN_LOOPBACK},
        {xsd::AudioDeviceType::IN_MICROPHONE, AudioDeviceType::IN_MICROPHONE},
        {xsd::AudioDeviceType::IN_MICROPHONE_BACK, AudioDeviceType::IN_MICROPHONE_BACK},
        {xsd::AudioDeviceType::IN_SUBMIX, AudioDeviceType::IN_SUBMIX},
        {xsd::AudioDeviceType::IN_TELEPHONY_RX, AudioDeviceType::IN_TELEPHONY_RX},
        {xsd::AudioDeviceType::IN_TV_TUNER, AudioDeviceType::IN_TV_TUNER},
        {xsd::AudioDeviceType::IN_DOCK, AudioDeviceType::IN_DOCK},
};

const static std::unordered_map<xsd::DirectionalityType, MicrophoneInfo::Directionality>
        XsdToDirectionalityType = {
                {xsd::DirectionalityType::OMNI, MicrophoneInfo::Directionality::OMNI},
                {xsd::DirectionalityType::BI_DIRECTIONAL,
                 MicrophoneInfo::Directionality::BI_DIRECTIONAL},
                {xsd::DirectionalityType::CARDIOID, MicrophoneInfo::Directionality::CARDIOID},
                {xsd::DirectionalityType::HYPER_CARDIOID,
                 MicrophoneInfo::Directionality::HYPER_CARDIOID},
                {xsd::DirectionalityType::SUPER_CARDIOID,
                 MicrophoneInfo::Directionality::SUPER_CARDIOID},
};

const static std::unordered_map<xsd::LocationType, MicrophoneInfo::Location> XsdToLocationType = {
        {xsd::LocationType::UNKNOWN, MicrophoneInfo::Location::UNKNOWN},
        {xsd::LocationType::MAINBODY, MicrophoneInfo::Location::MAINBODY},
        {xsd::LocationType::MAINBODY_MOVABLE, MicrophoneInfo::Location::MAINBODY_MOVABLE},
        {xsd::LocationType::PERIPHERAL, MicrophoneInfo::Location::PERIPHERAL},
};

const static std::unordered_map<xsd::ChannelMappingType, MicrophoneDynamicInfo::ChannelMapping>
        XsdToChannelMap = {
                {xsd::ChannelMappingType::UNUSED, MicrophoneDynamicInfo::ChannelMapping::UNUSED},
                {xsd::ChannelMappingType::DIRECT, MicrophoneDynamicInfo::ChannelMapping::DIRECT},
                {xsd::ChannelMappingType::PROCESSED,
                 MicrophoneDynamicInfo::ChannelMapping::PROCESSED},
};

const static std::unordered_map<xsd::PalInDevicesType, pal_device_id_t> XsdToPalDeviceType = {
        {xsd::PalInDevicesType::PAL_DEVICE_IN_HANDSET_MIC, PAL_DEVICE_IN_HANDSET_MIC},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_SPEAKER_MIC, PAL_DEVICE_IN_SPEAKER_MIC},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET,
         PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_WIRED_HEADSET, PAL_DEVICE_IN_WIRED_HEADSET},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_AUX_DIGITAL, PAL_DEVICE_IN_AUX_DIGITAL},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_HDMI, PAL_DEVICE_IN_HDMI},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_USB_ACCESSORY, PAL_DEVICE_IN_USB_ACCESSORY},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_USB_DEVICE, PAL_DEVICE_IN_USB_DEVICE},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_USB_HEADSET, PAL_DEVICE_IN_USB_HEADSET},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_FM_TUNER, PAL_DEVICE_IN_FM_TUNER},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_LINE, PAL_DEVICE_IN_LINE},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_SPDIF, PAL_DEVICE_IN_SPDIF},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_PROXY, PAL_DEVICE_IN_PROXY},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_HANDSET_VA_MIC, PAL_DEVICE_IN_HANDSET_VA_MIC},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_BLUETOOTH_A2DP, PAL_DEVICE_IN_BLUETOOTH_A2DP},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_HEADSET_VA_MIC, PAL_DEVICE_IN_HEADSET_VA_MIC},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_VI_FEEDBACK, PAL_DEVICE_IN_VI_FEEDBACK},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_TELEPHONY_RX, PAL_DEVICE_IN_TELEPHONY_RX},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_ULTRASOUND_MIC, PAL_DEVICE_IN_ULTRASOUND_MIC},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_EXT_EC_REF, PAL_DEVICE_IN_EXT_EC_REF},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_ECHO_REF, PAL_DEVICE_IN_ECHO_REF},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK,
         PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_BLUETOOTH_BLE, PAL_DEVICE_IN_BLUETOOTH_BLE},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_CPS_FEEDBACK, PAL_DEVICE_IN_CPS_FEEDBACK},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_DUMMY, PAL_DEVICE_IN_DUMMY},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_CPS2_FEEDBACK, PAL_DEVICE_IN_CPS2_FEEDBACK},
        {xsd::PalInDevicesType::PAL_DEVICE_IN_RECORD_PROXY, PAL_DEVICE_IN_RECORD_PROXY},
};

} // namespace qti::audio::core