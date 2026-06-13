/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <PalApi.h>
#include <aidl/android/hardware/audio/core/IModule.h>
#include <aidl/android/hardware/audio/core/VendorParameter.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <aidl/android/media/audio/common/AudioFormatDescription.h>
#include <aidl/android/media/audio/common/AudioPlaybackRate.h>
#include <aidl/android/media/audio/common/AudioPort.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <aidl/android/hardware/audio/core/ITelephony.h>
#include <aidl/android/media/audio/common/MicrophoneDynamicInfo.h>
#include <aidl/android/media/audio/common/MicrophoneInfo.h>
#include <qti-audio-core/AudioUsecase.h>
#include <system/audio.h>

#include <unordered_map>

#ifdef SEC_AUDIO_COMMON
#include <qti-audio-core/SecPlatformDefs.h>
#include "SecPalDefs.h"
#endif
#ifdef ENABLE_TAS_SPK_PROT
#include "TISpeakerProtDefs.h"
#endif
#ifdef SEC_AUDIO_SAMSUNGRECORD
#include "PreProcess.h"
#endif

#ifdef SEC_AUDIO_SAMSUNGRECORD
#define AUDIO_CAPTURE_PERIOD_DURATION_MSEC 20
#endif

namespace qti::audio::core {

struct HdmiParameters {
    int controller;
    int stream;
    pal_device_id_t deviceId;
};

enum class PlaybackRateStatus { SUCCESS, UNSUPPORTED, ILLEGAL_ARGUMENT };

using GetLatency = int32_t (*)();
using GetFrameCount =
        size_t (*)(const ::aidl::android::media::audio::common::AudioPortConfig& portConfig);
using GetBufferConfig = struct BufferConfig (*)(
        const ::aidl::android::media::audio::common::AudioPortConfig& portConfig);

/*
 * Helper to map getLatency, getFrameCount, getBufferConfig APIs
 * from platform to audiousecase. While Introducing new usecase
 * always provide the APIS.
 */
struct UsecaseOps {
    GetLatency getLatency;
    GetFrameCount getFrameCount;
    GetBufferConfig getBufferConfig;
};

template <typename UsecaseClass>
inline UsecaseOps makeUsecaseOps() {
    UsecaseOps ops;
    ops.getLatency = UsecaseClass::getLatency;
    ops.getFrameCount = UsecaseClass::getFrameCount;
    ops.getBufferConfig = UsecaseClass::getBufferConfig;
    return ops;
}

class Platform {
  private:
    explicit Platform();

    Platform(const Platform&) = delete;
    Platform& operator=(const Platform& x) = delete;

    Platform(Platform&& other) = delete;
    Platform& operator=(Platform&& other) = delete;
    static int palGlobalCallback(uint32_t event_id, uint32_t* event_data, uint64_t cookie);

  public:
    // BT related params used across
    bool bt_lc3_speech_enabled;
    static btsco_lc3_cfg_t btsco_lc3_cfg;
#ifdef SEC_AUDIO_BLUETOOTH
    bool bt_nrec{false};
    bool bt_sco_on{false};
#endif
#ifdef SEC_AUDIO_SUPPORT_BT_RVC
    bool bt_rvc_support{false};
#endif
#ifdef SEC_AUDIO_BT_OFFLOAD
    audio_format_t bt_a2dp_format{AUDIO_FORMAT_DEFAULT};
#endif
#ifdef ENABLE_TAS_SPK_PROT
    static pal_tispk_prot_param_t tiSpkProtParam;
#endif
    mutable bool mUSBCapEnable;
    int mCallState;
    int mCallMode;
#ifdef SEC_AUDIO_CALL
    float mVoiceVolume{-1.0f};
    bool mIsVoWiFi{false};
#ifdef SEC_AUDIO_CALL_SATELLITE
    bool mSatelliteCall{false};
#endif
    bool mRingbacktone{false};
    bool mVoiceMuteState[2]{false/*RX*/,false/*TX*/};
    bool mNbQuality{false};
#ifdef SEC_AUDIO_WB_AMR
    int mCallBand = WB;
#endif
#ifdef SEC_AUDIO_CALL_FORWARDING
    int mCallMemo{CALLMEMO_OFF};
    bool mCallForwarding{false};
#endif
    int mDeviceInfo{VOICE_DEVICE_INVALID};
    bool mDexConnected{false};
    bool mDexPadConnected{false};
    std::vector<::aidl::android::media::audio::common::AudioDevice> mTelephonyDevices{};
#endif
#ifdef SEC_AUDIO_ALL_SOUND_MUTE
    bool mAllSoundMute{false};
#endif
#ifdef SEC_AUDIO_CALL_HAC
    bool mHacIncall{false};
    int mHacMode{HAC_MODE_MIC};
#endif
#ifdef SEC_AUDIO_CALL_TRANSLATION
    bool mCallTranslation{false};
    int mVoiceTxControl{TRANSLATION_UNMUTE};
    int mVoiceRxControl{TRANSLATION_UNMUTE};
#endif
#ifdef SEC_AUDIO_CALL_VOIP
    bool mCngEnable{false};
    uint32_t  mVoipIsolationMode{EFFECTS_MICMODE_STANDARD};
    uint32_t  mCallIsolationMode{EFFECTS_MICMODE_STANDARD};
    std::vector<pal_device> mPalDevicesOnVoipRx{};
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    std::vector<::aidl::android::media::audio::common::AudioDevice> mOutDeepDevices{};
#endif
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
    bool mVoipViaSmartView{false};
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
    int mEnforcePlaybackState{NOT_MUTE};
#endif
#ifdef SEC_AUDIO_DUAL_SPEAKER
    bool mSpeakerLeftAmpOff{false};
#endif
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_MULTI_SPEAKER)
    int mRotationInfo{TOP_UP};
    int mFlatmotionInfo{FLATMOTION_FLAT};
#endif
#ifdef SEC_AUDIO_VOICE_TX_FOR_INCALL_MUSIC
    bool mScreenCall{false};
#endif
#ifdef SEC_AUDIO_FMRADIO
    fmradio_config_t mFM{false,AUDIO_DEVICE_NONE,0.0f,false};
#endif
#if defined(SEC_AUDIO_SUPPORT_FLIP_CALL) || defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP)
    bool mFolderclosed{false};
    bool mFlexmode{false};
#endif
#ifdef SEC_AUDIO_SUPPORT_GAMECHAT_SPK_AEC
    bool mGamechatMode{false};
#endif
#ifdef SEC_AUDIO_SAMSUNGRECORD
    uint32_t preprocess_eq_enables{S_REC};
    bool multidevice_rec{false};
    int32_t mUnconfiguredFlagsReceived{0};
#endif
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
    bool mAasEnabled{false};
    float mAasVolume{1.0f};
#endif
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    bool usb_input_dev_enabled{false};
    pal_device_id_t listenback_device{PAL_DEVICE_NONE};
    bool listenback_on{false};
    std::mutex karaoke_mutex;
#endif
#ifdef SEC_AUDIO_CAMCORDER
    bool tx_data_inversion{false};
#endif
#ifdef SEC_AUDIO_SUPPORT_NSRI
    bool is_NSRI_secure{false};
#endif
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    int register_voice_keyword{0};
    bool seamless_enabled{false};
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    int interpreter_mode{INTERPRETER_OFF};
#endif
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    bool mUSBGainForCombo{false};
#endif
#ifdef SEC_AUDIO_KARAOKE
    bool is_karaoke_on{false};
#endif
    static Platform& getInstance();

    size_t getFrameCount(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            Usecase const& inTag = Usecase::INVALID);

    struct BufferConfig getBufferConfig(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            Usecase const& inTag = Usecase::INVALID);

#ifdef SEC_AUDIO_SUPPORT_UHQ
    struct BufferConfig getBufferConfig(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            std::optional<std::pair<::aidl::android::media::audio::common::PcmType, pal_uhqa_state>>& uhqConfig,
            Usecase const& inTag);
#endif

    int32_t getLatencyMs(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            Usecase const& inTag = Usecase::INVALID);

    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> getMicrophoneInfo() {
        return mMicrophoneInfo;
    }
    std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>
            getMicrophoneDynamicInfo(
                    const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices);

    bool setParameter(const std::string& key, const std::string& value);
    bool setBluetoothParameters(const char* kvpairs);
#ifdef ENABLE_TAS_SPK_PROT
    bool setSpeakerProtectionParameters(const char* kvpairs);
#endif
    bool setVendorParameters(
            const std::vector<::aidl::android::hardware::audio::core::VendorParameter>&
                    in_parameters,
            bool in_async);

    std::string getParameter(const std::string& key) const;
    std::string toString() const;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    std::string toStringSec() const;
    void dump(int fd) const;
#endif

    bool isSoundCardUp() const noexcept;
    bool isSoundCardDown() const noexcept;

#ifdef SEC_AUDIO_BT_OFFLOAD
    bool isBluetoothA2dpDevice(const ::aidl::android::media::audio::common::AudioDevice& d) const
            noexcept;
#endif
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
    bool IsBtForMultiDevice(const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) const noexcept;
#endif

    size_t getMinimumStreamSizeFrames(
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sources,
            const std::vector<::aidl::android::media::audio::common::AudioPortConfig*>& sinks);
    std::unique_ptr<pal_stream_attributes> getPalStreamAttributes(
            const ::aidl::android::media::audio::common::AudioPortConfig& portConfig,
            const bool isInput
#ifdef SEC_AUDIO_SUPPORT_UHQ
            , std::optional<std::pair<::aidl::android::media::audio::common::PcmType, pal_uhqa_state>> uhqConfig = std::nullopt
#endif
            ) const;
    std::vector<pal_device> convertToPalDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            const noexcept;

    /*
     * @breif provides pal devices for given mixport and audiodevices.
     *
     * @param mixPortConfig mixportconfig for which devices are requested
     * @param tag usecase tag
     * @param setDevices vector of devices for which pal devices are requested
     * @param dummyDevice setDevices can be empty, in that case if client needs
     * dummy device in form of PAL_DEVICE_[IN/OUT]_DUMMY
     */

    std::vector<pal_device> configureAndFetchPalDevices(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            const Usecase& tag,
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& setDevices,
            const bool dummyDevice = false) const;

    std::vector<pal_device> getDummyPalDevices(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) const;

#ifdef SEC_AUDIO_COMMON
    std::vector<pal_device> configureSecPalDevicesForTelephony(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& setDevices) noexcept;
    std::vector<pal_device> configureSecPalDevicesForPlayback(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            const Usecase& tag,
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& setDevices) noexcept;
    std::vector<pal_device> configureSecPalDevicesForCapture(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            const Usecase& tag,
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& setDevices) const noexcept;
#endif
    /*
    * @breif In order to get stream position in the DSP pipeline
    *
    * @param,
    * Input Parameters:
    * palHandle, a valid stream pal handle
    * sampleRate, a valid stream sample rate
    *
    * Output Parameters:
    * dspFrames, num of frames delivered by DSP
    */
    void getPositionInFrames(pal_stream_handle_t* palHandle, int32_t const& sampleRate,
                                   int64_t* const dspFrames) const;

    /*
    * @brief requiresBufferReformat is used to check if format converter is needed for
    * a PCM format or not, it is not applicable for compressed formats.
    * It is possible that framework can use a format which might not
    * be supported at below layers, so HAL needs to convert the buffer in desired format
    * before writing.
    *
    * @param portConfig : mixport config of the stream.
    * return return a pair of input and output audio_format_t in case a format converter
    * is needed, otherwise nullopt.
    * For example, mix port using audio format FLOAT is not supported, closest to FLOAT,
    * INT_32 can be used as target format. so, return a pair of
    * <AUDIO_FORMAT_PCM_FLOAT, AUDIO_FORMAT_PCM_32_BIT>
    * Caller can utilize this to create a converter based of provided input, output formats.
    */
    static std::optional<std::pair<audio_format_t, audio_format_t>> requiresBufferReformat(
            const ::aidl::android::media::audio::common::AudioPortConfig& portConfig);

    /*
    * @brief creates a pal payload for a pal volume and sets to PAL
    * @param handle : valid pal stream handle
    * @param volumes vector of volumes in floats
    * return 0 in success, error code otherwise
    */
    int setVolume(pal_stream_handle_t* handle, const std::vector<float>& volumes) const;
#ifdef SEC_AUDIO_COMMON
    std::vector<uint8_t> getPalVolume(const std::vector<float>& volumes);
#endif

    std::unique_ptr<pal_buffer_config_t> getPalBufferConfig(const size_t bufferSize,
                                                            const size_t bufferCount) const;
    std::vector<::aidl::android::media::audio::common::AudioProfile> getDynamicProfiles(
            const ::aidl::android::media::audio::common::AudioPort& dynamicDeviceAudioPort) const;
    int handleDeviceConnectionChange(
            const ::aidl::android::media::audio::common::AudioPort& deviceAudioPort,
            const bool isConnect) const;
    uint32_t getBluetoothLatencyMs(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>&
                    bluetoothDevices);
    std::unique_ptr<pal_stream_attributes> getDefaultTelephonyAttributes() const;
    std::unique_ptr<pal_stream_attributes> getDefaultCRSTelephonyAttributes() const;
    void configurePalDevicesCustomKey(std::vector<pal_device>& palDevices,
                                      const std::string& customKey) const;

    bool setStreamMicMute(pal_stream_handle_t* streamHandlePtr, const bool muted);
    bool getMicMuteStatus();
    void setMicMuteStatus(bool mute);
    bool updateScreenState(const bool isTurnedOn) noexcept;
    bool isScreenTurnedOn() const noexcept;
#ifdef SEC_AUDIO_SPK_AMP_MUTE
    bool getSpeakerMuteStatus() const noexcept;
    void setSpeakerMute(const bool mute) noexcept;
#endif

    bool isHDREnabled() const { return mHDREnabled; }
    void setHDREnabled(bool const& enable) { mHDREnabled = enable; }

    int32_t getHDRSampleRate() const { return mHDRSampleRate; }

    void setHDRSampleRate(int32_t const& sampleRate) { mHDRSampleRate = sampleRate; }

    int32_t getHDRChannelCount() const { return mHDRChannelCount; }

    void setHDRChannelCount(int32_t const& channelCount) { mHDRChannelCount = channelCount; }

    bool isWNREnabled() const { return mWNREnabled; }
    void setWNREnabled(bool const& enable) { mWNREnabled = enable; }

    bool isANREnabled() const { return mANREnabled; }
    void setANREnabled(bool const& enable) { mANREnabled = enable; }

    bool isInverted() const { return mInverted; }
    void setInverted(bool const& enable) { mInverted = enable; }

    std::string getOrientation() const { return mOrientation; }

    void setOrientation(std::string const& value) { mOrientation = value; }

    std::string getFacing() const { return mFacing; }

    void setFacing(std::string const& value) { mFacing = value; }

    void setTelephony(const std::weak_ptr<::aidl::android::hardware::audio::core::ITelephony> telephony) noexcept {
        mTelephony = telephony;
    }

    std::weak_ptr<::aidl::android::hardware::audio::core::ITelephony> getTelephony() const noexcept {
        return mTelephony;
    }

    /*
    * @brief creates a pal payload for a speed factor and sets to PAL
    * @param handle : pal stream handle
    * @param tag usecase tag
    * @param playbackRate  playback rate to be set
    * return PlaybackRateStatus::SUCCESS on success, or if stream handle is not set.
    * return PlaybackRateStatus::UNSUPPORTED operation, usecase does not support speed operations
    * or speed parameters are not in the range
    * return PlaybackRateStatus::ILLEGAL_ARGUMENT in case of any other failure
    */
    PlaybackRateStatus setPlaybackRate(
            pal_stream_handle_t* handle, const Usecase& tag,
            const ::aidl::android::media::audio::common::AudioPlaybackRate& playbackRate);

    void setInCallMusicState(const bool state) noexcept { mInCallMusicEnabled = state; }
    bool getInCallMusicState() noexcept { return mInCallMusicEnabled; }

    // Set and Get Value Functions for Translate Record.
    void setTranslationRecordState(const bool state) noexcept { mIsTranslationRecordEnabled = state; }
    bool getTranslationRecordState() noexcept { return mIsTranslationRecordEnabled; }

    // Set and Get Value Functions for Voice Call Volume mute during Translation Record Usecase.
    void setTranslationRxMuteState(const bool state) noexcept { mIsTranslationRxMuteEnabled = state; }
    bool getTranslationRxMuteState() noexcept { return mIsTranslationRxMuteEnabled; }

    void setHACEnabled(const bool& enable) noexcept { mIsHACEnabled = enable; }

    bool isHACEnabled() const noexcept { return mIsHACEnabled; }

    void updateCallState(int callState) { mCallState = callState; }
    void updateCallMode(int callMode) { mCallMode = callMode; }

    int getCallState() { return mCallState; }
    int getCallMode() { return mCallMode; }

#ifdef SEC_AUDIO_CALL
    void setTelephonyDevices(const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {
        mTelephonyDevices = devices;
    }
    std::vector<::aidl::android::media::audio::common::AudioDevice> getTelephonyDevices()
                                                                        const { return mTelephonyDevices; }
    void setVoiceVolume(const float volume) { mVoiceVolume = volume; }
    float getVoiceVolume() { return mVoiceVolume; }
    void updateVoWiFiState(bool isVoWiFi) { mIsVoWiFi = isVoWiFi; }
    bool getVoWiFiState() { return mIsVoWiFi; }
#ifdef SEC_AUDIO_CALL_SATELLITE
    void setSatelliteCall(bool enable) { mSatelliteCall = enable; }
    bool getSatelliteCall() { return mSatelliteCall; }
#endif
    void setNbQuality(bool enable) { mNbQuality = enable; }
    bool getNbQuality() { return mNbQuality; }
    void setRingbacktone(bool enable) { mRingbacktone = enable; }
    bool getRingbacktone() { return mRingbacktone; }
    void setVoiceMuteState(int dir, bool mute) { mVoiceMuteState[dir] = mute; }
    bool getVoiceMuteState(int dir) { return mVoiceMuteState[dir]; }
#ifdef SEC_AUDIO_WB_AMR
    void updateSecCallBand(int callBand) { mCallBand = callBand; }
    int getSecCallBand() { return mCallBand; }
#endif
#ifdef SEC_AUDIO_CALL_FORWARDING
    bool isCallForwarding();
    void setCallForwarding(bool enable) { mCallForwarding = enable; }
    bool getCallForwarding() { return mCallForwarding; };
    void setCallMemo(int mode) { mCallMemo = mode; }
    int getCallMemo() { return mCallMemo; };
#endif
    void setDexConnected(bool connected) { mDexConnected = connected; }
    bool getDexConnected() { return mDexConnected; };
    void setDexPadConnected(bool connected) { mDexPadConnected = connected; }
    bool getDexPadConnected() { return mDexPadConnected; };
    void setDeviceInfo(int deviceType) { mDeviceInfo = deviceType; }
    int getDeviceInfo() { return mDeviceInfo;}
    int GetDeviceType(pal_device_id_t rx_device_id);
#endif
#ifdef SEC_AUDIO_ALL_SOUND_MUTE
    void setAllSoundMute(bool mute) { mAllSoundMute = mute; }
    bool getAllSoundMute() { return mAllSoundMute; }
#endif
#ifdef SEC_AUDIO_CALL_HAC
    void setHacIncall(bool mode) { mHacIncall = mode; }
    bool getHacIncall() { return mHacIncall; }
    void setHacMode(int mode) { mHacMode = mode; }
    int getHacMode() { return mHacMode; }

    int GetHacCustomKeyId();
    int GetVoWifiHacCustomKeyId();
#endif
#ifdef SEC_AUDIO_BLUETOOTH
    void setBtNrecState(bool isOn) { bt_nrec = isOn; }
    bool isBtNrecOn() { return bt_nrec; }
    void setBtScoState(bool isOn) { bt_sco_on = isOn; }
    bool isBtScoOn() { return bt_sco_on; }
#endif
#ifdef SEC_AUDIO_SUPPORT_BT_RVC
    void setBtRvcSupportState(bool isOn) { bt_rvc_support = isOn; }
    bool isBtRvcSupportState() { return bt_rvc_support; }
#endif
#ifdef SEC_AUDIO_BT_OFFLOAD
    void setBtA2dpFormat(audio_format_t format) { bt_a2dp_format = format; }
    audio_format_t getBtA2dpFormat() { return bt_a2dp_format; }
#endif
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
    void setVoipViaSmartView(bool enable) { mVoipViaSmartView = enable; }
    bool getVoipViaSmartView() { return mVoipViaSmartView; }
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
    void updateEnforcePlaybackState(int mode) { mEnforcePlaybackState = mode; }
    int getEnforcePlaybackState() { return mEnforcePlaybackState; }
#endif
#ifdef SEC_AUDIO_DUAL_SPEAKER
    void setSpeakerLeftAmpOff(bool isOff) { mSpeakerLeftAmpOff = isOff; }
    bool isSpeakerLeftAmpOff() { return mSpeakerLeftAmpOff; }
#endif
#ifdef SEC_AUDIO_CALL_TRANSLATION
    void setCallTranslation(bool mode) { mCallTranslation = mode; }
    bool getCallTranslation() { return mCallTranslation; }
    void setVoiceTxControl(int mode) { mVoiceTxControl = mode; }
    int getVoiceTxControl() { return mVoiceTxControl; }
    void setVoiceRxControl(int mode) { mVoiceRxControl = mode; }
    int getVoiceRxControl() { return mVoiceRxControl; }
#endif
#ifdef SEC_AUDIO_CALL_VOIP
    void setCngEnable(bool enable) { mCngEnable = enable; }
    bool getCngEnable() { return mCngEnable; }
    void setVoipIsolationMode(int mode) { mVoipIsolationMode = mode; }
    uint32_t getVoipIsolationMode() { return mVoipIsolationMode; }
    void setCallIsolationMode(int mode) { mCallIsolationMode = mode; }
    uint32_t getCallIsolationMode() { return mCallIsolationMode; }
#endif
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_MULTI_SPEAKER)
    void updateRotationInfo(int mode) { mRotationInfo = mode; }
    int getRotationInfo() { return mRotationInfo; }
    void updateFlatmotionInfo(int mode) { mFlatmotionInfo = mode; }
    int getFlatmotionInfo() { return mFlatmotionInfo; }
#endif
#ifdef SEC_AUDIO_VOICE_TX_FOR_INCALL_MUSIC
    void setScreenCall(bool mode) { mScreenCall = mode; }
    bool getScreenCall() { return mScreenCall; }
#endif
#ifdef SEC_AUDIO_FMRADIO
    void setFMRadioOn(bool on) { mFM.on = on; }
    bool getFMRadioOn() { return mFM.on; }
    void setFMRadioDevice(audio_devices_t device) { mFM.device = device; }
    audio_devices_t getFMRadioDevice() { return mFM.device; }
    void setFMRadioVolume(float volume) { mFM.volume = volume; }
    float getFMRadioVolume() { return mFM.volume; }
    void setFMRadioMute(bool mute) { mFM.mute = mute; }
    bool getFMRadioMute() { return mFM.mute; }
#endif
#if defined(SEC_AUDIO_SUPPORT_FLIP_CALL) || defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP)
    void setFolderclosed(bool closed){ mFolderclosed = closed; };
    bool getFolderclosed(){ return mFolderclosed; };
    void setFlexmode(bool mode){ mFlexmode = mode; };
    bool getFlexmode(){ return mFlexmode; };
#endif
#ifdef SEC_AUDIO_SUPPORT_GAMECHAT_SPK_AEC
    void setGamechatMode(bool mode) { mGamechatMode = mode; }
    bool getGamechatMode() { return mGamechatMode; };
#endif
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
    void setAasEnabled(const bool enable) { mAasEnabled = enable; }
    bool isAasEnabled() { return mAasEnabled; }
    void setAasVolume(const float volume) { mAasVolume = volume; }
    float getAasVolume() { return mAasVolume; }
#endif
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    void setUsbInputEnabled(bool mode) { usb_input_dev_enabled = mode; }
    bool isUsbInputEnabled() { return usb_input_dev_enabled; };
    void setListenbackDevice(pal_device_id_t device) { listenback_device = device; }
    pal_device_id_t getListenbackDevice() { return listenback_device; };
    void setListenBackEnabled(bool mode) { listenback_on = mode; }
    bool isListenBackEnabled() { return listenback_on; };
#endif
#ifdef SEC_AUDIO_CAMCORDER
    void setTxDataInversion(bool on) { tx_data_inversion = on; }
    bool isTxDataInversionEnabled() { return tx_data_inversion; }
#endif
#ifdef SEC_AUDIO_SUPPORT_NSRI
    void setNSRISecureEnabled(bool on) { is_NSRI_secure = on; }
    bool isNSRISecureEnabled() { return is_NSRI_secure; }
#endif
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    void setRegisterVoiceKeyword(int value) { register_voice_keyword = value; }
    void setSeamlessEnabled(bool on) { seamless_enabled = on; }
    bool isSeamlessEnabled() { return seamless_enabled; }
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
    void setInterpreterMode(int mode) { interpreter_mode = mode; }
    int getInterpreterMode() { return interpreter_mode; }
#endif
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    void setUSBGainForCombo(bool on) { mUSBGainForCombo = on; }
    bool getUSBGainForCombo() { return mUSBGainForCombo; }
#endif
    bool isA2dpSuspended();

    void setWFDProxyChannels(const uint32_t numProxyChannels) noexcept;
    void setProxyRecordFMQSize(const size_t& FMQSize) noexcept;
    size_t getProxyRecordFMQSize() const noexcept;
    uint32_t getWFDProxyChannels() const noexcept;
    /* Check if proxy record session is active in  PAL_DEVICE_IN_RECORD_PROXY */
    std::string IsProxyRecordActive() const noexcept;
    bool isIPAsProxyDeviceConnected() const noexcept { return mIsIPAsProxyConnected; };
    void setIPAsProxyDeviceConnected(bool isIPAsProxy) noexcept { mIsIPAsProxyConnected = isIPAsProxy; };

    void setHapticsVolume(const float hapticsVolume) const noexcept;
    void setHapticsIntensity(const int hapticsIntensity) const noexcept;
#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK
    void setHapticsSource(const int hapticsSource) const noexcept;
#endif

    void updateUHQA(const bool enable) noexcept;
#ifdef SEC_AUDIO_SUPPORT_UHQ
    void updateUHQA(const pal_uhqa_state sample) noexcept;
#endif
    bool isUHQAEnabled() const noexcept;
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
    void triggerSpeakerCalibration() const noexcept;
#endif
    void setFTMSpeakerProtectionMode(uint32_t const heatUpTime, uint32_t const runTime,
                                     bool const isFactoryTest, bool const isValidationMode,
                                     bool const isDynamicCalibration) const noexcept;
    std::optional<std::string> getFTMResult() const noexcept;
    std::optional<std::string> getSpeakerCalibrationResult() const noexcept;
#ifdef ENABLE_TAS_SPK_PROT
    std::optional<std::string> getSpeakerProtectionResult() const noexcept;
#endif

    void updateScreenRotation(const ::aidl::android::hardware::audio::core::IModule::ScreenRotation
                                      in_rotation) noexcept;
    ::aidl::android::hardware::audio::core::IModule::ScreenRotation getCurrentScreenRotation() const
            noexcept;

    bool platformSupportsOffloadSpeed() { return mOffloadSpeedSupported; }
    bool usecaseSupportsOffloadSpeed(const Usecase& tag) {
        return platformSupportsOffloadSpeed() && isOffload(tag);
    }

    bool isOffload(const Usecase& tag) { return tag == Usecase::COMPRESS_OFFLOAD_PLAYBACK; }
    int setLatencyMode(uint32_t mode);
    int getRecommendedLatencyModes(
          std::vector<::aidl::android::media::audio::common::AudioLatencyMode>* _aidl_return);

    void configurePalDevices(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            std::vector<pal_device>& palDevices);
    void setHdrOnPalDevice(pal_device* palDeviceIn);
    bool isHDRARMenabled();
    bool isHDRSPFEnabled();
    bool getUSBCapEnable() { return mUSBCapEnable; }

#ifdef SEC_AUDIO_SAMSUNGRECORD
    bool GetRecMultiMic(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig, const std::vector<::aidl::android::media::audio::common::AudioDevice>& connectedDevices, Usecase tag) const noexcept;
    int match_device_enums(const ::aidl::android::media::audio::common::AudioDevice& device) const noexcept;
    int get_device_types(const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) const noexcept;
    bool IsSupportPreprocess(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig, Usecase tag) const noexcept;
    uint32_t GetBufferSize(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig);
    int GetRecFormat(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig, const std::vector<::aidl::android::media::audio::common::AudioDevice>& connectedDevices, Usecase tag);
    uint32_t SelectPreProcessSolutions(const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) const noexcept;
    bool isLoopBackOff() { return mIsLoopBackOff; }
    bool isRmsTestMode() { return mIsRmsTestMode; }
    void setLoopBackOff(bool enable) { mIsLoopBackOff = enable; }
    void setRmsTestMode(bool enable) { mIsRmsTestMode = enable; }
    void setUnconfiguredFlagsReceived(int32_t flags) { mUnconfiguredFlagsReceived = flags; }
    int32_t getUnconfiguredFlagsReceived() { return mUnconfiguredFlagsReceived; }
#endif
#ifdef SEC_AUDIO_KARAOKE
    void setKaraokeEnabled(bool on) { is_karaoke_on = on; }
    bool isKaraokeEnabled() { return is_karaoke_on; }
    static bool isKaraokeUsecases(const Usecase& tag);
#endif
#ifdef SEC_AUDIO_COMMON
    bool isSecAudioFeatureSupported() { return mIsSecAudioFeatureSupported; }
    void setSecAudioFeatureSupported(bool supported) { mIsSecAudioFeatureSupported = supported; }
#endif
#if defined(SEC_AUDIO_OFFLOAD_COMPRESSED_OPUS) && defined(SEC_AUDIO_OFFLOAD_SOUNDSPEED)
    PlaybackRateStatus setSecPlaybackRate(pal_stream_handle_t* handle, const Usecase& tag,
            const ::aidl::android::media::audio::common::AudioPlaybackRate& playbackRate,
            const std::optional< ::aidl::android::media::audio::common::AudioOffloadInfo>& offloadInfo,
            std::function<void(const float&)> sendSpeed);
    bool isSecSupportsOffloadSpeed(const std::optional< ::aidl::android::media::audio::common::AudioOffloadInfo>& offloadInfo);
#endif
  private:
    void customizePalDevices(
            const ::aidl::android::media::audio::common::AudioPortConfig& mixPortConfig,
            const Usecase& tag, std::vector<pal_device>& palDevices) const noexcept;
    void configurePalDevicesForHIFIPCMFilter(std::vector<pal_device>&) const noexcept;
    std::vector<::aidl::android::media::audio::common::AudioProfile> getUsbProfiles(
            const ::aidl::android::media::audio::common::AudioPort& port) const;

    std::optional<struct HdmiParameters> getHdmiParameters(
            const ::aidl::android::media::audio::common::AudioDevice&) const;

    void initUsecaseOpMap();

  public:
    constexpr static uint32_t kDefaultOutputSampleRate = 48000;
    constexpr static uint32_t kDefaultPCMBidWidth = 16;
    constexpr static pal_audio_fmt_t kDefaultPalPCMFormat = PAL_AUDIO_FMT_PCM_S16_LE;
    constexpr static int32_t kDefaultLatencyMs = 51;

  private:
    std::vector<::aidl::android::media::audio::common::AudioDevice> mPrimaryPlaybackDevices{};

    std::map<std::string, std::string> mParameters;
    card_status_t mSndCardStatus{CARD_STATUS_OFFLINE};
    bool mInCallMusicEnabled{false};
    bool mIsTranslationRecordEnabled{false};
    bool mIsTranslationRxMuteEnabled{false};
    bool mIsScreenTurnedOn{false};
    uint32_t mWFDProxyChannels{0};
    bool mIsUHQAEnabled{false};
    bool mIsIPAsProxyConnected{false};
    ::aidl::android::hardware::audio::core::IModule::ScreenRotation mCurrentScreenRotation{
            ::aidl::android::hardware::audio::core::IModule::ScreenRotation::DEG_0};
    bool mOffloadSpeedSupported = false;
    bool mMicMuted = false;
#ifdef SEC_AUDIO_SPK_AMP_MUTE
    bool mSpeakerMuted = false;
#endif
#ifdef SEC_AUDIO_COMMON
    bool mIsSecAudioFeatureSupported{false};
#endif
    /* HDR */
    bool mHDREnabled{false};
    int32_t mHDRSampleRate{0};
    int32_t mHDRChannelCount{0};
    bool mWNREnabled{false};
    bool mANREnabled{false};
    bool mInverted{false};
    std::string mOrientation{""};
    std::string mFacing{""};

    /* HAC enablement*/
    bool mIsHACEnabled{false};

    std::unordered_map<Usecase, UsecaseOps> mUsecaseOpMap;
    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> mMicrophoneInfo;
    using PalDevToMicDynamicInfoMap = std::unordered_map<
            pal_device_id_t,
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>>;
    PalDevToMicDynamicInfoMap mMicrophoneDynamicInfoMap;
    // proxy related info
    size_t mProxyRecordFMQSize{0};
    std::weak_ptr<::aidl::android::hardware::audio::core::ITelephony> mTelephony;
#ifdef SEC_AUDIO_SAMSUNGRECORD
    bool mIsLoopBackOff{true};
    bool mIsRmsTestMode{false};
#endif
};
} // namespace qti::audio::core
