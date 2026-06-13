/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <array>
#include <cstddef>

#include "OffloadBundleTypes.h"
#include "PalDefs.h"
#include "ParamDelegator.h"
#include "effect-impl/EffectContext.h"

using aidl::android::media::audio::common::AudioDeviceDescription;

enum class EffectState {
    UNINITIALIZED,
    INITIALIZED,
    ACTIVE,
};

namespace aidl::qti::effects {

class OffloadBundleContext : public EffectContext {
  public:
    OffloadBundleContext(const Parameter::Common& common, const OffloadBundleEffectType& type,
                         bool processData)
        : EffectContext(common, processData), mType(type) {
        LOG(DEBUG) << __func__ << type << " ioHandle " << getIoHandle();
    }

    virtual ~OffloadBundleContext() override {
        LOG(DEBUG) << __func__ << " ioHandle " << getIoHandle();
    }

    // Generic APIS
    OffloadBundleEffectType getBundleType() const { return mType; }
    // Each effect context needs to implement these methods
    virtual void deInit() = 0;
    virtual RetCode enable() = 0;
    virtual RetCode disable() = 0;
    virtual RetCode start(pal_stream_handle_t* palHandle) = 0;
    virtual RetCode stop() = 0;

    virtual int setOffloadParameters(uint64_t flags) { return 0; }
    // Equalizer methods, implement in EqualizerContext
    virtual RetCode setEqualizerPreset(const std::size_t presetIdx) {
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }

    virtual RetCode setEqualizerBandLevels(const std::vector<Equalizer::BandLevel>& bandLevels) {
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }

    virtual std::vector<Equalizer::BandLevel> getEqualizerBandLevels() const { return {}; }

    virtual std::vector<int32_t> getEqualizerCenterFreqs() { return {}; }

    virtual int getEqualizerPreset() const { return 0; }
    // BassBoost methods, implement in BassBoostContext
    virtual RetCode setBassBoostStrength(int strength) { return RetCode::ERROR_ILLEGAL_PARAMETER; }

    virtual int getBassBoostStrength() { return 0; }
    // Virtualizer methods, implement in VirtualizerContext
    virtual RetCode setVirtualizerStrength(int strength) {
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    virtual int getVirtualizerStrength() const { return 0; }

    virtual RetCode setForcedDevice(const AudioDeviceDescription& device) {
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }

    virtual AudioDeviceDescription getForcedDevice() const { return {}; }

    virtual std::vector<Virtualizer::ChannelAngle> getSpeakerAngles(
            const Virtualizer::SpeakerAnglesPayload payload) {
        return {};
    }

    virtual bool deviceSupportsEffect(const std::vector<AudioDeviceDescription>& device) {
        return true;
    }
    // Reverb methods, implement ReverbContext

    virtual RetCode setPresetReverbPreset(const PresetReverb::Presets& preset) {
        return RetCode::SUCCESS;
    }
    virtual PresetReverb::Presets getPresetReverbPreset() const { return {}; }

    virtual RetCode setEnvironmentalReverbRoomLevel(int roomLevel) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbRoomLevel() const { return 0; }
    virtual RetCode setEnvironmentalReverbRoomHfLevel(int roomHfLevel) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbRoomHfLevel() const { return 0; }
    virtual RetCode setEnvironmentalReverbDecayTime(int decayTime) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbDecayTime() const { return 0; }
    virtual RetCode setEnvironmentalReverbDecayHfRatio(int decayHfRatio) {
        return RetCode::SUCCESS;
    }
    virtual int getEnvironmentalReverbDecayHfRatio() const { return 0; }
    virtual RetCode setReflectionsLevel(int level) { return RetCode::SUCCESS; }
    virtual bool getReflectionsLevel() const { return false; }
    virtual RetCode setReflectionsDelay(int delay) { return RetCode::SUCCESS; }
    virtual bool getReflectionsDelay() const { return false; }
    virtual RetCode setEnvironmentalReverbLevel(int level) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbLevel() const { return 0; }
    virtual RetCode setEnvironmentalReverbDelay(int delay) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbDelay() const { return 0; }
    virtual RetCode setEnvironmentalReverbDiffusion(int diffusion) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbDiffusion() const { return 0; }
    virtual RetCode setEnvironmentalReverbDensity(int density) { return RetCode::SUCCESS; }
    virtual int getEnvironmentalReverbDensity() const { return 0; }
    virtual RetCode setEnvironmentalReverbBypass(bool bypass) { return RetCode::SUCCESS; }
    virtual bool getEnvironmentalReverbBypass() const { return true; }

  protected:
    std::mutex mMutex;
    const OffloadBundleEffectType mType;
    pal_stream_handle_t* mPalHandle;
    EffectState mState = EffectState::UNINITIALIZED;
    bool isEffectActive() { return mState == EffectState::ACTIVE; }
};

class BassBoostContext final : public OffloadBundleContext {
  public:
    BassBoostContext(const Parameter::Common& common, const OffloadBundleEffectType& type,
                     bool processData);
    ~BassBoostContext() override;
    virtual void deInit() override;
    virtual RetCode enable() override;
    virtual RetCode disable() override;
    virtual RetCode start(pal_stream_handle_t* palHandle) override;
    virtual RetCode stop() override;
    RetCode setOutputDevice(const std::vector<AudioDeviceDescription>& device) override;
    RetCode setBassBoostStrength(int strength) override;
    int getBassBoostStrength() override;
    int setOffloadParameters(uint64_t flags) override;
    int setOffloadParameters(BassBoostParams* bassParam, uint64_t flags);
    bool deviceSupportsEffect(const std::vector<AudioDeviceDescription>& device) override;

  private:
    struct BassBoostParams mBassParams;
    bool mTempDisabled = false;
};

class EqualizerContext final : public OffloadBundleContext {
  public:
    EqualizerContext(const Parameter::Common& common, const OffloadBundleEffectType& type,
                     bool processData);
    ~EqualizerContext() override;
    void init();
    virtual void deInit() override;
    virtual RetCode enable() override;
    virtual RetCode disable() override;
    virtual RetCode start(pal_stream_handle_t* palHandle) override;
    virtual RetCode stop() override;

    RetCode setEqualizerPreset(const std::size_t presetIdx) override;
    RetCode setEqualizerBandLevels(const std::vector<Equalizer::BandLevel>& bandLevels) override;
    std::vector<Equalizer::BandLevel> getEqualizerBandLevels() const override;
    std::vector<int32_t> getEqualizerCenterFreqs() override;
    int getEqualizerPreset() const override { return mCurrentPreset; }
    int setOffloadParameters(uint64_t flags) override;
    int setOffloadParameters(EqualizerParams* params, uint64_t flags);
    void updateOffloadParameters();

  private:
    bool isBandLevelIndexInRange(const std::vector<Equalizer::BandLevel>& bandLevels) const;
    int mCurrentPreset = PRESET_CUSTOM; // current preset index;
    std::array<int, MAX_NUM_BANDS> mBandLevels;
    struct EqualizerParams mEqParams;
};

class VirtualizerContext final : public OffloadBundleContext {
  public:
    VirtualizerContext(const Parameter::Common& common, const OffloadBundleEffectType& type,
                       bool processData);
    ~VirtualizerContext() override;

    virtual void deInit() override;
    virtual RetCode enable() override;
    virtual RetCode disable() override;
    virtual RetCode start(pal_stream_handle_t* palHandle) override;
    virtual RetCode stop() override;
    RetCode setOutputDevice(
            const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& device)
            override;
    RetCode setVirtualizerStrength(int strength) override;
    int getVirtualizerStrength() const override;

    virtual RetCode setForcedDevice(const AudioDeviceDescription& device) override;

    virtual AudioDeviceDescription getForcedDevice() const override { return mForcedDevice; }

    std::vector<Virtualizer::ChannelAngle> getSpeakerAngles(
            const Virtualizer::SpeakerAnglesPayload payload) override;

    int setOffloadParameters(uint64_t flags) override;
    int setOffloadParameters(VirtualizerParams* virtParams, uint64_t flags);
    bool deviceSupportsEffect(const std::vector<AudioDeviceDescription>& device) override;

  private:
    bool isConfigSupported(size_t channelCount, const AudioDeviceDescription& device);

    struct VirtualizerParams mVirtParams;
    AudioDeviceDescription mForcedDevice;
    bool mTempDisabled = false;
};

class ReverbContext final : public OffloadBundleContext {
  public:
    ReverbContext(const Parameter::Common& common, const OffloadBundleEffectType& type,
                  bool processData);
    ~ReverbContext() override;

    virtual void deInit() override;
    virtual RetCode enable() override;
    virtual RetCode disable() override;
    virtual RetCode start(pal_stream_handle_t* palHandle) override;
    virtual RetCode stop() override;
    RetCode setOutputDevice(
            const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>& device)
            override;

    int setOffloadParameters(uint64_t flags) override;
    int setOffloadParameters(ReverbParams* reverbParams, uint64_t flags);

    virtual RetCode setPresetReverbPreset(const PresetReverb::Presets& preset) override;

    virtual PresetReverb::Presets getPresetReverbPreset() const override { return mNextPreset; }

    virtual RetCode setEnvironmentalReverbRoomLevel(int roomLevel) override;
    virtual int getEnvironmentalReverbRoomLevel() const override;
    virtual RetCode setEnvironmentalReverbRoomHfLevel(int roomHfLevel) override;
    virtual int getEnvironmentalReverbRoomHfLevel() const override;
    virtual RetCode setEnvironmentalReverbDecayTime(int decayTime) override;
    virtual int getEnvironmentalReverbDecayTime() const override;
    virtual RetCode setEnvironmentalReverbDecayHfRatio(int decayHfRatio) override;
    virtual int getEnvironmentalReverbDecayHfRatio() const override;
    virtual RetCode setReflectionsLevel(int level) override;
    virtual bool getReflectionsLevel() const override;
    virtual RetCode setReflectionsDelay(int delay);
    virtual bool getReflectionsDelay() const;
    virtual RetCode setEnvironmentalReverbLevel(int level) override;
    virtual int getEnvironmentalReverbLevel() const override;
    virtual RetCode setEnvironmentalReverbDelay(int delay) override;
    virtual int getEnvironmentalReverbDelay() const override;
    virtual RetCode setEnvironmentalReverbDiffusion(int diffusion) override;
    virtual int getEnvironmentalReverbDiffusion() const override;
    virtual RetCode setEnvironmentalReverbDensity(int density) override;
    virtual int getEnvironmentalReverbDensity() const override;
    virtual RetCode setEnvironmentalReverbBypass(bool bypass) override;
    virtual bool getEnvironmentalReverbBypass() const override;

  private:
    struct ReverbParams mReverbParams;

    PresetReverb::Presets mPreset;
    PresetReverb::Presets mNextPreset;

    bool isPreset();
};

} // namespace aidl::qti::effects
