/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <cstddef>
#include <memory>
#define LOG_TAG "AHAL_Effect_VolumeListenerQti"

#include <Utils.h>
#include <android-base/logging.h>
#include <unordered_set>

#include "VolumeListener.h"

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::State;
using aidl::qti::effects::kMusicVolumeListenerUUID;
using aidl::qti::effects::kRingVolumeListenerUUID;
using aidl::qti::effects::kAlarmVolumeListenerUUID;
using aidl::qti::effects::kVoiceCallVolumeListenerUUID;
using aidl::qti::effects::kNotificationVolumeListenerUUID;
using aidl::qti::effects::VolumeListener;
using aidl::android::media::audio::common::AudioUuid;

bool isUuidSupported(const AudioUuid* uuid) {
    return (*uuid == kMusicVolumeListenerUUID || *uuid == kRingVolumeListenerUUID ||
            *uuid == kAlarmVolumeListenerUUID || *uuid == kVoiceCallVolumeListenerUUID ||
            *uuid == kNotificationVolumeListenerUUID);
}

extern "C" binder_exception_t createEffect(const AudioUuid* uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (uuid == nullptr || !isUuidSupported(uuid)) {
        LOG(ERROR) << __func__ << "uuid not supported";
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<VolumeListener>(*uuid);
        LOG(VERBOSE) << __func__ << " instance " << instanceSpp->get() << " created";
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
    if (*uuid == kAlarmVolumeListenerUUID) {
        *_aidl_return = aidl::qti::effects::kAlarmVolumeListenerDesc;
    } else if (*uuid == kMusicVolumeListenerUUID) {
        *_aidl_return = aidl::qti::effects::kMusicVolumeListenerDesc;
    } else if (*uuid == kNotificationVolumeListenerUUID) {
        *_aidl_return = aidl::qti::effects::kNotificationVolumeListenerDesc;
    } else if (*uuid == kVoiceCallVolumeListenerUUID) {
        *_aidl_return = aidl::qti::effects::kVoiceCallVolumeListenerDesc;
    } else if (*uuid == kRingVolumeListenerUUID) {
        *_aidl_return = aidl::qti::effects::kRingVolumeListenerDesc;
    }
    return EX_NONE;
}

namespace aidl::qti::effects {

VolumeListener::VolumeListener(const AudioUuid& uuid) {
    LOG(VERBOSE) << __func__ << toString(uuid);
    if (uuid == kAlarmVolumeListenerUUID) {
        mType = VolumeListenerType::ALARM;
        mDescriptor = &kAlarmVolumeListenerDesc;
        mEffectName = &kAlarmVolumeListenerEffectName;
    } else if (uuid == kMusicVolumeListenerUUID) {
        mType = VolumeListenerType::MUSIC;
        mDescriptor = &kMusicVolumeListenerDesc;
        mEffectName = &kMusicVolumeListenerEffectName;
    } else if (uuid == kNotificationVolumeListenerUUID) {
        mType = VolumeListenerType::NOTIFICATION;
        mDescriptor = &kNotificationVolumeListenerDesc;
        mEffectName = &kNotificationVolumeListenerEffectName;
    } else if (uuid == kVoiceCallVolumeListenerUUID) {
        mType = VolumeListenerType::VOICECALL;
        mDescriptor = &kVoiceCallVolumeListenerDesc;
        mEffectName = &kVoiceCallVolumeListenerEffectName;
    } else if (uuid == kRingVolumeListenerUUID) {
        mType = VolumeListenerType::RING;
        mDescriptor = &kRingVolumeListenerDesc;
        mEffectName = &kRingVolumeListenerEffectName;
    } else {
        LOG(ERROR) << __func__ << toString(uuid) << " not supported yet!";
    }
}

ndk::ScopedAStatus VolumeListener::getDescriptor(Descriptor* _aidl_return) {
    LOG(VERBOSE) << __func__ << (*mDescriptor).toString();
    *_aidl_return = *mDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VolumeListener::commandImpl(CommandId command) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    LOG(VERBOSE) << __func__ << toString(command);
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

std::shared_ptr<EffectContext> VolumeListener::createContext(const Parameter::Common& common,
                                                             bool processData) {
    LOG(VERBOSE) << __func__;
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        // GlobalVolumeListenerSession is a singleton
        mContext =
                GlobalVolumeListenerSession::getSession().createSession(mType, common, processData);
    }
    return mContext;
}

RetCode VolumeListener::releaseContext() {
    if (mContext) {
        GlobalVolumeListenerSession::getSession().releaseSession(mContext->getSessionId());
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

ndk::ScopedAStatus VolumeListener::setParameterSpecific(const Parameter::Specific& specific) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VolumeListener::getParameterSpecific(const Parameter::Id& id,
                                                        Parameter::Specific* specific) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VolumeListener::setParameterCommon(const Parameter& param) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto tag = param.getTag();
    switch (tag) {
        case Parameter::common:
            RETURN_IF(mContext->setCommon(param.get<Parameter::common>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setCommFailed");
            break;
        case Parameter::deviceDescription: {
            auto ret = GlobalVolumeListenerSession::getSession().setOutputDevice(
                    mContext->getSessionId(), param.get<Parameter::deviceDescription>());
            RETURN_IF(ret != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
        } break;
        case Parameter::mode:
            RETURN_IF(mContext->setAudioMode(param.get<Parameter::mode>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setModeFailed");
            break;
        case Parameter::source:
            RETURN_IF(mContext->setAudioSource(param.get<Parameter::source>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setSourceFailed");
            break;
        case Parameter::volumeStereo: {
            auto ret = GlobalVolumeListenerSession::getSession().setVolumeStereo(
                    mContext->getSessionId(), param.get<Parameter::volumeStereo>());
            RETURN_IF(ret != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
        }
            RETURN_IF(mContext->setVolumeStereo(param.get<Parameter::volumeStereo>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setVolumeStereoFailed");
            break;
        default: {
            LOG(ERROR) << __func__ << " unsupportedParameterTag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commonParamNotSupported");
        }
    }
    return ndk::ScopedAStatus::ok();
}

} // namespace aidl::qti::effects
