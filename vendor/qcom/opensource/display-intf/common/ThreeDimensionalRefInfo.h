// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTITHREEDIMENSIONALREFINFO_H__
#define __COMMON_QTITHREEDIMENSIONALREFINFO_H__

#include <cstdint>

#define NUM_REF_DISPLAYS 32

typedef struct
    vendor_qti_hardware_display_common_ThreeDimensionalRefDisplayInfo {
  uint8_t left_view_id;
  uint8_t right_view_id;
  uint8_t exponent_ref_display_width;
  uint8_t mantissa_ref_display_width;
  uint8_t exponent_ref_viewing_distance;
  // Valid only if ref_viewing_distance_flag is 1
  uint8_t mantissa_ref_viewing_distance;
  // Valid only if additional_shift_present_flag is 1
  uint16_t num_sample_shift_plus512;
  uint8_t additional_shift_present_flag;
  uint8_t reserved[7]; // added for 64-bit alignment
} vendor_qti_hardware_display_common_ThreeDimensionalRefDisplayInfo;

typedef struct vendor_qti_hardware_display_common_ThreeDimensionalRefInfo {
  uint8_t prec_ref_display_width;
  uint8_t ref_viewing_distance_flag;
  uint8_t prec_ref_viewing_dist; // Valid only if ref_viewing_distance_flag is 1
  uint8_t num_ref_displays_minus1;
  struct vendor_qti_hardware_display_common_ThreeDimensionalRefDisplayInfo
      threedRefDispInfo[NUM_REF_DISPLAYS];
  uint8_t three_dimensional_reference_displays_extension_flag;
  uint8_t reserved[3]; // added for 64-bit alignment
} vendor_qti_hardware_display_common_ThreeDimensionalRefInfo;

#endif // __COMMON_QTITHREEDIMENSIONALREFINFO_H__