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
#ifndef __SDM_COMP_BUFFER_ALLOCATOR_H__
#define __SDM_COMP_BUFFER_ALLOCATOR_H__

#include <fcntl.h>
#include <sys/mman.h>

#include <core/sdm_types.h>
#include <core/buffer_allocator.h>

namespace sdm {

template <class Type>
inline Type ALIGN(Type x, Type align) {
  return (x + align - 1) & ~(align - 1);
}

class SDMCompBufferAllocator : public BufferAllocator {
 public:
  virtual int AllocateBuffer(BufferInfo *buffer_info);
  virtual int FreeBuffer(BufferInfo *buffer_info);
  virtual uint32_t GetBufferSize(BufferInfo *buffer_info);
  virtual int GetAllocatedBufferInfo(const BufferConfig &buffer_config,
                                     AllocatedBufferInfo *allocated_buffer_info);
  virtual int GetBufferLayout(const AllocatedBufferInfo &buf_info, uint32_t stride[4],
                              uint32_t offset[4], uint32_t *num_planes);

  virtual int SetBufferInfo(LayerBufferFormat format, int *target,
                            uint64_t *flags);
  virtual int GetAlignedWidthAndHeight(int width, int height, int format,
                                       uint32_t alloc_type, int *aligned_width,
                                       int *aligned_height);
  virtual bool GetSDMColorSpace(const int int_dataspace,
                                QtiDataspace *dataspace);
  virtual LayerBufferFormat GetSDMFormat(const int32_t &source,
                                         const int32_t flags,
                                         const int64_t compression_type);
  virtual DisplayError ColorMetadataToDataspace(Dataspace ds,
                                                uint32_t *int_dataspace);
  virtual int32_t TranslateFromLegacyDataspace(const int32_t &legacy_ds);

private:
  void GetAlignedWidthAndHeight(int width, int height, uint32_t *aligned_width,
                                uint32_t *aligned_height);
};

}  // namespace sdm
#endif  // __SDM_COMP_BUFFER_ALLOCATOR_H__
