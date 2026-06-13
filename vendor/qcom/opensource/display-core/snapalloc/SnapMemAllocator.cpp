// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapMemAllocator.h"
#include <iostream>

namespace snapalloc {

SnapMemAllocator *SnapMemAllocator::instance_{nullptr};
std::mutex SnapMemAllocator::mem_allocator_instance_mutex_;

SnapMemAllocator *SnapMemAllocator::GetInstance() {
  std::lock_guard<std::mutex> lock(mem_allocator_instance_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapMemAllocator();
  }
  return instance_;
}

SnapMemAllocator::SnapMemAllocator() : alloc_intf_{ISnapMemAllocBackend::GetInstance()} {};

Error SnapMemAllocator::AllocateMem(AllocData *alloc_data,
                                    vendor_qti_hardware_display_common_BufferUsage usage,
                                    vendor_qti_hardware_display_common_PixelFormat format) {
  std::lock_guard<std::mutex> lock(mem_allocator_mutex_);

  int ret = -1;
  int err = 0;

  if (!alloc_intf_) {
    return Error::NO_RESOURCES;
  }

  if (!alloc_data->size) {
    DLOGE("Failed to allocate buffer with size 0");
    return Error::BAD_VALUE;
  }

  // After this point we should have the right heap set, there is no fallback
  alloc_intf_->GetHeapInfo(usage, use_system_heap_for_sensors_, &alloc_data->heap_name,
                           &alloc_data->vm_names, &alloc_data->alloc_type, &alloc_data->flags,
                           &alloc_data->size);

  ret = alloc_intf_->AllocBuffer(alloc_data);

  if (ret < 0) {
    DLOGE("Failed to allocate buffer - heap name: %s, flags 0x%x ret %d ",
          alloc_data->heap_name.c_str(), alloc_data->flags, ret);
    return Error::BAD_VALUE;
  }

  if (!alloc_data->vm_names.empty()) {
    err = alloc_intf_->SecureMemPerms(alloc_data);
  }

  if (err) {
    DLOGE("Failed to modify secure use permissions - heap name: %s, flags 0x%x err %d",
          alloc_data->heap_name.c_str(), alloc_data->flags, err);
  }

  return Error::NONE;
}

Error SnapMemAllocator::FreeBuffer(void *base, unsigned int size, int fd, std::string buffer_path) {
  std::lock_guard<std::mutex> lock(mem_allocator_mutex_);

  if (!alloc_intf_) {
    return Error::NO_RESOURCES;
  }
  DLOGD_IF(enable_logs, "Freeing buffer base:%p size:%u fd:%d", base, size, fd);
  if (alloc_intf_) {
    return alloc_intf_->FreeBuffer(base, size, fd, std::move(buffer_path));
  }

  return Error::BAD_BUFFER;
}

Error SnapMemAllocator::MapBuffer(void **base, unsigned int size, int fd) {
  std::lock_guard<std::mutex> lock(mem_allocator_mutex_);

  if (!alloc_intf_) {
    return Error::NO_RESOURCES;
  }
  if (alloc_intf_) {
    return alloc_intf_->MapBuffer(base, size, fd);
  }

  return Error::BAD_BUFFER;
}

Error SnapMemAllocator::CleanBuffer(void *base, unsigned int size, int op, int fd) {
  std::lock_guard<std::mutex> lock(mem_allocator_mutex_);

  if (!alloc_intf_) {
    return Error::NO_RESOURCES;
  }
  if (alloc_intf_) {
    return alloc_intf_->CleanBuffer(base, size, op, fd);
  }

  return Error::BAD_BUFFER;
}

int SnapMemAllocator::ImportBuffer(int fd) {
  std::lock_guard<std::mutex> lock(mem_allocator_mutex_);

  if (alloc_intf_) {
    return alloc_intf_->ImportBuffer(fd);
  }
  DLOGE("ISnapMemAllocBackend is not available");
  return -1;
}

Error SnapMemAllocator::SetBufferPermission(
    int fd, vendor_qti_hardware_display_common_BufferPermission *buffer_perm,
    int64_t *mem_hdl) {
  std::lock_guard<std::mutex> lock(mem_allocator_mutex_);

  if (!alloc_intf_) {
    return Error::NO_RESOURCES;
  }
  if (alloc_intf_) {
    return alloc_intf_->SetBufferPermission(fd, buffer_perm, mem_hdl);
  }

  return Error::BAD_BUFFER;
}

}  // namespace  snapalloc
