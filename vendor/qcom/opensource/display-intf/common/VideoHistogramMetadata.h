// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_VIDEOHISTOGRAMMETADATA_H__
#define __COMMON_VIDEOHISTOGRAMMETADATA_H__

#include <cstdint>

#define QTI_VIDEO_HISTOGRAM_STATS_SIZE 4 * 1024

/**
 * Video histogram stats populated by video decoder.
 */
typedef struct vendor_qti_hardware_display_common_VideoHistogramMetadata {
  uint32_t stats_info[1024]; /* Video stats payload */
  uint32_t stat_len;         /* Payload size in bytes */
  uint32_t frame_type;       /* bit mask to indicate frame type */
  uint32_t display_width;
  uint32_t display_height;
  uint32_t decode_width;
  uint32_t decode_height;
  uint32_t reserved[12];
} vendor_qti_hardware_display_common_VideoHistogramMetadata;

#endif  // __COMMON_VIDEOHISTOGRAMMETADATA_H__
