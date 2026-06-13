/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: SessionAlsaCompress"

#include "SessionAlsaCompress.h"
#include "SessionAlsaUtils.h"
#include "Stream.h"
#include "ResourceManager.h"
#include "media_fmt_api.h"
#include "gapless_api.h"
#include <agm/agm_api.h>
#include <sstream>
#include <mutex>
#include <fstream>
#include <agm/agm_api.h>

#define CHS_2 2
#define AACObjHE_PS 29

void SessionAlsaCompress::updateCodecOptions(
    pal_param_payload *param_payload, pal_stream_direction_t stream_direction) {

    if (stream_direction == PAL_AUDIO_OUTPUT) {
        pal_snd_dec_t *pal_snd_dec = nullptr;

        pal_snd_dec = (pal_snd_dec_t *)param_payload->payload;
        PAL_DBG(LOG_TAG, "playback compress format %x", audio_fmt);
        switch (audio_fmt) {
            case PAL_AUDIO_FMT_MP3:
            case PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_END:
            case PAL_AUDIO_FMT_AMR_NB:
            case PAL_AUDIO_FMT_AMR_WB:
            case PAL_AUDIO_FMT_AMR_WB_PLUS:
            case PAL_AUDIO_FMT_QCELP:
            case PAL_AUDIO_FMT_EVRC:
            case PAL_AUDIO_FMT_G711:
            break;
            case PAL_AUDIO_FMT_PCM_S16_LE:
            case PAL_AUDIO_FMT_PCM_S8:
            case PAL_AUDIO_FMT_PCM_S24_LE:
            case PAL_AUDIO_FMT_PCM_S24_3LE:
            case PAL_AUDIO_FMT_PCM_S32_LE:
            break;
            case PAL_AUDIO_FMT_COMPRESSED_RANGE_BEGIN:
            case PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_BEGIN:
            break;
            case PAL_AUDIO_FMT_AAC:
            {
                codec.format = SND_AUDIOSTREAMFORMAT_RAW;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->aac_dec.audio_obj_type;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->aac_dec.pce_bits_size;
                PAL_VERBOSE(LOG_TAG, "format- %x audio_obj_type- %x pce_bits_size- %x",
                            codec.format, codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1]);
            }
            break;
            case PAL_AUDIO_FMT_AAC_ADTS:
            {
                codec.format = SND_AUDIOSTREAMFORMAT_MP4ADTS;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->aac_dec.audio_obj_type;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->aac_dec.pce_bits_size;
                PAL_VERBOSE(LOG_TAG, "format- %x audio_obj_type- %x pce_bits_size- %x",
                            codec.format, codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1]);
            }
            break;
            case PAL_AUDIO_FMT_AAC_ADIF:
            {
                codec.format = SND_AUDIOSTREAMFORMAT_ADIF;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->aac_dec.audio_obj_type;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->aac_dec.pce_bits_size;
                PAL_VERBOSE(LOG_TAG, "format- %x audio_obj_type- %x pce_bits_size- %x",
                            codec.format, codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1]);
            }
            break;
            case PAL_AUDIO_FMT_AAC_LATM:
            {
                codec.format = SND_AUDIOSTREAMFORMAT_MP4LATM;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->aac_dec.audio_obj_type;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->aac_dec.pce_bits_size;
                PAL_VERBOSE(LOG_TAG, "format- %x audio_obj_type- %x pce_bits_size- %x",
                            codec.format, codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1]);
            }
            break;
            case PAL_AUDIO_FMT_WMA_STD:
            {
                codec.format = pal_snd_dec->wma_dec.fmt_tag;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->wma_dec.avg_bit_rate/8;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->wma_dec.super_block_align;
                codec.options.generic.reserved[2] =
                                        pal_snd_dec->wma_dec.bits_per_sample;
                codec.options.generic.reserved[3] =
                                        pal_snd_dec->wma_dec.channelmask;
                codec.options.generic.reserved[4] =
                                        pal_snd_dec->wma_dec.encodeopt;
                codec.options.generic.reserved[5] =
                                        pal_snd_dec->wma_dec.encodeopt1;
                codec.options.generic.reserved[6] =
                                        pal_snd_dec->wma_dec.encodeopt2;
                PAL_VERBOSE(LOG_TAG, "format- %x super_block_align- %x bits_per_sample- %x"
                            ", channelmask - %x \n", codec.format,
                            codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3]);
                PAL_VERBOSE(LOG_TAG, "encodeopt - %x, encodeopt1 - %x, encodeopt2 - %x"
                            ", avg_byte_rate - %x \n", codec.options.generic.reserved[4],
                            codec.options.generic.reserved[5], codec.options.generic.reserved[6],
                            codec.options.generic.reserved[0]);
            }
            break;
            case PAL_AUDIO_FMT_WMA_PRO:
            {
                codec.format = pal_snd_dec->wma_dec.fmt_tag;
                if (pal_snd_dec->wma_dec.fmt_tag == 0x161)
                    codec.profile = SND_AUDIOPROFILE_WMA9;
                else if (pal_snd_dec->wma_dec.fmt_tag == 0x162)
                    codec.profile = PAL_SND_PROFILE_WMA9_PRO;
                else if (pal_snd_dec->wma_dec.fmt_tag == 0x163)
                    codec.profile = PAL_SND_PROFILE_WMA9_LOSSLESS;
                else if (pal_snd_dec->wma_dec.fmt_tag == 0x166)
                    codec.profile = SND_AUDIOPROFILE_WMA10;
                else if (pal_snd_dec->wma_dec.fmt_tag == 0x167)
                    codec.profile = PAL_SND_PROFILE_WMA10_LOSSLESS;

                codec.options.generic.reserved[0] =
                                        pal_snd_dec->wma_dec.avg_bit_rate/8;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->wma_dec.super_block_align;
                codec.options.generic.reserved[2] =
                                        pal_snd_dec->wma_dec.bits_per_sample;
                codec.options.generic.reserved[3] =
                                        pal_snd_dec->wma_dec.channelmask;
                codec.options.generic.reserved[4] =
                                        pal_snd_dec->wma_dec.encodeopt;
                codec.options.generic.reserved[5] =
                                        pal_snd_dec->wma_dec.encodeopt1;
                codec.options.generic.reserved[6] =
                                        pal_snd_dec->wma_dec.encodeopt2;
                PAL_VERBOSE(LOG_TAG, "format- %x super_block_align- %x"
                            "bits_per_sample- %x channelmask- %x\n",
                            codec.format, codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3]);
                PAL_VERBOSE(LOG_TAG, "encodeopt- %x encodeopt1- %x"
                            "encodeopt2- %x avg_byte_rate- %x\n",
                            codec.options.generic.reserved[4],
                            codec.options.generic.reserved[5],
                            codec.options.generic.reserved[6],
                            codec.options.generic.reserved[0]);
            }
            break;
            case PAL_AUDIO_FMT_ALAC:
            {
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->alac_dec.frame_length;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->alac_dec.compatible_version;
                codec.options.generic.reserved[2] =
                                        pal_snd_dec->alac_dec.bit_depth;
                codec.options.generic.reserved[3] =
                                        pal_snd_dec->alac_dec.pb;
                codec.options.generic.reserved[4] =
                                        pal_snd_dec->alac_dec.mb;
                codec.options.generic.reserved[5] =
                                        pal_snd_dec->alac_dec.kb;
                codec.options.generic.reserved[6] =
                                        pal_snd_dec->alac_dec.max_run;
                codec.options.generic.reserved[7] =
                                    pal_snd_dec->alac_dec.max_frame_bytes;
                codec.options.generic.reserved[8] =
                                    pal_snd_dec->alac_dec.avg_bit_rate;
                codec.options.generic.reserved[9] =
                                    pal_snd_dec->alac_dec.channel_layout_tag;
                PAL_VERBOSE(LOG_TAG, "frame_length- %x compatible_version- %x"
                            "bit_depth- %x pb- %x mb- %x kb- %x",
                            codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3],
                            codec.options.generic.reserved[4],
                            codec.options.generic.reserved[5]);
                PAL_VERBOSE(LOG_TAG, "max_run- %x max_frame_bytes- %x avg_bit_rate- %x"
                            "channel_layout_tag- %x",
                            codec.options.generic.reserved[6],
                            codec.options.generic.reserved[7],
                            codec.options.generic.reserved[8],
                            codec.options.generic.reserved[9]);
            }
            break;
            case PAL_AUDIO_FMT_APE:
            {
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->ape_dec.bits_per_sample;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->ape_dec.compatible_version;
                codec.options.generic.reserved[2] =
                                        pal_snd_dec->ape_dec.compression_level;
                codec.options.generic.reserved[3] =
                                        pal_snd_dec->ape_dec.format_flags;
                codec.options.generic.reserved[4] =
                                        pal_snd_dec->ape_dec.blocks_per_frame;
                codec.options.generic.reserved[5] =
                                        pal_snd_dec->ape_dec.final_frame_blocks;
                codec.options.generic.reserved[6] =
                                        pal_snd_dec->ape_dec.total_frames;
                codec.options.generic.reserved[7] =
                                        pal_snd_dec->ape_dec.seek_table_present;
                PAL_VERBOSE(LOG_TAG, "compatible_version- %x compression_level- %x "
                            "format_flags- %x blocks_per_frame- %x "
                            "final_frame_blocks - %x",
                            codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3],
                            codec.options.generic.reserved[4],
                            codec.options.generic.reserved[5]);
                PAL_VERBOSE(LOG_TAG, "total_frames- %x bits_per_sample- %x"
                            " seek_table_present - %x",
                            codec.options.generic.reserved[6],
                            codec.options.generic.reserved[0],
                            codec.options.generic.reserved[7]);
            }
            break;
            case PAL_AUDIO_FMT_FLAC:
            {
                codec.format = SND_AUDIOSTREAMFORMAT_FLAC;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->flac_dec.sample_size;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->flac_dec.min_blk_size;
                codec.options.generic.reserved[2] =
                                        pal_snd_dec->flac_dec.max_blk_size;
                codec.options.generic.reserved[3] =
                                        pal_snd_dec->flac_dec.min_frame_size;
                codec.options.generic.reserved[4] =
                                        pal_snd_dec->flac_dec.max_frame_size;
                PAL_VERBOSE(LOG_TAG, "sample_size- %x min_blk_size- %x "
                            "max_blk_size- %x min_frame_size- %x "
                            "max_frame_size- %x",
                            codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3],
                            codec.options.generic.reserved[4]);
            }
            break;
            case PAL_AUDIO_FMT_FLAC_OGG:
            {
                codec.format = SND_AUDIOSTREAMFORMAT_FLAC_OGG;
                codec.options.generic.reserved[0] =
                                        pal_snd_dec->flac_dec.sample_size;
                codec.options.generic.reserved[1] =
                                        pal_snd_dec->flac_dec.min_blk_size;
                codec.options.generic.reserved[2] =
                                        pal_snd_dec->flac_dec.max_blk_size;
                codec.options.generic.reserved[3] =
                                        pal_snd_dec->flac_dec.min_frame_size;
                codec.options.generic.reserved[4] =
                                        pal_snd_dec->flac_dec.max_frame_size;
                PAL_VERBOSE(LOG_TAG, "sample_size- %x min_blk_size- %x "
                            "max_blk_size- %x min_frame_size- %x "
                            "max_frame_size - %x",
                            codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3],
                            codec.options.generic.reserved[4]);
            }
            break;
            case PAL_AUDIO_FMT_VORBIS:
                codec.format = pal_snd_dec->vorbis_dec.bit_stream_fmt;
            break;
            case PAL_AUDIO_FMT_OPUS:
                codec.format = SND_AUDIOCODEC_BESPOKE;
                //First codec option is OPUS identifier for the generic codec
                codec.options.generic.reserved[0] = AGM_FORMAT_OPUS;
                if (pal_snd_dec->opus_dec.num_channels <= 0) {
                        int channel_map; int channel_map2;
                        PAL_INFO(LOG_TAG, "OPUS metadata is not set; "
                                 "hardcoding the data.");
                        codec.options.generic.reserved[1] = 0;
                        codec.options.generic.reserved[2] = 0;
                        codec.options.generic.reserved[3] = 1;
                        codec.options.generic.reserved[4] = 2;
                        codec.options.generic.reserved[5] = 312;
                        codec.options.generic.reserved[6] =
                                                pal_snd_dec->opus_dec.sample_rate;
                        codec.options.generic.reserved[7] = 0;
                        codec.options.generic.reserved[8] = 0;
                        codec.options.generic.reserved[9] = 1;
                        codec.options.generic.reserved[10] = 1;
                        channel_map = 256; //Channel map default values are 0 1
                        channel_map2 = 641;
                        memcpy(&codec.options.generic.reserved[11],
                                &channel_map, 4);
                        memcpy(&codec.options.generic.reserved[12],
                                &channel_map2, 4);
                }
                else {
                        codec.options.generic.reserved[1] =
                                                pal_snd_dec->opus_dec.bitstream_format;
                        codec.options.generic.reserved[2] =
                                                pal_snd_dec->opus_dec.payload_type;
                        codec.options.generic.reserved[3] =
                                                pal_snd_dec->opus_dec.version;
                        codec.options.generic.reserved[4] =
                                                pal_snd_dec->opus_dec.num_channels;
                        codec.options.generic.reserved[5] =
                                                pal_snd_dec->opus_dec.pre_skip;
                        codec.options.generic.reserved[6] =
                                                pal_snd_dec->opus_dec.sample_rate;
                        codec.options.generic.reserved[7] =
                                                pal_snd_dec->opus_dec.output_gain;
                        codec.options.generic.reserved[8] =
                                                pal_snd_dec->opus_dec.mapping_family;
                        codec.options.generic.reserved[9] =
                                                pal_snd_dec->opus_dec.stream_count;
                        codec.options.generic.reserved[10] =
                                                pal_snd_dec->opus_dec.coupled_count;
                        memcpy(&codec.options.generic.reserved[11],
                                &pal_snd_dec->opus_dec.channel_map[0], 4);
                        memcpy(&codec.options.generic.reserved[12],
                                &pal_snd_dec->opus_dec.channel_map[4], 4);
                }
                PAL_VERBOSE(LOG_TAG, "format- %d bitstream- 0x%x payload 0x%x version- 0x%x "
                            "num_channels- 0x%x pre_skip- 0x%x "
                            "sample_rate- 0x%x output_gain- 0x%x",
                            codec.options.generic.reserved[0],
                            codec.options.generic.reserved[1],
                            codec.options.generic.reserved[2],
                            codec.options.generic.reserved[3],
                            codec.options.generic.reserved[4],
                            codec.options.generic.reserved[5],
                            codec.options.generic.reserved[6],
                            codec.options.generic.reserved[7]);
                PAL_VERBOSE(LOG_TAG, "mapping_family- 0x%x stream_count- 0x%x"
                            "coupled_count- 0x%x channel_map- %d %d",
                            codec.options.generic.reserved[8],
                            codec.options.generic.reserved[9],
                            codec.options.generic.reserved[10],
                            codec.options.generic.reserved[11],
                            codec.options.generic.reserved[12]);
            break;
            default:
                PAL_ERR(LOG_TAG, "Entered default, format %x", audio_fmt);
            break;
        }
    }

    if (stream_direction == PAL_AUDIO_INPUT) {
        pal_snd_enc_t *pal_snd_enc = nullptr;
        PAL_DBG(LOG_TAG, "capture compress format %x", audio_fmt);
        pal_snd_enc = (pal_snd_enc_t *)param_payload->payload;
        switch (audio_fmt) {
            case PAL_AUDIO_FMT_AAC:
            case PAL_AUDIO_FMT_AAC_ADTS:
            case PAL_AUDIO_FMT_AAC_ADIF:
            case PAL_AUDIO_FMT_AAC_LATM: {
                codec.format = pal_snd_enc->aac_enc.enc_cfg.aac_fmt_flag;
                codec.profile = pal_snd_enc->aac_enc.enc_cfg.aac_enc_mode;
                codec.bit_rate = pal_snd_enc->aac_enc.aac_bit_rate;
                codec.rate_control = pal_snd_enc->aac_enc.global_cutoff_freq;
                PAL_DBG(LOG_TAG,
                        "requested AAC encode mode: %x, AAC format flag: %x, "
                        "AAC "
                        "bit rate: %d, global cut off frequency: %d",
                        codec.profile, codec.format, codec.bit_rate,
                        codec.rate_control);
                break;
            }
            default:
                break;
        }
    }

}

void SessionAlsaCompress::getSndCodecParam(struct snd_codec &codec, struct pal_stream_attributes &sAttr)
{
    struct pal_media_config *config = nullptr;
    PAL_DBG(LOG_TAG, "Enter");
    switch (sAttr.direction) {
        case PAL_AUDIO_OUTPUT:
            PAL_DBG(LOG_TAG, "playback codec setting");
            config = &sAttr.out_media_config;
            codec.id = getSndCodecId(config->aud_fmt_id);
            codec.ch_in = config->ch_info.channels;
            codec.ch_out = codec.ch_in;
            codec.sample_rate = config->sample_rate;
            codec.bit_rate = config->bit_width;
            break;

        case PAL_AUDIO_INPUT:
            PAL_VERBOSE(LOG_TAG, "capture codec setting");
            config = &sAttr.in_media_config;
            codec.id = getSndCodecId(config->aud_fmt_id);
            codec.ch_in = config->ch_info.channels;
            codec.ch_out = codec.ch_in;
            codec.sample_rate = config->sample_rate;
            break;

        case PAL_AUDIO_INPUT_OUTPUT:
            break;
        default:
            break;
    }
    PAL_DBG(LOG_TAG, "Exit");
}

int SessionAlsaCompress::getSndCodecId(pal_audio_fmt_t fmt)
{
    int id = -1;

    switch (fmt) {
        case PAL_AUDIO_FMT_MP3:
            id = SND_AUDIOCODEC_MP3;
            break;
        case PAL_AUDIO_FMT_AMR_NB:
            id = SND_AUDIOCODEC_AMR;
            break;
        case PAL_AUDIO_FMT_AMR_WB:
            id = SND_AUDIOCODEC_AMRWB;
            break;
        case PAL_AUDIO_FMT_AMR_WB_PLUS:
            id = SND_AUDIOCODEC_AMRWBPLUS;
            break;
        case PAL_AUDIO_FMT_QCELP:
        case PAL_AUDIO_FMT_EVRC:
        case PAL_AUDIO_FMT_G711:
             id = -1;
            break;
        case PAL_AUDIO_FMT_COMPRESSED_RANGE_BEGIN:
        case PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_BEGIN:
        case PAL_AUDIO_FMT_COMPRESSED_EXTENDED_RANGE_END:
            id = -1;
            break;
        case PAL_AUDIO_FMT_AAC:
        case PAL_AUDIO_FMT_AAC_ADTS:
        case PAL_AUDIO_FMT_AAC_ADIF:
        case PAL_AUDIO_FMT_AAC_LATM:
            id = SND_AUDIOCODEC_AAC;
            break;
        case PAL_AUDIO_FMT_WMA_STD:
            id = SND_AUDIOCODEC_WMA;
            break;
        case PAL_AUDIO_FMT_PCM_S8:
        case PAL_AUDIO_FMT_PCM_S16_LE:
        case PAL_AUDIO_FMT_PCM_S24_3LE:
        case PAL_AUDIO_FMT_PCM_S24_LE:
        case PAL_AUDIO_FMT_PCM_S32_LE:
            id = SND_AUDIOCODEC_PCM;
            break;
        case PAL_AUDIO_FMT_ALAC:
            id = SND_AUDIOCODEC_ALAC;
            break;
        case PAL_AUDIO_FMT_APE:
            id = SND_AUDIOCODEC_APE;
            break;
        case PAL_AUDIO_FMT_WMA_PRO:
            id = SND_AUDIOCODEC_WMA;
            break;
        case PAL_AUDIO_FMT_FLAC:
        case PAL_AUDIO_FMT_FLAC_OGG:
            id = SND_AUDIOCODEC_FLAC;
            break;
        case PAL_AUDIO_FMT_VORBIS:
            id = SND_AUDIOCODEC_VORBIS;
            break;
        case PAL_AUDIO_FMT_OPUS:
            id = SND_AUDIOCODEC_BESPOKE;
            break;
        default:
            PAL_ERR(LOG_TAG, "Entered default format %x", fmt);
            break;
    }

    return id;
}

bool SessionAlsaCompress::isGaplessFormat(pal_audio_fmt_t fmt)
{
    bool isSupported = false;

    /* If platform doesn't support Gapless,
     * then return false for all formats.
     */
    if (!(rm->isGaplessEnabled)) {
        return isSupported;
    }
    switch (fmt) {
        case PAL_AUDIO_FMT_DEFAULT_COMPRESSED:
        case PAL_AUDIO_FMT_AAC:
        case PAL_AUDIO_FMT_AAC_ADTS:
        case PAL_AUDIO_FMT_AAC_ADIF:
        case PAL_AUDIO_FMT_AAC_LATM:
        case PAL_AUDIO_FMT_VORBIS:
        case PAL_AUDIO_FMT_FLAC:
        case PAL_AUDIO_FMT_WMA_PRO:
        case PAL_AUDIO_FMT_APE:
        case PAL_AUDIO_FMT_WMA_STD:
        case PAL_AUDIO_FMT_OPUS:
            isSupported = true;
            break;
        case PAL_AUDIO_FMT_PCM_S8:
        case PAL_AUDIO_FMT_PCM_S16_LE:
        case PAL_AUDIO_FMT_PCM_S24_3LE:
        case PAL_AUDIO_FMT_PCM_S24_LE:
        case PAL_AUDIO_FMT_PCM_S32_LE:
            break;
        case PAL_AUDIO_FMT_ALAC:
            break;
        case PAL_AUDIO_FMT_FLAC_OGG:
            break;
        default:
            break;
    }
    PAL_DBG(LOG_TAG, "format %x, gapless supported %d", audio_fmt,
                      isSupported);
    return isSupported;
}

bool SessionAlsaCompress::isCodecConfigNeeded(
    pal_audio_fmt_t audio_fmt, pal_stream_direction_t stream_direction) {
    bool ret = false;
    if (stream_direction == PAL_AUDIO_OUTPUT) {
        switch (audio_fmt) {
            case PAL_AUDIO_FMT_VORBIS:
            case PAL_AUDIO_FMT_FLAC:
            case PAL_AUDIO_FMT_WMA_PRO:
            case PAL_AUDIO_FMT_APE:
            case PAL_AUDIO_FMT_WMA_STD:
            case PAL_AUDIO_FMT_OPUS:
                ret = true;
                break;
            case PAL_AUDIO_FMT_DEFAULT_COMPRESSED:
            case PAL_AUDIO_FMT_AAC:
            case PAL_AUDIO_FMT_AAC_ADTS:
            case PAL_AUDIO_FMT_AAC_ADIF:
            case PAL_AUDIO_FMT_AAC_LATM:
            case PAL_AUDIO_FMT_PCM_S8:
            case PAL_AUDIO_FMT_PCM_S16_LE:
            case PAL_AUDIO_FMT_PCM_S24_3LE:
            case PAL_AUDIO_FMT_PCM_S24_LE:
            case PAL_AUDIO_FMT_PCM_S32_LE:
            case PAL_AUDIO_FMT_ALAC:
            case PAL_AUDIO_FMT_FLAC_OGG:
                break;
            default:
                break;
        }
    }
    if (stream_direction == PAL_AUDIO_INPUT) {
        switch (audio_fmt) {
            case PAL_AUDIO_FMT_AAC:
            case PAL_AUDIO_FMT_AAC_ADTS:
            case PAL_AUDIO_FMT_AAC_ADIF:
            case PAL_AUDIO_FMT_AAC_LATM:
                ret = true;
                break;
            default:
                break;
        }
    }

    PAL_DBG(LOG_TAG, "format %x, need to send codec config %d", audio_fmt,
                      ret);
    return ret;
}

int SessionAlsaCompress::setCustomFormatParam(pal_audio_fmt_t audio_fmt)
{
    int32_t status = 0;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    uint32_t miid;
    struct pal_stream_attributes sAttr = {};
    struct media_format_t *media_fmt_hdr = nullptr;
    struct agm_buff buffer = {0, 0, 0, NULL, 0, NULL, {0, 0, 0}};

    if (audio_fmt == PAL_AUDIO_FMT_VORBIS) {
        payload_media_fmt_vorbis_t* media_fmt_vorbis = NULL;
        // set config for vorbis, as it cannot be upstreamed.
        if (!compressDevIds.size()) {
            PAL_ERR(LOG_TAG, "No compressDevIds found");
            status = -EINVAL;
            return status;
        }
        status = SessionAlsaUtils::getModuleInstanceId(mixer,
                    compressDevIds.at(0), rxAifBackEnds[0].second.data(),
                    STREAM_INPUT_MEDIA_FORMAT, &miid);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
            return status;
        }
        media_fmt_hdr = (struct media_format_t *)
                            malloc(sizeof(struct media_format_t)
                                + sizeof(struct pal_snd_dec_vorbis));
        if (!media_fmt_hdr) {
            PAL_ERR(LOG_TAG, "failed to allocate memory");
            return -ENOMEM;
        }
        media_fmt_hdr->data_format = DATA_FORMAT_RAW_COMPRESSED ;
        media_fmt_hdr->fmt_id = MEDIA_FMT_ID_VORBIS;
        media_fmt_hdr->payload_size = sizeof(struct pal_snd_dec_vorbis);
        media_fmt_vorbis = (payload_media_fmt_vorbis_t*)(((uint8_t*)media_fmt_hdr) +
            sizeof(struct media_format_t));

        ar_mem_cpy(media_fmt_vorbis,
                            sizeof(struct pal_snd_dec_vorbis),
                            &codec.format,
                            sizeof(struct pal_snd_dec_vorbis));
        if (sendNextTrackParams) {
            PAL_DBG(LOG_TAG, "sending next track param on datapath");
            buffer.timestamp = 0x0;
            buffer.flags = AGM_BUFF_FLAG_MEDIA_FORMAT;
            buffer.size = sizeof(struct media_format_t) +
                          sizeof(struct pal_snd_dec_vorbis);
            buffer.addr = (uint8_t *)media_fmt_hdr;
            payload = (uint8_t *)&buffer;
            payloadSize = sizeof(struct agm_buff);
            status = SessionAlsaUtils::mixerWriteDatapathParams(mixer,
                        compressDevIds.at(0), payload, payloadSize);
            free(media_fmt_hdr);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "mixerWriteWithMetadata failed %d", status);
                return status;
            }
            sendNextTrackParams = false;
        } else {
            status = builder->payloadCustomParam(&payload, &payloadSize,
                                        (uint32_t *)media_fmt_hdr,
                                        sizeof(struct media_format_t) +
                                        sizeof(struct pal_snd_dec_vorbis),
                                        miid, PARAM_ID_MEDIA_FORMAT);
            free(media_fmt_hdr);
            if (status) {
                PAL_ERR(LOG_TAG, "payloadCustomParam failed status = %d", status);
                return status;
            }
            status = SessionAlsaUtils::setMixerParameter(mixer,
                            compressDevIds.at(0), payload, payloadSize);
            freeCustomPayload(&payload, &payloadSize);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "setMixerParameter failed");
                return status;
            }
        }
    }
    return status;
}

void SessionAlsaCompress::offloadThreadLoop(SessionAlsaCompress* compressObj)
{
    std::shared_ptr<offload_msg> msg;
    uint32_t event_id = 0;
    int ret = 0;
    bool is_drain_called = false;
    std::unique_lock<std::mutex> lock(compressObj->cv_mutex_);

    while (1) {
        if (compressObj->msg_queue_.empty())
            compressObj->cv_.wait(lock);  /* wait for incoming requests */

        if (!compressObj->msg_queue_.empty()) {
            msg = compressObj->msg_queue_.front();
            compressObj->msg_queue_.pop();
            lock.unlock();

            if (msg && msg->cmd)
                compressObj->command = msg->cmd;

            if (msg && msg->cmd == OFFLOAD_CMD_EXIT)
                break; // exit the thread

            if (msg && msg->cmd == OFFLOAD_CMD_WAIT_FOR_BUFFER) {
                if (compressObj->rm->cardState == CARD_STATUS_ONLINE) {
                    PAL_VERBOSE(LOG_TAG, "calling compress_wait");
                    ret = compress_wait(compressObj->compress, -1);
                    PAL_VERBOSE(LOG_TAG, "out of compress_wait, ret %d", ret);
                    event_id = PAL_STREAM_CBK_EVENT_WRITE_READY;
                    compressObj->command = OFFLOAD_CMD_EXIT;
                }
                if (ret && (errno == -ECANCELED || errno == -ENETRESET)) {
                    PAL_INFO(LOG_TAG, "No need of sending callback during SSR or internal unblock");
                    lock.lock();
                    continue;
                }
            } else if (msg && msg->cmd == OFFLOAD_CMD_DRAIN) {
                if (!is_drain_called) {
                    PAL_INFO(LOG_TAG, "calling compress_drain");
                    if (compressObj->rm->cardState == CARD_STATUS_ONLINE &&
                        compressObj->compress != NULL) {
                         ret = compress_drain(compressObj->compress);
                         PAL_INFO(LOG_TAG, "out of compress_drain, ret %d", ret);
                    }
                }
                if (ret && (errno == -ECANCELED || errno == -ENETRESET)) {
                    PAL_INFO(LOG_TAG, "No need of sending callback during SSR or internal unblock");
                    lock.lock();
                    continue;
                }
                is_drain_called = false;
                event_id = PAL_STREAM_CBK_EVENT_DRAIN_READY;
            } else if (msg && msg->cmd == OFFLOAD_CMD_PARTIAL_DRAIN) {
                if (compressObj->rm->cardState == CARD_STATUS_ONLINE &&
                        compressObj->compress != NULL) {
                    if (compressObj->isGaplessFmt) {
                        PAL_DBG(LOG_TAG, "calling partial compress_drain");
                        ret = compress_next_track(compressObj->compress);
                        PAL_INFO(LOG_TAG, "out of compress next track, ret %d", ret);
                        if (ret == 0) {
                            ret = compress_partial_drain(compressObj->compress);
                            PAL_INFO(LOG_TAG, "out of partial compress_drain, ret %d", ret);
                        }
                        event_id = PAL_STREAM_CBK_EVENT_PARTIAL_DRAIN_READY;
                    } else {
                        PAL_DBG(LOG_TAG, "calling compress_drain");
                        ret = compress_drain(compressObj->compress);
                        PAL_INFO(LOG_TAG, "out of compress_drain, ret %d", ret);
                        is_drain_called = true;
                        event_id = PAL_STREAM_CBK_EVENT_DRAIN_READY;
                    }
                }
                if (ret && (errno == -ECANCELED || errno == -ENETRESET)) {
                    PAL_INFO(LOG_TAG, "No need of sending callback during SSR or internal unblock");
                    lock.lock();
                    continue;
                }
            }  else if (msg && msg->cmd == OFFLOAD_CMD_ERROR) {
                PAL_ERR(LOG_TAG, "Sending error to PAL client");
                event_id = PAL_STREAM_CBK_EVENT_ERROR;
            }
            if (compressObj->sessionCb)
                compressObj->sessionCb(compressObj->cbCookie, event_id, (void*)NULL, 0);

            lock.lock();
        }

    }
    PAL_DBG(LOG_TAG, "exit offloadThreadLoop");
}

SessionAlsaCompress::SessionAlsaCompress(std::shared_ptr<ResourceManager> Rm)
{
    rm = Rm;
    builder = new PayloadBuilder();

    /** set default snd codec params */
    codec.id = getSndCodecId(PAL_AUDIO_FMT_PCM_S16_LE);
    codec.ch_in = 2;
    codec.ch_out = codec.ch_in;
    codec.sample_rate = 48000;
    customPayload = NULL;
    customPayloadSize = 0;
    compress = NULL;
    sessionCb = NULL;
    this->cbCookie = 0;
    playback_started = false;
    capture_started = false;
    playback_paused = false;
    capture_paused = false;
    streamHandle = NULL;
    ecRefDevId = PAL_DEVICE_OUT_MIN;
#ifdef SEC_AUDIO_MUTE_DETECTION
    isCompressStart = false;
#endif
}

SessionAlsaCompress::~SessionAlsaCompress()
{
    delete builder;
    compressDevIds.clear();
}

int SessionAlsaCompress::open(Stream * s)
{
    int status = -EINVAL;
    struct pal_stream_attributes sAttr = {};
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::vector<std::pair<int32_t, std::string>> emptyBackEnds;

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    streamHandle = s;
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }

    status = s->getAssociatedDevices(associatedDevices);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
        goto exit;
    }

    compressDevIds = rm->allocateFrontEndIds(sAttr, 0);
    if (compressDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "no more FE vailable");
        return -EINVAL;
    }
    frontEndIdAllocated = true;

    for (int i = 0; i < compressDevIds.size(); i++) {
        PAL_DBG(LOG_TAG, "dev id size %zu, compressDevIds[%d] %d", compressDevIds.size(), i, compressDevIds[i]);
    }
    rm->getBackEndNames(associatedDevices, rxAifBackEnds, txAifBackEnds);
    if (rxAifBackEnds.empty() && txAifBackEnds.empty()) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "no backend specified for this stream");
        goto exit;
    }
    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG, "mixer error");
        goto exit;
    }

    switch(sAttr.direction){
        case PAL_AUDIO_OUTPUT:
            ioMode = sAttr.flags & PAL_STREAM_FLAG_NON_BLOCKING_MASK;
            if (!ioMode) {
                PAL_ERR(LOG_TAG, "IO mode 0x%x not supported", ioMode);
                status = -EINVAL;
                goto exit;
            }
            status =
                SessionAlsaUtils::open(s, rm, compressDevIds, rxAifBackEnds);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa utils open failed with %d",
                        status);
                rm->freeFrontEndIds(compressDevIds, sAttr, 0);
                frontEndIdAllocated = false;
            }
            audio_fmt = sAttr.out_media_config.aud_fmt_id;
            isGaplessFmt = isGaplessFormat(audio_fmt);

            // Register for  mixer event callback
            status = rm->registerMixerEventCallback(compressDevIds, sessionCb, cbCookie,
                            true);
            if (status == 0) {
                isMixerEventCbRegd = true;
            } else {
                // Not a fatal error. Only pop noise will come for Soft pause use case
                PAL_ERR(LOG_TAG, "Failed to register callback to rm");
                status = 0;
            }
            break;

        case PAL_AUDIO_INPUT:
            status =
                SessionAlsaUtils::open(s, rm, compressDevIds, txAifBackEnds);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa utils open failed with %d",
                        status);
                rm->freeFrontEndIds(compressDevIds, sAttr, 0);
                frontEndIdAllocated = false;
            }
            audio_fmt = sAttr.in_media_config.aud_fmt_id;
            break;

        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid direction");
            break;
    }

exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToDisconnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    struct pal_device dAttr = {};
    std::vector<std::pair<int32_t, std::string>> rxAifBackEndsToDisconnect;
    std::vector<std::pair<int32_t, std::string>> txAifBackEndsToDisconnect;
    int32_t status = 0;

    deviceList.push_back(deviceToDisconnect);
    rm->getBackEndNames(deviceList, rxAifBackEndsToDisconnect,
            txAifBackEndsToDisconnect);
    deviceToDisconnect->getDeviceAttributes(&dAttr);

    if (!rxAifBackEndsToDisconnect.empty()) {
        int cnt = 0;

        status = SessionAlsaUtils::disconnectSessionDevice(streamHandle, streamType, rm,
            dAttr, compressDevIds, rxAifBackEndsToDisconnect);

        for (const auto &elem : rxAifBackEnds) {
            cnt++;
            for (const auto &disConnectElem : rxAifBackEndsToDisconnect) {
                if (std::get<0>(elem) == std::get<0>(disConnectElem)) {
                    PAL_DBG(LOG_TAG, "Removed BE for dev %d from AIF list", std::get<0>(disConnectElem));
                    rxAifBackEnds.erase(rxAifBackEnds.begin() + cnt - 1, rxAifBackEnds.begin() + cnt);
                    cnt--;
                    break;
                } else if (&disConnectElem == &rxAifBackEndsToDisconnect.back()) {
                    PAL_ERR(LOG_TAG, "Failed to remove BE for dev %d from AIF list", std::get<0>(disConnectElem));
                }
            }
        }
    }

    if (!txAifBackEndsToDisconnect.empty()) {
        int cnt = 0;

        status = SessionAlsaUtils::disconnectSessionDevice(streamHandle, streamType, rm,
            dAttr, compressDevIds, txAifBackEndsToDisconnect);

        for (const auto &elem : txAifBackEnds) {
            cnt++;
            for (const auto &disConnectElem : txAifBackEndsToDisconnect) {
                if (std::get<0>(elem) == std::get<0>(disConnectElem)) {
                    PAL_DBG(LOG_TAG, "Removed BE for dev %d from AIF list", std::get<0>(disConnectElem));
                    txAifBackEnds.erase(txAifBackEnds.begin() + cnt - 1, txAifBackEnds.begin() + cnt);
                    cnt--;
                    break;
                } else if (&disConnectElem == &txAifBackEndsToDisconnect.back()) {
                    PAL_ERR(LOG_TAG, "Failed to remove BE for dev %d from AIF list", std::get<0>(disConnectElem));
                }
            }
        }
    }

    return status;
}

int SessionAlsaCompress::setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    struct pal_device dAttr = {};
    std::vector<std::pair<int32_t, std::string>> rxAifBackEndsToConnect;
    std::vector<std::pair<int32_t, std::string>> txAifBackEndsToConnect;
    int32_t status = 0;

    deviceList.push_back(deviceToConnect);
    rm->getBackEndNames(deviceList, rxAifBackEndsToConnect,
            txAifBackEndsToConnect);
    deviceToConnect->getDeviceAttributes(&dAttr);

    if (!rxAifBackEndsToConnect.empty())
        status = SessionAlsaUtils::setupSessionDevice(streamHandle, streamType, rm,
            dAttr, compressDevIds, rxAifBackEndsToConnect);

    if (!txAifBackEndsToConnect.empty())
        status = SessionAlsaUtils::setupSessionDevice(streamHandle, streamType, rm,
            dAttr, compressDevIds, txAifBackEndsToConnect);

    return status;
}

int SessionAlsaCompress::connectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    struct pal_device dAttr;
    std::vector<std::pair<int32_t, std::string>> rxAifBackEndsToConnect;
    std::vector<std::pair<int32_t, std::string>> txAifBackEndsToConnect;
    int32_t status = 0;

    deviceList.push_back(deviceToConnect);
    rm->getBackEndNames(deviceList, rxAifBackEndsToConnect,
            txAifBackEndsToConnect);
    deviceToConnect->getDeviceAttributes(&dAttr);

    if (!rxAifBackEndsToConnect.empty()) {
        for (const auto &elem : rxAifBackEndsToConnect)
            rxAifBackEnds.push_back(elem);
        status = SessionAlsaUtils::connectSessionDevice(this, streamHandle, streamType, rm,
            dAttr, compressDevIds, rxAifBackEndsToConnect);
        if (status) {
            int cnt = 0;
            for (const auto &elem : rxAifBackEnds) {
                cnt++;
                for (const auto &connectElem : rxAifBackEndsToConnect) {
                    if (std::get<0>(elem) == std::get<0>(connectElem)) {
                        rxAifBackEnds.erase(rxAifBackEnds.begin() + cnt - 1, rxAifBackEnds.begin() + cnt);
                        cnt--;
                        break;
                    }
                }
            }
        }
    }

    if (!txAifBackEndsToConnect.empty()) {
        for (const auto &elem : txAifBackEndsToConnect)
            txAifBackEnds.push_back(elem);
        status = SessionAlsaUtils::connectSessionDevice(this, streamHandle, streamType, rm,
            dAttr, compressDevIds, txAifBackEndsToConnect);
        if (status) {
            int cnt = 0;
            for (const auto &elem : txAifBackEnds) {
                cnt++;
                for (const auto &connectElem : txAifBackEndsToConnect) {
                    if (std::get<0>(elem) == std::get<0>(connectElem)) {
                        txAifBackEnds.erase(txAifBackEnds.begin() + cnt - 1, txAifBackEnds.begin() + cnt);
                        cnt--;
                        break;
                    }
                }
            }
        }

    }

    return status;
}

int SessionAlsaCompress::prepare(Stream * s __unused)
{
   return 0;
}

int SessionAlsaCompress::setTKV(Stream * s __unused, configType type, effect_pal_payload_t *effectPayload)
{
    int status = 0;
    uint32_t tagsent;
    struct agm_tag_config* tagConfig = nullptr;
    const char *setParamTagControl = "setParamTag";
    const char *stream = "COMPRESS";
    struct mixer_ctl *ctl;
    std::ostringstream tagCntrlName;
    int tkv_size = 0;

    switch (type) {
        case MODULE:
        {
            if (!compressDevIds.size()) {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            tkv.clear();
            pal_key_vector_t *pal_kvpair = (pal_key_vector_t *)effectPayload->payload;
            uint32_t num_tkvs = pal_kvpair->num_tkvs;
            for (uint32_t i = 0; i < num_tkvs; i++) {
                tkv.push_back(std::make_pair(pal_kvpair->kvp[i].key, pal_kvpair->kvp[i].value));
            }

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }

            tagConfig = (struct agm_tag_config*)malloc (sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));

            if (!tagConfig) {
                status = -ENOMEM;
                goto exit;
            }

            tagsent = effectPayload->tag;
            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }
            tagCntrlName<<stream<<compressDevIds.at(0)<<" "<<setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                status = -ENOENT;
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "mixer control: %s\n", tagCntrlName.str().data());

            tkv_size = tkv.size()*sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
                goto exit;
            }
            ctl = NULL;
            tkv.clear();

            break;
        }
        default:
            PAL_ERR(LOG_TAG, "invalid type ");
            status = -EINVAL;
            goto exit;
    }

exit:
    PAL_DBG(LOG_TAG, "exit status:%d ", status);
    if (tagConfig) {
        free(tagConfig);
        tagConfig = nullptr;
    }

    return status;
}

int SessionAlsaCompress::setConfig(Stream * s, configType type, uint32_t tag1,
        uint32_t tag2, uint32_t tag3)
{
    int status = 0;
    uint32_t tagsent = 0;
    struct agm_tag_config* tagConfig = nullptr;
    std::ostringstream tagCntrlName;
    char const *stream = "COMPRESS";
    const char *setParamTagControl = "setParamTag";
    struct mixer_ctl *ctl = nullptr;
    uint32_t tkv_size = 0;

    PAL_DBG(LOG_TAG, "Enter");
    switch (type) {
        case MODULE:
            if (!compressDevIds.size()) {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            tkv.clear();
            if (tag1)
                builder->populateTagKeyVector(s, tkv, tag1, &tagsent);
            if (tag2)
                builder->populateTagKeyVector(s, tkv, tag2, &tagsent);
            if (tag3)
                builder->populateTagKeyVector(s, tkv, tag3, &tagsent);

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }
            tagConfig = (struct agm_tag_config*)malloc (sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));
            if (!tagConfig) {
                status = -ENOMEM;
                goto exit;
            }
            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }
            tagCntrlName << stream << compressDevIds.at(0) << " " << setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                status = -ENOENT;
                goto exit;
            }

            tkv_size = tkv.size() * sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
                goto exit;
            }
            ctl = NULL;
            tkv.clear();
            break;
        default:
            status = 0;
            break;
    }

exit:
    if(tagConfig)
        free(tagConfig);
    PAL_DBG(LOG_TAG, "exit status:%d ", status);
    return status;
}

int SessionAlsaCompress::setConfig(Stream * s, configType type, int tag)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    uint32_t tagsent;
    struct agm_tag_config* tagConfig = nullptr;
    const char *setParamTagControl = "setParamTag";
    const char *stream = "COMPRESS";
    const char *setCalibrationControl = "setCalibration";
    const char *setBEControl = "control";
    struct mixer_ctl *ctl;
    struct agm_cal_config *calConfig = nullptr;
    std::ostringstream beCntrlName;
    std::ostringstream tagCntrlName;
    std::ostringstream calCntrlName;
    int tkv_size = 0;
    int ckv_size = 0;

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        return -EINVAL;
    }

    if ((sAttr.direction == PAL_AUDIO_OUTPUT && rxAifBackEnds.empty()) ||
        (sAttr.direction == PAL_AUDIO_INPUT && txAifBackEnds.empty())) {
        PAL_ERR(LOG_TAG, "No backend connected to this stream\n");
        return -EINVAL;
    }

    if (!compressDevIds.size()) {
        PAL_ERR(LOG_TAG, "No compressDevIds found");
        status = -EINVAL;
        goto exit;
    }
    beCntrlName<<stream<<compressDevIds.at(0)<<" "<<setBEControl;

    ctl = mixer_get_ctl_by_name(mixer, beCntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
        return -ENOENT;
    }
    mixer_ctl_set_enum_by_string(ctl, (sAttr.direction == PAL_AUDIO_OUTPUT) ?
                                 rxAifBackEnds[0].second.data() : txAifBackEnds[0].second.data());

    switch (type) {
        case MODULE:
            tkv.clear();
            status = builder->populateTagKeyVector(s, tkv, tag, &tagsent);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to set the tag configuration\n");
                goto exit;
            }

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }

            tagConfig = (struct agm_tag_config*)malloc (sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));

            if (!tagConfig) {
                status = -ENOMEM;
                goto exit;
            }

            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                free(tagConfig);
                goto exit;
            }
            //TODO: how to get the id '5'
            tagCntrlName<<stream<<compressDevIds.at(0)<<" "<<setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                if (tagConfig)
                    free(tagConfig);
                return -ENOENT;
            }
            PAL_VERBOSE(LOG_TAG, "mixer control: %s\n", tagCntrlName.str().data());

            tkv_size = tkv.size()*sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
            }
            ctl = NULL;
            if (tagConfig)
                free(tagConfig);
            tkv.clear();
            goto exit;
            //todo calibration
        case CALIBRATION:
            kvMutex.lock();
            ckv.clear();
            status = builder->populateCalKeyVector(s, ckv, tag);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to set the calibration data\n");
                goto unlock_kvMutex;
            }
            if (ckv.size() == 0) {
                status = -EINVAL;
                goto unlock_kvMutex;
            }

            calConfig = (struct agm_cal_config*)malloc (sizeof(struct agm_cal_config) +
                            (ckv.size() * sizeof(agm_key_value)));
            if (!calConfig) {
                status = -EINVAL;
                goto unlock_kvMutex;
            }
            status = SessionAlsaUtils::getCalMetadata(ckv, calConfig);
            //TODO: how to get the id '0'
            calCntrlName<<stream<<compressDevIds.at(0)<<" "<<setCalibrationControl;
            ctl = mixer_get_ctl_by_name(mixer, calCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", calCntrlName.str().data());
                status = -ENOENT;
                goto unlock_kvMutex;
            }
            PAL_VERBOSE(LOG_TAG, "mixer control: %s\n", calCntrlName.str().data());
            ckv_size = ckv.size()*sizeof(struct agm_key_value);
            //TODO make struct mixer and struct pcm as class private variables.
            status = mixer_ctl_set_array(ctl, calConfig, sizeof(struct agm_cal_config) + ckv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
            }
            ctl = NULL;
            ckv.clear();
            break;
        default:
            PAL_ERR(LOG_TAG, "invalid type ");
            status = -EINVAL;
            goto exit;
    }
unlock_kvMutex:
    if (calConfig)
        free(calConfig);
    kvMutex.unlock();
exit:
    PAL_DBG(LOG_TAG, "exit status:%d ", status);
    return status;
}

int SessionAlsaCompress::configureEarlyEOSDelay(void)
{
    int32_t status = 0;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    uint32_t miid;

    PAL_DBG(LOG_TAG, "Enter");
    status = SessionAlsaUtils::getModuleInstanceId(mixer,
                    compressDevIds.at(0), rxAifBackEnds[0].second.data(),
                    MODULE_GAPLESS, &miid);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
        return status;
    }
    param_id_gapless_early_eos_delay_t  *early_eos_delay = nullptr;
    early_eos_delay = (struct param_id_gapless_early_eos_delay_t *)
                            malloc(sizeof(struct param_id_gapless_early_eos_delay_t));
    if (!early_eos_delay) {
        PAL_ERR(LOG_TAG, "failed to allocate memory");
        return -ENOMEM;
    }
    early_eos_delay->early_eos_delay_ms = EARLY_EOS_DELAY_MS;

    status = builder->payloadCustomParam(&payload, &payloadSize,
                                        (uint32_t *)early_eos_delay,
                                        sizeof(struct param_id_gapless_early_eos_delay_t),
                                        miid, PARAM_ID_EARLY_EOS_DELAY);
    free(early_eos_delay);
    if (status) {
        PAL_ERR(LOG_TAG, "payloadCustomParam failed status = %d", status);
        return status;
    }
    if (payloadSize) {
        status = updateCustomPayload(payload, payloadSize);
        freeCustomPayload(&payload, &payloadSize);
        if(0 != status) {
            PAL_ERR(LOG_TAG, "%s: updateCustomPayload Failed\n", __func__);
            return status;
        }
    }
    return status;
}

int SessionAlsaCompress::start(Stream * s)
{
    struct compr_config compress_config = {};
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;
    size_t in_buf_size, in_buf_count, out_buf_size, out_buf_count;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct pal_device dAttr = {};
    struct volume_set_param_info vol_set_param_info = {};
    bool isStreamAvail = false;
    uint16_t volSize = 0;
    uint8_t *volPayload = nullptr;
    uint32_t miid;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    struct sessionToPayloadParam streamData = {};
    memset(&streamData, 0, sizeof(struct sessionToPayloadParam));

    PAL_DBG(LOG_TAG, "Enter");

    memset(&dAttr, 0, sizeof(struct pal_device));
#ifndef SEC_AUDIO_EARLYDROP_PATCH
    rm->voteSleepMonitor(s, true);
#endif    
    s->getStreamAttributes(&sAttr);
    getSndCodecParam(codec, sAttr);
    s->getBufInfo(&in_buf_size, &in_buf_count, &out_buf_size,
                          &out_buf_count);

    switch (sAttr.direction) {
        case PAL_AUDIO_OUTPUT:
            if (!compressDevIds.size()) {
                PAL_ERR(LOG_TAG, "frontendIDs are not available");
                status = -EINVAL;
                goto exit;
            }
            /** create an offload thread for posting callbacks */
            worker_thread = std::make_unique<std::thread>(offloadThreadLoop, this);

            if (SND_AUDIOCODEC_AAC == codec.id &&
                codec.ch_in < CHS_2 &&
                codec.options.generic.reserved[0] == AACObjHE_PS) {
                codec.ch_in = CHS_2;
                codec.ch_out = codec.ch_in;
            }
            compress_config.fragment_size = out_buf_size;
            compress_config.fragments = out_buf_count;
            compress_config.codec = &codec;
            // compress_open
            compress =
                compress_open(rm->getVirtualSndCard(), compressDevIds.at(0),
                              COMPRESS_IN, &compress_config);
            if (!compress) {
                PAL_ERR(LOG_TAG, "compress open failed");
                status = -EINVAL;
                {
                    // send the exit command to the waiting thread
                    auto msg = std::make_shared<offload_msg>(OFFLOAD_CMD_EXIT);
                    std::lock_guard<std::mutex> lock(cv_mutex_);
                    msg_queue_.push(msg);
                    cv_.notify_all();
                }
                worker_thread->join();
                worker_thread.reset(NULL);
                goto exit;
            }
            if (!is_compress_ready(compress)) {
                PAL_ERR(LOG_TAG, "compress open not ready %s",
                        compress_get_error(compress));
                status = -EINVAL;
                goto exit;
            }
            /** set non blocking mode for writes */
            compress_nonblock(compress, !!ioMode);

            status = s->getAssociatedDevices(associatedDevices);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
                goto exit;
            }
            rm->getBackEndNames(associatedDevices, rxAifBackEnds, txAifBackEnds);
            if (rxAifBackEnds.empty() && txAifBackEnds.empty()) {
                PAL_ERR(LOG_TAG, "no backend specified for this stream");
                goto exit;

            }

            status = SessionAlsaUtils::getModuleInstanceId(mixer, (compressDevIds.at(0)),
                     rxAifBackEnds[0].second.data(), STREAM_SPR, &spr_miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", STREAM_SPR, status);
                goto exit;;
            }
            setCustomFormatParam(audio_fmt);
            for (int i = 0; i < associatedDevices.size();i++) {
                status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                if(0 != status) {
                    PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
                    goto exit;
                }
                status = configureMFC(rm, sAttr, dAttr, compressDevIds,
                            rxAifBackEnds[i].second.data());
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "configure MFC failed");
                    goto exit;
                }

                if (isGaplessFmt) {
                    status = configureEarlyEOSDelay();
                }

                if (customPayload) {
                    status = SessionAlsaUtils::setMixerParameter(mixer, compressDevIds.at(0),
                                                             customPayload, customPayloadSize);
                    freeCustomPayload();
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed");
                        goto exit;
                    }
                }

                if (!status && isMixerEventCbRegd && !isPauseRegistrationDone) {
                    // Register for callback for Soft Pause
                    size_t payload_size = 0;
                    struct agm_event_reg_cfg event_cfg;
                    payload_size = sizeof(struct agm_event_reg_cfg);
                    memset(&event_cfg, 0, sizeof(event_cfg));
                    event_cfg.event_id = EVENT_ID_SOFT_PAUSE_PAUSE_COMPLETE;
                    event_cfg.event_config_payload_size = 0;
                    event_cfg.is_register = 1;
                    status = SessionAlsaUtils::registerMixerEvent(mixer,
                                    compressDevIds.at(0), rxAifBackEnds[0].second.data(),
                                    TAG_PAUSE, (void *)&event_cfg, payload_size);
                    if (status == 0) {
                        isPauseRegistrationDone = true;
                    } else {
                        // Not a fatal error
                        PAL_ERR(LOG_TAG, "Pause callback registration failed");
                        status = 0;
                    }
                }
                if ((ResourceManager::isChargeConcurrencyEnabled) &&
                    (dAttr.id == PAL_DEVICE_OUT_SPEAKER)) {
                    status = Session::NotifyChargerConcurrency(rm, true);
                    if (0 == status) {
                        status = Session::EnableChargerConcurrency(rm, s);
                        //Handle failure case of ICL config
                        if (0 != status) {
                            PAL_DBG(LOG_TAG, "Failed to set ICL Config status %d", status);
                            status = Session::NotifyChargerConcurrency(rm, false);
                        }
                    }
                    status = 0;
                }

                if (PAL_DEVICE_OUT_SPEAKER == dAttr.id && !strcmp(dAttr.custom_config.custom_key, "mspp")) {

                    uint8_t* payload = NULL;
                    size_t payloadSize = 0;
                    uint32_t miid;
                    int32_t volStatus;
                    volStatus = SessionAlsaUtils::getModuleInstanceId(mixer, compressDevIds.at(0),
                                                                    rxAifBackEnds[0].second.data(), TAG_MODULE_MSPP, &miid);
                    if (volStatus != 0) {
                        PAL_ERR(LOG_TAG,"get MSPP ModuleInstanceId failed");
                        break;
                    }

                    builder->payloadMSPPConfig(&payload, &payloadSize, miid, rm->linear_gain.gain);
                    if (payloadSize && payload) {
                        volStatus = updateCustomPayload(payload, payloadSize);
                        free(payload);
                        if (0 != volStatus) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            break;
                        }
                    }
                    volStatus = SessionAlsaUtils::setMixerParameter(mixer, compressDevIds.at(0),
                                                                 customPayload, customPayloadSize);
                    if (customPayload) {
                        free(customPayload);
                        customPayload = NULL;
                        customPayloadSize = 0;
                    }
                    if (volStatus != 0) {
                        PAL_ERR(LOG_TAG,"setMixerParameter failed for MSPP module");
                        break;
                    }

                    //to set soft pause delay for MSPP use case.
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, compressDevIds.at(0),
                                                                    rxAifBackEnds[0].second.data(), TAG_PAUSE, &miid);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"get Soft Pause ModuleInstanceId failed");
                        break;
                    }

                    builder->payloadSoftPauseConfig(&payload, &payloadSize, miid, MSPP_SOFT_PAUSE_DELAY);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        free(payload);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            break;
                        }
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, compressDevIds.at(0),
                                                                 customPayload, customPayloadSize);
                    if (customPayload) {
                        free(customPayload);
                        customPayload = NULL;
                        customPayloadSize = 0;
                    }
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"setMixerParameter failed for soft Pause module");
                        break;
                    }
                }
            }
#ifdef SEC_AUDIO_MUTE_DETECTION
            for (auto &dev: associatedDevices) {
                int dev_id = dev->getSndDeviceId();
                if (rm->isOutputDevId(dev_id)) {
                    struct pal_device devAttr = {};
                    int ret = dev->getDeviceAttributes(&devAttr);
                    if (ret == 0 && ((devAttr.id == PAL_DEVICE_OUT_HANDSET) || (devAttr.id == PAL_DEVICE_OUT_SPEAKER))) {
                        if (dev->getDeviceCount() == 1) {
                            isCompressStart = true; // after compress start.
                            break;
                        }
                    }
                }
            }
#endif  
            break;
        case PAL_AUDIO_INPUT:
            if (!compressDevIds.size()) {
                PAL_ERR(LOG_TAG, "frontendIDs are not available");
                status = -EINVAL;
                goto exit;
            }
            compress_cap_buf_size = in_buf_size;
            compress_config.fragment_size = in_buf_size;
            compress_config.fragments = in_buf_count;
            compress_config.codec = &codec;
            // compress_open for capture
            //can use PAL_STREAM_FLAG_FRAME_BY_FRAME
            compress =
                compress_open(rm->getVirtualSndCard(), compressDevIds.at(0),
                              COMPRESS_OUT, &compress_config);
            if (!compress) {
                PAL_ERR(LOG_TAG, "compress open failed");
                status = -EINVAL;
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "capture: compress open successful");
            if (!is_compress_ready(compress)) {
                PAL_ERR(LOG_TAG, "compress open not ready %s",
                        compress_get_error(compress));
                status = -EINVAL;
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "capture: compress is ready");
            status = s->getAssociatedDevices(associatedDevices);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
                goto exit;
            }
            rm->getBackEndNames(associatedDevices, rxAifBackEnds,
                                txAifBackEnds);
            if (rxAifBackEnds.empty() && txAifBackEnds.empty()) {
                PAL_ERR(LOG_TAG, "no backend specified for this stream");
                goto exit;
            }
            status = SessionAlsaUtils::getModuleInstanceId(mixer, compressDevIds.at(0),
                                                                txAifBackEnds[0].second.data(),
                                                                TAG_STREAM_MFC_SR, &miid);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                goto exit;
            }
            PAL_DBG(LOG_TAG, "miid : %x id = %d, data %s\n", miid,
                    compressDevIds.at(0), txAifBackEnds[0].second.data());

            streamData.bitWidth = sAttr.in_media_config.bit_width;
            streamData.sampleRate = sAttr.in_media_config.sample_rate;
            streamData.numChannel = sAttr.in_media_config.ch_info.channels;
            streamData.ch_info = nullptr;
            builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
            if (payloadSize && payload) {
                status = updateCustomPayload(payload, payloadSize);
                freeCustomPayload(&payload, &payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                    goto exit;
                }
            }

            if (customPayload) {
                status = SessionAlsaUtils::setMixerParameter(
                    mixer, compressDevIds.at(0), customPayload,
                    customPayloadSize);
                freeCustomPayload();
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    goto exit;
                }
            }

           for (int i = 0; i < associatedDevices.size();i++) {
               status = associatedDevices[i]->getDeviceAttributes(&dAttr);
               if (0 != status) {
                   PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                   goto exit;
               }
           }

           //Setting the device orientation during stream open for HDR record.
           if ((dAttr.id == PAL_DEVICE_IN_HANDSET_MIC || dAttr.id == PAL_DEVICE_IN_SPEAKER_MIC)
                   && strstr(dAttr.custom_config.custom_key, "unprocessed-hdr-mic")) {
               s->setOrientation(HDRConfigKeyToDevOrientation(dAttr.custom_config.custom_key));
               PAL_DBG(LOG_TAG,"HDR record set device orientation %d", s->getOrientation());
               if (setConfig(s, MODULE, ORIENTATION_TAG) != 0) {
                   PAL_DBG(LOG_TAG,"HDR record setting device orientation failed");
               }
            }
            if (dAttr.id == PAL_DEVICE_IN_PROXY || dAttr.id == PAL_DEVICE_IN_RECORD_PROXY) {
                status = configureMFC(rm, sAttr, dAttr, compressDevIds,
                txAifBackEnds[0].second.data());
                if(status != 0) {
                    PAL_ERR(LOG_TAG, "configure MFC failed");
                }
            }
            if (customPayload) {
                status = SessionAlsaUtils::setMixerParameter(
                    mixer, compressDevIds.at(0), customPayload,
                    customPayloadSize);
                freeCustomPayload();
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    goto exit;
                }
            }

            if (!capture_started) {
                status = compress_start(compress);
                if (status) {
                    PAL_ERR(LOG_TAG, "compress start failed with err %d", status);
                    return status;
                }
                capture_started = true;
            }

            break;
        default:
            break;
    }
    setInitialVolume();
    //Setting the device orientation during stream open
    if (PAL_DEVICE_OUT_SPEAKER == dAttr.id) {
        PAL_DBG(LOG_TAG,"set device orientation %d", rm->mOrientation);
        if (!strcmp(dAttr.custom_config.custom_key, "mspp")) {
            s->setOrientation(rm->mOrientation);
            if (setConfig(s, MODULE, ORIENTATION_TAG) != 0) {
                PAL_ERR(LOG_TAG,"Setting device orientation failed");
            }
        } else {
            pal_param_device_rotation_t rotation;
            rotation.rotation_type = rm->mOrientation == ORIENTATION_270 ?
                                    PAL_SPEAKER_ROTATION_RL : PAL_SPEAKER_ROTATION_LR;
            status = handleDeviceRotation(s, rotation.rotation_type, compressDevIds.at(0), mixer,
                                          builder, rxAifBackEnds);
            if (status != 0) {
                PAL_ERR(LOG_TAG,"handleDeviceRotation failed\n");
                goto exit;
            }
        }
    }
exit:
#ifndef SEC_AUDIO_EARLYDROP_PATCH
    if (status != 0)
        rm->voteSleepMonitor(s, false);
#endif        
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::pause(Stream * s)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    if (compress && playback_started) {
        status = compress_pause(compress);
        if (status == 0)
            playback_paused = true;
    }
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::resume(Stream * s __unused)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    if (compress && playback_paused) {
        status = compress_resume(compress);
        if (status == 0)
            playback_paused = false;
    }
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::stop(Stream * s __unused)
{
    int32_t status = 0;
    size_t payload_size = 0;
    struct agm_event_reg_cfg event_cfg;
    struct pal_stream_attributes sAttr = {};

    PAL_DBG(LOG_TAG, "Enter");

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    switch (sAttr.direction) {
        case PAL_AUDIO_OUTPUT:
            if (compress && playback_started) {
                status = compress_stop(compress);
                playback_started = false;
            }
            // Deregister for callback for Soft Pause
            if (isPauseRegistrationDone) {
                payload_size = sizeof(struct agm_event_reg_cfg);
                memset(&event_cfg, 0, sizeof(event_cfg));
                event_cfg.event_id = EVENT_ID_SOFT_PAUSE_PAUSE_COMPLETE;
                event_cfg.event_config_payload_size = 0;
                event_cfg.is_register = 0;
                if (!compressDevIds.size()) {
                    PAL_ERR(LOG_TAG, "frontendIDs are not available");
                    status = -EINVAL;
                    goto exit;
                }
                if (!rxAifBackEnds.size()) {
                    PAL_ERR(LOG_TAG, "rxAifBackEnds are not available");
                    status = -EINVAL;
                    goto exit;
                }
                status = SessionAlsaUtils::registerMixerEvent(mixer, compressDevIds.at(0),
                            rxAifBackEnds[0].second.data(), TAG_PAUSE, (void *)&event_cfg,
                            payload_size);
                if (status == 0 || rm->cardState == CARD_STATUS_OFFLINE) {
                    isPauseRegistrationDone = false;
                } else {
                    // Not a fatal error
                    PAL_ERR(LOG_TAG, "Pause callback deregistration failed\n");
                    status = 0;
                }
            }
            break;
        case PAL_AUDIO_INPUT:
            if (compress) {
                status = compress_stop(compress);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "compress_stop failed %d for capture",
                            status);
                } else {
                    PAL_VERBOSE(LOG_TAG,"compress_stop successful for capture");
                }
            }
            break;
        case PAL_AUDIO_INPUT_OUTPUT:
            break;
    }
exit:
#ifndef SEC_AUDIO_EARLYDROP_PATCH
    rm->voteSleepMonitor(s, false);
#endif    
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::close(Stream * s)
{
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;
    std::string backendname;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    int32_t beDevId = 0;
    int devCount = 0;

    PAL_DBG(LOG_TAG, "Enter");

    s->getStreamAttributes(&sAttr);
    status = s->getAssociatedDevices(associatedDevices);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices Failed\n");
        goto exit;
    }
    freeDeviceMetadata.clear();

    switch(sAttr.direction){
        case PAL_AUDIO_OUTPUT:
            for (auto &dev : associatedDevices) {
                beDevId = dev->getSndDeviceId();
                rm->getBackendName(beDevId, backendname);
                PAL_DBG(LOG_TAG, "backendname %s", backendname.c_str());
                devCount = dev->getDeviceCount();
                // Do not clear device metadata for A2DP device if SCO device is active
                if ((devCount == 1) && rm->isBtA2dpDevice((pal_device_id_t) beDevId))
                    devCount += SessionAlsaUtils::getScoDevCount();
                if (devCount > 1) {
                    PAL_DBG(LOG_TAG, "Rx dev still active\n");
                    freeDeviceMetadata.push_back(
                        std::make_pair(backendname, 0));
                } else {
                    freeDeviceMetadata.push_back(
                        std::make_pair(backendname, 1));
                    PAL_DBG(LOG_TAG, "Rx dev not active");
                }
            }
            status = SessionAlsaUtils::close(s, rm, compressDevIds,
                                             rxAifBackEnds, freeDeviceMetadata);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
            }
            if (compress) {
                if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
                    std::shared_ptr<offload_msg> msg = std::make_shared<offload_msg>(OFFLOAD_CMD_ERROR);
                    std::lock_guard<std::mutex> lock(cv_mutex_);
                    msg_queue_.push(msg);
                    cv_.notify_all();
                }
                {
                    std::shared_ptr<offload_msg> msg = std::make_shared<offload_msg>(OFFLOAD_CMD_EXIT);
                    std::lock_guard<std::mutex> lock(cv_mutex_);
                    msg_queue_.push(msg);
                    cv_.notify_all();
                }

                /* wait for handler to exit */
                worker_thread->join();
                worker_thread.reset(NULL);

                /* empty the pending messages in queue */
                while (!msg_queue_.empty())
                    msg_queue_.pop();
                compress_close(compress);
            }
            PAL_DBG(LOG_TAG, "out of compress close");

            // Deregister for mixer event callback
            if (isMixerEventCbRegd) {
                status = rm->registerMixerEventCallback(compressDevIds, sessionCb, cbCookie,
                                false);
                if (status == 0) {
                    isMixerEventCbRegd = false;
                } else {
                    // Not a fatal error
                    PAL_ERR(LOG_TAG, "Failed to deregister callback to rm");
                    status = 0;
                }
            }

            rm->freeFrontEndIds(compressDevIds, sAttr, 0);
#ifdef SEC_AUDIO_MUTE_DETECTION
            for (auto &dev: associatedDevices) {
                int dev_id = dev->getSndDeviceId();
                if(rm->isOutputDevId(dev_id)) {
                    struct pal_device devAttr = {};
                    int ret = dev->getDeviceAttributes(&devAttr);
                    if (ret == 0 && ((devAttr.id == PAL_DEVICE_OUT_HANDSET) || (devAttr.id == PAL_DEVICE_OUT_SPEAKER))) {
                        if (dev->getDeviceCount() == 1) {
                            rm->getKernelConfigInfo(RX_CONFIG_INFO); // after compress stop.
                            break;
                        }
                    }
                }
            }
#endif
            compress = NULL;
            freeCustomPayload();
            break;

        case PAL_AUDIO_INPUT:
            for (auto &dev : associatedDevices) {
                beDevId = dev->getSndDeviceId();
                rm->getBackendName(beDevId, backendname);
                PAL_DBG(LOG_TAG, "backendname %s", backendname.c_str());
                if (dev->getDeviceCount() != 0) {
                    PAL_DBG(LOG_TAG, "Tx dev still active\n");
                    freeDeviceMetadata.push_back(
                        std::make_pair(backendname, 0));
                } else {
                    freeDeviceMetadata.push_back(
                        std::make_pair(backendname, 1));
                    PAL_DBG(LOG_TAG, "Tx dev not active");
                }
            }
            status = SessionAlsaUtils::close(s, rm, compressDevIds,
                                             txAifBackEnds, freeDeviceMetadata);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
            }
            if (compress)
                compress_close(compress);

            rm->freeFrontEndIds(compressDevIds, sAttr, 0);
            compress = NULL;
            break;

        case PAL_AUDIO_INPUT_OUTPUT:
        default:
            break;
    }

 exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::read(Stream *s, int tag __unused,
                              struct pal_buffer *buf, int *size) {
    int status = 0, bytesRead = 0, offset = 0;
    struct pal_stream_attributes sAttr = {};

    PAL_VERBOSE(LOG_TAG, "Enter")
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }


    void *data = buf->buffer;
    data = static_cast<char *>(data) + offset;

    bytesRead = compress_read(compress, data, *size);

    if (bytesRead < 0) {
        PAL_ERR(LOG_TAG, "compress read failed, status %d", bytesRead);
        *size = 0;
        return -EINVAL;
    }
    *size = bytesRead;
    buf->size = bytesRead;
    PAL_VERBOSE(LOG_TAG, "exit bytesRead:%d ", bytesRead);
    return status;
}

int SessionAlsaCompress::fileWrite(Stream *s __unused, int tag __unused, struct pal_buffer *buf, int * size, int flag __unused)
{
    std::fstream fs;
    PAL_DBG(LOG_TAG, "Enter.");

    fs.open ("/data/testcompr.wav", std::fstream::binary | std::fstream::out | std::fstream::app);
    PAL_ERR(LOG_TAG, "file open success");
    char *buff= reinterpret_cast<char *>(buf->buffer);
    fs.write (buff,buf->size);
    PAL_ERR(LOG_TAG, "file write success");
    fs.close();
    PAL_ERR(LOG_TAG, "file close success");
    *size = (int)(buf->size);
    PAL_ERR(LOG_TAG, "Exit. size: %d", *size);
    return 0;
}

int SessionAlsaCompress::write(Stream *s __unused, int tag __unused, struct pal_buffer *buf, int * size, int flag __unused)
{
    int bytes_written = 0;
    int status;
    bool non_blocking = (!!ioMode);
    if (!buf || !(buf->buffer) || !(buf->size)) {
        PAL_VERBOSE(LOG_TAG, "buf: %pK, size: %zu",
                    buf, (buf ? buf->size : 0));
        return -EINVAL;
    }
    if (!compress) {
        PAL_ERR(LOG_TAG, "NULL pointer access,compress is invalid");
        return -EINVAL;
    }

    PAL_DBG(LOG_TAG, "buf->size is %zu buf->buffer is %pK ",
            buf->size, buf->buffer);

    bytes_written = compress_write(compress, buf->buffer, buf->size);

    PAL_VERBOSE(LOG_TAG, "writing buffer (%zu bytes) to compress device returned %d",
             buf->size, bytes_written);

    if (bytes_written >= 0 && bytes_written < (ssize_t)buf->size && non_blocking) {
        PAL_DBG(LOG_TAG, "No space available in compress driver, post msg to cb thread");

        if (command != OFFLOAD_CMD_WAIT_FOR_BUFFER)
        {
            std::shared_ptr<offload_msg> msg = std::make_shared<offload_msg>(OFFLOAD_CMD_WAIT_FOR_BUFFER);
            std::lock_guard<std::mutex> lock(cv_mutex_);
            msg_queue_.push(msg);

            cv_.notify_all();
        }
    }

    if (!playback_started && bytes_written > 0) {
        status = compress_start(compress);
        if (status) {
            PAL_ERR(LOG_TAG, "compress start failed with err %d", status);
            return status;
        }
        playback_started = true;
        if (isCompressStart) {
            isCompressStart = false;
            rm->getKernelConfigInfo(RX_CONFIG_INFO);
        }
    }

    if (size)
        *size = bytes_written;
    return 0;
}

int SessionAlsaCompress::readBufferInit(Stream *s __unused, size_t noOfBuf __unused, size_t bufSize __unused, int flag __unused)
{
    return 0;
}
int SessionAlsaCompress::writeBufferInit(Stream *s __unused, size_t noOfBuf __unused, size_t bufSize __unused, int flag __unused)
{
    return 0;
}

struct mixer_ctl* SessionAlsaCompress::getFEMixerCtl(const char *controlName, int *device, pal_stream_direction_t dir __unused)
{
    std::ostringstream CntrlName;
    struct mixer_ctl *ctl;

    if (compressDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
        return nullptr;
    }

    *device = compressDevIds.at(0);
    CntrlName << "COMPRESS" << compressDevIds.at(0) << " " << controlName;
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        return nullptr;
    }

    return ctl;
}

int32_t SessionAlsaCompress::getFrontEndId(uint32_t ldir __unused)
{
    int32_t device = -EINVAL;

    if (compressDevIds.size())
        device = compressDevIds.at(0);

    return device;
}

uint32_t SessionAlsaCompress::getMIID(const char *backendName, uint32_t tagId, uint32_t *miid)
{
    int status = 0;
    int device = 0;

    if (compressDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "compressDevIds is not available.");
        status = -EINVAL;
        return status;
    }
    device = compressDevIds.at(0);
    status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                                   backendName,
                                                   tagId, miid);
    if (0 != status)
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);

    return status;
}

int SessionAlsaCompress::setParameters(Stream *s __unused, int tagId, uint32_t param_id, void *payload)
{
    pal_param_payload *param_payload = (pal_param_payload *)payload;
    int status = 0;
    int device = 0;
    uint8_t* alsaParamData = NULL;
    size_t alsaPayloadSize = 0;
    uint32_t miid = 0;
    effect_pal_payload_t *effectPalPayload = nullptr;
    struct compr_gapless_mdata mdata;
    struct pal_compr_gapless_mdata *gaplessMdata = NULL;
    struct pal_stream_attributes sAttr = {};

    s->getStreamAttributes(&sAttr);

    PAL_DBG(LOG_TAG, "Enter");

    switch (param_id) {
        case PAL_PARAM_ID_DEVICE_ROTATION:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            pal_param_device_rotation_t *rotation =
                                     (pal_param_device_rotation_t *)payload;
            status = handleDeviceRotation(s, rotation->rotation_type, device, mixer,
                                          builder, rxAifBackEnds);
        }
        break;
        case PAL_PARAM_ID_BT_A2DP_TWS_CONFIG:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            pal_bt_tws_payload *tws_payload = (pal_bt_tws_payload *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            builder->payloadTWSConfig(&alsaParamData, &alsaPayloadSize,
                 miid, tws_payload->isTwsMonoModeOn, tws_payload->codecFormat);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set tws config status=%d\n", status);
                freeCustomPayload(&alsaParamData, &alsaPayloadSize);
            }
            return 0;
        }
        case PAL_PARAM_ID_BT_A2DP_LC3_CONFIG:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            pal_bt_lc3_payload *lc3_payload = (pal_bt_lc3_payload *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            builder->payloadLC3Config(&alsaParamData, &alsaPayloadSize,
                 miid, lc3_payload->isLC3MonoModeOn);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set lc3 config status=%d\n", status);
                freeCustomPayload(&alsaParamData, &alsaPayloadSize);
            }
            return 0;
        }
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
        case PAL_PARAM_ID_BT_A2DP_SUSPENDED:
        {
            pal_param_ss_pack_a2dp_suspend_t *a2dp_suspend_payload = (pal_param_ss_pack_a2dp_suspend_t *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            PAL_INFO(LOG_TAG, "mixer set a2dp_suspend %d\n", a2dp_suspend_payload->a2dp_suspend);

            builder->payloadSsPackSuspendConfig(&alsaParamData, &alsaPayloadSize,
                miid, a2dp_suspend_payload->a2dp_suspend);

            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set a2dp_suspend status=%d\n", status);
                free(alsaParamData);
            }
            return 0;
        }
        case PAL_PARAM_ID_BT_A2DP_DYN_BITRATE:
        {
            pal_param_bta2dp_dynbitrate_t *dynbitrate_payload = (pal_param_bta2dp_dynbitrate_t *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            PAL_INFO(LOG_TAG, "mixer set bitrate %d\n", dynbitrate_payload->bitrate);

            builder->payloadSsPackBitrate(&alsaParamData, &alsaPayloadSize,
                 miid, dynbitrate_payload->bitrate);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set bitrate status=%d\n", status);
                free(alsaParamData);
            }
            return 0;
        }
        case PAL_PARAM_ID_BT_A2DP_SBM_CONFIG:
        {
            pal_param_bta2dp_sbm_t *sbm_payload = (pal_param_bta2dp_sbm_t *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            PAL_INFO(LOG_TAG, "mixer set sbm config firstParam=%d, secondParam=%d\n",
                                    sbm_payload->firstParam, sbm_payload->secondParam);

            builder->payloadSsPackSbmConfig(&alsaParamData, &alsaPayloadSize,
                 miid, sbm_payload);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set sbm config status=%d\n", status);
                free(alsaParamData);
            }
            return 0;
        }
#endif
        case PAL_PARAM_ID_CODEC_CONFIGURATION:
            PAL_DBG(LOG_TAG, "Compress Codec Configuration");
            updateCodecOptions((pal_param_payload *)payload, sAttr.direction);
            if (compress && audio_fmt != PAL_AUDIO_FMT_VORBIS) {
                /* For some audio fmt, codec configuration is default like
                 * for mp3, and for some it is hardcoded like for aac, in
                 * these cases, we don't need to send codec params to ADSP
                 * again even if it comes from hal as it will not change.
                 */
                if (isCodecConfigNeeded(audio_fmt, sAttr.direction)) {
                    PAL_DBG(LOG_TAG, "Setting params for second clip for gapless");
                    status = compress_set_codec_params(compress, &codec);
                } else {
                    PAL_INFO(LOG_TAG, "No need to send params for second clip fmt %x", audio_fmt);
                }
            } else if (compress && (audio_fmt == PAL_AUDIO_FMT_VORBIS)) {
                PAL_DBG(LOG_TAG, "Setting params for second clip for gapless");
                sendNextTrackParams = true;
                status = setCustomFormatParam(audio_fmt);
            }
        break;
        case PAL_PARAM_ID_GAPLESS_MDATA:
        {
            if (!compress) {
                PAL_ERR(LOG_TAG, "Compress is invalid");
                status = -EINVAL;
                goto exit;
            }
            if (isGaplessFmt) {
                gaplessMdata = (pal_compr_gapless_mdata*)param_payload->payload;
                PAL_DBG(LOG_TAG, "Setting gapless metadata %d %d",
                                  gaplessMdata->encoderDelay,
                                  gaplessMdata->encoderPadding);
                mdata.encoder_delay = gaplessMdata->encoderDelay;
                mdata.encoder_padding = gaplessMdata->encoderPadding;
                status = compress_set_gapless_metadata(compress, &mdata);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "set gapless metadata failed");
                    goto exit;
                }
             } else {
                 PAL_ERR(LOG_TAG, "audio fmt %x is not gapless", audio_fmt);
                 status = -EINVAL;
                 goto exit;
             }
        }
        break;
        case PAL_PARAM_ID_VOLUME_USING_SET_PARAM:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            pal_volume_data *vdata = (struct pal_volume_data *)param_payload->payload;
            status = streamHandle->getStreamAttributes(&sAttr);
            if (sAttr.direction == PAL_AUDIO_OUTPUT) {
                device = compressDevIds.at(0);
                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                        rxAifBackEnds[0].second.data(), TAG_STREAM_VOLUME, &miid);
            } else {
                status = 0;
                PAL_INFO(LOG_TAG, "Unsupported stream direction %d(ignore)", sAttr.direction);
                goto exit;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, dir: %d (%d)", tagId,
                       sAttr.direction, status);
                goto exit;
            }

            if (vdata->no_of_volpair > 1 && sAttr.out_media_config.ch_info.channels > 1) {
                builder->payloadMultichVolumemConfig(&alsaParamData, &alsaPayloadSize, miid, vdata);
            } else {
                builder->payloadVolumeConfig(&alsaParamData, &alsaPayloadSize, miid, vdata);
            }

            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set volume config status=%d\n", status);
                freeCustomPayload(&alsaParamData, &alsaPayloadSize);
                alsaPayloadSize = 0;
            }
        }
        break;
        case PAL_PARAM_ID_MSPP_LINEAR_GAIN:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            pal_param_mspp_linear_gain_t *linear_gain = (pal_param_mspp_linear_gain_t *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);

            PAL_DBG(LOG_TAG, "set MSPP linear gain");
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            builder->payloadMSPPConfig(&alsaParamData, &alsaPayloadSize, miid, linear_gain->gain);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set MSPP config status=%d\n", status);
                free(alsaParamData);
            }
            return 0;
        }
        break;
        case PAL_PARAM_ID_VOLUME_CTRL_RAMP:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
            struct pal_vol_ctrl_ramp_param *rampParam = (struct pal_vol_ctrl_ramp_param *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            builder->payloadVolumeCtrlRamp(&alsaParamData, &alsaPayloadSize,
                 miid, rampParam->ramp_period_ms);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set vol ctrl ramp status=%d\n", status);
                freeCustomPayload(&alsaParamData, &alsaPayloadSize);
            }
            break;
        }
        case PAL_PARAM_ID_RECONFIG_ENCODER: {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }

            if (!payload) {
                PAL_ERR(LOG_TAG, "no payload");
                status = -EINVAL;
                goto exit;
            }

            auto pal_payload = (pal_param_payload *)payload;
            if (pal_payload->payload_size != sizeof(pal_snd_enc_t)) {
                PAL_ERR(LOG_TAG, "not expected payload size");
                status = -EINVAL;
                goto exit;
            }

            auto encoder_config = (pal_snd_enc_t *)(pal_payload->payload);

            uint32_t encoderMIID = 0;
            status = SessionAlsaUtils::getModuleInstanceId(
                mixer, device, txAifBackEnds[0].second.data(),
                TAG_STREAM_PLACEHOLDER_ENCODER, &encoderMIID);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d",
                        tagId, status);
                return status;
            }

            size_t dspPayloadSize = 0;
            auto dspPayload = builder->getPayloadEncoderBitrate(
                encoderMIID, encoder_config->aac_enc.aac_bit_rate,
                dspPayloadSize);
            if (dspPayload && dspPayloadSize > 0) {
                status = SessionAlsaUtils::setMixerParameter(
                    mixer, device, dspPayload.get(), dspPayloadSize);
                PAL_INFO(LOG_TAG, "issued new bitrate with status: %d", status);
            } else {
                PAL_ERR(LOG_TAG, "failed to build payload for encoder bitrate");
            }
            break;
        }
        case PAL_PARAM_ID_TIMESTRETCH_PARAMS:
        {
            if (compressDevIds.size()) {
                device = compressDevIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }

            pal_param_playback_rate_t *playbackRate =
                                        (pal_param_playback_rate_t *)(param_payload->payload);
            PAL_DBG(LOG_TAG, "speed %f, pitch %f", playbackRate->speed, playbackRate->pitch);
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), TAG_MODULE_TSM, &miid);

            builder->payloadPlaybackRateParametersConfig(&alsaParamData, &alsaPayloadSize,
                                                            miid, playbackRate);
            if (alsaPayloadSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               alsaParamData, alsaPayloadSize);
                PAL_INFO(LOG_TAG, "mixer set playbackRate parameters status=%d", status);
                freeCustomPayload(&alsaParamData, &alsaPayloadSize);
            }
            break;
        }
        default:
            PAL_INFO(LOG_TAG, "Unsupported param id %u", param_id);
        break;
    }
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::registerCallBack(session_callback cb, uint64_t cookie)
{
    sessionCb = cb;
    cbCookie = cookie;
    return 0;
}

int SessionAlsaCompress::flush()
{
    int status = 0;
    PAL_VERBOSE(LOG_TAG, "Enter flush");

    if (playback_started) {
        if (compressDevIds.size() > 0) {
            status = SessionAlsaUtils::flush(rm, compressDevIds.at(0));
        } else {
            PAL_ERR(LOG_TAG, "DevIds size is invalid");
            return -EINVAL;
        }
    }

    PAL_VERBOSE(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaCompress::drain(pal_drain_type_t type)
{
    std::shared_ptr<offload_msg> msg;

    if (!compress) {
       PAL_ERR(LOG_TAG, "compress is invalid");
       return -EINVAL;
    }

    PAL_VERBOSE(LOG_TAG, "drain type = %d", type);

    switch (type) {
    case PAL_DRAIN:
    {
        msg = std::make_shared<offload_msg>(OFFLOAD_CMD_DRAIN);
        std::lock_guard<std::mutex> drain_lock(cv_mutex_);
        msg_queue_.push(msg);
        cv_.notify_all();
    }
    break;

    case PAL_DRAIN_PARTIAL:
    {
        msg = std::make_shared<offload_msg>(OFFLOAD_CMD_PARTIAL_DRAIN);
        std::lock_guard<std::mutex> partial_lock(cv_mutex_);
        msg_queue_.push(msg);
        cv_.notify_all();
    }
    break;

    default:
        PAL_ERR(LOG_TAG, "invalid drain type = %d", type);
        return -EINVAL;
    }

    return 0;
}

int SessionAlsaCompress::getParameters(Stream *s __unused, int tagId __unused, uint32_t param_id __unused, void **payload __unused)
{
    return 0;
}

int SessionAlsaCompress::getTimestamp(struct pal_session_time *stime)
{
    int status = 0;
    status = SessionAlsaUtils::getTimestamp(mixer, compressDevIds, spr_miid, stime);
    if (0 != status) {
       PAL_ERR(LOG_TAG, "getTimestamp failed status = %d", status);
       return status;
    }
    return status;
}

int SessionAlsaCompress::setECRef(Stream *s __unused, std::shared_ptr<Device> rx_dev __unused, bool is_enable __unused)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    std::vector <std::shared_ptr<Device>> rxDeviceList;
    std::vector <std::string> backendNames;
    struct pal_device rxDevAttr = {};
    struct pal_device_info rxDevInfo = {};

    PAL_DBG(LOG_TAG, "Enter");
    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream or rx device");
        status = -EINVAL;
        goto exit;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }

    if (sAttr.direction != PAL_AUDIO_INPUT) {
        PAL_ERR(LOG_TAG, "EC Ref cannot be set to output stream");
        status = -EINVAL;
        goto exit;
    }

    rxDevInfo.isExternalECRefEnabledFlag = 0;
    if (rx_dev) {
        status = rx_dev->getDeviceAttributes(&rxDevAttr);
        if (status != 0) {
            PAL_ERR(LOG_TAG," get device attributes failed");
            goto exit;
        }
        rm->getDeviceInfo(rxDevAttr.id, sAttr.type, rxDevAttr.custom_config.custom_key, &rxDevInfo);
    } else if (!is_enable && ecRefDevId != PAL_DEVICE_OUT_MIN) {
        // if disable EC with null rx_dev, retrieve current EC device
        rxDevAttr.id = ecRefDevId;
        rx_dev = Device::getInstance(&rxDevAttr, rm);
        if (rx_dev) {
            status = rx_dev->getDeviceAttributes(&rxDevAttr);
            if (status) {
                PAL_ERR(LOG_TAG, "getDeviceAttributes failed for ec dev: %d", ecRefDevId);
                goto exit;
            }
        }
        rm->getDeviceInfo(rxDevAttr.id, sAttr.type, rxDevAttr.custom_config.custom_key, &rxDevInfo);
    }

    if (!is_enable) {
        if (ecRefDevId == PAL_DEVICE_OUT_MIN) {
            PAL_DBG(LOG_TAG, "EC ref not enabled, skip disabling");
            goto exit;
        } else if (rx_dev && ecRefDevId != rx_dev->getSndDeviceId()) {
            PAL_DBG(LOG_TAG, "Invalid rx dev %d for disabling EC ref, "
                "rx dev %d already enabled", rx_dev->getSndDeviceId(), ecRefDevId);
            goto exit;
        }

        if (rxDevInfo.isExternalECRefEnabledFlag) {
            status = checkAndSetExtEC(rm, s, false);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to disable External EC, status %d", status);
                goto exit;
            }
        } else {
            if (compressDevIds.size()) {
                status = SessionAlsaUtils::setECRefPath(mixer, compressDevIds.at(0), "ZERO");
                if (status) {
                    PAL_ERR(LOG_TAG, "Failed to disable EC Ref, status %d", status);
                    goto exit;
                }
            } else {
                PAL_ERR(LOG_TAG, "No compressDevIds found");
                status = -EINVAL;
                goto exit;
            }
        }
    } else if (is_enable && rx_dev) {
        if (ecRefDevId == rx_dev->getSndDeviceId()) {
            PAL_DBG(LOG_TAG, "EC Ref already set for dev %d", ecRefDevId);
            goto exit;
        }
        if (!compressDevIds.size()) {
            PAL_ERR(LOG_TAG, "No compressDevIds found");
            status = -EINVAL;
            goto exit;
        }
        // TODO: handle EC Ref switch case also
        rxDeviceList.push_back(rx_dev);
        backendNames = rm->getBackEndNames(rxDeviceList);

        if (rxDevInfo.isExternalECRefEnabledFlag) {
            // reset EC if internal EC is being used
            if (ecRefDevId != PAL_DEVICE_OUT_MIN && !rm->isExternalECRefEnabled(ecRefDevId)) {
                status = SessionAlsaUtils::setECRefPath(mixer, compressDevIds.at(0), "ZERO");
                if (status) {
                    PAL_ERR(LOG_TAG, "Failed to reset before set ext EC, status %d", status);
                    goto exit;
                }
            }
            status = checkAndSetExtEC(rm, s, true);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to enable External EC, status %d", status);
                goto exit;
            }
        } else {
            // avoid set internal ec if ext ec connected
            if (ecRefDevId != PAL_DEVICE_OUT_MIN && rm->isExternalECRefEnabled(ecRefDevId)) {
                PAL_ERR(LOG_TAG, "Cannot be set internal EC with external EC connected");
                status = -EINVAL;
                goto exit;
            }
            status = SessionAlsaUtils::setECRefPath(mixer, compressDevIds.at(0),
                    backendNames[0].c_str());
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to enable EC Ref, status %d", status);
                goto exit;
            }
        }
    } else {
        PAL_ERR(LOG_TAG, "Invalid operation");
        status = -EINVAL;
        goto exit;
    }
exit:
    if (status == 0) {
        if (is_enable && rx_dev)
            ecRefDevId = static_cast<pal_device_id_t>(rx_dev->getSndDeviceId());
        else
            ecRefDevId = PAL_DEVICE_OUT_MIN;
    }
    PAL_DBG(LOG_TAG, "Exit, status: %d", status);
    return status;
}
