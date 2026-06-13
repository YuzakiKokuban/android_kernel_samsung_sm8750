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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: PayloadBuilder"
#include <agm/agm_api.h>
#include <bt_intf.h>
#include <bt_ble.h>
#include <amdb_api.h>
#include "ResourceManager.h"
#include "PayloadBuilder.h"
#include "SessionGsl.h"
#include "StreamSoundTrigger.h"
#include "spr_api.h"
#include "pop_suppressor_api.h"
#include "sp_vi.h"
#include "sp_rx.h"
#include "cps_data_router.h"
#include "rx_haptics_api.h"
#include "wsa_haptics_vi_api.h"
#include "fluence_ffv_common_calibration.h"
#include "mspp_module_calibration_api.h"
#include "tsm_module_api.h"
#include "USBAudio.h"
#include "asr_module_calibration_api.h"

#ifdef ENABLE_TAS_SPK_PROT
#include "TISpeakerProtDefs.h"
#endif

#if defined(FEATURE_IPQ_OPENWRT) || defined(LINUX_ENABLED)
#define USECASE_XML_FILE "/etc/usecaseKvManager.xml"
#else
#define USECASE_XML_FILE "/vendor/etc/usecaseKvManager.xml"
#endif

#define PARAM_ID_CHMIXER_COEFF 0x0800101F
#define CUSTOM_STEREO_NUM_OUT_CH 0x0002
#define CUSTOM_STEREO_NUM_IN_CH 0x0002
#define Q14_GAIN_ZERO_POINT_FIVE 0x2000
#define CUSTOM_STEREO_CMD_PARAM_SIZE 24

#define PARAM_ID_CONN_PROXY_STREAM_INFO   0x080013C3
#define PARAM_ID_DISPLAY_PORT_INTF_CFG   0x8001154

#define PARAM_ID_USB_AUDIO_INTF_CFG                               0x080010D6

/* ID of the Master Gain parameter used by MODULE_ID_VOL_CTRL. */
#define PARAM_ID_VOL_CTRL_MASTER_GAIN 0x08001035
/* ID of the channel mixer coeff for MODULE_ID_MFC */
#define PARAM_ID_CHMIXER_COEFF 0x0800101F

#define Q24_MULTIPLIER 0x1000000

#define PARAM_ID_VOL_CTRL_MASTER_MUTE 0x08001036

#define MAX_CRS_VOL_INDEX 7

struct volume_ctrl_master_gain_t
{
    uint16_t master_gain;
    /**< @h2xmle_description  {Specifies linear master gain in Q13 format\n}
     *   @h2xmle_dataFormat   {Q13}
     *   @h2xmle_default      {0x2000} */

    uint16_t reserved;
    /**< @h2xmle_description  {Clients must set this field to 0.\n}
     *   @h2xmle_rangeList    {"0" = 0}
     *   @h2xmle_default      {0}     */
};
/* Structure type def for above payload. */
typedef struct volume_ctrl_master_gain_t volume_ctrl_master_gain_t;

#define PARAM_ID_VOL_CTRL_GAIN_RAMP_PARAMETERS 0x08001037
#define PARAM_VOL_CTRL_RAMPINGCURVE_LINEAR 0

struct param_id_module_gain_cfg_t
{
    uint16_t gain;
    /**< @h2xmle_description {Linear gain (in Q13 format)}
     *   @h2xmle_dataFormat   {Q13}
     *   @h2xmle_default      {0x2000} */

    uint16_t reserved;
    /**< @h2xmle_description  {Clients must set this field to 0.\n}
     *   @h2xmle_rangeList    {"0" = 0} */
};
/* Structure type def for above payload. */
typedef struct param_id_module_gain_cfg_t param_id_module_gain_cfg_t;
/* ID of the parameter used to set the gain */
#define PARAM_ID_GAIN_MODULE_GAIN 0x08001006

/* Structure for holding soft stepping volume parameters. */
struct volume_ctrl_gain_ramp_params_t
{
   uint32_t period_ms;
   uint32_t step_us;
   uint32_t ramping_curve;
};

/* ID to configure downstream delay */
#define PARAM_ID_SOFT_PAUSE_DOWNSTREAM_DELAY 0x0800103E

struct pause_downstream_delay_t
{
     uint32_t delay_ms;
     /**< Specifies the downstream delay from the stream to the device leg.

          @values 0 through 65535 milliseconds (Default = 25) */

     /*#< @h2xmle_description {Specifies the downstream delay from the stream to
                               the device leg (in milliseconds).}
          @h2xmle_range       {0..65535}
          @h2xmle_default     {25} */
};
typedef struct pause_downstream_delay_t pause_downstream_delay_t;


/* ID of the Output Media Format parameters used by MODULE_ID_MFC */
#define PARAM_ID_MFC_OUTPUT_MEDIA_FORMAT            0x08001024
#include "spf_begin_pack.h"
#include "spf_begin_pragma.h"
/* Payload of the PARAM_ID_MFC_OUTPUT_MEDIA_FORMAT parameter in the
 Media Format Converter Module. Following this will be the variable payload for channel_map. */

struct param_id_mfc_output_media_fmt_t
{
   int32_t sampling_rate;
   /**< @h2xmle_description  {Sampling rate in samples per second\n
                              ->If the resampler type in the MFC is chosen to be IIR,
                              ONLY the following sample rates are ALLOWED:
                              PARAM_VAL_NATIVE =-1;\n
                              PARAM_VAL_UNSET = -2;\n
                              8 kHz = 8000;\n
                              16kHz = 16000;\n
                              24kHz = 24000;\n
                              32kHz = 32000;\n
                              48kHz = 48000 \n
                              -> For resampler type FIR, all values in the range
                              below are allowed}
        @h2xmle_rangeList   {"PARAM_VAL_UNSET" = -2;
                             "PARAM_VAL_NATIVE" =-1;
                             "8 kHz"=8000;
                             "11.025 kHz"=11025;
                             "12 kHz"=12000;
                             "16 kHz"=16000;
                             "22.05 kHz"=22050;
                             "24 kHz"=24000;
                             "32 kHz"=32000;
                             "44.1 kHz"=44100;
                             "48 kHz"=48000;
                             "88.2 kHz"=88200;
                             "96 kHz"=96000;
                             "176.4 kHz"=176400;
                             "192 kHz"=192000;
                             "352.8 kHz"=352800;
                             "384 kHz"=384000}
        @h2xmle_default      {-1} */

   int16_t bit_width;
   /**< @h2xmle_description  {Bit width of audio samples \n
                              ->Samples with bit width of 16 (Q15 format) are stored in 16 bit words \n
                              ->Samples with bit width 24 bits (Q27 format) or 32 bits (Q31 format) are stored in 32 bit words}
        @h2xmle_rangeList    {"PARAM_VAL_NATIVE"=-1;
                              "PARAM_VAL_UNSET"=-2;
                              "16-bit"= 16;
                              "24-bit"= 24;
                              "32-bit"=32}
        @h2xmle_default      {-1}
   */

   int16_t num_channels;
   /**< @h2xmle_description  {Number of channels. \n
                              ->Ranges from -2 to 32 channels where \n
                              -2 is PARAM_VAL_UNSET and -1 is PARAM_VAL_NATIVE}
        @h2xmle_range        {-2..32}
        @h2xmle_default      {-1}
   */

   uint16_t channel_type[0];
   /**< @h2xmle_description  {Channel mapping array. \n
                              ->Specify a channel mapping for each output channel \n
                              ->If the number of channels is not a multiple of four, zero padding must be added
                              to the channel type array to align the packet to a multiple of 32 bits. \n
                              -> If num_channels field is set to PARAM_VAL_NATIVE (-1) or PARAM_VAL_UNSET(-2)
                              this field will be ignored}
        @h2xmle_variableArraySize {num_channels}
        @h2xmle_range        {1..63}
        @h2xmle_default      {1}    */
}
#include "spf_end_pragma.h"
#include "spf_end_pack.h"
;

struct param_id_usb_audio_intf_cfg_t
{
   uint32_t usb_token;
   uint32_t svc_interval;
};
/* Structure type def for above payload. */
typedef struct mspp_volume_ctrl_gain_t mspp_volume_ctrl_gain_t;

#include "spf_begin_pack.h"
struct chmixer_coeff_t
{
    uint16_t num_output_channels;
    uint16_t num_input_channels;
    uint16_t out_chmap[0];
    uint16_t in_chmap[0];
    uint16_t coeff[0];
}
#include "spf_end_pack.h"
;
typedef struct chmixer_coeff_t chmixer_coeff_t;

#include "spf_begin_pack.h"
#include "spf_begin_pragma.h"
struct param_id_chmixer_coeff_t
{
    uint32_t num_coeff_tbls;
    chmixer_coeff_t chmixer_coeff_tbl[0];
}
#include "spf_end_pragma.h"
#include "spf_end_pack.h"
;
typedef struct param_id_chmixer_coeff_t param_id_chmixer_coeff_t;

struct volume_ctrl_master_mute_t
{
   uint32_t mute_flag;
/**< @h2xmle_description {Specifies whether mute is enabled}
     @h2xmle_rangeList   {"Disable"= 0;
                          "Enable"=1}
     @h2xmle_default     {0}  */
};
/* Structure type def for above payload. */
typedef struct volume_ctrl_master_mute_t volume_ctrl_master_mute_t;

std::vector<allKVs> PayloadBuilder::all_streams;
std::vector<allKVs> PayloadBuilder::all_streampps;
std::vector<allKVs> PayloadBuilder::all_devices;
std::vector<allKVs> PayloadBuilder::all_devicepps;

template <typename T>
void PayloadBuilder::populateChannelMixerCoeff(T pcmChannel, uint8_t numChannel,
                int rotationType)
{
    /* Channel Coefficient table decides the output channel data for an input
     * channel data. There is a range from 0 to 0x4000 where 0 means output
     * will be mute and 0x4000 means unity. For a playback of 2 channels without
     * swap, the channel coefficients should look like below:
     *
     *                  Left(Output)| Right(Output)
     * Left (Input)     0x4000      | 0x0000
     * Right(Input)     0x0000      | 0x4000
     *
     * We need to send this table as {0x4000,0x0000,0x0000,0x4000} to SPF
     * Index w.r.t above Matrix      {{0,0} ,{0,1}, {1,0}, {1,1}}
     * Index expected by SPF         {0, 1, 2, 3}
     * For swap, we will change the coefficient so that output will be swapped
     *
     *                  Left(Output)| Right(Output)
     * Left (Input)     0x0000      | 0x4000
     * Right(Input)     0x4000      | 0x0000
     *
     */

    int numCoeff = numChannel * numChannel;
    for (int i = 0; i < numCoeff; i++)
        pcmChannel[i] = 0x0000;

    if (rotationType == PAL_SPEAKER_ROTATION_RL) {
        // Swap the channel data
        pcmChannel[1] = 0x4000;
        pcmChannel[2] = 0x4000;

    } else {
        pcmChannel[0] = 0X4000;
        pcmChannel[3] = 0X4000;
    }
}

template <typename T>
void PayloadBuilder::populateCRSChannelMixerCoeff(T pcmChannel, uint8_t OutnumChannel,
                                                    uint8_t InnumChannel)
{
    int numCoeff = OutnumChannel * InnumChannel;
    for (int i = 0; i < numCoeff; i++) {
         pcmChannel[i] = 0x2000;
    }
}

template <typename T>
void PayloadBuilder::populateChannelMap(T pcmChannel, uint8_t numChannel)
{
    if (numChannel == 1) {
        pcmChannel[0] = PCM_CHANNEL_C;
    } else if (numChannel == 2) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
    } else if (numChannel == 3) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
    } else if (numChannel == 4) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_LB;
        pcmChannel[3] = PCM_CHANNEL_RB;
    } else if (numChannel == 5) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LB;
        pcmChannel[4] = PCM_CHANNEL_RB;
    } else if (numChannel == 6) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LFE;
        pcmChannel[4] = PCM_CHANNEL_LB;
        pcmChannel[5] = PCM_CHANNEL_RB;
    } else if (numChannel == 7) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LS;
        pcmChannel[4] = PCM_CHANNEL_RS;
        pcmChannel[5] = PCM_CHANNEL_LB;
        pcmChannel[6] = PCM_CHANNEL_RB;
    } else if (numChannel == 8) {
        pcmChannel[0] = PCM_CHANNEL_L;
        pcmChannel[1] = PCM_CHANNEL_R;
        pcmChannel[2] = PCM_CHANNEL_C;
        pcmChannel[3] = PCM_CHANNEL_LS;
        pcmChannel[4] = PCM_CHANNEL_RS;
        pcmChannel[5] = PCM_CHANNEL_CS;
        pcmChannel[6] = PCM_CHANNEL_LB;
        pcmChannel[7] = PCM_CHANNEL_RB;
    }
}

void PayloadBuilder::payloadUsbAudioConfig(uint8_t** payload, size_t* size,
    uint32_t miid, struct usbAudioConfig *data)
{
    struct apm_module_param_data_t* header;
    struct param_id_usb_audio_intf_cfg_t *usbConfig;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
       sizeof(struct param_id_usb_audio_intf_cfg_t);


    if (payloadSize % 8 != 0)
        payloadSize = payloadSize + (8 - payloadSize % 8);

    payloadInfo = (uint8_t*) calloc(1, payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo new failed %s", strerror(errno));
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    usbConfig = (struct param_id_usb_audio_intf_cfg_t*)(payloadInfo + sizeof(struct apm_module_param_data_t));
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_USB_AUDIO_INTF_CFG;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_ERR(LOG_TAG,"header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                     header->module_instance_id, header->param_id,
                     header->error_code, header->param_size);

    usbConfig->usb_token = data->usb_token;
    usbConfig->svc_interval = data->svc_interval;
    PAL_VERBOSE(LOG_TAG,"customPayload address %pK and size %zu", payloadInfo, payloadSize);

    *size = payloadSize;
    *payload = payloadInfo;

}

void PayloadBuilder::payloadDpAudioConfig(uint8_t** payload, size_t* size,
    uint32_t miid, struct dpAudioConfig *data)
{
    PAL_DBG(LOG_TAG, "Enter:");
    struct apm_module_param_data_t* header;
    struct dpAudioConfig *dpConfig;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
        sizeof(struct dpAudioConfig);

    if (payloadSize % 8 != 0)
        payloadSize = payloadSize + (8 - payloadSize % 8);

    payloadInfo = (uint8_t*) calloc(1, payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    dpConfig = (struct dpAudioConfig*)(payloadInfo + sizeof(struct apm_module_param_data_t));
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_DISPLAY_PORT_INTF_CFG;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_ERR(LOG_TAG,"header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    dpConfig->channel_allocation = data->channel_allocation;
    dpConfig->mst_idx = data->mst_idx;
    dpConfig->dptx_idx = data->dptx_idx;
    PAL_ERR(LOG_TAG,"customPayload address %pK and size %zu", payloadInfo, payloadSize);

    *size = payloadSize;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "Exit:");
}

#define PLAYBACK_VOLUME_MAX 0x2000
void PayloadBuilder::payloadVolumeConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_volume_data* voldata)
{
    struct apm_module_param_data_t* header = nullptr;
    volume_ctrl_master_gain_t *volConf = nullptr;
    float voldB = 0.0f;
    long vol = 0;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (voldata->no_of_volpair == 1) {
        voldB = (voldata->volume_pair[0].vol);
    } else {
        voldB = (voldata->volume_pair[0].vol + voldata->volume_pair[1].vol)/2;
        PAL_DBG(LOG_TAG,"volume sent left:%f , right: %f \n",(voldata->volume_pair[0].vol),
                  (voldata->volume_pair[1].vol));
    }
    PAL_VERBOSE(LOG_TAG,"volume sent:%f \n",voldB);
    vol = (long)(voldB * (PLAYBACK_VOLUME_MAX*1.0));
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct volume_ctrl_master_gain_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_VOL_CTRL_MASTER_GAIN;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);
    volConf = (volume_ctrl_master_gain_t *) (payloadInfo + sizeof(struct apm_module_param_data_t));
    volConf->master_gain = vol;
    PAL_VERBOSE(LOG_TAG, "header params IID:%x param_id:%x error_code:%d param_size:%d",
                  header->module_instance_id, header->param_id,
                  header->error_code, header->param_size);
    *size = payloadSize + padBytes;;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "payload %pK size %zu", *payload, *size);
}

void PayloadBuilder::payloadMultichVolumemConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_volume_data* voldata)
{
    const uint32_t PLAYBACK_MULTI_VOLUME_GAIN = 1 << 28;
    struct apm_module_param_data_t* header = nullptr;
    volume_ctrl_multichannel_gain_t *volConf = nullptr;
    struct apm_module_param_data_t* muteheader = nullptr;
    volume_ctrl_master_mute_t *muteConf = nullptr;
    int numChannels;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0, mutePayloadSize = 0, mutePadBytes = 0;
    numChannels = voldata->no_of_volpair;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct volume_ctrl_multichannel_gain_t) +
                  numChannels * sizeof(volume_ctrl_channels_gain_config_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_VOL_CTRL_MULTICHANNEL_GAIN;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);
    volConf = (volume_ctrl_multichannel_gain_t *) (payloadInfo + sizeof(struct apm_module_param_data_t));
    volConf->num_config = numChannels;
    PAL_DBG(LOG_TAG, "num_config %d", numChannels);
    /*
     * Only L/R channel setting is supported. No need to convert channel_mask to channel_map.
     * If other channel types support, the conversion is needed.
     */
    for (uint32_t i = 0; i < numChannels; i++) {
         volConf->gain_data[i].channel_mask_lsb = (1 << voldata->volume_pair[i].channel_mask);
         volConf->gain_data[i].channel_mask_msb = 0;
         volConf->gain_data[i].gain = (uint32_t)((voldata->volume_pair[i].vol) *
                                        (PLAYBACK_MULTI_VOLUME_GAIN * 1.0));
    }
    PAL_DBG(LOG_TAG, "header params IID:%x param_id:%x error_code:%d param_size:%d",
                  header->module_instance_id, header->param_id,
                  header->error_code, header->param_size);

    //always unmute when set multi channel gain
    mutePayloadSize = sizeof(struct apm_module_param_data_t) +
                      sizeof(struct volume_ctrl_master_mute_t);
    mutePadBytes = PAL_PADDING_8BYTE_ALIGN(mutePayloadSize);
    payloadInfo = (uint8_t*) realloc(payloadInfo, payloadSize + padBytes +
                                        mutePayloadSize + mutePadBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo realloc failed %s", strerror(errno));
        return;
    }
    muteheader = (struct apm_module_param_data_t*) (payloadInfo + payloadSize + padBytes);
    muteheader->module_instance_id = miid;
    muteheader->param_id = PARAM_ID_VOL_CTRL_MASTER_MUTE;
    muteheader->error_code = 0x0;
    muteheader->param_size = mutePayloadSize -  sizeof(struct apm_module_param_data_t);
    muteConf = (volume_ctrl_master_mute_t *) (payloadInfo + payloadSize + padBytes +
                                                sizeof(struct apm_module_param_data_t));
    muteConf->mute_flag = 0;
    PAL_DBG(LOG_TAG, "header params IID:%x param_id:%x error_code:%d param_size:%d",
                  muteheader->module_instance_id, muteheader->param_id,
                  muteheader->error_code, muteheader->param_size);
    *size = payloadSize + padBytes + mutePayloadSize + mutePadBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "payload %pK size %zu", *payload, *size);
}

void PayloadBuilder::payloadGainConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_gain_data* gaindata)

{
    struct apm_module_param_data_t* header = nullptr;
    struct param_id_module_gain_cfg_t *gainConf = nullptr;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    PAL_VERBOSE(LOG_TAG,"Gain set:%f \n",gaindata->gain);
    uint16_t gainQ13 = gaindata->gain;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_module_gain_cfg_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_GAIN_MODULE_GAIN;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);
    gainConf = (param_id_module_gain_cfg_t *) (payloadInfo + sizeof(struct apm_module_param_data_t));
    gainConf->gain = gainQ13;
    PAL_DBG(LOG_TAG, "header params IID:%x param_id:%x error_code:%d param_size:%d",
                  header->module_instance_id, header->param_id,
                  header->error_code, header->param_size);
    *size = payloadSize + padBytes;;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "payload %pK size %zu", *payload, *size);
}

void PayloadBuilder::payloadVolumeCtrlRamp(uint8_t** payload, size_t* size,
        uint32_t miid, uint32_t ramp_period_ms)
{
    struct apm_module_param_data_t* header = NULL;
    struct volume_ctrl_gain_ramp_params_t *rampParams;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct volume_ctrl_gain_ramp_params_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = PARAM_ID_VOL_CTRL_GAIN_RAMP_PARAMETERS;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);
    rampParams = (struct volume_ctrl_gain_ramp_params_t*) (payloadInfo + sizeof(struct apm_module_param_data_t));
    rampParams->period_ms = ramp_period_ms;
    rampParams->step_us = 0;
    rampParams->ramping_curve = PARAM_VOL_CTRL_RAMPINGCURVE_LINEAR;
    PAL_VERBOSE(LOG_TAG, "header params IID:%x param_id:%x error_code:%d param_size:%d",
                  header->module_instance_id, header->param_id,
                  header->error_code, header->param_size);
    *size = payloadSize + padBytes;;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "payload %pK size %zu", *payload, *size);
}

void PayloadBuilder::payloadMFCMixerCoeff(uint8_t** payload, size_t* size,
        uint32_t miid, int numCh, int rotationType)
{
    struct apm_module_param_data_t* header = NULL;
    param_id_chmixer_coeff_t *mfcMixerCoeff = NULL;
    chmixer_coeff_t *chMixerCoeff = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t* payloadInfo = NULL;
    uint16_t* pcmChannel = NULL;
    int numChannels = numCh;

    // Only Stereo Speaker swap is supported
    if (numChannels != 2)
        return;

    PAL_DBG(LOG_TAG, "Enter");
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(param_id_chmixer_coeff_t) +
                  sizeof(chmixer_coeff_t) // Only 1 table is being send currently
                  + sizeof(uint16_t) * (numChannels)
                  + sizeof(uint16_t) * (numChannels)
                  + sizeof(uint16_t) * (numChannels*2);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    mfcMixerCoeff = (param_id_chmixer_coeff_t *)(payloadInfo +
                    sizeof(struct apm_module_param_data_t));
    // Set number of tables
    mfcMixerCoeff->num_coeff_tbls = 1;

    chMixerCoeff = (chmixer_coeff_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t) +
                    sizeof(param_id_chmixer_coeff_t));
    // Set Number of channels for input and output channels
    chMixerCoeff->num_output_channels = numChannels;
    chMixerCoeff->num_input_channels = numChannels;
    pcmChannel = (uint16_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t)+
                    sizeof(param_id_chmixer_coeff_t) +
                    sizeof(chmixer_coeff_t));
    // Populate output channel map
    populateChannelMap(pcmChannel, numChannels);

    pcmChannel = (uint16_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t)+
                    sizeof(param_id_chmixer_coeff_t) +
                    sizeof(chmixer_coeff_t) +
                    sizeof(uint16_t) * numChannels);
    // Populate input channel map
    populateChannelMap(pcmChannel, numChannels);


    pcmChannel = (uint16_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t)+
                    sizeof(param_id_chmixer_coeff_t) +
                    sizeof(chmixer_coeff_t) +
                    sizeof(uint16_t) * numChannels +
                    sizeof(uint16_t) * numChannels);

    populateChannelMixerCoeff(pcmChannel, numChannels, rotationType);

    header->module_instance_id = miid;
    header->error_code = 0x0;
    header->param_id = PARAM_ID_CHMIXER_COEFF;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    *size = payloadSize + padBytes;
    *payload = payloadInfo;

    PAL_DBG(LOG_TAG, "Exit");
}

void PayloadBuilder::payloadCRSMFCMixerCoeff(uint8_t** payload, size_t* size,
                   uint32_t miid)
{
    struct apm_module_param_data_t* header = NULL;
    param_id_chmixer_coeff_t *mfcMixerCoeff = NULL;
    chmixer_coeff_t *chMixerCoeff = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t* payloadInfo = NULL;
    uint16_t* pcmChannel = NULL;
    int numChannels = 2;

    // Only Stereo Speaker is supported
    if (numChannels != 2)
         return;

    PAL_ERR(LOG_TAG, "Enter");
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(param_id_chmixer_coeff_t) +
                  sizeof(chmixer_coeff_t) // Only 1 table is being send currently
                  + sizeof(uint16_t) * (numChannels)
                  + sizeof(uint16_t) * (numChannels)
                  + sizeof(uint16_t) * (numChannels);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    mfcMixerCoeff = (param_id_chmixer_coeff_t *)(payloadInfo +
                    sizeof(struct apm_module_param_data_t));
    // Set number of tables
    mfcMixerCoeff->num_coeff_tbls = 1;

    chMixerCoeff = (chmixer_coeff_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t) +
                    sizeof(param_id_chmixer_coeff_t));
    // Set Number of channels for input and output channels
    chMixerCoeff->num_output_channels = numChannels;
    chMixerCoeff->num_input_channels = 1;
    pcmChannel = (uint16_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t)+
                    sizeof(param_id_chmixer_coeff_t) +
                    sizeof(chmixer_coeff_t));
    // Populate output channel map
    populateChannelMap(pcmChannel, numChannels);

    pcmChannel = (uint16_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t)+
                    sizeof(param_id_chmixer_coeff_t) +
                    sizeof(chmixer_coeff_t) +
                    sizeof(uint16_t) * numChannels);
    // Populate input channel map
    populateChannelMap(pcmChannel, chMixerCoeff->num_input_channels);

    pcmChannel = (uint16_t *) (payloadInfo +
                    sizeof(struct apm_module_param_data_t)+
                    sizeof(param_id_chmixer_coeff_t) +
                    sizeof(chmixer_coeff_t) +
                    sizeof(uint16_t) * numChannels +
                    sizeof(uint16_t) * chMixerCoeff->num_input_channels);
    populateCRSChannelMixerCoeff(pcmChannel, numChannels, chMixerCoeff->num_input_channels);

    header->module_instance_id = miid;
    header->error_code = 0x0;
    header->param_id = PARAM_ID_CHMIXER_COEFF;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    *size = payloadSize + padBytes;
    *payload = payloadInfo;

    PAL_DBG(LOG_TAG, "Exit");
}

void PayloadBuilder::payloadMFCConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct sessionToPayloadParam* data)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_mfc_output_media_fmt_t *mfcConf;
    int numChannels;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (!data) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }
    numChannels = data->numChannel;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_mfc_output_media_fmt_t) +
                  sizeof(uint16_t)*numChannels;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    mfcConf = (struct param_id_mfc_output_media_fmt_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));
    pcmChannel = (uint16_t*)(payloadInfo + sizeof(struct apm_module_param_data_t) +
                                       sizeof(struct param_id_mfc_output_media_fmt_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_MFC_OUTPUT_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    mfcConf->sampling_rate = data->sampleRate;
    mfcConf->bit_width = data->bitWidth;
    mfcConf->num_channels = data->numChannel;
    if (data->ch_info) {
        for (int i = 0; i < data->numChannel; ++i) {
            pcmChannel[i] = (uint16_t) data->ch_info->ch_map[i];
        }
    } else {
        populateChannelMap(pcmChannel, data->numChannel);
    }

    *size = payloadSize + padBytes;
    *payload = payloadInfo;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "sample_rate:%d bit_width:%d num_channels:%d Miid:%d",
                      mfcConf->sampling_rate, mfcConf->bit_width,
                      mfcConf->num_channels, header->module_instance_id);
#else
    PAL_DBG(LOG_TAG, "sample_rate:%d bit_width:%d num_channels:%d Miid:%d",
                      mfcConf->sampling_rate, mfcConf->bit_width,
                      mfcConf->num_channels, header->module_instance_id);
#endif
    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

int PayloadBuilder::payloadPopSuppressorConfig(uint8_t** payload, size_t* size,
                                                uint32_t miid, bool enable)
{
    int status = 0;
    struct apm_module_param_data_t* header = NULL;
    struct param_id_pop_suppressor_mute_config_t *psConf;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_pop_suppressor_mute_config_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        status = -ENOMEM;
        goto exit;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    psConf = (struct param_id_pop_suppressor_mute_config_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_POP_SUPPRESSOR_MUTE_CONFIG;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    psConf->mute_enable = enable;
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "pop suppressor mute enable %d", psConf->mute_enable);

exit:
    return status;
}

PayloadBuilder::PayloadBuilder()
{

}

PayloadBuilder::~PayloadBuilder()
{

}

uint16_t numOfBitsSet(uint32_t lines)
{
    uint16_t numBitsSet = 0;
    while (lines) {
        numBitsSet++;
        lines = lines & (lines - 1);
    }
    return numBitsSet;
}

void PayloadBuilder::resetDataBuf(struct user_xml_data *data)
{
     data->offs = 0;
     data->data_buf[data->offs] = '\0';
}

void PayloadBuilder::processGraphKVData(struct user_xml_data *data, const XML_Char **attr)
{
    struct kvPairs kv = {};
    int size = -1, selector_size = -1;
    std::string key(attr[1]);
    std::string value(attr[3]);

    if (strcmp(attr[0], "key") !=0) {
          PAL_ERR(LOG_TAG, "key not found");
          return;
     }
    kv.key = ResourceManager::convertCharToHex(key);

    if (strcmp(attr[2], "value") !=0) {
        PAL_ERR(LOG_TAG, "value not found");
        return;
    }
    kv.value = ResourceManager::convertCharToHex(value);

    if (data->is_parsing_streams) {
        if (all_streams.size() > 0) {
            size = all_streams.size() - 1;
            if (all_streams[size].keys_values.size() > 0) {
                selector_size = all_streams[size].keys_values.size() - 1;
                all_streams[size].keys_values[selector_size].kv_pairs.push_back(kv);
                PAL_DBG(LOG_TAG, "stream_graph_kv done size: %zu",
                    all_streams[size].keys_values[selector_size].kv_pairs.size());
            }
        }
        return;
    } else if (data->is_parsing_streampps) {
        if (all_streampps.size() > 0) {
            size = all_streampps.size() - 1;
            if (all_streampps[size].keys_values.size() > 0) {
                selector_size = all_streampps[size].keys_values.size() - 1;
                all_streampps[size].keys_values[selector_size].kv_pairs.push_back(kv);
                PAL_DBG(LOG_TAG, "streampp_graph_kv:streampp size: %zu",
                    all_streampps[size].keys_values[selector_size].kv_pairs.size());
            }
        }
        return;
    } else if (data->is_parsing_devices) {
        if (all_devices.size() > 0) {
            size = all_devices.size() - 1;
            if (all_devices[size].keys_values.size() > 0) {
                selector_size = all_devices[size].keys_values.size() - 1;
                all_devices[size].keys_values[selector_size].kv_pairs.push_back(kv);
                PAL_DBG(LOG_TAG, "device_grap_kv:device size: %zu",
                    all_devices[size].keys_values[selector_size].kv_pairs.size());
            }
        }
        return;
    } else if (data->is_parsing_devicepps) {
        if (all_devicepps.size() > 0) {
            size = all_devicepps.size() - 1;
            if (all_devicepps[size].keys_values.size() > 0) {
                selector_size = all_devicepps[size].keys_values.size() - 1;
                all_devicepps[size].keys_values[selector_size].kv_pairs.push_back(kv);
                PAL_DBG(LOG_TAG, "devicepp_graph_kv: devicepp size: %zu",
                    all_devicepps[size].keys_values[selector_size].kv_pairs.size());
            }
        }
        return;
    } else {
        PAL_INFO(LOG_TAG, "No matching XML data\n");
    }
}

std::string PayloadBuilder::removeSpaces(const std::string& str)
{
    /* Remove leading and trailing spaces */
    return std::regex_replace(str, std::regex("^ +| +$|( ) +"), "$1");
}

std::vector<std::string> PayloadBuilder::splitStrings(const std::string& str)
{
    std::vector<std::string> tokens;
    std::stringstream check(str);
    std::string intermediate;

    while (getline(check, intermediate, ',')) {
        if (!removeSpaces(intermediate).empty())
            tokens.push_back(removeSpaces(intermediate));
    }

    return tokens;
}

void PayloadBuilder:: processKVSelectorData(struct user_xml_data *data,
    const XML_Char **attr)
{
    struct kvInfo kvinfo = {};
    int size = -1;

    std::vector<std::string> sel_values_superset;
    PAL_DBG(LOG_TAG, "process kv selectors stream:%d streampp:%d device:%d devicepp:%d",
        data->is_parsing_streams, data->is_parsing_streampps,
        data->is_parsing_devices, data->is_parsing_devicepps);

    for (int i = 0; attr[i]; i += 2) {
        kvinfo.selector_names.push_back(attr[i]);
        sel_values_superset.push_back(attr[i + 1]);
        PAL_DBG(LOG_TAG, "key_values attr :%s-%s", attr[i], attr[i + 1]);
    }

    for (int i = 0; i < kvinfo.selector_names.size(); i++) {
        PAL_DBG(LOG_TAG, "process kv selectors:%s", kvinfo.selector_names[i].c_str());
        selector_type_t selector_type =  selectorstypeLUT.at(kvinfo.selector_names[i]);

        std::vector<std::string> selector_values =
            splitStrings(sel_values_superset[i]);

        for (int j = 0; j < selector_values.size(); j++) {
            kvinfo.selector_pairs.push_back(std::make_pair(selector_type,
                selector_values[j]));
            PAL_DBG(LOG_TAG, "selector pair type:%d, value:%s", selector_type,
                selector_values[j].c_str());
        }
    }

    if (data->is_parsing_streams) {
        if (all_streams.size() > 0) {
            size = all_streams.size() - 1;
            all_streams[size].keys_values.push_back(kvinfo);
            PAL_DBG(LOG_TAG, "stream_keys_values after push_back size:%zu",
                all_streams[size].keys_values.size());
        }
        return;
    } else if (data->is_parsing_streampps) {
        if (all_streampps.size() > 0) {
            size = all_streampps.size() - 1;
            all_streampps[size].keys_values.push_back(kvinfo);
            PAL_DBG(LOG_TAG, "streampp_keys_values after push_back size:%zu",
                all_streampps[size].keys_values.size());
        }
        return;
    } else if (data->is_parsing_devices) {
        if (all_devices.size() > 0) {
            size = all_devices.size() - 1;
            all_devices[size].keys_values.push_back(kvinfo);
            PAL_DBG(LOG_TAG, "device_keys_values after push_back size:%zu",
                all_devices[size].keys_values.size());
        }
        return;
    } else if (data->is_parsing_devicepps) {
        if (all_devicepps.size() > 0) {
            size = all_devicepps.size() - 1;
            all_devicepps[size].keys_values.push_back(kvinfo);
            PAL_DBG(LOG_TAG, "devicepp_keys_values after push_back size:%zu",
                all_devicepps[size].keys_values.size());
        }
        return;
    } else {
        PAL_INFO(LOG_TAG, "No match for is_parsing");
    }
}

void PayloadBuilder:: processKVTypeData(struct user_xml_data *data,const XML_Char **attr)
{
    struct allKVs sdTypeKV = {};
    int32_t stream_id, dev_id;
    std::vector<std::string> typeNames;

    PAL_DBG(LOG_TAG, "stream-device ID/type:%s, tag_name:%d", attr[1], data->tag);
    if (data->tag == TAG_STREAM_SEL || data->tag == TAG_STREAMPP_SEL) {
        if (!strcmp(attr[0], "type")) {
            typeNames = splitStrings(attr[1]);
            for (int i = 0; i < typeNames.size(); i++) {
                stream_id = ResourceManager::getStreamType(typeNames[i]);
                sdTypeKV.id_type.push_back(stream_id);
                PAL_DBG(LOG_TAG, "type name:%s", typeNames[i].c_str());
            }
            if (data->tag == TAG_STREAM_SEL) {
                all_streams.push_back(sdTypeKV);
                PAL_DBG(LOG_TAG, "stream types all_size: %zu", all_streams.size());
            } else if (data->tag == TAG_STREAMPP_SEL) {
                all_streampps.push_back(sdTypeKV);
                PAL_DBG(LOG_TAG, "streampp types all_size: %zu", all_streampps.size());
            }
        }
        return;
    }
    if (data->tag == TAG_DEVICE_SEL || data->tag == TAG_DEVICEPP_SEL) {
        if (!strcmp(attr[0], "id")) {
            typeNames = splitStrings(attr[1]);
            for (int i = 0; i < typeNames.size(); i++) {
                dev_id = ResourceManager::getDeviceId(typeNames[i]);
                sdTypeKV.id_type.push_back(dev_id);
                PAL_DBG(LOG_TAG, "device ID name:%s", typeNames[i].c_str());
            }
            if (data->tag == TAG_DEVICE_SEL) {
                all_devices.push_back(sdTypeKV);
                PAL_DBG(LOG_TAG, " device types all_size: %zu", all_devices.size());
            } else if (data->tag == TAG_DEVICEPP_SEL ) {
                all_devicepps.push_back(sdTypeKV);
                PAL_DBG(LOG_TAG, "devicepp types all_size: %zu", all_devicepps.size() );
            }
        }
        return;
    }
    PAL_INFO(LOG_TAG, "No matching tags found");
    return;
}

bool PayloadBuilder::compareNumSelectors(struct kvInfo info_1, struct kvInfo info_2)
{
    return (info_1.selector_names.size() < info_2.selector_names.size());
}

void PayloadBuilder::startTag(void *userdata, const XML_Char *tag_name,
    const XML_Char **attr)
{
    struct user_xml_data *data = ( struct user_xml_data *)userdata;

    PAL_DBG(LOG_TAG, "StartTag :%s", tag_name);
    if (!strcmp(tag_name, "graph_key_value_pair_info")) {
        data->tag = TAG_USECASEXML_ROOT;
    } else if (!strcmp(tag_name, "streams")) {
        data->is_parsing_streams = true;
    } else if (!strcmp(tag_name, "stream")) {
        data->tag = TAG_STREAM_SEL;
        processKVTypeData(data, attr);
    } else if (!strcmp(tag_name, "keys_and_values")) {
        processKVSelectorData(data, attr);
    } else if (!strcmp(tag_name, "graph_kv")) {
        processGraphKVData(data, attr);
    } else if (!strcmp(tag_name, "streampps")) {
        data->is_parsing_streampps = true;
    } else if (!strcmp(tag_name, "streampp")) {
        data->tag = TAG_STREAMPP_SEL;
        processKVTypeData(data, attr);
    } else if (!strcmp(tag_name, "devices")) {
        data->is_parsing_devices = true;
    } else if (!strcmp(tag_name, "device")){
        data->tag = TAG_DEVICE_SEL;
        processKVTypeData(data, attr);
    } else if (!strcmp(tag_name, "devicepps")){
        data->is_parsing_devicepps = true;
    } else if (!strcmp(tag_name, "devicepp")){
        data->tag = TAG_DEVICEPP_SEL;
        processKVTypeData(data, attr);
    } else {
        PAL_INFO(LOG_TAG, "No matching Tag found");
    }
}

void PayloadBuilder::endTag(void *userdata, const XML_Char *tag_name)
{
    struct user_xml_data *data = ( struct user_xml_data *)userdata;
    int size = -1;

    PAL_DBG(LOG_TAG, "Endtag: %s", tag_name);
    if ( !strcmp(tag_name, "keys_and_values") || !strcmp(tag_name, "graph_kv")) {
        return;
    }
    if (!strcmp(tag_name, "streams")) {
        data->is_parsing_streams = false;
        PAL_DBG(LOG_TAG, "is_parsing_streams: %d", data->is_parsing_streams);
        return;
    }
    if (!strcmp(tag_name, "streampps")){
        data->is_parsing_streampps = false;
        PAL_DBG(LOG_TAG, "is_parsing_streampps: %d", data->is_parsing_streampps);
        return;
    }
    if (!strcmp(tag_name, "devices")) {
        data->is_parsing_devices = false;
        PAL_DBG(LOG_TAG, "is_parsing_devices: %d", data->is_parsing_devices);
        return;
    }
    if (!strcmp(tag_name, "devicepps")) {
        data->is_parsing_devicepps = false;
        PAL_DBG(LOG_TAG, "is_parsing_devicepps : %d", data->is_parsing_devicepps);
        return;
    }
    if (!strcmp(tag_name, "stream")) {
        if (all_streams.size() > 0) {
            size = all_streams.size() - 1;
            /* Sort the key value tags based on number of selectors in each tag */
            std::sort(all_streams[size].keys_values.begin(),
                all_streams[size].keys_values.end(),
                compareNumSelectors);
        }
        return;
    }
    if (!strcmp(tag_name, "streampp")){
        if (all_streampps.size() > 0) {
            size = all_streampps.size() - 1;
            std::sort(all_streampps[size].keys_values.begin(),
                all_streampps[size].keys_values.end(),
                compareNumSelectors);
        }
        return;
    }
    if (!strcmp(tag_name, "device")) {
        if (all_devices.size() > 0) {
            size = all_devices.size() - 1;
            std::sort(all_devices[size].keys_values.begin(),
                all_devices[size].keys_values.end(),
                compareNumSelectors);
        }
        return;
    }
    if (!strcmp(tag_name, "devicepp")) {
        if (all_devicepps.size() > 0) {
            size = all_devicepps.size() - 1;
            std::sort(all_devicepps[size].keys_values.begin(),
                all_devicepps[size].keys_values.end(),
                compareNumSelectors);
        }
        return;
    }
    return;
}

void PayloadBuilder::handleData(void *userdata, const char *s, int len)
{
   struct user_xml_data *data = (struct user_xml_data *)userdata;
   if (len + data->offs >= sizeof(data->data_buf) ) {
       data->offs += len;
       /* string length overflow, return */
       return;
   } else {
        memcpy(data->data_buf + data->offs, s, len);
         data->offs += len;
   }
}

int PayloadBuilder::init()
{
    XML_Parser parser;
    FILE *file = NULL;
    int ret = 0;
    int bytes_read;
    void *buf = NULL;
    struct user_xml_data tag_data;
    memset(&tag_data, 0, sizeof(tag_data));
    all_streams.clear();
    all_streampps.clear();
    all_devices.clear();
    all_devicepps.clear();

    PAL_INFO(LOG_TAG, "XML parsing started %s", USECASE_XML_FILE);
    file = fopen(USECASE_XML_FILE, "r");
    if (!file) {
        PAL_ERR(LOG_TAG, "Failed to open xml");
        ret = -EINVAL;
        goto done;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        PAL_ERR(LOG_TAG, "Failed to create XML");
        goto closeFile;
    }
    XML_SetUserData(parser,&tag_data);
    XML_SetElementHandler(parser, startTag, endTag);
    XML_SetCharacterDataHandler(parser, handleData);

    while (1) {
        buf = XML_GetBuffer(parser, 1024);
        if (buf == NULL) {
            PAL_ERR(LOG_TAG, "XML_Getbuffer failed");
            ret = -EINVAL;
            goto freeParser;
        }

        bytes_read = fread(buf, 1, 1024, file);
        if (bytes_read < 0) {
            PAL_ERR(LOG_TAG, "fread failed");
            ret = -EINVAL;
            goto freeParser;
        }

        if (XML_ParseBuffer(parser, bytes_read, bytes_read == 0) == XML_STATUS_ERROR) {
            PAL_ERR(LOG_TAG, "XML ParseBuffer failed ");
            ret = -EINVAL;
            goto freeParser;
        }
        if (bytes_read == 0)
            break;
    }

freeParser:
    XML_ParserFree(parser);
closeFile:
    fclose(file);
done:
    return ret;
}

void PayloadBuilder::payloadTimestamp(std::shared_ptr<std::vector<uint8_t>>& payload,
                                      size_t *size, uint32_t moduleId)
{
    size_t payloadSize, padBytes;
    struct apm_module_param_data_t* header;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_spr_session_time_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
    payload = std::make_shared<std::vector<uint8_t>>(payloadSize + padBytes);
    if (!payload) {
        PAL_ERR(LOG_TAG, "payload malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payload->data();
    header->module_instance_id = moduleId;
    header->param_id = PARAM_ID_SPR_SESSION_TIME;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);
    PAL_VERBOSE(LOG_TAG, "header params IID:%x param_id:%x error_code:%d param_size:%d",
                  header->module_instance_id, header->param_id,
                  header->error_code, header->param_size);
    *size = payloadSize + padBytes;
    PAL_DBG(LOG_TAG, "payload %pK size %zu", payload->data(), *size);
}

int PayloadBuilder::payloadACDBTunnelParam(uint8_t **alsaPayload,
            size_t *size, uint8_t *payload,
            const std::set <std::pair<int, int>> &acdbGKVSet,
            uint32_t moduleInstanceId, uint32_t sampleRate) {
    struct apm_module_param_data_t* header;
    struct agm_acdb_tunnel_param *payloadACDBTunnelInfo = NULL;
    size_t paddedPayloadSize = 0;
    uint32_t payloadSize = 0;
    uint32_t totalPaddedSize = 0;
    uint32_t parsedSize = 0;
    uint32_t dataLength = 0;
    struct agm_acdb_param *acdbParam = (struct agm_acdb_param *)payload;
    uint32_t appendSampleRateInCKV = 1;
    uint8_t *ptrSrc = nullptr;
    uint8_t *ptrDst = nullptr;
    uint32_t *ptr = nullptr;
    uint32_t offset = 0;
    pal_effect_custom_payload_t *effectCustomPayload = nullptr;
    uint32_t checkSum = 0;

    if (!acdbParam)
        return -EINVAL;

    if (sampleRate != 0 && acdbParam->isTKV == PARAM_TKV) {
        PAL_ERR(LOG_TAG, "Sample Rate %d CKV and TKV are not compatible.",
                    sampleRate);
        return -EINVAL;
    }

    if (sampleRate) {
        //CKV
        // step 1. check sample rate is in kv or not
        PAL_INFO(LOG_TAG, "CKV param to ACDB");
        pal_key_value_pair_t *rawCKVPair = nullptr;
        rawCKVPair = (pal_key_value_pair_t *)acdbParam->blob;
        for (int k = 0; k < acdbParam->num_kvs; k++) {
            if (rawCKVPair[k].key == SAMPLINGRATE) {
                PAL_INFO(LOG_TAG, "Sample rate is in CKV. No need to append.");
                appendSampleRateInCKV = 0;
                break;
            }
        }
        PAL_DBG(LOG_TAG, "is sample rate appended in CKV? %x",
                                appendSampleRateInCKV);
    } else {
        //TKV
        appendSampleRateInCKV = 0;
    }

    // multipl param check by param id
    dataLength = sizeof(struct agm_acdb_param) +
                    acdbParam->num_kvs * sizeof(struct gsl_key_value_pair);
    effectCustomPayload = (pal_effect_custom_payload_t *)
                                ((uint8_t *)acdbParam + dataLength);
    PAL_DBG(LOG_TAG, "first param id = 0x%x", effectCustomPayload->paramId);
    // step 1: get param data size = blob size - kv size - param id size
    __builtin_add_overflow(acdbParam->num_kvs * sizeof(struct gsl_key_value_pair),
                             sizeof(pal_effect_custom_payload_t), &checkSum);
    __builtin_sub_overflow(acdbParam->blob_size, checkSum, &payloadSize);
    PAL_DBG(LOG_TAG, "payloadSize = 0x%x", payloadSize);

    if (effectCustomPayload->paramId) {
        paddedPayloadSize = PAL_ALIGN_8BYTE(payloadSize);
        PAL_INFO(LOG_TAG, "payloadSize=%d paddedPayloadSize=%x",
                    payloadSize, paddedPayloadSize);
        payloadACDBTunnelInfo = (struct agm_acdb_tunnel_param *)calloc(1,
            sizeof(struct agm_acdb_tunnel_param) +
            (acdbParam->num_kvs + appendSampleRateInCKV + acdbGKVSet.size()) *
            sizeof(struct gsl_key_value_pair) +
            sizeof(struct apm_module_param_data_t) + paddedPayloadSize);
    } else {
        PAL_INFO(LOG_TAG, "This is multiple parameter case.");
        payloadACDBTunnelInfo = (struct agm_acdb_tunnel_param *)calloc(1,
            sizeof(struct agm_acdb_tunnel_param) +
            (acdbParam->num_kvs + appendSampleRateInCKV + acdbGKVSet.size()) *
            sizeof(struct gsl_key_value_pair) +
            sizeof(struct apm_module_param_data_t) + payloadSize * 2);
    }

    if (!payloadACDBTunnelInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return -ENOMEM;
    }

    // copy meta
    payloadACDBTunnelInfo->isTKV = acdbParam->isTKV;
    payloadACDBTunnelInfo->tag = acdbParam->tag;
    payloadACDBTunnelInfo->num_gkvs = acdbGKVSet.size();
    payloadACDBTunnelInfo->num_kvs = acdbParam->num_kvs;

    // copy gkv
    offset = sizeof(struct agm_acdb_tunnel_param);
    dataLength = acdbGKVSet.size() * sizeof(struct gsl_key_value_pair);
    ptrDst = (uint8_t *)payloadACDBTunnelInfo + offset;
    ptr = (uint32_t *)ptrDst;
    for (const auto& ele: acdbGKVSet) {
        *ptr++ = ele.first;
        *ptr++ = ele.second;
    }

    // copy tkv or ckv
    offset += dataLength;
    dataLength = acdbParam->num_kvs * sizeof(struct gsl_key_value_pair);
    ptrDst = (uint8_t *)payloadACDBTunnelInfo + offset;
    ptrSrc = (uint8_t *)acdbParam + sizeof(struct agm_acdb_param);
    ar_mem_cpy(ptrDst, dataLength, acdbParam->blob, dataLength);

    //update ckv for sample rate
    offset += dataLength;
    payloadACDBTunnelInfo->num_kvs += appendSampleRateInCKV;
    if (appendSampleRateInCKV) {
        dataLength = sizeof(struct gsl_key_value_pair);
        ptrDst = (uint8_t *)payloadACDBTunnelInfo + offset;
        ptr = (uint32_t *)ptrDst;
        *ptr++ = SAMPLINGRATE;
        *ptr = sampleRate;
        header = (struct apm_module_param_data_t *)(
                    (uint8_t *)payloadACDBTunnelInfo +
                    offset + sizeof(struct gsl_key_value_pair));
    } else {
        dataLength = 0;
        header = (struct apm_module_param_data_t *)
                    ((uint8_t *)payloadACDBTunnelInfo + offset);
    }

    offset += dataLength;
    /* actual param pointer */
    if (effectCustomPayload->paramId) {
        PAL_INFO(LOG_TAG, "This is single param id=0x%x",
            effectCustomPayload->paramId);
        header->module_instance_id = acdbParam->tag;
        header->param_id = effectCustomPayload->paramId;
        header->param_size = payloadSize;
        header->error_code = 0x0;
        PAL_DBG(LOG_TAG, "tag = 0x%x", acdbParam->tag);
        PAL_DBG(LOG_TAG, "padded payload size = 0x%x", paddedPayloadSize);
        if (paddedPayloadSize) {
            ptrDst = (uint8_t *)header + sizeof(struct apm_module_param_data_t);
            ptrSrc = (uint8_t *)effectCustomPayload->data;
            // padded bytes are zero by calloc. no need to copy.
            ar_mem_cpy(ptrDst, payloadSize, ptrSrc, payloadSize);
        }

        offset += sizeof(struct apm_module_param_data_t) + paddedPayloadSize;
        *size = offset;
        *alsaPayload = (uint8_t *)payloadACDBTunnelInfo;
        payloadACDBTunnelInfo->blob_size = (payloadACDBTunnelInfo->num_gkvs +
            payloadACDBTunnelInfo->num_kvs) * sizeof(struct gsl_key_value_pair)
            + sizeof(struct apm_module_param_data_t) + paddedPayloadSize;
    } else {
        PAL_INFO(LOG_TAG, "This is multiple param case.");
        legacyGefParamHeader *gefMultipleParamHeader = NULL;
        while (parsedSize < payloadSize) {
            PAL_INFO(LOG_TAG, "parsed size = 0x%x", parsedSize);
            gefMultipleParamHeader =
                (legacyGefParamHeader *)
                ((uint8_t *)(effectCustomPayload->data) + parsedSize);
            paddedPayloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                                + gefMultipleParamHeader->length);
            PAL_INFO(LOG_TAG, "total padded size = 0x%x current padded size=0x%x",
                        totalPaddedSize, paddedPayloadSize);
            PAL_INFO(LOG_TAG, "current param value length = 0x%x",
                        gefMultipleParamHeader->length);
            header->module_instance_id = acdbParam->tag;
            header->param_id = gefMultipleParamHeader->paramId;
            header->error_code = 0x0;
            header->param_size = gefMultipleParamHeader->length;
            PAL_DBG(LOG_TAG, "tag=0x%x param id = 0x%x param length=0x%x",
                        header->module_instance_id, header->param_id,
                        header->param_size);
            if (gefMultipleParamHeader->length) {
                ar_mem_cpy((uint8_t *)header +
                                sizeof(struct apm_module_param_data_t),
                                gefMultipleParamHeader->length,
                                (uint8_t *)gefMultipleParamHeader +
                                sizeof(legacyGefParamHeader),
                                gefMultipleParamHeader->length);
            }

            // offset to output data
            totalPaddedSize += paddedPayloadSize;
            // offset to input data
            parsedSize += sizeof(legacyGefParamHeader) +
                            gefMultipleParamHeader->length;
            PAL_DBG(LOG_TAG, "parsed size=0x%x total padded size=0x%x",
                                parsedSize, totalPaddedSize);
            header = (struct apm_module_param_data_t*)((uint8_t *)header + paddedPayloadSize);
        }

        payloadACDBTunnelInfo->blob_size =
            (payloadACDBTunnelInfo->num_kvs + payloadACDBTunnelInfo->num_gkvs)
            * sizeof(struct gsl_key_value_pair)
            + totalPaddedSize;

        offset += totalPaddedSize;
        *size = offset;
        *alsaPayload = (uint8_t *)payloadACDBTunnelInfo;
    }

    PAL_ERR(LOG_TAG, "ALSA payload %pK size %zu", *alsaPayload, *size);

    return 0;
}

int PayloadBuilder::payloadACDBParam(uint8_t **alsaPayload, size_t *size,
            uint8_t *payload,
            uint32_t moduleInstanceId, uint32_t sampleRate) {
    struct apm_module_param_data_t* header;
    //uint8_t* payloadInfo = NULL;
    struct agm_acdb_param *payloadInfo = NULL;
    size_t paddedSize = 0;
    uint32_t payloadSize = 0;
    uint32_t dataLength = 0;
    struct agm_acdb_param *acdbParam = (struct agm_acdb_param *)payload;
    uint32_t appendSampleRateInCKV = 1;
    uint8_t *ptrSrc = nullptr;
    uint8_t *ptrDst = nullptr;
    uint32_t *ptr = nullptr;
    pal_effect_custom_payload_t *effectCustomPayload = nullptr;
    uint32_t totalPaddedSize = 0;
    uint32_t parsedSize = 0;
    struct agm_acdb_param *repackedData = nullptr;

    if (!acdbParam)
        return -EINVAL;

    if (sampleRate != 0 && acdbParam->isTKV == PARAM_TKV) {
        PAL_ERR(LOG_TAG, "Sample Rate %d CKV and TKV are not compatible.",
                    sampleRate);
        return -EINVAL;
    }

    if (sampleRate) {
        //CKV
        // step 1. check sample rate is in kv or not
        PAL_INFO(LOG_TAG, "CKV param to ACDB");
        pal_key_value_pair_t *rawCKVPair = nullptr;
        rawCKVPair = (pal_key_value_pair_t *)acdbParam->blob;
        for (int k = 0; k < acdbParam->num_kvs; k++) {
            if (rawCKVPair[k].key == SAMPLINGRATE) {
                PAL_INFO(LOG_TAG, "Sample rate is in CKV. No need to append.");
                appendSampleRateInCKV = 0;
                break;
            }
        }
        PAL_DBG(LOG_TAG, "is sample rate appended in CKV? %x",
                                appendSampleRateInCKV);
    } else {
        //TKV
        appendSampleRateInCKV = 0;
    }

    // multipl param check by param id
    dataLength = sizeof(struct agm_acdb_param) +
                    acdbParam->num_kvs * sizeof(struct gsl_key_value_pair);
    effectCustomPayload = (pal_effect_custom_payload_t *)
                                ((uint8_t *)acdbParam + dataLength);
    if (effectCustomPayload->paramId) {
        // step 1: get param data size = blob size - kv size - param id size
        payloadSize = acdbParam->blob_size -
                        acdbParam->num_kvs * sizeof(struct gsl_key_value_pair)
                        - sizeof(pal_effect_custom_payload_t);
        paddedSize = PAL_ALIGN_8BYTE(payloadSize);
        PAL_INFO(LOG_TAG, "payloadSize=%d paddedSize=%x", payloadSize, paddedSize);
        payloadInfo = (struct agm_acdb_param *)calloc(1,
            sizeof(struct agm_acdb_param) +
            (acdbParam->num_kvs + appendSampleRateInCKV) *
            sizeof(struct gsl_key_value_pair) +
            sizeof(struct apm_module_param_data_t) + paddedSize);
        if (!payloadInfo) {
            PAL_ERR(LOG_TAG, "failed to allocate memory.");
            return -ENOMEM;
        }

        // copy acdb meta + kv
        dataLength = sizeof(struct agm_acdb_param) +
                        acdbParam->num_kvs * sizeof(struct gsl_key_value_pair);
        ar_mem_cpy((uint8_t *)payloadInfo, dataLength,
                    (uint8_t *)acdbParam, dataLength);
        //update blob size
        payloadInfo->blob_size = payloadInfo->blob_size +
                                sizeof(struct apm_module_param_data_t) -
                                sizeof(pal_effect_custom_payload_t) +
                                appendSampleRateInCKV * sizeof(struct gsl_key_value_pair)
                                + paddedSize - payloadSize;
        payloadInfo->num_kvs = payloadInfo->num_kvs + appendSampleRateInCKV;
        if (appendSampleRateInCKV) {
            ptr = (uint32_t *)((uint8_t *)payloadInfo + dataLength);
            *ptr++ = SAMPLINGRATE;
            *ptr = sampleRate;
            header = (struct apm_module_param_data_t *)((uint8_t *)payloadInfo +
                        dataLength + sizeof(struct gsl_key_value_pair));
        } else {
            header = (struct apm_module_param_data_t *)
                        ((uint8_t *)payloadInfo + dataLength);
        }

        effectCustomPayload = (pal_effect_custom_payload_t *)
                                    ((uint8_t *)acdbParam + dataLength);
        header->module_instance_id = moduleInstanceId;
        header->param_id = effectCustomPayload->paramId;
        header->param_size = payloadSize;
        header->error_code = 0x0;

        /* padded size = payload size + appended sze */
        if (paddedSize) {
            ptrDst = (uint8_t *)header + sizeof(struct apm_module_param_data_t);
            ptrSrc = (uint8_t *)effectCustomPayload->data;
            // padded bytes are zereo by calloc. no need to copy.
            ar_mem_cpy(ptrDst, payloadSize, ptrSrc, payloadSize);
        }
        *size = dataLength + paddedSize + sizeof(struct apm_module_param_data_t) +
                    appendSampleRateInCKV * sizeof(struct gsl_key_value_pair);
        *alsaPayload = (uint8_t *)payloadInfo;

    } else {
        // step 1: get param data size = blob size - kv size - param id size
        payloadSize = acdbParam->blob_size -
                        acdbParam->num_kvs * sizeof(struct gsl_key_value_pair)
                        - sizeof(pal_effect_custom_payload_t);

        repackedData =
                (struct agm_acdb_param *)calloc(1,
                    sizeof(struct agm_acdb_param) +
                    (acdbParam->num_kvs + appendSampleRateInCKV) *
                    sizeof(struct gsl_key_value_pair) +
                    sizeof(struct apm_module_param_data_t) + payloadSize * 2);

        if (!repackedData) {
                PAL_ERR(LOG_TAG, "failed to allocate memory of 0x%x bytes",
                                        payloadSize * 2);
                return -ENOMEM;
        }

        legacyGefParamHeader *gefMultipleParamHeader = NULL;
        // copy acdb meta + kv
        dataLength = sizeof(struct agm_acdb_param) +
                        acdbParam->num_kvs * sizeof(struct gsl_key_value_pair);

        ar_mem_cpy((uint8_t *)repackedData, dataLength,
                    (uint8_t *)acdbParam, dataLength);

        repackedData->num_kvs = acdbParam->num_kvs + appendSampleRateInCKV;
        if (appendSampleRateInCKV) {
            ptr = (uint32_t *)((uint8_t *)repackedData + dataLength);
            *ptr++ = SAMPLINGRATE;
            *ptr = sampleRate;
            header = (struct apm_module_param_data_t *)((uint8_t *)repackedData +
                        dataLength + sizeof(struct gsl_key_value_pair));
        } else {
            header = (struct apm_module_param_data_t *)
                        ((uint8_t *)repackedData + dataLength);
        }

        while (parsedSize < payloadSize) {
            PAL_DBG(LOG_TAG, "parsed size = 0x%x", parsedSize);
            gefMultipleParamHeader =
                (legacyGefParamHeader *)
                ((uint8_t *)(effectCustomPayload->data) + parsedSize);
            paddedSize= PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                                + gefMultipleParamHeader->length);
            PAL_DBG(LOG_TAG, "total padded size = 0x%x paddedSize=0x%x",
                        totalPaddedSize, paddedSize);
            PAL_DBG(LOG_TAG, "current param value length = 0x%x",
                        gefMultipleParamHeader->length);
            header->module_instance_id = moduleInstanceId;
            header->param_id = gefMultipleParamHeader->paramId;
            header->error_code = 0x0;
            header->param_size = gefMultipleParamHeader->length;
            PAL_DBG(LOG_TAG, "miid=0x%x param id = 0x%x length=0x%x",
                        header->module_instance_id, header->param_id, header->param_size);
            if (gefMultipleParamHeader->length) {
                ar_mem_cpy((uint8_t *)header + sizeof(struct apm_module_param_data_t),
                                 gefMultipleParamHeader->length,
                                 (uint8_t *)gefMultipleParamHeader + sizeof(legacyGefParamHeader),
                                 gefMultipleParamHeader->length);
            }
            // offset to output data
            totalPaddedSize += paddedSize;
            // offset to input data
            parsedSize += sizeof(legacyGefParamHeader) +
                            gefMultipleParamHeader->length;
            PAL_DBG(LOG_TAG, "parsed size=0x%x total padded size=0x%x",
                                parsedSize, totalPaddedSize);
            header = (struct apm_module_param_data_t*)((uint8_t *)header + paddedSize);
        }

        repackedData->blob_size = acdbParam->num_kvs * sizeof(struct gsl_key_value_pair)
                                    + totalPaddedSize;
        *size = dataLength + totalPaddedSize +
                    appendSampleRateInCKV * sizeof(struct gsl_key_value_pair);
        *alsaPayload = (uint8_t *)repackedData;
    }

    PAL_DBG(LOG_TAG, "ALSA payload %pK size %zu", *alsaPayload, *size);

    return 0;
}

int PayloadBuilder::payloadCustomParam(uint8_t **alsaPayload, size_t *size,
            uint32_t *customPayload, uint32_t customPayloadSize,
            uint32_t moduleInstanceId, uint32_t paramId) {
    struct apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t alsaPayloadSize = 0;
    uint32_t totalPaddedSize = 0;
    uint32_t parsedSize = 0;

    PAL_DBG(LOG_TAG, "param id = 0x%x", paramId);
    if (paramId) {
        alsaPayloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                            + customPayloadSize);
        payloadInfo = (uint8_t *)calloc(1, (size_t)alsaPayloadSize);
        if (!payloadInfo) {
            PAL_ERR(LOG_TAG, "failed to allocate memory.");
            return -ENOMEM;
        }

        header = (struct apm_module_param_data_t*)payloadInfo;
        header->module_instance_id = moduleInstanceId;
        header->param_id = paramId;
        header->error_code = 0x0;
        header->param_size = customPayloadSize;
        if (customPayloadSize)
            ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                             customPayloadSize,
                             customPayload,
                             customPayloadSize);
        *size = alsaPayloadSize;
        *alsaPayload = payloadInfo;
    } else {
        // make sure memory is big enough to handle padding
        uint8_t *repackedData = (uint8_t *)calloc(1, customPayloadSize * 2);
        if (!repackedData) {
            PAL_ERR(LOG_TAG, "failed to allocate memory of 0x%x bytes",
                        customPayloadSize * 2);
            return -ENOMEM;
        }
        legacyGefParamHeader *gefMultipleParamHeader = NULL;
        PAL_DBG(LOG_TAG, "custom payloadsize=0x%x", customPayloadSize);

        while (parsedSize < customPayloadSize) {
            gefMultipleParamHeader =
                (legacyGefParamHeader *)((uint8_t *)customPayload + parsedSize);
            alsaPayloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                                + gefMultipleParamHeader->length);
            PAL_DBG(LOG_TAG, "total padded size = 0x%x alsapayloadsize=0x%x",
                        totalPaddedSize, alsaPayloadSize);
            PAL_DBG(LOG_TAG, "current param length = 0x%x",
                        gefMultipleParamHeader->length);
            payloadInfo = repackedData + totalPaddedSize;
            header = (struct apm_module_param_data_t*)payloadInfo;
            header->module_instance_id = moduleInstanceId;
            header->param_id = gefMultipleParamHeader->paramId;
            header->error_code = 0x0;
            header->param_size = gefMultipleParamHeader->length;

            if (gefMultipleParamHeader->length)
                ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                                 gefMultipleParamHeader->length,
                                 (uint8_t *)customPayload + parsedSize +
                                 sizeof(legacyGefParamHeader),
                                 gefMultipleParamHeader->length);

            totalPaddedSize += alsaPayloadSize;
            parsedSize += sizeof(legacyGefParamHeader) +
                            gefMultipleParamHeader->length;
            PAL_DBG(LOG_TAG, "parsed size=0x%x total padded size=0x%x",
                                parsedSize, totalPaddedSize);
        }
        *size = totalPaddedSize;
        *alsaPayload = repackedData;
    }

    PAL_DBG(LOG_TAG, "ALSA payload %pK size %zu", *alsaPayload, *size);

    return 0;
}

int PayloadBuilder::payloadConfig(uint8_t **payload, size_t *size,
                                     uint8_t *config, size_t config_size,
                                     uint32_t miid, uint32_t param_id) {
    struct apm_module_param_data_t* header = nullptr;
    uint8_t* payloadInfo = nullptr;
    size_t payloadSize = 0;

    payloadSize = PAL_ALIGN_8BYTE(
                  sizeof(struct apm_module_param_data_t) + config_size);
    payloadInfo = (uint8_t *)calloc(1, payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return -ENOMEM;
    }

    header = (struct apm_module_param_data_t *)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = config_size;
    if (config_size) {
        ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
            config_size, config, config_size);
    }
    *size = payloadSize;
    *payload = payloadInfo;

    PAL_INFO(LOG_TAG, "PID 0x%x, payload %pK size %zu", param_id, *payload, *size);

    return 0;
}

void PayloadBuilder::payloadQuery(uint8_t **payload, size_t *size,
                    uint32_t moduleId, uint32_t paramId, uint32_t querySize)
{
    struct apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) + querySize;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = moduleId;
    header->param_id = paramId;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    *size = payloadSize + padBytes;
    *payload = payloadInfo;
}


int PayloadBuilder::payloadDualMono(uint8_t **payloadInfo)
{
    uint8_t *payload = NULL;
    uint32_t payload_size = 0;
    uint16_t *update_params_value16 = nullptr;

    payload_size = sizeof(pal_param_payload) + sizeof(effect_pal_payload_t) +
                    sizeof(pal_effect_custom_payload_t) + CUSTOM_STEREO_CMD_PARAM_SIZE;
    payload = (uint8_t *)calloc(1, payload_size);
    if (!payload) {
        ALOGE("%s: no mem. %d\n", __func__, __LINE__);
        return -ENOMEM;
    }

    pal_param_payload *pal_payload = (pal_param_payload *)payload;
    pal_payload->payload_size = payload_size - sizeof(pal_param_payload);
    effect_pal_payload_t *effect_payload = nullptr;
    effect_payload = (effect_pal_payload_t *)(payload +
            sizeof(pal_param_payload));
    effect_payload->isTKV = PARAM_NONTKV;
    effect_payload->tag = PER_STREAM_PER_DEVICE_MFC;
    effect_payload->payloadSize = payload_size - sizeof(pal_param_payload)
                                    - sizeof(effect_pal_payload_t);
    pal_effect_custom_payload_t *custom_stereo_payload =
        (pal_effect_custom_payload_t *)(payload +
            sizeof(pal_param_payload) + sizeof(effect_pal_payload_t));
    custom_stereo_payload->paramId = PARAM_ID_CHMIXER_COEFF;
    custom_stereo_payload->data[0] = 1;// num of coeff table
    update_params_value16 = (uint16_t *)&(custom_stereo_payload->data[1]);
    /*for stereo mixing num out ch*/
    *update_params_value16++ = CUSTOM_STEREO_NUM_OUT_CH;
    /*for stereo mixing num in ch*/
    *update_params_value16++ = CUSTOM_STEREO_NUM_IN_CH;
    /* Out ch map FL/FR*/
    *update_params_value16++ = PCM_CHANNEL_L;
    *update_params_value16++ = PCM_CHANNEL_R;
    /* In ch map FL/FR*/
    *update_params_value16++ = PCM_CHANNEL_L;
    *update_params_value16++ = PCM_CHANNEL_R;
    /* weight */
    *update_params_value16++ = Q14_GAIN_ZERO_POINT_FIVE;
    *update_params_value16++ = Q14_GAIN_ZERO_POINT_FIVE;
    *update_params_value16++ = Q14_GAIN_ZERO_POINT_FIVE;
    *update_params_value16++ = Q14_GAIN_ZERO_POINT_FIVE;
    *payloadInfo = payload;

    return 0;
}

void PayloadBuilder::payloadGetParam(Stream* s, uint8_t **payload, size_t *size, uint32_t moduleId,
                                     uint32_t param_id, size_t config_size)
{
    PAL_DBG(LOG_TAG, "param id : 0x%x, moduleID 0x%x", param_id, moduleId);

    struct apm_module_param_data_t* header;
    struct param_id_asr_output_t* asrOutputParam = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) + config_size;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = moduleId;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    if (param_id == PARAM_ID_ASR_OUTPUT) {
        asrOutputParam = (struct param_id_asr_output_t *)
                         (payloadInfo + (sizeof(struct apm_module_param_data_t)));
        asrOutputParam->output_token = s->GetOutputToken();
        asrOutputParam->num_outputs = s->GetNumEvents();
        asrOutputParam->payload_size =  s->GetPayloadSize();
    }

    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "payload %pK size %zu", *payload, *size);
}

void PayloadBuilder::payloadAFSInfo(uint8_t **payload, size_t *size, uint32_t moduleId)
{
    struct apm_module_param_data_t* header = NULL;
    struct amdb_param_id_module_version_info_t *module_version_info = NULL;
    struct amdb_module_version_info_payload_t *module_version_payload = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct amdb_param_id_module_version_info_t) +
                  sizeof(struct amdb_module_version_info_payload_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo memory allocation failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = AMDB_MODULE_INSTANCE_ID;
    header->param_id = AMDB_PARAM_ID_MODULE_VERSION_INFO;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    module_version_info = (amdb_param_id_module_version_info_t*)(payloadInfo +
                           sizeof(apm_module_param_data_t));
    module_version_info->proc_domain = 2;
    module_version_info->num_modules = 1;

    module_version_payload = (amdb_module_version_info_payload_t*)(payloadInfo +
                              sizeof(apm_module_param_data_t) +
                              sizeof(amdb_param_id_module_version_info_t));
    module_version_payload->module_id = 0x070010D2;


    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "payload size %zu", *size);
}

void PayloadBuilder::payloadTWSConfig(uint8_t** payload, size_t* size,
        uint32_t miid, bool isTwsMonoModeOn, uint32_t codecFormat)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    uint32_t param_id = 0, val = 2;
    size_t payloadSize = 0, customPayloadSize = 0;
    param_id_aptx_classic_switch_enc_pcm_input_payload_t *aptx_classic_payload;
    param_id_aptx_adaptive_enc_switch_to_mono_t *aptx_adaptive_payload;

    if (codecFormat == CODEC_TYPE_APTX_DUAL_MONO) {
        param_id = PARAM_ID_APTX_CLASSIC_SWITCH_ENC_PCM_INPUT;
        customPayloadSize = sizeof(param_id_aptx_classic_switch_enc_pcm_input_payload_t);
    } else {
        param_id = PARAM_ID_APTX_ADAPTIVE_ENC_SWITCH_TO_MONO;
        customPayloadSize = sizeof(param_id_aptx_adaptive_enc_switch_to_mono_t);
    }
    payloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                        + customPayloadSize);
    payloadInfo = (uint8_t *)calloc(1, (size_t)payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = customPayloadSize;
    val = (isTwsMonoModeOn == true) ? 1 : 2;
    if (codecFormat == CODEC_TYPE_APTX_DUAL_MONO) {
        aptx_classic_payload =
            (param_id_aptx_classic_switch_enc_pcm_input_payload_t*)(payloadInfo +
             sizeof(struct apm_module_param_data_t));
        aptx_classic_payload->transition_direction = val;
        ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                         customPayloadSize,
                         aptx_classic_payload,
                         customPayloadSize);
    } else {
        aptx_adaptive_payload =
            (param_id_aptx_adaptive_enc_switch_to_mono_t*)(payloadInfo +
             sizeof(struct apm_module_param_data_t));
        aptx_adaptive_payload->switch_between_mono_and_stereo = val;
        ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                         customPayloadSize,
                         aptx_adaptive_payload,
                         customPayloadSize);
    }

    *size = payloadSize;
    *payload = payloadInfo;
}

void PayloadBuilder::payloadNRECConfig(uint8_t** payload, size_t* size,
        uint32_t miid, bool isNrecEnabled)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    uint32_t param_id = 0, val = 0;
    size_t payloadSize = 0, customPayloadSize = 0;
    qcmn_global_effect_param_t *nrec_payload;

    param_id = PARAM_ID_FLUENCE_CMN_GLOBAL_EFFECT;
    customPayloadSize = sizeof(qcmn_global_effect_param_t);

    payloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                        + customPayloadSize);
    payloadInfo = (uint8_t *)calloc(1, (size_t)payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = customPayloadSize;
    val = (isNrecEnabled == true) ? 0x3 : 0x0;

    nrec_payload =
        (qcmn_global_effect_param_t *)(payloadInfo +
         sizeof(struct apm_module_param_data_t));
    nrec_payload->ecns_effect_mode = val;
    ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                     customPayloadSize,
                     nrec_payload,
                     customPayloadSize);

    *size = payloadSize;
    *payload = payloadInfo;
}


void PayloadBuilder::payloadLC3Config(uint8_t** payload, size_t* size,
        uint32_t miid, bool isLC3MonoModeOn)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    uint32_t param_id = 0, val = 2;
    size_t payloadSize = 0, customPayloadSize = 0;
    param_id_lc3_encoder_switch_enc_pcm_input_payload_t *lc3_payload;

    param_id = PARAM_ID_LC3_ENC_DOWNMIX_2_MONO;
    customPayloadSize = sizeof(param_id_lc3_encoder_switch_enc_pcm_input_payload_t);

    payloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                        + customPayloadSize);
    payloadInfo = (uint8_t *)calloc(1, (size_t)payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = customPayloadSize;
    val = (isLC3MonoModeOn == true) ? 1 : 2;

    lc3_payload =
        (param_id_lc3_encoder_switch_enc_pcm_input_payload_t *)(payloadInfo +
         sizeof(struct apm_module_param_data_t));
    lc3_payload->transition_direction = val;
    ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                     customPayloadSize,
                     lc3_payload,
                     customPayloadSize);

    *size = payloadSize;
    *payload = payloadInfo;
}

void PayloadBuilder::payloadRATConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_media_config *data)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_rat_mf_t *ratConf;
    int numChannel;
    uint32_t bitWidth;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (!data) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }

    numChannel = data->ch_info.channels;
    bitWidth = data->bit_width;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_rat_mf_t) +
                  sizeof(uint16_t)*numChannel;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    ratConf = (struct param_id_rat_mf_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));
    pcmChannel = (uint16_t*)(payloadInfo + sizeof(struct apm_module_param_data_t) +
                                       sizeof(struct param_id_rat_mf_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_RAT_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    ratConf->sample_rate = data->sample_rate;
    if ((bitWidth == BITWIDTH_16) || (bitWidth == BITWIDTH_32)) {
        ratConf->bits_per_sample = bitWidth;
        ratConf->q_factor =  bitWidth - 1;
    } else if (bitWidth == BITWIDTH_24) {
        ratConf->bits_per_sample = BITS_PER_SAMPLE_32;
        ratConf->q_factor = PCM_Q_FACTOR_27;
    }
    ratConf->data_format = DATA_FORMAT_FIXED_POINT;
    ratConf->num_channels = numChannel;
    populateChannelMap(pcmChannel, numChannel);
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "sample_rate:%d bits_per_sample:%d q_factor:%d data_format:%d num_channels:%d",
                      ratConf->sample_rate, ratConf->bits_per_sample, ratConf->q_factor,
                      ratConf->data_format, ratConf->num_channels);
    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadPcmCnvConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_media_config *data, bool isRx)
{
    struct apm_module_param_data_t* header = NULL;
    struct media_format_t *mediaFmtHdr;
    struct payload_pcm_output_format_cfg_t *mediaFmtPayload;
    int numChannels;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t *pcmChannel;

    if (!data) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }

    numChannels = data->ch_info.channels;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct media_format_t) +
                  sizeof(struct payload_pcm_output_format_cfg_t) +
                  sizeof(uint8_t)*numChannels;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return;
    }
    header          = (struct apm_module_param_data_t*)payloadInfo;
    mediaFmtHdr     = (struct media_format_t*)(payloadInfo +
                      sizeof(struct apm_module_param_data_t));
    mediaFmtPayload = (struct payload_pcm_output_format_cfg_t*)(payloadInfo +
                      sizeof(struct apm_module_param_data_t) +
                      sizeof(struct media_format_t));
    pcmChannel      = (uint8_t*)(payloadInfo + sizeof(struct apm_module_param_data_t) +
                      sizeof(struct media_format_t) +
                      sizeof(struct payload_pcm_output_format_cfg_t));

    header->module_instance_id = miid;
    header->param_id           = PARAM_ID_PCM_OUTPUT_FORMAT_CFG;
    header->error_code         = 0x0;
    header->param_size         = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    mediaFmtHdr->data_format  = DATA_FORMAT_FIXED_POINT;
    mediaFmtHdr->fmt_id       = MEDIA_FMT_ID_PCM;
    mediaFmtHdr->payload_size = sizeof(payload_pcm_output_format_cfg_t) +
                                sizeof(uint8_t) * numChannels;
    PAL_DBG(LOG_TAG, "mediaFmtHdr data_format:%x fmt_id:%x payload_size:%d channels:%d",
                      mediaFmtHdr->data_format, mediaFmtHdr->fmt_id,
                      mediaFmtHdr->payload_size, numChannels);

    mediaFmtPayload->endianness      = PCM_LITTLE_ENDIAN;
    mediaFmtPayload->num_channels    = data->ch_info.channels;
    if ((data->bit_width == BITWIDTH_16) || (data->bit_width == BITWIDTH_32)) {
        mediaFmtPayload->bit_width       = data->bit_width;
        mediaFmtPayload->bits_per_sample = data->bit_width;
        mediaFmtPayload->q_factor        = data->bit_width - 1;
        mediaFmtPayload->alignment       = PCM_LSB_ALIGNED;
    } else if (data->bit_width == BITWIDTH_24) {
        // convert to Q31 that's expected by HD encoders.
        mediaFmtPayload->bit_width       = BIT_WIDTH_24;
        mediaFmtPayload->bits_per_sample = BITS_PER_SAMPLE_32;
        mediaFmtPayload->q_factor        = isRx ? PCM_Q_FACTOR_31 : PCM_Q_FACTOR_27;
        mediaFmtPayload->alignment       = PCM_MSB_ALIGNED;
    } else {
        PAL_ERR(LOG_TAG, "invalid bit width %d", data->bit_width);
        free(payloadInfo);
        payloadInfo = NULL;
        *size = 0;
        *payload = NULL;
        return;
    }
    mediaFmtPayload->interleaved = isRx ? PCM_INTERLEAVED : PCM_DEINTERLEAVED_UNPACKED;
    PAL_DBG(LOG_TAG, "interleaved:%d bit_width:%d bits_per_sample:%d q_factor:%d",
                  mediaFmtPayload->interleaved, mediaFmtPayload->bit_width,
                  mediaFmtPayload->bits_per_sample, mediaFmtPayload->q_factor);
    populateChannelMap(pcmChannel, numChannels);
    *size = (payloadSize + padBytes);
    *payload = payloadInfo;

    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadCopPackConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_media_config *data)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_cop_pack_output_media_fmt_t *copPack  = NULL;
    int numChannel;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (!data) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }

    numChannel = data->ch_info.channels;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_cop_pack_output_media_fmt_t) +
                  sizeof(uint16_t)*numChannel;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    copPack = (struct param_id_cop_pack_output_media_fmt_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));
    pcmChannel = (uint16_t*)(payloadInfo +
                          sizeof(struct apm_module_param_data_t) +
                          sizeof(struct param_id_cop_pack_output_media_fmt_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_COP_PACKETIZER_OUTPUT_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    copPack->sampling_rate = data->sample_rate;
    copPack->bits_per_sample = data->bit_width;
    copPack->num_channels = numChannel;
    populateChannelMap(pcmChannel, numChannel);
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "sample_rate:%d bits_per_sample:%d num_channels:%d",
                      copPack->sampling_rate, copPack->bits_per_sample, copPack->num_channels);
    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadScramblingConfig(uint8_t** payload, size_t* size,
        uint32_t miid, uint32_t enable)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_cop_pack_enable_scrambling_t *copPack  = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_cop_pack_enable_scrambling_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    copPack = (struct param_id_cop_pack_enable_scrambling_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_COP_PACKETIZER_ENABLE_SCRAMBLING;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    copPack->enable_scrambler = enable;
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "enable_scrambler:%d", copPack->enable_scrambler);
    PAL_VERBOSE(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadCopV2StreamInfo(uint8_t **payload, size_t *size,
        uint32_t miid, void *codecInfo, bool isStreamMapDirIn)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_cop_v2_stream_info_t *streamInfo = NULL;
    uint8_t* payloadInfo = NULL;
    audio_lc3_codec_cfg_t *bleCfg = NULL;
    struct cop_v2_stream_info_map_t* streamMap = NULL;
    size_t payloadSize = 0, padBytes = 0, streamMapSize = 0;
    uint64_t channel_mask = 0;
    int i = 0;

    bleCfg = (audio_lc3_codec_cfg_t *)codecInfo;
    if (!bleCfg) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }

    if (!isStreamMapDirIn) {
        if (!bleCfg->is_enc_config_set)
            streamMapSize = DEF_STREAM_MAP_SZ;
        else
            streamMapSize = bleCfg->enc_cfg.stream_map_size;
        payloadSize = sizeof(struct apm_module_param_data_t) +
                      sizeof(struct param_id_cop_v2_stream_info_t) +
                      sizeof(struct cop_v2_stream_info_map_t) * streamMapSize;
    } else if (isStreamMapDirIn && bleCfg->dec_cfg.stream_map_size != 0) {
        streamMapSize = bleCfg->dec_cfg.stream_map_size;
        payloadSize = sizeof(struct apm_module_param_data_t) +
                      sizeof(struct param_id_cop_v2_stream_info_t) +
                      sizeof(struct cop_v2_stream_info_map_t) * streamMapSize;
    } else if (isStreamMapDirIn && bleCfg->dec_cfg.stream_map_size == 0) {
        PAL_ERR(LOG_TAG, "isStreamMapDirIn is true, but empty streamMapIn");
        return;
    }

    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    streamInfo = (struct param_id_cop_v2_stream_info_t*)(payloadInfo +
                  sizeof(struct apm_module_param_data_t));
    streamMap = (struct cop_v2_stream_info_map_t*)(payloadInfo +
                 sizeof(struct apm_module_param_data_t) +
                 sizeof(struct param_id_cop_v2_stream_info_t));

    header->module_instance_id = miid;
    header->param_id = ResourceManager::isCPEnabled ? PARAM_ID_CONN_PROXY_STREAM_INFO : PARAM_ID_COP_V2_STREAM_INFO;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    streamInfo->num_streams = streamMapSize;
    if (!isStreamMapDirIn && !bleCfg->is_enc_config_set) {
        for (i = 0; i < streamMapSize; i++) {
            channel_mask = convert_channel_map(def_stream_map_out[i].audio_location);
            streamMap[i].stream_id = def_stream_map_out[i].stream_id;
            streamMap[i].direction = def_stream_map_out[i].direction;
            streamMap[i].channel_mask_lsw = channel_mask  & 0x00000000FFFFFFFF;
            streamMap[i].channel_mask_msw = (channel_mask & 0xFFFFFFFF00000000) >> 32;
        }
    } else if (!isStreamMapDirIn) {
        for (i = 0; i < streamMapSize; i++) {
            channel_mask = convert_channel_map(bleCfg->enc_cfg.streamMapOut[i].audio_location);
            streamMap[i].stream_id = bleCfg->enc_cfg.streamMapOut[i].stream_id;
            streamMap[i].direction = bleCfg->enc_cfg.streamMapOut[i].direction;
            streamMap[i].channel_mask_lsw = channel_mask  & 0x00000000FFFFFFFF;
            streamMap[i].channel_mask_msw = (channel_mask & 0xFFFFFFFF00000000) >> 32;
        }
    } else {
        for (i = 0; i < streamMapSize; i++) {
            channel_mask = convert_channel_map(bleCfg->dec_cfg.streamMapIn[i].audio_location);
            streamMap[i].stream_id = bleCfg->dec_cfg.streamMapIn[i].stream_id;
            streamMap[i].direction = bleCfg->dec_cfg.streamMapIn[i].direction;
            streamMap[i].channel_mask_lsw = channel_mask  & 0x00000000FFFFFFFF;
            streamMap[i].channel_mask_msw = (channel_mask & 0xFFFFFFFF00000000) >> 32;
        }
    }
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo, *size);
}

/* Used for VI feedback device KV as of now */
int PayloadBuilder::getDeviceKV(int dev_id, std::vector<std::pair<int,int>>& deviceKV)
{
    PAL_DBG(LOG_TAG, "Enter: device ID: %d", dev_id);
    std::vector<std::pair<selector_type_t, std::string>> empty_selector_pairs;

    return retrieveKVs(empty_selector_pairs, dev_id, all_devices, deviceKV);
}

/** Used for BT device KVs only */
int PayloadBuilder::getBtDeviceKV(int dev_id, std::vector<std::pair<int,int>>& deviceKV,
    uint32_t codecFormat, bool isAbrEnabled, bool isHostless)
{
    int status = 0;
    PAL_INFO(LOG_TAG, "Enter: codecFormat:0x%x, isabrEnabled:%d, isHostless:%d",
        codecFormat, isAbrEnabled, isHostless);
    std::vector<std::pair<selector_type_t, std::string>> filled_selector_pairs;

    filled_selector_pairs.push_back(std::make_pair(CODECFORMAT_SEL,
                                   btCodecFormatLUT.at(codecFormat)));

    if (dev_id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
        dev_id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
        dev_id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
        filled_selector_pairs.push_back(std::make_pair(ABR_ENABLED_SEL,
            isAbrEnabled ? "TRUE" : "FALSE"));
        filled_selector_pairs.push_back(std::make_pair(HOSTLESS_SEL,
            isHostless ? "TRUE" : "FALSE"));
    } else if (dev_id == PAL_DEVICE_IN_BLUETOOTH_A2DP ||
        dev_id == PAL_DEVICE_IN_BLUETOOTH_BLE) {
        filled_selector_pairs.push_back(std::make_pair(HOSTLESS_SEL,
            isHostless ? "TRUE" : "FALSE"));
    }
    status = retrieveKVs(filled_selector_pairs, dev_id, all_devices, deviceKV);
    PAL_INFO(LOG_TAG, "Exit, status %d", status);
    return status;
}

#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
void PayloadBuilder::payloadSsPackConfig(uint8_t** payload, size_t* size,
        uint32_t miid, struct pal_media_config *data)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_ss_pack_output_media_fmt_t *copPack  = NULL;
    int numChannel;
    uint16_t* pcmChannel = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (!data) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }

    numChannel = data->ch_info.channels;
    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_ss_pack_output_media_fmt_t) +
                  sizeof(uint16_t)*numChannel;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    copPack = (struct param_id_ss_pack_output_media_fmt_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));
    pcmChannel = (uint16_t*)(payloadInfo +
                          sizeof(struct apm_module_param_data_t) +
                          sizeof(struct param_id_ss_pack_output_media_fmt_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_SS_PACKETIZER_OUTPUT_MEDIA_FORMAT;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    copPack->sampling_rate = data->sample_rate;
    copPack->bits_per_sample = data->bit_width;
    copPack->num_channels = numChannel;
    populateChannelMap(pcmChannel, numChannel);
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_INFO(LOG_TAG, "sample_rate:%d bits_per_sample:%d num_channels:%d",
                      copPack->sampling_rate, copPack->bits_per_sample, copPack->num_channels);
    PAL_INFO(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadSsPackEncConfig(uint8_t** payload, size_t* size,
        uint32_t miid, uint16_t enc_format)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_ss_pack_enc_format_t *copPack  = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_ss_pack_enc_format_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    copPack = (struct param_id_ss_pack_enc_format_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_ENC_FORMAT_ID;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_INFO(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);
    copPack->enc_format = enc_format;
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_INFO(LOG_TAG, "enc_format : %d", copPack->enc_format);
    PAL_INFO(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadSsPackSuspendConfig(uint8_t** payload, size_t* size,
        uint32_t miid, uint16_t a2dp_suspend)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_ss_pack_a2dp_suspend_t *copPack  = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_ss_pack_a2dp_suspend_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    copPack = (struct param_id_ss_pack_a2dp_suspend_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_A2DP_SUSPEND_ID;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_INFO(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);
    copPack->a2dp_suspend = a2dp_suspend;
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_INFO(LOG_TAG, "a2dp_suspend : %d", copPack->a2dp_suspend);
    PAL_INFO(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadSsPackMtuConfig(uint8_t** payload, size_t* size,
        uint32_t miid, uint16_t mtu)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_ss_pack_mtu_t *copPack  = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_ss_pack_mtu_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    copPack = (struct param_id_ss_pack_mtu_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_PEER_MTU_ID;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_INFO(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);
    copPack->mtu = mtu;
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_INFO(LOG_TAG, "mtu : %d", copPack->mtu);
    PAL_INFO(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadSsPackBitrate(uint8_t** payload, size_t* size,
        uint32_t miid, int32_t bitrate)
{
    struct apm_module_param_data_t* header = NULL;
    struct param_id_enc_bitrate_param_t *dynBitratePack  = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct param_id_enc_bitrate_param_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    dynBitratePack = (struct param_id_enc_bitrate_param_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = PARAM_ID_ENC_BITRATE;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);
    dynBitratePack->bitrate = bitrate;
    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "bitrate : %d", dynBitratePack->bitrate);
    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}

void PayloadBuilder::payloadSsPackSbmConfig(uint8_t** payload, size_t* size,
        uint32_t miid, pal_param_bta2dp_sbm_t *param_bt_a2dp_sbm)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    struct sbm_cfg_t *sbm_cfg;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                  sizeof(struct sbm_cfg_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo alloc failed %s", strerror(errno));
        return;
    }
    header = (struct apm_module_param_data_t*)payloadInfo;
    sbm_cfg = (struct sbm_cfg_t*)(payloadInfo +
               sizeof(struct apm_module_param_data_t));

    header->module_instance_id = miid;
    header->param_id = MEDIA_ID_SBM_ID;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    PAL_DBG(LOG_TAG, "header params \n IID:%x param_id:%x error_code:%d param_size:%d",
                      header->module_instance_id, header->param_id,
                      header->error_code, header->param_size);

    memset(sbm_cfg, 0x0, sizeof(struct sbm_cfg_t));
    sbm_cfg->prev_delay = (param_bt_a2dp_sbm->firstParam >> 16) & 0x0000FFFF;
    sbm_cfg->target_delay = (param_bt_a2dp_sbm->firstParam & 0x0000FFFF);
    sbm_cfg->current_buffer_level = (param_bt_a2dp_sbm->secondParam >> 16) & 0x0000FFFF;
    sbm_cfg->stop_flag = (param_bt_a2dp_sbm->secondParam & 0x0000FFFF);

    *size = payloadSize + padBytes;
    *payload = payloadInfo;
    PAL_DBG(LOG_TAG, "SBM prev_delay : %d, target_delay = %d, current_buffer_level = %d, stop_flag = %d",
            sbm_cfg->prev_delay, sbm_cfg->target_delay, sbm_cfg->current_buffer_level, sbm_cfg->stop_flag);
    PAL_DBG(LOG_TAG, "customPayload address %pK and size %zu", payloadInfo,
                *size);
}
#endif

/** Used for Loopback stream types only */
int PayloadBuilder::populateStreamKV(Stream* s, std::vector<std::pair<int,int>> &keyVectorRx,
        std::vector<std::pair<int,int>> &keyVectorTx, struct vsid_info vsidinfo)
{
    int status = 0;
    struct pal_stream_attributes *sattr = NULL;
    std::vector<std::string> selector_names;
    std::vector<std::pair<selector_type_t, std::string>> filled_selector_pairs;


    PAL_DBG(LOG_TAG, "Enter");
    sattr = new struct pal_stream_attributes();
    if (!sattr) {
        PAL_ERR(LOG_TAG, "sattr alloc failed %s", strerror(errno));
        status = -ENOMEM;
        goto exit;
    }
    memset(sattr, 0, sizeof(struct pal_stream_attributes));
    status = s->getStreamAttributes(sattr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes failed status %d", status);
        goto free_sattr;
    }

    PAL_INFO(LOG_TAG, "stream type %d", sattr->type);
    if (sattr->type == PAL_STREAM_LOOPBACK) {
        if (sattr->info.opt_stream_info.loopback_type == PAL_STREAM_LOOPBACK_HFP_RX) {
            filled_selector_pairs.push_back(std::make_pair(DIRECTION_SEL, "RX"));
            filled_selector_pairs.push_back(std::make_pair(SUB_TYPE_SEL,
                loopbackLUT.at(sattr->info.opt_stream_info.loopback_type)));
            retrieveKVs(filled_selector_pairs, sattr->type, all_streams, keyVectorRx);

            filled_selector_pairs.clear();
            filled_selector_pairs.push_back(std::make_pair(DIRECTION_SEL, "TX"));
            filled_selector_pairs.push_back(std::make_pair(SUB_TYPE_SEL,
                loopbackLUT.at(sattr->info.opt_stream_info.loopback_type)));
            retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVectorTx);
        } else if (sattr->info.opt_stream_info.loopback_type == PAL_STREAM_LOOPBACK_HFP_TX) {
           /* no StreamKV for HFP TX */
        } else {
            selector_names = retrieveSelectors(sattr->type, all_streams);
            if (selector_names.empty() != true)
               filled_selector_pairs = getSelectorValues(selector_names, s, NULL);
            retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVectorRx);
        }
    } else if (sattr->type == PAL_STREAM_VOICE_CALL) {
        filled_selector_pairs.push_back(std::make_pair(DIRECTION_SEL, "RX"));
        filled_selector_pairs.push_back(std::make_pair(VSID_SEL,
            vsidLUT.at(sattr->info.voice_call_info.VSID)));
        retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVectorRx);

        filled_selector_pairs.clear();
        filled_selector_pairs.push_back(std::make_pair(DIRECTION_SEL, "TX"));
        filled_selector_pairs.push_back(std::make_pair(VSID_SEL,
            vsidLUT.at(sattr->info.voice_call_info.VSID)));
        retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVectorTx);
    } else if (sattr->type == PAL_STREAM_ULTRASOUND) {
        filled_selector_pairs.push_back(std::make_pair(DIRECTION_SEL, "RX"));
        retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVectorRx);

        filled_selector_pairs.clear();
        filled_selector_pairs.push_back(std::make_pair(DIRECTION_SEL, "TX"));
        retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVectorTx);
    } else {
        PAL_DBG(LOG_TAG, "KVs not provided for stream type:%d", sattr->type);
    }
free_sattr:
    delete sattr;
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

/** Used for Loopback stream types only */
int PayloadBuilder::populateStreamPPKV(Stream* s, std::vector <std::pair<int,int>> &keyVectorRx,
        std::vector <std::pair<int,int>> &keyVectorTx __unused)
{
    int status = 0;
    struct pal_stream_attributes *sattr = NULL;
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;

    PAL_DBG(LOG_TAG, "Enter");
    sattr = new struct pal_stream_attributes();
    if (!sattr) {
        PAL_ERR(LOG_TAG, "sattr alloc failed %s", strerror(errno));
        status = -ENOMEM;
        goto exit;
    }
    memset(sattr, 0, sizeof(struct pal_stream_attributes));
    status = s->getStreamAttributes(sattr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed status %d", status);
        goto free_sattr;
    }

    PAL_INFO(LOG_TAG, "stream type %d", sattr->type);

    if (sattr->type == PAL_STREAM_VOICE_CALL) {
        selectors = retrieveSelectors(sattr->type, all_streampps);
        if (selectors.empty() != true)
            filled_selector_pairs = getSelectorValues(selectors, s, NULL);
        retrieveKVs(filled_selector_pairs ,sattr->type, all_streampps, keyVectorRx);
    } else {
        PAL_DBG(LOG_TAG, "KVs not provided for stream type:%d", sattr->type);
    }

free_sattr:
    delete sattr;
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

bool PayloadBuilder::compareSelectorPairs(
    std::vector<std::pair<selector_type_t, std::string>>& selector_pairs,
    std::vector<std::pair<selector_type_t, std::string>>& filled_selector_pairs)
{
    int count = 0;
    bool result = false;

    PAL_DBG(LOG_TAG, "Enter: selector size: %zu filled_sel size: %zu",
        selector_pairs.size(), filled_selector_pairs.size());
    if (selector_pairs.size() == filled_selector_pairs.size()) {
        std::sort(filled_selector_pairs.begin(), filled_selector_pairs.end());
        std::sort(selector_pairs.begin(), selector_pairs.end());
        result = std::equal(selector_pairs.begin(), selector_pairs.end(),
            filled_selector_pairs.begin());
        if (result) {
            PAL_DBG(LOG_TAG,"Return True");
            goto exit;
        }
    } else {
        for (int i = 0; i < filled_selector_pairs.size(); i++) {
            if (selector_pairs.end() != std::find(selector_pairs.begin(),
                selector_pairs.end(), filled_selector_pairs[i])) {
                count++;
                PAL_DBG(LOG_TAG,"Inside the find loop count=%d", count);
            }
        }
        PAL_DBG(LOG_TAG, "After find count:%d", count);
        if (filled_selector_pairs.size() == count) {
            result = true;
            PAL_DBG(LOG_TAG,"Return True");
            goto exit;
        }
    }
exit:
    PAL_DBG(LOG_TAG, "Exit result: %d", result);
    return result;
}

bool PayloadBuilder::findKVs(std::vector<std::pair<selector_type_t, std::string>>
    &filled_selector_pairs, uint32_t type, std::vector<allKVs> &any_type,
    std::vector<std::pair<int, int>> &keyVector)
{
    bool found = false;

    for (int32_t i = 0; i < any_type.size(); i++) {
        if (isIdTypeAvailable(type, any_type[i].id_type)) {
            for (int32_t j = 0; j < any_type[i].keys_values.size(); j++) {
                if (filled_selector_pairs.empty() != true) {
                    if (compareSelectorPairs(any_type[i].keys_values[j].selector_pairs,
                           filled_selector_pairs)) {
                        for (int32_t k = 0; k < any_type[i].keys_values[j].kv_pairs.size(); k++) {
                            keyVector.push_back(
                                std::make_pair(any_type[i].keys_values[j].kv_pairs[k].key,
                                any_type[i].keys_values[j].kv_pairs[k].value));
                            PAL_INFO(LOG_TAG, "key: 0x%x value: 0x%x\n",
                                any_type[i].keys_values[j].kv_pairs[k].key,
                                any_type[i].keys_values[j].kv_pairs[k].value);
                        }
                        found = true;
                        break;
                    }
                } else {
                    if (any_type[i].keys_values[j].selector_pairs.empty()) {
                        for (int32_t k = 0; k < any_type[i].keys_values[j].kv_pairs.size(); k++) {
                            keyVector.push_back(
                                std::make_pair(any_type[i].keys_values[j].kv_pairs[k].key,
                                any_type[i].keys_values[j].kv_pairs[k].value));
                            PAL_INFO(LOG_TAG, "key: 0x%x value: 0x%x\n",
                                any_type[i].keys_values[j].kv_pairs[k].key,
                                any_type[i].keys_values[j].kv_pairs[k].value);
                        }
                        found = true;
                        break;
                    }
                }
            }
        }
    }
    return found;
}

int PayloadBuilder::retrieveKVs(std::vector<std::pair<selector_type_t, std::string>>
    &filled_selector_pairs, uint32_t type, std::vector<allKVs> &any_type,
    std::vector<std::pair<int, int>> &keyVector)
{
    bool found = false, custom_config_fallback = false;
    int status = 0;

    PAL_DBG(LOG_TAG, "Enter");

    found = findKVs(filled_selector_pairs, type, any_type, keyVector);
    if (found) {
        PAL_DBG(LOG_TAG, "KVs found for the stream type/dev id: %d", type);
        goto exit;
    } else {
        /* Add a fallback approach to search for KVs again without custom config as selector */
        for (int i = 0; i < filled_selector_pairs.size(); i++) {
            if (filled_selector_pairs[i].first == CUSTOM_CONFIG_SEL) {
                PAL_INFO(LOG_TAG, "Fallback to find KVs without custom config %s",
                    filled_selector_pairs[i].second.c_str());
                filled_selector_pairs.erase(filled_selector_pairs.begin() + i);
                custom_config_fallback = true;
            }
        }
        if (custom_config_fallback) {
            found = findKVs(filled_selector_pairs, type, any_type, keyVector);
            if (found) {
                PAL_DBG(LOG_TAG, "KVs found without custom config for the stream type/dev id: %d",
                    type);
                goto exit;
            }
        }
        if (!found)
            PAL_INFO(LOG_TAG, "No KVs found for the stream type/dev id: %d", type);
    }
    status = -EINVAL;

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

std::vector<std::pair<selector_type_t, std::string>> PayloadBuilder::getSelectorValues(
    std::vector<std::string> &selector_names, Stream* s, struct pal_device* dAttr)
{
    int instance_id = 0;
    int status = 0;
    struct pal_stream_attributes *sattr = NULL;
    std::stringstream st;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::vector<std::pair<selector_type_t, std::string>> filled_selector_pairs;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    PAL_DBG(LOG_TAG, "Enter");
    sattr = new struct pal_stream_attributes();
    if (!sattr) {
        PAL_ERR(LOG_TAG, "sattr alloc failed %s", strerror(errno));
        goto exit;
    }
    memset(sattr, 0, sizeof(struct pal_stream_attributes));

    if (!s) {
        PAL_ERR(LOG_TAG, "stream is NULL");
        filled_selector_pairs.clear();
        goto free_sattr;
    }

    status = s->getStreamAttributes(sattr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes failed status %d", status);
        goto free_sattr;
    }

    for (int i = 0; i < selector_names.size(); i++) {
        PAL_DBG(LOG_TAG, "selectors_strings :%s", selector_names[i].c_str());
        selector_type_t selector_type =  selectorstypeLUT.at(selector_names[i]);
        switch (selector_type) {
            case DIRECTION_SEL:
                if (sattr->direction == PAL_AUDIO_OUTPUT)
                    filled_selector_pairs.push_back(std::make_pair(selector_type, "RX"));
                else if (sattr->direction == PAL_AUDIO_INPUT)
                    filled_selector_pairs.push_back(std::make_pair(selector_type, "TX"));
                else if (sattr->direction == PAL_AUDIO_INPUT_OUTPUT)
                    filled_selector_pairs.push_back(std::make_pair(selector_type, "RX_TX"));
                else
                    PAL_ERR(LOG_TAG, "Invalid stream direction %d", sattr->direction);
                PAL_INFO(LOG_TAG, "Direction: %d", sattr->direction);
            break;
            case BITWIDTH_SEL:
                /* If any usecase defined with bitwidth,need to update */
            break;
            case INSTANCE_SEL:
                if (sattr->type == PAL_STREAM_VOICE_UI)
                    instance_id = dynamic_cast<StreamSoundTrigger *>(s)->GetInstanceId();
                else
                    instance_id = rm->getStreamInstanceID(s);
                if (instance_id < INSTANCE_1) {
                    PAL_ERR(LOG_TAG, "Invalid instance id %d", instance_id);
                    goto free_sattr;
                }
                st << instance_id;
                filled_selector_pairs.push_back(std::make_pair(selector_type, st.str()));
                PAL_INFO(LOG_TAG, "Instance: %d", instance_id);
                break;
            case SUB_TYPE_SEL:
                if (sattr->type == PAL_STREAM_PROXY) {
                    if (sattr->direction == PAL_AUDIO_INPUT) {
                        if (sattr->info.opt_stream_info.tx_proxy_type == PAL_STREAM_PROXY_TX_WFD)
                            filled_selector_pairs.push_back(std::make_pair(selector_type,
                                "PAL_STREAM_PROXY_TX_WFD"));
                        else if (sattr->info.opt_stream_info.tx_proxy_type == PAL_STREAM_PROXY_TX_TELEPHONY_RX)
                            filled_selector_pairs.push_back(std::make_pair(selector_type,
                                "PAL_STREAM_PROXY_TX_TELEPHONY_RX"));
                        PAL_INFO(LOG_TAG, "TX Proxy type = %d",
                            sattr->info.opt_stream_info.tx_proxy_type);
                    } else if (sattr->direction == PAL_AUDIO_OUTPUT) {
                        if (sattr->info.opt_stream_info.rx_proxy_type == PAL_STREAM_PROXY_RX_WFD)
                            filled_selector_pairs.push_back(std::make_pair(selector_type,
                                "PAL_STREAM_PROXY_RX_WFD"));
                        PAL_INFO(LOG_TAG, "RX Proxy type = %d",
                            sattr->info.opt_stream_info.rx_proxy_type);
                    }
                } else if (sattr->type == PAL_STREAM_LOOPBACK) {
                    filled_selector_pairs.push_back(std::make_pair(selector_type,
                        loopbackLUT.at(sattr->info.opt_stream_info.loopback_type)));
                    PAL_INFO(LOG_TAG, "Loopback type: %d",
                        sattr->info.opt_stream_info.loopback_type);
                }
                break;
            case VUI_MODULE_TYPE_SEL:
                if (!s) {
                    PAL_ERR(LOG_TAG, "Invalid stream");
                    goto free_sattr;
                }

                if (s->getStreamSelector().length() != 0)
                    filled_selector_pairs.push_back(std::make_pair(selector_type,
                        s->getStreamSelector()));

                PAL_INFO(LOG_TAG, "VUI module type:%s", s->getStreamSelector().c_str());
                break;
            case ACD_MODULE_TYPE_SEL:
                if (!s) {
                    PAL_ERR(LOG_TAG, "Invalid stream");
                    goto free_sattr;
                }

                if (s->getStreamSelector().length() != 0)
                    filled_selector_pairs.push_back(std::make_pair(selector_type,
                        s->getStreamSelector()));

                PAL_INFO(LOG_TAG, "ACD module type:%s", s->getStreamSelector().c_str());
                break;
            case DEVICEPP_TYPE_SEL:
                if (!s) {
                    PAL_ERR(LOG_TAG, "Invalid stream");
                    goto free_sattr;
                }

                if (s->getDevicePPSelector().length() != 0)
                    filled_selector_pairs.push_back(std::make_pair(selector_type,
                        s->getDevicePPSelector()));

                PAL_INFO(LOG_TAG, "devicePP_type:%s", s->getDevicePPSelector().c_str());
                break;
            case STREAM_TYPE_SEL:
                filled_selector_pairs.push_back(std::make_pair(selector_type,
                    streamNameLUT.at(sattr->type)));
                PAL_INFO(LOG_TAG, "stream type: %d", sattr->type);
                break;
            case AUD_FMT_SEL:
                if (isPalPCMFormat(sattr->out_media_config.aud_fmt_id)) {
                   filled_selector_pairs.push_back(std::make_pair(AUD_FMT_SEL,
                        "PAL_AUDIO_FMT_PCM"));
                } else {
                   filled_selector_pairs.push_back(std::make_pair(AUD_FMT_SEL,
                        "PAL_AUDIO_FMT_NON_PCM"));
                }
                PAL_INFO(LOG_TAG, "audio format: %d",
                     sattr->out_media_config.aud_fmt_id);
                break;
            case CUSTOM_CONFIG_SEL:
                if (dAttr && strlen(dAttr->custom_config.custom_key)) {
                    filled_selector_pairs.push_back(
                        std::make_pair(CUSTOM_CONFIG_SEL,
                        dAttr->custom_config.custom_key));
                    PAL_INFO(LOG_TAG, "custom config key:%s",
                        dAttr->custom_config.custom_key);
                }
                break;
            default:
                PAL_DBG(LOG_TAG, "Selector type %d not handled", selector_type);
                break;
        }
    }
free_sattr:
    delete sattr;
exit:
    PAL_DBG(LOG_TAG, "Exit");
    return filled_selector_pairs;
}

void PayloadBuilder::removeDuplicateSelectors(std::vector<std::string> &gkv_selectors)
{
    auto end = gkv_selectors.end();
    for (auto i = gkv_selectors.begin(); i != end; ++i) {
        end = std::remove(i + 1, end, *i);
    }
    gkv_selectors.erase(end, gkv_selectors.end());
}

bool PayloadBuilder::isIdTypeAvailable(int32_t type, std::vector<int>& id_type)
{
    for (int32_t i = 0; i < id_type.size(); i++) {
       if (type == id_type[i]){
           PAL_DBG(LOG_TAG,"idtype :%d passed type :%d", id_type[i], type);
           return true;
       }
    }
    return false;
}

std::vector<std::string> PayloadBuilder::retrieveSelectors(int32_t type, std::vector<allKVs> &any_type)
{
    std::vector<std::string> gkv_selectors;
    PAL_DBG(LOG_TAG, "Enter: size_of_all :%zu type:%d", any_type.size(), type);

    /* looping for all keys_and_values selectors and store in the gkv_selectors */
    for (int32_t i = 0; i < any_type.size(); i++) {
         if (isIdTypeAvailable(type, any_type[i].id_type)) {
             PAL_DBG(LOG_TAG, "KeysAndValues_size: %zu", any_type[i].keys_values.size());
             for(int32_t j = 0; j < any_type[i].keys_values.size(); j++) {
                 for(int32_t k = 0; k < any_type[i].keys_values[j].selector_names.size(); k++) {
                     gkv_selectors.push_back(any_type[i].keys_values[j].selector_names[k]);
                 }
             }
         }
    }

    if (gkv_selectors.size())
          removeDuplicateSelectors(gkv_selectors);

    for (int32_t i = 0; i < gkv_selectors.size(); i++) {
         PAL_DBG(LOG_TAG, "gkv_selectors: %s", gkv_selectors[i].c_str());
    }
    return gkv_selectors;
}

int PayloadBuilder::populateStreamKV(Stream* s,
        std::vector <std::pair<int,int>> &keyVector)
{
    int status = -EINVAL;
    struct pal_stream_attributes *sattr = NULL;
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;

    PAL_DBG(LOG_TAG, "enter");
    sattr = new struct pal_stream_attributes;
    if (!sattr) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG,"sattr malloc failed %s status %d", strerror(errno), status);
        goto exit;
    }
    memset(sattr, 0, sizeof(struct pal_stream_attributes));

    status = s->getStreamAttributes(sattr);
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed status %d", status);
        goto free_sattr;
    }
    PAL_INFO(LOG_TAG, "stream type %d", sattr->type);
    selectors = retrieveSelectors(sattr->type, all_streams);
    if (selectors.empty() != true)
        filled_selector_pairs = getSelectorValues(selectors, s, NULL);

    if (sattr->type == PAL_STREAM_VOICE_CALL_MUSIC) {
        PAL_DBG(LOG_TAG, "ICMD + playback usecase is %d", sattr->info.incall_music_info.local_playback);
        if (sattr->info.incall_music_info.local_playback) {
            filled_selector_pairs.push_back(
                std::make_pair(CUSTOM_CONFIG_SEL,
                "icmd_plus"));
            PAL_INFO(LOG_TAG, "ICMD + playback usecase");
        }
    }

    retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVector);

free_sattr:
    delete sattr;
exit:
    return status;
}

int PayloadBuilder::populateStreamKVTunnel(Stream* s,
        std::vector <std::pair<int,int>> &keyVector, uint32_t instanceId)
{
    int status = -EINVAL;
    struct pal_stream_attributes *sattr = NULL;
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;
    std::stringstream st;

    PAL_DBG(LOG_TAG, "enter");
    sattr = new struct pal_stream_attributes;
    if (!sattr) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG,"sattr malloc failed %s status %d", strerror(errno), status);
        goto exit;
    }
    memset(sattr, 0, sizeof(struct pal_stream_attributes));

    status = s->getStreamAttributes(sattr);
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed status %d", status);
        goto free_sattr;
    }
    PAL_INFO(LOG_TAG, "stream type %d", sattr->type);
    selectors = retrieveSelectors(sattr->type, all_streams);

    for (int i = 0; i < selectors.size(); i++) {
        selector_type_t selector_type =  selectorstypeLUT.at(selectors[i]);
        PAL_DBG(LOG_TAG, "selector name is %s type is 0x%x",
                            selectors[i].c_str(), selector_type);
        // it avoids instance request.
        if (selector_type == INSTANCE_SEL) {
            selectors.erase(selectors.begin() + i);
        }
    }

    if (selectors.empty() != true)
        filled_selector_pairs = getSelectorValues(selectors, s, NULL);

    st << instanceId;
    filled_selector_pairs.push_back(std::make_pair(INSTANCE_SEL, st.str()));

    retrieveKVs(filled_selector_pairs ,sattr->type, all_streams, keyVector);

free_sattr:
    delete sattr;
exit:
    return status;
}

int PayloadBuilder::populateStreamDeviceKV(Stream* s __unused, int32_t beDevId __unused,
        std::vector <std::pair<int,int>> &keyVector __unused)
{
    int status = 0;

    return status;
}

int PayloadBuilder::populateStreamDeviceKV(Stream* s, int32_t rxBeDevId,
        std::vector <std::pair<int,int>> &keyVectorRx, int32_t txBeDevId,
        std::vector <std::pair<int,int>> &keyVectorTx, struct vsid_info vsidinfo,
                                           sidetone_mode_t sidetoneMode)
{
    int status = 0;
    std::vector <std::pair<int, int>> emptyKV;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    PAL_VERBOSE(LOG_TAG,"Enter");
    if (rm->isOutputDevId(rxBeDevId)) {
        status = populateStreamKV(s, keyVectorRx, emptyKV, vsidinfo);
        if (status)
            goto exit;
    }
    if (rm->isInputDevId(txBeDevId)) {
        status = populateStreamKV(s, emptyKV, keyVectorTx, vsidinfo);
        if (status)
            goto exit;
    }

    status = populateDeviceKV(s, rxBeDevId, keyVectorRx, txBeDevId,
            keyVectorTx, sidetoneMode);

exit:
    PAL_VERBOSE(LOG_TAG,"Exit, status %d", status);
    return status;
}

int PayloadBuilder::populateDeviceKV(Stream* s, int32_t beDevId,
        std::vector <std::pair<int,int>> &keyVector)
{
    int status = 0;
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;
    struct pal_device dAttr;
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    uint32_t soundCardId = 0;

    if (s)
        soundCardId = s->getSoundCardId();

    PAL_INFO(LOG_TAG, "Enter device id:%d", beDevId);

    /* For BT devices, device KV will be populated from Bluetooth device only so skip here */
    if (rm->isBtDevice((pal_device_id_t)beDevId)) {
        if (DUMMY_SND_CARD == soundCardId) {
            PAL_INFO(LOG_TAG, "Use default value for BT ACDB case.");
            keyVector.push_back(std::make_pair(DEVICERX, BT_RX));
            keyVector.push_back(std::make_pair(BT_PROFILE, A2DP));
            keyVector.push_back(std::make_pair(BT_FORMAT, GENERIC));
        }
        goto exit;
    }


    if (beDevId > 0) {
        memset (&dAttr, 0, sizeof(struct pal_device));
        dAttr.id = (pal_device_id_t)beDevId;
        dev = Device::getInstance(&dAttr, rm);
        if (dev) {
            status = dev->getDeviceAttributes(&dAttr, s);
            selectors = retrieveSelectors(beDevId, all_devices);
            if (selectors.empty() != true)
                filled_selector_pairs = getSelectorValues(selectors, s, &dAttr);
            retrieveKVs(filled_selector_pairs, beDevId, all_devices, keyVector);
        }
    }

exit:
    PAL_INFO(LOG_TAG, "Exit device id:%d, status %d", beDevId, status);
    return status;
}

int PayloadBuilder::populateDeviceKV(Stream* s, int32_t rxBeDevId,
        std::vector <std::pair<int,int>> &keyVectorRx, int32_t txBeDevId,
        std::vector <std::pair<int,int>> &keyVectorTx, sidetone_mode_t sidetoneMode)
{
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;

    PAL_DBG(LOG_TAG, "Enter");

    memset(&sAttr, 0, sizeof(struct pal_stream_attributes));
    if (s) {
        status = s->getStreamAttributes(&sAttr);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
            return status;
        }
    }

    populateDeviceKV(s, rxBeDevId, keyVectorRx);
    populateDeviceKV(s, txBeDevId, keyVectorTx);

    /* add sidetone kv if needed */
    if (sAttr.type == PAL_STREAM_VOICE_CALL && sidetoneMode == SIDETONE_SW) {
        PAL_DBG(LOG_TAG, "SW sidetone mode push kv");
        filled_selector_pairs.push_back(std::make_pair(SIDETONE_MODE_SEL, "SW"));
        retrieveKVs(filled_selector_pairs, txBeDevId, all_devices, keyVectorTx);
    }

    PAL_DBG(LOG_TAG, "Exit, status %d", status);

    return status;
}


int PayloadBuilder::populateDeviceKVTunnel(Stream* s, int32_t beDevId,
        std::vector <std::pair<int,int>> &keyVector)
{
    int status = 0;
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;
    struct pal_device dAttr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    /* For BT devices, device KV will be populated from Bluetooth device only so skip here */
    if (rm->isBtDevice((pal_device_id_t)beDevId)) {
        PAL_INFO(LOG_TAG, "Use default value for BT ACDB case.");
        keyVector.push_back(std::make_pair(DEVICERX, BT_RX));
        keyVector.push_back(std::make_pair(BT_PROFILE, A2DP));
        keyVector.push_back(std::make_pair(BT_FORMAT, GENERIC));
        return status;
    }


    if (beDevId > 0) {
        memset (&dAttr, 0, sizeof(struct pal_device));
        dAttr.id = (pal_device_id_t)beDevId;
        selectors = retrieveSelectors(beDevId, all_devices);
        if (selectors.empty() != true)
            filled_selector_pairs = getSelectorValues(selectors, s, &dAttr);
        retrieveKVs(filled_selector_pairs, beDevId, all_devices, keyVector);
    }

    PAL_INFO(LOG_TAG, "Exit device id:%d, status %d", beDevId, status);
    return status;
}


int PayloadBuilder::populateDevicePPKVTunnel(Stream* s, int32_t rxBeDevId,
        std::vector <std::pair<int,int>> &keyVectorRx)
{
    int status = 0;
    struct pal_device dAttr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;

    /* Populate Rx Device PP KV */
    if (rxBeDevId > 0) {
        PAL_INFO(LOG_TAG, "Rx device id:%d", rxBeDevId);
        memset (&dAttr, 0, sizeof(struct pal_device));
        dAttr.id = (pal_device_id_t)rxBeDevId;

        selectors = retrieveSelectors(dAttr.id, all_devicepps);
        if (selectors.empty() != true)
            filled_selector_pairs = getSelectorValues(selectors, s, &dAttr);

        retrieveKVs(filled_selector_pairs, rxBeDevId, all_devicepps,
            keyVectorRx);
    }

    PAL_DBG(LOG_TAG, "Exit, status: %d", status);
    return 0;
}

int PayloadBuilder::populateDevicePPKV(Stream* s, int32_t rxBeDevId,
        std::vector <std::pair<int,int>> &keyVectorRx, int32_t txBeDevId,
        std::vector <std::pair<int,int>> &keyVectorTx)
{
    int status = 0;
    struct pal_device dAttr;
    std::shared_ptr<Device> dev = nullptr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    std::vector <std::string> selectors;
    std::vector <std::pair<selector_type_t, std::string>> filled_selector_pairs;

    PAL_DBG(LOG_TAG, "Enter");
    /* Populate Rx Device PP KV */
    if (rxBeDevId > 0) {
        PAL_INFO(LOG_TAG, "Rx device id:%d", rxBeDevId);
        memset (&dAttr, 0, sizeof(struct pal_device));
        dAttr.id = (pal_device_id_t)rxBeDevId;
        dev = Device::getInstance(&dAttr, rm);
        if (dev) {
            status = dev->getDeviceAttributes(&dAttr, s);
            selectors = retrieveSelectors(dAttr.id, all_devicepps);
            if (selectors.empty() != true)
                filled_selector_pairs = getSelectorValues(selectors, s, &dAttr);
            retrieveKVs(filled_selector_pairs, rxBeDevId, all_devicepps,
                keyVectorRx);
        }
    }

    filled_selector_pairs.clear();
    selectors.clear();

    /* Populate Tx Device PP KV */
    if (txBeDevId > 0) {
        PAL_INFO(LOG_TAG, "Tx device id:%d", txBeDevId);
        memset (&dAttr, 0, sizeof(struct pal_device));
        dAttr.id = (pal_device_id_t)txBeDevId;
        dev = Device::getInstance(&dAttr, rm);
        if (dev) {
            status = dev->getDeviceAttributes(&dAttr, s);
            selectors = retrieveSelectors(dAttr.id, all_devicepps);
            if (selectors.empty() != true)
                filled_selector_pairs = getSelectorValues(selectors, s, &dAttr);
            retrieveKVs(filled_selector_pairs, txBeDevId, all_devicepps,
                keyVectorTx);
        }
    }
    PAL_DBG(LOG_TAG, "Exit, status: %d", status);
    return 0;
}

int PayloadBuilder::populateStreamCkv(Stream *s,
        std::vector <std::pair<int,int>> &keyVector,
        int tag __unused,
        struct pal_volume_data **volume_data __unused)
{
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    struct volume_set_param_info vol_set_param_info;

    PAL_DBG(LOG_TAG, "Enter");
    memset(&sAttr, 0, sizeof(struct pal_stream_attributes));

    status = s->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes failed status %d", status);
        goto exit;
    }

    switch (sAttr.type) {
        case PAL_STREAM_VOICE_UI:
            PAL_INFO(LOG_TAG, "stream channels %d",
                sAttr.in_media_config.ch_info.channels);
            /* Push stream channels CKV for SVA/PDK module calibration */
            keyVector.push_back(std::make_pair(STREAM_CHANNELS,
                sAttr.in_media_config.ch_info.channels));
            break;
        default:
            /*
             * Sending volume minimum as we want to ramp up instead of ramping
             * down while setting the desired volume. Thus avoiding glitch
             * TODO: Decide what to send as ckv in graph open
             */
            memset(&vol_set_param_info, 0, sizeof(struct volume_set_param_info));
            rm->getVolumeSetParamInfo(&vol_set_param_info);
            bool isStreamAvail = (find(vol_set_param_info.streams_.begin(),
                    vol_set_param_info.streams_.end(), sAttr.type) !=
                    vol_set_param_info.streams_.end());
            if ((vol_set_param_info.isVolumeUsingSetParam == false) ||
                ((vol_set_param_info.isVolumeUsingSetParam == true) && !isStreamAvail)) {
                keyVector.push_back(std::make_pair(VOLUME,LEVEL_15));
                PAL_DBG(LOG_TAG, "Entered default %x %x", VOLUME, LEVEL_15);
            }
            break;
     }
exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int PayloadBuilder::populateDevicePPCkv(Stream *s, std::vector <std::pair<int,int>> &keyVector)
{
    int status = 0;
    struct pal_stream_attributes *sattr = NULL;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct pal_device dAttr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    struct pal_device_info devInfo = {};

    PAL_DBG(LOG_TAG,"Enter");
    sattr = new struct pal_stream_attributes;
    if (!sattr) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG,"sattr malloc failed %s status %d", strerror(errno), status);
        goto exit;
    }
    memset (&dAttr, 0, sizeof(struct pal_device));
    memset (sattr, 0, sizeof(struct pal_stream_attributes));

    status = s->getStreamAttributes(sattr);
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed status %d\n",status);
        goto free_sattr;
    }
    status = s->getAssociatedDevices(associatedDevices);
    if (0 != status) {
       PAL_ERR(LOG_TAG,"getAssociatedDevices Failed \n");
       goto free_sattr;
    }
    for (int i = 0; i < associatedDevices.size();i++) {
        status = associatedDevices[i]->getDeviceAttributes(&dAttr);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getAssociatedDevices Failed \n");
            goto free_sattr;
        }

        devInfo.isUSBUUIdBasedTuningEnabledFlag = 0;
        rm->getDeviceInfo(dAttr.id, sattr->type, dAttr.custom_config.custom_key, &devInfo);

        switch (sattr->type) {
            case PAL_STREAM_VOICE_UI:
            case PAL_STREAM_ASR:
                PAL_INFO(LOG_TAG,"channels %d, id %d\n",dAttr.config.ch_info.channels, dAttr.id);
                /* Push Channels CKV for FFNS or FFECNS channel based calibration */
                keyVector.push_back(std::make_pair(CHANNELS,
                                                   dAttr.config.ch_info.channels));
                break;
            case PAL_STREAM_ACD:
            case PAL_STREAM_SENSOR_PCM_DATA:
                PAL_DBG(LOG_TAG,"channels %d, id %d\n",dAttr.config.ch_info.channels, dAttr.id);
                /* Push Channels CKV for FFECNS channel based calibration */
                keyVector.push_back(std::make_pair(CHANNELS,
                                                   dAttr.config.ch_info.channels));
                break;
            case PAL_STREAM_VOIP_RX:
            case PAL_STREAM_VOIP_TX:
                if ((devInfo.isUSBUUIdBasedTuningEnabledFlag) &&
                    (USB::isUsbConnected(dAttr.address))) {
                    keyVector.push_back(std::make_pair(USB_VENDOR_ID, USB::getVendorIdCkv()));
                }
#ifdef SEC_AUDIO_CALL_VOIP
                PAL_INFO(LOG_TAG,"sattr->type(%d) VoiP Sample Rate[%d]\n",
                            sattr->type, sattr->in_media_config.sample_rate);
                if (sattr->in_media_config.sample_rate == 8000) {
                    keyVector.push_back(std::make_pair(VOIP_SAMPLE_RATE, VOIP_SR_NB));
                } else if (sattr->in_media_config.sample_rate == 16000) {
                    keyVector.push_back(std::make_pair(VOIP_SAMPLE_RATE, VOIP_SR_WB));
                } else if (sattr->in_media_config.sample_rate == 32000) {
                    keyVector.push_back(std::make_pair(VOIP_SAMPLE_RATE, VOIP_SR_SWB));
                } else if (sattr->in_media_config.sample_rate == 48000) {
                    keyVector.push_back(std::make_pair(VOIP_SAMPLE_RATE, VOIP_SR_FB));
                }

                PAL_INFO(LOG_TAG,"sattr->type(%d) MIC_MODE[Default] : STANDARD_MODE", sattr->type);
                keyVector.push_back(std::make_pair(MIC_MODE, STANDARD_MODE));
#endif
                break;
            case PAL_STREAM_LOW_LATENCY:
            case PAL_STREAM_DEEP_BUFFER:
            case PAL_STREAM_SPATIAL_AUDIO:
#ifdef SEC_AUDIO_SUPPORT_MEDIA_OUTPUT
            case PAL_STREAM_GENERIC:
#endif
            case PAL_STREAM_PCM_OFFLOAD:
            case PAL_STREAM_COMPRESSED:
                if (dAttr.id == PAL_DEVICE_OUT_SPEAKER) {
                    PAL_INFO(LOG_TAG,"SpeakerProt Status[%d], RAS Status[%d]\n",
                            rm->isSpeakerProtectionEnabled, rm->isRasEnabled);
                }
                if (rm->isSpeakerProtectionEnabled == true &&
                    rm->isRasEnabled == true &&
                    dAttr.id == PAL_DEVICE_OUT_SPEAKER) {
                    if (dAttr.config.ch_info.channels == 2) {
                        PAL_INFO(LOG_TAG,"Enabling RAS - device channels[%d]\n",
                                dAttr.config.ch_info.channels);
                        keyVector.push_back(std::make_pair(RAS_SWITCH, RAS_ON));
                    } else {
                        PAL_INFO(LOG_TAG,"Disabling RAS - device channels[%d] \n",
                                dAttr.config.ch_info.channels);
                        keyVector.push_back(std::make_pair(RAS_SWITCH, RAS_OFF));
                    }
                }

                if ((dAttr.id == PAL_DEVICE_OUT_SPEAKER) ||
                    (dAttr.id == PAL_DEVICE_OUT_WIRED_HEADSET) ||
                    (dAttr.id == PAL_DEVICE_OUT_WIRED_HEADPHONE)) {
                    PAL_DBG(LOG_TAG, "Entered default %x %x", GAIN, GAIN_0);
                    keyVector.push_back(std::make_pair(GAIN, GAIN_0));
                }

                if ((devInfo.isUSBUUIdBasedTuningEnabledFlag) &&
                    (USB::isUsbConnected(dAttr.address))) {
                    keyVector.push_back(std::make_pair(USB_VENDOR_ID, USB::getVendorIdCkv()));
                }
                /* TBD: Push Channels for these types once Channels are added */
                //keyVector.push_back(std::make_pair(CHANNELS,
                //                                   dAttr.config.ch_info.channels));
                break;
            default:
                PAL_VERBOSE(LOG_TAG,"stream type %d doesn't support DevicePP CKV ", sattr->type);
                goto free_sattr;
        }
    }
free_sattr:
    delete sattr;
exit:
    PAL_DBG(LOG_TAG,"Exit, status %d", status);
    return status;
}

int PayloadBuilder::populateCalKeyVector(Stream *s, std::vector <std::pair<int,int>> &ckv, int tag) {
    int status = 0;
    PAL_VERBOSE(LOG_TAG,"enter \n");
    std::vector <std::pair<int,int>> keyVector;
    struct pal_stream_attributes sAttr;
    std::shared_ptr<CaptureProfile> cap_prof = nullptr;
    struct pal_device dAttr;
    int level = -1;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    struct pal_volume_data *voldata = NULL;
#ifdef SEC_AUDIO_CALL_VOIP
    struct pal_param_voip_mic_mode *micmode = NULL;
#endif

    long voldB = 0;
    float vol = 0;

    memset(&sAttr, 0, sizeof(struct pal_stream_attributes));
    status = s->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed");
        return status;
    }

    switch (static_cast<uint32_t>(tag)) {
#ifdef SEC_AUDIO_CALL_VOIP
    case TAG_MIC_MODE_CUSTOM:
        micmode = (struct pal_param_voip_mic_mode *)calloc(1, (sizeof(int32_t)));
        if (!micmode) {
            status = -ENOMEM;
            goto exit;
        }

        status = s->getMicMode(micmode);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getMicMode Failed \n");
            goto error_1;
        }

        if (micmode->mic_mode == STANDARD_MODE) {
            ckv.push_back(std::make_pair(MIC_MODE,STANDARD_MODE));
        }
        else if (micmode->mic_mode == VOICE_FOCUS_MODE) {
            ckv.push_back(std::make_pair(MIC_MODE,VOICE_FOCUS_MODE));
        }
        else if (micmode->mic_mode == ALL_SOUND_MODE) {
            ckv.push_back(std::make_pair(MIC_MODE,ALL_SOUND_MODE));
        }
        else if (micmode->mic_mode == TRANSLATION) {
            if (sAttr.type == PAL_STREAM_VOIP_RX) {
                ckv.push_back(std::make_pair(MIC_MODE,STANDARD_MODE));
                PAL_INFO(LOG_TAG,"voip mic_mode STANDARD_MODE \n");
            } else
                ckv.push_back(std::make_pair(MIC_MODE,TRANSLATION));
        }
        if (sAttr.type != PAL_STREAM_VOIP_RX || micmode->mic_mode != TRANSLATION)
            PAL_INFO(LOG_TAG,"voip mic_mode %s \n", micModeLUT.at(micmode->mic_mode).c_str());
        break;
#endif
    case TAG_STREAM_VOLUME:
        voldata = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                          (sizeof(struct pal_channel_vol_kv) * (0xFFFF))));
        if (!voldata) {
            status = -ENOMEM;
            goto exit;
        }

        status = s->getVolumeData(voldata);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getVolumeData Failed \n");
            goto error_1;
        }

        if (voldata->no_of_volpair == 1) {
            vol = (voldata->volume_pair[0].vol);
            PAL_VERBOSE(LOG_TAG,"volume sent:%f \n",(voldata->volume_pair[0].vol));
        } else {
            vol = (voldata->volume_pair[0].vol + voldata->volume_pair[1].vol)/2;
            PAL_VERBOSE(LOG_TAG,"volume sent left:%f , right: %f \n",(voldata->volume_pair[0].vol),
                      (voldata->volume_pair[1].vol));
        }

        /*scaling the volume by PLAYBACK_VOLUME_MAX factor*/
        voldB = (long)((voldata->volume_pair[0].vol) * (PLAYBACK_VOLUME_MAX*1.0));

#ifdef SEC_AUDIO_CALL_VOIP
        if (sAttr.type == PAL_STREAM_VOIP ||
            sAttr.type == PAL_STREAM_VOIP_RX ||
            sAttr.type == PAL_STREAM_VOIP_TX) { // level 8
            if (voldB == 0L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_8));
            }
            else if (voldB <= 1024L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_7));
            }
            else if (voldB <= 2048L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_6));
            }
            else if (voldB <= 3072L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_5));
            }
            else if (voldB <= 4096L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_4));
            }
            else if (voldB <= 5120L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_3));
            }
            else if (voldB <= 6144L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_2));
            }
            else if (voldB <= 7168L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_1));
            }
            else if (voldB <= 8192L) {
                ckv.push_back(std::make_pair(VOLUME,LEVEL_0));
            }
        } else
#endif
        if (voldB == 0L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_15));
        }
        else if (voldB <= 17L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_14));
        }
        else if (voldB <= 38L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_13));
        }
        else if (voldB <= 81L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_12));
        }
        else if (voldB <= 121L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_11));
        }
        else if (voldB <= 193L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_10));
        }
        else if (voldB <= 307L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_9));
        }
        else if (voldB <= 458L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_8));
        }
        else if (voldB <= 728L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_7));
        }
        else if (voldB <= 1157L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_6));
        }
        else if (voldB <= 1551L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_5));
        }
        else if (voldB <= 2185L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_4));
        }
        else if (voldB <= 3078L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_3));
        }
        else if (voldB <= 4129L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_2));
        }
        else if (voldB <= 5816L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_1));
        }
        else if (voldB <= 8192L) {
            ckv.push_back(std::make_pair(VOLUME,LEVEL_0));
        }
        else {
            //Sending LEVEL_0 in default case.
            PAL_INFO(LOG_TAG, "Setting default volume ckv as LEVEL_0");
            ckv.push_back(std::make_pair(VOLUME,LEVEL_0));
        }
        break;
    case TAG_DEVICE_PP_MBDRC:
        level = s->getGainLevel();
        if (level != -1) {
            ckv.push_back(std::make_pair(GAIN, level));
        } else {
            //Sending GAIN_0 in default case.
            PAL_INFO(LOG_TAG, "Setting default gain ckv as GAIN_0");
            ckv.push_back(std::make_pair(GAIN, GAIN_0));
        }
        break;
    case HANDSET_PROT_ENABLE:
         PAL_DBG(LOG_TAG, "Handset Mono channel speaker");
         ckv.push_back(std::make_pair(SPK_PRO_DEV_MAP, LEFT_MONO));
         break;
    case SPKR_PROT_ENABLE :
        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getAssociatedDevices Failed \n");
            goto exit;
        }

        for (int i = 0; i < associatedDevices.size(); i++) {
            status = associatedDevices[i]->getDeviceAttributes(&dAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG,"getAssociatedDevices Failed \n");
                goto exit;
            }
            if (dAttr.id == PAL_DEVICE_OUT_SPEAKER) {
                if (dAttr.config.ch_info.channels > 1) {
                    PAL_DBG(LOG_TAG, "Multi channel speaker");
                    ckv.push_back(std::make_pair(SPK_PRO_DEV_MAP, LEFT_RIGHT));
                }
                else {
                    PAL_DBG(LOG_TAG, "Mono channel speaker");
                    ckv.push_back(std::make_pair(SPK_PRO_DEV_MAP, RIGHT_MONO));
                }
                break;
            }
        }
        break;
    case HAPTICS_PROT_ENABLE :
        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"getAssociatedDevices Failed \n");
            return status;
        }

        for (int i = 0; i < associatedDevices.size(); i++) {
            status = associatedDevices[i]->getDeviceAttributes(&dAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG,"getdeviceAttributes Failed \n");
                return status;
            }
            if (dAttr.id == PAL_DEVICE_OUT_HAPTICS_DEVICE) {
                if (dAttr.config.ch_info.channels > 1) {
                    PAL_DBG(LOG_TAG, "Multi channel Haptics Dev");
                    ckv.push_back(std::make_pair(HAPTICS_PRO_DEV_MAP, HAPTICS_LEFT_RIGHT));
                }
                else {
                    PAL_DBG(LOG_TAG, "Mono channel Haptics Dev");
                    ckv.push_back(std::make_pair(HAPTICS_PRO_DEV_MAP, HAPTICS_LEFT_MONO));
                }
                break;
            }
        }
        break;
    case SPKR_VI_ENABLE :
        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"%s: getAssociatedDevices Failed \n", __func__);
            goto exit;
        }

        for (int i = 0; i < associatedDevices.size(); i++) {
            status = associatedDevices[i]->getDeviceAttributes(&dAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG,"%s: getAssociatedDevices Failed \n", __func__);
                goto exit;
            }
            if (dAttr.id == PAL_DEVICE_IN_VI_FEEDBACK) {
                if (dAttr.config.ch_info.channels > 1) {
                    PAL_DBG(LOG_TAG, "Multi channel speaker");
                    ckv.push_back(std::make_pair(SPK_PRO_VI_MAP, STEREO_SPKR));
                }
                else {
                    PAL_DBG(LOG_TAG, "Mono channel speaker");
                    ckv.push_back(std::make_pair(SPK_PRO_VI_MAP, RIGHT_SPKR));
                }
                break;
            }
        }
        break;
    case HAPTICS_VI_ENABLE :
        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"%s: getAssociatedDevices Failed \n", __func__);
            return status;
        }

        for (int i = 0; i < associatedDevices.size(); i++) {
            status = associatedDevices[i]->getDeviceAttributes(&dAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG,"%s: getAssociatedDevices Failed \n", __func__);
                return status;
            }
            if (dAttr.id == PAL_DEVICE_IN_HAPTICS_VI_FEEDBACK) {
                if (dAttr.config.ch_info.channels > 1) {
                    PAL_DBG(LOG_TAG, "Multi channel Haptics Dev");
                    ckv.push_back(std::make_pair(HAPTICS_PRO_VI_MAP, HAPTICS_VI_LEFT_RIGHT));
                }
                else {
                    PAL_DBG(LOG_TAG, "Mono channel Haptics Dev");
                    ckv.push_back(std::make_pair(HAPTICS_PRO_VI_MAP, HAPTICS_VI_LEFT_MONO));
                }
                break;
            }
        }
        break;
    default:
        break;
    }

    PAL_VERBOSE(LOG_TAG,"exit status- %d", status);
error_1:
    if (voldata)
        free(voldata);
exit:
    return status;
}

int PayloadBuilder::populateTagKeyVector(Stream *s, std::vector <std::pair<int,int>> &tkv, int tag, uint32_t* gsltag)
{
    int status = 0;
    PAL_VERBOSE(LOG_TAG,"enter, tag 0x%x", tag);
    struct pal_stream_attributes sAttr;
    struct pal_volume_data *voldata = NULL;
    int voldB = 0;
    float vol = 0.0f;
    int vol_index = 0;

    memset(&sAttr, 0, sizeof(struct pal_stream_attributes));
    status = s->getStreamAttributes(&sAttr);

    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        return status;
    }

    switch (tag) {
    case CRS_CALL_VOLUME:
       voldata = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                         (sizeof(struct pal_channel_vol_kv) * (0xFFFF))));
       if (!voldata) {
           status = -ENOMEM;
           break;
       }
       status = s->getVolumeData(voldata);
       if (0 != status) {
           PAL_ERR(LOG_TAG,"getVolumeData Failed \n");
           goto free_vol;
       }
       if (voldata->no_of_volpair == 1) {
            vol = (voldata->volume_pair[0].vol);
            PAL_VERBOSE(LOG_TAG,"volume sent:%f \n",(voldata->volume_pair[0].vol));
        }
       /*get crs volume index*/
        voldB = lrint(vol * 10.0);
        vol_index = MAX_CRS_VOL_INDEX - voldB;;

        if (vol_index >= 0 && vol_index < 1) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_0));
        }
        else if (vol_index >= 1 && vol_index < 2) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_1));
        }
        else if (vol_index >= 2 && vol_index < 3) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_2));
        }
        else if (vol_index >= 3 && vol_index < 4) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_3));
        }
        else if (vol_index >= 4 && vol_index < 5) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_4));
        }
        else if (vol_index >= 5 && vol_index < 6) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_5));
        }
        else if (vol_index >= 6 && vol_index < 7) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_6));
        }
        else if (vol_index >= 7) {
            tkv.push_back(std::make_pair(VOLUME,LEVEL_7));
        }
        else {
            //Sending LEVEL_4 in default case.
            PAL_INFO(LOG_TAG, "Setting default volume tkv as LEVEL_4");
            tkv.push_back(std::make_pair(VOLUME,LEVEL_4));
        }
        *gsltag = TAG_STREAM_VOLUME;
        break;
    case MUTE_TAG:
       tkv.push_back(std::make_pair(MUTE,ON));
       *gsltag = TAG_MUTE;
       break;
    case UNMUTE_TAG:
       tkv.push_back(std::make_pair(MUTE,OFF));
       *gsltag = TAG_MUTE;
       break;
    case VOICE_SLOW_TALK_OFF:
       tkv.push_back(std::make_pair(TAG_KEY_SLOW_TALK, TAG_VALUE_SLOW_TALK_OFF));
       *gsltag = TAG_STREAM_SLOW_TALK;
       break;
    case VOICE_SLOW_TALK_ON:
       tkv.push_back(std::make_pair(TAG_KEY_SLOW_TALK, TAG_VALUE_SLOW_TALK_ON));
       *gsltag = TAG_STREAM_SLOW_TALK;
       break;
    case CHARGE_CONCURRENCY_ON_TAG:
       tkv.push_back(std::make_pair(ICL, ICL_ON));
       *gsltag = TAG_DEVICE_AL;
       break;
    case CHARGE_CONCURRENCY_OFF_TAG:
       tkv.push_back(std::make_pair(ICL, ICL_OFF));
       *gsltag = TAG_DEVICE_AL;
       break;
    case PAUSE_TAG:
       tkv.push_back(std::make_pair(PAUSE,ON));
       *gsltag = TAG_PAUSE;
       break;
    case RESUME_TAG:
       tkv.push_back(std::make_pair(PAUSE,OFF));
       *gsltag = TAG_PAUSE;
       break;
    case MFC_SR_8K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_8K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_16K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_16K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_32K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_32K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_44K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_44K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_48K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_48K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_96K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_96K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_192K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_192K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case MFC_SR_384K:
       tkv.push_back(std::make_pair(SAMPLINGRATE,SAMPLINGRATE_384K));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case ECNS_ON_TAG:
       tkv.push_back(std::make_pair(ECNS,ECNS_ON));
       *gsltag = TAG_ECNS;
       break;
    case ECNS_OFF_TAG:
       tkv.push_back(std::make_pair(ECNS,ECNS_OFF));
       *gsltag = TAG_ECNS;
       break;
    case EC_ON_TAG:
       tkv.push_back(std::make_pair(ECNS,EC_ON));
       *gsltag = TAG_ECNS;
       break;
    case NS_ON_TAG:
       tkv.push_back(std::make_pair(ECNS,NS_ON));
       *gsltag = TAG_ECNS;
       break;
    case CHS_1:
       tkv.push_back(std::make_pair(CHANNELS, CHANNELS_1));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case CHS_2:
       tkv.push_back(std::make_pair(CHANNELS, CHANNELS_2));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case CHS_3:
       tkv.push_back(std::make_pair(CHANNELS, CHANNELS_3));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case CHS_4:
       tkv.push_back(std::make_pair(CHANNELS, CHANNELS_4));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case BW_16:
       tkv.push_back(std::make_pair(BITWIDTH, BITWIDTH_16));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case BW_24:
       tkv.push_back(std::make_pair(BITWIDTH, BITWIDTH_24));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case BW_32:
       tkv.push_back(std::make_pair(BITWIDTH, BITWIDTH_32));
       if (sAttr.direction == PAL_AUDIO_INPUT)
            *gsltag = TAG_STREAM_MFC_SR;
       else
            *gsltag = TAG_DEVICE_MFC_SR;
       break;
    case INCALL_RECORD_UPLINK:
       tkv.push_back(std::make_pair(TAG_KEY_MUX_DEMUX_CONFIG, TAG_VALUE_MUX_DEMUX_CONFIG_UPLINK));
       *gsltag = TAG_STREAM_MUX_DEMUX;
       break;
    case INCALL_RECORD_DOWNLINK:
       tkv.push_back(std::make_pair(TAG_KEY_MUX_DEMUX_CONFIG, TAG_VALUE_MUX_DEMUX_CONFIG_DOWNLINK));
       *gsltag = TAG_STREAM_MUX_DEMUX;
       break;
    case INCALL_RECORD_UPLINK_DOWNLINK_MONO:
       tkv.push_back(std::make_pair(TAG_KEY_MUX_DEMUX_CONFIG, TAG_VALUE_MUX_DEMUX_CONFIG_UPLINK_DOWNLINK_MONO));
       *gsltag = TAG_STREAM_MUX_DEMUX;
       break;
    case INCALL_RECORD_UPLINK_DOWNLINK_STEREO:
       tkv.push_back(std::make_pair(TAG_KEY_MUX_DEMUX_CONFIG, TAG_VALUE_MUX_DEMUX_CONFIG_UPLINK_DOWNLINK_STEREO));
       *gsltag = TAG_STREAM_MUX_DEMUX;
       break;
#ifdef SEC_AUDIO_CALL_ASR_PERFORMANCE
    case INCALL_RECORD_UPLINK_DOWNLINK_QUADRO:
       tkv.push_back(std::make_pair(TAG_KEY_MUX_DEMUX_CONFIG,  TAG_VALUE_MUX_DEMUX_CONFIG_UPLINK_DOWNLINK_QUADRO));
       *gsltag = TAG_STREAM_MUX_DEMUX;
       break;
#endif
    case LPI_LOGGING_ON:
       tkv.push_back(std::make_pair(LOGGING, LOGGING_ON));
       *gsltag = TAG_DATA_LOGGING;
       break;
    case LPI_LOGGING_OFF:
       tkv.push_back(std::make_pair(LOGGING, LOGGING_OFF));
       *gsltag = TAG_DATA_LOGGING;
       break;
   case DEVICE_MUTE:
       tkv.push_back(std::make_pair(MUTE,ON));
       *gsltag = TAG_DEV_MUTE;
       break;
    case DEVICE_UNMUTE:
       tkv.push_back(std::make_pair(MUTE,OFF));
       *gsltag = TAG_DEV_MUTE;
       break;
   case DEVICEPP_MUTE:
       tkv.push_back(std::make_pair(MUTE,ON));
       *gsltag = TAG_DEVPP_MUTE;
       break;
   case DEVICEPP_UNMUTE:
       tkv.push_back(std::make_pair(MUTE,OFF));
       *gsltag = TAG_DEVPP_MUTE;
       break;
   case ORIENTATION_TAG:
       PAL_INFO(LOG_TAG, "Create orientatation tkv");
       tkv.push_back(std::make_pair(TAG_KEY_ORIENTATION, s->getOrientation()));
       *gsltag = TAG_ORIENTATION;
       break;
#ifdef SEC_AUDIO_BARGEIN_MODE 
   case BARGEIN_MODE_TAG:
       tkv.push_back(std::make_pair(BARGEIN_MODE, s->getBargeinMode()));
       *gsltag = TAG_SPEECH_UI_MODE;
       break;
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
   case INTERPRETER_MODE_TAG:
       tkv.push_back(std::make_pair(INTERPRETER_MODE, s->getInterpreterMode()));
       *gsltag = TAG_INTERPRETER_MODE;
       break;
#endif
    default:
       PAL_ERR(LOG_TAG,"Tag not supported \n");
       break;
    }

    PAL_VERBOSE(LOG_TAG,"exit status- %d", status);
free_vol:
    if (voldata)
        free(voldata);
    return status;
}

void PayloadBuilder::payloadSPConfig(uint8_t** payload, size_t* size, uint32_t miid,
                int param_id, void *param)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (!param) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }


    switch(param_id) {
        case PARAM_ID_SP_TH_VI_R0T0_CFG :
            {
                param_id_sp_th_vi_r0t0_cfg_t *spConf;
                param_id_sp_th_vi_r0t0_cfg_t *data = NULL;
                vi_r0t0_cfg_t* r0t0 = NULL;
                data = (param_id_sp_th_vi_r0t0_cfg_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                              sizeof(param_id_sp_th_vi_r0t0_cfg_t) +
                              sizeof(vi_r0t0_cfg_t) * data->num_ch;

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                spConf = (param_id_sp_th_vi_r0t0_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                r0t0 = (vi_r0t0_cfg_t*) (payloadInfo +
                                sizeof(struct apm_module_param_data_t)
                                + sizeof(param_id_sp_th_vi_r0t0_cfg_t));

                spConf->num_ch = data->num_ch;
                for(int i = 0; i < data->num_ch; i++) {
                    r0t0[i].r0_cali_q24 = data->r0t0_cfg[i].r0_cali_q24;
                    r0t0[i].t0_cali_q6 = data->r0t0_cfg[i].t0_cali_q6;
                }
            }
        break;
        case PARAM_ID_SP_VI_OP_MODE_CFG :
            {
                param_id_sp_vi_op_mode_cfg_t *spConf;
                param_id_sp_vi_op_mode_cfg_t *data;
                uint32_t *channelMap;

                data = (param_id_sp_vi_op_mode_cfg_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                              sizeof(param_id_sp_vi_op_mode_cfg_t) +
                              sizeof(uint32_t) * data->num_speakers;

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                spConf = (param_id_sp_vi_op_mode_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                channelMap = (uint32_t *) (payloadInfo +
                                    sizeof(struct apm_module_param_data_t)
                                    + sizeof(param_id_sp_vi_op_mode_cfg_t));

                spConf->num_speakers = data->num_speakers;
                spConf->th_operation_mode = data->th_operation_mode;
                spConf->th_quick_calib_flag = data->th_quick_calib_flag;
                for(int i = 0; i < data->num_speakers; i++) {
                    if (spConf->th_operation_mode == 0) {
                        channelMap[i] = 0;
                    }
                    else if (spConf->th_operation_mode == 1) {
                        channelMap[i] = 0;
                    }
                }
            }
        break;
        case PARAM_ID_SP_VI_CHANNEL_MAP_CFG :
            {
                param_id_sp_vi_channel_map_cfg_t *spConf;
                param_id_sp_vi_channel_map_cfg_t *data;
                int32_t *channelMap;
                std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

                data = (param_id_sp_vi_channel_map_cfg_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_sp_vi_channel_map_cfg_t) +
                                    (sizeof(int32_t) * data->num_ch);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                spConf = (param_id_sp_vi_channel_map_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                channelMap = (int32_t *) (payloadInfo +
                                    sizeof(struct apm_module_param_data_t)
                                    + sizeof(param_id_sp_vi_channel_map_cfg_t));

                spConf->num_ch = data->num_ch;
                rm->getSpViChannelMapCfg(channelMap, data->num_ch);
            }
        break;
        case PARAM_ID_SP_OP_MODE :
            {
                param_id_sp_op_mode_t *spConf;
                param_id_sp_op_mode_t *data;

                data = (param_id_sp_op_mode_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_sp_op_mode_t);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                spConf = (param_id_sp_op_mode_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                spConf->operation_mode = data->operation_mode;
            }
        break;
        case PARAM_ID_SP_EX_VI_MODE_CFG :
            {
                param_id_sp_ex_vi_mode_cfg_t *spConf;
                param_id_sp_ex_vi_mode_cfg_t *data;

                data = (param_id_sp_ex_vi_mode_cfg_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_sp_ex_vi_mode_cfg_t);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                spConf = (param_id_sp_ex_vi_mode_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                spConf->ex_FTM_mode_enable_flag = data->ex_FTM_mode_enable_flag;
            }
        break;
        case PARAM_ID_SP_TH_VI_FTM_CFG :
        case PARAM_ID_SP_TH_VI_V_VALI_CFG :
        case PARAM_ID_SP_EX_VI_FTM_CFG :
            {
                param_id_sp_th_vi_ftm_cfg_t *spConf;
                param_id_sp_th_vi_ftm_cfg_t *data;
                vi_th_ftm_cfg_t *ftmCfg;
                std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

                data = (param_id_sp_th_vi_ftm_cfg_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_sp_th_vi_ftm_cfg_t) +
                                    sizeof(vi_th_ftm_cfg_t) * data->num_ch;

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
                spConf = (param_id_sp_th_vi_ftm_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                ftmCfg = (vi_th_ftm_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t)
                                + sizeof(param_id_sp_th_vi_ftm_cfg_t));

                spConf->num_ch = data->num_ch;
                for (int i = 0; i < data->num_ch; i++) {
                    ftmCfg[i].wait_time_ms =
                            rm->mSpkrProtModeValue.spkrHeatupTime;
                    ftmCfg[i].ftm_time_ms =
                            rm->mSpkrProtModeValue.operationModeRunTime;
                }
            }
        break;
        case PARAM_ID_SP_TH_VI_FTM_PARAMS:
            {
                param_id_sp_th_vi_ftm_params_t *data;
                data = (param_id_sp_th_vi_ftm_params_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_sp_th_vi_ftm_params_t) +
                                    sizeof(vi_th_ftm_params_t) * data->num_ch;
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
            }
        break;
        case PARAM_ID_SP_EX_VI_FTM_PARAMS:
            {
                param_id_sp_ex_vi_ftm_params_t *data;
                data = (param_id_sp_ex_vi_ftm_params_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_sp_ex_vi_ftm_params_t) +
                                    sizeof(vi_ex_ftm_params_t) * data->num_ch;
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
            }
        break;
        case PARAM_ID_CPS_LPASS_HW_INTF_CFG:
            {
                lpass_swr_hw_reg_cfg_t *data = NULL;
                lpass_swr_hw_reg_cfg_t *cfgPayload = NULL;
                param_id_cps_lpass_hw_intf_cfg_t *spConf = NULL;
                data = (lpass_swr_hw_reg_cfg_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(lpass_swr_hw_reg_cfg_t) +
                                    sizeof(pkd_reg_addr_t) * data->num_spkr +
                                    sizeof(uint32_t);
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
                spConf = (param_id_cps_lpass_hw_intf_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                cfgPayload = (lpass_swr_hw_reg_cfg_t * ) (payloadInfo +
                                sizeof(struct apm_module_param_data_t) +
                                sizeof(uint32_t));
                spConf->lpass_hw_intf_cfg_mode = 1;

                memcpy(cfgPayload, data, sizeof(lpass_swr_hw_reg_cfg_t) +
                                sizeof(pkd_reg_addr_t) * data->num_spkr);
            }
        break;
        case PARAM_ID_CPS_LPASS_SWR_THRESHOLDS_CFG:
            {
                param_id_cps_lpass_swr_thresholds_cfg_t *data = NULL;
                param_id_cps_lpass_swr_thresholds_cfg_t *spThrshConf = NULL;
                data = (param_id_cps_lpass_swr_thresholds_cfg_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_cps_lpass_swr_thresholds_cfg_t) +
                                    (sizeof(cps_reg_wr_values_t) * data->num_spkr);
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
                spThrshConf = (param_id_cps_lpass_swr_thresholds_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                memcpy(spThrshConf, data, sizeof(param_id_cps_lpass_swr_thresholds_cfg_t) +
                                (sizeof(cps_reg_wr_values_t) * data->num_spkr));
            }
        break;
        case PARAM_ID_CPS_CHANNEL_MAP :
            {
                param_id_cps_ch_map_t *spConf;
                param_id_cps_ch_map_t *data;
                int32_t *channelMap;

                data = (param_id_cps_ch_map_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_cps_ch_map_t) +
                                    (sizeof(int32_t) * data->num_ch);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                spConf = (param_id_cps_ch_map_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                channelMap = (int32_t *) (payloadInfo +
                                    sizeof(struct apm_module_param_data_t)
                                    + sizeof(param_id_cps_ch_map_t));

                spConf->num_ch = data->num_ch;
                for (int i = 0; i < data->num_ch; i++) {
                    channelMap[i] = i+1;
                }
            }
        break;
        default:
            {
                PAL_ERR(LOG_TAG, "unknown param id 0x%x", param_id);
            }
        break;
    }

    if (header) {
        header->module_instance_id = miid;
        header->param_id = param_id;
        header->error_code = 0x0;
        header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    }

    *size = payloadSize + padBytes;
    *payload = payloadInfo;
}

#ifdef ENABLE_TAS_SPK_PROT
void PayloadBuilder::payloadTISPConfig(uint8_t** payload, size_t* size, uint32_t miid,
                int param_id, void *param)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    switch(param_id) {

        case TI_PARAM_ID_SP_CALIB_DATA:
        case TI_PARAM_ID_SP_CALIB_INIT_L:
        case TI_PARAM_ID_SP_CALIB_INIT_R:
        case TI_PARAM_ID_SP_CALIB_DEINIT_L:
        case TI_PARAM_ID_SP_CALIB_DEINIT_R:
        case TI_PARAM_ID_SP_SET_VBAT_MON:
        case TI_PARAM_ID_SP_SET_DRV_OP_MODE:
        case TI_PARAM_ID_SP_SET_SKIN_TEMP_L:
        case TI_PARAM_ID_SP_SET_SKIN_TEMP_R:
        case TI_PARAM_ID_SP_SET_HANDSET_MODE:
            {

                if (!param) {
                    PAL_ERR(LOG_TAG, "Invalid input parameters");
                    return;
                }

                uint8_t *data;
                size_t ti_smartpa_payload_size =
                        TAS_DECODE_LEN_PARAM_IDX(param_id) * sizeof(int32_t);

                payloadSize = sizeof(struct apm_module_param_data_t) +
                        ti_smartpa_payload_size;
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                PAL_DBG(LOG_TAG, "TI Speakerprotection encoded parm_idx 0x%x param_id %d",
                        param_id, TAS_DECODE_PARAM_ID_FROM_PARAM_IDX(param_id));

                payloadInfo = (uint8_t*) calloc(1, (payloadSize + padBytes));
                if (payloadInfo) {
                    data = payloadInfo;
                    header = (struct apm_module_param_data_t *)data;
                    data += sizeof(struct apm_module_param_data_t);
                    memcpy(data, param, ti_smartpa_payload_size);

                } else {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s for TI Speakerprotection for parm_id 0x%x",
                            strerror(errno), param_id);
                    return;
                }
            }
        break;

        case TI_PARAM_ID_SP_BIG_DATA_L:
        case TI_PARAM_ID_SP_BIG_DATA_R:
        case TI_PARAM_ID_SP_GET_RE_RANGE_L:
        case TI_PARAM_ID_SP_GET_RE_RANGE_R:
        case TI_PARAM_ID_SP_GET_RE_L:
        case TI_PARAM_ID_SP_GET_RE_R:
        case TI_PARAM_ID_SP_GET_TV_L:
        case TI_PARAM_ID_SP_GET_TV_R:
        case TI_PARAM_ID_SP_GET_AMB_TEMP_RANGE_L:
        case TI_PARAM_ID_SP_GET_AMB_TEMP_RANGE_R:
        case TI_PARAM_ID_SP_GET_SILENCE_COUNT_L:
        case TI_PARAM_ID_SP_GET_SILENCE_COUNT_R:
            {

                PAL_INFO(LOG_TAG,
                        "payloadInfo for TI Speakerprotection for parm_id 0x%x %d",
                        param_id, TAS_DECODE_PARAM_ID_FROM_PARAM_IDX(param_id));

                payloadSize = sizeof(struct apm_module_param_data_t) +
                        (TAS_DECODE_LEN_PARAM_IDX(param_id) * sizeof(uint32_t));
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, (payloadSize + padBytes));
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG,
                            "payloadInfo malloc failed %s for TI Speakerprotection for parm_id 0x%x",
                            strerror(errno), param_id);
                    return;
                }

                header = (struct apm_module_param_data_t*)payloadInfo;
            }
        break;

        default:
            {
                PAL_ERR(LOG_TAG, "unknown param id 0x%x", param_id);
            }
        break;
    }

    if (header) {
        header->module_instance_id = miid;
        header->param_id = param_id;
        header->error_code = 0x0;
        header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    	PAL_DBG(LOG_TAG, "param_size %d", header->param_size);		
    }

    *size = payloadSize + padBytes;

    PAL_DBG(LOG_TAG, "size %d payloadSize %d",
            *size,
            payloadSize);

    *payload = payloadInfo;
}

void PayloadBuilder::payloadTISPCommConfig(uint8_t** payload, size_t* size, uint32_t miid,
                int param_id, void *param, int is_get)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    PAL_INFO(LOG_TAG, "param_id 0x%x", param_id);

    /* set param */
    if (is_get == PAL_TISA_SET_PARAM) {
        if (!param) {
            PAL_ERR(LOG_TAG, "Invalid input parameters");
            return;
        }

        uint8_t *data;
        size_t ti_smartpa_payload_size =
                TAS_DECODE_LEN_PARAM_IDX(param_id) * sizeof(int32_t);

        payloadSize = sizeof(struct apm_module_param_data_t) +
                ti_smartpa_payload_size;
        padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

        PAL_INFO(LOG_TAG, "TI-SmartPA: parm_idx 0x%x param_id %d buf sz=%d",
                param_id, TAS_DECODE_PARAM_ID_FROM_PARAM_IDX(param_id),
                ti_smartpa_payload_size);

        payloadInfo = (uint8_t*) calloc(1, (payloadSize + padBytes));
        if (payloadInfo) {
            data = payloadInfo;
            header = (struct apm_module_param_data_t *)data;
            data += sizeof(struct apm_module_param_data_t);
            memcpy(data, param, ti_smartpa_payload_size);

        } else {
            PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s for TI Speakerprotection for parm_id 0x%x",
                    strerror(errno), param_id);
            return;
        }
    } else {
        /* get param */
        PAL_INFO(LOG_TAG,
                "payloadInfo for TI Speakerprotection for parm_id 0x%x %d",
                param_id, TAS_DECODE_PARAM_ID_FROM_PARAM_IDX(param_id));

        payloadSize = sizeof(struct apm_module_param_data_t) +
                (TAS_DECODE_LEN_PARAM_IDX(param_id) * sizeof(uint32_t));
        padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

        payloadInfo = (uint8_t*) calloc(1, (payloadSize + padBytes));
        if (!payloadInfo) {
            PAL_ERR(LOG_TAG,
                    "payloadInfo malloc failed %s for TI Speakerprotection for parm_id 0x%x",
                    strerror(errno), param_id);
            return;
        }

        header = (struct apm_module_param_data_t*)payloadInfo;
    }

    if (header) {
        header->module_instance_id = miid;
        header->param_id = param_id;
        header->error_code = 0x0;
        header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    	PAL_DBG(LOG_TAG, "param_size %d",
            	header->param_size);		
		
    }

    *size = payloadSize + padBytes;

    PAL_DBG(LOG_TAG, "size %d payloadSize %d",
            *size,
            payloadSize);

    *payload = payloadInfo;
}
#endif // ENABLE_TAS_SPK_PROT

void PayloadBuilder::payloadMSPPConfig(uint8_t** payload, size_t* size,
        uint32_t miid, uint32_t gain)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    uint32_t param_id = 0;
    size_t payloadSize = 0, customPayloadSize = 0;
    mspp_volume_ctrl_gain_t *mspp_payload;

    param_id = PARAM_ID_MSPP_VOLUME;
    customPayloadSize = sizeof(mspp_volume_ctrl_gain_t);

    payloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                        + customPayloadSize);
    payloadInfo = (uint8_t *)calloc(1, (size_t)payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = customPayloadSize;

    mspp_payload =
        (mspp_volume_ctrl_gain_t *)(payloadInfo +
         sizeof(struct apm_module_param_data_t));
    mspp_payload->vol_lin_gain = gain;
    ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                     customPayloadSize,
                     mspp_payload,
                     customPayloadSize);

    *size = payloadSize;
    *payload = payloadInfo;
}

void PayloadBuilder::payloadSoftPauseConfig(uint8_t** payload, size_t* size,
        uint32_t miid, uint32_t delayMs)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    uint32_t param_id = 0;
    size_t payloadSize = 0, customPayloadSize = 0;
    pause_downstream_delay_t *pause_payload;

    param_id = PARAM_ID_SOFT_PAUSE_DOWNSTREAM_DELAY;
    customPayloadSize = sizeof(pause_downstream_delay_t);

    payloadSize = PAL_ALIGN_8BYTE(sizeof(struct apm_module_param_data_t)
                                        + customPayloadSize);
    payloadInfo = (uint8_t *)calloc(1, (size_t)payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return;
    }

    header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = customPayloadSize;

    pause_payload =
        (pause_downstream_delay_t *)(payloadInfo +
         sizeof(struct apm_module_param_data_t));
    pause_payload->delay_ms = delayMs;
    ar_mem_cpy(payloadInfo + sizeof(struct apm_module_param_data_t),
                     customPayloadSize,
                     pause_payload,
                     customPayloadSize);

    *size = payloadSize;
    *payload = payloadInfo;
}

void PayloadBuilder::payloadPlaybackRateParametersConfig(uint8_t** payload, size_t* size,
        uint32_t miid, pal_param_playback_rate_t *playbackRate)
{

    uint32_t param_id = PARAM_ID_TSM_SPEED_FACTOR;
    size_t apmParamSize = sizeof(struct apm_module_param_data_t);
    size_t customPayloadSize = sizeof(param_id_tsm_speed_t);
    size_t payloadSize = PAL_ALIGN_8BYTE(apmParamSize + customPayloadSize);

    uint8_t* payloadInfo = (uint8_t *)calloc(1, (size_t)payloadSize);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "failed to allocate memory.");
        return;
    }

    struct apm_module_param_data_t* header = (struct apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = miid;
    header->param_id = param_id;
    header->error_code = 0x0;
    header->param_size = customPayloadSize;

    param_id_tsm_speed_t *tsmSpeedParams = (param_id_tsm_speed_t *)(payloadInfo + apmParamSize);
    tsmSpeedParams->speed_factor = ((playbackRate->speed) * (Q24_MULTIPLIER * 1.0));
    PAL_INFO(LOG_TAG, "speed %f factor %u", playbackRate->speed, tsmSpeedParams->speed_factor);
    ar_mem_cpy(payloadInfo + apmParamSize, customPayloadSize, tsmSpeedParams, customPayloadSize);

    *size = payloadSize;
    *payload = payloadInfo;
}

void PayloadBuilder::payloadHapticsDevPConfig(uint8_t** payload, size_t* size, uint32_t miid,
                                                                   int param_id, void *param)
{
    struct apm_module_param_data_t* header = NULL;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;

    if (!param) {
        PAL_ERR(LOG_TAG, "Invalid input parameters");
        return;
    }

    switch(param_id) {
        case PARAM_ID_HAPTICS_VI_OP_MODE_PARAM :
            {
                param_id_haptics_vi_op_mode_param_t  *HpConf;
                param_id_haptics_vi_op_mode_param_t  *data;

                data = (param_id_haptics_vi_op_mode_param_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                              sizeof(param_id_haptics_vi_op_mode_param_t);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                HpConf = (param_id_haptics_vi_op_mode_param_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                HpConf->th_operation_mode = data->th_operation_mode;
            }
        break;
        case PARAM_ID_HAPTICS_OP_MODE:
            {
                param_id_haptics_op_mode *hpConf;
                param_id_haptics_op_mode *data;

                data = (param_id_haptics_op_mode *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_op_mode );

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                hpConf = (param_id_haptics_op_mode  *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                hpConf->operation_mode = data->operation_mode;
            }
        break;
        case PARAM_ID_HAPTICS_TH_VI_R0T0_SET_PARAM:
            {
                param_id_haptics_th_vi_r0t0_set_param_t *hpConf;
                param_id_haptics_th_vi_r0t0_set_param_t *data = NULL;
                data = (param_id_haptics_th_vi_r0t0_set_param_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                              sizeof(param_id_haptics_th_vi_r0t0_set_param_t);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                hpConf = (param_id_haptics_th_vi_r0t0_set_param_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                hpConf->num_channels = data->num_channels;
                for(int i = 0; i < data->num_channels; i++) {
                    hpConf->r0_cali_q24[i] = data->r0_cali_q24[i];
                    hpConf->t0_cali_q6[i] = data->t0_cali_q6[i];
                }
            }
        break;
        case PARAM_ID_HAPTICS_VI_CHANNEL_MAP_CFG:
            {
                PAL_DBG(LOG_TAG, "PARAM_ID_HAPTICS_VI_CHANNEL_MAP_CFG");
                param_id_haptics_vi_channel_map_cfg_t *hpConf;
                param_id_haptics_vi_channel_map_cfg_t *data;
                int32_t *channelMap;

                data = (param_id_haptics_vi_channel_map_cfg_t *) param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_vi_channel_map_cfg_t)+
                                    (sizeof(int32_t) * data->num_ch);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                hpConf = (param_id_haptics_vi_channel_map_cfg_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                channelMap = (int32_t *) (payloadInfo +
                                    sizeof(struct apm_module_param_data_t)
                                    + sizeof(param_id_haptics_vi_channel_map_cfg_t));

                hpConf->num_ch = data->num_ch;
                for (int i = 0; i < data->num_ch; i++) {
                    channelMap[i] = i+1;
                }
            }
        break;
        case PARAM_ID_HAPTICS_VI_EX_FTM_MODE_PARAM :
            {
                param_id_haptics_vi_ex_FTM_mode_param_t *hpConf;
                param_id_haptics_vi_ex_FTM_mode_param_t *data;

                data = (param_id_haptics_vi_ex_FTM_mode_param_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_vi_ex_FTM_mode_param_t);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;

                hpConf = (param_id_haptics_vi_ex_FTM_mode_param_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                hpConf->ex_FTM_mode_enable_flag = data->ex_FTM_mode_enable_flag;
            }
        break;
        case PARAM_ID_HAPTICS_EX_VI_FTM_SET_CFG :
        case PARAM_ID_HAPTICS_TH_VI_FTM_SET_CFG :
            {
                param_id_haptics_ex_vi_ftm_set_cfg *hpConf;
                param_id_haptics_ex_vi_ftm_set_cfg *data;
                vi_th_ftm_cfg_t *ftmCfg;
                std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

                data = (param_id_haptics_ex_vi_ftm_set_cfg *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_ex_vi_ftm_set_cfg);

                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
                hpConf = (param_id_haptics_ex_vi_ftm_set_cfg *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));

                hpConf->num_channels = data->num_channels;
                for (int i = 0; i < data->num_channels; i++) {
                    hpConf->wait_time_ms[i] =
                            rm->mSpkrProtModeValue.spkrHeatupTime;
                    hpConf->ftm_time_ms[i] =
                            rm->mSpkrProtModeValue.operationModeRunTime;
                }
            }
         break;
        case PARAM_ID_HAPTICS_TH_VI_FTM_GET_PARAM :
            {
                param_id_haptics_th_vi_ftm_get_param_t *data;
                data = (param_id_haptics_th_vi_ftm_get_param_t *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_th_vi_ftm_get_param_t );
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
            }
         break;
        case PARAM_ID_HAPTICS_EX_VI_FTM_GET:
            {
                param_id_haptics_ex_vi_ftm_get *data;
                data = (param_id_haptics_ex_vi_ftm_get *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_ex_vi_ftm_get ) +
                                    sizeof(wsa_ex_vi_ftm_get_param_t) * data->num_channels;
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
            }
        break;
      case PARAM_ID_HAPTICS_EX_VI_PERSISTENT:
            {
                param_id_haptics_ex_vi_persistent *hpConf;
                param_id_haptics_ex_vi_persistent *data;
                data = (param_id_haptics_ex_vi_persistent *) param;
                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_ex_vi_persistent);
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
                hpConf = (param_id_haptics_ex_vi_persistent *) (payloadInfo +
                                  sizeof(struct apm_module_param_data_t));
                memcpy(hpConf, data,  sizeof(param_id_haptics_ex_vi_persistent));
            }
         break;
      case PARAM_ID_HAPTICS_WAVE_DESIGNER_CFG:
            {
                pal_param_haptics_cnfg_t *data;
                param_id_haptics_wave_designer_config_t *hpconf = nullptr;
                rx_wave_designer_config_h  *hpwaveConf = nullptr;
                haptics_wave_designer_config_t *HConfig = nullptr;
                std::shared_ptr<AudioHapticsInterface> hap_info = AudioHapticsInterface::GetInstance();
                int32_t *pwltime = nullptr;
                int32_t *pwlacc = nullptr;

                data = (pal_param_haptics_cnfg_t *) param;

                if (data->mode == PAL_STREAM_HAPTICS_TOUCH) {
                    hap_info->getTouchHapticsEffectConfiguration(data->effect_id, data->isCompose, &HConfig);
                    if (HConfig == nullptr) {
                        PAL_ERR(LOG_TAG, "HapticsConfig is not found.");
                        return;
                    }
                    payloadSize = sizeof(struct apm_module_param_data_t) +
                                     sizeof(param_id_haptics_wave_designer_config_t) +
                                      (sizeof(rx_wave_designer_config_h) *
                                       HConfig->num_channels) +
                                      (sizeof(int32_t) * 2 *
                                      HConfig->num_pwl *
                                      HConfig->num_channels);
                    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                    if (!payloadInfo) {
                        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                        return;
                    }
                    header = (struct apm_module_param_data_t *) payloadInfo;
                    hpconf = (param_id_haptics_wave_designer_config_t *) (payloadInfo +
                                 sizeof(struct apm_module_param_data_t));
                    hpwaveConf = (rx_wave_designer_config_h *) (payloadInfo +
                                  sizeof(struct apm_module_param_data_t)
                                 + sizeof(param_id_haptics_wave_designer_config_t));

                    if (HConfig->num_pwl != 0) {
                        pwltime = (int32_t *) (payloadInfo +
                                     sizeof(struct apm_module_param_data_t) +
                                     sizeof(param_id_haptics_wave_designer_config_t) +
                                     sizeof(rx_wave_designer_config_h));
                        pwlacc = (int32_t *) (payloadInfo +
                                     sizeof(struct apm_module_param_data_t) +
                                     sizeof(param_id_haptics_wave_designer_config_t) +
                                     sizeof(rx_wave_designer_config_h) + sizeof(int32_t) *
                                      HConfig->num_pwl);
                    }
                    hpconf->num_channels = HConfig->num_channels;
                    PAL_DBG(LOG_TAG, "Haptics Effect num_channel %d", hpconf->num_channels);
                    hpconf->channel_mask = data->ch_mask;
                    PAL_DBG(LOG_TAG, "Haptics Effect, channel_mask %d", hpconf->channel_mask);
                    for (int ch = 0;ch < hpconf->num_channels; ch++) {
                        //For Precanned Mode, append effect ID detail as well
                        hpwaveConf[ch].wave_design_mode =
                                      (uint32_t)HConfig->wave_design_mode;
                        if (hpwaveConf[ch].wave_design_mode == 0x4) {
                            hpwaveConf[ch].wave_design_mode |= (data->effect_id << 16);
                        }
                        PAL_DBG(LOG_TAG, "Haptics Effect, desgn mode %d",
                                                 hpwaveConf[ch].wave_design_mode);
                        hpwaveConf[ch].auto_overdrive_brake_en =
                                           HConfig->auto_overdrive_brake_en;
                        PAL_DBG(LOG_TAG, "Haptics Effect, auto_ov_b_en %d",
                                                hpwaveConf[ch].auto_overdrive_brake_en);
                        hpwaveConf[ch].f0_tracking_en =
                                          HConfig->f0_tracking_en;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .f0_tracking_en %d",
                                                hpwaveConf[ch].f0_tracking_en);
                        hpwaveConf[ch].f0_tracking_param_reset_flag =
                                         HConfig->f0_tracking_param_reset_flag;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .f0_param_reset_flag %d",
                                         hpwaveConf[ch].f0_tracking_param_reset_flag);
                        hpwaveConf[ch].override_flag =
                                           HConfig->override_flag;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .override_flag %d",
                                                      hpwaveConf[ch].override_flag);
                        hpwaveConf[ch].tracked_freq_warmup_time_ms =
                                   HConfig->tracked_freq_warmup_time_ms;
                        PAL_DBG(LOG_TAG, "Haptics Effect, tracked_freq_warmup_time_ms %d",
                                               hpwaveConf[ch].tracked_freq_warmup_time_ms);
                        hpwaveConf[ch].settling_time_ms =
                                        HConfig->settling_time_ms;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .settling_time_ms %d",
                                                           hpwaveConf[ch].settling_time_ms);
                        hpwaveConf[ch].delay_time_ms =
                                        HConfig->delay_time_ms;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .delay_time_ms %d",
                                                             hpwaveConf[ch].delay_time_ms);
                        hpwaveConf[ch].wavegen_fstart_hz_q20 =
                                   HConfig->wavegen_fstart_hz_q20;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .wavegen_fstart_hz_q20 %d",
                                                         hpwaveConf[ch].wavegen_fstart_hz_q20);
                        hpwaveConf[ch].repetition_count =
                                        HConfig->repetition_count;
                        PAL_DBG(LOG_TAG, "Haptics Effect, .repetition_count %d",
                                                        hpwaveConf[ch].repetition_count);
                        hpwaveConf[ch].repetition_period_ms =
                                    HConfig->repetition_period_ms;
                        PAL_DBG(LOG_TAG, "Haptics Effect,.repetition_period_ms %d",
                                                      hpwaveConf[ch].repetition_period_ms);
                        hpwaveConf[ch].pilot_tone_en =
                                           HConfig->pilot_tone_en;
                        PAL_DBG(LOG_TAG, "Haptics Effect .pilot_tone_en %d",
                                                    hpwaveConf[ch].pilot_tone_en);

                        if (data->effect_id >= 0 && !(data->isCompose)) {
                            switch (data->strength) {
                                case 1 :
                                    hpwaveConf[ch].pulse_intensity = HConfig->mid_pulse_intensity;
                                    break;
                                case 2 :
                                    hpwaveConf[ch].pulse_intensity = HConfig->high_pulse_intensity;
                                    break;
                                default:
                                    hpwaveConf[ch].pulse_intensity = HConfig->low_pulse_intensity;
                                    break;
                            }
                        } else {
                            hpwaveConf[ch].pulse_intensity = (data->amplitude * 100);
                        }
                        if (hpwaveConf[ch].pulse_intensity > 100 ||
                                            hpwaveConf[ch].pulse_intensity <= 0)
                            hpwaveConf[ch].pulse_intensity = 30;
                            PAL_DBG(LOG_TAG, "Haptics Effect .pulse_intensity %d for strength %d",
                                                  hpwaveConf[ch].pulse_intensity, data->strength);
                        if (data->effect_id >= 0)
                            hpwaveConf[ch].pulse_width_ms =  HConfig->pulse_width_ms;
                        else
                            hpwaveConf[ch].pulse_width_ms = data->time;
                        PAL_DBG(LOG_TAG, "Haptics Effect .pulse_width_ms %d",
                                                         hpwaveConf[ch].pulse_width_ms);
                        hpwaveConf[ch].pulse_sharpness =
                                         HConfig->pulse_sharpness;
                        PAL_DBG(LOG_TAG, "Haptics Effect .pulse_sharpness %d",
                                                            hpwaveConf[ch].pulse_sharpness);
                        hpwaveConf[ch].num_pwl = HConfig->num_pwl;
                        PAL_DBG(LOG_TAG, "Haptics Effect num pwl %d", hpwaveConf[ch].num_pwl);
                        for (int i = 0; i < HConfig->num_pwl; i++) {
                             pwltime[i] = HConfig->pwl_time[i];
                             pwlacc[i]  = HConfig->pwl_acc[i];
                             PAL_DBG(LOG_TAG, "Haptics Effect pwltime %d and pwlacc %d", pwltime[i],pwlacc[i]);
                        }
                    }
                    free(HConfig);
                } else if(data->mode == PAL_STREAM_HAPTICS_RINGTONE) {
                    payloadSize = sizeof(struct apm_module_param_data_t) +
                                   sizeof(param_id_haptics_wave_designer_config_t) +
                                   (sizeof(rx_wave_designer_config_h));
                    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                    if (!payloadInfo) {
                        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                        return;
                    }
                    header = (struct apm_module_param_data_t *) payloadInfo;
                    hpconf = (param_id_haptics_wave_designer_config_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                    hpwaveConf = (rx_wave_designer_config_h *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t)
                                + sizeof(param_id_haptics_wave_designer_config_t));
                    hpconf->num_channels = 1;
                    hpconf->channel_mask = 1;
                    hpwaveConf[0].wave_design_mode = hap_info->getRingtoneHapticsEffectConfiguration();
                    PAL_ERR(LOG_TAG, "ringtone haptics mode %d", hpwaveConf[0].wave_design_mode);
                } else if(data->mode == PAL_STREAM_HAPTICS_PCM) {
                    payloadSize = sizeof(struct apm_module_param_data_t) +
                                   sizeof(param_id_haptics_wave_designer_config_t) +
                                   (sizeof(rx_wave_designer_config_h));
                    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                    if (!payloadInfo) {
                        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                        return;
                    }
                    header = (struct apm_module_param_data_t *) payloadInfo;
                    hpconf = (param_id_haptics_wave_designer_config_t *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                    hpwaveConf = (rx_wave_designer_config_h *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t)
                                + sizeof(param_id_haptics_wave_designer_config_t));
                    hpconf->num_channels = 1;
                    hpconf->channel_mask = 1;
                    hpwaveConf[0].wave_design_mode = 5;
                    hpwaveConf[0].repetition_count = 1;
                    hpwaveConf[0].num_pwl=0;
                    PAL_ERR(LOG_TAG, "PCM haptics mode %d", hpwaveConf[0].wave_design_mode);
                }
            }
            break;
            case PARAM_ID_HAPTICS_WAVE_DESIGNER_STOP_PARAM:
            {
                 param_id_haptics_wave_designer_wave_designer_stop_param_t *hpConf = nullptr;
                 param_id_haptics_wave_designer_wave_designer_stop_param_t *data;
                 data = (param_id_haptics_wave_designer_wave_designer_stop_param_t *) param;
                 payloadSize = sizeof(struct apm_module_param_data_t) +
                            sizeof(param_id_haptics_wave_designer_wave_designer_stop_param_t);
                 padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                 payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                 if (!payloadInfo) {
                    PAL_ERR(LOG_TAG,"payloadInfo malloc failed %s", strerror(errno));
                    return;
                 }
                 header = (struct apm_module_param_data_t*) payloadInfo;
                 hpConf = (param_id_haptics_wave_designer_wave_designer_stop_param_t *) (payloadInfo +
                                  sizeof(struct apm_module_param_data_t));
                 hpConf->channel_mask = data->channel_mask;
            }
            break;
            case PARAM_ID_HAPTICS_WAVE_DESIGNER_UPDATE_PARAM:
            {
                 param_id_haptics_wave_designer_update_param_t *hpConf = nullptr;
                 pal_param_haptics_cnfg_t *data;
                 rx_wave_designer_update_config_t *hpwaveConf = nullptr;
                 std::shared_ptr<AudioHapticsInterface> hap_info = AudioHapticsInterface::GetInstance();
                 haptics_wave_designer_config_t *HConfig = nullptr;

                 data = (pal_param_haptics_cnfg_t *)param;
                 hap_info->getTouchHapticsEffectConfiguration(-1, false, &HConfig);

                 if (HConfig == nullptr) {
                     PAL_ERR(LOG_TAG, "HapticsConfig is not found.");
                     return;
                 }

                 payloadSize = sizeof(struct apm_module_param_data_t) +
                            sizeof(param_id_haptics_wave_designer_update_param_t) +
                            sizeof(rx_wave_designer_config_h);

                 padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                 payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                 if (!payloadInfo) {
                    PAL_ERR(LOG_TAG,"payloadInfo malloc failed %s", strerror(errno));
                    return;
                 }

                 header = (struct apm_module_param_data_t*) payloadInfo;
                 hpConf = (param_id_haptics_wave_designer_update_param_t *) (payloadInfo +
                                  sizeof(struct apm_module_param_data_t));
                 hpwaveConf = (rx_wave_designer_update_config_t *) (payloadInfo +
                                  sizeof(struct apm_module_param_data_t) +
                                  sizeof(param_id_haptics_wave_designer_update_param_t));
                 hpConf->num_channels = HConfig->num_channels;
                 hpConf->channel_mask = HConfig->channel_mask;
                 hpwaveConf[0].pulse_intensity = data->amplitude * 100;
                 hpwaveConf[0].pulse_sharpness = HConfig->pulse_sharpness;
                 PAL_INFO(LOG_TAG,"updated intensity and sharpness %d and %d for ch_msk %d",
                              hpwaveConf[0].pulse_intensity, hpwaveConf[0].pulse_sharpness,
                              hpConf->channel_mask);

                 free(HConfig);
            }
            break;
            case PARAM_ID_HAPTICS_RX_PCMV_PLAYBACK:
            {
                pal_param_haptics_cnfg_t *data;
                param_id_haptics_rx_pcmv_playback *hpconf = nullptr;
                uint8_t *buf_ptr = nullptr;

                data = (pal_param_haptics_cnfg_t *)param;

                payloadSize = sizeof(struct apm_module_param_data_t) +
                                    sizeof(param_id_haptics_rx_pcmv_playback) +
                                    data->buffer_size;
                padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);
                payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
                if (!payloadInfo) {
                    PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
                    return;
                }
                header = (struct apm_module_param_data_t*) payloadInfo;
                hpconf = (param_id_haptics_rx_pcmv_playback *) (payloadInfo +
                                sizeof(struct apm_module_param_data_t));
                buf_ptr = (uint8_t*) (payloadInfo +
                            sizeof(struct apm_module_param_data_t) +
                            sizeof(struct param_id_haptics_rx_pcmv_playback));
                hpconf->channel_mask = 1;
                hpconf->buffer_size = data->buffer_size;
                memcpy(buf_ptr, data->buffer_ptr, hpconf->buffer_size);
            }
            break;
        default:
            {
                PAL_ERR(LOG_TAG, "unknown param id 0x%x", param_id);
            }
        break;
    }

    if (header) {
        header->module_instance_id = miid;
        header->param_id = param_id;
        header->error_code = 0x0;
        header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    }

    *size = payloadSize + padBytes;

    *payload = payloadInfo;
}

#define NUM_OF_IN_PORTS  1
#define NUM_OF_OUT_PORTS 3
void PayloadBuilder::payloadDAMPortConfig(uint8_t** payload, size_t* size,
                          uint32_t miid, uint8_t numChannel)
{
    size_t in_payloadSize = 0, in_padBytes = 0, out_payloadSize = 0, out_padBytes = 0;
    uint8_t* payloadInfo = NULL;
    struct apm_module_param_data_t* in_header = NULL;
    struct apm_module_param_data_t* out_header = NULL;
    param_id_audio_dam_input_ports_cfg_t* in_port_payload = NULL;
    param_id_audio_dam_output_ports_cfg_t* out_port_payload = NULL;

    in_payloadSize = sizeof(struct apm_module_param_data_t) +
                sizeof(param_id_audio_dam_input_ports_cfg_t) +
                (sizeof(audio_dam_input_port_cfg_t ) * NUM_OF_IN_PORTS +
                sizeof(uint32_t ) * NUM_OF_IN_PORTS * numChannel);
    in_padBytes = PAL_PADDING_8BYTE_ALIGN(in_payloadSize);

    payloadInfo = (uint8_t*) calloc(1, in_payloadSize + in_padBytes);
    if (payloadInfo == NULL) {
        PAL_ERR(LOG_TAG, "payload malloc failed %s", strerror(errno));
        return;
    }

    in_header = (struct apm_module_param_data_t*)(payloadInfo);
    in_header->module_instance_id = miid;
    in_header->param_id = PARAM_ID_AUDIO_DAM_INPUT_PORTS_CFG;
    in_header->error_code = 0x0;
    in_header->param_size = in_payloadSize -  sizeof(struct apm_module_param_data_t);

    PAL_DBG(LOG_TAG, "header params IID:%x param_id:%x error_code:%d"
                " param_size:%d", in_header->module_instance_id, in_header->param_id,
                in_header->error_code, in_header->param_size);

    in_port_payload = (param_id_audio_dam_input_ports_cfg_t *)
                            (payloadInfo + sizeof(struct apm_module_param_data_t));
    in_port_payload->num_input_ports = NUM_OF_IN_PORTS;
    audio_dam_input_port_cfg_t* in_port_cfg_ptr =
                    (audio_dam_input_port_cfg_t *)((uint8_t*)in_port_payload + sizeof(uint32_t));
    in_port_cfg_ptr->input_port_id = 2;
    in_port_cfg_ptr->num_channels = numChannel;
    uint32_t* channel_ids = (uint32_t*)((uint8_t*)in_port_cfg_ptr +
                                         sizeof(audio_dam_input_port_cfg_t));
    for (int i = 0; i < numChannel; ++i)
        channel_ids[i] = i + 1;

    out_payloadSize = sizeof(struct apm_module_param_data_t) +
                sizeof(param_id_audio_dam_output_ports_cfg_t) +
                (sizeof(audio_dam_output_port_cfg_t ) * NUM_OF_OUT_PORTS +
                sizeof(channel_map_t) * NUM_OF_OUT_PORTS * numChannel);
    out_padBytes = PAL_PADDING_8BYTE_ALIGN(out_payloadSize);

    payloadInfo = (uint8_t*) realloc(payloadInfo, in_payloadSize + in_padBytes +
                                        out_payloadSize + out_padBytes);
    if (payloadInfo == NULL) {
        PAL_ERR(LOG_TAG, "payload malloc failed %s", strerror(errno));
        return;
    }

    out_header = (struct apm_module_param_data_t*)(payloadInfo + in_payloadSize + in_padBytes);
    out_header->module_instance_id = miid;
    out_header->param_id = PARAM_ID_AUDIO_DAM_OUTPUT_PORTS_CFG;
    out_header->error_code = 0x0;
    out_header->param_size = out_payloadSize -  sizeof(struct apm_module_param_data_t);

    PAL_DBG(LOG_TAG, "header params IID:%x param_id:%x error_code:%d"
                " param_size:%d", out_header->module_instance_id, out_header->param_id,
                out_header->error_code, out_header->param_size);

    out_port_payload = (param_id_audio_dam_output_ports_cfg_t *)
                            (payloadInfo + in_payloadSize + in_padBytes +
                            sizeof(struct apm_module_param_data_t));
    out_port_payload->num_output_ports = NUM_OF_OUT_PORTS;
    audio_dam_output_port_cfg_t* out_port_cfg_ptr =
                (audio_dam_output_port_cfg_t *)((uint8_t*)out_port_payload + sizeof(uint32_t));
    channel_map_t* output_ch_map;
    for (int i = 0; i < NUM_OF_OUT_PORTS; ++i) {
        out_port_cfg_ptr->output_port_id = 2 * i + 1;
        out_port_cfg_ptr->num_channels = numChannel;
        output_ch_map = (channel_map_t*)((uint8_t*)out_port_cfg_ptr +
                                         sizeof(audio_dam_output_port_cfg_t));
        for (int j = 0; j < numChannel; ++j) {
            output_ch_map[j].input_ch_id = j + 1;
            output_ch_map[j].output_ch_id = j + 1;
            out_port_cfg_ptr =
                (audio_dam_output_port_cfg_t *)((uint8_t*)&output_ch_map[j] +
                                                sizeof(channel_map_t));
        }
    }

    *size = in_payloadSize + in_padBytes + out_payloadSize + out_padBytes;
    *payload = payloadInfo;

    PAL_DBG(LOG_TAG, "payload %pK size %zu, num of ch: %d", *payload, *size, numChannel);
}
#undef NUM_OF_IN_PORTS
#undef NUM_OF_OUT_PORTS

std::unique_ptr<uint8_t[]> PayloadBuilder::getPayloadEncoderBitrate(
    uint32_t encoderMIID, uint32_t newBitrate, size_t &outputPayloadSize) {
    const auto sizeAPM = sizeof(apm_module_param_data_t);
    const auto sizeParamBitrate = sizeof(param_id_enc_bitrate_param_t);

    auto payload = std::make_unique<uint8_t[]>(sizeAPM + sizeParamBitrate);
    if (!payload) {
        return nullptr;
    }

    auto header = (apm_module_param_data_t *)((uint8_t*)payload.get());
    if (!header) {
        return nullptr;
    }

    header->module_instance_id = encoderMIID;
    header->param_id = PARAM_ID_ENC_BITRATE;
    header->error_code = 0x0;
    header->param_size = sizeParamBitrate;

    auto bitrate_param =
        (param_id_enc_bitrate_param_t *)(((uint8_t *)payload.get()) + sizeAPM);
    bitrate_param->bitrate = newBitrate;

    outputPayloadSize = sizeAPM + sizeParamBitrate;

    return std::move(payload);
}

void PayloadBuilder::USToneRendererNotifyPayload(uint8_t **payload, size_t *size,
        struct pal_device *dAttr, uint32_t moduleId,
        us_tone_renderer_ep_media_format_status_t event)
{
    size_t payloadSize, padBytes;
    uint8_t* payloadInfo = NULL;
    struct apm_module_param_data_t* header = NULL;
    data_event_id_us_tone_renderer_media_format_change_t* fmt_payload = NULL;
    uint8_t* pcmChannel = NULL;

    payloadSize = sizeof(struct apm_module_param_data_t) +
                sizeof(param_id_us_tone_renderer_ep_media_format_cfg_t) +
                (sizeof(uint8_t) * dAttr->config.ch_info.channels);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t*) calloc(1, payloadSize + padBytes);
    if (payloadInfo == NULL) {
        PAL_ERR(LOG_TAG, "payload malloc failed %s", strerror(errno));
        return;
    }

    header = (struct apm_module_param_data_t*)(payloadInfo);
    header->module_instance_id = moduleId;
    header->param_id = PARAM_ID_US_TONE_RENDERER_EP_MEDIA_FORMAT_CFG;
    header->error_code = 0x0;
    header->param_size = payloadSize -  sizeof(struct apm_module_param_data_t);
    pcmChannel = (uint8_t*)(payloadInfo + sizeof(struct apm_module_param_data_t) +
                            sizeof(struct param_id_us_tone_renderer_ep_media_format_cfg_t));

    PAL_VERBOSE(LOG_TAG, "header params IID:%x param_id:%x error_code:%d"
                " param_size:%d", header->module_instance_id, header->param_id,
                header->error_code, header->param_size);

    fmt_payload = (param_id_us_tone_renderer_ep_media_format_cfg_t *)
                            (payloadInfo + sizeof(struct apm_module_param_data_t));
    fmt_payload->status = event;
    fmt_payload->sampling_rate = dAttr->config.sample_rate;
    fmt_payload->bit_width = dAttr->config.bit_width;
    fmt_payload->num_channels = dAttr->config.ch_info.channels;

    for (int i = 0; i < dAttr->config.ch_info.channels; ++i) {
        pcmChannel[i] = dAttr->config.ch_info.ch_map[i];
    }

    *size = payloadSize + padBytes;
    *payload = payloadInfo;

    PAL_DBG(LOG_TAG, "event: %d sample_rate:%d bit_width:%d num_channels:%d Miid:%d",
                      fmt_payload->status, fmt_payload->sampling_rate, fmt_payload->bit_width,
                      fmt_payload->num_channels, header->module_instance_id);
    PAL_DBG(LOG_TAG, "payload %pK size %zu", *payload, *size);
}
