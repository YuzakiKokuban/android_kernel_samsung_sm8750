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
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <utils/constants.h>
#include <utils/debug.h>
#include <vector>

#include "strategy.h"
#include "utils/rect.h"

#define __CLASS__ "Strategy"

namespace sdm {

Strategy::Strategy(ExtensionInterface *extension_intf,
                   BufferAllocator *buffer_allocator,
                   DisplayId display_id, SDMDisplayType type,
                   const std::vector<HWResourceInfo> &hw_resource_info,
                   DisplayInfoContext &info_ctx,
                   DisplayDeviceContext &device_ctx)
  : extension_intf_(extension_intf),
    display_id_info_(display_id),
    display_type_(type),
    hw_resource_info_(hw_resource_info),
    info_ctx_(info_ctx),
    device_ctx_(device_ctx),
    buffer_allocator_(buffer_allocator) {
    display_id_ = display_id_info_.GetDisplayId();
  }

DisplayError Strategy::Init() {
  DisplayError error = kErrorNone;

  if (extension_intf_) {
    error = extension_intf_->CreateStrategyExtn(display_id_info_, display_type_, buffer_allocator_,
                                                hw_resource_info_, info_ctx_, device_ctx_,
                                                &strategy_intf_);
    if (error != kErrorNone) {
      DLOGE("Failed to create strategy for display %d-%d", display_id_, display_type_);
      return error;
    }

    error = extension_intf_->CreatePartialUpdate(display_id_info_, display_type_, hw_resource_info_,
                                                 info_ctx_, &partial_update_intf_);
  }

  return kErrorNone;
}

DisplayError Strategy::Deinit() {
  if (strategy_intf_) {
    if (partial_update_intf_) {
      extension_intf_->DestroyPartialUpdate(partial_update_intf_);
    }

    extension_intf_->DestroyStrategyExtn(strategy_intf_);
  }

  return kErrorNone;
}

void Strategy::GenerateROI(DispLayerStack *disp_layer_stack, const PUConstraints &pu_constraints) {
  disp_layer_stack_ = disp_layer_stack;

  if (partial_update_intf_) {
    partial_update_intf_->Start(pu_constraints);
  }

  return GenerateROI();
}

DisplayError Strategy::Start(DispLayerStack *disp_layer_stack, uint32_t *max_attempts,
                             StrategyConstraints *constraints) {
  DisplayError error = kErrorNone;
  disp_layer_stack_ = disp_layer_stack;
  extn_start_success_ = false;

  if (strategy_intf_) {
    error = strategy_intf_->Start(disp_layer_stack_, max_attempts, constraints);
    if (error == kErrorNone || error == kErrorNeedsValidate ||
        error == kErrorNeedsLutRegen || error == kErrorNeedsQosRecalc ||
        error == kErrorNeedsQosRecalcAndLutRegen) {
      extn_start_success_ = true;
    } else {
      *max_attempts = 1;
      error = kErrorNeedsValidate;
    }
  }

  disp_layer_stack_->stack->flags.default_strategy = !extn_start_success_;
  return error;
}

DisplayError Strategy::Stop() {
  if (strategy_intf_) {
    return strategy_intf_->Stop();
  }

  return kErrorNone;
}

DisplayError Strategy::GetNextStrategy() {
  if (!disable_gpu_comp_ && !disp_layer_stack_->stack_info.gpu_target_index) {
    DLOGE("GPU composition is enabled and GPU target buffer not provided for display %d-%d.",
          display_id_, display_type_);
    return kErrorNotSupported;
  }

  if (extn_start_success_) {
    return strategy_intf_->GetNextStrategy();
  }

  // Do not fallback to GPU if GPU comp is disabled.
  if (disable_gpu_comp_) {
    return kErrorNotSupported;
  }

  // Mark all application layers for GPU composition. Find GPU target buffer and store its index for
  // programming the hardware.
  LayerStack *layer_stack = disp_layer_stack_->stack;
  for (uint32_t i = 0; i < disp_layer_stack_->stack_info.app_layer_count; i++) {
    layer_stack->layers.at(i)->composition = kCompositionGPU;
    layer_stack->layers.at(i)->request.flags.request_flags = 0;  // Reset layer request
  }

  for (auto& info : disp_layer_stack_->info) {
    info.second.hw_layers.clear();
    info.second.index.clear();
    info.second.roi_index.clear();
  }

  // When mixer resolution and panel resolutions are same (1600x2560) and FB resolution is
  // 1080x1920 FB_Target destination coordinates(mapped to FB resolution 1080x1920) need to
  // be mapped to destination coordinates of mixer resolution(1600x2560).
  Layer *gpu_target_layer =
                         layer_stack->layers.at(disp_layer_stack_->stack_info.gpu_target_index);
  Layer gpu_layer = *gpu_target_layer;

  float layer_mixer_width = FLOAT(info_ctx_.mixer_attributes.width);
  float layer_mixer_height = FLOAT(info_ctx_.mixer_attributes.height);
  float fb_width = FLOAT(info_ctx_.fb_config.x_pixels);
  float fb_height = FLOAT(info_ctx_.fb_config.y_pixels);
  LayerRect src_domain = (LayerRect){0.0f, 0.0f, fb_width, fb_height};
  LayerRect dst_domain = (LayerRect){0.0f, 0.0f, layer_mixer_width, layer_mixer_height};

  gpu_layer.transform.flip_horizontal ^= info_ctx_.hw_panel_info.panel_orientation.flip_horizontal;
  gpu_layer.transform.flip_vertical ^= info_ctx_.hw_panel_info.panel_orientation.flip_vertical;
  // Flip rect to match transform.
  TransformHV(src_domain, gpu_layer.dst_rect, gpu_layer.transform, &gpu_layer.dst_rect);
  // Scale to mixer resolution.
  MapRect(src_domain, dst_domain, gpu_layer.dst_rect, &gpu_layer.dst_rect);

  LayerRect src_rect = gpu_layer.src_rect;
  LayerRect dst_rect = gpu_layer.dst_rect;

  for (auto& info : disp_layer_stack_->info) {
    Layer layer = *gpu_target_layer;

    float dpu_lm_width = FLOAT(device_ctx_[info.first].mixer_attributes.width);
    uint32_t dpu_offset = info.first * dpu_lm_width;  // mixer start inedx for any DPU

    CalculateDstRect(dpu_offset, dpu_lm_width, dst_rect, &layer.dst_rect);

    float dpu_dst_width = layer.dst_rect.right - layer.dst_rect.left;
    int final_transform = ComputeTransform(layer.transform);
    float split_ratio = (dpu_dst_width / (dst_rect.right - dst_rect.left));

    CalculateSrcRect(layer, split_ratio, final_transform, &src_rect, &layer.src_rect);
    info.second.hw_layers.push_back(layer);

    info.second.index.push_back(disp_layer_stack_->stack_info.gpu_target_index);
    info.second.roi_index.push_back(0);
    info.second.common_info = &disp_layer_stack_->stack_info.common_info;
  }

  return kErrorNone;
}

void Strategy::GenerateROI() {
  bool split_display = false;

  if (partial_update_intf_ && partial_update_intf_->GenerateROI(disp_layer_stack_) == kErrorNone) {
    return;
  }

  float layer_mixer_width = info_ctx_.mixer_attributes.width;
  float layer_mixer_height = info_ctx_.mixer_attributes.height;

  bool is_src_split = true;
  std::bitset<8> core_id_map = display_id_info_.GetCoreIdMap();
  for (auto& res_info : hw_resource_info_) {
    if (!core_id_map[res_info.core_id]) {
      continue;
    }
    is_src_split &= res_info.is_src_split;
  }

  if (!is_src_split && info_ctx_.display_attributes.is_device_split) {
    split_display = true;
  }

  disp_layer_stack_->stack_info.left_frame_roi = {};
  disp_layer_stack_->stack_info.right_frame_roi = {};

  if (split_display) {
    float left_split = FLOAT(info_ctx_.mixer_attributes.split_left);
    disp_layer_stack_->stack_info.left_frame_roi.push_back(LayerRect(0.0f, 0.0f,
                                left_split, layer_mixer_height));
    disp_layer_stack_->stack_info.right_frame_roi.push_back(LayerRect(left_split,
                                0.0f, layer_mixer_width, layer_mixer_height));
  } else {
    disp_layer_stack_->stack_info.left_frame_roi.push_back(LayerRect(0.0f, 0.0f,
                                layer_mixer_width, layer_mixer_height));
    disp_layer_stack_->stack_info.right_frame_roi.push_back(LayerRect(0.0f, 0.0f, 0.0f, 0.0f));
  }
}

DisplayError Strategy::Reconfigure(DisplayInfoContext &info_ctx,
                                   DisplayDeviceContext &device_ctx) {
  DisplayError error = kErrorNone;

  if (!extension_intf_) {
    return kErrorNone;
  }

  // TODO(user): PU Intf will not be created for video mode panels, hence re-evaluate if
  // reconfigure is needed.
  if (partial_update_intf_) {
    extension_intf_->DestroyPartialUpdate(partial_update_intf_);
    partial_update_intf_ = NULL;
  }

  extension_intf_->CreatePartialUpdate(display_id_info_, display_type_, hw_resource_info_,
                                       info_ctx, &partial_update_intf_);

  if (partial_update_intf_ && spr_intf_) {
    partial_update_intf_->SetSprIntf(spr_intf_);
  }

  if (partial_update_intf_) {
    partial_update_intf_->SetDetailEnhancerData(de_data_);
  }

  error = strategy_intf_->Reconfigure(info_ctx, device_ctx, hw_resource_info_);
  if (error != kErrorNone) {
    return error;
  }

  info_ctx_ = info_ctx;
  device_ctx_ = device_ctx;

  return kErrorNone;
}

DisplayError Strategy::SetCompositionState(LayerComposition composition_type, bool enable) {
  DLOGI("composition type = %d, enable = %d for display %d-%d", composition_type, enable,
        display_id_, display_type_);

  if (composition_type == kCompositionGPU) {
    disable_gpu_comp_ = !enable;
  }

  if (strategy_intf_) {
    return strategy_intf_->SetCompositionState(composition_type, enable);
  }

  return kErrorNone;
}

DisplayError Strategy::Purge() {
  if (strategy_intf_) {
    return strategy_intf_->Purge();
  }

  return kErrorNone;
}

void Strategy::ResetStrategy(uint32_t *max_attempts) {
  if (strategy_intf_) {
    strategy_intf_->ResetStrategy(max_attempts);
  }
}

DisplayError Strategy::SetDrawMethod(const DisplayDrawMethod &draw_method) {
  if (strategy_intf_) {
    return strategy_intf_->SetDrawMethod(draw_method);
  }

  return kErrorNotSupported;
}

DisplayError Strategy::SetIdleTimeoutMs(uint32_t active_ms, uint32_t inactive_ms) {
  if (strategy_intf_) {
    return strategy_intf_->SetIdleTimeoutMs(active_ms, inactive_ms);
  }

  return kErrorNotSupported;
}

DisplayError Strategy::SetColorModesInfo(const std::vector<PrimariesTransfer> &colormodes_cs) {
  if (strategy_intf_) {
    return strategy_intf_->SetColorModesInfo(colormodes_cs);
  }
  return kErrorNotSupported;
}

DisplayError Strategy::SetBlendSpace(const PrimariesTransfer &blend_space) {
  if (strategy_intf_) {
    return strategy_intf_->SetBlendSpace(blend_space);
  }
  return kErrorNotSupported;
}

void Strategy::SetDisplayLayerStack(DispLayerStack *disp_layer_stack) {
  disp_layer_stack_ = disp_layer_stack;
  if (strategy_intf_) {
    strategy_intf_->SetDisplayLayerStack(disp_layer_stack);
  }
}

void Strategy::CalculateDstRect(uint32_t dpu_offset, uint32_t mixer_width,
                                LayerRect in_rect, LayerRect *out_rect) {
  float left = (in_rect.left > dpu_offset ? in_rect.left : dpu_offset);
  float right = (in_rect.right > (dpu_offset +  mixer_width) ? dpu_offset + mixer_width :
                 in_rect.right);
  *out_rect = {left - dpu_offset, in_rect.top, right - dpu_offset, in_rect.bottom};
}

void Strategy::CalculateSrcRect(const Layer &layer, float split_factor, int transform,
                                LayerRect *in_rect, LayerRect *out_rect) {
  float start_index = 0;
  switch (transform) {
    // src_rect for i DPU is present at width-i position,
    case kTransformFlipHorizontal:
    case kTransform180:
      start_index = in_rect->right;
      SplitFromRight(split_factor, layer, start_index, out_rect);
      in_rect->right = out_rect->left;
      break;

    case kTransform90:
      start_index = in_rect->bottom;
      SplitFromBottom(split_factor, layer, start_index, out_rect);
      in_rect->bottom = out_rect->top;
      break;

    case kTransform270:
      start_index = in_rect->top;
      SplitFromTop(split_factor, layer, start_index, out_rect);
      in_rect->top = out_rect->bottom;
      break;

    case kTransformFlipVertical:
    case kTransformNone:
    default:
      start_index = in_rect->left;
      SplitFromLeft(split_factor, layer, start_index, out_rect);
      in_rect->left = out_rect->right;
      break;
  }
}

DisplayError Strategy::SetSprIntf(std::shared_ptr<SPRIntf> intf) {
  if (partial_update_intf_) {
    DisplayError ret = partial_update_intf_->SetSprIntf(intf);
    if (ret != kErrorNone) {
      return ret;
    }
    spr_intf_ = intf;
    return ret;
  }
  return kErrorNotSupported;
}

DisplayError Strategy::SetDetailEnhancerData(const DisplayDetailEnhancerData &de_data) {
  de_data_ = de_data;
  if (!partial_update_intf_) {
    return kErrorNotSupported;
  }

  return partial_update_intf_->SetDetailEnhancerData(de_data);
}

}  // namespace sdm
