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

#include <stdlib.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <binder/TextOutput.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <cutils/list.h>
#include "utils.h"

using namespace android;

class IAGMClient : public ::android::IInterface
{
    public:
        DECLARE_META_INTERFACE(AGMClient);
};

class DummyBnClient : public ::android::BnInterface<IAGMClient>
{
    public:
        DummyBnClient(){
            AGM_LOGV("Constructor of DummyBnClient called\n");
        }
        ~DummyBnClient(){
            AGM_LOGV("Destructor of DummyBnClient called\n");
        }
    private:
        int32_t onTransact(uint32_t code,
                           const Parcel& data,
                           Parcel* reply,
                           uint32_t flags) override;
};


class server_death_notifier : public IBinder::DeathRecipient
{
    public:
        server_death_notifier();
        // DeathRecipient
        virtual void binderDied(const android::wp<IBinder>& who);
};

class client_death_notifier : public IBinder::DeathRecipient
{
    public:
        client_death_notifier();
        // DeathRecipient
        virtual void binderDied(const android::wp<IBinder>& who);
};

typedef struct {
     struct listnode list;
     uint64_t handle;
     //bool rx;
 } agm_client_session_handle;

typedef struct {
    struct listnode list;
    sp<IAGMClient> binder;
    pid_t pid;
    sp<client_death_notifier> Client_death_notifier;
    struct listnode agm_client_hndl_list;
} client_info;

client_info *get_client_handle_from_list(pid_t pid);
void agm_register_client(sp<IBinder> binder);
void agm_unregister_client(sp<IBinder> binder);
void agm_add_session_obj_handle(uint64_t handle);
void agm_remove_session_obj_handle(uint64_t handle);
