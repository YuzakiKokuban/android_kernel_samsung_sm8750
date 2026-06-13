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

#define LOG_TAG "PAL: SessionAlsaUtils"

#include "SessionAlsaUtils.h"

#include <sstream>
#include <string>
#include <set>
//#include "SessionAlsa.h"
//#include "SessionAlsaPcm.h"
//#include "SessionAlsaCompress.h"
#include "SessionAlsaVoice.h"
#include "ResourceManager.h"
#include "StreamSoundTrigger.h"
#include <agm/agm_api.h>
#include "spr_api.h"
#include "apm_api.h"
#include <tinyalsa/asoundlib.h>
#include <sound/asound.h>


static constexpr const char* const COMPRESS_SND_DEV_NAME_PREFIX = "COMPRESS";
static constexpr const char* const PCM_SND_DEV_NAME_PREFIX = "PCM";
static constexpr const char* const PCM_SND_VOICE_DEV_NAME_PREFIX = "VOICEMMODE";

static const char *feCtrlNames[] = {
    " control",
    " metadata",
    " connect",
    " disconnect",
    " setParam",
    " getTaggedInfo",
    " setParamTag",
    " getParam",
    " echoReference",
    " Sidetone",
    " loopback",
    " event",
    " setcal",
    " flush"
};

static const char *beCtrlNames[] = {
    " metadata",
    " rate ch fmt",
    " setParam",
    " grp config",
};

struct agmMetaData {
    uint8_t *buf;
    uint32_t size;
    agmMetaData(uint8_t *b, uint32_t s)
        :buf(b),size(s) {}
};

SessionAlsaUtils::~SessionAlsaUtils()
{

}

bool SessionAlsaUtils::isRxDevice(uint32_t devId)
{
    if ((devId > PAL_DEVICE_OUT_MIN) && (devId < PAL_DEVICE_OUT_MAX))
        return true;

    return false;
}

std::shared_ptr<Device> SessionAlsaUtils::getDeviceObj(int32_t beDevId,
        std::vector<std::shared_ptr<Device>> &associatedDevices)
{
    for (int i = 0; i < associatedDevices.size(); ++i) {
        if (beDevId == associatedDevices[i]->getSndDeviceId())
            return associatedDevices[i];
    }
    return nullptr;
}

unsigned int SessionAlsaUtils::bitsToAlsaFormat(unsigned int bits)
{
    switch (bits) {
        case 32:
            return SNDRV_PCM_FORMAT_S32_LE;
        case 8:
            return SNDRV_PCM_FORMAT_S8;
        case 24:
            return SNDRV_PCM_FORMAT_S24_3LE;
        default:
        case 16:
            return SNDRV_PCM_FORMAT_S16_LE;
    };
}

static unsigned int palToSndDriverFormat(uint32_t fmt_id)
{
    switch (fmt_id) {
        case PAL_AUDIO_FMT_PCM_S32_LE:
            return SNDRV_PCM_FORMAT_S32_LE;
        case PAL_AUDIO_FMT_PCM_S8:
            return SNDRV_PCM_FORMAT_S8;
        case PAL_AUDIO_FMT_PCM_S24_3LE:
            return SNDRV_PCM_FORMAT_S24_3LE;
        case PAL_AUDIO_FMT_PCM_S24_LE:
            return SNDRV_PCM_FORMAT_S24_LE;
        default:
        case PAL_AUDIO_FMT_PCM_S16_LE:
            return SNDRV_PCM_FORMAT_S16_LE;
    };
}

pcm_format SessionAlsaUtils::palToAlsaFormat(uint32_t fmt_id)
{
    switch (fmt_id) {
        case PAL_AUDIO_FMT_PCM_S32_LE:
            return PCM_FORMAT_S32_LE;
        case PAL_AUDIO_FMT_PCM_S8:
            return PCM_FORMAT_S8;
        case PAL_AUDIO_FMT_PCM_S24_3LE:
            return PCM_FORMAT_S24_3LE;
        case PAL_AUDIO_FMT_PCM_S24_LE:
            return PCM_FORMAT_S24_LE;
        default:
        case PAL_AUDIO_FMT_PCM_S16_LE:
            return PCM_FORMAT_S16_LE;
    };
}

int SessionAlsaUtils::setMixerCtlData(struct mixer_ctl *ctl, MixerCtlType id, void *data, int size)
{

    int rc = -EINVAL;

    if (!ctl || !data || size == 0) {
        PAL_ERR(LOG_TAG, "invalid mixer ctrl data passed");
        goto error;
    }

    switch (id) {
        case MixerCtlType::MIXER_SET_ID_STRING:
            mixer_ctl_set_enum_by_string(ctl, (const char *)data);
            break;
        case MixerCtlType::MIXER_SET_ID_VALUE:
            mixer_ctl_set_value(ctl, SNDRV_CTL_ELEM_TYPE_BYTES, *((int *)data));
            break;
        case MixerCtlType::MIXER_SET_ID_ARRAY:
            mixer_ctl_set_array(ctl, data, size);
            break;
    }

error:
    return rc;

}

void SessionAlsaUtils::getAgmMetaData(const std::vector <std::pair<int, int>> &kv,
        const std::vector <std::pair<int, int>> &ckv, struct prop_data *propData,
        struct agmMetaData &md)
{
    uint8_t *metaData = NULL;
    uint8_t *ptr = NULL;
    struct agm_key_value *kvPtr = NULL;
    uint32_t mdSize = 0;

    md.buf = nullptr;
    md.size = 0;

    // kv/ckv/propData may be empty when clearing metadata, skip size check
    mdSize = sizeof(uint32_t) * 4 +
        ((kv.size() + ckv.size()) * sizeof(struct agm_key_value));
    if (propData)
        mdSize += sizeof(uint32_t) * propData->num_values;

    metaData = (uint8_t*)calloc(1, mdSize);
    if (!metaData) {
        PAL_ERR(LOG_TAG, "Failed to allocate memory for agm meta data");
        return;
    }

    // Fill in gkv part
    ptr = metaData;
    *((uint32_t *)ptr) = kv.size();
    ptr += sizeof(uint32_t);
    kvPtr = (struct agm_key_value *)ptr;
    for (int i = 0; i < kv.size(); i++) {
        kvPtr[i].key = kv[i].first;
        kvPtr[i].value = kv[i].second;
    }

    // Fill in ckv part
    ptr += kv.size() * sizeof(struct agm_key_value);
    *((uint32_t *)ptr) = ckv.size();
    ptr += sizeof(uint32_t);
    kvPtr = (struct agm_key_value *)ptr;
    for (int i = 0; i < ckv.size(); i++) {
        kvPtr[i].key = ckv[i].first;
        kvPtr[i].value = ckv[i].second;
    }

    // Fill in prop info if any
    if (propData) {
        ptr += ckv.size() * sizeof(struct agm_key_value);
        *((uint32_t *)ptr) = propData->prop_id;
        ptr += sizeof(uint32_t);
        *((uint32_t *)ptr) = propData->num_values;
        ptr += sizeof(uint32_t);
        if (propData->num_values != 0)
            memcpy(ptr, (uint8_t *)propData->values, sizeof(uint32_t) * propData->num_values);
    }
    md.buf = metaData;
    md.size = mdSize;
}

int SessionAlsaUtils::getTagMetadata(int32_t tagsent, std::vector <std::pair<int, int>> &tkv,
        struct agm_tag_config *tagConfig)
{
    int status = 0;
    if (tkv.size() == 0 || !tagConfig) {
        PAL_ERR(LOG_TAG, "invalid key values passed");
        status = -EINVAL;
        goto error;
    }

    tagConfig->tag = tagsent;
    tagConfig->num_tkvs = tkv.size();

    for (int i=0;i < tkv.size(); i++) {
        tagConfig->kv[i].key = tkv[i].first;
        tagConfig->kv[i].value = tkv[i].second;
    }
error:
    return status;

}

int SessionAlsaUtils::getCalMetadata(std::vector <std::pair<int,int>> &ckv, struct agm_cal_config* calConfig)
{
    int status = 0;
    if (ckv.size() == 0 || !calConfig) {
        PAL_ERR(LOG_TAG, "invalid key values passed");
        status = -EINVAL;
        goto error;
    }

    calConfig->num_ckvs = ckv.size();

    for (int i = 0; i < ckv.size(); i++) {
        calConfig->kv[i].key = ckv[i].first;
        calConfig->kv[i].value = ckv[i].second;
    }

error:
    return status;
}

bool SessionAlsaUtils::isMmapUsecase(struct pal_stream_attributes &sAttr)
{

    return ((sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY) &&
            ((sAttr.flags & PAL_STREAM_FLAG_MMAP_MASK)
                        ||(sAttr.flags & PAL_STREAM_FLAG_MMAP_NO_IRQ_MASK))
            );

}

struct mixer_ctl *SessionAlsaUtils::getStaticMixerControl(struct mixer *am, std::string name)
{
    std::ostringstream cntrlName;

    cntrlName << name;
    PAL_DBG(LOG_TAG, "mixer control name is %s", cntrlName.str().data());

    return mixer_get_ctl_by_name(am, cntrlName.str().data());
}

struct mixer_ctl *SessionAlsaUtils::getFeMixerControl(struct mixer *am, std::string feName,
        uint32_t idx)
{
    std::ostringstream cntrlName;
    struct mixer_ctl *ctl = NULL;

    cntrlName << feName << feCtrlNames[idx];
    PAL_DBG(LOG_TAG, "mixer control %s", cntrlName.str().data());
    ctl = mixer_get_ctl_by_name(am, cntrlName.str().data());
    if (!ctl)
        PAL_FATAL(LOG_TAG, "invalid mixer control: %s", cntrlName.str().data());

    return ctl;
}

struct mixer_ctl *SessionAlsaUtils::getBeMixerControl(struct mixer *am, std::string beName,
        uint32_t idx)
{
    std::ostringstream cntrlName;

    cntrlName << beName << beCtrlNames[idx];
    PAL_DBG(LOG_TAG, "mixer control %s", cntrlName.str().data());
    return mixer_get_ctl_by_name(am, cntrlName.str().data());
}

int SessionAlsaUtils::getScoDevCount(void)
{
    std::shared_ptr<Device> dev = nullptr;
    struct pal_device scoDAttr = {};
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    scoDAttr.id = PAL_DEVICE_OUT_BLUETOOTH_SCO;
    dev = Device::getInstance(&scoDAttr, rm);
    if (dev == 0) {
        PAL_ERR(LOG_TAG, "device_id[%d] Instance query failed", scoDAttr.id );
        return 0;
    }

    return dev->getDeviceCount();
}

int SessionAlsaUtils::open(Stream * streamHandle, std::shared_ptr<ResourceManager> rmHandle,
    const std::vector<int> &DevIds, const std::vector<std::pair<int32_t, std::string>> &BackEnds)
{
    std::vector <std::pair<int, int>> streamKV;
    std::vector <std::pair<int, int>> streamCKV;
    std::vector <std::pair<int, int>> streamDeviceKV;
    std::vector <std::pair<int, int>> deviceKV;
    std::vector <std::pair<int, int>> devicePPCKV;
    std::vector <std::pair<int, int>> deviceCKV;
    std::vector <std::pair<int, int>> emptyKV;
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct agmMetaData streamMetaData(nullptr, 0);
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct agmMetaData streamDeviceMetaData(nullptr, 0);
    std::ostringstream feName;
    struct mixer_ctl *feMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    std::shared_ptr<Device> beDevObj = nullptr;
    struct mixer *mixerHandle;
    uint32_t i;
    uint32_t streamPropId[] = {0x08000010, 1, 0x1}; /** gsl_subgraph_platform_driver_props.xml */
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    uint32_t streamDevicePropId[] = {0x08000010, 1, 0x3}; /** gsl_subgraph_platform_driver_props.xml */
    struct pal_device_info devinfo = {};
    struct pal_device dAttr;
    PayloadBuilder* builder = nullptr;

    PAL_DBG(LOG_TAG, "Entry \n");

    memset(&dAttr, 0, sizeof(pal_device));
    status = streamHandle->getStreamAttributes(&sAttr);
    if(0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }

    if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD && sAttr.type != PAL_STREAM_VOICE_CALL_MUSIC) {
        status = streamHandle->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
            goto exit;
        }
    }

    builder = new PayloadBuilder();

    if (!(sAttr.type == PAL_STREAM_HAPTICS &&
              (sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH))) {
        // get streamKV
        if ((status = builder->populateStreamKV(streamHandle, streamKV)) != 0) {
            PAL_ERR(LOG_TAG, "get stream KV failed %d", status);
            goto exit;
        }
    }

    switch (sAttr.type) {
        case PAL_STREAM_ACD :
        case PAL_STREAM_CONTEXT_PROXY :
        case PAL_STREAM_COMMON_PROXY:
        case PAL_STREAM_SENSOR_PCM_DATA:
            // No need to set volume ckv
        break;
        case PAL_STREAM_LOOPBACK:
            if ((sAttr.info.opt_stream_info.loopback_type ==
                            PAL_STREAM_LOOPBACK_PLAYBACK_ONLY) ||
                (sAttr.info.opt_stream_info.loopback_type ==
                            PAL_STREAM_LOOPBACK_CAPTURE_ONLY)) {
                // Will continue without setting volume CKV
                break;
            }
            [[fallthrough]]; //Intentional fallthrough
        default:
            // Set the volume CKV
            status = builder->populateStreamCkv(streamHandle, streamCKV, 0,
                    (struct pal_volume_data **)nullptr);
            if (status) {
                PAL_ERR(LOG_TAG, "get stream ckv failed %d", status);
                goto exit;
            }

    }

    if ((streamKV.size() > 0) || (streamCKV.size() > 0)) {
        getAgmMetaData(streamKV, streamCKV, (struct prop_data *)streamPropId,
                streamMetaData);
        if (!streamMetaData.size) {
            PAL_ERR(LOG_TAG, "stream metadata is zero");
            status = -ENOMEM;
            goto exit;
        }
    }
    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    /** Get mixer controls (struct mixer_ctl *) for both FE and BE */
    if (sAttr.type == PAL_STREAM_COMPRESSED)
        feName << COMPRESS_SND_DEV_NAME_PREFIX << DevIds.at(0);
    else
        feName << PCM_SND_DEV_NAME_PREFIX << DevIds.at(0);
    for (i = FE_CONTROL; i <= FE_CONNECT; ++i) {
        feMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle, feName.str(), i);
        if (!feMixerCtrls[i]) {
            PAL_ERR(LOG_TAG, "invalid mixer control: %s%s", feName.str().data(),
                   feCtrlNames[i]);
            status = -EINVAL;
            goto freeStreamMetaData;
        }
    }
    mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONTROL], "ZERO");
    if (streamMetaData.size)
        mixer_ctl_set_array(feMixerCtrls[FE_METADATA], (void *)streamMetaData.buf,
                streamMetaData.size);

    for (std::vector<std::pair<int32_t, std::string>>::const_iterator be = BackEnds.begin();
           be != BackEnds.end(); ++be) {
        if ((status = builder->populateDeviceKV(streamHandle, be->first, deviceKV)) != 0) {
            PAL_ERR(LOG_TAG, "get device KV failed %d", status);
            goto freeStreamMetaData;
        }

        if (sAttr.direction == PAL_AUDIO_OUTPUT) {
            if (!(sAttr.type == PAL_STREAM_HAPTICS &&
                 (sAttr.info.opt_stream_info.haptics_type == PAL_STREAM_HAPTICS_TOUCH)))
                status = builder->populateDevicePPKV(streamHandle, be->first, streamDeviceKV, 0,
                          emptyKV);
        } else {
            for (i = 0; i < associatedDevices.size(); i++) {
                associatedDevices[i]->getDeviceAttributes(&dAttr, streamHandle);
                if (be->first == dAttr.id) {
                    break;
                }
            }
            if (i >= associatedDevices.size() ) {
                PAL_ERR(LOG_TAG, "could not find associated device kv cannot be set");

            } else{
                rmHandle->getDeviceInfo((pal_device_id_t)be->first, sAttr.type,
                                        dAttr.custom_config.custom_key, &devinfo);
            }
            status = builder->populateDevicePPKV(streamHandle, 0, emptyKV, be->first,
                     streamDeviceKV);
        }
        if (status != 0) {
            PAL_VERBOSE(LOG_TAG, "get device PP KV failed %d", status);
            status = 0; /**< ignore device PP KV failures */
        }
        status = builder->populateDevicePPCkv(streamHandle, devicePPCKV);
        if (status) {
            PAL_ERR(LOG_TAG, "populateDevicePP Ckv failed %d", status);
            status = 0; /**< ignore device PP CKV failures */
        }

        status = builder->populateStreamDeviceKV(streamHandle, be->first, streamDeviceKV);
        if (status) {
            PAL_VERBOSE(LOG_TAG, "get stream device KV failed %d", status);
            status = 0; /**< ignore stream device KV failures */
        }

        deviceCKV.clear();

        if (ResourceManager::isSpeakerProtectionEnabled) {
            PAL_DBG(LOG_TAG, "Speaker protection enabled");
            if (be->first == PAL_DEVICE_OUT_SPEAKER) {
                status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                                SPKR_PROT_ENABLE);
                if (status != 0) {
                    PAL_VERBOSE(LOG_TAG, "Unable to populate SP cal");
                    status = 0; /**< ignore device SP CKV failures */
                }
            }
        }

        if (ResourceManager::isHandsetProtectionEnabled &&
               ResourceManager::isSpeakerProtectionEnabled) {
            PAL_DBG(LOG_TAG, "Handset enabled");
            if (be->first == PAL_DEVICE_OUT_HANDSET) {
                status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                            HANDSET_PROT_ENABLE);
                if (status != 0) {
                    PAL_VERBOSE(LOG_TAG, "Unable to populate SP cal");
                    status = 0; /**< ignore device SP CKV failures */
                }
            }
        }

        if (be->first == PAL_DEVICE_OUT_HAPTICS_DEVICE &&
                                ResourceManager::isHapticsthroughWSA) {
            status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                                 HAPTICS_PROT_ENABLE);
            if (status != 0) {
                PAL_VERBOSE(LOG_TAG, "Unable to populate HP cal");
                status = 0; /**< ignore device HP CKV failures */
            }
        }

        if (deviceKV.size() > 0) {
            getAgmMetaData(deviceKV, deviceCKV, (struct prop_data *)devicePropId,
                    deviceMetaData);
            if (!deviceMetaData.size) {
                PAL_ERR(LOG_TAG, "device metadata is zero");
                status = -ENOMEM;
                goto freeMetaData;
            }
        }

        if (streamDeviceKV.size() > 0 || devicePPCKV.size() > 0) {
            getAgmMetaData(streamDeviceKV, devicePPCKV, (struct prop_data *)streamDevicePropId,
                    streamDeviceMetaData);
            if (!streamDeviceMetaData.size) {
                PAL_ERR(LOG_TAG, "stream/device metadata is zero");
                status = -ENOMEM;
                goto freeMetaData;
            }
        }
        beMetaDataMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle, be->second, BE_METADATA);
        if (!beMetaDataMixerCtrl) {
#ifdef SEC_AUDIO_FACTORY_TEST_MODE        
            PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", be->second.data(),
                    beCtrlNames[BE_METADATA]);
#else
            PAL_FATAL(LOG_TAG, "invalid mixer control: %s %s", be->second.data(),
                    beCtrlNames[BE_METADATA]);
#endif                    
            status = -EINVAL;
            goto freeMetaData;
        }

        /** set mixer controls */
        if (deviceMetaData.size)
            mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                    deviceMetaData.size);
        mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONTROL], be->second.data());
        if (streamDeviceMetaData.size) {
            mixer_ctl_set_array(feMixerCtrls[FE_METADATA], (void *)streamDeviceMetaData.buf,
                    streamDeviceMetaData.size);
        }
        mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONNECT], (be->second).data());

        deviceKV.clear();
        streamDeviceKV.clear();
        if (streamDeviceMetaData.buf)
            free(streamDeviceMetaData.buf);
        if (deviceMetaData.buf)
            free(deviceMetaData.buf);
        streamDeviceMetaData.buf = nullptr;
        deviceMetaData.buf = nullptr;
    }
freeMetaData:
    if (streamDeviceMetaData.buf)
        free(streamDeviceMetaData.buf);
    if (deviceMetaData.buf)
        free(deviceMetaData.buf);
freeStreamMetaData:
    if (streamMetaData.buf)
        free(streamMetaData.buf);
exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    PAL_DBG(LOG_TAG, "Exit, status %d", status);
    return status;
}

int SessionAlsaUtils::close(Stream * streamHandle, std::shared_ptr<ResourceManager> rmHandle,
    const std::vector<int> &DevIds, const std::vector<std::pair<int32_t, std::string>> &BackEnds,
    std::vector<std::pair<std::string, int>> &freedevicemetadata)
{
    int status = 0;
    uint32_t i;
    std::vector <std::pair<int, int>> emptyKV;
    struct pal_stream_attributes sAttr = {};
    struct agmMetaData streamMetaData(nullptr, 0);
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct agmMetaData streamDeviceMetaData(nullptr, 0);
    uint32_t streamPropId[] = {0x08000010, 1, 0x1}; /** gsl_subgraph_platform_driver_props.xml */
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    uint32_t streamDevicePropId[] = {0x08000010, 1, 0x3}; /** gsl_subgraph_platform_driver_props.xml */
    std::ostringstream feName;
    struct mixer_ctl *feMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    struct mixer *mixerHandle = nullptr;

    status = streamHandle->getStreamAttributes(&sAttr);
    if(0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }

    if (DevIds.size() <= 0) {
        PAL_ERR(LOG_TAG, "DevIds size is invalid \n");
        goto exit;
    }

    /** Get mixer controls (struct mixer_ctl *) for both FE and BE */
    if (sAttr.type == PAL_STREAM_COMPRESSED)
        feName << COMPRESS_SND_DEV_NAME_PREFIX << DevIds.at(0);
    else
        feName << PCM_SND_DEV_NAME_PREFIX << DevIds.at(0);

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "Error: Failed to get mixer handle\n");
        goto exit;
    }

    for (i = FE_CONTROL; i <= FE_DISCONNECT; ++i) {
        feMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle,
            feName.str(), i);
        if (!feMixerCtrls[i]) {
            PAL_ERR(LOG_TAG, "invalid mixer control: %s %s",
                feName.str().data(), feCtrlNames[i]);
            status = -EINVAL;
            goto exit;
        }
    }

    // clear device metadata
    for (auto be = BackEnds.begin(); be != BackEnds.end(); ++be) {
        getAgmMetaData(emptyKV, emptyKV, (struct prop_data *)devicePropId,
                deviceMetaData);
        getAgmMetaData(emptyKV, emptyKV, (struct prop_data *)streamDevicePropId,
                streamDeviceMetaData);
        if (!deviceMetaData.size || !streamDeviceMetaData.size) {
            PAL_ERR(LOG_TAG, "stream/device metadata is zero");
            status = -ENOMEM;
            goto freeMetaData;
        }
        beMetaDataMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle, be->second, BE_METADATA);
        if (!beMetaDataMixerCtrl) {
            PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", be->second.data(),
                    beCtrlNames[BE_METADATA]);
            status = -EINVAL;
            goto freeMetaData;
        }

        /** set mixer controls */
        mixer_ctl_set_enum_by_string(feMixerCtrls[FE_DISCONNECT], be->second.data());
        for (auto freeDevmeta = freedevicemetadata.begin(); freeDevmeta != freedevicemetadata.end(); ++freeDevmeta) {
            PAL_DBG(LOG_TAG, "backend %s and freedevicemetadata %d", freeDevmeta->first.data(), freeDevmeta->second);
            if (!(freeDevmeta->first.compare(be->second))) {
                if (freeDevmeta->second == 0) {
                    PAL_INFO(LOG_TAG, "No need to free device metadata as device is still active");
                } else {
                    mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                                    deviceMetaData.size);
                }
            }
        }

        mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONTROL], be->second.data());
        mixer_ctl_set_array(feMixerCtrls[FE_METADATA], (void *)streamDeviceMetaData.buf,
                streamDeviceMetaData.size);

        free(streamDeviceMetaData.buf);
        free(deviceMetaData.buf);
        streamDeviceMetaData.buf = nullptr;
        deviceMetaData.buf = nullptr;
    }

    // clear stream metadata
    mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONTROL], "ZERO");
    getAgmMetaData(emptyKV, emptyKV, (struct prop_data *)streamPropId,
            streamMetaData);
    if (streamMetaData.size)
        mixer_ctl_set_array(feMixerCtrls[FE_METADATA],
            (void *)streamMetaData.buf, streamMetaData.size);


freeMetaData:
    if (streamDeviceMetaData.buf)
        free(streamDeviceMetaData.buf);
    if (deviceMetaData.buf)
        free(deviceMetaData.buf);
    if (streamMetaData.buf)
        free(streamMetaData.buf);
exit:
    return status;
}

int SessionAlsaUtils::rwACDBTunnel(Stream * streamHandle, std::shared_ptr<ResourceManager> rmHandle,
    pal_device_id_t deviceId, void *payload, bool isParamWrite, uint32_t instanceId)
{
    std::vector <std::pair<int, int>> acdbGKV;
    std::vector <std::pair<int, int>> emptyKV;
    std::set <std::pair<int, int>> acdbGKVSet;
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct mixer_ctl *acdbMixerCtrl = nullptr;
    struct mixer *mixerHandle = nullptr;
    uint32_t i;
    PayloadBuilder* builder = nullptr;
    struct vsid_info vsidDummy = {};
    std::string acdbSetMixerName = "setACDBTunnel";
    std::string acdbGetMixerName = "getACDBTunnel";
    uint8_t *payloadData = NULL;
    agm_acdb_param *effectACDBPayload = nullptr;
    pal_param_payload *paramPayload = nullptr;
    size_t payloadSize = 0;
    uint32_t checkSum = 0;

    paramPayload = (pal_param_payload *)payload;
    if (!paramPayload)
        return -EINVAL;

    effectACDBPayload = (agm_acdb_param *)(paramPayload->payload);
    if (!effectACDBPayload)
        return -EINVAL;

    PAL_DBG(LOG_TAG, "Entry \n");

    status = streamHandle->getStreamAttributes(&sAttr);
    if(0 != status) {
        PAL_ERR(LOG_TAG,"getStreamAttributes Failed \n");
        goto exit;
    }

    builder = new PayloadBuilder();
    if (!builder) {
        PAL_ERR(LOG_TAG, "Payload builder creation failed \n");
        goto exit;
    }

    // get streamKV
    if ((status = builder->populateStreamKVTunnel(streamHandle, acdbGKV, instanceId)) != 0) {
        PAL_ERR(LOG_TAG, "get stream KV failed %d", status);
        goto exit;
    }

    for (auto kv = acdbGKV.begin(); kv != acdbGKV.end(); kv++) {
        PAL_DBG(LOG_TAG, "stream kv key=0x%x value=0x%x",
                    kv->first, kv->second);
    }

    if ((status = builder->populateDeviceKVTunnel(streamHandle, deviceId, acdbGKV)) != 0) {
        PAL_ERR(LOG_TAG, "get device KV failed %d", status);
        goto freePaylodData;
    }

    for (auto kv = acdbGKV.begin(); kv != acdbGKV.end(); kv++) {
        PAL_DBG(LOG_TAG, "device kv key=0x%x value=0x%x",
                    kv->first, kv->second);
    }

    status = builder->populateDevicePPKVTunnel(streamHandle, deviceId, acdbGKV);
    if (status) {
        PAL_ERR(LOG_TAG, "get device KV failed %d", status);
        goto freePaylodData;
    }

    for (auto kv = acdbGKV.begin(); kv != acdbGKV.end(); kv++) {
        PAL_DBG(LOG_TAG, "device pp kv key=0x%x value=0x%x",
                    kv->first, kv->second);
    }

    status = builder->populateStreamDeviceKV(streamHandle, deviceId, acdbGKV,
                0, emptyKV, vsidDummy, SIDETONE_OFF);

    if (status) {
        PAL_ERR(LOG_TAG, "get stream device KV failed %d", status);
        goto freePaylodData;
    }

    for (auto kv = acdbGKV.begin(); kv != acdbGKV.end(); kv++) {
        PAL_DBG(LOG_TAG, "stream device kv key=0x%x value=0x%x",
                    kv->first, kv->second);
    }

    /* remove duplicate element */
    for (const auto & i : acdbGKV) {
        acdbGKVSet.insert(i);
    }

    acdbGKV.clear();
    acdbGKV.assign(acdbGKVSet.begin(), acdbGKVSet.end());
    PAL_INFO(LOG_TAG, "ACDB gkv size = 0x%x", acdbGKV.size());
    for (auto kv = acdbGKV.begin(); kv != acdbGKV.end(); kv++) {
        PAL_INFO(LOG_TAG, "unique gkv key=0x%x value=0x%x",
                    kv->first, kv->second);
    }

    status = builder->payloadACDBTunnelParam(&payloadData,
         &payloadSize, (uint8_t *)effectACDBPayload, acdbGKVSet,
                                0, sAttr.out_media_config.sample_rate);
    if (!payloadData) {
        PAL_ERR(LOG_TAG, "failed to allocate memory. status = %d", status);
        goto exit;
    }

    PAL_DBG(LOG_TAG, "payload size = 0x%x", payloadSize);
    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "Error: Failed to get mixer handle\n");
        goto freePaylodData;
    }

    if (isParamWrite) {
        acdbMixerCtrl = SessionAlsaUtils::getStaticMixerControl(mixerHandle,
                            acdbSetMixerName);
    } else {
        acdbMixerCtrl = SessionAlsaUtils::getStaticMixerControl(mixerHandle,
                            acdbGetMixerName);
    }

    if (!acdbMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control");
        status = -EINVAL;
        goto freePaylodData;
    }

    /* set mixer controls */
    if (payloadSize)
        status = mixer_ctl_set_array(acdbMixerCtrl, payloadData,
                payloadSize);

    if (!isParamWrite && !status) {
        // parameter read. payloadData is used as reading buffer now
        status = mixer_ctl_get_array(acdbMixerCtrl, payloadData, payloadSize);

        struct agm_acdb_tunnel_param *newPayloadACDBTunnelInfo =
            (struct agm_acdb_tunnel_param *)payloadData;
        PAL_INFO(LOG_TAG, "istkv=0x%x tag=0x%x gkv=0x%x kv = 0x%x blob_size=0x%x",
                    newPayloadACDBTunnelInfo->isTKV,
                    newPayloadACDBTunnelInfo->tag,
                    newPayloadACDBTunnelInfo->num_gkvs,
                    newPayloadACDBTunnelInfo->num_kvs,
                    newPayloadACDBTunnelInfo->blob_size);

        uint8_t *ptrBuffer = nullptr;
        ptrBuffer = newPayloadACDBTunnelInfo->blob +
            (newPayloadACDBTunnelInfo->num_gkvs + newPayloadACDBTunnelInfo->num_kvs) *
            sizeof(pal_key_value_pair_t) + sizeof(struct apm_module_param_data_t);
        __builtin_add_overflow(effectACDBPayload->num_kvs * sizeof(pal_key_value_pair_t),
            sizeof(pal_effect_custom_payload_t), &checkSum);
        __builtin_sub_overflow(effectACDBPayload->blob_size, checkSum, &payloadSize);
        PAL_DBG(LOG_TAG, "payload size = 0x%x", payloadSize);
        ar_mem_cpy((uint8_t *)(effectACDBPayload->blob +
            sizeof(pal_effect_custom_payload_t) +
            effectACDBPayload->num_kvs * sizeof(pal_key_value_pair_t)),
            payloadSize, ptrBuffer, payloadSize);
    }

freePaylodData:
    if (payloadData)
        free(payloadData);

exit:
    acdbGKV.clear();
    acdbGKVSet.clear();
    if(builder) {
       delete builder;
       builder = NULL;
    }

    PAL_DBG(LOG_TAG,"Exit, status %d", status);

    return status;
}

int SessionAlsaUtils::setDeviceCustomPayload(std::shared_ptr<ResourceManager> rmHandle,
                                           std::string backEndName, void *payload, size_t size)
{
    struct mixer_ctl *ctl = NULL;
    struct mixer *mixerHandle = NULL;
    int status = 0;

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "Error: Failed to get mixer handle\n");
        return status;
    }

    ctl = SessionAlsaUtils::getBeMixerControl(mixerHandle, backEndName, BE_SETPARAM);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", backEndName.c_str(),
                beCtrlNames[BE_SETPARAM]);
        return -EINVAL;
    }

    return mixer_ctl_set_array(ctl, payload, size);
}

int SessionAlsaUtils::setDeviceMetadata(std::shared_ptr<ResourceManager> rmHandle,
                                           std::string backEndName,
                                           std::vector <std::pair<int, int>> &deviceKV)
{
    std::vector <std::pair<int, int>> emptyKV;
    int status = 0;
    struct agmMetaData deviceMetaData(nullptr, 0);
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    struct mixer *mixerHandle = NULL;
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "failed to get mixer handle\n");
        return status;
    }

    beMetaDataMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle,
                                                     backEndName, BE_METADATA);
    if (!beMetaDataMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", backEndName.c_str(),
                          beCtrlNames[BE_METADATA]);
        return -EINVAL;
    }

    if (deviceKV.size() > 0) {
        getAgmMetaData(deviceKV, emptyKV, (struct prop_data *)devicePropId,
                deviceMetaData);
        if (!deviceMetaData.size) {
            PAL_ERR(LOG_TAG, "get device meta data failed %d", status);
            return -ENOMEM;
        }
    }

    if (deviceMetaData.size)
        status = mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                        deviceMetaData.size);

    free(deviceMetaData.buf);
    deviceMetaData.buf = nullptr;

    return status;
}

int SessionAlsaUtils::setDeviceMediaConfig(std::shared_ptr<ResourceManager> rmHandle,
                                           std::string backEndName, struct pal_device *dAttr)
{
    struct mixer_ctl *ctl = NULL;
    long aif_media_config[4];
    long aif_group_atrr_config[5];
    struct mixer *mixerHandle = NULL;
    int status = 0;

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "Error: Failed to get mixer handle\n");
        return status;
    }

    aif_media_config[0] = dAttr->config.sample_rate;
    aif_media_config[1] = dAttr->config.ch_info.channels;

    if (!isPalPCMFormat((uint32_t)dAttr->config.aud_fmt_id)) {
        /*
         *Only for configuring the BT A2DP device backend we use
         *bitwidth instead of aud_fmt_id
         */
        aif_media_config[2] = bitsToAlsaFormat(dAttr->config.bit_width);
        aif_media_config[3] = AGM_DATA_FORMAT_COMPR_OVER_PCM_PACKETIZED;
    } else {
        aif_media_config[2] = palToSndDriverFormat((uint32_t)dAttr->config.aud_fmt_id);
        aif_media_config[3] = AGM_DATA_FORMAT_FIXED_POINT;
    }

    // if it's virtual port, need to set group attribute as well
    if (rmHandle->activeGroupDevConfig &&
        (dAttr->id == PAL_DEVICE_OUT_SPEAKER ||
        dAttr->id == PAL_DEVICE_OUT_HANDSET ||
        dAttr->id == PAL_DEVICE_OUT_ULTRASOUND)) {
        std::string truncatedBeName = backEndName;
        // remove "-VIRT-x" which length is 7
        truncatedBeName.erase(truncatedBeName.end() - 7, truncatedBeName.end());
        ctl = SessionAlsaUtils::getBeMixerControl(mixerHandle, truncatedBeName , BE_GROUP_ATTR);
        if (!ctl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", truncatedBeName.c_str(),
                beCtrlNames[BE_GROUP_ATTR]);
        return -EINVAL;
        }
        if (rmHandle->activeGroupDevConfig->grp_dev_hwep_cfg.sample_rate)
            aif_group_atrr_config[0] = rmHandle->activeGroupDevConfig->grp_dev_hwep_cfg.sample_rate;
        else
            aif_group_atrr_config[0] = dAttr->config.sample_rate;
        if (rmHandle->activeGroupDevConfig->grp_dev_hwep_cfg.channels)
            aif_group_atrr_config[1] = rmHandle->activeGroupDevConfig->grp_dev_hwep_cfg.channels;
        else
            aif_group_atrr_config[1] = dAttr->config.ch_info.channels;
        aif_group_atrr_config[2] = palToSndDriverFormat(
                                    rmHandle->activeGroupDevConfig->grp_dev_hwep_cfg.aud_fmt_id);
        aif_group_atrr_config[3] = AGM_DATA_FORMAT_FIXED_POINT;
        aif_group_atrr_config[4] = rmHandle->activeGroupDevConfig->grp_dev_hwep_cfg.slot_mask;

        mixer_ctl_set_array(ctl, &aif_group_atrr_config,
                               sizeof(aif_group_atrr_config)/sizeof(aif_group_atrr_config[0]));
        PAL_INFO(LOG_TAG, "%s rate ch fmt data_fmt slot_mask %ld %ld %ld %ld %ld\n", truncatedBeName.c_str(),
                aif_group_atrr_config[0], aif_group_atrr_config[1], aif_group_atrr_config[2],
                aif_group_atrr_config[3], aif_group_atrr_config[4]);
        memcpy(&rmHandle->currentGroupDevConfig, rmHandle->activeGroupDevConfig.get(),
               sizeof(group_dev_config_t));
        rmHandle->currentGroupDevConfig.grp_dev_hwep_cfg.sample_rate = aif_group_atrr_config[0];
        rmHandle->currentGroupDevConfig.grp_dev_hwep_cfg.channels = aif_group_atrr_config[1];
    }
    ctl = SessionAlsaUtils::getBeMixerControl(mixerHandle, backEndName , BE_MEDIAFMT);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", backEndName.c_str(),
                beCtrlNames[BE_MEDIAFMT]);
        return -EINVAL;
    }

    PAL_INFO(LOG_TAG, "%s rate ch fmt data_fmt %ld %ld %ld %ld\n", backEndName.c_str(),
                     aif_media_config[0], aif_media_config[1],
                     aif_media_config[2], aif_media_config[3]);

    return mixer_ctl_set_array(ctl, &aif_media_config,
                               sizeof(aif_media_config)/sizeof(aif_media_config[0]));
}

int SessionAlsaUtils::getTimestamp(struct mixer *mixer, const std::vector<int> &DevIds,
                                   uint32_t spr_miid, struct pal_session_time *stime)
{
    int status = 0;
    const char *getParamControl = "getParam";
    char *pcmDeviceName = NULL;
    std::ostringstream CntrlName;
    struct mixer_ctl *ctl;
    struct param_id_spr_session_time_t *spr_session_time;
    std::shared_ptr<std::vector<uint8_t>> payload = nullptr;
    size_t payloadSize = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    if (DevIds.size() > 0) {
        pcmDeviceName = rm->getDeviceNameFromID(DevIds.at(0));
    } else {
        PAL_ERR(LOG_TAG, "DevIds size is invalid");
        return -EINVAL;
    }

    if(!pcmDeviceName){
        PAL_ERR(LOG_TAG, "Device name from id not found");
        return -EINVAL;
    }
    CntrlName<<pcmDeviceName<<" "<<getParamControl;
    ctl = mixer_get_ctl_by_name(mixer, CntrlName.str().data());
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", CntrlName.str().data());
        return -ENOENT;
    }

    PayloadBuilder* builder = new PayloadBuilder();
    builder->payloadTimestamp(payload, &payloadSize, spr_miid);
    if (!payload) {
        PAL_ERR(LOG_TAG, "Timestamp payload formation failed");
        status = -EINVAL;
        goto exit;
    }
    status = mixer_ctl_set_array(ctl, payload->data(), payloadSize);
    if (0 != status) {
         PAL_ERR(LOG_TAG, "Set failed status = %d", status);
         goto exit;
    }

    if (payload && payloadSize <= MAX_UTIL_PAYLOAD_SIZE) {
        memset(payload->data(), 0, payloadSize);
        status = mixer_ctl_get_array(ctl, payload->data(), payloadSize);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "Get failed status = %d", status);
            payload.reset();
            goto exit;
        }
    } else {
        if (!payload) {
            PAL_ERR(LOG_TAG, "Failed to allocate payload memory\n");
            status = -ENOMEM;
            goto exit;
        } else {
            PAL_ERR(LOG_TAG, "Payloadsize exceeds max permissible value");
            payload.reset();
            status = -EINVAL;
            goto exit;
        }
    }

    spr_session_time = (struct param_id_spr_session_time_t *)
                     (payload->data() + sizeof(struct apm_module_param_data_t));
    stime->session_time.value_lsw = spr_session_time->session_time.value_lsw;
    stime->session_time.value_msw = spr_session_time->session_time.value_msw;
    stime->absolute_time.value_lsw = spr_session_time->absolute_time.value_lsw;
    stime->absolute_time.value_msw = spr_session_time->absolute_time.value_msw;
    stime->timestamp.value_lsw = spr_session_time->timestamp.value_lsw;
    stime->timestamp.value_msw = spr_session_time->timestamp.value_msw;
    //flags from Spf are igonred
exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int SessionAlsaUtils::getModuleInstanceId(struct mixer *mixer, int device, const char *intf_name,
                       int tag_id, uint32_t *miid)
{
    char *pcmDeviceName = NULL;
    char const *control = "getTaggedInfo";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0, i;
    void *payload;
    struct gsl_tag_module_info *tag_info;
    struct gsl_tag_module_info_entry *tag_entry;
    int offset = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);
    if(!pcmDeviceName){
        PAL_ERR(LOG_TAG, "Device name from id %d not found", device);
        return -EINVAL;
    }

    ret = setStreamMetadataType(mixer, device, intf_name);
    if (ret)
        return ret;

    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    payload = calloc(1024, sizeof(char));
    if (!payload) {
        free(mixer_str);
        return -ENOMEM;
    }

    ret = mixer_ctl_get_array(ctl, payload, 1024);
    if (ret < 0) {
        PAL_ERR(LOG_TAG, "Failed to mixer_ctl_get_array\n");
        free(payload);
        free(mixer_str);
        return ret;
    }
    tag_info = (struct gsl_tag_module_info *)payload;
    PAL_DBG(LOG_TAG, "num of tags associated with stream %d is %d\n", device, tag_info->num_tags);
    ret = -1;
    tag_entry = (struct gsl_tag_module_info_entry *)(&tag_info->tag_module_entry[0]);
    offset = 0;
    for (i = 0; i < tag_info->num_tags; i++) {
        tag_entry += offset/sizeof(struct gsl_tag_module_info_entry);

        PAL_DBG(LOG_TAG, "tag id[%d] = 0x%x, num_modules = 0x%x\n", i, tag_entry->tag_id, tag_entry->num_modules);
        offset = sizeof(struct gsl_tag_module_info_entry) + (tag_entry->num_modules * sizeof(struct gsl_module_id_info_entry));
        if (tag_entry->tag_id == tag_id) {
            struct gsl_module_id_info_entry *mod_info_entry;

            if (tag_entry->num_modules) {
                 mod_info_entry = &tag_entry->module_entry[0];
                 *miid = mod_info_entry->module_iid;
                 PAL_DBG(LOG_TAG, "MIID is 0x%x\n", *miid);
                 ret = 0;
                 break;
            }
        }
    }

    if (*miid == 0) {
         ret = -EINVAL;
         PAL_ERR(LOG_TAG, "No matching MIID found for tag: 0x%x, error:%d", tag_id, ret);
    }

    free(payload);
    free(mixer_str);
    return ret;
}

int SessionAlsaUtils::getTagsWithModuleInfo(struct mixer *mixer, int device, const char *intf_name,
                                            uint8_t *payload)
{
    char *pcmDeviceName = NULL;
    char const *control = "getTaggedInfo";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0, ret = 0;
    void *payload_;

    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);

    ret = setStreamMetadataType(mixer, device, intf_name);
    if (ret)
        return ret;

    if (!pcmDeviceName) {
        PAL_ERR(LOG_TAG, "pcmDeviceName not initialized");
        return -EINVAL;
    }

    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    payload_ = calloc(1024, sizeof(char));
    if (!payload_) {
        free(mixer_str);
        return -ENOMEM;
    }

    ret = mixer_ctl_get_array(ctl, payload_, 1024);
    if (ret < 0) {
        PAL_ERR(LOG_TAG, "Failed to mixer_ctl_get_array\n");
        free(payload_);
        free(mixer_str);
        return ret;
    }
    memcpy(payload, (uint8_t *)payload_, 1024);

    free(payload_);
    free(mixer_str);
    return ret;
}

int SessionAlsaUtils::setMixerParameter(struct mixer *mixer, int device,
                                        void *payload, int size)
{
    char *pcmDeviceName = NULL;
    char const *control = "setParam";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);
    if(!pcmDeviceName){
        PAL_ERR(LOG_TAG, "Device name from id %d not found", device);
        return -EINVAL;
    }

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", pcmDeviceName);
    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str) {
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }
    ret = mixer_ctl_set_array(ctl, payload, size);

    PAL_DBG(LOG_TAG, "ret = %d, cnt = %d\n", ret, size);
    free(mixer_str);
    return ret;
}

int SessionAlsaUtils::setStreamMetadataType(struct mixer *mixer, int device, const char *val)
{
    char *pcmDeviceName = NULL;
    char const *control = "control";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);
    if(!pcmDeviceName){
        PAL_ERR(LOG_TAG, "Device name from id %d not found", device);
        return -EINVAL;
    }
    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if(mixer_str == NULL) {
        ret = -ENOMEM;
        PAL_ERR(LOG_TAG, "calloc failed");
        return ret;
    }
    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);
    PAL_DBG(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_enum_by_string(ctl, val);
    free(mixer_str);
    return ret;
}

int SessionAlsaUtils::registerMixerEvent(struct mixer *mixer, int device, const char *intf_name, int tag_id, void *payload, int payload_size)
{
    struct agm_event_reg_cfg *event_cfg;
    int status = 0;
    uint32_t miid;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    // get module instance id
    status = SessionAlsaUtils::getModuleInstanceId(mixer, device, intf_name, tag_id, &miid);
    if (status) {
        PAL_ERR(LOG_TAG, "Failed to get tage info %x, status = %d", tag_id, status);
        return EINVAL;
    }

    event_cfg = (struct agm_event_reg_cfg *)payload;
    event_cfg->module_instance_id = miid;

    status = registerMixerEvent(mixer, device, (void *)payload, payload_size);
    return status;
}

int SessionAlsaUtils::registerMixerEvent(struct mixer *mixer, int device, void *payload, int payload_size)
{
    char *pcmDeviceName = NULL;
    char const *control = "event";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,status = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);
    if (!pcmDeviceName)
        return -EINVAL;

    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str)
        return -ENOMEM;

    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    status = mixer_ctl_set_array(ctl, (struct agm_event_reg_cfg *)payload,
                        payload_size);
    free(mixer_str);
    return status;
}

int SessionAlsaUtils::setECRefPath(struct mixer *mixer, int device, const char *intf_name)
{
    char *pcmDeviceName = NULL;
    char const *control = "echoReference";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0;
    int ret = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);
    if(!pcmDeviceName){
        PAL_ERR(LOG_TAG, "Device name from id %d not found", device);
        return -EINVAL;
    }

    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if(mixer_str == NULL) {
        ret = -ENOMEM;
        PAL_ERR(LOG_TAG, "calloc failed");
        return ret;
    }
    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);

    printf("%s mixer -%s-\n", __func__, mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        printf("Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }

    ret = mixer_ctl_set_enum_by_string(ctl, intf_name);
    free(mixer_str);
    return ret;
}

int SessionAlsaUtils::mixerWriteDatapathParams(struct mixer *mixer, int device,
                                        void *payload, int size)
{
    char *pcmDeviceName = NULL;
    char const *control = "datapathParams";
    char *mixer_str;
    struct mixer_ctl *ctl;
    int ctl_len = 0,ret = 0;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    pcmDeviceName = rm->getDeviceNameFromID(device);
    if(!pcmDeviceName){
        PAL_ERR(LOG_TAG, "Device name from id %d not found", device);
        return -EINVAL;
    }

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", pcmDeviceName);
    ctl_len = strlen(pcmDeviceName) + 1 + strlen(control) + 1;
    mixer_str = (char *)calloc(1, ctl_len);
    if (!mixer_str) {
        return -ENOMEM;
    }
    snprintf(mixer_str, ctl_len, "%s %s", pcmDeviceName, control);

    PAL_DBG(LOG_TAG, "- mixer -%s-\n", mixer_str);
    ctl = mixer_get_ctl_by_name(mixer, mixer_str);
    if (!ctl) {
        PAL_ERR(LOG_TAG, "Invalid mixer control: %s\n", mixer_str);
        free(mixer_str);
        return ENOENT;
    }
    PAL_DBG(LOG_TAG, "payload = %p\n", payload);
    ret = mixer_ctl_set_array(ctl, payload, size);

    PAL_DBG(LOG_TAG, "ret = %d, cnt = %d\n", ret, size);
    free(mixer_str);
    return ret;
}

int SessionAlsaUtils::open(Stream * streamHandle, std::shared_ptr<ResourceManager> rmHandle,
    const std::vector<int> &RxDevIds, const std::vector<int> &TxDevIds,
    const std::vector<std::pair<int32_t, std::string>> &rxBackEnds,
    const std::vector<std::pair<int32_t, std::string>> &txBackEnds)
{
    std::vector <std::pair<int, int>> streamRxKV, streamTxKV;
    std::vector <std::pair<int, int>> streamRxCKV, streamTxCKV;
    std::vector <std::pair<int, int>> streamDeviceRxKV, streamDeviceTxKV;
    std::vector <std::pair<int, int>> deviceRxKV, deviceTxKV;
    std::vector <std::pair<int, int>> deviceCKV;
    // Using as empty key vector pairs
    std::vector <std::pair<int, int>> emptyKV;
    int status = 0;
    struct pal_stream_attributes sAttr = {};
    struct agmMetaData streamRxMetaData(nullptr, 0);
    struct agmMetaData streamTxMetaData(nullptr, 0);
    struct agmMetaData deviceRxMetaData(nullptr, 0);
    struct agmMetaData deviceTxMetaData(nullptr, 0);
    struct agmMetaData streamDeviceRxMetaData(nullptr, 0);
    struct agmMetaData streamDeviceTxMetaData(nullptr, 0);
    std::ostringstream rxFeName, txFeName;
    struct mixer_ctl *rxFeMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *rxBeMixerCtrl = nullptr;
    struct mixer_ctl *txFeMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *txBeMixerCtrl = nullptr;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct mixer *mixerHandle;
    uint32_t streamPropId[] = {0x08000010, 1, 0x1}; /** gsl_subgraph_platform_driver_props.xml */
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    uint32_t streamDevicePropId[] = {0x08000010, 1, 0x3}; /** gsl_subgraph_platform_driver_props.xml */
    uint32_t i, rxDevNum, txDevNum;
    struct pal_device_info devinfo = {};
    struct vsid_info vsidinfo = {};
    sidetone_mode_t sidetoneMode = SIDETONE_OFF;
    struct pal_device dAttr = {};
    bool isDeviceFound = false;

    if (RxDevIds.empty() || TxDevIds.empty()) {
        PAL_ERR(LOG_TAG, "RX and TX FE Dev Ids are empty");
        return -EINVAL;
    }
    status = streamHandle->getStreamAttributes(&sAttr);
    if(0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        return status;
    }

    status = streamHandle->getAssociatedDevices(associatedDevices);
    if(0 != status) {
        PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
        return status;
    }

    PayloadBuilder* builder = new PayloadBuilder();

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    // get keyvalue pair info
    for (i = 0; i < associatedDevices.size(); i++) {
        associatedDevices[i]->getDeviceAttributes(&dAttr, streamHandle);
        if (txBackEnds[0].first == dAttr.id) {
            isDeviceFound = true;
            break;
        }
    }
    if (isDeviceFound) {
        rmHandle->getDeviceInfo((pal_device_id_t)txBackEnds[0].first, sAttr.type,
                dAttr.custom_config.custom_key, &devinfo);
    }
    else {
        PAL_ERR(LOG_TAG, "could not find associated device kv cannot be set");
    }

    if(sAttr.type == PAL_STREAM_VOICE_CALL){
        //get vsid info
        status = rmHandle->getVsidInfo(&vsidinfo);
        if(status) {
            PAL_ERR(LOG_TAG, "get vsid info failed");
        }

        status = rmHandle->getSidetoneMode((pal_device_id_t)txBackEnds[0].first, sAttr.type, &sidetoneMode);
        if(status) {
            PAL_ERR(LOG_TAG, "get sidetone mode failed");
        }
    }
    // get streamKV
    if ((status = builder->populateStreamKV(streamHandle, streamRxKV,
                    streamTxKV, vsidinfo)) != 0) {
        PAL_ERR(LOG_TAG, "get stream KV for Rx/Tx failed %d", status);
        goto exit;
    }
    // get streamKV
    if ((status = builder->populateStreamPPKV(streamHandle, streamRxKV,
                    streamTxKV)) != 0) {
        PAL_ERR(LOG_TAG, "get streamPP KV for Rx/Tx failed %d", status);
        goto exit;
    }
    // get streamCKV
    if ((sAttr.type != PAL_STREAM_VOICE_CALL) && (sAttr.type != PAL_STREAM_ULTRASOUND)) {
        status = builder->populateStreamCkv(streamHandle, streamRxCKV, 0,
            (struct pal_volume_data **)nullptr);
        if (status) {
            PAL_ERR(LOG_TAG, "get stream ckv failed %d", status);
            goto exit;
        }
    }
    // get deviceKV
    if ((status = builder->populateDeviceKV(streamHandle, rxBackEnds[0].first,
                    deviceRxKV, txBackEnds[0].first, deviceTxKV, sidetoneMode)) != 0) {
        PAL_ERR(LOG_TAG, "get device KV for Rx/Tx failed %d", status);
        goto exit;
    }
     // get devicePP
    if ((status = builder->populateDevicePPKV(streamHandle,
                    rxBackEnds[0].first, streamDeviceRxKV, txBackEnds[0].first,
                    streamDeviceTxKV))!= 0) {
        PAL_ERR(LOG_TAG, "get device KV failed %d", status);
        goto exit;
    }
    // get streamdeviceKV
    status = builder->populateStreamDeviceKV(streamHandle, rxBackEnds[0].first,
            streamDeviceRxKV, txBackEnds[0].first, streamDeviceTxKV, vsidinfo,
                                             sidetoneMode);
    if (status) {
        PAL_VERBOSE(LOG_TAG, "get stream device KV for Rx/Tx failed %d", status);
        status = 0; /**< ignore stream device KV failures */
    }

    // get audio mixer
    if ((streamRxKV.size() > 0) || (streamRxCKV.size() > 0)) {
        SessionAlsaUtils::getAgmMetaData(streamRxKV, streamRxCKV,
                (struct prop_data *)streamPropId, streamRxMetaData);
        if (!streamRxMetaData.size) {
            PAL_ERR(LOG_TAG, "stream RX metadata is zero");
            status = -ENOMEM;
            goto freeRxMetaData;
        }
    }
    deviceCKV.clear();
    if (ResourceManager::isSpeakerProtectionEnabled) {
        PAL_DBG(LOG_TAG, "Speaker protection enabled");
        if (rxBackEnds[0].first == PAL_DEVICE_OUT_SPEAKER) {
            status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                            SPKR_PROT_ENABLE);
            if (status != 0) {
                PAL_VERBOSE(LOG_TAG, "Unable to populate SP cal");
                status = 0; /**< ignore device SP CKV failures */
            }
        }
    }
    if (ResourceManager::isHandsetProtectionEnabled &&
            ResourceManager::isSpeakerProtectionEnabled) {
        PAL_DBG(LOG_TAG, "Handset enabled");
        if (rxBackEnds[0].first == PAL_DEVICE_OUT_HANDSET) {
            status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                          HANDSET_PROT_ENABLE);
         if (status != 0) {
             PAL_VERBOSE(LOG_TAG, "Unable to populate SP cal");
             status = 0; /**< ignore device SP CKV failures */
         }
      }
    }

    if (rxBackEnds[0].first == PAL_DEVICE_OUT_HAPTICS_DEVICE &&
                             ResourceManager::isHapticsthroughWSA) {
        status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                       HAPTICS_PROT_ENABLE);
        if (status != 0) {
            PAL_VERBOSE(LOG_TAG, "Unable to populate HP cal");
            status = 0; /**< ignore device HP CKV failures */
        }
    }

    if (deviceRxKV.size() > 0) {
        SessionAlsaUtils::getAgmMetaData(deviceRxKV, deviceCKV,
                (struct prop_data *)devicePropId, deviceRxMetaData);
        if (!deviceRxMetaData.size) {
            PAL_ERR(LOG_TAG, "device RX metadata is zero");
            status = -ENOMEM;
            goto freeRxMetaData;
        }
    }
    if (streamDeviceRxKV.size() > 0) {
        SessionAlsaUtils::getAgmMetaData(streamDeviceRxKV, emptyKV,
                (struct prop_data *)streamDevicePropId, streamDeviceRxMetaData);
        if (!streamDeviceRxMetaData.size) {
            PAL_ERR(LOG_TAG, "stream/device RX metadata is zero");
            status = -ENOMEM;
            goto freeRxMetaData;
        }
    }

    if ((streamTxKV.size() > 0) || (streamTxCKV.size() > 0)) {
        SessionAlsaUtils::getAgmMetaData(streamTxKV, streamTxCKV,
                (struct prop_data *)streamPropId, streamTxMetaData);
        if (!streamTxMetaData.size) {
            PAL_ERR(LOG_TAG, "stream TX metadata is zero");
            status = -ENOMEM;
            goto freeTxMetaData;
        }
    }
    if (deviceTxKV.size() > 0) {
        SessionAlsaUtils::getAgmMetaData(deviceTxKV, emptyKV,
                (struct prop_data *)devicePropId, deviceTxMetaData);
        if (!deviceTxMetaData.size) {
            PAL_ERR(LOG_TAG, "device TX metadata is zero");
            status = -ENOMEM;
            goto freeTxMetaData;
        }
    }

    if (streamDeviceTxKV.size() > 0) {
        SessionAlsaUtils::getAgmMetaData(streamDeviceTxKV, emptyKV,
                (struct prop_data *)streamDevicePropId, streamDeviceTxMetaData);
        if (!streamDeviceTxMetaData.size) {
            PAL_ERR(LOG_TAG, "stream/device TX metadata is zero");
            status = -ENOMEM;
            goto freeTxMetaData;
        }
    }

    if (sAttr.type == PAL_STREAM_VOICE_CALL) {
        if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
            sAttr.info.voice_call_info.VSID == VOICELBMMODE1){
            rxFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 1 << "p";
            txFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 1 << "c";
        } else {
            rxFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 2 << "p";
            txFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 2 << "c";
        }

    } else {
        rxFeName << PCM_SND_DEV_NAME_PREFIX << RxDevIds.at(0);
        txFeName << PCM_SND_DEV_NAME_PREFIX << TxDevIds.at(0);
    }

    for (i = FE_CONTROL; i <= FE_CONNECT; ++i) {
        rxFeMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle, rxFeName.str(), i);
        txFeMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle, txFeName.str(), i);
        if (!rxFeMixerCtrls[i] || !txFeMixerCtrls[i]) {
            PAL_ERR(LOG_TAG, "invalid mixer control: (%s%s)/(%s%s)",
                    rxFeName.str().data(), feCtrlNames[i],
                    txFeName.str().data(), feCtrlNames[i]);
            status = -EINVAL;
            goto freeTxMetaData;
        }
    }

    rxBeMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle,
            rxBackEnds[0].second, BE_METADATA);
    txBeMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle, txBackEnds[0].second, BE_METADATA);
    if (!rxBeMixerCtrl || !txBeMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: (%s%s)/(%s%s)",
                rxBackEnds[0].second.data(), beCtrlNames[BE_METADATA],
                txBackEnds[0].second.data(), beCtrlNames[BE_METADATA]);
        status = -EINVAL;
        goto freeTxMetaData;
    }

    if (SessionAlsaUtils::isRxDevice(associatedDevices[0]->getSndDeviceId()))
        rxDevNum = 0;
    else
        rxDevNum = 1;

    txDevNum = !rxDevNum;

    /** set TX mixer controls */
    mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_CONTROL], "ZERO");
    if (streamTxMetaData.size)
        mixer_ctl_set_array(txFeMixerCtrls[FE_METADATA], (void *)streamTxMetaData.buf,
                streamTxMetaData.size);
    if (deviceTxMetaData.size)
        mixer_ctl_set_array(txBeMixerCtrl, (void *)deviceTxMetaData.buf,
                deviceTxMetaData.size);
    if (streamDeviceTxMetaData.size) {
        mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_CONTROL], txBackEnds[0].second.data());
        mixer_ctl_set_array(txFeMixerCtrls[FE_METADATA], (void *)streamDeviceTxMetaData.buf,
                streamDeviceTxMetaData.size);
    }
    mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_CONNECT], txBackEnds[0].second.data());

    /** set RX mixer controls */
    mixer_ctl_set_enum_by_string(rxFeMixerCtrls[FE_CONTROL], "ZERO");
    if (streamRxMetaData.size)
        mixer_ctl_set_array(rxFeMixerCtrls[FE_METADATA], (void *)streamRxMetaData.buf,
                streamRxMetaData.size);
    if (deviceRxMetaData.size)
        mixer_ctl_set_array(rxBeMixerCtrl, (void *)deviceRxMetaData.buf,
                deviceRxMetaData.size);
    if (streamDeviceRxMetaData.size) {
        mixer_ctl_set_enum_by_string(rxFeMixerCtrls[FE_CONTROL], rxBackEnds[0].second.data());
        mixer_ctl_set_array(rxFeMixerCtrls[FE_METADATA], (void *)streamDeviceRxMetaData.buf,
                streamDeviceRxMetaData.size);
    }
    mixer_ctl_set_enum_by_string(rxFeMixerCtrls[FE_CONNECT], rxBackEnds[0].second.data());

    if (sAttr.type != PAL_STREAM_VOICE_CALL) {
        txFeMixerCtrls[FE_LOOPBACK] = getFeMixerControl(mixerHandle, txFeName.str(), FE_LOOPBACK);
        if (!txFeMixerCtrls[FE_LOOPBACK]) {
            PAL_ERR(LOG_TAG, "invalid mixer control %s%s",
                    txFeName.str().data(), feCtrlNames[i]);
            status = -EINVAL;
            goto freeTxMetaData;
        }
        mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_LOOPBACK], rxFeName.str().data());
    }
freeTxMetaData:
    free(streamDeviceTxMetaData.buf);
    free(deviceTxMetaData.buf);
    free(streamTxMetaData.buf);
freeRxMetaData:
    free(streamDeviceRxMetaData.buf);
    free(deviceRxMetaData.buf);
    free(streamRxMetaData.buf);
exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int SessionAlsaUtils::openDev(std::shared_ptr<ResourceManager> rmHandle,
    const std::vector<int> &DevIds, int32_t backEndId, std::string backEndName)
{
    std::vector <std::pair<int, int>> deviceKV;
    std::vector <std::pair<int, int>> emptyKV;
    int status = 0;
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct agmMetaData streamDeviceMetaData(nullptr, 0);
    std::ostringstream feName;
    struct mixer_ctl *feMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *beMetaDataMixerCtrl = nullptr;
    struct mixer *mixerHandle;
    uint32_t i;
    /** gsl_subgraph_platform_driver_props.xml */
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    struct pal_device_info devinfo = {};

    PayloadBuilder* builder = new PayloadBuilder();

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (0 != status) {
        PAL_ERR(LOG_TAG, "getVirtualAudioMixer failed");
        goto freeMetaData;
    }

    PAL_DBG(LOG_TAG, "Ext EC Ref Open Dev is called");

    if( DevIds.size() == 0)
    {
         PAL_ERR(LOG_TAG, "No Device id provided");
         status = -EINVAL;
         goto freeMetaData;
    }
    /** Get mixer controls (struct mixer_ctl *) for both FE and BE */
    feName << "ExtEC" << DevIds.at(0);
    for (i = FE_CONTROL; i <= FE_CONNECT; ++i) {
        feMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle, feName.str(), i);
        if (!feMixerCtrls[i]) {
            PAL_ERR(LOG_TAG, "invalid mixer control: %s%s", feName.str().data(),
                   feCtrlNames[i]);
            status = -EINVAL;
            goto freeMetaData;
        }
    }
    mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONTROL], "ZERO");

    if ((status = builder->populateDeviceKV(NULL, backEndId, deviceKV)) != 0) {
        PAL_ERR(LOG_TAG, "get device KV failed %d", status);
        goto freeMetaData;
    }

    if (deviceKV.size() > 0) {
        getAgmMetaData(deviceKV, emptyKV, (struct prop_data *)devicePropId,
                deviceMetaData);
        if (!deviceMetaData.size) {
            PAL_ERR(LOG_TAG, "device metadata is zero");
            status = -ENOMEM;
            goto freeMetaData;
        }
    }

    beMetaDataMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle, backEndName, BE_METADATA);
    if (!beMetaDataMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", backEndName.data(),
                beCtrlNames[BE_METADATA]);
        status = -EINVAL;
        goto freeMetaData;
    }

    /** set mixer controls */
    if (deviceMetaData.size)
        mixer_ctl_set_array(beMetaDataMixerCtrl, (void *)deviceMetaData.buf,
                deviceMetaData.size);
    mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONNECT], backEndName.data());
    deviceKV.clear();
    free(deviceMetaData.buf);
    deviceMetaData.buf = nullptr;

freeMetaData:
    if (deviceMetaData.buf)
        free(deviceMetaData.buf);

    delete builder;
    return status;
}



int SessionAlsaUtils::close(Stream * streamHandle, std::shared_ptr<ResourceManager> rmHandle,
    const std::vector<int> &RxDevIds, const std::vector<int> &TxDevIds,
    const std::vector<std::pair<int32_t, std::string>> &rxBackEnds,
    const std::vector<std::pair<int32_t, std::string>> &txBackEnds,
    std::vector<std::pair<std::string, int>> &freeDeviceMetaData)
{
    int status = 0;
    std::vector <std::pair<int, int>> emptyKV;
    struct pal_stream_attributes sAttr = {};
    struct agmMetaData streamRxMetaData(nullptr, 0);
    struct agmMetaData streamTxMetaData(nullptr, 0);
    struct agmMetaData deviceRxMetaData(nullptr, 0);
    struct agmMetaData deviceTxMetaData(nullptr, 0);
    struct agmMetaData streamDeviceRxMetaData(nullptr, 0);
    struct agmMetaData streamDeviceTxMetaData(nullptr, 0);
    std::ostringstream rxFeName, txFeName;
    struct mixer_ctl *rxFeMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *rxBeMixerCtrl = nullptr;
    struct mixer_ctl *txFeMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl *txBeMixerCtrl = nullptr;
    std::vector<std::shared_ptr<Device>> associatedDevices;
    struct mixer *mixerHandle;
    uint32_t streamPropId[] = {0x08000010, 1, 0x1}; /** gsl_subgraph_platform_driver_props.xml */
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    uint32_t streamDevicePropId[] = {0x08000010, 1, 0x3}; /** gsl_subgraph_platform_driver_props.xml */
    uint32_t i, rxDevNum, txDevNum;

    status = streamHandle->getStreamAttributes(&sAttr);
    if(0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }
    if (sAttr.type != PAL_STREAM_VOICE_CALL_RECORD && sAttr.type != PAL_STREAM_VOICE_CALL_MUSIC) {
        status = streamHandle->getAssociatedDevices(associatedDevices);
        if (0 != status) {
            PAL_ERR(LOG_TAG, "getAssociatedDevices Failed \n");
            goto exit;
        }
        if (associatedDevices.size() != 2) {
            PAL_ERR(LOG_TAG, "Loopback num devices expected 2, given:%zu",
                    associatedDevices.size());
            goto exit;
        }
    }
    status = rmHandle->getVirtualAudioMixer(&mixerHandle);

    // get audio mixer
    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
            (struct prop_data *)streamPropId, streamRxMetaData);
    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
            (struct prop_data *)devicePropId, deviceRxMetaData);
    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
            (struct prop_data *)streamDevicePropId, streamDeviceRxMetaData);
    if (!streamRxMetaData.size || !deviceRxMetaData.size ||
            !streamDeviceRxMetaData.size) {
        PAL_ERR(LOG_TAG, "stream/device RX metadata is zero");
        status = -ENOMEM;
        goto freeRxMetaData;
    }

    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
            (struct prop_data *)streamPropId, streamTxMetaData);
    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
            (struct prop_data *)devicePropId, deviceTxMetaData);
    SessionAlsaUtils::getAgmMetaData(emptyKV, emptyKV,
            (struct prop_data *)streamDevicePropId, streamDeviceTxMetaData);
    if (!streamTxMetaData.size || !deviceTxMetaData.size ||
            !streamDeviceTxMetaData.size) {
        PAL_ERR(LOG_TAG, "stream/device TX metadata is zero");
        status = -ENOMEM;
        goto freeTxMetaData;
    }

    if (sAttr.type == PAL_STREAM_VOICE_CALL) {
        if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
            sAttr.info.voice_call_info.VSID == VOICELBMMODE1){
            rxFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 1 << "p";
            txFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 1 << "c";
        } else {
            rxFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 2 << "p";
            txFeName << PCM_SND_VOICE_DEV_NAME_PREFIX  << 2 << "c";
        }

    } else {
        rxFeName << PCM_SND_DEV_NAME_PREFIX << RxDevIds.at(0);
        txFeName << PCM_SND_DEV_NAME_PREFIX << TxDevIds.at(0);
    }

    for (i = FE_CONTROL; i <= FE_DISCONNECT; ++i) {
        rxFeMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle, rxFeName.str(), i);
        txFeMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle, txFeName.str(), i);
        if (!rxFeMixerCtrls[i] || !txFeMixerCtrls[i]) {
            PAL_ERR(LOG_TAG, "invalid mixer control: (%s%s)/(%s%s)",
                    rxFeName.str().data(), feCtrlNames[i],
                    txFeName.str().data(), feCtrlNames[i]);
            status = -EINVAL;
            goto freeTxMetaData;
        }
    }

    rxBeMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle,
            rxBackEnds[0].second, BE_METADATA);
    txBeMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle, txBackEnds[0].second, BE_METADATA);
    if (!rxBeMixerCtrl || !txBeMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: (%s%s)/(%s%s)",
                rxBackEnds[0].second.data(), beCtrlNames[BE_METADATA],
                txBackEnds[0].second.data(), beCtrlNames[BE_METADATA]);
        status = -EINVAL;
        goto freeTxMetaData;
    }

    if (SessionAlsaUtils::isRxDevice(associatedDevices[0]->getSndDeviceId()))
        rxDevNum = 0;
    else
        rxDevNum = 1;

    txDevNum = !rxDevNum;

    if (sAttr.type != PAL_STREAM_VOICE_CALL) {
        txFeMixerCtrls[FE_LOOPBACK] = getFeMixerControl(mixerHandle, txFeName.str(), FE_LOOPBACK);
        if (!txFeMixerCtrls[FE_LOOPBACK]) {
            PAL_ERR(LOG_TAG, "invalid mixer control %s%s",
                    txFeName.str().data(), feCtrlNames[i]);
            status = -EINVAL;
            goto freeTxMetaData;
        }
        mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_LOOPBACK], "ZERO");
    }

    /** set TX mixer controls */
    mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_DISCONNECT], txBackEnds[0].second.data());
    mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_CONTROL], "ZERO");
    mixer_ctl_set_array(txFeMixerCtrls[FE_METADATA], (void *)streamTxMetaData.buf,
            streamTxMetaData.size);
    mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_CONTROL], txBackEnds[0].second.data());
    mixer_ctl_set_array(txFeMixerCtrls[FE_METADATA], (void *)streamDeviceTxMetaData.buf,
            streamDeviceTxMetaData.size);

    /** set RX mixer controls */
    mixer_ctl_set_enum_by_string(rxFeMixerCtrls[FE_DISCONNECT], rxBackEnds[0].second.data());
    mixer_ctl_set_enum_by_string(rxFeMixerCtrls[FE_CONTROL], "ZERO");
    mixer_ctl_set_array(rxFeMixerCtrls[FE_METADATA], (void *)streamRxMetaData.buf,
            streamRxMetaData.size);
    mixer_ctl_set_enum_by_string(rxFeMixerCtrls[FE_CONTROL], rxBackEnds[0].second.data());
    mixer_ctl_set_array(rxFeMixerCtrls[FE_METADATA], (void *)streamDeviceRxMetaData.buf,
            streamDeviceRxMetaData.size);

    /* set Backend mixer control */
    for (auto freeDevMeta = freeDeviceMetaData.begin(); freeDevMeta != freeDeviceMetaData.end(); ++freeDevMeta) {
        PAL_DBG(LOG_TAG, "backend %s and freedevicemetadata %d", freeDevMeta->first.data(), freeDevMeta->second);
        if (!(freeDevMeta->first.compare(txBackEnds[0].second))) {
            if (freeDevMeta->second == 0) {
                PAL_INFO(LOG_TAG, "No need to free TX device metadata as device is still active");
            } else {
                mixer_ctl_set_array(txBeMixerCtrl, (void *)deviceTxMetaData.buf,
                                    deviceTxMetaData.size);
            }
        }
        if (!(freeDevMeta->first.compare(rxBackEnds[0].second))) {
            if (freeDevMeta->second == 0) {
                PAL_INFO(LOG_TAG, "No need to free RX device metadata as device is still active");
            } else {
                mixer_ctl_set_array(rxBeMixerCtrl, (void *)deviceRxMetaData.buf,
                                    deviceRxMetaData.size);
            }
        }
    }
freeTxMetaData:
    if (streamDeviceTxMetaData.buf)
        free(streamDeviceTxMetaData.buf);
    if (deviceTxMetaData.buf)
        free(deviceTxMetaData.buf);
    if (streamTxMetaData.buf)
        free(streamTxMetaData.buf);
freeRxMetaData:
    if (streamDeviceRxMetaData.buf)
        free(streamDeviceRxMetaData.buf);
    if (deviceRxMetaData.buf)
        free(deviceRxMetaData.buf);
    if (streamRxMetaData.buf)
        free(streamRxMetaData.buf);
exit:
    return status;
}

int SessionAlsaUtils::disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToDisconnect)
{
    std::ostringstream disconnectCtrlName;
    int status = 0;
    struct mixer *mixerHandle = nullptr;
    struct mixer_ctl *disconnectCtrl = nullptr;
    struct pal_stream_attributes sAttr = {};
    std::vector <std::pair<int, int>> emptyKV;
    std::ostringstream feName;
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct agmMetaData streamDeviceMetaData(nullptr, 0);
    uint32_t devicePropId[] = { 0x08000010, 2, 0x2, 0x5 };
    uint32_t streamDevicePropId[] = { 0x08000010, 1, 0x3 };
    struct mixer_ctl* feMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    struct mixer_ctl* beMetaDataMixerCtrl = nullptr;
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();
    std::shared_ptr<Device> dev = nullptr;
    int sub = 1;
    uint32_t i;
    int devCount = 0;

    if (PAL_STREAM_VOICE_CALL == streamType) {
        if (SessionAlsaUtils::isRxDevice(aifBackEndsToDisconnect[0].first)) {
            rmHandle->pauseInCallMusic();
        }
    }

    switch (streamType) {
        case PAL_STREAM_COMPRESSED:
            disconnectCtrlName << COMPRESS_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " disconnect";
            feName << COMPRESS_SND_DEV_NAME_PREFIX << pcmDevIds.at(0);
            break;
        case PAL_STREAM_VOICE_CALL:
            status = streamHandle->getStreamAttributes(&sAttr);
            if (status) {
                PAL_ERR(LOG_TAG, "could not get stream attributes\n");
                return status;
            }
            if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                sAttr.info.voice_call_info.VSID == VOICELBMMODE1)
                sub = 1;
            else
                sub = 2;
            if (dAttr.id > PAL_DEVICE_OUT_MIN && dAttr.id < PAL_DEVICE_OUT_MAX) {
                feName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "p";
                disconnectCtrlName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "p" << " disconnect";
            } else if (dAttr.id > PAL_DEVICE_IN_MIN && dAttr.id < PAL_DEVICE_IN_MAX) {
                feName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "c";
                disconnectCtrlName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "c" << " disconnect";
            }
            break;
        default:
            feName << PCM_SND_DEV_NAME_PREFIX << pcmDevIds.at(0);
            disconnectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " disconnect";
            break;
    }
    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    disconnectCtrl = mixer_get_ctl_by_name(mixerHandle, disconnectCtrlName.str().data());
    if (!disconnectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", disconnectCtrlName.str().data());
        return -EINVAL;
    }
    for (i = FE_CONTROL; i <= FE_DISCONNECT; ++i) {
        feMixerCtrls[i] = SessionAlsaUtils::getFeMixerControl(mixerHandle,
            feName.str(), i);
        if (!feMixerCtrls[i]) {
            PAL_ERR(LOG_TAG, "invalid mixer control: %s %s",
                feName.str().data(), feCtrlNames[i]);
            status = -EINVAL;
            goto exit;
        }
    }

    /** Disconnect FE to BE */
    mixer_ctl_set_enum_by_string(disconnectCtrl, aifBackEndsToDisconnect[0].second.data());

    /** clear device metadata*/
    getAgmMetaData(emptyKV, emptyKV, (struct prop_data*)devicePropId,
        deviceMetaData);
    getAgmMetaData(emptyKV, emptyKV, (struct prop_data*)streamDevicePropId,
        streamDeviceMetaData);
    if (!deviceMetaData.size || !streamDeviceMetaData.size) {
        PAL_ERR(LOG_TAG, "stream/device metadata is zero");
        status = -ENOMEM;
        goto freeMetaData;
    }
    beMetaDataMixerCtrl = SessionAlsaUtils::getBeMixerControl(mixerHandle,
        aifBackEndsToDisconnect[0].second, BE_METADATA);
    if (!beMetaDataMixerCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s %s", aifBackEndsToDisconnect[0].second.data(),
            beCtrlNames[BE_METADATA]);
        status = -EINVAL;
        goto freeMetaData;
    }

    //To check if any active stream present on same backend
    dev = Device::getInstance(&dAttr, rm);
    if (dev == 0) {
        PAL_ERR(LOG_TAG, "device_id[%d] Instance query failed", dAttr.id );
        status = -EINVAL;
        goto freeMetaData;
    }

    devCount = dev->getDeviceCount();

    // Do not clear device metadata for A2DP device if SCO device is active
    if ((devCount == 1) && rm->isBtA2dpDevice(dAttr.id))
        devCount += getScoDevCount();

    if (devCount > 1) {
        PAL_INFO(LOG_TAG, "No need to free device metadata since active streams present on device");
    } else {
        mixer_ctl_set_array(beMetaDataMixerCtrl, (void*)deviceMetaData.buf,
            deviceMetaData.size);
    }

    mixer_ctl_set_enum_by_string(feMixerCtrls[FE_CONTROL],
        aifBackEndsToDisconnect[0].second.data());
    mixer_ctl_set_array(feMixerCtrls[FE_METADATA], (void*)streamDeviceMetaData.buf,
        streamDeviceMetaData.size);

freeMetaData:
    if (streamDeviceMetaData.buf)
        free(streamDeviceMetaData.buf);
    if (deviceMetaData.buf)
        free(deviceMetaData.buf);
exit:
    return status;
}

int SessionAlsaUtils::disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmTxDevIds,const std::vector<int> &pcmRxDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToDisconnect)
{
    std::ostringstream disconnectCtrlName;
    int status = 0;
    struct mixer *mixerHandle = nullptr;
    struct mixer_ctl *disconnectCtrl = nullptr;
    struct mixer_ctl *txFeMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    std::ostringstream txFeName;

    switch (streamType) {
         case PAL_STREAM_ULTRASOUND:
         case PAL_STREAM_LOOPBACK:
             status = rmHandle->getVirtualAudioMixer(&mixerHandle);
             txFeName << PCM_SND_DEV_NAME_PREFIX << pcmTxDevIds.at(0);
             txFeMixerCtrls[FE_LOOPBACK] = getFeMixerControl(mixerHandle, txFeName.str(), FE_LOOPBACK);
             if (!txFeMixerCtrls[FE_LOOPBACK]) {
                 PAL_ERR(LOG_TAG, "invalid mixer control %s",
                         txFeName.str().data());
                 status = -EINVAL;
                 return status;
             }
             mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_LOOPBACK], "ZERO");
             if (dAttr.id > PAL_DEVICE_OUT_MIN && dAttr.id < PAL_DEVICE_OUT_MAX) {
                 disconnectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmRxDevIds.at(0) << " disconnect";
             } else if (dAttr.id > PAL_DEVICE_IN_MIN && dAttr.id < PAL_DEVICE_IN_MAX) {
                 disconnectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmTxDevIds.at(0) << " disconnect";
             }
            break;
        default:
            disconnectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmRxDevIds.at(0) << " disconnect";
            break;
    }
    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    disconnectCtrl = mixer_get_ctl_by_name(mixerHandle, disconnectCtrlName.str().data());
    if (!disconnectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", disconnectCtrlName.str().data());
        return -EINVAL;
    }
    /** Disconnect FE to BE */
    mixer_ctl_set_enum_by_string(disconnectCtrl, aifBackEndsToDisconnect[0].second.data());

    return status;
}

int SessionAlsaUtils::connectSessionDevice(Session* sess, Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToConnect)
{
    struct mixer_ctl *connectCtrl;
    struct mixer *mixerHandle = nullptr;
    bool is_compress = false;
    int status = 0;
    std::ostringstream connectCtrlName;
    struct pal_stream_attributes sAttr = {};
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    int sub = 1;
    uint32_t miid;
    struct sessionToPayloadParam streamData = {};
    PayloadBuilder* builder = new PayloadBuilder();
    std::shared_ptr<ResourceManager> rm = ResourceManager::getInstance();

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "get mixer handle failed %d", status);
        goto exit;
    }
    status = streamHandle->getStreamAttributes(&sAttr);
    if (status) {
        PAL_ERR(LOG_TAG, "could not get stream attributes\n");
        goto exit;
    }
    switch (streamType) {
        case PAL_STREAM_COMPRESSED:
            connectCtrlName << COMPRESS_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " connect";
            is_compress = true;
            break;
        case PAL_STREAM_VOICE_CALL:
            if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                sAttr.info.voice_call_info.VSID == VOICELBMMODE1)
                sub = 1;
            else
                sub = 2;

            if (dAttr.id > PAL_DEVICE_OUT_MIN && dAttr.id < PAL_DEVICE_OUT_MAX) {
                connectCtrlName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "p" << " connect";
            } else if (dAttr.id > PAL_DEVICE_IN_MIN && dAttr.id < PAL_DEVICE_IN_MAX) {
                connectCtrlName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "c" << " connect";
            }
            break;
        default:
            connectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " connect";
            break;
    }

    /*Setup inCall MFC configuration before the speaker device subgraph moves to START state*/
    /*Make sure the speaker protection module can apply the correct media format*/
    if (SessionAlsaUtils::isRxDevice(aifBackEndsToConnect[0].first)) {
        struct sessionToPayloadParam deviceData;
        deviceData.bitWidth = dAttr.config.bit_width;
        deviceData.sampleRate = dAttr.config.sample_rate;
        deviceData.numChannel = dAttr.config.ch_info.channels;
        deviceData.ch_info = nullptr;
        rm->reconfigureInCallMusicStream(deviceData);
    }
    /* Configure MFC to match to device config */
    /* This has to be done after sending all mixer controls and before connect */
    if (PAL_STREAM_VOICE_CALL != streamType) {
        if (sAttr.direction == PAL_AUDIO_OUTPUT) {
            if (sess) {
                sess->configureMFC(rmHandle, sAttr, dAttr, pcmDevIds,
                                    aifBackEndsToConnect[0].second.data());

                if (strcmp(dAttr.custom_config.custom_key, "mspp") &&
                    dAttr.id == PAL_DEVICE_OUT_SPEAKER &&
                    dAttr.config.ch_info.channels == 2 &&
                    ((sAttr.type == PAL_STREAM_LOW_LATENCY) ||
                    (sAttr.type == PAL_STREAM_ULTRA_LOW_LATENCY) ||
                    (sAttr.type == PAL_STREAM_PCM_OFFLOAD) ||
                    (sAttr.type == PAL_STREAM_DEEP_BUFFER) ||
                    (sAttr.type == PAL_STREAM_COMPRESSED))) {
                    pal_param_device_rotation_t rotation;
                    rotation.rotation_type = rm->mOrientation == ORIENTATION_270 ?
                                            PAL_SPEAKER_ROTATION_RL : PAL_SPEAKER_ROTATION_LR;
                    status = sess->setParameters(streamHandle, 0, PAL_PARAM_ID_DEVICE_ROTATION, &rotation);
                    if (status != 0) {
                        PAL_ERR(LOG_TAG,"handleDeviceRotation failed");
                        status = 0; //rotaton setting failed is not fatal.
                        sess->getCustomPayload(&payload, &payloadSize);
                        if (payload) {
                            status = SessionAlsaUtils::setMixerParameter(mixerHandle,
                                                                     pcmDevIds.at(0),
                                                                     payload, payloadSize);
                            sess->freeCustomPayload();
                            payload = NULL;
                            payloadSize = 0;
                            if (status != 0) {
                                PAL_ERR(LOG_TAG, "setMixerParameter failed");
                                goto exit;
                            }
                        }
                    }
                } else {
                    sess->getCustomPayload(&payload, &payloadSize);
                    if (payload) {
                        status = SessionAlsaUtils::setMixerParameter(mixerHandle, pcmDevIds.at(0),
                                                                 payload, payloadSize);
                        sess->freeCustomPayload();
                        payload = NULL;
                        payloadSize = 0;
                        if (status != 0) {
                            PAL_ERR(LOG_TAG, "setMixerParameter failed");
                            goto exit;
                        }
                    }
                }

            } else {
                PAL_ERR(LOG_TAG, "invalid session audio object");
                status = -EINVAL;
                goto exit;
            }
        }
        if (sAttr.direction == PAL_AUDIO_INPUT) {
            if (strstr(dAttr.custom_config.custom_key , "unprocessed-hdr-mic") &&
                (dAttr.id == PAL_DEVICE_IN_HANDSET_MIC || dAttr.id == PAL_DEVICE_IN_SPEAKER_MIC)) {
                status = sess->setConfig(streamHandle, MODULE,  ORIENTATION_TAG);
                if (0 != status) {
                    PAL_ERR(LOG_TAG, "setting HDR record orientation config failed with status %d", status);
                    goto exit;
                }
            }
            if (streamType == PAL_STREAM_ULTRA_LOW_LATENCY ||
               (dAttr.id == PAL_DEVICE_IN_PROXY || dAttr.id == PAL_DEVICE_IN_RECORD_PROXY)) {
                if (sess) {
                    sess->configureMFC(rmHandle, sAttr, dAttr, pcmDevIds,
                                    aifBackEndsToConnect[0].second.data());
                    sess->getCustomPayload(&payload, &payloadSize);
                    if (payload) {
                        status = SessionAlsaUtils::setMixerParameter(mixerHandle, pcmDevIds.at(0),
                                                payload, payloadSize);
                    }
                    sess->freeCustomPayload();
                    payload = NULL;
                    payloadSize = 0;
                    if (status != 0) {
                        PAL_ERR(LOG_TAG, "setMixerParameter failed");
                        goto exit;
                    }
                } else {
                    PAL_ERR(LOG_TAG, "invalid session audio object");
                    status = -EINVAL;
                    goto exit;
                }
            }
        }
    } else if (!(SessionAlsaUtils::isMmapUsecase(sAttr))) {
        if (sess) {
            SessionAlsaVoice *voiceSession = dynamic_cast<SessionAlsaVoice *>(sess);
            if (!voiceSession) {
                PAL_ERR(LOG_TAG, "invalid session voice object");
                status = -EINVAL;
                goto exit;
            }
            if (streamHandle->getCurState() != STREAM_INIT) {
                if (SessionAlsaUtils::isRxDevice(aifBackEndsToConnect[0].first)) {
                    voiceSession->setSessionParameters(streamHandle, RX_HOSTLESS);
                } else {
                    voiceSession->setSessionParameters(streamHandle, TX_HOSTLESS);
               }
            }
        } else {
            PAL_ERR(LOG_TAG, "invalid session voice object");
            status = -EINVAL;
            goto exit;
        }
    }


    connectCtrl = mixer_get_ctl_by_name(mixerHandle, connectCtrlName.str().data());
    if (!connectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", connectCtrlName.str().data());
        status = -EINVAL;
        goto exit;
    }
    status = mixer_ctl_set_enum_by_string(connectCtrl, aifBackEndsToConnect[0].second.data());

    if (PAL_STREAM_VOICE_CALL == streamType) {
        SessionAlsaVoice *voiceSession = dynamic_cast<SessionAlsaVoice *>(sess);
        if (!voiceSession) {
            PAL_ERR(LOG_TAG, "invalid session voice object");
            status = -EINVAL;
            goto exit;
        }
        if (SessionAlsaUtils::isRxDevice(aifBackEndsToConnect[0].first)) {
            rm->resumeInCallMusic();
        }
    }
exit:
    if (builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

int SessionAlsaUtils::connectSessionDevice(Session* sess, Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmTxDevIds,const std::vector<int> &pcmRxDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToConnect)
{
    std::ostringstream connectCtrlName;
    int status = 0;
    struct mixer *mixerHandle = nullptr;
    struct mixer_ctl *connectCtrl = nullptr;
    struct mixer_ctl *txFeMixerCtrls[FE_MAX_NUM_MIXER_CONTROLS] = { nullptr };
    std::ostringstream txFeName,rxFeName;
    struct pal_stream_attributes sAttr = {};
    uint8_t* payload = NULL;
    size_t payloadSize = 0;
    bool is_out_dev = false;

    if (dAttr.id > PAL_DEVICE_OUT_MIN && dAttr.id < PAL_DEVICE_OUT_MAX) {
        is_out_dev = true;
        connectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmRxDevIds.at(0) << " connect";
    } else if (dAttr.id > PAL_DEVICE_IN_MIN && dAttr.id < PAL_DEVICE_IN_MAX) {
        connectCtrlName << PCM_SND_DEV_NAME_PREFIX << pcmTxDevIds.at(0) << " connect";
    }
    if (rmHandle == nullptr || streamHandle == nullptr) {
        status = -EINVAL;
        PAL_ERR(LOG_TAG, "rmHandle or streamHandle is invalid");
        goto exit;
    }
    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "get mixer handle failed %d", status);
        goto exit;
    }
    status = streamHandle->getStreamAttributes(&sAttr);
    if (status) {
        PAL_ERR(LOG_TAG, "could not get stream attributes, status:%d", status);
        goto exit;
    }
    if ((((dAttr.id == PAL_DEVICE_OUT_SPEAKER || dAttr.id == PAL_DEVICE_OUT_HANDSET) ||
          (rmHandle->activeGroupDevConfig && dAttr.id == PAL_DEVICE_OUT_ULTRASOUND)) &&
          (streamType == PAL_STREAM_ULTRASOUND)) ||
        (is_out_dev && streamType == PAL_STREAM_LOOPBACK)) {
        if (sess) {
            sess->configureMFC(rmHandle,sAttr, dAttr, pcmRxDevIds,
                                aifBackEndsToConnect[0].second.data());
            sess->getCustomPayload(&payload, &payloadSize);
            if (payload) {
                status = SessionAlsaUtils::setMixerParameter(mixerHandle, pcmRxDevIds.at(0),
                                                         payload, payloadSize);
                sess->freeCustomPayload();
                payload = NULL;
                payloadSize = 0;
                if (status != 0) {
                    PAL_ERR(LOG_TAG, "setMixerParameter failed");
                    goto exit;
                }
            }
        } else {
            PAL_ERR(LOG_TAG, "invalid session ultrasound object");
            status = -EINVAL;
            goto exit;
        }
    }

    connectCtrl = mixer_get_ctl_by_name(mixerHandle, connectCtrlName.str().data());
    if (!connectCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", connectCtrlName.str().data());
        status = -EINVAL;
        goto exit;
    }
    /** connect FE to BE */
    mixer_ctl_set_enum_by_string(connectCtrl, aifBackEndsToConnect[0].second.data());

    switch (streamType) {
         case PAL_STREAM_ULTRASOUND:
         case PAL_STREAM_LOOPBACK:
             status = rmHandle->getVirtualAudioMixer(&mixerHandle);
             txFeName << PCM_SND_DEV_NAME_PREFIX << pcmTxDevIds.at(0);
             rxFeName << PCM_SND_DEV_NAME_PREFIX << pcmRxDevIds.at(0);
             txFeMixerCtrls[FE_LOOPBACK] = getFeMixerControl(mixerHandle, txFeName.str(), FE_LOOPBACK);
             if (!txFeMixerCtrls[FE_LOOPBACK]) {
                 PAL_ERR(LOG_TAG, "invalid mixer control %s",
                         txFeName.str().data());
                 status = -EINVAL;
                 goto exit;
             }
             mixer_ctl_set_enum_by_string(txFeMixerCtrls[FE_LOOPBACK], rxFeName.str().data());
             break;
         default:
             PAL_ERR(LOG_TAG, "unknown stream type %d",streamType);
             break;
    }

exit:
    return status;
}

int SessionAlsaUtils::setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToConnect)
{
    std::ostringstream cntrlName;
    std::ostringstream aifMdName;
    std::ostringstream aifMfCtrlName;
    std::ostringstream feMdName;
    std::ostringstream connectCtrlName;
    std::vector <std::pair<int, int>> streamDeviceKV;
    std::vector <std::pair<int, int>> deviceKV;
    std::vector <std::pair<int, int>> emptyKV;
    std::vector <std::pair<int, int>> deviceCKV;
    std::vector <std::pair<int, int>> devicePPCKV;
    int status = 0;
    struct agmMetaData deviceMetaData(nullptr, 0);
    struct agmMetaData streamDeviceMetaData(nullptr, 0);
    struct mixer_ctl *feCtrl = nullptr;
    struct mixer_ctl *feMdCtrl = nullptr;
    struct mixer_ctl *aifMdCtrl = nullptr;
    PayloadBuilder* builder = new PayloadBuilder();
    struct mixer *mixerHandle = nullptr;
    uint32_t devicePropId[] = {0x08000010, 2, 0x2, 0x5};
    uint32_t streamDevicePropId[] = {0x08000010, 1, 0x3}; /** gsl_subgraph_platform_driver_props.xml */
    bool is_compress = false;
    struct pal_stream_attributes sAttr = {};
    int sub = 1;
    struct pal_device_info devinfo = {};
    struct vsid_info vsidinfo = {};
    sidetone_mode_t sidetoneMode = SIDETONE_OFF;

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_VERBOSE(LOG_TAG, "get mixer handle failed %d", status);
        goto exit;
    }

    status = streamHandle->getStreamAttributes(&sAttr);
    if(0 != status) {
        PAL_ERR(LOG_TAG, "getStreamAttributes Failed \n");
        goto exit;
    }

    if(sAttr.type == PAL_STREAM_VOICE_CALL){
        //get vsid info
        status = rmHandle->getVsidInfo(&vsidinfo);
        if(status) {
            PAL_ERR(LOG_TAG, "get vsid info failed");
        }
        PAL_DBG(LOG_TAG, "Device Id : %d  Stream Type : %d passed to getSidetoneMode", dAttr.id , sAttr.type );
        status = rmHandle->getSidetoneMode(dAttr.id, sAttr.type, &sidetoneMode);
        if(status) {
            PAL_ERR(LOG_TAG, "get sidetone mode failed");
        }
    }

    if (SessionAlsaUtils::isRxDevice(aifBackEndsToConnect[0].first))
        status = builder->populateStreamDeviceKV(streamHandle,
            aifBackEndsToConnect[0].first, streamDeviceKV, 0, emptyKV, vsidinfo,
                                                 sidetoneMode);
    else
        status = builder->populateStreamDeviceKV(streamHandle,
            0, emptyKV, aifBackEndsToConnect[0].first, streamDeviceKV, vsidinfo,
                                                 sidetoneMode);

    if (status) {
        PAL_VERBOSE(LOG_TAG, "get stream device KV failed %d", status);
        status = 0; /**< ignore stream device KV failures */
    }
    if ((status = builder->populateDeviceKV(streamHandle,
                    aifBackEndsToConnect[0].first, deviceKV)) != 0) {
        PAL_ERR(LOG_TAG, "get device KV failed %d", status);
        goto exit;
    }
    if (SessionAlsaUtils::isRxDevice(aifBackEndsToConnect[0].first))
        status = builder->populateDevicePPKV(streamHandle,
                aifBackEndsToConnect[0].first, streamDeviceKV,
                0, emptyKV);
    else {
        rmHandle->getDeviceInfo(dAttr.id, streamType,
                                dAttr.custom_config.custom_key, &devinfo);
        status = builder->populateDevicePPKV(streamHandle, 0, emptyKV,
                aifBackEndsToConnect[0].first, streamDeviceKV);
    }
    if (status != 0) {
        PAL_ERR(LOG_TAG, "get device PP KV failed %d", status);
        status = 0; /** ignore error */
    }

    deviceCKV.clear();

    if (ResourceManager::isSpeakerProtectionEnabled) {
        PAL_DBG(LOG_TAG, "Speaker protection enabled");
        if (aifBackEndsToConnect[0].first == PAL_DEVICE_OUT_SPEAKER) {
            status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                            SPKR_PROT_ENABLE);
            if (status != 0) {
                PAL_VERBOSE(LOG_TAG, "Unable to populate SP cal");
                status = 0; /**< ignore device SP CKV failures */
            }
        }
    }
    if (aifBackEndsToConnect[0].first == PAL_DEVICE_OUT_HAPTICS_DEVICE &&
                                         ResourceManager::isHapticsthroughWSA) {
            status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                            HAPTICS_PROT_ENABLE);
            if (status != 0) {
                PAL_VERBOSE(LOG_TAG, "Unable to populate HP cal");
                status = 0; /**< ignore device HP CKV failures */
            }
    }

   if (ResourceManager::isHandsetProtectionEnabled &&
          ResourceManager::isSpeakerProtectionEnabled) {
       PAL_DBG(LOG_TAG, "Handset enabled");
       if (aifBackEndsToConnect[0].first == PAL_DEVICE_OUT_HANDSET) {
            status = builder->populateCalKeyVector(streamHandle, deviceCKV,
                            HANDSET_PROT_ENABLE);
            if (status != 0) {
                PAL_VERBOSE(LOG_TAG, "Unable to populate SP cal");
                status = 0; /**< ignore device SP CKV failures */
            }
       }
   }


    if (deviceKV.size() > 0) {
        SessionAlsaUtils::getAgmMetaData(deviceKV, deviceCKV, (struct prop_data *)devicePropId,
                deviceMetaData);
        if (!deviceMetaData.size) {
            PAL_ERR(LOG_TAG, "device meta data is zero");
            status = -ENOMEM;
            goto freeMetaData;
        }
    }

    status = builder->populateDevicePPCkv(streamHandle, devicePPCKV);
    if (status) {
        PAL_ERR(LOG_TAG, "populateDevicePP Ckv failed %d", status);
        status = 0; /**< ignore device PP CKV failures */
    }

    if (streamDeviceKV.size() > 0 || devicePPCKV.size() > 0) {
        SessionAlsaUtils::getAgmMetaData(streamDeviceKV, devicePPCKV,
                (struct prop_data *)streamDevicePropId,
                streamDeviceMetaData);
        if (!streamDeviceMetaData.size) {
            PAL_ERR(LOG_TAG, "stream/device meta data is zero");
            status = -ENOMEM;
            goto freeMetaData;
        }
    }

    switch (streamType) {
        case PAL_STREAM_COMPRESSED:
            cntrlName << COMPRESS_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " control";
            aifMdName << aifBackEndsToConnect[0].second.data() << " metadata";
            feMdName << COMPRESS_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " metadata";
            is_compress = true;
            break;
        case PAL_STREAM_VOICE_CALL:
            status = streamHandle->getStreamAttributes(&sAttr);
            if (status) {
                PAL_ERR(LOG_TAG, "could not get stream attributes\n");
                goto exit;
            }
            if (sAttr.info.voice_call_info.VSID == VOICEMMODE1 ||
                sAttr.info.voice_call_info.VSID == VOICELBMMODE1)
                sub = 1;
            else
                sub = 2;

            if (dAttr.id > PAL_DEVICE_OUT_MIN && dAttr.id < PAL_DEVICE_OUT_MAX) {
                cntrlName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "p" << " control";
                aifMdName << aifBackEndsToConnect[0].second.data() << " metadata";
                feMdName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "p" << " metadata";
            } else if (dAttr.id > PAL_DEVICE_IN_MIN && dAttr.id < PAL_DEVICE_IN_MAX) {
                cntrlName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "c" << " control";
                aifMdName << aifBackEndsToConnect[0].second.data() << " metadata";
                feMdName << PCM_SND_VOICE_DEV_NAME_PREFIX << sub << "c" << " metadata";

            }
            break;
        default:
            cntrlName << PCM_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " control";
            aifMdName << aifBackEndsToConnect[0].second.data() << " metadata";
            feMdName << PCM_SND_DEV_NAME_PREFIX << pcmDevIds.at(0) << " metadata";
            break;
    }

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);

    aifMdCtrl = mixer_get_ctl_by_name(mixerHandle, aifMdName.str().data());
    PAL_DBG(LOG_TAG, "mixer control %s", aifMdName.str().data());
    if (!aifMdCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", aifMdName.str().data());
        status = -EINVAL;
        goto freeMetaData;
    }
    if (deviceMetaData.size)
        mixer_ctl_set_array(aifMdCtrl, (void *)deviceMetaData.buf, deviceMetaData.size);

    feCtrl = mixer_get_ctl_by_name(mixerHandle, cntrlName.str().data());
    PAL_DBG(LOG_TAG, "mixer control %s", cntrlName.str().data());
    if (!feCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", cntrlName.str().data());
        status = -EINVAL;
        goto freeMetaData;
    }
    mixer_ctl_set_enum_by_string(feCtrl, aifBackEndsToConnect[0].second.data());

    feMdCtrl = mixer_get_ctl_by_name(mixerHandle, feMdName.str().data());
    PAL_DBG(LOG_TAG, "mixer control %s", feMdName.str().data());
    if (!feMdCtrl) {
        PAL_ERR(LOG_TAG, "invalid mixer control: %s", feMdName.str().data());
        status = -EINVAL;
        goto freeMetaData;
    }
    if (streamDeviceMetaData.size)
        mixer_ctl_set_array(feMdCtrl, (void *)streamDeviceMetaData.buf, streamDeviceMetaData.size);
freeMetaData:
    free(streamDeviceMetaData.buf);
    free(deviceMetaData.buf);

exit:
    if(builder) {
       delete builder;
       builder = NULL;
    }
    return status;
}

unsigned int SessionAlsaUtils::bytesToFrames(size_t bufSizeInBytes, unsigned int channels,
                           enum pcm_format format)
{
    unsigned int bits = pcm_format_to_bits(format);
    unsigned int ch = (channels == 0)? 1 : channels ;

    return (bufSizeInBytes * 8)/(ch*bits);
}

int SessionAlsaUtils::flush(std::shared_ptr<ResourceManager> rmHandle, uint32_t id)
{
    int status = 0;
    int doFlush = 1;
    struct mixer *mixerHandle = nullptr;
    struct mixer_ctl *ctl = nullptr;
    char *pcmDeviceName = nullptr;

    pcmDeviceName = rmHandle->getDeviceNameFromID(id);

    if(!pcmDeviceName) {
        PAL_ERR(LOG_TAG, "Device name from id not found");
        return -EINVAL;
    }

    status = rmHandle->getVirtualAudioMixer(&mixerHandle);
    if (status) {
        PAL_ERR(LOG_TAG, "Error: Failed to get mixer handle\n");
        return status;
    }


    ctl = getFeMixerControl(mixerHandle, std::string(pcmDeviceName), FE_FLUSH);
    if (!ctl) {
        return -ENOENT;
    }

    mixer_ctl_set_value(ctl, 0, doFlush);

    return status;
}
