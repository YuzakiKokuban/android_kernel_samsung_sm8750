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
#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <android-base/thread_annotations.h>
#include <fmq/EventFlag.h>
#include <system/thread_defs.h>

#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectTypes.h"

namespace aidl::qti::effects {

class EffectThread {
  public:
    // default priority is same as HIDL: ANDROID_PRIORITY_URGENT_AUDIO
    EffectThread();
    virtual ~EffectThread();

    // called by effect implementation.
    RetCode createThread(const std::string& name, int priority = ANDROID_PRIORITY_URGENT_AUDIO);
    RetCode destroyThread();
    RetCode startThread();
    RetCode stopThread();

    // Will call process() in a loop if the thread is running.
    void threadLoop();

    /**
     * process() call effectProcessImpl() for effect data processing, it is necessary for the
     * processing to be called under Effect thread mutex mThreadMutex, to avoid the effect state
     * change before/during data processing, and keep the thread and effect state consistent.
     */
    virtual void process() = 0;

  private:
    static constexpr int kMaxTaskNameLen = 15;

    std::mutex mThreadMutex;
    std::condition_variable mCv;
    bool mStop GUARDED_BY(mThreadMutex) = true;
    bool mExit GUARDED_BY(mThreadMutex) = false;
    std::thread mThread;
    int mPriority;
    std::string mName;
    bool mThreadCreated = false;
};
} // namespace aidl::qti::effects
