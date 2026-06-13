// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapTestAllocator.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace snapalloc {

SnapTestAllocator *SnapTestAllocator::instance_{nullptr};
std::mutex SnapTestAllocator::Snap_test_alloc_mutex_;

SnapTestAllocator *SnapTestAllocator::GetInstance() {
  std::lock_guard<std::mutex> lock(Snap_test_alloc_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapTestAllocator();
  }
  return instance_;
}

Error SnapTestAllocator::AllocBuffer(AllocData *ad[[maybe_unused]]) {
#ifdef SHM_ALLOCATE
  int fd = shm_open("test", O_RDWR | O_CREAT, S_IRWXU);
  if (fd >= 0) {
    ftruncate(fd, ad->size);
  }
  ad->fd = fd;
#endif
  return Error::NONE;
}

Error SnapTestAllocator::FreeBuffer(void * /* base */, unsigned int /* size */, int /*fd*/,
                                    std::string shm_path[[maybe_unused]]) {
  int err = 0;
#ifdef SHM_ALLOCATE
  int err = shm_unlink(shm_path.c_str());
#endif
  if (!err) {
    return Error::NONE;
  }
  return Error::BAD_VALUE;
}

Error SnapTestAllocator::MapBuffer(void **base, unsigned int size, int fd) {
  *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (*base != nullptr) {
    return Error::NONE;
  }

  return Error::BAD_VALUE;
}

Error SnapTestAllocator::CleanBuffer(void *base, unsigned int size, int op, int fd) {
  /* TODO: May remove for test allocator - original uses dma buf specific sync */
  (void)base;
  (void)size;
  (void)op;
  (void)fd;
  return Error::UNSUPPORTED;
}

int SnapTestAllocator::ImportBuffer(int fd) {
  return fd;
}

Error SnapTestAllocator::SecureMemPerms(AllocData *ad) {
  (void)ad;
  return Error::UNSUPPORTED;
}

void SnapTestAllocator::GetHeapInfo(vendor_qti_hardware_display_common_BufferUsage usage,
                                    bool sensor_flag, std::string *heap_name,
                                    std::vector<std::string> *vm_names, unsigned int *alloc_type,
                                    unsigned int *flags, unsigned int *alloc_size) {
  (void)usage;
  (void)sensor_flag;
  (void)heap_name;
  (void)alloc_type;
  (void)vm_names;
  (void)flags;
  (void)alloc_size;
}

void SnapTestAllocator::
    GetVMPermission(/*vendor_qti_hardware_display_common_BufferPermission buf_perm,*/
                    std::bitset<kVmPermissionMax> *vm_perm) {
  //(void)buf_perm;
  (void)*vm_perm;
}

Error SnapTestAllocator::SetBufferPermission(
    int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm, int64_t *mem_hdl) {
  (void)fd;
  (void)buffer_perm;
  (void)mem_hdl;
  return Error::UNSUPPORTED;
}

}  // namespace snapalloc
