/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 * This code is used under the BSD license.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2011-2012, Intel Corporation
 * Copyright (c) 2013-2014, Wolfson Microelectronic Ltd.
 * All rights reserved.
 *
 * Author: Vinod Koul <vinod.koul@linux.intel.com>
 * Author: Charles Keepax <ckeepax@opensource.wolfsonmicro.com>
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
 * Copyright (c) 2011-2012, Intel Corporation
 * Copyright (c) 2013-2014, Wolfson Microelectronic Ltd.
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
#include <sys/types.h>
#include <sys/stat.h>
#define __force
//#define __bitwise
#define __user
#include <sound/compress_params.h>
#include <sound/compress_offload.h>
#include <tinycompress/tinycompress.h>

#include "agmmixer.h"

static int verbose;
static int file;
static FILE *finfo;
static bool streamed;

static const unsigned int DEFAULT_CHANNELS = 1;
static const unsigned int DEFAULT_RATE = 44100;
static const unsigned int DEFAULT_FORMAT = SNDRV_PCM_FORMAT_S16_LE;

struct riff_chunk {
    char desc[4];
    uint32_t size;
} __attribute__((__packed__));

struct wave_header {
    struct {
        struct riff_chunk chunk;
        char format[4];
    } __attribute__((__packed__)) riff;

    struct {
        struct riff_chunk chunk;
        uint16_t type;
        uint16_t channels;
        uint32_t rate;
        uint32_t byterate;
        uint16_t blockalign;
        uint16_t samplebits;
    } __attribute__((__packed__)) fmt;

    struct {
        struct riff_chunk chunk;
    } __attribute__((__packed__)) data;
} __attribute__((__packed__));

static const struct wave_header blank_wave_header = {
    .riff = {
        .chunk = {
            .desc = "RIFF",
        },
        .format = "WAVE",
    },
    .fmt = {
        .chunk = {
            .desc = "fmt ", /* Note the space is important here */
            .size = sizeof(blank_wave_header.fmt) -
                sizeof(blank_wave_header.fmt.chunk),
        },
        .type = 0x01,   /* PCM */
    },
    .data = {
        .chunk = {
            .desc = "data",
        },
    },
};

static void init_wave_header(struct wave_header *header, uint16_t channels,
                 uint32_t rate, uint16_t samplebits)
{
    memcpy(header, &blank_wave_header, sizeof(blank_wave_header));

    header->fmt.channels = channels;
    header->fmt.rate = rate;
    header->fmt.byterate = channels * rate * (samplebits / 8);
    header->fmt.blockalign = channels * (samplebits / 8);
    header->fmt.samplebits = samplebits;
}

static void size_wave_header(struct wave_header *header, uint32_t size)
{
    header->riff.chunk.size = sizeof(*header) -
                  sizeof(header->riff.chunk) + size;
    header->data.chunk.size = size;
}

static void usage(void)
{
    fprintf(stderr, "usage: crec [OPTIONS] [filename]\n"
        "-D\tcard number\n"
        "-d\tdevice node\n"
        "-buf\tbuffer size\n"
        "-f\tfragments\n"
        "-v\tverbose mode\n"
        "-l\tlength of record in seconds\n"
        " [-help print usage]\n"
        " [-c channels] [-r rate] [-b bits]\n"
        " [-n n_periods] [-i intf_name] [-dkv device_kv]\n"
        " [-dppkv deviceppkv] : Assign 0 if no device pp in the graph\n"
        " [-ikv instance_kv] :  Assign 0 if no instance kv in the graph\n"
        " [-skv stream_kv]");

    exit(EXIT_FAILURE);
}

static int print_time(struct compress *compress)
{
    unsigned int avail;
    struct timespec tstamp;

    if (compress_get_hpointer(compress, &avail, &tstamp) != 0) {
        fprintf(stderr, "Error querying timestamp\n");
        fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
        return -1;
    } else {
        fprintf(finfo, "DSP recorded %jd.%jd\n",
               (intmax_t)tstamp.tv_sec, (intmax_t)tstamp.tv_nsec*1000);
    }
    return 0;
}

static int finish_record(void)
{
    struct wave_header header;
    int ret;
    size_t nread, written;

    if (!file)
        return -ENOENT;

    /* can't rewind if streaming to stdout */
    if (streamed)
        return 0;

    /* Get amount of data written to file */
    ret = lseek(file, 0, SEEK_END);
    if (ret < 0)
        return -errno;

    written = ret;
    if (written < sizeof(header))
        return -ENOENT;
    written -= sizeof(header);

    /* Sync file header from file */
    ret = lseek(file, 0, SEEK_SET);
    if (ret < 0)
        return -errno;

    nread = read(file, &header, sizeof(header));
    if (nread != sizeof(header))
        return -errno;

    /* Update file header */
    ret = lseek(file, 0, SEEK_SET);
    if (ret < 0)
        return -errno;

    size_wave_header(&header, written);

    written = write(file, &header, sizeof(header));
    if (written != sizeof(header))
        return -errno;

    return 0;
}

static void capture_samples(char *name, unsigned int card, unsigned int device,
             unsigned long buffer_size, unsigned int frag,
             unsigned int length, unsigned int rate,
             unsigned int channels, unsigned int format, struct device_config *dev_config, unsigned int stream_kv,
             unsigned int device_kv, unsigned int instance_kv, unsigned int devicepp_kv)
{
    struct compr_config config;
    struct snd_codec codec;
    struct compress *compress;
    struct mixer *mixer;
    struct wave_header header;
    char *buffer;
    size_t written;
    int read, ret;
    unsigned int size, total_read = 0;
    unsigned int samplebits;
    uint32_t miid = 0;
    char *intf_name = dev_config->name;
    //TODO: Change default stream_kv to COMPRESS_RECORD later.
    stream_kv = stream_kv ? stream_kv : PCM_RECORD;

    switch (format) {
    case SNDRV_PCM_FORMAT_S32_LE:
        samplebits = 32;
        break;
    default:
        samplebits = 16;
        break;
    }

    /* Convert length from seconds to bytes */
    length = length * rate * (samplebits / 8) * channels;

    if (verbose)
        fprintf(finfo, "%s: entry, reading %u bytes\n", __func__, length);
        if (!name) {
            file = STDOUT_FILENO;
            exit(EXIT_FAILURE);
        } else {
            file = open(name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
            if (file == -1) {
               fprintf(stderr, "Unable to open file '%s'\n", name);
               exit(EXIT_FAILURE);
            }
        }

    /* Write a header, will update with size once record is complete */
    if (!streamed) {
        init_wave_header(&header, channels, rate, samplebits);
        written = write(file, &header, sizeof(header));
        if (written != sizeof(header)) {
            fprintf(stderr, "Error writing output file header: %s\n",
                    strerror(errno));
            goto file_exit;
        }
    }

    memset(&codec, 0, sizeof(codec));
    memset(&config, 0, sizeof(config));
    codec.id = SND_AUDIOCODEC_PCM;
    codec.ch_in = channels;
    codec.ch_out = channels;
    codec.sample_rate = rate;
    if (!codec.sample_rate) {
        fprintf(stderr, "invalid sample rate %d\n", rate);
        goto file_exit;
    }
    codec.format = format;
    if ((buffer_size != 0) && (frag != 0)) {
        config.fragment_size = buffer_size/frag;
        config.fragments = frag;
    }
    config.codec = &codec;

    mixer = mixer_open(card);
    if (!mixer) {
        printf("Failed to open mixer\n");
        goto file_exit;
    }

    /* set device/audio_intf media config mixer control */
    if (set_agm_device_media_config(mixer, intf_name, dev_config)) {
        printf("Failed to set device media config\n");
        goto mixer_exit;
    }

    /* set audio interface metadata mixer control */
    if (set_agm_audio_intf_metadata(mixer, intf_name, device_kv, CAPTURE,
                                    dev_config->rate, dev_config->bits, stream_kv)) {
        printf("Failed to set device metadata\n");
        goto mixer_exit;
    }

    /* set audio interface metadata mixer control */
        /* Change pcm_record to compress_record */
    if (set_agm_capture_stream_metadata(mixer, device, stream_kv, CAPTURE, STREAM_COMPRESS,
                                        instance_kv)) {
        printf("Failed to set pcm metadata\n");
        goto mixer_exit;
    }

    if (devicepp_kv != 0) {
        if (set_agm_streamdevice_metadata(mixer, device, stream_kv, CAPTURE, STREAM_COMPRESS,
                                intf_name, devicepp_kv)) {
            printf("Failed to set pcm metadata\n");
            goto mixer_exit;
        }
    }

    ret = agm_mixer_get_miid (mixer, device, intf_name, STREAM_COMPRESS, TAG_STREAM_MFC, &miid);
    if (ret) {
        printf("MFC not present for this graph");
    } else {
        if (configure_mfc(mixer, device, intf_name, TAG_STREAM_MFC,
                     STREAM_COMPRESS, rate, channels, pcm_format_to_bits(format), miid)) {
            printf("Failed to configure pspd mfc\n");
            goto mixer_exit;
        }
    }

    /* Note:  No common metadata as of now*/

    /* connect stream to audio intf */
    if (connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_COMPRESS, true)) {
        printf("Failed to connect stream to audio interface\n");
        goto mixer_exit;
    }

    compress = compress_open(card, device, COMPRESS_OUT, &config);
    if (!compress || !is_compress_ready(compress)) {
        fprintf(stderr, "Unable to open Compress device %d:%d\n",
            card, device);
        fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
        goto mixer_exit;
    };

    if (verbose)
        fprintf(finfo, "%s: Opened compress device\n", __func__);

    size = config.fragment_size;
    buffer = malloc(size * config.fragments);
    if (!buffer) {
        fprintf(stderr, "Unable to allocate %d bytes\n", size);
        goto comp_exit;
    }

    fprintf(finfo, "Recording file %s On Card %u device %u, with buffer of %lu bytes\n",
           name, card, device, buffer_size);
    fprintf(finfo, "Codec %u Format %u Channels %u, %u Hz\n",
           codec.id, codec.format, codec.ch_out, rate);

    compress_start(compress);

    if (verbose)
        fprintf(finfo, "%s: Capturing audio NOW!!!\n", __func__);

    do {
        read = compress_read(compress, buffer, size);
        if (read < 0) {
            fprintf(stderr, "Error reading sample\n");
            fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
            goto buf_exit;
        }
        if ((unsigned int)read != size) {
            fprintf(stderr, "We read %d, DSP sent %d\n",
                size, read);
        }

        if (read > 0) {
            total_read += read;

            written = write(file, buffer, read);
            if (written != (size_t)read) {
                fprintf(stderr, "Error writing output file: %s\n",
                    strerror(errno));
                goto buf_exit;
            }
            if (verbose) {
                print_time(compress);
                fprintf(finfo, "%s: read %d\n", __func__, read);
            }
        }
    } while (!length || total_read < length);

    ret = compress_stop(compress);
    if (ret < 0) {
        fprintf(stderr, "Error closing stream\n");
        fprintf(stderr, "ERR: %s\n", compress_get_error(compress));
    }

    ret = finish_record();
    if (ret < 0) {
        fprintf(stderr, "Failed to finish header: %s\n", strerror(ret));
        goto buf_exit;
    }

    if (verbose)
        fprintf(finfo, "%s: exit success\n", __func__);

    free(buffer);
    close(file);
    file = 0;

    compress_close(compress);

    return;
buf_exit:
    free(buffer);
comp_exit:
    compress_close(compress);
    connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_COMPRESS, false);
mixer_exit:
    mixer_close(mixer);
file_exit:
    close(file);

    if (verbose)
        fprintf(finfo, "%s: exit failure\n", __func__);

    exit(EXIT_FAILURE);
}

static void sig_handler(int signum __attribute__ ((unused)))
{
    finish_record();

    if (file)
        close(file);

    _exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    char *file = NULL;
    unsigned long buffer_size = 0;
    unsigned int card = 0, device = 0, frag = 0, length = 0;
    unsigned int rate = DEFAULT_RATE, channels = DEFAULT_CHANNELS;
    unsigned int bits = 16;
    unsigned int format = DEFAULT_FORMAT;
    char* intf_name = NULL;
    int ret = 0;
    unsigned int devicepp_kv = DEVICEPP_TX_AUDIO_FLUENCE_SMECNS;
    unsigned int stream_kv = 0;
    unsigned int instance_kv = INSTANCE_1;
    struct device_config config;
    unsigned int device_kv = 0;

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        fprintf(stderr, "Error registering signal handler\n");
        exit(EXIT_FAILURE);
    }

    if (argc < 3)
        usage();

    verbose = 0;

    file = argv[1];
    /* parse command line arguments */
    argv += 2;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            argv++;
            if (*argv)
                device = atoi(*argv);
        } else if (strcmp(*argv, "-c") == 0) {
            argv++;
            if (*argv)
                channels = atoi(*argv);
        } else if (strcmp(*argv, "-r") == 0) {
            argv++;
            if (*argv)
                rate = atoi(*argv);
        } else if (strcmp(*argv, "-b") == 0) {
            argv++;
            if (*argv)
                bits = atoi(*argv);
        } else if (strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                card = atoi(*argv);
        } else if (strcmp(*argv, "-l") == 0) {
            argv++;
            if (*argv)
                length = atoi(*argv);
        } else if (strcmp(*argv, "-i") == 0) {
            argv++;
            if (*argv)
                intf_name = *argv;
        } else if (strcmp(*argv, "-dkv") == 0) {
            argv++;
            if (*argv)
                device_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-skv") == 0) {
            argv++;
            if (*argv)
                stream_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-ikv") == 0) {
            argv++;
            if (*argv)
                instance_kv = atoi(*argv);
        } else if (strcmp(*argv, "-dppkv") == 0) {
            argv++;
            if (*argv)
                devicepp_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-buf") == 0) {
            argv++;
            if (*argv)
                 buffer_size = atoi(*argv);
        } else if (strcmp(*argv, "-f") == 0) {
            argv++;
            if (*argv)
                frag = atoi(*argv);
        } else if (strcmp(*argv, "-v") == 0) {
            argv++;
            if (*argv)
                verbose = atoi(*argv);
        } else if (strcmp(*argv, "-help") == 0) {
            usage();
        }
        if (*argv)
            argv++;
    }

    if (intf_name == NULL) {
        printf("Invalid audio interface index denoted by -i\n");
        exit(EXIT_FAILURE);
    }

    ret = get_device_media_config(BACKEND_CONF_FILE, intf_name, &config);
    if (ret) {
        printf("Invalid input, entry not found for %s\n", intf_name);
        fclose(file);
        return ret;
    }

    switch (bits) {
    case 32:
        format = SNDRV_PCM_FORMAT_S32_LE;
        break;
    default:
        format = SNDRV_PCM_FORMAT_S16_LE;
        break;
    }
    capture_samples(file, card, device, buffer_size, frag, length,
            rate, channels, format, &config, stream_kv, device_kv, instance_kv,
             devicepp_kv);

    fprintf(finfo, "Finish capturing... Close Normally\n");

    exit(EXIT_SUCCESS);
}

