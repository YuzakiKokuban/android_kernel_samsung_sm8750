/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __COMMON_PIXELFORMAT_H__
#define __COMMON_PIXELFORMAT_H__

#include <cstdint>

typedef enum vendor_qti_hardware_display_common_PixelFormat : uint32_t {
  // Range is 0x0 to 0x100 except for formats where fourcc code is used
  PIXEL_FORMAT_UNSPECIFIED = 0,
  /**
   * RGBA_8888 format:
   * Contains 1 plane in the following order -
   * (A) RGBA plane
   *
   * <-------- RGB_Stride -------->
   * <------- Width ------->
   * R G B A R G B A R G B A . . . .  ^           ^
   * R G B A R G B A R G B A . . . .  |           |
   * R G B A R G B A R G B A . . . .  Height      |
   * R G B A R G B A R G B A . . . .  |       RGB_Scanlines
   * R G B A R G B A R G B A . . . .  |           |
   * R G B A R G B A R G B A . . . .  |           |
   * R G B A R G B A R G B A . . . .  |           |
   * R G B A R G B A R G B A . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   */
  RGBA_8888 = 0x1,
  /**
   * RGBX_8888 format:
   * Contains 1 plane in the following order -
   * (A) RGBX plane, where X is an unused component
   *
   * <-------- RGB_Stride -------->
   * <------- Width ------->
   * R G B X R G B X R G B X . . . .  ^           ^
   * R G B X R G B X R G B X . . . .  |           |
   * R G B X R G B X R G B X . . . .  Height      |
   * R G B X R G B X R G B X . . . .  |       RGB_Scanlines
   * R G B X R G B X R G B X . . . .  |           |
   * R G B X R G B X R G B X . . . .  |           |
   * R G B X R G B X R G B X . . . .  |           |
   * R G B X R G B X R G B X . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   */
  RGBX_8888 = 0x2,
  /**
   * RGB_888 format:
   * Contains 1 plane in the following order -
   * (A) RGB plane
   *
   * <-------- RGB_Stride -------->
   * <------- Width ------->
   * R G B R G B R G B R G B . . . .  ^           ^
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  Height      |
   * R G B R G B R G B R G B . . . .  |       RGB_Scanlines
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   */
  RGB_888 = 0x3,
  /**
   * RGB_565 format:
   * Contains 1 plane in the following order -
   * (A) RGB plane
   *
   * <-------- RGB_Stride -------->
   * <------- Width ------->
   * R G B R G B R G B R G B . . . .  ^           ^
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  Height      |
   * R G B R G B R G B R G B . . . .  |       RGB_Scanlines
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  |           |
   * R G B R G B R G B R G B . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   */
  RGB_565 = 0x4,
  /**
   * BGRA_8888 format:
   * Contains 1 plane in the following order -
   * (A) BGRA plane
   *
   * <-------- RGB_Stride -------->
   * <------- Width ------->
   * B G R A B G R A B G R A . . . .  ^           ^
   * B G R A B G R A B G R A . . . .  |           |
   * B G R A B G R A B G R A . . . .  Height      |
   * B G R A B G R A B G R A . . . .  |       RGB_Scanlines
   * B G R A B G R A B G R A . . . .  |           |
   * B G R A B G R A B G R A . . . .  |           |
   * B G R A B G R A B G R A . . . .  |           |
   * B G R A B G R A B G R A . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   */
  BGRA_8888 = 0x5,
  YCBCR_422_SP = 0x10, //  NV16
  /**
   * YCrCb_420_SP format:
   * YUV 4:2:0 image with a plane of 8 bit Y samples followed
   * by an interleaved V/U plane containing 8 bit 2x2 subsampled
   * colour difference samples.
   *
   * <-------- Y/UV_Stride -------->
   * <------- Width ------->
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  ^           ^
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  Height      |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |          Y_Scanlines
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   * V U V U V U V U V U V U . . . .  ^
   * V U V U V U V U V U V U . . . .  |
   * V U V U V U V U V U V U . . . .  |
   * V U V U V U V U V U V U . . . .  UV_Scanlines
   * . . . . . . . . . . . . . . . .  |
   * . . . . . . . . . . . . . . . .  V
   * . . . . . . . . . . . . . . . .  --> Padding & Buffer size alignment
   *
   */
  YCrCb_420_SP = 0x11, //  NV21
  YCBCR_422_I = 0x14,  //  YUY2
  RGBA_FP16 = 0x16,
  RAW16 = 0x20,
  BLOB = 0x21,
  IMPLEMENTATION_DEFINED = 0x22,
  YCBCR_420_888 = 0x23,
  RAW_OPAQUE = 0x24,
  RAW10 = 0x25,
  RAW12 = 0x26,
  RAW14 = 0x144,
  RGBA_1010102 = 0x2B,
  Y8 = 0x20203859,
  Y16 = 0x20363159,
  YV12 = 0x32315659, //  YCrCb  4:2:0  Planar
  DEPTH_16 = 0x30,
  DEPTH_24 = 0x31,
  DEPTH_24_STENCIL_8 = 0x32,
  DEPTH_32F = 0x33,
  DEPTH_32F_STENCIL_8 = 0x34,
  STENCIL_8 = 0x35,
  /**
   * YCBCR_P010 format:
   * YUV 4:2:0 image with a plane of 10 bit Y samples followed
   * by an interleaved U/V plane containing 10 bit 2x2 subsampled
   * colour difference samples.
   *
   * <-------- Y/UV_Stride -------->
   * <------- Width ------->
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  ^           ^
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  Height      |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |          Y_Scanlines
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   * U V U V U V U V U V U V . . . .  ^
   * U V U V U V U V U V U V . . . .  |
   * U V U V U V U V U V U V . . . .  |
   * U V U V U V U V U V U V . . . .  UV_Scanlines
   * . . . . . . . . . . . . . . . .  |
   * . . . . . . . . . . . . . . . .  V
   * . . . . . . . . . . . . . . . .  --> Buffer size alignment
   *
   */
  YCBCR_P010 = 0x36,
  HSV_888 = 0x37,
  /* R8 format:
   * Contains 1 plane in the following order -
   * (A) R plane
   *
   * <-------- RGB_Stride -------->
   * <------- Width ------->
   * R R R R R R R R R R R R . . . .  ^           ^
   * R R R R R R R R R R R R . . . .  |           |
   * R R R R R R R R R R R R . . . .  Height      |
   * R R R R R R R R R R R R . . . .  |       RGB_Scanlines
   * R R R R R R R R R R R R . . . .  |           |
   * R R R R R R R R R R R R . . . .  |           |
   * R R R R R R R R R R R R . . . .  |           |
   * R R R R R R R R R R R R . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   *
   */
  R_8 = 0x38,
  RGBA_5551 = 0x6,
  RGBA_4444 = 0x7,
  /**
   * YCbCr_420_SP format:
   * YUV 4:2:0 image with a plane of 8 bit Y samples followed
   * by an interleaved U/V plane containing 8 bit 2x2 subsampled
   * colour difference samples.
   *
   * <-------- Y/UV_Stride -------->
   * <------- Width ------->
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  ^           ^
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  Height      |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |          Y_Scanlines
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  |           |
   * Y Y Y Y Y Y Y Y Y Y Y Y . . . .  V           |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              |
   * . . . . . . . . . . . . . . . .              V
   * U V U V U V U V U V U V . . . .  ^
   * U V U V U V U V U V U V . . . .  |
   * U V U V U V U V U V U V . . . .  |
   * U V U V U V U V U V U V . . . .  UV_Scanlines
   * . . . . . . . . . . . . . . . .  |
   * . . . . . . . . . . . . . . . .  V
   * . . . . . . . . . . . . . . . .  --> Padding & Buffer size alignment
   *
   */
  YCbCr_420_SP = 0x109,
  YCrCb_422_SP = 0x10B,
  RG_88 = 0x10E,
  YCbCr_444_SP = 0x10F,
  YCrCb_444_SP = 0x110,
  YCrCb_422_I = 0x111,
  BGRX_8888 = 0x112,
  NV21_ZSL = 0x113,
  BGR_565 = 0x115,
  RAW8 = 0x123,
  // 10  bit
  ARGB_2101010 = 0x117,
  RGBX_1010102 = 0x118,
  XRGB_2101010 = 0x119,
  BGRA_1010102 = 0x11A,
  ABGR_2101010 = 0x11B,
  BGRX_1010102 = 0x11C,
  XBGR_2101010 = 0x11D,
  TP10 = 0x7FA30C09,

  CbYCrY_422_I = 0x120,
  BGR_888 = 0x121,

  // Camera utils format
  MULTIPLANAR_FLEX = 0x127,

  // Khronos ASTC formats
  COMPRESSED_RGBA_ASTC_4x4_KHR = 0x93B0,
  COMPRESSED_RGBA_ASTC_5x4_KHR = 0x93B1,
  COMPRESSED_RGBA_ASTC_5x5_KHR = 0x93B2,
  COMPRESSED_RGBA_ASTC_6x5_KHR = 0x93B3,
  COMPRESSED_RGBA_ASTC_6x6_KHR = 0x93B4,
  COMPRESSED_RGBA_ASTC_8x5_KHR = 0x93B5,
  COMPRESSED_RGBA_ASTC_8x6_KHR = 0x93B6,
  COMPRESSED_RGBA_ASTC_8x8_KHR = 0x93B7,
  COMPRESSED_RGBA_ASTC_10x5_KHR = 0x93B8,
  COMPRESSED_RGBA_ASTC_10x6_KHR = 0x93B9,
  COMPRESSED_RGBA_ASTC_10x8_KHR = 0x93BA,
  COMPRESSED_RGBA_ASTC_10x10_KHR = 0x93BB,
  COMPRESSED_RGBA_ASTC_12x10_KHR = 0x93BC,
  COMPRESSED_RGBA_ASTC_12x12_KHR = 0x93BD,
  COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR = 0x93D0,
  COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR = 0x93D1,
  COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR = 0x93D2,
  COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR = 0x93D3,
  COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR = 0x93D4,
  COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR = 0x93D5,
  COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR = 0x93D6,
  COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR = 0x93D7,
  COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR = 0x93D8,
  COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR = 0x93D9,
  COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR = 0x93DA,
  COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = 0x93DB,
  COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = 0x93DC,
  COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = 0x93DD,

  /* Legacy formats/currently unsupported
   * R_16_UINT
   * RG_1616_UINT
   * RGBA_10101010
   * RGB888_UBWC_FSC
   * RGB101010_UBWC_FSC
   * YCbCr_422_I_10BIT
   * YCbCr_422_I_10BIT_COMPRESSED
   * YCbCr_420_SP_4R_UBWC
   **/

  /* Explicit UBWC formats are removed - set UBWC flag with corresponding linear
   *format YCbCr_420_SP_VENUS_UBWC YCbCr_420_P010_UBWC YCbCr_420_TP10_UBWC
   **/

  /* To be deprecated when ExtenableTypes are plumbed */
  NV12_ENCODEABLE = 0x102,
  NV21_ENCODEABLE = 0x7FA30C00,
  YCbCr_420_SP_VENUS = 0x7FA30C04,
  YCbCr_420_SP_TILED = 0x7FA30C03,
  YCrCb_420_SP_ADRENO = 0x7FA30C01,
  YCrCb_420_SP_VENUS = 0x114,
  YCbCr_420_P010_VENUS = 0x7FA30C0A,
  NV12_HEIF = 0x116,
  NV12_LINEAR_FLEX = 0x125,
  NV12_UBWC_FLEX = 0x126,
  NV12_UBWC_FLEX_2_BATCH = 0x128,
  NV12_UBWC_FLEX_4_BATCH = 0x129,
  NV12_UBWC_FLEX_8_BATCH = 0x130,
  /* --------------------------------------------------------------------------------*/

} vendor_qti_hardware_display_common_PixelFormat;

#endif  // __COMMON_PIXELFORMAT_H__
