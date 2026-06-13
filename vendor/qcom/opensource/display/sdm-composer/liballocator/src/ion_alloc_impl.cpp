/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.

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

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#include <linux/dma-buf.h>
#include <linux/msm_ion.h>
#include <linux/msm_ion_ids.h>

#include <ion/ion.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <utils/constants.h>

#include "debug_handler.h"
#include "ion_alloc_impl.h"
#include "formats.h"

#define SZ_2M 0x200000
#define SZ_4K 0x1000
#define DEBUG 0
#define __CLASS__ "IonAllocator"

namespace sdm {

template <class Type>
inline Type ALIGN(Type x, Type align) {
  return (x + align - 1) & ~(align - 1);
}

IonAllocator* IonAllocator::ion_allocator_ = NULL;
int64_t IonAllocator::id_ = -1;

IonAllocator *IonAllocator::GetInstance() {
  if (!ion_allocator_) {
    ion_allocator_ = new IonAllocator();
    int error = ion_allocator_->Init();
    if (error != 0) {
      DLOGE("Ion initialization failed");
      return NULL;
    }
  }
  return ion_allocator_;
}

int IonAllocator::Init() {
  if (ion_dev_fd_ == -1) {
    ion_dev_fd_ = ion_open();
  }

  if (ion_dev_fd_ < 0) {
    DLOGE("%s: Failed to open ion device - %s", __FUNCTION__, strerror(errno));
    ion_dev_fd_ = -1;
    return -EINVAL;
  }

  return 0;
}

void IonAllocator::Deinit() {
  if (ion_dev_fd_ > -1) {
    ion_close(ion_dev_fd_);
  }

  ion_dev_fd_ = -1;
}

int IonAllocator::AllocBuffer(AllocData *data, BufferHandle *buffer_handle) {
  if (!data || !buffer_handle) {
    DLOGE("Invalid parameter data %p buffer_handle %p", data, buffer_handle);
    return -EINVAL;
  }

  DLOGD_IF(DEBUG, "AllocData WxHxF %dx%dx%d uncached %d usage_hints %x size %d", data->width,
           data->height, data->format, data->uncached, data->usage_hints, data->size);

  int err = 0;
  int fd = -1;
  uint32_t flags = 0;
  uint32_t heap_id = 0;
  // TODO(user): Revisit here
  uint32_t align = SZ_4K;
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  GetAlignedWidthAndHeight(data->width, data->height, &aligned_w, &aligned_h);
  uint32_t size = data->size;
  if (!size) {
    size = GetBufferSize(data->width, data->height, data->format);
  }

  GetIonHeapInfo(data, &heap_id, &flags);
  err = ion_alloc_fd(ion_dev_fd_, size, align, heap_id, flags, &fd);
  if (err) {
    DLOGE("Ion alloc failed ion_fd %d size %d align %d heap_id %x flags %x err %d",
          ion_dev_fd_, size, align, heap_id, flags, err);
    return err;
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

int IonAllocator::FreeBuffer(BufferHandle *buffer_handle) {
  if (!buffer_handle) {
    return -EINVAL;
  }
  close(buffer_handle->fd);

  return 0;
}

int IonAllocator::SyncBuffer(CacheOp op, int dma_buf_fd) {
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

int IonAllocator::MapBuffer(int fd, unsigned int size, void **base) {
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
    DLOGE("ion: Failed to map memory in the client: %s", strerror(errno));
  } else {
    DLOGD_IF(DEBUG, "ion: Mapped buffer base:%p size:%u fd:%d", addr, size, fd);
  }

  return err;
}

int IonAllocator::UnmapBuffer(void *base, unsigned int size) {
  if (!base || !size) {
    DLOGE("Invalid parameters base %p, size %d", base, size);
    return -EINVAL;
  }

  DLOGD_IF(DEBUG, "ion: Unmapping buffer  base:%p size:%u", base, size);

  int err = 0;
  if (munmap(base, size)) {
    err = -errno;
    DLOGE("ion: Failed to unmap memory at %p : %s", base, strerror(errno));
  }

  return err;
}

int IonAllocator::CloneBuffer(const CloneData &data, BufferHandle *buffer_handle) {
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

void IonAllocator::GetIonHeapInfo(AllocData *data, uint32_t *ion_heap_id,
                                         uint32_t *ion_flags) {
  uint32_t heap_id = ION_HEAP(ION_SYSTEM_HEAP_ID);
  uint32_t flags = 0;
  if (data->usage_hints.trusted_ui) {
    heap_id = ION_HEAP(ION_TUI_CARVEOUT_HEAP_ID);
  }

  flags |= data->uncached ? 0 : ION_FLAG_CACHED;

  *ion_flags = flags;
  *ion_heap_id = heap_id;
}

void IonAllocator::GetAlignedWidthAndHeight(int width, int height, uint32_t *aligned_width,
                                                   uint32_t *aligned_height) {
  if (!aligned_width || !aligned_height) {
    return;
  }

  *aligned_width = ALIGN(width, 32);
  *aligned_height = ALIGN(height, 32);
}

uint32_t IonAllocator::GetBufferSize(int width, int height, BufferFormat format) {
  uint32_t aligned_w = 0;
  uint32_t aligned_h = 0;
  GetAlignedWidthAndHeight(width, height, &aligned_w, &aligned_h);

  return (aligned_w * aligned_h * GetBpp(format));
}

}  // namespace sdm
