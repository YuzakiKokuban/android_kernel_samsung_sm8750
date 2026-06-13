/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <vector>

#define LOG_TAG "AHAL_ModuleStub_QTI"
#include <Utils.h>
#include <android-base/logging.h>

#include <qti-audio-core/Bluetooth.h>
#include <qti-audio-core/ModuleStub.h>
#include <qti-audio-core/StreamStub.h>

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::MicrophoneInfo;

using ::aidl::android::hardware::audio::common::getFrameSizeInBytes;
using ::aidl::android::hardware::audio::common::isBitPositionFlagSet;
using ::aidl::android::hardware::audio::common::isValidAudioMode;
using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;

using ::aidl::android::hardware::audio::core::AudioPatch;
using ::aidl::android::hardware::audio::core::AudioRoute;
using ::aidl::android::hardware::audio::core::IBluetooth;
using ::aidl::android::hardware::audio::core::IBluetoothA2dp;
using ::aidl::android::hardware::audio::core::IBluetoothLe;
using ::aidl::android::hardware::audio::core::IStreamIn;
using ::aidl::android::hardware::audio::core::IStreamOut;
using ::aidl::android::hardware::audio::core::VendorParameter;

namespace qti::audio::core {

ndk::ScopedAStatus ModuleStub::getBluetooth(std::shared_ptr<IBluetooth>* _aidl_return) {
    if (!mBluetooth) {
        mBluetooth = ndk::SharedRefBase::make<Bluetooth>();
    }
    *_aidl_return = mBluetooth.getInstance();
    LOG(DEBUG) << __func__
               << ": returning instance of IBluetooth: " << _aidl_return->get()->asBinder().get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleStub::getBluetoothA2dp(std::shared_ptr<IBluetoothA2dp>* _aidl_return) {
    if (!mBluetoothA2dp) {
        mBluetoothA2dp = ndk::SharedRefBase::make<BluetoothA2dp>();
    }
    *_aidl_return = mBluetoothA2dp.getInstance();
    LOG(DEBUG) << __func__ << ": returning instance of IBluetoothA2dp: "
               << _aidl_return->get()->asBinder().get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleStub::getBluetoothLe(std::shared_ptr<IBluetoothLe>* _aidl_return) {
    if (!mBluetoothLe) {
        mBluetoothLe = ndk::SharedRefBase::make<BluetoothLe>();
    }
    *_aidl_return = mBluetoothLe.getInstance();
    LOG(DEBUG) << __func__
               << ": returning instance of IBluetoothLe: " << _aidl_return->get()->asBinder().get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleStub::createInputStream(StreamContext&& context,
                                                 const SinkMetadata& sinkMetadata,
                                                 const std::vector<MicrophoneInfo>& microphones,
                                                 std::shared_ptr<StreamIn>* result) {
    return createStreamInstance<StreamInStub>(result, std::move(context), sinkMetadata,
                                              microphones);
}

ndk::ScopedAStatus ModuleStub::createOutputStream(
        StreamContext&& context, const SourceMetadata& sourceMetadata,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    return createStreamInstance<StreamOutStub>(result, std::move(context), sourceMetadata,
                                               offloadInfo);
}

} // namespace qti::audio::core
