/*
 * Copyright (C) 2023 The Android Open Source Project
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

#pragma once

#include <string>
#include <system/audio.h>
#include <hardware/audio.h>

namespace qti::audio::core::SecParameters {

enum class Feature_SEC : uint16_t {
    SEC_GENERIC = 0,
    SEC_TELEPHONY,
    SEC_FTM,
    SEC_SUBKEY,
};

// Voice
const static std::string kFactoryEchoRefMuteDetect{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_ECHOREF_MUTE_DETECT};
const static std::string kFactoryEchoRefMuteValue{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_ECHOREF_MUTE_VALUE};
const static std::string kTtyMode{AUDIO_PARAMETER_KEY_TTY_MODE};
const static std::string kVoiceCallBand{AUDIO_PARAMETER_SEC_GLOBAL_CALL_BAND};
const static std::string kVoiceCallForwardingEnable{AUDIO_PARAMETER_SEC_GLOBAL_CALL_FORWARDING_ENABLE};
const static std::string kVoiceCallMemoState{AUDIO_PARAMETER_SEC_GLOBAL_CALL_MEMO_STATE};
const static std::string kVoiceCallNBQualityEnable{AUDIO_PARAMETER_SEC_LOCAL_CALL_NB_QUALITY_ENABLE};
const static std::string kVoiceCallRingbacktoneState{AUDIO_PARAMETER_SEC_GLOBAL_CALL_RINGBACKTONE_STATE};
const static std::string kVoiceCallSatelliteEnable{AUDIO_PARAMETER_SEC_GLOBAL_CALL_SATELLITE_ENABLE};
const static std::string kVoiceCallState{AUDIO_PARAMETER_SEC_GLOBAL_CALL_STATE};
const static std::string kVoiceCallTranslationMode{AUDIO_PARAMETER_SEC_LOCAL_CALL_TRANSLATION_MODE};
const static std::string kVoiceEffectDVAdaptSound{AUDIO_PARAMETER_SEC_GLOBAL_EFFECT_DV_ADAPT_SOUND};
const static std::string kVoiceEffectDVAdaptSoundCallPatam{AUDIO_PARAMETER_SEC_LOCAL_EFFECT_DV_ADAPT_SOUND_CALL_PARAM};
const static std::string kVoiceFactoryEchoRefMuteCNGEnable{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_ECHOREF_MUTE_CNG_ENABLE};
const static std::string kVoiceHAC{AUDIO_PARAMETER_KEY_HAC};
const static std::string kVoiceHACMode{AUDIO_PARAMETER_SEC_LOCAL_CALL_HAC_MODE};
const static std::string kVoiceScreenCall{AUDIO_PARAMETER_SEC_LOCAL_SCREEN_CALL};
const static std::string kVoiceStreamEnforcedActiveInCall{AUDIO_PARAMETER_SEC_LOCAL_STREAM_ENFORCED_ACTIVE_IN_CALL};
const static std::string kVoiceTxControlMode{AUDIO_PARAMETER_SEC_LOCAL_VOICE_TX_CONTROL_MODE};
const static std::string kVoiceRxControlMode{AUDIO_PARAMETER_SEC_LOCAL_VOICE_RX_CONTROL_MODE};
const static std::string kVoiceMicInputControlMode{AUDIO_PARAMETER_SEC_LOCAL_MIC_INPUT_CONTROL_MODE};
const static std::string kVoiceMicInputControlModeCall{AUDIO_PARAMETER_SEC_LOCAL_MIC_INPUT_CONTROL_MODE_CALL};
const static std::string kVoiceVSID{AUDIO_PARAMETER_SEC_GLOBAL_CALL_SIM_SLOT};

// Factory Test
const static std::string kFactoryTestLoopback{AUDIO_PARAMETER_FACTORY_TEST_LOOPBACK};
const static std::string kFactoryTestMicPath{AUDIO_PARAMETER_KEY_FACTORY_RMS_TEST};
const static std::string kFactoryTestPath{AUDIO_PARAMETER_FACTORY_TEST_PATH};
const static std::string kFactoryTestRoute{AUDIO_PARAMETER_FACTORY_TEST_ROUTE};
const static std::string kFactoryTestType{AUDIO_PARAMETER_FACTORY_TEST_TYPE};
const static std::string kFactoryTestSpkPath{AUDIO_PARAMETER_FACTORY_TEST_SPKPATH};
const static std::string kFactoryTestCalibration{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_CALIBRATION_KEY};
const static std::string kFactoryTestCalAmpType{AUDIO_PARAMETER_SUBKEY_FACTORY_CALIBRATION_AMP};
const static std::string kFactoryTestCalTarget{AUDIO_PARAMETER_SUBKEY_FACTORY_CALIBRATION_TARGET};
const static std::string kFactoryTestCalStatus{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_CALIBRATION_STATUS};
const static std::string kFactoryTestCalOff{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_CALIBRATION_OFF};
const static std::string kFactoryTestCalRead{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_CALIBRATION_READ};
const static std::string kFactoryTestCalAmpTI{AUDIO_PARAMETER_VALUE_TAS_V2};
const static std::string kVoiceFactoryEchoRefStatus{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_ECHOREF_STATUS};
const static std::string kVoiceFactoryEchoRefValue{AUDIO_PARAMETER_SEC_GLOBAL_FACTORY_ECHOREF_VALUE};

// Effect
const static std::string kEffectOffloadVSPParam{AUDIO_PARAMETER_SEC_GLOBAL_EFFECT_OFFLOAD_VSP_PARAM};
const static std::string kEffectSoundBalanceValue{AUDIO_PARAMETER_SEC_GLOBAL_EFFECT_SOUND_BALANCE_VALUE};
const static std::string kEffectToMonoEnable{AUDIO_PARAMETER_SEC_GLOBAL_EFFECT_TO_MONO_ENABLE};
const static std::string kRecordConversationEnergyKey{AUDIO_PARAMETER_SEC_GLOBAL_RECORD_CONVERSATION_ENERGY_KEY};

// Generic
const static std::string kAllSoundMuteEnable{AUDIO_PARAMETER_SEC_LOCAL_ALL_SOUND_MUTE_ENABLE};
const static std::string kA2dpFormat{AUDIO_PARAMETER_SEC_LOCAL_A2DP_FORMAT};
const static std::string kA2dpSuspendForBle{AUDIO_PARAMETER_SEC_LOCAL_A2DP_SUSPEND_FOR_BLE};
const static std::string kBargeinMode{AUDIO_PARAMETER_SEC_GLOBAL_BARGEIN_MODE};
const static std::string kBtScoCodecType{AUDIO_PARAMETER_SEC_GLOBAL_BT_SCO_CODEC_TYPE};
const static std::string kEffectSoundBoosterDspSupport{AUDIO_PARAMETER_SEC_LOCAL_EFFECT_SOUNDBOOSTER_DSP_SUPPORT};
const static std::string kFMRadioMode{AUDIO_PARAMETER_SEC_LOCAL_FMRADIO_MODE};
const static std::string kFMRadioVolume{AUDIO_PARAMETER_SEC_LOCAL_FMRADIO_VOLUME};
const static std::string kFMRadioMute{AUDIO_PARAMETER_SEC_GLOBAL_FMRADIO_MUTE};
const static std::string kGameChatEnable{AUDIO_PARAMETER_SEC_LOCAL_GAME_CHAT_ENABLE};
const static std::string kHwDisplayRotation{AUDIO_PARAMETER_SEC_GLOBAL_HW_DISPLAY_ROTATION};
const static std::string kHwFlatMotionState{AUDIO_PARAMETER_SEC_LOCAL_HW_FLAT_MOTION_STATE};
const static std::string kHwFolderState{AUDIO_PARAMETER_SEC_LOCAL_HW_FOLDER_STATE};
const static std::string kHwInterfaceTestcase{AUDIO_PARAMETER_SEC_LOCAL_HW_INTERFACE_TESTCASE};
const static std::string kHwSpeakerAmpBigData{AUDIO_PARAMETER_SEC_LOCAL_HW_SPEAKER_AMP_BIGDATA};
const static std::string kHwSpeakerAmpBigDataSupport{AUDIO_PARAMETER_SEC_LOCAL_HW_SPEAKER_AMP_BIGDATA_SUPPORT};
const static std::string kHwSpeakerAmpMaxTemperature{AUDIO_PARAMETER_SEC_GLOBAL_HW_SPEAKER_AMP_MAX_TEMPERATURE};
const static std::string kHwSpeakerAmpTemperatureRCV{AUDIO_PARAMETER_SEC_GLOBAL_HW_SPEAKER_AMP_TEMPERATURE_RCV};
const static std::string kHwSpeakerAmpTemperatureSPK{AUDIO_PARAMETER_SEC_GLOBAL_HW_SPEAKER_AMP_TEMPERATURE_SPK};
const static std::string kInterpreterMode{AUDIO_PARAMETER_SEC_GLOBAL_INTERPRETER_MODE};
const static std::string kKaraokeEnable{AUDIO_PARAMETER_SEC_LOCAL_KARAOKE_ENABLE};
const static std::string kOffloadVariableRateSupported{"offloadVariableRateSupported"};
const static std::string kPcmDumpApCallState{AUDIO_PARAMETER_SEC_GLOBAL_PCM_DUMP_AP_CALL_STATE};
const static std::string kPcmDumpRecordState{AUDIO_PARAMETER_SEC_LOCAL_PCM_DUMP_RECORD_STATE};
const static std::string kPcmDumpState{AUDIO_PARAMETER_SEC_GLOBAL_PCM_DUMP_STATE};
const static std::string kRecordBeamformingMode{AUDIO_PARAMETER_SEC_GLOBAL_RECORD_BEAMFORMING_MODE};
const static std::string kRecordInputLatency{AUDIO_PARAMETER_SEC_GLOBAL_RECORD_INPUT_LATENCY};
const static std::string kRecordNSRISecurityEnable{AUDIO_PARAMETER_SEC_GLOBAL_RECORD_NSRI_SECURITY_ENABLE};
const static std::string kRecordSecVoiceRecorderEnable{AUDIO_PARAMETER_SEC_GLOBAL_RECORD_SEC_VOICE_RECORDER_ENABLE};
const static std::string kRecordTxInversion{AUDIO_PARAMETER_SEC_GLOBAL_RECORD_TX_INVERSION};
const static std::string kRemoteMicEnable{AUDIO_PARAMETER_SEC_LOCAL_REMOTE_MIC_ENABLE};
const static std::string kRemoteMicVolume{AUDIO_PARAMETER_SEC_LOCAL_REMOTE_MIC_VOLUME};
const static std::string kScoRvcSupport{AUDIO_PARAMETER_SEC_GLOBAL_SCO_RVC_SUPPORT};
const static std::string kSetupTestcase{AUDIO_PARAMETER_SEC_GLOBAL_SETUP_TESTCASE};
const static std::string kSupportSecAudioFeature{AUDIO_PARAMETER_SEC_LOCAL_SUPPORT_SEC_AUDIO_FEATURE};
const static std::string kVoiceWakeupRegisterVoiceKeyword{AUDIO_PARAMETER_SEC_GLOBAL_VOICE_WAKEUP_REGISTER_VOICE_KEYWORD};
const static std::string kVoiceWakeupSeamlessEnable{AUDIO_PARAMETER_SEC_GLOBAL_VOICE_WAKEUP_SEAMLESS_ENABLE};
const static std::string kVoipViaSmartView{AUDIO_PARAMETER_SEC_LOCAL_VOIP_VIA_SMART_VIEW};
const static std::string kMultiMicMode{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_MODE};
const static std::string kInputFlag{AUDIO_PARAMETER_SEC_LOCAL_RECORD_INPUT_FLAG};

// Stream Out
const static std::string kDualSpeakerAmpLeftPowerEnable{AUDIO_PARAMETER_SEC_LOCAL_DUAL_SPEAKER_AMP_LEFT_POWER_ENABLE};
const static std::string kEffectUpscalerMode{AUDIO_PARAMETER_SEC_LOCAL_EFFECT_UPSCALER_MODE};
const static std::string kHapticsSource{AUDIO_PARAMETER_SEC_LOCAL_HAPTIC_SOURCE};
const static std::string kUhqUpdateFormat{AUDIO_PARAMETER_SEC_LOCAL_UHQ_UPDATE_FORMAT};
const static std::string kVolumeVoice{AUDIO_PARAMETER_SEC_LOCAL_VOLUME_VOICE};

// Stream In

// Subkey
const static std::string kDexKey{AUDIO_PARAMETER_SEC_LOCAL_DEX_KEY};
const static std::string kSubkeyDexType{AUDIO_PARAMETER_SUBKEY_DEX_TYPE};
const static std::string kSubkeyDexConnected{AUDIO_PARAMETER_SUBKEY_DEX_CONNECTED};
const static std::string kEffectListenBackKey{AUDIO_PARAMETER_SEC_LOCAL_EFFECT_LISTENBACK_KEY};
const static std::string kSubkeyEffectListenBackState{AUDIO_PARAMETER_SUBKEY_EFFECT_LISTENBACK_STATE};
const static std::string kMultiMicKey{AUDIO_PARAMETER_SEC_LOCAL_MULTI_MIC_KEY};
const static std::string kSubkeyMultiMicAudioFocusEnable{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_AUDIO_FOCUS_ENABLE};
const static std::string kSubkeyMultiMicCameraDirection{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_CAMERA_DIRECTION};
const static std::string kSubkeyMultiMicFocusCoordinate{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_FOCUS_COORDINATE};
const static std::string kSubkeyMultiMicMode{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_MODE};
const static std::string kSubkeyMultiMicPhoneOrientation{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_PHONE_ORIENTATION};
const static std::string kSubkeyMultiMicSensitivityLevel{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_SENSITIVITY_LEVEL};
const static std::string kSubkeyMultiMicZoomLevel{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_ZOOM_LEVEL};
const static std::string kSubkeyMultiMicZoomMax{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_ZOOM_MAX};
const static std::string kSubkeyMultiMicZoomMin{AUDIO_PARAMETER_SUBKEY_MULTI_MIC_ZOOM_MIN};
const static std::string kSubkeyUhqWideResolution{AUDIO_PARAMETER_SEC_LOCAL_UHQ_WIDE_RESOLUTION_ENABLE};
const static std::string kSubkeyUhqForceRouting{AUDIO_PARAMETER_SEC_LOCAL_UHQ_FORCE_ROUTING};

}; // namespace qti::audio::core::SecParameters
