/*
** Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
** Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted (subject to the limitations in the
** disclaimer below) provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**
**   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
** GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
** HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
** GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
** IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef __AGM_DEVICE_H__
#define __AGM_DEVICE_H__

#include <pthread.h>
#include <agm/agm_list.h>
#include <agm/agm_priv.h>
#ifdef DEVICE_USES_ALSALIB
#include <alsa/asoundlib.h>
#endif

#define MAX_DEV_NAME_LEN     80

#define  PCM_INTF_IDX_PRIMARY       0x0
#define  PCM_INTF_IDX_SECONDARY     0x1
#define  PCM_INTF_IDX_TERTIARY      0x2
#define  PCM_INTF_IDX_QUATERNARY    0x3
#define  PCM_INTF_IDX_QUINARY       0x4
#define  PCM_INTF_IDX_SENARY        0x5

#define  CODEC_DMA                  0x0
#define  MI2S                       0x1
#define  TDM                        0x2
#define  AUXPCM                     0x3
#define  SLIMBUS                    0x4
#define  DISPLAY_PORT               0x5
#define  USB_AUDIO                  0x6
#define  PCM_RT_PROXY               0x7
#define  AUDIOSS_DMA                0x8
#define  PCM_DUMMY                  0x9
#define  BTFM_PROXY                 0xa

#define  AUDIO_OUTPUT               0x1 /**< playback usecases*/
#define  AUDIO_INPUT                0x2 /**< capture/voice activation usecases*/

struct device_obj;

struct refcount {
    int open;
    int prepare;
    int start;
};

enum device_state {
    DEV_CLOSED,
    DEV_OPENED,
    DEV_PREPARED,
    DEV_STARTED,
    DEV_STOPPED,
};

struct hw_ep_cdc_dma_i2s_tdm_config {
    /* lpaif type e.g. LPAIF, LPAIF_WSA, LPAIF_RX_TX*/
    uint32_t lpaif_type;
    /* Interface ID e.g. Primary, Secondary, Tertiary ….*/
    uint32_t intf_idx;
};

struct hw_ep_slimbus_config {
    /* Slimbus device id : Expected values :
     * DEV1 : 0
     * DEV2 : 1
     */
    uint32_t dev_id;
};

struct hw_ep_pcm_rt_proxy_config {
    /* RT proxy device id */
    uint32_t dev_id;
};

struct hw_ep_audioss_dma_config {
    /* lpaid type e.g LPAIF_VA */
    uint32_t lpaif_type;
    /* AudioSS DMA device id */
    uint32_t dev_id;
};

struct hw_ep_pcm_dummy_config {
    /* PCM dummy device id */
    uint32_t dev_id;
};

union hw_ep_config {
    struct hw_ep_cdc_dma_i2s_tdm_config cdc_dma_i2s_tdm_config;
    struct hw_ep_slimbus_config slim_config;
    struct hw_ep_pcm_rt_proxy_config pcm_rt_proxy_config;
    struct hw_ep_audioss_dma_config audioss_dma_config;
    struct hw_ep_pcm_dummy_config pcm_dummy_config;
};

typedef struct hw_ep_info
{
    /* interface e.g. CODEC_DMA, I2S, AUX_PCM, SLIM, DISPLAY_PORT */
    uint32_t intf;
    /* Direction of the interface RX or TX (Sink or Source)*/
    uint32_t dir;

    union hw_ep_config ep_config;
}hw_ep_info_t;

struct device_group_data {
    char name[MAX_DEV_NAME_LEN];
    bool has_multiple_dai_link;
    struct refcount refcnt;
    struct agm_group_media_config media_config;
    struct listnode list_node;
};

struct device_obj {
    /*
     * name of the device object populated from /proc/asound/pcm
     * <Interface_type>-<LPAIF_TYPE>-<DIRECTION>-<IDX>
     * <SLIM>-<DEVICEID>-<DIRECTION>-<IDX>
     * e.g.
     * TDM-LPAIF_WSA-RX-SECONDARY
     * SLIM-DEVICE_1-TX-0
     */
    char name[MAX_DEV_NAME_LEN];

    struct listnode list_node;
    pthread_mutex_t lock;
    /* pcm device info associated with the device object */
    uint32_t card_id;
    hw_ep_info_t hw_ep_info;
    uint32_t pcm_id;
#ifdef DEVICE_USES_ALSALIB
    snd_pcm_t *pcm;
#else
    struct pcm *pcm;
#endif
    struct agm_media_config media_config;
    struct agm_meta_data_gsl metadata;
    struct refcount refcnt;
    int state;
    void *params;
    size_t params_size;

    bool is_virtual_device;
    int num_virtual_child;
    struct device_obj *parent_dev;
    struct device_group_data *group_data;
};

/* Initializes device_obj, enumerate and fill device related information */
int device_init();
void device_deinit();
/* Returns list of supported devices */
int device_get_aif_info_list(struct aif_info *aif_list, size_t *audio_intfs);
/* returns device_obj associated with device_id */
int device_get_obj(uint32_t device_idx, struct device_obj **dev_obj);
int device_get_hw_ep_info(struct device_obj *dev_obj,
                                     struct hw_ep_info *hw_ep_info_);
int populate_device_hw_ep_info(struct device_obj *dev_obj);
struct device_obj* populate_virtual_device_hw_ep_info(struct device_obj *dev_obj, int num);
int device_open(struct device_obj *dev_obj);
int device_prepare(struct device_obj *dev_obj);
int device_start(struct device_obj *dev_obj);
int device_stop(struct device_obj *dev_obj);
int device_close(struct device_obj *dev_obj);

enum device_state device_current_state(struct device_obj *obj);
/* api to set device media config */
int device_set_media_config(struct device_obj *obj,
                 struct agm_media_config *device_media_config);
/* api to set device meta graph keys + cal keys */
int device_set_metadata(struct device_obj *obj, uint32_t size,
                 uint8_t *payload);
/* api to set device setparam payload */
int device_set_params(struct device_obj *obj, void *payload, size_t size);

int populate_device_hw_ep_info(struct device_obj *dev_obj);

int device_get_snd_card_id();
int device_get_channel_map(struct device_obj *dev_obj, uint32_t **chmap);

int device_group_set_media_config(struct device_group_data *grp_data,
          struct agm_group_media_config *device_media_config);
int device_get_group_data(uint32_t group_id , struct device_group_data **grp_data);
int device_get_group_list(struct aif_info *aif_list, size_t *num_groups);

int device_get_start_refcnt(struct device_obj *dev_obj);
int device_get_state(struct device_obj *dev_obj);
bool get_file_path_extn(char* file_path_extn, char* file_path_extn_wo_variant);
#endif
