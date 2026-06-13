// Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __VIDEO_CONSTRAINT_PROVIDER_H__
#define __VIDEO_CONSTRAINT_PROVIDER_H__

#include <map>
#include <mutex>

#include "SnapConstraintProvider.h"
#include "SnapUtils.h"

namespace snapalloc {
class VideoConstraintProvider : public SnapConstraintProvider {
 public:
  VideoConstraintProvider(VideoConstraintProvider &other) = delete;
  void operator=(const VideoConstraintProvider &) = delete;
  static VideoConstraintProvider *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  int GetProviderType() { return kVideo; }

  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);

  int GetCapabilities(BufferDescriptor desc, CapabilitySet *out);
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);
  int BuildConstraints(BufferDescriptor desc, BufferConstraints *data);

 private:
  VideoConstraintProvider(){};
  ~VideoConstraintProvider(){};
  static std::mutex video_provider_mutex_;
  static VideoConstraintProvider *instance_;

  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
  std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map_;
};
}  // namespace snapalloc

#endif  // __VIDEO_CONSTRAINT_PROVIDER_H__