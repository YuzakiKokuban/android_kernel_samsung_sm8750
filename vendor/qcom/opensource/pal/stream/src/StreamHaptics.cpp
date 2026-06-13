/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
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
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "PAL: StreamHaptics"

#include "StreamHaptics.h"
#include "Session.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include <unistd.h>
#include "rx_haptics_api.h"
#include "wsa_haptics_vi_api.h"

StreamHaptics::StreamHaptics(const struct pal_stream_attributes *sattr, struct pal_device *dattr __unused,
                    const uint32_t no_of_devices __unused, const struct modifier_kv *modifiers __unused,
                    const uint32_t no_of_modifiers __unused, const std::shared_ptr<ResourceManager> rm):
                  StreamPCM(sattr,dattr,no_of_devices,modifiers,no_of_modifiers,rm)
{
    session->registerCallBack((session_callback)HandleCallBack,((uint64_t) this));
}

StreamHaptics::~StreamHaptics()
{
}

int32_t  StreamHaptics::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = -1;
    std::vector <Stream *> activeStreams;
    struct pal_stream_attributes ActivesAttr;
    Stream *stream = nullptr;

    if (!payload)
    {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "invalid params");
        goto error;
    }

    PAL_DBG(LOG_TAG, "Enter, set parameter %u", param_id);
    status = rm->getActiveStream_l(activeStreams, mDevices[0]);
    if ((0 != status) || (activeStreams.size() == 0 )) {
         PAL_DBG(LOG_TAG, "No Haptics stream is active");
         goto error;
    } else {
       /* If Setparam for touch haptics is called when Ringtone Haptics is active
          skip the Setparam for touch haptics*/
        PAL_DBG(LOG_TAG, "activestreams size %d",activeStreams.size());
        for (int i = 0; i<activeStreams.size(); i++) {
            stream = static_cast<Stream *>(activeStreams[i]);
            stream->getStreamAttributes(&ActivesAttr);
            if (ActivesAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_RINGTONE) {
                status = -EINVAL;
                goto error;
            } else
                continue;
        }
    }

    mStreamMutex.lock();
    // Stream may not know about tags, so use setParameters instead of setConfig
    switch (param_id) {
        case PAL_PARAM_ID_HAPTICS_CNFG:
        {
            status = session->setParameters(NULL, 0, param_id, payload);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d, Failed to setParam for registering an event",
                          status);
            break;
        }
        case PARAM_ID_HAPTICS_WAVE_DESIGNER_STOP_PARAM:
        {
            status = session->setParameters(NULL, 0, param_id, payload);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d, Failed to setParam for registering an event",
                          status);
            break;
        }
        case PARAM_ID_HAPTICS_WAVE_DESIGNER_UPDATE_PARAM:
        {
            status = session->setParameters(NULL, 0, param_id, payload);
            if (status)
                PAL_ERR(LOG_TAG, "Error:%d, Failed to setParam", status);
            break;
        }
        case PARAM_ID_HAPTICS_EX_VI_PERSISTENT:
        {
            status =  mDevices[0]->setParameter(param_id, nullptr);
            if (status)
               PAL_ERR(LOG_TAG, "Error:%d, Failed to setParam", status);
            break;
        }
        default:
            PAL_ERR(LOG_TAG, "Error:Unsupported param id %u", param_id);
            status = -EINVAL;
            break;
    }

    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "exit, session parameter %u set with status %d", param_id, status);
error:
    return status;
}

int32_t StreamHaptics::start()
{
    int32_t status = 0, devStatus = 0, cachedStatus = 0;
    int32_t tmp = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
            session, mStreamAttr->direction, currentState);

    /* check for haptic concurrency*/
    if (rm->IsHapticsThroughWSA())
        HandleHapticsConcurrency(mStreamAttr);

    mStreamMutex.lock();
    if (rm->cardState == CARD_STATUS_OFFLINE) {
        cachedState = STREAM_STARTED;
        PAL_ERR(LOG_TAG, "Sound card offline. Update the cached state %d",
                cachedState);
        goto exit;
    }

    if (currentState == STREAM_INIT || currentState == STREAM_STOPPED) {
        switch (mStreamAttr->direction) {
        case PAL_AUDIO_OUTPUT:
            PAL_VERBOSE(LOG_TAG, "Inside PAL_AUDIO_OUTPUT device count - %zu",
                            mDevices.size());

            rm->lockGraph();
            /* Any device start success will be treated as positive status.
             * This allows stream be played even if one of devices failed to start.
             */
            status = -EINVAL;
            for (int32_t i=0; i < mDevices.size(); i++) {
                devStatus = mDevices[i]->start();
                if (devStatus == 0) {
                    status = 0;
                } else {
                    cachedStatus = devStatus;

                    tmp = session->disconnectSessionDevice(this, mStreamAttr->type, mDevices[i]);
                    if (0 != tmp) {
                        PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", tmp);
                    }

                    tmp = mDevices[i]->close();
                    if (0 != tmp) {
                        PAL_ERR(LOG_TAG, "device close failed with status %d", tmp);
                    }
                    mDevices.erase(mDevices.begin() + i);
                    i--;
                }
            }
            if (0 != status) {
                status = cachedStatus;
                PAL_ERR(LOG_TAG, "Rx device start failed with status %d", status);
                rm->unlockGraph();
                goto exit;
            } else {
                PAL_VERBOSE(LOG_TAG, "devices started successfully");
            }

            status = session->prepare(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Rx session prepare is failed with status %d",
                        status);
                rm->unlockGraph();
                goto session_fail;
            }
            PAL_VERBOSE(LOG_TAG, "session prepare successful");

            status = session->start(this);
            if (errno == -ENETRESET) {
                if (rm->cardState != CARD_STATUS_OFFLINE) {
                    PAL_ERR(LOG_TAG, "Sound card offline, informing RM");
                    rm->ssrHandler(CARD_STATUS_OFFLINE);
                }
                cachedState = STREAM_STARTED;
                /* Returning status 0,  hal shouldn't be
                 * informed of failure because we have cached
                 * the state and will start from STARTED state
                 * during SSR up Handling.
                 */
                status = 0;
                rm->unlockGraph();
                goto session_fail;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Rx session start is failed with status %d",
                        status);
                rm->unlockGraph();
                goto session_fail;
            }
            PAL_VERBOSE(LOG_TAG, "session start successful");
            rm->unlockGraph();

            break;
        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Stream type is not supported, status %d", status);
            break;
        }
        for (int i = 0; i < mDevices.size(); i++) {
            rm->registerDevice(mDevices[i], this);
        }
        /*pcm_open and pcm_start done at once here,
         *so directly jump to STREAM_STARTED state.
         */
        currentState = STREAM_STARTED;
    } else if (currentState == STREAM_STARTED) {
        PAL_INFO(LOG_TAG, "Stream already started, state %d", currentState);
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Stream is not opened yet");
        status = -EINVAL;
        goto exit;
    }
    goto exit;
session_fail:
    for (int32_t i=0; i < mDevices.size(); i++) {
        devStatus = mDevices[i]->stop();
        if (devStatus)
            status = devStatus;
        rm->deregisterDevice(mDevices[i], this);
    }
exit:
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit. state %d, status %d", currentState, status);
    mStreamMutex.unlock();
    return status;
}

int32_t StreamHaptics::HandleHapticsConcurrency(struct pal_stream_attributes *sattr)
{
    std::vector <Stream *> activeStreams;
    struct pal_stream_attributes ActivesAttr;
    Stream *stream = nullptr;
    param_id_haptics_wave_designer_wave_designer_stop_param_t HapticsStopParam;
    pal_param_payload *param_payload = nullptr;
    Session *ActHapticsSession = nullptr;
    struct param_id_haptics_wave_designer_state event_info;
    int status = 0;

    /* if input stream and the priority stream is same,
       go ahead and enable the stream */
    status = rm->getActiveStream_l(activeStreams, mDevices[0]);
    if ((0 != status) || (activeStreams.size() <= 1)) {
         PAL_DBG(LOG_TAG, "No Active Haptics stream is present.");
         goto exit;
    } else {
       /* If incoming stream is Ringtone Haptics and active stream is Touch
          stop the Touch haptics and start Ringtone*/
        PAL_DBG(LOG_TAG, "activestreams size %d",activeStreams.size());
        for (int i = 0; i<activeStreams.size(); i++) {
            stream = static_cast<Stream *>(activeStreams[i]);
            stream->getStreamAttributes(&ActivesAttr);
            if (ActivesAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH) {
                /* Set the stop haptics param for touch haptics. */
                param_payload = (pal_param_payload *) calloc (1,
                               sizeof(pal_param_payload) +
                               sizeof(param_id_haptics_wave_designer_wave_designer_stop_param_t));
                if (!param_payload)
                    goto exit;
                HapticsStopParam.channel_mask = 1;
                param_payload->payload_size =
                             sizeof(param_id_haptics_wave_designer_wave_designer_stop_param_t);
                memcpy(param_payload->payload, &HapticsStopParam, param_payload->payload_size);
                stream->getAssociatedSession(&ActHapticsSession);
                status = ActHapticsSession->setParameters(NULL, 0,
                                         PARAM_ID_HAPTICS_WAVE_DESIGNER_STOP_PARAM,
                                                              (void*)param_payload);
                if (status)
                    PAL_ERR(LOG_TAG, "Error:%d, Stop SetParam is Failed", status);
            }
        }
    }
exit:
    return status;
}

int32_t  StreamHaptics::registerCallBack(pal_stream_callback cb, uint64_t cookie)
{
    callback_ = cb;
    cookie_ = cookie;

    PAL_VERBOSE(LOG_TAG, "callback_ = %pK", callback_);

    return 0;
}

void StreamHaptics::HandleEvent(uint32_t event_id, void *data, uint32_t event_size) {
    struct param_id_haptics_wave_designer_state *event_info = nullptr;
    event_info = (struct param_id_haptics_wave_designer_state *)data;
    uint32_t event_type[2];
    event_type[0] = (uint16_t)event_info->state[0];
    event_type[1] = (uint16_t)event_info->state[1];

    PAL_INFO(LOG_TAG, "event received with value for vib 1 - %d vib 2- %d",
                    event_type[0], event_type[1]);

    if (callback_) {
        PAL_INFO(LOG_TAG, "Notify detection event to client");
        callback_((pal_stream_handle_t *)this, event_id, event_type,
                  event_size, cookie_);
    }
}

void StreamHaptics::HandleCallBack(uint64_t hdl, uint32_t event_id,
                                      void *data, uint32_t event_size) {
    StreamHaptics *StreamHAPTICS = nullptr;
    PAL_DBG(LOG_TAG, "Enter, event detected on SPF, event id = 0x%x, event size =%d",
                      event_id, event_size);
    // Handle event form DSP
    if (event_id == EVENT_ID_WAVEFORM_STATE) {
        StreamHAPTICS = (StreamHaptics *)hdl;
        StreamHAPTICS->HandleEvent(event_id, data, event_size);
    }
    PAL_DBG(LOG_TAG, "Exit");
}

int32_t StreamHaptics::ssrDownHandler()
{
    int32_t status = 0;

    mStreamMutex.lock();

    if (false == isStreamSSRDownFeasibile()) {
        mStreamMutex.unlock();
        goto skip_down_handling;
    }

    /* Updating cached state here only if it's STREAM_IDLE,
     * Otherwise we can assume it is updated by hal thread
     * already.
     */
    if (cachedState == STREAM_IDLE)
        cachedState = currentState;
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK cached State %d",
            session, cachedState);

    if (currentState == STREAM_INIT || currentState == STREAM_STOPPED) {
        mStreamMutex.unlock();
        status = close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream close failed. status %d", status);
            goto exit;
        }
    } else if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        mStreamMutex.unlock();
        rm->unlockActiveStream();
        status = stop();
        rm->lockActiveStream();
        if (0 != status)
            PAL_ERR(LOG_TAG, "stream stop failed. status %d",  status);
        status = close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream close failed. status %d", status);
            goto exit;
        }
    } else {
        PAL_ERR(LOG_TAG, "stream state is %d, nothing to handle", currentState);
        mStreamMutex.unlock();
        goto exit;
    }

exit :
    currentState = STREAM_IDLE;
skip_down_handling :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamHaptics::ssrUpHandler()
{
    int32_t status = 0;

    if (mStreamAttr->info.opt_stream_info.haptics_type != PAL_STREAM_HAPTICS_RINGTONE)
        goto skip_up_handling;

    mStreamMutex.lock();
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK state %d",
            session, cachedState);

    if (skipSSRHandling) {
        skipSSRHandling = false;
        mStreamMutex.unlock();
        goto skip_up_handling;
    }

    if (cachedState == STREAM_INIT) {
        mStreamMutex.unlock();
        status = open();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream open failed. status %d", status);
            goto exit;
        }
    } else if (cachedState == STREAM_STARTED) {
        mStreamMutex.unlock();
        status = open();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream open failed. status %d", status);
            goto exit;
        }
        rm->unlockActiveStream();
        status = start();
        rm->lockActiveStream();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream start failed. status %d", status);
            goto exit;
        }
        /* For scenario when we get SSR down while handling SSR up,
         * status will be 0, so we need to have this additonal check
         * to keep the cached state as STREAM_STARTED.
         */
        if (currentState != STREAM_STARTED) {
            goto exit;
        }
    } else if (cachedState == STREAM_PAUSED) {
        mStreamMutex.unlock();
        status = open();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream open failed. status %d", status);
            goto exit;
        }
        rm->unlockActiveStream();
        status = start();
        rm->lockActiveStream();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "stream start failed. status %d", status);
            goto exit;
        }
        if (currentState != STREAM_STARTED)
            goto exit;
        status = pause();
        if (0 != status) {
           PAL_ERR(LOG_TAG, "stream set pause failed. status %d", status);
            goto exit;
        }
    } else {
        mStreamMutex.unlock();
        PAL_ERR(LOG_TAG, "stream not in correct state to handle %d", cachedState);
    }
exit :
    cachedState = STREAM_IDLE;
skip_up_handling :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}
