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

#define LOG_TAG "agm_death_notifier"

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
#include <pthread.h>
#include <cutils/list.h>
#include <signal.h>
#include "ipc_interface.h"
#include "agm_death_notifier.h"
#include "utils.h"

#ifdef DYNAMIC_LOG_ENABLED
#include <log_xml_parser.h>
#define LOG_MASK AGM_MOD_FILE_AGM_DEATH_NOTIFIER
#include <log_utils.h>
#endif

using namespace android;

struct listnode g_client_list;
pthread_mutex_t g_client_list_lock;
bool g_client_list_init = false;
//initialize mutex

client_death_notifier::client_death_notifier(void)
{
    AGM_LOGV("%s:%d\n", __func__, __LINE__);
    sp<ProcessState> proc(ProcessState::self());
    proc->startThreadPool();
}
sp<client_death_notifier> Client_death_notifier = NULL;

client_info *get_client_handle_from_list(pid_t pid)
{
    struct listnode *node = NULL;
    client_info *handle = NULL;

    pthread_mutex_lock(&g_client_list_lock);
    list_for_each(node, &g_client_list) {
        handle = node_to_item(node, client_info, list);
        if (handle->pid == pid) {
            AGM_LOGV("%s: Found handle %p\n", __func__, handle);
            pthread_mutex_unlock(&g_client_list_lock);
            return handle;
        }
    }
    pthread_mutex_unlock(&g_client_list_lock);
    return NULL;
}

void agm_register_client(sp<IBinder> binder)
{
    client_info *client_handle = NULL;
    pid_t pid = IPCThreadState::self()->getCallingPid();
    android::sp<IAGMClient> client_binder =
                                  android::interface_cast<IAGMClient>(binder);
    Client_death_notifier = new client_death_notifier();
    IInterface::asBinder(client_binder)->linkToDeath(Client_death_notifier);
    AGM_LOGD("%s: Client registered and death notifier linked to AGM\n",
                                                              __func__);
    if (g_client_list_init == false) {
        pthread_mutex_init(&g_client_list_lock,
                                           (const pthread_mutexattr_t *) NULL);
        list_init(&g_client_list);
        g_client_list_init = true;
    }
    client_handle = (client_info *)calloc(1, sizeof(client_info));
    if (client_handle == NULL) {
        AGM_LOGE("%s: Cannot allocate memory for client handle\n",
                                                        __func__);
        return;
    }
    pthread_mutex_lock(&g_client_list_lock);
    client_handle->binder = client_binder;
    client_handle->pid = pid;
    client_handle->Client_death_notifier = Client_death_notifier;
    list_add_tail(&g_client_list, &client_handle->list);
    list_init(&client_handle->agm_client_hndl_list);
    pthread_mutex_unlock(&g_client_list_lock);
    
}

void agm_add_session_obj_handle(uint64_t handle)
{
    client_info *client_handle = NULL;
    agm_client_session_handle *hndl = NULL;

    client_handle =
          get_client_handle_from_list(IPCThreadState::self()->getCallingPid());
    if (client_handle == NULL) {
        AGM_LOGE("%s: Could not find client handle\n", __func__);
        goto exit;
    }

    pthread_mutex_lock(&g_client_list_lock);
    hndl = (agm_client_session_handle *)calloc(1,
                                            sizeof(agm_client_session_handle));
    if (hndl == NULL) {
        AGM_LOGE("%s: Cannot allocate memory to store agm session handle\n",
                                                                  __func__);
        goto exit;
    }
    hndl->handle = handle;
    list_add_tail(&client_handle->agm_client_hndl_list, &hndl->list);

exit:
    pthread_mutex_unlock(&g_client_list_lock);
}

void agm_remove_session_obj_handle(uint64_t handle)
{
    client_info *client_handle = NULL;
    struct listnode *node = NULL;
    agm_client_session_handle *hndl = NULL;
    struct listnode *tempnode = NULL;

    client_handle =
          get_client_handle_from_list(IPCThreadState::self()->getCallingPid());
    if (client_handle == NULL) {
        AGM_LOGE("%s: Could not find client handle\n", __func__);
        return;
    }

    pthread_mutex_lock(&g_client_list_lock);
    list_for_each_safe(node, tempnode, &client_handle->agm_client_hndl_list) {
        hndl = node_to_item(node, agm_client_session_handle, list);
        if (hndl->handle == handle) {
            AGM_LOGV("%s: Removed handle 0x%llx\n", __func__, handle);
            list_remove(node);
            free(hndl);
            break;
        }
    }
    pthread_mutex_unlock(&g_client_list_lock);
}

void agm_unregister_client(sp<IBinder> binder)
{
    android::sp<IAGMClient> client_binder =
                                   android::interface_cast<IAGMClient>(binder);
    client_info *handle = NULL;
    struct listnode *tempnode = NULL;
    struct listnode *node = NULL;

    AGM_LOGV("%s: enter\n", __func__);
    pthread_mutex_lock(&g_client_list_lock);
    list_for_each_safe(node, tempnode, &g_client_list) {
        handle = node_to_item(node, client_info, list);
        if (handle->pid == IPCThreadState::self()->getCallingPid()) {
            if (handle->Client_death_notifier != NULL) {
                IInterface::asBinder(client_binder)->unlinkToDeath(handle->Client_death_notifier);
                handle->Client_death_notifier.clear();
                AGM_LOGV("%s: unlink to death %d\n", __func__, handle->pid);
            }
            list_remove(node);
            free(handle);
        }
    }
    AGM_LOGV("%s: exit\n", __func__);
    pthread_mutex_unlock(&g_client_list_lock);
}

void client_death_notifier::binderDied(const wp<IBinder>& who)
{
    client_info *handle = NULL;
    struct listnode *node = NULL;
    struct listnode *tempnode = NULL;
    agm_client_session_handle *hndl = NULL;
    struct listnode *sess_node = NULL;
    struct listnode *sess_tempnode = NULL;

    pthread_mutex_lock(&g_client_list_lock);
    list_for_each_safe(node, tempnode, &g_client_list) {
        handle = node_to_item(node, client_info, list);
        if (IInterface::asBinder(handle->binder).get() == who.unsafe_get()) {
            list_for_each_safe(sess_node, sess_tempnode,
                                             &handle->agm_client_hndl_list) {
                hndl = node_to_item(sess_node, agm_client_session_handle, list);
                   if (hndl->handle) {
                       agm_session_close(hndl->handle);
                       list_remove(sess_node);
                       free(hndl);
                   }
                }
                list_remove(node);
                free(handle);
        }
    }
    pthread_mutex_unlock(&g_client_list_lock);
    AGM_LOGD("%s: exit\n", __func__);
}

const android::String16 IAGMClient::descriptor("IAGMClient");
const android::String16& IAGMClient::getInterfaceDescriptor() const {
    return IAGMClient::descriptor;
}

class BpClient: public ::android:: BpInterface<IAGMClient>
{
public:
    BpClient(const sp<IBinder>& impl) : BpInterface<IAGMClient>(impl) {
        AGM_LOGD("BpClient::BpClient()\n");
    }
};

android::sp<IAGMClient> IAGMClient::asInterface
    (const android::sp<android::IBinder>& obj)
{
    AGM_LOGV("IAGMClient::asInterface()\n");
    android::sp<IAGMClient> intr;
    if (obj != NULL) {
        intr = static_cast<IAGMClient*>(obj->queryLocalInterface
                        (IAGMClient::descriptor).get());
        AGM_LOGD("IAGMClient::asInterface() interface %s\n",
            ((intr == 0)?"zero":"non zero"));
        if (intr == NULL)
            intr = new BpClient(obj);
    }
    return intr;
}
IAGMClient :: IAGMClient() { AGM_LOGD("IAGMClient::IAGMClient()"); }
IAGMClient :: ~IAGMClient() { AGM_LOGD("IAGMClient::~IAGMClient()"); }

int32_t DummyBnClient::onTransact(uint32_t code,
                                   const Parcel& data,
                                   Parcel* reply,
                                   uint32_t flags)
{
    int status = 0;
    AGM_LOGV("DummyBnClient::onTransact(%i) %i\n", code, flags);
    //data.checkInterface(this); //Alternate option to check interface
    CHECK_INTERFACE(IAGMClient, data, reply);
    switch(code) {
        default:
            return BBinder::onTransact(code, data, reply, flags);
            break;
    }
    return status;
}

