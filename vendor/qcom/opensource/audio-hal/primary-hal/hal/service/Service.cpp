/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NDEBUG 0
#define LOG_TAG "AHAL_Service_QTI"

#include <dlfcn.h>
#include <cstdlib>
#include <ctime>

#include <algorithm>

#include <chrono>
#include <string>
#include <vector>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_ibinder_platform.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/ProcessState.h>
#include <log/log.h>
#include "ConfigManager.h"

#define REGISTER_RETRY_COUNT 10
#define SLEEP_TIME_SECONDS 1

static bool registerServiceImplementation(const Interface& interface) {
    auto libraryName = interface.libraryName;
    auto interfaceMethod = interface.method;
    void* handle = dlopen(libraryName.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        const char* error = dlerror();
        ALOGE("Failed to dlopen %s: %s", libraryName.c_str(),
              error != nullptr ? error : "unknown error");
        return false;
    }
    auto instantiate =
            reinterpret_cast<binder_status_t (*)()>(dlsym(handle, interfaceMethod.c_str()));
    if (instantiate == nullptr) {
        const char* error = dlerror();
        ALOGE("Factory function %s not found in libName %s: %s", interfaceMethod.c_str(),
              libraryName.c_str(), error != nullptr ? error : "unknown error");
        dlclose(handle);
        return false;
    }
    return (instantiate() == STATUS_OK);
}

void registerInterfaces(const Interfaces& interfaces) {
    for (const auto& interface : interfaces) {
        if (registerServiceImplementation(interface)) {
            ALOGI("successfully registered %s", interface.toString().c_str());
        } else if (interface.mandatory) {
            int32_t retryCount = 0;
            bool isRegistered = false;
            while (retryCount < REGISTER_RETRY_COUNT) {
                ALOGI("failed to register service: %s, retry count: %d",
                        interface.toString().c_str(), retryCount + 1);
                isRegistered = registerServiceImplementation(interface);
                if (isRegistered) {
                    ALOGI("successfully registered %s", interface.toString().c_str());
                    break;
                } else {
                    //the service may failed to register due to resource busy, sleep and try again
                    sleep(SLEEP_TIME_SECONDS);
                }
                ++retryCount;
            }
            LOG_ALWAYS_FATAL_IF(!isRegistered, "failed to register %s ",
                                interface.toString().c_str());
        } else {
            ALOGW("failed to register optional %s ", interface.toString().c_str());
        }
    }
}

bool registerFromConfigs() {
    auto interfaces = parseInterfaces();
    registerInterfaces(interfaces);
    return !interfaces.empty();
}

/*
* Don't modify default entries unless the library is a must for stub mode bootup.
*/
void registerDefaultInterfaces() {
    Interfaces defaultInterfaces = {
            {.name = "audiohal-default",
             .libraryName = "libaudiocorehal.default.so",
             .method = "registerServices",
             .mandatory = true},
            {.name = "audioeffecthal",
             .libraryName = "libaudioeffecthal.qti.so",
             .method = "registerService",
             .mandatory = true},
            {.name = "bthal",
             .libraryName = "android.hardware.bluetooth.audio_sw.so",
             .method = "registerIModuleBluetoothSWQti",
             .mandatory = false},
    };

    registerInterfaces(defaultInterfaces);
}

void registerAvailableInterfaces() {
    auto stubmode = ::android::base::GetIntProperty<int8_t>("vendor.audio.hal.stubmode", 0);
    if (stubmode || !registerFromConfigs()) {
        ALOGI("registerDefaultInterfaces stub mode %d", stubmode);
        registerDefaultInterfaces();
    }
}

void setLogSeverity() {
    // by default use DEBUG logging enabled
    auto logLevel = ::android::base::GetIntProperty<int8_t>("vendor.audio.hal.loglevel", 1);
    // system/libbase/include/android-base/logging.h, check LogSeverity for types
    android::base::SetMinimumLogSeverity(static_cast<::android::base::LogSeverity>(logLevel));
}

int main() {
    auto startTime = std::chrono::steady_clock::now();
    // Random values are used in the implementation.
    std::srand(std::time(nullptr));
    setLogSeverity();

    ABinderProcess_setThreadPoolMaxThreadCount(16);
    ABinderProcess_startThreadPool();

    registerAvailableInterfaces();
    auto endTime = std::chrono::steady_clock::now();
    float timeTaken =
            std::chrono::duration_cast<std::chrono::duration<float>>(endTime - startTime).count();
    ALOGI("registration took %.2f seconds ", timeTaken);
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
