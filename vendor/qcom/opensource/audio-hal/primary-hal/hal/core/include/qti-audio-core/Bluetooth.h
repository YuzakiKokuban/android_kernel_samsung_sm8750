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
#include <aidl/android/hardware/audio/core/BnBluetooth.h>
#include <aidl/android/hardware/audio/core/BnBluetoothA2dp.h>
#include <aidl/android/hardware/audio/core/BnBluetoothLe.h>
#include <qti-audio-core/Platform.h>
#include <extensions/AudioExtension.h>

namespace qti::audio::core {

class Bluetooth : public ::aidl::android::hardware::audio::core::BnBluetooth {
  public:
    Bluetooth();

  private:
    ndk::ScopedAStatus setScoConfig(const ScoConfig& in_config, ScoConfig* _aidl_return) override;
    ndk::ScopedAStatus setHfpConfig(const HfpConfig& in_config, HfpConfig* _aidl_return) override;

    ScoConfig mScoConfig;
    HfpConfig mHfpConfig;
    Platform& mPlatform{Platform::getInstance()};
    AudioExtension& mAudExt{AudioExtension::getInstance()};
};

class BluetoothA2dp : public ::aidl::android::hardware::audio::core::BnBluetoothA2dp {
  public:
    BluetoothA2dp() = default;

  private:
    ndk::ScopedAStatus isEnabled(bool* _aidl_return) override;
    ndk::ScopedAStatus setEnabled(bool in_enabled) override;
    ndk::ScopedAStatus supportsOffloadReconfiguration(bool* _aidl_return) override;
    ndk::ScopedAStatus reconfigureOffload(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters) override;

    bool mEnabled = false;
    Platform& mPlatform{Platform::getInstance()};
};

class BluetoothLe : public ::aidl::android::hardware::audio::core::BnBluetoothLe {
  public:
    BluetoothLe() = default;

  private:
    ndk::ScopedAStatus isEnabled(bool* _aidl_return) override;
    ndk::ScopedAStatus setEnabled(bool in_enabled) override;
    ndk::ScopedAStatus supportsOffloadReconfiguration(bool* _aidl_return) override;
    ndk::ScopedAStatus reconfigureOffload(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters) override;

    bool mEnabled = false;
    Platform& mPlatform{Platform::getInstance()};
};

} // namespace qti::audio::core
