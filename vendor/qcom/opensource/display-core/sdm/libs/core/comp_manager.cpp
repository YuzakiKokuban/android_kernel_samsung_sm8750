/*
* Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
* ​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <core/buffer_allocator.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <utility>

#include "comp_manager.h"
#include "strategy.h"

#define __CLASS__ "CompManager"

namespace sdm {

DisplayError CompManager::Init(const std::vector<HWResourceInfo> &hw_res_info,
                               ExtensionInterface *extension_intf,
                               BufferAllocator *buffer_allocator,
                               SocketHandler *socket_handler) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayError error = kErrorNone;

  if (extension_intf) {
    extension_intf->CreateCwbManagerExtn(this, hw_res_info, &cwb_mgr_intf_);
    error = extension_intf->CreateResourceExtn(hw_res_info, buffer_allocator, &resource_intf_);
    extension_intf->CreateDppsControlExtn(&dpps_ctrl_intf_, socket_handler);
    extension_intf->CreateCapabilitiesExtn(hw_res_info[0], &cap_intf_);
  } else {
    error = ResourceDefault::CreateResourceDefault(hw_res_info, &resource_intf_);
  }

  if (error != kErrorNone) {
    if (extension_intf) {
      extension_intf->DestroyDppsControlExtn(dpps_ctrl_intf_);
      extension_intf->DestroyCapabilitiesExtn(cap_intf_);
      extension_intf->DestroyCwbManagerExtn(cwb_mgr_intf_);
    }
    return error;
  }

  hw_res_info_ = hw_res_info;
  buffer_allocator_ = buffer_allocator;
  extension_intf_ = extension_intf;

  return error;
}

DisplayError CompManager::Deinit() {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  if (extension_intf_) {
    extension_intf_->DestroyResourceExtn(resource_intf_);
    extension_intf_->DestroyDppsControlExtn(dpps_ctrl_intf_);
    extension_intf_->DestroyCapabilitiesExtn(cap_intf_);
    extension_intf_->DestroyCwbManagerExtn(cwb_mgr_intf_);
  } else {
    ResourceDefault::DestroyResourceDefault(resource_intf_);
  }

  return kErrorNone;
}

DisplayError CompManager::RegisterDisplay(DisplayId display_id, SDMDisplayType type,
                                          DisplayDeviceContext &device_ctx,
                                          DisplayClientContext &client_ctx,
                                          Handle *display_ctx,
                                          std::map<uint32_t, HWQosData> *default_qos_data,
                                          CompManagerEventHandler *event_handler) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayError error = kErrorNone;

  DisplayCompositionContext *display_comp_ctx = new DisplayCompositionContext();
  if (!display_comp_ctx) {
    return kErrorMemory;
  }

  Strategy *&strategy = display_comp_ctx->strategy;
  strategy = new Strategy(extension_intf_, buffer_allocator_, display_id, type,
                          hw_res_info_, client_ctx, device_ctx);
  if (!strategy) {
    DLOGE("Unable to create strategy for display %d-%d", display_id.GetDisplayId(), type);
    delete display_comp_ctx;
    return kErrorMemory;
  }

  error = strategy->Init();
  if (error != kErrorNone) {
    delete strategy;
    delete display_comp_ctx;
    return error;
  }

  error = resource_intf_->RegisterDisplay(display_id, type, device_ctx, client_ctx,
                                          &display_comp_ctx->display_resource_ctx);
  if (error != kErrorNone) {
    strategy->Deinit();
    delete strategy;
    delete display_comp_ctx;
    display_comp_ctx = NULL;
    return error;
  }

  error = resource_intf_->GetDefaultQoSData(display_comp_ctx->display_resource_ctx,
                                            default_qos_data);
  if (error != kErrorNone) {
    strategy->Deinit();
    delete strategy;
    resource_intf_->UnregisterDisplay(display_comp_ctx->display_resource_ctx);
    delete display_comp_ctx;
    display_comp_ctx = NULL;
    return error;
  }

  error = resource_intf_->Perform(ResourceInterface::kCmdDedicatePipes,
                                  display_comp_ctx->display_resource_ctx);
  if (error != kErrorNone) {
    strategy->Deinit();
    delete strategy;
    resource_intf_->UnregisterDisplay(display_comp_ctx->display_resource_ctx);
    delete display_comp_ctx;
    display_comp_ctx = NULL;
    return error;
  }

  registered_displays_.insert(display_id.GetDisplayId());
  callback_map_[display_id.GetDisplayId()] = event_handler;
  display_comp_ctx->is_primary_panel = client_ctx.hw_panel_info.is_primary_panel;
  display_comp_ctx->display_id = display_id;
  display_comp_ctx->display_type = type;
  display_comp_ctx->fb_config = client_ctx.fb_config;
  display_comp_ctx->dest_scaler_blocks_used = client_ctx.mixer_attributes.dest_scaler_blocks_used;
  *display_ctx = display_comp_ctx;
  // New non-primary display device has been added, so move the composition mode to safe mode until
  // resources for the added display is configured properly.
  if (!display_comp_ctx->is_primary_panel) {
    max_sde_secondary_fetch_layers_ = UINT32(Debug::GetSecondaryMaxFetchLayers());
  }

  display_demura_status_[display_id.GetDisplayId()] = false;

  DLOGV_IF(kTagCompManager, "Registered displays [%s], display %d-%d",
           StringDisplayList(registered_displays_).c_str(),
                             display_comp_ctx->display_id.GetDisplayId(),
           display_comp_ctx->display_type);

  return kErrorNone;
}

DisplayError CompManager::UnregisterDisplay(Handle display_ctx) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (!display_comp_ctx) {
    return kErrorParameters;
  }

  resource_intf_->UnregisterDisplay(display_comp_ctx->display_resource_ctx);

  Strategy *&strategy = display_comp_ctx->strategy;
  strategy->Deinit();
  delete strategy;

  callback_map_.erase(display_comp_ctx->display_id.GetDisplayId());
  registered_displays_.erase(display_comp_ctx->display_id.GetDisplayId());
  powered_on_displays_.erase(display_comp_ctx->display_id.GetDisplayId());

  DLOGV_IF(kTagCompManager, "Registered displays [%s], display %d-%d",
           StringDisplayList(registered_displays_).c_str(),
           display_comp_ctx->display_id.GetDisplayId(),
           display_comp_ctx->display_type);

  delete display_comp_ctx;
  display_comp_ctx = NULL;
  return kErrorNone;
}

DisplayError CompManager::CheckEnforceSplit(Handle comp_handle,
                                            uint32_t new_refresh_rate) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayError error = kErrorNone;
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(comp_handle);

  error = resource_intf_->Perform(ResourceInterface::kCmdCheckEnforceSplit,
                                  display_comp_ctx->display_resource_ctx, new_refresh_rate);
  return error;
}

DisplayError CompManager::ReconfigureDisplay(Handle comp_handle,
                                             DisplayDeviceContext &device_ctx,
                                             DisplayClientContext &client_ctx,
                                             std::map<uint32_t, HWQosData> *default_qos_data) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DTRACE_SCOPED();

  DisplayError error = kErrorNone;
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(comp_handle);

  error = resource_intf_->ReconfigureDisplay(display_comp_ctx->display_resource_ctx,
                                             device_ctx, client_ctx);
  if (error != kErrorNone) {
    DLOGW("ReconfigureDisplay on display %d-%d returned error=%d",
          display_comp_ctx->display_id.GetDisplayId(), display_comp_ctx->display_type, error);
    return error;
  }

  error = resource_intf_->GetDefaultQoSData(display_comp_ctx->display_resource_ctx,
                                            default_qos_data);
  if (error != kErrorNone) {
    DLOGW("GetDefaultQosData Data on display %d-%d returned error=%d",
          display_comp_ctx->display_id.GetDisplayId(), display_comp_ctx->display_type, error);
    return error;
  }

  error = resource_intf_->Perform(ResourceInterface::kCmdCheckEnforceSplit,
                                  display_comp_ctx->display_resource_ctx,
                                  client_ctx.display_attributes.fps);
  if (error != kErrorNone) {
    DLOGW("CheckEnforceSplit returned error=%d", error);
    return error;
  }

  if (display_comp_ctx->strategy) {
    error = display_comp_ctx->strategy->Reconfigure(client_ctx, device_ctx);
    if (error != kErrorNone) {
      DLOGE("Unable to Reconfigure strategy on display %d-%d.",
             display_comp_ctx->display_id.GetDisplayId(), display_comp_ctx->display_type);
      display_comp_ctx->strategy->Deinit();
      delete display_comp_ctx->strategy;
      display_comp_ctx->strategy = NULL;
      return error;
    }
  }

  // Update new resolution.
  display_comp_ctx->fb_config = client_ctx.fb_config;
  return error;
}

void CompManager::PrepareStrategyConstraints(Handle comp_handle,
                                             DispLayerStack *disp_layer_stack) {
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(comp_handle);
  StrategyConstraints *constraints = &display_comp_ctx->constraints;
  Handle &display_resource_ctx = display_comp_ctx->display_resource_ctx;

  // Call Layer Precheck to get feedback
  LayerFeedback feedback(disp_layer_stack->stack_info.app_layer_count);
  if (resource_intf_)
    resource_intf_->Precheck(display_resource_ctx, disp_layer_stack, &feedback);

  constraints->safe_mode = safe_mode_;
  uint32_t num_blending_stages = INT_MAX;
  uint32_t num_vig_pipe = INT_MAX;
  uint32_t num_dma_pipe = INT_MAX;
  uint32_t num_rgb_pipe = INT_MAX;
  bool separate_rotator = true;

  std::bitset<8> core_id_map = display_comp_ctx->display_id.GetCoreIdMap();
  for (auto& res_info : hw_res_info_) {
    if (!core_id_map[res_info.core_id]) {
      continue;
    }
    num_blending_stages = std::min(num_blending_stages, res_info.num_blending_stages);
    num_vig_pipe = std::min(num_vig_pipe, res_info.num_vig_pipe);
    num_dma_pipe = std::min(num_dma_pipe, res_info.num_dma_pipe);
    num_rgb_pipe = std::min(num_rgb_pipe, res_info.num_rgb_pipe);
    separate_rotator &= res_info.separate_rotator;
  }

  constraints->max_layers = num_blending_stages;
  constraints->feedback = feedback;

  // Limit 2 layer SDE Comp if its not a Primary Display.
  // Safe mode is the policy for External display on a low end device.
  if (!display_comp_ctx->is_primary_panel) {
    bool low_end_hw = ((num_vig_pipe + num_rgb_pipe +
                        num_dma_pipe) <= kSafeModeThreshold);
    constraints->max_layers = display_comp_ctx->display_type == kBuiltIn ?
                              max_sde_builtin_fetch_layers_ : max_sde_secondary_fetch_layers_;
    constraints->safe_mode = (low_end_hw && !separate_rotator) ? true : safe_mode_;
  }

  // If a strategy fails after successfully allocating resources, then set safe mode
  if (display_comp_ctx->remaining_strategies != display_comp_ctx->max_strategies) {
    constraints->safe_mode = true;
  }

  if (secure_event_ == kTUITransitionStart) {
    constraints->max_layers = 1;
  }

  uint32_t size_ff = 1;  // gpu target layer always present
  if (disp_layer_stack->stack_info.stitch_present)
    size_ff++;
  if (disp_layer_stack->stack_info.demura_present)
    size_ff++;
  if (disp_layer_stack->stack_info.cwb_present)
    size_ff++;
  uint32_t app_layer_count = UINT32(disp_layer_stack->stack->layers.size()) - size_ff;
  if (display_comp_ctx->idle_fallback) {
    // Handle the GPU based idle timeout by falling back
    constraints->safe_mode = true;
  }

  // Avoid safe mode, if there is only one app layer.
  if (app_layer_count == 1) {
     constraints->safe_mode = false;
  }
}

void CompManager::GenerateROI(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *disp_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  return disp_comp_ctx->strategy->GenerateROI(disp_layer_stack, disp_comp_ctx->pu_constraints);
}

DisplayError CompManager::HandleQosValidation(Handle display_ctx,
                                              DispLayerStack *disp_layer_stack,
                                              DisplayError error) {
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  // CWB info must be updated to HWLayersInfo of display layer stack before QoS validation.
  for (auto &info : disp_layer_stack->info) {
    // TODO (user): Need to split CWB buffer and config for each core, when CWB supports
    // for dual DPU.
    info.second.output_buffer = disp_layer_stack->stack->output_buffer;
    info.second.hw_cwb_config = disp_layer_stack->stack_info.hw_cwb_config;
  }

  // Revalidate/recalculate QoS, if it needs as per returned error code.
  if (error == kErrorNeedsQosRecalc ||
      error == kErrorNeedsQosRecalcAndLutRegen) {
    auto err = resource_intf_->ValidateQoS(
        display_comp_ctx->display_resource_ctx, disp_layer_stack);
    if (err == kErrorNone) {
      if (error == kErrorNeedsQosRecalcAndLutRegen) {
        // Here, QoS is validated, but still tone-map LUT regeneration is pending.
        // So, update return error for LUT regeneration during further process.
        error = kErrorNeedsLutRegen;
      } else {
        error = kErrorNone;
      }
    } else {
      // Reset strategy internal parameters for full validation and get max
      // strategies available.
      display_comp_ctx->strategy->ResetStrategy(&display_comp_ctx->max_strategies);
      display_comp_ctx->remaining_strategies = display_comp_ctx->max_strategies;
      error = kErrorNeedsValidate;
    }
  }

  return error;
}

DisplayError CompManager::PrePrepare(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (display_comp_ctx->idle_fallback) {
    display_comp_ctx->constraints.idle_timeout = true;
  }

  display_comp_ctx->constraints.tonemapping_query_mandatory =
        resource_intf_->ToneMapQueryRequested(display_comp_ctx->display_resource_ctx);

  StrategyConstraints *constraints = &display_comp_ctx->constraints;
  Handle &display_resource_ctx = display_comp_ctx->display_resource_ctx;
  if (resource_intf_) {
    resource_intf_->UpdateWBstatus(display_resource_ctx, &constraints->feedback);
  }

  DisplayError error = display_comp_ctx->strategy->Start(disp_layer_stack,
                                                         &display_comp_ctx->max_strategies,
                                                         &display_comp_ctx->constraints);
  display_comp_ctx->remaining_strategies = display_comp_ctx->max_strategies;

  error = HandleQosValidation(display_comp_ctx, disp_layer_stack, error);

  resource_intf_->Perform(ResourceInterface::kCmdSetCacMode, display_comp_ctx->display_resource_ctx,
                          &disp_layer_stack->stack_info.enable_cac);

  // Select a composition strategy, and try to allocate resources for it.
  resource_intf_->Start(display_comp_ctx->display_resource_ctx, disp_layer_stack->stack);

  if (error == kErrorNone || error == kErrorNeedsLutRegen) {
    resource_intf_->HandleSkipValidate(display_comp_ctx->display_resource_ctx);
  }

  return error;
}

DisplayError CompManager::Prepare(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DTRACE_SCOPED();
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  Handle &display_resource_ctx = display_comp_ctx->display_resource_ctx;
  DisplayError error = kErrorUndefined;

  PrepareStrategyConstraints(display_ctx, disp_layer_stack);

  resource_intf_->Perform(ResourceInterface::kCmdSetCacMode, display_comp_ctx->display_resource_ctx,
                          &disp_layer_stack->stack_info.enable_cac);

  // Select a composition strategy, and try to allocate resources for it.
  resource_intf_->Start(display_resource_ctx, disp_layer_stack->stack);

  bool exit = false;
  uint32_t &count = display_comp_ctx->remaining_strategies;
  for (; !exit && count > 0; count--) {
    error = display_comp_ctx->strategy->GetNextStrategy();
    if (error != kErrorNone) {
      // Composition strategies exhausted. Resource Manager could not allocate resources even for
      // GPU composition. This will never happen.
      exit = true;
    }

    if (!exit) {
      LayerFeedback updated_feedback(disp_layer_stack->stack_info.app_layer_count);
      error = resource_intf_->Prepare(display_resource_ctx, disp_layer_stack, &updated_feedback);
      // Exit if successfully prepared resource, else try next strategy.
      exit = (error == kErrorNone);
      display_comp_ctx->constraints.feedback = updated_feedback;
    }
  }

  if (error != kErrorNone) {
    resource_intf_->Stop(display_resource_ctx, disp_layer_stack);
    DLOGE("Composition strategies exhausted for display = %d-%d. (first frame = %s)",
          display_comp_ctx->display_id.GetDisplayId(), display_comp_ctx->display_type,
          display_comp_ctx->first_cycle_ ? "True" : "False");
    return error;
  }

  return error;
}

DisplayError CompManager::PostPrepare(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  Handle &display_resource_ctx = display_comp_ctx->display_resource_ctx;

  DisplayError error = kErrorNone;

  display_comp_ctx->strategy->Stop();

  error = resource_intf_->Stop(display_resource_ctx, disp_layer_stack);
  if (error != kErrorNone) {
    DLOGE("Resource stop failed for display %d-%d", display_comp_ctx->display_id.GetDisplayId(),
          display_comp_ctx->display_type);
  }

  error = resource_intf_->PostPrepare(display_resource_ctx, disp_layer_stack);
  if (error != kErrorNone) {
    return error;
  }

  return kErrorNone;
}

DisplayError CompManager::Commit(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  DisplayError error = resource_intf_->Commit(display_comp_ctx->display_resource_ctx,
                                              disp_layer_stack);
  if (error != kErrorNone) {
    return error;
  }
  std::map<uint32_t, HWQosData> default_qos_data;
  if (secure_event_ == kTUITransitionStart) {
    error = resource_intf_->GetDefaultQoSData(display_comp_ctx->display_resource_ctx,
                                              &default_qos_data);
    if (error != kErrorNone) {
      return error;
    }

    for (auto& info : disp_layer_stack->info) {
      info.second.qos_data = default_qos_data[info.first];
    }
  }
  return kErrorNone;
}

DisplayError CompManager::PostCommit(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayError error = kErrorNone;
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  error = resource_intf_->PostCommit(display_comp_ctx->display_resource_ctx, disp_layer_stack);
  if (error != kErrorNone) {
    return error;
  }

  display_comp_ctx->idle_fallback = false;
  display_comp_ctx->first_cycle_ = false;
  display_comp_ctx->constraints.idle_timeout = false;
  display_comp_ctx->constraints.gpu_fallback_mode = false;

  DLOGV_IF(kTagCompManager, "Registered displays [%s], display %d-%d",
           StringDisplayList(registered_displays_).c_str(),
           display_comp_ctx->display_id.GetDisplayId(),
           display_comp_ctx->display_type);

  return kErrorNone;
}

void CompManager::Purge(Handle display_ctx) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  resource_intf_->Purge(display_comp_ctx->display_resource_ctx);

  display_comp_ctx->strategy->Purge();
}

DisplayError CompManager::SetIdleTimeoutMs(Handle display_ctx, uint32_t active_ms,
                                           uint32_t inactive_ms) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return display_comp_ctx->strategy->SetIdleTimeoutMs(active_ms, inactive_ms);
}

void CompManager::ProcessIdleTimeout(Handle display_ctx) {
  DTRACE_SCOPED();
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (!display_comp_ctx) {
    return;
  }

  display_comp_ctx->idle_fallback = true;
}

void CompManager::DoGpuFallback(Handle display_ctx) {
  DTRACE_SCOPED();
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (display_comp_ctx) {
    display_comp_ctx->constraints.gpu_fallback_mode = true;
  }
}

void CompManager::ProcessIdlePowerCollapse(Handle display_ctx) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
          reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (display_comp_ctx) {
    resource_intf_->Perform(ResourceInterface::kCmdResetLUT,
                            display_comp_ctx->display_resource_ctx);
  }
}

DisplayError CompManager::SetMaxMixerStages(Handle display_ctx, uint32_t max_mixer_stages) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayError error = kErrorNone;
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (display_comp_ctx) {
    error = resource_intf_->SetMaxMixerStages(display_comp_ctx->display_resource_ctx,
                                              max_mixer_stages);
  }

  return error;
}

DisplayError CompManager::GetHDRCapability(bool *hdr_plus_support, bool *dolby_vision_supported) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayError error = kErrorNone;
  if (cap_intf_) {
    DLOGD_IF(kTagCompManager, "Attempting to get HDR10+ capability");
    error = cap_intf_->GetCapability(kHDR10PlusCapability, hdr_plus_support);
    if (error != kErrorNone) {
      DLOGW("Failed to get HDR10+ capability");
    } else {
      error = cap_intf_->GetCapability(kDolbyVisionCapability, dolby_vision_supported);
      if (error != kErrorNone) {
        DLOGW("Failed to get Dolby vision capability");
      }
    }
  } else {
    DLOGW("Failed to get HDR capabilities");
  }

  return error;
}

void CompManager::ControlPartialUpdate(Handle display_ctx, bool enable) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  display_comp_ctx->pu_constraints.enable = enable;
}

DisplayError CompManager::ValidateScaling(const LayerRect &crop, const LayerRect &dst,
                                          bool rotate90) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  BufferLayout layout = Debug::IsUbwcTiledFrameBuffer() ? kUBWC : kLinear;
  return resource_intf_->ValidateScaling(crop, dst, rotate90, layout, true);
}

DisplayError CompManager::ValidateAndSetCursorPosition(Handle display_ctx,
                                                       DispLayerStack *disp_layer_stack,
                                                       int x, int y) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  Handle &display_resource_ctx = display_comp_ctx->display_resource_ctx;
  return resource_intf_->ValidateAndSetCursorPosition(display_resource_ctx, disp_layer_stack, x, y,
                                                      &display_comp_ctx->fb_config);
}

DisplayError CompManager::SetMaxBandwidthMode(HWBwModes mode) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (mode >= kBwModeMax) {
    return kErrorNotSupported;
  }

  return resource_intf_->SetMaxBandwidthMode(mode);
}

DisplayError CompManager::GetScaleLutConfig(HWScaleLutInfo *lut_info) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return resource_intf_->GetScaleLutConfig(lut_info);
}

DisplayError CompManager::SetDetailEnhancerData(Handle display_ctx,
                                                const DisplayDetailEnhancerData &de_data) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (!display_comp_ctx->dest_scaler_blocks_used) {
    return kErrorResources;
  }

  display_comp_ctx->strategy->SetDetailEnhancerData(de_data);
  return resource_intf_->SetDetailEnhancerData(display_comp_ctx->display_resource_ctx, de_data);
}

DisplayError CompManager::SetCompositionState(Handle display_ctx,
                                              LayerComposition composition_type, bool enable) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return display_comp_ctx->strategy->SetCompositionState(composition_type, enable);
}

DisplayError CompManager::ControlDpps(bool enable) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  // DPPS feature and HDR using SSPP tone mapping can co-exist
  // DPPS feature and HDR using DSPP tone mapping are mutually exclusive
  bool src_tone_map = true;
  for (auto& val : hw_res_info_) {
    src_tone_map = src_tone_map & val.src_tone_map.any();
  }

  if (dpps_ctrl_intf_ && !src_tone_map) {
    int err = 0;
    if (enable) {
      err = dpps_ctrl_intf_->On();
    } else {
      err = dpps_ctrl_intf_->Off();
    }
    if (err) {
      return kErrorUndefined;
    }
  }

  return kErrorNone;
}

uint32_t CompManager::GetActiveDisplayCount() {
  return powered_on_displays_.size();
}

bool CompManager::SetDisplayState(Handle display_ctx, DisplayState state,
                                  const SyncPoints &sync_points) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  resource_intf_->Perform(ResourceInterface::kCmdSetDisplayState,
                          display_comp_ctx->display_resource_ctx, state);

  switch (state) {
  case kStateOff:
    Purge(display_ctx);
    powered_on_displays_.erase(display_comp_ctx->display_id.GetDisplayId());
    break;

  case kStateOn:
  case kStateDoze:
    resource_intf_->Perform(ResourceInterface::kCmdDedicatePipes,
                            display_comp_ctx->display_resource_ctx);
    powered_on_displays_.insert(display_comp_ctx->display_id.GetDisplayId());
    break;

  case kStateDozeSuspend:
    powered_on_displays_.erase(display_comp_ctx->display_id.GetDisplayId());
    break;

  default:
    break;
  }

  bool inactive = (state == kStateOff) || (state == kStateDozeSuspend);
  UpdateStrategyConstraints(display_comp_ctx->is_primary_panel, inactive);

  resource_intf_->UpdateSyncHandle(display_comp_ctx->display_resource_ctx, sync_points);

  return true;
}

DisplayError CompManager::SetColorModesInfo(Handle display_ctx,
                                            const std::vector<PrimariesTransfer> &colormodes_cs) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  display_comp_ctx->strategy->SetColorModesInfo(colormodes_cs);

  return kErrorNone;
}

std::string CompManager::StringDisplayList(const std::set<int32_t> &displays) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  std::string displays_str;
  for (auto disps : displays) {
    if (displays_str.empty()) {
      displays_str = std::to_string(disps);
    } else {
      displays_str += ", " + std::to_string(disps);
    }
  }
  return displays_str;
}

DisplayError CompManager::SetBlendSpace(Handle display_ctx, const PrimariesTransfer &blend_space) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  display_comp_ctx->strategy->SetBlendSpace(blend_space);

  resource_intf_->SetBlendSpace(display_comp_ctx->display_resource_ctx, blend_space);

  return kErrorNone;
}

void CompManager::HandleSecureEvent(Handle display_ctx, SecureEvent secure_event) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  // Disable rotator for non secure layers at the end of secure display session, because scm call
  // has been made to end secure display session during the display commit. Since then access to
  // non secure memory is unavailable. So this results in smmu page fault when rotator tries to
  // access the non secure memory.
  if (secure_event == kSecureDisplayEnd) {
    resource_intf_->Perform(ResourceInterface::kCmdDisableRotatorOneFrame,
                            display_comp_ctx->display_resource_ctx);
  }
  if (secure_event == kTUITransitionStart) {
    resource_intf_->HandleTUITransition(display_comp_ctx->display_resource_ctx, true);
  }
  if (secure_event == kTUITransitionEnd) {
    resource_intf_->Perform(ResourceInterface::kCmdResetLUT,
                            display_comp_ctx->display_resource_ctx);
    resource_intf_->HandleTUITransition(display_comp_ctx->display_resource_ctx, false);
    safe_mode_ = false;
  }
  safe_mode_ = (secure_event == kTUITransitionStart) ? true : safe_mode_;
  secure_event_ = secure_event;
}

void CompManager::PostHandleSecureEvent(Handle display_ctx, SecureEvent secure_event) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  if (secure_event == kSecureDisplayEnd) {
    resource_intf_->HandleTUITransition(display_comp_ctx->display_resource_ctx, false);
    secure_event_ = kSecureEventMax;
  }
}

void CompManager::UpdateStrategyConstraints(bool is_primary, bool disabled) {
  if (!is_primary) {
    return;
  }

  // Allow builtin display to use all pipes when primary is suspended.
  // Restore it back to 2 after primary poweron.
  max_sde_builtin_fetch_layers_ = (disabled && (powered_on_displays_.size() <= 1)) ?
                                   kMaxSDELayers : max_sde_secondary_fetch_layers_;
}

bool CompManager::CheckResourceState(Handle display_ctx, bool *res_exhausted,
                                     HWDisplayAttributes attr) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  bool res_wait_needed = false;

  resource_intf_->Perform(ResourceInterface::kCmdGetResourceStatus,
                          display_comp_ctx->display_resource_ctx, res_exhausted, &attr,
                          &res_wait_needed);
  return res_wait_needed;
}

DisplayError CompManager::SetDrawMethod(Handle display_ctx, const DisplayDrawMethod &draw_method) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  auto error = display_comp_ctx->strategy->SetDrawMethod(draw_method);
  if (error != kErrorNone) {
    return error;
  }
  error = resource_intf_->SetDrawMethod(display_comp_ctx->display_resource_ctx, draw_method);
  if (error != kErrorNone) {
    return error;
  }

  return kErrorNone;
}

bool CompManager::IsRotatorSupportedFormat(LayerBufferFormat format) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (resource_intf_) {
    return resource_intf_->IsRotatorSupportedFormat(format);
  }

  return false;
}

bool CompManager::IsDisplayHWAvailable() {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (resource_intf_) {
    return resource_intf_->IsDisplayHWAvailable();
  }

  return false;
}

DisplayError CompManager::FreeDemuraFetchResources(const uint32_t &display_id) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return resource_intf_->FreeDemuraFetchResources(display_id);
}

DisplayError CompManager::GetDemuraFetchResourceCount(MultiDpuDemuraMap *fetch_resource_cnt) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return resource_intf_->GetDemuraFetchResourceCount(fetch_resource_cnt);
}

DisplayError CompManager::ReserveDemuraFetchResources(const uint32_t &display_id,
                                                      const int8_t &preferred_rect) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return resource_intf_->ReserveDemuraFetchResources(display_id, preferred_rect);
}

DisplayError CompManager::ReserveABCFetchResources(const uint32_t &display_id, bool is_primary,
                                                   const int8_t &req_cnt) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return resource_intf_->ReserveABCFetchResources(display_id, is_primary, req_cnt);
}

DisplayError CompManager::GetDemuraFetchResources(Handle display_ctx,
                                                  vector<FetchResourceList> *frl) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  return resource_intf_->GetDemuraFetchResources(display_comp_ctx->display_resource_ctx, frl);
}

DisplayError CompManager::SetMaxSDEClk(Handle display_ctx, uint32_t clk) {
  DTRACE_SCOPED();
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (resource_intf_) {
    DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
    return resource_intf_->SetMaxSDEClk(display_comp_ctx->display_resource_ctx, clk);
  }

  return kErrorNotSupported;
}

void CompManager::GetRetireFence(Handle display_ctx, shared_ptr<Fence> *retire_fence) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (resource_intf_ == nullptr) {
    return;
  }

  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  resource_intf_->Perform(ResourceInterface::kCmdGetRetireFence,
                          display_comp_ctx->display_resource_ctx, retire_fence);
}

void CompManager::NeedsValidate(Handle display_ctx, bool *needs_validate) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (resource_intf_ == nullptr) {
    return;
  }

  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  resource_intf_->Perform(ResourceInterface::kCmdNeedsValidate,
                          display_comp_ctx->display_resource_ctx, needs_validate);
}

DisplayError CompManager::SetBacklightLevel(Handle display_ctx,
    const uint32_t &backlight_level) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return resource_intf_->Perform(ResourceInterface::kCmdSetBacklightLevel,
                                  display_comp_ctx->display_resource_ctx,
                                  backlight_level);
}

DisplayError CompManager::ForceToneMapConfigure(Handle display_ctx,
    DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return resource_intf_->ForceToneMapConfigure(display_comp_ctx->display_resource_ctx,
                                               disp_layer_stack);
}

DisplayError CompManager::GetDefaultQosData(Handle display_ctx,
                                            std::map<uint32_t, HWQosData> *default_qos_data) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  return resource_intf_->GetDefaultQoSData(display_comp_ctx->display_resource_ctx,
                                           default_qos_data);
}

DisplayError CompManager::HandleCwbFrequencyBoost(bool isRequest) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  DisplayError error = kErrorNone;
  error = resource_intf_->Perform(ResourceInterface::kCmdSetCwbBoost, &isRequest);
  return error;
}

DisplayError CompManager::PreCommit(Handle display_ctx) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
                             reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return resource_intf_->PreCommit(display_comp_ctx->display_resource_ctx);
}

void CompManager::SetSafeMode(bool enable) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  safe_mode_ = enable;
}

bool CompManager::IsSafeMode() {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return safe_mode_;
}

std::string CompManager::Dump(Handle display_ctx) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return resource_intf_->Dump(display_comp_ctx->display_resource_ctx);
}

DppsControlInterface* CompManager::GetDppsControlIntf() {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return dpps_ctrl_intf_;
}

void CompManager::SetDemuraStatus(bool status) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  demura_enabled_ = status;
}

bool CompManager::GetDemuraStatus() {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return demura_enabled_;
}

void CompManager::SetDemuraStatusForDisplay(const int32_t &display_id, bool status) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  display_demura_status_[display_id] = status;
}

bool CompManager::GetDemuraStatusForDisplay(const int32_t &display_id) {
  return display_demura_status_[display_id];
}

DisplayError CompManager::CaptureCwb(Handle display_ctx, const LayerBuffer &output_buffer,
                                     const CwbConfig &cwb_config) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  DisplayError error = kErrorNone;
  error = cwb_mgr_intf_->CaptureCwb(display_comp_ctx->display_id.GetDisplayId(), kCwbClientExternal,
                                    output_buffer, cwb_config, this);
  return error;
}

void CompManager::NotifyCwbDone(int32_t display_id, int32_t status, const LayerBuffer &buffer) {
  if (callback_map_[display_id]) {
    callback_map_[display_id]->NotifyCwbDone(status, buffer);
  }
}

void CompManager::TriggerRefresh(int32_t display_id) {
  callback_map_[display_id]->Refresh();
}

void CompManager::TriggerCwbTeardown(int32_t display_id, bool sync_teardown) {
  callback_map_[display_id]->OnCwbTeardown(sync_teardown);
}

bool CompManager::HasPendingCwbRequest(Handle display_ctx) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  return cwb_mgr_intf_->HasPendingCwbRequest(display_comp_ctx->display_id.GetDisplayId());
}

bool CompManager::HandleCwbTeardown(Handle display_ctx) {
  DisplayCompositionContext *display_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);

  return resource_intf_->HandleCwbTeardown(display_comp_ctx->display_resource_ctx);
}

DisplayError CompManager::RequestVirtualDisplayId(int32_t *vdisp_id) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return resource_intf_->RequestVirtualDisplayId(vdisp_id);
}

DisplayError CompManager::AllocateVirtualDisplayId(int32_t *vdisp_id) {
  return resource_intf_->AllocateVirtualDisplayId(vdisp_id);
}

DisplayError CompManager::DeallocateVirtualDisplayId(int32_t vdisp_id) {
  return resource_intf_->DeallocateVirtualDisplayId(vdisp_id);
}

uint32_t CompManager::GetMixerCount(DisplayId display_id) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  return resource_intf_->GetMixerCount(display_id);
}

void CompManager::SetDisplayLayerStack(Handle display_ctx, DispLayerStack *disp_layer_stack) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);

  DisplayCompositionContext *disp_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  disp_comp_ctx->strategy->SetDisplayLayerStack(disp_layer_stack);
}

void CompManager::GetDSConfig(Handle display_ctx, HWLayersInfo *hw_layers_info) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  if (resource_intf_) {
    DisplayCompositionContext *display_comp_ctx =
        reinterpret_cast<DisplayCompositionContext *>(display_ctx);
    resource_intf_->GetDSConfig(display_comp_ctx->display_resource_ctx, hw_layers_info);
  }
}

DisplayError CompManager::SetSprIntf(Handle display_ctx, std::shared_ptr<SPRIntf> intf) {
  DisplayCompositionContext *disp_comp_ctx =
      reinterpret_cast<DisplayCompositionContext *>(display_ctx);
  return disp_comp_ctx->strategy->SetSprIntf(intf);
}

bool CompManager::IsMirroredOfAnyDisplay(int32_t display_id, const LayerStack *layer_stack,
                                         int32_t *out_src_display) {
  if (resource_intf_ && resource_intf_->Perform(ResourceInterface::kCmdGetMirrorSource, display_id,
                                                layer_stack, out_src_display) == kErrorNone) {
    return true;
  }

  return false;
}

bool CompManager::IsActiveDisplay(int32_t display_id) {
  std::lock_guard<std::recursive_mutex> obj(comp_mgr_mutex_);
  return !!powered_on_displays_.count(display_id);
}

}  // namespace sdm
