/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <StreamWorker.h>
#include <Utils.h>
#include <aidl/android/hardware/audio/common/SinkMetadata.h>
#include <aidl/android/hardware/audio/common/SourceMetadata.h>
#include <aidl/android/hardware/audio/core/BnStreamCommon.h>
#include <aidl/android/hardware/audio/core/BnStreamIn.h>
#include <aidl/android/hardware/audio/core/BnStreamOut.h>
#include <aidl/android/hardware/audio/core/IStreamCallback.h>
#include <aidl/android/hardware/audio/core/IStreamOutEventCallback.h>
#include <aidl/android/hardware/audio/core/StreamDescriptor.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <aidl/android/media/audio/common/AudioIoFlags.h>
#include <aidl/android/media/audio/common/AudioOffloadInfo.h>
#include <aidl/android/media/audio/common/MicrophoneInfo.h>
#include <extensions/AudioExtension.h>
#include <error/expected_utils.h>
#include <fmq/AidlMessageQueue.h>
#include <system/thread_defs.h>
#include <utils/Errors.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <memory>
#include <optional>
#include <variant>

#include <Utils.h>
#include <qti-audio-core/ChildInterface.h>
#include <qti-audio-core/Platform.h>
#include <qti-audio-core/Utils.h>

#ifdef SEC_AUDIO_SAMSUNGRECORD
#include "AudioPreProcess.h"
#endif

namespace qti::audio::core {

class Telephony;

// This class is similar to StreamDescriptor, but unlike
// the descriptor, it actually owns the objects implementing
// data exchange: FMQs etc, whereas StreamDescriptor only
// contains their descriptors.
class StreamContext {
  public:
    typedef ::android::AidlMessageQueue<
            ::aidl::android::hardware::audio::core::StreamDescriptor::Command,
            ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            CommandMQ;
    typedef ::android::AidlMessageQueue<
            ::aidl::android::hardware::audio::core::StreamDescriptor::Reply,
            ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            ReplyMQ;
    typedef ::android::AidlMessageQueue<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    // Ensure that this value is not used by any of
    // ::aidl::android::hardware::audio::core::StreamDescriptor.State enums
    static constexpr int32_t STATE_CLOSED = -1;

    struct DebugParameters {
        // An extra delay for transient states, in ms.
        int transientStateDelayMs = 0;
        // Force the "burst" command to move the SM to the TRANSFERRING state.
        bool forceTransientBurst = false;
        // Force the "drain" command to be synchronous, going directly to the
        // IDLE state.
        bool forceSynchronousDrain = false;
    };

    StreamContext() = default;
    StreamContext(
            std::unique_ptr<CommandMQ> commandMQ, std::unique_ptr<ReplyMQ> replyMQ,
            const ::aidl::android::media::audio::common::AudioFormatDescription& format,
            const ::aidl::android::media::audio::common::AudioChannelLayout& channelLayout,
            int sampleRate, std::unique_ptr<DataMQ> dataMQ,
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCallback> asyncCallback,
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamOutEventCallback>
                    outEventCallback,
            ::aidl::android::media::audio::common::AudioPortConfig mixPortConfig,
            DebugParameters debugParameters, const int nominalLatency,
            std::weak_ptr<Telephony> telephony)
        : mCommandMQ(std::move(commandMQ)),
          mInternalCommandCookie(std::rand()),
          mReplyMQ(std::move(replyMQ)),
          mFormat(format),
          mChannelLayout(channelLayout),
          mSampleRate(sampleRate),
          mDataMQ(std::move(dataMQ)),
          mAsyncCallback(asyncCallback),
          mOutEventCallback(outEventCallback),
          mMixPortConfig(mixPortConfig),
          mNominalLatency(nominalLatency),
          mDebugParameters(debugParameters),
          mTelephony(telephony) {}
    StreamContext(StreamContext&& other)
        : mCommandMQ(std::move(other.mCommandMQ)),
          mInternalCommandCookie(other.mInternalCommandCookie),
          mReplyMQ(std::move(other.mReplyMQ)),
          mFormat(other.mFormat),
          mChannelLayout(other.mChannelLayout),
          mSampleRate(other.mSampleRate),
          mDataMQ(std::move(other.mDataMQ)),
          mAsyncCallback(std::move(other.mAsyncCallback)),
          mOutEventCallback(std::move(other.mOutEventCallback)),
          mMixPortConfig(std::move(other.mMixPortConfig)),
          mDebugParameters(std::move(other.mDebugParameters)),
          mFrameCount(other.mFrameCount),
          mNominalLatency(other.mNominalLatency),
          mTelephony(other.mTelephony) {}
    StreamContext& operator=(StreamContext&& other) {
        mCommandMQ = std::move(other.mCommandMQ);
        mInternalCommandCookie = other.mInternalCommandCookie;
        mReplyMQ = std::move(other.mReplyMQ);
        mFormat = std::move(other.mFormat);
        mChannelLayout = std::move(other.mChannelLayout);
        mSampleRate = other.mSampleRate;
        mDataMQ = std::move(other.mDataMQ);
        mAsyncCallback = std::move(other.mAsyncCallback);
        mOutEventCallback = std::move(other.mOutEventCallback);
        mMixPortConfig = std::move(other.mMixPortConfig);
        mDebugParameters = std::move(other.mDebugParameters);
        mFrameCount = other.mFrameCount;
        mNominalLatency =  other.mNominalLatency;
        mTelephony = other.mTelephony;
        return *this;
    }

    void fillDescriptor(::aidl::android::hardware::audio::core::StreamDescriptor* desc);
    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCallback> getAsyncCallback()
            const {
        return mAsyncCallback;
    }
    size_t getBufferSizeInFrames() const;
    ::aidl::android::media::audio::common::AudioChannelLayout getChannelLayout() const {
        return mChannelLayout;
    }
    CommandMQ* getCommandMQ() const { return mCommandMQ.get(); }
    DataMQ* getDataMQ() const { return mDataMQ.get(); }
    ::aidl::android::media::audio::common::AudioFormatDescription getFormat() const {
        return mFormat;
    }
    ::aidl::android::media::audio::common::AudioIoFlags getFlags() const {
        return mMixPortConfig.flags.value();
    }
#ifdef SEC_AUDIO_SAMSUNGRECORD
    void setFlags(::aidl::android::media::audio::common::AudioIoFlags flags) {
        mMixPortConfig.flags.value() = flags;
    }
#endif
    bool getForceTransientBurst() const { return mDebugParameters.forceTransientBurst; }
    bool getForceSynchronousDrain() const { return mDebugParameters.forceSynchronousDrain; }
    size_t getFrameSize() const;
    int getInternalCommandCookie() const { return mInternalCommandCookie; }
    int32_t getMixPortHandle() const {
        return mMixPortConfig.ext.get<::aidl::android::media::audio::common::AudioPortExt::mix>()
                .handle;
    }
    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamOutEventCallback>
            getOutEventCallback() const {
        return mOutEventCallback;
    }
    int getPortId() const { return mMixPortConfig.portId; }
    ReplyMQ* getReplyMQ() const { return mReplyMQ.get(); }
    int getTransientStateDelayMs() const { return mDebugParameters.transientStateDelayMs; }
    int getSampleRate() const { return mSampleRate; }
    bool isValid() const;
    // 'reset' is called on a Binder thread when closing the stream. Does not use
    // locking because it only cleans MQ pointers which were also set on the Binder thread.
    void reset();
    // 'advanceFrameCount' and 'getFrameCount' are only called on the worker thread.
    long advanceFrameCount(size_t increase) { return mFrameCount += increase; }
    long getFrameCount() const { return mFrameCount; }
    const ::aidl::android::media::audio::common::AudioPortConfig& getMixPortConfig() const {
        return mMixPortConfig;
    }
    int32_t getNominalLatencyMs() const { return mNominalLatency; }
    std::weak_ptr<Telephony> getTelephony() { return mTelephony; }

  private:
    std::unique_ptr<CommandMQ> mCommandMQ;
    int mInternalCommandCookie; // The value used to confirm that the command
                                // was posted internally
    std::unique_ptr<ReplyMQ> mReplyMQ;
    ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    int mSampleRate;
    ::aidl::android::media::audio::common::AudioPortConfig mMixPortConfig;
    std::unique_ptr<DataMQ> mDataMQ;
    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCallback> mAsyncCallback;
    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamOutEventCallback>
            mOutEventCallback; // Only used by output streams
    DebugParameters mDebugParameters;
    long mFrameCount = 0;
    int32_t mNominalLatency = 0;
    std::weak_ptr<Telephony> mTelephony;
};

// This interface provides operations of the stream which are executed on the worker thread.
struct DriverInterface {
    virtual ~DriverInterface() = default;
    // All the methods below are called on the worker thread.
    // This function is only called once.
    virtual ::android::status_t init() = 0;
    virtual ::android::status_t drain(
            ::aidl::android::hardware::audio::core::StreamDescriptor::DrainMode mode) = 0;
    virtual ::android::status_t flush() = 0;
    virtual ::android::status_t pause() = 0;
    virtual ::android::status_t standby() = 0;
    virtual ::android::status_t start() = 0;
    virtual ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                         int32_t* latencyMs) = 0;
    // No need to implement 'refinePosition' unless the driver can provide more precise
    // data than just total frame count. For example, the driver may correctly account
    // for any intermediate buffers.
    virtual ::android::status_t refinePosition(
            ::aidl::android::hardware::audio::core::StreamDescriptor::Reply* /*reply*/) {
        return ::android::OK;
    }

    // This function is only called once.
    // Implementers must provide implementation to shutdown the platform resources
    virtual void shutdown() { return;}
};

class StreamWorkerCommonLogic : public ::android::hardware::audio::common::StreamLogic {
  public:
    bool isClosed() const {
        return static_cast<int32_t>(mState.load()) == StreamContext::STATE_CLOSED;
    }
    void setClosed() {
        mState = static_cast<::aidl::android::hardware::audio::core::StreamDescriptor::State>(
                StreamContext::STATE_CLOSED);
    }
    void setIsConnected(bool connected) { mIsConnected = connected; }

    /**
     * IStreamCallback equivalents for StreamWorker
     **/
    virtual void publishTransferReady() {}
    virtual void publishDrainReady() {}
    virtual void publishError() {}

  protected:
    using DataBufferElement = int8_t;

    StreamWorkerCommonLogic(StreamContext* context, DriverInterface* driver)
        : mContext(context),
          mDriver(driver),
          mTransientStateDelayMs(context->getTransientStateDelayMs()) {}
    pid_t getTid() const;
    std::string init() override;
    void populateReply(::aidl::android::hardware::audio::core::StreamDescriptor::Reply* reply,
                       bool isConnected) const;
    void populateReplyWrongState(
            ::aidl::android::hardware::audio::core::StreamDescriptor::Reply* reply,
            const ::aidl::android::hardware::audio::core::StreamDescriptor::Command& command) const;
    void switchToTransientState(
            ::aidl::android::hardware::audio::core::StreamDescriptor::State state) {
        mState = state;
        mTransientStateStart = std::chrono::steady_clock::now();
    }

    // The context is only used for reading, except for updating the frame count,
    // which happens on the worker thread only.
    StreamContext* const mContext;
    DriverInterface* const mDriver;
    // Atomic fields are used both by the main and worker threads.
    std::atomic<bool> mIsConnected = false;
    static_assert(std::atomic<::aidl::android::hardware::audio::core::StreamDescriptor::State>::
                          is_always_lock_free);
    std::atomic<::aidl::android::hardware::audio::core::StreamDescriptor::State> mState =
            ::aidl::android::hardware::audio::core::StreamDescriptor::State::STANDBY;
    std::atomic<::aidl::android::hardware::audio::core::StreamDescriptor::DrainMode>
        mRecentDrainMode =
        ::aidl::android::hardware::audio::core::StreamDescriptor::DrainMode::DRAIN_UNSPECIFIED;
    // All fields below are used on the worker thread only.
    const std::chrono::duration<int, std::milli> mTransientStateDelayMs;
    std::chrono::time_point<std::chrono::steady_clock> mTransientStateStart;
    // We use an array and the "size" field instead of a vector to be able to
    // detect memory allocation issues.
    std::unique_ptr<DataBufferElement[]> mDataBuffer;
    size_t mDataBufferSize;
    /**
     * only used in Asynchrous Stream(In|Out) context, to synchronize the
     * callbacks from the hardware.
     * Hardware sends callback any time irrespective of the Stream State.
     * Hence the synchronization.
     **/
    std::mutex mAsyncMutex;
    enum StreamCallbackType {
        TR = 1,  // TransferReady
        DR = 2,  // DrainReady
        ER = 3,  // Error
    };
    std::optional<StreamCallbackType> mPendingCallBack = std::nullopt;
    std::condition_variable mPendingCV;
};

// This interface is used to decouple stream implementations from a concrete
// StreamWorker implementation.
struct StreamWorkerInterface {
    using CreateInstance =
            std::function<StreamWorkerInterface*(StreamContext* context, DriverInterface* driver)>;
    virtual ~StreamWorkerInterface() = default;
    virtual bool isClosed() const = 0;
    virtual void setIsConnected(bool isConnected) = 0;
    virtual void setClosed() = 0;
    virtual bool start() = 0;
    virtual pid_t getTid() = 0;
    virtual void join() = 0;
    virtual void publishTransferReady() = 0;
    virtual void publishDrainReady() = 0;
    virtual void publishError() = 0;
};

template <class WorkerLogic>
class StreamWorkerImpl : public StreamWorkerInterface,
                         public ::android::hardware::audio::common::StreamWorker<WorkerLogic> {
    using WorkerImpl = ::android::hardware::audio::common::StreamWorker<WorkerLogic>;

  public:
    StreamWorkerImpl(StreamContext* context, DriverInterface* driver)
        : WorkerImpl(context, driver) {}
    bool isClosed() const override { return WorkerImpl::isClosed(); }
    void setIsConnected(bool isConnected) override { WorkerImpl::setIsConnected(isConnected); }
    void setClosed() override { WorkerImpl::setClosed(); }
    bool start() override {
        // This is an "audio service thread," must have elevated priority.
        return WorkerImpl::start(WorkerImpl::kThreadName, ANDROID_PRIORITY_URGENT_AUDIO);
    }
    pid_t getTid() override { return WorkerImpl::getTid(); }
    void join() override { return WorkerImpl::join(); }
    void publishTransferReady() override { return WorkerImpl::publishTransferReady(); };
    void publishDrainReady() override { return WorkerImpl::publishDrainReady(); }
    void publishError() override { return WorkerImpl::publishError(); }
};

class StreamInWorkerLogic : public StreamWorkerCommonLogic {
  public:
    static const std::string kThreadName;
    StreamInWorkerLogic(StreamContext* context, DriverInterface* driver)
        : StreamWorkerCommonLogic(context, driver) {}

  protected:
    Status cycle() override;

  private:
    bool read(size_t clientSize,
              ::aidl::android::hardware::audio::core::StreamDescriptor::Reply* reply);
};
using StreamInWorker = StreamWorkerImpl<StreamInWorkerLogic>;

class StreamOutWorkerLogic : public StreamWorkerCommonLogic {
  public:
    static const std::string kThreadName;
    StreamOutWorkerLogic(StreamContext* context, DriverInterface* driver)
        : StreamWorkerCommonLogic(context, driver),
          mEventCallback(context->getOutEventCallback()) {}
    void publishTransferReady() override;
    void publishDrainReady() override;
    void publishError() override;

  protected:
    Status cycle() override;

  private:
    bool write(size_t clientSize,
               ::aidl::android::hardware::audio::core::StreamDescriptor::Reply* reply);

    std::shared_ptr<::aidl::android::hardware::audio::core::IStreamOutEventCallback> mEventCallback;
};
using StreamOutWorker = StreamWorkerImpl<StreamOutWorkerLogic>;

// This interface provides operations of the stream which are executed on a Binder pool thread.
// These methods originate both from the AIDL interface and its implementation.
struct StreamCommonInterface {
    using ConnectedDevices = std::vector<::aidl::android::media::audio::common::AudioDevice>;
    using Metadata =
            std::variant<::aidl::android::hardware::audio::common::SinkMetadata /*IStreamIn*/,
                         ::aidl::android::hardware::audio::common::SourceMetadata /*IStreamOut*/>;

    static constexpr bool isInput(const Metadata& metadata) { return metadata.index() == 0; }

    virtual ~StreamCommonInterface() = default;
    // Methods below originate from the 'IStreamCommon' interface.
    // This is semantically equivalent to inheriting from 'IStreamCommon' with a benefit
    // that concrete stream implementations can inherit both from this interface and IStreamIn/Out.
    virtual ndk::ScopedAStatus close() = 0;
    virtual ndk::ScopedAStatus prepareToClose() = 0;
    virtual ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) = 0;
    virtual ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return) = 0;
    virtual ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async) = 0;
    virtual ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>&
                    in_effect) = 0;
    virtual ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>&
                    in_effect) = 0;

    // Methods below are common for both 'IStreamIn' and 'IStreamOut'. Note that
    // 'updateMetadata' in them uses an individual structure which is wrapped here.
    // The 'Common' suffix is added to distinguish them from the methods from 'IStreamIn/Out'.
    virtual ndk::ScopedAStatus getStreamCommonCommon(
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon>*
                    _aidl_return) = 0;
    virtual ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) = 0;
    virtual Metadata getMetadataCommon() = 0;

    // Methods below are called by implementation of 'IModule', 'IStreamIn' and 'IStreamOut'.
    virtual ndk::ScopedAStatus initInstance(
            const std::shared_ptr<StreamCommonInterface>& delegate) = 0;
    virtual const StreamContext& getContext() const = 0;
    virtual bool isClosed() const = 0;
    virtual const ConnectedDevices& getConnectedDevices() const = 0;
    virtual ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) = 0;
    /**
     * API to configure the connected devices based on the latest platform configuration
     * Example: whenever there is a HAC enabled on the platform, we would want to reconfigure
     * VOIP playback stream with HAC enabled Handset speaker.
     */
    virtual ndk::ScopedAStatus reconfigureConnectedDevices() = 0;
    virtual ndk::ScopedAStatus configureMMapStream(int32_t* fd, int64_t* burstSizeFrames,
                                                   int32_t* flags, int32_t* bufferSizeFrames) = 0;
    virtual void setStreamMicMute(const bool muted) = 0;
#ifdef SEC_AUDIO_COMMON
    virtual ndk::ScopedAStatus ForceSetDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            bool force = false) = 0;
    virtual bool isStreamUsecase(Usecase tag) = 0;
    virtual bool isDeviceAvailable(pal_device_id_t pal_device) = 0;
    virtual bool HasPalStreamHandle() = 0;
    virtual void forceShutdown() = 0;
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    virtual void RerouteForInterpreter() = 0;
#endif
};

// This is equivalent to automatically generated 'IStreamCommonDelegator' but uses
// a weak pointer to avoid creating a reference loop. The loop will occur because
// 'IStreamIn/Out.getStreamCommon' must return the same instance every time, thus
// the stream implementation must hold a strong pointer to an instance of 'IStreamCommon'.
// Also, we use 'StreamCommonInterface' here instead of 'IStreamCommon'.
class StreamCommonDelegator : public ::aidl::android::hardware::audio::core::BnStreamCommon {
  public:
    explicit StreamCommonDelegator(const std::shared_ptr<StreamCommonInterface>& delegate)
        : mDelegate(delegate) {}

  private:
    ndk::ScopedAStatus close() override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->close()
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus prepareToClose() override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->prepareToClose()
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->updateHwAvSyncId(in_hwAvSyncId)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return)
            override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->getVendorParameters(in_ids, _aidl_return)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async) override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->setVendorParameters(in_parameters, in_async)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->addEffect(in_effect)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override {
        auto delegate = mDelegate.lock();
        return delegate != nullptr ? delegate->removeEffect(in_effect)
                                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    // It is possible that on the client side the proxy for IStreamCommon will
    // outlive the IStream* instance, and the server side IStream* instance will
    // get destroyed while this IStreamCommon instance is still alive.
    std::weak_ptr<StreamCommonInterface> mDelegate;
};

// The implementation of DriverInterface must be provided by each concrete stream implementation.
// Note that StreamCommonImpl does not own the context. This is to support swapping on the fly
// implementations of the stream while keeping the same IStreamIn/Out instance. It's that instance
// who must be owner of the context.
class StreamCommonImpl : virtual public StreamCommonInterface, virtual public DriverInterface {
  public:
    StreamCommonImpl(StreamContext* context, const Metadata& metadata,
                     const StreamWorkerInterface::CreateInstance& createWorker)
        : mContextRef(*context), mMetadata(metadata), mWorker(createWorker(context, this)) {}
    StreamCommonImpl(StreamContext* context, const Metadata& metadata)
        : StreamCommonImpl(context, metadata, isInput(metadata) ? getDefaultInWorkerCreator()
                                                                : getDefaultOutWorkerCreator()) {}
    virtual ~StreamCommonImpl() override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus prepareToClose() override;
    ndk::ScopedAStatus updateHwAvSyncId(int32_t in_hwAvSyncId) override;
    ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return)
            override;
    ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async) override;
    ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;

    ndk::ScopedAStatus getStreamCommonCommon(
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon>* _aidl_return)
            override;
    ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) override;
    Metadata getMetadataCommon() { return mMetadata; }

    ndk::ScopedAStatus initInstance(
            const std::shared_ptr<StreamCommonInterface>& delegate) override;
    const StreamContext& getContext() const override { return mContextRef; }
    bool isClosed() const override { return mWorker->isClosed(); }
    const ConnectedDevices& getConnectedDevices() const override { return mConnectedDevices; }
    ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            override;
    void setStreamMicMute(const bool muted) override;
    ndk::ScopedAStatus configureMMapStream(int32_t* fd, int64_t* burstSizeFrames, int32_t* flags,
                                           int32_t* bufferSizeFrames) override;
#ifdef SEC_AUDIO_COMMON
    ndk::ScopedAStatus ForceSetDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            bool force = false) override;
    bool isStreamUsecase(Usecase tag) override;
    bool isDeviceAvailable(pal_device_id_t pal_device) override;
    bool HasPalStreamHandle() override;
    void forceShutdown() override;
    bool isCustomKeyRouted(const int ck_id);
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    void RerouteForInterpreter() override;
#endif

    // start of Equivalent of IStreamCallbacks
    void publishTransferReady() { mWorker->publishTransferReady(); }

    void publishDrainReady() { mWorker->publishDrainReady(); }

    void publishError() { mWorker->publishError(); }
    // end of Equivalent of IStreamCallbacks

  protected:
    static StreamWorkerInterface::CreateInstance getDefaultInWorkerCreator() {
        return [](StreamContext* ctx, DriverInterface* driver) -> StreamWorkerInterface* {
            return new StreamInWorker(ctx, driver);
        };
    }
    static StreamWorkerInterface::CreateInstance getDefaultOutWorkerCreator() {
        return [](StreamContext* ctx, DriverInterface* driver) -> StreamWorkerInterface* {
            return new StreamOutWorker(ctx, driver);
        };
    }
    virtual void onClose() = 0;
    void stopWorker();

    const StreamContext& mContextRef;
    Metadata mMetadata;
    std::unique_ptr<StreamWorkerInterface> mWorker;
    ChildInterface<StreamCommonDelegator> mCommon;
    ConnectedDevices mConnectedDevices;
#ifdef SEC_AUDIO_COMMON
    ConnectedDevices mPreviousDevices;
    std::vector<pal_device> mPalDevices{};
#endif
};

// Note: 'StreamIn/Out' can not be used on their own. Instead, they must be used for defining
// concrete input/output stream implementations.
class StreamIn : virtual public StreamCommonInterface,
                 public ::aidl::android::hardware::audio::core::BnStreamIn {
  public:
    virtual ~StreamIn() override = default;
    ndk::ScopedAStatus getMetadata(
            ::aidl::android::hardware::audio::common::SinkMetadata& out_sinkMetadata) {
        out_sinkMetadata = std::get<::aidl::android::hardware::audio::common::SinkMetadata>(
                getMetadataCommon());
        return ndk::ScopedAStatus::ok();
    }
    virtual int32_t setAggregateSinkMetadata(bool) { return 0; }
#ifdef SEC_AUDIO_SAMSUNGRECORD
    std::shared_ptr<AudioPreProcess> PreProcessInit();
#endif

  protected:
    void defaultOnClose();

    ndk::ScopedAStatus getStreamCommon(
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon>* _aidl_return)
            override {
        return getStreamCommonCommon(_aidl_return);
    }
    ndk::ScopedAStatus updateMetadata(const ::aidl::android::hardware::audio::common::SinkMetadata&
                                              in_sinkMetadata) override {
        return updateMetadataCommon(in_sinkMetadata);
    }
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus getMicrophoneDirection(MicrophoneDirection* _aidl_return) override;
    ndk::ScopedAStatus setMicrophoneDirection(MicrophoneDirection in_direction) override;
    ndk::ScopedAStatus getMicrophoneFieldDimension(float* _aidl_return) override;
    ndk::ScopedAStatus setMicrophoneFieldDimension(float in_zoom) override;
    ndk::ScopedAStatus getHwGain(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwGain(const std::vector<float>& in_channelGains) override;

    friend class ndk::SharedRefBase;

    StreamIn(StreamContext&& context,
             const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

    StreamContext mContext;
    const std::map<::aidl::android::media::audio::common::AudioDevice, std::string> mMicrophones;
#ifdef SEC_AUDIO_SAMSUNGRECORD
    std::shared_ptr<AudioPreProcess> preprocess_;
    friend class AudioPreProcess;
#endif
};

class StreamOut : virtual public StreamCommonInterface,
                  public ::aidl::android::hardware::audio::core::BnStreamOut {
  public:
    virtual ~StreamOut() override = default;
    ndk::ScopedAStatus getMetadata(
            ::aidl::android::hardware::audio::common::SourceMetadata& out_sourceMetadata) {
        out_sourceMetadata = std::get<::aidl::android::hardware::audio::common::SourceMetadata>(
                getMetadataCommon());
        return ndk::ScopedAStatus::ok();
    }
    virtual int32_t setAggregateSourceMetadata(bool) { return 0; }
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    virtual int updateListenback(bool on) { return 0; }
#endif
  protected:
    void defaultOnClose();
    ndk::ScopedAStatus getStreamCommon(
            std::shared_ptr<::aidl::android::hardware::audio::core::IStreamCommon>* _aidl_return)
            override {
        return getStreamCommonCommon(_aidl_return);
    }
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata)
            override {
        return updateMetadataCommon(in_sourceMetadata);
    }
    ndk::ScopedAStatus updateOffloadMetadata(
            const ::aidl::android::hardware::audio::common::AudioOffloadMetadata&
                    in_offloadMetadata) override;
    ndk::ScopedAStatus getHwVolume(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwVolume(const std::vector<float>& in_channelVolumes) override;
    ndk::ScopedAStatus getAudioDescriptionMixLevel(float* _aidl_return) override;
    ndk::ScopedAStatus setAudioDescriptionMixLevel(float in_leveldB) override;
    ndk::ScopedAStatus getDualMonoMode(
            ::aidl::android::media::audio::common::AudioDualMonoMode* _aidl_return) override;
    ndk::ScopedAStatus setDualMonoMode(
            ::aidl::android::media::audio::common::AudioDualMonoMode in_mode) override;
    ndk::ScopedAStatus getRecommendedLatencyModes(
            std::vector<::aidl::android::media::audio::common::AudioLatencyMode>* _aidl_return)
            override;
    ndk::ScopedAStatus setLatencyMode(
            ::aidl::android::media::audio::common::AudioLatencyMode in_mode) override;
    ndk::ScopedAStatus getPlaybackRateParameters(
            ::aidl::android::media::audio::common::AudioPlaybackRate* _aidl_return) override;
    ndk::ScopedAStatus setPlaybackRateParameters(
            const ::aidl::android::media::audio::common::AudioPlaybackRate& in_playbackRate)
            override;
    ndk::ScopedAStatus selectPresentation(int32_t in_presentationId, int32_t in_programId) override;

    friend class ndk::SharedRefBase;

    StreamOut(StreamContext&& context,
              const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                      offloadInfo);

    StreamContext mContext;
    const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo> mOffloadInfo;
    std::optional<::aidl::android::hardware::audio::common::AudioOffloadMetadata> mOffloadMetadata;
};

// The recommended way to create a stream instance.
// 'StreamImpl' is the concrete stream implementation, 'StreamInOrOut' is either 'StreamIn' or
// 'StreamOut', the rest are the arguments forwarded to the constructor of 'StreamImpl'.
template <class StreamImpl, class StreamInOrOut, class... Args>
ndk::ScopedAStatus createStreamInstance(std::shared_ptr<StreamInOrOut>* result, Args&&... args) {
    std::shared_ptr<StreamInOrOut> stream =
            ::ndk::SharedRefBase::make<StreamImpl>(std::forward<Args>(args)...);
    RETURN_STATUS_IF_ERROR(stream->initInstance(stream));
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

class StreamWrapper {
  public:
    explicit StreamWrapper(const std::shared_ptr<StreamIn>& streamIn)
        : mStream(streamIn), mStreamBinder(streamIn->asBinder()) {}
    explicit StreamWrapper(const std::shared_ptr<StreamOut>& streamOut)
        : mStream(streamOut), mStreamBinder(streamOut->asBinder()) {}
    ndk::SpAIBinder getBinder() const { return mStreamBinder; }
    bool isStreamOpen() const {
        auto s = mStream.lock();
        return s && !s->isClosed();
    }
    ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
        auto s = mStream.lock();
        if (s) return s->setConnectedDevices(devices);
        return ndk::ScopedAStatus::ok();
    }

    void setStreamMicMute(const bool muted) {
        auto s = mStream.lock();
        if (s) return s->setStreamMicMute(muted);
        return;
    }

    ndk::ScopedAStatus configureMMapStream(int32_t* fd, int64_t* burstSizeFrames, int32_t* flags,
                                           int32_t* bufferSizeFrames) {
        auto s = mStream.lock();
        if (s) return s->configureMMapStream(fd, burstSizeFrames, flags, bufferSizeFrames);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

#ifdef SEC_AUDIO_COMMON
    ndk::ScopedAStatus ForceSetDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            bool force = false) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    bool isStreamUsecase(Usecase tag) { return false; };
    bool isDeviceAvailable(pal_device_id_t pal_device) { return false; };
    bool HasPalStreamHandle() { return false; };
    void forceShutdown() { return; };
#endif

  private:
    std::weak_ptr<StreamCommonInterface> mStream;
    ndk::SpAIBinder mStreamBinder;
};

class Streams {
  public:
    Streams() = default;
    Streams(const Streams&) = delete;
    Streams& operator=(const Streams&) = delete;
    size_t count(int32_t id) {
        // Streams do not remove themselves from the collection on close.
        erase_if(mStreams, [](const auto& pair) { return !pair.second.isStreamOpen(); });
        return mStreams.count(id);
    }
    void insert(int32_t portId, int32_t portConfigId, StreamWrapper sw) {
        mStreams.insert(std::pair{portConfigId, sw});
        mStreams.insert(std::pair{portId, std::move(sw)});
    }
    ndk::ScopedAStatus setStreamConnectedDevices(
            int32_t portConfigId,
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
        if (auto it = mStreams.find(portConfigId); it != mStreams.end()) {
            return it->second.setConnectedDevices(devices);
        }
        return ndk::ScopedAStatus::ok();
    }
    void setStreamMicMute(int32_t portConfigId, const bool muted) {
        if (auto it = mStreams.find(portConfigId); it != mStreams.end()) {
            return it->second.setStreamMicMute(muted);
        }
        return;
    }
    std::string toString() const {
        std::ostringstream os;
        os << std::endl << " --- mStreams ---" << std::endl;
        std::for_each(mStreams.cbegin(), mStreams.cend(), [&](const auto& pair) {
            os << "PortConfigId/PortId:" << pair.first << std::endl;
        });
        os << std::endl << " --- mStreams end ---" << std::endl << std::endl;
        return os.str();
    }

  private:
    // Maps port ids and port config ids to streams. Multimap because a port
    // (not port config) can have multiple streams opened on it.
    std::multimap<int32_t, StreamWrapper> mStreams;
};

} // namespace qti::audio::core
