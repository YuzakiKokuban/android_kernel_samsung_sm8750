// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAPALLOC_BUFFERDESCRIPTOR_H__
#define __SNAPALLOC_BUFFERDESCRIPTOR_H__

#include <PixelFormat.h>
#include <BufferUsage.h>
#include <KeyValuePair.h>
#include <vector>

namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace snapalloc {

#define QTI_MAX_NAME_LEN 256

typedef struct BufferDescriptor {
  char name[QTI_MAX_NAME_LEN];
  int width;
  int height;
  int layerCount;
  vendor_qti_hardware_display_common_PixelFormat format = PIXEL_FORMAT_UNSPECIFIED;
  vendor_qti_hardware_display_common_BufferUsage usage = CPU_READ_NEVER;
  long reservedSize;
  std::vector<vendor_qti_hardware_display_common_KeyValuePair> additionalOptions;
} BufferDescriptor;

}  // namespace snapalloc
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // __SNAPALLOC_BUFFERDESCRIPTOR_H__
