/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>

#include "Lights.h"

using ::aidl::android::hardware::light::Lights;

std::shared_ptr<Lights> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<Lights>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (service == nullptr ) {
        return -1;
    }
    android::fuzzService(service->asBinder().get(), FuzzedDataProvider(data, size));

    return 0;
}
