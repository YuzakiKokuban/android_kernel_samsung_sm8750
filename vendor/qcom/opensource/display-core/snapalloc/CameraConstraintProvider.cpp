// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "CameraConstraintProvider.h"

#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <string>

#include "SnapConstraintDefs.h"
#include "SnapConstraintParser.h"

namespace snapalloc {
CameraConstraintProvider *CameraConstraintProvider::instance_{nullptr};
std::mutex CameraConstraintProvider::camera_provider_mutex_;

CameraConstraintProvider *CameraConstraintProvider::GetInstance(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  std::lock_guard<std::mutex> lock(camera_provider_mutex_);

  if (instance_ == nullptr) {
    instance_ = new CameraConstraintProvider();
    instance_->Init(format_data_map);
  }
  return instance_;
}

void CameraConstraintProvider::Init(
    std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map) {
  lib_ = ::dlopen("libcamxexternalformatutils.so", RTLD_NOW);
  SnapConstraintParser *parser = SnapConstraintParser::GetInstance();
  if (lib_) {
    DLOGD_IF(enable_logs, "Camera lib is available");

    *reinterpret_cast<void **>(&LINK_camera_get_stride_in_bytes) =
        ::dlsym(lib_, "CamxFormatUtil_GetStrideInBytes");
    *reinterpret_cast<void **>(&LINK_camera_get_stride_in_pixels) =
        ::dlsym(lib_, "CamxFormatUtil_GetStrideInPixels");
    *reinterpret_cast<void **>(&LINK_camera_get_scanline) =
        ::dlsym(lib_, "CamxFormatUtil_GetScanline");
    *reinterpret_cast<void **>(&LINK_camera_get_plane_size) =
        ::dlsym(lib_, "CamxFormatUtil_GetPlaneSize");
    *reinterpret_cast<void **>(&LINK_camera_get_buffer_size) =
        ::dlsym(lib_, "CamxFormatUtil_GetBufferSize");
    *reinterpret_cast<void **>(&LINK_camera_get_ubwc_info) =
        ::dlsym(lib_, "CamxFormatUtil_GetUBWCInfo");
    *reinterpret_cast<void **>(&LINK_camera_get_plane_alignment) =
        ::dlsym(lib_, "CamxFormatUtil_GetPlaneAlignment");
    *reinterpret_cast<void **>(&LINK_camera_get_plane_offset) =
        ::dlsym(lib_, "CamxFormatUtil_GetPlaneOffset");
    *reinterpret_cast<void **>(&LINK_camera_is_per_plane_fd_needed) =
        ::dlsym(lib_, "CamxFormatUtil_IsPerPlaneFdNeeded");
    *reinterpret_cast<void **>(&LINK_camera_get_bpp) = ::dlsym(lib_, "CamxFormatUtil_GetBpp");
    *reinterpret_cast<void **>(&LINK_camera_get_per_plane_bpp) =
        ::dlsym(lib_, "CamxFormatUtil_GetPerPlaneBpp");
    *reinterpret_cast<void **>(&LINK_camera_get_subsampling_factor) =
        ::dlsym(lib_, "CamxFormatUtil_GetSubsamplingFactor");
    *reinterpret_cast<void **>(&LINK_camera_get_plane_count) =
        ::dlsym(lib_, "CamxFormatUtil_GetPlaneCount");
    *reinterpret_cast<void **>(&LINK_camera_get_plane_types) =
        ::dlsym(lib_, "CamxFormatUtil_GetPlaneTypes");
    *reinterpret_cast<void **>(&LINK_camera_get_pixel_increment) =
        ::dlsym(lib_, "CamxFormatUtil_GetPixelIncrement");
    *reinterpret_cast<void **>(&LINK_camera_get_plane_start_address_alignment) =
        ::dlsym(lib_, "CamxFormatUtil_GetPlaneStartAddressAlignment");

    if (!format_data_map.empty()) {
      format_data_map_ = format_data_map;
    } else {
      parser->ParseFormats(&format_data_map_);
    }
  } else {
    DLOGW("Camera lib is not available - read json file");
    parser->ParseAlignments("/vendor/etc/display/camera_alignments.json", &constraint_set_map_);
  }
}

CamxPixelFormat CameraConstraintProvider::GetCameraPixelFormat(int snap_format, int modifier) {
  CamxPixelFormat format = (CamxPixelFormat)0;
  SnapFormatDescriptor snap_desc = {
      .format = static_cast<vendor_qti_hardware_display_common_PixelFormat>(snap_format),
      .modifier = static_cast<vendor_qti_hardware_display_common_PixelFormatModifier>(modifier)};

  if (snap_to_camera_pixel_format_.find(snap_desc) != snap_to_camera_pixel_format_.end()) {
    format = static_cast<CamxPixelFormat>(snap_to_camera_pixel_format_.at(snap_desc));
  } else {
    DLOGW("%s: No map for format: 0x%x", __FUNCTION__, snap_format);
  }
  return format;
}

int CameraConstraintProvider::GetBufferSize(int format, int width, int height, int modifier,
                                            unsigned int *size) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_buffer_size) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_buffer_size(cam_format, width, height, size);
      if (result != 0) {
        DLOGW("%s: Failed to get the buffer size. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetBufferSize. Error code : %d", __FUNCTION__, result);
  }
  return result;
}

int CameraConstraintProvider::GetStrideInBytes(int format, int plane_type, int width, int modifier,
                                               int *stride_bytes) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_stride_in_bytes) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_stride_in_bytes(cam_format, GetCamxPlaneType(plane_type), width,
                                               stride_bytes);
      if (result != 0) {
        DLOGW("%s: Failed to get the stride in bytes. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetStrideInBytes. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetStrideInPixels(int format, int plane_type, int width, int modifier,
                                                float *stride_pixel) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_stride_in_pixels) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_stride_in_pixels(cam_format, GetCamxPlaneType(plane_type), width,
                                                stride_pixel);
      if (result != 0) {
        DLOGW("%s: Failed to get the stride in pixels. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetStrideInPixels. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetPixelIncrement(int format, int plane_type, int modifier,
                                                int *pixel_increment) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_pixel_increment) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_pixel_increment(cam_format, GetCamxPlaneType(plane_type),
                                               pixel_increment);
      if (result != 0) {
        DLOGW("%s: Failed to get pixel increment. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPixelIncrement. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetPlaneOffset(int format, int plane_type, int width, int height,
                                             int modifier, int *offset) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_plane_offset) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_plane_offset(cam_format, GetCamxPlaneType(plane_type), offset, width,
                                            height);
      if (result != 0) {
        DLOGW("%s: Failed to get the plane offset. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPlaneOffset. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetSubsamplingFactor(int format, int plane_type, bool isHorizontal,
                                                   int modifier, int *subsampling_factor) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_subsampling_factor) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_subsampling_factor(cam_format, GetCamxPlaneType(plane_type),
                                                  isHorizontal, subsampling_factor);
      if (result != 0) {
        DLOGW("%s: Failed to get the sub-sampling factor. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetSubsamplingFactor. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetPlaneTypes(int format, int modifier,
                                            PlaneComponent *plane_component_array,
                                            int *plane_count) {
  CamxPlaneType plane_types_array[8] = {};
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_plane_types) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_plane_types(cam_format, plane_types_array, plane_count);
      if (result == 0) {
        for (int plane = 0; plane < *plane_count; plane++) {
          plane_component_array[plane] = GetPlaneComponent(plane_types_array[plane]);
        }
      } else {
        DLOGW("%s: Failed to get the plane types. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPlaneTypes. Error code : %d", __FUNCTION__, result);
  }

  return result;
}

int CameraConstraintProvider::GetScanline(int format, int plane_type, int height, int modifier,
                                          int *scanlines) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_scanline) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result =
          LINK_camera_get_scanline(cam_format, GetCamxPlaneType(plane_type), height, scanlines);
      if (result != 0) {
        DLOGW("%s: Failed to get the scanlines. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetScanline. Error code : %d", __FUNCTION__, result);
  }

  return result;
}

int CameraConstraintProvider::GetPlaneSize(int format, int plane_type, int width, int height,
                                           int modifier, unsigned int *size) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_plane_size) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result =
          LINK_camera_get_plane_size(cam_format, GetCamxPlaneType(plane_type), width, height, size);
      if (result != 0) {
        DLOGW("%s: Failed to get the plane size. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPlaneSize. Error code : %d", __FUNCTION__, result);
  }

  return result;
}

int CameraConstraintProvider::GetUBWCInfo(int format, int modifier, bool *is_supported, bool *is_pi,
                                          int *version) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_ubwc_info) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_ubwc_info(cam_format, is_supported, is_pi, version);
      if (result != 0) {
        DLOGW("%s: Failed to get the UBWC info. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetUBWCInfo. Error code : %d", __FUNCTION__, result);
  }

  return result;
}

int CameraConstraintProvider::GetPlaneAlignment(int format, int plane_type, int modifier,
                                                unsigned int *alignment) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_plane_alignment) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_plane_alignment(cam_format, GetCamxPlaneType(plane_type), alignment);
      if (result != 0) {
        DLOGW("%s: Failed to get the plane alignment. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPlaneAlignment. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::IsPerPlaneFdNeeded(int format, int modifier,
                                                 bool *is_per_plane_fd_needed) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_is_per_plane_fd_needed) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_is_per_plane_fd_needed(cam_format, is_per_plane_fd_needed);
      if (result != 0) {
        DLOGW("%s: Failed to get per_plane_fd flag. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_IsPerPlaneFdNeeded. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetBpp(int format, int modifier, int *bpp) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_bpp) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_bpp(cam_format, bpp);
      if (result != 0) {
        DLOGW("%s: Failed to get the bpp. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetBpp. Error code : %d", __FUNCTION__, result);
  }

  return result;
}

int CameraConstraintProvider::GetPerPlaneBpp(int format, int modifier, int plane_type, int *bpp) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_per_plane_bpp) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_per_plane_bpp(cam_format, GetCamxPlaneType(plane_type), bpp);
      if (result != 0) {
        DLOGW("%s: Failed to get the per plane bpp. Error code: %d", __FUNCTION__, result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPerPlaneBpp. Error code : %d", __FUNCTION__,
          result);
  }

  return result;
}

int CameraConstraintProvider::GetPlaneStartAddressAlignment(int format, int modifier,
                                                            int plane_type, int *alignment) {
  CamxFormatResult result = static_cast<CamxFormatResult>(-1);
  if (LINK_camera_get_plane_start_address_alignment) {
    CamxPixelFormat cam_format = GetCameraPixelFormat(format, modifier);
    if (cam_format != ((CamxPixelFormat)0)) {
      result = LINK_camera_get_plane_start_address_alignment(
          cam_format, GetCamxPlaneType(plane_type), alignment);
      if (result != 0) {
        DLOGW("%s: Failed to get the plane star address alignment. Error code: %d", __FUNCTION__,
              result);
      }
    }
  } else {
    DLOGW("%s: Failed to link CamxFormatUtil_GetPlaneStartAddressAlignment. Error code : %d",
          __FUNCTION__, result);
  }

  return result;
}

PlaneComponent CameraConstraintProvider::GetPlaneComponent(CamxPlaneType plane_type) {
  PlaneComponent plane_component = (PlaneComponent)0;
  switch (plane_type) {
    case CAMERA_PLANE_TYPE_RAW:
      plane_component = (PlaneComponent)PLANE_COMPONENT_RAW;
      break;
    case CAMERA_PLANE_TYPE_Y:
      plane_component = (PlaneComponent)PLANE_COMPONENT_Y;
      break;
    case CAMERA_PLANE_TYPE_UV:
      plane_component = (PlaneComponent)(PLANE_COMPONENT_Cb | PLANE_COMPONENT_Cr);
      break;
    case CAMERA_PLANE_TYPE_U:
      plane_component = (PlaneComponent)PLANE_COMPONENT_Cb;
      break;
    case CAMERA_PLANE_TYPE_V:
      plane_component = (PlaneComponent)PLANE_COMPONENT_Cr;
      break;
    case CAMERA_PLANE_TYPE_META_Y:
      plane_component = (PlaneComponent)(PLANE_COMPONENT_META | PLANE_COMPONENT_Y);
      break;
    case CAMERA_PLANE_TYPE_META_VU:
      plane_component =
          (PlaneComponent)(PLANE_COMPONENT_META | PLANE_COMPONENT_Cb | PLANE_COMPONENT_Cr);
      break;
    default:
      DLOGW("%s: No PlaneComponent mapping for plane_type: %d", __FUNCTION__, plane_type);
      break;
  }

  return plane_component;
}

CamxPlaneType CameraConstraintProvider::GetCamxPlaneType(int plane_type) {
  CamxPlaneType camx_plane_type = (CamxPlaneType)0;
  switch (plane_type) {
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_RAW):
      camx_plane_type = CAMERA_PLANE_TYPE_RAW;
      break;
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_Y):
      camx_plane_type = CAMERA_PLANE_TYPE_Y;
      break;
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_CB) |
        static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_CR):
      camx_plane_type = CAMERA_PLANE_TYPE_UV;
      break;
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_CB):
      camx_plane_type = CAMERA_PLANE_TYPE_U;
      break;
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_CR):
      camx_plane_type = CAMERA_PLANE_TYPE_V;
      break;
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_META) |
        static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_Y):
      camx_plane_type = CAMERA_PLANE_TYPE_META_Y;
      break;
    case static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_META) |
        static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_CR) |
        static_cast<int>(PLANE_LAYOUT_COMPONENT_TYPE_CB):
      camx_plane_type = CAMERA_PLANE_TYPE_META_VU;
      break;
    default:
      DLOGW("%s: No CamxPlane for plane_type: %d", __FUNCTION__, plane_type);
      break;
  }

  return camx_plane_type;
}

int CameraConstraintProvider::GetCapabilities(BufferDescriptor desc, CapabilitySet *out) {
  out->ubwc_caps.version = 0;

  // TODO: Evaluate if camera custom formats are used without camera flags
  if (desc.usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_OUTPUT ||
      desc.usage & vendor_qti_hardware_display_common_BufferUsage::CAMERA_INPUT) {
    DLOGD_IF(enable_logs, "CameraConstraintProvider is enabled");
    out->enabled = true;
  } else {
    DLOGD_IF(enable_logs, "CameraConstraintProvider is not enabled");
    out->enabled = false;
  }

  if (IsAstc(desc.format)) {
    out->enabled = false;
  }

  return 0;
}

int CameraConstraintProvider::BuildConstraints(BufferDescriptor desc, BufferConstraints *data) {
  int format = static_cast<uint64_t>(desc.format);
  if (format_data_map_.find(desc.format) == format_data_map_.end()) {
    DLOGW("Could not find entry for format ", static_cast<uint64_t>(format));
    return -1;
  }
  uint64_t pixel_format_modifier = GetPixelFormatModifier(desc);
  FormatData format_data = format_data_map_.at(desc.format);
  //GetBufferSize(format, desc.width, desc.height, &data->size_align_bytes);
  int ret_val = 0;
  for (auto const &plane : format_data.planes) {
    PlaneConstraints plane_layout;
    // Hardcode for json files
    plane_layout.alignment_type = ALIGNED_OUTPUT;
    int plane_type = 0;
    for (auto const &component : plane.components) {
      vendor_qti_hardware_display_common_PlaneLayoutComponentType component_type = component.type;
      plane_type |= static_cast<int>(component.type);
      plane_layout.components.push_back(component_type);
    }
    int value = 0;
    ret_val = GetStrideInBytes(format, plane_type, desc.width, pixel_format_modifier, &value);
    plane_layout.stride.horizontal_stride = value;
    if (ret_val) {
      DLOGW("Error in GetStrideInBytes");
      return -1;
    }
    value = 0;
    ret_val = GetScanline(format, plane_type, desc.height, pixel_format_modifier, &value);
    plane_layout.scanline.scanline = value;
    if (ret_val) {
      DLOGW("Error in GetScanline");
      return -1;
    }
    unsigned int alignment = 0;
    ret_val = GetPlaneAlignment(format, plane_type, pixel_format_modifier, &alignment);

    // For RAW formats, plane size is width * height
    if (format == static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RAW10) ||
        format == static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RAW12) ||
        format == static_cast<int>(vendor_qti_hardware_display_common_PixelFormat::RAW14)) {
      data->size_align_bytes = alignment;
    } else {
      plane_layout.size_align = static_cast<uint64_t>(alignment);
    }
    if (ret_val) {
      DLOGW("Error in GetPlaneAlignment");
      return -1;
    }
    data->planes.push_back(plane_layout);
  }
  return 0;
}

int CameraConstraintProvider::GetConstraints(BufferDescriptor desc, BufferConstraints *out) {
#ifdef __ANDROID__
  if (lib_ != nullptr) {
    DLOGD_IF(enable_logs, "Using camera libs for alignment calculations");
    BufferConstraints data;
    int status = 0;
    status = BuildConstraints(desc, &data);
    if (status) {
      DLOGW("Error while getting constraints from camera libs");
      return -1;
    }
    *out = data;
    return 0;
  }
#endif

  if (constraint_set_map_.empty()) {
    DLOGD_IF(enable_logs, "Camera constraint set map is empty");
    return -1;
  }
  if (constraint_set_map_.find(desc.format) != constraint_set_map_.end()) {
    *out = constraint_set_map_.at(desc.format);
  } else {
    DLOGD_IF(enable_logs, "Camera could not find entry for format %lu",
             static_cast<uint64_t>(desc.format));
  }
  return 0;
}

}  // namespace snapalloc