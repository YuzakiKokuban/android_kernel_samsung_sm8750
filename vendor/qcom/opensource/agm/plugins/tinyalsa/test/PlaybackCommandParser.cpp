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

#include "PlaybackCommandParser.h"

void PlaybackCommandParser::parseCommandLine(char **argv)
{
    argv += 2;
    while (*argv) {
        if (strcmp(*argv, "-d") == 0) {
            playbackCommand.setDevice(argv);
        } else if (strcmp(*argv, "-D") == 0) {
            playbackCommand.setCard(argv);
        } else if (strcmp(*argv, "-num_intf") == 0) {
            playbackCommand.setInterfaceNumber(argv);
        } else if (strcmp(*argv, "-i") == 0) {
            playbackCommand.setInterfaceName(argv);
        } else if (strcmp(*argv, "-h") == 0) {
            playbackCommand.setHaptics(argv);
        } else if (strcmp(*argv, "-dkv") == 0) {
            playbackCommand.setDeviceKeyVector(argv);
        } else if (strcmp(*argv, "-skv") == 0) {
            playbackCommand.setStreamKeyVector(argv);
        } else if (strcmp(*argv, "-ikv") == 0) {
            playbackCommand.setInstanceKeyVector(argv);
        } else if (strcmp(*argv, "-dppkv") == 0) {
            playbackCommand.setDeviceppKeyVector(argv);
        } else if (strcmp(*argv, "-is_24_LE") == 0) {
            playbackCommand.set24LE(argv);
        } else if (strcmp(*argv, "-c") == 0) {
            playbackCommand.setChannel(argv);
        } else if (strcmp(*argv, "-r") == 0) {
            playbackCommand.setSampleRate(argv);
        } else if (strcmp(*argv, "-b") == 0) {
            playbackCommand.setBitWidth(argv);
        } else if (strcmp(*argv, "-usb_d") == 0) {
            playbackCommand.setUsbDevice(argv);
        } else if (strcmp(*argv, "-help") == 0) {
            usage();
        }

        if (*argv)
            argv++;
    }
}

PlaybackCommand& PlaybackCommandParser::getPlaybackCommand() {
    return playbackCommand;
}

void PlaybackCommandParser::usage(void)
{
    std::cout << "Usage: %s file.wav [-help print usage] [-D card] [-d device]" << std::endl;
    std::cout << "[-c channels] [-r rate] [-b bits]" << std::endl;
    std::cout << " [-num_intf num of interfaces followed by interface name]" << std::endl;
    std::cout << " [-i intf_name] : Can be multiple if num_intf is more than 1" << std::endl;
    std::cout << " [-dkv device_kv] : Can be multiple if num_intf is more than 1" << std::endl;
    std::cout << " [-dppkv deviceppkv] : Assign 0 if no device pp in the graph" << std::endl;
    std::cout << " [-ikv instance_kv] :  Assign 0 if no instance kv in the graph" << std::endl;
    std::cout << " [-skv stream_kv] [-h haptics usecase]" << std::endl;
    std::cout << " [is_24_LE] : [0-1] Only to be used if user wants to play S24_LE clip" << std::endl;
    std::cout << " [-usb_d usb device]" << std::endl;
    std::cout << " 0: If clip bps is 32, and format is S32_LE" << std::endl;
    std::cout << " 1: If clip bps is 24, and format is S24_LE" << std::endl;
}
