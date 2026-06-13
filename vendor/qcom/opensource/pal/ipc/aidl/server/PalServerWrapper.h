/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <aidl/vendor/qti/hardware/pal/BnPAL.h>
#include <aidl/vendor/qti/hardware/pal/IPALCallback.h>
#include <fmq/AidlMessageQueue.h>
#include <fmq/EventFlag.h>
#include <log/log.h>
#include <utils/Thread.h>
#include <mutex>
#include <unordered_map>
#include "PalApi.h"

using ::android::AidlMessageQueue;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::hardware::EventFlag;
using ::android::Thread;

namespace aidl::vendor::qti::hardware::pal {

class StreamInfo {
    int64_t mHandle = 0;
    std::mutex mLock;

    using FdPair = std::pair<int, int>;
    std::vector<FdPair> mInOutFdPairs;

  public:
    StreamInfo(int64_t handle) : mHandle(handle) {
        ALOGI("StreamInfo created for handle %llx", mHandle);
    }
    ~StreamInfo();

    void addSharedMemoryFdPairs(int input, int dupFd);
    // remove the Fd and return input fd for this.
    int removeSharedMemoryFdPairs(int dupFd);
    void closeSharedMemoryFdPairs();
    void forceCloseStream();
};

class CallbackInfo {
    typedef ::android::AidlMessageQueue<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;
    typedef ::android::AidlMessageQueue<
            PalReadWriteDoneCommand, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            CommandMQ;

  public:
    int mEventType;
    int64_t mClientData;
    int64_t mHandle;
    std::unique_ptr<DataMQ> mDataMQ = nullptr;
    std::unique_ptr<CommandMQ> mCommandMQ = nullptr;
    EventFlag *mEfGroup = nullptr;
    std::shared_ptr<IPALCallback> mCallback;
    struct pal_stream_attributes mStreamAttributes;
    CallbackInfo(const std::shared_ptr<IPALCallback> &callback, int64_t clientData) {
        mCallback = callback;
        mClientData = clientData;
        ALOGV("%s, callback %p handle %llx clientData %llu", __func__, callback.get(), clientData);
    }

    ~CallbackInfo() {
        ALOGV("%s, callback %p handle %llx clientData %llu", __func__, mCallback.get(), mHandle,
              mClientData);
        if (mEfGroup) {
            EventFlag::deleteEventFlag(&mEfGroup);
        }
    }
    void setStreamAttr(struct pal_stream_attributes *attr) {
        memcpy(&mStreamAttributes, attr, sizeof(mStreamAttributes));
    }
    void setHandle(int64_t handle) { mHandle = handle; }
    int32_t callReadWriteTransferThread(PalReadWriteDoneCommand cmd, const int8_t *data,
                                        size_t dataSize);
    int32_t prepareMQForTransfer(int64_t handle, int64_t cookie);
    int64_t getStreamHandle() { return mHandle; }
    int64_t getClientData() { return mClientData; }
    std::shared_ptr<IPALCallback> getCallback() { return mCallback; }
};

/*
* Interface for common stream operations.
*/
class IStreamOps {
  public:
    virtual void addStreamHandle(int64_t handle) = 0;
    virtual void removeStreamHandle(int64_t handle) = 0;
    virtual void addSharedMemoryFdPairs(int64_t handle, int input, int dupFd) = 0;
    virtual int removeSharedMemoryFdPairs(int64_t handle, int dupFd) = 0;
    virtual bool isValidStreamHandle(int64_t handle) = 0;
    virtual ~IStreamOps() = default;
};

class PalServerWrapper;

class ClientInfo : public IStreamOps {
    std::vector<std::shared_ptr<CallbackInfo>> mCallbackInfo;

    int mPid = 0;
    std::mutex mCallbackLock;
    std::mutex mStreamLock;
    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
    static PalServerWrapper *sPalServerWrapper;

  public:
    // Handle vs streamInfo related Info
    std::unordered_map<int64_t, std::shared_ptr<StreamInfo>> mStreamInfoMap;
    ClientInfo(int pid) : mPid(pid) {
        mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(
                AIBinder_DeathRecipient_new(ClientInfo::onDeath));
    }

    static void setPalServerWrapper(PalServerWrapper *wrapper);
    int getPid() { return mPid; }
    // this could only happen when client goes out of scope
    virtual ~ClientInfo() { cleanup(); }
    void cleanup();
    // must be called with lock held.
    // keeps the StreamInfo object for given stream handle.
    // If it does not exist, creates a streamInfo
    // Currently added with addStreamHandle and
    // removed with removeStreamHandle
    std::shared_ptr<StreamInfo> getStreamInfo_l(int64_t handle);

    void clearCallbacks();
    void clearStreams();
    void addStreamHandle(int64_t handle);
    void removeStreamHandle(int64_t handle);
    void addSharedMemoryFdPairs(int64_t handle, int input, int dupFd) override;
    // remove the Fd and return input fd for this.
    int removeSharedMemoryFdPairs(int64_t handle, int dupFd) override;
    bool isValidStreamHandle(int64_t handle);
    void closeSharedMemoryFdPairs(int64_t handle);
    void registerCallback(int64_t handle, const std::shared_ptr<IPALCallback> &callback,
                          std::shared_ptr<CallbackInfo> callBackInfo);
    void unregisterCallback(int64_t handle);
    static void onDeath(void *cookie);
    void onDeath();
    void getStreamMediaConfig(int64_t handle, pal_media_config *config);
    static int32_t onCallback(pal_stream_handle_t *handle, uint32_t eventId, uint32_t *eventData,
                              uint32_t eventDataSize, uint64_t cookie);
};

class PalServerWrapper : public BnPAL, public IStreamOps {
  public:
    virtual ~PalServerWrapper() {}
    ::ndk::ScopedAStatus ipc_pal_stream_open(const PalStreamAttributes &attributes,
                                             const std::vector<PalDevice> &devices,
                                             const std::vector<ModifierKV> &modifiers,
                                             const std::shared_ptr<IPALCallback> &cb,
                                             int64_t ipcClientData, int64_t *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_close(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_start(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_stop(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_pause(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_suspend(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_resume(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_flush(const int64_t handle) override;
    ::ndk::ScopedAStatus ipc_pal_stream_drain(const int64_t handle,
                                              const PalDrainType type) override;
    ::ndk::ScopedAStatus ipc_pal_stream_set_buffer_size(
            const int64_t handle, const PalBufferConfig &rxAidlBufCfg,
            const PalBufferConfig &txAidlBufCfg, std::vector<PalBufferConfig> *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_buffer_size(const int64_t handle, int32_t inBufSize,
                                                        int32_t outBufSize,
                                                        int32_t *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_write(const int64_t handle,
                                              const std::vector<PalBuffer> &buffer,
                                              int32_t *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_read(const int64_t handle,
                                             const std::vector<PalBuffer> &buffer,
                                             PalReadReturnData *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_set_param(const int64_t handle, int32_t paramId,
                                                  const PalParamPayloadShmem &payload) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_param(const int64_t handle, int32_t param_id,
                                                  PalParamPayload *paramPayload) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_device(const int64_t handle,
                                                   std::vector<PalDevice> *devs) override;
    ::ndk::ScopedAStatus ipc_pal_stream_set_device(const int64_t handle,
                                                   const std::vector<PalDevice> &devs) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_volume(const int64_t handle,
                                                   PalVolumeData *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_set_volume(const int64_t handle,
                                                   const PalVolumeData &aidlVol) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_mute(const int64_t handle, bool *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_set_mute(const int64_t handle, bool state) override;
    ::ndk::ScopedAStatus ipc_pal_get_mic_mute(bool *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_set_mic_mute(bool state) override;
    ::ndk::ScopedAStatus ipc_pal_get_timestamp(const int64_t handle,
                                               PalSessionTime *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_add_remove_effect(const int64_t handle, PalAudioEffect effect,
                                                   bool enable) override;
    ::ndk::ScopedAStatus ipc_pal_set_param(int32_t paramId,
                                           const std::vector<uint8_t> &payload) override;
    ::ndk::ScopedAStatus ipc_pal_get_param(int32_t paramId,
                                           std::vector<uint8_t> *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_create_mmap_buffer(const int64_t handle,
                                                           int32_t minSizeFrames,
                                                           PalMmapBuffer *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_mmap_position(const int64_t handle,
                                                          PalMmapPosition *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_register_global_callback(const std::shared_ptr<IPALCallback> &cb,
                                                          int64_t cookie) override;
    ::ndk::ScopedAStatus ipc_pal_gef_rw_param(int32_t paramId,
                                              const std::vector<uint8_t> &paramPayload,
                                              PalDeviceId devId, PalStreamType streamType,
                                              int8_t dir,
                                              std::vector<uint8_t> *aidlReturn) override;
    ::ndk::ScopedAStatus ipc_pal_stream_get_tags_with_module_info(
            const int64_t handle, int32_t size, std::vector<uint8_t> *aidlReturn) override;
    void addStreamHandle(int64_t handle) override;
    void removeStreamHandle(int64_t handle) override;

    void addSharedMemoryFdPairs(int64_t handle, int input, int dupFd) override;
    // remove the Fd and return input fd for this.
    int removeSharedMemoryFdPairs(int64_t handle, int dupFd) override;
    bool isValidStreamHandle(int64_t handle) override;

    // it returns the client as per caller pid, must be called with lock held
    std::shared_ptr<ClientInfo> getClient_l();
    void removeClient(int pid);
    void removeClient_l(int pid);
    void removeClientInfoData(int64_t handle);

    std::mutex mLock;
    // pid vs clientInfo
    std::unordered_map<int /*pid */, std::shared_ptr<ClientInfo>> mClients;
};
}
