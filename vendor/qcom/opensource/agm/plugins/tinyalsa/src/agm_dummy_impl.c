/*
 * Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <agm/agm_api.h>
#include <agm/utils.h>

#define ARRAY_SIZE(a) \
        (sizeof(a) / sizeof(a[0]))

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_DUMMY_IMPL
#include <log_utils.h>
#endif

int agm_session_write(struct session_obj *handle, void *buff, size_t count)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}


int agm_session_read(struct session_obj *handle, void *buff, size_t count)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_stop(struct session_obj *handle)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_start(struct session_obj *handle)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_prepare(struct session_obj *handle)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_close(struct session_obj *handle)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_set_config(struct session_obj *handle,
        struct agm_session_config *session_config,
        struct agm_media_config *media_config,
        struct agm_buffer_config *buffer_config)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_open(uint32_t session_id, enum agm_session_mode sess_mode,
                     struct session_obj **handle)
{
    struct session_obj *h = malloc(sizeof(100));
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    *handle = h;
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}


int agm_audio_intf_set_media_config(uint32_t audio_intf,
                                    struct agm_media_config *media_config)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_audio_intf_set_metadata(uint32_t audio_intf,
                                struct agm_meta_data *metadata)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_set_metadata(uint32_t session_id,
                                struct agm_meta_data *metadata)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_audio_inf_set_metadata(uint32_t session_id,
                                       uint32_t audio_intf,
                                       struct agm_meta_data *metadata)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}

int agm_session_audio_inf_connect(uint32_t session_id,
	uint32_t audio_intf, bool state)
{
    AGM_LOGD("%s: s_id %u, aif_id %u, %s\n", __func__,
                 session_id, audio_intf,
                 state ? "connect" : "disconnect");
    return 0;
}

static struct aif_info be_list[] = {
    {
        .aif_name = "SLIM_0_RX",
        .dir = RX,
    },
    {
        .aif_name = "SLIM_1_RX",
        .dir = RX,
    },
    {
        .aif_name = "SLIM_0_TX",
        .dir = TX,
    },
    {
        .aif_name = "SLIM_1_TX",
        .dir = TX,
    },
};

int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info)
{
    int i;

    AGM_LOGD("%s %d\n", __func__, __LINE__);
    if (*num_aif_info == 0) {
        *num_aif_info = ARRAY_SIZE(be_list);
        return 0;
    }
    for (i = 0; i < *num_aif_info; i++)
        memcpy((aif_list + i), &be_list[i], sizeof(struct aif_info));
    return 0;
}

int agm_session_set_loopback(uint32_t capture_session_id,
                            uint32_t playback_session_id)
{
    AGM_LOGD("%s %d\n", __func__, __LINE__);
    return 0;
}
