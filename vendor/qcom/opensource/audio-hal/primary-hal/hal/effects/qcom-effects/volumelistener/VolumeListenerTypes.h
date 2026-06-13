/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <sstream>
#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Flags;

namespace aidl::qti::effects {

static const std::string kAlarmVolumeListenerEffectName = "Qti-AlarmVolumeListener";
static const std::string kMusicVolumeListenerEffectName = "Qti-MusicVolumeListener";
static const std::string kNotificationVolumeListenerEffectName = "Qti-NotificationVolumeListener";
static const std::string kRingVolumeListenerEffectName = "Qti-RingVolumeListener";
static const std::string kVoiceCallVolumeListenerEffectName = "Qti-VoiceCallVolumeListener";

enum class VolumeListenerType { ALARM, MUSIC, NOTIFICATION, RING, VOICECALL };

static Flags kVolumeFlags = {.type = Flags::Type::INSERT,
                             .volume = Flags::Volume::IND,
                             .offloadIndication = true,
                             .deviceIndication = true,
                             .bypass = true};

static const Descriptor kAlarmVolumeListenerDesc = {
        .common = {.id = {.type = kAlarmVolumeListenerUUID,
                          .uuid = kAlarmVolumeListenerUUID,
                          .proxy = std::nullopt},
                   .flags = kVolumeFlags,
                   .name = kAlarmVolumeListenerEffectName,
                   .implementor = "Qualcomm Technologies Inc"}};

static const Descriptor kMusicVolumeListenerDesc = {
        .common = {.id = {.type = kMusicVolumeListenerUUID,
                          .uuid = kMusicVolumeListenerUUID,
                          .proxy = std::nullopt},
                   .flags = kVolumeFlags,
                   .name = kMusicVolumeListenerEffectName,
                   .implementor = "Qualcomm Technologies Inc"}};

static const Descriptor kNotificationVolumeListenerDesc = {
        .common = {.id = {.type = kNotificationVolumeListenerUUID,
                          .uuid = kNotificationVolumeListenerUUID,
                          .proxy = std::nullopt},
                   .flags = kVolumeFlags,
                   .name = kNotificationVolumeListenerEffectName,
                   .implementor = "Qualcomm Technologies Inc"}};

static const Descriptor kVoiceCallVolumeListenerDesc = {
        .common = {.id = {.type = kVoiceCallVolumeListenerUUID,
                          .uuid = kVoiceCallVolumeListenerUUID,
                          .proxy = std::nullopt},
                   .flags = kVolumeFlags,
                   .name = kVoiceCallVolumeListenerEffectName,
                   .implementor = "Qualcomm Technologies Inc"}};

static const Descriptor kRingVolumeListenerDesc = {
        .common = {.id = {.type = kRingVolumeListenerUUID,
                          .uuid = kRingVolumeListenerUUID,
                          .proxy = std::nullopt},
                   .flags = kVolumeFlags,
                   .name = kRingVolumeListenerEffectName,
                   .implementor = "Qualcomm Technologies Inc"}};

inline std::ostream& operator<<(std::ostream& out, const VolumeListenerType& type) {
    switch (type) {
        case VolumeListenerType::ALARM:
            return out << "ALARM";
        case VolumeListenerType::MUSIC:
            return out << "MUSIC";
        case VolumeListenerType::NOTIFICATION:
            return out << "NOTIFICATION";
        case VolumeListenerType::RING:
            return out << "RING";
        case VolumeListenerType::VOICECALL:
            return out << "VOICECALL";
    }
    return out << "Enum_VolumeListenerError";
}

} // namespace aidl::qti::effects