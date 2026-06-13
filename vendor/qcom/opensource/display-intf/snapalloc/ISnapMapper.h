/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <vector>

#include "SnapHandle.h"
#include <MetadataType.h>
#include <Address.h>
#include <Rect.h>
#include <Fence.h>

#include "AllocationResult.h"
#include "BufferDescriptor.h"
#include "Error.h"

namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace snapalloc {

class ISnapMapper {
 public:
  virtual ~ISnapMapper() {}
  virtual Error Retain(const SnapHandle &in_handle) = 0;
  virtual Error Release(const SnapHandle &in_handle) = 0;
  virtual Error Lock(const SnapHandle &in_handle,
                     vendor_qti_hardware_display_common_BufferUsage usage,
                     const vendor_qti_hardware_display_common_Rect &access_region,
                     const vendor_qti_hardware_display_common_Fence &in_fence,
                     vendor_qti_hardware_display_common_Address *base_addr) = 0;
  virtual Error Unlock(
      const SnapHandle &in_handle,
      vendor_qti_hardware_display_common_Fence *fence) = 0;  // pointer with fence return
  virtual Error ValidateBufferSize(const SnapHandle &in_handle,
                                   const BufferDescriptor &in_desc) = 0;
  virtual Error FlushLockedBuffer(const SnapHandle &in_handle) = 0;
  virtual Error RereadLockedBuffer(const SnapHandle &in_handle) = 0;
  virtual Error GetMetadata(const SnapHandle &in_handle,
                            vendor_qti_hardware_display_common_MetadataType type, void *out) = 0;
  virtual Error SetMetadata(const SnapHandle &in_handle,
                            vendor_qti_hardware_display_common_MetadataType type, void *in) = 0;
  virtual Error GetFromBufferDescriptor(const BufferDescriptor &in_desc,
                                        vendor_qti_hardware_display_common_MetadataType type,
                                        void *out) = 0;
  virtual Error DumpBuffer(const SnapHandle &in_handle) = 0;
  virtual Error DumpBuffers() = 0;
  virtual Error ListSupportedMetadataTypes() = 0;
  virtual Error GetMetadataState(const SnapHandle &in_handle,
                                 vendor_qti_hardware_display_common_MetadataType type,
                                 bool *out) = 0;
  virtual Error RetainViewBuffer(const SnapHandle &in_meta_handle, uint32_t view,
                                 SnapHandle **out_view_handle) = 0;
};

}  // namespace snapalloc
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
