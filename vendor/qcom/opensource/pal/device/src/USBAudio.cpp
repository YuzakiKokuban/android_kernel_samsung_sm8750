/*
 * Copyright (c) 2016, 2018-2021, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.

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
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#define LOG_TAG "PAL: USB"
#include "USBAudio.h"

#include <cstdio>
#include <cmath>
#include "USBAudio.h"
#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include "Device.h"
#include "kvh2xml.h"
#include <unistd.h>
#include <fstream>

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
android::SimpleLog UsbProfileLog{5};
std::vector<std::pair<int, int>> usbvidpid;
#endif
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
struct pal_usb_device_address usbAddress_ = {0,};
#endif

std::shared_ptr<Device> USB::objRx = nullptr;
std::shared_ptr<Device> USB::objTx = nullptr;

std::shared_ptr<Device> USB::getInstance(struct pal_device *device,
                                             std::shared_ptr<ResourceManager> Rm)
{
    if (!device)
       return NULL;

    if ((device->id == PAL_DEVICE_OUT_USB_DEVICE) ||
        (device->id == PAL_DEVICE_OUT_USB_HEADSET)){
        if (!objRx) {
            std::shared_ptr<Device> sp(new USB(device, Rm));
            objRx = sp;
        }
        return objRx;
    } else if ((device->id == PAL_DEVICE_IN_USB_DEVICE) ||
               (device->id == PAL_DEVICE_IN_USB_HEADSET)){
        if (!objTx) {
            std::shared_ptr<Device> sp(new USB(device, Rm));
            objTx = sp;
        }
        return objTx;
    }

    return NULL;
}

std::shared_ptr<Device> USB::getObject(pal_device_id_t id)
{
    if ((id == PAL_DEVICE_OUT_USB_DEVICE) ||
        (id == PAL_DEVICE_OUT_USB_HEADSET)) {
        if (objRx) {
            if (objRx->getSndDeviceId() == id)
                return objRx;
        }
    } else if ((id == PAL_DEVICE_IN_USB_DEVICE) ||
               (id == PAL_DEVICE_IN_USB_HEADSET)) {
        if (objTx) {
            if (objTx->getSndDeviceId() == id)
                return objTx;
        }
    }
    return NULL;
}

USB::USB(struct pal_device *device, std::shared_ptr<ResourceManager> Rm) :
Device(device, Rm)
{
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    usbvidpid.clear();
#endif
#ifdef SEC_AUDIO_FMRADIO
    fm_enabled = false;
#endif
}

USB::~USB()
{
    PAL_INFO(LOG_TAG, "dtor called");
}

int USB::start()
{
    int status = 0;

    if (customPayload)
        free(customPayload);

    customPayload = NULL;
    customPayloadSize = 0;

    status = configureUsb();
    if (status != 0) {
        PAL_ERR(LOG_TAG,"USB Endpoint Configuration Failed");
        return status;
    }

    status = Device::start();
    return status;
}

int USB::configureUsb()
{
    int status = 0;
    std::string backEndName;
    Stream *stream = NULL;
    std::shared_ptr<Device> dev = nullptr;
    PayloadBuilder* builder = new PayloadBuilder();
    struct usbAudioConfig cfg;
    uint8_t* payload = NULL;
    std::vector<Stream*> activestreams;
    Session *session = NULL;
    size_t payloadSize = 0;
    uint32_t miid = 0;
    int32_t tagId;

    rm->getBackendName(deviceAttr.id, backEndName);
    dev = Device::getInstance(&deviceAttr, rm);
    status = rm->getActiveStream_l(activestreams, dev);
    if ((0 != status) || (activestreams.size() == 0)) {
        PAL_ERR(LOG_TAG, "no active stream available");
        status = -EINVAL;
        goto exit;
    }
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    setUSBAudioGain();
#endif
    stream = static_cast<Stream *>(activestreams[0]);
    stream->getAssociatedSession(&session);
    if (deviceAttr.id == PAL_DEVICE_IN_USB_HEADSET) {
        tagId = DEVICE_HW_ENDPOINT_TX;
        cfg.usb_token = (deviceAttr.address.card_id << 16)|0x1;
        cfg.svc_interval = 0;
    } else{
        tagId = DEVICE_HW_ENDPOINT_RX;
        cfg.usb_token = deviceAttr.address.card_id << 16;
        cfg.svc_interval = 0;
    }
    status = session->getMIID(backEndName.c_str(), tagId, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %d, status = %d", tagId, status);
        goto exit;
    }
    builder->payloadUsbAudioConfig(&payload, &payloadSize, miid, &cfg);
    if (payloadSize) {
        status = updateCustomPayload(payload, payloadSize);
        freeCustomPayload(&payload, &payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
            goto exit;
        }
    }
exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int32_t USB::isSampleRateSupported(unsigned int sampleRate)
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
//TBD why do these channels have to be supported, USBs support only 1/2?
int32_t USB::isChannelSupported(unsigned int numChannels)
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

int USB::init(pal_param_device_connection_t device_conn)
{
    typename std::vector<std::shared_ptr<USBCardConfig>>::iterator iter;
    int ret = 0;

    for (iter = usb_card_config_list_.begin();
         iter != usb_card_config_list_.end(); iter++) {
         if ((*iter)->isConfigCached(device_conn.device_config.usb_addr))
              break;
    }

    if (iter == usb_card_config_list_.end()) {
#ifdef SEC_AUDIO_USB_DUMMY_TEST
        sec_pal_usb_test_init();
#endif
        std::shared_ptr<USBCardConfig> sp(
            new USBCardConfig(device_conn.device_config.usb_addr));
        if (!sp) {
            PAL_ERR(LOG_TAG, "failed to create new usb_card_config object.");
            return -EINVAL;
        }
        if (isUSBOutDevice(device_conn.id))
            ret = sp->getCapability(USB_PLAYBACK, device_conn.device_config.usb_addr);
        else
            ret = sp->getCapability(USB_CAPTURE, device_conn.device_config.usb_addr);

        if (ret == 0) {
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
            USBAudioGainControl(device_conn.device_config.usb_addr, device_conn.id);
#endif
            usb_card_config_list_.push_back(sp);
        }
        setVendorIdCkv(device_conn.device_config.usb_addr);
    } else {
        PAL_INFO(LOG_TAG, "usb info has been cached.");
    }

    return ret;
}

int USB::deinit(pal_param_device_connection_t device_conn)
{
    typename std::vector<std::shared_ptr<USBCardConfig>>::iterator iter;

    for (iter = usb_card_config_list_.begin();
         iter != usb_card_config_list_.end(); iter++) {
         if ((*iter)->isConfigCached(device_conn.device_config.usb_addr))
              break;
    }

    if (iter != usb_card_config_list_.end()) {
        //TODO: usb_remove_capability
        usb_card_config_list_.erase(iter);
    } else {
        PAL_INFO(LOG_TAG, "usb info has not been cached.");
    }
#ifdef SEC_AUDIO_USB_DUMMY_TEST
    sec_pal_usb_test_deinit();
#endif
#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    if (usbAddress_.usb_ar) {
        audio_route_free(usbAddress_.usb_ar);
        usbAddress_.usb_ar = nullptr;
    }
#endif
    return 0;
}

int32_t USB::isBitWidthSupported(unsigned int bitWidth __unused)
{
    return 0;
}

int32_t USB::checkAndUpdateBitWidth(unsigned int *bitWidth __unused)
{
    return 0;
}

int32_t USB::checkAndUpdateSampleRate(unsigned int *sampleRate __unused)
{
    return 0;
}

bool USB::isUSBOutDevice(pal_device_id_t pal_dev_id) {
    if (pal_dev_id == PAL_DEVICE_OUT_USB_DEVICE ||
        pal_dev_id == PAL_DEVICE_OUT_USB_HEADSET)
        return true;
    else
        return false;
}

int USB::usb_vendor_id_ckv_ = 0;

void USB::setVendorIdCkv(struct pal_usb_device_address addr) {
    std::vector<std::string>::iterator it;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    std::string vendor_id_usb;
    usb_vendor_id_ckv_ = 0;    //reset value to 0 to load default
    std::ifstream in("/proc/asound/card"+std::to_string(addr.card_id)+"/usbid");

    if(in.good()) {
        if (getline(in, vendor_id_usb)) {
            PAL_DBG(LOG_TAG, "USB_Vendor_ID of connected usb device is %s", vendor_id_usb.c_str());
        }
    }

    if (vendor_id_usb.empty())
        goto done;

    it = std::find(rm->usb_vendor_uuid_list.begin(), rm->usb_vendor_uuid_list.end(), vendor_id_usb);

    if (it != rm->usb_vendor_uuid_list.end()) {
        usb_vendor_id_ckv_ = it - rm->usb_vendor_uuid_list.begin();
    }

done:
    PAL_DBG(LOG_TAG, "USB_Vendor_ID_CKV index is: %d", usb_vendor_id_ckv_);
}

int USB::getVendorIdCkv() {
    return usb_vendor_id_ckv_;
}

bool USBCardConfig::isCaptureProfileSupported()
{
    usb_usecase_type_t capture_type = USB_CAPTURE;
    typename std::vector<std::shared_ptr<USBDeviceConfig>>::iterator iter;

    for (iter = usb_device_config_list_.begin();
         iter != usb_device_config_list_.end(); iter++) {
         if ((*iter)->getType() == capture_type)
             return true;
    }

    return false;
}

int USB::getDefaultConfig(pal_param_device_capability_t capability)
{
    typename std::vector<std::shared_ptr<USBCardConfig>>::iterator iter;
    int status = 0;

    if (!isUsbConnected(capability.addr)) {
        PAL_ERR(LOG_TAG, "No usb sound card present");
        return -EINVAL;
    }

    for (iter = usb_card_config_list_.begin();
            iter != usb_card_config_list_.end();
            iter++) {
        if ((*iter)->isConfigCached(capability.addr)) {
            PAL_DBG(LOG_TAG, "usb device is found.");
            // for capture, check if profile is supported or not
            if (capability.is_playback == false) {
                memset(capability.config, 0, sizeof(struct dynamic_media_config));
                if ((*iter)->isCaptureProfileSupported())
                    status = (*iter)->readSupportedConfig(capability.config,
                            capability.is_playback, capability.addr.card_id);
            } else {
                status = (*iter)->readSupportedConfig(capability.config,
                        capability.is_playback, capability.addr.card_id);
            }
            break;
        }
    }

    if (iter == usb_card_config_list_.end()) {
        PAL_ERR(LOG_TAG, "usb device card=%d device=%d is not found.",
                    capability.addr.card_id, capability.addr.device_num);
        return -EINVAL;
    }

    return status;
}

int USB::selectBestConfig(struct pal_device *dattr,
                          struct pal_stream_attributes *sattr,
                          bool is_playback, struct pal_device_info *devinfo)
{
    typename std::vector<std::shared_ptr<USBCardConfig>>::iterator iter;

    int status = 0;

    for (iter = usb_card_config_list_.begin();
            iter != usb_card_config_list_.end(); iter++) {
        if ((*iter)->isConfigCached(dattr->address)) {
#ifdef SEC_AUDIO_SUPPORT_UHQ
            PAL_INFO(LOG_TAG, "usb device is found. rm->isUHQAEnabled:%d ", rm->stateUHQA);
            status = (*iter)->readBestConfig(&dattr->config, sattr, is_playback,
                                   devinfo, rm->stateUHQA);
#else
            PAL_DBG(LOG_TAG, "usb device is found.");
            status = (*iter)->readBestConfig(&dattr->config, sattr, is_playback,
                                   devinfo, rm->isUHQAEnabled);
#endif
#ifdef SEC_AUDIO_USB_DUMMY_TEST
            if (isUsbAlive(dattr->address.card_id)) {
                sec_pal_usb_test_best_config(&dattr->config, is_playback);
            }
#endif
            break;
        }
    }

    if (iter == usb_card_config_list_.end()) {
        PAL_ERR(LOG_TAG, "usb device card=%d device=%d is not found.",
                    dattr->address.card_id, dattr->address.device_num);
        return -EINVAL;
    }

    return status;

}

#ifdef SEC_AUDIO_USB_GAIN_CONTROL
int USB::setUSBAudioGain()
{
    int ret = 0;
    char mSndDeviceName[128] = {0};
    char gain_name[MAX_PATH_LEN] = {0};
    typename std::vector<std::shared_ptr<USBCardConfig>>::iterator iter;

    ret = rm->getSndDeviceName(deviceAttr.id , mSndDeviceName);
    if (0 != ret) {
        PAL_ERR(LOG_TAG, "Failed to obtain snd device name");
        goto done;
    }

    // Add "-gain" post-fix for usb gain control
    snprintf(gain_name, sizeof(gain_name), "%s%s", mSndDeviceName, "-gain");

    /* Apply gain only if audio_route library is initialized */
    if (usbAddress_.usb_ar) {
#ifdef SEC_AUDIO_FMRADIO
        bool isCallActive = false;
        if (strstr(gain_name, "call") || strstr(gain_name, "voip")) {
            isCallActive = true;
        }
        if (isFmUsbEnabled()
                && !isCallActive && rm->isPluginPlaybackDevice(deviceAttr.id)) {
            enableDevice(usbAddress_.usb_ar, "fm-usb-headphones-gain");
        } else
#endif
        {
            enableDevice(usbAddress_.usb_ar, gain_name);
        }
    }

done:
    return ret;
}
#endif
const unsigned int USBCardConfig::out_chn_mask_[MAX_SUPPORTED_CHANNEL_MASKS] =
    {0x3, 0x80000003, 0x80000007, 0x8000000f, 0x8000001f, 0x8000003f,
    0x8000007f, 0x800000ff};

const unsigned int USBCardConfig::in_chn_mask_[MAX_SUPPORTED_CHANNEL_MASKS] =
    {0x10, 0x80000001, 0xc, 0x80000003, 0x80000007, 0x8000000f, 0x8000001f,
    0x8000003f};

bool USBCardConfig::isConfigCached(struct pal_usb_device_address addr) {
    if(address_.card_id == addr.card_id && address_.device_num == addr.device_num)
        return true;
    else
        return false;
}

void USBCardConfig::setEndian(int endian){
    endian_ = endian;
}

void USBCardConfig::dumpCapabilities(char* read_buf, int type) {
    char* start = nullptr;
    const char s[2] = "\n";
    char* token;
    char *tmp = nullptr;

    const char* direction = type == USB_PLAYBACK ? PLAYBACK_PROFILE_STR : CAPTURE_PROFILE_STR;

    start = strstr(read_buf, direction);
    token = strtok_r(start, s, &tmp);
    while (token != nullptr) {
        if (!strcmp(token, CAPTURE_PROFILE_STR) && (type == USB_PLAYBACK)) {
            break;
        }
        PAL_INFO(LOG_TAG, " %s", token);
        token = strtok_r(nullptr, s, &tmp);
    }
}

int USBCardConfig::getCapability(usb_usecase_type_t type,
                                        struct pal_usb_device_address addr) {
    int32_t size = 0;
    FILE *fd = NULL;
    int32_t channels_no;
    char *str_start = NULL;
    char *str_end = NULL;
    char *channel_start = NULL;
    char *bit_width_start = NULL;
    char *rates_str_start = NULL;
    char *target = NULL;
    char *read_buf = NULL;
    char *rates_str = NULL;
    char *interval_str_start = NULL;
    char path[128];
    int ret = 0;
    char *bit_width_str = NULL;
    size_t num_read = 0;
    const char* suffix;
    bool jack_status;
    //std::shared_ptr<USBDeviceConfig> usb_device_info = nullptr;
#ifdef SEC_AUDIO_USB_CONFIGURATION
    size_t buf_size = 0;
#endif

    bool check = false;
#ifdef SEC_AUDIO_QUICK_USB_DETECTION
    int retries = USB_PCM_OPEN_RETRY_CNT;
#endif
    memset(path, 0, sizeof(path));
    PAL_DBG(LOG_TAG, "for %s", (type == USB_PLAYBACK) ?
          PLAYBACK_PROFILE_STR : CAPTURE_PROFILE_STR);

#ifdef SEC_AUDIO_USB_DUMMY_TEST
    if (sec_pal_usb_test_process()) {
        ret = sec_pal_usb_test_profile(path, sizeof(path));
        if (type == USB_CAPTURE) {
            sec_pal_usb_test_supported_channels(addr.card_id);
        }
    } else
#endif
    ret = snprintf(path, sizeof(path), "/proc/asound/card%u/stream0",
             addr.card_id);
    if(ret < 0) {
        PAL_ERR(LOG_TAG, "failed on snprintf (%d) to path %s\n", ret, path);
        ret = -EINVAL;
        goto done;
    }
#ifdef SEC_AUDIO_QUICK_USB_DETECTION
    while (retries--) {
#ifdef SEC_AUDIO_USB_DUMMY_TEST
        if (sec_pal_usb_test_process()) {
            break;
        }
#endif
        if (usb_recognition_state(addr.card_id) && (access(path, F_OK) < 0)) {
            PAL_INFO(LOG_TAG, "%s doesn't exist, retrying\n", path);
            usleep(20000);
        } else {
            break;
        }
    }
#endif
#ifdef SEC_AUDIO_USB_CONFIGURATION
    fd = fopen(path, "rb");
    if (!fd) {
        PAL_ERR(LOG_TAG, "failed to open config file %s error: %d\n", path, errno);
        ret = -EINVAL;
        goto done;
    }

    buf_size = USB_BUFF_SIZE + 1;

    read_buf = (char *)calloc(1, buf_size);
    if (!read_buf) {
        PAL_ERR(LOG_TAG, "Failed to create read_buf");
        ret = -ENOMEM;
        goto done;
    }

    /*
     * ALSA's USB-Config stream file size can been
     * increased if more information is added
     * therefore file reading logic is
     * changed to read until end-of-file
     */
    while (!feof(fd)) {
        ssize_t read_now = 0;
        void *realloc_buf = NULL;

        if (num_read == (buf_size - 1)) {
            buf_size += USB_BUFF_SIZE;
            realloc_buf = realloc(read_buf, buf_size);
            if (!realloc_buf) {
                PAL_ERR(LOG_TAG, "Failed to reallocated read_buf");
                ret = -ENOMEM;
                goto done;
            } else {
                read_buf = (char *)realloc_buf;
            }

            PAL_INFO(LOG_TAG, "%s: current buffer size %zu reallocate size %zu \n",
                        __func__, num_read, buf_size);
        }

        read_now = fread((read_buf + num_read), 1, USB_BUFF_SIZE, fd);
        if(ferror(fd)) {
            PAL_ERR(LOG_TAG, "file read error");
            goto done;
        } else {
            num_read += read_now;
        }
    }
#else
    fd = fopen(path, "r");
    if (!fd) {
        PAL_ERR(LOG_TAG, "failed to open config file %s error: %d\n", path, errno);
        ret = -EINVAL;
        goto done;
    }

    read_buf = (char *)calloc(1, USB_BUFF_SIZE + 1);
    if (!read_buf) {
        PAL_ERR(LOG_TAG, "Failed to create read_buf");
        ret = -ENOMEM;
        goto done;
    }

    if ((num_read = fread(read_buf, 1, USB_BUFF_SIZE, fd)) < 0) {
        PAL_ERR(LOG_TAG, "file read error");
        goto done;
    }
#endif
    read_buf[num_read] = '\0';

    str_start = strstr(read_buf, ((type == USB_PLAYBACK) ?
                       PLAYBACK_PROFILE_STR : CAPTURE_PROFILE_STR));
    if (str_start == NULL) {
        PAL_INFO(LOG_TAG, "error %s section not found in usb config file",
                ((type == USB_PLAYBACK) ?
               PLAYBACK_PROFILE_STR : CAPTURE_PROFILE_STR));
        ret = -ENOENT;
        goto done;
    }

    str_end = strstr(read_buf, ((type == USB_PLAYBACK) ?
                       CAPTURE_PROFILE_STR : PLAYBACK_PROFILE_STR));

    if (str_end > str_start)
        check = true;

    while (str_start != NULL) {
        str_start = strstr(str_start, "Altset ");
        if ((str_start == NULL) || (check  && (str_start >= str_end))) {
            PAL_VERBOSE(LOG_TAG,"done parsing %s\n", str_start);
            break;
        }
        PAL_VERBOSE(LOG_TAG,"remaining string %s\n", str_start);
        str_start += sizeof("Altset ");
        std::shared_ptr<USBDeviceConfig> usb_device_info(new USBDeviceConfig());
        if (!usb_device_info) {
            PAL_ERR(LOG_TAG, "error unable to create usb device config object");
            ret = -ENOMEM;
            break;
        }
        usb_device_info->setType(type);
        /* Bit bit_width parsing */
        bit_width_start = strstr(str_start, "Format: ");
        if (bit_width_start == NULL || (check && (bit_width_start >= str_end))) {
            PAL_INFO(LOG_TAG, "Could not find bit_width string");
            continue;
        }
        target = strchr(bit_width_start, '\n');
        if (target == NULL) {
            PAL_INFO(LOG_TAG, "end of line not found");
            continue;
        }
        size = target - bit_width_start;
        if ((bit_width_str = (char *)malloc(size + 1)) == NULL) {
            PAL_ERR(LOG_TAG, "unable to allocate memory to hold bit width strings");
            ret = -EINVAL;
            break;
        }
        memcpy(bit_width_str, bit_width_start, size);
        bit_width_str[size] = '\0';

        const char *formats[] = {"S32", "S24_3", "S24", "S16", "U32"};
        const int bit_width[] = {32, 24, 24, 16, 32};
        for (size_t i = 0; i < sizeof(formats)/sizeof(formats[0]); i++) {
            const char * s = strstr(bit_width_str, formats[i]);
            if (s) {
                usb_device_info->setBitWidth(bit_width[i]);
                setEndian(strstr(s, "BE") ? 1 : 0);
                break;
            }
        }

        if (bit_width_str)
            free(bit_width_str);

        /* channels parsing */
        channel_start = strstr(str_start, CHANNEL_NUMBER_STR);
        if (channel_start == NULL || (check && (channel_start >= str_end))) {
            PAL_INFO(LOG_TAG, "could not find Channels string");
            continue;
        }
        channels_no = atoi(channel_start + strlen(CHANNEL_NUMBER_STR));
        usb_device_info->setChannels(channels_no);

        /* Sample rates parsing */
        rates_str_start = strstr(str_start, "Rates: ");
        if (rates_str_start == NULL || (check && (rates_str_start >= str_end))) {
            PAL_INFO(LOG_TAG, "cant find rates string");
            continue;
        }
        target = strchr(rates_str_start, '\n');
        if (target == NULL) {
            PAL_INFO(LOG_TAG, "end of line not found");
            continue;
        }
        size = target - rates_str_start;
        if ((rates_str = (char *)malloc(size + 1)) == NULL) {
            PAL_INFO(LOG_TAG, "unable to allocate memory to hold sample rate strings");
            ret = -EINVAL;

            break;
        }
        memcpy(rates_str, rates_str_start, size);
        rates_str[size] = '\0';
        ret = usb_device_info->getSampleRates(type, rates_str);
        if (rates_str)
            free(rates_str);
        if (ret < 0) {
            PAL_INFO(LOG_TAG, "error unable to get sample rate values");
            continue;
        }
        // Data packet interval is an optional field.
        // Assume 0ms interval if this cannot be read
        // LPASS USB and HLOS USB will figure out the default to use
        usb_device_info->setInterval(DEFAULT_SERVICE_INTERVAL_US);
        interval_str_start = strstr(str_start, DATA_PACKET_INTERVAL_STR);
        if (interval_str_start != NULL) {
            interval_str_start += strlen(DATA_PACKET_INTERVAL_STR);
            ret = usb_device_info->getServiceInterval(interval_str_start);
            if (ret < 0) {
                PAL_INFO(LOG_TAG, "error unable to get service interval, assume default");
            }
        }
        /* jack status parsing */
        suffix = (type == USB_PLAYBACK) ? USB_OUT_JACK_SUFFIX : USB_IN_JACK_SUFFIX;
        jack_status = getJackConnectionStatus(addr.card_id, suffix);
        PAL_DBG(LOG_TAG, "jack_status %d", jack_status);
        usb_device_info->setJackStatus(jack_status);

        /* Add to list if every field is valid */
        usb_device_config_list_.push_back(usb_device_info);
        format_list_map.insert( std::pair<int, std::shared_ptr<USBDeviceConfig>>(usb_device_info->getBitWidth(),usb_device_info));
    }

#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    getUSBVidPid(addr);
#endif
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    saveUSBCapability(read_buf);
#endif

    dumpCapabilities(read_buf, type);

done:
    if (fd)
        fclose(fd);

    if (read_buf)
        free(read_buf);

    return ret;
}

USBCardConfig::USBCardConfig(struct pal_usb_device_address address) {
    address_ = address;
}

unsigned int USBCardConfig::getMax(unsigned int x, unsigned int y) {
    return (((x) >= (y)) ? (x) : (y));
}

unsigned int USBCardConfig::getMin(unsigned int x, unsigned int y) {
    return (((x) <= (y)) ? (x) : (y));
}

int USBCardConfig::getMaxBitWidth(bool is_playback)
{
    unsigned int max_bw = 16;
    typename std::vector<std::shared_ptr<USBDeviceConfig>>::iterator iter;

    for (iter = usb_device_config_list_.begin();
         iter != usb_device_config_list_.end(); iter++) {
         if ((*iter)->getType() == is_playback)
             max_bw = getMax(max_bw, (*iter)->getBitWidth());
    }

    return max_bw;
}

int USBCardConfig::getMaxChannels(bool is_playback)
{
    unsigned int max_ch = 1;
    typename std::vector<std::shared_ptr<USBDeviceConfig>>::iterator iter;

    for (iter = usb_device_config_list_.begin();
         iter != usb_device_config_list_.end(); iter++) {
         if ((*iter)->getType() == is_playback)
             max_ch = getMax(max_ch, (*iter)->getChannels());
    }

    return max_ch;
}

unsigned int USBCardConfig::getFormatByBitWidth(int bitwidth) {
    unsigned int default_format = PCM_16_BIT;
    switch (bitwidth) {
        case 24:
            // XXX : usb.c returns 24 for s24 and s24_le?
            default_format = PCM_24_BIT_PACKED;
            break;
        case 32:
            default_format = PCM_32_BIT;
            break;
        case 16:
        default :
            default_format = PCM_16_BIT;
            break;
    }

    return default_format;
}

void USBCardConfig::readSupportedFormat(bool is_playback, uint32_t *format) {
    int i = 0;
    unsigned int bw;
    unsigned int bitWidth[MAX_SUPPORTED_FORMATS + 1];
    typename std::vector<std::shared_ptr<USBDeviceConfig>>::iterator iter;
    bool insert;

    for (iter = usb_device_config_list_.begin();
         iter != usb_device_config_list_.end(); iter++) {
        insert = true;
        if ((*iter)->getType() == is_playback) {
            bw = (*iter)->getBitWidth();
            for (int j = 0; j < i; j++) {
                if (bw == bitWidth[j]) {
                    insert = false;
                    break;
                }
            }
            if (insert) {
                bitWidth[i] = bw;
                PAL_DBG(LOG_TAG, "%s supported bw %d", is_playback ? "P" : "C", bitWidth[i]);
                i++;
                if (i == (MAX_SUPPORTED_FORMATS + 1)) {
                    PAL_ERR(LOG_TAG, "reached the maximum num of formats");
                    break;
                }
            }
        }
    }
    /* sort the bit width with descending order */
    for (int j = 0; j < i - 1; j++) {
        unsigned int temp_bw;
        for (int k = j + 1; k < i; k++) {
            if (bitWidth[j] <  bitWidth[k]) {
                temp_bw = bitWidth[j];
                bitWidth[j] = bitWidth[k];
                bitWidth[k] = temp_bw;
            }
        }
    }
    /* convert bw to format */
    for (int j = 0; j < i; j++)
        format[j] = getFormatByBitWidth(bitWidth[j]);
}

void USBCardConfig::readSupportedSampleRate(bool is_playback, uint32_t *sample_rate) {
    usb_usecase_type_t type = is_playback ? USB_PLAYBACK : USB_CAPTURE;

    typename std::vector<std::shared_ptr<USBDeviceConfig>>::iterator iter;

    for (iter = usb_device_config_list_.begin();
        iter != usb_device_config_list_.end(); iter++) {
        if ((*iter)->getType() == is_playback){
            usb_supported_sample_rates_mask_[type] |= (*iter)->getSRMask(type);
        }
    }
#define _MIN(x, y) (((x) <= (y)) ? (x) : (y))
    PAL_DBG(LOG_TAG, "supported_sample_rates_mask_ 0x%x", usb_supported_sample_rates_mask_[type]);
    uint32_t bm = usb_supported_sample_rates_mask_[type];
    uint32_t tries = _MIN(MAX_SUPPORTED_SAMPLE_RATES, (uint32_t)__builtin_popcount(bm));
#undef _MIN

    int i = 0;
    while (tries) {
        int idx = __builtin_ffs(bm) - 1;
        sample_rate[i++] = USBDeviceConfig::supported_sample_rates_[idx];
        bm &= ~(1<<idx);
        tries--;
    }

    for (int j = 0; j < i; j++)
        PAL_DBG(LOG_TAG, "%s %d", is_playback ? "P" : "C", sample_rate[j]);
}

void USBCardConfig::readSupportedChannelMask(bool is_playback, uint32_t *channel) {

    int channels = getMaxChannels(is_playback);
    int channel_count;

    if (channels > MAX_HIFI_CHANNEL_COUNT)
        channels = MAX_HIFI_CHANNEL_COUNT;
     /* need to calculate the mask from channel count either because this is
      * the query case or the specified mask isn't valid for this device,
      * or is more than the FW can handle
      */
    if (is_playback) {
        channel[0] = channels <= 2
                     /* position mask for mono and stereo*/
                     ? audio_channel_out_mask_from_count(channels)
                     /* otherwise indexed */
                     : audio_channel_mask_for_index_assignment_from_count(channels);
    } else {
        channel[0] = channels <= 2
             /* position mask for mono & stereo */
             ? audio_channel_in_mask_from_count(channels)
             /* otherwise indexed */
             : audio_channel_mask_for_index_assignment_from_count(channels);
    }
    PAL_INFO(LOG_TAG, "%s supported ch %d, channel mask: %08x ",
            is_playback ? "P" : "C", channels, channel[0]);
}

bool USBCardConfig::readDefaultJackStatus(bool is_playback) {
    bool jack_status = true;
    typename std::vector<std::shared_ptr<USBDeviceConfig>>::iterator iter;

    for (iter = usb_device_config_list_.begin();
        iter != usb_device_config_list_.end(); iter++) {
        if ((*iter)->getType() == is_playback){
            jack_status = (*iter)->getJackStatus();
            break;
        }
    }

    return jack_status;
}

int USBCardConfig::readSupportedConfig(struct dynamic_media_config *config, bool is_playback, int usb_card)
{
    const char* suffix;
    readSupportedFormat(is_playback, config->format);
    readSupportedSampleRate(is_playback, config->sample_rate);
    readSupportedChannelMask(is_playback, config->mask);
    suffix = is_playback ? USB_OUT_JACK_SUFFIX : USB_IN_JACK_SUFFIX;
    config->jack_status = getJackConnectionStatus(usb_card, suffix);
    PAL_DBG(LOG_TAG, "config->jack_status = %d", config->jack_status);

    return 0;
}
#ifdef SEC_AUDIO_SUPPORT_UHQ
int USBCardConfig::readBestConfig(struct pal_media_config *config,
                                struct pal_stream_attributes *sattr, bool is_playback,
                                struct pal_device_info *devinfo, pal_uhqa_state state)
#else
int USBCardConfig::readBestConfig(struct pal_media_config *config,
                                struct pal_stream_attributes *sattr, bool is_playback,
                                struct pal_device_info *devinfo, bool uhqa)
#endif
{
    std::shared_ptr<USBDeviceConfig> candidate_config = nullptr;
    int max_bit_width = 0;
    int max_channel = 0;
    int bitwidth = 16;
    int candidate_sr = 0;
    int ret = -EINVAL;
    struct pal_media_config media_config;
    std::map<int, std::shared_ptr<USBDeviceConfig>> candidate_list;
    std::vector<std::shared_ptr<USBDeviceConfig>> profile_list_max_ch;
    std::vector<std::shared_ptr<USBDeviceConfig>> profile_list_match_ch;
    int target_bit_width = devinfo->bit_width == 0 ?
                           config->bit_width : devinfo->bit_width;

    int target_sample_rate = devinfo->samplerate == 0 ?
                           config->sample_rate : devinfo->samplerate;

#ifdef SEC_AUDIO_SUPPORT_UHQ
    bool uhqa = false;
    if (sattr->type != PAL_STREAM_DEEP_BUFFER &&
            sattr->type != PAL_STREAM_COMPRESSED) {
        PAL_INFO(LOG_TAG, "PAL_UHQ_STATE_NORMAL for type = %d", sattr->type);
        state = PAL_UHQ_STATE_NORMAL;
    }
    uhqa = (state != PAL_UHQ_STATE_NORMAL);
    PAL_INFO(LOG_TAG, "USB %s uhqa = %d", is_playback ? "output" : "input", state);

    if (is_playback && (state == PAL_UHQ_STATE_384KHZ)) {
        target_bit_width = getMaxBitWidth(is_playback);
    }
#endif
#ifdef SEC_AUDIO_CALL
    bool is_voice_or_voip = false;
    if (sattr->type == PAL_STREAM_VOICE_CALL ||
         sattr->type == PAL_STREAM_VOIP_RX ||
         sattr->type == PAL_STREAM_VOIP_TX) {
        is_voice_or_voip = true;
    }
#endif
    if (is_playback) {
        PAL_INFO(LOG_TAG, "USB output uhqa = %d", uhqa);
        media_config = sattr->out_media_config;
    } else {
        PAL_INFO(LOG_TAG, "USB input uhqa = %d", uhqa);
        media_config = sattr->in_media_config;
    }

    if (!format_list_map.empty()) {
        if (format_list_map.count(target_bit_width) == 0) {
            /* if bit width does not match, use highest width. */
            auto max_fmt = format_list_map.rbegin();
            max_bit_width = max_fmt->first;
            config->bit_width = max_bit_width;
            PAL_INFO(LOG_TAG, "Target bitwidth of %d is not supported by USB. Use USB width of %d",
                         target_bit_width, max_bit_width);
            target_bit_width = max_bit_width;
        } else {
            /* 1. bit width matches. */
            config->bit_width = target_bit_width;
            PAL_INFO(LOG_TAG, "found matching BitWidth = %d", config->bit_width);
        }

        max_channel = getMaxChannels(is_playback);
        auto profile_list = format_list_map.equal_range(target_bit_width);
        for (auto iter = profile_list.first; iter != profile_list.second; ++iter) {
            auto cfg_iter = iter->second;
            if (cfg_iter->getType() != is_playback)
                continue;
            if (cfg_iter->getChannels() == media_config.ch_info.channels) {
                profile_list_match_ch.push_back(cfg_iter);
            } else if(cfg_iter->getChannels() == max_channel) {
                profile_list_max_ch.push_back(cfg_iter);
            }
        }

        std::vector<std::shared_ptr<USBDeviceConfig>> profile_list_ch;
        if (!profile_list_match_ch.empty()) {
            /*2. channal matches */
            profile_list_ch = profile_list_match_ch;
            PAL_INFO(LOG_TAG, "found matching channels = %d", media_config.ch_info.channels);
        } else {
            profile_list_ch = profile_list_max_ch;
            PAL_INFO(LOG_TAG, "Target Channel of %d is not supported by USB. Use USB channel of %d",
                         media_config.ch_info.channels, max_channel);
        }

        if (!profile_list_ch.empty()) {
            /*3. get best Sample Rate */
#if defined (SEC_AUDIO_CALL) || defined(SEC_AUDIO_SUPPORT_UHQ)
            target_sample_rate = media_config.sample_rate;
            if (is_voice_or_voip ||
                    (!uhqa && is_playback && 
                        (sattr->type == PAL_STREAM_COMPRESSED || sattr->type == PAL_STREAM_PCM_OFFLOAD))) {
                target_sample_rate = devinfo->samplerate;
            }
#endif
            if (uhqa && is_playback) {
                for (auto ch_iter = profile_list_ch.begin(); ch_iter!= profile_list_ch.end(); ++ch_iter) {
#ifdef SEC_AUDIO_SUPPORT_USB_OFFLOAD
                    target_sample_rate = state;
#else
                    if ((*ch_iter)->isRateSupported(SAMPLINGRATE_192K)) {
                        config->sample_rate = SAMPLINGRATE_192K;
                        candidate_config = *ch_iter;
                        break;
                    } else if ((*ch_iter)->isRateSupported(SAMPLINGRATE_96K)) {
                        config->sample_rate = SAMPLINGRATE_96K;
                        candidate_config = *ch_iter;
                    }
#endif
               }
               if (candidate_config) {
                   PAL_INFO(LOG_TAG, "uhqa: found matching SampleRate = %d", config->sample_rate);
                   goto UpdateBestCh;
               }
            }

            for (auto ch_iter = profile_list_ch.begin(); ch_iter!= profile_list_ch.end(); ++ch_iter) {
                int ret = (*ch_iter)->getBestRate(target_sample_rate, candidate_sr,
                                        &config->sample_rate);
                if (ret == 0) {
                    PAL_INFO(LOG_TAG, "found matching SampleRate = %d", config->sample_rate);
                    candidate_config = *ch_iter;
                    break;
                }
                /* if target Sample Rate is not supported by USB, look for best one
                   in all profile list that channel and bit-width match.*/
                candidate_list.insert(std::pair<int, std::shared_ptr<USBDeviceConfig>>
                                               (config->sample_rate, *ch_iter));
                candidate_sr = config->sample_rate;
                candidate_config = candidate_list[candidate_sr];
            }
UpdateBestCh:
            if (candidate_config)
                candidate_config->updateBestChInfo(&media_config.ch_info, &config->ch_info);
        }
    } else {
        PAL_ERR(LOG_TAG, "format_list_map is empty!");
    }
    return 0;
}

#ifdef SEC_AUDIO_USB_GAIN_CONTROL
void USB::USBAudioGainLoadXml(struct pal_usb_device_address addr)
{
    int ret = 0;
    char xml_path_name[MAX_PATH_LEN] = {0};
    std::string chip = "";
#ifdef SEC_AUDIO_QUICK_USB_DETECTION
    char path[MAX_PATH_LEN] = {0};
    int retries = USB_PCM_OPEN_RETRY_CNT;
#endif

    if (usbAddress_.pid == SEC_GRAY_HEADPHONE_PID
        || usbAddress_.pid == SEC_GRAY_HEADSET_PID ){
        chip = "gray";
    } else {
        chip = "default";
    }

    /* Form the gain xml name: <mixer_usb>_<chip>.xml */
    ret = snprintf(xml_path_name, sizeof(xml_path_name), "%s%s%s",
        GAIN_XML_PATH, chip.c_str(), ".xml");
    if (ret < 0) {
        PAL_ERR(LOG_TAG, "failed to create path for gain XML\n");
        goto done;
    }

    PAL_INFO(LOG_TAG,"Mixer gain XML = %s\n", xml_path_name);
#ifdef SEC_AUDIO_QUICK_USB_DETECTION
    snprintf(path, sizeof(path), "/dev/snd/controlC%u", addr.card_id);
    while (retries--) {
        if (usb_recognition_state(addr.card_id) && (access(path, F_OK) < 0)) {
            PAL_INFO(LOG_TAG, "%s doesn't exist, retrying\n", path);
            usleep(20000);
        } else {
            break;
        }
    }
#endif
    /* Open the gain xml using the audio_route library */
    if (!usbAddress_.usb_ar) {
        usbAddress_.usb_ar = audio_route_init(addr.card_id, xml_path_name);
        if (!usbAddress_.usb_ar)
            PAL_ERR(LOG_TAG,"Unable to parse gain XML at - %s\n", xml_path_name);
    } else {
        PAL_INFO(LOG_TAG,"already configured\n");
    } 

done:
    return;
}

void USBCardConfig::getUSBVidPid(struct pal_usb_device_address addr)
{
    int card, ret = 0;
    char path[MAX_PATH_LEN], read_buf[USBID_SIZE];
    char *endptr;
    int32_t fd = -1;
#ifdef SEC_AUDIO_QUICK_USB_DETECTION
    int retries = USB_PCM_OPEN_RETRY_CNT;
#endif

    card = addr.card_id;

    memset(path, 0, sizeof(path));
    memset(read_buf, 0, sizeof(read_buf));

    /* read usbid file of this sound card to get the VID:PID */
    ret = snprintf(path, sizeof(path), "/proc/asound/card%u/usbid",
             card);
    if (ret < 0) {
        PAL_ERR(LOG_TAG, "failed on snprintf (%d) to path %s\n", ret, path);
        goto done;
    }
#ifdef SEC_AUDIO_QUICK_USB_DETECTION
    while (retries--) {
        if (usb_recognition_state(addr.card_id) && (access(path, F_OK) < 0)) {
            PAL_INFO(LOG_TAG, "%s doesn't exist, retrying\n", path);
            usleep(20000);
        } else {
            break;
        }
    }
#endif

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        PAL_ERR(LOG_TAG, "error failed to open usbid file %s error: %d\n", path, errno);
        ret = -EINVAL;
        goto done;
    }

    ret = read(fd, read_buf, USBID_SIZE);
    if (ret < 0) {
        PAL_ERR(LOG_TAG, "file read error\n");
        goto done;
    }

    /*
     * - Output of "cat /proc/asound/card%u/usbid" is of format "VID:PID"
     * where each VID and PID is 4 chars long.
     */
    usbAddress_.vid = address_.vid = (int)strtol(read_buf, &endptr, 16);
    if (endptr == NULL || *endptr == '\0' || *endptr != ':') {
        PAL_ERR(LOG_TAG, "failed to parse USB VID");
        address_.vid = -1;
        goto done;
    }
    usbAddress_.pid = address_.pid = (int)strtol((endptr+1), &endptr, 16);

    PAL_INFO(LOG_TAG, "vid = %d, pid = %d\n", address_.vid, address_.pid);

done:
    if (fd >= 0) close(fd);
    return;
}

void USB::USBAudioGainControl(struct pal_usb_device_address addr, pal_device_id_t deviceId)
{
    /* Check the VID and range(s) of PID. Proceed further only if we support */
    if (usbAddress_.vid == SEC_VID) {
        USBAudioGainLoadXml(addr);
#ifdef SEC_AUDIO_USB_PROFILING
        if (USB::isUSBOutDevice(deviceId)) {
            PAL_INFO(LOG_TAG, "USB_Profiling_register_card");
        }
#endif
    } else {
        PAL_VERBOSE(LOG_TAG, "Incompatible USB audio device. No GAIN control\n");
    }
    return;
}
#endif

const unsigned int USBDeviceConfig::supported_sample_rates_[] =
    {384000, 352800, 192000, 176400, 96000, 88200, 64000,
     48000, 44100, 32000, 24000, 22050, 16000, 11025, 8000};

void USBDeviceConfig::setBitWidth(unsigned int bit_width) {
    bit_width_ = bit_width;
}

void USBDeviceConfig::setJackStatus(bool jack_status) {
    jack_status_ = jack_status;
}

void USBDeviceConfig::setChannels(unsigned int channels) {
    channels_ = channels;
}

void USBDeviceConfig::setType(usb_usecase_type_t type) {
    type_ = type;
}

void USBDeviceConfig::setInterval(unsigned long interval) {
    service_interval_us_ = interval;
}

unsigned int USBDeviceConfig::getBitWidth() {
    return bit_width_;
}

unsigned int USBDeviceConfig::getChannels() {
    return channels_;
}

unsigned int USBDeviceConfig::getType() {
    return type_;
}

unsigned long USBDeviceConfig::getInterval() {
    return service_interval_us_;
}

unsigned int USBDeviceConfig::getDefaultRate() {
    return rates_[0];
}

bool USBDeviceConfig::getJackStatus() {
    return jack_status_;
}

bool USBDeviceConfig::isRateSupported(int requested_rate)
{
    if (find(rates_.begin(),rates_.end(),requested_rate) != rates_.end()) {
        return true;
    }
    PAL_INFO(LOG_TAG, "requested rate not supported = %d", requested_rate);
    return false;
}

void USBDeviceConfig::usb_find_sample_rate_candidate(int base, int requested_rate,
                                    int cur_rate, int candidate_rate, unsigned int *best_rate) {
    if (cur_rate % base == 0 && candidate_rate % base != 0) {
        *best_rate = cur_rate;
    } else if ((cur_rate % base == 0 && candidate_rate % base == 0) ||
               (cur_rate % base != 0 && candidate_rate % base != 0)) {
        if (abs(double(requested_rate - candidate_rate)) >
            abs(double(requested_rate - cur_rate))) {
            *best_rate = cur_rate;
        } else if (abs(double(requested_rate - candidate_rate)) ==
                   abs(double(requested_rate - cur_rate)) && (cur_rate > candidate_rate)) {
            *best_rate = cur_rate;
        } else {
            *best_rate = candidate_rate;
        }
    }
}
// return 0 if match, else return -EINVAL with USB best sample rate
int USBDeviceConfig::getBestRate(int requested_rate, int candidate_rate, unsigned int *best_rate) {

    for (int cur_rate : rates_) {
        if (requested_rate == cur_rate) {
            *best_rate = requested_rate;
            return 0;
        }
        if (candidate_rate == 0) {
            candidate_rate = cur_rate;
        }
        PAL_DBG(LOG_TAG, "candidate_rate %d, cur_rate %d, requested_rate %d",
                           candidate_rate, cur_rate, requested_rate);
        if (requested_rate % SAMPLINGRATE_8K == 0) {
            usb_find_sample_rate_candidate(SAMPLINGRATE_8K, requested_rate,
                                      cur_rate, candidate_rate, best_rate);
            candidate_rate = *best_rate;
        } else {
            usb_find_sample_rate_candidate(SAMPLINGRATE_22K, requested_rate,
                                      cur_rate, candidate_rate, best_rate);
            candidate_rate = *best_rate;
        }
    }
    PAL_INFO(LOG_TAG, "requested_rate %d, best_rate %u", requested_rate, *best_rate);

    return -EINVAL;
}

// return 0 if match, else return -EINVAL with USB channel
int USBDeviceConfig::updateBestChInfo(struct pal_channel_info *requested_ch_info,
                                        struct pal_channel_info *best_ch_info)
{
    struct pal_channel_info usb_ch_info;

    usb_ch_info.channels = channels_;
    for (int i = 0; i < channels_; i++) {
        usb_ch_info.ch_map[i] = PAL_CHMAP_CHANNEL_FL + i;
    }

    *best_ch_info = usb_ch_info;

    if (channels_ != requested_ch_info->channels) {
        PAL_ERR(LOG_TAG, "channel num mismatch. use USB's: %d", channels_);
        return -EINVAL;
    }

    return 0;
}

int USBDeviceConfig::getSampleRates(int type, char *rates_str) {
    unsigned int i;
    char *next_sr_string, *temp_ptr;
    unsigned int sr, min_sr, max_sr, sr_size = 0;

    /* Sample rate string can be in any of the folloing two bit_widthes:
     * Rates: 8000 - 48000 (continuous)
     * Rates: 8000, 44100, 48000
     * Support both the bit_widths
     */

    PAL_VERBOSE(LOG_TAG, "rates_str %s", rates_str);
    next_sr_string = strtok_r(rates_str, "Rates: ", &temp_ptr);
    if (next_sr_string == NULL) {
        PAL_ERR(LOG_TAG, "could not find min rates string");
        return -EINVAL;
    }
    if (strstr(rates_str, "continuous") != NULL) {
        min_sr = (unsigned int)atoi(next_sr_string);
        next_sr_string = strtok_r(NULL, " ,.-", &temp_ptr);
        if (next_sr_string == NULL) {
            PAL_ERR(LOG_TAG, "could not find max rates string");
            return -EINVAL;
        }
        max_sr = (unsigned int)atoi(next_sr_string);

        for (i = 0; i < MAX_SAMPLE_RATE_SIZE; i++) {
            if (supported_sample_rates_[i] >= min_sr &&
                supported_sample_rates_[i] <= max_sr) {
                // FIXME: we don't support >192KHz in recording path for now
                if ((supported_sample_rates_[i] > SAMPLE_RATE_192000) &&
                        (type == USB_CAPTURE))
                    continue;
                rates_.push_back(supported_sample_rates_[i]);
                supported_sample_rates_mask_[type] |= (1<<i);
                PAL_DBG(LOG_TAG, "continuous sample rate supported_sample_rates_[%d] %d",
                        i, supported_sample_rates_[i]);
            }
        }
    } else {
        do {
            sr = (unsigned int)atoi(next_sr_string);
            // FIXME: we don't support >192KHz in recording path for now
            if ((sr > SAMPLE_RATE_192000) && (type == USB_CAPTURE)) {
                next_sr_string = strtok_r(NULL, " ,.-", &temp_ptr);
                continue;
            }

            for (i = 0; i < MAX_SAMPLE_RATE_SIZE; i++) {
                if (supported_sample_rates_[i] == sr) {
                    PAL_DBG(LOG_TAG, "sr %d, supported_sample_rates_[%d] %d -> matches!!",
                              sr, i, supported_sample_rates_[i]);
                    rates_.push_back(supported_sample_rates_[i]);
                    supported_sample_rates_mask_[type] |= (1<<i);
                }
            }
            next_sr_string = strtok_r(NULL, " ,.-", &temp_ptr);
        } while (next_sr_string != NULL);
    }
    return 0;
}

int USBDeviceConfig::getServiceInterval(const char *interval_str_start)
{
    unsigned long interval = 0;
    char time_unit[8] = {0};
    int multiplier = 0;

    const char *eol = strchr(interval_str_start, '\n');
    if (!eol) {
        PAL_ERR(LOG_TAG, "No EOL found");
        return -1;
    }
    char *tmp = (char *)calloc(1, eol-interval_str_start+1);
    if (!tmp) {
        PAL_ERR(LOG_TAG, "failed to allocate tmp");
        return -1;
    }
    memcpy(tmp, interval_str_start, eol-interval_str_start);
    tmp[eol-interval_str_start] = '\0';
    sscanf(tmp, "%lu %2s", &interval, &time_unit[0]);
    if (!strcmp(time_unit, "us")) {
        multiplier = 1;
    } else if (!strcmp(time_unit, "ms")) {
        multiplier = 1000;
    } else if (!strcmp(time_unit, "s")) {
        multiplier = 1000000;
    } else {
        PAL_ERR(LOG_TAG, "unknown time_unit %s, assume default", time_unit);
        interval = DEFAULT_SERVICE_INTERVAL_US;
        multiplier = 1;
    }
    interval *= multiplier;
    PAL_DBG(LOG_TAG, "set service_interval_us %lu", interval);
    service_interval_us_ = interval;
    free(tmp);

    return 0;
}

bool USB::isUsbAlive(int card)
{
    char path[128];
    (void) snprintf(path, sizeof(path), "/proc/asound/card%u/stream0", card);
    if (access(path,F_OK)) {
        PAL_ERR(LOG_TAG, "failed on snprintf (%d) to path %s\n", card, path);
        return false;
    }

    PAL_INFO(LOG_TAG, "usb card is accessible");
    return true;
}

bool USB::isUsbConnected(struct pal_usb_device_address addr)
{
    if (isUsbAlive(addr.card_id)
#ifdef SEC_AUDIO_USB_DUMMY_TEST
        || sec_pal_usb_test_process()
#endif
        ) {
        PAL_INFO(LOG_TAG, "usb device is connected");
        return true;
    }

    PAL_ERR(LOG_TAG, "usb device is not connected");
    return false;
}

bool USBCardConfig::getJackConnectionStatus(int usb_card, const char* suffix)
{
    int i = 0, value = 0;
    struct mixer_ctl* ctrl = NULL;
    struct mixer* usb_card_mixer = mixer_open(usb_card);
    if (usb_card_mixer == NULL) {
        PAL_ERR(LOG_TAG, "Invalid mixer card %d type %s", usb_card, suffix);
        return true;
    }
    while ((ctrl = mixer_get_ctl(usb_card_mixer, i++)) != NULL) {
        const char* mixer_name = mixer_ctl_get_name(ctrl);
        if (strstr(mixer_name, suffix)) {
            break;
        } else {
            ctrl = NULL;
        }
    }
    if (!ctrl) {
        PAL_DBG(LOG_TAG, "Invalid mixer control");
        mixer_close(usb_card_mixer);
        return true;
    }
    mixer_ctl_update(ctrl);
    value = mixer_ctl_get_value(ctrl, 0);
    PAL_DBG(LOG_TAG, "ctrl %s - value %d", mixer_ctl_get_name(ctrl), value);
    mixer_close(usb_card_mixer);

    return value != 0;
}

#ifdef SEC_AUDIO_FMRADIO
int32_t USB::setParameter(uint32_t param_id, void *param)
{
    int ret = 0;

    PAL_DBG(LOG_TAG, " Enter.");

#ifdef SEC_AUDIO_USB_GAIN_CONTROL
    if (param_id == PAL_PARAM_ID_FMRADIO_USB_GAIN) {
        pal_param_fmradio_usb_gain_t* param_fmradio_usb_gain = (pal_param_fmradio_usb_gain_t *)param;
        setFmUsbEnable(param_fmradio_usb_gain->enable);
        if (param_fmradio_usb_gain->enable) {
            /* Apply gain only if audio_route library is initialized */
            if (usbAddress_.usb_ar) {
                enableDevice(usbAddress_.usb_ar, "fm-usb-headphones-gain");
            }
        } else {
            setUSBAudioGain(); // restore gain
        }
    }
#endif
    return ret;
}
#endif

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
void USBCardConfig::saveUSBCapability(char *read_buf)
{
    if (std::find(usbvidpid.begin(),
                   usbvidpid.end(),
                   std::make_pair(address_.vid, address_.pid)) == usbvidpid.end() ) {
        UsbProfileLog.log("vid %d pid %d\n", address_.vid, address_.pid);
        UsbProfileLog.logs(-1 , read_buf);
        usbvidpid.push_back(std::make_pair(address_.vid, address_.pid));
    } else {
        PAL_DBG(LOG_TAG, "usb device info has been cached.");
    }
}

void USB::dump(int fd)
{
    auto dumpLoggerUsbProfile = [fd](android::SimpleLog& logger) {
        dprintf(fd, "\nUSB Device Audio Profile :\n");
        logger.dump(fd, "" /* prefix */);
    };
    dumpLoggerUsbProfile(UsbProfileLog);
}
#endif
