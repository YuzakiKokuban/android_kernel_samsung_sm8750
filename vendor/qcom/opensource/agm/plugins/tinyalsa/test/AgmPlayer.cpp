/*
** Copyright (c) 2019, 2021, The Linux Foundation. All rights reserved.
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
**
** Changes from Qualcomm Innovation Center are provided under the following license:
** Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
** SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "AgmPlayer.h"
#include "SignalHandler.h"

int AgmPlayer::playSample(std::ifstream& file, ChunkFormat format, PlaybackCommand playbackCommand)
{
    int ret = 0;

    getPlaybackInfo(&playbackCommand);
    setPlaybackInfo(format);

    ret = openMixer(card);
    if (ret < 0) {
        return ret;
    }

    allocConfigMemory();
    ret = setDeviceConfig();
    if (ret < 0) {
        goto err;
    }

    ret = setStreamConfig();
    if (ret < 0) {
        goto err;
    }

    ret = setDevicePostProcessingConfig();
    if (ret < 0) {
        goto err;
    }

    ret = startPlayback(file);
    if (ret < 0) {
        goto err;
    }
    stopPlayback();

err:
    deallocConfigMemory();
    closeMixer();
    return ret;
}

void AgmPlayer::getPlaybackInfo(PlaybackCommand* playbackCommand)
{
    card = playbackCommand->getCard();
    device = playbackCommand->getDevice();
    device_kv = playbackCommand->getDeviceKeyVector();
    stream_kv = playbackCommand->getStreamKeyVector();
    instance_kv = playbackCommand->getInstanceKeyVector();
    devicepp_kv = playbackCommand->getDeviceppKeyVector();
    haptics = playbackCommand->getHaptics();
    intf_name = playbackCommand->getInterfaceName();
    intf_num = playbackCommand->getInterfaceNumber();
    is_24_LE = playbackCommand->is24LE();
    usb_device = playbackCommand->getUsbDevice();
    channels = playbackCommand->getChannel();
    rate = playbackCommand->getSampleRate();
    bits = playbackCommand->getBitWidth();
}

void AgmPlayer::setPlaybackInfo(ChunkFormat format)
{
    config.channels = format.num_channels;
    config.rate = format.sample_rate;

    switch (format.bits_per_sample) {
        case 32:
            config.format = is_24_LE? PCM_FORMAT_S24_LE : PCM_FORMAT_S32_LE;
            break;
        case 24:
            config.format = PCM_FORMAT_S24_3LE;
            break;
        case 16:
            config.format = PCM_FORMAT_S16_LE;
            break;
        default:
            std::cout << "Unsupported bit width" << std::endl;
            break;
    }

    if (haptics) {
        playback_path = HAPTICS;
        stream_kv = stream_kv ? stream_kv : HAPTICS_PLAYBACK;
    } else {
        playback_path = PLAYBACK;
        stream_kv = stream_kv ? stream_kv : PCM_LL_PLAYBACK;
    }
}

int AgmPlayer::openMixer(unsigned int card)
{
    return agmMixer->mixerOpen(card);
}

int AgmPlayer::closeMixer(void)
{
    return agmMixer->mixerClose();
}

void AgmPlayer::allocConfigMemory(void)
{
    dev_config = new device_config[intf_num];
    if (!dev_config) {
        std::cout << "Failed to allocate memory for dev config" << std::endl;
        return;
    }

    grp_config = new group_config[intf_num];
    if (!grp_config) {
        std::cout << "Failed to allocate memory for group config" << std::endl;
        return;
    }
}

void AgmPlayer::deallocConfigMemory(void)
{
    if (dev_config) {
        delete[] dev_config;
        dev_config = nullptr;
    }

    if (grp_config) {
        delete[] grp_config;
        grp_config = nullptr;
    }
}

bool AgmPlayer::isUSBInterface(int index)
{
    if(intf_name[index] != NULL && strcmp(intf_name[index], "USB_AUDIO-RX") == 0) {
        return true;
    }
    return false;
}

void AgmPlayer::getDeviceMediaConfig(int index)
{
    if(isUSBInterface(index)) {
        dev_config[index].rate = rate;
        dev_config[index].ch = channels;
        dev_config[index].bits = bits;
    }
    else {
        dev_config[index] = agmMixer->getDeviceMediaConfig(BACKEND_CONF_FILE, intf_name[index]);
    }
}

int AgmPlayer::setDeviceConfig(void)
{
    int ret = 0;

    for (int index = 0; index < intf_num; index++) {
        getDeviceMediaConfig(index);

        ret = agmMixer->setDeviceMediaConfig(intf_name[index], &dev_config[index]);
        if (ret)
            return ret;

        ret = agmMixer->setAudioInterfaceMetadata(intf_name[index], device_kv[index], playback_path,
                                    dev_config[index].rate, dev_config[index].bits, stream_kv);
        if (ret)
            return ret;
    }

    return ret;
}

int AgmPlayer::setStreamConfig(void)
{
    return agmMixer->setStreamMetadata(device, stream_kv, instance_kv);
}

int AgmPlayer::setDevicePostProcessingConfig(void)
{
    int ret = 0;

    for (int index = 0; index < intf_num; index++) {
        ret = agmMixer->setStreamDeviceMetadata(device, stream_kv, intf_name[index], devicepp_kv[index]);
        if (ret)
            return ret;

        if (isUSBInterface(index)) {
            ret = agmMixer->setDeviceCustomPayload(intf_name[index], device, usb_device);
            if (ret)
                return ret;
        }

        ret = agmMixer->connectAudioInterfaceToStream(device, intf_name[index]);
        if (ret)
            return ret;

        ret = agmMixer->configureMFC(device, intf_name[index], dev_config[index]);
        if (ret)
            return ret;

        grp_config[index] = agmMixer->getGroupConfig(intf_name[index]);
        ret = agmMixer->setGroupConfig(device, intf_name[index], device_kv[index], grp_config[index], dev_config[index].ch);
        if (ret)
            return ret;
    }

    return ret;
}

int AgmPlayer::startPlayback(std::ifstream& file)
{
    int size = 0;
    int num_read = 0;
    int ret = 0;
    char *buffer;
    SignalHandler stream;

    ret = agmPcm->pcmOpen(card, device, &config);
    if (ret < 0)
        return ret;

    size = agmPcm->pcmFramesToBytes();
    buffer = new char[size];
    if (!buffer) {
        std::cout << "Unable to allocate " << size << " bytes" << std::endl;
        return -ENOMEM;
    }

    ret = agmPcm->pcmStart();
    if (ret < 0)
        goto err;

    stream.open();
    do {
        file.read(buffer, size);
        num_read = file.gcount();
        if (num_read > 0) {
            ret = agmPcm->pcmWrite(buffer, num_read);
            if (ret < 0)
                break;
        }
    } while (!stream.isClosed() && num_read > 0);

err:
    if (buffer) {
        delete[] buffer;
        buffer = nullptr;
    }
    return ret;
}

void AgmPlayer::stopPlayback(void)
{
    agmPcm->pcmStop();
    for (int index = 0; index < intf_num; index++) {
        agmMixer->disconnectAudioInterfaceToStream(device, intf_name[index]);
    }
    agmPcm->pcmClose();
}

AgmPlayer::AgmPlayer()
{
    agmMixer = new AgmMixerWrapperImpl();
    agmPcm = new AgmPcmWrapperImpl();

    memset(&config, 0, sizeof(config));
    config.period_size = 1024;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
};

AgmPlayer::AgmPlayer(AgmMixerWrapper *agmMixer, AgmPcmWrapper *agmPcm)
  : agmMixer(agmMixer), agmPcm(agmPcm)
{
    memset(&config, 0, sizeof(config));
    config.period_size = 1024;
    config.period_count = 4;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
}
