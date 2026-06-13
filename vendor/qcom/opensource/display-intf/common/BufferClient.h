// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_BUFFERCLIENT_H__
#define __COMMON_BUFFERCLIENT_H__

typedef enum vendor_qti_hardware_display_common_BufferClient {
  BUFFERCLIENT_INVALID = -1,
  BUFFERCLIENT_DPU = 0,
  BUFFERCLIENT_UNTRUSTED_VM = 1,
  BUFFERCLIENT_TRUSTED_VM = 2,
  BUFFERCLIENT_MAX = 3,
} vendor_qti_hardware_display_common_BufferClient;

#endif  // __COMMON_BUFFERCLIENT_H__
