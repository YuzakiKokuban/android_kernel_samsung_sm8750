// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAPALLOC_ALLOCATIONRESULT_H__
#define __SNAPALLOC_ALLOCATIONRESULT_H__

#include "SnapHandle.h"
#include <vector>

namespace vendor {
namespace qti {
namespace hardware {
namespace display {
namespace snapalloc {

typedef struct AllocationResult {
  int stride;
  std::vector<SnapHandle *> handles;
} AllocationResult;

}  // namespace snapalloc
}  // namespace display
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // __SNAPALLOC_ALLOCATIONRESULT_H__
