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

#ifndef __PLAYBACKCOMMAND_H__
#define __PLAYBACKCOMMAND_H__

#include "agmmixer.h"

class PlaybackCommand {
private:
    unsigned int card;
    unsigned int device;
    int intf_num;
    unsigned int stream_kv;
    unsigned int instance_kv;
    unsigned int *devicepp_kv;
    unsigned int *device_kv;
    unsigned int usb_device;
    unsigned int channels;
    unsigned int rate;
    unsigned int bits;
    char **intf_name;
    bool haptics;
    bool is_24_LE;

public:
    PlaybackCommand()
        : card(100),
          device(100),
          intf_num(1),
          stream_kv(0),
          instance_kv(INSTANCE_1),
          haptics(false),
          intf_name(nullptr),
          is_24_LE(false),
          devicepp_kv(nullptr),
          device_kv(nullptr),
          usb_device(1),
          channels(2),
          rate(48000),
          bits(16)
    {
        devicepp_kv = new unsigned int[intf_num];
        device_kv = new unsigned int[intf_num];

        if (!device_kv || !devicepp_kv) {
            std::cout << " insufficient memory" << std::endl;
            exit(1);
        }

        device_kv[0] = SPEAKER;
        devicepp_kv[0] = DEVICEPP_RX_AUDIO_MBDRC;
    }

    ~PlaybackCommand()
    {
        if (!devicepp_kv) {
            delete[] devicepp_kv;
            devicepp_kv = nullptr;
        }
        if (!device_kv) {
            delete[] device_kv;
            device_kv = nullptr;
        }
    }

    void setCard(char **argv);
    void setDevice(char **argv);
    void setInterfaceNumber(char **argv);
    void setStreamKeyVector(char **argv);
    void setInstanceKeyVector(char **argv);
    void setHaptics(char **argv);
    void setInterfaceName(char **argv);
    void set24LE(char **argv);
    void setDeviceKeyVector(char **argv);
    void setDeviceppKeyVector(char **argv);
    void setChannel(char **argv);
    void setSampleRate(char **argv);
    void setBitWidth(char **argv);
    void setUsbDevice(char **argv);

    unsigned int getCard();
    unsigned int getDevice();
    int getInterfaceNumber();
    unsigned int getStreamKeyVector();
    unsigned int getInstanceKeyVector();
    bool getHaptics();
    char **getInterfaceName();
    bool is24LE();
    unsigned int *getDeviceKeyVector();
    unsigned int *getDeviceppKeyVector();
    unsigned int getChannel();
    unsigned int getSampleRate();
    unsigned int getBitWidth();
    unsigned int getUsbDevice();
};

#endif
