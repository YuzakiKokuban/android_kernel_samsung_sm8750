/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "sdm_layer_builder.h"
#include <utils/debug.h>

#define __CLASS__ "SDMLayerBuilder"

namespace sdm {

Locker SDMLayerBuilder::locker_[kNumDisplays];

// final cleanup if any displays were lost
SDMLayerBuilder::~SDMLayerBuilder() {
  for (auto i : display_layer_stack_) {
    DeInit(i.first);
  }
}

DisplayError SDMLayerBuilder::Init(BufferAllocator *buffer_allocator,
                                   uint64_t display_id) {
  SCOPE_LOCK(locker_[display_id]);
  buffer_allocator_ = buffer_allocator;

  Debug::Get()->GetProperty(DISABLE_SDR_HISTOGRAM, &disable_sdr_histogram_);
  if (disable_sdr_histogram_) {
    DLOGI("Non-HDR histogram handling disabled");
  }

  Debug::Get()->GetProperty(DISABLE_MASK_LAYER_HINT, &disable_mask_layer_hint_);
  DLOGI("disable_mask_layer_hint_: %d", disable_mask_layer_hint_);

  // initialize layer stack
  display_layer_stack_[display_id];

  return kErrorNone;
}

DisplayError SDMLayerBuilder::DeInit(uint64_t display_id) {
  SCOPE_LOCK(locker_[display_id]);
  auto display = display_layer_stack_.find(display_id);
  if (display == display_layer_stack_.end()) {
    DLOGW("Display: %" PRIu64 " not found", display_id);
    return kErrorNotSupported;
  }

  auto &layer_stack = display->second;
  auto &layer_set = layer_stack.layer_set_;

  for (auto sdm_layer : layer_set) {
    delete sdm_layer;
  }
  display_layer_stack_.erase(display);

  return kErrorNone;
}

LayerBufferFormat SDMLayerBuilder::GetSDMFormat(const int32_t &source, const int32_t flags,
                                                const int64_t compression_type) {
  return buffer_allocator_->GetSDMFormat(source, flags, compression_type);
}

SDMLayer *SDMLayerBuilder::GetSDMLayer(uint64_t display_id, int64_t layer_id) {
  auto stack = display_layer_stack_.find(display_id);
  if (stack == display_layer_stack_.end()) {
    DLOGW("Display: %" PRIu64 " not found", display_id);
    return nullptr;
  }

  auto &layer_stack = stack->second;
  auto &layer_map = layer_stack.layer_map_;

  auto layer = layer_map.find(layer_id);
  if (layer == layer_map.end()) {
    DLOGW("GetLayer(%" PRIu64 ") failed: no such layer", layer_id);
    return nullptr;
  }

  return layer->second;
}

DisplayError SDMLayerBuilder::SetCursorPosition(uint64_t disp_id,
                                                int64_t layer_id, int32_t x,
                                                int32_t y) {
  return CallLayerFunction(disp_id, layer_id, &SDMLayer::SetCursorPosition, x,
                           y);
}

DisplayError SDMLayerBuilder::SetLayerAsMask(uint64_t disp_id,
                                             int64_t layer_id) {
  if (disable_mask_layer_hint_) {
    DLOGW("Mask layer hint is disabled!");
    return kErrorResources;
  }

  auto sdm_layer = GetSDMLayer(disp_id, layer_id);
  if (!sdm_layer) {
    DLOGW("Failed to retrieve the sdm layer fpr display: %" PRIu64, disp_id);
    return kErrorResources;
  }

  sdm_layer->SetLayerAsMask();

  return kErrorNone;
}

DisplayError SDMLayerBuilder::CreateLayer(uint64_t display_id,
                                          int64_t *out_layer_id) {
  SCOPE_LOCK(locker_[display_id]);
  if (display_layer_stack_.find(display_id) == display_layer_stack_.end()) {
    DLOGW("Display: %" PRIu64 " not found - may have been deleted already", display_id);
    return kErrorNotSupported;
  }
  auto layer = new SDMLayer(display_id, buffer_allocator_);
  auto layer_id = layer->GetId();

  if (disable_sdr_histogram_) {
    layer->IgnoreSdrHistogramMetadata(true);
  }

  auto &stack = display_layer_stack_[display_id];

  stack.layer_set_.emplace(layer);
  stack.layer_map_.emplace(std::make_pair(layer_id, layer));

  *out_layer_id = layer_id;
  stack.geometry_changes_ |= GeometryChanges::kAdded;

  return kErrorNone;
}

DisplayError SDMLayerBuilder::DestroyLayer(uint64_t display_id,
                                           int64_t layer_id) {
  SCOPE_LOCK(locker_[display_id]);
  auto stack = display_layer_stack_.find(display_id);
  if (stack == display_layer_stack_.end()) {
    DLOGW("Display: %" PRIu64 " not found - may have been deleted already", display_id);
    return kErrorNotSupported;
  }

  auto &layer_stack = stack->second;
  auto &layer_set = layer_stack.layer_set_;
  auto &layer_map = layer_stack.layer_map_;

  auto layer_iter = layer_map.find(layer_id);
  if (layer_iter == layer_map.end()) {
    DLOGW("Layer: %" PRIu64 " not found", layer_id);
    return kErrorNotSupported;
  }

  const auto layer = layer_iter->second;
  layer_map.erase(layer_iter);

  const auto z_range = layer_set.equal_range(layer);
  for (auto current = z_range.first; current != z_range.second; ++current) {
    if (*current == layer) {
      current = layer_set.erase(current);
      delete layer;
      break;
    }
  }

  layer_stack.geometry_changes_ |= GeometryChanges::kRemoved;
  return kErrorNone;
}

SDMCompositionType
SDMLayerBuilder::GetDeviceSelectedCompositionType(uint64_t display_id,
                                                  int64_t layer_id) {
  auto layer = GetSDMLayer(display_id, layer_id);
  if (!layer) {
    return SDMCompositionType::COMP_INVALID;
  }

  return layer->GetDeviceSelectedCompositionType();
}

DisplayError
SDMLayerBuilder::SetLayerBuffer(uint64_t display_id, int64_t layer_id,
                                const SnapHandle *buffer,
                                const shared_ptr<Fence> &acquire_fence) {
  SCOPE_LOCK(locker_[display_id]);
  auto layer = GetSDMLayer(display_id, layer_id);
  if (!layer) {
    return kErrorNotSupported;
  }

  return layer->SetLayerBuffer(buffer, acquire_fence);
}

DisplayError SDMLayerBuilder::SetLayerBlendMode(uint64_t display, int64_t layer,
                                                int32_t int_mode) {
  auto mode = static_cast<LayerBlending>(int_mode);
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerBlendMode, mode);
}

DisplayError SDMLayerBuilder::SetLayerDisplayFrame(uint64_t display,
                                                   int64_t layer,
                                                   SDMRect frame) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerDisplayFrame,
                           frame);
}

DisplayError SDMLayerBuilder::SetLayerPlaneAlpha(uint64_t display,
                                                 int64_t layer, float alpha) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerPlaneAlpha,
                           alpha);
}

DisplayError SDMLayerBuilder::SetLayerSourceCrop(uint64_t display,
                                                 int64_t layer, SDMRect crop) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerSourceCrop, crop);
}

DisplayError SDMLayerBuilder::SetLayerTransform(uint64_t display, int64_t layer,
                                                SDMTransform tf) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerTransform, tf);
}

DisplayError SDMLayerBuilder::SetLayerZOrder(uint64_t display_id,
                                             int64_t layer_id, uint32_t z) {
  SCOPE_LOCK(locker_[display_id]);
  auto stack = display_layer_stack_.find(display_id);
  if (stack == display_layer_stack_.end()) {
    DLOGW("Display: %" PRIu64 " not found", display_id);
    return kErrorResources;
  }

  auto &layer_stack = stack->second;
  auto &layer_map = layer_stack.layer_map_;
  auto &layer_set = layer_stack.layer_set_;

  const auto map_layer = layer_map.find(layer_id);
  if (map_layer == layer_map.end()) {
    DLOGW("[%" PRIu64 "] updateLayerZ failed to find layer", layer_id);
    return kErrorResources;
  }

  const auto layer = map_layer->second;
  const auto z_range = layer_set.equal_range(layer);
  bool layer_on_display = false;
  for (auto current = z_range.first; current != z_range.second; ++current) {
    if (*current == layer) {
      if ((*current)->GetZ() == z) {
        // Don't change anything if the Z hasn't changed
        return kErrorNone;
      }
      current = layer_set.erase(current);
      layer_on_display = true;
      break;
    }
  }

  if (!layer_on_display) {
    DLOGE("[%" PRIu64 "] updateLayerZ failed to find layer on display",
          layer_id);
    return kErrorNotSupported;
  }

  layer->SetLayerZOrder(z);
  layer_set.emplace(layer);
  return kErrorNone;
}

DisplayError SDMLayerBuilder::SetLayerType(uint64_t display, int64_t layer,
                                           SDMLayerTypes type) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerType, type);
}

DisplayError SDMLayerBuilder::SetLayerFlag(uint64_t display, int64_t layer,
                                           SDMLayerFlag flag) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerFlag, flag);
}

DisplayError SDMLayerBuilder::SetLayerSurfaceDamage(uint64_t display,
                                                    int64_t layer_id,
                                                    SDMRegion damage) {
  SCOPE_LOCK(locker_[display]);
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  auto layer = GetSDMLayer(display, layer_id);
  if (!layer) {
    return kErrorNotSupported;
  }

  return layer->SetLayerSurfaceDamage(damage);
}

DisplayError SDMLayerBuilder::SetLayerVisibleRegion(uint64_t display,
                                                    int64_t layer,
                                                    SDMRegion damage) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerVisibleRegion,
                           damage);
}

DisplayError SDMLayerBuilder::SetLayerCompositionType(uint64_t display,
                                                      int64_t layer,
                                                      int32_t int_type) {
  auto type = static_cast<SDMCompositionType>(int_type);
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerCompositionType,
                           type);
}

DisplayError SDMLayerBuilder::SetLayerColor(uint64_t display, int64_t layer,
                                            SDMColor color) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerColor, color);
}

DisplayError SDMLayerBuilder::SetLayerDataspace(uint64_t display, int64_t layer,
                                                int32_t dataspace) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerDataspace,
                           dataspace);
}

DisplayError SDMLayerBuilder::SetLayerPerFrameMetadata(uint64_t display,
                                                       int64_t layer_id,
                                                       uint32_t num_elements,
                                                       const int32_t *int_keys,
                                                       const float *metadata) {
  SCOPE_LOCK(locker_[display]);
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  auto keys = reinterpret_cast<const SDMPerFrameMetadataKey *>(int_keys);
  auto layer = GetSDMLayer(display, layer_id);
  if (!layer) {
    return kErrorNotSupported;
  }

  return layer->SetLayerPerFrameMetadata(num_elements, keys, metadata);
}

DisplayError SDMLayerBuilder::SetLayerColorTransform(uint64_t display,
                                                     int64_t layer,
                                                     const float *matrix) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerColorTransform,
                           matrix);
}

DisplayError SDMLayerBuilder::SetLayerPerFrameMetadataBlobs(
    uint64_t display, int64_t layer, uint32_t num_elements,
    const int32_t *int_keys, const uint32_t *sizes, const uint8_t *metadata) {
  auto keys = reinterpret_cast<const SDMPerFrameMetadataKey *>(int_keys);
  return CallLayerFunction(display, layer,
                           &SDMLayer::SetLayerPerFrameMetadataBlobs,
                           num_elements, keys, sizes, metadata);
}

DisplayError SDMLayerBuilder::SetLayerBrightness(uint64_t display,
                                                 int64_t layer,
                                                 float brightness) {
  return CallLayerFunction(display, layer, &SDMLayer::SetLayerBrightness,
                           brightness);
}

} // namespace sdm
