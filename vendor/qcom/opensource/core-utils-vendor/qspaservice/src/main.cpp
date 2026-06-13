/*
  * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
  * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "Qspa.h"
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::vendor::qti::hardware::qspa::Qspa;
using namespace std;

int main(){
    ABinderProcess_setThreadPoolMaxThreadCount(8);
    ABinderProcess_startThreadPool();
    std::shared_ptr<Qspa> qspa = ndk::SharedRefBase::make<Qspa>();
    const std::string qspaName = std::string() + Qspa::descriptor + "/default";
    if (qspa == nullptr) {
        ALOGE("QSPA object is null, Failed to register !");
    }
    else {
        if (qspa->asBinder() == nullptr) {
           ALOGE("QSPA binder object is null!");
        }
        else {
           binder_status_t status = AServiceManager_addService(qspa->asBinder().get(), qspaName.c_str());
           CHECK_EQ(status, STATUS_OK);
           ALOGI("QSPA service registered !");
           ABinderProcess_joinThreadPool();
        }
    }
    return EXIT_FAILURE;
}