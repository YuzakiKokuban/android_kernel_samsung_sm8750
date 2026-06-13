/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <qti-audio-core/AudioUsecase.h>
#include <qti-audio-core/Stream.h>
#include <system/audio_effects/effect_uuid.h>

#ifdef SEC_AUDIO_COMMON
#include "SecFTM.h"
#endif
#ifdef SEC_AUDIO_DYNAMIC_NREC
#include "AudioEffect.h"
#endif

namespace qti::audio::core {

class StreamInPrimary : public StreamIn, public StreamCommonImpl {
  public:
    friend class ndk::SharedRefBase;
    StreamInPrimary(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

    virtual ~StreamInPrimary() override;

    int32_t setAggregateSinkMetadata(bool voiceActive) override;
    // Methods of 'DriverInterface'.
    ::android::status_t init() override;
    ::android::status_t drain(
            ::aidl::android::hardware::audio::core::StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t refinePosition(
            ::aidl::android::hardware::audio::core::StreamDescriptor::Reply*
            /*reply*/) override;
    void shutdown() override;

    // methods of StreamCommonInterface

    ndk::ScopedAStatus getVendorParameters(
            const std::vector<std::string>& in_ids,
            std::vector<::aidl::android::hardware::audio::core::VendorParameter>* _aidl_return)
            override;
    ndk::ScopedAStatus setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async) override;
    ndk::ScopedAStatus addEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;
    ndk::ScopedAStatus removeEffect(
            const std::shared_ptr<::aidl::android::hardware::audio::effect::IEffect>& in_effect)
            override;

    ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) override;

    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;

    // Methods called IModule
    ndk::ScopedAStatus setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            override;
    ndk::ScopedAStatus reconfigureConnectedDevices() override;
    void setStreamMicMute(const bool muted) override;
    ndk::ScopedAStatus configureMMapStream(int32_t* fd, int64_t* burstSizeFrames, int32_t* flags,
                                           int32_t* bufferSizeFrames) override;
#ifdef SEC_AUDIO_COMMON
    ndk::ScopedAStatus ForceSetDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            bool force = false) override;
    bool isStreamUsecase(Usecase tag) override { return (mTag == tag); }
    bool isDeviceAvailable(pal_device_id_t pal_device) override;
    bool HasPalStreamHandle() override { return (mPalHandle != nullptr) ?  true : false; }
    void forceShutdown() override { return shutdown(); }
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    void RerouteForInterpreter() override;
#endif
#ifdef SEC_AUDIO_CAMCORDER
    bool isTxDataInvertable();
#endif
    void onClose() override { defaultOnClose(); }
    static std::mutex sinkMetadata_mutex_;
    void checkHearingAidRoutingForVoice(const Metadata& metadata, bool voiceActive);

  protected:
    /*
     * This API opens, configures and starts pal stream.
     * also responsible for validity of pal handle.
     */
    void configure();
    void resume();
    void shutdown_I();
    /* burst zero indicates that burst command with zero bytes issued from framework */
    ::android::status_t burstZero();
    ::android::status_t startMMAP();
    ::android::status_t stopMMAP();
    size_t getPlatformDelay() const noexcept;

    // API which are *_I are internal 
    ndk::ScopedAStatus configureConnectedDevices_I();

#ifdef SEC_AUDIO_BLE_OFFLOAD
    void updateRecordMetadataForBLE();
#endif

    const Usecase mTag;
    const std::string mTagName;
    const size_t mFrameSizeBytes;

    // All the public must check the validity of this resource, if using
    pal_stream_handle_t* mPalHandle{nullptr};

    std::variant<std::monostate, PcmRecord, CompressCapture, VoipRecord, MMapRecord,
                 VoiceCallRecord, FastRecord, UltraFastRecord, HotwordRecord>  mExt;

    // references
    Platform& mPlatform{Platform::getInstance()};
    const ::aidl::android::media::audio::common::AudioPortConfig& mMixPortConfig;
#ifdef SEC_AUDIO_SAMSUNGRECORD
    unsigned int mDeviceChannels;
    audio_format_t mDeviceFormat;
#endif
#ifdef SEC_AUDIO_COMMON
    SecFTM& mSecFTM{SecFTM::getInstance()};
#endif
#ifdef SEC_AUDIO_DYNAMIC_NREC
    AudioEffect& mAudioEffect{AudioEffect::getInstance()};
#endif
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    AudioExtension& mAudExt{AudioExtension::getInstance()};
#endif
  private:
    ::android::status_t onReadError(const size_t sleepFrameCount);
    struct BufferConfig getBufferConfig();
    void applyEffects();

    bool mAECEnabled = false;
    bool mNSEnabled = false;
    bool mEffectsApplied = true;
    std::string mLogPrefix = "";
};

} // namespace qti::audio::core
