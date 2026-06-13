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
* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <utils/constants.h>
#include <utils/debug.h>
#include <private/hw_interface.h>
#include <private/hw_info_interface.h>
#include <algorithm>
#include <vector>
#include <utility>
#include "display_virtual.h"

#define __CLASS__ "DisplayVirtual"

namespace sdm {

DisplayVirtual::DisplayVirtual(DisplayEventHandler *event_handler,
                               sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                               BufferAllocator *buffer_allocator, CompManager *comp_manager)
    : DisplayBase(kVirtual, event_handler, kDeviceVirtual, buffer_allocator, comp_manager,
                  hw_info_intf) {}

DisplayVirtual::DisplayVirtual(DisplayId display_id, DisplayEventHandler *event_handler,
                               sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                               BufferAllocator *buffer_allocator, CompManager *comp_manager)
    : DisplayBase(display_id, kVirtual, event_handler, kDeviceVirtual, buffer_allocator,
                  comp_manager, hw_info_intf) {}

DisplayError DisplayVirtual::Init() {
  ClientLock lock(disp_mutex_);

  DisplayError error = comp_manager_->AllocateVirtualDisplayId(&display_id_);
  if (error != kErrorNone) {
    return error;
  }

  display_id_info_ = DisplayId(display_id_);
  error = DPUCoreFactory::Create(display_id_info_, kVirtual, hw_info_intf_, buffer_allocator_,
                                 &dpu_core_mux_);
  if (error != kErrorNone) {
    return error;
  }

  dpu_core_mux_->GetHWInterface(&hw_intf_);

  if (-1 == display_id_info_.GetDisplayId()) {
    dpu_core_mux_->GetDisplayId(&display_id_);
    display_id_info_ = DisplayId(display_id_);
  }

  core_id_ = display_id_info_.GetCoreIdMap();
  std::bitset<32> core_id_bitset = std::bitset<32>(core_id_);
  core_count_ = core_id_bitset.count();

  for (int i = 0; i < core_id_.size(); i++) {
    if (!core_id_[i]) {
      continue;
    }
    default_clock_hz_.insert(std::pair<uint32_t, uint32_t>(i, 0));
    cached_framebuffer_.insert(std::pair<uint32_t, LayerBuffer>(i, {}));
    cached_qos_data_.insert(std::pair<uint32_t, HWQosData>(i, {}));
    disp_layer_stack_->info.insert(std::pair<uint32_t, HWLayersInfo>(i, {}));
  }

  for (auto info_intf = hw_info_intf_.Begin(); info_intf != hw_info_intf_.End(); info_intf++) {
    HWResourceInfo hw_resource_info = HWResourceInfo();
    info_intf->second->GetHWResourceInfo(&hw_resource_info);
    hw_resource_info_.push_back(hw_resource_info);
  }

  uint32_t max_mixer_stages = INT_MAX;

  for (auto& res_info : hw_resource_info_) {
    max_mixer_stages = std::min(max_mixer_stages, res_info.num_blending_stages);
  }

  int property_value = Debug::GetMaxPipesPerMixer(display_type_);
  if (property_value >= 0) {
    max_mixer_stages = std::min(UINT32(property_value), max_mixer_stages);
  }
  DisplayBase::SetMaxMixerStages(max_mixer_stages);

  return error;
}

DisplayError DisplayVirtual::Deinit() {
  auto error = DisplayBase::Deinit();
  if (display_id_ != -1) {
    comp_manager_->DeallocateVirtualDisplayId(display_id_);
  }
  return error;
}

DisplayError DisplayVirtual::GetNumVariableInfoConfigs(uint32_t *count) {
  ClientLock lock(disp_mutex_);
  *count = 1;
  return kErrorNone;
}

DisplayError DisplayVirtual::GetConfig(uint32_t index, DisplayConfigVariableInfo *variable_info) {
  ClientLock lock(disp_mutex_);
  *variable_info = client_ctx_.display_attributes;
  return kErrorNone;
}

DisplayError DisplayVirtual::GetActiveConfig(uint32_t *index) {
  ClientLock lock(disp_mutex_);
  *index = 0;
  return kErrorNone;
}

DisplayError DisplayVirtual::SetActiveConfig(DisplayConfigVariableInfo *variable_info) {
  ClientLock lock(disp_mutex_);

  if (!variable_info) {
    return kErrorParameters;
  }

  DisplayError error = kErrorNone;
  DisplayClientContext client_ctx = {};
  DisplayDeviceContext device_ctx;
  client_ctx = client_ctx_;
  device_ctx = device_ctx_;

  client_ctx.display_attributes.x_pixels = variable_info->x_pixels;
  client_ctx.display_attributes.y_pixels = variable_info->y_pixels;
  client_ctx.display_attributes.fps = variable_info->fps;

  if (client_ctx.display_attributes == client_ctx_.display_attributes) {
    return kErrorNone;
  }

  error = dpu_core_mux_->SetDisplayAttributes(client_ctx.display_attributes);
  if (error != kErrorNone) {
    return error;
  }

  uint32_t active_index = 0;
  dpu_core_mux_->GetActiveConfig(&active_index);
  dpu_core_mux_->GetDisplayAttributes(active_index, &device_ctx, &client_ctx);
  dpu_core_mux_->GetHWPanelInfo(&device_ctx, &client_ctx);

  if (set_max_lum_ != -1.0 || set_min_lum_ != -1.0) {
    client_ctx.hw_panel_info.peak_luminance = set_max_lum_;
    client_ctx.hw_panel_info.blackness_level = set_min_lum_;
    DLOGI("set peak_luminance %f blackness_level %f for display %d-%d", display_id_,
          display_type_, client_ctx.hw_panel_info.peak_luminance,
          client_ctx.hw_panel_info.blackness_level);
  }

  error = dpu_core_mux_->GetMixerAttributes(&device_ctx, &client_ctx);
  if (error != kErrorNone) {
    return error;
  }

  // fb_config will be updated only once after creation of virtual display
  if (client_ctx.fb_config.x_pixels == 0 || client_ctx.fb_config.y_pixels == 0) {
    error = dpu_core_mux_->GetFbConfig(client_ctx.display_attributes.x_pixels,
                                       client_ctx.display_attributes.y_pixels,
                                       &device_ctx, &client_ctx);
      if (error != kErrorNone) {
        return error;
      }
  }

  // if display is already connected, reconfigure the display with new configuration.
  if (!display_comp_ctx_) {
    error = comp_manager_->RegisterDisplay(display_id_info_, display_type_, device_ctx, client_ctx,
                                           &display_comp_ctx_, &cached_qos_data_, this);
  } else {
    error = comp_manager_->ReconfigureDisplay(display_comp_ctx_, device_ctx, client_ctx,
                                              &cached_qos_data_);
  }
  if (error != kErrorNone) {
    return error;
  }

  for (auto& qos_data : cached_qos_data_) {
    default_clock_hz_.at(qos_data.first) = qos_data.second.clock_hz;
  }

  client_ctx_ = client_ctx;
  device_ctx_ = device_ctx;

  DLOGI("Virtual display %d-%d resolution changed to [%dx%d]", display_id_,
        display_type_, client_ctx_.display_attributes.x_pixels,
        client_ctx_.display_attributes.y_pixels);

  return kErrorNone;
}

DisplayError DisplayVirtual::Prepare(LayerStack *layer_stack) {
  ClientLock lock(disp_mutex_);

  DisplayError error = PrePrepare(layer_stack);
  if (error == kErrorNone) {
    return error;
  }

  if (error == kErrorNeedsLutRegen && (ForceToneMapUpdate(layer_stack) == kErrorNone)) {
    return kErrorNone;
  }

  return DisplayBase::Prepare(layer_stack);
}

DisplayError DisplayVirtual::GetColorModeCount(uint32_t *mode_count) {
  ClientLock lock(disp_mutex_);

  // Color Manager isn't supported for virtual displays.
  *mode_count = 1;

  return kErrorNone;
}

DisplayError DisplayVirtual::SetPanelLuminanceAttributes(float min_lum, float max_lum) {
  set_max_lum_ = max_lum;
  set_min_lum_ = min_lum;
  return kErrorNone;
}

DisplayError DisplayVirtual::colorSamplingOn() {
    return kErrorNone;
}

DisplayError DisplayVirtual::colorSamplingOff() {
    return kErrorNone;
}

}  // namespace sdm

