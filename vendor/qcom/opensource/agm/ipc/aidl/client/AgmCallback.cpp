/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AgmIpc::ClientCallback"

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
#include <log/log.h>
#include "AgmCallback.h"

namespace aidl::vendor::qti::hardware::agm {

AgmCallback::AgmCallback(uint32_t sessionId, agm_event_cb callback, uint32_t event,
                         void *clientData)
    : mSessionId(sessionId), mCallback(callback), mEvent(event), mClientData(clientData) {
    ALOGV("%s , sessionId %d, event %d ", __func__, mSessionId, mEvent);
}

AgmCallback::~AgmCallback() {
    ALOGV("%s dest.. , sessionId %d, event %d ", __func__, mSessionId, mEvent);
}

::ndk::ScopedAStatus AgmCallback::eventCallback(
        const ::aidl::vendor::qti::hardware::agm::AgmEventCallbackParameter &in_eventParam) {
    ALOGV("%s size %d event %x, module %08x, mSession %d ", __func__,
          in_eventParam.eventPayload.size(), in_eventParam.eventId, in_eventParam.sourceModuleId,
          mSessionId);

    int payloadSize = in_eventParam.eventPayload.size();
    struct agm_event_cb_params *agmEventCallbackParam = NULL;
    agmEventCallbackParam = (struct agm_event_cb_params *)calloc(
            1, (sizeof(struct agm_event_cb_params) + payloadSize));
    if (!agmEventCallbackParam) {
        ALOGE("Not enough memory for agmEventCallbackParam");
        return ::ndk::ScopedAStatus::ok();
    }

    // TODO move to convert
    agmEventCallbackParam->event_payload_size = payloadSize;
    agmEventCallbackParam->event_id = in_eventParam.eventId;
    agmEventCallbackParam->source_module_id = in_eventParam.sourceModuleId;
    int8_t *src = (int8_t *)in_eventParam.eventPayload.data();
    int8_t *dst = (int8_t *)agmEventCallbackParam->event_payload;
    memcpy(dst, src, payloadSize);

    mCallback(mSessionId, agmEventCallbackParam, mClientData);
    free(agmEventCallbackParam);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus AgmCallback::eventCallbackReadWriteDone(
        const ::aidl::vendor::qti::hardware::agm::AgmReadWriteEventCallbackParams
                &in_rwDonePayload) {
    struct agm_event_cb_params *agmLegacyEventParams = NULL;
    struct agm_event_read_write_done_payload agmLegacyReadWriteDonePayload;
    struct agm_buff *buffer = &agmLegacyReadWriteDonePayload.buff;

    // memset(&agmLegacyReadWriteDonePayload, 0, sizeof(struct agm_event_read_write_done_payload));
    agmLegacyEventParams = (struct agm_event_cb_params *)calloc(
            1, (sizeof(struct agm_event_cb_params) +
                sizeof(struct agm_event_read_write_done_payload)));
    if (!agmLegacyEventParams) {
        ALOGE("Not enough memory for agmLegacyEventParams");
        return status_tToBinderResult(-ENOMEM);
    }

    agmLegacyEventParams->event_payload_size = sizeof(agmLegacyReadWriteDonePayload);
    agmLegacyEventParams->event_id = in_rwDonePayload.eventId;
    agmLegacyEventParams->source_module_id = in_rwDonePayload.sourceModuleId;

    agmLegacyReadWriteDonePayload.tag = in_rwDonePayload.payload.tag;
    agmLegacyReadWriteDonePayload.status = in_rwDonePayload.payload.status;
    agmLegacyReadWriteDonePayload.md_status = in_rwDonePayload.payload.metadataStatus;
    buffer->timestamp = in_rwDonePayload.payload.buffer.timestamp;
    buffer->flags = in_rwDonePayload.payload.buffer.flags;
    buffer->size = in_rwDonePayload.payload.buffer.size;

    // fdInfo <int /*fd*/ ,int /*dup*/>
    auto fdInfo = AidlToLegacy::getFdIntFromNativeHandle(
            in_rwDonePayload.payload.buffer.externalAllocInfo.allocHandle, false /*dodup*/);

    buffer->alloc_info.alloc_handle = fdInfo.second; // allochandle->data[1];
    ALOGV("alloc handleinput[0] %d and input[1] %d ", fdInfo.first, fdInfo.second);
    buffer->alloc_info.alloc_size = in_rwDonePayload.payload.buffer.externalAllocInfo.allocatedSize;
    buffer->alloc_info.offset = in_rwDonePayload.payload.buffer.externalAllocInfo.offset;

    if (!in_rwDonePayload.payload.buffer.metadata.empty()) {
        buffer->metadata_size = in_rwDonePayload.payload.buffer.metadata.size();
        buffer->metadata = (uint8_t *)calloc(1, buffer->metadata_size);
        if (!buffer->metadata) {
            ALOGE("Not enough memory for buffer->metadata");
            free(agmLegacyEventParams);
            return status_tToBinderResult(-ENOMEM);
        }
        memcpy(buffer->metadata, in_rwDonePayload.payload.buffer.metadata.data(),
               buffer->metadata_size);
    }
    memcpy(agmLegacyEventParams->event_payload, &agmLegacyReadWriteDonePayload,
           sizeof(agmLegacyReadWriteDonePayload));

    mCallback(mSessionId, agmLegacyEventParams, mClientData);
    if (buffer->metadata) free(buffer->metadata);
    free(agmLegacyEventParams);
    return ::ndk::ScopedAStatus::ok();
}
}
