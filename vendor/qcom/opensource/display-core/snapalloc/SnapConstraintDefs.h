// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_CONSTRAINT_DEFS_H__
#define __SNAP_CONSTRAINT_DEFS_H__

#include <cstdint>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "SnapTypes.h"

namespace snapalloc {

enum AlignmentType { ALIGN_UNINITIALIZED = 0, ALIGNED_OUTPUT = 1, ALIGNMENT = 2 };

struct UBWCCapabilities {
  int version;
};

struct CapabilitySet {
  bool enabled;                            // Provider is required for the allocation
  std::vector<uint64_t> formats_supported;  // Implementation defined formats supported
                                            // To be filled when input format
                                            // is implementation defined
  UBWCCapabilities ubwc_caps;  // UBWC capabilities of the provider
};

struct PlaneConstraints {
  std::vector<vendor_qti_hardware_display_common_PlaneLayoutComponentType> components;
  AlignmentType alignment_type;

  // All dimensions are in bytes
  union stride {
    uint64_t horizontal_stride;
    uint64_t horizontal_stride_align = 1;
  } stride;
  union scanline {
    uint64_t scanline;
    uint64_t scanline_align = 1;
  } scanline;
  uint64_t size_align = 1;

  // UBWC
  uint32_t block_width;
  uint32_t block_height;
};

// Used for storing constraints from parsing alignment jsons
struct BufferConstraints {
  uint64_t modifier = 0;
  std::vector<PlaneConstraints> planes;
  uint32_t size_align_bytes = 1;
};

// Used for format data - attributes that are common among providers
class PlaneLayoutData {
 public:
  std::vector<vendor_qti_hardware_display_common_PlaneLayoutComponent> components;
  uint32_t sample_increment_bits;
  uint64_t horizontal_subsampling = 0;
  uint64_t vertical_subsampling = 0;
};

struct FormatData {
  std::vector<PlaneLayoutData> planes;
  uint32_t bits_per_pixel;
};

}  // namespace snapalloc

#endif  // __SNAP_CONSTRAINT_DEFS_H__