// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_TEST_ALLOCATOR_H__
#define __SNAP_TEST_ALLOCATOR_H__

#include "SnapMemAllocDefs.h"

#include <bitset>
#include <cstdint>
#include <mutex>
#include <string>

#include "ISnapMemAllocBackend.h"
#include "SnapTypes.h"

namespace snapalloc {

class SnapTestAllocator : public ISnapMemAllocBackend {
 public:
  static SnapTestAllocator *GetInstance();
  Error AllocBuffer(AllocData *ad);
  Error FreeBuffer(void * /*base */, unsigned int /* size */, int /*fd*/, std::string shm_path);
  Error MapBuffer(void **base, unsigned int size, int fd);
  Error CleanBuffer(void *base, unsigned int size, int op, int fd);
  int ImportBuffer(int fd);
  Error SecureMemPerms(AllocData *ad);
  void GetHeapInfo(vendor_qti_hardware_display_common_BufferUsage usage, bool sensor_flag,
                   std::string *heap_name, std::vector<std::string> *vm_names,
                   unsigned int *alloc_type, unsigned int *flags, unsigned int *alloc_size);
  Error SetBufferPermission(
      int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm, int64_t *mem_hdl);

 private:
  void GetVMPermission(
      /*vendor_qti_hardware_display_common_BufferPermission buf_perm,*/ std::bitset<
          kVmPermissionMax> *vm_perm);
  ~SnapTestAllocator() {}
  SnapTestAllocator() {}
  static std::mutex Snap_test_alloc_mutex_;
  static SnapTestAllocator *instance_;
};

}  // namespace snapalloc

#endif  // __SNAP_TEST_ALLOCATOR_H__
