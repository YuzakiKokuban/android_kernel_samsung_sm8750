/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <agm/agm_api.h>
#include <aidl/vendor/qti/hardware/agm/AgmBuff.h>
#include <aidl/vendor/qti/hardware/agm/AgmBufferConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmCalConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmDataMode.h>
#include <aidl/vendor/qti/hardware/agm/AgmEventCallbackParameter.h>
#include <aidl/vendor/qti/hardware/agm/AgmEventRegistrationConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmGroupMediaConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmMediaConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmMediaFormat.h>
#include <aidl/vendor/qti/hardware/agm/AgmReadWriteEventCallbackParams.h>
#include <aidl/vendor/qti/hardware/agm/AgmSessionConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmSessionMode.h>
#include <aidl/vendor/qti/hardware/agm/AgmTagConfig.h>
#include <aidl/vendor/qti/hardware/agm/AifInfo.h>
#include <aidl/vendor/qti/hardware/agm/Direction.h>
#include <aidl/vendor/qti/hardware/agm/MmapBufInfo.h>

using namespace ::aidl::vendor::qti::hardware::agm;

struct LegacyToAidl {
    /**
    * @brief convertAgmSessionConfigToAidl converts legacy agm_session_config to AIDL
    * based AgmSessionConfig type.
    * @param session_config  agm_session_config in legacy types
    * @param format agm_media_format to fetch session codec info for compress codecs.
    * @return AgmSessionConfig AIDL object
    */
    static AgmSessionConfig convertAgmSessionConfigToAidl(struct agm_session_config *sessionConfig,
                                                          agm_media_format format);

    /**
    * @brief convertAgmMediaConfigToAidl converts legacy agm_media_config to AIDL
    * based AgmMediaConfig type.
    * @param media_config  agm_media_config in legacy types
    * @return AgmMediaConfig AIDL object
    */
    static AgmMediaConfig convertAgmMediaConfigToAidl(struct agm_media_config *mediaConfig);

    /**
    * @brief convertAgmBufferConfigToAidl converts legacy agm_buffer_config to AIDL
    * based AgmBufferConfig type.
    * @param buffer_config  agm_buffer_config in legacy types
    * @return AgmBufferConfig AIDL object
    */
    static AgmBufferConfig convertAgmBufferConfigToAidl(struct agm_buffer_config *bufferConfig);

    /**
    * @brief convertAgmGroupMediaConfigToAidl converts legacy agm_group_media_config to AIDL
    * based AgmGroupMediaConfig type.
    * @param media_config  agm_group_media_config in legacy types
    * @return AgmGroupMediaConfig AIDL object
    */
    static AgmGroupMediaConfig convertAgmGroupMediaConfigToAidl(
            struct agm_group_media_config *mediaConfig);

    /**
    * @brief convertAgmTagConfigToAidl converts legacy agm_tag_config to AIDL
    * based AgmTagConfig type.
    * @param tag_config  agm_tag_config in legacy types
    * @return AgmTagConfig AIDL object
    */
    static AgmTagConfig convertAgmTagConfigToAidl(struct agm_tag_config *tagConfig);

    /**
    * @brief convertAgmCalConfigToAidl converts legacy agm_cal_config to AIDL
    * based AgmCalConfig type.
    * @param tag_config  agm_cal_config in legacy types
    * @return AgmCalConfig AIDL object
    */
    static AgmCalConfig convertAgmCalConfigToAidl(struct agm_cal_config *calConfig);

    /**
    * @brief convertAgmEventRegistrationConfigToAidl converts legacy agm_event_reg_cfg to AIDL
    * based AgmEventRegistrationConfig type.
    * @param evt_reg_cfg  agm_event_reg_cfg in legacy types
    * @return AgmEventRegistrationConfig AIDL object
    */
    static AgmEventRegistrationConfig convertAgmEventRegistrationConfigToAidl(
            agm_event_reg_cfg *eventRegConfig);

    /**
    * @brief convertCompressCodecInfoToAidl converts legacy agm_session_config to AIDL
    * based AgmSessionCodec type.
    * @param session_config  agm_session_config in legacy types
    * @param format  agm_media_format in legacy types
    * @return AgmSessionCodec AIDL object
    */
    static AgmSessionCodec convertCompressCodecInfoToAidl(struct agm_session_config *sessionConfig,
                                                          agm_media_format format);

    /**
    * @brief convertCompressDecoderInfoToAidl converts legacy agm_session_codec to AIDL
    * based AgmSessionCodec type.
    * @param sessionCodec agm_session_codec in legacy types
    * @param format agm_media_format in legacy types
    * @return AgmSessionCodec AIDL object
    */
    static AgmSessionCodec convertCompressDecoderInfoToAidl(union agm_session_codec *sessionCodec,
                                                            agm_media_format format);

    /**
    * @brief convertCompressEncoderInfoToAidl converts legacy agm_session_codec to AIDL
    * based AgmSessionCodec type.
    * @param sessionCodec  agm_session_codec in legacy types
    * @param format  agm_media_format in legacy types
    * @return AgmSessionCodec AIDL object
    */
    static AgmSessionCodec convertCompressEncoderInfoToAidl(union agm_session_codec *sessionCodec,
                                                            agm_media_format format);

    /**
    * @brief convertAacCompressDecoderToAidl converts legacy agm_session_aac_dec to AIDL
    * based AgmSessionAacDec type.
    * @param legacyDecoder agm_session_aac_dec in legacy types
    * @return AgmSessionAacDec AIDL object
    */
    static AgmSessionAacDec convertAacCompressDecoderToAidl(
            struct agm_session_aac_dec *legacyDecoder);

    /**
    * @brief convertFlacCompressDecoderToAidl converts legacy agm_session_flac_dec to AIDL
    * based AgmSessionFlacDec type.
    * @param legacyDecoder agm_session_flac_dec in legacy types
    * @return AgmSessionFlacDec AIDL object
    */
    static AgmSessionFlacDec convertFlacCompressDecoderToAidl(
            struct agm_session_flac_dec *legacyDecoder);

    /**
    * @brief convertAlacCompressDecoderToAidl converts legacy agm_session_alac_dec to AIDL
    * based AgmSessionAlacDec type.
    * @param legacyDecoder agm_session_alac_dec in legacy types
    * @return AgmSessionAlacDec AIDL object
    */
    static AgmSessionAlacDec convertAlacCompressDecoderToAidl(
            struct agm_session_alac_dec *legacyDecoder);

    /**
    * @brief convertApeCompressDecoderToAidl converts legacy agm_session_ape_dec to AIDL
    * based AgmSessionApeDec type.
    * @param legacyDecoder agm_session_ape_dec in legacy types
    * @return AgmSessionApeDec AIDL object
    */
    static AgmSessionApeDec convertApeCompressDecoderToAidl(
            struct agm_session_ape_dec *legacyDecoder);

    /**
    * @brief convertWmaProCompressDecoderToAidl converts legacy agm_session_wmapro_dec to AIDL
    * based AgmSessionWmaproDec type.
    * @param legacyDecoder  agm_session_wmapro_dec in legacy types
    * @return AgmSessionWmaproDec AIDL object
    */
    static AgmSessionWmaproDec convertWmaProCompressDecoderToAidl(
            struct agm_session_wmapro_dec *legacyDecoder);

    /**
    * @brief convertWmaStandardCompressDecoderToAidl converts legacy agm_session_config to AIDL
    * based AgmSessionWmaDec type.
    * @param legacyDecoder  agm_session_config in legacy types
    * @return AgmSessionWmaDec AIDL object
    */
    static AgmSessionWmaDec convertWmaStandardCompressDecoderToAidl(
            struct agm_session_wma_dec *legacyDecoder);


    /**
    * @brief convertOpusCompressDecoderToAidl converts legacy agm_session_config to AIDL
    * based AgmSessionOpusDec type.
    * @param legacyDecoder  agm_session_config in legacy types
    * @return AgmSessionOpusDec AIDL object
    */
    static AgmSessionOpusDec convertOpusCompressDecoderToAidl(
            struct agm_session_opus_dec *legacyDecoder);

    /**
    * @brief convertAacCompressEncoderToAidl converts legacy agm_session_aac_enc to AIDL
    * based AgmSessionAacEnc type.
    * @param sessionConfig  agm_session_aac_enc in legacy types
    * @return AgmSessionAacEnc AIDL object
    */
    static AgmSessionAacEnc convertAacCompressEncoderToAidl(
            struct agm_session_aac_enc *legacyEncoder);

    /**
    * @brief convertAgmEventCallbackParametersToAidl converts legacy agm_event_cb_params to AIDL
    * based AgmEventCallbackParameter type.
    * @param eventParams  agm_event_cb_params in legacy types
    * @return AgmEventCallbackParameter AIDL object
    */
    static AgmEventCallbackParameter convertAgmEventCallbackParametersToAidl(
            struct agm_event_cb_params *eventParams);

    /**
    * @brief convertRawPayloadToVector converts legacy void * memory to std::vector
    * so it can be used over AIDL.
    * @param payload  void * memory chunk
    * @param size size of memory chunk passed, vector will be allocated according to this size
    * @return std::vector<uint8_t> copied from memory chunk
    */
    static std::vector<uint8_t> convertRawPayloadToVector(void *payload, size_t size);

    /**
    * @brief convertAifInfoListToAidl converts legacy aif_info to AIDL
    * based AifInfo type.
    * @param agmLegacyAifInfoList  aif_info in legacy types
    * @param size size of aifInfo list
    * @return vector of AifInfo based on legacy aif_info list
    */
    static std::vector<AifInfo> convertAifInfoListToAidl(struct aif_info *agmLegacyAifInfoList,
                                                         int size);
    /*
    * input buff is legacy agm buffer.
    * @param externalMemory if nt mode use case
    * @param copyBuffers for read usecases no need to copy the input buffers.
    */

    /**
    * @brief convertAgmBufferToAidl converts legacy agm_buff to AIDL
    * based AgmBuff type.
    * @param buf agm_buff in legacy type.
    * @param externalMemory set externalMemory to indicate non tunnel mode usecase.
    * NonTunnel mode has metadata and fd infos.
    * @param copyBuffers a boolean indicating whether buffers needs to be copied in returned
    * AIDL types, for read usecases, AgmBuff does not need copyBuffers.
    * @return AgmBuff converted from agm legacyBuffer
    */
    static AgmBuff convertAgmBufferToAidl(struct agm_buff *buf, bool externalMemory = false,
                                          bool copyBuffers = true);

    /**
    * @brief getDupedFdFromAgmEventParams gets a dup'ed Fd from callback params.
    * @param eventParams  callback parameter of type agm_event_cb_params
    * @return fd found in the callback.
    */
    static int getDupedFdFromAgmEventParams(struct agm_event_cb_params *eventParams);

    /**
    * @brief cleanUpMetadataMemory dellocates the memory of metadata field in agm_event_cb_params.
    * This is used explicitly for callback associated with read_with_metadata API, as
    * read_with_metadata API allocates a metadata which is used during callback on
    * AGM_EVENT_READ_DONE
    * callback.
    * @param eventParams  agm_event_cb_params in legacy types
    * @return AgmSessionConfig AIDL object
    */
    static void cleanUpMetadataMemory(struct agm_event_cb_params *eventParams);

    /**
    * @brief convertAgmReadWriteEventCallbackParamsToAidl converts legacy agm_event_cb_params to
    * AIDL
    * based AgmReadWriteEventCallbackParams type.
    * @param eventParams  agm_event_cb_params in legacy types
    * @param inputFd fd information sent during callback
    * @return AgmReadWriteEventCallbackParams AIDL object
    */
    static AgmReadWriteEventCallbackParams convertAgmReadWriteEventCallbackParamsToAidl(
            struct agm_event_cb_params *eventParams, int inputFd);

    /**
    * @brief convertMmapBufferInfoToAidl converts agm_buf_info agm_session_config to AIDL
    * based MmapBufInfo type.
    * @param legacyBufferInfo  agm_buf_info in legacy types
    * @param aidlBufferInfo MmapBufInfo AIDL pointer
    * @param flags flags associated with agm_buf_info
    */
    static void convertMmapBufferInfoToAidl(struct agm_buf_info *legacyBufferInfo,
                                            MmapBufInfo *aidlBufferInfo, int flags);
};
