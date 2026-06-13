// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_UBWCSTATS_H__
#define __COMMON_UBWCSTATS_H__

#include <cstdint>

#include "UBWCVersion.h"
#include "UBWC_CR_Stats.h"

#define QTI_UBWC_STATS_ARRAY_SIZE 2
#define QTI_MAX_UBWC_STATS_LENGTH 32

typedef struct vendor_qti_hardware_display_common_UBWCStats {
  vendor_qti_hardware_display_common_UBWCVersion version; /* Union depends on this version. */
  uint8_t bDataValid;                                     /* If [non-zero], CR Stats data is valid.
                              * Consumers may use stats data.
                              * If [zero], CR Stats data is invalid.
                              * Consumers *Shall* not use stats data */
  union {
    struct vendor_qti_hardware_display_common_UBWC_CR_Stats ubwc_stats;
    uint32_t reserved[QTI_MAX_UBWC_STATS_LENGTH]; /* This is for future */
  };
} vendor_qti_hardware_display_common_UBWCStats;

#endif  // __COMMON_UBWCSTATS_H__
