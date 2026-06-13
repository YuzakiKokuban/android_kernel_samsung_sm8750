/*
 * Copyright (c) 2021-2022, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define ATRACE_TAG (ATRACE_TAG_AUDIO | ATRACE_TAG_HAL)
#define LOG_TAG "PAL: ASREngine"

#include "ASREngine.h"

#include <cmath>
#include <cutils/trace.h>
#include <string.h>
#include "Session.h"
#include "Stream.h"
#include "StreamASR.h"
#include "ResourceManager.h"
#include "kvh2xml.h"

#define ASR_DBG_LOGS
#ifdef ASR_DBG_LOGS
#define PAL_DBG(LOG_TAG,...)  PAL_INFO(LOG_TAG,__VA_ARGS__)
#endif

#define FILENAME_LEN 128
std::shared_ptr<ASREngine> ASREngine::eng;

ASREngine::ASREngine(Stream *s, std::shared_ptr<ASRStreamConfig> smCfg)
{
    PAL_DBG(LOG_TAG, "Enter");
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::shared_ptr<ResourceManager> rm = nullptr;
    static std::shared_ptr<ASREngine> eng = nullptr;

    isCrrDevUsingExtEc = false;
    exitThread = false;
    ecRefCount = 0;
    devDisconnectCount = 0;
    numOutput = 0;
    rxEcDev = nullptr;
    asrInfo = nullptr;
    smCfg = smCfg;
    speechCfg = nullptr;
    outputCfg = nullptr;
    engState = ASR_ENG_IDLE;
    streamHandle = s;

    asrInfo = ASRPlatformInfo::GetInstance();
    if (!asrInfo) {
        PAL_ERR(LOG_TAG, "No ASR platform present");
        throw std::runtime_error("No ASR platform present");
    }

    for (int i = ASR_INPUT_CONFIG; i < ASR_MAX_PARAM_IDS; i++) {
        paramIds[i] = smCfg->GetParamId((asr_param_id_type)i);
        moduleTagIds[i] = smCfg->GetModuleTagId((asr_param_id_type)i);
    }

    status = streamHandle->getStreamAttributes(&sAttr);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get stream attributes");
        throw std::runtime_error("Failed to get stream attributes");
   }

    rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Failed to get ResourceManager instance");
        throw std::runtime_error("Failed to get ResourceManager instance");
    }

    session = Session::makeSession(rm, &sAttr);
    if (!session) {
        PAL_ERR(LOG_TAG, "Failed to create session");
        throw std::runtime_error("Failed to create session");
    }

    session->registerCallBack(HandleSessionCallBack, (uint64_t)this);

    eventThreadHandler = std::thread(ASREngine::EventProcessingThread, this);
    if (!eventThreadHandler.joinable()) {
        PAL_ERR(LOG_TAG, "Error:%d failed to create event processing thread",
               status);
        throw std::runtime_error("Failed to create event processing thread");
    }

    PAL_DBG(LOG_TAG, "Exit");
}

ASREngine::~ASREngine()
{
    PAL_INFO(LOG_TAG, "Enter");

    smCfg = nullptr;
    asrInfo = nullptr;
    session = nullptr;
    streamHandle = nullptr;

    {
        std::unique_lock<std::mutex> lck(mutexEngine);
        exitThread = true;
        cv.notify_one();
    }

    if(eventThreadHandler.joinable()) {
        eventThreadHandler.join();
    }

    PAL_INFO(LOG_TAG, "Exit");
}

std::shared_ptr<ASREngine> ASREngine::GetInstance(
     Stream *s,
     std::shared_ptr<ASRStreamConfig> smCfg)
{
     if (!eng)
         eng = std::make_shared<ASREngine>(s, smCfg);

     return eng;
}

bool ASREngine::IsEngineActive()
{
    if (engState == ASR_ENG_ACTIVE ||
        engState == ASR_ENG_TEXT_RECEIVED)
        return true;

    return false;
}

int32_t ASREngine::setParameters(Stream *s, asr_param_id_type_t pid, void *paramPayload)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter, param id %d ", pid);

    uint32_t tagId = 0;
    uint32_t paramId = 0;
    uint8_t *payload = nullptr;
    uint8_t *data = nullptr;
    size_t payloadSize = 0;
    size_t dataSize = 0;
    uint32_t sesParamId = 0;
    uint32_t miid = 0;
    uint32_t id = pid;
    StreamASR* sAsr = dynamic_cast<StreamASR *>(s);

    if (pid < ASR_INPUT_CONFIG || pid >= ASR_MAX_PARAM_IDS) {
        PAL_ERR(LOG_TAG, "Invalid param id %d", pid);
        status = -EINVAL;
        goto exit;
    }

    tagId = moduleTagIds[pid];
    paramId = paramIds[pid];

    status = session->getMIID(nullptr, tagId, &miid);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Failed to get instance id for tag %x, status = %d",
                tagId, status);
        goto exit;
    }

    switch (id) {
        case ASR_INPUT_CONFIG : {
            param_id_asr_config_t *config = sAsr->GetSpeechConfig();
            if (config == nullptr) {
                PAL_ERR(LOG_TAG, "No config available, can't start the engine!!!");
                goto exit;
            } else if (speechCfg != nullptr && speechCfg == config) {
                PAL_INFO(LOG_TAG, "Same config, no need to set it again!!!");
                goto exit;
            }

            data = (uint8_t *)config;
            dataSize = sizeof(param_id_asr_config_t);
            sesParamId = PAL_PARAM_ID_ASR_CONFIG;
            if (speechCfg)
                speechCfg = nullptr;
            speechCfg = config;
            break;
        }
        case ASR_FORCE_OUTPUT : {
            param_id_asr_force_output_t *param = (param_id_asr_force_output_t *)
                                    calloc(1, sizeof(param_id_asr_force_output_t));
            param->force_output = 1;
            data = (uint8_t *)param;
            dataSize = sizeof(param_id_asr_force_output_t);
            sesParamId = PAL_PARAM_ID_ASR_FORCE_OUTPUT;
            break;
        }
        case ASR_OUTPUT_CONFIG : {
            param_id_asr_output_config_t *opConfig = sAsr->GetOutputConfig();
            if (opConfig == nullptr) {
                PAL_ERR(LOG_TAG, "No output config available, can't start the engine!!!");
                goto exit;
            } else if (outputCfg != nullptr && outputCfg == opConfig) {
                PAL_INFO(LOG_TAG, "Same config, no need to set it again!!!");
                goto exit;
            }
            data = (uint8_t *)opConfig;
            dataSize = sizeof(param_id_asr_output_config_t);
            sesParamId = PAL_PARAM_ID_ASR_OUTPUT;
            if (outputCfg)
                outputCfg = nullptr;
            outputCfg = opConfig;
            break;
        }
        case ASR_INPUT_BUF_DURATON: {
            param_id_asr_input_threshold_t *ipConfig = sAsr->GetInputBufConfig();
            if (ipConfig == nullptr) {
                PAL_ERR(LOG_TAG, "No input config available, can't start the engine!!!");
                goto exit;
            } else if (inputCfg != nullptr && inputCfg == ipConfig) {
                PAL_INFO(LOG_TAG, "Same config, no need to set it again!!!");
                goto exit;
            }
            data = (uint8_t *)ipConfig;
            dataSize = sizeof(param_id_asr_input_threshold_t);
            sesParamId = PAL_PARAM_ID_ASR_SET_PARAM;
            if (inputCfg)
                inputCfg = nullptr;
            inputCfg = ipConfig;
            break;
        }
        default : {
            PAL_ERR(LOG_TAG, "Unexpected param ID is sent, not implemented yet");
        }
    }

    status = builder->payloadConfig(&payload, &payloadSize, data, dataSize,
                                        miid, paramId);
    if (status || !payload) {
        PAL_ERR(LOG_TAG, "Failed to construct ASR payload, status = %d",
            status);
        return -ENOMEM;
    }

    status = session->setParameters(streamHandle, tagId, sesParamId, payload);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Failed to set payload for param id %x, status = %d",
            sesParamId, status);
    }

exit:
    if (pid == ASR_FORCE_OUTPUT && data)
        free(data);

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t ASREngine::StartEngine(Stream *s)
{
    PAL_DBG(LOG_TAG, "Enter");

    int32_t status = 0;
    uint8_t *eventPayload = NULL;
    size_t eventPayloadSize = sizeof(struct event_id_asr_output_reg_cfg_t);
    struct event_id_asr_output_reg_cfg_t *eventConfig =  NULL;

    std::lock_guard<std::mutex> lck(mutexEngine);

    eventPayload = (uint8_t *)calloc(1, eventPayloadSize);
    if (eventPayload == NULL)
        goto exit;

    eventConfig = (struct event_id_asr_output_reg_cfg_t *)eventPayload;
    eventConfig->event_payload_type = 0;

    status = session->open(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to open session", status);
        goto exit;
    }

    session->setEventPayload(EVENT_ID_ASR_OUTPUT, (void *)eventPayload, eventPayloadSize);

    status = setParameters(s, ASR_INPUT_CONFIG);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to set engine config, can't start the engine!!!");
        goto exit;
    }

    status = setParameters(s, ASR_INPUT_BUF_DURATON);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to set input config, can't start the engine!!!");
        goto exit;
    }

    status = setParameters(s, ASR_OUTPUT_CONFIG);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to set output config, can't start the engine!!!");
        goto exit;
    }

    status = session->prepare(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to prepare session", status);
        goto exit;
    }

    status = session->start(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to start session", status);
        goto exit;
    }

    engState = ASR_ENG_ACTIVE;

exit:
    if (eventConfig)
        free(eventConfig);

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t ASREngine::StopEngine(Stream *s)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    status = session->stop(s);
    if (status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to stop session", status);
    }

    status = session->close(s);
    if (status)
        PAL_ERR(LOG_TAG, "Error: %d Failed to close session", status);

    engState = ASR_ENG_IDLE;
exit:
    PAL_DBG(LOG_TAG, "Exit, status = %d", status);
    return status;
}

void ASREngine::ParseEventAndNotifyStream() {

    PAL_DBG(LOG_TAG, "Enter.");

    int32_t status = 0;
    bool eventStatus = false;
    void *payload = nullptr;
    uint8_t *temp;
    size_t eventSize = 0;
    event_id_asr_output_event_t *event;
    asr_output_status_t *ev;
    pal_asr_event *eventToStream;
    StreamASR *sAsr;

    event = (struct event_id_asr_output_event_t *)eventQ.front();
    if (event == nullptr) {
        PAL_ERR(LOG_TAG, "Invalid event!!!");
        goto exit;
    }

    PAL_INFO(LOG_TAG, "Output mode : %d, output token : %d, num output : %d, payload size : %d",
            event->asr_out_mode, event->output_token, event->num_outputs, event->payload_size);

    if (event->num_outputs == 0) {
        PAL_ERR(LOG_TAG, "event raised without any transcript");
        goto exit;
    }

    eventSize = sizeof(pal_asr_event) + event->num_outputs * sizeof(pal_asr_engine_event);
    eventToStream = (pal_asr_event *)calloc(1, eventSize);
    if (eventToStream == nullptr) {
        PAL_ERR(LOG_TAG, "Failed to allocate memory for stream event!!");
        goto exit;
    }

    eventToStream->num_events = event->num_outputs;
    numOutput = event->num_outputs;
    outputToken = event->output_token;
    payloadSize = event->payload_size;

    status = session->getParameters(streamHandle,
                           moduleTagIds[ASR_OUTPUT], PAL_PARAM_ID_ASR_OUTPUT,
                           &payload);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Failed to get output payload");
        goto cleanup;
    }

    temp = (uint8_t *)payload;
    ev = (asr_output_status_t *)(temp + sizeof(struct param_id_asr_output_t));

    for (int i = 0; i < event->num_outputs; i++) {
        eventStatus = (ev[i].status == 0 ? true : false);
        if (!eventStatus) {
            PAL_INFO(LOG_TAG, "Recieved failure event, ignoring this event!!!");
            goto cleanup;
        }
        eventToStream->event[i].is_final = ev[i].is_final;
        eventToStream->event[i].confidence = ev[i].confidence;
        eventToStream->event[i].text_size = ev[i].text_size < 0 ? 0 : ev[i].text_size;
        for (int j = 0; j < ev[i].text_size; ++j)
            eventToStream->event[i].text[j] = ev[i].text[j];
    }


    eventToStream->status = PAL_ASR_EVENT_STATUS_SUCCESS ;

    sAsr = dynamic_cast<StreamASR *>(streamHandle);
    sAsr->HandleEventData(eventToStream, eventSize);
    numOutput = 0;
    outputToken = 0;
    payloadSize = 0;

cleanup:
    if (eventToStream)
        free(eventToStream);

    if (payload)
        free(payload);

    if (event)
        free(event);

exit:
    eventQ.pop();
}

void ASREngine::EventProcessingThread(ASREngine *engine)
{
    PAL_INFO(LOG_TAG, "Enter. start thread loop");
    if (!engine) {
        PAL_ERR(LOG_TAG, "Error:%d Invalid engine", -EINVAL);
        return;
    }
    std::unique_lock<std::mutex> lck(engine->mutexEngine);
    while (!engine->exitThread) {
        while (engine->eventQ.empty()) {
            PAL_DBG(LOG_TAG, "waiting on cond");
            engine->cv.wait(lck);
            PAL_DBG(LOG_TAG, "done waiting on cond");

            if (engine->exitThread) {
                PAL_VERBOSE(LOG_TAG, "Exit thread");
                break;
            }
        }
        //Adding this condition, as destructor can also notify this thread without any event
        if (!engine->eventQ.empty())
            engine->ParseEventAndNotifyStream();
    }

    PAL_DBG(LOG_TAG, "Exit");
}

void ASREngine::HandleSessionEvent(uint32_t event_id __unused,
                                   void *data, uint32_t size)
{
    void *eventData = nullptr;

    std::unique_lock<std::mutex> lck(mutexEngine);

    if (engState == ASR_ENG_IDLE) {
        PAL_INFO(LOG_TAG, "Engine not active, ignore");
        lck.unlock();
        return;
    }

    eventData = calloc(1, size);
    if (!eventData) {
        PAL_ERR(LOG_TAG, "Error:failed to allocate mem for event_data");
        return;
    }

    memcpy(eventData, data, size);
    eventQ.push(eventData);
    cv.notify_one();
}

void ASREngine::HandleSessionCallBack(uint64_t hdl, uint32_t eventId,
                                      void *data, uint32_t eventSize)
{
    ASREngine *engine = nullptr;

    PAL_INFO(LOG_TAG, "Enter, event detected on SPF, event id = 0x%x", eventId);
    if ((hdl == 0) || !data) {
        PAL_ERR(LOG_TAG, "Error:%d Invalid engine handle or event data", -EINVAL);
        return;
    }

    if (eventId != EVENT_ID_ASR_OUTPUT)
        return;

    engine = (ASREngine *)hdl;
    engine->HandleSessionEvent(eventId, data, eventSize);

    PAL_DBG(LOG_TAG, "Exit");
    return;
}

int32_t ASREngine::setECRef(Stream *s, std::shared_ptr<Device> dev, bool isEnable,
                                        bool setECForFirstTime) {
    int32_t status = 0;
    bool forceEnable = false;
    bool isDevEnabledExtEc = false;

    if (!session) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }
    PAL_DBG(LOG_TAG, "Enter, EC ref count : %d, enable : %d", ecRefCount, isEnable);
    PAL_DBG(LOG_TAG, "Rx device : %s, stream is setting EC for first time : %d",
            dev ? dev->getPALDeviceName().c_str() :  "Null", setECForFirstTime);

    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    if (!rm) {
        PAL_ERR(LOG_TAG, "Failed to get resource manager instance");
        return -EINVAL;
    }

    if (dev)
        isDevEnabledExtEc = rm->isExternalECRefEnabled(dev->getSndDeviceId());
    std::unique_lock<std::recursive_mutex> lck(ecRefMutex);
    if (isEnable) {
        if (isCrrDevUsingExtEc && !isDevEnabledExtEc) {
            PAL_ERR(LOG_TAG, "Internal EC connot be set, when external EC is active");
            return -EINVAL;
        }
        if (setECForFirstTime) {
            ecRefCount++;
        } else if (rxEcDev != dev ){
            forceEnable = true;
        } else {
            return status;
        }
        if (forceEnable || ecRefCount == 1) {
            status = session->setECRef(s, dev, isEnable);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set EC Ref for rx device %s",
                        dev ?  dev->getPALDeviceName().c_str() : "Null");
                if (setECForFirstTime) {
                    ecRefCount--;
                }
                if (forceEnable || ecRefCount == 0) {
                    rxEcDev = nullptr;
                }
            } else {
                isCrrDevUsingExtEc = isDevEnabledExtEc;
                rxEcDev = dev;
            }
        }
    } else {
        if (!dev || dev == rxEcDev) {
            if (ecRefCount > 0) {
                ecRefCount--;
                if (ecRefCount == 0) {
                    status = session->setECRef(s, dev, isEnable);
                    if (status) {
                        PAL_ERR(LOG_TAG, "Failed to reset EC Ref");
                    } else {
                        rxEcDev = nullptr;
                        isCrrDevUsingExtEc = false;
                    }
                }
            } else {
                PAL_DBG(LOG_TAG, "Skipping EC disable, as ref count is 0");
            }
        } else {
            PAL_DBG(LOG_TAG, "Skipping EC disable, as EC disable is not for correct device");
        }
    }
    PAL_DBG(LOG_TAG, "Exit, EC ref count : %d", ecRefCount);

    return status;
}

int32_t ASREngine::ConnectSessionDevice(
    Stream* streamHandle, pal_stream_type_t streamType,
    std::shared_ptr<Device> deviceToConnect)
{
    PAL_DBG(LOG_TAG, "Enter, devDisconnectCount: %d", devDisconnectCount);
    int32_t status = 0;

    if (!session) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    if (devDisconnectCount == 0)
        status = session->connectSessionDevice(streamHandle, streamType,
                                            deviceToConnect);
    if (status != 0)
        devDisconnectCount++;

    PAL_DBG(LOG_TAG, "Exit, devDisconnectCount: %d", devDisconnectCount);
    return status;
}

int32_t ASREngine::DisconnectSessionDevice(
    Stream* streamHandle, pal_stream_type_t streamType,
    std::shared_ptr<Device> deviceToDisconnect)
{
    PAL_DBG(LOG_TAG, "Enter, devDisconnectCount: %d", devDisconnectCount);
    int32_t status = 0;

    if (!session) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    devDisconnectCount++;
    status = session->disconnectSessionDevice(streamHandle, streamType,
                                               deviceToDisconnect);
    if (status != 0)
        devDisconnectCount--;
    PAL_DBG(LOG_TAG, "Exit, devDisconnectCount: %d", devDisconnectCount);
    return status;
}

int32_t ASREngine::SetupSessionDevice(
    Stream* streamHandle, pal_stream_type_t streamType,
    std::shared_ptr<Device> deviceToDisconnect)
{
    PAL_DBG(LOG_TAG, "Enter, devDisconnectCount: %d", devDisconnectCount);
    int32_t status = 0;

    if (!session) {
        PAL_ERR(LOG_TAG, "Invalid session");
        return -EINVAL;
    }

    devDisconnectCount--;
    if (devDisconnectCount < 0)
        devDisconnectCount = 0;

    if (devDisconnectCount == 0)
        status = session->setupSessionDevice(streamHandle, streamType,
                                          deviceToDisconnect);
    if (status != 0)
        devDisconnectCount++;

    PAL_DBG(LOG_TAG, "Enter, devDisconnectCount: %d", devDisconnectCount);
    return status;
}
