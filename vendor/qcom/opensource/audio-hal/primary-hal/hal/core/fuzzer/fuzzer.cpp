/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <core-impl/AudioPolicyConfigXmlConverter.h>
#include <core-impl/ChildInterface.h>
#include <core-impl/Config.h>
#include <qti-audio-core/Module.h>
#include <qti-audio-core/ModulePrimary.h>

#include <fuzzbinder/libbinder_ndk_driver.h>
#include <fuzzer/FuzzedDataProvider.h>

#define LOG_TAG "AIDL_FUZZER_AUDIO_CORE_HAL"

using aidl::android::hardware::audio::core::internal::AudioPolicyConfigXmlConverter;

extern AudioPolicyConfigXmlConverter gAudioPolicyConverter;
extern std::shared_ptr<::aidl::android::hardware::audio::core::Config> gConfigDefaultAosp;
extern std::shared_ptr<::qti::audio::core::ModulePrimary> gModuleDefaultQti;

// init
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    gConfigDefaultAosp = ndk::SharedRefBase::make<::aidl::android::hardware::audio::core::Config>(gAudioPolicyConverter);
    gModuleDefaultQti = ndk::SharedRefBase::make<::qti::audio::core::ModulePrimary>();

    return 0;
}

// one fuzzing test case
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    FuzzedDataProvider provider(data, size);

    uint32_t index = provider.ConsumeIntegralInRange<uint32_t>(1, 2);

    if (index == 2 && gModuleDefaultQti != nullptr) {
        android::fuzzService(gModuleDefaultQti->asBinder().get(), std::move(provider));
    }

    if (index == 1 && gConfigDefaultAosp != nullptr) {
        android::fuzzService(gConfigDefaultAosp->asBinder().get(), std::move(provider));
    }

    return 0;
}