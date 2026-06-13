/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "VoiceProcessingTypes.h"
#include "effect-impl/EffectContext.h"

namespace aidl::qti::effects {

enum VoiceProcessingState {
    UNINITIALIZED,
    INITIALIZED,
    ACTIVE,
};

class VoiceProcessingContext final : public EffectContext {
  public:
    VoiceProcessingContext(const Parameter::Common& common, const VoiceProcessingType& type,
                           bool processData);
    ~VoiceProcessingContext();
    VoiceProcessingType getVoiceProcessingType() const { return mType; }
    RetCode enable();
    RetCode disable();
    void reset();
    int getAcousticEchoCancelerEchoDelay() const { return 0; }
    bool getAcousticEchoCancelerMobileMode() const { return false; }
    NoiseSuppression::Level getNoiseSuppressionLevel() const {
        return NoiseSuppression::Level::MEDIUM;
    }

  private:
    VoiceProcessingState mState = UNINITIALIZED;
    VoiceProcessingType mType;
};

} // namespace aidl::qti::effects