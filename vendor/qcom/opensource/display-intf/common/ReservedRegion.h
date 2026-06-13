// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_RESERVEDREGION_H__
#define __COMMON_RESERVEDREGION_H__

#include <cstdint>

#include "Address.h"

/*
 * Additional shared memory in buffer, outside of content and metadata,
 * for client use.
 */
typedef struct vendor_qti_hardware_display_common_ReservedRegion {
  uint32_t size;
  vendor_qti_hardware_display_common_Address reserved_region_addr;
} vendor_qti_hardware_display_common_ReservedRegion;

#endif  // __COMMON_RESERVEDREGION_H__
