/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <PalDefs.h>
#include <aidl/vendor/qti/hardware/pal/ModifierKV.h>
#include <aidl/vendor/qti/hardware/pal/PalAudioEffect.h>
#include <aidl/vendor/qti/hardware/pal/PalBuffer.h>
#include <aidl/vendor/qti/hardware/pal/PalBufferConfig.h>
#include <aidl/vendor/qti/hardware/pal/PalDevice.h>
#include <aidl/vendor/qti/hardware/pal/PalDrainType.h>
#include <aidl/vendor/qti/hardware/pal/PalMmapBuffer.h>
#include <aidl/vendor/qti/hardware/pal/PalMmapPosition.h>
#include <aidl/vendor/qti/hardware/pal/PalParamPayload.h>
#include <aidl/vendor/qti/hardware/pal/PalSessionTime.h>
#include <aidl/vendor/qti/hardware/pal/PalStreamAttributes.h>
#include <aidl/vendor/qti/hardware/pal/PalVolumeData.h>

namespace aidl::vendor::qti::hardware::pal {

struct LegacyToAidl {
    static PalStreamAttributes convertPalStreamAttributesToAidl(
            struct pal_stream_attributes *palStreamAttributes);

    static std::vector<PalDevice> convertPalDeviceToAidl(struct pal_device *palDevice,
                                                         int noOfDevices);
    static PalMediaConfig convertPalMediaConfigToAidl(struct pal_media_config *palMediaConfig);

    static PalUsbDeviceAddress convertPalUSBDeviceAddressToAidl(
            struct pal_usb_device_address *palUSBAddress);

    static std::vector<ModifierKV> convertModifierKVToAidl(struct modifier_kv *modifierKV,
                                                           int noOfModifiers);

    static PalDrainType convertPalDrainTypeToAidl(pal_drain_type_t palDrainType);

    static PalBufferConfig convertPalBufferConfigToAidl(struct pal_buffer_config *palBufferConfig);

    static PalBuffer convertPalBufferToAidl(struct pal_buffer *palBuffer);

    static PalParamPayload convertPalParamPayloadToAidl(pal_param_payload *palParamPayload);

    static PalAudioEffect convertPalAudioEffectToAidl(pal_audio_effect_t effect);

    static PalMmapBuffer convertPalMmapBufferToAidl(struct pal_mmap_buffer *palMmapBuffer);

    static PalMmapPosition convertPalMmapPositionToAidl(struct pal_mmap_position *palMmapPosition);

    static PalVolumeData convertPalVolDataToAidl(pal_volume_data *palVolData);

    static PalSessionTime convertPalSessionTimeToAidl(struct pal_session_time *palSessTime);

    static std::vector<uint8_t> convertRawPalParamPayloadToVector(void *payload, size_t size);
};
}
