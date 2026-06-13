// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __DEFAULT_CONSTRAINT_PROVIDER_H__
#define __DEFAULT_CONSTRAINT_PROVIDER_H__

#include <map>
#include <mutex>
#include "SnapConstraintProvider.h"
#include "SnapUtils.h"

namespace snapalloc {
class DefaultConstraintProvider : public SnapConstraintProvider {
 public:
  DefaultConstraintProvider(DefaultConstraintProvider &other) = delete;
  void operator=(const DefaultConstraintProvider &) = delete;
  static DefaultConstraintProvider *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);
  int GetProviderType() { return kDefault; }

  int GetCapabilities(BufferDescriptor desc, CapabilitySet *out);
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);

 private:
  DefaultConstraintProvider(){};
  ~DefaultConstraintProvider(){};
  static std::mutex default_provider_mutex_;
  static DefaultConstraintProvider *instance_;

  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
};
}  // namespace snapalloc

#endif  // __DEFAULT_CONSTRAINT_PROVIDER_H__