 /*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define LOG_TAG "PAL: HapticsDev"
#include "HapticsDev.h"
#include "ResourceManager.h"
#include "Device.h"
#include "kvh2xml.h"
#include "HapticsDevProtection.h"

std::shared_ptr<Device> HapticsDev::obj = nullptr;

std::shared_ptr<Device> HapticsDev::getObject()
{
    return obj;
}

std::shared_ptr<Device> HapticsDev::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (!obj) {
        if (ResourceManager::isHapticsProtectionEnabled &&
                      ResourceManager::isHapticsthroughWSA) {
            std::shared_ptr<Device> sp(new HapticsDevProtection(device, Rm));
            obj = sp;
        } else {
            std::shared_ptr<Device> sp(new HapticsDev(device, Rm));
            obj = sp;
        }
    }
    return obj;
}


HapticsDev::HapticsDev(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{
#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK
    rm = Rm;
    hapticSource = HAPTIC_SOURCE_ACH;
#endif
}

HapticsDev::~HapticsDev()
{

}

int32_t HapticsDev::isSampleRateSupported(uint32_t sampleRate)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "sampleRate %u", sampleRate);
    switch (sampleRate) {
        case SAMPLINGRATE_48K:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "sample rate not supported rc %d", rc);
            break;
    }
    return rc;
}

int32_t HapticsDev::isChannelSupported(uint32_t numChannels)
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

int32_t HapticsDev::isBitWidthSupported(uint32_t bitWidth)
{
    int32_t rc = 0;
    PAL_DBG(LOG_TAG, "bitWidth %u", bitWidth);
    switch (bitWidth) {
        case BITWIDTH_16:
            break;
        default:
            rc = -EINVAL;
            PAL_ERR(LOG_TAG, "bit width not supported rc %d", rc);
            break;
    }
    return rc;
}

#ifdef SEC_AUDIO_SUPPORT_HAPTIC_PLAYBACK
int HapticsDev::start()
{
    // in Device::open(), mixer control "Haptics Source" is set to ach by the path "haptics-dev"
    // so if haptic source is a2h, set mixer control to a2h again.
    if (hapticSource == HAPTIC_SOURCE_A2H) {
        setHapticSource(hapticSource);
    }
    return Device::start();
}

int32_t HapticsDev::setParameter(uint32_t param_id, void *param)
{
    pal_param_haptic_source_t* param_haptic_source = (pal_param_haptic_source_t *)param;
    hapticSource = param_haptic_source->haptic_source;

    std::shared_ptr<Device> dev = nullptr;
    std::vector<Stream*> activestreams;
    dev = Device::getInstance(&deviceAttr, rm);
    int status = rm->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_VERBOSE(LOG_TAG, "no active stream available");
    } else {
        // if haptic device is active, set mixer control to switch haptic source
        setHapticSource(hapticSource);
    }
    return 0;
}

void HapticsDev::setHapticSource(haptic_source_t source)
{
    int ret = 0;
    struct mixer *mixer = NULL;
    struct mixer_ctl *ctl = NULL;
    char mixer_ctl_name[MIXER_PATH_MAX_LENGTH] = "Haptics Source";

    ret = rm->getHwAudioMixer(&mixer);
    if (ret) {
        PAL_ERR(LOG_TAG," mixer error");
        goto exit;
    }

    ctl = mixer_get_ctl_by_name(mixer, mixer_ctl_name);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Could not get ctl for mixer cmd - %s", mixer_ctl_name);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "Setting mixer control: Haptics Source, value: %d", (int)source);
    mixer_ctl_set_value(ctl, 0, (int)source);

exit:
    return;
}
#endif
