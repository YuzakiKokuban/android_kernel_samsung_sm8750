/*
** Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
**
** Changes from Qualcomm Innovation Center are provided under the following license:
** Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "agmmixer.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

#define FORMAT_PCM 1

struct wav_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;
};

int capturing = 1;

static unsigned int capture_sample(FILE *file, unsigned int card, unsigned int device,
                            unsigned int usb_device, unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count, unsigned int cap_time,
                            struct device_config *dev_config, unsigned int stream_kv,
                            unsigned int device_kv, unsigned int instance_kv,
                            unsigned int devicepp_kv);

static void sigint_handler(int sig)
{
    capturing = 0;
}

static void usage(void)
{
    printf(" Usage: %s file.wav [-help print usage] [-D card] [-d device]\n"
           " [-c channels] [-r rate] [-b bits] [-p period_size]\n"
           " [-n n_periods] [-T capture time] [-i intf_name] [-dkv device_kv]\n"
           " [-dppkv deviceppkv] : Assign 0 if no device pp in the graph\n"
           " [-ikv instance_kv] :  Assign 0 if no instance kv in the graph\n"
           " [-skv stream_kv]\n"
           " [-is_24_LE] : [0-1] Only to be used if user wants to record 32 bps clip\n"
           " [-usb_d usb device]\n"
           " 0: If bps is 32, and format should be S32_LE\n"
           " 1: If bps is 24, and format should be S24_LE\n");
}

int main(int argc, char **argv)
{
    FILE *file;
    struct wav_header header;
    unsigned int card = 100;
    unsigned int device = 101;
    unsigned int usb_device = 1;
    unsigned int channels = 2;
    unsigned int rate = 44100;
    unsigned int bits = 16;
    unsigned int frames;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
    unsigned int cap_time = 0;
    char *intf_name = NULL;
    unsigned int device_kv = 0;
    struct device_config config;
    enum pcm_format format;
    int ret = 0;
    unsigned int devicepp_kv = 0;
    unsigned int stream_kv = 0;
    unsigned int instance_kv = INSTANCE_1;
    bool is_24_LE = false;

    if (argc < 2) {
        usage();
        return 1;
    }

    file = fopen(argv[1], "wb");
    if (!file) {
        printf("Unable to create file '%s'\n", argv[1]);
        return 1;
    }

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
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        } else if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
        } else if (strcmp(*argv, "-T") == 0) {
            argv++;
            if (*argv)
                cap_time = atoi(*argv);
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
        } else if (strcmp(*argv, "-is_24_LE") == 0) {
            argv++;
            if (*argv)
                is_24_LE = atoi(*argv);
        } else if (strcmp(*argv, "-usb_d") == 0) {
            argv++;
            if (*argv)
                usb_device = atoi(*argv);
        }else if (strcmp(*argv, "-help") == 0) {
            usage();
        }
        if (*argv)
            argv++;
    }

    header.riff_id = ID_RIFF;
    header.riff_sz = 0;
    header.riff_fmt = ID_WAVE;
    header.fmt_id = ID_FMT;
    header.fmt_sz = 16;
    header.audio_format = FORMAT_PCM;
    header.num_channels = channels;
    header.sample_rate = rate;

    switch (bits) {
    case 32:
        if (is_24_LE)
            format = PCM_FORMAT_S24_LE;
        else
            format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_3LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        printf("%u bits is not supported.\n", bits);
        fclose(file);
        return 1;
    }

    if (intf_name == NULL)
        return 1;

    ret = get_device_media_config(BACKEND_CONF_FILE, intf_name, &config);
    if (ret) {
        printf("Invalid input, entry not found for %s\n", intf_name);
        fclose(file);
        return ret;
    }
    if (config.format != PCM_FORMAT_INVALID) {
        printf("Valid format from backend_conf %d\n", config.format);
        config.bits = get_pcm_bit_width(config.format);
    }

    header.bits_per_sample = pcm_format_to_bits(format);
    header.byte_rate = (header.bits_per_sample / 8) * channels * rate;
    header.block_align = channels * (header.bits_per_sample / 8);
    header.data_id = ID_DATA;

    /* leave enough room for header */
    fseek(file, sizeof(struct wav_header), SEEK_SET);

    /* install signal handler and begin capturing */
    signal(SIGINT, sigint_handler);
    signal(SIGHUP, sigint_handler);
    signal(SIGTERM, sigint_handler);
    frames = capture_sample(file, card, device, usb_device, header.num_channels,
                            header.sample_rate, format,
                            period_size, period_count, cap_time, &config,
                            stream_kv, device_kv, instance_kv, devicepp_kv);
    printf("Captured %u frames\n", frames);

    /* write header now all information is known */
    header.data_sz = frames * header.block_align;
    header.riff_sz = header.data_sz + sizeof(header) - 8;
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(struct wav_header), 1, file);

    fclose(file);

    return 0;
}

unsigned int capture_sample(FILE *file, unsigned int card, unsigned int device,
                            unsigned int usb_device, unsigned int channels, unsigned int rate,
                            enum pcm_format format, unsigned int period_size,
                            unsigned int period_count, unsigned int cap_time,
                            struct device_config *dev_config, unsigned int stream_kv,
                            unsigned int device_kv, unsigned int instance_kv, unsigned int devicepp_kv)
{
    struct pcm_config config;
    struct pcm *pcm;
    struct mixer *mixer;
    char *buffer;
    char *intf_name = dev_config->name;
    unsigned int size;
    unsigned int bytes_read = 0;
    unsigned int frames = 0;
    struct timespec end;
    struct timespec now;
    uint32_t miid = 0;
    int ret = 0;
    struct usbAudioConfig cfg;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;

    stream_kv = stream_kv ? stream_kv : PCM_RECORD;

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    if (NULL == intf_name) {
        printf("No interface name mentioned, Exiting !!!\n");
        return 0;
    }

    mixer = mixer_open(card);
    if (!mixer) {
        printf("Failed to open mixer\n");
        return 0;
    }

    if(strcmp(intf_name, "USB_AUDIO-TX") == 0) {
        dev_config->rate = rate;
        dev_config->ch = channels;
    }

    /* set device/audio_intf media config mixer control */
    if (set_agm_device_media_config(mixer, intf_name, dev_config)) {
        printf("Failed to set device media config\n");
        goto err_close_mixer;
    }

    /* set audio interface metadata mixer control */
    if (set_agm_audio_intf_metadata(mixer, intf_name, device_kv, CAPTURE,
                                    dev_config->rate, dev_config->bits, stream_kv)) {
        printf("Failed to set device metadata\n");
        goto err_close_mixer;
    }

    /* set stream metadata mixer control */
    if (set_agm_capture_stream_metadata(mixer, device, stream_kv, CAPTURE, STREAM_PCM,
                                        instance_kv)) {
        printf("Failed to set pcm metadata\n");
        goto err_close_mixer;
    }

    if (devicepp_kv != 0) {
        if (set_agm_streamdevice_metadata(mixer, device, stream_kv, CAPTURE, STREAM_PCM,
                                intf_name, devicepp_kv)) {
            printf("Failed to set pcm metadata\n");
            goto err_close_mixer;
        }
    }

    ret = agm_mixer_get_miid (mixer, device, intf_name, STREAM_PCM, TAG_STREAM_MFC, &miid);
    if (ret) {
        printf("MFC not present for this graph\n");
    } else {
        if (configure_mfc(mixer, device, intf_name, TAG_STREAM_MFC,
                     STREAM_PCM, rate, channels, get_pcm_bit_width(format), miid)) {
            printf("Failed to configure stream mfc\n");
            goto err_close_mixer;
        }
    }

    if (strcmp(intf_name, "USB_AUDIO-TX") == 0) {
        ret = agm_mixer_get_miid (mixer, device, intf_name, STREAM_PCM, DEVICE_HW_ENDPOINT_TX, &miid);
        if (ret == 0) {
            cfg.usb_token = (usb_device << 16)|0x1;
            cfg.svc_interval = 0;
            get_agm_usb_audio_config_payload(&payload, &payloadSize, miid, &cfg);

            if (payloadSize) {
                ret = set_agm_device_custom_payload(mixer, intf_name, payload, payloadSize);
            } else {
                ret = -1;
                printf("set_agm_device_custom_payload failed\n");
                goto err_close_mixer;
            }
        } else {
            printf("Failed to get miid for USB_AUDIO-TX\n");
            goto err_close_mixer;
        }

    }

    /* connect pcm stream to audio intf */
    if (connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_PCM, true)) {
        printf("Failed to connect pcm to audio interface\n");
        goto err_close_mixer;
    }

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        printf("Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        goto err_close_mixer;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = (char *)malloc(size);
    if (!buffer) {
        printf("Unable to allocate %u bytes\n", size);
        goto err_close_pcm;
    }

    printf("Capturing sample: %u ch, %u hz, %u bit\n", channels, rate,
           pcm_format_to_bits(format));

    if (pcm_start(pcm) < 0) {
        printf("start error\n");
        goto err_close_pcm;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;

    while (capturing && !pcm_read(pcm, buffer, size)) {
        if (fwrite(buffer, 1, size, file) != size) {
            printf("Error capturing sample\n");
            break;
        }
        bytes_read += size;
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
                break;
        }
    }

    frames = pcm_bytes_to_frames(pcm, bytes_read);
    free(buffer);

    pcm_stop(pcm);
err_close_pcm:
    connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_PCM, false);
    pcm_close(pcm);
err_close_mixer:
    if (payload) {
      free(payload);
    }
    mixer_close(mixer);
    return frames;
}
