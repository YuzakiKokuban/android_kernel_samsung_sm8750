/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _BT_BASE_H_
#define _BT_BASE_H_

#include "bt_intf.h"

#define CH_MONO                         1
#define CH_STEREO                       2
#define SAMPLING_RATE_16K               16000 // SS_BT_HFP - H_127 : RVP
#define SAMPLING_RATE_32K               32000
#define SAMPLING_RATE_44K               44100
#define SAMPLING_RATE_48K               48000
#define SAMPLING_RATE_96K               96000
#define ENCODER_BIT_FORMAT_DEFAULT      16
#define ENCODER_BIT_FORMAT_PCM_24       24


enum {
    AUDIO_LOCATION_NONE                  = 0x00000000, // Mono/Unspecified
    AUDIO_LOCATION_FRONT_LEFT            = 0x00000001,
    AUDIO_LOCATION_FRONT_RIGHT           = 0x00000002,
    AUDIO_LOCATION_FRONT_CENTER          = 0x00000004,
    AUDIO_LOCATION_LOW_FREQUENCY         = 0x00000008,
    AUDIO_LOCATION_BACK_LEFT             = 0x00000010,
    AUDIO_LOCATION_BACK_RIGHT            = 0x00000020,
    AUDIO_LOCATION_FRONT_LEFT_OF_CENTER  = 0x00000040,
    AUDIO_LOCATION_FRONT_RIGHT_OF_CENTER = 0x00000080,
    AUDIO_LOCATION_BACK_CENTER           = 0x00000100,
    AUDIO_LOCATION_LOW_FREQUENCY_2       = 0x00000200,
    AUDIO_LOCATION_SIDE_LEFT             = 0x00000400,
    AUDIO_LOCATION_SIDE_RIGHT            = 0x00000800,
    AUDIO_LOCATION_TOP_FRONT_LEFT        = 0x00001000,
    AUDIO_LOCATION_TOP_FRONT_RIGHT       = 0x00002000,
    AUDIO_LOCATION_TOP_FRONT_CENTER      = 0x00004000,
    AUDIO_LOCATION_TOP_CENTER            = 0x00008000,
    AUDIO_LOCATION_TOP_BACK_LEFT         = 0x00010000,
    AUDIO_LOCATION_TOP_BACK_RIGHT        = 0x00020000,
    AUDIO_LOCATION_TOP_SIDE_LEFT         = 0x00040000,
    AUDIO_LOCATION_TOP_SIDE_RIGHT        = 0x00080000,
    AUDIO_LOCATION_TOP_BACK_CENTER       = 0x00100000,
    AUDIO_LOCATION_BOTTOM_FRONT_CENTER   = 0x00200000,
    AUDIO_LOCATION_BOTTOM_FRONT_LEFT     = 0x00400000,
    AUDIO_LOCATION_BOTTOM_FRONT_RIGHT    = 0x00800000,
    AUDIO_LOCATION_FRONT_LEFT_WIDE       = 0x01000000,
    AUDIO_LOCATION_FRONT_RIGHT_WIDE      = 0x02000000,
    AUDIO_LOCATION_LEFT_SURROUND         = 0x04000000,
    AUDIO_LOCATION_RIGHT_SURROUND        = 0x08000000,
    AUDIO_LOCATION_RESERVED_1            = 0x10000000,
    AUDIO_LOCATION_RESERVED_2            = 0x20000000,
    AUDIO_LOCATION_RESERVED_3            = 0x40000000,
    AUDIO_LOCATION_RESERVED_4            = 0x80000000,
};


int bt_base_populate_real_module_id(custom_block_t *blk, int32_t real_module_id);

int bt_base_populate_enc_bitrate(custom_block_t *blk, int32_t bit_rate);

int bt_base_populate_enc_output_cfg(custom_block_t *blk, uint32_t fmt_id,
                                    void *payload, size_t size);

int bt_base_populate_enc_cmn_param(custom_block_t *blk, uint32_t param_id,
                                   void *payload, size_t size);

#endif /* _BT_BASE_H_ */
