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
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 */

#define ATRACE_TAG (ATRACE_TAG_AUDIO | ATRACE_TAG_HAL)
#define LOG_TAG "PAL: ACDEngine"

#include "ACDEngine.h"

#include <cmath>
#include <cutils/trace.h>
#include "Session.h"
#include "Stream.h"
#include "StreamACD.h"
#include "ResourceManager.h"
#include "kvh2xml.h"
#include "acd_api.h"

#define FILENAME_LEN 128
std::shared_ptr<ACDEngine> ACDEngine::eng_;

ACDEngine::ACDEngine(Stream *s, std::shared_ptr<ACDStreamConfig> sm_cfg) :
    ContextDetectionEngine(s, sm_cfg)
{
    int i;

    PAL_DBG(LOG_TAG, "Enter");

    session_->registerCallBack(HandleSessionCallBack, (uint64_t)this);

    PAL_DBG(LOG_TAG, "Exit");
}

ACDEngine::~ACDEngine()
{
    PAL_INFO(LOG_TAG, "Enter");

    PAL_INFO(LOG_TAG, "Exit");
}

std::shared_ptr<ContextDetectionEngine> ACDEngine::GetInstance(
     Stream *s,
     std::shared_ptr<ACDStreamConfig> sm_cfg)
{
     if (!eng_)
         eng_ = std::make_shared<ACDEngine>(s, sm_cfg);

     return eng_;
}

bool ACDEngine::IsEngineActive()
{
    if (eng_state_ == ENG_ACTIVE)
        return true;

    return false;
}

int32_t ACDEngine::ProcessStartEngine(Stream *s)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    status = session_->prepare(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to prepare session", status);
        goto exit;
    }

    status = session_->start(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to start session", status);
        goto exit;
    }

    eng_state_ = ENG_ACTIVE;
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t ACDEngine::StartEngine(Stream *s)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    std::lock_guard<std::mutex> lck(mutex_);

    if (IsEngineActive())
        goto exit;

    status = ProcessStartEngine(s);
    if (0 != status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to start Engine", status);

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t ACDEngine::ProcessStopEngine(Stream *s)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    status = session_->stop(s);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to stop session", status);

    eng_state_ = ENG_LOADED;
    PAL_DBG(LOG_TAG, "Exit, status = %d", status);
    return status;
}

int32_t ACDEngine::StopEngine(Stream *s)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    std::lock_guard<std::mutex> lck(mutex_);

    if (AreOtherStreamsAttached(s))
        goto exit;

    status = ProcessStopEngine(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to stop recognition", status);
        goto exit;
    }

exit:
    PAL_DBG(LOG_TAG, "Exit, status = %d", status);
    return status;
}

void ACDEngine::ParseEventAndNotifyClient()
{
    uint8_t *event_data;
    uint8_t *opaque_ptr;
    uint64_t detection_ts = 0;
    int i;
    struct acd_context_event *event = NULL;
    struct acd_per_context_event_info *event_info = NULL;
    int num_contexts = 0;
    std::map<Stream *, std::vector<struct acd_per_context_event_info *>*> stream_event_info;

    /* ParseEvent */
    while (!eventQ.empty())
    {
        struct acd_key_info_t *key_info = NULL;
        struct acd_generic_key_id_reg_cfg_t *reg_cfg = NULL;
        struct event_id_acd_detection_event_t *detection_event = NULL;
        int i;

        event_data = (uint8_t *)eventQ.front();
        eventQ.pop();
        opaque_ptr = event_data;
        detection_event = (struct event_id_acd_detection_event_t *)opaque_ptr;
        detection_ts = ((detection_event->event_timestamp_msw << 8) |
                        detection_event->event_timestamp_lsw);
        opaque_ptr += sizeof(struct event_id_acd_detection_event_t);
        key_info = (struct acd_key_info_t *)opaque_ptr;
        PAL_INFO(LOG_TAG, "key id: %d", key_info->key_id);

        opaque_ptr += sizeof(struct acd_key_info_t);
        reg_cfg = (struct acd_generic_key_id_reg_cfg_t *)opaque_ptr;
        PAL_INFO(LOG_TAG, "Num contexts: %d", reg_cfg->num_contexts);

        opaque_ptr += sizeof(struct acd_generic_key_id_reg_cfg_t);
        for (i = 0; i < reg_cfg->num_contexts; i++) {
            uint32_t context_id;
            uint32_t event_type;
            std::map<Stream *, struct stream_context_info *> *stream_ctx_data;

            event_info = (struct acd_per_context_event_info *)opaque_ptr;
            opaque_ptr += sizeof(struct acd_per_context_event_info);
            context_id = event_info->context_id;
            event_type = event_info->event_type;

            PAL_INFO(LOG_TAG, "Context Detection event %d received", event_type);

            auto iter = contextinfo_stream_map_.find(context_id);
            if (iter != contextinfo_stream_map_.end()) {
                stream_ctx_data = iter->second;

                PAL_INFO(LOG_TAG, "Received event contextId 0x%x, confidenceScore %d",
                            context_id, event_info->confidence_score);

                for (auto iter2 = stream_ctx_data->begin();
                     iter2 != stream_ctx_data->end(); ++iter2) {
                    bool notify_stream = false;
                    struct stream_context_info *context_cfg = iter2->second;
                    StreamACD *s = dynamic_cast<StreamACD *>(iter2->first);

                    PAL_VERBOSE(LOG_TAG, "Stream Threshold value for contextid[%d] is %d",
                                context_id, context_cfg->threshold);

                    if ((event_type == AUDIO_CONTEXT_EVENT_STOPPED) &&
                         (context_cfg->last_event_type != AUDIO_CONTEXT_EVENT_STOPPED)) {
                        notify_stream = true;
                    } else if ((event_type == AUDIO_CONTEXT_EVENT_STARTED) &&
                               (event_info->confidence_score >= context_cfg->threshold)) {
                        notify_stream = true;
                    } else if (event_type == AUDIO_CONTEXT_EVENT_DETECTED) {
                        if (context_cfg->last_event_type == AUDIO_CONTEXT_EVENT_STARTED) {
                            notify_stream = true;
                        } else if (context_cfg->last_event_type == AUDIO_CONTEXT_EVENT_STOPPED) {
                            if (event_info->confidence_score >= context_cfg->threshold) {
                                PAL_INFO(LOG_TAG, "Changing event type to Started");
                                event_type = AUDIO_CONTEXT_EVENT_STARTED;
                                notify_stream = true;
                            }
                        } else if (context_cfg->last_event_type == AUDIO_CONTEXT_EVENT_DETECTED) {
                            if (abs(double((int)event_info->confidence_score - (int)context_cfg->last_confidence_score)) >= context_cfg->step_size)
                                notify_stream = true;
                        }
                    }
                    PAL_DBG(LOG_TAG, "last_event_type = %d, last_confidence_score = %d",
                            context_cfg->last_event_type, context_cfg->last_confidence_score);

                    if (notify_stream) {
                        std::vector<struct acd_per_context_event_info *> *event_list;
                        auto iter3 = stream_event_info.find(s);
                        struct acd_per_context_event_info *stream_event_data =
                            (struct acd_per_context_event_info *)calloc(1, sizeof(struct acd_per_context_event_info));
                        if(stream_event_data != NULL){
                            memcpy(stream_event_data, event_info, sizeof(*event_info));
                            stream_event_data->event_type = event_type;
                            context_cfg->last_event_type = event_type;
                            context_cfg->last_confidence_score = event_info->confidence_score;

                            if (iter3 != stream_event_info.end()) {
                                event_list = iter3->second;
                                event_list->push_back(stream_event_data);
                            } else {
                                event_list = new(std::vector<struct acd_per_context_event_info *>);
                                event_list->push_back(stream_event_data);
                                stream_event_info.insert(std::make_pair(s, event_list));
                            }
                        }
                    }
                }
            } else {
                PAL_ERR(LOG_TAG, "Error:%d Received unregistered context %d event", -EINVAL, context_id);
            }
        }
        free(event_data);
    }
    /* NotifyClient */
    mutex_.unlock();
    for (auto iter4 = stream_event_info.begin();
         iter4 != stream_event_info.end();) {
        StreamACD *s = dynamic_cast<StreamACD *>(iter4->first);
        std::vector<struct acd_per_context_event_info *> event_list = *(iter4->second);

        num_contexts = event_list.size();
        event = (struct acd_context_event *)calloc(1, sizeof(*event) + (num_contexts * sizeof(acd_per_context_event_info)));
        if(event != NULL){
            event->detection_ts = detection_ts;
            event->num_contexts = num_contexts;
            opaque_ptr = (uint8_t *)event + sizeof(*event);
            for (i = 0; i < num_contexts; i++) {
                event_info = event_list[i];
                memcpy(opaque_ptr, event_info, sizeof(*event_info));
                free(event_info);
                opaque_ptr += sizeof(*event_info);
            }
            s->SetEngineDetectionData(event);
            delete(iter4->second);
            stream_event_info.erase(iter4++);
            free(event);
        }
    }
    mutex_.lock();
}

void ACDEngine::EventProcessingThread(ACDEngine *engine)
{
    PAL_INFO(LOG_TAG, "Enter. start thread loop");
    if (!engine) {
        PAL_ERR(LOG_TAG, "Error:%d Invalid engine", -EINVAL);
        return;
    }

    std::unique_lock<std::mutex> lck(engine->mutex_);
    while (!engine->exit_thread_) {
        if (engine->eventQ.empty()) {
            PAL_DBG(LOG_TAG, "waiting on cond");
            engine->cv_.wait(lck);
            PAL_DBG(LOG_TAG, "done waiting on cond");

            if (engine->exit_thread_) {
                PAL_VERBOSE(LOG_TAG, "Exit thread");
                break;
            }
        }
        engine->ParseEventAndNotifyClient();
    }
    PAL_DBG(LOG_TAG, "Exit");
}

void ACDEngine::HandleSessionEvent(uint32_t event_id __unused,
                                               void *data, uint32_t size)
{
    void *event_data = calloc(1, size);
    if (!event_data) {
        PAL_ERR(LOG_TAG, "Error:failed to allocate mem for event_data");
        return;
    }

    std::unique_lock<std::mutex> lck(mutex_);
    memcpy(event_data, data, size);
    eventQ.push(event_data);
    cv_.notify_one();
}

void ACDEngine::HandleSessionCallBack(uint64_t hdl, uint32_t event_id,
                                      void *data, uint32_t event_size)
{
    ACDEngine *engine = nullptr;

    PAL_INFO(LOG_TAG, "Enter, event detected on SPF, event id = 0x%x", event_id);
    if ((hdl == 0) || !data) {
        PAL_ERR(LOG_TAG, "Error:%d Invalid engine handle or event data", -EINVAL);
        return;
    }

    if (event_id != EVENT_ID_ACD_DETECTION_EVENT)
        return;

    engine = (ACDEngine *)hdl;
    if (engine->eng_state_ == ENG_ACTIVE)
        engine->HandleSessionEvent(event_id, data, event_size);
    else
        PAL_INFO(LOG_TAG, "Engine not active or buffering might be going on, ignore");

    PAL_DBG(LOG_TAG, "Exit");
    return;
}

bool ACDEngine::AreOtherStreamsAttached(Stream *s) {
    for (uint32_t i = 0; i < eng_streams_.size(); i++)
        if (s != eng_streams_[i])
            return true;

    return false;
}

int32_t ACDEngine::RegDeregSoundModel(uint32_t sess_param_id, uint8_t *payload, size_t payload_size) {
    int32_t status = 0;
    uint32_t param_id, miid = 0;
    uint32_t tag_id = CONTEXT_DETECTION_ENGINE;
    uint8_t *session_payload = nullptr;
    size_t len = 0;

    PAL_DBG(LOG_TAG, "Enter, param : %u", sess_param_id);

    status = session_->getMIID(nullptr, tag_id, &miid);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to get miid for tag %x", status, tag_id);
        return status;
    }

    if (sess_param_id == PAL_PARAM_ID_LOAD_SOUND_MODEL)
        param_id = PARAM_ID_DETECTION_ENGINE_REGISTER_MULTI_SOUND_MODEL;
    else
        param_id = PARAM_ID_DETECTION_ENGINE_DEREGISTER_MULTI_SOUND_MODEL;

    status = builder_->payloadCustomParam(&session_payload, &len,
                         (uint32_t *)payload, payload_size, miid, param_id);

    if (status || !session_payload) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to construct ACD soundmodel payload", status);
        return -ENOMEM;
    }

    status = session_->setParameters(stream_handle_, tag_id, sess_param_id, session_payload);
    if (status != 0)
        PAL_ERR(LOG_TAG, "Error:%d Failed to send sound model payload", status);

    return status;
}

int32_t ACDEngine::PopulateSoundModel(std::string model_file_name, uint32_t model_uuid)
{
    FILE *fp;
    size_t size = 0, bytes_read = 0;
    int32_t status = 0;
    void *payload = NULL;
    char filename[FILENAME_LEN];
    struct param_id_detection_engine_register_multi_sound_model_t *sm_data =
           nullptr;

    snprintf(filename, FILENAME_LEN, "%s%s", ACD_SM_FILEPATH, model_file_name.c_str());
    fp = fopen(filename, "rb");
    if (!fp) {
        PAL_ERR(LOG_TAG, "Error:%d Unable to open soundmodel file '%s'", -EIO,
            model_file_name.c_str());
        return -EIO;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    payload = calloc(1, size);
    if (!payload) {
        status = -ENOMEM;
        goto close_fp;
    }

    bytes_read = fread((char*)payload, 1, size , fp);
    if (bytes_read != size) {
        status = -EIO;
        PAL_ERR(LOG_TAG, "Error:%d failed to read data from soundmodel file' %s'\n",
            status, model_file_name.c_str());
        goto free_payload;
    }

    sm_data = (struct param_id_detection_engine_register_multi_sound_model_t *)
         calloc(1, sizeof(
         struct param_id_detection_engine_register_multi_sound_model_t) +
         size);

    if (sm_data == nullptr) {
        status =  -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate memory for sm_data", status);
        goto free_smdata;
    }

    sm_data->model_id = model_uuid;
    sm_data->model_size = size;
    ar_mem_cpy(sm_data->model, size, payload, size);
    size += (sizeof(param_id_detection_engine_register_multi_sound_model_t));

    status = RegDeregSoundModel(PAL_PARAM_ID_LOAD_SOUND_MODEL, (uint8_t *)sm_data, size);

free_smdata:
    free(sm_data);
free_payload:
    free(payload);
close_fp:
    fclose(fp);
    return status;
}

/* Decide is model load/unload is needed or not based on requested context id. */
void ACDEngine::UpdateModelCount(struct pal_param_context_list *context_cfg, bool enable)
{
    uint32_t i, model_id;
    std::string model_type;
    std::shared_ptr<ACDSoundModelInfo> sm_info;
    std::unordered_map<uint32_t, std::string> model_to_update;

    /* Step 1. Update model_to_update based on model associated with context id */
    for (i = 0; i < context_cfg->num_contexts; i++) {
        sm_info = sm_cfg_->GetSoundModelInfoByContextId(context_cfg->context_id[i]);
        if (!sm_info)
            return;
        model_id = sm_info->GetModelId();
        model_type = sm_info->GetModelType();
        model_to_update[model_id] = model_type;
    }

    /*
     * Step 2.  Add only those models to the list,
     * whose model count will be 1 upon loading and 0 upon unloading
     */
    for (auto model : model_to_update) {
        model_id = model.first;
        model_type = model.second;

        PAL_DBG(LOG_TAG, "Before: model_count_[%s] = %d", model_type.c_str(), model_count_[model_id]);
        if (enable) {
            if (++model_count_[model_id] == 1)
                model_load_needed_[model_id] = model_type;
        } else {
            if (--model_count_[model_id] == 0)
                model_unload_needed_[model_id] = model_type;
        }
        PAL_DBG(LOG_TAG, "After: model_count_[%s] = %d", model_type.c_str(), model_count_[model_id]);
    }
}

void ACDEngine::RemoveEventInfoForStream(Stream *s)
{
    std::map<Stream *, struct stream_context_info *> *stream_ctx_data;
    uint32_t context_id;
    struct stream_context_info *context_info = NULL, *current_context_info = NULL;

    for (auto iter1 = contextinfo_stream_map_.begin();
              iter1 != contextinfo_stream_map_.end(); ++iter1) {
        context_id = iter1->first;
        stream_ctx_data = iter1->second;

        /* For each context id, find data associated with stream s,
         * delete the entry and update final threshold value and step size.
         */
        auto iter2 = stream_ctx_data->find(s);

        if (iter2 != stream_ctx_data->end()) {
            context_info = iter2->second;
            /* Delete entry associated with stream s */
            stream_ctx_data->erase(s);

            /* update cumulative threshold value associated with context id */
            auto iter3 = cumulative_contextinfo_map_.find(context_id);
            if (iter3 != cumulative_contextinfo_map_.end()) {
                current_context_info = iter3->second;
                /* If stream threshold value/step size does not match with cumulative
                 * context_info values, then continue with next context_id.
                 */
                if ((current_context_info->threshold != context_info->threshold) &&
                    (current_context_info->step_size != context_info->step_size)) {
                    continue;
                }

                /* if there is no entry present with context_id, remove the key too */
                if (stream_ctx_data->size() == 0) {
                    cumulative_contextinfo_map_.erase(context_id);
                    free(current_context_info);
                    current_context_info = NULL;
                    is_confidence_value_updated_ = true;
                } else {
                    current_context_info->threshold = 100;
                    current_context_info->step_size = 100;

                    for (auto iter4 = stream_ctx_data->begin();
                         iter4 != stream_ctx_data->end(); ++iter4) {
                        struct stream_context_info *context_cfg = iter4->second;
                        /* update cumulative threshold values based on new set of
                         * stream context info.
                         */
                        if (context_cfg->threshold < current_context_info->threshold) {
                            current_context_info->threshold = context_cfg->threshold;
                            is_confidence_value_updated_ = true;
                        }
                        if (context_cfg->step_size < current_context_info->step_size) {
                            current_context_info->step_size = context_cfg->step_size;
                            is_confidence_value_updated_ = true;
                        }
                    }
                }
            }
            free(context_info);
            context_info = NULL;
        }
    }
}

void ACDEngine::AddEventInfoForStream(Stream *s, struct acd_recognition_cfg *recog_cfg)
{
    int i, num_contexts;
    struct acd_per_context_cfg *context_cfg = NULL;
    struct stream_context_info *context_info = NULL;
    uint8_t *opaque_data = (uint8_t *)recog_cfg;

    num_contexts = recog_cfg->num_contexts;
    context_cfg = (struct acd_per_context_cfg *)(opaque_data + sizeof(struct acd_recognition_cfg));
    for (i = 0; i < num_contexts; i++) {
        std::map<Stream *, struct stream_context_info *> *stream_ctx_data;

        /* Update contextinfo per stream */
        auto iter1 = contextinfo_stream_map_.find(context_cfg[i].context_id);
        if (iter1 != contextinfo_stream_map_.end()) {
            stream_ctx_data = iter1->second;

            // if entry is found, get the map handle and insert new value
            context_info = (struct stream_context_info *)calloc(1, sizeof(struct stream_context_info));
            if (!context_info) {
                PAL_ERR(LOG_TAG, "Error:failed to allocate mem for context_info");
                return;
            }

            context_info->threshold = context_cfg[i].threshold;
            context_info->step_size = context_cfg[i].step_size;
            stream_ctx_data->insert(std::make_pair(s, context_info));
        } else {
        //   if entry is not found, create entry and insert it to map
            stream_ctx_data = new std::map<Stream *, struct stream_context_info *>;
            context_info = (struct stream_context_info *)calloc(1, sizeof(struct stream_context_info));
            if (!context_info) {
                PAL_ERR(LOG_TAG, "Error:failed to allocate mem for context_info");
                return;
            }

            context_info->threshold = context_cfg[i].threshold;
            context_info->step_size = context_cfg[i].step_size;
            stream_ctx_data->insert(std::make_pair(s, context_info));
#ifndef LINUX_ENABLED
            contextinfo_stream_map_.insert(std::make_pair(context_cfg[i].context_id,
                stream_ctx_data));
#else
            uint32_t ctx_id = context_cfg[i].context_id;
            contextinfo_stream_map_.insert(std::make_pair(ctx_id, stream_ctx_data));
#endif
        }

        /* update/add cumulative contextinfo per context id */
        auto iter3 = cumulative_contextinfo_map_.find(context_cfg[i].context_id);
        if (iter3 != cumulative_contextinfo_map_.end()) {
            context_info = iter3->second;
            if (context_cfg[i].threshold < context_info->threshold) {
                context_info->threshold = context_cfg[i].threshold;
                is_confidence_value_updated_ = true;
            }
            if (context_cfg[i].step_size < context_info->step_size) {
                context_info->step_size = context_cfg[i].step_size;
                is_confidence_value_updated_ = true;
            }
        } else {
            context_info = (struct stream_context_info *)calloc(1, sizeof(struct stream_context_info));
            if (!context_info) {
                PAL_ERR(LOG_TAG, "Error:failed to allocate mem for context_info");
                return;
            }

            context_info->threshold = context_cfg[i].threshold;
            context_info->step_size = context_cfg[i].step_size;
#ifndef LINUX_ENABLED
            cumulative_contextinfo_map_.insert(std::make_pair(context_cfg[i].context_id,
                context_info));
#else
            uint32_t ctx_id = context_cfg[i].context_id;
            cumulative_contextinfo_map_.insert(std::make_pair(ctx_id, context_info));
#endif
            is_confidence_value_updated_ = true;
        }
    }
}

void ACDEngine::UpdateEventInfoForStream(Stream *s, struct acd_recognition_cfg *recog_cfg)
{
    std::map<Stream *, struct stream_context_info *> *stream_ctx_data;
    uint32_t context_id;
    int i, num_contexts;
    struct acd_per_context_cfg *context_cfg = NULL;
    struct stream_context_info *context_info = NULL, *per_stream_context_info = NULL;
    uint8_t *opaque_data = (uint8_t *)recog_cfg;

    PAL_DBG(LOG_TAG, "Enter");
    num_contexts = recog_cfg->num_contexts;
    context_cfg = (struct acd_per_context_cfg *)(opaque_data + sizeof(struct acd_recognition_cfg));

    /* 1. Add/Update existing context id threshold values and
     *    delete stream entries with older context id
     */
    for (auto iter1 = contextinfo_stream_map_.begin();
              iter1 != contextinfo_stream_map_.end();) {
        bool found = false;
        struct stream_context_info *context_info = NULL;

        context_id = iter1->first;
        stream_ctx_data = iter1->second;

        iter1++;
        /* For each context id, find context data associated with stream s,
         * add/update threshold value and step size.
         */
        auto iter2 = stream_ctx_data->find(s);

        for (i = 0; i < num_contexts; i++) {
            if (context_id == context_cfg[i].context_id) {
                if (iter2 == stream_ctx_data->end())
                    context_info = (struct stream_context_info *)calloc(1, sizeof(struct stream_context_info));
                else
                    context_info = iter2->second;

                if (!context_info) {
                    PAL_ERR(LOG_TAG, "Error:failed to allocate mem for context_info");
                    return;
                }

                context_info->threshold = context_cfg[i].threshold;
                context_info->step_size = context_cfg[i].step_size;

                if (iter2 == stream_ctx_data->end())
                    stream_ctx_data->insert(std::make_pair(s, context_info));

                found = true;
                PAL_INFO(LOG_TAG, "Added/Updated Context id 0x%x, Threshold %d, step_size %d",
                                   context_id, context_info->threshold, context_info->step_size);
            }
        }
        /* Delete entry associated with stream s, if new config does not have context_id */
        if (found == false) {
            stream_ctx_data->erase(s);
            if (stream_ctx_data->size() == 0)
                 contextinfo_stream_map_.erase(context_id);
        }
    }

    /* Add new entries to contextinfo_stream_map_ */
    for (i = 0; i < num_contexts; i++) {
        std::map<Stream *, struct stream_context_info *> *stream_ctx_data;

        /* Update contextinfo per stream */
        auto iter1 = contextinfo_stream_map_.find(context_cfg[i].context_id);
        if (iter1 == contextinfo_stream_map_.end()) {
            // if entry is not found, create entry and insert it to map
            stream_ctx_data = new std::map<Stream *, struct stream_context_info *>;
            context_info = (struct stream_context_info *)calloc(1, sizeof(struct stream_context_info));
            if (!context_info) {
                PAL_ERR(LOG_TAG, "Error:failed to allocate mem for context_info");
                return;
            }

            context_info->threshold = context_cfg[i].threshold;
            context_info->step_size = context_cfg[i].step_size;
            stream_ctx_data->insert(std::make_pair(s, context_info));
#ifndef LINUX_ENABLED
            contextinfo_stream_map_.insert(std::make_pair(context_cfg[i].context_id,
                stream_ctx_data));
#else
            uint32_t ctx_id = context_cfg[i].context_id;
            contextinfo_stream_map_.insert(std::make_pair(ctx_id, stream_ctx_data));
#endif
            PAL_INFO(LOG_TAG, "Added Context id 0x%x, Threshold %d, step_size %d",
                         context_cfg[i].context_id, context_info->threshold, context_info->step_size);
        }
    }

    for (auto iter1 = cumulative_contextinfo_map_.begin();
              iter1 != cumulative_contextinfo_map_.end();) {
        context_id = iter1->first;
        context_info = iter1->second;

        iter1++;
        auto iter2 = contextinfo_stream_map_.find(context_id);
        if (iter2 == contextinfo_stream_map_.end()) {
            cumulative_contextinfo_map_.erase(context_id);
            PAL_INFO(LOG_TAG, "Removed context id 0x%x from cumulative contextinfo map", context_id);
            is_confidence_value_updated_ = true;
        }
     }

    for (auto iter1 = contextinfo_stream_map_.begin();
              iter1 != contextinfo_stream_map_.end(); ++iter1) {
        context_id = iter1->first;
        stream_ctx_data = iter1->second;
        /* update/add cumulative contextinfo per context id */
        for (auto iter2 = stream_ctx_data->begin();
                 iter2 != stream_ctx_data->end(); ++iter2) {
            per_stream_context_info = iter2->second;
            auto iter3 = cumulative_contextinfo_map_.find(context_id);
            if (iter3 != cumulative_contextinfo_map_.end()) {
                context_info = iter3->second;

                if (per_stream_context_info->threshold < context_info->threshold) {
                    PAL_INFO(LOG_TAG, "Updated threshold value for context id 0x%x, old = %d, new = %d",
                             context_id,context_info->threshold, per_stream_context_info->threshold);
                    context_info->threshold = per_stream_context_info->threshold;
                    is_confidence_value_updated_ = true;
                }
                if (per_stream_context_info->step_size < context_info->step_size) {
                    PAL_INFO(LOG_TAG, "Updated step_size for context id 0x%x, old = %d, new = %d",
                             context_id,context_info->step_size, per_stream_context_info->step_size);
                    context_info->step_size = per_stream_context_info->step_size;
                    is_confidence_value_updated_ = true;
                }
            } else {
                context_info = (struct stream_context_info *)calloc(1, sizeof(struct stream_context_info));
                if (!context_info) {
                    PAL_ERR(LOG_TAG, "Error:failed to allocate mem for context_info");
                    return;
                }

                context_info->threshold = context_cfg[i].threshold;
                context_info->step_size = context_cfg[i].step_size;
#ifndef LINUX_ENABLED
                cumulative_contextinfo_map_.insert(std::make_pair(context_cfg[i].context_id,
                    context_info));
#else
                uint32_t ctx_id = context_cfg[i].context_id;
                cumulative_contextinfo_map_.insert(std::make_pair(ctx_id, context_info));
#endif
                PAL_INFO(LOG_TAG, "Added context id 0x%x from cumulative contextinfo map", context_id);
                is_confidence_value_updated_ = true;
            }
        }
    }
    PAL_INFO(LOG_TAG, "Exit, is_confidence_value_updated = %d", is_confidence_value_updated_);
}

bool ACDEngine::IsModelBinAvailable(uint32_t model_id)
{
    std::shared_ptr<ACDSoundModelInfo> sm_info = NULL;
    std::string bin_name;

    sm_info = sm_cfg_->GetSoundModelInfoByModelId(model_id);
    if (sm_info) {
        bin_name = sm_info->GetModelBinName();
        if (!bin_name.empty())
            return true;
    }
    return false;
}

int32_t ACDEngine::UnloadSoundModel()
{
    int32_t status = 0;
    uint32_t model_id;
    std::string model_type;
    struct param_id_detection_engine_deregister_multi_sound_model_t
                                                     deregister_config;
    memset(&deregister_config, 0, sizeof(struct param_id_detection_engine_deregister_multi_sound_model_t));

    for (auto model : model_unload_needed_) {
        model_id = model.first;
        model_type = model.second;

        /*
         * Skip Model Unloading if the bin file doesn't exist or
         * If the same model-id is already present in the model_load_needed_
         */
        if (model_load_needed_.find(model_id) != model_load_needed_.end() ||
            !IsModelBinAvailable(model_id)) {
            PAL_INFO(LOG_TAG, "Skipping Unloading of model type: %s\t id:%d",
                    model_type.c_str(), model_id);
            continue;
        }

        PAL_INFO(LOG_TAG, "Unloading model type: %s id: %d", model_type.c_str(), model_id);

        std::shared_ptr<ACDSoundModelInfo> modelInfo = sm_cfg_->GetSoundModelInfoByModelId(model_id);
        if (!modelInfo) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Error:failed to obtain model Info by model ID %d", model_id);
            return status;
        }
        deregister_config.model_id = modelInfo->GetModelUUID();
        if (deregister_config.model_id)
            status = RegDeregSoundModel(PAL_PARAM_ID_UNLOAD_SOUND_MODEL,
                                        (uint8_t *)&deregister_config,
                                        sizeof(deregister_config));
    }

    return status;
}

int32_t ACDEngine::PopulateEventPayload()
{
    struct event_id_acd_detection_cfg_t *detection_cfg = NULL;
    struct acd_key_info_t *key_info = NULL;
    struct acd_generic_key_id_reg_cfg_t *reg_cfg = NULL;
    struct acd_per_context_cfg *context_cfg = NULL;
    uint8_t *event_payload = NULL;
    size_t event_payload_size = 0;
    uint32_t num_keys = 1;
    uint32_t offset = 0;
    uint32_t num_contexts = cumulative_contextinfo_map_.size();

    if (num_contexts == 0)
        return 0;

    event_payload_size = sizeof(struct event_id_acd_detection_cfg_t) +
        (num_keys * sizeof(struct acd_key_info_t)) +
        (num_keys * sizeof(struct acd_generic_key_id_reg_cfg_t)) +
        (num_contexts * sizeof(struct acd_per_context_cfg));

    event_payload = (uint8_t *) calloc(1, event_payload_size);
    if (!event_payload) {
        PAL_ERR(LOG_TAG, "Error:failed to allocate mem for event_data");
        return -ENOMEM;
    }

    detection_cfg = (struct event_id_acd_detection_cfg_t *) event_payload;
    detection_cfg->num_keys = 1;

    offset = sizeof(struct event_id_acd_detection_cfg_t);
    key_info = (struct acd_key_info_t *)(event_payload + offset);
    key_info->key_id = ACD_KEY_ID_GENERIC_INFO ;
    key_info->key_payload_size = sizeof(struct acd_generic_key_id_reg_cfg_t) +
        (num_contexts * sizeof(struct acd_per_context_cfg));

    offset += sizeof(struct acd_key_info_t);
    reg_cfg = (struct acd_generic_key_id_reg_cfg_t *)(event_payload + offset);
    reg_cfg->num_contexts = num_contexts;
    offset += sizeof(struct acd_generic_key_id_reg_cfg_t);

    for (auto iter1 = cumulative_contextinfo_map_.begin();
              iter1 != cumulative_contextinfo_map_.end(); ++iter1) {
        struct stream_context_info *context_info = iter1->second;

        context_cfg = (struct acd_per_context_cfg *) (event_payload + offset);
        context_cfg->context_id = iter1->first;
        context_cfg->threshold = context_info->threshold;
        context_cfg->step_size = context_info->step_size;
        PAL_INFO(LOG_TAG, "Registering event for context id 0x%x, threshold %d, step_size %d",
                     context_cfg->context_id, context_cfg->threshold, context_cfg->step_size);
        offset += sizeof(struct acd_per_context_cfg);
    }
    session_->setEventPayload(EVENT_ID_ACD_DETECTION_EVENT, (void *)event_payload, event_payload_size);
    free(event_payload);
    return 0;
}

int32_t ACDEngine::LoadSoundModel()
{
    int32_t status = 0;
    uint32_t model_id;
    std::string model_type;
    std::shared_ptr<ACDSoundModelInfo> sm_info;
    std::string bin_name;
    uint32_t uuid;

    for (auto model : model_load_needed_) {
        model_id = model.first;
        model_type = model.second;

        /*
         * Skip Model Loading if the bin file doesn't exist or
         * If the same model-id is already present in the model_unload_needed_
         */
        if (model_unload_needed_.find(model_id) != model_unload_needed_.end() ||
            !IsModelBinAvailable(model_id)) {
            PAL_INFO(LOG_TAG, "Skipping loading of model type: %s id:%d",
                    model_type.c_str(), model_id);
            continue;
        }

        PAL_INFO(LOG_TAG, "Loading model type %s id: %d", model_type.c_str(),  model_id);

        sm_info = sm_cfg_->GetSoundModelInfoByModelId(model_id);
        if (!sm_info) {
            return -EINVAL;
        }

        bin_name = sm_info->GetModelBinName();
        if (!bin_name.empty()) {
            uuid = sm_info->GetModelUUID();
            status = PopulateSoundModel(bin_name, uuid);
        }
    }

    return status;
}

int32_t ACDEngine::HandleMultiStreamLoadUnload(Stream *s)
{
    int32_t status = 0;
    bool restore_eng_state = false;

    PAL_DBG(LOG_TAG, "Enter");
    std::unique_lock<std::mutex> lck(mutex_);

    if (IsEngineActive()) {
        ProcessStopEngine(eng_streams_[0]);
        restore_eng_state = true;
    }

    status = UnloadSoundModel();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to unload sound model", status);
        session_->close(s);
        goto exit;
    }

    status = PopulateEventPayload();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to setup Event payload", status);
        session_->close(s);
        goto exit;
    }

    status = LoadSoundModel();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to load sound model", status);
        session_->close(s);
        goto exit;
    }
    eng_state_ = ENG_LOADED;

    if (restore_eng_state)
       status = ProcessStartEngine(eng_streams_[0]);
exit:
    PAL_DBG(LOG_TAG, "Exit, status = %d", status);
    return status;
}

void ACDEngine::ResetModelLoadUnloadFlags()
{
    is_confidence_value_updated_ = false;
    model_load_needed_.clear();
    model_unload_needed_.clear();
}

int32_t ACDEngine::SetupEngine(Stream *st, void *config)
{
    int32_t status = 0;
    struct acd_recognition_cfg *recog_cfg = NULL;
    struct pal_param_context_list *context_cfg = (struct pal_param_context_list *)config;
    StreamACD *s = dynamic_cast<StreamACD *>(st);

    PAL_DBG(LOG_TAG, "Enter");

    std::unique_lock<std::mutex> lck(mutex_);
    ResetModelLoadUnloadFlags();
    UpdateModelCount(context_cfg, true);
    recog_cfg = s->GetRecognitionConfig();
    if (recog_cfg)
        AddEventInfoForStream(s, recog_cfg);

    /* Check whether any stream is already attached to this engine */
    if (AreOtherStreamsAttached(s)) {
        if (model_load_needed_.size() || is_confidence_value_updated_) {
            lck.unlock();
            status = HandleMultiStreamLoadUnload(s);
            lck.lock();
        }
        /* Register stream if new model is not required to be loaded */
        goto exit;
    }

    status = session_->open(s);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to open session", status);
        goto exit;
    }

    status = PopulateEventPayload();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to setup Event payload", status);
        session_->close(s);
        goto exit;
    }

    status = LoadSoundModel();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to load sound model", status);
        session_->close(s);
        goto exit;
    }
    exit_thread_ = false;
    event_thread_handler_ = std::thread(ACDEngine::EventProcessingThread, this);

    if (!event_thread_handler_.joinable()) {
        PAL_ERR(LOG_TAG, "Error:%d failed to create event processing thread",
                status);
        session_->close(s);
        status = -EINVAL;
        goto exit;
    }

    eng_state_ = ENG_LOADED;
exit:
    if (!status)
        eng_streams_.push_back(s);

    return status;
}

int32_t ACDEngine::ReconfigureEngine(Stream *st, void *old_cfg, void *new_cfg)
{
    int32_t status = 0;
    struct acd_recognition_cfg *recog_cfg = NULL;
    StreamACD *s = dynamic_cast<StreamACD *>(st);

    ResetModelLoadUnloadFlags();
    UpdateModelCount((struct pal_param_context_list *)old_cfg, false);
    UpdateModelCount((struct pal_param_context_list *)new_cfg, true);

    recog_cfg = s->GetRecognitionConfig();
    if (recog_cfg)
        UpdateEventInfoForStream(s, recog_cfg);

    if (model_load_needed_ != model_unload_needed_ || is_confidence_value_updated_) {
        status = HandleMultiStreamLoadUnload(s);
    }

    return status;
}

int32_t ACDEngine::TeardownEngine(Stream *st, void *config)
{
    int32_t status = 0;
    struct pal_param_context_list *cfg = (struct pal_param_context_list *)config;
    struct acd_recognition_cfg *recog_cfg = NULL;
    StreamACD *s = dynamic_cast<StreamACD *>(st);

    PAL_DBG(LOG_TAG, "Enter");
    std::unique_lock<std::mutex> lck(mutex_);
    ResetModelLoadUnloadFlags();
    UpdateModelCount(cfg, false);

    recog_cfg = s->GetRecognitionConfig();
    if (recog_cfg)
        RemoveEventInfoForStream(s);

    /* Check whether any stream is already attached to this engine */
    if (AreOtherStreamsAttached(s)) {
        if (model_unload_needed_.size() || is_confidence_value_updated_) {
            lck.unlock();
            status = HandleMultiStreamLoadUnload(s);
            lck.lock();
        }
        /* Register stream if new model is not required to be loaded */
        goto exit;
    }

    exit_thread_ = true;
    if (event_thread_handler_.joinable()) {
        cv_.notify_one();
        lck.unlock();
        event_thread_handler_.join();
        lck.lock();
        PAL_INFO(LOG_TAG, "Thread joined");
    }

    /* No need to unload soundmodel as the graph/engine instance will get closed */
    status = session_->close(s);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to close session", status);

    eng_state_ = ENG_IDLE;
exit:
    auto iter = std::find(eng_streams_.begin(), eng_streams_.end(), s);
    if (iter != eng_streams_.end())
        eng_streams_.erase(iter);

    return status;
}
