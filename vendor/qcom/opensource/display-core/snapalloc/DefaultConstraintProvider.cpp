// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DefaultConstraintProvider.h"

#include <dlfcn.h>
#include <fstream>
#include <iostream>

#include "SnapConstraintParser.h"

namespace snapalloc {
DefaultConstraintProvider *DefaultConstraintProvider::instance_{nullptr};
std::mutex DefaultConstraintProvider::default_provider_mutex_;

DefaultConstraintProvider *DefaultConstraintProvider::GetInstance(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  std::lock_guard<std::mutex> lock(default_provider_mutex_);

  if (instance_ == nullptr) {
    instance_ = new DefaultConstraintProvider();
    instance_->Init(format_data_map);
  }
  return instance_;
}

void DefaultConstraintProvider::Init(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  SnapConstraintParser *parser = SnapConstraintParser::GetInstance();
  parser->ParseAlignments("/vendor/etc/display/default_alignments.json", &constraint_set_map_);
}

int DefaultConstraintProvider::GetCapabilities(BufferDescriptor desc, CapabilitySet *out) {
  // Only call default if no others are enabled so that it can always set out->enabled to true
  out->enabled = true;
  // Default constraint provider is not tied to HW, so it does not have a UBWC version
  out->ubwc_caps.version = 0;
  (void)desc;
  DLOGD_IF(enable_logs, "DefaultConstraintProvider is enabled");
  return 0;
}

int DefaultConstraintProvider::GetConstraints(BufferDescriptor desc, BufferConstraints *out) {
  if (constraint_set_map_.empty()) {
    DLOGD_IF(enable_logs, "Default constraint set map is empty");
    return -1;
  }
  if (constraint_set_map_.find(desc.format) != constraint_set_map_.end()) {
    *out = constraint_set_map_.at(desc.format);
  } else {
    DLOGD_IF(enable_logs, "Default could not find entry for format %lu",
             static_cast<uint64_t>(desc.format));
  }
  return 0;
}

}  // namespace snapalloc