/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Effect_OffloadBundleQti"
#include <Utils.h>
#include <algorithm>
#include <unordered_set>

#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include "OffloadBundleAidl.h"

using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::qti::effects::OffloadBundleAidl;
using aidl::qti::effects::kEqualizerOffloadQtiUUID;
using aidl::qti::effects::kBassBoostOffloadQtiUUID;
using aidl::qti::effects::kVirtualizerOffloadQtiUUID;
using aidl::qti::effects::kAuxEnvReverbOffloadQtiUUID;
using aidl::qti::effects::kInsertEnvReverbOffloadQtiUUID;
using aidl::qti::effects::kAuxPresetReverbOffloadQtiUUID;
using aidl::qti::effects::kInsertPresetReverbOffloadQtiUUID;

using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioUuid;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::BassBoost;
using aidl::android::hardware::audio::effect::Virtualizer;
using aidl::android::hardware::audio::effect::Parameter;

bool isUuidSupported(const AudioUuid* uuid) {
    return (*uuid == kEqualizerOffloadQtiUUID || *uuid == kBassBoostOffloadQtiUUID ||
            *uuid == kVirtualizerOffloadQtiUUID || *uuid == kAuxEnvReverbOffloadQtiUUID ||
            *uuid == kInsertEnvReverbOffloadQtiUUID || *uuid == kAuxPresetReverbOffloadQtiUUID ||
            *uuid == kInsertPresetReverbOffloadQtiUUID);
}

extern "C" binder_exception_t createEffect(
        const AudioUuid* uuid,
        std::shared_ptr<aidl::android::hardware::audio::effect::IEffect>* instanceSpp) {
    if (uuid == nullptr || !isUuidSupported(uuid)) {
        LOG(ERROR) << __func__ << "uuid not supported " << aidl::qti::effects::toString(*uuid);
        return EX_ILLEGAL_ARGUMENT;
    }
    if (instanceSpp) {
        *instanceSpp = ndk::SharedRefBase::make<OffloadBundleAidl>(*uuid);
        LOG(DEBUG) << __func__ << " instance " << instanceSpp->get() << " created";
        return EX_NONE;
    } else {
        LOG(ERROR) << __func__ << " invalid input parameter!";
        return EX_ILLEGAL_ARGUMENT;
    }
}

extern "C" void startEffect(int ioHandle, uint64_t* palHandle) {
    aidl::qti::effects::GlobalOffloadSession::getGlobalSession().startEffect(ioHandle, palHandle);
}

extern "C" void stopEffect(int ioHandle) {
    aidl::qti::effects::GlobalOffloadSession::getGlobalSession().stopEffect(ioHandle);
}

extern "C" binder_exception_t queryEffect(
        const AudioUuid* in_impl_uuid,
        aidl::android::hardware::audio::effect::Descriptor* _aidl_return) {
    if (!in_impl_uuid || !isUuidSupported(in_impl_uuid)) {
        LOG(ERROR) << __func__ << "uuid not supported "
                   << aidl::qti::effects::toString(*in_impl_uuid);
        return EX_ILLEGAL_ARGUMENT;
    }
    if (*in_impl_uuid == kEqualizerOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kEqualizerDesc;
    } else if (*in_impl_uuid == kBassBoostOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kBassBoostDesc;
    } else if (*in_impl_uuid == kVirtualizerOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kVirtualizerDesc;
    } else if (*in_impl_uuid == kAuxEnvReverbOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kAuxEnvReverbDesc;
    } else if (*in_impl_uuid == kInsertEnvReverbOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kInsertEnvReverbDesc;
    } else if (*in_impl_uuid == kAuxPresetReverbOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kAuxPresetReverbDesc;
    } else if (*in_impl_uuid == kInsertPresetReverbOffloadQtiUUID) {
        *_aidl_return = aidl::qti::effects::kInsertPresetReverbDesc;
    }
    return EX_NONE;
}

namespace aidl::qti::effects {

OffloadBundleAidl::OffloadBundleAidl(const AudioUuid& uuid) {
    if (uuid == kEqualizerOffloadQtiUUID) {
        mType = OffloadBundleEffectType::EQUALIZER;
        mDescriptor = &kEqualizerDesc;
        mEffectName = &kEqualizerEffectName;
    } else if (uuid == kBassBoostOffloadQtiUUID) {
        mType = OffloadBundleEffectType::BASS_BOOST;
        mDescriptor = &kBassBoostDesc;
        mEffectName = &kBassBoostEffectName;
    } else if (uuid == kVirtualizerOffloadQtiUUID) {
        mType = OffloadBundleEffectType::VIRTUALIZER;
        mDescriptor = &kVirtualizerDesc;
        mEffectName = &kVirtualizerEffectName;
    } else if (uuid == kAuxEnvReverbOffloadQtiUUID) {
        mType = OffloadBundleEffectType::AUX_ENV_REVERB;
        mDescriptor = &kAuxEnvReverbDesc;
        mEffectName = &kAuxEnvReverbEffectName;
    } else if (uuid == kInsertEnvReverbOffloadQtiUUID) {
        mType = OffloadBundleEffectType::INSERT_ENV_REVERB;
        mDescriptor = &kInsertEnvReverbDesc;
        mEffectName = &kInsertEnvReverbEffectName;
    } else if (uuid == kAuxPresetReverbOffloadQtiUUID) {
        mType = OffloadBundleEffectType::AUX_PRESET_REVERB;
        mDescriptor = &kAuxPresetReverbDesc;
        mEffectName = &kAuxPresetReverbEffectName;
    } else if (uuid == kInsertPresetReverbOffloadQtiUUID) {
        mType = OffloadBundleEffectType::INSERT_PRESET_REVERB;
        mDescriptor = &kInsertPresetReverbDesc;
        mEffectName = &kInsertPresetReverbEffectName;
    } else {
        LOG(ERROR) << __func__ << toString(uuid) << " not supported!";
    }
    LOG(DEBUG) << __func__ << "  " << toString(uuid) << " " << mType;
}

OffloadBundleAidl::~OffloadBundleAidl() {
    cleanUp();
    LOG(DEBUG) << __func__ << mType;
}

ndk::ScopedAStatus OffloadBundleAidl::getDescriptor(
        aidl::android::hardware::audio::effect::Descriptor* _aidl_return) {
    RETURN_IF(!_aidl_return, EX_ILLEGAL_ARGUMENT, "Parameter:nullptr");
    LOG(DEBUG) << _aidl_return->toString();
    *_aidl_return = *mDescriptor;
    return ndk::ScopedAStatus::ok();
}

void OffloadBundleAidl::stopEffectIfNeeded(const Parameter::Common& common) {
    int ioHandle = common.ioHandle;
    int previousHandle = mContext->getIoHandle();
    if (ioHandle != previousHandle) {
        LOG(DEBUG) << getEffectName() << __func__ << " stop on previous handle " << previousHandle
                   << " new handle " << ioHandle;
        aidl::qti::effects::GlobalOffloadSession::getGlobalSession().stopEffect(previousHandle);
    }
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterCommon(const Parameter& param) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    const auto& tag = param.getTag();

    LOG(VERBOSE) << mType << " " << __func__ << param.toString();
    if (tag == Parameter::common) {
        stopEffectIfNeeded(param.get<Parameter::common>());
        RETURN_IF(mContext->setCommon(param.get<Parameter::common>()) != RetCode::SUCCESS,
                  EX_ILLEGAL_ARGUMENT, "setCommFailed");
    } else {
        // for rest of params use base class.
        return EffectImpl::setParameterCommon(param);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterSpecific(const Parameter::Specific& specific) {
    LOG(DEBUG) << __func__ << " specific " << specific.toString();
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");

    auto tag = specific.getTag();
    switch (tag) {
        case Parameter::Specific::equalizer:
            return setParameterEqualizer(specific);
        case Parameter::Specific::bassBoost:
            return setParameterBassBoost(specific);
        case Parameter::Specific::virtualizer:
            return setParameterVirtualizer(specific);
        case Parameter::Specific::presetReverb:
            return setParameterPresetReverb(specific);
        case Parameter::Specific::environmentalReverb:
            return setParameterEnvironmentalReverb(specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "specificParamNotSupported");
    }
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterEqualizer(const Parameter::Specific& specific) {
    auto& eq = specific.get<Parameter::Specific::equalizer>();
    RETURN_IF(!inRange(eq, kEqRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto eqTag = eq.getTag();
    switch (eqTag) {
        case Equalizer::preset:
            RETURN_IF(mContext->setEqualizerPreset(eq.get<Equalizer::preset>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBandLevelsFailed");
            return ndk::ScopedAStatus::ok();
        case Equalizer::bandLevels:
            RETURN_IF(mContext->setEqualizerBandLevels(eq.get<Equalizer::bandLevels>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBandLevelsFailed");
            return ndk::ScopedAStatus::ok();
        default:
            LOG(ERROR) << __func__ << " unsupported parameter " << specific.toString();
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "eqTagNotSupported");
    }
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterBassBoost(const Parameter::Specific& specific) {
    auto& bb = specific.get<Parameter::Specific::bassBoost>();
    auto bbTag = bb.getTag();
    RETURN_IF(!inRange(bb, kBassBoostRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    switch (bbTag) {
        case BassBoost::strengthPm: {
            RETURN_IF(mContext->setBassBoostStrength(bb.get<BassBoost::strengthPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setStrengthFailed");
            return ndk::ScopedAStatus::ok();
        }
        default:
            LOG(ERROR) << __func__ << " unsupported parameter " << specific.toString();
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "bbTagNotSupported");
    }
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterVirtualizer(const Parameter::Specific& specific) {
    auto& vr = specific.get<Parameter::Specific::virtualizer>();
    RETURN_IF(!inRange(vr, kVirtualizerRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto vrTag = vr.getTag();
    switch (vrTag) {
        case Virtualizer::strengthPm: {
            RETURN_IF(mContext->setVirtualizerStrength(vr.get<Virtualizer::strengthPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setStrengthFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Virtualizer::device: {
            RETURN_IF(mContext->setForcedDevice(vr.get<Virtualizer::device>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDeviceFailed");
            return ndk::ScopedAStatus::ok();
        }
        case Virtualizer::speakerAngles:
            FALLTHROUGH_INTENDED;
        case Virtualizer::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(vrTag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VirtualizerTagNotSupported");
        }
        default:
            LOG(ERROR) << __func__ << " unsupported parameter " << specific.toString();
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "vrTagNotSupported");
    }
}

ndk::ScopedAStatus OffloadBundleAidl::getParameterSpecific(const Parameter::Id& id,
                                                           Parameter::Specific* specific) {
    RETURN_IF(!specific, EX_NULL_POINTER, "nullPtr");
    auto tag = id.getTag();

    switch (tag) {
        case Parameter::Id::equalizerTag:
            return getParameterEqualizer(id.get<Parameter::Id::equalizerTag>(), specific);
        case Parameter::Id::bassBoostTag:
            return getParameterBassBoost(id.get<Parameter::Id::bassBoostTag>(), specific);
        case Parameter::Id::virtualizerTag:
            return getParameterVirtualizer(id.get<Parameter::Id::virtualizerTag>(), specific);
        case Parameter::Id::environmentalReverbTag:
            return getParameterEnvironmentalReverb(id.get<Parameter::Id::environmentalReverbTag>(),
                                                   specific);
        case Parameter::Id::presetReverbTag:
            return getParameterPresetReverb(id.get<Parameter::Id::presetReverbTag>(), specific);
        default:
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "wrongIdTag");
    }
}

ndk::ScopedAStatus OffloadBundleAidl::getParameterEqualizer(const Equalizer::Id& id,
                                                            Parameter::Specific* specific) {
    RETURN_IF(id.getTag() != Equalizer::Id::commonTag, EX_ILLEGAL_ARGUMENT,
              "EqualizerTagNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    Equalizer eqParam;

    auto tag = id.get<Equalizer::Id::commonTag>();
    switch (tag) {
        case Equalizer::bandLevels: {
            eqParam.set<Equalizer::bandLevels>(mContext->getEqualizerBandLevels());
            break;
        }
        case Equalizer::preset: {
            eqParam.set<Equalizer::preset>(mContext->getEqualizerPreset());
            break;
        }
        case Equalizer::bandFrequencies: {
            eqParam.set<Equalizer::bandFrequencies>(kBandFrequencies);
            break;
        }
        case Equalizer::presets: {
            eqParam.set<Equalizer::presets>(kPresets);
            break;
        }
        case Equalizer::centerFreqMh: {
            eqParam.set<Equalizer::centerFreqMh>(mContext->getEqualizerCenterFreqs());
            break;
        }
        case Equalizer::vendor: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "unsupportedTag");
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "unsupportedTag");
        }
    }

    specific->set<Parameter::Specific::equalizer>(eqParam);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus OffloadBundleAidl::getParameterBassBoost(const BassBoost::Id& id,
                                                            Parameter::Specific* specific) {
    RETURN_IF(id.getTag() != BassBoost::Id::commonTag, EX_ILLEGAL_ARGUMENT,
              "BassBoostTagNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    BassBoost bbParam;

    auto tag = id.get<BassBoost::Id::commonTag>();
    switch (tag) {
        case BassBoost::strengthPm: {
            bbParam.set<BassBoost::strengthPm>(mContext->getBassBoostStrength());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " not handled tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "BassBoostTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::bassBoost>(bbParam);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus OffloadBundleAidl::getParameterVirtualizer(const Virtualizer::Id& id,
                                                              Parameter::Specific* specific) {
    RETURN_IF((id.getTag() != Virtualizer::Id::commonTag) &&
                      (id.getTag() != Virtualizer::Id::speakerAnglesPayload),
              EX_ILLEGAL_ARGUMENT, "VirtualizerTagNotSupported");

    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    Virtualizer vrParam;

    if (id.getTag() == Virtualizer::Id::speakerAnglesPayload) {
        auto angles = mContext->getSpeakerAngles(id.get<Virtualizer::Id::speakerAnglesPayload>());
        RETURN_IF(angles.size() == 0, EX_ILLEGAL_ARGUMENT, "getSpeakerAnglesFailed");
        Virtualizer param = Virtualizer::make<Virtualizer::speakerAngles>(angles);
        specific->set<Parameter::Specific::virtualizer>(param);
        return ndk::ScopedAStatus::ok();
    }

    auto tag = id.get<Virtualizer::Id::commonTag>();
    switch (tag) {
        case Virtualizer::strengthPm: {
            vrParam.set<Virtualizer::strengthPm>(mContext->getVirtualizerStrength());
            break;
        }
        case Virtualizer::device: {
            vrParam.set<Virtualizer::device>(mContext->getForcedDevice());
            break;
        }
        case Virtualizer::speakerAngles:
            FALLTHROUGH_INTENDED;
        case Virtualizer::vendor: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "VirtualizerTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::virtualizer>(vrParam);
    return ndk::ScopedAStatus::ok();

}

std::shared_ptr<EffectContext> OffloadBundleAidl::createContext(const Parameter::Common& common,
                                                                bool processData) {
    if (mContext) {
        LOG(DEBUG) << __func__ << " context already exist";
    } else {
        // GlobalSession is a singleton
        mContext =
                GlobalOffloadSession::getGlobalSession().createSession(mType, common, processData);
    }

    return mContext;
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterPresetReverb(
        const Parameter::Specific& specific) {
    auto& presetReverbParam = specific.get<Parameter::Specific::presetReverb>();
    auto tag = presetReverbParam.getTag();
    RETURN_IF(!inRange(presetReverbParam, kPresetReverbRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    switch (tag) {
        case PresetReverb::preset: {
            RETURN_IF(mContext->setPresetReverbPreset(
                              presetReverbParam.get<PresetReverb::preset>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setPresetFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "PresetReverbTagNotSupported");
        }
    }
}

ndk::ScopedAStatus OffloadBundleAidl::setParameterEnvironmentalReverb(
        const Parameter::Specific& specific) {
    auto& reverbParam = specific.get<Parameter::Specific::environmentalReverb>();
    RETURN_IF(!inRange(reverbParam, kEnvReverbRanges), EX_ILLEGAL_ARGUMENT, "outOfRange");
    auto tag = reverbParam.getTag();

    switch (tag) {
        case EnvironmentalReverb::roomLevelMb: {
            RETURN_IF(mContext->setEnvironmentalReverbRoomLevel(
                              reverbParam.get<EnvironmentalReverb::roomLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::roomHfLevelMb: {
            RETURN_IF(mContext->setEnvironmentalReverbRoomHfLevel(
                              reverbParam.get<EnvironmentalReverb::roomHfLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setRoomHfLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::decayTimeMs: {
            RETURN_IF(mContext->setEnvironmentalReverbDecayTime(
                              reverbParam.get<EnvironmentalReverb::decayTimeMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayTimeFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::decayHfRatioPm: {
            RETURN_IF(mContext->setEnvironmentalReverbDecayHfRatio(
                              reverbParam.get<EnvironmentalReverb::decayHfRatioPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDecayHfRatioFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::reflectionsLevelMb: {
            RETURN_IF(mContext->setReflectionsLevel(
                              reverbParam.get<EnvironmentalReverb::reflectionsLevelMb>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setReflectionsLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::reflectionsDelayMs: {
            RETURN_IF(mContext->setReflectionsDelay(
                              reverbParam.get<EnvironmentalReverb::reflectionsDelayMs>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setReflectionsDelayFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::levelMb: {
            RETURN_IF(mContext->setEnvironmentalReverbLevel(
                              reverbParam.get<EnvironmentalReverb::levelMb>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setLevelFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::delayMs: {
            RETURN_IF(mContext->setEnvironmentalReverbDelay(
                              reverbParam.get<EnvironmentalReverb::delayMs>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDelayFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::diffusionPm: {
            RETURN_IF(mContext->setEnvironmentalReverbDiffusion(
                              reverbParam.get<EnvironmentalReverb::diffusionPm>()) !=
                              RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setDiffusionFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::densityPm: {
            RETURN_IF(
                    mContext->setEnvironmentalReverbDensity(
                            reverbParam.get<EnvironmentalReverb::densityPm>()) != RetCode::SUCCESS,
                    EX_ILLEGAL_ARGUMENT, "setDensityFailed");
            return ndk::ScopedAStatus::ok();
        }
        case EnvironmentalReverb::bypass: {
            RETURN_IF(mContext->setEnvironmentalReverbBypass(
                              reverbParam.get<EnvironmentalReverb::bypass>()) != RetCode::SUCCESS,
                      EX_ILLEGAL_ARGUMENT, "setBypassFailed");
            return ndk::ScopedAStatus::ok();
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "EnvironmentalReverbTagNotSupported");
        }
    }
}

ndk::ScopedAStatus OffloadBundleAidl::getParameterPresetReverb(const PresetReverb::Id& id,
                                                               Parameter::Specific* specific) {
    RETURN_IF(id.getTag() != PresetReverb::Id::commonTag, EX_ILLEGAL_ARGUMENT,
              "PresetReverbTagNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    PresetReverb prParam;
    auto tag = id.get<PresetReverb::Id::commonTag>();
    switch (tag) {
        case PresetReverb::preset: {
            prParam.set<PresetReverb::preset>(mContext->getPresetReverbPreset());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "PresetReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::presetReverb>(prParam);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus OffloadBundleAidl::getParameterEnvironmentalReverb(
        const EnvironmentalReverb::Id& id, Parameter::Specific* specific) {
    RETURN_IF(id.getTag() != EnvironmentalReverb::Id::commonTag, EX_ILLEGAL_ARGUMENT,
              "EnvironmentalReverbTagNotSupported");
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    EnvironmentalReverb envReverbParam;

    auto tag = id.get<EnvironmentalReverb::Id::commonTag>();
    switch (tag) {
        case EnvironmentalReverb::roomLevelMb: {
            envReverbParam.set<EnvironmentalReverb::roomLevelMb>(
                    mContext->getEnvironmentalReverbRoomLevel());
            break;
        }
        case EnvironmentalReverb::roomHfLevelMb: {
            envReverbParam.set<EnvironmentalReverb::roomHfLevelMb>(
                    mContext->getEnvironmentalReverbRoomHfLevel());
            break;
        }
        case EnvironmentalReverb::decayTimeMs: {
            envReverbParam.set<EnvironmentalReverb::decayTimeMs>(
                    mContext->getEnvironmentalReverbDecayTime());
            break;
        }
        case EnvironmentalReverb::decayHfRatioPm: {
            envReverbParam.set<EnvironmentalReverb::decayHfRatioPm>(
                    mContext->getEnvironmentalReverbDecayHfRatio());
            break;
        }
        case EnvironmentalReverb::reflectionsLevelMb: {
            envReverbParam.set<EnvironmentalReverb::reflectionsLevelMb>(
                    mContext->getReflectionsLevel());
            break;
        }
        case EnvironmentalReverb::reflectionsDelayMs: {
            envReverbParam.set<EnvironmentalReverb::reflectionsDelayMs>(
                    mContext->getReflectionsDelay());
            break;
        }
        case EnvironmentalReverb::levelMb: {
            envReverbParam.set<EnvironmentalReverb::levelMb>(
                    mContext->getEnvironmentalReverbLevel());
            break;
        }
        case EnvironmentalReverb::delayMs: {
            envReverbParam.set<EnvironmentalReverb::delayMs>(
                    mContext->getEnvironmentalReverbDelay());
            break;
        }
        case EnvironmentalReverb::diffusionPm: {
            envReverbParam.set<EnvironmentalReverb::diffusionPm>(
                    mContext->getEnvironmentalReverbDiffusion());
            break;
        }
        case EnvironmentalReverb::densityPm: {
            envReverbParam.set<EnvironmentalReverb::densityPm>(
                    mContext->getEnvironmentalReverbDensity());
            break;
        }
        case EnvironmentalReverb::bypass: {
            envReverbParam.set<EnvironmentalReverb::bypass>(
                    mContext->getEnvironmentalReverbBypass());
            break;
        }
        default: {
            LOG(ERROR) << __func__ << " unsupported tag: " << toString(tag);
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(
                    EX_ILLEGAL_ARGUMENT, "EnvironmentalReverbTagNotSupported");
        }
    }

    specific->set<Parameter::Specific::environmentalReverb>(envReverbParam);
    return ndk::ScopedAStatus::ok();
}

RetCode OffloadBundleAidl::releaseContext() {
    if (mContext) {
        GlobalOffloadSession::getGlobalSession().releaseSession(mType, mContext->getSessionId());
        mContext.reset();
    }
    return RetCode::SUCCESS;
}

ndk::ScopedAStatus OffloadBundleAidl::commandImpl(
        aidl::android::hardware::audio::effect::CommandId command) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    switch (command) {
        case aidl::android::hardware::audio::effect::CommandId::START:
            mContext->enable();
            break;
        case aidl::android::hardware::audio::effect::CommandId::STOP:
            mContext->disable();
            break;
        case aidl::android::hardware::audio::effect::CommandId::RESET:
            mContext->disable();
            mContext->resetBuffer();
            break;
        default:
            LOG(ERROR) << __func__ << " commandId " << toString(command) << " not supported";
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commandIdNotSupported");
    }
    return ndk::ScopedAStatus::ok();
}

} // namespace aidl::android::hardware::audio::effect
