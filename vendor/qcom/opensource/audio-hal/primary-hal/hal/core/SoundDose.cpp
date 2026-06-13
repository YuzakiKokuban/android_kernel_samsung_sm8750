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
 * ​​​​​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_SoundDose_QTI"

#include <android-base/logging.h>
#include <qti-audio-core/SoundDose.h>

namespace qti::audio::core {

ndk::ScopedAStatus SoundDose::setOutputRs2UpperBound(float in_rs2ValueDbA) {
    if (in_rs2ValueDbA < static_cast<float>(MIN_RS2) ||
        in_rs2ValueDbA > static_cast<float>(DEFAULT_MAX_RS2)) {
        LOG(ERROR) << __func__ << ": RS2 value is invalid: " << in_rs2ValueDbA;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    mRs2Value = in_rs2ValueDbA;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SoundDose::getOutputRs2UpperBound(float* _aidl_return) {
    *_aidl_return = mRs2Value;
    LOG(DEBUG) << __func__ << ": returning " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SoundDose::registerSoundDoseCallback(
        const std::shared_ptr<ISoundDose::IHalSoundDoseCallback>& in_callback) {
    if (in_callback.get() == nullptr) {
        LOG(ERROR) << __func__ << ": Callback is nullptr";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (mCallback != nullptr) {
        LOG(ERROR) << __func__ << ": Sound dose callback was already registered";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    mCallback = in_callback;
    LOG(DEBUG) << __func__ << ": Registered sound dose callback ";
    return ndk::ScopedAStatus::ok();
}

} // namespace qti::audio::core