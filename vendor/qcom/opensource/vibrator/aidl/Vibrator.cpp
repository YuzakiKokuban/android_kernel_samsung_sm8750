/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "vendor.qti.vibrator"

#include <inttypes.h>
#include <log/log.h>
#include <string.h>
#include <unistd.h>

#include "Vibrator.h"
#include "VibratorOL/Vibrator.h"
#include "VibratorCL/Vibrator.h"
#include "VibratorSelector/VibratorSelector.h"

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

class Vibrator::VibratorPrivate {
private:
    VibratorOL mVibratorOL;
    VibratorCL mVibratorCL;
    IVibrator* mSelectedVibrator;
    std::mutex VibratorSelectionLock;
    bool mSupportCL;
    std::shared_ptr<VibratorSelector> mVibSelector;
public:
    VibratorPrivate() {
        mSupportCL = mVibratorOL.mSupportVISense;
        mVibSelector = nullptr;
        mSelectedVibrator = &mVibratorOL;
        int32_t ret;

        if (mSupportCL) {
            ret = VibratorSelector::init();
            if (!ret) {
                mVibSelector = VibratorSelector::GetInstance();
            }
        }
    }

    ndk::ScopedAStatus getCapabilities(int32_t* _aidl_return) {
        int32_t capabilities;

        /* Merge the capabilities of both mVibratorOL and mVibratorCL */
        mVibratorOL.getCapabilities(_aidl_return);
        if (mSupportCL) {
            mVibratorCL.getCapabilities(&capabilities);
            *_aidl_return |= capabilities;
        }

        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>& callback) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForOnApi(timeoutMs) == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->on(timeoutMs, callback);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus off() {
        /* The selected vibrator should be always used to turn off vibration */
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();
        status = mSelectedVibrator->off();
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus perform(Effect effect, EffectStrength es, const std::shared_ptr<IVibratorCallback>& callback, int32_t* _aidl_return) {

        int effect_id = static_cast<int> (effect);
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForPerformApi(effect_id) == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->perform(effect, es, callback, _aidl_return);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getSupportedEffects(std::vector<Effect>* _aidl_return) {
        std::vector<Effect> effectsCL;

        /* Merge the effects being supported by both mVibratorOL and mVibratorCL */
        mVibratorOL.getSupportedEffects(_aidl_return);
        if (mSupportCL) {
            mVibratorCL.getSupportedEffects(&effectsCL);
            for (uint32_t i = 0; i < effectsCL.size() ; i++) {
                if (std::find(_aidl_return->begin(), _aidl_return->end(), effectsCL[i]) == _aidl_return->end())
                    _aidl_return->insert(_aidl_return->end(), effectsCL[i]);
            }
        }

        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus setAmplitude(float amplitude) {
        /* Set amplitude should be only called after On() vibration is enabled so use existing mSelectedVibrator */
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();
        status = mSelectedVibrator->setAmplitude(amplitude);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus setExternalControl(bool enabled) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        if (mSupportCL)
            mVibratorCL.setExternalControl(enabled);

        status = mVibratorOL.setExternalControl(enabled);

        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getCompositionDelayMax(int32_t* maxDelayMs) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->getCompositionDelayMax(maxDelayMs);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getCompositionSizeMax(int32_t* maxSize) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->getCompositionSizeMax(maxSize);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->getSupportedPrimitives(supported);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus getPrimitiveDuration(CompositePrimitive primitive, int32_t* durationMs) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->getPrimitiveDuration(primitive, durationMs);
        VibratorSelectionLock.unlock();

        return status;
    }

    ndk::ScopedAStatus compose(const std::vector<CompositeEffect>& composite,
                    const std::shared_ptr<IVibratorCallback>& callback) {
        ndk::ScopedAStatus status;

        VibratorSelectionLock.lock();

        mSelectedVibrator = &mVibratorOL;
        if (mVibSelector && mVibSelector->getVibForComposeApi() == VIB_TYPE_CL)
            mSelectedVibrator = &mVibratorCL;

        status = mSelectedVibrator->compose(composite, callback);
        VibratorSelectionLock.unlock();
        return status;
    }
};


Vibrator::Vibrator() {
    pImpl = new Vibrator::VibratorPrivate;
}

Vibrator::~Vibrator() {
    if (NULL != pImpl) {
        delete pImpl;
        pImpl = NULL;
    }
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    return pImpl->getCapabilities(_aidl_return);
}

ndk::ScopedAStatus Vibrator::off() {
    return pImpl->off();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
    const std::shared_ptr<IVibratorCallback>& callback) {
    return pImpl->on(timeoutMs, callback);
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength es, const std::shared_ptr<IVibratorCallback>& callback, int32_t* _aidl_return) {
    return pImpl->perform(effect, es, callback, _aidl_return);
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    return pImpl->getSupportedEffects(_aidl_return);
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    return pImpl->setAmplitude(amplitude);
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    return pImpl->setExternalControl(enabled);
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    return pImpl->getCompositionDelayMax(maxDelayMs);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    return pImpl->getCompositionSizeMax(maxSize);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
    return pImpl->getSupportedPrimitives(supported);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
    int32_t* durationMs) {
    return pImpl->getPrimitiveDuration(primitive, durationMs);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
    const std::shared_ptr<IVibratorCallback>& callback) {
    return pImpl->compose(composite, callback);
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id __unused, Effect effect __unused,
    EffectStrength strength __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* resonantFreqHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getQFactor(float* qFactor __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* freqResolutionHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* freqMinimumHz __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* _aidl_return __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* durationMs __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* maxSize __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* supported __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& composite __unused,
    const std::shared_ptr<IVibratorCallback>& callback __unused) {
    return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl