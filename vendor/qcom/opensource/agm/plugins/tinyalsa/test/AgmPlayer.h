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

#ifndef __AGMPLAY_H__
#define __AGMPLAY_H__

#include <iostream>
#include "ChunkParser.h"
#include "RiffWaveParser.h"
#include "PlaybackCommand.h"
#include "AgmMixerWrapper.h"
#include "AgmPcmWrapper.h"


class AgmPlayer {
public:
    AgmPlayer();
    AgmPlayer(AgmMixerWrapper *agmMixer, AgmPcmWrapper *agmPcm);
    ~AgmPlayer() {};
    int playSample(std::ifstream& file, ChunkFormat fmt, PlaybackCommand playbackCommand);

private:
    unsigned int card;
    unsigned int device;
    unsigned int *device_kv;
    unsigned int stream_kv;
    unsigned int instance_kv;
    unsigned int *devicepp_kv;
    unsigned int usb_device;
    unsigned int channels;
    unsigned int rate;
    unsigned int bits;
    bool haptics;
    char **intf_name;
    int intf_num;
    bool is_24_LE;
    struct pcm_config config;
    struct group_config *grp_config;
    struct device_config *dev_config;
    enum usecase_type playback_path;
    AgmMixerWrapper *agmMixer;
    AgmPcmWrapper *agmPcm;

    int openMixer(unsigned int card);
    int closeMixer(void);
    void getPlaybackInfo(PlaybackCommand* playbackCommand);
    void setPlaybackInfo(ChunkFormat format);
    void allocConfigMemory(void);
    void deallocConfigMemory(void);
    bool isUSBInterface(int index);
    void getDeviceMediaConfig(int index);
    int setDeviceConfig(void);
    int setStreamConfig(void);
    int setDevicePostProcessingConfig(void);
    int startPlayback(std::ifstream& file);
    void stopPlayback(void);
};

#endif
