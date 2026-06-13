// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTICOLORREMAPPINGINFO_H__
#define __COMMON_QTICOLORREMAPPINGINFO_H__

#include "QtiColorPrimaries.h"
#include "QtiGammaTransfer.h"
#include "QtiMatrixCoEfficients.h"

typedef struct vendor_qti_hardware_display_common_QtiColorRemappingInfo {
  bool criEnabled;
  uint32_t crId;
  uint32_t crCancelFlag;
  uint32_t crPersistenceFlag;
  uint32_t crVideoSignalInfoPresentFlag;
  uint32_t crRange;
  vendor_qti_hardware_display_common_QtiColorPrimaries crPrimaries;
  vendor_qti_hardware_display_common_QtiGammaTransfer crTransferFunction;
  vendor_qti_hardware_display_common_QtiMatrixCoEfficients crMatrixCoefficients;
  uint32_t crInputBitDepth;
  uint32_t crOutputBitDepth;
  uint32_t crPreLutNumValMinusOne[3];
  uint32_t crPreLutCodedValue[3 * 33];
  uint32_t crPreLutTargetValue[3 * 33];
  uint32_t crMatrixPresentFlag;
  uint32_t crLog2MatrixDenom;
  int32_t crCoefficients[3 * 3];
  uint32_t crPostLutNumValMinusOne[3];
  uint32_t crPostLutCodedValue[3 * 33];
  uint32_t crPostLutTargetValue[3 * 33];
} vendor_qti_hardware_display_common_QtiColorRemappingInfo;

#endif  // __COMMON_QTICOLORREMAPPINGINFO_H__
