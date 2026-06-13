/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalIpc::LegacyToAidl::Converter"

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
#include <aidl/vendor/qti/hardware/pal/PalStreamAttributes.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <log/log.h>
#include <pal/PalLegacyToAidl.h>
#include <pal/Utils.h>

namespace aidl::vendor::qti::hardware::pal {

PalMediaConfig LegacyToAidl::convertPalMediaConfigToAidl(struct pal_media_config *palMediaConfig) {
    PalMediaConfig aidlMediaConfig;
    aidlMediaConfig.sampleRate = static_cast<int>(palMediaConfig->sample_rate);
    aidlMediaConfig.bitwidth = palMediaConfig->bit_width;
    aidlMediaConfig.chInfo.channels = palMediaConfig->ch_info.channels;
    memcpy(aidlMediaConfig.chInfo.chMap.data(), palMediaConfig->ch_info.ch_map,
           PAL_MAX_CHANNELS_SUPPORTED);
    aidlMediaConfig.audioFormatId = static_cast<PalAudioFmt>(palMediaConfig->aud_fmt_id);
    return std::move(aidlMediaConfig);
}

PalUsbDeviceAddress LegacyToAidl::convertPalUSBDeviceAddressToAidl(
        struct pal_usb_device_address *palUSBAddress) {
    PalUsbDeviceAddress aidlAddress;
    aidlAddress.cardId = palUSBAddress->card_id;
    aidlAddress.deviceNum = palUSBAddress->device_num;
    return std::move(aidlAddress);
}

PalStreamAttributes LegacyToAidl::convertPalStreamAttributesToAidl(
        struct pal_stream_attributes *palStreamAttr) {
    if (palStreamAttr == nullptr) {
        return {};
    }

    PalStreamAttributes aidlStreamAttr;
    PalStreamInfo aidlStreamInfo;
    pal_stream_info palStreamInfo;

    aidlStreamAttr.type = static_cast<PalStreamType>(palStreamAttr->type);

    aidlStreamAttr.type = static_cast<PalStreamType>(palStreamAttr->type);

    ALOGD("%s: %d channels[in %d : out %d] format[in %d : out %d] flags %d", __func__, __LINE__,
          palStreamAttr->in_media_config.ch_info.channels,
          palStreamAttr->out_media_config.ch_info.channels,
          palStreamAttr->in_media_config.aud_fmt_id, palStreamAttr->out_media_config.aud_fmt_id,
          palStreamAttr->flags);

    // AIDL Stream Info
    palStreamInfo = palStreamAttr->info.opt_stream_info;
    aidlStreamInfo.version = static_cast<long>(palStreamInfo.version);
    aidlStreamInfo.size = static_cast<long>(palStreamInfo.size);
    aidlStreamInfo.durationUs = static_cast<long>(palStreamInfo.duration_us);
    aidlStreamInfo.hasVideo = palStreamInfo.has_video;
    aidlStreamInfo.rxProxyType = palStreamInfo.rx_proxy_type;
    aidlStreamInfo.txProxyType = palStreamInfo.tx_proxy_type;
    aidlStreamInfo.isStreaming = palStreamInfo.is_streaming;
    aidlStreamInfo.loopbackType = palStreamInfo.loopback_type;
    aidlStreamInfo.hapticsType = palStreamInfo.haptics_type;
    aidlStreamAttr.info = aidlStreamInfo;

    aidlStreamAttr.flags = static_cast<PalStreamFlag>(palStreamAttr->flags);

    aidlStreamAttr.direction = static_cast<PalStreamDirection>(palStreamAttr->direction);

    // InMediaConfig
    aidlStreamAttr.inMediaConfig = convertPalMediaConfigToAidl(&(palStreamAttr->in_media_config));

    // OutMediaConfig
    aidlStreamAttr.outMediaConfig = convertPalMediaConfigToAidl(&(palStreamAttr->out_media_config));

    ALOGV("%s config %s", __func__, aidlStreamAttr.toString().c_str());
    return std::move(aidlStreamAttr);
}

std::vector<PalDevice> LegacyToAidl::convertPalDeviceToAidl(struct pal_device *palDevice,
                                                            int noOfDevices) {
    if (palDevice == nullptr) {
        return {};
    }

    std::vector<PalDevice> aidlPalDevice;
    aidlPalDevice.resize(noOfDevices);

    for (auto i = 0; i < noOfDevices; i++) {
        aidlPalDevice[i].id = static_cast<PalDeviceId>(palDevice[i].id);

        // AIDL Media Config
        aidlPalDevice[i].config = convertPalMediaConfigToAidl(&(palDevice[i].config));

        // AIDL address
        aidlPalDevice[i].address = convertPalUSBDeviceAddressToAidl(&(palDevice[i].address));

        aidlPalDevice[i].sndDevName.resize(DEVICE_NAME_MAX_SIZE);
        strlcpy(aidlPalDevice[i].sndDevName.data(), palDevice[i].sndDevName, DEVICE_NAME_MAX_SIZE);

        aidlPalDevice[i].customConfig.customKey.resize(PAL_MAX_CUSTOM_KEY_SIZE);
        strlcpy(aidlPalDevice[i].customConfig.customKey.data(),
                palDevice[i].custom_config.custom_key, PAL_MAX_CUSTOM_KEY_SIZE);
    }

    return std::move(aidlPalDevice);
}

std::vector<ModifierKV> LegacyToAidl::convertModifierKVToAidl(struct modifier_kv *modifierKv,
                                                              int noOfModifiers) {
    if (modifierKv == nullptr) {
        return {};
    }

    std::vector<ModifierKV> aidlKvVec;
    aidlKvVec.resize(noOfModifiers);

    for (auto i = 0; i < noOfModifiers; i++) {
        aidlKvVec[i].key = modifierKv[i].key;
        aidlKvVec[i].value = modifierKv[i].value;
    }

    return std::move(aidlKvVec);
}

PalDrainType LegacyToAidl::convertPalDrainTypeToAidl(pal_drain_type_t palDrainType) {
    if (!palDrainType) {
        return {};
    }

    return static_cast<PalDrainType>(palDrainType);
}

PalBufferConfig LegacyToAidl::convertPalBufferConfigToAidl(
        struct pal_buffer_config *palBufferConfig) {
    PalBufferConfig aidlConfig;

    if (palBufferConfig == nullptr) {
        return {};
    }

    aidlConfig.bufCount = static_cast<int>(palBufferConfig->buf_count);
    aidlConfig.bufSize = static_cast<int>(palBufferConfig->buf_size);
    aidlConfig.maxMetadataSize = static_cast<int>(palBufferConfig->max_metadata_size);

    return std::move(aidlConfig);
}

PalParamPayload LegacyToAidl::convertPalParamPayloadToAidl(pal_param_payload *palParamPayload) {
    PalParamPayload aidlPayload;

    if (palParamPayload == nullptr) {
        return {};
    }

    aidlPayload.payload.resize(palParamPayload->payload_size);
    memcpy(aidlPayload.payload.data(), palParamPayload->payload, palParamPayload->payload_size);

    return std::move(aidlPayload);
}

aidl::android::hardware::common::NativeHandle fdToNativeHandle(int fd, int intToCopy = -1) {
    aidl::android::hardware::common::NativeHandle handle;
    handle.fds.emplace_back(dup(fd));
    if (intToCopy != -1) handle.ints.emplace_back(intToCopy);
    return std::move(handle);
}

PalBuffer LegacyToAidl::convertPalBufferToAidl(struct pal_buffer *palBuffer) {
    PalBuffer aidlBuffer;
    TimeSpec aidlTimeSpec;

    if (palBuffer == nullptr) {
        return {};
    }

    aidlBuffer.size = static_cast<int>(palBuffer->size);
    aidlBuffer.offset = static_cast<int>(palBuffer->offset);
    aidlBuffer.flags = static_cast<int>(palBuffer->flags);
    aidlBuffer.frameIndex = static_cast<long>(palBuffer->frame_index);

    // AIDL Time Stamp
    if (palBuffer->ts) {
        aidlTimeSpec.tvSec = palBuffer->ts->tv_sec;
        aidlTimeSpec.tvNSec = palBuffer->ts->tv_nsec;
        aidlBuffer.timeStamp = aidlTimeSpec;
    }

    if (palBuffer->size && palBuffer->buffer) {
        aidlBuffer.buffer.resize(palBuffer->size);
        memcpy(aidlBuffer.buffer.data(), palBuffer->buffer, palBuffer->size);
    }

    aidlBuffer.allocInfo.allocHandle = fdToNativeHandle(palBuffer->alloc_info.alloc_handle,
                                                        palBuffer->alloc_info.alloc_handle);
    aidlBuffer.allocInfo.allocSize = static_cast<int>(palBuffer->alloc_info.alloc_size);
    aidlBuffer.allocInfo.offset = palBuffer->alloc_info.offset;

    return std::move(aidlBuffer);
}

PalSessionTime LegacyToAidl::convertPalSessionTimeToAidl(struct pal_session_time *palSessTime) {
    PalSessionTime aidlSessTime;

    if (!palSessTime) {
        return {};
    }
    aidlSessTime.sessionTime.valLsw = palSessTime->session_time.value_lsw;
    aidlSessTime.sessionTime.valMsw = palSessTime->session_time.value_msw;
    aidlSessTime.absoluteTime.valLsw = palSessTime->absolute_time.value_lsw;
    aidlSessTime.absoluteTime.valMsw = palSessTime->absolute_time.value_msw;
    aidlSessTime.timestamp.valLsw = palSessTime->timestamp.value_lsw;
    aidlSessTime.timestamp.valMsw = palSessTime->timestamp.value_msw;

    return std::move(aidlSessTime);
}

PalAudioEffect LegacyToAidl::convertPalAudioEffectToAidl(pal_audio_effect_t palAudioEffect) {
    if (!palAudioEffect) {
        return {};
    }

    return static_cast<PalAudioEffect>(palAudioEffect);
}

PalMmapBuffer LegacyToAidl::convertPalMmapBufferToAidl(struct pal_mmap_buffer *palMmapBuffer) {
    PalMmapBuffer aidlBuffer;

    if (palMmapBuffer == nullptr) {
        return {};
    }

    aidlBuffer.buffer = reinterpret_cast<long>(palMmapBuffer->buffer);
    aidlBuffer.fd = palMmapBuffer->fd;
    aidlBuffer.bufferSizeFrames = palMmapBuffer->buffer_size_frames;
    aidlBuffer.burstSizeFrames = palMmapBuffer->burst_size_frames;
    aidlBuffer.flags = static_cast<PalMmapBufferFlags>(palMmapBuffer->flags);

    return aidlBuffer;
}

PalMmapPosition LegacyToAidl::convertPalMmapPositionToAidl(
        struct pal_mmap_position *palMmapPosition) {
    PalMmapPosition aidlPosition;

    if (palMmapPosition == nullptr) {
        return {};
    }

    aidlPosition.timeNanoseconds = static_cast<long>(palMmapPosition->time_nanoseconds);
    aidlPosition.positionFrames = static_cast<int>(palMmapPosition->position_frames);

    return aidlPosition;
}

PalVolumeData LegacyToAidl::convertPalVolDataToAidl(pal_volume_data *palVolData) {
    PalVolumeData aidlPalVolData;

    if (palVolData == nullptr) {
        return {};
    }

    aidlPalVolData.volPair.resize(palVolData->no_of_volpair);
    memcpy(aidlPalVolData.volPair.data(), palVolData->volume_pair,
           sizeof(PalChannelVolKv) * palVolData->no_of_volpair);

    return std::move(aidlPalVolData);
}

std::vector<uint8_t> LegacyToAidl::convertRawPalParamPayloadToVector(void *payload, size_t size) {
    if (payload == nullptr) {
        return {};
    }

    std::vector<uint8_t> aidlPayload(size, 0);
    memcpy(aidlPayload.data(), payload, size);
    return std::move(aidlPayload);
}
}
