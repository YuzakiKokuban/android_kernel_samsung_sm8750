// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_CONSTRAINT_PROVIDER_H__
#define __SNAP_CONSTRAINT_PROVIDER_H__

#include "SnapConstraintDefs.h"
#include "SnapTypes.h"

#include <cstdint>
#include <vector>

namespace snapalloc {

enum ConstraintProviderType {
  kDefault = 0,
  kCamera = 1,
  kVideo = 2,
  kGraphics = 3,
  kCPU = 4,
  kDisplay = 5,
  kMax = 31
};

class SnapConstraintProvider {
 public:
  virtual ~SnapConstraintProvider(){};
  virtual void Init(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) = 0;
  virtual int GetCapabilities(BufferDescriptor desc, CapabilitySet *out) = 0;
  virtual int GetConstraints(BufferDescriptor desc, BufferConstraints *out) = 0;
  virtual int GetProviderType() = 0;
};
}  // namespace snapalloc

#endif  // __SNAP_CONSTRAINT_PROVIDER_H__
