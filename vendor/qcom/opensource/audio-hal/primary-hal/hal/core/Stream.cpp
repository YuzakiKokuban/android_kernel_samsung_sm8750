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

#define ATRACE_TAG (ATRACE_TAG_AUDIO | ATRACE_TAG_HAL)

#define LOG_TAG "AHAL_Stream_QTI"
#include <Utils.h>

#include <android-base/logging.h>
#include <android/binder_ibinder_platform.h>
#include <pthread.h>
#include <qti-audio-core/Module.h>
#include <qti-audio-core/ModulePrimary.h>
#include <qti-audio-core/Stream.h>
#include <utils/SystemClock.h>
#include <utils/Trace.h>

// uncomment this to enable logging of very verbose logs like burst commands.
// #define VERY_VERBOSE_LOGGING 1
using aidl::android::hardware::audio::common::AudioOffloadMetadata;
using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::common::getFrameSizeInBytes;
using aidl::android::hardware::audio::common::isBitPositionFlagSet;
using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDualMonoMode;
using aidl::android::media::audio::common::AudioInputFlags;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioLatencyMode;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioOutputFlags;
using aidl::android::media::audio::common::AudioPlaybackRate;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

using ::aidl::android::hardware::audio::common::getChannelCount;
using ::aidl::android::hardware::audio::common::getFrameSizeInBytes;
using ::aidl::android::hardware::audio::core::IStreamCallback;
using ::aidl::android::hardware::audio::core::IStreamCommon;
using ::aidl::android::hardware::audio::core::StreamDescriptor;
using ::aidl::android::hardware::audio::core::VendorParameter;

namespace qti::audio::core {

void StreamContext::fillDescriptor(StreamDescriptor* desc) {
    if (mCommandMQ) {
        desc->command = mCommandMQ->dupeDesc();
    }
    if (mReplyMQ) {
        desc->reply = mReplyMQ->dupeDesc();
    }
    if (mDataMQ) {
        desc->frameSizeBytes = getFrameSize();
        desc->bufferSizeFrames = getBufferSizeInFrames();
        desc->audio.set<StreamDescriptor::AudioBuffer::Tag::fmq>(mDataMQ->dupeDesc());
    }
}

size_t StreamContext::getBufferSizeInFrames() const {
    if (mDataMQ) {
        return getFrameSize() ?
            mDataMQ->getQuantumCount() * mDataMQ->getQuantumSize() / getFrameSize() : 0;
    }
    return 0;
}

size_t StreamContext::getFrameSize() const {
    return getFrameSizeInBytes(mFormat, mChannelLayout);
}

bool StreamContext::isValid() const {
    if (mCommandMQ && !mCommandMQ->isValid()) {
        LOG(ERROR) << "command FMQ is invalid";
        return false;
    }
    if (mReplyMQ && !mReplyMQ->isValid()) {
        LOG(ERROR) << "reply FMQ is invalid";
        return false;
    }
    if (getFrameSize() == 0) {
        LOG(ERROR) << "frame size is invalid";
        return false;
    }
    if (mDataMQ && !mDataMQ->isValid()) {
        LOG(ERROR) << "data FMQ is invalid";
        return false;
    }
    return true;
}

void StreamContext::reset() {
    mCommandMQ.reset();
    mReplyMQ.reset();
    mDataMQ.reset();
}

pid_t StreamWorkerCommonLogic::getTid() const {
#if defined(__ANDROID__)
    return pthread_gettid_np(pthread_self());
#else
    return 0;
#endif
}

std::string StreamWorkerCommonLogic::init() {
    if (mContext->getCommandMQ() == nullptr) return "Command MQ is null";
    if (mContext->getReplyMQ() == nullptr) return "Reply MQ is null";
    StreamContext::DataMQ* const dataMQ = mContext->getDataMQ();
    if (dataMQ == nullptr) return "Data MQ is null";
    if (sizeof(DataBufferElement) != dataMQ->getQuantumSize()) {
        return "Unexpected Data MQ quantum size: " + std::to_string(dataMQ->getQuantumSize());
    }
    mDataBufferSize = dataMQ->getQuantumCount() * dataMQ->getQuantumSize();
    mDataBuffer.reset(new (std::nothrow) DataBufferElement[mDataBufferSize]);
    if (mDataBuffer == nullptr) {
        return "Failed to allocate data buffer for element count " +
               std::to_string(dataMQ->getQuantumCount()) + ", size in bytes: " +
               std::to_string(mDataBufferSize);
    }
    if (::android::status_t status = mDriver->init(); status != STATUS_OK) {
        return "Failed to initialize the driver: " + std::to_string(status);
    }
    return "";
}

void StreamWorkerCommonLogic::populateReply(StreamDescriptor::Reply* reply,
                                            bool isConnected) const {
    if (reply->status != STATUS_DEAD_OBJECT) {
        reply->status = STATUS_OK;
    }

    static const StreamDescriptor::Position kUnknownPosition = {
      .frames = StreamDescriptor::Position::UNKNOWN,
      .timeNs = StreamDescriptor::Position::UNKNOWN};

    reply->latencyMs = mContext->getNominalLatencyMs();

    reply->observable.frames = mContext->getFrameCount();
    reply->observable.timeNs = ::android::uptimeNanos();
    if (auto status = mDriver->refinePosition(reply); status == ::android::OK) {
        return;
    }
    else {
       if (hasMMapFlagsEnabled(mContext->getFlags())) {
           // if mmap position fails,return error to framework
           // for any error other than.. not enough data, AAudio will stop
           reply->status = STATUS_INVALID_OPERATION;
       }
    }

    reply->observable = reply->hardware = kUnknownPosition;
}

void StreamWorkerCommonLogic::populateReplyWrongState(
        StreamDescriptor::Reply* reply, const StreamDescriptor::Command& command) const {
    LOG(WARNING) << "command '" << toString(command.getTag())
                 << "' can not be handled in the state " << toString(mState);
    reply->status = STATUS_INVALID_OPERATION;
}

const std::string StreamInWorkerLogic::kThreadName = "reader";

StreamInWorkerLogic::Status StreamInWorkerLogic::cycle() {
    // Note: for input streams, draining is driven by the client, thus
    // "empty buffer" condition can only happen while handling the 'burst'
    // command. Thus, unlike for output streams, it does not make sense to
    // delay the 'DRAINING' state here by 'mTransientStateDelayMs'.
    // TODO: Add a delay for transitions of async operations when/if they added.

    StreamDescriptor::Command command{};
    if (!mContext->getCommandMQ()->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    using Tag = StreamDescriptor::Command::Tag;
    using LogSeverity = ::android::base::LogSeverity;
    const LogSeverity severity =
            command.getTag() == Tag::burst || command.getTag() == Tag::getStatus
                    ? LogSeverity::VERBOSE
                    : LogSeverity::DEBUG;

#ifdef VERY_VERBOSE_LOGGING
    LOG(severity) << __func__ << ": received command " << command.toString() << " in "
                  << kThreadName;
#else
    if (command.getTag() != Tag::burst && command.getTag() != Tag::getStatus)
        LOG(DEBUG) << __func__ << ": received command " << command.toString() << " in "
                   << kThreadName;
#endif

    StreamDescriptor::Reply reply{};
    reply.status = STATUS_BAD_VALUE;
    switch (command.getTag()) {
        case Tag::halReservedExit:
            if (const int32_t cookie = command.get<Tag::halReservedExit>();
                cookie == mContext->getInternalCommandCookie()) {
                mDriver->shutdown();
                setClosed();
                // This is an internal command, no need to reply.
                return Status::EXIT;
            } else {
                LOG(WARNING) << __func__ << ": EXIT command has a bad cookie: " << cookie;
            }
            break;
        case Tag::getStatus:
            populateReply(&reply, mIsConnected);
            break;
        case Tag::start:
            if (mState == StreamDescriptor::State::STANDBY ||
                mState == StreamDescriptor::State::DRAINING) {
                if (::android::status_t status = mDriver->start(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = mState == StreamDescriptor::State::STANDBY
                                     ? StreamDescriptor::State::IDLE
                                     : StreamDescriptor::State::ACTIVE;
                } else {
                    LOG(ERROR) << __func__ << ": start failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::burst:
            if (const int32_t fmqByteCount = command.get<Tag::burst>(); fmqByteCount >= 0) {
#ifdef VERY_VERBOSE_LOGGING
                LOG(VERBOSE) << __func__ << ": '" << toString(command.getTag()) << "' command for "
                             << fmqByteCount << " bytes";
#endif
                if (mState == StreamDescriptor::State::IDLE ||
                    mState == StreamDescriptor::State::ACTIVE ||
                    mState == StreamDescriptor::State::PAUSED ||
                    mState == StreamDescriptor::State::DRAINING) {
                    if (!read(fmqByteCount, &reply)) {
                        // uncomment below, to treat the failure as HARD error, stream not recoverable
                        // mState = StreamDescriptor::State::ERROR;
                    }
                    if (mState == StreamDescriptor::State::IDLE ||
                        mState == StreamDescriptor::State::PAUSED) {
                        mState = StreamDescriptor::State::ACTIVE;
                    } else if (mState == StreamDescriptor::State::DRAINING) {
                        // To simplify the reference code, we assume that the
                        // read operation has consumed all the data remaining in
                        // the hardware buffer. In a real implementation, here
                        // we would either remain in the 'DRAINING' state, or
                        // transfer to 'STANDBY' depending on the buffer state.
                        mState = StreamDescriptor::State::STANDBY;
                    }
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid burst byte count: " << fmqByteCount;
            }
            break;
        case Tag::drain:
            if (const auto mode = command.get<Tag::drain>();
                mode == StreamDescriptor::DrainMode::DRAIN_UNSPECIFIED) {
                if (mState == StreamDescriptor::State::ACTIVE) {
                    if (::android::status_t status = mDriver->drain(mode);
                        status == ::android::OK) {
                        populateReply(&reply, mIsConnected);
                        mState = StreamDescriptor::State::DRAINING;
                    } else {
                        LOG(ERROR) << __func__ << ": drain failed: " << status;
                        // uncomment below, to treat the failure as HARD error, stream not recoverable
                        // mState = StreamDescriptor::State::ERROR;
                    }
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid drain mode: " << toString(mode);
            }
            break;
        case Tag::standby:
            if (mState == StreamDescriptor::State::IDLE) {
                if (::android::status_t status = mDriver->standby(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = StreamDescriptor::State::STANDBY;
                } else {
                    LOG(ERROR) << __func__ << ": standby failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::pause:
            if (mState == StreamDescriptor::State::ACTIVE) {
                if (::android::status_t status = mDriver->pause(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = StreamDescriptor::State::PAUSED;
                } else {
                    LOG(ERROR) << __func__ << ": pause failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::flush:
            if (mState == StreamDescriptor::State::PAUSED) {
                if (::android::status_t status = mDriver->flush(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = StreamDescriptor::State::STANDBY;
                } else {
                    LOG(ERROR) << __func__ << ": flush failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
    }
    reply.state = mState;
    LOG(severity) << __func__ << ": writing reply " << reply.toString();
    if (!mContext->getReplyMQ()->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }
    return Status::CONTINUE;
}

bool StreamInWorkerLogic::read(size_t clientSize, StreamDescriptor::Reply* reply) {
    ATRACE_CALL();
    StreamContext::DataMQ* const dataMQ = mContext->getDataMQ();
    const size_t byteCount = std::min({clientSize, dataMQ->availableToWrite(), mDataBufferSize});
    const bool isConnected = mIsConnected;
    const size_t frameSize = mContext->getFrameSize();
    size_t actualFrameCount = 0;
    bool fatal = false;
    int32_t latency = mContext->getNominalLatencyMs();
    if (frameSize == 0) {
        LOG(ERROR) << __func__ << "frame size is invalid";
        return fatal;
    }
    if (isConnected) {
        if (::android::status_t status = mDriver->transfer(mDataBuffer.get(), byteCount / frameSize,
                                                           &actualFrameCount, &latency);
            status != ::android::OK) {
            fatal = true;
            LOG(ERROR) << __func__ << ": read failed: " << status;
        }
    } else {
        usleep(3000); // Simulate blocking transfer delay.
        for (size_t i = 0; i < byteCount; ++i) mDataBuffer[i] = 0;
        actualFrameCount = byteCount / frameSize;
    }
    const size_t actualByteCount = actualFrameCount * frameSize;
    if (bool success = actualByteCount > 0 ? dataMQ->write(&mDataBuffer[0], actualByteCount) : true;
        success) {
#ifdef VERY_VERBOSE_LOGGING
        LOG(VERBOSE) << __func__ << ": writing of " << actualByteCount << " bytes into data MQ"
                     << " succeeded; connected? " << isConnected;
#endif
        // Frames are provided and counted regardless of connection status.
        reply->fmqByteCount += actualByteCount;
        mContext->advanceFrameCount(actualFrameCount);
        populateReply(reply, isConnected);
    } else {
        LOG(WARNING) << __func__ << ": writing of " << actualByteCount
                     << " bytes of data to MQ failed";
        reply->status = STATUS_NOT_ENOUGH_DATA;
    }
    reply->latencyMs = latency;
    return !fatal;
}

const std::string StreamOutWorkerLogic::kThreadName = "writer";

void StreamOutWorkerLogic::publishTransferReady() {
    if (!mContext->getAsyncCallback()) {
        return;
    }
    std::unique_lock lock{mAsyncMutex};
    mPendingCallBack = std::nullopt;
    if (mState == StreamDescriptor::State::TRANSFERRING) {
        mState = StreamDescriptor::State::ACTIVE;
        mContext->getAsyncCallback()->onTransferReady();
        LOG(VERBOSE) << __func__ << ": sent transfer ready to client";
    } else if (mState == StreamDescriptor::State::TRANSFER_PAUSED) {
        mPendingCallBack = StreamCallbackType::TR;
        LOG(VERBOSE) << __func__ << ": pending transfer ready";
    } else {
        LOG(WARNING) << __func__ << ": shouldn't happen !!";
    }
}

void StreamOutWorkerLogic::publishDrainReady() {
    if (!mContext->getAsyncCallback()) {
        return;
    }
    std::unique_lock lock{mAsyncMutex};
    mPendingCallBack = std::nullopt;
    if (mState == StreamDescriptor::State::DRAINING) {
        mContext->getAsyncCallback()->onDrainReady();
        if (mRecentDrainMode == StreamDescriptor::DrainMode::DRAIN_ALL)
            mState = StreamDescriptor::State::IDLE;
        LOG(VERBOSE) << __func__ << ": sent drain ready to client";
    } else if (mState == StreamDescriptor::State::DRAIN_PAUSED) {
        mPendingCallBack = StreamCallbackType::DR;
        LOG(VERBOSE) << __func__ << ": pending drain ready";
    } else {
        LOG(WARNING) << __func__ << ": shouldn't happen !!";
    }
}

void StreamOutWorkerLogic::publishError() {
    if (!mContext->getAsyncCallback()) {
        return;
    }
    mContext->getAsyncCallback()->onError();
    LOG(WARNING) << __func__ << ": sent Error to the client";
}

StreamOutWorkerLogic::Status StreamOutWorkerLogic::cycle() {
    StreamDescriptor::Command command{};
    if (!mContext->getCommandMQ()->readBlocking(&command, 1)) {
        LOG(ERROR) << __func__ << ": reading of command from MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }

    std::unique_lock asyncLock{mAsyncMutex, std::defer_lock};
    if (mContext->getAsyncCallback()) {
        // Accquring the lock in case of Asynchronous Stream
        asyncLock.lock();
    } else {
        // Synchronous case
        if (mState == StreamDescriptor::State::DRAINING) {
            if (auto stateDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - mTransientStateStart);
                stateDurationMs >= mTransientStateDelayMs) {
                // In blocking mode, after some duration, expecting, hardware is drained.
                mState = StreamDescriptor::State::IDLE;
            }
        }
    }

    LOG(VERBOSE) << __func__ << ": received command " << command.toString() << " in "
                   << kThreadName;

    StreamDescriptor::Reply reply{};
    reply.status = STATUS_BAD_VALUE;
    using Tag = StreamDescriptor::Command::Tag;
    switch (command.getTag()) {
        case Tag::halReservedExit:
            if (const int32_t cookie = command.get<Tag::halReservedExit>();
                cookie == mContext->getInternalCommandCookie()) {
#ifdef SEC_AUDIO_OFFLOAD
                if (mContext->getAsyncCallback()) {
                    // do explicit unlock
                    asyncLock.unlock();
                }
                mDriver->shutdown();
                if (mContext->getAsyncCallback()) {
                    // do explicit lock
                    asyncLock.lock();
                }
#else
                mDriver->shutdown();
#endif
                setClosed();
                // This is an internal command, no need to reply.
                return Status::EXIT;
            } else {
                LOG(WARNING) << __func__ << ": EXIT command has a bad cookie: " << cookie;
            }
            break;
        case Tag::getStatus:
            populateReply(&reply, mIsConnected);
            break;
        case Tag::start: {
            std::optional<StreamDescriptor::State> nextState;
            switch (mState) {
                case StreamDescriptor::State::STANDBY:
                    nextState = StreamDescriptor::State::IDLE;
                    break;
                case StreamDescriptor::State::PAUSED:
                    nextState = StreamDescriptor::State::ACTIVE;
                    break;
                case StreamDescriptor::State::DRAIN_PAUSED:
                    nextState = StreamDescriptor::State::DRAINING;
                    break;
                case StreamDescriptor::State::TRANSFER_PAUSED:
                    nextState = StreamDescriptor::State::TRANSFERRING;
                    break;
                default:
                    populateReplyWrongState(&reply, command);
            }
            if (nextState.has_value()) {
                if (::android::status_t status = mDriver->start(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    if (*nextState == StreamDescriptor::State::IDLE ||
                        *nextState == StreamDescriptor::State::ACTIVE) {
                        mState = *nextState;
                    } else {
                        switchToTransientState(*nextState);
                    }
                } else {
                    LOG(ERROR) << __func__ << ": start failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            }
        } break;
        case Tag::burst:
            if (const int32_t fmqByteCount = command.get<Tag::burst>(); fmqByteCount >= 0) {
#ifdef VERY_VERBOSE_LOGGING
                LOG(VERBOSE) << __func__ << ": '" << toString(command.getTag()) << "' command for "
                             << fmqByteCount << " bytes";
#endif
                if (mState != StreamDescriptor::State::ERROR &&
                    mState != StreamDescriptor::State::TRANSFERRING &&
                    mState != StreamDescriptor::State::TRANSFER_PAUSED) {
                    if (!write(fmqByteCount, &reply)) {
                        LOG(ERROR) << __func__ << ": write failed, but dont put in error state ";
                    }
                    std::shared_ptr<IStreamCallback> asyncCallback = mContext->getAsyncCallback();
                    if (mState == StreamDescriptor::State::STANDBY ||
                        mState == StreamDescriptor::State::DRAIN_PAUSED ||
                        mState == StreamDescriptor::State::PAUSED) {
                        if (asyncCallback == nullptr ||
                            mState != StreamDescriptor::State::DRAIN_PAUSED) {
                            mState = StreamDescriptor::State::PAUSED;
                        } else {
                            mState = StreamDescriptor::State::TRANSFER_PAUSED;
                        }
                    } else if (mState == StreamDescriptor::State::IDLE ||
                               mState == StreamDescriptor::State::DRAINING ||
                               mState == StreamDescriptor::State::ACTIVE) {
                        if (asyncCallback == nullptr || reply.fmqByteCount == fmqByteCount) {
                            mState = StreamDescriptor::State::ACTIVE;
                        } else {
                            //If write status is not ok, then dont put state in transferring
                            if (reply.status == STATUS_OK) {
                                switchToTransientState(StreamDescriptor::State::TRANSFERRING);
                            }
                        }
                    }
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid burst byte count: " << fmqByteCount;
            }
            break;
        case Tag::drain:
            if (mRecentDrainMode = command.get<Tag::drain>();
                mRecentDrainMode == StreamDescriptor::DrainMode::DRAIN_ALL ||
                mRecentDrainMode == StreamDescriptor::DrainMode::DRAIN_EARLY_NOTIFY) {
                if (mState == StreamDescriptor::State::ACTIVE ||
                    mState == StreamDescriptor::State::TRANSFERRING) {
                    if (::android::status_t status = mDriver->drain(mRecentDrainMode);
                        status == ::android::OK) {
                        populateReply(&reply, mIsConnected);
                        if (mState == StreamDescriptor::State::ACTIVE &&
                            mContext->getForceSynchronousDrain()) {
                            mState = StreamDescriptor::State::IDLE;
                        } else {
                            switchToTransientState(StreamDescriptor::State::DRAINING);
                        }
                    } else {
                        LOG(ERROR) << __func__ << ": drain failed: " << status;
                        // uncomment below, to treat the failure as HARD error, stream not recoverable
                        // mState = StreamDescriptor::State::ERROR;
                    }
                } else if (mState == StreamDescriptor::State::TRANSFER_PAUSED) {
                    mState = StreamDescriptor::State::DRAIN_PAUSED;
                    populateReply(&reply, mIsConnected);
                } else {
                    populateReplyWrongState(&reply, command);
                }
            } else {
                LOG(WARNING) << __func__ << ": invalid drain mode: " << toString(mRecentDrainMode);
            }
            break;
        case Tag::standby:
            if (mState == StreamDescriptor::State::IDLE) {
                if (mContext->getAsyncCallback()) {
                   // do explicit unlock, so that callback can acquire
                    asyncLock.unlock();
                }
                if (::android::status_t status = mDriver->standby(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = StreamDescriptor::State::STANDBY;
                } else {
                    LOG(ERROR) << __func__ << ": standby failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
                if (mContext->getAsyncCallback()) {
                    // do explicit lock
                    asyncLock.lock();
                }
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
        case Tag::pause: {
            std::optional<StreamDescriptor::State> nextState;
            switch (mState) {
                case StreamDescriptor::State::ACTIVE:
                    nextState = StreamDescriptor::State::PAUSED;
                    break;
                case StreamDescriptor::State::DRAINING:
                    nextState = StreamDescriptor::State::DRAIN_PAUSED;
                    break;
                case StreamDescriptor::State::TRANSFERRING:
                    nextState = StreamDescriptor::State::TRANSFER_PAUSED;
                    break;
                default:
                    populateReplyWrongState(&reply, command);
            }
            if (nextState.has_value()) {
                if (::android::status_t status = mDriver->pause(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = nextState.value();
                } else {
                    LOG(ERROR) << __func__ << ": pause failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            }
        } break;
        case Tag::flush:
            if (mState == StreamDescriptor::State::PAUSED ||
                mState == StreamDescriptor::State::DRAIN_PAUSED ||
                mState == StreamDescriptor::State::TRANSFER_PAUSED) {
                if (::android::status_t status = mDriver->flush(); status == ::android::OK) {
                    populateReply(&reply, mIsConnected);
                    mState = StreamDescriptor::State::IDLE;
                } else {
                    LOG(ERROR) << __func__ << ": flush failed: " << status;
                    // uncomment below, to treat the failure as HARD error, stream not recoverable
                    // mState = StreamDescriptor::State::ERROR;
                }
            } else {
                populateReplyWrongState(&reply, command);
            }
            break;
    }
    reply.state = mState;

    using LogSeverity = ::android::base::LogSeverity;
    const LogSeverity severity =
            (reply.status != STATUS_OK) ? LogSeverity::ERROR : LogSeverity::VERBOSE;
    LOG(severity) << __func__ << ": writing reply " << reply.toString();

    if (!mContext->getReplyMQ()->writeBlocking(&reply, 1)) {
        LOG(ERROR) << __func__ << ": writing of reply " << reply.toString() << " to MQ failed";
        mState = StreamDescriptor::State::ERROR;
        return Status::ABORT;
    }

    if (mContext->getAsyncCallback() && mPendingCallBack && command.getTag() != Tag::getStatus) {
        /**
         * After the writing the reply, handle the pending callback, if any
         * and issue to the client based on the stream state.
         **/
        if (command.getTag() == Tag::start && mState == StreamDescriptor::State::TRANSFERRING &&
            mPendingCallBack == StreamCallbackType::TR) {
            mContext->getAsyncCallback()->onTransferReady();
            mState = StreamDescriptor::State::ACTIVE;
            mPendingCallBack = {};
            LOG(VERBOSE) << __func__ << ": sent pending transfer ready !!!";
        } else if (command.getTag() == Tag::start && mState == StreamDescriptor::State::DRAINING &&
                   mPendingCallBack == StreamCallbackType::DR) {
            mContext->getAsyncCallback()->onDrainReady();
            if (mRecentDrainMode == StreamDescriptor::DrainMode::DRAIN_ALL)
                mState = StreamDescriptor::State::IDLE;
            mPendingCallBack = {};
            LOG(VERBOSE) << __func__ << ": sent pending drain ready !!!";
        } else if (command.getTag() == Tag::flush || command.getTag() == Tag::drain) {
            // clear the pending callbacks
            mPendingCallBack = {};
            LOG(VERBOSE) << __func__ << ": cleared the pending callback !!!";
        } else {
            // clear the pending callbacks
            // mPendingCallBack = {};
            LOG(WARNING) << __func__ << ": shouldn't happen !!!";
        }
    }

    return Status::CONTINUE;
}

bool StreamOutWorkerLogic::write(size_t clientSize, StreamDescriptor::Reply* reply) {
    ATRACE_CALL();
    StreamContext::DataMQ* const dataMQ = mContext->getDataMQ();
    const size_t readByteCount = dataMQ->availableToRead();
    const size_t frameSize = mContext->getFrameSize();
    bool fatal = false;
    int32_t latency = mContext->getNominalLatencyMs();
    if (frameSize == 0) {
        LOG(ERROR) << __func__ << "frame size is invalid";
        return fatal;
    }
    if (bool success = readByteCount > 0 ? dataMQ->read(&mDataBuffer[0], readByteCount) : true) {
        const bool isConnected = mIsConnected;
#ifdef VERY_VERBOSE_LOGGING
        LOG(VERBOSE) << __func__ << ": reading of " << readByteCount << " bytes from data MQ"
                     << " succeeded; connected? " << isConnected;
#endif
        // Amount of data that the HAL module is going to actually use.
        size_t byteCount = std::min({clientSize, readByteCount, mDataBufferSize});
        if (byteCount >= frameSize && mContext->getForceTransientBurst()) {
            // In order to prevent the state machine from going to ACTIVE state,
            // simulate partial write.
            byteCount -= frameSize;
        }
        size_t actualFrameCount = 0;
        // No need to check for connected device, if there is issue, write returns failure
        if (::android::status_t status = mDriver->transfer(
                 mDataBuffer.get(), byteCount / frameSize, &actualFrameCount, &latency);
                status != ::android::OK) {
                reply->status = STATUS_DEAD_OBJECT;
                fatal = true;
                LOG(ERROR) << __func__ << ": write failed: " << status;
        }

        const size_t actualByteCount = actualFrameCount * frameSize;
        // Frames are consumed and counted regardless of the connection status.
        if (actualByteCount >
            static_cast<size_t>(std::numeric_limits<std::int32_t>::max() - reply->fmqByteCount)) {
            reply->fmqByteCount = std::numeric_limits<std::int32_t>::max();
        } else {
            reply->fmqByteCount += actualByteCount;
        }
        if (actualByteCount > static_cast<size_t>(LONG_MAX - mContext->getFrameCount())) {
            mContext->advanceFrameCount(LONG_MAX - mContext->getFrameCount());
        } else {
            mContext->advanceFrameCount(actualFrameCount);
        }
        populateReply(reply, isConnected);
    } else {
        LOG(WARNING) << __func__ << ": reading of " << readByteCount
                     << " bytes of data from MQ failed";
        reply->status = STATUS_NOT_ENOUGH_DATA;
    }
    reply->latencyMs = latency;
    return !fatal;
}

StreamCommonImpl::~StreamCommonImpl() {
    if (!isClosed()) {
        LOG(ERROR) << __func__ << ": stream was not closed prior to destruction, resource leak";
        stopWorker();
        // The worker and the context should clean up by themselves via
        // destructors.
    }
    LOG(VERBOSE) << __func__ << ": destroy " << std::hex << this;
}

ndk::ScopedAStatus StreamCommonImpl::initInstance(
        const std::shared_ptr<StreamCommonInterface>& delegate) {
    mCommon = ndk::SharedRefBase::make<StreamCommonDelegator>(delegate);
    if (!mWorker->start()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (auto flags = getContext().getFlags();
        (flags.getTag() == AudioIoFlags::Tag::input &&
         isBitPositionFlagSet(flags.template get<AudioIoFlags::Tag::input>(),
                              AudioInputFlags::FAST)) ||
        (flags.getTag() == AudioIoFlags::Tag::output &&
         isBitPositionFlagSet(flags.template get<AudioIoFlags::Tag::output>(),
                              AudioOutputFlags::FAST))) {
        // FAST workers should be run with a SCHED_FIFO scheduler, however the host process
        // might be lacking the capability to request it, thus a failure to set is not an error.
        pid_t workerTid = mWorker->getTid();
        if (workerTid > 0) {
            struct sched_param param;
            param.sched_priority = 3; // Must match SchedulingPolicyService.PRIORITY_MAX (Java).
            LOG(DEBUG) << __func__ << ": increase scheduling for tid : " << workerTid;
            if (sched_setscheduler(workerTid, SCHED_FIFO | SCHED_RESET_ON_FORK, &param) != 0) {
                LOG(WARNING) << __func__ << ": failed to set FIFO scheduler for a fast thread";
            }
        } else {
            LOG(WARNING) << __func__ << ": invalid worker tid: " << workerTid;
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamCommonImpl::getStreamCommonCommon(
        std::shared_ptr<IStreamCommon>* _aidl_return) {
    if (!mCommon) {
        LOG(FATAL) << __func__ << ": the common interface was not created";
    }
    *_aidl_return = mCommon.getInstance();
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->get()->asBinder().get();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamCommonImpl::updateHwAvSyncId(int32_t in_hwAvSyncId) {
    LOG(DEBUG) << __func__ << ": id " << in_hwAvSyncId;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamCommonImpl::getVendorParameters(
        const std::vector<std::string>& in_ids, std::vector<VendorParameter>* _aidl_return) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamCommonImpl::setVendorParameters(
        const std::vector<VendorParameter>& in_parameters, bool in_async) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamCommonImpl::addEffect(
        const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect) {
    if (in_effect == nullptr) {
        LOG(DEBUG) << __func__ << ": null effect";
    } else {
        LOG(DEBUG) << __func__ << ": effect Binder" << in_effect->asBinder().get();
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamCommonImpl::removeEffect(
        const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect) {
    if (in_effect == nullptr) {
        LOG(DEBUG) << __func__ << ": null effect";
    } else {
        LOG(DEBUG) << __func__ << ": effect Binder" << in_effect->asBinder().get();
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamCommonImpl::close() {
    ModulePrimary::outListMutex.lock();
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        stopWorker();
        LOG(DEBUG) << __func__ << ": joining the worker thread...";
        mWorker->join();
        LOG(DEBUG) << __func__ << ": worker thread joined";
        onClose();
        mWorker->setClosed();
        ModulePrimary::outListMutex.unlock();
        return ndk::ScopedAStatus::ok();
    } else {
        LOG(ERROR) << __func__ << ": stream was already closed";
        ModulePrimary::outListMutex.unlock();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
}

ndk::ScopedAStatus StreamCommonImpl::prepareToClose() {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": stream was closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

void StreamCommonImpl::stopWorker() {
    if (auto commandMQ = mContextRef.getCommandMQ(); commandMQ != nullptr) {
        LOG(DEBUG) << __func__ << ": asking the worker to exit...";
        auto cmd = StreamDescriptor::Command::make<StreamDescriptor::Command::Tag::halReservedExit>(
                mContextRef.getInternalCommandCookie());
        // Note: never call 'pause' and 'resume' methods of StreamWorker
        // in the HAL implementation. These methods are to be used by
        // the client side only. Preventing the worker loop from running
        // on the HAL side can cause a deadlock.
        if (!commandMQ->writeBlocking(&cmd, 1)) {
            LOG(ERROR) << __func__ << ": failed to write exit command to the MQ";
        }
        LOG(DEBUG) << __func__ << ": done";
    }
}

ndk::ScopedAStatus StreamCommonImpl::updateMetadataCommon(const Metadata& metadata) {
    LOG(DEBUG) << __func__;
    if (!isClosed()) {
        if (metadata.index() != mMetadata.index()) {
            LOG(FATAL) << __func__ << ": changing metadata variant is not allowed";
        }
        mMetadata = metadata;
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": stream was closed";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}

ndk::ScopedAStatus StreamCommonImpl::setConnectedDevices(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
    mWorker->setIsConnected(!devices.empty());
    mConnectedDevices = devices;
    return ndk::ScopedAStatus::ok();
}

void StreamCommonImpl::setStreamMicMute(const bool muted) {
    return;
}

ndk::ScopedAStatus StreamCommonImpl::configureMMapStream(int32_t* fd, int64_t* burstSizeFrames,
                                                         int32_t* flags,
                                                         int32_t* bufferSizeFrames) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

#ifdef SEC_AUDIO_COMMON
ndk::ScopedAStatus StreamCommonImpl::ForceSetDevices(
        const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices, bool force) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
}
bool StreamCommonImpl::isStreamUsecase(Usecase tag) { return false; };
bool StreamCommonImpl::isDeviceAvailable(pal_device_id_t pal_device) { return false; };
bool StreamCommonImpl::HasPalStreamHandle() { return false; };
void StreamCommonImpl::forceShutdown() { return; };
bool StreamCommonImpl::isCustomKeyRouted(const int ck_id) {
    if (mPalDevices.size() == 0) {
        return false;
    }
    return (strcmp(mPalDevices[0].custom_config.custom_key, ck_table[ck_id]) == 0);
}
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
void StreamCommonImpl::RerouteForInterpreter() { return; };
#endif

namespace {
static std::map<AudioDevice, std::string> transformMicrophones(
        const std::vector<MicrophoneInfo>& microphones) {
    std::map<AudioDevice, std::string> result;
    std::transform(microphones.begin(), microphones.end(), std::inserter(result, result.begin()),
                   [](const auto& mic) { return std::make_pair(mic.device, mic.id); });
    return result;
}
} // namespace

StreamIn::StreamIn(StreamContext&& context, const std::vector<MicrophoneInfo>& microphones)
    : mContext(std::move(context)), mMicrophones(transformMicrophones(microphones)) {
    LOG(DEBUG) << __func__;
}

void StreamIn::defaultOnClose() {
    mContext.reset();
}

#ifdef SEC_AUDIO_SAMSUNGRECORD
std::shared_ptr<AudioPreProcess> StreamIn::PreProcessInit() {
    std::shared_ptr<AudioPreProcess> preprocess(new AudioPreProcess());

    return preprocess;
}
#endif

ndk::ScopedAStatus StreamIn::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return) {
    std::vector<MicrophoneDynamicInfo> result;
    std::vector<MicrophoneDynamicInfo::ChannelMapping> channelMapping{
            getChannelCount(getContext().getChannelLayout()),
            MicrophoneDynamicInfo::ChannelMapping::DIRECT};
    for (auto it = getConnectedDevices().begin(); it != getConnectedDevices().end(); ++it) {
        if (auto micIt = mMicrophones.find(*it); micIt != mMicrophones.end()) {
            MicrophoneDynamicInfo dynMic;
            dynMic.id = micIt->second;
            dynMic.channelMapping = channelMapping;
            result.push_back(std::move(dynMic));
        }
    }
    *_aidl_return = std::move(result);
    LOG(DEBUG) << __func__ << ": returning " << ::android::internal::ToString(*_aidl_return);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamIn::getMicrophoneDirection(MicrophoneDirection* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::setMicrophoneDirection(MicrophoneDirection in_direction) {
    LOG(DEBUG) << __func__ << ": direction " << toString(in_direction);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::getMicrophoneFieldDimension(float* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::setMicrophoneFieldDimension(float in_zoom) {
    LOG(DEBUG) << __func__ << ": zoom " << in_zoom;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::getHwGain(std::vector<float>* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamIn::setHwGain(const std::vector<float>& in_channelGains) {
    LOG(DEBUG) << __func__ << ": gains " << ::android::internal::ToString(in_channelGains);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

StreamOut::StreamOut(StreamContext&& context, const std::optional<AudioOffloadInfo>& offloadInfo)
    : mContext(std::move(context)), mOffloadInfo(offloadInfo) {
    LOG(DEBUG) << __func__;
}

void StreamOut::defaultOnClose() {
    mContext.reset();
}

ndk::ScopedAStatus StreamOut::updateOffloadMetadata(
        const AudioOffloadMetadata& in_offloadMetadata) {
    LOG(DEBUG) << __func__;
    if (isClosed()) {
        LOG(ERROR) << __func__ << ": stream was closed";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (!mOffloadInfo.has_value()) {
        LOG(ERROR) << __func__ << ": not a compressed offload stream";
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    if (in_offloadMetadata.sampleRate < 0) {
        LOG(ERROR) << __func__ << ": invalid sample rate value: " << in_offloadMetadata.sampleRate;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_offloadMetadata.averageBitRatePerSecond < 0) {
        LOG(ERROR) << __func__
                   << ": invalid average BPS value: " << in_offloadMetadata.averageBitRatePerSecond;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_offloadMetadata.delayFrames < 0) {
        LOG(ERROR) << __func__
                   << ": invalid delay frames value: " << in_offloadMetadata.delayFrames;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_offloadMetadata.paddingFrames < 0) {
        LOG(ERROR) << __func__
                   << ": invalid padding frames value: " << in_offloadMetadata.paddingFrames;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    mOffloadMetadata = in_offloadMetadata;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamOut::getHwVolume(std::vector<float>* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setHwVolume(const std::vector<float>& in_channelVolumes) {
    LOG(DEBUG) << __func__ << ": gains " << ::android::internal::ToString(in_channelVolumes);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getAudioDescriptionMixLevel(float* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setAudioDescriptionMixLevel(float in_leveldB) {
    LOG(DEBUG) << __func__ << ": description mix level " << in_leveldB;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getDualMonoMode(AudioDualMonoMode* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setDualMonoMode(AudioDualMonoMode in_mode) {
    LOG(DEBUG) << __func__ << ": dual mono mode " << toString(in_mode);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getRecommendedLatencyModes(
        std::vector<AudioLatencyMode>* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setLatencyMode(AudioLatencyMode in_mode) {
    LOG(DEBUG) << __func__ << ": latency mode " << toString(in_mode);
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::getPlaybackRateParameters(AudioPlaybackRate* _aidl_return) {
    LOG(DEBUG) << __func__;
    (void)_aidl_return;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::setPlaybackRateParameters(const AudioPlaybackRate& in_playbackRate) {
    LOG(DEBUG) << __func__ << ": " << in_playbackRate.toString();
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamOut::selectPresentation(int32_t in_presentationId, int32_t in_programId) {
    LOG(DEBUG) << __func__ << ": presentationId " << in_presentationId << ", programId "
               << in_programId;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

} // namespace qti::audio::core
