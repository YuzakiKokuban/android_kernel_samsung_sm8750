/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Effect_EqualizerQti"

#include <Utils.h>
#include <cstddef>
#include "OffloadBundleContext.h"
#include "OffloadBundleTypes.h"

namespace aidl::qti::effects {

using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;

EqualizerContext::EqualizerContext(const Parameter::Common& common,
                                   const OffloadBundleEffectType& type, bool processData)
    : OffloadBundleContext(common, type, processData) {
    LOG(DEBUG) << __func__ << type << " ioHandle " << common.ioHandle;
    init(); // init default state
    mState = EffectState::INITIALIZED;
}

EqualizerContext::~EqualizerContext() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    deInit();
}

void EqualizerContext::init() {
    // init with pre-defined preset NORMAL
    for (std::size_t i = 0; i < MAX_NUM_BANDS; i++) {
        mBandLevels[i] = kBandPresetLevels[0 /* normal */][i];
    }
    memset(&mEqParams, 0, sizeof(struct EqualizerParams));
    mEqParams.config.presetId = PRESET_INVALID;
    mEqParams.config.pregain = Q27_UNITY;
    mEqParams.config.numBands = MAX_NUM_BANDS;
}

void EqualizerContext::deInit() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    stop();
}

RetCode EqualizerContext::enable() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    std::lock_guard lg(mMutex);
    if (isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::ACTIVE;
    mEqParams.enable = 1;
    setOffloadParameters(EQ_ENABLE_FLAG | EQ_BANDS_LEVEL);
    return RetCode::SUCCESS;
}

RetCode EqualizerContext::disable() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    std::lock_guard lg(mMutex);
    if (!isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::INITIALIZED;
    mEqParams.enable = 0;
    setOffloadParameters(EQ_ENABLE_FLAG);
    return RetCode::SUCCESS;
}

RetCode EqualizerContext::start(pal_stream_handle_t* palHandle) {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    std::lock_guard lg(mMutex);
    mPalHandle = palHandle;
    if (isEffectActive()) {
        setOffloadParameters(EQ_ENABLE_FLAG | EQ_BANDS_LEVEL);
    } else {
        LOG(DEBUG) << "Not yet enabled";
    }

    return RetCode::SUCCESS;
}

RetCode EqualizerContext::stop() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    struct EqualizerParams eqParam = {0}; // by default enable bit is 0
    setOffloadParameters(&eqParam, EQ_ENABLE_FLAG);
    mPalHandle = nullptr;
    return RetCode::SUCCESS;
}

RetCode EqualizerContext::setEqualizerPreset(const std::size_t presetIdx) {
    std::lock_guard lg(mMutex);
    if (presetIdx < 0 || presetIdx >= MAX_NUM_PRESETS) {
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }

    // Translation from existing implementation, first we update then send config to PAL.
    // ideally, send it to PAL and check if operation is successful then only update
    mCurrentPreset = presetIdx;
    for (std::size_t i = 0; i < MAX_NUM_BANDS; i++) {
        mBandLevels[i] = kBandPresetLevels[presetIdx][i];
    }

    updateOffloadParameters();

    setOffloadParameters(EQ_ENABLE_FLAG | EQ_PRESET);

    return RetCode::SUCCESS;
}

bool EqualizerContext::isBandLevelIndexInRange(
        const std::vector<Equalizer::BandLevel>& bandLevels) const {
    const auto[min, max] =
            std::minmax_element(bandLevels.begin(), bandLevels.end(),
                                [](const auto& a, const auto& b) { return a.index < b.index; });
    return min->index >= 0 && max->index < MAX_NUM_BANDS;
}

RetCode EqualizerContext::setEqualizerBandLevels(
        const std::vector<Equalizer::BandLevel>& bandLevels) {
    std::lock_guard lg(mMutex);
    RETURN_VALUE_IF(bandLevels.size() > MAX_NUM_BANDS, RetCode::ERROR_ILLEGAL_PARAMETER,
                    "Exceeds Max Size");

    RETURN_VALUE_IF(bandLevels.empty(), RetCode::ERROR_ILLEGAL_PARAMETER, "Empty Bands");

    RETURN_VALUE_IF(!isBandLevelIndexInRange(bandLevels), RetCode::ERROR_ILLEGAL_PARAMETER,
                    "indexOutOfRange");

    // Translation from existing implementation, first we update then send config to PAL.
    // ideally, send it to PAL and check if operation is successful then only update
    for (auto& bandLevel : bandLevels) {
        int level = bandLevel.levelMb;
        if (level > 0) {
            level = (int)((level + 50) / 100);
        } else {
            level = (int)((level - 50) / 100);
        }
        LOG(VERBOSE) << __func__ << " level " << bandLevel.index << " level" << bandLevel.levelMb
                     << " refined level" << level;
        mBandLevels[bandLevel.index] = level;
        mCurrentPreset = PRESET_CUSTOM;
    }

    updateOffloadParameters();
    setOffloadParameters(EQ_ENABLE_FLAG | EQ_BANDS_LEVEL);

    return RetCode::SUCCESS;
}

std::vector<Equalizer::BandLevel> EqualizerContext::getEqualizerBandLevels() const {
    std::vector<Equalizer::BandLevel> bandLevels;
    bandLevels.reserve(MAX_NUM_BANDS);
    for (std::size_t i = 0; i < MAX_NUM_BANDS; i++) {
        bandLevels.emplace_back(
                Equalizer::BandLevel{static_cast<int32_t>(i), mBandLevels[i] * 100});
    }
    return bandLevels;
}

std::vector<int32_t> EqualizerContext::getEqualizerCenterFreqs() {
    std::vector<int32_t> result;

    std::for_each(kBandFrequencies.begin(), kBandFrequencies.end(),
                  [&](const auto& band) { result.emplace_back((band.minMh + band.maxMh) / 2); });
    return result;
}

void EqualizerContext::updateOffloadParameters() {
    for (int i = 0; i < MAX_NUM_BANDS; i++) {
        mEqParams.config.presetId = mCurrentPreset;
        mEqParams.bandConfig[i].bandIndex = i;
        mEqParams.bandConfig[i].filterType = EQ_BAND_BOOST;
        mEqParams.bandConfig[i].frequencyMhz = kPresetsFrequencies[i] * 1000;
        mEqParams.bandConfig[i].gainMb = mBandLevels[i] * 100;
        mEqParams.bandConfig[i].qFactor = Q8_UNITY;
    }
}

int EqualizerContext::setOffloadParameters(uint64_t flags) {
    if (mPalHandle) {
        ParamDelegator::updatePalParameters(mPalHandle, &mEqParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

int EqualizerContext::setOffloadParameters(EqualizerParams* params, uint64_t flags) {
    if (mPalHandle) {
        ParamDelegator::updatePalParameters(mPalHandle, params, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

} // namespace aidl::qti::effects
