// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_UBWCVERSION_H__
#define __COMMON_UBWCVERSION_H__

typedef enum vendor_qti_hardware_display_common_UBWCVersion {
  UBWC_VERSION_UNUSED = 0,
  UBWC_VERSION_1_0 = 0x1,
  UBWC_VERSION_2_0 = 0x2,
  UBWC_VERSION_3_0 = 0x3,
  UBWC_VERSION_4_0 = 0x4,
  UBWC_VERSION_5_0 = 0x5,
  UBWC_VERSION_MAX = 0xFF,
} vendor_qti_hardware_display_common_UBWCVersion;

#endif  // __COMMON_UBWCVERSION_H__
