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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "StreamInCall"

#include "StreamInCall.h"
#include "Session.h"
#include "kvh2xml.h"
#include "SessionGsl.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include <unistd.h>

StreamInCall::StreamInCall(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
                    const uint32_t no_of_devices, const struct modifier_kv *modifiers,
                    const uint32_t no_of_modifiers, const std::shared_ptr<ResourceManager> rm)
{
    mStreamMutex.lock();
    uint32_t in_channels = 0, out_channels = 0;
    uint32_t attribute_size = 0;

    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby, can not create stream");
        usleep(SSR_RECOVERY);
        mStreamMutex.unlock();
        throw std::runtime_error("Sound card offline/standby");
    }

    session = NULL;
    mGainLevel = -1;
    mStreamAttr = (struct pal_stream_attributes *)nullptr;
    mDevices.clear();
    currentState = STREAM_IDLE;
    //Modify cached values only at time of SSR down.
    cachedState = STREAM_IDLE;

    PAL_DBG(LOG_TAG, "Enter");

    //TBD handle modifiers later
    mNoOfModifiers = 0; //no_of_modifiers;
    mModifiers = (struct modifier_kv *) (NULL);
    std::ignore = modifiers;
    std::ignore = no_of_modifiers;

    // Setting default volume to unity
    mVolumeData = (struct pal_volume_data *)malloc(sizeof(struct pal_volume_data)
                      +sizeof(struct pal_channel_vol_kv));
    if (!mVolumeData) {
        PAL_ERR(LOG_TAG, "Failed to allocate memory for volume data");
        mStreamMutex.unlock();
        throw std::runtime_error("failed to allocate memory for volume data");
    }
    mVolumeData->no_of_volpair = 1;
    mVolumeData->volume_pair[0].channel_mask = 0x03;
    mVolumeData->volume_pair[0].vol = 1.0f;

    if (!sattr || !dattr) {
        PAL_ERR(LOG_TAG,"invalid arguments");
        mStreamMutex.unlock();
        throw std::runtime_error("invalid arguments");
    }

    attribute_size = sizeof(struct pal_stream_attributes);
    mStreamAttr = (struct pal_stream_attributes *) calloc(1, attribute_size);
    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "malloc for stream attributes failed %s", strerror(errno));
        mStreamMutex.unlock();
        throw std::runtime_error("failed to malloc for stream attributes");
    }

    ar_mem_cpy(mStreamAttr, sizeof(pal_stream_attributes), sattr, sizeof(pal_stream_attributes));

    if (mStreamAttr->in_media_config.ch_info.channels > PAL_MAX_CHANNELS_SUPPORTED) {
        PAL_ERR(LOG_TAG,"in_channels is invalid %d", in_channels);
        mStreamAttr->in_media_config.ch_info.channels = PAL_MAX_CHANNELS_SUPPORTED;
    }
    if (mStreamAttr->out_media_config.ch_info.channels > PAL_MAX_CHANNELS_SUPPORTED) {
        PAL_ERR(LOG_TAG,"out_channels is invalid %d", out_channels);
        mStreamAttr->out_media_config.ch_info.channels = PAL_MAX_CHANNELS_SUPPORTED;
    }

    PAL_VERBOSE(LOG_TAG, "Create new Session");
    session = Session::makeSession(rm, sattr);
    if (!session) {
        PAL_ERR(LOG_TAG, "session creation failed");
        free(mStreamAttr);
        mStreamMutex.unlock();
        throw std::runtime_error("failed to create session object");
    }

    PAL_VERBOSE(LOG_TAG, "Create new Devices with no_of_devices - %d", no_of_devices);

    mStreamMutex.unlock();
    rm->registerStream(this);
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);
    return;
}

int32_t  StreamInCall::open()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK device count - %zu", session,
                mDevices.size());
    mStreamMutex.lock();
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby, can not open stream");
        usleep(SSR_RECOVERY);
        status = -EIO;
        goto exit;
    }

    if (currentState == STREAM_IDLE) {
        status = session->open(this);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session open failed with status %d", status);
            goto exit;
        }
        PAL_VERBOSE(LOG_TAG, "session open successful");

        currentState = STREAM_INIT;
        PAL_DBG(LOG_TAG, "Exit. streamInCall opened. state %d", currentState);
    } else if (currentState == STREAM_INIT) {
        PAL_INFO(LOG_TAG, "Stream is already opened, state %d", currentState);
        status = 0;
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Stream is not in correct state %d", currentState);
        //TBD : which error code to return here.
        status = -EINVAL;
        goto exit;
    }
exit:
    palStateEnqueue(this, PAL_STATE_OPENED, status);
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t  StreamInCall::close()
{
    int32_t status = 0;
    mStreamMutex.lock();

    if (currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already closed");
        mStreamMutex.unlock();
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK device count - %zu state %d",
            session, mDevices.size(), currentState);

    if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        mStreamMutex.unlock();
        status = stop();
        if (0 != status)
            PAL_ERR(LOG_TAG, "stream stop failed. status %d",  status);
        mStreamMutex.lock();
    }

    rm->lockGraph();
    status = session->close(this);
    rm->unlockGraph();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session close failed with status %d", status);
    }

    currentState = STREAM_IDLE;
    palStateEnqueue(this, PAL_STATE_CLOSED, status);
    mStreamMutex.unlock();


    PAL_DBG(LOG_TAG, "Exit. closed the stream successfully %d status %d",
             currentState, status);
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamInCall::start()
{
    int32_t status = 0;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
              session, mStreamAttr->direction, currentState);
#else			  
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
              session, mStreamAttr->direction, currentState);
#endif
    mStreamMutex.lock();
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_STARTED;
        PAL_ERR(LOG_TAG, "Sound card offline/standby. Update the cached state %d",
                cachedState);
        goto exit;
    }


    if (currentState == STREAM_INIT || currentState == STREAM_STOPPED) {
        switch (mStreamAttr->direction) {
        case PAL_AUDIO_OUTPUT:
            rm->lockGraph();
            PAL_VERBOSE(LOG_TAG, "Inside PAL_AUDIO_OUTPUT device count - %zu",
                            mDevices.size());

            status = session->prepare(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Rx session prepare is failed with status %d",
                        status);
                rm->unlockGraph();
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "session prepare successful");

            status = session->start(this);
            if (errno == -ENETRESET) {
                if (PAL_CARD_STATUS_UP(rm->cardState)) {
                    PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
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
                goto exit;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Rx session start is failed with status %d",
                        status);
                rm->unlockGraph();
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "session start successful");
            rm->unlockGraph();
            break;

        case PAL_AUDIO_INPUT:
            PAL_VERBOSE(LOG_TAG, "Inside PAL_AUDIO_INPUT device count - %zu",
                        mDevices.size());

            status = session->prepare(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Tx session prepare is failed with status %d",
                        status);
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "session prepare successful");

            status = session->start(this);
            if (errno == -ENETRESET) {
                if (PAL_CARD_STATUS_UP(rm->cardState)) {
                    PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
                    rm->ssrHandler(CARD_STATUS_OFFLINE);
                }
                status = 0;
                cachedState = STREAM_STARTED;
                goto exit;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Tx session start is failed with status %d",
                        status);
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "session start successful");
            break;
        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Stream type is not supported, status %d", status);
            break;
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

exit:
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);
    mStreamMutex.unlock();
    rm->unlockActiveStream();
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamInCall::stop()
{
    int32_t status = 0;

    mStreamMutex.lock();
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
                session, mStreamAttr->direction, currentState);

    if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        switch (mStreamAttr->direction) {
        case PAL_AUDIO_OUTPUT:
            PAL_VERBOSE(LOG_TAG, "In PAL_AUDIO_OUTPUT case, device count - %zu",
                        mDevices.size());

            status = session->stop(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Rx session stop failed with status %d", status);
            }
            PAL_VERBOSE(LOG_TAG, "session stop successful");
            break;

        case PAL_AUDIO_INPUT:
            PAL_ERR(LOG_TAG, "In PAL_AUDIO_INPUT case, device count - %zu",
                        mDevices.size());

            status = session->stop(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Tx session stop failed with status %d", status);
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "session stop successful");
            break;

        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Stream type is not supported with status %d", status);
            break;
        }
        currentState = STREAM_STOPPED;
    } else if (currentState == STREAM_STOPPED || currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already in Stopped state %d", currentState);
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Stream should be in start/pause state, %d", currentState);
        status = -EINVAL;
        goto exit;
    }

exit:
   palStateEnqueue(this, PAL_STATE_STOPPED, status);
   mStreamMutex.unlock();
   PAL_DBG(LOG_TAG, "Exit. status %d, state %d", status, currentState);
   return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamInCall::prepare()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);

    mStreamMutex.lock();
    status = session->prepare(this);
    if (0 != status)
        PAL_ERR(LOG_TAG, "session prepare failed with status = %d", status);
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. status - %d", status);

    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t  StreamInCall::setStreamAttributes(struct pal_stream_attributes *sattr)
{
    int32_t status = -EINVAL;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);

    if (!sattr)
    {
        PAL_ERR(LOG_TAG, "NULL stream attributes sent");
        goto exit;
    }
    memset(mStreamAttr, 0, sizeof(struct pal_stream_attributes));
    mStreamMutex.lock();
    ar_mem_cpy (mStreamAttr, sizeof(struct pal_stream_attributes), sattr,
                      sizeof(struct pal_stream_attributes));
    mStreamMutex.unlock();
    status = session->setConfig(this, MODULE, 0);  //TODO:gkv or ckv or tkv need to pass
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session setConfig failed with status %d", status);
        goto exit;
    }
    PAL_DBG(LOG_TAG, "Exit. session setConfig successful");

exit:
    return status;
}

int32_t StreamInCall::setVolume(struct pal_volume_data *volume)
{
    int32_t status = 0;
    uint8_t volSize = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    if (!volume || (volume->no_of_volpair == 0)) {
       PAL_ERR(LOG_TAG, "Invalid arguments");
       status = -EINVAL;
       goto exit;
    }

    // if already allocated free and reallocate
    if (mVolumeData) {
        free(mVolumeData);
        mVolumeData = NULL;
    }

    volSize = sizeof(uint32_t) + (sizeof(struct pal_channel_vol_kv) * (volume->no_of_volpair));
    mVolumeData = (struct pal_volume_data *)calloc(1, volSize);
    if (!mVolumeData) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "failed to calloc for volume data");
        goto exit;
    }

    /* Allow caching of stream volume as part of mVolumeData
     * till the pcm_open is not done or if sound card is offline.
     */
    ar_mem_cpy(mVolumeData, volSize, volume, volSize);
    for (int32_t i=0; i < (mVolumeData->no_of_volpair); i++) {
        PAL_INFO(LOG_TAG, "Volume payload mask:%x vol:%f",
                      (mVolumeData->volume_pair[i].channel_mask), (mVolumeData->volume_pair[i].vol));
    }

    if (a2dpMuted) {
        PAL_DBG(LOG_TAG, "a2dp muted, just cache volume update");
        goto exit;
    }

    if ((rm->cardState == CARD_STATUS_ONLINE) && (currentState != STREAM_IDLE)
            && (currentState != STREAM_INIT)) {
        status = session->setConfig(this, CALIBRATION, TAG_STREAM_VOLUME);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session setConfig for VOLUME_TAG failed with status %d",
                    status);
            goto exit;
        }
        if (unMutePending) {
            unMutePending = false;
            mute_l(false);
        }
    }

exit:
    if (volume) {
        PAL_DBG(LOG_TAG, "Exit. Volume payload No.of vol pair:%d ch mask:%x gain:%f",
                         (volume->no_of_volpair), (volume->volume_pair->channel_mask),
                         (volume->volume_pair->vol));
    }
    return status;
}

int32_t  StreamInCall::read(struct pal_buffer* buf)
{
    int32_t status = 0;
    int32_t size;
    PAL_VERBOSE(LOG_TAG, "Enter. session handle - %pK, state %d",
            session, currentState);

    mStreamMutex.lock();
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
            || cachedState != STREAM_IDLE) {
       /* calculate sleep time based on buf->size, sleep and return buf->size */
        uint32_t streamSize;
        uint32_t byteWidth = mStreamAttr->in_media_config.bit_width / 8;
        uint32_t sampleRate = mStreamAttr->in_media_config.sample_rate;
        struct pal_channel_info chInfo = mStreamAttr->in_media_config.ch_info;

        streamSize = byteWidth * chInfo.channels;
        if ((streamSize == 0) || (sampleRate == 0)) {
            PAL_ERR(LOG_TAG, "stream_size= %d, srate = %d",
                    streamSize, sampleRate);
            status =  -EINVAL;
            goto exit;
        }
        size = buf->size;
        memset(buf->buffer, 0, size);
        usleep((uint64_t)size * 1000000 / streamSize / sampleRate);
        PAL_DBG(LOG_TAG, "Sound card offline, dropped buffer size - %d", size);
        status = size;
        goto exit;
    }

    if (currentState == STREAM_STARTED) {
        status = session->read(this, SHMEM_ENDPOINT, buf, &size);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session read is failed with status %d", status);
            if (errno == -ENETRESET &&
                (PAL_CARD_STATUS_UP(rm->cardState))) {
                PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
                rm->ssrHandler(CARD_STATUS_OFFLINE);
                size = buf->size;
                status = size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
                size = buf->size;
                status = size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else {
                status = errno;
                goto exit;
            }
        }
    } else {
        PAL_ERR(LOG_TAG, "Stream not started yet, state %d", currentState);
        status = -EINVAL;
        goto exit;
    }
    mStreamMutex.unlock();
    PAL_VERBOSE(LOG_TAG, "Exit. session read successful size - %d", size);
    return size;
exit :
    mStreamMutex.unlock();
    PAL_VERBOSE(LOG_TAG, "Exit session read failed status %d", status);
    return status;
}

int32_t  StreamInCall::write(struct pal_buffer* buf)
{
    int32_t status = 0;
    int32_t size = 0;
    uint32_t frameSize = 0;
    uint32_t byteWidth = 0;
    uint32_t sampleRate = 0;
    uint32_t channelCount = 0;

    PAL_VERBOSE(LOG_TAG, "Enter. session handle - %pK, state %d",
            session, currentState);

    mStreamMutex.lock();
    // If cached state is not STREAM_IDLE, we are still processing SSR up.
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
            || cachedState != STREAM_IDLE) {
        byteWidth = mStreamAttr->out_media_config.bit_width / 8;
        sampleRate = mStreamAttr->out_media_config.sample_rate;
        channelCount = mStreamAttr->out_media_config.ch_info.channels;

        frameSize = byteWidth * channelCount;
        if ((frameSize == 0) || (sampleRate == 0)) {
            PAL_ERR(LOG_TAG, "frameSize=%d, sampleRate=%d", frameSize, sampleRate);
            mStreamMutex.unlock();
            return -EINVAL;
        }
        size = buf->size;
        usleep((uint64_t)size * 1000000 / frameSize / sampleRate);
        PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
        mStreamMutex.unlock();
        PAL_VERBOSE(LOG_TAG, "Exit size: %d", size);
        return size;
    }

    if (currentState == STREAM_STARTED) {
        status = session->write(this, SHMEM_ENDPOINT, buf, &size, 0);
        mStreamMutex.unlock();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session write is failed with status %d", status);

            /* ENETRESET is the error code returned by AGM during SSR */
            if (errno == -ENETRESET &&
                (PAL_CARD_STATUS_UP(rm->cardState))) {
                PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
                rm->ssrHandler(CARD_STATUS_OFFLINE);
                size = buf->size;
                status = size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
                size = buf->size;
                status = size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else {
                status = errno;
                goto exit;
            }
        }
        PAL_DBG(LOG_TAG, "Exit. session write successful size - %d", size);
        return size;
    } else if (currentState == STREAM_PAUSED) {
        //if data is written during pause size 0 will be returned and no data will be written
        mStreamMutex.unlock();
        return size;
    } else {
        PAL_ERR(LOG_TAG, "Stream not started yet, state %d", currentState);
        if (currentState == STREAM_STOPPED)
            status = -EIO;
        else
            status = -EINVAL;

        mStreamMutex.unlock();
        goto exit;
    }

exit:
    PAL_ERR(LOG_TAG, "Exit. session write failed status %d", status);
    return status;
}

int32_t  StreamInCall::registerCallBack(pal_stream_callback /*cb*/, uint64_t /*cookie*/)
{
    return 0;
}

int32_t  StreamInCall::getCallBack(pal_stream_callback * /*cb*/)
{
    return 0;
}

int32_t StreamInCall::getParameters(uint32_t /*param_id*/, void ** /*payload*/)
{
    return 0;
}

int32_t  StreamInCall::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = 0;

    if (!payload)
    {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "wrong params");
        goto error;
    }

    PAL_DBG(LOG_TAG, "start, set parameter %u, session handle - %p", param_id, session);

    mStreamMutex.lock();
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE for param ID: %d", param_id);
        mStreamMutex.unlock();
        return -EINVAL;
    }
    // Stream may not know about tags, so use setParameters instead of setConfig
    switch (param_id) {
        default:
            PAL_ERR(LOG_TAG, "Unsupported param id %u", param_id);
            status = -EINVAL;
            break;
    }

    mStreamMutex.unlock();
    PAL_VERBOSE(LOG_TAG, "exit, session parameter %u set with status %d", param_id, status);
error:
    return status;
}

int32_t StreamInCall::mute_l(bool state)
{
    int32_t status = 0;
    bool mute_by_volume = true;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "Enter. session handle - %pK state %d", session, state);
#else
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK state %d", session, state);
#endif
    if (mVolumeData) {
        for (int32_t i = 0; i < (mVolumeData->no_of_volpair); i++) {
            if (mVolumeData->volume_pair[i].vol != 0.0f) {
                mute_by_volume = false;
                PAL_VERBOSE(LOG_TAG, "Volume payload mask:%x vol:%f",
                   (mVolumeData->volume_pair[i].channel_mask), (mVolumeData->volume_pair[i].vol));
                break;
           }
        }
    }
    if (mute_by_volume) {
        PAL_DBG(LOG_TAG, "Skip mute/unmute as stream muted by volume");
        unMutePending = !state;
        goto exit;
    }
    status = session->setConfig(this, MODULE, (state ? MUTE_TAG : UNMUTE_TAG), TX_HOSTLESS);

exit:
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "Exit status: %d", status);
#else
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
#endif
    return status;
}

int32_t StreamInCall::mute(bool state)
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = mute_l(state);
    mStreamMutex.unlock();

    return status;
}

int32_t StreamInCall::pause_l()
{
    int32_t status = 0;
    std::unique_lock<std::mutex> pauseLock(pauseMutex);
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_PAUSED;
        isPaused = true;
        PAL_ERR(LOG_TAG, "Sound Card Offline/standby, cached state %d", cachedState);
        goto exit;
    }

    status = session->setConfig(this, MODULE, PAUSE_TAG);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session setConfig for pause failed with status %d",
                status);
        goto exit;
    }

    isPaused = true;
    currentState = STREAM_PAUSED;
    palStateEnqueue(this, PAL_STATE_PAUSED, status);
    PAL_DBG(LOG_TAG, "Exit. session setConfig successful");
exit:
    return status;
}

int32_t StreamInCall::pause()
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = pause_l();
    mStreamMutex.unlock();

    return status;
}

int32_t StreamInCall::resume_l()
{
    int32_t status = 0;
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_STARTED;
        PAL_ERR(LOG_TAG, "Sound Card offline/standby, cached state %d", cachedState);
        goto exit;
    }

    status = session->setConfig(this, MODULE, RESUME_TAG);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session setConfig for pause failed with status %d",
                status);
        goto exit;
    }
    isPaused = false;
    currentState = STREAM_STARTED;
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit. session setConfig successful");
exit:
    return status;
}

int32_t StreamInCall::resume()
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = resume_l();
    mStreamMutex.unlock();

    return status;
}

int32_t StreamInCall::flush()
{
    int32_t status = 0;

    mStreamMutex.lock();
    PAL_DBG(LOG_TAG, "flush called for stream type %d", mStreamAttr->type);

    if (isPaused == false) {
         PAL_ERR(LOG_TAG, "Error, flush called while stream is not Paused isPaused:%d", isPaused);
         goto exit;
    }

    if (currentState == STREAM_STOPPED || currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Already flushed, state %d", currentState);
        goto exit;
    }

    status = session->flush();
exit:
    mStreamMutex.unlock();
    return status;
}

int32_t StreamInCall::isSampleRateSupported(uint32_t sampleRate)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "sampleRate %u", sampleRate);
    switch(sampleRate) {
        case SAMPLINGRATE_8K:
        case SAMPLINGRATE_16K:
        case SAMPLINGRATE_22K:
        case SAMPLINGRATE_32K:
        case SAMPLINGRATE_44K:
        case SAMPLINGRATE_48K:
        case SAMPLINGRATE_96K:
        case SAMPLINGRATE_192K:
        case SAMPLINGRATE_384K:
            break;
       default:
            rc = 0;
            PAL_VERBOSE(LOG_TAG, "sample rate received %d rc %d", sampleRate, rc);
            break;
    }
    return rc;
}

int32_t StreamInCall::isChannelSupported(uint32_t numChannels)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    switch(numChannels) {
        case CHANNELS_1:
        case CHANNELS_2:
        case CHANNELS_3:
        case CHANNELS_4:
        case CHANNELS_5:
        case CHANNELS_5_1:
        case CHANNELS_7:
        case CHANNELS_8:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "channels not supported %d rc %d", numChannels, rc);
            break;
    }
    return rc;
}

int32_t StreamInCall::isBitWidthSupported(uint32_t bitWidth)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "bitWidth %u", bitWidth);
    switch(bitWidth) {
        case BITWIDTH_16:
        case BITWIDTH_24:
        case BITWIDTH_32:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "bit width not supported %d rc %d", bitWidth, rc);
            break;
    }
    return rc;
}

int32_t StreamInCall::setECRef(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = setECRef_l(dev, is_enable);
    mStreamMutex.unlock();

    return status;
}

int32_t StreamInCall::setECRef_l(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;

    if (!session)
        return -EINVAL;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);

    status = session->setECRef(this, dev, is_enable);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to set ec ref in session");
    }

    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}

int32_t StreamInCall::ssrDownHandler()
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
        status = stop();
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
skip_down_handling:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamInCall::addRemoveEffect(pal_audio_effect_t /*effect*/, bool /*enable*/)
{
    PAL_ERR(LOG_TAG, "Function not supported");
    return -ENOSYS;
}

int32_t StreamInCall::ssrUpHandler()
{
    int32_t status = 0;

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
        goto exit;
    }
exit :
    cachedState = STREAM_IDLE;
skip_up_handling :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamInCall::reconfigureModule(uint32_t tagID, const char* BE, struct sessionToPayloadParam  *data)
{
    uint32_t status =0;
    SessionAlsaPcm* sPCM = nullptr;
    if(!session){
        PAL_ERR(LOG_TAG,"no session cannot configure")
        status = -EINVAL;
        goto exit;
   }
   sPCM = dynamic_cast<SessionAlsaPcm*>(session);
   status = sPCM->reconfigureModule(tagID, BE, data);

exit:
    return status;
}

StreamInCall::~StreamInCall(){

    cachedState = STREAM_IDLE;
    rm->resetStreamInstanceID(this);
    rm->deregisterStream(this);
    if (mStreamAttr) {
        free(mStreamAttr);
        mStreamAttr = (struct pal_stream_attributes *)NULL;
    }

    if(mVolumeData)  {
        free(mVolumeData);
        mVolumeData = (struct pal_volume_data *)NULL;
    }

    /*switch back to proper config if there is a concurrency and device is still running*/
    for (int32_t i=0; i < mDevices.size(); i++)
        rm->restoreDevice(mDevices[i]);

    mDevices.clear();
    delete session;
    session = nullptr;
}
