// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTIMASTERINGDISPLAY_H__
#define __COMMON_QTIMASTERINGDISPLAY_H__

#include <cstdint>

#include "XyColor.h"

/*
 * Mastering display metadata.
 */
typedef struct vendor_qti_hardware_display_common_QtiMasteringDisplay {
  /**
   * Color volume SEI (Supplement Enhancement Information).
   * Indicates if QtiMasterDisplay info should be used.
   */
  bool colorVolumeSEIEnabled;
  /**
   * Chromaticity for red in the RGB primaries (unit 1/50000).
   */
  vendor_qti_hardware_display_common_XyColor primaryRed;
  /**
   * Chromaticity for green in the RGB primaries (unit 1/50000).
   */
  vendor_qti_hardware_display_common_XyColor primaryGreen;
  /**
   * Chromaticity for blue in the RGB primaries (unit 1/50000).
   */
  vendor_qti_hardware_display_common_XyColor primaryBlue;
  /**
   * Chromaticity for the white point (unit 1/50000).
   */
  vendor_qti_hardware_display_common_XyColor whitePoint;
  /**
   * Maximum luminance in candelas per square meter.
   */
  uint32_t maxDisplayLuminance;
  /**
   * Minimum luminance in 1/10000 candelas per square meter.
   */
  uint32_t minDisplayLuminance;
} vendor_qti_hardware_display_common_QtiMasteringDisplay;

#endif  // __COMMON_QTIMASTERINGDISPLAY_H__
