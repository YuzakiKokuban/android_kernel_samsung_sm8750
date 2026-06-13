// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_INTERLACED_H__
#define __COMMON_INTERLACED_H__

/**
 * Describes how buffer's planes are interlaced.
 */
typedef enum vendor_qti_hardware_display_common_Interlaced {
  INTERLACED_NONE = 0,
  /*
     * Horizontal interlacing
     * Height of interlaced plane = 1/2 of buffer height
     */
  TOP_BOTTOM = 1,
  /*
     * Vertical interlacing
     * Width of interlaced plane = 1/2 of buffer width
     */
  RIGHT_LEFT = 2,
  /**
     * Qualcomm interlaced definition
     */
  QTI_INTERLACED = 10,
} vendor_qti_hardware_display_common_Interlaced;

#endif  // __COMMON_INTERLACED_H__
