/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>
#include <cstdlib>
#include <memory>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

#include "GlobalVolumeListenerSession.h"
#include "VolumeListenerContext.h"
#include "VolumeListenerTypes.h"
namespace aidl::qti::effects {

class VolumeListener final : public EffectImpl {
  public:
    VolumeListenerType mType = VolumeListenerType::ALARM;

    VolumeListener(const AudioUuid& uuid);
    ~VolumeListener() {
        cleanUp();
    }

    ndk::ScopedAStatus commandImpl(CommandId command) override;
    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;

    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common,
                                                 bool processData) override;

    RetCode releaseContext() override;

    ndk::ScopedAStatus setParameterCommon(const Parameter& param) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;

    std::string getEffectName() override { return *mEffectName; };

  private:
    std::shared_ptr<VolumeListenerContext> mContext;
};
} // namespace aidl::qti::effects
