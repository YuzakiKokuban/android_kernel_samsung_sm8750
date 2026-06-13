/*
 * Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "QTI PowerHAL"

#include "Power.h"
#include "PowerHintSession.h"

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>
#include <fmq/EventFlag.h>
#include <thread>

#include <aidl/android/hardware/power/BnPower.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using ::aidl::android::hardware::power::BnPower;
using ::aidl::android::hardware::power::IPower;
using ::aidl::android::hardware::power::Mode;
using ::aidl::android::hardware::power::Boost;
using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::aidl::android::hardware::power::ChannelMessage;
using ::android::AidlMessageQueue;

using ::ndk::ScopedAStatus;
using ::ndk::SharedRefBase;

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {

void setInteractive(bool interactive) {
   set_interactive(interactive ? 1:0);
}

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {
    LOG(INFO) << "Power setMode: " << static_cast<int32_t>(type) << " to: " << enabled;
    switch(type){
        case Mode::DOUBLE_TAP_TO_WAKE:
        case Mode::LOW_POWER:
        case Mode::LAUNCH:
        case Mode::DEVICE_IDLE:
        case Mode::DISPLAY_INACTIVE:
        case Mode::AUDIO_STREAMING_LOW_LATENCY:
        case Mode::CAMERA_STREAMING_SECURE:
        case Mode::CAMERA_STREAMING_LOW:
        case Mode::CAMERA_STREAMING_MID:
        case Mode::CAMERA_STREAMING_HIGH:
        case Mode::VR:
            LOG(INFO) << "Mode " << static_cast<int32_t>(type) << "Not Supported";
            break;
        case Mode::EXPENSIVE_RENDERING:
            set_expensive_rendering(enabled);
            break;
        case Mode::INTERACTIVE:
            //setInteractive(enabled);
            //power_hint(POWER_HINT_INTERACTION, NULL);
            break;
        case Mode::SUSTAINED_PERFORMANCE:
        case Mode::FIXED_PERFORMANCE:
            power_hint(POWER_HINT_SUSTAINED_PERFORMANCE, NULL);
            break;
        default:
            LOG(INFO) << "Mode " << static_cast<int32_t>(type) << "Not Supported";
            break;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    LOG(INFO) << "Power isModeSupported: " << static_cast<int32_t>(type);

    switch(type){
        case Mode::EXPENSIVE_RENDERING:
            if (is_expensive_rendering_supported()) {
                *_aidl_return = true;
            } else {
                *_aidl_return = false;
            }
            break;
        case Mode::INTERACTIVE:
        case Mode::SUSTAINED_PERFORMANCE:
        case Mode::FIXED_PERFORMANCE:
            *_aidl_return = true;
            break;
        default:
            *_aidl_return = false;
            break;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    LOG(INFO) << "Power setBoost: " << static_cast<int32_t>(type)
                 << ", duration: " << durationMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool* _aidl_return) {
    LOG(INFO) << "Power isBoostSupported: " << static_cast<int32_t>(type);
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::createHintSession(int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos,
                                            std::shared_ptr<IPowerHintSession>* _aidl_return) {
    LOG(INFO) << "Power createHintSession";
    if (threadIds.size() == 0) {
        LOG(ERROR) << "Error: threadIds.size() shouldn't be " << threadIds.size();
        *_aidl_return = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    *_aidl_return = setPowerHintSession(tgid, uid, threadIds);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::createHintSessionWithConfig(
        int32_t tgid, int32_t uid, const std::vector<int32_t>& threadIds, int64_t durationNanos,
        SessionTag, SessionConfig* config, std::shared_ptr<IPowerHintSession>* _aidl_return) 
{
    auto out = createHintSession(tgid, uid, threadIds, durationNanos, _aidl_return);
    static_cast<PowerHintSessionImpl*>(_aidl_return->get())->getSessionConfig(config);
    return out;
}

ndk::ScopedAStatus Power::getSessionChannel(int32_t, int32_t, ChannelConfig* _aidl_return) {
    static AidlMessageQueue<ChannelMessage, SynchronizedReadWrite> stubQueue{20, true};
    static std::thread stubThread([&] {
        ChannelMessage data;
        // This loop will only run while there is data waiting
        // to be processed, and blocks on a futex all other times
        while (stubQueue.readBlocking(&data, 1, 0)) {
        }
    });
    _aidl_return->channelDescriptor = stubQueue.dupeDesc();
    _aidl_return->readFlagBitmask = 0x01;
    _aidl_return->writeFlagBitmask = 0x02;
    _aidl_return->eventFlagDescriptor = std::nullopt;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::closeSessionChannel(int32_t, int32_t) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getHintSessionPreferredRate(int64_t* outNanoseconds) {
    LOG(INFO) << "Power getHintSessionPreferredRate";
    *outNanoseconds = getSessionPreferredRate();
    return ndk::ScopedAStatus::ok();
}

}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
