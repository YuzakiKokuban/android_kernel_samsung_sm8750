/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#pragma once

#include <mutex>
#include <vector>

struct PerfLockConfig {
    bool usePerfLock = false;
    std::string libraryName;
    std::vector<int> perfLockOpts;
};

/**
 * A Scoped object for real perf lock.
 * Only one among the existing PerfLock instances possibly acquires real perf lock.
 **/
class PerfLock final {
  public:
    // All Public APIs are gaurded by sMutex
    PerfLock(const std::string &);
    ~PerfLock();
    // Read config from resource_manager and set configs
    static void setPerfLockOpt(const PerfLockConfig & config);

  private:
    // disable copy
    PerfLock(const PerfLock&) = delete;
    PerfLock& operator=(const PerfLock& x) = delete;

    // disable move
    PerfLock(PerfLock&& other) = delete;
    PerfLock& operator=(PerfLock&& other) = delete;

    // function mapping for dlsym
    using AcquirePerfLock = int (*)(int, int, int*, int);
    using ReleasePerfLock = int (*)(int);

    inline static AcquirePerfLock sAcquirePerfLock{nullptr};
    inline static ReleasePerfLock sReleasePerfLock{nullptr};
    // this mutex is a class Mutex
    inline static std::mutex sMutex;
    inline static bool sIsAcquired = false;
    inline static int kPerfLockOptsSize = 0;
    inline static int sPerfLockCounter = 0;
    inline static std::vector<int> kPerfLockOpts;
    inline static int sHandle{0};
    inline static bool usePerfLock;
    inline static std::string sLibraryName;

    std::string mCaller;
    static bool init();

    // Below functions needs to be called with sMutex lock held
    void acquire_l();
    void release_l();
};
