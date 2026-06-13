/*
** Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
**/

#define LOG_TAG "AGM: device"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <agm/device.h>
#include <agm/metadata.h>
#include <agm/utils.h>
#ifdef DEVICE_USES_ALSALIB
#include <alsa/asoundlib.h>
#else
#include <tinyalsa/asoundlib.h>
#endif

#define SNDCARD_PATH "/sys/kernel/snd_card/card_state"
#define PCM_DEVICE_FILE "/proc/asound/pcm"

// { SEC_AUDIO_BOOT_ON_ERR // 100s -> 60s
#define MAX_RETRY 60 /*Device will try these many times before return an error*/
// } SEC_AUDIO_BOOT_ON_ERR
#define RETRY_INTERVAL 1 /*Retry interval in seconds*/

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_DEVICE
#include <log_utils.h>
#endif

#define TRUE 1
#define FALSE 0

#define BUF_SIZE 1024
#define FILE_PATH_EXTN_MAX_SIZE 80
#define MAX_RETRY_CNT 20
#define SND_CARD_DEVICE_FILE "/proc/asound/cards"

/* Global list to store supported devices */
static struct listnode device_list;
static struct listnode device_group_data_list;
static uint32_t num_audio_intfs;
static uint32_t num_group_devices;

#ifdef DEVICE_USES_ALSALIB
static snd_ctl_t *mixer;
#else
static struct mixer *mixer = NULL;
#endif

#define MAX_BUF_SIZE                 2048
/**
  * The maximum period bytes for dummy dai is 8192 bytes.
  * Hard coding of period size to 960 frames was leading
  * to bigger period bytes for multi channels.
  * So, now based on frame size, Period size is being calculated.
  * 1 frame = bytes_per_sample * channels
  * period size = 8192/(bytes_per_sample * channels)
  */
#define MAX_PERIOD_BUFFER            8192
#define DEFAULT_PERIOD_COUNT         2

#define MAX_USR_INPUT 9

#define AGM_PCM_RATE_5512  (5512)
#define AGM_PCM_RATE_8000  (8000)
#define AGM_PCM_RATE_11025 (11025)
#define AGM_PCM_RATE_16000 (16000)
#define AGM_PCM_RATE_22050 (2205)
#define AGM_PCM_RATE_32000 (32000)
#define AGM_PCM_RATE_44100 (44100)
#define AGM_PCM_RATE_48000 (48000)
#define AGM_PCM_RATE_64000 (64000)
#define AGM_PCM_RATE_88200 (88200)
#define AGM_PCM_RATE_96000 (96000)
#define AGM_PCM_RATE_176400 (176400)
#define AGM_PCM_RATE_192000 (192000)
#define AGM_PCM_RATE_352800 (352800)
#define AGM_PCM_RATE_384000 (384000)

#define AGM_DEFAULT_PCM_RATE (AGM_PCM_RATE_48000)

/** Sound card state */
typedef enum snd_card_status_t {
    SND_CARD_STATUS_OFFLINE = 0,
    SND_CARD_STATUS_ONLINE,
    SND_CARD_STATUS_NONE,
} snd_card_status_t;

int get_pcm_bits_per_sample(enum agm_media_format fmt_id)
{
     int bits_per_sample = 16;

     switch(fmt_id) {
         case AGM_FORMAT_PCM_S8:      /**< 8-bit signed */
              bits_per_sample = 8;
              break;
         case AGM_FORMAT_PCM_S24_LE:  /**< 24-bits in 4-bytes, 8_24 form*/
              bits_per_sample = 32;
              break;
         case AGM_FORMAT_PCM_S24_3LE: /**< 24-bits in 3-bytes */
              bits_per_sample = 24;
              break;
         case AGM_FORMAT_PCM_S32_LE:  /**< 32-bit signed */
              bits_per_sample = 32;
              break;
         case AGM_FORMAT_PCM_S16_LE:  /**< 16-bit signed */
         default:
              bits_per_sample = 16;
              break;
     }
     return bits_per_sample;
}

int device_get_snd_card_id()
{
    struct device_obj *dev_obj = node_to_item(list_head(&device_list),
                                              struct device_obj, list_node);

    if (dev_obj == NULL) {
        AGM_LOGE("%s: Invalid device object\n", __func__);
        return -EINVAL;
    }
    return dev_obj->card_id;
}

static struct device_obj *device_get_pcm_obj(struct device_obj *dev_obj)
{
    if (dev_obj->parent_dev)
        return dev_obj->parent_dev;
    else
        return dev_obj;
}

#ifdef DEVICE_USES_ALSALIB
snd_pcm_format_t agm_to_alsa_format(enum agm_media_format format)
{
    switch (format) {
    case AGM_FORMAT_PCM_S32_LE:
        return SND_PCM_FORMAT_S32_LE;
    case AGM_FORMAT_PCM_S8:
        return SND_PCM_FORMAT_S8;
    case AGM_FORMAT_PCM_S24_3LE:
        return SND_PCM_FORMAT_S24_3LE;
    case AGM_FORMAT_PCM_S24_LE:
        return SND_PCM_FORMAT_S24_LE;
    default:
    case AGM_FORMAT_PCM_S16_LE:
        return SND_PCM_FORMAT_S16_LE;
    };
}


int device_open(struct device_obj *dev_obj)
{
    int ret = 0;
    snd_pcm_t *pcm;
    char pcm_name[80];
    snd_pcm_stream_t stream;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_format_t format;
    snd_pcm_uframes_t period_size;
    unsigned int rate, channels, period_count;
    struct device_group_data *grp_data = NULL;
    struct agm_media_config *media_config = NULL;
    struct device_obj *obj = NULL;

    if (dev_obj == NULL) {
        AGM_LOGE("%s: Invalid device object\n", __func__);
        return -EINVAL;
    }

    obj = device_get_pcm_obj(dev_obj);

    snprintf(pcm_name, sizeof(pcm_name), "hw:%u,%u", obj->card_id, obj->pcm_id);

    stream = (obj->hw_ep_info.dir == AUDIO_OUTPUT) ?
             SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;

    pthread_mutex_lock(&obj->lock);

    if (obj->group_data)
        grp_data = obj->group_data;

    if (obj->refcnt.open) {
        AGM_LOGE("%s: PCM device %u already opened\n",
                           __func__, obj->pcm_id);
        obj->refcnt.open++;
        if (grp_data)
            grp_data->refcnt.open++;
        goto done;
    }

    if (grp_data && !grp_data->has_multiple_dai_link)
        media_config = &grp_data->media_config.config;
    else
        media_config = &dev_obj->media_config;

    channels = media_config->channels;
    rate = media_config->rate;
    format = agm_to_alsa_format(media_config->format);
    period_size = (MAX_PERIOD_BUFFER)/(channels *
                          (get_pcm_bits_per_sample(media_config->format)/8));
    period_count = DEFAULT_PERIOD_COUNT;

    ret = snd_pcm_open(&pcm, pcm_name, stream, 0);
    if (ret < 0) {
        AGM_LOGE("%s: Unable to open PCM device %s", __func__, pcm_name);
        goto done;
    }

    snd_pcm_hw_params_alloca(&hwparams);

    ret = snd_pcm_hw_params_any(pcm, hwparams);
    if (ret < 0) {
        AGM_LOGE("Default config not available for %s, exiting", pcm_name);
        goto done;
    }
    snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hwparams, format);
    snd_pcm_hw_params_set_channels(pcm, hwparams, channels);
    snd_pcm_hw_params_set_rate(pcm, hwparams, rate, 0);
    snd_pcm_hw_params_set_period_size(pcm, hwparams, period_size, 0);
    snd_pcm_hw_params_set_periods(pcm, hwparams, period_count, 0);

    ret = snd_pcm_hw_params(pcm, hwparams);
    if (ret < 0) {
        AGM_LOGE("%s unable to set hw params for %s, rate[%u], ch[%u], fmt[%u]",
                 __func__, pcm_name, rate, channels, format);
        goto done;
    }
    obj->pcm = pcm;
    obj->state = DEV_OPENED;
    obj->refcnt.open++;
    if (grp_data)
        grp_data->refcnt.open++;
done:
    pthread_mutex_unlock(&obj->lock);
    return ret;
}
#else
enum pcm_format agm_to_pcm_format(enum agm_media_format format)
{
    switch (format) {
    case AGM_FORMAT_PCM_S32_LE:
        return PCM_FORMAT_S32_LE;
    case AGM_FORMAT_PCM_S8:
        return PCM_FORMAT_S8;
    case AGM_FORMAT_PCM_S24_3LE:
        return PCM_FORMAT_S24_3LE;
    case AGM_FORMAT_PCM_S24_LE:
        return PCM_FORMAT_S24_LE;
    default:
    case AGM_FORMAT_PCM_S16_LE:
        return PCM_FORMAT_S16_LE;
    };
}

bool device_pcm_is_rate_supported(unsigned int rate)
{
    switch(rate) {
    case AGM_PCM_RATE_5512:
    case AGM_PCM_RATE_8000:
    case AGM_PCM_RATE_11025:
    case AGM_PCM_RATE_16000:
    case AGM_PCM_RATE_22050:
    case AGM_PCM_RATE_32000:
    case AGM_PCM_RATE_44100:
    case AGM_PCM_RATE_48000:
    case AGM_PCM_RATE_64000:
    case AGM_PCM_RATE_88200:
    case AGM_PCM_RATE_96000:
    case AGM_PCM_RATE_176400:
    case AGM_PCM_RATE_192000:
    case AGM_PCM_RATE_352800:
    case AGM_PCM_RATE_384000:
        return true;
    default:
        return false;
    };
}

int device_open(struct device_obj *dev_obj)
{
    int ret = 0;
    struct pcm *pcm = NULL;
    struct pcm_config config;
    uint32_t pcm_flags;
    struct device_group_data *grp_data = NULL;
    struct agm_media_config *media_config = NULL;
    struct device_obj *obj = NULL;

    if (dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return -EINVAL;
    }

    obj = device_get_pcm_obj(dev_obj);
    pthread_mutex_lock(&obj->lock);

    if (obj->group_data)
        grp_data = obj->group_data;

    if (obj->refcnt.open) {
        AGM_LOGI("PCM device %u already opened\n",
                 obj->pcm_id);
        obj->refcnt.open++;
        if (grp_data)
            grp_data->refcnt.open++;
        goto done;
    }

    memset(&config, 0, sizeof(struct pcm_config));
    if (grp_data && !grp_data->has_multiple_dai_link)
        media_config = &grp_data->media_config.config;
    else
        media_config = &dev_obj->media_config;

    config.channels = media_config->channels;
    config.rate = media_config->rate;
    if (!device_pcm_is_rate_supported(config.rate)) {
        AGM_LOGD("Unsupported PCM rate %d changing to default rate %d\n",
              config.rate, AGM_DEFAULT_PCM_RATE);
        config.rate = AGM_DEFAULT_PCM_RATE;
    }

    config.format = agm_to_pcm_format(media_config->format);
    config.period_size = (MAX_PERIOD_BUFFER)/(config.channels *
                          (get_pcm_bits_per_sample(media_config->format)/8));

    config.period_count = DEFAULT_PERIOD_COUNT;
    config.start_threshold = config.period_size / 4;
    config.stop_threshold = INT_MAX;

    pcm_flags = (obj->hw_ep_info.dir == AUDIO_OUTPUT) ? PCM_OUT : PCM_IN;
    pcm = pcm_open(obj->card_id, obj->pcm_id, pcm_flags,
                &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        AGM_LOGE("Unable to open PCM device %u (%s) rate %u ch %d fmt %u",
                obj->pcm_id, pcm_get_error(pcm), config.rate,
                config.channels, config.format);
        AGM_LOGE("Period Size %d \n", config.period_size);
        ret = -EIO;
        goto done;
    }
    obj->pcm = pcm;
    obj->state = DEV_OPENED;
    obj->refcnt.open++;
    if (grp_data)
        grp_data->refcnt.open++;
done:
    pthread_mutex_unlock(&obj->lock);
    return ret;
}
#endif

int device_prepare(struct device_obj *dev_obj)
{
    int ret = 0;
    struct device_group_data *grp_data = NULL;
    struct device_obj *obj = NULL;

    if (dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return -EINVAL;
    }

    obj = device_get_pcm_obj(dev_obj);

    pthread_mutex_lock(&obj->lock);

    if (obj->group_data)
        grp_data = obj->group_data;

    if (obj->refcnt.prepare) {
        AGM_LOGD("PCM device %u already in prepare state\n",
              obj->pcm_id);
        obj->refcnt.prepare++;
        if (grp_data)
            grp_data->refcnt.prepare++;
        pthread_mutex_unlock(&obj->lock);
        return ret;
    }
#ifdef DEVICE_USES_ALSALIB
    ret = snd_pcm_prepare(obj->pcm);
#else
    ret = pcm_prepare(obj->pcm);
#endif
    if (ret) {
        AGM_LOGE("PCM device %u prepare failed, ret = %d\n",
              obj->pcm_id, ret);
        goto done;
    }

    obj->state = DEV_PREPARED;
    obj->refcnt.prepare++;

done:
    pthread_mutex_unlock(&obj->lock);
    return ret;
}

int device_start(struct device_obj *dev_obj)
{
    int ret = 0;
    struct device_group_data *grp_data = NULL;
    struct device_obj *obj = NULL;

    if (dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return -EINVAL;
    }

    obj = device_get_pcm_obj(dev_obj);

    pthread_mutex_lock(&obj->lock);
    if (obj->state < DEV_PREPARED) {
            AGM_LOGE("PCM device %u not yet prepared, exiting\n",
                  obj->pcm_id);
            ret =  -1;
            goto done;
    }

    if (obj->group_data)
        grp_data = obj->group_data;

    if (obj->refcnt.start) {
        AGM_LOGI("PCM device %u already in start state\n",
              obj->pcm_id);
        obj->refcnt.start++;
        if (grp_data)
            grp_data->refcnt.start++;
        goto done;
    }

    obj->state = DEV_STARTED;
    obj->refcnt.start++;
    if (grp_data)
        grp_data->refcnt.start++;

done:
    pthread_mutex_unlock(&obj->lock);
    return ret;
}

int device_stop(struct device_obj *dev_obj)
{
    int ret = 0;
    struct device_group_data *grp_data = NULL;
    struct device_obj *obj = NULL;

    if(dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return -EINVAL;
    }

    obj = device_get_pcm_obj(dev_obj);

    pthread_mutex_lock(&obj->lock);
    if (!obj->refcnt.start) {
        AGM_LOGE("PCM device %u already stopped\n",
              obj->pcm_id);
        goto done;
    }

    if (obj->group_data) {
        grp_data = obj->group_data;
        grp_data->refcnt.start--;
    }

    obj->refcnt.start--;
    if (obj->refcnt.start == 0) {
#ifdef DEVICE_USES_ALSALIB
        ret = snd_pcm_drop(obj->pcm);
#else
        ret = pcm_stop(obj->pcm);
#endif
        if (ret) {
            AGM_LOGE("PCM device %u stop failed, ret = %d\n",
                    obj->pcm_id, ret);
        }
        obj->state = DEV_STOPPED;
    }

done:
    pthread_mutex_unlock(&obj->lock);
    return ret;
}

int device_close(struct device_obj *dev_obj)
{
    int ret = 0;
    struct device_group_data *grp_data = NULL;
    struct device_obj *obj = NULL;

    if (dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return -EINVAL;
    }

    obj = device_get_pcm_obj(dev_obj);

    pthread_mutex_lock(&obj->lock);
    if (!obj->refcnt.open) {
        AGM_LOGE("PCM device %u already closed\n",
              obj->pcm_id);
        goto done;
    }

    if (obj->group_data) {
        grp_data = obj->group_data;
        if (--grp_data->refcnt.open == 0) {
            grp_data->refcnt.prepare = 0;
            grp_data->refcnt.start = 0;
        }
    }

    if (--obj->refcnt.open == 0) {
#ifdef DEVICE_USES_ALSALIB
        ret = snd_pcm_close(obj->pcm);
#else
        ret = pcm_close(obj->pcm);
#endif
        if (ret) {
            AGM_LOGE("PCM device %u close failed, ret = %d\n",
                     obj->pcm_id, ret);
        }
// { SEC_AUDIO_ADD_FOR_DEBUG
        else {
            AGM_LOGI("PCM device %u closed\n", obj->pcm_id);
        }
// } SEC_AUDIO_ADD_FOR_DEBUG

        obj->state = DEV_CLOSED;
        obj->refcnt.prepare = 0;
        obj->refcnt.start = 0;
    }

done:
    pthread_mutex_unlock(&obj->lock);
    return ret;
}

enum device_state device_current_state(struct device_obj *dev_obj)
{
    return dev_obj->state;
}

int device_get_aif_info_list(struct aif_info *aif_list, size_t *audio_intfs)
{
    struct device_obj *dev_obj;
    uint32_t copied = 0;
    uint32_t requested = *audio_intfs;
    struct listnode *dev_node, *temp;

    if (*audio_intfs == 0){
        *audio_intfs = num_audio_intfs;
    } else {
        list_for_each_safe(dev_node, temp, &device_list) {
            dev_obj = node_to_item(dev_node, struct device_obj, list_node);
            strlcpy(aif_list[copied].aif_name, dev_obj->name,
                                          AIF_NAME_MAX_LEN);
            aif_list[copied].dir = dev_obj->hw_ep_info.dir;
            copied++;
            if (copied == requested)
                break;
        }
        *audio_intfs = copied;
    }
    return 0;
}

int device_get_group_list(struct aif_info *aif_list, size_t *num_groups)
{
    struct device_group_data *grp_data;
    uint32_t copied = 0;
    uint32_t requested = *num_groups;
    struct listnode *grp_node, *temp;

    if (*num_groups == 0){
        *num_groups = num_group_devices;
    } else {
        list_for_each_safe(grp_node, temp, &device_group_data_list) {
            grp_data = node_to_item(grp_node, struct device_group_data, list_node);
            strlcpy(aif_list[copied].aif_name, grp_data->name,
                                          AIF_NAME_MAX_LEN);
            copied++;
            if (copied == requested)
                break;
        }
        *num_groups = copied;
    }
    return 0;
}

int device_get_obj(uint32_t device_idx, struct device_obj **dev_obj)
{
    int i = 0;
    struct listnode *dev_node, *temp;
    struct device_obj *obj;

    if (device_idx > num_audio_intfs) {
        AGM_LOGE("Invalid device_id %u, max_supported device id: %d\n",
                device_idx, num_audio_intfs);
        return -EINVAL;
    }

    list_for_each_safe(dev_node, temp, &device_list) {
        if (i++ == device_idx) {
            obj = node_to_item(dev_node, struct device_obj, list_node);
            *dev_obj = obj;
            return 0;
        }
    }
    return -EINVAL;
}

int device_get_group_data(uint32_t group_id , struct device_group_data **grp_data)
{
    int i = 0;
    struct listnode *group_node, *temp;
    struct device_group_data *data;

    if (group_id >= num_group_devices) {
        AGM_LOGE("Invalid group_id %u, max_supported device id: %d\n",
                group_id, num_group_devices);
        return -EINVAL;
    }

    list_for_each_safe(group_node, temp, &device_group_data_list) {
        if (i++ == group_id) {
            data = node_to_item(group_node, struct device_group_data, list_node);
            *grp_data = data;
            return 0;
        }
    }
    return -EINVAL;
}

int device_set_media_config(struct device_obj *dev_obj,
          struct agm_media_config *device_media_config)
{
   if (dev_obj == NULL || device_media_config == NULL) {
       AGM_LOGE("Invalid device object\n");
       return -EINVAL;
   }

   pthread_mutex_lock(&dev_obj->lock);
   dev_obj->media_config.channels = device_media_config->channels;
   dev_obj->media_config.rate = device_media_config->rate;
   dev_obj->media_config.format = device_media_config->format;
   dev_obj->media_config.data_format = device_media_config->data_format;
   pthread_mutex_unlock(&dev_obj->lock);

   return 0;
}

int device_group_set_media_config(struct device_group_data *grp_data,
          struct agm_group_media_config *media_config)
{
   if (grp_data == NULL || media_config == NULL) {
       AGM_LOGE("Invalid input\n");
       return -EINVAL;
   }
   memcpy(&grp_data->media_config, media_config, sizeof(*media_config));

   return 0;
}

int device_set_metadata(struct device_obj *dev_obj, uint32_t size,
                                                uint8_t *metadata)
{
   int ret = 0;

   pthread_mutex_lock(&dev_obj->lock);
   metadata_free(&dev_obj->metadata);
   ret = metadata_copy(&(dev_obj->metadata), size, metadata);
#ifdef AGM_DEBUG_METADATA
   AGM_LOGI("Setting device metadata for %s\n", dev_obj->name);
   metadata_print(&(dev_obj->metadata));
#endif
   pthread_mutex_unlock(&dev_obj->lock);

   return ret;
}

int device_set_params(struct device_obj *dev_obj,
                      void *payload, size_t size)
{
   int ret = 0;

   pthread_mutex_lock(&dev_obj->lock);

   if (dev_obj->params) {
       free(dev_obj->params);
       dev_obj->params = NULL;
       dev_obj->params_size = 0;
   }

   dev_obj->params = calloc(1, size);
   if (!dev_obj->params) {
       AGM_LOGE("No memory for dev params on dev_id:%d\n",
                                   dev_obj->pcm_id);
       ret = -EINVAL;
       goto done;
   }

   memcpy(dev_obj->params, payload, size);
   dev_obj->params_size = size;

done:
   pthread_mutex_unlock(&dev_obj->lock);
   return ret;
}

#ifdef DEVICE_USES_ALSALIB
int device_get_channel_map(struct device_obj *dev_obj, uint32_t **chmap)
{
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_info_t *info;
    snd_ctl_elem_value_t *control;
    char *mixer_str = NULL;
    int i, ctl_len = 0, ret = 0, card_id;
    uint8_t *payload = NULL;
    char card[16];
    char *dev_name = NULL;

    if (mixer == NULL) {
        card_id = device_get_snd_card_id();
        if (card_id < 0) {
            AGM_LOGE("%s: failed to get card_id", __func__);
            return -EINVAL;
        }

        snprintf(card, 16, "hw:%u", card_id);
        if ((ret = snd_ctl_open(&mixer, card, 0)) < 0) {
            AGM_LOGE("Control device %s open error: %s", card, snd_strerror(ret));
            return ret;
        }

    }

    if (dev_obj->parent_dev)
        dev_name = dev_obj->parent_dev->name;
    else
        dev_name = dev_obj->name;

    ctl_len = strlen(dev_name) + 1 + strlen("Channel Map") + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        AGM_LOGE("%s: Failed to allocate memory for mixer_str", __func__);
        return -ENOMEM;
    }

    payload = calloc(16, sizeof(uint32_t));
    if (!payload) {
        AGM_LOGE("%s: Failed to allocate memory for payload", __func__);
        ret = -ENOMEM;
        goto done;
    }

    snprintf(mixer_str, ctl_len, "%s %s", dev_name, "Channel Map");

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_info_alloca(&info);
    snd_ctl_elem_value_alloca(&control);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_name(id, (const char *)mixer_str);

    snd_ctl_elem_info_set_id(info, id);
    ret = snd_ctl_elem_info(mixer, info);
    if (ret < 0) {
        AGM_LOGE("Cannot get element info for %s\n", mixer_str);
        goto done;
    }
    snd_ctl_elem_info_get_id(info, id);

    snd_ctl_elem_value_set_id(control, id);
    ret = snd_ctl_elem_read(mixer, control);
    if (ret < 0) {
        AGM_LOGE("Failed to mixer_ctl_get_array\n");
        goto err_get_ctl;
    }

    for (i = 0; i < 16 * sizeof(uint32_t); i++)
        payload[i] = snd_ctl_elem_value_get_byte(control, i);

    *chmap = payload;
    goto done;

err_get_ctl:
done:
    free(mixer_str);
    return ret;
}
#else
int device_get_channel_map(struct device_obj *dev_obj, uint32_t **chmap)
{
    struct mixer_ctl *ctl = NULL;
    char *mixer_str = NULL;
    int ctl_len = 0, ret = 0, card_id;
    void *payload = NULL;
    char *dev_name = NULL;

    if (mixer == NULL) {
        card_id = device_get_snd_card_id();
        if (card_id < 0) {
            AGM_LOGE("failed to get card_id");
            return -EINVAL;
        }

        mixer = mixer_open(card_id);
        if (!mixer) {
            AGM_LOGE("failed to get mixer handle");
            return -EINVAL;
        }
    }

    if (dev_obj->parent_dev)
        dev_name = dev_obj->parent_dev->name;
    else
        dev_name = dev_obj->name;

    ctl_len = strlen(dev_name) + 1 + strlen("Channel Map") + 1;
    mixer_str = calloc(1, ctl_len);
    if (!mixer_str) {
        AGM_LOGE("Failed to allocate memory for mixer_str");
        return -ENOMEM;
    }

    payload = calloc(16, sizeof(uint32_t));
    if (!payload) {
        AGM_LOGE("Failed to allocate memory for payload");
        ret = -ENOMEM;
        goto done;
    }

    snprintf(mixer_str, ctl_len, "%s %s", dev_name, "Channel Map");

    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        AGM_LOGE("Invalid mixer control: %s\n", mixer_str);
        ret = -ENOENT;
        goto err_get_ctl;
    }

    ret = mixer_ctl_get_array(ctl, payload, 16 * sizeof(uint32_t));
    if (ret < 0) {
        AGM_LOGE("Failed to mixer_ctl_get_array\n");
        goto err_get_ctl;
    }
    *chmap = payload;
    goto done;

err_get_ctl:
    free(payload);
done:
    free(mixer_str);
    return ret;
}
#endif

int device_get_start_refcnt(struct device_obj *dev_obj)
{
   if (dev_obj == NULL) {
       AGM_LOGE("Invalid device object\n");
       return 0;
   }

   if (dev_obj->group_data)
       return dev_obj->group_data->refcnt.start;
   else
       return dev_obj->refcnt.start;

}

int device_get_state(struct device_obj *dev_obj)
{
    if (dev_obj == NULL) {
        AGM_LOGE("Invalid device object\n");
        return DEV_CLOSED;
    }

    if (dev_obj->parent_dev)
        return dev_obj->parent_dev->state;
    else
        return dev_obj->state;
}

static struct device_group_data* device_get_group_data_by_name(char *dev_name)
{
    struct device_group_data *grp_data = NULL;
    char group_name[MAX_DEV_NAME_LEN];
    char *ptr = NULL;
    int pos = 0;
    struct listnode *grp_node, *temp;

    memset(group_name, 0, MAX_DEV_NAME_LEN);

    ptr = strstr(dev_name, "-VIRT");
    pos = ptr - dev_name + 1;
    strlcpy(group_name, dev_name, pos);

    list_for_each_safe(grp_node, temp, &device_group_data_list) {
           grp_data = node_to_item(grp_node, struct device_group_data, list_node);
           if (!strncmp(group_name, grp_data->name, MAX_DEV_NAME_LEN)) {
               grp_data->has_multiple_dai_link = true;
               goto done;
           }
    }

    grp_data = calloc(1, sizeof(struct device_group_data));
    if (grp_data == NULL) {
        AGM_LOGE("memory allocation for group_data failed\n");
        return NULL;
    }

    strlcpy(grp_data->name, group_name, pos);
    list_add_tail(&device_group_data_list, &grp_data->list_node);
    num_group_devices++;

done:
    return grp_data;
}

int parse_snd_card()
{
    char buffer[MAX_BUF_SIZE];
    unsigned int count = 0, i = 0;
    FILE *fp;
    int ret = 0;
    struct listnode *dev_node, *temp;
    struct device_obj *dev_obj = NULL;

    fp = fopen(PCM_DEVICE_FILE, "r");
    if (!fp) {
        AGM_LOGE("ERROR. %s file open failed\n",
                         PCM_DEVICE_FILE);
        return -ENODEV;
    }

    list_init(&device_list);
    list_init(&device_group_data_list);
    num_group_devices = 0;
    while (fgets(buffer, MAX_BUF_SIZE - 1, fp) != NULL)
    {
        dev_obj = calloc(1, sizeof(struct device_obj));

        if (!dev_obj) {
            AGM_LOGE("failed to allocate device_obj mem\n");
            ret = -ENOMEM;
            goto free_device;
        }

        AGM_LOGV("buffer: %s\n", buffer);
        /* For Non-DPCM Dai-links, it is in the format of:
         * <card_num>-<pcm_device_id>: <pcm->idname> : <pcm->name> :
                                                <playback/capture> 1
         * Here, pcm->idname is in the form of "<dai_link->stream_name>
                                          <codec_name>-<num_codecs>"
         */
        sscanf(buffer, "%02u-%02u: %79s", &dev_obj->card_id,
                           &dev_obj->pcm_id, dev_obj->name);
        AGM_LOGD("%d:%d:%s\n", dev_obj->card_id, dev_obj->pcm_id, dev_obj->name);

        /* populate the hw_ep_info for all the available pcm-id's*/
        ret = populate_device_hw_ep_info(dev_obj);
        if (ret) {
           AGM_LOGE("hw_ep_info parsing failed %s\n",
                                dev_obj->name);
           free(dev_obj);
           ret = 0;
           continue;
        }

        pthread_mutex_init(&dev_obj->lock, (const pthread_mutexattr_t *) NULL);
        list_add_tail(&device_list, &dev_obj->list_node);
        count++;
        if (dev_obj->num_virtual_child) {
            dev_obj->group_data = device_get_group_data_by_name(dev_obj->name);

            /* Enumerate virtual backends */
            for (int i = 0; i < dev_obj->num_virtual_child; i++) {
                struct device_obj *child_dev_obj = populate_virtual_device_hw_ep_info(dev_obj, i);
                if (child_dev_obj) {
                    list_add_tail(&device_list, &child_dev_obj->list_node);
                    count++;
                }
                child_dev_obj = NULL;
            }
        }

        dev_obj = NULL;
    }
    /*
     * count 0 indicates that expected sound card was not registered
     * inform the upper layers to try again after sometime.
     */
    if (count == 0) {
        ret = -EAGAIN;
        goto free_device;
    }

    num_audio_intfs = count;
    goto close_file;

free_device:
    list_for_each_safe(dev_node, temp, &device_list) {
        dev_obj = node_to_item(dev_node, struct device_obj, list_node);
        list_remove(dev_node);
        free(dev_obj);
        dev_obj = NULL;
    }

    list_remove(&device_group_data_list);
    list_remove(&device_list);
close_file:
    fclose(fp);
    return ret;
}

static int wait_for_snd_card_to_online()
{
    int ret = 0;
    uint32_t retries = MAX_RETRY;
    int fd = -1;
    char buf[12];
    snd_card_status_t card_status = SND_CARD_STATUS_NONE;

    /* wait here till snd card is registered                               */
    /* maximum wait period = (MAX_RETRY * RETRY_INTERVAL_US) micro-seconds */
    do {
        if ((fd = open(SNDCARD_PATH, O_RDWR)) < 0) {
            AGM_LOGE("Failed to open snd sysfs node, will retry for %d times ...", (retries - 1));
        } else {
            memset(buf , 0 ,sizeof(buf));
            lseek(fd,0L,SEEK_SET);
            read(fd, buf, 1);
            close(fd);
            fd = -1;

            buf[sizeof(buf) - 1] = '\0';
            card_status = SND_CARD_STATUS_NONE;
            sscanf(buf , "%d", &card_status);

            if (card_status == SND_CARD_STATUS_ONLINE) {
                AGM_LOGV("snd sysfs node open successful");
                break;
            }
        }
        retries--;
        sleep(RETRY_INTERVAL);
    } while ( retries > 0);

    if (0 == retries) {
        AGM_LOGE("Failed to open snd sysfs node, exiting ... ");
        ret = -EIO;
    }

    return ret;
}

int device_init()
{
    int ret = 0;

    ret = wait_for_snd_card_to_online();
    if (ret) {
        AGM_LOGE("Not found any SND card online\n");
        return ret;
    }

    ret = parse_snd_card();
    if (ret)
        AGM_LOGE("no valid snd device found\n");

    return ret;
}

void device_deinit()
{
    unsigned int list_count = 0;
    struct device_obj *dev_obj = NULL;
    struct device_group_data *grp_data = NULL;
    struct listnode *dev_node, *grp_node, *temp;

    AGM_LOGE("device deinit called\n");
    list_for_each_safe(dev_node, temp, &device_list) {
        dev_obj = node_to_item(dev_node, struct device_obj, list_node);
        list_remove(dev_node);

        metadata_free(&dev_obj->metadata);

        if (dev_obj->params)
            free(dev_obj->params);

        free(dev_obj);
        dev_obj = NULL;
    }

    list_for_each_safe(grp_node, temp, &device_group_data_list) {
            grp_data = node_to_item(grp_node, struct device_group_data, list_node);
            list_remove(grp_node);
            free(grp_data);
            grp_data = NULL;
    }

    list_remove(&device_group_data_list);
    list_remove(&device_list);

#ifdef DEVICE_USES_ALSALIB
    if (mixer)
        snd_ctl_close(mixer);
#else
    if (mixer)
        mixer_close(mixer);
#endif
}

static void split_snd_card_name(const char * in_snd_card_name, char* file_path_extn,
                                char* file_path_extn_wo_variant)
{
    /* Sound card name follows below mentioned convention:
       <target name>-<form factor>-<variant>-snd-card.
    */
    char *snd_card_name = NULL;
    char *tmp = NULL;
    char *card_sub_str = NULL;

    snd_card_name = strdup(in_snd_card_name);
    if (snd_card_name == NULL) {
        goto done;
    }

    card_sub_str = strtok_r(snd_card_name, "-", &tmp);
    if (card_sub_str == NULL) {
        AGM_LOGE("called on invalid snd card name(%s)", in_snd_card_name);
        goto done;
    }
    strlcat(file_path_extn, card_sub_str, FILE_PATH_EXTN_MAX_SIZE);

    while ((card_sub_str = strtok_r(NULL, "-", &tmp))) {
        if (strncmp(card_sub_str, "snd", strlen("snd"))) {
            strlcpy(file_path_extn_wo_variant, file_path_extn, FILE_PATH_EXTN_MAX_SIZE);
            strlcat(file_path_extn, "_", FILE_PATH_EXTN_MAX_SIZE);
            strlcat(file_path_extn, card_sub_str, FILE_PATH_EXTN_MAX_SIZE);
        }
        else
            break;
    }

done:
    if (snd_card_name)
        free(snd_card_name);
}

static bool check_and_update_snd_card(char *in_snd_card_str)
{
    char *str = NULL, *tmp = NULL;
    int card = 0, card_id = -1;
    bool is_updated = false;
    char *snd_card_str = strdup(in_snd_card_str);

    if (snd_card_str == NULL) {
        return is_updated;
    }

    card = device_get_snd_card_id();
    if (card < 0) {
        goto done;
    }

    str = strtok_r(snd_card_str, "[:] ", &tmp);
    if (str == NULL) {
        goto done;
    }
    card_id = atoi(str);

    str = strtok_r(NULL, "[:] ", &tmp);
    if (str == NULL) {
        goto done;
    }

    if (card_id == card) {
        is_updated = true;
    }

done:
    if (snd_card_str)
        free(snd_card_str);
    return is_updated;
}

static bool update_snd_card_info(char snd_card_name[])
{
    bool is_updated = false;
    char line1[BUF_SIZE] = {0}, line2[BUF_SIZE] = {0};
    FILE *file = NULL;
    int len = 0, retries = MAX_RETRY;
    char *card_name = NULL, *tmp = NULL;

    if (access(SND_CARD_DEVICE_FILE, F_OK) != -1) {
        file = fopen(SND_CARD_DEVICE_FILE, "r");
        if (file == NULL) {
            AGM_LOGE("open %s: failed\n", SND_CARD_DEVICE_FILE);
            goto done;
        }
    } else {
        AGM_LOGE("Unable to access %s\n", SND_CARD_DEVICE_FILE);
        goto done;
    }

    /* Look for only default codec sound card */
    /* Ignore USB sound card if detected */
    /* Example of data read from /proc/asound/cards: */
    /* card-id [dummyidpsndcard]: dummy-idp-variant-snd- - dummy-idp-variant-snd-card */
    /*                       dummy-idp-variant-snd-card */
    do {
        if (!fgets(line1, BUF_SIZE - 1, file)) {
            break;
        }
        len = strnlen(line1, BUF_SIZE);
        line1[len - 1] = '\0';

        if (!fgets(line2, BUF_SIZE - 1, file)) {
            break;
        }
        len = strnlen(line2, BUF_SIZE);
        line2[len - 1] = '\0';

        if (check_and_update_snd_card(line1)) {
            card_name = strtok_r(line2, "[:] ", &tmp);
            if (card_name != NULL) {
                strlcpy(snd_card_name, card_name, FILE_PATH_EXTN_MAX_SIZE);
                is_updated = true;
            }
        }
    } while(!is_updated && --retries);

done:
    if (file)
        fclose(file);
    return is_updated;
}

bool get_file_path_extn(char* file_path_extn, char* file_path_extn_wo_variant)
{
    int snd_card_found = false, retry = 0;
    char snd_card_name[FILE_PATH_EXTN_MAX_SIZE];

    do {
        snd_card_found = update_snd_card_info(snd_card_name);

        if (snd_card_found) {
            split_snd_card_name(snd_card_name, file_path_extn, file_path_extn_wo_variant);
            AGM_LOGV("Found Codec sound card");
            break;
        } else {
            AGM_LOGI("Sound card not found, retry %d", retry++);
            sleep(1);
        }
    } while (!snd_card_found && retry <= MAX_RETRY_CNT);

    return snd_card_found;
}
