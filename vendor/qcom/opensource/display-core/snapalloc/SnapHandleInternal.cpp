// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapHandleInternal.h"
#include <array>
#include <fcntl.h>

namespace snapalloc {

struct SnapHandleInternal::FdPair {
  int fd;
  int fd_metadata;
};

struct SnapHandleInternal::SnapHandleProperties {
  uint32_t view;
  int flags;
  int aligned_width_in_bytes;
  int aligned_width_in_pixels;
  int aligned_height;
  int unaligned_width;
  int unaligned_height;
  vendor_qti_hardware_display_common_PixelFormat format;
  int buffer_type;
  unsigned layer_count;
  uint64_t id;
  vendor_qti_hardware_display_common_BufferUsage usage =
      vendor_qti_hardware_display_common_BufferUsage::CPU_READ_NEVER;
  unsigned size;
  uint64_t base;
  uint64_t base_metadata;
  uint64_t fb_id;
  unsigned reserved_size;
  unsigned custom_content_md_reserved_size;
  uint64_t pixel_format_modifier;
  uint64_t reserved_region_base;
  uint64_t custom_content_md_region_base;
  //static const int kNumFds = 2;
  unsigned flush = false;
  // Lock count to ensure nested lock/unlock situations are handled correctly
  int lock_count = 0;

  int ref_count = 0;
};

#define DEFINE_FD_ACCESSOR(cls, type, var) \
  type &cls::var() {                       \
    return currentFdPair().var;            \
  }

#define DEFINE_PROPERTY_ACCESSOR(cls, type, var) \
  type &cls::var() {                             \
    return currentProperties().var;              \
  }

DEFINE_FD_ACCESSOR(SnapHandleInternal, int, fd)
DEFINE_FD_ACCESSOR(SnapHandleInternal, int, fd_metadata)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, flags)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint32_t, view)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, aligned_width_in_bytes)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, aligned_width_in_pixels)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, aligned_height)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, unaligned_width)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, unaligned_height)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, vendor_qti_hardware_display_common_PixelFormat, format)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, buffer_type)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, unsigned, layer_count)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, id)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, vendor_qti_hardware_display_common_BufferUsage, usage)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, unsigned, size)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, base)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, base_metadata)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, fb_id)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, unsigned, reserved_size)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, unsigned, custom_content_md_reserved_size)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, pixel_format_modifier)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, reserved_region_base)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, uint64_t, custom_content_md_region_base)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, unsigned, flush)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, lock_count)
DEFINE_PROPERTY_ACCESSOR(SnapHandleInternal, int, ref_count)

template <int N>
class SnapHandleData : public SnapHandleInternal {
 public:
  std::array<FdPair, N> fdPairArray{};
  std::array<SnapHandleProperties, N> propertiesArray{};

  int getNumInts() { return sizeof(propertiesArray) / sizeof(int); }

  static int getNumInts(int n) { return sizeof(SnapHandleProperties) / sizeof(int) * n; }

  FdPair &getFdPair(int n) { return fdPairArray[n]; }

  SnapHandleProperties &getProperties(int n) { return propertiesArray[n]; }

  static int getExpectedNumFds() { return sizeof(std::array<FdPair, N>) / sizeof(int); }

  static int getExpectedNumInts() {
    return sizeof(std::array<SnapHandleProperties, N>) / sizeof(int);
  }

  void setProperties(SnapHandleProperties &prop, int flags, uint32_t view, int width_in_bytes,
                     int width_in_pixels, int height, int uw, int uh,
                     vendor_qti_hardware_display_common_PixelFormat format, int buf_type,
                     uint64_t id, unsigned size,
                     vendor_qti_hardware_display_common_BufferUsage usage,
                     uint64_t pixel_format_modifier, unsigned layer_count, unsigned reserved_size,
                     unsigned custom_content_md_size) {
    prop.flags = flags;
    prop.aligned_width_in_bytes = width_in_bytes;
    prop.aligned_width_in_pixels = width_in_pixels;
    prop.aligned_height = height;
    prop.unaligned_width = uw;
    prop.unaligned_height = uh;
    prop.format = format;
    prop.buffer_type = buf_type;
    prop.id = id;
    prop.usage = usage;
    prop.size = size;
    prop.base = 0;
    prop.base_metadata = 0;
    prop.layer_count = layer_count;
    prop.reserved_size = reserved_size;
    prop.custom_content_md_reserved_size = custom_content_md_size;
    prop.pixel_format_modifier = pixel_format_modifier;
    prop.view = view;
  }
};
void SnapHandleInternal::closeFds() {
  std::vector<FdPair> fd_pairs = (this)->getFds();
  for (const auto &fd_pair : fd_pairs) {
    close(fd_pair.fd);
    close(fd_pair.fd_metadata);
  }
}
uint32_t SnapHandleInternal::getViewInfo() {
  uint32_t viewInfo = 0;

  switch (getN()) {
    case 1:
      viewInfo = (this)->view();
      break;
    case 2:
      for (const auto &properties : static_cast<SnapHandleData<2> *>(this)->propertiesArray) {
        viewInfo |= properties.view;
      }
      break;
    default:
      DLOGE("Unsupported view info handle of N %d", getN());
      viewInfo = (this)->view();
      break;
  }
  return viewInfo;
}

std::vector<SnapHandleInternal::FdPair> SnapHandleInternal::getFds() {
  //create vector,switch case, push on to vectors
  std::vector<SnapHandleInternal::FdPair> fd_pairs;
  switch (getN()) {
    case 1:
      fd_pairs.emplace_back(static_cast<SnapHandleData<1> *>(this)->getFdPair(0));
      break;
    case 2:
      fd_pairs.emplace_back(static_cast<SnapHandleData<2> *>(this)->getFdPair(0));
      fd_pairs.emplace_back(static_cast<SnapHandleData<2> *>(this)->getFdPair(1));
      break;
    default:
      DLOGE("Unsupported meta handle of N %d", getN());
      fd_pairs.emplace_back(static_cast<SnapHandleData<1> *>(this)->getFdPair(0));
      break;
  }
  return fd_pairs;
}

SnapHandleInternal *SnapHandleInternal::CreateViewHandle(uint32_t view) {
  int N = getN();

  if (N > 2) {
    DLOGE("Unsupported meta handle of N %d", N);
    return nullptr;
  }

  int view_index = -1;
  switch (view) {
    case PRIV_VIEW_MASK_PRIMARY:
      view_index = 0;
      break;
    case PRIV_VIEW_MASK_SECONDARY:
      view_index = 1;
      break;
    default:
      DLOGE("Unsupported view mask %d", view);
      return nullptr;
  }

  if (view_index >= N) {
    DLOGE("Meta Handle doesn't contain the requested view %d", view);
    return nullptr;
  }

  size_t handle_size = sizeof(SnapHandleProperties) + sizeof(FdPair) + sizeof(SnapHandle);
  SnapHandleData<1> *view_handle = static_cast<SnapHandleData<1> *>(malloc(handle_size));

  view_handle->num_ints = SnapHandleData<1>::getExpectedNumInts();
  view_handle->num_fds = SnapHandleData<1>::getExpectedNumFds();
  view_handle->version = static_cast<int>(sizeof(SnapHandle));

  switch (N) {
    case 2:
      view_handle->getFdPair(0).fd = fcntl(
          static_cast<SnapHandleData<2> *>(this)->getFdPair(view_index).fd, F_DUPFD_CLOEXEC, 0);
      view_handle->getFdPair(0).fd_metadata =
          fcntl(static_cast<SnapHandleData<2> *>(this)->getFdPair(view_index).fd_metadata,
                F_DUPFD_CLOEXEC, 0);
      view_handle->getProperties(0) =
          static_cast<SnapHandleData<2> *>(this)->getProperties(view_index);
      break;
    default:
      DLOGE("Unsupported Meta Handle");
      free(view_handle);
      return nullptr;
  }

  return view_handle;
}

int SnapHandleInternal::getN() {
  return num_fds / (sizeof(FdPair) / sizeof(int));
}

SnapHandleInternal::FdPair &SnapHandleInternal::currentFdPair() {
  switch (getN()) {
    case 1:
      return static_cast<SnapHandleData<1> *>(this)->getFdPair(0);
    case 2:
      return static_cast<SnapHandleData<2> *>(this)->getFdPair(0);
    default:
      // trespass
      DLOGE("Unsupported meta handle of N %d", getN());
      return static_cast<SnapHandleData<1> *>(this)->getFdPair(0);
  }
}

SnapHandleInternal::SnapHandleProperties &SnapHandleInternal::currentProperties() {
  switch (getN()) {
    case 1:
      return static_cast<SnapHandleData<1> *>(this)->getProperties(0);
    case 2:
      return static_cast<SnapHandleData<2> *>(this)->getProperties(0);
    default:
      // trespass
      DLOGE("Unsupported meta handle of N %d", getN());
      return static_cast<SnapHandleData<1> *>(this)->getProperties(0);
  }
}

int SnapHandleInternal::validate(SnapHandle *h) {
  if (!h) {
    DLOGE("null SnapHandleInternal");
    return -1;
  }

  SnapHandleInternal *ih = static_cast<SnapHandleInternal *>(h);
  const int n = ih->getN();

  if (n == 1) {
    if (ih->num_fds != SnapHandleData<1>::getExpectedNumFds() ||
        ih->num_ints != SnapHandleData<1>::getExpectedNumInts()) {
      DLOGE(
          "Invalid SnapHandleInternal (at %p): ver(%d) N(%d) Expected ints: (%d) Actual ints:(%d) "
          "Expected fds: (%d) Actual fds:(%d) ",
          h, h->version, n, SnapHandleData<1>::getExpectedNumInts(), h->num_ints,
          SnapHandleData<1>::getExpectedNumFds(), h->num_fds);
      return -1;
    }
  } else if (n == 2) {
    if (ih->num_fds != SnapHandleData<2>::getExpectedNumFds() ||
        ih->num_ints != SnapHandleData<2>::getExpectedNumInts()) {
      DLOGE(
          "Invalid SnapHandleInternal (at %p): ver(%d) N(%d) Expected ints: (%d) Actual ints:(%d) "
          "Expected fds: (%d) Actual fds:(%d) ",
          h, h->version, n, SnapHandleData<2>::getExpectedNumInts(), h->num_ints,
          SnapHandleData<2>::getExpectedNumFds(), h->num_fds);
      return -1;
    }
  } else {
    DLOGE("Invalid SnapHandleInternal (at %p): ver(%d) N(%d) ints(%d) fds(%d)", h, h->version, n,
          h->num_ints, h->num_fds);
    return -1;
  }
  return 0;
}

SnapHandleInternal *SnapHandleInternal::createSingleHandle(
    int fd, int meta_fd, int flags, int width_in_bytes, int width_in_pixels, int height, int uw,
    int uh, vendor_qti_hardware_display_common_PixelFormat format, int buf_type, uint64_t id,
    unsigned size, vendor_qti_hardware_display_common_BufferUsage usage,
    uint64_t pixel_format_modifier, unsigned layer_count, unsigned reserved_size,
    unsigned custom_content_md_size) {
  size_t handle_size = sizeof(SnapHandleProperties) + sizeof(FdPair) + sizeof(SnapHandle);
  SnapHandleData<1> *h = static_cast<SnapHandleData<1> *>(malloc(handle_size));

  h->num_ints = SnapHandleData<1>::getExpectedNumInts();
  h->num_fds = SnapHandleData<1>::getExpectedNumFds();
  h->version = static_cast<int>(sizeof(SnapHandle));

  FdPair &fd_primary = h->getFdPair(0);
  fd_primary.fd = fd;
  fd_primary.fd_metadata = meta_fd;

  SnapHandleProperties &prop = h->getProperties(0);
  h->setProperties(prop, flags, vendor_qti_hardware_display_common_QtiViews::PRIV_VIEW_MASK_PRIMARY,
                   width_in_bytes, width_in_pixels, height, uw, uh, format, buf_type, id, size,
                   usage, pixel_format_modifier, layer_count, reserved_size,
                   custom_content_md_size);
  return h;
}

SnapHandleInternal *SnapHandleInternal::createMultiviewHandle(
    int fd1, int meta_fd1, int fd2, int meta_fd2, int flags, int width_in_bytes,
    int width_in_pixels, int height, int uw, int uh,
    vendor_qti_hardware_display_common_PixelFormat format, int buf_type, uint64_t id1, uint64_t id2,
    unsigned size, vendor_qti_hardware_display_common_BufferUsage usage,
    uint64_t pixel_format_modifier, unsigned layer_count, unsigned reserved_size,
    unsigned custom_content_md_size) {
  size_t handle_size = ((sizeof(SnapHandleProperties) + sizeof(FdPair)) * 2 + sizeof(SnapHandle));
  SnapHandleData<2> *h = static_cast<SnapHandleData<2> *>(malloc(handle_size));

  h->num_ints = SnapHandleData<2>::getExpectedNumInts();
  h->num_fds = SnapHandleData<2>::getExpectedNumFds();
  h->version = static_cast<int>(sizeof(SnapHandle));

  FdPair &fd_primary = h->getFdPair(0);
  fd_primary.fd = fd1;
  fd_primary.fd_metadata = meta_fd1;

  FdPair &fd_secondary = h->getFdPair(1);
  fd_secondary.fd = fd2;
  fd_secondary.fd_metadata = meta_fd2;

  SnapHandleProperties &prop_primary = h->getProperties(0);
  h->setProperties(
      prop_primary, flags, vendor_qti_hardware_display_common_QtiViews::PRIV_VIEW_MASK_PRIMARY,
      width_in_bytes, width_in_pixels, height, uw, uh, format, buf_type, id1, size, usage,
      pixel_format_modifier, layer_count, reserved_size, custom_content_md_size);

  SnapHandleProperties &prop_secondary = h->getProperties(1);
  h->setProperties(
      prop_secondary, flags, vendor_qti_hardware_display_common_QtiViews::PRIV_VIEW_MASK_SECONDARY,
      width_in_bytes, width_in_pixels, height, uw, uh, format, buf_type, id2, size, usage,
      pixel_format_modifier, layer_count, reserved_size, custom_content_md_size);

  return h;
}

}  // namespace snapalloc