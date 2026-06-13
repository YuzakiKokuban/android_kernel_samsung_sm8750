/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>

#include "Vibrator.h"

using aidl::android::hardware::vibrator::Vibrator;

std::shared_ptr<Vibrator> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<Vibrator>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (service == nullptr) {
        return -1;
    }
    android::fuzzService(service->asBinder().get(), FuzzedDataProvider(data, size));
    return 0;
}
