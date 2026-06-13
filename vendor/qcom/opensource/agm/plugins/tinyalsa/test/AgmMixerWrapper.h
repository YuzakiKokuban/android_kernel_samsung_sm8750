/*
** Copyright (c) 2024, The Linux Foundation. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above
**     copyright notice, this list of conditions and the following
**     disclaimer in the documentation and/or other materials provided
**     with the distribution.
**   * Neither the name of The Linux Foundation nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
** OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
** IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** Changes from Qualcomm Innovation Center are provided under the following license:
** Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __AGMMIXERWRAPPER_H__
#define __AGMMIXERWRAPPER_H__

#include <iostream>
#include "agmmixer.h"

class AgmMixerWrapper {
protected:
    struct mixer *mixer;
    struct device_config deviceConfig;
    struct group_config groupConfig;
    struct usbAudioConfig usbAudioConfig;

public:
    virtual ~AgmMixerWrapper() = default;
    virtual int mixerOpen(unsigned int card) = 0;
    virtual int mixerClose(void) = 0;
    virtual struct device_config getDeviceMediaConfig(char* filename, char *intf_name) = 0;
    virtual int setDeviceMediaConfig(char *intf_name, struct device_config *config) = 0;
    virtual int setAudioInterfaceMetadata(char *intf_name, unsigned int dkv,
                                enum usecase_type usecase, int rate, int bitwidth, uint32_t stream_kv) = 0;
    virtual int setStreamMetadata(int device, uint32_t stream_kv, unsigned int instance_kv) = 0;
    virtual int setStreamDeviceMetadata(int device, uint32_t stream_kv, char *intf_name,
                                unsigned int devicepp_kv) = 0;
    virtual int connectAudioInterfaceToStream(unsigned int device, char *intf_name) = 0;
    virtual int configureMFC(int device, char *intf_name, struct device_config) = 0;
    virtual struct group_config getGroupConfig(char *intf_name) = 0;
    virtual int setGroupConfig(unsigned int device, char *intf_name, unsigned int device_kv, struct group_config config, unsigned int channels) = 0;
    virtual int setDeviceCustomPayload(char *intf_name, int device, unsigned int usb_device) = 0;
    virtual int disconnectAudioInterfaceToStream(unsigned int device, char *intf_name) = 0;
};

class AgmMixerWrapperImpl: public AgmMixerWrapper {
public:
    int mixerOpen(unsigned int card) override {
        int ret = 0;
        mixer = mixer_open(card);
        if (!mixer) {
            std::cout << "Failed to open mixer" << std::endl;
            ret = -1;
        }
        return ret;
    }

    int mixerClose(void) override {
        mixer_close(mixer);
        return 0;
    }

    struct device_config getDeviceMediaConfig(char* filename, char *intf_name) override {
        if (get_device_media_config(BACKEND_CONF_FILE, intf_name, &deviceConfig)) {
            std::cout << "Invalid input, entry not found for :" << intf_name << std::endl;
        }

        if (deviceConfig.format != PCM_FORMAT_INVALID) {
            deviceConfig.bits = get_pcm_bit_width(deviceConfig.format);
        }
        return deviceConfig;
    }

    int setDeviceMediaConfig(char *intf_name, struct device_config *config) override {
        int ret = 0;
        ret = set_agm_device_media_config(mixer, intf_name, config);
        if (ret) {
            std::cout << "Failed to set agm device media config " << ret << std::endl;
        }
        return ret;
    }

    int setAudioInterfaceMetadata(char *intf_name, unsigned int dkv,
                                enum usecase_type usecase, int rate, int bitwidth, uint32_t stream_kv) override {
        int ret = 0;
        ret = set_agm_audio_intf_metadata(mixer, intf_name, dkv, usecase,
                                    rate, bitwidth, stream_kv);
        if (ret) {
            std::cout << "Failed to set device metadata " << ret << std::endl;
        }
        return ret;
    }

    int setStreamMetadata(int device, uint32_t stream_kv, unsigned int instance_kv) override {
        return set_agm_stream_metadata(mixer, device, stream_kv, PLAYBACK, STREAM_PCM, instance_kv);
    }

    int setStreamDeviceMetadata(int device, uint32_t stream_kv, char *intf_name,
                                unsigned int devicepp_kv) override {
        int ret = 0;

        if (devicepp_kv == 0) {
            std::cout << "There is no devicepp keyvector" << std::endl;
            return -1;
        }

        ret = set_agm_streamdevice_metadata(mixer, device, stream_kv, PLAYBACK, STREAM_PCM, intf_name,
                                        devicepp_kv);
        if (ret) {
            std::cout << "Failed to set streamdevice metadata " << ret << std::endl;
        }

        return ret;
    }

    int setDeviceCustomPayload(char *intf_name, int device, unsigned int usb_device) override {
        int ret = 0;
        unsigned int miid = 0;
        struct usbAudioConfig cfg;
        uint8_t* payload;
        size_t payloadSize;
        ret = agm_mixer_get_miid (mixer, device, intf_name, STREAM_PCM, DEVICE_HW_ENDPOINT_RX, &miid);
        if (ret) {
            std::cout << "Failed to get miid for USB_AUDIO-TX " << ret << std::endl;
            return ret;
        }

        cfg.usb_token = usb_device << 16;
        cfg.svc_interval = 0;
        get_agm_usb_audio_config_payload(&payload, &payloadSize, miid, &cfg);

        if (payloadSize) {
            ret = set_agm_device_custom_payload(mixer, intf_name, payload, payloadSize);
        } else {
            ret = -1;
            std::cout << "set_agm_device_custom_payload failed" << std::endl;
        }
        return ret;
    }

    int connectAudioInterfaceToStream(unsigned int device, char *intf_name) override {
        int ret = 0;
        ret = connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_PCM, true);
        if (ret) {
            std::cout << "Failed to connect pcm to audio interface " << ret << std::endl;
        }
        return ret;
    }

    virtual int configureMFC(int device, char *intf_name, struct device_config config) override {
        int ret = 0;
        unsigned int miid = 0;
        ret = agm_mixer_get_miid(mixer, device, intf_name, STREAM_PCM, PER_STREAM_PER_DEVICE_MFC, &miid);
        if (ret) {
            std::cout << "MFC not present for this graph " << ret << std::endl;
            return ret;
        }

        ret = configure_mfc(mixer, device, intf_name, PER_STREAM_PER_DEVICE_MFC,
                        STREAM_PCM, config.rate, config.ch,
                        config.bits, miid);
        if (ret) {
            std::cout << "Failed to configure pspd mfc " << ret << std::endl;
            return ret;
        }

        return ret;
    }

    struct group_config getGroupConfig(char *intf_name) override {
        if (isVirtualInterface(intf_name)) {
            if (get_group_device_info(BACKEND_CONF_FILE, intf_name, &groupConfig)) {
                std::cout << "Failed to get grp device config" << std::endl;
            }
        }
        return groupConfig;
    }

    int setGroupConfig(unsigned int device, char *intf_name, unsigned int device_kv, struct group_config config, unsigned int channels) override {
        int ret = 0;
        if (isVirtualInterface(intf_name)) {
            ret = set_agm_group_device_config(mixer, intf_name, &config);
            if (ret) {
                std::cout << "Failed to set grp device config " << ret << std::endl;
                return ret;
            }

            if ((device_kv == SPEAKER) || (device_kv == HANDSET)) {
                ret = set_agm_group_mux_config(mixer, device, &config, intf_name, channels);
                if (ret) {
                    std::cout << "Failed to set grp device config " << ret << std::endl;
                    return ret;
                }
            }
        }
        return ret;
    }

    int disconnectAudioInterfaceToStream(unsigned int device, char *intf_name) override {
        int ret = 0;
        ret = connect_agm_audio_intf_to_stream(mixer, device, intf_name, STREAM_PCM, false);
        if (ret) {
            std::cout << "Failed to disconnect pcm to audio interface " << ret << std::endl;
        }
        return ret;
    }

private:
    char* isVirtualInterface(char *intfName) {
        return strstr(intfName, "VIRT-");
    }
};

#endif
