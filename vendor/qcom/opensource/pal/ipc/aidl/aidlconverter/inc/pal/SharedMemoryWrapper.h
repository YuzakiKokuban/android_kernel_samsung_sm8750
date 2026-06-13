/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

namespace aidl::vendor::qti::hardware::pal {

/**
 * @brief Helper class to read the opaque pointer from Fd
 * Scoped based management to automatically unmap shared memory
 * when the object goes out of scope.
 */
class SharedMemoryWrapper {
  public:
    /** Use this instantiation when shared memory creation and mapping
      * is needed. It takes size as an argument to know how much memory
      * area needs to be mapped.
      *
      * In this instantiation, shared memory fd is created, checked
      * for validity and mapped. During destruction, close of fd is
      * needed, so it sets mCloseFd to true.
      */
    SharedMemoryWrapper(int size);

    /** Use this instantiation when only mapping of existing fd
      * is needed. It takes shared Fd that needs to be mapped and size to
      * know the mapping size as arguments.
      *
      * In this instantiation, shared memory fd is checked for validity
      * and mapped. During destruction, close of fd is not needed as no fd
      * is created, so it sets mCloseFd to false.
      */
    SharedMemoryWrapper(int sharedFd, int size);
    ~SharedMemoryWrapper();
    int validSharedMemory(int sharedFd, int size);
    void* mapSharedMemory(int sharedFd, int size);
    void* getData();
    int getFd();

  private:
    int mSharedFd;
    int mSize;
    void* mSharedMemory = nullptr;
    bool mCloseFd;
};
}
