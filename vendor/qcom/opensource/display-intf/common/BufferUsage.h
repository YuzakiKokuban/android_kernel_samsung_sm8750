// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __COMMON_BUFFERUSAGE_H__
#define __COMMON_BUFFERUSAGE_H__

#include <cstdint>
typedef enum vendor_qti_hardware_display_common_BufferUsage : uint64_t {
  /** Bit 0-3 is an enum */
  CPU_READ_MASK = 0xf,
  /** Buffer is never read by CPU */
  CPU_READ_NEVER = 0,
  /** Buffer is rarely read by CPU */
  CPU_READ_RARELY = 2,
  /** Buffer is often read by CPU */
  CPU_READ_OFTEN = 3,
  /** Bit 4-7 is an enum */
  CPU_WRITE_MASK = 0xf << 4,
  /** Buffer is never written by CPU */
  CPU_WRITE_NEVER = 0 << 4,
  /** Buffer is rarely written by CPU */
  CPU_WRITE_RARELY = 2 << 4,
  /** Buffer is often written by CPU */
  CPU_WRITE_OFTEN = 3 << 4,
  /** Buffer is used as a GPU texture */
  GPU_TEXTURE = 1 << 8,
  /** Buffer is used as a GPU render target */
  GPU_RENDER_TARGET = 1 << 9,
  /** bit 10 must be zero */
  /** Buffer is used as a composer overlay layer */
  COMPOSER_OVERLAY = 1 << 11,
  /** Buffer is used as a composer client layer */
  COMPOSER_CLIENT_TARGET = 1 << 12,
  /** bit 13 must be zero */
  /** Buffer has protected content */
  PROTECTED = 1 << 14,
  /** Buffer is used as composer cursor layer */
  COMPOSER_CURSOR = 1 << 15,
  /** Buffer is used as video encoder input */
  VIDEO_ENCODER = 1 << 16,
  /** Buffer is used as camera output */
  CAMERA_OUTPUT = 1 << 17,
  /** Buffer is used as camera input */
  CAMERA_INPUT = 1 << 18,
  /** bit 19 must be zero */
  /** Buffer is used as a renderscript allocation */
  RENDERSCRIPT = 1 << 20,
  /** bit 21 must be zero */
  /** Buffer is used as video decoder output */
  VIDEO_DECODER = 1 << 22,
  /** Buffer is used as a sensor direct report output */
  SENSOR_DIRECT_DATA = 1 << 23,
  /**
   * Buffer is used as as an OpenGL shader storage or uniform
   * buffer object
   */
  GPU_DATA_BUFFER = 1 << 24,
  /** Buffer is used as a cube map texture */
  GPU_CUBE_MAP = 1 << 25,
  /** Buffer contains a complete mipmap hierarchy */
  GPU_MIPMAP_COMPLETE = 1 << 26,
  /** Buffer is used as input for HEIC encoder */
  HW_IMAGE_ENCODER = 1 << 27,

  /* Non linear, Universal Bandwidth Compression */
  QTI_ALLOC_UBWC = 1 << 28,

  /**
   * Buffer is allocated with uncached memory (using O_DSYNC),
   * cannot be used with noncontiguous heaps
   */
  QTI_PRIVATE_UNCACHED = 1 << 29,

  /* Buffer has a 10 bit format if format is implementation defined */
  QTI_PRIVATE_10BIT = 1 << 30,

  /* Buffer is used for secure display */
  QTI_PRIVATE_SECURE_DISPLAY = 1ULL << 31,

  /* Buffer is used for front-buffer rendering */
  FRONT_BUFFER = 1ULL << 32,

  /* Buffer is used for Samsung Fingerprint Mask View */
  QTI_PRIVATE_FINGERPRINT_MASK_BUFFER = 1L << 34,

  /* Bits 33-47 must be zero and are reserved for future versions
   * Bits 48-63 are reserved for vendor extensions
   */

  /* This flag is used to indicate video NV21 format */
  QTI_PRIVATE_VIDEO_NV21_ENCODER = 1ULL << 48,

  /* Buffer uses PI format */
  QTI_PRIVATE_ALLOC_UBWC_PI = 1ULL << 49,

  /* Buffer is accessed by CDSP */
  QTI_PRIVATE_CDSP = 1ULL << 50,

  /* Buffer is used for WFD */
  QTI_PRIVATE_WFD = 1ULL << 51,

  /* Buffer uses video HW use case  */
  QTI_PRIVATE_VIDEO_HW = 1ULL << 52,

  /* Buffer used for trusted VM use case */
  QTI_PRIVATE_TRUSTED_VM = 1ULL << 53,

  /* UBWC - NV12 4R */
  QTI_ALLOC_UBWC_4R = 1ULL << 55,

  /* UBWC - 8:5 compression ratio */
  QTI_ALLOC_UBWC_L_8_TO_5 = 1ULL << 56,

  /* UBWC - 2:1 compression ratio */
  QTI_ALLOC_UBWC_L_2_TO_1 = 1ULL << 57,

  /* This flag is used to indicate multiview use case */
  QTI_PRIVATE_MULTI_VIEW_INFO = 1ULL << 58,
} vendor_qti_hardware_display_common_BufferUsage;

inline vendor_qti_hardware_display_common_BufferUsage operator|(
    vendor_qti_hardware_display_common_BufferUsage lhs,
    vendor_qti_hardware_display_common_BufferUsage rhs) {
  return static_cast<vendor_qti_hardware_display_common_BufferUsage>(static_cast<uint64_t>(lhs) |
                                                                     static_cast<uint64_t>(rhs));
}

inline vendor_qti_hardware_display_common_BufferUsage operator&(
    vendor_qti_hardware_display_common_BufferUsage lhs,
    vendor_qti_hardware_display_common_BufferUsage rhs) {
  return static_cast<vendor_qti_hardware_display_common_BufferUsage>(static_cast<uint64_t>(lhs) &
                                                                     static_cast<uint64_t>(rhs));
}

inline vendor_qti_hardware_display_common_BufferUsage operator&=(
    vendor_qti_hardware_display_common_BufferUsage &lhs,
    vendor_qti_hardware_display_common_BufferUsage rhs) {
  lhs = lhs & rhs;
  return lhs;
}

inline vendor_qti_hardware_display_common_BufferUsage operator|=(
    vendor_qti_hardware_display_common_BufferUsage &lhs,
    vendor_qti_hardware_display_common_BufferUsage rhs) {
  lhs = lhs | rhs;
  return lhs;
}

#endif  // __COMMON_BUFFERUSAGE_H__
