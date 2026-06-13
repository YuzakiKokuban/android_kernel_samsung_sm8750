// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_CVPMETADATA_H__
#define __COMMON_CVPMETADATA_H__

#include <cstdint>

#define QTI_CVP_METADATA_SIZE 1024

typedef enum vendor_qti_hardware_display_common_CVPMetadataFlags {
  /* bit wise flags */
  QTI_CVP_METADATA_FLAG_NONE = 0x00000000,
  QTI_CVP_METADATA_FLAG_REPEAT = 0x00000001,
} vendor_qti_hardware_display_common_CVPMetadataFlags;

typedef struct vendor_qti_hardware_display_common_CVPMetadata {
  /** Payload size in bytes */
  uint32_t size;
  uint8_t payload[QTI_CVP_METADATA_SIZE];
  uint32_t capture_frame_rate;
  /** Frame rate in Q16 format.
   *      Eg: fps = 7.5, then
   *      capture_frame_rate = 7 << 16 --> Upper 16 bits to represent 7
   *      capture_frame_rate |= 5 -------> Lower 16 bits to represent 5
   *
   *  If size > 0, framerate is valid
   *  If size = 0, invalid data, so ignore all parameters
   */
  uint32_t cvp_frame_rate;
  vendor_qti_hardware_display_common_CVPMetadataFlags flags;
  uint32_t reserved[8];
} vendor_qti_hardware_display_common_CVPMetadata;

#endif  // __COMMON_CVPMETADATA_H__
