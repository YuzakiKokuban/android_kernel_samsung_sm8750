/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Flags;
using aidl::android::hardware::audio::effect::Range;
using aidl::android::hardware::audio::effect::AcousticEchoCanceler;
using aidl::android::hardware::audio::effect::NoiseSuppression;

namespace aidl::qti::effects {

enum class VoiceProcessingType {
    AcousticEchoCanceler,
    NoiseSuppression,
};

static const std::string kAcousticEchoCancelerEffectName = "aec";
static const std::string kNoiseSuppressionEffectName = "ns";

static Flags kVoiceProcessingFlags = {
        .type = Flags::Type::PRE_PROC,
        .deviceIndication = true,
        .hwAcceleratorMode = Flags::HardwareAccelerator::TUNNEL,
        .offloadIndication = true,
};

static const std::vector<Range::AcousticEchoCancelerRange> kAcousticEchoCancelerRanges = {
        MAKE_RANGE(AcousticEchoCanceler, echoDelayUs, 0, -1),
        MAKE_RANGE(AcousticEchoCanceler, mobileMode, true, false),
};

static const Capability kAcousticEchoCancelerCap = {.range = kAcousticEchoCancelerRanges};

static const Descriptor kAcousticEchoCancelerDesc = {
        .common = {.id = {.type = kAcousticEchoCancelerTypeUUID,
                          .uuid = kAcousticEchoCancelerQtiUUID,
                          .proxy = std::nullopt},
                   .flags = kVoiceProcessingFlags,
                   .name = kAcousticEchoCancelerEffectName,
                   .implementor = "Qualcomm Technologies Inc"},
        .capability = kAcousticEchoCancelerCap,
};

static const std::vector<Range::NoiseSuppressionRange> kNoiseSuppressionRanges = {
        MAKE_RANGE(NoiseSuppression, level, NoiseSuppression::Level::MEDIUM,
                   NoiseSuppression::Level::LOW),
        MAKE_RANGE(NoiseSuppression, type, NoiseSuppression::Type::MULTI_CHANNEL,
                   NoiseSuppression::Type::SINGLE_CHANNEL),
};

static const Capability kNoiseSuppresionCap = {.range = kNoiseSuppressionRanges};

static const Descriptor kNoiseSuppressionDesc = {
        .common = {.id = {.type = kNoiseSuppressionTypeUUID,
                          .uuid = kNoiseSuppressionQtiUUID,
                          .proxy = std::nullopt},
                   .flags = kVoiceProcessingFlags,
                   .name = kNoiseSuppressionEffectName,
                   .implementor = "Qualcomm Technologies Inc"},
        .capability = kNoiseSuppresionCap,
};

inline std::ostream& operator<<(std::ostream& out, const VoiceProcessingType& type) {
    switch (type) {
        case VoiceProcessingType::AcousticEchoCanceler:
            return out << "AcousticEchoCanceler";
        case VoiceProcessingType::NoiseSuppression:
            return out << "NoiseSuppression";
    }
    return out << "Enum_VoiceProcessingError";
}

} // namespace aidl::qti::effects