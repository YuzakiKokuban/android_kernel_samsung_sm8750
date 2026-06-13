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

#include <cstddef>
#include <memory>

#define LOG_TAG "AHAL_EffectThreadQti"
#include <android-base/logging.h>
#include <pthread.h>
#include <sys/resource.h>

#include "effect-impl/EffectThread.h"
#include "effect-impl/EffectTypes.h"

using ::android::hardware::EventFlag;
using ::aidl::android::hardware::audio::effect::kEventFlagNotEmpty;
namespace aidl::qti::effects {

#define RET_SUCCESS_IF_THREAD_NOT_CREATED(threadCreated)                    \
    {                                                                       \
        if (!threadCreated) {                                               \
            LOG(VERBOSE) << __func__ << " no-op as thread wasn't created "; \
            return RetCode::SUCCESS;                                        \
        }                                                                   \
    }

EffectThread::EffectThread() {
    LOG(VERBOSE) << __func__ << this;
}

EffectThread::~EffectThread() {
    destroyThread();
    LOG(VERBOSE) << __func__ << " done" << this;
}
RetCode EffectThread::createThread(const std::string& name, int priority) {
    if (mThread.joinable()) {
        LOG(WARNING) << mName << __func__ << " thread already created, no-op";
        return RetCode::SUCCESS;
    }
    mName = name;
    mPriority = priority;
    {
        std::lock_guard lg(mThreadMutex);
        mStop = true;
        mExit = false;
    }

    mThread = std::thread(&EffectThread::threadLoop, this);
    mThreadCreated = true;
    LOG(VERBOSE) << mName << __func__ << " priority " << mPriority << " done";
    return RetCode::SUCCESS;
}

RetCode EffectThread::destroyThread() {
    RET_SUCCESS_IF_THREAD_NOT_CREATED(mThreadCreated);

    {
        std::lock_guard lg(mThreadMutex);
        mStop = mExit = true;
    }
    mCv.notify_one();

    if (mThread.joinable()) {
        mThread.join();
    }

    LOG(DEBUG) << mName << __func__;
    return RetCode::SUCCESS;
}

RetCode EffectThread::startThread() {
    RET_SUCCESS_IF_THREAD_NOT_CREATED(mThreadCreated);

    {
        std::lock_guard lg(mThreadMutex);
        mStop = false;
        mCv.notify_one();
    }

    LOG(DEBUG) << mName << __func__;
    return RetCode::SUCCESS;
}

RetCode EffectThread::stopThread() {
    RET_SUCCESS_IF_THREAD_NOT_CREATED(mThreadCreated);

    {
        std::lock_guard lg(mThreadMutex);
        mStop = true;
        mCv.notify_one();
    }

    LOG(DEBUG) << mName << __func__;
    return RetCode::SUCCESS;
}

void EffectThread::threadLoop() {
    pthread_setname_np(pthread_self(), mName.substr(0, kMaxTaskNameLen - 1).c_str());
    setpriority(PRIO_PROCESS, 0, mPriority);
    LOG(VERBOSE) << mName << __func__ << "Enter: ";
    while (true) {
        {
            std::unique_lock l(mThreadMutex);
            ::android::base::ScopedLockAssertion lock_assertion(mThreadMutex);
            mCv.wait(l, [&]() REQUIRES(mThreadMutex) { return mExit || !mStop; });
            if (mExit) {
                LOG(VERBOSE) << " " << mName << __func__ << " EXIT!";
                return;
            }
        }
        process();
    }
}

} // namespace aidl::qti::effects
