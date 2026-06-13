/*
 * Copyright (C) 2022 The Android Open Source Project
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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <cstdlib>
#include <memory>

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>

#include "EffectContext.h"
#include "EffectThread.h"
#include "EffectTypes.h"
#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectThread.h"
#include "effect-impl/EffectTypes.h"

extern "C" binder_exception_t destroyEffect(
        const std::shared_ptr<aidl::android::hardware::audio::effect::IEffect>& instanceSp);

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::BnEffect;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::audio::effect::Flags;

namespace aidl::qti::effects {

class EffectImpl : public BnEffect, public EffectThread {
  public:
    EffectImpl() = default;
    virtual ~EffectImpl() = default;

    virtual ndk::ScopedAStatus open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) override;
    virtual ndk::ScopedAStatus close() override;
    virtual ndk::ScopedAStatus command(CommandId id) override;
    virtual ndk::ScopedAStatus reopen(OpenEffectReturn* ret) override;

    virtual ndk::ScopedAStatus getState(State* state) override;
    virtual ndk::ScopedAStatus setParameter(const Parameter& param) override;
    virtual ndk::ScopedAStatus getParameter(const Parameter::Id& id, Parameter* param) override;

    virtual ndk::ScopedAStatus setParameterCommon(const Parameter& param);
    virtual ndk::ScopedAStatus getParameterCommon(const Parameter::Tag& tag, Parameter* param);

    /* Methods MUST be implemented by each effect instances */
    virtual ndk::ScopedAStatus getDescriptor(Descriptor* desc) = 0;
    virtual ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) = 0;
    virtual ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                                    Parameter::Specific* specific) = 0;

    virtual std::string getEffectName() = 0;
    virtual std::shared_ptr<EffectContext> createContext(const Parameter::Common& common,
                                                         bool processData);

    virtual RetCode releaseContext() = 0;

    /**
     * @brief effectProcessImpl is running in worker thread which created in EffectThread.
     *
     * EffectThread will make sure effectProcessImpl only be called after startThread() successful
     * and before stopThread() successful.
     *
     * effectProcessImpl implementation must not call any EffectThread interface, otherwise it will
     * cause deadlock.
     *
     * @param in address of input float buffer.
     * @param out address of output float buffer.
     * @param samples number of samples to process.
     * @return IEffect::Status
     */
    virtual IEffect::Status effectProcessImpl(float* in, float* out, int samples);

    /**
     * process() get data from data MQs, and call effectProcessImpl() for effect data processing.
     * Its important for the implementation to use mImplMutex for context synchronization.
     */
    void process() override;

  protected:
    // current Hal version
    int mVersion = 0;
    // Use kEventFlagNotEmpty for V1 HAL, kEventFlagDataMqNotEmpty for V2 and above
    int mDataMqNotEmptyEf = aidl::android::hardware::audio::effect::kEventFlagDataMqNotEmpty;
    State mState = State::INIT;
    const Descriptor* mDescriptor;
    const std::string* mEffectName;

    IEffect::Status status(binder_status_t status, size_t consumed, size_t produced);
    void cleanUp();

    std::mutex mImplMutex;
    std::shared_ptr<EffectContext> mImplContext;

    /**
     * Optional CommandId handling methods for effects to override.
     * For CommandId::START, EffectImpl call commandImpl before starting the EffectThread
     * processing.
     * For CommandId::STOP and CommandId::RESET, EffectImpl call commandImpl after stop the
     * EffectThread processing.
     */
    virtual ndk::ScopedAStatus commandImpl(CommandId id);

    RetCode notifyEventFlag(uint32_t flag);

    // used with data processing.
    ::android::hardware::EventFlag* mEventFlag;

  private:
    bool mProcessData = false;
    bool isOffloadOrBypass() {
        return (mDescriptor->common.flags.hwAcceleratorMode == Flags::HardwareAccelerator::TUNNEL ||
                mDescriptor->common.flags.bypass);
    }
};
} // namespace aidl::qti::effects
