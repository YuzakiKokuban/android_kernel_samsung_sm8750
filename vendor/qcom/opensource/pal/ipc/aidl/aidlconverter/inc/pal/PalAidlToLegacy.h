/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalDefs.h>
#include <aidl/vendor/qti/hardware/pal/ModifierKV.h>
#include <aidl/vendor/qti/hardware/pal/PalBuffer.h>
#include <aidl/vendor/qti/hardware/pal/PalBufferConfig.h>
#include <aidl/vendor/qti/hardware/pal/PalCallbackBuffer.h>
#include <aidl/vendor/qti/hardware/pal/PalDevice.h>
#include <aidl/vendor/qti/hardware/pal/PalDrainType.h>
#include <aidl/vendor/qti/hardware/pal/PalParamPayload.h>
#include <aidl/vendor/qti/hardware/pal/PalSessionTime.h>
#include <aidl/vendor/qti/hardware/pal/PalStreamAttributes.h>
#include <aidl/vendor/qti/hardware/pal/PalVolumeData.h>

namespace aidl::vendor::qti::hardware::pal {

struct AidlToLegacy {
    static void convertPalStreamAttributes(const PalStreamAttributes &aidlConfig,
                                           struct pal_stream_attributes *palStreamAttributes);

    static void convertPalDevice(const std::vector<PalDevice> &aidlConfig,
                                 struct pal_device *palDevice);

    static void convertPalMediaConfig(const PalMediaConfig &aidlMediaConfig,
                                      struct pal_media_config *palMediaConfig);

    static void convertPalUSBDeviceAddress(const PalUsbDeviceAddress aidlAddress,
                                           struct pal_usb_device_address *palDeviceAddress);

    static void convertModifierKV(const std::vector<ModifierKV> &aidlConfig,
                                  struct modifier_kv *modifierKV);

    static void convertPalVolumeData(const PalVolumeData &aidlConfig,
                                     pal_volume_data *palVolumeData);

    static std::pair<int, int> getFdIntFromNativeHandle(
            const aidl::android::hardware::common::NativeHandle &nativeHandle, bool doDup = true);

    static void convertPalCallbackBuffer(const PalCallbackBuffer *rwDonePayload,
                                         pal_callback_buffer *cbBuffer);

    static void convertPalSessionTime(const PalSessionTime &aildSessTime,
                                      struct pal_session_time *palSessTime);
};
}
