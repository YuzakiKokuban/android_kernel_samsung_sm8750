// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTICONTENTLIGHTLEVEL_H__
#define __COMMON_QTICONTENTLIGHTLEVEL_H__

#include <cstdint>

typedef struct vendor_qti_hardware_display_common_QtiContentLightLevel {
  /**
   * Light level SEI (Supplement Enhancement Information).
   * Indicates if QtiContentLightLevel info should be used.
   */
  bool lightLevelSEIEnabled;
  uint32_t maxContentLightLevel;      /* Unit: candelas per square meter */
  uint32_t maxFrameAverageLightLevel; /* Unit: candelas per square meter */
} vendor_qti_hardware_display_common_QtiContentLightLevel;

#endif  // __COMMON_QTICONTENTLIGHTLEVEL_H__
