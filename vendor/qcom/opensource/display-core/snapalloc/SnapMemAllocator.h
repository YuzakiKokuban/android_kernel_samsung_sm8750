// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_MEM_ALLOCATOR_H__
#define __SNAP_MEM_ALLOCATOR_H__

#include <cstdint>
#include <mutex>

#include "ISnapMemAllocBackend.h"
#include "SnapMemAllocDefs.h"
#include "SnapTypes.h"
#include "SnapUtils.h"

namespace snapalloc {

class SnapMemAllocator {
 public:
  SnapMemAllocator(SnapMemAllocator &other) = delete;
  void operator=(const SnapMemAllocator &) = delete;
  static SnapMemAllocator *GetInstance();
  Error MapBuffer(void **base, unsigned int size, int fd);
  int ImportBuffer(int fd);  // not in diagram - check if needed
  Error FreeBuffer(void *base, unsigned int size, int fd, std::string buffer_path);
  Error CleanBuffer(void *base, unsigned int size, int op, int fd);
  Error AllocateMem(AllocData *data, vendor_qti_hardware_display_common_BufferUsage usage,
                    vendor_qti_hardware_display_common_PixelFormat format);
  Error SetBufferPermission(
      int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm,
      int64_t *mem_hdl);

 private:
  ~SnapMemAllocator();
  SnapMemAllocator();
  static std::mutex mem_allocator_instance_mutex_;
  std::mutex mem_allocator_mutex_;
  static SnapMemAllocator *instance_;
  bool use_system_heap_for_sensors_ = true;
  ISnapMemAllocBackend *alloc_intf_ = nullptr;
};

}  // namespace snapalloc

#endif  // __SNAP_MEM_ALLOCATOR_H__