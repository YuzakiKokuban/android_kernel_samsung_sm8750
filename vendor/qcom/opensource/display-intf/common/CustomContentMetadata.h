// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_CUSTOMCONTENTMETADATA_H__
#define __COMMON_CUSTOMCONTENTMETADATA_H__

#include <cstdint>

#define QTI_CUSTOM_METADATA_SIZE_BYTES 1024 * 42

/*
 * Dynamic metadata separate from QtiDynamicMetadata.
 * Used in limited cases.
 */
typedef struct vendor_qti_hardware_display_common_CustomContentMetadata {
  uint64_t size;
  uint8_t metadataPayload[QTI_CUSTOM_METADATA_SIZE_BYTES];
} vendor_qti_hardware_display_common_CustomContentMetadata;

#endif  // __COMMON_CUSTOMCONTENTMETADATA_H__
