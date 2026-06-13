/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalSharedMemoryWrapper"

#include <android-base/macros.h>
#include <cutils/ashmem.h>
#include <log/log.h>
#include <pal/SharedMemoryWrapper.h>
#include <sys/mman.h>

namespace aidl::vendor::qti::hardware::pal {

SharedMemoryWrapper::SharedMemoryWrapper(int size) : mSize(size) {
    int status = 0;
    int sharedFd = ashmem_create_region("pal_ipc_shmem", size);

    status = validSharedMemory(sharedFd, size);
    if (status) {
        LOG_ALWAYS_FATAL("%s: Failed to create ashmem region of size %d", __func__, size);
    } else {
        mSharedMemory = mapSharedMemory(sharedFd, size);
        if (mSharedMemory == MAP_FAILED) {
            LOG_ALWAYS_FATAL("%s: Failed to map SharedMemory fd %d", __func__, sharedFd);
        }
    }
    mSharedFd = sharedFd;
    mCloseFd = true;
    ALOGV("%s ptr %p", __func__, mSharedMemory);
}

SharedMemoryWrapper::SharedMemoryWrapper(int sharedFd, int size)
    : mSharedFd(sharedFd), mSize(size) {
    int status = 0;
    status = validSharedMemory(sharedFd, size);
    if (status) {
        LOG_ALWAYS_FATAL("%s: Failed to create ashmem region of size %d", __func__, size);
    } else {
        mSharedMemory = mapSharedMemory(sharedFd, size);
        if (mSharedMemory == MAP_FAILED) {
            LOG_ALWAYS_FATAL("%s: Failed to map SharedMemory fd %d", __func__, sharedFd);
        }
    }
    mCloseFd = false;
    ALOGV("%s ptr %p", __func__, mSharedMemory);
}

void* SharedMemoryWrapper::getData() {
    ALOGV("%s ptr %p", __func__, mSharedMemory);
    return mSharedMemory;
}

int SharedMemoryWrapper::getFd() {
    return mSharedFd;
}

int SharedMemoryWrapper::validSharedMemory(int sharedFd, int size) {
    int status = 0;
    ALOGV("%s: SharedMemory fd %d, size %d", __func__, sharedFd, size);
    if ((sharedFd < 0) || !ashmem_valid(sharedFd) || (size != ashmem_get_size_region(sharedFd))) {
        ALOGE("%s: Invalid SharedMemory fd %d", __func__, sharedFd);
        status = -1;
    }
    return status;
}

void* SharedMemoryWrapper::mapSharedMemory(int sharedFd, int size) {
    ALOGV("%s: SharedMemory fd %d, size %d", __func__, sharedFd, size);
    void* mSharedMemory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, sharedFd, 0);
    return mSharedMemory;
}

SharedMemoryWrapper::~SharedMemoryWrapper() {
    if (mSharedMemory != nullptr && munmap(mSharedMemory, mSize) < 0) {
        ALOGE("%s: unmap failed %d", __func__, mSharedFd);
    }
    if (mCloseFd) {
        ALOGI("%s Closing fd %d", __func__, mSharedFd);
        close(mSharedFd);
    }
}
}
