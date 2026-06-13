/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_LAYERS_H__
#define __SDM_LAYERS_H__

/* This class translates SDM3 Layer functions to the SDM LayerStack
 */

#include <Dataspace.h>
#include <Error.h>
#include <ISnapMapper.h>
#include <QtiColorRemappingInfo.h>
#include <QtiContentLightLevel.h>
#include <QtiDynamicMetadata.h>
#include <QtiGammaTransfer.h>
#include <QtiMasteringDisplay.h>
#include <QtiMatrixCoEfficients.h>
#include <SnapHandle.h>
#include <UBWCStats.h>
#include <VideoHistogramMetadata.h>
#include <VideoTimestampInfo.h>
#include <core/layer_buffer.h>
#include <core/layer_stack.h>
#include <utils/utils.h>

#include <map>
#include <set>

#include "core/buffer_allocator.h"

namespace sdm {

using PerFrameMetadataKey = uint32_t;
using BufferUsage = vendor_qti_hardware_display_common_BufferUsage;
using Dataspace = vendor_qti_hardware_display_common_Dataspace;
using Error = vendor::qti::hardware::display::snapalloc::Error;
using ISnapMapper = vendor::qti::hardware::display::snapalloc::ISnapMapper;
using MetadataType = vendor_qti_hardware_display_common_MetadataType;
using QtiColorPrimaries = vendor_qti_hardware_display_common_QtiColorPrimaries;
using QtiColorRange = vendor_qti_hardware_display_common_QtiColorRange;
using QtiColorRemappingInfo =
    vendor_qti_hardware_display_common_QtiColorRemappingInfo;
using QtiContentLightLevel =
    vendor_qti_hardware_display_common_QtiContentLightLevel;
using QtiDynamicMetadata =
    vendor_qti_hardware_display_common_QtiDynamicMetadata;
using QtiGammaTransfer = vendor_qti_hardware_display_common_QtiGammaTransfer;
using QtiMatrixCoEfficients =
    vendor_qti_hardware_display_common_QtiMatrixCoEfficients;
using QtiMasteringDisplay =
    vendor_qti_hardware_display_common_QtiMasteringDisplay;
using SnapHandle = ::vendor::qti::hardware::display::snapalloc::SnapHandle;
using UBWCStats = vendor_qti_hardware_display_common_UBWCStats;
using VideoHistogramMetadata =
    vendor_qti_hardware_display_common_VideoHistogramMetadata;
using VideoTimestampInfo =
    vendor_qti_hardware_display_common_VideoTimestampInfo;

// intermediate struct to hold some color metadata values which will be queried
// individually only used here for ease of access / convenience in populating
// each metadata member
struct ColorMetadata {
  Dataspace dataspace;
  QtiMatrixCoEfficients matrixCoefficients;
  QtiMasteringDisplay masteringDisplayInfo;
  QtiContentLightLevel contentLightLevel;
  QtiColorRemappingInfo cRI;
  QtiDynamicMetadata dynamicMetadata;
};

Error SetCSC(const SnapHandle *handle, ColorMetadata *color_metadata, std::shared_ptr<ISnapMapper> snapmapper_);
Error GetMetadata(const SnapHandle *handle, MetadataType type, void *out,
                  std::shared_ptr<ISnapMapper> snapmapper_);
bool IsBT2020(const QtiColorPrimaries &color_primary);
bool IsBT2020(const QtiColorPrimaries &color_primary);

class SDMLayer {
public:
  explicit SDMLayer(Display display_id, BufferAllocator *buf_allocator);
  ~SDMLayer();
  uint32_t GetZ() const { return z_; }
  LayerId GetId() const { return id_; }
  std::string GetName() const { return name_; }
  SDMLayerTypes GetType() const { return type_; }
  Layer *GetSDMLayer() { return layer_; }
  void ResetPerFrameData();

  DisplayError SetLayerBlendMode(LayerBlending mode);
  DisplayError SetLayerBuffer(const SnapHandle *buffer,
                              shared_ptr<Fence> acquire_fence);
  DisplayError SetLayerColor(SDMColor color);
  DisplayError SetLayerCompositionType(SDMCompositionType type);
  DisplayError SetLayerDataspace(int32_t dataspace);
  DisplayError SetLayerDisplayFrame(SDMRect frame);
  DisplayError SetCursorPosition(int32_t x, int32_t y);
  DisplayError SetLayerPlaneAlpha(float alpha);
  DisplayError SetLayerSourceCrop(SDMRect crop);
  DisplayError SetLayerSurfaceDamage(SDMRegion damage);
  DisplayError SetLayerTransform(SDMTransform transform);
  DisplayError SetLayerVisibleRegion(SDMRegion visible);
  DisplayError SetLayerPerFrameMetadata(uint32_t num_elements,
                                        const SDMPerFrameMetadataKey *keys,
                                        const float *metadata);
  DisplayError SetLayerPerFrameMetadataBlobs(uint32_t num_elements,
                                             const SDMPerFrameMetadataKey *keys,
                                             const uint32_t *sizes,
                                             const uint8_t *metadata);
  DisplayError SetLayerZOrder(uint32_t z);
  DisplayError SetLayerType(SDMLayerTypes type);
  DisplayError SetLayerFlag(SDMLayerFlag flag);
  DisplayError SetLayerColorTransform(const float *matrix);
  DisplayError SetLayerBrightness(float brightness);
  void SetComposition(const LayerComposition &sdm_composition);
  SDMCompositionType GetClientRequestedCompositionType() {
    return client_requested_;
  }
  SDMCompositionType GetOrigClientRequestedCompositionType() {
    return client_requested_orig_;
  }
  void UpdateClientCompositionType(SDMCompositionType type) {
    client_requested_ = type;
  }
  SDMCompositionType GetDeviceSelectedCompositionType() {
    return device_selected_;
  }
  int32_t GetLayerDataspace() { return dataspace_; }
  uint32_t GetGeometryChanges() { return geometry_changes_; }
  void ResetGeometryChanges();
  void ResetValidation() { layer_->update_mask.reset(); }
  bool NeedsValidation() {
    return (geometry_changes_ || layer_->update_mask.any());
  }
  bool IsSingleBuffered() { return single_buffer_; }
  bool IsScalingPresent();
  bool IsRotationPresent();
  bool IsDataSpaceSupported();
  bool IsProtected() { return secure_; }
  bool IsSurfaceUpdated() { return surface_updated_; }
  bool IsNonIntegralSourceCrop() { return non_integral_source_crop_; }
  bool HasMetaDataRefreshRate() { return has_metadata_refresh_rate_; }
  bool IsColorTransformSet() { return color_transform_matrix_set_; }
  void SetLayerAsMask();
  bool BufferLatched() { return buffer_flipped_; }
  void ResetBufferFlip() { buffer_flipped_ = false; }
  shared_ptr<Fence> GetReleaseFence();
  void SetReleaseFence(const shared_ptr<Fence> &release_fence);
  bool IsLayerCompatible() { return compatible_; }
  void IgnoreSdrHistogramMetadata(bool disable) {
    ignore_sdr_histogram_md_ = disable;
  }

private:
  std::shared_ptr<ISnapMapper> snapmapper_;
  Layer *layer_ = nullptr;
  SDMLayerTypes type_ = kLayerUnknown;
  uint32_t z_ = 0;
  const LayerId id_;
  std::string name_;
  const Display display_id_;
  static std::atomic<LayerId> next_id_;
  shared_ptr<Fence> release_fence_;
  BufferAllocator *buffer_allocator_ = NULL;
  int32_t dataspace_ = 0;
  LayerTransform layer_transform_ = {};
  LayerRect dst_rect_ = {};
  bool single_buffer_ = false;
  int buffer_fd_ = -1;
  bool dataspace_supported_ = false;
  bool surface_updated_ = true;
  bool non_integral_source_crop_ = false;
  bool has_metadata_refresh_rate_ = false;
  bool color_transform_matrix_set_ = false;
  bool buffer_flipped_ = false;
  bool secure_ = false;
  bool compatible_ = false;
  bool ignore_sdr_histogram_md_ = false;

  // SDMCompositionType requested by client(SF) Original
  SDMCompositionType client_requested_orig_ = SDMCompositionType::COMP_DEVICE;
  // SDMCompositionType requested by client(SF) Modified for internal use
  SDMCompositionType client_requested_ = SDMCompositionType::COMP_DEVICE;
  // SDMCompositionType selected by SDM
  SDMCompositionType device_selected_ = SDMCompositionType::COMP_DEVICE;
  uint32_t geometry_changes_ = GeometryChanges::kNone;

  void SetRect(const SDMRect &source, LayerRect *target, bool round = false);
  uint32_t GetUint32Color(const SDMColor &source);

  void GetUBWCStatsFromMetaData(UBWCStats *cr_stats, UbwcCrStatsVector *cr_vec);
  DisplayError SetMetaData(const SnapHandle *handle, Layer *layer);
  uint32_t RoundToStandardFPS(float fps);
  void ValidateAndSetCSC(const SnapHandle *handle);
  void SetDirtyRegions(const SDMRegion& surface_damage);
};

struct SortLayersByZ {
  bool operator()(const SDMLayer *lhs, const SDMLayer *rhs) const {
    return lhs->GetZ() < rhs->GetZ();
  }
};

} // namespace sdm
#endif // __SDM_LAYERS_H__
