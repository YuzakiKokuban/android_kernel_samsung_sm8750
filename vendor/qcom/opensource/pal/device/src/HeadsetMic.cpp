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

#define LOG_TAG "PAL: HeadsetMic"

#include "HeadsetMic.h"
#include <tinyalsa/asoundlib.h>
#include "PalAudioRoute.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"

std::shared_ptr<Device> HeadsetMic::obj = nullptr;

std::shared_ptr<Device> HeadsetMic::getObject()
{
    return obj;
}

std::shared_ptr<Device> HeadsetMic::getInstance(struct pal_device *device,
                                                std::shared_ptr<ResourceManager> Rm)
{
    if (!obj) {
        std::shared_ptr<Device> sp(new HeadsetMic(device, Rm));
        obj = sp;
    }
    return obj;
}


HeadsetMic::HeadsetMic(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{
   
}

HeadsetMic::~HeadsetMic()
{

}

int32_t HeadsetMic::isSampleRateSupported(uint32_t sampleRate)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "sampleRate %u", sampleRate);
    switch (sampleRate) {\
       //check what all need to be added
        case SAMPLINGRATE_48K:
        case SAMPLINGRATE_96K:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "sample rate not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t HeadsetMic::isChannelSupported(uint32_t numChannels)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    switch (numChannels) {
        case CHANNELS_1:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "channels not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t HeadsetMic::isBitWidthSupported(uint32_t bitWidth)
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

int32_t HeadsetMic::checkAndUpdateBitWidth(uint32_t *bitWidth)
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

int32_t HeadsetMic::checkAndUpdateSampleRate(uint32_t *sampleRate)
{
    int32_t rc = 0;

    /* TODO: support native 44.1 later */
    if (*sampleRate < SAMPLINGRATE_48K)
        *sampleRate = SAMPLINGRATE_48K;
    else if (*sampleRate > SAMPLINGRATE_48K && *sampleRate < SAMPLINGRATE_96K)
        *sampleRate = SAMPLINGRATE_96K;
    else if (*sampleRate > SAMPLINGRATE_96K && *sampleRate < SAMPLINGRATE_192K)
        *sampleRate = SAMPLINGRATE_192K;
    else if (*sampleRate > SAMPLINGRATE_192K && *sampleRate < SAMPLINGRATE_384K)
        *sampleRate = SAMPLINGRATE_384K;

    PAL_DBG(LOG_TAG, "sampleRate %d", *sampleRate);

    return rc;
}
