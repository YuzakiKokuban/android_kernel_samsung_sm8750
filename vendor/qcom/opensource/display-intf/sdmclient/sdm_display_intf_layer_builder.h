/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_LAYER_BUILDER_INTF_H__
#define __SDM_DISPLAY_LAYER_BUILDER_INTF_H__

#include <unordered_map>
#include <unordered_set>

#include <SnapHandle.h>
#include <core/buffer_allocator.h>
#include <core/sdm_types.h>

namespace sdm {

using SnapHandle = vendor::qti::hardware::display::snapalloc::SnapHandle;

class SDMDisplayLayerBuilderIntf {
public:
  virtual ~SDMDisplayLayerBuilderIntf() {}

  virtual DisplayError Init(BufferAllocator *buffer_allocator,
                            uint64_t display_id) = 0;

  virtual DisplayError DeInit(uint64_t display_id) = 0;

  virtual DisplayError CreateLayer(uint64_t display_id,
                                   int64_t *out_layer_id) = 0;

  virtual DisplayError DestroyLayer(uint64_t display_id, int64_t layer_id) = 0;

  virtual DisplayError SetLayerBuffer(uint64_t display_id, int64_t layer_id, const SnapHandle *buffer,
                             const shared_ptr<Fence> &acquire_fence) = 0;

  virtual DisplayError SetLayerBlendMode(uint64_t display_id, int64_t layer_id,
                                         int32_t int_mode) = 0;

  virtual DisplayError SetLayerDisplayFrame(uint64_t display_id,
                                            int64_t layer_id,
                                            SDMRect frame) = 0;

  virtual DisplayError SetLayerPlaneAlpha(uint64_t display_id, int64_t layer_id,
                                          float alpha) = 0;

  virtual DisplayError SetLayerSourceCrop(uint64_t display_id, int64_t layer_id,
                                          SDMRect crop) = 0;

  virtual DisplayError SetLayerTransform(uint64_t display_id, int64_t layer_id,
                                         SDMTransform transform) = 0;

  virtual DisplayError SetLayerZOrder(uint64_t display_id, int64_t layer_id,
                                      uint32_t z) = 0;

  virtual DisplayError SetLayerType(uint64_t display_id, int64_t layer_id,
                                    SDMLayerTypes type) = 0;

  virtual DisplayError SetLayerFlag(uint64_t display_id, int64_t layer_id,
                                    SDMLayerFlag flag) = 0;

  virtual DisplayError SetLayerSurfaceDamage(uint64_t display_id,
                                             int64_t layer_id,
                                             SDMRegion damage) = 0;
  virtual DisplayError SetLayerVisibleRegion(uint64_t display_id,
                                             int64_t layer_id,
                                             SDMRegion damage) = 0;

  virtual DisplayError SetLayerCompositionType(uint64_t display_id,
                                               int64_t layer_id,
                                               int32_t int_type) = 0;

  virtual DisplayError SetLayerColor(uint64_t display_id, int64_t layer_id,
                                     SDMColor color) = 0;

  virtual DisplayError SetLayerDataspace(uint64_t display_id, int64_t layer_id,
                                         int32_t dataspace) = 0;

  virtual DisplayError SetLayerPerFrameMetadata(uint64_t display_id,
                                                int64_t layer_id,
                                                uint32_t num_elements,
                                                const int32_t *int_keys,
                                                const float *metadata) = 0;

  virtual DisplayError SetLayerColorTransform(uint64_t display_id,
                                              int64_t layer_id,
                                              const float *matrix) = 0;

  virtual DisplayError
  SetLayerPerFrameMetadataBlobs(uint64_t display_id, int64_t layer_id,
                                uint32_t num_elements, const int32_t *int_keys,
                                const uint32_t *sizes,
                                const uint8_t *metadata) = 0;

  virtual DisplayError SetLayerBrightness(uint64_t display_id, int64_t layer_id,
                                          float brightness) = 0;

  virtual DisplayError SetLayerAsMask(uint64_t display_id,
                                      int64_t layer_id) = 0;

  virtual SDMCompositionType
  GetDeviceSelectedCompositionType(uint64_t display_id, int64_t layer_id) = 0;

  virtual DisplayError SetCursorPosition(uint64_t disp_id, int64_t layer_id,
                                         int32_t x, int32_t y) = 0;

  virtual LayerBufferFormat GetSDMFormat(const int32_t &source,
                                         const int32_t flags,
                                         const int64_t compression_type) = 0;
};

} // namespace sdm

#endif // __SDM_DISPLAY_LAYER_BUILDER_INTF_H__
