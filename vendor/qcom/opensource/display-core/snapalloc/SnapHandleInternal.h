// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_HANDLE_INTERNAL_H__
#define __SNAP_HANDLE_INTERNAL_H__

#include <unistd.h>
#include <memory>

#include "SnapTypes.h"
#include "Debug.h"

namespace snapalloc {

inline bool isSnapHandleEmpty(SnapHandle *handle) {
  return (handle->num_ints == 0 || handle->num_fds == 0);
}

class SnapHandleInternal : public SnapHandle {
 protected:
  SnapHandleInternal() {}
  struct FdPair;
  struct SnapHandleProperties;

 public:
  int &fd();
  int &fd_metadata();
  uint32_t &view();
  int &flags();
  int &aligned_width_in_bytes();
  int &aligned_width_in_pixels();
  int &aligned_height();
  int &unaligned_width();
  int &unaligned_height();
  vendor_qti_hardware_display_common_PixelFormat &format();
  int &buffer_type();
  unsigned &layer_count();
  uint64_t &id();
  vendor_qti_hardware_display_common_BufferUsage &usage();
  unsigned &size();
  uint64_t &base();
  uint64_t &base_metadata();
  uint64_t &fb_id();
  unsigned &reserved_size();
  unsigned &custom_content_md_reserved_size();
  uint64_t &pixel_format_modifier();
  uint64_t &reserved_region_base();
  uint64_t &custom_content_md_region_base();
  //static const int kNumFds = 2;
  unsigned &flush();
  // Lock count to ensure nested lock/unlock situations are handled correctly
  int &lock_count();

  int GetRefCount() { return ref_count(); }
  void IncRef() { ++ref_count(); }
  bool DecRef() { return --ref_count() == 0; }
  void ResetRefCount() { ref_count() = 0; }

  static int validate(SnapHandle *h);

  static SnapHandleInternal *createSingleHandle(
      int fd, int meta_fd, int flags, int width_in_bytes, int width_in_pixels, int height, int uw,
      int uh, vendor_qti_hardware_display_common_PixelFormat format, int buf_type, uint64_t id,
      unsigned size, vendor_qti_hardware_display_common_BufferUsage usage,
      uint64_t pixel_format_modifier, unsigned layer_count, unsigned reserved_size,
      unsigned custom_content_md_size);

  static SnapHandleInternal *createMultiviewHandle(
      int fd1, int meta_fd1, int fd2, int meta_fd2, int flags, int width_in_bytes,
      int width_in_pixels, int height, int uw, int uh,
      vendor_qti_hardware_display_common_PixelFormat format, int buf_type, uint64_t id1,
      uint64_t id2, unsigned size, vendor_qti_hardware_display_common_BufferUsage usage,
      uint64_t pixel_format_modifier, unsigned layer_count, unsigned reserved_size,
      unsigned custom_content_md_size);

  std::vector<FdPair> getFds();
  void closeFds();
  uint32_t getViewInfo();
  SnapHandleInternal *CreateViewHandle(uint32_t view);

 private:
  SnapHandleInternal(const SnapHandleInternal &other) = delete;
  SnapHandleInternal &operator=(const SnapHandleInternal &other) = delete;

  int getN();
  FdPair &currentFdPair();
  SnapHandleProperties &currentProperties();

  int &ref_count();
};

}  // namespace snapalloc

#endif  // __SNAP_HANDLE_INTERNAL_H__