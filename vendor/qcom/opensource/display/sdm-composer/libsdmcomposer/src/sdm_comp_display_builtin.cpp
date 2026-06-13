/*
* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
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
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <utils/constants.h>
#include <utils/debug.h>
#include <errno.h>
#include <unistd.h>

#include "sdm_comp_display_builtin.h"
#include "sdm_comp_debugger.h"
#include "formats.h"
#include "utils/fence.h"
#include "utils/rect.h"


#define __CLASS__ "SDMCompDisplayBuiltIn"

namespace sdm {

SDMCompDisplayBuiltIn::SDMCompDisplayBuiltIn(CoreInterface *core_intf,
    CallbackInterface *callback, SDMCompDisplayType disp_type, int32_t disp_id)
    : core_intf_(core_intf), callback_(callback), display_type_(disp_type),
      display_id_(disp_id) {
}

int SDMCompDisplayBuiltIn::Init() {
  int status = 0;
  DisplayError error = core_intf_->CreateDisplay(display_id_, this, &display_intf_);
  if (error != kErrorNone) {
    if (kErrorDeviceRemoved == error) {
      DLOGW("Display creation cancelled. Display %d-%d removed.", display_id_, display_type_);
      return -ENODEV;
    } else {
      DLOGE("Display create failed. Error = %d display_id = %d event_handler = %p disp_intf = %p",
            error, display_id_, this, &display_intf_);
      return -EINVAL;
    }
  }

  error = display_intf_->SetDisplayState(kStateOn, false /* tear_down */, NULL /* release_fence */);
  if (error != kErrorNone) {
    status = -EINVAL;
    goto cleanup;
  }

  display_intf_->GetActiveConfig(&active_config_);
  display_intf_->GetConfig(active_config_, &variable_info_);
  display_intf_->SetCompositionState(kCompositionGPU, false);

  CreateLayerSet();
  error = display_intf_->GetStcColorModes(&stc_mode_list_);
  if (error != kErrorNone) {
    DLOGW("Failed to get Stc color modes, error %d", error);
    stc_mode_list_.list.clear();
  } else {
    DLOGI("Stc mode count %d", stc_mode_list_.list.size());
  }

  PopulateColorModes();

  return status;
cleanup:
  DestroyLayerSet();
  core_intf_->DestroyDisplay(display_intf_);
  return status;
}

int SDMCompDisplayBuiltIn::Deinit() {
  DisplayConfigFixedInfo fixed_info = {};
  display_intf_->GetConfig(&fixed_info);
  DisplayError error = kErrorNone;

  if (!fixed_info.is_cmdmode) {
    error = display_intf_->Flush(&layer_stack_);
    if (error != kErrorNone && error != kErrorPermission) {
      DLOGE("Flush failed. Error = %d", error);
      return -EINVAL;
    }
  }

  DestroyLayerSet();
  error = core_intf_->DestroyDisplay(display_intf_);
  if (error != kErrorNone) {
    DLOGE("Display destroy failed. Error = %d", error);
    return -EINVAL;
  }
  first_commit_ = true;

  return 0;
}

int SDMCompDisplayBuiltIn::GetNumVariableInfoConfigs(uint32_t *count) {
  if (!count) {
    return -EINVAL;
  }
  DisplayError error = display_intf_->GetNumVariableInfoConfigs(count);
  if (error != kErrorNone) {
    return -EINVAL;
  }
  return 0;
}

int SDMCompDisplayBuiltIn::GetDisplayConfig(int config_idx,
                                            DisplayConfigVariableInfo *variable_info) {
  if (!variable_info) {
    return -EINVAL;
  }
  DisplayError error = display_intf_->GetConfig(config_idx, variable_info);
  if (error != kErrorNone) {
    return -EINVAL;
  }
  return 0;
}

int SDMCompDisplayBuiltIn::SetDisplayConfig(int config_idx) {
  DisplayError error = display_intf_->SetActiveConfig(config_idx);
  if (error != kErrorNone) {
    return -EINVAL;
  }
  active_config_ = config_idx;

  error = display_intf_->GetConfig(config_idx, &variable_info_);
  if (error != kErrorNone) {
    return -EINVAL;
  }
  return 0;
}

void SDMCompDisplayBuiltIn::SetFrameBufferConfig() {
  DisplayConfigVariableInfo fb_config;
  display_intf_->GetFrameBufferConfig(&fb_config);
  fb_config.x_pixels = mixer_config_.width;
  fb_config.y_pixels = mixer_config_.height;
  display_intf_->SetFrameBufferConfig(fb_config);
}

void SDMCompDisplayBuiltIn::SetMixerConfig(uint32_t width, uint32_t height) {
  mixer_config_.width = width;
  mixer_config_.height = height;
  SetFrameBufferConfig();
}

int SDMCompDisplayBuiltIn::GetDisplayAttributes(SDMCompDisplayAttributes *display_attributes) {
  if (!display_attributes) {
    return -EINVAL;
  }

  // 1. When DS is not enabled, Layers are to be rendered at display resolution, which is same as
  //    LM config.
  // 2. When DS is enabled (for AI scaler), layers are to be rendered at resolution less than
  //    display resolution. In this scenario, LM is reconfigured to new resolution.
  // So in both scenarios, mixer attributes can be considered as Display attributes.
  display_attributes->x_res = mixer_config_.width;
  display_attributes->y_res = mixer_config_.height;
  display_attributes->x_dpi = variable_info_.x_dpi;
  display_attributes->y_dpi = variable_info_.y_dpi;
  display_attributes->vsync_period = variable_info_.vsync_period_ns;
  display_attributes->is_yuv = variable_info_.is_yuv;
  display_attributes->fps = variable_info_.fps;
  display_attributes->smart_panel = variable_info_.smart_panel;

  return 0;
}

int SDMCompDisplayBuiltIn::ShowBuffer(BufferHandle *buf_handle, int32_t *retire_fence) {
  int status = PrepareLayerStack(buf_handle);
  if (status != 0) {
    DLOGE("PrepareLayerStack failed %d", status);
    return status;
  }

  status = ApplyCurrentColorModeWithRenderIntent();
  if (status != 0) {
      DLOGW("Failed to ApplyCurrentColorModeWithRenderIntent. Error = %d", status);
  }

  if (!validated_) {
    DisplayError error = display_intf_->Prepare(&layer_stack_);
    if (error != kErrorNone) {
      DLOGW("Prepare failed. Error = %d", error);
      return -EINVAL;
    }

    // Cache layer, if demura is configured, demura layer will be created in Prepare() function
    demura_layer_ = nullptr;
    for (auto &layer : layer_stack_.layers) {
      if (layer->flags.is_demura) {
        DLOGI("Demura layer is present");
        demura_layer_ = layer;
      }
    }

    // RC resources available after first cycle so enable skip_validate from 2nd cycle
    if (!first_commit_) {
      validated_ = true;
    }
  } else {
    if (demura_layer_ != nullptr) {
      layer_stack_.layers.push_back(demura_layer_);
    }
  }

  DisplayError error = display_intf_->Commit(&layer_stack_);
  if (error != kErrorNone) {
    DLOGW("Commit failed. Error = %d", error);
    return -EINVAL;
  }
  Layer *layer = layer_stack_.layers.at(0);
  *retire_fence = Fence::Dup(layer_stack_.retire_fence);
  buf_handle->consumer_fence_fd = Fence::Dup(layer->input_buffer.release_fence);
  first_commit_ = false;

  return 0;
}

void SDMCompDisplayBuiltIn::PopulateColorModes() {
  for (uint32_t i = 0; i < stc_mode_list_.list.size(); i++) {
    snapdragoncolor::ColorMode stc_mode = stc_mode_list_.list[i];
    ColorPrimaries gamut = static_cast<ColorPrimaries>(stc_mode.gamut);
    GammaTransfer gamma = static_cast<GammaTransfer>(stc_mode.gamma);
    RenderIntent intent = static_cast<RenderIntent>(stc_mode.intent);
    color_mode_map_[gamut][gamma][intent] = stc_mode;
  }
}

int SDMCompDisplayBuiltIn::SetColorModeWithRenderIntent(struct ColorMode mode) {
  if (mode.gamut < ColorPrimaries_BT709_5 || mode.gamut >= ColorPrimaries_Max) {
    DLOGE("Invalid color primaries %d", mode.gamut);
    return -EINVAL;
  }

  if (mode.gamma < Transfer_sRGB || mode.gamma >= Transfer_Max) {
    DLOGE("Invalid gamma transfer %d", mode.gamma);
    return -EINVAL;
  }

  if (mode.intent >= kRenderIntentMaxRenderIntent ) {
    DLOGE("Invalid intent  %d", mode.intent);
    return -EINVAL;
  }

  if (current_mode_.gamut == mode.gamut && current_mode_.gamma == mode.gamma &&
      current_mode_.intent == mode.intent) {
    return 0;
  }

  current_mode_ = mode;
  apply_mode_ = true;

  Refresh();

  return 0;
}

int SDMCompDisplayBuiltIn::GetStcColorModeFromMap(const ColorMode &mode,
                                                  snapdragoncolor::ColorMode *out_mode) {
  if (!out_mode) {
    DLOGE("Invalid parameter, out_mode is NULL");
    return -EINVAL;
  }

  if (color_mode_map_.find(mode.gamut) == color_mode_map_.end()) {
    DLOGE("Color Primary %d is not supported", mode.gamut);
    return -ENOTSUP;
  }

  if (color_mode_map_[mode.gamut].find(mode.gamma) == color_mode_map_[mode.gamut].end()) {
    DLOGE("Gamma Transfer %d is not supported", mode.gamma);
    return -ENOTSUP;
  }

  auto iter = color_mode_map_[mode.gamut][mode.gamma].find(mode.intent);
  if (iter != color_mode_map_[mode.gamut][mode.gamma].end()) {
    // Found the mode
    *out_mode = iter->second;
    return 0;
  }

  DLOGW("Can't find color mode gamut %d gamma %d intent %d", mode.gamut, mode.gamma, mode.intent);

  return -ENOTSUP;
}

int SDMCompDisplayBuiltIn::ApplyCurrentColorModeWithRenderIntent() {
  if (stc_mode_list_.list.size() < 1) {
    return 0;
  }

  if (!apply_mode_) {
    return 0;
  }

  snapdragoncolor::ColorMode mode;
  int ret = GetStcColorModeFromMap(current_mode_, &mode);
  if (ret) {
    DLOGW("Cannot find mode for current_color_mode_ gamut %d gamma %d intent %d ",
           current_mode_.gamut, current_mode_.gamma, current_mode_.intent);
    return 0;
  }

  DLOGI("Applying Stc mode (gamut %d gamma %d intent %d hw_assets.size %d)",
        mode.gamut, mode.gamma, mode.intent, mode.hw_assets.size());
  DisplayError error = display_intf_->SetStcColorMode(mode);
  if (error != kErrorNone) {
    DLOGE("Failed to apply Stc color mode: gamut %d gamma %d intent %d err %d",
        mode.gamut, mode.gamma, mode.intent, error);
    return 0;
  }

  apply_mode_ = false;
  validated_ = false;

  DLOGV_IF(kTagQDCM, "Successfully applied mode gamut = %d, gamma = %d, intent = %d",
           current_mode_.gamut, current_mode_.gamma, current_mode_.intent);
  return 0;
}

int SDMCompDisplayBuiltIn::GetColorModes(uint32_t *out_num_modes, struct ColorMode *out_modes) {
  if (!out_num_modes || !out_modes) {
    DLOGE("Invalid parameters");
    return -EINVAL;
  }

  auto it = stc_mode_list_.list.begin();
  *out_num_modes = std::min(*out_num_modes, UINT32(stc_mode_list_.list.size()));
  ColorMode mode = {};
  for (uint32_t i = 0; i < *out_num_modes; it++, i++) {
    mode.gamut = static_cast<ColorPrimaries>(it->gamut);
    mode.gamma = static_cast<GammaTransfer>(it->gamma);
    mode.intent = static_cast<RenderIntent>(it->intent);
    out_modes[i] = mode;
  }
  return 0;
}

void SDMCompDisplayBuiltIn::CreateLayerSet() {
  Layer *layer = new Layer();
  layer->flags.updating = 1;

  layer->src_rect = LayerRect(0, 0, variable_info_.x_pixels, variable_info_.y_pixels);
  layer->dst_rect = layer->src_rect;

  layer->input_buffer = {};
  layer->input_buffer.width = variable_info_.x_pixels;
  layer->input_buffer.height = variable_info_.y_pixels;
  layer->input_buffer.unaligned_width = variable_info_.x_pixels;
  layer->input_buffer.unaligned_height = variable_info_.y_pixels;
  layer->input_buffer.format = kFormatInvalid;
  layer->input_buffer.planes[0].fd = -1;
  layer->input_buffer.handle_id = -1;
  layer->frame_rate = variable_info_.fps;
  layer->blending = kBlendingPremultiplied;

  layer_set_.push_back(layer);
}

void SDMCompDisplayBuiltIn::DestroyLayerSet() {
  // Remove any layer if any and clear layer stack
  for (Layer *layer : layer_set_)
      delete layer;
  layer_set_.clear();
}

int SDMCompDisplayBuiltIn::PrepareLayerStack(BufferHandle *buf_handle) {
  if (!buf_handle) {
    DLOGE("buf_handle pointer is null");
    return -EINVAL;
  }

  Layer *layer = layer_set_.at(0);
  layer->input_buffer.planes[0].fd = buf_handle->fd;
  layer->input_buffer.planes[0].stride = buf_handle->stride_in_bytes;
  layer->input_buffer.handle_id = buf_handle->buffer_id;
  layer->input_buffer.buffer_id = buf_handle->buffer_id;

  if (cached_buf_handle_.width == buf_handle->width &&
    cached_buf_handle_.height == buf_handle->height &&
    cached_buf_handle_.aligned_width == buf_handle->aligned_width &&
    cached_buf_handle_.aligned_height == buf_handle->aligned_height &&
    cached_buf_handle_.format == buf_handle->format &&
    cached_buf_handle_.src_crop == buf_handle->src_crop) {
    layer_stack_.layers.clear();
    for (auto &it : layer_set_)
      layer_stack_.layers.push_back(it);
    return 0;
  }

  BufferFormat buf_format = GetSDMCompFormat(layer->input_buffer.format);
  LayerRect src_crop = LayerRect(buf_handle->src_crop.left, buf_handle->src_crop.top,
                                 buf_handle->src_crop.right, buf_handle->src_crop.bottom);

  layer->input_buffer.width = buf_handle->width;
  layer->input_buffer.height = buf_handle->height;
  layer->input_buffer.unaligned_width = buf_handle->width;
  layer->input_buffer.unaligned_height = buf_handle->height;
  layer->input_buffer.format = GetSDMFormat(buf_handle->format);
  layer->frame_rate = variable_info_.fps;
  layer->blending = kBlendingPremultiplied;
  layer->src_rect = LayerRect(0, 0, buf_handle->width, buf_handle->height);
  if (IsValid(src_crop)) {
    layer->src_rect = Intersection(src_crop, layer->src_rect);
  }
  layer->dst_rect.right = (float)mixer_config_.width;
  layer->dst_rect.bottom = (float)mixer_config_.height;

  DLOGI("WxHxF %dx%dx%d Crop[LTRB] [%.0f %.0f %.0f %.0f]", layer->input_buffer.width,
        layer->input_buffer.height, layer->input_buffer.format, layer->src_rect.left,
        layer->src_rect.top, layer->src_rect.right, layer->src_rect.bottom);

  layer_stack_.layers.clear();
  validated_ = false;
  for (auto &it : layer_set_)
    layer_stack_.layers.push_back(it);

  cached_buf_handle_.width = buf_handle->width;
  cached_buf_handle_.height = buf_handle->height;
  cached_buf_handle_.aligned_width = buf_handle->aligned_width;
  cached_buf_handle_.aligned_height = buf_handle->aligned_height;
  cached_buf_handle_.format = buf_handle->format;
  cached_buf_handle_.src_crop = buf_handle->src_crop;

  return 0;
}

DisplayError SDMCompDisplayBuiltIn::HandleEvent(DisplayEvent event) {
  DLOGI("Received display event %d", event);
  switch (event) {
    case kDisplayPowerResetEvent:
    case kPanelDeadEvent:
      if (callback_) {
        callback_->OnError();
      }
      break;
    default:
      break;
  }
  return kErrorNone;
}

int SDMCompDisplayBuiltIn::SetPanelBrightness(float brightness_level) {
  if (brightness_level < min_panel_brightness_) {
    DLOGE("brightness level is invalid!! brightness_level %f, min_panel_brightness %f",
          brightness_level, min_panel_brightness_);
    return -EINVAL;
  }
  // if min_panel_brightness is not set, then set panel_brightness value as min_panel_brightness
  if (min_panel_brightness_ == 0.0f) {
    min_panel_brightness_ = brightness_level;
  }

  DisplayError err = display_intf_->SetPanelBrightness(brightness_level);
  if (err != kErrorNone) {
    return -EINVAL;
  }
  return 0;
}

int SDMCompDisplayBuiltIn::SetMinPanelBrightness(float min_brightness) {
  if (min_brightness < 0.0f || min_brightness > 1.0) {
    DLOGE("Invalid min brightness settings %f", min_brightness);
    return -EINVAL;
  }
  min_panel_brightness_ = min_brightness;
  return 0;
}

}  // namespace sdm
