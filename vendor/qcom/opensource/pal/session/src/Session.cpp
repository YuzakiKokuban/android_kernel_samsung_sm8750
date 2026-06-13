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
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "PAL: Session"

#include "Session.h"
#include "Stream.h"
#include "ResourceManager.h"
#include "SessionGsl.h"
#include "SessionAlsaPcm.h"
#include "SessionAlsaCompress.h"
#include "SessionAgm.h"
#include "SessionAlsaUtils.h"
#include "SessionAlsaVoice.h"

#include <sstream>

#ifndef SEC_AUDIO_MUTE_DETECTION
#include "hw_intf_cmn_api.h" // move to Session.h
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/klog.h>        /* Definition of SYSLOG_* constants */
#include <time.h>

#ifdef SEC_AUDIO_MUTE_DETECTION
#include <audioInfo.h>
#include <SecAudioHalProxy.h>
#endif

#ifndef DUMP_OUT_PATH
#define DUMP_OUT_PATH "/data/vendor/audio/"
#endif //DUMP_OUT_PATH

#define MAX_DUMP_FILENAME_SIZE 255
#define REGDUMP_OUT_SIZE 256*1024
#define SYSLOG_ACTION_READ_ALL 3
#define SYSLOG_ACTION_SIZE_BUFFER 10
#define TIMESTAMP_FORMAT_STRING "_%Y_%m_%d_%H_%M_%S"

#define BOLERO_PROC_INTF    "/proc/lpass_cdc_reginfo/lpass_cdc_regdump"
#define WCD939X_PROC_INTF   "/proc/wcd939x_reginfo/wcd939x_regdump"
#define WSA884X_1_PROC_INTF "/proc/wsa884x_reginfo_1/wsa884x_regdump"
#define WSA884X_2_PROC_INTF "/proc/wsa884x_reginfo_2/wsa884x_regdump"
#define WSA883X_1_PROC_INTF "/proc/wsa883x_reginfo_1/wsa883x_regdump"
#define WSA883X_2_PROC_INTF "/proc/wsa883x_reginfo_2/wsa883x_regdump"
#define WSA_SWR_PROC_INTF  "/proc/wsa_swr_ctrl/swr_mstr_ctrl_regdump"
#define WSA2_SWR_PROC_INTF "/proc/wsa2_swr_ctrl/swr_mstr_ctrl_regdump"
#define VA_SWR_PROC_INTF   "/proc/va_swr_ctrl/swr_mstr_ctrl_regdump"
#define RX_SWR_PROC_INTF   "/proc/rx_swr_ctrl/swr_mstr_ctrl_regdump"

#define KMSG_FILE     "kernel_log"
#define SILENCE_EVENT_INFO DUMP_OUT_PATH "silence_event_info"
#define BOLERO_REGDUMP_OUT_FILE          "lpass_cdc_regdump"
#define WCD939X_REGDUMP_OUT_FILE         "wcd939x_regdump"
#define VA_SWR_REGDUMP_OUT_FILE          "va_swr_regdump"

#define KMSG_OUT_FILE              DUMP_OUT_PATH KMSG_FILE TIMESTAMP_FORMAT_STRING
#define SILENCE_EVENT_INFO         DUMP_OUT_PATH "silence_event_info" TIMESTAMP_FORMAT_STRING
#define BOLERO_REGDUMP_OUT_PATH    DUMP_OUT_PATH BOLERO_REGDUMP_OUT_FILE TIMESTAMP_FORMAT_STRING
#define WCD939X_REGDUMP_OUT_PATH   DUMP_OUT_PATH WCD939X_REGDUMP_OUT_FILE TIMESTAMP_FORMAT_STRING
#define VA_SWR_REGDUM_OUT_PATH     DUMP_OUT_PATH VA_SWR_REGDUMP_OUT_FILE TIMESTAMP_FORMAT_STRING

/* Forward Declaration for Silence Detection Callback */
void handleSilenceDetectionCb(uint64_t hdl __unused,
                uint32_t event_id, void *event_data, uint32_t event_size);

struct pcm *Session::pcmEcTx = NULL;
std::vector<int> Session::pcmDevEcTxIds = {0};
int Session::extECRefCnt = 0;
std::mutex Session::extECMutex;

#ifdef SEC_AUDIO_OFFLOAD
#define TAG_COMPRESSED_OFFLOAD_PRE_MFC 0x0C00000C
#endif

Session::Session()
{
    isMixerEventCbRegd = false;
    isPauseRegistrationDone = false;
    silenceEventRegistered = false;
#ifdef SEC_AUDIO_MUTE_DETECTION
    silenceEventRegisteredForMcst = false;
#endif
}

Session::~Session()
{

}

void Session::setPmQosMixerCtl(pmQosVote vote)
{
    int status = 0;
    struct audio_route *audioRoute;

    status = rm->getAudioRoute(&audioRoute);
    if (!status) {
        if (vote == PM_QOS_VOTE_DISABLE) {
            audio_route_reset_and_update_path(audioRoute, "PM_QOS_Vote");
            PAL_DBG(LOG_TAG,"mixer control disabled for PM_QOS Vote \n");
        } else if (vote == PM_QOS_VOTE_ENABLE) {
            audio_route_apply_and_update_path(audioRoute, "PM_QOS_Vote");
            PAL_DBG(LOG_TAG,"mixer control enabled for PM_QOS Vote \n");
        }
    } else {
        PAL_ERR(LOG_TAG,"could not get audioRoute, not setting mixer control for PM_QOS \n");
    }
}

Session* Session::makeSession(const std::shared_ptr<ResourceManager>& rm, const struct pal_stream_attributes *sAttr)
{
    if (!rm || !sAttr) {
        PAL_ERR(LOG_TAG, "Invalid parameters passed");
        return nullptr;
    }

    Session* s = (Session*) nullptr;

    switch (sAttr->type) {
        //create compressed if the stream type is compressed
        case PAL_STREAM_COMPRESSED:
            s =  new SessionAlsaCompress(rm);
            break;
        case PAL_STREAM_VOICE_CALL:
            s = new SessionAlsaVoice(rm);
            break;
        case PAL_STREAM_NON_TUNNEL:
            s = new SessionAgm(rm);
            break;
        default:
            s = new SessionAlsaPcm(rm);
            break;
    }
    return s;
}

Session* Session::makeACDBSession(const std::shared_ptr<ResourceManager>& rm,
                                    const struct pal_stream_attributes *sAttr)
{
    if (!rm || !sAttr) {
        PAL_ERR(LOG_TAG,"Invalid parameters passed");
        return nullptr;
    }

    Session* s = (Session*) nullptr;
    s = new SessionAgm(rm);

    return s;
}

void Session::getSamplerateChannelBitwidthTags(struct pal_media_config *config,
        uint32_t &mfc_sr_tag, uint32_t &ch_tag, uint32_t &bitwidth_tag)
{
    switch (config->sample_rate) {
        case SAMPLINGRATE_8K :
            mfc_sr_tag = MFC_SR_8K;
            break;
        case SAMPLINGRATE_16K :
            mfc_sr_tag = MFC_SR_16K;
            break;
        case SAMPLINGRATE_32K :
            mfc_sr_tag = MFC_SR_32K;
            break;
        case SAMPLINGRATE_44K :
            mfc_sr_tag = MFC_SR_44K;
            break;
        case SAMPLINGRATE_48K :
            mfc_sr_tag = MFC_SR_48K;
            break;
        case SAMPLINGRATE_96K :
            mfc_sr_tag = MFC_SR_96K;
            break;
        case SAMPLINGRATE_192K :
            mfc_sr_tag = MFC_SR_192K;
            break;
        case SAMPLINGRATE_384K :
            mfc_sr_tag = MFC_SR_384K;
            break;
        default:
            mfc_sr_tag = MFC_SR_48K;
            break;
    }
    switch (config->ch_info.channels) {
        case CHANNELS_1:
            ch_tag = CHS_1;
            break;
        case CHANNELS_2:
            ch_tag = CHS_2;
            break;
        case CHANNELS_3:
            ch_tag = CHS_3;
            break;
        case CHANNELS_4:
            ch_tag = CHS_4;
            break;
        case CHANNELS_5:
            ch_tag = CHS_5;
            break;
        case CHANNELS_6:
            ch_tag = CHS_6;
            break;
        case CHANNELS_7:
            ch_tag = CHS_7;
            break;
        case CHANNELS_8:
            ch_tag = CHS_8;
            break;
        default:
            ch_tag = CHS_1;
            break;
    }
    switch (config->bit_width) {
        case BITWIDTH_16:
            bitwidth_tag = BW_16;
            break;
        case BITWIDTH_24:
            bitwidth_tag = BW_24;
            break;
        case BITWIDTH_32:
            bitwidth_tag = BW_32;
            break;
        default:
            bitwidth_tag = BW_16;
            break;
    }
}

uint32_t Session::getModuleInfo(const char *control, uint32_t tagId, uint32_t *miid, struct mixer_ctl **ctl, int *device)
{
    int status = 0;
    int dev = 0;
    struct mixer_ctl *mixer_ctl = NULL;

    if (!rxAifBackEnds.empty()) { /** search in RX GKV */
        mixer_ctl = getFEMixerCtl(control, &dev, PAL_AUDIO_OUTPUT);
        if (!mixer_ctl) {
            PAL_ERR(LOG_TAG, "Invalid mixer control\n");
            status = -ENOENT;
            goto exit;
        }
        for (int i = 0; i < rxAifBackEnds.size(); i++) {
            status = SessionAlsaUtils::getModuleInstanceId(mixer, dev, rxAifBackEnds[i].second.data(), tagId, miid);
            if (status) /** if not found, reset miid to 0 again */
                *miid = 0;
            else
                break;
        }
    }

    if (!txAifBackEnds.empty() && !(*miid)) { /** search in TX GKV */
        mixer_ctl = getFEMixerCtl(control, &dev, PAL_AUDIO_INPUT);
        if (!mixer_ctl) {
            PAL_ERR(LOG_TAG, "Invalid mixer control\n");
            status = -ENOENT;
            goto exit;
        }
        for (int i = 0; i < txAifBackEnds.size(); i++) {
            status = SessionAlsaUtils::getModuleInstanceId(mixer, dev, txAifBackEnds[i].second.data(), tagId, miid);
            if (status)
                *miid = 0;
            else
                break;
        }
    }

    if (*miid == 0) {
        PAL_ERR(LOG_TAG, "failed to look for module with tagID 0x%x", tagId);
        status = -EINVAL;
        goto exit;
    }

    if (device)
        *device = dev;

    if (ctl)
        *ctl = mixer_ctl;

    PAL_DBG(LOG_TAG, "got miid = 0x%04x, device = %d", *miid, dev);
exit:
    if (status) {
        if (device)
            *device = 0;
        if (ctl)
            *ctl = NULL;
        *miid = 0;
        PAL_ERR(LOG_TAG, "Exit. status %d", status);
    }
    return status;
}

int Session::getEffectParameters(Stream *s __unused, effect_pal_payload_t *effectPayload)
{
    int status = 0;
    uint8_t *ptr = NULL;

    uint8_t *payloadData = NULL;
    size_t payloadSize = 0;
    uint32_t miid = 0;
    const char *control = "getParam";
    struct mixer_ctl *ctl = NULL;
    pal_effect_custom_payload_t *effectCustomPayload = nullptr;
    PayloadBuilder builder;

    PAL_DBG(LOG_TAG, "Enter.");

    status = getModuleInfo(control, effectPayload->tag, &miid, &ctl, NULL);
    if (status || !miid) {
        PAL_ERR(LOG_TAG, "failed to look for module with tagID 0x%x, status = %d",
                    effectPayload->tag, status);
        status = -EINVAL;
        goto exit;
    }

    effectCustomPayload = (pal_effect_custom_payload_t *)(effectPayload->payload);
    if (effectPayload->payloadSize < sizeof(pal_effect_custom_payload_t)) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "memory for retrieved data is too small");
        goto exit;
    }

    builder.payloadQuery(&payloadData, &payloadSize,
                            miid, effectCustomPayload->paramId,
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
    ar_mem_cpy(effectCustomPayload->data, effectPayload->payloadSize,
                        ptr, effectPayload->payloadSize);

exit:
    ctl = NULL;
    if (payloadData)
        free(payloadData);
    PAL_ERR(LOG_TAG, "Exit. status %d", status);
    return status;
}

int Session::setEffectParametersTKV(Stream *s __unused, effect_pal_payload_t *effectPayload)
{
    int status = 0;
    int device = 0;
    uint32_t tag;
    uint32_t nTkvs;
    uint32_t tagConfigSize;
    struct mixer_ctl *ctl;
    pal_key_vector_t *palKVPair;
    struct agm_tag_config* tagConfig = NULL;
    std::vector <std::pair<int, int>> tkv;
    const char *control = "setParamTag";

    PAL_DBG(LOG_TAG, "Enter.");

    palKVPair = (pal_key_vector_t *)effectPayload->payload;
    nTkvs =  palKVPair->num_tkvs;
    tkv.clear();
    for (int i = 0; i < nTkvs; i++) {
        tkv.push_back(std::make_pair(palKVPair->kvp[i].key, palKVPair->kvp[i].value));
    }
    if (tkv.size() == 0) {
        status = -EINVAL;
        goto exit;
    }

    tagConfigSize = sizeof(struct agm_tag_config) + (tkv.size() * sizeof(agm_key_value));
    tagConfig = (struct agm_tag_config *) malloc(tagConfigSize);
    if(!tagConfig) {
        status = -ENOMEM;
        goto exit;
    }

    tag = effectPayload->tag;
    status = SessionAlsaUtils::getTagMetadata(tag, tkv, tagConfig);
    if (0 != status) {
        goto exit;
    }

    /* Prepare mixer control */
    ctl = getFEMixerCtl(control, &device, PAL_AUDIO_OUTPUT);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control\n");
        status = -ENOENT;
        goto exit;
    }

    status = mixer_ctl_set_array(ctl, tagConfig, tagConfigSize);
    if (status != 0) {
        PAL_DBG(LOG_TAG, "Unable to set TKV in Rx path, trying in Tx\n");
        /* Rx set failed, Try on Tx path we well */
        ctl = getFEMixerCtl(control, &device, PAL_AUDIO_INPUT);
        if (!ctl) {
            PAL_ERR(LOG_TAG, "Invalid mixer control\n");
            status = -ENOENT;
            goto exit;
        }

        status = mixer_ctl_set_array(ctl, tagConfig, tagConfigSize);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "failed to set the param %d", status);
            goto exit;
        }
    }

exit:
    ctl = NULL;

    if (tagConfig) {
        free(tagConfig);
        tagConfig = NULL;
    }
    PAL_INFO(LOG_TAG, "mixer set tkv status = %d\n", status);
    return status;
}

int Session::setEffectParametersNonTKV(Stream *s __unused, effect_pal_payload_t *effectPayload)
{
    int status = 0;
    int device = 0;
    PayloadBuilder builder;

    uint32_t miid = 0;
    const char *control = "setParam";
    size_t payloadSize = 0;
    uint8_t *payloadData = NULL;
    pal_effect_custom_payload_t *effectCustomPayload = nullptr;

    PAL_DBG(LOG_TAG, "Enter.");

    if (!effectPayload) {
        PAL_ERR(LOG_TAG, "Invalid effectPayload address.\n");
        return -EINVAL;
    }

    /* This is set param call, find out miid first */
    status = getModuleInfo(control, effectPayload->tag, &miid, NULL, &device);
    if (status || !miid) {
        PAL_ERR(LOG_TAG, "failed to look for module with tagID 0x%x, status = %d",
                    effectPayload->tag, status);
        return -EINVAL;
    }

    /* Now we got the miid, build set param payload */
    effectCustomPayload = (pal_effect_custom_payload_t *)effectPayload->payload;
    status = builder.payloadCustomParam(&payloadData, &payloadSize,
            effectCustomPayload->data,
            effectPayload->payloadSize - sizeof(uint32_t),
            miid, effectCustomPayload->paramId);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "payloadCustomParam failed. status = %d",
                status);
        goto exit;
    }
    /* set param through set mixer param */
    status = SessionAlsaUtils::setMixerParameter(mixer,
            device,
            payloadData,
            payloadSize);
    PAL_INFO(LOG_TAG, "mixer set param status = %d\n", status);

exit:
    if (payloadData) {
        free(payloadData);
        payloadData = NULL;
    }
    if (status && effectCustomPayload) {
        PAL_ERR(LOG_TAG, "setEffectParameters for param_id %d failed, status = %d",
                effectCustomPayload->paramId, status);
    }
    return status;

}

int Session::setEffectParameters(Stream *s, effect_pal_payload_t *effectPayload)
{
    int status = 0;

    PAL_DBG(LOG_TAG, "Enter.");

    /* Identify whether this is tkv or set param call */
    if (effectPayload->isTKV) {
        /* This is tkv set call */
        status = setEffectParametersTKV(s, effectPayload);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Get setEffectParameters with TKV payload failed"
                                ", status = %d", status);
            goto exit;
        }
    } else {
        status = setEffectParametersNonTKV(s, effectPayload);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Get setEffectParameters with non TKV payload failed"
                                ", status = %d", status);
            goto exit;
        }
    }

exit:
    if (status)
        PAL_ERR(LOG_TAG, "Exit. status %d", status);

    return status;
}

int Session::rwACDBParameters(void *payload, uint32_t sampleRate,
                                bool isParamWrite)
{
    int status = 0;
    uint8_t *payloadData = NULL;
    size_t payloadSize = 0;
    uint32_t miid = 0;
    char const *control = "setParamTagACDB";
    struct mixer_ctl *ctl = NULL;
    pal_effect_custom_payload_t *effectCustomPayload = nullptr;
    PayloadBuilder builder;
    pal_param_payload *paramPayload = nullptr;
    agm_acdb_param *effectACDBPayload = nullptr;

    paramPayload = (pal_param_payload *)payload;
    if (!paramPayload)
        return -EINVAL;

    effectACDBPayload = (agm_acdb_param *)(paramPayload->payload);
    if (!effectACDBPayload)
        return -EINVAL;

    PAL_DBG(LOG_TAG, "Enter.");

    status = getModuleInfo(control, effectACDBPayload->tag, &miid, &ctl, NULL);
    if (status || !miid) {
        PAL_ERR(LOG_TAG, "failed to look for module with tagID 0x%x, status = %d",
                    effectACDBPayload->tag, status);
        status = -EINVAL;
        goto exit;
    }

    effectCustomPayload =
        (pal_effect_custom_payload_t *)(effectACDBPayload->blob);

    status = builder.payloadACDBParam(&payloadData, &payloadSize,
                            (uint8_t *)effectACDBPayload,
                            miid, sampleRate);
    if (!payloadData) {
        PAL_ERR(LOG_TAG, "failed to create payload data.");
        goto exit;
    }

   if (isParamWrite) {
        status = mixer_ctl_set_array(ctl, payloadData, payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Set custom config failed, status = %d", status);
            goto exit;
        }
    }

exit:
    ctl = NULL;
    free(payloadData);
    PAL_ERR(LOG_TAG, "Exit. status %d", status);
    return status;
}

int Session::rwACDBParamTunnel(void *payload, pal_device_id_t palDeviceId,
                        pal_stream_type_t palStreamType, uint32_t sampleRate,
                        uint32_t instanceId, bool isParamWrite, Stream * s)
{
    int status = -EINVAL;
    struct pal_stream_attributes sAttr = {};

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    streamHandle = s;
    if (0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        goto exit;
    }

    PAL_INFO(LOG_TAG, "PAL device id=0x%x", palDeviceId);
    status = SessionAlsaUtils::rwACDBTunnel(s, rm, palDeviceId, payload, isParamWrite, instanceId);
    if (status) {
        PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
    }

exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}


int Session::updateCustomPayload(void *payload, size_t size)
{
    if (!customPayloadSize || !customPayload) {
        customPayload = calloc(1, size);
    } else {
        customPayload = realloc(customPayload, customPayloadSize + size);
    }

    if (!customPayload) {
        PAL_ERR(LOG_TAG, "failed to allocate memory for custom payload");
        return -ENOMEM;
    }

    memcpy((uint8_t *)customPayload + customPayloadSize, payload, size);
    customPayloadSize += size;
    PAL_INFO(LOG_TAG, "customPayloadSize = %zu", customPayloadSize);
    return 0;
}

int Session::getCustomPayload(uint8_t **payload, size_t *payloadSize)
{
    if (customPayloadSize) {
        *payload = (uint8_t *)customPayload;
        *payloadSize = customPayloadSize;
    }
    return 0;
}

int Session::freeCustomPayload(uint8_t **payload, size_t *payloadSize)
{
    if (*payload) {
        free(*payload);
        *payload = NULL;
        *payloadSize = 0;
    }
    return 0;
}

int Session::freeCustomPayload()
{
    if (customPayload) {
        free(customPayload);
        customPayload = NULL;
        customPayloadSize = 0;
    }
    return 0;
}

int Session::pause(Stream * s __unused)
{
    return 0;
}

int Session::resume(Stream * s __unused)
{
     return 0;
}

int Session::handleDeviceRotation(Stream *s, pal_speaker_rotation_type rotation_type,
        int device, struct mixer *mixer, PayloadBuilder* builder,
        std::vector<std::pair<int32_t, std::string>> rxAifBackEnds)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct pal_device dAttr = {};
    uint32_t miid = 0;
    uint8_t* alsaParamData = NULL;
    size_t alsaPayloadSize = 0;
    int mfc_tag = TAG_MFC_SPEAKER_SWAP;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    if (PAL_AUDIO_OUTPUT== sAttr.direction) {
        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed\n");
            return status;
        }

        for (int i = 0; i < associatedDevices.size(); i++) {
             status = associatedDevices[i]->getDeviceAttributes(&dAttr);
             if (0 != status) {
                 PAL_ERR(LOG_TAG, "get Device Attributes Failed\n");
                 return status;
             }

             if ((PAL_DEVICE_OUT_SPEAKER == dAttr.id) &&
                  (2 == dAttr.config.ch_info.channels) &&
                  (strcmp(dAttr.custom_config.custom_key, "mspp") != 0)) {
                 /* Get DevicePP MFC MIID and configure to match to device config */
                 /* This has to be done after sending all mixer controls and
                  * before connect
                  */
                status =
                        SessionAlsaUtils::getModuleInstanceId(mixer,
                                                              device,
                                                              rxAifBackEnds[i].second.data(),
                                                              mfc_tag, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                    return status;
                }
                PAL_DBG(LOG_TAG, "miid : %x id = %d, data %s, dev id = %d\n", miid,
                    device, rxAifBackEnds[i].second.data(), dAttr.id);

                if (rm->activeGroupDevConfig) {
                    if (rm->activeGroupDevConfig->devpp_mfc_cfg.channels)
                        dAttr.config.ch_info.channels =rm->activeGroupDevConfig->devpp_mfc_cfg.channels;
                }
                builder->payloadMFCMixerCoeff((uint8_t **)&alsaParamData,
                                            &alsaPayloadSize, miid,
                                            dAttr.config.ch_info.channels,
                                            rotation_type);

                if (alsaPayloadSize) {
                    status = updateCustomPayload(alsaParamData, alsaPayloadSize);
                    freeCustomPayload(&alsaParamData, &alsaPayloadSize);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                        return status;
                    }
                }
                status = SessionAlsaUtils::setMixerParameter(mixer,
                                                             device,
                                                             customPayload,
                                                             customPayloadSize);
                freeCustomPayload();
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    return status;
                }
            }
        }
    }
    return status;
}

int Session::HDRConfigKeyToDevOrientation(const char* hdr_custom_key)
{
    if (!strcmp(hdr_custom_key, "unprocessed-hdr-mic-portrait"))
        return ORIENTATION_0;
    else if (!strcmp(hdr_custom_key, "unprocessed-hdr-mic-landscape"))
        return ORIENTATION_90;
    else if (!strcmp(hdr_custom_key, "unprocessed-hdr-mic-inverted-portrait"))
        return ORIENTATION_180;
    else if (!strcmp(hdr_custom_key, "unprocessed-hdr-mic-inverted-landscape"))
        return ORIENTATION_270;

    PAL_DBG(LOG_TAG,"unknown device orientation %s for HDR record",hdr_custom_key);
    return ORIENTATION_0;
}

/* This set slot mask tag for device with virtual port enabled */
int Session::setSlotMask(const std::shared_ptr<ResourceManager>& rm, struct pal_stream_attributes &sAttr,
            struct pal_device &dAttr, const std::vector<int> &pcmDevIds)
{
    int status = 0;
    std::vector <std::pair<int, int>> tkv;
    struct agm_tag_config* tagConfig = NULL;
    const char *setParamTagControl = " setParamTag";
    const char *streamPcm = "PCM";
    const char *streamComp = "COMPRESS";
    const char *streamVoice = "VOICEMMODE";
    const char *feCtl = " control";
    struct mixer_ctl *ctl;
    std::ostringstream tagCntrlName;
    std::ostringstream feName;
    std::string backendname;
    int tkv_size = 0;
    uint32_t slot_mask = 0;

    if (rm->activeGroupDevConfig) {
        if (0 == rm->activeGroupDevConfig->grp_dev_hwep_cfg.slot_mask) {
            slot_mask = slotMaskLUT.at(dAttr.config.ch_info.channels) |
                            slotMaskBwLUT.at(dAttr.config.bit_width);
            tkv.push_back(std::make_pair(TAG_KEY_SLOT_MASK, slot_mask));
        } else {
            tkv.push_back(std::make_pair(TAG_KEY_SLOT_MASK,
                          rm->activeGroupDevConfig->grp_dev_hwep_cfg.slot_mask));
        }
    } else if (rm->isDeviceMuxConfigEnabled) {
         slot_mask = slotMaskLUT.at(dAttr.config.ch_info.channels) |
                         slotMaskBwLUT.at(dAttr.config.bit_width);
         tkv.push_back(std::make_pair(TAG_KEY_SLOT_MASK, slot_mask));
    }

    tagConfig = (struct agm_tag_config*)malloc(sizeof(struct agm_tag_config) +
                    (tkv.size() * sizeof(agm_key_value)));

    if (!tagConfig) {
        status = -EINVAL;
        goto exit;
    }

    status = SessionAlsaUtils::getTagMetadata(TAG_DEVICE_MUX, tkv, tagConfig);
    if (0 != status) {
        goto exit;
    }

    if (PAL_STREAM_COMPRESSED == sAttr.type) {
        tagCntrlName<<streamComp<<pcmDevIds.at(0)<<setParamTagControl;
        feName<<streamComp<<pcmDevIds.at(0)<<feCtl;
    } else if (PAL_STREAM_VOICE_CALL == sAttr.type) {
        if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
            sAttr.info.voice_call_info.VSID == VOICELBMMODE1){
            tagCntrlName<<streamVoice<<1<<"p"<<setParamTagControl;
            feName<<streamVoice<<1<<"p"<<feCtl;
        } else {
            tagCntrlName<<streamVoice<<2<<"p"<<setParamTagControl;
            feName<<streamVoice<<2<<"p"<<feCtl;
        }
    } else {
        tagCntrlName<<streamPcm<<pcmDevIds.at(0)<<setParamTagControl;
        feName << streamPcm<<pcmDevIds.at(0)<<feCtl;
    }

    // set FE ctl to BE first in case this is called from connectionSessionDevice
    rm->getBackendName(dAttr.id, backendname);
    ctl = mixer_get_ctl_by_name(mixer, feName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", feName.str().data());
        status = -EINVAL;
        goto exit;
    }
    mixer_ctl_set_enum_by_string(ctl, backendname.c_str());
    ctl = NULL;

    // set tag data
    ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
        status = -EINVAL;
        goto exit;
    }
    tkv_size = tkv.size()*sizeof(struct agm_key_value);
    status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
    }

exit:
    if (tagConfig)
        free(tagConfig);
    return status;
}

/* This is to set devicePP MFC(if exists) and PSPD MFC and stream MFC*/
int Session::configureMFC(const std::shared_ptr<ResourceManager>& rm, struct pal_stream_attributes &sAttr,
            struct pal_device &dAttr, const std::vector<int> &pcmDevIds, const char* intf)
{
    int status = 0;
    std::shared_ptr<Device> dev = nullptr;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    struct pal_media_config codecConfig;
    struct sessionToPayloadParam mfcData;
    PayloadBuilder* builder = new PayloadBuilder();
    uint32_t miid = 0;
    bool devicePPMFCSet =  true;

    // clear any cached custom payload
    freeCustomPayload();

    /* Prepare devicePP MFC payload */
    /* Try to set devicePP MFC for virtual port enabled device to match to DMA config */
    if (rm->activeGroupDevConfig &&
            (dAttr.id == PAL_DEVICE_OUT_SPEAKER ||
             dAttr.id == PAL_DEVICE_OUT_HANDSET ||
             dAttr.id == PAL_DEVICE_OUT_ULTRASOUND)) {
        status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0), intf,
                                                       TAG_DEVICE_PP_MFC, &miid);
        if (status == 0) {
            PAL_DBG(LOG_TAG, "miid : %x id = %d, data %s, dev id = %d\n", miid,
                    pcmDevIds.at(0), intf, dAttr.id);

            if (rm->activeGroupDevConfig->devpp_mfc_cfg.bit_width)
                mfcData.bitWidth = rm->activeGroupDevConfig->devpp_mfc_cfg.bit_width;
            else
                mfcData.bitWidth = dAttr.config.bit_width;
            if (rm->activeGroupDevConfig->devpp_mfc_cfg.sample_rate)
                mfcData.sampleRate = rm->activeGroupDevConfig->devpp_mfc_cfg.sample_rate;
            else
                mfcData.sampleRate = dAttr.config.sample_rate;
            if (rm->activeGroupDevConfig->devpp_mfc_cfg.channels)
                mfcData.numChannel = rm->activeGroupDevConfig->devpp_mfc_cfg.channels;
            else
                mfcData.numChannel = dAttr.config.ch_info.channels;
            mfcData.ch_info = nullptr;

            builder->payloadMFCConfig((uint8_t**)&payload, &payloadSize, miid, &mfcData);
            if (!payloadSize) {
                PAL_ERR(LOG_TAG, "payloadMFCConfig failed\n");
                status = -EINVAL;
                goto exit;
            }
            status = updateCustomPayload(payload, payloadSize);
            freeCustomPayload(&payload, &payloadSize);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                goto exit;
            }
        } else {
            PAL_INFO(LOG_TAG, "deviePP MFC doesn't exist for stream %d \n", sAttr.type);
            devicePPMFCSet = false;
        }

        /* set TKV for slot mask */
        setSlotMask(rm, sAttr, dAttr, pcmDevIds);
    } else if (rm->isDeviceMuxConfigEnabled && (dAttr.id == PAL_DEVICE_OUT_SPEAKER ||
              dAttr.id == PAL_DEVICE_OUT_HANDSET)) {
        setSlotMask(rm, sAttr, dAttr, pcmDevIds);
    }

    /* Prepare stream MFC payload */
    if (sAttr.direction == PAL_AUDIO_INPUT) {
        status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0), intf,
                                                       TAG_STREAM_MFC_SR, &miid);
        if (status == 0) {
            PAL_DBG(LOG_TAG, "miid : %x id = %d, data %s, dev id = %d\n", miid,
                    pcmDevIds.at(0), intf, dAttr.id);
            if (isPalPCMFormat(sAttr.in_media_config.aud_fmt_id))
                mfcData.bitWidth = ResourceManager::palFormatToBitwidthLookup(
                                                    sAttr.in_media_config.aud_fmt_id);
            else
                mfcData.bitWidth = sAttr.in_media_config.bit_width;
            mfcData.sampleRate = sAttr.in_media_config.sample_rate;
            mfcData.numChannel = sAttr.in_media_config.ch_info.channels;
            mfcData.ch_info = nullptr;
            builder->payloadMFCConfig((uint8_t **)&payload, &payloadSize, miid, &mfcData);
            if (payloadSize && payload) {
                status = updateCustomPayload(payload, payloadSize);
                freeCustomPayload(&payload, &payloadSize);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "updateCustomPayload failed\n");
                    goto exit;
                }
            }
        }
    }
    /* Prepare PSPD MFC payload */
    /* Get PSPD MFC MIID and configure to match to device config */
    /* This has to be done after sending all mixer controls and before connect */
    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0), intf,
                                                   TAG_DEVICE_MFC_SR, &miid);
    if (status == 0) {
        PAL_DBG(LOG_TAG, "miid : %x id = %d, data %s, dev id = %d\n", miid,
                pcmDevIds.at(0), intf, dAttr.id);

        if (dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP ||
            dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_SCO ||
            dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE ||
            dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE_BROADCAST) {
            dev = Device::getInstance((struct pal_device *)&dAttr , rm);
            if (!dev) {
                PAL_ERR(LOG_TAG, "Device getInstance failed");
                status = -EINVAL;
                goto exit;
            }
            status = dev->getCodecConfig(&codecConfig);
            if(0 != status) {
                PAL_ERR(LOG_TAG, "getCodecConfig Failed \n");
                goto exit;
            }
            mfcData.bitWidth = codecConfig.bit_width;
            mfcData.sampleRate = codecConfig.sample_rate;
            mfcData.numChannel = codecConfig.ch_info.channels;
            mfcData.ch_info = nullptr;
#ifdef SEC_AUDIO_BLE_OFFLOAD
            PAL_DBG(LOG_TAG, "getCodecConfig sampleRate %d bitWidth %d numChannel %d\n",
                             mfcData.sampleRate, mfcData.bitWidth, mfcData.numChannel);
#endif
        } else {
            mfcData.bitWidth = dAttr.config.bit_width;
            if (!devicePPMFCSet && rm->activeGroupDevConfig->devpp_mfc_cfg.sample_rate)
                mfcData.sampleRate = rm->activeGroupDevConfig->devpp_mfc_cfg.sample_rate;
            else
                mfcData.sampleRate = dAttr.config.sample_rate;
            if (!devicePPMFCSet && rm->activeGroupDevConfig->devpp_mfc_cfg.channels)
                mfcData.numChannel = rm->activeGroupDevConfig->devpp_mfc_cfg.channels;
            else
                mfcData.numChannel = dAttr.config.ch_info.channels;
            mfcData.ch_info = nullptr;
        }

        if (dAttr.id == PAL_DEVICE_OUT_AUX_DIGITAL ||
            dAttr.id == PAL_DEVICE_OUT_AUX_DIGITAL_1 ||
            dAttr.id == PAL_DEVICE_OUT_HDMI)
            mfcData.ch_info = &dAttr.config.ch_info;

        builder->payloadMFCConfig((uint8_t **)&payload, &payloadSize, miid, &mfcData);
        if (!payloadSize) {
            PAL_ERR(LOG_TAG, "payloadMFCConfig failed\n");
            status = -EINVAL;
            goto exit;
        }

        status = updateCustomPayload(payload, payloadSize);
        freeCustomPayload(&payload, &payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
            goto exit;
        }
    } else {
        PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
        if ((sAttr.direction == (PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT))||
            (sAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER))
            status = 0;
    }

#ifdef SEC_AUDIO_OFFLOAD
    /* update TAG_COMPRESSED_OFFLOAD_PRE_MFC for samsung adsp solution */
    if ((sAttr.type == PAL_STREAM_COMPRESSED) &&
        (dAttr.id == PAL_DEVICE_OUT_SPEAKER ||
            dAttr.id == PAL_DEVICE_OUT_USB_HEADSET ||
            dAttr.id == PAL_DEVICE_OUT_AUX_DIGITAL ||
            dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP)) {

        struct sessionToPayloadParam preMfcData;
        status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0), intf,
                                                    TAG_COMPRESSED_OFFLOAD_PRE_MFC, &miid);
        if (status == 0) {
            PAL_INFO(LOG_TAG, "for OFFLOAD_PRE_MFC miid : %x id = %d, data %s, dev id = %d\n",
                    miid, pcmDevIds.at(0), intf, dAttr.id);

            if (dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) {
                preMfcData = mfcData;
            } else {
                preMfcData.bitWidth = BIT_WIDTH_24;
                preMfcData.sampleRate = DEFAULT_SAMPLING_RATE;
                preMfcData.numChannel = DEFAULT_CHANNEL_COUNT;
                //TEMP_FOR_U_SETUP preMfcData.rotation_type = PAL_SPEAKER_ROTATION_LR;
                preMfcData.ch_info = nullptr;
            }

            builder->payloadMFCConfig((uint8_t **)&payload, &payloadSize, miid, &preMfcData);
            if (!payloadSize) {
                PAL_ERR(LOG_TAG, "payloadMFCConfig failed\n");
                status = -EINVAL;
                goto exit;
            }

            status = updateCustomPayload(payload, payloadSize);
#ifdef SEC_AUDIO_COMMON //QC_FIXME
            free(payload);
#else
            delete payload;
#endif
            if (0 != status) {
                PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                goto exit;
            }
        } else {
            PAL_ERR(LOG_TAG,"getModuleInstanceId failed");
            if (sAttr.direction == (PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT))
                status = 0;
        }
    }
#endif

exit:
    if (builder) {
        delete builder;
        builder = NULL;
    }
    return status;
}

int Session::checkAndSetExtEC(const std::shared_ptr<ResourceManager>& rm,
                              Stream *s, bool is_enable)
{
    struct pcm_config config;
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;
    std::shared_ptr<Device> dev = nullptr;
    std::vector <std::shared_ptr<Device>> extEcTxDeviceList;
    int32_t extEcbackendId;
    std::vector <std::string> extEcbackendNames;
    struct pal_device device = {};

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_INFO(LOG_TAG, "Enter. %d (extECRefCnt %d)", is_enable, extECRefCnt);
#else
    PAL_DBG(LOG_TAG, "Enter.");
#endif

    extECMutex.lock();
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG,"stream get attributes failed");
        status = -EINVAL;
        goto exit;
    }

    device.id = PAL_DEVICE_IN_EXT_EC_REF;
    rm->getDeviceConfig(&device, &sAttr);
    dev = Device::getInstance(&device, rm);
    if (!dev) {
        PAL_ERR(LOG_TAG, "dev get instance failed");
        status = -EINVAL;
        goto exit;
    }

#ifdef SEC_AUDIO_COMMON
    if (rm->ssrStarted) {
        PAL_INFO(LOG_TAG, "ssrStarted. %d (extECRefCnt %d)", is_enable, extECRefCnt);
        extECRefCnt = 0;
        if (pcmEcTx) {
            status = pcm_stop(pcmEcTx);
            if (status) {
                PAL_ERR(LOG_TAG, "pcm_stop - ec_tx failed %d", status);
            }
            dev->stop();

            status = pcm_close(pcmEcTx);
            if (status) {
                PAL_ERR(LOG_TAG, "pcm_close - ec_tx failed %d", status);
            }
            dev->close();

            rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
            pcmEcTx = NULL;
        }
        rm->ssrStarted = false;
    }
#endif

    if(!is_enable) {
        if (extECRefCnt > 0)
            extECRefCnt --;
        if (extECRefCnt == 0) {
            if (pcmEcTx) {
                status = pcm_stop(pcmEcTx);
                if (status) {
                    PAL_ERR(LOG_TAG, "pcm_stop - ec_tx failed %d", status);
                }
                dev->stop();

                status = pcm_close(pcmEcTx);
                if (status) {
                    PAL_ERR(LOG_TAG, "pcm_close - ec_tx failed %d", status);
                }
                dev->close();

                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                pcmEcTx = NULL;
                ecRefDevId = PAL_DEVICE_OUT_MIN;
                extECMutex.unlock();
                rm->restoreInternalECRefs();
                extECMutex.lock();
            }
        }
    } else {
        extECRefCnt ++;
        if (extECRefCnt == 1) {
            extECMutex.unlock();
            rm->disableInternalECRefs(s);
            extECMutex.lock();
            extEcTxDeviceList.push_back(dev);
            pcmDevEcTxIds = rm->allocateFrontEndExtEcIds();
            if (pcmDevEcTxIds.size() == 0) {
                PAL_ERR(LOG_TAG, "ResourceManger::getBackEndNames returned no EXT_EC device Ids");
                status = -EINVAL;
                goto exit;
            }
            status = dev->open();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "dev open failed");
                status = -EINVAL;
                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                goto exit;
            }
            status = dev->start();
            if (0 != status) {
                PAL_ERR(LOG_TAG, "dev start failed");
                dev->close();
                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                status = -EINVAL;
                goto exit;
            }

            extEcbackendId = extEcTxDeviceList[0]->getSndDeviceId();
            extEcbackendNames = rm->getBackEndNames(extEcTxDeviceList);
            status = SessionAlsaUtils::openDev(rm, pcmDevEcTxIds, extEcbackendId,
                extEcbackendNames.at(0).c_str());
            if (0 != status) {
                PAL_ERR(LOG_TAG, "SessionAlsaUtils::openDev failed");
                dev->stop();
                dev->close();
                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                status = -EINVAL;
                goto exit;
            }
            pcmEcTx = pcm_open(rm->getVirtualSndCard(), pcmDevEcTxIds.at(0), PCM_IN, &config);
            if (!pcmEcTx) {
                PAL_ERR(LOG_TAG, "Exit pcm-ec-tx open failed");
                dev->stop();
                dev->close();
                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                status = -EINVAL;
                goto exit;
            }

            if (!pcm_is_ready(pcmEcTx)) {
                PAL_ERR(LOG_TAG, "Exit pcm-ec-tx open not ready");
                pcmEcTx = NULL;
                dev->stop();
                dev->close();
                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                status = -EINVAL;
                goto exit;
            }

            status = pcm_start(pcmEcTx);
            if (status) {
                PAL_ERR(LOG_TAG, "pcm_start ec_tx failed %d", status);
                pcm_close(pcmEcTx);
                pcmEcTx = NULL;
                dev->stop();
                dev->close();
                rm->freeFrontEndEcTxIds(pcmDevEcTxIds);
                status = -EINVAL;
                goto exit;
            }
        }
    }

exit:

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    if (is_enable && (status < 0)) {
        PAL_ERR(LOG_TAG, "Reset extECRefCnt as EXT EC graph fails to setup");
        extECRefCnt = 0;
    }
#else
    if (is_enable && status) {
        PAL_DBG(LOG_TAG, "Reset extECRefCnt as EXT EC graph fails to setup");
        extECRefCnt = 0;
    }
#endif
    extECMutex.unlock();
    PAL_DBG(LOG_TAG, "Exit.");
    return status;
}

/*
 Handle case when charging going is on and PB starts on speaker.
 Below steps are to ensure HW transition from charging->boost->charging
 which will avoid USB collapse due to HW transition in less moment.
 1. Set concurrency bit to update charger driver for respective session
 2. Enable Speaker boost when Audio done voting as part of PCM_open
 3. Set ICL config in device:Speaker module.
*/
int Session::NotifyChargerConcurrency(std::shared_ptr<ResourceManager>rm, bool state)
{
    int status = -EINVAL;

    if (!rm)
        goto exit;

    PAL_DBG(LOG_TAG, "Enter concurrency state %d Notify state %d \n",
            rm->getConcurrentBoostState(), state);

    ResourceManager::mChargerBoostMutex.lock();
    if (rm->getChargerOnlineState()) {
        if (rm->getConcurrentBoostState() ^ state)
            status = rm->chargerListenerSetBoostState(state, CHARGER_ON_PB_STARTS);

        if (0 != status)
            PAL_ERR(LOG_TAG, "Failed to notify PMIC: %d", status);
    }

    PAL_DBG(LOG_TAG, "Exit concurrency state %d with status %d",
            rm->getConcurrentBoostState(), status);
    ResourceManager::mChargerBoostMutex.unlock();
exit:
    return status;
}

/* Handle case when charging going on and PB starts on speaker*/
int Session::EnableChargerConcurrency(std::shared_ptr<ResourceManager>rm, Stream *s)
{
    int status = -EINVAL;

    if (!rm)
        goto exit;

    PAL_DBG(LOG_TAG, "Enter concurrency state %d", rm->getConcurrentBoostState());

    if ((s && rm->getChargerOnlineState()) &&
        (rm->getConcurrentBoostState())) {
         status = rm->setSessionParamConfig(PAL_PARAM_ID_CHARGER_STATE, s,
                                               CHARGE_CONCURRENCY_ON_TAG);
        if (0 != status) {
            PAL_DBG(LOG_TAG, "Set SessionParamConfig with status %d", status);
            status = rm->chargerListenerSetBoostState(false, CHARGER_ON_PB_STARTS);
            if (0 != status)
                PAL_ERR(LOG_TAG, "Failed to notify PMIC: %d", status);
        }
    }
    PAL_DBG(LOG_TAG, "Exit concurrency state %d with status %d",
            rm->getConcurrentBoostState(), status);
exit:
    return status;
}

void Session::setInitialVolume() {
    int32_t status = 0;
    struct volume_set_param_info vol_set_param_info = {};
    uint16_t volSize = 0;
    uint8_t *volPayload = nullptr;
    struct pal_stream_attributes sAttr = {};
    bool isStreamAvail = false;
    struct pal_vol_ctrl_ramp_param ramp_param = {};
    Session *session = NULL;
    bool forceSetParameters = false;

    PAL_DBG(LOG_TAG, "Enter");

    if (!streamHandle) {
        PAL_ERR(LOG_TAG, "streamHandle is invalid");
        goto exit;
    }
    status = streamHandle->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }

#ifdef SEC_AUDIO_OFFLOAD
    if (sAttr.type == PAL_STREAM_COMPRESSED) {
        PAL_DBG(LOG_TAG, "use sec volume in compressed case");
        goto exit;
    }
#endif

    for (int32_t i = 0; streamHandle->mVolumeData &&
        i < (streamHandle->mVolumeData->no_of_volpair); i++) {
        if((i > 0) &&
            (abs(streamHandle->mVolumeData->volume_pair[0].vol -
                streamHandle->mVolumeData->volume_pair[i].vol) > VOLUME_TOLERANCE)) {
                forceSetParameters = true;
                break;
        }
    }

    memset(&vol_set_param_info, 0, sizeof(struct volume_set_param_info));
    rm->getVolumeSetParamInfo(&vol_set_param_info);
    isStreamAvail = (find(vol_set_param_info.streams_.begin(),
                vol_set_param_info.streams_.end(), sAttr.type) !=
                vol_set_param_info.streams_.end());
    if ((isStreamAvail && vol_set_param_info.isVolumeUsingSetParam) || forceSetParameters) {
        if (sAttr.direction == PAL_AUDIO_OUTPUT) {
           /* DSP default volume is highest value, non-0 rampping period
            * brings volume burst from highest amplitude to new volume
            * at the begining, that makes pop noise heard.
            * set ramp period to 0 ms before pcm_start only for output,
            * so desired volume can take effect instantly at the begining.
            */
            ramp_param.ramp_period_ms = 0;
            status = setParameters(streamHandle, TAG_STREAM_VOLUME,
                                   PAL_PARAM_ID_VOLUME_CTRL_RAMP, &ramp_param);
        }
        // apply if there is any cached volume
        if (streamHandle->mVolumeData) {
            volSize = (sizeof(struct pal_volume_data) +
                      (sizeof(struct pal_channel_vol_kv) *
                      (streamHandle->mVolumeData->no_of_volpair)));
            volPayload = new uint8_t[sizeof(pal_param_payload) +
                volSize]();
            pal_param_payload *pld = (pal_param_payload *)volPayload;
            pld->payload_size = sizeof(struct pal_volume_data);
            memcpy(pld->payload, streamHandle->mVolumeData, volSize);
            status = setParameters(streamHandle, TAG_STREAM_VOLUME,
                    PAL_PARAM_ID_VOLUME_USING_SET_PARAM, (void *)pld);
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            if (status != 0) {
                PAL_ERR(LOG_TAG,"Setting volume failed %d", status);
            } else {
                PAL_INFO(LOG_TAG, "Volume payload mask:%x vol:%f",
                    (streamHandle->mVolumeData->volume_pair[0].channel_mask), (streamHandle->mVolumeData->volume_pair[0].vol));
            }
#endif
            delete[] volPayload;
        }
        if (sAttr.direction == PAL_AUDIO_OUTPUT) {
            //set ramp period back to default.
            ramp_param.ramp_period_ms = DEFAULT_RAMP_PERIOD;
            status = setParameters(streamHandle, TAG_STREAM_VOLUME,
                                   PAL_PARAM_ID_VOLUME_CTRL_RAMP, &ramp_param);
        }
    } else {
        // Setting the volume as in stream open, no default volume is set.
        if (sAttr.type != PAL_STREAM_ACD &&
            sAttr.type != PAL_STREAM_VOICE_UI &&
            sAttr.type != PAL_STREAM_CONTEXT_PROXY &&
            sAttr.type != PAL_STREAM_ULTRASOUND &&
            sAttr.type != PAL_STREAM_SENSOR_PCM_DATA &&
            sAttr.type != PAL_STREAM_HAPTICS &&
            sAttr.type != PAL_STREAM_COMMON_PROXY) {

            if (setConfig(streamHandle, CALIBRATION, TAG_STREAM_VOLUME) != 0) {
                PAL_ERR(LOG_TAG,"Setting volume failed");
            }
        }
    }
#ifdef SEC_AUDIO_COMMON
    if (status != 0) {
        PAL_DBG(LOG_TAG, "Setting volume failed status: %d, should be reture 0 always", status);
        status = 0;
    }
#endif

exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
}

#if 0
int setConfig(Stream * s, pal_stream_type_t sType, configType type, uint32_t tag1,
        uint32_t tag2, uint32_t tag3)
{
    int status = 0;
    uint32_t tagsent = 0;
    struct agm_tag_config* tagConfig = nullptr;
    std::ostringstream tagCntrlName;
    char const *stream = "PCM";
    const char *setParamTagControl = "setParamTag";
    struct mixer_ctl *ctl = nullptr;
    uint32_t tkv_size = 0;

    if (sType == PAL_STREAM_COMPRESSED)
        stream = "COMPRESS";

    switch (type) {
        case MODULE:
            tkv.clear();
            if (tag1)
                builder->populateTagKeyVector(s, tkv, tag1, &tagsent);
            if (tag2)
                builder->populateTagKeyVector(s, tkv, tag2, &tagsent);
            if (tag3)
                builder->populateTagKeyVector(s, tkv, tag3, &tagsent);

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }
            tagConfig = (struct agm_tag_config*)malloc (sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));
            if(!tagConfig) {
                status = -ENOMEM;
                goto exit;
            }
            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }
            tagCntrlName << stream << pcmDevIds.at(0) << " " << setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                return -ENOENT;
            }

            tkv_size = tkv.size() * sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
                goto exit;
            }
            ctl = NULL;
            tkv.clear();
            break;
        default:
            status = 0;
            break;
    }

exit:
    return status;
}
#endif

/*
 *
 * 1. Register to listen at AGM level for Silence Detection Even
 * 2. Register a Callback to receive events from DSP
 * 3. Get MIID of HW_ENDPOINT_TX
 * 4. Configure PARAM_ID_SILECENCE_DETECTION payload
 *
 **/
#ifdef SEC_AUDIO_MUTE_DETECTION
int Session::enableSilenceDetection(const std::shared_ptr<ResourceManager> rm,
                struct mixer *mixer, const std::vector<int> &devIds,
                const char *intf_name, uint64_t cookie, uint32_t eventId)
#else
int Session::enableSilenceDetection(const std::shared_ptr<ResourceManager> rm,
                struct mixer *mixer, const std::vector<int> &devIds,
                const char *intf_name, uint64_t cookie)
#endif
{
    int status = 0;
    uint32_t miid = 0;
    size_t pad_bytes = 0, payloadSize = 0;
    uint8_t* payload = NULL;
    struct apm_module_param_data_t* header = NULL;
    param_id_silence_detection_t *silence_detection_cfg = NULL;
    struct agm_event_reg_cfg event_cfg = {};

#ifdef SEC_AUDIO_MUTE_DETECTION
    int tagId = DEVICE_HW_ENDPOINT_TX;
    if (eventId == EVENT_ID_DV_MUTE_DETECTION) {
        if (silenceEventRegisteredForMcst == true)
            goto silence_ev_setup_done;
        tagId = VOICE_DV_withoutkey;
    } else {
        if (silenceEventRegistered == true)
            goto silence_ev_setup_done;
    }

    PAL_INFO(LOG_TAG, "Registering For Silence Detection Events, eventId %x\n", eventId);
    event_cfg.event_id = eventId;
#else
    if (silenceEventRegistered == true)
        goto silence_ev_setup_done;

    PAL_INFO(LOG_TAG, "Registering For Silence Detection Events \n");
    event_cfg.event_id = EVENT_ID_SILENCE_DETECTION;
#endif
    event_cfg.event_config_payload_size = 0;
    event_cfg.is_register = 1;

#ifdef SEC_AUDIO_MUTE_DETECTION
    status  = SessionAlsaUtils::registerMixerEvent(mixer, devIds.at(0), intf_name,
                    tagId, (void *)&event_cfg,
                    sizeof(struct agm_event_reg_cfg));
#else
    status  = SessionAlsaUtils::registerMixerEvent(mixer, devIds.at(0), intf_name,
                    DEVICE_HW_ENDPOINT_TX, (void *)&event_cfg,
                    sizeof(struct agm_event_reg_cfg));
#endif
    if (status) {
        PAL_ERR(LOG_TAG, "Failed Registering for SILENCE DETECTION EVENT\n");
        goto silence_ev_setup_done;
    }

    PAL_INFO(LOG_TAG, "Registered for Silence Detection Event\n");

    status = rm->registerMixerEventCallback(devIds, handleSilenceDetectionCb, cookie, true);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Failed to register DSP cb for silence detection Event");
        goto err_silence_ev_cb_reg;
    }

    PAL_INFO(LOG_TAG, "Registered CB for Silence Detection\n");

#ifdef SEC_AUDIO_MUTE_DETECTION
    status =  SessionAlsaUtils::getModuleInstanceId(mixer,
                    devIds.at(0), intf_name, tagId, &miid);
#else
    status =  SessionAlsaUtils::getModuleInstanceId(mixer,
                    devIds.at(0), intf_name, DEVICE_HW_ENDPOINT_TX, &miid);
#endif
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Error retriving MIID for HW_ENDPOINT_TX\n");
        goto err_silence_ev_setup;
    }

    payloadSize = sizeof(struct apm_module_param_data_t)+sizeof(param_id_silence_detection_t);
    pad_bytes = PAL_PADDING_8BYTE_ALIGN(payloadSize);

    payload = (uint8_t *)calloc(1, payloadSize+pad_bytes);
    if (!payload){
        PAL_ERR(LOG_TAG, "payload info calloc failed \n");
        goto err_silence_ev_setup;
    }
    header = (struct apm_module_param_data_t  *)payload;
    header->module_instance_id = miid;
    header->param_id =  PARAM_ID_SILENCE_DETECTION;
    header->error_code = 0x0;
    header->param_size = payloadSize - sizeof(struct apm_module_param_data_t);

    silence_detection_cfg = (param_id_silence_detection_t *)(payload +
                    sizeof(struct apm_module_param_data_t));
    silence_detection_cfg->enable_detection = 1;
    silence_detection_cfg->detection_duration_ms = ResourceManager::silenceDetectionDuration;

    PAL_INFO(LOG_TAG, "Sending Silence Detection Custom Payload\n");
    status = updateCustomPayload (payload, (payloadSize+pad_bytes));
    freeCustomPayload(&payload, &payloadSize);
    if (status !=0 ) {
        PAL_ERR(LOG_TAG, "updateCustomPayload failed for SILENCE DETECTION \n");
        goto err_silence_ev_setup;
    }

    status = SessionAlsaUtils::setMixerParameter(mixer, devIds.at(0),
                    customPayload, customPayloadSize);
    freeCustomPayload();
    if (status !=0) {
        PAL_ERR(LOG_TAG, "setMixerParameter failed for Silence Detection Parameter");
        goto err_silence_ev_setup;
    }
#ifdef SEC_AUDIO_MUTE_DETECTION
    if (eventId == EVENT_ID_DV_MUTE_DETECTION)
        silenceEventRegisteredForMcst = true;
    else
#endif
    silenceEventRegistered = true;
    PAL_INFO(LOG_TAG, "Silence Detection setup sucessfully \n");

    goto silence_ev_setup_done;

err_silence_ev_setup:
                rm->registerMixerEventCallback(devIds, handleSilenceDetectionCb,
                                cookie, false);
err_silence_ev_cb_reg:
                event_cfg.is_register = 0;
#ifdef SEC_AUDIO_MUTE_DETECTION
                status  = SessionAlsaUtils::registerMixerEvent(mixer, devIds.at(0),
                               intf_name, tagId,
                                (void *)&event_cfg, sizeof(struct agm_event_reg_cfg));
#else
                status  = SessionAlsaUtils::registerMixerEvent(mixer, devIds.at(0),
                               intf_name, DEVICE_HW_ENDPOINT_TX,
                                (void *)&event_cfg, sizeof(struct agm_event_reg_cfg));
#endif
silence_ev_setup_done:
                status = 0;
    return status;
}

#ifdef SEC_AUDIO_MUTE_DETECTION
int Session::disableSilenceDetection(const std::shared_ptr<ResourceManager> rm,
                struct mixer *mixer, const std::vector<int> &devIds,
                const char *intf_name, uint64_t cookie, uint32_t eventId)
#else
int Session::disableSilenceDetection(const std::shared_ptr<ResourceManager> rm,
                struct mixer *mixer, const std::vector<int> &devIds,
                const char *intf_name, uint64_t cookie)
#endif
{
    int status = 0;
    struct agm_event_reg_cfg event_cfg = {};

#ifdef SEC_AUDIO_MUTE_DETECTION
    int tagId = DEVICE_HW_ENDPOINT_TX;
    event_cfg.event_id = eventId;
#else
    event_cfg.event_id = EVENT_ID_SILENCE_DETECTION;
#endif
    event_cfg.event_config_payload_size = 0;
    event_cfg.is_register = 0;

#ifdef SEC_AUDIO_MUTE_DETECTION
    if (eventId == EVENT_ID_DV_MUTE_DETECTION) {
        if (silenceEventRegisteredForMcst == false)
            goto exit;
        tagId = VOICE_DV_withoutkey;
    } else {
        if (silenceEventRegistered == false)
            goto exit;
    }

    PAL_INFO(LOG_TAG, "De-registering For Silence Detection Events, eventId %x\n", eventId);
    status  = SessionAlsaUtils::registerMixerEvent(mixer, devIds.at(0),
                intf_name, tagId,
                (void *)&event_cfg, sizeof(struct agm_event_reg_cfg));
#else
    if (silenceEventRegistered == false)
        goto exit;

    PAL_INFO(LOG_TAG, "De-registering For Silence Detection Events\n");
    status  = SessionAlsaUtils::registerMixerEvent(mixer, devIds.at(0),
                intf_name, DEVICE_HW_ENDPOINT_TX,
                (void *)&event_cfg, sizeof(struct agm_event_reg_cfg));
#endif
    if (status)
        PAL_ERR(LOG_TAG, "Unable to deregister SILENCE DETECTION EVENT\n");

    status = rm->registerMixerEventCallback(devIds, handleSilenceDetectionCb, cookie, false);
    if (status != 0)
        PAL_ERR(LOG_TAG, "Failed to deregister  silence detection Callback to rm");

#ifdef SEC_AUDIO_MUTE_DETECTION
    if (eventId == EVENT_ID_DV_MUTE_DETECTION)
        silenceEventRegisteredForMcst = false;
    else
#endif
    silenceEventRegistered = false;

exit:
    return status;
}

int dump_kernel_log(char *kmsg_out_file)
{
    int kmsg_fd = 0, log_out_size = 0;
    ssize_t kernel_buf_size = 0;
    char *kernel_buf = NULL;

    kernel_buf_size = klogctl(SYSLOG_ACTION_SIZE_BUFFER, NULL, 0);
    PAL_INFO(LOG_TAG, "%s::kernel_buf_size :: %zd", __func__, kernel_buf_size);

    kernel_buf = (char *)malloc(kernel_buf_size);
    if (!kernel_buf) {
        PAL_ERR(LOG_TAG, "%s:: error allocating memory", __func__);
        return -ENOMEM;
    }

    kmsg_fd = open(kmsg_out_file, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
    if (kmsg_fd < 0){
        PAL_ERR(LOG_TAG, "%s::Error opening kernel msg out file", __func__);
        free(kernel_buf);
        return kmsg_fd;
    }

    klogctl(SYSLOG_ACTION_READ_ALL, kernel_buf, kernel_buf_size);
    log_out_size = write(kmsg_fd, kernel_buf, kernel_buf_size);
    if (log_out_size < 0) {
        PAL_ERR(LOG_TAG, "%s: %s  Unable to write.\n", __func__, kmsg_out_file);
        goto close_kmsg_fd;
    }
    PAL_INFO(LOG_TAG, "%s: Writing %s log, %d bytes\n",__func__,
                    kmsg_out_file, log_out_size);

close_kmsg_fd:
    close(kmsg_fd);
    free(kernel_buf);
    return log_out_size;
}

int dump_registers(char *in_file_path, char *regdump_out_file)
{
    char *reg_dump = NULL;
    int infile_fd = 0,  regdump_wr_fd = 0;
    int read_out_size = 0, regdump_size = 0;
    size_t sysfs_page_size = sysconf(_SC_PAGESIZE);

    reg_dump = (char *)malloc(REGDUMP_OUT_SIZE);
    if (!reg_dump) {
        PAL_ERR(LOG_TAG, "%s:: error allocating memory", __func__);
        return -ENOMEM;
    }

    infile_fd = open(in_file_path, O_RDONLY);
    if (infile_fd < 0) {
        PAL_ERR(LOG_TAG, "%s: %s  not found.\n", __func__, in_file_path);
        read_out_size = -1;
        goto free_buf;
    }

    PAL_INFO(LOG_TAG, "Reading %s regdump interface \n", in_file_path);
    read_out_size = read(infile_fd, reg_dump, REGDUMP_OUT_SIZE);
    if (read_out_size < 0) {
        PAL_ERR(LOG_TAG, "%s: %s  Unable to Read.\n", __func__, in_file_path);
        read_out_size  = -1;
        goto close_infile;
    }
    PAL_INFO(LOG_TAG, "Regdump ReadOut Buffer Size = %d", read_out_size);

    regdump_wr_fd = open(regdump_out_file, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
    if (regdump_wr_fd < 0) {
        PAL_ERR(LOG_TAG, "%s: %s  Unable to Open for writing.\n", __func__, regdump_out_file);
        read_out_size = -1;
        goto close_infile;
    }
    regdump_size =  write(regdump_wr_fd, reg_dump, read_out_size);
    if (regdump_size < 0) {
        PAL_ERR(LOG_TAG, "%s: %s  Unable to write.\n", __func__, regdump_out_file);
        read_out_size = -1;
        goto close_regdump_file;
    }
    PAL_INFO(LOG_TAG, "Bolero Regmap Dump Size %ld and file %s", regdump_size, regdump_out_file);

close_regdump_file:
    close(regdump_wr_fd);
close_infile:
    close(infile_fd);
free_buf:
    free(reg_dump);
    return read_out_size;
}

int dump_silence_event_status(char *out_file, uint32_t channel_group, uint32_t status_ch_mask)
{
    char event_data_buf[255];
    int pos = 0,  write_out_bytes = 0, fd = 0;

    pos = snprintf(event_data_buf, 255, "channel_group :: %u \n",
                        channel_group);
    pos += snprintf(event_data_buf+pos, 255-pos, "Channel_Status :: %u \n",
                        status_ch_mask);

    fd = open(out_file, O_CREAT|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
    if (fd < 0) {
        PAL_ERR(LOG_TAG,
                "%s::Error Opening silence data status file\n", __func__);
        return -EINVAL;
    }

    write_out_bytes = write(fd, event_data_buf, pos);
    if (write_out_bytes < 1)
        PAL_ERR(LOG_TAG, "%s::failed writing silence event status",__func__);

    close(fd);
    return write_out_bytes;
}

/*
 *Callback from DSP for SILENCE Detection
 */
void handleSilenceDetectionCb(uint64_t hdl __unused, uint32_t event_id, void *event_data, uint32_t event_size)
{
    char out_file_name[MAX_DUMP_FILENAME_SIZE];
    uint32_t channel_group = 0 , status_ch_mask = 0;
    event_cfg_silence_detection_t *silence_event = nullptr;
    struct tm *timenow;
    time_t now = time(NULL);
    timenow = gmtime(&now);
#ifdef SEC_AUDIO_MUTE_DETECTION
    dv_mute_event_payload_info_t *mcst_event = nullptr;
    android::String8 value;
    android::String8 deviceInfo;
    std::vector<std::shared_ptr<Device>> deviceList;
#endif
    PAL_INFO(LOG_TAG, "Silence Detection event raised\n");

    switch (event_id) {

#ifdef SEC_AUDIO_MUTE_DETECTION
    case EVENT_ID_DV_MUTE_DETECTION:
        PAL_INFO(LOG_TAG, "EVENT_ID_DV_MUTE_DETECTION received from DiamonVoice Solution\n");
        mcst_event = (dv_mute_event_payload_info_t *)event_data;
        PAL_INFO(LOG_TAG, "mcst=%d, dv_rx_mute=%d \n", mcst_event->mcst, mcst_event->dv_rx_mute);
        value = android::String8::format("mcst=%d, dv_rx_mute=%d", mcst_event->mcst, mcst_event->dv_rx_mute);
        android::AudioInfo::GetInstance()->setSecSaveAudioLog(value);
        break;
#endif
    case EVENT_ID_SILENCE_DETECTION:
        PAL_INFO(LOG_TAG, "EVENT_ID_SILENCE_DETECTION received from DSP\n");
#ifdef SEC_AUDIO_MUTE_DETECTION
        value = android::String8::format("EVENT_ID_SILENCE_DETECTION received from DSP");
        android::AudioInfo::GetInstance()->setSecSaveAudioLog(value);
#ifdef SEC_AUDIO_REPORT_HAL_ERROR
        android::SecAudioHalProxy::onErrorReported(AUDIO_ERROR_SILENCE_DETECTION);
#endif
#endif
#ifndef SEC_AUDIO_COMMON // dump is not used
        strftime(out_file_name, MAX_DUMP_FILENAME_SIZE,
                        SILENCE_EVENT_INFO, timenow);

        silence_event = (event_cfg_silence_detection_t *)event_data;
        channel_group = silence_event->num_32_channel_group;
        status_ch_mask = silence_event->detections[0].status_ch_mask;
        dump_silence_event_status(out_file_name, channel_group, status_ch_mask);

        /*
         * Read BOLERO/CDC Registers
         **/
        strftime(out_file_name, MAX_DUMP_FILENAME_SIZE,
                        BOLERO_REGDUMP_OUT_PATH, timenow);
        dump_registers(BOLERO_PROC_INTF, out_file_name);

        /*
         * Read SWR VA Macro Registers
         **/
        strftime(out_file_name, MAX_DUMP_FILENAME_SIZE,
                        VA_SWR_REGDUM_OUT_PATH, timenow);
        dump_registers(VA_SWR_PROC_INTF, out_file_name);

        /*
         * Read WCD939X  Registers
         **/
        strftime(out_file_name, MAX_DUMP_FILENAME_SIZE,
                        WCD939X_REGDUMP_OUT_PATH, timenow);
        dump_registers(WCD939X_PROC_INTF, out_file_name);

        /*
         * kernel msg (/dev/kmsg) read
         **/
        strftime(out_file_name, MAX_DUMP_FILENAME_SIZE,
                        KMSG_OUT_FILE, timenow);
        dump_kernel_log(out_file_name);
#endif
        break;

    default:

        break;

    }

    return;
}
