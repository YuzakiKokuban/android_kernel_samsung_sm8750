/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_VoiceProcessingQti"

#include "VoiceProcessing.h"
#include <Utils.h>
#include <android-base/logging.h>

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::qti::effects::kAcousticEchoCancelerQtiUUID;
using aidl::qti::effects::kNoiseSuppressionQtiUUID;
using aidl::android::media::audio::common::AudioUuid;
using aidl::qti::effects::VoiceProcessing;

bool isUuidSupported(const AudioUuid* uuid) {
    return (*uuid == kAcousticEchoCancelerQtiUUID || *uuid == kNoiseSuppressionQtiUUID);
}

extern "C" binder_exception_t createEffect(const AudioUuid* uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (!uuid || !isUuidSupported(uuid)) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<VoiceProcessing>(*uuid);
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" binder_exception_t queryEffect(const AudioUuid* uuid, Descriptor* _aidl_return) {
    if (uuid == nullptr || !isUuidSupported(uuid)) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (*uuid == kAcousticEchoCancelerQtiUUID) {
        *_aidl_return = aidl::qti::effects::kAcousticEchoCancelerDesc;
    } else if (*uuid == kNoiseSuppressionQtiUUID) {
        *_aidl_return = aidl::qti::effects::kNoiseSuppressionDesc;
    }
    return EX_NONE;
}

namespace aidl::qti::effects {

VoiceProcessing::VoiceProcessing(const AudioUuid& uuid) {
    LOG(DEBUG) << __func__ << toString(uuid);
    if (uuid == kAcousticEchoCancelerQtiUUID) {
        mType = VoiceProcessingType::AcousticEchoCanceler;
        mDescriptor = &kAcousticEchoCancelerDesc;
        mEffectName = &kAcousticEchoCancelerEffectName;
    } else if (uuid == kNoiseSuppressionQtiUUID) {
        mType = VoiceProcessingType::NoiseSuppression;
        mDescriptor = &kNoiseSuppressionDesc;
        mEffectName = &kNoiseSuppressionEffectName;
    } else {
        LOG(ERROR) << __func__ << uuid.toString() << " not supported yet!";
    }
}

ndk::ScopedAStatus VoiceProcessing::getDescriptor(Descriptor* _aidl_return) {
    RETURN_IF(!_aidl_return, EX_ILLEGAL_ARGUMENT, "Parameter:nullptr");

    *_aidl_return = *mDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VoiceProcessing::commandImpl(CommandId command) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    switch (command) {
        case CommandId::START:
            mContext->enable();
            break;
        case CommandId::STOP:
            mContext->disable();
            break;
        case CommandId::RESET:
            mContext->reset();
            break;
        default:
            LOG(ERROR) << __func__ << " commandId " << toString(command) << " not supported";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commandIdNotSupported");
    }
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> VoiceProcessing::createContext(const Parameter::Common& common,
                                                              bool processData) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        // GlobalVoiceProcessingSession is a singleton
        mContext = GlobalVoiceProcessingSession::getSession().createSession(mType, common,
                                                                            processData);
    }

    return mContext;
}

RetCode VoiceProcessing::releaseContext() {
    if (mContext) {
        GlobalVoiceProcessingSession::getSession().releaseSession(mType, mContext->getSessionId());
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

ndk::ScopedAStatus VoiceProcessing::setParameterSpecific(const Parameter::Specific& specific) {
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "ParamUnsupported");
}

ndk::ScopedAStatus VoiceProcessing::getParameterNoiseSuppression(const NoiseSuppression::Id& id,
                                                                 Parameter::Specific* specific) {
    RETURN_IF(id.getTag() != NoiseSuppression::Id::commonTag, EX_ILLEGAL_ARGUMENT,
              "NoiseSuppressionTagNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    NoiseSuppression param;

    auto tag = id.get<NoiseSuppression::Id::commonTag>();
    switch (tag) {
        case NoiseSuppression::level: {
            param.set<NoiseSuppression::level>(mContext->getNoiseSuppressionLevel());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "NoiseSuppressionTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::noiseSuppression>(param);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VoiceProcessing::getParameterAcousticEchoCanceler(
        const AcousticEchoCanceler::Id& id, Parameter::Specific* specific) {
    RETURN_IF(id.getTag() != AcousticEchoCanceler::Id::commonTag, EX_ILLEGAL_ARGUMENT,
              "AcousticEchoCancelerTagNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    AcousticEchoCanceler param;
    auto tag = id.get<AcousticEchoCanceler::Id::commonTag>();
    switch (tag) {
        case AcousticEchoCanceler::echoDelayUs: {
            param.set<AcousticEchoCanceler::echoDelayUs>(
                    mContext->getAcousticEchoCancelerEchoDelay());
            break;
        }
        case AcousticEchoCanceler::mobileMode: {
            param.set<AcousticEchoCanceler::mobileMode>(
                    mContext->getAcousticEchoCancelerMobileMode());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "AcousticEchoCancelerTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::acousticEchoCanceler>(param);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VoiceProcessing::getParameterSpecific(const Parameter::Id& id,
                                                         Parameter::Specific* specific) {
    RETURN_IF(!specific, EX_NULL_POINTER, "nullPtr");
    auto tag = id.getTag();

    switch (tag) {
        case Parameter::Id::acousticEchoCancelerTag:
            return getParameterAcousticEchoCanceler(
                    id.get<Parameter::Id::acousticEchoCancelerTag>(), specific);
        case Parameter::Id::noiseSuppressionTag:
            return getParameterNoiseSuppression(id.get<Parameter::Id::noiseSuppressionTag>(),
                                                specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "wrongIdTag");
    }
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                            "ParamUnsupported");
}

// Processing method running in EffectWorker thread.
IEffect::Status VoiceProcessing::effectProcessImpl(float* in, float* out, int sampleToProcess) {
    if (!mContext) {
        LOG(ERROR) << __func__ << " nullContext";
        return {EX_NULL_POINTER, 0, 0};
    }
    for (int i = 0; i < sampleToProcess; i++) {
        *out++ = *in++;
    }
    return {STATUS_OK, sampleToProcess, sampleToProcess};
}

} // namespace aidl::qti::effects