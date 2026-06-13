// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTIMATRIXCOEFFICIENTS_H__
#define __COMMON_QTIMATRIXCOEFFICIENTS_H__

typedef enum vendor_qti_hardware_display_common_QtiMatrixCoEfficients {
  QtiMatrixCoEff_Identity = 0,
  QtiMatrixCoEff_BT709_5 = 1,
  /* Unspecified = 2, Reserved = 3 */
  QtiMatrixCoeff_FCC_73_682 = 4,
  QtiMatrixCoEff_BT601_6_625 = 5,
  QtiMatrixCoEff_BT601_6_525 = 6,
  QtiMatrixCoEff_SMPTE240M = 7,  // used with 601_525_Unadjusted
  QtiMatrixCoEff_YCgCo = 8,
  QtiMatrixCoEff_BT2020 = 9,
  QtiMatrixCoEff_BT2020Constant = 10,
  QtiMatrixCoEff_BT601_6_Unadjusted = 11,  // Used with BT601_625(KR=0.222, KB=0.071)
  QtiMatrixCoEff_DCIP3 = 12,
  QtiMatrixCoEff_Chroma_NonConstant = 13,
  QtiMatrixCoEff_Max = 0xffff,
} vendor_qti_hardware_display_common_QtiMatrixCoEfficients;

#endif  // __COMMON_QTIMATRIXCOEFFICIENTS_H__
