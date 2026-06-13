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

#define LOG_TAG "AHAL_EffectImplQti"

#include "effect-impl/EffectImpl.h"
#include <memory>
#include "effect-impl/EffectTypes.h"
#include "include/effect-impl/EffectTypes.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kEventFlagDataMqNotEmpty;
using aidl::android::hardware::audio::effect::kReopenSupportedVersion;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::PcmType;
using ::android::hardware::EventFlag;
using ::aidl::android::hardware::audio::effect::kEventFlagNotEmpty;

extern "C" binder_exception_t destroyEffect(const std::shared_ptr<IEffect>& instanceSp) {
    State state;
    ndk::ScopedAStatus status = instanceSp->getState(&state);
    if (!status.isOk() || State::INIT != state) {
        LOG(ERROR) << __func__ << " instance " << instanceSp.get()
                   << " in state: " << toString(state) << ", status: " << status.getDescription();
        return EX_ILLEGAL_STATE;
    }
    LOG(VERBOSE) << __func__ << " instance " << instanceSp.get() << " destroyed";
    return EX_NONE;
}

namespace aidl::qti::effects {

ndk::ScopedAStatus EffectImpl::open(const Parameter::Common& common,
                                    const std::optional<Parameter::Specific>& specific,
                                    OpenEffectReturn* ret) {
    LOG(DEBUG) << getEffectName() << "  " << __func__ << " sessionId " << common.session
               << " ioHandle " << common.ioHandle;
    // effect only support 32bits float
    RETURN_IF(common.input.base.format.pcm != common.output.base.format.pcm ||
                      common.input.base.format.pcm != PcmType::FLOAT_32_BIT,
              EX_ILLEGAL_ARGUMENT, "dataMustBe32BitsFloat");
    std::lock_guard lg(mImplMutex);
    RETURN_OK_IF(mState != State::INIT);
    // check if the effects needs data processing or not, based on that init worker thread & FMQs
    mProcessData = !isOffloadOrBypass();
    mImplContext = createContext(common, mProcessData);
    RETURN_IF(!mImplContext, EX_NULL_POINTER, "nullContext");

    RETURN_IF(!getInterfaceVersion(&mVersion).isOk(), EX_UNSUPPORTED_OPERATION,
              "FailedToGetInterfaceVersion");
    mImplContext->setVersion(mVersion);
    mEventFlag = mImplContext->getStatusEventFlag();
    mDataMqNotEmptyEf =
            mVersion >= kReopenSupportedVersion ? kEventFlagDataMqNotEmpty : kEventFlagNotEmpty;

    if (specific.has_value()) {
        RETURN_IF_ASTATUS_NOT_OK(setParameterSpecific(specific.value()), "setSpecParamErr");
    }

    mState = State::IDLE;

    if (mProcessData) {
        mEventFlag = mImplContext->getStatusEventFlag();
        mImplContext->dupeFmq(ret);
        RETURN_IF(createThread(getEffectName()) != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
                  "FailedToCreateWorker");
    } else {
        LOG(VERBOSE) << __func__ << " " << getEffectName() << " effect does not process data";
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::reopen(OpenEffectReturn* ret) {
    std::lock_guard lg(mImplMutex);
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "alreadyClosed");

    LOG(DEBUG) << getEffectName() << "  " << __func__;
    // TODO:  add reopen implementation
    RETURN_IF(!mImplContext, EX_NULL_POINTER, "nullContext");
    mImplContext->dupeFmqWithReopen(ret);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::close() {
    {
        std::lock_guard lg(mImplMutex);
        RETURN_OK_IF(mState == State::INIT);
        RETURN_IF(mState == State::PROCESSING, EX_ILLEGAL_STATE, "closeAtProcessing");
        mState = State::INIT;
    }

    RETURN_IF(notifyEventFlag(mDataMqNotEmptyEf) != RetCode::SUCCESS, EX_ILLEGAL_STATE,
              "notifyEventFlagFailed");
    // stop the worker thread, ignore the return code
    RETURN_IF(destroyThread() != RetCode::SUCCESS, EX_UNSUPPORTED_OPERATION,
              "FailedToDestroyWorker");

    {
        std::lock_guard lg(mImplMutex);
        releaseContext();
        mImplContext.reset();
    }

    LOG(DEBUG) << getEffectName() << "  " << __func__;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameter(const Parameter& param) {
    std::lock_guard lg(mImplMutex);
    LOG(VERBOSE) << getEffectName() << __func__ << " with: " << param.toString();

    const auto& tag = param.getTag();
    switch (tag) {
        case Parameter::common:
        case Parameter::deviceDescription:
        case Parameter::mode:
        case Parameter::source:
        case Parameter::offload:
            FALLTHROUGH_INTENDED;
        case Parameter::volumeStereo:
            return setParameterCommon(param);
        case Parameter::specific: {
            return setParameterSpecific(param.get<Parameter::specific>());
        }
        default: {
            LOG(DEBUG) << getEffectName() << "  " << __func__ << " unsupportedParameterTag "
                       << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "ParameterNotSupported");
        }
    }
}

ndk::ScopedAStatus EffectImpl::getParameter(const Parameter::Id& id, Parameter* param) {
    std::lock_guard lg(mImplMutex);
    LOG(VERBOSE) << getEffectName() << "  " << __func__ << id.toString();
    auto tag = id.getTag();
    switch (tag) {
        case Parameter::Id::commonTag: {
            RETURN_IF_ASTATUS_NOT_OK(getParameterCommon(id.get<Parameter::Id::commonTag>(), param),
                                     "CommonParamNotSupported");
            break;
        }
        case Parameter::Id::vendorEffectTag:
            FALLTHROUGH_INTENDED;
        default: {
            Parameter::Specific specific;
            RETURN_IF_ASTATUS_NOT_OK(getParameterSpecific(id, &specific), "SpecParamNotSupported");
            param->set<Parameter::specific>(specific);
            break;
        }
    }
    LOG(VERBOSE) << getEffectName() << __func__ << id.toString() << param->toString();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::setParameterCommon(const Parameter& param) {
    RETURN_IF(!mImplContext, EX_NULL_POINTER, "nullContext");

    const auto& tag = param.getTag();

    LOG(VERBOSE) << getEffectName() << __func__ << param.toString();
    switch (tag) {
        case Parameter::common:
            RETURN_IF(mImplContext->setCommon(param.get<Parameter::common>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setCommFailed");
            break;
        case Parameter::deviceDescription:
            RETURN_IF(mImplContext->setOutputDevice(param.get<Parameter::deviceDescription>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
            break;
        case Parameter::mode:
            RETURN_IF(mImplContext->setAudioMode(param.get<Parameter::mode>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setModeFailed");
            break;
        case Parameter::source:
            RETURN_IF(mImplContext->setAudioSource(param.get<Parameter::source>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setSourceFailed");
            break;
        case Parameter::volumeStereo:
            RETURN_IF(mImplContext->setVolumeStereo(param.get<Parameter::volumeStereo>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setVolumeStereoFailed");
            break;
        case Parameter::offload:
            RETURN_IF(mImplContext->setOffload(param.get<Parameter::offload>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setOffloadError");
            break;
        default: {
            LOG(DEBUG) << getEffectName() << "  " << __func__ << " unsupportedParameterTag "
                       << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commonParamNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getParameterCommon(const Parameter::Tag& tag, Parameter* param) {
    RETURN_IF(!mImplContext, EX_NULL_POINTER, "nullContext");

    switch (tag) {
        case Parameter::common: {
            param->set<Parameter::common>(mImplContext->getCommon());
            break;
        }
        case Parameter::deviceDescription: {
            param->set<Parameter::deviceDescription>(mImplContext->getOutputDevice());
            break;
        }
        case Parameter::mode: {
            param->set<Parameter::mode>(mImplContext->getAudioMode());
            break;
        }
        case Parameter::source: {
            param->set<Parameter::source>(mImplContext->getAudioSource());
            break;
        }
        case Parameter::volumeStereo: {
            param->set<Parameter::volumeStereo>(mImplContext->getVolumeStereo());
            break;
        }
        default: {
            LOG(DEBUG) << getEffectName() << "  " << __func__ << " unsupported tag "
                       << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "tagNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::getState(State* state) {
    *state = mState;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::command(CommandId command) {
    std::lock_guard lg(mImplMutex);
    RETURN_IF(mState == State::INIT, EX_ILLEGAL_STATE, "instanceNotOpen");
    LOG(DEBUG) << getEffectName() << "  " << __func__ << ": receive command: " << toString(command)
               << " at state " << toString(mState);

    switch (command) {
        case CommandId::START:
            RETURN_OK_IF(mState == State::PROCESSING);
            RETURN_IF_ASTATUS_NOT_OK(commandImpl(command), "commandImplFailed");
            mState = State::PROCESSING;
            RETURN_IF(notifyEventFlag(mDataMqNotEmptyEf) != RetCode::SUCCESS, EX_ILLEGAL_STATE,
                      "notifyEventFlagFailed");
            startThread();
            break;
        case CommandId::STOP:
        case CommandId::RESET:
            RETURN_OK_IF(mState == State::IDLE);
            mState = State::IDLE;
            RETURN_IF(notifyEventFlag(mDataMqNotEmptyEf) != RetCode::SUCCESS, EX_ILLEGAL_STATE,
                      "notifyEventFlagFailed");
            stopThread();
            RETURN_IF_ASTATUS_NOT_OK(commandImpl(command), "commandImplFailed");
            break;
        default:
            LOG(DEBUG) << getEffectName() << "  " << __func__ << " instance still processing";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "CommandIdNotSupported");
    }
    LOG(DEBUG) << getEffectName() << "  " << __func__ << " transfer to state: " << toString(mState);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus EffectImpl::commandImpl(CommandId command) {
    RETURN_IF(!mImplContext, EX_NULL_POINTER, "nullContext");
    if (command == CommandId::RESET) {
        mImplContext->resetBuffer();
    }
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> EffectImpl::createContext(const Parameter::Common& common,
                                                         bool processData) {
    return std::make_shared<EffectContext>(common, processData);
}

RetCode EffectImpl::releaseContext() {
    if (mImplContext) {
        mImplContext.reset();
    }
    return RetCode::SUCCESS;
}

void EffectImpl::cleanUp() {
    command(CommandId::STOP);
    close();
}

RetCode EffectImpl::notifyEventFlag(uint32_t flag) {
    if (!mProcessData) {
        return RetCode::SUCCESS;
    }

    if (!mEventFlag) {
        LOG(ERROR) << getEffectName() << __func__ << ": StatusEventFlag invalid";
        return RetCode::ERROR_EVENT_FLAG_ERROR;
    }
    if (const auto ret = mEventFlag->wake(flag); ret != ::android::OK) {
        LOG(ERROR) << getEffectName() << __func__ << ": wake failure with ret " << ret;
        return RetCode::ERROR_EVENT_FLAG_ERROR;
    }
    return RetCode::SUCCESS;
}

IEffect::Status EffectImpl::status(binder_status_t status, size_t consumed, size_t produced) {
    IEffect::Status ret;
    ret.status = status;
    ret.fmqConsumed = consumed;
    ret.fmqProduced = produced;
    return ret;
}

void EffectImpl::process() {
    // ATRACE_CALL();
    /**
     * wait for the EventFlag without lock, it's ok because the mEfGroup pointer will not change
     * in the life cycle of workerThread (threadLoop).
     */
    uint32_t efState = 0;
    if (!mEventFlag ||
        ::android::OK != mEventFlag->wait(mDataMqNotEmptyEf, &efState, 0 /* no timeout */,
                                          true /* retry */) ||
        !(efState & mDataMqNotEmptyEf)) {
        LOG(ERROR) << getEffectName() << __func__ << ": StatusEventFlag - " << mEventFlag
                   << " efState - " << std::hex << efState;
        return;
    }

    {
        std::lock_guard lg(mImplMutex);
        if (mState != State::PROCESSING) {
            LOG(DEBUG) << getEffectName() << " skip process in state: " << toString(mState);
            return;
        }
        RETURN_VALUE_IF(!mImplContext, void(), "nullContext");
        auto statusMQ = mImplContext->getStatusFmq();
        auto inputMQ = mImplContext->getInputDataFmq();
        auto outputMQ = mImplContext->getOutputDataFmq();
        auto buffer = mImplContext->getWorkBuffer();
        if (!inputMQ || !outputMQ) {
            return;
        }

        assert(mImplContext->getWorkBufferSize() >=
               std::max(inputMQ->availableToRead(), outputMQ->availableToWrite()));
        auto processSamples = std::min(inputMQ->availableToRead(), outputMQ->availableToWrite());
        if (processSamples) {
            inputMQ->read(buffer, processSamples);
            IEffect::Status status = effectProcessImpl(buffer, buffer, processSamples);
            outputMQ->write(buffer, status.fmqProduced);
            statusMQ->writeBlocking(&status, 1);
            LOG(VERBOSE) << getEffectName() << __func__ << ": done processing, effect consumed "
                         << status.fmqConsumed << " produced " << status.fmqProduced;
        }
    }
}

// A placeholder processing implementation to copy samples from input to output
IEffect::Status EffectImpl::effectProcessImpl(float* in, float* out, int samples) {
    for (int i = 0; i < samples; i++) {
        *out++ = *in++;
    }
    LOG(VERBOSE) << getEffectName() << __func__ << " done processing " << samples << " samples";
    return {STATUS_OK, samples, samples};
}

} // namespace aidl::qti::effects
