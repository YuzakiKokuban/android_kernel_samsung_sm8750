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
#include <stdarg.h>
#include <utils/constants.h>
#include <utils/debug.h>

#include "sdm_debugger.h"
#include "sdm_display_virtual.h"
#include <BufferUsage.h>

#define __CLASS__ "SDMDisplayVirtual"

namespace sdm {

using BufferUsage = vendor_qti_hardware_display_common_BufferUsage;

void SDMDisplayVirtual::Destroy(SDMDisplay *sdm_display) {
  sdm_display->Deinit();
  delete sdm_display;
}

SDMDisplayVirtual::SDMDisplayVirtual(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                                     SDMCompositorCallbacks *callbacks, Display id, int32_t sdm_id,
                                     uint32_t width, uint32_t height)
    : SDMDisplay(core_intf, buffer_allocator, callbacks, nullptr, kVirtual, id, sdm_id,
                 DISPLAY_CLASS_VIRTUAL),
      width_(width),
      height_(height) {}

DisplayError SDMDisplayVirtual::Init() {
  flush_on_error_ = true;
  return kErrorNone;
}

DisplayError SDMDisplayVirtual::Deinit() { return SDMDisplay::Deinit(); }

bool SDMDisplayVirtual::NeedsGPUBypass() {
  return display_paused_ || active_secure_sessions_.any() ||
         sdm_layer_stack_->layer_set_.empty();
}

DisplayError SDMDisplayVirtual::Present(shared_ptr<Fence> *out_retire_fence) {
  return kErrorNone;
}

DisplayError SDMDisplayVirtual::PreValidateDisplay(bool *exit_validate) {
  return kErrorNone;
}

DisplayError SDMDisplayVirtual::CommitOrPrepare(
    bool validate_only, shared_ptr<Fence> *out_retire_fence,
    uint32_t *out_num_types, uint32_t *out_num_requests, bool *needs_commit) {
  return kErrorNone;
}

DisplayError SDMDisplayVirtual::DumpVDSBuffer() {
  if (dump_frame_count_ && !flush_ && dump_output_layer_) {
    if (output_handle_) {
      BufferInfo buffer_info;
      SnapHandle *output_handle = (SnapHandle *) output_buffer_->buffer_id;
      vendor_qti_hardware_display_common_Address base_ptr;
      vendor_qti_hardware_display_common_Rect access_region = {0,0,0,0};
      vendor_qti_hardware_display_common_Fence snap_fence = {0};
      auto error = 
          snapmapper_->Lock(*output_handle, BufferUsage::CPU_READ_OFTEN,
                           access_region, snap_fence, &base_ptr);
      if (error != Error::NONE) {
        DLOGE("Failed to map output buffer, error = %d", error);
        dump_frame_index_ = dump_frame_count_ = 0;
        return kErrorParameters;
      }
      uint32_t width, height, alloc_size = 0;
      int32_t format = 0, flag = 0;
      int64_t compression_type, is_ubwc;
      snapmapper_->GetMetadata(*output_handle, MetadataType::STRIDE, &width);
      snapmapper_->GetMetadata(*output_handle, MetadataType::ALIGNED_HEIGHT_IN_PIXELS, &height);
      snapmapper_->GetMetadata(*output_handle, MetadataType::PIXEL_FORMAT_ALLOCATED, &format);
      snapmapper_->GetMetadata(*output_handle, MetadataType::ALLOCATION_SIZE, &alloc_size);
      snapmapper_->GetMetadata(*output_handle, MetadataType::COMPRESSION, &compression_type);
      snapmapper_->GetMetadata(*output_handle, MetadataType::IS_UBWC, &is_ubwc);

      buffer_info.buffer_config.width = width;
      buffer_info.buffer_config.height = height;
      flag = INT32(is_ubwc ? MetadataType::IS_UBWC : 0);
      buffer_info.buffer_config.format =
          buffer_allocator_->GetSDMFormat(format, flag, compression_type);
      buffer_info.alloc_buffer_info.aligned_width = width;
      buffer_info.alloc_buffer_info.aligned_height = height;
      buffer_info.alloc_buffer_info.size = alloc_size;
      DumpOutputBuffer(buffer_info, (void *)(base_ptr.addressPointer), layer_stack_.retire_fence);
      dump_frame_count_--;
      dump_frame_index_++;

      vendor_qti_hardware_display_common_Fence unmap_fence = {-1};
      error = snapmapper_->Unlock(*output_handle, &unmap_fence);
      if (error != Error::NONE) {
        DLOGE("Failed to unmap buffer, error = %d", error);
        return kErrorParameters;
      }
    } else {
      DLOGW("Output buffer handle is detected as null."
            "%d output frames are not dumped with index %d onwards for display "
            "%d-%d.",
            dump_frame_count_, dump_frame_index_, sdm_id_, type_);
      dump_frame_index_ = dump_frame_count_ = 0;
    }
  }

  return kErrorNone;
}

DisplayError
SDMDisplayVirtual::SetOutputBuffer(const SnapHandle *output_handle,
                                   shared_ptr<Fence> release_fence) {
  int output_handle_format = 0;
  int64_t output_compression_type, output_ubwc_flag;
  snapmapper_->GetMetadata(*output_handle, MetadataType::IS_UBWC, &output_ubwc_flag);
  snapmapper_->GetMetadata(*output_handle, MetadataType::PIXEL_FORMAT_ALLOCATED, &output_handle_format);
  snapmapper_->GetMetadata(*output_handle, MetadataType::COMPRESSION, &output_compression_type);
  ColorMetadata color_metadata = {};
  int ubwc_flag = output_ubwc_flag ? INT32(MetadataType::IS_UBWC) : 0;

  if (output_handle_format ==
      static_cast<int>(SDMPixelFormat::PIXEL_FORMAT_RGBA_8888)) {
    output_handle_format =
        static_cast<int>(SDMPixelFormat::PIXEL_FORMAT_RGBX_8888);
  }

  LayerBufferFormat new_sdm_format =
      buffer_allocator_->GetSDMFormat(output_handle_format, ubwc_flag, output_compression_type);
  if (new_sdm_format == kFormatInvalid) {
    return kErrorParameters;
  }

  if (sdm::SetCSC(output_handle, &color_metadata, snapmapper_) != Error::NONE) {
    return kErrorParameters;
  }

  output_buffer_->flags.secure = 0;
  output_buffer_->flags.video = 0;
  output_buffer_->buffer_id = reinterpret_cast<uint64_t>(output_handle);
  output_buffer_->format = new_sdm_format;
  output_buffer_->dataspace = color_metadata.dataspace;
  output_buffer_->matrixCoefficients = color_metadata.matrixCoefficients;
  output_buffer_->masteringDisplayInfo = color_metadata.masteringDisplayInfo;
  output_buffer_->contentLightLevel = color_metadata.contentLightLevel;
  output_buffer_->cRI = color_metadata.cRI;
  output_buffer_->dynamicMetadata = color_metadata.dynamicMetadata;
  output_handle_ = output_handle;

  // TZ Protected Buffer - L1
  BufferUsage usage;
  snapmapper_->GetMetadata(*output_handle, MetadataType::USAGE, &usage);
  if (usage & BufferUsage::PROTECTED) {
    output_buffer_->flags.secure = 1;
  }

  // ToDo: Need to extend for non-RGB formats
  int fd = 0;
  uint32_t width = 0;
  snapmapper_->GetMetadata(*output_handle, MetadataType::FD, &fd);
  snapmapper_->GetMetadata(*output_handle, MetadataType::STRIDE, &width);
  output_buffer_->planes[0].fd = fd;
  output_buffer_->planes[0].offset = 0;
  output_buffer_->planes[0].stride = width;

  output_buffer_->acquire_fence = release_fence;

  return kErrorNone;
}

DisplayError SDMDisplayVirtual::SetFrameDumpConfig(uint32_t count,
                                                   uint32_t bit_mask_layer_type,
                                                   int32_t format,
                                                   CwbConfig &cwb_config) {
  SDMDisplay::SetFrameDumpConfig(count, bit_mask_layer_type, format);
  dump_output_layer_ = ((bit_mask_layer_type & (1 << OUTPUT_LAYER_DUMP)) != 0);

  DLOGI("output_layer_dump_enable %d", dump_output_layer_);
  return kErrorNone;
}

DisplayError SDMDisplayVirtual::GetDisplayType(int32_t *out_type) {
  if (out_type == nullptr) {
    return kErrorParameters;
  }

  *out_type = INT32(SDMDisplayBasicType::kVirtual);

  return kErrorNone;
}

DisplayError SDMDisplayVirtual::SetColorMode(SDMColorMode mode) {
  return kErrorNone;
}

DisplayError SDMDisplayVirtual::SetColorModeWithRenderIntent(SDMColorMode mode,
                                                             SDMRenderIntent intent) {
  return kErrorNone;
}

} // namespace sdm
