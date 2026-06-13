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

#pragma once

#include <qti-audio-core/Stream.h>

namespace qti::audio::core {

class StreamStub : public StreamCommonImpl {
  public:
    StreamStub(StreamContext* context, const Metadata& metadata);
    // Methods of 'DriverInterface'.
    ::android::status_t init() override;
    ::android::status_t drain(
            ::aidl::android::hardware::audio::core::StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    void shutdown() override;

  private:
    const size_t mFrameSizeBytes;
    const int mSampleRate;
    const bool mIsAsynchronous;
    const bool mIsInput;
    bool mIsInitialized = false; // Used for validating the state machine logic.
    bool mIsStandby = true;      // Used for validating the state machine logic.
};

class StreamInStub final : public StreamIn, public StreamStub {
  public:
    friend class ndk::SharedRefBase;
    StreamInStub(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);
    ~StreamInStub() override;
    int32_t setAggregateSinkMetadata(bool) override;
    ndk::ScopedAStatus reconfigureConnectedDevices() override;

  private:
    void onClose() override { defaultOnClose(); }
};

class StreamOutStub final : public StreamOut, public StreamStub {
  public:
    friend class ndk::SharedRefBase;
    StreamOutStub(StreamContext&& context,
                  const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
                  const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                          offloadInfo);
    ~StreamOutStub() override;
    int32_t setAggregateSourceMetadata(bool) override;
    ndk::ScopedAStatus reconfigureConnectedDevices() override;

  private:
    void onClose() override { defaultOnClose(); }
};

} // namespace qti::audio::core
