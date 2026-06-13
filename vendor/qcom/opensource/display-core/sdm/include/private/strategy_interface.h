/*
* Copyright (c) 2014 - 2019, 2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted
* provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright notice, this list of
*      conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright notice, this list of
*      conditions and the following disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __STRATEGY_INTERFACE_H__
#define __STRATEGY_INTERFACE_H__

#include <core/sdm_types.h>
#include <core/display_interface.h>
#include <vector>
#include "hw_info_types.h"
#include "layer_feedback.h"

namespace sdm {

struct StrategyConstraints {
  bool safe_mode = false;   //!< In this mode, strategy manager chooses the composition strategy
                            //!< that requires minimum number of pipe for the current frame. i.e.,
                            //!< video only composition, secure only composition or GPU composition

  uint32_t max_layers = kMaxSDELayers;  //!< Maximum number of layers that shall be programmed
                                        //!< on hardware for the given layer stack.
  LayerFeedback feedback = LayerFeedback(0);  //!< Feedback from Layer Precheck

  bool idle_timeout = false;
  bool gpu_fallback_mode = false;  //!< This flag forces GPU composition strategy.
  bool tonemapping_query_mandatory = false;  //!< This flag forces a strategy with tonemapping query
};

class StrategyInterface {
 public:
  virtual DisplayError Start(DispLayerStack *disp_layer_stack, uint32_t *max_attempts,
                             StrategyConstraints *constraints) = 0;
  virtual DisplayError GetNextStrategy() = 0;
  virtual DisplayError Stop() = 0;
  virtual DisplayError SetDrawMethod(const DisplayDrawMethod &draw_method) = 0;
  virtual DisplayError Reconfigure(DisplayInfoContext &info_ctx, DisplayDeviceContext &device_ctx,
                                   const std::vector<HWResourceInfo> &hw_res_info) = 0;
  virtual DisplayError SetCompositionState(LayerComposition composition_type, bool enable) = 0;
  virtual DisplayError Purge() = 0;
  virtual void ResetStrategy(uint32_t *max_attempts) = 0;
  virtual DisplayError SetIdleTimeoutMs(uint32_t active_ms, uint32_t inactive_ms) = 0;
  /* Sets the list of color modes supported on a display */
  virtual DisplayError SetColorModesInfo(const std::vector<PrimariesTransfer> &colormodes_cs) = 0;
  virtual DisplayError SetBlendSpace(const PrimariesTransfer &blend_space) = 0;
  virtual void SetDisplayLayerStack(DispLayerStack *disp_layer_stack) = 0;

  virtual ~StrategyInterface() { }
};

}  // namespace sdm

#endif  // __STRATEGY_INTERFACE_H__

