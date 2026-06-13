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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: StreamASR"

#include <unistd.h>

#include "StreamASR.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"


StreamASR::StreamASR(struct pal_stream_attributes *sattr,
                     struct pal_device *dattr,
                     uint32_t noOfDevices,
                     struct modifier_kv *modifiers __unused,
                     uint32_t no_of_modifiers __unused,
                     std::shared_ptr<ResourceManager> rm)
{
    PAL_INFO(LOG_TAG, "Enter");

    palRecConfig = nullptr;
    recConfig = nullptr;
    outputConfig = nullptr;
    smCfg = nullptr;
    cmCfg = nullptr;
    deviceOpened = false;
    currentState = STREAM_IDLE;
    asrIdle = nullptr;
    asrActive = nullptr;
    asrSsr = nullptr;
    asrStates = {};
    callback = nullptr;
    cookie = 0;
    curState = nullptr;
    prevState = nullptr;
    engine = nullptr;
    stateToRestore = ASR_STATE_NONE;

    mVolumeData = (struct pal_volume_data *)malloc(sizeof(struct pal_volume_data)
                      +sizeof(struct pal_channel_vol_kv));
    if (!mVolumeData) {
        PAL_ERR(LOG_TAG, "Error:mVolumeData allocation failed");
        throw std::runtime_error("mVolumeData allocation failed");
    }
    mVolumeData->no_of_volpair = 1;
    mVolumeData->volume_pair[0].channel_mask = 0x03;
    mVolumeData->volume_pair[0].vol = 1.0f;

    mNoOfModifiers = 0;
    mModifiers = (struct modifier_kv *) (nullptr);

    mStreamAttr = (struct pal_stream_attributes *)calloc(1,
        sizeof(struct pal_stream_attributes));
    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "Error:%d stream attributes allocation failed", -EINVAL);
        throw std::runtime_error("stream attributes allocation failed");
    }

    if (!dattr) {
        PAL_ERR(LOG_TAG,"Error:invalid device arguments");
        throw std::runtime_error("invalid device arguments");
    }

    ar_mem_cpy(mStreamAttr, sizeof(pal_stream_attributes),
                     sattr, sizeof(pal_stream_attributes));
    mStreamAttr->in_media_config.sample_rate = SAMPLINGRATE_16K;
    mStreamAttr->in_media_config.bit_width = BITWIDTH_16;
    mStreamAttr->in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    mStreamAttr->in_media_config.ch_info.channels = CHANNELS_1;
    mStreamAttr->direction = PAL_AUDIO_INPUT;

    // get ASR platform info
    asrInfo = ASRPlatformInfo::GetInstance();
    if (!asrInfo) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to get asr platform info", -EINVAL);
        throw std::runtime_error("Failed to get asr platform info");
    }

    cmCfg = asrInfo->GetCommonConfig();
    if (!cmCfg) {
        PAL_ERR(LOG_TAG, "Error: %d Failed to get common config, -EINVAL");
        throw std::runtime_error("Failed to get common config");
    }

    rm->registerStream(this);

    // Create internal states
    asrIdle = new ASRIdle(*this);
    asrActive = new ASRActive(*this);
    asrSsr = new ASRSSR(*this);

    AddState(asrIdle);
    AddState(asrActive);
    AddState(asrSsr);

    // Set initial state
    if (rm->cardState == CARD_STATUS_OFFLINE) {
        curState = asrSsr;
    } else {
        curState = asrIdle;
    }

exit:
    PAL_INFO(LOG_TAG, "Exit");
}

StreamASR::~StreamASR()
{
    PAL_INFO(LOG_TAG, "Enter.");

    if (asrIdle)
        delete asrIdle;
    if (asrActive)
        delete asrActive;
    if (asrSsr)
        delete asrSsr;

    asrStates.clear();
    engine = nullptr;

    rm->resetStreamInstanceID(this, mInstanceID);

    rm->deregisterStream(this);
    if (mStreamAttr) {
        free(mStreamAttr);
    }
    mDevices.clear();
    PAL_INFO(LOG_TAG, "Exit");
}

int32_t StreamASR::close()
{
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter, stream direction %d", mStreamAttr->direction);

    std::lock_guard<std::mutex> lck(mStreamMutex);

    if (recConfig) {
        free(recConfig);
        recConfig = nullptr;
    }

    if (outputConfig) {
        free(outputConfig);
        outputConfig = nullptr;
    }

    if (palRecConfig) {
        free(palRecConfig);
    }

    if (engine) {
        engine->releaseEngine();
        engine = nullptr;
    }

    palStateEnqueue(this, PAL_STATE_CLOSED, status);
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamASR::start()
{
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter, stream direction %d", mStreamAttr->direction);

    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ASREventConfig> ev_cfg(
       new ASRStartRecognitionEventConfig());
    status = curState->ProcessEvent(ev_cfg);
    if (!status) {
        currentState = STREAM_STARTED;
    }
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamASR::stop()
{
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter.");

    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ASREventConfig> ev_cfg(
       new ASRStopRecognitionEventConfig());
    status = curState->ProcessEvent(ev_cfg);
    if (!status) {
        currentState = STREAM_STOPPED;
    }
    palStateEnqueue(this, PAL_STATE_STOPPED, status);

    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamASR::HandleConcurrentStream(bool active) {
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter");
    if (active == false)
        mStreamMutex.lock();

    std::shared_ptr<ASREventConfig> ev_cfg(
        new ASRConcurrentStreamEventConfig(active));
    status = curState->ProcessEvent(ev_cfg);

    if (active == true)
        mStreamMutex.unlock();

    PAL_INFO(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamASR::setParameters(uint32_t paramId, void *payload)
{
    PAL_INFO(LOG_TAG, "Enter, param id %d", paramId);

    int32_t status = 0;
    pal_param_payload *paramPayload = (pal_param_payload *)payload;

    if (!paramPayload) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Invalid payload for param ID: %d",
                         status, paramId);
        return status;
    }

    std::lock_guard<std::mutex> lck(mStreamMutex);
    switch (paramId) {
        case PAL_PARAM_ID_ASR_MODEL: {
            PAL_VERBOSE(LOG_TAG, "Currently model loading is not supported");
            break;
        }
        case PAL_PARAM_ID_ASR_CONFIG: {
            std::shared_ptr<ASREventConfig> evCfg(
                           new ASRSpeechCfgEventConfig(paramPayload->payload));
            status = curState->ProcessEvent(evCfg);
            break;
        }
        case PAL_PARAM_ID_ASR_FORCE_OUTPUT: {
            std::shared_ptr<ASREventConfig> evCfg(new ASRForceOutputConfig());
            status = curState->ProcessEvent(evCfg);
            break;
        }
        case PAL_PARAM_ID_ASR_CUSTOM: {
            PAL_INFO(LOG_TAG, "Currently this param id is not in use!!");
            break;
        }
        default: {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Error:%d Unsupported param %u", status, paramId);
            break;
        }
    }

    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

void StreamASR::HandleEventData(pal_asr_event *event, size_t eventSize) {

    PAL_INFO(LOG_TAG, "Enter. event status : %d, num events : %d",
             event->status, event->num_events);
    for (int i = 0; i < event->num_events; ++i) {
        PAL_INFO(LOG_TAG, "Event no : %d, is_final : %d, confidence : %d,\
                          text_size : %d, text : %s", i, event->event[i].is_final,
                          event->event[i].confidence, event->event[i].text_size,
                          event->event[i].text);
    }

    if (callback) {
        callback((pal_stream_handle_t *)this, 0, (uint32_t *)event, eventSize, cookie);
    }
    PAL_INFO(LOG_TAG, "Exit.");
}

void StreamASR::sendAbort() {

    PAL_INFO(LOG_TAG, "Enter.");
    struct pal_asr_event *cbEvent = (struct pal_asr_event *)
                                     calloc(1, sizeof(struct pal_asr_event));
    if (cbEvent == NULL) {
        PAL_ERR(LOG_TAG, "Error: Failed to allocate memory for asr event!!");
        goto exit;
    }

    cbEvent->status = PAL_ASR_EVENT_STATUS_ABORTED;

    mStreamMutex.unlock();
    if (callback)
        callback((pal_stream_handle_t *)this, 0, (uint32_t *)cbEvent,
                      sizeof(struct pal_asr_event), cookie);

    mStreamMutex.lock();
    free(cbEvent);
exit:
    PAL_INFO(LOG_TAG, "Exit.");
}

int32_t StreamASR::registerCallBack(pal_stream_callback cb,
                                             uint64_t ck)
{
    PAL_INFO(LOG_TAG, "Enter.");
    callback = cb;
    cookie = ck;

    PAL_INFO(LOG_TAG, "Exit, callback = %pK", callback);

    return 0;
}

int32_t StreamASR::setECRef(std::shared_ptr<Device> dev, bool isEnable)
{
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (rm->getLPIUsage()) {
        PAL_INFO(LOG_TAG, "EC ref will be handled in LPI/NLPI switch");
        return status;
    }
    status = setECRef_l(dev, isEnable);

    return status;
}

int32_t StreamASR::setECRef_l(std::shared_ptr<Device> dev, bool isEnable)
{
    PAL_INFO(LOG_TAG, "Enter, enable %d", isEnable);

    int32_t status = 0;
    std::shared_ptr<ASREventConfig> evCfg(
        new ASRECRefEventConfig(dev, isEnable));

    if (!capProf || !capProf->isECRequired()) {
        PAL_INFO(LOG_TAG, "No need to set ec ref");
        goto exit;
    }

    if (dev && !rm->checkECRef(dev, mDevices[0])) {
        PAL_INFO(LOG_TAG, "No need to set ec ref for unmatching rx device");
        goto exit;
    }

    status = curState->ProcessEvent(evCfg);
    if (status) {
        PAL_ERR(LOG_TAG, "Error:%d Failed to handle ec ref event", status);
    }

exit:
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

pal_device_id_t StreamASR::GetAvailCaptureDevice()
{
    if (asrInfo->GetSupportDevSwitch() &&
        rm->isDeviceAvailable(PAL_DEVICE_IN_WIRED_HEADSET))
        return PAL_DEVICE_IN_HEADSET_VA_MIC;
    else
        return PAL_DEVICE_IN_HANDSET_VA_MIC;
}

bool StreamASR::UseLpiCaptureProfile() {

    if (outputConfig->output_mode == BUFFERED)
        return true;

    return false;
}

std::shared_ptr<CaptureProfile> StreamASR::GetCurrentCaptureProfile()
{
    std::shared_ptr<CaptureProfile> capProf = nullptr;
    enum StInputModes inputMode = ST_INPUT_MODE_HANDSET;
    enum StOperatingModes operatingMode = ST_OPERATING_MODE_HIGH_PERF;

    if (GetAvailCaptureDevice() == PAL_DEVICE_IN_HEADSET_VA_MIC)
        inputMode = ST_INPUT_MODE_HEADSET;

    if (!UseLpiCaptureProfile())
        rm->setForceNLPI(true);

    if (rm->getLPIUsage())
        operatingMode = ST_OPERATING_MODE_LOW_POWER;

    capProf = smCfg->GetCaptureProfile(
                std::make_pair(operatingMode, inputMode));
    if (!capProf) {
        PAL_ERR(LOG_TAG, "Error:Failed to get capture profile");
        goto exit;
    }

    PAL_INFO(LOG_TAG, "cap_prof %s: dev_id=0x%x, chs=%d, sr=%d, snd_name=%s, ec_ref=%d",
        capProf->GetName().c_str(), capProf->GetDevId(),
        capProf->GetChannels(), capProf->GetSampleRate(),
        capProf->GetSndName().c_str(), capProf->isECRequired());

exit:
    return capProf;
}

int32_t StreamASR::DisconnectDevice(pal_device_id_t deviceId) {
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter");
    /*
     * NOTE: mStreamMutex will be unlocked after ConnectDevice handled
     * because device disconnect/connect should be handled sequencely,
     * and no other commands from client should be handled between
     * device disconnect and connect.
     */
    mStreamMutex.lock();
    std::shared_ptr<ASREventConfig> evCfg(
        new ASRDeviceDisconnectedEventConfig(deviceId));
    status = curState->ProcessEvent(evCfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d", status, deviceId);

    PAL_INFO(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamASR::ConnectDevice(pal_device_id_t deviceId) {
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter");
    std::shared_ptr<ASREventConfig> evCfg(
        new ASRDeviceConnectedEventConfig(deviceId));
    status = curState->ProcessEvent(evCfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d", status, deviceId);
    mStreamMutex.unlock();
    PAL_INFO(LOG_TAG, "Exit, status %d", status);

    return status;
}

void StreamASR::AddState(ASRState* state)
{
   asrStates.insert(std::make_pair(state->GetStateId(), state));
}

int32_t StreamASR::GetCurrentStateId()
{
    if (curState)
        return curState->GetStateId();

    return ASR_STATE_NONE;
}

int32_t StreamASR::GetPreviousStateId()
{
    if (prevState)
        return prevState->GetStateId();

    return ASR_STATE_NONE;
}

void StreamASR::TransitTo(int32_t stateId)
{
    auto it = asrStates.find(stateId);

    if (it == asrStates.end()) {
        PAL_ERR(LOG_TAG, "Error:%d Unknown transit state %d", -EINVAL, stateId);
        return;
    }
    prevState = curState;
    curState = it->second;
    auto oldState = asrStateNameMap.at(prevState->GetStateId());
    auto newState = asrStateNameMap.at(it->first);
    PAL_INFO(LOG_TAG, "Stream : state transitioned from %s to %s",
           oldState.c_str(), newState.c_str());
}

int32_t StreamASR::ProcessInternalEvent(
    std::shared_ptr<ASREventConfig> evCfg) {
    return curState->ProcessEvent(evCfg);
}

void StreamASR::GetUUID(class SoundTriggerUUID *uuid,
                        const struct st_uuid *vendorUuid)
{

    uuid->timeLow = (uint32_t)vendorUuid->timeLow;
    uuid->timeMid = (uint16_t)vendorUuid->timeMid;
    uuid->timeHiAndVersion = (uint16_t)vendorUuid->timeHiAndVersion;
    uuid->clockSeq = (uint16_t)vendorUuid->clockSeq;
    uuid->node[0] = (uint8_t)vendorUuid->node[0];
    uuid->node[1] = (uint8_t)vendorUuid->node[1];
    uuid->node[2] = (uint8_t)vendorUuid->node[2];
    uuid->node[3] = (uint8_t)vendorUuid->node[3];
    uuid->node[4] = (uint8_t)vendorUuid->node[4];
    uuid->node[5] = (uint8_t)vendorUuid->node[5];
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

int32_t StreamASR::SetupStreamConfig(const struct st_uuid *vendorUuid)
{
    int32_t status = 0;
    class SoundTriggerUUID uuid;

    PAL_INFO(LOG_TAG, "Enter");
    GetUUID(&uuid, vendorUuid);

    smCfg = asrInfo->GetStreamConfig(uuid);
    if (!smCfg) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Failed to get stream config", status);
        goto exit;
    }
    mStreamSelector = smCfg->GetStreamConfigName();
    mInstanceID = rm->getStreamInstanceID(this);
exit:
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamASR::SetupDetectionEngine()
{
    int status = 0;
    pal_device_id_t devId;
    std::shared_ptr<Device> dev = nullptr;
    struct st_uuid uuid = GetVendorUuid();

    PAL_INFO(LOG_TAG, "Enter");
    if (smCfg == NULL) {
        status = SetupStreamConfig(&uuid);
        if (status) {
            PAL_ERR(LOG_TAG, "Error:%d Failed to setup Stream Config", status);
            goto errorExit;
        }
    }

    if (rm->getLPIUsage() &&
        !UseLpiCaptureProfile()) {
        mStreamMutex.unlock();
        rm->setForceNLPI(true);
        rm->forceSwitchSoundTriggerStreams(true);
        mStreamMutex.lock();
    }

    devId = GetAvailCaptureDevice();
    PAL_INFO(LOG_TAG, "Select available caputre device %d", devId);

    dev = GetPalDevice(this, devId);
    if (!dev) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Error:%d Device creation is failed", status);
        goto errorExit;
    }
    mDevices.clear();
    mDevices.push_back(dev);

    engine = ASREngine::GetInstance(this, smCfg);

    if (!engine) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d engine creation failed", status);
    }

errorExit:
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

bool StreamASR::compareConfig(struct pal_asr_config *oldConfig, struct pal_asr_config *newConfig) {

    if (oldConfig == nullptr ||
        oldConfig->input_language_code != newConfig->input_language_code ||
        oldConfig->output_language_code != newConfig->output_language_code ||
        oldConfig->enable_language_detection != newConfig->enable_language_detection ||
        oldConfig->enable_translation != newConfig->enable_translation ||
        oldConfig->enable_continuous_mode != newConfig->enable_continuous_mode ||
        oldConfig->threshold != newConfig->threshold ||
        oldConfig->timeout_duration != newConfig->timeout_duration ||
        oldConfig->silence_detection_duration != newConfig->silence_detection_duration ||
        oldConfig->enable_partial_transcription != newConfig->enable_partial_transcription ||
        oldConfig->outputBufferMode != newConfig->outputBufferMode)
        return false;

    return true;
}

int32_t StreamASR::SetRecognitionConfig(struct pal_asr_config *asrRecCfg)
{
    PAL_INFO(LOG_TAG, "Enter");

    int32_t status = 0;
    size_t len = 0;

    if (compareConfig(palRecConfig, asrRecCfg)) {
        PAL_DBG(LOG_TAG, "Same config, no need to set it again!!!");
        goto exit;
    }

    if (recConfig)
        free(recConfig);

    if (outputConfig)
        free(outputConfig);

    if (palRecConfig)
        free(palRecConfig);

    recConfig = (param_id_asr_config_t *)calloc(1, sizeof(param_id_asr_config_t));
    if (!recConfig) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate recConfig", status);
        goto exit;
    }

    outputConfig = (param_id_asr_output_config_t *)calloc(1, sizeof(param_id_asr_output_config_t));
    if (!outputConfig) {
        free(recConfig);
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate outputConfig", status);
        goto exit;
    }

    inputConfig = (param_id_asr_input_threshold_t *)calloc(1, sizeof(param_id_asr_input_threshold_t));
    if (!inputConfig) {
        free(recConfig);
        free(outputConfig);
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "Error:%d Failed to allocate inputConfig", status);
        goto exit;
    }

    recConfig->input_language_code          = (uint32_t)asrRecCfg->input_language_code;
    recConfig->output_language_code         = (uint32_t)asrRecCfg->output_language_code;
    recConfig->enable_language_detection    = (uint32_t)asrRecCfg->enable_language_detection;
    recConfig->enable_translation           = (uint32_t)asrRecCfg->enable_translation;
    recConfig->enable_continuous_mode       = (uint32_t)asrRecCfg->enable_continuous_mode;
    recConfig->threshold                    = asrRecCfg->threshold;
    recConfig->timeout_duration             = asrRecCfg->timeout_duration;
    recConfig->vad_hangover_duration        = asrRecCfg->silence_detection_duration == 0 ?
                                                  VAD_HANG_OVER_DURTION_DEFAULT_MS :
                                                  asrRecCfg->silence_detection_duration;
    recConfig->enable_partial_transcription = asrRecCfg->enable_partial_transcription;

    outputConfig->output_mode  = asrRecCfg->outputBufferMode ? BUFFERED : NON_BUFFERED;
    outputConfig->out_buf_size = cmCfg->GetOutputBufferSize(outputConfig->output_mode);
    outputConfig->num_bufs     = 2;

    inputConfig->buf_duration_ms = cmCfg->GetInputBufferSize(outputConfig->output_mode);

    PAL_INFO(LOG_TAG, "Sending configs lang_code : %d, op_lang_code : %d, en_lang_det : %d,"
        "en_transl :%d, cont_mode : %d, threshold : %d, time_dur : %d,"
        "sl_time_dur : %d, partial_trans : %d", recConfig->input_language_code,
        recConfig->output_language_code, recConfig->enable_language_detection,
        recConfig->enable_translation, recConfig->enable_continuous_mode,
        recConfig->threshold, recConfig->timeout_duration,
        recConfig->vad_hangover_duration, recConfig->enable_partial_transcription);

    PAL_INFO(LOG_TAG, "Recieved output buffer mode : %d, sending output mode : %d",
        asrRecCfg->outputBufferMode, outputConfig->output_mode);

    palRecConfig = (struct pal_asr_config *)calloc(1, sizeof(struct pal_asr_config));
    ar_mem_cpy(palRecConfig, sizeof(struct pal_asr_config), asrRecCfg, sizeof(struct pal_asr_config));

    // dump asr recognition config data
    if (asrInfo->GetEnableDebugDumps()) {
        ST_DBG_DECLARE(FILE *rec_opaque_fd = NULL; static int rec_opaque_cnt = 0);
        ST_DBG_FILE_OPEN_WR(rec_opaque_fd, ST_DEBUG_DUMP_LOCATION,
            "asrRecConfig", "bin", rec_opaque_cnt);
        ST_DBG_FILE_WRITE(rec_opaque_fd,
            (uint8_t *)recConfig, sizeof(struct pal_asr_config));
        ST_DBG_FILE_CLOSE(rec_opaque_fd);
        PAL_INFO(LOG_TAG, "palAsrConfig data stored in: asrRecConfig_%d.bin",
            rec_opaque_cnt);
        rec_opaque_cnt++;
    }

    status = SetupDetectionEngine();
    if (status) {
        PAL_ERR(LOG_TAG, "Error: %d Failed to get engine instance", status);
    }

exit:
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamASR::ASRIdle::ProcessEvent(
    std::shared_ptr<ASREventConfig> evCfg)
{
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "ASRIdle: handle event %d for stream instance %u",
        evCfg->id, asrStream.mInstanceID);

    switch (evCfg->id) {
        case ASR_EV_SPEECH_CONFIG: {
            ASRSpeechCfgEventData *data = (ASRSpeechCfgEventData *)evCfg->data.get();
            status = asrStream.SetRecognitionConfig(
               (struct pal_asr_config *)data->config);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send recog config", status);

            break;
        }
        case ASR_EV_START_SPEECH_RECOGNITION: {
            if (asrStream.paused) {
                status = -EBUSY;
                break;
            }
            auto& dev = asrStream.mDevices[0];

            /* Update cap dev based on mode and configuration and start it */
            struct pal_device dattr;
            bool backendUpdate = false;
            std::shared_ptr<CaptureProfile> capProf = nullptr;

            backendUpdate = asrStream.rm->UpdateSoundTriggerCaptureProfile(
                                             &asrStream, true);
            if (backendUpdate ) {
                status = rm->StopOtherDetectionStreams(&asrStream);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to stop other Detection streams", status);

                status = rm->StartOtherDetectionStreams(&asrStream);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to start other Detection streams", status);
            }

            dev->getDeviceAttributes(&dattr);

            asrStream.capProf = asrStream.GetCurrentCaptureProfile();
            asrStream.mDevPPSelector = asrStream.capProf->GetName();

            capProf = asrStream.rm->GetSoundTriggerCaptureProfile();
            if (!capProf) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Error:%d Invalid capture profile", status);
                break;
            }

            dattr.config.bit_width = capProf->GetBitWidth();
            dattr.config.ch_info.channels = capProf->GetChannels();
            dattr.config.sample_rate = capProf->GetSampleRate();
            dev->setDeviceAttributes(dattr);

            PAL_INFO(LOG_TAG, "updated device attr dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    capProf->GetDevId(), capProf->GetChannels(),
                    capProf->GetSampleRate(), capProf->isECRequired());

            /* now start the device */
            PAL_INFO(LOG_TAG, "Start device %d-%s", dev->getSndDeviceId(),
                    dev->getPALDeviceName().c_str());
            dev->setSndName(capProf->GetSndName());

            if (!asrStream.deviceOpened) {
                status = dev->open();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error:%d Device open failed", status);
                    break;
                }
                asrStream.deviceOpened = true;
            }

            status = dev->start();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d Device start failed", status);
                break;
            } else {
                asrStream.rm->registerDevice(dev, &asrStream);
            }
            PAL_INFO(LOG_TAG, "device started");

            /* Start the engines */
            status = asrStream.engine->StartEngine(&asrStream);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d Start asr engine failed", status);
                goto err_exit;
            }
            TransitTo(ASR_STATE_ACTIVE);
            break;
        err_exit:
            if (asrStream.mDevices.size() > 0) {
                asrStream.rm->deregisterDevice(asrStream.mDevices[0], &asrStream);
                asrStream.mDevices[0]->stop();
            }
            break;
        }
        case ASR_EV_FORCE_OUTPUT: {
            PAL_ERR(LOG_TAG, "Error: stream is not in active state");
            break;
        }
        case ASR_EV_DEVICE_DISCONNECTED: {
            ASRDeviceDisconnectedEventData *data =
                        (ASRDeviceDisconnectedEventData *)evCfg->data.get();
            pal_device_id_t deviceId = data->devId;
            if (asrStream.mDevices.size() == 0) {
                PAL_INFO(LOG_TAG, "No device to disconnect");
                break;
            } else {
                int currDeviceId = asrStream.mDevices[0]->getSndDeviceId();
                pal_device_id_t currDevice =
                    static_cast<pal_device_id_t>(currDeviceId);
                if (currDevice != deviceId) {
                    PAL_ERR(LOG_TAG, "Error:%d Device %d not connected, ignore",
                        -EINVAL, deviceId);
                    break;
                }
            }
            asrStream.mDevices.clear();
            break;
        }
        case ASR_EV_DEVICE_CONNECTED: {
            ASRDeviceConnectedEventData *data =
                           (ASRDeviceConnectedEventData *)evCfg->data.get();
            std::shared_ptr<Device> dev = nullptr;
            pal_device_id_t devId = data->devId;

            dev = asrStream.GetPalDevice(&asrStream, devId);
            if (!dev) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Error:%d Device creation failed", status);
                goto connect_err;
            }

            asrStream.mDevices.clear();
            asrStream.mDevices.push_back(dev);
        connect_err:
            break;
        }
        case ASR_EV_PAUSE: {
            asrStream.paused = true;
            break;
        }
        case ASR_EV_RESUME: {
            asrStream.paused = false;
            break;
        }
        case ASR_EV_CONCURRENT_STREAM: {
            PAL_INFO(LOG_TAG, "no action needed for concurrent stream in idle state");
            break;
        }
        case ASR_EV_SSR_OFFLINE:
            asrStream.stateToRestore = ASR_STATE_IDLE;
            TransitTo(ASR_STATE_SSR);
            break;
        default:
            PAL_INFO(LOG_TAG, "Unhandled event %d", evCfg->id);
            break;
    }
    return status;
}

int32_t StreamASR::ASRActive::ProcessEvent(
    std::shared_ptr<ASREventConfig> evCfg)
{
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Active handle event %d for stream instance %u",
        evCfg->id, asrStream.mInstanceID);

    switch (evCfg->id) {
        case ASR_EV_PAUSE: {
            asrStream.paused = true;
            [[fallthrough]];
        }
        case ASR_EV_STOP_SPEECH_RECOGNITION: {
            bool backendUpdate = false;

            rm->setForceNLPI(false);

            backendUpdate = asrStream.rm->UpdateSoundTriggerCaptureProfile(
                                             &asrStream, false);

            if (backendUpdate) {
                status = rm->StopOtherDetectionStreams(&asrStream);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to stop other Detection streams", status);
            }

            PAL_INFO(LOG_TAG, "Stop engine");
            status = asrStream.engine->StopEngine(&asrStream);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Stop engine failed", status);

            auto& dev = asrStream.mDevices[0];
            PAL_INFO(LOG_TAG, "Stop device %d-%s", dev->getSndDeviceId(),
                    dev->getPALDeviceName().c_str());
            status = dev->stop();
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Device stop failed", status);

            asrStream.rm->deregisterDevice(dev, &asrStream);

            status = dev->close();
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Device close failed", status);
            asrStream.deviceOpened = false;

            if (rm->getLPIUsage() &&
                !asrStream.UseLpiCaptureProfile()) {
                asrStream.mStreamMutex.unlock();
                rm->forceSwitchSoundTriggerStreams(false);
                asrStream.mStreamMutex.lock();
            }

            if (backendUpdate) {
                status = rm->StartOtherDetectionStreams(&asrStream);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d Failed to start other Detection streams", status);
            }
            if (evCfg->id == ASR_EV_PAUSE) {
                asrStream.sendAbort();
            }

            TransitTo(ASR_STATE_IDLE);
            break;
        }
        case ASR_EV_FORCE_OUTPUT: {
            status = asrStream.engine->setParameters(&asrStream, ASR_FORCE_OUTPUT);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to setparam for force output", status);
            break;
        }
        case ASR_EV_DEVICE_DISCONNECTED: {
            ASRDeviceDisconnectedEventData *data =
                        (ASRDeviceDisconnectedEventData *)evCfg->data.get();
            pal_device_id_t deviceId = data->devId;

            int currDeviceId = asrStream.mDevices[0]->getSndDeviceId();
            if (currDeviceId != deviceId) {
                PAL_ERR(LOG_TAG, "Error:%d Device %d not connected, ignore",
                    -EINVAL, deviceId);
                break;
            }
            auto& dev = asrStream.mDevices[0];

            asrStream.rm->deregisterDevice(dev, &asrStream);

            asrStream.engine->DisconnectSessionDevice(&asrStream,
                                                      asrStream.mStreamAttr->type, dev);

            status = dev->stop();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d device stop failed", status);
                goto disconnect_err;
            }

            status = dev->close();
            asrStream.deviceOpened = false;
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d device close failed", status);
                goto disconnect_err;
            }
        disconnect_err:
            asrStream.mDevices.clear();
            break;
        }
        case ASR_EV_DEVICE_CONNECTED: {
            ASRDeviceConnectedEventData *data =
                           (ASRDeviceConnectedEventData *)evCfg->data.get();
            pal_device_id_t devId = data->devId;
            std::shared_ptr<Device> dev = nullptr;

            dev = asrStream.GetPalDevice(&asrStream, devId);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Error:%d Device creation failed", -EINVAL);
                status = -EINVAL;
                break;
            }
            if (!asrStream.deviceOpened) {
                status = dev->open();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Error:%d device %d open failed", status,
                        dev->getSndDeviceId());
                    break;
                }
                asrStream.deviceOpened = true;
            }

            asrStream.mDevices.clear();
            asrStream.mDevices.push_back(dev);

            PAL_INFO(LOG_TAG, "Update capture profile before SetupSessionDevice");
            asrStream.capProf = asrStream.GetCurrentCaptureProfile();
            asrStream.mDevPPSelector = asrStream.capProf->GetName();

            status = asrStream.engine->SetupSessionDevice(&asrStream,
                                                          asrStream.mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d setupSessionDevice for %d failed",
                        status, dev->getSndDeviceId());
                dev->close();
                asrStream.deviceOpened = false;
                break;
            }

            status = dev->start();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d device %d start failed",
                    status, dev->getSndDeviceId());
                break;
            }

            status = asrStream.engine->ConnectSessionDevice(&asrStream,
                asrStream.mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:%d connectSessionDevice for %d failed",
                      status, dev->getSndDeviceId());
                dev->close();
                asrStream.deviceOpened = false;
            } else {
                asrStream.rm->registerDevice(dev, &asrStream);
            }
            break;
        }
        case ASR_EV_SPEECH_CONFIG: {
            ASRSpeechCfgEventData *data = (ASRSpeechCfgEventData *)evCfg->data.get();
            status = asrStream.SetRecognitionConfig(
               (struct pal_asr_config *)data->config);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Error:%d Failed to send recog config", status);

            break;
        }
        case ASR_EV_EC_REF: {
            ASRECRefEventData *data = (ASRECRefEventData *)evCfg->data.get();
            Stream *s = static_cast<Stream *>(&asrStream);
            PAL_ERR(LOG_TAG, "EC enable : %d", data->isEnable);
            status = asrStream.engine->setECRef(s, data->dev, data->isEnable);
            if (status) {
                PAL_ERR(LOG_TAG, "Error:%d Failed to set EC Ref in engine", status);
            }
            break;
        }
        case ASR_EV_CONCURRENT_STREAM: {
            ASRConcurrentStreamEventData *data =
                           (ASRConcurrentStreamEventData *)evCfg->data.get();
            std::shared_ptr<CaptureProfile> newCapProf = nullptr;
            bool active = false;

            active = data->active;
            newCapProf = asrStream.GetCurrentCaptureProfile();
            if (!newCapProf) {
                PAL_ERR(LOG_TAG, "Failed to initialize new capture profile");
                status = -EINVAL;
                break;
            }
            if (asrStream.capProf != newCapProf) {
                PAL_INFO(LOG_TAG,
                    "current capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    asrStream.capProf->GetName().c_str(),
                    asrStream.capProf->GetDevId(),
                    asrStream.capProf->GetChannels(),
                    asrStream.capProf->GetSampleRate(),
                    asrStream.capProf->isECRequired());
                PAL_INFO(LOG_TAG,
                    "new capture profile %s: dev_id=0x%x, chs=%d, sr=%d, ec_ref=%d\n",
                    newCapProf->GetName().c_str(),
                    newCapProf->GetDevId(),
                    newCapProf->GetChannels(),
                    newCapProf->GetSampleRate(),
                    newCapProf->isECRequired());
                if (!active) {
                    std::shared_ptr<ASREventConfig> evCfg1(
                        new ASRDeviceDisconnectedEventConfig(asrStream.GetAvailCaptureDevice()));
                    status = asrStream.ProcessInternalEvent(evCfg1);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to disconnect device %d", status,
                                    asrStream.GetAvailCaptureDevice());
                } else {
                    std::shared_ptr<ASREventConfig> evCfg1(
                        new ASRDeviceConnectedEventConfig(asrStream.GetAvailCaptureDevice()));
                    status = asrStream.ProcessInternalEvent(evCfg1);
                    if (status)
                        PAL_ERR(LOG_TAG, "Error:%d Failed to connect device %d", status, asrStream.GetAvailCaptureDevice());
                }
            } else {
              PAL_INFO(LOG_TAG,"no action needed, same capture profile");
            }
            break;
        }
        case ASR_EV_SSR_OFFLINE: {
            asrStream.stateToRestore = ASR_STATE_ACTIVE;
            std::shared_ptr<ASREventConfig> evCfg1(
                new ASRStopRecognitionEventConfig());
            status = asrStream.ProcessInternalEvent(evCfg1);
            TransitTo(ASR_STATE_SSR);
            break;
        }
    }

    return status;
}

int32_t StreamASR::ASRSSR::ProcessEvent(std::shared_ptr<ASREventConfig> evCfg)
{
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "SSR: handle event %d for stream instance %u",
        evCfg->id, asrStream.mInstanceID);

    switch (evCfg->id) {
        case ASR_EV_SSR_ONLINE: {
            TransitTo(ASR_STATE_IDLE);

            if (asrStream.stateToRestore == ASR_STATE_ACTIVE) {
                std::shared_ptr<ASREventConfig> evCfg1(
                    new ASRStartRecognitionEventConfig());
                status = asrStream.ProcessInternalEvent(evCfg1);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Failed to Start, status %d", status);
                    break;
                }
            }

            asrStream.stateToRestore = ASR_STATE_NONE;
            break;
        }
        case ASR_EV_SPEECH_CONFIG: {
            ASRSpeechCfgEventData *data = (ASRSpeechCfgEventData *)evCfg->data.get();
            status = asrStream.SetRecognitionConfig(
                (struct pal_asr_config *)data->config);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to update recognition config,"
                    "status %d", status);
            }
            break;
        }

        case ASR_EV_START_SPEECH_RECOGNITION: {
            if (asrStream.stateToRestore == ASR_STATE_IDLE) {
                if (!asrStream.recConfig) {
                    PAL_ERR(LOG_TAG, "Recognition config not set ");
                    status = -EINVAL;
                    break;
                }
                asrStream.stateToRestore = ASR_STATE_ACTIVE;
            } else {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                        asrStream.stateToRestore);
                status = -EINVAL;
            }
            break;
        }
        case ASR_EV_RESUME: {
            if (asrStream.paused) {
                if (asrStream.currentState == STREAM_STARTED)
                    asrStream.stateToRestore = ASR_STATE_ACTIVE;
                asrStream.paused = false;
            }
            break;
        }
        case ASR_EV_PAUSE: {
            asrStream.paused = true;
            if (asrStream.currentState == STREAM_STARTED)
                asrStream.sendAbort();
            break;
        }
        case ASR_EV_STOP_SPEECH_RECOGNITION: {
            if (asrStream.stateToRestore != ASR_STATE_ACTIVE) {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                    asrStream.stateToRestore);
                status = -EINVAL;
            } else {
                asrStream.stateToRestore = ASR_STATE_IDLE;
            }
            break;
        }
        case ASR_EV_FORCE_OUTPUT: {
            if (asrStream.stateToRestore != ASR_STATE_ACTIVE) {
                PAL_ERR(LOG_TAG, "Invalid operation, client state = %d now",
                        asrStream.stateToRestore);
            } else {
                asrStream.sendAbort();
            }
            break;
        }
        default: {
            PAL_INFO(LOG_TAG, "Unhandled event %d", evCfg->id);
            return status;
        }
    }
    PAL_INFO(LOG_TAG, "Exit: SSR: event %d handled", evCfg->id);

    return status;
}
int32_t StreamASR::Resume(bool isInternal) {
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter");
    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ASREventConfig> evCfg(new ASRResumeEventConfig());
    status = curState->ProcessEvent(evCfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Resume failed", status);
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_INFO(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamASR::Pause(bool isInternal) {
    int32_t status = 0;

    PAL_INFO(LOG_TAG, "Enter");
    std::lock_guard<std::mutex> lck(mStreamMutex);
    std::shared_ptr<ASREventConfig> evCfg(new ASRPauseEventConfig());
    status = curState->ProcessEvent(evCfg);
    if (status)
        PAL_ERR(LOG_TAG, "Error:%d Pause failed", status);
    palStateEnqueue(this, PAL_STATE_PAUSED, status);
    PAL_INFO(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamASR::ssrDownHandler() {
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (false == isStreamSSRDownFeasibile())
        return status;

    std::shared_ptr<ASREventConfig> evCfg(new ASRSSROfflineConfig());
    status = curState->ProcessEvent(evCfg);

    return status;
}

int32_t StreamASR::ssrUpHandler() {
    int32_t status = 0;

    std::lock_guard<std::mutex> lck(mStreamMutex);
    if (skipSSRHandling) {
        skipSSRHandling = false;
        return status;
    }

    std::shared_ptr<ASREventConfig> evCfg(new ASRSSROnlineConfig());
    status = curState->ProcessEvent(evCfg);

    return status;
}

uint32_t StreamASR::GetOutputToken() {

    if (engine) {
        return engine->GetOutputToken();
    }

    return 0;
}

uint32_t StreamASR::GetNumEvents() {

    if (engine)
        return engine->GetNumOutput();

    return 0;
}

uint32_t StreamASR::GetPayloadSize() {

    if (engine)
        return engine->GetPayloadSize();

    return 0;
}

int32_t StreamASR::isSampleRateSupported(uint32_t sampleRate) {
    int32_t rc = 0;

    PAL_INFO(LOG_TAG, "sampleRate %u", sampleRate);
    switch (sampleRate) {
        case SAMPLINGRATE_16K:
        case SAMPLINGRATE_48K:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "sample rate not supported rc %d", rc);
            break;
    }

    return rc;
}

int32_t StreamASR::isChannelSupported(uint32_t numChannels) {
    int32_t rc = 0;

    PAL_INFO(LOG_TAG, "numChannels %u", numChannels);
    switch (numChannels) {
        case CHANNELS_1:
        case CHANNELS_2:
        case CHANNELS_3:
        case CHANNELS_4:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "channels not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t StreamASR::isBitWidthSupported(uint32_t bitWidth) {
    int32_t rc = 0;

    PAL_INFO(LOG_TAG, "bitWidth %u", bitWidth);
    switch (bitWidth) {
        case BITWIDTH_16:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "bit width not supported rc %d", rc);
            break;
    }
    return rc;
}
