/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define NDEBUG 0
#define LOG_TAG "PAL: PerfLock"
#include <dlfcn.h>
#include "PalCommon.h"
#include "PerfLock.h"

#include <sstream>

PerfLock::PerfLock(const std::string &caller) : mCaller(caller) {
    static bool isInit = init();
    std::scoped_lock lock (sMutex);
    acquire_l();
}

PerfLock::~PerfLock() {
    std::scoped_lock lock (sMutex);
    release_l();
}

void PerfLock::acquire_l() {
    ++sPerfLockCounter;
    if (!sIsAcquired && sAcquirePerfLock != nullptr) {
        sHandle = sAcquirePerfLock(0, 0, kPerfLockOpts.data(), kPerfLockOptsSize);
        if (sHandle > 0) {
            sIsAcquired = true;
            PAL_VERBOSE(LOG_TAG, "succesful perf_lock_acq for %s", mCaller.c_str());
        } else {
            PAL_VERBOSE(LOG_TAG, "failed perf_lock_acq for %s", mCaller.c_str());
        }
    }
}

void PerfLock::release_l() {
    --sPerfLockCounter;
    if (sHandle > 0 && sReleasePerfLock != nullptr && (sPerfLockCounter == 0)) {
        sReleasePerfLock(sHandle);
        sIsAcquired = false;
        PAL_VERBOSE(LOG_TAG, "succesful perf_lock_rel for %s", mCaller.c_str());
    } else {
        PAL_VERBOSE(LOG_TAG, "failed perf_lock_rel for %s", mCaller.c_str());
    }
}

void PerfLock::setPerfLockOpt(const PerfLockConfig & config) {
      if (config.usePerfLock) {
        kPerfLockOptsSize = config.perfLockOpts.size();
        kPerfLockOpts = config.perfLockOpts;
        sLibraryName = config.libraryName;
    }
}

// static
bool PerfLock::init() {
    void* libHandle = dlopen(sLibraryName.c_str(), RTLD_LAZY);
    if (libHandle == nullptr) {
        const char* error = dlerror();
        PAL_ERR(LOG_TAG, "Failed to dlopen %s error %s", sLibraryName.c_str(), error);
        return false;
    }
    sAcquirePerfLock = reinterpret_cast<AcquirePerfLock>(dlsym(libHandle, "perf_lock_acq"));
    if (sAcquirePerfLock == nullptr) {
        PAL_ERR(LOG_TAG, "failed to find perf_lock_acq ");
        dlclose(libHandle);
        return false;
    }
    sReleasePerfLock = reinterpret_cast<ReleasePerfLock>(dlsym(libHandle, "perf_lock_rel"));
    if (sReleasePerfLock == nullptr) {
        PAL_ERR(LOG_TAG, "failed to find perf_lock_rel ");
        dlclose(libHandle);
        return false;
    }

    std::stringstream hexStream;

    for (const auto i : kPerfLockOpts) {
        hexStream << std::hex << i << " ";
    }

    PAL_INFO(LOG_TAG, "initialized perflock library %s size %d, locks %s", sLibraryName.c_str(),
        kPerfLockOptsSize, hexStream.str().c_str());

    return true;
}
