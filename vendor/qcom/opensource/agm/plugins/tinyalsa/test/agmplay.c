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
** Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
** Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <errno.h>
#include <tinyalsa/asoundlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <agm/agm_api.h>
#include "agmmixer.h"

#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct chunk_header {
    uint32_t id;
    uint32_t sz;
};

struct chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

static int close = 0;

void play_sample(FILE *file, unsigned int card, unsigned int device, unsigned int usb_device,
                 unsigned int channels, unsigned int rate, unsigned int bits,
                 unsigned int *device_kv, unsigned int stream_kv, unsigned int instance_kv,
                 unsigned int *devicepp_kv, struct chunk_fmt fmt, bool haptics, char **intf_name,
                 int intf_num, bool is_24_LE);

void stream_close(int sig)
{
    /* allow the stream to be closed gracefully */
    signal(sig, SIG_IGN);
    close = 1;
}

static void usage(void)
{
    printf(" Usage: %s file.wav [-help print usage] [-D card] [-d device]\n"
           " [-c channels] [-r rate] [-b bits]\n"
           " [-num_intf num of interfaces followed by interface name]\n"
           " [-i intf_name] : Can be multiple if num_intf is more than 1\n"
           " [-dkv device_kv] : Can be multiple if num_intf is more than 1\n"
           " [-dppkv deviceppkv] : Assign 0 if no device pp in the graph\n"
           " [-ikv instance_kv] :  Assign 0 if no instance kv in the graph\n"
           " [-skv stream_kv] [-h haptics usecase]\n"
           " [is_24_LE] : [0-1] Only to be used if user wants to play S24_LE clip\n"
           " [-usb_d usb device]\n"
           " 0: If clip bps is 32, and format is S32_LE\n"
           " 1: If clip bps is 24, and format is S24_LE\n");
}

int main(int argc, char **argv)
{
    FILE *file;
    struct riff_wave_header riff_wave_header;
    struct chunk_header chunk_header;
    struct chunk_fmt chunk_fmt;
    unsigned int card = 100, device = 100, i=0;
    unsigned int usb_device = 1;
    unsigned int channels = 2;
    unsigned int rate = 48000;
    unsigned int bits = 16;
    int intf_num = 1;
    uint32_t dkv = SPEAKER;
    uint32_t dppkv = DEVICEPP_RX_AUDIO_MBDRC;
    unsigned int stream_kv = 0;
    unsigned int instance_kv = INSTANCE_1;
    bool haptics = false;
    char **intf_name = NULL;
    char *filename;
    int more_chunks = 1, ret = 0;
    bool is_24_LE = false;
    unsigned int *devicepp_kv = (unsigned int *) malloc(intf_num * sizeof(unsigned int));
    unsigned int *device_kv = (unsigned int *) malloc(intf_num * sizeof(unsigned int));

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

    filename = argv[1];
    file = fopen(filename, "rb");
    if (!file) {
        printf("Unable to open file '%s'\n", filename);
        return 1;
    }

    fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
    if ((riff_wave_header.riff_id != ID_RIFF) ||
        (riff_wave_header.wave_id != ID_WAVE)) {
        printf("Error: '%s' is not a riff/wave file\n", filename);
        fclose(file);
        return 1;
    }

    do {
        fread(&chunk_header, sizeof(chunk_header), 1, file);

        switch (chunk_header.id) {
        case ID_FMT:
            fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
            /* If the format header is larger, skip the rest */
            if (chunk_header.sz > sizeof(chunk_fmt))
                fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
            break;
        case ID_DATA:
            /* Stop looking for chunks */
            more_chunks = 0;
            break;
        default:
            /* Unknown chunk, skip bytes */
            fseek(file, chunk_header.sz, SEEK_CUR);
        }
    } while (more_chunks);

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
        } else if (strcmp(*argv, "-h") == 0) {
            argv++;
            if (*argv)
                haptics = *argv;
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
                if (*argv) {
                    devicepp_kv[i] = convert_char_to_hex(*argv);
                }
            }
        } else if (strcmp(*argv, "-is_24_LE") == 0) {
            argv++;
            if (*argv) {
                is_24_LE = atoi(*argv);
            }
        } else if (strcmp(*argv, "-usb_d") == 0) {
            argv++;
            if (*argv)
                usb_device = atoi(*argv);
        } else if (strcmp(*argv, "-help") == 0) {
            usage();
        }
        if (*argv)
            argv++;
    }

    if (intf_name == NULL)
        return 1;

    play_sample(file, card, device, usb_device, channels, rate, bits, device_kv, stream_kv,
                 instance_kv, devicepp_kv, chunk_fmt, haptics, intf_name, intf_num, is_24_LE);

    fclose(file);
    if (device_kv)
        free(device_kv);
    if (devicepp_kv)
        free(devicepp_kv);
    if (intf_name)
        free(intf_name);

    return 0;
}

void play_sample(FILE *file, unsigned int card, unsigned int device, unsigned int usb_device,
                 unsigned int channels, unsigned int rate, unsigned int bits,
                 unsigned int *device_kv, unsigned int stream_kv, unsigned int instance_kv,
                 unsigned int *devicepp_kv, struct chunk_fmt fmt, bool haptics, char **intf_name,
                 int intf_num, bool is_24_LE)
{
    struct pcm_config config;
    struct pcm *pcm;
    struct mixer *mixer;
    char *buffer;
    int playback_path, playback_value;
    int size, index, ret = 0;
    int num_read;
    struct group_config *grp_config = NULL;
    struct device_config *dev_config = NULL;
    uint32_t miid = 0;
    struct usbAudioConfig cfg;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;

    grp_config = (struct group_config *) malloc(intf_num * sizeof(struct group_config));
    if (!grp_config) {
        printf("Failed to allocate memory for group config");
        return;
    }

    dev_config = (struct device_config *) malloc(intf_num * sizeof(struct device_config));
    if (!dev_config) {
        printf("Failed to allocate memory for dev config");
        return;
    }

    memset(&config, 0, sizeof(config));
    config.channels = fmt.num_channels;
    config.rate = fmt.sample_rate;
    config.period_size = 1024;
    config.period_count = 4;
    if (fmt.bits_per_sample == 32) {
        if (is_24_LE)
            config.format = PCM_FORMAT_S24_LE;
        else
            config.format = PCM_FORMAT_S32_LE;
    } else if (fmt.bits_per_sample == 24)
        config.format = PCM_FORMAT_S24_3LE;
    else if (fmt.bits_per_sample == 16)
        config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    mixer = mixer_open(card);
    if (!mixer) {
        printf("Failed to open mixer\n");
        return;
    }

    if (haptics) {
        playback_path = HAPTICS;
        stream_kv = stream_kv ? stream_kv : HAPTICS_PLAYBACK;
    } else {
        playback_path = PLAYBACK;
        stream_kv = stream_kv ? stream_kv : PCM_LL_PLAYBACK;
    }

    for (index = 0; index < intf_num; index++) {
        if(intf_name[index] != NULL && strcmp(intf_name[index], "USB_AUDIO-RX") == 0) {
            dev_config[index].rate = rate;
            dev_config[index].ch = channels;
            dev_config[index].bits = bits;
            dev_config[index].format = PCM_FORMAT_INVALID;
        } else {
            ret = get_device_media_config(BACKEND_CONF_FILE, intf_name[index], &dev_config[index]);
            if (ret) {
                printf("Invalid input, entry not found for : %s\n", intf_name[index]);
                fclose(file);
            }
            if (dev_config[index].format != PCM_FORMAT_INVALID) {
                printf("Valid format from backend_conf %d\n", dev_config[index].format);
                /* Updating bitwitdh based on format to avoid mismatch between bitwidth
                 * and format, as device bw will be used to configure MFC.
                 */
                dev_config[index].bits = get_pcm_bit_width(dev_config[index].format);
            }
        }
        printf("Backend %s rate ch bit fmt : %d, %d, %d %d\n", intf_name[index],
            dev_config[index].rate, dev_config[index].ch, dev_config[index].bits,
            dev_config[index].format);

        /* set device/audio_intf media config mixer control */
        if (set_agm_device_media_config(mixer, intf_name[index], &dev_config[index])) {
            printf("Failed to set device media config\n");
            goto err_close_mixer;
        }

        /* set audio interface metadata mixer control */
        if (set_agm_audio_intf_metadata(mixer, intf_name[index], device_kv[index], playback_path,
                                    dev_config[index].rate, dev_config[index].bits, stream_kv)) {
            printf("Failed to set device metadata\n");
            goto err_close_mixer;
        }
    }

    /* set audio interface metadata mixer control */
    if (set_agm_stream_metadata(mixer, device, stream_kv, PLAYBACK, STREAM_PCM,
                                instance_kv)) {
        printf("Failed to set pcm metadata\n");
        goto err_close_mixer;
    }

     /* Note:  No common metadata as of now*/
    for (index = 0; index < intf_num; index++) {
         if (devicepp_kv[index] != 0) {
             if (set_agm_streamdevice_metadata(mixer, device, stream_kv, PLAYBACK, STREAM_PCM, intf_name[index],
                                               devicepp_kv[index])) {
                 printf("Failed to set pcm metadata\n");
                 goto err_close_mixer;
             }
         }

        if (intf_name[index] != NULL && strcmp(intf_name[index], "USB_AUDIO-RX") == 0) {
            ret = agm_mixer_get_miid (mixer, device, intf_name[index], STREAM_PCM, DEVICE_HW_ENDPOINT_RX, &miid);
            if (ret == 0) {
                cfg.usb_token = usb_device << 16;
                cfg.svc_interval = 0;
                get_agm_usb_audio_config_payload(&payload, &payloadSize, miid, &cfg);

                if (payloadSize) {
                    ret = set_agm_device_custom_payload(mixer, intf_name[index], payload, payloadSize);
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
        if (connect_agm_audio_intf_to_stream(mixer, device, intf_name[index], STREAM_PCM, true)) {
            printf("Failed to connect pcm to audio interface\n");
            goto err_close_mixer;
        }

        ret = agm_mixer_get_miid (mixer, device, intf_name[index], STREAM_PCM, PER_STREAM_PER_DEVICE_MFC, &miid);
        if (ret) {
            printf("MFC not present for this graph\n");
        } else {
            if (configure_mfc(mixer, device, intf_name[index], PER_STREAM_PER_DEVICE_MFC,
                           STREAM_PCM, dev_config[index].rate, dev_config[index].ch,
                           dev_config[index].bits, miid)) {
                printf("Failed to configure pspd mfc\n");
                goto err_close_mixer;
            }
        }

        if (strstr(intf_name[index], "VIRT-")) {
            if (get_group_device_info(BACKEND_CONF_FILE, intf_name[index], &grp_config[index]))
                goto err_close_mixer;

            if (set_agm_group_device_config(mixer, intf_name[index], &grp_config[index])) {
                printf("Failed to set grp device config\n");
                goto err_close_mixer;
            }
        }
    }

    pcm = pcm_open(card, device, PCM_OUT, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        printf("Unable to open PCM device %u (%s)\n",
                device, pcm_get_error(pcm));
        goto err_close_mixer;
    }

    for (index = 0; index < intf_num; index++) {
        if (strstr(intf_name[index], "VIRT-") || (device_kv[index] == SPEAKER) || (device_kv[index] == HANDSET)) {
            if (set_agm_group_mux_config(mixer, device, &grp_config[index], intf_name[index], dev_config[index].ch)) {
                printf("Failed to set grp device config\n");
            }
        }
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        printf("Unable to allocate %d bytes\n", size);
        goto err_close_pcm;
    }

    printf("Playing sample: %u ch, %u hz, %u bit\n", fmt.num_channels,
            fmt.sample_rate, fmt.bits_per_sample);

    if (pcm_start(pcm) < 0) {
        printf("start error\n");
        goto err_close_pcm;
    }

    /* catch ctrl-c to shutdown cleanly */
    signal(SIGINT, stream_close);

    do {
        num_read = fread(buffer, 1, size, file);
        if (num_read > 0) {
            if (pcm_write(pcm, buffer, num_read)) {
                printf("Error playing sample\n");
                break;
            }
        }
    } while (!close && num_read > 0);

    pcm_stop(pcm);
    /* disconnect pcm stream to audio intf */
    for (index = 0; index < intf_num; index++) {
        connect_agm_audio_intf_to_stream(mixer, device, intf_name[index], STREAM_PCM, false);
    }

    free(buffer);
err_close_pcm:
    pcm_close(pcm);
err_close_mixer:
    if (payload) {
      free(payload);
    }
    if (dev_config)
        free(dev_config);
    if (grp_config)
        free(grp_config);
    mixer_close(mixer);
}
