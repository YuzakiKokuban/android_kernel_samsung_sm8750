/*
** Copyright (c) 2019, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** Changes from Qualcomm Innovation Center are provided under the following license:
** Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include "agmmixer.h"

static int close_f = 0;

void sigint_handler(int sig)
{
    close_f = 1;
}

void play_loopback(unsigned int card, unsigned int p_device, unsigned int c_device, unsigned int channels,
                  unsigned int rate, enum pcm_format format, struct device_config *cap_config,
                  struct device_config *p_config, unsigned int period_size,
                  unsigned int period_count, unsigned int play_cap_time, char* capture_intf,
                  char* play_intf, unsigned int pdkv, unsigned int cdkv, unsigned int stream_kv,
                  unsigned int do_loopback);

void usage()
{
    printf(" Usage: %s [-D card] [-P Hostless Playback device] [-C Hostless Capture device] [-p period_size]\n"
           " [-n n_periods]  [-c channels] [-r rate] [-b bits] [-T playback/capture time ]\n"
           " [-i capture intf] [-o playback intf]\n"
           " [-cdkv capture_device_kv] [-pdkv playback_device_kv] [-skv stream_kv]\n"
           " Used to enable 'hostless' mode for audio devices with a DSP back-end.\n"
           " Alternatively, specify '-l' for loopback mode: this program will read\n"
           " from the capture device and write to the playback device.\n");
}

int main(int argc, char **argv)
{
    unsigned int card = 100;
    unsigned int p_device = 1;
    unsigned int c_device = 0;
    unsigned int period_size = 1024;
    unsigned int period_count = 4;
    unsigned int bits = 16;
    unsigned int num_channels = 2;
    unsigned int sample_rate = 48000;
    unsigned int play_cap_time = 10;
    char* c_intf_name = NULL;
    char* p_intf_name = NULL;
    unsigned int do_loopback = 0;
    enum pcm_format format;
    struct device_config capture_config;
    struct device_config play_config;
    unsigned int c_device_kv = 0;
    unsigned int p_device_kv = 0;
    unsigned int stream_kv = 0;
    unsigned int ret = 0;

    if (argc < 2) {
        usage();
        return 1;
    }

   /* parse command line arguments */
    argv += 1;
    while (*argv) {
        if (strcmp(*argv, "-P") == 0) {
            argv++;
            if (*argv)
                p_device = atoi(*argv);
        } else if (strcmp(*argv, "-C") == 0) {
            argv++;
            if (*argv)
                c_device = atoi(*argv);
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        } else if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
        }  else if (strcmp(*argv, "-c") == 0) {
            argv++;
            if (*argv)
                num_channels = atoi(*argv);
        } else if (strcmp(*argv, "-r") == 0) {
            argv++;
            if (*argv)
                sample_rate = atoi(*argv);
        } else if (strcmp(*argv, "-b") == 0) {
            argv++;
            if (*argv)
                bits = atoi(*argv);
        } else if (strcmp(*argv, "-T") == 0) {
            argv++;
            if (*argv)
                play_cap_time = atoi(*argv);
        } else if (strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                card = atoi(*argv);
        } else if (strcmp(*argv, "-i") == 0) {
            argv++;
            if (*argv)
                c_intf_name = *argv;
        } else if (strcmp(*argv, "-o") == 0) {
            argv++;
            if (*argv)
                p_intf_name = *argv;
        } else if (strcmp(*argv, "-l") == 0) {
            do_loopback = 1;
        } else if (strcmp(*argv, "-skv") == 0) {
            argv++;
            if (*argv)
                stream_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-cdkv") == 0) {
            argv++;
            if (*argv)
                c_device_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-pdkv") == 0) {
            argv++;
            if (*argv)
                p_device_kv = convert_char_to_hex(*argv);
        } else if (strcmp(*argv, "-help") == 0) {
            usage();
        }

        if (*argv)
            argv++;
    }

    ret = get_device_media_config(BACKEND_CONF_FILE, c_intf_name, &capture_config);
    if (ret) {
        printf("Invalid input, assigning default values for : %s\n", c_intf_name);
        capture_config.rate = sample_rate;
        capture_config.bits = bits;
        capture_config.ch = num_channels;
    }

    ret = get_device_media_config(BACKEND_CONF_FILE, p_intf_name, &play_config);
    if (ret) {
        printf("Invalid input, assigning default values for : %s\n", p_intf_name);
        play_config.rate = sample_rate;
        play_config.bits = bits;
        play_config.ch = num_channels;
    }

    switch (bits) {
    case 32:
        format = PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = PCM_FORMAT_S16_LE;
        break;
    default:
        printf("%u bits is not supported.\n", bits);
        return 1;
    }
    play_loopback(card, p_device, c_device, num_channels, sample_rate, format, &capture_config, &play_config, period_size,
                  period_count, play_cap_time, c_intf_name, p_intf_name, p_device_kv,
                  c_device_kv, stream_kv, do_loopback);

    return 0;
}

void play_loopback(unsigned int card, unsigned int p_device, unsigned int c_device, unsigned int channels,
                  unsigned int rate, enum pcm_format format, struct device_config *cap_config,
                  struct device_config *p_config, unsigned int period_size,
                  unsigned int period_count, unsigned int play_cap_time, char* capture_intf,
                  char* play_intf, unsigned int pdkv, unsigned int cdkv, unsigned int stream_kv,
                  unsigned int do_loopback)
{
    struct pcm_config config;
    struct pcm *p_pcm, *c_pcm;
    struct mixer *mixer;
    char *buffer = NULL;
    char *p_intf_name = play_intf;
    char *c_intf_name = capture_intf;
    unsigned int size;
    unsigned int bytes_read = 0;
    unsigned int frames = 0, ret = 0, miid = 0;
    struct timespec end;
    struct timespec now;
    struct group_config grp_config;
    stream_kv = stream_kv ? stream_kv : PCM_RX_LOOPBACK;

    if (!cap_config || !p_config || !capture_intf || !play_intf) {
        printf("%s: %d: Invalid arguments.\n", __func__, __LINE__);
        return;
    }

    memset(&config, 0, sizeof(config));
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = period_count;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    mixer = mixer_open(card);
    if (!mixer) {
        printf("Failed to open mixer\n");
        return;
    }

    /* set device/audio_intf media config mixer control */
    if (set_agm_device_media_config(mixer, p_intf_name, p_config)) {
        printf("Failed to set playback device media config\n");
        goto err_close_mixer;
    }

    if (set_agm_device_media_config(mixer, c_intf_name, cap_config)) {
        printf("Failed to set capture device media config\n");
        goto err_close_mixer;
    }

    /* set audio interface metadata mixer control */
    if (set_agm_audio_intf_metadata(mixer, c_intf_name, cdkv, CAPTURE, rate, cap_config->bits, stream_kv)) {
        printf("Failed to set capture device metadata\n");
        goto err_close_mixer;
    }

    if (set_agm_audio_intf_metadata(mixer, p_intf_name, pdkv, PLAYBACK, rate, p_config->bits, stream_kv)) {
        printf("Failed to set playback device metadata\n");
        goto err_close_mixer;
    }

    if (set_agm_stream_metadata(mixer, p_device, stream_kv, LOOPBACK, STREAM_PCM,
                                0)) {
        printf("Failed to capture stream metadata\n");
        goto err_close_mixer;
    }

    /* Note:  No common metadata as of now*/

    /* connect pcm stream to audio intf */
    if (connect_agm_audio_intf_to_stream(mixer, p_device, p_intf_name, STREAM_PCM, true)) {
        printf("Failed to connect playback pcm to audio interface\n");
        goto err_close_mixer;
    }

    if (connect_agm_audio_intf_to_stream(mixer, c_device, c_intf_name, STREAM_PCM, true)) {
        printf("Failed to connect capture pcm to audio interface\n");
        connect_agm_audio_intf_to_stream(mixer, p_device, p_intf_name, STREAM_PCM, false);
        goto err_close_mixer;
    }

    if (connect_play_pcm_to_cap_pcm(mixer, p_device, c_device)) {
        printf("Failed to connect capture pcm to audio interface\n");
        goto err_disconnect;
    }

    if (strstr(p_intf_name, "VIRT-")) {
        if (get_group_device_info(BACKEND_CONF_FILE, p_intf_name, &grp_config))
            goto err_disconnect;

        if (set_agm_group_device_config(mixer, p_intf_name, &grp_config)) {
            printf("Failed to set grp device config\n");
            goto err_disconnect;
        }
    }
    ret = agm_mixer_get_miid (mixer, p_device, p_intf_name, STREAM_PCM, PER_STREAM_PER_DEVICE_MFC, &miid);
    if (ret) {
        printf("MFC not present for this graph\n");
    } else {
        if (configure_mfc(mixer, p_device, p_intf_name, PER_STREAM_PER_DEVICE_MFC,
                           STREAM_PCM, p_config->rate, p_config->ch,
                           p_config->bits, miid)) {
            printf("Failed to configure pspd mfc\n");
            goto err_disconnect;
        }
    }

    p_pcm = pcm_open(card, p_device, PCM_OUT, &config);
    if (!p_pcm || !pcm_is_ready(p_pcm)) {
        printf("Unable to open playback PCM device (%s)\n",
                pcm_get_error(p_pcm));
        goto err_disconnect;
    }
    if (strstr(p_intf_name, "VIRT-") || (pdkv == SPEAKER) || (pdkv == HANDSET)) {
        if (set_agm_group_mux_config(mixer, p_device, &grp_config, p_intf_name, p_config->ch)) {
            printf("Failed to set mux config\n");
            goto err_close_p_pcm;
        }
    }

    if (pcm_start(p_pcm) < 0) {
        printf("start error");
        goto err_close_p_pcm;
    }

    c_pcm = pcm_open(card, c_device, PCM_IN, &config);
    if (!c_pcm || !pcm_is_ready(c_pcm)) {
        printf("Unable to open playback PCM device (%s)\n",
                pcm_get_error(c_pcm));
        goto err_close_p_pcm;
    }

    if (pcm_start(c_pcm) < 0) {
        printf("start error");
        goto err_close_c_pcm;
    }

    if (do_loopback) {
        size = pcm_frames_to_bytes(c_pcm, pcm_get_buffer_size(c_pcm));
        buffer = malloc(size);
        if (!buffer) {
            printf("Unable to allocate %d bytes\n", size);
            goto err_close_c_pcm;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    end.tv_sec += play_cap_time;
    while (1) {
        if (close_f)
            break;
        if (do_loopback) {
            if (pcm_read(c_pcm, buffer, size)) {
                printf("Unable to read from PCM capture device %u (%s)\n",
                        c_device, pcm_get_error(c_pcm));
                break;
            }
            if (pcm_write(p_pcm, buffer, size)) {
                printf("Unable to write to PCM playback device %u (%s)\n",
                        p_device, pcm_get_error(p_pcm));
                break;
            }
        } else {
            usleep(100000);
        }

        if (play_cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
                break;
        }
    }
    connect_play_pcm_to_cap_pcm(mixer, -1, c_device);
err_close_c_pcm:
    pcm_close(c_pcm);
err_close_p_pcm:
    if (buffer)
        free(buffer);
    pcm_close(p_pcm);
err_disconnect:
    connect_agm_audio_intf_to_stream(mixer, p_device, p_intf_name, STREAM_PCM, false);
    connect_agm_audio_intf_to_stream(mixer, c_device, c_intf_name, STREAM_PCM, false);
err_close_mixer:
    mixer_close(mixer);
}
