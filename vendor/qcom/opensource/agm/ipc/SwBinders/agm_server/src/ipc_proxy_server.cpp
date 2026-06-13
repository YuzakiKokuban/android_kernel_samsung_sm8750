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
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#define LOG_TAG "ipc_proxy"

#include <utils/Log.h>

#include <stdlib.h>
#include <utils/RefBase.h>
#include <binder/TextOutput.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <pthread.h>
#include <cutils/list.h>
#include <signal.h>
#include <sys/prctl.h>
#include <system/thread_defs.h>
#include <sys/resource.h>
#include <cstring>
#include <memory.h>
#include <string.h>
#include "ipc_interface.h"
#include "agm_death_notifier.h"
#include <agm/agm_api.h>
#include "agm_server_wrapper.h"
#include "agm_callback.h"
#include "utils.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define MAX_KVPAIR 48

#ifndef memscpy
#define memscpy(dst, dst_size, src, bytes_to_copy) (void) \
                    memcpy(dst, src, MIN(dst_size, bytes_to_copy))
#endif

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_IPC_PROXY_SERVER
#include <log_utils.h>
#endif

struct listnode clbk_data_list;
pthread_mutex_t clbk_data_list_lock;
bool clbk_data_list_init = false;

android::sp<IAGMClient> clt_binder;

using namespace android;

enum {
    OPEN = ::android::IBinder::FIRST_CALL_TRANSACTION,
    REG_CLIENT,
    UNREG_CLIENT,
    SET_MEDIA_CONFIG,
    INIT,
    AUDIO_INTF_SET_META,
    SESSION_SET_META,
    SESSION_AUDIO_INTF_SET_META,
    SESSION_SET_CONFIG,
    CLOSE,
    PREPARE,
    START,
    STOP,
    PAUSE,
    RESUME,
    CONNECT,
    READ,
    WRITE,
    LOOPBACK,
    GET_BUFF_CNT,
    GET_AIF_LIST,
    REG_EVENT,
    REG_CB,
    GET_TAG_MODULE_INFO,
    SESSION_AIF_SET_PARAMS,
    SESSION_SET_PARAMS,
    SET_PARAMS_WITH_TAG,
    SET_ECREF,
    SET_CALIBRATION,
    EOS,
    GET_SESSION_TIME,
    GET_PARAMS,
    BUF_TSTAMP,
    AIF_SET_PARAMS,
    SET_GAPLESS_SESSION_METADATA,
    GET_BUF_INFO,
};

class BpAgmService : public ::android::BpInterface<IAgmService>
{
    public:
        BpAgmService(const android::sp<android::IBinder>& impl) : BpInterface<IAgmService>(impl)
        {
            AGM_LOGV("BpAgmService() called\n");
            android::Parcel data, reply;

            sp<IBinder> binder = new DummyBnClient();
            android::ProcessState::self()->startThreadPool();
            clt_binder = interface_cast<IAGMClient>(binder);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeStrongBinder(IInterface::asBinder(clt_binder));
            remote()->transact(REG_CLIENT, data, &reply);
            AGM_LOGD("calling REG_CLIENT from BpAgmService\n");
        }

        ~BpAgmService()
        {
            android:: Parcel data, reply;

            AGM_LOGV("~BpAgmservice() destructor called\n");
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeStrongBinder(IInterface::asBinder(clt_binder));
            remote()->transact(UNREG_CLIENT, data, &reply);
            AGM_LOGD("calling UNREG_CLIENT from BpAgmService\n");
        }

        virtual int ipc_agm_audio_intf_set_media_config(uint32_t audio_intf,
                                      struct agm_media_config *media_config)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(audio_intf);
            uint32_t param_size = sizeof(agm_media_config);
            data.writeUint32(param_size);
            data.writeBlob(param_size, false, &blob);
            memset(blob.data(), 0x0, param_size);
            memscpy(blob.data(), param_size, media_config, param_size);
            remote()->transact(SET_MEDIA_CONFIG, data, &reply);
            blob.release();
            return reply.readInt32();
        }

        virtual int ipc_agm_session_set_config(uint64_t handle,
            struct agm_session_config *session_config,
            struct agm_media_config *media_config,
            struct agm_buffer_config *buffer_config)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob1, blob2, blob3;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            uint32_t session_param_size = sizeof(agm_session_config);
            data.writeUint32(session_param_size);
            data.writeBlob(session_param_size, false, &blob1);
            memset(blob1.data(), 0x0, session_param_size);
            memscpy(blob1.data(), session_param_size, session_config,
                                                 session_param_size);
            uint32_t media_param_size = sizeof(agm_media_config);
            data.writeUint32(media_param_size);
            data.writeBlob(media_param_size, false, &blob2);
            memset(blob2.data(), 0x0, media_param_size);
            memscpy(blob2.data(), media_param_size, media_config,
                                               media_param_size);
            uint32_t buffer_param_size = sizeof(agm_buffer_config);
            data.writeUint32(buffer_param_size);
            data.writeBlob(buffer_param_size, false, &blob3);
            memset(blob3.data(), 0x0, buffer_param_size);
            memscpy(blob3.data(), buffer_param_size, buffer_config,
                                                        buffer_param_size);
            remote()->transact(SESSION_SET_CONFIG, data, &reply);
            blob1.release();
            blob2.release();
            blob3.release();
            return reply.readInt32();
        }

        virtual int ipc_agm_init()
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            remote()->transact(INIT, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_set_loopback(uint32_t capture_session_id,
                                    uint32_t playback_session_id, bool state)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(capture_session_id);
            data.writeUint32(playback_session_id);
            data.write(&state, sizeof(bool));
            remote()->transact(LOOPBACK, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_audio_intf_set_metadata(uint32_t audio_intf,
                                       uint32_t size, uint8_t *metadata)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(audio_intf);
            data.writeUint32(size);
            data.writeBlob(size, false, &blob);
            memset(blob.data(), 0x0, size);
            memcpy(blob.data(), metadata, size);
            remote()->transact(AUDIO_INTF_SET_META, data, &reply);
            blob.release();

            return reply.readInt32();
        }

        virtual int32_t ipc_agm_session_set_metadata(uint32_t session_id,
                                        uint32_t size, uint8_t *metadata)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(session_id);
            data.writeUint32(size);
            data.writeBlob(size, false, &blob);
            memset(blob.data(), 0x0, size);
            memcpy(blob.data(), metadata, size);
            remote()->transact(SESSION_SET_META, data, &reply);
            blob.release();

            return reply.readInt32();
        }

        virtual int32_t ipc_agm_session_audio_inf_set_metadata(
                            uint32_t session_id, uint32_t audio_intf,
                                    uint32_t size, uint8_t *metadata)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(session_id);
            data.writeUint32(audio_intf);
            data.writeUint32(size);
            data.writeBlob(size, false, &blob);
            memset(blob.data(), 0x0, size);
            memcpy(blob.data(), metadata, size);
            remote()->transact(SESSION_AUDIO_INTF_SET_META, data, &reply);
            blob.release();

            return reply.readInt32();
        }

        virtual int ipc_agm_session_open(uint32_t session_id,
                                         enum agm_session_mode sess_mode,
                                         uint64_t *handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(session_id);
            data.writeUint32(sess_mode);
            remote()->transact(OPEN, data, &reply);
            *handle = (uint64_t)reply.readInt64();
            return reply.readInt32();
        }

        virtual int ipc_agm_session_close(uint64_t handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            remote()->transact(CLOSE, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_prepare(uint64_t handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            remote()->transact(PREPARE, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_start(uint64_t handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            remote()->transact(START, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_stop(uint64_t handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            remote()->transact(STOP, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_pause(uint64_t handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            remote()->transact(PAUSE, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_resume(uint64_t handle)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            remote()->transact(RESUME, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_audio_inf_connect(uint32_t session_id,
                                          uint32_t audio_intf, bool state)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(session_id);
            data.writeUint32(audio_intf);
            data.write(&state, sizeof(bool));
            remote()->transact(CONNECT, data, &reply);
            return reply.readInt32();
        }

        virtual size_t ipc_agm_get_hw_processed_buff_cnt(uint64_t handle,
                                      enum direction dir)
        {
            android::Parcel data, reply;

            AGM_LOGV("%s:%d\n", __func__, __LINE__);
            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)handle);
            data.writeUint32(dir);
            remote()->transact(GET_BUFF_CNT, data, &reply);
            return reply.readInt32();
        }

        virtual int ipc_agm_session_read(uint64_t session_handle, void *buff,
                                          size_t *count)
        {
            int rc = 0;
            android::Parcel data, reply;
            android::Parcel::ReadableBlob blob;

            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)session_handle);
            data.writeUint32(*count);
            remote()->transact(READ, data, &reply);
            rc = reply.readInt32();
            if (rc != 0) {
                AGM_LOGE("read failed error out %d\n", rc);
                goto fail_read;
            }
            *count = reply.readUint32();
            reply.readBlob(*count, &blob);
            memset(buff, 0x0, *count);
            memcpy(buff, blob.data(), *count);
        fail_read:
            return rc;
        }

        virtual int ipc_agm_session_write(uint64_t session_handle, void *buff,
                                          size_t *count)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob;

            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeInt64((long)session_handle);
            data.writeUint32(*count);
            data.writeBlob(*count, false, &blob);
            memset(blob.data(), 0x0, *count);
            memcpy(blob.data(), buff, *count);
            remote()->transact(WRITE, data, &reply);
            *count = reply.readUint32();
            blob.release();
            return reply.readInt32();
        }

        virtual int ipc_agm_get_aif_info_list(struct aif_info *aif_list,
                                                   size_t *num_aif_info)
        {
            android::Parcel data, reply;
            int count = *num_aif_info;

            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            if (aif_list == NULL && count == 0) {
                data.writeUint32(count);
                data.writeInt64((long)aif_list);
                remote()->transact(GET_AIF_LIST, data, &reply);
                *num_aif_info = reply.readInt32();
                return  reply.readInt32();
            } else if (aif_list != NULL && count != 0) {
                android::Parcel::ReadableBlob aif_list_blob;

                uint32_t total_bytecnt = count * sizeof(struct aif_info);
                data.writeUint32(count);
                data.writeInt64((long)aif_list);
                remote()->transact(GET_AIF_LIST, data, &reply);
                reply.readBlob(total_bytecnt, &aif_list_blob);
                memset(aif_list, 0x0, total_bytecnt);
                memcpy(aif_list, aif_list_blob.data(), total_bytecnt);
                aif_list_blob.release();
                return reply.readInt32();
            }
            return -EINVAL;
        }

        virtual int ipc_agm_session_register_for_events(uint32_t session_id,
                                      struct agm_event_reg_cfg *evt_reg_cfg)
        {
            android::Parcel data, reply;
            android::Parcel::WritableBlob blob;

            data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
            data.writeUint32(session_id);
            data.writeUint32(evt_reg_cfg->module_instance_id);
            data.writeUint32(evt_reg_cfg->event_id);
            data.writeUint32(evt_reg_cfg->event_config_payload_size);
            data.writeUint32(evt_reg_cfg->is_register);
            uint32_t param_size = evt_reg_cfg->event_config_payload_size;
            data.writeUint32(param_size);
            data.writeBlob(param_size, false, &blob);
            memset(blob.data(), 0x0, param_size);
            memscpy(blob.data(), param_size, evt_reg_cfg->event_config_payload,
                                                                   param_size);
            remote()->transact(REG_EVENT, data, &reply);
            blob.release();
            return reply.readInt32();
      }

      virtual int ipc_agm_session_register_cb(uint32_t session_id,
                           agm_event_cb cb, enum event_type event,
                                                void *client_data)
      {
          android::Parcel data, reply;

          data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
          data.writeUint32(session_id);
          data.write(&cb, sizeof(agm_event_cb *));
          data.writeUint32(event);
          data.writeInt64((long)client_data);
          sp<IBinder> binder = new BnCallback();
          android::ProcessState::self()->startThreadPool();
          sp<ICallback> clbk = interface_cast<ICallback>(binder);
          data.writeStrongBinder(IInterface::asBinder(clbk));
          remote()->transact(REG_CB, data, &reply);
          return reply.readInt32();
     }

     virtual int ipc_agm_session_aif_get_tag_module_info(uint32_t session_id,
                                uint32_t aif_id, void *payload, size_t *size)
     {
         android::Parcel data, reply;
         int count = *size;

         data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
         if (payload != NULL && count != 0) {
             android::Parcel::ReadableBlob tag_info_blob;

             data.writeUint32(count);
             data.writeUint32(session_id);
             data.writeUint32(aif_id);
             remote()->transact(GET_TAG_MODULE_INFO, data, &reply);
             count = (size_t) reply.readUint32();
             reply.readBlob(count, &tag_info_blob);
             memcpy(payload, tag_info_blob.data(), count);
             tag_info_blob.release();
             return reply.readInt32();
         }
         return -EINVAL;
    }

    virtual int ipc_agm_session_aif_set_params(uint32_t session_id,
                              uint32_t aif_id, void *payload, size_t count)
    {
        android::Parcel data, reply;
        android::Parcel::WritableBlob blob;

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(session_id);
        data.writeUint32(aif_id);
        data.writeUint32(count);
        data.writeBlob(count, false, &blob);
        memset(blob.data(), 0x0, count);
        memcpy(blob.data(), payload, count);
        remote()->transact(SESSION_AIF_SET_PARAMS, data, &reply);
        blob.release();
        return reply.readInt32();
    }

    virtual int ipc_agm_aif_set_params(uint32_t aif_id,
                                 void *payload, size_t count)
    {
        android::Parcel data, reply;
        android::Parcel::WritableBlob blob;

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(aif_id);
        data.writeUint32(count);
        data.writeBlob(count, false, &blob);
        memset(blob.data(), 0x0, count);
        memcpy(blob.data(), payload, count);
        remote()->transact(AIF_SET_PARAMS, data, &reply);
        blob.release();
        return reply.readInt32();
    }

    virtual int ipc_agm_session_set_params(uint32_t session_id,
                                           void *payload, size_t count)
    {
        android::Parcel data, reply;
        android::Parcel::WritableBlob blob;

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(session_id);
        data.writeUint32(count);
        data.writeBlob(count, false, &blob);
        memset(blob.data(), 0x0, count);
        memcpy(blob.data(), payload, count);
        remote()->transact(SESSION_SET_PARAMS, data, &reply);
        blob.release();
        return  reply.readInt32();
    }

    virtual int ipc_agm_set_params_with_tag(uint32_t session_id,
                                            uint32_t aif_id,
                                            struct agm_tag_config *tag_config)
    {
        android::Parcel data, reply;
        android::Parcel::WritableBlob tkv_blob;
        uint32_t kv_size = sizeof(agm_key_value);
        uint32_t tkv_size = kv_size * (tag_config->num_tkvs);

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(session_id);
        data.writeUint32(aif_id);
        data.writeUint32(tag_config->tag);
        data.writeUint32(tag_config->num_tkvs);
        data.writeBlob(tkv_size, false, &tkv_blob);
        memset(tkv_blob.data(), 0x0, tkv_size);
        memcpy(tkv_blob.data(), &tag_config->kv[0], tkv_size);
        remote()->transact(SET_PARAMS_WITH_TAG, data, &reply);
        tkv_blob.release();
        return  reply.readInt32();
    }

    virtual int ipc_agm_session_eos(uint64_t handle)
    {
        android::Parcel data, reply;

        AGM_LOGV("%s:%d\n", __func__, __LINE__);
        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeInt64((long)handle);
        remote()->transact(EOS, data, &reply);
        return reply.readInt32();
    }

    virtual int ipc_agm_get_session_time(uint64_t handle, uint64_t *timestamp)
    {
        android::Parcel data, reply;

        AGM_LOGV("%s:%d\n", __func__, __LINE__);
        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeInt64((long)handle);
        data.writeUint64(*timestamp);
        remote()->transact(GET_SESSION_TIME, data, &reply);
        *timestamp = reply.readUint64();
        return reply.readInt32();
    }

    virtual int ipc_agm_session_set_ec_ref(uint32_t capture_session_id,
                                           uint32_t aif_id, bool state)
{
        android::Parcel data, reply;

        AGM_LOGV("%s:%d\n", __func__, __LINE__);
        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(capture_session_id);
        data.writeUint32(aif_id);
        data.write(&state, sizeof(bool));
        remote()->transact(SET_ECREF, data, &reply);
        return reply.readInt32();
    }

    virtual int ipc_agm_session_aif_set_cal(uint32_t session_id,
                                            uint32_t audio_intf,
                            struct agm_cal_config *cal_config) {
        android::Parcel data, reply;
        android::Parcel::WritableBlob ckv_blob;
        uint32_t kv_size = sizeof(agm_key_value);
        uint32_t ckv_size = kv_size * (cal_config->num_ckvs);

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        AGM_LOGV("\n%s: CKV key %x value:%x\n", __func__,
                           (cal_config->kv[0].key),(cal_config->kv[0].value));
        data.writeUint32(session_id);
        data.writeUint32(audio_intf);
        data.writeUint32(cal_config->num_ckvs);
        data.writeBlob(ckv_size, false, &ckv_blob);
        memset(ckv_blob.data(), 0x0, ckv_size);
        memcpy(ckv_blob.data(), cal_config->kv, ckv_size);
        remote()->transact(SET_CALIBRATION, data, &reply);
        ckv_blob.release();
        return reply.readInt32();
    }

    virtual int ipc_agm_session_get_params(uint32_t session_id,
                                           void *payload, size_t count)
     {
        android::Parcel data, reply;
        android::Parcel::WritableBlob blob;
        android::Parcel::ReadableBlob get_param_blob;

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(session_id);
        data.writeUint32(count);
        data.writeBlob(count, false, &blob);
        memset(blob.data(), 0x0, count);
        memcpy(blob.data(), payload, count);

        remote()->transact(GET_PARAMS, data, &reply);

        reply.readBlob(count, &get_param_blob);
        memcpy(payload, get_param_blob.data(), count);
        blob.release();
        get_param_blob.release();
        return reply.readInt32();
    }

    virtual int ipc_agm_get_buffer_timestamp(uint32_t session_id, uint64_t *timestamp)
    {
        android::Parcel data, reply;
        ALOGV("%s:%d\n", __func__, __LINE__);
        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(session_id);
        remote()->transact(BUF_TSTAMP, data, &reply);
        *timestamp = reply.readUint64();
        return reply.readInt32();
    }

    virtual int ipc_agm_set_gapless_session_metadata(uint64_t handle,
                                   enum agm_gapless_silence_type type,
                                                     uint32_t silence)
    {
        android::Parcel data, reply;
        ALOGV("%s:%d\n", __func__, __LINE__);
        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeInt64((long)handle);
        data.writeUint32(type);
        data.writeUint32(silence);
        remote()->transact(SET_GAPLESS_SESSION_METADATA, data, &reply);
        return reply.readInt32();
    }

    virtual int ipc_agm_session_get_buf_info(uint32_t session_id,
                          struct agm_buf_info *buf_info, uint32_t flag)
    {
        android::Parcel data, reply;

        data.writeInterfaceToken(IAgmService::getInterfaceDescriptor());
        data.writeUint32(session_id);
        data.writeUint32(flag);
        remote()->transact(GET_BUF_INFO, data, &reply);
        if (flag & DATA_BUF) {
            buf_info->data_buf_fd = dup(reply.readFileDescriptor());
            buf_info->data_buf_size = reply.readInt32();
        }
        if (flag & POS_BUF) {
            buf_info->pos_buf_fd = dup(reply.readFileDescriptor());
            buf_info->pos_buf_size = reply.readInt32();
        }
        return reply.readInt32();
    }
};

void ipc_cb (uint32_t session_id, struct agm_event_cb_params *event_params,
                                                         void *client_data)
{
    struct listnode *node = NULL;
    clbk_data *handle = NULL;

    pthread_mutex_lock(&clbk_data_list_lock);
    list_for_each(node, &clbk_data_list) {
        handle = node_to_item(node, clbk_data, list);
        AGM_LOGV("%s: session_id %d, client_data %p\n",
                   __func__, handle->session_id, handle->client_data);
        if (handle != NULL && handle->session_id == session_id &&
                              handle->client_data == client_data) {
            AGM_LOGV("%s: Found handle %p\n", __func__, handle);
            break;
        }
    }

    if (handle!= NULL) {
        sp<ICallback> cb_binder = handle->cb_binder;
        if (cb_binder == NULL) {
            AGM_LOGE("%s Invalid binder handle\n", __func__);
            pthread_mutex_unlock(&clbk_data_list_lock);
            return;
        }
        cb_binder->event_cb(session_id, event_params,
               handle->client_data, handle->cb_func);
    }
    pthread_mutex_unlock(&clbk_data_list_lock);
}


IMPLEMENT_META_INTERFACE(AgmService, "AgmService");

android::status_t BnAgmService::onTransact(uint32_t code,
             const android::Parcel& data, android::Parcel* reply,
                                                  uint32_t flags)
{
    AGM_LOGD("BnAgmService::onTransact(%i) %i\n", code, flags);
    int rc = -EINVAL;
    data.checkInterface(this);

    switch (code) {
    case REG_CLIENT : {
        sp<IBinder> binder = data.readStrongBinder();
        agm_register_client(binder);
        break; }

    case UNREG_CLIENT : {
        sp<IBinder> binder = data.readStrongBinder();
        agm_unregister_client(binder);
        break; }

   case OPEN : {
        uint32_t session_id;
        enum agm_session_mode sess_mode;
        uint64_t handle = 0;
        session_id = data.readUint32();
        sess_mode = (enum agm_session_mode)data.readUint32();
        rc = ipc_agm_session_open(session_id, sess_mode, &handle);
        if (handle != 0)
            agm_add_session_obj_handle(handle);reply->writeInt64((long)handle);
        reply->writeInt32(rc);
        break; }

    case SESSION_SET_META : {
        uint32_t session_id = 0;
        size_t count = 0;
        void *bn_payload;
        android::Parcel::ReadableBlob blob;

        session_id = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            rc =  -ENOMEM;
            goto fail_ses_set_meta;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_session_set_metadata(session_id, count,
                                     (uint8_t *)bn_payload);
    fail_ses_set_meta:
        if (bn_payload)
            free(bn_payload);
        reply->writeInt32(rc);
        break;
    }
    case SESSION_AUDIO_INTF_SET_META : {
        uint32_t session_id, audio_intf = 0;
        size_t count = 0;
        void *bn_payload;
        android::Parcel::ReadableBlob blob;

        session_id = data.readUint32();
        audio_intf = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            rc =  -ENOMEM;
            goto fail_ses_aud_set_meta;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_session_audio_inf_set_metadata(session_id, audio_intf,
                                             count, (uint8_t *)bn_payload);
    fail_ses_aud_set_meta:
        if (bn_payload)
            free(bn_payload);
        reply->writeInt32(rc);
        break;
    }
    case INIT : {
        rc = ipc_agm_init();
        reply->writeInt32(rc);
        break;
        }

    case GET_BUFF_CNT :{
        enum direction dir;
        uint64_t handle = (uint64_t )data.readInt64();
        dir = (direction) data.readUint32();
        rc = (uint32_t)ipc_agm_get_hw_processed_buff_cnt(handle, dir);
        reply->writeInt32(rc);
        break;
        }

    case AUDIO_INTF_SET_META : {
        uint32_t audio_intf;
        size_t count = 0;
        void *bn_payload;
        android::Parcel::ReadableBlob blob;

        audio_intf = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            rc =  -ENOMEM;
            goto fail_audio_set_meta;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_audio_intf_set_metadata(audio_intf, count,
                                        (uint8_t *)bn_payload);

    fail_audio_set_meta:
        if (bn_payload)
            free(bn_payload);
        reply->writeInt32(rc);
        break;
    }

    case LOOPBACK : {
        uint32_t capture_session_id;
        uint32_t playback_session_id;
        capture_session_id = data.readUint32();
        playback_session_id = data.readUint32();
        int state = data.readUint32();
        rc = ipc_agm_session_set_loopback(capture_session_id,
                                 playback_session_id, state);
        reply->writeInt32(rc);
        break; }

    case CLOSE : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_close(handle);
        agm_remove_session_obj_handle(handle);
        reply->writeInt32(rc);
        break; }

    case PREPARE : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_prepare(handle);
        reply->writeInt32(rc);
        break; }

    case START : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_start(handle);
        reply->writeInt32(rc);
        break; }

    case STOP : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_stop(handle);
        reply->writeInt32(rc);
        break; }

    case PAUSE : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_pause(handle);
        reply->writeInt32(rc);
        break; }

    case RESUME : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_resume(handle);
        reply->writeInt32(rc);
        break; }

    case CONNECT : {
        uint32_t session_id;
        uint32_t audio_intf;

        session_id = data.readUint32();
        audio_intf = data.readUint32();
        int state = data.readUint32();
        rc = ipc_agm_session_audio_inf_connect(session_id, audio_intf, state);
        reply->writeInt32(rc);
        break; }

    case SET_MEDIA_CONFIG : {
        uint32_t audio_intf;
        audio_intf = data.readUint32();
        struct agm_media_config media_config;
        android::Parcel::ReadableBlob blob;
        uint32_t blob_size;
        data.readUint32(&blob_size);
        data.readBlob(blob_size, &blob);
        memset(&media_config, 0x0, sizeof(agm_media_config));
        memscpy(&media_config, sizeof(agm_media_config), blob.data(),
                                                          blob_size);
        blob.release();
        rc = ipc_agm_audio_intf_set_media_config(audio_intf, &media_config);
        reply->writeInt32(rc);
        break; }

    case SESSION_SET_CONFIG : {
        uint64_t handle = (uint64_t )data.readInt64();
        struct agm_session_config session_config;
        struct agm_media_config media_config;
        struct agm_buffer_config buffer_config;
        android::Parcel::ReadableBlob blob;
        uint32_t blob1_size, blob2_size, blob3_size;

        data.readUint32(&blob1_size);
        data.readBlob(blob1_size, &blob);
        memset(&session_config, 0x0, sizeof(agm_session_config));
        memscpy(&session_config, sizeof(agm_session_config), blob.data(),
                                                             blob1_size);
        blob.release();
        data.readUint32(&blob2_size);
        data.readBlob(blob2_size, &blob);
        memset(&media_config, 0x0, sizeof(agm_media_config));
        memscpy(&media_config, sizeof(agm_media_config), blob.data(),
                                                         blob2_size);
        blob.release();

        data.readUint32(&blob3_size);
        data.readBlob(blob3_size, &blob);
        memset(&buffer_config, 0x0, sizeof(agm_buffer_config));
        memscpy(&buffer_config, sizeof(agm_buffer_config), blob.data(),
                                                           blob3_size);
        blob.release();
        rc = ipc_agm_session_set_config(handle, &session_config, &media_config,
                                                               &buffer_config);
        reply->writeInt32(rc);
        break; }

    case READ : {
        int32_t rc;
        size_t byte_count;
        uint64_t handle;
        void *buf = NULL;
        android::Parcel::WritableBlob blob;
        handle = (uint64_t )data.readInt64();
        byte_count = data.readUint32();

        buf = (void *)calloc(1, byte_count);
        if (buf == NULL) {
            AGM_LOGE("calloc failed\n");
            rc = -ENOMEM;
            reply->writeInt32(rc);
            goto free_buff;
        }

        rc = ipc_agm_session_read(handle, buf, &byte_count);
        reply->writeInt32(rc);
        if (rc != 0) {
            AGM_LOGE("session_read failed %d\n", rc);
            goto free_buff;
        }
        reply->writeUint32(byte_count);
        reply->writeBlob(byte_count, false, &blob);
        memset(blob.data(), 0x0, byte_count);
        memcpy(blob.data(), buf, byte_count);

    free_buff:
        if (buf)
           free(buf);
       break; }

    case WRITE: {
        uint32_t rc;
        size_t byte_count;
        void *buf;
        uint64_t handle;
        android::Parcel::ReadableBlob blob;
        handle = (uint64_t )data.readInt64();
        byte_count = data.readUint32();

        buf = calloc(1,byte_count);
        if (buf == NULL) {
            AGM_LOGE("calloc failed\n");
            rc = -ENOMEM;
            goto fail_write;
        }
        data.readBlob(byte_count, &blob);
        memcpy(buf, blob.data(), byte_count);
        rc = ipc_agm_session_write(handle, buf, &byte_count);
        reply->writeUint32(byte_count);
        blob.release();

    fail_write:
        if (buf)
           free(buf);
        reply->writeInt32(rc);
       break; }

    case GET_AIF_LIST: {
        uint32_t rc, cp_val;
        size_t count = 0;
        struct aif_info *aif_list, *aif_list_bn;
        android::Parcel::WritableBlob aif_list_blob;

        count = (size_t) data.readUint32();
        if (count == 0) {
            aif_list = (struct aif_info*)data.readInt64();
            rc = ipc_agm_get_aif_info_list(aif_list, &count);
            reply->writeInt32(count);
            reply->writeInt32(rc);
        } else if (count != 0){
            aif_list_bn = (struct aif_info*)calloc(count,
                                    sizeof(struct aif_info));
            if (aif_list_bn == NULL) {
                AGM_LOGE("calloc failed\n");
                return -ENOMEM;
            }
            rc = ipc_agm_get_aif_info_list(aif_list_bn, &count);
            int total_bytecnt =  count*sizeof(struct aif_info);
            if(!rc) {
            for (cp_val= 0; cp_val < count; cp_val++)
                AGM_LOGV("aif_list %s:name:%s count:%d\n", __func__,
                         aif_list_bn[cp_val].aif_name, total_bytecnt);
            }
            reply->writeBlob(total_bytecnt, false, &aif_list_blob);
            memset(aif_list_blob.data(), 0x0, total_bytecnt);
            memcpy(aif_list_blob.data(), aif_list_bn, total_bytecnt);
            aif_list_blob.release();
            reply->writeInt32(rc);
            free(aif_list_bn);
        }
        break; }


     case REG_EVENT : {
         uint32_t session_id;
         uint32_t module_instance_id;
         uint32_t event_id;
         uint32_t payload_size;
         uint8_t is_register;
         struct agm_event_reg_cfg *evt_reg_cfg = NULL;
         android::Parcel::ReadableBlob blob;
         uint32_t blob_size;

         session_id = data.readUint32();
         module_instance_id = data.readUint32();
         event_id = data.readUint32();
         payload_size = data.readUint32();
         is_register = (uint8_t) data.readUint32();
         evt_reg_cfg = (struct agm_event_reg_cfg*)calloc(1,
                 (sizeof(agm_event_reg_cfg)+payload_size));
         if (evt_reg_cfg == NULL) {
             AGM_LOGE("calloc failed\n");
             return -ENOMEM;
         }
         evt_reg_cfg->module_instance_id = module_instance_id;
         evt_reg_cfg->event_id = event_id;
         evt_reg_cfg->event_config_payload_size = payload_size;
         evt_reg_cfg->is_register = is_register;
         data.readUint32(&blob_size);
         data.readBlob(blob_size, &blob);

         memset(evt_reg_cfg->event_config_payload, 0x0,
               evt_reg_cfg->event_config_payload_size);
         memscpy(evt_reg_cfg->event_config_payload,
                 evt_reg_cfg->event_config_payload_size,
                                blob.data(), blob_size);
         blob.release();
         rc = ipc_agm_session_register_for_events(session_id, evt_reg_cfg);
         free(evt_reg_cfg);
         reply->writeInt32(rc);
         break; }

    case REG_CB : {
        enum event_type evnt;
        clbk_data *clbk_data_obj = NULL;

        if (clbk_data_list_init == false) {
            pthread_mutex_init(&clbk_data_list_lock,
                               (const pthread_mutexattr_t *) NULL);
            list_init(&clbk_data_list);
            clbk_data_list_init = true;
        }

        clbk_data_obj = (clbk_data *)calloc(1, sizeof(clbk_data));
        if (clbk_data_obj == NULL) {
            AGM_LOGE("calloc failed\n");
            return -ENOMEM;
        }

        pthread_mutex_lock(&clbk_data_list_lock);
        clbk_data_obj->session_id = data.readUint32();
        data.read(&clbk_data_obj->cb_func, sizeof(agm_event_cb));
        evnt = (event_type) data.readUint32();
        clbk_data_obj->client_data = (void *)data.readInt64();
        sp<IBinder> binder = data.readStrongBinder();
        clbk_data_obj->cb_binder = interface_cast<ICallback>(binder);
        if (clbk_data_obj->cb_func != NULL) {
            list_add_tail(&clbk_data_list, &clbk_data_obj->list);
            rc = ipc_agm_session_register_cb(clbk_data_obj->session_id,
                            &ipc_cb, evnt, clbk_data_obj->client_data);
        } else {
            clbk_data *clbk_data_obj_tmp = NULL;
            struct listnode *node = NULL, *next = NULL;
            list_for_each_safe(node, next, &clbk_data_list) {
                clbk_data_obj_tmp = node_to_item(node, clbk_data, list);
                if ((clbk_data_obj_tmp->session_id == clbk_data_obj->session_id) &&
                    (clbk_data_obj_tmp->client_data == clbk_data_obj->client_data)) {
                    list_remove(&clbk_data_obj_tmp->list);
                    free(clbk_data_obj_tmp);
                }
            }
            rc = ipc_agm_session_register_cb(clbk_data_obj->session_id,
                            NULL, evnt, clbk_data_obj->client_data);
            free(clbk_data_obj);
        }

        pthread_mutex_unlock(&clbk_data_list_lock);
        reply->writeInt32(rc);
        break ; }

    case GET_TAG_MODULE_INFO: {
        uint32_t rc, pcm_idx, be_idx;
        size_t count = 0;
        void *bn_payload = NULL;

        count = (size_t) data.readUint32();
        pcm_idx = data.readUint32();
        be_idx = data.readUint32();
        if (count != 0){
            bn_payload = calloc(count, sizeof(uint8_t));
            if (bn_payload == NULL) {
                AGM_LOGE("calloc failed\n");
                return -ENOMEM;
            }
            rc = ipc_agm_session_aif_get_tag_module_info(pcm_idx, be_idx,
                                                     bn_payload, &count);
            android::Parcel::WritableBlob tag_info_blob;
            reply->writeUint32(count);
            reply->writeBlob(count, false, &tag_info_blob);
            memcpy(tag_info_blob.data(), bn_payload, count);
            tag_info_blob.release();
            free(bn_payload);
            reply->writeInt32(rc);
        }
        break; }

    case SESSION_AIF_SET_PARAMS: {
        uint32_t rc, pcm_idx, be_idx;
        size_t count = 0;
        android::Parcel::ReadableBlob blob;
        void *bn_payload;

        pcm_idx = data.readUint32();
        be_idx = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            reply->writeInt32(-ENOMEM);
            return -ENOMEM;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_session_aif_set_params(pcm_idx, be_idx, bn_payload, count);
        blob.release();
        free(bn_payload);
        reply->writeInt32(rc);
        break; }

    case AIF_SET_PARAMS: {
        uint32_t rc, be_idx;
        size_t count = 0;
        android::Parcel::ReadableBlob blob;
        void *bn_payload;

        be_idx = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            reply->writeInt32(-ENOMEM);
            return -ENOMEM;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_aif_set_params(be_idx, bn_payload, count);
        blob.release();
        free(bn_payload);
        reply->writeInt32(rc);
        break; }

    case SESSION_SET_PARAMS: {
        uint32_t rc, pcm_idx;
        size_t count = 0;
        void *bn_payload;
        android::Parcel::ReadableBlob blob;

        pcm_idx = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            rc =  -ENOMEM;
            goto session_set_param_fail;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_session_set_params(pcm_idx, bn_payload, count);

        free(bn_payload);
    session_set_param_fail:
        reply->writeInt32(rc);
        break; }

    case SET_PARAMS_WITH_TAG: {
        uint32_t rc, pcm_idx, be_idx;
        struct agm_tag_config *atc = NULL;
        uint32_t tag, num_tkv,tkv_blob_size = 0;
        android::Parcel::ReadableBlob tkv_blob;

        pcm_idx = data.readUint32();
        be_idx = data.readUint32();
        tag = data.readUint32();
        num_tkv = data.readUint32();
        tkv_blob_size = num_tkv * (uint32_t)sizeof(struct agm_key_value);
        AGM_LOGV("Bn:tkv.num_kvs = %d size %d\n", num_tkv, tkv_blob_size);
        atc = (struct agm_tag_config*)calloc(1, sizeof(struct agm_tag_config) +
                                              num_tkv * sizeof(agm_key_value));
        if (!atc) {
            AGM_LOGE("calloc failed\n");
            rc = -ENOMEM;
            goto set_param_with_tag_fail;
        }
        atc->num_tkvs = num_tkv;
        atc->tag = tag;
        data.readBlob(tkv_blob_size, &tkv_blob);
        memcpy(&atc->kv[0], tkv_blob.data(), tkv_blob_size);
        tkv_blob.release();
        rc = ipc_agm_set_params_with_tag (pcm_idx, be_idx, atc);
    set_param_with_tag_fail:
        reply->writeInt32(rc);
        break; }

    case SET_ECREF :     {
        uint32_t cap_sess_id;
        uint32_t aif_id;

        cap_sess_id = data.readUint32();
        aif_id = data.readUint32();
        int state = data.readUint32();
        rc = ipc_agm_session_set_ec_ref(cap_sess_id, aif_id, state);
        reply->writeInt32(rc);
        break; }

    case SET_CALIBRATION: {
        uint32_t session_id, audio_intf = 0;
        struct agm_cal_config *acc = NULL;
        uint32_t num_ckv;
        uint32_t ckv_blob_size = 0;
        android::Parcel::ReadableBlob ckv_blob;

        session_id = data.readUint32();
        audio_intf = data.readUint32();
        num_ckv = data.readUint32();
        if (num_ckv > MAX_KVPAIR) {
            AGM_LOGE("Num KVs %d more than expected: %d\n", num_ckv, MAX_KVPAIR);
            rc = -EINVAL;
            goto fail_ses_aud_set_cal_data;
        }
        ckv_blob_size = num_ckv * (uint32_t)sizeof(struct agm_key_value);
        acc = (struct agm_cal_config *) calloc (1, sizeof(struct agm_cal_config)
                                                               + ckv_blob_size);
        if (!acc) {
            AGM_LOGE("calloc failed\n");
            rc = -ENOMEM;
            goto fail_ses_aud_set_cal_data;
        }
        acc->num_ckvs = num_ckv;
        data.readBlob(ckv_blob_size, &ckv_blob);
        memcpy(acc->kv, ckv_blob.data(), ckv_blob_size);
        ckv_blob.release();
        rc = ipc_agm_session_aif_set_cal(session_id, audio_intf, acc);
    fail_ses_aud_set_cal_data:
        if (acc)
            free(acc);
        reply->writeInt32(rc);
        break; }

    case EOS : {
        uint64_t handle = (uint64_t )data.readInt64();
        rc = ipc_agm_session_eos(handle);
        reply->writeInt32(rc);
        break; }

    case GET_SESSION_TIME : {
        uint64_t handle = (uint64_t )data.readInt64();
        uint64_t ts = data.readUint64();
        rc = ipc_agm_get_session_time(handle, &ts);
        reply->writeUint64(ts);
        reply->writeInt32(rc);
        break; }

    case GET_PARAMS: {
        uint32_t rc, pcm_idx;
        size_t count = 0;
        void *bn_payload;
        android::Parcel::ReadableBlob blob;
        android::Parcel::WritableBlob get_param_blob;

        pcm_idx = data.readUint32();
        count = (size_t) data.readUint32();
        data.readBlob(count, &blob);

        bn_payload = calloc(count, sizeof(uint8_t));
        if (!bn_payload) {
            AGM_LOGE("calloc failed\n");
            rc =  -ENOMEM;
            goto session_get_param_fail;
        }

        memcpy(bn_payload, blob.data(), count);
        rc = ipc_agm_session_get_params(pcm_idx, bn_payload, count);

        reply->writeBlob(count, false, &get_param_blob);
        memcpy(get_param_blob.data(), bn_payload, count);
        free(bn_payload);
    session_get_param_fail:
        reply->writeInt32(rc);
        get_param_blob.release();
        blob.release();
        break; }

    case BUF_TSTAMP : {
        uint32_t rc, pcm_idx;
        uint64_t ts = 0;

        pcm_idx = data.readUint32();
        rc = ipc_agm_get_buffer_timestamp(pcm_idx, &ts);
        reply->writeUint64(ts);
        reply->writeInt32(rc);
        break; }

    case SET_GAPLESS_SESSION_METADATA : {
        uint64_t handle = (uint64_t )data.readInt64();
        agm_gapless_silence_type type = (agm_gapless_silence_type) data.readUint32();
        uint32_t silence = data.readUint32();
        rc = ipc_agm_set_gapless_session_metadata(handle, type, silence);
        reply->writeInt32(rc);
        break; }

    case GET_BUF_INFO : {
        int rc;
        uint32_t session_id, flag;
        struct agm_buf_info buf_info;

        session_id = data.readUint32();
        flag = data.readUint32();
        rc = ipc_agm_session_get_buf_info(session_id, &buf_info, flag);
        if (flag & DATA_BUF) {
            reply->writeFileDescriptor(buf_info.data_buf_fd);
            reply->writeInt32(buf_info.data_buf_size);
        }
        if (flag & POS_BUF) {
            reply->writeFileDescriptor(buf_info.pos_buf_fd);
            reply->writeInt32(buf_info.pos_buf_size);
        }
        reply->writeInt32(rc);
        break; }

    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
return 0;
}
