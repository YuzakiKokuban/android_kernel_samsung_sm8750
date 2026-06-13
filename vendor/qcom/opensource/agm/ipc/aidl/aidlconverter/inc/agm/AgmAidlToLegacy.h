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
#include <aidl/vendor/qti/hardware/agm/AgmEventRegistrationConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmGroupMediaConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmMediaConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmMediaFormat.h>
#include <aidl/vendor/qti/hardware/agm/AgmSessionConfig.h>
#include <aidl/vendor/qti/hardware/agm/AgmSessionMode.h>
#include <aidl/vendor/qti/hardware/agm/AgmTagConfig.h>
#include <aidl/vendor/qti/hardware/agm/AifInfo.h>
#include <aidl/vendor/qti/hardware/agm/Direction.h>
#include <aidl/vendor/qti/hardware/agm/MmapBufInfo.h>
#include <aidlcommonsupport/NativeHandle.h>

namespace aidl::vendor::qti::hardware::agm {

struct AidlToLegacy {
    /**
    * @brief convertAgmMediaConfig converts AgmMediaConfig from AIDL to agm_media_config
    * legacy type. Client needs to pass valid allocated memory of agm_media_config,
    * who is also responsible for dellocating the memory.
    * @param aidlConfig AgmMediaConfig media config in aidl types.
    * @param legacyConfig agm_media_config type.
    */
    static void convertAgmMediaConfig(const AgmMediaConfig &aidlConfig,
                                      struct agm_media_config *legacyConfig);

    /**
    * @brief convertAgmGroupMediaConfig converts AgmGroupMediaConfig from AIDL to
    * agm_group_media_config
    * legacy type. Client needs to pass valid allocated memory of agm_group_media_config,
    * who is also responsible for dellocating the memory.
    * @param aidlConfig AgmGroupMediaConfig media config in aidl types.
    * @param legacyConfig agm_group_media_config type.
    */
    static void convertAgmGroupMediaConfig(const AgmGroupMediaConfig &aidlConfig,
                                           struct agm_group_media_config *legacyConfig);

    /**
    * @brief convertAgmSessionConfig converts AgmSessionConfig from AIDL to agm_session_config
    * legacy type. Client needs to pass valid allocated memory of agm_session_config,
    * who is also responsible for dellocating the memory.
    * @param aidlConfig AgmSessionConfig media config in AIDL types.
    * @param legacyConfig agm_session_config type.
    */
    static void convertAgmSessionConfig(const AgmSessionConfig &aidlConfig,
                                        struct agm_session_config *legacyConfig);

    /**
    * @brief convertAgmBufferConfig converts AgmBufferConfig from AIDL to agm_buffer_config
    * legacy type. Client needs to pass valid allocated memory of agm_buffer_config,
    * who is also responsible for dellocating the memory.
    * @param aidlConfig AgmBufferConfig media config in AIDL types.
    * @param legacyConfig agm_buffer_config type.
    */
    static void convertAgmBufferConfig(const AgmBufferConfig &aidlConfig,
                                       struct agm_buffer_config *legacyConfig);

    /**
    * @brief convertAifInfoList converts AifInfo from AIDL to aif_info
    * legacy type. Client needs to pass valid allocated memory of aif_info,
    * who is also responsible for dellocating the memory.
    * @param aidlAifList std::vector of AifInfo
    * @param aif_list aif_info type.
    */
    static void convertAifInfoList(const std::vector<AifInfo> &aidlAifList,
                                   struct aif_info *legacyAifList);

    /**
    * @brief convertAgmEventRegistrationConfig converts AgmEventRegistrationConfig from AIDL to
    * agm_event_reg_cfg
    * legacy type. Client needs to pass valid allocated memory of agm_event_reg_cfg,
    * who is also responsible for dellocating the memory.
    * @param aidlEventRegistrationConfig  AgmEventRegistrationConfig in AIDL
    * @param legacyEventRegConfig agm_event_reg_cfg type.
    */
    static void convertAgmEventRegistrationConfig(
            const AgmEventRegistrationConfig &aidlEventRegistrationConfig,
            struct agm_event_reg_cfg *legacyEventRegConfig);

    /**
    * @brief convertCompressCodecInfo converts AgmSessionConfig from AIDL to agm_session_config
    * legacy type. Client needs to pass valid allocated memory of agm_session_config,
    * who is also responsible for dellocating the memory.
    * @param aidlConfig  AgmSessionConfig in AIDL
    * @param legacyConfig agm_session_config type.
    */
    static void convertCompressCodecInfo(const AgmSessionConfig &aidlConfig,
                                         struct agm_session_config *legacyConfig);
    /**
    * @brief convertAacCompressDecoder converts AgmSessionAacDec from AIDL to agm_session_aac_dec
    * Client needs to pass valid allocated memory of agm_session_config,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is aac type based on getTag of AIDL type.
    * @param legacyDecoder agm_session_aac_dec type.
    */
    static void convertAacCompressDecoder(const AgmSessionAacDec &aidlDecoder,
                                          struct agm_session_aac_dec *legacyDecoder);

    /**
    * @brief convertFlacCompressDecoder converts AgmSessionFlacDec from AIDL to agm_session_flac_dec
    * Client needs to pass valid allocated memory of agm_session_config,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is Flac type based on getTag of AIDL union type.
    * @param legacyDecoder agm_session_flac_dec type.
    */
    static void convertFlacCompressDecoder(const AgmSessionFlacDec &aidlDecoder,
                                           struct agm_session_flac_dec *legacyDecoder);

    /**
    * @brief convertAlacCompressDecoder converts AgmSessionAlacDec from AIDL to agm_session_alac_dec
    * Client needs to pass valid allocated memory of agm_session_config,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is alac type based on getTag of AIDL union type.
    * @param legacyDecoder agm_session_alac_dec type.
    */
    static void convertAlacCompressDecoder(const AgmSessionAlacDec &aidlDecoder,
                                           struct agm_session_alac_dec *legacyDecoder);

    /**
    * @brief convertApeCompressDecoder converts AgmSessionApeDec from AIDL to agm_session_ape_dec
    * Client needs to pass valid allocated memory of agm_session_ape_dec,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is ape type based on getTag of AIDL union type.
    * @param legacyDecoder agm_session_ape_dec type.
    */
    static void convertApeCompressDecoder(const AgmSessionApeDec &aidlDecoder,
                                          struct agm_session_ape_dec *legacyDecoder);

    /**
    * @brief convertWmaProCompressDecoder converts AgmSessionWmaproDec from AIDL to
    * agm_session_wmapro_dec
    * Client needs to pass valid allocated memory of agm_session_wmapro_dec,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is wmapro type based on getTag of AIDL union type.
    * @param legacyDecoder agm_session_wmapro_dec type.
    */
    static void convertWmaProCompressDecoder(const AgmSessionWmaproDec &aidlDecoder,
                                             struct agm_session_wmapro_dec *legacyDecoder);

    /**
    * @brief convertWmaStandardCompressDecoder converts AgmSessionCodec from AIDL to
    * agm_session_wma_dec
    * Client needs to pass valid allocated memory of agm_session_wma_dec,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is wma type based on getTag of AIDL union type.
    * @param legacyDecoder agm_session_wma_dec type.
    */
    static void convertWmaStandardCompressDecoder(const AgmSessionWmaDec &aidlDecoder,
                                                  struct agm_session_wma_dec *legacyDecoder);

    /**
    * @brief convertOpusCompressDecoder converts AgmSessionCodec from AIDL to
    * agm_session_opus_dec
    * Client needs to pass valid allocated memory of agm_session_opus_dec,
    * who is also responsible for dellocating the memory.
    * @param aidlDecoder codec is opus type based on getTag of AIDL union type.
    * @param legacyDecoder agm_session_opus_dec type.
    */
    static void convertOpusCompressDecoder(const AgmSessionOpusDec &aidlDecoder,
                                                  struct agm_session_opus_dec *legacyDecoder);
    /**
    * @brief convertAacCompressEncoder converts AgmSessionCodec from AIDL to agm_session_aac_enc
    * Client needs to pass valid allocated memory of agm_session_aac_enc,
    * who is also responsible for dellocating the memory.
    * @param aidlEncoder codec is aac type encoder based on getTag of AIDL union type.
    * @param legacyEncoder agm_session_aac_enc type.
    */
    static void convertAacCompressEncoder(const AgmSessionAacEnc &aidlEncoder,
                                          struct agm_session_aac_enc *legacyEncoder);
    /**
    * @brief getFdIntFromNativeHandle returns fd and associated ints based on
    * AIDL NativeHandle.
    * @param nativeHandle aidl NativeHandle
    * @param doDup fd needs to be duped or not.
    * @return pair of ints (fd, associated int)
    */
    static std::pair<int, int> getFdIntFromNativeHandle(
            const aidl::android::hardware::common::NativeHandle &nativeHandle, bool doDup = true);

    /**
    * @brief getFdFromNativeHandle returns dup'ed fd from nativehandle.
    * @param nativeHandle aidl NativeHandle
    * @return dup'ed fd
    */
    static int getFdFromNativeHandle(
            const aidl::android::hardware::common::NativeHandle &nativeHandle);

    /**
    * @brief convertAgmBuffer converts AgmBuff from AIDL to agm_buff
    * legacy type. Client needs to pass valid allocated memory of agm_buff,
    * who is also responsible for dellocating the memory.
    * @param aidlBuffer buffer in AgmBuff format.
    * @param legacyAgmBuffer agm_buff type.
    */
    static void convertAgmBuffer(const AgmBuff &aidlBuffer, struct agm_buff *legacyAgmBuffer);

    /**
    * @brief convertMmapBufInfo converts MmapBufInfo from AIDL to agm_buf_info
    * legacy type. Client needs to pass valid allocated memory of agm_buf_info,
    * who is also responsible for dellocating the memory.
    * @param aidlMmapBufferInfo buffer info in MmapBufInfo AIDL type.
    * @param flags indicating whether a data or pos buffer
    * @param legacyAgmBufferInfo agm_session_config type.
    */
    static void convertMmapBufInfo(const MmapBufInfo &aidlMmapBufferInfo,
                                   struct agm_buf_info *legacyAgmBufferInfo, uint32_t flag);

    /**
    * @brief convertAgmCalConfig converts AgmCalConfig from AIDL to agm_cal_config
    * legacy type. Client needs to pass valid allocated memory of agm_cal_config,
    * who is also responsible for dellocating the memory.
    * @param aidlCalConfig config in AgmCalConfig type.
    * @param legacyAgmCalConfig agm_cal_config type.
    */
    static void convertAgmCalConfig(const AgmCalConfig &aidlCalConfig,
                                    struct agm_cal_config *legacyAgmCalConfig);

    /**
    * @brief convertAgmTagConfig converts AgmTagConfig from AIDL to agm_tag_config
    * legacy type. Client needs to pass valid allocated memory of agm_tag_config,
    * who is also responsible for dellocating the memory.
    * @param aidlCalConfig config in AgmTagConfig type.
    * @param legacyAgmTagConfig agm_tag_config type.
    */
    static void convertAgmTagConfig(const AgmTagConfig &aidlTagConfig,
                                    struct agm_tag_config *legacyAgmTagConfig);
};
}