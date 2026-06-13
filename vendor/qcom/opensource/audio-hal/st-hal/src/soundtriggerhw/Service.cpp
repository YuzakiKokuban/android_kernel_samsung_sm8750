/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "STHAL: SoundTriggerHw"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <soundtriggerhw/SoundTriggerHw.h>
#include <soundtriggerhw/SoundTriggerCommon.h>
#include <log/log.h>

using aidl::android::hardware::soundtrigger3::SoundTriggerHw;

extern "C" __attribute__((visibility("default"))) binder_status_t
createISoundTriggerFactory()
{

    binder_status_t status;

    auto soundTriggerFactory = ::ndk::SharedRefBase::make<SoundTriggerHw>();
    status = soundTriggerFactory->isInitDone();
    if (!status) {
        STHAL_ERR(LOG_TAG, "SoundTriggerHw initialization failed.");
        return STATUS_INVALID_OPERATION;
    }

    const std::string stInterfaceName =
        std::string() + SoundTriggerHw::descriptor + "/default";
    status = AServiceManager_addService(
        soundTriggerFactory->asBinder().get(), stInterfaceName.c_str());

    if (status != STATUS_OK) {
        STHAL_WARN(LOG_TAG, "Could not register %s, status=%d",
            stInterfaceName.c_str(), status);
    }
    return status;
}
