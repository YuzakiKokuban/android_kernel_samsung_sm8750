// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapMapper.h"
#include <sync/sync.h>
#include "SnapTypes.h"
using ::snapalloc::Debug;

namespace vendor::qti::hardware::display::snapalloc {

SnapMapper::SnapMapper() {
  snap_alloc_core_ = ::snapalloc::SnapAllocCore::GetInstance();
};

void SnapMapper::WaitFenceFd(int fence_fd) {
  const int timeout = 3000;
  const int error = sync_wait(fence_fd, timeout);
  if (error < 0) {
    DLOGE("%s: lock fence %d didn't signal in %u ms -  error: %s", __FUNCTION__, fence_fd, timeout,
          strerror(errno));
  }
}

Error SnapMapper::Retain(const SnapHandle &in_handle) {
  SnapHandle *hnd = const_cast<SnapHandle *>(&in_handle);

  auto error = snap_alloc_core_->Retain(hnd);
  return error;
}

Error SnapMapper::RetainViewBuffer(const SnapHandle &in_meta_handle, uint32_t view,
                                   SnapHandle **out_view_handle) {
  SnapHandle *meta_hnd = const_cast<SnapHandle *>(&in_meta_handle);

  auto error = snap_alloc_core_->RetainViewBuffer(meta_hnd, view, out_view_handle);
  return error;
}

Error SnapMapper::Release(const SnapHandle &in_handle) {
  SnapHandle *hnd = const_cast<SnapHandle *>(&in_handle);

  auto error = snap_alloc_core_->Release(hnd);
  return error;
}

Error SnapMapper::Lock(const SnapHandle &in_handle,
                       vendor_qti_hardware_display_common_BufferUsage in_usage,
                       const vendor_qti_hardware_display_common_Rect &access_region,
                       const vendor_qti_hardware_display_common_Fence &in_fence,
                       vendor_qti_hardware_display_common_Address *base_addr) {
  auto err = Error::NONE;
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  if (in_fence.fence_fd > 0) {
    WaitFenceFd(in_fence.fence_fd);
  }
  uint64_t address;
  err = snap_alloc_core_->Lock(const_cast<SnapHandle *>(&in_handle), in_usage, access_region,
                               &address);
  base_addr->addressPointer = address;

  return err;
}

Error SnapMapper::Unlock(const SnapHandle &in_handle,
                         vendor_qti_hardware_display_common_Fence *fence) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }
  auto err = snap_alloc_core_->Unlock(const_cast<SnapHandle *>(&in_handle));
  fence->fence_fd = -1;
  return err;
}

Error SnapMapper::FlushLockedBuffer(const SnapHandle &in_handle) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  auto err = snap_alloc_core_->FlushLockedBuffer(const_cast<SnapHandle *>(&in_handle));
  return err;
}

Error SnapMapper::RereadLockedBuffer(const SnapHandle &in_handle) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  auto err = snap_alloc_core_->RereadLockedBuffer(const_cast<SnapHandle *>(&in_handle));
  return err;
}

Error SnapMapper::GetMetadata(const SnapHandle &in_handle,
                              vendor_qti_hardware_display_common_MetadataType in_type, void *out) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  auto err = snap_alloc_core_->GetMetadata(const_cast<SnapHandle *>(&in_handle), in_type, out);
  return err;
}

Error SnapMapper::SetMetadata(const SnapHandle &in_handle,
                              vendor_qti_hardware_display_common_MetadataType in_type, void *in) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  auto err = snap_alloc_core_->SetMetadata(const_cast<SnapHandle *>(&in_handle), in_type, in);
  return err;
}

Error SnapMapper::GetFromBufferDescriptor(const BufferDescriptor &in_descriptor,
                                          vendor_qti_hardware_display_common_MetadataType in_type,
                                          void *out) {
  auto err = snap_alloc_core_->GetFromBufferDescriptor(in_descriptor, in_type, out);
  return err;
}

Error SnapMapper::ValidateBufferSize(const SnapHandle &in_handle,
                                     const BufferDescriptor &in_descriptor) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  auto err =
      snap_alloc_core_->ValidateBufferSize(const_cast<SnapHandle *>(&in_handle), in_descriptor);
  return err;
}

Error SnapMapper::DumpBuffer(const SnapHandle &in_handle) {
  (void)in_handle;
  return Error::UNSUPPORTED;
}

Error SnapMapper::DumpBuffers() {
  return Error::UNSUPPORTED;
}

Error SnapMapper::ListSupportedMetadataTypes() {
  return Error::UNSUPPORTED;
}

Error SnapMapper::GetMetadataState(const SnapHandle &in_handle, vendor_qti_hardware_display_common_MetadataType in_type, bool *out) {
  if (::snapalloc::isSnapHandleEmpty(const_cast<SnapHandle *>(&in_handle))) {
    return Error::BAD_BUFFER;
  }

  auto err = snap_alloc_core_->GetMetadataState(const_cast<SnapHandle *>(&in_handle), in_type, out);
  return err;
}

extern "C" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"

std::shared_ptr<ISnapMapper> FETCH_ISnapMapper(DebugCallbackIntf *dbg) {
  ::snapalloc::Debug *debug_instance = ::snapalloc::Debug::GetInstance();
  debug_instance->RegisterDebugCallback(dbg);

  std::shared_ptr<ISnapMapper> obj = std::make_shared<SnapMapper>();
  return obj;
}
#pragma clang diagnostic pop
}

}  // namespace vendor::qti::hardware::display::snapalloc
