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

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __ALLOC_INTERFACE_H__
#define __ALLOC_INTERFACE_H__

#include "buffer_interface.h"

namespace sdm {

enum CacheOp {
  kCacheReadStart = 0x1,    //!< Invalidate the cache before the buffer read
  kCacheReadDone,           //!< Flush the cache after the buffer read
  kCacheWriteStart,         //!< Invalidate the cache before the buffer write
  kCacheWriteDone,          //!< Flush the cache after the buffer write
  kCacheInvalidate,         //!< Invalidate the cache before the buffer read/write
  kCacheClean,              //!< Flush the cache after the buffer read/write
};

struct AllocData {
  bool uncached = false;          //!< Specifies the buffer to be cached or uncached
  uint32_t width;                 //!< Width of the buffer to be allocated.
  uint32_t height;                //!< Height of the buffer to be allocated.
  BufferFormat format;            //!< Format of the buffer to be allocated.
  uint32_t size;                  //!< Size of the buffer to be allocated. Allocator allocates the
                                  //!< buffer of this size, if size is non zero otherwise allocator
                                  //!< allocates the buffer based on width, height and format.
  struct UsageHints {
    union {
      struct {
        uint32_t trusted_ui : 1;  //!< Denotes buffer to be allocated from trusted ui heap.
        uint32_t tui_demura : 2;  //!< Denotes buffer to be allocated from trusted ui demura heap.
      };
      uint64_t hints;
    };
  };
  UsageHints usage_hints;         //!< Hints to know about the producer of the buffer
};

struct CloneData {
  int fd = -1;                    //!< Buffer fd to be cloned.
  uint32_t width;                 //!< Width of the buffer to be cloned.
  uint32_t height;                //!< Height of the buffer to be cloned.
  BufferFormat format;            //!< Format of the buffer to be cloned.
};

class AllocInterface {
 public:
  /*! @brief Method to get the instance of allocator interface.

    @details This function opens the ion device and provides the ion allocator interface

    @return Returns AllocInterface pointer on sucess otherwise NULL
  */
  static AllocInterface *GetInstance();


  /*! @brief Method to to allocate buffer for a given buffer attributes

    @param[in]  data - \link AllocData \endlink
    @param[out] buffer_handle - \link BufferHandle \endlink

    @return Returns 0 on sucess otherwise errno
  */
  virtual int AllocBuffer(AllocData *data, BufferHandle *buffer_handle) = 0;


  /*! @brief Method to deallocate buffer

    @param[in] buffer_handle - \link BufferHandle \endlink

    @return Returns 0 on sucess otherwise errno
  */
  virtual int FreeBuffer(BufferHandle *buffer_handle) = 0;


  /*! @brief This creates a new mapping in the virtual address space of the calling process and
       provides the pointer.

    @param[in] fd - fd of the buffer to be mapped
    @param[in] size - size of the buffer to be mapped
    @param[out] base - virtual base address after mapping

    @return Returns 0 on sucess otherwise errno
  */
  virtual int MapBuffer(int fd, unsigned int size, void **base) = 0;


  /*! @brief This function unmaps the buffer for the given pointer.

    @param[in] base - virtual base address to be unmapped
    @param[in] size - size of the buffer to be unmapped

    @return Returns 0 on sucess otherwise errno
  */
  virtual int UnmapBuffer(void *base, unsigned int size) = 0;


  /*! @brief This function helps to invalidate and flush the cache for the read write operation.

    @param[in] CacheOp - \link CacheOp \endlink
    @param[in] fd - fd of the buffer to be mapped

    @return Returns 0 on sucess otherwise errno
  */
  virtual int SyncBuffer(CacheOp op, int fd) = 0;

  /*! @brief This function helps to clone the buffer handle for the given buffer parameters

    @param[in] clone_data - \link CloneData \endlink
    @param[out] buffer_handle - \link BufferHandle \endlink

    @return Returns 0 on sucess otherwise errno
  */
  virtual int CloneBuffer(const CloneData &clone_data, BufferHandle *buffer_handle) = 0;

 protected:
  virtual ~AllocInterface() { }
};

}  // namespace sdm

#endif  // __ALLOC_INTERFACE_H__
