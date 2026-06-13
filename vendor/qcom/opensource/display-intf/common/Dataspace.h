// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_DATASPACE_H__
#define __COMMON_DATASPACE_H__

#include <cstdint>

#include "QtiColorPrimaries.h"
#include "QtiColorRange.h"
#include "QtiGammaTransfer.h"

/**
 * Dataspace defines how color should be interpreted,
 * using three components - color primaries, range, and transfer.
 * See vendor.qti.hardware.display.common.QtiColorPrimaries,
 * vendor.qti.hardware.display.common.QtiColorRange, and
 * vendor.qti.hardware.display.common.QtiGammaTransfer for definitions.
 */
typedef struct vendor_qti_hardware_display_common_Dataspace {
  vendor_qti_hardware_display_common_QtiColorPrimaries colorPrimaries;
  vendor_qti_hardware_display_common_QtiColorRange range;
  vendor_qti_hardware_display_common_QtiGammaTransfer transfer;
} vendor_qti_hardware_display_common_Dataspace;

#endif  // __COMMON_DATASPACE_H__
