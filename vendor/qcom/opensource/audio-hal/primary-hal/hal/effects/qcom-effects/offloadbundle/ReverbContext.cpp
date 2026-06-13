/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Effect_ReverbQti"
#include <Utils.h>
#include <cstddef>

#include "OffloadBundleContext.h"
#include "OffloadBundleTypes.h"

namespace aidl::qti::effects {

using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;

bool ReverbContext::isPreset() {
    return (mType == OffloadBundleEffectType::AUX_PRESET_REVERB ||
            mType == OffloadBundleEffectType::INSERT_PRESET_REVERB);
}

ReverbContext::ReverbContext(const Parameter::Common& common, const OffloadBundleEffectType& type,
                             bool processData)
    : OffloadBundleContext(common, type, processData) {
    LOG(DEBUG) << __func__ << type << " ioHandle " << common.ioHandle;
    if (isPreset()) {
        mPreset = PresetReverb::Presets::NONE;
        mNextPreset = PresetReverb::Presets::NONE;
    }
    mState = EffectState::INITIALIZED;
}

ReverbContext::~ReverbContext() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    deInit();
}

void ReverbContext::deInit() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    stop();
}

RetCode ReverbContext::enable() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::ACTIVE;
    if (isPreset() && mNextPreset == PresetReverb::Presets::NONE) {
        return RetCode::SUCCESS;
    }
    mReverbParams.enable = 1;
    return RetCode::SUCCESS;
}

RetCode ReverbContext::disable() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (!isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::INITIALIZED;
    mReverbParams.enable = 0;
    setOffloadParameters(&mReverbParams, REVERB_ENABLE_FLAG);
    return RetCode::SUCCESS;
}

RetCode ReverbContext::start(pal_stream_handle_t* palHandle) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();

    mPalHandle = palHandle;
    if (isEffectActive() && isPreset()) {
        setOffloadParameters(&mReverbParams, REVERB_ENABLE_FLAG | REVERB_PRESET);
    } else {
        LOG(DEBUG) << __func__ << mType << " inactive or non preset";
    }

    return RetCode::SUCCESS;
}

RetCode ReverbContext::stop() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    struct ReverbParams reverbParam;
    setOffloadParameters(&reverbParam, REVERB_ENABLE_FLAG);
    mPalHandle = nullptr;
    return RetCode::SUCCESS;
}

RetCode ReverbContext::setOutputDevice(
        const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& device) {
    mOutputDevice = device;
    return RetCode::SUCCESS;
}

RetCode ReverbContext::setPresetReverbPreset(const PresetReverb::Presets& preset) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << " preset " << toString(preset);
    mNextPreset = preset;
    mReverbParams.preset = static_cast<int32_t>(preset);
    if (preset != PresetReverb::Presets::NONE) {
        mReverbParams.enable = 1;
        setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_PRESET);
    }
    return RetCode::SUCCESS;
}

RetCode ReverbContext::setEnvironmentalReverbRoomLevel(int roomLevel) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << roomLevel;
    mReverbParams.roomLevel = roomLevel;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_ROOM_LEVEL);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbRoomLevel() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.roomLevel;
    return mReverbParams.roomLevel;
}

RetCode ReverbContext::setEnvironmentalReverbRoomHfLevel(int roomHfLevel) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << roomHfLevel;
    mReverbParams.roomHfLevel = roomHfLevel;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_ROOM_HF_LEVEL);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbRoomHfLevel() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.roomHfLevel;
    return mReverbParams.roomHfLevel;
}

RetCode ReverbContext::setEnvironmentalReverbDecayTime(int decayTime) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << decayTime;
    mReverbParams.decayTime = decayTime;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_DECAY_TIME);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbDecayTime() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.decayTime;
    return mReverbParams.decayTime;
}

RetCode ReverbContext::setEnvironmentalReverbDecayHfRatio(int decayHfRatio) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << decayHfRatio;
    mReverbParams.decayHfRatio = decayHfRatio;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_DECAY_HF_RATIO);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbDecayHfRatio() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.decayHfRatio;
    return mReverbParams.decayHfRatio;
}

RetCode ReverbContext::setEnvironmentalReverbLevel(int level) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << level;
    mReverbParams.level = level;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_LEVEL);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbLevel() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.level;
    return mReverbParams.level;
}

RetCode ReverbContext::setEnvironmentalReverbDelay(int delay) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << delay;
    mReverbParams.delay = delay;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_DELAY);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbDelay() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.delay;
    return mReverbParams.delay;
}

RetCode ReverbContext::setEnvironmentalReverbDiffusion(int diffusion) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << diffusion;
    mReverbParams.diffusion = diffusion;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_DIFFUSION);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbDiffusion() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.diffusion;
    return mReverbParams.diffusion;
}

RetCode ReverbContext::setEnvironmentalReverbDensity(int density) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << density;
    mReverbParams.density = density;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_DENSITY);
    return RetCode::SUCCESS;
}

int ReverbContext::getEnvironmentalReverbDensity() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.density;
    return mReverbParams.density;
}

RetCode ReverbContext::setEnvironmentalReverbBypass(bool bypass) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << bypass;
    mReverbParams.bypass = bypass;
    return RetCode::SUCCESS;
}

bool ReverbContext::getEnvironmentalReverbBypass() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << mReverbParams.bypass;
    return mReverbParams.bypass;
}

RetCode ReverbContext::setReflectionsLevel(int level) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << level;
    mReverbParams.reflectionsLevel = level;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_DENSITY);
    return RetCode::SUCCESS;
}

bool ReverbContext::getReflectionsLevel() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  "
               << mReverbParams.reflectionsLevel;
    return mReverbParams.reflectionsLevel;
}

RetCode ReverbContext::setReflectionsDelay(int delay) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  " << delay;
    mReverbParams.reflectionsDelay = delay;
    setOffloadParameters(REVERB_ENABLE_FLAG | REVERB_REFLECTIONS_DELAY);
    return RetCode::SUCCESS;
}

bool ReverbContext::getReflectionsDelay() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << "  "
               << mReverbParams.reflectionsDelay;
    return mReverbParams.reflectionsDelay;
}

int ReverbContext::setOffloadParameters(ReverbParams* reverbParams, uint64_t flags) {
    if (mPalHandle) {
        ParamDelegator::updatePalParameters(mPalHandle, reverbParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

int ReverbContext::setOffloadParameters(uint64_t flags) {
    if (mPalHandle) {
        ParamDelegator::updatePalParameters(mPalHandle, &mReverbParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

} // namespace aidl::qti::effects
