// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DisplayConstraintProvider.h"

#include <dlfcn.h>
#include <fstream>
#include <iostream>

#include "SnapConstraintParser.h"

namespace snapalloc {
DisplayConstraintProvider *DisplayConstraintProvider::instance_{nullptr};
std::mutex DisplayConstraintProvider::display_provider_mutex_;

DisplayConstraintProvider *DisplayConstraintProvider::GetInstance(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  std::lock_guard<std::mutex> lock(display_provider_mutex_);

  if (instance_ == nullptr) {
    instance_ = new DisplayConstraintProvider();
    instance_->Init(format_data_map);
  }
  return instance_;
}

void DisplayConstraintProvider::Init(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  SnapConstraintParser *parser = SnapConstraintParser::GetInstance();
  parser->ParseAlignments("/vendor/etc/display/display_alignments.json", &constraint_set_map_);
  if (!format_data_map.empty()) {
    format_data_map_ = format_data_map;
  } else {
    parser->ParseFormats(&format_data_map_);
  }
}

int DisplayConstraintProvider::GetCapabilities(BufferDescriptor desc, CapabilitySet *out) {
  out->ubwc_caps.version = 0;

  if (desc.usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_OVERLAY ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_CLIENT_TARGET ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_CURSOR) {
    DLOGD_IF(enable_logs, "DisplayConstraintProvider is enabled");
    out->enabled = true;
  } else {
    DLOGD_IF(enable_logs, "DisplayConstraintProvider is not enabled");
    out->enabled = false;
  }

  if (IsAstc(desc.format)) {
    out->enabled = false;
  }

  return 0;
}

int DisplayConstraintProvider::BuildConstraints(BufferDescriptor desc, BufferConstraints *data) {
  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGW("Could not find entry for format %lu", static_cast<uint64_t>(desc.format));
    return -1;
  }

  FormatData format_data = format_data_map_.at(desc.format);
  // Hardcode based on json
  data->size_align_bytes = 4096;
  int ret_val = 0;
  for (auto const &plane : format_data.planes) {
    PlaneConstraints plane_layout;
    // Hardcode for json files
    plane_layout.alignment_type = ALIGNED_OUTPUT;
    for (auto const &component : plane.components) {
      vendor_qti_hardware_display_common_PlaneLayoutComponentType component_type = component.type;
      // TODO: Skip if plane stride, scanline, size already filled out from first component
      MmmColorFormatMapper mapper = MmmColorFormatMapper();
      int mmm_color_format = 0;
      uint64_t pixel_format_modifier = GetPixelFormatModifier(desc);
      mmm_color_format = mapper.MapPixelFormatWithMmmColorFormat(
          desc.format, desc.usage,
          static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
              pixel_format_modifier),
          false);  // false indicates not ubwc
      if (mmm_color_format < 0) {
        DLOGW("Failed to get format mapping to use mmm_color_fmt");
        return -1;
      }
      switch (component_type) {
        case PLANE_LAYOUT_COMPONENT_TYPE_Y:
          plane_layout.stride.horizontal_stride = mapper.GetYStride(mmm_color_format, desc.width);
          plane_layout.scanline.scanline = mapper.GetYScanlines(mmm_color_format, desc.height);
          plane_layout.size_align = 1;
          break;
        case PLANE_LAYOUT_COMPONENT_TYPE_R:
        case PLANE_LAYOUT_COMPONENT_TYPE_G:
        case PLANE_LAYOUT_COMPONENT_TYPE_B:
        case PLANE_LAYOUT_COMPONENT_TYPE_A:
          plane_layout.stride.horizontal_stride = mapper.GetRgbStride(mmm_color_format, desc.width);
          plane_layout.scanline.scanline = mapper.GetRgbScanlines(mmm_color_format, desc.height);
          plane_layout.size_align = 1;
          break;
        case PLANE_LAYOUT_COMPONENT_TYPE_CB:
        case PLANE_LAYOUT_COMPONENT_TYPE_CR:
          plane_layout.stride.horizontal_stride = mapper.GetUVStride(mmm_color_format, desc.width);
          plane_layout.scanline.scanline = mapper.GetUVScanlines(mmm_color_format, desc.height);
          plane_layout.size_align = 1;
        default:
          break;
      }
      plane_layout.components.push_back(component_type);
    }
    data->planes.push_back(plane_layout);
  }
  return 0;
}

int DisplayConstraintProvider::GetConstraints(BufferDescriptor desc, BufferConstraints *out) {
  (void)out;
#ifdef __ANDROID__
  DLOGD_IF(enable_logs, "Using display libs for alignment calculations");
  BufferConstraints data;
  int status = 0;
  status = BuildConstraints(desc, &data);
  if (status != Error::NONE) {
    DLOGW("Error while getting constraints from display libs width %d, height %d, format %d",
          desc.width, desc.height, static_cast<uint64_t>(desc.format));
    return -1;
  }
  *out = data;
  return 0;
#endif
  if (constraint_set_map_.empty()) {
    DLOGW("DisplayConstraintProvider constraint set map is empty");
    return -1;
  }
  if (constraint_set_map_.find(desc.format) != constraint_set_map_.end()) {
    *out = constraint_set_map_.at(desc.format);
  } else {
    DLOGW("DisplayConstraintProvider could not find entry for format %lu",
          static_cast<uint64_t>(desc.format));
  }
  return 0;
}

}  // namespace snapalloc
