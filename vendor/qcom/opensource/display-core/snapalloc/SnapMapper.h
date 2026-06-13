// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAPMAPPER_H__
#define __SNAPMAPPER_H__

#include <ISnapMapper.h>
#include "SnapAllocCore.h"

namespace vendor::qti::hardware::display::snapalloc {

class SnapMapper : public ISnapMapper {
 public:
  SnapMapper();

  Error Retain(const SnapHandle &in_handle) override;
  Error Release(const SnapHandle &in_handle) override;
  Error Lock(const SnapHandle &in_handle, vendor_qti_hardware_display_common_BufferUsage usage,
             const vendor_qti_hardware_display_common_Rect &access_region,
             const vendor_qti_hardware_display_common_Fence &in_fence,
             vendor_qti_hardware_display_common_Address *base_addr) override;
  Error Unlock(const SnapHandle &in_handle,
               vendor_qti_hardware_display_common_Fence *fence) override;
  Error ValidateBufferSize(const SnapHandle &in_handle, const BufferDescriptor &in_desc) override;
  Error FlushLockedBuffer(const SnapHandle &in_handle) override;
  Error RereadLockedBuffer(const SnapHandle &in_handle) override;
  Error GetMetadata(const SnapHandle &in_handle,
                    vendor_qti_hardware_display_common_MetadataType type, void *out) override;
  Error SetMetadata(const SnapHandle &in_handle,
                    vendor_qti_hardware_display_common_MetadataType type, void *in) override;
  Error GetFromBufferDescriptor(const BufferDescriptor &in_desc,
                                vendor_qti_hardware_display_common_MetadataType type,
                                void *out) override;
  Error DumpBuffer(const SnapHandle &in_handle) override;
  Error DumpBuffers() override;
  Error ListSupportedMetadataTypes() override;
  Error GetMetadataState(const SnapHandle &in_handle, vendor_qti_hardware_display_common_MetadataType type, bool *out) override;
  Error RetainViewBuffer(const SnapHandle &in_meta_handle, uint32_t view,
                         SnapHandle **out_view_handle) override;

 private:
  void WaitFenceFd(int fence_fd);

  ::snapalloc::SnapAllocCore *snap_alloc_core_;
};

}  // namespace vendor::qti::hardware::display::snapalloc

#endif  // __SNAPMAPPER_H__