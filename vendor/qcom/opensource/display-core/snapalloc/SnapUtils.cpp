// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SnapUtils.h"

uint64_t GetPixelFormatModifier(BufferDescriptor desc) {
  for (auto type : desc.additionalOptions) {
    // TODO: use a versioned string
    if (std::strcmp(type.key, "pixel_format_modifier") == 0) {
      return type.value;
    }
  }
  return 0;
}

bool CpuCanRead(vendor_qti_hardware_display_common_BufferUsage usage) {
  return usage & vendor_qti_hardware_display_common_BufferUsage::CPU_READ_MASK;
}

bool CpuCanWrite(vendor_qti_hardware_display_common_BufferUsage usage) {
  return usage & vendor_qti_hardware_display_common_BufferUsage::CPU_WRITE_MASK;
}

bool CpuCanAccess(vendor_qti_hardware_display_common_BufferUsage usage) {
  return CpuCanRead(usage) || CpuCanWrite(usage);
}

// TODO: read this from formats.json

[[clang::no_destroy]] static std::unordered_map<vendor_qti_hardware_display_common_PixelFormat,
                                                FormatTraits>
    format_traits_map{
        // {{Format},{rgb,yuv,tile rendered, gpu depth stencil, astc,
        // ubwc_supported, width_even, height_even}}
        {{vendor_qti_hardware_display_common_PixelFormat::RGBA_8888},
         {true, false, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGBX_8888},
         {true, false, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGBA_FP16},
         {true, false, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCBCR_P010},
         {false, true, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::BGRA_8888},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGB_888},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP},
         {false, true, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::NV21_ZSL},
         {false, true, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCrCb_420_SP},
         {false, true, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::TP10},
         {false, true, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGB_565},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YV12},
         {false, true, false, false, false, false, true, true}},
        {{vendor_qti_hardware_display_common_PixelFormat::R_8},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGBA_1010102},
         {true, false, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::BGR_565},
         {true, false, false, false, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RG_88},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RAW8},
         {false, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RAW10},
         {false, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RAW12},
         {false, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RAW14},
         {false, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RAW16},
         {false, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::DEPTH_16},
         {false, false, true, true, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::DEPTH_24},
         {false, false, true, true, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::DEPTH_24_STENCIL_8},
         {false, false, true, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::DEPTH_32F},
         {false, false, true, true, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::DEPTH_32F_STENCIL_8},
         {false, false, true, true, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::STENCIL_8},
         {false, false, true, true, false, true, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::BLOB},
         {false, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_SP},
         {false, true, false, false, false, false, true, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_I},
         {false, true, false, false, false, false, true, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::CbYCrY_422_I},
         {false, true, false, false, false, false, true, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCrCb_422_SP},
         {false, true, false, false, false, false, true, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCBCR_420_888},
         {false, true, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::YCrCb_422_I},
         {false, true, false, false, false, false, true, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_4x4_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_5x4_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_5x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_6x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_6x6_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_8x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_8x6_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_8x8_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x6_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x8_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x10_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_12x10_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_12x12_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR},
         {true, false, false, false, true, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGBA_4444},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::RGBA_5551},
         {true, false, false, false, false, false, false, false}},
        {{vendor_qti_hardware_display_common_PixelFormat::Y16},
         {false, true, false, false, false, false, false, false}},
    };

bool IsUbwcSupported(vendor_qti_hardware_display_common_PixelFormat format) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.ubwc_supported) {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool IsTileRendered(vendor_qti_hardware_display_common_PixelFormat format) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.tile_rendered) {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool IsAstc(vendor_qti_hardware_display_common_PixelFormat format) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.astc) {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool IsRgb(vendor_qti_hardware_display_common_PixelFormat format) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.rgb) {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool IsYuv(vendor_qti_hardware_display_common_PixelFormat format) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.yuv) {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool IsGpuDepthStencil(vendor_qti_hardware_display_common_PixelFormat format) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.gpu_depth_stencil) {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool CheckWidthConstraints(
    vendor_qti_hardware_display_common_PixelFormat format, int width) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.width_even) {
      if (width & 1) {
        DLOGE("Width is odd for format %lu", static_cast<uint64_t>(format));
        return false;
      } else {
        return true;
      }
    } else {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}

bool CheckHeightConstraints(
    vendor_qti_hardware_display_common_PixelFormat format, int height) {
  auto format_traits = format_traits_map.find(format);
  if (format_traits != format_traits_map.end()) {
    if (format_traits->second.width_even) {
      if (height & 1) {
        DLOGE("Height is odd for format %lu", static_cast<uint64_t>(format));
        return false;
      } else {
        return true;
      }
    } else {
      return true;
    }
  } else {
    DLOGW("Format %lu not found in format traits map", static_cast<uint64_t>(format));
  }
  return false;
}
