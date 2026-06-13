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
 */

#define LOG_TAG "PAL: Speaker"
#include "Speaker.h"
#include "ResourceManager.h"
#include "SpeakerProtection.h"
#include "Device.h"
#include "kvh2xml.h"

#ifdef ENABLE_TFA98XX_SUPPORT
#include "SpeakerProtectionTFA.h"
#elif ENABLE_TAS_SPK_PROT
#include "SpeakerProtectionTI.h"
#endif

std::shared_ptr<Device> Speaker::obj = nullptr;

std::shared_ptr<Device> Speaker::getObject()
{
    return obj;
}


std::shared_ptr<Device> Speaker::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (!obj) {
        if (ResourceManager::isSpeakerProtectionEnabled) {
            std::shared_ptr<Device> sp(new SpeakerProtection(device, Rm));
            obj = sp;
        }
        else {
#ifdef ENABLE_TFA98XX_SUPPORT
            std::shared_ptr<Device> sp(new SpeakerProtectionTFA(device, Rm));
#elif ENABLE_TAS_SPK_PROT
            std::shared_ptr<Device> sp(new SpeakerProtectionTI(device, Rm));
#else
            std::shared_ptr<Device> sp(new Speaker(device, Rm));
#endif
            obj = sp;
        }
    }
    return obj;
}


Speaker::Speaker(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{
#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
    rm = Rm;
    curMuteStatus = PAL_DEVICE_SPEAKER_UNMUTE;
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
}

Speaker::Speaker()
{

}

Speaker::~Speaker()
{

}

int32_t Speaker::isSampleRateSupported(uint32_t sampleRate)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "sampleRate %u", sampleRate);
    switch (sampleRate) {
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

int32_t Speaker::isChannelSupported(uint32_t numChannels)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "numChannels %u", numChannels);
    switch (numChannels) {
        case CHANNELS_1:
        case CHANNELS_2:
        case CHANNELS_3:
        case CHANNELS_4:
        case CHANNELS_8:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "channels not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t Speaker::isBitWidthSupported(uint32_t bitWidth)
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

int Speaker::close()
{
    int status = 0;

    status = Device::close();
    if (status == 0 && deviceCount == 0) {
        std::shared_ptr<ResourceManager> Rm = nullptr;
        Rm = ResourceManager::getInstance();
        if (Rm && Rm->isChargeConcurrencyEnabled && Rm->getChargerOnlineState() &&
            Rm->getConcurrentBoostState()) {
            status = Rm->chargerListenerSetBoostState(false, CONCURRENCY_PB_STOPS);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Failed to notify PMIC: %d", status);
        } else {
            PAL_DBG(LOG_TAG, "Concurrent State unchanged, ignore notifying");
        }
    }
    return status;
}

#if defined(SEC_AUDIO_DUAL_SPEAKER) || defined(SEC_AUDIO_SCREEN_MIRRORING) // { SUPPORT_VOIP_VIA_SMART_VIEW
int32_t Speaker::setParameter(uint32_t param_id, void *param)
{
    int ret = 0;

    PAL_DBG(LOG_TAG, " Enter.");
    struct audio_route *audioRoute = NULL;
    ret = rm->getAudioRoute(&audioRoute);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", ret);
        return 0;
    }
    pal_param_speaker_status_t* param_speaker_status = (pal_param_speaker_status_t *)param;

    if (curMuteStatus == PAL_DEVICE_SPEAKER_MUTE) { // current : full mute
        if (param_speaker_status->mute_status == PAL_DEVICE_SPEAKER_UNMUTE) {
            disableDevice(audioRoute, "amp-mute");
            curMuteStatus = PAL_DEVICE_SPEAKER_UNMUTE; // full mute -> full unmute
        }
    } else { // current : full unmute, upper mute, upper unmute
        if (param_speaker_status->mute_status == PAL_DEVICE_SPEAKER_MUTE) {
            if (curMuteStatus == PAL_DEVICE_UPPER_SPEAKER_MUTE) {
                disableDevice(audioRoute, "upper-spk-amp-mute");
            }
            enableDevice(audioRoute, "amp-mute");
            curMuteStatus = PAL_DEVICE_SPEAKER_MUTE; // upper mute or full unmute -> full mute
        } else if (param_speaker_status->mute_status == PAL_DEVICE_UPPER_SPEAKER_MUTE) {
            enableDevice(audioRoute, "upper-spk-amp-mute");
            curMuteStatus = PAL_DEVICE_UPPER_SPEAKER_MUTE; // full unmute -> upper mute
        } else if (param_speaker_status->mute_status == PAL_DEVICE_UPPER_SPEAKER_UNMUTE) {
            disableDevice(audioRoute, "upper-spk-amp-mute");
            curMuteStatus = PAL_DEVICE_SPEAKER_UNMUTE; // upper mute -> full unmute
        }
    }
    return 0;
}
#endif // } SUPPORT_VOIP_VIA_SMART_VIEW
