// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_METADATASTATUS_H__
#define __COMMON_METADATASTATUS_H__

#include "MetadataType.h"

#define IS_VENDOR_METADATA_TYPE(x) (x >= QTI_VT_TIMESTAMP)

#define GET_STANDARD_METADATA_STATUS_INDEX(x) x
#define GET_VENDOR_METADATA_STATUS_INDEX(x) x - QTI_VT_TIMESTAMP

#define METADATA_SET_SIZE 512

typedef struct vendor_qti_hardware_display_common_MetadataStatus {
  /** isStandardMetadataSet will return true for a given
     *  vendor.qti.hardware.display.common.MetadataType if
     *  it has been explicitly set via ISnapMapper query.
     *  If it is false, the metadata has not been set
     *  and should be treated as a default value.
     */
  bool isStandardMetadataSet[METADATA_SET_SIZE];
  /**
     *  isVendorMetadataSet uses
     *  vendor.qti.hardware.display.common.MetadataType - 10000
     *  as an index.
     */
  bool isVendorMetadataSet[METADATA_SET_SIZE];
} vendor_qti_hardware_display_common_MetadataStatus;

#endif  // __COMMON_METADATASTATUS_H__
