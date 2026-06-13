// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_MEM_ALLOC_DEFS_H__
#define __SNAP_MEM_ALLOC_DEFS_H__

#include <bitset>
#include <map>
#include <string>
#include <vector>

#include "membuf_wrapper.h"

namespace snapalloc {

struct AllocData {
  void *base = NULL;
  int fd = -1;
  unsigned int size = 0;
  unsigned int align = 1;
  uintptr_t handle = 0;
  bool uncached = false;
  unsigned int flags = 0x0;
  std::string heap_name = "";
  std::vector<std::string> vm_names;
  unsigned int alloc_type = 0x0;
};

// clang-format off
enum {
  CACHE_CLEAN = 0x1,
  CACHE_INVALIDATE,
  CACHE_CLEAN_AND_INVALIDATE,
  CACHE_READ_DONE
};
// clang-format on

}  // namespace snapalloc

#endif  // __SNAP_MEM_ALLOC_DEFS_H__
