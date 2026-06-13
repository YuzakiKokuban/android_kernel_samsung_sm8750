// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_PLANELAYOUTCOMPONENT_H__
#define __COMMON_PLANELAYOUTCOMPONENT_H__

#include "PlaneLayoutComponentType.h"

/**
 * PlaneLayoutComponent describes a subpixel, or channel of a pixel.
 * For example, RGB has 3 components  - red, green, and blue.
 */
typedef struct vendor_qti_hardware_display_common_PlaneLayoutComponent {
  vendor_qti_hardware_display_common_PlaneLayoutComponentType type;
  /**
   * Offsets in bits from start of plane to first instance of component.
   * This can be used with the plane offset to determine exact location
   * of the first instance of this commponent.
   */
  int offset_in_bits;
  /**
   * Size in bits of component.
   */
  int size_in_bits;
} vendor_qti_hardware_display_common_PlaneLayoutComponent;

#endif  // __COMMON_PLANELAYOUTCOMPONENT_H__
