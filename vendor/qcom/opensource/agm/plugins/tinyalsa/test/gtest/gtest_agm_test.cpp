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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <iostream>
#include "../AgmPlayer.h"
#include "../PlaybackCommandParser.h"
#include "../MockAgmMixerWrapper.h"
#include "../MockAgmPcmWrapper.h"

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        riffWaveParser = new RiffWaveParser();
        chunkParser = new ChunkParser();
    }

    void TearDown() override {
        delete riffWaveParser;
        delete chunkParser;
    }

    std::ifstream file;
    HeaderParser* riffWaveParser;
    HeaderParser* chunkParser;
};

TEST_F(ParserTest, RiffWavParserIsValidFail) {
    file.open("/sdcard/sample.m4a", std::ios::binary);
    if (!file) {
        std::cout << "please, push the sample.m4a file in /sdcard/ folder" << std::endl;
        exit(1);
    }
    riffWaveParser->parseHeader(file);
    EXPECT_FALSE(riffWaveParser->isValid());
    file.close();
}

TEST_F(ParserTest, RiffWavParserIsValidSuccess) {
    file.open("/sdcard/sample.wav", std::ios::binary);
    if (!file) {
        std::cout << "please, push the sample.wav file in /sdcard/ folder" << std::endl;
        exit(1);
    }
    riffWaveParser->parseHeader(file);
    EXPECT_TRUE(riffWaveParser->isValid());
    file.close();
}

TEST_F(ParserTest, ChunkParserParseHeaderSuccess) {
    ChunkFormat chunkFormat;

    file.open("/sdcard/sample.wav", std::ios::binary);
    if (!file) {
        std::cout << "please, push the sample.wav file in /sdcard/ folder" << std::endl;
        exit(1);
    }
    riffWaveParser->parseHeader(file);
    EXPECT_EQ(true, riffWaveParser->isValid());

    chunkParser->parseHeader(file);
    chunkFormat = chunkParser->getFormat();
    EXPECT_EQ(1, chunkFormat.audio_format);
    EXPECT_EQ(2, chunkFormat.num_channels);
    EXPECT_EQ(48000, chunkFormat.sample_rate);
    EXPECT_EQ(192000, chunkFormat.byte_rate);
    EXPECT_EQ(4, chunkFormat.block_align);
    EXPECT_EQ(16, chunkFormat.bits_per_sample);
    file.close();
}

class PlaybackCommandParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        command = new char*[30];
    }

    void TearDown() override {
        if (command) {
            delete[] command;
            command = nullptr;
        }
    }

    void makeTestCommand() {
        memset(command, 0, sizeof(command));
        command[0] = "agmplay";
        command[1] = "/sdcard/48.wav";
        command[2] = "-d";
        command[3] = "100";
        command[4] = "-D";
        command[5] = "100";
        command[6] = "-num_intf";
        command[7] = "1";
        command[8] = "-i";
        command[9] = "TDM-LPAIF-RX-PRIMARY";
        command[10] = "-h";
        command[11] = "false";
        command[12] = "-dkv";
        command[13] = "0xA2000001";
        command[14] = "-skv";
        command[15] = "0";
        command[16] = "-ikv";
        command[17] = "1";
        command[18] = "-dppkv";
        command[19] = "0xAC000002";
        command[20] = "-is_24_LE";
        command[21] = "false";
        command[22] = "-c";
        command[23] = "2";
        command[24] = "-r";
        command[25] = "48000";
        command[26] = "-b";
        command[27] = "16";
        command[28] = "-usb_d";
        command[29] = "1";
    }

    PlaybackCommandParser parser;
    char** command = nullptr;
    char **interfaceName = nullptr;
    unsigned int *device_kv = nullptr;
    unsigned int *devicepp_kv = nullptr;
};

TEST_F(PlaybackCommandParserTest, TestParseCommandLine) {
    makeTestCommand();
    parser.parseCommandLine(command);

    device_kv = parser.getPlaybackCommand().getDeviceKeyVector();
    devicepp_kv = parser.getPlaybackCommand().getDeviceppKeyVector();
    interfaceName = parser.getPlaybackCommand().getInterfaceName();

    EXPECT_EQ(100, parser.getPlaybackCommand().getCard());
    EXPECT_EQ(100, parser.getPlaybackCommand().getDevice());
    EXPECT_EQ(1, parser.getPlaybackCommand().getInterfaceNumber());
    EXPECT_STREQ("TDM-LPAIF-RX-PRIMARY", interfaceName[0]);
    EXPECT_FALSE(parser.getPlaybackCommand().getHaptics() );
    EXPECT_EQ(0xA2000001, *device_kv);
    EXPECT_EQ(0, parser.getPlaybackCommand().getStreamKeyVector());
    EXPECT_EQ(1, parser.getPlaybackCommand().getInstanceKeyVector());
    EXPECT_EQ(0xAC000002, *devicepp_kv);
    EXPECT_FALSE(parser.getPlaybackCommand().is24LE());
    EXPECT_EQ(2, parser.getPlaybackCommand().getChannel());
    EXPECT_EQ(48000, parser.getPlaybackCommand().getSampleRate());
    EXPECT_EQ(16, parser.getPlaybackCommand().getBitWidth());
    EXPECT_EQ(1, parser.getPlaybackCommand().getUsbDevice());
}

class AgmPlayTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        riffWaveParser = new RiffWaveParser();
        chunkParser = new ChunkParser();
        command = new char*[30];

        file.open("/sdcard/sample.wav", std::ios::binary);
        if (!file) {
            std::cout << "please, push the sample.wav file in /sdcard/ folder" << std::endl;
            exit(1);
        }
        riffWaveParser->parseHeader(file);
        if (!riffWaveParser->isValid()) {
            std::cout << "It is not a riff/wave file" << std::endl;
            file.close();
            exit(1);
        }
        chunkParser->parseHeader(file);
        makeTestCommand();
        parser.parseCommandLine(command);
        agmPlayer = new AgmPlayer(&mockAgmMixer, &mockAgmPcm);
    }

    void TearDown() override {
        delete agmPlayer;
        delete riffWaveParser;
        delete chunkParser;
        if (command) {
            delete[] command;
            command == nullptr;
        }
        file.close();
    }

    void makeTestCommand() {
        memset(command, 0, sizeof(command));
        command[0] = "agmplay";
        command[1] = "/sdcard/48.wav";
        command[2] = "-d";
        command[3] = "100";
        command[4] = "-D";
        command[5] = "100";
        command[6] = "-i";
        command[7] = "TDM-LPAIF-RX-PRIMARY";
        command[8] = "-dkv";
        command[9] = "0xA2000001";
    }

    char** command = nullptr;
    std::ifstream file;
    HeaderParser* riffWaveParser;
    HeaderParser* chunkParser;
    PlaybackCommandParser parser;
    AgmPlayer *agmPlayer;
    MockAgmMixerWrapper mockAgmMixer;
    MockAgmPcmWrapper mockAgmPcm;
};

TEST_F(AgmPlayTestFixture, openMixerFail) {
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, setDeviceMediaConfigFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, setAudioInterfaceMetadataFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, setStreamConfigFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, setStreamDeviceMetadataFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, connectAudioInterfaceToStreamFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, configureMFCFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, configureMFC(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, setGroupConfigFail) {
    struct device_config deviceConfig;
    struct group_config groupConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, configureMFC(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getGroupConfig(testing::_))
       .WillOnce(testing::Return(groupConfig));
    EXPECT_CALL(mockAgmMixer, setGroupConfig(testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, pcmOpenFail) {
    struct device_config deviceConfig;
    struct group_config groupConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, configureMFC(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getGroupConfig(testing::_))
       .WillOnce(testing::Return(groupConfig));
    EXPECT_CALL(mockAgmMixer, setGroupConfig(testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmOpen(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, pcmStartFail) {
    struct device_config deviceConfig;
    struct group_config groupConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, configureMFC(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getGroupConfig(testing::_))
       .WillOnce(testing::Return(groupConfig));
    EXPECT_CALL(mockAgmMixer, setGroupConfig(testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmOpen(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmFramesToBytes())
       .WillOnce(testing::Return(16384));
    EXPECT_CALL(mockAgmPcm, pcmStart())
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, pcmWriteFail) {
    struct device_config deviceConfig;
    struct group_config groupConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, configureMFC(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getGroupConfig(testing::_))
       .WillOnce(testing::Return(groupConfig));
    EXPECT_CALL(mockAgmMixer, setGroupConfig(testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmOpen(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmFramesToBytes())
       .WillOnce(testing::Return(16384));
    EXPECT_CALL(mockAgmPcm, pcmStart())
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmWrite(testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayTestFixture, playSampleSuccess) {
    struct device_config deviceConfig;
    struct group_config groupConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(deviceConfig));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, connectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, configureMFC(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, getGroupConfig(testing::_))
       .WillOnce(testing::Return(groupConfig));
    EXPECT_CALL(mockAgmMixer, setGroupConfig(testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmOpen(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmFramesToBytes())
       .WillOnce(testing::Return(16384));
    EXPECT_CALL(mockAgmPcm, pcmStart())
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmWrite(testing::_, testing::_))
        .WillRepeatedly(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmStop())
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, disconnectAudioInterfaceToStream(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmPcm, pcmClose())
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(0, agmPlayer->playSample(file, format, command));
}

class AgmPlayUSBTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        riffWaveParser = new RiffWaveParser();
        chunkParser = new ChunkParser();
        command = new char*[30];

        file.open("/sdcard/sample.wav", std::ios::binary);
        if (!file) {
            std::cout << "please, push the sample.wav file in /sdcard/ folder" << std::endl;
            exit(1);
        }
        riffWaveParser->parseHeader(file);
        if (!riffWaveParser->isValid()) {
            std::cout << "It is not a riff/wave file" << std::endl;
            file.close();
            exit(1);
        }
        chunkParser->parseHeader(file);

        makeTestCommand();
        parser.parseCommandLine(command);

        agmPlayer = new AgmPlayer(&mockAgmMixer, &mockAgmPcm);
    }

    void TearDown() override {
        delete agmPlayer;
        delete riffWaveParser;
        delete chunkParser;
        if (command) {
            delete[] command;
            command == nullptr;
        }
        file.close();
    }

    void makeTestCommand() {
        memset(command, 0, sizeof(command));
        command[0] = "agmplay";
        command[1] = "/sdcard/48.wav";
        command[2] = "-d";
        command[3] = "100";
        command[4] = "-D";
        command[5] = "100";
        command[6] = "-i";
        command[7] = "USB_AUDIO-RX";
        command[8] = "-dkv";
        command[9] = "0xA2000001";
    }

    char** command = nullptr;
    std::ifstream file;
    HeaderParser* riffWaveParser;
    HeaderParser* chunkParser;
    PlaybackCommandParser parser;
    AgmPlayer *agmPlayer;
    MockAgmMixerWrapper mockAgmMixer;
    MockAgmPcmWrapper mockAgmPcm;
};

TEST_F(AgmPlayUSBTestFixture, setUSBDeviceMediaConfigFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}

TEST_F(AgmPlayUSBTestFixture, setDeviceCustomPayloadFail) {
    struct device_config deviceConfig;
    ChunkFormat format = chunkParser->getFormat();
    PlaybackCommand command = parser.getPlaybackCommand();

    EXPECT_CALL(mockAgmMixer, mixerOpen(testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setDeviceMediaConfig(testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setAudioInterfaceMetadata(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamMetadata(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setStreamDeviceMetadata(testing::_, testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(0));
    EXPECT_CALL(mockAgmMixer, setDeviceCustomPayload(testing::_, testing::_, testing::_))
       .WillOnce(testing::Return(-1));
    EXPECT_CALL(mockAgmMixer, mixerClose())
       .WillOnce(testing::Return(0));

    EXPECT_EQ(-1 , agmPlayer->playSample(file, format, command));
}
