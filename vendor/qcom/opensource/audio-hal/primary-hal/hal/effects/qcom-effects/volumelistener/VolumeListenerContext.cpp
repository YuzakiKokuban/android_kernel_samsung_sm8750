/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <cstddef>
#include <memory>
#define LOG_TAG "AHAL_Effect_VolumeListenerQti"
#include <unordered_set>

#include <android-base/logging.h>
#include "GlobalVolumeListenerSession.h"
#include "VolumeListenerContext.h"

using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;

namespace aidl::qti::effects {

bool VolumeListenerContext::sHeadsetCalEnabled =
        GlobalVolumeListenerSession::getSession().getConfig().isHeadsetCalEnabled();

bool VolumeListenerContext::isEarpiece(AudioDeviceDescriptionVector& devices) {
    for (const auto& device : devices) {
        if (device != AudioDeviceDescription{AudioDeviceType::OUT_SPEAKER_EARPIECE, ""}) {
            return false;
        }
    }
    return true;
}

bool VolumeListenerContext::isWiredHeadset(AudioDeviceDescriptionVector& devices) {
    for (const auto& device : devices) {
        if (device != AudioDeviceDescription{AudioDeviceType::OUT_HEADSET,
                                             AudioDeviceDescription::CONNECTION_ANALOG} &&
            device != AudioDeviceDescription{AudioDeviceType::OUT_HEADPHONE,
                                             AudioDeviceDescription::CONNECTION_ANALOG}) {
            return false;
        }
    }
    return true;
}

// Adapted from dev & OUT_SPK, any of the device is a speaker device.
bool VolumeListenerContext::isSpeaker(AudioDeviceDescriptionVector& devices) {
    const AudioDeviceDescriptionVector& dev = devices;
    return isSpeaker(dev);
}

bool VolumeListenerContext::isSpeaker(const AudioDeviceDescriptionVector& devices) {
    for (const auto& device : devices) {
        if (device == AudioDeviceDescription{AudioDeviceType::OUT_SPEAKER, ""} ||
            device == AudioDeviceDescription{AudioDeviceType::OUT_SPEAKER_SAFE, ""}) {
            return true;
        }
    }
    return false;
}

VolumeListenerContext::VolumeListenerContext(const Parameter::Common& common,
                                             VolumeListenerType type, bool processData)
    : EffectContext(common, processData) {
    mType = type;
    LOG(VERBOSE) << __func__ << details();
    mState = VolumeListenerState::INITIALIZED;
}

VolumeListenerContext::~VolumeListenerContext() {
    LOG(VERBOSE) << __func__ << details();
    mState = VolumeListenerState::UNINITIALIZED;
}

bool VolumeListenerContext::isValidVoiceCallContext() {
    return mType == VolumeListenerType::VOICECALL && sHeadsetCalEnabled &&
           (isEarpiece(mOutputDevice) || isWiredHeadset(mOutputDevice));
}

bool VolumeListenerContext::isValidContext() {
    return isValidVoiceCallContext() || isSpeaker(mOutputDevice);
}

RetCode VolumeListenerContext::enable() {
    LOG(VERBOSE) << __func__ << details();
    if (mState != VolumeListenerState::INITIALIZED) {
        LOG(ERROR) << __func__ << "state not initialized";
        return RetCode::ERROR_EFFECT_LIB_ERROR;
    }
    mState = VolumeListenerState::ACTIVE;
    return RetCode::SUCCESS;
}

RetCode VolumeListenerContext::disable() {
    LOG(VERBOSE) << __func__ << details();
    if (mState != VolumeListenerState::ACTIVE) {
        LOG(ERROR) << __func__ << "state not active";
        return RetCode::ERROR_EFFECT_LIB_ERROR;
    }
    mState = VolumeListenerState::INITIALIZED;
    return RetCode::SUCCESS;
}

void VolumeListenerContext::reset() {
    LOG(VERBOSE) << __func__ << details();
    disable();
    resetBuffer();
}

RetCode VolumeListenerContext::setOutputDevice(
        const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& devices) {
    mOutputDevice = devices;
    return RetCode::SUCCESS;
}

RetCode VolumeListenerContext::setVolumeStereo(const Parameter::VolumeStereo& volumeStereo) {
    LOG(VERBOSE) << __func__ << details() << " " << volumeStereo.toString();
    mVolumeStereo = volumeStereo;
    return RetCode::SUCCESS;
}

} // namespace aidl::qti::effects
