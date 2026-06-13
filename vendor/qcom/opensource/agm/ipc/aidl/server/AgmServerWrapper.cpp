/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#define NDEBUG 0
#define LOG_TAG "AgmIpc::Server"

#include <agm/AgmAidlToLegacy.h>
#include <agm/AgmLegacyToAidl.h>
#include <agm/Utils.h>

#include "AgmServerWrapper.h"

#include <agm/BinderStatus.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <cutils/ashmem.h>
#include <cutils/native_handle.h>
#include "gsl_intf.h"
using ndk::ScopedAStatus;

namespace aidl::vendor::qti::hardware::agm {

void SessionInfo::dump() {
    ALOGV("session Id %d handle %llx, aif size %d", mSessionId, mHandle, mAifIds.size());
}

SessionInfo::~SessionInfo() {
    ALOGV("%s session Id %d handle %llx, noOfAifs %d fdPairs %d", __func__, mSessionId, mHandle,
          mAifIds.size(), mInOutFdPairs.size());
}

void SessionInfo::forceCloseSession() {
    std::lock_guard<std::mutex> guard(mLock);
    if (mHandle) {
        ALOGV("force closing session with handle %llx", mHandle);
        agm_session_close(mHandle);
    }

    // force clean up of aifIds.
    for (const auto &aifId : mAifIds) {
        agm_session_aif_set_params(mSessionId, aifId, NULL, 0);
        agm_session_aif_connect(mSessionId, aifId, false);
        agm_session_aif_set_metadata(mSessionId, aifId, 0, NULL);
    }

    agm_session_set_metadata(mSessionId, 0, NULL);
    agm_session_set_params(mSessionId, NULL, 0);
}

void SessionInfo::connectSessionAif(uint32_t aifId, bool state) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s session %d aifId %d state %d aif size %d ", __func__, mSessionId, aifId, state,
          mAifIds.size());
    if (state) {
        mAifIds.insert(aifId);
    } else {
        mAifIds.erase(aifId);
    }
    ALOGV("%s session %d aifId %d state %d aif size %d ", __func__, mSessionId, aifId, state,
          mAifIds.size());
}

void SessionInfo::addSharedMemoryFdPairs(int inputFd, int dupFd) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s session %d Fds[input %d - dup %d] size %d", __func__, mSessionId, inputFd, dupFd,
          mInOutFdPairs.size());
    mInOutFdPairs.push_back(std::make_pair(inputFd, dupFd));
}

int SessionInfo::removeSharedMemoryFdPairs(int dupFd) {
    std::lock_guard<std::mutex> guard(mLock);
    auto itr = mInOutFdPairs.begin();
    auto inputFd = -1;
    for (; itr != mInOutFdPairs.end(); itr++) {
        if (itr->second == dupFd) {
            inputFd = itr->first;
            itr = mInOutFdPairs.erase(itr);
            break;
        }
    }
    ALOGV("%s session %d Fds[input %d - dup %d] size %d", __func__, mSessionId, inputFd, dupFd,
          mInOutFdPairs.size());
    return inputFd;
}

AgmServerWrapper *ClientInfo::sAgmServerWrapper = nullptr;
void ClientInfo::setAgmServerWrapper(AgmServerWrapper *wrapper) {
    sAgmServerWrapper = wrapper;
}

void ClientInfo::onDeath(void *cookie) {
    ClientInfo *client = static_cast<ClientInfo *>(cookie);
    ALOGI("Client died (pid): %llu", client->getPid());
    client->onDeath();
}

void ClientInfo::onDeath() {
    sAgmServerWrapper->removeClient(mPid);
}

void ClientInfo::registerCallback(const std::shared_ptr<IAGMCallback> &callback,
                                  int32_t in_sessionId, int32_t in_eventType,
                                  int64_t in_clientData) {
    std::lock_guard<std::mutex> guard(mCallbackLock);
    ALOGV("%s, adding callback for session %d size %d ", __func__, in_sessionId,
          mCallbackInfo.size());
    agm_session_register_cb(in_sessionId, &ClientInfo::onCallback, (enum event_type)in_eventType,
                            (void *)callback.get());

    auto linkRet = AIBinder_linkToDeath(callback->asBinder().get(), mDeathRecipient.get(),
                                        this /* cookie */);
    if (linkRet != STATUS_OK) {
        ALOGV("%s, linkToDeath failed pid %d", __func__, mPid);
    } else {
        ALOGV("%s, linkToDeath success for client pid %d", __func__, mPid);
    }
    auto callbackInfo =
            std::make_shared<CallbackInfo>(callback, in_sessionId, in_eventType, in_clientData);
    mCallbackInfo.emplace_back(callbackInfo);
}

void ClientInfo::unregisterCallback(int32_t in_sessionId, int32_t in_eventType,
                                    int64_t in_clientData) {
    // remove based on sessionid, eventType, clientData from CallbackInfos.
    std::lock_guard<std::mutex> guard(mCallbackLock);
    ALOGV("%s, before removing callback for session %d size %d ", __func__, in_sessionId,
          mCallbackInfo.size());
    auto itr = std::find_if(mCallbackInfo.begin(), mCallbackInfo.end(),
                            [=](const std::shared_ptr<CallbackInfo> &callback) {
                                return ((callback->getSessionId() == in_sessionId) &&
                                        (callback->getEventType() == in_eventType) &&
                                        (callback->getClientData() == in_clientData));
                            });

    std::shared_ptr<IAGMCallback> registeredCallback = nullptr;
    if (itr != mCallbackInfo.end()) {
        registeredCallback = (*itr)->getCallback();
        agm_session_register_cb(in_sessionId, NULL, (enum event_type)in_eventType,
                                (void *)registeredCallback.get());
        mCallbackInfo.erase(itr);
        AIBinder_unlinkToDeath(registeredCallback->asBinder().get(), mDeathRecipient.get(),
                               nullptr);
    }

    ALOGV("%s, removing callback for session %d size %d ", __func__, in_sessionId,
          mCallbackInfo.size());
}

void ClientInfo::clearCallbacks() {
    std::lock_guard<std::mutex> guard(mCallbackLock);
    ALOGV("client going out of scope clear callback of size %d", mCallbackInfo.size());
    for (const auto &callback : mCallbackInfo) {
        // unregister at agm level..
        agm_session_register_cb(callback->getSessionId(), NULL,
                                (enum event_type)callback->getEventType(),
                                (void *)callback->getCallback().get());
    }
    mCallbackInfo.clear();
}

void ClientInfo::clearSessions() {
    std::lock_guard<std::mutex> guard(mSessionLock);
    ALOGI("%s session size %d ", __func__, mSessionsInfoMap.size());
    for (const auto &session : mSessionsInfoMap) {
        session.second->forceCloseSession();
    }
    mSessionsInfoMap.clear();
}

std::shared_ptr<SessionInfo> ClientInfo::getSessionInfo_l(int32_t sessionId) {
    if (mSessionsInfoMap.count(sessionId) == 0) {
        ALOGV("new session %d ", sessionId);
        mSessionsInfoMap[sessionId] = std::make_shared<SessionInfo>(sessionId);
    }
    return mSessionsInfoMap[sessionId];
}

void ClientInfo::addSessionHandle(uint32_t sessionId, uint64_t handle) {
    std::lock_guard<std::mutex> guard(mSessionLock);

    auto sessionInfo = getSessionInfo_l(sessionId);
    sessionInfo->setHandle(handle);
    ALOGI("%s sessionId %d handle %llx ", __func__, sessionId, handle);
}

void ClientInfo::removeSessionHandle(uint64_t handle) {
    std::lock_guard<std::mutex> guard(mSessionLock);
    ALOGV("%s,  removeSessionhandle %llx in session of size %d ", __func__, handle,
          mSessionsInfoMap.size());
    auto itr = mSessionsInfoMap.begin();
    for (; itr != mSessionsInfoMap.end();) {
        auto sessionInfo = itr->second;
        if (handle == sessionInfo->getHandle()) {
            ALOGI("%s removing handle %llx for session %d", __func__, handle, itr->first);
            mSessionsInfoMap.erase(itr); // TODO current behavior is similar to HIDL, on close ->
                                         // remove sessionhandle
            break;
        }
        itr++;
    }
    ALOGV("%s, Exit: removeSessionhandle %llx in session of size %d ", __func__, handle,
          mSessionsInfoMap.size());
}

void ClientInfo::connectSessionAif(uint32_t sessionId, uint32_t aifId, bool state) {
    std::lock_guard<std::mutex> guard(mSessionLock);
    ALOGV("%s sessionId %d aifId %d state %d", __func__, sessionId, aifId, state);
    auto sessionInfo = getSessionInfo_l(sessionId);
    sessionInfo->connectSessionAif(aifId, state);
}

void ClientInfo::cleanup() {
    // Do a cleanup related to client going out of scope.
    ALOGI("%s client %d callbacks %d sessions %d ", __func__, mPid,
          mCallbackInfo.size(), mSessionsInfoMap.size());
    clearCallbacks();
    clearSessions();
}

void ClientInfo::addSharedMemoryFdPairs(uint64_t handle, int inputFd, int dupFd) {
    std::lock_guard<std::mutex> guard(mSessionLock);
    ALOGV("%s handle %llx, inputFd %d dupFd %d sessionSize %d", __func__, handle, inputFd, dupFd,
          mSessionsInfoMap.size());
    for (auto &sessionInfo : mSessionsInfoMap) {
        auto &sessionInfoObj = sessionInfo.second;
        if (sessionInfoObj->getHandle() == handle) {
            sessionInfoObj->addSharedMemoryFdPairs(inputFd, dupFd);
            break;
        }
    }
}

int ClientInfo::removeSharedMemoryFdPairs(uint32_t sessionId, int dupFd) {
    std::lock_guard<std::mutex> guard(mSessionLock);

    auto inputFd = -1;
    for (auto &sessionInfo : mSessionsInfoMap) {
        auto &sessionInfoObj = sessionInfo.second;
        if (sessionInfo.first == sessionId) {
            inputFd = sessionInfoObj->removeSharedMemoryFdPairs(dupFd);
            break;
        }
    }
    ALOGV("%s sessionId %d, inputFd %d dupFd %d sessionSize %d", __func__, sessionId, inputFd,
          dupFd, mSessionsInfoMap.size());
    return inputFd;
}

void ClientInfo::onCallback(uint32_t sessionId, struct agm_event_cb_params *eventParams,
                            void *clientData) {
    uint32_t eventId = eventParams->event_id;

    IAGMCallback *callback = reinterpret_cast<IAGMCallback *>(clientData);

    if (!AIBinder_isAlive(callback->asBinder().get())) {
        ALOGW("callback binder has died");
        return;
    }

    if ((eventParams->event_payload_size > 0) &&
        ((eventId == AGM_EVENT_READ_DONE) || (eventId == AGM_EVENT_WRITE_DONE))) {
        int dupFd = LegacyToAidl::getDupedFdFromAgmEventParams(eventParams);
        int inputFd = sAgmServerWrapper->removeSharedMemoryFdPairs(sessionId, dupFd);
        auto aidlReadWriteDoneParams =
                LegacyToAidl::convertAgmReadWriteEventCallbackParamsToAidl(eventParams, inputFd);
        callback->eventCallbackReadWriteDone(aidlReadWriteDoneParams);
        LegacyToAidl::cleanUpMetadataMemory(eventParams);
    } else {
        auto aidlEventParams = LegacyToAidl::convertAgmEventCallbackParametersToAidl(eventParams);
        callback->eventCallback(aidlEventParams);
    }
}

inline uint64_t convertAidlHandleToLegacy(int64_t handleAidl) {
    return (static_cast<uint64_t>(handleAidl));
}

void AgmServerWrapper::addSessionHandle(uint32_t sessionId, uint64_t handle) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s, caller session %d", __func__, sessionId);

    auto client = getClient_l();
    client->addSessionHandle(sessionId, handle);
}

void AgmServerWrapper::removeSessionHandle(uint64_t handle) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s, caller handle %llx", __func__, handle);

    auto client = getClient_l();
    client->removeSessionHandle(handle);
}

void AgmServerWrapper::addSharedMemoryFdPairs(uint64_t handle, int inputFd, int dupFd) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s, caller handle %llx inputFd %d dupFd %d", __func__, handle, inputFd, dupFd);

    auto client = getClient_l();
    client->addSharedMemoryFdPairs(handle, inputFd, dupFd);
}

int AgmServerWrapper::removeSharedMemoryFdPairs(uint32_t sessionId, int dupFd) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s, caller sessionId %d  dupFd %d", __func__, sessionId, dupFd);

    int inputFd = -1;
    for (auto &client: mClients) {
         auto &clientInfoObj = client.second;
         inputFd = clientInfoObj->removeSharedMemoryFdPairs(sessionId, dupFd);
         if (inputFd != -1)
             return inputFd;
    }
    return inputFd;
}

void AgmServerWrapper::connectSessionAif(uint32_t sessionId, uint32_t aifId, bool state) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s, caller session %d aif %d state %d", __func__, sessionId, aifId, state);

    auto client = getClient_l();
    client->connectSessionAif(sessionId, aifId, state);
}

void AgmServerWrapper::removeClient(int pid) {
    std::lock_guard<std::mutex> guard(mLock);
    if (mClients.count(pid) != 0) {
        ALOGI("%s removing client %d", __func__, pid);
        mClients.erase(pid);
    } else {
        /*
        * client is already gone, nothing to do.
        */
    }
}

std::shared_ptr<ClientInfo> AgmServerWrapper::getClient_l() {
    int pid = AIBinder_getCallingPid();
    if (mClients.count(pid) == 0) {
        ALOGV("%s new client pid %d, total clients %d ", __func__, pid, mClients.size());
        mClients[pid] = std::make_shared<ClientInfo>(pid);
        ClientInfo::setAgmServerWrapper(this);
    }
    return mClients[pid];
}

AgmServerWrapper::AgmServerWrapper() {
    mInitialized = (agm_init() == 0);
    ALOGI("%s created", __func__);
}

AgmServerWrapper::~AgmServerWrapper() {
    ALOGI("%s destroyed", __func__);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_init() {
    ALOGV("%s ", __func__);
    return ScopedAStatus::ok();
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_deinit() {
    ALOGV("%s ", __func__);
    return ScopedAStatus::ok();
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_start(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_start(in_handle));
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_stop(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_stop(in_handle));
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_suspend(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_suspend(in_handle));
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_close(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    removeSessionHandle(in_handle);
    return status_tToBinderResult(agm_session_close(in_handle));
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_eos(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_eos(in_handle));
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_flush(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_flush(in_handle));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_register_callback(
        const std::shared_ptr<IAGMCallback> &in_callback, int32_t in_sessionId,
        int32_t in_eventType, bool in_register, int64_t in_clientData) {
    std::lock_guard<std::mutex> guard(mLock);
    ALOGV("%s called with sessionId = %llx register %d eventType %d", __func__,
          (unsigned long long)in_sessionId, in_register, in_eventType);

    auto client = getClient_l();
    if (in_register)
        client->registerCallback(in_callback, in_sessionId, in_eventType, in_clientData);
    else
        client->unregisterCallback(in_sessionId, in_eventType, in_clientData);

    return ScopedAStatus::ok();
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_get_buf_info(int32_t in_sessionId,
                                                                    int32_t in_flag,
                                                                    MmapBufInfo *_aidl_return) {
    struct agm_buf_info agmLegacyBufferInfo;

    memset(&agmLegacyBufferInfo, 0, sizeof(struct agm_buf_info));
    int ret = agm_session_get_buf_info(in_sessionId, &agmLegacyBufferInfo, in_flag);
    if (!ret) {
        LegacyToAidl::convertMmapBufferInfoToAidl(&agmLegacyBufferInfo, _aidl_return, in_flag);
    }
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_get_params(
        int32_t in_sessionId, const std::vector<uint8_t> &in_buffer,
        std::vector<uint8_t> *_aidl_return) {
    ALOGV("%s sessionId %d, size %d ", __func__, in_sessionId, in_buffer.size());

    auto agmLegacyPayload = VALUE_OR_RETURN(allocate<uint8_t>(in_buffer.size()));

    memcpy(agmLegacyPayload.get(), in_buffer.data(), in_buffer.size());

    int ret = agm_session_get_params(in_sessionId, agmLegacyPayload.get(), in_buffer.size());
    if (!ret) {
        _aidl_return->resize(in_buffer.size());

        if (_aidl_return->size() != in_buffer.size()) {
            ALOGE("%s could not resize: required size %d, resized size %d ", __func__,
                    in_buffer.size(), _aidl_return->size());
            return status_tToBinderResult(-ENOMEM);
        }

        memcpy(_aidl_return->data(), agmLegacyPayload.get(), in_buffer.size());
    }
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_open(int32_t in_sessionId,
                                                            AgmSessionMode in_sessionMode,
                                                            int64_t *_aidl_return) {
    enum agm_session_mode sessionMode = static_cast<agm_session_mode>(in_sessionMode);
    ALOGV("%s: sessionId =%d sessionMode =%d", __func__, in_sessionId, sessionMode);

    uint64_t handle = 0;
    int ret = agm_session_open(in_sessionId, sessionMode, &handle);

    *_aidl_return = static_cast<int64_t>(handle); // TODO;
    if (!ret) {
        addSessionHandle(in_sessionId, handle);
    }

    ALOGV("%s handle received is : %llx, ret %d, rethandle %llx", __func__,
          (unsigned long long)handle, ret, *_aidl_return);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_pause(int64_t in_handle) {
    ALOGV("%s handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_pause(in_handle));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_prepare(int64_t in_handle) {
    ALOGV("%s handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_prepare(in_handle));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_read(int64_t in_handle, int32_t in_count,
                                                            std::vector<uint8_t> *_aidl_return) {
    auto agmLegacyBuffer = VALUE_OR_RETURN(allocate<void>(in_count));
    void *agmLegacyBufferPtr = agmLegacyBuffer.get();
    size_t bytesRead = (size_t)in_count;
    int ret = agm_session_read(in_handle, agmLegacyBufferPtr, &bytesRead);
    _aidl_return->resize(bytesRead);
    memcpy(_aidl_return->data(), agmLegacyBufferPtr, bytesRead);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_read_with_metadata(
        int64_t in_handle, const AgmBuff &in_buffer, int32_t in_capturedSize,
        IAGM::AgmReadWithMetadataReturn *_aidl_return) {
    struct agm_buff agmLegacyBuffer;
    uint32_t captured_size = in_capturedSize;

    uint32_t bufSize = in_buffer.size;
    agmLegacyBuffer.addr = (uint8_t *)calloc(1, bufSize);
    if (!agmLegacyBuffer.addr) {
        ALOGE("%s: failed to calloc", __func__);
        return status_tToBinderResult(-ENOMEM);
    }
    agmLegacyBuffer.size = (size_t)bufSize;
    agmLegacyBuffer.metadata_size = in_buffer.metadata.size();

    // metadata is deallocated during READ_DONE callback.
    agmLegacyBuffer.metadata = (uint8_t *)calloc(1, agmLegacyBuffer.metadata_size);
    if (!agmLegacyBuffer.metadata) {
        free(agmLegacyBuffer.addr);
        ALOGE("%s: failed to calloc", __func__);
        return status_tToBinderResult(-ENOMEM);
    }
    agmLegacyBuffer.timestamp = 0;
    agmLegacyBuffer.flags = 0;

    auto fdHandle = AidlToLegacy::getFdIntFromNativeHandle(in_buffer.externalAllocInfo.allocHandle);

    agmLegacyBuffer.alloc_info.alloc_handle = (fdHandle.first);
    addSharedMemoryFdPairs(in_handle, fdHandle.second, agmLegacyBuffer.alloc_info.alloc_handle);

    agmLegacyBuffer.alloc_info.alloc_size = in_buffer.externalAllocInfo.allocatedSize;
    agmLegacyBuffer.alloc_info.offset = in_buffer.externalAllocInfo.offset;

    int32_t ret = agm_session_read_with_metadata(in_handle, &agmLegacyBuffer, &captured_size);

    if (agmLegacyBuffer.addr) free(agmLegacyBuffer.addr);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_register_for_events(
        int32_t in_sessionId, const AgmEventRegistrationConfig &in_evt_reg_cfg) {
    ALOGV("%s sessionId %d ", __func__, in_sessionId);
    auto allocSize = sizeof(struct agm_event_reg_cfg) + in_evt_reg_cfg.eventConfigPayload.size();
    auto agmLegacyEventRegConfig = VALUE_OR_RETURN(allocate<agm_event_reg_cfg>(allocSize));
    AidlToLegacy::convertAgmEventRegistrationConfig(in_evt_reg_cfg, agmLegacyEventRegConfig.get());

    return status_tToBinderResult(
            agm_session_register_for_events(in_sessionId, agmLegacyEventRegConfig.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_resume(int64_t in_handle) {
    ALOGV("%s called with handle = %llx", __func__, (unsigned long long)in_handle);
    return status_tToBinderResult(agm_session_resume(in_handle));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_set_config(
        int64_t in_handle, const AgmSessionConfig &in_sessionConfig,
        const AgmMediaConfig &in_mediaConfig, const AgmBufferConfig &in_bufferConfig) {
    auto agmLegacyMediaConfig =
            VALUE_OR_RETURN(allocate<agm_media_config>(sizeof(struct agm_media_config)));
    auto agmLegacySessionConfig =
            VALUE_OR_RETURN(allocate<agm_session_config>(sizeof(struct agm_session_config)));
    auto agmLegacyBufferConfig =
            VALUE_OR_RETURN(allocate<agm_buffer_config>(sizeof(struct agm_buffer_config)));

    AidlToLegacy::convertAgmMediaConfig(in_mediaConfig, agmLegacyMediaConfig.get());
    AidlToLegacy::convertAgmSessionConfig(in_sessionConfig, agmLegacySessionConfig.get());
    AidlToLegacy::convertAgmBufferConfig(in_bufferConfig, agmLegacyBufferConfig.get());

    return status_tToBinderResult(agm_session_set_config(in_handle, agmLegacySessionConfig.get(),
                                                         agmLegacyMediaConfig.get(),
                                                         agmLegacyBufferConfig.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_set_ec_ref(int32_t in_sessionId,
                                                                  int32_t in_aif_id,
                                                                  bool in_state) {
    ALOGV("%s sessionId %d aifId %d state %d ", __func__, in_sessionId, in_aif_id, in_state);
    return status_tToBinderResult(agm_session_set_ec_ref(in_sessionId, in_aif_id, in_state));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_set_loopback(int32_t in_captureSessionId,
                                                                    int32_t in_playbackSessionId,
                                                                    bool in_state) {
    ALOGV("%s capture session %d playback session %d state %d ", __func__, in_captureSessionId,
          in_playbackSessionId, in_state);
    return status_tToBinderResult(
            agm_session_set_loopback(in_captureSessionId, in_playbackSessionId, in_state));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_set_metadata(
        int32_t in_sessionId, const std::vector<uint8_t> &in_metadata) {
    ALOGV("%s sessionId %d,  size %d ", __func__, in_sessionId, in_metadata.size());
    auto metadataLegacy = VALUE_OR_RETURN(allocate<uint8_t>(in_metadata.size()));
    memcpy(metadataLegacy.get(), in_metadata.data(), in_metadata.size());
    return status_tToBinderResult(
            agm_session_set_metadata(in_sessionId, in_metadata.size(), metadataLegacy.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_set_non_tunnel_mode_config(
        int64_t in_handle, const AgmSessionConfig &in_sessionConfig,
        const AgmMediaConfig &in_inMediaConfig, const AgmMediaConfig &in_outMediaConfig,
        const AgmBufferConfig &in_inBufferConfig, const AgmBufferConfig &in_outBufferConfig) {
    auto agmLegacyInMediaConfig =
            VALUE_OR_RETURN(allocate<agm_media_config>(sizeof(struct agm_media_config)));
    auto agmLegacyOutMediaConfig =
            VALUE_OR_RETURN(allocate<agm_media_config>(sizeof(struct agm_media_config)));
    auto agmLegacySessionConfig =
            VALUE_OR_RETURN(allocate<agm_session_config>(sizeof(struct agm_session_config)));
    auto agmLegacyInBufferConfig =
            VALUE_OR_RETURN(allocate<agm_buffer_config>(sizeof(struct agm_buffer_config)));
    auto agmLegacyOutBufferConfig =
            VALUE_OR_RETURN(allocate<agm_buffer_config>(sizeof(struct agm_buffer_config)));

    AidlToLegacy::convertAgmMediaConfig(in_inMediaConfig, agmLegacyInMediaConfig.get());
    AidlToLegacy::convertAgmMediaConfig(in_outMediaConfig, agmLegacyOutMediaConfig.get());
    AidlToLegacy::convertAgmSessionConfig(in_sessionConfig, agmLegacySessionConfig.get());
    AidlToLegacy::convertAgmBufferConfig(in_inBufferConfig, agmLegacyInBufferConfig.get());
    AidlToLegacy::convertAgmBufferConfig(in_outBufferConfig, agmLegacyOutBufferConfig.get());

    return status_tToBinderResult(agm_session_set_non_tunnel_mode_config(
            in_handle, agmLegacySessionConfig.get(), agmLegacyInMediaConfig.get(),
            agmLegacyOutMediaConfig.get(), agmLegacyInBufferConfig.get(),
            agmLegacyOutBufferConfig.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_set_params(
        int32_t in_sessionId, const std::vector<uint8_t> &in_payload) {
    size_t payloadSize = in_payload.size();
    ALOGV("%s : sessionId = %d size = %d", __func__, in_sessionId, payloadSize);

    auto legacyPayload = VALUE_OR_RETURN(allocate<void>(payloadSize));
    memcpy(legacyPayload.get(), in_payload.data(), payloadSize);
    return status_tToBinderResult(
            agm_session_set_params(in_sessionId, legacyPayload.get(), payloadSize));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_write(int64_t in_handle,
                                                             const std::vector<uint8_t> &in_buff,
                                                             int32_t *_aidl_return) {
    size_t count = (size_t)in_buff.size();
    ALOGV("%s called with handle = %llx ", __func__, (unsigned long long)in_handle, count);

    auto buffer = VALUE_OR_RETURN(allocate<void>(count));
    memcpy(buffer.get(), in_buff.data(), count);
    int ret = agm_session_write(in_handle, buffer.get(), &count);
    *_aidl_return = static_cast<int32_t>(count);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_write_datapath_params(
        int32_t in_sessionId, const AgmBuff &in_buff) {
    ALOGW("%s sessionId %d", __func__, in_sessionId);
    struct agm_buff agmLegacyBuffer;
    uint32_t bufSize = in_buff.buffer.size();
    agmLegacyBuffer.addr = nullptr;
    agmLegacyBuffer.metadata = nullptr;

    agmLegacyBuffer.addr = (uint8_t *)calloc(1, bufSize);
    if (!agmLegacyBuffer.addr) {
        ALOGE("%s: failed to allocate agm buffer", __func__);
        return status_tToBinderResult(-ENOMEM);
    }

    AidlToLegacy::convertAgmBuffer(in_buff, &agmLegacyBuffer);

    int ret = agm_session_write_datapath_params(in_sessionId, &agmLegacyBuffer);

    if (agmLegacyBuffer.addr != nullptr) free(agmLegacyBuffer.addr);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_write_with_metadata(int64_t in_handle,
                                                                           const AgmBuff &in_buff,
                                                                           int32_t *_aidl_return) {
    struct agm_buff agmLegacyBuffer;
    uint32_t bufSize = in_buff.size;
    agmLegacyBuffer.addr = nullptr;
    agmLegacyBuffer.metadata = nullptr;

    agmLegacyBuffer.addr = (uint8_t *)calloc(1, bufSize);
    if (!agmLegacyBuffer.addr) {
        ALOGE("%s: failed to calloc", __func__);
        return status_tToBinderResult(-ENOMEM);
    }

    agmLegacyBuffer.size = (size_t)bufSize;
    agmLegacyBuffer.timestamp = in_buff.timestamp;
    agmLegacyBuffer.flags = in_buff.flags;
    if (in_buff.metadata.size()) {
        agmLegacyBuffer.metadata_size = in_buff.metadata.size();
        agmLegacyBuffer.metadata = (uint8_t *)calloc(1, agmLegacyBuffer.metadata_size);
        if (!agmLegacyBuffer.metadata) {
            ALOGE("%s: failed to calloc", __func__);
            free(agmLegacyBuffer.addr);
            return status_tToBinderResult(-ENOMEM);
        }
        memcpy(agmLegacyBuffer.metadata, in_buff.metadata.data(), agmLegacyBuffer.metadata_size);
    }

    auto fdInfo = AidlToLegacy::getFdIntFromNativeHandle(in_buff.externalAllocInfo.allocHandle);

    agmLegacyBuffer.alloc_info.alloc_handle = fdInfo.first;

    addSharedMemoryFdPairs(in_handle, fdInfo.second, agmLegacyBuffer.alloc_info.alloc_handle);

    agmLegacyBuffer.alloc_info.alloc_size = in_buff.externalAllocInfo.allocatedSize;
    agmLegacyBuffer.alloc_info.offset = in_buff.externalAllocInfo.offset;

    if (bufSize && agmLegacyBuffer.addr) memcpy(agmLegacyBuffer.addr, in_buff.buffer.data(), bufSize);

    size_t consumed_size = 0;
    int32_t ret = agm_session_write_with_metadata(in_handle, &agmLegacyBuffer, &consumed_size);

    *_aidl_return = consumed_size;
    if (agmLegacyBuffer.metadata != nullptr) free(agmLegacyBuffer.metadata);
    if (agmLegacyBuffer.addr != nullptr) free(agmLegacyBuffer.addr);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_aif_group_set_media_config(
        int32_t in_groupId, const AgmGroupMediaConfig &in_config) {
    ALOGV("%s called with aif_id = %d", __func__, in_groupId);

    auto agmLegacyConfig = VALUE_OR_RETURN(
            allocate<agm_group_media_config>(sizeof(struct agm_group_media_config)));
    AidlToLegacy::convertAgmGroupMediaConfig(in_config, agmLegacyConfig.get());
    return status_tToBinderResult(
            agm_aif_group_set_media_config(in_groupId, agmLegacyConfig.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_aif_set_media_config(
        int32_t in_aifId, const AgmMediaConfig &in_config) {
    ALOGV("%s called with aif_id = %d", __func__, in_aifId);

    auto agmLegacyConfig =
            VALUE_OR_RETURN(allocate<agm_media_config>(sizeof(struct agm_media_config)));
    AidlToLegacy::convertAgmMediaConfig(in_config, agmLegacyConfig.get());

    return status_tToBinderResult(agm_aif_set_media_config(in_aifId, agmLegacyConfig.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_aif_set_metadata(
        int32_t in_aifId, const std::vector<uint8_t> &in_metadata) {
    ALOGV("%s called with aif_id = %d,  size %d", __func__, in_aifId, in_metadata.size());

    auto agmLegacyMetadata = VALUE_OR_RETURN(allocate<uint8_t>(in_metadata.size()));
    memcpy(agmLegacyMetadata.get(), in_metadata.data(), in_metadata.size());

    return status_tToBinderResult(
            agm_aif_set_metadata(in_aifId, in_metadata.size(), agmLegacyMetadata.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_aif_set_params(
        int32_t in_aifId, const std::vector<uint8_t> &in_payload) {
    ALOGV("%s %d aifId %d size %d", __func__, in_aifId, in_payload.size());

    auto agmLegacyPayload = VALUE_OR_RETURN(allocate<uint8_t>(in_payload.size()));
    memcpy(agmLegacyPayload.get(), in_payload.data(), in_payload.size());

    return status_tToBinderResult(
            agm_aif_set_params(in_aifId, agmLegacyPayload.get(), in_payload.size()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_aif_connect(int32_t in_sessionId,
                                                                   int32_t in_aifId,
                                                                   bool in_state) {
    ALOGV("%s sessionId %d,aifId %d, state %d ", __func__, in_sessionId, in_aifId, in_state);

    connectSessionAif(in_sessionId, in_aifId, in_state);
    return status_tToBinderResult(agm_session_aif_connect(in_sessionId, in_aifId, in_state));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_aif_get_tag_module_info(
        int32_t in_sessionId, int32_t in_aifId, int32_t in_size,
        std::vector<uint8_t> *_aidl_return) {
    ALOGV("%s : in_sessionId = %d, aif_id =%d, size = %d", __func__, in_sessionId, in_aifId,
          in_size);

    uint8_t *agmLegacyPayload = NULL;
    size_t payloadSize = (size_t)in_size;

    if (payloadSize) {
        agmLegacyPayload = (uint8_t *)calloc(1, payloadSize);
        if (agmLegacyPayload == NULL) {
            ALOGE("%s: Cannot allocate memory for agm payload ", __func__);
            return status_tToBinderResult(-ENOMEM);
        }
    }

    int32_t ret = agm_session_aif_get_tag_module_info(in_sessionId, in_aifId, agmLegacyPayload,
                                                      &payloadSize);

    ALOGV("%s: got size %d, ret %d ", __func__, payloadSize, ret);
    _aidl_return->resize(payloadSize);

    if (agmLegacyPayload) memcpy(_aidl_return->data(), agmLegacyPayload, payloadSize);

    free(agmLegacyPayload);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_aif_set_cal(
        int32_t in_sessionId, int32_t in_aifId, const AgmCalConfig &in_calConfig) {
    auto numKvPairs = in_calConfig.kv.size();
    ALOGV("%s : sessionId = %d, aifId = %d calKeys %d", __func__, in_sessionId, in_aifId,
          numKvPairs);

    RETURN_IF_KVPAIR_EXCEEDS_RANGE(numKvPairs);
    auto allocSize = sizeof(struct agm_cal_config) + numKvPairs * sizeof(struct agm_key_value);
    auto agmLegacyCalConfig = VALUE_OR_RETURN(allocate<agm_cal_config>(allocSize));

    AidlToLegacy::convertAgmCalConfig(in_calConfig, agmLegacyCalConfig.get());
    return status_tToBinderResult(
            agm_session_aif_set_cal(in_sessionId, in_aifId, agmLegacyCalConfig.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_aif_set_metadata(
        int32_t in_sessionId, int32_t in_aifId, const std::vector<uint8_t> &in_metadata) {
    auto agmLegacyMetadata = VALUE_OR_RETURN(allocate<uint8_t>(in_metadata.size()));
    memcpy(agmLegacyMetadata.get(), in_metadata.data(), in_metadata.size());
    return status_tToBinderResult(agm_session_aif_set_metadata(
            in_sessionId, in_aifId, in_metadata.size(), agmLegacyMetadata.get()));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_session_aif_set_params(
        int32_t in_sessionId, int32_t in_aifId, const std::vector<uint8_t> &in_payload) {
    size_t payloadSize = in_payload.size();
    ALOGV("%s : sessionId = %d, aif_id =%d, size = %d", __func__, in_sessionId, in_aifId,
          payloadSize);

    auto legacyPayload = VALUE_OR_RETURN(allocate<void>(payloadSize));
    memcpy(legacyPayload.get(), in_payload.data(), payloadSize);
    return status_tToBinderResult(
            agm_session_aif_set_params(in_sessionId, in_aifId, legacyPayload.get(), payloadSize));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_get_aif_info_list(
        int32_t in_numAifInfo, std::vector<AifInfo> *_aidl_return) {
    ALOGV("%s numberOfAif = %d ", __func__, in_numAifInfo);

    AgmAifUniquePtrType agmLegacyAifInfoListUPtr(nullptr, free);

    if (in_numAifInfo != 0) {
        agmLegacyAifInfoListUPtr =
                VALUE_OR_RETURN(allocate<aif_info>(sizeof(struct aif_info) * in_numAifInfo));
    }
    struct aif_info *agmLegacyAifInfoList = agmLegacyAifInfoListUPtr.get();
    size_t numberOfAifs = (size_t)in_numAifInfo;
    int32_t ret = agm_get_aif_info_list(agmLegacyAifInfoList, &numberOfAifs);
    ALOGV("%s got agmAifs = %d, ret %d  ", __func__, numberOfAifs, ret);

    *_aidl_return = LegacyToAidl::convertAifInfoListToAidl(agmLegacyAifInfoList, numberOfAifs);
    return ScopedAStatus::ok();
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_get_buffer_timestamp(int32_t in_sessiondId,
                                                                    int64_t *_aidl_return) {
    uint64_t timestampLegacy;
    int ret = agm_get_buffer_timestamp(in_sessiondId, &timestampLegacy);
    *_aidl_return = timestampLegacy;
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_get_group_aif_info_list(
        int32_t in_numberOfGroups, std::vector<AifInfo> *_aidl_return) {
    AgmAifUniquePtrType agmLegacyAifInfoListUPtr(nullptr, free);
    if (in_numberOfGroups != 0) {
        agmLegacyAifInfoListUPtr =
                VALUE_OR_RETURN(allocate<aif_info>(sizeof(struct aif_info) * in_numberOfGroups));
    }

    struct aif_info *agmLegacyAifInfoList = agmLegacyAifInfoListUPtr.get();
    size_t numberOfGroupAifs = (size_t)in_numberOfGroups;
    int32_t ret = agm_get_group_aif_info_list(agmLegacyAifInfoList, &numberOfGroupAifs);

    *_aidl_return = LegacyToAidl::convertAifInfoListToAidl(agmLegacyAifInfoList, numberOfGroupAifs);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_get_hw_processed_buff_cnt(int64_t in_handle,
                                                                         Direction in_direction) {
    ALOGV("%s handle %llx", __func__, in_handle);
    enum direction legacyDir = (enum direction)(in_direction);
    return status_tToBinderResult(agm_get_hw_processed_buff_cnt(in_handle, legacyDir));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_get_params_from_acdb_tunnel(
        const std::vector<uint8_t> &in_payload, std::vector<uint8_t> *_aidl_return) {
    size_t payloadSize = in_payload.size();
    ALOGV("%s :size = %d", __func__, payloadSize);

    auto legacyPayload = VALUE_OR_RETURN(allocate<void>(payloadSize));
    memcpy(legacyPayload.get(), in_payload.data(), payloadSize);
    int32_t ret = agm_get_params_from_acdb_tunnel(legacyPayload.get(), &payloadSize);
    _aidl_return->resize(payloadSize);
    memcpy(_aidl_return->data(), legacyPayload.get(), payloadSize);
    return status_tToBinderResult(ret);
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_get_session_time(int64_t in_handle,
                                                                int64_t *_aidl_return) {
    uint64_t timestampLegacy;
    int ret = agm_get_session_time(in_handle, &timestampLegacy);
    *_aidl_return = timestampLegacy;
    return status_tToBinderResult(ret);
}
::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_sessionid_flush(int32_t in_sessiondId) {
    ALOGV("%s sessionId %d", __func__, in_sessiondId);
    return status_tToBinderResult(agm_sessionid_flush(in_sessiondId));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_set_gapless_session_metadata(
        int64_t in_handle, AgmGaplessSilenceType in_type, int32_t in_silence) {
    ALOGV("%s handle %x", __func__, in_handle);
    enum agm_gapless_silence_type agmLegacySilenceType =
            static_cast<agm_gapless_silence_type>(in_type);
    return status_tToBinderResult(
            agm_set_gapless_session_metadata(in_handle, agmLegacySilenceType, in_silence));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_set_params_to_acdb_tunnel(
        const std::vector<uint8_t> &in_payload) {
    size_t payloadSize = in_payload.size();
    ALOGV("%s :size = %d", __func__, payloadSize);
    auto legacyPayload = VALUE_OR_RETURN(allocate<void>(payloadSize));

    memcpy(legacyPayload.get(), in_payload.data(), payloadSize);
    return status_tToBinderResult(agm_set_params_to_acdb_tunnel(legacyPayload.get(), payloadSize));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_set_params_with_tag(
        int32_t in_sessiondId, int32_t in_aifId, const AgmTagConfig &in_tagConfig) {
    ALOGV("%s : sessionId = %d, aif_id = %d", __func__, in_sessiondId, in_aifId);
    int tkvSize = in_tagConfig.kv.size();
    RETURN_IF_KVPAIR_EXCEEDS_RANGE(tkvSize);

    size_t tagConfigSize = (sizeof(struct agm_tag_config) + (tkvSize) * sizeof(agm_key_value));

    auto agmLegacyTagConfigUniquePtr = VALUE_OR_RETURN(allocate<agm_tag_config>(tagConfigSize));
    struct agm_tag_config *agmLegacyTagConfig = agmLegacyTagConfigUniquePtr.get();

    AidlToLegacy::convertAgmTagConfig(in_tagConfig, agmLegacyTagConfig);

    return status_tToBinderResult(
            agm_set_params_with_tag(in_sessiondId, in_aifId, agmLegacyTagConfig));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_set_params_with_tag_to_acdb(
        int32_t in_sessionId, int32_t in_aifId, const std::vector<uint8_t> &in_payload) {
    size_t payloadSize = in_payload.size();
    ALOGV("%s : sessionId = %d size = %d", __func__, in_sessionId, payloadSize);
    auto legacyPayload = VALUE_OR_RETURN(allocate<void>(payloadSize));
    memcpy(legacyPayload.get(), in_payload.data(), payloadSize);
    return status_tToBinderResult(agm_set_params_with_tag_to_acdb(
            in_sessionId, in_aifId, legacyPayload.get(), payloadSize));
}

::ndk::ScopedAStatus AgmServerWrapper::ipc_agm_dump(const AgmDumpInfo &in_dumpInfo) {
    return ScopedAStatus::ok();
}
}
