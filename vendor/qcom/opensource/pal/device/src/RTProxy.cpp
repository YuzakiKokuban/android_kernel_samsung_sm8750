/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: RTProxy"
#define LOG_TAG_OUT "PAL: RTProxyOut"
#include "RTProxy.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"

#include "PayloadBuilder.h"
#include "Stream.h"
#include "Session.h"

RTProxyIn::RTProxyIn(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{
    rm = Rm;
    setDeviceAttributes(*device);
}

RTProxyIn::~RTProxyIn()
{

}

int32_t RTProxyIn::isSampleRateSupported(uint32_t sampleRate)
{
    PAL_DBG(LOG_TAG, "sampleRate %u", sampleRate);
    /* Proxy supports all sample rates, accept by default */
    return 0;
}

int32_t RTProxyIn::isChannelSupported(uint32_t numChannels)
{
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    /* Proxy supports all channels, accept by default */
    return 0;
}

int32_t RTProxyIn::isBitWidthSupported(uint32_t bitWidth)
{
    PAL_DBG(LOG_TAG, "bitWidth %u", bitWidth);
    /* Proxy supports all bitwidths, accept by default */
    return 0;
}


std::shared_ptr<Device> RTProxyIn::objPlay = nullptr;
std::shared_ptr<Device> RTProxyIn::objRecord = nullptr;


std::shared_ptr<Device> RTProxyIn::getObject(pal_device_id_t id)
{
    std::shared_ptr<Device> obj = nullptr;

    if (id == PAL_DEVICE_IN_TELEPHONY_RX ||
       id == PAL_DEVICE_IN_PROXY) {
        obj = objPlay;
    }
    if (id == PAL_DEVICE_IN_RECORD_PROXY) {
        obj = objRecord;
    }
    return obj;
}
std::shared_ptr<Device> RTProxyIn::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    std::shared_ptr<Device> obj = nullptr;

    if (device->id == PAL_DEVICE_IN_TELEPHONY_RX ||
       device->id == PAL_DEVICE_IN_PROXY) {
        if (!objPlay) {
            std::shared_ptr<Device> sp(new RTProxyIn(device, Rm));
            objPlay = sp;
        }
        obj = objPlay;
    }
    if (device->id == PAL_DEVICE_IN_RECORD_PROXY) {
        if (!objRecord) {
            std::shared_ptr<Device> sp(new RTProxyIn(device, Rm));
            objRecord = sp;
        }
        obj = objRecord;
    }
    return obj;
}

int RTProxyIn::start() {
    int status = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    uint32_t ratMiid = 0;
    //struct pal_media_config config;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activestreams;
    std::shared_ptr<Device> dev = nullptr;
    std::string backEndName;
    PayloadBuilder* builder = new PayloadBuilder();

   if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;
    mDeviceMutex.lock();
    if (0 < deviceStartStopCount) {
        goto start;
    }
    rm->getBackendName(deviceAttr.id, backEndName);

    dev = getInstance(&deviceAttr, rm);

    status = rm->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        status = -EINVAL;
        goto error;
    }
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);

    status = session->getMIID(backEndName.c_str(), RAT_RENDER, &ratMiid);
    if (status) {
        PAL_INFO(LOG_TAG,
         "Failed to get tag info %x Skipping RAT Configuration Setup, status = %d",
          RAT_RENDER, status);
        //status = -EINVAL;
        status = 0;
        goto start;
    }

    builder->payloadRATConfig(&paramData, &paramSize, ratMiid, &deviceAttr.config);
    if (dev && paramSize) {
        dev->updateCustomPayload(paramData, paramSize);
        if (paramData)
            free(paramData);
        paramData = NULL;
        paramSize = 0;
    } else {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid RAT module param size");
        goto error;
    }

start:
    status = Device::start_l();
error:
    mDeviceMutex.unlock();
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}


std::shared_ptr<Device> RTProxyOut::objPlay = nullptr;
std::shared_ptr<Device> RTProxyOut::objRecord = nullptr;

RTProxyOut::RTProxyOut(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
    Device(device, Rm)
{
    rm = Rm;
    setDeviceAttributes(*device);
}

std::shared_ptr<Device> RTProxyOut::getObject(pal_device_id_t id)
{
    std::shared_ptr<Device> obj = nullptr;

    if (id == PAL_DEVICE_OUT_HEARING_AID ||
       id == PAL_DEVICE_OUT_PROXY) {
        obj = objPlay;
    }
    if (id == PAL_DEVICE_OUT_RECORD_PROXY) {
        obj = objRecord;
    }
    return obj;
}

std::shared_ptr<Device> RTProxyOut::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    std::shared_ptr<Device> obj = nullptr;

    if (device->id == PAL_DEVICE_OUT_HEARING_AID ||
       device->id == PAL_DEVICE_OUT_PROXY) {
        if (!objPlay) {
            std::shared_ptr<Device> sp(new RTProxyOut(device, Rm));
            objPlay = sp;
        }
        obj = objPlay;
    }
    if (device->id == PAL_DEVICE_OUT_RECORD_PROXY) {
        if (!objRecord) {
            std::shared_ptr<Device> sp(new RTProxyOut(device, Rm));
            objRecord = sp;
        }
        obj = objRecord;
    }
    return obj;
}


int32_t RTProxyOut::isSampleRateSupported(uint32_t sampleRate)
{
    PAL_DBG(LOG_TAG, "sampleRate %u", sampleRate);
    /* Proxy supports all sample rates, accept by default */
    return 0;
}

int32_t RTProxyOut::isChannelSupported(uint32_t numChannels)
{
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    /* Proxy supports all channels, accept by default */
    return 0;
}

int32_t RTProxyOut::isBitWidthSupported(uint32_t bitWidth)
{
    PAL_DBG(LOG_TAG, "bitWidth %u", bitWidth);
    /* Proxy supports all bitwidths, accept by default */
    return 0;
}
int RTProxyOut::start() {
    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;

    return Device::start();
}

RTProxyOut::~RTProxyOut()
{

}
