/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <array>

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include "effect-impl/EffectTypes.h"
#include "effect-impl/EffectUUID.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Flags;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::BassBoost;
using aidl::android::hardware::audio::effect::Virtualizer;
using aidl::android::hardware::audio::effect::PresetReverb;
using aidl::android::hardware::audio::effect::EnvironmentalReverb;
using aidl::android::hardware::audio::effect::Range;

namespace aidl::qti::effects {

static const std::string kEqualizerEffectName = "Qti-Offload-Equalizer";

constexpr inline size_t MAX_NUM_PRESETS = 10;
constexpr inline size_t MAX_NUM_BANDS = 5;
constexpr inline int PRESET_CUSTOM = -1;
constexpr inline int PRESET_INVALID = -2;

static const std::vector<Equalizer::BandFrequency> kBandFrequencies = {{0, 30000, 120000},
                                                                       {1, 120001, 460000},
                                                                       {2, 460001, 1800000},
                                                                       {3, 1800001, 7000000},
                                                                       {4, 7000001, 20000000}};

constexpr inline std::array<uint16_t, MAX_NUM_BANDS> kPresetsFrequencies = {60, 230, 910, 3600,
                                                                            14000};

constexpr inline std::array<std::array<int16_t, MAX_NUM_BANDS>, MAX_NUM_PRESETS> kBandPresetLevels =
        {{{3, 0, 0, 0, 3},    /* Normal Preset */
          {5, 3, -2, 4, 4},   /* Classical Preset */
          {6, 0, 2, 4, 1},    /* Dance Preset */
          {0, 0, 0, 0, 0},    /* Flat Preset */
          {3, 0, 0, 2, -1},   /* Folk Preset */
          {4, 1, 9, 3, 0},    /* Heavy Metal Preset */
          {5, 3, 0, 1, 3},    /* Hip Hop Preset */
          {4, 2, -2, 2, 5},   /* Jazz Preset */
          {-1, 2, 5, 1, -2},  /* Pop Preset */
          {5, 3, -1, 3, 5}}}; /* Rock Preset */

static const std::vector<Equalizer::Preset> kPresets = {
        {0, "Normal"},      {1, "Classical"}, {2, "Dance"}, {3, "Flat"}, {4, "Folk"},
        {5, "Heavy Metal"}, {6, "Hip Hop"},   {7, "Jazz"},  {8, "Pop"},  {9, "Rock"}};

static Range::EqualizerRange presetRange = MAKE_RANGE(Equalizer, preset, 0, MAX_NUM_PRESETS - 1);
static Range::EqualizerRange bandLevelRange = MAKE_RANGE(
        Equalizer, bandLevels,
        std::vector<Equalizer::BandLevel>{Equalizer::BandLevel({.index = 0, .levelMb = -1500})},
        std::vector<Equalizer::BandLevel>{
                Equalizer::BandLevel({.index = MAX_NUM_BANDS - 1, .levelMb = 1500})});
static Range::EqualizerRange bandFrequencyRange =
        MAKE_RANGE(Equalizer, bandFrequencies, kBandFrequencies, kBandFrequencies);
static Range::EqualizerRange presetsRange = MAKE_RANGE(Equalizer, presets, kPresets, kPresets);
static Range::EqualizerRange centerFreqRange =
        MAKE_RANGE(Equalizer, centerFreqMh, std::vector<int>({1}), std::vector<int>({}));

const std::vector<Range::EqualizerRange> kEqRanges = {
        presetRange, bandLevelRange, bandFrequencyRange, presetsRange, centerFreqRange};

static const Capability kEqualizerCapabilites = {.range = kEqRanges};

static const Descriptor kEqualizerDesc = {
        .common = {.id = {.type = kEqualizerTypeUUID,
                          .uuid = kEqualizerOffloadQtiUUID,
                          .proxy = kEqualizerProxyUUID},
                   .flags = {.type = Flags::Type::INSERT,
                             .volume = Flags::Volume::CTRL,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                             .deviceIndication = true,
                             .offloadIndication = true},
                   .name = kEqualizerEffectName,
                   .implementor = "Qualcomm Technologies Inc."},
        .capability = kEqualizerCapabilites};

static const bool mStrengthSupported = true;
static const int mMaxBBStrengthSupported = 1000;

const std::vector<Range::BassBoostRange> kBassBoostRanges = {
        MAKE_RANGE(BassBoost, strengthPm, 0, mMaxBBStrengthSupported)};

static const Capability kBassBoostCap = {.range = kBassBoostRanges};

static const std::string kBassBoostEffectName = "Qti-Offload-BassBoost";

static const Descriptor kBassBoostDesc = {
        .common = {.id = {.type = kBassBoostTypeUUID,
                          .uuid = kBassBoostOffloadQtiUUID,
                          .proxy = kBassBoostProxyUUID},
                   .flags = {.type = Flags::Type::INSERT,
                             .volume = Flags::Volume::CTRL,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                             .deviceIndication = true,
                             .offloadIndication = true},
                   .name = kBassBoostEffectName,
                   .implementor = "Qualcomm Technologies Inc."},
        .capability = kBassBoostCap};

static const int mMaxVirtualizerStrengthSupported = 1000;
static const std::vector<Range::VirtualizerRange> kVirtualizerRanges = {
        MAKE_RANGE(Virtualizer, strengthPm, 0, mMaxVirtualizerStrengthSupported)};

static const Capability kVirtualizerCap = {.range = kVirtualizerRanges};

static const std::string kVirtualizerEffectName = "Qti-Offload-Virtualizer";

static const Descriptor kVirtualizerDesc = {
        .common = {.id = {.type = kVirtualizerTypeUUID,
                          .uuid = kVirtualizerOffloadQtiUUID,
                          .proxy = kVirtualizerProxyUUID},
                   .flags = {.type = Flags::Type::INSERT,
                             .volume = Flags::Volume::CTRL,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                             .deviceIndication = true,
                             .offloadIndication = true},
                   .name = kVirtualizerEffectName,
                   .implementor = "Qualcomm Technologies Inc."},
        .capability = kVirtualizerCap};

constexpr inline int kMinLevel = -6000;
constexpr inline int kMaxDecayTime = 7000;

static const std::vector<Range::EnvironmentalReverbRange> kEnvReverbRanges = {
        MAKE_RANGE(EnvironmentalReverb, roomLevelMb, kMinLevel, 0),
        MAKE_RANGE(EnvironmentalReverb, roomHfLevelMb, -4000, 0),
        MAKE_RANGE(EnvironmentalReverb, decayTimeMs, 0, kMaxDecayTime),
        MAKE_RANGE(EnvironmentalReverb, decayHfRatioPm, 100, 2000),
        MAKE_RANGE(EnvironmentalReverb, levelMb, kMinLevel, 0),
        MAKE_RANGE(EnvironmentalReverb, delayMs, 0, 65),
        MAKE_RANGE(EnvironmentalReverb, diffusionPm, 0, 1000),
        MAKE_RANGE(EnvironmentalReverb, densityPm, 0, 1000)};

static const Capability kEnvReverbCap = {
        .range = Range::make<Range::environmentalReverb>(kEnvReverbRanges)};

static const std::string kAuxEnvReverbEffectName = "Qti-Auxiliary Environmental Reverb";
static const Descriptor kAuxEnvReverbDesc = {
        .common = {.id = {.type = kEnvReverbTypeUUID,
                          .uuid = kAuxEnvReverbOffloadQtiUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::AUXILIARY,
                             .volume = Flags::Volume::CTRL,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                             .offloadIndication = true},
                   .name = kAuxEnvReverbEffectName,
                   .implementor = "Qualcomm Technologies Inc."},
        .capability = kEnvReverbCap};

static const std::string kInsertEnvReverbEffectName = "Qti-Insert Environmental Reverb";
static const Descriptor kInsertEnvReverbDesc = {
        .common = {.id = {.type = kEnvReverbTypeUUID,
                          .uuid = kInsertEnvReverbOffloadQtiUUID,
                          .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .insert = Flags::Insert::FIRST,
                             .volume = Flags::Volume::CTRL,
                             .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                             .offloadIndication = true},
                   .name = kInsertEnvReverbEffectName,
                   .implementor = "Qualcomm Technologies Inc."},
        .capability = kEnvReverbCap};

static const std::vector<PresetReverb::Presets> kSupportedPresets{
        ndk::enum_range<PresetReverb::Presets>().begin(),
        ndk::enum_range<PresetReverb::Presets>().end()};

static const std::vector<Range::PresetReverbRange> kPresetReverbRanges = {
        MAKE_RANGE(PresetReverb, supportedPresets, kSupportedPresets, kSupportedPresets)};

static const Capability kPresetReverbCap = {
        .range = Range::make<Range::presetReverb>(kPresetReverbRanges)};

static const std::string kAuxPresetReverbEffectName = "Qti-Auxiliary Preset Reverb";
static const Descriptor kAuxPresetReverbDesc = {
        .common =
                {
                        .id = {.type = kPresetReverbTypeUUID,
                               .uuid = kAuxPresetReverbOffloadQtiUUID,
                               .proxy = std::nullopt},
                        .flags = {.type = Flags::Type::AUXILIARY,
                                  .volume = Flags::Volume::CTRL,
                                  .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                                  .offloadIndication = true},
                        .name = kAuxPresetReverbEffectName,
                        .implementor = "Qualcomm Technologies Inc.",
                },
        .capability = kPresetReverbCap};

static const std::string kInsertPresetReverbEffectName = "Qti-Insert Preset Reverb";
static const Descriptor kInsertPresetReverbDesc = {
        .common =
                {
                        .id = {.type = kPresetReverbTypeUUID,
                               .uuid = kInsertPresetReverbOffloadQtiUUID,
                               .proxy = std::nullopt},
                        .flags = {.type = Flags::Type::INSERT,
                                  .insert = Flags::Insert::FIRST,
                                  .volume = Flags::Volume::CTRL,
                                  .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
                                  .offloadIndication = true},
                        .name = kInsertPresetReverbEffectName,
                        .implementor = "Qualcomm Technologies Inc.",
                },
        .capability = kPresetReverbCap};

enum class OffloadBundleEffectType {
    BASS_BOOST,
    VIRTUALIZER,
    EQUALIZER,
    AUX_ENV_REVERB,
    INSERT_ENV_REVERB,
    AUX_PRESET_REVERB,
    INSERT_PRESET_REVERB,
};

inline std::ostream& operator<<(std::ostream& out, const OffloadBundleEffectType& type) {
    out << " Type ";
    switch (type) {
        case OffloadBundleEffectType::BASS_BOOST:
            return out << "BASS_BOOST";
        case OffloadBundleEffectType::VIRTUALIZER:
            return out << "VIRTUALIZER";
        case OffloadBundleEffectType::EQUALIZER:
            return out << "EQUALIZER";
        case OffloadBundleEffectType::AUX_ENV_REVERB:
            return out << "AUX_ENV_REVERB";
        case OffloadBundleEffectType::INSERT_ENV_REVERB:
            return out << "INSERT_ENV_REVERB";
        case OffloadBundleEffectType::AUX_PRESET_REVERB:
            return out << "AUX_PRESET_REVERB";
        case OffloadBundleEffectType::INSERT_PRESET_REVERB:
            return out << "INSERT_PRESET_REVERB";
    }
    return out << "EnumOffloadBundleEffectTypeError";
}

} // namespace aidl::qti::effects
