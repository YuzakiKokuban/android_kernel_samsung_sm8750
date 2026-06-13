/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <aidl/android/hardware/audio/core/BnTelephony.h>
#include <aidl/android/media/audio/common/AudioDevice.h>
#include <extensions/AudioExtension.h>
#include <qti-audio-core/Platform.h>
#include <qti-audio-core/Stream.h>

#include <android/binder_enums.h>

#ifdef SEC_AUDIO_COMMON
#include "SecFTM.h"
#include "AudioEffect.h"
#endif

namespace qti::audio::core {

class Telephony : public ::aidl::android::hardware::audio::core::BnTelephony {
  public:
    Telephony();
    virtual ~Telephony() override;

    enum class CallState : uint8_t {
        IN_ACTIVE = 1,
        ACTIVE = 2,
#ifdef SEC_AUDIO_CALL_SATELLITE
        EXTMODEM_ACTIVE = 3,
#endif
    };
    friend std::ostream& operator<<(std::ostream& os, const CallState& state);
    enum class VSID : int64_t {
        VSID_1 = 0x11C05000,
        VSID_2 = 0x11DC5000,
#ifdef SEC_AUDIO_COMMON
        VSID_LB_1 = 0x12006000,
        VSID_LB_2 = 0x121C6000,
#endif
    };
    friend std::ostream& operator<<(std::ostream& os, const VSID& vsid);
    using CallType = std::string;

    struct SetUpdates {
        /*
         call state is key the set update which decides validity or need of
         other parameters.
         */
        CallState mCallState{CallState::IN_ACTIVE};
        CallType mCallType{""};
        bool mIsCrsCall{false};
        VSID mVSID{VSID::VSID_1};
        std::string toString() const {
            std::ostringstream os;
            os << "{ mCallState:" << mCallState << ", mVSID:" << mVSID
               << ", mIsCrsCall:" << mIsCrsCall << ", mCallType:" << mCallType << "}";
            return os.str();
        }
    };

    struct CallStatus {
        CallState current_;
        CallState new_;
    };

    float mCRSVolume = 0.4f; //default CRS call volume
    bool mIsCRSStarted{false};
    VSID mCRSVSID{VSID::VSID_1};
    constexpr static size_t KCodecBackendDefaultBitWidth = 16;
    const static ::aidl::android::media::audio::common::AudioDevice kDefaultRxDevice;
    const static ::aidl::android::media::audio::common::AudioDevice kDefaultCRSRxDevice;
    static constexpr int32_t VSID1_VOICE_SESSION = 0;
    static constexpr int32_t VSID2_VOICE_SESSION = 1;
    static constexpr int32_t MAX_VOICE_SESSIONS = 2;
    static constexpr int32_t MIN_CRS_VOL_INDEX = 0;
    static constexpr int32_t MAX_CRS_VOL_INDEX = 7;
    struct SetUpdateSession {
        CallStatus state;
        SetUpdates CallUpdate;
    };

    struct VoiceSession {
        SetUpdateSession session[MAX_VOICE_SESSIONS];
    };
    VoiceSession mVoiceSession;

#ifdef SEC_AUDIO_CALL
    VSID mCurVSID{VSID::VSID_1};
    bool mIsVolteVT{false};
    bool mIsVoWiFi{false};
    int mCallBand = WB;
#endif

    /* All the public APIs are guarded by mLock, Hence never call a public
     * API from anther public API */
  public:
    ndk::ScopedAStatus getSupportedAudioModes(
            std::vector<::aidl::android::media::audio::common::AudioMode>* _aidl_return) override;
    ndk::ScopedAStatus switchAudioMode(
            ::aidl::android::media::audio::common::AudioMode in_mode) override;
    ndk::ScopedAStatus setTelecomConfig(const TelecomConfig& in_config,
                                        TelecomConfig* _aidl_return) override;

    /* This API is called when there are "TELEPHONY related set parameters"
    on the primary module */
    void reconfigure(const SetUpdates& setUpdates);

    void updateVolumeBoost(const bool enable);
    void updateSlowTalk(const bool enable);
    void updateHDVoice(const bool enable);
    void updateDeviceMute(const bool isMute, const std::string& muteDirection);
#ifdef SEC_AUDIO_CALL
    void updateSecCallState(const int state);
    void updateSecVSID(const int vsid);
    void updateSecCallBand(const int band);
    void configureMicMode();
    void setCallForwarding(bool enable);
#endif
#ifdef SEC_AUDIO_CALL_SATELLITE
    void configureExtModemCall();
#endif
    bool isCrsCallSupported();
    void setCRSVolumeFromIndex(const int index);
    void updateVoiceVolume();
#ifdef SEC_AUDIO_CALL_SATELLITE
    void updateExtModemCallVolume();
    void updateExtModemMicMute();
#endif
    void setMicMute(const bool muted);
    void updateCalls();

    // The following below API are both aimed to solve routing on telephony
    /**
    * brief sets Rx and Tx devices from device to device patch.
    * @param devices devices obtained from the patch
    * @param updateRx whether device update is for rx devices or tx devices.
    * true in case when rx devices needs updation, false otherwise.
    */
    void setDevices(const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
                    const bool updateRx);
    /**
     * The following API resets the RX and TX device
     * @param resetRx, indicates device to reset, true for RX, false for TX
     **/
    void resetDevices(const bool resetRx);

#ifdef SEC_AUDIO_COMMON
    void updateLoopback(const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
                        const bool loopbackon);
#endif

    // Telephony to decide its strategy where there is external device connection change
    void onExternalDeviceConnectionChanged(
            const ::aidl::android::media::audio::common::AudioDevice& extDevice,
            const bool& connect);

    /* Telephony to act on primary stream devices change */
    void onOutputPrimaryStreamDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>&);

    /* Telephony to act upon bluetooth sco enabled or disabled */
    void onBluetoothScoEvent(const bool& enable);

    /* set the voip stream */
    void setVoipPlaybackStream(std::weak_ptr<StreamCommonInterface> voipStream);

    /* called on playback stream start/close */
    void onPlaybackStart();
    void onPlaybackClose();

    void updateVoiceMetadataForBT(bool call_active);
    std::weak_ptr<StreamOut> mStreamOutPrimary;
    std::weak_ptr<StreamIn> mStreamInPrimary;

  protected:
    ndk::ScopedAStatus startCall();
    ndk::ScopedAStatus stopCall();
    void VoiceStop();
    void configureVolumeBoost();
    void configureSlowTalk();
    void configureHDVoice();
    void configureDeviceMute();
    void updateDevices();
    void updateTtyMode();
    void updateCrsDevice();
    void startCrsLoopback();
    void stopCrsLoopback();
    void triggerHACinVoipPlayback();
    ::aidl::android::media::audio::common::AudioDevice getMatchingTxDevice(
            const ::aidl::android::media::audio::common::AudioDevice & rxDevice);
    bool isAnyCallActive();

#ifdef SEC_AUDIO_RECOVERY
    void initSecConfig();
#endif

  protected:
    // Gaurd all the public APIs
    std::mutex mLock;
    TelecomConfig mTelecomConfig;
    const std::vector<::aidl::android::media::audio::common::AudioMode> mSupportedAudioModes = {
            ::aidl::android::media::audio::common::AudioMode::NORMAL,
            ::aidl::android::media::audio::common::AudioMode::RINGTONE,
            ::aidl::android::media::audio::common::AudioMode::IN_CALL,
            ::aidl::android::media::audio::common::AudioMode::IN_COMMUNICATION,
            ::aidl::android::media::audio::common::AudioMode::CALL_SCREEN,
    };

    ::aidl::android::media::audio::common::AudioMode mAudioMode{
            ::aidl::android::media::audio::common::AudioMode::NORMAL};

    SetUpdates mSetUpdates{};
    bool mIsVolumeBoostEnabled{false};
    bool mIsSlowTalkEnabled{false};
    bool mIsHDVoiceEnabled{false};
    bool mIsDeviceMuted{false};
    bool hasValidPlaybackStream{false};
    bool mIsVoiceStarted{false};
    std::string mMuteDirection{""};

    using TtyMap = std::map<TelecomConfig::TtyMode, pal_tty_t>;
    const TtyMap mTtyMap{
            {TelecomConfig::TtyMode::OFF, PAL_TTY_OFF},
            {TelecomConfig::TtyMode::FULL, PAL_TTY_FULL},
            {TelecomConfig::TtyMode::HCO, PAL_TTY_HCO},
            {TelecomConfig::TtyMode::VCO, PAL_TTY_VCO},
    };

    ::aidl::android::media::audio::common::AudioDevice mRxDevice; // speaker, earpiece
    ::aidl::android::media::audio::common::AudioDevice mTxDevice; // mic, speaker mic
    pal_stream_handle_t* mPalCrsHandle{nullptr};
    pal_stream_handle_t* mPalHandle{nullptr};
    // Stream Handle for VOIP Playback
    std::weak_ptr<StreamCommonInterface> mVoipStreamWptr;
    Platform& mPlatform{Platform::getInstance()};
#ifdef SEC_AUDIO_COMMON
    SecFTM& mSecFTM{SecFTM::getInstance()};
    AudioEffect& mAudioEffect{AudioEffect::getInstance()};
    AudioExtension& mAudExt{AudioExtension::getInstance()};
#endif
};

} // namespace qti::audio::core
