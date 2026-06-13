/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "AllocationResult.h"
#include "BufferDescriptor.h"
#include "Error.h"

namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace snapalloc {

class ISnapAlloc {
 public:
  virtual ~ISnapAlloc() {}
  virtual Error Allocate(const BufferDescriptor &in_descriptor, int in_count,
                         AllocationResult *allocation_result) = 0;
  virtual Error IsSupported(const BufferDescriptor &in_descriptor, bool *is_supported) = 0;
};

}  // namespace snapalloc
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor