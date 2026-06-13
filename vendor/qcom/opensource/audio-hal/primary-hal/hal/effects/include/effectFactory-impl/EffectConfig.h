/*
 * Copyright (C) 2017 The Android Open Source Project
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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <cutils/properties.h>
#include <system/audio_effects/effect_uuid.h>
#include <tinyxml2.h>

#include <aidl/android/hardware/audio/effect/Processing.h>
#include "effect-impl/EffectTypes.h"
#include "effect-impl/EffectUUID.h"

using aidl::android::hardware::audio::effect::getEffectTypeUuidAcousticEchoCanceler;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAutomaticGainControlV1;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAutomaticGainControlV2;
using aidl::android::hardware::audio::effect::getEffectTypeUuidBassBoost;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDownmix;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDynamicsProcessing;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEqualizer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidHapticGenerator;
using aidl::android::hardware::audio::effect::getEffectTypeUuidLoudnessEnhancer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEnvReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEnvReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEnvReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidPresetReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidPresetReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidPresetReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidNoiseSuppression;
using aidl::android::hardware::audio::effect::getEffectTypeUuidSpatializer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVirtualizer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVisualizer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVolume;
using aidl::android::hardware::audio::effect::getEffectUuidZero;
using aidl::android::hardware::audio::effect::Processing;

namespace aidl::qti::effects {

/**
 *  Library contains a mapping from library name to path.
 *  Effect contains a mapping from effect name to Libraries and implementation UUID.
 *  Pre/post processor contains a mapping from processing name to effect names.
 */
class EffectConfig {
  public:
    explicit EffectConfig(const std::string& file);

    struct Library {
        std::string name;                                                     // library name
        ::aidl::android::media::audio::common::AudioUuid uuid;                // implementation UUID
        std::optional<::aidl::android::media::audio::common::AudioUuid> type; // optional type UUID
    };
    // <effects>
    struct EffectLibraries {
        std::optional<struct Library> proxyLibrary;
        std::vector<struct Library> libraries;
    };

    int getSkippedElements() const { return mSkippedElements; }
    const std::unordered_map<std::string, std::string> getLibraryMap() const { return mLibraryMap; }
    const std::unordered_map<std::string, struct EffectLibraries> getEffectsMap() const {
        return mEffectsMap;
    }

    static bool findUuid(const std::pair<std::string, struct EffectLibraries>& effectElem,
                         ::aidl::android::media::audio::common::AudioUuid* uuid);

    using ProcessingLibrariesMap = std::map<Processing::Type, std::vector<struct EffectLibraries>>;
    const ProcessingLibrariesMap& getProcessingMap() const;

  private:
    static constexpr const char* kEffectLibPath[] =
#ifdef __LP64__
            {"/odm/lib64/soundfx", "/vendor/lib64/soundfx", "/system/lib64/soundfx"};
#else
            {"/odm/lib/soundfx", "/vendor/lib/soundfx", "/system/lib/soundfx"};
#endif

    int mSkippedElements;
    /* Parsed Libraries result */
    std::unordered_map<std::string, std::string> mLibraryMap;
    /* Parsed Effects result */
    std::unordered_map<std::string, struct EffectLibraries> mEffectsMap;
    /**
     * For parsed pre/post processing result: {key: AudioStreamType/AudioSource, value:
     * EffectLibraries}
     */
    ProcessingLibrariesMap mProcessingMap;

    /** @return all `node`s children that are elements and match the tag if provided. */
    std::vector<std::reference_wrapper<const tinyxml2::XMLElement>> getChildren(
            const tinyxml2::XMLNode& node, const char* childTag = nullptr);

    /** Parse a library xml note and push the result in mLibraryMap or return false on failure. */
    bool parseLibrary(const tinyxml2::XMLElement& xml);

    /** Parse an effect from an xml element describing it.
     * @return true and pushes the effect in mEffectsMap on success, false on failure.
     */
    bool parseEffect(const tinyxml2::XMLElement& xml);

    bool parseProcessing(Processing::Type::Tag typeTag, const tinyxml2::XMLElement& xml);

    // Function to parse effect.library name and effect.uuid from xml
    bool parseLibrary(const tinyxml2::XMLElement& xml, struct Library& library,
                      bool isProxy = false);

    const char* dump(const tinyxml2::XMLElement& element,
                     tinyxml2::XMLPrinter&& printer = {}) const;

    bool resolveLibrary(const std::string& path, std::string* resolvedPath);

    std::optional<Processing::Type> stringToProcessingType(Processing::Type::Tag typeTag,
                                                           const std::string& type);
};

} // namespace aidl::qti::effects
