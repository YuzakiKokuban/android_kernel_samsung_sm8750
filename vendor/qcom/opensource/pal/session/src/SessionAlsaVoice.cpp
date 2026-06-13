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
Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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


#define LOG_TAG "PAL: SessionAlsaVoice"

#include "SessionAlsaVoice.h"
#include "SessionAlsaUtils.h"
#include "Stream.h"
#include "ResourceManager.h"
#include "apm_api.h"
#include <sstream>
#include <string>
#include <agm/agm_api.h>
#include "audio_route/audio_route.h"
#include <cutils/properties.h>
#ifdef FEATURE_IPQ_OPENWRT
#include "audio_route.h"
#else
#include "audio_route/audio_route.h"
#endif

#ifdef SEC_AUDIO_CALL
#include "SecPalDefs.h"
#endif

#define PAL_PADDING_8BYTE_ALIGN(x)  ((((x) + 7) & 7) ^ 7)
#define MAX_VOL_INDEX 5
#define MIN_VOL_INDEX 0
#define percent_to_index(val, min, max) \
            ((val) * ((max) - (min)) * 0.01 + (min) + .5)

#define NUM_OF_CAL_KEYS 4
#define MAX_RETRY 1
#define POP_SUPPRESSOR_RAMP_DELAY (1*1000)

static uint32_t retries = 0;

SessionAlsaVoice::SessionAlsaVoice(std::shared_ptr<ResourceManager> Rm)
{
   rm = Rm;
   builder = new PayloadBuilder();
   streamHandle = NULL;
   pcmRx = NULL;
   pcmTx = NULL;
   customPayload = NULL;
   customPayloadSize = 0;

   max_vol_index = rm->getMaxVoiceVol();
   if (max_vol_index == -1){
      max_vol_index = MAX_VOL_INDEX;
   }
   mState = SESSION_IDLE;
}

SessionAlsaVoice::~SessionAlsaVoice()
{
   delete builder;

}

struct mixer_ctl* SessionAlsaVoice::getFEMixerCtl(const char *controlName, int *device, pal_stream_direction_t dir)
{
    std::ostringstream CntrlName;
    struct mixer_ctl *ctl;
    char *stream = (char*)"VOICEMMODE1p";

    if (dir == PAL_AUDIO_OUTPUT) {
        if (pcmDevRxIds.size()) {
            *device = pcmDevRxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            return NULL;
        }
    } else if (dir == PAL_AUDIO_INPUT) {
        if (pcmDevTxIds.size()) {
            *device = pcmDevTxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            return NULL;
        }
    }

    if (vsid == VOICEMMODE1 ||
        vsid == VOICELBMMODE1) {
        if (dir == PAL_AUDIO_INPUT) {
            stream = (char*)"VOICEMMODE1c";
        } else {
            stream = (char*)"VOICEMMODE1p";
        }
    } else {
        if (dir == PAL_AUDIO_INPUT) {
            stream = (char*)"VOICEMMODE2c";
        } else {
            stream = (char*)"VOICEMMODE2p";
        }
    }

    CntrlName << stream << " " << controlName;
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        return NULL;
    }

    return ctl;
}

int32_t SessionAlsaVoice::getFrontEndId(uint32_t ldir)
{
    int device = -EINVAL;
    switch(ldir) {
    case RX_HOSTLESS:
        if (pcmDevRxIds.size())
            device = pcmDevRxIds.at(0);
        break;
    case TX_HOSTLESS:
        if (pcmDevTxIds.size())
            device = pcmDevTxIds.at(0);
        break;
    default:
        break;
    }
    return device;
}

bool SessionAlsaVoice::isActive()
{
    PAL_VERBOSE(LOG_TAG, "state = %d", mState);
    return mState == SESSION_STARTED;
}

uint32_t SessionAlsaVoice::getMIID(const char *backendName, uint32_t tagId, uint32_t *miid)
{
    int status = 0;
    int device = 0;

    switch (tagId) {
    case DEVICE_HW_ENDPOINT_TX:
    case BT_PLACEHOLDER_DECODER:
    case COP_DEPACKETIZER_V2:
    case TAG_ECNS:
        if (pcmDevTxIds.size()) {
            device = pcmDevTxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "pcmDevTxIds:%x is not available.",tagId);
            return -EINVAL;
        }
        break;
    case DEVICE_HW_ENDPOINT_RX:
    case BT_PLACEHOLDER_ENCODER:
    case COP_PACKETIZER_V2:
    case COP_PACKETIZER_V0:
    case TAG_DEVICE_PP_MFC:
    case MODULE_SP:
        if (pcmDevRxIds.size()) {
            device = pcmDevRxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "pcmDevRxIds:%x is not available.",tagId);
            return -EINVAL;
        }
        break;
    case RAT_RENDER:
    case BT_PCM_CONVERTER:
        if(strstr(backendName,"TX")) {
          if (pcmDevTxIds.size()) {
              device = pcmDevTxIds.at(0);
          } else {
              PAL_ERR(LOG_TAG, "pcmDevTxIds:%x is not available.",tagId);
              return -EINVAL;
          }
        } else {
           if (pcmDevRxIds.size()) {
               device = pcmDevRxIds.at(0);
           } else {
               PAL_ERR(LOG_TAG, "pcmDevRxIds:%x is not available.",tagId);
               return -EINVAL;
           }
        }
        break;
    default:
        PAL_INFO(LOG_TAG, "Unsupported tag info %x",tagId);
        return -EINVAL;
    }

    status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                                   backendName,
                                                   tagId, miid);
    if (0 != status)
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);

    return status;
}


int SessionAlsaVoice::prepare(Stream * s __unused)
{
   return 0;
}

int SessionAlsaVoice::open(Stream * s)
{
    int status = -EINVAL;
    struct pal_stream_attributes sAttr;
    std::vector<std::shared_ptr<Device>> associatedDevices;

    PAL_DBG(LOG_TAG,"Enter");
    status = s->getStreamAttributes(&sAttr);
    streamHandle = s;
    if(0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        goto exit;
    }

    status = s->getAssociatedDevices(associatedDevices);
    if(0 != status) {
        PAL_ERR(LOG_TAG,"getAssociatedDevices Failed \n");
        goto exit;
    }
    /*check to allow CRS SVA concurrency*/
    struct pal_device deviceAttribute;
    for (int32_t i = 0; i < associatedDevices.size(); i++) {
        status = associatedDevices[i]->getDeviceAttributes(&deviceAttribute, s);
        if (status) {
            PAL_ERR(LOG_TAG, "getDeviceAttributes failed with status %d", status);
            goto exit;
        }
        PAL_INFO(LOG_TAG, "device custom key=%s",
                        deviceAttribute.custom_config.custom_key);
        if (!strncmp(deviceAttribute.custom_config.custom_key,
                    "crsCall", sizeof("crsCall"))) {
                PAL_INFO(LOG_TAG, "setting RM CRS")
                rm->isCRSCallEnabled = true;
        }
    }

 if (sAttr.direction != (PAL_AUDIO_INPUT|PAL_AUDIO_OUTPUT)) {
        PAL_ERR(LOG_TAG,"Voice session dir must be input and output");
        goto exit;
    }

    pcmDevRxIds = rm->allocateFrontEndIds(sAttr, RX_HOSTLESS);
    pcmDevTxIds = rm->allocateFrontEndIds(sAttr, TX_HOSTLESS);
    if (!pcmDevRxIds.size() || !pcmDevTxIds.size()) {
        if (pcmDevRxIds.size()) {
            rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
            pcmDevRxIds.clear();
        }
        if (pcmDevTxIds.size()) {
            rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
            pcmDevTxIds.clear();
        }
        PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
        status = -EINVAL;
        goto exit;
    }

    vsid = sAttr.info.voice_call_info.VSID;
    ttyMode = sAttr.info.voice_call_info.tty_mode;

    rm->getBackEndNames(associatedDevices, rxAifBackEnds, txAifBackEnds);

    if (txAifBackEnds.empty()) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "no TX backend specified for this stream\n");
        goto exit;
    }

    if (rxAifBackEnds.empty()) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "no RX backend specified for this stream\n");
        goto exit;
    }

    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG,"mixer error");
        rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
        rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
        pcmDevRxIds.clear();
        pcmDevTxIds.clear();
        goto exit;
    }

    status = SessionAlsaUtils::open(s, rm, pcmDevRxIds, pcmDevTxIds,
                                    rxAifBackEnds, txAifBackEnds);

    if (status) {
        PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
        rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
        rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
        pcmDevRxIds.clear();
        pcmDevTxIds.clear();
    }

exit:
    PAL_DBG(LOG_TAG,"Exit ret: %d", status);
    return status;
}

int SessionAlsaVoice::setSessionParameters(Stream *s, int dir)
{
    int status = 0;
    int pcmId = 0;

    if (dir == RX_HOSTLESS) {
        if (pcmDevRxIds.size()) {
            pcmId = pcmDevRxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "pcmDevRxIds is not available.");
            status = -EINVAL;
            goto exit;
        }
        status = build_rx_mfc_payload(s);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"populating Rx mfc payload failed :%d", status);
            goto exit;
        }

        // populate_vsid_payload, appends to the existing payload
        status = populate_vsid_payload(s);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"populating vsid payload for RX Failed:%d", status);
            goto exit;
        }

        // set tagged slot mask
        status = setTaggedSlotMask(s);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"setTaggedSlotMask failed:%d", status);
            goto exit;
        }

        status = populateRatPayload(s);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"populateRatPayload failed:%d", status);
            goto exit;
        }
    } else {
        if (pcmDevTxIds.size()) {
            pcmId = pcmDevTxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "pcmDevTxIds is not available.");
            status = -EINVAL;
            goto exit;
        }
        status = populate_vsid_payload(s);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"populating vsid payload for TX Failed:%d", status);
            goto exit;
        }
        status = populate_ch_info_payload(s);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"populating channel info for TX Failed..skipping:%d", status);
        }
    }

    status = SessionAlsaUtils::setMixerParameter(mixer, pcmId,
            customPayload, customPayloadSize);

    if (status != 0) {
        PAL_ERR(LOG_TAG,"setMixerParameter failed:%d for dir:%s",
                status, (dir == RX_HOSTLESS)?"RX":"TX");
        goto exit;
    }

exit:
    freeCustomPayload();
    return status;
}

int SessionAlsaVoice::populate_vsid_payload(Stream *s __unused)
{
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* vsidPayload = NULL;
    size_t vsidpayloadSize = 0, padBytes = 0;
    uint8_t *vsid_pl = NULL;
    vcpm_param_vsid_payload_t vsid_payload;


    vsidpayloadSize = sizeof(struct apm_module_param_data_t)+
                  sizeof(vcpm_param_vsid_payload_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(vsidpayloadSize);

    vsidPayload = (uint8_t *) calloc(1, vsidpayloadSize + padBytes);
    if (!vsidPayload) {
        PAL_ERR(LOG_TAG, "vsid payload allocation failed %s", strerror(errno));
        return -EINVAL;
    }

    header = (apm_module_param_data_t*)vsidPayload;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_VSID;
    header->error_code = 0x0;
    header->param_size = vsidpayloadSize - sizeof(struct apm_module_param_data_t);

    vsid_payload.vsid = vsid;
    vsid_pl = (uint8_t*)vsidPayload + sizeof(apm_module_param_data_t);
    ar_mem_cpy(vsid_pl,  sizeof(vcpm_param_vsid_payload_t),
                     &vsid_payload,  sizeof(vcpm_param_vsid_payload_t));
    vsidpayloadSize += padBytes;

    if (vsidPayload && vsidpayloadSize) {
        status = updateCustomPayload(vsidPayload, vsidpayloadSize);
        freeCustomPayload(&vsidPayload, &vsidpayloadSize);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"updateCustomPayload for vsid payload Failed %d\n", status);
            return status;
        }
    }

    /* call loopback delay playload if in loopback mode*/
    if ((vsid == VOICELBMMODE1 || vsid == VOICELBMMODE2)) {
        populateVSIDLoopbackPayload(s);
    }

    return status;
}

int SessionAlsaVoice::getDeviceChannelInfo(Stream *s, uint16_t *channels)
{
    int status = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct pal_device dAttr;
    int dev_id = 0;
    int idx = 0;

    memset(&dAttr, 0, sizeof(struct pal_device));

    status = s->getAssociatedDevices(associatedDevices);
    if ((0 != status) || (associatedDevices.size() == 0)) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices fails or empty associated devices");
        goto exit;
    }

    rm->getBackEndNames(associatedDevices, rxAifBackEnds, txAifBackEnds);
    if (rxAifBackEnds.empty() && txAifBackEnds.empty()) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "no backend specified for this stream");
        return status;
    }

    for (idx = 0; idx < associatedDevices.size(); idx++) {
        dev_id = associatedDevices[idx]->getSndDeviceId();
        if (rm->isInputDevId(dev_id)) {
            status = associatedDevices[idx]->getDeviceAttributes(&dAttr);
            break;
        }
    }

    if (idx >= associatedDevices.size() || dAttr.id <= PAL_DEVICE_IN_MIN ||
            dAttr.id >= PAL_DEVICE_IN_MAX) {
        PAL_ERR(LOG_TAG, "Failed to get device attributes");
        status = -EINVAL;
        goto exit;
    }

    if (dAttr.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET || dAttr.id == PAL_DEVICE_IN_BLUETOOTH_BLE)
    {
        struct pal_media_config codecConfig;
        status = associatedDevices[idx]->getCodecConfig(&codecConfig);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"getCodecConfig Failed \n");
            goto exit;
        }
        *channels = codecConfig.ch_info.channels;
        PAL_DBG(LOG_TAG,"set devicePPMFC to match codec configuration for %d\n", dAttr.id);
    } else {
        *channels = dAttr.config.ch_info.channels;
    }

exit:
    return status;
}

int SessionAlsaVoice::populate_ch_info_payload(Stream *s)
{
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* ch_infoPayload = NULL;
    size_t ch_info_payloadSize = 0, padBytes = 0;
    uint8_t *ch_info_pl;
    vcpm_param_id_tx_dev_pp_channel_info_t ch_info_payload;
    uint16_t channels = 0;

    status = getDeviceChannelInfo(s, &channels);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"device get channel info failed");
        return status;
    }

    ch_info_payloadSize = sizeof(struct apm_module_param_data_t)+
                  sizeof(vcpm_param_id_tx_dev_pp_channel_info_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(ch_info_payloadSize);

    ch_infoPayload = (uint8_t *) calloc(1, (ch_info_payloadSize + padBytes));

    if (!ch_infoPayload) {
        PAL_ERR(LOG_TAG, "channel info payload allocation failed %s",
            strerror(errno));
        return -ENOMEM;
    }
    header = (apm_module_param_data_t*)ch_infoPayload;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_TX_DEV_PP_CHANNEL_INFO;
    header->error_code = 0x0;
    header->param_size = ch_info_payloadSize - sizeof(struct apm_module_param_data_t);

    ch_info_payload.vsid = vsid;
    ch_info_payload.num_channels = channels;
    PAL_DBG(LOG_TAG, "vsid %d num_channels %d", ch_info_payload.vsid,
                ch_info_payload.num_channels);
    ch_info_pl = (uint8_t*)ch_infoPayload + sizeof(apm_module_param_data_t);
    ar_mem_cpy(ch_info_pl,  sizeof(vcpm_param_id_tx_dev_pp_channel_info_t),
                     &ch_info_payload,  sizeof(vcpm_param_id_tx_dev_pp_channel_info_t));
    ch_info_payloadSize += padBytes;

    if (ch_infoPayload && ch_info_payloadSize) {
        status = updateCustomPayload(ch_infoPayload, ch_info_payloadSize);
        freeCustomPayload(&ch_infoPayload, &ch_info_payloadSize);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"updateCustomPayload for channel info payload Failed %d\n",
                status);
            return status;
        }
    }
    return status;
}

int SessionAlsaVoice::populateVSIDLoopbackPayload(Stream* s){
    int status = 0;
    struct vsid_info vsidInfo;
    apm_module_param_data_t* header;
    uint8_t* loopbackPayload = NULL;
    size_t loopbackPayloadSize = 0, padBytes = 0;
    uint8_t *loopback_pl;
    vcpm_param_id_voc_pkt_loopback_delay_t vsid_loopback_payload;

    rm->getVsidInfo(&vsidInfo);

    PAL_DBG(LOG_TAG, "loopback delay is %d", vsidInfo.loopback_delay);

    if (vsidInfo.loopback_delay == 0) {
        goto exit;
    }

    loopbackPayloadSize = sizeof(struct apm_module_param_data_t)+
                  sizeof(vcpm_param_id_voc_pkt_loopback_delay_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(loopbackPayloadSize);

    loopbackPayload = (uint8_t *) calloc(1, loopbackPayloadSize + padBytes);
    if (!loopbackPayload) {
        PAL_ERR(LOG_TAG, "loopback payload allocation failed %s",
            strerror(errno));
        return -EINVAL;
    }
    header = (apm_module_param_data_t*)loopbackPayload;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_VOC_PKT_LOOPBACK_DELAY;
    header->error_code = 0x0;
    header->param_size = loopbackPayloadSize - sizeof(struct apm_module_param_data_t);

    vsid_loopback_payload.vsid = vsid;
    vsid_loopback_payload.delay_ms = vsidInfo.loopback_delay;
    loopback_pl = (uint8_t*)loopbackPayload + sizeof(apm_module_param_data_t);
    ar_mem_cpy(loopback_pl,  sizeof(vcpm_param_id_voc_pkt_loopback_delay_t),
                     &vsid_loopback_payload,  sizeof(vcpm_param_id_voc_pkt_loopback_delay_t));

    loopbackPayloadSize += padBytes;

    if (loopbackPayload && loopbackPayloadSize) {
        status = updateCustomPayload(loopbackPayload, loopbackPayloadSize);
        freeCustomPayload(&loopbackPayload, &loopbackPayloadSize);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"updateCustomPayload for loopback payload Failed %d\n", status);
            return status;
        }
    }
exit:
    return status;
}

int SessionAlsaVoice::populateRatPayload(Stream *s)
{
    int status = 0;
    int devId = 0;
    int idx = 0;
    uint32_t miid = 0;
    uint8_t* ratPayload = NULL;
    size_t ratPayloadSize = 0;
    struct pal_device dAttr;
    struct pal_media_config config;
    std::vector<std::shared_ptr<Device>> associatedDevices;

    if(!s){
        PAL_ERR(LOG_TAG, "invalid stream pointer")
        status = -EINVAL;
        goto exit;
    }

    status = s->getAssociatedDevices(associatedDevices);
    if ((0 != status) || (associatedDevices.size() == 0)) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices fails or empty associated devices");
        goto exit;
    }
    for (idx = 0; idx < associatedDevices.size(); idx++) {
        devId = associatedDevices[idx]->getSndDeviceId();
        if (rm->isOutputDevId(devId)) {
            break;
        }
    }
    if (associatedDevices[idx]->isScoNbWbActive()) {
        status = getMIID(rxAifBackEnds[0].second.c_str(), RAT_RENDER, &miid);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"getModuleInstanceId failed for RAT_RENDER: %X status: %d",
                RAT_RENDER, status);
            goto exit;
        }
        associatedDevices[idx]->getDeviceAttributes(&dAttr);
        builder->payloadRATConfig(&ratPayload, &ratPayloadSize, miid, &dAttr.config);
        if (ratPayload && ratPayloadSize) {
            status = updateCustomPayload(ratPayload, ratPayloadSize);
            freeCustomPayload(&ratPayload, &ratPayloadSize);
            if (status != 0)
                PAL_ERR(LOG_TAG,"updateCustomPayload for RAT_RENDER %XFailed\n", RAT_RENDER);
        }
    }

exit:
    return status;
}

int SessionAlsaVoice::build_rx_mfc_payload(Stream *s) {
    int status = 0;
    std::vector<uint32_t> rx_mfc_tags{PER_STREAM_PER_DEVICE_MFC, TAG_DEVICE_PP_MFC, TAG_MFC_SIDETONE};

    for (uint32_t rx_mfc_tag : rx_mfc_tags) {
        status = populate_rx_mfc_payload(s, rx_mfc_tag);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"populating Rx mfc: %X payload failed :%d",
                    rx_mfc_tag, status);
            if (rx_mfc_tag == TAG_DEVICE_PP_MFC)
                goto exit;
        }
    }
    status = 0;

exit:
    return status;
}

int SessionAlsaVoice::populate_rx_mfc_payload(Stream *s, uint32_t rx_mfc_tag)
{
    int status = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct sessionToPayloadParam deviceData;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    uint32_t miid = 0;

    status = s->getAssociatedDevices(associatedDevices);
    if ((0 != status) || (associatedDevices.size() == 0)) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices fails or empty associated devices");
        goto exit;
    }

    rm->getBackEndNames(associatedDevices, rxAifBackEnds, txAifBackEnds);
    if (rxAifBackEnds.empty() && txAifBackEnds.empty()) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "no backend specified for this stream");
        return status;
    }
    if (!pcmDevRxIds.size()) {
        PAL_ERR(LOG_TAG, "No pcmDevRxIds found");
        status = -EINVAL;
        goto exit;
    }
    // For MIID of the MFC in the sidetone graph
    if (rx_mfc_tag == TAG_MFC_SIDETONE) {
        status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevTxIds.at(0), txAifBackEnds[0].second.c_str(), rx_mfc_tag, &miid);
    }
    else {
        status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevRxIds.at(0), rxAifBackEnds[0].second.c_str(), rx_mfc_tag, &miid);
    }
    if (status != 0) {
        PAL_ERR(LOG_TAG,"getModuleInstanceId failed for Rx mfc: %X status: %d",
            rx_mfc_tag, status);
        return status;
    }

    status = getDeviceData(s, &deviceData);
    if(status){
        PAL_ERR(LOG_TAG,"failed to get deviceData")
        goto exit;
    }
    builder->payloadMFCConfig(&payload, &payloadSize, miid, &deviceData);
    if (payload && payloadSize) {
        status = updateCustomPayload(payload, payloadSize);
        freeCustomPayload(&payload, &payloadSize);
        if (status != 0)
            PAL_ERR(LOG_TAG,"updateCustomPayload for Rx mfc %XFailed\n", rx_mfc_tag);
    }

exit:
    return status;
}

int SessionAlsaVoice::populate_rx_mfc_coeff_payload(std::shared_ptr<Device> CrsDevice) {
    uint32_t miid = 0;
    int32_t status = 0;
    uint8_t* alsaParamData = NULL;
    size_t alsaPayloadSize = 0;
    std::string backendname;

    rm->getBackendName(CrsDevice->getSndDeviceId(), backendname);
    status = getMIID(backendname.c_str(), TAG_DEVICE_PP_MFC, &miid);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Unable to get MIID");
        return status;
    }

    builder->payloadCRSMFCMixerCoeff((uint8_t **)&alsaParamData, &alsaPayloadSize, miid);
    if (alsaPayloadSize) {
        status = updateCustomPayload(alsaParamData, alsaPayloadSize);
        freeCustomPayload(&alsaParamData, &alsaPayloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
            return status;
        }
    }
    status = SessionAlsaUtils::setMixerParameter(mixer,
                                                 pcmDevRxIds.at(0),
                                                 customPayload,
                                                 customPayloadSize);
    freeCustomPayload();
    if (status != 0) {
        PAL_ERR(LOG_TAG, "setMixerParameter failed");
        return status;
    }

    return status;
}

int SessionAlsaVoice::getDeviceData(Stream *s, struct sessionToPayloadParam *deviceData)
{
    int status = 0;
    int dev_id = 0;
    struct pal_device dAttr;
    int idx = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;

    if(!s){
        PAL_ERR(LOG_TAG, "invalid stream pointer")
        status = -EINVAL;
        goto exit;
    }
    if(!deviceData){
        PAL_ERR(LOG_TAG, "invalid deviceData pointer")
        status = -EINVAL;
        goto exit;
    }

    memset(&dAttr, 0, sizeof(struct pal_device));
    status = s->getAssociatedDevices(associatedDevices);
    if ((0 != status) || (associatedDevices.size() == 0)) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices fails or empty associated devices");
        goto exit;
    }
    for (idx = 0; idx < associatedDevices.size(); idx++) {
        dev_id = associatedDevices[idx]->getSndDeviceId();
        if (rm->isOutputDevId(dev_id)) {
            status = associatedDevices[idx]->getDeviceAttributes(&dAttr);
            break;
        }
    }
    if (dAttr.id == 0) {
        PAL_ERR(LOG_TAG, "Failed to get device attributes");
        status = -EINVAL;
        goto exit;
    }

    if (dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_SCO || dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE)
    {
        struct pal_media_config codecConfig;
        status = associatedDevices[idx]->getCodecConfig(&codecConfig);
        if(0 != status) {
           PAL_ERR(LOG_TAG,"getCodecConfig Failed \n");
            goto exit;
        }
        deviceData->bitWidth = codecConfig.bit_width;
        deviceData->sampleRate = codecConfig.sample_rate;
        deviceData->numChannel = codecConfig.ch_info.channels;
        deviceData->ch_info = nullptr;
        PAL_DBG(LOG_TAG,"set devicePPMFC to match codec configuration for device %d\n", dAttr.id);
    } else {
        // update device pp configuration if virtual port is enabled
        if (rm->activeGroupDevConfig &&
            (dAttr.id == PAL_DEVICE_OUT_SPEAKER ||
             dAttr.id == PAL_DEVICE_OUT_HANDSET)) {
            if (rm->activeGroupDevConfig->devpp_mfc_cfg.sample_rate)
                dAttr.config.sample_rate = rm->activeGroupDevConfig->devpp_mfc_cfg.sample_rate;
            if (rm->activeGroupDevConfig->devpp_mfc_cfg.channels)
                dAttr.config.ch_info.channels = rm->activeGroupDevConfig->devpp_mfc_cfg.channels;
        }
        deviceData->bitWidth = dAttr.config.bit_width;
        deviceData->sampleRate = dAttr.config.sample_rate;
        deviceData->numChannel = dAttr.config.ch_info.channels;
        deviceData->ch_info = nullptr;
    }
exit:
    return status;
}


int SessionAlsaVoice::setTaggedSlotMask(Stream * s)
{
    int status = 0;
    struct pal_device dAttr;
    struct pal_stream_attributes sAttr;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    int dev_id = 0;
    int idx = 0;

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        return status;
    }

    memset(&dAttr, 0, sizeof(struct pal_device));
    status = s->getAssociatedDevices(associatedDevices);
    if ((0 != status) || (associatedDevices.size() == 0)) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices fails or empty associated devices");
        return status;
    }

    for (idx = 0; idx < associatedDevices.size(); idx++) {
        dev_id = associatedDevices[idx]->getSndDeviceId();
        if (rm->isOutputDevId(dev_id)) {
            status = associatedDevices[idx]->getDeviceAttributes(&dAttr);
            break;
        }
    }
    if (dAttr.id == 0) {
        PAL_ERR(LOG_TAG, "Failed to get device attributes");
        status = -EINVAL;
        return status;
    }
    if ((rm->isDeviceMuxConfigEnabled || rm->isUPDVirtualPortEnabled) &&
        (dAttr.id == PAL_DEVICE_OUT_SPEAKER ||dAttr.id == PAL_DEVICE_OUT_HANDSET)) {
         setSlotMask(rm, sAttr, dAttr, pcmDevRxIds);
    }

    return status;
}

int SessionAlsaVoice::start(Stream * s)
{
    struct pcm_config config;
    struct pal_stream_attributes sAttr;
    int32_t status = 0;
    std::shared_ptr<Device> rxDevice = nullptr;
    pal_param_payload *palPayload = NULL;
    int txDevId = PAL_DEVICE_NONE;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    struct pal_volume_data *volume = NULL;
    bool isTxStarted = false, isRxStarted = false;
    struct pal_device dAttr = {};
    std::vector<std::shared_ptr<Device>> associatedDevices;

    PAL_DBG(LOG_TAG,"Enter");

    rm->voteSleepMonitor(s, true);

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        goto exit;
    }

    if (mState == SESSION_IDLE) {
        s->getBufInfo(&in_buf_size,&in_buf_count,&out_buf_size,&out_buf_count);
        memset(&config, 0, sizeof(config));

        config.rate = sAttr.out_media_config.sample_rate;
        if (sAttr.out_media_config.bit_width == 32)
            config.format = PCM_FORMAT_S32_LE;
        else if (sAttr.out_media_config.bit_width == 24)
            config.format = PCM_FORMAT_S24_3LE;
        else if (sAttr.out_media_config.bit_width == 16)
            config.format = PCM_FORMAT_S16_LE;
        config.channels = sAttr.out_media_config.ch_info.channels;
        config.period_size = out_buf_size;
        config.period_count = out_buf_count;
        config.start_threshold = 0;
        config.stop_threshold = 0;
        config.silence_threshold = 0;

        /*setup external ec if needed*/
        status = getRXDevice(s, rxDevice);
        if (status) {
            PAL_ERR(LOG_TAG, "failed, could not find associated RX device");
            goto exit;
        }
        setExtECRef(s, rxDevice, true);

        pcmRx = pcm_open(rm->getVirtualSndCard(), pcmDevRxIds.at(0), PCM_OUT, &config);
        if (!pcmRx) {
            PAL_ERR(LOG_TAG, "Exit pcm-rx open failed");
            ssr_trigger_enable = true;
            status = -EINVAL;
            goto err_pcm_open;
        }

        if (!pcm_is_ready(pcmRx)) {
            PAL_ERR(LOG_TAG, "Exit pcm-rx open not ready");
            ssr_trigger_enable = true;
            status = -EINVAL;
            goto err_pcm_open;
        }

        config.rate = sAttr.in_media_config.sample_rate;
        if (sAttr.in_media_config.bit_width == 32)
            config.format = PCM_FORMAT_S32_LE;
        else if (sAttr.in_media_config.bit_width == 24)
            config.format = PCM_FORMAT_S24_3LE;
        else if (sAttr.in_media_config.bit_width == 16)
            config.format = PCM_FORMAT_S16_LE;
        config.channels = sAttr.in_media_config.ch_info.channels;
        config.period_size = in_buf_size;
        config.period_count = in_buf_count;

        pcmTx = pcm_open(rm->getVirtualSndCard(), pcmDevTxIds.at(0), PCM_IN, &config);
        if (!pcmTx) {
            PAL_ERR(LOG_TAG, "Exit pcm-tx open failed");
            ssr_trigger_enable = true;
            status = -EINVAL;
            goto err_pcm_open;
        }

        if (!pcm_is_ready(pcmTx)) {
            PAL_ERR(LOG_TAG, "Exit pcm-tx open not ready");
            ssr_trigger_enable = true;
            status = -EINVAL;
            goto err_pcm_open;
        }
    }
    mState = SESSION_OPENED;
    status = SessionAlsaVoice::setConfig(s, MODULE, VSID, RX_HOSTLESS);
    if (status) {
        PAL_ERR(LOG_TAG, "setConfig failed %d", status);
        goto err_pcm_open;
    }

    SessionAlsaVoice::setConfig(s, MODULE, CHANNEL_INFO, TX_HOSTLESS);
    volume = (struct pal_volume_data *)malloc(sizeof(uint32_t) +
                                                (sizeof(struct pal_channel_vol_kv)));
    if (!volume) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "volume malloc failed %s", strerror(errno));
        goto err_pcm_open;
    }

    /*if no volume is set set a default volume*/
    if ((s->getVolumeData(volume))) {
        PAL_INFO(LOG_TAG, "no volume set, setting default vol to %f",
                 default_volume);
        volume->no_of_volpair = 1;
        volume->volume_pair[0].channel_mask = 1;
        volume->volume_pair[0].vol = default_volume;
        /*call will cache the volume but not apply it as stream has not moved to start state*/
        s->setVolume(volume);
    };
    /*call to apply volume*/
    if (rm->isCRSCallEnabled) {
        setConfig(s, MODULE, CRS_CALL_VOLUME, RX_HOSTLESS);
    } else {
        setConfig(s, CALIBRATION, TAG_STREAM_VOLUME, RX_HOSTLESS);
    }

    /*set tty mode*/
    if (ttyMode) {
        palPayload = (pal_param_payload *)calloc(1,
                                 sizeof(pal_param_payload) + sizeof(ttyMode));
        if(palPayload != NULL){
            palPayload->payload_size = sizeof(ttyMode);
            *(palPayload->payload) = ttyMode;
            setParameters(s, TTY_MODE, PAL_PARAM_ID_TTY_MODE, palPayload);
        }
    }

    /* configuring RAT_RENDER, updating custom payload if it is a NB/WB SCO usecase*/
    status = populateRatPayload(s);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"Exit Configuring RAT_RENDER failed with status %d", status);
        goto err_pcm_open;
    }

    /* configuring Rx MFC's, updating custom payload and send mixer controls at once*/
    status = build_rx_mfc_payload(s);

    if (status != 0) {
        PAL_ERR(LOG_TAG,"Exit Configuring Rx mfc failed with status %d", status);
        goto err_pcm_open;
    }
    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevRxIds.at(0),
                                                 customPayload, customPayloadSize);
    freeCustomPayload();
    if (status != 0) {
        PAL_ERR(LOG_TAG,"setMixerParameter failed");
        goto err_pcm_open;
    }

    /* set slot_mask as TKV to configure MUX module */
    status = setTaggedSlotMask(s);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"setTaggedSlotMask failed");
        goto err_pcm_open;
    }

    if (ResourceManager::isLpiLoggingEnabled()) {
        status = payloadTaged(s, MODULE, LPI_LOGGING_ON, pcmDevTxIds.at(0), TX_HOSTLESS);
        if (status)
            PAL_ERR(LOG_TAG, "Failed to set data logging param status = %d", status);
    }

    if (ResourceManager::isChargeConcurrencyEnabled) {
        if (PAL_DEVICE_OUT_SPEAKER == rxDevice->getSndDeviceId()) {
            status = Session::NotifyChargerConcurrency(rm, true);
            if (0 == status) {
                status = Session::EnableChargerConcurrency(rm, s);
                //Handle failure case of ICL config
                if (0 != status) {
                    PAL_DBG(LOG_TAG, "Failed to set ICL Config status %d", status);
                    status = Session::NotifyChargerConcurrency(rm, false);
                }
            }
            status = 0;
        }
    }

    if (ResourceManager::isSilenceDetectionEnabledVoice) {

        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed for Silence Detection\n");
            goto silence_det_setup_done;
        }

        for (int i=0; i<associatedDevices.size(); i++) {
        status = associatedDevices[i]->getDeviceAttributes(&dAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getDeviceAttributes Failed for Silence Detection\n");
                goto silence_det_setup_done;
            }
        }

        if ((dAttr.id != PAL_DEVICE_IN_HANDSET_MIC) && (dAttr.id != PAL_DEVICE_IN_SPEAKER_MIC))
            goto silence_det_setup_done;

        (void) Session::enableSilenceDetection(rm, mixer, pcmDevTxIds, txAifBackEnds[0].second.data(), (uint64_t)this);

silence_det_setup_done:
        status = 0;

#ifdef SEC_AUDIO_MUTE_DETECTION
        for (int idx = 0; idx < associatedDevices.size(); idx++) {
            int dev_id = associatedDevices[idx]->getSndDeviceId();
            if (rm->isOutputDevId(dev_id)) {
                status = associatedDevices[idx]->getDeviceAttributes(&dAttr);
                if ((dAttr.id == PAL_DEVICE_OUT_HANDSET) || (dAttr.id == PAL_DEVICE_OUT_SPEAKER)) {
                    (void) Session::enableSilenceDetection(rm, mixer, pcmDevRxIds, rxAifBackEnds[0].second.data(), (uint64_t)this, EVENT_ID_DV_MUTE_DETECTION);
                }
                break;
            }
        }
#endif
    }

    status = pcm_start(pcmRx);
    if (status) {
        PAL_ERR(LOG_TAG, "pcm_start rx failed %d", status);
        goto err_pcm_open;
    }
   isRxStarted = true;

    status = pcm_start(pcmTx);
    if (status) {
        PAL_ERR(LOG_TAG, "pcm_start tx failed %d", status);
        goto err_pcm_open;
    }
    isTxStarted = true;

    if (rm->isCRSCallEnabled) {
        status = populate_rx_mfc_coeff_payload(rxDevice);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"populating Rx mfc coeff payload failed :%d", status);
        }
    }

    /*set sidetone*/
    if (sideTone_cnt == 0) {
        status = getTXDeviceId(s, &txDevId);
        if (status){
            PAL_ERR(LOG_TAG, "could not find TX device associated with this stream cannot set sidetone");
            goto err_pcm_open;
        } else {
            status = setSidetone(txDevId,s,1);
            if(0 != status) {
               PAL_ERR(LOG_TAG,"enabling sidetone failed \n");
            }
        }
    }

#ifdef SEC_AUDIO_MUTE_DETECTION
    if (ResourceManager::isSilenceDetectionEnabledVoice) { // after pcm start
        rm->getKernelConfigInfo(TX_CONFIG_INFO);
        rm->getKernelConfigInfo(RX_CONFIG_INFO);
    }
#endif

    retries = 0;
    status = 0;
    goto exit;

err_pcm_open:
    if (ResourceManager::isSilenceDetectionEnabledVoice)
    {
        (void) Session::disableSilenceDetection(rm, mixer,
                        pcmDevTxIds, txAifBackEnds[0].second.data(), (uint64_t)this);
#ifdef SEC_AUDIO_MUTE_DETECTION
        (void) Session::disableSilenceDetection(rm, mixer,
                        pcmDevRxIds, rxAifBackEnds[0].second.data(), (uint64_t)this, EVENT_ID_DV_MUTE_DETECTION);
#endif
    }

    /*teardown external ec if needed*/
    setExtECRef(s, rxDevice, false);
    if (pcmRx) {
        if (isRxStarted)
            pcm_stop(pcmRx);
        pcm_close(pcmRx);
        pcmRx = NULL;
    }
    if (pcmTx) {
        if (isTxStarted)
            pcm_stop(pcmTx);
        pcm_close(pcmTx);
        pcmTx = NULL;
    }
    if (ssr_trigger_enable) {
        retries++;
        if (retries >= MAX_RETRY) {
            if (status)
                rm->voteSleepMonitor(s, false);
            PAL_ERR(LOG_TAG,"graph open failure reach to max allowed value");
            if (property_set("vendor.audio.ssr.trigger", "1")) {
                PAL_ERR(LOG_TAG, "set property failed");
            }
            status = 0;
            retries = 0;
        }
     }
    mState = SESSION_STARTED;

exit:
    freeCustomPayload();
    if (payload)
        free(payload);
    if (palPayload) {
        free(palPayload);
    }
    if (volume)
        free(volume);
    if (status)
        rm->voteSleepMonitor(s, false);
    PAL_DBG(LOG_TAG,"Exit ret: %d", status);
    return status;
}

int SessionAlsaVoice::stop(Stream * s)
{
    int status = 0;
    int txDevId = PAL_DEVICE_NONE;
    std::shared_ptr<Device> rxDevice = nullptr;

    PAL_DBG(LOG_TAG,"Enter");

    if (ResourceManager::isSilenceDetectionEnabledVoice)
    {
        (void) Session::disableSilenceDetection(rm, mixer,
                        pcmDevTxIds, txAifBackEnds[0].second.data(), (uint64_t)this);
#ifdef SEC_AUDIO_MUTE_DETECTION
        (void) Session::disableSilenceDetection(rm, mixer,
                        pcmDevRxIds, rxAifBackEnds[0].second.data(), (uint64_t)this, EVENT_ID_DV_MUTE_DETECTION);
#endif
    }

    /*disable sidetone*/
    if (sideTone_cnt > 0) {
        status = getTXDeviceId(s, &txDevId);
        if (status){
            PAL_ERR(LOG_TAG, "could not find TX device associated with this stream cannot set sidetone");
        } else {
            status = setSidetone(txDevId,s,0);
            if(0 != status) {
               PAL_ERR(LOG_TAG,"disabling sidetone failed");
            }
        }
    }
    /*config mute on pop suppressor*/
    if (!ssr_trigger_enable) {
        setPopSuppressorMute(s);
        usleep(POP_SUPPRESSOR_RAMP_DELAY);
    }

    if (pcmRx && isActive()) {
        status = pcm_stop(pcmRx);
        if (status) {
            PAL_ERR(LOG_TAG, "pcm_stop - rx failed %d", status);
        }
    }

    if (pcmTx && isActive()) {
        status = pcm_stop(pcmTx);
        if (status) {
            PAL_ERR(LOG_TAG, "pcm_stop - tx failed %d", status);
        }
    }

    /*teardown external ec if needed*/
    status = getRXDevice(s, rxDevice);
    if (status) {
        PAL_ERR(LOG_TAG, "failed, could not find associated RX device");
    } else {
        setExtECRef(s, rxDevice, false);
    }

    rm->voteSleepMonitor(s, false);
    PAL_DBG(LOG_TAG,"Exit ret: %d", status);
    ssr_trigger_enable = false;
    mState = SESSION_STOPPED;

    return status;
}

int SessionAlsaVoice::close(Stream * s)
{
    int status = 0;
    struct pal_stream_attributes sAttr;
    std::string backendname;
    int32_t beDevId = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::vector<std::pair<std::string, int>> freeDeviceMetadata;

    PAL_DBG(LOG_TAG,"Enter");
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
#ifdef SEC_AUDIO_COMMON //QC_FIXME
	    //23682425 [Title] pal Fixed frontEndiIds on all scenarios in session close
        goto exit;
#else
        return status;
#endif
    }

    status = s->getAssociatedDevices(associatedDevices);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices failed\n");
        goto exit;
    }
    freeDeviceMetadata.clear();

    for (auto &dev: associatedDevices) {
         beDevId = dev->getSndDeviceId();
         rm->getBackendName(beDevId, backendname);
         PAL_DBG(LOG_TAG, "backendname %s", backendname.c_str());
         if (dev->getDeviceCount() > 1) {
             PAL_DBG(LOG_TAG, "dev %d still active", beDevId);
             freeDeviceMetadata.push_back(std::make_pair(backendname, 0));
         } else {
             PAL_DBG(LOG_TAG, "dev %d not active", beDevId);
             freeDeviceMetadata.push_back(std::make_pair(backendname, 1));
         }
    }
    status = SessionAlsaUtils::close(s, rm, pcmDevRxIds, pcmDevTxIds,
             rxAifBackEnds, txAifBackEnds, freeDeviceMetadata);

    if (pcmRx) {
        status = pcm_close(pcmRx);
        if (status) {
            PAL_ERR(LOG_TAG, "pcm_close - rx failed %d", status);
        }
    }

    if (pcmTx) {
        status = pcm_close(pcmTx);
        if (status) {
            PAL_ERR(LOG_TAG, "pcm_close - tx failed %d", status);
        }
    }

#ifdef SEC_AUDIO_MUTE_DETECTION
    if (ResourceManager::isSilenceDetectionEnabledVoice) { // after pcm stop
        rm->getKernelConfigInfo(TX_CONFIG_INFO);
        rm->getKernelConfigInfo(RX_CONFIG_INFO);
    }
#endif

exit:
    if (pcmDevRxIds.size()) {
        rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
        pcmDevRxIds.clear();
        pcmRx = NULL;
    }
    if (pcmDevTxIds.size()) {
        rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
        pcmDevTxIds.clear();
        pcmTx = NULL;
    }
    PAL_DBG(LOG_TAG,"Exit ret: %d", status);
    mState = SESSION_IDLE;
    return status;
}

#ifdef SEC_AUDIO_CALL
int SessionAlsaVoice::getParameters(Stream *s, int tagId, uint32_t param_id, void **payload)
{
    int status = 0;
    uint8_t *ptr = NULL;
    uint8_t *payloadData = NULL;
    size_t payloadSize = 0;
    int device = pcmDevRxIds.at(0);
    uint32_t miid = 0;
    const char *control = "getParam";

    struct mixer_ctl *ctl;
    std::ostringstream CntrlName;
    PAL_DBG(LOG_TAG, "Enter.");

    pal_param_payload *param_payload = (pal_param_payload *)payload;

    switch (param_id) {
        case PAL_PARAM_ID_UIEFFECT:
        {
        switch (static_cast<uint32_t>(tagId)) {
            case TAG_ECHOREF_NO_KEY:
            {
                char *stream = SessionAlsaVoice::getMixerVoiceStream(s, TX_HOSTLESS);
                CntrlName << stream << " " << control;
                ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
                if (!ctl) {
                    PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
                    return -ENOENT;
                }
                device = pcmDevTxIds.at(0);
                if (!txAifBackEnds.empty()) { /** search in TX GKV */
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, device, txAifBackEnds[0].second.data(),
                            tagId, &miid);
                    if (status)
                        miid = 0;
                }
                if (miid == 0) {
                    PAL_ERR(LOG_TAG, "failed to look for module with tagID 0x%x(device %d)", tagId, device);
                    status = -EINVAL;
                    goto exit;
                }

                pal_effect_custom_payload_t *customPayload = nullptr;
                effect_pal_payload_t *effectPayload = (effect_pal_payload_t *)(param_payload->payload);
                customPayload = (pal_effect_custom_payload_t *)(effectPayload->payload);
                PAL_INFO(LOG_TAG, "customPayload paramId = 0x%x", customPayload->paramId);
                if (effectPayload->payloadSize < sizeof(pal_effect_custom_payload_t)) {
                    status = -EINVAL;
                    PAL_ERR(LOG_TAG, "memory for retrieved data is too small");
                    goto exit;
                }

                builder->payloadQuery(&payloadData, &payloadSize,
                                        miid, customPayload->paramId,
                                        effectPayload->payloadSize - sizeof(uint32_t));
                status = mixer_ctl_set_array(ctl, payloadData, payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Set custom config failed, status = %d", status);
                    goto exit;
                }

                status = mixer_ctl_get_array(ctl, payloadData, payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "Get custom config failed, status = %d", status);
                    goto exit;
                }

                ptr = (uint8_t *)payloadData + sizeof(struct apm_module_param_data_t);
                ar_mem_cpy(customPayload->data, effectPayload->payloadSize,
                                    ptr, effectPayload->payloadSize);
                break;
            }
            default:
                PAL_ERR(LOG_TAG,"Failed unsupported tag type %x \n",
                        static_cast<uint32_t>(tagId));
                status = -EINVAL;
                break;
            }
            break;
        }
        default:
            status = EINVAL;
            PAL_ERR(LOG_TAG, "Unsupported param id %u status %d", param_id, status);
            goto exit;
    }

exit:
    if (payloadData)
        free(payloadData);
    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}
#endif

int SessionAlsaVoice::setParameters(Stream *s, int tagId, uint32_t param_id __unused, void *payload)
{
    int status = 0;
    int device = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;

    uint32_t tty_mode;
    int mute_dir = RX_HOSTLESS;
    int mute_tag = DEVICE_UNMUTE;
    pal_param_payload *PalPayload = (pal_param_payload *)payload;

#ifdef SEC_AUDIO_CALL
    PAL_INFO(LOG_TAG,"Enter setParam called with tag: %x ", tagId);
#else
    PAL_INFO(LOG_TAG,"Enter setParam called with tag: %d ", tagId);
#endif

    switch (static_cast<uint32_t>(tagId)) {
#ifdef SEC_AUDIO_CALL
        case VOICE_MIC_MODE:
            mic_mode = *((bool *)PalPayload->payload);
            status = payloadCalKeys(s, &paramData, &paramSize);
            if (!paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            PAL_INFO(LOG_TAG, "setParameters VOICE_MIC_MODE  %d", mic_mode);
            status = setVoiceMixerParameter(s, mixer, paramData, paramSize,
                                            TX_HOSTLESS);
            // TBD : need for Rx side as well with additional parameter

            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                        status);
            }
            break;
#endif
        case VOICE_VOLUME_BOOST:
            volume_boost = *((bool *)PalPayload->payload);
            status = payloadCalKeys(s, &paramData, &paramSize);
            if (!paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            status = setVoiceMixerParameter(s, mixer, paramData, paramSize,
                                            RX_HOSTLESS);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                        status);
            }
            break;

        case VOICE_SLOW_TALK_OFF:
        case VOICE_SLOW_TALK_ON:
            if (pcmDevRxIds.size()) {
                device = pcmDevRxIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "pcmDevRxIds is not available.");
                status = -EINVAL;
                goto exit;
            }
            slow_talk = *((bool *)PalPayload->payload);
            status = payloadTaged(s, MODULE, tagId, device, RX_HOSTLESS);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice slow_Talk params status = %d",
                        status);
            }
            break;

        case TTY_MODE:
            tty_mode = *((uint32_t *)PalPayload->payload);
            status = payloadSetTTYMode(&paramData, &paramSize,
                                       tty_mode);
            status = setVoiceMixerParameter(s, mixer, paramData, paramSize,
                                            RX_HOSTLESS);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice tty params status = %d",
                        status);
                break;
            }

            if (!paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get tty payload status %d", status);
                goto exit;
            }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_INFO(LOG_TAG, "set voice tty params status = %d", status);
#endif
            break;

        case VOICE_HD_VOICE:
            hd_voice = *((bool *)PalPayload->payload);
            status = payloadCalKeys(s, &paramData, &paramSize);
            if (!paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            status = setVoiceMixerParameter(s, mixer, paramData, paramSize,
                                            RX_HOSTLESS);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                        status);
            }
            break;
      case DEVICE_MUTE:
            if (pcmDevRxIds.size()) {
                device = pcmDevRxIds.at(0);
            } else {
                PAL_ERR(LOG_TAG, "pcmDevRxIds is not available.");
                status = -EINVAL;
                goto exit;
            }
            dev_mute = *((pal_device_mute_t *)PalPayload->payload);
            if (dev_mute.dir == PAL_AUDIO_INPUT) {
                mute_dir = TX_HOSTLESS;
            }
            if (dev_mute.mute == 1) {
                mute_tag = DEVICE_MUTE;
            }
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_INFO(LOG_TAG, "setting device mute dir %d mute flag %d", mute_dir, mute_tag);
#else
            PAL_DBG(LOG_TAG, "setting device mute dir %d mute flag %d", mute_dir, mute_tag);
#endif
            status = payloadTaged(s, MODULE, mute_tag, device, mute_dir);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set device mute params status = %d",
                        status);
            }
            break;
       default:
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_ERR(LOG_TAG,"Failed unsupported tag type %x \n",
                    static_cast<uint32_t>(tagId));
#else
            PAL_ERR(LOG_TAG,"Failed unsupported tag type %d \n",
                    static_cast<uint32_t>(tagId));
#endif
            status = -EINVAL;
            break;
    }

    if (0 != status) {
        PAL_ERR(LOG_TAG,"Failed to set config data");
        goto exit;
    }

    PAL_VERBOSE(LOG_TAG, "%pK - payload and %zu size", paramData , paramSize);

exit:
if (paramData) {
    free(paramData);
}
    PAL_DBG(LOG_TAG,"exit status:%d ", status);
    return status;

}

int SessionAlsaVoice::setConfig(Stream * s, configType type, int tag)
{
    int status = 0;
    int device = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG,"Enter setConfig called with tag: %x ", tag);
#else
    PAL_DBG(LOG_TAG,"Enter setConfig called with tag: %d ", tag);
#endif

    switch (static_cast<uint32_t>(tag)) {
        case TAG_STREAM_VOLUME:
            status = payloadCalKeys(s, &paramData, &paramSize);
            status = SessionAlsaVoice::setVoiceMixerParameter(s, mixer,
                                                           paramData,
                                                           paramSize,
                                                           RX_HOSTLESS);
            if (status) {
               PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                     status);
            }
            if (!paramData) {
               status = -ENOMEM;
               PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
               goto exit;
            }
            break;
        case MUTE_TAG:
        case UNMUTE_TAG:
            if (pcmDevTxIds.size()) {
               device = pcmDevTxIds.at(0);
               status = payloadTaged(s, type, tag, device, TX_HOSTLESS);
            } else {
              PAL_ERR(LOG_TAG, "pcmDevTxIds:%x is not available.",tag);
              status = -EINVAL;
            }
            break;
        case CHARGE_CONCURRENCY_ON_TAG:
        case CHARGE_CONCURRENCY_OFF_TAG:
            if (pcmDevRxIds.size()) {
               device = pcmDevRxIds.at(0);
               status = payloadTaged(s, type, tag, device, RX_HOSTLESS);
            } else {
              PAL_ERR(LOG_TAG, "pcmDevRxIds:%x is not available.",tag);
              status = -EINVAL;
            }
            break;
        case CRS_CALL_VOLUME:
            if (pcmDevRxIds.size()) {
               device = pcmDevRxIds.at(0);
               status = payloadTaged(s, type, tag, device, RX_HOSTLESS);
            } else {
               PAL_ERR(LOG_TAG, "pcmDevRxIds is not available.");
               status = -EINVAL;
            }
            break;
        default:
            PAL_ERR(LOG_TAG,"Failed unsupported tag type %d", static_cast<uint32_t>(tag));
            status = -EINVAL;
            break;
    }
    if (0 != status) {
        PAL_ERR(LOG_TAG,"Failed to set config data");
        goto exit;
    }

#ifndef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_VERBOSE(LOG_TAG, "%pK - payload and %zu size", paramData , paramSize);
#endif

exit:
if (paramData) {
    free(paramData);
}
    PAL_DBG(LOG_TAG,"Exit status:%d ", status);
    return status;
}

int SessionAlsaVoice::setConfig(Stream * s, configType type __unused, int tag, int dir)
{
    int status = 0;
    int device = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG,"Enter setConfig called with tag: %x ", tag);
#else
    PAL_DBG(LOG_TAG,"Enter setConfig called with tag: %d ", tag);
#endif

    switch (static_cast<uint32_t>(tag)) {

       case TAG_STREAM_VOLUME:
            status = payloadCalKeys(s, &paramData, &paramSize);
            if (status || !paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            status = SessionAlsaVoice::setVoiceMixerParameter(s, mixer,
                                                              paramData,
                                                              paramSize,
                                                              dir);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                        status);
            }
            if (!paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            break;

        case MUTE_TAG:
        case UNMUTE_TAG:
            if (pcmDevTxIds.size()) {
                device = pcmDevTxIds.at(0);
                status = payloadTaged(s, type, tag, device, TX_HOSTLESS);
            } else {
                PAL_ERR(LOG_TAG, "pcmDevTxIds:%x is not available.",tag);
                status = -EINVAL;
            }
            break;

        case VSID:
            status = payloadSetVSID(s);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            status = SessionAlsaVoice::setVoiceMixerParameter(s, mixer,
                                                              customPayload,
                                                              customPayloadSize,
                                                              dir);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                        status);
                goto exit;
            }
            break;

        case CHANNEL_INFO:
            status = payloadSetChannelInfo(s, &paramData, &paramSize);
            status = SessionAlsaVoice::setVoiceMixerParameter(s, mixer,
                                                              paramData,
                                                              paramSize,
                                                              dir);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to set voice params status = %d",
                        status);
                break;
            }

            if (!paramData) {
                status = -ENOMEM;
                PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
                goto exit;
            }
            break;

        case CRS_CALL_VOLUME:
            if (pcmDevRxIds.size()) {
               device = pcmDevRxIds.at(0);
               status = payloadTaged(s, type, tag, device, RX_HOSTLESS);
            } else {
               PAL_ERR(LOG_TAG, "pcmDevRxIds is not available.");
               status = -EINVAL;
            }
            break;

        default:
            PAL_ERR(LOG_TAG,"Failed unsupported tag type %d", static_cast<uint32_t>(tag));
            status = -EINVAL;
            break;
    }
    if (0 != status) {
        PAL_ERR(LOG_TAG,"Failed to set config data\n");
        goto exit;
    }

    PAL_VERBOSE(LOG_TAG, "%pK - payload and %zu size", paramData , paramSize);

exit:
    freeCustomPayload();
    freeCustomPayload(&paramData, &paramSize);
    PAL_DBG(LOG_TAG,"Exit status:%d ", status);
    return status;
}

int SessionAlsaVoice::payloadTaged(Stream * s, configType type, int tag,
                                   int device __unused, int dir){
    int status = 0;
    uint32_t tagsent;
    struct agm_tag_config* tagConfig;
    const char *setParamTagControl = "setParamTag";
    struct mixer_ctl *ctl;
    std::ostringstream tagCntrlName;
    int tkv_size = 0;
    const char *stream = SessionAlsaVoice::getMixerVoiceStream(s, dir);
    switch (type) {
        case MODULE:
            tkv.clear();
            status = builder->populateTagKeyVector(s, tkv, tag, &tagsent);
            if (0 != status) {
                PAL_ERR(LOG_TAG,"Failed to set the tag configuration\n");
                goto exit;
            }

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }

            tagConfig = (struct agm_tag_config*)malloc (sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));

            if(!tagConfig) {
                status = -EINVAL;
                goto exit;
            }

            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }
            tagCntrlName<<stream<<" "<<setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                if (tagConfig)
                    free(tagConfig);
                return -ENOENT;
            }

            tkv_size = tkv.size()*sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG,"failed to set the tag calibration %d", status);
                goto exit;
            }
            ctl = NULL;
            tkv.clear();
            if (tagConfig) {
                free(tagConfig);
            }
            break;
        default:
            PAL_ERR(LOG_TAG,"invalid type ");
            status = -EINVAL;
    }

exit:
    return status;
}

int SessionAlsaVoice::payloadSetVSID(Stream* s){
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t *vsid_pl;
    vcpm_param_vsid_payload_t vsid_payload;

    payloadSize = sizeof(struct apm_module_param_data_t)+
                  sizeof(vcpm_param_vsid_payload_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = (uint8_t *) calloc(1, payloadSize + padBytes);
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return -EINVAL;
    }
    header = (apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_VSID;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    vsid_payload.vsid = vsid;
    vsid_pl = (uint8_t*)payloadInfo + sizeof(apm_module_param_data_t);
    ar_mem_cpy(vsid_pl,  sizeof(vcpm_param_vsid_payload_t),
                     &vsid_payload,  sizeof(vcpm_param_vsid_payload_t));
    payloadSize += padBytes;

    if (payloadInfo && payloadSize) {
        status = updateCustomPayload(payloadInfo, payloadSize);
        freeCustomPayload(&payloadInfo, &payloadSize);
        if (status != 0) {
            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
            return status;
        }
    }

    /* call loopback delay playload if in loopback mode*/
    if ((vsid == VOICELBMMODE1 || vsid == VOICELBMMODE2)) {
        populateVSIDLoopbackPayload(s);
    }


    return status;
}

int SessionAlsaVoice::payloadSetChannelInfo(Stream * s, uint8_t **payload, size_t *size)
{
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t *ch_info_pl;
    vcpm_param_id_tx_dev_pp_channel_info_t ch_info_payload;
    uint16_t channels = 0;

    status = getDeviceChannelInfo(s, &channels);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"device get channel info failed");
        return status;
    }

    payloadSize = sizeof(struct apm_module_param_data_t)+
                  sizeof(vcpm_param_id_tx_dev_pp_channel_info_t);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return -EINVAL;
    }
    header = (apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_TX_DEV_PP_CHANNEL_INFO;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    PAL_DBG(LOG_TAG, "vsid %d num_channels %d", vsid, channels);
    ch_info_payload.vsid = vsid;
    ch_info_payload.num_channels = channels;
    ch_info_pl = (uint8_t*)payloadInfo + sizeof(apm_module_param_data_t);
    ar_mem_cpy(ch_info_pl,  sizeof(vcpm_param_id_tx_dev_pp_channel_info_t),
                     &ch_info_payload,  sizeof(vcpm_param_id_tx_dev_pp_channel_info_t));

    *size = payloadSize + padBytes;
    *payload = payloadInfo;

    return status;
}

int SessionAlsaVoice::payloadCalKeys(Stream * s, uint8_t **payload, size_t *size)
{
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t *vol_pl;
    vcpm_param_cal_keys_payload_t cal_keys;
    vcpm_ckv_pair_t cal_key_pair[NUM_OF_CAL_KEYS];
    float volume = 0.0;
    int vol;
    struct pal_volume_data *voldata = NULL;
#ifdef SEC_AUDIO_CALL
    char VolIdexMaxTemp[PROPERTY_VALUE_MAX] = "";
    char VolIndexMaxInit[PROPERTY_VALUE_MAX] = "";
#endif

    voldata = (struct pal_volume_data *)calloc(1, (sizeof(uint32_t) +
                      (sizeof(struct pal_channel_vol_kv) * (0xFFFF))));
    if (!voldata) {
        status = -ENOMEM;
        goto exit;
    }
    status = s->getVolumeData(voldata);
    if(0 != status) {
        PAL_ERR(LOG_TAG,"getVolumeData Failed");
        goto exit;
    }

    PAL_VERBOSE(LOG_TAG,"volume sent:%f", (voldata->volume_pair[0].vol));
    volume = (voldata->volume_pair[0].vol);

    payloadSize = sizeof(apm_module_param_data_t) +
                  sizeof(vcpm_param_cal_keys_payload_t) +
                  sizeof(vcpm_ckv_pair_t)*NUM_OF_CAL_KEYS;
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return -EINVAL;
    }
    header = (apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_CAL_KEYS;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);
    cal_keys.vsid = vsid;
    cal_keys.num_ckv_pairs = NUM_OF_CAL_KEYS;
    if (volume < 0.0) {
            volume = 0.0;
    } else if (volume > 1.0) {
        volume = 1.0;
    }

    vol = lrint(volume * 100.0);

    // Voice volume levels from android are mapped to driver volume levels as follows.
    // 0 -> 5, 20 -> 4, 40 ->3, 60 -> 2, 80 -> 1, 100 -> 0
    // So adjust the volume to get the correct volume index in driver
    vol = 100 - vol;

    /*volume key*/
    cal_key_pair[0].cal_key_id = VCPM_CAL_KEY_ID_VOLUME_LEVEL;
#ifdef SEC_AUDIO_CALL
    VolIndexMaxInit[0] = '0' + max_vol_index;
    property_get("ro.config.vc_call_vol_steps", VolIdexMaxTemp, VolIndexMaxInit);
    cal_key_pair[0].value = (int)percent_to_index(vol, MIN_VOL_INDEX, atoi(VolIdexMaxTemp));
#else
    cal_key_pair[0].value = percent_to_index(vol, MIN_VOL_INDEX, max_vol_index);
#endif

    /*cal key for volume boost*/
    cal_key_pair[1].cal_key_id = VCPM_CAL_KEY_ID_VOL_BOOST;
    cal_key_pair[1].value = volume_boost;

     /*cal key for BWE/HD_VOICE*/
    cal_key_pair[2].cal_key_id = VCPM_CAL_KEY_ID_BWE;
    cal_key_pair[2].value = hd_voice;

#ifdef SEC_AUDIO_CALL
    /*cal key for Mic mode*/
    cal_key_pair[3].cal_key_id = VCPM_CAL_KEY_ID_MIC_MODE;
    cal_key_pair[3].value = mic_mode;
#endif

    vol_pl = (uint8_t*)payloadInfo + sizeof(apm_module_param_data_t);
    ar_mem_cpy(vol_pl, sizeof(vcpm_param_cal_keys_payload_t),
                     &cal_keys, sizeof(vcpm_param_cal_keys_payload_t));

    vol_pl += sizeof(vcpm_param_cal_keys_payload_t);
    ar_mem_cpy(vol_pl, sizeof(vcpm_ckv_pair_t)*NUM_OF_CAL_KEYS,
                     &cal_key_pair, sizeof(vcpm_ckv_pair_t)*NUM_OF_CAL_KEYS);


    *size = payloadSize + padBytes;
    *payload = payloadInfo;

#ifdef SEC_AUDIO_CALL
    PAL_INFO(LOG_TAG, "Volume level: %d, volume boost: %d, HD voice: %d mic_mode: %d",
            cal_key_pair[0].value, volume_boost, hd_voice, mic_mode);
#else
    PAL_DBG(LOG_TAG, "Volume level: %lf, volume boost: %d, HD voice: %d",
            percent_to_index(vol, MIN_VOL_INDEX, max_vol_index),
            volume_boost, hd_voice);
#endif

exit:
    if (voldata) {
        free(voldata);
    }
    return status;
}

int SessionAlsaVoice::payloadSetTTYMode(uint8_t **payload, size_t *size, uint32_t mode){
    int status = 0;
    apm_module_param_data_t* header;
    uint8_t* payloadInfo = NULL;
    size_t payloadSize = 0, padBytes = 0;
    uint8_t *phrase_pl;
    vcpm_param_id_tty_mode_t tty_payload;

    payloadSize = sizeof(struct apm_module_param_data_t)+
                  sizeof(tty_payload);
    padBytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payloadInfo = new uint8_t[payloadSize + padBytes]();
    if (!payloadInfo) {
        PAL_ERR(LOG_TAG, "payloadInfo malloc failed %s", strerror(errno));
        return -EINVAL;
    }
    header = (apm_module_param_data_t*)payloadInfo;
    header->module_instance_id = VCPM_MODULE_INSTANCE_ID;
    header->param_id = VCPM_PARAM_ID_TTY_MODE;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    tty_payload.vsid = vsid;
    tty_payload.mode = mode;
    phrase_pl = (uint8_t*)payloadInfo + sizeof(apm_module_param_data_t);
    ar_mem_cpy(phrase_pl,  sizeof(vcpm_param_id_tty_mode_t),
                     &tty_payload,  sizeof(vcpm_param_id_tty_mode_t));

    *size = payloadSize + padBytes;
    *payload = payloadInfo;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "vsid: 0x%x, mode: %d", vsid, mode);
#endif
    return status;
}

int SessionAlsaVoice::setSidetone(int deviceId,Stream * s, bool enable){
    int status = 0;
    sidetone_mode_t mode;

    status = rm->getSidetoneMode((pal_device_id_t)deviceId, PAL_STREAM_VOICE_CALL, &mode);
    if(status) {
            PAL_ERR(LOG_TAG, "get sidetone mode failed");
    }
    if (mode == SIDETONE_HW) {
        PAL_DBG(LOG_TAG, "HW sidetone mode being set");
        if (enable) {
            status = setHWSidetone(s,1);
        } else {
            status = setHWSidetone(s,0);
        }
    }
    /*if SW mode it will be set via kv in graph open*/
    return status;
}

int SessionAlsaVoice::setHWSidetone(Stream * s, bool enable){
    int status = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::shared_ptr<Device> rxDevice = nullptr;
    struct audio_route *audioRoute;
    bool set = false;

    status = s->getAssociatedDevices(associatedDevices);
    status = rm->getAudioRoute(&audioRoute);

    if (getRXDevice(s, rxDevice) != 0) {
        PAL_ERR(LOG_TAG, "failed, could not find associated RX device");
        return status;
    }

    status = s->getAssociatedDevices(associatedDevices);
    for(int i =0; i < associatedDevices.size(); i++) {
        switch(associatedDevices[i]->getSndDeviceId()){
            case PAL_DEVICE_IN_HANDSET_MIC:
                if(enable) {
                    if (rxDevice->getSndDeviceId() == PAL_DEVICE_OUT_WIRED_HEADPHONE)
                        audio_route_apply_and_update_path(audioRoute, "sidetone-headphone-handset-mic");
                    else
                        audio_route_apply_and_update_path(audioRoute, "sidetone-handset");
                    sideTone_cnt++;
                } else {
                    if (rxDevice->getSndDeviceId() == PAL_DEVICE_OUT_WIRED_HEADPHONE)
                        audio_route_reset_and_update_path(audioRoute, "sidetone-headphone-handset-mic");
                    else
                        audio_route_reset_and_update_path(audioRoute, "sidetone-handset");
                    sideTone_cnt--;
                }
                set = true;
                break;
            case PAL_DEVICE_IN_WIRED_HEADSET:
                if(enable) {
                    audio_route_apply_and_update_path(audioRoute, "sidetone-headphones");
                    sideTone_cnt++;
                } else {
                    audio_route_reset_and_update_path(audioRoute, "sidetone-headphones");
                    sideTone_cnt--;
                }
                set = true;
                break;
            default:
                PAL_DBG(LOG_TAG,"codec sidetone not supported on device %d",associatedDevices[i]->getSndDeviceId());
                break;

        }
        if(set)
            break;
    }
    return status;
}

int SessionAlsaVoice::disconnectSessionDevice(Stream *streamHandle,
                                              pal_stream_type_t streamType,
                                              std::shared_ptr<Device> deviceToDisconnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    std::vector<std::string> aifBackEndsToDisconnect;
    struct pal_device dAttr;
    int status = 0;
    int txDevId = PAL_DEVICE_NONE;

    deviceList.push_back(deviceToDisconnect);
    rm->getBackEndNames(deviceList, rxAifBackEnds,txAifBackEnds);

    deviceToDisconnect->getDeviceAttributes(&dAttr);

    if (rxAifBackEnds.size() > 0) {
        /*config mute on pop suppressor*/
        if (streamHandle->getCurState() != STREAM_INIT) {
            setPopSuppressorMute(streamHandle);
            usleep(POP_SUPPRESSOR_RAMP_DELAY);
        }

        /*if HW sidetone is enable disable it */
        if (sideTone_cnt > 0) {
            status = getTXDeviceId(streamHandle, &txDevId);
            if (status){
                PAL_ERR(LOG_TAG, "could not find TX device associated with this stream cannot set sidetone");
            } else {
                status = setSidetone(txDevId,streamHandle,0);
                if(0 != status) {
                   PAL_ERR(LOG_TAG,"disabling sidetone failed");
                }
            }
        }
        status =  SessionAlsaUtils::disconnectSessionDevice(streamHandle,
                                                            streamType, rm,
                                                            dAttr, pcmDevRxIds,
                                                            rxAifBackEnds);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"disconnectSessionDevice on RX Failed \n");
            return status;
        }
    } else if (txAifBackEnds.size() > 0) {
        /*if HW sidetone is enable disable it */
        if (sideTone_cnt > 0) {
            status = getTXDeviceId(streamHandle, &txDevId);
            if (status){
                PAL_ERR(LOG_TAG, "could not find TX device associated with this stream cannot set sidetone");
            } else {
                status = setSidetone(txDevId,streamHandle,0);
                if(0 != status) {
                   PAL_ERR(LOG_TAG,"disabling sidetone failed");
                }
            }
        }
        status =  SessionAlsaUtils::disconnectSessionDevice(streamHandle,
                                                            streamType, rm,
                                                            dAttr, pcmDevTxIds,
                                                            txAifBackEnds);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"disconnectSessionDevice on TX Failed");
        }
    }

    /*teardown external ec if needed*/
    if (SessionAlsaUtils::isRxDevice(dAttr.id)) {
        setExtECRef(streamHandle,deviceToDisconnect,false);
    }

    return status;
}

int SessionAlsaVoice::setupSessionDevice(Stream* streamHandle,
                                 pal_stream_type_t streamType,
                                 std::shared_ptr<Device> deviceToConnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    std::vector<std::string> aifBackEndsToConnect;
    struct pal_device dAttr;
    int status = 0;

    deviceList.push_back(deviceToConnect);
    rm->getBackEndNames(deviceList, rxAifBackEnds, txAifBackEnds);
    deviceToConnect->getDeviceAttributes(&dAttr);

    /*setup external ec if needed*/
    if (SessionAlsaUtils::isRxDevice(dAttr.id)) {
        setExtECRef(streamHandle,deviceToConnect,true);
    }

    if (rxAifBackEnds.size() > 0) {
        status =  SessionAlsaUtils::setupSessionDevice(streamHandle, streamType,
                                                       rm, dAttr, pcmDevRxIds,
                                                       rxAifBackEnds);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"setupSessionDevice on RX Failed");
            return status;
        }
    } else if (txAifBackEnds.size() > 0) {
        status =  SessionAlsaUtils::setupSessionDevice(streamHandle, streamType,
                                                       rm, dAttr, pcmDevTxIds,
                                                       txAifBackEnds);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"setupSessionDevice on TX Failed");
        }
    }
    return status;
}

int SessionAlsaVoice::connectSessionDevice(Stream* streamHandle,
                                           pal_stream_type_t streamType,
                                           std::shared_ptr<Device> deviceToConnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    std::vector<std::string> aifBackEndsToConnect;
    std::shared_ptr<Device> rxDevice = nullptr;
    struct pal_device dAttr;
    int status = 0;
    int txDevId = PAL_DEVICE_NONE;

    deviceList.push_back(deviceToConnect);
    rm->getBackEndNames(deviceList, rxAifBackEnds, txAifBackEnds);
    deviceToConnect->getDeviceAttributes(&dAttr);

    if (rxAifBackEnds.size() > 0) {
        setTaggedSlotMask(streamHandle);
        status = SessionAlsaUtils::connectSessionDevice(this, streamHandle,
                                                         streamType, rm,
                                                         dAttr, pcmDevRxIds,
                                                         rxAifBackEnds);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"connectSessionDevice on RX Failed");
            return status;
        }

        if(sideTone_cnt == 0) {
           if (deviceToConnect->getSndDeviceId() == PAL_DEVICE_OUT_HANDSET ||
               deviceToConnect->getSndDeviceId() == PAL_DEVICE_OUT_WIRED_HEADSET ||
               deviceToConnect->getSndDeviceId() == PAL_DEVICE_OUT_WIRED_HEADPHONE ||
               deviceToConnect->getSndDeviceId() == PAL_DEVICE_OUT_USB_DEVICE ||
               deviceToConnect->getSndDeviceId() == PAL_DEVICE_OUT_USB_HEADSET) {
               // set sidetone on new tx device after pcm_start
               status = getTXDeviceId(streamHandle, &txDevId);
               if (status){
                   PAL_ERR(LOG_TAG,"could not find TX device associated with this stream\n");
               }
               if (txDevId != PAL_DEVICE_NONE) {
                   status = setSidetone(txDevId, streamHandle, 1);
               }
               if (0 != status) {
                   PAL_ERR(LOG_TAG,"enabling sidetone failed");
               }
           }
        }
        if (rm->isCRSCallEnabled) {
            status = populate_rx_mfc_coeff_payload(deviceToConnect);
            if (status != 0) {
                PAL_ERR(LOG_TAG,"populating Rx mfc coeff payload failed :%d", status);
            }
        }
    } else if (txAifBackEnds.size() > 0) {
        status =  SessionAlsaUtils::connectSessionDevice(this, streamHandle,
                                                         streamType, rm,
                                                         dAttr, pcmDevTxIds,
                                                         txAifBackEnds);
        if(0 != status) {
            PAL_ERR(LOG_TAG,"connectSessionDevice on TX Failed");
            return status;
        }

        if(sideTone_cnt == 0) {
           if (deviceToConnect->getSndDeviceId() > PAL_DEVICE_IN_MIN &&
               deviceToConnect->getSndDeviceId() < PAL_DEVICE_IN_MAX) {
               txDevId = deviceToConnect->getSndDeviceId();
           }
           if (getRXDevice(streamHandle, rxDevice) != 0) {
               PAL_DBG(LOG_TAG,"no active rx device, no need to setSidetone");
               return status;
           } else if (rxDevice && rxDevice->getDeviceCount() != 0 &&
                      txDevId != PAL_DEVICE_NONE) {
               status = setSidetone(txDevId, streamHandle, 1);
           }
           if (0 != status) {
               PAL_ERR(LOG_TAG,"enabling sidetone failed");
           }
        }
        if (rm->isCRSCallEnabled) {
            status = populate_rx_mfc_coeff_payload(rxDevice);
            if (status != 0) {
                PAL_ERR(LOG_TAG,"populating Rx mfc coeff payload failed :%d", status);
            }
        }
    }
    return status;
}

int SessionAlsaVoice::setVoiceMixerParameter(Stream * s, struct mixer *mixer,
                                             void *payload, int size, int dir)
{
    char *control = (char*)"setParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    struct pal_stream_attributes sAttr;
    char *stream = SessionAlsaVoice::getMixerVoiceStream(s, dir);

    ret = s->getStreamAttributes(&sAttr);

    if (ret) {
         PAL_ERR(LOG_TAG, "could not get stream attributes\n");
        return ret;
    }

    ctl_len = strlen(stream) + 4 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str) {
        free(payload);
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", stream, control);

    PAL_VERBOSE(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }


    ret = mixer_ctl_set_array(ctl, payload, size);

    PAL_VERBOSE(LOG_TAG, "ret = %d, cnt = %d\n", ret, size);
    free(mixer_str);
    return ret;
}

char* SessionAlsaVoice::getMixerVoiceStream(Stream *s, int dir){
    char *stream = (char*)"VOICEMMODE1p";
    struct pal_stream_attributes sAttr;

    s->getStreamAttributes(&sAttr);
    if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
        sAttr.info.voice_call_info.VSID == VOICELBMMODE1) {
        if (dir == TX_HOSTLESS) {
            stream = (char*)"VOICEMMODE1c";
        } else {
            stream = (char*)"VOICEMMODE1p";
        }
    } else {
        if (dir == TX_HOSTLESS) {
            stream = (char*)"VOICEMMODE2c";
        } else {
            stream = (char*)"VOICEMMODE2p";
        }
    }

    return stream;
}

int SessionAlsaVoice::setExtECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct pal_device rxDevAttr = {};
    struct pal_device_info rxDevInfo = {};

    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream");
        status = -EINVAL;
        goto exit;
    }

    status = s->getStreamAttributes(&sAttr);
    if(0 != status) {
       PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
       goto exit;
    }

    rxDevInfo.isExternalECRefEnabledFlag = 0;
    if (rx_dev) {
        status = rx_dev->getDeviceAttributes(&rxDevAttr, s);
        if (status != 0) {
            PAL_ERR(LOG_TAG," get device attributes failed");
            goto exit;
        }
        rm->getDeviceInfo(rxDevAttr.id, sAttr.type, rxDevAttr.custom_config.custom_key, &rxDevInfo);
    }

    if (rxDevInfo.isExternalECRefEnabledFlag) {
        status = checkAndSetExtEC(rm, s, is_enable);
        if (status)
            PAL_ERR(LOG_TAG,"Failed to enable Ext EC for voice");
    }

exit:
    return status;
}

int SessionAlsaVoice::getTXDeviceId(Stream *s, int *id)
{
    int status = 0;
    int i;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    *id = PAL_DEVICE_NONE;

    status = s->getAssociatedDevices(associatedDevices);
    if(0 != status) {
        PAL_ERR(LOG_TAG,"getAssociatedDevices Failed");
        return status;
    }

    for (i =0; i < associatedDevices.size(); i++) {
        if (associatedDevices[i]->getSndDeviceId() > PAL_DEVICE_IN_MIN &&
            associatedDevices[i]->getSndDeviceId() < PAL_DEVICE_IN_MAX) {
            *id = associatedDevices[i]->getSndDeviceId();
            break;
        }
    }
    if(i >= PAL_DEVICE_IN_MAX){
        status = -EINVAL;
    }
    return status;
}

int SessionAlsaVoice::getRXDevice(Stream *s, std::shared_ptr<Device> &rx_dev)
{
    int status = 0;
    int i;
    std::vector<std::shared_ptr<Device>> associatedDevices;

    rx_dev = nullptr;
    status = s->getAssociatedDevices(associatedDevices);
    if(0 != status) {
        PAL_ERR(LOG_TAG,"getAssociatedDevices Failed");
        return status;
    }

    for (i = 0; i < associatedDevices.size(); i++) {
        if (associatedDevices[i]->getSndDeviceId() > PAL_DEVICE_OUT_MIN &&
            associatedDevices[i]->getSndDeviceId() < PAL_DEVICE_OUT_MAX) {
            rx_dev = associatedDevices[i];
            break;
        }
    }
    if(rx_dev == nullptr) {
        status = -EINVAL;
    }
    return status;
}

int SessionAlsaVoice::setPopSuppressorMute(Stream *s)
{
    int status = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    uint32_t miid = 0;

    if (!rxAifBackEnds.size()) {
        PAL_ERR(LOG_TAG, "No RX backends found");
        status = -EINVAL;
        goto exit;
    }

    if (!pcmDevRxIds.size()) {
        PAL_ERR(LOG_TAG, "No pcmDevRxIds found");
        status = -EINVAL;
        goto exit;
    }

    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevRxIds.at(0),
                                                   rxAifBackEnds[0].second.c_str(),
                                                   DEVICE_POP_SUPPRESSOR, &miid);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"getModuleInstanceId failed for Rx pop suppressor: 0x%x status: %d",
            DEVICE_POP_SUPPRESSOR, status);
        goto exit;
    }

    if (!builder) {
        PAL_ERR(LOG_TAG,"failed: builder instance not found")
        status = -EINVAL;
        goto exit;
    }

    status = builder->payloadPopSuppressorConfig((uint8_t**)&payload, &payloadSize, miid, true);
    if (status) {
        PAL_ERR(LOG_TAG,"pop suppressor payload creation failed: status: %d",
                status);
        goto exit;
    }

    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevRxIds.at(0),
                                                 payload, payloadSize);
    if (status) {
        PAL_ERR(LOG_TAG,"setMixerParameter failed");
    }
exit:
    if (payload)
        free(payload);
    return status;
}

