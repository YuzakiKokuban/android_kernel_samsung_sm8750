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
#include "PlaybackCommandParser.h"

int main(int argc, char **argv)
{
    std::ifstream file;
    HeaderParser* riffWaveParser = new RiffWaveParser();
    HeaderParser* chunkParser = new ChunkParser();
    PlaybackCommandParser playbackCommandParser;

    if (argc < 3) {
        playbackCommandParser.usage();
        exit(1);
    }

    file.open(argv[1], std::ios::binary);
    if (!file) {
        std::cout << "Unable to open file" << std::endl;
        exit(1);
    }

    riffWaveParser->parseHeader(file);
    if (!riffWaveParser->isValid()) {
        std::cout << "It is not a riff/wave file" << std::endl;
        file.close();
        exit(1);
    }
    
    chunkParser->parseHeader(file);
    playbackCommandParser.parseCommandLine(argv);

    if (playbackCommandParser.getPlaybackCommand().getInterfaceName() == nullptr) {
        std::cout << "interface name is NULL" << std::endl;
        file.close();
        exit(1);
    }

    AgmPlayer agmPlayer;
    agmPlayer.playSample(file, chunkParser->getFormat(), playbackCommandParser.getPlaybackCommand());

    file.close();
    return 0;
}
