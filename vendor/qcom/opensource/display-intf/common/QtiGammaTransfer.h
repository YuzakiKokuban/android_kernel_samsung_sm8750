// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTIGAMMATRANSFER_H__
#define __COMMON_QTIGAMMATRANSFER_H__

// Based on BT.2380 / H.265 specifications
typedef enum vendor_qti_hardware_display_common_QtiGammaTransfer {
  // Unused = 0;
  QtiTransfer_sRGB = 1,  // IEC 61966-2-1 sRGB
  /* Unspecified = 2, Reserved = 3 */
  QtiTransfer_Gamma2_2 = 4,
  QtiTransfer_Gamma2_8 = 5,
  QtiTransfer_SMPTE_170M = 6,  // BT.601-6 525 or 625
  QtiTransfer_SMPTE_240M = 7,  // SMPTE_240M
  QtiTransfer_Linear = 8,
  QtiTransfer_Log = 9,
  QtiTransfer_Log_Sqrt = 10,
  QtiTransfer_XvYCC = 11,         // IEC 61966-2-4
  QtiTransfer_BT1361 = 12,        // Rec.ITU-R BT.1361 extended gamut
  QtiTransfer_sYCC = 13,          // IEC 61966-2-1 sRGB or sYCC
  QtiTransfer_BT2020_2_1 = 14,    // Rec. ITU-R BT.2020-2 (same as the values 6, and 15)
  QtiTransfer_BT2020_2_2 = 15,    // Rec. ITU-R BT.2020-2 (same as the values 6, and 14)
  QtiTransfer_SMPTE_ST2084 = 16,  // 2084
  QtiTransfer_ST_428 = 17,        // SMPTE ST 428-1
  QtiTransfer_HLG = 18,           // ARIB STD-B67
  QtiTransfer_Max = 0xffff,
} vendor_qti_hardware_display_common_QtiGammaTransfer;

#endif  // __COMMON_QTIGAMMATRANSFER_H__
