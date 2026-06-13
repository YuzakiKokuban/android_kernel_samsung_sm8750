/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalIpc::Service"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include "PalServerWrapper.h"

using namespace aidl::vendor::qti::hardware::pal;

extern "C" __attribute__((visibility("default"))) binder_status_t registerService() {
    ALOGI("register PAL Service");
    auto palService = ::ndk::SharedRefBase::make<PalServerWrapper>();
    ndk::SpAIBinder palBinder = palService->asBinder();
    const std::string interfaceName = std::string() + IPAL::descriptor + "/default";

    binder_status_t status = AServiceManager_addService(palBinder.get(), interfaceName.c_str());
    ALOGI("register PAL Service interface %s registered %s ", interfaceName.c_str(),
          (status == STATUS_OK) ? "yes" : "no");
    return status;
}
