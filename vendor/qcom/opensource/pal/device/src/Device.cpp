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

#define LOG_TAG "PAL: Device"

#include "Device.h"
#include <tinyalsa/asoundlib.h>
#include "ResourceManager.h"
#include "SessionAlsaUtils.h"
#include "Device.h"
#include "Speaker.h"
#include "SpeakerProtection.h"
#include "Headphone.h"
#include "USBAudio.h"
#include "SpeakerMic.h"
#include "Stream.h"
#include "HeadsetMic.h"
#include "HandsetMic.h"
#include "HandsetVaMic.h"
#include "HeadsetVaMic.h"
#include "Handset.h"
#include "Bluetooth.h"
#include "DisplayPort.h"
#include "RTProxy.h"
#include "FMDevice.h"
#include "HapticsDev.h"
#include "UltrasoundDevice.h"
#include "ExtEC.h"
#include "ECRefDevice.h"
#include "DummyDev.h"
#ifdef SEC_AUDIO_CALL_SATELLITE
#include "ExtModem.h"
#include "ExtModemMic.h"
#endif
#ifdef SEC_AUDIO_MUTE_DETECTION
#include <audioInfo.h>
#endif
#include <dlfcn.h>

#ifdef ENABLE_TFA98XX_SUPPORT
#include "SpeakerProtectionTFA.h"
#elif ENABLE_TAS_SPK_PROT
#include "SpeakerProtectionTI.h"
#endif

#define MAX_CHANNEL_SUPPORTED 2
#define DEFAULT_OUTPUT_SAMPLING_RATE 48000
#define DEFAULT_OUTPUT_CHANNEL 2

typedef void (*write_qmp_mode)(const char *hdr_custom_key);

std::shared_ptr<Device> Device::getInstance(struct pal_device *device,
                                                 std::shared_ptr<ResourceManager> Rm)
{
    if (!device || !Rm) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return NULL;
    }

    PAL_VERBOSE(LOG_TAG, "Enter device id %d", device->id);

    //TBD: decide on supported devices from XML and not in code
    switch (device->id) {
    case PAL_DEVICE_NONE:
        PAL_DBG(LOG_TAG,"device none");
        return nullptr;
    case PAL_DEVICE_OUT_HANDSET:
        PAL_VERBOSE(LOG_TAG, "handset device");
        return Handset::getInstance(device, Rm);
    case PAL_DEVICE_OUT_SPEAKER:
        PAL_VERBOSE(LOG_TAG, "speaker device");
        return Speaker::getInstance(device, Rm);
    case PAL_DEVICE_IN_VI_FEEDBACK:
        PAL_VERBOSE(LOG_TAG, "speaker feedback device");
#ifdef ENABLE_TFA98XX_SUPPORT
        return SpeakerFeedbackTFA::getInstance(device, Rm);
#else
        return SpeakerFeedback::getInstance(device, Rm);
#endif
    case PAL_DEVICE_IN_CPS_FEEDBACK:
        PAL_VERBOSE(LOG_TAG, "speaker feedback device CPS");
        return SpeakerFeedback::getInstance(device, Rm);
    case PAL_DEVICE_OUT_WIRED_HEADSET:
    case PAL_DEVICE_OUT_WIRED_HEADPHONE:
        PAL_VERBOSE(LOG_TAG, "headphone device");
        return Headphone::getInstance(device, Rm);
    case PAL_DEVICE_OUT_USB_DEVICE:
    case PAL_DEVICE_OUT_USB_HEADSET:
    case PAL_DEVICE_IN_USB_DEVICE:
    case PAL_DEVICE_IN_USB_HEADSET:
        PAL_VERBOSE(LOG_TAG, "USB device");
        return USB::getInstance(device, Rm);
    case PAL_DEVICE_IN_HANDSET_MIC:
        PAL_VERBOSE(LOG_TAG, "HandsetMic device");
        return HandsetMic::getInstance(device, Rm);
    case PAL_DEVICE_IN_SPEAKER_MIC:
        PAL_VERBOSE(LOG_TAG, "speakerMic device");
        return SpeakerMic::getInstance(device, Rm);
    case PAL_DEVICE_IN_WIRED_HEADSET:
        PAL_VERBOSE(LOG_TAG, "HeadsetMic device");
        return HeadsetMic::getInstance(device, Rm);
    case PAL_DEVICE_IN_HANDSET_VA_MIC:
        PAL_VERBOSE(LOG_TAG, "HandsetVaMic device");
        return HandsetVaMic::getInstance(device, Rm);
    case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
    case PAL_DEVICE_OUT_BLUETOOTH_SCO:
        PAL_VERBOSE(LOG_TAG, "BTSCO device");
        return BtSco::getInstance(device, Rm);
    case PAL_DEVICE_IN_BLUETOOTH_A2DP:
    case PAL_DEVICE_OUT_BLUETOOTH_A2DP:
    case PAL_DEVICE_IN_BLUETOOTH_BLE:
    case PAL_DEVICE_OUT_BLUETOOTH_BLE:
    case PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST:
        PAL_VERBOSE(LOG_TAG, "BTA2DP device");
        return BtA2dp::getInstance(device, Rm);
    case PAL_DEVICE_OUT_AUX_DIGITAL:
    case PAL_DEVICE_OUT_AUX_DIGITAL_1:
    case PAL_DEVICE_OUT_HDMI:
        PAL_VERBOSE(LOG_TAG, "Display Port device");
        return DisplayPort::getInstance(device, Rm);
    case PAL_DEVICE_IN_HEADSET_VA_MIC:
        PAL_VERBOSE(LOG_TAG, "HeadsetVaMic device");
        return HeadsetVaMic::getInstance(device, Rm);
    case PAL_DEVICE_OUT_PROXY:
        PAL_VERBOSE(LOG_TAG, "RTProxyOut device");
        return RTProxyOut::getInstance(device, Rm);
    case PAL_DEVICE_OUT_RECORD_PROXY:
        PAL_VERBOSE(LOG_TAG, "RTProxyOut record device");
        return RTProxyOut::getInstance(device, Rm);
    case PAL_DEVICE_OUT_HEARING_AID:
        PAL_VERBOSE(LOG_TAG, "RTProxy Hearing Aid device");
        return RTProxyOut::getInstance(device, Rm);
    case PAL_DEVICE_OUT_HAPTICS_DEVICE:
        PAL_VERBOSE(LOG_TAG, "Haptics Device");
        return HapticsDev::getInstance(device, Rm);
    case PAL_DEVICE_IN_PROXY:
        PAL_VERBOSE(LOG_TAG, "RTProxyIn device");
        return RTProxyIn::getInstance(device, Rm);
    case PAL_DEVICE_IN_RECORD_PROXY:
        PAL_VERBOSE(LOG_TAG, "RTProxyIn record device");
        return RTProxyIn::getInstance(device, Rm);
    case PAL_DEVICE_IN_TELEPHONY_RX:
        PAL_VERBOSE(LOG_TAG, "RTProxy Telephony Rx device");
        return RTProxyIn::getInstance(device, Rm);
    case PAL_DEVICE_IN_FM_TUNER:
        PAL_VERBOSE(LOG_TAG, "FM device");
        return FMDevice::getInstance(device, Rm);
    case PAL_DEVICE_IN_ULTRASOUND_MIC:
    case PAL_DEVICE_OUT_ULTRASOUND:
    case PAL_DEVICE_OUT_ULTRASOUND_DEDICATED:
        PAL_VERBOSE(LOG_TAG, "Ultrasound device");
        return UltrasoundDevice::getInstance(device, Rm);
    case PAL_DEVICE_IN_EXT_EC_REF:
        PAL_VERBOSE(LOG_TAG, "ExtEC device");
        return ExtEC::getInstance(device, Rm);
    case PAL_DEVICE_IN_ECHO_REF:
        PAL_VERBOSE(LOG_TAG, "Echo ref device");
        return ECRefDevice::getInstance(device, Rm);
    case PAL_DEVICE_OUT_DUMMY:
    case PAL_DEVICE_IN_DUMMY:
        PAL_VERBOSE(LOG_TAG, "Dummy device");
        return DummyDev::getInstance(device, Rm);
#ifdef SEC_AUDIO_CALL_SATELLITE
    case PAL_DEVICE_OUT_EXT_MODEM:
        PAL_VERBOSE(LOG_TAG, "ExtModem device");
        return ExtModem::getInstance(device, Rm);
    case PAL_DEVICE_IN_EXT_MODEM_MIC:
        PAL_VERBOSE(LOG_TAG, "ExtModemMic device");
        return ExtModemMic::getInstance(device, Rm);
#endif
    default:
        PAL_ERR(LOG_TAG,"Unsupported device id %d",device->id);
        return nullptr;
    }
}

std::shared_ptr<Device> Device::getObject(pal_device_id_t dev_id)
{

    switch(dev_id) {
    case PAL_DEVICE_NONE:
        PAL_DBG(LOG_TAG,"device none");
        return nullptr;
    case PAL_DEVICE_OUT_HANDSET:
        PAL_VERBOSE(LOG_TAG, "handset device");
        return Handset::getObject();
    case PAL_DEVICE_OUT_SPEAKER:
        PAL_VERBOSE(LOG_TAG, "speaker device");
        return Speaker::getObject();
    case PAL_DEVICE_IN_VI_FEEDBACK:
        PAL_VERBOSE(LOG_TAG, "speaker feedback device");
#ifdef ENABLE_TFA98XX_SUPPORT
        return SpeakerFeedbackTFA::getObject();
#else
        return SpeakerFeedback::getObject();
#endif
        return SpeakerFeedback::getObject();
    case PAL_DEVICE_OUT_WIRED_HEADSET:
    case PAL_DEVICE_OUT_WIRED_HEADPHONE:
        PAL_VERBOSE(LOG_TAG, "headphone device");
        return Headphone::getObject(dev_id);
    case PAL_DEVICE_OUT_USB_DEVICE:
    case PAL_DEVICE_OUT_USB_HEADSET:
    case PAL_DEVICE_IN_USB_DEVICE:
    case PAL_DEVICE_IN_USB_HEADSET:
        PAL_VERBOSE(LOG_TAG, "USB device");
        return USB::getObject(dev_id);
    case PAL_DEVICE_OUT_AUX_DIGITAL:
    case PAL_DEVICE_OUT_AUX_DIGITAL_1:
    case PAL_DEVICE_OUT_HDMI:
        PAL_VERBOSE(LOG_TAG, "Display Port device");
        return DisplayPort::getObject(dev_id);
    case PAL_DEVICE_IN_HANDSET_MIC:
        PAL_VERBOSE(LOG_TAG, "handset mic device");
        return HandsetMic::getObject();
    case PAL_DEVICE_IN_SPEAKER_MIC:
        PAL_VERBOSE(LOG_TAG, "speaker mic device");
        return SpeakerMic::getObject();
    case PAL_DEVICE_IN_WIRED_HEADSET:
        PAL_VERBOSE(LOG_TAG, "headset mic device");
        return HeadsetMic::getObject();
    case PAL_DEVICE_OUT_BLUETOOTH_A2DP:
    case PAL_DEVICE_IN_BLUETOOTH_A2DP:
    case PAL_DEVICE_OUT_BLUETOOTH_BLE:
    case PAL_DEVICE_IN_BLUETOOTH_BLE:
    case PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST:
        PAL_VERBOSE(LOG_TAG, "BT A2DP device %d", dev_id);
        return BtA2dp::getObject(dev_id);
    case PAL_DEVICE_OUT_BLUETOOTH_SCO:
    case PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET:
        PAL_VERBOSE(LOG_TAG, "BT SCO device %d", dev_id);
        return BtSco::getObject(dev_id);
    case PAL_DEVICE_OUT_PROXY:
    case PAL_DEVICE_OUT_HEARING_AID:
    case PAL_DEVICE_OUT_RECORD_PROXY:
        PAL_VERBOSE(LOG_TAG, "RTProxyOut device %d", dev_id);
        return RTProxyOut::getObject(dev_id);
    case PAL_DEVICE_IN_PROXY:
    case PAL_DEVICE_IN_TELEPHONY_RX:
    case PAL_DEVICE_IN_RECORD_PROXY:
        PAL_VERBOSE(LOG_TAG, "RTProxy device %d", dev_id);
        return RTProxyIn::getObject(dev_id);
    case PAL_DEVICE_IN_FM_TUNER:
        PAL_VERBOSE(LOG_TAG, "FMDevice %d", dev_id);
        return FMDevice::getObject();
    case PAL_DEVICE_IN_ULTRASOUND_MIC:
    case PAL_DEVICE_OUT_ULTRASOUND:
    case PAL_DEVICE_OUT_ULTRASOUND_DEDICATED:
        PAL_VERBOSE(LOG_TAG, "Ultrasound device %d", dev_id);
        return UltrasoundDevice::getObject(dev_id);
    case PAL_DEVICE_IN_EXT_EC_REF:
        PAL_VERBOSE(LOG_TAG, "ExtEC device %d", dev_id);
        return ExtEC::getObject();
    case PAL_DEVICE_IN_HANDSET_VA_MIC:
        PAL_VERBOSE(LOG_TAG, "Handset VA Mic device %d", dev_id);
        return HandsetVaMic::getObject();
    case PAL_DEVICE_IN_HEADSET_VA_MIC:
        PAL_VERBOSE(LOG_TAG, "Headset VA Mic device %d", dev_id);
        return HeadsetVaMic::getObject();
    case PAL_DEVICE_IN_ECHO_REF:
        PAL_VERBOSE(LOG_TAG, "Echo ref device %d", dev_id);
        return ECRefDevice::getObject();
    case PAL_DEVICE_OUT_HAPTICS_DEVICE:
        PAL_VERBOSE(LOG_TAG, "Haptics device %d", dev_id);
        return HapticsDev::getObject();
    case PAL_DEVICE_OUT_DUMMY:
    case PAL_DEVICE_IN_DUMMY:
        PAL_VERBOSE(LOG_TAG, "Dummy device %d", dev_id);
        return DummyDev::getObject(dev_id);
#ifdef SEC_AUDIO_CALL_SATELLITE
    case PAL_DEVICE_OUT_EXT_MODEM:
        PAL_VERBOSE(LOG_TAG, "ExtModem device %d", dev_id);
        return ExtModem::getObject();
    case PAL_DEVICE_IN_EXT_MODEM_MIC:
        PAL_VERBOSE(LOG_TAG, "ExtModemMic device %d", dev_id);
        return ExtModemMic::getObject();
#endif
    default:
        PAL_ERR(LOG_TAG,"Unsupported device id %d",dev_id);
        return nullptr;
    }
}

int32_t Device::initHdrRoutine(const char *hdr_custom_key)
{
    void *handle = NULL;
    handle = dlopen("vendor/lib64/libqmp.so", RTLD_NOW);
    if (!handle) {
        PAL_ERR(LOG_TAG, "Failed to open libqmp.so");
    }

   write_qmp_mode write = reinterpret_cast<write_qmp_mode>(dlsym(handle, "write_qmp_mode"));
   if (!write) {
       dlclose(handle);
       return -1;
   }
   write(hdr_custom_key);
   dlclose(handle);
   return 0;
}

Device::Device(struct pal_device *device, std::shared_ptr<ResourceManager> Rm)
{
    rm = Rm;
    rm->getHwAudioMixer(&hwMixerHandle);
    rm->getVirtualAudioMixer(&virtualMixerHandle);
    memset(&deviceAttr, 0, sizeof(struct pal_device));
    ar_mem_cpy(&deviceAttr, sizeof(struct pal_device), device,
                     sizeof(struct pal_device));

    mPALDeviceName.clear();
    customPayload = NULL;
    customPayloadSize = 0;
    strlcpy(mSndDeviceName, "", DEVICE_NAME_MAX_SIZE);
    mCurrentPriority = MIN_USECASE_PRIORITY;
    PAL_DBG(LOG_TAG,"device instance for id %d created", device->id);

}

Device::Device()
{
    strlcpy(mSndDeviceName, "", DEVICE_NAME_MAX_SIZE);
    mCurrentPriority = MIN_USECASE_PRIORITY;
    mPALDeviceName.clear();
}

Device::~Device()
{
    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;
    mCurrentPriority = MIN_USECASE_PRIORITY;
    PAL_DBG(LOG_TAG,"device instance for id %d destroyed", deviceAttr.id);
}

int Device::getDeviceAttributes(struct pal_device *dattr, Stream* streamHandle)
{
    struct pal_device *strDevAttr;

    if (!dattr) {
        PAL_ERR(LOG_TAG, "Invalid device attributes");
        return  -EINVAL;
    }

    ar_mem_cpy(dattr, sizeof(struct pal_device),
            &deviceAttr, sizeof(struct pal_device));

    /* overwrite custom key if stream is specified */
    mDeviceMutex.lock();
    if (streamHandle != NULL) {
        if (mStreamDevAttr.empty()) {
            PAL_DBG(LOG_TAG, "no device attr for associated streams for dev %d", getSndDeviceId());
            mDeviceMutex.unlock();
            return 0;
        }
        for (auto it = mStreamDevAttr.begin(); it != mStreamDevAttr.end(); ++it) {
            Stream* curStream = (*it).second.first;
            if (curStream == streamHandle) {
                PAL_DBG(LOG_TAG,"found entry for stream: %pK", streamHandle);
                strDevAttr = (*it).second.second;
                strlcpy(dattr->custom_config.custom_key, strDevAttr->custom_config.custom_key,
                        PAL_MAX_CUSTOM_KEY_SIZE);
                break;
            }
        }
    }
    mDeviceMutex.unlock();

    return 0;
}

int Device::getCodecConfig(struct pal_media_config *config __unused)
{
    return 0;
}


int Device::getDefaultConfig(pal_param_device_capability_t capability __unused)
{
    return 0;
}

int Device::setDeviceAttributes(struct pal_device &dattr)
{
    int status = 0;

    mDeviceMutex.lock();
    PAL_INFO(LOG_TAG,"DeviceAttributes for Device Id %d updated", dattr.id);
    ar_mem_cpy(&deviceAttr, sizeof(struct pal_device), &dattr,
                     sizeof(struct pal_device));

    mDeviceMutex.unlock();
    return status;
}

int Device::freeCustomPayload(uint8_t **payload, size_t *payloadSize)
{
    if (*payload) {
        free(*payload);
        *payload = NULL;
        *payloadSize = 0;
    }
    return 0;
}

int Device::updateCustomPayload(void *payload, size_t size)
{
    if (!customPayloadSize) {
        customPayload = calloc(1, size);
    } else {
        customPayload = realloc(customPayload, customPayloadSize + size);
    }

    if (!customPayload) {
        PAL_ERR(LOG_TAG, "failed to allocate memory for custom payload");
        return -ENOMEM;
    }

    memcpy((uint8_t *)customPayload + customPayloadSize, payload, size);
    customPayloadSize += size;
    PAL_INFO(LOG_TAG, "customPayloadSize = %zu", customPayloadSize);
    return 0;
}

void* Device::getCustomPayload()
{
    return customPayload;
}

size_t Device::getCustomPayloadSize()
{
    return customPayloadSize;
}

int Device::getSndDeviceId()
{
    PAL_VERBOSE(LOG_TAG,"Device Id %d acquired", deviceAttr.id);
    return deviceAttr.id;
}

void Device::getCurrentSndDevName(char *name){
    strlcpy(name, mSndDeviceName, DEVICE_NAME_MAX_SIZE);
}

std::string Device::getPALDeviceName()
{
    PAL_VERBOSE(LOG_TAG, "Device name %s acquired", mPALDeviceName.c_str());
    return mPALDeviceName;
}

int Device::init(pal_param_device_connection_t device_conn __unused)
{
    return 0;
}

int Device::deinit(pal_param_device_connection_t device_conn __unused)
{
    return 0;
}

int Device::open()
{
    int status = 0;

    mDeviceMutex.lock();
    mPALDeviceName = rm->getPALDeviceName(this->deviceAttr.id);
    PAL_INFO(LOG_TAG, "Enter. deviceCount %d for device id %d (%s)", deviceCount,
            this->deviceAttr.id, mPALDeviceName.c_str());
#ifdef SEC_AUDIO_MUTE_DETECTION
    if(deviceCount == 0) {
        android::String8 deviceLog = android::String8::format("Enter. deviceCount %d for device id %d (%s)", deviceCount,
            this->deviceAttr.id, mPALDeviceName.c_str());
        android::AudioInfo::GetInstance()->setSecSaveAudioLog(deviceLog);
    }
#endif

    devObj = Device::getInstance(&deviceAttr, rm);

    if (deviceCount == 0) {
        std::string backEndName;
        rm->getBackendName(this->deviceAttr.id, backEndName);
        if (strlen(backEndName.c_str())) {
            SessionAlsaUtils::setDeviceMediaConfig(rm, backEndName, &(this->deviceAttr));
        }

        status = rm->getAudioRoute(&audioRoute);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Failed to get the audio_route address status %d", status);
            goto exit;
        }
        status = rm->getSndDeviceName(deviceAttr.id , mSndDeviceName); //getsndName

        if (!UpdatedSndName.empty()) {
            PAL_DBG(LOG_TAG,"Update sndName %s, currently %s",
                    UpdatedSndName.c_str(), mSndDeviceName);
            strlcpy(mSndDeviceName, UpdatedSndName.c_str(), DEVICE_NAME_MAX_SIZE);
        }

        PAL_DBG(LOG_TAG, "audio_route %pK SND device name %s", audioRoute, mSndDeviceName);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Failed to obtain the device name from ResourceManager status %d", status);
            goto exit;
        }

        if (ResourceManager::isQmpEnabled) {
            if (strstr(this->deviceAttr.custom_config.custom_key, "unprocessed-hdr-mic")){
                if (Device::initHdrRoutine(this->deviceAttr.custom_config.custom_key))
                    PAL_ERR(LOG_TAG, "Failed to set QMP hdr config");
            }
        }
        enableDevice(audioRoute, mSndDeviceName);
    }
    ++deviceCount;

exit:
    PAL_INFO(LOG_TAG, "Exit. deviceCount %d for device id %d (%s), exit status: %d", deviceCount,
            this->deviceAttr.id, mPALDeviceName.c_str(), status);
    mDeviceMutex.unlock();
    return status;
}

int Device::close()
{
    int status = 0;
    mDeviceMutex.lock();
    PAL_INFO(LOG_TAG, "Enter. deviceCount %d for device id %d (%s)", deviceCount,
            this->deviceAttr.id, mPALDeviceName.c_str());
    if (deviceCount > 0) {
        --deviceCount;

       if (deviceCount == 0) {
           PAL_DBG(LOG_TAG, "Disabling device %d with snd dev %s", deviceAttr.id, mSndDeviceName);
           disableDevice(audioRoute, mSndDeviceName);
           mCurrentPriority = MIN_USECASE_PRIORITY;
           deviceStartStopCount = 0;
           if(rm->num_proxy_channels != 0)
               rm->num_proxy_channels = 0;
       }
    }
    PAL_INFO(LOG_TAG, "Exit. deviceCount %d for device id %d (%s), exit status %d", deviceCount,
            this->deviceAttr.id, mPALDeviceName.c_str(), status);

#ifdef SEC_AUDIO_MUTE_DETECTION
    if(deviceCount == 0) {
        android::String8 deviceLog = android::String8::format("Exit. deviceCount %d for device id %d (%s), exit status %d", deviceCount,
            this->deviceAttr.id, mPALDeviceName.c_str(), status);
        android::AudioInfo::GetInstance()->setSecSaveAudioLog(deviceLog);
    }
#endif

    mDeviceMutex.unlock();
    return status;
}

int Device::prepare()
{
    return 0;
}

int Device::start()
{
    int status = 0;

    mDeviceMutex.lock();
    status = start_l();
    mDeviceMutex.unlock();

    return status;
}

// must be called with mDeviceMutex held
int Device::start_l()
{
    int status = 0;
    std::string backEndName;

    PAL_DBG(LOG_TAG, "Enter. deviceCount %d deviceStartStopCount %d"
        " for device id %d (%s)", deviceCount, deviceStartStopCount,
            this->deviceAttr.id, mPALDeviceName.c_str());
    if (0 == deviceStartStopCount) {
        rm->getBackendName(this->deviceAttr.id, backEndName);
        if (!strlen(backEndName.c_str())) {
            PAL_ERR(LOG_TAG, "Error: Backend name not defined for %d in xml file\n", this->deviceAttr.id);
            status = -EINVAL;
            goto exit;
        }
        if (rm->isPluginPlaybackDevice(this->deviceAttr.id) || rm->isDpDevice(this->deviceAttr.id)) {
            /* avoid setting invalid device attribute and the failure of starting device
             * when plugin device disconnects. Audio Policy Manager will go on finishing device switch.
             */
            if (this->deviceAttr.config.sample_rate == 0) {
                PAL_DBG(LOG_TAG, "overwrite samplerate to default value");
                this->deviceAttr.config.sample_rate = DEFAULT_OUTPUT_SAMPLING_RATE;
            }
            if (this->deviceAttr.config.bit_width == 0) {
                PAL_DBG(LOG_TAG, "overwrite bit width to default value");
                this->deviceAttr.config.bit_width = 16;
            }
            if (this->deviceAttr.config.ch_info.channels == 0) {
                PAL_DBG(LOG_TAG, "overwrite channel to default value");
                this->deviceAttr.config.ch_info.channels = DEFAULT_OUTPUT_CHANNEL;
                this->deviceAttr.config.ch_info.ch_map[0] = PAL_CHMAP_CHANNEL_FL;
                this->deviceAttr.config.ch_info.ch_map[1] = PAL_CHMAP_CHANNEL_FR;
            }
            if (this->deviceAttr.config.aud_fmt_id == 0) {
                PAL_DBG(LOG_TAG, "overwrite aud_fmt_id to default value");
                this->deviceAttr.config.aud_fmt_id = PAL_AUDIO_FMT_PCM_S16_LE;
            }
        }
        SessionAlsaUtils::setDeviceMediaConfig(rm, backEndName, &(this->deviceAttr));

        if (customPayloadSize) {
            status = SessionAlsaUtils::setDeviceCustomPayload(rm, backEndName,
                                        customPayload, customPayloadSize);
            if (status)
                 PAL_ERR(LOG_TAG, "Error: Dev setParam failed for %d\n",
                                   this->deviceAttr.id);
        }
    }
    deviceStartStopCount++;
exit :
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int Device::stop()
{
    int status = 0;

    mDeviceMutex.lock();
    status = stop_l();
    mDeviceMutex.unlock();

    return status;
}

// must be called with mDeviceMutex held
int Device::stop_l()
{
    PAL_DBG(LOG_TAG, "Enter. deviceCount %d deviceStartStopCount %d"
        " for device id %d (%s)", deviceCount, deviceStartStopCount,
            this->deviceAttr.id, mPALDeviceName.c_str());

    if (deviceStartStopCount > 0) {
        --deviceStartStopCount;
    }

    return 0;
}

int32_t Device::setDeviceParameter(uint32_t param_id __unused, void *param __unused)
{
    return 0;
}

int32_t Device::getDeviceParameter(uint32_t param_id __unused, void **param __unused)
{
    return 0;
}

int32_t Device::setParameter(uint32_t param_id __unused, void *param __unused)
{
    return 0;
}

int32_t Device::getParameter(uint32_t param_id __unused, void **param __unused)
{
    return 0;
}

int32_t Device::configureDeviceClockSrc(char const *mixerStrClockSrc, const uint32_t clockSrc)
{
    struct mixer *hwMixerHandle = NULL;
    struct mixer_ctl *clockSrcCtrl = NULL;
    int32_t ret = 0;

    if (!mixerStrClockSrc) {
        PAL_ERR(LOG_TAG, "Invalid argument - mixerStrClockSrc");
        ret = -EINVAL;
        goto exit;
    }

    ret = rm->getHwAudioMixer(&hwMixerHandle);
    if (ret) {
        PAL_ERR(LOG_TAG, "getHwAudioMixer() failed %d", ret);
        goto exit;
    }

    /* Hw mixer control registration is optional in case
     * clock source selection is not required
     */
    clockSrcCtrl = mixer_get_ctl_by_name(hwMixerHandle, mixerStrClockSrc);
    if (!clockSrcCtrl) {
        PAL_DBG(LOG_TAG, "%s hw mixer control not identified", mixerStrClockSrc);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "HwMixer set %s = %d", mixerStrClockSrc, clockSrc);
    ret = mixer_ctl_set_value(clockSrcCtrl, 0, clockSrc);
    if (ret)
        PAL_ERR(LOG_TAG, "HwMixer set %s = %d failed", mixerStrClockSrc, clockSrc);

exit:
    return ret;
}

/* insert inDevAttr if incoming device has higher priority */
bool Device::compareStreamDevAttr(const struct pal_device *inDevAttr,
                            const struct pal_device_info *inDevInfo,
                            struct pal_device *curDevAttr,
                            const struct pal_device_info *curDevInfo)
{
    bool insert = false;

    if (!inDevAttr || !inDevInfo || !curDevAttr || !curDevInfo) {
        PAL_ERR(LOG_TAG, "invalid pointer cannot update attr");
        goto exit;
    }

     /* check snd device name */
    if (inDevInfo->sndDevName_overwrite && !curDevInfo->sndDevName_overwrite) {
        PAL_DBG(LOG_TAG, "snd overwrite found");
        insert = true;
        goto exit;
    }

#ifdef SEC_AUDIO_COMMON
    // device selection is based on stream priority from SM8550.
    // but, need to switch by custom key in the scenario of same priority.
    // refer to P220930-10662 & FRAMEWORKG-100677.
    if ((inDevInfo->priority == curDevInfo->priority) && (strcmp(inDevAttr->sndDevName, curDevAttr->sndDevName) != 0)) {
        PAL_INFO(LOG_TAG, "snd overwrite found with same priority");
        insert = true;
        goto exit;
    }
#endif

    /* check channels */
    if (inDevInfo->channels_overwrite && !curDevInfo->channels_overwrite) {
        PAL_DBG(LOG_TAG, "ch overwrite found");
        insert = true;
        goto exit;
    } else if ((inDevInfo->channels_overwrite && curDevInfo->channels_overwrite) ||
               (!inDevInfo->channels_overwrite && !curDevInfo->channels_overwrite)) {
        if (inDevAttr->config.ch_info.channels > curDevAttr->config.ch_info.channels) {
            PAL_DBG(LOG_TAG, "incoming dev has higher ch count, in ch: %d, cur ch: %d",
                            inDevAttr->config.ch_info.channels, curDevAttr->config.ch_info.channels);
            insert = true;
            goto exit;
        }
    }

    /* check sample rate */
    if (inDevInfo->samplerate_overwrite && !curDevInfo->samplerate_overwrite) {
        PAL_DBG(LOG_TAG, "sample rate overwrite found");
        insert = true;
        goto exit;
    } else if ((inDevInfo->samplerate_overwrite && curDevInfo->samplerate_overwrite) &&
               (inDevAttr->config.sample_rate > curDevAttr->config.sample_rate)) {
        PAL_DBG(LOG_TAG, "both have sr overwrite set, incoming dev has higher sr: %d, cur sr: %d",
                        inDevAttr->config.sample_rate, curDevAttr->config.sample_rate);
        insert = true;
        goto exit;
    } else if (!inDevInfo->samplerate_overwrite && !curDevInfo->samplerate_overwrite) {
        if ((inDevAttr->config.sample_rate % SAMPLINGRATE_44K == 0) &&
            (curDevAttr->config.sample_rate % SAMPLINGRATE_44K != 0)) {
            PAL_DBG(LOG_TAG, "incoming sample rate is 44.1K");
            insert = true;
            goto exit;
        } else if (inDevAttr->config.sample_rate > curDevAttr->config.sample_rate) {
            if (curDevAttr->config.sample_rate % SAMPLINGRATE_44K == 0 &&
                inDevAttr->config.sample_rate % SAMPLINGRATE_48K == 0) {
                PAL_DBG(LOG_TAG, "current stream is running at 44.1KHz");
                insert = false;
            } else {
                PAL_DBG(LOG_TAG, "incoming dev has higher sr: %d, cur sr: %d",
                            inDevAttr->config.sample_rate, curDevAttr->config.sample_rate);
                insert = true;
                goto exit;
            }
        }
    }

    /* check streams bit width */
    if (inDevInfo->bit_width_overwrite && !curDevInfo->bit_width_overwrite) {
        if (isPalPCMFormat(inDevAttr->config.aud_fmt_id)) {
            PAL_DBG(LOG_TAG, "bit width overwrite found");
            insert = true;
            goto exit;
        }
    } else if ((inDevInfo->bit_width_overwrite && curDevInfo->bit_width_overwrite) ||
               (!inDevInfo->bit_width_overwrite && !curDevInfo->bit_width_overwrite)) {
        if (isPalPCMFormat(inDevAttr->config.aud_fmt_id) &&
            (inDevAttr->config.bit_width > curDevAttr->config.bit_width)) {
            PAL_DBG(LOG_TAG, "incoming dev has higher bw: %d, cur bw: %d",
                            inDevAttr->config.bit_width, curDevAttr->config.bit_width);
            insert = true;
            goto exit;
        }
    }

exit:
    return insert;
}

int Device::insertStreamDeviceAttr(struct pal_device *inDevAttr,
                                 Stream* streamHandle)
{
    pal_device_info inDevInfo, curDevInfo;
    struct pal_device *curDevAttr, *newDevAttr;
    std::string key = "";
    pal_stream_attributes strAttr;

    if (!streamHandle) {
        PAL_ERR(LOG_TAG, "invalid stream handle");
        return -EINVAL;
    }
    if (!inDevAttr) {
        PAL_ERR(LOG_TAG, "invalid dev cannot get device attr");
        return -EINVAL;
    }

    streamHandle->getStreamAttributes(&strAttr);

    newDevAttr = (struct pal_device *) calloc(1, sizeof(struct pal_device));
    if (!newDevAttr) {
        PAL_ERR(LOG_TAG, "failed to allocate memory for pal device");
        return -ENOMEM;
    }

    key = inDevAttr->custom_config.custom_key;

    /* get the incoming stream dev info */
    rm->getDeviceInfo(inDevAttr->id, strAttr.type, key, &inDevInfo);

    ar_mem_cpy(newDevAttr, sizeof(struct pal_device), inDevAttr,
                 sizeof(struct pal_device));

    mDeviceMutex.lock();
    if (mStreamDevAttr.empty()) {
        mStreamDevAttr.insert(std::make_pair(inDevInfo.priority, std::make_pair(streamHandle, newDevAttr)));
        PAL_DBG(LOG_TAG, "insert the first device attribute");
        goto exit;
    }

    /*
     * this map is sorted with stream priority, and the top one will always
     * be with highest stream priority.
     * <priority(it.first):<stream_attr(it.second.first):pal_device(it.second.second)>>
     * If stream priority is the same, new attributes will be inserted to the map with:
     *   1. device attr with snd name overwrite flag set
     *   2. device attr with channel overwrite set, or a higher channel count
     *   3. device attr with sample rate overwrite set, or a higher sampling rate
     *   4. device attr with bit depth overwrite set, or a higher bit depth
     */
    for (auto it = mStreamDevAttr.begin(); ; it++) {
        /* get the current stream dev info to be compared with incoming device */
        struct pal_stream_attributes curStrAttr;
        if (it != mStreamDevAttr.end()) {
            (*it).second.first->getStreamAttributes(&curStrAttr);
            curDevAttr = (*it).second.second;
            rm->getDeviceInfo(curDevAttr->id, curStrAttr.type,
                            curDevAttr->custom_config.custom_key, &curDevInfo);
        }

        if (it == mStreamDevAttr.end()) {
            /* if reaches to the end, then the new dev attr will be inserted to the end */
            PAL_DBG(LOG_TAG, "incoming stream: %d has lowest priority, insert to the end", strAttr.type);
            mStreamDevAttr.insert(std::make_pair(inDevInfo.priority, std::make_pair(streamHandle, newDevAttr)));
            break;
        } else if (inDevInfo.priority < (*it).first) {
            /* insert if incoming stream has higher priority than current */
            mStreamDevAttr.insert(it, std::make_pair(inDevInfo.priority, std::make_pair(streamHandle, newDevAttr)));
            break;
        } else if (inDevInfo.priority == (*it).first) {
            /* if stream priority is the same, check attributes priority */
            if (compareStreamDevAttr(inDevAttr, &inDevInfo, curDevAttr, &curDevInfo)) {
                PAL_DBG(LOG_TAG, "incoming stream: %d has higher priority than cur stream %d",
                                strAttr.type, curStrAttr.type);
                mStreamDevAttr.insert(it, std::make_pair(inDevInfo.priority, std::make_pair(streamHandle, newDevAttr)));
                break;
            }
        }
    }

exit:
    PAL_DBG(LOG_TAG, "dev: %d attr inserted are: priority: 0x%x, stream type: %d, ch: %d,"
                     " sr: %d, bit_width: %d, fmt: %d, sndDev: %s, custom_key: %s",
                    getSndDeviceId(), inDevInfo.priority, strAttr.type,
                    newDevAttr->config.ch_info.channels,
                    newDevAttr->config.sample_rate,
                    newDevAttr->config.bit_width,
                    newDevAttr->config.aud_fmt_id,
                    newDevAttr->sndDevName,
                    newDevAttr->custom_config.custom_key);

#if DUMP_DEV_ATTR
    PAL_DBG(LOG_TAG, "======dump StreamDevAttr Inserted dev: %d ======", getSndDeviceId());
    int i = 0;
    for (auto it = mStreamDevAttr.begin(); it != mStreamDevAttr.end(); it++) {
        pal_stream_attributes dumpstrAttr;
        uint32_t dumpPriority = (*it).first;
        (*it).second.first->getStreamAttributes(&dumpstrAttr);
        struct pal_device *dumpDevAttr = (*it).second.second;
        PAL_DBG(LOG_TAG, "entry: %d", i);
        PAL_DBG(LOG_TAG, "str pri: 0x%x, str type: %d, ch %d, sr %d, bit_width %d,"
                         " fmt %d, sndDev: %s, custom_key: %s",
                         dumpPriority, dumpstrAttr.type,
                         dumpDevAttr->config.ch_info.channels,
                         dumpDevAttr->config.sample_rate,
                         dumpDevAttr->config.bit_width,
                         dumpDevAttr->config.aud_fmt_id,
                         dumpDevAttr->sndDevName,
                         dumpDevAttr->custom_config.custom_key);
        i++;
    }
#endif

    mDeviceMutex.unlock();
    return 0;
}

void Device::removeStreamDeviceAttr(Stream* streamHandle)
{
    mDeviceMutex.lock();
    if (mStreamDevAttr.empty()) {
        PAL_ERR(LOG_TAG, "empty device attr for device %d", getSndDeviceId());
        mDeviceMutex.unlock();
        return;
    }

    for (auto it = mStreamDevAttr.begin(); it != mStreamDevAttr.end(); it++) {
        Stream* curStream = (*it).second.first;
        if (curStream == streamHandle) {
            uint32_t priority = (*it).first;
            pal_stream_attributes strAttr;
            (*it).second.first->getStreamAttributes(&strAttr);
            pal_device *devAttr = (*it).second.second;
            PAL_DBG(LOG_TAG, "found entry for stream:%d", strAttr.type);
            PAL_DBG(LOG_TAG, "dev: %d attr removed are: priority: 0x%x, stream type: %d, ch: %d,"
                             " sr: %d, bit_width: %d, fmt: %d, sndDev: %s, custom_key: %s",
                            getSndDeviceId(), priority, strAttr.type,
                            devAttr->config.ch_info.channels,
                            devAttr->config.sample_rate,
                            devAttr->config.bit_width,
                            devAttr->config.aud_fmt_id,
                            devAttr->sndDevName,
                            devAttr->custom_config.custom_key);
            free((*it).second.second);
            mStreamDevAttr.erase(it);
            break;
        }
    }

#if DUMP_DEV_ATTR
    PAL_DBG(LOG_TAG, "=====dump StreamDevAttr after removing dev: %d ======", getSndDeviceId());
    int i = 0;
    for (auto it = mStreamDevAttr.begin(); it != mStreamDevAttr.end(); it++) {
        pal_stream_attributes dumpstrAttr;
        uint32_t dumpPriority = (*it).first;
        (*it).second.first->getStreamAttributes(&dumpstrAttr);
        struct pal_device *dumpDevAttr = (*it).second.second;
        PAL_DBG(LOG_TAG, "entry: %d", i);
        PAL_DBG(LOG_TAG, "str pri: 0x%x, str type: %d, ch %d, sr %d, bit_width %d,"
                         " fmt %d, sndDev: %s, custom_key: %s",
                         dumpPriority, dumpstrAttr.type,
                         dumpDevAttr->config.ch_info.channels,
                         dumpDevAttr->config.sample_rate,
                         dumpDevAttr->config.bit_width,
                         dumpDevAttr->config.aud_fmt_id,
                         dumpDevAttr->sndDevName,
                         dumpDevAttr->custom_config.custom_key);
        i++;
    }
#endif
    mDeviceMutex.unlock();
}

int Device::getTopPriorityDeviceAttr(struct pal_device *deviceAttr, uint32_t *streamPrio)
{
    mDeviceMutex.lock();
    if (mStreamDevAttr.empty()) {
        PAL_ERR(LOG_TAG, "empty device attr for device %d", getSndDeviceId());
        mDeviceMutex.unlock();
        return -EINVAL;
    }

    auto it = mStreamDevAttr.begin();
    *streamPrio = (*it).first;
    ar_mem_cpy(deviceAttr, sizeof(struct pal_device),
            (*it).second.second, sizeof(struct pal_device));
    /* update snd dev name */
    std::string sndDevName(deviceAttr->sndDevName);
    rm->updateSndName(deviceAttr->id, sndDevName);
    /* update sample rate if it's valid */
    if (mSampleRate)
        deviceAttr->config.sample_rate = mSampleRate;
    if (mBitWidth) {
        deviceAttr->config.bit_width = mBitWidth;
        deviceAttr->config.aud_fmt_id = rm->getAudioFmt(mBitWidth);
    }
#if DUMP_DEV_ATTR
    pal_stream_attributes dumpstrAttr;
    (*it).second.first->getStreamAttributes(&dumpstrAttr);
    PAL_DBG(LOG_TAG, "======dump StreamDevAttr Retrieved dev: %d ======", getSndDeviceId());
    PAL_DBG(LOG_TAG, "str pri: 0x%x, str type: %d, ch %d, sr %d, bit_width %d,"
                     " fmt %d, sndDev: %s, custom_key: %s",
                     (*it).first, dumpstrAttr.type,
                     deviceAttr->config.ch_info.channels,
                     deviceAttr->config.sample_rate,
                     deviceAttr->config.bit_width,
                     deviceAttr->config.aud_fmt_id,
                     deviceAttr->sndDevName,
                     deviceAttr->custom_config.custom_key);
#endif

    mDeviceMutex.unlock();
    return 0;
}
