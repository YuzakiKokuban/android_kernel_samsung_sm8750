/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

namespace aidl::android::hardware::soundtrigger3 {

/**
 * @brief Helper class to read the opaque pointer from Fd
 * Scoped based management to automatically unmap shared memory
 * when the object goes out of scope.
 */
class SharedMemoryWrapper {
public :
    SharedMemoryWrapper(int fd, int size);
    ~SharedMemoryWrapper();
    const uint8_t* data();

private:
    int mSharedMemoryFd;
    int mExpectedMmapSize;
    void *mSharedMemory = nullptr;
};

} // namespace aidl::android::hardware::soundtrigger3
