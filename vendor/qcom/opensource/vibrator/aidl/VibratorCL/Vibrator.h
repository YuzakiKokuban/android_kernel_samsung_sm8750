/*
 * Copyright (c) 2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.

 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <aidl/android/hardware/vibrator/BnVibrator.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

struct haptics_effect_config_t {
    size_t size;
    uint32_t duration;
    uint8_t *data;
};

class VibratorCL : public BnVibrator {
public:
    VibratorCL();
    ~VibratorCL();
    bool mSupportGain;
    bool mSupportEffects;
    bool mSupportExternalControl;
    bool mInExternalControl;
    static std::mutex HapticsMutex;
    static std::mutex EventMutex;
    static std::condition_variable cv;
    static std::condition_variable Eventcv;
    static std::thread OffThread;
    static std::atomic<bool> CalThrdCreated;
    static std::vector<haptics_effect_config_t> PcmEffectInfo;
    static bool OffThrdCreated;
    static bool ActiveUsecase;
    enum haptics_mode_t {
        HAPTICS_HOSTLESS,
        HAPTICS_STREAMING,
    } haptics_mode_t;
    static bool inComposition;
    int on(int32_t timeoutMs);
    void offEffect();
    void HapticsWait();
    void HapticsCalibThread();
    void HapticsWaitTillWaveformComp();
    void HapticsPCMRead();
    bool IsPCMSupported(int effectID);
    int32_t StopHapticsStream();
    int32_t offCurrentEffect();
    static int32_t StreamHapticsCallback(uint64_t *stream_handle,
                         uint32_t event_id, uint32_t *event_data,
                         uint32_t event_size, uint64_t cookie);
    ndk::ScopedAStatus getCapabilities(int32_t* _aidl_return) override;
    ndk::ScopedAStatus off() override;
    ndk::ScopedAStatus on(int32_t timeoutMs,
            const std::shared_ptr<IVibratorCallback>& callback) override;
    ndk::ScopedAStatus perform(Effect effect, EffectStrength strength,
            const std::shared_ptr<IVibratorCallback>& callback,
            int32_t* _aidl_return) override;
    ndk::ScopedAStatus getSupportedEffects(std::vector<Effect>* _aidl_return) override;
    ndk::ScopedAStatus setAmplitude(float amplitude) override;
    ndk::ScopedAStatus setExternalControl(bool enabled) override;
    ndk::ScopedAStatus getCompositionDelayMax(int32_t* maxDelayMs);
    ndk::ScopedAStatus getCompositionSizeMax(int32_t* maxSize);
    ndk::ScopedAStatus getSupportedPrimitives(std::vector<CompositePrimitive>* supported) override;
    ndk::ScopedAStatus getPrimitiveDuration(CompositePrimitive primitive,
                                            int32_t* durationMs) override;
    ndk::ScopedAStatus compose(const std::vector<CompositeEffect>& composite,
                               const std::shared_ptr<IVibratorCallback>& callback) override;
    ndk::ScopedAStatus getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return) override;
    ndk::ScopedAStatus alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) override;
    ndk::ScopedAStatus alwaysOnDisable(int32_t id) override;
    ndk::ScopedAStatus getResonantFrequency(float *resonantFreqHz) override;
    ndk::ScopedAStatus getQFactor(float *qFactor) override;
    ndk::ScopedAStatus getFrequencyResolution(float *freqResolutionHz) override;
    ndk::ScopedAStatus getFrequencyMinimum(float *freqMinimumHz) override;
    ndk::ScopedAStatus getBandwidthAmplitudeMap(std::vector<float> *_aidl_return) override;
    ndk::ScopedAStatus getPwlePrimitiveDurationMax(int32_t *durationMs) override;
    ndk::ScopedAStatus getPwleCompositionSizeMax(int32_t *maxSize) override;
    ndk::ScopedAStatus getSupportedBraking(std::vector<Braking>* supported) override;
    ndk::ScopedAStatus composePwle(const std::vector<PrimitivePwle> &composite,
                               const std::shared_ptr<IVibratorCallback> &callback) override;
private:
    int play(int effectId, int strength, long* playLengthMs, uint32_t timeoutMs, bool isCompose, float amplitude);
    void composePlayThread(const std::vector<CompositeEffect>& composite,
        const std::shared_ptr<IVibratorCallback>& callback);
};

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
