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
#include "sdm_layers.h"
#include "sdm_debugger.h"
#include <UBWCVersion.h>
#include <cmath>
#include <dlfcn.h>
#include <stdint.h>
#include <utility>
#include <atomic>
#include <utils/debug.h>

#define __CLASS__ "SDMLayer"

using sdm::DisplayError;

namespace sdm {

using UBWCVersion = vendor_qti_hardware_display_common_UBWCVersion;

std::atomic<LayerId> SDMLayer::next_id_(1);

Error GetMetadata(const SnapHandle *handle, MetadataType type, void *out,
                  std::shared_ptr<ISnapMapper> snapmapper_) {
  bool metadata_set = false;

  snapmapper_->GetMetadataState(*handle, type, &metadata_set);
  if (!metadata_set) {
    return Error::METADATA_NOT_SET;
  }
  return snapmapper_->GetMetadata(*handle, type, out);
}

Error SetCSC(const SnapHandle *handle, ColorMetadata *color_metadata, std::shared_ptr<ISnapMapper> snapmapper_) {
  snapmapper_->GetMetadata(*handle, MetadataType::DATASPACE, &color_metadata->dataspace);
  snapmapper_->GetMetadata(*handle, MetadataType::MATRIX_COEFFICIENTS, &color_metadata->matrixCoefficients);
  snapmapper_->GetMetadata(*handle, MetadataType::MASTERING_DISPLAY, &color_metadata->masteringDisplayInfo);
  snapmapper_->GetMetadata(*handle, MetadataType::CONTENT_LIGHT_LEVEL, &color_metadata->contentLightLevel);
  snapmapper_->GetMetadata(*handle, MetadataType::COLOR_REMAPPING_INFO, &color_metadata->cRI);
  snapmapper_->GetMetadata(*handle, MetadataType::DYNAMIC_METADATA, &color_metadata->dynamicMetadata);

  return Error::NONE;
}

bool IsHdr(const QtiColorPrimaries &color_primary,
           const QtiGammaTransfer &gamma_transfer) {
  return (color_primary == QtiColorPrimaries::QtiColorPrimaries_BT2020) &&
         ((gamma_transfer == QtiGammaTransfer::QtiTransfer_SMPTE_ST2084) ||
          (gamma_transfer == QtiGammaTransfer::QtiTransfer_HLG));
}

bool IsBT2020(const QtiColorPrimaries &color_primary) {
  switch (color_primary) {
  case QtiColorPrimaries::QtiColorPrimaries_BT2020:
    return true;
    break;
  default:
    return false;
  }
}

static bool IsSdrDimmingDisabled() {
  static bool read_prop = false;
  static bool disable_sdr_dimming = false;
  if (!read_prop) {
    int value = 0;
    SDMDebugHandler::Get()->GetProperty(DISABLE_SDR_DIMMING, &value);
    disable_sdr_dimming = (value == 1);
  }
  read_prop = true;
  return disable_sdr_dimming;
}

// Layer operations
SDMLayer::SDMLayer(Display display_id, BufferAllocator *buf_allocator)
    : id_(next_id_++), display_id_(display_id),
      buffer_allocator_(buf_allocator) {
  layer_ = new Layer();
  geometry_changes_ |= kAdded;

  // Initialize pointer to snapalloc
  const std::string snapalloc_lib_name =
      "vendor.qti.hardware.display.snapalloc-impl.so";
  void *snap_impl_lib_ = ::dlopen(snapalloc_lib_name.c_str(), RTLD_NOW);
  if (!snap_impl_lib_) {
    DLOGE("Dlopen error for snapalloc impl: %s", dlerror());
    return;
  }

  std::shared_ptr<ISnapMapper> (*LINK_FETCH_ISnapMapper)(DebugCallbackIntf *) = nullptr;
  *reinterpret_cast<void **>(&LINK_FETCH_ISnapMapper) =
      ::dlsym(snap_impl_lib_, "FETCH_ISnapMapper");
  if (LINK_FETCH_ISnapMapper) {
    snapmapper_ = LINK_FETCH_ISnapMapper(nullptr);
  } else {
    DLOGE("Failed to get snapalloc instance");
  }
}

SDMLayer::~SDMLayer() {
  // Close any fences left for this layer
  release_fence_ = nullptr;
  if (layer_) {
    if (buffer_fd_ >= 0) {
      ::close(buffer_fd_);
    }
    delete layer_;
  }
}

DisplayError SDMLayer::SetLayerBuffer(const SnapHandle *handle,
                                      shared_ptr<Fence> acquire_fence) {
  if (!handle) {
    if (client_requested_ == SDMCompositionType::COMP_DEVICE ||
        client_requested_ == SDMCompositionType::COMP_CURSOR) {
      DLOGW("Invalid buffer handle: %p on layer: %d client requested comp type "
            "%d",
            &handle, UINT32(id_), client_requested_);
      return kErrorParameters;
    } else {
      return kErrorNone;
    }
  }

  int fd;
  snapmapper_->GetMetadata(*handle, MetadataType::FD, &fd);

  if (fd < 0) {
    return kErrorParameters;
  }

  LayerBuffer *layer_buffer = &layer_->input_buffer;
  int32_t aligned_width, aligned_height;
  // Get custom width and height
  snapmapper_->GetMetadata(*handle, MetadataType::CUSTOM_DIMENSIONS_STRIDE, &aligned_width);
  snapmapper_->GetMetadata(*handle, MetadataType::CUSTOM_DIMENSIONS_HEIGHT, &aligned_height);

  int fmt, flag = 0;
  snapmapper_->GetMetadata(*handle, MetadataType::PIXEL_FORMAT_ALLOCATED, &fmt);
  int64_t is_ubwc = 0;
  snapmapper_->GetMetadata(*handle, MetadataType::IS_UBWC, &is_ubwc);
  flag = is_ubwc ? INT32(MetadataType::IS_UBWC) : 0;

  int64_t compression_type = 0;
  snapmapper_->GetMetadata(*handle, MetadataType::COMPRESSION, &compression_type);

  LayerBufferFormat format = buffer_allocator_->GetSDMFormat(fmt, flag, compression_type);
  if ((format != layer_buffer->format) || (UINT32(aligned_width) != layer_buffer->width) ||
      (UINT32(aligned_height) != layer_buffer->height)) {
    // Layer buffer geometry has changed.
    geometry_changes_ |= kBufferGeometry;
  }

  layer_buffer->format = format;
  layer_buffer->width = UINT32(aligned_width);
  layer_buffer->height = UINT32(aligned_height);

  uint64_t width_temp, height_temp = 0;
  auto err_w =
      GetMetadata(handle, MetadataType::WIDTH, &width_temp, snapmapper_);
  if (err_w != Error::NONE) {
    DLOGE("Failed to retrieve unaligned width: %d", INT32(err_w));
  }
  auto err_h =
      GetMetadata(handle, MetadataType::HEIGHT, &height_temp, snapmapper_);
  if (err_h != Error::NONE) {
    DLOGE("Failed to retrieve unaligned height");
  }
  layer_buffer->unaligned_width = UINT32(width_temp);
  layer_buffer->unaligned_height = UINT32(height_temp);
  uint32_t buffer_type = 0;
  snapmapper_->GetMetadata(*handle, MetadataType::BUFFER_TYPE, &buffer_type);

  layer_buffer->flags.video = (buffer_type == 1) ? true : false;
  if (SetMetaData(handle, layer_) != kErrorNone) {
    return kErrorParameters;
  }

  // TZ Protected Buffer - L1
  BufferUsage handle_flags;
  snapmapper_->GetMetadata(*handle, MetadataType::USAGE, &handle_flags);

  secure_ = (handle_flags & BufferUsage::PROTECTED);
  bool secure_camera = secure_ && (handle_flags & BufferUsage::CAMERA_OUTPUT);
  bool secure_display = (handle_flags & BufferUsage::QTI_PRIVATE_SECURE_DISPLAY);
  if (secure_ != layer_buffer->flags.secure ||
      secure_camera != layer_buffer->flags.secure_camera ||
      secure_display != layer_buffer->flags.secure_display) {
    // Secure attribute of layer buffer has changed.
    layer_->update_mask.set(kSecurity);
  }
  layer_buffer->flags.secure = secure_;
  layer_buffer->flags.secure_camera = secure_camera;
  layer_buffer->flags.secure_display = secure_display;
#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
  layer_buffer->flags.fingerprint_indisplay_layer =
        (handle_flags & BufferUsage::QTI_PRIVATE_FINGERPRINT_MASK_BUFFER);
#endif

  layer_buffer->acquire_fence = acquire_fence;

  int buffer_fd = buffer_fd_;
  buffer_fd_ = ::dup(fd);
  if (buffer_fd >= 0) {
    ::close(buffer_fd);
  }

  layer_buffer->planes[0].fd = buffer_fd_;
  layer_buffer->planes[0].offset = 0;
  auto err = GetMetadata(handle, MetadataType::ALIGNED_WIDTH_IN_PIXELS,
                         &layer_buffer->planes[0].stride, snapmapper_);
  if (err != Error::NONE) {
    DLOGW("Failed to retrieve aligned width");
  }

  err = GetMetadata(handle, MetadataType::ALLOCATION_SIZE, &layer_buffer->size,
                    snapmapper_);

  if (err != Error::NONE) {
    DLOGW("Failed to retrieve allocation size");
  }
  buffer_flipped_ = reinterpret_cast<uint64_t>(handle) != layer_buffer->buffer_id;
  layer_buffer->buffer_id = reinterpret_cast<uint64_t>(handle);

  err = GetMetadata(handle, MetadataType::BUFFER_ID, &layer_buffer->handle_id,
                    snapmapper_);

  if (err != Error::NONE) {
    DLOGW("Failed to retrieve buffer id");
  }
  err = GetMetadata(handle, MetadataType::USAGE, &layer_buffer->usage,
                    snapmapper_);
  if (err != Error::NONE) {
    DLOGW("Failed to retrieve handle usage");
  }
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerSurfaceDamage(SDMRegion damage) {
  surface_updated_ = true;
  if ((damage.num_rects == 1) && (damage.rects[0].bottom == 0) &&
      (damage.rects[0].right == 0)) {
    surface_updated_ = false;
  }

#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
  //P200201-02558 : Fingerprint layer should be updated always
  LayerBuffer *layer_buffer = &layer_->input_buffer;
  if(layer_buffer->flags.fingerprint_indisplay_layer) {
    surface_updated_ = true;
  }
#endif

  if (!layer_->flags.updating && surface_updated_) {
    layer_->update_mask.set(kSurfaceInvalidate);
  }

  // Check if there is an update in SurfaceDamage rects.
  if (layer_->dirty_regions.size() != damage.num_rects) {
    layer_->update_mask.set(kSurfaceInvalidate);
  } else {
    for (uint32_t j = 0; j < damage.num_rects; j++) {
      LayerRect damage_rect;
      SetRect(damage.rects[j], &damage_rect);
      if (damage_rect != layer_->dirty_regions.at(j)) {
        layer_->update_mask.set(kSurfaceDamage);
        break;
      }
    }
  }

  SetDirtyRegions(damage);
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerBlendMode(LayerBlending blending) {
  if (layer_->blending != blending) {
    geometry_changes_ |= kBlendMode;
    layer_->blending = blending;
  }
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerColor(SDMColor color) {
  if (client_requested_ != SDMCompositionType::COMP_SOLID_COLOR) {
    return kErrorNone;
  }
  if (layer_->solid_fill_color != GetUint32Color(color)) {
    layer_->solid_fill_color = GetUint32Color(color);
    layer_->update_mask.set(kSurfaceInvalidate);
    surface_updated_ = true;
  } else {
    surface_updated_ = false;
  }

  layer_->input_buffer.format = kFormatARGB8888;
  DLOGV_IF(kTagClient, "[%" PRIu64 "][%" PRIu64 "] Layer color set to %x",
           display_id_, id_, layer_->solid_fill_color);
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerCompositionType(SDMCompositionType type) {
  // Validation is required when the client changes the composition type
  if ((type != client_requested_) || (type != device_selected_) ||
      (type == SDMCompositionType::COMP_CLIENT)) {
    layer_->update_mask.set(kClientCompRequest);
  }
  client_requested_ = type;
  client_requested_orig_ = type;
  switch (type) {
  case SDMCompositionType::COMP_CLIENT:
    break;
  case SDMCompositionType::COMP_DEVICE:
    // We try and default to this in SDM
    break;
  case SDMCompositionType::COMP_SOLID_COLOR:
    break;
  case SDMCompositionType::COMP_CURSOR:
    break;
  case SDMCompositionType::COMP_DISPLAY_DECORATION:
    break;
  case SDMCompositionType::COMP_INVALID:
    return kErrorParameters;
  default:
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerDataspace(int32_t dataspace) {
     // Map deprecated dataspace values to appropriate new enums
  dataspace = buffer_allocator_->TranslateFromLegacyDataspace(dataspace);

  // cache the dataspace, to be used later to update SDM ColorMetaData
  if (dataspace_ != dataspace) {
    geometry_changes_ |= kDataspace;
    dataspace_ = dataspace;
    if (layer_->input_buffer.buffer_id) {
      ValidateAndSetCSC((SnapHandle *) layer_->input_buffer.buffer_id);
    }
  }
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerDisplayFrame(SDMRect frame) {
  LayerRect dst_rect = {};

  SetRect(frame, &dst_rect);
  if (dst_rect_ != dst_rect) {
    geometry_changes_ |= kDisplayFrame;
    dst_rect_ = dst_rect;
  }

  return kErrorNone;
}

void SDMLayer::ResetPerFrameData() {
  layer_->dst_rect = dst_rect_;
  layer_->transform = layer_transform_;
}

DisplayError SDMLayer::SetCursorPosition(int32_t x, int32_t y) {
  SDMRect frame = {};
  frame.left = x;
  frame.top = y;
  frame.right = x + INT(layer_->dst_rect.right - layer_->dst_rect.left);
  frame.bottom = y + INT(layer_->dst_rect.bottom - layer_->dst_rect.top);
  SetLayerDisplayFrame(frame);

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerPlaneAlpha(float alpha) {
  if (alpha < 0.0f || alpha > 1.0f) {
    return kErrorParameters;
  }

  //  Conversion of float alpha in range 0.0 to 1.0 similar to the SDM Adapter
  uint16_t plane_alpha = static_cast<uint16_t>(std::round(65535.0f * alpha));

  if (layer_->plane_alpha != plane_alpha) {
    geometry_changes_ |= kPlaneAlpha;
    layer_->plane_alpha = plane_alpha;
  }

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerSourceCrop(SDMRect crop) {
  LayerRect src_rect = {};
  SetRect(crop, &src_rect);
  non_integral_source_crop_ =
      ((crop.left != roundf(crop.left)) || (crop.top != roundf(crop.top)) ||
       (crop.right != roundf(crop.right)) ||
       (crop.bottom != roundf(crop.bottom)));
  if (non_integral_source_crop_) {
    DLOGV_IF(kTagClient, "Crop: LTRB %f %f %f %f", crop.left, crop.top,
             crop.right, crop.bottom);
  }
  if (layer_->src_rect != src_rect) {
    geometry_changes_ |= kSourceCrop;
    layer_->src_rect = src_rect;
  }

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerTransform(SDMTransform transform) {
  LayerTransform layer_transform = {};
  switch (static_cast<int32_t>(transform)) {
  case static_cast<int32_t>(SDMTransform::TRANSFORM_FLIP_H):
    layer_transform.flip_horizontal = true;
    break;
  case static_cast<int32_t>(SDMTransform::TRANSFORM_FLIP_V):
    layer_transform.flip_vertical = true;
    break;
  case static_cast<int32_t>(SDMTransform::TRANSFORM_ROT_90):
    layer_transform.rotation = 90.0f;
    break;
  case static_cast<int32_t>(SDMTransform::TRANSFORM_ROT_180):
    layer_transform.flip_horizontal = true;
    layer_transform.flip_vertical = true;
    break;
  case static_cast<int32_t>(SDMTransform::TRANSFORM_ROT_270):
    layer_transform.rotation = 90.0f;
    layer_transform.flip_horizontal = true;
    layer_transform.flip_vertical = true;
    break;
  // Legacy SDM2 case, remove if not in use
  case (static_cast<int32_t>(SDMTransform::TRANSFORM_FLIP_H) |
        static_cast<int32_t>(SDMTransform::TRANSFORM_ROT_90)):
    layer_transform.rotation = 90.0f;
    layer_transform.flip_horizontal = true;
    break;
  // Legacy SDM2 case, remove if not in use
  case (static_cast<int32_t>(SDMTransform::TRANSFORM_FLIP_V) |
        static_cast<int32_t>(SDMTransform::TRANSFORM_ROT_90)):
    layer_transform.rotation = 90.0f;
    layer_transform.flip_vertical = true;
    break;
  case static_cast<int32_t>(SDMTransform::TRANSFORM_NONE):
    break;
  default:
    //  bad transform
    return kErrorParameters;
  }

  if (layer_transform_ != layer_transform) {
    geometry_changes_ |= kTransform;
    layer_transform_ = layer_transform;
  }

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerVisibleRegion(SDMRegion visible) {
  layer_->visible_regions.clear();
  for (uint32_t i = 0; i < visible.num_rects; i++) {
    LayerRect rect;
    SetRect(visible.rects[i], &rect);
    layer_->visible_regions.push_back(rect);
  }

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerZOrder(uint32_t z) {
  if (z_ != z) {
    geometry_changes_ |= kZOrder;
    z_ = z;
  }

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerType(SDMLayerTypes type) {
  type_ = type;

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerFlag(SDMLayerFlag flag) {
  compatible_ = (flag == SDMLayerFlag::LAYER_FLAG_COMPATIBLE);

  return kErrorNone;
}

DisplayError SDMLayer::SetLayerColorTransform(const float *matrix) {
  if (std::memcmp(matrix, layer_->color_transform_matrix,
                  sizeof(layer_->color_transform_matrix))) {
    std::memcpy(layer_->color_transform_matrix, matrix,
                sizeof(layer_->color_transform_matrix));
    layer_->update_mask.set(kColorTransformUpdate);
    color_transform_matrix_set_ = true;
    if (!std::memcmp(matrix, kIdentityMatrix, sizeof(kIdentityMatrix))) {
      color_transform_matrix_set_ = false;
    }
  }
  return kErrorNone;
}

DisplayError
SDMLayer::SetLayerPerFrameMetadata(uint32_t num_elements,
                                   const SDMPerFrameMetadataKey *keys,
                                   const float *metadata) {
  auto old_mastering_display = layer_->input_buffer.masteringDisplayInfo;
  auto old_content_light = layer_->input_buffer.contentLightLevel;
  auto &mastering_display = layer_->input_buffer.masteringDisplayInfo;
  auto &content_light = layer_->input_buffer.contentLightLevel;
  for (uint32_t i = 0; i < num_elements; i++) {
    switch (keys[i]) {
    case SDMPerFrameMetadataKey::DISPLAY_RED_PRIMARY_X:
      mastering_display.colorVolumeSEIEnabled = true;
      mastering_display.primaryRed.x = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y:
      mastering_display.primaryRed.y = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X:
      mastering_display.primaryGreen.x = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y:
      mastering_display.primaryGreen.y = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X:
      mastering_display.primaryBlue.x = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y:
      mastering_display.primaryBlue.y = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::WHITE_POINT_X:
      mastering_display.whitePoint.x = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::WHITE_POINT_Y:
      mastering_display.whitePoint.y = UINT32(metadata[i] * 50000);
      break;
    case SDMPerFrameMetadataKey::MAX_LUMINANCE:
      mastering_display.maxDisplayLuminance = UINT32(metadata[i]);
      break;
    case SDMPerFrameMetadataKey::MIN_LUMINANCE:
      mastering_display.minDisplayLuminance = UINT32(metadata[i] * 10000);
      break;
    case SDMPerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL:
      content_light.lightLevelSEIEnabled = true;
      content_light.maxContentLightLevel = UINT32(metadata[i]);
      break;
    case SDMPerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL:
      content_light.maxFrameAverageLightLevel = UINT32(metadata[i]);
      break;
    default:
      break;
    }
  }
  if ((!SameConfig(&old_mastering_display, &mastering_display,
                   UINT32(sizeof(QtiMasteringDisplay)))) ||
      (!SameConfig(&old_content_light, &content_light,
                   UINT32(sizeof(QtiContentLightLevel))))) {
    layer_->update_mask.set(kContentMetadata);
    geometry_changes_ |= kDataspace;
  }
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerPerFrameMetadataBlobs(
    uint32_t num_elements, const SDMPerFrameMetadataKey *keys,
    const uint32_t *sizes, const uint8_t *metadata) {
  if (!keys || !sizes || !metadata) {
    DLOGW("metadata or sizes or keys is null");
    // According to Google, it is expected for the layer metadata in certain
    // scenarios. When this happens, simply return Error::None. More info on the
    // Google bug (b/275697888)
    return kErrorNone;
  }

  QtiDynamicMetadata &dyn_metadata = layer_->input_buffer.dynamicMetadata;
  for (uint32_t i = 0; i < num_elements; i++) {
    switch (keys[i]) {
    case SDMPerFrameMetadataKey::HDR10_PLUS_SEI:
      if (sizes[i] > QTI_HDR_DYNAMIC_META_DATA_SZ) {
        DLOGE("Size of HDR10_PLUS_SEI = %d", sizes[i]);
        return kErrorParameters;
      }
      // if dynamic metadata changes, store and set needs validate
      if (!SameConfig(
              static_cast<const uint8_t *>(dyn_metadata.dynamicMetaDataPayload),
              metadata, sizes[i])) {
        geometry_changes_ |= kDataspace;
        dyn_metadata.dynamicMetaDataValid = true;
        dyn_metadata.dynamicMetaDataLen = sizes[i];
        std::memcpy(dyn_metadata.dynamicMetaDataPayload, metadata, sizes[i]);
        layer_->update_mask.set(kContentMetadata);
      }
      break;
    default:
      DLOGW("Invalid key = %d", keys[i]);
      return kErrorParameters;
    }
  }
  return kErrorNone;
}

DisplayError SDMLayer::SetLayerBrightness(float brightness) {
  if (std::isnan(brightness) || brightness < 0.0f || brightness > 1.0f) {
    DLOGE("Invalid brightness = %f", brightness);
    return kErrorParameters;
  }

  // When SDR dimming is disabled, layer brightness needs to be reset for device
  // composition
  if (brightness != 1.0f && IsSdrDimmingDisabled() &&
      client_requested_ == SDMCompositionType::COMP_DEVICE) {
    brightness = 1.0f;
  }

  if (layer_->layer_brightness != brightness) {
    DLOGV_IF(kTagClient, "Update layer brightness from %f to %f",
             layer_->layer_brightness, brightness);
    layer_->layer_brightness = brightness;
    geometry_changes_ |= kLayerBrightness;
  }

  return kErrorNone;
}

void SDMLayer::SetRect(const SDMRect &source, LayerRect *target, bool round) {
  if (!round) {
    target->left = FLOAT(source.left);
    target->top = FLOAT(source.top);
    target->right = FLOAT(source.right);
    target->bottom = FLOAT(source.bottom);
  } else {
    target->left = std::ceil(source.left);
    target->top = std::ceil(source.top);
    target->right = std::floor(source.right);
    target->bottom = std::floor(source.bottom);
  }
}

uint32_t SDMLayer::GetUint32Color(const SDMColor &source) {
  // Returns 32 bit ARGB
  uint32_t a = UINT32(source.a) << 24;
  uint32_t r = UINT32(source.r) << 16;
  uint32_t g = UINT32(source.g) << 8;
  uint32_t b = UINT32(source.b);
  uint32_t color = a | r | g | b;
  return color;
}

void SDMLayer::GetUBWCStatsFromMetaData(UBWCStats *cr_stats, UbwcCrStatsVector *cr_vec) {
  // TODO(user): Check if we can use UBWCStats directly
  // in layer_buffer or copy directly to Vector
  if (cr_stats->bDataValid) {
    switch (cr_stats->version) {
      case UBWCVersion::UBWC_VERSION_5_0:
      case UBWCVersion::UBWC_VERSION_4_0:
      case UBWCVersion::UBWC_VERSION_3_0:
      case UBWCVersion::UBWC_VERSION_2_0:
        cr_vec->push_back(std::make_pair(32, cr_stats->ubwc_stats.nCRStatsTile32));
        cr_vec->push_back(std::make_pair(64, cr_stats->ubwc_stats.nCRStatsTile64));
        cr_vec->push_back(std::make_pair(96, cr_stats->ubwc_stats.nCRStatsTile96));
        cr_vec->push_back(std::make_pair(128, cr_stats->ubwc_stats.nCRStatsTile128));
        cr_vec->push_back(std::make_pair(160, cr_stats->ubwc_stats.nCRStatsTile160));
        cr_vec->push_back(std::make_pair(192, cr_stats->ubwc_stats.nCRStatsTile192));
        cr_vec->push_back(std::make_pair(256, cr_stats->ubwc_stats.nCRStatsTile256));
        break;
      default:
        DLOGW("Invalid UBWC Version %d", cr_stats->version);
        break;
    } // switch(cr_stats->version)
  }   // if (cr_stats->bDatvalid)
}

DisplayError SDMLayer::SetMetaData(const SnapHandle *handle, Layer *layer) {
  LayerBuffer *layer_buffer = &layer->input_buffer;

  std::string name = "";
  snapmapper_->GetMetadata(*handle, MetadataType::NAME, &name);
  name_ = name;

  float fps = 0;
  uint32_t frame_rate = layer->frame_rate;
  if (GetMetadata(handle, MetadataType::REFRESH_RATE, &fps, snapmapper_) ==
      Error::NONE) {
    frame_rate = (fps != 0) ? RoundToStandardFPS(fps) : layer->frame_rate;
    has_metadata_refresh_rate_ = true;
  }

  int32_t interlaced = 0;
  snapmapper_->GetMetadata(*handle, MetadataType::PP_PARAM_INTERLACED, &interlaced);
  bool interlace = interlaced ? true : false;

  if (interlace != layer_buffer->flags.interlace) {
    DLOGI("Layer buffer interlaced metadata has changed. old=%d, new=%d",
          layer_buffer->flags.interlace, interlace);
  }

  uint32_t linear_format = 0;
  if (GetMetadata(handle, MetadataType::LINEAR_FORMAT, &linear_format,
                  snapmapper_) == Error::NONE) {
    layer_buffer->format =
        buffer_allocator_->GetSDMFormat(INT32(linear_format), 0, 0);
  }

  if ((interlace != layer_buffer->flags.interlace) ||
      (frame_rate != layer->frame_rate)) {
    // Layer buffer metadata has changed.
    layer->frame_rate = frame_rate;
    layer_buffer->flags.interlace = interlace;
    layer_->update_mask.set(kMetadataUpdate);
  }

  // Check if metadata is set
  UBWCStats cr_stats[NUM_UBWC_CR_STATS_LAYERS] = {};

  for (int i = 0; i < NUM_UBWC_CR_STATS_LAYERS; i++) {
    layer_buffer->ubwc_crstats[i].clear();
  }

  if (GetMetadata(handle, MetadataType::UBWC_CR_STATS_INFO, cr_stats,
                  snapmapper_) == Error::NONE) {
    // Only copy top layer for now as only top field for interlaced is used
    GetUBWCStatsFromMetaData(&cr_stats[0], &(layer_buffer->ubwc_crstats[0]));
  }

  uint32_t single_buffer = 0;
  snapmapper_->GetMetadata(*handle, MetadataType::SINGLE_BUFFER_MODE, &single_buffer);
  single_buffer_ = (single_buffer == 1);

  // Handle colorMetaData / Dataspace handling now
  ValidateAndSetCSC(handle);

  bool extended_md_set;
  snapmapper_->GetMetadataState(*handle, MetadataType::CUSTOM_CONTENT_METADATA, &extended_md_set);
  if (extended_md_set) {
    std::shared_ptr<CustomContentMetadata> dv_md = std::make_shared<CustomContentMetadata>();
    auto err =
        snapmapper_->GetMetadata(*handle, MetadataType::CUSTOM_CONTENT_METADATA, dv_md.get());

    if (!err) {
      if (!layer_buffer->extended_content_metadata ||
          dv_md->size != layer_buffer->extended_content_metadata->size ||
          !SameConfig(layer_buffer->extended_content_metadata->metadataPayload,
                      dv_md->metadataPayload, dv_md->size)) {
        layer_buffer->extended_content_metadata = dv_md;
        layer_->update_mask.set(kContentMetadata);
      }
    }
  } else if (layer_buffer->extended_content_metadata) {
    // Buffer switch scenario - cleanup old metadata
    layer_buffer->extended_content_metadata = nullptr;
    layer_->update_mask.set(kContentMetadata);
  }

  bool anamorphic_compression_md_set = false;
  auto err = snapmapper_->GetMetadataState(*handle, MetadataType::ANAMORPHIC_COMPRESSION_METADATA,
                                           &anamorphic_compression_md_set);
  if (anamorphic_compression_md_set) {
    err = snapmapper_->GetMetadata(*handle, MetadataType::ANAMORPHIC_COMPRESSION_METADATA,
                                   &layer_buffer->anamorphicMetadata);
    if (err) {
      DLOGW("Failed to get anamorphic compression metadata");
    }
  }

  if (!ignore_sdr_histogram_md_ || IsHdr(layer_buffer->dataspace.colorPrimaries,
                                         layer_buffer->dataspace.transfer)) {
    VideoHistogramMetadata histogram = {};
    if (layer_->update_mask.test(kContentMetadata) == false &&
        GetMetadata(handle, MetadataType::VIDEO_HISTOGRAM_STATS, &histogram,
                    snapmapper_) == Error::NONE) {
      uint32_t bins = histogram.stat_len / sizeof(histogram.stats_info[0]);
      layer_buffer->hist_data.display_width = layer_buffer->unaligned_width;
      layer_buffer->hist_data.display_height = layer_buffer->unaligned_height;
      if (histogram.stat_len <= sizeof(histogram.stats_info) && bins > 0) {
        layer_buffer->hist_data.stats_info.clear();
        layer_buffer->hist_data.stats_info.reserve(bins);
        for (uint32_t i = 0; i < bins; i++) {
          layer_buffer->hist_data.stats_info.push_back(histogram.stats_info[i]);
        }

        layer_buffer->hist_data.stats_valid = true;
        layer_->update_mask.set(kContentMetadata);
      }
    }
  }

  layer_buffer->timestamp_data.valid = false;

  bool timestamp_set = false;
  snapmapper_->GetMetadataState(*handle, MetadataType::VIDEO_TS_INFO, &timestamp_set);
  if (timestamp_set) {
    VideoTimestampInfo timestamp_info = {};
    int err = static_cast<int>(snapmapper_->GetMetadata(*handle, MetadataType::VIDEO_TS_INFO, &timestamp_info));

    if (!err && timestamp_info.enable) {
      layer_buffer->timestamp_data.valid = true;
      layer_buffer->timestamp_data.frame_number = timestamp_info.frame_number;
      layer_buffer->timestamp_data.frame_timestamp_us =
          timestamp_info.frame_timestamp_us;
    }
  }

  return kErrorNone;
}

bool SDMLayer::IsDataSpaceSupported() {
  if (client_requested_ != SDMCompositionType::COMP_DEVICE &&
      client_requested_ != SDMCompositionType::COMP_CURSOR) {
    // Layers marked for GPU can have any dataspace
    return true;
  }

  return dataspace_supported_;
}

// helper for copying metadata from layer buffer
void CopyMetadataFromBuffer(ColorMetadata *new_metadata, LayerBuffer *buffer) {
  new_metadata->dataspace = buffer->dataspace;
  new_metadata->matrixCoefficients = buffer->matrixCoefficients;
  new_metadata->masteringDisplayInfo = buffer->masteringDisplayInfo;
  new_metadata->contentLightLevel = buffer->contentLightLevel;
  new_metadata->cRI = buffer->cRI;
  new_metadata->dynamicMetadata = buffer->dynamicMetadata;
}

void SDMLayer::ValidateAndSetCSC(const SnapHandle *handle) {
  LayerBuffer *layer_buffer = &layer_->input_buffer;
  bool use_color_metadata = true;
  Dataspace csc;
  if (dataspace_ != 0) {
    use_color_metadata = false;
    bool valid_csc = buffer_allocator_->GetSDMColorSpace(dataspace_, &csc);

    if (!valid_csc) {
      dataspace_supported_ = false;
      return;
    }

    if (layer_buffer->dataspace.transfer != csc.transfer ||
        layer_buffer->dataspace.colorPrimaries != csc.colorPrimaries ||
        layer_buffer->dataspace.range != csc.range) {
      // ColorMetadata updated. Needs validate.
      layer_->update_mask.set(kMetadataUpdate);
      // if we are here here, update the sdm layer csc.
      layer_buffer->dataspace.transfer = csc.transfer;
      layer_buffer->dataspace.colorPrimaries = csc.colorPrimaries;
      layer_buffer->dataspace.range = csc.range;
    }
  }

  if (IsBT2020(layer_buffer->dataspace.colorPrimaries)) {
    // android_dataspace_t doesnt support mastering display and light levels
    // so retrieve it from metadata for BT2020(HDR)
    use_color_metadata = true;
  }

  if (use_color_metadata) {
    ColorMetadata new_metadata;
    CopyMetadataFromBuffer(&new_metadata, layer_buffer);
    if (sdm::SetCSC(handle, &new_metadata, snapmapper_) == Error::NONE) {
      // If dataspace is KNOWN, overwrite the snapalloc metadata CSC using the
      // previously derived CSC from dataspace.
      if (dataspace_ != 0) {
        new_metadata.dataspace.colorPrimaries =
            layer_buffer->dataspace.colorPrimaries;
        new_metadata.dataspace.transfer = layer_buffer->dataspace.transfer;
        new_metadata.dataspace.range = layer_buffer->dataspace.range;
      }
      if ((layer_buffer->dataspace.colorPrimaries !=
           new_metadata.dataspace.colorPrimaries) ||
          (layer_buffer->dataspace.transfer !=
           new_metadata.dataspace.transfer) ||
          (layer_buffer->dataspace.range != new_metadata.dataspace.range)) {
        layer_buffer->dataspace.colorPrimaries =
            new_metadata.dataspace.colorPrimaries;
        layer_buffer->dataspace.transfer = new_metadata.dataspace.transfer;
        layer_buffer->dataspace.range = new_metadata.dataspace.range;
        layer_->update_mask.set(kMetadataUpdate);
      }
      if (layer_buffer->matrixCoefficients != new_metadata.matrixCoefficients) {
        layer_buffer->matrixCoefficients = new_metadata.matrixCoefficients;
        layer_->update_mask.set(kMetadataUpdate);
      }
      DLOGV_IF(
          kTagClient,
          "Layer id = %d ColorVolEnabled = %d ContentLightLevelEnabled = %d "
          "cRIEnabled = %d Dynamic Metadata valid = %d size = %d",
          UINT32(id_), new_metadata.masteringDisplayInfo.colorVolumeSEIEnabled,
          new_metadata.contentLightLevel.lightLevelSEIEnabled,
          new_metadata.cRI.criEnabled,
          new_metadata.dynamicMetadata.dynamicMetaDataValid,
          new_metadata.dynamicMetadata.dynamicMetaDataLen);
      // Read color metadata from snapalloc handle if it's enabled by clients,
      // this will override the values set using the Composer
      // API's(SetLayerPerFrameMetaData)
      if (new_metadata.masteringDisplayInfo.colorVolumeSEIEnabled &&
          !SameConfig(&new_metadata.masteringDisplayInfo,
                      &layer_buffer->masteringDisplayInfo,
                      UINT32(sizeof(QtiMasteringDisplay)))) {
        layer_buffer->masteringDisplayInfo = new_metadata.masteringDisplayInfo;
        layer_->update_mask.set(kContentMetadata);
      }
      if (new_metadata.contentLightLevel.lightLevelSEIEnabled &&
          !SameConfig(&new_metadata.contentLightLevel,
                      &layer_buffer->contentLightLevel,
                      UINT32(sizeof(QtiContentLightLevel)))) {
        layer_buffer->contentLightLevel = new_metadata.contentLightLevel;
        layer_->update_mask.set(kContentMetadata);
      }
      if (new_metadata.cRI.criEnabled &&
          !SameConfig(&new_metadata.cRI, &layer_buffer->cRI,
                      UINT32(sizeof(QtiColorRemappingInfo)))) {
        layer_buffer->cRI = new_metadata.cRI;
        layer_->update_mask.set(kMetadataUpdate);
      }
      if (new_metadata.dynamicMetadata.dynamicMetaDataValid &&
          ((new_metadata.dynamicMetadata.dynamicMetaDataLen !=
            layer_buffer->dynamicMetadata.dynamicMetaDataLen) ||
           !SameConfig(layer_buffer->dynamicMetadata.dynamicMetaDataPayload,
                       new_metadata.dynamicMetadata.dynamicMetaDataPayload,
                       new_metadata.dynamicMetadata.dynamicMetaDataLen))) {
        layer_buffer->dynamicMetadata.dynamicMetaDataValid = true;
        layer_buffer->dynamicMetadata.dynamicMetaDataLen =
            new_metadata.dynamicMetadata.dynamicMetaDataLen;
        std::memcpy(layer_buffer->dynamicMetadata.dynamicMetaDataPayload,
                    new_metadata.dynamicMetadata.dynamicMetaDataPayload,
                    new_metadata.dynamicMetadata.dynamicMetaDataLen);
        layer_->update_mask.set(kContentMetadata);
      }
    } else {
      dataspace_supported_ = false;
      return;
    }
  }

  dataspace_supported_ = true;
}

uint32_t SDMLayer::RoundToStandardFPS(float fps) {
  static const int32_t standard_fps[4] = {24, 30, 48, 60};
  int32_t frame_rate = (uint32_t)(fps);

  int count = INT(sizeof(standard_fps) / sizeof(standard_fps[0]));
  for (int i = 0; i < count; i++) {
    if ((standard_fps[i] - frame_rate) < 2) {
      // Most likely used for video, the fps can fluctuate
      // Ex: b/w 29 and 30 for 30 fps clip
      return standard_fps[i];
    }
  }

  return frame_rate;
}

void SDMLayer::SetComposition(const LayerComposition &composition) {
  auto sdm_composition = SDMCompositionType::COMP_INVALID;
  switch (composition) {
  case kCompositionGPU:
    sdm_composition = SDMCompositionType::COMP_CLIENT;
    break;
  case kCompositionCursor:
    sdm_composition = SDMCompositionType::COMP_CURSOR;
    break;
  default:
    sdm_composition = SDMCompositionType::COMP_DEVICE;
    break;
  }
  // Update solid fill composition
  if (composition == kCompositionSDE && layer_->flags.solid_fill != 0) {
    sdm_composition = SDMCompositionType::COMP_SOLID_COLOR;
  }
  // Update Display Decoration composition only for A8 mask layer i.e when
  // requested composition is DISPLAY_DECORATION
  SDMCompositionType requested_composition =
      GetClientRequestedCompositionType();
  if ((composition == kCompositionSDE &&
       layer_->input_buffer.flags.mask_layer != 0) &&
      (requested_composition == SDMCompositionType::COMP_DISPLAY_DECORATION)) {
    sdm_composition = SDMCompositionType::COMP_DISPLAY_DECORATION;
  }
  device_selected_ = sdm_composition;

  return;
}

shared_ptr<Fence> SDMLayer::GetReleaseFence() { return release_fence_; }

void SDMLayer::SetReleaseFence(const shared_ptr<Fence> &release_fence) {
  release_fence_ = release_fence;
}

bool SDMLayer::IsRotationPresent() {
  return ((layer_->transform.rotation != 0.0f) ||
          layer_->transform.flip_horizontal || layer_->transform.flip_vertical);
}

bool SDMLayer::IsScalingPresent() {
  uint32_t src_width =
      static_cast<uint32_t>(layer_->src_rect.right - layer_->src_rect.left);
  uint32_t src_height =
      static_cast<uint32_t>(layer_->src_rect.bottom - layer_->src_rect.top);
  uint32_t dst_width =
      static_cast<uint32_t>(layer_->dst_rect.right - layer_->dst_rect.left);
  uint32_t dst_height =
      static_cast<uint32_t>(layer_->dst_rect.bottom - layer_->dst_rect.top);

  if ((layer_->transform.rotation == 90.0) ||
      (layer_->transform.rotation == 270.0)) {
    std::swap(src_width, src_height);
  }

  return ((src_width != dst_width) || (dst_height != src_height));
}

void SDMLayer::SetDirtyRegions(const SDMRegion &surface_damage) {
  layer_->dirty_regions.clear();
  for (uint32_t i = 0; i < surface_damage.num_rects; i++) {
    LayerRect rect;
    SetRect(surface_damage.rects[i], &rect);
    layer_->dirty_regions.push_back(rect);
  }
}

void SDMLayer::SetLayerAsMask() {
  layer_->input_buffer.flags.mask_layer = true;
  DLOGV_IF(kTagClient,
           " Layer Id: "
           "[%" PRIu64 "]",
           id_);
}

void SDMLayer::ResetGeometryChanges() {
  geometry_changes_ = GeometryChanges::kNone;
  layer_->geometry_changes = GeometryChanges::kNone;
}

} // namespace sdm
