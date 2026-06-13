/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AgmIpc::Service"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include "AgmServerWrapper.h"

using namespace aidl::vendor::qti::hardware::agm;

extern "C" __attribute__((visibility("default"))) binder_status_t registerService() {
    ALOGI("register AGM Service");
    auto agmService = ::ndk::SharedRefBase::make<AgmServerWrapper>();
    ndk::SpAIBinder agmBinder = agmService->asBinder();
    const std::string interfaceName = std::string() + IAGM::descriptor + "/default";
    if (!agmService->isInitialized()) {
        ALOGE("failed to initialize AGM Service!");
        return -EINVAL;
    }

    if (!AServiceManager_isDeclared(interfaceName.c_str())) {
        ALOGW("%s interface %s is not declared in VINTF", __func__, interfaceName.c_str());
    }

    binder_status_t status = AServiceManager_addService(agmBinder.get(), interfaceName.c_str());
    ALOGI("register AGM Service interface %s registered %s status %d", interfaceName.c_str(),
          (status == STATUS_OK) ? "yes" : "no", status);
    return status;
}
