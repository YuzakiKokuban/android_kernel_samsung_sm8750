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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <chrono>
#include "ContextManager.h"
#include <asps/asps_acm_api.h>
#include <asps/asps_us_rendering_usecase_api.h>
#include "apm_api.h"

#define LOG_TAG "PAL: ContextManager"
#define TAG_MODULE_DEFAULT_SIZE 1024
#define ACKDATA_DEFAULT_SIZE 1024
#define PAL_ALIGN_8BYTE(x) (((x) + 7) & (~7))

std::mutex see_client::see_client_mutex;

int32_t ContextManager::process_register_request(uint32_t see_id, uint32_t usecase_id, uint32_t size,
    void *payload)
{
    int32_t rc = 0;
    Usecase *uc = NULL;
    see_client *seeclient = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter see_id:%d, usecase_id:0x%x, payload_size:%d", see_id, usecase_id, size);

    seeclient = SEE_Client_CreateIf_And_Get(see_id);
    if (seeclient == NULL) {
        rc = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d, Failed to get see_client:%d", rc, see_id);
        goto exit;
    }

    seeclient->lock_see_client();

    uc = seeclient->Usecase_Get(usecase_id);
    if (uc == NULL) {
        PAL_VERBOSE(LOG_TAG, "Creating new usecase:0x%x for see_id:%d", usecase_id, see_id);

        uc = UsecaseFactory::UsecaseCreate(usecase_id);
        if (uc == NULL) {
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "Error:%d, Failed to create usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            goto exit;
        }

        rc = uc->SetUseCaseData(size, payload);
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d, Failed to Setusecase() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            goto exit;
        }

        rc = uc->Open();
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d, Failed to Open() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            goto exit;
        }

        rc = uc->Configure();
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d, Failed to Configure() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            uc->StopAndClose();
            goto exit;
        }

        rc = uc->Start();
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d, Failed to Start() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            uc->StopAndClose();
            goto exit;
        }

        seeclient->Usecase_Add(usecase_id, uc);
    } else {
        /* ASPS can send a request for an already running usecase with updated context ids lists. Configure()
         * to send the new list to the stream.
         */
        PAL_VERBOSE(LOG_TAG, "Retrieving existing usecase:0x%x for see_id:%d", usecase_id, see_id);
        rc = uc->SetUseCaseData(size, payload);
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d, Failed to Setusecase() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            goto exit;
        }
        rc = uc->Configure();
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d, Failed to Configure() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
            uc->StopAndClose();
            goto exit;
        }
    }

    rc = build_and_send_register_ack(uc, see_id, usecase_id);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d, Failed to get AckData for usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
        goto exit;
    }

exit:
    // send basic ack with failure.
    if (rc) {
        send_asps_basic_response(rc, EVENT_ID_ASPS_SENSOR_REGISTER_REQUEST, see_id);
    }
    seeclient->unlock_see_client();
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t ContextManager::build_and_send_register_ack(Usecase *uc, uint32_t see_id, uint32_t uc_id)
{
    int32_t rc = 0;
    pal_param_payload *pal_param = NULL;
    apm_module_param_data_t *param_data = NULL;
    param_id_asps_sensor_register_ack_t *ack;
    uint32_t ack_payload_size = ACKDATA_DEFAULT_SIZE;
    uint32_t payload_size = 0;
    void *ack_payload = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter seeid: %d ucid:0x%x", see_id, uc_id);

    ack_payload = calloc(1, ack_payload_size);
    if (!ack_payload) {
        rc = -ENOMEM;
        goto exit;
    }
    rc = uc->GetAckDataOnSuccessfullStart(&ack_payload_size, ack_payload);
    if (rc == ENODATA) {
        free(ack_payload);
        ack_payload = calloc(1, ack_payload_size);
        if (!ack_payload) {
            rc = -ENOMEM;
            goto exit;
        }
        rc = uc->GetAckDataOnSuccessfullStart(&ack_payload_size, ack_payload);
    }
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d, Failed to get AckData for usecase:0x%x for see_client:%d",
                rc, uc_id, see_id);
        goto exit;
    }

    payload_size = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t) +
                            sizeof(struct param_id_asps_sensor_register_ack_t) + ack_payload_size);
    pal_param = (pal_param_payload *)calloc(1, sizeof(pal_param_payload) + payload_size);
    //malloc response
    if (!pal_param) {
        rc = -ENOMEM;
        goto exit;
    }

    param_data = (apm_module_param_data_t*)pal_param->payload;

    param_data->module_instance_id = ASPS_MODULE_INSTANCE_ID;
    param_data->param_size = PAL_ALIGN_8BYTE(sizeof(struct param_id_asps_sensor_register_ack_t) + ack_payload_size);
    param_data->param_id = PARAM_ID_ASPS_SENSOR_REGISTER_ACK;
    param_data->error_code = 0x0;

    ack = (struct param_id_asps_sensor_register_ack_t*)(pal_param->payload + sizeof(struct apm_module_param_data_t));
    memcpy(ack->payload, ack_payload, ack_payload_size);

    ack->see_sensor_iid = see_id;
    ack->usecase_id = uc_id;
    ack->payload_size = ack_payload_size;

    pal_param->payload_size = payload_size;

    rc = send_asps_response(PAL_PARAM_ID_MODULE_CONFIG, pal_param);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d sending register ack opcode %x",
            rc, PARAM_ID_ASPS_SENSOR_REGISTER_ACK);
        goto exit;
    }
exit:
    if (ack_payload)
        free(ack_payload);
    if (pal_param)
        free(pal_param);
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t ContextManager::process_deregister_request(uint32_t see_id, uint32_t usecase_id)
{
    int32_t rc = 0;
    Usecase *uc = NULL;
    see_client *seeclient = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter see_id:%d, usecase_id:0x%x", see_id, usecase_id);

    seeclient = SEE_Client_Get_Existing(see_id);
    if (seeclient == NULL) {
        rc = -1; //EFAILED;
        PAL_ERR(LOG_TAG, "Error:%d, Failed to get see_client:%d", rc, see_id);
        goto exit;
    }

    seeclient->lock_see_client();

    uc = seeclient->Usecase_Get(usecase_id);
    if (uc == NULL) {
        rc = -EINVAL;
        PAL_VERBOSE(LOG_TAG, "Error:%d Retrieving existing usecase:0x%x for see_id:%d", rc, usecase_id, see_id);
        goto exit;
    }

    rc = uc->StopAndClose();
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d, Failed to StopAndClose() usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
        rc = 0; //force rc to success as nothing can be done if teardown fails.
    }

    rc = seeclient->Usecase_Remove(usecase_id);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d, Failed to remove usecase:0x%x for see_client:%d", rc, usecase_id, see_id);
        rc = 0; //force rc to success as nothing can be done if teardown fails.
    }

exit:
    seeclient->unlock_see_client();
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t ContextManager::process_close_all()
{
    PAL_VERBOSE(LOG_TAG, "Enter:");

    this->CloseAll();

    PAL_VERBOSE(LOG_TAG, "Exit: rc:%d", 0);

    return 0;
}

int32_t ContextManager::send_asps_response(uint32_t param_id, pal_param_payload* payload)
{
    int32_t rc = 0;
    PAL_VERBOSE(LOG_TAG, "Enter:");

    rc = pal_stream_set_param(this->proxy_stream, param_id, payload);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d setting params on proxy stream for param %d", rc, param_id);
    }

    PAL_VERBOSE(LOG_TAG, "Exit: rc:%d", rc);
    return rc;
}

/* sends a basic ack for the event_id which is passed in */
int32_t ContextManager::send_asps_basic_response(int32_t status, uint32_t event_id, uint32_t see_id)
{
    int32_t rc = 0;
    pal_param_payload *pal_param;
    apm_module_param_data_t *param_data;
    struct param_id_asps_basic_ack_t *ack;

    PAL_VERBOSE(LOG_TAG, "Enter:");

    pal_param = (pal_param_payload *) calloc (1, sizeof(pal_param_payload) + sizeof(struct apm_module_param_data_t)
        + sizeof(struct param_id_asps_basic_ack_t));
    if (!pal_param) {
        rc = -ENOMEM;
        goto exit;
    }

    param_data = (apm_module_param_data_t*)pal_param->payload;

    param_data->module_instance_id = ASPS_MODULE_INSTANCE_ID;
    param_data->param_size = PAL_ALIGN_8BYTE(sizeof(struct param_id_asps_basic_ack_t));
    param_data->param_id = PARAM_ID_ASPS_BASIC_ACK;

    ack = (struct param_id_asps_basic_ack_t *)(pal_param->payload + sizeof(*param_data));
    ack->asps_event_id = event_id;
    ack->error_code = status; //todo: translate to AR_error_code
    ack->see_sensor_iid = see_id;

    pal_param->payload_size = PAL_ALIGN_8BYTE(sizeof(struct param_id_asps_basic_ack_t) + sizeof(struct apm_module_param_data_t));

    rc = pal_stream_set_param(this->proxy_stream, PAL_PARAM_ID_MODULE_CONFIG, pal_param);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d setting params on proxy stream for basick ack", rc);
    }

exit:
    if (pal_param)
        free(pal_param);
    PAL_VERBOSE(LOG_TAG, "Exit: rc:%d", rc);
    return rc;
}

ContextManager::ContextManager()
{
    PAL_VERBOSE(LOG_TAG, "Enter");
    PAL_VERBOSE(LOG_TAG, "Exit");
}

ContextManager::~ContextManager()
{
    PAL_VERBOSE(LOG_TAG, "Enter");
    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t ContextManager::Init()
{
    int32_t rc = 0;
    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = OpenAndStartProxyStream();
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to OpenAndStartProxyStream", rc);
        goto exit;
    }

    rc = CreateCommandProcessingThread();
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to CreateCommandProcessingThread", rc);
        goto stop_and_close_proxy_stream;
    }

    goto exit;

stop_and_close_proxy_stream:
    StopAndCloseProxyStream();

exit:
    PAL_VERBOSE(LOG_TAG, "Exit rc %d", rc);
    return rc;
}

void ContextManager::DeInit()
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    CloseAll();
    StopAndCloseProxyStream();
    DestroyCommandProcessingThread();

    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t ContextManager::ssrDownHandler()
{
    int32_t rc = 0;
    std::unique_lock<std::mutex> lck(request_queue_mtx, std::defer_lock);
    PAL_VERBOSE(LOG_TAG, "Enter");

    this->CloseAll();

    lck.lock();
    while (!request_cmd_queue.empty()) {
        free(request_cmd_queue.front());
        request_cmd_queue.pop();
    }
    lck.unlock();

    PAL_VERBOSE(LOG_TAG, "Exit rc %d", rc);
    return rc;
}

int32_t ContextManager::ssrUpHandler()
{
    int32_t rc = 0;
    PAL_VERBOSE(LOG_TAG, "Enter");

    PAL_VERBOSE(LOG_TAG, "Exit rc %d", rc);
    return rc;
}

int32_t ContextManager::StreamProxyCallback (pal_stream_handle_t *stream_handle,
               uint32_t event_id, uint32_t *event_data, uint32_t event_size, uint64_t cookie)
{
    RequestCommand *request_command;
    ContextManager* cm = ((ContextManager*)cookie);

    PAL_VERBOSE(LOG_TAG, "Enter");
    std::unique_lock<std::mutex> lck(cm->request_queue_mtx);
    request_command = RequestCommandFactory::RequestCommandCreate(event_id, event_data);
    cm->request_cmd_queue.push(request_command);
    cm->request_queue_cv.notify_one();

    PAL_VERBOSE(LOG_TAG, "Exit");
    return 0;
}

void ContextManager::CloseAll()
{
    std::map<uint32_t, see_client*>::iterator it_see_client;
    see_client *see = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter");
    for (auto it_see_client = this->see_clients.begin(); it_see_client != this->see_clients.cend();) {
        see = it_see_client->second;
        PAL_VERBOSE(LOG_TAG, "Calling CloseAllUsecases for see_client:%d", see->Get_SEE_ID());
        see->CloseAllUsecases();
        see_clients.erase(it_see_client++);
        delete see;
    }

    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t ContextManager::OpenAndStartProxyStream()
{
    int32_t rc = 0;
    struct pal_stream_attributes stream_attr;

    PAL_VERBOSE(LOG_TAG, "Enter");

    memset(&stream_attr, 0, sizeof(struct pal_stream_attributes));
    stream_attr.type = PAL_STREAM_CONTEXT_PROXY;
    stream_attr.direction = PAL_AUDIO_INPUT;

    rc = pal_stream_open(&stream_attr, 0, NULL, 0, NULL,
                         (pal_stream_callback)&ContextManager::StreamProxyCallback,(uint64_t)this, &this->proxy_stream);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to open() proxy stream", rc);
        goto exit;
    }

    rc = pal_stream_start(this->proxy_stream);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to start() proxy stream", rc);
        goto close_stream;
    }

    goto exit;

close_stream:
    pal_stream_close(this->proxy_stream);;
    this->proxy_stream = NULL;

exit:
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t ContextManager::StopAndCloseProxyStream()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter");

    if (this->proxy_stream) {
        rc = pal_stream_stop(this->proxy_stream);
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d Failed to stop() proxy stream", rc);
        }

        rc = pal_stream_close(this->proxy_stream);
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d Failed to close() proxy stream", rc);
        }

        this->proxy_stream = NULL;
    }

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

void ContextManager::CommandThreadRunner(ContextManager& cm)
{
    RequestCommand *request_command;
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Entering CommandThreadRunner");

    std::unique_lock<std::mutex> lck(cm.request_queue_mtx);
    while (!cm.exit_cmd_thread_) {
        // wait until we have a command to process.
        if (cm.request_cmd_queue.empty()) {
            cm.request_queue_cv.wait(lck);
            if (cm.exit_cmd_thread_) {
                PAL_DBG(LOG_TAG, "Received exit request");
                continue;
            }
        }

        if (!cm.request_cmd_queue.empty()) {
            request_command = cm.request_cmd_queue.front();
            cm.request_cmd_queue.pop();

            rc = request_command->Process(cm);
            if (rc) {
                PAL_ERR(LOG_TAG, "Error:%d failed to process request", rc);
            }

            free(request_command);
        }
    }
    PAL_VERBOSE(LOG_TAG, "Exiting CommandThreadRunner");
}

int32_t ContextManager::CreateCommandProcessingThread()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter");

    exit_cmd_thread_ = false;
    cmd_thread_ = std::thread(CommandThreadRunner, std::ref(*this));

    PAL_VERBOSE(LOG_TAG, "Exit rc: %d", rc);
    return rc;
}

void ContextManager::DestroyCommandProcessingThread()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter");

    exit_cmd_thread_ = true;
    request_queue_cv.notify_all();

    if (cmd_thread_.joinable()) {
        PAL_DBG(LOG_TAG, "Join cmd_thread_ thread");
        cmd_thread_.join();
    }

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
}

ACDUUID ContextManager::GetUUID() {

    ACDUUID default_ACDUUID;

    SoundTriggerUUID::StringToUUID("4e93281b-296e-4d73-9833-2710c3c7c1db", default_ACDUUID);

    return default_ACDUUID;
}

RequestCommand *RequestCommandFactory::RequestCommandCreate(uint32_t event_id,
    uint32_t* event_data)
{
    RequestCommand *rq = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter");
    switch (event_id) {
    case EVENT_ID_ASPS_SENSOR_REGISTER_REQUEST:
        rq = new CommandRegister(event_id, event_data);
        break;
    case EVENT_ID_ASPS_SENSOR_DEREGISTER_REQUEST:
        rq = new CommandDeregister(event_id, event_data);
        break;
    case EVENT_ID_ASPS_GET_SUPPORTED_CONTEXT_IDS:
        rq = new CommandGetContextIDs(event_id, event_data);
        break;
    case EVENT_ID_ASPS_CLOSE_ALL:
        rq = new CommandCloseAll(event_id, event_data);
        break;
    default:
        PAL_ERR(LOG_TAG, "Unknown eventID %d", event_id);
    }

    PAL_VERBOSE(LOG_TAG, "Exit");
    return rq;
}

RequestCommand::RequestCommand(uint32_t event_id, uint32_t* event_data)
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    PAL_VERBOSE(LOG_TAG, "Exit");
}

RequestCommand::~RequestCommand()
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    PAL_VERBOSE(LOG_TAG, "Exit");
}

CommandRegister::CommandRegister(uint32_t event_id, uint32_t* event_data) :
    RequestCommand(event_id, event_data)
{
    event_id_asps_sensor_register_request_t *data =
        (event_id_asps_sensor_register_request_t*)event_data;
    PAL_VERBOSE(LOG_TAG, "Enter");

    this->payload_size = data->payload_size;
    this->usecase_id = data->usecase_id;
    this->see_sensor_iid = data->see_sensor_iid;

    this->payload = (uint32_t *) calloc (1, this->payload_size);
    if (!this->payload) {
        PAL_ERR(LOG_TAG, "Error: %d failed to alloc memory for register command payload", -ENOMEM);
        goto exit;
    }
    memcpy(this->payload, data->payload, this->payload_size);

exit:
    PAL_VERBOSE(LOG_TAG, "Exit");
}

CommandRegister::~CommandRegister()
{
    PAL_VERBOSE(LOG_TAG, "Enter");
    free(this->payload);
    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t CommandRegister::Process(ContextManager& cm)
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = cm.process_register_request(this->see_sensor_iid, this->usecase_id,
        this->payload_size, this->payload);
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

CommandDeregister::CommandDeregister(uint32_t event_id, uint32_t* event_data) :
    RequestCommand(event_id, event_data)
{
    event_id_asps_sensor_deregister_request_t *data =
        (event_id_asps_sensor_deregister_request_t*)event_data;

    PAL_VERBOSE(LOG_TAG, "Enter");

    this->see_sensor_iid = data->see_sensor_iid;
    this->usecase_id = data->usecase_id;

    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t CommandDeregister::Process(ContextManager& cm)
{
    int32_t rc = 0;
    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = cm.process_deregister_request(this->see_sensor_iid, this->usecase_id);
    if (rc) {
        PAL_ERR(LOG_TAG, "deregister request failed %d", rc);
    }

    // we send basic response in success and failure case.

    cm.send_asps_basic_response(rc, EVENT_ID_ASPS_SENSOR_DEREGISTER_REQUEST,
        this->see_sensor_iid);

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

CommandGetContextIDs::CommandGetContextIDs(uint32_t event_id, uint32_t* event_data) :
    RequestCommand(event_id, event_data)
{
    event_id_asps_get_supported_context_ids_t *data =
        (event_id_asps_get_supported_context_ids_t*)event_data;

    PAL_VERBOSE(LOG_TAG, "Enter");

    this->see_sensor_iid = data->see_sensor_iid;

    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t CommandGetContextIDs::Process(ContextManager& cm)
{
    uint32_t num_contexts;
    int32_t rc = 0;
    struct param_id_asps_supported_context_ids_t *response;
    std::vector<uint32_t> context_ids;
    apm_module_param_data_t *param_data = NULL;
    pal_param_payload *pal_param = NULL;
    size_t payload_size = 0;

    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = UsecaseACD::GetSupportedContextIDs(context_ids);
    if (rc) {
        PAL_ERR(LOG_TAG,"Failed to GetSupportedContextIDs, Error:%d", rc);
        goto exit;
    }

    num_contexts = context_ids.size();
    payload_size = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t) +
                                   sizeof(struct param_id_asps_supported_context_ids_t) +
                                   sizeof(uint32_t) * (num_contexts));
    pal_param = (pal_param_payload *) calloc(1, sizeof(pal_param_payload) + payload_size);
    if (!pal_param) {
        rc = -ENOMEM;
        goto exit;
    }
    pal_param->payload_size = payload_size;

    param_data = (apm_module_param_data_t*)pal_param->payload;
    param_data->param_size = PAL_ALIGN_8BYTE(sizeof(struct param_id_asps_supported_context_ids_t) +
                                               (sizeof(uint32_t) * (num_contexts)));
    param_data->module_instance_id = ASPS_MODULE_INSTANCE_ID;
    param_data->param_id = PARAM_ID_ASPS_SUPPORTED_CONTEXT_IDS;

    response = (struct param_id_asps_supported_context_ids_t*)(pal_param->payload +
                                                  sizeof(struct apm_module_param_data_t));
    response->see_sensor_iid = this->see_sensor_iid;
    response->num_supported_contexts = num_contexts;

    memcpy((void *)response->supported_context_ids, context_ids.data(),
        sizeof(uint32_t) * num_contexts);

    rc = cm.send_asps_response(PAL_PARAM_ID_MODULE_CONFIG, pal_param);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d sending supported context IDs response opcode %x ",
            rc, PARAM_ID_ASPS_SUPPORTED_CONTEXT_IDS);
        goto exit;
    }

exit:
    if (pal_param)
        free(pal_param);

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

CommandCloseAll::CommandCloseAll(uint32_t event_id, uint32_t* event_data) :
    RequestCommand(event_id, event_data)
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t CommandCloseAll::Process(ContextManager& cm)
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    cm.process_close_all();

    //no failure case -- cannot recover
    PAL_VERBOSE(LOG_TAG, "Exit");
    return 0;
}

see_client::see_client(uint32_t id)
{
    PAL_VERBOSE(LOG_TAG, "Enter seeid:%d", id);

    this->see_id = id;

    PAL_VERBOSE(LOG_TAG, "Exit ");
}

see_client::~see_client()
{
    PAL_VERBOSE(LOG_TAG, "Enter seeid:%d", this->see_id);

    // ACM should close all active usecases before calling destructor.
    usecases.clear();

    PAL_VERBOSE(LOG_TAG, "Exit ");
}

see_client* ContextManager::SEE_Client_CreateIf_And_Get(uint32_t see_id)
{
    std::map<uint32_t, see_client*>::iterator it;
    see_client* client = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter seeid:%d", see_id);

    it = see_clients.find(see_id);
    if (it != see_clients.end()) {
        client = it->second;
    } else {
        client = new see_client(see_id);
        see_clients.insert(std::make_pair(see_id, client));
    }

    PAL_VERBOSE(LOG_TAG, "Exit");

    return client;
}

see_client* ContextManager::SEE_Client_Get_Existing(uint32_t see_id)
{
    std::map<uint32_t, see_client*>::iterator it;
    see_client* client = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter seeid:%d", see_id);

    it = see_clients.find(see_id);
    if (it != see_clients.end()) {
        client = it->second;
    }
    else {
        PAL_ERR(LOG_TAG, "Error:%d see_id:%d doesnt exist", -EINVAL, see_id);
    }

    PAL_VERBOSE(LOG_TAG, "Exit");
    return client;
}

uint32_t see_client::Get_SEE_ID()
{
    PAL_VERBOSE(LOG_TAG, "Enter");
    PAL_VERBOSE(LOG_TAG, "Exit");
    return this->see_id;
}

Usecase* see_client::Usecase_Get(uint32_t usecase_id)
{
    std::map<uint32_t, Usecase*>::iterator it;
    Usecase* uc = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter usecase_id:0x%x", usecase_id);

    it = usecases.find(usecase_id);
    if (it != usecases.end()) {
        uc = it->second;
    }

    PAL_VERBOSE(LOG_TAG, "Exit");
    return uc;
}

int32_t see_client::Usecase_Remove(uint32_t usecase_id)
{
    int32_t rc = 0;
    std::map<uint32_t, Usecase*>::iterator it;
    Usecase* uc = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter usecase_id:0x%x", usecase_id);

    it = usecases.find(usecase_id);
    if (it != usecases.end()) {
        PAL_VERBOSE(LOG_TAG, "Found usecase_id:0x%x", usecase_id);
        uc = it->second;

        //remove from usecase map
        usecases.erase(it);

        //delete uc, expectation is that the UC has been stopped and closed.
        if (uc)
            delete uc;
    }
    else {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Cannot find Found usecase_id:0x%x", rc, usecase_id);
    }

    PAL_VERBOSE(LOG_TAG, "Exit: rc:%d", rc);

    return rc;
}

int32_t see_client::Usecase_Add(uint32_t usecase_id, Usecase* uc)
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter: usecase_id:0x%x", usecase_id);

    usecases.insert(std::make_pair(usecase_id, uc));

    PAL_VERBOSE(LOG_TAG, "Exit: rc:%d", rc);

    return rc;
}

void see_client::CloseAllUsecases()
{
    std::map<uint32_t, Usecase*>::iterator it_uc;
    Usecase *uc = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter:");

    see_client_mutex.lock();

    for (auto it_uc = this->usecases.begin(); it_uc != this->usecases.cend();) {
        uc = it_uc->second;
        PAL_VERBOSE(LOG_TAG, "Calling StopAndClose on usecase_id:0x%x", uc->GetUseCaseID());
        uc->StopAndClose();
        usecases.erase(it_uc++);
        delete uc;
    }

    see_client_mutex.unlock();

    PAL_VERBOSE(LOG_TAG, "Exit:");
}

Usecase* UsecaseFactory::UsecaseCreate(int32_t usecase_id)
{
    Usecase* ret_usecase = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter usecase_id:0x%x", usecase_id);

    try {
        switch (usecase_id) {
        case ASPS_USECASE_ID_ACD:
            ret_usecase = new UsecaseACD(usecase_id);
            break;
        case ASPS_USECASE_ID_PCM_DATA:
        case ASPS_USECASE_ID_PCM_DATA_V2:
            ret_usecase = new UsecasePCMData(usecase_id);
            break;
        case ASPS_USECASE_ID_UPD:
            ret_usecase = new UsecaseUPD(usecase_id);
            break;
        case ASPS_USECASE_ID_ULTRASOUND_RENDERING:
            ret_usecase = new UsecasePCMRenderer(usecase_id);
            break;
        default:
            ret_usecase = NULL;
            PAL_ERR(LOG_TAG, "Error:%d Invalid usecaseid:%d", -EINVAL, usecase_id);
            break;
        }
    }
    catch (std::exception &) {
        return NULL;
    }

    PAL_VERBOSE(LOG_TAG, "Exit:");
    return ret_usecase;
}

Usecase::Usecase(uint32_t usecase_id)
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    this->usecase_id = usecase_id;
    this->stream_attributes = (struct pal_stream_attributes *)
        calloc (1, sizeof(struct pal_stream_attributes));
    if (!this->stream_attributes) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for Usecase StreamAtrributes", -ENOMEM);
        throw std::runtime_error("Failed to allocate memory for Usecase StreamAtrributes");
    }
    PAL_VERBOSE(LOG_TAG, "Exit");
}

Usecase::~Usecase()
{
    PAL_VERBOSE(LOG_TAG, "Enter");

    if (this->stream_attributes) {
        free(this->stream_attributes);
        this->stream_attributes = NULL;
    }

    if (this->pal_devices) {
        free(this->pal_devices);
        this->pal_devices = NULL;
    }

    pal_stream = NULL;
    this->usecase_id = 0;
    this->no_of_devices = 0;

    PAL_VERBOSE(LOG_TAG, "Exit");
}

uint32_t Usecase::GetUseCaseID()
{
    return this->usecase_id;
}

int32_t Usecase::Open()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);

    rc = pal_stream_open(this->stream_attributes, this->no_of_devices, this->pal_devices, 0, NULL, NULL, 0, &(this->pal_stream));
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to Open() usecase:0x%x", rc, this->usecase_id);
    }

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);

    return rc;
}

int32_t Usecase::Configure()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);

    return rc;
}

int32_t Usecase::Start()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);

    rc = pal_stream_start(this->pal_stream);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to Start() usecase:0x%x", rc, this->usecase_id);
    }

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);

    return rc;
}

int32_t Usecase::StopAndClose()
{
    int32_t rc = 0;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);

    rc = pal_stream_stop(this->pal_stream);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to stop() usecase:0x%x", rc, this->usecase_id);
    }

    rc = pal_stream_close(this->pal_stream);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to Close() usecase:0x%x", rc, this->usecase_id);
    }

    rc = 0;
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);

    return rc;
}

int32_t Usecase::GetModuleIIDs(std::vector<int32_t> tags,
    std::map<int32_t, std::vector<uint32_t>>& tag_miid_map)
{
    int32_t rc = 0;
    size_t tag_module_size = TAG_MODULE_DEFAULT_SIZE;
    std::vector<uint8_t> tag_module_info(tag_module_size);
    struct pal_tag_module_info* tag_info;
    struct pal_tag_module_mapping* tag_entry;
    std::vector<uint32_t> miid_list;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);

    rc = pal_stream_get_tags_with_module_info(this->pal_stream,
        &tag_module_size, (uint8_t*)(tag_module_info.data()));
    if (rc == ENODATA) {
        tag_module_info.resize(tag_module_size);

        rc = pal_stream_get_tags_with_module_info(this->pal_stream,
            &tag_module_size, (uint8_t*)tag_module_info.data());
    }
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d Could not retrieve tag module info from PAL", rc);
        goto exit;
    }

    tag_info = (struct pal_tag_module_info*)tag_module_info.data();
    if (!tag_info) {
        rc = -ENODATA;
        PAL_ERR(LOG_TAG, "Error:%d Could not retrieve tag module info from PAL", rc);
        goto exit;
    }
    tag_entry = (struct pal_tag_module_mapping*)&tag_info->pal_tag_module_list[0];
    if (!tag_entry) {
        rc = -ENODATA;
        PAL_ERR(LOG_TAG, "Error:%d Could not retrieve tag module info from PAL", rc);
        goto exit;
    }

    for (uint32_t idx = 0, offset = 0; idx < tag_info->num_tags; idx++) {
        tag_entry += offset / sizeof(struct pal_tag_module_mapping);

        offset = sizeof(struct pal_tag_module_mapping) +
            (tag_entry->num_modules * sizeof(struct module_info));

        //compare against all tags on the usecase
        if (std::find(tags.begin(), tags.end(), tag_entry->tag_id) != tags.end()) {
            for (uint32_t i = 0; i < tag_entry->num_modules; ++i) {
                PAL_VERBOSE(LOG_TAG, "tag_id:0x%x -> miid:0x%x",
                    tag_entry->mod_list[i].module_iid,tag_entry->tag_id);
                tag_miid_map[tag_entry->tag_id].push_back(tag_entry->mod_list[i].module_iid);
            }
        }
    }

    if (tag_miid_map.empty()) {
        rc = -ENODATA;
        PAL_ERR(LOG_TAG, "Error:%d no miids found for tag %x", rc, tags.front());
        goto exit;
    }

exit:
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t Usecase::SetUseCaseData(uint32_t size, void *data)
{
    int32_t rc = 0;
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

UsecaseACD::UsecaseACD(uint32_t usecase_id) : Usecase(usecase_id)
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", usecase_id);

    this->requested_context_list = NULL;
    this->stream_attributes->type = PAL_STREAM_ACD;
    this->no_of_devices = 1;
    this->pal_devices = (struct pal_device *) calloc (this->no_of_devices, sizeof(struct pal_device));
    if (!this->pal_devices) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for pal_devices",-ENOMEM);
        throw std::runtime_error("Failed to allocate memory for pal_devices");
    }

    //input device
    this->pal_devices[0].id = PAL_DEVICE_IN_HANDSET_VA_MIC;
    this->pal_devices[0].config.bit_width = 16;
    this->pal_devices[0].config.sample_rate = 16000;
    this->pal_devices[0].config.ch_info.channels = 1;

    this->tags.push_back(CONTEXT_DETECTION_ENGINE);

    PAL_VERBOSE(LOG_TAG, "Exit ");
}

UsecaseACD::~UsecaseACD()
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);
    if (this->requested_context_list) {
        free(this->requested_context_list);
        this->requested_context_list = NULL;
    }
    tags.clear();

    PAL_VERBOSE(LOG_TAG, "Exit ");
}

int32_t UsecaseACD::Configure()
{
    int32_t rc = 0;
    pal_param_payload *context_payload = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);

    context_payload = (pal_param_payload *) calloc (1, sizeof(pal_param_payload)
        + sizeof(uint32_t) * (1 + this->requested_context_list->num_contexts));
    if (!context_payload) {
        rc = -ENOMEM;
        goto exit;
    }
    context_payload->payload_size = sizeof(this->requested_context_list);
    memcpy(context_payload->payload, this->requested_context_list, sizeof(uint32_t) * (1 + this->requested_context_list->num_contexts));
    rc = pal_stream_set_param(this->pal_stream, PAL_PARAM_ID_CONTEXT_LIST, context_payload);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d setting parameters to stream usecase:0x%x", rc, this->usecase_id);
        goto exit;
    }

exit:
    if (context_payload)
        free(context_payload);

    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t UsecaseACD::GetAckDataOnSuccessfullStart(uint32_t *size __unused, void *data __unused)
{
    int32_t rc = 0;
    asps_acd_usecase_register_ack_payload_t *data_ptr;
    std::map<int32_t, std::vector<uint32_t>> tag_miid_map;

    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = GetModuleIIDs(this->tags, tag_miid_map);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d failed to get module iids", rc);
        goto exit;
    }
    /*
     * ASSUMPTION: There is only on MIID and one tag here
     * This will need to change later, and we will need a way of associating
     * miid with contextID, which we do not have now.
     */

    if (*size >= sizeof(asps_acd_usecase_register_ack_payload_t)
        + sizeof(uint32_t) * requested_context_list->num_contexts) {

        data_ptr = (asps_acd_usecase_register_ack_payload_t*)data;
        data_ptr->module_instance_id = tag_miid_map[this->tags.front()].front();
        data_ptr->num_contexts = requested_context_list->num_contexts;
        memcpy(data_ptr->context_ids, requested_context_list->context_id,
            sizeof(uint32_t) * requested_context_list->num_contexts);
    } else {
        rc = -ENODATA;
        PAL_ERR(LOG_TAG, "size %d too small for ack data in UsecaseACD, need %d",
            (int)(*size), sizeof(asps_acd_usecase_register_ack_payload_t)
            + sizeof(uint32_t) * requested_context_list->num_contexts);
    }

    *size = sizeof(asps_acd_usecase_register_ack_payload_t)
        + sizeof(uint32_t) * requested_context_list->num_contexts;

exit:
    PAL_VERBOSE(LOG_TAG, "Exit %d", rc);
    return rc;
}

int32_t UsecaseACD::GetSupportedContextIDs(std::vector<uint32_t>& context_ids)
{
    int32_t rc = 0;
    std::shared_ptr<ACDPlatformInfo> info;
    std::shared_ptr<ACDStreamConfig> stream_cfg;
    std::vector<std::shared_ptr<ACDSoundModelInfo>> models;

    PAL_VERBOSE(LOG_TAG, "Enter");

    info = UsecaseACD::GetAcdPlatformInfo();
    stream_cfg = info->GetStreamConfig(ContextManager::GetUUID());
    models = stream_cfg->GetSoundModelList();

    for (auto& model : models) {
        for (auto& context : model->GetSupportedContextList()) {
            context_ids.push_back(context->GetContextId());
        }
    }

    PAL_VERBOSE(LOG_TAG, "Exit %d", rc);
    return rc;
}

std::shared_ptr<ACDPlatformInfo> UsecaseACD::GetAcdPlatformInfo()
{
    PAL_VERBOSE(LOG_TAG, "Enter");
    PAL_VERBOSE(LOG_TAG, "Exit");
    return ACDPlatformInfo::GetInstance();
}

int32_t UsecaseACD::SetUseCaseData(uint32_t size, void *data)
{
    int rc = 0;
    int no_contexts = 0;
    uint32_t *ptr = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x, size:%d", this->usecase_id, size);

    if (!size || !data) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Invalid size:%d or data:%p for usecase:0x%x", rc, size, data, this->usecase_id);
        goto exit;
    }

    if (this->requested_context_list) {
        free(this->requested_context_list);
        this->requested_context_list = NULL;
    }

    ptr = (uint32_t *)data;
    no_contexts = (uint32_t)ptr[0];
    PAL_VERBOSE(LOG_TAG, "Number of contexts:%d for usecase:0x%x, size:%d", no_contexts, this->usecase_id, size);

    if (size < (sizeof(uint32_t) + (no_contexts * sizeof(uint32_t)))) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Insufficient data for no_contexts:%d and usecase:0x%x", rc, no_contexts, this->usecase_id);
        goto exit;
    }
    this->requested_context_list = (struct pal_param_context_list *) calloc (size, sizeof(uint8_t));
    if (!this->requested_context_list) {
        rc = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for context_list", rc);
        goto exit;
    }
    memcpy(this->requested_context_list, data, size);

exit:
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

UsecasePCMData::UsecasePCMData(uint32_t usecase_id) : Usecase(usecase_id)
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", usecase_id);

    this->stream_attributes->type = PAL_STREAM_SENSOR_PCM_DATA;
    this->stream_attributes->direction = PAL_AUDIO_INPUT;
    this->no_of_devices = 1;
    this->pal_devices = (struct pal_device *) calloc (this->no_of_devices, sizeof(struct pal_device));
    if (!this->pal_devices) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for pal_devices", -ENOMEM);
        throw std::runtime_error("Failed to allocate mem for pal_devices");
    }

    //input device
    this->pal_devices[0].id = PAL_DEVICE_IN_HANDSET_VA_MIC;
    this->pal_devices[0].config.bit_width = 16;
    this->pal_devices[0].config.sample_rate = 16000;
    this->pal_devices[0].config.ch_info.channels = 1;
    this->pal_devices[0].config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    this->tags.push_back(RD_SHMEM_ENDPOINT);
    this->tags.push_back(TAG_STREAM_MFC);
    this->tags.push_back(STREAM_PCM_CONVERTER);

    PAL_VERBOSE(LOG_TAG, "Exit");
}

UsecasePCMData::~UsecasePCMData()
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);
    //cleanup is done in baseclass
    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t UsecasePCMData::GetAckDataOnSuccessfullStart(uint32_t *size, void *data)
{
    int32_t rc = 0;
    uint32_t *data_ptr = (uint32_t *)data;
    size_t no_of_miid = 0;
    std::map<int32_t, std::vector<uint32_t>> tag_miid_map;

    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = GetModuleIIDs(this->tags, tag_miid_map);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d failed to get module iids", rc);
        goto exit;
    }

    //grab all miids for all tags in the order that tags exist in the UC vector
    for (auto tag : tags) {
        for (uint32_t miid : tag_miid_map[tag]) {
            *data_ptr = miid;
            ++data_ptr;
            ++no_of_miid;
        }
    }
    *size = (no_of_miid * sizeof(uint32_t));

exit:
    PAL_DBG(LOG_TAG, "Exit %d, number of MIID %d", rc, no_of_miid);
    return rc;
}

int32_t UsecasePCMData::Configure()
{
    int32_t rc = 0;
    pal_param_payload *pal_param = NULL;
    pal_audio_effect_t effect = PAL_AUDIO_EFFECT_NONE;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x, pcm_data_type:%d",
                this->usecase_id, this->pcm_data_type);

    if (this->usecase_id == ASPS_USECASE_ID_PCM_DATA_V2) {
        pal_param = (pal_param_payload *) calloc(1, sizeof(pal_param_payload) + sizeof(uint32_t));
        if (!pal_param) {
            rc = -ENOMEM;
            goto exit;
        }

        pal_param->payload_size = sizeof(uint32_t);
        memcpy(pal_param->payload, &(this->pcm_data_buffering), sizeof(uint32_t));
        rc = pal_stream_set_param(this->pal_stream, PAL_PARAM_ID_CUSTOM_CONFIGURATION, pal_param);
        if (rc) {
            PAL_ERR(LOG_TAG, "Error:%d setting parameters to stream usecase:0x%x", rc, this->usecase_id);
            goto exit;
        }
    }

    if (this->pcm_data_type == PCM_DATA_EFFECT_NS)
        effect = PAL_AUDIO_EFFECT_NS;

    rc = pal_add_remove_effect(this->pal_stream, effect, true);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d setting PCM effect to stream usecase:0x%x", rc, this->usecase_id);
        goto exit;
    }

exit:
    if (pal_param)
        free(pal_param);
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

int32_t UsecasePCMData::SetUseCaseData(uint32_t size, void *data)
{
    int rc = 0;
    uint32_t pcm_data_type_requested = 0;
    uint32_t pcm_data_buffering_requested = 0;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x, size:%d", this->usecase_id, size);

    if (size <= 0 || !data) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Invalid size:%d or data:%p for usecase:0x%x", rc, size, data, this->usecase_id);
        goto exit;
    }

    if (this->usecase_id == ASPS_USECASE_ID_PCM_DATA_V2) {
        pcm_data_buffering_requested = ((asps_pcm_data_v2_usecase_register_payload_t *)data)->requires_buffering;
        //update the pcm_data_buffering config based on the request of sensor
        this->pcm_data_buffering = pcm_data_buffering_requested;
        this->pal_devices[0].config.bit_width = ((asps_pcm_data_v2_usecase_register_payload_t *)data)->bit_width;
        this->pal_devices[0].config.sample_rate = ((asps_pcm_data_v2_usecase_register_payload_t *)data)->sample_rate;
        this->pal_devices[0].config.ch_info.channels = ((asps_pcm_data_v2_usecase_register_payload_t *)data)->num_channels;

        if (this->pal_devices[0].config.sample_rate == 16000)
            this->pal_devices[0].id = PAL_DEVICE_IN_HANDSET_VA_MIC;
        else if (this->pal_devices[0].config.sample_rate == 48000 ||
                 this->pal_devices[0].config.sample_rate == 96000)
            this->pal_devices[0].id = PAL_DEVICE_IN_ULTRASOUND_MIC;
    }

    pcm_data_type_requested = ((asps_pcm_data_usecase_register_payload_t *)data)->stream_type;
    if ((pcm_data_type_requested <= 0) ||
        (pcm_data_type_requested != PCM_DATA_EFFECT_RAW &&
            pcm_data_type_requested != PCM_DATA_EFFECT_NS)) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Invalid pcm_data_type_requested:%d for usecase:0x%x",
                rc, pcm_data_type_requested, this->usecase_id);
        goto exit;
    }
    //update the pcm_data_type config based on the request of sensor
    this->pcm_data_type = pcm_data_type_requested;

exit:
    PAL_VERBOSE(LOG_TAG, "Exit rc:%d", rc);
    return rc;
}

UsecaseUPD::UsecaseUPD(uint32_t usecase_id) : Usecase(usecase_id)
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", usecase_id);

    this->stream_attributes->type = PAL_STREAM_ULTRASOUND;
    this->stream_attributes->direction = PAL_AUDIO_INPUT_OUTPUT;

    this->no_of_devices = 2;
    this->pal_devices = (struct pal_device *) calloc(this->no_of_devices, sizeof(struct pal_device));
    if (!this->pal_devices) {
         PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for pal_devices", -ENOMEM);
         throw std::runtime_error("Failed to allocate memory for pal_devices");
    }

    this->tags.push_back(ULTRASOUND_DETECTION_MODULE);

    PAL_VERBOSE(LOG_TAG, "Exit");
}

UsecaseUPD::~UsecaseUPD()
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);
    //cleanup is done in baseclass
    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t UsecaseUPD::GetAckDataOnSuccessfullStart(uint32_t * size, void * data)
{
    int32_t rc = 0;
    uint32_t *data_ptr = (uint32_t *)data;
    int32_t no_of_miid = 0;
    std::map<int32_t, std::vector<uint32_t>> tag_miid_map;

    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = GetModuleIIDs(this->tags, tag_miid_map);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d failed to get module iids", rc);
        goto exit;
    }

    // grab all miids for all tags, in the order that tags exist in the UC vector
    for (auto tag : tags) {
        for (uint32_t miid : tag_miid_map[tag]) {
            *data_ptr = miid;
            ++data_ptr;
            ++no_of_miid;
        }
    }
    *size = (no_of_miid * sizeof(uint32_t));
exit:
    PAL_VERBOSE(LOG_TAG, "Exit %d", rc);
    return rc;
}

UsecasePCMRenderer::UsecasePCMRenderer(uint32_t usecase_id) : Usecase(usecase_id)
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", usecase_id);

    this->stream_attributes->type = PAL_STREAM_SENSOR_PCM_RENDERER;
    this->stream_attributes->direction = PAL_AUDIO_OUTPUT;

    this->no_of_devices = 1;
    this->pal_devices = (struct pal_device *) calloc(this->no_of_devices, sizeof(struct pal_device));

    if (!this->pal_devices) {
         PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for pal_devices", -ENOMEM);
         throw std::runtime_error("Failed to allocate memory for pal_devices");
    }

    this->tags.push_back(TAG_TONE_RENDERER_MODULE);

    PAL_VERBOSE(LOG_TAG, "Exit");
}

UsecasePCMRenderer::~UsecasePCMRenderer()
{
    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x", this->usecase_id);
    //cleanup is done in baseclass
    PAL_VERBOSE(LOG_TAG, "Exit");
}

int32_t UsecasePCMRenderer::SetUseCaseData(uint32_t size, void *data)
{
    int rc = 0;
    asps_ultrasound_rendering_usecase_register_payload_t *us_renderer_reg = NULL;
    uint8_t* channel_type = NULL;

    PAL_VERBOSE(LOG_TAG, "Enter usecase:0x%x, size:%d", this->usecase_id, size);
    if (size < sizeof(asps_ultrasound_rendering_usecase_register_payload_t) || !data) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Invalid size:%d or data:%p for usecase:0x%x", rc, size, data, this->usecase_id);
        goto exit;
    }

    us_renderer_reg = (asps_ultrasound_rendering_usecase_register_payload_t *)data;
    this->stream_attributes->out_media_config.sample_rate = us_renderer_reg->sampling_rate;
    // currently only support up to 2 channels
    if (us_renderer_reg->num_channels > 2) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "invalid num of channels: %d", us_renderer_reg->num_channels);
        goto exit;
    }
    this->stream_attributes->out_media_config.ch_info.channels = us_renderer_reg->num_channels;
    this->stream_attributes->out_media_config.bit_width = us_renderer_reg->bit_width;

    if (us_renderer_reg->num_channels >
         (size - sizeof(asps_ultrasound_rendering_usecase_register_payload_t))) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "num of channels: %d surpass payload size: %d",
                          us_renderer_reg->num_channels, size);
        goto exit;
    }
    channel_type = (uint8_t*)data + sizeof(asps_ultrasound_rendering_usecase_register_payload_t);
    for (uint32_t i = 0; i < us_renderer_reg->num_channels; i++)
        this->stream_attributes->out_media_config.ch_info.ch_map[i] = channel_type[i];

    // set custom key to apply corresponding path
    if (us_renderer_reg->num_channels == 1) {
        if (channel_type[0] == 0x1)
            strlcpy(this->pal_devices->custom_config.custom_key, "top-spkr",
                sizeof(this->pal_devices->custom_config.custom_key));
        else
            strlcpy(this->pal_devices->custom_config.custom_key, "bottom-spkr",
                sizeof(this->pal_devices->custom_config.custom_key));
    } else {
        strlcpy(this->pal_devices->custom_config.custom_key, "stereo-spkr",
                sizeof(this->pal_devices->custom_config.custom_key));
    }
exit:
    return rc;
}

int32_t UsecasePCMRenderer::GetAckDataOnSuccessfullStart(uint32_t * size, void * data)
{
    int32_t rc = 0;
    uint32_t *data_ptr = (uint32_t *)data;
    int32_t no_of_miid = 0;
    std::map<int32_t, std::vector<uint32_t>> tag_miid_map;

    PAL_VERBOSE(LOG_TAG, "Enter");

    rc = GetModuleIIDs(this->tags, tag_miid_map);
    if (rc) {
        PAL_ERR(LOG_TAG, "Error:%d failed to get module iids", rc);
        goto exit;
    }

    // grab all miids for all tags, in the order that tags exist in the UC vector
    for (auto tag : tags) {
        for (uint32_t miid : tag_miid_map[tag]) {
            *data_ptr = miid;
            ++data_ptr;
            ++no_of_miid;
        }
    }
    *size = (no_of_miid * sizeof(uint32_t));
exit:
    PAL_VERBOSE(LOG_TAG, "Exit %d", rc);
    return rc;
}
