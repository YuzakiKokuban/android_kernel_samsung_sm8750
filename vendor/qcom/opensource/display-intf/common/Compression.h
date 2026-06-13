// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_COMPRESSION_H__
#define __COMMON_COMPRESSION_H__

typedef enum vendor_qti_hardware_display_common_Compression {
  COMPRESSION_NONE = 0,
  DISPLAY_STREAM_COMPRESSION = 1,
  QTI_COMPRESSION_UBWC = 10,
  QTI_COMPRESSION_UBWC_LOSSY_8_TO_5 = 11,
  QTI_COMPRESSION_UBWC_LOSSY_2_TO_1 = 12,
  COMPRESSION_MAX = 0xFF,
} vendor_qti_hardware_display_common_Compression;

#endif  // __COMMON_COMPRESSION_H__
