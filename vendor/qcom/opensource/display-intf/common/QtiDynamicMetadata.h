// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTIDYNAMICMETADATA_H__
#define __COMMON_QTIDYNAMICMETADATA_H__

#include <cstdint>

#define QTI_HDR_DYNAMIC_META_DATA_SZ 1024

/**
 * Dynamic HDR metadata.
 * This is not used in tone mapping until it has been set for the first time.
 */
typedef struct vendor_qti_hardware_display_common_QtiDynamicMetadata {
  bool dynamicMetaDataValid;
  uint32_t dynamicMetaDataLen;
  uint8_t dynamicMetaDataPayload[QTI_HDR_DYNAMIC_META_DATA_SZ];
} vendor_qti_hardware_display_common_QtiDynamicMetadata;

#endif  // __COMMON_QTIDYNAMICMETADATA_H__
