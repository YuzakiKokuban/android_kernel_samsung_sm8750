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
**/

#define LOG_TAG "agm_callback"

#include <stdlib.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <binder/TextOutput.h>

#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/MemoryDealer.h>
#include <memory.h>
#include <pthread.h>
#include "agm_callback.h"
#include "utils.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef memscpy
#define memscpy(dst, dst_size, src, bytes_to_copy) (void) \
                    memcpy(dst, src, MIN(dst_size, bytes_to_copy))
#endif

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_CALLBACK
#include <log_utils.h>
#endif

using namespace android;

const android::String16 ICallback::descriptor("ICallback");
const android::String16& ICallback::getInterfaceDescriptor() const
{
    return ICallback::descriptor;
}

class BpCallback: public ::android:: BpInterface<ICallback>
{
public:
    BpCallback(const sp<IBinder>& impl) :  BpInterface<ICallback>(impl)
    {
            AGM_LOGD("BpCallback::BpCallback()\n");
    }

    int event_cb(uint32_t session_id, struct agm_event_cb_params *event_params,
                                       void *client_data, agm_event_cb cb_func)
    {
        AGM_LOGV("%s %d\n", __func__, __LINE__);
        android::Parcel data, reply;
        data.writeInterfaceToken(ICallback::getInterfaceDescriptor());
        data.writeUint32(session_id);
        data.writeUint32(event_params->source_module_id);
        data.writeUint32(event_params->event_id);
        data.writeUint32(event_params->event_payload_size);
        uint32_t param_size = event_params->event_payload_size;
        android::Parcel::WritableBlob blob;
        data.writeBlob(param_size, false, &blob);
        memset(blob.data(), 0x0, param_size);
        memscpy(blob.data(), param_size, event_params->event_payload, param_size);
        blob.release();
        data.writeInt64((long)client_data);
        data.write(&cb_func, sizeof(agm_event_cb *));
        return remote()->transact(event_params->event_id, data, &reply);
   }
};

android::sp<ICallback> ICallback::asInterface
         (const android::sp<android::IBinder>& obj)
{
   AGM_LOGD("ICallback::asInterface()\n");
   android::sp<ICallback> intr;
   if (obj != NULL) {
       intr = static_cast<ICallback*>(obj->queryLocalInterface(ICallback::descriptor).get());
       AGM_LOGD("ICallback::asInterface() interface %s\n",
                 ((intr == 0)?"zero":"non zero"));
       if (intr == NULL)
           intr = new BpCallback(obj);
   }
    return intr;
}

ICallback::ICallback()
{AGM_LOGD("ICallback::ICallback()\n"); }
ICallback::~ICallback()
{AGM_LOGD("ICallback::~ICallback()\n"); }

int32_t BnCallback::onTransact(uint32_t code,
                                   const Parcel& data,
                                   Parcel* reply __unused,
                                   uint32_t flags)
{
    AGM_LOGD("BnCallback::onTransact(%i) %i\n", code, flags);
    data.checkInterface(this);
    uint32_t session_id;
    uint32_t source_module_id;
    uint32_t event_id;
    uint32_t event_payload_size;
    void *client_data;
    agm_event_cb cb_func;
    struct agm_event_cb_params *event_params = NULL;
    session_id = data.readUint32();
    source_module_id = data.readUint32();
    event_id = data.readUint32();
    event_payload_size = data.readUint32();
    event_params = (struct agm_event_cb_params *) calloc(1,
                          (sizeof(agm_event_cb_params) + event_payload_size));
    if (event_params == NULL) {
        AGM_LOGE("%s: Cannot allocate memory for struct \
                                            agm_event_cb_params\n", __func__);
        return -ENOMEM;
    }
    event_params->source_module_id = source_module_id;
    event_params->event_id = (agm_event_id) event_id;
    event_params->event_payload_size = event_payload_size;
    android::Parcel::ReadableBlob blob;
    uint32_t blob_size = event_payload_size ;
    data.readBlob(blob_size, &blob);
    memset(event_params->event_payload, 0x0, event_params->event_payload_size);
    memscpy(event_params->event_payload,
                    event_params->event_payload_size , blob.data(), blob_size);
    blob.release();
    client_data = (void *)data.readInt64();
    data.read(&cb_func, sizeof(agm_event_cb *));
    return event_cb(session_id, event_params, client_data, cb_func);
}

int BnCallback::event_cb(uint32_t session_id,
                         struct agm_event_cb_params *event_params,
                         void *client_data, agm_event_cb cb_func)
{
    AGM_LOGD("Calling Client Registered Callback(%x)\n", cb_func);
    cb_func(session_id, event_params, client_data);
    return 0;
}
