// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "UBWCPolicy.h"

#include <fstream>
#include <iostream>

#include "SnapConstraintDefs.h"
#include "SnapTypes.h"
#include "SnapUtils.h"

namespace snapalloc {
UBWCPolicy *UBWCPolicy::instance_{nullptr};
std::mutex UBWCPolicy::ubwc_policy_mutex_;

UBWCPolicy::UBWCPolicy(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  constraint_parser_ = SnapConstraintParser::GetInstance();
  graphics_provider_ = GraphicsConstraintProvider::GetInstance(format_data_map);
  debug_ = Debug::GetInstance();
}

UBWCPolicy *UBWCPolicy::GetInstance(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  std::lock_guard<std::mutex> lock(ubwc_policy_mutex_);

  if (instance_ == nullptr) {
    instance_ = new UBWCPolicy(format_data_map);
    instance_->Init(format_data_map);
  }
  return instance_;
}

void UBWCPolicy::Init(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  if (!format_data_map.empty()) {
    format_data_map_ = format_data_map;
  } else {
    constraint_parser_->ParseFormats(&format_data_map_);
  }
#ifndef __ANDROID__
  constraint_parser_->ParseAlignments("/vendor/etc/display/ubwc_alignments.json",
                                      &constraint_set_map_);
#endif
}

bool UBWCPolicy::IsUBWCAlloc(BufferDescriptor desc) {
  if (debug_->IsUBWCDisabled()) {
    return false;
  }

  // Explicit UBWC formats passed by the clients.Ignore the usage bits and allow UBWC.
  if (GetPixelFormatModifier(desc) ==
      static_cast<uint64_t>(vendor_qti_hardware_display_common_PixelFormatModifier::
                                PIXEL_FORMAT_MODIFIER_EXPLICIT_UBWC)) {
    DLOGI("%s - Explicit ubwc format %d passed by the clients", __FUNCTION__, desc.format);
    return true;
  }

  // TODO: remove explicit R8 handling
  if (desc.format == vendor_qti_hardware_display_common_PixelFormat::R_8) {
    return false;
  }

  // Explicit UBWC formats will have UBWC flags set - check for these first
  bool enable = false;
  if (desc.usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_ALLOC_UBWC_PI) {
    enable = true;
  }

  if (IsTileRendered(desc.format)) {
    enable = true;
  }

  if (IsUbwcSupported(desc.format) &&
      (desc.usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_CLIENT_TARGET)) {
    enable = true;
  }

  if (enable && (desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_TEXTURE ||
                 desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_RENDER_TARGET)) {
    vendor_qti_hardware_display_common_PixelFormatModifier pixel_format_modifier =
        static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
            GetPixelFormatModifier(desc));
    enable = graphics_provider_->IsUBWCSupportedByGPU(desc.format, pixel_format_modifier);
  }

  if (IsAstc(desc.format)) {
    enable = false;
  }

  // TODO: remove for UBWC-P
  if (enable && !CpuCanAccess(desc.usage)) {
    return true;
  }

  return false;
}

vendor_qti_hardware_display_common_Compression UBWCPolicy::GetUBWCScheme(
    vendor_qti_hardware_display_common_PixelFormat format,
    vendor_qti_hardware_display_common_BufferUsage usage) {
#ifdef DRM_FORMAT_MOD_QCOM_LOSSY_8_5
  if (format == vendor_qti_hardware_display_common_PixelFormat::RGBA_8888) {
    if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_L_8_TO_5) {
      return QTI_COMPRESSION_UBWC_LOSSY_8_TO_5;
    }
    if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_L_2_TO_1) {
      return QTI_COMPRESSION_UBWC_LOSSY_2_TO_1;
    }
  }
#endif

  if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC ||
      usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_ALLOC_UBWC_PI) {
    return QTI_COMPRESSION_UBWC;
  }
  return COMPRESSION_NONE;
}

uint64_t UBWCPolicy::GetMetaPlaneSize(uint64_t width, uint64_t height, uint32_t block_width,
                                      uint32_t block_height, uint64_t stride_align,
                                      uint64_t scanline_align, uint64_t size_align) {
  uint64_t size = 0;
  int meta_width = 0;
  int meta_height = 0;
  meta_height = ALIGN(((height + block_height - 1) / block_height), scanline_align);
  meta_width = ALIGN(((width + block_width - 1) / block_width), stride_align);
  if (OVERFLOW_MUL((uint64_t)meta_width, (uint64_t)meta_height)) {
    DLOGW("%s: Size overflow! %d x %d", meta_width, meta_height);
    return 0;
  }
  size = static_cast<uint64_t>(ALIGN(((uint64_t)meta_width * (uint64_t)meta_height), size_align));
  return size;
}

int UBWCPolicy::GetBatchSize(vendor_qti_hardware_display_common_PixelFormatModifier modifier) {
  int batchsize = 1;
  switch (modifier) {
    case PIXEL_FORMAT_MODIFIER_UBWC_FLEX:
      batchsize = 16;
      break;
    case PIXEL_FORMAT_MODIFIER_UBWC_FLEX_2_BATCH:
      batchsize = 2;
      break;
    case PIXEL_FORMAT_MODIFIER_UBWC_FLEX_4_BATCH:
      batchsize = 4;
      break;
    case PIXEL_FORMAT_MODIFIER_UBWC_FLEX_8_BATCH:
      batchsize = 8;
      break;
    default:
      break;
  }
  return batchsize;
}

int UBWCPolicy::OffTargetAlloc(BufferDescriptor desc, AllocData *out_ad,
                               vendor_qti_hardware_display_common_BufferLayout *out_layout) {
  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGE("Could not find entry for format", static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }

  FormatData format_data = format_data_map_.at(desc.format);
  // TODO: Truncation possible - update for off-target tests
  uint32_t bpp = (format_data.bits_per_pixel) / 8;
  if (format_data.planes.size() > QTI_MAX_NUM_PLANES) {
    DLOGE("Format data plane count %d exceeds max", format_data.planes.size());
    return Error::UNSUPPORTED;
  }

  BufferConstraints ubwc_constraints;

  if (constraint_set_map_.empty()) {
    DLOGE("Constraint set map is empty");
    return Error::NO_RESOURCES;
  }
  if (constraint_set_map_.find(desc.format) != constraint_set_map_.end()) {
    ubwc_constraints = constraint_set_map_.at(desc.format);
  } else {
    DLOGE("%s: could not find entry for format %lu", __FUNCTION__,
          static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }
  if (ubwc_constraints.planes.empty()) {
    DLOGE("Alignment data is not present for the format %d", static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }

  // Buffer size calculations
  int buffer_size = 0;
  for (auto const &plane_constraints : ubwc_constraints.planes) {
    uint64_t plane_size = 0;

    if (plane_constraints.components[0] == PLANE_LAYOUT_COMPONENT_TYPE_META) {
      plane_size = GetMetaPlaneSize(
          desc.width, desc.height, plane_constraints.block_width, plane_constraints.block_height,
          plane_constraints.stride.horizontal_stride_align,
          plane_constraints.scanline.scanline_align, ubwc_constraints.size_align_bytes);
    } else {
      OVERFLOW_ERR_RETURN(desc.width, bpp, OverflowType::MUL);
      OVERFLOW_ERR_RETURN(
          (ALIGN(desc.width * bpp, plane_constraints.stride.horizontal_stride_align)),
          (ALIGN(desc.height, plane_constraints.scanline.scanline_align)), OverflowType::MUL);
      plane_size =
          ALIGN(((ALIGN(desc.width * bpp, plane_constraints.stride.horizontal_stride_align)) *
                 (ALIGN(desc.height, plane_constraints.scanline.scanline_align))),
                ubwc_constraints.size_align_bytes);
    }
    buffer_size += plane_size;
  }
  out_ad->size = buffer_size;

  // Plane Layout
  out_layout->bpp = bpp;
  out_layout->plane_count = format_data.planes.size();
  for (int plane_index = 0; plane_index < format_data.planes.size(); plane_index++) {
    // Populate fixed data
    out_layout->planes[plane_index].component_count =
        format_data.planes[plane_index].components.size();
    for (int j = 0; j < out_layout->planes[plane_index].component_count; j++) {
      out_layout->planes[plane_index].components[j].type =
          format_data.planes[plane_index].components[j].type;
      out_layout->planes[plane_index].components[j].size_in_bits =
          format_data.planes[plane_index].components[j].size_in_bits;
      out_layout->planes[plane_index].components[j].offset_in_bits =
          format_data.planes[plane_index].components[j].offset_in_bits;
    }
    out_layout->planes[plane_index].sample_increment_bits =
        format_data.planes[plane_index].sample_increment_bits;
    out_layout->planes[plane_index].horizontal_subsampling =
        format_data.planes[plane_index].horizontal_subsampling;
    out_layout->planes[plane_index].vertical_subsampling =
        format_data.planes[plane_index].vertical_subsampling;
    PlaneConstraints plane_layout_constraint = ubwc_constraints.planes.at(plane_index);
    // TODO: factor in subsampling here - off-target tests
    OVERFLOW_ERR_RETURN(desc.width, bpp, OverflowType::MUL);
    out_layout->planes[plane_index].horizontal_stride_in_bytes =
        ALIGN(desc.width * bpp, plane_layout_constraint.stride.horizontal_stride);
    // TODO: factor in subsampling here - off-target tests
    OVERFLOW_ERR_RETURN(desc.height, bpp, OverflowType::MUL);
    out_layout->planes[plane_index].scanlines =
        ALIGN(desc.height * bpp, plane_layout_constraint.scanline.scanline);
    out_layout->planes[plane_index].size_in_bytes =
        ALIGN((out_layout->planes[plane_index].horizontal_stride_in_bytes *
               out_layout->planes[plane_index].scanlines),
              plane_layout_constraint.size_align);
  }
  out_layout->aligned_width_in_bytes = out_layout->planes[0].horizontal_stride_in_bytes;
  out_layout->aligned_height = out_layout->planes[0].scanlines;
  return 0;
}

Error UBWCPolicy::GetUBWCAlloc(BufferDescriptor desc, UBWCCapabilities caps, AllocData *out_ad,
                               vendor_qti_hardware_display_common_BufferLayout *out_layout) {
  (void)desc;
  (void)caps;

#ifdef DRM_FORMAT_MOD_QCOM_LOSSY_8_5
  if ((desc.usage & vendor_qti_hardware_display_common_BufferUsage::COMPOSER_CLIENT_TARGET) &&
      ((desc.usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_L_8_TO_5) ||
      (desc.usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_L_2_TO_1))) {
    DLOGE("Lossy not supported for framebuffer target");
    return Error::UNSUPPORTED;
  }
#endif

#ifdef __ANDROID__
  if (format_data_map_.empty()) {
    DLOGE("Error while reading the format data");
    return Error::UNSUPPORTED;
  }

  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGE("%s: could not find entry for format %lu", __FUNCTION__,
          static_cast<uint64_t>(desc.format));
    return Error::UNSUPPORTED;
  }

  FormatData format_data = format_data_map_.at(desc.format);
  // TODO: Truncation possible if not divisible by 8
  out_layout->bpp = format_data.bits_per_pixel / 8;
  if (format_data.planes.size() > QTI_MAX_NUM_PLANES) {
    DLOGE("Format data plane count %d exceeds max", format_data.planes.size());
    return Error::BAD_VALUE;
  }
  // TODO: Remove hard-coding
  int alignment = 4096;
  int height = desc.height;
  // Divide input height by 2 for interlaced case
  for (auto &type : desc.additionalOptions) {
    if (std::strcmp(type.key, "interlaced") == 0) {
      if (type.value == 1) {
        height = (height + 1) >> 1;
        ;
      }
    }
  }

  MmmColorFormatMapper mapper = MmmColorFormatMapper();
  int mmm_color_format = 0;
  vendor_qti_hardware_display_common_PixelFormatModifier pixel_format_modifier =
      static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
          GetPixelFormatModifier(desc));
  mmm_color_format = mapper.MapPixelFormatWithMmmColorFormat(
      desc.format, desc.usage, pixel_format_modifier, true);  // true indicates ubwc is enabled
  if (mmm_color_format != -1) {
    // Double the number of planes to account for meta planes
    out_layout->plane_count = format_data.planes.size() * 2;
    if (IsYuv(desc.format)) {
      OVERFLOW_ERR_RETURN(mapper.GetYStride(mmm_color_format, desc.width) * out_layout->bpp,
                          mapper.GetYScanlines(mmm_color_format, desc.height), OverflowType::MUL);
    } else if (IsRgb(desc.format)) {
      OVERFLOW_ERR_RETURN(mapper.GetRgbStride(mmm_color_format, desc.width) * out_layout->bpp,
                          mapper.GetRgbScanlines(mmm_color_format, desc.height), OverflowType::MUL);
    } else {
      DLOGD_IF(enable_logs, "Overflow check skipped for format %d", static_cast<int>(desc.format));
    }

    out_ad->size = mapper.GetBufferSize(mmm_color_format, desc.width, height);
    if ((pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX) ||
        (pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_2_BATCH) ||
        (pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_4_BATCH) ||
        (pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_8_BATCH)) {
      out_ad->size *= GetBatchSize(pixel_format_modifier);
    }
    int meta_offset = 0;
    // Buffer layout
    for (int plane_index = 0; plane_index < format_data.planes.size(); plane_index++) {
      int meta_plane_index = plane_index + format_data.planes.size();
      int data_plane_index = plane_index;
      // Populate fixed data
      out_layout->planes[data_plane_index].component_count =
          format_data.planes[plane_index].components.size();
      out_layout->planes[meta_plane_index].component_count =
          format_data.planes[plane_index].components.size() + 1;
      for (int j = 0; j < out_layout->planes[data_plane_index].component_count; j++) {
        out_layout->planes[meta_plane_index].components[j].type =
            format_data.planes[plane_index].components[j].type;
        out_layout->planes[data_plane_index].components[j].type =
            format_data.planes[plane_index].components[j].type;
        out_layout->planes[data_plane_index].components[j].size_in_bits =
            format_data.planes[plane_index].components[j].size_in_bits;
        out_layout->planes[data_plane_index].components[j].offset_in_bits =
            format_data.planes[plane_index].components[j].offset_in_bits;
      }
      // Set last component in meta plane to META
      out_layout->planes[meta_plane_index]
          .components[out_layout->planes[meta_plane_index].component_count - 1]
          .type = PLANE_LAYOUT_COMPONENT_TYPE_META;

      out_layout->planes[data_plane_index].sample_increment_bits =
          format_data.planes[plane_index].sample_increment_bits;
      out_layout->planes[data_plane_index].horizontal_subsampling =
          format_data.planes[plane_index].horizontal_subsampling;
      out_layout->planes[data_plane_index].vertical_subsampling =
          format_data.planes[plane_index].vertical_subsampling;
      vendor_qti_hardware_display_common_PlaneLayoutComponentType component_type =
          format_data.planes[plane_index].components[0].type;
      switch (component_type) {
        case PLANE_LAYOUT_COMPONENT_TYPE_Y:
          out_layout->planes[data_plane_index].horizontal_stride_in_bytes =
              mapper.GetYStride(mmm_color_format, desc.width);
          out_layout->planes[data_plane_index].scanlines =
              mapper.GetYScanlines(mmm_color_format, height);
          out_layout->planes[meta_plane_index].horizontal_stride_in_bytes =
              mapper.GetYMetaStride(mmm_color_format, desc.width);
          out_layout->planes[meta_plane_index].scanlines =
              mapper.GetYMetaScanlines(mmm_color_format, height);
          break;
        case PLANE_LAYOUT_COMPONENT_TYPE_R:
        case PLANE_LAYOUT_COMPONENT_TYPE_G:
        case PLANE_LAYOUT_COMPONENT_TYPE_B:
        case PLANE_LAYOUT_COMPONENT_TYPE_A:
          out_layout->planes[data_plane_index].horizontal_stride_in_bytes =
              mapper.GetRgbStride(mmm_color_format, desc.width);
          out_layout->planes[data_plane_index].scanlines =
              mapper.GetRgbScanlines(mmm_color_format, height);
          out_layout->planes[meta_plane_index].horizontal_stride_in_bytes =
              mapper.GetRgbMetaStride(mmm_color_format, desc.width);
          out_layout->planes[meta_plane_index].scanlines =
              mapper.GetRgbMetaScanlines(mmm_color_format, height);
          break;
        case PLANE_LAYOUT_COMPONENT_TYPE_CB:
        case PLANE_LAYOUT_COMPONENT_TYPE_CR:
          out_layout->planes[data_plane_index].horizontal_stride_in_bytes =
              mapper.GetUVStride(mmm_color_format, desc.width);
          out_layout->planes[data_plane_index].scanlines =
              mapper.GetUVScanlines(mmm_color_format, height);
          out_layout->planes[meta_plane_index].horizontal_stride_in_bytes =
              mapper.GetUVMetaStride(mmm_color_format, desc.width);
          out_layout->planes[meta_plane_index].scanlines =
              mapper.GetUVMetaScanlines(mmm_color_format, height);
          break;
        default:
          break;
      }
      OVERFLOW_ERR_RETURN(out_layout->planes[meta_plane_index].horizontal_stride_in_bytes,
                          out_layout->planes[meta_plane_index].scanlines, OverflowType::MUL);
      out_layout->planes[meta_plane_index].size_in_bytes =
          ALIGN((out_layout->planes[meta_plane_index].horizontal_stride_in_bytes *
                 out_layout->planes[meta_plane_index].scanlines),
                alignment);
      OVERFLOW_ERR_RETURN(out_layout->planes[data_plane_index].horizontal_stride_in_bytes,
                          out_layout->planes[data_plane_index].scanlines, OverflowType::MUL);
      out_layout->planes[data_plane_index].size_in_bytes =
          ALIGN((out_layout->planes[data_plane_index].horizontal_stride_in_bytes *
                 out_layout->planes[data_plane_index].scanlines),
                alignment);
      DLOGD_IF(enable_logs, "Meta plane size %d, data plane size %d",
               out_layout->planes[meta_plane_index].size_in_bytes,
               out_layout->planes[data_plane_index].size_in_bytes);
      out_layout->planes[meta_plane_index].offset_in_bytes = meta_offset;

      OVERFLOW_ERR_RETURN(meta_offset, out_layout->planes[meta_plane_index].size_in_bytes,
                          OverflowType::ADD);
      OVERFLOW_ERR_RETURN(out_layout->planes[data_plane_index].size_in_bytes,
                          out_layout->planes[meta_plane_index].size_in_bytes, OverflowType::ADD);
      OVERFLOW_ERR_RETURN((out_layout->planes[data_plane_index].size_in_bytes +
                           out_layout->planes[meta_plane_index].size_in_bytes),
                          meta_offset, OverflowType::ADD);
      OVERFLOW_ERR_RETURN((out_layout->planes[data_plane_index].size_in_bytes +
                           out_layout->planes[meta_plane_index].size_in_bytes),
                          out_layout->size_in_bytes, OverflowType::ADD);
      out_layout->planes[data_plane_index].offset_in_bytes =
          meta_offset + out_layout->planes[meta_plane_index].size_in_bytes;
      meta_offset += (out_layout->planes[meta_plane_index].size_in_bytes +
                      out_layout->planes[data_plane_index].size_in_bytes);
      out_layout->size_in_bytes +=
          out_layout->planes[data_plane_index].size_in_bytes +
          out_layout->planes[meta_plane_index].size_in_bytes;
    }
    if ((pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX) ||
        (pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_2_BATCH) ||
        (pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_4_BATCH) ||
        (pixel_format_modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_8_BATCH)) {
      out_layout->size_in_bytes = out_ad->size;
    }

    out_layout->aligned_width_in_bytes = out_layout->planes[0].horizontal_stride_in_bytes;
    DLOGD_IF(enable_logs, "out_layout->aligned_width_in_bytes %d, out_layout->size_in_bytes %d",
             out_layout->aligned_width_in_bytes, out_layout->size_in_bytes);
    out_layout->aligned_height = out_layout->planes[0].scanlines;
  } else {
    // TODO: meta plane handling (if needed)
    DLOGD_IF(enable_logs, "using graphics to get UBWC allocation");
    vendor_qti_hardware_display_common_PixelFormatModifier pixel_format_modifier =
        static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
            GetPixelFormatModifier(desc));
    if (graphics_provider_->IsUBWCSupportedByGPU(desc.format, pixel_format_modifier)) {
      int size = 0;
      if (graphics_provider_ != nullptr) {
        vendor_qti_hardware_display_common_GraphicsMetadata graphics_metadata;

        int ret = graphics_provider_->GetInitialMetadata(desc, &graphics_metadata, true);
        if (!ret) {
          size = graphics_provider_->AdrenoGetAlignedGpuBufferSize(graphics_metadata.data);
          if (size > 0)
            out_ad->size = size;
        }
      }

      // Plane layout
      BufferConstraints data;
      int status = 0;
      status = graphics_provider_->BuildConstraints(desc, &data);
      if (status != 0) {
        DLOGE("Error while getting constraints from graphics libs");
        return Error::NO_RESOURCES;
      }
      out_layout->plane_count = format_data.planes.size();
      for (int plane_index = 0; plane_index < format_data.planes.size(); plane_index++) {
        vendor_qti_hardware_display_common_PlaneLayout *plane_data =
            &out_layout->planes[plane_index];
        PlaneLayoutData plane = format_data.planes[plane_index];
        plane_data->sample_increment_bits = plane.sample_increment_bits;
        plane_data->horizontal_subsampling = plane.horizontal_subsampling;
        plane_data->vertical_subsampling = plane.vertical_subsampling;
        PlaneConstraints plane_constraint = data.planes.at(plane_index);
        plane_data->horizontal_stride_in_bytes = plane_constraint.stride.horizontal_stride;
        plane_data->scanlines = plane_constraint.scanline.scanline;

        // TODO: need to get size directly from graphics here, or do stride * scanlines
        plane_data->size_in_bytes = size;
        plane_data->component_count = format_data.planes[plane_index].components.size();
        DLOGD_IF(enable_logs, "size %d", plane_data->size_in_bytes);
        for (int j = 0; j < plane.components.size(); j++) {
          auto component = plane.components[j];
          plane_data->components[j].type = component.type;
          plane_data->components[j].offset_in_bits = component.offset_in_bits;
          plane_data->components[j].size_in_bits = component.size_in_bits;
        }
      }
      out_layout->aligned_width_in_bytes = out_layout->planes[0].horizontal_stride_in_bytes;
      out_layout->aligned_height = out_layout->planes[0].scanlines;
      out_layout->size_in_bytes = size;
      DLOGD_IF(enable_logs, "aligned_width_in_bytes %d aligned_height %d, size %d",
               out_layout->aligned_width_in_bytes, out_layout->aligned_height, size);
    } else {
      DLOGE("%s Format 0x%x is not supported by GPU for UBWC policy", __FUNCTION__,
            static_cast<int>(desc.format));
      return Error::UNSUPPORTED;
    }
  }
  return Error::NONE;
#endif

  // Off-target testing
  int status = OffTargetAlloc(desc, out_ad, out_layout);
  if (status) {
    DLOGE("Failed to allocate using off-target alignments");
    return Error::BAD_VALUE;
  }
  return Error::NONE;
}
}  // namespace snapalloc
