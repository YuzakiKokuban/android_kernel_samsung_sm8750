// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_UTILS_H__
#define __SNAP_UTILS_H__

#include "Debug.h"
#include "SnapTypes.h"
#include <display/drm/sde_drm.h>

#ifdef __ANDROID__
#include <display/media/mmm_color_fmt.h>
#endif
#include <string>
#include <unordered_map>

#define SIZE_2MB 0x200000
#define SIZE_1MB 0x100000
#define SIZE_4K 4096
#define SIZE_8K 8192

#ifdef SLAVE_SIDE_CP
#define SECURE_ALIGN SIZE_1M
#else  // MASTER_SIDE_CP
#define SECURE_ALIGN SIZE_4K
#endif

#define PAGE_SIZE 4096
#define ROUND_UP_PAGESIZE(x) roundUpToPageSize(x)
inline int roundUpToPageSize(int x) {
  return (x + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}

enum OverflowType { ADD = 0, MUL };

#define OVERFLOW_MUL(x, y)                                                               \
  (sizeof(x) == 4) ? (((y) != 0) && ((x) > (std::numeric_limits<int32_t>::max() / (y)))) \
                   : (((y) != 0) && ((x) > (std::numeric_limits<int64_t>::max() / (y))))

#define OVERFLOW_ADD(x, y)                                                              \
  (sizeof(x) == 4) ? (((y) > 0) && ((x) > (std::numeric_limits<int32_t>::max() - (y)))) \
                   : (((y) > 0) && ((x) > (std::numeric_limits<int64_t>::max() - (y))))

#define OVERFLOW_ERR_RETURN(x, y, type)                       \
  {                                                           \
    if (type == OverflowType::ADD) {                          \
      if (OVERFLOW_ADD(x, y)) {                               \
        DLOGE("Addition overflow detected. Exiting..");       \
        return Error::BAD_VALUE;                              \
      }                                                       \
    } else if (type == OverflowType::MUL) {                   \
      if (OVERFLOW_MUL(x, y)) {                               \
        DLOGE("Multiplication overflow detected. Exiting.."); \
        return Error::BAD_VALUE;                              \
      }                                                       \
    }                                                         \
  }

#define UINT(exp) static_cast<unsigned int>(exp)

#define PROPERTY_VALUE_MAX 255
extern bool enable_logs;

inline int ALIGN(int operand, int alignment) {
  int max_val = std::numeric_limits<int>::max();
  if (operand > (max_val - (int)alignment)) {
    return operand;
  }

  int remainder = (operand % alignment);

  return (0 == remainder) ? operand : operand - remainder + alignment;
}

uint64_t GetPixelFormatModifier(BufferDescriptor desc);

bool CpuCanRead(vendor_qti_hardware_display_common_BufferUsage usage);
bool CpuCanWrite(vendor_qti_hardware_display_common_BufferUsage usage);
bool CpuCanAccess(vendor_qti_hardware_display_common_BufferUsage usage);

struct SnapFormatDescriptor {
  vendor_qti_hardware_display_common_PixelFormat format;
  vendor_qti_hardware_display_common_PixelFormatModifier modifier;

  bool operator==(const SnapFormatDescriptor &snap_fmt_desc) const {
    if (format == snap_fmt_desc.format && modifier == snap_fmt_desc.modifier) {
      return true;
    }
    return false;
  }
};

class SnapFormatDescriptorHash {
 public:
  size_t operator()(const SnapFormatDescriptor &snap_fmt_desc) const {
    return (std::hash<int>{}(static_cast<uint64_t>(snap_fmt_desc.format)) ^
            std::hash<int>{}(static_cast<uint64_t>(snap_fmt_desc.modifier)));
  }
};

class MmmColorFormatMapper {
 public:
  MmmColorFormatMapper(){};
  ~MmmColorFormatMapper(){};
  static inline unsigned int GetYStride(unsigned int color_fmt, unsigned int width) {
    return MMM_COLOR_FMT_Y_STRIDE(color_fmt, width);
  }
  static inline unsigned int GetUVStride(unsigned int color_fmt, unsigned int width) {
    return MMM_COLOR_FMT_UV_STRIDE(color_fmt, width);
  }
  static inline unsigned int GetYScanlines(unsigned int color_fmt, unsigned int height) {
    return MMM_COLOR_FMT_Y_SCANLINES(color_fmt, height);
  }
  static inline unsigned int GetUVScanlines(unsigned int color_fmt, unsigned int height) {
    return MMM_COLOR_FMT_UV_SCANLINES(color_fmt, height);
  }
  static inline unsigned int GetYMetaStride(unsigned int color_fmt, unsigned int width) {
    return MMM_COLOR_FMT_Y_META_STRIDE(color_fmt, width);
  }
  static inline unsigned int GetYMetaScanlines(unsigned int color_fmt, unsigned int height) {
    return MMM_COLOR_FMT_Y_META_SCANLINES(color_fmt, height);
  }
  static inline unsigned int GetUVMetaStride(unsigned int color_fmt, unsigned int width) {
    return MMM_COLOR_FMT_UV_META_STRIDE(color_fmt, width);
  }
  static inline unsigned int GetUVMetaScanlines(unsigned int color_fmt, unsigned int height) {
    return MMM_COLOR_FMT_UV_META_SCANLINES(color_fmt, height);
  }
  static inline unsigned int GetRgbStride(unsigned int color_fmt, unsigned int width) {
    return MMM_COLOR_FMT_RGB_STRIDE(color_fmt, width);
  }
  static inline unsigned int GetRgbScanlines(unsigned int color_fmt, unsigned int height) {
    return MMM_COLOR_FMT_RGB_SCANLINES(color_fmt, height);
  }
  static inline unsigned int GetRgbMetaStride(unsigned int color_fmt, unsigned int width) {
    return MMM_COLOR_FMT_RGB_META_STRIDE(color_fmt, width);
  }
  static inline unsigned int GetRgbMetaScanlines(unsigned int color_fmt, unsigned int height) {
    return MMM_COLOR_FMT_RGB_META_SCANLINES(color_fmt, height);
  }
  static inline unsigned int GetBufferSize(unsigned int color_fmt, unsigned int width,
                                           unsigned int height) {
    return MMM_COLOR_FMT_BUFFER_SIZE(color_fmt, width, height);
  }
  static inline unsigned int GetBufferSizeUsed(unsigned int color_fmt, unsigned int width,
                                               unsigned int height, unsigned int interlace) {
    return MMM_COLOR_FMT_BUFFER_SIZE_USED(color_fmt, width, height, interlace);
  }

  int MapPixelFormatWithMmmColorFormat(
      vendor_qti_hardware_display_common_PixelFormat snap_format,
      vendor_qti_hardware_display_common_BufferUsage usage,
      vendor_qti_hardware_display_common_PixelFormatModifier modifier, bool ubwc_enabled,
      int compression_ratio = 0) {
    switch (snap_format) {
      case vendor_qti_hardware_display_common_PixelFormat::RGBA_8888:
      case vendor_qti_hardware_display_common_PixelFormat::RGBX_8888: {
        if (ubwc_enabled) {
          #ifdef DRM_FORMAT_MOD_QCOM_LOSSY_8_5
          if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_L_8_TO_5) {
            return mmm_color_fmts::MMM_COLOR_FMT_RGBA8888_L_8_5_UBWC;
          }
          if (usage & vendor_qti_hardware_display_common_BufferUsage::QTI_ALLOC_UBWC_L_2_TO_1) {
            return mmm_color_fmts::MMM_COLOR_FMT_RGBA8888_L_2_1_UBWC;
          }
          #endif
          return mmm_color_fmts::MMM_COLOR_FMT_RGBA8888_UBWC;
        }
        return mmm_color_fmts::MMM_COLOR_FMT_RGBA8888;
      }
      case vendor_qti_hardware_display_common_PixelFormat::RGBA_1010102: {
        if (ubwc_enabled) {
          return mmm_color_fmts::MMM_COLOR_FMT_RGBA1010102_UBWC;
        }
        return -1;
      }
      case vendor_qti_hardware_display_common_PixelFormat::BGR_565:
      case vendor_qti_hardware_display_common_PixelFormat::RGB_565: {
        if (ubwc_enabled) {
          return mmm_color_fmts::MMM_COLOR_FMT_RGB565_UBWC;
        }
        return -1;
      }
      case vendor_qti_hardware_display_common_PixelFormat::RGBA_FP16: {
        if (ubwc_enabled) {
          return MMM_COLOR_FMT_RGBA16161616F_UBWC;
        }
        return -1;
      }
      case vendor_qti_hardware_display_common_PixelFormat::YCBCR_P010: {
        if (ubwc_enabled) {
          return mmm_color_fmts::MMM_COLOR_FMT_P010_UBWC;
        }
        if (usage & vendor_qti_hardware_display_common_BufferUsage::HW_IMAGE_ENCODER) {
          return mmm_color_fmts::MMM_COLOR_FMT_P010_512;
        }
        return mmm_color_fmts::MMM_COLOR_FMT_P010;
      }
      case vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP: {
        if (ubwc_enabled) {
          if (modifier == PIXEL_FORMAT_MODIFIER_4R) {
            return mmm_color_fmts::MMM_COLOR_FMT_NV124R_UBWC;
          }
          return mmm_color_fmts::MMM_COLOR_FMT_NV12_UBWC;
        } else if (usage & vendor_qti_hardware_display_common_BufferUsage::HW_IMAGE_ENCODER) {
          return mmm_color_fmts::MMM_COLOR_FMT_NV12_512;
        } else if ((modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX) ||
                   (modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_2_BATCH) ||
                   (modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_4_BATCH) ||
                   (modifier == PIXEL_FORMAT_MODIFIER_UBWC_FLEX_8_BATCH)) {
          return mmm_color_fmts::MMM_COLOR_FMT_NV12_UBWC;
        } else {
          return mmm_color_fmts::MMM_COLOR_FMT_NV12;
        }
      }
      case vendor_qti_hardware_display_common_PixelFormat::YCrCb_420_SP: {
        return mmm_color_fmts::MMM_COLOR_FMT_NV21;
      }
      case vendor_qti_hardware_display_common_PixelFormat::TP10: {
        return mmm_color_fmts::MMM_COLOR_FMT_NV12_BPP10_UBWC;
      }
      default:
        return -1;
    }
  }
};

struct FormatTraits {
  bool rgb;
  bool yuv;
  bool tile_rendered;
  bool gpu_depth_stencil;
  bool astc;
  bool ubwc_supported;
  bool width_even;
  bool height_even;
};

bool IsUbwcSupported(vendor_qti_hardware_display_common_PixelFormat format);
bool IsTileRendered(vendor_qti_hardware_display_common_PixelFormat format);
bool IsAstc(vendor_qti_hardware_display_common_PixelFormat format);
bool IsRgb(vendor_qti_hardware_display_common_PixelFormat format);
bool IsYuv(vendor_qti_hardware_display_common_PixelFormat format);
bool IsGpuDepthStencil(vendor_qti_hardware_display_common_PixelFormat format);
bool CheckWidthConstraints(
    vendor_qti_hardware_display_common_PixelFormat format, int width);
bool CheckHeightConstraints(
    vendor_qti_hardware_display_common_PixelFormat format, int height);

#define QTI_VT_TIMESTAMP 10000
#define IS_VENDOR_METADATA_TYPE(x) (x >= QTI_VT_TIMESTAMP)
#define GET_STANDARD_METADATA_STATUS_INDEX(x) x
#define GET_VENDOR_METADATA_STATUS_INDEX(x) x - QTI_VT_TIMESTAMP

#endif  // __SNAP_UTILS_H__
