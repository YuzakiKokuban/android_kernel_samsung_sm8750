/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_TAG "PAL: MetadataParser"

#include <string>
#include <log/log.h>
#include <unistd.h>
#include "PalDefs.h"
#include "MetadataParser.h"

int MetadataParser::parseMetadata(uint8_t* metadata, size_t metadataSize,
                                  pal_clbk_buffer_info *bufferInfo) {
    size_t mdBytesRead = 0;

    if (!metadata || metadataSize < std::min(START_METADATA_SIZE(), END_METADATA_SIZE())) {
        //TODO: may not work for multiple frames/buffer
        ALOGE("%s: Metadata payload smaller than expected, bytes 0x%x, expected 0x%x",
               __func__, mdBytesRead, std::min(START_METADATA_SIZE(), END_METADATA_SIZE()));
        return -EINVAL;
    }

    while (mdBytesRead < metadataSize) {
        metadata_header_t* metadataItem =
            reinterpret_cast<metadata_header_t*>(metadata + mdBytesRead);
        if (!metadataItem) {
            return 0;
        }
        mdBytesRead += sizeof(metadata_header_t);

        switch (metadataItem->metadata_id) {
            //TODO: check if 2 start metadata/2 end metadata is present
            case MODULE_CMN_MD_ID_BUFFER_START: {
                size_t startMetadataPayloadSize = metadataItem->payload_size;
                if (mdBytesRead + startMetadataPayloadSize > metadataSize) {
                    ALOGE("%s: Metadata item payload size larger than advertized metadata size"
                            " metadata id 0x%x, mdBytesRead = 0x%x, item payload size 0x%x,"
                            " metadata size = 0x%x ", __func__, metadataItem->metadata_id,
                            mdBytesRead, startMetadataPayloadSize, metadataSize);
                    return -EINVAL;
                }
                module_cmn_md_buffer_start_t* startMetadata =
                    reinterpret_cast<module_cmn_md_buffer_start_t*>(metadata + mdBytesRead);
                if (!startMetadata) {
                    ALOGE("%s: Metadata start payload not found at offset 0x%x",
                           __func__, mdBytesRead);
                    return -EINVAL;
                }
                bufferInfo->frame_index = static_cast<uint64_t>((static_cast<uint64_t>(
                        startMetadata->buffer_index_msw) << 32) | startMetadata->buffer_index_lsw);
                ALOGV("%s: startMetadata frame_index %llu", __func__, bufferInfo->frame_index);
                mdBytesRead += sizeof(module_cmn_md_buffer_start_t);
                break;
            }
            case MODULE_CMN_MD_ID_BUFFER_END: {
                size_t endMetadataPayloadSize = metadataItem->payload_size;
                if (mdBytesRead + endMetadataPayloadSize > metadataSize) {
                    ALOGE("%s: Metadata item payload size larger than advertized metadata size,"
                          " metadata id 0x%x, mdBytesRead = 0x%x, item payload size 0x%x,"
                          " metadata size = 0x%x ", __func__, metadataItem->metadata_id,
                          mdBytesRead, endMetadataPayloadSize, static_cast<uint32_t>(metadataSize));
                    return -EINVAL;
                }
                module_cmn_md_buffer_end_t* endMetadata =
                    reinterpret_cast<module_cmn_md_buffer_end_t*>(metadata + mdBytesRead);
                if (!endMetadata) {
                    ALOGE("%s: Metadata end payload not found at offset 0x%x",
                          __func__, mdBytesRead);
                    return -EINVAL;
                }
                // TODO: compare previous input buffer index from start metadata,
                // treat different values as error
                bufferInfo->frame_index = static_cast<uint64_t>((static_cast<uint64_t>(
                            endMetadata->buffer_index_msw) << 32) | endMetadata->buffer_index_lsw);

                if (endMetadata->flags) {
                  ALOGV("%s: End Metdata Flags=0x%x", __func__, endMetadata->flags);
                  if (((endMetadata->flags & MD_END_PAYLOAD_FLAGS_BIT_MASK_ERROR_RECOVERY_DONE)
                          >> MD_END_PAYLOAD_FLAGS_SHIFT_ERROR_RECOVERY_DONE)
                        == MD_END_RESULT_ERROR_RECOVERY_DONE ) {
                      ALOGI("%s: Error detected in input buffer and recovery attempted", __func__);
                  } else if ( ((endMetadata->flags & MD_END_PAYLOAD_FLAGS_BIT_MASK_ERROR_RESULT)
                      >> MD_END_PAYLOAD_FLAGS_SHIFT_ERROR_RESULT) == MD_END_RESULT_FAILED ) {
                      ALOGI("%s: Non-recoverable error detected in input buffer", __func__);
                  }
                }
                mdBytesRead += sizeof(module_cmn_md_buffer_end_t);
                break;
            }
            case MODULE_CMN_MD_ID_MEDIA_FORMAT: {
              size_t mfMetadataPayloadSize = metadataItem->payload_size;
              if (mdBytesRead + mfMetadataPayloadSize > metadataSize){
                  ALOGE("%s: Metadata item payload size larger than advertized metadata size,"
                        " metadata id 0x%x, mdBytesRead = 0x%x, item payload size 0x%x,"
                        " metadata size = 0x%x ", __func__, metadataItem->metadata_id,
                        mdBytesRead, mfMetadataPayloadSize, metadataSize);
                  return -EINVAL;
              }
              media_format_t* mfPayload =
                  reinterpret_cast<media_format_t*>(metadata + mdBytesRead);
              mdBytesRead += sizeof(media_format_t);

              if (!mfPayload) {
                  ALOGE("%s: Media metadata payload not found at offset 0x%x",
                        __func__, mdBytesRead);
                  return -EINVAL;
              }
              if (mfPayload->fmt_id != MEDIA_FMT_ID_PCM) {
                ALOGE("%s: Format ID within Media metadata payload not PCM,"
                      " fmt_id=x%x, offset=0x%x", __func__, mfPayload->fmt_id,
                      (uint32_t)offsetof(media_format_t, fmt_id));
                return -EINVAL;
              }
              payload_media_fmt_pcm_t* pcmPayload =
                  reinterpret_cast<payload_media_fmt_pcm_t*>(metadata + mdBytesRead);
              bufferInfo->sample_rate = pcmPayload->sample_rate;
              bufferInfo->channel_count = pcmPayload->num_channels;
              ALOGI("%s: sample_rate=%u, channel_count=%u", __func__,
                        bufferInfo->sample_rate, bufferInfo->channel_count);
              mdBytesRead +=
                      ALIGN(sizeof(payload_media_fmt_pcm_t) +
                            pcmPayload->num_channels * sizeof(int8_t), 4);
              break;
            }
            default: {
                ALOGE("%s: Unknown Metadata marker found at offset 0x%x, Metadata ID=0x%x",
                         __func__, mdBytesRead, metadataItem->metadata_id);
                // increment bytes read
                mdBytesRead += metadataItem->payload_size;
                break;
            }
        }
        ALOGV("%s: mdBytesRead=%zu, metadataSize=%zu", __func__, mdBytesRead, metadataSize);
    } // end while

    return 0;
}

void MetadataParser::fillMetaData(uint8_t *metadata,
                  uint64_t frameIndex, size_t filledLength,
                  pal_media_config *streamMediaConfig) {
    bool isEncode = false;

    if (streamMediaConfig->aud_fmt_id == PAL_AUDIO_FMT_PCM_S8 ||
            streamMediaConfig->aud_fmt_id == PAL_AUDIO_FMT_PCM_S16_LE ||
            streamMediaConfig->aud_fmt_id == PAL_AUDIO_FMT_PCM_S24_LE ||
            streamMediaConfig->aud_fmt_id == PAL_AUDIO_FMT_PCM_S24_3LE ||
            streamMediaConfig->aud_fmt_id == PAL_AUDIO_FMT_PCM_S32_LE) {
        isEncode = true;
    }

    auto getOffsetForEndMetadata = [&](uint32_t bufSize)
    {
        uint32_t sampleSizePerCh =
            BYTES_PER_SAMPLE(streamMediaConfig->bit_width) *
                streamMediaConfig->ch_info.channels;
        return (isEncode) ? (bufSize / sampleSizePerCh) : bufSize;
    };

    // Fill start metadata
    metadata_header_t* startMetadata = reinterpret_cast<metadata_header_t*>(metadata);
    startMetadata->metadata_id = MODULE_CMN_MD_ID_BUFFER_START;
    startMetadata->flags = static_cast<uint32_t>(MD_HEADER_FLAGS_BUFFER_ASSOCIATED << 4);
    startMetadata->offset = 0;
    startMetadata->payload_size = sizeof(module_cmn_md_buffer_start_t);
    metadata += sizeof(metadata_header_t);
    module_cmn_md_buffer_start_t startMetadataPayload = {GET_LSW(frameIndex),
                                                         GET_MSW(frameIndex)};
    memcpy(reinterpret_cast<void*>(metadata),
           reinterpret_cast<const void*>(&startMetadataPayload),
           sizeof(module_cmn_md_buffer_start_t));
    metadata += startMetadata->payload_size;

    // Fill end metadata
    metadata_header_t* endMetadata = reinterpret_cast<metadata_header_t*>(metadata);
    endMetadata->metadata_id = MODULE_CMN_MD_ID_BUFFER_END;
    endMetadata->flags = static_cast<uint32_t>(MD_HEADER_FLAGS_BUFFER_ASSOCIATED << 4);
    endMetadata->offset = getOffsetForEndMetadata(filledLength);
    endMetadata->payload_size = sizeof(module_cmn_md_buffer_end_t);
    metadata += sizeof(metadata_header_t);
    module_cmn_md_buffer_end_t endMetadataPayload = {GET_LSW(frameIndex),
                                                     GET_MSW(frameIndex),
                                                     0};
    memcpy(reinterpret_cast<void*>(metadata),
           reinterpret_cast<const void*>(&endMetadataPayload),
           sizeof(module_cmn_md_buffer_end_t));
}
