/*
** Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
**/

#define LOG_TAG "PLUGIN: AGMCTL"

#include <alsa/asoundlib.h>
#include <alsa/control_external.h>
#include <agm/agm_api.h>
#include <agm/agm_list.h>
#include <snd-card-def.h>
#include "utils.h"

#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(a[0]))
#define MIXER_NAME_LEN  128
#define PCM_NAME_LEN    24

/* enum for different types of control */
enum {
    AGM_FE_CTL_START = 0,
    AGM_FE_CTL_NAME_METADATA = AGM_FE_CTL_START,
    AGM_FE_CTL_NAME_SET_PARAM,
    AGM_FE_CTL_NAME_SET_PARAM_TAG,
    AGM_FE_CTL_NAME_CONNECT,
    AGM_FE_CTL_NAME_DISCONNECT,
    AGM_FE_CTL_NAME_CONTROL,
    AGM_FE_CTL_NAME_GET_TAG_INFO,
    AGM_FE_CTL_NAME_EVENT,
    AGM_FE_CTL_NAME_SET_CALIBRATION,
    AGM_FE_CTL_NAME_GET_PARAM,
    AGM_FE_CTL_NAME_BUF_INFO,
    /* Add new common FE control enum here */
    AGM_FE_CTL_END,
    AGM_TX_CTL_START = AGM_FE_CTL_END,
    AGM_FE_TX_CTL_NAME_LOOPBACK = AGM_TX_CTL_START,
    AGM_FE_TX_CTL_NAME_ECHOREF,
    AGM_FE_TX_CTL_NAME_BUF_TSTAMP,
    /* Add new Tx only FE control enum here */
    AGM_TX_CTL_END,
    AGM_BE_CTL_START = AGM_TX_CTL_END,
    AGM_BE_CTL_NAME_MEDIA_CONFIG = AGM_BE_CTL_START,
    AGM_BE_CTL_NAME_METADATA,
    AGM_BE_CTL_NAME_SET_PARAM,
    /* Add new Backend control enum here */
    AGM_BE_CTL_END,
};

/* struct containing control attibutes for each controls */
/* Any new control entry must be added here */
static const struct agm_ctl_attribute_info {
    int type;
    unsigned int access;
    unsigned int count;
} attr_info[AGM_BE_CTL_END] = {
    /********************** START OF COMMON FE CONTROLS ***********************/
    [AGM_FE_CTL_NAME_METADATA] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                  SND_CTL_EXT_ACCESS_TLV_CALLBACK, 1024},
    [AGM_FE_CTL_NAME_SET_PARAM] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                   SND_CTL_EXT_ACCESS_TLV_CALLBACK, 128 * 1024},
    [AGM_FE_CTL_NAME_SET_PARAM_TAG] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                       SND_CTL_EXT_ACCESS_TLV_CALLBACK, 1024},
    [AGM_FE_CTL_NAME_CONNECT] = {SND_CTL_ELEM_TYPE_ENUMERATED, SND_CTL_EXT_ACCESS_READWRITE, 1},
    [AGM_FE_CTL_NAME_DISCONNECT] = {SND_CTL_ELEM_TYPE_ENUMERATED, SND_CTL_EXT_ACCESS_READWRITE, 1},
    [AGM_FE_CTL_NAME_CONTROL] = {SND_CTL_ELEM_TYPE_ENUMERATED, SND_CTL_EXT_ACCESS_READWRITE, 1},
    [AGM_FE_CTL_NAME_GET_TAG_INFO] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                      SND_CTL_EXT_ACCESS_TLV_CALLBACK, 1024},
    [AGM_FE_CTL_NAME_EVENT] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                               SND_CTL_EXT_ACCESS_TLV_CALLBACK, 512},
    [AGM_FE_CTL_NAME_SET_CALIBRATION] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                         SND_CTL_EXT_ACCESS_TLV_CALLBACK, 512},
    [AGM_FE_CTL_NAME_GET_PARAM] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                   SND_CTL_EXT_ACCESS_TLV_CALLBACK, 128 * 1024},
    [AGM_FE_CTL_NAME_BUF_INFO] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                  SND_CTL_EXT_ACCESS_TLV_CALLBACK, 512},
    /*********************** END OF COMMON FE CONTROLS ************************/
    /********************** START OF TX FE CONTROLS ***********************/
    [AGM_FE_TX_CTL_NAME_LOOPBACK] = {SND_CTL_ELEM_TYPE_ENUMERATED, SND_CTL_EXT_ACCESS_READWRITE, 1},
    [AGM_FE_TX_CTL_NAME_ECHOREF] = {SND_CTL_ELEM_TYPE_ENUMERATED, SND_CTL_EXT_ACCESS_READWRITE, 1},
    [AGM_FE_TX_CTL_NAME_BUF_TSTAMP] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                       SND_CTL_EXT_ACCESS_TLV_CALLBACK, 512},
    /*********************** END OF TX FE CONTROLS ************************/
    /************************** START OF BE CONTROLS **************************/
    [AGM_BE_CTL_NAME_MEDIA_CONFIG] = {SND_CTL_ELEM_TYPE_INTEGER, SND_CTL_EXT_ACCESS_READWRITE, 4},
    [AGM_BE_CTL_NAME_METADATA] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                  SND_CTL_EXT_ACCESS_TLV_CALLBACK, 1024},
    [AGM_BE_CTL_NAME_SET_PARAM] = {SND_CTL_ELEM_TYPE_BYTES, SND_CTL_EXT_ACCESS_TLV_READWRITE |
                                   SND_CTL_EXT_ACCESS_TLV_CALLBACK, 64 * 1024},
    /*************************** END OF BE CONTROLS ***************************/
};

/* control name for common frontend controls*/
static char *agm_fe_ctl_name_extn[] = {
    "metadata",
    "setParam",
    "setParamTag",
    "connect",
    "disconnect",
    "control",
    "getTaggedInfo",
    "event",
    "setCalibration",
    "getParam",
    "getBufInfo"
    /* add new control entry here */
};

/* control name for tx only frontend controls*/
static char *agm_tx_ctl_name_extn[] = {
    "loopback",
    "echoReference",
    "bufTimestamp",
    /* add new control entry here */
};

/* control name for backend controls*/
static char *agm_be_ctl_name_extn[] = {
    "rate ch fmt",
    "metadata",
    "setParam",
    /* add new control entry here */
};

/* struct containing information per control */
struct agm_mixer_controls {
    char mixer_name[MIXER_NAME_LEN + 1];
    int pcm_be_id; /* stores pcm_id for FE controls and AIF id for BE controls */
    uint32_t ctl_id;  //enum for control
    struct agmctl_priv *priv;
};

struct agmctl_priv {
    snd_ctl_ext_t ext;

    int card;
    void *card_node;

    size_t aif_count;
    struct aif_info *aif_list;

    uint32_t rx_count;
    char (*rx_names)[PCM_NAME_LEN + 1];
    int *rx_pcm_id;

    uint32_t pcm_count;
    /*
     * Mixer ctl data cache for
     * "PCM<id> control"
     * Unused for BE devs
     */
    int *pcm_mtd_ctl;
    int *pcm_idx;

    /*
     * Mixer ctl data cache for
     * "PCM<id> getParam"
     * Unused for BE devs
     */
    void *get_param_payload;
    int get_param_payload_size;

    uint32_t total_ctl_cnt;
    struct agm_mixer_controls *controls;
};

static enum agm_media_format alsa_to_agm_fmt(int fmt)
{
    enum agm_media_format agm_pcm_fmt = AGM_FORMAT_INVALID;

    switch (fmt) {
    case SND_PCM_FORMAT_S8:
        agm_pcm_fmt = AGM_FORMAT_PCM_S8;
        break;
    case SND_PCM_FORMAT_S16_LE:
        agm_pcm_fmt = AGM_FORMAT_PCM_S16_LE;
        break;
    case SND_PCM_FORMAT_S24_LE:
        agm_pcm_fmt = AGM_FORMAT_PCM_S24_LE;
        break;
    case SND_PCM_FORMAT_S24_3LE:
        agm_pcm_fmt = AGM_FORMAT_PCM_S24_3LE;
        break;
    case SND_PCM_FORMAT_S32_LE:
        agm_pcm_fmt = AGM_FORMAT_PCM_S32_LE;
        break;
    }

    return agm_pcm_fmt;
}

static int agmctl_pcm_get_control_value(struct agmctl_priv *agmctl, int pcm_idx)
{
    int i;

    for (i = 0; i < agmctl->pcm_count; i++) {
        if (agmctl->pcm_idx[i] == pcm_idx)
            break;
    }
    if (i == agmctl->pcm_count) {
        AGM_LOGD("%s: pcm id %d out of range\n", __func__, pcm_idx);
        return -EINVAL;
    }

    return agmctl->pcm_mtd_ctl[i];
}

static int agmctl_pcm_metadata_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t max_bytes)
{
    int pcm_idx = control->pcm_be_id;
    int ret = 0, be_idx = -1;
    int pcm_control;

    pcm_control = agmctl_pcm_get_control_value(control->priv, pcm_idx);
    if (pcm_control > 0) {
        be_idx = pcm_control - 1;
        ret = agm_session_aif_set_metadata(pcm_idx, be_idx, max_bytes, data);
    } else {
        ret = agm_session_set_metadata(pcm_idx, max_bytes, data);
    }

    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: set_session_metadata failed err %d for %d\n",
               __func__, ret, pcm_idx);
    return ret;
}

static int agmctl_pcm_setparam_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t max_bytes)
{
    int pcm_idx = control->pcm_be_id;
    int ret = 0, be_idx = -1;
    int pcm_control;

    pcm_control = agmctl_pcm_get_control_value(control->priv, pcm_idx);
    if (pcm_control > 0) {
        be_idx = pcm_control - 1;
        ret = agm_session_aif_set_params(pcm_idx, be_idx,
                                         data, max_bytes);
    } else {
        ret = agm_session_set_params(pcm_idx, data, max_bytes);
    }

    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: set_params failed err %d for %d\n",
               __func__, ret, pcm_idx);
    return ret;
}

static int agmctl_pcm_setparamtag_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t max_bytes __unused)
{
    int pcm_idx = control->pcm_be_id;
    int ret = 0, be_idx = -1;
    int pcm_control;

    pcm_control = agmctl_pcm_get_control_value(control->priv, pcm_idx);
    if (pcm_control <= 0) {
        AGM_LOGE("%s: err: control not set to Backend\n", __func__);
        return -EINVAL;
    }

    be_idx = pcm_control - 1;
    ret = agm_set_params_with_tag(pcm_idx, be_idx, (struct agm_tag_config *)data);
    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: set_paramtag failed err %d for %d\n",
               __func__, ret, pcm_idx);
    return ret;
}

static int agmctl_pcm_event_get(struct agm_mixer_controls *control  __unused,
                                   unsigned char *data __unused, size_t len __unused)
{
    /* TODO : Handle event */
    return 0;
}

static int agmctl_pcm_event_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t max_bytes __unused)
{
    int pcm_idx = control->pcm_be_id;
    struct agm_event_reg_cfg *evt_reg_cfg;
    int ret = 0;

    evt_reg_cfg = (struct agm_event_reg_cfg *)data;
    ret = agm_session_register_for_events(pcm_idx, evt_reg_cfg);

    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: set_event failed, err %d, session_id %u\n",
               __func__, ret, pcm_idx);
    return ret;
}

static int agmctl_pcm_calibration_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t max_bytes __unused)
{
    struct agm_cal_config *cal_config = (struct agm_cal_config *)data;
    int pcm_idx = control->pcm_be_id;
    int ret, be_idx = -1;
    int pcm_control;

    pcm_control = agmctl_pcm_get_control_value(control->priv, pcm_idx);
    if (pcm_control <= 0) {
        AGM_LOGE("%s: err: control not set to Backend\n", __func__);
        return -EINVAL;
    }

    be_idx = pcm_control - 1;
    ret = agm_session_aif_set_cal(pcm_idx, be_idx, cal_config);
    if (ret)
        AGM_LOGE("%s: set_calbration failed, err %d, aif_id %u\n",
               __func__, ret, be_idx);
    return ret;
}

static int agmctl_pcm_get_param_get(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len)
{
    void *payload = data;
    int pcm_idx = control->pcm_be_id;
    int ret = 0;

    if (!control->priv->get_param_payload) {
        AGM_LOGE("%s: put() for getParam not called\n", __func__);
        return -EINVAL;
    }

    if (len < control->priv->get_param_payload_size) {
        AGM_LOGE("%s: Buffer size less than expected\n", __func__);
        return -EINVAL;
    }

    memcpy(data, control->priv->get_param_payload, control->priv->get_param_payload_size);
    ret = agm_session_get_params(pcm_idx, payload, control->priv->get_param_payload_size);

    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: failed err %d for %d\n", __func__, ret, pcm_idx);

    free(control->priv->get_param_payload);
    control->priv->get_param_payload = NULL;
    control->priv->get_param_payload_size = 0;
    return ret;
}

static int agmctl_pcm_get_param_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len)
{
    void *payload;

    if (control->priv->get_param_payload) {
        free(control->priv->get_param_payload);
        control->priv->get_param_payload = NULL;
    }
    payload = data;
    control->priv->get_param_payload_size = len;

    control->priv->get_param_payload = calloc(1, control->priv->get_param_payload_size);
    if (!control->priv->get_param_payload)
        return -ENOMEM;

    memcpy(control->priv->get_param_payload, payload, control->priv->get_param_payload_size);

    return 0;
}

static int agmctl_pcm_buf_timestamp_get(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len __unused)
{
    int pcm_idx = control->pcm_be_id;
    int ret;
    uint64_t *timestamp = NULL;

    timestamp = (uint64_t *)data;
    ret = agm_get_buffer_timestamp(pcm_idx, timestamp);
    return ret;
}

static int agmctl_pcm_tag_info_get(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len)
{
    void *payload;
    int pcm_idx = control->pcm_be_id;
    int be_idx = -1, ret = 0;
    size_t get_size = 0;
    int pcm_control;

    pcm_control = agmctl_pcm_get_control_value(control->priv, pcm_idx);
    if (pcm_control <= 0) {
        AGM_LOGE("%s: err: control not set to Backend\n", __func__);
        return -EINVAL;
    }

    be_idx = pcm_control - 1;
    ret = agm_session_aif_get_tag_module_info(pcm_idx, be_idx,
                    NULL, &get_size);
    if (ret || get_size == 0 || len < get_size) {
        AGM_LOGE("%s: invalid size, ret %d, tlv_size %zu, get_size %zu\n",
                __func__, ret, len, get_size);
        return -EINVAL;
    }

    payload = (void *)data;
    ret = agm_session_aif_get_tag_module_info(pcm_idx, be_idx,
                            payload, &get_size);
    if (ret)
        AGM_LOGE("%s: session_aif_get_tag_module_info failed err %d for %d\n",
               __func__, ret, pcm_idx);
    return ret;
}

static int agmctl_pcm_connect_put(struct agm_mixer_controls *control,
                                  unsigned int *item)
{
    bool state = true;
    int be_idx = *item;
    int rc, pcm_idx = control->pcm_be_id;

    if (control->ctl_id == AGM_FE_CTL_NAME_DISCONNECT)
        state = false;

    rc = agm_session_aif_connect(pcm_idx, be_idx, state);
    if (rc == -EALREADY)
        rc = 0;

    if (rc)
        AGM_LOGE("%s: connect failed err %d, pcm_idx %d be_idx %d\n",
                 __func__, rc, pcm_idx, be_idx);

    return rc;
}

static int agmctl_pcm_loopback_put(struct agm_mixer_controls *control,
                                  unsigned int *item)
{
    bool state = true;
    int rx_idx = *item;
    int rc, pcm_idx = control->pcm_be_id;

    if (rx_idx == 0)
        state = false;
    else
        rx_idx = control->priv->rx_pcm_id[rx_idx - 1];

    rc = agm_session_set_loopback(pcm_idx, rx_idx, state);;
    if (rc == -EALREADY)
        rc = 0;

    if (rc)
        AGM_LOGE("%s: loopback failed err %d, rx_idx %d tx_idx %d\n",
                 __func__, rc, rx_idx, pcm_idx);

    return rc;
}

static int agmctl_pcm_ecref_put(struct agm_mixer_controls *control,
                                  unsigned int *item)
{
    bool state = true;
    int be_idx = *item;
    int rc, pcm_idx = control->pcm_be_id;

    if (be_idx == 0)
        state = false;
    else
        be_idx--;

    rc = agm_session_set_ec_ref(pcm_idx, be_idx, state);;
    if (rc == -EALREADY)
        rc = 0;

    if (rc)
        AGM_LOGE("%s: set ecref failed err %d, pcm_idx %d be_idx %d\n",
               __func__, rc, pcm_idx, be_idx);

    return rc;
}

static int agmctl_pcm_control_put(struct agm_mixer_controls *control,
                                  unsigned int *item)
{
    int i, idx = control->pcm_be_id;
    unsigned int val;

    val = *item;
    if (val > control->priv->aif_count) {
        AGM_LOGD("%s: invalid input pcm ctl id %d\n", __func__, val);
        return -EINVAL;
    }

    for (i = 0; i < control->priv->pcm_count; i++) {
        if (control->priv->pcm_idx[i] == idx)
            break;
    }
    if (i == control->priv->pcm_count) {
        AGM_LOGD("%s: pcm id %d out of range\n", __func__, idx);
        return -EINVAL;
    }

    control->priv->pcm_mtd_ctl[i] = val;

    AGM_LOGD("%s: value = %u\n", __func__, val);
    return 0;
}

static int agmctl_pcm_buf_info_get(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len __unused)
{
    struct agm_buf_info *buf_info;
    int pcm_idx = control->pcm_be_id;
    int ret = 0;

    buf_info = (struct agm_buf_info *) data;

    ret = agm_session_get_buf_info(pcm_idx, buf_info, DATA_BUF);
    return ret;
}

static int agmctl_be_metadata_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len)
{
    int be_idx = control->pcm_be_id;
    int ret = 0;

    ret = agm_aif_set_metadata(be_idx, len, data);
    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: set_aif_metadata failed err %d for %d\n",
               __func__, ret, be_idx);
    return ret;
}

static int agmctl_be_setparam_put(struct agm_mixer_controls *control,
                                   unsigned char *data, size_t len)
{
    int be_idx = control->pcm_be_id;
    int ret = 0;

    ret = agm_aif_set_metadata(be_idx, len, data);
    if (ret == -EALREADY)
        ret = 0;

    if (ret)
        AGM_LOGE("%s: set_params failed err %d for %d\n",
               __func__, ret, be_idx);
    return ret;
}

static int agmctl_be_media_config_put(struct agm_mixer_controls *control,
                                      long *val)
{
    struct agm_media_config media_fmt;
    int be_idx = control->pcm_be_id;
    int ret = 0;

    media_fmt.rate = val[0];
    media_fmt.channels = val[1];
    media_fmt.format = alsa_to_agm_fmt(val[2]);
    media_fmt.data_format = val[3];

    ret = agm_aif_set_media_config(be_idx, &media_fmt);
    if (ret)
        AGM_LOGE("%s: set_media_config failed, err %d, aif_id %u rate %u \
                 channels %u fmt %u, data_fmt %u\n",
                 __func__, ret, be_idx, media_fmt.rate, media_fmt.channels,
                 media_fmt.format, media_fmt.data_format);
    return ret;
}

static int agmctl_form_common_fe_controls(struct agmctl_priv *agmctl, void *pcm_node, int *ctl_idx)
{
    int ret, i, pcm_id, ctl_per_pcm;
    char *name;

    ctl_per_pcm = (int)ARRAY_SIZE(agm_fe_ctl_name_extn);

    ret = snd_card_def_get_str(pcm_node, "name", &name);
    if (ret) {
        AGM_LOGE("%s failed to get name\n", __func__);
        return -EINVAL;
    }
    ret = snd_card_def_get_int(pcm_node, "id", &pcm_id);
    if (ret) {
        AGM_LOGE("%s failed to get id for %s\n", __func__, name);
        return -EINVAL;
    }
    for (i = 0; i < ctl_per_pcm; i++) {
        agmctl->controls[*ctl_idx].priv = agmctl;
        agmctl->controls[*ctl_idx].pcm_be_id = pcm_id;
        agmctl->controls[*ctl_idx].ctl_id = AGM_FE_CTL_START + i;
        snprintf(agmctl->controls[*ctl_idx].mixer_name, MIXER_NAME_LEN, "%s %s",
                 name, agm_fe_ctl_name_extn[i]);

        AGM_LOGD("control[%d] is [%s]\n", *ctl_idx, agmctl->controls[*ctl_idx].mixer_name);
        (*ctl_idx)++;
    }

    return 0;
}

static int agmctl_populate_rx_name(struct agmctl_priv *agmctl, void *pcm_node, int *rx_idx)
{
    int ret, pcm_id;
    char *name, *rx_name;

    ret = snd_card_def_get_str(pcm_node, "name", &name);
    if (ret) {
        AGM_LOGE("%s failed to get name\n", __func__);
        return -EINVAL;
    }
    ret = snd_card_def_get_int(pcm_node, "id", &pcm_id);
    if (ret) {
        AGM_LOGE("%s failed to get id for %s\n", __func__, name);
        return -EINVAL;
    }

    agmctl->rx_pcm_id[*rx_idx] = pcm_id;
    rx_name = agmctl->rx_names[*rx_idx];
    strlcpy(rx_name, name, PCM_NAME_LEN);

    (*rx_idx)++;

    return 0;
}

static int agmctl_form_tx_controls(struct agmctl_priv *agmctl, void *pcm_node, int *ctl_idx)
{
    int ret, i, pcm_id, ctl_per_pcm;
    char *name;

    ctl_per_pcm = (int)ARRAY_SIZE(agm_tx_ctl_name_extn);

    ret = snd_card_def_get_str(pcm_node, "name", &name);
    if (ret) {
        AGM_LOGE("%s failed to get name\n", __func__);
        return -EINVAL;
    }

    ret = snd_card_def_get_int(pcm_node, "id", &pcm_id);
    if (ret) {
        AGM_LOGE("%s failed to get id for %s\n", __func__, name);
        return -EINVAL;
    }

    for (i = 0; i < ctl_per_pcm; i++) {
        agmctl->controls[*ctl_idx].priv = agmctl;
        agmctl->controls[*ctl_idx].pcm_be_id = pcm_id;
        agmctl->controls[*ctl_idx].ctl_id = AGM_TX_CTL_START + i;
        snprintf(agmctl->controls[*ctl_idx].mixer_name, MIXER_NAME_LEN, "%s %s",
                 name, agm_tx_ctl_name_extn[i]);

        AGM_LOGD("control[%d] is [%s]\n", *ctl_idx, agmctl->controls[*ctl_idx].mixer_name);
        (*ctl_idx)++;
    }

    return 0;
}

static int agmctl_form_be_controls(struct agmctl_priv *agmctl, int be_idx, int *ctl_idx)
{
    int i, ctl_per_be;

    ctl_per_be = (int)ARRAY_SIZE(agm_be_ctl_name_extn);

    for (i = 0; i < ctl_per_be; i++) {
        agmctl->controls[*ctl_idx].priv = agmctl;
        agmctl->controls[*ctl_idx].pcm_be_id = be_idx;
        agmctl->controls[*ctl_idx].ctl_id = AGM_BE_CTL_START + i;
        snprintf(agmctl->controls[*ctl_idx].mixer_name, MIXER_NAME_LEN, "%s %s",
                 agmctl->aif_list[be_idx].aif_name, agm_be_ctl_name_extn[i]);

        AGM_LOGD("control[%d] is [%s]\n", *ctl_idx, agmctl->controls[*ctl_idx].mixer_name);
        (*ctl_idx)++;
    }

    return 0;
}

static int amp_form_mixer_controls(struct agmctl_priv *agmctl, int *ctl_idx)
{
    void **pcm_node_list = NULL;
    int num_pcms = 0, num_compr = 0;
    int total_pcms, total_tx = 0, rx_idx = 0;
    int ret, val = 0, i;
    int ctl_per_pcm;

    /* Get both pcm and compressed node count */
    num_pcms = snd_card_def_get_num_node(agmctl->card_node,
                                         SND_NODE_TYPE_PCM);
    num_compr = snd_card_def_get_num_node(agmctl->card_node,
                                          SND_NODE_TYPE_COMPR);
    if (num_pcms <= 0 && num_compr <= 0) {
        AGM_LOGE("%s: no pcms(%d)/compr(%d) nodes found for card %u\n",
               __func__, num_pcms, num_compr, agmctl->card);
        ret = -EINVAL;
        goto done;
    }

    /* It is valid that any card could have just PCMs or just comprs or both */
    num_pcms = (num_pcms < 0) ? 0 : num_pcms;
    num_compr = (num_compr < 0) ? 0 : num_compr;

    total_pcms = num_pcms + num_compr;
    ctl_per_pcm = (int)ARRAY_SIZE(agm_fe_ctl_name_extn);
    agmctl->total_ctl_cnt = total_pcms * ctl_per_pcm;

    pcm_node_list = calloc(total_pcms, sizeof(*pcm_node_list));
    if (!pcm_node_list) {
        AGM_LOGE("%s: alloc for pcm_node_list failed\n", __func__);
        return -ENOMEM;
    }

    agmctl->pcm_count = total_pcms;
    agmctl->pcm_mtd_ctl = calloc(total_pcms, sizeof(int));
    if (!agmctl->pcm_mtd_ctl) {
        ret = -ENOMEM;
        goto done;
    }

    agmctl->pcm_idx = calloc(total_pcms, sizeof(int));
    if (!agmctl->pcm_idx) {
        ret = -ENOMEM;
        goto done;
    }

    if (num_pcms > 0) {
        ret = snd_card_def_get_nodes_for_type(agmctl->card_node,
                                              SND_NODE_TYPE_PCM,
                                              pcm_node_list, num_pcms);
        if (ret) {
            AGM_LOGE("%s: failed to get pcm node list, err %d\n",
                   __func__, ret);
            goto done;
        }
    }

    if (num_compr > 0) {
        ret = snd_card_def_get_nodes_for_type(agmctl->card_node,
                                              SND_NODE_TYPE_COMPR,
                                              &pcm_node_list[num_pcms],
                                              num_compr);
        if (ret) {
            AGM_LOGE("%s: failed to get compr node list, err %d\n",
                   __func__, ret);
            goto done;
        }
    }

    /* count TX and RX PCMs + Comprs*/
    for (i = 0; i < total_pcms; i++) {
        void *pcm_node = pcm_node_list[i];

        snd_card_def_get_int(pcm_node, "id", &agmctl->pcm_idx[i]);

        val = 0;
        snd_card_def_get_int(pcm_node, "playback", &val);
        if (val == 1)
            agmctl->rx_count++;

        val = 0;
        snd_card_def_get_int(pcm_node, "capture", &val);
        if (val == 1)
            total_tx++;
    }

    ctl_per_pcm = (int)ARRAY_SIZE(agm_tx_ctl_name_extn);
    agmctl->total_ctl_cnt += total_tx * ctl_per_pcm;
    agmctl->total_ctl_cnt +=  agmctl->aif_count * ARRAY_SIZE(agm_be_ctl_name_extn);

    AGM_LOGD("Total virtual controls:%d", agmctl->total_ctl_cnt);
    agmctl->controls = calloc(agmctl->total_ctl_cnt, sizeof(struct agm_mixer_controls));
    if (!agmctl->controls) {
        ret = -ENOMEM;
        goto done;
    }

    agmctl->rx_names = calloc(agmctl->rx_count, sizeof(*agmctl->rx_names));
    if (!agmctl->rx_names) {
        ret = -ENOMEM;
        goto done;
    }

    agmctl->rx_pcm_id = calloc(agmctl->rx_count, sizeof(*agmctl->rx_pcm_id));
    if (!agmctl->rx_pcm_id) {
        ret = -ENOMEM;
        goto done;
    }

    for (i = 0; i < total_pcms; i++) {
        void *pcm_node = pcm_node_list[i];

        agmctl_form_common_fe_controls(agmctl, pcm_node, ctl_idx);

        val = 0;
        snd_card_def_get_int(pcm_node, "playback", &val);
        if (val == 1)
            agmctl_populate_rx_name(agmctl, pcm_node, &rx_idx);

        val = 0;
        snd_card_def_get_int(pcm_node, "capture", &val);
        if (val == 1)
            agmctl_form_tx_controls(agmctl, pcm_node, ctl_idx);
    }

    for (i = 0; i < agmctl->aif_count; i++) {
        agmctl_form_be_controls(agmctl, i, ctl_idx);
    }

done:
    if (pcm_node_list)
        free(pcm_node_list);

    if (ret) {
        if (agmctl->pcm_mtd_ctl)
            free(agmctl->pcm_mtd_ctl);

        if (agmctl->pcm_idx)
            free(agmctl->pcm_idx);

        if (agmctl->controls)
            free(agmctl->controls);

        if (agmctl->rx_names)
            free(agmctl->rx_names);

        if (agmctl->rx_pcm_id)
            free(agmctl->rx_pcm_id);
    }
    return ret;
}

static int agmctl_elem_count(snd_ctl_ext_t * ext)
{
    struct agmctl_priv *agmctl = ext->private_data;

    return agmctl->total_ctl_cnt;
}

static int agmctl_elem_list(snd_ctl_ext_t * ext, unsigned int offset,
                           snd_ctl_elem_id_t * id)
{
    struct agmctl_priv *agmctl = ext->private_data;

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

    if (offset < agmctl->total_ctl_cnt)
        snd_ctl_elem_id_set_name(id, agmctl->controls[offset].mixer_name);

    return 0;
}

static snd_ctl_ext_key_t agmctl_find_elem(snd_ctl_ext_t * ext,
                                         const snd_ctl_elem_id_t * id)
{
    const char *name;
    unsigned int numid, i;
    struct agmctl_priv *agmctl = ext->private_data;

    numid = snd_ctl_elem_id_get_numid(id);
    if (numid > 0 && numid < agmctl->total_ctl_cnt)
            return numid - 1;

    name = snd_ctl_elem_id_get_name(id);

    for (i = 0; i < agmctl->total_ctl_cnt; i++) {
        if (strcmp(name, agmctl->controls[i].mixer_name) == 0) {
            snd_ctl_elem_id_set_numid((snd_ctl_elem_id_t *)id, i + 1);
            return i;
        }
    }

    return SND_CTL_EXT_KEY_NOT_FOUND;
}

static int agmctl_get_attribute(snd_ctl_ext_t * ext, snd_ctl_ext_key_t key,
                               int *type, unsigned int *access,
                               unsigned int *count)
{
    struct agmctl_priv *agmctl = ext->private_data;
    int ctl_id;

    if (key >= agmctl->total_ctl_cnt) {
        AGM_LOGE("Invalid key %lu\n", key);
        return -EINVAL;
    }

    ctl_id = agmctl->controls[key].ctl_id;

    *type = attr_info[ctl_id].type;
    *access = attr_info[ctl_id].access;
    *count = attr_info[ctl_id].count;
    return 0;
}

static int agmctl_get_integer_info(snd_ctl_ext_t * ext,
                                  snd_ctl_ext_key_t key, long *imin,
                                  long *imax, long *istep)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_BE_CTL_NAME_MEDIA_CONFIG:
        *istep = 1;
        *imin = 0;
        *imax = 384000;
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }
    return rc;
}

static int agmctl_get_enumerated_info(snd_ctl_ext_t *ext,
                                        snd_ctl_ext_key_t key,
                                        unsigned int *items)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_FE_CTL_NAME_CONNECT:
    case AGM_FE_CTL_NAME_DISCONNECT:
        *items = agmctl->aif_count;
        break;
    case AGM_FE_CTL_NAME_CONTROL:
    case AGM_FE_TX_CTL_NAME_ECHOREF:
        *items = agmctl->aif_count + 1;
        break;
    case AGM_FE_TX_CTL_NAME_LOOPBACK:
        *items = agmctl->rx_count + 1;
        break;
    default:
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        rc = -EINVAL;
        break;
    }

    return rc;
}

static int agmctl_get_enumerated_name(snd_ctl_ext_t *ext,
                                        snd_ctl_ext_key_t key,
                                        unsigned int item, char *name,
                                        size_t name_max_len)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_FE_CTL_NAME_CONNECT:
    case AGM_FE_CTL_NAME_DISCONNECT:
        if (item >= agmctl->aif_count)
            break;

        strlcpy(name, agmctl->aif_list[item].aif_name, name_max_len);
        break;
    case AGM_FE_CTL_NAME_CONTROL:
    case AGM_FE_TX_CTL_NAME_ECHOREF:
        if (item > agmctl->aif_count)
            break;
        if (item == 0)
            strlcpy(name, "ZERO", name_max_len);
        else
            strlcpy(name, agmctl->aif_list[item-1].aif_name, name_max_len);

        break;
    case AGM_FE_TX_CTL_NAME_LOOPBACK:
        if (item >= agmctl->rx_count)
            break;

        if (item == 0)
            strlcpy(name, "ZERO", name_max_len);
        else
            strlcpy(name, agmctl->rx_names[item - 1], name_max_len);

        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }

    return rc;
}

static int agmctl_read_enumerated(snd_ctl_ext_t *ext,
                                    snd_ctl_ext_key_t key,
                                    unsigned int *item __unused)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_FE_CTL_NAME_CONNECT:
    case AGM_FE_CTL_NAME_DISCONNECT:
    case AGM_FE_CTL_NAME_CONTROL:
    case AGM_FE_TX_CTL_NAME_LOOPBACK:
    case AGM_FE_TX_CTL_NAME_ECHOREF:
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }

    return rc;
}

static int agmctl_write_enumerated(snd_ctl_ext_t *ext,
                  snd_ctl_ext_key_t key, unsigned int *item)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_FE_CTL_NAME_CONNECT:
    case AGM_FE_CTL_NAME_DISCONNECT:
        rc = agmctl_pcm_connect_put(&agmctl->controls[key], item);
        break;
    case AGM_FE_TX_CTL_NAME_LOOPBACK:
        rc = agmctl_pcm_loopback_put(&agmctl->controls[key], item);
        break;
    case AGM_FE_TX_CTL_NAME_ECHOREF:
        rc = agmctl_pcm_ecref_put(&agmctl->controls[key], item);
        break;
    case AGM_FE_CTL_NAME_CONTROL:
        rc = agmctl_pcm_control_put(&agmctl->controls[key], item);
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }

    return rc;
}

static int agmctl_read_integer(snd_ctl_ext_t * ext, snd_ctl_ext_key_t key,
                              long *value __unused)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_BE_CTL_NAME_MEDIA_CONFIG:
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }
    return rc;
}

static int agmctl_write_integer(snd_ctl_ext_t * ext, snd_ctl_ext_key_t key,
                               long *value)
{
    int rc = 0;
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    switch (agmctl->controls[key].ctl_id) {
    case AGM_BE_CTL_NAME_MEDIA_CONFIG:
        rc = agmctl_be_media_config_put(&agmctl->controls[key], value);
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }
    return rc;

}

static int agmctl_read_bytes(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key,
                             unsigned char *tlv, size_t tlv_size)
{
    struct agmctl_priv *agmctl = ext->private_data;
    int rc = 0;
    unsigned char *data;
    size_t len;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    len = (size_t)*(tlv + sizeof(unsigned int));
    data = (unsigned char *)(tlv + 2 * sizeof(unsigned int));

    switch (agmctl->controls[key].ctl_id) {
    case AGM_FE_CTL_NAME_GET_TAG_INFO:
        rc = agmctl_pcm_tag_info_get(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_TX_CTL_NAME_BUF_TSTAMP:
        rc = agmctl_pcm_buf_timestamp_get(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_BUF_INFO:
        rc = agmctl_pcm_buf_info_get(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_GET_PARAM:
        rc = agmctl_pcm_get_param_get(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_EVENT:
        rc = agmctl_pcm_event_get(&agmctl->controls[key], data, len);
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }
    return rc;
}

static int agmctl_write_bytes(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key,
                              unsigned char *tlv, size_t tlv_size)
{
    struct agmctl_priv *agmctl = ext->private_data;
    int rc = 0;
    unsigned char *data;
    size_t len;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    len = (size_t)*(tlv + sizeof(unsigned int));
    data = (unsigned char *)(tlv + 2 * sizeof(unsigned int));

    switch (agmctl->controls[key].ctl_id) {
    case AGM_FE_CTL_NAME_METADATA:
        rc = agmctl_pcm_metadata_put(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_SET_PARAM:
        rc = agmctl_pcm_setparam_put(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_SET_PARAM_TAG:
        rc = agmctl_pcm_setparamtag_put(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_EVENT:
        rc = agmctl_pcm_event_put(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_SET_CALIBRATION:
        rc = agmctl_pcm_calibration_put(&agmctl->controls[key], data, len);
        break;
    case AGM_FE_CTL_NAME_GET_PARAM:
        rc = agmctl_pcm_get_param_put(&agmctl->controls[key], data, len);
        break;
    case AGM_BE_CTL_NAME_METADATA:
        rc = agmctl_be_metadata_put(&agmctl->controls[key], data, len);
        break;
    case AGM_BE_CTL_NAME_SET_PARAM:
        rc = agmctl_be_setparam_put(&agmctl->controls[key], data, len);
        break;
    default:
        rc = -EINVAL;
        AGM_LOGE("Unsupported control %d\n", agmctl->controls[key].ctl_id);
        break;
    }
    return rc;
}

int agm_ext_tlv_callback(snd_ctl_ext_t *ext, snd_ctl_ext_key_t key,
                         int op_flag, unsigned int numid __unused,
                         unsigned int *tlv, unsigned int tlv_size)
{
    struct agmctl_priv *agmctl = ext->private_data;

    if (key >= agmctl->total_ctl_cnt)
        return -EINVAL;

    if (op_flag == 0)
        return agmctl_read_bytes(ext, key, (unsigned char *)tlv, (size_t)tlv_size);
    else if (op_flag == 1)
        return agmctl_write_bytes(ext, key, (unsigned char *)tlv, (size_t)tlv_size);
    else
        return -EINVAL;
}

static void agmctl_close(snd_ctl_ext_t * ext)
{
    struct agmctl_priv *agmctl = ext->private_data;

    snd_card_def_put_card(agmctl->card_node);
    free(agmctl->aif_list);
    free(agmctl);
}

static const snd_ctl_ext_callback_t agm_ext_callback = {
        .elem_count = agmctl_elem_count,
        .elem_list = agmctl_elem_list,
        .find_elem = agmctl_find_elem,
        .get_attribute = agmctl_get_attribute,
        .get_integer_info = agmctl_get_integer_info,
        .get_enumerated_info = agmctl_get_enumerated_info,
        .get_enumerated_name = agmctl_get_enumerated_name,
        .read_enumerated = agmctl_read_enumerated,
        .read_integer = agmctl_read_integer,
        .write_enumerated = agmctl_write_enumerated,
        .write_integer = agmctl_write_integer,
        .read_bytes = agmctl_read_bytes,
        .write_bytes = agmctl_write_bytes,
        .close = agmctl_close,
};

SND_CTL_PLUGIN_DEFINE_FUNC(agm)
{
    snd_config_iterator_t it, next;
    struct agmctl_priv *ctl = NULL;
    long card = 0;
    int rc = 0, ctl_idx = 0;
    size_t aif_count = 0;
    struct aif_info *aif_list = NULL;

    ctl = calloc(1, sizeof(*ctl));
    if (!ctl)
        return -ENOMEM;

    snd_config_for_each(it, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(it);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0 || strcmp(id, "hint") == 0)
            continue;
        if (strcmp(id, "card") == 0) {
            if (snd_config_get_integer(n, &card) < 0) {
                    AGM_LOGE("Invalid type for %s", id);
                    rc = -EINVAL;
                    goto err_free_ctl;
            }
            ctl->card = card;
            continue;
        }
        AGM_LOGE("Unknown field %s", id);
        rc = -EINVAL;
        goto err_free_ctl;
    }

    rc = agm_get_aif_info_list(NULL, &aif_count);
    if (rc || aif_count == 0)
        goto err_free_ctl;

    aif_list = calloc(aif_count, sizeof(struct aif_info));
    if (!aif_list)
        goto err_free_ctl;

    rc = agm_get_aif_info_list(aif_list, &aif_count);
    if (rc)
        goto err_free_aif_list;

    ctl->aif_count = aif_count;
    ctl->aif_list = aif_list;

    ctl->card_node = snd_card_def_get_card(card);
    if (!ctl->card_node) {
        AGM_LOGE("%s: card node not found for card %ld\n", __func__, card);
        rc = -EINVAL;
        goto err_free_aif_list;
    }

    rc = amp_form_mixer_controls(ctl, &ctl_idx);
    if (rc)
        goto err_put_card;

    ctl->ext.version = SND_CTL_EXT_VERSION;
    ctl->ext.card_idx = 0;
    strlcpy(ctl->ext.id, "agm", sizeof(ctl->ext.id));
    strlcpy(ctl->ext.driver, "AGM control plugin",
            sizeof(ctl->ext.driver));
    strlcpy(ctl->ext.name, "agm", sizeof(ctl->ext.name));
    strlcpy(ctl->ext.longname, "AudioGraphManager",
            sizeof(ctl->ext.longname));
    strlcpy(ctl->ext.mixername, "agm",
            sizeof(ctl->ext.mixername));
    ctl->ext.poll_fd = -1;

    ctl->ext.callback = &agm_ext_callback;
    ctl->ext.tlv.c = &agm_ext_tlv_callback;
    ctl->ext.private_data = ctl;

    rc = snd_ctl_ext_create(&ctl->ext, name, mode);
    if (rc < 0)
            goto err_put_card;

    AGM_LOGD("%s: exit", __func__);
    *handlep = ctl->ext.handle;

    return 0;

err_put_card:
    snd_card_def_put_card(ctl->card_node);
err_free_aif_list:
    free(aif_list);
err_free_ctl:
    free(ctl);
    return rc;
}
SND_CTL_PLUGIN_SYMBOL(agm);
