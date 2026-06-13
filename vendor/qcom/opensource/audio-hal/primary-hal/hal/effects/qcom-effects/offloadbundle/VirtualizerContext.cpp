/*

 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AHAL_Effect_VirtualizerQti"

#include <Utils.h>
#include <cstddef>

#include "OffloadBundleContext.h"
#include "OffloadBundleTypes.h"

namespace aidl::qti::effects {

using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using android::media::audio::common::AudioChannelLayout;

VirtualizerContext::VirtualizerContext(const Parameter::Common& common,
                                       const OffloadBundleEffectType& type, bool processData)
    : OffloadBundleContext(common, type, processData) {
    LOG(DEBUG) << __func__ << type << " ioHandle " << common.ioHandle;
    mState = EffectState::INITIALIZED;
}

VirtualizerContext::~VirtualizerContext() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    deInit();
}

void VirtualizerContext::deInit() {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    stop();
}

RetCode VirtualizerContext::enable() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;
    mState = EffectState::ACTIVE;
    mVirtParams.enable = 1;
    setOffloadParameters(VIRTUALIZER_ENABLE_FLAG | VIRTUALIZER_STRENGTH);
    return RetCode::SUCCESS;
}

RetCode VirtualizerContext::disable() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    if (!isEffectActive()) return RetCode::ERROR_ILLEGAL_PARAMETER;

    mState = EffectState::INITIALIZED;
    mVirtParams.enable = 0;
    setOffloadParameters(VIRTUALIZER_ENABLE_FLAG);
    return RetCode::SUCCESS;
}

RetCode VirtualizerContext::start(pal_stream_handle_t* palHandle) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    mPalHandle = palHandle;
    if (isEffectActive()) {
        setOffloadParameters(VIRTUALIZER_ENABLE_FLAG | VIRTUALIZER_STRENGTH);
    } else {
        LOG(DEBUG) << __func__ << "Not yet enabled "
                   << " ioHandle " << getIoHandle();
    }

    return RetCode::SUCCESS;
}

RetCode VirtualizerContext::stop() {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();

    struct VirtualizerParams virtParams; // by default enable is 0.
    setOffloadParameters(&virtParams, VIRTUALIZER_ENABLE_FLAG);
    mPalHandle = nullptr;
    return RetCode::SUCCESS;
}

RetCode VirtualizerContext::setOutputDevice(
        const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& device) {
    std::lock_guard lg(mMutex);
    mOutputDevice = device;
    if (deviceSupportsEffect(mOutputDevice)) {
        if (mTempDisabled) {
            if (isEffectActive()) {
                mVirtParams.enable = 1;
                setOffloadParameters(VIRTUALIZER_ENABLE_FLAG);
            }
        }
        mTempDisabled = false;
    } else if (!mTempDisabled) {
        if (isEffectActive()) {
            mVirtParams.enable = 0;
            setOffloadParameters(VIRTUALIZER_ENABLE_FLAG);
        }
        mTempDisabled = true;
    }
    return RetCode::SUCCESS;
}

RetCode VirtualizerContext::setVirtualizerStrength(int strength) {
    std::lock_guard lg(mMutex);
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << " strength " << strength;
    mVirtParams.strength = strength;
    setOffloadParameters(VIRTUALIZER_ENABLE_FLAG | VIRTUALIZER_STRENGTH);
    return RetCode::SUCCESS;
}

int VirtualizerContext::getVirtualizerStrength() const {
    LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle() << " strength " << mVirtParams.strength;
    return mVirtParams.strength;
}

RetCode VirtualizerContext::setForcedDevice(const AudioDeviceDescription& device) {
    std::lock_guard lg(mMutex);
    AudioDeviceDescription noneDevice;
    if (device != noneDevice) {
        RETURN_VALUE_IF(true != deviceSupportsEffect({device}), RetCode::ERROR_EFFECT_LIB_ERROR,
                        " deviceNotSupportVirtualizer");
    }
    mForcedDevice = device;
    return RetCode::SUCCESS;
}

std::vector<Virtualizer::ChannelAngle> VirtualizerContext::getSpeakerAngles(
        const Virtualizer::SpeakerAnglesPayload payload) {
    std::vector<Virtualizer::ChannelAngle> angles;
    auto channels = ::aidl::android::hardware::audio::common::getChannelCount(payload.layout);
    RETURN_VALUE_IF(!isConfigSupported(channels, payload.device), angles, "unsupportedConfig");

    if (channels == 1) {
        angles = {{.channel = (int32_t)AudioChannelLayout::CHANNEL_FRONT_LEFT,
                   .azimuthDegree = 0,
                   .elevationDegree = 0}};
    } else {
        angles = {{.channel = (int32_t)AudioChannelLayout::CHANNEL_FRONT_LEFT,
                   .azimuthDegree = -90,
                   .elevationDegree = 0},
                  {.channel = (int32_t)AudioChannelLayout::CHANNEL_FRONT_RIGHT,
                   .azimuthDegree = 90,
                   .elevationDegree = 0}};
    }
    return angles;
}

int VirtualizerContext::setOffloadParameters(uint64_t flags) {
    if (mPalHandle) {
        ParamDelegator::updatePalParameters(mPalHandle, &mVirtParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

int VirtualizerContext::setOffloadParameters(VirtualizerParams* virtParams, uint64_t flags) {
    if (mPalHandle) {
        ParamDelegator::updatePalParameters(mPalHandle, virtParams, flags);
    } else {
        LOG(VERBOSE) << " PalHandle not set";
    }
    return 0;
}

bool VirtualizerContext::isConfigSupported(size_t channelCount,
                                           const AudioDeviceDescription& device) {
    return ((channelCount == 1 || channelCount == 2) && deviceSupportsEffect({device}));
}

bool VirtualizerContext::deviceSupportsEffect(const std::vector<AudioDeviceDescription>& devices) {
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

} // namespace aidl::qti::effects
