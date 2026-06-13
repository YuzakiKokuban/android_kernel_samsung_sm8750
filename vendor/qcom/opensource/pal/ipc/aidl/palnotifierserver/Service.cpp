/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalIpc::Service"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include "PalServerNotify.h"

using namespace aidl::vendor::qti::hardware::paleventnotifier;

extern "C" __attribute__((visibility("default"))) binder_status_t registerPalNotifierService() {
    ALOGI("register PAL Event Notifier Service");
    auto palEventNotifier = ::ndk::SharedRefBase::make<PalServerNotify>();
    ndk::SpAIBinder paleventnotifybinder = palEventNotifier->asBinder();
    const std::string interfaceName = std::string() + IPALEventNotifier::descriptor + "/default";

    binder_status_t status = AServiceManager_addService(paleventnotifybinder.get(), interfaceName.c_str());
    ALOGI("register PAL Event Notifier Service interface %s registered %s ", interfaceName.c_str(),
          (status == STATUS_OK) ? "yes" : "no");

    return status;
}
