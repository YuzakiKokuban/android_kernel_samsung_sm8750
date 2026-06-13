/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following
 * license:
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
Changes from Qualcomm Innovation Center are provided under the following license:
Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SESSION_ALSAVOICE_H
#define SESSION_ALSAVOICE_H

#include "PayloadBuilder.h"
#include "Session.h"
#include "PalAudioRoute.h"
#include "vcpm_api.h"
#include <tinyalsa/asoundlib.h>
#include <thread>

class Stream;
class Session;

class SessionAlsaVoice : public Session
{
private:
    PayloadBuilder* builder;
    struct pcm *pcmRx;
    struct pcm *pcmTx;
    size_t in_buf_size, in_buf_count, out_buf_size, out_buf_count;
    std::vector<int> pcmDevRxIds;
    std::vector<int> pcmDevTxIds;
    std::shared_ptr<Device> dev = nullptr;
    std::vector <std::pair<int, int>> gkv;
    std::vector <std::pair<int, int>> ckv;
    std::vector <std::pair<int, int>> tkv;
    std::thread threadHandler;
    uint32_t vsid = 0x11C0500; /*defualt*/
    float default_volume = 0.4;
    int max_vol_index = -1;
    uint32_t ttyMode = PAL_TTY_OFF;
    bool volume_boost = vol_boost_disable;
    bool slow_talk = false;
    bool hd_voice = false;
    pal_device_mute_t dev_mute = {};
    int sideTone_cnt = 0;
    sessionState mState;
    bool ssr_trigger_enable = false;

#ifdef SEC_AUDIO_CALL_VOIP
    int mic_mode = STANDARD_MODE;
#endif

public:

    SessionAlsaVoice(std::shared_ptr<ResourceManager> Rm);
    ~SessionAlsaVoice();
    int open(Stream * s) override;
    int prepare(Stream * s) override;
    int setConfig(Stream * s, configType type, int tag = 0) override;
    int setConfig(Stream * s, configType type, int tag = 0, int dir = 0) override;
#ifdef SEC_AUDIO_CALL
    int getParameters(Stream *s, int tagId, uint32_t param_id, void **payload) override;
#endif
    int setParameters(Stream *streamHandle, int tagId, uint32_t param_id,
                      void *payload) override;
    int setSessionParameters(Stream *s, int dir);
    int start(Stream * s) override;
    int stop(Stream * s) override;
    int close(Stream * s) override;
    int setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int disconnectSessionDevice(Stream *streamHandle,
                                pal_stream_type_t streamType,
                                std::shared_ptr<Device> deviceToDisconnect);
    int connectSessionDevice(Stream* streamHandle,
                             pal_stream_type_t streamType,
                             std::shared_ptr<Device> deviceToConnect);
    int setECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable) {return 0;};
    bool isActive();
private:
    int payloadCalKeys(Stream * s, uint8_t **payload, size_t *size);
    int payloadTaged(Stream * s, configType type, int tag, int device, int dir);
    int payloadSetVSID(Stream* s);
    int payloadSetChannelInfo(Stream * s, uint8_t **payload, size_t *size);
    int payloadSetTTYMode(uint8_t **payload, size_t *size, uint32_t mode);
    int setVoiceMixerParameter(Stream * s, struct mixer *mixer, void *payload,
                          int size, int dir);
    char* getMixerVoiceStream(Stream *s, int dir);
    int32_t getFrontEndId(uint32_t ldir) override;
    uint32_t getMIID(const char *backendName, uint32_t tagId, uint32_t *miid) override;
    struct mixer_ctl* getFEMixerCtl(const char *controlName, int *device, pal_stream_direction_t dir) override;
    int setSidetone(int deviceId, Stream * s, bool enable);
    int setHWSidetone(Stream * s, bool enable);
    int getTXDeviceId(Stream *s, int *id);
    int populate_rx_mfc_payload(Stream *s, uint32_t rx_mfc_tag);
    int populate_rx_mfc_coeff_payload(std::shared_ptr<Device> CrsDevice);
    int populate_vsid_payload(Stream *s);
    int populate_ch_info_payload(Stream *s);
    int populateVSIDLoopbackPayload(Stream* s);
    int getDeviceChannelInfo(Stream *s, uint16_t *channels);
    int build_rx_mfc_payload(Stream *s);
    int populateRatPayload(Stream *s);
    int setTaggedSlotMask(Stream * s);
    int setPopSuppressorMute(Stream *s);
    int setExtECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable);
    int getRXDevice(Stream *s, std::shared_ptr<Device> &rx_dev);
    int getDeviceData(Stream *s, struct sessionToPayloadParam *deviceData);
};

#endif //SESSION_ALSAVOICE_H
