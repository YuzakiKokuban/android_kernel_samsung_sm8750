/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef METADATA_PARSER_H
#define METADATA_PARSER_H

#include <chrono>
#include <memory>
#include <numeric>
#include <thread>
#include <utility>
#include <vector>
#include <unordered_map>

#include "media_fmt_api_basic.h"
#include "metadata_api.h"
#include "wr_sh_mem_ep_api.h"
#include "rd_sh_mem_ep_api.h"

#define ALIGN(num, to) (((num) + (to-1)) & (~(to-1)))

static constexpr uint32_t BYTES_PER_SAMPLE(uint32_t x) { return (x / 8); }
static constexpr uint32_t GET_LSW(uint64_t num) { return static_cast<uint32_t>(num & UINT32_MAX); }
static constexpr uint32_t GET_MSW(uint64_t num) { return static_cast<uint32_t>((num & ~UINT32_MAX) >> 32); }

struct ChannelHelper {
    static constexpr const int MAX_NUM_CHANNELS = 16;
};

enum MetadataType: uint8_t {
    START_METADATA = 0,
    END_METADATA,
    MEDIA_FORMAT_EVENT
};

size_t START_METADATA_SIZE() {
    return sizeof(metadata_header_t) + sizeof(module_cmn_md_buffer_start_t);
}

size_t END_METADATA_SIZE() {
    return sizeof(metadata_header_t) + sizeof(module_cmn_md_buffer_end_t);
}

size_t MEDIA_FORMAT_METADATA_SIZE() {
    return sizeof(metadata_header_t)
            + sizeof(struct media_format_t)
            + sizeof(payload_media_fmt_pcm_t)
            + (ChannelHelper::MAX_NUM_CHANNELS*sizeof(int8_t));
}

//update applicable lists for new metadata item added above
static std::vector<size_t> WRITE_METADATA_ITEM_SIZES = {
      START_METADATA_SIZE(),
      END_METADATA_SIZE()
};

static std::vector<size_t> READ_METADATA_ITEM_SIZES = {
      START_METADATA_SIZE(),
      END_METADATA_SIZE(),
      MEDIA_FORMAT_METADATA_SIZE()
};

class MetadataParser {
  public:
    static size_t WRITE_METADATA_MAX_SIZE() {
        return std::accumulate(WRITE_METADATA_ITEM_SIZES.begin(),
                               WRITE_METADATA_ITEM_SIZES.end(), 0);
    }

    static size_t READ_METADATA_MAX_SIZE() {
        return std::accumulate(READ_METADATA_ITEM_SIZES.begin(),
                               READ_METADATA_ITEM_SIZES.end(), 0);
    }

    int parseMetadata(uint8_t* metadata, size_t metadataSize,
                      pal_clbk_buffer_info *bufferInfo);
    void fillMetaData(uint8_t *metadata,
                      uint64_t frameIndex, size_t filledLength,
                      pal_media_config *streamMediaConfig);
};

#endif
