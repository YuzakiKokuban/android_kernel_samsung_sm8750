// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __GRAPHICS_CONSTRAINT_PROVIDER_H__
#define __GRAPHICS_CONSTRAINT_PROVIDER_H__

#include <map>
#include <mutex>

#include "SnapConstraintProvider.h"
#include "SnapUtils.h"

typedef enum {
  SURFACE_TILE_MODE_DISABLE = 0x0,  // used for linear surface
  SURFACE_TILE_MODE_ENABLE = 0x1    // used for tiled surface
} surface_tile_mode_t;

typedef enum {
  SURFACE_RASTER_MODE_UNKNOWN = 0x0,  // used when we don't know the raster mode to be used
  SURFACE_RASTER_MODE_TW = 0x1,       // raster_mode = TypeWriter (TW)
  SURFACE_RASTER_MODE_CB = 0x2,       // raster_mode = CheckerBoard (CB)
} surface_rastermode_t;

// Adreno Pixel Formats
typedef enum {
  ADRENO_PIXELFORMAT_UNKNOWN = 0,
  ADRENO_PIXELFORMAT_R16G16B16A16_FLOAT = 10,
  ADRENO_PIXELFORMAT_R10G10B10A2_UNORM = 24,  // Vertex, Normalized GL_UNSIGNED_INT_10_10_10_2_OES
  ADRENO_PIXELFORMAT_R8G8B8A8 = 28,
  ADRENO_PIXELFORMAT_R8G8B8A8_SRGB = 29,
  ADRENO_PIXELFORMAT_D32_FLOAT = 40,
  ADRENO_PIXELFORMAT_D24_UNORM_S8_UINT = 45,
  ADRENO_PIXELFORMAT_R8G8_UNORM = 49,
  ADRENO_PIXELFORMAT_D16_UNORM = 55,
  ADRENO_PIXELFORMAT_R8_UNORM = 61,
  ADRENO_PIXELFORMAT_B5G6R5 = 85,
  ADRENO_PIXELFORMAT_B5G5R5A1 = 86,
  ADRENO_PIXELFORMAT_B8G8R8A8_UNORM = 87,
  ADRENO_PIXELFORMAT_B8G8R8A8 = 90,
  ADRENO_PIXELFORMAT_B8G8R8A8_SRGB = 91,
  ADRENO_PIXELFORMAT_B8G8R8X8_SRGB = 93,
  ADRENO_PIXELFORMAT_NV12 = 103,
  ADRENO_PIXELFORMAT_P010 = 104,
  ADRENO_PIXELFORMAT_YUY2 = 107,
  ADRENO_PIXELFORMAT_B4G4R4A4 = 115,
  ADRENO_PIXELFORMAT_NV12_EXT = 506,       // NV12 with non-std alignment and offsets
  ADRENO_PIXELFORMAT_R8G8B8X8 = 507,       //  GL_RGB8 (Internal)
  ADRENO_PIXELFORMAT_R8G8B8 = 508,         //  GL_RGB8
  ADRENO_PIXELFORMAT_A1B5G5R5 = 519,       //  GL_RGB5_A1
  ADRENO_PIXELFORMAT_R8G8B8X8_SRGB = 520,  //  GL_SRGB8
  ADRENO_PIXELFORMAT_R8G8B8_SRGB = 521,    //  GL_SRGB8
  ADRENO_PIXELFORMAT_A2B10G10R10_UNORM = 532,
  // Vertex, Normalized GL_UNSIGNED_INT_10_10_10_2_OES
  ADRENO_PIXELFORMAT_R10G10B10X2_UNORM = 537,
  ADRENO_PIXELFORMAT_D24_UNORM_X8_UINT = 548,
  ADRENO_PIXELFORMAT_D24_UNORM = 549,
  ADRENO_PIXELFORMAT_D32_FLOAT_X24S8_UINT = 551,
  ADRENO_PIXELFORMAT_S8_UINT = 552,
  ADRENO_PIXELFORMAT_ASTC_4X4 = 568,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_5X4 = 569,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_5X5 = 570,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_6X5 = 571,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_6X6 = 572,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_8X5 = 573,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_8X6 = 574,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_8X8 = 575,         // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X5 = 576,        // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X6 = 577,        // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X8 = 578,        // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X10 = 579,       // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_12X10 = 580,       // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_12X12 = 581,       // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_4X4_SRGB = 582,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_5X4_SRGB = 583,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_5X5_SRGB = 584,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_6X5_SRGB = 585,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_6X6_SRGB = 586,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_8X5_SRGB = 587,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_8X6_SRGB = 588,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_8X8_SRGB = 589,    // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X5_SRGB = 590,   // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X6_SRGB = 591,   // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X8_SRGB = 592,   // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_10X10_SRGB = 593,  // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_12X10_SRGB = 594,  // ASTC Compressed
  ADRENO_PIXELFORMAT_ASTC_12X12_SRGB = 595,  // ASTC Compressed
  // Vertex, Normalized GL_UNSIGNED_INT_10_10_10_2_OES
  ADRENO_PIXELFORMAT_R5G6B5 = 610,    //  RGBA version of B5G6R5
  ADRENO_PIXELFORMAT_R5G5B5A1 = 611,  //  RGBA version of B5G5R5A1
  ADRENO_PIXELFORMAT_R4G4B4A4 = 612,  //  RGBA version of B4G4R4A4
  ADRENO_PIXELFORMAT_UYVY = 614,      //  YUV 4:2:2 packed progressive (1 plane)
  ADRENO_PIXELFORMAT_YV12 = 616,
  ADRENO_PIXELFORMAT_NV21 = 619,
  ADRENO_PIXELFORMAT_Y8U8V8A8 = 620,  // YUV 4:4:4 packed (1 plane)
  ADRENO_PIXELFORMAT_Y8 = 625,        //  Single 8-bit luma only channel YUV format
  ADRENO_PIXELFORMAT_TP10 = 654,      // YUV 4:2:0 planar 10 bits/comp (2 planes)
  ADRENO_PIXELFORMAT_NV12_4R = 660,   // Same as NV12, but with different tiling
} ADRENOPIXELFORMAT;

namespace snapalloc {
class GraphicsConstraintProvider : public SnapConstraintProvider {
 public:
  GraphicsConstraintProvider(GraphicsConstraintProvider &other) = delete;
  void operator=(const GraphicsConstraintProvider &) = delete;
  static GraphicsConstraintProvider *GetInstance(
      std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map = {});

  /* Attempt to dlopen the adreno library.
   * If this fails, the GraphicsConstraintProvider will use the default alignment table.
   * If the library is available, it will queried for one-time capabilities.
   * @return GraphicsConstraintProvider instance
   */
  void Init(std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map);
  int GetProviderType() { return kGraphics; }

  int GetCapabilities(BufferDescriptor desc, CapabilitySet *out);
  int GetConstraints(BufferDescriptor desc, BufferConstraints *out);
  ADRENOPIXELFORMAT GetGpuPixelFormat(
      vendor_qti_hardware_display_common_PixelFormat Snap_format,
      vendor_qti_hardware_display_common_PixelFormatModifier modifier);
  bool AdrenoSizeAPIAvaliable();
  int GetInitialMetadata(BufferDescriptor desc,
                         vendor_qti_hardware_display_common_GraphicsMetadata *graphics_metadata,
                         bool is_ubwc_enabled);
  uint32_t AdrenoGetMetadataBlobSize();
  uint32_t AdrenoGetAlignedGpuBufferSize(void *metadata_blob);
  bool IsUBWCSupportedByGPU(vendor_qti_hardware_display_common_PixelFormat format,
                            vendor_qti_hardware_display_common_PixelFormatModifier modifier);
  int BuildConstraints(BufferDescriptor desc, BufferConstraints *data);

 private:
  GraphicsConstraintProvider(){};
  ~GraphicsConstraintProvider(){};
  static std::mutex graphics_provider_mutex_;
  static GraphicsConstraintProvider *instance_;

  void *lib_ = nullptr;
  std::map<vendor_qti_hardware_display_common_PixelFormat, BufferConstraints> constraint_set_map_;
  std::map<vendor_qti_hardware_display_common_PixelFormat, FormatData> format_data_map_;
  void GetAlignedWidthAndHeight(int width, int height, int format, int usage,
                                unsigned int *aligned_w, unsigned int *aligned_h, bool ubwc_enabled,
                                bool tile_enabled);
  void AlignUnCompressedRGB(int width, int height, int format, int tileEnabled,
                            int pixel_format_modifier, unsigned int *aligned_w,
                            unsigned int *aligned_h);
  void AlignCompressedRGB(int width, int height, int format, unsigned int *aligned_w,
                          unsigned int *aligned_h);
  void AlignGpuDepthStencilFormat(int width, int height, int format, int tile_enabled,
                                  int pixel_format_modifier, unsigned int *aligned_w,
                                  unsigned int *aligned_h);
  uint32_t GetGpuPixelAlignment();
  bool IsPISupportedByGPU(int format, uint64_t usage);
  int AdrenoInitMemoryLayout(void *metadata_blob, int width, int height, int depth, int format,
                             int num_samples, int isUBWC, uint64_t usage, uint32_t num_planes);

  int GetBppFromFormatData(vendor_qti_hardware_display_common_PixelFormat Snap_format);
  // link(s)to adreno surface padding library.
  int (*LINK_adreno_compute_padding)(int width, int bpp, int surface_tile_height,
                                     surface_rastermode_t raster_mode,
                                     int padding_threshold) = NULL;
  void (*LINK_adreno_compute_aligned_width_and_height)(int width, int height, int bpp,
                                                       surface_tile_mode_t tile_mode,
                                                       surface_rastermode_t raster_mode,
                                                       int padding_threshold, int *aligned_w,
                                                       int *aligned_h) = NULL;
  void (*LINK_adreno_compute_fmt_aligned_width_and_height)(
      int width, int height, int plane_id, ADRENOPIXELFORMAT format, uint32_t num_samples,
      surface_tile_mode_t tile_mode, surface_rastermode_t raster_mode, int padding_threshold,
      int *aligned_w, int *aligned_h) = NULL;
  void (*LINK_adreno_compute_compressedfmt_aligned_width_and_height)(
      int width, int height, int format, surface_tile_mode_t tile_mode,
      surface_rastermode_t raster_mode, int padding_threshold, int *aligned_w, int *aligned_h,
      int *bpp) = NULL;
  int (*LINK_adreno_isUBWCSupportedByGpu)(ADRENOPIXELFORMAT format) = NULL;
  unsigned int (*LINK_adreno_get_gpu_pixel_alignment)(void) = NULL;

  uint32_t (*LINK_adreno_get_metadata_blob_size)(void) = NULL;
  int (*LINK_adreno_init_memory_layout)(void *metadata_blob, int width, int height, int depth,
                                        ADRENOPIXELFORMAT format, uint32_t num_samples,
                                        surface_tile_mode_t tile_mode, uint64_t usage,
                                        uint32_t num_planes) = NULL;
  uint64_t (*LINK_adreno_get_aligned_gpu_buffer_size)(void *metadata_blob) = NULL;
  int (*LINK_adreno_isPISupportedByGpu)(int format, uint64_t usage) = NULL;

  int AdrenoInitMemoryLayout(void *metadata_blob, int width, int height, int depth,
                             ADRENOPIXELFORMAT format, int num_samples, int isUBWC, uint64_t usage,
                             uint32_t num_planes);
  bool gfx_ubwc_disable_ = false;
  bool gfx_ahardware_buffer_disable_ = false;
  std::unordered_map<SnapFormatDescriptor, ADRENOPIXELFORMAT, SnapFormatDescriptorHash>
      snap_to_adreno_pixel_format_ = {
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_8888,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R8G8B8A8},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_FP16,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R16G16B16A16_FLOAT},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBX_8888,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R8G8B8X8},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGRA_8888,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_B8G8R8A8_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGB_888,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R8G8B8},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGB_565,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_B5G6R5},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGR_565,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R5G6B5},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_5551,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R5G5B5A1},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_4444,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R4G4B4A4},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::R_8,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R8_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RG_88,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R8G8_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_1010102,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R10G10B10A2_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBX_1010102,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_R10G10B10X2_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::ABGR_2101010,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_A2B10G10R10_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_NV12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX},
           ADRENO_PIXELFORMAT_NV12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX_2_BATCH},
           ADRENO_PIXELFORMAT_NV12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX_4_BATCH},
           ADRENO_PIXELFORMAT_NV12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_UBWC_FLEX_8_BATCH},
           ADRENO_PIXELFORMAT_NV12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_VENUS},
           ADRENO_PIXELFORMAT_NV12_EXT},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .modifier = PIXEL_FORMAT_MODIFIER_4R},
           ADRENO_PIXELFORMAT_NV12_4R},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCBCR_P010,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_P010},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::TP10,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_TP10},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::DEPTH_16,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_D16_UNORM},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::DEPTH_24,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_D24_UNORM_X8_UINT},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::DEPTH_24_STENCIL_8,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_D24_UNORM_S8_UINT},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::DEPTH_32F,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_D32_FLOAT},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::STENCIL_8,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_S8_UINT},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_4x4_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_4X4},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_4X4_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_5x4_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_5X4},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_5X4_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_5x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_5X5},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_5X5_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_6x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_6X5},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_6X5_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_6x6_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_6X6},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_6X6_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_8x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_8X5},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_8X5_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_8x6_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_8X6},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_8X6_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_8x8_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_8X8},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_8X8_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X5},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X5_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x6_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X6},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X6_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x8_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X8},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X8_SRGB},
          {{.format =
                vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_10x10_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X10},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_10X10_SRGB},
          {{.format =
                vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_12x10_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_12X10},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_12X10_SRGB},
          {{.format =
                vendor_qti_hardware_display_common_PixelFormat::COMPRESSED_RGBA_ASTC_12x12_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_12X12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::
                COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_ASTC_12X12_SRGB},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YV12,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_YV12},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_I,
            .modifier = PIXEL_FORMAT_MODIFIER_NONE},
           ADRENO_PIXELFORMAT_YUY2},
      };

};
}  // namespace snapalloc

#endif  // __GRAPHICS_CONSTRAINT_PROVIDER_H__
