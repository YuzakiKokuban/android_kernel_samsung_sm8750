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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <Utils.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioFormatType.h>
#include <aidl/android/media/audio/common/AudioIoFlags.h>
#include <aidl/android/media/audio/common/AudioOutputFlags.h>
#include <aidl/android/media/audio/common/PcmType.h>
#include <android-base/logging.h>
#include <audio_module_config_qti.h>
#include <audio_module_config_qti_enums.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>
#include <media/stagefright/foundation/MediaDefs.h>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include <qti-audio-core/ModuleConfig.h>

// { SEC_AUDIO_COMMON
#include <system/audio.h>
// } SEC_AUDIO_COMMON

using ::aidl::android::hardware::audio::common::makeBitPositionFlagMask;
using ::aidl::android::hardware::audio::core::AudioPatch;
using ::aidl::android::hardware::audio::core::AudioRoute;
using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioEncapsulationType;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioGain;
using ::aidl::android::media::audio::common::AudioGainConfig;
using ::aidl::android::media::audio::common::AudioIoFlags;
using ::aidl::android::media::audio::common::AudioOutputFlags;
using ::aidl::android::media::audio::common::AudioPort;
using ::aidl::android::media::audio::common::AudioPortConfig;
using ::aidl::android::media::audio::common::AudioPortDeviceExt;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioPortMixExt;
using ::aidl::android::media::audio::common::AudioProfile;
using ::aidl::android::media::audio::common::Int;
using ::aidl::android::media::audio::common::MicrophoneInfo;
using ::aidl::android::media::audio::common::PcmType;

namespace xsd = ::audio_module_config_qti;

namespace qti::audio::core {

const static char kRouteDelimiter = ',';
const static std::string kDefaultOutputDevice = "speaker";

const static std::unordered_map<xsd::AudioPcmType, PcmType> XsdToPcmType = {
        {xsd::AudioPcmType::DEFAULT, PcmType::DEFAULT},
        {xsd::AudioPcmType::UINT_8_BIT, PcmType::UINT_8_BIT},
        {xsd::AudioPcmType::INT_16_BIT, PcmType::INT_16_BIT},
        {xsd::AudioPcmType::INT_32_BIT, PcmType::INT_32_BIT},
        {xsd::AudioPcmType::FIXED_Q_8_24, PcmType::FIXED_Q_8_24},
        {xsd::AudioPcmType::FLOAT_32_BIT, PcmType::FLOAT_32_BIT},
        {xsd::AudioPcmType::INT_24_BIT, PcmType::INT_24_BIT},
};

const static std::unordered_map<xsd::AudioDeviceType, AudioDeviceType> XsdToAudioDeviceType = {
        {xsd::AudioDeviceType::NONE, AudioDeviceType::NONE},
        {xsd::AudioDeviceType::IN_DEFAULT, AudioDeviceType::IN_DEFAULT},
        {xsd::AudioDeviceType::IN_ACCESSORY, AudioDeviceType::IN_ACCESSORY},
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
#ifdef SEC_AUDIO_COMMON
        {xsd::AudioDeviceType::IN_MICROPHONE_MULTI, AudioDeviceType::IN_MICROPHONE_MULTI},
#endif
        {xsd::AudioDeviceType::OUT_DEFAULT, AudioDeviceType::OUT_DEFAULT},
        {xsd::AudioDeviceType::OUT_ACCESSORY, AudioDeviceType::OUT_ACCESSORY},
        {xsd::AudioDeviceType::OUT_AFE_PROXY, AudioDeviceType::OUT_AFE_PROXY},
        {xsd::AudioDeviceType::OUT_CARKIT, AudioDeviceType::OUT_CARKIT},
        {xsd::AudioDeviceType::OUT_DEVICE, AudioDeviceType::OUT_DEVICE},
        {xsd::AudioDeviceType::OUT_ECHO_CANCELLER, AudioDeviceType::OUT_ECHO_CANCELLER},
        {xsd::AudioDeviceType::OUT_FM, AudioDeviceType::OUT_FM},
        {xsd::AudioDeviceType::OUT_HEADPHONE, AudioDeviceType::OUT_HEADPHONE},
        {xsd::AudioDeviceType::OUT_HEADSET, AudioDeviceType::OUT_HEADSET},
        {xsd::AudioDeviceType::OUT_HEARING_AID, AudioDeviceType::OUT_HEARING_AID},
        {xsd::AudioDeviceType::OUT_LINE_AUX, AudioDeviceType::OUT_LINE_AUX},
        {xsd::AudioDeviceType::OUT_SPEAKER, AudioDeviceType::OUT_SPEAKER},
        {xsd::AudioDeviceType::OUT_SPEAKER_EARPIECE, AudioDeviceType::OUT_SPEAKER_EARPIECE},
        {xsd::AudioDeviceType::OUT_SPEAKER_SAFE, AudioDeviceType::OUT_SPEAKER_SAFE},
        {xsd::AudioDeviceType::OUT_SUBMIX, AudioDeviceType::OUT_SUBMIX},
        {xsd::AudioDeviceType::OUT_TELEPHONY_TX, AudioDeviceType::OUT_TELEPHONY_TX},
        {xsd::AudioDeviceType::OUT_DOCK, AudioDeviceType::OUT_DOCK},
        {xsd::AudioDeviceType::OUT_BROADCAST, AudioDeviceType::OUT_BROADCAST},
};

const static std::unordered_map<xsd::AudioChannelLayout, int32_t> XsdToAudioChannelLayout = {
        {xsd::AudioChannelLayout::LAYOUT_MONO, AudioChannelLayout::LAYOUT_MONO},
        {xsd::AudioChannelLayout::LAYOUT_STEREO, AudioChannelLayout::LAYOUT_STEREO},
        {xsd::AudioChannelLayout::LAYOUT_2POINT1, AudioChannelLayout::LAYOUT_2POINT1},
        {xsd::AudioChannelLayout::LAYOUT_TRI, AudioChannelLayout::LAYOUT_TRI},
        {xsd::AudioChannelLayout::LAYOUT_TRI_BACK, AudioChannelLayout::LAYOUT_TRI_BACK},
        {xsd::AudioChannelLayout::LAYOUT_3POINT1, AudioChannelLayout::LAYOUT_3POINT1},
        {xsd::AudioChannelLayout::LAYOUT_2POINT0POINT2, AudioChannelLayout::LAYOUT_2POINT0POINT2},
        {xsd::AudioChannelLayout::LAYOUT_2POINT1POINT2, AudioChannelLayout::LAYOUT_2POINT1POINT2},
        {xsd::AudioChannelLayout::LAYOUT_3POINT0POINT2, AudioChannelLayout::LAYOUT_3POINT0POINT2},
        {xsd::AudioChannelLayout::LAYOUT_3POINT1POINT2, AudioChannelLayout::LAYOUT_3POINT1POINT2},
        {xsd::AudioChannelLayout::LAYOUT_QUAD, AudioChannelLayout::LAYOUT_QUAD},
        {xsd::AudioChannelLayout::LAYOUT_QUAD_SIDE, AudioChannelLayout::LAYOUT_QUAD_SIDE},
        {xsd::AudioChannelLayout::LAYOUT_SURROUND, AudioChannelLayout::LAYOUT_SURROUND},
        {xsd::AudioChannelLayout::LAYOUT_PENTA, AudioChannelLayout::LAYOUT_PENTA},
        {xsd::AudioChannelLayout::LAYOUT_5POINT1, AudioChannelLayout::LAYOUT_5POINT1},
        {xsd::AudioChannelLayout::LAYOUT_5POINT1_SIDE, AudioChannelLayout::LAYOUT_5POINT1_SIDE},
        {xsd::AudioChannelLayout::LAYOUT_5POINT1POINT2, AudioChannelLayout::LAYOUT_5POINT1POINT2},
        {xsd::AudioChannelLayout::LAYOUT_5POINT1POINT4, AudioChannelLayout::LAYOUT_5POINT1POINT4},
        {xsd::AudioChannelLayout::LAYOUT_6POINT1, AudioChannelLayout::LAYOUT_6POINT1},
        {xsd::AudioChannelLayout::LAYOUT_7POINT1, AudioChannelLayout::LAYOUT_7POINT1},
        {xsd::AudioChannelLayout::LAYOUT_7POINT1POINT2, AudioChannelLayout::LAYOUT_7POINT1POINT2},
        {xsd::AudioChannelLayout::LAYOUT_7POINT1POINT4, AudioChannelLayout::LAYOUT_7POINT1POINT4},
        {xsd::AudioChannelLayout::LAYOUT_9POINT1POINT4, AudioChannelLayout::LAYOUT_9POINT1POINT4},
        {xsd::AudioChannelLayout::LAYOUT_9POINT1POINT6, AudioChannelLayout::LAYOUT_9POINT1POINT6},
        {xsd::AudioChannelLayout::LAYOUT_13POINT_360RA, AudioChannelLayout::LAYOUT_13POINT_360RA},
        {xsd::AudioChannelLayout::LAYOUT_22POINT2, AudioChannelLayout::LAYOUT_22POINT2},
        {xsd::AudioChannelLayout::LAYOUT_MONO_HAPTIC_A, AudioChannelLayout::LAYOUT_MONO_HAPTIC_A},
        {xsd::AudioChannelLayout::LAYOUT_STEREO_HAPTIC_A,
         AudioChannelLayout::LAYOUT_STEREO_HAPTIC_A},
        {xsd::AudioChannelLayout::LAYOUT_HAPTIC_AB, AudioChannelLayout::LAYOUT_HAPTIC_AB},
        {xsd::AudioChannelLayout::LAYOUT_MONO_HAPTIC_AB, AudioChannelLayout::LAYOUT_MONO_HAPTIC_AB},
        {xsd::AudioChannelLayout::LAYOUT_STEREO_HAPTIC_AB,
         AudioChannelLayout::LAYOUT_STEREO_HAPTIC_AB},
        {xsd::AudioChannelLayout::LAYOUT_FRONT_BACK, AudioChannelLayout::LAYOUT_FRONT_BACK},
};

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

static inline bool maybeVendorExtension(const std::string& s) {
    // Only checks whether the string starts with the "vendor prefix".
    static const std::string vendorPrefix = "VX_";
    return s.size() > vendorPrefix.size() && s.substr(0, vendorPrefix.size()) == vendorPrefix;
}

static auto findPortByTagName(const std::vector<AudioPort>& collection, std::string tagName) {
    return std::find_if(collection.begin(), collection.end(),
                        [&](const auto& e) { return (e.name == tagName); });
}

static int32_t findPortIdByTagName(const std::vector<AudioPort>& ports, std::string tagName) {
    auto portItr = findPortByTagName(ports, tagName);
    if (portItr == ports.end()) {
        return -EINVAL;
    }
    return (*portItr).id;
}

static std::vector<std::string> getAudioHalConfigurationPaths() {
    static const std::vector<std::string> paths = []() {
        return std::vector<std::string>({"/vendor/etc/audio"});
    }();
    return paths;
}

static std::string getReadAbleConfigurationFile(const char* fileName) {
    for (const auto& path : getAudioHalConfigurationPaths()) {
        std::string tryPath = path + "/" + fileName;
        if (::access(tryPath.c_str(), R_OK) == 0) {
            return tryPath;
        }
    }
    return {};
}

static void fillProfile(AudioProfile* profile, const std::string& name,
                        const std::vector<int32_t>& channelLayouts,
                        const std::vector<int64_t>& sampleRates,
                        AudioEncapsulationType encapsulationType) {
    profile->name = name;
    for (auto layout : channelLayouts) {
        profile->channelMasks.push_back(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout));
    }
    profile->sampleRates.insert(profile->sampleRates.end(), sampleRates.begin(), sampleRates.end());
    profile->encapsulationType = encapsulationType;
}

static AudioProfile createProfile(
        const std::string& name, PcmType pcmType, const std::vector<int32_t>& channelLayouts,
        const std::vector<int64_t>& sampleRates,
        AudioEncapsulationType encapsulationType = AudioEncapsulationType::NONE) {
    AudioProfile profile;
    profile.format.type = AudioFormatType::PCM;
    profile.format.pcm = pcmType;
    fillProfile(&profile, name, channelLayouts, sampleRates, encapsulationType);
    return profile;
}

static AudioProfile createProfile(
        const std::string& name, const std::string& encodingType,
        const std::vector<int32_t>& channelLayouts, const std::vector<int64_t>& sampleRates,
        AudioEncapsulationType encapsulationType = AudioEncapsulationType::NONE) {
    AudioProfile profile;
    profile.format.encoding = encodingType;
    profile.format.type = ::aidl::android::media::audio::common::AudioFormatType::NON_PCM;
    fillProfile(&profile, name, channelLayouts, sampleRates, encapsulationType);
    return profile;
}

static AudioGain createGain(int32_t mode, int32_t channelMask,
                            std::pair<int32_t, int32_t> minMaxGain,
                            std::pair<int32_t, int32_t> minMaxRamp,
                            std::pair<int32_t, int32_t> stepAndDefault, bool useForVolume) {
    AudioGain gain;
    gain.mode = mode;
    gain.channelMask = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(channelMask);
    gain.minValue = std::get<0>(minMaxGain);
    gain.maxValue = std::get<1>(minMaxGain);
    gain.minRampMs = std::get<0>(minMaxRamp);
    gain.maxRampMs = std::get<1>(minMaxRamp);
    gain.stepValue = std::get<0>(stepAndDefault);
    gain.defaultValue = std::get<1>(stepAndDefault);
    gain.useForVolume = useForVolume;
    return gain;
}

static AudioPortExt createDeviceExt(AudioDeviceType devType, int32_t flags,
                                    std::vector<AudioFormatDescription> formats,
                                    AudioDeviceAddress address = "", std::string connection = "") {
    AudioPortDeviceExt deviceExt;
    deviceExt.device.type.type = devType;
    deviceExt.device.type.connection = connection;
    deviceExt.flags = flags;
    deviceExt.device.address = address;
    deviceExt.encodedFormats = formats;
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

static AudioPortExt createPortMixExt(int32_t maxOpenStreamCount, int32_t maxActiveStreamCount,
                                     int32_t recommendedMuteDurationMs = 0) {
    AudioPortMixExt mixExt;
    mixExt.maxOpenStreamCount = maxOpenStreamCount;
    mixExt.maxActiveStreamCount = maxActiveStreamCount;
    mixExt.recommendedMuteDurationMs = recommendedMuteDurationMs;
    return AudioPortExt::make<AudioPortExt::Tag::mix>(mixExt);
}

static AudioPort createPort(int32_t id, const std::string& name, int32_t flags, bool isInput,
                            const AudioPortExt& ext) {
    AudioPort port;
    port.id = id;
    port.name = name;
    port.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                         : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    port.ext = ext;
    return port;
}

static AudioPortConfig createPortConfig(int32_t id, int32_t portId, PcmType pcmType, int32_t layout,
                                        int32_t sampleRate, int32_t flags, bool isInput,
                                        const AudioPortExt& ext) {
    AudioPortConfig config;
    config.id = id;
    config.portId = portId;
    config.sampleRate = Int{.value = sampleRate};
    config.channelMask = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout);
    config.format = AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = pcmType};
    config.gain = AudioGainConfig();
    config.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                           : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    config.ext = ext;
    return config;
}

static AudioRoute createRoute(const std::vector<AudioPort>& sources, const AudioPort& sink) {
    AudioRoute route;
    route.sinkPortId = sink.id;
    std::transform(sources.begin(), sources.end(), std::back_inserter(route.sourcePortIds),
                   [](const auto& port) { return port.id; });
    return route;
}

static void sortAudioProfiles(std::vector<AudioProfile>& profiles) {
    std::sort(profiles.begin(), profiles.end());
}

static void dumpProfiles(const AudioProfile& profile, int32_t portId) {
    LOG(INFO) << "  --------- PROFILE for Port ID = " << portId << " ----------";
    LOG(INFO) << "  Name: " << profile.name;
    if (profile.format.type == AudioFormatType::PCM) {
        LOG(INFO) << "  Format: PCM, type: 0x" << std::hex
                  << static_cast<int32_t>(profile.format.pcm);
    } else {
        LOG(INFO) << "  Format: NON_PCM, encoding: " << profile.format.encoding;
    }
    std::string sampleRates;
    std::for_each(profile.sampleRates.begin(), profile.sampleRates.end(),
                  [&](int32_t rate) { sampleRates += std::to_string(rate) + ", "; });
    if (!sampleRates.empty()) {
        sampleRates = sampleRates.substr(0, sampleRates.size() - 2);
    }
    LOG(INFO) << "  Sample rates: " << sampleRates;

    std::ostringstream os;
    std::for_each(profile.channelMasks.begin(), profile.channelMasks.end(),
                  [&](const auto ele) { os << ele.toString(); });
    LOG(INFO) << "  Channel Masks: " << os.str();

    // TODO: Print channel layouts
}

static void dumpRoute(const AudioRoute& route) {
    LOG(DEBUG) << "\n---------ROUTE DUMP----------";
    std::string sourcePorts;
    std::for_each(route.sourcePortIds.begin(), route.sourcePortIds.end(),
                  [&](int32_t rate) { sourcePorts += std::to_string(rate) + ", "; });
    if (!sourcePorts.empty()) {
        sourcePorts = sourcePorts.substr(0, sourcePorts.size() - 2);
    }
    LOG(DEBUG) << "Source Port IDs: " << sourcePorts;
}

static void dumpMixExt(const AudioPortExt& ext) {
    auto& mixExt = ext.get<AudioPortExt::Tag::mix>();
    LOG(DEBUG) << "MixExt: maxOpenStreamCount: " << mixExt.maxOpenStreamCount
               << " maxActiveStreamCount: " << mixExt.maxActiveStreamCount;
    if (mixExt.recommendedMuteDurationMs) {
        LOG(DEBUG) << "MixExt: recommendedMuteDurationMs: " << mixExt.recommendedMuteDurationMs;
    }
}

static void dumpDeviceExt(const AudioPortExt& ext) {
    auto& deviceExt = ext.get<AudioPortExt::Tag::device>();
    LOG(DEBUG) << "DeviceExt: type: 0x" << std::hex
               << static_cast<int32_t>(deviceExt.device.type.type)
               << ", connection: " << deviceExt.device.type.connection;
    LOG(DEBUG) << "DeviceExt: flags: 0x" << std::hex << deviceExt.flags
               << " address: " << deviceExt.device.address.get<AudioDeviceAddress::Tag::id>();
    std::for_each(deviceExt.encodedFormats.begin(), deviceExt.encodedFormats.end(),
                  [&](const auto& format) {
                      LOG(DEBUG) << "DeviceExt: encoding format: " << format.encoding;
                  });
}

static void dumpPort(const AudioPort& port, bool isInput, bool isMix) {
    LOG(DEBUG) << "\n---------PORT DUMP----------";
    LOG(DEBUG) << "Port ID: " << port.id;
    LOG(DEBUG) << "Port Name: " << port.name;
    if (isInput) {
        LOG(DEBUG) << "Input flags: 0x" << std::hex << port.flags.get<AudioIoFlags::Tag::input>();
    } else {
        LOG(DEBUG) << "Output flags: 0x" << std::hex << port.flags.get<AudioIoFlags::Tag::output>();
    }
    auto dumpExtension = isMix ? dumpMixExt : dumpDeviceExt;
    dumpExtension(port.ext);
    std::for_each(port.profiles.begin(), port.profiles.end(),
                  [&](const auto& profile) { dumpProfiles(profile, port.id); });
}

static std::vector<AudioProfile> populateProfiles(
        const std::variant<const xsd::MixPorts::MixPort, const xsd::DevicePorts::DevicePort>&
                audioPort) {
    std::vector<AudioProfile> audioProfiles;

    auto isFormatInvalid = [](const xsd::Profile& profile) {
        if (!profile.hasPcmType() && !profile.hasEncoding()) {
            return true;
        }
        return false;
    };
    auto getName = [](const xsd::Profile& profile) {
        if (!profile.hasName()) {
            return "";
        }
        return profile.getName().c_str();
    };
    auto getFormat = [](const xsd::Profile& profile) -> std::variant<PcmType, const std::string> {
        if (profile.hasEncoding()) {
            return profile.getEncoding();
        }
        if (XsdToPcmType.find(profile.getPcmType()) == XsdToPcmType.end()) {
            return PcmType::DEFAULT;
        }
        return XsdToPcmType.at(profile.getPcmType());
    };
    auto getEncapsulationType = [](const xsd::Profile& profile) {
        if (!profile.hasEncapsulationType()) {
            return AudioEncapsulationType::NONE;
        }
        return static_cast<AudioEncapsulationType>(profile.getEncapsulationType());
    };
    auto getChannels = [](const xsd::Profile& profile) {
        std::vector<int32_t> channels;
        if (!profile.hasChannelLayouts()) {
            return channels;
        }
        std::for_each(profile.getChannelLayouts().begin(), profile.getChannelLayouts().end(),
                      [&](const auto& chLayout) {
                          channels.push_back(XsdToAudioChannelLayout.at(chLayout));
                      });
        return channels;
    };

    std::visit(
            [&](const auto& port) {
                for (const auto& profile : port.getProfile()) {
                    // Todo check if profiles must channel layouts
                    if (isFormatInvalid(profile) || !profile.hasSamplingRates() ||
                        !profile.hasChannelLayouts()) {
                        if (profile.hasName()) {
                            LOG(WARNING) << __func__ << ": Ignore invalid profile "
                                         << profile.getName();
                        }
                        continue;
                    }
                    auto format = getFormat(profile);
                    std::visit(
                            overloaded{[&](const std::string& formatStr) {
                                           audioProfiles.push_back(createProfile(
                                                   getName(profile), formatStr,
                                                   getChannels(profile), profile.getSamplingRates(),
                                                   getEncapsulationType(profile)));
                                       },
                                       [&](const PcmType& formatPcm) {
                                           audioProfiles.push_back(createProfile(
                                                   getName(profile), formatPcm,
                                                   getChannels(profile), profile.getSamplingRates(),
                                                   getEncapsulationType(profile)));
                                       }},
                            format);
                }
            },
            audioPort);

    return audioProfiles;
}

static std::vector<AudioGain> populateGains(
        const std::variant<const xsd::MixPorts::MixPort, const xsd::DevicePorts::DevicePort>&
                audioPort) {
    std::vector<AudioGain> audioGains;
    auto getMode = [](const xsd::Gains::Gain& gain) {
        if (!gain.hasMode()) {
            return 0;
        }
        return static_cast<int32_t>(gain.getMode()[0]);
    };
    auto getChannelLayout = [](const xsd::Gains::Gain& gain) {
        if (!gain.hasChannel_layout()) {
            return xsd::AudioChannelLayout::LAYOUT_MONO;
        }
        return gain.getChannel_layout();
    };
    auto getMinMaxGain = [](const xsd::Gains::Gain& gain) {
        int32_t min = gain.hasMinValueMB() ? gain.getMinValueMB() : 0;
        int32_t max = gain.hasMaxValueMB() ? gain.getMaxValueMB() : 0;
        return std::make_pair(min, max);
    };
    auto getMinMaxRamp = [](const xsd::Gains::Gain& gain) {
        int32_t min = gain.hasMinRampMs() ? gain.getMinRampMs() : 0;
        int32_t max = gain.hasMaxRampMs() ? gain.getMaxRampMs() : 0;
        return std::make_pair(min, max);
    };
    auto getStepAndDefault = [](const xsd::Gains::Gain& gain) {
        int32_t step = gain.hasStepValueMB() ? gain.getStepValueMB() : 0;
        int32_t defaultVal = gain.hasDefaultValueMB() ? gain.getDefaultValueMB() : 0;
        return std::make_pair(step, defaultVal);
    };
    auto getUseForVolume = [](const xsd::Gains::Gain& gain) {
        if (!gain.hasUseForVolume()) {
            return false;
        }
        return gain.getUseForVolume();
    };

    std::visit(
            [&](const auto& port) {
                for (const auto& gain : port.getFirstGains()->getGain()) {
                    auto g = createGain(getMode(gain), static_cast<int32_t>(getChannelLayout(gain)),
                                        getMinMaxGain(gain), getMinMaxRamp(gain),
                                        getStepAndDefault(gain), getUseForVolume(gain));
                    audioGains.push_back(g);
                }
            },
            audioPort);

    return audioGains;
}

static void populateMixPorts(const xsd::Modules::Module& module,
                             std::unique_ptr<ModuleConfig>& moduleConfig) {
    auto getFlags = [](const xsd::MixPorts::MixPort& mixPort, bool isInput) {
        if (!mixPort.hasFlags()) {
            return 0;
        }
        int32_t flags = 0;
        auto flagVec = mixPort.getFlags();
        std::for_each(flagVec.begin(), flagVec.end(), [&](const auto& flagStr) {
            LOG(VERBOSE) << "  flag " << flagStr;
            flags |= 1 << (isInput ? static_cast<int32_t>(xsd::stringToAudioInputFlag(flagStr))
                                   : static_cast<int32_t>(xsd::stringToAudioOutputFlag(flagStr)));
        });
        return flags;
    };

    if (!module.getFirstMixPorts()) {
        LOG(ERROR) << __func__ << " No mix ports";
        return;
    }
    for (const auto& mixPort : module.getFirstMixPorts()->getMixPort()) {
        std::string name = mixPort.hasName() ? mixPort.getName() : "";
        auto role = mixPort.hasRole() ? mixPort.getRole() : xsd::Role::UNKNOWN;
        bool isInput = role == xsd::Role::source ? false : true;

        int32_t maxOpenCount = mixPort.hasMaxOpenCount() ? mixPort.getMaxOpenCount() : 1;
        int32_t maxActiveCount = mixPort.hasMaxActiveCount() ? mixPort.getMaxActiveCount() : 1;
        int32_t recommendedMuteDurationMs =
                mixPort.hasRecommendedMuteDurationMs() ? mixPort.getRecommendedMuteDurationMs() : 0;

        auto port = createPort(
                moduleConfig->nextPortId++, name, getFlags(mixPort, isInput), isInput,
                createPortMixExt(maxOpenCount, maxActiveCount, recommendedMuteDurationMs));
        if (mixPort.hasProfile()) {
            port.profiles = populateProfiles(mixPort);
        }
        if (mixPort.hasGains()) {
            port.gains = populateGains(mixPort);
        }
        moduleConfig->ports.emplace_back(std::move(port));

        // dumpPort(moduleConfig->ports.at(port.id), isInput, true);
    }
}

static void populateDevicePorts(const xsd::Modules::Module& module,
                                std::unique_ptr<ModuleConfig>& moduleConfig) {
    auto getFlags = [](const xsd::DevicePorts::DevicePort& devPort) {
        if (devPort.hasDefaultDevice() && devPort.getDefaultDevice() && !devPort.hasConnection()) {
            return 1 << AudioPortDeviceExt::FLAG_INDEX_DEFAULT_DEVICE;
        }
        return 0;
    };
    auto getFormats = [](const xsd::DevicePorts::DevicePort& devPort) {
        std::vector<AudioFormatDescription> encodings;
        if (!devPort.hasEncodings()) {
            return encodings;
        }
        for (const auto& encodingType : devPort.getEncodings()) {
            auto format = AudioFormatDescription{.type = AudioFormatType::NON_PCM,
                                                 .encoding = encodingType};
            encodings.push_back(format);
        }
        return encodings;
    };

    if (!module.getFirstDevicePorts()) {
        LOG(ERROR) << __func__ << " No device ports";
        return;
    }
    for (const auto& devicePort : module.getFirstDevicePorts()->getDevicePort()) {
        std::string name = devicePort.hasTagName() ? devicePort.getTagName() : "";
        auto role = devicePort.hasRole() ? devicePort.getRole() : xsd::Role::UNKNOWN;
        bool isInput = role == xsd::Role::source ? true : false;
        AudioDeviceType devType = devicePort.hasDeviceType()
                                          ? XsdToAudioDeviceType.at(devicePort.getDeviceType())
                                          : AudioDeviceType::NONE;

        auto address = devicePort.hasAddress()
                               ? AudioDeviceAddress::make<AudioDeviceAddress::Tag::id>(
                                         devicePort.getAddress())
                               : AudioDeviceAddress{};

        std::string connections =
                devicePort.hasConnection() ? toString(devicePort.getConnection()) : "";
        auto deviceExt = createDeviceExt(devType, getFlags(devicePort), getFormats(devicePort),
                                         address, connections);
        auto port = createPort(moduleConfig->nextPortId++, name, 0, isInput, deviceExt);
        if (devicePort.hasProfile()) {
            port.profiles = populateProfiles(devicePort);
        }
        if (devicePort.hasGains()) {
            port.gains = populateGains(devicePort);
        }
        // only external device ports
        if (!devicePort.hasAttached() || (devicePort.hasAttached() && !devicePort.getAttached())) {
            // not attached
            moduleConfig->mExternalDevicePortProfiles[port.id] = port.profiles;
            port.profiles.clear();
        }
        moduleConfig->ports.emplace_back(std::move(port));
        // dumpPort(moduleConfig->ports.at(port.id), isInput, false);
    }
}

static void populateRoutes(const xsd::Modules::Module& module,
                           std::unique_ptr<ModuleConfig>& moduleConfig) {
    if (!module.getFirstRoutes()) {
        LOG(ERROR) << __func__ << " No routes";
        return;
    }
    for (const auto& route : module.getFirstRoutes()->getRoute()) {
        if (!route.hasSources() || !route.hasSink()) {
            if (route.hasSink()) {
                LOG(WARNING) << __func__ << ": Invalid route for sink " << route.getSink();
            }
            continue;
        }

        std::string source = route.getSources();
        std::vector<std::string> srcTags;
        std::string::size_type pos = 0;
        while ((pos = source.find(kRouteDelimiter)) != std::string::npos) {
            std::string tag(source, 0, pos);
            LOG(VERBOSE) << __func__ << ": source tag " << tag;
            srcTags.push_back(tag);
            if (pos > source.size()) {
                break;
            }
            source = source.substr(pos + 1);
        }
        if (!source.empty()) {
            LOG(VERBOSE) << __func__ << ": source tag " << source;
            srcTags.push_back(source);
        }

        std::vector<AudioPort> sources;
        for (const auto& tag : srcTags) {
            auto srcItr = findPortByTagName(moduleConfig->ports, tag);
            if (srcItr != moduleConfig->ports.end()) {
                sources.push_back(*srcItr);
            }
        }

        auto sink = findPortByTagName(moduleConfig->ports, route.getSink());
        LOG(VERBOSE) << __func__ << ": sink tag " << route.getSink();
        if (sink != moduleConfig->ports.end()) {
            moduleConfig->routes.push_back(createRoute(sources, *sink));
            // dumpRoute(moduleConfig->routes.back());
        }
    }
}

static std::unique_ptr<ModuleConfig> getModuleConfig(const xsd::Modules::Module& module) {
    auto moduleConfig = std::make_unique<ModuleConfig>();
    populateMixPorts(module, moduleConfig);
    populateDevicePorts(module, moduleConfig);
    populateRoutes(module, moduleConfig);

    return std::move(moduleConfig);
}

// static
#ifdef SEC_AUDIO_COMMON
std::unique_ptr<ModuleConfig> ModuleConfig::getPrimaryConfiguration(bool secAudioFeatureEnabled) {
    const std::string& filePath = secAudioFeatureEnabled ? 
            kPrimaryModuleConfigFileName : kGsiPrimaryModuleConfigFileName;
    auto xsdConfig =
            xsd::read(getReadAbleConfigurationFile(filePath.c_str()).c_str());
#else // QC
std::unique_ptr<ModuleConfig> ModuleConfig::getPrimaryConfiguration() {
    auto xsdConfig =
            xsd::read(getReadAbleConfigurationFile(kPrimaryModuleConfigFileName.c_str()).c_str());
#endif
    if (!xsdConfig.has_value()) {
        LOG(WARNING) << __func__ << ": primary config retrieval failed, setting defaults";
        return nullptr;
    }
    auto modules = xsdConfig.value();
    if (!modules.has_module()) {
        LOG(WARNING) << __func__ << ": config has no modules at all, setting defaults";
        return nullptr;
    }
    auto module =
            std::find_if(modules.get_module().cbegin(), modules.get_module().cend(), [](auto& ele) {
                if (ele.hasName() && ele.getName() == "default") {
                    return true;
                }
                return false;
            });
    if (module == modules.get_module().cend()) {
        LOG(WARNING) << __func__ << ": config has no default module, setting defaults";
        return nullptr;
    }
    return std::move(getModuleConfig(*module));
}

std::string ModuleConfig::toString() const {
    std::ostringstream os;

    os << std::endl << "--ModuleConfig start--" << std::endl;

    os << std::endl << "port Configs:" << std::endl;
    std::for_each(portConfigs.cbegin(), portConfigs.cend(),
                  [&](const auto& ele) { os << ele.toString() << std::endl; });

    os << std::endl << "initial PortConfigs:" << std::endl;
    std::for_each(initialConfigs.cbegin(), initialConfigs.cend(),
                  [&](const auto& ele) { os << ele.toString() << std::endl; });

    os << std::endl << "ports:" << std::endl;
    std::for_each(ports.cbegin(), ports.cend(),
                  [&](const auto& ele) { os << ele.toString() << std::endl; });

    os << std::endl << "mExternalDevicePortProfiles:" << std::endl;
    for (const auto & [ portId, profilesList ] : mExternalDevicePortProfiles) {
        os << "External device port id:" << portId << std::endl;
        std::for_each(profilesList.cbegin(), profilesList.cend(),
                      [&](auto& ele) { os << ele.toString() << std::endl; });
        os << std::endl;
    }

    os << std::endl << "routes:" << std::endl;
    std::for_each(routes.cbegin(), routes.cend(),
                  [&](const auto& ele) { os << ele.toString() << std::endl; });

    os << std::endl << "patches:" << std::endl;
    std::for_each(patches.cbegin(), patches.cend(),
                  [&](const auto& ele) { os << ele.toString() << std::endl; });
    os << std::endl << "--ModuleConfig end--" << std::endl;
    return os.str();
}

} // namespace qti::audio::core
