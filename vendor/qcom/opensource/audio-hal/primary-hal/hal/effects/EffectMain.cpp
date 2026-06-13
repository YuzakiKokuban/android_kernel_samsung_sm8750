/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_EffectMainQti"

#include "effectFactory-impl/EffectFactory.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android-base/properties.h>
#include <system/audio_config.h>

// { SEC_AUDIO_SUPPORT_AIDL_EFFECT
#include <system/audio.h>
// } SEC_AUDIO_SUPPORT_AIDL_EFFECT

#ifdef SEC_AUDIO_SUPPORT_AIDL_EFFECT
static const char* kDefaultConfigName = "audio_effects_config_sec.xml";
#else
/** Default name of effect configuration file. */
static const char* kDefaultConfigName = "audio_effects_config.xml";
#endif
static const char* kStubConfigName = "audio_effects_config_stub.xml";

static inline std::string getEffectConfig() {
    auto stubmode = ::android::base::GetIntProperty<int8_t>("vendor.audio.hal.stubmode", 0);
    if (stubmode) {
        LOG(INFO) << __func__ << " using effects in stub mode";
        return android::audio_find_readable_configuration_file(kStubConfigName);
    }

    return android::audio_find_readable_configuration_file(kDefaultConfigName);
}

extern "C" __attribute__((visibility("default"))) binder_status_t registerService() {
    auto configFile = getEffectConfig();
    if (configFile == "") {
        LOG(ERROR) << __func__ << ": config file " << kDefaultConfigName << " not found!";
        return EXIT_FAILURE;
    }
    LOG(INFO) << __func__ << ": start factory with configFile:" << configFile;
    auto effectFactory = ndk::SharedRefBase::make<aidl::qti::effects::Factory>(configFile);
    int version = 0;
    effectFactory->getInterfaceVersion(&version);
    std::string serviceName = std::string() + effectFactory->descriptor + "/default";
    binder_status_t status =
            AServiceManager_addService(effectFactory->asBinder().get(), serviceName.c_str());
    LOG(DEBUG) << __func__ << " " << serviceName << " version " << version << " status " << status;
    return status;
}
