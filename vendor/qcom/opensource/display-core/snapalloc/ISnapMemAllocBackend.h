// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef __ISNAPMEMALLOCBACKEND_H__
#define __ISNAPMEMALLOCBACKEND_H__

#include "SnapMemAllocDefs.h"
#include "SnapTypes.h"

#include <cstdint>
#include <string>

namespace snapalloc {

class ISnapMemAllocBackend {
 public:
  static ISnapMemAllocBackend *GetInstance();

  virtual Error AllocBuffer(AllocData *ad) = 0;
  virtual Error FreeBuffer(void *base, unsigned int size, int fd, std::string path) = 0;
  virtual Error MapBuffer(void **base, unsigned int size, int fd) = 0;
  virtual Error CleanBuffer(void *base, unsigned int size, int op, int fd) = 0;
  virtual int ImportBuffer(int fd) = 0;
  virtual Error SecureMemPerms(AllocData *ad) = 0;
  virtual void GetHeapInfo(vendor_qti_hardware_display_common_BufferUsage usage, bool sensor_flag,
                           std::string *heap_name, std::vector<std::string> *vm_names,
                           unsigned int *alloc_type, unsigned int *flags,
                           unsigned int *alloc_size) = 0;
  virtual Error SetBufferPermission(
      int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm,
      int64_t *mem_hdl) = 0;

 protected:
  virtual ~ISnapMemAllocBackend() {}
};

}  // namespace snapalloc

#endif  // __ISNAPMEMALLOCBACKEND_H__