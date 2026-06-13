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
 * ​​​​​Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/core/sounddose/BnSoundDose.h>
#include <aidl/android/media/audio/common/AudioDevice.h>

#include <mutex>

using aidl::android::media::audio::common::AudioDevice;

namespace qti::audio::core {

class SoundDose : public ::aidl::android::hardware::audio::core::sounddose::BnSoundDose {
  public:
    SoundDose() : mRs2Value(DEFAULT_MAX_RS2){};

    ndk::ScopedAStatus setOutputRs2UpperBound(float in_rs2ValueDbA) override;
    ndk::ScopedAStatus getOutputRs2UpperBound(float* _aidl_return) override;
    ndk::ScopedAStatus registerSoundDoseCallback(
            const std::shared_ptr<ISoundDose::IHalSoundDoseCallback>& in_callback) override;

  private:
    std::shared_ptr<ISoundDose::IHalSoundDoseCallback> mCallback;
    float mRs2Value;
};

} // namespace qti::audio::core