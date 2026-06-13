// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_KEYVALUEPAIR_H__
#define __COMMON_KEYVALUEPAIR_H__

#include <cstdint>
#include <string>

typedef struct vendor_qti_hardware_display_common_KeyValuePair {
  char key[256];
  uint64_t value;
} vendor_qti_hardware_display_common_KeyValuePair;

#endif  // __COMMON_KEYVALUEPAIR_H__
