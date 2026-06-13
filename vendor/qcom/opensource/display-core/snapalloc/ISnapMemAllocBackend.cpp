// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "ISnapMemAllocBackend.h"

#include "SnapDMAAllocator.h"
#include "SnapTestAllocator.h"

namespace snapalloc {

ISnapMemAllocBackend *ISnapMemAllocBackend::GetInstance() {
#ifdef SHM_ALLOCATE
  return SnapTestAllocator::GetInstance();
#else
  return SnapDMAAllocator::GetInstance();
#endif
}

}  // namespace snapalloc
