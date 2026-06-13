/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 */

#define LOG_TAG "PAL: Headphone"
#include "Headphone.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"

std::shared_ptr<Device> Headphone::obj = nullptr;

std::shared_ptr<Device> Headphone::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (!obj) {
        std::shared_ptr<Device> sp(new Headphone(device, Rm));
        obj = sp;
    }
    return obj;
}

std::shared_ptr<Device> Headphone::getObject(pal_device_id_t id)
{
    if (obj) {
        if (obj->getSndDeviceId() == id)
            return obj;
    }
    return NULL;
}

Headphone::Headphone(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{

}

Headphone::~Headphone()
{
PAL_ERR(LOG_TAG, "dtor called");
}

int32_t Headphone::isSampleRateSupported(uint32_t sampleRate)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "sampleRate %d", sampleRate);

    if (sampleRate % SAMPLINGRATE_44K == 0)
        return rc;

    switch (sampleRate) {
        case SAMPLINGRATE_44K:
        case SAMPLINGRATE_48K:
        case SAMPLINGRATE_96K:
        case SAMPLINGRATE_192K:
        case SAMPLINGRATE_384K:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "sample rate not supported rc %d", rc);
            break;
    }
    return rc;
}
//TBD why do these channels have to be supported, headphones support only 1/2?
int32_t Headphone::isChannelSupported(uint32_t numChannels)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    switch (numChannels) {
        case CHANNELS_1:
        case CHANNELS_2:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "channels not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t Headphone::isBitWidthSupported(uint32_t bitWidth)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "bitWidth %u", bitWidth);
    switch (bitWidth) {
        case BITWIDTH_16:
        case BITWIDTH_24:
        case BITWIDTH_32:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "bit width not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t Headphone::checkAndUpdateBitWidth(uint32_t *bitWidth)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "bitWidth %u", *bitWidth);
    switch (*bitWidth) {
        case BITWIDTH_16:
        case BITWIDTH_24:
        case BITWIDTH_32:
            break;
        default:
            *bitWidth = BITWIDTH_16;
            PAL_DBG(LOG_TAG, "bit width not supported, setting to default 16 bit");
            break;
    }
    return rc;
}

int32_t Headphone::checkAndUpdateSampleRate(uint32_t *sampleRate)
{
    int32_t rc = 0;

    if ((*sampleRate % SAMPLINGRATE_44K == 0) &&
        (NATIVE_AUDIO_MODE_MULTIPLE_MIX_IN_DSP == ResourceManager::getNativeAudioSupport())) {
        PAL_DBG(LOG_TAG, "napb: setting sampling rate to %d", *sampleRate);
    } else if (*sampleRate <= SAMPLINGRATE_48K)
        *sampleRate = SAMPLINGRATE_48K;
    else if (*sampleRate > SAMPLINGRATE_48K && *sampleRate <= SAMPLINGRATE_96K)
        *sampleRate = SAMPLINGRATE_96K;
    else if (*sampleRate > SAMPLINGRATE_96K && *sampleRate <= SAMPLINGRATE_192K)
        *sampleRate = SAMPLINGRATE_192K;
    else if (*sampleRate > SAMPLINGRATE_192K && *sampleRate <= SAMPLINGRATE_384K)
        *sampleRate = SAMPLINGRATE_384K;

    PAL_DBG(LOG_TAG, "sampleRate %d", *sampleRate);

    return rc;
}
