/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AHAL_MicrophoneInfoParser_QTI"

#include <android-base/logging.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <qti-audio-core/MicrophoneInfoParser.h>
#include <qti-audio-core/MicrophoneInfoTypes.h>
#include <qti_audio_microphone_info.h>
#include <system/audio_config.h>

using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace xsd = ::qti::audio::microphone_info;
using MicrophoneInfoType = xsd::MicrophoneInfoType;
using FrequencyResponseType = xsd::FrequencyResponseType;
using MicrophoneInfoAndDynamicInfo = qti::audio::microphone_info::MicrophoneInfoAndDynamicInfo;

namespace qti::audio::core {
std::vector<::aidl::android::media::audio::common::MicrophoneInfo::FrequencyResponsePoint>
        MicrophoneInfoParser::getFrequencyResponse(const FrequencyResponseType *freqResponse) {
    std::vector<float> freq;
    std::vector<float> response;
    if (freqResponse->hasFrequencyHz()) {
        freq = freqResponse->getFrequencyHz();
    }
    if (freqResponse->hasLeveldB()) {
        response = freqResponse->getLeveldB();
    }

    if (freq.size() != response.size()) {
        LOG(ERROR) << __func__ << " frequency / respone size mismatch " << freq.size() << " "
                   << response.size();
        return {};
    }

    std::vector<::aidl::android::media::audio::common::MicrophoneInfo::FrequencyResponsePoint> frps;
    for (unsigned long i = 0; i < freq.size(); i++) {
        frps.push_back({.frequencyHz = freq[i], .leveldB = response[i]});
    }
    return frps;
}

static std::optional<AudioDevice> populateDevice(const MicrophoneInfoType &xmlInfo) {
    if (!xmlInfo.hasDevice()) {
        LOG(ERROR) << __func__ << " device attribute is missing for microphone";
        return std::nullopt;
    }
    auto xsdDevice = xmlInfo.getFirstDevice();
    if (!xsdDevice->hasName()) {
        LOG(ERROR) << __func__ << " name attribute is missing for device";
        return std::nullopt;
    }

    auto devName = xsdDevice->getName();
    if (XsdToAudioDeviceType.count(devName) == 0) {
        LOG(ERROR) << __func__ << " unknown device " << toString(devName);
        return std::nullopt;
    }

    AudioDeviceType fwkDevice = XsdToAudioDeviceType.at(devName);
    auto address =
            xsdDevice->hasAddress()
                    ? AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(xsdDevice->getAddress())
                    : AudioDeviceAddress{};

    std::string connection = xsdDevice->hasConnection() ? xsdDevice->getConnection() : "";
    AudioDevice device = {.type = {.type = fwkDevice, .connection = connection},
                          .address = address};
    return device;
}

void MicrophoneInfoParser::populateMicrophoneInfo(const MicrophoneInfoAndDynamicInfo &mXsdcConfig) {
    for (const auto &xmlInfo : mXsdcConfig.getFirstMicrophoneInfoList()->getMicrophoneInfo()) {
        ::aidl::android::media::audio::common::MicrophoneInfo info;
        if (!xmlInfo.hasId()) {
            LOG(ERROR) << __func__ << " invalid id attribute";
            return;
        }

        info.id = xmlInfo.getId();

        auto dev = populateDevice(xmlInfo);
        if (!dev.has_value()) {
            LOG(ERROR) << __func__ << " invalid device attribute";
            return;
        }

        info.device = dev.value();

        if (xmlInfo.hasLocation()) info.location = XsdToLocationType.at(xmlInfo.getLocation());

        if (xmlInfo.hasGroup()) info.group = xmlInfo.getGroup();

        if (xmlInfo.hasIndexInTheGroup()) info.indexInTheGroup = xmlInfo.getIndexInTheGroup();

        if (xmlInfo.hasSensitivity()) {
            auto xsdSensitivity = xmlInfo.getFirstSensitivity();
            ::aidl::android::media::audio::common::MicrophoneInfo::Sensitivity ss;
            ss.leveldBFS = xsdSensitivity->hasLeveldBFS() ? xsdSensitivity->getLeveldBFS() : 0.0f;
            ss.maxSpldB = xsdSensitivity->hasMaxSpldB() ? xsdSensitivity->getMaxSpldB() : 0.0f;
            ss.minSpldB = xsdSensitivity->hasMinSpldB() ? xsdSensitivity->getMinSpldB() : 0.0f;
            info.sensitivity = ss;
        }

        if (xmlInfo.hasDirectionality()) {
            info.directionality = XsdToDirectionalityType.at(xmlInfo.getDirectionality());
        }

        if (xmlInfo.hasFrequencyResponse()) {
            info.frequencyResponse = getFrequencyResponse(xmlInfo.getFirstFrequencyResponse());
        } else {
            // should have some frequencies.. may be an error, drop
        }

        auto getCordinates = [](auto &xsdCord) {
            MicrophoneInfo::Coordinate coordinates;
            if (xsdCord->hasX() && xsdCord->hasY() && xsdCord->hasZ()) {
                coordinates.x = xsdCord->getX();
                coordinates.y = xsdCord->getY();
                coordinates.z = xsdCord->getZ();
            }
            return coordinates;
        };

        if (xmlInfo.hasPosition(); auto coords = xmlInfo.getFirstPosition()) {
            info.position = getCordinates(coords);
        }

        if (xmlInfo.hasOrientation(); auto coords = xmlInfo.getFirstOrientation()) {
            info.orientation = getCordinates(coords);
        }

        mInfo.emplace_back(info);
    }
}

void MicrophoneInfoParser::populateMicrophoneDynamicInfo(
        const MicrophoneInfoAndDynamicInfo &mXsdcConfig) {
    auto infoList = mXsdcConfig.getFirstMicrophoneDynamicInfoList()->getMicrophoneDynamicInfo();

    for (const auto &xmlInfo : infoList) {
        if (!xmlInfo.hasDevice() || !xmlInfo.hasMicInfo()) {
            LOG(ERROR) << " invalid device or micInfo attributes";
            return;
        }

        if (XsdToPalDeviceType.count(xmlInfo.getDevice()) == 0) {
            LOG(ERROR) << __func__ << " unknown device " << toString(xmlInfo.getDevice());
            return;
        }

        pal_device_id_t palDevice = XsdToPalDeviceType.at(xmlInfo.getDevice());
        std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo> infos;

        for (const auto &micInfo : xmlInfo.getMicInfo()) {
            ::aidl::android::media::audio::common::MicrophoneDynamicInfo info;
            if (!micInfo.hasId()) {
                LOG(ERROR) << " skip micinfo with invalid id";
                continue;
            }
            info.id = micInfo.getId();
            if (micInfo.hasChannelMapping()) {
                for (const auto channel : micInfo.getChannelMapping()) {
                    info.channelMapping.push_back(XsdToChannelMap.at(channel));
                }
            }
            infos.push_back(info);
        }
        mDynamicInfoMap[palDevice] = infos;
    }
}

MicrophoneInfoParser::MicrophoneInfoParser(const std::string &fileName) {
    auto configFile = android::audio_find_readable_configuration_file(fileName.c_str());
    if (configFile == "") {
        LOG(WARNING) << __func__ << " file " << fileName << " not found";
        return;
    }
    auto xsdConfig = xsd::read(configFile.c_str());
    if (!xsdConfig.has_value()) {
        LOG(WARNING) << __func__ << ": could not read the xml";
        return;
    }

    auto mXsdcConfig = xsdConfig.value();
    if (!mXsdcConfig.hasMicrophoneInfoList() || !mXsdcConfig.hasMicrophoneDynamicInfoList()) {
        LOG(ERROR) << " invalid micInfo or dynamic mic info ";
        return;
    }

    populateMicrophoneInfo(mXsdcConfig);
    populateMicrophoneDynamicInfo(mXsdcConfig);
}

} // namespace qti::audio::core
