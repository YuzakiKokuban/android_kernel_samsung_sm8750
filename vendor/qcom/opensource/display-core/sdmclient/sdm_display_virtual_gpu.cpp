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
#include "sdm_display_virtual_gpu.h"
#include "concurrency_mgr.h"
#include <Rect.h>

#define __CLASS__ "SDMDisplayVirtualGPU"

namespace sdm {

DisplayError SDMDisplayVirtualGPU::Init() {
  // Create client target.
  client_target_ = new SDMLayer(id_, buffer_allocator_);

  // Create Null Display interface.
  DisplayError error = core_intf_->CreateNullDisplay(&display_intf_);
  if (error != kErrorNone) {
    DLOGE(
        "Null Display create failed. Error = %d display_id = %d disp_intf = %p",
        error, sdm_id_, display_intf_);
    return error;
  }

  disable_animation_ = Debug::IsExtAnimDisabled();

  return SDMDisplayVirtual::Init();
}

DisplayError SDMDisplayVirtualGPU::Deinit() {
  // Destory color convert instance. This destroys thread and underlying GL
  // resources.
  color_convert_task_.PerformTask(ColorConvertTaskCode::kCodeDestroyInstance,
                                  nullptr);

  DisplayError error = core_intf_->DestroyNullDisplay(display_intf_);
  if (error != kErrorNone) {
    DLOGE("Null Display destroy failed. Error = %d", error);
    return error;
  }

  delete client_target_;

  for (auto sdm_layer : sdm_layer_stack_->layer_set_) {
    delete sdm_layer;
  }

  return kErrorNone;
}

SDMDisplayVirtualGPU::SDMDisplayVirtualGPU(CoreInterface *core_intf,
                                           BufferAllocator *buffer_allocator,
                                           SDMCompositorCallbacks *callbacks, Display id,
                                           int32_t sdm_id, uint32_t width, uint32_t height,
                                           float min_lum, float max_lum)
    : SDMDisplayVirtual(core_intf, buffer_allocator, callbacks, id, sdm_id, width, height),
      color_convert_task_(*this) {}

DisplayError SDMDisplayVirtualGPU::Validate(uint32_t *out_num_types,
                                            uint32_t *out_num_requests) {
  DTRACE_SCOPED();

  // Reset previous changes.
  layer_changes_.clear();
  layer_requests_.clear();

  // Mark all layers to GPU if there is no need to bypass.
  bool needs_gpu_bypass = NeedsGPUBypass() || FreezeScreen();
  for (auto sdm_layer : sdm_layer_stack_->layer_set_) {
    auto layer = sdm_layer->GetSDMLayer();
    layer->composition = needs_gpu_bypass ? kCompositionSDE : kCompositionGPU;

    if (needs_gpu_bypass) {
      if (sdm_layer->GetClientRequestedCompositionType() ==
          SDMCompositionType::COMP_CLIENT) {
        layer_changes_[sdm_layer->GetId()] = SDMCompositionType::COMP_DEVICE;
        layer_requests_[sdm_layer->GetId()] =
            SDMLayerRequest::ClearClientTarget;
      }
    } else {
      if (sdm_layer->GetClientRequestedCompositionType() !=
          SDMCompositionType::COMP_CLIENT) {
        layer_changes_[sdm_layer->GetId()] = SDMCompositionType::COMP_CLIENT;
      }
    }
  }

  // Derive client target dataspace based on the color mode - bug/115482728
  uint32_t client_target_dataspace = 0;
  Dataspace ds;
  GetColorMetadataFromColorMode(GetCurrentColorMode(), ds);
  buffer_allocator_->ColorMetadataToDataspace(ds, &client_target_dataspace);
  SetClientTargetDataSpace(static_cast<int32_t>(client_target_dataspace));

  *out_num_types = UINT32(layer_changes_.size());
  *out_num_requests = UINT32(layer_requests_.size());

  has_client_composition_ = !needs_gpu_bypass;
  validate_done_ = true;

  return ((*out_num_types > 0) ? kErrorNeedsCommit : kErrorNone);
}

DisplayError SDMDisplayVirtualGPU::CommitOrPrepare(
    bool validate_only, shared_ptr<Fence> *out_retire_fence,
    uint32_t *out_num_types, uint32_t *out_num_requests, bool *needs_commit) {
  // Perform validate and commit.
  auto status = Validate(out_num_types, out_num_requests);

  *needs_commit = true;
  return status;
}

DisplayError
SDMDisplayVirtualGPU::SetOutputBuffer(const SnapHandle *buf,
                                      shared_ptr<Fence> release_fence) {
  DisplayError error = SDMDisplayVirtual::SetOutputBuffer(buf, release_fence);
  if (error != kErrorNone) {
    return error;
  }

  snapmapper_->GetMetadata(*buf, MetadataType::STRIDE, &output_buffer_->width);
  snapmapper_->GetMetadata(*buf, MetadataType::ALIGNED_HEIGHT_IN_PIXELS, &output_buffer_->height);
  snapmapper_->GetMetadata(*buf, MetadataType::WIDTH, &output_buffer_->unaligned_width);
  snapmapper_->GetMetadata(*buf, MetadataType::HEIGHT, &output_buffer_->unaligned_height);

  // Update active dimensions.
  bool is_crop_set = false;
  snapmapper_->GetMetadataState(*buf, MetadataType::CROP, &is_crop_set);
  if (is_crop_set) {
    vendor_qti_hardware_display_common_Rect crop_rect;
    if (snapmapper_->GetMetadata(*buf, MetadataType::CROP, &crop_rect) == Error::NONE) {
      output_buffer_->unaligned_width = crop_rect.right;
      output_buffer_->unaligned_height = crop_rect.bottom;
      color_convert_task_.PerformTask(ColorConvertTaskCode::kCodeReset, nullptr);
    }
  }

  return kErrorNone;
}

DisplayError
SDMDisplayVirtualGPU::Present(shared_ptr<Fence> *out_retire_fence) {
  DTRACE_SCOPED();

  auto status = kErrorNone;

  if (!output_buffer_->buffer_id) {
    return kErrorResources;
  }

  if (NeedsGPUBypass()) {
    return status;
  }

  layer_stack_.output_buffer = output_buffer_;

  // Ensure that blit is initialized.
  // GPU context gets in secure or non-secure mode depending on output buffer
  // provided.
  color_convert_task_.PerformTask(ColorConvertTaskCode::kCodeGetInstance, nullptr);

  ColorConvertBlitContext ctx = {};

  Layer *sdm_layer = client_target_->GetSDMLayer();
  LayerBuffer &input_buffer = sdm_layer->input_buffer;
  ctx.src_hnd = (void *)input_buffer.buffer_id;
  ctx.dst_hnd = (void *)&output_handle_;
  ctx.dst_rect = {0, 0};
  ctx.dst_rect.right = FLOAT(output_buffer_->unaligned_width);
  ctx.dst_rect.bottom = FLOAT(output_buffer_->unaligned_height);
  ctx.src_acquire_fence = input_buffer.acquire_fence;
  ctx.dst_acquire_fence = output_buffer_->acquire_fence;

  color_convert_task_.PerformTask(ColorConvertTaskCode::kCodeBlit, &ctx);

  // todo blit
  DumpVDSBuffer();

  *out_retire_fence = ctx.release_fence;

  return status;
}

void SDMDisplayVirtualGPU::OnTask(const ColorConvertTaskCode &task_code,
                                  SyncTask<ColorConvertTaskCode>::TaskContext *task_context) {
  switch (task_code) {
    case ColorConvertTaskCode::kCodeGetInstance: {
      callbacks_->InitColorConvert(id_, output_buffer_->flags.secure);
    } break;
    case ColorConvertTaskCode::kCodeBlit: {
      DTRACE_SCOPED();
      ColorConvertBlitContext *ctx = reinterpret_cast<ColorConvertBlitContext *>(task_context);
      callbacks_->ColorConvertBlit(id_, ctx);
    } break;
    case ColorConvertTaskCode::kCodeReset: {
      DTRACE_SCOPED();
      callbacks_->ResetColorConvert(id_);
    } break;
    case ColorConvertTaskCode::kCodeDestroyInstance: {
      callbacks_->DestroyColorConvert(id_);
    } break;
  }
}

bool SDMDisplayVirtualGPU::FreezeScreen() {
  if (!disable_animation_) {
    return false;
  }

  bool freeze_screen = false;
  if (animating_ && !animation_in_progress_) {
    // Start of animation. GPU comp is needed.
    animation_in_progress_ = true;
  } else if (!animating_ && animation_in_progress_) {
    // End of animation. Start composing.
    animation_in_progress_ = false;
  } else if (animating_ && animation_in_progress_) {
    // Animation in progress...
    freeze_screen = true;
  }

  return freeze_screen;
}

} // namespace sdm
