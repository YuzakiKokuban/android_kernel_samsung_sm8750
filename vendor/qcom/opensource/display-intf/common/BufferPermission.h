// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_BUFFERPERMIISSION_H__
#define __COMMON_BUFFERPERMIISSION_H__

union vendor_qti_hardware_display_common_BufferPermission {
  struct {
    uint8_t read : 1;
    uint8_t write : 1;
    uint8_t execute : 1;
  };
  uint8_t permission;
};

#endif  // __COMMON_BUFFERPERMIISSION_H__
