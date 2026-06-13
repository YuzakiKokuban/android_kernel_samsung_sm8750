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

#ifndef __MOCK_AGMMIXERWRAPPER_H__
#define __MOCK_AGMMIXERWRAPPER_H__

#include "AgmMixerWrapper.h"

class MockAgmMixerWrapper : public AgmMixerWrapper {
  public:
    MOCK_METHOD(int, mixerOpen, (unsigned int card), (override));
    MOCK_METHOD(int, mixerClose, (), (override));
    MOCK_METHOD(struct device_config, getDeviceMediaConfig, (char* filename, char *intf_name), (override));
    MOCK_METHOD(int, setDeviceMediaConfig, (char *intf_name, struct device_config *config), (override));
    MOCK_METHOD(int, setAudioInterfaceMetadata, (char *intf_name, unsigned int dkv, enum usecase_type usecase, int rate, int bitwidth, uint32_t stream_kv), (override));
    MOCK_METHOD(int, setStreamMetadata, (int device, uint32_t stream_kv, unsigned int instance_kv), (override));
    MOCK_METHOD(int, setStreamDeviceMetadata, (int device, uint32_t stream_kv, char *intf_name, unsigned int devicepp_kv), (override));
    MOCK_METHOD(int, connectAudioInterfaceToStream, (unsigned int device, char *intf_name), (override));
    MOCK_METHOD(int, configureMFC, (int device, char *intf_name, struct device_config), (override));
    MOCK_METHOD(struct group_config, getGroupConfig, (char *intf_name), (override));
    MOCK_METHOD(int, setGroupConfig, (unsigned int device, char *intf_name, unsigned int device_kv, struct group_config config, unsigned int channels), (override));
    MOCK_METHOD(int, setDeviceCustomPayload, (char *intf_name, int device, unsigned int usb_device), (override));
    MOCK_METHOD(int, disconnectAudioInterfaceToStream, (unsigned int device, char *intf_name), (override));
};

#endif
