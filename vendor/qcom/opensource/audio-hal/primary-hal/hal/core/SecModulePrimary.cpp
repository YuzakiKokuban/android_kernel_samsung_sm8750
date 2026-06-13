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

#define LOG_TAG "AHAL_SecModulePrimary"

#include <Utils.h>
#include <android-base/logging.h>
#include <android/binder_to_string.h>
#include <hardware/audio.h>
#include <qti-audio-core/StreamInPrimary.h>
#include <qti-audio-core/StreamOutPrimary.h>
#include <qti-audio-core/Telephony.h>
#include <qti-audio-core/Utils.h>
#include <system/audio.h>
#include <PalDefs.h>

#include <qti-audio-core/PlatformUtils.h>
#include <qti-audio/PlatformConverter.h>
#include <aidl/qti/audio/core/VString.h>

#include <qti-audio-core/ModulePrimary.h>
#include <qti-audio-core/SecModulePrimary.h>

#ifdef SEC_AUDIO_DUMP
#include "SecCoreUtils_Interface.h"
#endif

#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
#include <Calibration_Interface.h>
#endif

#ifdef SEC_AUDIO_SAMSUNGRECORD
#include <AudioPreProcess.h>
#endif

#ifdef SEC_AUDIO_AMP_BIGDATA
#include "audioInfo.h"
#endif

using aidl::android::hardware::audio::common::isValidAudioMode;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::Boolean;
using aidl::android::media::audio::common::Float;

using ::aidl::android::hardware::audio::core::VendorParameter;
using ::aidl::qti::audio::core::VString;

namespace qti::audio::core {

SecModulePrimary::SecModulePrimary() {
    LOG(INFO) << __func__  << ": SecModulePrimary()";
#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    mAudExt.mKarokeExtension->init();
#endif
#ifdef SEC_AUDIO_DSM_AMP
    mAudExt.mSpeakerFeedbackExtension->init();
#endif
}


SecModulePrimary::FeatureToSetHandlerMap SecModulePrimary::fillFeatureToSetHandlerMap() {
    FeatureToSetHandlerMap map{
            {SecParameters::Feature_SEC::SEC_GENERIC, &SecModulePrimary::onSetSECGenericParameters},
            {SecParameters::Feature_SEC::SEC_TELEPHONY, &SecModulePrimary::onSetSECTelephonyParameters},
            {SecParameters::Feature_SEC::SEC_FTM, &SecModulePrimary::onSetSECFTMParameters},
            {SecParameters::Feature_SEC::SEC_SUBKEY, &SecModulePrimary::onSetSECSubkeyParameters},
    };
    return map;
}


SecModulePrimary::SetParameterToFeatureMap SecModulePrimary::fillSetParameterToFeatureMap() {
    SetParameterToFeatureMap map{
// SEC_TELEPHONY
            {SecParameters::kVoiceCallBand, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallForwardingEnable, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallMemoState, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallNBQualityEnable, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallRingbacktoneState, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallSatelliteEnable, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallState, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallTranslationMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceEffectDVAdaptSound, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceEffectDVAdaptSoundCallPatam, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceFactoryEchoRefMuteCNGEnable, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceHACMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceMicInputControlMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceMicInputControlModeCall, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceRxControlMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceScreenCall, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceStreamEnforcedActiveInCall, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceTxControlMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceVSID, SecParameters::Feature_SEC::SEC_TELEPHONY},
// SEC_FTM
            {SecParameters::kFactoryTestLoopback, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestMicPath, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestPath, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestRoute, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestType, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestSpkPath, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalibration, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalAmpType, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalTarget, SecParameters::Feature_SEC::SEC_FTM},
// SEC_GENERIC
            {SecParameters::kAllSoundMuteEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kA2dpFormat, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kA2dpSuspendForBle, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kBtScoCodecType, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kDualSpeakerAmpLeftPowerEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kFMRadioMode, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kFMRadioVolume, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kFMRadioMute, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kGameChatEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwDisplayRotation, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwFlatMotionState, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwFolderState, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpTemperatureRCV, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpTemperatureSPK, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kInterpreterMode, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kPcmDumpApCallState, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kPcmDumpRecordState, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kPcmDumpState, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kScoRvcSupport, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kSupportSecAudioFeature, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kVoipViaSmartView, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRemoteMicEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRemoteMicVolume, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordBeamformingMode, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordSecVoiceRecorderEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordTxInversion, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kBargeinMode, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordNSRISecurityEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kVoiceWakeupRegisterVoiceKeyword, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kVoiceWakeupSeamlessEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kKaraokeEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kInputFlag, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kEffectOffloadVSPParam, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kEffectSoundBalanceValue, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kEffectToMonoEnable, SecParameters::Feature_SEC::SEC_GENERIC},
// SEC_SUBKEY
            {SecParameters::kDexKey, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyDexType, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyDexConnected, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kEffectListenBackKey, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyEffectListenBackState, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kMultiMicKey, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicAudioFocusEnable, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicCameraDirection, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicFocusCoordinate, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicMode, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicPhoneOrientation, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicSensitivityLevel, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicZoomLevel, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicZoomMax, SecParameters::Feature_SEC::SEC_SUBKEY},
            {SecParameters::kSubkeyMultiMicZoomMin, SecParameters::Feature_SEC::SEC_SUBKEY},
    };
    return map;
}

bool SecModulePrimary::processSetVendorParameters(const std::vector<VendorParameter>& parameters) {
    FeatureToVendorParametersMap pendingActions{};
    pendingActions.clear();
    for (const auto& p : parameters) {
        const auto searchId = mSetParameterToFeatureMap_SEC.find(p.id);
        if (searchId == mSetParameterToFeatureMap_SEC.cend()) {
            LOG(DEBUG) << __func__ << ": not configured " << p.id;
            continue;
        }
        LOG(DEBUG) << __func__ << ": configured " << p.id;

        auto itr = pendingActions.find(searchId->second);
        if (itr == pendingActions.cend()) {
            pendingActions[searchId->second] = std::vector<VendorParameter>({p});
            continue;
        }
        itr->second.push_back(p);
    }

    for (const auto & [ key, value ] : pendingActions) {
        const auto search = SecModulePrimary::mFeatureToSetHandlerMap.find(key);
        if (search == SecModulePrimary::mFeatureToSetHandlerMap.cend()) {
            LOG(DEBUG) << __func__
                       << ": no handler set on Feature:" << static_cast<int>(search->first);
            continue;
        }
        LOG(DEBUG) << __func__
                   << ": handler set on Feature:" << static_cast<int>(search->first);
        auto handler = std::bind(search->second, this, value);
        handler(); // a dynamic dispatch to a SetHandler
    }

    return true;
}

void SecModulePrimary::onSetSECGenericParameters(const std::vector<VendorParameter>& params) {
    for (const auto& param : params) {
        std::string paramValue{};
        int val = 0;
        if (!extractParameter<VString>(param, &paramValue)) {
            LOG(ERROR) << ": extraction failed for " << param.id;
            continue;
        }

#ifdef SEC_AUDIO_BLUETOOTH
        if (SecParameters::kBtScoCodecType == param.id) {
            /* bt_sco_codec_type parameters */
            pal_param_btsco_t param_bt_sco;
            memset(&param_bt_sco, 0, sizeof(pal_param_btsco_t));
            if (paramValue == AUDIO_PARAMETER_VALUE_RVP) {
                param_bt_sco.bt_sco_codec_type = PAL_BT_SCO_CODEC_TYPE_RVP;
            } else {
                param_bt_sco.bt_sco_codec_type = PAL_BT_SCO_CODEC_TYPE_NONE;
            }
            LOG(INFO) << __func__ << " BTSCO CODEC TYPE = " << param_bt_sco.bt_sco_codec_type;
            pal_set_param(PAL_PARAM_ID_BT_SCO_CODEC_TYPE, (void*)&param_bt_sco, sizeof(pal_param_btsco_t));
        }
#endif
#ifdef SEC_AUDIO_BT_OFFLOAD
        else if (SecParameters::kA2dpFormat == param.id) {
            mPlatform.setBtA2dpFormat(static_cast<audio_format_t>(getInt64FromString(paramValue)));
        }
#endif
#ifdef SEC_AUDIO_BLE_OFFLOAD
        else if (SecParameters::kA2dpSuspendForBle == param.id) {
            /* bt sco/a2dp and ble cannot active at the same time */
            pal_param_bta2dp_t param_bt_a2dp;
            memset(&param_bt_a2dp, 0, sizeof(pal_param_bta2dp_t));
            param_bt_a2dp.a2dp_suspended_for_ble = getBoolFromString(paramValue);
            if (param_bt_a2dp.a2dp_suspended_for_ble) {
                UpdateSCOdeviceState();
            }
            std::unique_lock<std::mutex> guard(AudioExtension::reconfig_wait_mutex_);
            pal_set_param(PAL_PARAM_ID_BT_A2DP_SUSPENDED_FOR_BLE, (void*)&param_bt_a2dp,
                                sizeof(pal_param_bta2dp_t));
        }
#endif
#ifdef SEC_AUDIO_SUPPORT_BT_RVC
        else if (SecParameters::kScoRvcSupport == param.id) {
            mPlatform.setBtRvcSupportState(getBoolFromString(paramValue));
        }
#endif
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
        else if (SecParameters::kRemoteMicEnable == param.id) {
            bool enableAas = getBoolFromString(paramValue);
            if (mPlatform.isAasEnabled() != enableAas) {
                pal_device_id_t deviceId = enableAas ? getPrimaryOutPalDeviceId() : PAL_DEVICE_NONE;
                mPlatform.setAasEnabled(enableAas);
                if (mAudExt.mAasExtension->updateAasStream(enableAas, deviceId) == 0) {
                    mAudioEffect.setAASVolume(mPlatform.getAasVolume());
                }
            }
        } else if (SecParameters::kRemoteMicVolume == param.id) {
            mPlatform.setAasVolume(getFloatFromString(paramValue));
            mAudioEffect.setAASVolume(mPlatform.getAasVolume());
        }
#endif
#ifdef SEC_AUDIO_BARGEIN_MODE
        else if (SecParameters::kBargeinMode == param.id) {
            pal_param_bargein_mode_t param_bargein_mode;
            memset(&param_bargein_mode, 0, sizeof(param_bargein_mode));

            param_bargein_mode.mode = getInt64FromString(paramValue);
            LOG(INFO) << __func__ << " Bargein mode = " << param_bargein_mode.mode;

            pal_set_param(PAL_PARAM_ID_BARGEIN_MODE, (void *)&param_bargein_mode,
                        sizeof(param_bargein_mode));
        }
#endif
#ifdef SEC_AUDIO_KARAOKE
        else if (SecParameters::kKaraokeEnable == param.id) {
            bool current = mPlatform.isKaraokeEnabled();
            mPlatform.setKaraokeEnabled(getBoolFromString(paramValue));
            if(mPlatform.isKaraokeEnabled() != current) {
                setKaraokeDevice();
            }
        }
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
        else if (SecParameters::kInterpreterMode == param.id) {
            pal_param_interpreter_mode_t param_interpreter_mode;
            memset(&param_interpreter_mode, 0, sizeof(param_interpreter_mode));
            mPlatform.setInterpreterMode(getInt64FromString(paramValue));
            param_interpreter_mode.mode = mPlatform.getInterpreterMode();
            LOG(INFO) << __func__ << " interpreter_mode mode = " << param_interpreter_mode.mode;
            pal_set_param(PAL_PARAM_ID_INTERPRETER_MODE, (void *)&param_interpreter_mode,
                        sizeof(param_interpreter_mode));
        }
#endif
#ifdef SEC_AUDIO_DUMP
        else if (SecParameters::kPcmDumpApCallState == param.id) {
            sec_ap_call_pcm_set_dumpstate(getBoolFromString(paramValue));
        } else if (SecParameters::kPcmDumpRecordState == param.id) {
            sec_rec_pcm_set_dumpstate(getInt64FromString(paramValue));
        } else if (SecParameters::kPcmDumpState == param.id) {
            sec_pcm_set_dumpstate(getInt64FromString(paramValue));
        }
#endif
#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
        else if (SecParameters::kVoipViaSmartView == param.id) {
            UpdateSmartViewState(getBoolFromString(paramValue));
        }
#endif // { SUPPORT_VOIP_VIA_SMART_VIEW
#ifdef SEC_AUDIO_ALL_SOUND_MUTE
        else if (SecParameters::kAllSoundMuteEnable == param.id) {
            LOG(INFO) << __func__ << "ALL Sound Mute = " << paramValue;
            bool muteVoice = getBoolFromString(paramValue);
            mPlatform.setAllSoundMute(muteVoice);
            mTelephony->updateDeviceMute(muteVoice, std::string("rx"));
#ifdef SEC_AUDIO_CALL_SATELLITE
            mTelephony->updateExtModemCallVolume();
#endif
#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
            mAudioEffect.setAASVolume(mPlatform.getAasVolume());
#endif
        }
#endif
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_MULTI_SPEAKER)
        else if (SecParameters::kHwDisplayRotation == param.id) {
            mPlatform.updateRotationInfo(static_cast<int32_t>(getInt64FromString(paramValue)));
        } else if (SecParameters::kHwFlatMotionState == param.id) {
            // 0 : Not flat, Uneven motion state, 1 : Flat motion state
            mPlatform.updateFlatmotionInfo(static_cast<int32_t>(getInt64FromString(paramValue)));
#ifdef SEC_AUDIO_SUPPORT_SOUNDBOOSTER_ON_DSP
            mAudioEffect.send_soundbooster_flatmotion();
#endif
        }
#endif
#ifdef SEC_AUDIO_OFFLOAD
        else if (SecParameters::kEffectSoundBalanceValue == param.id) {
            mAudioEffect.set_soundbalance_value(getInt64FromString(paramValue));
            auto streamOut = SecModulePrimary::GetStreamOut(Usecase::COMPRESS_OFFLOAD_PLAYBACK);
            if (streamOut) mAudioEffect.send_soundalive_lrsm_value();
        } else if (SecParameters::kEffectToMonoEnable == param.id) {
            mAudioEffect.set_mono_enable(getBoolFromString(paramValue));
            auto streamOut = SecModulePrimary::GetStreamOut(Usecase::COMPRESS_OFFLOAD_PLAYBACK);
            if (streamOut) mAudioEffect.send_soundalive_lrsm_value();
        }
#endif
#ifdef SEC_AUDIO_OFFLOAD_SOUNDSPEED
        else if (SecParameters::kEffectOffloadVSPParam == param.id) {
            mAudioEffect.set_soundspeed_value(getFloatFromString(paramValue));
            auto streamOut = SecModulePrimary::GetStreamOut(Usecase::COMPRESS_OFFLOAD_PLAYBACK);
            if (streamOut) mAudioEffect.send_soundspeed_value();
        }
#endif
#ifdef SEC_AUDIO_SAMSUNGRECORD
        else if (SecParameters::kRecordBeamformingMode == param.id) {
            LOG(INFO) << __func__ << "Beamforming mode = " << paramValue;
            if(getInt64FromString(paramValue) == 0 || getInt64FromString(paramValue) == 1) {
                mPlatform.preprocess_eq_enables |= S_REC_BF;
            } else {
                mPlatform.preprocess_eq_enables &= ~(S_REC_BF);
            }
        } else if (SecParameters::kRecordSecVoiceRecorderEnable == param.id) {
            if (strcmp(paramValue.c_str(), AUDIO_PARAMETER_VALUE_TRUE) == 0) {
                mPlatform.preprocess_eq_enables |= S_REC_SS_VOICERECORDER;
            } else {
                mPlatform.preprocess_eq_enables &= ~(S_REC_SS_VOICERECORDER);
            }
        } else if (SecParameters::kInputFlag == param.id) {
            LOG(INFO) << __func__ << "Received unconfigured input flag : " << paramValue;
            mPlatform.setUnconfiguredFlagsReceived(getInt64FromString(paramValue));
        }
#endif
#ifdef SEC_AUDIO_CAMCORDER
        else if (SecParameters::kRecordTxInversion == param.id) {
            mPlatform.setTxDataInversion(getBoolFromString(paramValue));
        }
#endif
#ifdef SEC_AUDIO_SUPPORT_NSRI
        else if (SecParameters::kRecordNSRISecurityEnable == param.id) {
            mPlatform.setNSRISecureEnabled(getBoolFromString(paramValue));
            LOG(DEBUG) << __func__ << " is_NSRI_secure state is changed to " << mPlatform.isNSRISecureEnabled();
        }
#endif
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
        else if (SecParameters::kVoiceWakeupRegisterVoiceKeyword == param.id) {
            mPlatform.setRegisterVoiceKeyword(getInt64FromString(paramValue));
        } else if (SecParameters::kVoiceWakeupSeamlessEnable == param.id) {
            mPlatform.setSeamlessEnabled(getBoolFromString(paramValue));
            LOG(DEBUG) << __func__ << " seamless enabled: " << mPlatform.isSeamlessEnabled();
        }
#endif
#ifdef SEC_AUDIO_FMRADIO
        else if (SecParameters::kFMRadioMode == param.id) {
            LOG(VERBOSE) << __func__ << " fmradio_mode = " << paramValue;
            mPlatform.setFMRadioOn(paramValue == AUDIO_PARAMETER_VALUE_ON);
            audio_devices_t fmRadioMode = mPlatform.getFMRadioOn() ? AUDIO_DEVICE_OUT_FM :
                                                                     AUDIO_DEVICE_NONE;
            audio_devices_t fmRadioDevice = mPlatform.getFMRadioDevice();
#ifdef SEC_AUDIO_PREVOLUME_SOUNDBOOSTER
            if (!mPlatform.getFMRadioOn()) {
                mAudioEffect.send_soundbooster_on(PARAM_VOLUME_FM, false);
            } else if (mPlatform.getFMRadioOn() && (fmRadioDevice == AUDIO_DEVICE_OUT_SPEAKER)) {
                mAudioEffect.send_soundbooster_on(PARAM_VOLUME_FM, true);
            }
#endif
            struct str_parms* fmParms = str_parms_create();
            str_parms_add_int(fmParms, kHandleFM.c_str(), (int)(fmRadioDevice | fmRadioMode));
            mAudExt.mFmExtension->audio_extn_fm_set_parameters(fmParms);
        } else if (SecParameters::kFMRadioVolume == param.id) {
            LOG(VERBOSE) << __func__ << " fmradio_volume = " << paramValue;
            float fmRadioVolume = getFloatFromString(paramValue);
            mPlatform.setFMRadioVolume(fmRadioVolume);
#if defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_ON_DSP) || defined(SEC_AUDIO_PREVOLUME_SOUNDBOOSTER)
            mAudioEffect.setSoundBoosterVolumeForFMRadio();
#else
            struct str_parms* fmParms = str_parms_create();
            str_parms_add_float(fmParms, kFMVolume.c_str(), fmRadioVolume);
            mAudExt.mFmExtension->audio_extn_fm_set_parameters(fmParms);
#endif
        } else if (SecParameters::kFMRadioMute == param.id) {
            LOG(VERBOSE) << __func__ << " fmradio_mute on/off state = " << paramValue;
            const bool fmRadioMute = getBoolFromString(paramValue);
            mPlatform.setFMRadioMute(fmRadioMute);
#if defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_ON_DSP) || defined(SEC_AUDIO_PREVOLUME_SOUNDBOOSTER)
            mAudioEffect.setSoundBoosterVolumeForFMRadio();
#else
            struct str_parms* fmParms = str_parms_create();
            str_parms_add_int(fmParms, kFMMute.c_str(), fmRadioMute ? 1 : 0);
            mAudExt.mFmExtension->audio_extn_fm_set_parameters(fmParms);
#endif
        }
#endif
#if defined(SEC_AUDIO_SUPPORT_FLIP_CALL) || defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP)
        else if (SecParameters::kHwFolderState == param.id) {
            SetFolderState(getInt64FromString(paramValue));
        }
#endif
#ifdef SEC_AUDIO_SUPPORT_GAMECHAT_SPK_AEC
        else if (SecParameters::kGameChatEnable == param.id) {
            bool newGamechatMode = getBoolFromString(paramValue);
            if (mPlatform.getGamechatMode() != newGamechatMode) {
                LOG(INFO) << __func__ << " gamechat mode is changed (" <<
                        makeParamValue(mPlatform.getGamechatMode()) << " -> " << paramValue << ")";
                mPlatform.setGamechatMode(newGamechatMode);
                // already ap call path enabled, need to re-route voip spk <-> game voip spk path
                RerouteForVoip();
            }
        }
#endif
#ifdef SEC_AUDIO_AMP_SDHMS
        else if (SecParameters::kHwSpeakerAmpTemperatureRCV == param.id||
                SecParameters::kHwSpeakerAmpTemperatureSPK == param.id) {
            uint32_t pal_param_id = 0;
            pal_param_amp_ssrm_t param_amp_ssrm = {};

            if (SecParameters::kHwSpeakerAmpTemperatureRCV == param.id) {
                pal_param_id = PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV;
            } else {
                pal_param_id = PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK;
            }
            param_amp_ssrm.temperature = getInt64FromString(paramValue);

            LOG(DEBUG) << __func__ << " set temperature for " << param.id << ": "
                        << param_amp_ssrm.temperature;
            pal_set_param(pal_param_id, (void*)&param_amp_ssrm, sizeof(pal_param_amp_ssrm_t));
        }
#endif
        else if (SecParameters::kSupportSecAudioFeature == param.id) {
            LOG(INFO) << __func__ << " kSupportSecAudioFeature = " << paramValue;
            mPlatform.setSecAudioFeatureSupported(true);
        }
    }
    return;
}

void SecModulePrimary::onSetSECTelephonyParameters(const std::vector<VendorParameter>& parameters) {
    if (!mTelephony) {
        LOG(ERROR) << __func__ << ": Telephony not created";
        return;
    }

    for (const auto& p : parameters) {
        std::string paramValue{};
        if (!extractParameter<VString>(p, &paramValue)) {
            LOG(ERROR) << ": extraction failed for " << p.id;
            continue;
        }
        if (SecParameters::kVoiceCallState == p.id) {
            int state = getInt64FromString(paramValue);
            bool prevVoWiFiState = mPlatform.getVoWiFiState();
            // 1st : check volte status
            mTelephony->updateSecCallState(state);
            // 2nd : check voipWificalling state
            if (state & CALL_STATUS_VOIPWIFICALLING) {
                mPlatform.updateVoWiFiState(
                    (state & CALL_STATUS_VOIPWIFICALLING_ON) ? true : false);
            }
            // 3rd : check voip aosp state
            if (state & CALL_STATUS_VOIP_AOSP_ON) {
                mPlatform.updateVoWiFiState(false); // reset vowifi for aosp voip
            }
            // if other AP call path is already actived before, set path again here
            if ((prevVoWiFiState != mPlatform.getVoWiFiState())
                    && (mPlatform.getCallMode() == AUDIO_MODE_IN_COMMUNICATION)) {
                LOG(INFO) << ": reroute voip for wifi call "
                          << mPlatform.getVoWiFiState() ? "ON" : "OFF";
                RerouteForVoip();
            }
        } else if (SecParameters::kVoiceVSID == p.id) {
            mTelephony->updateSecVSID(stoi(paramValue, nullptr, 16));
        } else if (SecParameters::kVoiceCallBand == p.id) {
            int callBand = -1;
            if (paramValue == AUDIO_PARAMETER_VALUE_SWB) callBand = SWB;
            if (paramValue == AUDIO_PARAMETER_VALUE_WB)  callBand = WB;
            if (paramValue == AUDIO_PARAMETER_VALUE_NB)  callBand = NB;
            mTelephony->updateSecCallBand(callBand);
        }
#ifdef SEC_AUDIO_CALL_SATELLITE
        else if (SecParameters::kVoiceCallSatelliteEnable == p.id) {
            LOG(DEBUG) << __func__ << " Satellite call = " << paramValue;
            mPlatform.setSatelliteCall(getBoolFromString(paramValue));
            mTelephony->configureExtModemCall();
        }
#endif
#ifdef SEC_AUDIO_CALL_HAC
        else if (SecParameters::kVoiceHACMode == p.id) {
            int newHacMode = getInt64FromString(paramValue);
            if (HAC_MODE_MIC <= newHacMode && newHacMode < HAC_MODE_MAX) {
                mPlatform.setHacMode(newHacMode);
            } else {
                LOG(ERROR) << __func__ << "invalid hac mode: " << newHacMode;
            }
        }
#endif
#ifdef SEC_AUDIO_ENFORCED_AUDIBLE
        else if (SecParameters::kVoiceStreamEnforcedActiveInCall == p.id) {
            bool mute = getBoolFromString(paramValue);
            mPlatform.updateEnforcePlaybackState(mute ? MUTE_CALL : NOT_MUTE);
            if (mPlatform.getCallMode() == AUDIO_MODE_IN_CALL) {
                mTelephony->setMicMute(mute);
                mTelephony->updateDeviceMute(mute, std::string("rx"));
            } else if (mute && mPlatform.getCallMode() != AUDIO_MODE_IN_COMMUNICATION) {
#ifdef SEC_AUDIO_SUPPORT_FAST_FLAG_SHUTTER
                Usecase TargetUsecase = Usecase::LOW_LATENCY_PLAYBACK;
#else
                Usecase TargetUsecase = Usecase::PRIMARY_PLAYBACK;
#endif
                auto streamOut = GetStreamOut(TargetUsecase);
                if (streamOut && (streamOut->isDeviceAvailable(PAL_DEVICE_OUT_USB_HEADSET)
                               || streamOut->isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_A2DP))) {
                    mPlatform.updateEnforcePlaybackState(MUTE_CALL_AND_REC);
                }
            }
        }
#endif
#ifdef SEC_AUDIO_ADAPT_SOUND
        else if (SecParameters::kVoiceEffectDVAdaptSound == p.id) {
            LOG(INFO) << __func__ << " set DHA : " << paramValue;
            mAudioEffect.SetDHAData(paramValue.c_str(), DHA_UPDATE);
        } else if (SecParameters::kVoiceEffectDVAdaptSoundCallPatam == p.id) {
            mAudioEffect.setSupportAdaptSoundCallParam(getBoolFromString(paramValue));
        }
#endif
#ifdef SEC_AUDIO_CALL
        else if (SecParameters::kVoiceCallNBQualityEnable == p.id) {
            bool enable = getBoolFromString(paramValue);
            if (mPlatform.getNbQuality() != enable) {
                mPlatform.setNbQuality(enable);
                mAudioEffect.SetNBQuality();
            }
        }
        else if (SecParameters::kVoiceCallRingbacktoneState == p.id) {
            bool enable = getBoolFromString(paramValue);
            if (mPlatform.getRingbacktone() != enable) {
                mPlatform.setRingbacktone(enable);
                mAudioEffect.SetRingbackGain();
            }
        }
#endif
#ifdef SEC_AUDIO_CALL_FORWARDING
        else if (SecParameters::kVoiceCallForwardingEnable == p.id) {
            bool enable = getBoolFromString(paramValue);
            if (mPlatform.getCallForwarding() != enable) {
                mPlatform.setCallForwarding(enable);
                mTelephony->setCallForwarding(enable);
            }
        }
        else if (SecParameters::kVoiceCallMemoState == p.id) {
            int callMemoState = CALLMEMO_OFF;
            if (getBoolFromString(paramValue)) {
                callMemoState = CALLMEMO_ON;  // use call music mixer/device
            } else if (paramValue == AUDIO_PARAMETER_VALUE_RECORDING) {
                callMemoState = CALLMEMO_REC;  // use call mixer/device, tx/rx mut
            }

            if (mPlatform.getCallMemo() != callMemoState) {
                int preCallMemo = mPlatform.getCallMemo();
                bool needRouting = true;
                if (mPlatform.getCallState() != 2/*CallState::ACTIVE*/) {
                    needRouting = false; // if not in call state, skip routing
                }
                LOG(INFO) << __func__ << " callmemo flag change 0x" << std::hex  << mPlatform.getCallMemo()
                            << " to 0x" << std::hex  << callMemoState << " needRouting " << needRouting;

                switch (callMemoState) {
                case CALLMEMO_ON :
                    mPlatform.setCallMemo(callMemoState);
                    if (needRouting) {
                        mTelephony->setCallForwarding(true);
                    }
                    break;
                case CALLMEMO_REC :
                    mPlatform.setCallMemo(CALLMEMO_OFF);  // to disable music path
                    if (needRouting) {
                        mTelephony->setCallForwarding(false);
                        mTelephony->updateDeviceMute(true, std::string("rx"));
                        mTelephony->setMicMute(true);
                    }
                    mPlatform.setCallMemo(callMemoState);
                    break;
                case CALLMEMO_OFF :
                default :
                    mPlatform.setCallMemo(callMemoState);
                    if (needRouting) {
                        mTelephony->setCallForwarding(false);
                    }
                    if (preCallMemo == CALLMEMO_REC) {
                        // if rec -> off case, need to unmute rx/tx
                        mTelephony->updateDeviceMute(false, std::string("rx"));
                        mTelephony->setMicMute(false);
                    }
                    break;
                }
            } else {
                LOG(INFO) << __func__ << " callmemo skip to set same flag on 0x" << std::hex  << mPlatform.getCallMemo();
            }
        }
#endif
#ifdef SEC_AUDIO_CALL_TRANSLATION
        else if (SecParameters::kVoiceCallTranslationMode == p.id) {
            LOG(DEBUG) << __func__ << " Enter CallTranslation " << paramValue;
            bool enable = getBoolFromString(paramValue);
            if (mPlatform.getCallTranslation() != enable) {
                mPlatform.setCallTranslation(enable);
                mTelephony->configureMicMode();
                // tx control for cp call
                mAudioEffect.ControlTxVolumeDown();
                // tx control for ap call
                mAudioEffect.SetVoipTxEffectForTranslation(enable);
                // rx control for ap/cp call
                mAudioEffect.SetVoiceRxEffectForTranslation(enable);
            }
        } else if (SecParameters::kVoiceTxControlMode == p.id) {
            int mode = getInt64FromString(paramValue);
            if (mPlatform.getVoiceTxControl() != mode) {
                mPlatform.setVoiceTxControl(mode);
            }
            mAudioEffect.ControlTxVolumeDown();
        } else if (SecParameters::kVoiceRxControlMode == p.id) {
            int mode = getInt64FromString(paramValue);
            if (mPlatform.getVoiceRxControl() != mode) {
                mPlatform.setVoiceRxControl((mode < 0) ? 0 : mode);
                mTelephony->updateDeviceMute(mPlatform.getVoiceRxControl(), std::string("rx"));
                LOG(INFO) << __func__ << " mVoiceRxControl " << paramValue;
            }
        }
#endif
#ifdef SEC_AUDIO_CALL_VOIP
        else if (SecParameters::kVoiceFactoryEchoRefMuteCNGEnable == p.id) {
            bool enable = getBoolFromString(paramValue);
            if (mPlatform.getVoipIsolationMode() != enable) {
                mPlatform.setCngEnable(enable);
                mAudioEffect.SetCNGForEchoRefMute(enable);
            }
        } else if (SecParameters::kVoiceMicInputControlMode == p.id) {
            uint32_t mode = getInt64FromString(paramValue);
            if (mPlatform.getVoipIsolationMode() != mode) {
                mPlatform.setVoipIsolationMode(mode);
                LOG(INFO) << __func__ << " set voip_isolation_mode as " << mode;
                mAudioEffect.SetVoipMicModeEffect();
            }
        } else if (SecParameters::kVoiceMicInputControlModeCall == p.id) {
            uint32_t mode = getInt64FromString(paramValue);
            if (mPlatform.getCallIsolationMode() != mode) {
                mPlatform.setCallIsolationMode(mode);
                LOG(INFO) << __func__ << " set call_isolation_mode as " << mode;
                mTelephony->configureMicMode();
                mAudioEffect.SetVoipMicModeEffect();
            }
        }
#endif
#ifdef SEC_AUDIO_VOICE_TX_FOR_INCALL_MUSIC
        else if (SecParameters::kVoiceScreenCall == p.id) {
            LOG(DEBUG) << __func__ << " Enter new_screen_call " << paramValue;
            mPlatform.setScreenCall(getBoolFromString(paramValue));
        }
#endif
    }
    return;
}

#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
void SecModulePrimary::processCalibrationParameters(const std::vector<VendorParameter>& parameters) {
    // processing in calibraion helper
    struct str_parms* parms = NULL;
    std::string kvpairs = getkvPairsForVendorParameter(parameters);
    if (!kvpairs.empty()) {
        parms = str_parms_create_str(kvpairs.c_str());
        LOG(DEBUG) << __func__ << " kvpairs : " << kvpairs;
        sec_cal_process(NULL, parms);
        if (parms)
            str_parms_destroy(parms);
    }
}

std::string SecModulePrimary::getCalibrationResults(std::string calValue, std::string ampType) {
    // processing in calibraion helper
    std::string calResult{AUDIO_PARAMETER_VALUE_FAIL};
    struct str_parms* parms = NULL;
    std::string ampInfo{""};
    if (calValue == AUDIO_PARAMETER_VALUE_READ) {
        ampInfo = "amp=" + ampType;
    }
    std::string kvpairs = SecParameters::kFactoryTestCalibration+ "=" + calValue + ";" + ampInfo;
    if (!kvpairs.empty()) {
        parms = str_parms_create_str(kvpairs.c_str());
        LOG(DEBUG) << __func__ << " kvpairs : " << kvpairs;
        calResult = std::string(sec_cal_get_result(NULL, parms));
        if (parms)
            str_parms_destroy(parms);
    }
    return calResult;
}
#endif

void SecModulePrimary::onSetSECFTMParameters(const std::vector<VendorParameter>& parameters) {
    if (!mTelephony) {
        LOG(ERROR) << __func__ << ": Telephony not created";
        return;
    }

    AudioDevice factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
    AudioDevice factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::NONE};
    int ftmode = mSecFTM.getFTMConfig().mode;
    int loopbacktype = SecFTM::LOOPBACK_OFF;
    bool loopbackOn = false;
    std::string calCommand{""};
    std::string ampType{""};

    for (const auto& param : parameters) {
        std::string paramValue{};
        if (!extractParameter<VString>(param, &paramValue)) {
            LOG(ERROR) << ": extraction failed for " << param.id;
            continue;
        }

        if (SecParameters::kFactoryTestLoopback == param.id) {//set Loopback on/off
            if (paramValue == AUDIO_PARAMETER_VALUE_ON) {
                loopbackOn = true;
            }
            mSecFTM.setLoopbackMode(loopbackOn ? true : false);
            if ((loopbackOn && (ftmode & SecFTM::FACTORY_LOOPBACK_ACTIVE))
                || (!loopbackOn && !(ftmode & SecFTM::FACTORY_LOOPBACK_ACTIVE))) {
                LOG(DEBUG) << __func__ << "FACTORY_TEST_LOOPBACK already set as "
                            << (loopbackOn ? " on" : "off");
            } else if (loopbackOn) {
                ftmode |= SecFTM::FACTORY_LOOPBACK_ACTIVE;
                mSecFTM.setFactoryMode(ftmode);
            } else {
                bool isIncallMusicLoopbackOff =
                    (mSecFTM.getIncallmusicLoopbackType()
                        != SecFTM::INCALLMUSIC_LOOPBACK_OFF) ? true : false;
                if (isIncallMusicLoopbackOff) {
                    mTelephony->setMicMute(false);
                }
                loopbackOn = false;
                ftmode &= ~SecFTM::FACTORY_LOOPBACK_ACTIVE;
                mSecFTM.setFactoryMode(ftmode);
                mSecFTM.setLoopbackType(SecFTM::LOOPBACK_OFF);
                mSecFTM.setIncallmusicLoopbackType(SecFTM::INCALLMUSIC_LOOPBACK_OFF);
                mSecFTM.setRcvAsSpk2(false);

                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::NONE};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::NONE};
                mSecFTM.setDevices({factoryRxDevice, factoryTxDevice}, SecFTM::FTM_INOUT_BOTH);
                if (isIncallMusicLoopbackOff) {
                    ForceSetOutDevices({factoryRxDevice}, true);
                    mTelephony->updateLoopback({factoryRxDevice, factoryTxDevice}, false);
                } else {
                    mTelephony->updateLoopback({factoryRxDevice, factoryTxDevice}, false);
                    if (mSecFTM.getLoopbackScoOn()) {
                        mSecFTM.secFTMhandleBTScoConnection(false);
                    }
                    ForceSetOutDevices({factoryRxDevice});
                }
                ForceSetInDevices({factoryTxDevice});
            }
        } else if (SecParameters::kFactoryTestType == param.id) { //set Loopback type
            if (paramValue == "packet") {
                loopbacktype = SecFTM::LOOPBACK_PACKET;
            } else if (paramValue == "packet_nodelay") {
                loopbacktype = SecFTM::LOOPBACK_PACKET_NODELAY;
            } else if (paramValue == "codec") {
                loopbacktype = SecFTM::LOOPBACK_CODEC;
            } else if (paramValue == "realtime") {
                loopbacktype = SecFTM::LOOPBACK_REALTIME;
            } else if (paramValue == "pcm") {
                loopbacktype = SecFTM::LOOPBACK_PCM;
            }
            if (loopbacktype != SecFTM::LOOPBACK_OFF) {
                mSecFTM.setLoopbackType(loopbacktype);
            }
        } else if (SecParameters::kFactoryTestPath == param.id) {
            mSecFTM.setRcvAsSpk2(false);
            if (paramValue == "ear_ear") {
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_HEADSET,
                                              .type.connection = AudioDeviceDescription::CONNECTION_USB};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                                              .type.connection = AudioDeviceDescription::CONNECTION_USB};
                // set dummpy usb address for factory test
                factoryRxDevice.address = AudioDeviceAddress::make<AudioDeviceAddress::Tag::alsa>(std::vector<int32_t>{1, 0});
                factoryTxDevice.address = AudioDeviceAddress::make<AudioDeviceAddress::Tag::alsa>(std::vector<int32_t>{1, 0});
            } else if (paramValue == "mic_rcv") {
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER_EARPIECE};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
            } else if (paramValue == "mic1_spk") {
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
            } else if (paramValue == "mic1_spk2") {
                // to checking rcv as spk2 on dualspk support models
                mSecFTM.setRcvAsSpk2(true);
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
            } else if (paramValue == "mic2_spk") {
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_BACK};
            } else if (paramValue == "mic3_spk") {
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_MIC3
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI,
                        .type.connection = AudioDeviceDescription::CONNECTION_BUILTIN_MIC3};
            } else if (paramValue == "dualmic_spk") { //need to check
            } else if (paramValue == "dualmic_rcv") {
            } else if (paramValue == "mic_ear") {
            } else if (paramValue == "mic2_ear") {
            } else if (paramValue == "dualmic_ear") {
            } else if (paramValue == "incallmusic_rcv") {
                mSecFTM.setIncallmusicLoopbackType(SecFTM::INCALLMUSIC_LOOPBACK_RCV);
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER_EARPIECE};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE};
            } else if (paramValue == "incallmusic_spk") {
                mSecFTM.setIncallmusicLoopbackType(SecFTM::INCALLMUSIC_LOOPBACK_SPK);
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_BACK};
            } else if (paramValue == "incallmusic_all_spk") {
                mSecFTM.setIncallmusicLoopbackType(SecFTM::INCALLMUSIC_LOOPBACK_SPK_ALL);
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_BACK};
            } else if (paramValue == "bt_bt") {
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_DEVICE,
                                              .type.connection = AudioDeviceDescription::CONNECTION_BT_SCO};
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::IN_HEADSET,
                                              .type.connection = AudioDeviceDescription::CONNECTION_BT_SCO};
                mSecFTM.secFTMhandleBTScoConnection(true);
            }
            mSecFTM.setDevices({factoryRxDevice, factoryTxDevice}, SecFTM::FTM_INOUT_BOTH);
            if (loopbacktype != SecFTM::LOOPBACK_REALTIME && !mSecFTM.getLoopbackScoOn()) {
                if (mSecFTM.getIncallmusicLoopbackType() != SecFTM::INCALLMUSIC_LOOPBACK_OFF) {
                    //close graph before opening incallmusic path
                    ForceSetOutDevices({factoryRxDevice}, true);
                    mTelephony->updateLoopback({factoryRxDevice, factoryTxDevice}, true);
                    //mute input from mic
                    mTelephony->setMicMute(true);
                } else {
                    mTelephony->updateLoopback({factoryRxDevice, factoryTxDevice}, true);
                }
            } else {
                ForceSetOutDevices({factoryRxDevice});
                ForceSetInDevices({factoryTxDevice});
            }
        } else if (SecParameters::kFactoryTestRoute == param.id) {
            if (paramValue == "rcv") {
                ftmode |= SecFTM::FACTORY_ROUTE_ACTIVE;
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER_EARPIECE};
                mSecFTM.setDevices({factoryRxDevice}, SecFTM::FTM_OUTPUT);
            } else if (paramValue == "spk") {
                ftmode |= SecFTM::FACTORY_ROUTE_ACTIVE;
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER};
                mSecFTM.setDevices({factoryRxDevice}, SecFTM::FTM_OUTPUT);
            } else if (paramValue ==  "hdmi") {
                ftmode |= SecFTM::FACTORY_ROUTE_ACTIVE;
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::OUT_DEVICE,
                                              .type.connection = AudioDeviceDescription::CONNECTION_HDMI};
                mSecFTM.setDevices({factoryRxDevice}, SecFTM::FTM_OUTPUT);
            } else if (paramValue == "off") {
                ftmode &= ~SecFTM::FACTORY_ROUTE_ACTIVE;
#if SEC_AUDIO_MULTI_SPEAKER == 4
                if (mSecFTM.isSpeakerFactoryRxDevice()
                        && !(ftmode & SecFTM::FACTORY_LOOPBACK_ACTIVE)) {
                    mSecFTM.setFactorySoundBoosterMode(SecFTM::SB_ON_DEFAULT);
                    mAudioEffect.send_factory_soundbooster_mode(mSecFTM.getFactorySoundBoosterMode());
                }
#endif
                factoryRxDevice = AudioDevice{.type.type = AudioDeviceType::NONE};
                mSecFTM.setDevices({factoryRxDevice}, SecFTM::FTM_OUTPUT);
            }
            mSecFTM.setFactoryMode(ftmode);
            if ((ftmode & SecFTM::FACTORY_LOOPBACK_ACTIVE) == 0) {
                ForceSetOutDevices({factoryRxDevice});
            }
        } else if (SecParameters::kFactoryTestMicPath == param.id) {
            ftmode &= ~SecFTM::FACTORY_ATCMD_ACTIVE;
            if (paramValue == "on") {
                ftmode |= SecFTM::FACTORY_IN_ROUTE_ACTIVE;
                mSecFTM.setRMSTestMode(true);
            } else if (paramValue == "all_mic") {
                if (mSecFTM.getIncallmusicLoopbackType() == SecFTM::INCALLMUSIC_LOOPBACK_RCV) {
                    factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_BUILTIN_MIC
                            .type.type = AudioDeviceType::IN_MICROPHONE};
                } else if (mSecFTM.getIncallmusicLoopbackType() == SecFTM::INCALLMUSIC_LOOPBACK_SPK
                        || mSecFTM.getIncallmusicLoopbackType() == SecFTM::INCALLMUSIC_LOOPBACK_SPK_ALL) {
                    factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_BACK_MIC
                            .type.type = AudioDeviceType::IN_MICROPHONE_BACK};
                } else {
                    factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_MULTI_MIC
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI,
                        .type.connection = AudioDeviceDescription::CONNECTION_BUILTIN_MULTI_MIC};
                }
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue == "mic3_mic4") {
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_MIC3_MIC4
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI,
                        .type.connection = AudioDeviceDescription::CONNECTION_BUILTIN_MIC3_MIC4};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue ==  "mic3") {
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_MIC3
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI,
                        .type.connection = AudioDeviceDescription::CONNECTION_BUILTIN_MIC3};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue ==  "mic4") {
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_MIC4
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI,
                        .type.connection = AudioDeviceDescription::CONNECTION_BUILTIN_MIC4};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue ==  "2mic") {
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_2MIC
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue ==  "main") {
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_BUILTIN_MIC
                        .type.type = AudioDeviceType::IN_MICROPHONE};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue ==  "sub") {
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_BACK_MIC
                        .type.type = AudioDeviceType::IN_MICROPHONE_BACK};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
            } else if (paramValue == "spk_mic1") {    /*at + looptest=0,3,4 */
                factoryTxDevice = AudioDevice{ // AUDIO_DEVICE_IN_2MIC
                        .type.type = AudioDeviceType::IN_MICROPHONE_MULTI};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
                ftmode |= SecFTM::FACTORY_ATCMD_ACTIVE;
            } else if (paramValue ==  "off") {
                ftmode &= ~SecFTM::FACTORY_IN_ROUTE_ACTIVE;
                ftmode &= ~SecFTM::FACTORY_ATCMD_ACTIVE;
                factoryTxDevice = AudioDevice{.type.type = AudioDeviceType::NONE};
                mSecFTM.setDevices({factoryTxDevice}, SecFTM::FTM_INPUT);
                mSecFTM.setRMSTestMode(false);
            }
            mSecFTM.setFactoryMode(ftmode);
            if (ftmode & SecFTM::FACTORY_IN_ROUTE_ACTIVE) {
                ForceSetInDevices({factoryTxDevice});
            }
        }
#if SEC_AUDIO_MULTI_SPEAKER == 4
        else if (SecParameters::kFactoryTestSpkPath == param.id) {
            if (paramValue == "spk1") {
                mSecFTM.setFactorySoundBoosterMode(SecFTM::SB_ON_TOP_LEFT);
            } else if (paramValue == "spk2") {
                mSecFTM.setFactorySoundBoosterMode(SecFTM::SB_ON_TOP_RIGHT);
            } else if (paramValue == "spk3") {
                mSecFTM.setFactorySoundBoosterMode(SecFTM::SB_ON_BOTTOM_RIGHT);
            } else if (paramValue == "spk4") {
                mSecFTM.setFactorySoundBoosterMode(SecFTM::SB_ON_BOTTOM_LEFT);
            } else if (paramValue == "spk") {
                mSecFTM.setFactorySoundBoosterMode(SecFTM::SB_ON_DEFAULT);
            }
            mAudioEffect.send_factory_soundbooster_mode(mSecFTM.getFactorySoundBoosterMode());
        }
#endif
        else if (SecParameters::kFactoryTestCalibration == param.id) {
            calCommand = paramValue;
        } else if (SecParameters::kFactoryTestCalAmpType == param.id) {
            ampType = paramValue;
        }
    }

#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
    if ((calCommand == AUDIO_PARAMETER_VALUE_ON)
            || (calCommand == AUDIO_PARAMETER_VALUE_PASS)
            || (calCommand == AUDIO_PARAMETER_VALUE_FAIL)) {
        // processing in calibraion helper
        processCalibrationParameters(parameters);
        if ((calCommand == AUDIO_PARAMETER_VALUE_ON)
                && (ampType != AUDIO_PARAMETER_VALUE_CIRRUS)) {
            // send trigger calibration parameter to pal
            LOG(DEBUG) << __func__ << " triggerSpeakerCalibration";
            mPlatform.triggerSpeakerCalibration();
        }
    }
#endif
    return;
}

void SecModulePrimary::onSetSECSubkeyParameters(const std::vector<VendorParameter>& parameters) {
    int ret = 0, val = 0;
    char value[32];
    // because of subkey, all parameter sets should be processed at once without dividing.
    std::string kvpairs = getkvPairsForVendorParameter(parameters);
    struct str_parms *parms = str_parms_create_str(kvpairs.c_str());
    if (!parms)
       return;

    LOG(DEBUG) << __func__ << " Enter: " << kvpairs.c_str();

#ifdef SEC_AUDIO_CALL
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_SEC_LOCAL_DEX_KEY, value, sizeof(value));
    if (ret >= 0) {
        ret = str_parms_get_str(parms, AUDIO_PARAMETER_SUBKEY_DEX_TYPE, value, sizeof(value));
        if (ret >= 0) {
            if (!strcmp(value, AUDIO_PARAMETER_VALUE_DEX_STATION)) {
                ret = str_parms_get_str(parms, AUDIO_PARAMETER_SUBKEY_DEX_CONNECTED, value, sizeof(value));
                if (ret >= 0) {
                    bool is_connected = (strcmp(value, AUDIO_PARAMETER_VALUE_TRUE)) ? false : true;
                    if (mPlatform.getDexConnected() != is_connected) {
                        LOG(INFO) << __func__ << " DEXconnected changed from "
                                  << makeParamValue(mPlatform.getDexConnected()) << " to "
                                  << makeParamValue(is_connected);
                        mPlatform.setDexConnected(is_connected);

                        if (mPlatform.getCallState() == 2/*CallState::ACTIVE*/) {
                            mTelephony->setDevices(mPlatform.getTelephonyDevices(), true);
                        }
                        RerouteForVoip();
                    }
                    str_parms_del(parms, AUDIO_PARAMETER_SUBKEY_DEX_CONNECTED);
                }
            } else if (!strcmp(value, AUDIO_PARAMETER_VALUE_DEX_PAD)) {
                ret = str_parms_get_str(parms, AUDIO_PARAMETER_SUBKEY_DEX_CONNECTED, value, sizeof(value));
                if (ret >= 0) {
                    bool is_connected = (strcmp(value, AUDIO_PARAMETER_VALUE_TRUE)) ? false : true;
                    if (mPlatform.getDexPadConnected() != is_connected) {
                        LOG(INFO) << __func__ << " DEXPADconnected changed from "
                                  << makeParamValue(mPlatform.getDexPadConnected()) << " to "
                                  << makeParamValue(is_connected);
                        mPlatform.setDexPadConnected(is_connected);
#ifdef SEC_AUDIO_SUPPORT_CALL_MICMODE
                        if ((mPlatform.getCallMode() == AUDIO_MODE_IN_CALL)
                                && (mPlatform.getCallIsolationMode()
                                        == EFFECTS_MICMODE_VOICE_FOCUS)) {
                            if (mPlatform.getCallState() == 2/*CallState::ACTIVE*/) {
                                mTelephony->configureMicMode();
                            }
                        }
#endif
#ifdef SEC_AUDIO_CALL_VOIP
                        if (mPlatform.getCallMode() == AUDIO_MODE_IN_COMMUNICATION) {
                            mAudioEffect.SetVoipMicModeEffect();
                        }
#endif
                    }
                    str_parms_del(parms, AUDIO_PARAMETER_SUBKEY_DEX_CONNECTED);
                }
            }
            str_parms_del(parms, AUDIO_PARAMETER_SUBKEY_DEX_TYPE);
        }
        str_parms_del(parms, AUDIO_PARAMETER_SEC_LOCAL_DEX_KEY);
    }
#endif

#ifdef SEC_AUDIO_SAMSUNGRECORD
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_SEC_LOCAL_MULTI_MIC_KEY, value, sizeof(value));
    if (ret >= 0) {
        PreProcessSetParams(parms);
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_PROVIDEO
        ret = str_parms_get_int(parms, AUDIO_PARAMETER_SUBKEY_MULTI_MIC_MODE, &val);
        if (ret >= 0) {
            SetProVideoState(val);
            str_parms_del(parms, AUDIO_PARAMETER_SUBKEY_MULTI_MIC_MODE);
        }
#endif
        str_parms_del(parms, AUDIO_PARAMETER_SEC_LOCAL_MULTI_MIC_KEY);
    }
#endif

#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_SEC_LOCAL_EFFECT_LISTENBACK_KEY, value, sizeof(value));
    if (ret >= 0) {
        ret = str_parms_get_int(parms, AUDIO_PARAMETER_SUBKEY_EFFECT_LISTENBACK_STATE, &val);
        if (ret >= 0) {
            bool old_state = mPlatform.isListenBackEnabled();
            bool listenback_on = (val == 0) ? false : true;
            mPlatform.setListenBackEnabled(listenback_on);
            if (old_state != listenback_on) {
                updateStreamListenbackMode(listenback_on);
            }
            str_parms_del(parms, AUDIO_PARAMETER_SUBKEY_EFFECT_LISTENBACK_STATE);
        }
    }
#endif
    str_parms_destroy(parms);
}

SecModulePrimary::GetParameterToFeatureMap SecModulePrimary::fillGetParameterToFeatureMap() {
    GetParameterToFeatureMap map{
            {SecParameters::kAllSoundMuteEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kDualSpeakerAmpLeftPowerEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kGameChatEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpBigData, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpBigDataSupport, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpMaxTemperature, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpTemperatureRCV, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kHwSpeakerAmpTemperatureSPK, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kSetupTestcase, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kEffectSoundBoosterDspSupport, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kInterpreterMode, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordInputLatency, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordTxInversion, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kBargeinMode, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordNSRISecurityEnable, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kRecordConversationEnergyKey, SecParameters::Feature_SEC::SEC_GENERIC},
            {SecParameters::kVoiceFactoryEchoRefStatus, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalStatus, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalOff, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalRead, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryTestCalAmpTI, SecParameters::Feature_SEC::SEC_FTM},
            {SecParameters::kFactoryEchoRefMuteDetect, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceCallForwardingEnable, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceHAC, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceHACMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceMicInputControlMode, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceMicInputControlModeCall, SecParameters::Feature_SEC::SEC_TELEPHONY},
            {SecParameters::kVoiceScreenCall, SecParameters::Feature_SEC::SEC_TELEPHONY}
    };
    return map;
}

// static
SecModulePrimary::FeatureToGetHandlerMap SecModulePrimary::fillFeatureToGetHandlerMap() {
        FeatureToGetHandlerMap map{
            {SecParameters::Feature_SEC::SEC_GENERIC, &SecModulePrimary::onGetSECGenericParameters},
            {SecParameters::Feature_SEC::SEC_TELEPHONY, &SecModulePrimary::onGetSECTelephonyParameters},
            {SecParameters::Feature_SEC::SEC_FTM, &SecModulePrimary::onGetSECFTMParameters}
    };
    return map;
}

#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
SecModulePrimary::CalValueToGetParameterMap SecModulePrimary::fillCalValueToGetParameterMap() {
    CalValueToGetParameterMap map{{AUDIO_PARAMETER_VALUE_STATUS, SecParameters::kFactoryTestCalStatus},
                                 {AUDIO_PARAMETER_VALUE_OFF, SecParameters::kFactoryTestCalOff},
                                 {AUDIO_PARAMETER_VALUE_READ, SecParameters::kFactoryTestCalRead}};
    return map;
}
#endif

std::vector<VendorParameter> SecModulePrimary::processGetVendorParameters(
        const std::vector<std::string>& ids) {
    FeatureToStringMap pendingActions{};
    // only group of features are mapped to Feature, rest are kept as generic.
    // If the key is found in feature map, use the feature otherwise call GENERIC feature.
    for (const auto& id : ids) {
        auto search = mGetParameterToFeatureMap.find(id);
        SecParameters::Feature_SEC mappedFeature = SecParameters::Feature_SEC::SEC_GENERIC;
        if (search != mGetParameterToFeatureMap.cend()) {
            mappedFeature = search->second;
        }
        auto itr = pendingActions.find(mappedFeature);
        if (itr == pendingActions.cend()) {
            pendingActions[mappedFeature] = std::vector<std::string>({id});
            continue;
        }
        itr->second.push_back(id);
    }

    std::vector<VendorParameter> result{};
    for (const auto & [ key, value ] : pendingActions) {
        const auto search = mFeatureToGetHandlerMap.find(key);
        if (search == mFeatureToGetHandlerMap.cend()) {
            LOG(ERROR) << __func__
                       << ": no handler set on Feature:" << static_cast<int>(search->first);
            continue;
        }
        auto handler = std::bind(search->second, this, value);
        auto keyResult = handler(); // a dynamic dispatch to GetHandler
        result.insert(result.end(), keyResult.begin(), keyResult.end());
    }
    return result;
}

std::vector<VendorParameter> SecModulePrimary::onGetSECGenericParameters(
        const std::vector<std::string>& ids) {
    std::vector<VendorParameter> results{};
    for (const auto& id : ids) {
        VendorParameter param;
        VString parcel;
#ifdef SEC_AUDIO_ALL_SOUND_MUTE
        if (id == SecParameters::kAllSoundMuteEnable) {
            param.id = SecParameters::kAllSoundMuteEnable;
            parcel.value = makeParamValue(mPlatform.getAllSoundMute());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_SETUP_TC
        else if (id == SecParameters::kSetupTestcase) {
            param.id = SecParameters::kHwInterfaceTestcase;
            /* AudioReach - hw interface name defined at SystemAudio */
            parcel.value = AUDIO_PARAMETER_VALUE_TRUE;
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#if defined(SEC_AUDIO_DUAL_SPEAKER) && !defined(SEC_AUDIO_FACTORY_TEST_MODE)
        else if (id == SecParameters::kDualSpeakerAmpLeftPowerEnable) {
            param.id = SecParameters::kDualSpeakerAmpLeftPowerEnable;
            parcel.value = makeParamValue(mPlatform.isSpeakerLeftAmpOff());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_SUPPORT_GAMECHAT_SPK_AEC
        else if (id == SecParameters::kGameChatEnable) {
            param.id = SecParameters::kGameChatEnable;
            parcel.value = makeParamValue(mPlatform.getGamechatMode());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
        else if (id == SecParameters::kInterpreterMode) {
            param.id = SecParameters::kInterpreterMode;
            parcel.value = std::to_string(mPlatform.getInterpreterMode());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_CAMCORDER
        else if (id == SecParameters::kRecordTxInversion) {
            param.id = SecParameters::kRecordTxInversion;
            parcel.value = makeParamValue(mPlatform.isTxDataInversionEnabled());
	        setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_SUPPORT_NSRI
        else if (id == SecParameters::kRecordNSRISecurityEnable) {
            param.id = SecParameters::kRecordNSRISecurityEnable;
            parcel.value = makeParamValue(mPlatform.isNSRISecureEnabled());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_SAMSUNGRECORD
        else if (id == SecParameters::kRecordInputLatency) {
            param.id = SecParameters::kRecordInputLatency;
            auto streamIn = getHighestPriorityStreamIn();
            parcel.value = "0";
            if (streamIn) {
                parcel.value = std::to_string(AudioPreProcess::GetLatency(streamIn->getContext().getMixPortConfig(), streamIn->getConnectedDevices()));
            }
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
// { SEC_AUDIO_AMP_BIGDATA
        else if (id == SecParameters::kHwSpeakerAmpBigDataSupport) {
            param.id = SecParameters::kHwSpeakerAmpBigDataSupport;
#ifdef SEC_AUDIO_AMP_BIGDATA
            parcel.value = AUDIO_PARAMETER_VALUE_TRUE;
#else
            parcel.value = AUDIO_PARAMETER_VALUE_FALSE;
#endif
            setParameter(parcel, param);
            results.push_back(param);
        }
// } SEC_AUDIO_AMP_BIGDATA
#ifdef SEC_AUDIO_AMP_BIGDATA
        else if (id == SecParameters::kHwSpeakerAmpMaxTemperature) {
            pal_param_amp_bigdata_t *param_amp_bigdata;
            size_t size = 0;
            if (int32_t ret =
                        ::pal_get_param(PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE,
                                        (void **)&param_amp_bigdata, &size, nullptr);
                (ret || size <= 0)) {
                LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE failed, ret:" << ret
                        << ", data size:" << size;
            }

            if (param_amp_bigdata != NULL) {
                param.id = SecParameters::kHwSpeakerAmpMaxTemperature;
                std::string kvpairs =
                    ";rcv=" + std::to_string(param_amp_bigdata->value[KEEP_MAX_TEMP_L])
                    + ";spk=" + std::to_string(param_amp_bigdata->value[KEEP_MAX_TEMP_R]);
                parcel.value = kvpairs;
                setParameter(parcel, param);
                results.push_back(param);
            }
        }
        else if (id == SecParameters::kHwSpeakerAmpBigData) {
            pal_param_amp_bigdata_t *param_amp_bigdata;
            size_t size = 0;
            if (int32_t ret =
                        ::pal_get_param(PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET,
                                        (void **)&param_amp_bigdata, &size, nullptr);
                (ret || size <= 0)) {
                LOG(ERROR) << __func__ << ": PAL_PARAM_ID_SPEAKER_AMP_BIGDATA_SAVE_RESET failed, ret:" << ret
                        << ", data size:" << size;
            }

            if (param_amp_bigdata != NULL) {
                param.id = SecParameters::kHwSpeakerAmpBigData;
                std::string kvpairs = ";";
                std::string logging_item;
                for (int i = android::AUDIO_LOGGING_HW_AMP_INDEX_START; i < android::AUDIO_LOGGING_ITEM_MAX; i++) {
                    logging_item = android::logging_item[i];
                    kvpairs += logging_item + "="
                        + std::to_string(param_amp_bigdata->value[i-android::AUDIO_LOGGING_HW_AMP_INDEX_START]) + ";";
                }
                parcel.value = kvpairs;
                setParameter(parcel, param);
                results.push_back(param);
            }
        }
#endif
#ifdef SEC_AUDIO_AMP_SDHMS
        else if (id == SecParameters::kHwSpeakerAmpTemperatureRCV ||
                id == SecParameters::kHwSpeakerAmpTemperatureSPK) {
            pal_param_amp_ssrm_t *param_amp_ssrm = NULL;
            size_t size = 0;
            uint32_t pal_param_id = 0;
            if (id == SecParameters::kHwSpeakerAmpTemperatureRCV) {
                pal_param_id = PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_RCV;
            } else {
                pal_param_id = PAL_PARAM_ID_SPEAKER_AMP_TEMPERATURE_SPK;
            }

            if (int32_t ret =
                        ::pal_get_param(pal_param_id, (void **)&param_amp_ssrm, &size, nullptr);
                (ret || size <= 0)) {
                LOG(ERROR) << __func__ << " pal_param_id:" << pal_param_id <<" failed, ret:" << ret
                        << ", data size:" << size;
            }
            if (param_amp_ssrm != NULL) {
                param.id = id;
                if (param_amp_ssrm->temperature == AMP_OFF_VALUE) {
                    parcel.value = AUDIO_PARAMETER_VALUE_AMP_OFF;
                } else {
                    LOG(DEBUG) << __func__ << " get temperature for " << id << ": "
                                << param_amp_ssrm->temperature;
                    parcel.value = std::to_string(param_amp_ssrm->temperature);
                }
                setParameter(parcel, param);
                results.push_back(param);
            }
        }
#endif
        else if (id == SecParameters::kEffectSoundBoosterDspSupport) {
            param.id = SecParameters::kEffectSoundBoosterDspSupport;
#ifdef SEC_AUDIO_SUPPORT_SOUNDBOOSTER_ON_DSP
            parcel.value = AUDIO_PARAMETER_VALUE_TRUE;
#else
            parcel.value = AUDIO_PARAMETER_VALUE_FALSE;
#endif
            setParameter(parcel, param);
            results.push_back(param);
        }
#ifdef SEC_AUDIO_BARGEIN_MODE
        else if (id == SecParameters::kBargeinMode) {
            pal_param_bargein_mode_t param_bargein_mode;
            size_t size = 0;
            int status = pal_get_param(PAL_PARAM_ID_BARGEIN_MODE, (void**)&param_bargein_mode,
                                       &size, nullptr);
            param.id = SecParameters::kBargeinMode;
            if (!status) {
                parcel.value = std::to_string(param_bargein_mode.mode);
            } else {
                parcel.value = std::to_string(-1);
            }
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_SAMSUNGRECORD
        else if (id == SecParameters::kRecordConversationEnergyKey) {
            param.id = "left";
            uint32_t power = mAudioEffect.getInterViewInputEnergy();
            parcel.value = std::to_string((power >> 16) & 0x0000ffff) +
                            ";right="+std::to_string(power & 0x0000ffff);
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
        else {
            LOG(ERROR) << __func__ << ": unknown parameter in Generic feature. id:" << id;
        }
    }
    return results;
}

std::vector<VendorParameter> SecModulePrimary::onGetSECTelephonyParameters(
        const std::vector<std::string>& ids) {
    std::vector<VendorParameter> results{};
    for (const auto& id : ids) {
        VendorParameter param;
        VString parcel;
#ifdef SEC_AUDIO_CALL_HAC
        if (id == SecParameters::kVoiceHAC) {
            param.id = SecParameters::kVoiceHAC;
            parcel.value = mPlatform.getHacIncall() ? AUDIO_PARAMETER_VALUE_HAC_ON :
                                                      AUDIO_PARAMETER_VALUE_HAC_OFF;
            setParameter(parcel, param);
            results.push_back(param);
        }
        else if (id == SecParameters::kVoiceHACMode) {
            param.id = SecParameters::kVoiceHACMode;
            parcel.value = std::to_string(mPlatform.getHacMode());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_CALL
        else if (id == SecParameters::kFactoryEchoRefMuteDetect) {
            param.id = SecParameters::kFactoryEchoRefMuteValue;
            const auto& ftmResult = mAudioEffect.getEchoRefMute();
            if (ftmResult) {
                parcel.value = ftmResult.value();
            } else {
                parcel.value = "";
            }
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_CALL_FORWARDING
        else if (id == SecParameters::kVoiceCallForwardingEnable) {
            param.id = SecParameters::kVoiceCallForwardingEnable;
            parcel.value = makeParamValue(mPlatform.getCallForwarding());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
#ifdef SEC_AUDIO_CALL_VOIP
        else if (id == SecParameters::kVoiceMicInputControlMode) {
            param.id = SecParameters::kVoiceMicInputControlMode;
            parcel.value = std::to_string(mPlatform.getVoipIsolationMode());
            setParameter(parcel, param);
            results.push_back(param);
        }
        else if (id == SecParameters::kVoiceMicInputControlModeCall) {
            param.id = SecParameters::kVoiceMicInputControlModeCall;
            parcel.value = std::to_string(mPlatform.getCallIsolationMode());
            setParameter(parcel, param);
            results.push_back(param);
        }
#endif
        else {
            LOG(ERROR) << __func__ << ": unknown parameter in Telephony feature. id:" << id;
        }
    }
    return results;
}

std::vector<VendorParameter> SecModulePrimary::onGetSECFTMParameters(
        const std::vector<std::string>& ids) {
    std::vector<VendorParameter> results{};
    std::string calValue{""};
    std::string ampType{""};

    for (const auto& id : ids) {
        VendorParameter param;
        VString parcel;
        if (id == SecParameters::kVoiceFactoryEchoRefStatus) {
            param.id = SecParameters::kVoiceFactoryEchoRefValue;
            const auto& ftmResult = mAudioEffect.getSecFTMResult();
            if (ftmResult) {
                parcel.value = ftmResult.value();
            } else {
                parcel.value = "";
            }
            setParameter(parcel, param);
            results.push_back(param);
        } else if (id == SecParameters::kFactoryTestCalStatus) {
            calValue = AUDIO_PARAMETER_VALUE_STATUS;
        } else if (id == SecParameters::kFactoryTestCalOff) {
            calValue = AUDIO_PARAMETER_VALUE_OFF;
        } else if (id == SecParameters::kFactoryTestCalRead) {
            calValue = AUDIO_PARAMETER_VALUE_READ;
        } else if (id == SecParameters::kFactoryTestCalAmpTI) {
            ampType = SecParameters::kFactoryTestCalAmpTI;
            results.push_back(makeVendorParameter(id, ampType));
        } else {
            LOG(ERROR) << __func__ << ": unknown parameter in FTM feature. id:" << id;
        }
    }
#ifdef SEC_AUDIO_SPEAKER_CALIBRATION
    if (calValue == AUDIO_PARAMETER_VALUE_STATUS
            || calValue == AUDIO_PARAMETER_VALUE_OFF
            || calValue == AUDIO_PARAMETER_VALUE_READ) {
        // processing in calibraion helper
        VendorParameter param;
        auto search = mCalValueToGetParameterMap.find(calValue);
        std::string mappedParamId{};
        if (search != mCalValueToGetParameterMap.cend()) {
            mappedParamId = search->second;
        }
        param = makeVendorParameter(mappedParamId,
                            getCalibrationResults(calValue, ampType));
        results.push_back(param);
    }
#endif
    return results;
}

ndk::ScopedAStatus SecModulePrimary::ForceSetOutDevices(
       const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices, bool force) {

    if (devices.empty()) {
        LOG(DEBUG) << __func__ << ": stream is not connected";
        //return ndk::ScopedAStatus::ok();
    }

    ModulePrimary::outListMutex.lock();
    std::vector<std::weak_ptr<StreamOut>>& outStreams = ModulePrimary::getOutStreams();
    if (!outStreams.empty()) {
        for (auto it = outStreams.begin(); it < outStreams.end(); it++) {
            if (it->lock() && !it->lock()->isClosed()) {
                it->lock()->ForceSetDevices(devices, force);
                LOG(DEBUG) << __func__ << " connected to " << devices;
            }
        }
    }
    ModulePrimary::outListMutex.unlock();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SecModulePrimary::ForceSetInDevices(
       const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices) {

    if (devices.empty()) {
        LOG(DEBUG) << __func__ << ": stream is not connected";
        //return ndk::ScopedAStatus::ok();
    }

    ModulePrimary::inListMutex.lock();
    std::vector<std::weak_ptr<StreamIn>>& inStreams = ModulePrimary::getInStreams();
    if (!inStreams.empty()) {
        for (auto it = inStreams.begin(); it < inStreams.end(); it++) {
            if (it->lock() && !it->lock()->isClosed()) {
                it->lock()->ForceSetDevices(devices);
                LOG(DEBUG) << __func__ << " connected to " << devices;
            }
        }
    }
    ModulePrimary::inListMutex.unlock();
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<StreamOut> SecModulePrimary::GetStreamOut(Usecase tag) {
    std::shared_ptr<StreamOut> streamOut = nullptr;
    ModulePrimary::outListMutex.lock();
    std::vector<std::weak_ptr<StreamOut>>& outStreams = ModulePrimary::getOutStreams();
    if (!outStreams.empty()) {
        for (auto it = outStreams.begin(); it < outStreams.end(); it++) {
            if (it->lock() && !it->lock()->isClosed() && it->lock()->isStreamUsecase(tag)) {
                streamOut = it->lock();
                break;
            }
        }
    }
    ModulePrimary::outListMutex.unlock();
    return streamOut;
}

std::shared_ptr<StreamIn> SecModulePrimary::GetStreamIn(Usecase tag) {
    std::shared_ptr<StreamIn> streamIn = nullptr;
    ModulePrimary::inListMutex.lock();
    std::vector<std::weak_ptr<StreamIn>>& inStreams = ModulePrimary::getInStreams();
    if (!inStreams.empty()) {
        for (auto it = inStreams.begin(); it < inStreams.end(); it++) {
            if (it->lock() && !it->lock()->isClosed() && it->lock()->isStreamUsecase(tag)) {
                streamIn = it->lock();
                break;
            }
        }
    }
    ModulePrimary::inListMutex.unlock();
    return streamIn;
}

#ifdef SEC_AUDIO_SAMSUNGRECORD
/*
 * Aligned with policy.h
 */
static int source_priority(int inputSource)
{
    switch (inputSource) {
    case AUDIO_SOURCE_VOICE_COMMUNICATION:
        return 12;
    case AUDIO_SOURCE_VOICE_UPLINK:
    case AUDIO_SOURCE_VOICE_DOWNLINK:
    case AUDIO_SOURCE_VOICE_CALL:
        return 11;
    case AUDIO_SOURCE_SEC_CAMCORDER:
    case AUDIO_SOURCE_CAMCORDER:
        return 10;
    case AUDIO_SOURCE_VOICE_PERFORMANCE:
        return 9;
    case AUDIO_SOURCE_VOICENOTE_BEAMFORMING:
    case AUDIO_SOURCE_UNPROCESSED:
        return 8;
    case AUDIO_SOURCE_MIC:
        return 7;
    case AUDIO_SOURCE_ECHO_REFERENCE:
        return 6;
    case AUDIO_SOURCE_2MIC_SVOICE_DRIVING:
    case AUDIO_SOURCE_2MIC_SVOICE_NORMAL:
    case AUDIO_SOURCE_BARGEIN_DRIVING:
    case AUDIO_SOURCE_SEC_VOICE_RECOGNITION:
    case AUDIO_SOURCE_FM_RX:
    case AUDIO_SOURCE_FM_TUNER:
        return 5;
    case AUDIO_SOURCE_VOICE_RECOGNITION:
        return 4;
    case AUDIO_SOURCE_HOTWORD:
        return 2;
    case AUDIO_SOURCE_ULTRASOUND:
        return 1;
    default:
        break;
    }
    return 0;
}

std::shared_ptr<StreamIn> SecModulePrimary::getHighestPriorityStreamIn() {
    std::shared_ptr<StreamIn> streamIn = nullptr;
    ModulePrimary::inListMutex.lock();
    std::vector<std::weak_ptr<StreamIn>>& inStreams = ModulePrimary::getInStreams();
    int last_priority = -1;
    int priority = 0;
    if (!inStreams.empty()) {
        for (auto it = inStreams.begin(); it < inStreams.end(); it++) {
            if (it->lock()) {
                const auto& source = getAudioSource(it->lock()->getContext().getMixPortConfig());
                priority = source_priority(static_cast<int>(source.value()));
                if (priority > last_priority) {
                    last_priority = priority;
                    streamIn = it->lock();
                }
            }
        }
    }
    ModulePrimary::inListMutex.unlock();
    return streamIn;
}
#endif

#ifdef SEC_AUDIO_CALL_VOIP
ndk::ScopedAStatus SecModulePrimary::RerouteForVoip() {
    auto streamOut = GetStreamOut(Usecase::VOIP_PLAYBACK);
    auto streamIn = GetStreamIn(Usecase::VOIP_RECORD);

    if (streamIn) {
        streamIn->ForceSetDevices(streamIn->getConnectedDevices());
    }
    if (streamOut && mPlatform.getCallMode() == AUDIO_MODE_IN_COMMUNICATION) {
        streamOut->ForceSetDevices(streamOut->getConnectedDevices());
    }

    if (streamIn && streamOut) {
        mAudioEffect.SetVoipMicModeEffect();
    }

    return ndk::ScopedAStatus::ok();
}
#endif

#ifdef SEC_AUDIO_SCREEN_MIRRORING // { SUPPORT_VOIP_VIA_SMART_VIEW
ndk::ScopedAStatus SecModulePrimary::UpdateSmartViewState(bool newVoipViaSmartView) {
    if (mPlatform.getVoipViaSmartView() != newVoipViaSmartView) {
        mPlatform.setVoipViaSmartView(newVoipViaSmartView);
        pal_param_speaker_status_t paramSpeakerStatus =
            { newVoipViaSmartView ? PAL_DEVICE_SPEAKER_MUTE : PAL_DEVICE_SPEAKER_UNMUTE };
        if (newVoipViaSmartView) { // mute > routing
            pal_set_param(PAL_PARAM_ID_SPEAKER_STATUS, (void*)&paramSpeakerStatus,
                                                    sizeof(pal_param_speaker_status_t));
            RerouteForVoip();
        } else { // routing > unmute
            RerouteForVoip();
            pal_set_param(PAL_PARAM_ID_SPEAKER_STATUS, (void*)&paramSpeakerStatus,
                                                    sizeof(pal_param_speaker_status_t));
        }
    }
    return ndk::ScopedAStatus::ok();
}
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW

#if defined(SEC_AUDIO_SUPPORT_FLIP_CALL) || defined(SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP)
void SecModulePrimary::SetFolderState(int state) {

    // SEM_LID_STATE_OPEN : 1, SEM_LID_STATE_CLOSED : 0
    // 1) 90 (flex) : flex on & fold open		 => flex path
    // 2) 90 -> 180 : flex off(change)/fold open => normal path
    // 3) 90 -> 0	: flex on/fold close(change) => flip path

    bool prevFolderclosed = mPlatform.getFolderclosed();
    bool prevFlexmode = mPlatform.getFlexmode();
    switch (state) {
        case FOLDER_CLOSE:
            mPlatform.setFolderclosed(true);
            break;
        case FOLDER_OPEN:
            mPlatform.setFolderclosed(false);
            break;
#ifdef SEC_AUDIO_SUPPORT_PERSONAL_VIDEOCALL
        case FOLDER_FLEX_ON:
            mPlatform.setFlexmode(true);
            break;
        case FOLDER_FLEX_OFF:
            mPlatform.setFlexmode(false);
            break;
#endif
        default:
            break;
    }

#ifdef SEC_AUDIO_SUPPORT_FLIP_CALL
    if ((prevFolderclosed != mPlatform.getFolderclosed()) || (prevFlexmode != mPlatform.getFlexmode())) {

        LOG(DEBUG) << "Folding(" << (prevFolderclosed != mPlatform.getFolderclosed() ? "Flip":"Flex")
                   << ") state changed, fold " << (mPlatform.getFolderclosed() ? "close":"open")
                   << ", flex " << (mPlatform.getFlexmode() ? "on":"off");

        // by flip state, separate call tx path (folding : -flip path)
        // by flex state, separate vt call tx path (flex mode : -flex path)
        // by flip state, separate call tx path (folding : -flip path)
        if (mPlatform.getCallState() == 2 /* CallState::ACTIVE */) {
            mTelephony->setDevices(mPlatform.getTelephonyDevices(), true);
        }
        RerouteForVoip();

        if (prevFolderclosed != mPlatform.getFolderclosed()) {
            bool isVoicenoteBeamformingActive = false;
            ModulePrimary::inListMutex.lock();
            std::vector<std::weak_ptr<StreamIn>>& inStreams = ModulePrimary::getInStreams();
            if (!inStreams.empty()) {
                for (auto it = inStreams.begin(); it < inStreams.end(); it++) {
                    ::aidl::android::hardware::audio::common::SinkMetadata sinkMetadata;
                    if (it->lock() && !it->lock()->isClosed()) {
                        it->lock()->getMetadata(sinkMetadata);
                        isVoicenoteBeamformingActive |=
                            (std::find_if(sinkMetadata.tracks.cbegin(), sinkMetadata.tracks.cend(),
                                [&](const auto& t) { return static_cast<audio_source_t>(t.source)
                                                           == AUDIO_SOURCE_VOICENOTE_BEAMFORMING; })
                                != sinkMetadata.tracks.cend());
                    }
                }
            }
            ModulePrimary::inListMutex.unlock();
            if (isVoicenoteBeamformingActive) {
                mAudioEffect.SetInterviewMode(!mPlatform.getFolderclosed());
            }
        }
    }
#endif

#ifdef SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP
    if (prevFolderclosed != mPlatform.getFolderclosed()) {
        mAudioEffect.updateSoundBoosterFoldDegree();
    }
#endif
}
#endif

#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_PROVIDEO
void SecModulePrimary::SetProVideoState(int mode) {
#ifdef SEC_AUDIO_RECORDALIVE_SUPPORT_MULTIDEVICE_PROVIDEO
    bool new_multidevice_rec_state = (mode == MULTIDEVICE_MIC_PROVIDEO_MODE) ? true : false;
    if (mPlatform.multidevice_rec != new_multidevice_rec_state) {
        LOG(DEBUG) << __func__ << ": multidevice_rec is changed (" << mPlatform.multidevice_rec << " -> " << new_multidevice_rec_state << ")";
        mPlatform.multidevice_rec = new_multidevice_rec_state;
        pal_param_btmix_record_t param_btmix;
        memset(&param_btmix, 0, sizeof(param_btmix));
        param_btmix.enable = new_multidevice_rec_state;
        pal_set_param(PAL_PARAM_ID_BTMIX_RECORD, (void *)&param_btmix,
                    sizeof(param_btmix));
    }

    mPlatform.preprocess_eq_enables &= ~(S_REC_PROVIDEO|S_REC_PROVIDEO_MULTIDEVICE);
    if (mode == MULTI_MIC_PROVIDEO_MODE) {
        mPlatform.preprocess_eq_enables |= S_REC_PROVIDEO;
    } else if (mode == MULTIDEVICE_MIC_PROVIDEO_MODE) {
        mPlatform.preprocess_eq_enables |= (S_REC_PROVIDEO|S_REC_PROVIDEO_MULTIDEVICE);
    }
#else
    if (mode == MULTI_MIC_PROVIDEO_MODE) {
        mPlatform.preprocess_eq_enables |= S_REC_PROVIDEO;
    } else {
        mPlatform.preprocess_eq_enables &= ~(S_REC_PROVIDEO);
    }
#endif
}
#endif

#ifdef SEC_AUDIO_SUPPORT_REMOTE_MIC
pal_device_id_t SecModulePrimary::getPrimaryOutPalDeviceId() {
    pal_device_id_t deviceId = PAL_DEVICE_NONE;
    auto streamOut = GetStreamOut(Usecase::PRIMARY_PLAYBACK);
    if (streamOut) {
        auto connectedDevices = streamOut->getConnectedDevices();
        if (!connectedDevices.empty()) {
            auto palDevices = mPlatform.convertToPalDevices(connectedDevices);
            deviceId = palDevices[0].id;
        }
    }
    return deviceId;
}
#endif

#ifdef SEC_AUDIO_COMMON
bool SecModulePrimary::CheckComboDevice() {

    Usecase usecase_list[] = {
#ifdef SEC_AUDIO_SUPPORT_MEDIA_OUTPUT
                                Usecase::PRIMARY_PLAYBACK,
#else
                                Usecase::DEEP_BUFFER_PLAYBACK,
#endif
                                Usecase::HAPTICS_PLAYBACK};

    int size = sizeof(usecase_list) / sizeof(usecase_list[0]);
    for (int i = 0; i < size; i++) {
        auto streamOut = GetStreamOut(usecase_list[i]);
        if (streamOut == nullptr) {
            continue;
        } else if (streamOut->HasPalStreamHandle() &&
                (streamOut->getConnectedDevices().size() == 2)) {
            return true;
        }
    }
    return false;
}
#endif

#ifdef SEC_AUDIO_KARAOKE
void SecModulePrimary::setKaraokeDevice() {
    if (!mPlatform.isKaraokeEnabled())
        return;

    bool active_output = false;
    std::shared_ptr<StreamOut> streamOut = nullptr;
    Usecase output_list[] = {Usecase::DEEP_BUFFER_PLAYBACK,
                                Usecase::ULL_PLAYBACK,
                                Usecase::LOW_LATENCY_PLAYBACK};
    int size = sizeof(output_list) / sizeof(output_list[0]);
    for (int i = 0; i < size; i++) {
        streamOut = GetStreamOut(output_list[i]);
        if (!streamOut || !streamOut->HasPalStreamHandle() || streamOut->getConnectedDevices().size() > 1) {
            continue;
        }

        if (streamOut->isDeviceAvailable(PAL_DEVICE_OUT_SPEAKER)) {
            std::vector<AudioDevice> devices = {AudioDevice{.type.type = AudioDeviceType::OUT_SPEAKER}};
            streamOut->ForceSetDevices(devices);
            active_output = true;
        }
    }

    if (active_output) {
        ModulePrimary::inListMutex.lock();
        std::vector<std::weak_ptr<StreamIn>>& inStreams = ModulePrimary::getInStreams();
        //in force routing
        if (!inStreams.empty()) {
            std::vector<AudioDevice> devices;
            devices.push_back(AudioDevice{.type.type = AudioDeviceType::IN_MICROPHONE_BACK});
            for (auto it = inStreams.begin(); it < inStreams.end(); it++) {
                if (it->lock() && !it->lock()->isClosed()) {
                    it->lock()->ForceSetDevices(devices);
                }
            }
        }
        ModulePrimary::inListMutex.unlock();
    }
}
#endif

#ifdef SEC_AUDIO_BLE_OFFLOAD
void SecModulePrimary::UpdateSCOdeviceState() {
    // if sco device active, do shutdown out stream
    if (!mPlatform.isBtScoOn())  {
        return;
    }

    ModulePrimary::outListMutex.lock();
    std::vector<std::weak_ptr<StreamOut>>& outStreams = ModulePrimary::getOutStreams();
    if (!outStreams.empty()) {
        for (auto it = outStreams.begin(); it < outStreams.end(); it++) {
            if (it->lock() && !it->lock()->isClosed()) {
                auto devices = it->lock()->getConnectedDevices();
                if (!devices.empty() &&
                        it->lock()->HasPalStreamHandle() &&
                        it->lock()->isDeviceAvailable(PAL_DEVICE_OUT_BLUETOOTH_SCO)) {
                    LOG(DEBUG) << __func__ << "do forceShutdown for re-open pcm";
                    it->lock()->forceShutdown();
                }
            }
        }
    }
    ModulePrimary::outListMutex.unlock();
    return;
}
#endif

#ifdef SEC_AUDIO_SUPPORT_AFE_LISTENBACK
bool SecModulePrimary::isListenbackDevice(pal_device_id_t deviceId) {
    if (deviceId == PAL_DEVICE_OUT_WIRED_HEADSET ||
            deviceId == PAL_DEVICE_OUT_WIRED_HEADPHONE ||
            deviceId == PAL_DEVICE_OUT_USB_HEADSET) {
        return true;
    }

    return false;
}

bool SecModulePrimary::isListenbackUsecase(Usecase usecase) {
    switch (usecase) {
        case Usecase::DEEP_BUFFER_PLAYBACK:
        case Usecase::ULL_PLAYBACK:
        case Usecase::PCM_RECORD:
        case Usecase::FAST_RECORD:
        case Usecase::MMAP_RECORD:
            return true;
        default:
            break;
    }
    return false;
}

int SecModulePrimary::getListenbackOutputCount() {
    Usecase output_list[] = {Usecase::DEEP_BUFFER_PLAYBACK, Usecase::ULL_PLAYBACK};
    int cnt = 0;
    int size = sizeof(output_list) / sizeof(output_list[0]);
    for (int i = 0; i < size; i++) {
        auto streamOut = GetStreamOut(output_list[i]);
        // support listenback for 3.5 pi or usb headset only
        if (streamOut && streamOut->HasPalStreamHandle()) {
            cnt++;
        }
    }

    return cnt;
}

void SecModulePrimary::updateStreamListenbackMode(bool enable) {
    Usecase output_list[] = {Usecase::DEEP_BUFFER_PLAYBACK, Usecase::ULL_PLAYBACK};
    int size = sizeof(output_list) / sizeof(output_list[0]);
    for (int i = 0; i < size; i++) {
        auto streamOut = GetStreamOut(output_list[i]);
        // support listenback for 3.5 pi or usb headset only
        if (streamOut && streamOut->HasPalStreamHandle()) {
            streamOut->updateListenback(enable);
            break;
        }
    }
}
#endif

#ifdef SEC_AUDIO_USB_GAIN_CONTROL
void SecModulePrimary::updateUsbAudioGain() {
    LOG(DEBUG) << __func__ << " do ForceSetDevices for gain setting for deep";
    auto streamOut = SecModulePrimary::GetStreamOut(Usecase::DEEP_BUFFER_PLAYBACK);
    if (streamOut) {
        streamOut->ForceSetDevices(streamOut->getConnectedDevices());
    }
}
#endif

SecModulePrimary& SecModulePrimary::getInstance() {
    static const auto instance = []() {
        std::unique_ptr<SecModulePrimary> secModulePrimary{new SecModulePrimary()};
        return std::move(secModulePrimary);
    }();
    return *(instance.get());
}

}
