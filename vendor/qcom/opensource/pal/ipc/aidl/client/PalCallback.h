/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalDefs.h>
#include <aidl/vendor/qti/hardware/pal/BnPALCallback.h>
#include <aidl/vendor/qti/hardware/pal/PalMessageQueueFlagBits.h>
#include <fmq/AidlMessageQueue.h>
#include <fmq/EventFlag.h>
#include <log/log.h>
#include <pal/PalAidlToLegacy.h>
#include <pal/PalLegacyToAidl.h>
#include <utils/Thread.h>

using ::android::AidlMessageQueue;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::hardware::EventFlag;
using ::android::Thread;
using android::sp;

namespace aidl::vendor::qti::hardware::pal {

class PalCallback : public BnPALCallback {
  public:
    typedef AidlMessageQueue<int8_t, SynchronizedReadWrite> DataMQ;
    typedef AidlMessageQueue<PalReadWriteDoneCommand, SynchronizedReadWrite> CommandMQ;
    ::ndk::ScopedAStatus eventCallback(int64_t handle, int32_t eventId, int32_t eventDataSize,
                                       const std::vector<uint8_t>& eventData,
                                       int64_t cookie) override;

    ::ndk::ScopedAStatus eventCallbackRWDone(
            int64_t handle, int32_t eventId, int32_t eventDataSize,
            const std::vector<::aidl::vendor::qti::hardware::pal::PalCallbackBuffer>&
                    aidlRWDonePayload,
            int64_t cookie) override;
    ::ndk::ScopedAStatus prepareMQForTransfer(int64_t handle, int64_t cookie,
                                              PalCallbackReturnData* aidlReturn) override;

    PalCallback(pal_stream_callback callBack) {
        if (callBack) {
            mCallback = callBack;
        }
    }
    void cleanupDataTransferThread();
    virtual ~PalCallback();

  protected:
    pal_stream_callback mCallback;
    std::unique_ptr<DataMQ> mDataMQ = nullptr;
    std::unique_ptr<CommandMQ> mCommandMQ = nullptr;
    EventFlag* mEfGroup = nullptr;
    std::atomic<bool> mStopDataTransferThread = false;
    sp<Thread> mDataTransferThread = nullptr;
    uint64_t mCookie;
};

class DataTransferThread : public Thread {
  public:
    DataTransferThread(std::atomic<bool>* stop, int64_t streamHandle,
                       pal_stream_callback clbkObject, PalCallback::DataMQ* dataMQ,
                       PalCallback::CommandMQ* commandMQ, EventFlag* efGroup, uint64_t cookie)
        : Thread(false),
          mStop(stop),
          mStreamHandle(streamHandle),
          mStreamCallback(clbkObject),
          mDataMQ(dataMQ),
          mCommandMQ(commandMQ),
          mEfGroup(efGroup),
          mBuffer(nullptr),
          mStreamCookie(cookie) {}
    bool init() {
        mBuffer.reset(new (std::nothrow) int8_t[mDataMQ->getQuantumCount()]);
        return mBuffer != nullptr;
    }
    virtual ~DataTransferThread() {}

  private:
    std::atomic<bool>* mStop;
    uint64_t mStreamHandle;
    pal_stream_callback mStreamCallback;
    PalCallback::DataMQ* mDataMQ;
    PalCallback::CommandMQ* mCommandMQ;
    EventFlag* mEfGroup;
    std::unique_ptr<int8_t[]> mBuffer;
    uint64_t mStreamCookie;

    bool threadLoop() override;

    void startTransfer(int eventId);
};
}
