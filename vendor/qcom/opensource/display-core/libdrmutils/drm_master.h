/*
* Copyright (c) 2017, 2021 The Linux Foundation. All rights reserved.
*
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

#ifndef __DRM_MASTER_H__
#define __DRM_MASTER_H__

#include <utils/multi_core_instantiator.h>
#include <mutex>
#include <map>
#include <cstring>

#include "drm_logger.h"

namespace drm_utils {

#define MAX_RETRY 10

struct DRMBuffer {
  int fd = -1;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t drm_format = 0;
  uint64_t drm_format_modifier = 0;
  uint32_t stride[4] = {};
  uint32_t offset[4] = {};
  uint32_t num_planes = 1;
};

class DRMMaster {
 public:
  ~DRMMaster();
  /* Converts from ION fd --> Prime Handle --> FB_ID.
   * Input:
   *   drm_buffer: A DRMBuffer obj that packages description of buffer
   * Output:
   *   fb_id: Pointer to store DRM framebuffer id into
   * Returns:
   *   ioctl error code
   */
  int CreateFbId(const DRMBuffer &drm_buffer, uint32_t *fb_id);
  /* Removes the fb_id from DRM
   * Input:
   *   fb_id: DRM FB to be removed
   * Returns:
   *   ioctl error code
   */
  int RemoveFbId(uint32_t fb_id);
  /* Poplulates master DRM fd
   * Input:
   *   fd: Pointer to store master fd into
   */
  void GetHandle(int *fd) { *fd = dev_fd_; }
  void CreateEventHandle(int *fd);
  /* Returns true if the ref counted version of rmfb is being used */
  bool IsRmFbRefCounted();

  /* Creates an instance of DRMMaster if it doesn't exist and initializes it. Threadsafe.
   * Input:
   *   master: Pointer to store a pointer to the instance
   * Returns:
   *   -ENODEV if device cannot be opened or initilization fails
   */
  static int GetInstance(DRMMaster **master, uint32_t core_id = 0);
  static void DestroyInstance(uint32_t core_id = 0);

 private:
  DRMMaster() {}
  int Init(uint32_t core_id);

  int dev_fd_ = -1;              // Master fd for DRM
  uint32_t core_id_ = 0;            // Master core_id index
  char path_[64];                // Path of master core_id
  static sdm::MultiCoreInstance<uint32_t, DRMMaster*> s_instance;
  static std::mutex s_lock;
  std::mutex lock_;
};

}  // namespace drm_utils

#endif  // __DRM_MASTER_H__
