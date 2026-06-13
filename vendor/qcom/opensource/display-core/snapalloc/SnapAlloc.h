// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAPALLOC_H__
#define __SNAPALLOC_H__

#include <ISnapAlloc.h>
#include "SnapAllocCore.h"

namespace vendor::qti::hardware::display::snapalloc {

class SnapAlloc : public ISnapAlloc {
 public:
  SnapAlloc();

  Error Allocate(const BufferDescriptor &in_descriptor, int in_count,
                 AllocationResult *allocation_result) override;
  Error IsSupported(const BufferDescriptor &in_descriptor, bool *is_supported) override;

 private:
  ::snapalloc::SnapAllocCore *snap_alloc_core_;
};

}  // namespace vendor::qti::hardware::display::snapalloc

#endif  // __SNAPALLOC_H__
