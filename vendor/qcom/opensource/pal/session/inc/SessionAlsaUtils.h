/*
 * Copyright (c) 2019, 2021 The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef SESSION_ALSAUTILS_H
#define SESSION_ALSAUTILS_H

#include "Session.h"
#include "ResourceManager.h"
#include "PayloadBuilder.h"

#include <tinyalsa/asoundlib.h>
#include <sound/asound.h>

#define PADDING_8BYTE_ALIGN(x) ((((x) + 7) & 7) ^ 7)
#define MAX_UTIL_PAYLOAD_SIZE ( \
           sizeof(struct apm_module_param_data_t) + \
           sizeof(struct param_id_spr_session_time_t) + \
           PADDING_8BYTE_ALIGN(sizeof(struct apm_module_param_data_t) + \
           sizeof(struct param_id_spr_session_time_t)))

class Stream;
class Session;

enum class MixerCtlType: uint32_t {
    MIXER_SET_ID_STRING,
    MIXER_SET_ID_VALUE,
    MIXER_SET_ID_ARRAY,
};

enum FeCtrlsIndex {
    FE_CONTROL,
    FE_METADATA,
    FE_CONNECT,
    FE_DISCONNECT,
    FE_SETPARAM,
    FE_GETTAGGEDINFO,
    FE_SETPARAMTAG,
    FE_GETPARAM,
    FE_ECHOREFERENCE,
    FE_SIDETONE,
    FE_LOOPBACK,
    FE_EVENT,
    FE_SETCAL,
    FE_FLUSH,
    FE_MAX_NUM_MIXER_CONTROLS,
};

enum BeCtrlsIndex {
    BE_METADATA,
    BE_MEDIAFMT,
    BE_SETPARAM,
    BE_GROUP_ATTR,
    BE_MAX_NUM_MIXER_CONTROLS,
};


class SessionAlsaUtils
{
private:
    SessionAlsaUtils() {};
    static struct mixer_ctl *getFeMixerControl(struct mixer *am, std::string feName,
        uint32_t idx);
    static struct mixer_ctl *getBeMixerControl(struct mixer *am, std::string beName,
        uint32_t idx);
    static struct mixer_ctl *getStaticMixerControl(struct mixer *am, std::string name);
public:
    ~SessionAlsaUtils();
    static bool isRxDevice(uint32_t devId);
    static int setMixerCtlData(struct mixer_ctl *ctl, MixerCtlType id, void *data, int size);
    static int getTagMetadata(int32_t tagsent, std::vector <std::pair<int, int>> &tkv, struct agm_tag_config *tagConfig);
    static int getCalMetadata(std::vector <std::pair<int, int>> &ckv, struct agm_cal_config* calConfig);
    static unsigned int bitsToAlsaFormat(unsigned int bits);
    static int openDev(std::shared_ptr<ResourceManager> rmHandle,
            const std::vector<int> &DevIds, int32_t backEndId, std::string backEndName);
    static int open(Stream * s, std::shared_ptr<ResourceManager> rm, const std::vector<int> &DevIds,
            const std::vector<std::pair<int32_t, std::string>> &BackEnds);
    static int open(Stream * s, std::shared_ptr<ResourceManager> rm,
                    const std::vector<int> &RxDevIds, const std::vector<int> &TxDevIds,
                    const std::vector<std::pair<int32_t, std::string>> &rxBackEnds,
                    const std::vector<std::pair<int32_t, std::string>> &txBackEnds);
    static int rwACDBTunnel(Stream * streamHandle, std::shared_ptr<ResourceManager> rmHandle,
                    pal_device_id_t deviceId, void *payload, bool isParamWrite, uint32_t instanceId);
    static int close(Stream * s, std::shared_ptr<ResourceManager> rm, const std::vector<int> &DevIds,
            const std::vector<std::pair<int32_t, std::string>> &BackEnds, std::vector<std::pair<std::string, int>> &freedevicemetadata);
    static int close(Stream * s, std::shared_ptr<ResourceManager> rm,
                    const std::vector<int> &RxDevIds, const std::vector<int> &TxDevIds,
                    const std::vector<std::pair<int32_t, std::string>> &rxBackEnds,
                    const std::vector<std::pair<int32_t, std::string>> &txBackEnds,
                    std::vector<std::pair<std::string, int>> &freeDeviceMetaData);
    static int getModuleInstanceId(struct mixer *mixer, int device, const char *intf_name,
                       int tag_id, uint32_t *miid);
    static int getTagsWithModuleInfo(struct mixer *mixer, int device, const char *intf_name,
                       uint8_t *payload);
    static int setMixerParameter(struct mixer *mixer, int device,
                                 void *payload, int size);
    static int setStreamMetadataType(struct mixer *mixer, int device, const char *val);
    static int registerMixerEvent(struct mixer *mixer, int device, const char *intf_name, int tag_id, void *payload, int payload_size);
    static int registerMixerEvent(struct mixer *mixer, int device, void *payload, int payload_size);
    static int setECRefPath(struct mixer *mixer, int device, const char *intf_name);

    static int getTimestamp(struct mixer *mixer, const std::vector<int> &DevIds, uint32_t spr_miid, struct pal_session_time *stime);
    static int disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rm, struct pal_device &dAttr,
        const std::vector<int> &pcmDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToDisconnect);
    static int disconnectSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmTxDevIds,const std::vector<int> &pcmRxDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToDisconnect);
    static int connectSessionDevice(Session* sess, Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rm, struct pal_device &dAttr,
        const std::vector<int> &pcmDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToConnect);
    static int connectSessionDevice(Session* sess, Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rmHandle, struct pal_device &dAttr,
        const std::vector<int> &pcmTxDevIds,const std::vector<int> &pcmRxDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToConnect);
    static int setupSessionDevice(Stream* streamHandle, pal_stream_type_t streamType,
        std::shared_ptr<ResourceManager> rm, struct pal_device &dAttr,
        const std::vector<int> &pcmDevIds,
        const std::vector<std::pair<int32_t, std::string>> &aifBackEndsToConnect);
    static std::shared_ptr<Device> getDeviceObj(int32_t beDevId,
        std::vector<std::shared_ptr<Device>> &associatedDevices);
    static pcm_format palToAlsaFormat(uint32_t fmt_id);
    static int setDeviceMetadata(std::shared_ptr<ResourceManager> rmHandle,
                                std::string backEndName,
                                std::vector <std::pair<int, int>> &deviceKV);
    static int setDeviceMediaConfig(std::shared_ptr<ResourceManager> rmHandle,
                            std::string backEndName, struct pal_device *dAttr);
    static int setDeviceCustomPayload(std::shared_ptr<ResourceManager> rmHandle,
                           std::string backEndName, void *payload, size_t size);
    static unsigned int bytesToFrames(size_t bufSizeInBytes, unsigned int channels,
                           enum pcm_format format);
    static bool isMmapUsecase(struct pal_stream_attributes &attr);
    static void getAgmMetaData(const std::vector <std::pair<int, int>> &kv,
                        const std::vector <std::pair<int, int>> &ckv,
                        struct prop_data *propData,
                        struct agmMetaData &md);
    static int rwParameterACDB(Stream * streamHandle, struct mixer *mixer,
                        void *inParamPayload, size_t inPayloadSize,
                        pal_device_id_t palDeviceId, uint32_t sampleRate,
                        uint32_t instanceId, bool isParamWrite);
   static int mixerWriteDatapathParams(struct mixer *mixer, int device,
                                        void *payload, int size);
   static int flush(std::shared_ptr<ResourceManager> rm, uint32_t id);
   static int getScoDevCount(void);

};

#endif //SESSION_ALSA_UTILS
