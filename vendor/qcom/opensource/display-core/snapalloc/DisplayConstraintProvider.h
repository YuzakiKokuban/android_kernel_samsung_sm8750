// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __DISPLAY_CONSTRAINT_PROVIDER_H__
#define __DISPLAY_CONSTRAINT_PROVIDER_H__

#include <map>
#include <mutex>

#include "SnapConstraintProvider.h"
#include "SnapUtils.h"

namespace snapalloc {
class DisplayConstraintProvider : public SnapConstraintProvider {
 public:
  DisplayConstraintProvider(DisplayConstraintProvider &other) = delete;
  void operator=(const DisplayConstraintProvider &) = delete;
  static DisplayConstraintProvider *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);
  int GetProviderType() { return kDisplay; }

  int GetCapabilities(BufferDescriptor desc, CapabilitySet *out);
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);
  int BuildConstraints(BufferDescriptor desc, BufferConstraints *data);

 private:
  DisplayConstraintProvider(){};
  ~DisplayConstraintProvider(){};
  static std::mutex display_provider_mutex_;
  static DisplayConstraintProvider *instance_;

  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
  std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map_;
};
}  // namespace snapalloc

#endif  // __DISPLAY_CONSTRAINT_PROVIDER_H__