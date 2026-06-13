/* * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved. *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

#define LOG_TAG "PAL: ECRefDevice"
#include "ECRefDevice.h"
#include "ResourceManager.h"
#include "Device.h"
#include "PayloadBuilder.h"
#include "Stream.h"
#include "Session.h"
#include "kvh2xml.h"
#include <vector>


std::shared_ptr<Device> ECRefDevice::obj = nullptr;

std::shared_ptr<Device> ECRefDevice::getObject()
{
    return obj;
}

std::shared_ptr<Device> ECRefDevice::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (!obj) {
        std::shared_ptr<Device> sp(new ECRefDevice(device, Rm));
        obj = sp;
    }
    return obj;
}


ECRefDevice::ECRefDevice(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{
    rm = Rm;
    memset(&mDeviceAttr, 0, sizeof(struct pal_device));
    memcpy(&mDeviceAttr, device, sizeof(struct pal_device));
}

ECRefDevice::~ECRefDevice()
{

}

int32_t ECRefDevice::isSampleRateSupported(int32_t sampleRate)
{
    int32_t rc = 0;
    if(sampleRate != SAMPLINGRATE_48K) {
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "sample rate not supported rc %d", rc);
    }
    return rc;

}

int32_t ECRefDevice::isChannelSupported(int32_t numChannels)
{
    int32_t rc = 0;
    if(numChannels != CHANNELS_1){
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "channels not supported rc %d", rc);
    }
    return rc;
}

int32_t ECRefDevice::isBitWidthSupported(int32_t bitWidth)
{
    int32_t rc = 0;
    if (bitWidth != BITWIDTH_16){
        rc = -EINVAL;
        PAL_ERR(LOG_TAG, "bit width not supported rc %d", rc);
    }
    return rc;

}

void ECRefDevice::checkAndUpdateBitWidth(uint32_t *bitWidth)
{
    if (*bitWidth != BITWIDTH_16){
        *bitWidth = BITWIDTH_16;
        PAL_DBG(LOG_TAG, "bit width not supported, setting to default 16 bit");
    }
}

void ECRefDevice::checkAndUpdateSampleRate(uint32_t *sampleRate)
{
    if(*sampleRate != SAMPLINGRATE_48K) {
        *sampleRate = SAMPLINGRATE_48K;
        PAL_ERR(LOG_TAG, "sample rate not supported, setting to default 48k");
    }
}

int ECRefDevice::start()
{
    int status = 0;
    Stream *stream = NULL;
    Session *session = NULL;
    std::vector<Stream*> activestreams;
    std::shared_ptr<PayloadBuilder> builder = std::make_shared<PayloadBuilder>();
    std::string backEndName;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    uint32_t ratMiid = 0;

    codecConfig.sample_rate = SAMPLINGRATE_48K;
    codecConfig.bit_width = BITWIDTH_16;
    codecConfig.ch_info.channels = CHANNELS_1;

    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;

    rm->getBackendName(mDeviceAttr.id, backEndName);

    status = rm->getActiveStream_l(activestreams, getObject());
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        status = -EINVAL;
        goto error;
    }
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);

    /* RAT Module Configuration */
    status = session->getMIID(backEndName.c_str(), RAT_RENDER, &ratMiid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d",
          RAT_RENDER, status);
        goto error;
    }

    builder->payloadRATConfig(&paramData, &paramSize, ratMiid, &codecConfig);
    if (paramSize) {
        updateCustomPayload(paramData, paramSize);
        free(paramData);
        paramData = NULL;
        paramSize = 0;
    } else {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "Invalid RAT module param size");
        goto error;
    }

    status = Device::start();

error:
    PAL_DBG(LOG_TAG, "Exit");
    return status;
}
