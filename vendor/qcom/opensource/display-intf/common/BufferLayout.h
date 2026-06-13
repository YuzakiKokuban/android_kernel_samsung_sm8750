// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_BUFFERLAYOUT_H__
#define __COMMON_BUFFERLAYOUT_H__

#include "PlaneLayout.h"

#define QTI_MAX_NUM_PLANES 8

typedef struct vendor_qti_hardware_display_common_BufferLayout {
  /**
   * Layout for each plane
   */
  vendor_qti_hardware_display_common_PlaneLayout planes[QTI_MAX_NUM_PLANES];
  /**
   * Number of planes in the buffer
   */
  int plane_count;
  /**
   * Overall buffer size in bytes, including padding.
   */
  int size_in_bytes;
  /**
   * Bytes per pixel
   */
  int bpp;
  /**
   * Aligned width (in bytes) of buffer
   */
  int aligned_width_in_bytes;
  /**
   * Aligned height (in number of rows) of buffer
   */
  int aligned_height;
} vendor_qti_hardware_display_common_BufferLayout;

#endif  // __COMMON_BUFFERLAYOUT_H__
