/*
 * Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 */

#ifndef SESSION_GSL_H
#define SESSION_GSL_H

#define BUFF_FLAG_EOS 0x1

#include "gsl_intf.h"
#include "Session.h"
#include <dlfcn.h>
#include "apm_api.h"
#include "common_enc_dec_api.h"
#include "module_cmn_api.h"
#include "hw_intf_cmn_api.h"
#include "media_fmt_api.h"
#include "module_cmn_api.h"
#include "i2s_api.h"
#include "slimbus_api.h"
#include "rate_adapted_timer_api.h"
#include "cop_packetizer_cmn_api.h"
#include "cop_packetizer_v0_api.h"
#include "cop_v2_depacketizer_api.h"
#include "cop_v2_packetizer_api.h"
#include "ss_packetizer_cmn_api.h"
#include "lc3_encoder_api.h"
#include "pcm_tdm_api.h"
#include "audio_dam_buffer_api.h"
#include "codec_dma_api.h"
#include "detection_cmn_api.h"
#include "aptx_classic_encoder_api.h"
#include "aptx_adaptive_encoder_api.h"
#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include "Stream.h"
#include "PalCommon.h"

/* Param ID definitions */
#define PARAM_ID_MEDIA_FORMAT 0x0800100C
#define PARAM_ID_SOFT_PAUSE_START 0x0800102E
#define PARAM_ID_SOFT_PAUSE_RESUME 0x0800102F
#define PARAM_ID_VOL_CTRL_MULTICHANNEL_GAIN 0x08001038
#define PARAM_ID_VOL_CTRL_MASTER_GAIN 0x08001035
#define PLAYBACK_VOLUME_MASTER_GAIN_DEFAULT 0x2000
#define PARAM_ID_FFV_DOA_TRACKING_MONITOR 0x080010A4

#define WSA_CODEC_DMA_CORE  LPAIF_WSA
#define VA_CODEC_DMA_CORE   LPAIF_VA
#define RXTX_CODEC_DMA_CORE LPAIF_RXTX

#define CODEC_RX0 1
#define CODEC_TX0 1
#define CODEC_RX1 2
#define CODEC_TX1 2
#define CODEC_RX2 3
#define CODEC_TX2 3
#define CODEC_RX3 4
#define CODEC_TX3 4
#define CODEC_RX4 5
#define CODEC_TX4 5
#define CODEC_RX5 6
#define CODEC_TX5 6
#define CODEC_RX6 7
#define CODEC_RX7 8

#define SLIM_RX0 144
#define SLIM_RX1 145
#define SLIM_TX0 128
#define SLIM_TX1 129
#define SLIM_TX7 135

struct gslCmdGetReadWriteBufInfo {
    uint32_t buff_size;
    uint32_t num_buffs;
    uint32_t start_threshold;
    uint32_t stop_threshold;
    uint32_t attritubes;
	uint32_t tag;
};

struct __attribute__((__packed__)) volume_ctrl_channels_gain_config_t
{
    uint32_t channel_mask_lsb;
    uint32_t channel_mask_msb;
    uint32_t gain;
};

struct __attribute__((__packed__)) volume_ctrl_multichannel_gain_t
{
    uint32_t num_config;
    volume_ctrl_channels_gain_config_t gain_data[0];
};

class Stream;
class Session;

class SessionGsl : public Session
{
private:
    void * graphHandle;
    void * payload = NULL;
    std::shared_ptr<ResourceManager> rm;
    size_t size = 0;
    size_t gkvLen, ckvLen, tkvLen;
    struct gslCmdGetReadWriteBufInfo *infoBuffer;
    static int seek;
    static void* gslLibHandle;
    PayloadBuilder* builder;
      //TODO: move this to private
    struct gsl_key_vector *gkv;
    struct gsl_key_vector *ckv;
    struct gsl_key_vector *tkv;

    int fileWrite(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag);
    int fileRead(Stream *s, int tag, struct pal_buffer *buf, int * size);
    int setCalibration(Stream *s, int tag);
    int populateSVASoundModel(int tagId, void* graphHandle, struct pal_st_sound_model *pSoundModel);
    int populateSVAWakeUpConfig(int tagId, void* graphHandle, struct detection_engine_config_voice_wakeup *pWakeUpConfig);
    int populateSVAWakeUpBufferConfig(int tagId, void* graphHandle, struct detection_engine_voice_wakeup_buffer_config *pWakeUpBufConfig);
    int populateSVAStreamSetupDuration(int tagId, void* graphHandle, struct audio_dam_downstream_setup_duration *pSetupDuration);
    int populateSVAEventConfig(int tagId, void* graphHandle, struct detection_engine_generic_event_cfg *pEventConfig);
    int populateSVAEngineReset(int tagId, void* graphHandle, Stream *st);
    SessionGsl();
    
public:
    SessionGsl(std::shared_ptr<ResourceManager> Rm);
    virtual ~SessionGsl();
    static int init(std::string acdbFile);
    static void deinit();
    int open(Stream * s) override;
    int prepare(Stream * s) override;
    int setTKV(Stream * s, configType type, effect_pal_payload_t *payload) override;
    int setConfig(Stream * s, configType type, int tag = 0) override;
    int setConfig(Stream * s, configType type, uint32_t tag1,
            uint32_t tag2, uint32_t tag3) override;
    int setPayloadConfig(Stream *s);
    //int getConfig(Stream * s) override;
    int start(Stream * s) override;
    int stop(Stream * s) override;
    int close(Stream * s) override;
    int readBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int writeBufferInit(Stream *s, size_t noOfBuf, size_t bufSize, int flag) override;
    int read(Stream *s, int tag, struct pal_buffer *buf, int * size) override;
    int write(Stream *s, int tag, struct pal_buffer *buf, int * size, int flag) override;
    int getParameters(Stream *s, int tagId, uint32_t param_id, void **payload) override;
    int setParameters(Stream *s, int tagId, uint32_t param_id, void *payload) override;
    static void stCallBack(struct gsl_event_cb_params *event_params, void *client_data);
    void checkAndConfigConcurrency(Stream *s);
    int getTimestamp(struct pal_session_time *stime) override;
    int registerCallBack(session_callback cb, uint64_t cookie) override;
    int drain(pal_drain_type_t type) override;
    int setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int connectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect) override;
    int disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToDisconnect) override;
    uint32_t getMIID(const char *backendName __unused, uint32_t tagId __unused,  uint32_t *miid __unused) {return 0;};
    int setECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable) override;
};

#endif //SESSION_GSL_H
