/*
* Copyright (c) 2014 - 2021, The Linux Foundation. All rights reserved.
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
*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __STRATEGY_H__
#define __STRATEGY_H__

#include <core/display_interface.h>
#include <private/extension_interface.h>
#include <core/buffer_allocator.h>
#include <private/spr_intf.h>
#include <vector>

namespace sdm {

class Strategy {
 public:
  Strategy(ExtensionInterface *extension_intf, BufferAllocator *buffer_allocator,
           DisplayId display_id, SDMDisplayType type,
           const std::vector<HWResourceInfo> &hw_resource_info,
           DisplayInfoContext &info_ctx, DisplayDeviceContext &device_ctx);

  DisplayError Init();
  DisplayError Deinit();

  DisplayError Start(DispLayerStack *disp_layer_stack, uint32_t *max_attempts,
                      StrategyConstraints *constraints);
  DisplayError GetNextStrategy();
  DisplayError Stop();
  DisplayError SetDrawMethod(const DisplayDrawMethod &draw_method);
  DisplayError Reconfigure(DisplayInfoContext &info_ctx,
                           DisplayDeviceContext &device_ctx);
  DisplayError SetCompositionState(LayerComposition composition_type, bool enable);
  DisplayError Purge();
  void ResetStrategy(uint32_t *max_attempts);
  DisplayError SetIdleTimeoutMs(uint32_t active_ms, uint32_t inactive_ms);
  DisplayError SetColorModesInfo(const std::vector<PrimariesTransfer> &colormodes_cs);
  DisplayError SetBlendSpace(const PrimariesTransfer &blend_space);
  void GenerateROI(DispLayerStack *disp_layer_stack, const PUConstraints &pu_constraints);
  void SetDisplayLayerStack(DispLayerStack *disp_layer_stack);
  DisplayError SetSprIntf(std::shared_ptr<SPRIntf> intf);
  DisplayError SetDetailEnhancerData(const DisplayDetailEnhancerData &de_data);

 private:
  void GenerateROI();
  void CalculateDstRect(uint32_t dpu_offset, uint32_t mixer_width,
                        LayerRect in_rect, LayerRect *out_rect);
  void CalculateSrcRect(const Layer &layer, float split_factor, int transform,
                        LayerRect *in_rect, LayerRect *out_rect);

  ExtensionInterface *extension_intf_ = NULL;
  StrategyInterface *strategy_intf_ = NULL;
  PartialUpdateInterface *partial_update_intf_ = NULL;
  DisplayId display_id_info_ = {};
  int32_t display_id_;
  SDMDisplayType display_type_;
  std::vector<HWResourceInfo> hw_resource_info_;
  DispLayerStack *disp_layer_stack_ = NULL;
  DisplayInfoContext info_ctx_;
  DisplayDeviceContext device_ctx_;
  bool extn_start_success_ = false;
  bool disable_gpu_comp_ = false;
  BufferAllocator *buffer_allocator_ = NULL;
  std::shared_ptr<SPRIntf> spr_intf_ = nullptr;
  DisplayDetailEnhancerData de_data_ = {};
};

}  // namespace sdm

#endif  // __STRATEGY_H__

