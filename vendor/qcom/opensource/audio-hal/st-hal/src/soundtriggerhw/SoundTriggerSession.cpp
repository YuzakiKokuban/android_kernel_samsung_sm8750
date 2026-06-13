/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "STHAL: SoundTriggerSession"
#define ATRACE_TAG (ATRACE_TAG_AUDIO | ATRACE_TAG_HAL)
/* #define LOG_NDEBUG 0 */

#include <soundtriggerhw/SoundTriggerSession.h>
#include <soundtriggerhw/SoundTriggerCommon.h>
#include <utils/PalToAidlConverter.h>
#include <utils/AidlToPalConverter.h>
#include <utils/CoreUtils.h>

#include <chrono>
#include <cstring>
#include <log/log.h>
#include <thread>
#include <utils/Trace.h>

#include "PalApi.h"

namespace aidl::android::hardware::soundtrigger3 {

SoundTriggerSession::SoundTriggerSession(
                        SoundModelHandle handle,
                        const std::shared_ptr<ISoundTriggerHwCallback> &stHwCallback)
{
    STHAL_INFO(LOG_TAG, "Enter, handle %d", handle);
    mSessionHandle = handle;
    mClientCallback = stHwCallback;
}

SoundTriggerSession::~SoundTriggerSession()
{
    STHAL_VERBOSE(LOG_TAG, "Enter, handle %d", mSessionHandle);
}

int SoundTriggerSession::loadSoundModel(const SoundModel &model)
{
    std::lock_guard<std::mutex> lock(mSessionMutex);
    return loadSoundModel_l(model);
}

int SoundTriggerSession::loadPhraseSoundModel(const PhraseSoundModel &model)
{
    std::lock_guard<std::mutex> lock(mSessionMutex);
    return loadPhraseSoundModel_l(model);
}

int SoundTriggerSession::unloadSoundModel()
{
    std::lock_guard<std::mutex> lock(mSessionMutex);
    return unloadSoundModel_l();
}

int SoundTriggerSession::startRecognition(int32_t deviceHandle, int32_t ioHandle,
                                          const RecognitionConfig &config)
{
    std::lock_guard<std::mutex> lock(mSessionMutex);
    return startRecognition_l(deviceHandle, ioHandle, config);
}

int SoundTriggerSession::stopRecognition()
{
    std::lock_guard<std::mutex> lock(mSessionMutex);
    return stopRecognition_l();
}

std::string SoundTriggerSession::getModuleVersion()
{
    int status = 0;
    char version[PAL_SOUND_TRIGGER_MAX_STRING_LEN];
    pal_param_payload *paramPayload = nullptr;
    struct version_arch_payload *versionPayload = nullptr;

    STHAL_VERBOSE(LOG_TAG, "Enter, handle %d", mSessionHandle);
    std::lock_guard<std::mutex> lock(mSessionMutex);
    status = openPALStream(PAL_STREAM_VOICE_UI);
    if (status) {
        STHAL_ERR(LOG_TAG, "Failed to open pal stream, status = %d", status);
        goto exit;
    }

    status = pal_stream_get_param(mPalHandle,
                                  PAL_PARAM_ID_WAKEUP_MODULE_VERSION,
                                  &paramPayload);
    if (status || paramPayload == nullptr) {
        STHAL_ERR(LOG_TAG, "Failed to get version, status = %d", status);
        goto exit;
    }

    versionPayload = (struct version_arch_payload *)paramPayload;
    snprintf(version, PAL_SOUND_TRIGGER_MAX_STRING_LEN, "%d, %s",
             versionPayload->version, versionPayload->arch);

exit:
    if (mPalHandle) {
        status = pal_stream_close(mPalHandle);
        if (status) {
            STHAL_ERR(LOG_TAG, "error, failed to close pal stream, status = %d", status);
        }
        mPalHandle = nullptr;
    }
    if (paramPayload != nullptr)
        delete paramPayload;

    STHAL_VERBOSE(LOG_TAG, "Exit, status = %d, version = %s", status, version);
    return version;
}

int SoundTriggerSession::palCallback(pal_stream_handle_t *streamHandle,
                                     uint32_t eventId, uint32_t *eventData,
                                     uint32_t eventSize, uint64_t cookie)
{
    if (!streamHandle || !eventData) {
        STHAL_ERR(LOG_TAG, "error, invalid handle/eventData id %d, size %d", eventId, eventSize);
        return -EINVAL;
    }

    SoundTriggerSession *session = (SoundTriggerSession *)cookie;
    session->onCallback(eventData);
    return 0;
}

void SoundTriggerSession::onCallback(uint32_t *eventData)
{
    struct pal_st_recognition_event *palEvent = (struct pal_st_recognition_event *)eventData;
    bool sessionLockStatus = false;

    STHAL_VERBOSE(LOG_TAG, "Enter, handle %d model type %d ", getSessionHandle(), palEvent->type);
    /*
     * Sometimes Client may call unload directly, which may get blocked in PAL when releasing
     * second stage engine thread, as it is waiting for this callback to finish. Check if session
     * state changes to non ACTIVE state.
     */
    do {
        sessionLockStatus = mSessionMutex.try_lock();
    } while(!sessionLockStatus && isSessionActive_l());

    if (isSessionActive_l()) {
        if (palEvent->type == PAL_SOUND_MODEL_TYPE_GENERIC) {
            onRecognitionCallback_l(palEvent);
        } else {
            onPhraseRecognitionCallback_l((pal_st_phrase_recognition_event *)eventData);
        }
        sessionLockStatus = false;
    } else {
        STHAL_WARN(LOG_TAG, "skip notification as handle %d has stopped", getSessionHandle());
    }

    if (sessionLockStatus)
        mSessionMutex.unlock();
    STHAL_INFO(LOG_TAG, "Exit, handle %d model type %d", getSessionHandle(), palEvent->type);
}

void SoundTriggerSession::onRecognitionCallback_l(struct pal_st_recognition_event *palEvent)
{
    RecognitionEvent event;

    PalToAidlConverter::convertRecognitionEvent(palEvent, event);
    STHAL_INFO(LOG_TAG, "sending %s", event.toString().c_str());
    mSessionMutex.unlock();
    mClientCallback->recognitionCallback(getSessionHandle(), event);
}

void SoundTriggerSession::onPhraseRecognitionCallback_l(
    struct pal_st_phrase_recognition_event *palPhraseEvent)
{
    PhraseRecognitionEvent event;

    PalToAidlConverter::convertPhraseRecognitionEvent(palPhraseEvent, event);
    STHAL_INFO(LOG_TAG, "sending %s", event.toString().c_str());
    mSessionMutex.unlock();
    mClientCallback->phraseRecognitionCallback(getSessionHandle(), event);
}

int SoundTriggerSession::openPALStream(pal_stream_type_t stream)
{
    int status = 0;
    struct pal_stream_attributes streamAttributes;
    struct pal_device device;

    STHAL_VERBOSE(LOG_TAG, "Enter");

    device.id = PAL_DEVICE_IN_HANDSET_VA_MIC;
    device.config.sample_rate = 48000;
    device.config.bit_width = 16;
    device.config.ch_info.channels = 2;
    device.config.ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
    device.config.ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;
    device.config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    streamAttributes.type = stream;
    streamAttributes.flags = (pal_stream_flags_t)0;
    streamAttributes.direction = PAL_AUDIO_INPUT;
    streamAttributes.in_media_config.sample_rate = 16000;
    streamAttributes.in_media_config.bit_width = 16;
    streamAttributes.in_media_config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
    streamAttributes.in_media_config.ch_info.channels = 1;
    streamAttributes.in_media_config.ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;

    status = pal_stream_open(&streamAttributes, 1, &device, 0, nullptr,
                             &palCallback, (uint64_t)this, &mPalHandle);

    if (status) {
        STHAL_ERR(LOG_TAG, "Pal Stream Open Error (%d)", status);
        status = -EINVAL;
    }

    STHAL_VERBOSE(LOG_TAG, "Exit, status = %d", status);
    return status;
}

int SoundTriggerSession::configurePALSession_l(const SoundModel &model,
                                               std::vector<uint8_t> & payload)
{
    int status = 0;
    pal_stream_type_t stream;
    pal_param_payload *paramPayload = nullptr;

    stream = CoreUtils::getStreamType(model);
    status = openPALStream(stream);
    if (status) {
        STHAL_ERR(LOG_TAG, " error, failed to open PAL stream");
        goto exit;
    }

    paramPayload = reinterpret_cast<pal_param_payload *> (payload.data());
    status = pal_stream_set_param(mPalHandle,
                                  PAL_PARAM_ID_LOAD_SOUND_MODEL,
                                  paramPayload);

exit:
    STHAL_INFO(LOG_TAG, "Exit, status = %d", status);
    return status;
}

int SoundTriggerSession::loadSoundModel_l(const SoundModel &aidlModel)
{
    int status = 0;
    std::vector<uint8_t> payload;

    STHAL_INFO(LOG_TAG, "Enter handle %d, model %s", mSessionHandle,
                                        aidlModel.toString().c_str());
    if(!CoreUtils::isValidSoundModel(aidlModel)) {
        STHAL_INFO(LOG_TAG, "invalid sound model");
        return -EINVAL;
    }

    AidlToPalConverter::convertSoundModel(aidlModel, payload);

    status = configurePALSession_l(aidlModel, payload);
    if (status) {
        STHAL_ERR(LOG_TAG, "failed to load sound model into PAL status = %d", status);
        goto exit;
    }

    setSessionState_l(SessionState::LOADED);

exit:
    STHAL_INFO(LOG_TAG, "Exit, status = %d", status);
    return status;
}

int SoundTriggerSession::loadPhraseSoundModel_l(const PhraseSoundModel &aidlModel)
{
    int status = 0;
    std::vector<uint8_t> payload;

    STHAL_INFO(LOG_TAG, "Enter, handle %d model %s", mSessionHandle,
                                     aidlModel.toString().c_str());
    if(!CoreUtils::isValidPhraseSoundModel(aidlModel)) {
        STHAL_INFO(LOG_TAG, "invalid pharse sound model");
        return -EINVAL;
    }

    AidlToPalConverter::convertPhraseSoundModel(aidlModel, payload);

    status = configurePALSession_l(aidlModel.common, payload);
    if (status) {
        STHAL_ERR(LOG_TAG, "failed to load sound model into PAL status %d", status);
        goto exit;
    }

    setSessionState_l(SessionState::LOADED);

exit:
    STHAL_INFO(LOG_TAG, "Exit, status = %d", status);
    return status;
}

int SoundTriggerSession::unloadSoundModel_l()
{
    int status = 0;

    STHAL_INFO(LOG_TAG, "Enter, handle %d", mSessionHandle);
    if (isSessionActive_l()) {
        status = stopRecognition_l();
        if (status) {
            STHAL_ERR(LOG_TAG, "Failed to stop recognition, status = %d", status);
        }
    }

    status = pal_stream_close(mPalHandle);
    if (status) {
        STHAL_ERR(LOG_TAG, "Failed to close PAL stream, status = %d", status);
    }
    mPalHandle = nullptr;

    setSessionState_l(SessionState::IDLE);

    STHAL_INFO(LOG_TAG, "Exit, status = %d", status);
    return status;
}

int SoundTriggerSession::startRecognition_l(int32_t deviceHandle, int32_t ioHandle,
                                            const RecognitionConfig &config)
{
    int status = 0;
    std::vector<uint8_t> payload;

    STHAL_INFO(LOG_TAG, "Enter, handle %d, config %s", mSessionHandle, config.toString().c_str());

    AidlToPalConverter::convertRecognitionConfig(config, payload);
    pal_param_payload *paramPayload = reinterpret_cast<pal_param_payload *> (payload.data());

    struct pal_st_recognition_config *palRecConfig =
                            (struct pal_st_recognition_config *)paramPayload->payload;

    palRecConfig->cookie = (uint8_t *)this;
    palRecConfig->capture_device = (uint32_t)deviceHandle;
#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
    if (AidlToPalConverter::amodel_mode > 0)
        palRecConfig->capture_handle = AUDIO_IO_HANDLE_NONE;
    else
#endif
    palRecConfig->capture_handle = ioHandle;
    CoreUtils::printPalStRecognitionConfig(palRecConfig);

    status = pal_stream_set_param(mPalHandle,
                                  PAL_PARAM_ID_RECOGNITION_CONFIG,
                                  paramPayload);
    if (status) {
        STHAL_ERR(LOG_TAG, "error, failed to set recognition config, status = %d", status);
        goto exit;
    }

    status = pal_stream_start(mPalHandle);
    if (status) {
        STHAL_ERR(LOG_TAG, "error, failed to start PAL stream, status = %d", status);
        goto exit;
    }

    setSessionState_l(SessionState::ACTIVE);

exit:
    STHAL_INFO(LOG_TAG, "Exit, handle %d, status = %d", mSessionHandle, status);
    return status;
}

int SoundTriggerSession::stopRecognition_l()
{
    int status = 0;

    STHAL_INFO(LOG_TAG, "Enter, handle %d", mSessionHandle);

    status = pal_stream_stop(mPalHandle);
    if (status) {
        STHAL_ERR(LOG_TAG, "error, failed to stop PAL stream, status = %d", status);
    }

    setSessionState_l(SessionState::STOPPED);

    STHAL_INFO(LOG_TAG, "Exit, handle %d, status = %d", mSessionHandle, status);
    return status;
}

} // namespace aidl::android::hardware::soundtrigger3
