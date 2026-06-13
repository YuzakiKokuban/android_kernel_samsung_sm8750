/*
 * Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
 * This code is used under the BSD license.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2011-2012, Intel Corporation
 * All rights reserved.
 *
 * Author: Vinod Koul <vinod.koul@linux.intel.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * LGPL LICENSE
 *
 * Copyright (c) 2011-2012, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to
 * the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <linux/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/time.h>
#define __force
//#define __bitwise
#define __user
#include <sound/compress_params.h>
#include <tinycompress/tinycompress.h>

#include "agmmixer.h"


#define MP3_SYNC 0xe0ff

const int mp3_sample_rates[3][3] = {
    {44100, 48000, 32000},        /* MPEG-1 */
    {22050, 24000, 16000},        /* MPEG-2 */
    {11025, 12000,  8000},        /* MPEG-2.5 */
};

const int mp3_bit_rates[3][3][15] = {
    {
        /* MPEG-1 */
        {  0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448}, /* Layer 1 */
        {  0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384}, /* Layer 2 */
        {  0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}, /* Layer 3 */
    },
    {
        /* MPEG-2 */
        {  0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256}, /* Layer 1 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, /* Layer 2 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, /* Layer 3 */
    },
    {
        /* MPEG-2.5 */
        {  0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256}, /* Layer 1 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, /* Layer 2 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, /* Layer 3 */
    },
};

enum mpeg_version {
    MPEG1  = 0,
    MPEG2  = 1,
    MPEG25 = 2
};

enum mp3_stereo_mode {
    STEREO = 0x00,
    JOINT = 0x01,
    DUAL = 0x02,
    MONO = 0x03
};

static int verbose;

static void usage(void)
{
    fprintf(stderr, "usage: cplay [OPTIONS] filename\n"
        "-D\tcard number\n"
        "-d\tdevice node\n"
        "-b\tbuffer size\n"
        "-f\tfragments\n\n"
        "-v\tverbose mode\n"
        "-help\tPrints this help list\n\n"
        "-t\tcodec type\n\n"
            "1 : mp3"
            "2 : aac"
        "-p\tpause/resume with 2 secs sleep\n\n"
        " [-num_intf num of interfaces followed by interface name]\n"
        " [-i intf_name] : Can be multiple if num_intf is more than 1\n"
        " [-dkv device_kv] : Can be multiple if num_intf is more than 1\n"
        " [-dppkv deviceppkv] : Assign 0 if no device pp in the graph\n"
        " [-ikv instance_kv] :  Assign 0 if no instance kv in the graph\n"
        " [-skv stream_kv]");

    exit(EXIT_FAILURE);
}

void play_samples(char *name, unsigned int card, unsigned int device, unsigned int *device_kv,
                  unsigned int stream_kv, unsigned int instance_kv, unsigned int *devicepp_kv,
                  unsigned long buffer_size, unsigned int frag, unsigned int format,
                  int pause, char **intf_name, int intf_num);

struct mp3_header {
    uint16_t sync;
    uint8_t format1;
    uint8_t format2;
};

#define ADTS_SYNC 0xF0FF
#define MP3_FORMAT 1
#define AAC_ADTS_FORMAT 2

struct adts_header {
    uint32_t sync;
    uint8_t format1;
    uint8_t format2;
    uint8_t format3;
};

uint aac_sample_rates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 48000, 48000, 48000};

int parse_aac_header(struct adts_header *header, unsigned int *num_channels,
        unsigned int *sample_rate, unsigned int *protection_absent)
{
    int profile, sample_rate_idx;

    /* check sync bits */
    if ((header->sync & 0xF0FF) != ADTS_SYNC) {
        fprintf(stderr, "Error: Can't find sync word: 0x%X\n", header->sync);
        return -1;
    }
    *protection_absent = (header->sync >> 8) & 0x01;
    profile = ((header->sync >> 22) & 0x03) - 1;
    sample_rate_idx = ((header->sync >> 18) & 0x0F);
//  channel_idx = ((header->format1 >> 7) & 0x07);
//  frame_length  = ((header->format1 >> 14) & 0x1FFF);

    *num_channels = 2;
    *sample_rate = aac_sample_rates[sample_rate_idx];
//   *size = frame_length - ((protection_absent == 1)? 7: 9));

    if (verbose)
        printf("%s: exit sr %d\n", __func__, *sample_rate);
    return 0;
}

int parse_mp3_header(struct mp3_header *header, unsigned int *num_channels,
        unsigned int *sample_rate, unsigned int *bit_rate)
{
    int ver_idx, mp3_version, layer, bit_rate_idx, sample_rate_idx, channel_idx;

    /* check sync bits */
    if ((header->sync & MP3_SYNC) != MP3_SYNC) {
        fprintf(stderr, "Error: Can't find sync word\n");
        return -1;
    }
    ver_idx = (header->sync >> 11) & 0x03;
    mp3_version = ver_idx == 0 ? MPEG25 : ((ver_idx & 0x1) ? MPEG1 : MPEG2);
    layer = 4 - ((header->sync >> 9) & 0x03);
    bit_rate_idx = ((header->format1 >> 4) & 0x0f);
    sample_rate_idx = ((header->format1 >> 2) & 0x03);
    channel_idx = ((header->format2 >> 6) & 0x03);

    if (sample_rate_idx == 3 || layer == 4 || bit_rate_idx == 15) {
        fprintf(stderr, "Error: Can't find valid header\n");
        return -1;
    }
    *num_channels = (channel_idx == MONO ? 1 : 2);
    *sample_rate = mp3_sample_rates[mp3_version][sample_rate_idx];
    *bit_rate = (mp3_bit_rates[mp3_version][layer - 1][bit_rate_idx]) * 1000;
    if (verbose)
        printf("%s: exit\n", __func__);
    return 0;
}

int check_codec_format_supported(unsigned int card, unsigned int device, struct snd_codec *codec)
{
    if (is_codec_supported(card, device, COMPRESS_IN, codec) == false) {
        fprintf(stderr, "Error: This codec or format is not supported by DSP\n");
        return -1;
    }
    return 0;
}

static int print_time(struct compress *compress)
{
    unsigned int avail;
    struct timespec tstamp;

    if (compress_get_hpointer(compress, &avail, &tstamp) != 0) {
        fprintf(stderr, "Error querying timestamp\n");
        fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
        return -1;
    } else
        fprintf(stderr, "DSP played %ld.%ld\n", tstamp.tv_sec, tstamp.tv_nsec*1000);
    return 0;
}

int main(int argc, char **argv)
{
    char *file;
    unsigned long buffer_size = 0;
    char **intf_name = NULL;
    int ret = 0, i = 0;
    unsigned int card = 0, device = 0, frag = 0, audio_format = 0, pause = 0;
    int intf_num = 1;
    uint32_t dkv = SPEAKER;
    uint32_t dppkv = DEVICEPP_RX_AUDIO_MBDRC;
    unsigned int stream_kv = 0;
    unsigned int instance_kv = INSTANCE_1;
    unsigned int *device_kv = (unsigned int *) malloc(intf_num * sizeof(unsigned int));
    unsigned int *devicepp_kv = (unsigned int *) malloc(intf_num * sizeof(unsigned int));

    if (!device_kv || !devicepp_kv) {
        printf(" insufficient memory\n");
        return 1;
    }

    if (argc < 3) {
        usage();
        return 1;
    }

    device_kv[0] = dkv;
    devicepp_kv[0] = dppkv;

    file = argv[1];
    /* parse command line arguments */
    argv += 2;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            argv++;
            if (*argv)
                device = atoi(*argv);
        } else if (strcmp(*argv, "-t") == 0) {
            argv++;
            if (*argv)
                audio_format = atoi(*argv);
        } else if (strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                card = atoi(*argv);
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                pause = atoi(*argv);
        } else if (strcmp(*argv, "-f") == 0) {
            argv++;
            if (*argv)
                frag = atoi(*argv);
        } else if (strcmp(*argv, "-v") == 0) {
            argv++;
            if (*argv)
                verbose = atoi(*argv);
        } else if (strcmp(*argv, "-num_intf") == 0) {
                      argv++;
            if (*argv)
                intf_num = atoi(*argv);
        } else if (strcmp(*argv, "-i") == 0) {
            intf_name = (char**) malloc(intf_num * sizeof(char*));
            if (!intf_name) {
                printf("insufficient memory\n");
                return 1;
            }
            for (i = 0; i < intf_num ; i++){
                argv++;
                if (*argv)
                    intf_name[i] = *argv;
            }
        } else if (strcmp(*argv, "-dkv") == 0) {
            device_kv = (unsigned int *) realloc(device_kv, intf_num * sizeof(unsigned int));
            if (!device_kv) {
                printf(" insufficient memory\n");
                return 1;
            }
            for (i = 0; i < intf_num ; i++) {
                argv++;
                if (*argv) {
                    device_kv[i] = convert_char_to_hex(*argv);
                }
            }
        } else if (strcmp(*argv, "-skv") == 0) {
            argv++;
            if (*argv)
                stream_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-ikv") == 0) {
            argv++;
            if (*argv) {
                instance_kv = atoi(*argv);
            }
        } else if (strcmp(*argv, "-dppkv") == 0) {
            devicepp_kv = (unsigned int *) realloc(devicepp_kv, intf_num * sizeof(unsigned int));
            if (!devicepp_kv) {
                printf(" insufficient memory\n");
                return 1;
            }
            for (i = 0; i < intf_num ; i++) {
                devicepp_kv[i] = DEVICEPP_RX_AUDIO_MBDRC;
            }
            for (i = 0; i < intf_num ; i++)
            {
                argv++;
                if(*argv) {
                    devicepp_kv[i] = convert_char_to_hex(*argv);
                }
            }
        } else if (strcmp(*argv, "-help") == 0) {
            usage();
        }

        if (*argv)
            argv++;
    }

    if (intf_name == NULL)
        return 1;

    play_samples(file, card, device, device_kv, stream_kv, instance_kv, devicepp_kv,
                 buffer_size, frag, audio_format, pause, 
                 intf_name, intf_num);

    fprintf(stderr, "Finish Playing.... Close Normally\n");
    if (device_kv)
        free(device_kv);
    if (devicepp_kv)
        free(devicepp_kv);
    if (intf_name)
        free(intf_name);
    exit(EXIT_SUCCESS);
}

void play_samples(char *name, unsigned int card, unsigned int device, unsigned int *device_kv,
                  unsigned int stream_kv, unsigned int instance_kv, unsigned int *devicepp_kv,
                  unsigned long buffer_size, unsigned int frag, unsigned int format,
                  int pause, char **intf_name, int intf_num)
{
    struct compr_config config;
    struct snd_codec codec;
    struct compress *compress;
    struct mp3_header header;
    struct adts_header adts_header;
    struct mixer *mixer;
    FILE *file;
    char *buffer;
    int num_read, wrote;
    unsigned int channels = 0, rate = 0, bits = 0;
    struct device_config *dev_config = NULL;
    struct group_config *grp_config = NULL;
    int size, index, ret = 0;
    uint32_t miid = 0;

    dev_config = (struct device_config *) malloc(intf_num * sizeof(struct device_config));
    if (!dev_config) {
        printf("Failed to allocate memory for dev config");
        return;
    }
    grp_config = (struct group_config *) malloc(intf_num * sizeof(struct group_config));
    if (!grp_config) {
        printf("Failed to allocate memory for group config");
        return;
    }

    stream_kv = stream_kv ? stream_kv : COMPRESSED_OFFLOAD_PLAYBACK;

    if (verbose)
        printf("%s: entry\n", __func__);
    file = fopen(name, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file '%s'\n", name);
        exit(EXIT_FAILURE);
    }

    if (format == MP3_FORMAT) {
        fread(&header, sizeof(header), 1, file);

        if (parse_mp3_header(&header, &channels, &rate, &bits) == -1) {
            fclose(file);
            exit(EXIT_FAILURE);
        }

        codec.id = SND_AUDIOCODEC_MP3;
#ifdef SND_COMPRESS_DEC_HDR
    } else if (format == AAC_ADTS_FORMAT) {
        uint16_t protection_absent, crc;
        fread(&adts_header, sizeof(adts_header), 1, file);
        if (parse_aac_header(&adts_header, &channels, &rate, (unsigned int*)&protection_absent) == -1) {
            fclose(file);
            exit(EXIT_FAILURE);
        }
        if (!protection_absent) {
            fread(&crc, 2, 1, file);
        }
        codec.id = SND_AUDIOCODEC_AAC;
        codec.format = SND_AUDIOSTREAMFORMAT_MP4ADTS;
        bits = 16;
        codec.options.aac_dec.audio_obj_type = 29;
        codec.options.aac_dec.pce_bits_size = 0;
#endif
    } else {
        printf("unknown format");
    }
    codec.ch_in = channels;
    codec.ch_out = channels;
    codec.sample_rate = rate;
    if (!codec.sample_rate) {
        fprintf(stderr, "invalid sample rate %d\n", rate);
        fclose(file);
        exit(EXIT_FAILURE);
    }
    codec.bit_rate = bits;
    codec.rate_control = 0;
    codec.profile = 0;
    codec.level = 0;
    codec.ch_mode = 0;
    codec.format = 0;
    if ((buffer_size != 0) && (frag != 0)) {
        config.fragment_size = buffer_size/frag;
        config.fragments = frag;
    } else {
        /* use driver defaults */
        config.fragment_size = 0;
        config.fragments = 0;
    }
    config.codec = &codec;

    mixer = mixer_open(card);
    if (!mixer) {
        printf("Failed to open mixer\n");
        goto FILE_EXIT;
    }
    for (index = 0; index < intf_num; index++) {
        ret = get_device_media_config(BACKEND_CONF_FILE, intf_name[index], &dev_config[index]);
        if (ret) {
            printf("Invalid input, entry not found for : %s\n", intf_name[index]);
            fclose(file);
        }
        printf("Backend %s rate ch bit : %d, %d, %d\n", intf_name[index],
            dev_config[index].rate, dev_config[index].ch, dev_config[index].bits);

        /* set device/audio_intf media config mixer control */
        if (set_agm_device_media_config(mixer, intf_name[index], &dev_config[index])) {
            printf("Failed to set device media config\n");
            goto MIXER_EXIT;
        }

        /* set audio interface metadata mixer control */
        if (set_agm_audio_intf_metadata(mixer, intf_name[index], device_kv[index], PLAYBACK,
                                    dev_config[index].rate, dev_config[index].bits, stream_kv)) {
            printf("Failed to set device metadata\n");
            goto MIXER_EXIT;
        }
    }

    /* set audio interface metadata mixer control */
    if (set_agm_stream_metadata(mixer, device, stream_kv, PLAYBACK, STREAM_COMPRESS,
                                instance_kv)) {
        printf("Failed to set stream metadata\n");
        goto MIXER_EXIT;
    }

    /* Note:  No common metadata as of now*/
    for (index = 0; index < intf_num; index++) {
         if (devicepp_kv[index] != 0) {
             if (set_agm_streamdevice_metadata(mixer, device, stream_kv, PLAYBACK, STREAM_COMPRESS, intf_name[index],
                                               devicepp_kv[index])) {
                 printf("Failed to set pcm metadata\n");
                 goto MIXER_EXIT;
             }
         }

        /* connect stream to audio intf */
        if (connect_agm_audio_intf_to_stream(mixer, device, intf_name[index], STREAM_COMPRESS, true)) {
            printf("Failed to connect pcm to audio interface\n");
            goto MIXER_EXIT;
        }

        ret = agm_mixer_get_miid (mixer, device, intf_name[index], STREAM_PCM, PER_STREAM_PER_DEVICE_MFC, &miid);
        if (ret) {
            printf("MFC not present for this graph\n");
        } else {
            if (configure_mfc(mixer, device, intf_name[index], PER_STREAM_PER_DEVICE_MFC,
                           STREAM_COMPRESS, dev_config[index].rate, dev_config[index].ch,
                           dev_config[index].bits, miid)) {
                printf("Failed to configure pspd mfc\n");
                goto MIXER_EXIT;
            }
        }

        if (strstr(intf_name[index], "VIRT-")) {
            if (get_group_device_info(BACKEND_CONF_FILE, intf_name[index], &grp_config[index]))
                goto MIXER_EXIT;

            if (set_agm_group_device_config(mixer, intf_name[index], &grp_config[index])) {
                printf("Failed to set grp device config\n");
                goto MIXER_EXIT;
            }
        }
    }


    compress = compress_open(card, device, COMPRESS_IN, &config);
    if (!compress || !is_compress_ready(compress)) {
        fprintf(stderr, "Unable to open Compress device %d:%d\n",
                card, device);
        fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
        goto MIXER_EXIT;
    };
    if (verbose)
        printf("%s: Opened compress device\n", __func__);

    for (index = 0; index < intf_num; index++) {
        if (strstr(intf_name[index], "VIRT-") || (device_kv[index] == SPEAKER) || (device_kv[index] == HANDSET)) {
            if (set_agm_group_mux_config(mixer, device, &grp_config[index], intf_name[index], dev_config[index].ch)) {
                printf("Failed to set grp device config\n");
            }
        }
    }
    size = config.fragment_size;
    buffer = malloc(size * config.fragments);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        goto COMP_EXIT;
    }

    /* we will write frag fragment_size and then start */
    num_read = fread(buffer, 1, size * config.fragments, file);
    if (num_read > 0) {
        if (verbose)
            printf("%s: Doing first buffer write of %d\n", __func__, num_read);
        wrote = compress_write(compress, buffer, num_read);
        if (wrote < 0) {
            fprintf(stderr, "Error %d playing sample\n", wrote);
            fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
            goto BUF_EXIT;
        }
        if (wrote != num_read) {
            /* TODO: Buufer pointer needs to be set here */
            fprintf(stderr, "We wrote %d, DSP accepted %d\n", num_read, wrote);
        }
    }
    printf("Playing file %s On Card %u device %u, with buffer of %lu bytes\n",
            name, card, device, buffer_size);
    printf("Format %u Channels %u, %u Hz, Bit Rate %d\n",
            SND_AUDIOCODEC_MP3, channels, rate, bits);

    compress_start(compress);
    if (verbose)
        printf("%s: You should hear audio NOW!!!\n", __func__);

    do {
        num_read = fread(buffer, 1, size, file);
        if (num_read > 0) {
            wrote = compress_write(compress, buffer, num_read);
            if (wrote < 0) {
                fprintf(stderr, "Error playing sample\n");
                fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
                goto BUF_EXIT;
            }
            if (wrote != num_read) {
                /* TODO: Buffer pointer needs to be set here */
                fprintf(stderr, "We wrote %d, DSP accepted %d\n", num_read, wrote);
            }
            if (verbose) {
                print_time(compress);
                printf("%s: wrote %d\n", __func__, wrote);
            }
            if (pause) {
                printf("%s: pause \n", __func__);
                compress_pause(compress);
                sleep(2);
                printf("%s: resume \n", __func__);
                compress_resume(compress);
            }
        }
    } while (num_read > 0);

    if (verbose)
        printf("%s: exit success\n", __func__);
    /* issue drain if it supports */
    compress_drain(compress);
    /* disconnect stream to audio intf */
    for(index = 0; index < intf_num; index++) {
        connect_agm_audio_intf_to_stream(mixer, device, intf_name[index], STREAM_COMPRESS, false);
    }
    free(buffer);
    fclose(file);
    compress_close(compress);
    return;
BUF_EXIT:
    free(buffer);
COMP_EXIT:
    compress_close(compress);
MIXER_EXIT:
    if (dev_config)
        free(dev_config);
    if (grp_config)
        free(grp_config);
    mixer_close(mixer);
FILE_EXIT:
    fclose(file);
    if (verbose)
        printf("%s: exit failure\n", __func__);
    exit(EXIT_FAILURE);
}
