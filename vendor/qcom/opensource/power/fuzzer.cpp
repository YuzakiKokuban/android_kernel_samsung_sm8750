/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Power.h"

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <log/log.h>

using aidl::android::hardware::power::impl::Power;

std::shared_ptr<Power> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<Power>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( service == nullptr ) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(service->asBinder().get(),std::move(provider));

    return 0;
}