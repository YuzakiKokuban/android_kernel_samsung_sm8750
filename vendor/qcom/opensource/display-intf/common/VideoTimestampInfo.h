// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_VIDEOTIMESTAMPINFO_H__
#define __COMMON_VIDEOTIMESTAMPINFO_H__

#include <cstdint>

#define VIDEO_TIMESTAMP_INFO_SIZE 16

typedef struct vendor_qti_hardware_display_common_VideoTimestampInfo {
  uint32_t enable;            /* Enable video timestamp info */
  uint32_t frame_number;      /* Frame number/counter */
  int64_t frame_timestamp_us; /* Frame timestamp in us */
} vendor_qti_hardware_display_common_VideoTimestampInfo;

#endif  // __COMMON_VIDEOTIMESTAMPINFO_H__
