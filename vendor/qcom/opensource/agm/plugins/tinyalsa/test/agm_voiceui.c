/*
** Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
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

#include <errno.h>
#include <tinyalsa/asoundlib.h>
#include <sound/asound.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

#include <agm/agm_api.h>
#include "agmmixer.h"

static pthread_t event_thread;
static int done = 0;

char *audio_interface_name[] = {
    "CODEC_DMA-LPAIF_VA-TX-0",
    "CODEC_DMA-LPAIF_VA-TX-1",
    "CODEC_DMA-LPAIF_RXTX-TX-3",
    "MI2S-LPAIF_AXI-TX-PRIMARY",
    "TDM-LPAIF_AXI-TX-PRIMARY",
    "AUXPCM-LPAIF_AXI-TX-PRIMARY",
};

char *ec_aif_name[] = {
    "CODEC_DMA-LPAIF_WSA-RX-0",
    "CODEC_DMA-LPAIF_WSA-RX-1",
    "MI2S-LPAIF_AXI-RX-PRIMARY",
    "TDM-LPAIF_AXI-RX-PRIMARY",
    "AUXPCM-LPAIF_AXI-RX-PRIMARY",
};

static void read_event_data(struct mixer *mixer, char *mixer_str)
{
    struct mixer_ctl *ctl;
    char *buf = NULL;
    unsigned int num_values;
    int i, ret;
    struct agm_event_cb_params *params;

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    num_values = mixer_ctl_get_num_values(ctl);
    printf("%s - %d\n", __func__, num_values);
    buf = calloc(1, num_values);
    if (!buf) {
        printf("Failed to allocate memory for buffer\n");
        return;
    }

    ret = mixer_ctl_get_array(ctl, buf, num_values);
    if (ret < 0) {
        printf("Failed to mixer_ctl_get_array\n");
        free(buf);
        return;
    }

    params = (struct agm_event_cb_params *)buf;
    printf("%s params.source_module_id %x\n", __func__, params->source_module_id);
    printf("%s params.event_id %d\n", __func__, params->event_id);
    printf("%s params.event_payload_size %x\n", __func__, params->event_payload_size);
}

static void event_wait_thread_loop(void *context)
{
    struct mixer *mixer = (struct mixer *)context;
    int ret = 0;
    struct ctl_event mixer_event = {0};

    printf("subscribing for event\n");
    mixer_subscribe_events(mixer, 1);

    printf("going to wait for event\n");
    ret = mixer_wait_event(mixer, 30000);
    if (ret < 0) {
        printf("%s: mixer_wait_event err!, ret = %d\n", __func__, ret);
    } else if (ret > 0) {
        ret = mixer_read_event(mixer, &mixer_event);
        if (ret >= 0) {
            printf("Event Received %s\n",  mixer_event.data.elem.id.name);
            read_event_data(mixer, mixer_event.data.elem.id.name);
        } else {
            printf("%s: mixer_read failed, ret = %d\n", __func__, ret);
        }
        done = 1;
    }

    mixer_subscribe_events(mixer, 0);
}

static void record_lab_buffer(struct pcm *pcm, unsigned int cap_time)
{
    struct timespec end;
    struct timespec now;
    unsigned int size;
    char *buffer;
    FILE *file;
    int ret;

    ret = pthread_join(event_thread, (void **) NULL);
    if (ret < 0) {
        printf("%s: Unable to join event thread\n", __func__);
        return;
    }

    if (!done)
        return;

    file = fopen("/data/voice_rec.wav", "wb");
    if (!file) {
        printf("Unable to create voice_rec file\n");
        return;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = malloc(size);
    if (!buffer) {
        printf("Unable to allocate %u bytes\n", size);
        fclose(file);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    end.tv_sec = now.tv_sec + cap_time;
    end.tv_nsec = now.tv_nsec;

    while (!pcm_read(pcm, buffer, size)) {
        if (fwrite(buffer, 1, size, file) != size) {
            printf("Error capturing sample\n");
            break;
        }
        if (cap_time) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (now.tv_sec > end.tv_sec ||
                (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
                break;
        }
    }

    free(buffer);
    fclose(file);
}

static int get_file_size(char *filename)
{
    FILE *fp;
    int size = 0;

    fp = fopen(filename, "rb");
    if (!fp) {
        printf("%s: Unable to open file '%s'\n", __func__, filename);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    printf("%s - sizeof %s is %d\n", __func__, filename, size);
    fseek(fp, 0, SEEK_SET);
    fclose(fp);
    return size;
}

static void fill_payload(char *filename, int size, void *payload)
{
    FILE *fp;
    int bytes_read;

    fp = fopen(filename, "rb");
    if (!fp) {
        printf("%s: Unable to open file '%s'\n", __func__, filename);
        return;
    }

    bytes_read = fread((char*)payload, 1, size , fp);
    if (bytes_read != size) {
        printf("%s failed to read data from file %s, bytes read = %d\n", __func__, filename, bytes_read);
    }
    fclose(fp);
}

static void* merge_payload(uint32_t miid, int num, int *sum,  ...)
{
    va_list valist;
    int i = 0, total_size = 0, offset = 0;
    int *size = calloc(num, sizeof(int));
    char **temp = calloc(num, sizeof(char *));
    void *payload = NULL;
    uint8_t *buf;
    uint32_t *module_instance_id = NULL;

    if (!size || !temp) {
        printf("Failed to allocate memory for size and temp\n");
        return NULL;
    }

    va_start(valist, num);
    for (i = 0; i < num; i++) {
        temp[i] = va_arg(valist, char *);
        size[i] = get_file_size(temp[i]);
        total_size += size[i];
    }
    va_end(valist);

    payload = calloc(1, total_size);
    if (!payload)
        return NULL;

    buf = payload;
    for (i = 0; i < num; i++) {
        fill_payload(temp[i], size[i], buf);
        // Update SVA miid
        module_instance_id = (uint32_t *)buf;
        *module_instance_id = miid;
        buf += size[i];
    }
    *sum = total_size;
    /* TODO : free memory */
    return payload;
}

void voice_ui_test(unsigned int card, unsigned int device, unsigned int audio_intf, unsigned int cap_time, int ec_aif,
                    unsigned int device_kv, unsigned int stream_kv, unsigned int instance_kv, unsigned int devicepp_kv)
{
    struct mixer *mixer;
    char *intf_name = audio_interface_name[audio_intf];
    char *ec_intf_name = NULL;
    struct pcm_config config;
    struct pcm *pcm;
    struct device_config dev_config;
    int ret = 0;
    enum pcm_format format = PCM_FORMAT_S16_LE;
    uint32_t miid = 0, param_size = 0;
    void *param_buf = NULL;

    memset(&config, 0, sizeof(config));
    config.channels = 2;
    config.rate = 48000;
    config.period_size = 1024;
    config.period_count = 4;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
    stream_kv = stream_kv ? stream_kv : VOICE_UI;

    dev_config.rate = config.rate;
    dev_config.ch = config.channels;
    dev_config.bits = get_pcm_bit_width(config.format);
    dev_config.format = config.format;
    mixer = mixer_open(card);
    if (!mixer) {
        printf("Failed to open mixer\n");
        return;
    }

    /* set device/audio_intf media config mixer control */
    if (set_agm_device_media_config(mixer, intf_name, &dev_config)) {
        printf("Failed to set device media config\n");
        goto err_close_mixer;
    }

    /* set audio interface metadata mixer control */
    if (set_agm_audio_intf_metadata(mixer, intf_name, 0, CAPTURE, config.rate,
                                    pcm_format_to_bits(format), stream_kv)) {
        printf("Failed to set device metadata\n");
        goto err_close_mixer;
    }

    /* set stream metadata mixer control */
    if (set_agm_stream_metadata(mixer, device, stream_kv, CAPTURE, STREAM_PCM,
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
    /* connect pcm stream to audio intf */
    if (connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_PCM, true)) {
        printf("Failed to connect pcm to audio interface\n");
        goto err_close_mixer;
    }

    ret = agm_mixer_get_miid(mixer, device, intf_name, STREAM_PCM, DEVICE_SVA, &miid);
    if (ret) {
        printf("%s Get MIID from tag data failed\n", __func__);
        goto err_close_mixer;
    }

    param_buf = merge_payload(miid, 3, &param_size, "/vendor/etc/sound_model",
        "/vendor/etc/wakeup_config", "/vendor/etc/buffer_config");

    if (agm_mixer_set_param(mixer, device, STREAM_PCM, param_buf, param_size)) {
        printf("setparam failed\n");
        goto err_close_mixer;
    }

    pcm = pcm_open(card, device, PCM_IN, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        printf("Unable to open PCM device (%s)\n",
                pcm_get_error(pcm));
        goto err_close_mixer;
    }

#if 0
    /* Load Sound Model */
    if (agm_mixer_set_param_with_file(mixer, device, STREAM_PCM, "/etc/sound_model")) {
        printf("soundmodel load failed\n");
        goto err_close_pcm;
    }

    /* Register voice wakeup config */
    if (agm_mixer_set_param_with_file(mixer, device, STREAM_PCM, "/etc/wakeup_config")) {
        printf("wakeup config registration failed\n");
        goto err_close_pcm;
    }
#endif
//   Call API to register event with GSL here
//   Same as GSL_CMD_REGISTER_CUSTOM_EVENT
    if (agm_mixer_register_event(mixer, device, STREAM_PCM, miid, 1)) {
        printf("GSL event registration failed\n");
        goto err_close_pcm;
    }

#if 0
    /* Register buffer config */
    if (agm_mixer_set_param_with_file(mixer, device, STREAM_PCM, "/etc/buffer_config")) {
        printf("buffer config registration failed\n");
        goto err_close_pcm;
    }
#endif
    /* Setup EC ref path */
    if (ec_aif != -1) {
        if (agm_mixer_set_ecref_path(mixer, device, STREAM_PCM, ec_aif_name[ec_aif])) {
            printf("EC ref path failed\n");
            goto err_close_pcm;
        }
    }

    if (pcm_start(pcm) < 0) {  // This internally calls pcm_prepare too.
        printf("start error\n");
        goto err_close_pcm;
    }

    pthread_create(&event_thread,
                    (const pthread_attr_t *) NULL, event_wait_thread_loop, mixer);

    record_lab_buffer(pcm, cap_time);

    /* Reset Engine */
    if (agm_mixer_set_param_with_file(mixer, device, STREAM_PCM, "/vendor/etc/engine_reset")) {
        printf("stream setup duration configuration failed\n");
        goto err_close_pcm;
    }

err_close_pcm:
    if (ec_aif != -1)
        agm_mixer_set_ecref_path(mixer, device, STREAM_PCM, "ZERO");
    pcm_close(pcm);
err_close_mixer:
    mixer_close(mixer);
    printf("completed event test\n");
}

int main(int argc, char **argv)
{
    unsigned int device = 100;
    unsigned int card = 0;
    unsigned int audio_intf = 0;
    int ec_aif = -1;
    unsigned int cap_time = 5;
    unsigned int device_kv = 0;
    unsigned int devicepp_kv = DEVICEPP_TX_FLUENCE_FFECNS;
    unsigned int stream_kv = 0;
    unsigned int instance_kv = INSTANCE_1;

    argv += 1;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            argv++;
            if (*argv)
                device = atoi(*argv);
        }
        if (*argv && strcmp(*argv, "-D") == 0) {
            argv++;
            if (*argv)
                card = atoi(*argv);
        }
        if (*argv && strcmp(*argv, "-i") == 0) {
            argv++;
            if (*argv)
                audio_intf = atoi(*argv);
            if (audio_intf >= sizeof(audio_interface_name)/sizeof(char *)) {
                printf("Invalid audio interface index denoted by -i\n");
                return 1;
            }
        }
        if (*argv && strcmp(*argv, "-e") == 0) {
            argv++;
            if (*argv)
                ec_aif = atoi(*argv);
            if (ec_aif >= sizeof(ec_aif_name)/sizeof(char *)) {
                printf("Invalid echoref audio interface index denoted by -i\n");
                return 1;
            }
        }
        if (*argv && strcmp(*argv, "-T") == 0) {
            argv++;
            if (*argv)
                cap_time = atoi(*argv);
        } else if (*argv && strcmp(*argv, "-dkv") == 0) {
            argv++;
            if (*argv)
                device_kv = convert_char_to_hex(*argv);
        } else if (*argv && strcmp(*argv, "-skv") == 0) {
            argv++;
            if (*argv)
                stream_kv = convert_char_to_hex(*argv);
        } else if (*argv && strcmp(*argv, "-ikv") == 0) {
            argv++;
            if (*argv)
                instance_kv = atoi(*argv);
        } else if (*argv && strcmp(*argv, "-dppkv") == 0) {
            argv++;
            if (*argv)
                devicepp_kv = convert_char_to_hex(*argv);
        }

        if (*argv)
            argv++;
    }

    voice_ui_test(card, device, audio_intf, cap_time, ec_aif, device_kv, stream_kv,
                  instance_kv, devicepp_kv);
    return 0;
}
