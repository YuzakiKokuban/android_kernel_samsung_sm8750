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
#ifndef __SDM_DISPLAY_VIRTUAL_GPU_H__
#define __SDM_DISPLAY_VIRTUAL_GPU_H__

#include "sdm_display_virtual.h"
#include "utils/sync_task.h"

namespace sdm {

struct ColorConvertGetInstanceContext
    : public SyncTask<ColorConvertTaskCode>::TaskContext {
  LayerBuffer *output_buffer = NULL;
};

class SDMDisplayVirtualGPU
    : public SDMDisplayVirtual,
      public SyncTask<ColorConvertTaskCode>::TaskHandler {
public:
 SDMDisplayVirtualGPU(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                      SDMCompositorCallbacks *callbacks, Display id, int32_t sdm_id, uint32_t width,
                      uint32_t height, float min_lum, float max_lum);
 virtual DisplayError Init();
 virtual DisplayError Deinit();
 virtual DisplayError Validate(uint32_t *out_num_types, uint32_t *out_num_requests);
 virtual DisplayError Present(shared_ptr<Fence> *out_retire_fence);
 virtual DisplayError SetOutputBuffer(const SnapHandle *buf, shared_ptr<Fence> release_fence);
 virtual DisplayError CommitOrPrepare(bool validate_only, shared_ptr<Fence> *out_retire_fence,
                                      uint32_t *out_num_types, uint32_t *out_num_requests,
                                      bool *needs_commit);
 virtual bool FreezeScreen();
 virtual DisplayError SetColorTransform(const float *matrix, SDMColorTransform hint) {
   return kErrorNone;
 }

private:
  // SyncTask methods.
  void OnTask(const ColorConvertTaskCode &task_code,
              SyncTask<ColorConvertTaskCode>::TaskContext *task_context);

  SyncTask<ColorConvertTaskCode> color_convert_task_;
  // GLColorConvert *gl_color_convert_ = nullptr;

  bool disable_animation_ = false;
  bool animation_in_progress_ = false;
};

} // namespace sdm

#endif // __SDM_DISPLAY_VIRTUAL_GPU_H__
