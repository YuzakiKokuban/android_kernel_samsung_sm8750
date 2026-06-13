/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
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

#define LOG_TAG "StreamNonTunnel"

#include "Session.h"
#include "kvh2xml.h"
#include "SessionGsl.h"
#include "ResourceManager.h"
#include <unistd.h>

static void handleSessionCallBack(uint64_t hdl, uint32_t event_id, void *data,
                                  uint32_t event_size)
{
    Stream *s = NULL;
    s = reinterpret_cast<Stream *>(hdl);
    if (s->streamCb)
        s->streamCb(reinterpret_cast<pal_stream_handle_t *>(s), event_id, (uint32_t *)data,
          event_size, s->cookie);
}

StreamNonTunnel::StreamNonTunnel(const struct pal_stream_attributes *sattr, struct pal_device *dattr __unused,
                    const uint32_t no_of_devices __unused, const struct modifier_kv *modifiers,
                    const uint32_t no_of_modifiers, const std::shared_ptr<ResourceManager> rm)
{
    mStreamMutex.lock();
    uint32_t in_channels = 0, out_channels = 0;
    uint32_t attribute_size = 0;
    if (!sattr) {
        PAL_ERR(LOG_TAG,"invalid arguments");
        mStreamMutex.unlock();
        throw std::runtime_error("invalid arguments");
    }

    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby, can not create stream");
        usleep(SSR_RECOVERY);
        mStreamMutex.unlock();
        throw std::runtime_error("Sound card offline/standby");
    }

    session = NULL;
    mStreamAttr = (struct pal_stream_attributes *)nullptr;
    inMaxMetadataSz = 0;
    outMaxMetadataSz = 0;
    mDevices.clear();
    currentState = STREAM_IDLE;
    ssrInNTMode = false;
    //Modify cached values only at time of SSR down.
    cachedState = STREAM_IDLE;

    PAL_DBG(LOG_TAG, "Enter");

    //TBD handle modifiers later
    mNoOfModifiers = 0; //no_of_modifiers;
    mModifiers = (struct modifier_kv *) (NULL);
    std::ignore = modifiers;
    std::ignore = no_of_modifiers;


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

    session->registerCallBack(handleSessionCallBack, (uint64_t)this);

    mStreamMutex.unlock();
    rm->registerStream(this);
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);
    return;
}

StreamNonTunnel::~StreamNonTunnel()
{
    rm->resetStreamInstanceID(this);
    rm->deregisterStream(this);
    if (mStreamAttr) {
        free(mStreamAttr);
        mStreamAttr = (struct pal_stream_attributes *)NULL;
    }
    delete session;
    session = nullptr;
}

int32_t  StreamNonTunnel::open()
{
    int32_t status = 0;

    mStreamMutex.lock();
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
            || ssrInNTMode == true) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby, can not open stream");
        usleep(SSR_RECOVERY);
        status = -ENETRESET;
        goto exit;
    }

    if (currentState == STREAM_IDLE) {
        PAL_VERBOSE(LOG_TAG, "Enter. session handle - %pK", session);
        status = session->open(this);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session open failed with status %d", status);
            goto exit;
        }
        PAL_VERBOSE(LOG_TAG, "session open successful");

        currentState = STREAM_INIT;
        PAL_DBG(LOG_TAG, "Exit. streamLL opened. state %d", currentState);
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
    return status;
}

int32_t  StreamNonTunnel::close()
{
    int32_t status = 0;
    mStreamMutex.lock();

    PAL_INFO(LOG_TAG, "Enter. session handle - %pK state %d",
            session, currentState);

    if (currentState == STREAM_IDLE) {
        PAL_VERBOSE(LOG_TAG, "closed the devices successfully");
        goto exit;
    } else if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        status = stop();
        if (0 != status)
            PAL_ERR(LOG_TAG, "stream stop failed. status %d",  status);
    }

    status = session->close(this);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session close failed with status %d", status);
    }

exit:
    currentState = STREAM_IDLE;
    palStateEnqueue(this, PAL_STATE_CLOSED, status);
    mStreamMutex.unlock();
    PAL_INFO(LOG_TAG, "Exit. closed the stream successfully %d status %d",
             currentState, status);
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamNonTunnel::start()
{
    int32_t status = 0;
    mStreamMutex.lock();
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
            || ssrInNTMode == true) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        status = -ENETRESET;
        goto exit;
    }

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
              session, mStreamAttr->direction, currentState);

    if (currentState == STREAM_INIT || currentState == STREAM_STOPPED ||
            currentState == STREAM_SUSPENDED) {
        status = session->prepare(this);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Rx session prepare is failed with status %d",
                    status);
            goto exit;
        }
        PAL_VERBOSE(LOG_TAG, "session prepare successful");

        status = session->start(this);
        if (status == -ENETRESET &&
                (PAL_CARD_STATUS_UP(rm->cardState))) {
                PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
                rm->ssrHandler(CARD_STATUS_OFFLINE);
                goto exit;
        }
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Rx session start is failed with status %d",
                    status);
            goto exit;
        }
        PAL_VERBOSE(LOG_TAG, "session start successful");
        currentState = STREAM_STARTED;
    } else if (currentState == STREAM_STARTED) {
        PAL_INFO(LOG_TAG, "Stream already started, state %d", currentState);
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Stream is not opened yet");
        status = -EINVAL;
        goto exit;
    }
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);

exit:
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    mStreamMutex.unlock();
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamNonTunnel::stop()
{
    int32_t status = 0;

    mStreamMutex.lock();
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
                session, mStreamAttr->direction, currentState);

    if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        status = session->stop(this);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Rx session stop failed with status %d", status);
        }
        PAL_VERBOSE(LOG_TAG, "session stop successful");
        currentState = STREAM_STOPPED;
    } else if (currentState == STREAM_STOPPED || currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already in Stopped state %d", currentState);
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Stream should be in start/pause state, %d", currentState);
        status = -EINVAL;
        goto exit;
    }
    PAL_DBG(LOG_TAG, "Exit. status %d, state %d", status, currentState);

exit:
    palStateEnqueue(this, PAL_STATE_STOPPED, status);
    mStreamMutex.unlock();
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamNonTunnel::prepare()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);

    mStreamMutex.lock();

    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
            || ssrInNTMode == true) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        mStreamMutex.unlock();
        return -ENETRESET;
    }

    status = session->prepare(this);
    if (0 != status)
        PAL_ERR(LOG_TAG, "session prepare failed with status = %d", status);
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. status - %d", status);

    return status;
}

int32_t  StreamNonTunnel::read(struct pal_buffer* buf)
{
    int32_t status = 0;
    int32_t size;
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK, state %d",
            session, currentState);

    mStreamMutex.lock();
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
             || ssrInNTMode == true) {
         PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        status = -ENETRESET;
        goto exit;
    }

    if (currentState == STREAM_STARTED) {
        status = session->read(this, SHMEM_ENDPOINT, buf, &size);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session read is failed with status %d", status);
            if (status == -ENETRESET &&
                (PAL_CARD_STATUS_UP(rm->cardState))) {
                PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
                rm->ssrHandler(CARD_STATUS_OFFLINE);
                size = buf->size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
                size = buf->size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else {
                goto exit;
            }
        }
    } else {
        PAL_ERR(LOG_TAG, "Stream not started yet, state %d", currentState);
        status = -EINVAL;
        goto exit;
    }
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. session read successful size - %d", size);
    return size;
exit :
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "session read failed status %d", status);
    return status;
}

int32_t  StreamNonTunnel::write(struct pal_buffer* buf)
{
    int32_t status = 0;
    int32_t size = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK, state %d",
            session, currentState);

    mStreamMutex.lock();

    // If cached state is not STREAM_IDLE, we are still processing SSR up.
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
            || ssrInNTMode == true) {
        size = buf->size;
        PAL_DBG(LOG_TAG, "sound card offline/standby dropped buffer size - %d", size);
        mStreamMutex.unlock();
        return -ENETRESET;
    }
    mStreamMutex.unlock();
    //we should allow writes to go through in Start/Pause state as well.
    if ( (currentState == STREAM_STARTED) ||
        (currentState == STREAM_PAUSED) ) {
        status = session->write(this, SHMEM_ENDPOINT, buf, &size, 0);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session write is failed with status %d", status);

            /* ENETRESET is the error code returned by AGM during SSR */
            if (status == -ENETRESET &&
                (PAL_CARD_STATUS_UP(rm->cardState))) {
                PAL_ERR(LOG_TAG, "Sound card offline/standby, informing RM");
                rm->ssrHandler(CARD_STATUS_OFFLINE);
                size = buf->size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
                size = buf->size;
                PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
                goto exit;
            } else {
                goto exit;
            }
         }
         PAL_DBG(LOG_TAG, "Exit. session write successful size - %d", size);
         return size;
    } else {
        PAL_ERR(LOG_TAG, "Stream not started yet, state %d", currentState);
        if (currentState == STREAM_STOPPED)
            status = -EIO;
        else
            status = -EINVAL;
        goto exit;
    }

exit :
    PAL_DBG(LOG_TAG, "session write failed status %d", status);
    return status;
}

int32_t  StreamNonTunnel::registerCallBack(pal_stream_callback cb, uint64_t cookie)
{
    streamCb = cb;
    this->cookie = cookie;
    return 0;  return 0;
}

int32_t StreamNonTunnel::getTagsWithModuleInfo(size_t *size, uint8_t *payload)
{
    int32_t status = 0;

    if (*size > 0 && !payload)
    {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "wrong params");
        goto exit;
    }

    status = session->getTagsWithModuleInfo(this, size, payload);
exit:
    return status;
}

int32_t  StreamNonTunnel::getCallBack(pal_stream_callback * /*cb*/)
{
    return 0;
}

int32_t StreamNonTunnel::getParameters(uint32_t /*param_id*/, void ** /*payload*/)
{
    return 0;
}

int32_t  StreamNonTunnel::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = 0;

    if (!payload)
    {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "wrong params");
        goto error;
    }

    mStreamMutex.lock();
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE for param ID: %d", param_id);
        mStreamMutex.unlock();
        return -EINVAL;
    }
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
             || ssrInNTMode == true) {
         PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        status = -ENETRESET;
        goto error;
    }

    PAL_DBG(LOG_TAG, "start, set parameter %u, session handle - %p", param_id, session);

    // Stream may not know about tags, so use setParameters instead of setConfig
    switch (param_id) {
        case PAL_PARAM_ID_MODULE_CONFIG:
            status = session->setParameters(this, 0, param_id, payload);
            break;
        default:
            PAL_ERR(LOG_TAG, "Unsupported param id %u", param_id);
            status = -EINVAL;
            break;
    }

error:
    mStreamMutex.unlock();
    PAL_VERBOSE(LOG_TAG, "exit, session parameter %u set with status %d", param_id, status);
    return status;
}

int32_t StreamNonTunnel::drain(pal_drain_type_t type)
{
    PAL_ERR(LOG_TAG, "drain");
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
             || ssrInNTMode == true) {
         PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        return -ENETRESET;
    }
    return session->drain(type);
}

int32_t StreamNonTunnel::flush()
{
    int32_t status = 0;

    mStreamMutex.lock();
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
             || ssrInNTMode == true) {
         PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        status = -ENETRESET;
        goto exit;
    }

    status = session->flush();

exit:
    mStreamMutex.unlock();
    return status;
}

int32_t StreamNonTunnel::suspend()
{
    int32_t status = 0;

    mStreamMutex.lock();
    if ((PAL_CARD_STATUS_DOWN(rm->cardState))
             || ssrInNTMode) {
         PAL_ERR(LOG_TAG, "Sound card offline/standby currentState %d",
                currentState);
        status = -ENETRESET;
        goto exit;
    }

    if (currentState == STREAM_STARTED) {
        status = session->suspend(this);
        if (status) {
            PAL_ERR(LOG_TAG, "Rx session suspend failed with status %d", status);
            goto exit;
        }
        currentState = STREAM_SUSPENDED;
        palStateEnqueue(this, PAL_STATE_SUSPENDED, status);
    }
exit:
    mStreamMutex.unlock();
    return status;
}

int32_t StreamNonTunnel::isSampleRateSupported(uint32_t sampleRate)
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

int32_t StreamNonTunnel::isChannelSupported(uint32_t numChannels)
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

int32_t StreamNonTunnel::isBitWidthSupported(uint32_t bitWidth)
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

int32_t StreamNonTunnel::ssrDownHandler()
{
    int32_t status = 0;


    mStreamMutex.lock();
    /* In NonTunnelMode once SSR happens, that session is not reusuable
     * Hence set the ssr to true and return all subsequent calls with
     * -ENETRESET, untill the client sets up a new session.
     *
     */
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK currentState State %d",
            session, currentState);

    ssrInNTMode = true;
    if (streamCb)
        streamCb(reinterpret_cast<pal_stream_handle_t *>(this), PAL_STREAM_CBK_EVENT_ERROR, NULL, 0, this->cookie);

    mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamNonTunnel::ssrUpHandler()
{
    return 0;
}
