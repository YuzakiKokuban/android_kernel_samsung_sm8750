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
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: SessionAlsaPcm"

#include <agm/agm_api.h>
#include <asps/asps_acm_api.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <amdb_api.h>
#include "rx_haptics_api.h"
#include "audio_dam_buffer_api.h"
#include "sh_mem_pull_push_mode_api.h"
#include "apm_api.h"
#include "us_detect_api.h"
#include "us_gen_api.h"
#include "SessionAlsaPcm.h"
#include "SessionAlsaUtils.h"
#include "Stream.h"
#include "ResourceManager.h"
#include "detection_cmn_api.h"
#include "acd_api.h"
#include "asr_module_calibration_api.h"

std::mutex SessionAlsaPcm::pcmLpmRefCntMtx;
int SessionAlsaPcm::pcmLpmRefCnt = 0;

#define SESSION_ALSA_MMAP_DEFAULT_OUTPUT_SAMPLING_RATE (48000)
#define SESSION_ALSA_MMAP_PERIOD_SIZE (SESSION_ALSA_MMAP_DEFAULT_OUTPUT_SAMPLING_RATE/1000)
#define SESSION_ALSA_MMAP_PERIOD_COUNT_MIN 64
#define SESSION_ALSA_MMAP_PERIOD_COUNT_MAX 2048
#define SESSION_ALSA_MMAP_PERIOD_COUNT_DEFAULT (SESSION_ALSA_MMAP_PERIOD_COUNT_MAX)
/* Param ID definitions */
#define PARAM_ID_FFV_DOA_TRACKING_MONITOR 0x080010A4

SessionAlsaPcm::SessionAlsaPcm(std::shared_ptr<ResourceManager> Rm)
{
   rm = Rm;
   builder = new PayloadBuilder();
   customPayload = NULL;
   customPayloadSize = 0;
   eventPayload = NULL;
   eventPayloadSize = 0;
   pcm = NULL;
   pcmRx = NULL;
   pcmTx = NULL;
   mState = SESSION_IDLE;
   ecRefDevId = PAL_DEVICE_OUT_MIN;
   streamHandle = NULL;
   vaMicChannels = 0;
}

SessionAlsaPcm::~SessionAlsaPcm()
{
   delete builder;
}

int SessionAlsaPcm::prepare(Stream * s)
{
    int status = 0;
    uint32_t channels = 0;
    struct pal_stream_attributes sAttr = {};
    struct pal_device dAttr = {};
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::shared_ptr<Device> dev = nullptr;

    PAL_DBG(LOG_TAG, "Enter");
    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream");
        return -EINVAL;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }
    // explicitly set ckv for VoiceUI/ACD/SPCM
    if (sAttr.type == PAL_STREAM_VOICE_UI ||
        sAttr.type == PAL_STREAM_ACD ||
        sAttr.type == PAL_STREAM_SENSOR_PCM_DATA ||
        sAttr.type == PAL_STREAM_ASR) {
        status = s->getAssociatedDevices(associatedDevices);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed");
            goto exit;
        }

        for (int i = 0; i < associatedDevices.size(); i++) {
            associatedDevices[i]->getDeviceAttributes(&dAttr);
            channels = dAttr.config.ch_info.channels;
            if (channels != vaMicChannels) {
                if (mState != SESSION_IDLE) {
                    PAL_DBG(LOG_TAG, "Set device ckv");
                    status = setConfig(s, CALIBRATION, HW_EP_TX);
                    if (status != 0) {
                        if (status == -EALREADY) {
                            PAL_ERR(LOG_TAG, "Calibration already set, ignore");
                            status = 0;
                        } else {
                            PAL_ERR(LOG_TAG,
                               "Failed to set devicepp ckv, status %d", status);
                            goto exit;
                        }
                    }
                }
                vaMicChannels = channels;
            }
        }
    }

exit:
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int SessionAlsaPcm::open(Stream * s)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    std::vector<std::shared_ptr<Device>> associatedDevices;
    int ldir = 0;
    std::vector<int> pcmId;

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    streamHandle = s;
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }
    if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD &&
        sAttr.type != PAL_STREAM_VOICE_CALL_MUSIC  &&
        sAttr.type != PAL_STREAM_CONTEXT_PROXY &&
        sAttr.type != PAL_STREAM_COMMON_PROXY) {
        status = s->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
            goto exit;
        }

        rm->getBackEndNames(associatedDevices, rxAifBackEnds, txAifBackEnds);
        if (rxAifBackEnds.empty() && txAifBackEnds.empty()) {
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "no backend specified for this stream");
            goto exit;

        }
        if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD &&
            sAttr.type != PAL_STREAM_VOICE_CALL_MUSIC) {
            if (sAttr.direction == PAL_AUDIO_INPUT) {
                if (txAifBackEnds.empty() || !rxAifBackEnds.empty()) {
                    PAL_ERR(LOG_TAG, "backend specified incorrectly for this stream\n");
                    return -EINVAL;
                }
            }
            if (sAttr.direction == PAL_AUDIO_OUTPUT) {
                if (rxAifBackEnds.empty() || !txAifBackEnds.empty()) {
                    PAL_ERR(LOG_TAG, "backend specified incorrectly for this stream\n");
                    return -EINVAL;
                }
            }
        }
    }
    status = rm->getVirtualAudioMixer(&mixer);
    if (status) {
        PAL_ERR(LOG_TAG, "mixer error");
        goto exit;
    }
    if (sAttr.type != PAL_STREAM_LOOPBACK) {
        if (sAttr.direction == PAL_AUDIO_INPUT) {
            if (sAttr.type == PAL_STREAM_ACD || sAttr.type == PAL_STREAM_SENSOR_PCM_DATA ||
                sAttr.type == PAL_STREAM_ASR)
                ldir = TX_HOSTLESS;

            pcmDevIds = rm->allocateFrontEndIds(sAttr, ldir);
            if (pcmDevIds.size() == 0) {
                PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
                status = -EINVAL;
                goto exit;
            }
        } else if (sAttr.direction == PAL_AUDIO_OUTPUT) {
            if (sAttr.type == PAL_STREAM_HAPTICS &&
                sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH) {
                PAL_DBG(LOG_TAG, "haptics type = %d",sAttr.info.opt_stream_info.haptics_type);
                ldir = RX_HOSTLESS;
            }
            if (sAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER) {
                ldir = RX_HOSTLESS;
            }
            pcmDevIds = rm->allocateFrontEndIds(sAttr, ldir);
            if (pcmDevIds.size() == 0) {
                PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
                status = -EINVAL;
                goto exit;
            }
        } else {
            pcmDevRxIds = rm->allocateFrontEndIds(sAttr, RX_HOSTLESS);
            pcmDevTxIds = rm->allocateFrontEndIds(sAttr, TX_HOSTLESS);
            if (!pcmDevRxIds.size() || !pcmDevTxIds.size()) {
                PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
                status = -EINVAL;
                goto exit;
            }
        }
    } else {
        if (sAttr.info.opt_stream_info.loopback_type == PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) {
            // Loopback for RX path
            pcmDevIds = rm->allocateFrontEndIds(sAttr, RX_HOSTLESS);
            if (!pcmDevIds.size()) {
                PAL_ERR(LOG_TAG, "allocateFrontEndIds for RX loopback failed");
                status = -EINVAL;
                goto exit;
            }
        }
        else if (sAttr.info.opt_stream_info.loopback_type ==
                  PAL_STREAM_LOOPBACK_CAPTURE_ONLY) {
            // Loopback for TX path
            pcmDevTxIds = rm->allocateFrontEndIds(sAttr, TX_HOSTLESS);
            if (!pcmDevTxIds.size()) {
                PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
                status = -EINVAL;
                goto exit;
            }
        }
        else {
            pcmDevRxIds = rm->allocateFrontEndIds(sAttr, RX_HOSTLESS);
            pcmDevTxIds = rm->allocateFrontEndIds(sAttr, TX_HOSTLESS);
            if (!pcmDevRxIds.size() || !pcmDevTxIds.size()) {
                if (pcmDevRxIds.size()) {
                    PAL_ERR(LOG_TAG, "freeFrontEndIds Rx called as failed");
                    rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
                }
                if (pcmDevTxIds.size()) {
                    PAL_ERR(LOG_TAG, "freeFrontEndIds Tx called as failed");
                    rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
                }
                PAL_ERR(LOG_TAG, "allocateFrontEndIds failed");
                status = -EINVAL;
                goto exit;
            }
        }
    }
    frontEndIdAllocated = true;
    switch (sAttr.direction) {
        case PAL_AUDIO_INPUT:
            status = SessionAlsaUtils::open(s, rm, pcmDevIds, txAifBackEnds);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
                rm->freeFrontEndIds(pcmDevIds, sAttr, ldir);
                frontEndIdAllocated = false;
            }
            break;
        case PAL_AUDIO_OUTPUT:
            status = SessionAlsaUtils::open(s, rm, pcmDevIds, rxAifBackEnds);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
                rm->freeFrontEndIds(pcmDevIds, sAttr, ldir);
                frontEndIdAllocated = false;
            }
            else if ((sAttr.type == PAL_STREAM_PCM_OFFLOAD) ||
                     (sAttr.type == PAL_STREAM_DEEP_BUFFER) ||
                     (sAttr.type == PAL_STREAM_SPATIAL_AUDIO) ||
                     (sAttr.type == PAL_STREAM_LOW_LATENCY)) {
                     // Register for Mixer Event callback for
                     // only playback related streams
                     status = rm->registerMixerEventCallback(pcmDevIds,
                         sessionCb, cbCookie, true);
                     if (status == 0) {
                         isMixerEventCbRegd = true;
                     } else {
                         // Not a fatal error
                         PAL_ERR(LOG_TAG, "Failed to register callback to rm");
                         // If registration fails for this then pop noise
                         // issue will come. It isn't fatal so not throwing error.
                         status = 0;
                     }
            }
            break;
        case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
            if (sAttr.info.opt_stream_info.loopback_type ==
                    PAL_STREAM_LOOPBACK_CAPTURE_ONLY) {
                status = SessionAlsaUtils::open(s, rm, pcmDevTxIds, txAifBackEnds);
                if (status) {
                    PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
                    rm->freeFrontEndIds(pcmDevIds, sAttr, TX_HOSTLESS);
                    frontEndIdAllocated = false;
                }
            }
            else if (sAttr.info.opt_stream_info.loopback_type ==
                        PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) {
                status = SessionAlsaUtils::open(s, rm, pcmDevRxIds, rxAifBackEnds);
                if (status) {
                    PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
                    rm->freeFrontEndIds(pcmDevIds, sAttr, RX_HOSTLESS);
                    frontEndIdAllocated = false;
                }
            }
            else {
                if (txAifBackEnds.empty() || rxAifBackEnds.empty()){
                    PAL_ERR(LOG_TAG, "tx and rx backends are not specified correctly for this stream\n");
                    status = -EINVAL;
                }
                if (0 == status) {
                    status = SessionAlsaUtils::open(s, rm, pcmDevRxIds, pcmDevTxIds,
                             rxAifBackEnds, txAifBackEnds);
                }
                if (status) {
                    PAL_ERR(LOG_TAG, "session alsa open failed with %d", status);
                    rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
                    rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
                    frontEndIdAllocated = false;
                }
            }
            break;
        default:
            PAL_ERR(LOG_TAG, "unsupported direction");
            break;
    }

    if (status)
        goto exit;

    if (sAttr.type == PAL_STREAM_VOICE_UI ||
        sAttr.type == PAL_STREAM_ACD ||
        sAttr.type == PAL_STREAM_ASR ||
        sAttr.type == PAL_STREAM_CONTEXT_PROXY ||
        sAttr.type == PAL_STREAM_ULTRASOUND ||
       (sAttr.type == PAL_STREAM_HAPTICS &&
        sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH)) {
        switch (sAttr.type) {
            case PAL_STREAM_VOICE_UI:
            case PAL_STREAM_CONTEXT_PROXY:
            case PAL_STREAM_ACD:
            case PAL_STREAM_ASR:
            case PAL_STREAM_HAPTICS:
                pcmId = pcmDevIds;
                break;
            case PAL_STREAM_ULTRASOUND:
                pcmId = pcmDevTxIds;
                break;
            default:
                break;
        }
        status = rm->registerMixerEventCallback(pcmId,
                             sessionCb, cbCookie, true);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "Failed to register callback to rm");
        }
    }
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaPcm::setConfig(Stream * s, configType type, uint32_t tag1,
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
    PAL_DBG(LOG_TAG, "Enter tags: %d %d %d", tag1, tag2, tag3);
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
            tagConfig = (struct agm_tag_config*)malloc(sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));
            if (!tagConfig) {
                status = -ENOMEM;
                goto exit;
            }
            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }
            if (pcmDevIds.size() > 0) {
                tagCntrlName << stream << pcmDevIds.at(0) << " " << setParamTagControl;
            } else {
                PAL_ERR(LOG_TAG, "pcmDevIds not found.");
                status = -EINVAL;
                goto exit;
            }
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                status = -ENOENT;
                goto exit;
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
    PAL_DBG(LOG_TAG, "exit status: %d ", status);
    if (tagConfig) {
        free(tagConfig);
    }
    return status;
}

struct mixer_ctl* SessionAlsaPcm::getFEMixerCtl(const char *controlName, int *device, pal_stream_direction_t dir)
{
    std::ostringstream CntrlName;
    struct mixer_ctl *ctl;

    if (dir == PAL_AUDIO_OUTPUT) {
        if (pcmDevIds.size()) {
            *device = pcmDevIds.at(0);
        } else if (pcmDevRxIds.size()) {
            *device = pcmDevRxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            return NULL;
        }
    } else if (dir == PAL_AUDIO_INPUT) {
        if (pcmDevIds.size()) {
            *device = pcmDevIds.at(0);
        } else if (pcmDevTxIds.size()) {
            *device = pcmDevTxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            return NULL;
        }
    }

    CntrlName << "PCM" << *device << " " << controlName;
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        return NULL;
    }

    return ctl;
}

int32_t SessionAlsaPcm::getFrontEndId(uint32_t ldir)
{
    int32_t status = 0;
    int32_t device = -EINVAL;
    struct pal_stream_attributes sAttr = {};

    if (!streamHandle) {
        PAL_ERR(LOG_TAG, "Session handle not found");
        goto exit;
    }
    status = streamHandle->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }

    switch(sAttr.direction) {
    case (PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT):
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
        break;
    default:
        if (pcmDevIds.size())
            device = pcmDevIds.at(0);
    }
exit:
    return device;
}

uint32_t SessionAlsaPcm::getMIID(const char *backendName, uint32_t tagId, uint32_t *miid)
{
    int status = 0;
    int device = 0;
    struct pal_stream_attributes sAttr = {};

    if (!streamHandle) {
        PAL_ERR(LOG_TAG, "Session handle not found");
        status = -EINVAL;
        goto exit;
    }
    status = streamHandle->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }
    if (sAttr.direction == (PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT)) {
        switch (tagId) {
            case DEVICE_HW_ENDPOINT_TX:
            case BT_PLACEHOLDER_DECODER:
            case COP_DEPACKETIZER_V2:
                if (!pcmDevTxIds.size()){
                    PAL_ERR(LOG_TAG, "pcmDevTxIds not found.");
                    status = -EINVAL;
                    goto exit;
                }
                device = pcmDevTxIds.at(0);
                break;
            case DEVICE_HW_ENDPOINT_RX:
            case BT_PLACEHOLDER_ENCODER:
            case COP_PACKETIZER_V2:
            case COP_PACKETIZER_V0:
            case MODULE_SP:
                if (!pcmDevRxIds.size()){
                    PAL_ERR(LOG_TAG, "pcmDevRxIds not found.");
                    status = -EINVAL;
                    goto exit;
                }
                device = pcmDevRxIds.at(0);
                break;
            case RAT_RENDER:
            case BT_PCM_CONVERTER:
                if(strstr(backendName,"TX")) {
                    if (!pcmDevTxIds.size()) {
                        PAL_ERR(LOG_TAG, "pcmDevTxIds not found.");
                        status = -EINVAL;
                        goto exit;
                    }
                    device = pcmDevTxIds.at(0);
                } else {
                    if (!pcmDevRxIds.size()) {
                        PAL_ERR(LOG_TAG, "pcmDevRxIds not found.");
                        status = -EINVAL;
                        goto exit;
                    }
                    device = pcmDevRxIds.at(0);
                }
                break;
            default:
                PAL_INFO(LOG_TAG, "Unsupported loopback tag info %x",tagId);
                status = -EINVAL;
                goto exit;
        }
    } else {
        if (!pcmDevIds.size()){
            PAL_ERR(LOG_TAG, "pcmDevIds not found.");
            status = -EINVAL;
            goto exit;
        }
        device = pcmDevIds.at(0);
    }
    /* REPLACE THIS WITH STORED INFO DURING INITIAL SETUP */
    if (backendName) {
        status = SessionAlsaUtils::getModuleInstanceId(mixer,
            device, backendName, tagId, miid);
    } else {
        status = SessionAlsaUtils::getModuleInstanceId(mixer,
            device, txAifBackEnds[0].second.data(), tagId, miid);
    }

exit:
    if (0 != status)
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);

    return status;
}

int SessionAlsaPcm::setConfig(Stream * s, configType type, int tag)
{
    int status = 0;
    uint32_t tagsent;
    struct agm_tag_config *tagConfig = nullptr;
    struct agm_cal_config *calConfig = nullptr;
    const char *setParamTagControl = "setParamTag";
    const char *stream = "PCM";
    const char *setCalibrationControl = "setCalibration";
    const char *setBEControl = "control";
    struct mixer_ctl *ctl;
    std::ostringstream tagCntrlName;
    std::ostringstream calCntrlName;
    std::ostringstream beCntrlName;
    pal_stream_attributes sAttr = {};
    int tag_config_size = 0;
    int cal_config_size = 0;

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD &&
        sAttr.type != PAL_STREAM_VOICE_CALL_MUSIC  &&
        sAttr.type != PAL_STREAM_CONTEXT_PROXY) {
        if ((sAttr.direction == PAL_AUDIO_OUTPUT && rxAifBackEnds.empty()) ||
            (sAttr.direction == PAL_AUDIO_INPUT && txAifBackEnds.empty())) {
            PAL_ERR(LOG_TAG, "No backend connected to this stream\n");
            return -EINVAL;
        }

        if (PAL_STREAM_LOOPBACK == sAttr.type) {
            if (pcmDevRxIds.size() > 0)
                beCntrlName << stream << pcmDevRxIds.at(0) << " " << setBEControl;
        } else {
            if (pcmDevIds.size() > 0)
                beCntrlName << stream << pcmDevIds.at(0) << " " << setBEControl;
        }

        ctl = mixer_get_ctl_by_name(mixer, beCntrlName.str().data());
        if (!ctl) {
            PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", beCntrlName.str().data());
            return -ENOENT;
        }
        mixer_ctl_set_enum_by_string(ctl, (sAttr.direction == PAL_AUDIO_INPUT) ?
                                     txAifBackEnds[0].second.data() : rxAifBackEnds[0].second.data());
    }

    PAL_DBG(LOG_TAG, "Enter tag: %d", tag);
    switch (type) {
        case MODULE:
            tkv.clear();
            status = builder->populateTagKeyVector(s, tkv, tag, &tagsent);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to set the tag configuration\n");
                goto exit;
            }

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }

            tag_config_size = sizeof(struct agm_tag_config) +
                              tkv.size() * sizeof(struct agm_key_value);
            tagConfig = (struct agm_tag_config*)malloc(tag_config_size);

            if (!tagConfig) {
                status = -EINVAL;
                goto exit;
            }

            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }

            if (PAL_STREAM_LOOPBACK == sAttr.type) {
                if (pcmDevRxIds.size() > 0)
                    tagCntrlName << stream << pcmDevRxIds.at(0) << " " << setParamTagControl;
            } else {
                if (pcmDevIds.size() > 0)
                    tagCntrlName << stream << pcmDevIds.at(0) << " " << setParamTagControl;
            }

            if (tagCntrlName.str().length() == 0) {
                status = -EINVAL;
                goto exit;
            }

            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                status = -ENOENT;
                goto exit;
            }

            status = mixer_ctl_set_array(ctl, tagConfig, tag_config_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
                goto exit;
            }
            ctl = NULL;
            tkv.clear();
            goto exit;
        case CALIBRATION:
            kvMutex.lock();
            ckv.clear();
            if (sAttr.type == PAL_STREAM_VOICE_UI ||
                sAttr.type == PAL_STREAM_ACD ||
                sAttr.type == PAL_STREAM_SENSOR_PCM_DATA ||
                sAttr.type == PAL_STREAM_ASR) {
                status = builder->populateDevicePPCkv(s, ckv);
            } else {
                status = builder->populateCalKeyVector(s, ckv, tag);
            }

            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to set the calibration data\n");
                goto unlock_kvMutex;
            }

            if (ckv.size() == 0) {
                status = -EINVAL;
                goto unlock_kvMutex;
            }

            cal_config_size = sizeof(struct agm_cal_config) +
                              (ckv.size() * sizeof(agm_key_value));
            calConfig = (struct agm_cal_config*)malloc(cal_config_size);

            if (!calConfig) {
                status = -EINVAL;
                goto unlock_kvMutex;
            }

            status = SessionAlsaUtils::getCalMetadata(ckv, calConfig);
            if (PAL_STREAM_LOOPBACK == sAttr.type) {

                if ((sAttr.info.opt_stream_info.loopback_type ==
                                PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) ||
                    (sAttr.info.opt_stream_info.loopback_type ==
                                PAL_STREAM_LOOPBACK_CAPTURE_ONLY)) {
                    // Currently Playback only and Capture only loopback don't
                    // support volume
                    PAL_DBG(LOG_TAG, "RX/TX only Loopback don't support volume");
                    status = -EINVAL;
                    goto unlock_kvMutex;
                }

                if (pcmDevRxIds.size() > 0)
                    calCntrlName << stream << pcmDevRxIds.at(0) << " " << setCalibrationControl;
            } else {
                if (pcmDevIds.size() > 0)
                    calCntrlName << stream << pcmDevIds.at(0) << " " << setCalibrationControl;
            }

            if (calCntrlName.str().length() == 0) {
                status = -EINVAL;
                goto unlock_kvMutex;
            }

            ctl = mixer_get_ctl_by_name(mixer, calCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", calCntrlName.str().data());
                status = -ENOENT;
                goto unlock_kvMutex;
            }

            status = mixer_ctl_set_array(ctl, calConfig, cal_config_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
            }
            ctl = NULL;
            ckv.clear();
            break;
        default:
            PAL_ERR(LOG_TAG, "invalid type %d", type);
            status = -EINVAL;
            goto exit;
    }
unlock_kvMutex:
    if (calConfig)
        free(calConfig);
    kvMutex.unlock();
exit:
    if (tagConfig)
        free(tagConfig);

#ifdef SEC_AUDIO_ADD_FOR_DEBUG
    if (status != 0) {
        PAL_ERR(LOG_TAG, "exit status: %d (failed)", status);
    } else
#endif
    {
        PAL_DBG(LOG_TAG, "exit status: %d ", status);
    }
    return status;
}

int SessionAlsaPcm::setTKV(Stream * s, configType type, effect_pal_payload_t *effectPayload)
{
    int status = 0;
    uint32_t tagsent;
    struct agm_tag_config* tagConfig = nullptr;
    const char *setParamTagControl = "setParamTag";
    const char *stream = "PCM";
    struct mixer_ctl *ctl;
    std::ostringstream tagCntrlName;
    int tkv_size = 0;
    pal_stream_attributes sAttr = {};

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    switch (type) {
        case MODULE:
        {
            tkv.clear();
            pal_key_vector_t *pal_kvpair = (pal_key_vector_t *)effectPayload->payload;
            uint32_t num_tkvs =  pal_kvpair->num_tkvs;
            for (int i = 0; i < num_tkvs; i++) {
                    tkv.push_back(std::make_pair(pal_kvpair->kvp[i].key, pal_kvpair->kvp[i].value));
            }

            if (tkv.size() == 0) {
                status = -EINVAL;
                goto exit;
            }

            tagConfig = (struct agm_tag_config*)malloc(sizeof(struct agm_tag_config) +
                            (tkv.size() * sizeof(agm_key_value)));

            if(!tagConfig) {
                status = -ENOMEM;
                goto exit;
            }

            tagsent = effectPayload->tag;
            status = SessionAlsaUtils::getTagMetadata(tagsent, tkv, tagConfig);
            if (0 != status) {
                goto exit;
            }

            if (PAL_STREAM_LOOPBACK == sAttr.type) {
                if (pcmDevRxIds.size() > 0)
                    tagCntrlName<<stream<<pcmDevRxIds.at(0)<<" "<<setParamTagControl;
            } else {
                if (pcmDevIds.size() > 0)
                    tagCntrlName<<stream<<pcmDevIds.at(0)<<" "<<setParamTagControl;
            }

            if (tagCntrlName.str().length() == 0) {
                PAL_ERR(LOG_TAG, "Invalid tagCntrlName.");
                status = -EINVAL;
                goto exit;
            }

            ctl = mixer_get_ctl_by_name(mixer, tagCntrlName.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlName.str().data());
                status = -ENOENT;
                goto exit;
            }
            PAL_VERBOSE(LOG_TAG, "mixer control: %s\n", tagCntrlName.str().data());

            tkv_size = tkv.size()*sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the tag calibration %d", status);
                goto exit;
            }
            ctl = NULL;
            tkv.clear();

            break;
        }
        default:
            PAL_ERR(LOG_TAG, "invalid type ");
            status = -EINVAL;
            goto exit;
    }

exit:
    PAL_DBG(LOG_TAG, "exit status:%d ", status);
    if (tagConfig) {
        free(tagConfig);
        tagConfig = nullptr;
    }

    return status;
}

void SessionAlsaPcm::deRegisterAdmStream(Stream *s)
{
    if (rm->admDeregisterStreamFn)
        rm->admDeregisterStreamFn(rm->admData, static_cast<void *>(s));
}

void SessionAlsaPcm::registerAdmStream(Stream *s, pal_stream_direction_t dir,
        pal_stream_flags_t flags, struct pcm *pcm, struct pcm_config *cfg)
{
    switch (dir) {
    case PAL_AUDIO_INPUT:
        if (rm->admRegisterInputStreamFn) {
            rm->admRegisterInputStreamFn(rm->admData, static_cast<void *>(s));

            if (flags & PAL_STREAM_FLAG_MMAP_MASK) {
                if (rm->admSetConfigFn)
                    rm->admSetConfigFn(rm->admData, static_cast<void *>(s),
                            pcm, cfg);
            }
        }
        break;
    case PAL_AUDIO_OUTPUT:
        if (rm->admRegisterOutputStreamFn) {
            rm->admRegisterOutputStreamFn(rm->admData, static_cast<void *>(s));

            if (flags & PAL_STREAM_FLAG_MMAP_MASK) {
                if (rm->admSetConfigFn)
                    rm->admSetConfigFn(rm->admData, static_cast<void *>(s),
                            pcm, cfg);
            }
        }
        break;
    default:
        break;
    }
}

void SessionAlsaPcm::requestAdmFocus(Stream *s,  long ns)
{
    if (rm->admRequestFocusV2Fn)
        rm->admRequestFocusV2Fn(rm->admData, static_cast<void *>(s),
                ns);
    else if (rm->admRequestFocusFn)
        rm->admRequestFocusFn(rm->admData, static_cast<void *>(s));
}

void SessionAlsaPcm::AdmRoutingChange(Stream *s)
{
    if (rm->admOnRoutingChangeFn)
        rm->admOnRoutingChangeFn(rm->admData, static_cast<void *>(s));
}

void SessionAlsaPcm::releaseAdmFocus(Stream *s)
{
    if (rm->admAbandonFocusFn)
        rm->admAbandonFocusFn(rm->admData, static_cast<void *>(s));
}

int SessionAlsaPcm::start(Stream * s)
{
    struct pcm_config config = {};
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct pal_device dAttr = {};
    struct pal_media_config codecConfig = {};
    struct sessionToPayloadParam streamData = {};
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    uint32_t miid;
    int payload_size = 0;
    struct agm_event_reg_cfg event_cfg = {};
    struct agm_event_reg_cfg *acd_event_cfg = nullptr;
    struct agm_event_reg_cfg *asr_event_cfg = nullptr;
    int tagId = 0;
    int DeviceId;
    struct disable_lpm_info lpm_info = {};
    bool isStreamAvail = false;
    bool us_notify_format = false;

    PAL_DBG(LOG_TAG, "Enter");

    memset(&dAttr, 0, sizeof(struct pal_device));
#ifndef SEC_AUDIO_EARLYDROP_PATCH    
    rm->voteSleepMonitor(s, true);
#endif    
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }

    /*
     * For VoiceUI streams, multi streams may use same session
     * to handle graph operation, always use valid stream handle
     * to avoid crash if one VoiceUI stream is closed.
     */
    if (sAttr.type == PAL_STREAM_VOICE_UI && streamHandle != s) {
        streamHandle = s;
    }

    if (mState == SESSION_IDLE) {
        s->getBufInfo(&in_buf_size,&in_buf_count,&out_buf_size,&out_buf_count);
        memset(&config, 0, sizeof(config));

        if (sAttr.direction == PAL_AUDIO_INPUT) {
            config.rate = sAttr.in_media_config.sample_rate;
            config.format =
                   SessionAlsaUtils::palToAlsaFormat((uint32_t)sAttr.in_media_config.aud_fmt_id);
            config.channels = sAttr.in_media_config.ch_info.channels;
            config.period_size = SessionAlsaUtils::bytesToFrames(in_buf_size,
                config.channels, config.format);
            config.period_count = in_buf_count;
        } else {
            config.rate = sAttr.out_media_config.sample_rate;
            config.format =
                   SessionAlsaUtils::palToAlsaFormat((uint32_t)sAttr.out_media_config.aud_fmt_id);
            config.channels = sAttr.out_media_config.ch_info.channels;
            config.period_size = SessionAlsaUtils::bytesToFrames(out_buf_size,
                config.channels, config.format);
            config.period_count = out_buf_count;
        }
        config.start_threshold = 0;
        config.stop_threshold = 0;
        config.silence_threshold = 0;
        switch(sAttr.direction) {
            case PAL_AUDIO_INPUT:
                if (pcmDevIds.size() == 0) {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
                if(SessionAlsaUtils::isMmapUsecase(sAttr)) {
                    config.start_threshold = 0;
                    config.stop_threshold = INT32_MAX;
                    config.silence_threshold = 0;
                    config.silence_size = 0;
                    config.avail_min = config.period_size;
                    pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0),
                        PCM_IN |PCM_MMAP| PCM_NOIRQ, &config);
                } else {
                    pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0), PCM_IN, &config);
                }

                if (!pcm) {
                    PAL_ERR(LOG_TAG, "pcm open failed");
                    status = errno;
                    goto exit;
                }

                if (!pcm_is_ready(pcm)) {
                    PAL_ERR(LOG_TAG, "pcm open not ready");
                    status = errno;
                    goto exit;
                }
                break;
            case PAL_AUDIO_OUTPUT:
                if (pcmDevIds.size() == 0) {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
                if(SessionAlsaUtils::isMmapUsecase(sAttr)) {
                    config.start_threshold = config.period_size * 8;
                    config.stop_threshold = INT32_MAX;
                    config.silence_threshold = 0;
                    config.silence_size = 0;
                    config.avail_min = config.period_size;
                    pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0),
                        PCM_OUT |PCM_MMAP| PCM_NOIRQ, &config);
                } else {
                    pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0), PCM_OUT, &config);
                }

                if (!pcm) {
                    PAL_ERR(LOG_TAG, "pcm open failed");
                    status = errno;
                    goto exit;
                }

                if (!pcm_is_ready(pcm)) {
                    PAL_ERR(LOG_TAG, "pcm open not ready");
                    status = errno;
                    goto exit;
                }
                break;
            case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
                if (!pcmDevRxIds.empty()) {
                    pcmRx = pcm_open(rm->getVirtualSndCard(), pcmDevRxIds.at(0), PCM_OUT, &config);
                    if (!pcmRx) {
                        PAL_ERR(LOG_TAG, "pcm-rx open failed");
                        status = errno;
                        goto exit;
                    }

                    if (!pcm_is_ready(pcmRx)) {
                        PAL_ERR(LOG_TAG, "pcm-rx open not ready");
                        status = errno;
                        goto exit;
                    }
                }
                if (!pcmDevTxIds.empty()) {
                    pcmTx = pcm_open(rm->getVirtualSndCard(), pcmDevTxIds.at(0), PCM_IN, &config);
                    if (!pcmTx) {
                        PAL_ERR(LOG_TAG, "pcm-tx open failed");
                        status = errno;
                        goto exit;
                    }

                    if (!pcm_is_ready(pcmTx)) {
                        PAL_ERR(LOG_TAG, "pcm-tx open not ready");
                        status = errno;
                        goto exit;
                    }
                }
                break;
            default :
                PAL_ERR(LOG_TAG, "Exit pcm open failed. Invalid direction");
                status = -EINVAL;
                goto exit;
        }
        mState = SESSION_OPENED;

        if (SessionAlsaUtils::isMmapUsecase(sAttr) &&
                !(sAttr.flags & PAL_STREAM_FLAG_MMAP_NO_IRQ_MASK))
            registerAdmStream(s, sAttr.direction, sAttr.flags, pcm, &config);
    }
    if (sAttr.type == PAL_STREAM_VOICE_UI) {
        payload_size = sizeof(struct agm_event_reg_cfg);
        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 1;
        event_cfg.event_id = s->getCallbackEventId();
        event_cfg.module_instance_id = svaMiid;
        if (pcmDevIds.size() == 0) {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            status = -EINVAL;
            goto exit;
        }
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)&event_cfg, payload_size);

        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 1;
        event_cfg.event_id = EVENT_ID_SH_MEM_PUSH_MODE_EOS_MARKER;
        tagId = SHMEM_ENDPOINT;
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            txAifBackEnds[0].second.data(), tagId, (void *)&event_cfg,
            payload_size);
    } else if (sAttr.type == PAL_STREAM_ULTRASOUND && RegisterForEvents) {
        payload_size = sizeof(struct agm_event_reg_cfg);

        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 1;
        event_cfg.event_id = EVENT_ID_GENERIC_US_DETECTION;
        tagId = ULTRASOUND_DETECTION_MODULE;
        if (!pcmDevTxIds.size()) {
            PAL_ERR(LOG_TAG, "pcmDevTxIds not found.");
            status = -EINVAL;
            goto exit;
        }
        DeviceId = pcmDevTxIds.at(0);
        SessionAlsaUtils::registerMixerEvent(mixer, DeviceId,
                txAifBackEnds[0].second.data(), tagId, (void *)&event_cfg,
                payload_size);
    } else if (sAttr.type == PAL_STREAM_ACD) {
        PAL_DBG(LOG_TAG, "register ACD models");
        SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                            customPayload, customPayloadSize);
        freeCustomPayload();
    } else if (sAttr.type == PAL_STREAM_CONTEXT_PROXY) {
        status = register_asps_event(1);
    } else if (sAttr.type == PAL_STREAM_HAPTICS &&
              sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH) {
        payload_size = sizeof(struct agm_event_reg_cfg);

        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 1;
        event_cfg.event_id = EVENT_ID_WAVEFORM_STATE;
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
                rxAifBackEnds[0].second.data(), MODULE_HAPTICS_GEN, (void *)&event_cfg,
                payload_size);

    } else if (sAttr.type == PAL_STREAM_ASR) {
        payload_size = sizeof(struct agm_event_reg_cfg) + eventPayloadSize;
        asr_event_cfg = (struct agm_event_reg_cfg *)calloc(1, payload_size);
        memset(&event_cfg, 0, sizeof(event_cfg));
        asr_event_cfg->event_config_payload_size = eventPayloadSize;
        asr_event_cfg->is_register = 1;
        asr_event_cfg->event_id = eventId;
        asr_event_cfg->module_instance_id = asrMiid;
        memcpy(asr_event_cfg->event_config_payload, eventPayload, eventPayloadSize);
        if (pcmDevIds.size() == 0) {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            status = -EINVAL;
            goto exit;
        }
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)asr_event_cfg, payload_size);
    }

    switch (sAttr.direction) {
        case PAL_AUDIO_INPUT:
            if (pcmDevIds.size() == 0) {
                PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                status = -EINVAL;
                goto exit;
            }
            if ((sAttr.type != PAL_STREAM_VOICE_UI) &&
                (sAttr.type != PAL_STREAM_ACD) &&
                (sAttr.type != PAL_STREAM_ASR) &&
                (sAttr.type != PAL_STREAM_CONTEXT_PROXY) &&
                (sAttr.type != PAL_STREAM_SENSOR_PCM_DATA) &&
                (sAttr.type != PAL_STREAM_ULTRA_LOW_LATENCY) &&
                (sAttr.type != PAL_STREAM_COMMON_PROXY)) {
                /* Get MFC MIID and configure to match to stream config */
                /* This has to be done after sending all mixer controls and before connect */
                if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD)
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                                                txAifBackEnds[0].second.data(),
                                                                TAG_STREAM_MFC_SR, &miid);
                else
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                                                "ZERO", TAG_STREAM_MFC_SR, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                    goto exit;
                }
                if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD) {
                    PAL_ERR(LOG_TAG, "miid : %x id = %d, data %s\n", miid,
                        pcmDevIds.at(0), txAifBackEnds[0].second.data());
                } else {
                    PAL_ERR(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
                }

                if (isPalPCMFormat(sAttr.in_media_config.aud_fmt_id))
                    streamData.bitWidth = ResourceManager::palFormatToBitwidthLookup(sAttr.in_media_config.aud_fmt_id);
                else
                    streamData.bitWidth = sAttr.in_media_config.bit_width;
                streamData.sampleRate = sAttr.in_media_config.sample_rate;
                streamData.numChannel = sAttr.in_media_config.ch_info.channels;
                streamData.ch_info = nullptr;
                builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
                if (payloadSize && payload) {
                    status = updateCustomPayload(payload, payloadSize);
                    freeCustomPayload(&payload, &payloadSize);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                        goto exit;
                    }
                }

                if (sAttr.type == PAL_STREAM_VOIP_TX) {
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                               txAifBackEnds[0].second.data(), TAG_DEVICEPP_EC_MFC, &miid);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"getModuleInstanceId failed\n");
                        goto set_mixer;
                    }
                    PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
                    status = s->getAssociatedDevices(associatedDevices);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                        goto set_mixer;
                    }
                    for (int i = 0; i < associatedDevices.size();i++) {
                        status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                            goto set_mixer;
                        }
                        if ((dAttr.id == PAL_DEVICE_IN_BLUETOOTH_A2DP) ||
                            (dAttr.id == PAL_DEVICE_IN_BLUETOOTH_BLE) ||
                            (dAttr.id == PAL_DEVICE_IN_BLUETOOTH_SCO_HEADSET)) {
                            struct pal_media_config codecConfig;
                            status = associatedDevices[i]->getCodecConfig(&codecConfig);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG, "getCodecConfig Failed \n");
                                goto set_mixer;
                            }
                            streamData.sampleRate = codecConfig.sample_rate;
                            streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                            streamData.numChannel = 0xFFFF;
                        } else if (dAttr.id == PAL_DEVICE_IN_USB_DEVICE ||
                                   dAttr.id == PAL_DEVICE_IN_USB_HEADSET) {
                            streamData.sampleRate = (dAttr.config.sample_rate % SAMPLINGRATE_8K == 0 &&
                                                     dAttr.config.sample_rate <= SAMPLINGRATE_48K) ?
                                                     dAttr.config.sample_rate : SAMPLINGRATE_48K;
                            streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                            streamData.numChannel = 0xFFFF;
                        } else {
                            streamData.sampleRate = dAttr.config.sample_rate;
                            streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                            streamData.numChannel = 0xFFFF;
                        }
                        builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
                        if (payloadSize && payload) {
                            status = updateCustomPayload(payload, payloadSize);
                            freeCustomPayload(&payload, &payloadSize);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                                goto set_mixer;
                            }
                        }
                    }
                }
#ifdef SEC_AUDIO_CALL_RECORD
                if ((sAttr.type == PAL_STREAM_VOICE_CALL_RECORD) && (sAttr.in_media_config.ch_info.channels == 2)) {
                    // To ensure the channel mapping rules in call recording (L-Rx/R-Tx).
                    builder->payloadMFCMixerCoeff(&payload, &payloadSize, miid,
                                            sAttr.in_media_config.ch_info.channels,
                                            PAL_CALL_RECORD_RX_TX_NON_MIX);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        freeCustomPayload(&payload, &payloadSize);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            goto set_mixer;
                        }
                    }
                }
#endif
                if (sAttr.type == PAL_STREAM_VOIP_TX) {
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                               txAifBackEnds[0].second.data(), DEVICE_MFC, &miid);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"getModuleInstanceId failed\n");
                        goto configure_pspfmfc;
                    }
                    PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
                    status = s->getAssociatedDevices(associatedDevices);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                        goto set_mixer;
                    }
                    for (int i = 0; i < associatedDevices.size();i++) {
                        status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                            goto set_mixer;
                        }
                        if (dAttr.id == PAL_DEVICE_IN_USB_DEVICE || dAttr.id == PAL_DEVICE_IN_USB_HEADSET) {
                            streamData.sampleRate = (dAttr.config.sample_rate % SAMPLINGRATE_8K == 0 &&
                                                     dAttr.config.sample_rate <= SAMPLINGRATE_48K) ?
                                                     dAttr.config.sample_rate : SAMPLINGRATE_48K;
                            streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                            streamData.numChannel = 0xFFFF;
                        } else {
                            streamData.sampleRate = dAttr.config.sample_rate;
                            streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                            streamData.numChannel = 0xFFFF;
                        }
                        builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
                        if (payloadSize && payload) {
                            status = updateCustomPayload(payload, payloadSize);
                            freeCustomPayload(&payload, &payloadSize);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                                goto set_mixer;
                            }
                        }
                    }
                }

configure_pspfmfc:
                status = s->getAssociatedDevices(associatedDevices);
                if (0 != status) {
                    PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                    goto set_mixer;
                }
                if (associatedDevices.size() < 1) {
                    PAL_ERR(LOG_TAG,"no device present\n");
                    goto set_mixer;
                }
                status = associatedDevices[0]->getDeviceAttributes(&dAttr);
                if (0 != status) {
                    PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                    goto set_mixer;
                }
                if (dAttr.id == PAL_DEVICE_IN_PROXY || dAttr.id == PAL_DEVICE_IN_RECORD_PROXY) {
                    status = configureMFC(rm, sAttr, dAttr, pcmDevIds,
                    txAifBackEnds[0].second.data());
                    if(status != 0) {
                         PAL_ERR(LOG_TAG, "configure MFC failed");
                    }
                }

set_mixer:
                status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                             customPayload, customPayloadSize);
                freeCustomPayload();
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    goto exit;
                }
                if (sAttr.type == PAL_STREAM_VOICE_CALL_RECORD) {
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                                                "ZERO", RAT_RENDER, &miid);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                        goto exit;
                    }
                    PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
                    codecConfig.bit_width = sAttr.in_media_config.bit_width;
                    codecConfig.sample_rate = 48000;
                    codecConfig.aud_fmt_id =  sAttr.in_media_config.aud_fmt_id;
                    /* RAT RENDER always set to stereo for uplink+downlink record*/
                    /* As mux_demux gives only stereo o/p & there is no MFC between mux and RAT */
                    if (sAttr.info.voice_rec_info.record_direction == INCALL_RECORD_VOICE_UPLINK_DOWNLINK) {
#ifdef SEC_AUDIO_CALL_ASR_PERFORMANCE
                        if (sAttr.in_media_config.ch_info.channels == 4)
                            codecConfig.ch_info.channels = 4;
                        else
#endif
                        codecConfig.ch_info.channels = 2;
                    } else {
                        /*
                         * RAT needs to be in sync with Mux/Demux o/p.
                         * In case of only UL or DL record, Mux/Demux will provide only 1 channel o/p.
                         * If the recording being done is stereo then there will be a mismatch between RAT and Mux/Demux.
                         * which will lead to noisy clip. Hence, RAT needs to be hard-coded based on record direction.
                         */
                        codecConfig.ch_info.channels = 1;
                    }
                    builder->payloadRATConfig(&payload, &payloadSize, miid, &codecConfig);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        freeCustomPayload(&payload, &payloadSize);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                            goto exit;
                        }
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                                 customPayload, customPayloadSize);
                    freeCustomPayload();
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed for RAT render");
                        goto exit;
                    }
                    switch (sAttr.info.voice_rec_info.record_direction) {
                        case INCALL_RECORD_VOICE_UPLINK:
                            tagId = INCALL_RECORD_UPLINK;
                            break;
                        case INCALL_RECORD_VOICE_DOWNLINK:
                            tagId = INCALL_RECORD_DOWNLINK;
                            break;
                        case INCALL_RECORD_VOICE_UPLINK_DOWNLINK:
                            if (sAttr.in_media_config.ch_info.channels == 2)
                                tagId = INCALL_RECORD_UPLINK_DOWNLINK_STEREO;
#ifdef SEC_AUDIO_CALL_ASR_PERFORMANCE
                            else if (sAttr.in_media_config.ch_info.channels == 4)
                                tagId = INCALL_RECORD_UPLINK_DOWNLINK_QUADRO;
#endif
                            else
                                tagId = INCALL_RECORD_UPLINK_DOWNLINK_MONO;
                            break;
                    }
                    status = setConfig(s, MODULE, tagId);
                    if (status)
                        PAL_ERR(LOG_TAG, "Failed to set incall record params status = %d", status);
                }
            } else if (sAttr.type == PAL_STREAM_VOICE_UI ||
                       sAttr.type == PAL_STREAM_ASR) {
                SessionAlsaUtils::setMixerParameter(mixer,
                    pcmDevIds.at(0), customPayload, customPayloadSize);
                freeCustomPayload();
            } else if (sAttr.type == PAL_STREAM_ACD) {
                if (eventPayload) {
                    PAL_DBG(LOG_TAG, "register ACD events");
                    payload_size = sizeof(struct agm_event_reg_cfg) + eventPayloadSize;

                    acd_event_cfg = (struct agm_event_reg_cfg *)calloc(1, payload_size);
                    if (acd_event_cfg) {
                        acd_event_cfg->event_id = eventId;
                        acd_event_cfg->event_config_payload_size = eventPayloadSize;
                        acd_event_cfg->is_register = 1;
                        memcpy(acd_event_cfg->event_config_payload, eventPayload, eventPayloadSize);
                        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
                                                             txAifBackEnds[0].second.data(),
                                                             CONTEXT_DETECTION_ENGINE,
                                                             (void *)acd_event_cfg,
                                                             payload_size);
                        free(acd_event_cfg);
                    } else {
                        PAL_ERR(LOG_TAG, "get acd_event_cfg instance memory allocation failed");
                        status = -ENOMEM;
                        goto exit;
                    }
                } else {
                    PAL_INFO(LOG_TAG, "eventPayload is NULL");
                }
            } else if (sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                                    txAifBackEnds[0].second.data(),
                                                    TAG_STREAM_MFC_SR, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed\n");
                } else {
                    PAL_DBG(LOG_TAG, "ULL record, miid : %x id = %d\n", miid, pcmDevIds.at(0));
                    if (isPalPCMFormat(sAttr.in_media_config.aud_fmt_id))
                        streamData.bitWidth = ResourceManager::palFormatToBitwidthLookup(sAttr.in_media_config.aud_fmt_id);
                    else
                        streamData.bitWidth = sAttr.in_media_config.bit_width;
                    streamData.sampleRate = sAttr.in_media_config.sample_rate;
                    streamData.numChannel = sAttr.in_media_config.ch_info.channels;
                    streamData.ch_info = nullptr;
                    builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        freeCustomPayload(&payload, &payloadSize);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                            goto exit;
                        }
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                         customPayload, customPayloadSize);
                    freeCustomPayload();
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed");
                        goto exit;
                    }
                }
            } else if (sAttr.type == PAL_STREAM_SENSOR_PCM_DATA) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                           txAifBackEnds[0].second.data(), DEVICE_ADAM, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG,"getModuleInstanceId failed\n");
                } else {
                    status = s->getAssociatedDevices(associatedDevices);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                        goto exit;
                    }
                    if (associatedDevices.empty()) {
                        PAL_ERR(LOG_TAG,"No device attached\n");
                        goto exit;
                    }
                    status = associatedDevices[0]->getDeviceAttributes(&dAttr);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                        goto exit;
                    }
                    if (dAttr.id == PAL_DEVICE_IN_ULTRASOUND_MIC &&
                        dAttr.config.ch_info.channels > 1) {
                        builder->payloadDAMPortConfig(&payload, &payloadSize, miid,
                                                      dAttr.config.ch_info.channels);
                        if (payloadSize && payload) {
                            status = updateCustomPayload(payload, payloadSize);
                            freeCustomPayload(&payload, &payloadSize);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                                goto exit;
                            }
                        }
                        status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                                 customPayload, customPayloadSize);
                        freeCustomPayload();
                        if (status != 0) {
                            PAL_ERR(LOG_TAG, "setMixerParameter failed for RAT render");
                            goto exit;
                        }
                    }
                }
            }

            if (sAttr.type == PAL_STREAM_DEEP_BUFFER) {
               status = s->getAssociatedDevices(associatedDevices);
               if (0 != status) {
                   PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                   goto exit;
               }
               for (int i = 0; i < associatedDevices.size();i++) {
                   status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                   if (0 != status) {
                       PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                       goto exit;
                   }
               }

               //Setting the device orientation during stream open for HDR record.
               if ((dAttr.id == PAL_DEVICE_IN_HANDSET_MIC || dAttr.id == PAL_DEVICE_IN_SPEAKER_MIC)
                       && strstr(dAttr.custom_config.custom_key, "unprocessed-hdr-mic")) {
                   s->setOrientation(HDRConfigKeyToDevOrientation(dAttr.custom_config.custom_key));
                   PAL_DBG(LOG_TAG,"HDR record set device orientation %d", s->getOrientation());
                   if (setConfig(s, MODULE, ORIENTATION_TAG) != 0) {
                       PAL_DBG(LOG_TAG,"HDR record setting device orientation failed");
                   }
                }

#ifdef SEC_AUDIO_BARGEIN_MODE
               if ((dAttr.id == PAL_DEVICE_IN_SPEAKER_MIC)
                    && (strstr(dAttr.custom_config.custom_key, "bargein_aec") ||  strstr(dAttr.custom_config.custom_key, "bargein_tts"))) {
                   s->setBargeinMode(rm->bargein_mode);
                   PAL_DBG(LOG_TAG,"bargein set mode %d", s->getBargeinMode());
                   if (setConfig(s, MODULE, BARGEIN_MODE_TAG)) {
                       PAL_DBG(LOG_TAG, "Bargein mode setting failed");
                   }
                }
#endif
#ifdef SEC_AUDIO_INTERPRETER_MODE
               if (strstr(dAttr.custom_config.custom_key, "interpreter_")) {
                   s->setInterpreterMode(rm->interpreter_mode);
                   PAL_DBG(LOG_TAG,"Interpreter set mode %d", s->getInterpreterMode());
                   if (setConfig(s, MODULE, INTERPRETER_MODE_TAG)) {
                       PAL_DBG(LOG_TAG, "Interpreter mode setting failed");
                   }
                }
#endif
            }

            if (ResourceManager::isSilenceDetectionEnabledPcm) {
#ifdef SEC_AUDIO_MUTE_DETECTION
                bool isRmsTest = false;
#endif
                status = s->getAssociatedDevices(associatedDevices);
                if (0 != status) {
                    PAL_ERR(LOG_TAG,"getAssociatedDevices Failed for Silence Detection\n");
                    goto silence_det_setup_done;
                }

                for (int i = 0; i < associatedDevices.size();i++) {
                    status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"get Device Attributes Failed for Silence Detection\n");
                        goto silence_det_setup_done;
                    }
#ifdef SEC_AUDIO_MUTE_DETECTION
                    if (strstr(dAttr.custom_config.custom_key, "echo_")) {
                        isRmsTest = true;
                    }
#endif
                }

                if (dAttr.id == PAL_DEVICE_IN_HANDSET_MIC || dAttr.id == PAL_DEVICE_IN_SPEAKER_MIC) {
#ifdef SEC_AUDIO_MUTE_DETECTION
                    if (!isRmsTest)
#endif
                    (void) Session::enableSilenceDetection(rm, mixer, pcmDevIds,
                                    txAifBackEnds[0].second.data(), (uint64_t)this);
                }

silence_det_setup_done:
                status = 0;
            }

            if (ResourceManager::isLpiLoggingEnabled()) {
                struct audio_route *audioRoute;

                status = rm->getAudioRoute(&audioRoute);
                if (!status)
                    audio_route_apply_and_update_path(audioRoute, "lpi-pcm-logging");
                PAL_INFO(LOG_TAG, "LPI data logging Param ON");
                /* No error check as TAG/TKV may not required for non LPI usecases */
                setConfig(s, MODULE, LPI_LOGGING_ON);
            }

            if (pcm) {
                status = pcm_start(pcm);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_start failed %d", status);
                    if (ResourceManager::isSilenceDetectionEnabledPcm)
                        (void) Session::disableSilenceDetection(rm, mixer, pcmDevIds, txAifBackEnds[0].second.data(), (uint64_t)this);
                    goto exit;
                }
            }

#ifdef SEC_AUDIO_MUTE_DETECTION
            if (ResourceManager::isSilenceDetectionEnabledPcm) { // after pcm start
                rm->getKernelConfigInfo(TX_CONFIG_INFO);
                rm->getKernelConfigInfo(RX_CONFIG_INFO);
            }
#endif

            break;
        case PAL_AUDIO_OUTPUT:
            if (sAttr.type == PAL_STREAM_VOICE_CALL_MUSIC) {
                if (pcmDevIds.size() == 0) {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
                /*if in call music plus playback configure MFC*/
                if( sAttr.info.incall_music_info.local_playback){
                    status = configureInCallRxMFC();
                }
                if (0 != status) {
                    PAL_INFO(LOG_TAG, "Unable to configure MFC voice call has not started %d", status);
                }
                goto pcm_start;
            }
            if (!rxAifBackEnds.size()) {
                PAL_ERR(LOG_TAG, "rxAifBackEnds are not available");
                status = -EINVAL;
                goto exit;
            }
            if (sAttr.type == PAL_STREAM_HAPTICS && rm->IsHapticsThroughWSA()) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                      rxAifBackEnds[0].second.data(), MODULE_HAPTICS_GEN, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                    goto exit;
                }
                PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));

                if (sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_RINGTONE) {
                    hpCnfg = (pal_param_haptics_cnfg_t *) calloc(1, sizeof(pal_param_haptics_cnfg_t));
                    if (hpCnfg == NULL) {
                        PAL_ERR(LOG_TAG, "Haptics config memory allocation failed.");
                        status = -ENOMEM;
                        goto exit;
                    }
                    hpCnfg->mode = PAL_STREAM_HAPTICS_RINGTONE;
                }
                if (hpCnfg != NULL) {
                    if (hpCnfg->mode == PAL_STREAM_HAPTICS_PCM) {
                        builder->payloadHapticsDevPConfig(&payload, &payloadSize,
                            miid, PARAM_ID_HAPTICS_RX_PCMV_PLAYBACK,(void *)hpCnfg);

                        if (payloadSize && payload) {
                            status = updateCustomPayload(payload, payloadSize);
                            free(payload);
                            if (0 != status) {
                                PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                                free(hpCnfg);
                                goto exit;
                            }
                        }
                    }

                    builder->payloadHapticsDevPConfig(&payload, &payloadSize,
                            miid, PARAM_ID_HAPTICS_WAVE_DESIGNER_CFG,(void *)hpCnfg);

                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        freeCustomPayload(&payload, &payloadSize);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "updateCustomPayload Failed\n");
                            free(hpCnfg);
                            goto exit;
                        }
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                           customPayload, customPayloadSize);
                    freeCustomPayload();
                    free(hpCnfg->buffer_ptr);
                    free(hpCnfg);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed for Haptics wavegen");
                        goto exit;
                    }
                    if (sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH)
                        goto pcm_start;
                }
                else {
                    PAL_ERR(LOG_TAG, "haptics config is not set");
                    goto exit;
                }
            }
            status = s->getAssociatedDevices(associatedDevices);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getAssociatedDevices Failed\n");
                goto exit;
            }
            for (int i = 0; i < associatedDevices.size();i++) {
                status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "get Device Attributes Failed\n");
                    goto exit;
                }

                status = configureMFC(rm, sAttr, dAttr, pcmDevIds,
                            rxAifBackEnds[i].second.data());
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "configure MFC failed");
                    goto exit;
                }
                if (customPayload) {
                    if (pcmDevIds.size() == 0) {
                        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                        status = -EINVAL;
                        goto exit;
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                     customPayload, customPayloadSize);
                    freeCustomPayload();
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed");
                        goto exit;
                    }
                }
                if ((ResourceManager::isChargeConcurrencyEnabled) &&
                    (dAttr.id == PAL_DEVICE_OUT_SPEAKER)) {
                    status = Session::NotifyChargerConcurrency(rm, true);
                    if (0 == status) {
                        status = Session::EnableChargerConcurrency(rm, s);
                        //Handle failure case of ICL config
                        if (0 != status) {
                            PAL_DBG(LOG_TAG, "Failed to set ICL Config status %d", status);
                            status = Session::NotifyChargerConcurrency(rm, false);
                        }
                    }
                    /*
                     Irespective of status, Audio continues to play for success
                     status, PB continues in Buck mode otherwise play in Boost mode.
                   */
                    status = 0;
                }
            }

            if (PAL_DEVICE_OUT_SPEAKER == dAttr.id &&
                ((sAttr.type == PAL_STREAM_LOW_LATENCY) ||
                (sAttr.type == PAL_STREAM_PCM_OFFLOAD) ||
                (sAttr.type == PAL_STREAM_DEEP_BUFFER))) {
                // Set MSPP volume during initlization.
                if (!strcmp(dAttr.custom_config.custom_key, "mspp")) {
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                rxAifBackEnds[0].second.data(), TAG_MODULE_MSPP, &miid);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"get MSPP ModuleInstanceId failed");
                        goto pcm_start;
                    }
                    PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));

                    builder->payloadMSPPConfig(&payload, &payloadSize, miid, rm->linear_gain.gain);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        free(payload);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            goto pcm_start;
                        }
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                                 customPayload, customPayloadSize);
                    if (customPayload) {
                        free(customPayload);
                        customPayload = NULL;
                        customPayloadSize = 0;
                    }
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"setMixerParameter failed for MSPP module");
                        goto pcm_start;
                    }

                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                            rxAifBackEnds[0].second.data(), TAG_PAUSE, &miid);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"get Soft Pause ModuleInstanceId failed");
                        goto pcm_start;
                    }
                    PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));

                    builder->payloadSoftPauseConfig(&payload, &payloadSize, miid,
                                                            MSPP_SOFT_PAUSE_DELAY);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        free(payload);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            goto pcm_start;
                        }
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                                 customPayload, customPayloadSize);
                    if (customPayload) {
                        free(customPayload);
                        customPayload = NULL;
                        customPayloadSize = 0;
                    }
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"setMixerParameter failed for soft Pause module");
                        goto pcm_start;
                    }

                    s->setOrientation(rm->mOrientation);
                    PAL_DBG(LOG_TAG,"MSPP set device orientation %d", s->getOrientation());

                    if (setConfig(s, MODULE, ORIENTATION_TAG) != 0) {
                        PAL_DBG(LOG_TAG,"MSPP setting device orientation failed");
                    }
                } else {
                    pal_param_device_rotation_t rotation;
                    rotation.rotation_type = rm->mOrientation == ORIENTATION_270 ?
                                            PAL_SPEAKER_ROTATION_RL : PAL_SPEAKER_ROTATION_LR;
                    status = handleDeviceRotation(s, rotation.rotation_type, pcmDevIds.at(0),
                                                    mixer, builder, rxAifBackEnds);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"handleDeviceRotation failed\n");
                        status = 0;
                        goto pcm_start;
                    }
                }
            }
            //set voip_rx ec ref MFC config to match with rx stream
            if (sAttr.type == PAL_STREAM_VOIP_RX) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                            rxAifBackEnds[0].second.data(), TAG_DEVICEPP_EC_MFC, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG,"getModuleInstanceId failed\n");
                    status = 0;
                    goto pcm_start;
                }
                PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
                status = s->getAssociatedDevices(associatedDevices);
                if (0 != status) {
                    PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                    status = 0;
                    goto pcm_start;
                }
                for (int i = 0; i < associatedDevices.size();i++) {
                    status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                        status = 0;
                        goto pcm_start;
                    }
                    //NN NS is not enabled for BT right now, need to change bitwidth based on BT config
                    //when anti howling is enabled. Currently returning success if graph does not have
                    //TAG_DEVICEPP_EC_MFC tag
                    if ((dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_A2DP) ||
                        (dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_BLE) ||
                        (dAttr.id == PAL_DEVICE_OUT_BLUETOOTH_SCO)) {
                        struct pal_media_config codecConfig;
                        status = associatedDevices[i]->getCodecConfig(&codecConfig);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "getCodecConfig Failed \n");
                            status = 0;
                            goto pcm_start;
                        }
                        streamData.sampleRate = codecConfig.sample_rate;
                        streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                        streamData.numChannel = 0xFFFF;
                    } else if (dAttr.id == PAL_DEVICE_OUT_USB_DEVICE || dAttr.id == PAL_DEVICE_OUT_USB_HEADSET) {
                        streamData.sampleRate = (dAttr.config.sample_rate % SAMPLINGRATE_8K == 0 &&
                                                     dAttr.config.sample_rate <= SAMPLINGRATE_48K) ?
                                                     dAttr.config.sample_rate : SAMPLINGRATE_48K;
                        streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                        streamData.numChannel = 0xFFFF;
                    } else {
                        streamData.sampleRate = dAttr.config.sample_rate;
                        streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                        streamData.numChannel = 0xFFFF;
                    }
                    builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        freeCustomPayload(&payload, &payloadSize);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            status = 0;
                            goto pcm_start;
                        }
                    }
                }
                status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                             customPayload, customPayloadSize);
                freeCustomPayload();
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    status = 0;
                    goto pcm_start;
                }
            }
            if (sAttr.type == PAL_STREAM_VOIP_RX) {
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, pcmDevIds.at(0),
                                               rxAifBackEnds[0].second.data(), DEVICE_MFC, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG,"getModuleInstanceId failed\n");
                    status = 0;
                    goto pcm_start;
                }
                PAL_INFO(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
                status = s->getAssociatedDevices(associatedDevices);
                if (0 != status) {
                    PAL_ERR(LOG_TAG,"getAssociatedDevices Failed\n");
                    status = 0;
                    goto pcm_start;
                }
                for (int i = 0; i < associatedDevices.size();i++) {
                    status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                    if (0 != status) {
                        PAL_ERR(LOG_TAG,"get Device Attributes Failed\n");
                        status = 0;
                        goto pcm_start;
                    }
                    if (dAttr.id == PAL_DEVICE_OUT_USB_DEVICE || dAttr.id == PAL_DEVICE_OUT_USB_HEADSET) {
                        streamData.sampleRate = (dAttr.config.sample_rate % SAMPLINGRATE_8K == 0 &&
                                                     dAttr.config.sample_rate <= SAMPLINGRATE_48K) ?
                                                     dAttr.config.sample_rate : SAMPLINGRATE_48K;
                        streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                        streamData.numChannel = 0xFFFF;
                    } else {
                        streamData.sampleRate = dAttr.config.sample_rate;
                        streamData.bitWidth   = AUDIO_BIT_WIDTH_DEFAULT_16;
                        streamData.numChannel = 0xFFFF;
                    }
                    builder->payloadMFCConfig(&payload, &payloadSize, miid, &streamData);
                    if (payloadSize && payload) {
                        status = updateCustomPayload(payload, payloadSize);
                        freeCustomPayload(&payload, &payloadSize);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
                            status = 0;
                            goto pcm_start;
                        }
                    }
                }
                status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                             customPayload, customPayloadSize);
                freeCustomPayload();
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    status = 0;
                    goto pcm_start;
                }
            }
pcm_start:
            if (sAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER) {
                if (rm->activeGroupDevConfig) {
                    if ((dAttr.config.sample_rate !=
                         rm->currentGroupDevConfig.grp_dev_hwep_cfg.sample_rate) ||
                        (dAttr.config.ch_info.channels !=
                         rm->currentGroupDevConfig.grp_dev_hwep_cfg.channels) ||
                        (dAttr.config.bit_width != ResourceManager::palFormatToBitwidthLookup(
                         rm->currentGroupDevConfig.grp_dev_hwep_cfg.aud_fmt_id))) {
                         us_notify_format = true;
                         dAttr.config.sample_rate =
                             rm->currentGroupDevConfig.grp_dev_hwep_cfg.sample_rate;
                         dAttr.config.ch_info.channels =
                             rm->currentGroupDevConfig.grp_dev_hwep_cfg.channels;
                         dAttr.config.bit_width = ResourceManager::palFormatToBitwidthLookup(
                             rm->currentGroupDevConfig.grp_dev_hwep_cfg.aud_fmt_id);
                    }
                } else if (dAttr.id != PAL_DEVICE_OUT_ULTRASOUND_DEDICATED) {
                    us_notify_format = true;
                }
            }
            if (us_notify_format) {
                status = notifyUPDToneRendererFmtChng(&dAttr,
                            US_TONE_RENDERER_EP_MEDIA_FORMAT_INFO_CHANGE_START);
                if (status) {
                    PAL_ERR(LOG_TAG, "Error notifying Ultrasound tone renderer "
                            "format change START. status = %d", status);
                    goto exit;
                }
            }
            setInitialVolume();
            memset(&lpm_info, 0, sizeof(struct disable_lpm_info));
            rm->getDisableLpmInfo(&lpm_info);
            isStreamAvail = (find(lpm_info.streams_.begin(),
                            lpm_info.streams_.end(), sAttr.type) !=
                            lpm_info.streams_.end());
            if (isStreamAvail && lpm_info.isDisableLpm) {
                std::lock_guard<std::mutex> lock(pcmLpmRefCntMtx);
                PAL_DBG(LOG_TAG,"pcmLpmRefCnt %d\n", pcmLpmRefCnt);
                pcmLpmRefCnt++;
                if (1 == pcmLpmRefCnt)
                    setPmQosMixerCtl(PM_QOS_VOTE_ENABLE);
                else
                    PAL_DBG(LOG_TAG,"pcmLpmRefCnt %d\n", pcmLpmRefCnt);
            }

#ifdef SEC_AUDIO_MUTE_DETECTION
            if (ResourceManager::isSilenceDetectionEnabledPcm && sAttr.type == PAL_STREAM_VOIP_RX) {
                for (int idx = 0; idx < associatedDevices.size(); idx++) {
                    int dev_id = associatedDevices[idx]->getSndDeviceId();
                    if (rm->isOutputDevId(dev_id)) {
                        status = associatedDevices[idx]->getDeviceAttributes(&dAttr);
                        if (0 != status) {
                            PAL_ERR(LOG_TAG, "getDeviceAttributes failed for Silence Detection\n");
                            status = 0;
                            break;
                        }
                        if (strstr(dAttr.custom_config.custom_key, "wifi_call") &&
                            ((dAttr.id == PAL_DEVICE_OUT_HANDSET) || (dAttr.id == PAL_DEVICE_OUT_SPEAKER))) {
                            (void) Session::enableSilenceDetection(rm, mixer,
                                pcmDevIds, rxAifBackEnds[0].second.data(), (uint64_t)this, EVENT_ID_DV_MUTE_DETECTION);
                        }
                        break;
                    }
                }
            }
 #endif

            if (pcm) {
                status = pcm_start(pcm);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_start failed %d", status);
#ifdef SEC_AUDIO_MUTE_DETECTION
                    if (ResourceManager::isSilenceDetectionEnabledPcm && sAttr.type == PAL_STREAM_VOIP_RX) {
                        (void) Session::disableSilenceDetection(rm, mixer,
                            pcmDevIds, rxAifBackEnds[0].second.data(), (uint64_t)this, EVENT_ID_DV_MUTE_DETECTION);
                    }
#endif
                } else {
                    if (us_notify_format) {
                        status = notifyUPDToneRendererFmtChng(&dAttr,
                                    US_TONE_RENDERER_EP_MEDIA_FORMAT_INFO_CHANGE_DONE);
                        if (status) {
                            PAL_ERR(LOG_TAG, "Error notifying Ultrasound tone renderer "
                                    "format change START. status = %d", status);
                            goto exit;
                        }
                    }
#ifdef SEC_AUDIO_MUTE_DETECTION
                    for (auto &dev: associatedDevices) {
                        int dev_id = dev->getSndDeviceId();
                        if (rm->isOutputDevId(dev_id)) {
                            struct pal_device devAttr = {};
                            int ret = dev->getDeviceAttributes(&devAttr);
                            if (ret == 0 && ((devAttr.id == PAL_DEVICE_OUT_HANDSET) || (devAttr.id == PAL_DEVICE_OUT_SPEAKER))) {
                                if (dev->getDeviceCount() == 1) {
                                    rm->getKernelConfigInfo(RX_CONFIG_INFO); // after pcm start.
                                    break;
                                }
                            }
                        }
                    }
#endif
                }
            }

            if (!status && isMixerEventCbRegd && !isPauseRegistrationDone) {
                // Stream supports Soft Pause and registration with RM is
                // successful. So register for Soft pause callback from adsp.
                payload_size = sizeof(struct agm_event_reg_cfg);
                memset(&event_cfg, 0, sizeof(event_cfg));

                event_cfg.event_id = EVENT_ID_SOFT_PAUSE_PAUSE_COMPLETE;
                event_cfg.event_config_payload_size = 0;
                event_cfg.is_register = 1;

                if (pcmDevIds.size() == 0) {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
                status = SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
                            rxAifBackEnds[0].second.data(), TAG_PAUSE, (void *)&event_cfg,
                            payload_size);
                if (status == 0) {
                    isPauseRegistrationDone = true;
                } else {
                    // Not a fatal error
                    PAL_ERR(LOG_TAG, "Register for Pause failed %d", status);
                    // If registration fails for this then pop issue will come.
                    // It isn't fatal so not throwing error.
                    status = 0;
                }
            }
            break;
        case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
            if (!rxAifBackEnds.size()) {
                PAL_ERR(LOG_TAG, "rxAifBackEnds are not available");
                status = -EINVAL;
                goto exit;
            }
            status = s->getAssociatedDevices(associatedDevices);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getAssociatedDevices Failed");
                goto exit;
            }
            for (int i = 0; i < associatedDevices.size(); i++) {
                if (!SessionAlsaUtils::isRxDevice(
                            associatedDevices[i]->getSndDeviceId()))
                    continue;

                status = associatedDevices[i]->getDeviceAttributes(&dAttr);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "get Device Attributes Failed");
                    goto exit;
                }
                status = configureMFC(rm, sAttr, dAttr, pcmDevRxIds,
                            rxAifBackEnds[0].second.data());
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "configure MFC failed");
                    goto exit;
                }
                if (customPayload) {
                    if (!pcmDevRxIds.size()) {
                        PAL_ERR(LOG_TAG, "pcmDevRxIds not found.");
                        status = -EINVAL;
                        goto exit;
                    }
                    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevRxIds.at(0),
                                                             customPayload, customPayloadSize);
                    freeCustomPayload();
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed");
                        goto exit;
                    }
                }
                if ((ResourceManager::isChargeConcurrencyEnabled) &&
                    (dAttr.id == PAL_DEVICE_OUT_SPEAKER)) {
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

            if (pcmRx) {
                status = pcm_start(pcmRx);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_start rx failed %d", status);
                }
            }
            if (pcmTx) {
                status = pcm_start(pcmTx);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_start tx failed %d", status);
                    goto exit;
                }
            }
           break;
    }
    if (sAttr.direction != PAL_AUDIO_OUTPUT) {
        setInitialVolume();
    }
    mState = SESSION_STARTED;

exit:
#ifndef SEC_AUDIO_EARLYDROP_PATCH
    if (status != 0)
        rm->voteSleepMonitor(s, false);
#endif
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaPcm::stop(Stream * s)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct agm_event_reg_cfg event_cfg;
    int payload_size = 0;
    int tagId;
    int DeviceId;

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }
    switch (sAttr.direction) {
        case PAL_AUDIO_INPUT:
            if (pcm && isActive()) {
                status = pcm_stop(pcm);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_stop failed %d", status);
                }
            }
            if (ResourceManager::isLpiLoggingEnabled()) {
                struct audio_route *audioRoute;

                status = rm->getAudioRoute(&audioRoute);
                if (!status)
                    audio_route_reset_and_update_path(audioRoute, "lpi-pcm-logging");
            }

            if (ResourceManager::isSilenceDetectionEnabledPcm) {
#ifdef SEC_AUDIO_MUTE_DETECTION
                if (pcmDevIds.size() > 0 && txAifBackEnds.size() > 0)
#endif
                (void) Session::disableSilenceDetection(rm, mixer, pcmDevIds, txAifBackEnds[0].second.data(), (uint64_t)this);
            }
        break;
        case PAL_AUDIO_OUTPUT:
            if (pcm && isActive()) {
                status = pcm_stop(pcm);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_stop failed %d", status);
                }
            }

            if (isPauseRegistrationDone) {
                // Stream supports Soft Pause and was registered with RM
                // sucessfully. Thus Deregister callback for Soft Pause
                payload_size = sizeof(struct agm_event_reg_cfg);

                memset(&event_cfg, 0, sizeof(event_cfg));
                event_cfg.event_id = EVENT_ID_SOFT_PAUSE_PAUSE_COMPLETE;
                event_cfg.event_config_payload_size = 0;
                event_cfg.is_register = 0;

                if (!pcmDevIds.size()) {
                    PAL_ERR(LOG_TAG, "frontendIDs are not available");
                    status = -EINVAL;
                    goto exit;
                }
                if (!rxAifBackEnds.size()) {
                    PAL_ERR(LOG_TAG, "rxAifBackEnds are not available");
                    status = -EINVAL;
                    goto exit;
                }
                status = SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
                        rxAifBackEnds[0].second.data(), TAG_PAUSE, (void *)&event_cfg,
                        payload_size);
                if (status == 0 || rm->cardState == CARD_STATUS_OFFLINE) {
                    isPauseRegistrationDone = false;
                } else {
                    // Not a fatal error
                    PAL_ERR(LOG_TAG, "Pause deregistration failed");
                    status = 0;
                }
            }

#ifdef SEC_AUDIO_MUTE_DETECTION
            if (ResourceManager::isSilenceDetectionEnabledPcm
                && sAttr.type == PAL_STREAM_VOIP_RX && pcmDevIds.size() > 0 && rxAifBackEnds.size() > 0) {
                (void) Session::disableSilenceDetection(rm, mixer,
                    pcmDevIds, rxAifBackEnds[0].second.data(), (uint64_t)this, EVENT_ID_DV_MUTE_DETECTION);
            }
#endif
            break;
        case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
            if (pcmRx && isActive()) {
                status = pcm_stop(pcmRx);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_stop - rx failed %d", status);
                }
            }
            if (pcmTx && isActive()) {
                status = pcm_stop(pcmTx);
                if (status) {
                    status = errno;
                    PAL_ERR(LOG_TAG, "pcm_stop - tx failed %d", status);
                }
            }
            break;
    }
#ifndef SEC_AUDIO_EARLYDROP_PATCH    
    rm->voteSleepMonitor(s, false);
#endif    
    mState = SESSION_STOPPED;

    if (sAttr.type == PAL_STREAM_VOICE_UI) {
        payload_size = sizeof(struct agm_event_reg_cfg);
        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 0;
        event_cfg.event_id = s->getCallbackEventId();
        event_cfg.module_instance_id = svaMiid;
        if (!pcmDevIds.size()) {
            PAL_ERR(LOG_TAG, "pcmDevIds not found.");
            status = -EINVAL;
            goto exit;
        }
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)&event_cfg, payload_size);

        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 0;
        event_cfg.event_id = EVENT_ID_SH_MEM_PUSH_MODE_EOS_MARKER;
        tagId = SHMEM_ENDPOINT;
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            txAifBackEnds[0].second.data(), tagId, (void *)&event_cfg,
            payload_size);

    } else if (sAttr.type == PAL_STREAM_ULTRASOUND && RegisterForEvents) {
        payload_size = sizeof(struct agm_event_reg_cfg);
        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 0;
        event_cfg.event_id = EVENT_ID_GENERIC_US_DETECTION;
        tagId = ULTRASOUND_DETECTION_MODULE;
        if (!pcmDevTxIds.size()) {
            PAL_ERR(LOG_TAG, "pcmDevTxIds not found.");
            status = -EINVAL;
            goto exit;
        }
        DeviceId = pcmDevTxIds.at(0);
        RegisterForEvents = false;
        SessionAlsaUtils::registerMixerEvent(mixer, DeviceId,
                txAifBackEnds[0].second.data(), tagId, (void *)&event_cfg,
                payload_size);
    } else if (sAttr.type == PAL_STREAM_ACD || sAttr.type == PAL_STREAM_ASR) {
        if (eventPayload == NULL) {
            PAL_INFO(LOG_TAG, "eventPayload is NULL");
            goto exit;
        }

        payload_size = sizeof(struct agm_event_reg_cfg);
        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_id = eventId;
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 0;
        tagId = sAttr.type == PAL_STREAM_ACD ? CONTEXT_DETECTION_ENGINE :
                                               TAG_MODULE_ASR;
        if (!txAifBackEnds.empty()) {
            if (!pcmDevIds.size()) {
                PAL_ERR(LOG_TAG, "pcmDevIds not found.");
                status = -EINVAL;
                goto exit;
            }
            SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
                    txAifBackEnds[0].second.data(), tagId, (void *)&event_cfg,
                    payload_size);
        }
    } else if(sAttr.type == PAL_STREAM_CONTEXT_PROXY) {
        status = register_asps_event(0);
    } else if(sAttr.type == PAL_STREAM_HAPTICS &&
              sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH) {
        payload_size = sizeof(struct agm_event_reg_cfg);

        memset(&event_cfg, 0, sizeof(event_cfg));
        event_cfg.event_config_payload_size = 0;
        event_cfg.is_register = 0;
        event_cfg.event_id = EVENT_ID_WAVEFORM_STATE;
        SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
                rxAifBackEnds[0].second.data(), MODULE_HAPTICS_GEN, (void *)&event_cfg,
                payload_size);
    }
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int SessionAlsaPcm::close(Stream * s)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    std::string backendname;
    int32_t beDevId = 0;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    int ldir = 0;
    std::vector<int> pcmId;
    struct disable_lpm_info lpm_info;
    bool isStreamAvail = false;
    int devCount = 0;

    PAL_DBG(LOG_TAG, "Enter");
    if (!frontEndIdAllocated) {
        PAL_DBG(LOG_TAG, "Session not opened or already closed");
        goto exit;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }
    if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD &&
        sAttr.type != PAL_STREAM_VOICE_CALL_MUSIC  &&
        sAttr.type != PAL_STREAM_CONTEXT_PROXY) {
        status = s->getAssociatedDevices(associatedDevices);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed\n");
            goto exit;
        }
    }
    freeDeviceMetadata.clear();

    switch (sAttr.direction) {
        case PAL_AUDIO_INPUT:
            for (auto &dev: associatedDevices) {
                beDevId = dev->getSndDeviceId();
                rm->getBackendName(beDevId, backendname);
                PAL_DBG(LOG_TAG, "backendname %s", backendname.c_str());
                if (dev->getDeviceCount() > 1) {
                    PAL_DBG(LOG_TAG, "Tx dev still active\n");
                    freeDeviceMetadata.push_back(std::make_pair(backendname, 0));
                } else {
                    freeDeviceMetadata.push_back(std::make_pair(backendname, 1));
                    PAL_DBG(LOG_TAG, "Tx dev not active");
                }
            }
            status = SessionAlsaUtils::close(s, rm, pcmDevIds, txAifBackEnds, freeDeviceMetadata);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
            }
            if (SessionAlsaUtils::isMmapUsecase(sAttr) &&
                !(sAttr.flags & PAL_STREAM_FLAG_MMAP_NO_IRQ_MASK))
                deRegisterAdmStream(s);
            if (pcm)
                status = pcm_close(pcm);
            if (status) {
                status = errno;
                PAL_ERR(LOG_TAG, "pcm_close failed %d", status);
            }
            if (sAttr.type == PAL_STREAM_ACD ||
                sAttr.type == PAL_STREAM_ASR ||
                sAttr.type == PAL_STREAM_SENSOR_PCM_DATA)
                ldir = TX_HOSTLESS;

            rm->freeFrontEndIds(pcmDevIds, sAttr, ldir);
            pcm = NULL;

#ifdef SEC_AUDIO_MUTE_DETECTION
            if (ResourceManager::isSilenceDetectionEnabledPcm) {
                rm->getKernelConfigInfo(TX_CONFIG_INFO); // after pcm_stop
                rm->getKernelConfigInfo(RX_CONFIG_INFO);
            }
#endif

            break;
        case PAL_AUDIO_OUTPUT:
            for (auto &dev: associatedDevices) {
                beDevId = dev->getSndDeviceId();
                rm->getBackendName(beDevId, backendname);
                PAL_DBG(LOG_TAG, "backendname %s", backendname.c_str());
                devCount = dev->getDeviceCount();
                // Do not clear device metadata for A2DP device if SCO device is active
                if ((devCount == 1) && rm->isBtA2dpDevice((pal_device_id_t) beDevId))
                    devCount += SessionAlsaUtils::getScoDevCount();
                if (devCount > 1) {
                    PAL_DBG(LOG_TAG, "Rx dev still active");
                    freeDeviceMetadata.push_back(std::make_pair(backendname, 0));
                } else {
                    PAL_DBG(LOG_TAG, "Rx dev not active");
                    freeDeviceMetadata.push_back(std::make_pair(backendname, 1));
                }
            }
            status = SessionAlsaUtils::close(s, rm, pcmDevIds, rxAifBackEnds, freeDeviceMetadata);
            if (status) {
                PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
            }
            if (SessionAlsaUtils::isMmapUsecase(sAttr) &&
                !(sAttr.flags & PAL_STREAM_FLAG_MMAP_NO_IRQ_MASK))
                deRegisterAdmStream(s);

            memset(&lpm_info, 0, sizeof(struct disable_lpm_info));
            rm->getDisableLpmInfo(&lpm_info);
            isStreamAvail = (find(lpm_info.streams_.begin(),
                            lpm_info.streams_.end(), sAttr.type) !=
                            lpm_info.streams_.end());
            if (isStreamAvail && lpm_info.isDisableLpm) {
                std::lock_guard<std::mutex> lock(pcmLpmRefCntMtx);
                PAL_DBG(LOG_TAG, "pcm_close pcmLpmRefCnt %d", pcmLpmRefCnt);
                pcmLpmRefCnt--;
                if (pcmLpmRefCnt < 0) { //May not happen, to catch the error, if it happens to be
                    PAL_ERR(LOG_TAG, "pcm_close Unacceptable pcmLpmRefCnt %d, resetting to 0", pcmLpmRefCnt);
                    pcmLpmRefCnt = 0;
                }
                if (0 == pcmLpmRefCnt)
                    setPmQosMixerCtl(PM_QOS_VOTE_DISABLE);
                PAL_DBG(LOG_TAG, "pcm_close pcmLpmRefCnt %d", pcmLpmRefCnt);
            }

            if (pcm)
                status = pcm_close(pcm);
            if (status) {
                status = errno;
                PAL_ERR(LOG_TAG, "pcm_close failed %d", status);
            }
            // Deregister callback for Mixer Event
            if (!status && isMixerEventCbRegd) {
                status = rm->registerMixerEventCallback(pcmDevIds,
                    sessionCb, cbCookie, false);
                if (status == 0) {
                    isMixerEventCbRegd = false;
                } else {
                    // Not a fatal error
                    PAL_ERR(LOG_TAG, "Failed to deregister callback to rm");
                    status = 0;
                }
            }
            if ((sAttr.type == PAL_STREAM_HAPTICS &&
                 sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH) ||
                (sAttr.type == PAL_STREAM_LOOPBACK &&
                 sAttr.info.opt_stream_info.loopback_type == PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) ||
                (sAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER))
                ldir = RX_HOSTLESS;

            rm->freeFrontEndIds(pcmDevIds, sAttr, ldir);
#ifdef SEC_AUDIO_MUTE_DETECTION
            for (auto &dev: associatedDevices) {
                int dev_id = dev->getSndDeviceId();
                if (rm->isOutputDevId(dev_id)) {
                    struct pal_device devAttr = {};
                    int ret = dev->getDeviceAttributes(&devAttr);
                    if (ret == 0 && ((devAttr.id == PAL_DEVICE_OUT_HANDSET) || (devAttr.id == PAL_DEVICE_OUT_SPEAKER))) {
                        if (dev->getDeviceCount() == 1) {
                            rm->getKernelConfigInfo(RX_CONFIG_INFO); // after pcm stop
                            break;
                        }
                    }
                }
            }
#endif
            pcm = NULL;
            break;
        case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
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
            if (sAttr.info.opt_stream_info.loopback_type ==
                    PAL_STREAM_LOOPBACK_CAPTURE_ONLY) {
                status = SessionAlsaUtils::close(s, rm, pcmDevTxIds, txAifBackEnds, freeDeviceMetadata);
                if (status) {
                    PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
                }
            }
            else if (sAttr.info.opt_stream_info.loopback_type ==
                       PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) {
                status = SessionAlsaUtils::close(s, rm, pcmDevRxIds, rxAifBackEnds, freeDeviceMetadata);
                if (status) {
                    PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
                }
            }
            else {
                status = SessionAlsaUtils::close(s, rm, pcmDevRxIds, pcmDevTxIds,
                        rxAifBackEnds, txAifBackEnds, freeDeviceMetadata);
                if (status) {
                    PAL_ERR(LOG_TAG, "session alsa close failed with %d", status);
                }
           }
            if (pcmRx)
                status = pcm_close(pcmRx);
            if (status) {
                status = errno;
                PAL_ERR(LOG_TAG, "pcm_close - rx failed %d", status);
            }
            if (pcmTx)
                status = pcm_close(pcmTx);
            if (status) {
                status = errno;
               PAL_ERR(LOG_TAG, "pcm_close - tx failed %d", status);
            }

            if (pcmDevRxIds.size())
                rm->freeFrontEndIds(pcmDevRxIds, sAttr, RX_HOSTLESS);
            if (pcmDevTxIds.size())
                rm->freeFrontEndIds(pcmDevTxIds, sAttr, TX_HOSTLESS);
            pcmRx = NULL;
            pcmTx = NULL;
            break;
    }
    frontEndIdAllocated = false;
    mState = SESSION_IDLE;

    if (sAttr.type == PAL_STREAM_VOICE_UI ||
        sAttr.type == PAL_STREAM_ACD ||
        sAttr.type == PAL_STREAM_ASR ||
        sAttr.type == PAL_STREAM_CONTEXT_PROXY ||
        sAttr.type == PAL_STREAM_ULTRASOUND ||
        (sAttr.type == PAL_STREAM_HAPTICS &&
        sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH)) {
        switch (sAttr.type) {
            case PAL_STREAM_VOICE_UI:
            case PAL_STREAM_ACD:
            case PAL_STREAM_ASR:
            case PAL_STREAM_CONTEXT_PROXY:
            case PAL_STREAM_HAPTICS:
                pcmId = pcmDevIds;
                break;
            case PAL_STREAM_ULTRASOUND:
                pcmId = pcmDevTxIds;
                break;
            default:
                break;
        }
        status = rm->registerMixerEventCallback(pcmId,
                      sessionCb, cbCookie, false);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "Failed to deregister callback to rm");
        }
    }

    freeCustomPayload();
    if (eventPayload) {
        free(eventPayload);
        eventPayload = NULL;
        eventPayloadSize = 0;
        eventId = 0;
    }
exit:
    ecRefDevId = PAL_DEVICE_OUT_MIN;
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

/* TODO: Check if this can be moved to Session class */
int SessionAlsaPcm::disconnectSessionDevice(Stream *streamHandle,
        pal_stream_type_t streamType, std::shared_ptr<Device> deviceToDisconnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    struct pal_device dAttr = {};
    std::vector<std::pair<int32_t, std::string>> rxAifBackEndsToDisconnect;
    std::vector<std::pair<int32_t, std::string>> txAifBackEndsToDisconnect;
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;

    deviceList.push_back(deviceToDisconnect);
    rm->getBackEndNames(deviceList, rxAifBackEndsToDisconnect,
            txAifBackEndsToDisconnect);
    deviceToDisconnect->getDeviceAttributes(&dAttr);

    if (streamType == PAL_STREAM_SENSOR_PCM_RENDERER) {
        status = notifyUPDToneRendererFmtChng(&dAttr,
                    US_TONE_RENDERER_EP_MEDIA_FORMAT_INFO_CHANGE_START);
        if (status) {
            PAL_ERR(LOG_TAG, "Error notifying Ultrasound tone renderer "
                    "format change START. status = %d", status);
        } else {
            /*sleep for 20ms to wait for EOS propagation to HWEP*/
            usleep(20000);
        }
    }

    status = streamHandle->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
    }
    if (!rxAifBackEndsToDisconnect.empty()) {
        int cnt = 0;
        if ((streamType != PAL_STREAM_ULTRASOUND &&
            streamType != PAL_STREAM_LOOPBACK) || (streamType == PAL_STREAM_LOOPBACK &&
            sAttr.info.opt_stream_info.loopback_type == PAL_STREAM_LOOPBACK_PLAYBACK_ONLY))
            status = SessionAlsaUtils::disconnectSessionDevice(streamHandle, streamType, rm,
                     dAttr, (pcmDevIds.size() ? pcmDevIds : pcmDevRxIds), rxAifBackEndsToDisconnect);
        else {
            if (PAL_STREAM_ULTRASOUND == streamType) {
                status = setParameters(streamHandle, DEVICE_POP_SUPPRESSOR,
                                        PAL_PARAM_ID_ULTRASOUND_RAMPDOWN, NULL);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "SetParameters failed for Rampdown, status = %d", status);
                }
                /* TODO: Need to adjust the delay based on requirement */
                usleep(20000);
            }

            status = SessionAlsaUtils::disconnectSessionDevice(streamHandle, streamType, rm,
                     dAttr, pcmDevTxIds, pcmDevRxIds, rxAifBackEndsToDisconnect);
        }

        for (const auto &elem : rxAifBackEnds) {
            cnt++;
            for (const auto &disConnectElem : rxAifBackEndsToDisconnect) {
                if (std::get<0>(elem) == std::get<0>(disConnectElem)) {
                    PAL_DBG(LOG_TAG, "Removed BE for dev %d from AIF list", std::get<0>(disConnectElem));
                    rxAifBackEnds.erase(rxAifBackEnds.begin() + cnt - 1, rxAifBackEnds.begin() + cnt);
                    cnt--;
                    break;
                } else if (&disConnectElem == &rxAifBackEndsToDisconnect.back()) {
                    PAL_ERR(LOG_TAG, "Failed to remove BE for dev %d from AIF list", std::get<0>(disConnectElem));
                }
            }
        }
    }
    if (!txAifBackEndsToDisconnect.empty()) {
        int cnt = 0;
        if (streamType != PAL_STREAM_LOOPBACK)
            status = SessionAlsaUtils::disconnectSessionDevice(streamHandle, streamType, rm,
                     dAttr, (pcmDevIds.size() ? pcmDevIds : pcmDevTxIds), txAifBackEndsToDisconnect);
        else
            status = SessionAlsaUtils::disconnectSessionDevice(streamHandle, streamType, rm,
                     dAttr, pcmDevTxIds, pcmDevRxIds, txAifBackEndsToDisconnect);

        for (const auto &elem : txAifBackEnds) {
            cnt++;
            for (const auto &disConnectElem : txAifBackEndsToDisconnect) {
                if (std::get<0>(elem) == std::get<0>(disConnectElem)) {
                    PAL_DBG(LOG_TAG, "Removed BE for dev %d from AIF list", std::get<0>(disConnectElem));
                    txAifBackEnds.erase(txAifBackEnds.begin() + cnt - 1, txAifBackEnds.begin() + cnt);
                    cnt--;
                    break;
                } else if (&disConnectElem == &txAifBackEndsToDisconnect.back()) {
                    PAL_ERR(LOG_TAG, "Failed to remove BE for dev %d from AIF list", std::get<0>(disConnectElem));
                }
            }
        }
    }

    return status;
}

int SessionAlsaPcm::setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    struct pal_device dAttr = {};
    std::vector<std::pair<int32_t, std::string>> rxAifBackEndsToConnect;
    std::vector<std::pair<int32_t, std::string>> txAifBackEndsToConnect;
    int32_t status = 0;

    deviceList.push_back(deviceToConnect);
    rm->getBackEndNames(deviceList, rxAifBackEndsToConnect,
            txAifBackEndsToConnect);
    deviceToConnect->getDeviceAttributes(&dAttr);

    if (!rxAifBackEndsToConnect.empty())
        status = SessionAlsaUtils::setupSessionDevice(streamHandle, streamType, rm,
            dAttr, (pcmDevIds.size() ? pcmDevIds : pcmDevRxIds), rxAifBackEndsToConnect);

    if (!txAifBackEndsToConnect.empty())
        status = SessionAlsaUtils::setupSessionDevice(streamHandle, streamType, rm,
            dAttr, (pcmDevIds.size() ? pcmDevIds : pcmDevTxIds), txAifBackEndsToConnect);

    return status;
}

int SessionAlsaPcm::connectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<Device> deviceToConnect)
{
    std::vector<std::shared_ptr<Device>> deviceList;
    struct pal_device dAttr = {};
    std::vector<std::pair<int32_t, std::string>> rxAifBackEndsToConnect;
    std::vector<std::pair<int32_t, std::string>> txAifBackEndsToConnect;
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;

    deviceList.push_back(deviceToConnect);
    rm->getBackEndNames(deviceList, rxAifBackEndsToConnect,
            txAifBackEndsToConnect);
    deviceToConnect->getDeviceAttributes(&dAttr);

    status = streamHandle->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
    }
    if (!rxAifBackEndsToConnect.empty()) {
        for (const auto &elem : rxAifBackEndsToConnect)
            rxAifBackEnds.push_back(elem);

        if ((streamType != PAL_STREAM_ULTRASOUND &&
            streamType != PAL_STREAM_LOOPBACK) || (streamType == PAL_STREAM_LOOPBACK &&
            sAttr.info.opt_stream_info.loopback_type == PAL_STREAM_LOOPBACK_PLAYBACK_ONLY))
            status = SessionAlsaUtils::connectSessionDevice(this, streamHandle, streamType, rm,
                     dAttr, (pcmDevIds.size() ? pcmDevIds : pcmDevRxIds), rxAifBackEndsToConnect);
        else
            status = SessionAlsaUtils::connectSessionDevice(this, streamHandle, streamType, rm,
                     dAttr, pcmDevTxIds, pcmDevRxIds, rxAifBackEndsToConnect);

        if (status) {
            PAL_ERR(LOG_TAG, "failed to connect rxAifBackEnds: %d",
                    (pcmDevIds.size() ? pcmDevIds.at(0) : pcmDevRxIds.at(0)));
            int cnt = 0;
            for (const auto &elem : rxAifBackEnds) {
                cnt++;
                for (const auto &connectElem : rxAifBackEndsToConnect) {
                    if (std::get<0>(elem) == std::get<0>(connectElem)) {
                        rxAifBackEnds.erase(rxAifBackEnds.begin() + cnt - 1, rxAifBackEnds.begin() + cnt);
                        cnt--;
                        break;
                    }
                }
            }
        } else {
            if (streamType == PAL_STREAM_SENSOR_PCM_RENDERER) {
                // update sr/ch/bw in dAttr if virtual port is enabled
                if (rm->activeGroupDevConfig) {
                    dAttr.config.sample_rate =
                         rm->currentGroupDevConfig.grp_dev_hwep_cfg.sample_rate;
                    dAttr.config.ch_info.channels =
                         rm->currentGroupDevConfig.grp_dev_hwep_cfg.channels;
                    dAttr.config.bit_width = ResourceManager::palFormatToBitwidthLookup(
                         rm->currentGroupDevConfig.grp_dev_hwep_cfg.aud_fmt_id);
                }
                status = notifyUPDToneRendererFmtChng(&dAttr,
                            US_TONE_RENDERER_EP_MEDIA_FORMAT_INFO_CHANGE_DONE);
                if (status) {
                    PAL_ERR(LOG_TAG, "Error notifying Ultrasound tone renderer "
                            "format change START. status = %d", status);
                }
            }
        }
    }

    if (!txAifBackEndsToConnect.empty()) {
        for (const auto &elem : txAifBackEndsToConnect)
            txAifBackEnds.push_back(elem);

        if (streamType != PAL_STREAM_LOOPBACK)
            status = SessionAlsaUtils::connectSessionDevice(this, streamHandle, streamType, rm,
                     dAttr, (pcmDevIds.size() ? pcmDevIds : pcmDevTxIds), txAifBackEndsToConnect);
        else
            status = SessionAlsaUtils::connectSessionDevice(this, streamHandle, streamType, rm,
                     dAttr, pcmDevTxIds, pcmDevRxIds, txAifBackEndsToConnect);

        if (status) {
            int cnt = 0;
            PAL_ERR(LOG_TAG, "failed to connect txAifBackEnds: %d",
                    (pcmDevIds.size() ? pcmDevIds.at(0) : pcmDevTxIds.at(0)));
            for (const auto &elem : txAifBackEnds) {
                cnt++;
                for (const auto &connectElem : txAifBackEndsToConnect) {
                    if (std::get<0>(elem) == std::get<0>(connectElem)) {
                        txAifBackEnds.erase(txAifBackEnds.begin() + cnt - 1, txAifBackEnds.begin() + cnt);
                        cnt--;
                        break;
                    }
                }
            }
        }
    }

    if (sAttr.direction == PAL_AUDIO_INPUT && (pcm && isActive())
#ifdef SEC_AUDIO_KARAOKE
        && sAttr.type == PAL_STREAM_DEEP_BUFFER
#endif
        ) {
        //check if it's new TX device connected, reset the pcm buffer to avoid noise.
        status = pcm_stop(pcm);
        if (status) {
            status = errno;
            PAL_ERR(LOG_TAG, "pcm_stop failed %d", status);
        }
        status = pcm_start(pcm);
        if (status) {
            status = errno;
            PAL_ERR(LOG_TAG, "pcm_start failed %d", status);
        }
    }

    return status;
}

int SessionAlsaPcm::read(Stream *s, int tag __unused, struct pal_buffer *buf, int * size)
{
    int status = 0, bytesRead = 0, bytesToRead = 0, offset = 0, pcmReadSize = 0;
    struct pal_stream_attributes sAttr = {};

    PAL_VERBOSE(LOG_TAG, "Enter")
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }
    while (1) {
        offset = bytesRead + buf->offset;
        bytesToRead = buf->size - offset;
        if (!bytesToRead)
            break;
        if ((bytesToRead / in_buf_size) >= 1)
            pcmReadSize = in_buf_size;
        else
            pcmReadSize = bytesToRead;
        void *data = buf->buffer;
        data = static_cast<char*>(data) + offset;

        if(SessionAlsaUtils::isMmapUsecase(sAttr))
        {
            long ns = 0;
            if (sAttr.in_media_config.sample_rate)
                ns = pcm_bytes_to_frames(pcm, pcmReadSize)*1000000000LL/
                    sAttr.in_media_config.sample_rate;
            requestAdmFocus(s, ns);
            status =  pcm_mmap_read(pcm, data,  pcmReadSize);
            releaseAdmFocus(s);
        } else {
            status =  pcm_read(pcm, data,  pcmReadSize);
        }

        if ((0 != status) || (pcmReadSize == 0)) {
            PAL_ERR(LOG_TAG, "Failed to read data %d bytes read %d", status, pcmReadSize);
            break;
        }

#ifdef SEC_AUDIO_DUMP
        sec_pal_read_pcm(&sAttr, data, pcmReadSize, PCM_DUMP_PURE);
#endif
        bytesRead += pcmReadSize;
    }

    *size = bytesRead;
    PAL_VERBOSE(LOG_TAG, "exit bytesRead:%d status:%d ", bytesRead, status);
    return status;
}

int SessionAlsaPcm::write(Stream *s, int tag, struct pal_buffer *buf, int * size,
                          int flag)
{
    int status = 0;
    size_t bytesWritten = 0, bytesRemaining = 0, offset = 0, sizeWritten = 0;
    struct pal_stream_attributes sAttr = {};

    PAL_VERBOSE(LOG_TAG, "Enter buf:%p tag:%d flag:%d", buf, tag, flag);

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    if (pcm == NULL) {
        PAL_ERR(LOG_TAG, "PCM is NULL");
        return -EINVAL;
    }

    void *data = nullptr;

    bytesRemaining = buf->size;

    while ((bytesRemaining / out_buf_size) > 1) {
        offset = bytesWritten + buf->offset;
        data = buf->buffer;
        data = static_cast<char *>(data) + offset;
        sizeWritten = out_buf_size;  //initialize 0

        if (!pcm) {
            PAL_ERR(LOG_TAG, "pcm is NULL");
            status = -EINVAL;
            goto exit;
        }
#ifdef SEC_AUDIO_DUMP
        sec_pal_write_pcm(&sAttr, data, sizeWritten, PCM_DUMP_LAST);
#endif
        if (SessionAlsaUtils::isMmapUsecase(sAttr)) {
            long ns = 0;
            if (sAttr.out_media_config.sample_rate)
                ns = pcm_bytes_to_frames(pcm, sizeWritten)*1000000000LL/
                    sAttr.out_media_config.sample_rate;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_VERBOSE(LOG_TAG, "1.bufsize:%u ns:%ld", sizeWritten, ns);
#else
            PAL_DBG(LOG_TAG, "1.bufsize:%u ns:%ld", sizeWritten, ns);
#endif
            requestAdmFocus(s, ns);
            status =  pcm_mmap_write(pcm, data,  sizeWritten);
            releaseAdmFocus(s);
        } else {
            status =  pcm_write(pcm, data,  sizeWritten);
        }

        if (0 != status) {
            PAL_ERR(LOG_TAG, "Failed to write the data");
            goto exit;
        }
        bytesWritten += sizeWritten;
        __builtin_sub_overflow(bytesRemaining, sizeWritten, &bytesRemaining);
    }
    offset = bytesWritten + buf->offset;
    sizeWritten = bytesRemaining;
    data = buf->buffer;

    if (!pcm) {
        PAL_ERR(LOG_TAG, "pcm is NULL");
        status = -EINVAL;
        goto exit;
    }

    data = static_cast<char *>(data) + offset;
#ifdef SEC_AUDIO_DUMP
    sec_pal_write_pcm(&sAttr, data, sizeWritten, PCM_DUMP_LAST);
#endif
    if (SessionAlsaUtils::isMmapUsecase(sAttr)) {
        if (sizeWritten) {
            long ns = 0;
            if (sAttr.out_media_config.sample_rate)
                ns = pcm_bytes_to_frames(pcm, sizeWritten)*1000000000LL/
                    sAttr.out_media_config.sample_rate;
#ifdef SEC_AUDIO_ADD_FOR_DEBUG
            PAL_VERBOSE(LOG_TAG, "2.bufsize:%u ns:%ld", sizeWritten, ns);
#else
            PAL_DBG(LOG_TAG, "2.bufsize:%u ns:%ld", sizeWritten, ns);
#endif
            requestAdmFocus(s, ns);
            status =  pcm_mmap_write(pcm, data,  sizeWritten);
            releaseAdmFocus(s);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "Error! pcm_mmap_write failed");
                goto exit;
            }
        }
    } else {
        status =  pcm_write(pcm, data,  sizeWritten);
        if (status != 0) {
            PAL_ERR(LOG_TAG, "Error! pcm_write failed");
            goto exit;
        }
    }
    bytesWritten += sizeWritten;
    *size = bytesWritten;
exit:
    PAL_VERBOSE(LOG_TAG, "exit status: %d", status);
    return status;
}

int SessionAlsaPcm::readBufferInit(Stream * /*streamHandle*/, size_t /*noOfBuf*/, size_t /*bufSize*/,
                                   int /*flag*/)
{
    return 0;
}
int SessionAlsaPcm::writeBufferInit(Stream * /*streamHandle*/, size_t /*noOfBuf*/, size_t /*bufSize*/,
                                    int /*flag*/)
{
    return 0;
}

void SessionAlsaPcm::setEventPayload(uint32_t event_id, void *payload, size_t payload_size)
{
    eventId = event_id;
    if (eventPayload)
        free(eventPayload);

    eventPayloadSize = payload_size;
    eventPayload = calloc(1, payload_size);
    if (!eventPayload) {
        PAL_ERR(LOG_TAG, "Memory alloc failed for eventPayload");
        return;
    }
    memcpy(eventPayload, payload, payload_size);
}

int SessionAlsaPcm::setParameters(Stream *streamHandle, int tagId, uint32_t param_id, void *payload)
{
    int status = 0;
    int device = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;
    uint32_t miid = 0;
    effect_pal_payload_t *effectPalPayload = nullptr;
    struct pal_stream_attributes sAttr = {};

    PAL_DBG(LOG_TAG, "Enter. param id: %d", param_id);
    if (pcmDevIds.size() > 0)
        device = pcmDevIds.at(0);
    switch (param_id) {
        case PAL_PARAM_ID_DEVICE_ROTATION:
        {
            pal_param_device_rotation_t *rotation =
                                         (pal_param_device_rotation_t *)payload;
            status = handleDeviceRotation(streamHandle, rotation->rotation_type,
                                          device, mixer, builder, rxAifBackEnds);
            goto exit;
        }
        case PAL_PARAM_ID_LOAD_SOUND_MODEL:
        case PAL_PARAM_ID_UNLOAD_SOUND_MODEL:
        case PAL_PARAM_ID_WAKEUP_ENGINE_CONFIG:
        case PAL_PARAM_ID_WAKEUP_BUFFERING_CONFIG:
        case PAL_PARAM_ID_WAKEUP_ENGINE_RESET:
        case PAL_PARAM_ID_WAKEUP_ENGINE_PER_MODEL_RESET:
        case PAL_PARAM_ID_WAKEUP_CUSTOM_CONFIG:
        case PAL_PARAM_ID_ASR_CONFIG:
        case PAL_PARAM_ID_ASR_OUTPUT:
        case PAL_PARAM_ID_ASR_FORCE_OUTPUT:
        case PAL_PARAM_ID_ASR_SET_PARAM:
        {
            struct apm_module_param_data_t* header =
                (struct apm_module_param_data_t *)payload;
            if (param_id == PAL_PARAM_ID_ASR_CONFIG ||
                param_id == PAL_PARAM_ID_ASR_OUTPUT ||
                param_id == PAL_PARAM_ID_ASR_FORCE_OUTPUT ||
                param_id == PAL_PARAM_ID_ASR_SET_PARAM)
                asrMiid = header->module_instance_id;
            else
                svaMiid = header->module_instance_id;
            paramData = (uint8_t *)payload;
            paramSize = PAL_ALIGN_8BYTE(header->param_size +
                sizeof(struct apm_module_param_data_t));
            if (mState == SESSION_IDLE) {
                if (!customPayloadSize) {
                    customPayload = (uint8_t *)calloc(1, paramSize);
                } else {
                    customPayload = (uint8_t *)realloc(customPayload,
                        customPayloadSize + paramSize);
                }
                if (!customPayload) {
                    status = -ENOMEM;
                    PAL_ERR(LOG_TAG, "failed to allocate memory for custom payload");
                    goto exit;
                }

                ar_mem_cpy((uint8_t *)customPayload + customPayloadSize, paramSize,
                    paramData, paramSize);
                customPayloadSize += paramSize;
                PAL_INFO(LOG_TAG, "customPayloadSize = %zu", customPayloadSize);
            } else {
                if (pcmDevIds.size() > 0) {
                    status = SessionAlsaUtils::setMixerParameter(mixer,
                        pcmDevIds.at(0), paramData, paramSize);
                    if (status) {
                        PAL_ERR(LOG_TAG, "Failed to set mixer param, status = %d",
                            status);
                        goto exit;
                    }
                } else {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
            }
            break;
        }
        case PAL_PARAM_ID_BT_A2DP_TWS_CONFIG:
        {
            pal_bt_tws_payload *tws_payload = (pal_bt_tws_payload *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            builder->payloadTWSConfig(&paramData, &paramSize, miid,
                    tws_payload->isTwsMonoModeOn, tws_payload->codecFormat);
            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set tws config status=%d\n", status);
                freeCustomPayload(&paramData, &paramSize);
            }
            return 0;
        }
        case PAL_PARAM_ID_BT_A2DP_LC3_CONFIG:
        {
            pal_bt_lc3_payload *lc3_payload = (pal_bt_lc3_payload *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            builder->payloadLC3Config(&paramData, &paramSize, miid,
                    lc3_payload->isLC3MonoModeOn);
            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set lc3 config status=%d\n", status);
                freeCustomPayload(&paramData, &paramSize);
            }
            return 0;
        }
#ifdef SEC_PRODUCT_FEATURE_BLUETOOTH_SUPPORT_A2DP_OFFLOAD
        case PAL_PARAM_ID_BT_A2DP_SUSPENDED:
        {
            pal_param_ss_pack_a2dp_suspend_t *a2dp_suspend_payload = (pal_param_ss_pack_a2dp_suspend_t *)payload;
            device = pcmDevIds.at(0);
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            PAL_INFO(LOG_TAG, "mixer set a2dp_suspend %d\n", a2dp_suspend_payload->a2dp_suspend);

            builder->payloadSsPackSuspendConfig(&paramData, &paramSize,
                miid, a2dp_suspend_payload->a2dp_suspend);

            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set a2dp_suspend status=%d\n", status);
                free(paramData);
            }
            return 0;
        }
        case PAL_PARAM_ID_BT_A2DP_DYN_BITRATE:
        {
            pal_param_bta2dp_dynbitrate_t *dynbitrate_payload = (pal_param_bta2dp_dynbitrate_t *)payload;
            device = pcmDevIds.at(0);
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            PAL_INFO(LOG_TAG, "mixer set bitrate %d\n", dynbitrate_payload->bitrate);

            builder->payloadSsPackBitrate(&paramData, &paramSize,
                 miid, dynbitrate_payload->bitrate);
            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set bitrate status=%d\n", status);
                free(paramData);
            }
            return 0;
        }
        case PAL_PARAM_ID_BT_A2DP_SBM_CONFIG:
        {
            pal_param_bta2dp_sbm_t *sbm_payload = (pal_param_bta2dp_sbm_t *)payload;
            device = pcmDevIds.at(0);
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            PAL_INFO(LOG_TAG, "mixer set sbm config firstParam=%d, secondParam=%d\n",
                                    sbm_payload->firstParam, sbm_payload->secondParam);

            builder->payloadSsPackSbmConfig(&paramData, &paramSize,
                 miid, sbm_payload);
            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set sbm config status=%d\n", status);
                free(paramData);
            }
            return 0;
        }
#endif
        case PAL_PARAM_ID_MODULE_CONFIG:
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            if (param_payload->payload_size) {
                 status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                                              param_payload->payload,
                                                              param_payload->payload_size);
                 PAL_INFO(LOG_TAG, "mixer set module config status=%d\n", status);
            }
            return 0;
        }
        case PAL_PARAM_ID_UPD_REGISTER_FOR_EVENTS:
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            pal_param_upd_event_detection_t *detection_payload =
                                   (pal_param_upd_event_detection_t *)param_payload->payload;
            RegisterForEvents = detection_payload->register_status;
            return 0;
        }
        case PAL_PARAM_ID_VOLUME_USING_SET_PARAM:
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            pal_volume_data *vdata = (struct pal_volume_data *)param_payload->payload;
            status = streamHandle->getStreamAttributes(&sAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
                goto exit;
            }
            if (sAttr.direction == PAL_AUDIO_OUTPUT) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                        rxAifBackEnds[0].second.data(), TAG_STREAM_VOLUME, &miid);
            } else if (sAttr.direction == PAL_AUDIO_INPUT) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                        txAifBackEnds[0].second.data(), TAG_STREAM_VOLUME, &miid);
            } else if (sAttr.direction == (PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT)) {
                status = -EINVAL;
                if (pcmDevRxIds.size()) {
                    device = pcmDevRxIds.at(0);
                    status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                            rxAifBackEnds[0].second.data(), TAG_STREAM_VOLUME, &miid);
                    if (status) {
                        if (pcmDevTxIds.size() > 0)
                            device = pcmDevTxIds.at(0);
                        status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                                                       txAifBackEnds[0].second.data(),
                                                                       tagId, &miid);
                    }
                }
            } else {
                status = 0;
                goto exit;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, dir: %d (%d)", tagId,
                       sAttr.direction, status);
                status = 0;
                goto exit;
            }

            if (vdata->no_of_volpair > 1 && sAttr.out_media_config.ch_info.channels > 1) {
                builder->payloadMultichVolumemConfig(&paramData, &paramSize, miid, vdata);
            } else {
                builder->payloadVolumeConfig(&paramData, &paramSize, miid, vdata);
            }

            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set volume config status=%d\n", status);
                freeCustomPayload(&paramData, &paramSize);
                paramSize = 0;
            }
            return 0;
        }
        case PAL_PARAM_ID_MSPP_LINEAR_GAIN:
        {
            pal_param_mspp_linear_gain_t *linear_gain = (pal_param_mspp_linear_gain_t *)payload;
            device = pcmDevIds.at(0)
            ;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            PAL_INFO(LOG_TAG, "Set mspp linear gain");
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }

            builder->payloadMSPPConfig(&paramData, &paramSize, miid, linear_gain->gain);
            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set MSPP config status=%d\n", status);
                free(paramData);
            }
            return 0;
        }
        case PAL_PARAM_ID_SET_UPD_DUTY_CYCLE:
        {
            std::vector <std::pair<int, int>> tkv;
            const char *setParamTagControl = " setParamTag";
            const char *streamPcm = "PCM";
            struct mixer_ctl *ctl;
            std::ostringstream tagCntrlNameRx;
            std::ostringstream tagCntrlNameTx;
            struct agm_tag_config* tagConfig = NULL;
            int tkv_size = 0;

            status = streamHandle->getStreamAttributes(&sAttr);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
                goto exit;
            }

            tkv.push_back(std::make_pair(TAG_KEY_DUTY_CYCLE, *(int*)payload));
            tagConfig = (struct agm_tag_config*)malloc(sizeof(struct agm_tag_config) +
                    (tkv.size() * sizeof(agm_key_value)));

            if (!tagConfig) {
                status = -EINVAL;
                goto exit;
            }

            status = SessionAlsaUtils::getTagMetadata(TAG_DUTY_CYCLE, tkv, tagConfig);
            if (0 != status) {
                if (tagConfig)
                    free(tagConfig);
                goto exit;
            }

            // set UPD RX tag data
            if (sAttr.type == PAL_STREAM_ULTRASOUND)
                tagCntrlNameRx<<streamPcm<<pcmDevRxIds.at(0)<<setParamTagControl;
            else // SENSOR_RENDERER
                tagCntrlNameRx<<streamPcm<<pcmDevIds.at(0)<<setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlNameRx.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlNameRx.str().data());
                status = -EINVAL;
                if (tagConfig)
                    free(tagConfig);
                goto exit;
            }
            tkv_size = tkv.size()*sizeof(struct agm_key_value);
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the RX duty cycle calibration %d", status);
            }

            // TX duty cycle param is N/A for sensor renderer
            if (sAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER)
                return 0;

            // set UPD TX tag data
            tagCntrlNameTx<<streamPcm<<pcmDevTxIds.at(0)<<setParamTagControl;
            ctl = mixer_get_ctl_by_name(mixer, tagCntrlNameTx.str().data());
            if (!ctl) {
                PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", tagCntrlNameTx.str().data());
                status = -EINVAL;
                if (tagConfig)
                    free(tagConfig);
                goto exit;
            }
            status = mixer_ctl_set_array(ctl, tagConfig, sizeof(struct agm_tag_config) + tkv_size);
            if (status != 0) {
                PAL_ERR(LOG_TAG, "failed to set the TX duty cycle calibration %d", status);
            }
            if (tagConfig)
                free(tagConfig);
            return 0;
        }
        case PAL_PARAM_ID_ULTRASOUND_RAMPDOWN:
        {
            uint32_t rampdown = 1;
            uint32_t paramId = PARAM_ID_EXAMPLE_US_GEN_PARAM_2;
            device = pcmDevRxIds.at(0);
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                                                rxAifBackEnds[0].second.data(),
                                                                tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                break;
            } else {
                PAL_DBG(LOG_TAG, "got Ultrasound Generator miid = 0x%08x", miid);
                status = builder->payloadCustomParam(&paramData, &paramSize,
                            &rampdown,
                            sizeof(rampdown),
                            miid, paramId);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "payloadCustomParam failed. status = %d",
                                status);
                    break;
                }
                status = SessionAlsaUtils::setMixerParameter(mixer,
                                                             device,
                                                             paramData,
                                                             paramSize);
                PAL_DBG(LOG_TAG, "mixer set param status=%d\n", status);
            }
            break;
        }
        case PAL_PARAM_ID_VOLUME_CTRL_RAMP:
        {
            struct pal_vol_ctrl_ramp_param *rampParam = (struct pal_vol_ctrl_ramp_param *)payload;
            status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                               rxAifBackEnds[0].second.data(), tagId, &miid);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d", tagId, status);
                return status;
            }
            builder->payloadVolumeCtrlRamp(&paramData, &paramSize,
                 miid, rampParam->ramp_period_ms);
            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_INFO(LOG_TAG, "mixer set vol ctrl ramp status=%d\n", status);
                freeCustomPayload(&paramData, &paramSize);
            }
            return 0;
         }
        case PAL_PARAM_ID_HAPTICS_CNFG :
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            pal_param_haptics_cnfg_t *HapticsCnfg = (pal_param_haptics_cnfg_t *)param_payload->payload;

            PAL_DBG(LOG_TAG, "Store Haptics configuration for future use");

            hpCnfg = (pal_param_haptics_cnfg_t *) calloc(1, sizeof(pal_param_haptics_cnfg_t));
            if (hpCnfg == NULL) {
                PAL_ERR(LOG_TAG, "Haptics config memory allocation failed.");
                return -ENOMEM;
            }

            memcpy(hpCnfg, HapticsCnfg, sizeof(pal_param_haptics_cnfg_t ));
            if(HapticsCnfg->buffer_size) {
                hpCnfg->buffer_ptr = (uint8_t*) calloc(1, HapticsCnfg->buffer_size);
                memcpy(hpCnfg->buffer_ptr, (uint8_t*)HapticsCnfg + sizeof(pal_param_haptics_cnfg_t),
                         HapticsCnfg->buffer_size);
            }
            if (isActive()) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                      rxAifBackEnds[0].second.data(), MODULE_HAPTICS_GEN, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                    free(hpCnfg);
                    return status;
                }
                if (hpCnfg != NULL) {
                    if (hpCnfg->mode == PAL_STREAM_HAPTICS_PCM) {
                        builder->payloadHapticsDevPConfig(&paramData, &paramSize,
                            miid, PARAM_ID_HAPTICS_RX_PCMV_PLAYBACK,(void *)hpCnfg);

                        if (paramSize) {
                            status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                                         paramData, paramSize);
                            if (status)
                                PAL_ERR(LOG_TAG, "setMixerParam for haptics PCM Failed\n");
                            freeCustomPayload(&paramData, &paramSize);
                        }
                    }
                    builder->payloadHapticsDevPConfig(&paramData, &paramSize,
                               miid, PARAM_ID_HAPTICS_WAVE_DESIGNER_CFG,(void *)hpCnfg);
                    if (paramSize) {
                        status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                                         paramData, paramSize);
                        if (status)
                           PAL_ERR(LOG_TAG, "setMixerParam failed for haptics Wave\n");
                        freeCustomPayload(&paramData, &paramSize);
                    }
                    free(hpCnfg->buffer_ptr);
                    free(hpCnfg);
                }
            }
            return status;
        }
        case PARAM_ID_HAPTICS_WAVE_DESIGNER_STOP_PARAM :
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            param_id_haptics_wave_designer_wave_designer_stop_param_t *HapticsCnfg =
             (param_id_haptics_wave_designer_wave_designer_stop_param_t *)param_payload->payload;

                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                      rxAifBackEnds[0].second.data(), MODULE_HAPTICS_GEN, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                    return status;
                }
                builder->payloadHapticsDevPConfig(&paramData, &paramSize,
                            miid, PARAM_ID_HAPTICS_WAVE_DESIGNER_STOP_PARAM,(void *)HapticsCnfg);
                if (paramSize) {
                    status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                                     paramData, paramSize);
                    if (status)
                        PAL_ERR(LOG_TAG, "setMixerParam failed for stop haptics Wave\n");
                    freeCustomPayload(&paramData, &paramSize);
                }
            return status;
        }
        case PARAM_ID_HAPTICS_WAVE_DESIGNER_UPDATE_PARAM :
        {
           pal_param_payload *param_payload = (pal_param_payload *)payload;
           pal_param_haptics_cnfg_t *HapticsCnfg = (pal_param_haptics_cnfg_t *)param_payload->payload;

            PAL_DBG(LOG_TAG, "Update Haptics configuration\n");
                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                                          rxAifBackEnds[0].second.data(), MODULE_HAPTICS_GEN, &miid);
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "getModuleInstanceId failed");
                    return status;
                }
                builder->payloadHapticsDevPConfig(&paramData, &paramSize,
                               miid, PARAM_ID_HAPTICS_WAVE_DESIGNER_UPDATE_PARAM ,(void *)HapticsCnfg);
                if (paramSize) {
                    status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                                         paramData, paramSize);
                    if (status)
                        PAL_ERR(LOG_TAG, "setMixerParam failed for haptics Wave\n");
                    freeCustomPayload(&paramData, &paramSize);
                }
            return 0;
        }
        case PAL_PARAM_ID_GAIN_USING_SET_PARAM:
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
            pal_gain_data *gdata = (struct pal_gain_data *)param_payload->payload;
            status = streamHandle->getStreamAttributes(&sAttr);
            PAL_DBG(LOG_TAG, "Gainlog - Get Stream attribs status - %d", status);
            if (0 != status) {
                PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
                goto exit;
            }
            if (sAttr.direction == PAL_AUDIO_OUTPUT &&
               (sAttr.type == PAL_STREAM_DEEP_BUFFER || PAL_STREAM_PCM_OFFLOAD)) {
                status = SessionAlsaUtils::getModuleInstanceId(mixer, device,
                         rxAifBackEnds[0].second.data(), tagId, &miid);
                PAL_DBG(LOG_TAG, "Gainlog - Get MIID status - %d", status);
            } else {
                status = 0;
                goto exit;
            }
            if (0 != status) {
                PAL_ERR(LOG_TAG, " GainLog - Failure to get gain tag info %x", tagId);
                goto exit;
            }

            builder->payloadGainConfig(&paramData, &paramSize, miid, gdata);

            if (paramSize) {
                status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                               paramData, paramSize);
                PAL_DBG(LOG_TAG, "GainLog - mixer set gain config status=%d\n", status);
                freeCustomPayload(&paramData, &paramSize);
            }
            return 0;
        }
        default:
            status = -EINVAL;
            PAL_ERR(LOG_TAG, "Unsupported param id %u status %d", param_id, status);
            goto exit;
    }

    if (!paramData) {
        status = -ENOMEM;
        PAL_ERR(LOG_TAG, "failed to get payload status %d", status);
        goto exit;
    }

    PAL_VERBOSE(LOG_TAG, "%pK - payload and %zu size", paramData , paramSize);

exit:
    if (paramData)
        free(paramData);

    PAL_DBG(LOG_TAG, "Exit. status %d", status);
    return status;
}

int SessionAlsaPcm::register_asps_event(uint32_t reg)
{
    int32_t status = 0;
    struct agm_event_reg_cfg *event_cfg = nullptr;
    uint32_t payload_size = sizeof(struct agm_event_reg_cfg);
    event_cfg = (struct agm_event_reg_cfg *)calloc(1, payload_size);
    if (!event_cfg) {
        status = -ENOMEM;
        return status;
    }
    event_cfg->event_config_payload_size = 0;
    event_cfg->is_register = reg;
    event_cfg->event_id = EVENT_ID_ASPS_GET_SUPPORTED_CONTEXT_IDS;
    event_cfg->module_instance_id = ASPS_MODULE_INSTANCE_ID;
    if (pcmDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
        status = -EINVAL;
        free(event_cfg);
        return status;
    }
    SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)event_cfg, payload_size);

    event_cfg->event_id = EVENT_ID_ASPS_SENSOR_REGISTER_REQUEST;
    SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)event_cfg, payload_size);

    event_cfg->event_id = EVENT_ID_ASPS_SENSOR_DEREGISTER_REQUEST;
    SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)event_cfg, payload_size);

    event_cfg->event_id = EVENT_ID_ASPS_CLOSE_ALL;
    SessionAlsaUtils::registerMixerEvent(mixer, pcmDevIds.at(0),
            (void *)event_cfg, payload_size);
    free(event_cfg);
    return status;
}

int SessionAlsaPcm::setECRef(Stream *s, std::shared_ptr<Device> rx_dev, bool is_enable)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    std::vector <std::shared_ptr<Device>> rxDeviceList;
    std::vector <std::string> backendNames;
    struct pal_device rxDevAttr = {};
    struct pal_device_info rxDevInfo = {};
    std::vector <std::shared_ptr<Device>> tx_devs;
    std::shared_ptr<Device> ec_rx_dev = nullptr;

    PAL_DBG(LOG_TAG, "Enter");
    if (!s) {
        PAL_ERR(LOG_TAG, "Invalid stream or rx device");
        status = -EINVAL;
        goto exit;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }

    if (sAttr.direction != PAL_AUDIO_INPUT) {
        PAL_ERR(LOG_TAG, "EC Ref cannot be set to output stream");
        status = -EINVAL;
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
    } else if (!is_enable && ecRefDevId != PAL_DEVICE_OUT_MIN) {
        // if disable EC with null rx_dev, retrieve current EC device
        rxDevAttr.id = ecRefDevId;
        rx_dev = Device::getInstance(&rxDevAttr, rm);
        if (rx_dev) {
            status = rx_dev->getDeviceAttributes(&rxDevAttr, s);
            if (status) {
                PAL_ERR(LOG_TAG, "getDeviceAttributes failed for ec dev: %d", ecRefDevId);
                goto exit;
            }
        }
        rm->getDeviceInfo(rxDevAttr.id, sAttr.type, rxDevAttr.custom_config.custom_key, &rxDevInfo);
    }

    if (!is_enable) {
        if (ecRefDevId == PAL_DEVICE_OUT_MIN) {
            PAL_DBG(LOG_TAG, "EC ref not enabled, skip disabling");
            goto exit;
        } else if (rx_dev && ecRefDevId != rx_dev->getSndDeviceId()) {
            PAL_DBG(LOG_TAG, "Invalid rx dev %d for disabling EC ref, "
                "rx dev %d already enabled", rx_dev->getSndDeviceId(), ecRefDevId);
            status = -EINVAL;
            goto exit;
        }

        if (rxDevInfo.isExternalECRefEnabledFlag) {
            status = checkAndSetExtEC(rm, s, false);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to disable External EC, status %d", status);
                goto exit;
            }
        } else {
            if (pcmDevIds.size() == 0) {
                PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                status = -EINVAL;
                goto exit;
            }
            status = SessionAlsaUtils::setECRefPath(mixer, pcmDevIds.at(0), "ZERO");
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to disable EC Ref, status %d", status);
                goto exit;
            }
        }
    } else if (is_enable && rx_dev) {
        if (ecRefDevId == rx_dev->getSndDeviceId()) {
            PAL_DBG(LOG_TAG, "EC Ref already set for dev %d", ecRefDevId);
            goto exit;
        }
        // TODO: handle EC Ref switch case also
        rxDeviceList.push_back(rx_dev);
        backendNames = rm->getBackEndNames(rxDeviceList);

        if (rxDevInfo.isExternalECRefEnabledFlag) {
            // reset EC if internal EC is being used
            if (ecRefDevId != PAL_DEVICE_OUT_MIN && !rm->isExternalECRefEnabled(ecRefDevId)) {
                if (pcmDevIds.size() == 0) {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
                status = SessionAlsaUtils::setECRefPath(mixer, pcmDevIds.at(0), "ZERO");
                if (status) {
                    PAL_ERR(LOG_TAG, "Failed to reset before set ext EC, status %d", status);
                    goto exit;
                }
            }
            status = checkAndSetExtEC(rm, s, true);
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to enable External EC, status %d", status);
                goto exit;
            }
        } else {
            // avoid set internal ec if ext ec connected
            if (ecRefDevId != PAL_DEVICE_OUT_MIN && rm->isExternalECRefEnabled(ecRefDevId)) {
                PAL_ERR(LOG_TAG, "Cannot be set internal EC with external EC connected");
                status = -EINVAL;
                goto exit;
            }
            // reset EC if different EC device is being used
            if (ecRefDevId != PAL_DEVICE_OUT_MIN && ecRefDevId != rx_dev->getSndDeviceId()) {
                PAL_DBG(LOG_TAG, "EC ref is enabled with %d, reset EC first", ecRefDevId);
                rxDevAttr.id = ecRefDevId;
                ec_rx_dev = Device::getInstance(&rxDevAttr, rm);

                s->getAssociatedDevices(tx_devs);
                if (tx_devs.size()) {
                    for (int i = 0; i < tx_devs.size(); ++i) {
                        status = rm->updateECDeviceMap_l(ec_rx_dev, tx_devs[i], s, 0, true);
                        if (status) {
                            PAL_ERR(LOG_TAG, "Failed to update EC Device map for device %s, status: %d",
                                    tx_devs[i]->getPALDeviceName().c_str(), status);
                        }
                    }
                }
                if (pcmDevIds.size() == 0) {
                    PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                    status = -EINVAL;
                    goto exit;
                }
                status = SessionAlsaUtils::setECRefPath(mixer, pcmDevIds.at(0), "ZERO");
                if (status) {
                    PAL_ERR(LOG_TAG, "Failed to reset before set ext EC, status %d", status);
                    goto exit;
                }
            }
            if (pcmDevIds.size() == 0) {
                PAL_ERR(LOG_TAG, "frontendIDs is not available.");
                status = -EINVAL;
                goto exit;
            }
            status = SessionAlsaUtils::setECRefPath(mixer, pcmDevIds.at(0),
                    backendNames[0].c_str());
            if (status) {
                PAL_ERR(LOG_TAG, "Failed to enable EC Ref, status %d", status);
                goto exit;
            }
        }
    } else {
        PAL_ERR(LOG_TAG, "Invalid operation");
        status = -EINVAL;
        goto exit;
    }
exit:
    if (status == 0) {
        if (is_enable && rx_dev) {
            ecRefDevId = static_cast<pal_device_id_t>(rx_dev->getSndDeviceId());
        } else {
            if (!is_enable && rx_dev &&
                rxDevInfo.isExternalECRefEnabledFlag &&
                ecRefDevId != rx_dev->getSndDeviceId()) {
                PAL_DBG(LOG_TAG, "internal EC recovered, skip resetting ecRefDevId");
            } else {
                ecRefDevId = PAL_DEVICE_OUT_MIN;
            }
        }
    }
    PAL_DBG(LOG_TAG, "Exit, status: %d", status);
    return status;
}

int SessionAlsaPcm::getParameters(Stream *s, int tagId, uint32_t param_id, void **payload)
{
    int status = 0;
    uint8_t *ptr = NULL;
    uint8_t *config = NULL;
    uint8_t *payloadData = NULL;
    size_t payloadSize = 0;
    size_t configSize = 0;
    int device = 0;
    uint32_t miid = 0;
    const char *control = "getParam";
    const char *stream = "PCM";
    struct mixer_ctl *ctl;
    std::ostringstream CntrlName;
    PAL_INFO(LOG_TAG, "Enter, param id 0x%x, tag id : 0x%x", param_id, tagId);

    if (pcmDevIds.size() > 0) {
        device = pcmDevIds.at(0);
        CntrlName << stream << pcmDevIds.at(0) << " " << control;
    } else {
        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
        status = -EINVAL;
        goto exit;
    }
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        status = -ENOENT;
        goto exit;
    }

    if (!rxAifBackEnds.empty()) { /** search in RX GKV */
        status = SessionAlsaUtils::getModuleInstanceId(mixer, device, rxAifBackEnds[0].second.data(),
                tagId, &miid);
        if (status) /** if not found, reset miid to 0 again */
            miid = 0;
    }

    if (!txAifBackEnds.empty()) { /** search in TX GKV */
        status = SessionAlsaUtils::getModuleInstanceId(mixer, device, txAifBackEnds[0].second.data(),
                tagId, &miid);
        if (status)
            miid = 0;
    }

    if (miid == 0 && param_id != PAL_PARAM_ID_SVA_WAKEUP_MODULE_VERSION) {
        PAL_ERR(LOG_TAG, "failed to look for module with tagID 0x%x", tagId);
        status = -EINVAL;
        goto exit;
    }


    switch (param_id) {
        case PAL_PARAM_ID_DIRECTION_OF_ARRIVAL:
        {
            configSize = sizeof(struct ffv_doa_tracking_monitor_t);
            builder->payloadGetParam(s, &payloadData, &payloadSize, miid,
                                    PARAM_ID_FFV_DOA_TRACKING_MONITOR, configSize);
            break;
        }
        case PAL_PARAM_ID_WAKEUP_MODULE_VERSION:
        {
            payloadData = (uint8_t *)*payload;
            struct apm_module_param_data_t *header =
                (struct apm_module_param_data_t *)payloadData;
            configSize = header->param_size;
            payloadSize = PAL_ALIGN_8BYTE(
                configSize + sizeof(struct apm_module_param_data_t));
            break;
        }
        case PAL_PARAM_ID_SVA_WAKEUP_MODULE_VERSION:
        {
            configSize = sizeof(struct amdb_param_id_module_version_info_t) +
                         sizeof(struct amdb_module_version_info_payload_t);
            builder->payloadAFSInfo(&payloadData, &payloadSize, miid);
            break;
        }
        case PAL_PARAM_ID_ASR_OUTPUT:
        {
            configSize = sizeof(param_id_asr_output_t) +
                          (s->GetNumEvents() * sizeof(asr_output_status_t));
            builder->payloadGetParam(s, &payloadData, &payloadSize, miid,
                          PARAM_ID_ASR_OUTPUT, configSize);
            break;
        }
#ifdef SEC_AUDIO_CALL
        case PAL_PARAM_ID_UIEFFECT:
        {
            pal_param_payload *param_payload = (pal_param_payload *)payload;
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
            goto exit;
        }
#endif
        default:
            status = EINVAL;
            PAL_ERR(LOG_TAG, "Unsupported param id %u status %d", param_id, status);
            goto exit;
    }

    status = mixer_ctl_set_array(ctl, payloadData, payloadSize);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "Set custom config failed, status = %d", status);
        goto exit;
    }

    if (payloadData) {
        status = mixer_ctl_get_array(ctl, payloadData, payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Get custom config failed, status = %d", status);
            goto exit;
        }
    } else {
        PAL_ERR(LOG_TAG, "Failed to allocate payloadData memory\n");
        status = -ENOMEM;
        goto exit;
    }

    ptr = (uint8_t *)payloadData + sizeof(struct apm_module_param_data_t);
    config = (uint8_t *)calloc(1, configSize);
    if (!config) {
        PAL_ERR(LOG_TAG, "Failed to allocate memory for config");
        status = -ENOMEM;
        goto exit;
    }

    ar_mem_cpy(config, configSize, ptr, configSize);
    *payload = (void *)config;


exit:
    freeCustomPayload(&payloadData, &payloadSize);
    PAL_INFO(LOG_TAG, "Exit. status %d", status);
    return status;
}

int SessionAlsaPcm::registerCallBack(session_callback cb, uint64_t cookie)
{
    sessionCb = cb;
    cbCookie = cookie;
    return 0;
}

int SessionAlsaPcm::getTimestamp(struct pal_session_time *stime)
{
    int status = 0;

    if (pcmDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
        status = -EINVAL;
        return status;
    }
    if (!spr_miid) {
        status = SessionAlsaUtils::getModuleInstanceId(mixer,
                pcmDevIds.at(0), rxAifBackEnds[0].second.data(),
                STREAM_SPR, &spr_miid);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d",
                    STREAM_SPR, status);
            return status;
        }
    }
    status = SessionAlsaUtils::getTimestamp(mixer, pcmDevIds, spr_miid, stime);
    if (0 != status)
       PAL_ERR(LOG_TAG, "getTimestamp failed status = %d", status);

    return status;
}

int SessionAlsaPcm::drain(pal_drain_type_t type __unused)
{
    return 0;
}

int SessionAlsaPcm::flush()
{
    int status = 0;
    PAL_VERBOSE(LOG_TAG, "Enter flush");

    if (pcmDevIds.size() > 0) {
        status = SessionAlsaUtils::flush(rm, pcmDevIds.at(0));
    } else {
        PAL_ERR(LOG_TAG, "DevIds size is invalid");
        return -EINVAL;
    }

    PAL_VERBOSE(LOG_TAG, "Exit status: %d", status);
    return status;
}

bool SessionAlsaPcm::isActive()
{
    PAL_VERBOSE(LOG_TAG, "state = %d", mState);
    return mState == SESSION_STARTED;
}

int SessionAlsaPcm::getTagsWithModuleInfo(Stream *s, size_t *size __unused, uint8_t *payload)
{
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    int DeviceId;

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        return status;
    }

    if(sAttr.type == PAL_STREAM_ULTRASOUND) {
        if (pcmDevTxIds.size() > 0) {
            DeviceId = pcmDevTxIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            status = -EINVAL;
            return status;
        }
    } else {
        if (pcmDevIds.size() > 0) {
            DeviceId = pcmDevIds.at(0);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            status = -EINVAL;
            return status;
        }

    }

    if (sAttr.type == PAL_STREAM_SENSOR_PCM_RENDERER)
        status = SessionAlsaUtils::getTagsWithModuleInfo(mixer, DeviceId,
                                      rxAifBackEnds[0].second.data(), payload);
    else
        status = SessionAlsaUtils::getTagsWithModuleInfo(mixer, DeviceId,
                                      txAifBackEnds[0].second.data(), payload);
    if (0 != status)
        PAL_ERR(LOG_TAG, "get tags failed = %d", status);

    return status;
}

void SessionAlsaPcm::adjustMmapPeriodCount(struct pcm_config *config, int32_t min_size_frames)
{
    int periodCountRequested = (min_size_frames + config->period_size - 1)
                               / config->period_size;
    int periodCount = SESSION_ALSA_MMAP_PERIOD_COUNT_MIN;

    PAL_VERBOSE(LOG_TAG, "original config.period_size = %d config.period_count = %d",
                config->period_size, config->period_count);

    while (periodCount < periodCountRequested &&
        (periodCount * 2) < SESSION_ALSA_MMAP_PERIOD_COUNT_MAX) {
        periodCount *= 2;
    }
    config->period_count = periodCount;

    PAL_VERBOSE(LOG_TAG, "requested config.period_count = %d",
                config->period_count);

}


int SessionAlsaPcm::createMmapBuffer(Stream *s, int32_t min_size_frames,
                                   struct pal_mmap_buffer *info)
{
    unsigned int offset1 = 0;
    unsigned int frames1 = 0;
    const char *step = "enter";
    uint32_t buffer_size;
    struct pcm_config config;
    struct pal_stream_attributes sAttr = {};
    int32_t status = 0;
    unsigned int pcm_flags = 0;
    const char *control = "getBufInfo";
    const char *stream = "PCM";
    struct mixer_ctl *ctl;
    std::ostringstream CntrlName;
    struct agm_buf_info buf_info;

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    if (info == NULL || !(min_size_frames > 0 && min_size_frames < INT32_MAX)) {
        PAL_ERR(LOG_TAG, "info = %p, min_size_frames = %d",
                info, min_size_frames);
        return -EINVAL;
    }

    if (!(((sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY) &&
            (sAttr.flags & PAL_STREAM_FLAG_MMAP_NO_IRQ)) ||
            (sAttr.type == PAL_STREAM_VOICE_UI) || (sAttr.type == PAL_STREAM_ACD))) {
         PAL_ERR(LOG_TAG, "called on stream type [%d] flags[%d]",
            sAttr.type, sAttr.flags);
         return -ENOSYS;
     }

    if (pcmDevIds.size() == 0) {
        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
        return -EINVAL;
    }

    if (mState == SESSION_IDLE) {
        s->getBufInfo(&in_buf_size,&in_buf_count,&out_buf_size,&out_buf_count);
        memset(&config, 0, sizeof(config));

        switch(sAttr.direction) {
            case PAL_AUDIO_INPUT:
                pcm_flags = PCM_IN | PCM_MMAP | PCM_NOIRQ | PCM_MONOTONIC;
                config.rate = sAttr.in_media_config.sample_rate;
                config.format =
                    SessionAlsaUtils::palToAlsaFormat((uint32_t)sAttr.in_media_config.aud_fmt_id);
                config.channels = sAttr.in_media_config.ch_info.channels;
                config.period_size = SessionAlsaUtils::bytesToFrames(in_buf_size,
                    config.channels, config.format);
                config.period_count = in_buf_count;
                config.start_threshold = 0;
                config.stop_threshold = INT32_MAX;
                config.silence_threshold = 0;
                config.silence_size = 0;
                config.avail_min = config.period_size;
                break;
            case PAL_AUDIO_OUTPUT:
                pcm_flags = PCM_OUT | PCM_MMAP | PCM_NOIRQ | PCM_MONOTONIC;
                config.rate = sAttr.out_media_config.sample_rate;
                config.format =
                    SessionAlsaUtils::palToAlsaFormat((uint32_t)sAttr.out_media_config.aud_fmt_id);
                config.channels = sAttr.out_media_config.ch_info.channels;
                config.period_size = SessionAlsaUtils::bytesToFrames(out_buf_size,
                    config.channels, config.format);
                config.period_count = out_buf_count;
                config.start_threshold = config.period_size * 8;
                config.stop_threshold = INT32_MAX;
                config.silence_threshold = 0;
                config.silence_size = 0;
                config.avail_min = config.period_size;
                break;
            case PAL_AUDIO_INPUT | PAL_AUDIO_OUTPUT:
                return -EINVAL;
                break;
        }

        this->adjustMmapPeriodCount(&config, min_size_frames);

        PAL_DBG(LOG_TAG, "Opening PCM device card_id(%d) device_id(%d), channels %d",
                rm->getVirtualSndCard(), pcmDevIds.at(0), config.channels);

        pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0),
                             pcm_flags, &config);

        if ((!pcm || !pcm_is_ready(pcm)) && ecRefDevId != PAL_DEVICE_OUT_MIN) {
           PAL_ERR(LOG_TAG, "Failed to open EC graph")
           retryOpenWithoutEC(s, pcm_flags, &config);
        }

        if(!pcm) {
            PAL_ERR(LOG_TAG, "pcm open failed, status : %d", errno);
            step = "open";
            status = errno;
            goto exit;
        }

        if (!pcm_is_ready(pcm)) {
            PAL_ERR(LOG_TAG, "pcm open not ready, status : %d", errno);
            pcm = nullptr;
            step = "open";
            status = errno;
            goto exit;
        }

         status = pcm_mmap_begin(pcm, &info->buffer, &offset1, &frames1);
         if (status < 0)  {
             step = "begin";
             goto exit;
         }

         info->flags = 0;
         info->buffer_size_frames = pcm_get_buffer_size(pcm);
         buffer_size = pcm_frames_to_bytes(pcm, info->buffer_size_frames);
         info->burst_size_frames = config.period_size;


        CntrlName << stream << pcmDevIds.at(0) << " " << control;
        ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
        if (!ctl) {
            PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
            status = -ENOENT;
            goto exit;
        }

        //TODO call a mixer control to get the fd.
        memset(&buf_info, 0, sizeof(buf_info));
        status = mixer_ctl_get_array(ctl, (void *)&buf_info, sizeof(struct agm_buf_info));
        if (status < 0) {
            // Fall back to non exclusive mode
            info->fd = pcm_get_poll_fd(pcm);
        } else {
            info->fd = buf_info.data_buf_fd;
            //mmap_shared_memory_fd = buf_info->shared_memory_fd; // for closing later

            PAL_VERBOSE(LOG_TAG, "opened shared_memory_fd = %d",
                info->fd);

            if (buf_info.data_buf_size < buffer_size) {
                status = -EINVAL;
                step = "mmap";
                goto exit;
            }
            info->flags |= PAL_MMMAP_BUFF_FLAGS_APP_SHAREABLE;
        }
        memset(info->buffer, 0, pcm_frames_to_bytes(pcm,info->buffer_size_frames));

        status = pcm_mmap_commit(pcm, 0, SESSION_ALSA_MMAP_PERIOD_SIZE);
        if (status < 0) {
            step = "commit";
            goto exit;
        }

        //TODO
        //out->mmap_time_offset_nanos = get_mmap_out_time_offset();
        PAL_DBG(LOG_TAG, "got mmap buffer address %pK info->buffer_size_frames %d",
                info->buffer, info->buffer_size_frames);
        mState = SESSION_OPENED;
    }

 exit:
     if (status < 0) {
        if (pcm == NULL) {
            PAL_ERR(LOG_TAG, "%s - %d",step, status);
        } else {
            PAL_ERR(LOG_TAG, "%s - %d",step, status);
            if (pcm) {
                pcm_close(pcm);
                pcm = NULL;
            }
        }
     } else {
         status = 0;
     }
     return status;
 }

void SessionAlsaPcm::retryOpenWithoutEC(Stream *s, unsigned int pcm_flags, struct pcm_config *config)
{
    int status = 0;
    struct pal_device rxDevAttr = {};
    std::shared_ptr<Device> rx_dev;
    std::vector <std::shared_ptr<Device>> tx_devs;

    rxDevAttr.id = ecRefDevId;
    rx_dev = Device::getInstance(&rxDevAttr, rm);
    status = setECRef(s, rx_dev, false);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to reset EC, status : %d", status);
    }
    s->getAssociatedDevices(tx_devs);
    if (!tx_devs.size()) {
        PAL_ERR(LOG_TAG, "No tx device is associated with this stream");
        return;
    }
    for (int i = 0; i < tx_devs.size(); ++i) {
        status = rm->updateECDeviceMap_l(rx_dev, tx_devs[i], s, 0, false);
        if (status) {
            PAL_ERR(LOG_TAG, "Failed to update EC Device map for device %s, status: %d",
                    tx_devs[i]->getPALDeviceName().c_str(), status);
        }
    }
    if (pcmDevIds.size() > 0) {
        pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0),
                       pcm_flags, config);
    } else {
        PAL_ERR(LOG_TAG, "frontendIDs is not available.");
    }
}

 int SessionAlsaPcm::GetMmapPosition(Stream *s, struct pal_mmap_position *position)
 {
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct timespec ts = { 0, 0 };

#ifndef SEC_AUDIO_ADD_FOR_DEBUG
    PAL_DBG(LOG_TAG, "enter");
#endif

    if (pcm == NULL) {
        return -ENOSYS;
    }

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

     if (position == NULL) {
         return -EINVAL;
     }

    if (!(((sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY) &&
            (sAttr.flags & PAL_STREAM_FLAG_MMAP_NO_IRQ)) ||
            (sAttr.type == PAL_STREAM_VOICE_UI) || (sAttr.type == PAL_STREAM_ACD))) {
         PAL_ERR(LOG_TAG, "called on stream type [%d] flags[%d]",
            sAttr.type, sAttr.flags);
         return -ENOSYS;
     }

     status = pcm_mmap_get_hw_ptr(pcm, (unsigned int *)&position->position_frames, &ts);
     if (status < 0) {
         status = -errno;
         PAL_ERR(LOG_TAG, "%d", status);
         return status;
     }
     position->time_nanoseconds = ts.tv_sec*1000000000LL + ts.tv_nsec
             /*+ out->mmap_time_offset_nanos*/;
#ifndef SEC_AUDIO_ADD_FOR_DEBUG
     PAL_DBG(LOG_TAG, "Exit status: %d", status);
#endif
     return status;
 }

int SessionAlsaPcm::ResetMmapBuffer(Stream *s) {
    int status = 0;
    struct pal_stream_attributes sAttr = {};

    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        return status;
    }

    if (sAttr.type != PAL_STREAM_VOICE_UI) {
        return -EINVAL;
    }

    if (pcm) {
        status = pcm_ioctl(pcm, SNDRV_PCM_IOCTL_RESET);
        if (status)
            PAL_ERR(LOG_TAG, "Failed to reset mmap, status %d", status);
    } else {
        PAL_ERR(LOG_TAG, "cannot reset mmap as pcm not ready");
        status = -EINVAL;
    }

    return status;
}

// NOTE: only used by Voice UI for Google hotword api query
int SessionAlsaPcm::openGraph(Stream *s) {
    int status = 0;
    struct pcm_config config;
    struct pal_stream_attributes sAttr = {};
    std::vector<std::shared_ptr<Device>> associatedDevices;

    PAL_DBG(LOG_TAG, "Enter");
    status = s->getStreamAttributes(&sAttr);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "stream get attributes failed");
        goto exit;
    }

    if (sAttr.type != PAL_STREAM_VOICE_UI) {
        PAL_ERR(LOG_TAG, "Invalid stream type %d", sAttr.type);
        status = -EINVAL;
        goto exit;
    }

    status = open(s);
    if (status != 0) {
        PAL_ERR(LOG_TAG, "Failed to open session, status = %d", status);
        goto exit;
    }

    if (mState == SESSION_IDLE) {
        s->getBufInfo(&in_buf_size,&in_buf_count,&out_buf_size,&out_buf_count);
        memset(&config, 0, sizeof(config));

        config.rate = sAttr.in_media_config.sample_rate;
        config.format =
            SessionAlsaUtils::palToAlsaFormat((uint32_t)sAttr.in_media_config.aud_fmt_id);
        config.channels = sAttr.in_media_config.ch_info.channels;
        config.period_size = SessionAlsaUtils::bytesToFrames(in_buf_size,
            config.channels, config.format);
        config.period_count = in_buf_count;
        config.start_threshold = 0;
        config.stop_threshold = 0;
        config.silence_threshold = 0;

        if (pcmDevIds.size() > 0) {
            pcm = pcm_open(rm->getVirtualSndCard(), pcmDevIds.at(0), PCM_IN, &config);
        } else {
            PAL_ERR(LOG_TAG, "frontendIDs is not available.");
            status = -EINVAL;
            goto exit;
        }

        if (!pcm) {
            PAL_ERR(LOG_TAG, "pcm open failed");
            status = errno;
            goto exit;
        }

        if (!pcm_is_ready(pcm)) {
            PAL_ERR(LOG_TAG, "pcm open not ready");
            status = errno;
            goto exit;
        }

        mState = SESSION_OPENED;
    } else {
        PAL_ERR(LOG_TAG, "Invalid session state %d", mState);
        status = -EINVAL;
        goto exit;
    }
exit:
    PAL_DBG(LOG_TAG, "Exit status: %d", status);
    return status;
}

int32_t SessionAlsaPcm::configureInCallRxMFC(){
    std::vector <std::shared_ptr<Device>> devices;
    std::shared_ptr<Device> rxDev= nullptr;
    struct pal_device dattr = {};
    sessionToPayloadParam deviceData;
    typename std::vector<std::shared_ptr<Device>>::iterator iter;
    int32_t status = 0;

    status = rm->getActiveVoiceCallDevices(devices);
    if(devices.empty()){
        PAL_ERR(LOG_TAG, "Cannot start an in Call stream without a running voice call");
        status = -EINVAL;
        goto exit;
    }
    for (iter = devices.begin(); iter != devices.end(); iter++) {
        if ((*iter) && rm->isOutputDevId((*iter)->getSndDeviceId())) {
            status = (*iter)->getDeviceAttributes(&dattr);
            if (status) {
                PAL_ERR(LOG_TAG,"get Device attributes failed\n");
                status = -EINVAL;
                goto exit;
            }
            deviceData.bitWidth = dattr.config.bit_width;
            deviceData.sampleRate = dattr.config.sample_rate;
            deviceData.numChannel = dattr.config.ch_info.channels;
            deviceData.ch_info = nullptr;
            status = reconfigureModule(PER_STREAM_PER_DEVICE_MFC, "ZERO", &deviceData);
            break;
        }
    }
exit:
    return status;
}

int SessionAlsaPcm::reconfigureModule(uint32_t tagID, const char* BE, struct sessionToPayloadParam *data)
{
    uint32_t status =0;
    uint32_t miid = 0;
    uint8_t* payload = NULL;
    size_t payloadSize = 0;

    status = getMIID(BE, tagID, &miid);
    if(status){
        PAL_INFO(LOG_TAG,"could not find tagID 0x%x for backend %s", tagID, BE);
        goto exit;
    }
    PAL_DBG(LOG_TAG, "miid : %x id = %d\n", miid, pcmDevIds.at(0));
    builder->payloadMFCConfig(&payload, &payloadSize, miid, data);
    if (payloadSize && payload) {
        status = updateCustomPayload(payload, payloadSize);
        freeCustomPayload(&payload, &payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG,"updateCustomPayload Failed\n");
            status = -EINVAL;
            goto exit;
        }
    }
    status = SessionAlsaUtils::setMixerParameter(mixer, pcmDevIds.at(0),
                                                customPayload, customPayloadSize);
    freeCustomPayload();
    if (status) {
        PAL_ERR(LOG_TAG, "setMixerParameter failed");
        goto exit;
    }

exit:
    return status;
}

int SessionAlsaPcm::notifyUPDToneRendererFmtChng(struct pal_device *dAttr,
        us_tone_renderer_ep_media_format_status_t event)
{
    std::string backEndName;
    int status = 0;
    int device = 0;
    uint32_t miid = 0;
    int tagId = 0;
    uint8_t* paramData = NULL;
    size_t paramSize = 0;

    if (pcmDevIds.size() > 0)
        device = pcmDevIds.at(0);

    rm->getBackendName(dAttr->id, backEndName);

    status = getMIID(backEndName.c_str(), TAG_TONE_RENDERER_MODULE, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tag info %x, status = %d",
                tagId, status);
        return status;
    }

    builder->USToneRendererNotifyPayload(&paramData, &paramSize, dAttr, miid, event);
    if (paramSize) {
        status = SessionAlsaUtils::setMixerParameter(mixer, device,
                                                paramData, paramSize);
        PAL_INFO(LOG_TAG, "mixer notify US tone renderer format change,"
                    " status=%d", status);
        return status;
    }

    return 0;
}
