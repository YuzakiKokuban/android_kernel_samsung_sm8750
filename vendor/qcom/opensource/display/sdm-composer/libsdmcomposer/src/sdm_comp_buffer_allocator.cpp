/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <core/buffer_allocator.h>
#include <utils/constants.h>
#include <utils/debug.h>

#include "sdm_comp_buffer_allocator.h"
#include "sdm_comp_debugger.h"
#include "formats.h"
#include "alloc_interface.h"

#define __CLASS__ "SDMCompBufferAllocator"

namespace sdm {

int SDMCompBufferAllocator::AllocateBuffer(BufferInfo *buffer_info) {
  AllocInterface *alloc_intf = AllocInterface::GetInstance();
  if (!alloc_intf) {
    return -ENOMEM;
  }
  const BufferConfig &buffer_config = buffer_info->buffer_config;
  AllocatedBufferInfo *alloc_buffer_info = &buffer_info->alloc_buffer_info;
  BufferHandle *buffer_handle = new BufferHandle();

  AllocData data = {};
  data.width = buffer_config.width;
  data.height = buffer_config.height;
  data.format = GetSDMCompFormat(buffer_config.format);
  data.uncached = true;
  data.usage_hints.trusted_ui = true;
  if (buffer_config.tui_demura) {
    data.usage_hints.trusted_ui = false;
    data.usage_hints.tui_demura = true;
  }

  int error = alloc_intf->AllocBuffer(&data, buffer_handle);
  if (error != 0) {
    DLOGE("Allocation failed WxHxF %dx%dx%d, uncached %d", data.width, data.height, data.format,
          data.uncached);
    delete buffer_handle;
    return -ENOMEM;
  }

  alloc_buffer_info->fd = buffer_handle->fd;
  alloc_buffer_info->stride = buffer_handle->stride_in_bytes;
  alloc_buffer_info->aligned_width = buffer_handle->aligned_width;
  alloc_buffer_info->aligned_height = buffer_handle->aligned_height;
  alloc_buffer_info->format = buffer_config.format;
  alloc_buffer_info->size = buffer_handle->size;
  alloc_buffer_info->id = buffer_handle->buffer_id;
  buffer_info->private_data = buffer_handle;

  return 0;
}

int SDMCompBufferAllocator::FreeBuffer(BufferInfo *buffer_info) {
  AllocInterface *alloc_intf = AllocInterface::GetInstance();
  if (!alloc_intf) {
    return -ENOMEM;
  }
  AllocatedBufferInfo &alloc_buffer_info = buffer_info->alloc_buffer_info;
  BufferHandle *buffer_handle = reinterpret_cast<BufferHandle *>(buffer_info->private_data);
  alloc_intf->FreeBuffer(buffer_handle);

  delete buffer_handle;
  alloc_buffer_info.fd = -1;
  alloc_buffer_info.stride = 0;
  alloc_buffer_info.size = 0;
  buffer_info->private_data = NULL;
  return 0;
}

uint32_t SDMCompBufferAllocator::GetBufferSize(BufferInfo *buffer_info) {
  const BufferConfig &buffer_config = buffer_info->buffer_config;
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  BufferFormat buf_format = GetSDMCompFormat(buffer_config.format);
  GetAlignedWidthAndHeight(buffer_config.width, buffer_config.height, &aligned_w, &aligned_h);

  return (aligned_w * aligned_h * GetBpp(buf_format));
}

int SDMCompBufferAllocator::GetAllocatedBufferInfo(
  const BufferConfig &buffer_config, AllocatedBufferInfo *allocated_buffer_info) {
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  BufferFormat buf_format = GetSDMCompFormat(buffer_config.format);
  GetAlignedWidthAndHeight(buffer_config.width, buffer_config.height, &aligned_w, &aligned_h);

  allocated_buffer_info->stride = aligned_w * GetBpp(buf_format);
  allocated_buffer_info->aligned_width = aligned_w;
  allocated_buffer_info->aligned_height = aligned_h;
  allocated_buffer_info->size = (aligned_w * aligned_h * GetBpp(buf_format));

  return 0;
}

int SDMCompBufferAllocator::GetBufferLayout(const AllocatedBufferInfo &buf_info, uint32_t stride[4],
                                            uint32_t offset[4], uint32_t *num_planes) {
  if (!num_planes) {
    return -EINVAL;
  }
  BufferFormat buf_format = GetSDMCompFormat(buf_info.format);

  *num_planes = 1;
  stride[0] = static_cast<uint32_t>(buf_info.aligned_width * GetBpp(buf_format));
  offset[0] = 0;

  return 0;
}

void SDMCompBufferAllocator::GetAlignedWidthAndHeight(int width, int height,
                                                      uint32_t *aligned_width,
                                                      uint32_t *aligned_height) {
  if (!aligned_width || !aligned_height) {
    return;
  }

  *aligned_width = ALIGN(width, 32);
  *aligned_height = ALIGN(height, 32);
}

int SDMCompBufferAllocator::SetBufferInfo(LayerBufferFormat format, int *target,
                                          uint64_t *flags) {
  return 0;
}
int SDMCompBufferAllocator::GetAlignedWidthAndHeight(int width, int height,
                                                     int format,
                                                     uint32_t alloc_type,
                                                     int *aligned_width,
                                                     int *aligned_height) {
  return 0;
}
bool SDMCompBufferAllocator::GetSDMColorSpace(const int int_dataspace,
                                              QtiDataspace *dataspace) {
  return false;
}
LayerBufferFormat
SDMCompBufferAllocator::GetSDMFormat(const int32_t &source, const int32_t flags,
                                     const int64_t compression_type) {
  return kFormatInvalid;
}
DisplayError
SDMCompBufferAllocator::ColorMetadataToDataspace(Dataspace ds,
                                                 uint32_t *int_dataspace) {
  return kErrorNone;
}
int32_t
SDMCompBufferAllocator::TranslateFromLegacyDataspace(const int32_t &legacy_ds) {
  return 0;
}

}  // namespace sdm
