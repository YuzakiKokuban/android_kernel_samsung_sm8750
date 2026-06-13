/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "Qspa.h"
#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>

using aidl::vendor::qti::hardware::qspa::Qspa;

std::shared_ptr<Qspa> service;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    service = ndk::SharedRefBase::make<Qspa>();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if( service == nullptr ) return 0;

    FuzzedDataProvider provider(data, size);
    android::fuzzService(service->asBinder().get(), std::move(provider));

    return 0;
}