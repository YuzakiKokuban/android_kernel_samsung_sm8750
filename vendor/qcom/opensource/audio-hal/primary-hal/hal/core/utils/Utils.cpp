/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Utils_QTI"

#include <aidl/android/media/audio/common/AudioInputFlags.h>
#include <aidl/android/media/audio/common/AudioOutputFlags.h>
#include <android-base/logging.h>
#include <audio_utils/format.h>
#include <qti-audio-core/Utils.h>

using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioPortConfig;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioIoFlags;
using ::aidl::android::media::audio::common::AudioInputFlags;
using ::aidl::android::media::audio::common::AudioOutputFlags;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioSource;
using ::aidl::android::media::audio::common::AudioPortMixExtUseCase;

using ::aidl::android::hardware::audio::core::VendorParameter;

using ::aidl::qti::audio::core::VString;

namespace qti::audio::core {

BufferFormatConverter::BufferFormatConverter(audio_format_t inFormat, audio_format_t outFormat,
                                             size_t bufSize) {
    mInFormat = inFormat;
    mOutFormat = outFormat;
    mAllocSize = bufSize;
    mInBytesPerSample = audio_bytes_per_sample(inFormat);
    mOutBytesPerSample = audio_bytes_per_sample(outFormat);
    int sizeoffloat = sizeof(float);
    mBuffer = std::make_unique<uint8_t[]>(mAllocSize);
    if (!mBuffer) {
        LOG(ERROR) << __func__ << " failed to init convert buffer";
        // alloc size to 0, so convert won't operate
        mAllocSize = 0;
    }
    LOG(VERBOSE) << __func__ << "inFormat " << inFormat << " outFormat " << mOutFormat
                 << " inBytesPerSample " << mInBytesPerSample << " outBytesPerSample "
                 << mOutBytesPerSample << " size " << mAllocSize;
}

std::optional<std::pair<uint8_t*, size_t>> BufferFormatConverter::convert(const void* buffer,
                                                                          size_t bytes) {
    if (bytes > mAllocSize) {
        LOG(ERROR) << " Error writing" << bytes << " to convertBuffer of capacity " << mAllocSize;
        return std::nullopt;
    }
    size_t frames = bytes / mInBytesPerSample;
    memcpy_by_audio_format(mBuffer.get(), mOutFormat, buffer, mInFormat, frames);
    uint8_t* outBuffer = reinterpret_cast<uint8_t*>(mBuffer.get());
    return std::make_pair(outBuffer, frames * mOutBytesPerSample);
}

bool isMixPortConfig(const AudioPortConfig& audioPortConfig) noexcept {
    return audioPortConfig.ext.getTag() == AudioPortExt::Tag::mix;
};

bool isInputMixPortConfig(const AudioPortConfig& audioPortConfig) noexcept {
    return isMixPortConfig(audioPortConfig) && audioPortConfig.flags &&
           audioPortConfig.flags.value().getTag() == AudioIoFlags::Tag::input;
}

bool isDevicePortConfig(const AudioPortConfig& audioPortConfig) noexcept {
    return audioPortConfig.ext.getTag() == AudioPortExt::Tag::device;
};

bool isOutputAudioDevice(const AudioDevice& device) noexcept {
    if (device.type.type >= AudioDeviceType::OUT_DEFAULT) {
        return true;
    }
    return false;
}

bool isTelephonyRXDevice(const AudioDevice& device) noexcept {
    return device.type.type == AudioDeviceType::IN_TELEPHONY_RX;
};

bool isTelephonyTXDevice(const AudioDevice& device) noexcept {
    return device.type.type == AudioDeviceType::OUT_TELEPHONY_TX;
};

bool isBluetoothSCODevice(const AudioDevice& device) noexcept {
    return (device.type.connection == AudioDeviceDescription::CONNECTION_BT_SCO);
}

bool isBluetoothLEDevice(const AudioDevice& device) noexcept {
    return (device.type.connection == AudioDeviceDescription::CONNECTION_BT_LE);
}

bool isBluetoothLETXDevice(const AudioDevice& device) noexcept {
    return (device.type.type == AudioDeviceType::IN_HEADSET &&
            device.type.connection == AudioDeviceDescription::CONNECTION_BT_LE);
}

bool isBluetoothDevice(const AudioDevice& device) noexcept {
    return (device.type.connection == AudioDeviceDescription::CONNECTION_BT_A2DP ||
            device.type.connection == AudioDeviceDescription::CONNECTION_BT_LE);
}

bool hasBluetoothDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isBluetoothDevice);
    return itr != devices.cend();
}

bool hasBluetoothSCODevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isBluetoothSCODevice);
    return itr != devices.cend();
}

bool isBluetoothA2dpDevice(const AudioDevice& device) noexcept {
    return (device.type.connection == AudioDeviceDescription::CONNECTION_BT_A2DP);
}

bool isBluetoothA2dpTXDevice(const AudioDevice& device) noexcept {
    return (device.type.type == AudioDeviceType::IN_DEVICE &&
            device.type.connection == AudioDeviceDescription::CONNECTION_BT_A2DP);
}

bool hasBluetoothLEDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isBluetoothLEDevice);
    return itr != devices.cend();
}

bool hasBluetoothA2dpDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isBluetoothA2dpDevice);
    return itr != devices.cend();
}

bool hasInputMMapFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::input) {
        constexpr auto inputMMapFlag = static_cast<int32_t>(
            1 << static_cast<int32_t>(AudioInputFlags::MMAP_NOIRQ));
        return ((inputMMapFlag & ioFlags.get<AudioIoFlags::Tag::input>()) != 0);
    }
    return false;
}

bool hasOutputMMapFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto outputMMapFlag = static_cast<int32_t>(
            1 << static_cast<int32_t>(AudioOutputFlags::MMAP_NOIRQ));
        return ((outputMMapFlag & ioFlags.get<AudioIoFlags::Tag::output>()) !=
                0);
    }
    return false;
}

bool hasMMapFlagsEnabled(const AudioIoFlags& ioFlags) noexcept {
    return (hasInputMMapFlag(ioFlags) || hasOutputMMapFlag(ioFlags));
}

bool isInputAFEProxyDevice(const AudioDevice& device) noexcept {
    return device.type.type == AudioDeviceType::IN_AFE_PROXY;
}

bool isIPDevice(const AudioDevice& d) noexcept {
    return isIPInDevice(d) || isIPOutDevice(d);
}

bool isIPInDevice(const AudioDevice& d) noexcept {
    if(d.type.type == AudioDeviceType::IN_DEVICE &&
       d.type.connection == AudioDeviceDescription::CONNECTION_IP_V4) {
        return true;
    }
    return false;
}

bool isIPOutDevice(const AudioDevice& d) noexcept {
    if(d.type.type == AudioDeviceType::OUT_DEVICE &&
       d.type.connection == AudioDeviceDescription::CONNECTION_IP_V4) {
        return true;
    }
    return false;
}

bool isOutputSpeakerEarpiece(const AudioDevice& d) noexcept {
    if (d.type.type == AudioDeviceType::OUT_SPEAKER_EARPIECE) {
        return true;
    }
    return false;
}

bool hasOutputSpeakerEarpiece(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isOutputSpeakerEarpiece);
    return itr != devices.cend();
}

bool isHdmiDevice(const AudioDevice& d) noexcept {
    if (d.type.connection == AudioDeviceDescription::CONNECTION_HDMI) {
        return true;
    }
    return false;
}

bool isOutputDevice(const AudioDevice& d) noexcept {
    if (d.type.type >= AudioDeviceType::OUT_DEFAULT) {
        return true;
    }
    return false;
}

bool isInputDevice(const AudioDevice& d) noexcept {
    if (d.type.type < AudioDeviceType::OUT_DEFAULT) {
        return true;
    }
    return false;
}

bool isValidAlsaAddr(const std::vector<int>& alsaAddress) noexcept {
    if (alsaAddress.size() != 2 || alsaAddress[0] < 0 || alsaAddress[1] < 0) {
        LOG(ERROR) << __func__
                   << ": malformed alsa address: "
                   << ::android::internal::ToString(alsaAddress);
        return false;
    }
    return true;
}

bool isUsbDevice(const AudioDevice& d) noexcept {
    if (d.type.connection == AudioDeviceDescription::CONNECTION_USB) {
        return true;
    }
    return false;
}

bool hasOutputDirectFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto directFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::DIRECT));
        return ((directFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
    }
    return false;
}

bool hasInputRawFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::input) {
        constexpr auto rawFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioInputFlags::RAW));
        return ((rawFlag & ioFlags.get<AudioIoFlags::Tag::input>()) != 0);
    }
    return false;
}

bool hasOutputRawFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto rawFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::RAW));
        return ((rawFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
    }
    return false;
}

bool hasOutputVoipRxFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto voipRxFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::VOIP_RX));
        return ((voipRxFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
    }
    return false;
}

bool hasOutputDeepBufferFlag(const AudioIoFlags& ioFlags) noexcept {
     if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
         constexpr auto DeepBufferRxFlag =
                 static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::DEEP_BUFFER));
         return ((DeepBufferRxFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
     }
     return false;
}

bool hasOutputCompressOffloadFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto compressOffloadFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::COMPRESS_OFFLOAD));
        return ((compressOffloadFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
    }
    return false;
}

#ifdef SEC_AUDIO_SAMSUNGRECORD
bool hasInputRemoteMicFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::input) {
        constexpr auto remoteMicFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioInputFlags::REMOTE_MIC));
        return ((remoteMicFlag & ioFlags.get<AudioIoFlags::Tag::input>()) != 0);
    }
    return false;
}

bool hasInputDirectFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::input) {
        constexpr auto directFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioInputFlags::DIRECT));
        return ((directFlag & ioFlags.get<AudioIoFlags::Tag::input>()) != 0);
    }
    return false;
}

bool hasInputFastFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::input) {
        constexpr auto fastFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioInputFlags::FAST));
        return ((fastFlag & ioFlags.get<AudioIoFlags::Tag::input>()) != 0);
    }
    return false;
}
#endif

#ifdef SEC_AUDIO_CALL
bool hasOutputPrimaryFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto primaryFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::PRIMARY));
        return ((primaryFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
    }
    return false;
}
#endif

#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA
bool hasOutputFastMediaFlag(const AudioIoFlags& ioFlags) noexcept {
    if (ioFlags.getTag() == AudioIoFlags::Tag::output) {
        constexpr auto fastmediaFlag =
                static_cast<int32_t>(1 << static_cast<int32_t>(AudioOutputFlags::MEDIA));
        return ((fastmediaFlag & ioFlags.get<AudioIoFlags::Tag::output>()) != 0);
    }
    return false;
}
#endif

#ifdef SEC_AUDIO_COMMON
bool isNoneDevice(const AudioDevice& device) noexcept {
    return device.type.type == AudioDeviceType::NONE;
}

bool hasNoneDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isNoneDevice);
    return itr != devices.cend();
}

bool isSpeakerDevice(const AudioDevice& device) noexcept {
    return !isBluetoothDevice(device) 
        && (device.type.type == AudioDeviceType::OUT_SPEAKER);
}

bool hasSpeakerDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isSpeakerDevice);
    return itr != devices.cend();
}

bool isUsbHeadsetDevice(const AudioDevice& device) noexcept {
    return (device.type.type == AudioDeviceType::OUT_HEADSET &&
            device.type.connection == AudioDeviceDescription::CONNECTION_USB);
}

bool hasUsbHeadsetDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isUsbHeadsetDevice);
    return itr != devices.cend();
}

bool isHdmiOutputDevice(const AudioDevice& device) noexcept {
    return (device.type.connection == AudioDeviceDescription::CONNECTION_HDMI &&
            device.type.type == AudioDeviceType::OUT_DEVICE);
}

bool hasHdmiOutputDevice(const std::vector<AudioDevice>& devices) noexcept {
    auto itr = std::find_if(devices.cbegin(), devices.cend(), isHdmiOutputDevice);
    return itr != devices.cend();
}
#endif

std::optional<AudioSource> getAudioSource(const AudioPortConfig& mixPortconfig) noexcept {
    if (mixPortconfig.ext.getTag() != AudioPortExt::Tag::mix) {
        LOG(ERROR) << __func__ << ": not a mix port, " << mixPortconfig.toString();
        return std::nullopt;
    }
    if (mixPortconfig.ext.get<AudioPortExt::Tag::mix>().usecase.getTag() !=
        AudioPortMixExtUseCase::Tag::source) {
        LOG(ERROR) << __func__ << ": no source provided, " << mixPortconfig.toString();
        return std::nullopt;
    }
    return mixPortconfig.ext.get<AudioPortExt::Tag::mix>()
            .usecase.get<AudioPortMixExtUseCase::Tag::source>();
}

std::optional<int32_t> getSampleRate(const AudioPortConfig& portConfig) noexcept {
    if (portConfig.sampleRate) {
        return portConfig.sampleRate.value().value;
    }
    LOG(ERROR) << __func__ << ": no sample rate in port config " << portConfig.toString();
    return std::nullopt;
}

std::vector<int32_t> getActiveInputMixPortConfigIds(
        const std::vector<AudioPortConfig>& activePortConfigs) {
    std::vector<int32_t> result;
    for (const auto& activePortConfig : activePortConfigs) {
        if (isInputMixPortConfig(activePortConfig)) {
            result.emplace_back(activePortConfig.id);
        }
    }
    return result;
}

int64_t getInt64FromString(const std::string& s) noexcept {
    // Todo handle actual value 0
    return static_cast<int64_t>(strtol(s.c_str(), nullptr, 10));
}

float getFloatFromString(const std::string& s) noexcept {
    // Todo handle actual value 0
    return strtof(s.c_str(), nullptr);
}

bool getBoolFromString(const std::string& s) noexcept {
#ifdef SEC_AUDIO_COMMON
    return (s == "true" || s == "on");
#else
    return (s == "true");
#endif
}

bool setParameter(const VString& parcel, VendorParameter& parameter) noexcept {
    if (parameter.ext.setParcelable(parcel) != android::OK) {
        LOG(ERROR) << __func__ << ": failed to set parcel for " << parameter.id;
        return false;
    }
    return true;
}

VendorParameter makeVendorParameter(const std::string& id, const std::string& value) {
    VString parcel;
    parcel.value = value;
    VendorParameter param;
    param.id = id;
    if (param.ext.setParcelable(parcel) != android::OK) {
        LOG(ERROR) << __func__ << ": failed to set parcel for " << param.id;
    }
    return param;
}

std::string makeParamValue(bool const& isTrue) noexcept {
    return isTrue ? "true" : "false";
}

} // namespace qti::audio::core
