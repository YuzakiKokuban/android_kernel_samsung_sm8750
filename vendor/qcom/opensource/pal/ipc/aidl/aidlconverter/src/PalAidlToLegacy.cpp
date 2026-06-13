/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalIpc::AidlToLegacy::Converter"

#include <PalDefs.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <log/log.h>
#include <pal/PalAidlToLegacy.h>
#include <pal/Utils.h>

namespace aidl::vendor::qti::hardware::pal {

void AidlToLegacy::convertPalMediaConfig(const PalMediaConfig &aidlMediaConfig,
                                         struct pal_media_config *palMediaConfig) {
    palMediaConfig->sample_rate = aidlMediaConfig.sampleRate;
    palMediaConfig->bit_width = aidlMediaConfig.bitwidth;
    palMediaConfig->aud_fmt_id = (pal_audio_fmt_t)aidlMediaConfig.audioFormatId;
    palMediaConfig->ch_info.channels = aidlMediaConfig.chInfo.channels;
    memcpy(&palMediaConfig->ch_info.ch_map, &aidlMediaConfig.chInfo.chMap, sizeof(uint8_t[64]));
}

void AidlToLegacy::convertPalUSBDeviceAddress(const PalUsbDeviceAddress aidlAddress,
                                              struct pal_usb_device_address *palDeviceAddress) {
    palDeviceAddress->card_id = aidlAddress.cardId;
    palDeviceAddress->device_num = aidlAddress.deviceNum;
}

void AidlToLegacy::convertPalStreamAttributes(const PalStreamAttributes &aidlConfig,
                                              struct pal_stream_attributes *palStreamAttributes) {
    // Stream Info
    palStreamAttributes->type = (pal_stream_type_t)aidlConfig.type;
    palStreamAttributes->info.opt_stream_info.version = aidlConfig.info.version;
    palStreamAttributes->info.opt_stream_info.size = aidlConfig.info.size;
    palStreamAttributes->info.opt_stream_info.duration_us = aidlConfig.info.durationUs;
    palStreamAttributes->info.opt_stream_info.rx_proxy_type = aidlConfig.info.rxProxyType;
    palStreamAttributes->info.opt_stream_info.tx_proxy_type = aidlConfig.info.txProxyType;
    palStreamAttributes->info.opt_stream_info.has_video = aidlConfig.info.hasVideo;
    palStreamAttributes->info.opt_stream_info.is_streaming = aidlConfig.info.isStreaming;
    palStreamAttributes->info.opt_stream_info.loopback_type = aidlConfig.info.loopbackType;
    palStreamAttributes->info.opt_stream_info.haptics_type = aidlConfig.info.hapticsType;
    palStreamAttributes->flags = (pal_stream_flags_t)aidlConfig.flags;
    palStreamAttributes->direction = (pal_stream_direction_t)aidlConfig.direction;

    // In Media Config
    convertPalMediaConfig(aidlConfig.inMediaConfig, &(palStreamAttributes->in_media_config));
    // Out Media Config
    convertPalMediaConfig(aidlConfig.outMediaConfig, &(palStreamAttributes->out_media_config));
}

void AidlToLegacy::convertPalDevice(const std::vector<PalDevice> &aidlDevConfig,
                                    struct pal_device *palDevice) {
    for (unsigned long i = 0; i < aidlDevConfig.size(); i++) {
        palDevice[i].id = (pal_device_id_t)aidlDevConfig[i].id;
        // Media Config
        convertPalMediaConfig(aidlDevConfig[i].config, &(palDevice[i].config));
        // USB Device Address
        convertPalUSBDeviceAddress(aidlDevConfig[i].address, &(palDevice[i].address));

        strlcpy(palDevice[i].sndDevName, aidlDevConfig[i].sndDevName.c_str(), DEVICE_NAME_MAX_SIZE);
        strlcpy(palDevice[i].custom_config.custom_key,
                aidlDevConfig[i].customConfig.customKey.c_str(), PAL_MAX_CUSTOM_KEY_SIZE);
    }
}

void AidlToLegacy::convertModifierKV(const std::vector<ModifierKV> &aidlKv,
                                     struct modifier_kv *modifierKv) {
    for (unsigned long i = 0; i < aidlKv.size(); i++) {
        modifierKv[i].key = aidlKv[i].key;
        modifierKv[i].value = aidlKv[i].value;
    }
}

void AidlToLegacy::convertPalVolumeData(const PalVolumeData &aidlVolConfig,
                                        pal_volume_data *palVolumeData) {
    palVolumeData->no_of_volpair = aidlVolConfig.volPair.size();
    for (unsigned long i = 0; i < aidlVolConfig.volPair.size(); i++) {
        palVolumeData->volume_pair[i].channel_mask = aidlVolConfig.volPair[i].chMask;
        palVolumeData->volume_pair[i].vol = aidlVolConfig.volPair[i].vol;
    }
}

std::pair<int, int> AidlToLegacy::getFdIntFromNativeHandle(
        const aidl::android::hardware::common::NativeHandle &nativeHandle, bool doDup) {
    std::pair<int, int> fdIntPair = {-1, -1};
    if (!nativeHandle.fds.empty()) {
        if (doDup) {
            fdIntPair.first = dup(nativeHandle.fds.at(0).get());
        } else {
            fdIntPair.first = (nativeHandle.fds.at(0).get());
        }
    }
    if (!nativeHandle.ints.empty()) {
        fdIntPair.second = nativeHandle.ints.at(0);
    }

    return std::move(fdIntPair);
}

void AidlToLegacy::convertPalCallbackBuffer(const PalCallbackBuffer *rwDonePayload,
                                            pal_callback_buffer *cbBuffer) {
    if (cbBuffer->size > 0 && rwDonePayload->buffer.size() == cbBuffer->size)
        memcpy(cbBuffer->buffer, rwDonePayload->buffer.data(), cbBuffer->size);
    cbBuffer->ts->tv_sec = rwDonePayload->timeStamp.tvSec;
    cbBuffer->ts->tv_nsec = rwDonePayload->timeStamp.tvNSec;
    cbBuffer->status = rwDonePayload->status;
    cbBuffer->cb_buf_info.frame_index = rwDonePayload->cbBufInfo.frameIndex;
    cbBuffer->cb_buf_info.sample_rate = rwDonePayload->cbBufInfo.sampleRate;
    cbBuffer->cb_buf_info.bit_width = rwDonePayload->cbBufInfo.bitwidth;
    cbBuffer->cb_buf_info.channel_count = rwDonePayload->cbBufInfo.channelCount;
}

void AidlToLegacy::convertPalSessionTime(const PalSessionTime &aildSessTime,
                                         struct pal_session_time *palSessTime) {
    palSessTime->session_time.value_lsw = aildSessTime.sessionTime.valLsw;
    palSessTime->session_time.value_msw = aildSessTime.sessionTime.valMsw;
    palSessTime->absolute_time.value_lsw = aildSessTime.absoluteTime.valLsw;
    palSessTime->absolute_time.value_msw = aildSessTime.absoluteTime.valMsw;
    palSessTime->timestamp.value_lsw = aildSessTime.timestamp.valLsw;
    palSessTime->timestamp.value_msw = aildSessTime.timestamp.valMsw;
}
}
