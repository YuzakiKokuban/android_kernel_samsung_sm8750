// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_VIDEOTRANSCODESTATSMETADATA_H__
#define __COMMON_VIDEOTRANSCODESTATSMETADATA_H__

#include <cstdint>

#define QTI_VIDEO_TRANSCODE_PAYLOAD_NUM 32
#define QTI_VIDEO_TRANSCODE_STATS_SIZE (QTI_VIDEO_TRANSCODE_PAYLOAD_NUM * 4)

typedef struct vendor_qti_hardware_display_common_VideoTranscodeStatsMetadata {
  uint32_t stats_info[QTI_VIDEO_TRANSCODE_PAYLOAD_NUM]; /* Transcode stats payload */
  uint32_t stat_len;                                    /* Full payload size in bytes */
} vendor_qti_hardware_display_common_VideoTranscodeStatsMetadata;

#endif  // __COMMON_VIDEOTRANSCODESTATSMETADATA_H__
