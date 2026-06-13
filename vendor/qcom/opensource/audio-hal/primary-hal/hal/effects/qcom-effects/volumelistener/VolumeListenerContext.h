/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "PalApi.h"
#include "PalDefs.h"
#include "VolumeListenerTypes.h"
#include "effect-impl/EffectContext.h"

#define LIN_VOLUME_QFACTOR_28 28
#define MAX_VOLUME_CAL_STEPS 15
#define MAX_GAIN_LEVELS 5
#define DEFAULT_CAL_STEP 0

namespace aidl::qti::effects {

enum class VolumeListenerState {
    UNINITIALIZED,
    INITIALIZED,
    ACTIVE,
};

using AudioDeviceDescriptionVector =
        std::vector<aidl::android::media::audio::common::AudioDeviceDescription>;

class VolumeListenerContext final : public EffectContext {
  public:
    VolumeListenerContext(const Parameter::Common& common, VolumeListenerType type,
                          bool processData);
    ~VolumeListenerContext();
    virtual RetCode setOutputDevice(const AudioDeviceDescriptionVector& devices) override;
    virtual RetCode setVolumeStereo(const Parameter::VolumeStereo& volumeStereo) override;
    RetCode enable();
    RetCode disable();
    void reset();

    bool isActive() { return mState == VolumeListenerState::ACTIVE; }
    bool isEffectActiveAndApplicable() { return isActive() && isValidContext(); }
    float getMaxOfLeftRightChannels() { return fmax(mVolumeStereo.left, mVolumeStereo.right); }

  private:
    bool isValidVoiceCallContext();
    bool isValidContext();
    bool isSpeaker(AudioDeviceDescriptionVector& devices);
    bool isSpeaker(const AudioDeviceDescriptionVector& devices);
    bool isWiredHeadset(AudioDeviceDescriptionVector& devices);
    bool isEarpiece(AudioDeviceDescriptionVector& devices);

    static bool sHeadsetCalEnabled;

    VolumeListenerState mState;
    VolumeListenerType mType;

    std::string details() {
        std::ostringstream os;
        os << " ";
        os << mType;
        os << " session ";
        os << mCommon.session;
        return os.str();
    }
};

} // namespace aidl::qti::effects
