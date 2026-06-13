/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "AgmIpc::AidlToLegacy::Converter"

#include <agm/AgmAidlToLegacy.h>
#include <log/log.h>

namespace aidl::vendor::qti::hardware::agm {

void AidlToLegacy::convertCompressCodecInfo(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionConfig &aidlConfig,
        struct agm_session_config *legacyConfig) {
    if (aidlConfig.codec.has_value()) {
        auto aidlCodec = aidlConfig.codec.value();
        auto codecType = aidlConfig.codec.value().getTag();
        ALOGV("compress codec type %s ", toString(codecType).c_str());
        switch (codecType) {
            case AgmSessionCodec::aacDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::aacDecoder>();
                convertAacCompressDecoder(dec, &legacyConfig->codec.aac_dec);
                break;
            }
            case AgmSessionCodec::flacDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::flacDecoder>();
                convertFlacCompressDecoder(dec, &legacyConfig->codec.flac_dec);
                break;
            }
            case AgmSessionCodec::alacDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::alacDecoder>();
                convertAlacCompressDecoder(dec, &legacyConfig->codec.alac_dec);
                break;
            }
            case AgmSessionCodec::apeDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::apeDecoder>();
                convertApeCompressDecoder(dec, &legacyConfig->codec.ape_dec);
                break;
            }
            case AgmSessionCodec::wmaDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::wmaDecoder>();
                convertWmaStandardCompressDecoder(dec, &legacyConfig->codec.wma_dec);
                break;
            }
            case AgmSessionCodec::wmaproDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::wmaproDecoder>();
                convertWmaProCompressDecoder(dec, &legacyConfig->codec.wmapro_dec);
                break;
            }
            case AgmSessionCodec::opusDecoder: {
                auto dec = aidlCodec.get<AgmSessionCodec::opusDecoder>();
                convertOpusCompressDecoder(dec, &legacyConfig->codec.opus_dec);
                break;
            }
            case AgmSessionCodec::aacEncoder: {
                auto enc = aidlCodec.get<AgmSessionCodec::aacEncoder>();
                convertAacCompressEncoder(enc, &legacyConfig->codec.aac_enc);
                break;
            }
            default:
                ALOGE("unknow type %s ", toString(codecType).c_str());
        }
    }
}

void AidlToLegacy::convertAacCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionAacDec &aidlDecoder,
        struct agm_session_aac_dec *legacyDecoder) {
    legacyDecoder->aac_fmt_flag = aidlDecoder.formatFlag;
    legacyDecoder->audio_obj_type = aidlDecoder.objectType;
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->total_size_of_PCE_bits = aidlDecoder.sizeOfPCEBits;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
}

void AidlToLegacy::convertFlacCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionFlacDec &aidlDecoder,
        struct agm_session_flac_dec *legacyDecoder) {
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->sample_size = aidlDecoder.sampleSize;
    legacyDecoder->min_blk_size = aidlDecoder.minBlockSize;
    legacyDecoder->max_blk_size = aidlDecoder.maxBlockSize;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    legacyDecoder->min_frame_size = aidlDecoder.minFrameSize;
    legacyDecoder->max_frame_size = aidlDecoder.maxFrameSize;
    ALOGV("%s config %s", __func__, aidlDecoder.toString().c_str());
}

void AidlToLegacy::convertAlacCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionAlacDec &aidlDecoder,
        struct agm_session_alac_dec *legacyDecoder) {
    legacyDecoder->frame_length = aidlDecoder.frameLength;
    legacyDecoder->compatible_version = aidlDecoder.compatibleVersion;
    legacyDecoder->bit_depth = aidlDecoder.bitDepth;
    legacyDecoder->pb = aidlDecoder.pb;
    legacyDecoder->mb = aidlDecoder.mb;
    legacyDecoder->kb = aidlDecoder.kb;
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->max_run = aidlDecoder.maxRun;
    legacyDecoder->max_frame_bytes = aidlDecoder.maxFrameBytes;
    legacyDecoder->avg_bit_rate = aidlDecoder.averageBitRate;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    legacyDecoder->channel_layout_tag = aidlDecoder.channelLayoutTag;
    ALOGV("%s config %s", __func__, aidlDecoder.toString().c_str());
}
void AidlToLegacy::convertApeCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionApeDec &aidlDecoder,
        struct agm_session_ape_dec *legacyDecoder) {
    legacyDecoder->compatible_version = aidlDecoder.compatibleVersion;
    legacyDecoder->compression_level = aidlDecoder.compressionLevel;
    legacyDecoder->format_flags = aidlDecoder.formatFlags;
    legacyDecoder->blocks_per_frame = aidlDecoder.blocksPerFrame;
    legacyDecoder->final_frame_blocks = aidlDecoder.finalFrameBlocks;
    legacyDecoder->total_frames = aidlDecoder.totalFrames;
    legacyDecoder->bit_width = aidlDecoder.bitWidth;
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    legacyDecoder->seek_table_present = aidlDecoder.seekTablePresent;
    ALOGV("%s config %s", __func__, aidlDecoder.toString().c_str());
}

void AidlToLegacy::convertWmaStandardCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionWmaDec &aidlDecoder,
        struct agm_session_wma_dec *legacyDecoder) {
    legacyDecoder->fmt_tag = aidlDecoder.formatTag;
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    legacyDecoder->avg_bytes_per_sec = aidlDecoder.averageBytesPerSecond;
    legacyDecoder->blk_align = aidlDecoder.blockAlign;
    legacyDecoder->bits_per_sample = aidlDecoder.bitsPerSample;
    legacyDecoder->channel_mask = aidlDecoder.channelMask;
    legacyDecoder->enc_options = aidlDecoder.encoderOption;
    legacyDecoder->reserved = aidlDecoder.reserved;
    ALOGV("%s config %s", __func__, aidlDecoder.toString().c_str());
}

void AidlToLegacy::convertWmaProCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionWmaproDec &aidlDecoder,
        struct agm_session_wmapro_dec *legacyDecoder) {
    legacyDecoder->fmt_tag = aidlDecoder.formatTag;
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    legacyDecoder->avg_bytes_per_sec = aidlDecoder.averageBytesPerSecond;
    legacyDecoder->blk_align = aidlDecoder.blockAlign;
    legacyDecoder->bits_per_sample = aidlDecoder.bitsPerSample;
    legacyDecoder->channel_mask = aidlDecoder.channelMask;
    legacyDecoder->enc_options = aidlDecoder.encoderOption;
    legacyDecoder->advanced_enc_option = aidlDecoder.advancedEncoderOption;
    legacyDecoder->advanced_enc_option2 = aidlDecoder.advancedEncoderOption2;
    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
}

void AidlToLegacy::convertOpusCompressDecoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionOpusDec &aidlDecoder,
        struct agm_session_opus_dec *legacyDecoder) {
    legacyDecoder->bitstream_format = aidlDecoder.bitStreamFormat;
    legacyDecoder->payload_type = aidlDecoder.type;
    legacyDecoder->version = aidlDecoder.version;
    legacyDecoder->num_channels = aidlDecoder.channels;
    legacyDecoder->pre_skip = aidlDecoder.preSkip;
    legacyDecoder->sample_rate = aidlDecoder.sampleRate;
    legacyDecoder->mapping_family = aidlDecoder.mappingFamily;
    legacyDecoder->stream_count = aidlDecoder.streamCount;
    // both types are uint8 type with predefined size, so use hardcoded values
    memcpy(legacyDecoder->channel_map, aidlDecoder.channelMap.data(), 8);
    memcpy(legacyDecoder->reserved, aidlDecoder.reserved.data(), 3);

    ALOGI("%s config %s", __func__, aidlDecoder.toString().c_str());
}

void AidlToLegacy::convertAacCompressEncoder(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionAacEnc &aidlEncoder,
        struct agm_session_aac_enc *legacyEncoder) {
    legacyEncoder->aac_bit_rate = aidlEncoder.bitRate;
    legacyEncoder->global_cutoff_freq = aidlEncoder.globalCutOffFrequency;
    legacyEncoder->enc_cfg.aac_enc_mode = aidlEncoder.mode;
    legacyEncoder->enc_cfg.aac_fmt_flag = aidlEncoder.formatFlags;
    ALOGI("%s config %s", __func__, aidlEncoder.toString().c_str());
}

void AidlToLegacy::convertAgmMediaConfig(
        const ::aidl::vendor::qti::hardware::agm::AgmMediaConfig &aidlConfig,
        struct agm_media_config *legacyConfig) {
    legacyConfig->rate = aidlConfig.rate;
    legacyConfig->channels = aidlConfig.channels;
    // How to check the range of enums ??
    legacyConfig->format = static_cast<agm_media_format>(aidlConfig.format);
    legacyConfig->data_format = aidlConfig.dataFormat;
}

void AidlToLegacy::convertAgmGroupMediaConfig(
        const ::aidl::vendor::qti::hardware::agm::AgmGroupMediaConfig &aidlConfig,
        struct agm_group_media_config *legacyConfig) {
    legacyConfig->config.rate = aidlConfig.rate;
    legacyConfig->config.channels = aidlConfig.channels;
    legacyConfig->config.format = static_cast<agm_media_format>(aidlConfig.format);
    legacyConfig->config.data_format = aidlConfig.dataFormat;
    legacyConfig->slot_mask = aidlConfig.slotMask;
}

void AidlToLegacy::convertAgmSessionConfig(
        const ::aidl::vendor::qti::hardware::agm::AgmSessionConfig &aidlConfig,
        struct agm_session_config *legacyConfig) {
    // TODO check range of enum
    legacyConfig->dir = static_cast<direction>(aidlConfig.direction);
    legacyConfig->sess_mode = static_cast<agm_session_mode>(aidlConfig.sessionMode);
    legacyConfig->start_threshold = aidlConfig.startThreshold;
    legacyConfig->stop_threshold = aidlConfig.stopThreshold;
    legacyConfig->data_mode = static_cast<agm_data_mode>(aidlConfig.dataMode);
    legacyConfig->sess_flags = (aidlConfig.flags);
    convertCompressCodecInfo(aidlConfig, legacyConfig);
}

void AidlToLegacy::convertAgmBufferConfig(
        const ::aidl::vendor::qti::hardware::agm::AgmBufferConfig &aidlConfig,
        struct agm_buffer_config *legacyConfig) {
    legacyConfig->count = aidlConfig.count;
    legacyConfig->size = aidlConfig.size;
    legacyConfig->max_metadata_size = aidlConfig.maxMetadataSize;
}

void AidlToLegacy::convertAifInfoList(
        const std::vector<::aidl::vendor::qti::hardware::agm::AifInfo> &aidlList,
        struct aif_info *legacyAifList) {
    for (unsigned long i = 0; i < aidlList.size(); i++) {
        strlcpy(legacyAifList[i].aif_name, aidlList[i].aifName.c_str(), AIF_NAME_MAX_LEN);
        legacyAifList[i].dir = (enum direction)aidlList[i].direction;
    }
}

void AidlToLegacy::convertAgmEventRegistrationConfig(
        const ::aidl::vendor::qti::hardware::agm::AgmEventRegistrationConfig &aidlConfig,
        struct agm_event_reg_cfg *legacyConfig) {
    legacyConfig->module_instance_id = aidlConfig.moduleInstanceId;
    legacyConfig->event_id = aidlConfig.eventId;
    legacyConfig->is_register = aidlConfig.registerEvent;
    legacyConfig->event_config_payload_size = aidlConfig.eventConfigPayload.size();
    memcpy(legacyConfig->event_config_payload, aidlConfig.eventConfigPayload.data(),
           legacyConfig->event_config_payload_size);
}

std::pair<int, int> AidlToLegacy::getFdIntFromNativeHandle(
        const aidl::android::hardware::common::NativeHandle &nativeHandle, bool doDup) {
    std::pair<int, int> fdIntPair = {-1, -1};
    if (!nativeHandle.fds.empty()) {
        if (doDup) {
            fdIntPair.first = dup(nativeHandle.fds.at(0).get());
        } else {
            fdIntPair.first = (nativeHandle.fds.at(0).get());
        }
    }
    if (!nativeHandle.ints.empty()) {
        fdIntPair.second = nativeHandle.ints.at(0);
    }

    return std::move(fdIntPair);
}

int AidlToLegacy::getFdFromNativeHandle(
        const aidl::android::hardware::common::NativeHandle &nativeHandle) {
    if (!nativeHandle.fds.empty()) {
        return dup(nativeHandle.fds.at(0).get());
    }
    return -1;
}

void AidlToLegacy::convertAgmBuffer(const ::aidl::vendor::qti::hardware::agm::AgmBuff &aidlBuffer,
                                    struct agm_buff *agmLegacyBuffer) {
    uint32_t bufSize = aidlBuffer.buffer.size();
    agmLegacyBuffer->size = (size_t)bufSize;
    agmLegacyBuffer->timestamp = aidlBuffer.timestamp;
    agmLegacyBuffer->flags = aidlBuffer.flags;
    memcpy(agmLegacyBuffer->addr, aidlBuffer.buffer.data(), bufSize);
}

void AidlToLegacy::convertMmapBufInfo(const MmapBufInfo &aidlMmapBufferInfo,
                                      struct agm_buf_info *legacyAgmBufferInfo, uint32_t flag) {
    if (flag & DATA_BUF) {
        legacyAgmBufferInfo->data_buf_fd =
                AidlToLegacy::getFdFromNativeHandle(aidlMmapBufferInfo.dataFdHandle);
        legacyAgmBufferInfo->data_buf_size = aidlMmapBufferInfo.dataSize;
    }
    if (flag & POS_BUF) {
        legacyAgmBufferInfo->pos_buf_fd =
                AidlToLegacy::getFdFromNativeHandle(aidlMmapBufferInfo.positionFdHandle);
        legacyAgmBufferInfo->pos_buf_size = aidlMmapBufferInfo.posSize;
    }
}

void AidlToLegacy::convertAgmCalConfig(const AgmCalConfig &aidlCalConfig,
                                       struct agm_cal_config *agmLegacyCalConfig) {
    agmLegacyCalConfig->num_ckvs = aidlCalConfig.kv.size();
    for (unsigned long i = 0; i < aidlCalConfig.kv.size(); i++) {
        agmLegacyCalConfig->kv[i].key = aidlCalConfig.kv[i].key;
        agmLegacyCalConfig->kv[i].value = aidlCalConfig.kv[i].value;
    }
}

void AidlToLegacy::convertAgmTagConfig(const AgmTagConfig &aidlCalConfig,
                                       struct agm_tag_config *agmLegacyTagConfig) {
    agmLegacyTagConfig->num_tkvs = aidlCalConfig.kv.size();
    agmLegacyTagConfig->tag = aidlCalConfig.tag;
    for (unsigned long i = 0; i < aidlCalConfig.kv.size(); i++) {
        agmLegacyTagConfig->kv[i].key = aidlCalConfig.kv[i].key;
        agmLegacyTagConfig->kv[i].value = aidlCalConfig.kv[i].value;
    }
}
}