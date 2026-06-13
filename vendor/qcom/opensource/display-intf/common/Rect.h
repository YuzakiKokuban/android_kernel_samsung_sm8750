// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_RECT_H__
#define __COMMON_RECT_H__

#include <cstdint>

/**
 * Position of a rectangle.
 */
typedef struct vendor_qti_hardware_display_common_Rect {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
} vendor_qti_hardware_display_common_Rect;

#endif  // __COMMON_RECT_H__
