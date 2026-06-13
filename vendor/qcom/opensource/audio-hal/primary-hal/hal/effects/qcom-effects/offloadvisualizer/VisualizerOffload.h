/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/effect/BnEffect.h>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

#include "VisualizerOffloadContext.h"

namespace aidl::qti::effects {

class VisualizerOffload final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Capability kCapability;
    static const Descriptor kDescriptor;
    VisualizerOffload();
    ~VisualizerOffload() { LOG(DEBUG) << __func__; }

    ndk::ScopedAStatus commandImpl(CommandId command) override;
    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common,
                                                 bool processData) override;
    RetCode releaseContext() override;

    std::string getEffectName() override { return kEffectName; }

  private:
    static const std::vector<Range::VisualizerRange> kRanges;
    std::shared_ptr<VisualizerOffloadContext> mContext;
    ndk::ScopedAStatus getParameterVisualizer(const Visualizer::Tag& tag,
                                              Parameter::Specific* specific);
};

} // namespace aidl::qti::effects
