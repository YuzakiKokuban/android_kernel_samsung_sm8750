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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef PAYLOAD_BUILDER_H_
#define PAYLOAD_BUILDER_H_

#include <vector>
#include <set>
#include <algorithm>
#include <expat.h>
#include <map>
#include <regex>
#include <sstream>
#include "PalDefs.h"
#include "gsl_intf.h"
#include "kvh2xml.h"
#include "PalCommon.h"
#include "Stream.h"
#include "Device.h"
#include "ResourceManager.h"
#include "AudioHapticsInterface.h"
#include "us_tone_renderer_api.h"

#ifdef ENABLE_TAS_SPK_PROT
#include "TIPayloadBuilder.h"
#endif

#define PAL_ALIGN_8BYTE(x) (((x) + 7) & (~7))
#define PAL_PADDING_8BYTE_ALIGN(x)  ((((x) + 7) & 7) ^ 7)

#define MSM_MI2S_SD0 (1 << 0)
#define MSM_MI2S_SD1 (1 << 1)
#define MSM_MI2S_SD2 (1 << 2)
#define MSM_MI2S_SD3 (1 << 3)
#define MSM_MI2S_SD4 (1 << 4)
#define MSM_MI2S_SD5 (1 << 5)
#define MSM_MI2S_SD6 (1 << 6)
#define MSM_MI2S_SD7 (1 << 7)

#define PCM_ENCODER STREAM_PCM_ENCODER
#define PCM_DECODER STREAM_PCM_DECODER
#define PCM_CONVERTOR STREAM_PCM_CONVERTER
#define IN_MEDIA STREAM_INPUT_MEDIA_FORMAT
#define CODEC_DMA 0
#define I2S 1
#define HW_EP_TX DEVICE_HW_ENDPOINT_TX
#define HW_EP_RX DEVICE_HW_ENDPOINT_RX
#define TAG_STREAM_MFC_SR TAG_STREAM_MFC
#define TAG_DEVICE_MFC_SR PER_STREAM_PER_DEVICE_MFC

struct sessionToPayloadParam {
    uint32_t sampleRate;                /**< sample rate */
    uint32_t bitWidth;                  /**< bit width */
    uint32_t numChannel;
    struct pal_channel_info *ch_info;    /**< channel info */
    int direction;
    int native;
    void *metadata;
    sessionToPayloadParam():sampleRate(48000),bitWidth(16),
    numChannel(2),ch_info(nullptr), direction(0),
    native(0),metadata(nullptr) {}
};

struct usbAudioConfig {
    uint32_t usb_token;
    uint32_t svc_interval;
};

struct dpAudioConfig{
  uint32_t channel_allocation;
  uint32_t mst_idx;
  uint32_t dptx_idx;
};

typedef enum {
    DIRECTION_SEL = 1,
    BITWIDTH_SEL,
    INSTANCE_SEL,
    SUB_TYPE_SEL,
    VSID_SEL,
    VUI_MODULE_TYPE_SEL,
    ACD_MODULE_TYPE_SEL,
    ASR_MODULE_TYPE_SEL,
    STREAM_TYPE_SEL,
    CODECFORMAT_SEL,
    ABR_ENABLED_SEL,
    AUD_FMT_SEL,
    DEVICEPP_TYPE_SEL,
    CUSTOM_CONFIG_SEL,
    HOSTLESS_SEL,
    SIDETONE_MODE_SEL,
} selector_type_t;

const std::map<std::string, selector_type_t> selectorstypeLUT {
    {std::string{ "Direction" },             DIRECTION_SEL},
    {std::string{ "BitWidth" },              BITWIDTH_SEL},
    {std::string{ "Instance" },              INSTANCE_SEL},
    {std::string{ "SubType" },               SUB_TYPE_SEL},
    {std::string{ "VSID" },                  VSID_SEL},
    {std::string{ "VUIModuleType" },         VUI_MODULE_TYPE_SEL},
    {std::string{ "ACDModuleType" },         ACD_MODULE_TYPE_SEL},
    {std::string{ "ASRModuleType" },         ASR_MODULE_TYPE_SEL},
    {std::string{ "StreamType" },            STREAM_TYPE_SEL},
    {std::string{ "DevicePPType" },          DEVICEPP_TYPE_SEL},
    {std::string{ "CodecFormat" },           CODECFORMAT_SEL},
    {std::string{ "AbrEnabled" },            ABR_ENABLED_SEL},
    {std::string{ "AudioFormat" },           AUD_FMT_SEL},
    {std::string{ "CustomConfig" },          CUSTOM_CONFIG_SEL},
    {std::string{ "Hostless" },              HOSTLESS_SEL},
    {std::string{ "SidetoneMode" },          SIDETONE_MODE_SEL},
};

#ifdef SEC_AUDIO_CALL_VOIP
static const std::map<uint32_t, std::string> micModeLUT {
    {STANDARD_MODE,           std::string{"STANDARD_MODE"}},
    {VOICE_FOCUS_MODE,        std::string{"VOICE_FOCUS_MODE"}},
    {ALL_SOUND_MODE,          std::string{"ALL_SOUND_MODE"}},
    {TRANSLATION,             std::string{"TRANSLATION"}},
};
#endif

struct kvPairs {
    unsigned int key;
    unsigned int value;
};

struct kvInfo {
    std::vector<std::string> selector_names;
    std::vector<std::pair<selector_type_t, std::string>> selector_pairs;
    std::vector<kvPairs> kv_pairs;
};

struct allKVs {
    std::vector<int> id_type;
    std::vector<kvInfo> keys_values;
};

typedef enum {
    TAG_USECASEXML_ROOT,
    TAG_STREAM_SEL,
    TAG_STREAMPP_SEL,
    TAG_DEVICE_SEL,
    TAG_DEVICEPP_SEL,
} usecase_xml_tag;

typedef struct {
    uint32_t moduleId;
    uint32_t paramId;
    uint16_t length;
    uint16_t reserved;
} __attribute__((packed)) legacyGefParamHeader;

struct user_xml_data{
    char data_buf[1024];
    size_t offs;
    usecase_xml_tag tag;
    bool is_parsing_streams;
    bool is_parsing_streampps;
    bool is_parsing_devices;
    bool is_parsing_devicepps;
};
class SessionGsl;

class PayloadBuilder
{
protected:
   static std::vector<allKVs> all_streams;
   static std::vector<allKVs> all_streampps;
   static std::vector<allKVs> all_devices;
   static std::vector<allKVs> all_devicepps;

public:
    void payloadUsbAudioConfig(uint8_t** payload, size_t* size,
                           uint32_t miid,
                           struct usbAudioConfig *data);
    void payloadDpAudioConfig(uint8_t** payload, size_t* size,
                           uint32_t miid,
                           struct dpAudioConfig *data);
    void payloadVolumeCtrlRamp(uint8_t** payload, size_t* size,
         uint32_t miid, uint32_t ramp_period_ms);
    void payloadMFCConfig(uint8_t** payload, size_t* size,
                           uint32_t miid,
                           struct sessionToPayloadParam* data);
    void payloadMFCMixerCoeff(uint8_t** payload, size_t* size,
                           uint32_t miid, int numCh, int rotationType);
    void payloadCRSMFCMixerCoeff(uint8_t** payload, size_t* size,
                           uint32_t miid);
    void payloadVolumeConfig(uint8_t** payload, size_t* size,
                           uint32_t miid,
                           struct pal_volume_data * data);
    void payloadMultichVolumemConfig(uint8_t** payload, size_t* size,
                           uint32_t miid,
                           struct pal_volume_data * data);
    void payloadGainConfig(uint8_t** payload, size_t* size,
                           uint32_t miid,
                           struct pal_gain_data * data);
    int payloadCustomParam(uint8_t **alsaPayload, size_t *size,
                            uint32_t *customayload, uint32_t customPayloadSize,
                            uint32_t moduleInstanceId, uint32_t dspParamId);
    int payloadACDBParam(uint8_t **alsaPayload, size_t *size,
                            uint8_t *acdbParam,
                            uint32_t moduleInstanceId,
                            uint32_t sampleRate);
    int payloadACDBTunnelParam(uint8_t **alsaPayload, size_t *size,
                            uint8_t *acdbParam,
                            const std::set <std::pair<int, int>> &acdbGKVSet,
                            uint32_t moduleInstanceId,
                            uint32_t sampleRate);
    int payloadConfig(uint8_t **payload, size_t *size, uint8_t *config,
                      size_t config_size, uint32_t miid, uint32_t param_id);
    void payloadGetParam(Stream* s, uint8_t **payload, size_t *size, uint32_t moduleId,
                         uint32_t param_id, size_t config_size);
    void payloadQuery(uint8_t **payload, size_t *size, uint32_t moduleId,
                            uint32_t paramId, uint32_t querySize);
    template <typename T>
    void populateChannelMap(T pcmChannel, uint8_t numChannel);
    template <typename T>
    void populateChannelMixerCoeff(T pcmChannel, uint8_t numChannel,
                                 int rotationType);
    template <typename T>
    void populateCRSChannelMixerCoeff(T pcmChannel, uint8_t OutnumChannel,
                                 uint8_t InnumChannel);
    void payloadLC3Config(uint8_t** payload, size_t* size,
                          uint32_t miid, bool isLC3MonoModeOn);
    void payloadRATConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          struct pal_media_config *data);
    void payloadPcmCnvConfig(uint8_t** payload, size_t* size, uint32_t miid,
                             struct pal_media_config *data, bool isRx);
    void payloadCopPackConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          struct pal_media_config *data);
    void payloadNRECConfig(uint8_t** payload, size_t* size,
        uint32_t miid, bool isNrecEnabled);
    void payloadCopV2StreamInfo(uint8_t** payload, size_t* size, uint32_t miid, void *data,
                          bool isStreamMapDirIn);
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
    void payloadSsPackConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          struct pal_media_config *data);
    void payloadSsPackEncConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          uint16_t enc_format);
    void payloadSsPackSuspendConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          uint16_t a2dp_suspend);
    void payloadSsPackMtuConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          uint16_t mtu);
    void payloadSsPackBitrate(uint8_t** payload, size_t* size, uint32_t miid,
                          int32_t bitrate);
    void payloadSsPackSbmConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          pal_param_bta2dp_sbm_t *param_bt_a2dp_sbm);
#endif
    void payloadTWSConfig(uint8_t** payload, size_t* size, uint32_t miid,
                          bool isTwsMonoModeOn, uint32_t codecFormat);
    void payloadSPConfig(uint8_t** payload, size_t* size, uint32_t miid,
                         int paramId, void *data);
#ifdef ENABLE_TAS_SPK_PROT
    void payloadTISPConfig(uint8_t** payload, size_t* size, uint32_t miid,
                         int paramId, void *data);
    /* add a member function */
    void payloadTISPCommConfig(uint8_t** payload, size_t* size, uint32_t miid,
                    int param_id, void *param, int is_get);
#endif
    void payloadHapticsDevPConfig(uint8_t** payload, size_t* size, uint32_t miid,
                         int param_id, void *param);
    void payloadScramblingConfig(uint8_t** payload, size_t* size,
            uint32_t miid, uint32_t enable);
    int payloadPopSuppressorConfig(uint8_t** payload, size_t* size,
                                   uint32_t miid, bool enable);
    void payloadMSPPConfig(uint8_t** payload, size_t* size,
                          uint32_t miid, uint32_t gain);
    void payloadSoftPauseConfig(uint8_t** payload, size_t* size,
                          uint32_t miid, uint32_t delayMs);
    std::unique_ptr<uint8_t[]> getPayloadEncoderBitrate(
        uint32_t encoderMIID, uint32_t newBitrate, size_t &outputPayloadSize);
    void payloadPlaybackRateParametersConfig(uint8_t** payload, size_t* size,
                          uint32_t miid, pal_param_playback_rate *playbackRate);
    void payloadDAMPortConfig(uint8_t** payload, size_t* size,
                          uint32_t miid, uint8_t numChannel);

    int populateStreamKV(Stream* s, std::vector <std::pair<int,int>> &keyVector);
    int populateStreamKV(Stream* s, std::vector <std::pair<int,int>> &keyVectorRx,
        std::vector <std::pair<int,int>> &keyVectorTx ,struct vsid_info vsidinfo);
    int populateStreamKVTunnel(Stream* s,
        std::vector <std::pair<int,int>> &keyVector, uint32_t instanceId);
    int populateStreamPPKV(Stream* s, std::vector <std::pair<int,int>> &keyVectorRx,
        std::vector <std::pair<int,int>> &keyVectorTx);
    int populateStreamDeviceKV(Stream* s, int32_t beDevId, std::vector <std::pair<int,int>> &keyVector);
    int populateStreamDeviceKV(Stream* s, int32_t rxBeDevId, std::vector <std::pair<int,int>> &keyVectorRx,
        int32_t txBeDevId, std::vector <std::pair<int,int>> &keyVectorTx, struct vsid_info vsidinfo, sidetone_mode_t sidetoneMode);
    int populateDeviceKV(Stream* s, int32_t beDevId, std::vector <std::pair<int,int>> &keyVector);
    int populateDeviceKV(Stream* s, int32_t rxBeDevId, std::vector <std::pair<int,int>> &keyVectorRx,
        int32_t txBeDevId, std::vector <std::pair<int,int>> &keyVectorTx, sidetone_mode_t sidetoneMode);
    int populateDeviceKVTunnel(Stream* s, int32_t beDevId,
        std::vector <std::pair<int,int>> &keyVector);
    int populateDevicePPKV(Stream* s, int32_t rxBeDevId, std::vector <std::pair<int,int>> &keyVectorRx,
        int32_t txBeDevId, std::vector <std::pair<int,int>> &keyVectorTx);
    int populateDevicePPKVTunnel(Stream* s, int32_t rxBeDevId,
        std::vector <std::pair<int,int>> &keyVectorRx);
    int populateDevicePPCkv(Stream *s, std::vector <std::pair<int,int>> &keyVector);
    int populateStreamCkv(Stream *s, std::vector <std::pair<int,int>> &keyVector, int tag, struct pal_volume_data **);
    int populateCalKeyVector(Stream *s, std::vector <std::pair<int,int>> &ckv, int tag);
    int populateTagKeyVector(Stream *s, std::vector <std::pair<int,int>> &tkv, int tag, uint32_t* gsltag);
    void payloadTimestamp(std::shared_ptr<std::vector<uint8_t>>& module_payload, size_t *size, uint32_t moduleId);
    static int init();
    static void endTag(void *userdata, const XML_Char *tag_name);
    static void startTag(void *userdata, const XML_Char *tag_name, const XML_Char **attr);
    static void handleData(void *userdata, const char *s, int len);
    static void resetDataBuf(struct user_xml_data *data);
    static void processKVTypeData(struct user_xml_data *data, const XML_Char **attr);
    static void processKVSelectorData(struct user_xml_data *data, const XML_Char **attr);
    static void processGraphKVData(struct user_xml_data *data, const XML_Char **attr);
    static bool isIdTypeAvailable(int32_t type, std::vector<int32_t>& id_type);
    static void removeDuplicateSelectors(std::vector<std::string> &gkv_selectors);
    static std::vector <std::string> retrieveSelectors(int32_t type,
        std::vector<allKVs> &any_type);
    static std::vector <std::pair<selector_type_t, std::string>> getSelectorValues(
        std::vector<std::string> &selectors, Stream* s, struct pal_device* dAttr);
    static bool compareSelectorPairs(std::vector <std::pair<selector_type_t, std::string>>
        &selector_val, std::vector<std::pair<selector_type_t, std::string>> &filled_selector_pairs);
    static int retrieveKVs(std::vector<std::pair<selector_type_t, std::string>>
        &filled_selector_pairs, uint32_t type, std::vector<allKVs> &any_type,
        std::vector<std::pair<int32_t, int32_t>> &keyVector);
    static bool findKVs(std::vector<std::pair<selector_type_t, std::string>>
        &filled_selector_pairs, uint32_t type, std::vector<allKVs> &any_type,
        std::vector<std::pair<int32_t, int32_t>> &keyVector);
    static std::string removeSpaces(const std::string& str);
    static std::vector<std::string> splitStrings(const std::string& str);
    static int getBtDeviceKV(int dev_id, std::vector<std::pair<int, int>> &deviceKV,
        uint32_t codecFormat, bool isAbrEnabled, bool isHostless);
    static int getDeviceKV(int dev_id, std::vector<std::pair<int, int>> &deviceKV);
    static bool compareNumSelectors(struct kvInfo info_1, struct kvInfo info_2);
    static int payloadDualMono(uint8_t **payloadInfo);
    void payloadAFSInfo(uint8_t **payload, size_t *size, uint32_t moduleId);
    void USToneRendererNotifyPayload(uint8_t **payload, size_t *size,
            struct pal_device *dAttr, uint32_t moduleId,
            us_tone_renderer_ep_media_format_status_t event);
    PayloadBuilder();
    ~PayloadBuilder();
};
#endif //SESSION_H
