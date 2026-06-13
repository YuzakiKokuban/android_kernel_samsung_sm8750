// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTIANAMORPHICMETADATA_H__
#define __COMMON_QTIANAMORPHICMETADATA_H__

#include <cstdint>
#include <cfloat>

typedef struct vendor_qti_hardware_display_common_QtiAnamorphicMetadataPerEye {
  double leftCR = 0;            /* Horizontal left compression ratio. Needs to be > 1 */
  double rightCR = 0;           /* Horizontal right compression ratio. Needs to be > 1 */
  double topCR = 0;             /* Vertical top compression ratio. Needs to be > 1 */
  double bottomCR = 0;          /* Vertical bottom compression ratio. Needs to be > 1 */
  uint32_t foveaX = 0;          /* X position of the fovea region */
  uint32_t foveaY = 0;          /* Y position of the fovea region */
  uint32_t foveaWidth = 0;      /* Width of the fovea region */
  uint32_t foveaHeight = 0;     /* Height of the fovea region */
} vendor_qti_hardware_display_common_QtiAnamorphicMetadataPerEye;


typedef struct vendor_qti_hardware_display_common_QtiAnamorphicMetadata {
  bool leftEyeDataValid = false;
  bool rightEyeDataValid = false;
  vendor_qti_hardware_display_common_QtiAnamorphicMetadataPerEye leftEyeParams;    /* Left Eye */
  vendor_qti_hardware_display_common_QtiAnamorphicMetadataPerEye rightEyeParams;   /* Right Eye */
} vendor_qti_hardware_display_common_QtiAnamorphicMetadata;

#endif  // __COMMON_QTIANAMORPHICMETADATA_H___
