/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PalClientWrapper"
#include <PalApi.h>
#include <PalDefs.h>
#include <aidl/vendor/qti/hardware/pal/BnPALCallback.h>
#include <aidl/vendor/qti/hardware/pal/IPAL.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>
#include <pal/BinderStatus.h>
#include <pal/PalAidlToLegacy.h>
#include <pal/PalLegacyToAidl.h>
#include <pal/SharedMemoryWrapper.h>
#include <pal/Utils.h>
#include "PalCallback.h"

using ::aidl::vendor::qti::hardware::pal::AidlToLegacy;
using ::aidl::vendor::qti::hardware::pal::IPAL;
using ::aidl::vendor::qti::hardware::pal::IPALCallback;
using ::aidl::vendor::qti::hardware::pal::LegacyToAidl;
using ::aidl::vendor::qti::hardware::pal::ModifierKV;
using ::aidl::vendor::qti::hardware::pal::PalBuffer;
using ::aidl::vendor::qti::hardware::pal::PalBufferConfig;
using ::aidl::vendor::qti::hardware::pal::PalCallback;
using ::aidl::vendor::qti::hardware::pal::PalDevice;
using ::aidl::vendor::qti::hardware::pal::PalDeviceId;
using ::aidl::vendor::qti::hardware::pal::PalMmapBuffer;
using ::aidl::vendor::qti::hardware::pal::PalMmapPosition;
using ::aidl::vendor::qti::hardware::pal::PalParamPayload;
using ::aidl::vendor::qti::hardware::pal::PalReadReturnData;
using ::aidl::vendor::qti::hardware::pal::PalSessionTime;
using ::aidl::vendor::qti::hardware::pal::PalStreamAttributes;
using ::aidl::vendor::qti::hardware::pal::PalStreamType;
using ::aidl::vendor::qti::hardware::pal::SharedMemoryWrapper;
using ::aidl::vendor::qti::hardware::pal::PalParamPayloadShmem;
using ::ndk::ScopedFileDescriptor;

static std::shared_ptr<IPAL> gPalClient = nullptr;
static bool gPalServiceDied = false;
::ndk::ScopedAIBinder_DeathRecipient gDeathRecipient;
std::mutex gLock;

#define RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client)           \
    ({                                                         \
        if (client.get() == nullptr) {                         \
            ALOGE(" %s PAL service doesn't exist ", __func__); \
            return -EINVAL;                                    \
        }                                                      \
    })

void serviceDied(void *cookie) {
    ALOGE("%s : PAL Service died, cookie : %llu", __func__, (unsigned long long)cookie);
    gPalServiceDied = true;
    _exit(1);
}

std::shared_ptr<IPAL> getPal() {
    std::lock_guard<std::mutex> guard(gLock);
    if (gPalClient == nullptr) {
        const std::string instance = std::string() + IPAL::descriptor + "/default";

        // pool of threads for handling Binder transactions.
        ABinderProcess_startThreadPool();
        auto binder = ::ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str()));
        ALOGV("got binder %p", binder.get());

        auto newClient = IPAL::fromBinder(binder);
        if (newClient == nullptr) {
            ALOGE("%s: %d, Could not get PAL client from binder.", __func__, __LINE__);
            return nullptr;
        }
        gPalClient = newClient;

        gDeathRecipient =
                ::ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        auto status = ::ndk::ScopedAStatus::fromStatus(
                AIBinder_linkToDeath(binder.get(), gDeathRecipient.get(), (void *)serviceDied));

        if (!status.isOk()) {
            ALOGE("linking service to death failed: %d: %s", status.getStatus(),
                  status.getMessage());
        } else {
            ALOGI("linked to death %d: %s", status.getStatus(), status.getMessage());
        }
    }

    ALOGV("%s gPalClient %p ", __func__, gPalClient.get());
    return gPalClient;
}

int32_t pal_init() {
    gPalClient = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(gPalClient);
    return 0;
}

void pal_deinit() {
    return;
}

int32_t pal_stream_open(struct pal_stream_attributes *attr, uint32_t no_of_devices,
                        struct pal_device *devices, uint32_t no_of_modifiers,
                        struct modifier_kv *modifiers, pal_stream_callback cb, uint64_t cookie,
                        pal_stream_handle_t **stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    int32_t ret = -EINVAL;
    std::shared_ptr<IPALCallback> ClbkBinder = ::ndk::SharedRefBase::make<PalCallback>(cb);
    struct pal_stream_info info = attr->info.opt_stream_info;

    ALOGV("%s: ver [%ld] sz [%ld] dur[%ld] has_video [%d] is_streaming [%d] lpbk_type [%d]",
          __func__, info.version, info.size, info.duration_us, info.has_video, info.is_streaming,
          info.loopback_type);

    std::vector<PalDevice> aidlDevVec;
    std::vector<ModifierKV> aidlKvVec;

    auto aidlPalStreamAttr = LegacyToAidl::convertPalStreamAttributesToAidl(attr);

    if (devices) {
        aidlDevVec = LegacyToAidl::convertPalDeviceToAidl(devices, no_of_devices);
    } else {
        ALOGE("Invalid devices");
    }
    if (modifiers) {
        aidlKvVec = LegacyToAidl::convertModifierKVToAidl(modifiers, no_of_modifiers);
    } else {
        ALOGW("Invalid Modifiers");
    }

    int64_t aidlCookie = int64_t(cookie);
    int64_t aidlReturn;

    ret = statusTFromBinderStatus(client->ipc_pal_stream_open(
            aidlPalStreamAttr, aidlDevVec, aidlKvVec, ClbkBinder, aidlCookie, &aidlReturn));
    *stream_handle = (uint64_t *)aidlReturn;
    return ret;
}

int32_t pal_stream_close(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_close(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_start(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_start(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_stop(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_stop(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_pause(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_pause(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_resume(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_resume(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_flush(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_flush(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_drain(pal_stream_handle_t *stream_handle, pal_drain_type_t type) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    auto aidlDrainType = LegacyToAidl::convertPalDrainTypeToAidl(type);
    return statusTFromBinderStatus(client->ipc_pal_stream_drain(
            convertLegacyHandleToAidlHandle(stream_handle), aidlDrainType));
}

int32_t pal_stream_suspend(pal_stream_handle_t *stream_handle) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(
            client->ipc_pal_stream_suspend(convertLegacyHandleToAidlHandle(stream_handle)));
}

int32_t pal_stream_get_buffer_size(pal_stream_handle_t *stream_handle, size_t *in_buffer,
                                   size_t *out_buffer) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return -EINVAL;
}

int32_t pal_stream_get_tags_with_module_info(pal_stream_handle_t *stream_handle, size_t *size,
                                             uint8_t *payload) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    std::vector<uint8_t> aidlPayload;

    auto status = client->ipc_pal_stream_get_tags_with_module_info(
            convertLegacyHandleToAidlHandle(stream_handle), *size, &aidlPayload);

    if (payload && (*size > 0) && (*size <= aidlPayload.size())) {
        memcpy(payload, aidlPayload.data(), *size);
    } else if (payload && (*size > aidlPayload.size())) {
        memcpy(payload, aidlPayload.data(), aidlPayload.size());
    }
    *size = aidlPayload.size();
    return statusTFromBinderStatus(status);
}

int32_t pal_stream_set_buffer_size(pal_stream_handle_t *stream_handle,
                                   pal_buffer_config_t *in_buff_cfg,
                                   pal_buffer_config_t *out_buff_cfg) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    if (in_buff_cfg) {
        ALOGV("%s:%d input incnt %d buf_sz %d max_metadata_size %d", __func__, __LINE__,
              in_buff_cfg->buf_count, in_buff_cfg->buf_size, in_buff_cfg->max_metadata_size);
    }
    if (out_buff_cfg) {
        ALOGV("%s:%d output incnt %d buf_sz %d max_metadata_size %d", __func__, __LINE__,
              out_buff_cfg->buf_count, out_buff_cfg->buf_size, out_buff_cfg->max_metadata_size);
    }

    auto aidlInBufCfg = LegacyToAidl::convertPalBufferConfigToAidl(in_buff_cfg);
    auto aidlOutBufCfg = LegacyToAidl::convertPalBufferConfigToAidl(out_buff_cfg);

    std::vector<PalBufferConfig> _aidl_return_buff_cfg;

    auto status = client->ipc_pal_stream_set_buffer_size((int64_t)stream_handle, aidlInBufCfg,
                                                         aidlOutBufCfg, &_aidl_return_buff_cfg);
    if (!_aidl_return_buff_cfg.empty()) {
        in_buff_cfg->buf_count = _aidl_return_buff_cfg[0].bufCount;
        in_buff_cfg->buf_size = _aidl_return_buff_cfg[0].bufSize;
        in_buff_cfg->max_metadata_size = _aidl_return_buff_cfg[0].maxMetadataSize;
    }
    if (_aidl_return_buff_cfg.size() == 2) {
        out_buff_cfg->buf_count = _aidl_return_buff_cfg[1].bufCount;
        out_buff_cfg->buf_size = _aidl_return_buff_cfg[1].bufSize;
        out_buff_cfg->max_metadata_size = _aidl_return_buff_cfg[1].maxMetadataSize;
    }

    return statusTFromBinderStatus(status);
}

ssize_t pal_stream_read(pal_stream_handle_t *stream_handle, struct pal_buffer *buf) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    int ret = -EINVAL;
    if (stream_handle == nullptr) {
        return ret;
    }

    std::vector<PalBuffer> aidlPalBufVec;

    auto aidlBuf = LegacyToAidl::convertPalBufferToAidl(buf);

    ALOGV("%s:%d size %d %d", __func__, __LINE__, aidlBuf.size, buf->size);
    ALOGV("%s:%d alloc handle %d sending %d", __func__, __LINE__, buf->alloc_info.alloc_handle,
          aidlBuf.allocInfo.allocSize);

    aidlPalBufVec.push_back(std::move(aidlBuf));

    PalReadReturnData _aidl_return_buf;
    auto status =
            client->ipc_pal_stream_read((int64_t)stream_handle, aidlPalBufVec, &_aidl_return_buf);
    if (_aidl_return_buf.ret > 0) {
        if (_aidl_return_buf.buffer.data()->size > buf->size) {
            ALOGE("ret buf sz %d bigger than request buf sz %d",
                  _aidl_return_buf.buffer.data()->size, buf->size);
            return -ENOMEM;
        } else {
            if (buf->ts) {
                buf->ts->tv_sec = _aidl_return_buf.buffer.data()->timeStamp.tvSec;
                buf->ts->tv_nsec = _aidl_return_buf.buffer.data()->timeStamp.tvNSec;
            }
            buf->flags = _aidl_return_buf.buffer.data()->flags;
            if (buf->buffer) {
                memcpy(buf->buffer, _aidl_return_buf.buffer.data()->buffer.data(), buf->size);
            }
        }
    }
    ret = _aidl_return_buf.ret;
    return ret;
}

ssize_t pal_stream_write(pal_stream_handle_t *stream_handle, struct pal_buffer *buf) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    if (stream_handle == nullptr) {
        return -EINVAL;
    }

    ALOGV("%s:%d hndl %p", __func__, __LINE__, stream_handle);

    int32_t aidlReturn;
    std::vector<PalBuffer> aidlPalBufVec;
    auto aidlBuf = LegacyToAidl::convertPalBufferToAidl(buf);

    ALOGV("%s:%d size %d %d", __func__, __LINE__, aidlBuf.size, buf->size);
    ALOGV("%s:%d alloc handle %d sending %d", __func__, __LINE__, buf->alloc_info.alloc_handle,
          aidlBuf.allocInfo.allocSize);

    aidlPalBufVec.push_back(std::move(aidlBuf));

    auto status = client->ipc_pal_stream_write((int64_t)stream_handle, aidlPalBufVec, &aidlReturn);
    if (aidlReturn >= 0 && status.isOk()) {
        return aidlReturn;
    } else {
        return statusTFromBinderStatus(status);
    }
}

int32_t pal_stream_get_device(pal_stream_handle_t *stream_handle, uint32_t no_of_devices,
                              struct pal_device *devices) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    ALOGD("%s:%d:", __func__, __LINE__);
    return 0;
}

int32_t pal_stream_set_device(pal_stream_handle_t *stream_handle, uint32_t no_of_devices,
                              struct pal_device *devices) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    if (devices == nullptr) {
        return -EINVAL;
    }

    auto aidlStreamHandle = convertLegacyHandleToAidlHandle(stream_handle);

    ALOGD("%s:%d:total device size %d", __func__, __LINE__, no_of_devices);

    auto aidlPalDevVec = LegacyToAidl::convertPalDeviceToAidl(devices, no_of_devices);

    return statusTFromBinderStatus(
            client->ipc_pal_stream_set_device(aidlStreamHandle, aidlPalDevVec));
}

int32_t pal_stream_get_param(pal_stream_handle_t *stream_handle, uint32_t param_id,
                             pal_param_payload **param_payload) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    ALOGV("%s:%d:", __func__, __LINE__);
    if (stream_handle == NULL || !(*param_payload)) {
        return -EINVAL;
    }
    PalParamPayload _aidl_return_paramPayload;

    auto aidlStreamHandle = convertLegacyHandleToAidlHandle(stream_handle);
    auto status = client->ipc_pal_stream_get_param(aidlStreamHandle, param_id,
                                                   &_aidl_return_paramPayload);
    if (status.isOk()) {
        *param_payload = (pal_param_payload *)calloc(
                1, sizeof(pal_param_payload) + _aidl_return_paramPayload.payload.size());
        if (*param_payload == nullptr) {
            ALOGE("%s:%d Failed to allocate memory for (*param_payload)", __func__, __LINE__);
            return -ENOMEM;
        } else {
            (*param_payload)->payload_size = _aidl_return_paramPayload.payload.size();
            memcpy((*param_payload)->payload, _aidl_return_paramPayload.payload.data(),
                   (*param_payload)->payload_size);
        }
    }
    return statusTFromBinderStatus(status);
}

int32_t pal_stream_set_param(pal_stream_handle_t *stream_handle, uint32_t param_id,
                             pal_param_payload *param_payload) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    if (stream_handle == nullptr || param_payload == nullptr) {
        return -EINVAL;
    }

    int status = 0;
    SharedMemoryWrapper memWrapper(param_payload->payload_size);
    int sharedFd = memWrapper.getFd();
    void *memPayload = memWrapper.getData();

    ALOGV("%s ptr %p", __func__, memPayload);
    if (memPayload) {
        PalParamPayloadShmem payload;
        memcpy(memPayload, param_payload->payload, param_payload->payload_size);
        payload.fd = ScopedFileDescriptor(sharedFd);
        payload.payloadSize = param_payload->payload_size;
        auto aidlStreamHandle = convertLegacyHandleToAidlHandle(stream_handle);
        status = statusTFromBinderStatus(
                client->ipc_pal_stream_set_param(aidlStreamHandle, param_id, payload));
    } else {
        ALOGE("%s:%d Failed to get shared memory", __func__, __LINE__);
        return -EINVAL;
    }
    return status;
}

int32_t pal_stream_get_volume(pal_stream_handle_t *stream_handle, struct pal_volume_data *volume) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    ALOGD("%s:%d:", __func__, __LINE__);
    return -EINVAL;
}

int32_t pal_stream_set_volume(pal_stream_handle_t *stream_handle, struct pal_volume_data *volume) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    if (stream_handle == nullptr) {
        ALOGE("Invalid stream_handle!");
        return -EINVAL;
    }
    if (volume == nullptr) {
        ALOGE("Invalid volume!");
        return -EINVAL;
    }

    auto aidlStreamHandle = convertLegacyHandleToAidlHandle(stream_handle);
    auto aidlVolData = LegacyToAidl::convertPalVolDataToAidl(volume);

    return statusTFromBinderStatus(
            client->ipc_pal_stream_set_volume(aidlStreamHandle, aidlVolData));
}

int32_t pal_stream_get_mute(pal_stream_handle_t *stream_handle, bool *state) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    ALOGD("%s:%d:", __func__, __LINE__);
    return -EINVAL;
}

int32_t pal_stream_set_mute(pal_stream_handle_t *stream, bool state) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    int64_t aidlHandle = convertLegacyHandleToAidlHandle(stream);

    return statusTFromBinderStatus(client->ipc_pal_stream_set_mute(aidlHandle, state));
}

int32_t pal_get_mic_mute(bool *state) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(client->ipc_pal_get_mic_mute(state));
}

int32_t pal_set_mic_mute(bool state) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(client->ipc_pal_set_mic_mute(state));
}

int32_t pal_get_timestamp(pal_stream_handle_t *stream_handle, struct pal_session_time *stime) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    int64_t aidlHandle = convertLegacyHandleToAidlHandle(stream_handle);

    PalSessionTime aidlSessionTime;

    auto status =
            statusTFromBinderStatus(client->ipc_pal_get_timestamp(aidlHandle, &aidlSessionTime));

    if (stime != NULL) {
        AidlToLegacy::convertPalSessionTime(aidlSessionTime, stime);
    }

    return status;
}

int32_t pal_add_remove_effect(pal_stream_handle_t *stream_handle, pal_audio_effect_t effect,
                              bool enable) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    int64_t aidlHandle = convertLegacyHandleToAidlHandle(stream_handle);

    auto aidlAudioEffect = LegacyToAidl::convertPalAudioEffectToAidl(effect);
    return statusTFromBinderStatus(
            client->ipc_pal_add_remove_effect(aidlHandle, aidlAudioEffect, enable));
}

int32_t pal_set_param(uint32_t param_id, void *param_payload, size_t payload_size) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    int32_t aidlParamId = static_cast<uint32_t>(param_id);

    std::vector<uint8_t> aidlPayload(payload_size, 0);
    memcpy(aidlPayload.data(), param_payload, payload_size);

    int32_t aidlPayloadSize = static_cast<int32_t>(payload_size);

    return statusTFromBinderStatus(client->ipc_pal_set_param(aidlParamId, aidlPayload));
}

int32_t pal_get_param(uint32_t param_id, void **param_payload, size_t *payload_size, void *query) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    int32_t aidlParamId = static_cast<uint32_t>(param_id);
    uint32_t size;
    std::vector<uint8_t> aidlPayload;

    auto status = client->ipc_pal_get_param(aidlParamId, &aidlPayload);

    size = aidlPayload.size();

    if (status.isOk() && *param_payload == NULL) {
        *param_payload = calloc(1, size);
        if (!(*param_payload)) {
            ALOGE("Failed to allocate memory for (*param_payload) %s %d", __func__, __LINE__);
            return -ENOMEM;
        } else {
            memcpy(*param_payload, aidlPayload.data(), size);
            *payload_size = size;
        }
    }
    return statusTFromBinderStatus(status);
}

int32_t pal_stream_create_mmap_buffer(pal_stream_handle_t *stream_handle, int32_t min_size_frames,
                                      struct pal_mmap_buffer *info) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    int64_t aidlHandle = convertLegacyHandleToAidlHandle(stream_handle);

    PalMmapBuffer aidlBuffer = LegacyToAidl::convertPalMmapBufferToAidl(info);

    return statusTFromBinderStatus(
            client->ipc_pal_stream_create_mmap_buffer(aidlHandle, min_size_frames, &aidlBuffer));
}

int32_t pal_stream_get_mmap_position(pal_stream_handle_t *stream_handle,
                                     struct pal_mmap_position *position) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    int64_t aidlHandle = convertLegacyHandleToAidlHandle(stream_handle);

    PalMmapPosition aidlPosition = LegacyToAidl::convertPalMmapPositionToAidl(position);

    return statusTFromBinderStatus(
            client->ipc_pal_stream_get_mmap_position(aidlHandle, &aidlPosition));
}

int32_t pal_register_global_callback(pal_global_callback cb, uint64_t cookie) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    std::shared_ptr<IPALCallback> aidlPalCallback;
    auto aidlCookie = static_cast<int64_t>(cookie);

    return statusTFromBinderStatus(
            client->ipc_pal_register_global_callback(aidlPalCallback, aidlCookie));
}

int32_t pal_gef_rw_param(uint32_t param_id, void *param_payload, size_t payload_size,
                         pal_device_id_t pal_device_id, pal_stream_type_t pal_stream_type,
                         unsigned int dir) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);

    auto aidlParamId = static_cast<int32_t>(param_id);
    std::vector<uint8_t> aidlPayload(payload_size, 0);
    auto aidlPalDeviceId = static_cast<PalDeviceId>(pal_device_id);
    auto aidlPalStreamType = static_cast<PalStreamType>(pal_stream_type);
    auto aidlDir = static_cast<int8_t>(dir);
    std::vector<uint8_t> aidlReturn;

    return statusTFromBinderStatus(client->ipc_pal_gef_rw_param(
            aidlParamId, aidlPayload, aidlPalDeviceId, aidlPalStreamType, aidlDir, &aidlReturn));
}

int32_t pal_gef_rw_param_acdb(uint32_t param_id, void *param_payload, size_t payload_size,
                              pal_device_id_t pal_device_id, pal_stream_type_t pal_stream_type,
                              uint32_t sample_rate, uint32_t instance_id, uint32_t dir,
                              bool is_play) {
    auto client = getPal();
    RETURN_IF_PAL_SERVICE_NOT_REGISTERED(client);
    auto aidlParamId = static_cast<int32_t>(param_id);
    std::vector<uint8_t> aidlPayload(payload_size, 0);
    auto aidlPayloadSize = static_cast<int32_t>(payload_size);
    auto aidlPalDeviceId = static_cast<PalDeviceId>(pal_device_id);
    auto aidlPalStreamType = static_cast<PalStreamType>(pal_stream_type);
    auto aidlSampleRate = static_cast<int32_t>(sample_rate);

    return 0;
}
