/*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <cstdint>

#ifndef __COLOREXTENSIONS_H__
#define __COLOREXTENSIONS_H__

#define CUSTOM_METADATA_SIZE_BYTES 1024*42
typedef struct CustomContentMetadata {
  uint64_t size;
  uint8_t metadataPayload[CUSTOM_METADATA_SIZE_BYTES];
} CustomContentMetadata;


#endif  // __COLOREXTENSIONS_H__

