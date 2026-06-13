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

#define LOG_TAG "PAL: Stream"
#include <semaphore.h>
#include "Stream.h"
#include "StreamPCM.h"
#include "StreamInCall.h"
#include "StreamCompress.h"
#include "StreamSoundTrigger.h"
#include "StreamACD.h"
#include "StreamASR.h"
#include "StreamContextProxy.h"
#include "StreamUltraSound.h"
#include "StreamSensorPCMData.h"
#include "StreamCommonProxy.h"
#include "StreamHaptics.h"
#include "StreamSensorRenderer.h"
#include "Session.h"
#include "SessionAlsaPcm.h"
#include "ResourceManager.h"
#include "Device.h"
#include "USBAudio.h"
#include "mem_logger.h"
#ifdef SEC_AUDIO_COMMON
#include "SecPalDefs.h"
#endif

std::shared_ptr<ResourceManager> Stream::rm = nullptr;
std::mutex Stream::mBaseStreamMutex;
std::mutex Stream::pauseMutex;
std::condition_variable Stream::pauseCV;


void Stream::handleSoftPauseCallBack(uint64_t hdl, uint32_t event_id,
                                        void *data __unused,
                                        uint32_t event_size __unused) {

    PAL_DBG(LOG_TAG,"Event id %x ", event_id);

    if (event_id == EVENT_ID_SOFT_PAUSE_PAUSE_COMPLETE) {
        PAL_DBG(LOG_TAG, "Pause done");
        pauseCV.notify_all();
    }
}

Stream* Stream::create(struct pal_stream_attributes *sAttr, struct pal_device *dAttr,
    uint32_t noOfDevices, struct modifier_kv *modifiers, uint32_t noOfModifiers)
{
    std::lock_guard<std::mutex> lock(mBaseStreamMutex);
    Stream* stream = NULL;
    int status = 0;
    uint32_t count = 0;
    struct pal_device *palDevsAttr = nullptr;
    std::vector <Stream *> streamsToSwitch;
    struct pal_device streamDevAttr;

    PAL_DBG(LOG_TAG, "Enter.");

    if (!sAttr || ((noOfDevices > 0) && !dAttr)) {
        PAL_ERR(LOG_TAG, "Invalid input paramters");
        goto exit;
    }

    /* get RM instance */
    if (!rm) {
        rm = ResourceManager::getInstance();
        if (!rm) {
            PAL_ERR(LOG_TAG, "ResourceManager getInstance failed");
            goto exit;
        }
    }
    PAL_VERBOSE(LOG_TAG,"get RM instance success and noOfDevices %d \n", noOfDevices);

    palDevsAttr = (pal_device *)calloc(noOfDevices, sizeof(struct pal_device));
    if (!palDevsAttr) {
        PAL_ERR(LOG_TAG, "palDevsAttr not created");
        goto exit;
    }

    if (sAttr->type == PAL_STREAM_NON_TUNNEL || sAttr->type == PAL_STREAM_CONTEXT_PROXY ||
        sAttr->type == PAL_STREAM_COMMON_PROXY) {
        goto stream_create;
    }

    if ((sAttr->type == PAL_STREAM_VOICE_CALL_MUSIC) ||
        (sAttr->type == PAL_STREAM_VOICE_CALL_RECORD))
        goto stream_create;

    if (sAttr->type == PAL_STREAM_SENSOR_PCM_DATA) {
        ar_mem_cpy(palDevsAttr, sizeof(struct pal_device),
                   dAttr, sizeof(struct pal_device));
        goto stream_create;
    }

    for (int i = 0; i < noOfDevices; i++) {
        struct pal_device_info devinfo = {};
        palDevsAttr[i] = {};

        if (sAttr->type == PAL_STREAM_ULTRASOUND ||
            sAttr->type == PAL_STREAM_SENSOR_PCM_RENDERER) {
            if (i == 0) { // first assign output device
                if (rm->IsVirtualPortForUPDEnabled())
                    dAttr[i].id = PAL_DEVICE_OUT_ULTRASOUND;
                else
                    dAttr[i].id = PAL_DEVICE_OUT_ULTRASOUND_DEDICATED;
            } else { // then assign input device
                dAttr[i].id = PAL_DEVICE_IN_ULTRASOUND_MIC;
            }
        }

        if (sAttr->type == PAL_STREAM_HAPTICS) {
            if (rm->IsHapticsThroughWSA()) {
                strlcpy(dAttr[i].custom_config.custom_key, "haptics-over-wsa", PAL_MAX_CUSTOM_KEY_SIZE);
            }
        }

        //TODO: shift this to rm or somewhere else where we can read the supported config from xml
        palDevsAttr[count].id = dAttr[i].id;
        if (palDevsAttr[count].id == PAL_DEVICE_OUT_USB_DEVICE ||
            palDevsAttr[count].id == PAL_DEVICE_OUT_USB_HEADSET ||
            palDevsAttr[count].id == PAL_DEVICE_IN_USB_DEVICE ||
            palDevsAttr[count].id == PAL_DEVICE_IN_USB_HEADSET ||
            rm->isBtDevice(palDevsAttr[count].id)) {
            palDevsAttr[count].address = dAttr[i].address;
        }
        PAL_VERBOSE(LOG_TAG, "count: %d, i: %d, length of dAttr custom_config: %d", count, i, strlen(dAttr[i].custom_config.custom_key));
        if (strlen(dAttr[i].custom_config.custom_key)) {
            strlcpy(palDevsAttr[count].custom_config.custom_key, dAttr[i].custom_config.custom_key, PAL_MAX_CUSTOM_KEY_SIZE);
            PAL_DBG(LOG_TAG, "found custom key %s", dAttr[i].custom_config.custom_key);

        } else {
            strlcpy(palDevsAttr[count].custom_config.custom_key, "", PAL_MAX_CUSTOM_KEY_SIZE);
            PAL_DBG(LOG_TAG, "no custom key found");
        }

        if (palDevsAttr[count].address.card_id != DUMMY_SND_CARD) {
            status = rm->getDeviceConfig((struct pal_device *)&palDevsAttr[count], sAttr);
            if (status) {
               PAL_ERR(LOG_TAG, "Not able to get Device config %d", status);
               goto exit;
            }
            // check if it's grouped device and group config needs to update
            rm->lockActiveStream();
            status = rm->checkAndUpdateGroupDevConfig((struct pal_device *)&palDevsAttr[count], sAttr,
                                                    streamsToSwitch, &streamDevAttr, true);
            rm->unlockActiveStream();
            if (status) {
                PAL_ERR(LOG_TAG, "no valid group device config found");
            }
        }

        count++;
    }

stream_create:
    PAL_DBG(LOG_TAG, "stream type 0x%x", sAttr->type);
    if (rm->isStreamSupported(sAttr, palDevsAttr, noOfDevices)) {
        try {
            switch (sAttr->type) {
                case PAL_STREAM_LOW_LATENCY:
                case PAL_STREAM_DEEP_BUFFER:
                case PAL_STREAM_SPATIAL_AUDIO:
                case PAL_STREAM_GENERIC:
                case PAL_STREAM_VOIP_TX:
                case PAL_STREAM_VOIP_RX:
                case PAL_STREAM_PCM_OFFLOAD:
                case PAL_STREAM_VOICE_CALL:
                case PAL_STREAM_LOOPBACK:
                case PAL_STREAM_ULTRA_LOW_LATENCY:
                case PAL_STREAM_PROXY:
                case PAL_STREAM_RAW:
                case PAL_STREAM_VOICE_RECOGNITION:
                    //TODO:for now keeping PAL_STREAM_PLAYBACK_GENERIC for ULLA need to check
                    stream = new StreamPCM(sAttr,
                                           palDevsAttr,
                                           noOfDevices,
                                           modifiers,
                                           noOfModifiers,
                                           rm);
                    break;
                case PAL_STREAM_COMPRESSED:
                    stream = new StreamCompress(sAttr,
                                                palDevsAttr,
                                                noOfDevices,
                                                modifiers,
                                                noOfModifiers,
                                                rm);
                    break;
                case PAL_STREAM_VOICE_UI:
                    stream = new StreamSoundTrigger(sAttr,
                                                    palDevsAttr,
                                                    noOfDevices,
                                                    modifiers,
                                                    noOfModifiers,
                                                    rm);
                    break;
                case PAL_STREAM_VOICE_CALL_RECORD:
                case PAL_STREAM_VOICE_CALL_MUSIC:
                    stream = new StreamInCall(sAttr,
                                              palDevsAttr,
                                              noOfDevices,
                                              modifiers,
                                              noOfModifiers,
                                              rm);
                    break;
                case PAL_STREAM_NON_TUNNEL:
                    stream = new StreamNonTunnel(sAttr,
                                                 NULL,
                                                 0,
                                                 modifiers,
                                                 noOfModifiers,
                                                 rm);
                    break;
                case PAL_STREAM_ACD:
                    stream = new StreamACD(sAttr,
                                           palDevsAttr,
                                           noOfDevices,
                                           modifiers,
                                           noOfModifiers,
                                           rm);
                    break;
                case PAL_STREAM_ASR:
                    stream = new StreamASR(sAttr,
                                           palDevsAttr,
                                           noOfDevices,
                                           modifiers,
                                           noOfModifiers,
                                           rm);
                    break;
                case PAL_STREAM_HAPTICS:
                    stream = new StreamHaptics(sAttr,
                                           palDevsAttr,
                                           noOfDevices,
                                           modifiers,
                                           noOfModifiers,
                                           rm);
                    break;
                case PAL_STREAM_CONTEXT_PROXY:
                    stream = new StreamContextProxy(sAttr,
                                                    NULL,
                                                    0,
                                                    modifiers,
                                                    noOfModifiers,
                                                    rm);
                    break;
                case PAL_STREAM_ULTRASOUND:
                    stream = new StreamUltraSound(sAttr,
                                                  palDevsAttr,
                                                  noOfDevices,
                                                  modifiers,
                                                  noOfModifiers,
                                                  rm);
                    break;
                case PAL_STREAM_SENSOR_PCM_DATA:
                    stream = new StreamSensorPCMData(sAttr,
                                                     palDevsAttr,
                                                     noOfDevices,
                                                     modifiers,
                                                     noOfModifiers,
                                                     rm);
                    break;
                case PAL_STREAM_COMMON_PROXY:
                    stream = new StreamCommonProxy(sAttr,
                                                   NULL,
                                                   0,
                                                   modifiers,
                                                   noOfModifiers,
                                                   rm);
                break;
                case PAL_STREAM_SENSOR_PCM_RENDERER:
                    stream = new StreamSensorRenderer(sAttr,
                                                  palDevsAttr,
                                                  noOfDevices,
                                                  modifiers,
                                                  noOfModifiers,
                                                  rm);
                    break;
                default:
                    PAL_ERR(LOG_TAG, "unsupported stream type 0x%x", sAttr->type);
                    break;
            }
        }
        catch (const std::exception& e) {
            PAL_ERR(LOG_TAG, "Stream create failed for stream type 0x%x", sAttr->type);
            if (palDevsAttr) {
                free(palDevsAttr);
            }
            throw std::runtime_error(e.what());
        }
    } else {
        PAL_ERR(LOG_TAG,"Requested config not supported");
        goto exit;
    }
exit:
    if (palDevsAttr) {
        free(palDevsAttr);
    }
    if (!stream) {
        PAL_ERR(LOG_TAG, "stream creation failed");
    }

    PAL_DBG(LOG_TAG, "Exit stream %pK create %s", stream,
            stream ? "successful" : "failed");
    return stream;
}

int32_t  Stream::getStreamAttributes(struct pal_stream_attributes *sAttr)
{
    int32_t status = 0;

    if (!sAttr || !mStreamAttr) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream attribute pointer, status %d", status);
        goto exit;
    }
    ar_mem_cpy(sAttr, sizeof(pal_stream_attributes), mStreamAttr,
                     sizeof(pal_stream_attributes));
    PAL_VERBOSE(LOG_TAG, "stream_type %d stream_flags %d direction %d",
            sAttr->type, sAttr->flags, sAttr->direction);

exit:
    return status;
}

const std::string& Stream::getStreamSelector() const {
    return mStreamSelector;
}

const std::string& Stream::getDevicePPSelector() const {
    return mDevPPSelector;
}

int32_t  Stream::getModifiers(struct modifier_kv *modifiers,uint32_t *noOfModifiers)
{
    int32_t status = 0;

    if (!mModifiers || !noOfModifiers) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid modifers pointer, status %d", status);
        goto exit;
    }
    ar_mem_cpy (modifiers, sizeof(modifier_kv), mModifiers,
                      sizeof(modifier_kv));
    *noOfModifiers = mNoOfModifiers;
    PAL_DBG(LOG_TAG, "noOfModifiers %u", *noOfModifiers);
exit:
    return status;
}

int32_t Stream::getEffectParameters(void *effect_query)
{
    int32_t status = 0;

    if (!effect_query) {
        PAL_ERR(LOG_TAG, "invalid query");
        return -EINVAL;
    }

    mStreamMutex.lock();
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE");
        mStreamMutex.unlock();
        return -EINVAL;
    }
    pal_param_payload *pal_param = (pal_param_payload *)effect_query;
    effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)pal_param->payload;
    status = session->getEffectParameters(this, effectPayload);
    if (status) {
       PAL_ERR(LOG_TAG, "getParameters failed with %d", status);
    }
    mStreamMutex.unlock();

    return status;
}

int32_t Stream::setEffectParameters(void *effect_param)
{
    int32_t status = 0;

    if (!effect_param) {
        PAL_ERR(LOG_TAG, "invalid query");
        return -EINVAL;
    }

    mStreamMutex.lock();
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE");
        mStreamMutex.unlock();
        return -EINVAL;
    }

    pal_param_payload *pal_param = (pal_param_payload *)effect_param;
    effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)pal_param->payload;
    status = session->setEffectParameters(this, effectPayload);
    if (status) {
       PAL_ERR(LOG_TAG, "setEffectParameters failed with %d", status);
    }

    mStreamMutex.unlock();

    return status;
}
int32_t Stream::rwACDBParameters(void *payload, uint32_t sampleRate,
                                    bool isParamWrite)
{
    int32_t status = 0;

    mStreamMutex.lock();
    if (currentState == STREAM_IDLE) {
        PAL_ERR(LOG_TAG, "Invalid stream state: IDLE");
        mStreamMutex.unlock();
        return -EINVAL;
    }
    status = session->rwACDBParameters(payload, sampleRate, isParamWrite);
    mStreamMutex.unlock();

    return status;
}

int32_t Stream::getStreamType (pal_stream_type_t* streamType)
{
    int32_t status = 0;

    if (!streamType || !mStreamAttr) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid stream type, status %d", status);
        goto exit;
    }
    *streamType = mStreamAttr->type;
    PAL_DBG(LOG_TAG, "streamType - %d", *streamType);

exit:
    return status;
}

int32_t Stream::getStreamDirection(pal_stream_direction_t *dir)
{
    int32_t status = 0;

    if (!dir || !mStreamAttr) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid inputs, status %d", status);
    } else {
        *dir = mStreamAttr->direction;
        PAL_DBG(LOG_TAG, "stream direction - %d", *dir);
    }
    return status;
}

uint32_t Stream::getRenderLatency()
{
    uint32_t delayMs = 0;

    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "invalid mStreamAttr");
        return delayMs;
    }

    switch (mStreamAttr->type) {
    case PAL_STREAM_DEEP_BUFFER:
        delayMs = PAL_DEEP_BUFFER_PLATFORM_DELAY / 1000;
        break;
    case PAL_STREAM_GENERIC:
        delayMs = PAL_GENERIC_PLATFORM_DELAY / 1000;
        break;
    case PAL_STREAM_LOW_LATENCY:
        delayMs = PAL_LOW_LATENCY_PLATFORM_DELAY / 1000;
        break;
    case PAL_STREAM_SPATIAL_AUDIO:
        delayMs = PAL_SPATIAL_AUDIO_PLATFORM_DELAY / 1000;
        break;
    case PAL_STREAM_COMPRESSED:
    case PAL_STREAM_PCM_OFFLOAD:
        delayMs = PAL_PCM_OFFLOAD_PLATFORM_DELAY / 1000;
        break;
    case PAL_STREAM_ULTRA_LOW_LATENCY:
        delayMs = PAL_ULL_PLATFORM_DELAY / 1000;
        break;
    case PAL_STREAM_VOIP_RX:
        delayMs = PAL_VOIP_PLATFORM_DELAY / 1000;
        break;
    default:
        break;
    }

    return delayMs;
}

uint32_t Stream::getLatency()
{
    uint32_t latencyMs = 0;

    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "invalid mStreamAttr");
        return latencyMs;
    }

    switch (mStreamAttr->type) {
    case PAL_STREAM_GENERIC:
        latencyMs = PAL_GENERIC_OUTPUT_PERIOD_DURATION *
            PAL_GENERIC_PLAYBACK_PERIOD_COUNT;
        break;
    case PAL_STREAM_DEEP_BUFFER:
        latencyMs = PAL_DEEP_BUFFER_OUTPUT_PERIOD_DURATION *
            PAL_DEEP_BUFFER_PLAYBACK_PERIOD_COUNT;
        break;
    case PAL_STREAM_LOW_LATENCY:
        latencyMs = PAL_LOW_LATENCY_OUTPUT_PERIOD_DURATION *
            PAL_LOW_LATENCY_PLAYBACK_PERIOD_COUNT;
        break;
    case PAL_STREAM_SPATIAL_AUDIO:
        latencyMs = PAL_SPATIAL_AUDIO_PERIOD_DURATION *
            PAL_SPATIAL_AUDIO_PLAYBACK_PERIOD_COUNT;
        break;
    case PAL_STREAM_COMPRESSED:
    case PAL_STREAM_PCM_OFFLOAD:
        latencyMs = PAL_PCM_OFFLOAD_OUTPUT_PERIOD_DURATION *
            PAL_PCM_OFFLOAD_PLAYBACK_PERIOD_COUNT;
        break;
    case PAL_STREAM_VOIP_RX:
        latencyMs = PAL_VOIP_OUTPUT_PERIOD_DURATION *
            PAL_VOIP_PLAYBACK_PERIOD_COUNT;
        break;
    case PAL_STREAM_ULTRA_LOW_LATENCY:
        latencyMs = PAL_ULL_OUTPUT_PERIOD_DURATION *
            PAL_ULL_PLAYBACK_PERIOD_COUNT;
        break;
    default:
        break;
    }

    latencyMs += getRenderLatency();
    return latencyMs;
}

int32_t Stream::getAssociatedDevices(std::vector <std::shared_ptr<Device>> &aDevices)
{
    int32_t status = 0;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_VERBOSE(LOG_TAG, "no. of devices %zu", mDevices.size());
#else
    PAL_DBG(LOG_TAG, "no. of devices %zu", mDevices.size());
#endif
    for (int32_t i=0; i < mDevices.size(); i++) {
        aDevices.push_back(mDevices[i]);
    }

    return status;
}

int32_t Stream::getAssociatedOutDevices(std::vector <std::shared_ptr<Device>> &aDevices)
{
    int32_t status = 0;

    for (int32_t i=0; i < mDevices.size(); i++) {
        if (rm->isOutputDevId(mDevices[i]->getSndDeviceId()))
            aDevices.push_back(mDevices[i]);
    }

    return status;
}

int32_t Stream::getAssociatedInDevices(std::vector <std::shared_ptr<Device>> &aDevices)
{
    int32_t status = 0;

    for (int32_t i=0; i < mDevices.size(); i++) {
        if (rm->isInputDevId(mDevices[i]->getSndDeviceId()))
            aDevices.push_back(mDevices[i]);
    }

    return status;
}

int32_t Stream::getPalDevices(std::vector <std::shared_ptr<Device>> &PalDevices)
{
    int32_t status = 0;

    PAL_DBG(LOG_TAG, "no. of devices %zu", mDevices.size());
    for (int32_t i=0; i < mPalDevices.size(); i++) {
        PalDevices.push_back(mPalDevices[i]);
    }

    return status;
}

void Stream::removePalDevice(Stream* streamHandle, int palDevId)
{
    std::vector <std::shared_ptr<Device>>::iterator dIter;
    int devId;

    for (dIter = mPalDevices.begin(); dIter != mPalDevices.end();) {
        devId = (*dIter)->getSndDeviceId();
        if (devId == palDevId) {
            (*dIter)->removeStreamDeviceAttr(streamHandle);
            dIter = mPalDevices.erase(dIter);
        } else {
            dIter++;
        }
    }
}

void Stream::clearOutPalDevices(Stream* streamHandle)
{
    std::vector <std::shared_ptr<Device>>::iterator dIter;
    int devId;

    for (dIter = mPalDevices.begin(); dIter != mPalDevices.end();) {
        devId = (*dIter)->getSndDeviceId();
        if (!rm->isInputDevId(devId)) {
            (*dIter)->removeStreamDeviceAttr(streamHandle);
            dIter = mPalDevices.erase(dIter);
        } else {
            dIter++;
        }
    }
}

void Stream::addPalDevice(Stream* streamHandle, struct pal_device *dattr)
{
    std::shared_ptr<Device> dev = nullptr;

    dev = Device::getInstance(dattr, rm);
    if (!dev) {
        PAL_ERR(LOG_TAG, "No device instance found");
        return;
    }
    dev->insertStreamDeviceAttr(dattr, streamHandle);
    mPalDevices.push_back(dev);
}

int32_t Stream::getSoundCardId()
{
    struct pal_device devAttr;
    if (mDevices.size()) {
        mDevices[0]->getDeviceAttributes(&devAttr);
        PAL_DBG(LOG_TAG, "sound card id = 0x%x",
                    devAttr.address.card_id);
        return devAttr.address.card_id;
    }

    return -EINVAL;
}

int32_t Stream::getAssociatedSession(Session **s)
{
    int32_t status = 0;

    if (!s) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid session, status %d", status);
        goto exit;
    }
    *s = session;
    PAL_DBG(LOG_TAG, "session %pK", *s);
exit:
    return status;
}

int32_t Stream::getVolumeData(struct pal_volume_data *vData)
{
    int32_t status = 0;

    if (!vData) {
        status = -EINVAL;
        PAL_INFO(LOG_TAG, "Volume Data not set yet");
        goto exit;
    }

    if (mVolumeData != NULL) {
        ar_mem_cpy(vData, sizeof(uint32_t) +
                      (sizeof(struct pal_channel_vol_kv) * (mVolumeData->no_of_volpair)),
                      mVolumeData, sizeof(uint32_t) +
                      (sizeof(struct pal_channel_vol_kv) * (mVolumeData->no_of_volpair)));

        PAL_DBG(LOG_TAG, "num config %x", (mVolumeData->no_of_volpair));
        for(int32_t i=0; i < (mVolumeData->no_of_volpair); i++) {
            PAL_VERBOSE(LOG_TAG,"Volume payload mask:%x vol:%f",
                (mVolumeData->volume_pair[i].channel_mask), (mVolumeData->volume_pair[i].vol));
        }
    } else {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "volume has not been set, status %d", status);
    }
exit:
    return status;
}

#ifdef SEC_AUDIO_CALL_VOIP
int32_t Stream::getMicMode(struct pal_param_voip_mic_mode *vData)
{
    int32_t status = 0;
    PAL_INFO(LOG_TAG, "enter");

    if (!vData) {
        status = -EINVAL;
        PAL_INFO(LOG_TAG, "Mic Mode not set yet");
        goto exit;
    }

    if (mMicMode != NULL) {
        ar_mem_cpy(vData, sizeof(int32_t), mMicMode, sizeof(int32_t));
        PAL_DBG(LOG_TAG, "VoIP Mic Mode %x", (mMicMode->mic_mode));
    } else {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Micmode has not been set, status %d", status);
    }
exit:
    return status;
}
#endif

int32_t Stream::setBufInfo(pal_buffer_config *in_buffer_cfg,
                           pal_buffer_config *out_buffer_cfg)
{
    int32_t status = 0;
    struct pal_stream_attributes sattr;
    int16_t nBlockAlignIn, nBlockAlignOut ;        // block size of data

    status = getStreamAttributes(&sattr);
    /*In case of extern mem mode PAL wont get any buffer size info from clients
      If clients still call this api then it could be to notify the metadata size*/
    if (sattr.flags & PAL_STREAM_FLAG_EXTERN_MEM) {
        if (out_buffer_cfg)
            outMaxMetadataSz = out_buffer_cfg->max_metadata_size;
        if (in_buffer_cfg)
            inMaxMetadataSz = in_buffer_cfg->max_metadata_size;
         /*in EXTERN_MEM mode set buf count and size to 0*/
         inBufCount = 0;
         inBufSize = 0;
         outBufCount = 0;
         outBufSize = 0;
    } else {
        if (in_buffer_cfg) {
            PAL_DBG(LOG_TAG, "In Buffer size %zu, In Buffer count %zu metadata sz %zu",
                    in_buffer_cfg->buf_size, in_buffer_cfg->buf_count, in_buffer_cfg->max_metadata_size);
            inBufCount = in_buffer_cfg->buf_count;
            inMaxMetadataSz = in_buffer_cfg->max_metadata_size;
        }
        if (out_buffer_cfg) {
            PAL_DBG(LOG_TAG, "Out Buffer size %zu and Out Buffer count %zu metaData sz %zu",
                    out_buffer_cfg->buf_size, out_buffer_cfg->buf_count, out_buffer_cfg->max_metadata_size);
            outBufCount = out_buffer_cfg->buf_count;
            outMaxMetadataSz = out_buffer_cfg->max_metadata_size;
        }

        if (sattr.direction == PAL_AUDIO_OUTPUT) {
            if(!out_buffer_cfg) {
               status = -EINVAL;
               PAL_ERR(LOG_TAG, "Invalid output buffer size status %d", status);
               goto exit;
            }
            outBufSize = out_buffer_cfg->buf_size;
            nBlockAlignOut = ((sattr.out_media_config.bit_width) / 8) *
                          (sattr.out_media_config.ch_info.channels);
            PAL_DBG(LOG_TAG, "no of buf %zu and send buf %zu", outBufCount, outBufSize);

            //If the read size is not a multiple of BlockAlign;
            //Make sure we read blockaligned bytes from the file.
            if ((outBufSize % nBlockAlignOut) != 0) {
                outBufSize = ((outBufSize / nBlockAlignOut) * nBlockAlignOut);
            }
            out_buffer_cfg->buf_size = outBufSize;

        } else if (sattr.direction == PAL_AUDIO_INPUT) {
            if (!in_buffer_cfg) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Invalid input buffer size status %d", status);
                goto exit;
            }
            inBufSize = in_buffer_cfg->buf_size;
            //inBufSize = (sattr->in_media_config.bit_width) * (sattr->in_media_config.ch_info.channels) * 32;
            nBlockAlignIn = ((sattr.in_media_config.bit_width) / 8) *
                          (sattr.in_media_config.ch_info.channels);
            //If the read size is not a multiple of BlockAlign;
            //Make sure we read blockaligned bytes from the file.
            if ((inBufSize % nBlockAlignIn) != 0) {
                inBufSize = ((inBufSize / nBlockAlignIn) * nBlockAlignIn);
            }
            in_buffer_cfg->buf_size = inBufSize;
        } else {
            if (!in_buffer_cfg || !out_buffer_cfg) {
                status = -EINVAL;
                PAL_ERR(LOG_TAG, "Invalid buffer size status %d", status);
                goto exit;
            }
            outBufSize = out_buffer_cfg->buf_size;
            nBlockAlignOut = ((sattr.out_media_config.bit_width) / 8) *
                          (sattr.out_media_config.ch_info.channels);
            PAL_DBG(LOG_TAG, "no of buf %zu and send buf %zu", outBufCount, outBufSize);

            //If the read size is not a multiple of BlockAlign;
            //Make sure we read blockaligned bytes from the file.
            if ((outBufSize % nBlockAlignOut) != 0) {
                outBufSize = ((outBufSize / nBlockAlignOut) * nBlockAlignOut);
            }
            out_buffer_cfg->buf_size = outBufSize;

            inBufSize = in_buffer_cfg->buf_size;
            nBlockAlignIn = ((sattr.in_media_config.bit_width) / 8) *
                          (sattr.in_media_config.ch_info.channels);
            //If the read size is not a multiple of BlockAlign;
            //Make sure we read blockaligned bytes from the file.
            if ((inBufSize % nBlockAlignIn) != 0) {
                inBufSize = ((inBufSize / nBlockAlignIn) * nBlockAlignIn);
            }
            in_buffer_cfg->buf_size = inBufSize;
        }
    }
exit:
    return status;
}

int32_t Stream::getMaxMetadataSz(size_t *in_max_metdata_sz, size_t *out_max_metadata_sz)
{
    int32_t status = 0;

    if (in_max_metdata_sz)
        *in_max_metdata_sz = inMaxMetadataSz;
    if (out_max_metadata_sz)
        *out_max_metadata_sz = outMaxMetadataSz;

    if (in_max_metdata_sz)
        PAL_DBG(LOG_TAG, "in max metadataSize %zu",
                *in_max_metdata_sz);

    if (out_max_metadata_sz)
        PAL_DBG(LOG_TAG, "in max metadataSize %zu",
                *out_max_metadata_sz);

    return status;
}

int32_t Stream::getBufInfo(size_t *in_buf_size, size_t *in_buf_count,
                           size_t *out_buf_size, size_t *out_buf_count)
{
    int32_t status = 0;

    if (in_buf_size)
        *in_buf_size = inBufSize;
    if (in_buf_count)
        *in_buf_count = inBufCount;
    if (out_buf_size)
        *out_buf_size = outBufSize;
    if (out_buf_count)
        *out_buf_count = outBufCount;

    if (in_buf_size && in_buf_count)
        PAL_DBG(LOG_TAG, "In Buffer size %zu, In Buffer count %zu",
                *in_buf_size, *in_buf_count);

    if (out_buf_size && out_buf_count)
        PAL_DBG(LOG_TAG, "Out Buffer size %zu and Out Buffer count %zu",
                *out_buf_size, *out_buf_count);

    return status;
}

int32_t Stream::getBufSize(size_t *in_buf_size, size_t *out_buf_size)
{
    int32_t status = 0;
    struct pal_stream_attributes *sattr = NULL;
    sattr = (struct pal_stream_attributes *)calloc(1, sizeof(struct pal_stream_attributes));
    if (!sattr) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "stream attribute malloc failed %s, status %d", strerror(errno), status);
        goto exit;
    }

    if (!in_buf_size)
        PAL_DBG(LOG_TAG, "Invalid In Buffer size");

    if (!out_buf_size)
        PAL_DBG(LOG_TAG, "Invalid Out Buffer size");

    status = getStreamAttributes(sattr);
    if (sattr->direction == PAL_AUDIO_OUTPUT) {
        if(!out_buf_size) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid output buffer size status %d", status);
            goto exit;
        }
        switch (sattr->type) {
            case PAL_STREAM_DEEP_BUFFER:
            case PAL_STREAM_PCM_OFFLOAD:
                *out_buf_size = ((sattr->out_media_config.bit_width) / 8) *
                                (sattr->out_media_config.sample_rate) *
                                (sattr->out_media_config.ch_info.channels);

                *out_buf_size = *out_buf_size / 1000;
                *out_buf_size = *out_buf_size * DEEP_BUFFER_OUTPUT_PERIOD_DURATION;
                break;
            case PAL_STREAM_COMPRESSED:
                *out_buf_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE;
                break;
            default:
                PAL_ERR(LOG_TAG, "unsupported stream type 0x%x", sattr->type);
                break;
        }
        PAL_DBG(LOG_TAG, "out_buf_size %zu", *out_buf_size);
    } else if (sattr->direction == PAL_AUDIO_INPUT) {
        if(!in_buf_size) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Invalid input buffer size status %d", status);
            goto exit;
        }

        switch (sattr->type) {
            case PAL_STREAM_DEEP_BUFFER:
            case PAL_STREAM_PCM_OFFLOAD:
                *in_buf_size = ((sattr->out_media_config.bit_width) / 8) *
                                (sattr->out_media_config.sample_rate) *
                                (sattr->out_media_config.ch_info.channels);

                *in_buf_size = *in_buf_size / 1000;
                *in_buf_size = *in_buf_size * AUDIO_CAPTURE_PERIOD_DURATION_MSEC;
                break;
            case PAL_STREAM_COMPRESSED:
                *in_buf_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE;
                break;
            default:
                PAL_ERR(LOG_TAG, "unsupported stream type 0x%x", sattr->type);
                break;
        }
        PAL_DBG(LOG_TAG, "in_buf_size %zu", *in_buf_size);
    }

exit:
    return status;
}

bool Stream::isStreamAudioOutFmtSupported(pal_audio_fmt_t format)
{
    switch (format) {
    case PAL_AUDIO_FMT_PCM_S8:
    case PAL_AUDIO_FMT_PCM_S16_LE:
    case PAL_AUDIO_FMT_PCM_S24_3LE:
    case PAL_AUDIO_FMT_PCM_S24_LE:
    case PAL_AUDIO_FMT_PCM_S32_LE:
    case PAL_AUDIO_FMT_MP3:
    case PAL_AUDIO_FMT_AAC:
    case PAL_AUDIO_FMT_AAC_ADTS:
    case PAL_AUDIO_FMT_AAC_ADIF:
    case PAL_AUDIO_FMT_AAC_LATM:
    case PAL_AUDIO_FMT_WMA_STD:
    case PAL_AUDIO_FMT_ALAC:
    case PAL_AUDIO_FMT_APE:
    case PAL_AUDIO_FMT_WMA_PRO:
    case PAL_AUDIO_FMT_FLAC:
    case PAL_AUDIO_FMT_VORBIS:
    case PAL_AUDIO_FMT_AMR_NB:
    case PAL_AUDIO_FMT_AMR_WB:
    case PAL_AUDIO_FMT_AMR_WB_PLUS:
    case PAL_AUDIO_FMT_EVRC:
    case PAL_AUDIO_FMT_G711:
    case PAL_AUDIO_FMT_QCELP:
    case PAL_AUDIO_FMT_OPUS:
        return true;
    default:
        return false;
    }
}

int32_t Stream::getTimestamp(struct pal_session_time *stime)
{
    int32_t status = 0;
    if (!stime) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid session time pointer, status %d", status);
        goto exit;
    }
    if (PAL_CARD_STATUS_DOWN(rm->cardState)) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Sound card offline/standby, status %d", status);
        goto exit;
    }
    rm->lockResourceManagerMutex();
    status = session->getTimestamp(stime);
    rm->unlockResourceManagerMutex();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Failed to get session timestamp status %d", status);
        if (errno == -ENETRESET &&
            (PAL_CARD_STATUS_UP(rm->cardState))) {
            PAL_ERR(LOG_TAG, "Sound card offline/standby, add stream to SSR");
            rm->ssrHandler(CARD_STATUS_OFFLINE);
            status = -EINVAL;
        }
    }
exit:
    return status;
}

int32_t Stream::handleBTDeviceNotReadyToDummy(bool& a2dpSuspend)
{
    int32_t status = 0;
    int sndDevId = 0;
    struct pal_device dattr = {};
    struct pal_device switchDevDattr = {};
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<Device> switchDev = nullptr;
    pal_param_bta2dp_t *param_bt_a2dp = nullptr;
    std::vector <Stream *> activeStreams;

    a2dpSuspend = false;

    /* SCO device is not ready */
    if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_SCO) &&
        !rm->isDeviceReady(PAL_DEVICE_OUT_BLUETOOTH_SCO)) {
        // If it's sco + speaker combo device, route to speaker.
        // Otherwise, return -EAGAIN.
        if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_SPEAKER)) {
            PAL_INFO(LOG_TAG, "BT SCO output device is not ready, route to speaker");
            for (auto iter = mDevices.begin(); iter != mDevices.end();) {
                if ((*iter)->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER) {
                    iter++;
                    continue;
                }

                // Invoke session API to explicitly update the device metadata
                rm->lockGraph();
                status = session->disconnectSessionDevice(this, mStreamAttr->type, (*iter));
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", status);
                    rm->unlockGraph();
                    goto exit;
                }

                status = (*iter)->close();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "device close failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
                iter = mDevices.erase(iter);
                removePalDevice(this, PAL_DEVICE_OUT_BLUETOOTH_SCO);
                rm->unlockGraph();
            }
        } else {
            PAL_ERR(LOG_TAG, "BT SCO output device is not ready");
            status = -EAGAIN;
            goto exit;
        }
    }

    /* A2DP/BLE device is not ready */
    if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_A2DP) ||
        rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_BLE) ||
        rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)) {
        if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {
            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
        } else if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)){
            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST;
        } else {
            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE;
        }
        dev = Device::getInstance(&dattr, rm);
        if (!dev) {
            status = -ENODEV;
            PAL_ERR(LOG_TAG, "failed to get a2dp/ble device object");
            goto exit;
        }
        dev->getDeviceParameter(PAL_PARAM_ID_BT_A2DP_SUSPENDED,
                        (void **)&param_bt_a2dp);
        if (param_bt_a2dp->a2dp_suspended == false) {
            PAL_DBG(LOG_TAG, "BT A2DP/BLE output device is good to go");
            goto exit;
        }

        PAL_INFO(LOG_TAG, "BT A2DP/BLE output device is not ready");

        suspendedOutDevIds.clear();
        suspendedOutDevIds.push_back(dattr.id);

        for (auto iter = mDevices.begin(); iter != mDevices.end();) {
            sndDevId = (*iter)->getSndDeviceId();
            if (sndDevId == dattr.id) {
                rm->lockGraph();
                status = session->disconnectSessionDevice(this, mStreamAttr->type, (*iter));
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", status);
                    rm->unlockGraph();
                    goto exit;
                }

                /* Special handling for aaudio usecase on A2DP/BLE.
                * A2DP/BLE device starts even when stream is not in START state,
                * hence stop A2DP/BLE device to match device start&stop count.
                */
                if (isMMap) {
                    status = (*iter)->stop();
                    if (0 != status) {
                        PAL_ERR(LOG_TAG, "BT A2DP/BLE device stop failed with status %d", status);
                    }
                }

                status = (*iter)->close();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "device close failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
                iter = mDevices.erase(iter);
                removePalDevice(this, sndDevId);
                rm->unlockGraph();
            } else {
                iter++;
            }
        }

        if (mDevices.size() == 0) {
            a2dpSuspend = true;
            switchDevDattr.id = PAL_DEVICE_OUT_DUMMY;
            switchDev = Device::getInstance(&switchDevDattr, rm);
            if (!switchDev) {
                status = -ENODEV;
                PAL_ERR(LOG_TAG, "Failed to get out dummy device instance");
                goto exit;
            }

            status = rm->getDeviceConfig(&switchDevDattr, NULL);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to get device config");
                goto exit;
            }

            switchDev->setDeviceAttributes(switchDevDattr);
            status = switchDev->open();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "device open failed with status %d", status);
                goto exit;
            }

            status = session->setupSessionDevice(this, mStreamAttr->type, switchDev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "setupSessionDevice failed:%d", status);
                switchDev->close();
                goto exit;
            }

            status = session->connectSessionDevice(this, mStreamAttr->type, switchDev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "connectSessionDevice failed:%d", status);
                switchDev->close();
                goto exit;
            }
            mDevices.push_back(switchDev);
            addPalDevice(this, &switchDevDattr);
        } else {
            suspendedOutDevIds.push_back(switchDevDattr.id);
        }
    }

exit:
    return status;
}

int32_t Stream::handleBTDeviceNotReady(bool& a2dpSuspend)
{
    int32_t status = 0;
    struct pal_device dattr = {};
    struct pal_device spkrDattr = {};
    struct pal_device handsetDattr = {};
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<Device> spkrDev = nullptr;
    std::shared_ptr<Device> handsetDev = nullptr;
    pal_param_bta2dp_t *param_bt_a2dp = nullptr;
    std::vector <Stream *> activeStreams;

    a2dpSuspend = false;

    /* Check for BT device connected state */
    for (int32_t i = 0; i < mDevices.size(); i++) {
        pal_device_id_t dev_id = (pal_device_id_t)mDevices[i]->getSndDeviceId();
        if (rm->isBtDevice(dev_id) && !(rm->isDeviceAvailable(dev_id))) {
            PAL_ERR(LOG_TAG, "BT device %d not connected", dev_id);
            status = -ENODEV;
            goto exit;
        }
    }

    /* SCO device is not ready */
    if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_SCO) &&
        !rm->isDeviceReady(PAL_DEVICE_OUT_BLUETOOTH_SCO)) {
        // If it's sco + speaker combo device, route to speaker.
        // Otherwise, return -EAGAIN.
        if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_SPEAKER)) {
            PAL_INFO(LOG_TAG, "BT SCO output device is not ready, route to speaker");
            for (auto iter = mDevices.begin(); iter != mDevices.end();) {
                if ((*iter)->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER) {
                    iter++;
                    continue;
                }

                // Invoke session API to explicitly update the device metadata
                rm->lockGraph();
                status = session->disconnectSessionDevice(this, mStreamAttr->type, (*iter));
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", status);
                    rm->unlockGraph();
                    goto exit;
                }

                status = (*iter)->close();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "device close failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
                iter = mDevices.erase(iter);
                rm->unlockGraph();
            }
        } else {
            PAL_ERR(LOG_TAG, "BT SCO output device is not ready");
            status = -EAGAIN;
            goto exit;
        }
    }

    /* A2DP/BLE device is not ready */
    if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_A2DP) ||
        rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_BLE) ||
        rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)) {
        if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {
            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_A2DP;
        } else if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST)){
            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST;
        } else {
            dattr.id = PAL_DEVICE_OUT_BLUETOOTH_BLE;
        }
        dev = Device::getInstance(&dattr, rm);
        if (!dev) {
            status = -ENODEV;
            PAL_ERR(LOG_TAG, "failed to get a2dp/ble device object");
            goto exit;
        }
        dev->getDeviceParameter(PAL_PARAM_ID_BT_A2DP_SUSPENDED,
                        (void **)&param_bt_a2dp);

#ifdef SEC_AUDIO_BLE_OFFLOAD
        bool is_a2dp_suspend_for_ble = false;
        // check a2dp suspend for ble state only for a2dp device case
        if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_BLUETOOTH_A2DP)
                && param_bt_a2dp->a2dp_suspended_for_ble == true){
            PAL_INFO(LOG_TAG, "BT A2DP suspended for BLE");
            is_a2dp_suspend_for_ble = true;
        }
#endif

        if ((param_bt_a2dp->a2dp_suspended == false)
#ifdef SEC_AUDIO_BLE_OFFLOAD
            && (is_a2dp_suspend_for_ble == false)
#endif
        ) {
            PAL_DBG(LOG_TAG, "BT A2DP/BLE output device is good to go");
            goto exit;
        }

        if (rm->isDeviceAvailable(mDevices, PAL_DEVICE_OUT_SPEAKER)) {
            // If it's a2dp + speaker combo device, route to speaker.
            PAL_INFO(LOG_TAG, "BT A2DP/BLE output device is not ready, route to speaker");

            /* In combo use case, if ringtone routed to a2dp + spkr and at that time a2dp/ble
             * device is in suspended state, so during resume ringtone won't be able to route
             * to BLE device. In that case, add both speaker and a2dp/ble into suspended devices
             * list so that a2dp/ble will be restored during a2dpResume without removing speaker
             * from stream
             */
            suspendedOutDevIds.clear();
            suspendedOutDevIds.push_back(PAL_DEVICE_OUT_SPEAKER);
            suspendedOutDevIds.push_back(dattr.id);

            for (auto iter = mDevices.begin(); iter != mDevices.end();) {
                if ((*iter)->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER) {
                    iter++;
                    continue;
                }

                rm->lockGraph();
                status = session->disconnectSessionDevice(this, mStreamAttr->type, (*iter));
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", status);
                    rm->unlockGraph();
                    goto exit;
                }

                status = (*iter)->close();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "device close failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
                iter = mDevices.erase(iter);
                rm->unlockGraph();
            }
        } else {
            // For non-combo device, mute the stream and route to speaker or handset
            PAL_INFO(LOG_TAG, "BT A2DP/BLE output device is not ready");

            // Mark the suspendedOutDevIds state early - As a2dpResume may happen during this time.
            a2dpSuspend = true;
            suspendedOutDevIds.clear();
            suspendedOutDevIds.push_back(dattr.id);

            rm->lockGraph();
            for (int i = 0; i < mDevices.size(); i++) {
                status = session->disconnectSessionDevice(this, mStreamAttr->type, mDevices[i]);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", status);
                    rm->unlockGraph();
                    goto exit;
                }
                /* Special handling for aaudio usecase on A2DP/BLE.
                * A2DP/BLE device starts even when stream is not in START state,
                * hence stop A2DP/BLE device to match device start&stop count.
                */
                if (((mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_A2DP) ||
                    (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_BLE)) && isMMap) {
                    status = mDevices[i]->stop();
                    if (0 != status) {
                        PAL_ERR(LOG_TAG, "BT A2DP/BLE device stop failed with status %d", status);
                        }
                }
                status = mDevices[i]->close();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "device close failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
            }
            mDevices.clear();
            rm->unlockGraph();
            clearOutPalDevices(this);

            /* Check whether there's active stream associated with handset or speaker
             * - Device selected to switch by default is speaker.
             * - Check handset as well if no stream on speaker.
             */
            // NOTE: lockGraph is not intended for speaker or handset device
            spkrDattr.id = PAL_DEVICE_OUT_SPEAKER;
            spkrDev = Device::getInstance(&spkrDattr , rm);
            handsetDattr.id = PAL_DEVICE_OUT_HANDSET;
            handsetDev = Device::getInstance(&handsetDattr , rm);
            if (!spkrDev || !handsetDev) {
                status = -ENODEV;
                PAL_ERR(LOG_TAG, "Failed to get speaker or handset instance");
                goto exit;
            }

            dattr.id = spkrDattr.id;
            dev = spkrDev;

            rm->getActiveStream_l(activeStreams, spkrDev);
            if (activeStreams.empty()) {
                rm->getActiveStream_l(activeStreams, handsetDev);
                if (!activeStreams.empty()) {
                    // active streams found on handset
                    dattr.id = PAL_DEVICE_OUT_HANDSET;
                    dev = handsetDev;
                } else {
                    // no active stream found on both speaker and handset, get the deafult
                    pal_device_info devInfo;
                    memset(&devInfo, 0, sizeof(pal_device_info));
                    status = rm->getDeviceConfig(&dattr, NULL);
                    if (!status) {
                        // get the default device info and update snd name
                        rm->getDeviceInfo(dattr.id, (pal_stream_type_t)0,
                                dattr.custom_config.custom_key, &devInfo);
                        rm->updateSndName(dattr.id, devInfo.sndDevName);
                    }
                    dev->setDeviceAttributes(dattr);
                }
            }

            PAL_INFO(LOG_TAG, "mute stream and route to device %d", dattr.id);

            status = dev->open();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "device open failed with status %d", status);
                goto exit;
            }

            mDevices.push_back(dev);
            status = session->setupSessionDevice(this, mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "setupSessionDevice failed:%d", status);
                dev->close();
                mDevices.pop_back();
                goto exit;
            }

            /* Special handling for aaudio usecase on Speaker
             * Speaker device start needs to be called before graph_open
             * to start VI feedback graph and send SP payload to AGM.
             */
            if (dev->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER && isMMap) {
                status = dev->start();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Speaker device start failed with status %d", status);
                    dev->close();
                    mDevices.pop_back();
                    goto exit;
                }
            }
            status = session->connectSessionDevice(this, mStreamAttr->type, dev);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "connectSessionDevice failed:%d", status);
                dev->close();
                mDevices.pop_back();
                goto exit;
            }
            dev->getDeviceAttributes(&dattr, this);
            addPalDevice(this, &dattr);
        }
    }

exit:
    return status;
}

int32_t Stream::disconnectStreamDevice(Stream* streamHandle, pal_device_id_t dev_id)
{
    int32_t status = 0;
    mStreamMutex.lock();
    status = disconnectStreamDevice_l(streamHandle, dev_id);
    mStreamMutex.unlock();

    return status;
}

int32_t Stream::disconnectStreamDevice_l(Stream* streamHandle, pal_device_id_t dev_id)
{
    int32_t status = 0;

    if (currentState == STREAM_IDLE || PAL_CARD_STATUS_DOWN(rm->cardState)) {
        for (int i = 0; i < mDevices.size(); i++) {
            if (dev_id == mDevices[i]->getSndDeviceId()) {
                mDevices.erase(mDevices.begin() + i);
                PAL_DBG(LOG_TAG, "stream is in IDLE state or SSR coming, erase device: %d", dev_id);
                break;
            }
        }
        status = 0;
        goto exit;
    }

    // Stream does not know if the same device is being used by other streams or not
    // So if any other streams are using the same device that has to be handled outside of stream
    // resouce manager ??
    for (int i = 0; i < mDevices.size(); i++) {
        if (dev_id == mDevices[i]->getSndDeviceId()) {
            PAL_DBG(LOG_TAG, "device %d name %s, going to stop",
                mDevices[i]->getSndDeviceId(), mDevices[i]->getPALDeviceName().c_str());
            if (currentState != STREAM_STOPPED) {
                rm->deregisterDevice(mDevices[i], this);
            }

            if(mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER &&
                     ResourceManager::isSpeakerProtectionEnabled) {
               status = mDevices[i]->stop();
               if (0 != status) {
                   PAL_ERR(LOG_TAG, "device stop failed with status %d", status);
                   rm->unlockGraph();
                   goto exit;
               }
            }

            rm->lockGraph();
            status = session->disconnectSessionDevice(streamHandle, mStreamAttr->type, mDevices[i]);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "disconnectSessionDevice failed:%d", status);
                rm->unlockGraph();
                goto exit;
            }
            /* Special handling for aaudio usecase on A2DP/BLE.
             * A2DP/BLE device starts even when stream is not in START state,
             * hence stop A2DP/BLE device to match device start&stop count.
             */

            if ((currentState != STREAM_INIT && currentState != STREAM_STOPPED) ||
                ((currentState == STREAM_INIT || currentState == STREAM_STOPPED) &&
                ((mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_A2DP) ||
                (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_BLE) ||
                (mDevices[i]->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER)) &&
                 isMMap)) {
                status = mDevices[i]->stop();
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "device stop failed with status %d", status);
                    rm->unlockGraph();
                    goto exit;
                }
            }

            status = mDevices[i]->close();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "device close failed with status %d", status);
                rm->unlockGraph();
                goto exit;
            }
            mDevices.erase(mDevices.begin() + i);
            rm->unlockGraph();
            break;
        }
    }
    rm->checkAndSetDutyCycleParam();

exit:
    return status;
}

int32_t Stream::connectStreamDevice(Stream* streamHandle, struct pal_device *dattr)
{
    int32_t status = 0;
    mStreamMutex.lock();
    status = connectStreamDevice_l(streamHandle, dattr);
    mStreamMutex.unlock();

    return status;
}

int32_t Stream::connectStreamDevice_l(Stream* streamHandle, struct pal_device *dattr)
{
    int32_t status = 0;
    std::shared_ptr<Device> dev = nullptr;
    std::string newBackEndName;
    std::string curBackEndName;

    if (!dattr) {
        PAL_ERR(LOG_TAG, "invalid params");
        status = -EINVAL;
        goto exit;
    }

    dev = Device::getInstance(dattr, rm);
    if (!dev) {
        PAL_ERR(LOG_TAG, "Device creation failed");
        goto exit;
    }

    dev->setDeviceAttributes(*dattr);

    if (currentState == STREAM_IDLE || PAL_CARD_STATUS_DOWN(rm->cardState)) {
        PAL_DBG(LOG_TAG, "stream is in IDLE state or SSR coming, insert %d to mDevices", dev->getSndDeviceId());
        mDevices.push_back(dev);
        status = 0;
        goto exit;
    }

    /* Avoid stream connecting to devices sharing the same backend.
     * - For A2DP streams may play on combo devices like Speaker and A2DP.
     *   However, if a2dp suspend is called, all streams on a2dp will temporarily
     *   move to speaker. If the stream is already connected to speaker, speaker
     *   will be connected twice.
     * - For multi-recording stream connecting to bt-sco-mic and handset-mic,
     *   if a2dp suspend arrives, stream will switch from bt-sco-mic to speaker-mic.
     *   Hence, both speaker-mic and handset-mic will be enabled.
     */
    rm->getBackendName(dev->getSndDeviceId(), newBackEndName);
    for (auto iter = mDevices.begin(); iter != mDevices.end(); iter++) {
        rm->getBackendName((*iter)->getSndDeviceId(), curBackEndName);
        if (newBackEndName == curBackEndName) {
            PAL_INFO(LOG_TAG,
                "stream is already connected to device %d name %s - return",
                dev->getSndDeviceId(), dev->getPALDeviceName().c_str());
            status = 0;
            goto exit;
        }
    }

    /* For UC2: USB insertion on playback, after disabling PA, notify PMIC
     * assuming that current Concurrent Boost status is false and Limiter
     * is not configured for speaker.Audio will continue to playback irrespective
     * of success/failure after notifying PMIC about enabling concurrency
     */
    if (ResourceManager::isChargeConcurrencyEnabled && dev
        && dev->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER &&
        !rm->getConcurrentBoostState() && !rm->getInputCurrentLimitorConfigStatus())
        status = rm->chargerListenerSetBoostState(true, PB_ON_CHARGER_INSERT);

    PAL_DBG(LOG_TAG, "device %d name %s, going to start",
        dev->getSndDeviceId(), dev->getPALDeviceName().c_str());

    status = dev->open();
    if (0 != status) {
        PAL_ERR(LOG_TAG, "device %d open failed with status %d",
            dev->getSndDeviceId(), status);
        goto exit;
    }

    mDevices.push_back(dev);
    status = session->setupSessionDevice(streamHandle, mStreamAttr->type, dev);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "setupSessionDevice for %d failed with status %d",
                dev->getSndDeviceId(), status);
        goto dev_close;
    }

    /* Special handling for aaudio usecase on A2DP/BLE/Speaker.
     * For mmap usecase, if device switch happens to A2DP/BLE/Speaker device
     * before stream_start then start A2DP/BLE/speaker dev. since it won't be
     * started again as a part of pal_stream_start().
     *
     * Currently device switch to BT is not supported for stopped mmap stream.
     */
    // TODO: add support for device switch to BT for stopped streams
    rm->lockGraph();
    if ((currentState != STREAM_INIT && currentState != STREAM_STOPPED) ||
        ((currentState == STREAM_INIT || currentState == STREAM_STOPPED) &&
        ((dev->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_A2DP) ||
        (dev->getSndDeviceId() == PAL_DEVICE_OUT_BLUETOOTH_BLE) ||
        (dev->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER)) &&
        isMMap)) {
        status = dev->start();
        if (0 != status) {
            PAL_ERR(LOG_TAG, "device %d name %s, start failed with status %d",
                dev->getSndDeviceId(), dev->getPALDeviceName().c_str(), status);
            rm->unlockGraph();
            goto dev_close;
        }
    } else if (rm->isBtDevice((pal_device_id_t)dev->getSndDeviceId()) &&
            currentState == STREAM_STOPPED) {
        PAL_DBG(LOG_TAG, "stream is in %d state, no need to switch to BT", currentState);
        status = 0;
        rm->unlockGraph();
        goto exit;
    }

    status = session->connectSessionDevice(streamHandle, mStreamAttr->type, dev);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "connectSessionDevice failed:%d", status);
        rm->unlockGraph();
        goto dev_stop;
    }
    rm->unlockGraph();
    if (currentState != STREAM_STOPPED) {
        rm->registerDevice(dev, this);
    }

    rm->checkAndSetDutyCycleParam();
    palStateEnqueue(streamHandle, (pal_state_queue_state) currentState, status);

    /* For UC2: USB insertion on playback, After USB online notification,
     * As enabling PA is done assuming that current Concurrent Boost state
     * is True and Audio will config Limiter for speaker.
     */
    if (ResourceManager::isChargeConcurrencyEnabled && dev &&
        (dev->getSndDeviceId() == PAL_DEVICE_OUT_SPEAKER) && rm->getConcurrentBoostState()
        && !rm->getInputCurrentLimitorConfigStatus() && rm->getChargerOnlineState())
        status = rm->setSessionParamConfig(PAL_PARAM_ID_CHARGER_STATE, streamHandle,
                                           CHARGE_CONCURRENCY_ON_TAG);
    goto exit;

dev_stop:
    if (status != -ENETRESET) {
        dev->stop();
    }

dev_close:
    /* Do not pop the current device from stream, if session connect failed due to SSR down
     * event so that when SSR is up that device will be associated to stream.
     */
    if (status != -ENETRESET) {
        mDevices.pop_back();
        dev->close();
    }

exit:
    /* check if USB is not available restore to default device */
    if (dev && status && (dev->getSndDeviceId() == PAL_DEVICE_OUT_USB_HEADSET ||
                   dev->getSndDeviceId() == PAL_DEVICE_IN_USB_HEADSET)) {
       if (USB::isUsbConnected(dattr->address)) {
           PAL_ERR(LOG_TAG, "USB still connected, connect failed");
       } else {
           status = -ENOSYS;
           PAL_ERR(LOG_TAG, "failed to connect to USB device");
       }

    }
    return status;
}

/*
  legend:
  s1 - current stream
  s2 - existing stream
  d1, d11, d2 - PAL_DEVICE enums
  B1, B2 - backend strings

case 1
  a) s1:  d1(B1) ->  d2(B2)
     OR
  b) s1:  d1(B1) ->  d2(B1)

  resolution: tear down d1 and setup d2.

case 2
  s1:  d1(B1) ->  d1(B1) , d2(B2)

  resolution: check if d1 needs device switch by calling isDeviceSwitchRequired.
    If needed, tear down and setup d1 again. Also setup d2.

case 3:
  s1:  d1(B1), d2(B2) ->  d1(B1)

  resolution: check if d1 needs device switch by calling isDeviceSwitchRequired.
    If needed, tear down and setup d1 again. Also teardown d2.

case 4
  s2->dev d1(B1)
  s1: d2(B2) -> d1(B1)

  resolution: check if s2 is voice call stream, then use same device attribute
    for s1. Else check if isDeviceSwitchRequired() returns true for s2 d1.
    If yes, tear down and setup d1 again for s2. Also, setup d1 for s2.

case 5
  s2->dev d2(B1)
  s1: d11(B2) -> d1(B1)
  resolution: If s2 is voice call stream, then setup d2 for s1 instead of d1
    and do nothing for s2. Else, tear down d2 from s2 and setup d1 for both
    s1 and s2.

case 6
  s2->dev d2(B2)
  s1: d2(B2) -> d1(B2)
  resolution: same handling as case 5

case 7
  s2->dev d1(B1), d2(B2)
  s1: d1(B1) -> d2(B2)

  resolution: nothing to be done for s2+d1. For s2+d2, it is
    same as case 4.

*/
int32_t Stream::switchDevice(Stream* streamHandle, uint32_t numDev, struct pal_device *newDevices)
{
    int32_t status = 0;
    int32_t connectCount = 0, disconnectCount = 0;
    bool isNewDeviceA2dp = false;
    bool isCurDeviceA2dp = false;
    bool isCurDeviceSco = false;
    bool isCurrentDeviceProxyOut = false;
    bool isCurrentDeviceDpOut = false;
    bool matchFound = false;
    bool voice_call_switch = false;
    bool force_switch_dev_id[PAL_DEVICE_IN_MAX] = {};
    uint32_t curDeviceSlots[PAL_DEVICE_IN_MAX], newDeviceSlots[PAL_DEVICE_IN_MAX];
    std::vector <std::tuple<Stream *, uint32_t>> streamDevDisconnect, sharedBEStreamDev, streamsSkippingSwitch;
    std::vector <std::tuple<Stream *, struct pal_device *>> StreamDevConnect;
    struct pal_device dAttr;
    struct pal_device_info deviceInfo;
    uint32_t temp_prio = MIN_USECASE_PRIORITY;
    pal_stream_attributes strAttr;
    char CurrentSndDeviceName[DEVICE_NAME_MAX_SIZE] = {0};
    std::vector <Stream *> streamsToSwitch;
    struct pal_device streamDevAttr;
    struct pal_device sco_Dattr = {};
    std::vector <Stream*>::iterator sIter;
    bool has_out_device = false, has_in_device = false;
    std::vector <std::shared_ptr<Device>>::iterator dIter;
    struct pal_volume_data *volume = NULL;
    pal_device_id_t curBtDevId = PAL_DEVICE_NONE;
    pal_device_id_t newBtDevId;
    bool isBtReady = false;

    rm->lockActiveStream();
    mStreamMutex.lock();

    if ((numDev == 0) || (numDev > PAL_DEVICE_IN_MAX) || (!newDevices) || (!streamHandle)) {
        PAL_ERR(LOG_TAG, "invalid param for device switch");
        mStreamMutex.unlock();
        rm->unlockActiveStream();
        return -EINVAL;
    }

    streamHandle->getStreamAttributes(&strAttr);

    for (int i = 0; i < mDevices.size(); i++) {
        pal_device_id_t curDevId = (pal_device_id_t)mDevices[i]->getSndDeviceId();

        if (curDevId == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
            curDevId == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
            curDevId == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
            isCurDeviceA2dp = true;
            curBtDevId = curDevId;
        }

        if (curDevId == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
            isCurDeviceSco = true;
            curBtDevId = curDevId;
        }

        if (curDevId == PAL_DEVICE_OUT_PROXY)
            isCurrentDeviceProxyOut = true;

        if (curDevId == PAL_DEVICE_OUT_AUX_DIGITAL ||
            curDevId == PAL_DEVICE_OUT_AUX_DIGITAL_1 ||
            curDevId == PAL_DEVICE_OUT_HDMI)
            isCurrentDeviceDpOut = true;

        /*
         * If stream is currently running on same device, then check if
         * it needs device switch. If not needed, then do not add it to
         * streamDevConnect and streamDevDisconnect list. This handles case 2,3.
         */
        matchFound = false;
        for (int j = 0; j < numDev; j++) {
            if (curDevId == newDevices[j].id) {
                matchFound = true;
                /* special handle if same device switch is triggered by different custom key */
                struct pal_device curDevAttr;
                mDevices[i]->getDeviceAttributes(&curDevAttr, streamHandle);
                if (strcmp(newDevices[j].custom_config.custom_key,
                               curDevAttr.custom_config.custom_key) != 0) {
                    PAL_DBG(LOG_TAG, "found diff custom key is %s, running dev has %s, device switch needed",
                        newDevices[j].custom_config.custom_key,
                        curDevAttr.custom_config.custom_key);
                    if (newDevices[j].id >= PAL_DEVICE_IN_MAX) {
                        PAL_ERR(LOG_TAG, "invalid device id: %d", newDevices[j].id);
                        break;
                    }
                    force_switch_dev_id[newDevices[j].id] = true;
                }
                break;
            }
        }
        /* If mDevice[i] does not match with any of new device,
           then update disconnect list */
        if (matchFound == false) {
            curDeviceSlots[disconnectCount] = i;
            disconnectCount++;
        }
    }

    /* remove stream-device attr and mPalDevices which has same dir with new devices*/
    for (int i = 0; i < numDev; i++) {
        if (rm->isOutputDevId(newDevices[i].id))
            has_out_device = true;
        if (rm->isInputDevId(newDevices[i].id))
            has_in_device = true;
    }
    for (dIter = mPalDevices.begin(); dIter != mPalDevices.end(); ) {
        if (rm->isOutputDevId((*dIter)->getSndDeviceId()) &&
            has_out_device) {
            (*dIter)->removeStreamDeviceAttr(streamHandle);
            dIter = mPalDevices.erase(dIter);
        } else if (rm->isInputDevId((*dIter)->getSndDeviceId()) &&
            has_in_device) {
            (*dIter)->removeStreamDeviceAttr(streamHandle);
            dIter = mPalDevices.erase(dIter);
        } else {
            dIter++;
        }
    }

    for (int i = 0; i < numDev; i++) {
        struct pal_device_info devinfo = {};
        std::shared_ptr<Device> dev = nullptr;
        bool devReadyStatus = false;
        pal_param_bta2dp_t* param_bt_a2dp = nullptr;
        std::vector<Stream*> bleRecordStream;
        struct pal_device inBleDattr = {};

        /*
         * When A2DP, Out Proxy and DP device is disconnected the
         * music playback is paused and the policy manager sends routing=0
         * But the audioflinger continues to write data until standby time
         * (3sec). As BT is turned off, the write gets blocked.
         * Avoid this by routing audio to speaker until standby.
         *
         * If a stream is active on SCO and playback has ended, APM will send
         * routing=0. Stream will be closed in PAL after standby time. If SCO
         * device gets disconnected, this stream will not receive new routing
         * and stream will remain with SCO for the time being. If SCO device
         * gets connected again with different config in the meantime and
         * capture stream tries to start ABR path, it will lead to error due to
         * config mismatch. Added OUT_SCO device handling to resolve this.
         */
        // This assumes that PAL_DEVICE_NONE comes as single device
        if ((newDevices[i].id == PAL_DEVICE_NONE) &&
            ((isCurrentDeviceProxyOut) || (isCurrentDeviceDpOut) ||
             ((isCurDeviceA2dp || isCurDeviceSco) && (!rm->isDeviceReady(curBtDevId))))) {
#ifdef SEC_AUDIO_FACTORY_TEST_MODE
            if ((newDevices[i].id == PAL_DEVICE_NONE) && isCurrentDeviceDpOut) {
                PAL_DBG(LOG_TAG, "skip to force switch DP stream to speaker in case of factory DP test");
                mStreamMutex.unlock();
                rm->unlockActiveStream();
                goto done;
            }
#endif
            newDevices[i].id = PAL_DEVICE_OUT_DUMMY;

            if (rm->getDeviceConfig(&newDevices[i], mStreamAttr)) {
                continue;
            }
        }

        if (newDevices[i].id == PAL_DEVICE_NONE) {
            mStreamMutex.unlock();
            rm->unlockActiveStream();
            return 0;
        }

        if (newDevices[i].id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) {
            inBleDattr.id = PAL_DEVICE_IN_BLUETOOTH_BLE;
            dev = Device::getInstance(&inBleDattr, rm);
            if (dev) {
                rm->getActiveStream_l(bleRecordStream, dev);
                if (bleRecordStream.size() > 0)
                    newDevices[i].id = PAL_DEVICE_OUT_DUMMY;
                dev = nullptr;
            }
        }
        devReadyStatus = rm->isDeviceReady(newDevices[i].id);
        if (rm->isBtA2dpDevice(newDevices[i].id)) {
            isNewDeviceA2dp = true;
            newBtDevId = newDevices[i].id;
            isBtReady = devReadyStatus;
            if (!devReadyStatus &&
                !rm->isDeviceAvailable(newDevices, numDev, PAL_DEVICE_OUT_SPEAKER)) {
                /* update suspended device to a2dp and don't route as BT returned error
                 * However it is still possible a2dp routing called as part of a2dp restore
                 */
                PAL_ERR(LOG_TAG, "A2DP profile is not ready, ignoring routing request");
                if (rm->isOutputDevId(newBtDevId)) {
                    suspendedOutDevIds.clear();
                    suspendedOutDevIds.push_back(newDevices[i].id);
                } else if (rm->isInputDevId(newBtDevId)) {
                    suspendedInDevIds.clear();
                    suspendedInDevIds.push_back(newDevices[i].id);
                }
            }
        }
        if (devReadyStatus) {
            newDeviceSlots[connectCount] = i;
            connectCount++;
        }
        /* insert current stream-device attr to Device */
        dev = Device::getInstance(&newDevices[i],rm);
        if (!dev) {
            PAL_ERR(LOG_TAG, "No device instance found");
            mStreamMutex.unlock();
            rm->unlockActiveStream();
            return -ENODEV;
        }
        dev->insertStreamDeviceAttr(&newDevices[i], streamHandle);
        mPalDevices.push_back(dev);
    }

    /*  No new device is ready */
    if ((numDev != 0) && (connectCount == 0)) {
        PAL_INFO(LOG_TAG, "No new device is ready to connect");
        mStreamMutex.unlock();
        rm->unlockActiveStream();
        return 0;
    }

    PAL_INFO(LOG_TAG,"number of active devices %zu, new devices %d", mDevices.size(), connectCount);

    /* created stream device connect and disconnect list */
    streamDevDisconnect.clear();
    StreamDevConnect.clear();

    for (int i = 0; i < connectCount; i++) {
        std::vector <Stream *> activeStreams;
        pal_device_id_t newDeviceId = newDevices[newDeviceSlots[i]].id;

        sharedBEStreamDev.clear();
        // get active stream device pairs sharing the same backend with new devices.
        rm->getSharedBEActiveStreamDevs(sharedBEStreamDev, newDeviceId);

        rm->getDeviceInfo((pal_device_id_t)newDeviceId,
                          strAttr.type, newDevices[newDeviceSlots[i]].custom_config.custom_key,
                          &deviceInfo);

        /* This can result in below situation:
         * 1) No matching SharedBEStreamDev (handled in else part).
         *    e.g. - case 1a, 2.
         * 2) sharedBEStreamDev having same stream as current stream but different device id.
         *    e.g. case 1b.
         * 3) sharedBEStreamDev having different stream from current but same id as current id.
         *    e.g. - case 4,7.
         * 4) sharedBEStreamDev having different stream from current and different device id.
         *    e.g. case 5,6.
         * 5) sharedBEStreamDev having same stream as current stream and same device id.
         *    e.g. spkr -> spkr + hs. This will only come in list if speaker needs device switch.
         * This list excludes same stream and same id which does not need device switch as it
         * is removed above.
         */
        if (sharedBEStreamDev.size() > 0) {
            /* compare new stream-device attr with current active stream-device attr */
            bool switchStreams = rm->compareSharedBEStreamDevAttr(sharedBEStreamDev, &newDevices[newDeviceSlots[i]], true);
            for (const auto &elem : sharedBEStreamDev) {
                Stream *sharedStream = std::get<0>(elem);
                struct pal_device curDevAttr;
                bool custom_switch = false;

                curDevAttr.id = (pal_device_id_t)std::get<1>(elem);
                /*
                 * for current stream, if custom key updated, even rest of the attr
                 * like sample rate/channels/bit width/... are the same, still need
                 * to switch device to update custom config like devicePP
                 */
                if (curDevAttr.id >= PAL_DEVICE_IN_MAX) {
                    PAL_ERR(LOG_TAG, "cur device id: %d is invalid", curDevAttr.id);
                    continue;
                }
                if ((sharedStream == streamHandle) && force_switch_dev_id[curDevAttr.id]) {
                    custom_switch = true;
                }
                /* If prioirty based attr diffs with running dev switch all devices */
                if (switchStreams || custom_switch) {
                    streamDevDisconnect.push_back(elem);
                    StreamDevConnect.push_back({std::get<0>(elem), &newDevices[newDeviceSlots[i]]});
                    matchFound = true;
                    custom_switch = false;
                } else {
                    matchFound = false;
                }
            }
            /*In case of SWB-->WB/NB SCO voip SHO, APM sends explicit routing separately
            * for RX and TX path, it causes ar_osal timewait crash at the DSP to
            * route just one RX/TX path with new GKV and add graph for feedback path
            * since other TX/RX path still intact with SWB codec. Thus, forcedly route
            * other path also to WB/NB if it's active on SCO device.
            */
            if (matchFound && ((strAttr.type == PAL_STREAM_VOIP_RX &&
                newDeviceId == PAL_DEVICE_OUT_BLUETOOTH_SCO &&
                rm->isDeviceActive(PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET)) ||
                (strAttr.type == PAL_STREAM_VOIP_TX &&
                    newDeviceId == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET &&
                    rm->isDeviceActive(PAL_DEVICE_OUT_BLUETOOTH_SCO)))) {
                std::shared_ptr<Device> scoDev = nullptr;
                std::vector <Stream*> activeStreams;
                if (newDeviceId == PAL_DEVICE_OUT_BLUETOOTH_SCO) {
                    sco_Dattr.id = PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET;
                } else {
                    sco_Dattr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
                }
                scoDev = Device::getInstance(&sco_Dattr, rm);
                status = rm->getDeviceConfig(&sco_Dattr, NULL);
                if (status) {
                    PAL_ERR(LOG_TAG, "getDeviceConfig for bt-sco failed");
                    mStreamMutex.unlock();
                    rm->unlockActiveStream();
                    return status;
                }

                rm->getActiveStream_l(activeStreams, scoDev);
                for (sIter = activeStreams.begin(); sIter != activeStreams.end(); sIter++) {
                    (*sIter)->lockStreamMutex();
                    streamDevDisconnect.push_back({ (*sIter), sco_Dattr.id });
                    StreamDevConnect.push_back({ (*sIter), &sco_Dattr });
                    (*sIter)->unlockStreamMutex();
                }
            }
        } else {
            rm->updateSndName(newDeviceId, deviceInfo.sndDevName);
            matchFound = true;
            /*
             * check if virtual port group config needs to update, this handles scenario below:
             *   upd is already active on ultrasound device, active audio stream switched
             *   from other devices to speaker/handset, so group config needs to be updated to
             *   concurrency config
             */
            status = rm->checkAndUpdateGroupDevConfig(&newDevices[newDeviceSlots[i]], mStreamAttr,
                                                        streamsToSwitch, &streamDevAttr, true);
            if (status) {
                PAL_ERR(LOG_TAG, "no valid group device config found");
                streamsToSwitch.clear();
            }
            if (!streamsToSwitch.empty()) {
                for(sIter = streamsToSwitch.begin(); sIter != streamsToSwitch.end(); sIter++) {
                    streamDevDisconnect.push_back({(*sIter), streamDevAttr.id});
                    StreamDevConnect.push_back({(*sIter), &streamDevAttr});
                }
            }
            streamsToSwitch.clear();

        }
        // check if headset config needs to update when haptics is active
        if (!rm->IsHapticsThroughWSA())
            rm->checkHapticsConcurrency(&newDevices[newDeviceSlots[i]], NULL,
                                                  streamsToSwitch/* not used */, NULL);
        /*
         * switch all streams that are running on the current device if
         * switching device for Voice Call
         */
        sharedBEStreamDev.clear();
        for (int j = 0; j < mDevices.size(); j++) {
            uint32_t mDeviceId = mDevices[j]->getSndDeviceId();
            if (rm->matchDevDir(newDeviceId, mDeviceId) && newDeviceId != mDeviceId) {
                if (mDeviceId == PAL_DEVICE_OUT_PROXY || newDeviceId == PAL_DEVICE_OUT_PROXY)
                    continue;
                rm->getSharedBEActiveStreamDevs(sharedBEStreamDev, mDevices[j]->getSndDeviceId());
                if (strAttr.type == PAL_STREAM_VOICE_CALL &&
                    newDeviceId != PAL_DEVICE_OUT_HEARING_AID) {
                    voice_call_switch = true;
                }
                if (voice_call_switch) {
                    for (const auto &elem : sharedBEStreamDev) {
                        if (!rm->isValidDeviceSwitchForStream(std::get<0>(elem), newDevices[newDeviceSlots[i]].id)) {
                            if (std::get<0>(elem) != NULL)
                                streamsSkippingSwitch.push_back(elem);
                            continue;
                        }

                        streamDevDisconnect.push_back(elem);
                        StreamDevConnect.push_back({std::get<0>(elem), &newDevices[newDeviceSlots[i]]});
                    }
                }
            }
        }

        /* Add device associated with current stream to streamDevDisconnect/StreamDevConnect list */
        for (int j = 0; j < disconnectCount; j++) {
            // check to make sure device direction is the same.
            // for shared BE, new device on the slot may change in compareSharedBEStreamDevAttr()
            // and do device switch if current device differs from new device.
            if (rm->matchDevDir(mDevices[curDeviceSlots[j]]->getSndDeviceId(), newDeviceId) &&
                mDevices[curDeviceSlots[j]]->getSndDeviceId() != newDevices[newDeviceSlots[i]].id) {
                streamDevDisconnect.push_back({streamHandle, mDevices[curDeviceSlots[j]]->getSndDeviceId()});
                // if something disconnected incoming device and current dev diff so push on a switchwe need to add the deivce
                matchFound = true;
                // for switching from speaker/handset to other devices, if this is the last stream that active
                // on speaker/handset, need to update group config from concurrency to standalone
                // if switching happens between speaker & handset, skip the group config check as they're shared backend
                if ((mDevices[curDeviceSlots[j]]->getDeviceCount() == 1) &&
                    (newDeviceId != PAL_DEVICE_OUT_SPEAKER &&
                     newDeviceId != PAL_DEVICE_OUT_HANDSET)) {
                    mDevices[curDeviceSlots[j]]->getDeviceAttributes(&dAttr);
                    status = rm->checkAndUpdateGroupDevConfig(&dAttr, mStreamAttr, streamsToSwitch, &streamDevAttr, false);
                    if (status) {
                        PAL_ERR(LOG_TAG, "no valid group device config found");
                        streamsToSwitch.clear();
                    }
                    if (!streamsToSwitch.empty()) {
                        for(sIter = streamsToSwitch.begin(); sIter != streamsToSwitch.end(); sIter++) {
                            streamDevDisconnect.push_back({(*sIter), streamDevAttr.id});
                            StreamDevConnect.push_back({(*sIter), &streamDevAttr});
                        }
                    }
                    streamsToSwitch.clear();
                }
            }
        }

        if (matchFound || (disconnectCount == 0)) {
            StreamDevConnect.push_back({streamHandle, &newDevices[newDeviceSlots[i]]});
        }
    }

    /* Handle scenario when there is only device to disconnect.
       e.g. case 3 : device switch from spkr+hs to spkr */
    if (connectCount == 0) {
        for (int j = 0; j < disconnectCount; j++) {
            if (rm->matchDevDir(mDevices[curDeviceSlots[j]]->getSndDeviceId(), newDevices[0].id))
                streamDevDisconnect.push_back({streamHandle, mDevices[curDeviceSlots[j]]->getSndDeviceId()});
        }
    }

    /* Check if there is device to disconnect or connect */
    if (!streamDevDisconnect.size() && !StreamDevConnect.size()) {
        PAL_INFO(LOG_TAG, "No device to switch, returning");
        mStreamMutex.unlock();
        rm->unlockActiveStream();
        goto done;
    }

    mStreamMutex.unlock();
    rm->unlockActiveStream();

    status = rm->restoreDeviceConfigForUPD(streamDevDisconnect, StreamDevConnect,
                                           streamsSkippingSwitch);
    if (status) {
        PAL_ERR(LOG_TAG, "Error restoring device config for UPD");
        goto done;
    }

    status = rm->streamDevSwitch(streamDevDisconnect, StreamDevConnect);
    if (status) {
        PAL_ERR(LOG_TAG, "Device switch failed");
    }

done:
    mStreamMutex.lock();
    if (a2dpMuted) {
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
        PAL_INFO(LOG_TAG, "a2dpMuted is true, unmuting stream");
#endif
        if (mVolumeData) {
            volume = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                              (sizeof(struct pal_channel_vol_kv) * (mVolumeData->no_of_volpair))));
        }
        if (!volume) {
            PAL_ERR(LOG_TAG, "pal_volume_data memory allocation failure");
            mStreamMutex.unlock();
            return -ENOMEM;
        }
        status = streamHandle->getVolumeData(volume);
        if (status) {
            PAL_ERR(LOG_TAG, "getVolumeData failed %d", status);
        }
        a2dpMuted = false;
        status = streamHandle->setVolume(volume); //apply cached volume.
        if (status) {
            PAL_ERR(LOG_TAG, "setVolume failed %d", status);
        }
        mute_l(false);
        if (volume) {
            free(volume);
        }
    }
    if ((numDev > 1) && isNewDeviceA2dp && !isBtReady) {
        suspendedOutDevIds.clear();
        suspendedOutDevIds.push_back(newBtDevId);
        if (ResourceManager::isDummyDevEnabled) {
            suspendedOutDevIds.push_back(PAL_DEVICE_OUT_DUMMY);
        } else {
            suspendedOutDevIds.push_back(PAL_DEVICE_OUT_SPEAKER);
        }
    } else {
        suspendedOutDevIds.clear();
    }
    mStreamMutex.unlock();
    return status;
}

bool Stream::checkStreamMatch(pal_device_id_t pal_device_id,
                                        pal_stream_type_t pal_stream_type)
{
    int status = 0;
    struct pal_device dAttr;
    bool match = false;

    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "stream attribute is null");
        return false;
    }

    if (pal_stream_type == mStreamAttr->type ||
            pal_stream_type == PAL_STREAM_GENERIC)
        match = true;
    else
        return false;

    //device
    for (int i = 0; i < mDevices.size();i++) {
       status = mDevices[i]->getDeviceAttributes(&dAttr);
       if (0 != status) {
          PAL_ERR(LOG_TAG,"getDeviceAttributes Failed \n");
          return false;
       }
       if (pal_device_id == dAttr.id || pal_device_id == PAL_DEVICE_NONE) {
           match = true;
           // as long as one device matches, it is enough.
           break;
       } else
           match = false;
    }

    return match;
}

#ifdef SEC_AUDIO_COMMON
bool Stream::checkStreamEffectMatch(pal_device_id_t pal_device_id,
                                    pal_stream_type_t pal_stream_type,
                                    uint32_t param_id)
{
    int status = 0;
    struct pal_device dAttr;
    bool match = false;

    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "stream attribute is null");
        return false;
    }

    if (pal_stream_type == mStreamAttr->type ||
            pal_stream_type == PAL_STREAM_GENERIC)
        match = true;
    else
        return false;

    if (param_id == PARAM_ID_PP_AUDIO_INTERVIEW_PARAMS &&
            (mStreamAttr->direction != PAL_AUDIO_INPUT ||
                mStreamAttr->type != PAL_STREAM_DEEP_BUFFER)) {
        return false;
    }

    if (pal_stream_type == PAL_STREAM_GENERIC &&
            (mStreamAttr->direction != PAL_AUDIO_OUTPUT ||
                mStreamAttr->type == PAL_STREAM_VOIP_RX)) {
        return false;
    }

    if (param_id == PARAM_ID_PP_SB_PARAMS_FLATMOTION &&
            (mStreamAttr->type != PAL_STREAM_GENERIC &&
            mStreamAttr->type != PAL_STREAM_DEEP_BUFFER)) {
        return false;
    }

    if (param_id == PARAM_ID_PP_SB_PARAMS_VOLUME &&
            (mStreamAttr->type != PAL_STREAM_COMPRESSED &&
#ifdef SEC_AUDIO_PREVOLUME_SOUNDBOOSTER
            mStreamAttr->type != PAL_STREAM_ULTRA_LOW_LATENCY &&
            mStreamAttr->type != PAL_STREAM_GENERIC &&
#elif defined (SEC_AUDIO_SUPPORT_SOUNDBOOSTER_ON_DSP)
            mStreamAttr->type != PAL_STREAM_DEEP_BUFFER &&
#ifdef SEC_AUDIO_SUPPORT_SOUNDBOOSTER_MMAP_ON_DSP
            mStreamAttr->type != PAL_STREAM_ULTRA_LOW_LATENCY &&
#endif
#ifdef SEC_AUDIO_SUPPORT_LOWLATENCY_MEDIA			
            mStreamAttr->type != PAL_STREAM_LOW_LATENCY &&
#endif
#endif
            mStreamAttr->type != PAL_STREAM_LOOPBACK)) {
        return false;
    }

#ifdef SEC_AUDIO_SUPPORT_SOUNDBOOSTER_FOLD_PARAM_ON_DSP
    if (param_id == PARAM_ID_PP_SB_PARAMS_FOLD_DEGREE &&
            (mStreamAttr->type != PAL_STREAM_LOW_LATENCY &&
            mStreamAttr->type != PAL_STREAM_DEEP_BUFFER &&
            mStreamAttr->type != PAL_STREAM_COMPRESSED &&
            mStreamAttr->type != PAL_STREAM_GENERIC &&
            mStreamAttr->type != PAL_STREAM_ULTRA_LOW_LATENCY &&
            mStreamAttr->type != PAL_STREAM_LOOPBACK)) {
        return false;
    }
#endif

    //device
    for (int i = 0; i < mDevices.size();i++) {
        status = mDevices[i]->getDeviceAttributes(&dAttr);
        if (0 != status) {
           PAL_ERR(LOG_TAG,"getDeviceAttributes Failed \n");
           return false;
        }

        if (param_id == PARAM_ID_PP_VOLUMEMONITOR_GET_PARAMS ||
            param_id == PARAM_ID_PP_VOLUMEMONITOR_SET_PARAMS) {
            if (dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
                dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
                dAttr.id == PAL_DEVICE_OUT_USB_HEADSET) {
                match = true;
                break;
            } else {
                match = false;
            }
        } else {
            if (pal_device_id == dAttr.id || pal_device_id == PAL_DEVICE_NONE) {
                match = true;
                // as long as one device matches, it is enough.
                break;
            } else
                match = false;
        }
    }

    return match;
}
#endif

int Stream::initStreamSmph()
{
    return sem_init(&mInUse, 0, 1);
}

int Stream::deinitStreamSmph()
{
    return sem_destroy(&mInUse);
}

int Stream::postStreamSmph()
{
    return sem_post(&mInUse);
}

int Stream::waitStreamSmph()
{
    return sem_wait(&mInUse);
}

void Stream::handleStreamException(struct pal_stream_attributes *attributes,
                                   pal_stream_callback cb, uint64_t cookie)
{
    if (!attributes || !cb) {
        PAL_ERR(LOG_TAG,"Invalid params for exception handling \n");
        return;
    }

    if (attributes->type == PAL_STREAM_COMPRESSED) {
         PAL_DBG(LOG_TAG, "Exception for Compressed clip");
         // Notify flinger about the error occurred while creating compress
         // offload session
         cb(NULL, PAL_STREAM_CBK_EVENT_ERROR, 0, 0, cookie);

    }
}

/* GetPalDevice only applies to Sound Trigger streams */
std::shared_ptr<Device> Stream::GetPalDevice(Stream *streamHandle, pal_device_id_t dev_id)
{
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;
    std::shared_ptr<CaptureProfile> common_cap_prof = nullptr;
    std::shared_ptr<Device> device = nullptr;
    StreamSoundTrigger *st_st = nullptr;
    StreamACD *st_acd = nullptr;
    StreamASR *st_asr = nullptr;
    StreamSensorPCMData *st_sns_pcm_data = nullptr;
    struct pal_device dev;

    if (!streamHandle) {
        PAL_ERR(LOG_TAG, "Stream is invalid");
        goto exit;
    }

    if (!mStreamAttr) {
        PAL_ERR(LOG_TAG, "Stream attribute is null");
        goto exit;
    }

    PAL_DBG(LOG_TAG, "Enter, stream: %d, device_id: %d", mStreamAttr->type, dev_id);

    if (mStreamAttr->type == PAL_STREAM_VOICE_UI) {
        st_st = dynamic_cast<StreamSoundTrigger*>(streamHandle);
        cap_prof = st_st->GetCurrentCaptureProfile();
    } else if (mStreamAttr->type == PAL_STREAM_ACD) {
        st_acd = dynamic_cast<StreamACD*>(streamHandle);
        cap_prof = st_acd->GetCurrentCaptureProfile();
    } else if (mStreamAttr->type == PAL_STREAM_ASR) {
        st_asr = dynamic_cast<StreamASR*>(streamHandle);
        cap_prof = st_asr->GetCurrentCaptureProfile();
    } else {
        st_sns_pcm_data = dynamic_cast<StreamSensorPCMData*>(streamHandle);
        cap_prof = st_sns_pcm_data->GetCurrentCaptureProfile();
    }

    if (!cap_prof && !rm->GetSoundTriggerCaptureProfile() &&
        !rm->GetTXMacroCaptureProfile()) {
        PAL_ERR(LOG_TAG, "Failed to get local and common cap_prof for stream: %d",
                mStreamAttr->type);
        goto exit;
    }

    common_cap_prof = (dev_id == PAL_DEVICE_IN_ULTRASOUND_MIC) ?
                       rm->GetTXMacroCaptureProfile(): rm->GetSoundTriggerCaptureProfile();
    if (common_cap_prof) {
        /* Use the rm's common capture profile if local capture profile is not
         * available, or the common capture profile has the highest priority.
         */
        if (!cap_prof ||
            common_cap_prof->ComparePriority(cap_prof) >= CAPTURE_PROFILE_PRIORITY_HIGH) {
            PAL_DBG(LOG_TAG, "common cap_prof %s has the highest priority.",
                    common_cap_prof->GetName().c_str());
            cap_prof = common_cap_prof;
        }
    }

    dev.id = dev_id;
    dev.config.bit_width = cap_prof->GetBitWidth();
    dev.config.ch_info.channels = cap_prof->GetChannels();
    dev.config.sample_rate = cap_prof->GetSampleRate();
    dev.config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;

    device = Device::getInstance(&dev, rm);
    if (!device) {
        PAL_ERR(LOG_TAG, "Failed to get device instance");
        goto exit;
    }

    device->setDeviceAttributes(dev);
    device->setSndName(cap_prof->GetSndName());

exit:
    PAL_DBG(LOG_TAG, "Exit");
    return device;
}

void Stream::setCachedState(stream_state_t state)
{
    mStreamMutex.lock();
    cachedState = state;
    PAL_DBG(LOG_TAG, "set cachedState to %d", cachedState);
    mStreamMutex.unlock();
}

void Stream::clearmDevices()
{
    for (auto iter = mDevices.begin(); iter != mDevices.end();) {
        iter = mDevices.erase(iter);
    }
}

void Stream::removemDevice(int palDevId)
{
    std::vector <std::shared_ptr<Device>>::iterator dIter;
    int devId;

    for (dIter = mDevices.begin(); dIter != mDevices.end();) {
        devId = (*dIter)->getSndDeviceId();
        if (devId == palDevId) {
            mDevices.erase(dIter);
        } else {
            dIter++;
        }
    }
}

void Stream::addmDevice(struct pal_device *dattr)
{
    std::shared_ptr<Device> dev = nullptr;

    dev = Device::getInstance(dattr, rm);
    if (!dev) {
        PAL_ERR(LOG_TAG, "No device instance found");
        return;
    }
    dev->setDeviceAttributes(*dattr);
    mDevices.push_back(dev);
}

bool Stream::isStreamSSRDownFeasibile()
{
    bool is_ssr_down_feasible = true;

    PAL_DBG(LOG_TAG, "Enter cardState %d stream type %d",
            rm->cardState, mStreamAttr->type);

    if (false ==
        rm->isSsrDownFeasible(rm, mStreamAttr->type)) {
        PAL_INFO(LOG_TAG, "SSRDown skipped for stream type %d",
                 mStreamAttr->type);
        skipSSRHandling = true;
        is_ssr_down_feasible = false;
    }

    PAL_DBG(LOG_TAG, "Exit: is_ssr_down_feasible %d",
            is_ssr_down_feasible);
    return is_ssr_down_feasible;
}
