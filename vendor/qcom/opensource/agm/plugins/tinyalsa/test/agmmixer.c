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
** Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <expat.h>
#include <tinyalsa/asoundlib.h>
#include <sound/asound.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <agm/agm_api.h>
#include "agmmixer.h"

#define EVENT_ID_DETECTION_ENGINE_GENERIC_INFO 0x0800104F

#define PARAM_ID_DETECTION_ENGINE_GENERIC_EVENT_CFG 0x0800104E
#define PARAM_ID_MFC_OUTPUT_MEDIA_FORMAT            0x08001024

#define PARAM_ID_USB_AUDIO_INTF_CFG 0x080010D6

enum {
    DEVICE = 1,
    GROUP,
};

enum pcm_channel_map
{
   PCM_CHANNEL_L = 1,
   PCM_CHANNEL_R = 2,
   PCM_CHANNEL_C = 3,
   PCM_CHANNEL_LS = 4,
   PCM_CHANNEL_RS = 5,
   PCM_CHANNEL_LFE = 6,
   PCM_CHANNEL_CS = 7,
   PCM_CHANNEL_CB = PCM_CHANNEL_CS,
   PCM_CHANNEL_LB = 8,
   PCM_CHANNEL_RB = 9,
};
/* Payload of the PARAM_ID_MFC_OUTPUT_MEDIA_FORMAT parameter in the
 Media Format Converter Module. Following this will be the variable payload for channel_map. */
struct param_id_mfc_output_media_fmt_t
{
   int32_t sampling_rate;
   int16_t bit_width;
   int16_t num_channels;
   uint16_t channel_type[0];
}__attribute__((packed));

struct apm_module_param_data_t
{
   uint32_t module_instance_id;
   uint32_t param_id;
   uint32_t param_size;
   uint32_t error_code;
};

struct param_id_usb_audio_intf_cfg_t
{
   uint32_t usb_token;
   /**< @h2xmle_description {Valid token for the USB audio device} */
   uint32_t svc_interval;
   /**< @h2xmle_description {USB device service interval in microseconds. Though the acceptable
         values are between 125- 3276800, the actual supported values are
         determined by the equation specified in USB 3.0 documentation.}
         @h2xmle_range      {125..3276800}
         @h2xmle_default {125}
   */
};

struct detection_engine_generic_event_cfg {
    uint32_t event_mode;
};


struct gsl_module_id_info_entry {
    uint32_t module_id; /**< module id */
    uint32_t module_iid; /**< globally unique module instance id */
};

/**
 * Structure mapping the tag_id to module info (mid and miid)
 */
struct gsl_tag_module_info_entry {
    uint32_t tag_id; /**< tag id of the module */
    uint32_t num_modules; /**< number of modules matching the tag_id */
    struct gsl_module_id_info_entry module_entry[0]; /**< module list */
};

struct gsl_tag_module_info {
    uint32_t num_tags; /**< number of tags */
    struct gsl_tag_module_info_entry tag_module_entry[0];
    /**< variable payload of type struct gsl_tag_module_info_entry */
};

unsigned int slot_mask_map[5] = { 0, SLOT_MASK1, SLOT_MASK3, SLOT_MASK7, SLOT_MASK15};

#define PADDING_8BYTE_ALIGN(x)  ((((x) + 7) & 7) ^ 7)

static unsigned int bits_to_sndrv_format(unsigned int bits)
{
    switch (bits) {
    case 32:
        return SNDRV_PCM_FORMAT_S32_LE;
    case 8:
        return SNDRV_PCM_FORMAT_S8;
    case 24:
        return SNDRV_PCM_FORMAT_S24_3LE;
    default:
    case 16:
        return SNDRV_PCM_FORMAT_S16_LE;
    };
}

static unsigned int alsa_to_sndrv_format(enum pcm_format fmt)
{
    switch (fmt) {
    case PCM_FORMAT_S32_LE:
        return SNDRV_PCM_FORMAT_S32_LE;
    case PCM_FORMAT_S8:
        return SNDRV_PCM_FORMAT_S8;
    case PCM_FORMAT_S24_3LE:
        return SNDRV_PCM_FORMAT_S24_3LE;
    case PCM_FORMAT_S24_LE:
        return SNDRV_PCM_FORMAT_S24_LE;
    default:
    case PCM_FORMAT_S16_LE:
        return SNDRV_PCM_FORMAT_S16_LE;
    };
}

static enum pcm_format get_pcm_format(char *str)
{
    if (!strncmp(str, "PCM_FORMAT_S16_LE", strlen("PCM_FORMAT_S16_LE"))) {
        return PCM_FORMAT_S16_LE;
    } else if (!strncmp(str, "PCM_FORMAT_S32_LE", strlen("PCM_FORMAT_S32_LE"))) {
        return PCM_FORMAT_S32_LE;
    } else if (!strncmp(str, "PCM_FORMAT_S8", strlen("PCM_FORMAT_S8"))) {
        return PCM_FORMAT_S8;
    } else if (!strncmp(str, "PCM_FORMAT_S24_LE", strlen("PCM_FORMAT_S24_LE"))) {
        return PCM_FORMAT_S24_LE;
    } else if (!strncmp(str, "PCM_FORMAT_S24_3LE", strlen("PCM_FORMAT_S24_3LE"))) {
        return PCM_FORMAT_S24_3LE;
    } else {
        return PCM_FORMAT_INVALID;
    }
}

int get_pcm_bit_width(enum pcm_format fmt_id)
{
    int bit_width = 16;

    switch (fmt_id) {
    case PCM_FORMAT_S24_3LE:
    /*
     *This api returns the number of audio data bit width specific to the format
     *e.g. In S24_LE, even if the number of bytes is 4, the audio data is only in 3 bytes
     *Hence we return 24 as the bit_width, whereas the bitspersample for this format would
     *return 32
     */
    case PCM_FORMAT_S24_LE:
        bit_width = 24;
        break;
    case PCM_FORMAT_S32_LE:
        bit_width = 32;
        break;
    case PCM_FORMAT_S8:
        bit_width = 8;
    case PCM_FORMAT_S16_LE:
    default:
        break;
    }

    return bit_width;
}

void start_tag(void *userdata, const XML_Char *tag_name, const XML_Char **attr)
{
    struct device_config *config = (struct device_config *)userdata;
    enum pcm_format fmt;

    if (strncmp(tag_name, "device", strlen("device")) != 0)
        return;

    if (strcmp(attr[0], "name") != 0) {
        printf("name not found\n");
        return;
    }

    if (strcmp(attr[2], "rate") != 0) {
        printf("rate not found\n");
        return;
    }

    if (strcmp(attr[4], "ch") != 0) {
        printf("channels not found\n");
        return;
    }

    if (strcmp(attr[6], "bits") != 0) {
        printf("bits not found\n");
        return;
    }

    if (strncmp(config->name, attr[1], sizeof(config->name)))
        return;

    if (attr[8]) {
        if (strcmp(attr[8], "format") == 0) {
            printf("PCM format found\n");
            fmt = get_pcm_format(attr[9]);
            if (fmt != PCM_FORMAT_INVALID && fmt < PCM_FORMAT_MAX)
                config->format = fmt;
        }
    } else {
           config->format = PCM_FORMAT_INVALID;
    }

    config->rate = atoi(attr[3]);
    config->ch = atoi(attr[5]);
    config->bits = atoi(attr[7]);
}

void start_group_tag(void *userdata, const XML_Char *tag_name, const XML_Char **attr)
{
    struct group_config *config = (struct group_config *)userdata;
    enum pcm_format fmt;

    if (strncmp(tag_name, "group_device", strlen("group_device")) != 0)
        return;

    if (strcmp(attr[0], "name") != 0) {
        printf("name not found\n");
        return;
    }

    if (strcmp(attr[2], "rate") != 0) {
        printf("rate not found\n");
        return;
    }

    if (strcmp(attr[4], "ch") != 0) {
        printf("channels not found\n");
        return;
    }

    if (strcmp(attr[6], "bits") != 0) {
        printf("bits not found\n");
        return;
    }

    if (strcmp(attr[8], "slot_mask") != 0) {
        printf("slot_mask not found\n");
        return;
    }

    if (strncmp(config->name, attr[1], sizeof(config->name)))
        return;

    if (attr[10]) {
        if (strcmp(attr[10], "format") == 0) {
            printf("PCM format found\n");
            fmt = get_pcm_format(attr[11]);
            if (fmt != PCM_FORMAT_INVALID && fmt < PCM_FORMAT_MAX)
                config->format = fmt;
        }
    } else {
        config->format = PCM_FORMAT_INVALID;
    }
    config->rate = atoi(attr[3]);
    config->ch = atoi(attr[5]);
    config->bits = atoi(attr[7]);
    config->slot_mask = atoi(attr[9]);
}

int convert_char_to_hex(char *char_num)
{
    uint64_t hex_num = 0;
    uint32_t base = 1;
    int32_t len = strlen(char_num);
    for (int i = len-1; i>=2; i--) {
        if (char_num[i] >= '0' && char_num[i] <= '9') {
            hex_num += (char_num[i] - 48) * base;
            base = base << 4;
        } else if (char_num[i] >= 'A' && char_num[i] <= 'F') {
            hex_num += (char_num[i] - 55) * base;
            base = base << 4;
        } else if (char_num[i] >= 'a' && char_num[i] <= 'f') {
            hex_num += (char_num[i] - 87) * base;
            base = base << 4;
        }
    }
    return (int32_t) hex_num;
}

static int get_backend_info(char* filename, char *intf_name, void *config, int type)
{
    FILE *file = NULL;
    XML_Parser parser;
    int ret = 0;
    int bytes_read;
    void *buf = NULL;
    struct device_config *dev_cfg;
    struct group_config *grp_cfg;

    file = fopen(filename, "r");
    if (!file) {
        ret = -EINVAL;
        printf("Failed to open xml file name %s ret %d", filename, ret);
        goto done;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ret = -EINVAL;
        printf("Failed to create XML ret %d", ret);
        goto closeFile;
    }
    if (type == DEVICE) {
        dev_cfg = (struct device_config *)config;
        memset(dev_cfg, 0, sizeof(*dev_cfg));
        strlcpy(dev_cfg->name, intf_name, sizeof(dev_cfg->name));
        XML_SetElementHandler(parser, start_tag, NULL);
    } else {
        grp_cfg = (struct group_config *)config;
        memset(grp_cfg, 0, sizeof(*grp_cfg));
        strlcpy(grp_cfg->name, intf_name, sizeof(grp_cfg->name));
        XML_SetElementHandler(parser, start_group_tag, NULL);
    }

    XML_SetUserData(parser, config);

    while (1) {
        buf = XML_GetBuffer(parser, 1024);
        if (buf == NULL) {
            ret = -EINVAL;
            printf("XML_Getbuffer failed ret %d", ret);
            goto freeParser;
        }

        bytes_read = fread(buf, 1, 1024, file);
        if (bytes_read < 0) {
            ret = -EINVAL;
            printf("fread failed ret %d", ret);
            goto freeParser;
        }

        if (XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            ret = -EINVAL;
            printf("XML ParseBuffer failed for %s file ret %d", filename, ret);
            goto freeParser;
        }
        if (bytes_read == 0 || ((struct device_config *)config)->rate != 0)
            break;
    }

    if (((struct device_config *)config)->rate == 0) {
        ret = -EINVAL;
        printf("Entry not found\n");
    }
freeParser:
    XML_ParserFree(parser);
closeFile:
    fclose(file);
done:
    return ret;
}

int get_device_media_config(char* filename, char *intf_name, struct device_config *config)
{
    return get_backend_info(filename, intf_name, (void *)config, DEVICE);
}

int get_group_device_info(char* filename, char *intf_name, struct group_config *config)
{
    char *be_name = strdup(intf_name);
    if (be_name == NULL) {
        printf("%s(): Insufficient memory to create be_name \n", __func__);
        return -ENOMEM;
    }
    int be_len = strlen(intf_name) - 7;

    be_name[be_len] = '\0';
    return get_backend_info(filename, be_name, (void *)config, GROUP);
}

int set_agm_group_device_config(struct mixer *mixer, char *intf_name, struct group_config *config)
{
    char *stream = "PCM";
    char *grp_ctl = "grp config";
    char *control = "setParamTag";
    char *mixer_str = NULL;
    struct mixer_ctl *ctl;
    long grp_config[5];
    int ctl_len = 0, be_len;
    int ret = 0;
    char *be_name = strdup(intf_name);
    if (be_name == NULL) {
        printf("%s(): Insufficient memory to create be_name \n", __func__);
        ret = -ENOMEM;
        goto done;
    }

    be_len = strlen(intf_name) - 7;
    be_name[be_len] = '\0';

    ctl_len = strlen(be_name) + 1 + strlen(grp_ctl) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        ret = -ENOMEM;
        goto done;
    }
    snprintf(mixer_str, ctl_len, "%s %s", be_name, grp_ctl);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        ret = ENOENT;
        goto done;
    }

    grp_config[0] = config->rate;
    grp_config[1] = config->ch;
    if (config->format == PCM_FORMAT_INVALID)
        grp_config[2] = bits_to_sndrv_format(config->bits);
    else
        grp_config[2] = alsa_to_sndrv_format(config->format);
    grp_config[3] = AGM_DATA_FORMAT_FIXED_POINT;
    grp_config[4] = config->slot_mask;

    ret = mixer_ctl_set_array(ctl, &grp_config, sizeof(grp_config)/sizeof(grp_config[0]));
    if (ret) {
        printf("Failed to set grp media config mixer ctl\n");
        goto done;
    }

done:
    if (be_name)
        free(be_name);

    if (mixer_str)
        free(mixer_str);
    return ret;
}

int set_agm_group_mux_config(struct mixer *mixer, unsigned int device, struct group_config *config, char *intf_name, unsigned int channels)
{
    char *stream = "PCM";
    char *control = "setParamTag";
    char *mixer_str = NULL;
    struct mixer_ctl *ctl;
    int ctl_len = 0, val_len;
    int ret = 0;
    struct agm_tag_config* tag_config = NULL;
    uint32_t miid = 0;

    ret = set_agm_stream_metadata_type(mixer, device, intf_name, STREAM_PCM);
    if (ret)
        return 0;

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str realloc failed\n");
        goto done;
    }

    val_len = sizeof(struct agm_tag_config) + sizeof(struct agm_key_value);

    tag_config = (struct agm_tag_config*)calloc(1, val_len);
    if (!tag_config)
        goto done;

    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        goto done;
    }
    tag_config->tag = TAG_DEVICE_MUX;
    tag_config->num_tkvs = 1;
    tag_config->kv[0].key = TAG_KEY_SLOT_MASK;
    ret = agm_mixer_get_miid(mixer, device, intf_name, STREAM_PCM, TAG_DEVICE_MUX, &miid);
    if (strstr(intf_name, "VIRT-"))
        tag_config->kv[0].value = config->slot_mask;
    else if (ret == 0)
        tag_config->kv[0].value = slot_mask_map[channels];
    else {
        ret = 0;
        goto done;
    }

    mixer_ctl_set_array(ctl, tag_config, val_len);
done:
    if (mixer_str)
        free(mixer_str);
    if (tag_config)
        free(tag_config);
    return ret;
}

int set_agm_device_media_config(struct mixer *mixer, char *intf_name, struct device_config *config)
{
    char *control = "rate ch fmt";
    char *mixer_str;
    struct mixer_ctl *ctl;
    long media_config[4];
    int ctl_len = 0;
    int ret = 0;

    ctl_len = strlen(intf_name) + 1 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", intf_name, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    media_config[0] = config->rate;
    media_config[1] = config->ch;
    if (config->format == PCM_FORMAT_INVALID)
        media_config[2] = bits_to_sndrv_format(config->bits);
    else
        media_config[2] = alsa_to_sndrv_format(config->format);
    media_config[3] = AGM_DATA_FORMAT_FIXED_POINT;

    ret = mixer_ctl_set_array(ctl, &media_config, sizeof(media_config)/sizeof(media_config[0]));
    free(mixer_str);
    return ret;
}

int set_agm_device_custom_payload(struct mixer *mixer, char *intf_name, void *payload, size_t size)
{
    char *control = "setParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    long media_config[4];
    int ctl_len = 0;
    int ret = 0;

    ctl_len = strlen(intf_name) + 1 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", intf_name, control);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_array(ctl, payload, size);
    free(mixer_str);
    return ret;
}

void get_agm_usb_audio_config_payload(uint8_t** payload, size_t* size,
    uint32_t miid, struct usbAudioConfig *data)
{
    struct apm_module_param_data_t* header;
    struct param_id_usb_audio_intf_cfg_t *usbConfig;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
       sizeof(struct param_id_usb_audio_intf_cfg_t);


    if (payloadSize % 8 != 0)
        payloadSize = payloadSize + (8 - payloadSize % 8);

    payloadInfo = (uint8_t*) calloc(1, payloadSize);
    if (!payloadInfo) {
        printf("payloadInfo new failed %s \n", strerror(errno));
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    usbConfig = (struct param_id_usb_audio_intf_cfg_t*)(payloadInfo +
                sizeof(struct apm_module_param_data_t));
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_USB_AUDIO_INTF_CFG;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    printf("header params \n IID:%x param_id:%x error_code:%d param_size:%d \n",
                     header->module_instance_id, header->param_id,
                     header->error_code, header->param_size);

    usbConfig->usb_token = data->usb_token;
    usbConfig->svc_interval = data->svc_interval;
    printf("customPayload address %pK and size %zu \n", payloadInfo, payloadSize);

    *size = payloadSize;
    *payload = payloadInfo;

}

int connect_play_pcm_to_cap_pcm(struct mixer *mixer, unsigned int p_device, unsigned int c_device)
{
    char *pcm = "PCM";
    char *control = "loopback";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0;
    char *val;
    int val_len = 0;
    int ret = 0;

    ctl_len = strlen(pcm) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", pcm, c_device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    if (p_device < 0) {
        val = "ZERO";
    } else {
        val_len = strlen(pcm) + 4;
        val = calloc(1, val_len);
        if (!val) {
            printf("val calloc failed\n");
            free(mixer_str);
            return -ENOMEM;
        }
        snprintf(val, val_len, "%s%d", pcm, p_device);
    }

    ret = mixer_ctl_set_enum_by_string(ctl, val);
    free(mixer_str);
    if (p_device < 0)
        free(val);

    return ret;
}

int connect_agm_audio_intf_to_stream(struct mixer *mixer, unsigned int device, char *intf_name, enum stream_type stype, bool connect)
{
    char *stream = "PCM";
    char *control;
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0;
    int ret = 0;

    if (connect)
        control = "connect";
    else
        control = "disconnect";

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_enum_by_string(ctl, intf_name);
    free(mixer_str);
    return ret;
}

int agm_mixer_set_ecref_path(struct mixer *mixer, unsigned int device, enum stream_type stype, char *intf_name)
{
    char *stream = "PCM";
    char *control;
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0;
    int ret = 0;

    control = "echoReference";

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_enum_by_string(ctl, intf_name);
    free(mixer_str);
    return ret;
}

int set_agm_audio_intf_metadata(struct mixer *mixer, char *intf_name, unsigned int dkv,
                                enum usecase_type usecase, int rate, int bitwidth, uint32_t val)
{
    char *control = "metadata";
    struct mixer_ctl *ctl;
    char *mixer_str;
    struct agm_key_value *gkv = NULL, *ckv = NULL;
    struct prop_data *prop = NULL;
    uint8_t *metadata = NULL;
    uint32_t num_gkv = 1, num_ckv = 3, num_props = 0;
    uint32_t gkv_size, ckv_size, prop_size, ckv_index = 0;
    int ctl_len = 0, offset = 0;
    int ret = 0;

    gkv_size = num_gkv * sizeof(struct agm_key_value);
    ckv_size = num_ckv * sizeof(struct agm_key_value);
    prop_size = sizeof(struct prop_data) + (num_props * sizeof(uint32_t));

    metadata = calloc(1, sizeof(num_gkv) + sizeof(num_ckv) + gkv_size + ckv_size + prop_size);
    if (!metadata)
        return -ENOMEM;

    gkv = calloc(num_gkv, sizeof(struct agm_key_value));
    ckv = calloc(num_ckv, sizeof(struct agm_key_value));
    prop = calloc(1, prop_size);
    if (!gkv || !ckv || !prop) {
        if (ckv)
            free(ckv);
        if (gkv)
            free(gkv);
        if (prop)
            free(prop);
        free(metadata);
        return -ENOMEM;
    }

    if (usecase == PLAYBACK) {
        gkv[0].key = DEVICERX;
        gkv[0].value = dkv ? dkv : SPEAKER;
    } else if (usecase == HAPTICS) {
        gkv[0].key = DEVICERX;
        gkv[0].value = HAPTICS_DEVICE;
    } else if (val == VOICE_UI) {
        gkv[0].key = DEVICETX;
        gkv[0].value = dkv ? dkv : HANDSETMIC_VA;
    } else {
        gkv[0].key = DEVICETX;
        gkv[0].value = dkv ? dkv : HANDSETMIC;
    }
    ckv[ckv_index].key = SAMPLINGRATE;
    ckv[ckv_index].value = rate;

    ckv_index++;
    ckv[ckv_index].key = BITWIDTH;
    ckv[ckv_index].value = bitwidth;

    ckv_index++;
    ckv[ckv_index].key = GAIN;;
    ckv[ckv_index].value = 0;

    prop->prop_id = 0;  //Update prop_id here
    prop->num_values = num_props;

    memcpy(metadata, &num_gkv, sizeof(num_gkv));
    offset += sizeof(num_gkv);
    memcpy(metadata + offset, gkv, gkv_size);
    offset += gkv_size;
    memcpy(metadata + offset, &num_ckv, sizeof(num_ckv));
    offset += sizeof(num_ckv);
    memcpy(metadata + offset, ckv, ckv_size);
    offset += ckv_size;
    memcpy(metadata + offset, prop, prop_size);

    ctl_len = strlen(intf_name) + 1 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        free(gkv);
        free(ckv);
        free(prop);
        free(metadata);
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", intf_name, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(gkv);
        free(ckv);
        free(prop);
        free(metadata);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_array(ctl, metadata, sizeof(num_gkv) + sizeof(num_ckv) + gkv_size + ckv_size + prop_size);

    free(gkv);
    free(ckv);
    free(prop);
    free(metadata);
    free(mixer_str);
    return ret;
}

int set_agm_stream_metadata_type(struct mixer *mixer, int device, char *val, enum stream_type stype)
{
    char *stream = "PCM";
    char *control = "control";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        printf("mixer_str calloc failed\n");
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_enum_by_string(ctl, val);
    free(mixer_str);
    return ret;
}

void populateChannelMap(uint16_t *pcmChannel, uint8_t numChannel)
{
    if (numChannel == 1) {
        pcmChannel[0] = PCM_CHANNEL_C;
    } else if (numChannel == 2) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
    } else if (numChannel == 3) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
    } else if (numChannel == 4) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_LB;
        pcmChannel[3] = PCM_CHANNEL_RB;
    }  else if (numChannel == 5) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LB;
        pcmChannel[4] = PCM_CHANNEL_RB;
    }  else if (numChannel == 6) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LFE;
        pcmChannel[4] = PCM_CHANNEL_LB;
        pcmChannel[5] = PCM_CHANNEL_RB;
    }  else if (numChannel == 7) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LFE;
        pcmChannel[4] = PCM_CHANNEL_LB;
        pcmChannel[5] = PCM_CHANNEL_RB;
        pcmChannel[6] = PCM_CHANNEL_CS;
    }  else if (numChannel == 8) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LFE;
        pcmChannel[4] = PCM_CHANNEL_LB;
        pcmChannel[5] = PCM_CHANNEL_RB;
        pcmChannel[6] = PCM_CHANNEL_LS;
        pcmChannel[7] = PCM_CHANNEL_RS;
    }
}

int configure_mfc(struct mixer *mixer, int device, char *intf_name, int tag,
                  enum stream_type stype, unsigned int rate,
                  unsigned int channels, unsigned int bits, uint32_t miid)
{
    int ret = 0;
    struct apm_module_param_data_t* header = NULL;
    struct param_id_mfc_output_media_fmt_t *mfcConf;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0, size;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_mfc_output_media_fmt_t) +
                  sizeof(uint16_t)*channels;

    padBytes = PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        return -ENOMEM;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    mfcConf = (struct param_id_mfc_output_media_fmt_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));
    pcmChannel = (uint16_t*)(payloadInfo + sizeof(struct apm_module_param_data_t) +
                                       sizeof(struct param_id_mfc_output_media_fmt_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_MFC_OUTPUT_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    mfcConf->sampling_rate = rate;
    mfcConf->bit_width = bits;
    mfcConf->num_channels = channels;
    populateChannelMap(pcmChannel, channels);
    size = payloadSize + padBytes;

    ret = agm_mixer_set_param(mixer, device, stype, (void *)payloadInfo, (int)size);
    free(payloadInfo);
    return ret;
}

int set_agm_capture_stream_metadata(struct mixer *mixer, int device, uint32_t val, enum usecase_type usecase,
                                    enum stream_type stype, unsigned int instance_kv)
{
    char *stream = "PCM";
    char *control = "metadata";
    char *mixer_str;
    struct mixer_ctl *ctl;
    uint8_t *metadata = NULL;
    struct agm_key_value *gkv = NULL, *ckv = NULL;
    struct prop_data *prop = NULL;
    uint32_t num_gkv = 1, num_ckv = 1, num_props = 0;
    uint32_t gkv_size, ckv_size, prop_size, index = 0;
    int ctl_len = 0, ret = 0, offset = 0;
    char *type = "ZERO";

    ret = set_agm_stream_metadata_type(mixer, device, type, stype);
    if (ret)
        return ret;

    if (instance_kv != 0)
       num_gkv += 1;

    gkv_size = num_gkv * sizeof(struct agm_key_value);
    ckv_size = num_ckv * sizeof(struct agm_key_value);
    prop_size = sizeof(struct prop_data) + (num_props * sizeof(uint32_t));

    metadata = calloc(1, sizeof(num_gkv) + sizeof(num_ckv) + gkv_size + ckv_size + prop_size);
    if (!metadata)
        return -ENOMEM;

    gkv = calloc(num_gkv, sizeof(struct agm_key_value));
    ckv = calloc(num_ckv, sizeof(struct agm_key_value));
    prop = calloc(1, prop_size);
    if (!gkv || !ckv || !prop) {
        if (ckv)
            free(ckv);
        if (gkv)
            free(gkv);
        free(metadata);
        return -ENOMEM;
    }

    gkv[index].key = STREAMTX;
    gkv[index].value = val;
    index++;

    if (instance_kv != 0) {
        gkv[index].key = INSTANCE;
        gkv[index].value = instance_kv;
        index++;
    }

    index = 0;
    ckv[index].key = VOLUME;
    ckv[index].value = LEVEL_0;

    prop->prop_id = 0;  //Update prop_id here
    prop->num_values = num_props;

    memcpy(metadata, &num_gkv, sizeof(num_gkv));
    offset += sizeof(num_gkv);
    memcpy(metadata + offset, gkv, gkv_size);
    offset += gkv_size;
    memcpy(metadata + offset, &num_ckv, sizeof(num_ckv));

    offset += sizeof(num_ckv);
    memcpy(metadata + offset, ckv, ckv_size);
    offset += ckv_size;
    memcpy(metadata + offset, prop, prop_size);

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        ret = -ENOMEM;
        goto done;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        ret = ENOENT;
        goto done;
    }

    ret = mixer_ctl_set_array(ctl, metadata, sizeof(num_gkv) + sizeof(num_ckv) + gkv_size + ckv_size + prop_size);

done:
    free(gkv);
    free(ckv);
    free(prop);
    free(metadata);
    free(mixer_str);
    return ret;
}

int set_agm_streamdevice_metadata(struct mixer *mixer, int device, uint32_t val, enum usecase_type usecase,
                                  enum stream_type stype, char *intf_name,
                                  unsigned int devicepp_kv)
{
    char *stream = "PCM";
    char *control = "metadata";
    char *mixer_str;
    struct mixer_ctl *ctl;
    uint8_t *metadata = NULL;
    struct agm_key_value *gkv = NULL;
    uint32_t num_gkv = 2;
    uint32_t gkv_size, index = 0;
    int ctl_len = 0, ret = 0, offset = 0;
    char *type = intf_name;

    ret = set_agm_stream_metadata_type(mixer, device, type, stype);
    if (ret)
        return ret;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    gkv_size = num_gkv * sizeof(struct agm_key_value);

    metadata = calloc(1, sizeof(num_gkv) +  gkv_size);
    if (!metadata)
        return -ENOMEM;

    gkv = calloc(num_gkv, sizeof(struct agm_key_value));
    if (!gkv) {
        free(metadata);
        return -ENOMEM;
    }
    if (val == VOICE_UI) {
        gkv[index].key = STREAMTX;
        gkv[index].value = val;
        index++;
    } else if (usecase == PLAYBACK) {
        gkv[index].key = STREAMRX;
        gkv[index].value = val;
        index++;
    } else if (usecase == CAPTURE) {
        gkv[index].key = STREAMTX;
        gkv[index].value = val;
        index++;
    }
    if (val == VOICE_UI) {
        gkv[index].key = DEVICEPP_TX;
        gkv[index].value = devicepp_kv;
        index++;
    } else if (usecase == PLAYBACK) {
        gkv[index].key = DEVICEPP_RX;
        gkv[index].value = devicepp_kv;
        index++;
    } else if (usecase == CAPTURE) {
        gkv[index].key = DEVICEPP_TX;
        gkv[index].value = devicepp_kv;
        index++;
    }

    memcpy(metadata, &num_gkv, sizeof(num_gkv));
    offset += sizeof(num_gkv);
    memcpy(metadata + offset, gkv, gkv_size);
    offset += gkv_size;

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        free(gkv);
        free(metadata);
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(gkv);
        free(metadata);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_array(ctl, metadata, sizeof(num_gkv) + gkv_size);

    free(gkv);
    free(metadata);
    free(mixer_str);
    return ret;
}

int set_agm_stream_metadata(struct mixer *mixer, int device, uint32_t val, enum usecase_type usecase,
                            enum stream_type stype, unsigned int instance_kv)
{
    char *stream = "PCM";
    char *control = "metadata";
    char *mixer_str;
    struct mixer_ctl *ctl;
    uint8_t *metadata = NULL;
    struct agm_key_value *gkv = NULL, *ckv = NULL;
    struct prop_data *prop = NULL;
    uint32_t num_gkv = 1, num_ckv = 1, num_props = 0;
    uint32_t gkv_size, ckv_size, prop_size, index = 0;
    int ctl_len = 0, ret = 0, offset = 0;
    char *type = "ZERO";

    ret = set_agm_stream_metadata_type(mixer, device, type, stype);
    if (ret)
        return ret;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    if (instance_kv != 0)
       num_gkv += 1;

    if (val == VOICE_UI) {
        num_gkv = 3;
    }

    gkv_size = num_gkv * sizeof(struct agm_key_value);
    ckv_size = num_ckv * sizeof(struct agm_key_value);
    prop_size = sizeof(struct prop_data) + (num_props * sizeof(uint32_t));

    metadata = calloc(1, sizeof(num_gkv) + sizeof(num_ckv) + gkv_size + ckv_size + prop_size);
    if (!metadata)
        return -ENOMEM;

    gkv = calloc(num_gkv, sizeof(struct agm_key_value));
    ckv = calloc(num_ckv, sizeof(struct agm_key_value));
    prop = calloc(1, prop_size);
    if (!gkv || !ckv || !prop) {
        if (ckv)
            free(ckv);
        if (gkv)
            free(gkv);
        if (prop)
            free(prop);
        free(metadata);
        return -ENOMEM;
    }

    if (val == VOICE_UI) {
        gkv[index].key = STREAMTX;
        gkv[index].value = val;
        index++;
        if (instance_kv != 0){
            gkv[index].key = INSTANCE;
            gkv[index].value = instance_kv;
            index++;
        }
        gkv[index].key = STREAM_CONFIG;
        gkv[index].value = STREAM_CFG_VUI_SVA;
        index++;
    } else if (usecase == PLAYBACK) {
        gkv[index].key = STREAMRX;
        gkv[index].value = val;

        index++;
        if (instance_kv != 0) {
            printf("Instance key is added\n");
            gkv[index].key = INSTANCE;
            gkv[index].value = instance_kv;
            index++;
        }
    } else if (usecase == LOOPBACK) {
        gkv[index].key = STREAMRX;
        gkv[index].value = val;
    }

    index = 0;
    ckv[index].key = VOLUME;
    ckv[index].value = LEVEL_0;

    prop->prop_id = 0;  //Update prop_id here
    prop->num_values = num_props;

    memcpy(metadata, &num_gkv, sizeof(num_gkv));
    offset += sizeof(num_gkv);
    memcpy(metadata + offset, gkv, gkv_size);
    offset += gkv_size;
    memcpy(metadata + offset, &num_ckv, sizeof(num_ckv));

    offset += sizeof(num_ckv);
    memcpy(metadata + offset, ckv, ckv_size);
    offset += ckv_size;
    memcpy(metadata + offset, prop, prop_size);

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        free(gkv);
        free(ckv);
        free(prop);
        free(metadata);
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(gkv);
        free(ckv);
        free(prop);
        free(metadata);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_array(ctl, metadata, sizeof(num_gkv) + sizeof(num_ckv) + gkv_size + ckv_size + prop_size);

    free(gkv);
    free(ckv);
    free(prop);
    free(metadata);
    free(mixer_str);
    return ret;
}

int agm_mixer_register_event(struct mixer *mixer, int device, enum stream_type stype, uint32_t miid, uint8_t is_register)
{
    char *stream = "PCM";
    char *control = "event";
    char *mixer_str;
    struct mixer_ctl *ctl;
    struct agm_event_reg_cfg *event_cfg;
    int payload_size = 0;
    int ctl_len = 0,ret = 0;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ctl_len = sizeof(struct agm_event_reg_cfg) + payload_size;
    event_cfg = calloc(1, ctl_len);
    if (!event_cfg) {
        free(mixer_str);
        return -ENOMEM;
    }

    event_cfg->module_instance_id = miid;
    event_cfg->event_id = EVENT_ID_DETECTION_ENGINE_GENERIC_INFO;
    event_cfg->event_config_payload_size = payload_size;
    event_cfg->is_register = !!is_register;

    ret = mixer_ctl_set_array(ctl, event_cfg, ctl_len);
    free(event_cfg);
    free(mixer_str);
    return ret;
}

int agm_mixer_get_miid(struct mixer *mixer, int device, char *intf_name,
                       enum stream_type stype, int tag_id, uint32_t *miid)
{
    char *stream = "PCM";
    char *control = "getTaggedInfo";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0, i;
    void *payload;
    struct gsl_tag_module_info *tag_info;
    struct gsl_tag_module_info_entry *tag_entry;
    int offset = 0;

    ret = set_agm_stream_metadata_type(mixer, device, intf_name, stype);
    if (ret)
        return ret;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    payload = calloc(1024, sizeof(char));
    if (!payload) {
        free(mixer_str);
        return -ENOMEM;
    }

    ret = mixer_ctl_get_array(ctl, payload, 1024);
    if (ret < 0) {
        printf("Failed to mixer_ctl_get_array\n");
        free(payload);
        free(mixer_str);
        return ret;
    }
    tag_info = payload;
    ret = -1;
    tag_entry = &tag_info->tag_module_entry[0];
    offset = 0;
    for (i = 0; i < tag_info->num_tags; i++) {
        tag_entry += offset/sizeof(struct gsl_tag_module_info_entry);

        offset = sizeof(struct gsl_tag_module_info_entry) + (tag_entry->num_modules * sizeof(struct gsl_module_id_info_entry));
        if (tag_entry->tag_id == tag_id) {
            struct gsl_module_id_info_entry *mod_info_entry;

            if (tag_entry->num_modules) {
                 mod_info_entry = &tag_entry->module_entry[0];
                 *miid = mod_info_entry->module_iid;
                 ret = 0;
                 break;
            }
        }
    }

    free(payload);
    free(mixer_str);
    return ret;
}

int agm_mixer_set_param(struct mixer *mixer, int device,
                        enum stream_type stype, void *payload, int size)
{
    char *stream = "PCM";
    char *control = "setParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";


    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        free(payload);
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }


    ret = mixer_ctl_set_array(ctl, payload, size);

    free(mixer_str);
    return ret;
}

int agm_mixer_set_param_with_file(struct mixer *mixer, int device,
                                  enum stream_type stype, char *path)
{
    FILE *fp;
    int size, bytes_read;
    void *payload;
    char *stream = "PCM";
    char *control = "setParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    fp = fopen(path, "rb");
    if (!fp) {
        printf("%s: Unable to open file '%s'\n", __func__, path);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    printf("%s: size of %s file is %d bytes\n", __func__, path, size);

    payload = calloc(1, size);
    if (!payload)
        return -ENOMEM;

    fseek(fp, 0, SEEK_SET);
    bytes_read = fread((char*)payload, 1, size , fp);
    if (bytes_read != size) {
        printf("%s failed to read data from file %s, bytes read = %d\n", __func__, path, bytes_read);
        free(payload);
        return -1;
    }

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        free(payload);
        fclose(fp);
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        free(payload);
        fclose(fp);
        return ENOENT;
    }


    ret = mixer_ctl_set_array(ctl, payload, size);

    free(mixer_str);
    free(payload);
    fclose(fp);
    return ret;
}

int agm_mixer_get_event_param(struct mixer *mixer, int device, enum stream_type stype, uint32_t miid)
{
    char *stream = "PCM";
    char *control = "getParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    struct apm_module_param_data_t* header;
    struct detection_engine_generic_event_cfg *pEventCfg;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }


    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct detection_engine_generic_event_cfg);

    if (payloadSize % 8 != 0)
        payloadSize = payloadSize + (8 - payloadSize % 8);

    payload = (uint8_t*)malloc((size_t)payloadSize);
    if (!payload) {
        printf("malloc failed\n");
        free(mixer_str);
        return -ENOMEM;
    }
    header = (struct apm_module_param_data_t*)payload;
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_DETECTION_ENGINE_GENERIC_EVENT_CFG;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);

    pEventCfg = (struct detection_engine_generic_event_cfg *)
                (payload + sizeof(struct apm_module_param_data_t));

    ret = mixer_ctl_set_array(ctl, payload, payloadSize);
    if (ret) {
         printf("%s set failed\n", __func__);
         goto exit;
    }

    memset(payload, 0, payloadSize);
    ret = mixer_ctl_get_array(ctl, payload, payloadSize);
    if (ret) {
         printf("%s set failed\n", __func__);
         goto exit;
    }

    printf("received payload data is 0x%x\n", pEventCfg->event_mode);
exit:
    free(mixer_str);
    free(payload);
    return ret;
}

int agm_mixer_get_buf_tstamp(struct mixer *mixer, int device, enum stream_type stype, uint64_t *tstamp)
{
    char *stream = "PCM";
    char *control = "bufTimestamp";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    uint64_t ts = 0;

    if (stype == STREAM_COMPRESS)
        stream = "COMPRESS";

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s%d %s", stream, device, control);

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_get_array(ctl, &ts, sizeof(uint64_t));
    if (ret) {
         printf("%s get failed\n", __func__);
         goto exit;
    }

    *tstamp = ts;
exit:
    free(mixer_str);
    return ret;
}
