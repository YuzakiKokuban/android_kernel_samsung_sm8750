/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <string>
#ifdef ENABLE_TAS_SPK_PROT
#include "TISpeakerProtDefs.h"
#endif

namespace qti::audio::core::Parameters {

/**
 * Since the parameters from the Android framework enables or disables features
 * which would impact small to big level, It is highly recommended to write
 * verbose comments for each parameter. As Parameter is composition 'id' and 'its
 * possibles values', hence list all the values with verbose explaination
 **/

// HDR Recording
const static std::string kHdrRecord{"hdr_record_on"};
const static std::string kHdrChannelCount{"hdr_audio_channel_count"};
const static std::string kHdrSamplingRate{"hdr_audio_sampling_rate"};
const static std::string kWnr{"wnr_on"};
const static std::string kAns{"ans_on"};
const static std::string kOrientation{"orientation"};
const static std::string kInverted{"inverted"};
const static std::string kFacing{"facing"};


// voice
const static std::string kVoiceCallState{"call_state"};
const static std::string kVoiceCallType{"call_type"};
const static std::string kVoiceVSID{"vsid"};
const static std::string kVoiceDeviceMute{"device_mute"};
const static std::string kVolumeBoost{"volume_boost"};
const static std::string kVoiceDirection{"direction"};
const static std::string kVoiceSlowTalk{"st_enable"};
const static std::string kVoiceHDVoice{"hd_voice"};
const static std::string kVoiceIsCRsSupported{"isCRSsupported"};
const static std::string kVoiceCRSCall{"crs_call"};
const static std::string kVoiceCRSVolume{"CRS_volume"};
const static std::string kVoiceTranslationRxMute{"voice_translation_rx_mute"};
/** kVoiceTranslationRxMute : helps to set the Voice Rx Volume
* to mute when the param is set to enabled during the
* voice call translation usecase running.
**/


// WFD
const static std::string kCanOpenProxy{"can_open_proxy"};
const static std::string kWfdChannelMap{"wfd_channel_cap"};
const static std::string kWfdProxyRecordActive{"proxyRecordActive"};

/**
 * USE_IP_IN_DEVICE_FOR_PROXY_RECORD: Use this parameter to set/unset if ip-v4 in device
 * in getting used a proxy device. Set it before making the device available and unset
 * it while making device unavailable.
 **/
const static std::string kWfdIPAsProxyDevConnected{"USE_IP_IN_DEVICE_FOR_PROXY_RECORD"};
/**
 * clients have need to hardcode
 * frame count requirement per read.
 * Ideally, client should be able read
 * as AHAL provided. Still, AHAL supports
 * this way to set module vendor parameter
 * to request a custom FMQ size from client
 * FMQ size.
 * example:
 * As the session starts, client sets
 * proxy_record_fmq_size = 480

 * As session ends, client unsets
 * proxy_record_fmq_size = 0

 * After the session of proxy record finishes,
 * client is resposible to unset the module
 * vendor parameter.

 * For upcoming requirements, this way is
 * depreciated.
 **/
const static std::string kProxyRecordFMQSize{"proxy_record_fmq_size"};

// Generic
const static std::string kInCallMusic{"icmd_playback"};
const static std::string kUHQA{"UHQA"};
const static std::string kOffloadPlaySpeedSupported{"offloadVariableRateSupported"};
const static std::string kSupportsHwSuspend{"supports_hw_suspend"};
const static std::string kIsDirectPCMTrack{"is_direct_pcm_track"};
const static std::string kTranslateRecord{"translate_record"};
#ifdef ENABLE_TAS_SPK_PROT
const static std::string kTiSmartPaKey{AUDIO_PARAM_TI_SMARTPA};
#endif
/**
 * translate_record : AUDIO_FLUENCE_FFECNS PCM_RECORD
 * Use this parameter to for the Voice Translation usecase.
 * Set param support for APK to select FFECNS record and populate
 * custom key for FFECNS record based on the setparam.
 **/

// FTM
const static std::string kFbspCfgWaitTime{"fbsp_cfg_wait_time"};
const static std::string kFbspFTMWaitTime{"fbsp_cfg_ftm_time"};
const static std::string kFbspValiWaitTime{"fbsp_v_vali_wait_time"};
const static std::string kFbspValiValiTime{"fbsp_v_vali_vali_time"};
const static std::string kTriggerSpeakerCall{"trigger_spkr_cal"};
const static std::string kFTMParam{"get_ftm_param"};
const static std::string kFTMSPKRParam{"get_spkr_cal"};

// Audio Extn
const static std::string kFMStatus{"fm_status"};

// Bluetooth
const static std::string kA2dpSuspended{"A2dpSuspended"};

// Haptics
const static std::string kHapticsVolume{"haptics_volume"};
const static std::string kHapticsIntensity{"haptics_intensity"};

}; // namespace qti::audio::core::Parameters
