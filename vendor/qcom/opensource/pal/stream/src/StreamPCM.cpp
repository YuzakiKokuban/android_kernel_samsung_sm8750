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

#define LOG_TAG "PAL: StreamPCM"

#include "StreamPCM.h"
#include "Session.h"
#include "kvh2xml.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include <unistd.h>
#include <chrono>
#ifdef SEC_AUDIO_COMMON
#include "SecPalDefs.h"
#endif

StreamPCM::StreamPCM(const struct pal_stream_attributes *sattr, struct pal_device *dattr,
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
    std::shared_ptr<Device> dev = nullptr;
    mStreamAttr = (struct pal_stream_attributes *)nullptr;
    mDevices.clear();
    currentState = STREAM_IDLE;
    //Modify cached values only at time of SSR down.
    cachedState = STREAM_IDLE;
    bool isDeviceConfigUpdated = false;

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
    bool str_registered = false;
    for (int i = 0; i < no_of_devices; i++) {
        //Check with RM if the configuration given can work or not
        //for e.g., if incoming stream needs 24 bit device thats also
        //being used by another stream, then the other stream should route

        dev = Device::getInstance((struct pal_device *)&dattr[i] , rm);
        if (!dev) {
            PAL_ERR(LOG_TAG, "Device creation failed");
            free(mStreamAttr);

            //TBD::free session too
            mStreamMutex.unlock();
            throw std::runtime_error("failed to create device object");
        }
        dev->insertStreamDeviceAttr(&dattr[i], this);
        mPalDevices.push_back(dev);
        mStreamMutex.unlock();
        /* Stream mutex is unlocked before calling stream specific API
         * in resource manager to avoid deadlock issues between stream
         * and active stream mutex from ResourceManager.
         */
        if (!str_registered) {
            rm->registerStream(this);
            str_registered = true;
        }
        isDeviceConfigUpdated = rm->updateDeviceConfig(&dev, &dattr[i], sattr);
        mStreamMutex.lock();

        if (isDeviceConfigUpdated)
            PAL_VERBOSE(LOG_TAG, "Device config updated");
        if (dattr[i].id == PAL_DEVICE_IN_RECORD_PROXY) {
            ResourceManager::setProxyRecordActive(true);
        }

#ifdef SEC_AUDIO_SOUND_TRIGGER_TYPE
        if (mStreamAttr->direction == PAL_AUDIO_INPUT && mStreamAttr->type == PAL_STREAM_DEEP_BUFFER
            && !strcmp(dattr[i].custom_config.custom_key, ck_table[CUSTOM_KEY_VR_LOWPOWER])) {
            mIsHotwordRecord = true;
        }
#endif

        /* Create only update device attributes first time so update here using set*/
        /* this will have issues if same device is being currently used by different stream */
       // dev->setDeviceAttributes((struct pal_device)dattr[i]);
        mDevices.push_back(dev);
        dev = nullptr;
    }


    // Register for Soft pause events
    if (mStreamAttr->direction == PAL_AUDIO_OUTPUT )
        session->registerCallBack(handleSoftPauseCallBack, (uint64_t)this);

    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. state %d", currentState);
    return;
}

int32_t  StreamPCM::open()
{
    int32_t status = 0;
    int32_t ret = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK device count - %zu", session,
            mDevices.size());

    mStreamMutex.lock();
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_ERR(LOG_TAG, "Sound card offline/standby, can not open stream");
        usleep(SSR_RECOVERY);
        status = -EIO;
        goto exit;
    }

    /* Check for BT device connected state */
    for (int32_t i = 0; i < mDevices.size(); i++) {
        pal_device_id_t dev_id = (pal_device_id_t) mDevices[i]->getSndDeviceId();
        if (rm->isBtDevice(dev_id) && !(rm->isDeviceAvailable(dev_id))) {
            PAL_ERR(LOG_TAG, "BT device %d not connected, cannot open stream", dev_id);
            status = -ENODEV;
            goto exit;
        }
    }

    if (currentState == STREAM_IDLE) {
        bool checkDeviceCustomKeyForDualMono = false;
        bool setEffectParametersForDualMono = false;
        // enable dual mono
        if (rm->isDualMonoEnabled == true) {
            PAL_INFO(LOG_TAG, "Dual mono feature is on");
            if (mStreamAttr->type == PAL_STREAM_LOW_LATENCY) {
                PAL_INFO(LOG_TAG, "stream type is low-latency");
                checkDeviceCustomKeyForDualMono = true;
            }
        }

        for (int32_t i = 0; i < mDevices.size(); i++) {
            status = mDevices[i]->open();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "device open failed with status %d", status);
                goto exit;
            }

            if (checkDeviceCustomKeyForDualMono) {
                struct pal_device deviceAttribute;
                ret = mDevices[i]->getDeviceAttributes(&deviceAttribute, this);
                if (ret) {
                    PAL_ERR(LOG_TAG, "getDeviceAttributes failed with status %d", ret);
                }
                PAL_INFO(LOG_TAG, "device custom key=%s",
                            deviceAttribute.custom_config.custom_key);
                if (deviceAttribute.id == PAL_DEVICE_OUT_SPEAKER &&
                        !strncmp(deviceAttribute.custom_config.custom_key,
                            "speaker-safe", sizeof("speaker-safe"))) {
                    setEffectParametersForDualMono = true;
                }
            }
        }

        rm->lockGraph();
        status = session->open(this);
        rm->unlockGraph();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "session open failed with status %d", status);
            goto closeDevice;
        }
        PAL_VERBOSE(LOG_TAG, "session open successful");

        if (setEffectParametersForDualMono) {
            uint8_t* paramData = NULL;
            ret = PayloadBuilder::payloadDualMono(&paramData);
            if (ret) {
                PAL_ERR(LOG_TAG, "failed to create dual mono info");
            } else {
                pal_param_payload *pal_param = (pal_param_payload *)paramData;
                effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)pal_param->payload;
                ret = session->setEffectParameters(this, effectPayload);
                if (ret) {
                    PAL_ERR(LOG_TAG, "failed to set dual mono param.");
                } else {
                    PAL_INFO(LOG_TAG, "dual mono setparameter succeeded.");
                }
                free(paramData);
            }
        }
        currentState = STREAM_INIT;
        PAL_DBG(LOG_TAG, "stream pcm opened. state %d", currentState);
        goto exit;
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
int32_t  StreamPCM::close()
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
    } else if (currentState == STREAM_INIT || currentState == STREAM_STOPPED) {
        /* Special handling for aaudio usecase on A2DP/BLE/Speaker.
         * A2DP/BLE device starts even when stream is still in STREAM_INIT state,
         * hence stop A2DP/BLE/Speaker device to match device start&stop count.
         */
        for (int32_t i=0; i < mDevices.size(); i++) {
            if (((rm->isBtDevice((pal_device_id_t) mDevices[i]->getSndDeviceId())) ||
                 (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER)) && isMMap) {
                status = mDevices[i]->stop();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "BT A2DP/BLE device stop failed with status %d", status);
                }
            }
        }
    }

    rm->lockGraph();
    status = session->close(this);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session close failed with status %d", status);
    }

    for (int32_t i = 0; i < mDevices.size(); i++) {
        status = mDevices[i]->close();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "device close is failed with status %d", status);
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

StreamPCM::~StreamPCM()
{
    cachedState = STREAM_IDLE;
    ResourceManager::setProxyRecordActive(false);

    rm->resetStreamInstanceID(this);
    /* Stream mutex is not taken before calling stream specific API
     * in resource manager to avoid deadlock issues between stream
     * and active stream mutex from ResourceManager.
     */
    rm->deregisterStream(this);
    /* remove the device-stream attribute entry for the stopped stream */
    for (int32_t i=0; i < mPalDevices.size(); i++)
        mPalDevices[i]->removeStreamDeviceAttr(this);
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
    mPalDevices.clear();
    delete session;
    session = nullptr;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamPCM::start()
{
    int32_t status = 0, devStatus = 0, cachedStatus = 0;
    int32_t tmp = 0;
    bool a2dpSuspend = false;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK mStreamAttr->direction - %d state %d",
            session, mStreamAttr->direction, currentState);

    mStreamMutex.lock();
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
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

            // handle scenario where BT device is not ready
            if (ResourceManager::isDummyDevEnabled) {
                status = handleBTDeviceNotReadyToDummy(a2dpSuspend);
            } else {
                status = handleBTDeviceNotReady(a2dpSuspend);
            }
            if (0 != status)
                goto exit;

            rm->lockGraph();
            /* Any device start success will be treated as positive status.
             * This allows stream be played even if one of devices failed to start.
             */
            status = -EINVAL;
            if (!mDevices.size()) {
                PAL_ERR(LOG_TAG, "No Rx device available to start the usecase");
                rm->unlockGraph();
                goto exit;
            }

            for (int32_t i=0; i < mDevices.size(); i++) {
                if ((rm->isBtDevice((pal_device_id_t) mDevices[i]->getSndDeviceId())
                   || (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER))
                    && isMMap) {
                    PAL_DBG(LOG_TAG, "skip BT device start as it's done already");
                    status = 0;
                    continue;
                }

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

            if (a2dpSuspend) {
                PAL_DBG(LOG_TAG, "mute the stream on speaker");
                if (!a2dpMuted) {
                    mute_l(true);
                    a2dpMuted = true;
                }
            }
            break;

        case PAL_AUDIO_INPUT:
            PAL_VERBOSE(LOG_TAG, "Inside PAL_AUDIO_INPUT device count - %zu",
                        mDevices.size());

            rm->lockGraph();
            for (int32_t i=0; i < mDevices.size(); i++) {
                status = mDevices[i]->start();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Tx device start is failed with status %d",
                            status);
                    rm->unlockGraph();
                    goto exit;
                }
            }
            PAL_VERBOSE(LOG_TAG, "devices started successfully");
            status = session->prepare(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Tx session prepare is failed with status %d",
                        status);
                rm->unlockGraph();
                goto session_fail;
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
                rm->unlockGraph();
                goto session_fail;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Tx session start is failed with status %d",
                        status);
                rm->unlockGraph();
                goto session_fail;
            }
            rm->unlockGraph();
            PAL_VERBOSE(LOG_TAG, "session start successful");
            break;
        case PAL_AUDIO_OUTPUT | PAL_AUDIO_INPUT:
            PAL_VERBOSE(LOG_TAG, "Inside Loopback case device count - %zu",
                        mDevices.size());
            rm->lockGraph();
            // start output device
            for (int32_t i=0; i < mDevices.size(); i++)
            {
                int32_t dev_id = mDevices[i]->getSndDeviceId();
                if (dev_id <= PAL_DEVICE_OUT_MIN || dev_id >= PAL_DEVICE_OUT_MAX)
                    continue;
                status = mDevices[i]->start();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Rx device start is failed with status %d",
                            status);
                    rm->unlockGraph();
                    goto exit;
                }
            }
            PAL_VERBOSE(LOG_TAG, "output devices started successfully");
            // start input device
            for (int32_t i=0; i < mDevices.size(); i++) {
                int32_t dev_id = mDevices[i]->getSndDeviceId();
                if (dev_id <= PAL_DEVICE_IN_MIN || dev_id >= PAL_DEVICE_IN_MAX)
                    continue;
                status = mDevices[i]->start();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Tx device start is failed with status %d", status);

                    /* In case of a voice call, if OUT dev started successfully
                     * but IN dev failed to start then stop OUT dev before closing it
                     */
                    for (int32_t i = 0; i < mDevices.size(); i++) {
                      int32_t dev_id = mDevices[i]->getSndDeviceId();

                      if (dev_id <= PAL_DEVICE_OUT_MIN || dev_id >= PAL_DEVICE_OUT_MAX)
                          continue;

                      int32_t tempStatus = mDevices[i]->stop();
                      if (0 != tempStatus) {
                          PAL_ERR(LOG_TAG, "Rx device stop is failed with status %d",
                                                              tempStatus);
                          rm->unlockGraph();
                          goto exit;
                      }

                    }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
                    PAL_INFO(LOG_TAG, "RX devices stop successful");
#else
                    PAL_VERBOSE(LOG_TAG, "RX devices stop successful");
#endif
                    rm->unlockGraph();
                    goto exit;
                }
            }
            PAL_VERBOSE(LOG_TAG, "input devices started successfully");

            status = session->prepare(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "session prepare is failed with status %d", status);
                rm->unlockGraph();
                goto session_fail;
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
                rm->unlockGraph();
                goto session_fail;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "session start is failed with status %d", status);
                rm->unlockGraph();
                goto session_fail;
            }
            rm->unlockGraph();
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
    }
exit:
    palStateEnqueue(this, PAL_STATE_STARTED, status);
    PAL_DBG(LOG_TAG, "Exit. state %d, status %d", currentState, status);
    mStreamMutex.unlock();
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamPCM::stop()
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
        switch (mStreamAttr->direction) {
        case PAL_AUDIO_OUTPUT:
            PAL_VERBOSE(LOG_TAG, "In PAL_AUDIO_OUTPUT case, device count - %zu",
                        mDevices.size());

            rm->lockGraph();
            status = session->stop(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Rx session stop failed with status %d", status);
            }
            PAL_VERBOSE(LOG_TAG, "session stop successful");

            for (int32_t i=0; i < mDevices.size(); i++) {
                if ((rm->isBtDevice((pal_device_id_t) mDevices[i]->getSndDeviceId())||
                     (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER)) && isMMap) {
                    PAL_DBG(LOG_TAG, "skip BT A2DP/BLE/Speaker device stop, to be done in close/disconnect");
                    status = 0;
                    continue;
                }
                status = mDevices[i]->stop();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Rx device stop failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
            }
            rm->unlockGraph();
            PAL_VERBOSE(LOG_TAG, "devices stop successful");
            break;

        case PAL_AUDIO_INPUT:
            PAL_ERR(LOG_TAG, "In PAL_AUDIO_INPUT case, device count - %zu",
                        mDevices.size());

            rm->lockGraph();
            for (int32_t i=0; i < mDevices.size(); i++) {
                status = mDevices[i]->stop();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Tx device stop failed with status %d", status);
                }
            }
            PAL_VERBOSE(LOG_TAG, "devices stop successful");

            status = session->stop(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Tx session stop failed with status %d", status);
                rm->unlockGraph();
                goto exit;
            }
            rm->unlockGraph();
            PAL_VERBOSE(LOG_TAG, "session stop successful");
            break;

        case PAL_AUDIO_OUTPUT | PAL_AUDIO_INPUT:
            PAL_VERBOSE(LOG_TAG, "In LOOPBACK case, device count - %zu", mDevices.size());

            rm->lockGraph();
            for (int32_t i=0; i < mDevices.size(); i++) {
                int32_t dev_id = mDevices[i]->getSndDeviceId();
                if (dev_id <= PAL_DEVICE_IN_MIN || dev_id >= PAL_DEVICE_IN_MAX)
                    continue;
                status = mDevices[i]->stop();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Tx device stop is failed with status %d",
                            status);
                }
            }
            PAL_VERBOSE(LOG_TAG, "TX devices stop successful");
            status = session->stop(this);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "session stop is failed with status %d", status);
            }
            PAL_VERBOSE(LOG_TAG, "session stop successful");

            for (int32_t i=0; i < mDevices.size(); i++) {
                 int32_t dev_id = mDevices[i]->getSndDeviceId();
                 if (dev_id <= PAL_DEVICE_OUT_MIN || dev_id >= PAL_DEVICE_OUT_MAX)
                     continue;
                 status = mDevices[i]->stop();
                 if (0 != status) {
                     PAL_ERR(LOG_TAG, "Rx device stop is failed with status %d",
                             status);
                     rm->unlockGraph();
                     goto exit;
                }
            }
            rm->unlockGraph();
            PAL_VERBOSE(LOG_TAG, "RX devices stop successful");
            break;
        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Stream type is not supported with status %d", status);
            break;
        }
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
    PAL_DBG(LOG_TAG, "Exit. status %d, state %d", status, currentState);
    mStreamMutex.unlock();
    return status;
}

//TBD: move this to Stream, why duplicate code?
int32_t StreamPCM::prepare()
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
int32_t  StreamPCM::setStreamAttributes(struct pal_stream_attributes *sattr)
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

    PAL_DBG(LOG_TAG, "session setConfig successful");
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamPCM::setVolume(struct pal_volume_data *volume)
{
    int32_t status = 0;
    struct volume_set_param_info vol_set_param_info;
    uint8_t volSize = 0;
    bool forceSetParameters = false;

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
     * till the stream_start is not done or if sound card is offline.
     */
    ar_mem_cpy(mVolumeData, volSize, volume, volSize);
    for (int32_t i = 0; i < (mVolumeData->no_of_volpair); i++) {
        if ((i > 0) &&
            (abs(mVolumeData->volume_pair[0].vol -
                mVolumeData->volume_pair[i].vol) > VOLUME_TOLERANCE)) {
                forceSetParameters = true;
        }
        PAL_INFO(LOG_TAG, "Volume payload mask:%x vol:%f",
                      (mVolumeData->volume_pair[i].channel_mask), (mVolumeData->volume_pair[i].vol));
    }

    if (a2dpMuted) {
        PAL_DBG(LOG_TAG, "a2dp muted, just cache volume update");
        goto exit;
    }

    memset(&vol_set_param_info, 0, sizeof(struct volume_set_param_info));
    rm->getVolumeSetParamInfo(&vol_set_param_info);
    if ((rm->cardState == CARD_STATUS_ONLINE) && (currentState != STREAM_IDLE)
            && (currentState != STREAM_INIT) && (!isPaused)) {
        bool isStreamAvail = (find(vol_set_param_info.streams_.begin(),
                    vol_set_param_info.streams_.end(), mStreamAttr->type) !=
                    vol_set_param_info.streams_.end());
        if (!forceSetParameters && mVolumeData->volume_pair[0].vol == 0.0f &&
            !vol_set_param_info.isVolumeUsingSetParam) {
            //if the volume is 0, force settting parameters as well
            if (rm->isCRSCallEnabled) {
                status = session->setConfig(this, MODULE, CRS_CALL_VOLUME, RX_HOSTLESS);
            } else {
                status = session->setConfig(this, CALIBRATION, TAG_STREAM_VOLUME);
            }
            forceSetParameters = true;
        }
        if ((isStreamAvail && vol_set_param_info.isVolumeUsingSetParam) || forceSetParameters) {
            uint8_t *volPayload = new uint8_t[sizeof(pal_param_payload) + volSize]();
            pal_param_payload *pld = (pal_param_payload *)volPayload;
            pld->payload_size = sizeof(struct pal_volume_data);
            memcpy(pld->payload, mVolumeData, volSize);
            status = session->setParameters(this, TAG_STREAM_VOLUME,
                    PAL_PARAM_ID_VOLUME_USING_SET_PARAM, (void *)pld);
            delete[] volPayload;
            PAL_DBG(LOG_TAG, "set volume by parameter, status: %d", status);
        } else {
            if (rm->isCRSCallEnabled) {
                status = session->setConfig(this, MODULE, CRS_CALL_VOLUME, RX_HOSTLESS);
            } else {
                status = session->setConfig(this, CALIBRATION, TAG_STREAM_VOLUME);
            }
        }

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

int32_t  StreamPCM::read(struct pal_buffer* buf)
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
    PAL_DBG(LOG_TAG, "Exit. session read failed status %d", status);
    return status;
}

int32_t StreamPCM::write(struct pal_buffer* buf)
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
    // or when a softpause happens during a2dpsuspend, stream does not write data.
    if (PAL_CARD_STATUS_DOWN(rm->cardState) ||
        cachedState != STREAM_IDLE || a2dpPaused) {
        byteWidth = mStreamAttr->out_media_config.bit_width / 8;
        sampleRate = mStreamAttr->out_media_config.sample_rate;
        channelCount = mStreamAttr->out_media_config.ch_info.channels;

        frameSize = byteWidth * channelCount;
        if ((frameSize == 0) || (sampleRate == 0)) {
            PAL_ERR(LOG_TAG, "frameSize=%d, sampleRate=%d", frameSize, sampleRate);
            mStreamMutex.unlock();
            status = -EINVAL;
            goto exit;
        }
        size = buf->size;
        usleep((uint64_t)size * 1000000 / frameSize / sampleRate);
#ifdef SEC_ADD_FOR_DEBUG
        PAL_DBG(LOG_TAG, "cardState:%d stream cachedState:%d dropped buffer size - %d",
                            rm->cardState, cachedState, size);
#else
        PAL_DBG(LOG_TAG, "dropped buffer size - %d", size);
#endif
        mStreamMutex.unlock();
        PAL_VERBOSE(LOG_TAG, "Exit size: %d", size);
        return size;
    }

    // we should allow writes to go through in Start/Pause state as well.
    if ((currentState == STREAM_STARTED) ||
        (currentState == STREAM_PAUSED) ) {
#ifdef SEC_AUDIO_DUMP
        sec_pal_write_pcm(mStreamAttr, buf->buffer, buf->size, PCM_DUMP_PURE);
#endif
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
                goto exit;
            }
        } else if (currentState == STREAM_PAUSED && !isPaused) {
            rm->lockActiveStream();
            mStreamMutex.lock();
            for (int i = 0; i < mDevices.size(); i++) {
                rm->registerDevice(mDevices[i], this);
            }
            mStreamMutex.unlock();
            rm->unlockActiveStream();
            currentState = STREAM_STARTED;
            palStateEnqueue(this, PAL_STATE_STARTED, status);
        }
        PAL_VERBOSE(LOG_TAG, "Exit. session write successful size - %d", size);
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
    PAL_ERR(LOG_TAG, "Exit session write failed status %d", status);
    return status;
}

int32_t  StreamPCM::registerCallBack(pal_stream_callback /*cb*/, uint64_t /*cookie*/)
{
    return 0;
}

int32_t  StreamPCM::getCallBack(pal_stream_callback * /*cb*/)
{
    return 0;
}

int32_t StreamPCM::setDeviceMute(pal_stream_direction_t dir, bool state)
{
    int32_t status = 0;
    PAL_DBG(LOG_TAG, "Enter. ");

    if (dir == PAL_AUDIO_OUTPUT)
        deviceMuteStateRx = state;
    else
        deviceMuteStateTx = state;

    return status;
}

int32_t StreamPCM::getDeviceMute(pal_stream_direction_t dir, bool *state)
{
    int32_t status = 0;
    PAL_DBG(LOG_TAG, "Enter. ");

    if(!state)
    {
        PAL_ERR(LOG_TAG, "NULL volume pointer sent");
        status = -EINVAL;
        return status;
    }
    if (dir == PAL_AUDIO_OUTPUT)
        *state = deviceMuteStateRx;
    else
        *state = deviceMuteStateTx;

    return status;
}

int32_t StreamPCM::getParameters(uint32_t param_id, void ** payload)
{
    pal_param_payload *param_payload = nullptr;

#ifdef SEC_AUDIO_COMMON
    PAL_DBG(LOG_TAG, "Enter, set parameter %u, session handle - %p", param_id, session);
    int32_t status = 0;
    mStreamMutex.lock();
#else
    PAL_DBG(LOG_TAG, "Enter.");
#endif

    switch (param_id) {
        case PAL_PARAM_ID_DEVICE_MUTE:
        {
            param_payload = (pal_param_payload *)(*payload);
            pal_device_mute_t *deviceMutePayload = (pal_device_mute_t *) (param_payload + sizeof(pal_param_payload));
            getDeviceMute(deviceMutePayload->dir, &(deviceMutePayload->mute));
            break;
        }
#ifdef SEC_AUDIO_COMMON
        case PAL_PARAM_ID_UIEFFECT:
        {
            pal_param_payload *pal_param = (pal_param_payload *)payload;
            effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)pal_param->payload;
            pal_effect_custom_payload_t *effectCustomPayload = nullptr;
            effectCustomPayload = (pal_effect_custom_payload_t *)effectPayload->payload;

            int status = 0;
            PAL_INFO(LOG_TAG, "PAL_PARAM_ID_UIEFFECT tagId(0x%x) paramId(0x%x)",
                                effectPayload->tag, effectCustomPayload->paramId);
            if (effectPayload->tag == TAG_ECHOREF_NO_KEY) {
                if (((effectCustomPayload->paramId == PARAM_ID_ECHOREF_MUTE_DETECT_PARAM) && (mStreamAttr->type == PAL_STREAM_VOIP_TX))
                        || mStreamAttr->type == PAL_STREAM_VOICE_CALL) {
                    status = session->getParameters(this, effectPayload->tag, param_id, payload);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG, "getParameters(tagId 0x%x) Failed", effectPayload->tag);
                    }
                }
            }
            break;
        }
#endif
        default:
            PAL_INFO(LOG_TAG, "Not supported for param id %u", param_id);
            break;
    }

#ifdef SEC_AUDIO_COMMON
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "exit, session parameter %u set with status %d", param_id, status);
    return status;
#else
    return 0;
#endif
}

int32_t  StreamPCM::setParameters(uint32_t param_id, void *payload)
{
    int32_t status = 0;
    int32_t setConfigStatus = 0;
    pal_param_payload *param_payload = NULL;
    effect_pal_payload_t *effectPalPayload = nullptr;
    pal_device_mute_t *deviceMutePayload = nullptr;

    PAL_DBG(LOG_TAG, "Enter, set parameter %u, session handle - %p", param_id, session);

    if (!payload)
    {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "wrong params");
        goto exit;
    }

    mStreamMutex.lock();
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE for param ID: %d", param_id);
        mStreamMutex.unlock();
        return -EINVAL;
    }
    // Stream may not know about tags, so use setParameters instead of setConfig
    switch (param_id) {
        case PAL_PARAM_ID_TTY_MODE:
        {
            param_payload = (pal_param_payload *)payload;
            if (param_payload->payload_size > sizeof(uint32_t)) {
                PAL_ERR(LOG_TAG, "Invalid payload size %d", param_payload->payload_size);
                status = -EINVAL;
                break;
            }
            uint32_t tty_mode = *((uint32_t *)param_payload->payload);
            status = session->setParameters(this, TTY_MODE, param_id, payload);
            if (status)
               PAL_ERR(LOG_TAG, "setParam for tty mode %d failed with %d",
                       tty_mode, status);
            break;
        }
        case PAL_PARAM_ID_DEVICE_ROTATION:
        {
            // Call Session for Setting the parameter.
            if (NULL != session) {
                /* To avoid pop while switching channels, it is required to mute
                   the playback first and then swap the channel and unmute */
                if (mStreamAttr->type == PAL_STREAM_LOW_LATENCY ||
                    mStreamAttr->type == PAL_STREAM_ULTRA_LOW_LATENCY) {
                    setConfigStatus = session->setConfig(this, MODULE, MUTE_TAG);
                } else {
                    setConfigStatus = session->setConfig(this, MODULE, DEVICEPP_MUTE);
                }
                if (setConfigStatus) {
                    PAL_INFO(LOG_TAG, "DevicePP Mute failed");
                }
                mStreamMutex.unlock();
                usleep(MUTE_RAMP_PERIOD); // Wait for Mute ramp down to happen
                mStreamMutex.lock();
                status = session->setParameters(this, 0,
                                                PAL_PARAM_ID_DEVICE_ROTATION,
                                                payload);
                mStreamMutex.unlock();
                usleep(MUTE_RAMP_PERIOD); // Wait for channel swap to take affect
                mStreamMutex.lock();
                if (mStreamAttr->type == PAL_STREAM_LOW_LATENCY ||
                    mStreamAttr->type == PAL_STREAM_ULTRA_LOW_LATENCY) {
                    setConfigStatus = session->setConfig(this, MODULE, UNMUTE_TAG);
                } else {
                    setConfigStatus = session->setConfig(this, MODULE, DEVICEPP_UNMUTE);
                }
                if (setConfigStatus) {
                    PAL_INFO(LOG_TAG, "DevicePP Unmute failed");
                }
            } else {
                PAL_ERR(LOG_TAG, "Session is null");
                status = -EINVAL;
            }
        }
        break;
#ifdef SEC_AUDIO_CALL
        case PAL_PARAM_ID_MIC_MODE:
         {
             status = session->setParameters(this, VOICE_MIC_MODE, param_id, payload);
             if (status)
                PAL_ERR(LOG_TAG, "setParam for mic mode failed with %d",
                        status);
             break;
         }
#endif
        case PAL_PARAM_ID_VOLUME_BOOST:
        {
            status = session->setParameters(this, VOICE_VOLUME_BOOST, param_id, payload);
            if (status)
               PAL_ERR(LOG_TAG, "setParam for volume boost failed with %d",
                       status);
            break;
        }
        case PAL_PARAM_ID_SLOW_TALK:
        {
            bool slow_talk = false;
            param_payload = (pal_param_payload *)payload;
            slow_talk = *((bool *)param_payload->payload);

            uint32_t slow_talk_tag =
                          slow_talk ? VOICE_SLOW_TALK_ON : VOICE_SLOW_TALK_OFF;
            status = session->setParameters(this, slow_talk_tag,
                                            param_id, payload);
            if (status)
               PAL_ERR(LOG_TAG, "setParam for slow talk failed with %d",
                       status);
            break;
        }
        case PAL_PARAM_ID_DEVICE_MUTE:
        {
            param_payload = (pal_param_payload *)payload;
            deviceMutePayload = (pal_device_mute_t *)(param_payload->payload);
            status = session->setParameters(this, DEVICE_MUTE,
                                            param_id, payload);
            if (status) {
               PAL_ERR(LOG_TAG, "setParam for device mute failed with %d",
                       status);
            } else {
               setDeviceMute(deviceMutePayload->dir, deviceMutePayload->mute);
            }
            break;
        }
        default:
            PAL_ERR(LOG_TAG, "Unsupported param id %u", param_id);
            status = -EINVAL;
            break;
    }

    mStreamMutex.unlock();
exit:
    PAL_DBG(LOG_TAG, "exit, session parameter %u set with status %d", param_id, status);
    return status;
}

int32_t StreamPCM::mute_l(bool state)
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
    if (mute_by_volume &&
        mStreamAttr->direction == PAL_AUDIO_OUTPUT) {
        PAL_DBG(LOG_TAG, "Skip mute/unmute as stream muted by volume");
        unMutePending = !state;
        goto exit;
    }
    status = session->setConfig(this, MODULE, state ? MUTE_TAG : UNMUTE_TAG);

exit:
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "Exit status: %d", status);
#else
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
#endif
    return status;
}

int32_t StreamPCM::mute(bool state)
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = mute_l(state);
    mStreamMutex.unlock();

    return status;
}

int32_t StreamPCM::pause_l()
{
    int32_t status = 0;
    std::unique_lock<std::mutex> pauseLock(pauseMutex);
    struct pal_vol_ctrl_ramp_param ramp_param;
    struct pal_volume_data *volume = NULL;
    uint8_t volSize = 0;
    struct pal_volume_data *voldata = NULL;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_PAUSED;
        isPaused = true;
        PAL_ERR(LOG_TAG, "Sound Card Offline/standby, cached state %d", cachedState);
        goto exit;
    }

    if (isPaused) {
        PAL_INFO(LOG_TAG, "Stream is already paused");
    } else {
        status = session->setConfig(this, MODULE, PAUSE_TAG);
        if (0 != status) {
           PAL_ERR(LOG_TAG, "session setConfig for pause failed with status %d",
                    status);
           return status;
        }
        if (session->isPauseRegistrationDone) {
            PAL_DBG(LOG_TAG, "Waiting for Pause to complete from ADSP");
            pauseCV.wait_for(pauseLock, std::chrono::microseconds(VOLUME_RAMP_PERIOD));
        } else {
            PAL_DBG(LOG_TAG, "Pause event registration not done, sleeping for %d",
                    VOLUME_RAMP_PERIOD);
            usleep(VOLUME_RAMP_PERIOD);
        }
        palStateEnqueue(this, PAL_STATE_PAUSED, status);
        PAL_DBG(LOG_TAG, "session setConfig successful");

        //caching the volume before setting it to 0
        if (mVolumeData) {
            voldata = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                        (sizeof(struct pal_channel_vol_kv) *
                        (mVolumeData->no_of_volpair))));
        }
        if (!voldata) {
            status = -ENOMEM;
            goto exit;
        }

        status = this->getVolumeData(voldata);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getVolumeData Failed \n");
            goto exit;
        }
        /* set ramp period to 0 to make volume be changed to 0 instantly.
         * ramp down is already done in soft pause, ramp down twice both
         * in volume module and pause with non-0 period, the curve of
         * final ramp down becomes not smooth.
         */
        ramp_param.ramp_period_ms = 0;
        status = session->setParameters(this,
                                        TAG_STREAM_VOLUME,
                                        PAL_PARAM_ID_VOLUME_CTRL_RAMP,
                                        &ramp_param);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "setParam for vol ctrl failed, status %d", status);
            status = 0; //non-fatal
        }

        volSize = sizeof(uint32_t) + (sizeof(struct pal_channel_vol_kv) *
                                            (voldata->no_of_volpair));
        /* set volume to 0 to avoid the secerio of doing ramping up
         * from higher volume to lower volume in coming resume.
         */
        volume = (struct pal_volume_data *)calloc(1, volSize);
        if (!volume) {
            PAL_ERR(LOG_TAG, "Failed to allocate mem for volume");
            status = -ENOMEM;
            goto exit;
        }
        ar_mem_cpy(volume, volSize, voldata, volSize);
        for (int32_t i = 0; i < (voldata->no_of_volpair); i++) {
            volume->volume_pair[i].vol = 0x0;
        }
        setVolume(volume);
        if (mVolumeData) {
            free(mVolumeData);
            mVolumeData = NULL;
        }
        mVolumeData = (struct pal_volume_data *)calloc(1, volSize);
        if (!mVolumeData) {
            PAL_ERR(LOG_TAG, "failed to calloc for volume data");
            status = -ENOMEM;
            goto exit;
        }
        ar_mem_cpy(mVolumeData, volSize, voldata, volSize);

         /* set ramp period to default */
        ramp_param.ramp_period_ms = DEFAULT_RAMP_PERIOD;
        status = session->setParameters(this,
                                        TAG_STREAM_VOLUME,
                                        PAL_PARAM_ID_VOLUME_CTRL_RAMP,
                                        &ramp_param);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"setParam for vol ctrl failed, status %d", status);
            status = 0; //non-fatal
        }
    }
exit:
    isPaused = true;
    currentState = STREAM_PAUSED;
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    if (volume) {
         free(volume);
         volume = NULL;
    }
    if (voldata) {
         free(voldata);
         voldata = NULL;
    }
    return status;
}

int32_t StreamPCM::pause()
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = pause_l();
    mStreamMutex.unlock();

    return status;
}

int32_t StreamPCM::resume_l()
{
    int32_t status = 0;
    struct pal_vol_ctrl_ramp_param ramp_param;
    struct pal_volume_data *voldata = NULL;
    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        cachedState = STREAM_STARTED;
        PAL_ERR(LOG_TAG, "Sound Card offline/standby, cached state %d", cachedState);
        goto exit;
    }

    status = session->setConfig(this, MODULE, RESUME_TAG);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session setConfig for resume failed with status %d",
                status);
        goto exit;
    }

    if (mStreamAttr->direction == PAL_AUDIO_OUTPUT &&
        (mStreamAttr->type == PAL_STREAM_LOW_LATENCY ||
         mStreamAttr->type == PAL_STREAM_PCM_OFFLOAD ||
         mStreamAttr->type == PAL_STREAM_DEEP_BUFFER)) {
            pal_param_device_rotation_t rotation;
            rotation.rotation_type = rm->mOrientation == ORIENTATION_270 ?
                                    PAL_SPEAKER_ROTATION_RL : PAL_SPEAKER_ROTATION_LR;
            status = session->setParameters(this, 0, PAL_PARAM_ID_DEVICE_ROTATION, &rotation);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "session setParameters for rotation failed with status %d",
                        status);
            }
        }

    isPaused = false;

    //since we set the volume to 0 in pause, in resume we need to set vol back to default
    if (mVolumeData) {
        voldata = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                      (sizeof(struct pal_channel_vol_kv) * (mVolumeData->no_of_volpair))));
    }
    if (!voldata) {
        status = -ENOMEM;
        goto exit;
    }

    status = this->getVolumeData(voldata);
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getVolumeData Failed \n");
        goto exit;
    }

    setVolume(voldata);
    free(voldata);
    PAL_DBG(LOG_TAG, "session setConfig successful");
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int32_t StreamPCM::resume()
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = resume_l();
    mStreamMutex.unlock();

    return status;
}

int32_t StreamPCM::flush()
{
    int32_t status = 0;

    mStreamMutex.lock();
    if (isPaused == false) {
         PAL_ERR(LOG_TAG, "Error, flush called while stream is not Paused isPaused:%d", isPaused);
         goto exit;
    }

    if (mStreamAttr->type != PAL_STREAM_PCM_OFFLOAD) {
         PAL_VERBOSE(LOG_TAG, "flush called for non PCM OFFLOAD stream, ignore");
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

int32_t StreamPCM::isSampleRateSupported(uint32_t sampleRate)
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

int32_t StreamPCM::isChannelSupported(uint32_t numChannels)
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

int32_t StreamPCM::isBitWidthSupported(uint32_t bitWidth)
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

int32_t StreamPCM::addRemoveEffect(pal_audio_effect_t effect, bool enable)
{
    int32_t status = 0;
    int32_t tag = INVALID_TAG;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    mStreamMutex.lock();
    if (!enable) {
        if (PAL_AUDIO_EFFECT_ECNS == effect) {
           tag = ECNS_OFF_TAG;
        }
    } else {
        if (PAL_AUDIO_EFFECT_EC == effect) {
            tag = EC_ON_TAG;
        } else if (PAL_AUDIO_EFFECT_NS == effect) {
            tag = NS_ON_TAG;
        } else if (PAL_AUDIO_EFFECT_ECNS == effect) {
            tag = ECNS_ON_TAG;
        }
    }
    if (tag == INVALID_TAG) {
        PAL_ERR(LOG_TAG, "Invalid effect ID %d", effect);
        status = -EINVAL;
        goto exit;
    }
    status = session->setConfig(this, MODULE, tag);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "session setConfig for addRemoveEffect failed with status %d",
                status);
        goto exit;
    }
    PAL_DBG(LOG_TAG, "session setConfig successful");
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    mStreamMutex.unlock();
    return status;
}

int32_t StreamPCM::setECRef(std::shared_ptr<Device> dev, bool is_enable)
{
    int32_t status = 0;

    mStreamMutex.lock();
    status = setECRef_l(dev, is_enable);
    mStreamMutex.unlock();

    return status;
}

int32_t StreamPCM::setECRef_l(std::shared_ptr<Device> dev, bool is_enable)
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

int32_t StreamPCM::ssrDownHandler()
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

int32_t StreamPCM::ssrUpHandler()
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
    }
exit :
    cachedState = STREAM_IDLE;
skip_up_handling :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int32_t StreamPCM::createMmapBuffer(int32_t min_size_frames,
                                   struct pal_mmap_buffer *info)
{
    int32_t status = 0;
    bool btDevStarted = false;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);
    mStreamMutex.lock();
    if (currentState == STREAM_INIT) {
        rm->lockGraph();
        for (int32_t i=0; i < mDevices.size(); i++) {
            if (rm->isBtDevice((pal_device_id_t) mDevices[i]->getSndDeviceId()) ||
                (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER)) {
                PAL_DBG(LOG_TAG, "start BT devices as to populate the full GKVs");
                status = mDevices[i]->start();
                btDevStarted = !status;
                if (!btDevStarted && mDevices.size() == 1) {
                    PAL_ERR(LOG_TAG, "BT device start failed: %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
            }
        }
        status = session->createMmapBuffer(this, min_size_frames, info);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "createMmapBuffer failed with status = %d", status);
            for (int32_t i=0; i < mDevices.size(); i++) {
                if (rm->isBtDevice((pal_device_id_t) mDevices[i]->getSndDeviceId())) {
                    if (btDevStarted) {
                        int32_t tempStatus = mDevices[i]->stop();
                        if (0 != tempStatus)
                            PAL_ERR(LOG_TAG, "BT device stop failed: %d", tempStatus);
                    }
                }
            }
            rm->unlockGraph();
            goto exit;
        }
        isMMap = true;
        rm->unlockGraph();
    } else {
        status = -EINVAL;
    }

exit:
    mStreamMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit. status - %d", status);
    return status;
}

int32_t StreamPCM::GetMmapPosition(struct pal_mmap_position *position)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "Enter. session handle - %pK", session);

    status = session->GetMmapPosition(this, position);
    if (0 != status)
        PAL_ERR(LOG_TAG, "GetMmapPosition failed with status = %d", status);
    PAL_DBG(LOG_TAG, "Exit. status - %d", status);

    return status;
}
