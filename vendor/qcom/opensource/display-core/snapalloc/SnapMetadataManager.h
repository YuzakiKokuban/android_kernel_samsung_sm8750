// Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef __SNAP_METADATA_MANAGER_H__
#define __SNAP_METADATA_MANAGER_H__

#include <drm/drm_fourcc.h>
#include <sys/mman.h>

#include <cstdint>
#include <mutex>

#include "SnapConstraintManager.h"
#include "SnapHandleInternal.h"
#include "SnapMemAllocator.h"
#include "SnapMetadataManagerDefs.h"
#include "SnapTypes.h"
#include "SnapUtils.h"

namespace snapalloc {

class SnapMetadataManager {
 public:
  SnapMetadataManager(SnapMetadataManager &other) = delete;
  void operator=(const SnapMetadataManager &) = delete;
  static SnapMetadataManager *GetInstance();
  Error Get(SnapHandleInternal *hnd, vendor_qti_hardware_display_common_MetadataType type,
            void *out);
  Error Set(SnapHandleInternal *hnd, vendor_qti_hardware_display_common_MetadataType type,
            void *in);
  Error GetFromBufferDescriptor(BufferDescriptor desc,
                                vendor_qti_hardware_display_common_MetadataType type, void *out);
  Error DumpBuffer(SnapHandleInternal *hnd);
  Error DumpBuffers();
  uint64_t GetMetaDataSize(uint64_t reserved_region_size, uint64_t custom_content_md_region_size);
  Error ValidateAndMap(SnapHandleInternal *hnd);
  void UnmapAndReset(SnapHandleInternal *hnd);
  Error GetCustomDimensions(SnapHandleInternal *hnd, SnapMetadata *metadata, int32_t *stride,
                            int32_t *height);
  Error InitializeMetadata(
      SnapHandleInternal *hnd,
      vendor_qti_hardware_display_common_PixelFormat pixel_format_requested,
      BufferDescriptor out_desc, const AllocData ad,
      vendor_qti_hardware_display_common_BufferLayout *layout);  // TODO: make this API extensible
  uint32_t GetCustomContentMetadataSize(vendor_qti_hardware_display_common_PixelFormat format,
                                        vendor_qti_hardware_display_common_BufferUsage usage);
  Error GetMetadataState(SnapHandleInternal *hnd, vendor_qti_hardware_display_common_MetadataType type, bool *out);
  typedef Error (SnapMetadataManager::*MetadataHelper)(SnapMetadata *metadata,
                                                       SnapHandleInternal *handle, void *in_set,
                                                       void *out_get, BufferDescriptor *buf_des);

 private:
  ~SnapMetadataManager();
  SnapMetadataManager();

  static std::mutex metadata_mgr_mutex_;
  static SnapMetadataManager *instance_;
  SnapConstraintManager *constraint_mgr_ = nullptr;
  SnapMemAllocator *mem_allocator_ = nullptr;
  UBWCPolicy *ubwc_policy_ = nullptr;

  Error IsMetadataTypeSettable(vendor_qti_hardware_display_common_MetadataType type, bool *out);
  void SetMetadataState(SnapMetadata *metadata,
                        vendor_qti_hardware_display_common_MetadataType type, bool metadata_state);
  bool GetMetadataStateInternal(SnapMetadata *metadata,
                        vendor_qti_hardware_display_common_MetadataType type);
  int ConvertAndSetColorMetadata(SnapMetadata *metadata,
                                 vendor_qti_hardware_display_common_MetadataType type, void *out);
  int GetDRMFormat(vendor_qti_hardware_display_common_PixelFormat format,
                   vendor_qti_hardware_display_common_BufferUsage usage, int flags,
                   uint32_t *drm_format, uint64_t *drm_format_modifier);
  Error GetRgbDataAddress(SnapHandleInternal *hnd, void **rgb_data);
  Error BufferIDHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                       void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error NameHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                   void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error WidthHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                    void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error HeightHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                     void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error LayerCountHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                         void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error PixelFormatRequestedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                   void *in_set = nullptr, void *out_get = nullptr,
                                   BufferDescriptor *buf_des = nullptr);
  Error PixelFormatFourCCHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                void *in_set = nullptr, void *out_get = nullptr,
                                BufferDescriptor *buf_des = nullptr);
  Error DRMPixelFormatModifierHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                     void *in_set = nullptr, void *out_get = nullptr,
                                     BufferDescriptor *buf_des = nullptr);
  Error UsageHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                    void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error AllocationSizeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                             void *in_set = nullptr, void *out_get = nullptr,
                             BufferDescriptor *buf_des = nullptr);
  Error ProtectedContentHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                               void *in_set = nullptr, void *out_get = nullptr,
                               BufferDescriptor *buf_des = nullptr);
  Error CompressionHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                          void *in_set = nullptr, void *out_get = nullptr,
                          BufferDescriptor *buf_des = nullptr);
  Error InterlacedHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                         void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error ChromaSitingHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                           void *in_set = nullptr, void *out_get = nullptr,
                           BufferDescriptor *buf_des = nullptr);
  Error PlaneLayoutsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                           void *in_set = nullptr, void *out_get = nullptr,
                           BufferDescriptor *buf_des = nullptr);
  Error CropHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                   void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error DataspaceHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                        void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error BlendModeHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                        void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error VTTimestampHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                          void *in_set = nullptr, void *out_get = nullptr,
                          BufferDescriptor *buf_des = nullptr);
  Error PPParamInterlacedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                void *in_set = nullptr, void *out_get = nullptr,
                                BufferDescriptor *buf_des = nullptr);
  Error VideoPerfModeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                            void *in_set = nullptr, void *out_get = nullptr,
                            BufferDescriptor *buf_des = nullptr);
  Error GraphicsMetadataHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                               void *in_set = nullptr, void *out_get = nullptr,
                               BufferDescriptor *buf_des = nullptr);
  Error UBWCCRStatsInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                              void *in_set = nullptr, void *out_get = nullptr,
                              BufferDescriptor *buf_des = nullptr);
  Error RefreshRateHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                          void *in_set = nullptr, void *out_get = nullptr,
                          BufferDescriptor *buf_des = nullptr);
  Error MapSecureBufferHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                              void *in_set = nullptr, void *out_get = nullptr,
                              BufferDescriptor *buf_des = nullptr);
  Error LinearFormatHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                           void *in_set = nullptr, void *out_get = nullptr,
                           BufferDescriptor *buf_des = nullptr);
  Error SingleBufferModeHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                               void *in_set = nullptr, void *out_get = nullptr,
                               BufferDescriptor *buf_des = nullptr);
  Error CVPMetadataHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                          void *in_set = nullptr, void *out_get = nullptr,
                          BufferDescriptor *buf_des = nullptr);
  Error VideoHistogramStatsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                  void *in_set = nullptr, void *out_get = nullptr,
                                  BufferDescriptor *buf_des = nullptr);
  Error FDHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                 void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error AlignedWidthInPixelsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                   void *in_set = nullptr, void *out_get = nullptr,
                                   BufferDescriptor *buf_des = nullptr);
  Error AlignedHeightInPixelsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                    void *in_set = nullptr, void *out_get = nullptr,
                                    BufferDescriptor *buf_des = nullptr);
  Error StandardMetadataStatusHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                     void *in_set = nullptr, void *out_get = nullptr,
                                     BufferDescriptor *buf_des = nullptr);
  Error VendorMetadataStatusHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                   void *in_set = nullptr, void *out_get = nullptr,
                                   BufferDescriptor *buf_des = nullptr);
  Error BufferTypeHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                         void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error VideoTSInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                          void *in_set = nullptr, void *out_get = nullptr,
                          BufferDescriptor *buf_des = nullptr);
  Error CustomDimensionsStrideHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                     void *in_set = nullptr, void *out_get = nullptr,
                                     BufferDescriptor *buf_des = nullptr);
  Error CustomDimensionsHeightHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                     void *in_set = nullptr, void *out_get = nullptr,
                                     BufferDescriptor *buf_des = nullptr);
  Error RGBDataAddressHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                             void *in_set = nullptr, void *out_get = nullptr,
                             BufferDescriptor *buf_des = nullptr);
  Error BufferPermissionHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                               void *in_set = nullptr, void *out_get = nullptr,
                               BufferDescriptor *buf_des = nullptr);
  Error MemHandleHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                        void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error TimedRenderingHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                             void *in_set = nullptr, void *out_get = nullptr,
                             BufferDescriptor *buf_des = nullptr);
  Error CustomContentMetadataHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                    void *in_set = nullptr, void *out_get = nullptr,
                                    BufferDescriptor *buf_des = nullptr);
  Error VideoTranscodeStatsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                  void *in_set = nullptr, void *out_get = nullptr,
                                  BufferDescriptor *buf_des = nullptr);
  Error EarlyNotifyLineCountHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                   void *in_set = nullptr, void *out_get = nullptr,
                                   BufferDescriptor *buf_des = nullptr);
  Error ReservedRegionHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                             void *in_set = nullptr, void *out_get = nullptr,
                             BufferDescriptor *buf_des = nullptr);
  Error FormatModifierHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                             void *in_set = nullptr, void *out_get = nullptr,
                             BufferDescriptor *buf_des = nullptr);
  Error MasteringDisplayHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                               void *in_set = nullptr, void *out_get = nullptr,
                               BufferDescriptor *buf_des = nullptr);
  Error ContentLightLevelHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                void *in_set = nullptr, void *out_get = nullptr,
                                BufferDescriptor *buf_des = nullptr);
  Error DynamicMetadataHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                              void *in_set = nullptr, void *out_get = nullptr,
                              BufferDescriptor *buf_des = nullptr);
  Error MatrixCoefficientsHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                 void *in_set = nullptr, void *out_get = nullptr,
                                 BufferDescriptor *buf_des = nullptr);
  Error ColorRemappingInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                 void *in_set = nullptr, void *out_get = nullptr,
                                 BufferDescriptor *buf_des = nullptr);
  Error BaseAddressHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                          void *in_set = nullptr, void *out_get = nullptr,
                          BufferDescriptor *buf_des = nullptr);
  Error IsUBWCHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                     void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error IsTileRenderedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                             void *in_set = nullptr, void *out_get = nullptr,
                             BufferDescriptor *buf_des = nullptr);
  Error IsCachedHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                       void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error HeapNameHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                       void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error PixelFormatAllocatedHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                 void *in_set = nullptr, void *out_get = nullptr,
                                 BufferDescriptor *buf_des = nullptr);
  Error BufferDequeueDurationHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                    void *in_set = nullptr, void *out_get = nullptr,
                                    BufferDescriptor *buf_des = nullptr);
  Error AnamorphicCompressionHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                                    void *in_set = nullptr, void *out_get = nullptr,
                                    BufferDescriptor *buf_des = nullptr);
  Error BaseViewHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                       void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error MultiViewHelper(SnapMetadata *metadata, SnapHandleInternal *handle, void *in_set = nullptr,
                        void *out_get = nullptr, BufferDescriptor *buf_des = nullptr);
  Error ThreeDimensionalRefInfoHelper(SnapMetadata *metadata, SnapHandleInternal *handle,
                            void *in_set = nullptr, void *out_get = nullptr,
                            BufferDescriptor *buf_des = nullptr);

  struct DRMFormatDescriptor {
    uint32_t drm_format;
    uint64_t drm_modifier;
  };

  struct SnapFormatUsageDescriptor {
    vendor_qti_hardware_display_common_PixelFormat format;
    vendor_qti_hardware_display_common_Compression compression_type;
    bool operator==(const SnapFormatUsageDescriptor &snap_fmt_usage_desc) const {
      if (format == snap_fmt_usage_desc.format &&
          compression_type == snap_fmt_usage_desc.compression_type) {
        return true;
      }
      return false;
    }
  };

  class SnapFormatUsageDescriptorHash {
   public:
    size_t operator()(const SnapFormatUsageDescriptor &snap_fmt_usage_desc) const {
      return (std::hash<int>{}(static_cast<uint64_t>(snap_fmt_usage_desc.format)) ^
              std::hash<int>{}(static_cast<uint64_t>(snap_fmt_usage_desc.compression_type)));
    }
  };

  std::unordered_map<SnapFormatUsageDescriptor, DRMFormatDescriptor, SnapFormatUsageDescriptorHash>
      snap_to_drm_format_ = {
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_8888,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ABGR8888, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_5551,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ABGR1555, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_4444,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ABGR4444, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGRA_8888,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ARGB8888, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBX_8888,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_XBGR8888, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBX_8888,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_XBGR8888, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGRX_8888,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_XRGB8888, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGB_888,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_BGR888, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGB_565,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_BGR565, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGR_565,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_BGR565, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGR_565,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_BGR565, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_1010102,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ABGR2101010, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_1010102,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_ABGR2101010, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::ARGB_2101010,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_BGRA1010102, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBX_1010102,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_XBGR2101010, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBX_1010102,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_XBGR2101010, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::XRGB_2101010,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_BGRX1010102, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGRA_1010102,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ARGB2101010, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::ABGR_2101010,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_RGBA1010102, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::BGRX_1010102,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_XRGB2101010, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::XBGR_2101010,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_RGBX1010102, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCbCr_420_SP,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          /* TODO: refactor map to include modifier
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX, .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX, .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_TILE}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX_2_BATCH, .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX_2_BATCH, .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_TILE}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX_4_BATCH, .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX_4_BATCH, .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_TILE}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX_8_BATCH, .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_COMPRESSED}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::NV12_UBWC_FLEX_8_BATCH, .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_TILE}}, */
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCrCb_420_SP,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV21, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCBCR_P010,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12, .drm_modifier = DRM_FORMAT_MOD_QCOM_DX}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCBCR_P010,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12,
            .drm_modifier =
                static_cast<int>(DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_DX)}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::TP10,
            .compression_type = QTI_COMPRESSION_UBWC},
           {.drm_format = DRM_FORMAT_NV12,
            .drm_modifier = static_cast<int>(DRM_FORMAT_MOD_QCOM_COMPRESSED |
                                             DRM_FORMAT_MOD_QCOM_DX | DRM_FORMAT_MOD_QCOM_TIGHT)}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::TP10,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV12,
            .drm_modifier = static_cast<int>(DRM_FORMAT_MOD_QCOM_TILE | DRM_FORMAT_MOD_QCOM_DX |
                                             DRM_FORMAT_MOD_QCOM_TIGHT)}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YCBCR_422_SP,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_NV16, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::YV12,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_YVU420, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_FP16,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_ABGR16161616F, .drm_modifier = 0}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::R_8,
            .compression_type = COMPRESSION_NONE},
           {.drm_format = DRM_FORMAT_R8, .drm_modifier = 0}},
#ifdef DRM_FORMAT_MOD_QCOM_LOSSY_8_5
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_8888,
            .compression_type = QTI_COMPRESSION_UBWC_LOSSY_8_TO_5},
           {.drm_format = DRM_FORMAT_ABGR8888,
            .drm_modifier =
                static_cast<int>(DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_LOSSY_8_5)}},
          {{.format = vendor_qti_hardware_display_common_PixelFormat::RGBA_8888,
            .compression_type = QTI_COMPRESSION_UBWC_LOSSY_2_TO_1},
           {.drm_format = DRM_FORMAT_ABGR8888,
            .drm_modifier =
                static_cast<int>(DRM_FORMAT_MOD_QCOM_COMPRESSED | DRM_FORMAT_MOD_QCOM_LOSSY_2_1)}},
#endif
  };

  std::unordered_map<vendor_qti_hardware_display_common_MetadataType, MetadataHelper>
      metadata_helper_function_map = {
          {BUFFER_ID, &SnapMetadataManager::BufferIDHelper},
          {NAME, &SnapMetadataManager::NameHelper},
          {WIDTH, &SnapMetadataManager::WidthHelper},
          {HEIGHT, &SnapMetadataManager::HeightHelper},
          {LAYER_COUNT, &SnapMetadataManager::LayerCountHelper},
          {PIXEL_FORMAT_REQUESTED, &SnapMetadataManager::PixelFormatRequestedHelper},
          {PIXEL_FORMAT_FOURCC, &SnapMetadataManager::PixelFormatFourCCHelper},
          {DRM_PIXEL_FORMAT_MODIFIER, &SnapMetadataManager::DRMPixelFormatModifierHelper},
          {USAGE, &SnapMetadataManager::UsageHelper},
          {ALLOCATION_SIZE, &SnapMetadataManager::AllocationSizeHelper},
          {PROTECTED_CONTENT, &SnapMetadataManager::ProtectedContentHelper},
          {COMPRESSION, &SnapMetadataManager::CompressionHelper},
          {INTERLACED, &SnapMetadataManager::InterlacedHelper},
          {CHROMA_SITING, &SnapMetadataManager::ChromaSitingHelper},
          {PLANE_LAYOUTS, &SnapMetadataManager::PlaneLayoutsHelper},
          {CROP, &SnapMetadataManager::CropHelper},
          {DATASPACE, &SnapMetadataManager::DataspaceHelper},
          {BLEND_MODE, &SnapMetadataManager::BlendModeHelper},
          {VT_TIMESTAMP, &SnapMetadataManager::VTTimestampHelper},
          {PP_PARAM_INTERLACED, &SnapMetadataManager::PPParamInterlacedHelper},
          {VIDEO_PERF_MODE, &SnapMetadataManager::VideoPerfModeHelper},
          {GRAPHICS_METADATA, &SnapMetadataManager::GraphicsMetadataHelper},
          {UBWC_CR_STATS_INFO, &SnapMetadataManager::UBWCCRStatsInfoHelper},
          {REFRESH_RATE, &SnapMetadataManager::RefreshRateHelper},
          {MAP_SECURE_BUFFER, &SnapMetadataManager::MapSecureBufferHelper},
          {LINEAR_FORMAT, &SnapMetadataManager::LinearFormatHelper},
          {SINGLE_BUFFER_MODE, &SnapMetadataManager::SingleBufferModeHelper},
          {CVP_METADATA, &SnapMetadataManager::CVPMetadataHelper},
          {VIDEO_HISTOGRAM_STATS, &SnapMetadataManager::VideoHistogramStatsHelper},
          {FD, &SnapMetadataManager::FDHelper},
          {ALIGNED_WIDTH_IN_PIXELS, &SnapMetadataManager::AlignedWidthInPixelsHelper},
          {STRIDE, &SnapMetadataManager::AlignedWidthInPixelsHelper},
          {ALIGNED_HEIGHT_IN_PIXELS, &SnapMetadataManager::AlignedHeightInPixelsHelper},
          {STANDARD_METADATA_STATUS, &SnapMetadataManager::StandardMetadataStatusHelper},
          {VENDOR_METADATA_STATUS, &SnapMetadataManager::VendorMetadataStatusHelper},
          {BUFFER_TYPE, &SnapMetadataManager::BufferTypeHelper},
          {VIDEO_TS_INFO, &SnapMetadataManager::VideoTSInfoHelper},
          {CUSTOM_DIMENSIONS_STRIDE, &SnapMetadataManager::CustomDimensionsStrideHelper},
          {CUSTOM_DIMENSIONS_HEIGHT, &SnapMetadataManager::CustomDimensionsHeightHelper},
          {RGB_DATA_ADDRESS, &SnapMetadataManager::RGBDataAddressHelper},
          {BUFFER_PERMISSION, &SnapMetadataManager::BufferPermissionHelper},
          {MEM_HANDLE, &SnapMetadataManager::MemHandleHelper},
          {TIMED_RENDERING, &SnapMetadataManager::TimedRenderingHelper},
          {CUSTOM_CONTENT_METADATA, &SnapMetadataManager::CustomContentMetadataHelper},
          {VIDEO_TRANSCODE_STATS, &SnapMetadataManager::VideoTranscodeStatsHelper},
          {EARLYNOTIFY_LINECOUNT, &SnapMetadataManager::EarlyNotifyLineCountHelper},
          {RESERVED_REGION, &SnapMetadataManager::ReservedRegionHelper},
          {FORMAT_MODIFIER, &SnapMetadataManager::FormatModifierHelper},
          {MASTERING_DISPLAY, &SnapMetadataManager::MasteringDisplayHelper},
          {CONTENT_LIGHT_LEVEL, &SnapMetadataManager::ContentLightLevelHelper},
          {DYNAMIC_METADATA, &SnapMetadataManager::DynamicMetadataHelper},
          {MATRIX_COEFFICIENTS, &SnapMetadataManager::MatrixCoefficientsHelper},
          {COLOR_REMAPPING_INFO, &SnapMetadataManager::ColorRemappingInfoHelper},
          {BASE_ADDRESS, &SnapMetadataManager::BaseAddressHelper},
          {IS_UBWC, &SnapMetadataManager::IsUBWCHelper},
          {IS_TILE_RENDERED, &SnapMetadataManager::IsTileRenderedHelper},
          {IS_CACHED, &SnapMetadataManager::IsCachedHelper},
          {HEAP_NAME, &SnapMetadataManager::HeapNameHelper},
          {PIXEL_FORMAT_ALLOCATED, &SnapMetadataManager::PixelFormatAllocatedHelper},
          {BUFFER_DEQUEUE_DURATION, &SnapMetadataManager::BufferDequeueDurationHelper},
          {ANAMORPHIC_COMPRESSION_METADATA, &SnapMetadataManager::AnamorphicCompressionHelper},
          {BASE_VIEW, &SnapMetadataManager::BaseViewHelper},
          {MULTI_VIEW_INFO, &SnapMetadataManager::MultiViewHelper},
          {THREE_DIMENSIONAL_REF_INFO, &SnapMetadataManager::ThreeDimensionalRefInfoHelper},
  };
  struct metadata_traits {
    bool is_settable;
  };
  std::unordered_map<vendor_qti_hardware_display_common_MetadataType, metadata_traits>
      metadatatype_traits_map{
          {BUFFER_ID, {false}},
          {NAME, {false}},
          {WIDTH, {false}},
          {HEIGHT, {false}},
          {LAYER_COUNT, {false}},
          {PIXEL_FORMAT_REQUESTED, {false}},
          {PIXEL_FORMAT_FOURCC, {false}},
          {DRM_PIXEL_FORMAT_MODIFIER, {false}},
          {USAGE, {false}},
          {ALLOCATION_SIZE, {false}},
          {PROTECTED_CONTENT, {false}},
          {COMPRESSION, {false}},
          {INTERLACED, {false}},
          {CHROMA_SITING, {false}},
          {PLANE_LAYOUTS, {false}},
          {CROP, {true}},
          {DATASPACE, {true}},
          {BLEND_MODE, {true}},
          {VT_TIMESTAMP, {true}},
          {PP_PARAM_INTERLACED, {true}},
          {VIDEO_PERF_MODE, {true}},
          {GRAPHICS_METADATA, {true}},
          {UBWC_CR_STATS_INFO, {true}},
          {REFRESH_RATE, {true}},
          {MAP_SECURE_BUFFER, {true}},
          {LINEAR_FORMAT, {true}},
          {SINGLE_BUFFER_MODE, {true}},
          {CVP_METADATA, {true}},
          {VIDEO_HISTOGRAM_STATS, {true}},
          {FD, {false}},
          {ALIGNED_WIDTH_IN_PIXELS, {false}},
          {STRIDE, {false}},
          {ALIGNED_HEIGHT_IN_PIXELS, {false}},
          {STANDARD_METADATA_STATUS, {true}},
          {VENDOR_METADATA_STATUS, {true}},
          {BUFFER_TYPE, {false}},
          {VIDEO_TS_INFO, {true}},
          {CUSTOM_DIMENSIONS_STRIDE, {false}},
          {CUSTOM_DIMENSIONS_HEIGHT, {false}},
          {RGB_DATA_ADDRESS, {false}},
          {BUFFER_PERMISSION, {true}},
          {MEM_HANDLE, {false}},
          {TIMED_RENDERING, {true}},
          {CUSTOM_CONTENT_METADATA, {true}},
          {VIDEO_TRANSCODE_STATS, {true}},
          {EARLYNOTIFY_LINECOUNT, {true}},
          {RESERVED_REGION, {false}},
          {FORMAT_MODIFIER, {false}},
          {MASTERING_DISPLAY, {true}},
          {CONTENT_LIGHT_LEVEL, {true}},
          {DYNAMIC_METADATA, {true}},
          {MATRIX_COEFFICIENTS, {true}},
          {COLOR_REMAPPING_INFO, {true}},
          {BASE_ADDRESS, {false}},
          {IS_UBWC, {false}},
          {IS_TILE_RENDERED, {false}},
          {IS_CACHED, {false}},
          {HEAP_NAME, {false}},
          {PIXEL_FORMAT_ALLOCATED, {false}},
          {BUFFER_DEQUEUE_DURATION, {true}},
          {ANAMORPHIC_COMPRESSION_METADATA, {true}},
          {BASE_VIEW, {false}},
          {MULTI_VIEW_INFO, {false}},
          {THREE_DIMENSIONAL_REF_INFO, {false}},
      };
};
}  // namespace snapalloc

#endif  // __SNAP_METADATA_MANAGER_H__
