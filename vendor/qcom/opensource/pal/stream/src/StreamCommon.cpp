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
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: StreamCommon"
#define RXDIR 0
#define TXDIR 1

#include "StreamCommon.h"
#include "Session.h"
#include "kvh2xml.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include <unistd.h>

StreamCommon::StreamCommon(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
                    const uint32_t no_of_devices, const struct modifier_kv *modifiers,
                    const uint32_t no_of_modifiers, const std::shared_ptr<ResourceManager> rm)
{
    mStreamMutex.lock();
    uint32_t in_channels = 0, out_channels = 0;
    uint32_t attribute_size = 0;

    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Error:Sound card offline/standby, can not create stream");
        usleep(SSR_RECOVERY);
        mStreamMutex.unlock();
        throw std::runtime_error("Sound card offline/standby");
    }

    session = NULL;
    mGainLevel = -1;
    std::shared_ptr<Device> dev = nullptr;
    mStreamAttr = (struct pal_stream_attributes *)nullptr;
    mDevices.clear();
    currentState = STREAM_IDLE;
    //Modify cached values only at time of SSR down.
    cachedState = STREAM_IDLE;
    cookie_ = 0;
    bool isDeviceConfigUpdated = false;

    PAL_DBG(LOG_TAG, "Enter");

    //TBD handle modifiers later
    mNoOfModifiers = 0; //no_of_modifiers;
    mModifiers = (struct modifier_kv *) (NULL);
    std::ignore = modifiers;
    std::ignore = no_of_modifiers;

    if (!sattr) {
        PAL_ERR(LOG_TAG,"Error:invalid arguments");
        mStreamMutex.unlock();
        throw std::runtime_error("invalid arguments");
    }

    attribute_size = sizeof(struct pal_stream_attributes);
    mStreamAttr = (struct pal_stream_attributes *) calloc(1, attribute_size);
    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "Error:malloc for stream attributes failed %s", strerror(errno));
        mStreamMutex.unlock();
        throw std::runtime_error("failed to malloc for stream attributes");
    }

    memcpy(mStreamAttr, sattr, sizeof(pal_stream_attributes));

    if (mStreamAttr->in_media_config.ch_info.channels > PAL_MAX_CHANNELS_SUPPORTED) {
        PAL_ERR(LOG_TAG,"Error:in_channels is invalid %d", in_channels);
        mStreamAttr->in_media_config.ch_info.channels = PAL_MAX_CHANNELS_SUPPORTED;
    }
    if (mStreamAttr->out_media_config.ch_info.channels > PAL_MAX_CHANNELS_SUPPORTED) {
        PAL_ERR(LOG_TAG,"Error:out_channels is invalid %d", out_channels);
        mStreamAttr->out_media_config.ch_info.channels = PAL_MAX_CHANNELS_SUPPORTED;
    }

    PAL_VERBOSE(LOG_TAG, "Create new Session for stream type %d", sattr->type);
    session = Session::makeSession(rm, sattr);
    if (!session) {
        PAL_ERR(LOG_TAG, "Error:session creation failed");
        free(mStreamAttr);
        mStreamMutex.unlock();
        throw std::runtime_error("failed to create session object");
    }

    PAL_VERBOSE(LOG_TAG, "Create new Devices with no_of_devices - %d", no_of_devices);
    /* update handset/speaker sample rate for UPD with shared backend */
    if ((sattr->type == PAL_STREAM_ULTRASOUND ||
         sattr->type == PAL_STREAM_SENSOR_PCM_RENDERER) && !rm->IsDedicatedBEForUPDEnabled()) {
        struct pal_device devAttr = {};
        struct pal_device_info inDeviceInfo;
        pal_device_id_t upd_dev[] = {PAL_DEVICE_OUT_SPEAKER, PAL_DEVICE_OUT_HANDSET};
        for (int i = 0; i < sizeof(upd_dev)/sizeof(upd_dev[0]); i++) {
            devAttr.id = upd_dev[i];
            dev = Device::getInstance(&devAttr, rm);
            if (!dev)
                continue;
            rm->getDeviceInfo(devAttr.id, sattr->type, "", &inDeviceInfo);
            dev->setSampleRate(inDeviceInfo.samplerate);
            if (devAttr.id == PAL_DEVICE_OUT_HANDSET)
                dev->setBitWidth(inDeviceInfo.bit_width);
        }
    }
    for (int i = 0; i < no_of_devices; i++) {
        //Check with RM if the configuration given can work or not
        //for e.g., if incoming stream needs 24 bit device thats also
        //being used by another stream, then the other stream should route

        dev = Device::getInstance((struct pal_device *)&dattr[i] , rm);
        if (!dev) {
            PAL_ERR(LOG_TAG, "Error:Device creation failed");
            free(mStreamAttr);

            //TBD::free session too
            mStreamMutex.unlock();
            throw std::runtime_error("failed to create device object");
        }
        dev->insertStreamDeviceAttr(&dattr[i], this);
        mPalDevices.push_back(dev);
        mStreamMutex.unlock();
        // streams with VA MIC is handled in rm::handleConcurrentStreamSwitch()
        if (dattr[i].id != PAL_DEVICE_IN_HANDSET_VA_MIC &&
            dattr[i].id != PAL_DEVICE_IN_HEADSET_VA_MIC)
            isDeviceConfigUpdated = rm->updateDeviceConfig(&dev, &dattr[i], sattr);
        mStreamMutex.lock();

        if (isDeviceConfigUpdated)
            PAL_VERBOSE(LOG_TAG, "Device config updated");

        /* Create only update device attributes first time so update here using set*/
        /* this will have issues if same device is being currently used by different stream */
        mDevices.push_back(dev);
    }

    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);
    return;
}

StreamCommon::~StreamCommon()
{
    pal_stream_type_t type = PAL_STREAM_MAX;

    PAL_DBG(LOG_TAG, "Enter");
    cachedState = STREAM_IDLE;

    /* remove the device-stream attribute entry for the stopped stream */
    for (int32_t i=0; i < mPalDevices.size(); i++)
        mPalDevices[i]->removeStreamDeviceAttr(this);

    if (mStreamAttr) {
        type = mStreamAttr->type;
        free(mStreamAttr);
        mStreamAttr = (struct pal_stream_attributes *)NULL;
    }

    /* restore handset/speaker sample rate to default for UPD with shared backend */
    if ((type == PAL_STREAM_ULTRASOUND ||
         type == PAL_STREAM_SENSOR_PCM_RENDERER) && !rm->IsDedicatedBEForUPDEnabled()) {
        std::shared_ptr<Device> dev = nullptr;
        struct pal_device devAttr = {};
        pal_device_id_t upd_dev[] = {PAL_DEVICE_OUT_SPEAKER, PAL_DEVICE_OUT_HANDSET};
        for (int i = 0; i < sizeof(upd_dev)/sizeof(upd_dev[0]); i++) {
            devAttr.id = upd_dev[i];
            dev = Device::getInstance(&devAttr, rm);
            if (!dev)
                continue;
            dev->setSampleRate(0);
            dev->setBitWidth(0);
        }
    }

    /*switch back to proper config if there is a concurrency and device is still running*/
    for (int32_t i=0; i < mDevices.size(); i++)
        rm->restoreDevice(mDevices[i]);

    mDevices.clear();
    mPalDevices.clear();
    delete session;
    session = nullptr;
    PAL_DBG(LOG_TAG, "Exit");
}

int32_t  StreamCommon::open()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK device count - %zu", session,
            mDevices.size());

    mStreamMutex.lock();
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Error:Sound card offline/standby, can not open stream");
        usleep(SSR_RECOVERY);
        status = -EIO;
        goto exit;
    }

    if (currentState == STREAM_IDLE) {
        for (int32_t i = 0; i < mDevices.size(); i++) {
            status = mDevices[i]->open();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Error:device open failed with status %d", status);
                goto exit;
            }
        }

        status = session->open(this);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error:session open failed with status %d", status);
            goto closeDevice;
        }
        PAL_VERBOSE(LOG_TAG, "session open successful");
        currentState = STREAM_INIT;
        PAL_DBG(LOG_TAG, "streamLL opened. state %d", currentState);
        goto exit;
    } else if (currentState == STREAM_INIT) {
        PAL_INFO(LOG_TAG, "Stream is already opened, state %d", currentState);
        status = 0;
        goto exit;
    } else {
        PAL_ERR(LOG_TAG, "Error:Stream is not in correct state %d", currentState);
        //TBD : which error code to return here.
        status = -EINVAL;
        goto exit;
    }
closeDevice:
    for (int32_t i = 0; i < mDevices.size(); i++) {
        status = mDevices[i]->close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "device close is failed with status %d", status);
        }
    }
exit:
    palStateEnqueue(this, PAL_STATE_OPENED, status);
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit ret %d", status)
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t  StreamCommon::close()
{
    int32_t status = 0;
    mStreamMutex.lock();

    if (currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already closed");
        mStreamMutex.unlock();
        return status;
    }

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK device count - %zu stream_type - %d state %d",
             session, mDevices.size(), mStreamAttr->type, currentState);

    if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        mStreamMutex.unlock();
        status = stop();
        if (0 != status)
            PAL_ERR(LOG_TAG, "Error:stream stop failed. status %d",  status);
        mStreamMutex.lock();
    }

    rm->lockGraph();
    status = session->close(this);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:session close failed with status %d", status);
    }

    for (int32_t i = 0; i < mDevices.size(); i++) {
        status = mDevices[i]->close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error:device close is failed with status %d", status);
        }
    }
    PAL_VERBOSE(LOG_TAG, "closed the devices successfully");
    currentState = STREAM_IDLE;
    rm->unlockGraph();
    rm->checkAndSetDutyCycleParam();
    palStateEnqueue(this, PAL_STATE_CLOSED, status);
    mStreamMutex.unlock();

    PAL_DBG(LOG_TAG, "Exit. closed the stream successfully %d status %d",
             currentState, status);
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamCommon::start()
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
            session, mStreamAttr->direction, currentState);

    mStreamMutex.lock();
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_STARTED;
        PAL_ERR(LOG_TAG, "Error:Sound card offline/standby. Update the cached state %d",
                cachedState);
        goto exit;
    }

    if (currentState == STREAM_INIT || currentState == STREAM_STOPPED) {
        rm->lockGraph();
        status = start_device();
        if (0 != status) {
            rm->unlockGraph();
            goto exit;
        }
        PAL_VERBOSE(LOG_TAG, "device started successfully");
        status = startSession();
        if (0 != status) {
            rm->unlockGraph();
            goto exit;
        }
        rm->unlockGraph();
        PAL_VERBOSE(LOG_TAG, "session start successful");

        /*pcm_open and pcm_start done at once here,
         *so directly jump to STREAM_STARTED state.
         */
        currentState = STREAM_STARTED;
        mStreamMutex.unlock();
        rm->lockActiveStream();
        mStreamMutex.lock();
        for (int i = 0; i < mDevices.size(); i++) {
            rm->registerDevice(mDevices[i], this);
        }
        rm->unlockActiveStream();
        rm->checkAndSetDutyCycleParam();
    } else if (currentState == STREAM_STARTED) {
        PAL_INFO(LOG_TAG, "Stream already started, state %d", currentState);
    } else {
        PAL_ERR(LOG_TAG, "Error:Stream is not opened yet");
        status = -EINVAL;
    }
exit:
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);
    mStreamMutex.unlock();
    return status;
}

int32_t StreamCommon::start_device()
{
    int32_t status = 0;
    for (int32_t i=0; i < mDevices.size(); i++) {
         status = mDevices[i]->start();
         if (0 != status) {
             PAL_ERR(LOG_TAG, "Error:%s device start is failed with status %d",
                     GET_DIR_STR(mStreamAttr->direction), status);
         }
    }
    return status;
}

int32_t StreamCommon::startSession()
{
    int32_t status = 0, devStatus = 0;
    status = session->prepare(this);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%s session prepare is failed with status %d",
                    GET_DIR_STR(mStreamAttr->direction), status);
        goto session_fail;
    }
    PAL_VERBOSE(LOG_TAG, "session prepare successful");

    status = session->start(this);
    if (errno == -ENETRESET) {
        if (PAL_CARD_STATUS_UP(rm->cardState)) {
            PAL_ERR(LOG_TAG, "Error:Sound card offline/standby, informing RM");
            rm->ssrHandler(CARD_STATUS_OFFLINE);
        }
        cachedState = STREAM_STARTED;
        status = 0;
        goto session_fail;
    }
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Error:%s session start is failed with status %d",
                    GET_DIR_STR(mStreamAttr->direction), status);
        goto session_fail;
    }
    goto exit;

session_fail:
    for (int32_t i=0; i < mDevices.size(); i++) {
        devStatus = mDevices[i]->stop();
        if (devStatus)
            status = devStatus;
    }
exit:
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamCommon::stop()
{
    int32_t status = 0;

    mStreamMutex.lock();
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
                session, mStreamAttr->direction, currentState);

    if (currentState == STREAM_STARTED || currentState == STREAM_PAUSED) {
        mStreamMutex.unlock();
        rm->lockActiveStream();
        mStreamMutex.lock();
        currentState = STREAM_STOPPED;
        for (int i = 0; i < mDevices.size(); i++) {
            rm->deregisterDevice(mDevices[i], this);
        }
        rm->unlockActiveStream();
        PAL_VERBOSE(LOG_TAG, "In %s, device count - %zu",
                    GET_DIR_STR(mStreamAttr->direction), mDevices.size());

        rm->lockGraph();
        status = session->stop(this);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error:%s session stop failed with status %d",
                    GET_DIR_STR(mStreamAttr->direction), status);
        }
        PAL_VERBOSE(LOG_TAG, "session stop successful");
        for (int32_t i=0; i < mDevices.size(); i++) {
             status = mDevices[i]->stop();
             if (0 != status) {
                 PAL_ERR(LOG_TAG, "Error:%s device stop failed with status %d",
                         GET_DIR_STR(mStreamAttr->direction), status);
             }
        }
        rm->unlockGraph();
        PAL_VERBOSE(LOG_TAG, "devices stop successful");
    } else if (currentState == STREAM_STOPPED || currentState == STREAM_IDLE) {
        PAL_INFO(LOG_TAG, "Stream is already in Stopped state %d", currentState);
    } else {
        PAL_ERR(LOG_TAG, "Error:Stream should be in start/pause state, %d", currentState);
        status = -EINVAL;
    }
    palStateEnqueue(this, PAL_STATE_STOPPED, status);
    PAL_DBG(LOG_TAG, "Exit. status %d, state %d", status, currentState);

    mStreamMutex.unlock();
    return status;
}

int32_t StreamCommon::setVolume(struct pal_volume_data *volume)
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
    }

exit:
    if (volume) {
        PAL_DBG(LOG_TAG, "Exit. Volume payload No.of vol pair:%d ch mask:%x gain:%f",
                          (volume->no_of_volpair), (volume->volume_pair->channel_mask),
                          (volume->volume_pair->vol));
    }
    return status;
}

int32_t  StreamCommon::registerCallBack(pal_stream_callback cb, uint64_t cookie)
{
    callback_ = cb;
    cookie_ = cookie;

    PAL_VERBOSE(LOG_TAG, "callback_ = %pK", callback_);

    return 0;
}

int32_t StreamCommon::getTagsWithModuleInfo(size_t *size, uint8_t *payload)
{
    int32_t status = -EINVAL;

    PAL_DBG(LOG_TAG, "Enter");
    if (!payload) {
        PAL_ERR(LOG_TAG, "payload is NULL");
        goto exit;
    }

    if (session)
        status = session->getTagsWithModuleInfo(this, size, payload);
    else
        PAL_ERR(LOG_TAG, "session handle is NULL");

exit:
    return status;
}

int32_t StreamCommon::ssrDownHandler()
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

    switch (currentState) {
    case STREAM_INIT:
    case STREAM_STOPPED:
        mStreamMutex.unlock();
        status = close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error:stream close failed. status %d", status);
            goto exit;
        }
        break;
     case STREAM_STARTED:
     case STREAM_PAUSED:
        mStreamMutex.unlock();
        rm->unlockActiveStream();
        status = stop();
        rm->lockActiveStream();
        if (0 != status)
            PAL_ERR(LOG_TAG, "Error:stream stop failed. status %d",  status);
        status = close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error:stream close failed. status %d", status);
            goto exit;
        }
        break;
     default:
        PAL_ERR(LOG_TAG, "Error:stream state is %d, nothing to handle", currentState);
        mStreamMutex.unlock();
        goto exit;
    }

exit :
    currentState = STREAM_IDLE;
skip_down_handling :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamCommon::ssrUpHandler()
{
    int32_t status = 0;

    mStreamMutex.lock();
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK state %d",
            session, cachedState);

    if (skipSSRHandling) {
        skipSSRHandling = false;
        mStreamMutex.unlock();
        goto exit;
    }

    switch (cachedState) {
    case STREAM_INIT:
        mStreamMutex.unlock();
        status = open();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Error:stream open failed. status %d", status);
            goto exit;
        }
        break;
    case STREAM_STARTED:
    case STREAM_PAUSED:
    {
         mStreamMutex.unlock();
         status = open();
         if (0 != status) {
             PAL_ERR(LOG_TAG, "Error:stream open failed. status %d", status);
             goto exit;
         }
         rm->unlockActiveStream();
         status = start();
         rm->lockActiveStream();
         if (0 != status) {
             PAL_ERR(LOG_TAG, "Error:stream start failed. status %d", status);
             goto exit;
         }
         /* For scenario when we get SSR down while handling SSR up,
          * status will be 0, so we need to have this additonal check
          * to keep the cached state as STREAM_STARTED.
          */
         if (currentState != STREAM_STARTED) {
             goto exit;
         }
        }
        break;
     default:
        mStreamMutex.unlock();
        PAL_ERR(LOG_TAG, "Error:stream not in correct state to handle %d", cachedState);
        break;
    }
    cachedState = STREAM_IDLE;
exit :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}
