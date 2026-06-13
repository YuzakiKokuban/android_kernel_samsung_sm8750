/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define NDEBUG 0
#define LOG_TAG "AgmIpc::Client"

#include <agm/agm_api.h>
#include <aidl/vendor/qti/hardware/agm/IAGM.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <log/log.h>

#include <agm/AgmAidlToLegacy.h>
#include <agm/AgmLegacyToAidl.h>
#include <agm/BinderStatus.h>
#include <aidl/vendor/qti/hardware/agm/BnAGMCallback.h>
#include <aidlcommonsupport/NativeHandle.h>
#include "AgmCallback.h"

using ::aidl::vendor::qti::hardware::agm::IAGM;
using ::aidl::vendor::qti::hardware::agm::IAGMCallback;
using ::aidl::vendor::qti::hardware::agm::AidlToLegacy;
using ::aidl::vendor::qti::hardware::agm::AgmSessionMode;
using ::aidl::vendor::qti::hardware::agm::AifInfo;
using ::aidl::vendor::qti::hardware::agm::AgmCallback;
using ::aidl::vendor::qti::hardware::agm::MmapBufInfo;

static std::shared_ptr<IAGM> gAgmClient = nullptr;
static ::ndk::ScopedAIBinder_DeathRecipient gDeathRecipient;
std::mutex gLock;

#define RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client)            \
    ({                                                         \
        if (client.get() == nullptr) {                         \
            ALOGE(" %s Agm service doesn't exist ", __func__); \
            return -EINVAL;                                    \
        }                                                      \
    })

void serviceDied(void *cookie) {
    ALOGE("%s : AGM Service died ,cookie : %llu", __func__, (unsigned long long)cookie);
    std::lock_guard<std::mutex> guard(gLock);
    gAgmClient = nullptr;
}

std::shared_ptr<IAGM> getAgm() {
    std::lock_guard<std::mutex> guard(gLock);
    if (gAgmClient == nullptr) {
        const std::string instance = std::string() + IAGM::descriptor + "/default";
        ABinderProcess_startThreadPool();
        auto binder = ::ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str()));
        ALOGV("%s got binder %p", __func__, binder.get());

        auto newClient = IAGM::fromBinder(binder);

        if (newClient == nullptr) {
            ALOGE("could not get agmclient fromBinder");
            return nullptr;
        }
        gAgmClient = newClient;
        ALOGI("%s gAgmClient %p ", __func__, gAgmClient.get());

        gDeathRecipient =
                ::ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        auto status = ::ndk::ScopedAStatus::fromStatus(
                AIBinder_linkToDeath(binder.get(), gDeathRecipient.get(), (void *)serviceDied));

        if (!status.isOk()) {
            ALOGV("linking service to death failed: %d: %s", status.getStatus(),
                  status.getMessage());
        } else {
            ALOGI("linked to death %d: %s", status.getStatus(), status.getMessage());
        }
    }
    ALOGV("%s gAgmClient %p ", __func__, gAgmClient.get());
    return gAgmClient;
}

int agm_register_service_crash_callback(agm_service_crash_cb cb, uint64_t cookie) {
    return 0;
}

int agm_aif_set_media_config(uint32_t audio_intf, struct agm_media_config *media_config) {
    ALOGV("%s audio_intf = %d", __func__, audio_intf);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlConfig = LegacyToAidl::convertAgmMediaConfigToAidl(media_config);
    return statusTFromBinderStatus(client->ipc_agm_aif_set_media_config(audio_intf, aidlConfig));
}

int agm_session_set_config(uint64_t handle, struct agm_session_config *session_config,
                           struct agm_media_config *media_config,
                           struct agm_buffer_config *buffer_config) {
    ALOGV("%s handle = %llx ", __func__, (unsigned long long)handle);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlSessionConfig =
            LegacyToAidl::convertAgmSessionConfigToAidl(session_config, media_config->format);
    auto aidlMediaConfig = LegacyToAidl::convertAgmMediaConfigToAidl(media_config);
    auto aidlBufferConfig = LegacyToAidl::convertAgmBufferConfigToAidl(buffer_config);

    return statusTFromBinderStatus(client->ipc_agm_session_set_config(
            handle, aidlSessionConfig, aidlMediaConfig, aidlBufferConfig));
}

int agm_init() {
    return 0;
}

int agm_deinit() {
    return 0;
}

int agm_aif_set_metadata(uint32_t audio_intf, uint32_t size, uint8_t *metadata) {
    ALOGV("%s audio_intf = %d, size =%d ", __func__, audio_intf, size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlMetadata = LegacyToAidl::convertRawPayloadToVector(metadata, size);

    return statusTFromBinderStatus(client->ipc_agm_aif_set_metadata(audio_intf, aidlMetadata));
}

int agm_session_set_metadata(uint32_t session_id, uint32_t size, uint8_t *metadata) {
    ALOGV("%s session_id = %d, size = %d", __func__, session_id, size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlMetadata = LegacyToAidl::convertRawPayloadToVector(metadata, size);

    return statusTFromBinderStatus(client->ipc_agm_session_set_metadata(session_id, aidlMetadata));
}

int agm_session_aif_set_metadata(uint32_t session_id, uint32_t audio_intf, uint32_t size,
                                 uint8_t *metadata) {
    ALOGV("%s  session_id = %d, aif = %d, size = %d", __func__, session_id, audio_intf, size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlMetadata = LegacyToAidl::convertRawPayloadToVector(metadata, size);

    return statusTFromBinderStatus(
            client->ipc_agm_session_aif_set_metadata(session_id, audio_intf, aidlMetadata));
}

int agm_session_close(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_close(handle));
}

int agm_session_prepare(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_prepare(handle));
}

int agm_session_start(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(client->ipc_agm_session_start(handle));
}

int agm_session_stop(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_stop(handle));
}

int agm_session_pause(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_pause(handle));
}

int agm_session_flush(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_flush(handle));
}

int agm_sessionid_flush(uint32_t session_id) {
    ALOGV("%s  session id = %d", __func__, session_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_sessionid_flush(session_id));
}

int agm_session_resume(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_resume(handle));
}

int agm_session_suspend(uint64_t handle) {
    ALOGV("%s  handle = %llx ", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(client->ipc_agm_session_suspend(handle));
}

int agm_session_open(uint32_t session_id, enum agm_session_mode sess_mode, uint64_t *handle) {
    ALOGV("%s  handle = %x , *handle = %x", __func__, handle, *handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlSessionMode = static_cast<AgmSessionMode>(sess_mode);
    int64_t aidlHandle;
    auto aidlStatus = client->ipc_agm_session_open(session_id, aidlSessionMode, &aidlHandle);

    *handle = convertAidlHandleToLegacy(aidlHandle);
    auto ret = statusTFromBinderStatus(aidlStatus);
    ALOGV("%s Received handle = %p, ret %d", __func__, (unsigned long long)*handle, ret);

    return ret;
}

int agm_session_aif_connect(uint32_t session_id, uint32_t audio_intf, bool state) {
    ALOGV("%s session_id =%d, aif = %d, state = %s", __func__, session_id, audio_intf,
          state ? "true" : "false");
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(
            client->ipc_agm_session_aif_connect(session_id, audio_intf, state));
}

int agm_session_read(uint64_t handle, void *buf, size_t *byte_count) {
    ALOGV("%s  handle = %llx", __func__, (unsigned long long)handle);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    int32_t bytesToRead = (int32_t)(*byte_count);
    std::vector<uint8_t> aidlReturn;

    auto status = client->ipc_agm_session_read(handle, bytesToRead, &aidlReturn);

    if (status.isOk()) {
        memcpy(buf, aidlReturn.data(), aidlReturn.size());
        *byte_count = aidlReturn.size();
    }
    return statusTFromBinderStatus(status);
}

int agm_session_write(uint64_t handle, void *buf, size_t *byte_count) {
    ALOGV("%s  handle = %llx, bytes %d ", __func__, (unsigned long long)handle, *byte_count);

    auto client = getAgm();

    auto aidlBuffer = LegacyToAidl::convertRawPayloadToVector(buf, *byte_count);

    int32_t bytesWritten = 0;
    auto ret = statusTFromBinderStatus(
            client->ipc_agm_session_write(handle, aidlBuffer, &bytesWritten), __func__);

    if (ret != -ENOMEM) {
        *byte_count = (size_t)bytesWritten;
    }

    return ret;
}

int agm_session_set_loopback(uint32_t capture_session_id, uint32_t playback_session_id,
                             bool state) {
    ALOGV("%s called capture_session_id = %d, playback_session_id = %d", __func__,
          capture_session_id, playback_session_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(
            client->ipc_agm_session_set_loopback(capture_session_id, playback_session_id, state));
}

size_t agm_get_hw_processed_buff_cnt(uint64_t handle, enum direction dir) {
    ALOGV("%s  handle = %llx", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlDirection = static_cast<Direction>(dir);
    return statusTFromBinderStatus(
            client->ipc_agm_get_hw_processed_buff_cnt(handle, aidlDirection));
}

int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info) {
    ALOGV("%s: Enter: noOfAif %d, aifListEmpty %d", __func__, *num_aif_info, (aif_list == NULL));
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    std::vector<AifInfo> aidlAifList;
    auto aidlStatus = client->ipc_agm_get_aif_info_list((int32_t)(*num_aif_info), &aidlAifList);

    if (aif_list != NULL) {
        AidlToLegacy::convertAifInfoList(aidlAifList, aif_list);
    }

    *num_aif_info = (size_t)aidlAifList.size();
    auto ret = statusTFromBinderStatus(aidlStatus);
    ALOGV("%s: Exit size %d ret %d ", __func__, *num_aif_info);
    return ret;
}

int agm_session_aif_get_tag_module_info(uint32_t session_id, uint32_t aif_id, void *payload,
                                        size_t *size) {
    ALOGV("%s session_id =%d, aif_id = %d", __func__, session_id, aif_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    uint32_t aidlListSize = *size; // TODO cast
    std::vector<uint8_t> aidlModuleInfoList;
    auto status = client->ipc_agm_session_aif_get_tag_module_info(session_id, aif_id, aidlListSize,
                                                                  &aidlModuleInfoList);
    if (status.isOk()) {
        if (payload != NULL) memcpy(payload, aidlModuleInfoList.data(), aidlModuleInfoList.size());
        *size = aidlModuleInfoList.size();
    }
    auto ret = statusTFromBinderStatus(status);
    ALOGV("%s session_id =%d, aif_id = %d ret %d, size %d ", __func__, session_id, aif_id, ret,
          *size);
    return ret;
}

int agm_session_get_params(uint32_t session_id, void *payload, size_t size) {
    ALOGV("%s  sessionId %d  size %d ", __func__, session_id, size);
    auto client = getAgm();

    if (size <= 0) {
        ALOGE("%s  sessionId %d : Invalid input size %d ", __func__, session_id, size);
        return -EINVAL;
    }

    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, size);

    std::vector<uint8_t> aidlReturn;
    auto status = client->ipc_agm_session_get_params(session_id, aidlPayload, &aidlReturn);

    if (aidlReturn.empty()) {
        ALOGE("%s  sessionId %d : Invalid input size %d ", __func__, session_id, size);
        return -ENOMEM;
    }

    if (status.isOk() && payload) {
        memcpy(payload, aidlReturn.data(), size);
    }

    return statusTFromBinderStatus(status);
}

int agm_aif_set_params(uint32_t aif_id, void *payload, size_t size) {
    ALOGV("%s  aif_id = %d", __func__, aif_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, size);

    return statusTFromBinderStatus(client->ipc_agm_aif_set_params(aif_id, aidlPayload));
}

int agm_session_aif_set_params(uint32_t session_id, uint32_t aif_id, void *payload, size_t size) {
    ALOGV("%s session_id =%d, aif_id = %d", __func__, session_id, aif_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, size);

    return statusTFromBinderStatus(
            client->ipc_agm_session_aif_set_params(session_id, aif_id, aidlPayload));
}

int agm_session_set_params(uint32_t session_id, void *payload, size_t size) {
    ALOGV("%s session_id =%d, size = %zu", __func__, session_id, size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, size);

    return statusTFromBinderStatus(client->ipc_agm_session_set_params(session_id, aidlPayload));
}

int agm_set_params_with_tag(uint32_t session_id, uint32_t aif_id,
                            struct agm_tag_config *tag_config) {
    ALOGV("%s session_id =%d, aif_id = %d", __func__, session_id, aif_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlTagConfig = LegacyToAidl::convertAgmTagConfigToAidl(tag_config);

    return statusTFromBinderStatus(
            client->ipc_agm_set_params_with_tag(session_id, aif_id, aidlTagConfig));
}

int agm_set_params_with_tag_to_acdb(uint32_t session_id, uint32_t aif_id, void *payload,
                                    size_t size) {
    ALOGV("%s session_id =%d, aif_id = %d", __func__, session_id, aif_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);
    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, size);
    return statusTFromBinderStatus(
            client->ipc_agm_set_params_with_tag_to_acdb(session_id, aif_id, aidlPayload));
}

int agm_set_params_to_acdb_tunnel(void *payload, size_t size) {
    ALOGV("%s size = %d", __func__, size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, size);

    return statusTFromBinderStatus(client->ipc_agm_set_params_to_acdb_tunnel(aidlPayload));
}

int agm_get_params_from_acdb_tunnel(void *payload, size_t *size) {
    ALOGV("%s size = %d", __func__, size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlPayload = LegacyToAidl::convertRawPayloadToVector(payload, *size);

    std::vector<uint8_t> aidlReturn;
    auto status = client->ipc_agm_get_params_from_acdb_tunnel(aidlPayload, &aidlReturn);

    if (status.isOk() && payload != NULL) {
        memcpy(payload, aidlReturn.data(), aidlReturn.size());
        *size = aidlReturn.size();
    }

    return statusTFromBinderStatus(status);
}

int agm_session_register_for_events(uint32_t session_id, struct agm_event_reg_cfg *evt_reg_cfg) {
    ALOGV("%s session_id =%d size %d ", __func__, session_id,
          evt_reg_cfg->event_config_payload_size);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlAgmEventRegConfig = LegacyToAidl::convertAgmEventRegistrationConfigToAidl(evt_reg_cfg);

    auto status = client->ipc_agm_session_register_for_events(session_id, aidlAgmEventRegConfig);
    return statusTFromBinderStatus(status);
}

int agm_session_aif_set_cal(uint32_t session_id, uint32_t aif_id,
                            struct agm_cal_config *cal_config) {
    ALOGV("%s session_id =%d, aif_id = %d", __func__, session_id, aif_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlCalConfig = LegacyToAidl::convertAgmCalConfigToAidl(cal_config);
    return statusTFromBinderStatus(
            client->ipc_agm_session_aif_set_cal(session_id, aif_id, aidlCalConfig));
}

int agm_session_set_ec_ref(uint32_t capture_session_id, uint32_t aif_id, bool state) {
    ALOGV("%s : capture_session_id = %d, aif_id = %d ", __func__, capture_session_id, aif_id);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    return statusTFromBinderStatus(
            client->ipc_agm_session_set_ec_ref(capture_session_id, aif_id, state));
}

int agm_session_register_cb(uint32_t session_id, agm_event_cb cb, enum event_type evt_type,
                            void *client_data) {
    ALOGV("%s session_id =%d, evt_type = %d, client_data = %p , register %d ", __func__, session_id,
          evt_type, client_data, (cb != NULL));

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    std::shared_ptr<IAGMCallback> aidlAgmCallback;
    if (cb != NULL) {
        aidlAgmCallback =
                ::ndk::SharedRefBase::make<AgmCallback>(session_id, cb, evt_type, client_data);
        ALOGV("%s, creating new callback %p agm ptr %p ", __func__, aidlAgmCallback.get(), cb);
    }

    int64_t clientDataAidl = reinterpret_cast<int64_t>(client_data);
    auto status = client->ipc_agm_session_register_callback(aidlAgmCallback, session_id, evt_type,
                                                            (cb != NULL), clientDataAidl);
    return statusTFromBinderStatus(status);
}

int agm_session_eos(uint64_t handle) {
    ALOGV("%s  handle = %llx", __func__, (unsigned long long)handle);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);
    return statusTFromBinderStatus(client->ipc_agm_session_eos(handle));
}

int agm_get_session_time(uint64_t handle, uint64_t *timestamp) {
    ALOGV("%s  handle = %llx", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    int64_t aidlTimestamp;
    auto status = client->ipc_agm_get_session_time(handle, &aidlTimestamp);

    *timestamp = aidlTimestamp;
    return statusTFromBinderStatus(status);
}

int agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp) {
    ALOGV("%s: session_id = %d", __func__, session_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    int64_t aidlTimestamp;
    auto status = client->ipc_agm_get_buffer_timestamp(session_id, &aidlTimestamp);
    *timestamp = aidlTimestamp;
    return statusTFromBinderStatus(status);
}

int agm_session_get_buf_info(uint32_t session_id, struct agm_buf_info *buf_info, uint32_t flag) {
    ALOGV("%s: session_id = %d flag %d", __func__, session_id, flag);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    MmapBufInfo aidlMmapBufRet;
    auto status = client->ipc_agm_session_get_buf_info(session_id, flag, &aidlMmapBufRet);
    if (status.isOk()) {
        AidlToLegacy::convertMmapBufInfo(aidlMmapBufRet, buf_info, flag);
    }
    return statusTFromBinderStatus(status);
}

int agm_set_gapless_session_metadata(uint64_t handle, enum agm_gapless_silence_type type,
                                     uint32_t silence) {
    ALOGV("%s  handle = %x", __func__, handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    AgmGaplessSilenceType aidlSilenceType = static_cast<AgmGaplessSilenceType>(type);

    return statusTFromBinderStatus(
            client->ipc_agm_set_gapless_session_metadata(handle, aidlSilenceType, silence));
}

int agm_session_set_non_tunnel_mode_config(uint64_t handle,
                                           struct agm_session_config *session_config,
                                           struct agm_media_config *in_media_config,
                                           struct agm_media_config *out_media_config,
                                           struct agm_buffer_config *in_buffer_config,
                                           struct agm_buffer_config *out_buffer_config) {
    ALOGV("%s  handle = %llx", __func__, (unsigned long long)handle);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlSessionConfig =
            LegacyToAidl::convertAgmSessionConfigToAidl(session_config, in_media_config->format);
    auto aidlInMediaConfig = LegacyToAidl::convertAgmMediaConfigToAidl(in_media_config);
    auto aidlOutMediaConfig = LegacyToAidl::convertAgmMediaConfigToAidl(out_media_config);
    auto aidlInBufferConfig = LegacyToAidl::convertAgmBufferConfigToAidl(in_buffer_config);
    auto aidlOutBufferConfig = LegacyToAidl::convertAgmBufferConfigToAidl(out_buffer_config);

    return statusTFromBinderStatus(client->ipc_agm_session_set_non_tunnel_mode_config(
            handle, aidlSessionConfig, aidlInMediaConfig, aidlOutMediaConfig, aidlInBufferConfig,
            aidlOutBufferConfig));
}

int agm_session_write_with_metadata(uint64_t handle, struct agm_buff *buf, size_t *consumed_size) {
    ALOGV("%s  handle = %x", __func__, handle);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlBuffer = LegacyToAidl::convertAgmBufferToAidl(buf, true, false);
    int32_t written = 0;
    auto status = client->ipc_agm_session_write_with_metadata(handle, aidlBuffer, &written);
    if (status.isOk()) {
        *consumed_size = written;
    }
    return statusTFromBinderStatus(status);
}

int agm_session_read_with_metadata(uint64_t handle, struct agm_buff *buf, uint32_t *captured_size) {
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlBuffer = LegacyToAidl::convertAgmBufferToAidl(buf, true, false /*copyBuffer*/);

    IAGM::AgmReadWithMetadataReturn aidlReturn;
    // TODO change the API, on read return don't need AgmBuffer.
    auto status = client->ipc_agm_session_read_with_metadata(handle, aidlBuffer, *captured_size,
                                                             &aidlReturn);

    return statusTFromBinderStatus(status);
}

int agm_aif_group_set_media_config(uint32_t group_id, struct agm_group_media_config *media_config) {
    ALOGV("%s called, group_id = %d ", __func__, group_id);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlConfig = LegacyToAidl::convertAgmGroupMediaConfigToAidl(media_config);

    return statusTFromBinderStatus(
            client->ipc_agm_aif_group_set_media_config(group_id, aidlConfig));
}

int agm_get_group_aif_info_list(struct aif_info *aif_list, size_t *num_groups) {
    ALOGV("%s called ", __func__);
    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    std::vector<AifInfo> aidlAifList;
    auto aidlStatus = client->ipc_agm_get_group_aif_info_list((int32_t)(*num_groups), &aidlAifList);

    if (aif_list != NULL) {
        AidlToLegacy::convertAifInfoList(aidlAifList, aif_list);
    }

    *num_groups = (size_t)aidlAifList.size();
    ALOGV("%s: Exit size %d  ", __func__, *num_groups);
    return statusTFromBinderStatus(aidlStatus);
}

int agm_session_write_datapath_params(uint32_t session_id, struct agm_buff *buf) {
    ALOGV("%s  session id = %d ", __func__, session_id);

    auto client = getAgm();
    RETURN_IF_AGM_SERVICE_NOT_REGISTERED(client);

    auto aidlBuffer = LegacyToAidl::convertAgmBufferToAidl(buf, false, true /*copyBuffer*/);

    return statusTFromBinderStatus(
            client->ipc_agm_session_write_datapath_params(session_id, aidlBuffer));
}

int agm_dump(struct agm_dump_info *dump_info) {
    return 0;
}
