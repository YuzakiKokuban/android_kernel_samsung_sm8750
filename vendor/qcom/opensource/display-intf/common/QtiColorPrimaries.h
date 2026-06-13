// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTICOLORPRIMARIES_H__
#define __COMMON_QTICOLORPRIMARIES_H__

// Based loosely on BT.2380 / H.265 specifications
typedef enum vendor_qti_hardware_display_common_QtiColorPrimaries {
  // Unused = 0;
  QtiColorPrimaries_BT709_5 = 1, /* ITU-R BT.709-5 or equivalent */
  // Unspecified = 2, Reserved = 3
  QtiColorPrimaries_BT470_6M = 4,     /* ITU-R BT.470-6 System M or equivalent */
  QtiColorPrimaries_BT601_6_625 = 5,  /* ITU-R BT.601-6 625 or equivalent */
  QtiColorPrimaries_BT601_6_525 = 6,  /* ITU-R BT.601-6 525 or equivalent */
  QtiColorPrimaries_SMPTE_240M = 7,   /* SMPTE_240M */
  QtiColorPrimaries_GenericFilm = 8,  /* Generic Film */
  QtiColorPrimaries_BT2020 = 9,       /* ITU-R BT.2020 or equivalent */
  QtiColorPrimaries_SMPTE_ST428 = 10, /* SMPTE_240M */
  QtiColorPrimaries_AdobeRGB = 11,    /* Adobe RGB */
  QtiColorPrimaries_DCIP3 = 12,       /* DCI-P3 */
  QtiColorPrimaries_EBU3213 = 22,     /* EBU 3213 */
  QtiColorPrimaries_Max = 0xffff,
} vendor_qti_hardware_display_common_QtiColorPrimaries;

#endif  // __COMMON_QTICOLORPRIMARIES_H__
