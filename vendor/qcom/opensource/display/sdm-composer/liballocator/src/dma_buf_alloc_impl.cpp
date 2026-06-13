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

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#include <linux/dma-buf.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <utils/constants.h>

#include <string>

#include "debug_handler.h"
#include "dma_buf_alloc_impl.h"
#include "formats.h"

#define SZ_4K 0x1000
#define DEBUG 0
#define __CLASS__ "DmaBufAllocator"

namespace sdm {

template <class Type>
inline Type ALIGN(Type x, Type align) {
  return (x + align - 1) & ~(align - 1);
}

DmaBufAllocator* DmaBufAllocator::dma_buf_allocator_ = NULL;
BufferAllocator DmaBufAllocator::buffer_allocator_ = {};
int64_t DmaBufAllocator::id_ = -1;

DmaBufAllocator *DmaBufAllocator::GetInstance() {
  if (!dma_buf_allocator_) {
    dma_buf_allocator_ = new DmaBufAllocator();
  }
  return dma_buf_allocator_;
}

int DmaBufAllocator::AllocBuffer(AllocData *data, BufferHandle *buffer_handle) {
  if (!data || !buffer_handle) {
    DLOGE("Invalid parameter data %p buffer_handle %p", data, buffer_handle);
    return -EINVAL;
  }

  DLOGD_IF(DEBUG, "AllocData WxHxF %dx%dx%d uncached %d usage_hints %x size %d", data->width,
           data->height, data->format, data->uncached, data->usage_hints, data->size);

  int fd = -1;
  uint32_t flags = 0;
  std::string heap_name = {};
  uint32_t align = SZ_4K;
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  GetAlignedWidthAndHeight(data->width, data->height, &aligned_w, &aligned_h);
  uint32_t size = data->size;
  if (!size) {
    size = GetBufferSize(data->width, data->height, data->format);
  }

  GetHeapInfo(data, &heap_name);
  fd = buffer_allocator_.Alloc(heap_name, size, flags, align);
  if (fd < 0) {
    DLOGE("Dmabuf alloc failed buf_fd %d size %d align %d heap_name %x flags %x",
          fd, size, align, heap_name.c_str(), flags);
    return -ENOMEM;
  }

  buffer_handle->fd = fd;
  buffer_handle->width = data->width;
  buffer_handle->height = data->height;
  buffer_handle->format = data->format;
  buffer_handle->aligned_width = aligned_w;
  buffer_handle->aligned_height = aligned_h;
  buffer_handle->size = size;
  buffer_handle->stride_in_bytes = aligned_w * GetBpp(data->format);
  buffer_handle->uncached = data->uncached;
  buffer_handle->buffer_id = id_++;

  DLOGD_IF(DEBUG, "Allocated buffer Aligned WxHxF %dx%dx%d stride_in_bytes %d size:%u fd:%d",
           buffer_handle->aligned_width, buffer_handle->aligned_height, buffer_handle->format,
           buffer_handle->stride_in_bytes, buffer_handle->size, buffer_handle->fd);

  return 0;
}

int DmaBufAllocator::FreeBuffer(BufferHandle *buffer_handle) {
  if (!buffer_handle) {
    return -EINVAL;
  }
  close(buffer_handle->fd);

  return 0;
}

int DmaBufAllocator::SyncBuffer(CacheOp op, int dma_buf_fd) {
  struct dma_buf_sync sync;
  int err = 0;

  switch (op) {
    case kCacheReadStart:
      sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
      break;
    case kCacheReadDone:
      sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
      break;
    case kCacheWriteStart:
      sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_WRITE;
      break;
    case kCacheWriteDone:
      sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_WRITE;
      break;
    case kCacheInvalidate:
      sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
      break;
    case kCacheClean:
      sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
      break;
    default:
      DLOGE("%s: Invalid operation %d", __FUNCTION__, op);
      return -1;
  }

  if (ioctl(dma_buf_fd, INT(DMA_BUF_IOCTL_SYNC), &sync)) {
    err = -errno;
    DLOGE("%s: DMA_BUF_IOCTL_SYNC failed with error - %s", __FUNCTION__, strerror(errno));
    return err;
  }

  return 0;
}

int DmaBufAllocator::MapBuffer(int fd, unsigned int size, void **base) {
  if (!base || fd < 0 || !size) {
    DLOGE("Invalid parameters base %p, fd %d, size %d", base, fd, size);
    return -EINVAL;
  }
  int err = 0;
  void *addr = 0;

  addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  *base = addr;
  if (addr == MAP_FAILED) {
    err = -errno;
    DLOGE("Failed to map memory in the client: %s", strerror(errno));
  } else {
    DLOGD_IF(DEBUG, "Mapped buffer base:%p size:%u fd:%d", addr, size, fd);
  }

  return err;
}

int DmaBufAllocator::UnmapBuffer(void *base, unsigned int size) {
  if (!base || !size) {
    DLOGE("Invalid parameters base %p, size %d", base, size);
    return -EINVAL;
  }

  DLOGD_IF(DEBUG, "Unmapping buffer  base:%p size:%u", base, size);

  int err = 0;
  if (munmap(base, size)) {
    err = -errno;
    DLOGE("Failed to unmap memory at %p : %s", base, strerror(errno));
  }

  return err;
}

int DmaBufAllocator::CloneBuffer(const CloneData &data, BufferHandle *buffer_handle) {
  if (!buffer_handle) {
    DLOGE("Invalid parameter data %p buffer_handle %p", buffer_handle);
    return -EINVAL;
  }

  DLOGD_IF(DEBUG, "CloneData WxHxF %dx%dx%d fd %d", data.width, data.height, data.format, data.fd);

  int err = 0;
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  GetAlignedWidthAndHeight(data.width, data.height, &aligned_w, &aligned_h);
  uint32_t size = GetBufferSize(data.width, data.height, data.format);

  buffer_handle->fd = dup(data.fd);
  buffer_handle->width = data.width;
  buffer_handle->height = data.height;
  buffer_handle->format = data.format;
  buffer_handle->aligned_width = aligned_w;
  buffer_handle->aligned_height = aligned_h;
  buffer_handle->size = size;
  buffer_handle->stride_in_bytes = aligned_w * GetBpp(data.format);
  buffer_handle->buffer_id = id_++;

  DLOGD_IF(DEBUG, "Cloned buffer Aligned WxHxF %dx%dx%d stride_in_bytes %d size:%u fd:%d",
           buffer_handle->aligned_width, buffer_handle->aligned_height, buffer_handle->format,
           buffer_handle->stride_in_bytes, buffer_handle->size, buffer_handle->fd);

  return 0;
}

void DmaBufAllocator::GetHeapInfo(AllocData *data, std::string *heap_name) {
  if (data->usage_hints.trusted_ui) {
    *heap_name = "qcom,tui";
  } else if (data->usage_hints.tui_demura) {
    *heap_name = "qcom,tui_demura";
  }
}

void DmaBufAllocator::GetAlignedWidthAndHeight(int width, int height, uint32_t *aligned_width,
                                                   uint32_t *aligned_height) {
  if (!aligned_width || !aligned_height) {
    return;
  }

  *aligned_width = ALIGN(width, 32);
  *aligned_height = ALIGN(height, 32);
}

uint32_t DmaBufAllocator::GetBufferSize(int width, int height, BufferFormat format) {
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  GetAlignedWidthAndHeight(width, height, &aligned_w, &aligned_h);

  return (aligned_w * aligned_h * GetBpp(format));
}

}  // namespace sdm
