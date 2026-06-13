/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AgmIpc::LegacyToAidl::Converter"

#include <agm/AgmLegacyToAidl.h>
#include <agm/Utils.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <log/log.h>
#include "gsl_intf.h"

AgmSessionAacDec LegacyToAidl::convertAacCompressDecoderToAidl(
        struct agm_session_aac_dec *legacyDecoder) {
    AgmSessionAacDec aidlDecoder;
    aidlDecoder.formatFlag = legacyDecoder->aac_fmt_flag;
    aidlDecoder.objectType = legacyDecoder->audio_obj_type;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.sizeOfPCEBits = legacyDecoder->total_size_of_PCE_bits;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    ALOGV("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionFlacDec LegacyToAidl::convertFlacCompressDecoderToAidl(
        struct agm_session_flac_dec *legacyDecoder) {
    AgmSessionFlacDec aidlDecoder;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.sampleSize = legacyDecoder->sample_size;
    aidlDecoder.minBlockSize = legacyDecoder->min_blk_size;
    aidlDecoder.maxBlockSize = legacyDecoder->max_blk_size;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    aidlDecoder.minFrameSize = legacyDecoder->min_frame_size;
    aidlDecoder.maxFrameSize = legacyDecoder->max_frame_size;
    ALOGV("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionAlacDec LegacyToAidl::convertAlacCompressDecoderToAidl(
        struct agm_session_alac_dec *legacyDecoder) {
    AgmSessionAlacDec aidlDecoder;
    aidlDecoder.frameLength = legacyDecoder->frame_length;
    aidlDecoder.compatibleVersion = legacyDecoder->compatible_version;
    aidlDecoder.bitDepth = legacyDecoder->bit_depth;
    aidlDecoder.pb = legacyDecoder->pb;
    aidlDecoder.mb = legacyDecoder->mb;
    aidlDecoder.kb = legacyDecoder->kb;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.maxRun = legacyDecoder->max_run;
    aidlDecoder.maxFrameBytes = legacyDecoder->max_frame_bytes;
    aidlDecoder.averageBitRate = legacyDecoder->avg_bit_rate;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    aidlDecoder.channelLayoutTag = legacyDecoder->channel_layout_tag;
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionApeDec LegacyToAidl::convertApeCompressDecoderToAidl(
        struct agm_session_ape_dec *legacyDecoder) {
    AgmSessionApeDec aidlDecoder;
    aidlDecoder.compatibleVersion = legacyDecoder->compatible_version;
    aidlDecoder.compressionLevel = legacyDecoder->compression_level;
    aidlDecoder.formatFlags = legacyDecoder->format_flags;
    aidlDecoder.blocksPerFrame = legacyDecoder->blocks_per_frame;
    aidlDecoder.finalFrameBlocks = legacyDecoder->final_frame_blocks;
    aidlDecoder.totalFrames = legacyDecoder->total_frames;
    aidlDecoder.bitWidth = legacyDecoder->bit_width;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    aidlDecoder.seekTablePresent = legacyDecoder->seek_table_present;
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionWmaproDec LegacyToAidl::convertWmaProCompressDecoderToAidl(
        struct agm_session_wmapro_dec *legacyDecoder) {
    AgmSessionWmaproDec aidlDecoder;
    aidlDecoder.formatTag = legacyDecoder->fmt_tag;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    aidlDecoder.averageBytesPerSecond = legacyDecoder->avg_bytes_per_sec;
    aidlDecoder.blockAlign = legacyDecoder->blk_align;
    aidlDecoder.bitsPerSample = legacyDecoder->bits_per_sample;
    aidlDecoder.channelMask = legacyDecoder->channel_mask;
    aidlDecoder.encoderOption = legacyDecoder->enc_options;
    aidlDecoder.advancedEncoderOption = legacyDecoder->advanced_enc_option;
    aidlDecoder.advancedEncoderOption2 = legacyDecoder->advanced_enc_option2;
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionWmaDec LegacyToAidl::convertWmaStandardCompressDecoderToAidl(
        struct agm_session_wma_dec *legacyDecoder) {
    AgmSessionWmaDec aidlDecoder;
    aidlDecoder.formatTag = legacyDecoder->fmt_tag;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    aidlDecoder.averageBytesPerSecond = legacyDecoder->avg_bytes_per_sec;
    aidlDecoder.blockAlign = legacyDecoder->blk_align;
    aidlDecoder.bitsPerSample = legacyDecoder->bits_per_sample;
    aidlDecoder.channelMask = legacyDecoder->channel_mask;
    aidlDecoder.encoderOption = legacyDecoder->enc_options;
    aidlDecoder.reserved = legacyDecoder->reserved;
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionOpusDec LegacyToAidl::convertOpusCompressDecoderToAidl(
        struct agm_session_opus_dec *legacyDecoder) {
    AgmSessionOpusDec aidlDecoder;
    aidlDecoder.bitStreamFormat = legacyDecoder->bitstream_format;
    aidlDecoder.type = legacyDecoder->payload_type;
    aidlDecoder.version = legacyDecoder->version;
    aidlDecoder.channels = legacyDecoder->num_channels;
    aidlDecoder.preSkip = legacyDecoder->pre_skip;
    aidlDecoder.sampleRate = legacyDecoder->sample_rate;
    aidlDecoder.mappingFamily = legacyDecoder->mapping_family;
    aidlDecoder.streamCount = legacyDecoder->stream_count;
    // both types are uint8 type with predefined size, so use hardcoded values
    memcpy(aidlDecoder.channelMap.data(), legacyDecoder->channel_map, 8);
    memcpy(aidlDecoder.reserved.data(), legacyDecoder->reserved, 3);
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
    return std::move(aidlDecoder);
}

AgmSessionAacEnc LegacyToAidl::convertAacCompressEncoderToAidl(
        struct agm_session_aac_enc *legacyEncoder) {
    AgmSessionAacEnc aidlEncoder;
    aidlEncoder.bitRate = legacyEncoder->aac_bit_rate;
    aidlEncoder.globalCutOffFrequency = legacyEncoder->global_cutoff_freq;
    aidlEncoder.mode = legacyEncoder->enc_cfg.aac_enc_mode;
    aidlEncoder.formatFlags = legacyEncoder->enc_cfg.aac_fmt_flag;
    ALOGI("%s config %s", __func__, aidlEncoder.toString().c_str());
    return std::move(aidlEncoder);
}

AgmSessionCodec LegacyToAidl::convertCompressDecoderInfoToAidl(
        union agm_session_codec *sessionCodec, agm_media_format format) {
    AgmSessionCodec codec;
    switch (format) {
        case AGM_FORMAT_AAC: {
            auto aacDec = convertAacCompressDecoderToAidl(&sessionCodec->aac_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::aacDecoder>(aacDec);
            break;
        }
        case AGM_FORMAT_FLAC: {
            auto flacDec = convertFlacCompressDecoderToAidl(&sessionCodec->flac_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::flacDecoder>(flacDec);
            break;
        }
        case AGM_FORMAT_ALAC: {
            auto alacDec = convertAlacCompressDecoderToAidl(&sessionCodec->alac_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::alacDecoder>(alacDec);
            break;
        }
        case AGM_FORMAT_APE: {
            auto apeDec = convertApeCompressDecoderToAidl(&sessionCodec->ape_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::apeDecoder>(apeDec);
            break;
        }
        case AGM_FORMAT_WMAPRO: {
            auto wmaproDec = convertWmaProCompressDecoderToAidl(&sessionCodec->wmapro_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::wmaproDecoder>(wmaproDec);
            break;
        }
        case AGM_FORMAT_WMASTD: {
            auto wmaDec = convertWmaStandardCompressDecoderToAidl(&sessionCodec->wma_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::wmaDecoder>(wmaDec);
            break;
        }
        case AGM_FORMAT_OPUS: {
            auto opusDec = convertOpusCompressDecoderToAidl(&sessionCodec->opus_dec);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::opusDecoder>(opusDec);
            break;
        }
        default:
            break;
    }
    return codec;
}

AgmSessionCodec LegacyToAidl::convertCompressEncoderInfoToAidl(
        union agm_session_codec *sessionCodec, agm_media_format format) {
    AgmSessionCodec codec;
    switch (format) {
        case AGM_FORMAT_AAC: {
            auto enc = convertAacCompressEncoderToAidl(&sessionCodec->aac_enc);
            codec = AgmSessionCodec::make<AgmSessionCodec::Tag::aacEncoder>(enc);
            break;
        }
        default:
            break;
    }
    return codec;
}

AgmSessionCodec LegacyToAidl::convertCompressCodecInfoToAidl(
        struct agm_session_config *legacyConfig, agm_media_format format) {
    if (legacyConfig->dir == RX) {
        return convertCompressDecoderInfoToAidl(&legacyConfig->codec, format);
    } else if (legacyConfig->dir == TX) {
        return convertCompressEncoderInfoToAidl(&legacyConfig->codec, format);
    }
    return {};
}

AgmSessionConfig LegacyToAidl::convertAgmSessionConfigToAidl(
        struct agm_session_config *legacyConfig, agm_media_format format) {
    AgmSessionConfig aidlConfig;
    aidlConfig.direction = static_cast<Direction>(legacyConfig->dir);
    aidlConfig.sessionMode = static_cast<AgmSessionMode>(legacyConfig->sess_mode);
    aidlConfig.startThreshold = legacyConfig->start_threshold;
    aidlConfig.stopThreshold = legacyConfig->stop_threshold;

    // only for compressed formats having configs create configs
    if (needsCodecSpecificInfo(format)) {
        aidlConfig.codec = convertCompressCodecInfoToAidl(legacyConfig, format);
    }
    aidlConfig.dataMode = static_cast<AgmDataMode>(legacyConfig->data_mode);
    aidlConfig.flags = legacyConfig->sess_flags;
    return aidlConfig;
}

AgmMediaConfig LegacyToAidl::convertAgmMediaConfigToAidl(struct agm_media_config *legacyConfig) {
    AgmMediaConfig aidlConfig;
    aidlConfig.rate = legacyConfig->rate;
    aidlConfig.channels = legacyConfig->channels;
    aidlConfig.format = static_cast<AgmMediaFormat>(legacyConfig->format);
    aidlConfig.dataFormat = legacyConfig->data_format;
    return aidlConfig;
}

AgmBufferConfig LegacyToAidl::convertAgmBufferConfigToAidl(
        struct agm_buffer_config *buffer_config) {
    AgmBufferConfig aidlConfig;
    aidlConfig.count = buffer_config->count;
    aidlConfig.size = buffer_config->size;
    aidlConfig.maxMetadataSize = buffer_config->max_metadata_size;
    return aidlConfig;
}

AgmGroupMediaConfig LegacyToAidl::convertAgmGroupMediaConfigToAidl(
        struct agm_group_media_config *legacyConfig) {
    AgmGroupMediaConfig aidlConfig;
    aidlConfig.rate = legacyConfig->config.rate;
    aidlConfig.channels = legacyConfig->config.channels;
    aidlConfig.format =
            static_cast<AgmMediaFormat>(legacyConfig->config.format); // return value check?? TODO
    aidlConfig.dataFormat = legacyConfig->config.data_format;
    aidlConfig.slotMask = legacyConfig->slot_mask;
    return aidlConfig;
}

AgmTagConfig LegacyToAidl::convertAgmTagConfigToAidl(struct agm_tag_config *legacyConfig) {
    AgmTagConfig aidlConfig;
    aidlConfig.tag = legacyConfig->tag;
    aidlConfig.kv.resize(legacyConfig->num_tkvs);
    for (unsigned long i = 0; i < legacyConfig->num_tkvs; i++) {
        aidlConfig.kv[i].key = legacyConfig->kv[i].key;
        aidlConfig.kv[i].value = legacyConfig->kv[i].value;
    }
    return aidlConfig;
}

AgmCalConfig LegacyToAidl::convertAgmCalConfigToAidl(struct agm_cal_config *legacyConfig) {
    AgmCalConfig aidlConfig;
    aidlConfig.kv.resize(legacyConfig->num_ckvs);
    for (unsigned long i = 0; i < legacyConfig->num_ckvs; i++) {
        aidlConfig.kv[i].key = legacyConfig->kv[i].key;
        aidlConfig.kv[i].value = legacyConfig->kv[i].value;
    }
    return aidlConfig;
}

AgmEventRegistrationConfig LegacyToAidl::convertAgmEventRegistrationConfigToAidl(
        agm_event_reg_cfg *legacyConfig) {
    AgmEventRegistrationConfig aidlConfig;
    aidlConfig.moduleInstanceId = legacyConfig->module_instance_id;
    aidlConfig.eventId = legacyConfig->event_id;
    aidlConfig.registerEvent = legacyConfig->is_register;
    aidlConfig.eventConfigPayload.resize(legacyConfig->event_config_payload_size);
    memcpy(aidlConfig.eventConfigPayload.data(), legacyConfig->event_config_payload,
           legacyConfig->event_config_payload_size);
    return aidlConfig;
}

AgmEventCallbackParameter LegacyToAidl::convertAgmEventCallbackParametersToAidl(
        struct agm_event_cb_params *legacyParams) {
    AgmEventCallbackParameter aidlParams;
    aidlParams.sourceModuleId = legacyParams->source_module_id;
    aidlParams.eventId = legacyParams->event_id;
    aidlParams.eventPayload.resize(legacyParams->event_payload_size);

    memcpy(aidlParams.eventPayload.data(), legacyParams->event_payload,
           legacyParams->event_payload_size);
    return aidlParams;
}

std::vector<uint8_t> LegacyToAidl::convertRawPayloadToVector(void *payload, size_t size) {
    std::vector<uint8_t> aidlPayload(size);
    memcpy(aidlPayload.data(), payload, size);
    return aidlPayload;
}

/*
* if intToCopy is -1, don't copy, otherwise emplace the int.
*/

aidl::android::hardware::common::NativeHandle fdToNativeHandle(int fd, int intToCopy = -1) {
    aidl::android::hardware::common::NativeHandle handle;
    handle.fds.emplace_back(dup(fd));
    if (intToCopy != -1) handle.ints.emplace_back(intToCopy);
    return std::move(handle);
}

AgmBuff LegacyToAidl::convertAgmBufferToAidl(struct agm_buff *legacyBuffer, bool externalMemory,
                                             bool copyBuffers) {
    AgmBuff aidlBuffer;
    aidlBuffer.timestamp = legacyBuffer->timestamp;
    aidlBuffer.flags = legacyBuffer->flags;
    aidlBuffer.size = legacyBuffer->size;
    aidlBuffer.buffer.resize(legacyBuffer->size);

    if (copyBuffers) {
        if (legacyBuffer->size && legacyBuffer->addr) {
            memcpy(aidlBuffer.buffer.data(), legacyBuffer->addr, legacyBuffer->size);
        } else {
            ALOGE("%s: buf size or addr is null", __func__);
            return {};
        }
    }

    if (externalMemory) {
        aidlBuffer.metadata.resize(legacyBuffer->metadata_size);
        if ((legacyBuffer->metadata_size > 0) && legacyBuffer->metadata) {
            memcpy(aidlBuffer.metadata.data(), legacyBuffer->metadata, legacyBuffer->metadata_size);
        }

        aidlBuffer.externalAllocInfo.allocHandle = fdToNativeHandle(
                legacyBuffer->alloc_info.alloc_handle, legacyBuffer->alloc_info.alloc_handle);
        aidlBuffer.externalAllocInfo.allocatedSize = legacyBuffer->alloc_info.alloc_size;
        aidlBuffer.externalAllocInfo.offset = legacyBuffer->alloc_info.offset;
    }
    return std::move(aidlBuffer);
}

int LegacyToAidl::getDupedFdFromAgmEventParams(struct agm_event_cb_params *eventParams) {
    struct gsl_event_read_write_done_payload *gslReadWritePayload =
            (struct gsl_event_read_write_done_payload *)eventParams->event_payload;
    return gslReadWritePayload->buff.alloc_info.alloc_handle;
}

void LegacyToAidl::cleanUpMetadataMemory(struct agm_event_cb_params *eventParams) {
    struct gsl_event_read_write_done_payload *gslReadWritePayload =
            (struct gsl_event_read_write_done_payload *)eventParams->event_payload;
    if (gslReadWritePayload->buff.metadata) {
        free(gslReadWritePayload->buff.metadata);
    }
}

AgmReadWriteEventCallbackParams LegacyToAidl::convertAgmReadWriteEventCallbackParamsToAidl(
        struct agm_event_cb_params *eventParams, int inputFd) {
    struct gsl_event_read_write_done_payload *gslReadWritePayload =
            (struct gsl_event_read_write_done_payload *)eventParams->event_payload;

    AgmBuff agmBuffer;
    agmBuffer.timestamp = gslReadWritePayload->buff.timestamp;
    agmBuffer.flags = gslReadWritePayload->buff.flags;
    agmBuffer.size = gslReadWritePayload->buff.size;

    agmBuffer.externalAllocInfo.allocHandle =
            fdToNativeHandle(gslReadWritePayload->buff.alloc_info.alloc_handle, inputFd);
    agmBuffer.externalAllocInfo.allocatedSize = gslReadWritePayload->buff.alloc_info.alloc_size;
    agmBuffer.externalAllocInfo.offset = gslReadWritePayload->buff.alloc_info.offset;

    if (gslReadWritePayload->buff.metadata_size > 0) {
        agmBuffer.metadata.resize(gslReadWritePayload->buff.metadata_size);
        memcpy(agmBuffer.metadata.data(), gslReadWritePayload->buff.metadata,
               gslReadWritePayload->buff.metadata_size);
    }

    AgmEventReadWriteDonePayload readWritePayload;
    readWritePayload.tag = gslReadWritePayload->tag;
    readWritePayload.status = gslReadWritePayload->status;
    readWritePayload.metadataStatus = gslReadWritePayload->md_status;
    readWritePayload.buffer = std::move(agmBuffer);

    AgmReadWriteEventCallbackParams aidlReadWriteDoneParams;
    aidlReadWriteDoneParams.sourceModuleId = eventParams->source_module_id;
    aidlReadWriteDoneParams.eventId = eventParams->event_id;
    aidlReadWriteDoneParams.payload = std::move(readWritePayload);
    return std::move(aidlReadWriteDoneParams);
}

void LegacyToAidl::convertMmapBufferInfoToAidl(struct agm_buf_info *agmLegacyBufferInfo,
                                               MmapBufInfo *aidlBufferInfo, int flags) {
    if (flags & DATA_BUF) {
        ALOGV("%s data fd %d size %d", __func__, agmLegacyBufferInfo->data_buf_fd,
              agmLegacyBufferInfo->data_buf_size);
        aidlBufferInfo->dataFdHandle = fdToNativeHandle(agmLegacyBufferInfo->data_buf_fd);
        aidlBufferInfo->dataSize = agmLegacyBufferInfo->data_buf_size;
    }
    if (flags & POS_BUF) {
        ALOGV("%s pos fd %d size %d ", __func__, agmLegacyBufferInfo->pos_buf_fd,
              agmLegacyBufferInfo->pos_buf_size);
        aidlBufferInfo->positionFdHandle = fdToNativeHandle(agmLegacyBufferInfo->pos_buf_fd);
        aidlBufferInfo->posSize = agmLegacyBufferInfo->pos_buf_size;
    }
}

std::vector<AifInfo> LegacyToAidl::convertAifInfoListToAidl(struct aif_info *legacyList,
                                                            int listSize) {
    std::vector<AifInfo> aidlList;
    aidlList.resize(listSize);
    if (legacyList != NULL) {
        for (unsigned long i = 0; i < aidlList.size(); i++) {
            aidlList[i].aifName = legacyList[i].aif_name;
            aidlList[i].direction = (Direction)legacyList[i].dir;
        }
    }
    return std::move(aidlList);
}
