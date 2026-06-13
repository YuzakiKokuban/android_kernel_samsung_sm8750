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
#ifndef __SDM_DISPLAY_INTF_DRAWCYCLE_H__
#define __SDM_DISPLAY_INTF_DRAWCYCLE_H__

#include <color_metadata.h>
#include <core/display_interface.h>
#include <core/layer_stack.h>
#include <core/sdm_types.h>
#include <utils/fence.h>

namespace sdm {

class SDMDisplayDrawCycleIntf {
public:
  SDMDisplayDrawCycleIntf() {}
  virtual ~SDMDisplayDrawCycleIntf() {}

  /**
   * Attempt to prepare and present a layer stack to the specified display in
   * one call.
   *
   * SDM will always assume a full layer stack, so any incremental builds or
   * optimizations must be done in compositor backend before submitting the
   * layer stack.
   *
   * If SDM is unable to handle the layer stack, return an error code with
   * details on what the client needs to do next, and afterwards, resubmit the
   * layer stack to thie method.
   *
   * The layer stack should not be modified until this call returns to avoid any
   * concurrency issues.
   *
   * FBT = Frame Buffer Target
   *
   * Draw method contracts:
   *    - Unified
   *    - Unified with GPU target
   *
   * @param display_id: The id of the specified display
   * @param draw_method: The method SDM will use to draw the layer stack
   * @param layer_stack: pointer to the layer stack to be presented
   *
   * @return: SUCCESS: SDM was able to prepare and present the layer stack in
   * one call NEED_FENCE_BIND: If a fbt was submitted with a speculative fence,
   *          NEED_FBT: If SDM is unable to handle all of the layers, the layers
   * which cannot be handled will be marked for GPU comp. Client must send those
   * layers to GPU and resubmit layer stack with an fbt on top FAILED: If a
   * fatal error occurs when attempting to present. Check the returned error
   * code to see what went wrong
   */
  virtual DisplayError PresentDisplay(uint64_t display_id,
                                      DisplayDrawMethod draw_method,
                                      LayerStack *layer_stack,
                                      SDMPresentResult *present_type) = 0;

  /**
   * Flush any pending buffers/fences submitted previously through a
   * PresentDisplay() call
   *
   * Client shall call this method to request SDM to release all buffers and
   * respective fences currently in use. This may result in a blank display
   * until a new frame is submitted.
   *
   * @param display_id: The id of the specified display
   * @param layer_stack: The layer stack to flush
   */
  virtual DisplayError Flush(uint64_t display_id, LayerStack *layer_stack) = 0;

  /**
   * Client can use this method to enable/disable a specific type of layer
   * composition. If client disables a composition type, SDM will not handle any
   * of the layer composition using the disabled method in a draw cycle. On lack
   * of resources to handle all layers using other enabled composition methods,
   * Prepare() will return an error.
   *
   * Default state of all composition types is enabled
   *
   * @param display_id: The id of the specified display
   * @param composition: The type of composition to enable/disable
   * @param enable: Enable or disable comp type
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the comp type cannot be disabled/enabled
   *                  - Currently, only gpu comp can be disabled
   */
  virtual DisplayError SetCompositionState(uint64_t display_id,
                                           LayerComposition composition,
                                           bool enable) = 0;

  /**
   * Set the expected time for the current frame to present on the display
   *
   * @param display_id: The id of the specified display
   * @param expected_time: Expected present time in ns
   */
  virtual DisplayError SetExpectedPresentTime(uint64_t display,
                                              uint64_t expectedPresentTime) = 0;

  /**
   * Check if a frame buffer target(fbt) with the given configuration is
   * supported by the hardware.
   *
   * @param display_id: The id of the specified display
   * @param width: fbt width
   * @param height: fbt height
   * @param format: fbt format
   * @param color_metadata: fbt color metadata
   *
   * @return: kErrorNone if the config is supported
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if one of the following conditions is met:
   *                - fbt format is not supported on the display
   *                - fbt width/height combo is not supported on the display
   *                - unsupported GammaTransfer or ColorPrimaries in fbt
   * metadata
   *                - fbt color metadata requests an unsupported mode
   */
  virtual DisplayError GetClientTargetSupport(uint64_t display_id,
                                              int32_t in_width,
                                              int32_t in_height,
                                              LayerBufferFormat format,
                                              Dataspace color_metadata) = 0;

  /**
   * Get a list of layer id's on the given display and their corresponding
   * release fences.
   *
   * Since we can't return the actual fence impl, we will return the
   * fence fd and the client can create their own fence using the fd.
   *
   * @param display_id: The id of the given display
   *
   * @return: parcel containing list of layer id's and their release fence.
   *          The layer id at slot layers[i] corresponds to the fence at
   * fences[i]
   *
   * @exception: kErrorParameters if display_id is not found
   */
  // QUESTION(JJ): Can we replace this format of out_num pointers with something
  // more logical? Maybe vector<pair<LayerId, shared_ptr<Fence>>> so only need 1
  // call seems like cleaner call flow to me
  virtual DisplayError
  GetReleaseFences(uint64_t display, uint32_t *out_num_elements,
                   LayerId *out_layers,
                   std::vector<shared_ptr<Fence>> *out_fences) = 0;

  /**
   * Enable or disable vsync on a given display
   *
   * @param display_id: The id of the given display
   * @param enable: true to enable vsync, false to disable
   */
  virtual DisplayError SetVsyncEnabled(uint64_t display_id, bool enable) = 0;

  /**
   * Set the output buffer for a virtual display
   *
   * @param display_id: The id of the specified display
   * @param buffer: The buffer to be written to
   * @param release_fence: Fence that represents when the buffer is safe to
   * access
   *
   * @exception: kErrorParameters if display_id is not found
   * @exception: kErrorNotSupported if the display isn't virtual
   */
  virtual DisplayError SetOutputBuffer(uint64_t display, const SnapHandle *buffer,
                              const shared_ptr<Fence> &release_fence) = 0;

  virtual DisplayError GetVsyncPeriod(uint64_t display_id,
                                      uint32_t *vsync_period) = 0;

  virtual void Refresh(uint64_t display_id);

  virtual DisplayError SetClientTarget(uint64_t display, const SnapHandle *target,
                            shared_ptr<Fence> acquire_fence, int32_t dataspace,
                            const SDMRegion& damage, uint32_t version) = 0;


  virtual DisplayError CommitOrPrepare(uint64_t display, bool validate_only,
                                       shared_ptr<Fence> *out_retire_fence,
                                       uint32_t *out_num_types,
                                       uint32_t *out_num_requests,
                                       bool *needs_commit) = 0;

  virtual DisplayError PresentDisplay(uint64_t display,
                                      shared_ptr<Fence> *out_retire_fence) = 0;

  virtual DisplayError GetChangedCompositionTypes(uint64_t display,
                                                  uint32_t *out_num_elements,
                                                  LayerId *out_layers,
                                                  int32_t *out_types) = 0;

  virtual DisplayError GetDisplayRequests(uint64_t display,
                                          int32_t *out_display_requests,
                                          uint32_t *out_num_elements,
                                          LayerId *out_layers,
                                          int32_t *out_layer_requests) = 0;

  virtual DisplayError SetDisplayElapseTime(uint64_t display,
                                            uint64_t time) = 0;

  virtual DisplayError AcceptDisplayChanges(uint64_t display) = 0;

  virtual DisplayError GetActiveConfigIndex(uint64_t display,
                                            uint32_t *config) = 0;

  virtual DisplayError SetActiveConfigIndex(uint64_t display,
                                            uint32_t config) = 0;

  virtual DisplayError
  MinHdcpEncryptionLevelChanged(uint64_t display, uint32_t min_enc_level) = 0;

  virtual void LayerStackUpdated(uint64_t display) = 0;

  virtual void WaitForDrawCycleToComplete(uint64_t display) = 0;

  virtual DisplayError NotifyExpectedPresent(uint64_t display, uint64_t expected_present_time,
                                             uint32_t frame_interval_ns) = 0;

  virtual DisplayError SetFrameIntervalNs(uint64_t display, uint32_t frame_interval_ns) = 0;
};

} // namespace sdm

#endif // __SDM_DISPLAY_INTF_DRAWCYCLE_H__
