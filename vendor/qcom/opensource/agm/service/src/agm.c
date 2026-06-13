/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#define LOG_TAG "AGM: API"
#include <agm/agm_api.h>
#include <agm/device.h>
#include <agm/session_obj.h>
#include <agm/utils.h>
#include <agm/agm_memlogger.h>
#include "ats.h"
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_SRC
#include <log_utils.h>
#endif

// { SEC_AUDIO_COMMON
#include <system/audiofw_feature.h>
#include <cutils/properties.h>
// } SEC_AUDIO_COMMON

#define RETRY_INTERVAL_US 500 * 1000
static bool agm_initialized = 0;
static pthread_t ats_thread;
static const int MAX_RETRIES = 120;

static void *ats_init_thread(void *obj __unused)
{
    int ret = 0;
    int retry = 0;

    while(retry++ < MAX_RETRIES) {
        ret = ats_init();
        if (0 != ret) {
            AGM_LOGE("ats_init failed retry %d err %d", retry, ret);
            usleep(RETRY_INTERVAL_US);
        } else {
            AGM_LOGD("ATS initialized");
            break;
        }
    }
    return NULL;
}

int agm_init()
{
#ifdef SEC_AUDIO_BOOT_ON_ERR
    AGM_LOGD("Enter. agm_initialized %d ", agm_initialized);
#endif

    int ret = 0;

#ifdef SEC_FACTORY_INTERPOSER
    AGM_LOGE("use default primary hal on secondary bin(not support audio)");
    property_set("vendor.audio.use.primary.default", "true");
    goto exit;
#endif
#ifdef SEC_AUDIO_BOOT_ON_ERR
    if (property_get_bool("vendor.audio.use.primary.default", false)) {
        AGM_LOGE("fail to init agm, sndcard is not active");
        goto exit;
    }
#endif

    if (agm_initialized)
        goto exit;

    pthread_attr_t tattr;
    struct sched_param param;

#ifdef DYNAMIC_LOG_ENABLED
    register_for_dynamic_logging("agm");
    log_utils_init();
#endif

    agm_memlog_init();
    pthread_attr_init (&tattr);
    pthread_attr_getschedparam (&tattr, &param);
    param.sched_priority = SCHED_FIFO;
    pthread_attr_setschedparam (&tattr, &param);

    ret = session_obj_init();
    if (0 != ret) {
        AGM_LOGE("Session_obj_init failed with %d", ret);

#ifdef SEC_AUDIO_BOOT_ON_ERR
        if (ret == -EAGAIN || ret == -EIO) {
            property_set("vendor.audio.use.primary.default", "true");
            AGM_LOGE("sound card err, vendor.audio.use.primary.default as true");
            ret = 0;
        }
#endif

        goto exit;
    }

    agm_initialized = 1;

    ret = pthread_create(&ats_thread, (const pthread_attr_t *) &tattr,
                                           ats_init_thread, NULL);
    if (ret)
        AGM_LOGE(" ats init thread creation failed\n");

exit:
#ifdef SEC_AUDIO_BOOT_ON_ERR
    AGM_LOGD("Exit. agm_initialized %d ", agm_initialized);
#endif
    return ret;
}

int agm_deinit()
{
    //close all sessions first
    if (agm_initialized) {
        AGM_LOGD("Deinitializing ATS...");
        ats_deinit();
        session_obj_deinit();
        agm_memlog_deinit();
        agm_initialized = 0;
    }

    return 0;
}

int agm_get_aif_info_list(struct aif_info *aif_list, size_t *num_aif_info)
{
    if (!num_aif_info || ((*num_aif_info != 0) && !aif_list)) {
        AGM_LOGE("Error Invalid params\n");
        return -EINVAL;
    }

    return device_get_aif_info_list(aif_list, num_aif_info);
}

int agm_get_group_aif_info_list(struct aif_info *aif_list, size_t *num_groups)
{
    if (!num_groups || ((*num_groups != 0) && !aif_list)) {
        AGM_LOGE("Error Invalid params\n");
        return -EINVAL;
    }

    return device_get_group_list(aif_list, num_groups);
}

int agm_aif_set_metadata(uint32_t aif_id, uint32_t size, uint8_t *metadata)
{
    struct device_obj *obj = NULL;
    int32_t ret = 0;

    ret = device_get_obj(aif_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving device obj with audio_intf id=%d\n",
                                         ret, aif_id);
        goto done;
    }

    ret = device_set_metadata(obj, size, metadata);
    if (ret) {
        AGM_LOGE("Error:%d setting metadata device obj with"
                                  "audio_intf id=%d\n", ret, aif_id);
        goto done;
    }

done:
    return ret;
}

int agm_aif_set_media_config(uint32_t aif_id,
                  struct agm_media_config *media_config)
{
    struct device_obj *obj = NULL;
    int ret = 0;

    ret = device_get_obj(aif_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d, retrieving device obj with audio_intf id=%d\n",
                                                        ret, aif_id);
        goto done;
    }

    ret = device_set_media_config(obj, media_config);
    if (ret) {
        AGM_LOGE("Error:%d setting mediaconfig device obj \
                              with audio_intf id=%d\n", ret, aif_id);
        goto done;
    }

done:
    return ret;
}

int agm_aif_group_set_media_config(uint32_t aif_group_id,
                  struct agm_group_media_config *media_config)
{
    struct device_group_data *grp_data = NULL;
    int ret = 0;

    ret = device_get_group_data(aif_group_id, &grp_data);
    if (ret) {
        AGM_LOGE("Error:%d, retrieving device obj with audio_intf id=%d\n",
                                                        ret, aif_group_id);
        goto done;
    }

    ret = device_group_set_media_config(grp_data, media_config);
    if (ret) {
        AGM_LOGE("Error:%d setting mediaconfig for device group \
                              with group id=%d\n", ret, aif_group_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_set_metadata(uint32_t session_id,
                      uint32_t size, uint8_t *metadata)
{

    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_set_sess_metadata(obj, size, metadata);
    if (ret) {
        AGM_LOGE("Error:%d setting metadata for session obj with \
                               session id=%d\n", ret, session_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_aif_set_metadata(uint32_t session_id,
                    uint32_t aif_id,
                    uint32_t size, uint8_t *metadata)
{

    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_set_sess_aif_metadata(obj, aif_id, size, metadata);
    if (ret) {
        AGM_LOGE("Error:%d setting metadata for session obj \
          with session id=%d, aif_id=%d\n", ret, session_id, aif_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_aif_get_tag_module_info(uint32_t session_id,
                                 uint32_t aif_id, void *payload, size_t *size)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_get_tag_with_module_info(obj, aif_id, payload, size);
    if (ret) {
        AGM_LOGE("Error:%d setting parameters for session obj with \
                      session id=%d, aif_id=%d\n",
                      ret, session_id, aif_id);
        goto done;
    }

done:
    return ret;
}

int agm_get_params_from_acdb_tunnel(void *payload, size_t *size)
{
    int ret = 0;
    struct agm_acdb_tunnel_param *payloadACDBTunnelInfo = NULL;
    uint32_t k = 0;
    uint32_t *ptr = NULL;
    uint32_t tag = 0;
    struct agm_key_vector_gsl gkv = {0, NULL};

    AGM_LOGD("enter\n");

    if (!payload) {
        AGM_LOGE("payload is nullptr");
        return -EINVAL;
    }

    payloadACDBTunnelInfo = (struct agm_acdb_tunnel_param *)payload;
    AGM_LOGD("payload size is 0x%x", *size);
    AGM_LOGD("tag=%x istkv=%x num_gkvs=0x%x num_kvs=0x%x blob_size=0x%x",
        payloadACDBTunnelInfo->tag,
        payloadACDBTunnelInfo->isTKV,
        payloadACDBTunnelInfo->num_gkvs,
        payloadACDBTunnelInfo->num_kvs,
        payloadACDBTunnelInfo->blob_size);

    ptr = payloadACDBTunnelInfo->blob;
    for (k = 0; k < payloadACDBTunnelInfo->blob_size / 4; k++) {
        AGM_LOGV("%d data = 0x%x", k, *ptr++);
    }

    ptr = payloadACDBTunnelInfo->blob + sizeof(struct agm_key_value) *
            (payloadACDBTunnelInfo->num_gkvs + payloadACDBTunnelInfo->num_kvs);
    // tag is stored at miid. Convertion happens next.
    AGM_LOGI("tag = 0x%x", *ptr);

    gkv.num_kvs = payloadACDBTunnelInfo->num_gkvs;
    gkv.kv = payloadACDBTunnelInfo->blob;

    ret = session_dummy_rw_acdb_tunnel(payload, FALSE);
    if (ret) {
         AGM_LOGE("Error get tag list");
         goto error;
    }

error:
    return ret;
}

int agm_session_aif_set_cal(uint32_t session_id,
                 uint32_t aif_id,
                 struct agm_cal_config *cal_config)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_set_sess_aif_cal(obj, aif_id, cal_config);
    if (ret) {
        AGM_LOGE("Error:%d setting calibration for session obj \
                   with session id=%d, aif_id=%d\n",
                   ret, session_id, aif_id);
goto done;
}

done:
return ret;
}

/* This does not support runtime update of device param  payload */
int agm_aif_set_params(uint32_t aif_id,
                        void* payload, size_t size)
{
    struct device_obj *obj = NULL;
    int32_t ret = 0;

    ret = device_get_obj(aif_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving device obj with audio_intf id=%d\n",
                                         ret, aif_id);
        goto done;
    }

    ret = device_set_params(obj, payload, size);
    if (ret) {
        AGM_LOGE("Error:%d set params for aif_id=%d\n",
                        ret, aif_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_aif_set_params(uint32_t session_id,
                        uint32_t aif_id,
                        void* payload, size_t size)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with \
                        session id=%d\n", ret, session_id);
        goto done;
    }

    ret = session_obj_set_sess_aif_params(obj, aif_id, payload, size);
    if (ret) {
        AGM_LOGE("Error:%d setting parameters for session obj with \
                                          session id=%d, aif_id=%d\n",
                                        ret, session_id, aif_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_get_params(uint32_t session_id,
        void* payload, size_t size)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
            AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                    ret, session_id);
            goto done;
    }

    ret = session_obj_get_sess_params(obj, payload, size);
    if (ret) {
            AGM_LOGE("Error:%d getting parameters for session obj with"
                         "session id=%d\n",ret, session_id);
            goto done;
    }

done:
    return ret;
}

int agm_session_set_params(uint32_t session_id,
                         void* payload, size_t size)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_set_sess_params(obj, payload, size);
    if (ret) {
        AGM_LOGE("Error:%d setting parameters for session obj with \
                               session id=%d\n", ret, session_id);
    goto done;
}

done:
    return ret;
}

int agm_set_params_with_tag(uint32_t session_id, uint32_t aif_id,
                               struct agm_tag_config *tag_config)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_set_sess_aif_params_with_tag(obj, aif_id, tag_config);
    if (ret) {
        AGM_LOGE("Error:%d setting parameters for session obj with \
                           session id=%d\n", ret, session_id);
        goto done;
    }

done:
    return ret;
}

int agm_set_params_with_tag_to_acdb(uint32_t session_id, uint32_t aif_id,
                                       void *payload, size_t size)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_rw_acdb_params_with_tag(obj, aif_id,
                (struct agm_acdb_param *)payload, true);
    if (ret) {
        AGM_LOGE("Error:%d setting parameters for session obj with \
                           session id=%d\n", ret, session_id);
        goto done;
    }

done:
    return ret;
}

int agm_set_params_to_acdb_tunnel(void *payload, size_t size)
{
    int ret = 0;
    struct agm_acdb_tunnel_param *payloadACDBTunnelInfo = NULL;
    uint32_t k = 0;
    uint32_t *ptr = NULL;
    uint32_t tag = 0;

    AGM_LOGD("enter\n");

    if (!payload) {
        AGM_LOGE("payload is nullptr");
        return -EINVAL;
    }

    payloadACDBTunnelInfo = (struct agm_acdb_tunnel_param *)payload;
    AGM_LOGD("payload size is 0x%x", size);
    AGM_LOGD("tag=%x istkv=%x num_gkvs=0x%x num_kvs=0x%x blob_size=0x%x",
        payloadACDBTunnelInfo->tag,
        payloadACDBTunnelInfo->isTKV,
        payloadACDBTunnelInfo->num_gkvs,
        payloadACDBTunnelInfo->num_kvs,
        payloadACDBTunnelInfo->blob_size);

    ptr = payloadACDBTunnelInfo->blob;
    for (k = 0; k < payloadACDBTunnelInfo->blob_size / 4; k++) {
        AGM_LOGV("%d data = 0x%x", k, *ptr++);
    }

    ptr = payloadACDBTunnelInfo->blob + sizeof(struct agm_key_value) *
            (payloadACDBTunnelInfo->num_gkvs + payloadACDBTunnelInfo->num_kvs);
    // tag is stored at miid. Convertion happens next.
    AGM_LOGI("tag = 0x%x", *ptr);

    ret = session_dummy_rw_acdb_tunnel(payload, TRUE);
    if (ret) {
         AGM_LOGE("Error get tag list");
         goto error;
    }

error:
    return ret;
}

int agm_session_register_cb(uint32_t session_id, agm_event_cb cb,
                            enum event_type evt_type, void *client_data)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_register_cb(obj, cb, evt_type, client_data);
    if (ret) {
        AGM_LOGE("Error:%d registering callback for session obj with \
                               session id=%d\n", ret, session_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_register_for_events(uint32_t session_id,
                            struct agm_event_reg_cfg *evt_reg_cfg)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    if (!evt_reg_cfg) {
        AGM_LOGE("Invalid ev_reg_cfg for session id=%d\n",
                                        session_id);
        ret = -EINVAL;
        goto done;
    }

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_register_for_events(obj, evt_reg_cfg);
    if (ret) {
        AGM_LOGE("Error:%d registering event for session obj with \
                        session id=%d\n", ret, session_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_aif_connect(uint32_t session_id,
                            uint32_t aif_id,
                            bool state)
{

    struct session_obj *obj = NULL;
    int ret = 0;

    AGM_LOGI("%sconnecting aifid:%d with session id=%d\n",
                                      (state ? "": "dis"), aif_id, session_id);

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        goto done;
    }

    ret = session_obj_sess_aif_connect(obj, aif_id, state);
    if (ret) {
        AGM_LOGE("Error:%d Connecting aifid:%d with session id=%d\n",
                                      ret, aif_id, session_id);
        goto done;
    }

done:
return ret;
}

int agm_session_open(uint32_t session_id,
                     enum agm_session_mode sess_mode,
                     uint64_t *hndl)
{

    struct session_obj **handle = (struct session_obj**) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_open(session_id, sess_mode, handle);
}

int agm_session_set_config(uint64_t hndl,
                           struct agm_session_config *stream_config,
                           struct agm_media_config *media_config,
                           struct agm_buffer_config *buffer_config)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    return session_obj_set_config(handle, stream_config, media_config,
                                                       buffer_config);
}

int agm_session_prepare(uint64_t hndl)
{

    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_prepare(handle);
}

int agm_session_start(uint64_t hndl)
{

    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_start(handle);
}

int agm_session_stop(uint64_t hndl)
{

    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_stop(handle);
}

int agm_session_close(uint64_t hndl)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_close(handle);
}

int agm_session_pause(uint64_t hndl)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_pause(handle);
}

int agm_session_flush(uint64_t hndl)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_flush(handle);
}

int agm_sessionid_flush(uint32_t session_id)
{
    struct session_obj *handle = NULL;
    int ret = 0;

    handle = session_obj_retrieve_from_pool(session_id);
    if (!handle) {
        AGM_LOGE("Incorrect session_id:%d, doesn't match sess_obj from pool",
                                        session_id);
        return -EINVAL;
    }
    return session_obj_flush(handle);
}

int agm_session_resume(uint64_t hndl)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_resume(handle);
}

int agm_session_suspend(uint64_t hndl)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_suspend(handle);
}

int agm_session_write(uint64_t hndl, void *buff, size_t *count)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_write(handle, buff, count);
}

int agm_session_read(uint64_t hndl, void *buff, size_t *count)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_read(handle, buff, count);
}

size_t agm_get_hw_processed_buff_cnt(uint64_t hndl, enum direction dir)
{
    struct session_obj *handle = (struct session_obj *) hndl;
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(hndl)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_hw_processed_buff_cnt(handle, dir);
}

int agm_session_set_loopback(uint32_t capture_session_id,
                 uint32_t playback_session_id, bool state)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(capture_session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                         ret, capture_session_id);
        goto done;
    }

    ret = session_obj_set_loopback(obj, playback_session_id, state);
    if (ret) {
        AGM_LOGE("Error:%d setting loopback for session obj with \
                     session id=%d\n", ret, capture_session_id);
        goto done;
    }

done:
    return ret;
}


int agm_session_set_ec_ref(uint32_t capture_session_id, uint32_t aif_id,
                                                             bool state)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(capture_session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                         ret, capture_session_id);
        goto done;
    }

    ret = session_obj_set_ec_ref(obj, aif_id, state);
    if (ret) {
        AGM_LOGE("Error:%d setting ec_ref for session obj with \
                       session id=%d\n", ret, capture_session_id);
        goto done;
    }

done:
    return ret;
}

int agm_session_eos(uint64_t handle)
{
    if (!handle) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(handle)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_eos((struct session_obj *) handle);
}

int agm_get_session_time(uint64_t handle, uint64_t *timestamp)
{
    if (!handle || !timestamp) {
        AGM_LOGE("Invalid handle or timestamp pointer\n");
        return -EINVAL;
    }

    if (!session_obj_valid_check(handle)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_get_timestamp((struct session_obj *) handle, timestamp);
}

int agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        return ret;
    }

    if (!timestamp) {
        AGM_LOGE("Invalid timestamp pointer\n");
        return -EINVAL;
    }

    return session_obj_buffer_timestamp(obj, timestamp);
}

int agm_session_get_buf_info(uint32_t session_id, struct agm_buf_info *buf_info, uint32_t flag)
{
    struct session_obj *obj = NULL;
    int ret = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        memset(buf_info, 0, sizeof(struct agm_buf_info));
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                 ret, session_id);
        goto done;
    }

    ret = session_obj_get_sess_buf_info(obj, buf_info, flag);
    if (ret) {
        memset(buf_info, 0, sizeof(struct agm_buf_info));
        AGM_LOGE("Error:%d getting buf_info for session id=%d, flag = %d\n",
                 ret, session_id, flag);
    }

done:
    return ret;
}

int agm_register_service_crash_callback(agm_service_crash_cb cb __unused,
                                        uint64_t cookie __unused)
{

    AGM_LOGE("client directly communicating with agm need not call this api");
    return -ENOSYS;
}

int agm_set_gapless_session_metadata(uint64_t handle,
                         enum agm_gapless_silence_type type,
                         uint32_t silence)
{
    if (!handle) {
        AGM_LOGE("%s Invalid handle\n", __func__);
        return -EINVAL;
    }

    if (!session_obj_valid_check(handle)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_set_gapless_metadata((struct session_obj *) handle, type,
                                             silence);
}

int agm_session_write_with_metadata(uint64_t handle, struct agm_buff *buff,
                                    size_t *consumed_size)
{
    if (!handle) {
        AGM_LOGE("%s Invalid handle\n", __func__);
        return -EINVAL;
    }

    if (!session_obj_valid_check(handle)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_write_with_metadata((struct session_obj *) handle, buff,
                                            consumed_size);
}

int agm_session_read_with_metadata(uint64_t handle, struct agm_buff *buff,
                                    uint32_t *captured_size )
{
    if (!handle) {
        AGM_LOGE("%s Invalid handle\n", __func__);
        return -EINVAL;
    }

    if (!session_obj_valid_check(handle)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_read_with_metadata((struct session_obj *) handle, buff,
                                           captured_size);
}

int agm_session_set_non_tunnel_mode_config(uint64_t handle,
                                       struct agm_session_config *session_config,
                                       struct agm_media_config *in_media_config,
                                       struct agm_media_config *out_media_config,
                                       struct agm_buffer_config *in_buffer_config,
                                       struct agm_buffer_config *out_buffer_config)
{
    if (!handle) {
        AGM_LOGE("%s Invalid handle\n", __func__);
        return -EINVAL;
    }

    if (!session_obj_valid_check(handle)) {
        AGM_LOGE("Invalid handle\n");
        return -EINVAL;
    }
    return session_obj_set_non_tunnel_mode_config((struct session_obj *) handle,
                                            session_config,
                                            in_media_config,
                                            out_media_config,
                                            in_buffer_config,
                                            out_buffer_config);
}

int agm_session_write_datapath_params(uint32_t session_id, struct agm_buff *buff)
{
    struct session_obj *obj = NULL;
    int ret = 0;
    size_t consumed_size = 0;

    ret = session_obj_get(session_id, &obj);
    if (ret) {
        AGM_LOGE("Error:%d retrieving session obj with session id=%d\n",
                                                 ret, session_id);
        return ret;
    }

    return session_obj_write_with_metadata(obj, buff, &consumed_size);
}

int agm_dump(struct agm_dump_info *dump_info __unused)
{
    // Placeholder for future enhancements
    return 0;
}
