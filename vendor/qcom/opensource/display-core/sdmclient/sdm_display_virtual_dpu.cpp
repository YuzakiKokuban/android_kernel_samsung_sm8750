/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

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
#include "sdm_display_virtual_dpu.h"
#include <BufferDescriptor.h>

#define __CLASS__ "SDMDisplayVirtualDPU"

namespace sdm {

using vendor::qti::hardware::display::snapalloc::BufferDescriptor;
using BufferUsage = vendor_qti_hardware_display_common_BufferUsage;
using KeyValuePair = vendor_qti_hardware_display_common_KeyValuePair;

SDMDisplayVirtualDPU::SDMDisplayVirtualDPU(CoreInterface *core_intf,
                                           BufferAllocator *buffer_allocator,
                                           SDMCompositorCallbacks *callbacks, Display id,
                                           int32_t sdm_id, uint32_t width, uint32_t height,
                                           float min_lum, float max_lum)
    : SDMDisplayVirtual(core_intf, buffer_allocator, callbacks, id, sdm_id, width, height),
      min_lum_(min_lum),
      max_lum_(max_lum) {}

DisplayError SDMDisplayVirtualDPU::Init() {
  DisplayError status = SDMDisplay::Init();
  if (status) {
    DLOGE("Init failed: %d", status);
    return status;
  }

  if (max_lum_ != -1.0 || min_lum_ != -1.0) {
    SetPanelLuminanceAttributes(min_lum_, max_lum_);
  }

  status = SetConfig(width_, height_);
  if (status) {
    DLOGE("Failed to set width: %d height: %d", width_, height_);
    return status;
  }

  status = SetPowerMode(SDMPowerMode::POWER_MODE_ON, false /* teardown */);
  if (status) {
    DLOGW("Failed to set power mode on virtual display");
    return status;
  }

  // TODO(user): Validate that we support this width/height
  status = SetFrameBufferResolution(width_, height_);
  if (status != kErrorNone) {
    DLOGW("Failed to set FrameBuffer resolution on virtual display");
    return status;
  }

  return SDMDisplayVirtual::Init();
}

DisplayError SDMDisplayVirtualDPU::SetConfig(uint32_t width, uint32_t height) {
  DisplayConfigVariableInfo variable_info;
  variable_info.x_pixels = width;
  variable_info.y_pixels = height;
  // TODO(user): Need to get the framerate of primary display and update it.
  variable_info.fps = 60;

  DisplayError err = display_intf_->SetActiveConfig(&variable_info);
  if (err != kErrorNone) {
    return err;
  }
  return kErrorNone;
}

DisplayError
SDMDisplayVirtualDPU::SetOutputBuffer(const SnapHandle *output_handle,
                                      shared_ptr<Fence> release_fence) {
  DisplayError error =
      SDMDisplayVirtual::SetOutputBuffer(output_handle, release_fence);
  if (error != kErrorNone) {
    return error;
  }

  int output_handle_format;
  uint64_t modifier;
  snapmapper_->GetMetadata(*output_handle, MetadataType::PIXEL_FORMAT_ALLOCATED,
                           &output_handle_format);
  snapmapper_->GetMetadata(*output_handle, MetadataType::FORMAT_MODIFIER, &modifier);
  KeyValuePair modifier_pair = {"pixel_format_modifier", modifier};

  int active_aligned_w, active_aligned_h;
  int new_width, new_height;
  int new_aligned_w = 0, new_aligned_h = 0;
  uint32_t active_width, active_height;

  GetMixerResolution(&active_width, &active_height);

  snapmapper_->GetMetadata(*output_handle, MetadataType::CUSTOM_DIMENSIONS_STRIDE, &new_width);
  snapmapper_->GetMetadata(*output_handle, MetadataType::CUSTOM_DIMENSIONS_HEIGHT, &new_height);

  BufferUsage usage;
  snapmapper_->GetMetadata(*output_handle, MetadataType::USAGE, &usage);

  // Get new aligned width/height
  BufferDescriptor new_descriptor;
  new_descriptor.width = new_width;
  new_descriptor.height = new_height;
  new_descriptor.format =
      static_cast<vendor_qti_hardware_display_common_PixelFormat>(output_handle_format);
  new_descriptor.usage = usage;
  new_descriptor.layerCount = 1;
  new_descriptor.additionalOptions.push_back(modifier_pair);
  snapmapper_->GetFromBufferDescriptor(new_descriptor, MetadataType::ALIGNED_WIDTH_IN_PIXELS,
                                       &new_aligned_w);
  snapmapper_->GetFromBufferDescriptor(new_descriptor, MetadataType::ALIGNED_HEIGHT_IN_PIXELS,
                                       &new_aligned_h);

  // Get active aligned width/height
  BufferDescriptor active_descriptor;
  active_descriptor.width = active_width;
  active_descriptor.height = active_height;
  active_descriptor.format =
      static_cast<vendor_qti_hardware_display_common_PixelFormat>(output_handle_format);
  active_descriptor.usage = usage;
  active_descriptor.layerCount = 1;
  active_descriptor.additionalOptions.push_back(modifier_pair);
  snapmapper_->GetFromBufferDescriptor(active_descriptor, MetadataType::ALIGNED_WIDTH_IN_PIXELS,
                                       &active_aligned_w);
  snapmapper_->GetFromBufferDescriptor(active_descriptor, MetadataType::ALIGNED_HEIGHT_IN_PIXELS,
                                       &active_aligned_h);

  if (new_aligned_w != active_aligned_w || new_aligned_h != active_aligned_h) {
    auto status = SetConfig(UINT32(new_width), UINT32(new_height));
    if (status != kErrorNone) {
      DLOGE("SetConfig failed custom WxH %dx%d", new_width, new_height);
      return status;
    }
  }

  output_buffer_->width = UINT32(new_aligned_w);
  output_buffer_->height = UINT32(new_aligned_h);
  output_buffer_->unaligned_width = UINT32(new_width);
  output_buffer_->unaligned_height = UINT32(new_height);

  return kErrorNone;
}

DisplayError SDMDisplayVirtualDPU::PreValidateDisplay(bool *exit_validate) {
  // Draw method gets set as part of first commit.
  SetDrawMethod();

  if (NeedsGPUBypass()) {
    MarkLayersForGPUBypass();
    *exit_validate = true;
    return kErrorNone;
  }

  BuildLayerStack();

  // Client(SurfaceFlinger) doesn't retain framebuffer post GPU composition.
  // This can result in flickers in cached framebuffer is used.
  for (auto &layer : layer_stack_.layers) {
    layer->flags.updating = true;
  }

  layer_stack_.output_buffer = output_buffer_;
  // If Output buffer of Virtual Display is not secure, set SKIP flag on the
  // secure layers.
  if (!output_buffer_->flags.secure && layer_stack_.flags.secure_present) {
    for (auto sdm_layer : sdm_layer_stack_->layer_set_) {
      Layer *layer = sdm_layer->GetSDMLayer();
      if (layer->input_buffer.flags.secure) {
        layer_stack_.flags.skip_present = true;
        layer->flags.skip = true;
      }
    }
  }

  if (force_gpu_comp_ && !layer_stack_.flags.secure_present) {
    MarkLayersForClientComposition();
  }

  *exit_validate = false;

  return kErrorNone;
}

DisplayError SDMDisplayVirtualDPU::Validate(uint32_t *out_num_types,
                                            uint32_t *out_num_requests) {
  bool exit_validate = false;
  auto status = PreValidateDisplay(&exit_validate);
  if (exit_validate) {
    return status;
  }

  return PrepareLayerStack(out_num_types, out_num_requests);
}

DisplayError
SDMDisplayVirtualDPU::Present(shared_ptr<Fence> *out_retire_fence) {
  auto status = kErrorNone;

  if (!output_buffer_->buffer_id) {
    return kErrorResources;
  }

  if (NeedsGPUBypass()) {
    return kErrorNone;
  }

  layer_stack_.output_buffer = output_buffer_;

  status = SDMDisplay::CommitLayerStack();
  if (status != kErrorNone) {
    return status;
  }

  status = PostCommitLayerStack(out_retire_fence);

  return status;
}

DisplayError SDMDisplayVirtualDPU::PostCommitLayerStack(
    shared_ptr<Fence> *out_retire_fence) {
  DTRACE_SCOPED();
  // Retire fence points to WB done.
  // Explicitly query for output buffer acquire fence.
  display_intf_->GetOutputBufferAcquireFence(&layer_stack_.retire_fence);

  DumpVDSBuffer();

  auto status = SDMDisplay::PostCommitLayerStack(out_retire_fence);

  return status;
}

DisplayError SDMDisplayVirtualDPU::CommitOrPrepare(
    bool validate_only, shared_ptr<Fence> *out_retire_fence,
    uint32_t *out_num_types, uint32_t *out_num_requests, bool *needs_commit) {
  DTRACE_SCOPED();

  layer_stack_.output_buffer = output_buffer_;
  auto status = SDMDisplay::CommitOrPrepare(validate_only, out_retire_fence,
                                            out_num_types, out_num_requests,
                                            needs_commit);
  return status;
}

DisplayError SDMDisplayVirtualDPU::SetPanelLuminanceAttributes(float min_lum,
                                                               float max_lum) {
  DisplayError err =
      display_intf_->SetPanelLuminanceAttributes(min_lum, max_lum);
  if (err != kErrorNone) {
    return kErrorParameters;
  }
  return kErrorNone;
}

DisplayError SDMDisplayVirtualDPU::SetColorTransform(const float *matrix, SDMColorTransform hint) {
  force_gpu_comp_ = (hint != SDMColorTransform::TRANSFORM_IDENTITY) ? true : false;
  return kErrorNone;
}

} // namespace sdm
