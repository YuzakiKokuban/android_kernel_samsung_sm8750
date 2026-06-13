// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_PLANELAYOUT_H__
#define __COMMON_PLANELAYOUT_H__

#include "PlaneLayoutComponent.h"

#define QTI_MAX_NUM_COMPONENTS 5

typedef struct vendor_qti_hardware_display_common_PlaneLayout {
  /**
   *  Offset of the plane, relative to the start of the buffer layout.
   */
  int offset_in_bytes;
  /**
   *  A sample contains all the components in a plane.
   *  For example, a buffer with semiplanar YUV format has
   *  one plane with a sample of Y and another plane with a sample of
   *  CbCr.
   *
   * The sample size is distance in bits between samples in the same row.
   */
  int sample_increment_bits;
  /**
   * Components describe the subpixel, or channels of a pixel.
   * This array contains the types of components in the plane.
   */
  vendor_qti_hardware_display_common_PlaneLayoutComponent components[QTI_MAX_NUM_COMPONENTS];
  /**
   * Number of components in the array.
   */
  int component_count;
  /**
   * The number of bytes between two consecutive rows.
   */
  int horizontal_stride_in_bytes;
  /**
   * The number of rows in a plane.
   */
  int scanlines;
  /**
   * Overall size in bytes of the plane, including padding.
   */
  int size_in_bytes;
  /**
   * Number of horizontally adjacent pixels using the same pixel data.
   * Must be a poisitive power of 2. A value of 1 indicates no subsampling.
   */
  int horizontal_subsampling;
  /**
   * Number of vertically adjacent pixels using the same pixel data.
   * Must be a poisitive power of 2. A value of 1 indicates no subsampling.
   */
  int vertical_subsampling;
} vendor_qti_hardware_display_common_PlaneLayout;

#endif  // __COMMON_PLANELAYOUT_H__
