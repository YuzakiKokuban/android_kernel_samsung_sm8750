/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

package vendor.qti.hardware.paleventnotifier;
@Backing(type="int") @VintfStability
enum PalStreamType {
  PAL_STREAM_LOW_LATENCY = 1,
  PAL_STREAM_DEEP_BUFFER = 2,
  PAL_STREAM_COMPRESSED = 3,
  PAL_STREAM_VOIP = 4,
  PAL_STREAM_VOIP_RX = 5,
  PAL_STREAM_VOIP_TX = 6,
  PAL_STREAM_VOICE_CALL_MUSIC = 7,
  PAL_STREAM_GENERIC = 8,
  PAL_STREAM_RAW = 9,
  PAL_STREAM_VOICE_RECOGNITION = 10,
  PAL_STREAM_VOICE_CALL_RECORD = 11,
  PAL_STREAM_VOICE_CALL_TX = 12,
  PAL_STREAM_VOICE_CALL_RX_TX = 13,
  PAL_STREAM_VOICE_CALL = 14,
  PAL_STREAM_LOOPBACK = 15,
  PAL_STREAM_TRANSCODE = 16,
  PAL_STREAM_VOICE_UI = 17,
  PAL_STREAM_PCM_OFFLOAD = 18,
  PAL_STREAM_ULTRA_LOW_LATENCY = 19,
  PAL_STREAM_PROXY = 20,
  PAL_STREAM_NON_TUNNEL = 21,
  PAL_STREAM_HAPTICS = 22,
  PAL_STREAM_ACD = 23,
  PAL_STREAM_CONTEXT_PROXY = 24,
  PAL_STREAM_SENSOR_PCM_DATA = 25,
  PAL_STREAM_ULTRASOUND = 26,
  PAL_STREAM_SPATIAL_AUDIO = 27,
}
