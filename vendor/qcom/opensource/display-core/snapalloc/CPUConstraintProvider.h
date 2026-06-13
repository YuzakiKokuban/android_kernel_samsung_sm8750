// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __CPU_CONSTRAINT_PROVIDER_H__
#define __CPU_CONSTRAINT_PROVIDER_H__

#include <map>
#include <mutex>
#include "SnapConstraintProvider.h"

namespace snapalloc {
class CPUConstraintProvider : public SnapConstraintProvider {
 public:
  CPUConstraintProvider(CPUConstraintProvider &other) = delete;
  void operator=(const CPUConstraintProvider &) = delete;
  static CPUConstraintProvider *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);
  int GetProviderType() { return kCPU; }

  int GetCapabilities(BufferDescriptor desc, CapabilitySet *out);
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);

 private:
  CPUConstraintProvider(){};
  ~CPUConstraintProvider(){};
  static std::mutex cpu_provider_mutex_;
  static CPUConstraintProvider *instance_;

  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
};
}  // namespace snapalloc

#endif  // __CPU_CONSTRAINT_PROVIDER_H__