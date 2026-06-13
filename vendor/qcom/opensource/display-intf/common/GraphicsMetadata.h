// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_GRAPHICSMETADATA_H__
#define __COMMON_GRAPHICSMETADATA_H__

#include <cstdint>

#define QTI_GRAPHICS_METADATA_SIZE 4096
#define QTI_GRAPHICS_METADATA_SIZE_BYTES = (GRAPHICS_METADATA_SIZE * sizeof(uint32_t));

/**
 * Graphics surface metadata.
 */
typedef struct vendor_qti_hardware_display_common_GraphicsMetadata {
  uint32_t size;  // TODO: get graphics to remove this to avoid "Metadata has bad signature" error
  uint32_t data[QTI_GRAPHICS_METADATA_SIZE];
} vendor_qti_hardware_display_common_GraphicsMetadata;

#endif  // __COMMON_GRAPHICSMETADATA_H__
