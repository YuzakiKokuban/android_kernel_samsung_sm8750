/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_VoiceProcessingContextQti"

#include <android-base/logging.h>

#include "VoiceProcessingContext.h"
#include "VoiceProcessingTypes.h"

using aidl::android::hardware::audio::effect::IEffect;

namespace aidl::qti::effects {

VoiceProcessingContext::VoiceProcessingContext(const Parameter::Common& common,
                                               const VoiceProcessingType& type, bool processData)
    : EffectContext(common, processData), mType(type) {
    LOG(DEBUG) << __func__;
    mState = UNINITIALIZED;
}

VoiceProcessingContext::~VoiceProcessingContext() {
    LOG(DEBUG) << __func__;
    mState = UNINITIALIZED;
}

RetCode VoiceProcessingContext::enable() {
    LOG(DEBUG) << __func__;
    if (mState != INITIALIZED) {
        return RetCode::ERROR_EFFECT_LIB_ERROR;
    }
    mState = ACTIVE;
    return RetCode::SUCCESS;
}

RetCode VoiceProcessingContext::disable() {
    LOG(DEBUG) << __func__;
    if (mState != ACTIVE) {
        return RetCode::ERROR_EFFECT_LIB_ERROR;
    }
    mState = INITIALIZED;
    return RetCode::SUCCESS;
}

void VoiceProcessingContext::reset() {
    LOG(DEBUG) << __func__;
    disable();
    resetBuffer();
}

} // namespace aidl::qti::effects