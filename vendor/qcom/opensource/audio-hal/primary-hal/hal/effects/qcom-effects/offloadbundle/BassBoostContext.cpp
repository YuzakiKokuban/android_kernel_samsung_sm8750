/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Effect_BassBoostQti"

#include <Utils.h>
#include <cstddef>

#include "OffloadBundleContext.h"
#include "OffloadBundleTypes.h"

namespace aidl::qti::effects {

using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;

BassBoostContext::BassBoostContext(const Parameter::Common& common,
                                   const OffloadBundleEffectType& type, bool processData)
    : OffloadBundleContext(common, type, processData) {
    LOG(DEBUG) << __func__ << type << " ioHandle " << common.ioHandle;
    mState = EffectState::INITIALIZED;
}

BassBoostContext::~BassBoostContext() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    deInit();
}

void BassBoostContext::deInit() {
    LOG(DEBUG) << __func__ << " ioHandle" << getIoHandle();
    stop();
}

RetCode BassBoostContext::enable() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::ACTIVE;
    mBassParams.mEnabled = 1;
    setOffloadParameters(BASSBOOST_ENABLE_FLAG);
    return RetCode::SUCCESS;
}

RetCode BassBoostContext::disable() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (!isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::INITIALIZED;
    mBassParams.mEnabled = 0;
    setOffloadParameters(BASSBOOST_ENABLE_FLAG);
    return RetCode::SUCCESS;
}

RetCode BassBoostContext::start(pal_stream_handle_t* palHandle) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    mPalHandle = palHandle;
    if (isEffectActive()) {
        setOffloadParameters(BASSBOOST_ENABLE_FLAG | BASSBOOST_STRENGTH);
    } else {
        LOG(DEBUG) << "Not yet enabled";
    }
    return RetCode::SUCCESS;
}

RetCode BassBoostContext::stop() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    struct BassBoostParams bassParams; // by default enable is 0
    setOffloadParameters(&bassParams, BASSBOOST_ENABLE_FLAG);
    mPalHandle = nullptr;
    return RetCode::SUCCESS;
}

bool BassBoostContext::deviceSupportsEffect(const std::vector<AudioDeviceDescription>& devices) {
    for (const auto& device : devices) {
        if (device != AudioDeviceDescription{AudioDeviceType::OUT_HEADSET,
                                             AudioDeviceDescription::CONNECTION_ANALOG} &&
            device != AudioDeviceDescription{AudioDeviceType::OUT_HEADPHONE,
                                             AudioDeviceDescription::CONNECTION_ANALOG} &&
            device != AudioDeviceDescription{AudioDeviceType::OUT_HEADPHONE,
                                             AudioDeviceDescription::CONNECTION_BT_A2DP} &&
            device != AudioDeviceDescription{AudioDeviceType::OUT_HEADSET,
                                             AudioDeviceDescription::CONNECTION_USB}) {
            return false;
        }
    }
    return true;
}

RetCode BassBoostContext::setOutputDevice(
        const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& device) {
    std::lock_guard lg(mMutex);
    mOutputDevice = device;
    if (deviceSupportsEffect(mOutputDevice)) {
        if (mTempDisabled) {
            if (isEffectActive()) {
                mBassParams.mEnabled = 1;
                setOffloadParameters(BASSBOOST_ENABLE_FLAG);
            }
        }
        mTempDisabled = false;
    } else if (!mTempDisabled) {
        if (isEffectActive()) {
            mBassParams.mEnabled = 0;
            setOffloadParameters(BASSBOOST_ENABLE_FLAG);
        }
        mTempDisabled = true;
    }

    return RetCode::SUCCESS;
}

RetCode BassBoostContext::setBassBoostStrength(int strength) {
    LOG(DEBUG) << __func__ << " strength " << strength;
    mBassParams.mStrength = strength;
    setOffloadParameters(BASSBOOST_ENABLE_FLAG | BASSBOOST_STRENGTH);
    return RetCode::SUCCESS;
}

int BassBoostContext::getBassBoostStrength() {
    LOG(DEBUG) << __func__ << " strength " << mBassParams.mStrength;
    return mBassParams.mStrength;
}

int BassBoostContext::setOffloadParameters(uint64_t flags) {
    if (mPalHandle) {
        LOG(DEBUG) << " Strength " << mBassParams.mStrength << " enabled " << mBassParams.mEnabled;
        ParamDelegator::updatePalParameters(mPalHandle, &mBassParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

int BassBoostContext::setOffloadParameters(BassBoostParams* bassParams, uint64_t flags) {
    if (mPalHandle) {
        LOG(DEBUG) << " Strength " << bassParams->mStrength << " enabled " << bassParams->mEnabled;
        ParamDelegator::updatePalParameters(mPalHandle, bassParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

} // namespace aidl::qti::effects
