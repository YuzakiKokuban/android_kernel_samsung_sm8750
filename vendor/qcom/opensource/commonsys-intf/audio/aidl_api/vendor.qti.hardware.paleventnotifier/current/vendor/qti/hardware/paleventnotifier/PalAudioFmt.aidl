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
enum PalAudioFmt {
  PAL_AUDIO_FMT_DEFAULT_PCM = 0x1,
  PAL_AUDIO_FMT_PCM_S16_LE = PAL_AUDIO_FMT_DEFAULT_PCM /* 1 */,
  PAL_AUDIO_FMT_DEFAULT_COMPRESSED = 0x2,
  PAL_AUDIO_FMT_MP3 = PAL_AUDIO_FMT_DEFAULT_COMPRESSED /* 2 */,
  PAL_AUDIO_FMT_AAC = 0x3,
  PAL_AUDIO_FMT_AAC_ADTS = 0x4,
  PAL_AUDIO_FMT_AAC_ADIF = 0x5,
  PAL_AUDIO_FMT_AAC_LATM = 0x6,
  PAL_AUDIO_FMT_WMA_STD = 0x7,
  PAL_AUDIO_FMT_ALAC = 0x8,
  PAL_AUDIO_FMT_APE = 0x9,
  PAL_AUDIO_FMT_WMA_PRO = 0xA,
  PAL_AUDIO_FMT_FLAC = 0xB,
  PAL_AUDIO_FMT_FLAC_OGG = 0xC,
  PAL_AUDIO_FMT_VORBIS = 0xD,
  PAL_AUDIO_FMT_AMR_NB = 0xE,
  PAL_AUDIO_FMT_AMR_WB = 0xF,
  PAL_AUDIO_FMT_AMR_WB_PLUS = 0x10,
  PAL_AUDIO_FMT_EVRC = 0x11,
  PAL_AUDIO_FMT_G711 = 0x12,
  PAL_AUDIO_FMT_QCELP = 0x13,
  PAL_AUDIO_FMT_PCM_S8 = 0x14,
  PAL_AUDIO_FMT_PCM_S24_3LE = 0x15,
  PAL_AUDIO_FMT_PCM_S24_LE = 0x16,
  PAL_AUDIO_FMT_PCM_S32_LE = 0x17,
  PAL_AUDIO_FMT_OPUS = 0x18,
  PAL_AUDIO_FMT_COMPRESSED_RANGE_BEGIN = 0xF0000000,
  PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_BEGIN = 0xF0000F00,
  PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_END = 0xF0000FFF,
  PAL_AUDIO_FMT_COMPRESSED_RANGE_END = PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_END /* -268431361 */,
}
