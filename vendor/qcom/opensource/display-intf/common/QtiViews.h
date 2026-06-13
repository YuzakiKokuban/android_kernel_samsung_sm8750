// Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_QTIVIEWS_H__
#define __COMMON_QTIVIEWS_H__

typedef enum vendor_qti_hardware_display_common_QtiViews {
  PRIV_VIEW_MASK_PRIMARY = 0x00000001,
  PRIV_VIEW_MASK_SECONDARY = 0x00000002,
  PRIV_VIEW_MASK_PRIMARY_DEPTH = 0x00000004,
  PRIV_VIEW_MASK_SECONDARY_DEPTH = 0x00000008,
} vendor_qti_hardware_display_common_QtiViews;

#endif  // __COMMON_QTIVIEWS_H__
