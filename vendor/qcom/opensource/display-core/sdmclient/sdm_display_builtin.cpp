/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <stdarg.h>
#include <sys/mman.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/utils.h>

#include <map>
#include <string>
#include <vector>

#include "concurrency_mgr.h"
#include "sdm_color_mode_stc.h"
#include "sdm_debugger.h"
#include "sdm_display_builtin.h"

#define __CLASS__ "SDMDisplayBuiltIn"

namespace sdm {

static void SetRect(LayerRect &src_rect, SDMRect *target) {
  target->left = src_rect.left;
  target->top = src_rect.top;
  target->right = src_rect.right;
  target->bottom = src_rect.bottom;
}

DisplayError SDMDisplayBuiltIn::Create(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                                       SDMCompositorCallbacks *callbacks,
                                       SDMDisplayEventHandler *event_handler, Display id,
                                       int32_t sdm_id, SDMDisplay **sdm_display) {
  uint32_t builtin_width = 0;
  uint32_t builtin_height = 0;

  SDMDisplay *sdm_display_builtin = new SDMDisplayBuiltIn(
      core_intf, buffer_allocator, callbacks, event_handler, id, sdm_id);
  auto status = sdm_display_builtin->Init();
  if (status != kErrorNone) {
    delete sdm_display_builtin;
    return status;
  }

  sdm_display_builtin->GetMixerResolution(&builtin_width, &builtin_height);
  int width = 0, height = 0;
  SDMDebugHandler::Get()->GetProperty(FB_WIDTH_PROP, &width);
  SDMDebugHandler::Get()->GetProperty(FB_HEIGHT_PROP, &height);
  if (width > 0 && height > 0) {
    builtin_width = UINT32(width);
    builtin_height = UINT32(height);
  }

  status = sdm_display_builtin->SetFrameBufferResolution(builtin_width,
                                                         builtin_height);
  if (status) {
    Destroy(sdm_display_builtin);
    return status;
  }

  *sdm_display = sdm_display_builtin;

  return status;
}

void SDMDisplayBuiltIn::Destroy(SDMDisplay *sdm_display) {
  sdm_display->Deinit();
  delete sdm_display;
}

SDMDisplayBuiltIn::SDMDisplayBuiltIn(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                                     SDMCompositorCallbacks *callbacks,
                                     SDMDisplayEventHandler *event_handler, Display id,
                                     int32_t sdm_id)
    : SDMDisplay(core_intf, buffer_allocator, callbacks, event_handler, kBuiltIn, id, sdm_id,
                 DISPLAY_CLASS_BUILTIN),
      buffer_allocator_(buffer_allocator),
      cpu_hint_(NULL),
      layer_stitch_task_(*this) {}

DisplayError SDMDisplayBuiltIn::Init() {
  cpu_hint_ = new CPUHint();
  if (cpu_hint_->Init(static_cast<SDMDebugHandler *>(SDMDebugHandler::Get()), callbacks_) !=
      kErrorNone) {
    delete cpu_hint_;
    cpu_hint_ = NULL;
  }

  layer_stack_.flags.use_metadata_refresh_rate = true;
  int disable_metadata_dynfps = 0;
  SDMDebugHandler::Get()->GetProperty(DISABLE_METADATA_DYNAMIC_FPS_PROP,
                                      &disable_metadata_dynfps);
  if (disable_metadata_dynfps) {
    layer_stack_.flags.use_metadata_refresh_rate = false;
  }

  auto status = SDMDisplay::Init();
  if (status != kErrorNone) {
    return status;
  }
  color_mode_ = new SDMColorModeStc(display_intf_);
  color_mode_->Init();

  int value = 0;
  SDMDebugHandler::Get()->GetProperty(ENABLE_OPTIMIZE_REFRESH, &value);
  enable_optimize_refresh_ = (value == 1);
  if (enable_optimize_refresh_) {
    DLOGI("Drop redundant drawcycles %" PRIu64, id_);
  }

  int vsyncs = 0;
  SDMDebugHandler::Get()->GetProperty(TRANSIENT_FPS_CYCLE_COUNT, &vsyncs);
  if (vsyncs > 0) {
    SetVsyncsApplyRateChange(UINT32(vsyncs));
  }

  is_primary_ = display_intf_->IsPrimaryDisplay();

  windowed_display_ =
      Debug::GetWindowRect(is_primary_, &window_rect_.left, &window_rect_.top,
                           &window_rect_.right, &window_rect_.bottom) == 0;
  DLOGI("Window rect : [%f %f %f %f] is_primary_=%d", window_rect_.left,
        window_rect_.top, window_rect_.right, window_rect_.bottom, is_primary_);

  if (is_primary_) {
    value = 0;
    SDMDebugHandler::Get()->GetProperty(ENABLE_POMS_DURING_DOZE, &value);
    enable_poms_during_doze_ = (value == 1);
    if (enable_poms_during_doze_) {
      DLOGI("Enable POMS during Doze mode %" PRIu64, id_);
    }
  }

  SDMDebugHandler::Get()->GetProperty(ENABLE_PERF_HINT_LARGE_COMP_CYCLE,
                                      &perf_hint_large_comp_cycle_);

  value = 0;
  DebugHandler::Get()->GetProperty(DISABLE_DYNAMIC_FPS, &value);
  disable_dyn_fps_ = (value == 1);

  value = 0;
  DebugHandler::Get()->GetProperty(ENABLE_ROUNDED_CORNER, &value);
  enable_round_corner_ = (value == 1);

  value = 0;
  if (DebugHandler::Get()->GetProperty(LARGE_COMP_HINT_THRESHOLD, &value) ==
      kErrorNone) {
    large_comp_hint_threshold_ = value;
  }

  uint32_t config_index = 0;
  GetActiveDisplayConfig(false, &config_index);
  DisplayConfigVariableInfo attr = {};
  GetDisplayAttributesForConfig(INT(config_index), &attr);
  active_refresh_rate_ = attr.fps;

  DLOGI("active_refresh_rate: %d", active_refresh_rate_);

  int enhance_idle_time = 0;
  SDMDebugHandler::Get()->GetProperty(ENHANCE_IDLE_TIME, &enhance_idle_time);
  enhance_idle_time_ = (enhance_idle_time == 1);
  DLOGI("enhance_idle_time: %d", enhance_idle_time);

  LoadMixedModePerfHintThreshold();

  SDMDisplay::TryDrawMethod(DisplayDrawMethod::kDrawUnified);

  return status;
}

void SDMDisplayBuiltIn::Dump(std::ostringstream *os) {
  SDMDisplay::Dump(os);
  *os << callbacks_->DumpHistogram(id_);
}

void SDMDisplayBuiltIn::ValidateUiScaling() {
  if (is_primary_ || !is_cmd_mode_) {
    force_reset_lut_ = false;
    return;
  }

  for (auto &sdm_layer : sdm_layer_stack_->layer_set_) {
    Layer *layer = sdm_layer->GetSDMLayer();
    if (sdm_layer->IsScalingPresent() && !layer->input_buffer.flags.video) {
      force_reset_lut_ = true;
      return;
    }
  }
  force_reset_lut_ = false;
}

DisplayError SDMDisplayBuiltIn::PreValidateDisplay(bool *exit_validate) {
  DTRACE_SCOPED();

  // Draw method gets set as part of first commit.
  SetDrawMethod();

  auto status = kErrorNone;
  bool res_exhausted = false;
  // If no resources are available for the current display, mark it for GPU by
  // pass and continue to do invalidate until the resources are available
  if (display_paused_ || CheckResourceState(&res_exhausted)) {
    MarkLayersForGPUBypass();
    *exit_validate = true;
    return status;
  }

  if (color_tranform_failed_) {
    // Must fall back to client composition
    MarkLayersForClientComposition();
  }

  // Fill in the remaining blanks in the layers and add them to the SDM
  // layerstack
  BuildLayerStack();

  // Check for scaling layers during Doze mode
  ValidateUiScaling();

  // Add stitch layer to layer stack.
  AppendStitchLayer();

  // Checks and replaces layer stack for solid fill
  SolidFillPrepare();

  // Apply current Color Mode and Render Intent.
  if (color_mode_->ApplyCurrentColorModeWithRenderIntent(
          static_cast<bool>(layer_stack_.flags.hdr_present)) != kErrorNone) {
    // Fallback to GPU Composition, if Color Mode can't be applied.
    MarkLayersForClientComposition();
  }

  uint32_t refresh_rate = 0;
  display_intf_->GetRefreshRate(&refresh_rate);
  current_refresh_rate_ = refresh_rate;

  if (sdm_layer_stack_->layer_set_.empty()) {
    // Avoid flush for Command mode panel.
    flush_ = !client_connected_;
    *exit_validate = true;
    return status;
  }

  display_idle_ = false;
  has_client_composition_ = false;

  *exit_validate = false;

  return status;
}

DisplayError SDMDisplayBuiltIn::CommitLayerStack() {
  SetDynamicDSIClock();

  skip_commit_ = CanSkipCommit();
  DisplayError error = SDMDisplay::CommitLayerStack();

  if (commit_counter_) {
    commit_counter_ = false;
    callbacks_->OnRefresh(id_);
  }

  return error;
}

bool SDMDisplayBuiltIn::CanSkipCommit() {
  if (layer_stack_invalid_) {
    return false;
  }

  // Reject repeated drawcycle requests if it satisfies all conditions.
  // 1. None of the layerstack attributes changed.
  // 2. No new buffer latched.
  // 3. No refresh request triggered by SDM.
  // 4. This display is not source of vsync.
  // 5. No CWB client
  bool buffers_latched = false;
  bool needs_validation = false;
  for (auto &sdm_layer : sdm_layer_stack_->layer_set_) {
    buffers_latched |= sdm_layer->BufferLatched();
    sdm_layer->ResetBufferFlip();
    needs_validation |= sdm_layer->NeedsValidation();
  }

  bool vsync_source = (event_handler_->GetVsyncSource() == id_);

  bool skip_commit = false;
  {
    std::unique_lock<std::mutex> lock(cwb_mutex_);
    skip_commit = enable_optimize_refresh_ && !pending_commit_ &&
                  !buffers_latched && !pending_refresh_ && !vsync_source &&
                  (cwb_buffer_map_.size() == 0) && !needs_validation;
  } // releasing the cwb state lock
  pending_refresh_ = false;

  return skip_commit;
}

DisplayError SDMDisplayBuiltIn::CommitStitchLayers() {
  if (disable_layer_stitch_) {
    return kErrorNone;
  }

  if (!display_intf_->IsValidated() || skip_commit_) {
    return kErrorNone;
  }

  LayerStitchContext ctx = {};
  Layer *stitch_layer = stitch_target_->GetSDMLayer();
  LayerBuffer &output_buffer = stitch_layer->input_buffer;
  for (auto &layer : layer_stack_.layers) {
    LayerComposition &composition = layer->composition;
    if (composition != kCompositionStitch) {
      continue;
    }

    SDMStitchParams params = {};
    // Stitch target doesn't have an input fence.
    // Render all layers at specified destination.
    LayerBuffer &input_buffer = layer->input_buffer;
    params.src_hnd = reinterpret_cast<void *>(input_buffer.buffer_id);
    params.dst_hnd = reinterpret_cast<void *>(output_buffer.buffer_id);
    SetRect(layer->stitch_info.dst_rect, &params.dst_rect);
    SetRect(layer->stitch_info.slice_rect, &params.scissor_rect);
    params.src_acquire_fence = input_buffer.acquire_fence;

    ctx.stitch_params.push_back(params);
  }

  if (!ctx.stitch_params.size()) {
    // No layers marked for stitch.
    return kErrorNone;
  }

  layer_stitch_task_.PerformTask(LayerStitchTaskCode::kCodeStitch, &ctx);
  // Set release fence.
  output_buffer.acquire_fence = ctx.release_fence;

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetPowerMode(SDMPowerMode mode, bool teardown) {
  auto status = SDMDisplay::SetPowerMode(mode, teardown);
  if (status != kErrorNone) {
    return status;
  }
  DLOGV_IF(kTagClient, "Setting Power State as \'%s\' for %d-%d",
           (mode == SDMPowerMode::POWER_MODE_ON)     ? "ON"
           : (mode == SDMPowerMode::POWER_MODE_OFF)  ? "OFF"
           : (mode == SDMPowerMode::POWER_MODE_DOZE) ? "DOZE"
                                                     : "DOZE_SUSPEND",
           sdm_id_, type_);
  if (cpu_hint_) {
    switch (mode) {
    case SDMPowerMode::POWER_MODE_DOZE:
    case SDMPowerMode::POWER_MODE_DOZE_SUSPEND:
      // Perf hal doesn't differentiate b/w doze and doze-suspend, so send doze
      // hint for both.
      cpu_hint_->ReqEvent(kPerfHintDisplayDoze);
      break;
    case SDMPowerMode::POWER_MODE_ON:
      cpu_hint_->ReqEvent(kPerfHintDisplayOn);
      break;
    case SDMPowerMode::POWER_MODE_OFF:
      cpu_hint_->ReqEvent(kPerfHintDisplayOff);
      break;
    default:
      break;
    }
  }

  DisplayConfigFixedInfo fixed_info = {};
  display_intf_->GetConfig(&fixed_info);
  is_cmd_mode_ = fixed_info.is_cmdmode;

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::Present(shared_ptr<Fence> *out_retire_fence) {
  auto status = kErrorNone;
  bool res_exhausted = false;

  DTRACE_SCOPED();

  // Proceed only if any resources are available to be allocated for the current
  // display, Otherwise keep doing invalidate
  if (CheckResourceState(&res_exhausted)) {
    Refresh();
    return status;
  }

  if (display_paused_) {
    return status;
  } else {
    if (status != kErrorNone) {
      DLOGE("Stitch failed: %d", status);
      return status;
    }

    status = CommitLayerStack();
    if (status == kErrorNone) {
      status = PostCommitLayerStack(out_retire_fence);
    }
  }

  // In case of scaling UI layer for command mode, clear LUTs
  if (force_reset_lut_) {
    display_intf_->ClearLUTs();
  }
  return status;
}

void SDMDisplayBuiltIn::PostCommitStitchLayers() {
  if (disable_layer_stitch_) {
    return;
  }

  // Close Stitch buffer acquire fence.
  Layer *stitch_layer = stitch_target_->GetSDMLayer();
  LayerBuffer &output_buffer = stitch_layer->input_buffer;
  for (auto &layer : layer_stack_.layers) {
    LayerComposition &composition = layer->composition;
    if (composition != kCompositionStitch) {
      continue;
    }
    LayerBuffer &input_buffer = layer->input_buffer;
    input_buffer.release_fence = output_buffer.acquire_fence;
  }
}

DisplayError SDMDisplayBuiltIn::GetColorModes(uint32_t *out_num_modes,
                                              SDMColorMode *out_modes) {
  if (out_modes == nullptr) {
    *out_num_modes = color_mode_->GetColorModeCount();
  } else {
    color_mode_->GetColorModes(out_num_modes, out_modes);
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::GetRenderIntents(SDMColorMode mode,
                                                 uint32_t *out_num_intents,
                                                 SDMRenderIntent *out_intents) {
  if (out_intents == nullptr) {
    *out_num_intents = color_mode_->GetRenderIntentCount(mode);
  } else {
    color_mode_->GetRenderIntents(mode, out_num_intents, out_intents);
  }
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetColorMode(SDMColorMode mode) {
  return SetColorModeWithRenderIntent(mode, SDMRenderIntent::COLORIMETRIC);
}

DisplayError
SDMDisplayBuiltIn::SetColorModeWithRenderIntent(SDMColorMode mode,
                                                SDMRenderIntent intent) {
  auto status = color_mode_->CacheColorModeWithRenderIntent(mode, intent);
  if (status != kErrorNone) {
    DLOGE("failed for mode = %d intent = %d", mode, intent);
    return status;
  }
  callbacks_->OnRefresh(id_);
  return status;
}

DisplayError SDMDisplayBuiltIn::SetColorModeById(int32_t color_mode_id) {
  auto status = color_mode_->SetColorModeById(color_mode_id);
  if (status != kErrorNone) {
    DLOGE("failed for mode = %d", color_mode_id);
    return status;
  }

  callbacks_->OnRefresh(id_);

  return status;
}

DisplayError
SDMDisplayBuiltIn::SetColorModeFromClientApi(int32_t color_mode_id) {
  DisplayError error = kErrorNone;
  std::string mode_string;

  error = display_intf_->GetColorModeName(color_mode_id, &mode_string);
  if (error) {
    DLOGE("Failed to get mode name for mode %d", color_mode_id);
    return kErrorNotSupported;
  }

  auto status = color_mode_->SetColorModeFromClientApi(mode_string);
  if (status != kErrorNone) {
    DLOGE("Failed to set mode = %d", color_mode_id);
    return status;
  }

  return status;
}

DisplayError SDMDisplayBuiltIn::RestoreColorTransform() {
  auto status = color_mode_->RestoreColorTransform();
  if (status != kErrorNone) {
    DLOGE("failed to RestoreColorTransform");
    return status;
  }

  callbacks_->OnRefresh(id_);

  return status;
}

DisplayError SDMDisplayBuiltIn::SetColorTransform(const float *matrix, SDMColorTransform hint) {
  if (!matrix) {
    return kErrorNotSupported;
  }

  auto status = color_mode_->SetColorTransform(matrix, hint);
  if (status != kErrorNone) {
    DLOGE("failed for hint = %d", hint);
    color_tranform_failed_ = true;
    return status;
  }

  callbacks_->OnRefresh(id_);
  color_tranform_failed_ = false;

  return status;
}

DisplayError SDMDisplayBuiltIn::SetDisplayDppsAdROI(
    uint32_t h_start, uint32_t h_end, uint32_t v_start, uint32_t v_end,
    uint32_t factor_in, uint32_t factor_out) {
  DisplayError error = kErrorNone;
  DisplayDppsAd4RoiCfg dpps_ad4_roi_cfg = {};
  uint32_t panel_width = 0, panel_height = 0;
  constexpr uint16_t kMaxFactorVal = 0xffff;

  if (h_start >= h_end || v_start >= v_end || factor_in > kMaxFactorVal ||
      factor_out > kMaxFactorVal) {
    DLOGE("Invalid roi region = [%u, %u, %u, %u, %u, %u]", h_start, h_end,
          v_start, v_end, factor_in, factor_out);
    return kErrorNotSupported;
  }

  GetPanelResolution(&panel_width, &panel_height);

  if (h_start >= panel_width || h_end > panel_width ||
      v_start >= panel_height || v_end > panel_height) {
    DLOGE("Invalid roi region = [%u, %u, %u, %u], panel resolution = [%u, %u]",
          h_start, h_end, v_start, v_end, panel_width, panel_height);
    return kErrorNotSupported;
  }

  dpps_ad4_roi_cfg.h_start = h_start;
  dpps_ad4_roi_cfg.h_end = h_end;
  dpps_ad4_roi_cfg.v_start = v_start;
  dpps_ad4_roi_cfg.v_end = v_end;
  dpps_ad4_roi_cfg.factor_in = factor_in;
  dpps_ad4_roi_cfg.factor_out = factor_out;

  error = display_intf_->SetDisplayDppsAdROI(&dpps_ad4_roi_cfg);
  if (error)
    return kErrorParameters;

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetFrameTriggerMode(uint32_t mode) {
  DisplayError error = kErrorNone;
  FrameTriggerMode trigger_mode = kFrameTriggerDefault;

  if (mode >= kFrameTriggerMax) {
    DLOGE("Invalid input mode %d", mode);
    return kErrorNotSupported;
  }

  trigger_mode = static_cast<FrameTriggerMode>(mode);
  error = display_intf_->SetFrameTriggerMode(trigger_mode);
  if (error)
    return kErrorParameters;

  callbacks_->OnRefresh(SDM_DISPLAY_PRIMARY);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::Perform(uint32_t operation, ...) {
  va_list args;
  va_start(args, operation);
  int val = 0;
  LayerSolidFill *solid_fill_color;
  LayerRect *rect = NULL;

  switch (operation) {
  case SET_METADATA_DYN_REFRESH_RATE:
    val = va_arg(args, int32_t);
    SetMetaDataRefreshRateFlag(val);
    break;
  case SET_BINDER_DYN_REFRESH_RATE:
    val = va_arg(args, int32_t);
    ForceRefreshRate(UINT32(val));
    break;
  case SET_DISPLAY_MODE:
    val = va_arg(args, int32_t);
    SetDisplayMode(UINT32(val));
    break;
  case SET_QDCM_SOLID_FILL_INFO:
    solid_fill_color = va_arg(args, LayerSolidFill *);
    SetQDCMSolidFillInfo(true, *solid_fill_color);
    break;
  case UNSET_QDCM_SOLID_FILL_INFO:
    solid_fill_color = va_arg(args, LayerSolidFill *);
    SetQDCMSolidFillInfo(false, *solid_fill_color);
    break;
  case SET_QDCM_SOLID_FILL_RECT:
    rect = va_arg(args, LayerRect *);
    solid_fill_rect_ = *rect;
    break;
  case UPDATE_TRANSFER_TIME:
    val = va_arg(args, int32_t);
    UpdateTransferTime(UINT32(val));
    break;
  default:
    DLOGW("Invalid operation %d", operation);
    va_end(args);
    return kErrorNotSupported;
  }
  va_end(args);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDisplayMode(uint32_t mode) {
  DisplayError error = kErrorNone;

  if (display_intf_) {
    error = display_intf_->SetDisplayMode(mode);
    if (error == kErrorNone) {
      DisplayConfigFixedInfo fixed_info = {};
      display_intf_->GetConfig(&fixed_info);
      is_cmd_mode_ = fixed_info.is_cmdmode;
    }
  }

  return error;
}

void SDMDisplayBuiltIn::SetMetaDataRefreshRateFlag(bool enable) {
  int disable_metadata_dynfps = 0;

  SDMDebugHandler::Get()->GetProperty(DISABLE_METADATA_DYNAMIC_FPS_PROP,
                                      &disable_metadata_dynfps);
  if (disable_metadata_dynfps) {
    return;
  }
  layer_stack_.flags.use_metadata_refresh_rate = enable;
}

void SDMDisplayBuiltIn::SetQDCMSolidFillInfo(bool enable,
                                             const LayerSolidFill &color) {
  solid_fill_enable_ = enable;
  solid_fill_color_ = color;
}

DisplayError SDMDisplayBuiltIn::GetActiveSecureSession(
    std::bitset<kSecureMax> *secure_sessions) {
  if (!secure_sessions) {
    return kErrorNotSupported;
  }
  secure_sessions->reset();
  for (auto sdm_layer : sdm_layer_stack_->layer_set_) {
    Layer *layer = sdm_layer->GetSDMLayer();
    if (layer->input_buffer.flags.secure_camera) {
      secure_sessions->set(kSecureCamera);
    }
    if (layer->input_buffer.flags.secure_display) {
      secure_sessions->set(kSecureDisplay);
    }
  }
  if (secure_event_ == kTUITransitionStart ||
      secure_event_ == kTUITransitionPrepare) {
    secure_sessions->set(kSecureTUI);
  }
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::HandleSecureSession(
    const std::bitset<kSecureMax> &secure_sessions, bool *power_on_pending,
    bool is_active_secure_display) {
  if (!power_on_pending) {
    return kErrorNotSupported;
  }

  if (!is_active_secure_display) {
    // Do handling as done on non-primary displays.
    DLOGI("Default handling for display %" PRIu64 " %d-%d", id_, sdm_id_,
          type_);
    return SDMDisplay::HandleSecureSession(secure_sessions, power_on_pending,
                                           is_active_secure_display);
  }

  if (current_power_mode_ != SDMPowerMode::POWER_MODE_ON) {
    return kErrorNone;
  }

  if (active_secure_sessions_[kSecureDisplay] !=
      secure_sessions[kSecureDisplay]) {
    SecureEvent secure_event = secure_sessions.test(kSecureDisplay)
                                   ? kSecureDisplayStart
                                   : kSecureDisplayEnd;
    bool needs_refresh = false;
    DisplayError err =
        display_intf_->HandleSecureEvent(secure_event, &needs_refresh);
    if (err != kErrorNone) {
      DLOGE("Set secure event failed");
      return err;
    }

    DLOGI("SecureDisplay state changed from %d to %d for display %" PRIu64
          " %d-%d",
          active_secure_sessions_.test(kSecureDisplay),
          secure_sessions.test(kSecureDisplay), id_, sdm_id_, type_);
  }
  active_secure_sessions_ = secure_sessions;
  *power_on_pending = false;
  return kErrorNone;
}

void SDMDisplayBuiltIn::ForceRefreshRate(uint32_t refresh_rate) {
  if ((refresh_rate && (refresh_rate < min_refresh_rate_ ||
                        refresh_rate > max_refresh_rate_)) ||
      layer_stack_.force_refresh_rate == refresh_rate) {
    // Cannot honor force refresh rate, as its beyond the range or new request
    // is same
    return;
  }

  layer_stack_.force_refresh_rate = refresh_rate;

  callbacks_->OnRefresh(id_);

  return;
}

void SDMDisplayBuiltIn::SetIdleTimeoutMs(uint32_t timeout_ms,
                                         uint32_t inactive_ms) {
  display_intf_->SetIdleTimeoutMs(timeout_ms, inactive_ms);
}

void SDMDisplayBuiltIn::HandleFrameCapture() {
  auto ret = kCWBReleaseFenceErrorNone;
  {
    std::unique_lock<std::mutex> lock(cwb_mutex_);
    auto &cwb_resp = cwb_capture_status_map_[kCWBClientColor];
    // If CWB request status is not notified, then need to wait for the
    // notification.
    if (cwb_resp.status == kCWBReleaseFenceNotChecked) {
      cwb_cv_.wait(lock);
    }
    ret = cwb_resp.status;
  }

  frame_capture_status_ = (ret == kCWBReleaseFenceWaitTimedOut) ? kErrorTimeOut
                          : (ret) ? kErrorNotSupported
                                  : kErrorNone;
  frame_capture_buffer_queued_ = false;

  DLOGV_IF(kTagQDCM, "Frame captured: frame_capture_buffer_queued_ %d",
           frame_capture_buffer_queued_);
}

DisplayError
SDMDisplayBuiltIn::FrameCaptureAsync(const BufferInfo &output_buffer_info,
                                     const CwbConfig &cwb_config) {
  // Note: This function is called in context of a binder thread and a lock is
  // already held
  if (output_buffer_info.alloc_buffer_info.fd < 0) {
    DLOGE("Invalid fd %d", output_buffer_info.alloc_buffer_info.fd);
    return kErrorNotSupported;
  }

  if (cwb_config.tap_point < CwbTapPoint::kLmTapPoint ||
      cwb_config.tap_point > CwbTapPoint::kDemuraTapPoint) {
    DLOGE("Invalid CWB tappoint passed by client ");
    return kErrorNotSupported;
  }

  void *buffer = output_buffer_info.private_data;
  DisplayError err =
      SetReadbackBuffer((void *)buffer, nullptr, cwb_config, kCWBClientColor);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }
  frame_capture_buffer_queued_ = true;
  frame_capture_status_ = kErrorTryAgain;

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDetailEnhancerConfig(
    const DisplayDetailEnhancerData &de_data) {
  DisplayError error = kErrorNotSupported;

  if (display_intf_) {
    error = display_intf_->SetDetailEnhancerData(de_data);
  }
  return error;
}

DisplayError SDMDisplayBuiltIn::SetHWDetailedEnhancerConfig(void *params) {
  DisplayError err = kErrorNone;
  DisplayDetailEnhancerData de_data;

  PPDETuningCfgData *de_tuning_cfg_data =
      reinterpret_cast<PPDETuningCfgData *>(params);
  if (de_tuning_cfg_data->cfg_pending) {
    if (!de_tuning_cfg_data->cfg_en) {
      de_data.enable = 0;
      DLOGV_IF(kTagQDCM, "Disable DE config");
    } else {
      de_data.override_flags = kOverrideDEEnable;
      de_data.enable = 1;
#ifdef DISP_DE_LPF_BLEND
      DLOGV_IF(
          kTagQDCM,
          "Enable DE: flags %u, sharp_factor %d, thr_quiet %d, thr_dieout %d, "
          "thr_low %d, thr_high %d, clip %d, quality %d, content_type %d, "
          "de_blend %d, "
          "de_lpf_h %d, de_lpf_m %d, de_lpf_l %d",
          de_tuning_cfg_data->params.flags,
          de_tuning_cfg_data->params.sharp_factor,
          de_tuning_cfg_data->params.thr_quiet,
          de_tuning_cfg_data->params.thr_dieout,
          de_tuning_cfg_data->params.thr_low,
          de_tuning_cfg_data->params.thr_high, de_tuning_cfg_data->params.clip,
          de_tuning_cfg_data->params.quality,
          de_tuning_cfg_data->params.content_type,
          de_tuning_cfg_data->params.de_blend,
          de_tuning_cfg_data->params.de_lpf_h,
          de_tuning_cfg_data->params.de_lpf_m,
          de_tuning_cfg_data->params.de_lpf_l);
#endif
      if (de_tuning_cfg_data->params.flags & kDeTuningFlagSharpFactor) {
        de_data.override_flags |= kOverrideDESharpen1;
        de_data.sharp_factor = de_tuning_cfg_data->params.sharp_factor;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagClip) {
        de_data.override_flags |= kOverrideDEClip;
        de_data.clip = de_tuning_cfg_data->params.clip;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagThrQuiet) {
        de_data.override_flags |= kOverrideDEThrQuiet;
        de_data.thr_quiet = de_tuning_cfg_data->params.thr_quiet;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagThrDieout) {
        de_data.override_flags |= kOverrideDEThrDieout;
        de_data.thr_dieout = de_tuning_cfg_data->params.thr_dieout;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagThrLow) {
        de_data.override_flags |= kOverrideDEThrLow;
        de_data.thr_low = de_tuning_cfg_data->params.thr_low;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagThrHigh) {
        de_data.override_flags |= kOverrideDEThrHigh;
        de_data.thr_high = de_tuning_cfg_data->params.thr_high;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagContentQualLevel) {
        switch (de_tuning_cfg_data->params.quality) {
        case kDeContentQualLow:
          de_data.quality_level = kContentQualityLow;
          break;
        case kDeContentQualMedium:
          de_data.quality_level = kContentQualityMedium;
          break;
        case kDeContentQualHigh:
          de_data.quality_level = kContentQualityHigh;
          break;
        case kDeContentQualUnknown:
        default:
          de_data.quality_level = kContentQualityUnknown;
          break;
        }
      }

      switch (de_tuning_cfg_data->params.content_type) {
      case kDeContentTypeVideo:
        de_data.content_type = kContentTypeVideo;
        break;
      case kDeContentTypeGraphics:
        de_data.content_type = kContentTypeGraphics;
        break;
      case kDeContentTypeUnknown:
      default:
        de_data.content_type = kContentTypeUnknown;
        break;
      }

      if (de_tuning_cfg_data->params.flags & kDeTuningFlagDeBlend) {
        de_data.override_flags |= kOverrideDEBlend;
        de_data.de_blend = de_tuning_cfg_data->params.de_blend;
      }
#ifdef DISP_DE_LPF_BLEND
      if (de_tuning_cfg_data->params.flags & kDeTuningFlagDeLpfBlend) {
        de_data.override_flags |= kOverrideDELpfBlend;
        de_data.de_lpf_en = true;
        de_data.de_lpf_h = de_tuning_cfg_data->params.de_lpf_h;
        de_data.de_lpf_m = de_tuning_cfg_data->params.de_lpf_m;
        de_data.de_lpf_l = de_tuning_cfg_data->params.de_lpf_l;
      }
#endif
    }
    err = SetDetailEnhancerConfig(de_data);
    if (err) {
      DLOGW("SetDetailEnhancerConfig failed. err = %d", err);
    }
    de_tuning_cfg_data->cfg_pending = false;
  }
  return err;
}

DisplayError SDMDisplayBuiltIn::ControlPartialUpdate(bool enable,
                                                     uint32_t *pending) {
  DisplayError error = kErrorNone;

  if (display_intf_) {
    error = display_intf_->ControlPartialUpdate(enable, pending);
  }

  return error;
}

DisplayError SDMDisplayBuiltIn::DisablePartialUpdateOneFrame() {
  DisplayError error = kErrorNone;

  if (display_intf_) {
    error = display_intf_->DisablePartialUpdateOneFrame();
  }

  return error;
}

DisplayError
SDMDisplayBuiltIn::SetDisplayedContentSamplingEnabledVndService(bool enabled) {
  std::unique_lock<decltype(sampling_mutex)> lk(sampling_mutex);
  vndservice_sampling_vote = enabled;
  if (api_sampling_vote || vndservice_sampling_vote) {
    callbacks_->StartHistogram(id_, 0);
    display_intf_->colorSamplingOn();
  } else {
    display_intf_->colorSamplingOff();
    callbacks_->StopHistogram(id_, false);
  }
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDisplayedContentSamplingEnabled(
    bool enabled, uint8_t component_mask, uint64_t max_frames) {
  std::unique_lock<decltype(sampling_mutex)> lk(sampling_mutex);
  if (enabled) {
    api_sampling_vote = true;
  } else {
    api_sampling_vote = false;
  }

  auto start = api_sampling_vote || vndservice_sampling_vote;
  if (start && max_frames == 0) {
    callbacks_->StartHistogram(id_, 0);
    display_intf_->colorSamplingOn();
  } else if (start) {
    callbacks_->StartHistogram(id_, max_frames);
    display_intf_->colorSamplingOn();
  } else {
    display_intf_->colorSamplingOff();
    callbacks_->StopHistogram(id_, false);
  }
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::GetDisplayedContentSamplingAttributes(
    int32_t *format, int32_t *dataspace, uint8_t *supported_components) {
  return callbacks_->GetHistogramAttributes(id_, format, dataspace, supported_components);
}

DisplayError SDMDisplayBuiltIn::GetDisplayedContentSample(
    uint64_t max_frames, uint64_t timestamp, uint64_t *numFrames,
    int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
    uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS]) {
  callbacks_->CollectHistogram(id_, max_frames, timestamp, samples_size, samples, numFrames);
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetMixerResolution(uint32_t width,
                                                   uint32_t height) {
  DisplayError error = display_intf_->SetMixerResolution(width, height);
  callbacks_->OnRefresh(id_);
  return error;
}

DisplayError SDMDisplayBuiltIn::GetMixerResolution(uint32_t *width,
                                                   uint32_t *height) {
  return display_intf_->GetMixerResolution(width, height);
}

DisplayError SDMDisplayBuiltIn::SetQSyncMode(QSyncMode qsync_mode) {
  // Client needs to ensure that config change and qsync mode change
  // are not triggered in the same drawcycle.
  if (pending_config_) {
    DLOGE("Failed to set qsync mode. Pending active config transition");
    return kErrorNotSupported;
  }

  auto err = display_intf_->SetQSyncMode(qsync_mode);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::ControlIdlePowerCollapse(bool enable,
                                                         bool synchronous) {
  DisplayError error = kErrorNone;

  if (display_intf_) {
    error = display_intf_->ControlIdlePowerCollapse(enable, synchronous);
  }
  return error;
}

DisplayError SDMDisplayBuiltIn::SetJitterConfig(uint32_t jitter_type,
                                                float value, uint32_t time) {
  DisplayError error = display_intf_->SetJitterConfig(jitter_type, value, time);
  if (error != kErrorNone) {
    DLOGE("Failed to set jitter configuration.");
    return error;
  }

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDynamicDSIClock() {
  // decrement the counter and set dsi clock when counter hit 0
  if (!scheduled_dynamic_dsi_clk_ || commit_counter_) {
    return kErrorNone;
  }

  DTRACE_SCOPED();

  DisplayError error = display_intf_->SetDynamicDSIClock(scheduled_dynamic_dsi_clk_);
  if (error != kErrorNone) {
    DLOGE(" failed: Clk: %" PRIu64 " Error: %d", scheduled_dynamic_dsi_clk_, error);
  }

  scheduled_dynamic_dsi_clk_ = 0;
  ControlIdlePowerCollapse(true, false);

  return error;
}

DisplayError SDMDisplayBuiltIn::ScheduleDynamicDSIClock(uint64_t bitclk) {
  if (scheduled_dynamic_dsi_clk_) {
    return kErrorPermission;
  }

  DTRACE_SCOPED();

  DisablePartialUpdateOneFrame();
  ControlIdlePowerCollapse(false, false);

  scheduled_dynamic_dsi_clk_ = bitclk;

  commit_counter_ = true;

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::GetDynamicDSIClock(uint64_t *bitclk) {
  if (display_intf_) {
    return display_intf_->GetDynamicDSIClock(bitclk);
  }

  return kErrorNotSupported;
}

DisplayError
SDMDisplayBuiltIn::GetSupportedDSIClock(std::vector<uint64_t> *bitclk_rates) {
  if (display_intf_) {
    return display_intf_->GetSupportedDSIClock(bitclk_rates);
  }

  return kErrorNotSupported;
}

DisplayError SDMDisplayBuiltIn::UpdateDisplayId(Display id) {
  id_ = id;
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetPendingRefresh() {
  pending_refresh_ = true;
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetPanelBrightness(float brightness) {
  DisplayError ret = display_intf_->SetPanelBrightness(brightness);
  if (ret != kErrorNone) {
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::GetPanelBrightness(float *brightness) {
  DisplayError ret = display_intf_->GetPanelBrightness(brightness);
  if (ret != kErrorNone) {
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError
SDMDisplayBuiltIn::GetPanelMaxBrightness(uint32_t *max_brightness_level) {
  DisplayError ret = display_intf_->GetPanelMaxBrightness(max_brightness_level);
  if (ret != kErrorNone) {
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetBppMode(uint32_t bpp) {
  DisplayError error = kErrorNotSupported;

  if (display_intf_) {
    error = display_intf_->SetBppMode(bpp);
  }

  return error;
}

DisplayError SDMDisplayBuiltIn::SetBLScale(uint32_t level) {
  DisplayError ret = display_intf_->SetBLScale(level);
  if (ret != kErrorNone) {
    return kErrorResources;
  }
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetClientTarget(const SnapHandle *target,
                                                shared_ptr<Fence> acquire_fence,
                                                int32_t dataspace,
                                                const SDMRegion &damage,
                                                uint32_t version) {
  DTRACE_SCOPED();
  DisplayError error = SDMDisplay::SetClientTarget(target, acquire_fence,
                                                   dataspace, damage, version);
  if (error != kErrorNone) {
    return error;
  }

  // windowed_display and dynamic scaling are not supported.
  if (windowed_display_) {
    return kErrorNone;
  }

// SetFrameBufferConfig shows wrong display size. It may conflicts between Samsung and QC's multiresolution feature
// So Samsung disable SetFrameBufferConfig. 
#if 0
  Layer *sdm_layer = client_target_->GetSDMLayer();
  uint32_t fb_width = 0, fb_height = 0;

  GetFrameBufferResolution(&fb_width, &fb_height);

  if (fb_width != sdm_layer->input_buffer.unaligned_width ||
      fb_height != sdm_layer->input_buffer.unaligned_height) {
    if (SetFrameBufferConfig(sdm_layer->input_buffer.unaligned_width,
                             sdm_layer->input_buffer.unaligned_height)) {
      return kErrorNotSupported;
    }
  }
#endif

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::UpdateTransferTime(uint32_t transfer_time) {
  DisplayError error = display_intf_->UpdateTransferTime(transfer_time);
  if (error != kErrorNone) {
    DLOGE(" failed: Transfer time: %" PRIu32 " Error: %d", transfer_time,
          error);
    return error;
  }
  return kErrorNone;
}

bool SDMDisplayBuiltIn::IsSmartPanelConfig(uint32_t config_id) {
  if (config_id < sdm_config_map_.size()) {
    uint32_t index = sdm_config_map_.at(config_id);
    return variable_config_map_.at(index).smart_panel;
  }

  return false;
}

bool SDMDisplayBuiltIn::HasSmartPanelConfig(void) {
  if (!enable_poms_during_doze_) {
    uint32_t config = 0;
    GetActiveDisplayConfig(false, &config);
    return IsSmartPanelConfig(config);
  }

  for (auto &config : variable_config_map_) {
    if (config.second.smart_panel) {
      return true;
    }
  }

  return false;
}

DisplayError SDMDisplayBuiltIn::Deinit() {
  // Destory color convert instance. This destroys thread and underlying GL
  // resources.
  callbacks_->DestroyLayerStitch(id_);

  callbacks_->StopHistogram(id_, true);
  return SDMDisplay::Deinit();
}

void SDMDisplayBuiltIn::OnTask(const LayerStitchTaskCode &task_code,
                               SyncTask<LayerStitchTaskCode>::TaskContext *task_context) {
  switch (task_code) {
    case LayerStitchTaskCode::kCodeGetInstance: {
      callbacks_->InitLayerStitch(id_);
    } break;
    case LayerStitchTaskCode::kCodeStitch: {
      DTRACE_SCOPED();
      LayerStitchContext *ctx = reinterpret_cast<LayerStitchContext *>(task_context);
      callbacks_->StitchLayers(id_, ctx);
    } break;
    case LayerStitchTaskCode::kCodeDestroyInstance: {
      callbacks_->DestroyLayerStitch(id_);
    } break;
  }
}

bool SDMDisplayBuiltIn::InitLayerStitch() {
  if (!is_primary_) {
    // Disable on all non-primary builtins.
    DLOGI("Non-primary builtin.");
    disable_layer_stitch_ = true;
    return true;
  }

  // Disable by default.
  int value = 1;
  Debug::Get()->GetProperty(DISABLE_LAYER_STITCH, &value);
  disable_layer_stitch_ = (value == 1);

  if (disable_layer_stitch_) {
    DLOGI("Layer Stitch Disabled !!!");
    return true;
  }

  // Initialize stitch context. This will be non-secure.
  layer_stitch_task_.PerformTask(LayerStitchTaskCode::kCodeGetInstance, nullptr);

  if (!AllocateStitchBuffer()) {
    return true;
  }

  stitch_target_ =
      new SDMLayer(id_, static_cast<BufferAllocator *>(buffer_allocator_));

  // Populate buffer params and pvt handle.
  InitStitchTarget();

  // DLOGI("Created LayerStitch instance: %p", gl_layer_stitch_);

  return true;
}

bool SDMDisplayBuiltIn::AllocateStitchBuffer() {
  // Buffer dimensions: FB width * (1.5 * height)
  DTRACE_SCOPED();

  DisplayError error = display_intf_->GetFrameBufferConfig(&fb_config_);
  if (error != kErrorNone) {
    DLOGE("Get frame buffer config failed. Error = %d", error);
    return false;
  }

  BufferConfig &config = buffer_info_.buffer_config;
  config.width = fb_config_.x_pixels;
  config.height = fb_config_.y_pixels * kBufferHeightFactor;

  // By default UBWC is enabled and below property is global enable/disable for
  // all buffers allocated through snapalloc , including framebuffer targets.
  int ubwc_disabled = 0;
  SDMDebugHandler::Get()->GetProperty(DISABLE_UBWC_PROP, &ubwc_disabled);
  config.format = ubwc_disabled ? kFormatRGBA8888 : kFormatRGBA8888Ubwc;

  config.gfx_client = true;

  // Populate default params.
  config.secure = false;
  config.cache = false;
  config.secure_camera = false;

  int err = buffer_allocator_->AllocateBuffer(&buffer_info_);

  if (err != 0) {
    DLOGE("Failed to allocate buffer. Error: %d", error);
    return false;
  }

  return true;
}

void SDMDisplayBuiltIn::InitStitchTarget() {
  LayerBuffer buffer = {};
  buffer.planes[0].fd = buffer_info_.alloc_buffer_info.fd;
  buffer.planes[0].offset = 0;
  buffer.planes[0].stride = buffer_info_.alloc_buffer_info.stride;
  buffer.size = buffer_info_.alloc_buffer_info.size;
  buffer.handle_id = buffer_info_.alloc_buffer_info.id;
  buffer.width = buffer_info_.alloc_buffer_info.aligned_width;
  buffer.height = buffer_info_.alloc_buffer_info.aligned_height;
  buffer.unaligned_width = fb_config_.x_pixels;
  buffer.unaligned_height = fb_config_.y_pixels * kBufferHeightFactor;
  buffer.format = buffer_info_.alloc_buffer_info.format;

  Layer *sdm_stitch_target = stitch_target_->GetSDMLayer();
  sdm_stitch_target->composition = kCompositionStitchTarget;
  sdm_stitch_target->input_buffer = buffer;
  sdm_stitch_target->input_buffer.buffer_id =
      reinterpret_cast<uint64_t>(buffer_info_.private_data);
}

void SDMDisplayBuiltIn::AppendStitchLayer() {
  if (disable_layer_stitch_) {
    return;
  }

  // Append stitch target buffer to layer stack.
  Layer *sdm_stitch_target = stitch_target_->GetSDMLayer();
  sdm_stitch_target->composition = kCompositionStitchTarget;
  sdm_stitch_target->dst_rect = {0, 0, FLOAT(fb_config_.x_pixels),
                                 FLOAT(fb_config_.y_pixels)};
  sdm_stitch_target->layer_id = stitch_target_->GetId();
  sdm_stitch_target->geometry_changes = stitch_target_->GetGeometryChanges();
  layer_stack_.layers.push_back(sdm_stitch_target);
}

DisplayError SDMDisplayBuiltIn::HistogramEvent(int fd, uint32_t blob_id) {
  uint32_t panel_width = 0;
  uint32_t panel_height = 0;
  GetPanelResolution(&panel_width, &panel_height);
  callbacks_->NotifyHistogram(id_, fd, blob_id, panel_width, panel_height);
  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::PostInit() {
  auto status = InitLayerStitch();
  if (!status) {
    DLOGW("Failed to initialize Layer Stitch context");
    // Disable layer stitch.
    disable_layer_stitch_ = true;
  }

  return kErrorNone;
}

bool SDMDisplayBuiltIn::NeedsLargeCompPerfHint() {
  if (!cpu_hint_) {
    DLOGV_IF(kTagResources, "CPU hint is not initialized");
    return false;
  }

  if (active_refresh_rate_ < 120) {
    return false;
  }

  if (large_comp_hint_threshold_ > 0 &&
      sdm_layer_stack_->layer_set_.size() >= large_comp_hint_threshold_) {
    DLOGV_IF(
        kTagResources,
        "Number of app layers %d meet requirement %d. Set perf hint for large "
        "comp cycle",
        sdm_layer_stack_->layer_set_.size(), large_comp_hint_threshold_);
    return true;
  }

  // Send hints when the device is in multi-display or when a skip layer is
  // present.
  if (layer_stack_.flags.skip_present || is_multi_display_) {
    DLOGV_IF(
        kTagResources,
        "Found skip_layer:%d or is_multidisplay:%d. Set perf hint for large "
        "comp cycle",
        layer_stack_.flags.skip_present, is_multi_display_);
    return true;
  }

  int gpu_layer_count = 0;
  for (auto sdm_layer : sdm_layer_stack_->layer_set_) {
    Layer *layer = sdm_layer->GetSDMLayer();
    if (layer->composition == kCompositionGPU) {
      gpu_layer_count++;
    }
  }

  // Return immediately if full MDP comp is in use
  if (!gpu_layer_count) {
    return false;
  }

  auto it = mixed_mode_threshold_.find(active_refresh_rate_);
  if (it != mixed_mode_threshold_.end()) {
    if (gpu_layer_count < it->second) {
      DLOGV_IF(kTagResources,
               "Number of GPU layers :%d does not meet mixed mode perf hints "
               "threshold:%d for %d fps",
               gpu_layer_count, it->second, active_refresh_rate_);
      return false;
    }
  } else {
    DLOGV_IF(kTagResources, "Mixed mode perf hints is not supported for %d fps",
             active_refresh_rate_);
    return false;
  }

  // Send hints when the number of GPU layers reaches the threshold for the
  // active refresh rate.
  DLOGV_IF(
      kTagResources,
      "Reached max GPU layers for %dfps. Set perf hint for large comp cycle",
      active_refresh_rate_);
  return true;
}

DisplayError
SDMDisplayBuiltIn::PostCommitLayerStack(shared_ptr<Fence> *out_retire_fence) {
  DTRACE_SCOPED();
  HandleFrameOutput();
  PostCommitStitchLayers();

  auto status = SDMDisplay::PostCommitLayerStack(out_retire_fence);
  /*  display_intf_->GetConfig(&fixed_info);
    is_cmd_mode_ = fixed_info.is_cmdmode;

    // For video mode panel with dynamic fps, update the active mode index.
    // This is needed to report the correct Vsync period when client queries
    // using GetDisplayVsyncPeriod API.
    if (!is_cmd_mode_ && !disable_dyn_fps_) {
      Config active_config = sdm_config_map_.at(0);
      GetActiveConfig(&active_config);
      SetActiveConfigIndex(active_config);
    }*/

  pending_commit_ = false;

  if (layer_stack_.request_flags.trigger_refresh) {
    callbacks_->OnRefresh(id_);
  }

  return status;
}

bool SDMDisplayBuiltIn::IsDisplayIdle() {
  // Notify only if this display is source of vsync.
  bool vsync_source = (event_handler_->GetVsyncSource() == id_);
  return vsync_source && display_idle_;
}

bool SDMDisplayBuiltIn::HasReadBackBufferSupport() {
  DisplayConfigFixedInfo fixed_info = {};
  display_intf_->GetConfig(&fixed_info);

  return fixed_info.readback_supported;
}

DisplayError
SDMDisplayBuiltIn::NotifyDisplayCalibrationMode(bool in_calibration) {
  auto status = color_mode_->NotifyDisplayCalibrationMode(in_calibration);
  if (status != kErrorNone) {
    DLOGE("Failed for notify QDCM mode = %d", in_calibration);
    return status;
  }

  return status;
}

uint32_t SDMDisplayBuiltIn::GetUpdatingAppLayersCount() {
  uint32_t updating_count = 0;

  for (uint i = 0; i < layer_stack_.layers.size(); i++) {
    auto layer = layer_stack_.layers.at(i);
    if (layer->composition == kCompositionGPUTarget) {
      break;
    }
    if (layer->flags.updating) {
      updating_count++;
    }
  }

  return updating_count;
}

DisplayError SDMDisplayBuiltIn::CommitOrPrepare(
    bool validate_only, shared_ptr<Fence> *out_retire_fence,
    uint32_t *out_num_types, uint32_t *out_num_requests, bool *needs_commit) {
  DTRACE_SCOPED();

  SetDynamicDSIClock();

  auto status = SDMDisplay::CommitOrPrepare(validate_only, out_retire_fence,
                                            out_num_types, out_num_requests,
                                            needs_commit);

  if (perf_hint_large_comp_cycle_) {
    bool needs_hint = NeedsLargeCompPerfHint();
    HandleLargeCompositionHint(!needs_hint);
  }

  // Need a commit call to flush the dsi dynamic clock
  if (!(*needs_commit) && commit_counter_) {
  	commit_counter_ = false;
    callbacks_->OnRefresh(id_);
  }

  return status;
}

void SDMDisplayBuiltIn::LoadMixedModePerfHintThreshold() {
  // For mixed mode composition, if perf hint for large composition cycles is
  // enabled and if the use case meets the threshold, SF and SDM will be running
  // on the gold CPU cores.

  // For 120 fps, 8 layers should fall back to GPU
  mixed_mode_threshold_.insert(std::make_pair<int32_t, int32_t>(120, 8));

  // For 144 fps, 6 layers should fall back to GPU
  mixed_mode_threshold_.insert(std::make_pair<int32_t, int32_t>(144, 6));

  // TODO(user): Profile performance on 180 and 240 Hz without maxing out the
  // CPU cores For 180 fps, 8 layers should fall back to GPU
  mixed_mode_threshold_.insert(std::make_pair<int32_t, int32_t>(180, 8));

  // For 240 fps, 4 layers should fall back to GPU
  mixed_mode_threshold_.insert(std::make_pair<int32_t, int32_t>(240, 4));
}

DisplayError SDMDisplayBuiltIn::SetAlternateDisplayConfig(bool set) {
  Config alt_config = 0;
  DisplayError error = kErrorNone;

  // return early if non-DSC mode is already set
  if (set && alternate_config_ != -1) {
    return kErrorNone;
  }

  if (!set && alternate_config_ == -1) {
    return kErrorNone;
  }

  error = display_intf_->SetAlternateDisplayConfig(&alt_config);
  if (error != kErrorNone) {
    return kErrorNotSupported;
  }

  auto status = SetActiveConfig(alt_config);
  if (set && status == kErrorNone) {
    alternate_config_ = alt_config;
  }

  if (!set) { // set alternate config to -1 on reset call
    alternate_config_ = -1;
  }

  // Trigger refresh. This config gets applied on next commit.
  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDimmingEnable(int int_enabled) {
  DLOGV("Display ID: %" PRId64 " enabled: %d", id_, int_enabled);
  DisplayError error = display_intf_->SetDimmingEnable(int_enabled);

  if (error != kErrorNone) {
    DLOGE("Failed. enabled = %d, error = %d", int_enabled, error);
    return kErrorParameters;
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDimmingMinBl(int min_bl) {
  DLOGV("Display ID: %" PRId64 " min_bl: %d", id_, min_bl);
  DisplayError error = display_intf_->SetDimmingMinBl(min_bl);

  if (error != kErrorNone) {
    DLOGE("Failed. min_bl = %d, error = %d", min_bl, error);
    return kErrorParameters;
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::RetrieveDemuraTnFiles() {
  DLOGV("Display ID: %" PRId64, id_);
  DisplayError error = display_intf_->RetrieveDemuraTnFiles();

  if (error != kErrorNone) {
    DLOGE("Failed. error = %d", error);
    return kErrorParameters;
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::IsCacV2Supported(bool *supported) {
  uint32_t cac_supported = 0;
  auto error = display_intf_->IsSupportedOnDisplay(kCacV2, &cac_supported);
  *supported = cac_supported ? true : false;

  return error;
}

DisplayError SDMDisplayBuiltIn::PerformCacConfig(CacConfig config, bool enable) {
  DLOGV("Display ID: %" PRId64 " cac_enable: %d", id_, enable);
  DisplayError error = display_intf_->PerformCacConfig(config, enable);

  if (error != kErrorNone) {
    DLOGE("Failed to set CAC Config: %d error = %d", enable, error);
  }

  return error;
}

DisplayError SDMDisplayBuiltIn::SetDemuraState(int state) {
  DLOGV("Display ID: %" PRId64 " state: %d", id_, state);
  DisplayError error = display_intf_->SetDemuraState(state);

  if (error != kErrorNone) {
    DLOGE("Failed. state = %d, error = %d", state, error);
    return kErrorParameters;
  }

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetDemuraConfig(int demura_idx) {
  DLOGV("Display ID: %" PRId64 " config: %d", id_, demura_idx);
  DisplayError error = display_intf_->SetDemuraConfig(demura_idx);

  if (error != kErrorNone) {
    DLOGE("Failed. config = %d, error = %d", demura_idx, error);
    return kErrorParameters;
  }

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetABCState(bool state) {
  DLOGV("Display ID: %" PRId64 " state: %d", id_, state);
  DisplayError error = display_intf_->SetABCState(state);

  if (error != kErrorNone) {
    DLOGE("Failed. state = %d, error = %d", state, error);
    return kErrorParameters;
  }

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetABCReconfig() {
  DLOGV("Display ID: %" PRId64, id_);
  DisplayError error = display_intf_->SetABCReconfig();

  if (error != kErrorNone) {
    DLOGE("Failed to Reconfig ABC feature, error = %d", error);
    return kErrorParameters;
  }

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

DisplayError SDMDisplayBuiltIn::SetABCMode(string mode_name) {
  DLOGV("Display ID: %" PRId64 " mode name: %s", id_, mode_name.c_str());
  DisplayError error = display_intf_->SetABCMode(mode_name);

  if (error != kErrorNone) {
    DLOGE("Failed to Reconfig ABC feature, error = %d", error);
    return kErrorParameters;
  }

  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

void SDMDisplayBuiltIn::HandleLargeCompositionHint(bool release) {
  if (!cpu_hint_) {
    return;
  }

  int tid = gettid();

  if (release) {
    if (sdm_tid_ != tid) {
      DLOGV_IF(kTagResources, "SDM's tid:%d is updated to :%d", sdm_tid_, tid);
      int ret = cpu_hint_->ReqHint(kSDM, tid);
      if (!ret) {
        sdm_tid_ = tid;
      }
    }

    // For long term large composition hint, release the acquired handle after
    // 100 milliseconds to avoid resending hints in animation launch use cases
    // and others.
    if (hint_release_start_time_ == 0) {
      hint_release_start_time_ = callbacks_->SystemTime(SYSTEM_TIME_MONOTONIC);
    }

    nsecs_t current_time = callbacks_->SystemTime(SYSTEM_TIME_MONOTONIC);
    if (nanoseconds_to_milliseconds(current_time - hint_release_start_time_) >=
        elapse_time_threshold_) {
      cpu_hint_->ReqHintRelease();
    }
    return;
  }

  if (sdm_tid_ != tid) {
    DLOGV_IF(kTagResources, "SDM's tid:%d is updated to :%d", sdm_tid_, tid);
    cpu_hint_->ReqHintsOffload(kPerfHintLargeCompCycle, tid);
    sdm_tid_ = tid;
  } else {
    // Sending tid as 0 indicates to Perf HAL that SDM's tid is unchanged for
    // the current frame
    cpu_hint_->ReqHintsOffload(kPerfHintLargeCompCycle, 0);
  }

  // Reset time when large composition hint is active
  hint_release_start_time_ = 0;
}

void SDMDisplayBuiltIn::ReqPerfHintRelease() {
  if (!cpu_hint_) {
    return;
  }
  cpu_hint_->ReqHintRelease();
}

DisplayError SDMDisplayBuiltIn::SetSsrcMode(const std::string &mode) {
  DLOGV("Display ID: %" PRId64 " mode: %s", id_, mode.c_str());
  DisplayError error = display_intf_->SetSsrcMode(mode);

  if (error != kErrorNone) {
    DLOGE("Failed. mode = %s, error = %d", mode.c_str(), error);
    return kErrorParameters;
  }

  callbacks_->OnRefresh(id_);

  return error;
}

DisplayError SDMDisplayBuiltIn::SetupVRRConfig() {
  // Enable Variable Refresh Rate state
  DisplayError error = display_intf_->SetVRRState(true);
  if (error != kErrorNone) {
    return error;
  }

  for (auto &[config_id, config] : variable_config_map_) {
    if (config.avr_step > 0) {
      // Publish AVR Step period as the Vsync Period for an AVR Step enabled mode.
      config.vsync_period_ns = (1000.f / static_cast<float>(config.avr_step)) * 1000000;
    }
  }

  return error;
}

int SDMDisplayBuiltIn::GetNotifyEptConfig() {
  return notify_ept_heads_up_config_;
}

DisplayError SDMDisplayBuiltIn::SetPanelFeatureConfig(int32_t type, void *data) {
  return display_intf_->SetPanelFeatureConfig(type, data);
}

DisplayError SDMDisplayBuiltIn::EnableCopr(bool en) {
  return display_intf_->EnableCopr(en);
}

DisplayError SDMDisplayBuiltIn::GetCoprStats(std::vector<int> *stats) {
  return display_intf_->GetCoprStats(stats);
}

} // namespace sdm
