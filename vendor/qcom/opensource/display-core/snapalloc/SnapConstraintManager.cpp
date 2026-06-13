// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapConstraintManager.h"

#include <iostream>

#include "GraphicsConstraintProvider.h"
#include "SnapConstraintParser.h"
#include "SnapTypes.h"
#include "SnapUtils.h"
#include "UBWCPolicy.h"

namespace snapalloc {

SnapConstraintManager *SnapConstraintManager::instance_{nullptr};
std::mutex SnapConstraintManager::constraint_mgr_mutex_;

SnapConstraintManager *SnapConstraintManager::GetInstance() {
  std::lock_guard<std::mutex> lock(constraint_mgr_mutex_);

  if (instance_ == nullptr) {
    instance_ = new SnapConstraintManager();
    instance_->Init();
  }
  return instance_;
}

void SnapConstraintManager::Init() {
  SnapConstraintParser *parser = SnapConstraintParser::GetInstance();
  parser->ParseFormats(&format_data_map_);

  default_provider_ = DefaultConstraintProvider::GetInstance(format_data_map_);

  ubwc_policy_ = UBWCPolicy::GetInstance(format_data_map_);

  GraphicsConstraintProvider *graphics_provider =
      GraphicsConstraintProvider::GetInstance(format_data_map_);
  providers_.push_back(graphics_provider);

  CameraConstraintProvider *camera_provider =
      CameraConstraintProvider::GetInstance(format_data_map_);
  providers_.push_back(camera_provider);

  DisplayConstraintProvider *disp_provider =
      DisplayConstraintProvider::GetInstance(format_data_map_);
  providers_.push_back(disp_provider);

  CPUConstraintProvider *cpu_provider = CPUConstraintProvider::GetInstance(format_data_map_);
  providers_.push_back(cpu_provider);

  VideoConstraintProvider *video_provider = VideoConstraintProvider::GetInstance(format_data_map_);
  providers_.push_back(video_provider);

  debug_ = Debug::GetInstance();
}

bool SnapConstraintManager::CanAllocateZSLForSecureCamera() {
  static bool inited = false;
  static bool can_allocate = true;
  if (inited) {
    return can_allocate;
  }
  std::string secure_preview_buffer_format_prop;
  debug_->IsSecurePreviewBufferFormatEnabled(&secure_preview_buffer_format_prop);
  if (!(secure_preview_buffer_format_prop.compare("420_sp") == 0)) {
    can_allocate = false;
  }
  inited = true;
  DLOGI("CanAllocateZSLForSecureCamera: %d", can_allocate);
  return can_allocate;
}

void SnapConstraintManager::GetImplDefinedFormat(
    vendor_qti_hardware_display_common_PixelFormat format,
    vendor_qti_hardware_display_common_BufferUsage usage,
    vendor_qti_hardware_display_common_PixelFormat *out_format,
    vendor_qti_hardware_display_common_PixelFormatModifier *out_modifier) {
  *out_format = format;
  if (format == vendor_qti_hardware_display_common_PixelFormat::IMPLEMENTATION_DEFINED ||
      format == vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) {
    if (((usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC ||
          usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_ALLOC_UBWC_PI ||
          usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_4R) &&
         format != vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) &&
        !(usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_10BIT)) {
      if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_4R) {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP;
        *out_modifier = PIXEL_FORMAT_MODIFIER_4R;
      } else {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP;
        // TODO: adding venus because gralloc adds venus here - re-evaluate if default should be something else
        *out_modifier = PIXEL_FORMAT_MODIFIER_VENUS;
      }
    } else if (usage & vendor_qti_hardware_display_common_BufferUsage::VIDEO_ENCODER) {
      if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_VIDEO_NV21_ENCODER) {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCrCb_420_SP;
        *out_modifier = PIXEL_FORMAT_MODIFIER_ENCODEABLE;
      } else if (usage & vendor_qti_hardware_display_common_BufferUsage::HW_IMAGE_ENCODER) {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP;
        *out_modifier = PIXEL_FORMAT_MODIFIER_HEIF;
      } else if (format == vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP;
        *out_modifier = PIXEL_FORMAT_MODIFIER_VENUS;
      } else {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP;
        *out_modifier = PIXEL_FORMAT_MODIFIER_ENCODEABLE;
      }
    } else if (usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_INPUT) {
      if (usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT) {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::NV21_ZSL;
      } else {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCrCb_420_SP;
      }
    } else if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_10BIT &&
               format != vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) {
      if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC) {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::TP10;
      } else {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCBCR_P010;
      }
    } else if (usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT) {
      if (format == vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) {
        if ((usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED) &&
            (!CanAllocateZSLForSecureCamera())) {
          *out_format = vendor_qti_hardware_display_common_PixelFormat::YCrCb_420_SP;
          *out_modifier = PIXEL_FORMAT_MODIFIER_VENUS;
        } else {
          *out_format = vendor_qti_hardware_display_common_PixelFormat::NV21_ZSL;
        }
      } else {
        *out_format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP;
        *out_modifier = PIXEL_FORMAT_MODIFIER_VENUS;
      }
    } else if (usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_OVERLAY) {
      // Default to RGBA8888
      *out_format = vendor_qti_hardware_display_common_PixelFormat::RGBA_8888;
    } else if (format == vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) {
      // If no other usage flags are detected, default the
      // flexible YUV format to NV21_ZSL
      *out_format = vendor_qti_hardware_display_common_PixelFormat::NV21_ZSL;
      DLOGD_IF(
          enable_logs,
          "Falling back to default YUV format - no camera/video specific format defined. Usage %lu",
          usage);
    }
  }
}

bool SnapConstraintManager::ValidateDescriptor(const BufferDescriptor &snap_desc) {
  if (format_data_map_.find(snap_desc.format) == format_data_map_.end()) {
    DLOGE("%s: Could not find entry for format %lu", __FUNCTION__,
          static_cast<uint64_t>(snap_desc.format));
    return false;
  }
  auto format_data = format_data_map_.at(snap_desc.format);
  int bpp = (format_data.bits_per_pixel) / 8;
  bpp = (bpp == -1 || bpp == 0) ? 1 : bpp;
  if (snap_desc.width == 0 || snap_desc.height == 0 ||
      (OVERFLOW_MUL((snap_desc.width * bpp), snap_desc.height)) ||
      (static_cast<int32_t>(snap_desc.format) <= 0) || snap_desc.layerCount <= 0) {
    DLOGE("Invalid Descriptor: uw%dxuh%d, format %d, layer_count %d, overflow_detected %d",
          snap_desc.width, snap_desc.height, snap_desc.format, snap_desc.layerCount,
          (OVERFLOW_MUL((snap_desc.width * bpp), snap_desc.height)) ? 1 : 0);
    return false;
  }

  return true;
}

Error SnapConstraintManager::GetAllocationData(
    BufferDescriptor in_desc, AllocData *out_ad,
    vendor_qti_hardware_display_common_BufferLayout *out_layout, BufferDescriptor *out_desc,
    int *out_priv_flags) {
  *out_desc = in_desc;

  // Check for width/height constraints for specific formats
  if (std::find(formats_with_w_h_constraints.begin(), formats_with_w_h_constraints.end(),
                out_desc->format) != formats_with_w_h_constraints.end()) {
    if ((!CheckWidthConstraints(out_desc->format, out_desc->width)) ||
        (!(CheckHeightConstraints(out_desc->format, out_desc->height)))) {
      DLOGE("Width and Height constraints for format %d not satisfied. Failing allocation",
            out_desc->format);
      return Error::BAD_VALUE;
    }
  }

  if ((
      in_desc.format == vendor_qti_hardware_display_common_PixelFormat::RAW10 ||
      in_desc.format == vendor_qti_hardware_display_common_PixelFormat::RAW12 ||
      in_desc.format == vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_SP)
      && (in_desc.usage & GPU_RENDER_TARGET || in_desc.usage & GPU_TEXTURE)) {
        DLOGE("Failing allocation for unsupported formats for GPU render/texture");
        return Error::BAD_VALUE;
  }

  if (in_desc.format == vendor_qti_hardware_display_common_PixelFormat::IMPLEMENTATION_DEFINED ||
      in_desc.format == vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888) {
    vendor_qti_hardware_display_common_PixelFormatModifier modifier = PIXEL_FORMAT_MODIFIER_NONE;
    GetImplDefinedFormat(in_desc.format, in_desc.usage, &(out_desc->format), &modifier);
    // Modifiers should not be set for impl defined format
    if (out_desc->additionalOptions.empty()) {
      vendor_qti_hardware_display_common_KeyValuePair desc_modifier = {
          .key = "pixel_format_modifier", .value = static_cast<uint64_t>(modifier)};
      out_desc->additionalOptions.emplace_back(desc_modifier);
    } else {
      for (int i = 0; i < out_desc->additionalOptions.size(); i++) {
        if (std::strcmp(out_desc->additionalOptions[i].key, "pixel_format_modifier") == 0) {
          out_desc->additionalOptions[i].value = static_cast<uint64_t>(modifier);
        }
      }
    }
  }

  if (!ValidateDescriptor(*out_desc)) {
    DLOGE("Buffer Descriptor is not valid. Failing allocation.");
    return Error::BAD_VALUE;
  }

  std::map<SnapConstraintProvider *, CapabilitySet> cap_map = GetCapabilities(*out_desc);
  auto err = Error::NONE;

  bool ubwc_disabled_prop = debug_->IsUBWCDisabled();
  bool ubwc_enabled = !ubwc_disabled_prop && ubwc_policy_->IsUBWCAlloc(*out_desc);
  SetSnapPrivateFlags(out_desc->format, out_desc->usage, ubwc_enabled, out_priv_flags);
  out_ad->uncached = UseUncached(out_desc->format, out_desc->usage, ubwc_enabled);

  if (ubwc_enabled) {
    DLOGD_IF(enable_logs, "IsUBWCAlloc is true");
    int ubwc_version = 0;
    for (auto const &cap : cap_map) {
      if (ubwc_version < cap.second.ubwc_caps.version) {
        ubwc_version = cap.second.ubwc_caps.version;
      }
    }
    ubwc_caps_.version = ubwc_version;
    err = ubwc_policy_->GetUBWCAlloc(*out_desc, ubwc_caps_, out_ad, out_layout);
  } else {
    if (ubwc_disabled_prop) {
      // Reset UBWC bit for UBWC disabled case
      if (static_cast<vendor_qti_hardware_display_common_BufferUsage>(
              (static_cast<uint64_t>(out_desc->usage) &
               static_cast<uint64_t>(
                   vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC)))) {
        out_desc->usage = static_cast<vendor_qti_hardware_display_common_BufferUsage>((
            static_cast<uint64_t>(out_desc->usage) ^
            static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC)));
      }
    }
    err = FetchAndMergeConstraints(*out_desc, cap_map, out_layout);
    if (err) {
      DLOGE("FetchAndMergeConstraints failed with error %d", err);
    } else {
      out_ad->size = out_layout->size_in_bytes;
    }
  }

  // Final buffer size must be aligned at minimum to page size
  vendor_qti_hardware_display_common_PixelFormatModifier pixel_format_modifier =
      static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
          GetPixelFormatModifier(*out_desc));
  auto align = GetDataAlignment(out_desc->format, out_desc->usage, pixel_format_modifier);
  OVERFLOW_ERR_RETURN(ALIGN(out_ad->size, align), out_desc->layerCount, OverflowType::MUL);
  out_ad->size = ALIGN(out_ad->size, align) * out_desc->layerCount;

  return err;
}

Error SnapConstraintManager::SetSnapPrivateFlags(
    vendor_qti_hardware_display_common_PixelFormat format,
    vendor_qti_hardware_display_common_BufferUsage usage, bool ubwc_enabled, int *snap_priv_flags) {
  if (ubwc_enabled) {
    *snap_priv_flags |= PRIV_FLAGS_UBWC_ALIGNED;
    *snap_priv_flags |= PRIV_FLAGS_TILE_RENDERED;
    // TODO: Include check for gpu - refer gralloc
    if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_ALLOC_UBWC_PI) {
      *snap_priv_flags |= PRIV_FLAGS_UBWC_ALIGNED_PI;
    }
  }

  if (IsTileRendered(format)) {
    *snap_priv_flags |= PRIV_FLAGS_TILE_RENDERED;
  }

  if (!UseUncached(format, usage, ubwc_enabled)) {
    *snap_priv_flags |= PRIV_FLAGS_CACHED;
  }

  return Error::NONE;
}

Error SnapConstraintManager::ConvertAlignedWidthFromBytesToPixels(
    vendor_qti_hardware_display_common_PixelFormat format, int width_in_bytes,
    int *width_in_pixels) {
  if (IsAstc(format)) {
    *width_in_pixels = width_in_bytes;
    return Error::NONE;
  }
  if (format_data_map_.find(format) == format_data_map_.end()) {
    DLOGE("Could not find entry for format %lu", static_cast<uint64_t>(format));
    return Error::UNSUPPORTED;
  }
  auto format_data = format_data_map_.at(format);
  *width_in_pixels = width_in_bytes / ((format_data.planes[0].sample_increment_bits) / 8);
  if (format == vendor_qti_hardware_display_common_PixelFormat::TP10) {
    OVERFLOW_ERR_RETURN(*width_in_pixels, 3, OverflowType::MUL);
    *width_in_pixels = (*width_in_pixels) * 3;
  }
  return Error::NONE;
}

Error SnapConstraintManager::ConstraintsToBufferLayout(
    BufferDescriptor desc, BufferConstraints *constraints,
    vendor_qti_hardware_display_common_BufferLayout *layout) {
  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGE("Could not find entry for format %d", static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }

  auto format_data = format_data_map_.at(desc.format);
  layout->bpp = (format_data.bits_per_pixel) / 8;

  if ((format_data.planes.size() > QTI_MAX_NUM_PLANES) || !format_data.planes.size()) {
    DLOGE("%s: Invalid format data plane count %d", __FUNCTION__, format_data.planes.size());
    return Error::BAD_VALUE;
  }

  layout->plane_count = format_data.planes.size();

  layout->aligned_width_in_bytes = constraints->planes[0].stride.horizontal_stride;
  DLOGD_IF(enable_logs,
           "layout->aligned_width_in_bytes %d constraints->planes[0].stride.horizontal_stride %d "
           "format_data.planes[0].sample_increment_bits %d in bytes %d",
           layout->aligned_width_in_bytes, constraints->planes[0].stride.horizontal_stride,
           format_data.planes[0].sample_increment_bits,
           format_data.planes[0].sample_increment_bits / 8);
  layout->aligned_height = constraints->planes[0].scanline.scanline;
  layout->size_in_bytes = 0;

  DLOGD_IF(enable_logs,
           "%s: format %d, plane_count %d, aligned_width_in_bytes %d, aligned_height %d plane size "
           "from format data %d",
           __FUNCTION__, desc.format, layout->plane_count, layout->aligned_width_in_bytes,
           layout->aligned_height, format_data.planes.size());
  int offset_sum = 0;
  for (int i = 0; i < format_data.planes.size(); i++) {
    // Populate fixed data

    DLOGD_IF(enable_logs, "%s: component count %d", __FUNCTION__,
             format_data.planes[i].components.size());
    layout->planes[i].component_count = format_data.planes[i].components.size();
    for (int j = 0; j < layout->planes[i].component_count; j++) {
      DLOGD_IF(enable_logs, "%s: component type %d size in bits %d offset in bits %d", __FUNCTION__,
               format_data.planes[i].components[j].type,
               format_data.planes[i].components[j].size_in_bits,
               format_data.planes[i].components[j].offset_in_bits);
      layout->planes[i].components[j].type = format_data.planes[i].components[j].type;
      layout->planes[i].components[j].size_in_bits =
          format_data.planes[i].components[j].size_in_bits;
      layout->planes[i].components[j].offset_in_bits =
          format_data.planes[i].components[j].offset_in_bits;
    }

    DLOGD_IF(enable_logs,
             "%s: sample_increment_bits %d horizontal_subsampling %d vertical_subsampling %d",
             __FUNCTION__, format_data.planes[i].sample_increment_bits,
             format_data.planes[i].horizontal_subsampling,
             format_data.planes[i].vertical_subsampling);
    layout->planes[i].sample_increment_bits = format_data.planes[i].sample_increment_bits;
    layout->planes[i].horizontal_subsampling = format_data.planes[i].horizontal_subsampling;
    layout->planes[i].vertical_subsampling = format_data.planes[i].vertical_subsampling;

    // Populate constraint-based data
    DLOGD_IF(enable_logs,
             "%s: format %d, i %d constraints->planes[i].stride.horizontal_stride %d "
             "constraints->planes[i].scanline.scanline %d constraints->planes[i].size_align %d",
             __FUNCTION__, desc.format, i, constraints->planes[i].stride.horizontal_stride,
             constraints->planes[i].scanline.scanline, constraints->planes[i].size_align);

    layout->planes[i].horizontal_stride_in_bytes = constraints->planes[i].stride.horizontal_stride;
    layout->planes[i].scanlines = constraints->planes[i].scanline.scanline;

    DLOGD_IF(enable_logs,
             "%s: format %d, i %d layout->planes[i].horizontal_stride_in_bytes %d  "
             "layout->planes[i].scanline %d",
             __FUNCTION__, desc.format, i, layout->planes[i].horizontal_stride_in_bytes,
             layout->planes[i].scanlines);

    OVERFLOW_ERR_RETURN(layout->planes[i].horizontal_stride_in_bytes, layout->planes[i].scanlines,
                        OverflowType::MUL);
    layout->planes[i].size_in_bytes =
        ALIGN(layout->planes[i].horizontal_stride_in_bytes * layout->planes[i].scanlines,
              constraints->planes[i].size_align);
    layout->planes[i].offset_in_bytes = offset_sum;

    if (desc.format == YCBCR_422_I) {
      // For interleaved formats, the stride for all components is the same
      // but the sizes must factor in subsampling
      layout->planes[i].size_in_bytes =
          ALIGN((layout->planes[i].horizontal_stride_in_bytes /
                 layout->planes[i].horizontal_subsampling) *
                    (layout->planes[i].scanlines / layout->planes[i].vertical_subsampling),
                constraints->planes[i].size_align);

      layout->planes[i].offset_in_bytes = layout->planes[i].components[0].offset_in_bits / 8.0;
      layout->planes[i].components[0].offset_in_bits = 0;
    }
    OVERFLOW_ERR_RETURN(offset_sum, layout->planes[i].size_in_bytes, OverflowType::ADD);
    offset_sum += layout->planes[i].size_in_bytes;
    DLOGD_IF(enable_logs,
             "%s: format %d, layout->planes[i].horizontal_stride_in_bytes %d "
             "layout->planes[i].scanlines %d "
             " constraints->planes[i].size_align %d layout->planes[i].size_in_bytes %d"
             " layout->planes[%d].offset_in_bytes %d",
             __FUNCTION__, desc.format, layout->planes[i].horizontal_stride_in_bytes,
             layout->planes[i].scanlines, constraints->planes[i].size_align,
             layout->planes[i].size_in_bytes, i, layout->planes[i].offset_in_bytes);
    OVERFLOW_ERR_RETURN(layout->size_in_bytes, layout->planes[i].size_in_bytes, OverflowType::ADD);
    layout->size_in_bytes += layout->planes[i].size_in_bytes;
  }
  layout->size_in_bytes = ALIGN(layout->size_in_bytes, constraints->size_align_bytes);

  return Error::NONE;
}

std::map<SnapConstraintProvider *, CapabilitySet> SnapConstraintManager::GetCapabilities(
    BufferDescriptor desc) {
  std::map<SnapConstraintProvider *, CapabilitySet> caps_map;
  for (auto const &provider : providers_) {
    CapabilitySet caps;
    provider->GetCapabilities(desc, &caps);
    if (caps.enabled) {
      caps_map.insert(std::make_pair(provider, caps));
    }
  }

  return caps_map;
}

Error SnapConstraintManager::MergeConstraints(std::vector<BufferConstraints> constraint_sets,
                                              BufferConstraints *merged_constraints) {
  *merged_constraints = constraint_sets[0];
  if (constraint_sets.size() > 1) {
    for (int i = 1; i < constraint_sets.size(); i++) {
      if (!constraint_sets[i].planes.empty()) {
        for (int j = 0; j < constraint_sets[i].planes.size(); j++) {
          merged_constraints->planes[j].stride.horizontal_stride =
              std::max(merged_constraints->planes[j].stride.horizontal_stride,
                       constraint_sets[i].planes[j].stride.horizontal_stride);

          merged_constraints->planes[j].scanline.scanline =
              std::max(merged_constraints->planes[j].scanline.scanline,
                       constraint_sets[i].planes[j].scanline.scanline);

          merged_constraints->planes[j].size_align = std::lcm(
              merged_constraints->planes[j].size_align, constraint_sets[i].planes[j].size_align);
        }
      }
      merged_constraints->size_align_bytes =
          std::lcm(merged_constraints->size_align_bytes, constraint_sets[i].size_align_bytes);
    }
  }
  return Error::NONE;
}

uint8_t SnapConstraintManager::GetBitsPerPixel(
    vendor_qti_hardware_display_common_PixelFormat format) {
  if (format_data_map_.find(format) == format_data_map_.end()) {
    DLOGE("Could not find entry for format %d", static_cast<uint64_t>(format));
    return Error::NONE;
  }

  auto format_data = format_data_map_.at(format);
  return format_data.bits_per_pixel;
}

Error SnapConstraintManager::AlignmentToAlignedConstraints(BufferDescriptor desc,
                                                           BufferConstraints alignment,
                                                           BufferConstraints *aligned) {
  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGE("Could not find entry for format %d", static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }

  auto format_data = format_data_map_.at(desc.format);
  DLOGD_IF(enable_logs, "alignment.size_align_bytes %d", alignment.size_align_bytes);
  aligned->size_align_bytes = alignment.size_align_bytes;
  if (!alignment.planes.empty()) {
    DLOGD_IF(enable_logs, "alignment.planes.size() %d", alignment.planes.size());
    for (int i = 0; i < alignment.planes.size(); i++) {
      PlaneConstraints plane;
      plane.components = alignment.planes[i].components;
      plane.alignment_type = ALIGNED_OUTPUT;

      // TODO: factor in subsampling from format data here for CbCr
      DLOGD_IF(enable_logs, "alignment.planes[i].stride.horizontal_stride_align %d",
               alignment.planes[i].stride.horizontal_stride_align);
      DLOGD_IF(enable_logs, "alignment.planes[i].scanline.scanline_align %d",
               alignment.planes[i].scanline.scanline_align);
      DLOGD_IF(enable_logs, "alignment.planes[i].size_align %d", alignment.planes[i].size_align);

      // TODO: If default constraint provider returns an aligned output for
      // YV12, move this special handling to default constraint provider
      if ((desc.format == vendor_qti_hardware_display_common_PixelFormat::YV12) &&
          (alignment.planes[i].components[0] != PLANE_LAYOUT_COMPONENT_TYPE_Y)) {
        OVERFLOW_ERR_RETURN((desc.width / 2), (format_data.planes[0].sample_increment_bits / 8),
                            OverflowType::MUL);
        plane.stride.horizontal_stride =
            ALIGN((desc.width / 2) * (format_data.planes[0].sample_increment_bits / 8),
                  alignment.planes[i].stride.horizontal_stride_align);
        plane.scanline.scanline =
            ALIGN(desc.height >> 1, alignment.planes[i].scanline.scanline_align);
      } else {
        // bpp = 3 case special handling. Multiply by bpp to convert into bytes
        if ((desc.format == vendor_qti_hardware_display_common_PixelFormat::RGB_888) ||
            (desc.format == vendor_qti_hardware_display_common_PixelFormat::BGR_888)) {
          plane.stride.horizontal_stride =
              ALIGN(desc.width, alignment.planes[i].stride.horizontal_stride_align) *
              (format_data.bits_per_pixel / 8);
        } else {
          OVERFLOW_ERR_RETURN(desc.width, (format_data.planes[0].sample_increment_bits / 8),
                              OverflowType::MUL);
          plane.stride.horizontal_stride =
              ALIGN(desc.width * format_data.planes[0].sample_increment_bits / 8,
                    alignment.planes[i].stride.horizontal_stride_align);
        }
        if ((IsYuv(desc.format)) &&
            ((alignment.planes[i].components[0] == PLANE_LAYOUT_COMPONENT_TYPE_CB) ||
             (alignment.planes[i].components[0] == PLANE_LAYOUT_COMPONENT_TYPE_CR))) {
          int height = desc.height;
          if (format_data.planes[i].vertical_subsampling == 2) {
            // height + 1 to avoid height being rounded down due to truncation when dividing by
            // vertical_subsampling
            height = height + 1;
          }
          plane.scanline.scanline = ALIGN((height / format_data.planes[i].vertical_subsampling),
                                          alignment.planes[i].scanline.scanline_align);
        } else {
          plane.scanline.scanline = ALIGN(desc.height, alignment.planes[i].scanline.scanline_align);
        }
      }

      plane.size_align = alignment.planes[i].size_align;
      aligned->planes.push_back(plane);
    }
  }
  return Error::NONE;
}

Error SnapConstraintManager::FetchAndMergeConstraints(
    BufferDescriptor desc, std::map<SnapConstraintProvider *, CapabilitySet> const &providers,
    vendor_qti_hardware_display_common_BufferLayout *out_layout) {
  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGE("Could not find entry for format %d", static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }

  std::vector<BufferConstraints> constraint_sets;

  uint8_t constraints_provided = 0;
  for (auto const &[provider, cap] : providers) {
    BufferConstraints provider_constraints;
    provider->GetConstraints(desc, &provider_constraints);
    int provider_type = provider->GetProviderType();
    if (provider_constraints.planes.empty()) {
      DLOGD_IF(enable_logs, "Provider type %d has not given alignment data - skipping",
               provider_type);
      continue;
    }
    if (provider_constraints.planes[0].alignment_type == ALIGNED_OUTPUT) {
      DLOGD_IF(enable_logs, "Provider type %d has given aligned output", provider_type);
      ++constraints_provided;
      constraint_sets.push_back(provider_constraints);
    } else if (provider_constraints.planes[0].alignment_type == ALIGNMENT) {
      DLOGD_IF(enable_logs, "Provider type %d has given alignment values", provider_type);

      // Convert to aligned output
      BufferConstraints aligned_constraints;
      DLOGD_IF(enable_logs, "calling AlignmentToAlignedConstraints");

      int err = AlignmentToAlignedConstraints(desc, provider_constraints, &aligned_constraints);
      if (err) {
        DLOGW("Failed to convert alignment to aligned constraints - skipping");
      } else {
        ++constraints_provided;
        constraint_sets.push_back(aligned_constraints);
      }
    } else {
      DLOGE("Alignment type undefined");
    }
  }

  // If no device constraint providers or none returned a constraint set, query the default provider
  if (!constraints_provided) {
    DLOGD_IF(enable_logs, "Query the default provider - no constraints provided");

    BufferConstraints default_constraints;
    default_provider_->GetConstraints(desc, &default_constraints);

    if (default_constraints.planes.empty()) {
      DLOGE("Error - no constraint data available");
      return Error::BAD_VALUE;
    }

    // Default provider uses json - need to align values
    BufferConstraints aligned_constraints;
    DLOGD_IF(enable_logs, "calling AlignmentToAlignedConstraints");
    int err = AlignmentToAlignedConstraints(desc, default_constraints, &aligned_constraints);
    if (err) {
      DLOGE(
          "Failed to convert alignment to aligned constraints - failing allocatsion due to no "
          "valid constraints");
      return Error::BAD_VALUE;
    }
    constraint_sets.push_back(aligned_constraints);
  }

  BufferConstraints merged_constraints;
  MergeConstraints(constraint_sets, &merged_constraints);

  ConstraintsToBufferLayout(desc, &merged_constraints, out_layout);

  // In the graphics only case, align the buffer size using Adreno API
  if (providers.size() == 1) {
    auto it = providers.begin();
    if (it->first->GetProviderType() == kGraphics) {
      DLOGD_IF(enable_logs, "getting size and metadata from graphics");
      GraphicsConstraintProvider *graphics_provider =
          static_cast<GraphicsConstraintProvider *>(it->first);
      vendor_qti_hardware_display_common_GraphicsMetadata graphics_metadata = {};
      int ret = graphics_provider->GetInitialMetadata(desc, &graphics_metadata, false);
      if (!ret) {
        auto size = graphics_provider->AdrenoGetAlignedGpuBufferSize(graphics_metadata.data);
        if (size > 0)
          out_layout->size_in_bytes = size;
      }
    }
  }

  DLOGD_IF(enable_logs, "out_layout->size_in_bytes %d at line %d", out_layout->size_in_bytes,
           __LINE__);

  return Error::NONE;
}

// TODO: remove this once json is primary source on-target
uint32_t SnapConstraintManager::GetDataAlignment(
    vendor_qti_hardware_display_common_PixelFormat format,
    vendor_qti_hardware_display_common_BufferUsage usage,
    vendor_qti_hardware_display_common_PixelFormatModifier modifier) {
  uint32_t align = PAGE_SIZE;

  if (format == vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP &&
      modifier == PIXEL_FORMAT_MODIFIER_TILED) {
    align = SIZE_8K;
  }

  if (usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED) {
    if (usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT ||
        usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_SECURE_DISPLAY) {
      // The alignment here reflects qsee mmu V7L/V8L requirement
      align = SIZE_2MB;
    } else {
      align = SECURE_ALIGN;
    }
  }

  return align;
}

bool SnapConstraintManager::UseUncached(vendor_qti_hardware_display_common_PixelFormat format,
                                        vendor_qti_hardware_display_common_BufferUsage usage,
                                        bool ubwc_enabled) {
  if ((usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_UNCACHED) ||
      (usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED)) {
    return true;
  }

  if ((usage & vendor_qti_hardware_display_common_BufferUsage::CPU_READ_MASK) ==
      static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::CPU_READ_RARELY)) {
    return true;
  }

  if ((usage & vendor_qti_hardware_display_common_BufferUsage::CPU_WRITE_MASK) ==
      static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::CPU_WRITE_RARELY)) {
    return true;
  }

  if ((usage & vendor_qti_hardware_display_common_BufferUsage::SENSOR_DIRECT_DATA) ||
      (usage & vendor_qti_hardware_display_common_BufferUsage::GPU_DATA_BUFFER)) {
    return true;
  }

  if (ubwc_enabled) {
    return true;
  }

  return false;
}

}  // namespace snapalloc
