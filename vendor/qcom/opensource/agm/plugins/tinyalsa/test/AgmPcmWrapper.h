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

#ifndef __AGMPCMWRAPPER_H__
#define __AGMPCMWRAPPER_H__

#include <iostream>
#include "agmmixer.h"

class AgmPcmWrapper {
protected:
    struct pcm *pcm;

public:
    virtual ~AgmPcmWrapper() = default;
    virtual int pcmOpen(unsigned int card, unsigned int device, struct pcm_config *config) = 0;
    virtual int pcmFramesToBytes(void) = 0;
    virtual int pcmStart(void) = 0;
    virtual int pcmWrite(char *buffer, int num_read) = 0;
    virtual int pcmStop(void) = 0;
    virtual int pcmClose(void) = 0;
};

class AgmPcmWrapperImpl: public AgmPcmWrapper {
public:
    int pcmOpen(unsigned int card, unsigned int device, struct pcm_config *config) override {
        int ret = 0;
        pcm = pcm_open(card, device, PCM_OUT, config);
        if (!pcm || !pcm_is_ready(pcm)) {
            std::cout << "Unable to open PCM device " << device << " (" << pcm_get_error(pcm) << ")" << std::endl;
            ret = -1;
        }
        return ret;
    }

    int pcmFramesToBytes(void) override {
        return pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    }

    int pcmStart(void) override {
        int ret = 0;
        ret = pcm_start(pcm);
        if (ret < 0) {
            std::cout << "start error" << std::endl;
            pcm_close(pcm);
        }
        return ret;
    }

    int pcmWrite(char *buffer, int num_read) override {
        int ret = 0;
        ret = pcm_write(pcm, buffer, num_read);
        if (ret < 0) {
            std::cout << "Error playing sample" << std::endl;
        }
        return ret;
    }

    int pcmStop(void) override {
        return pcm_stop(pcm);
    }

    int pcmClose(void) override {
        return pcm_close(pcm);
    }
};

#endif
