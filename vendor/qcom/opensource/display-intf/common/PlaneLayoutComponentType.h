// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_PLANELAYOUTCOMPONENTTYPE_H__
#define __COMMON_PLANELAYOUTCOMPONENTTYPE_H__

#include <string>
#include <unordered_map>

typedef enum vendor_qti_hardware_display_common_PlaneLayoutComponentType {
  /* Luma */
  PLANE_LAYOUT_COMPONENT_TYPE_Y = 1 << 0,
  /* Chroma blue */
  PLANE_LAYOUT_COMPONENT_TYPE_CB = 1 << 1,
  /* Chroma red */
  PLANE_LAYOUT_COMPONENT_TYPE_CR = 1 << 2,

  /* Red */
  PLANE_LAYOUT_COMPONENT_TYPE_R = 1 << 10,
  /* Green */
  PLANE_LAYOUT_COMPONENT_TYPE_G = 1 << 11,
  /* Blue */
  PLANE_LAYOUT_COMPONENT_TYPE_B = 1 << 12,

  /* Raw */
  PLANE_LAYOUT_COMPONENT_TYPE_RAW = 1 << 20,

  /* Blob */
  PLANE_LAYOUT_COMPONENT_TYPE_BLOB = 1 << 29,

  /* Alpha */
  PLANE_LAYOUT_COMPONENT_TYPE_A = 1 << 30,

  /* Meta */
  PLANE_LAYOUT_COMPONENT_TYPE_META = 1 << 31,
} vendor_qti_hardware_display_common_PlaneLayoutComponentType;

#endif  // __COMMON_PLANELAYOUTCOMPONENTTYPE_H__
