// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapAlloc.h"
#include "SnapHandleInternal.h"
#include "SnapTypes.h"
#include "SnapUtils.h"
using ::snapalloc::Debug;

namespace vendor::qti::hardware::display::snapalloc {

SnapAlloc::SnapAlloc() {
  snap_alloc_core_ = ::snapalloc::SnapAllocCore::GetInstance();
};

Error SnapAlloc::Allocate(const BufferDescriptor &in_descriptor, int in_count,
                          AllocationResult *allocation_result) {
  std::vector<::snapalloc::SnapHandleInternal *> handles;
  handles.reserve(in_count);

  auto err = snap_alloc_core_->Allocate(in_descriptor, in_count, &handles, false);
  if (err != Error::NONE) {
    return err;
  }

  if (!handles.empty()) {
    allocation_result->stride = handles[0]->aligned_width_in_pixels();
  }

  allocation_result->handles.reserve(in_count);

  for (int i = 0; i < in_count; i++) {
    allocation_result->handles.emplace_back(static_cast<SnapHandle *>(handles[i]));
    snap_alloc_core_->Retain(allocation_result->handles[i]);
  }

  return Error::NONE;
};

Error SnapAlloc::IsSupported(const BufferDescriptor &in_descriptor, bool *is_supported) {
  return snap_alloc_core_->IsSupported(in_descriptor, is_supported);
};

extern "C" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"

std::shared_ptr<ISnapAlloc> FETCH_ISnapAlloc(DebugCallbackIntf *dbg) {
  ::snapalloc::Debug *debug_instance = ::snapalloc::Debug::GetInstance();
  debug_instance->RegisterDebugCallback(dbg);

  std::shared_ptr<ISnapAlloc> obj = std::make_shared<SnapAlloc>();
  return obj;
}
#pragma clang diagnostic pop
}

}  // namespace vendor::qti::hardware::display::snapalloc
