/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>

#include "GlobalVoiceProcessingSession.h"
#include "VoiceProcessingContext.h"
#include "VoiceProcessingTypes.h"
#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::qti::effects {

class VoiceProcessing final : public EffectImpl {
  public:
    VoiceProcessingType mType = VoiceProcessingType::AcousticEchoCanceler;

    VoiceProcessing(const AudioUuid& uuid);
    ~VoiceProcessing() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus commandImpl(CommandId command) override;
    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;

    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common,
                                                 bool processData) override;

    RetCode releaseContext() override;

    std::string getEffectName() override { return *mEffectName; };
    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;

    ndk::ScopedAStatus getParameterAcousticEchoCanceler(const AcousticEchoCanceler::Id& id,
                                                        Parameter::Specific* specific);

    ndk::ScopedAStatus getParameterNoiseSuppression(const NoiseSuppression::Id& id,
                                                    Parameter::Specific* specific);

  private:
    std::shared_ptr<VoiceProcessingContext> mContext;
};
} // namespace aidl::qti::effects