// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "GraphicsConstraintProvider.h"

#include <dlfcn.h>
#include <cstdint>
#include <fstream>
#include <iostream>

#include "SnapConstraintParser.h"
#include "SnapUtils.h"
#include "UBWCPolicy.h"

namespace snapalloc {
GraphicsConstraintProvider *GraphicsConstraintProvider::instance_{nullptr};
std::mutex GraphicsConstraintProvider::graphics_provider_mutex_;

GraphicsConstraintProvider *GraphicsConstraintProvider::GetInstance(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  std::lock_guard<std::mutex> lock(graphics_provider_mutex_);

  if (instance_ == nullptr) {
    instance_ = new GraphicsConstraintProvider();
    instance_->Init(format_data_map);
  }
  return instance_;
}

void GraphicsConstraintProvider::Init(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  lib_ = ::dlopen("libadreno_utils.so", RTLD_NOW);
  SnapConstraintParser *parser = SnapConstraintParser::GetInstance();
  if (lib_) {
    DLOGI("Graphics lib is available");
    *reinterpret_cast<void **>(&LINK_adreno_compute_aligned_width_and_height) =
        ::dlsym(lib_, "compute_aligned_width_and_height");
    *reinterpret_cast<void **>(&LINK_adreno_compute_fmt_aligned_width_and_height) =
        ::dlsym(lib_, "compute_fmt_aligned_width_and_height");
    *reinterpret_cast<void **>(&LINK_adreno_compute_padding) =
        ::dlsym(lib_, "compute_surface_padding");
    *reinterpret_cast<void **>(&LINK_adreno_compute_compressedfmt_aligned_width_and_height) =
        ::dlsym(lib_, "compute_compressedfmt_aligned_width_and_height");
    *reinterpret_cast<void **>(&LINK_adreno_isUBWCSupportedByGpu) =
        ::dlsym(lib_, "isUBWCSupportedByGpu");
    *reinterpret_cast<void **>(&LINK_adreno_get_gpu_pixel_alignment) =
        ::dlsym(lib_, "get_gpu_pixel_alignment");
    *reinterpret_cast<void **>(&LINK_adreno_get_metadata_blob_size) =
        ::dlsym(lib_, "adreno_get_metadata_blob_size");
    *reinterpret_cast<void **>(&LINK_adreno_init_memory_layout) =
        ::dlsym(lib_, "adreno_init_memory_layout");
    *reinterpret_cast<void **>(&LINK_adreno_get_aligned_gpu_buffer_size) =
        ::dlsym(lib_, "adreno_get_aligned_gpu_buffer_size");
  } else {
    DLOGW("Graphics lib is not available - read json file");
    // change to shared pointer
    parser->ParseAlignments("/vendor/etc/display/graphics_alignments.json", &constraint_set_map_);
  }
  if (!format_data_map.empty()) {
    format_data_map_ = format_data_map;
  } else {
    parser->ParseFormats(&format_data_map_);
  }

  gfx_ubwc_disable_ = Debug::GetInstance()->IsUBWCDisabled();
}

static bool AdrenoAlignmentRequired(vendor_qti_hardware_display_common_BufferUsage usage,
                                    vendor_qti_hardware_display_common_PixelFormat format) {
  if ((usage & vendor_qti_hardware_display_common_BufferUsage::GPU_TEXTURE) ||
      (usage & vendor_qti_hardware_display_common_BufferUsage::GPU_RENDER_TARGET)) {
    if (format == YV12) {
      if ((usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_VIDEO_HW) &&
          ((usage & vendor_qti_hardware_display_common_BufferUsage::VIDEO_ENCODER) ||
           (usage & vendor_qti_hardware_display_common_BufferUsage::VIDEO_DECODER) ||
           !CpuCanAccess(usage))) {
        return true;
      }
    }

    if (format == YCBCR_422_I) {
      return true;
    }
  }
  return false;
}

int GraphicsConstraintProvider::GetInitialMetadata(
    BufferDescriptor desc, vendor_qti_hardware_display_common_GraphicsMetadata *graphics_metadata,
    bool is_ubwc_enabled) {
  uint64_t pixel_format_modifier = GetPixelFormatModifier(desc);
  auto adreno_format = GetGpuPixelFormat(
      desc.format,
      static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(pixel_format_modifier));

  uint64_t usage = desc.usage;
  int plane_count = 1;

  // TODO: Move this check to UBWCPolicy::IsUBWCAlloc
  bool ubwc_enabled_gfx = is_ubwc_enabled;
  if (is_ubwc_enabled) {
    if ((usage & vendor_qti_hardware_display_common_BufferUsage::GPU_TEXTURE) ||
        (usage & vendor_qti_hardware_display_common_BufferUsage::GPU_RENDER_TARGET)) {
      ubwc_enabled_gfx = IsUBWCSupportedByGPU(
          desc.format, static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
                           pixel_format_modifier));
    }
  }

  if (!ubwc_enabled_gfx) {
    usage = static_cast<uint64_t>(
        (static_cast<uint64_t>(usage) &
         ~static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC)));
  } else {
    usage |= static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC);
  }

  int tile_mode = ubwc_enabled_gfx;
  if (IsTileRendered(desc.format)) {
    tile_mode = true;
  }

  if (AdrenoAlignmentRequired(desc.usage, desc.format)) {
    FormatData format_data = format_data_map_.at(desc.format);
    plane_count = format_data.planes.size();
  }

  // Call adreno api for populating metadata blob
  // Layer count is for 2D/Cubemap arrays and depth is used for 3D slice
  // Using depth to pass layer_count here
  int ret = AdrenoInitMemoryLayout(
      graphics_metadata->data, desc.width, desc.height, desc.layerCount, /* depth */
      adreno_format, 1, tile_mode, static_cast<uint64_t>(usage), plane_count);

  if (ret != 0) {
    DLOGW("%s Graphics metadata init failed - ret val %d", __FUNCTION__, ret);
    return Error::BAD_DESCRIPTOR;
  }

  return Error::NONE;
}

uint32_t GraphicsConstraintProvider::AdrenoGetMetadataBlobSize() {
  if (LINK_adreno_get_metadata_blob_size) {
    return LINK_adreno_get_metadata_blob_size();
  }
  return 0;
}

int GraphicsConstraintProvider::AdrenoInitMemoryLayout(void *metadata_blob, int width, int height,
                                                       int depth, ADRENOPIXELFORMAT format,
                                                       int num_samples, int isUBWC, uint64_t usage,
                                                       uint32_t num_planes) {
  if (LINK_adreno_init_memory_layout) {
    surface_tile_mode_t tile_mode = static_cast<surface_tile_mode_t>(isUBWC);
    return LINK_adreno_init_memory_layout(metadata_blob, width, height, depth, format, num_samples,
                                          tile_mode, usage, num_planes);
  }
  return -1;
}

ADRENOPIXELFORMAT GraphicsConstraintProvider::GetGpuPixelFormat(
    vendor_qti_hardware_display_common_PixelFormat snap_format,
    vendor_qti_hardware_display_common_PixelFormatModifier modifier) {
  ADRENOPIXELFORMAT format = ADRENO_PIXELFORMAT_UNKNOWN;
  SnapFormatDescriptor snap_desc = {.format = snap_format, .modifier = modifier};
  if (snap_to_adreno_pixel_format_.find(snap_desc) != snap_to_adreno_pixel_format_.end()) {
    format = snap_to_adreno_pixel_format_.at(snap_desc);
  } else {
    DLOGW("%s: No map for format: 0x%x", __FUNCTION__, snap_format);
  }
  return format;
}

int GraphicsConstraintProvider::GetCapabilities(BufferDescriptor desc, CapabilitySet *out) {
  out->ubwc_caps.version = 0;

  // Add an early check for YUV formats where adreno alignments are required
  // to prevent disabling graphics constraint provider for remaining YUV formats
  if (AdrenoAlignmentRequired(desc.usage, desc.format)) {
    out->enabled = true;
    return 0;
  }

  if (IsYuv(desc.format)) {
    out->enabled = false;
    DLOGD_IF(enable_logs, "GraphicsConstraintProvider enabled: %d", out->enabled);
    return 0;
  }

  if ((desc.usage & vendor_qti_hardware_display_common_BufferUsage::PROTECTED &&
       desc.usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT) ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::QTI_PRIVATE_SECURE_DISPLAY) {
    out->enabled = false;
    DLOGD_IF(enable_logs, "GraphicsConstraintProvider enabled: %d", out->enabled);
    return 0;
  }

  if (desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_TEXTURE ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_RENDER_TARGET ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_CUBE_MAP ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_MIPMAP_COMPLETE ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::GPU_DATA_BUFFER ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::RENDERSCRIPT) {
    out->enabled = true;
  } else {
    out->enabled = false;
  }

  if (out->enabled == true) {
    uint64_t pixel_format_modifier = GetPixelFormatModifier(desc);
    if (GetGpuPixelFormat(
            desc.format,
            static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
                pixel_format_modifier)) == ADRENO_PIXELFORMAT_UNKNOWN) {
      out->enabled = false;
    }
  }
  DLOGD_IF(enable_logs, "GraphicsConstraintProvider enabled: %d", out->enabled);
  return 0;
}

int GraphicsConstraintProvider::BuildConstraints(BufferDescriptor desc, BufferConstraints *data) {
  vendor_qti_hardware_display_common_PixelFormat snap_format = desc.format;
  int format = static_cast<uint64_t>(snap_format);
  uint64_t pixel_format_modifier = GetPixelFormatModifier(desc);
  if (format_data_map_.find(snap_format) == format_data_map_.end()) {
    DLOGW("%s: could not find entry for format %lu", __FUNCTION__, static_cast<uint64_t>(format));
    return -1;
  }

  FormatData format_data = format_data_map_.at(snap_format);
  // Hardcode based on json
  data->size_align_bytes = 1;
  int ret_val = Error::UNSUPPORTED;
  int tile_enabled = 0;
  for (auto const &plane : format_data.planes) {
    PlaneConstraints plane_layout;
    plane_layout.alignment_type = ALIGNED_OUTPUT;

    plane_layout.size_align = 1;  // GetGpuPixelAlignment();

    tile_enabled = IsTileRendered(snap_format);
    unsigned int aligned_w, aligned_h = 0;
    if (format_data.bits_per_pixel % 8 != 0)
      DLOGW("Bpp is float: %f", static_cast<float>(format_data.bits_per_pixel) / 8.0f);

    if (IsRgb(snap_format) && IsAstc(snap_format)) {
      plane_layout.stride.horizontal_stride = desc.width;
      plane_layout.scanline.scanline = desc.height;
      /* TODO: gralloc does not use the returned values - uncomment when base gralloc issue resolved
      // This returns aligned width and height in blocks
      AlignCompressedRGB(desc.width, desc.height, format, &aligned_w, &aligned_h);
      plane_layout.stride.horizontal_stride =
          static_cast<uint64_t>(aligned_w) * (format_data.bits_per_pixel / 8.0f);
      plane_layout.scanline.scanline = static_cast<uint64_t>(aligned_h);*/
    } else if (IsRgb(snap_format) && !IsAstc(snap_format)) {
      aligned_h = 0;
      aligned_w = 0;
      // This returns aligned width in pixels
      AlignUnCompressedRGB(desc.width, desc.height, format, tile_enabled, pixel_format_modifier,
                           &aligned_w, &aligned_h);
      OVERFLOW_ERR_RETURN(static_cast<uint64_t>(aligned_w), (format_data.bits_per_pixel / 8.0f),
                          OverflowType::MUL);
      plane_layout.stride.horizontal_stride =
          static_cast<uint64_t>(aligned_w) * (format_data.bits_per_pixel / 8.0f);
      plane_layout.scanline.scanline = static_cast<uint64_t>(aligned_h);
    } else if (IsGpuDepthStencil(snap_format)) {
      DLOGD_IF(enable_logs, "Querying graphics for GpuDepthStencil case");
      // Depth formats are not supported by graphics when CPU bits are set
      if (CpuCanAccess(desc.usage)) {
        return Error::UNSUPPORTED;
      }

      aligned_h = 0;
      aligned_w = 0;
      AlignGpuDepthStencilFormat(desc.width, desc.height, format, tile_enabled,
                                 pixel_format_modifier, &aligned_w, &aligned_h);
      plane_layout.stride.horizontal_stride =
          static_cast<uint64_t>(aligned_w) * (format_data.bits_per_pixel / 8.0f);
      plane_layout.scanline.scanline = static_cast<uint64_t>(aligned_h);
    } else if (AdrenoAlignmentRequired(desc.usage, desc.format)) {
      aligned_h = 0;
      aligned_w = 0;
      surface_tile_mode_t tile_mode = static_cast<surface_tile_mode_t>(tile_enabled);
      surface_rastermode_t raster_mode =
          SURFACE_RASTER_MODE_UNKNOWN;  // Adreno unknown raster mode.
      int padding_threshold = 512;      // Threshold for padding surfaces.
      ADRENOPIXELFORMAT gpu_format = GetGpuPixelFormat(
          desc.format, static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(
                           pixel_format_modifier));
      if (LINK_adreno_compute_fmt_aligned_width_and_height &&
          gpu_format != ADRENO_PIXELFORMAT_UNKNOWN) {
        int input_width = desc.width;
        int input_height = desc.height;
        if ((desc.format == vendor_qti_hardware_display_common_PixelFormat::YV12) &&
            ((plane.components[0].type == PLANE_LAYOUT_COMPONENT_TYPE_CB) ||
             (plane.components[0].type == PLANE_LAYOUT_COMPONENT_TYPE_CR))) {
          // Input width and height need to be adjusted for subsampling
          // for the chroma planes for YV12,
          // as the API does not differentiate based on the plane
          input_width /= 2;
          input_height /= 2;
        }

        LINK_adreno_compute_fmt_aligned_width_and_height(
            input_width, input_height, format_data.planes.size(), gpu_format, 1 /*num_samples*/,
            tile_mode, raster_mode, padding_threshold, (int *)&aligned_w, (int *)&aligned_h);

        plane_layout.stride.horizontal_stride =
            static_cast<uint64_t>(aligned_w) * floor(format_data.bits_per_pixel / 8.0f);
        plane_layout.scanline.scanline = static_cast<uint64_t>(aligned_h);
      } else {
        DLOGW(
            "Not able to call LINK_adreno_compute_fmt_aligned_width_and_height - snap format %d "
            "graphics format %d",
            desc.format, gpu_format);
        return Error::UNSUPPORTED;
      }
    }
    for (auto const &component : plane.components) {
      vendor_qti_hardware_display_common_PlaneLayoutComponentType component_type = component.type;
      plane_layout.components.push_back(component_type);
    }
    data->planes.push_back(plane_layout);
    ret_val = Error::NONE;
  }
  return ret_val;
}

int GraphicsConstraintProvider::GetConstraints(BufferDescriptor desc, BufferConstraints *out) {
#ifdef __ANDROID__
  if (lib_ != nullptr && AdrenoSizeAPIAvaliable()) {
    DLOGI("Using graphics libs for alignment calculations");
    BufferConstraints data;
    int status = 0;
    status = BuildConstraints(desc, &data);
    if (status != Error::NONE) {
      DLOGW("Error while getting constraints from graphics libs");
      return status;
    }
    *out = data;
    return 0;
  }
#endif
  if (constraint_set_map_.empty()) {
    DLOGW("Graphics constraint set map is empty");
    return -1;
  }
  if (constraint_set_map_.find(desc.format) != constraint_set_map_.end()) {
    *out = constraint_set_map_.at(desc.format);
  } else {
    DLOGW("Graphics could not find entry for format %d", static_cast<uint64_t>(desc.format));
    return -1;
  }
  return 0;
}

bool GraphicsConstraintProvider::AdrenoSizeAPIAvaliable() {
  if (gfx_ahardware_buffer_disable_) {
    return false;
  }

  return (LINK_adreno_get_metadata_blob_size && LINK_adreno_init_memory_layout &&
          LINK_adreno_get_aligned_gpu_buffer_size);
}

bool GraphicsConstraintProvider::IsUBWCSupportedByGPU(
    vendor_qti_hardware_display_common_PixelFormat format,
    vendor_qti_hardware_display_common_PixelFormatModifier modifier) {
  if (!gfx_ubwc_disable_ && LINK_adreno_isUBWCSupportedByGpu) {
    ADRENOPIXELFORMAT gpu_format = GetGpuPixelFormat(format, modifier);
    return LINK_adreno_isUBWCSupportedByGpu(gpu_format);
  }

  return false;
}

void GraphicsConstraintProvider::AlignUnCompressedRGB(int width, int height, int format,
                                                      int tile_enabled, int pixel_format_modifier,
                                                      unsigned int *aligned_w,
                                                      unsigned int *aligned_h) {
  *aligned_w = (unsigned int)ALIGN(width, 32);
  *aligned_h = (unsigned int)ALIGN(height, 32);

  int bpp = 4;
  switch (format) {
    case static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RGB_888):
    case static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::BGR_888):
      bpp = 3;
      break;
    case static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RGB_565):
    case static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::BGR_565):
    case static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RGBA_5551):
    case static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RGBA_4444):
      bpp = 2;
      break;
    default:
      break;
  }

  vendor_qti_hardware_display_common_PixelFormat snap_format =
      static_cast<vendor_qti_hardware_display_common_PixelFormat>(format);
  vendor_qti_hardware_display_common_PixelFormatModifier modifier =
      static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(pixel_format_modifier);
  surface_tile_mode_t tile_mode = static_cast<surface_tile_mode_t>(tile_enabled);
  surface_rastermode_t raster_mode = SURFACE_RASTER_MODE_UNKNOWN;  // Adreno unknown raster mode.
  int padding_threshold = 512;  // Threshold for padding surfaces.
  // the function below computes aligned width and aligned height
  // based on linear or macro tile mode selected.
  if (LINK_adreno_compute_fmt_aligned_width_and_height) {
    // We call into adreno_utils only for RGB formats. So plane_id is 0 and
    // num_samples is 1 always. We may  have to add uitility function to
    // find out these if there is a need to call this API for YUV formats.
    LINK_adreno_compute_fmt_aligned_width_and_height(
        width, height, 0 /*plane_id*/, GetGpuPixelFormat(snap_format, modifier), 1 /*num_samples*/,
        tile_mode, raster_mode, padding_threshold, reinterpret_cast<int *>(aligned_w),
        reinterpret_cast<int *>(aligned_h));
  } else if (LINK_adreno_compute_aligned_width_and_height) {
    LINK_adreno_compute_aligned_width_and_height(
        width, height, bpp, tile_mode, raster_mode, padding_threshold,
        reinterpret_cast<int *>(aligned_w), reinterpret_cast<int *>(aligned_h));
  } else if (LINK_adreno_compute_padding) {
    int surface_tile_height = 1;  // Linear surface
    *aligned_w = UINT(LINK_adreno_compute_padding(width, bpp, surface_tile_height, raster_mode,
                                                  padding_threshold));
    DLOGW("%s: Warning!! Old GFX API is used to calculate stride", __FUNCTION__);
  } else {
    DLOGW(
        "%s: Warning!! Symbols compute_surface_padding and "
        "compute_fmt_aligned_width_and_height and "
        "compute_aligned_width_and_height not found",
        __FUNCTION__);
  }
}

void GraphicsConstraintProvider::AlignCompressedRGB(int width, int height, int format,
                                                    unsigned int *aligned_w,
                                                    unsigned int *aligned_h) {
  if (LINK_adreno_compute_compressedfmt_aligned_width_and_height) {
    int bytesPerPixel = 0;
    surface_rastermode_t raster_mode = SURFACE_RASTER_MODE_UNKNOWN;  // Adreno unknown raster mode.
    int padding_threshold = 512;  // Threshold for padding surfaces.

    LINK_adreno_compute_compressedfmt_aligned_width_and_height(
        width, height,
        GetGpuPixelFormat(static_cast<vendor_qti_hardware_display_common_PixelFormat>(format),
        vendor_qti_hardware_display_common_PixelFormatModifier::PIXEL_FORMAT_MODIFIER_NONE),
        SURFACE_TILE_MODE_DISABLE, raster_mode, padding_threshold,
        reinterpret_cast<int *>(aligned_w), reinterpret_cast<int *>(aligned_h), &bytesPerPixel);
  } else {
    *aligned_w = (unsigned int)ALIGN(width, 32);
    *aligned_h = (unsigned int)ALIGN(height, 32);
    DLOGW("%s: Warning!! compute_compressedfmt_aligned_width_and_height not found", __FUNCTION__);
  }
}

void GraphicsConstraintProvider::AlignGpuDepthStencilFormat(int width, int height, int format,
                                                            int tile_enabled,
                                                            int pixel_format_modifier,
                                                            unsigned int *aligned_w,
                                                            unsigned int *aligned_h) {
  surface_tile_mode_t tile_mode = static_cast<surface_tile_mode_t>(tile_enabled);
  surface_rastermode_t raster_mode = SURFACE_RASTER_MODE_UNKNOWN;  // Adreno unknown raster mode.
  int padding_threshold = 512;  // Threshold for padding surfaces.

  vendor_qti_hardware_display_common_PixelFormat snap_format =
      static_cast<vendor_qti_hardware_display_common_PixelFormat>(format);
  vendor_qti_hardware_display_common_PixelFormatModifier modifier =
      static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(pixel_format_modifier);
  if (LINK_adreno_compute_fmt_aligned_width_and_height) {
    LINK_adreno_compute_fmt_aligned_width_and_height(
        width, height, 0 /*plane_id*/, GetGpuPixelFormat(snap_format, modifier), 1 /*num_samples*/,
        tile_mode, raster_mode, padding_threshold, reinterpret_cast<int *>(aligned_w),
        reinterpret_cast<int *>(aligned_h));
  } else {
    DLOGW("%s: Warning!! compute_fmt_aligned_width_and_height not found", __FUNCTION__);
  }
}

uint32_t GraphicsConstraintProvider::GetGpuPixelAlignment() {
  if (LINK_adreno_get_gpu_pixel_alignment) {
    return LINK_adreno_get_gpu_pixel_alignment();
  }

  return 1;
}

uint32_t GraphicsConstraintProvider::AdrenoGetAlignedGpuBufferSize(void *metadata_blob) {
  if (LINK_adreno_get_aligned_gpu_buffer_size) {
    uint64_t size = LINK_adreno_get_aligned_gpu_buffer_size(metadata_blob);
    return static_cast<uint32_t>(size);
  }
  return -1;
}

bool GraphicsConstraintProvider::IsPISupportedByGPU(int format, uint64_t usage) {
  if (LINK_adreno_isPISupportedByGpu) {
    return LINK_adreno_isPISupportedByGpu(format, usage);
  }

  // TODO(user): Remove later once Adreno API is available
  if ((usage &
       static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::GPU_RENDER_TARGET))) {
    return false;
  }
  if ((usage &
       static_cast<uint64_t>(vendor_qti_hardware_display_common_BufferUsage::GPU_TEXTURE))) {
    return true;
  }

  return false;
}

}  // namespace snapalloc
