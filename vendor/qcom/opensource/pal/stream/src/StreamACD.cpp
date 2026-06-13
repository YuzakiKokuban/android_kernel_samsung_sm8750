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

#define LOG_TAG "PAL: StreamACD"

#include "StreamACD.h"

#include <unistd.h>
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"

StreamACD::StreamACD(struct pal_stream_attributes *sattr,
                                       struct pal_device *dattr,
                                       uint32_t no_of_devices,
                                       struct modifier_kv *modifiers __unused,
                                       uint32_t no_of_modifiers __unused,
                                       std::shared_ptr<ResourceManager> rm)
{
    int32_t enable_concurrency_count = 0;
    int32_t disable_concurrency_count = 0;

    rec_config_ = nullptr;
    context_config_ = nullptr;
    paused_ = false;
    device_opened_ = false;
    currentState = STREAM_IDLE;
    exit_thread_ = false;
    acd_idle_ = nullptr;
    acd_loaded_ = nullptr;
    acd_active = nullptr;
    acd_detected_ = nullptr;
    acd_ssr_ = nullptr;
    acd_states_ = {};
    cached_event_data_ = nullptr;
    callback_ = nullptr;
    cookie_ = 0;
    cur_state_ = nullptr;
    prev_state_ = nullptr;
    state_for_restore_ = ACD_STATE_NONE;

    // Setting default volume to unity
    mVolumeData = (struct pal_volume_data *)malloc(sizeof(struct pal_volume_data)
                      +sizeof(struct pal_channel_vol_kv));
    if (!mVolumeData) {
        PAL_ERR(LOG_TAG, "Error:mVolumeData allocation failed");
        throw std::runtime_error("mVolumeData allocation failed");
    }
    mVolumeData->no_of_volpair = 1;
    mVolumeData->volume_pair[0].channel_mask = 0x03;
    mVolumeData->volume_pair[0].vol = 1.0f;

    PAL_DBG(LOG_TAG, "Enter");
    mNoOfModifiers = 0;
    mModifiers = (struct modifier_kv *) (nullptr);

    mStreamAttr = (struct pal_stream_attributes *)calloc(1,
        sizeof(struct pal_stream_attributes));
    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "Error:%d stream attributes allocation failed", -EINVAL);
        throw std::runtime_error("stream attributes allocation failed");
    }

    if (!dattr) {
        PAL_ERR(LOG_TAG, "Error: pal_device is passed as null");
        throw std::runtime_error("pal_device is passed as null");
    }

    ar_mem_cpy(mStreamAttr, sizeof(pal_stream_attributes),
                     sattr, sizeof(pal_stream_attributes));
    mStreamAttr->in_media_config.sample_rate = 16000;
    mStreamAttr->in_media_config.bit_width = 16;
    mStreamAttr->in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    mStreamAttr->in_media_config.ch_info.channels = 1;
    mStreamAttr->direction = PAL_AUDIO_INPUT;

    // get ACD platform info
    acd_info_ = ACDPlatformInfo::GetInstance();
    if (!acd_info_) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to get acd platform info", -EINVAL);
        throw std::runtime_error("Failed to get acd platform info");
    }

    if (!acd_info_->IsACDEnabled()) {
        PAL_ERR(LOG_TAG, "Error:%d ACD not enabled, exiting", -EINVAL);
        throw std::runtime_error("ACD not enabled, exiting");
    }

    notification_thread_handler_ = std::thread(EventNotificationThread, this);
    if (!notification_thread_handler_.joinable()) {
        PAL_ERR(LOG_TAG, "Error:%d failed to create notification thread",
                -EINVAL);
        throw std::runtime_error("failed to create notification thread");
    }
    rm->registerStream(this);

    // Create internal states
    acd_idle_ = new ACDIdle(*this);
    acd_loaded_ = new ACDLoaded(*this);
    acd_active = new ACDActive(*this);
    acd_detected_ = new ACDDetected(*this);
    acd_ssr_ = new ACDSSR(*this);

    AddState(acd_idle_);
    AddState(acd_loaded_);
    AddState(acd_active);
    AddState(acd_detected_);
    AddState(acd_ssr_);

    // Set initial state
    cur_state_ = acd_idle_;

    // Print the concurrency feature flags supported
    PAL_INFO(LOG_TAG, "capture conc enable %d,voice conc enable %d,voip conc enable %d",
        acd_info_->GetConcurrentCaptureEnable(), acd_info_->GetConcurrentVoiceCallEnable(),
        acd_info_->GetConcurrentVoipCallEnable());

    // check concurrency count from rm
    rm->GetSoundTriggerConcurrencyCount(PAL_STREAM_ACD, &enable_concurrency_count,
        &disable_concurrency_count);

    /*
     * When voice/voip/record is active and concurrency is not
     * supported, mark paused as true, so that start recognition
     * will be skipped and when voice/voip/record stops, stream
     * will be resumed.
     */
    if (disable_concurrency_count) {
        paused_ = true;
    }
    PAL_DBG(LOG_TAG, "Exit");
}

StreamACD::~StreamACD()
{
    acd_states_.clear();
    exit_thread_ = true;
    if (notification_thread_handler_.joinable()) {
        std::unique_lock<std::mutex> lck(mutex_);
        cv_.notify_one();
        lck.unlock();
        notification_thread_handler_.join();
        PAL_INFO(LOG_TAG, "Thread joined");
    }

    rm->deregisterStream(this);
    if (mStreamAttr) {
        free(mStreamAttr);
    }
    mDevices.clear();
    PAL_DBG(LOG_TAG, "Exit");
}

int32_t StreamACD::getTagsWithModuleInfo(size_t *size, uint8_t *payload)
{
    int32_t status = 0;

    if (!payload) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d, Invalid payload", status);
        goto exit;
    }

    if (!engine_) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d, Engine not initialized yet", status);
        goto exit;
    }
    status = engine_->getTagsWithModuleInfo(this, size, payload);
exit:
    return status;
}

int32_t StreamACD::close()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter, stream direction %d", mStreamAttr->direction);

    std::lock_guard<std::mutex> lck(mStreamMutex);

    std::shared_ptr<ACDEventConfig> ev_cfg(new ACDUnloadEventConfig());
    status = cur_state_->ProcessEvent(ev_cfg);

    if (rec_config_) {
        free(rec_config_);
        rec_config_ = nullptr;
    }

    if (context_config_) {
        free(context_config_);
        context_config_ = nullptr;
    }

    if (cached_event_data_) {
        free(cached_event_data_);
        cached_event_data_ = nullptr;
    }
    palStateEnqueue(this, PAL_STATE_CLOSED, status);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::start()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter, stream direction %d", mStreamAttr->direction);

    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ACDEventConfig> ev_cfg(
       new ACDStartRecognitionEventConfig(false));
    status = cur_state_->ProcessEvent(ev_cfg);
    if (!status) {
        currentState = STREAM_STARTED;
    }
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::stop()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ACDEventConfig> ev_cfg(
       new ACDStopRecognitionEventConfig(false));
    status = cur_state_->ProcessEvent(ev_cfg);
    if (!status) {
        currentState = STREAM_STOPPED;
    }
    palStateEnqueue(this, PAL_STATE_STOPPED, status);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::Resume(bool is_internal) {
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ACDEventConfig> ev_cfg(new ACDResumeEventConfig());
    status = cur_state_->ProcessEvent(ev_cfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Resume failed", status);
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamACD::Pause(bool is_internal) {
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ACDEventConfig> ev_cfg(new ACDPauseEventConfig());
    status = cur_state_->ProcessEvent(ev_cfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Pause failed", status);
    palStateEnqueue(this, PAL_STATE_PAUSED, status);
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamACD::HandleConcurrentStream(bool active) {
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    if (active == false)
        mStreamMutex.lock();

    std::shared_ptr<ACDEventConfig> ev_cfg(
        new ACDConcurrentStreamEventConfig(active));
    status = cur_state_->ProcessEvent(ev_cfg);

    if (active == true)
        mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamACD::getParameters(uint32_t param_id __unused, void **payload __unused)
{
    return 0;
}

int32_t StreamACD::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = 0;
    pal_param_payload *param_payload = (pal_param_payload *)payload;

    if (!param_payload) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Invalid payload for param ID: %d", status, param_id);
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter, param id %d", param_id);

    std::lock_guard<std::mutex> lck(mStreamMutex);
    switch (param_id) {
    case PAL_PARAM_ID_LOAD_SOUND_MODEL: {
        std::shared_ptr<ACDEventConfig> ev_cfg(
                new ACDLoadEventConfig((void *)param_payload->payload));
        status = cur_state_->ProcessEvent(ev_cfg);
        break;
        }
    case PAL_PARAM_ID_RECOGNITION_CONFIG: {
        uint8_t *opaque_ptr = NULL;
        struct pal_st_recognition_config *config =
            (struct pal_st_recognition_config *)param_payload->payload;

        opaque_ptr = (uint8_t *)config + config->data_offset;
        opaque_ptr += sizeof(struct st_param_header);
        std::shared_ptr<ACDEventConfig> ev_cfg(
              new ACDRecognitionCfgEventConfig((void *)opaque_ptr));
          status = cur_state_->ProcessEvent(ev_cfg);
          break;
      }
      case PAL_PARAM_ID_CONTEXT_LIST: {
          std::shared_ptr<ACDEventConfig> ev_cfg(
              new ACDContextCfgEventConfig((void *)param_payload->payload));
          status = cur_state_->ProcessEvent(ev_cfg);
          break;
      }
      default: {
          status = -EINVAL;
          PAL_ERR(LOG_TAG, "Error:%d Unsupported param %u", status, param_id);
          break;
      }
    }

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::registerCallBack(pal_stream_callback cb,
                                             uint64_t cookie)
{
    callback_ = cb;
    cookie_ = cookie;

    PAL_VERBOSE(LOG_TAG, "callback_ = %pK", callback_);

    return 0;
}

int32_t StreamACD::getCallBack(pal_stream_callback *cb)
{
    if (!cb) {
        PAL_ERR(LOG_TAG, "Error:%d Invalid cb", -EINVAL);
        return -EINVAL;
    }
    // Do not expect this to be called.
    *cb = callback_;
    return 0;
}

int32_t StreamACD::setECRef(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (rm->getLPIUsage()) {
        PAL_DBG(LOG_TAG, "EC ref will be handled in LPI/NLPI switch");
        return status;
    }
    status = setECRef_l(dev, is_enable);

    return status;
}

int32_t StreamACD::setECRef_l(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;
    std::shared_ptr<ACDEventConfig> ev_cfg(
        new ACDECRefEventConfig(dev, is_enable));

    PAL_DBG(LOG_TAG, "Enter, enable %d", is_enable);

    if (!cap_prof_ || !cap_prof_->isECRequired()) {
        PAL_DBG(LOG_TAG, "No need to set ec ref");
        goto exit;
    }

    if (dev && !rm->checkECRef(dev, mDevices[0])) {
        PAL_DBG(LOG_TAG, "No need to set ec ref for unmatching rx device");
        goto exit;
    }

    status = cur_state_->ProcessEvent(ev_cfg);
    if (status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to handle ec ref event", status);
    }

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

void StreamACD::PopulateCallbackPayload(struct acd_context_event *event, void *payload)
{
    struct pal_st_recognition_event *recognition_event = NULL;
    struct st_param_header *st_header = NULL;
    int offset = 0;
    int data_size = sizeof(struct st_param_header) +
                    sizeof(struct acd_context_event) +
                    (event->num_contexts * sizeof(struct acd_per_context_event_info));

    PAL_DBG(LOG_TAG, "Enter");
    recognition_event = (struct pal_st_recognition_event *) payload;
    recognition_event->status = PAL_RECOGNITION_STATUS_SUCCESS;
    recognition_event->type = PAL_SOUND_MODEL_TYPE_GENERIC;
    recognition_event->st_handle = (pal_st_handle_t *)this;
    recognition_event->data_size = data_size;
    recognition_event->data_offset = sizeof(struct pal_st_recognition_event);
    recognition_event->media_config.bit_width = BITWIDTH_16;
    recognition_event->media_config.ch_info.channels = CHANNELS_1;
    recognition_event->media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    st_header = (struct st_param_header *)((uint8_t *)payload + sizeof(struct pal_st_recognition_event));
    st_header->key_id = ST_PARAM_KEY_CONTEXT_EVENT_INFO;
    st_header->payload_size = recognition_event->data_size - sizeof(struct st_param_header);

    PAL_INFO(LOG_TAG, "Populated %d contexts", event->num_contexts);
    offset = sizeof(struct pal_st_recognition_event) + sizeof(struct st_param_header);
    memcpy((uint8_t *)payload + offset, event, st_header->payload_size);
    PAL_DBG(LOG_TAG, "Exit");
}

void StreamACD::CacheEventData(struct acd_context_event *event)
{
    size_t new_event_size = sizeof(struct pal_st_recognition_event) +
                            sizeof(st_param_header) +
                            sizeof(struct acd_context_event) +
                            (event->num_contexts * sizeof(struct acd_per_context_event_info));
    struct acd_context_event *current_context_event;
    uint8_t *event_data = NULL;
    uint8_t *per_context_info;
    int offset = 0;

    PAL_DBG(LOG_TAG, "Enter");
    if (cached_event_data_) {
        offset = cached_event_data_->data_offset + sizeof(st_param_header);
        current_context_event =  (struct acd_context_event *)((uint8_t *)cached_event_data_ + offset);
        new_event_size +=  current_context_event->num_contexts * sizeof(struct acd_per_context_event_info);

        cached_event_data_ = (struct pal_st_recognition_event *)realloc(cached_event_data_, new_event_size);
        if (!cached_event_data_) {
            PAL_ERR(LOG_TAG, "Error:%d Failed to reallocate new memory space for cached_event_data_", -ENOMEM);
            goto exit;
        }
        /* Update current_context_event after realloc */
        current_context_event =  (struct acd_context_event *)((uint8_t *)cached_event_data_ + offset);

        cached_event_data_->data_size += event->num_contexts * sizeof(struct acd_per_context_event_info);
        per_context_info = (uint8_t *) ((uint8_t *) current_context_event +
                            sizeof(struct acd_context_event) +
                            (current_context_event->num_contexts * sizeof(struct acd_per_context_event_info)));
        event_data = (uint8_t *)event + sizeof(struct acd_context_event);
        for (int i = 0; i < event->num_contexts; i++) {
            memcpy(per_context_info, event_data, sizeof(struct acd_per_context_event_info));
            per_context_info += sizeof(struct acd_per_context_event_info);
            event_data += sizeof(struct acd_per_context_event_info);
        }
        current_context_event->num_contexts += event->num_contexts;
        PAL_INFO(LOG_TAG, "Total cached events = %d", current_context_event->num_contexts);
    } else {
        cached_event_data_ = (struct pal_st_recognition_event *) calloc(1, new_event_size);
        if (!cached_event_data_) {
            PAL_ERR(LOG_TAG, "failed to allocate memory for cached_event_data_");
            return;
        }
        PopulateCallbackPayload(event, cached_event_data_);
    }

exit:
    PAL_DBG(LOG_TAG, "Exit");
    return;
}

void StreamACD::SendCachedEventData()
{
    struct acd_context_event *context_event = NULL;
    uint32_t *ev_payload = NULL;

    if (callback_) {
notify:
        size_t event_size = cached_event_data_->data_size + sizeof(struct pal_st_recognition_event);
        context_event = (struct acd_context_event *)(((uint8_t*) cached_event_data_) + sizeof(struct pal_st_recognition_event) + sizeof(struct st_param_header));
        PAL_INFO(LOG_TAG, "Notify cached detection event to client with no of contexts=%d", context_event->num_contexts);
        ev_payload = (uint32_t *)calloc(1, event_size);
        if (!ev_payload) {
            ALOGE("memory allocation for ev_payload failed");
            free(cached_event_data_);
            cached_event_data_ = NULL;
            return;
        }
        memcpy(ev_payload, cached_event_data_, event_size);
        free(cached_event_data_);
        cached_event_data_ = NULL;
        /* SendCachedEventData() is always called with mutex_ lock acquired.
         *  Unlock it before calling callback */
        notificationInProgress = true;
        mutex_.unlock();
        callback_((pal_stream_handle_t *)this, 0, ev_payload, event_size, cookie_);
        free(ev_payload);
        ev_payload = NULL;
        mutex_.lock();
        notificationInProgress = false;
        /* If mutex_ lock is acquired by other thread handling detection event before
         * this thread, then cv_.notify_one will not be handled. Handle it here and
         * notify client if there is pending notification to be sent to client.
         */
        if (deferredNotification == true && cached_event_data_ != NULL) {
            deferredNotification = false;
            goto notify;
        }
    } else {
        free(cached_event_data_);
        cached_event_data_ = NULL;
    }
}

void StreamACD::SetEngineDetectionData(struct acd_context_event *event)
{
    PAL_DBG(LOG_TAG, "Enter");
    mStreamMutex.lock();
    std::shared_ptr<ACDEventConfig> ev_cfg(
       new ACDDetectedEventConfig((void *)event));
    cur_state_->ProcessEvent(ev_cfg);
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit");
}

pal_device_id_t StreamACD::GetAvailCaptureDevice()
{
    if (acd_info_->GetSupportDevSwitch() &&
        rm->isDeviceAvailable(PAL_DEVICE_IN_WIRED_HEADSET))
        return PAL_DEVICE_IN_HEADSET_VA_MIC;
    else
        return PAL_DEVICE_IN_HANDSET_VA_MIC;
}

std::shared_ptr<CaptureProfile> StreamACD::GetCurrentCaptureProfile()
{
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;
    enum StInputModes input_mode = ST_INPUT_MODE_HANDSET;
    enum StOperatingModes operating_mode = ST_OPERATING_MODE_HIGH_PERF;

    if (GetAvailCaptureDevice() == PAL_DEVICE_IN_HEADSET_VA_MIC)
        input_mode = ST_INPUT_MODE_HEADSET;

    if (rm->getLPIUsage())
        operating_mode = ST_OPERATING_MODE_LOW_POWER;

    cap_prof = sm_cfg_->GetCaptureProfile(
                std::make_pair(operating_mode, input_mode));
    if (!cap_prof) {
        PAL_ERR(LOG_TAG, "Error:Failed to get capture profile");
        goto exit;
    }

    PAL_DBG(LOG_TAG, "cap_prof %s: dev_id=0x%x, chs=%d, sr=%d, snd_name=%s, ec_ref=%d",
        cap_prof->GetName().c_str(), cap_prof->GetDevId(),
        cap_prof->GetChannels(), cap_prof->GetSampleRate(),
        cap_prof->GetSndName().c_str(), cap_prof->isECRequired());

exit:
    return cap_prof;
}

int32_t StreamACD::DisconnectDevice(pal_device_id_t device_id) {
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    /*
     * NOTE: mStreamMutex will be unlocked after ConnectDevice handled
     * because device disconnect/connect should be handled sequencely,
     * and no other commands from client should be handled between
     * device disconnect and connect.
     */
    mStreamMutex.lock();
    std::shared_ptr<ACDEventConfig> ev_cfg(
        new ACDDeviceDisconnectedEventConfig(device_id));
    status = cur_state_->ProcessEvent(ev_cfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d", status, device_id);

    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamACD::ConnectDevice(pal_device_id_t device_id) {
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter");
    std::shared_ptr<ACDEventConfig> ev_cfg(
        new ACDDeviceConnectedEventConfig(device_id));
    status = cur_state_->ProcessEvent(ev_cfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d", status, device_id);
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

void StreamACD::AddState(ACDState* state)
{
   acd_states_.insert(std::make_pair(state->GetStateId(), state));
}

int32_t StreamACD::GetCurrentStateId()
{
    if (cur_state_)
        return cur_state_->GetStateId();

    return ACD_STATE_NONE;
}

int32_t StreamACD::GetPreviousStateId()
{
    if (prev_state_)
        return prev_state_->GetStateId();

    return ACD_STATE_NONE;
}

void StreamACD::TransitTo(int32_t state_id)
{
    auto it = acd_states_.find(state_id);

    if (it == acd_states_.end()) {
        PAL_ERR(LOG_TAG, "Error:%d Unknown transit state %d", -EINVAL, state_id);
        return;
    }
    prev_state_ = cur_state_;
    cur_state_ = it->second;
    auto oldState = acdStateNameMap.at(prev_state_->GetStateId());
    auto newState = acdStateNameMap.at(it->first);
    PAL_DBG(LOG_TAG, "Stream : state transitioned from %s to %s",
           oldState.c_str(), newState.c_str());
}

int32_t StreamACD::ProcessInternalEvent(
    std::shared_ptr<ACDEventConfig> ev_cfg) {
    return cur_state_->ProcessEvent(ev_cfg);
}

struct acd_recognition_cfg* StreamACD::GetRecognitionConfig()
{
    return rec_config_;
}

void StreamACD::GetUUID(class SoundTriggerUUID *uuid,
                                const struct st_uuid *vendor_uuid)
{

    uuid->timeLow = (uint32_t)vendor_uuid->timeLow;
    uuid->timeMid = (uint16_t)vendor_uuid->timeMid;
    uuid->timeHiAndVersion = (uint16_t)vendor_uuid->timeHiAndVersion;
    uuid->clockSeq = (uint16_t)vendor_uuid->clockSeq;
    uuid->node[0] = (uint8_t)vendor_uuid->node[0];
    uuid->node[1] = (uint8_t)vendor_uuid->node[1];
    uuid->node[2] = (uint8_t)vendor_uuid->node[2];
    uuid->node[3] = (uint8_t)vendor_uuid->node[3];
    uuid->node[4] = (uint8_t)vendor_uuid->node[4];
    uuid->node[5] = (uint8_t)vendor_uuid->node[5];
    PAL_INFO(LOG_TAG, "Input vendor uuid : %08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
                        uuid->timeLow,
                        uuid->timeMid,
                        uuid->timeHiAndVersion,
                        uuid->clockSeq,
                        uuid->node[0],
                        uuid->node[1],
                        uuid->node[2],
                        uuid->node[3],
                        uuid->node[4],
                        uuid->node[5]);
}

/* Use below UUID for ACM usecase */
static const struct st_uuid qc_acd_uuid =
    { 0x4e93281b, 0x296e, 0x4d73, 0x9833, { 0x27, 0x10, 0xc3, 0xc7, 0xc1, 0xdb } };

struct st_uuid StreamACD::GetVendorUuid()
{
    return qc_acd_uuid;
}

int32_t StreamACD::SetupStreamConfig(const struct st_uuid *vendor_uuid)
{
    int32_t status = 0;
    class SoundTriggerUUID uuid;

    PAL_DBG(LOG_TAG, "Enter");
    GetUUID(&uuid, vendor_uuid);

    sm_cfg_ = acd_info_->GetStreamConfig(uuid);
    if (!sm_cfg_) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Failed to get stream config", status);
        goto exit;
    }
    mStreamSelector = sm_cfg_->GetStreamConfigName();
    mInstanceID = rm->getStreamInstanceID(this);
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::SetupDetectionEngine()
{
    int status = 0;
    pal_device_id_t dev_id;
    std::shared_ptr<Device> dev = nullptr;
    std::unique_lock<std::mutex> lck(mutex_, std::defer_lock);

    PAL_DBG(LOG_TAG, "Enter");
    if (sm_cfg_ == NULL) {
        /* Use QC ACD as default streamConfig */
        status = SetupStreamConfig(&qc_acd_uuid);
        if (status) {
            PAL_ERR(LOG_TAG, "Error:%d Failed to setup Stream Config", status);
            goto error_exit;
        }
    }

    // update best device
    dev_id = GetAvailCaptureDevice();
    PAL_DBG(LOG_TAG, "Select available caputre device %d", dev_id);

    dev = GetPalDevice(this, dev_id);
    if (!dev) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Device creation is failed", status);
        goto error_exit;
    }
    mDevices.clear();
    mDevices.push_back(dev);

    cap_prof_ = GetCurrentCaptureProfile();
    mDevPPSelector = cap_prof_->GetName();

    /* lock here to prevent multiple ACD engine instances to be created */
    lck.lock();
    engine_ = ContextDetectionEngine::Create(this, sm_cfg_);
    lck.unlock();

    if (!engine_) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d engine creation failed", status);
        goto error_exit;
    }

    status = engine_->SetupEngine(this, (void *)context_config_);
    if (status) {
        PAL_ERR(LOG_TAG, "Error:%d setup engine failed", status);
        goto error_exit;
    }

    TransitTo(ACD_STATE_LOADED);
error_exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::UpdateRecognitionConfig(struct acd_recognition_cfg *acd_recog_cfg)
{
    int32_t status = 0;
    struct acd_per_context_cfg *context_cfg = NULL;
    uint32_t i, num_contexts = 0;
    uint8_t *opaque_ptr = (uint8_t *)acd_recog_cfg;
    size_t len = 0;

    PAL_DBG(LOG_TAG, "Enter");

    opaque_ptr += sizeof(struct acd_recognition_cfg);
    context_cfg = (struct acd_per_context_cfg *)opaque_ptr;

    num_contexts = acd_recog_cfg->num_contexts;
    PAL_INFO(LOG_TAG, "Num Contexts =  %d", acd_recog_cfg->num_contexts);

    /* During transition from LPI<->NLPI and vice-versa, rec_config_ remains same.
     * Continue with engine setup if it is same.
     */
    if (rec_config_ == acd_recog_cfg)
        goto exit;

    if (rec_config_)
        free(rec_config_);

    rec_config_ = (struct acd_recognition_cfg *)calloc(1,
        sizeof(struct acd_recognition_cfg) +
        (num_contexts * sizeof(struct acd_per_context_cfg)));
    if (!rec_config_) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate rec_config", status);
        goto exit;
    }
    ar_mem_cpy(rec_config_, sizeof(struct acd_recognition_cfg),
                     acd_recog_cfg, sizeof(struct acd_recognition_cfg));
    ar_mem_cpy((uint8_t *)rec_config_ + sizeof(struct acd_recognition_cfg),
                     (num_contexts * sizeof(struct acd_per_context_cfg)),
                     context_cfg,
                     (num_contexts * sizeof(struct acd_per_context_cfg)));

    // dump acd recognition config data
    if (acd_info_->GetEnableDebugDumps()) {
        ST_DBG_DECLARE(FILE *rec_opaque_fd = NULL; static int rec_opaque_cnt = 0);
        ST_DBG_FILE_OPEN_WR(rec_opaque_fd, ST_DEBUG_DUMP_LOCATION,
            "acd_rec_config", "bin", rec_opaque_cnt);
        ST_DBG_FILE_WRITE(rec_opaque_fd,
            (uint8_t *)rec_config_, sizeof(struct acd_recognition_cfg) +
            (num_contexts * sizeof(struct acd_per_context_cfg)));
        ST_DBG_FILE_CLOSE(rec_opaque_fd);
        PAL_DBG(LOG_TAG, "acd_recognition_cfg data stored in: acd_rec_config_%d.bin",
            rec_opaque_cnt);
        rec_opaque_cnt++;
    }

   /* Fill context_cfg from recog_cfg data and use that for engine setup */
    len = sizeof(struct pal_param_context_list) + (num_contexts * sizeof(uint32_t));
    context_config_ = (struct pal_param_context_list *) calloc(1, len);
    if (!context_config_) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate context_config", status);
        goto exit;
    }
    context_config_->num_contexts = num_contexts;
    for (i = 0; i < num_contexts; i++)
        context_config_->context_id[i] = context_cfg[i].context_id;

    // dump acd context config data
    if (acd_info_->GetEnableDebugDumps()) {
        ST_DBG_DECLARE(FILE *ctx_opaque_fd = NULL; static int ctx_opaque_cnt = 0);
        ST_DBG_FILE_OPEN_WR(ctx_opaque_fd, ST_DEBUG_DUMP_LOCATION,
            "acd_context_config", "bin", ctx_opaque_cnt);
        ST_DBG_FILE_WRITE(ctx_opaque_fd,
            (uint8_t *)context_config_, len);
        ST_DBG_FILE_CLOSE(ctx_opaque_fd);
        PAL_DBG(LOG_TAG, "acd_context_cfg data stored in: acd_context_config_%d.bin",
            ctx_opaque_cnt);
        ctx_opaque_cnt++;
    }
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamACD::SendRecognitionConfig(struct acd_recognition_cfg *acd_recog_cfg)
{
    int32_t status = 0;
    struct pal_param_context_list *old_ctx_cfg = NULL;

    if (context_config_)
         old_ctx_cfg = context_config_;

    status = UpdateRecognitionConfig(acd_recog_cfg);
    if (status)
        goto exit;

    if (cur_state_->GetStateId() >= ACD_STATE_LOADED) {
        status = engine_->ReconfigureEngine(this, (void *)old_ctx_cfg, (void *)context_config_);
        if (old_ctx_cfg)
            free(old_ctx_cfg);
    } else {
        status = SetupDetectionEngine();
    }

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamACD::UpdateContextConfig(struct pal_param_context_list *config)
{
    int32_t status = 0;
    size_t len  = 0;

    if (context_config_ == config)
        goto exit;

    len = sizeof(struct pal_param_context_list) +
        (config->num_contexts * sizeof(uint32_t));

    context_config_ = (struct pal_param_context_list *) calloc(1, len);
    if (!context_config_) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate context_config", status);
        goto exit;
    }
    ar_mem_cpy(context_config_, len, config, len);

    // dump acd context config data
    if (acd_info_->GetEnableDebugDumps()) {
        ST_DBG_DECLARE(FILE *ctx_opaque_fd = NULL; static int ctx_opaque_cnt = 0);
        ST_DBG_FILE_OPEN_WR(ctx_opaque_fd, ST_DEBUG_DUMP_LOCATION,
            "acd_context_config", "bin", ctx_opaque_cnt);
        ST_DBG_FILE_WRITE(ctx_opaque_fd,
            (uint8_t *)context_config_, len);
        ST_DBG_FILE_CLOSE(ctx_opaque_fd);
        PAL_ERR(LOG_TAG, "acd_context_cfg data stored in: acd_context_config_%d.bin",
            ctx_opaque_cnt);
        ctx_opaque_cnt++;
    }
exit:
    return status;
}

int32_t StreamACD::SendContextConfig(struct pal_param_context_list *config)
{
    int32_t status = 0;
    struct pal_param_context_list *old_ctx_cfg = NULL;

    if (context_config_)
        old_ctx_cfg = context_config_;

    status = UpdateContextConfig(config);
    if (status)
       goto exit;

    if (cur_state_->GetStateId() >= ACD_STATE_LOADED) {
        status = engine_->ReconfigureEngine(this, (void *)old_ctx_cfg, (void *)context_config_);
        if (old_ctx_cfg)
            free(old_ctx_cfg);
    } else {
        status = SetupDetectionEngine();
    }

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

void StreamACD::EventNotificationThread(StreamACD *stream)
{
    PAL_DBG(LOG_TAG, "Enter. start thread loop");

    std::unique_lock<std::mutex> lck(stream->mutex_);
    while (!stream->exit_thread_) {
        PAL_DBG(LOG_TAG, "waiting on cond");
        stream->cv_.wait(lck);
        PAL_INFO(LOG_TAG, "Received start recognition");

        if (stream->exit_thread_) {
            PAL_DBG(LOG_TAG, "Exit thread");
            break;
        }

        stream->SendCachedEventData();
    }
    PAL_DBG(LOG_TAG, "Exit");
}

int32_t StreamACD::ACDIdle::ProcessEvent(
    std::shared_ptr<ACDEventConfig> ev_cfg)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "ACDIdle: handle event %d for stream instance %u",
        ev_cfg->id_, acd_stream_.mInstanceID);

    switch (ev_cfg->id_) {
        case ACD_EV_LOAD_SOUND_MODEL: {
            ACDLoadEventConfigData *data =
                (ACDLoadEventConfigData *)ev_cfg->data_.get();
            struct pal_st_sound_model *pal_acd_sm;

            pal_acd_sm = (struct pal_st_sound_model *)data->data_;
            acd_stream_.SetupStreamConfig(&pal_acd_sm->vendor_uuid);
            break;
        }
        case ACD_EV_RECOGNITION_CONFIG: {
            ACDRecognitionCfgEventConfigData *data =
                (ACDRecognitionCfgEventConfigData *)ev_cfg->data_.get();
            status = acd_stream_.SendRecognitionConfig(
               (struct acd_recognition_cfg *)data->data_);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send recog config", status);

            break;
        }
        case ACD_EV_CONTEXT_CONFIG: {
            ACDContextCfgEventConfigData *data =
                (ACDContextCfgEventConfigData *)ev_cfg->data_.get();
            status = acd_stream_.SendContextConfig(
               (struct pal_param_context_list *)data->data_);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to send recog config, status %d",
                    status);
            }
            break;
        }
        case ACD_EV_PAUSE: {
            acd_stream_.paused_ = true;
            break;
        }
        case ACD_EV_RESUME: {
            acd_stream_.paused_ = false;
            break;
        }
        case ACD_EV_DEVICE_DISCONNECTED: {
            ACDDeviceDisconnectedEventConfigData *data =
                (ACDDeviceDisconnectedEventConfigData *)ev_cfg->data_.get();
            pal_device_id_t device_id = data->dev_id_;
            if (acd_stream_.mDevices.size() == 0) {
                PAL_DBG(LOG_TAG, "No device to disconnect");
                break;
            } else {
                int curr_device_id = acd_stream_.mDevices[0]->getSndDeviceId();
                pal_device_id_t curr_device =
                    static_cast<pal_device_id_t>(curr_device_id);
                if (curr_device != device_id) {
                    PAL_ERR(LOG_TAG, "Error:%d Device %d not connected, ignore",
                        -EINVAL, device_id);
                    break;
                }
            }
            acd_stream_.mDevices.clear();
            break;
        }
        case ACD_EV_DEVICE_CONNECTED: {
            std::shared_ptr<Device> dev = nullptr;
            ACDDeviceConnectedEventConfigData *data =
                (ACDDeviceConnectedEventConfigData *)ev_cfg->data_.get();
            pal_device_id_t dev_id = data->dev_id_;

            dev = acd_stream_.GetPalDevice(&acd_stream_, dev_id);
            if (!dev) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Error:%d Device creation failed", status);
                goto connect_err;
            }

            acd_stream_.mDevices.clear();
            acd_stream_.mDevices.push_back(dev);
        connect_err:
            break;
        }
        case ACD_EV_CONCURRENT_STREAM: {
            PAL_INFO(LOG_TAG, "no action needed for concurrent stream in idle state");
            break;
        }
        case ACD_EV_SSR_OFFLINE:
            if (acd_stream_.state_for_restore_ == ACD_STATE_NONE)
                acd_stream_.state_for_restore_ = ACD_STATE_IDLE;

            TransitTo(ACD_STATE_SSR);
            break;
        default:
            PAL_DBG(LOG_TAG, "Unhandled event %d", ev_cfg->id_);
            break;
    }
    return status;
}

int32_t StreamACD::ACDLoaded::ProcessEvent(
    std::shared_ptr<ACDEventConfig> ev_cfg)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "ACDLoaded: handle event %d for stream instance %u",
        ev_cfg->id_, acd_stream_.mInstanceID);

    switch (ev_cfg->id_) {
        case ACD_EV_UNLOAD_SOUND_MODEL: {
            status = acd_stream_.engine_->TeardownEngine(&acd_stream_, acd_stream_.context_config_);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Unload engine failed", status);

            acd_stream_.rm->resetStreamInstanceID(&acd_stream_, acd_stream_.mInstanceID);

            TransitTo(ACD_STATE_IDLE);
            break;
        }
        case ACD_EV_RESUME: {
            acd_stream_.paused_ = false;
            if (!acd_stream_.isActive()) {
                // Possible if App has stopped recognition during active
                // concurrency.
                break;
            }
            // fall through to start
            [[fallthrough]];
        }
        case ACD_EV_START_RECOGNITION: {
            if (acd_stream_.paused_) {
               break; // Concurrency is active, start later.
            }
            auto& dev = acd_stream_.mDevices[0];

            /* Update cap dev based on mode and configuration and start it */
            struct pal_device dattr;
            bool backend_update = false;
            std::shared_ptr<CaptureProfile> cap_prof = nullptr;

            // Do not update capture profile when resuming stream
            if (ev_cfg->id_ == ACD_EV_START_RECOGNITION ||
               (ev_cfg->id_ == ACD_EV_RESUME &&
                !acd_stream_.rm->GetSoundTriggerCaptureProfile())) {
                backend_update = acd_stream_.rm->UpdateSoundTriggerCaptureProfile(
                    &acd_stream_, true);
                if (backend_update) {
                    status = rm->StopOtherDetectionStreams(&acd_stream_);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to stop other Detection streams", status);

                    status = rm->StartOtherDetectionStreams(&acd_stream_);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to start other Detection streams", status);
                }
            }

            dev->getDeviceAttributes(&dattr);

            cap_prof = acd_stream_.rm->GetSoundTriggerCaptureProfile();
            if (!cap_prof) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Error:%d Invalid capture profile", status);
                break;
            }

            dattr.config.bit_width = cap_prof->GetBitWidth();
            dattr.config.ch_info.channels = cap_prof->GetChannels();
            dattr.config.sample_rate = cap_prof->GetSampleRate();
            dev->setDeviceAttributes(dattr);

            PAL_INFO(LOG_TAG, "updated device attr dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    cap_prof->GetDevId(), cap_prof->GetChannels(),
                    cap_prof->GetSampleRate(), cap_prof->isECRequired());

            /* now start the device */
            PAL_DBG(LOG_TAG, "Start device %d-%s", dev->getSndDeviceId(),
                    dev->getPALDeviceName().c_str());
            dev->setSndName(cap_prof->GetSndName());

            if (!acd_stream_.device_opened_) {
                acd_stream_.rm->voteSleepMonitor(&acd_stream_, true);
                status = dev->open();
                acd_stream_.rm->voteSleepMonitor(&acd_stream_, false);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error:%d Device open failed", status);
                    break;
                }
                acd_stream_.device_opened_ = true;
            }

            status = dev->start();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d Device start failed", status);
                break;
            } else {
                acd_stream_.rm->registerDevice(dev, &acd_stream_);
            }
            PAL_DBG(LOG_TAG, "device started");

            /* Start the engines */
            status = acd_stream_.engine_->StartEngine(&acd_stream_);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d Start acd engine failed", status);
                goto err_exit;
            }
            TransitTo(ACD_STATE_ACTIVE);
            if (acd_stream_.state_for_restore_ == ACD_STATE_DETECTED) {
                TransitTo(ACD_STATE_DETECTED);
                acd_stream_.state_for_restore_ = ACD_STATE_NONE;
            } else if (acd_stream_.cached_event_data_) {
                std::unique_lock<std::mutex> lck(acd_stream_.mutex_);
                acd_stream_.cv_.notify_one();
                TransitTo(ACD_STATE_DETECTED);
            }
            break;
        err_exit:
            if (acd_stream_.mDevices.size() > 0) {
                acd_stream_.rm->deregisterDevice(acd_stream_.mDevices[0], &acd_stream_);
                acd_stream_.mDevices[0]->stop();
            }
            break;
        }
        case ACD_EV_PAUSE: {
            acd_stream_.paused_ = true;
            break;
        }
        case ACD_EV_DEVICE_DISCONNECTED:{
            ACDDeviceDisconnectedEventConfigData *data =
                (ACDDeviceDisconnectedEventConfigData *)ev_cfg->data_.get();
            pal_device_id_t device_id = data->dev_id_;

            if (acd_stream_.mDevices.size() == 0) {
                PAL_DBG(LOG_TAG, "No device to disconnect");
                break;
            } else {
                int curr_device_id = acd_stream_.mDevices[0]->getSndDeviceId();
                pal_device_id_t curr_device =
                    static_cast<pal_device_id_t>(curr_device_id);
                if (curr_device != device_id) {
                    PAL_ERR(LOG_TAG, "Error:%d Device %d not connected, ignore",
                        -EINVAL, device_id);
                    break;
                }
            }
            auto& dev = acd_stream_.mDevices[0];

            acd_stream_.engine_->DisconnectSessionDevice(&acd_stream_,
                    acd_stream_.mStreamAttr->type, dev);

            status = dev->close();
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d dev close failed", status);
            acd_stream_.device_opened_ = false;

            acd_stream_.mDevices.clear();
            break;
        }
        case ACD_EV_DEVICE_CONNECTED: {
            std::shared_ptr<Device> dev = nullptr;
            ACDDeviceConnectedEventConfigData *data =
                (ACDDeviceConnectedEventConfigData *)ev_cfg->data_.get();
            pal_device_id_t dev_id = data->dev_id_;

            dev = acd_stream_.GetPalDevice(&acd_stream_, dev_id);
            if (!dev) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Error:%d Dev creation failed", status);
                goto connect_err;
            }

            acd_stream_.mDevices.clear();
            acd_stream_.mDevices.push_back(dev);

            PAL_DBG(LOG_TAG, "Update capture profile before SetupSessionDevice");
            acd_stream_.cap_prof_ = acd_stream_.GetCurrentCaptureProfile();
            acd_stream_.mDevPPSelector = acd_stream_.cap_prof_->GetName();

            status = acd_stream_.engine_->SetupSessionDevice(&acd_stream_,
                acd_stream_.mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d setupSessionDevice for %d failed",
                        status, dev->getSndDeviceId());
                goto connect_err;
            }
            if (!acd_stream_.device_opened_) {
                acd_stream_.rm->voteSleepMonitor(&acd_stream_, true);
                status = dev->open();
                acd_stream_.rm->voteSleepMonitor(&acd_stream_, false);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error:%d device %d open failed",
                        status, dev->getSndDeviceId());
                    goto connect_err;
                }
                acd_stream_.device_opened_ = true;
            }

            if (acd_stream_.isActive()) {
                status = dev->start();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error:%d device %d start failed",
                        status, dev->getSndDeviceId());
                    dev->close();
                    acd_stream_.device_opened_ = false;
                    goto connect_err;
                }
            }

            status = acd_stream_.engine_->ConnectSessionDevice(&acd_stream_,
                acd_stream_.mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d connectSessionDevice for %d failed",
                      status, dev->getSndDeviceId());
                dev->close();
                acd_stream_.device_opened_ = false;
            } else {
                if (acd_stream_.isActive())
                    acd_stream_.rm->registerDevice(dev, &acd_stream_);
            }


        connect_err:
            break;
        }
        case ACD_EV_RECOGNITION_CONFIG: {
            ACDRecognitionCfgEventConfigData *data =
                (ACDRecognitionCfgEventConfigData *)ev_cfg->data_.get();
            status = acd_stream_.SendRecognitionConfig(
               (struct acd_recognition_cfg *)data->data_);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send recog config", status);

            break;
        }
        case ACD_EV_CONTEXT_CONFIG: {
            ACDContextCfgEventConfigData *data =
                (ACDContextCfgEventConfigData *)ev_cfg->data_.get();
            status = acd_stream_.SendContextConfig(
               (struct pal_param_context_list *)data->data_);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send context config", status);
            break;
        }
        case ACD_EV_EC_REF: {
            ACDECRefEventConfigData *data =
                (ACDECRefEventConfigData *)ev_cfg->data_.get();
            Stream *s = static_cast<Stream *>(&acd_stream_);
            status = acd_stream_.engine_->setECRef(s, data->dev_,
                data->is_enable_);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to set EC Ref in engine", status);
            break;
        }
        case ACD_EV_CONCURRENT_STREAM: {
            std::shared_ptr<CaptureProfile> new_cap_prof = nullptr;
            bool active = false;

            ACDConcurrentStreamEventConfigData *data =
                (ACDConcurrentStreamEventConfigData *)ev_cfg->data_.get();
            active = data->is_active_;
            new_cap_prof = acd_stream_.GetCurrentCaptureProfile();
            if (!new_cap_prof) {
                PAL_ERR(LOG_TAG, "Failed to get new capture profile");
                status = -EINVAL;
                break;
            }
            if (acd_stream_.cap_prof_ != new_cap_prof) {
                PAL_DBG(LOG_TAG,
                    "current capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    acd_stream_.cap_prof_->GetName().c_str(),
                    acd_stream_.cap_prof_->GetDevId(),
                    acd_stream_.cap_prof_->GetChannels(),
                    acd_stream_.cap_prof_->GetSampleRate(),
                    acd_stream_.cap_prof_->isECRequired());
                PAL_DBG(LOG_TAG,
                    "new capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    new_cap_prof->GetName().c_str(),
                    new_cap_prof->GetDevId(),
                    new_cap_prof->GetChannels(),
                    new_cap_prof->GetSampleRate(),
                    new_cap_prof->isECRequired());
                if (!active) {
                    std::shared_ptr<ACDEventConfig> ev_cfg1(
                        new ACDDeviceDisconnectedEventConfig(acd_stream_.GetAvailCaptureDevice()));
                    status = acd_stream_.ProcessInternalEvent(ev_cfg1);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d", status,
                                    acd_stream_.GetAvailCaptureDevice());
                } else {
                    std::shared_ptr<ACDEventConfig> ev_cfg1(
                        new ACDDeviceConnectedEventConfig(acd_stream_.GetAvailCaptureDevice()));
                    status = acd_stream_.ProcessInternalEvent(ev_cfg1);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d", status, acd_stream_.GetAvailCaptureDevice());
                }
            } else {
              PAL_INFO(LOG_TAG,"no action needed, same capture profile");
            }
            break;
        }
        case ACD_EV_DETECTED: {
            ACDDetectedEventConfigData *data =
                (ACDDetectedEventConfigData *) ev_cfg->data_.get();
            std::unique_lock<std::mutex> lck(acd_stream_.mutex_);
            acd_stream_.CacheEventData((struct acd_context_event *)data->data_);
            break;
        }
        case ACD_EV_SSR_OFFLINE: {
            if (acd_stream_.state_for_restore_ == ACD_STATE_NONE) {
                acd_stream_.state_for_restore_ = ACD_STATE_LOADED;
            }
            std::shared_ptr<ACDEventConfig> ev_cfg(new ACDUnloadEventConfig());
            status = acd_stream_.ProcessInternalEvent(ev_cfg);
            TransitTo(ACD_STATE_SSR);
            break;
        }
        default: {
            PAL_DBG(LOG_TAG, "Unhandled event %d", ev_cfg->id_);
            break;
        }
    }
    return status;
}

int32_t StreamACD::ACDActive::ProcessEvent(
    std::shared_ptr<ACDEventConfig> ev_cfg)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "ACDActive handle event %d for stream instance %u",
        ev_cfg->id_, acd_stream_.mInstanceID);

    switch (ev_cfg->id_) {
        case ACD_EV_DETECTED: {
            ACDDetectedEventConfigData *data =
                (ACDDetectedEventConfigData *) ev_cfg->data_.get();
            std::unique_lock<std::mutex> lck(acd_stream_.mutex_);
            acd_stream_.CacheEventData((struct acd_context_event *)data->data_);
            if (acd_stream_.cached_event_data_) {
                if (acd_stream_.notificationInProgress == true)
                    acd_stream_.deferredNotification = true;
                else
                    acd_stream_.cv_.notify_one();
                TransitTo(ACD_STATE_DETECTED);
            }
            break;
        }
        case ACD_EV_PAUSE: {
            acd_stream_.paused_ = true;
            // fall through to stop
            [[fallthrough]];
        }
        case ACD_EV_UNLOAD_SOUND_MODEL:
        case ACD_EV_STOP_RECOGNITION: {
            // Do not update capture profile when pausing stream
            bool backend_update = false;
            if (ev_cfg->id_ == ACD_EV_STOP_RECOGNITION ||
                ev_cfg->id_ == ACD_EV_UNLOAD_SOUND_MODEL) {
                backend_update = acd_stream_.rm->UpdateSoundTriggerCaptureProfile(
                    &acd_stream_, false);
                if (backend_update) {
                    status = rm->StopOtherDetectionStreams(&acd_stream_);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to stop other Detection streams", status);
                }
            }

            PAL_VERBOSE(LOG_TAG, "Stop engine");
            status = acd_stream_.engine_->StopEngine(&acd_stream_);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Stop engine failed", status);

            auto& dev = acd_stream_.mDevices[0];
            PAL_DBG(LOG_TAG, "Stop device %d-%s", dev->getSndDeviceId(),
                    dev->getPALDeviceName().c_str());
            status = dev->stop();
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Device stop failed", status);

            acd_stream_.rm->deregisterDevice(dev, &acd_stream_);

            status = dev->close();
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Device close failed", status);
            acd_stream_.device_opened_ = false;

            if (backend_update) {
                status = rm->StartOtherDetectionStreams(&acd_stream_);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to start other Detection streams", status);
            }

            TransitTo(ACD_STATE_LOADED);
            if (ev_cfg->id_ == ACD_EV_UNLOAD_SOUND_MODEL) {
                status = acd_stream_.ProcessInternalEvent(ev_cfg);
                if (status != 0)
                    PAL_ERR(LOG_TAG, "Failed to unload sound model, status = %d", status);
            }
            break;
        }
        case ACD_EV_DEVICE_DISCONNECTED: {
            ACDDeviceDisconnectedEventConfigData *data =
                (ACDDeviceDisconnectedEventConfigData *)ev_cfg->data_.get();
            pal_device_id_t device_id = data->dev_id_;

            int curr_device_id = acd_stream_.mDevices[0]->getSndDeviceId();
            if (curr_device_id != device_id) {
                PAL_ERR(LOG_TAG, "Error:%d Device %d not connected, ignore",
                    -EINVAL, device_id);
                break;
            }
            auto& dev = acd_stream_.mDevices[0];

            acd_stream_.rm->deregisterDevice(dev, &acd_stream_);

            acd_stream_.engine_->DisconnectSessionDevice(&acd_stream_,
                acd_stream_.mStreamAttr->type, dev);

            status = dev->stop();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d device stop failed", status);
                goto disconnect_err;
            }

            status = dev->close();
            acd_stream_.device_opened_ = false;
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d device close failed", status);
                goto disconnect_err;
            }
        disconnect_err:
            acd_stream_.mDevices.clear();
            break;
        }
        case ACD_EV_DEVICE_CONNECTED: {
            ACDDeviceConnectedEventConfigData *data =
                (ACDDeviceConnectedEventConfigData *)ev_cfg->data_.get();
            pal_device_id_t dev_id = data->dev_id_;
            std::shared_ptr<Device> dev = nullptr;

            dev = acd_stream_.GetPalDevice(&acd_stream_, dev_id);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Error:%d Device creation failed", -EINVAL);
                status = -EINVAL;
                break;
            }
            if (!acd_stream_.device_opened_) {
                acd_stream_.rm->voteSleepMonitor(&acd_stream_, true);
                status = dev->open();
                acd_stream_.rm->voteSleepMonitor(&acd_stream_, false);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error:%d device %d open failed", status,
                        dev->getSndDeviceId());
                    break;
                }
                acd_stream_.device_opened_ = true;
            }

            acd_stream_.mDevices.clear();
            acd_stream_.mDevices.push_back(dev);

            PAL_DBG(LOG_TAG, "Update capture profile before SetupSessionDevice");
            acd_stream_.cap_prof_ = acd_stream_.GetCurrentCaptureProfile();
            acd_stream_.mDevPPSelector = acd_stream_.cap_prof_->GetName();

            status = acd_stream_.engine_->SetupSessionDevice(&acd_stream_,
                acd_stream_.mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d setupSessionDevice for %d failed",
                        status, dev->getSndDeviceId());
                dev->close();
                acd_stream_.device_opened_ = false;
                break;
            }

            status = dev->start();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d device %d start failed",
                    status, dev->getSndDeviceId());
                break;
            }

            status = acd_stream_.engine_->ConnectSessionDevice(&acd_stream_,
                acd_stream_.mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d connectSessionDevice for %d failed",
                      status, dev->getSndDeviceId());
                dev->close();
                acd_stream_.device_opened_ = false;
            } else {
                acd_stream_.rm->registerDevice(dev, &acd_stream_);
            }
            break;
        }
        case ACD_EV_RECOGNITION_CONFIG: {
            ACDRecognitionCfgEventConfigData *data =
                (ACDRecognitionCfgEventConfigData *)ev_cfg->data_.get();
            status = acd_stream_.SendRecognitionConfig(
               (struct acd_recognition_cfg *)data->data_);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send recog config", status);

            break;
        }
        case ACD_EV_CONTEXT_CONFIG: {
            ACDContextCfgEventConfigData *data =
                (ACDContextCfgEventConfigData *)ev_cfg->data_.get();
            status = acd_stream_.SendContextConfig(
               (struct pal_param_context_list *)data->data_);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send context config", status);

            break;
        }
        case ACD_EV_EC_REF: {
            ACDECRefEventConfigData *data =
                (ACDECRefEventConfigData *)ev_cfg->data_.get();
            Stream *s = static_cast<Stream *>(&acd_stream_);
            status = acd_stream_.engine_->setECRef(s, data->dev_,
                data->is_enable_);
            if (status) {
                PAL_ERR(LOG_TAG, "Error:%d Failed to set EC Ref in engine", status);
            }
            break;
        }
        case ACD_EV_CONCURRENT_STREAM: {
            std::shared_ptr<CaptureProfile> new_cap_prof = nullptr;
            bool active = false;

            ACDConcurrentStreamEventConfigData *data =
                (ACDConcurrentStreamEventConfigData *)ev_cfg->data_.get();
            active = data->is_active_;
            new_cap_prof = acd_stream_.GetCurrentCaptureProfile();
            if (!new_cap_prof) {
                PAL_ERR(LOG_TAG, "Failed to initialize new capture profile");
                status = -EINVAL;
                break;
            }
            if (acd_stream_.cap_prof_ != new_cap_prof) {
                PAL_DBG(LOG_TAG,
                    "current capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    acd_stream_.cap_prof_->GetName().c_str(),
                    acd_stream_.cap_prof_->GetDevId(),
                    acd_stream_.cap_prof_->GetChannels(),
                    acd_stream_.cap_prof_->GetSampleRate(),
                    acd_stream_.cap_prof_->isECRequired());
                PAL_DBG(LOG_TAG,
                    "new capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    new_cap_prof->GetName().c_str(),
                    new_cap_prof->GetDevId(),
                    new_cap_prof->GetChannels(),
                    new_cap_prof->GetSampleRate(),
                    new_cap_prof->isECRequired());
                if (!active) {
                    std::shared_ptr<ACDEventConfig> ev_cfg1(
                        new ACDDeviceDisconnectedEventConfig(acd_stream_.GetAvailCaptureDevice()));
                    status = acd_stream_.ProcessInternalEvent(ev_cfg1);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d", status,
                                    acd_stream_.GetAvailCaptureDevice());
                } else {
                    std::shared_ptr<ACDEventConfig> ev_cfg1(
                        new ACDDeviceConnectedEventConfig(acd_stream_.GetAvailCaptureDevice()));
                    status = acd_stream_.ProcessInternalEvent(ev_cfg1);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d", status, acd_stream_.GetAvailCaptureDevice());
                }
            } else {
              PAL_INFO(LOG_TAG,"no action needed, same capture profile");
            }
            break;
        }
        case ACD_EV_SSR_OFFLINE: {
            if (acd_stream_.state_for_restore_ == ACD_STATE_NONE) {
                acd_stream_.state_for_restore_ = ACD_STATE_ACTIVE;
            }
            std::shared_ptr<ACDEventConfig> ev_cfg1(
                new ACDUnloadEventConfig());
            status = acd_stream_.ProcessInternalEvent(ev_cfg1);
            TransitTo(ACD_STATE_SSR);
            break;
        }
        case ACD_EV_RESUME: {
            acd_stream_.paused_ = false;
            if (false == acd_stream_.engine_->isEngActive()) {
                // Possible if ACD was paused by another
                // ST stream while in detected state.
                TransitTo(ACD_STATE_LOADED);
                status = acd_stream_.ProcessInternalEvent(ev_cfg);
            }
            break;
        }
        default: {
            PAL_DBG(LOG_TAG, "Unhandled event %d", ev_cfg->id_);
            break;
        }
    }
    if (acd_stream_.state_for_restore_ == ACD_STATE_DETECTED) {
        TransitTo(ACD_STATE_DETECTED);
        acd_stream_.state_for_restore_ = ACD_STATE_NONE;
    }
    return status;
}

int32_t StreamACD::ACDDetected::ProcessEvent(
    std::shared_ptr<ACDEventConfig> ev_cfg)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "ACDDetected: handle event %d for stream instance %u",
        ev_cfg->id_, acd_stream_.mInstanceID);

    switch (ev_cfg->id_) {
        case ACD_EV_DETECTED: {
            ACDDetectedEventConfigData *data =
                (ACDDetectedEventConfigData *) ev_cfg->data_.get();
            std::unique_lock<std::mutex> lck(acd_stream_.mutex_);
            acd_stream_.CacheEventData((struct acd_context_event *)data->data_);
            break;
        }
        case ACD_EV_START_RECOGNITION: {
            /* notify client if events are present , else move to active */
            std::unique_lock<std::mutex> lck(acd_stream_.mutex_);
            if (acd_stream_.cached_event_data_) {
                if (acd_stream_.notificationInProgress == true)
                    acd_stream_.deferredNotification = true;
                else
                    acd_stream_.cv_.notify_one();
            } else {
                TransitTo(ACD_STATE_ACTIVE);
            }

            break;
        }
        case ACD_EV_RESUME:
            acd_stream_.state_for_restore_ = ACD_STATE_DETECTED;
            TransitTo(ACD_STATE_LOADED);
            status = acd_stream_.ProcessInternalEvent(ev_cfg);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to process event %d", status, ev_cfg->id_);
            break;
        case ACD_EV_UNLOAD_SOUND_MODEL:
        case ACD_EV_STOP_RECOGNITION:
            /* if stream already in stopped state, move to LOADED state */
            if (acd_stream_.paused_ == true)
                TransitTo(ACD_STATE_LOADED);
            else
                TransitTo(ACD_STATE_ACTIVE);

            status = acd_stream_.ProcessInternalEvent(ev_cfg);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to process event %d", status, ev_cfg->id_);
            break;
        case ACD_EV_SSR_OFFLINE:
            TransitTo(ACD_STATE_ACTIVE);
            status = acd_stream_.ProcessInternalEvent(ev_cfg);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to process event %d", status, ev_cfg->id_);
            acd_stream_.state_for_restore_ = ACD_STATE_DETECTED;
            break;
        case ACD_EV_RECOGNITION_CONFIG:
        case ACD_EV_CONTEXT_CONFIG:
        case ACD_EV_EC_REF:
        case ACD_EV_DEVICE_DISCONNECTED:
        case ACD_EV_DEVICE_CONNECTED:
        case ACD_EV_CONCURRENT_STREAM:
        case ACD_EV_PAUSE:
            acd_stream_.state_for_restore_ = ACD_STATE_DETECTED;
            // fall through to default
            [[fallthrough]];
        default: {
            TransitTo(ACD_STATE_ACTIVE);
            status = acd_stream_.ProcessInternalEvent(ev_cfg);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to process event %d", status, ev_cfg->id_);

            break;
        }
    }

    return status;
}

int32_t StreamACD::ACDSSR::ProcessEvent(std::shared_ptr<ACDEventConfig> ev_cfg)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "ACDSSR: handle event %d for stream instance %u",
        ev_cfg->id_, acd_stream_.mInstanceID);

    switch (ev_cfg->id_) {
        case ACD_EV_SSR_ONLINE: {
            TransitTo(ACD_STATE_IDLE);

            if (acd_stream_.state_for_restore_ == ACD_STATE_LOADED ||
                acd_stream_.state_for_restore_ == ACD_STATE_ACTIVE ||
                acd_stream_.state_for_restore_ == ACD_STATE_DETECTED) {
                if (acd_stream_.rec_config_) {
                    status = acd_stream_.SendRecognitionConfig(
                        acd_stream_.rec_config_);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG, "Error:%d Failed to send recognition config",
                                status);
                        break;
                    }
                } else if (acd_stream_.context_config_) {
                     status = acd_stream_.SendContextConfig(acd_stream_.context_config_);
                     if (0 != status) {
                         PAL_ERR(LOG_TAG, "Error:%d Failed to send context config", status);
                         break;
                     }
                }
            }

            if (acd_stream_.state_for_restore_ == ACD_STATE_ACTIVE ||
                acd_stream_.state_for_restore_ == ACD_STATE_DETECTED) {
                std::shared_ptr<ACDEventConfig> ev_cfg1(
                    new ACDStartRecognitionEventConfig(false));
                status = acd_stream_.ProcessInternalEvent(ev_cfg1);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Failed to Start, status %d", status);
                    break;
                }
            }
            if (acd_stream_.state_for_restore_ == ACD_STATE_DETECTED)
                TransitTo(ACD_STATE_DETECTED);

            acd_stream_.state_for_restore_ = ACD_STATE_NONE;
            break;
        }
        case ACD_EV_LOAD_SOUND_MODEL: {
            if (acd_stream_.state_for_restore_ != ACD_STATE_IDLE) {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                    acd_stream_.state_for_restore_);
                status = -EINVAL;
            } else {
                ACDLoadEventConfigData *data =
                    (ACDLoadEventConfigData *)ev_cfg->data_.get();
                struct pal_st_sound_model *pal_acd_sm;

                pal_acd_sm = (struct pal_st_sound_model *)data->data_;
                status = acd_stream_.SetupStreamConfig(&pal_acd_sm->vendor_uuid);
            }
            break;
        }
        case ACD_EV_UNLOAD_SOUND_MODEL: {
            if (acd_stream_.state_for_restore_ < ACD_STATE_LOADED) {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                    acd_stream_.state_for_restore_);
                status = -EINVAL;
            } else {
                acd_stream_.state_for_restore_ = ACD_STATE_IDLE;
            }
            break;
        }
        case ACD_EV_RECOGNITION_CONFIG: {
            ACDRecognitionCfgEventConfigData *data =
                (ACDRecognitionCfgEventConfigData *)ev_cfg->data_.get();

            if (acd_stream_.context_config_)
                free(acd_stream_.context_config_);

            status = acd_stream_.UpdateRecognitionConfig(
                (struct acd_recognition_cfg *)data->data_);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to update recognition config,"
                    "status %d", status);
            }
            break;
        }
        case ACD_EV_CONTEXT_CONFIG: {
            ACDContextCfgEventConfigData *data =
                (ACDContextCfgEventConfigData *)ev_cfg->data_.get();

            if (acd_stream_.context_config_)
                free(acd_stream_.context_config_);

            status = acd_stream_.UpdateContextConfig(
               (struct pal_param_context_list *)data->data_);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to update context config", status);
            break;
        }

        case ACD_EV_START_RECOGNITION: {
            if (acd_stream_.state_for_restore_ == ACD_STATE_LOADED ||
                acd_stream_.state_for_restore_ == ACD_STATE_DETECTED) {
                ACDStartRecognitionEventConfigData *data =
                    (ACDStartRecognitionEventConfigData *)ev_cfg->data_.get();
                if (!acd_stream_.rec_config_) {
                    PAL_ERR(LOG_TAG, "Recognition config not set %d", data->restart_);
                    status = -EINVAL;
                    break;
                }
                if ((acd_stream_.state_for_restore_ == ACD_STATE_DETECTED) &&
                    (acd_stream_.cached_event_data_ != NULL)) {
                    std::unique_lock<std::mutex> lck(acd_stream_.mutex_);
                    acd_stream_.cv_.notify_one();
                } else {
                    acd_stream_.state_for_restore_ = ACD_STATE_ACTIVE;
                }
            } else {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                        acd_stream_.state_for_restore_);
                status = -EINVAL;
            }
            break;
        }
        case ACD_EV_STOP_RECOGNITION: {
            if (acd_stream_.state_for_restore_ != ACD_STATE_ACTIVE ||
                acd_stream_.state_for_restore_ != ACD_STATE_DETECTED) {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                    acd_stream_.state_for_restore_);
                status = -EINVAL;
            } else {
                acd_stream_.state_for_restore_ = ACD_STATE_LOADED;
            }
            break;
        }
        default: {
            PAL_INFO(LOG_TAG, "Unhandled event %d", ev_cfg->id_);
            return status;
        }
    }
    PAL_DBG(LOG_TAG, "Exit: ACDSSR: event %d handled", ev_cfg->id_);

    return status;
}

int32_t StreamACD::ssrDownHandler() {
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (false == isStreamSSRDownFeasibile())
        return status;

    std::shared_ptr<ACDEventConfig> ev_cfg(new ACDSSROfflineConfig());
    status = cur_state_->ProcessEvent(ev_cfg);

    return status;
}

int32_t StreamACD::ssrUpHandler() {
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (skipSSRHandling) {
        skipSSRHandling = false;
        return status;
    }

    std::shared_ptr<ACDEventConfig> ev_cfg(new ACDSSROnlineConfig());
    status = cur_state_->ProcessEvent(ev_cfg);

    return status;
}
