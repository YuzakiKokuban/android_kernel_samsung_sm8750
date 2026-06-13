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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_EffectConfigQti"
#include <android-base/logging.h>
#include <system/audio_effects/audio_effects_conf.h>
#include <system/audio_effects/effect_uuid.h>
#include <optional>
#include <string>

// { SEC_AUDIO_SUPPORT_AIDL_EFFECT
#include <system/audio.h>
// } SEC_AUDIO_SUPPORT_AIDL_EFFECT

#include "effectFactory-impl/EffectConfig.h"

using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::AudioUuid;
using ::aidl::android::hardware::audio::effect::stringToUuid;
#ifdef SEC_AUDIO_SUPPORT_AIDL_EFFECT
using ::aidl::android::hardware::audio::effect::getEffectTypeUuidSoundAlive;
using ::aidl::android::hardware::audio::effect::getEffectTypeUuidMySpaceEffect;
using ::aidl::android::hardware::audio::effect::getEffectTypeUuidDolbyAudioEffect;
using ::aidl::android::hardware::audio::effect::getEffectTypeUuidVolumeMonitor;
#endif

namespace aidl::qti::effects {

EffectConfig::EffectConfig(const std::string& file) {
    tinyxml2::XMLDocument doc;
    doc.LoadFile(file.c_str());
    LOG(VERBOSE) << __func__ << " loading " << file;
    // parse the xml file into maps
    if (doc.Error()) {
        LOG(ERROR) << __func__ << " tinyxml2 failed to load " << file
                   << " error: " << doc.ErrorStr();
        return;
    }

    auto registerFailure = [&](bool result) { mSkippedElements += result ? 0 : 1; };

    for (auto& xmlConfig : getChildren(doc, "audio_effects_conf")) {
        // Parse library
        for (auto& xmlLibraries : getChildren(xmlConfig, "libraries")) {
            for (auto& xmlLibrary : getChildren(xmlLibraries, "library")) {
                registerFailure(parseLibrary(xmlLibrary));
            }
        }

        // Parse effects
        for (auto& xmlEffects : getChildren(xmlConfig, "effects")) {
            for (auto& xmlEffect : getChildren(xmlEffects)) {
                registerFailure(parseEffect(xmlEffect));
            }
        }

        // Parse pre processing chains
        for (auto& xmlPreprocess : getChildren(xmlConfig, "preprocess")) {
            for (auto& xmlStream : getChildren(xmlPreprocess, "stream")) {
                // AudioSource
                registerFailure(parseProcessing(Processing::Type::source, xmlStream));
            }
        }

        // Parse post processing chains
        for (auto& xmlPostprocess : getChildren(xmlConfig, "postprocess")) {
            for (auto& xmlStream : getChildren(xmlPostprocess, "stream")) {
                // AudioStreamType
                registerFailure(parseProcessing(Processing::Type::streamType, xmlStream));
            }
        }
    }
    LOG(DEBUG) << __func__ << " successfully parsed " << file << ", skipping " << mSkippedElements
               << " element(s)";
}

std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> EffectConfig::getChildren(
        const tinyxml2::XMLNode& node, const char* childTag) {
    std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> children;
    for (auto* child = node.FirstChildElement(childTag); child != nullptr;
         child = child->NextSiblingElement(childTag)) {
        children.emplace_back(*child);
    }
    return children;
}

bool EffectConfig::resolveLibrary(const std::string& path, std::string* resolvedPath) {
    for (auto* libraryDirectory : kEffectLibPath) {
        std::string candidatePath = std::string(libraryDirectory) + '/' + path;
        if (access(candidatePath.c_str(), R_OK) == 0) {
            *resolvedPath = std::move(candidatePath);
            return true;
        }
    }
    return false;
}

bool EffectConfig::parseLibrary(const tinyxml2::XMLElement& xml) {
    const char* name = xml.Attribute("name");
    RETURN_VALUE_IF(!name, false, "noNameAttribute");
    const char* path = xml.Attribute("path");
    RETURN_VALUE_IF(!path, false, "noPathAttribute");

    std::string resolvedPath;
    if (!resolveLibrary(path, &resolvedPath)) {
        LOG(ERROR) << __func__ << " can't find " << path;
        return false;
    }
    mLibraryMap[name] = resolvedPath;
    LOG(VERBOSE) << __func__ << " " << name << " : " << resolvedPath;
    return true;
}

bool EffectConfig::parseEffect(const tinyxml2::XMLElement& xml) {
    struct EffectLibraries effectLibraries;
    std::vector<Library> libraries;
    std::string name = xml.Attribute("name");
    RETURN_VALUE_IF(name == "", false, "effectsNoName");

    struct Library library;
    if (std::strcmp(xml.Name(), "effectProxy") == 0) {
        // proxy lib and uuid
        RETURN_VALUE_IF(!parseLibrary(xml, library, true), false, "parseProxyLibFailed");
        effectLibraries.proxyLibrary = library;
        // proxy effect libs and UUID
        auto xmlProxyLib = xml.FirstChildElement();
        RETURN_VALUE_IF(!xmlProxyLib, false, "noLibForProxy");
        while (xmlProxyLib) {
            struct Library tempLibrary;
            RETURN_VALUE_IF(!parseLibrary(*xmlProxyLib, tempLibrary), false,
                            "parseEffectLibFailed");
            libraries.push_back(std::move(tempLibrary));
            xmlProxyLib = xmlProxyLib->NextSiblingElement();
        }
    } else {
        // expect only one library if not proxy
        RETURN_VALUE_IF(!parseLibrary(xml, library), false, "parseEffectLibFailed");
        libraries.push_back(std::move(library));
    }

    effectLibraries.libraries = std::move(libraries);
    mEffectsMap[name] = std::move(effectLibraries);
    return true;
}

bool EffectConfig::parseLibrary(const tinyxml2::XMLElement& xml, struct Library& library,
                                bool isProxy) {
    // Retrieve library name only if not effectProxy element
    if (!isProxy) {
        const char* name = xml.Attribute("library");
        RETURN_VALUE_IF(!name, false, "noLibraryAttribute");
        library.name = name;
    }

    const char* uuidStr = xml.Attribute("uuid");
    RETURN_VALUE_IF(!uuidStr, false, "noUuidAttribute");
    library.uuid = stringToUuid(uuidStr);
    bool typeSameAsUuid = false;
    if (const char* typeSameAsUuidStr = xml.Attribute("typeSameAsUuid")) {
        typeSameAsUuid = (0 == strcmp(typeSameAsUuidStr, "true"));
    }

    if (const char* typeUuidStr = xml.Attribute("type")) {
        library.type = stringToUuid(typeUuidStr);
        LOG(VERBOSE) << " type specified for " << library.name;
    } else if (typeSameAsUuid) {
        // for vendor effects, type and uuid are generally same, so instead of modifying
        // the XML to add "type", add "typeSameAsUuid" tag, so type will be mapped to uuid
        // However, if type and uuid needs to be different then set type explicitly in
        // the XML file.
        library.type = stringToUuid(uuidStr);
    }
    RETURN_VALUE_IF((library.uuid == getEffectUuidZero()), false, "invalidUuidAttribute");

    LOG(VERBOSE) << __func__ << (isProxy ? " proxy " : library.name) << " : uuid "
                 << toString(library.uuid)
                 << " : type: " << (library.type.has_value() ? toString(library.type.value()) : "");
    return true;
}

std::optional<Processing::Type> EffectConfig::stringToProcessingType(Processing::Type::Tag typeTag,
                                                                     const std::string& type) {
    // see list of audio stream types in audio_stream_type_t:
    // system/media/audio/include/system/audio_effects/audio_effects_conf.h
    // AUDIO_STREAM_DEFAULT_TAG is not listed here because according to SYS_RESERVED_DEFAULT in
    // AudioStreamType.aidl: "Value reserved for system use only. HALs must never return this value
    // to the system or accept it from the system".
    static const std::map<const std::string, AudioStreamType> sAudioStreamTypeTable = {
            {AUDIO_STREAM_VOICE_CALL_TAG, AudioStreamType::VOICE_CALL},
            {AUDIO_STREAM_SYSTEM_TAG, AudioStreamType::SYSTEM},
            {AUDIO_STREAM_RING_TAG, AudioStreamType::RING},
            {AUDIO_STREAM_MUSIC_TAG, AudioStreamType::MUSIC},
            {AUDIO_STREAM_ALARM_TAG, AudioStreamType::ALARM},
            {AUDIO_STREAM_NOTIFICATION_TAG, AudioStreamType::NOTIFICATION},
            {AUDIO_STREAM_BLUETOOTH_SCO_TAG, AudioStreamType::BLUETOOTH_SCO},
            {AUDIO_STREAM_ENFORCED_AUDIBLE_TAG, AudioStreamType::ENFORCED_AUDIBLE},
            {AUDIO_STREAM_DTMF_TAG, AudioStreamType::DTMF},
            {AUDIO_STREAM_TTS_TAG, AudioStreamType::TTS},
            {AUDIO_STREAM_ASSISTANT_TAG, AudioStreamType::ASSISTANT}};

    // see list of audio sources in audio_source_t:
    // system/media/audio/include/system/audio_effects/audio_effects_conf.h
    static const std::map<const std::string, AudioSource> sAudioSourceTable = {
            {MIC_SRC_TAG, AudioSource::MIC},
            {VOICE_UL_SRC_TAG, AudioSource::VOICE_UPLINK},
            {VOICE_DL_SRC_TAG, AudioSource::VOICE_DOWNLINK},
            {VOICE_CALL_SRC_TAG, AudioSource::VOICE_CALL},
            {CAMCORDER_SRC_TAG, AudioSource::CAMCORDER},
            {VOICE_REC_SRC_TAG, AudioSource::VOICE_RECOGNITION},
            {VOICE_COMM_SRC_TAG, AudioSource::VOICE_COMMUNICATION},
            {REMOTE_SUBMIX_SRC_TAG, AudioSource::REMOTE_SUBMIX},
            {UNPROCESSED_SRC_TAG, AudioSource::UNPROCESSED},
            {VOICE_PERFORMANCE_SRC_TAG, AudioSource::VOICE_PERFORMANCE}};

    if (typeTag == Processing::Type::streamType) {
        auto typeIter = sAudioStreamTypeTable.find(type);
        if (typeIter != sAudioStreamTypeTable.end()) {
            return typeIter->second;
        }
    } else if (typeTag == Processing::Type::source) {
        auto typeIter = sAudioSourceTable.find(type);
        if (typeIter != sAudioSourceTable.end()) {
            return typeIter->second;
        }
    }

    return std::nullopt;
}

bool EffectConfig::parseProcessing(Processing::Type::Tag typeTag, const tinyxml2::XMLElement& xml) {
    const char* typeStr = xml.Attribute("type");
    auto aidlType = stringToProcessingType(typeTag, typeStr);
    RETURN_VALUE_IF(!aidlType.has_value(), false, "illegalStreamType");
    RETURN_VALUE_IF(0 != mProcessingMap.count(aidlType.value()), false, "duplicateStreamType");

    for (auto& apply : getChildren(xml, "apply")) {
        const char* name = apply.get().Attribute("effect");
        if (mEffectsMap.find(name) == mEffectsMap.end()) {
            LOG(ERROR) << __func__ << " effect " << name << " doesn't exist, skipping";
            continue;
        }
        RETURN_VALUE_IF(!name, false, "noEffectAttribute");
        mProcessingMap[aidlType.value()].emplace_back(mEffectsMap[name]);
        LOG(VERBOSE) << __func__ << " " << typeStr << " : " << name
                     << " aidl: " << aidlType.value().toString();
    }
    return true;
}

const std::map<Processing::Type, std::vector<EffectConfig::EffectLibraries>>&
        EffectConfig::getProcessingMap() const {
    return mProcessingMap;
}

bool EffectConfig::findUuid(const std::pair<std::string, struct EffectLibraries>& effectElem,
                            AudioUuid* uuid) {
// Difference from EFFECT_TYPE_LIST_DEF, there could be multiple name mapping to same Effect Type
#define EFFECT_XML_TYPE_LIST_DEF(V)                        \
    V("acoustic_echo_canceler", AcousticEchoCanceler)      \
    V("automatic_gain_control_v1", AutomaticGainControlV1) \
    V("automatic_gain_control_v2", AutomaticGainControlV2) \
    V("bassboost", BassBoost)                              \
    V("downmix", Downmix)                                  \
    V("dynamics_processing", DynamicsProcessing)           \
    V("equalizer", Equalizer)                              \
    V("haptic_generator", HapticGenerator)                 \
    V("loudness_enhancer", LoudnessEnhancer)               \
    V("env_reverb", EnvReverb)                             \
    V("reverb_env_aux", EnvReverb)                         \
    V("reverb_env_ins", EnvReverb)                         \
    V("preset_reverb", PresetReverb)                       \
    V("reverb_pre_aux", PresetReverb)                      \
    V("reverb_pre_ins", PresetReverb)                      \
    V("noise_suppression", NoiseSuppression)               \
    V("spatializer", Spatializer)                          \
    V("virtualizer", Virtualizer)                          \
    V("visualizer", Visualizer)                            \
    V("volume", Volume)

#ifdef SEC_AUDIO_SUPPORT_AIDL_EFFECT
#define EFFECT_XML_TYPE_SEC_LIST_DEF(V)                    \
    V("soundalive", SoundAlive)                            \
    V("sa3d", MySpaceEffect)                               \
    V("dap", DolbyAudioEffect)                             \
    V("volumemonitor_hw", VolumeMonitor)                   \
    V("sa3d_sw", MySpaceEffect)                            \
    V("sa3d_hw", MySpaceEffect)
#endif

#define GENERATE_MAP_ENTRY_V(s, symbol) {s, &getEffectTypeUuid##symbol},

    const std::string xmlEffectName = effectElem.first;
    typedef const AudioUuid& (*UuidGetter)(void);
    static const std::map<std::string, UuidGetter> uuidMap{
            {EFFECT_XML_TYPE_LIST_DEF(GENERATE_MAP_ENTRY_V)}};
    if (auto it = uuidMap.find(xmlEffectName); it != uuidMap.end()) {
        *uuid = (*it->second)();
        LOG(VERBOSE) << __func__ << " " << xmlEffectName << " found in standard effects";
        return true;
    }

    const auto& libs = effectElem.second.libraries;
    for (const auto& lib : libs) {
        if (lib.type.has_value()) {
            LOG(VERBOSE) << __func__ << " " << xmlEffectName << " found from XML type "
                         << toString(lib.type.value());
            *uuid = lib.type.value();
            return true;
        }
    }

#ifdef SEC_AUDIO_SUPPORT_AIDL_EFFECT
    static const std::map<std::string, UuidGetter> secUuidMap{
            {EFFECT_XML_TYPE_SEC_LIST_DEF(GENERATE_MAP_ENTRY_V)}};

    if (auto it = secUuidMap.find(xmlEffectName); it != secUuidMap.end()) {
        *uuid = (*it->second)();
        LOG(VERBOSE) << __func__ << " " << xmlEffectName << " found in sec effects";
        return true;
    }
#endif

    // find in QTI specific effects
    if (auto it = kUuidNameTypeMap.find(xmlEffectName); it != kUuidNameTypeMap.end()) {
        *uuid = (it->second);
        LOG(VERBOSE) << __func__ << " " << xmlEffectName << " found in QTI effects";
        return true;
    }
    return false;
}

const char* EffectConfig::dump(const tinyxml2::XMLElement& element,
                               tinyxml2::XMLPrinter&& printer) const {
    element.Accept(&printer);
    return printer.CStr();
}

} // namespace aidl::qti::effects
