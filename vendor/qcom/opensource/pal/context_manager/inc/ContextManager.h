/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef CONTEXTMANAGER_H
#define CONTEXTMANAGER_H

#include <vector>
#include <thread>
#include <queue>
#include <condition_variable>
#include <PalApi.h>
#include <PalCommon.h>
#include "kvh2xml.h"
#include "ACDPlatformInfo.h"
#include "SoundTriggerPlatformInfo.h"

enum PCM_DATA_EFFECT {
    PCM_DATA_EFFECT_RAW = 1,
    PCM_DATA_EFFECT_NS = 2,
};

class ContextManager; /* forward declaration for RequestCommand */
class ACDPlatformInfo;
using ACDUUID = SoundTriggerUUID;

class Usecase
{
protected:
    uint32_t usecase_id;
    uint32_t no_of_devices;
    pal_device *pal_devices;
    pal_stream_attributes *stream_attributes;
    pal_stream_handle_t *pal_stream;

public:
    Usecase(uint32_t usecase_id);
    virtual ~Usecase();
    uint32_t GetUseCaseID();
    int32_t Open();
    int32_t Start();
    int32_t StopAndClose();

    int32_t GetModuleIIDs(std::vector<int32_t> tags,
        std::map<int32_t, std::vector<uint32_t>> &tag_miid_map);

    //virtual fucntions
    virtual int32_t SetUseCaseData(uint32_t size, void *data);
    virtual int32_t Configure();

    // caller should allocate sufficient memory the first time to avoid
    // calling this api twice. Size, will be updated to actual size;
    virtual int32_t GetAckDataOnSuccessfullStart(uint32_t *size, void *data) = 0;
};

class UsecaseACD :public Usecase
{
private:
    struct pal_param_context_list *requested_context_list;
    std::vector<int32_t> tags;

public:
    UsecaseACD(uint32_t usecase_id);
    ~UsecaseACD();
    int32_t SetUseCaseData(uint32_t size, void *data);
    int32_t Configure();

    // caller can allocate sufficient memory the first time to avoid
    // calling this api twice. Size, will be updated to actual size;
    int32_t GetAckDataOnSuccessfullStart(uint32_t *size, void *data);

    //static functions
    // caller should allocate sufficient memory the first time to avoid
    // calling this api twice. Size, will be updated to actual size;
    static int32_t GetSupportedContextIDs(std::vector<uint32_t> &context_ids);
    static std::shared_ptr<ACDPlatformInfo> GetAcdPlatformInfo();
};

class UsecaseUPD : public Usecase
{
private:
    std::vector<int32_t> tags;
public:
    UsecaseUPD(uint32_t usecase_id);
    ~UsecaseUPD();

    // caller can allocate sufficient memory the first time to avoid
    // calling this api twice. Size, will be updated to actual size;
    int32_t GetAckDataOnSuccessfullStart(uint32_t *size, void *data);
};

class UsecasePCMData : public Usecase
{
private:
    std::vector<int32_t> tags;
    uint32_t pcm_data_type;
    uint32_t pcm_data_buffering;
public:
    UsecasePCMData(uint32_t usecase_id);
    ~UsecasePCMData();
    int32_t SetUseCaseData(uint32_t size, void *data);
    int32_t Configure();

    // caller can allocate sufficient memory the first time to avoid
    // calling this api twice. Size, will be updated to actual size;
    int32_t GetAckDataOnSuccessfullStart(uint32_t *size, void *data);
};

class UsecasePCMRenderer : public Usecase
{
private:
    std::vector<int32_t> tags;
public:
    UsecasePCMRenderer(uint32_t usecase_id);
    ~UsecasePCMRenderer();
    int32_t SetUseCaseData(uint32_t size, void *data);
    // caller can allocate sufficient memory the first time to avoid
    // calling this api twice. Size, will be updated to actual size;
    int32_t GetAckDataOnSuccessfullStart(uint32_t *size, void *data);
};

class UsecaseFactory
{
public:
    static Usecase* UsecaseCreate(int32_t usecase_id);
};

class see_client
{
private:
    uint32_t see_id;
    std::map<uint32_t, Usecase*> usecases;

protected:
    static std::mutex see_client_mutex;

public:
    see_client(uint32_t id);
    ~see_client();
    uint32_t Get_SEE_ID();
    Usecase* Usecase_Get(uint32_t usecase_id);
    int32_t Usecase_Remove(uint32_t usecase_id);
    int32_t Usecase_Add(uint32_t usecase_id, Usecase* uc);
    void lock_see_client() { see_client_mutex.lock(); };
    void unlock_see_client() { see_client_mutex.unlock(); };
    void CloseAllUsecases();
};

class RequestCommand {
public:
    RequestCommand(uint32_t event_id, uint32_t *event_data);
    virtual ~RequestCommand();

    virtual int32_t Process(ContextManager& cm) = 0;
};

class CommandRegister : public RequestCommand {
public:
    CommandRegister(uint32_t event_id, uint32_t* event_data);
    ~CommandRegister();

    int32_t Process(ContextManager& cm);
private:
    uint32_t see_sensor_iid;
    uint32_t usecase_id;
    uint32_t payload_size;
    uint32_t *payload;
};

class CommandDeregister : public RequestCommand {
public:
    CommandDeregister(uint32_t event_id, uint32_t* event_data);
    int32_t Process(ContextManager& cm);
private:
    uint32_t see_sensor_iid;
    uint32_t usecase_id;
};

class CommandGetContextIDs : public RequestCommand {
public:
    CommandGetContextIDs(uint32_t event_id, uint32_t* event_data);
    int32_t Process(ContextManager& cm);
private:
    uint32_t see_sensor_iid;
};

class CommandCloseAll : public RequestCommand {
public:
    CommandCloseAll(uint32_t event_id, uint32_t* event_data);
    int32_t Process(ContextManager& cm);
};

class RequestCommandFactory
{
public:
    static RequestCommand *RequestCommandCreate(uint32_t event_id,
        uint32_t* event_data);
};

class ContextManager
{
private:
    std::map<uint32_t, see_client *> see_clients;
    pal_stream_handle_t *proxy_stream;
    bool exit_cmd_thread_;
    std::condition_variable request_queue_cv;
    std::mutex request_queue_mtx;
    std::thread cmd_thread_;
    std::queue<RequestCommand *> request_cmd_queue;

    see_client* SEE_Client_CreateIf_And_Get(uint32_t see_id);
    see_client * SEE_Client_Get_Existing(uint32_t see_id);
    int32_t OpenAndStartProxyStream();
    int32_t StopAndCloseProxyStream();
    int32_t CreateCommandProcessingThread();
    void DestroyCommandProcessingThread();
    void CloseAll();
    static void CommandThreadRunner(ContextManager& cm);
    int32_t build_and_send_register_ack(Usecase *uc, uint32_t see_id, uint32_t uc_id);

public:
    //functions
    ContextManager();
    ~ContextManager();
    int32_t Init();
    void DeInit();
    static int32_t StreamProxyCallback(pal_stream_handle_t *stream_handle,
                                   uint32_t event_id, uint32_t *event_data,
                                   uint32_t event_size, uint64_t cookie);
    int32_t ssrDownHandler();
    int32_t ssrUpHandler();
    int32_t process_deregister_request(uint32_t see_id, uint32_t usecase_id);
    int32_t process_register_request(uint32_t see_id, uint32_t usecase, uint32_t payload_size,
        void *payload);
    int32_t process_close_all();

    int32_t send_asps_response(uint32_t param_id, pal_param_payload *payload);
    int32_t send_asps_basic_response(int32_t status, uint32_t event_id, uint32_t see_id);

    static ACDUUID GetUUID();
};

#endif /*CONTEXTMANAGER_H*/
