// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __CAMERA_CONSTRAINT_PROVIDER_H__
#define __CAMERA_CONSTRAINT_PROVIDER_H__

#include <map>
#include <mutex>

#include "Debug.h"
#include "SnapConstraintProvider.h"
#include "SnapUtils.h"

// Plane types supported by the camera format
typedef enum {
  CAMERA_PLANE_TYPE_RAW,  // RAW plane for Single planar formats including UYVY and thier variants
  CAMERA_PLANE_TYPE_Y,    // Y only
  CAMERA_PLANE_TYPE_UV,   // UV, VU, Cb, Cr planes for YUV variants
  CAMERA_PLANE_TYPE_U,    // U plane only
  CAMERA_PLANE_TYPE_V,    // V plane only
  CAMERA_PLANE_TYPE_META_Y,   // Metadata plane for Y
  CAMERA_PLANE_TYPE_META_VU,  // Metadata plane for VU and UV
} CamxPlaneType;

// External camera pixel formats that are allocated by gralloc
typedef enum : unsigned int {
  CAMERA_PIXEL_FORMAT_NV21_ZSL = 0x113,           // NV21 format with alignment requirements for
                                                  // YUV reprocessing
  CAMERA_PIXEL_FORMAT_YUV_FLEX = 0x125,           // YUV format with fliexible alignment defined by
                                                  // individual APIs
  CAMERA_PIXEL_FORMAT_UBWC_FLEX = 0x126,          // YUV format with fliexible alignment defined by
                                                  // individual APIs
  CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX = 0x127,   // YUV format with fliexible alignment defined by
                                                  // individual APIs
  CAMERA_PIXEL_FORMAT_UBWC_FLEX_2_BATCH = 0x128,  // YUV format with fliexible alignment defined by
                                                  // individual APIs
  CAMERA_PIXEL_FORMAT_UBWC_FLEX_4_BATCH = 0x129,  // YUV format with fliexible alignment defined by
                                                  // individual APIs
  CAMERA_PIXEL_FORMAT_UBWC_FLEX_8_BATCH = 0x130,  // YUV format with fliexible alignment defined by
                                                  // individual APIs
  CAMERA_PIXEL_FORMAT_NV12_VENUS = 0x7FA30C04,    // NV12 video format
  CAMERA_PIXEL_FORMAT_NV12_HEIF = 0x00000116,     // HEIF video YUV420 format
  CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC = 0x7FA30C06,    // 8 bit YUV 420 semi-planar UBWC format
  CAMERA_PIXEL_FORMAT_YCbCr_420_TP10_UBWC = 0x7FA30C09,  // TP10 YUV 420 semi-planar UBWC format
  CAMERA_PIXEL_FORMAT_YCbCr_420_P010_UBWC = 0x124,       // P010 YUV 420 semi-planar UBWC format
  CAMERA_PIXEL_FORMAT_RAW_OPAQUE = 0x24,                 // Opaque RAW format
  CAMERA_PIXEL_FORMAT_RAW10 = 0x25,                      // Opaque RAW10 bit format
  CAMERA_PIXEL_FORMAT_RAW12 = 0x26,                      // Opaque RAW12 bit format
  CAMERA_PIXEL_FORMAT_RAW14 = 0x144,                     // Opaque RAW14 bit format
  CAMERA_PIXEL_FORMAT_RAW8 = 0x00000123,                 // Opaque RAW8 bit format
} CamxPixelFormat;

// Camera Result Codes
typedef enum : int {
  CamxFormatResultSuccess = 0,           // Operation was successful
  CamxFormatResultEFailed = 1,           // Operation encountered unspecified error
  CamxFormatResultEUnsupported = 2,      // Operation is not supported
  CamxFormatResultEInvalidState = 3,     // Invalid state
  CamxFormatResultEInvalidArg = 4,       // Invalid argument
  CamxFormatResultEInvalidPointer = 5,   // Invalid memory pointer
  CamxFormatResultENoSuch = 6,           // No such item exists or is valid
  CamxFormatResultEOutOfBounds = 7,      // Out of bounds
  CamxFormatResultENoMemory = 8,         // Out of memory
  CamxFormatResultENoMore = 10,          // No more items available
  CamxFormatResultENeedMore = 11,        // Operation requires more
  CamxFormatResultEPrivLevel = 13,       // Privileges are insufficient for requested operation
  CamxFormatResultENotImplemented = 26,  // Function or method is not implemented
} CamxFormatResult;

enum PlaneComponent {
  /* luma */
  PLANE_COMPONENT_Y = 1 << 0,
  /* chroma blue */
  PLANE_COMPONENT_Cb = 1 << 1,
  /* chroma red */
  PLANE_COMPONENT_Cr = 1 << 2,

  /* red */
  PLANE_COMPONENT_R = 1 << 10,
  /* green */
  PLANE_COMPONENT_G = 1 << 11,
  /* blue */
  PLANE_COMPONENT_B = 1 << 12,

  /* alpha */
  PLANE_COMPONENT_A = 1 << 20,

  /* raw data plane */
  PLANE_COMPONENT_RAW = 1 << 30,

  /* meta information plane */
  PLANE_COMPONENT_META = 1 << 31,
};

namespace snapalloc {
class CameraConstraintProvider : public SnapConstraintProvider {
 public:
  CameraConstraintProvider(CameraConstraintProvider &other) = delete;
  void operator=(const CameraConstraintProvider &) = delete;
  static CameraConstraintProvider *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  /* Attempt to dlopen the camx library.
   * If this fails, the CameraConstraintProvider will use the default alignment table.
   * If the library is available, it will queried for one-time capabilities.
   * @return CameraConstraintProvider instance
   */
  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);
  int GetProviderType() { return kCamera; }

  int GetCapabilities(BufferDescriptor desc, CapabilitySet *out);
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);

 private:
  CameraConstraintProvider(){};
  ~CameraConstraintProvider(){};
  static std::mutex camera_provider_mutex_;
  static CameraConstraintProvider *instance_;

  void *lib_ = nullptr;
  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
  std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map_;
  CamxPixelFormat GetCameraPixelFormat(int snap_format, int modifier);
  int GetUBWCInfo(int format, int modifier, bool *is_Supported, bool *is_PI, int *version);
  int GetPlaneAlignment(int format, int plane_type, int modifier, unsigned int *alignment);
  int IsPerPlaneFdNeeded(int format, int modifier, bool *is_per_plane_fd_needed);
  int GetBpp(int format, int modifier, int *bpp);
  int GetPerPlaneBpp(int format, int modifier, int plane_type, int *bpp);
  int GetPlaneStartAddressAlignment(int format, int modifier, int plane_type, int *alignment);
  int GetBufferSize(int format, int width, int height, int modifier, unsigned int *size);
  int GetStrideInBytes(int format, int plane_type, int width, int modifier, int *stride_bytes);
  int GetStrideInPixels(int format, int plane_type, int width, int modifier, float *stride_pixel);
  int GetPixelIncrement(int format, int plane_type, int modifier, int *pixel_increment);
  int GetPlaneOffset(int format, int plane_type, int width, int height, int modifier, int *offset);
  int GetSubsamplingFactor(int format, int plane_type, bool isHorizontal, int modifier,
                           int *subsampling_factor);
  int GetPlaneTypes(int format, int modifier, PlaneComponent *plane_component_array,
                    int *plane_count);
  int GetScanline(int format, int plane_type, int height, int modifier, int *scanlines);
  int GetPlaneSize(int format, int plane_type, int width, int height, int modifier,
                   unsigned int *size);

  int BuildConstraints(BufferDescriptor desc, BufferConstraints *data);

  PlaneComponent GetPlaneComponent(CamxPlaneType plane_type);

  CamxPlaneType GetCamxPlaneType(int plane_type);

  CamxFormatResult (*LINK_camera_get_stride_in_bytes)(CamxPixelFormat format,
                                                      CamxPlaneType plane_type, int width,
                                                      int *stride) = nullptr;

  CamxFormatResult (*LINK_camera_get_stride_in_pixels)(CamxPixelFormat format,
                                                       CamxPlaneType plane_type, int width,
                                                       float *stride) = nullptr;

  CamxFormatResult (*LINK_camera_get_scanline)(CamxPixelFormat format, CamxPlaneType plane_type,
                                               int height, int *scanLine) = nullptr;

  CamxFormatResult (*LINK_camera_get_plane_size)(CamxPixelFormat format, CamxPlaneType plane_type,
                                                 int width, int height,
                                                 unsigned int *aligned_size) = nullptr;

  CamxFormatResult (*LINK_camera_get_buffer_size)(CamxPixelFormat format, int width, int height,
                                                  unsigned int *buffer_size) = nullptr;

  CamxFormatResult (*LINK_camera_get_ubwc_info)(CamxPixelFormat format, bool *isSupported,
                                                bool *isPI, int *version) = nullptr;

  CamxFormatResult (*LINK_camera_get_plane_alignment)(CamxPixelFormat format,
                                                      CamxPlaneType plane_type,
                                                      unsigned int *alignment) = nullptr;

  CamxFormatResult (*LINK_camera_get_plane_offset)(CamxPixelFormat format, CamxPlaneType plane_type,
                                                   int *offset, int width, int height) = nullptr;

  CamxFormatResult (*LINK_camera_get_plane_types)(CamxPixelFormat format,
                                                  CamxPlaneType *plane_types_array,
                                                  int *plane_count) = nullptr;

  CamxFormatResult (*LINK_camera_is_per_plane_fd_needed)(CamxPixelFormat format,
                                                         bool *is_perplane_fd_needed) = nullptr;

  CamxFormatResult (*LINK_camera_get_bpp)(CamxPixelFormat format, int *bpp) = nullptr;

  CamxFormatResult (*LINK_camera_get_per_plane_bpp)(CamxPixelFormat format,
                                                    CamxPlaneType plane_type, int *bpp) = nullptr;

  CamxFormatResult (*LINK_camera_get_subsampling_factor)(CamxPixelFormat format,
                                                         CamxPlaneType plane_type,
                                                         bool is_horizontal,
                                                         int *subsampling_factor) = nullptr;

  CamxFormatResult (*LINK_camera_get_plane_count)(CamxPixelFormat format,
                                                  int *plane_count) = nullptr;

  CamxFormatResult (*LINK_camera_get_pixel_increment)(CamxPixelFormat format,
                                                      CamxPlaneType plane_type,
                                                      int *pixel_increment) = nullptr;

  CamxFormatResult (*LINK_camera_get_plane_start_address_alignment)(CamxPixelFormat format,
                                                                    CamxPlaneType planeType,
                                                                    int *pAlignment) = nullptr;

  std::unordered_map<SnapFormatDescriptor, CamxPixelFormat, SnapFormatDescriptorHash>
      snap_to_camera_pixel_format_ = {
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV21_ZSL,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_NV21_ZSL},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX},
           CAMERA_PIXEL_FORMAT_UBWC_FLEX},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_LINEAR_FLEX},
           CAMERA_PIXEL_FORMAT_YUV_FLEX},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX_2_BATCH},
           CAMERA_PIXEL_FORMAT_UBWC_FLEX_2_BATCH},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX_4_BATCH},
           CAMERA_PIXEL_FORMAT_UBWC_FLEX_4_BATCH},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX_8_BATCH},
           CAMERA_PIXEL_FORMAT_UBWC_FLEX_8_BATCH},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::MULTIPLANAR_FLEX,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RAW_OPAQUE,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_RAW_OPAQUE},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RAW10,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_RAW10},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RAW12,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_RAW12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RAW14,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_RAW14},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RAW8,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           CAMERA_PIXEL_FORMAT_RAW8}};
};
}  // namespace snapalloc

#endif  // __CAMERA_CONSTRAINT_PROVIDER_H__