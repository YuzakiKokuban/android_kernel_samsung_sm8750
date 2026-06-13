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

#include <iostream>
#include "PlaybackCommand.h"

void PlaybackCommand::setCard(char **argv)
{
    argv++;
    if (*argv)
        card = atoi(*argv);
}

void PlaybackCommand::setDevice(char **argv)
{
    argv++;
    if (*argv)
        device = atoi(*argv);
}

void PlaybackCommand::setInterfaceNumber(char **argv)
{
    argv++;
    if (*argv)
        intf_num = atoi(*argv);
}

void PlaybackCommand::setStreamKeyVector(char **argv)
{
    argv++;
    if (*argv)
        stream_kv = convert_char_to_hex(*argv);
}

void PlaybackCommand::setInstanceKeyVector(char **argv)
{
    argv++;
    if (*argv) {
        instance_kv = atoi(*argv);
    }
}

void PlaybackCommand::setHaptics(char **argv)
{
    argv++;
    if (*argv)
        haptics = atoi(*argv);
}

void PlaybackCommand::setInterfaceName(char **argv)
{
    intf_name = (char**) malloc(intf_num * sizeof(char*));
    if (!intf_name) {
        std::cout << "insufficient memory" << std::endl;
        exit(1);
    }

    for (int i = 0; i < intf_num ; i++){
        argv++;
        if (*argv)
            intf_name[i] = *argv;
    }
}

void PlaybackCommand::set24LE(char **argv)
{
    argv++;
    if (*argv) {
        is_24_LE = atoi(*argv);
    }
}

void PlaybackCommand::setDeviceppKeyVector(char **argv)
{
    devicepp_kv = new unsigned int[intf_num];
    if (!devicepp_kv) {
        std::cout << "insufficient memory" << std::endl;
        exit(1);
    }

    for (int i = 0; i < intf_num ; i++) {
        devicepp_kv[i] = DEVICEPP_RX_AUDIO_MBDRC;
    }

    for (int i = 0; i < intf_num ; i++)
    {
        argv++;
        if (*argv) {
            devicepp_kv[i] = convert_char_to_hex(*argv);
        }
    }
}

void PlaybackCommand::setDeviceKeyVector(char **argv)
{
    device_kv = new unsigned int[intf_num];
    if (!device_kv) {
        std::cout << "insufficient memory" << std::endl;
        exit(1);
    }
    for (int i = 0; i < intf_num ; i++) {
        argv++;
        if (*argv) {
            device_kv[i] = convert_char_to_hex(*argv);
        }
    }
}

void PlaybackCommand::setChannel(char **argv)
{
    argv++;
    if (*argv)
        channels = atoi(*argv);
}

void PlaybackCommand::setSampleRate(char **argv)
{
    argv++;
    if (*argv)
        rate = atoi(*argv);
}

void PlaybackCommand::setBitWidth(char **argv)
{
    argv++;
    if (*argv)
        bits = atoi(*argv);
}

void PlaybackCommand::setUsbDevice(char **argv)
{
    argv++;
    if (*argv)
        usb_device = atoi(*argv);
}

unsigned int PlaybackCommand::getCard(void)
{
    return card;
}

unsigned int PlaybackCommand::getDevice(void)
{
    return device;
}

int PlaybackCommand::getInterfaceNumber()
{
    return intf_num;
}

unsigned int PlaybackCommand::getStreamKeyVector()
{
    return stream_kv;
}

unsigned int PlaybackCommand::getInstanceKeyVector()
{
    return instance_kv;
}

bool PlaybackCommand::getHaptics()
{
    return haptics;
}

char **PlaybackCommand::getInterfaceName()
{
    return intf_name;
}

bool PlaybackCommand::is24LE()
{
    return is_24_LE;
}

unsigned int *PlaybackCommand::getDeviceppKeyVector()
{
    return devicepp_kv;
}

unsigned int *PlaybackCommand::getDeviceKeyVector()
{
    return device_kv;
}

unsigned int PlaybackCommand::getChannel()
{
    return channels;
}

unsigned int PlaybackCommand::getSampleRate()
{
    return rate;
}

unsigned int PlaybackCommand::getBitWidth()
{
    return bits;
}

unsigned int PlaybackCommand::getUsbDevice()
{
    return usb_device;
}