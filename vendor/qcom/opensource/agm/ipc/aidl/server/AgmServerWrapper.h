/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <agm/agm_api.h>
#include <aidl/vendor/qti/hardware/agm/BnAGM.h>
#include <aidl/vendor/qti/hardware/agm/IAGMCallback.h>

#include <log/log.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace aidl::vendor::qti::hardware::agm {

class SessionInfo {
    uint64_t mHandle = 0;
    std::mutex mLock;
    uint32_t mSessionId;

    using FdPair = std::pair<int, int>;
    std::vector<FdPair> mInOutFdPairs;
    std::unordered_set<uint32_t> mAifIds;

  public:
    SessionInfo(uint32_t sessionId) : mSessionId(sessionId) {
        ALOGI("SessionInfo created for session %d", mSessionId);
    }
    ~SessionInfo();
    void setHandle(uint64_t handle) { mHandle = handle; }

    void addSharedMemoryFdPairs(int input, int dupFd);
    // remove the Fd and return input fd for this.
    int removeSharedMemoryFdPairs(int dupFd);

    uint64_t getHandle() { return mHandle; }
    void connectSessionAif(uint32_t aifId, bool state);

    void forceCloseSession();
    void dump();
};

class CallbackInfo {
    uint32_t mSessionId;
    int mEventType;
    std::shared_ptr<IAGMCallback> mCallback;
    uint64_t mClientData;

  public:
    CallbackInfo(const std::shared_ptr<IAGMCallback> &callback, uint32_t sessionId, int eventType,
                 uint64_t clientData) {
        mCallback = callback;
        mSessionId = sessionId;
        mEventType = eventType;
        mClientData = clientData;
        // ALOGV("%s, callback %p session %d event %d clientData %llu", __func__, callback.get(),
        //       sessionId, eventType, clientData);
    }

    ~CallbackInfo() {
        // ALOGV("%s, callback %p session %d event %d clientData %llu", __func__, mCallback.get(),
        //       mSessionId, mEventType, mClientData);
    }
    uint32_t getSessionId() { return mSessionId; }
    int getEventType() { return mEventType; }
    uint64_t getClientData() { return mClientData; }
    std::shared_ptr<IAGMCallback> getCallback() { return mCallback; }
};

/*
* Interface for common session operations.
*/
class ISessionOps {
  public:
    virtual void addSessionHandle(uint32_t sessionId, uint64_t handle) = 0;
    virtual void removeSessionHandle(uint64_t handle) = 0;
    virtual void connectSessionAif(uint32_t sessionId, uint32_t aifId, bool state) = 0;
    virtual void addSharedMemoryFdPairs(uint64_t handle, int input, int dupFd) = 0;
    virtual int removeSharedMemoryFdPairs(uint32_t sessionId, int dupFd) = 0;
    virtual ~ISessionOps() = default;
};

class AgmServerWrapper;

class ClientInfo : public ISessionOps {
    std::vector<std::shared_ptr<CallbackInfo>> mCallbackInfo;

    // sessionId vs sessionId relatedInfo
    std::unordered_map<uint32_t, std::shared_ptr<SessionInfo>> mSessionsInfoMap;

    int mPid = 0;
    std::mutex mCallbackLock;
    std::mutex mSessionLock;
    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
    static AgmServerWrapper *sAgmServerWrapper;

  public:
    ClientInfo(int pid) : mPid(pid) {
        mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(
                AIBinder_DeathRecipient_new(ClientInfo::onDeath));
    }

    static void setAgmServerWrapper(AgmServerWrapper *wrapper);

    // this could only happen when client goes out of scope
    virtual ~ClientInfo() { cleanup(); }
    void cleanup();
    int getPid() { return mPid; }

    void registerCallback(const std::shared_ptr<IAGMCallback> &callback, int32_t in_sessionId,
                          int32_t in_eventType, int64_t in_clientData);
    void unregisterCallback(int32_t in_sessionId, int32_t in_eventType, int64_t in_clientData);

    std::shared_ptr<IAGMCallback> getCallback(int32_t in_sessionId, int32_t in_eventType,
                                              int64_t in_clientData);

    // must be called with lock held.
    // keeps the SessionInfo object for given sessionId.
    // If it does not exist, creates a sessionInfo
    // Currently added with addSessionHandle and
    // removed with removeSessionHandle
    std::shared_ptr<SessionInfo> getSessionInfo_l(int32_t in_sessionId);

    void clearCallbacks();
    void clearSessions();
    void addSessionHandle(uint32_t sessionId, uint64_t handle) override;
    void removeSessionHandle(uint64_t handle) override;
    void connectSessionAif(uint32_t sessionId, uint32_t aifId, bool state) override;

    void addSharedMemoryFdPairs(uint64_t handle, int input, int dupFd) override;
    // remove the Fd and return input fd for this.
    int removeSharedMemoryFdPairs(uint32_t sessionId, int dupFd) override;

    static void onDeath(void *cookie);
    void onDeath();

    static void onCallback(uint32_t sessionId, struct agm_event_cb_params *eventParams,
                           void *clientData);
};

class AgmServerWrapper : public BnAGM, public ISessionOps {
  public:
    explicit AgmServerWrapper();
    virtual ~AgmServerWrapper();
    bool isInitialized() { return mInitialized; }

    ::ndk::ScopedAStatus ipc_agm_init() override;
    ::ndk::ScopedAStatus ipc_agm_deinit() override;
    ::ndk::ScopedAStatus ipc_agm_session_start(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_stop(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_suspend(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_close(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_eos(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_flush(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_get_buf_info(
            int32_t in_sessionId, int32_t in_flag,
            ::aidl::vendor::qti::hardware::agm::MmapBufInfo *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_session_get_params(int32_t in_sessionId,
                                                    const std::vector<uint8_t> &in_buffer,
                                                    std::vector<uint8_t> *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_session_open(
            int32_t in_sessionId, ::aidl::vendor::qti::hardware::agm::AgmSessionMode in_sessionMode,
            int64_t *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_session_pause(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_prepare(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_read(int64_t in_handle, int32_t in_count,
                                              std::vector<uint8_t> *_aidl_return) override;

    ::ndk::ScopedAStatus ipc_agm_session_read_with_metadata(
            int64_t in_handle, const ::aidl::vendor::qti::hardware::agm::AgmBuff &in_buffer,
            int32_t in_capturedSize,
            ::aidl::vendor::qti::hardware::agm::IAGM::AgmReadWithMetadataReturn *_aidl_return)
            override;

    ::ndk::ScopedAStatus ipc_agm_session_register_callback(
            const std::shared_ptr<::aidl::vendor::qti::hardware::agm::IAGMCallback> &in_callback,
            int32_t in_sessionId, int32_t in_eventType, bool in_register,
            int64_t in_clientData) override;

    ::ndk::ScopedAStatus ipc_agm_session_register_for_events(
            int32_t in_sessionId,
            const ::aidl::vendor::qti::hardware::agm::AgmEventRegistrationConfig &in_evt_reg_cfg)
            override;
    ::ndk::ScopedAStatus ipc_agm_session_resume(int64_t in_handle) override;
    ::ndk::ScopedAStatus ipc_agm_session_set_config(
            int64_t in_handle,
            const ::aidl::vendor::qti::hardware::agm::AgmSessionConfig &in_sessionConfig,
            const ::aidl::vendor::qti::hardware::agm::AgmMediaConfig &in_mediaConfig,
            const ::aidl::vendor::qti::hardware::agm::AgmBufferConfig &in_bufferConfig) override;

    ::ndk::ScopedAStatus ipc_agm_session_set_ec_ref(int32_t in_sessionId, int32_t in_aifId,
                                                    bool in_state) override;
    ::ndk::ScopedAStatus ipc_agm_session_set_loopback(int32_t in_captureSessionId,
                                                      int32_t in_playbackSessionId,
                                                      bool in_state) override;
    ::ndk::ScopedAStatus ipc_agm_session_set_metadata(
            int32_t in_sessionId, const std::vector<uint8_t> &in_metadata) override;

    ::ndk::ScopedAStatus ipc_agm_session_set_non_tunnel_mode_config(
            int64_t in_handle,
            const ::aidl::vendor::qti::hardware::agm::AgmSessionConfig &in_sessionConfig,
            const ::aidl::vendor::qti::hardware::agm::AgmMediaConfig &in_inMediaConfig,
            const ::aidl::vendor::qti::hardware::agm::AgmMediaConfig &in_outMediaConfig,
            const ::aidl::vendor::qti::hardware::agm::AgmBufferConfig &in_inBufferConfig,
            const ::aidl::vendor::qti::hardware::agm::AgmBufferConfig &in_outBufferConfig) override;

    ::ndk::ScopedAStatus ipc_agm_session_set_params(
            int32_t in_sessionId, const std::vector<uint8_t> &in_payload) override;
    ::ndk::ScopedAStatus ipc_agm_session_write(int64_t in_handle,
                                               const std::vector<uint8_t> &in_buff,
                                               int32_t *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_session_write_datapath_params(
            int32_t in_sessionId,
            const ::aidl::vendor::qti::hardware::agm::AgmBuff &in_buff) override;
    ::ndk::ScopedAStatus ipc_agm_session_write_with_metadata(
            int64_t in_handle, const ::aidl::vendor::qti::hardware::agm::AgmBuff &in_buff,
            int32_t *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_aif_group_set_media_config(
            int32_t in_groupId,
            const ::aidl::vendor::qti::hardware::agm::AgmGroupMediaConfig &in_config) override;
    ::ndk::ScopedAStatus ipc_agm_aif_set_media_config(
            int32_t in_aifId,
            const ::aidl::vendor::qti::hardware::agm::AgmMediaConfig &in_config) override;
    ::ndk::ScopedAStatus ipc_agm_aif_set_metadata(int32_t in_aifId,
                                                  const std::vector<uint8_t> &in_metadata) override;
    ::ndk::ScopedAStatus ipc_agm_aif_set_params(int32_t in_aifId,
                                                const std::vector<uint8_t> &in_payload) override;
    ::ndk::ScopedAStatus ipc_agm_session_aif_connect(int32_t in_sessionId, int32_t in_aifId,
                                                     bool in_state) override;
    ::ndk::ScopedAStatus ipc_agm_session_aif_get_tag_module_info(
            int32_t in_sessionId, int32_t in_aifId, int32_t in_size,
            std::vector<uint8_t> *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_session_aif_set_cal(
            int32_t in_sessionId, int32_t in_aifId,
            const ::aidl::vendor::qti::hardware::agm::AgmCalConfig &in_calConfig) override;
    ::ndk::ScopedAStatus ipc_agm_session_aif_set_metadata(
            int32_t in_sessionId, int32_t in_aifId,
            const std::vector<uint8_t> &in_metadata) override;
    ::ndk::ScopedAStatus ipc_agm_session_aif_set_params(
            int32_t in_sessionId, int32_t in_aifId,
            const std::vector<uint8_t> &in_payload) override;
    ::ndk::ScopedAStatus ipc_agm_get_aif_info_list(
            int32_t in_numAifInfo,
            std::vector<::aidl::vendor::qti::hardware::agm::AifInfo> *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_get_buffer_timestamp(int32_t in_sessiondId,
                                                      int64_t *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_get_group_aif_info_list(
            int32_t in_numberOfGroups,
            std::vector<::aidl::vendor::qti::hardware::agm::AifInfo> *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_get_hw_processed_buff_cnt(
            int64_t in_handle, ::aidl::vendor::qti::hardware::agm::Direction in_direction) override;
    ::ndk::ScopedAStatus ipc_agm_get_params_from_acdb_tunnel(
            const std::vector<uint8_t> &in_payload, std::vector<uint8_t> *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_get_session_time(int64_t in_handle,
                                                  int64_t *_aidl_return) override;
    ::ndk::ScopedAStatus ipc_agm_sessionid_flush(int32_t in_sessiondId) override;
    ::ndk::ScopedAStatus ipc_agm_set_gapless_session_metadata(
            int64_t in_handle, ::aidl::vendor::qti::hardware::agm::AgmGaplessSilenceType in_type,
            int32_t in_silence) override;
    ::ndk::ScopedAStatus ipc_agm_set_params_to_acdb_tunnel(
            const std::vector<uint8_t> &in_payload) override;
    ::ndk::ScopedAStatus ipc_agm_set_params_with_tag(
            int32_t in_sessiondId, int32_t in_aifId,
            const ::aidl::vendor::qti::hardware::agm::AgmTagConfig &in_tagConfig) override;
    ::ndk::ScopedAStatus ipc_agm_set_params_with_tag_to_acdb(
            int32_t in_sessiondId, int32_t in_aifId,
            const std::vector<uint8_t> &in_payload) override;
    ::ndk::ScopedAStatus ipc_agm_dump(
            const ::aidl::vendor::qti::hardware::agm::AgmDumpInfo &in_dumpInfo)
            override;

    void addSessionHandle(uint32_t sessionId, uint64_t handle) override;
    void removeSessionHandle(uint64_t handle) override;
    void connectSessionAif(uint32_t sessionId, uint32_t aifId, bool state) override;

    void addSharedMemoryFdPairs(uint64_t handle, int input, int dupFd) override;
    // remove the Fd and return input fd for this.
    int removeSharedMemoryFdPairs(uint32_t sessionId, int dupFd) override;

    // it returns the client as per caller pid, must be called with lock held
    std::shared_ptr<ClientInfo> getClient_l();
    void removeClient(int pid);

    std::mutex mLock;
    // pid vs clientInfo
    std::unordered_map<int /*pid */, std::shared_ptr<ClientInfo>> mClients;
    bool mInitialized = false;
};
}