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

#define LOG_TAG "AHAL_Bluetooth_QTI"
#include <android-base/logging.h>
#include <cutils/properties.h>
#include <qti-audio-core/Bluetooth.h>
#include <qti-audio-core/Telephony.h>

using aidl::android::hardware::audio::core::VendorParameter;
using aidl::android::media::audio::common::Boolean;
using aidl::android::media::audio::common::Float;
using aidl::android::media::audio::common::Int;

namespace qti::audio::core {

Bluetooth::Bluetooth() {
    mScoConfig.isEnabled = Boolean{false};
    mScoConfig.isNrecEnabled = Boolean{false};
    mScoConfig.mode = ScoConfig::Mode::SCO;
    mHfpConfig.isEnabled = Boolean{false};
    mHfpConfig.sampleRate = Int{8000};
    mHfpConfig.volume = Float{HfpConfig::VOLUME_MAX};
}

ndk::ScopedAStatus Bluetooth::setScoConfig(const ScoConfig& in_config, ScoConfig* _aidl_return) {
    if (in_config.isEnabled.has_value()) {
        mScoConfig.isEnabled = in_config.isEnabled;
        mScoConfig.isEnabled.value().value == true ? mPlatform.setBluetoothParameters("BT_SCO=on")
                                                   : mPlatform.setBluetoothParameters("BT_SCO=off");
        /* never call telephony with any lock held*/
        if(auto telephony = mPlatform.getTelephony().lock()) {
            // Todo remove the unsafe casting, although we ITelephony is also Telephony
            auto tele = static_cast<Telephony*>(telephony.get());
            tele->onBluetoothScoEvent(mScoConfig.isEnabled.value().value);
        }
    }
    if (in_config.isNrecEnabled.has_value()) {
        mScoConfig.isNrecEnabled = in_config.isNrecEnabled;
        mScoConfig.isNrecEnabled.value().value == true
                ? mPlatform.setBluetoothParameters("bt_headset_nrec=on")
                : mPlatform.setBluetoothParameters("bt_headset_nrec=off");
    }
    if (in_config.mode != ScoConfig::Mode::UNSPECIFIED) {
        mScoConfig.mode = in_config.mode;
        if (mScoConfig.mode == ScoConfig::Mode::SCO) {
            mPlatform.setBluetoothParameters("bt_wbs=off");
        }
        if (mScoConfig.mode == ScoConfig::Mode::SCO_WB) {
            mPlatform.setBluetoothParameters("bt_wbs=on");
        } else if (mScoConfig.mode == ScoConfig::Mode::SCO_SWB) {
            mPlatform.setBluetoothParameters("bt_swb=1");
        }
    }
    if (in_config.debugName.has_value()) {
        mScoConfig.debugName = in_config.debugName;
    }
    *_aidl_return = mScoConfig;
    LOG(DEBUG) << __func__ << ": received " << in_config.toString() << ", returning "
               << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Bluetooth::setHfpConfig(const HfpConfig& in_config, HfpConfig* _aidl_return) {
    struct str_parms* parms = nullptr;
    std::string kvpairs = "";
    if (in_config.sampleRate.has_value() && in_config.sampleRate.value().value <= 0) {
        LOG(ERROR) << __func__ << ": invalid sample rate: " << in_config.sampleRate.value().value;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_config.volume.has_value() &&
        (in_config.volume.value().value < static_cast<float>(HfpConfig::VOLUME_MIN) ||
         in_config.volume.value().value > static_cast<float>(HfpConfig::VOLUME_MAX))) {
        LOG(ERROR) << __func__ << ": invalid volume: " << in_config.volume.value().value;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (in_config.isEnabled.has_value()) {
        mHfpConfig.isEnabled = in_config.isEnabled;
        std::string isEnabled = in_config.isEnabled.value().value?"true":"false";
        kvpairs += "hfp_enable=" + isEnabled + ";";
    }
    if (in_config.sampleRate.has_value()) {
        mHfpConfig.sampleRate = in_config.sampleRate;
        kvpairs += "hfp_set_sampling_rate=" + std::to_string(in_config.sampleRate.value().value) + ";";
    }
    if (in_config.volume.has_value()) {
        mHfpConfig.volume = in_config.volume;
        kvpairs += "hfp_volume=" + std::to_string(in_config.volume.value().value) + ";";
    }
    if (!kvpairs.empty()) {
        parms = str_parms_create_str(kvpairs.c_str());
        mAudExt.audio_extn_set_parameters(parms);
#ifdef SEC_AUDIO_COMMON
        if (parms)
            str_parms_destroy(parms);
#endif
    }
    *_aidl_return = mHfpConfig;
    LOG(DEBUG) << __func__ << ": received " << in_config.toString() << ", returning "
               << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::isEnabled(bool* _aidl_return) {
    *_aidl_return = mEnabled;
    LOG(DEBUG) << __func__ << ": returning " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::setEnabled(bool in_enabled) {
    mEnabled = in_enabled;
    mEnabled == true ? mPlatform.setBluetoothParameters("A2dpSuspended=false")
                     : mPlatform.setBluetoothParameters("A2dpSuspended=true");
    LOG(DEBUG) << __func__ << ": " << mEnabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::supportsOffloadReconfiguration(bool* _aidl_return) {
    bool supportReconfig = property_get_bool("ro.bluetooth.a2dp_offload.supported", false) &&
                           !property_get_bool("persist.bluetooth.a2dp_offload.disabled", false);
    *_aidl_return = supportReconfig;
    LOG(DEBUG) << __func__ << ": returning " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothA2dp::reconfigureOffload(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters
                __unused) {
    LOG(DEBUG) << __func__ << ": " << ::android::internal::ToString(in_parameters);
    mPlatform.setBluetoothParameters("reconfigA2dp=true");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::isEnabled(bool* _aidl_return) {
    *_aidl_return = mEnabled;
    LOG(DEBUG) << __func__ << ": returning " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::setEnabled(bool in_enabled) {
    mEnabled = in_enabled;
    mEnabled == true ? mPlatform.setBluetoothParameters("LeAudioSuspended=false")
                     : mPlatform.setBluetoothParameters("LeAudioSuspended=true");
    LOG(DEBUG) << __func__ << ": " << mEnabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::supportsOffloadReconfiguration(bool* _aidl_return) {
    *_aidl_return = true;
    LOG(DEBUG) << __func__ << ": returning " << *_aidl_return;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothLe::reconfigureOffload(
        const std::vector<::aidl::android::hardware::audio::core::VendorParameter>& in_parameters
                __unused) {
    LOG(DEBUG) << __func__ << ": " << ::android::internal::ToString(in_parameters);
    return ndk::ScopedAStatus::ok();
}

} // namespace qti::audio::core
