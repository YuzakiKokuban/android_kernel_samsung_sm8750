// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_UBWCCRSTATS_H__
#define __COMMON_UBWCCRSTATS_H__

#include <cstdint>

typedef struct vendor_qti_hardware_display_common_UBWC_CR_Stats {
  uint32_t nCRStatsTile32;  /**< UBWC Stats info for  32 Byte Tile */
  uint32_t nCRStatsTile64;  /**< UBWC Stats info for  64 Byte Tile */
  uint32_t nCRStatsTile96;  /**< UBWC Stats info for  96 Byte Tile */
  uint32_t nCRStatsTile128; /**< UBWC Stats info for 128 Byte Tile */
  uint32_t nCRStatsTile160; /**< UBWC Stats info for 160 Byte Tile */
  uint32_t nCRStatsTile192; /**< UBWC Stats info for 192 Byte Tile */
  uint32_t nCRStatsTile256; /**< UBWC Stats info for 256 Byte Tile */
} vendor_qti_hardware_display_common_UBWC_CR_Stats;

#endif  // __COMMON_UBWCCRSTATS_H__
