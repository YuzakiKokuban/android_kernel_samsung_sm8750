/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.qti.hardware.pal;
@Backing(type="int") @VintfStability
enum PalParamId {
  PAL_PARAM_ID_LOAD_SOUND_MODEL = 0,
  PAL_PARAM_ID_RECOGNITION_CONFIG = 1,
  PAL_PARAM_ID_ECNS_ON_OFF = 2,
  PAL_PARAM_ID_DIRECTION_OF_ARRIVAL = 3,
  PAL_PARAM_ID_UIEFFECT = 4,
  PAL_PARAM_ID_STOP_BUFFERING = 5,
  PAL_PARAM_ID_CODEC_CONFIGURATION = 6,
  PAL_PARAM_ID_DEVICE_CONNECTION = 7,
  PAL_PARAM_ID_SCREEN_STATE = 8,
  PAL_PARAM_ID_CHARGING_STATE = 9,
  PAL_PARAM_ID_DEVICE_ROTATION = 10,
  PAL_PARAM_ID_BT_SCO = 11,
  PAL_PARAM_ID_BT_SCO_WB = 12,
  PAL_PARAM_ID_BT_SCO_SWB = 13,
  PAL_PARAM_ID_BT_A2DP_RECONFIG = 14,
  PAL_PARAM_ID_BT_A2DP_RECONFIG_SUPPORTED = 15,
  PAL_PARAM_ID_BT_A2DP_SUSPENDED = 16,
  PAL_PARAM_ID_BT_A2DP_TWS_CONFIG = 17,
  PAL_PARAM_ID_BT_A2DP_ENCODER_LATENCY = 18,
  PAL_PARAM_ID_DEVICE_CAPABILITY = 19,
  PAL_PARAM_ID_GET_SOUND_TRIGGER_PROPERTIES = 20,
  PAL_PARAM_ID_TTY_MODE = 21,
  PAL_PARAM_ID_VOLUME_BOOST = 22,
  PAL_PARAM_ID_SLOW_TALK = 23,
  PAL_PARAM_ID_SPEAKER_RAS = 24,
  PAL_PARAM_ID_SP_MODE = 25,
  PAL_PARAM_ID_GAIN_LVL_MAP = 26,
  PAL_PARAM_ID_GAIN_LVL_CAL = 27,
  PAL_PARAM_ID_GAPLESS_MDATA = 28,
  PAL_PARAM_ID_HD_VOICE = 29,
  PAL_PARAM_ID_WAKEUP_ENGINE_CONFIG = 30,
  PAL_PARAM_ID_WAKEUP_BUFFERING_CONFIG = 31,
  PAL_PARAM_ID_WAKEUP_ENGINE_RESET = 32,
  PAL_PARAM_ID_WAKEUP_MODULE_VERSION = 33,
  PAL_PARAM_ID_WAKEUP_CUSTOM_CONFIG = 34,
  PAL_PARAM_ID_UNLOAD_SOUND_MODEL = 35,
  PAL_PARAM_ID_MODULE_CONFIG = 36,
  PAL_PARAM_ID_BT_A2DP_LC3_CONFIG = 37,
  PAL_PARAM_ID_PROXY_CHANNEL_CONFIG = 38,
  PAL_PARAM_ID_CONTEXT_LIST = 39,
  PAL_PARAM_ID_HAPTICS_INTENSITY = 40,
  PAL_PARAM_ID_HAPTICS_VOLUME = 41,
  PAL_PARAM_ID_BT_A2DP_DECODER_LATENCY = 42,
  PAL_PARAM_ID_CUSTOM_CONFIGURATION = 43,
  PAL_PARAM_ID_KW_TRANSFER_LATENCY = 44,
  PAL_PARAM_ID_BT_A2DP_FORCE_SWITCH = 45,
  PAL_PARAM_ID_BT_SCO_LC3 = 46,
  PAL_PARAM_ID_DEVICE_MUTE = 47,
  PAL_PARAM_ID_UPD_REGISTER_FOR_EVENTS = 48,
  PAL_PARAM_ID_SP_GET_CAL = 49,
  PAL_PARAM_ID_BT_A2DP_CAPTURE_SUSPENDED = 50,
  PAL_PARAM_ID_SNDCARD_STATE = 51,
  PAL_PARAM_ID_HIFI_PCM_FILTER = 52,
  PAL_PARAM_ID_CHARGER_STATE = 53,
  PAL_PARAM_ID_BT_SCO_NREC = 54,
  PAL_PARAM_ID_VOLUME_USING_SET_PARAM = 55,
  PAL_PARAM_ID_UHQA_FLAG = 56,
  PAL_PARAM_ID_STREAM_ATTRIBUTES = 57,
}
