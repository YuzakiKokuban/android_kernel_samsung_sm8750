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

#define LOG_TAG "AHAL_Telephony_QTI"
#include <Utils.h>
#include <android-base/logging.h>
#include <android/binder_to_string.h>
#include <hardware/audio.h>
#include <qti-audio-core/StreamInPrimary.h>
#include <qti-audio-core/StreamOutPrimary.h>
#include <qti-audio-core/Telephony.h>
#include <qti-audio-core/Utils.h>
#include <system/audio.h>

#ifdef SEC_AUDIO_FMRADIO
#include <cutils/str_parms.h>
#include <qti-audio-core/SecParameters.h>
#endif
#ifdef SEC_AUDIO_CALL_VOIP
#include <qti-audio-core/SecModulePrimary.h>
#endif

using aidl::android::hardware::audio::common::isValidAudioMode;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::Boolean;
using aidl::android::media::audio::common::Float;

namespace qti::audio::core {

const AudioDevice Telephony::kDefaultRxDevice =
        AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER_EARPIECE};
const AudioDevice Telephony::kDefaultCRSRxDevice =
        AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};

Telephony::Telephony() {
    mVoiceSession.session[VSID1_VOICE_SESSION].CallUpdate.mVSID = VSID::VSID_1;
    mVoiceSession.session[VSID1_VOICE_SESSION].state.current_ = CallState::IN_ACTIVE;
    mVoiceSession.session[VSID1_VOICE_SESSION].state.new_ = CallState::IN_ACTIVE;
    mVoiceSession.session[VSID2_VOICE_SESSION].CallUpdate.mVSID = VSID::VSID_2;
    mVoiceSession.session[VSID2_VOICE_SESSION].state.current_ = CallState::IN_ACTIVE;
    mVoiceSession.session[VSID2_VOICE_SESSION].state.new_ = CallState::IN_ACTIVE;
    mTelecomConfig.voiceVolume = Float{TelecomConfig::VOICE_VOLUME_MAX};
    mTelecomConfig.ttyMode = TelecomConfig::TtyMode::OFF;
    mTelecomConfig.isHacEnabled = Boolean{false};
    // Todo check on default RX device
    mRxDevice = kDefaultRxDevice;
    mTxDevice = getMatchingTxDevice(mRxDevice);
#ifdef SEC_AUDIO_CALL
    mPlatform.mTelephonyDevices = {kDefaultRxDevice};
#endif
}

Telephony::~Telephony() {
    stopCall();
}

ndk::ScopedAStatus Telephony::getSupportedAudioModes(std::vector<AudioMode>* _aidl_return) {
    std::scoped_lock lock{mLock};

    *_aidl_return = mSupportedAudioModes;
    LOG(DEBUG) << __func__ << ": returning " << ::android::internal::ToString(*_aidl_return);
    return ndk::ScopedAStatus::ok();
}

void Telephony::VoiceStop() {
    for (int i = 0; i < MAX_VOICE_SESSIONS; i++) {
         mVoiceSession.session[i].CallUpdate.mCallState = CallState::IN_ACTIVE;
         mVoiceSession.session[i].state.new_ = CallState::IN_ACTIVE;
#ifdef SEC_AUDIO_CALL
         mPlatform.updateCallState((int)mVoiceSession.session[i].CallUpdate.mCallState);
#endif
    }
    mIsVoiceStarted = false;
    updateCalls();

    LOG(DEBUG) << __func__ << ": Exit";
}

ndk::ScopedAStatus Telephony::switchAudioMode(AudioMode newAudioMode) {
    std::scoped_lock lock{mLock};

    if (std::find(mSupportedAudioModes.begin(), mSupportedAudioModes.end(), newAudioMode) ==
        mSupportedAudioModes.end()) {
        LOG(ERROR) << __func__ << ": illegal mode " << toString(newAudioMode);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    mPlatform.updateCallMode((int)newAudioMode);

    if (mAudioMode == newAudioMode) {
        LOG(VERBOSE) << __func__ << ": no change" << toString(newAudioMode);
        return ndk::ScopedAStatus::ok();
    }
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    if ((newAudioMode != AudioMode::NORMAL) && mPlatform.isListenBackEnabled()) {
        SecModulePrimary::updateStreamListenbackMode(false);
    }
#endif
#ifdef SEC_AUDIO_CALL
    if (newAudioMode == AudioMode::IN_CALL)  {
        mSetUpdates.mVSID = mCurVSID;
        mSetUpdates.mCallState = CallState::ACTIVE;
#ifdef SEC_AUDIO_CALL_SATELLITE
        if (mPlatform.getSatelliteCall()) {
            mSetUpdates.mCallState = CallState::EXTMODEM_ACTIVE;
        }
#endif
        mLock.unlock();
        reconfigure(mSetUpdates);
        mLock.lock();
        LOG(DEBUG) << __func__ << ": new call state is ACTIVE for vsid " << mSetUpdates.mVSID;

#ifdef SEC_AUDIO_RECOVERY
        if (mSecFTM.getFactoryMode() != SecFTM::FACTORY_TEST_INACTIVE) {
            LOG(DEBUG) << __func__ << ": initialize factory state before starting call";
            stopCall();
            mSecFTM.initFTMConfig();
        }
#endif
#ifdef SEC_AUDIO_FMRADIO
        if (mPlatform.getFMRadioOn()) {
            LOG(DEBUG) << __func__ << " FM force stop for call";
            struct str_parms *fmParms = str_parms_create();
            str_parms_add_int(fmParms, kHandleFM.c_str(),
                                    (int)(AUDIO_DEVICE_NONE|AUDIO_DEVICE_OUT_SPEAKER));
            mAudExt.mFmExtension->audio_extn_fm_set_parameters(fmParms);
            str_parms_destroy(fmParms);
        }
#endif
    }
#ifdef SEC_AUDIO_CALL_VOIP
    auto streamOut = SecModulePrimary::GetStreamOut(Usecase::VOIP_PLAYBACK);
    if (mAudioMode == AudioMode::IN_COMMUNICATION) {
        if (streamOut && streamOut->HasPalStreamHandle() && mPlatform.getCngEnable())
            mAudioEffect.SetCNGForEchoRefMute(false);
#ifdef SEC_AUDIO_RECOVERY
        if (newAudioMode == AudioMode::NORMAL) {
            initSecConfig();
        }
#endif
    }

    if (newAudioMode == AudioMode::IN_COMMUNICATION) {
        if (streamOut) {
            if (streamOut->HasPalStreamHandle()) {
                if (mPlatform.getCngEnable()) {
                    mAudioEffect.SetCNGForEchoRefMute(true);
                }
            }
            /* duo mt call : voicestream > mode 3, hac custom key setting error */
            streamOut->ForceSetDevices(streamOut->getConnectedDevices());
            mAudioEffect.SetVoipMicModeEffect();
        }
    }
#endif
#ifdef SEC_AUDIO_DUAL_SPEAKER
    else if (newAudioMode == AudioMode::NORMAL) {
        mAudioEffect.SetEchoRefMuteStatus(false);
        mAudioEffect.SetUpperSpeakerMute(false);
    }
#endif
#endif

    if (newAudioMode == AudioMode::NORMAL) {
        // safe to stop now
        VoiceStop();
    } else if (newAudioMode == AudioMode::RINGTONE && mSetUpdates.mIsCrsCall) {
        if (!mIsCRSStarted && !isAnyCallActive()) {
            updateCrsDevice();
            startCall();
            if (mRxDevice.type.type != AudioDeviceType::OUT_SPEAKER) {
                startCrsLoopback();
            }
            mIsCRSStarted = true;
            mCRSVSID = mSetUpdates.mVSID;
            LOG(DEBUG) << __func__ << " start CRS call";
        }
    }

    mAudioMode = newAudioMode;

    LOG(DEBUG) << __func__ << ": switching to AudioMode:" << toString(mAudioMode);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Telephony::setTelecomConfig(const TelecomConfig& in_config,
                                               TelecomConfig* _aidl_return) {
    std::unique_lock lock{mLock};

    if (in_config.voiceVolume.has_value() &&
        (in_config.voiceVolume.value().value <
                 static_cast<float>(TelecomConfig::VOICE_VOLUME_MIN) ||
         in_config.voiceVolume.value().value >
                 static_cast<float>(TelecomConfig::VOICE_VOLUME_MAX))) {
        LOG(ERROR) << __func__
                   << ": voice volume value is invalid: " << in_config.voiceVolume.value().value;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_config.voiceVolume.has_value()) {
        mTelecomConfig.voiceVolume = in_config.voiceVolume;
        // safe to update when there is volume provided
#ifdef SEC_AUDIO_CALL
        if (mTelecomConfig.voiceVolume) {
            mPlatform.setVoiceVolume(mTelecomConfig.voiceVolume.value().value);
        }
#endif
        updateVoiceVolume();
#ifdef SEC_AUDIO_CALL_SATELLITE
        updateExtModemCallVolume();
#endif
    }
    if (in_config.ttyMode != TelecomConfig::TtyMode::UNSPECIFIED) {
        mTelecomConfig.ttyMode = in_config.ttyMode;
        // safe to update when there is ttymode is provided
        updateTtyMode();
    }
    if (in_config.isHacEnabled.has_value()) {
        mTelecomConfig.isHacEnabled = in_config.isHacEnabled;
#ifdef SEC_AUDIO_CALL_HAC
        bool newHacIncall = in_config.isHacEnabled.value().value;
        if (mPlatform.getHacIncall() != newHacIncall) {
            mPlatform.setHacIncall(newHacIncall);
            if (mPlatform.getCallMode() == AUDIO_MODE_IN_COMMUNICATION) {
                auto streamOut = SecModulePrimary::GetStreamOut(Usecase::VOIP_PLAYBACK);
                if (streamOut && streamOut->HasPalStreamHandle()) {
                    streamOut->ForceSetDevices(streamOut->getConnectedDevices());
                }
            } else {
                if (isAnyCallActive()) {
                    mLock.unlock();
                    setDevices(mPlatform.getTelephonyDevices(), true);
                    mLock.lock();
                }
            }
        }
#else //TEMP_FOR_SETUP_V QC patch in 240802
        mPlatform.setHACEnabled(mTelecomConfig.isHacEnabled.value().value);
        /**
         * TODO remove this unusual way with streams
         * remove the telephony lock before handling the streams.
         * unlocking the telephony is necessary because the stream already have telephony instance.
         */

        lock.unlock();
        triggerHACinVoipPlayback();
        lock.lock();
#endif
    }
    *_aidl_return = mTelecomConfig;
    LOG(DEBUG) << __func__ << ": received " << in_config.toString() << ", returning "
               << _aidl_return->toString();
    return ndk::ScopedAStatus::ok();
}

void Telephony::setMicMute(const bool muted) {
    std::scoped_lock lock{mLock};
#ifdef SEC_AUDIO_CALL_SATELLITE
    updateExtModemMicMute();
#endif
    if (mPalHandle == nullptr) {
        return;
    }

#ifdef SEC_AUDIO_CALL_FORWARDING
    if (!muted &&
        (mPlatform.getMicMuteStatus() ||
        (mPlatform.isCallForwarding() || mPlatform.getCallMemo() == CALLMEMO_REC))) {
        LOG(DEBUG) << __func__ << " skip voice tx unmute on "
                   << (mPlatform.getMicMuteStatus()? "tx mute":"force mute");
        return;
    }
#endif
#ifdef SEC_AUDIO_CALL
    mPlatform.setVoiceMuteState(PAL_TX, muted);
#endif

    if (!mPlatform.setStreamMicMute(mPalHandle, muted)) {
        LOG(ERROR) << __func__ << ": failed";
    }
}

bool Telephony::isCrsCallSupported() {
    std::scoped_lock lock{mLock};
    return true;
}

bool Telephony::isAnyCallActive() {
    for (int i = 0; i < MAX_VOICE_SESSIONS; i++) {
         if (mVoiceSession.session[i].state.current_ == CallState::ACTIVE) {
             return true;
         }
    }
    return false;
}

void Telephony::resetDevices(const bool resetRx) {
    std::scoped_lock lock{mLock};
    LOG(VERBOSE) <<__func__<<": ignore reset device ";
}

#ifdef SEC_AUDIO_LOOPBACK_TEST
void Telephony::updateLoopback(const std::vector<AudioDevice>& devices, bool loopbackon) {
    std::scoped_lock lock{mLock};
    LOG(DEBUG) << __func__ << " Enter";

    if (loopbackon) {
        stopCall();
        int type = mSecFTM.getLoopbackType();
        if (type == SecFTM::LOOPBACK_PACKET
            || type == SecFTM::LOOPBACK_PACKET_NODELAY) {
            pal_param_loopback_t pal_param_loopback;
            if (type == SecFTM::LOOPBACK_PACKET) {
                pal_param_loopback.loopback_mode = PAL_LOOPBACK_PACKET_DELAY_MODE;
            } else {
                pal_param_loopback.loopback_mode = PAL_LOOPBACK_PACKET_NO_DELAY_MODE;
            }
            pal_set_param(PAL_PARAM_ID_LOOPBACK_MODE,
                        (void*)&pal_param_loopback, sizeof(pal_param_loopback_t));
        }
        updateDevices();
        startCall();
    } else {
        stopCall();
    }

    LOG(DEBUG) << __func__ << " Exit";
}
#endif

void Telephony::setDevices(const std::vector<AudioDevice>& devices, const bool updateRx) {
    std::scoped_lock lock{mLock};

    if (devices.size() != 1) {
        LOG(ERROR) << __func__ << " invalid size / combo devices unsupported: " << devices;
        return;
    }

    LOG(DEBUG) << __func__ << (updateRx ? " Rx " : " Tx") << " devices : " << devices;
    if (updateRx) {
        mRxDevice = devices[0]; // expected to have 1 device.
        mTxDevice = getMatchingTxDevice(mRxDevice);
        updateDevices();
#ifdef SEC_AUDIO_CALL
        mPlatform.setTelephonyDevices({mRxDevice});
        if (!mSecFTM.getLoopbackMode()) {
            configureMicMode();
        }
#endif
#ifdef SEC_AUDIO_CALL_TRANSLATION
        if (mPlatform.getCallTranslation()) {
            mAudioEffect.ControlTxVolumeDown();
            mAudioEffect.SetVoiceRxEffectForTranslation(true);
        }
#endif
    } else {
        // mTxDevice = devices;
        // /* update the voice call devices only on TX devices update. Because Rx
        //  * devices patch is followed by Tx Devices patch */
        // updateDevices();
    }
}

void Telephony::updateVoiceMetadataForBT(bool call_active) {
    ssize_t track_count = 1;
    std::vector<playback_track_metadata_t> sourceTracks;
    std::vector<record_track_metadata_t> sinkTracks;
    sourceTracks.resize(track_count);
    sinkTracks.resize(track_count);
    int32_t ret = 0;

    source_metadata_t btSourceMetadata;
    sink_metadata_t btSinkMetadata;

    if (call_active) {
        btSourceMetadata.track_count = track_count;
        btSourceMetadata.tracks = sourceTracks.data();

        btSourceMetadata.tracks->usage = AUDIO_USAGE_VOICE_COMMUNICATION;
        btSourceMetadata.tracks->content_type = AUDIO_CONTENT_TYPE_SPEECH;

        LOG(DEBUG) << __func__
                   << "Source metadata for voice call usage: " << btSourceMetadata.tracks->usage
                   << "content_type: " << btSourceMetadata.tracks->content_type;
        // Pass the source metadata to PAL
        pal_set_param(PAL_PARAM_ID_SET_SOURCE_METADATA, (void*)&btSourceMetadata, 0);

        btSinkMetadata.track_count = track_count;
        btSinkMetadata.tracks = sinkTracks.data();

        btSinkMetadata.tracks->source = AUDIO_SOURCE_VOICE_CALL;

        LOG(DEBUG) << __func__
                   << "Sink metadata for voice call source: " << btSinkMetadata.tracks->source;
        // Pass the sink metadata to PAL
        pal_set_param(PAL_PARAM_ID_SET_SINK_METADATA, (void*)&btSinkMetadata, 0);
    } else {
        /* When voice call ends, we need to restore metadata configuration for
         * source and sink sessions same as prior to the call. Send source
         * and sink metadata separately to BT.
         */
        if (mStreamOutPrimary.lock()) {
            StreamOutPrimary::sourceMetadata_mutex_.lock();
            ret = mStreamOutPrimary.lock()->setAggregateSourceMetadata(false);
            if (ret != 0) {
                LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_SET_SOURCE_METADATA for" << ret
                           << " failed";
            }
            StreamOutPrimary::sourceMetadata_mutex_.unlock();
        }

        if (mStreamInPrimary.lock()) {
            StreamInPrimary::sinkMetadata_mutex_.lock();
            ret = mStreamInPrimary.lock()->setAggregateSinkMetadata(false);
            if (ret != 0) {
                LOG(ERROR) << __func__ << " Set PAL_PARAM_ID_SET_SINK_METADATA for" << ret
                           << " failed";
            }
            StreamInPrimary::sinkMetadata_mutex_.unlock();
        }
    }
}

void Telephony::onExternalDeviceConnectionChanged(const AudioDevice& extDevice,
                                                  const bool& connect) {
    std::scoped_lock lock{mLock};
    // Placeholder for telephony to act upon external device connection
    if (isBluetoothSCODevice(extDevice) || isBluetoothA2dpDevice(extDevice)) {
        LOG(VERBOSE) << __func__ << ": sco/a2dp no change";
        return;
    }
    if (isAnyCallActive() || mAudioMode == AudioMode::IN_CALL) {
        LOG(VERBOSE) << __func__ << ": voice call exist";
        return;
    }
    if (connect) {
        if (isOutputDevice(extDevice)) {
            mRxDevice = extDevice;
            mTxDevice = getMatchingTxDevice(mRxDevice);
            updateDevices();
        } else {
            if (mTxDevice.type.type != extDevice.type.type) {
                mTxDevice = getMatchingTxDevice(mRxDevice);
                updateDevices();
            }
        }
    } else {
        if (isOutputDevice(extDevice)) {
            mRxDevice = kDefaultRxDevice;
            mTxDevice = getMatchingTxDevice(mRxDevice);
            updateDevices();
        }
    }
}

void Telephony::onOutputPrimaryStreamDevices(const std::vector<AudioDevice>& primaryStreamDevices) {
    std::scoped_lock lock{mLock};

    /**
     * CRS ringtone routing piggybacks on output primary stream devices
     **/
#ifdef SEC_AUDIO_CALL
    if (!mIsCRSStarted && !mIsVoiceStarted) {
        return;
    }
#else
    if (!mIsCRSStarted) {
        return;
    }
#endif
    if (primaryStreamDevices.size() == 1) {// combo devices unsupported.
        mRxDevice = primaryStreamDevices[0]; // expected to have 1 device.
        mTxDevice = getMatchingTxDevice(mRxDevice);
        updateDevices();
#ifdef SEC_AUDIO_CALL
        mPlatform.setTelephonyDevices({mRxDevice});
        if (!mSecFTM.getLoopbackMode()) {
            configureMicMode();
        }
#endif
#ifdef SEC_AUDIO_CALL_TRANSLATION
        if (mPlatform.getCallTranslation()) {
            mAudioEffect.ControlTxVolumeDown();
            mAudioEffect.SetVoiceRxEffectForTranslation(true);
        }
#endif
     }
}

void Telephony::onBluetoothScoEvent(const bool& enable) {
    std::scoped_lock lock{mLock};

    if (isAnyCallActive() || mAudioMode == AudioMode::IN_CALL) {
        LOG(VERBOSE) << __func__ << ": voice call exist";
        return;
    }

   if (enable) {
       if (mIsCRSStarted) {
           mRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_DEVICE,
                                   .type.connection = AudioDeviceDescription::CONNECTION_BT_SCO};
           mTxDevice = getMatchingTxDevice(mRxDevice);
           updateDevices();
       }
   } else {
       if (isBluetoothSCODevice(mRxDevice) || isBluetoothA2dpDevice(mRxDevice)) {
           mRxDevice = kDefaultRxDevice;
           mTxDevice = getMatchingTxDevice(mRxDevice);
           updateDevices();
     }
  }
}

void Telephony::updateCrsDevice() {
    LOG(VERBOSE) << __func__ << ": Enter";
    if (hasValidPlaybackStream) {
        return;
    }

    if (mRxDevice.type.type == AudioDeviceType::OUT_SPEAKER_EARPIECE) {
        mRxDevice = kDefaultCRSRxDevice;
        mTxDevice = getMatchingTxDevice(mRxDevice);
        LOG(VERBOSE) << __func__ << " force to speaker for CRS call";
    }
}

AudioDevice Telephony::getMatchingTxDevice(const AudioDevice& rxDevice) {
    if (rxDevice.type.type == AudioDeviceType::OUT_SPEAKER_EARPIECE) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_SPEAKER) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_BACK};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEADSET &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_ANALOG) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_ANALOG,
                           .address = rxDevice.address};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEADPHONE &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_ANALOG) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_DEVICE &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_ANALOG) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else if ((rxDevice.type.type == AudioDeviceType::OUT_DEVICE ||
                rxDevice.type.type == AudioDeviceType::OUT_HEADSET) &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_BT_SCO) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_BT_SCO};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEADSET &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_BT_LE) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_BT_LE};
    } else if (rxDevice.type.type == AudioDeviceType::OUT_CARKIT &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_BT_SCO) {
        return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                           .type.connection = AudioDeviceDescription::CONNECTION_BT_SCO};
    } else if ((rxDevice.type.type == AudioDeviceType::OUT_DEVICE ||
                rxDevice.type.type == AudioDeviceType::OUT_HEADSET) &&
               rxDevice.type.connection == AudioDeviceDescription::CONNECTION_USB) {
        if (mPlatform.getUSBCapEnable()) {
            return AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                               .type.connection = AudioDeviceDescription::CONNECTION_USB,
                               .address = rxDevice.address};
        } else {
            return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
        }
    } else if (rxDevice.type.type == AudioDeviceType::OUT_HEARING_AID) {
        return AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
    } else {
        LOG(ERROR) << __func__ << ": unable to find matching TX device for " << rxDevice.toString();
    }
    return {};
}

void Telephony::reconfigure(const SetUpdates& newUpdates) {
    std::scoped_lock lock{mLock};
    auto palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});
    LOG(DEBUG) << __func__ << " : Enter : current setUpdates" << mSetUpdates.toString() << " new setUpdates"
                 << newUpdates.toString();
    // Todo Implement
    mPlatform.updateCallState((int)mSetUpdates.mCallState);

    if (newUpdates.mIsCrsCall) {
        mSetUpdates.mIsCrsCall = newUpdates.mIsCrsCall;
        mSetUpdates.mVSID = newUpdates.mVSID;
        if (!mIsCRSStarted && !isAnyCallActive() &&
            mAudioMode == AudioMode::RINGTONE) {
             updateCrsDevice();
             startCall();
             if (mRxDevice.type.type != AudioDeviceType::OUT_SPEAKER) {
                 startCrsLoopback();
             }
             mIsCRSStarted  = true;
             mCRSVSID = newUpdates.mVSID;
             LOG(DEBUG) << __func__ << ": start CRS call";
             return;
         }
    } else {
         if (mIsCRSStarted && mCRSVSID == newUpdates.mVSID) {
             stopCall();
             if (mPalCrsHandle != nullptr) {
                 stopCrsLoopback();
             }
             mSetUpdates.mIsCrsCall = newUpdates.mIsCrsCall;
             mIsCRSStarted  = false;
             LOG(DEBUG) << __func__ << ": stop CRS call";
         }
    }

    for (int i = 0; i < MAX_VOICE_SESSIONS; i++) {
         if (newUpdates.mVSID == mVoiceSession.session[i].CallUpdate.mVSID) {
             mVoiceSession.session[i].state.new_  = newUpdates.mCallState;
             mVoiceSession.session[i].CallUpdate = newUpdates;
             break;
         }
    }
    if (isAnyCallActive() ||
       ((mAudioMode == AudioMode::IN_CALL) && mIsVoiceStarted)) {
       updateCalls();
    }

    LOG(DEBUG) << __func__ << ": Exit";
}

void Telephony::updateCalls() {
    auto status = ndk::ScopedAStatus::ok();
     auto palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});
     for (int i = 0; i < MAX_VOICE_SESSIONS; i++) {
            switch (mVoiceSession.session[i].state.new_) {
                  case CallState::ACTIVE:
                      switch (mVoiceSession.session[i].state.current_) {
                            case CallState::IN_ACTIVE:
                                LOG(DEBUG) << __func__ << " CallState: INACTIVE -> ACTIVE vsid:" << mVoiceSession.session[i].CallUpdate.mVSID;
                                if ((palDevices[0].id == PAL_DEVICE_OUT_BLUETOOTH_BLE) &&
                                    (palDevices[1].id == PAL_DEVICE_IN_BLUETOOTH_BLE)) {
                                    updateVoiceMetadataForBT(true);
                                }
                                if (!isAnyCallActive() && !mIsCRSStarted) {
                                    mSetUpdates =  mVoiceSession.session[i].CallUpdate;
                                    status = startCall();
                                    if (!status.isOk()) {
                                        LOG(ERROR) << __func__ << ": start call failed";
                                    } else {
                                        mIsVoiceStarted = true;
                                        mVoiceSession.session[i].state.current_ = mVoiceSession.session[i].state.new_;
                                    }
                                } else {
                                    LOG(DEBUG) << __func__ << ": voice already started";
                                }
                                break;

                            default:
                                LOG(INFO) << __func__ << " CallState: ACTIVE cannot be handled in "
                                          << "state " << mVoiceSession.session[i].state.current_
                                          << " vsid " << mVoiceSession.session[i].CallUpdate.mVSID;
                                break;
                      }
                      break;
#ifdef SEC_AUDIO_CALL_SATELLITE
                  case CallState::EXTMODEM_ACTIVE:
                      switch (mVoiceSession.session[i].state.current_) {
                            case CallState::IN_ACTIVE:
                                LOG(DEBUG) << __func__ << " CallState: INACTIVE -> EXTMODEM_ACTIVE vsid:"
                                           << mVoiceSession.session[i].CallUpdate.mVSID;
                                if (!mAudExt.mExtModemCallExtension->isCallActive() && !mIsCRSStarted) {
                                    mSetUpdates =  mVoiceSession.session[i].CallUpdate;
                                    mAudExt.mExtModemCallExtension->startCall(
                                                    reinterpret_cast<pal_device*>(palDevices.data()));
                                    if (mPlatform.getMicMuteStatus()) {
                                        updateExtModemMicMute();
                                    }
                                    updateExtModemCallVolume();
                                    mIsVoiceStarted = true;
                                    mVoiceSession.session[i].state.current_ = mVoiceSession.session[i].state.new_;
                                } else {
                                    LOG(DEBUG) << __func__ << ": extmodem call already started";
                                }
                                break;

                            default:
                                LOG(INFO) << __func__ << " CallState: EXTMODEM_ACTIVE cannot be handled in "
                                          << "state " << mVoiceSession.session[i].state.current_
                                          << " vsid " << mVoiceSession.session[i].CallUpdate.mVSID;
                                break;
                      }
                      break;
#endif
                  case CallState::IN_ACTIVE:
                      switch (mVoiceSession.session[i].state.current_) {
                            case CallState::ACTIVE:
                                LOG(DEBUG) << __func__ << " CallState: ACTIVE -> INACTIVE vsid:" << mVoiceSession.session[i].CallUpdate.mVSID;
                                mSetUpdates =  mVoiceSession.session[i].CallUpdate;
                                status = stopCall();
                                if (!status.isOk()) {
                                    LOG(ERROR) << __func__ << ": stop call failed";
                                } else {
                                    mVoiceSession.session[i].state.current_ = mVoiceSession.session[i].state.new_;
                                }
                                break;
#ifdef SEC_AUDIO_CALL_SATELLITE
                            case CallState::EXTMODEM_ACTIVE:
                                LOG(DEBUG) << __func__ << " CallState: EXTMODEM_ACTIVE -> INACTIVE vsid:"
                                           << mVoiceSession.session[i].CallUpdate.mVSID;
                                mSetUpdates =  mVoiceSession.session[i].CallUpdate;
                                if (mPlatform.getMicMuteStatus()) {
                                    mAudioEffect.setExtModemMicMute(
                                        mAudExt.mExtModemCallExtension->getTxStreamHandle(), false);
                                }
                                mAudExt.mExtModemCallExtension->stopCall();
                                mVoiceSession.session[i].state.current_ = mVoiceSession.session[i].state.new_;
                                break;
#endif
                             default:
                                 LOG(INFO) << __func__ << " CallState: Default cannot be handled in "
                                           << "state " << mVoiceSession.session[i].state.current_
                                           << " vsid " << mVoiceSession.session[i].CallUpdate.mVSID;
                                break;
                      }
                      break;
                  default:
                      break;
            }
     }
}

void Telephony::updateVolumeBoost(const bool enable) {
    std::scoped_lock lock{mLock};
    mIsVolumeBoostEnabled = enable;
    LOG(INFO) << __func__ << ": is enabled: " << mIsVolumeBoostEnabled;
    configureVolumeBoost();
}

void Telephony::updateSlowTalk(const bool enable) {
    std::scoped_lock lock{mLock};
    mIsSlowTalkEnabled = enable;
    LOG(INFO) << __func__ << ": is enabled: " << mIsSlowTalkEnabled;
    configureSlowTalk();
}

void Telephony::updateHDVoice(const bool enable) {
    std::scoped_lock lock{mLock};
    mIsHDVoiceEnabled = enable;
    LOG(INFO) << __func__ << ": is enabled: " << mIsHDVoiceEnabled;
    configureHDVoice();
}

void Telephony::updateDeviceMute(const bool isMute, const std::string& muteDirection) {
    std::scoped_lock lock{mLock};
    mIsDeviceMuted = isMute;
    mMuteDirection = muteDirection;
    LOG(INFO) << __func__ << ": is muted: " << mIsDeviceMuted
              << ", mute direction: " << mMuteDirection;
    configureDeviceMute();
}

#ifdef SEC_AUDIO_CALL
void Telephony::updateSecCallState(const int state) {
    std::scoped_lock lock{mLock};
    LOG(INFO) << __func__ << ": CallState val: 0x" << std::hex << state;
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
    // 1st : check volte status
    if (state & CALL_STATUS_VOLTE) {
        bool prevState = volte_vt;
        mIsVolteVT = (state & CALL_STATUS_VOLTE_VIDEO) ? true : false;

        // volte vt <-> voice/off, rerouting for spk path
        if ((mSetUpdates.mCallState == CallState::ACTIVE)
                && (prevState != mIsVolteVT)
                && (mRxDevice.type.type == AudioDeviceType::OUT_SPEAKER)) {
            updateDevices();
        }
    }
#endif
}

void Telephony::updateSecVSID(const int vsid) {
    std::scoped_lock lock{mLock};
    Telephony::SetUpdates newUpdates{};

    if (vsid == SIM_FORCE_UPDATE) {
        // For emergency call
        if (mSetUpdates.mCallState == CallState::ACTIVE) {
            newUpdates.mVSID = (mSetUpdates.mVSID == VSID::VSID_1)?
                            VSID::VSID_2: VSID::VSID_1;
            LOG(DEBUG) << __func__ << ": change vsid to " << newUpdates.mVSID
                       << " by SIM_FORCE_UPDATE";
        }
    } else if (vsid == SIM1_VSID1) {
        newUpdates.mVSID = VSID::VSID_1;
    } else if (vsid == SIM2_VSID2) {
        newUpdates.mVSID = VSID::VSID_2;
    }

    if ((mSetUpdates.mCallState == CallState::ACTIVE && mPalHandle != nullptr)
            && (mPlatform.getCallMode() == AUDIO_MODE_IN_CALL)
            && !(vsid & SIM_FORCE_UPDATE)) {
        LOG(ERROR) << __func__ << ": skip to change vsid to " << newUpdates.mVSID
                   << " during call";
    } else {
        if (mSetUpdates.mVSID != newUpdates.mVSID) {
            if (mSetUpdates.mCallState == CallState::ACTIVE && mPalHandle != nullptr) {
                // SIM_FORCE_UPDATE
                mLock.unlock();
                mSetUpdates.mCallState = CallState::IN_ACTIVE;
                reconfigure(mSetUpdates);
                newUpdates.mCallState = CallState::ACTIVE;
                reconfigure(newUpdates);
                mLock.lock();
            }
            mCurVSID = newUpdates.mVSID;
        }
    }
}

void Telephony::updateSecCallBand(const int callBand) {
    std::scoped_lock lock{mLock};
    if (callBand == -1) {
        LOG(ERROR) << __func__ << ": invalid call band";
        return;
    }
    mCallBand = callBand;
#ifdef SEC_AUDIO_WB_AMR
    mPlatform.updateSecCallBand(mCallBand);
#endif
}
#endif

void Telephony::configureVolumeBoost() {
    if (mPalHandle == nullptr) {
        LOG(ERROR) << __func__ << ": invalid pal handle";
        return;
    }
    auto byteSize = sizeof(pal_param_payload) + sizeof(bool);
    auto bytes = std::make_unique<uint8_t[]>(byteSize);
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(bytes.get());
    palParamPayload->payload_size = sizeof(bool);
    palParamPayload->payload[0] = mIsVolumeBoostEnabled;
    if (int32_t ret =
                ::pal_stream_set_param(mPalHandle, PAL_PARAM_ID_VOLUME_BOOST, palParamPayload);
        ret) {
        LOG(ERROR) << __func__ << ": failed to set PAL_PARAM_ID_VOLUME_BOOST";
        return;
    }
}

void Telephony::configureSlowTalk() {
    if (mPalHandle == nullptr) {
        LOG(ERROR) << __func__ << ": invalid pal handle";
        return;
    }
    auto byteSize = sizeof(pal_param_payload) + sizeof(bool);
    auto bytes = std::make_unique<uint8_t[]>(byteSize);
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(bytes.get());
    palParamPayload->payload_size = sizeof(bool);
    palParamPayload->payload[0] = mIsSlowTalkEnabled;
    if (int32_t ret = ::pal_stream_set_param(mPalHandle, PAL_PARAM_ID_SLOW_TALK, palParamPayload);
        ret) {
        LOG(ERROR) << __func__ << ": failed to set PAL_PARAM_ID_SLOW_TALK";
        return;
    }
}

void Telephony::configureHDVoice() {
    if (mPalHandle == nullptr) {
        LOG(ERROR) << __func__ << ": invalid pal handle";
        return;
    }
    auto byteSize = sizeof(pal_param_payload) + sizeof(bool);
    auto bytes = std::make_unique<uint8_t[]>(byteSize);
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(bytes.get());
    palParamPayload->payload_size = sizeof(bool);
    palParamPayload->payload[0] = mIsHDVoiceEnabled;
    if (int32_t ret = ::pal_stream_set_param(mPalHandle, PAL_PARAM_ID_HD_VOICE, palParamPayload);
        ret) {
        LOG(ERROR) << __func__ << ": failed to set PAL_PARAM_ID_HD_VOICE";
        return;
    }
}

void Telephony::configureDeviceMute() {
    if (mPalHandle == nullptr) {
        LOG(ERROR) << __func__ << ": invalid pal handle";
        return;
    }

#ifdef SEC_AUDIO_CALL_FORWARDING
    if ((mMuteDirection == "rx") && !mIsDeviceMuted
        && (mPlatform.getAllSoundMute() || mPlatform.getEnforcePlaybackState()
            || (mPlatform.isCallForwarding() || mPlatform.getCallMemo() == CALLMEMO_REC)
#ifdef SEC_AUDIO_CALL_TRANSLATION
            || (mPlatform.getVoiceRxControl() == TRANSLATION_MUTE)
#endif
        )) {
        LOG(DEBUG) << __func__ << " skip voice rx unmute on "
                    << (mPlatform.getAllSoundMute() ? "rx mute":"force mute");
        return;
    }
#endif
#ifdef SEC_AUDIO_CALL
    if (mMuteDirection == "rx") {
        mPlatform.setVoiceMuteState(PAL_RX, mIsDeviceMuted);
    }
#endif

    auto byteSize = sizeof(pal_param_payload) + sizeof(pal_device_mute_t);
    auto bytes = std::make_unique<uint8_t[]>(byteSize);
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(bytes.get());
    palParamPayload->payload_size = sizeof(pal_device_mute_t);
    auto palDeviceMute = reinterpret_cast<pal_device_mute_t*>(palParamPayload->payload);
    palDeviceMute->mute = mIsDeviceMuted;
    if (mMuteDirection == "rx") {
        palDeviceMute->dir = PAL_AUDIO_OUTPUT;
    } else {
        palDeviceMute->dir = PAL_AUDIO_INPUT;
    }
    if (int32_t ret = ::pal_stream_set_param(mPalHandle, PAL_PARAM_ID_DEVICE_MUTE, palParamPayload);
        ret) {
        LOG(ERROR) << __func__ << ": failed to set PAL_PARAM_ID_DEVICE_MUTE";
        return;
    }
}

void Telephony::setVoipPlaybackStream(std::weak_ptr<StreamCommonInterface> voipStream) {
    std::scoped_lock lock{mLock};
    mVoipStreamWptr = voipStream;
}

void Telephony::triggerHACinVoipPlayback() {
    auto voipStream = mVoipStreamWptr.lock();
    if (!voipStream) {
        return;
    }
    const auto& voipConnectedDevices = voipStream->getConnectedDevices();
    if (hasOutputSpeakerEarpiece(voipConnectedDevices)) {
        LOG(INFO) << __func__ << ": HAC status changed for VOIP playback";
        voipStream->reconfigureConnectedDevices();
    }
}

void Telephony::onPlaybackStart() {
    std::scoped_lock lock{mLock};
    hasValidPlaybackStream = true;
    if (mIsCRSStarted) {
        LOG(INFO) << __func__ << ": playback conc status changed for CRS call";
        updateDevices();
    }
}

void Telephony::onPlaybackClose() {
    std::scoped_lock lock{mLock};
    hasValidPlaybackStream = false;
    if (mIsCRSStarted) {
        LOG(INFO) << __func__ << ": playback conc status changed for CRS call";
        updateDevices();
    }
}

#ifdef SEC_AUDIO_CALL
void Telephony::configureMicMode() {
    if (mPalHandle == nullptr) {
        LOG(ERROR) << __func__ << ": invalid pal handle";
        return;
    }

    bool needToSetStandard = false;
    auto palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});
    if (!(palDevices[0].id == PAL_DEVICE_OUT_HANDSET && palDevices[1].id == PAL_DEVICE_IN_HANDSET_MIC) &&
         !(palDevices[0].id == PAL_DEVICE_OUT_SPEAKER && palDevices[1].id == PAL_DEVICE_IN_SPEAKER_MIC)){
        LOG(INFO) << __func__ << " device that support standard mode";
        needToSetStandard = true;
    }

    int mode = mPlatform.getCallIsolationMode();
    if (mPlatform.getCallTranslation()) {
        mode = EFFECTS_TRANSLATION;
    } else if (mPlatform.getDexPadConnected() || needToSetStandard) {
        mode = EFFECTS_MICMODE_STANDARD;
    }

    auto byteSize = sizeof(pal_param_payload) + sizeof(bool);
    auto bytes = std::make_unique<uint8_t[]>(byteSize);
    auto palParamPayload = reinterpret_cast<pal_param_payload*>(bytes.get());
    palParamPayload->payload_size = sizeof(int);
    palParamPayload->payload[0] = getVoipMicMode.at(mode);
    if (int32_t ret = ::pal_stream_set_param(mPalHandle, PAL_PARAM_ID_MIC_MODE, palParamPayload);
        ret) {
        LOG(ERROR) << __func__ << ": failed to set PAL_PARAM_ID_MIC_MODE";
        return;
    }
}
#endif

#ifdef SEC_AUDIO_CALL_FORWARDING
void Telephony::setCallForwarding(bool enable) {
    if (mPalHandle == nullptr) {
        LOG(ERROR) << __func__ << ": invalid pal handle";
        return;
    }

    auto inCallMusicStreamOut = SecModulePrimary::GetStreamOut(Usecase::IN_CALL_MUSIC);
    auto voipStreamOut = SecModulePrimary::GetStreamOut(Usecase::VOIP_PLAYBACK);

    if (inCallMusicStreamOut && inCallMusicStreamOut->HasPalStreamHandle()) {
        inCallMusicStreamOut->forceShutdown();
    }

    if (voipStreamOut && voipStreamOut->HasPalStreamHandle()) {
        voipStreamOut->forceShutdown();
    }

    updateDeviceMute(enable, std::string("rx"));
    setMicMute(enable);
}
#endif

#ifdef SEC_AUDIO_CALL_SATELLITE
void Telephony::configureExtModemCall() {
    if ((mSetUpdates.mCallState == CallState::ACTIVE && mPalHandle != nullptr)
            && (mPlatform.getCallMode() == AUDIO_MODE_IN_CALL)
            && mPlatform.getSatelliteCall()) {
        Telephony::SetUpdates newUpdates{};
        mSetUpdates.mCallState = CallState::IN_ACTIVE;
        reconfigure(mSetUpdates);
        newUpdates.mCallState = CallState::EXTMODEM_ACTIVE;
        newUpdates.mVSID = mCurVSID;
        reconfigure(newUpdates);
    }
}
#endif

void Telephony::setCRSVolumeFromIndex(const int index) {
    std::scoped_lock lock{mLock};
    if (index <= MAX_CRS_VOL_INDEX && index >= MIN_CRS_VOL_INDEX)
        mCRSVolume = index / 10.0;
    else {
        mCRSVolume = 0.4;
        LOG(INFO) << __func__ << ": use defalut CRS volume: " << mCRSVolume;
    }
    updateVoiceVolume();
}

void Telephony::updateVoiceVolume() {
    if (mPalHandle == nullptr) {
        return;
    }
    float volumeFloat = 0.0f;
    if (mSetUpdates.mIsCrsCall) {
        volumeFloat = mCRSVolume;
    } else if (mPlatform.getTranslationRxMuteState()) {
        volumeFloat = 0.0f;
        LOG(INFO) << __func__ << ": set voice volume to mute.";
    } else {
        volumeFloat = mTelecomConfig.voiceVolume ? mTelecomConfig.voiceVolume.value().value : 1.0;
#ifdef SEC_AUDIO_CALL
        if ((mPlatform.getVoiceVolume() != -1.0f) &&
                (volumeFloat != mPlatform.getVoiceVolume())) {
            LOG(DEBUG) << __func__ << " : apply mCallVolume " << mPlatform.getVoiceVolume()
                                   << " instead of volumeFloat " << volumeFloat;
            volumeFloat = mPlatform.getVoiceVolume();
        }
#endif
    }

#ifdef SEC_AUDIO_SUPPORT_BT_RVC
    if (mAudioEffect.SetScoVolume(volumeFloat) == 0) {
        LOG(DEBUG) << __func__ << " : sco volume applied on voice session, skip pal_stream_set_volume()";
    } else
#endif
    if (int32_t ret = mPlatform.setVolume(mPalHandle, {volumeFloat}); ret) {
        LOG(ERROR) << __func__ << ": pal stream set volume failed !!" << ret;
        return;
    }
    LOG(DEBUG) << __func__ << ": updated voice volume value as " << volumeFloat;
}

#ifdef SEC_AUDIO_CALL_SATELLITE
void Telephony::updateExtModemCallVolume() {
    if (!mAudExt.mExtModemCallExtension->isCallActive()) {
        LOG(VERBOSE) << __func__ << " ext modem call is not active";
        return;
    }
    float volumeFloat = 0.0f;

    volumeFloat = mTelecomConfig.voiceVolume ? mTelecomConfig.voiceVolume.value().value : 1.0;
#ifdef SEC_AUDIO_CALL
    if ((mPlatform.getVoiceVolume() != -1.0f) &&
            (volumeFloat != mPlatform.getVoiceVolume())) {
        LOG(DEBUG) << __func__ << " : apply mCallVolume " << mPlatform.getVoiceVolume()
                                << " instead of volumeFloat " << volumeFloat;
        volumeFloat = mPlatform.getVoiceVolume();
    }
#endif
    mAudioEffect.setExtModemCallVolume(
        mAudExt.mExtModemCallExtension->getRxStreamHandle(), volumeFloat);
}

void Telephony::updateExtModemMicMute() {
    if (!mAudExt.mExtModemCallExtension->isCallActive()) {
        LOG(VERBOSE) << __func__ << " ext modem call is not active";
        return;
    }

    mAudioEffect.setExtModemMicMute(
        mAudExt.mExtModemCallExtension->getTxStreamHandle(), mPlatform.getMicMuteStatus());
}
#endif

void Telephony::updateTtyMode() {
    if (mPalHandle == nullptr) {
        return;
    }
    // Todo fix this bad size conversion
    const size_t ttyModeSizeInBytes = 4;
    auto bytes = std::make_unique<uint8_t[]>(sizeof(pal_param_payload) + ttyModeSizeInBytes);
    auto paramPtr = reinterpret_cast<pal_param_payload*>(bytes.get());
    if (int32_t ret = ::pal_stream_set_param(mPalHandle, PAL_PARAM_ID_TTY_MODE, paramPtr); ret) {
        LOG(ERROR) << __func__ << ": failed to set PAL_PARAM_ID_TTY_MODE";
        return;
    }
    LOG(VERBOSE) << __func__ << ": success";
    return;
}

ndk::ScopedAStatus Telephony::startCall() {
    LOG(DEBUG) << __func__ << ": Enter: "
               << " Rx: " << mRxDevice.toString() << " Tx: " << mTxDevice.toString();
    auto attributes = mPlatform.getDefaultTelephonyAttributes();

    auto palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});

    attributes->info.voice_call_info.VSID = static_cast<uint32_t>(mSetUpdates.mVSID);
    {
        const auto ttyMode = mTtyMap.find(mTelecomConfig.ttyMode);
        attributes->info.voice_call_info.tty_mode =
                ttyMode != mTtyMap.cend() ? ttyMode->second : PAL_TTY_OFF;
    }

    const size_t numDevices = 2;
    //set custom key for hac mode
    if (mTelecomConfig.isHacEnabled.has_value() && mTelecomConfig.isHacEnabled.value().value &&
        palDevices[0].id == PAL_DEVICE_OUT_HANDSET) {
        strlcpy(palDevices[0].custom_config.custom_key, "HAC",
                sizeof(palDevices[0].custom_config.custom_key));
        LOG(VERBOSE) << __func__ << "setting custom key as " << palDevices[0].custom_config.custom_key;
    }
    if (mSetUpdates.mIsCrsCall) {
        strlcpy(palDevices[0].custom_config.custom_key, "crsCall",
                sizeof(palDevices[0].custom_config.custom_key));
        LOG(VERBOSE) << __func__ << "setting custom key as " << palDevices[0].custom_config.custom_key;
    }

#ifdef SEC_AUDIO_CALL
    palDevices = mPlatform.configureSecPalDevicesForTelephony({mRxDevice, mTxDevice});
#endif
#ifdef SEC_AUDIO_LOOPBACK_TEST
    if (mSecFTM.getLoopbackMode()) {
        attributes->info.voice_call_info.VSID = static_cast<uint32_t>(VSID::VSID_LB_1);
        palDevices = mSecFTM.convertToPalDevices(
                        {mSecFTM.getRxFTMDevice(), mSecFTM.getTxFTMDevice()});
    }
#endif
#ifdef SEC_AUDIO_DUAL_SPEAKER
    if (mPlatform.isSpeakerLeftAmpOff()
            && (mSecFTM.getFactoryMode() & (SecFTM::FACTORY_LOOPBACK_ACTIVE|SecFTM::FACTORY_ROUTE_ACTIVE))) {
        mAudioEffect.SetUpperSpeakerMute(false/*unmute*/, true/*force*/);
    }
#endif

    if (int32_t ret = ::pal_stream_open(
                attributes.get(), numDevices, reinterpret_cast<pal_device*>(palDevices.data()), 0,
                nullptr, nullptr, reinterpret_cast<uint64_t>(this), &mPalHandle);
        ret) {
        LOG(ERROR) << __func__ << ": pal stream open failed !!" << ret;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
#ifdef SEC_AUDIO_CALL
    if (!mSecFTM.getLoopbackMode()) {
        configureMicMode();
    }
#endif
    if (int32_t ret = ::pal_stream_start(mPalHandle); ret) {
        LOG(ERROR) << __func__ << ": pal stream start failed !!" << ret;
        pal_stream_close(mPalHandle);
        mPalHandle = nullptr;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mPlatform.getMicMuteStatus()) {
        mPlatform.setStreamMicMute(mPalHandle, true);
    }
    updateVoiceVolume();
#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
    if (!mPlatform.getAllSoundMute() && mPlatform.getVoiceMuteState(PAL_RX) && !mPlatform.getEnforcePlaybackState()) {
        mIsDeviceMuted = false;
        mPlatform.setVoiceMuteState(PAL_RX, mIsDeviceMuted);
    }
#endif
    if (mIsDeviceMuted) {
        configureDeviceMute();
    }
    if (mSetUpdates.mIsCrsCall) {
        mPlatform.setStreamMicMute(mPalHandle, true);
        LOG(DEBUG) << __func__ << ": CRS usecase mute TX";
    }

#ifdef SEC_AUDIO_CALL_TRANSLATION
    if (mPlatform.getVoiceRxControl() == TRANSLATION_MUTE) {
        mLock.unlock();
        updateDeviceMute(mPlatform.getVoiceRxControl(), std::string("rx"));
        mLock.lock();
    }

    if (mPlatform.getCallTranslation()) {
        mAudioEffect.ControlTxVolumeDown();
        mAudioEffect.SetVoiceRxEffectForTranslation(true);
    }
#endif
#ifdef SEC_AUDIO_DYNAMIC_NREC
    // reset dsp aec mixer when entering cp call
    mAudioEffect.SetECNS(true);
#endif
#ifdef SEC_AUDIO_CALL
    if (mSecFTM.getLoopbackType() == SecFTM::LOOPBACK_OFF) {
        mAudioEffect.SetRingbackGain();
        mAudioEffect.SetNBQuality();
#ifdef SEC_AUDIO_ADAPT_SOUND
        mAudioEffect.SetDHAData(NULL, DHA_SET);
#endif
        if (mPlatform.getCngEnable()) {
            mAudioEffect.SetCNGForEchoRefMute(true);
        }
        mAudioEffect.SetDeviceInfo(palDevices[0].id);
    }
#endif

    LOG(DEBUG) << __func__ << ": Exit : Voice Stream";
    return ndk::ScopedAStatus::ok();
}

void Telephony::startCrsLoopback() {
    LOG(DEBUG) << __func__ << ": Enter";
    if (hasValidPlaybackStream) {
        LOG(VERBOSE) << __func__ << ": block loopback start";
        return;
    }
    auto attributes = mPlatform.getDefaultCRSTelephonyAttributes();
    std::vector<::aidl::android::media::audio::common::AudioDevice> RxDevices;
    RxDevices = {kDefaultCRSRxDevice};

    auto palDevices = mPlatform.convertToPalDevices({RxDevices});
    palDevices[0].id = PAL_DEVICE_OUT_SPEAKER;
    palDevices[0].config.sample_rate = Platform::kDefaultOutputSampleRate;
    palDevices[0].config.bit_width = Platform::kDefaultPCMBidWidth;
    palDevices[0].config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    attributes->info.voice_call_info.VSID = static_cast<uint32_t>(mSetUpdates.mVSID);
    if (mSetUpdates.mIsCrsCall) {
        strlcpy(palDevices[0].custom_config.custom_key, "crsCall",
                sizeof(palDevices[0].custom_config.custom_key));
        LOG(VERBOSE) << __func__ << " setting custom key as ", palDevices[0].custom_config.custom_key;
    } else {
        strlcpy(palDevices[0].custom_config.custom_key, "",
                sizeof(palDevices[0].custom_config.custom_key));
    }

    const size_t numDevices = 1;
    if (int32_t ret = ::pal_stream_open(
                attributes.get(), numDevices, reinterpret_cast<pal_device*>(palDevices.data()), 0,
                nullptr, nullptr, reinterpret_cast<uint64_t>(this), &mPalCrsHandle);
        ret) {
        LOG(ERROR) << __func__ << ": pal stream open failed !!" << ret;
        return;
    }
    if (int32_t ret = ::pal_stream_start(mPalCrsHandle); ret) {
        LOG(ERROR) << __func__ << ": pal stream open failed !!" << ret;
        return;
    }
    updateVoiceVolume();
    LOG(DEBUG) << __func__ << ": Exit";
}

ndk::ScopedAStatus Telephony::stopCall() {
    LOG(DEBUG) << __func__ << ": Enter";
    if (mPalHandle == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    int32_t ret = 0;

#ifdef SEC_AUDIO_ADAPT_SOUND
    if (mSecFTM.getLoopbackType() == SecFTM::LOOPBACK_OFF) {
        mAudioEffect.SetDHAData(NULL, DHA_RESET);
    }
#endif
#ifdef SEC_AUDIO_RECOVERY
    initSecConfig();
#endif
#ifdef SEC_AUDIO_CALL
    if (mPlatform.getCngEnable()) {
        mAudioEffect.SetCNGForEchoRefMute(false);
    }
#endif

    auto palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});
    if (mSetUpdates.mIsCrsCall) {
        strlcpy(palDevices[0].custom_config.custom_key, "",
                sizeof(palDevices[0].custom_config.custom_key));
        LOG(VERBOSE) << __func__ << "setting custom key as ", palDevices[0].custom_config.custom_key;
    }
    if (int32_t ret = pal_stream_stop(mPalHandle); ret) {
        LOG(ERROR) << __func__ << ": pal stream stop failed !!" << ret;
    }
    if (int32_t ret = pal_stream_close(mPalHandle); ret) {
        LOG(ERROR) << __func__ << ": pal stream stop failed !!" << ret;
    }
    if ((palDevices[0].id == PAL_DEVICE_OUT_BLUETOOTH_BLE) &&
        (palDevices[1].id == PAL_DEVICE_IN_BLUETOOTH_BLE)) {
        updateVoiceMetadataForBT(false);
    }
    mPalHandle = nullptr;
    if (mSetUpdates.mIsCrsCall &&
        mRxDevice.type.type == AudioDeviceType::OUT_SPEAKER) {
        mRxDevice = kDefaultRxDevice;
        mTxDevice = getMatchingTxDevice(mRxDevice);
    }
#ifdef SEC_AUDIO_CALL
    mPlatform.setDeviceInfo(VOICE_DEVICE_INVALID);
#endif
    if (ret) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    LOG(DEBUG) << __func__ << ": EXIT";
    return ndk::ScopedAStatus::ok();
}

void Telephony::stopCrsLoopback() {
    LOG(DEBUG) << __func__ << ": Enter";
    if (mPalCrsHandle == nullptr) {
        return;
    }
    std::vector<::aidl::android::media::audio::common::AudioDevice> RxDevices;
    RxDevices = {kDefaultCRSRxDevice};
    auto palDevices = mPlatform.convertToPalDevices({RxDevices});
    if (mSetUpdates.mIsCrsCall) {
        strlcpy(palDevices[0].custom_config.custom_key, "",
                sizeof(palDevices[0].custom_config.custom_key));
        LOG(VERBOSE) << __func__ << "setting custom key as ", palDevices[0].custom_config.custom_key;
    }
    ::pal_stream_stop(mPalCrsHandle);
    ::pal_stream_close(mPalCrsHandle);
    mPalCrsHandle = nullptr;
    LOG(DEBUG) << __func__ << ": EXIT";
}

void Telephony::updateDevices() {

    auto palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});

    pal_param_bta2dp_t* param_bt_a2dp_ptr = nullptr;
    bool a2dp_capture_suspended = false;
    size_t bt_param_size = 0;
    bool a2dp_suspended = false;
    int ret = 0;
    int retry_cnt = 20;
    const int retry_period_ms = 100;
    bool is_suspend_setparam = false;
    LOG(DEBUG) << __func__ << ": Enter";

    if (!isAnyCallActive()) {
#ifdef SEC_AUDIO_CALL_SATELLITE
        if (mSetUpdates.mCallState == CallState::EXTMODEM_ACTIVE) {
            LOG(DEBUG) << __func__ << " CallState: EXTMODEM_ACTIVE";
        } else
#endif
        if (mAudioMode == AudioMode::IN_CALL && mPalHandle == nullptr) {
#ifndef SEC_AUDIO_COMMON
            mIsVoiceStarted = true;
#endif
            updateCalls();
#ifdef SEC_AUDIO_CALL_FORWARDING /* || CscFeature_Audio_SupportAutoAnswer */
            if (!mPlatform.getEnforcePlaybackState()) {
                mLock.unlock();
                if (mPlatform.isCallForwarding()) {
                    // if fwd or call_memo param sent before call setup, enable here
                    setCallForwarding(true);
                } else if (mPlatform.getCallMemo() == CALLMEMO_REC) {
                    // if call_memo rec param sent before call setup, enable here
                    setMicMute(true);
                    updateDeviceMute(true, std::string("rx"));
                }
                mLock.lock();
            }
#endif
            return;
        }
    }
#ifdef SEC_AUDIO_CALL_SATELLITE
    if (!mAudExt.mExtModemCallExtension->isCallActive()) {
        if (mAudioMode == AudioMode::IN_CALL
                && (mSetUpdates.mCallState == CallState::EXTMODEM_ACTIVE)) {
            mIsVoiceStarted = true;
            updateCalls();
            return;
        }
    }
#endif
    // TODO configure pal devices with custom key if any
    if (mSetUpdates.mCallState == CallState::ACTIVE) {
        /*In case of active LEA profile, if voice call accepted by an inactive legacy headset
         * over SCO profile. APM is not aware about SCO active profile until BT_SCO=ON
         * event triggers from BT. In meantime before BT_SCO=ON, when LEA is suspended via
         * setparam call, APM tries to route voice call to BLE device.
         * In RouteStream call, if suspended state for LEA is true it keep checks over a
         * sleep of 2 secs. This causes timecheck issue in audioservice. Thus check for
         * is_suspend_setparam flag to know whether BLE suspended due to the actual setparam
         * or reconfig_cb(suspend->resume).
       */
        if ((palDevices[0].id == PAL_DEVICE_OUT_BLUETOOTH_BLE) &&
            (palDevices[1].id == PAL_DEVICE_IN_BLUETOOTH_BLE)) {
            updateVoiceMetadataForBT(true);
            pal_param_bta2dp_t param_bt_a2dp;
            do {
                std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
                param_bt_a2dp_ptr = &param_bt_a2dp;
                param_bt_a2dp_ptr->dev_id = PAL_DEVICE_OUT_BLUETOOTH_BLE;

                ret = pal_get_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED, (void**)&param_bt_a2dp_ptr,
                                    &bt_param_size, nullptr);
                if (!ret && bt_param_size && param_bt_a2dp_ptr) {
                    a2dp_suspended = param_bt_a2dp_ptr->a2dp_suspended;
                    is_suspend_setparam = param_bt_a2dp_ptr->is_suspend_setparam;
                } else {
                    LOG(ERROR) << __func__ << "getparam for PAL_PARAM_ID_BT_A2DP_SUSPENDED failed";
                }
                param_bt_a2dp_ptr = &param_bt_a2dp;
                param_bt_a2dp_ptr->dev_id = PAL_DEVICE_IN_BLUETOOTH_BLE;
                bt_param_size = 0;
                ret = pal_get_param(PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED,
                                    (void**)&param_bt_a2dp_ptr, &bt_param_size, nullptr);
                if (!ret && bt_param_size && param_bt_a2dp_ptr)
                    a2dp_capture_suspended = param_bt_a2dp_ptr->a2dp_capture_suspended;
                else
                    LOG(ERROR) << __func__ << "getparam for BT_A2DP_CAPTURE_SUSPENDED failed";
                param_bt_a2dp_ptr = nullptr;
                bt_param_size = 0;
            } while (!is_suspend_setparam && (a2dp_suspended || a2dp_capture_suspended) &&
                     retry_cnt-- && !usleep(retry_period_ms * 1000));
            LOG(INFO) << __func__ << "a2dp_suspended status: " << a2dp_suspended
                      << "and a2dp_capture_suspended status: " << a2dp_capture_suspended;
        }
    }

    //set or remove custom key for hac mode
    if (mTelecomConfig.isHacEnabled.has_value() && mTelecomConfig.isHacEnabled.value().value &&
        palDevices[0].id == PAL_DEVICE_OUT_HANDSET) {
        strlcpy(palDevices[0].custom_config.custom_key, "HAC",
                sizeof(palDevices[0].custom_config.custom_key));
        LOG(VERBOSE) << __func__ << "setting custom key as " << palDevices[0].custom_config.custom_key;
    } else {
        strlcpy(palDevices[0].custom_config.custom_key, "",
                sizeof(palDevices[0].custom_config.custom_key));
    }

    if (mSetUpdates.mIsCrsCall) {
        if (mPalCrsHandle != nullptr)
            stopCrsLoopback();
        updateCrsDevice();
        palDevices = mPlatform.convertToPalDevices({mRxDevice, mTxDevice});
        strlcpy(palDevices[0].custom_config.custom_key, "crsCall",
                  sizeof(palDevices[0].custom_config.custom_key));
    }
#ifdef SEC_AUDIO_CALL_SATELLITE
    if (mAudExt.mExtModemCallExtension->isCallActive()) {
        mAudExt.mExtModemCallExtension->setDevice(
                reinterpret_cast<pal_device*>(palDevices.data()));
        updateExtModemCallVolume();
    }
#endif

    if (mPalHandle == nullptr) return;

#ifdef SEC_AUDIO_SPK_AMP_MUTE
    if ((mPlatform.mTelephonyDevices[0].type.type == AudioDeviceType::OUT_SPEAKER)
            && (mRxDevice.type.type != AudioDeviceType::OUT_SPEAKER)) {
        mPlatform.setSpeakerMute(true);
    }
#endif
#ifdef SEC_AUDIO_CALL
    palDevices = mPlatform.configureSecPalDevicesForTelephony({mRxDevice, mTxDevice});
#endif
#ifdef SEC_AUDIO_LOOPBACK_TEST
    if (mSecFTM.getLoopbackMode()) {
        palDevices = mSecFTM.convertToPalDevices(
                        {mSecFTM.getRxFTMDevice(), mSecFTM.getTxFTMDevice()});
    }
#endif

#ifdef SEC_AUDIO_DSM_AMP
    if (int32_t ret = (int32_t)mAudExt.mSpeakerFeedbackExtension->setDevice(mPalHandle, 2,
                                        reinterpret_cast<pal_device*>(palDevices.data()));
        !ret) {
        LOG(INFO) << __func__ << ": set devices on stream for SpeakerFeedback";
    } else
#endif
    if (int32_t ret = ::pal_stream_set_device(mPalHandle, 2,
                                              reinterpret_cast<pal_device*>(palDevices.data()));
        ret) {
        LOG(ERROR) << __func__ << ": failed to set devices";
        return;
    }
#ifdef SEC_AUDIO_CALL
    mAudioEffect.SetDeviceInfo(palDevices[0].id);
#endif
    if (mSetUpdates.mIsCrsCall) {
        if (mRxDevice.type.type != AudioDeviceType::OUT_SPEAKER) {
            startCrsLoopback();
        }
    }
    updateVoiceVolume();
    if (mIsDeviceMuted) {
        configureDeviceMute();
    }
#ifdef SEC_AUDIO_SPK_AMP_MUTE
    mPlatform.setSpeakerMute(false);
#endif
    LOG(DEBUG) << __func__ << ": Exit : Rx: " << mRxDevice.toString() << " Tx: " << mTxDevice.toString();
}

#ifdef SEC_AUDIO_RECOVERY
void Telephony::initSecConfig() {
#ifdef SEC_AUDIO_CALL
    mPlatform.setRingbacktone(false);
#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
    mPlatform.updateEnforcePlaybackState(NOT_MUTE);
#endif
#ifdef SEC_AUDIO_CALL_FORWARDING
    mPlatform.setCallMemo(CALLMEMO_OFF);
    mPlatform.setCallForwarding(false);
#endif
#endif
    LOG(DEBUG) << __func__ << ": initialize sec voice state";
}
#endif

std::ostream& operator<<(std::ostream& os, const Telephony::CallState& state) {
    switch (state) {
        case Telephony::CallState::IN_ACTIVE:
            os << "IN_ACTIVE";
            break;
        case Telephony::CallState::ACTIVE:
            os << "ACTIVE";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Telephony::VSID& vsid) {
    switch (vsid) {
        case Telephony::VSID::VSID_1:
            os << "VSID_1";
            break;
        case Telephony::VSID::VSID_2:
            os << "VSID_2";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    return os;
}

} // namespace qti::audio::core
