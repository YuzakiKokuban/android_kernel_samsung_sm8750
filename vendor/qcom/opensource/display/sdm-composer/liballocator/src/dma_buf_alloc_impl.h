/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#ifndef __DMA_BUF_ALLOC_IMPL_H__
#define __DMA_BUF_ALLOC_IMPL_H__

#include <BufferAllocator/BufferAllocator.h>

#include "alloc_interface.h"

namespace sdm {

class DmaBufAllocator : public AllocInterface {
 public:
  ~DmaBufAllocator() { }
  virtual int AllocBuffer(AllocData *data, BufferHandle *buffer_handle);
  virtual int FreeBuffer(BufferHandle *buffer_handle);
  virtual int MapBuffer(int fd, unsigned int size, void **base);
  virtual int UnmapBuffer(void *base, unsigned int size);
  virtual int SyncBuffer(CacheOp op, int fd);
  virtual int CloneBuffer (const CloneData &data, BufferHandle *buffer_handle);

  static DmaBufAllocator *GetInstance();

 private:
  DmaBufAllocator() {}
  void GetHeapInfo(AllocData *data, std::string *heap_name);
  void GetAlignedWidthAndHeight(int width, int height, uint32_t *aligned_width,
                                uint32_t *aligned_height);
  uint32_t GetBufferSize(int width, int height, BufferFormat format);

  static DmaBufAllocator* dma_buf_allocator_;
  static BufferAllocator buffer_allocator_;
  static int64_t id_;
};

}  // namespace sdm

#endif  // __DMA_BUF_ALLOC_IMPL_H__
