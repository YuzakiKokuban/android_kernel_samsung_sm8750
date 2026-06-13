/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_LAYER_BUILDER_H__
#define __SDM_LAYER_BUILDER_H__

#include <unordered_map>
#include <unordered_set>

#include <core/buffer_allocator.h>
#include <core/sdm_types.h>
#include <utils/locker.h>

#include "sdm_display_intf_layer_builder.h"
#include "sdm_layers.h"

namespace sdm {

struct SDMLayerStack {
  std::map<LayerId, SDMLayer *> layer_map_;
  std::multiset<SDMLayer *, SortLayersByZ> layer_set_;
  uint32_t geometry_changes_ = GeometryChanges::kNone;
};

class SDMLayerBuilder : public SDMDisplayLayerBuilderIntf {
public:
 ~SDMLayerBuilder();
 DisplayError Init(BufferAllocator *buffer_allocator, uint64_t display_id) override;
 DisplayError DeInit(uint64_t display_id) override;
 DisplayError CreateLayer(uint64_t display_id, int64_t *out_layer_id) override;
 DisplayError DestroyLayer(uint64_t display_id, int64_t layer_id) override;
 DisplayError SetLayerBuffer(uint64_t display_id, int64_t layer_id, const SnapHandle *buffer,
                             const shared_ptr<Fence> &acquire_fence) override;
 DisplayError SetLayerBlendMode(uint64_t display_id, int64_t layer_id, int32_t int_mode) override;
 DisplayError SetLayerDisplayFrame(uint64_t display_id, int64_t layer_id, SDMRect frame) override;
 DisplayError SetLayerPlaneAlpha(uint64_t display_id, int64_t layer_id, float alpha) override;
 DisplayError SetLayerSourceCrop(uint64_t display_id, int64_t layer_id, SDMRect crop) override;
 DisplayError SetLayerTransform(uint64_t display_id, int64_t layer_id,
                                SDMTransform transform) override;
 DisplayError SetLayerZOrder(uint64_t display_id, int64_t layer_id, uint32_t z) override;
 DisplayError SetLayerType(uint64_t display_id, int64_t layer_id, SDMLayerTypes type) override;
 DisplayError SetLayerFlag(uint64_t display_id, int64_t layer_id, SDMLayerFlag flag) override;
 DisplayError SetLayerSurfaceDamage(uint64_t display_id, int64_t layer_id,
                                    SDMRegion damage) override;
 DisplayError SetLayerVisibleRegion(uint64_t display_id, int64_t layer_id,
                                    SDMRegion damage) override;
 DisplayError SetLayerCompositionType(uint64_t display_id, int64_t layer_id,
                                      int32_t int_type) override;
 DisplayError SetLayerColor(uint64_t display_id, int64_t layer_id, SDMColor color) override;
 DisplayError SetLayerDataspace(uint64_t display_id, int64_t layer_id, int32_t dataspace) override;
 DisplayError SetLayerPerFrameMetadata(uint64_t display_id, int64_t layer_id, uint32_t num_elements,
                                       const int32_t *int_keys, const float *metadata) override;
 DisplayError SetLayerColorTransform(uint64_t display_id, int64_t layer_id,
                                     const float *matrix) override;
 DisplayError SetLayerPerFrameMetadataBlobs(uint64_t display_id, int64_t layer_id,
                                            uint32_t num_elements, const int32_t *int_keys,
                                            const uint32_t *sizes,
                                            const uint8_t *metadata) override;
 DisplayError SetLayerBrightness(uint64_t display_id, int64_t layer_id, float brightness) override;
 DisplayError SetLayerAsMask(uint64_t display_id, int64_t layer_id) override;
 SDMCompositionType GetDeviceSelectedCompositionType(uint64_t display_id,
                                                     int64_t layer_id) override;
 DisplayError SetCursorPosition(uint64_t disp_id, int64_t layer_id, int32_t x, int32_t y) override;
 LayerBufferFormat GetSDMFormat(const int32_t &source, const int32_t flags,
                                const int64_t compression_type) override;
 DisplayError GetSDMLayerStack(uint64_t display_id, SDMLayerStack **stack) {
   SCOPE_LOCK(locker_[display_id]);
   auto disp = display_layer_stack_.find(display_id);
   if (disp == display_layer_stack_.end()) {
     // initialize layer stack
     display_layer_stack_[display_id];
   }

   *stack = &display_layer_stack_[display_id];
   return kErrorNone;
 }

private:
  SDMLayer *GetSDMLayer(uint64_t display_id, int64_t layer_id);

  template <typename... Args>
  DisplayError CallLayerFunction(uint64_t display, int64_t layer_id,
                                 DisplayError (SDMLayer::*member)(Args...),
                                 Args... args) {
    if (display >= kNumDisplays) {
      return kErrorParameters;
    }

    SCOPE_LOCK(locker_[display]);
    auto layer = GetSDMLayer(display, layer_id);
    if (!layer) {
      return kErrorNotSupported;
    }

    return (layer->*member)(std::forward<Args>(args)...);
  }

  std::unordered_map<uint64_t, SDMLayerStack> display_layer_stack_;

  BufferAllocator *buffer_allocator_ = nullptr;

  int disable_sdr_histogram_ = 0; // disables handling of SDR histogram data.
  int32_t disable_mask_layer_hint_ = 0;

  static SDMLayerBuilder *layer_builder_;
  static uint32_t ref_count_;
  static std::mutex lock_;
  static Locker locker_[kNumDisplays];
};

} // namespace sdm

#endif // __SDM_LAYER_BUILDER_H__
