/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "PalCallback.h"
#include <aidl/vendor/qti/hardware/pal/BnPALCallback.h>
#include <aidl/vendor/qti/hardware/pal/IPAL.h>
#include <aidl/vendor/qti/hardware/pal/PalReadWriteDoneResult.h>
#include <log/log.h>
#include <pal/BinderStatus.h>

using android::status_t;
using android::sp;

namespace aidl::vendor::qti::hardware::pal {

void DataTransferThread::startTransfer(int eventId) {
    size_t availToRead = mDataMQ->availableToRead();

    if (mDataMQ->read(&mBuffer[0], availToRead)) {
        ALOGV("%s: calling client callback, data size %zu", __func__, availToRead);

        const PalCallbackBuffer *rwDonePayload = (PalCallbackBuffer *)&mBuffer[0];
        auto cbBuffer = std::make_unique<pal_callback_buffer>();
        auto bufTimeSpec = std::make_unique<timespec>();
        if (!bufTimeSpec) {
            ALOGE("%s: Failed to allocate memory for timespec", __func__);
            return;
        }
        cbBuffer.get()->ts = (timespec *)bufTimeSpec.get();

        std::vector<uint8_t> buffData;
        cbBuffer->size = rwDonePayload->size;
        if (cbBuffer->size > 0 && rwDonePayload->buffer.size() == cbBuffer->size) {
            buffData.resize(cbBuffer->size);
            cbBuffer->buffer = buffData.data();
        }
        AidlToLegacy::convertPalCallbackBuffer(rwDonePayload, cbBuffer.get());
        mStreamCallback((pal_stream_handle_t *)mStreamHandle, eventId, (uint32_t *)cbBuffer.get(),
                        (uint32_t)availToRead, mStreamCookie);
    }
}

bool DataTransferThread::threadLoop() {
    while (!std::atomic_load_explicit(mStop, std::memory_order_acquire)) {
        uint32_t efState = 0;
        mEfGroup->wait(static_cast<uint32_t>(PalMessageQueueFlagBits::NOT_EMPTY), &efState);
        if (!(efState & static_cast<uint32_t>(PalMessageQueueFlagBits::NOT_EMPTY))) {
            continue; // Nothing to do.
        }
        PalReadWriteDoneCommand eventId;
        if (!mCommandMQ->read(&eventId)) {
            continue;
        }
        startTransfer((int)eventId);
        mEfGroup->wake(static_cast<uint32_t>(PalMessageQueueFlagBits::NOT_FULL));
    }

    return false;
}

::ndk::ScopedAStatus PalCallback::prepareMQForTransfer(int64_t handle, int64_t cookie,
                                                       PalCallbackReturnData *callbackData) {
    status_t status;
    // Create message queues.
    if (mDataMQ) {
        ALOGE("the client attempts to call prepareForWriting twice");
        callbackData->ret = PalReadWriteDoneResult::INVALID_STATE;
        return status_tToBinderResult(-EINVAL);
    }

    std::unique_ptr<DataMQ> tempDataMQ(
            new DataMQ(sizeof(PalCallbackBuffer) * 2, true /* EventFlag */));

    std::unique_ptr<CommandMQ> tempCommandMQ(new CommandMQ(1));
    if (!tempDataMQ->isValid() || !tempCommandMQ->isValid()) {
        ALOGE_IF(!tempDataMQ->isValid(), "data MQ is invalid");
        ALOGE_IF(!tempCommandMQ->isValid(), "command MQ is invalid");
        callbackData->ret = PalReadWriteDoneResult::INVALID_ARGUMENTS;
        return status_tToBinderResult(-EINVAL);
    }
    EventFlag *tempRawEfGroup{};
    status = EventFlag::createEventFlag(tempDataMQ->getEventFlagWord(), &tempRawEfGroup);
    std::unique_ptr<EventFlag, void (*)(EventFlag *)> tempElfGroup(
            tempRawEfGroup, [](auto *ef) { EventFlag::deleteEventFlag(&ef); });
    if (status != ::android::OK || !tempElfGroup) {
        ALOGE("failed creating event flag for data MQ: %s", strerror(-status));
        callbackData->ret = PalReadWriteDoneResult::INVALID_ARGUMENTS;
        return status_tToBinderResult(-EINVAL);
    }

    // Create and launch the thread.
    auto tempDataTransferThread = sp<DataTransferThread>::make(
            &mStopDataTransferThread, handle, mCallback, tempDataMQ.get(), tempCommandMQ.get(),
            tempElfGroup.get(), cookie);
    if (!tempDataTransferThread->init()) {
        ALOGW("failed to start writer thread: %s", strerror(-status));
        callbackData->ret = PalReadWriteDoneResult::INVALID_ARGUMENTS;
        return status_tToBinderResult(-EINVAL);
    }
    status = tempDataTransferThread->run("read_write_cb", ::android::PRIORITY_URGENT_AUDIO);
    if (status != ::android::OK) {
        ALOGW("failed to start read_write_cb thread: %s", strerror(-status));
        callbackData->ret = PalReadWriteDoneResult::INVALID_ARGUMENTS;
        return status_tToBinderResult(-EINVAL);
    }

    mDataMQ = std::move(tempDataMQ);
    mCommandMQ = std::move(tempCommandMQ);
    mDataTransferThread = tempDataTransferThread;
    mEfGroup = tempElfGroup.release();
    callbackData->ret = PalReadWriteDoneResult::OK;
    callbackData->mqDataDesc = mDataMQ->dupeDesc();
    callbackData->mqCommandDesc = mCommandMQ->dupeDesc();
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus PalCallback::eventCallback(int64_t handle, int32_t eventId,
                                                int32_t eventDataSize,
                                                const std::vector<uint8_t> &eventData,
                                                int64_t cookie) {
    uint32_t *evData = NULL;
    int8_t *src = NULL;
    evData = (uint32_t *)calloc(1, eventDataSize);
    if (!evData) {
        goto exit;
    }

    src = (int8_t *)eventData.data();
    memcpy(evData, src, eventDataSize);

    mCallback((pal_stream_handle_t *)handle, eventId, evData, eventDataSize, cookie);

exit:
    if (evData) {
        free(evData);
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus PalCallback::eventCallbackRWDone(
        int64_t handle, int32_t eventId, int32_t eventDataSize,
        const std::vector<::aidl::vendor::qti::hardware::pal::PalCallbackBuffer> &aidlRWDonePayload,
        int64_t cookie) {
    const ::aidl::vendor::qti::hardware::pal::PalCallbackBuffer *rwDonePayload =
            aidlRWDonePayload.data();
    auto cbBuffer = std::make_unique<pal_callback_buffer>();

    AidlToLegacy::convertPalCallbackBuffer(rwDonePayload, cbBuffer.get());
    mCallback((pal_stream_handle_t *)handle, eventId, (uint32_t *)cbBuffer.get(), eventDataSize,
              cookie);

    return ::ndk::ScopedAStatus::ok();
}

void PalCallback::cleanupDataTransferThread() {
    mStopDataTransferThread.store(true, std::memory_order_release);
    if (mEfGroup) {
        mEfGroup->wake(static_cast<uint32_t>(PalMessageQueueFlagBits::NOT_EMPTY));
    }
    if (mDataTransferThread.get()) {
        status_t status = mDataTransferThread->join();
        ALOGE_IF(status, "write thread exit error: %s", strerror(-status));
    }
    if (mEfGroup) {
        status_t status = EventFlag::deleteEventFlag(&mEfGroup);
        ALOGE_IF(status, "write MQ event flag deletion error: %s", strerror(-status));
    }
}

PalCallback::~PalCallback() {
    cleanupDataTransferThread();
}
}
