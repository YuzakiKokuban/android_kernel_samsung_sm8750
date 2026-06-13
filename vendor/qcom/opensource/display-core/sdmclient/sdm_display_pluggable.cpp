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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <algorithm>
#include <utils/constants.h>
#include <utils/debug.h>

#include "sdm_debugger.h"
#include "sdm_display_pluggable.h"

#define __CLASS__ "SDMDisplayPluggable"

namespace sdm {

DisplayError SDMDisplayPluggable::Create(
    CoreInterface *core_intf, BufferAllocator *buffer_allocator, SDMCompositorCallbacks *callbacks,
    SDMDisplayEventHandler *event_handler, Display id, int32_t sdm_id, uint32_t primary_width,
    uint32_t primary_height, bool use_primary_res, SDMDisplay **sdm_display) {
  uint32_t pluggable_width = 0;
  uint32_t pluggable_height = 0;
  DisplayError error = kErrorNone;

  SDMDisplay *sdm_display_pluggable = new SDMDisplayPluggable(
      core_intf, buffer_allocator, callbacks, event_handler, id, sdm_id);
  auto status = sdm_display_pluggable->Init();
  if (status) {
    delete sdm_display_pluggable;
    return status;
  }

  error = sdm_display_pluggable->GetMixerResolution(&pluggable_width,
                                                    &pluggable_height);
  if (error != kErrorNone) {
    Destroy(sdm_display_pluggable);
    return error;
  }

  if (primary_width && primary_height) {
    // use_primary_res means SDMDisplayPluggable should directly set framebuffer
    // resolution to the provided primary_width and primary_height
    if (use_primary_res) {
      pluggable_width = primary_width;
      pluggable_height = primary_height;
    } else {
      int downscale_enabled = 0;
      SDMDebugHandler::Get()->GetProperty(ENABLE_EXTERNAL_DOWNSCALE_PROP,
                                          &downscale_enabled);
      if (downscale_enabled) {
        GetDownscaleResolution(primary_width, primary_height, &pluggable_width,
                               &pluggable_height);
      }
    }
  }

  status = sdm_display_pluggable->SetFrameBufferResolution(pluggable_width,
                                                           pluggable_height);
  if (status) {
    Destroy(sdm_display_pluggable);
    return status;
  }

  *sdm_display = sdm_display_pluggable;

  return status;
}

DisplayError SDMDisplayPluggable::Init() {
  auto status = SDMDisplay::Init();
  if (status) {
    return status;
  }
  color_mode_ = new SDMColorModeMgr(display_intf_);
  color_mode_->Init();

  SDMDisplay::TryDrawMethod(DisplayDrawMethod::kDrawUnified);

  return status;
}

void SDMDisplayPluggable::Destroy(SDMDisplay *sdm_display) {
  // Flush the display to have outstanding fences signaled.
  sdm_display->Flush();
  sdm_display->Deinit();
  delete sdm_display;
}

SDMDisplayPluggable::SDMDisplayPluggable(CoreInterface *core_intf,
                                         BufferAllocator *buffer_allocator,
                                         SDMCompositorCallbacks *callbacks,
                                         SDMDisplayEventHandler *event_handler, Display id,
                                         int32_t sdm_id)
    : SDMDisplay(core_intf, buffer_allocator, callbacks, event_handler, kPluggable, id, sdm_id,
                 DISPLAY_CLASS_PLUGGABLE) {}

DisplayError SDMDisplayPluggable::PreValidateDisplay(bool *exit_validate) {
  DTRACE_SCOPED();

  // Draw method gets set as part of first commit.
  SetDrawMethod();

  auto status = kErrorNone;
  bool res_exhausted = false;
  // If no resources are available for the current display, mark it for GPU by
  // pass and continue to do invalidate until the resources are available
  if (active_secure_sessions_[kSecureDisplay] || display_paused_ ||
      (mmrm_restricted_ &&
       (current_power_mode_ == SDMPowerMode::POWER_MODE_OFF ||
        current_power_mode_ == SDMPowerMode::POWER_MODE_DOZE_SUSPEND)) ||
      CheckResourceState(&res_exhausted)) {
    MarkLayersForGPUBypass();
    *exit_validate = true;
    return status;
  }

  BuildLayerStack();

  if (sdm_layer_stack_->layer_set_.empty()) {
    flush_ = !client_connected_;
    *exit_validate = true;
    return status;
  }

  // Apply current Color Mode and Render Intent.
  status = color_mode_->ApplyCurrentColorModeWithRenderIntent(
      static_cast<bool>(layer_stack_.flags.hdr_present));
  if (status != kErrorNone || has_color_tranform_) {
    // Fallback to GPU Composition if Color Mode can't be applied or if a color
    // tranform needs to be applied.
    MarkLayersForClientComposition();
  }

  *exit_validate = false;

  return status;
}

DisplayError SDMDisplayPluggable::Validate(uint32_t *out_num_types,
                                           uint32_t *out_num_requests) {
  bool exit_validate = false;
  auto status = PreValidateDisplay(&exit_validate);
  if (exit_validate) {
    return status;
  }

  // TODO(user): SetRefreshRate need to follow new interface when added.

  return PrepareLayerStack(out_num_types, out_num_requests);
}

DisplayError
SDMDisplayPluggable::PostCommitLayerStack(shared_ptr<Fence> *out_retire_fence) {
  DTRACE_SCOPED();
  auto status = kErrorNone;

  HandleFrameOutput();
  status = SDMDisplay::PostCommitLayerStack(out_retire_fence);

  return status;
}

DisplayError SDMDisplayPluggable::Present(shared_ptr<Fence> *out_retire_fence) {
  auto status = kErrorNone;
  bool res_exhausted = false;

  if (!active_secure_sessions_[kSecureDisplay] && !display_paused_ &&
      !(mmrm_restricted_ &&
        (current_power_mode_ == SDMPowerMode::POWER_MODE_OFF ||
         current_power_mode_ == SDMPowerMode::POWER_MODE_DOZE_SUSPEND))) {
    // Proceed only if any resources are available to be allocated for the
    // current display, Otherwise keep doing invalidate
    if (CheckResourceState(&res_exhausted)) {
      Refresh();
      return status;
    }

    status = SDMDisplay::CommitLayerStack();
    if (status == kErrorNone) {
      status = PostCommitLayerStack(out_retire_fence);
    }
  }
  return status;
}

void SDMDisplayPluggable::ApplyScanAdjustment(SDMRect *display_frame) {
  if ((underscan_width_ <= 0) || (underscan_height_ <= 0)) {
    return;
  }

  float width_ratio = FLOAT(underscan_width_) / 100.0f;
  float height_ratio = FLOAT(underscan_height_) / 100.0f;

  uint32_t mixer_width = 0;
  uint32_t mixer_height = 0;
  GetMixerResolution(&mixer_width, &mixer_height);

  if (mixer_width == 0 || mixer_height == 0) {
    DLOGV("Invalid mixer dimensions (%d, %d)", mixer_width, mixer_height);
    return;
  }

  uint32_t new_mixer_width = UINT32(mixer_width * FLOAT(1.0f - width_ratio));
  uint32_t new_mixer_height = UINT32(mixer_height * FLOAT(1.0f - height_ratio));

  int x_offset = INT((FLOAT(mixer_width) * width_ratio) / 2.0f);
  int y_offset = INT((FLOAT(mixer_height) * height_ratio) / 2.0f);

  display_frame->left =
      (display_frame->left * INT32(new_mixer_width) / INT32(mixer_width)) +
      x_offset;
  display_frame->top =
      (display_frame->top * INT32(new_mixer_height) / INT32(mixer_height)) +
      y_offset;
  display_frame->right =
      ((display_frame->right * INT32(new_mixer_width)) / INT32(mixer_width)) +
      x_offset;
  display_frame->bottom = ((display_frame->bottom * INT32(new_mixer_height)) /
                           INT32(mixer_height)) +
                          y_offset;
}

static void AdjustSourceResolution(uint32_t dst_width, uint32_t dst_height,
                                   uint32_t *src_width, uint32_t *src_height) {
  *src_height = (dst_width * (*src_height)) / (*src_width);
  *src_width = dst_width;
}

void SDMDisplayPluggable::GetDownscaleResolution(uint32_t primary_width,
                                                 uint32_t primary_height,
                                                 uint32_t *non_primary_width,
                                                 uint32_t *non_primary_height) {
  uint32_t primary_area = primary_width * primary_height;
  uint32_t non_primary_area = (*non_primary_width) * (*non_primary_height);

  if (primary_area > non_primary_area) {
    if (primary_height > primary_width) {
      std::swap(primary_height, primary_width);
    }
    AdjustSourceResolution(primary_width, primary_height, non_primary_width,
                           non_primary_height);
  }
}

void SDMDisplayPluggable::GetUnderScanConfig() {
  if (!display_intf_->IsUnderscanSupported()) {
    // Read user defined underscan width and height
    SDMDebugHandler::Get()->GetProperty(EXTERNAL_ACTION_SAFE_WIDTH_PROP,
                                        &underscan_width_);
    SDMDebugHandler::Get()->GetProperty(EXTERNAL_ACTION_SAFE_HEIGHT_PROP,
                                        &underscan_height_);
  }
}

DisplayError SDMDisplayPluggable::Flush() {
  return display_intf_->Flush(&layer_stack_);
}

DisplayError SDMDisplayPluggable::GetColorModes(uint32_t *out_num_modes,
                                                SDMColorMode *out_modes) {
  if (out_modes == nullptr) {
    *out_num_modes = color_mode_->GetColorModeCount();
  } else {
    color_mode_->GetColorModes(out_num_modes, out_modes);
  }
  return kErrorNone;
}

DisplayError
SDMDisplayPluggable::GetRenderIntents(SDMColorMode mode,
                                      uint32_t *out_num_intents,
                                      SDMRenderIntent *out_intents) {
  if (out_intents == nullptr) {
    *out_num_intents = color_mode_->GetRenderIntentCount(mode);
  } else {
    color_mode_->GetRenderIntents(mode, out_num_intents, out_intents);
  }
  return kErrorNone;
}

DisplayError SDMDisplayPluggable::SetColorMode(SDMColorMode mode) {
  return SetColorModeWithRenderIntent(mode, SDMRenderIntent::COLORIMETRIC);
}

DisplayError
SDMDisplayPluggable::SetColorModeWithRenderIntent(SDMColorMode mode,
                                                  SDMRenderIntent intent) {
  auto status = color_mode_->CacheColorModeWithRenderIntent(mode, intent);
  if (status != kErrorNone) {
    DLOGE("failed for mode = %d intent = %d", mode, intent);
    return status;
  }

  callbacks_->OnRefresh(id_);

  return status;
}

DisplayError SDMDisplayPluggable::SetColorTransform(const float *matrix, SDMColorTransform hint) {
  if (hint == SDMColorTransform::TRANSFORM_IDENTITY) {
    has_color_tranform_ = false;
    // From 2.1 IComposerClient.hal:
    // If the device is not capable of either using the hint or the matrix to
    // apply the desired color transform, it must force all layers to client
    // composition during VALIDATE_DISPLAY.
  } else {
    // Also, interpret HAL_COLOR_TRANSFORM_ARBITRARY_MATRIX hint as non-identity
    // matrix.
    has_color_tranform_ = true;
  }

  geometry_changes_ |= GeometryChanges::kColorTransform;
  callbacks_->OnRefresh(id_);

  return kErrorNone;
}

} // namespace sdm
