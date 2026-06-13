/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
#include "sdm_display_virtual_factory.h"
#include "sdm_display_virtual_dpu.h"
#include "sdm_display_virtual_gpu.h"

#define __CLASS__ "SDMVirtualDisplayFactory"

namespace sdm {

int SDMVirtualDisplayFactory::Create(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                                     SDMCompositorCallbacks *callbacks, Display id, int32_t sdm_id,
                                     uint32_t width, uint32_t height, int32_t *format,
                                     float min_lum, float max_lum, SDMDisplay **sdm_display) {
  int supported_virtual_displays = 0;
  DisplayError error =
      core_intf->GetMaxDisplaysSupported(kVirtual, &supported_virtual_displays);
  if (error != kErrorNone) {
    DLOGE("Could not find maximum virtual displays supported. Error = %d",
          error);
    return -1;
  }

  SDMDisplayVirtual *sdm_display_virtual = nullptr;
  if (supported_virtual_displays) {
    sdm_display_virtual =
        new SDMDisplayVirtualDPU(core_intf, buffer_allocator, callbacks, id,
                                 sdm_id, width, height, min_lum, max_lum);
  } else {
    // Create GPU based virtual display.
    sdm_display_virtual =
        new SDMDisplayVirtualGPU(core_intf, buffer_allocator, callbacks, id,
                                 sdm_id, width, height, min_lum, max_lum);
  }

  if (sdm_display_virtual == nullptr) {
    DLOGE("Failed to create instance");
    return -1;
  } else {
    DLOGI("Created %s based virtual display",
          (supported_virtual_displays ? "DPU" : "GPU"));
  }

  int status = sdm_display_virtual->Init();
  if (status) {
    DLOGW("Failed to initialize virtual display");
    Destroy(sdm_display_virtual);
    return status;
  }

  // TODO(user): Populate format correctly
  DLOGI("Creating virtual display: w: %d h:%d format:0x%x", width, height,
        *format);

  *sdm_display = sdm_display_virtual;

  return status;
}

bool SDMVirtualDisplayFactory::IsGPUColorConvertSupported() {
  int value = 0;
  SDMDebugHandler::Get()->GetProperty(DISABLE_GPU_COLOR_CONVERT, &value);
  return (value == 0);
}

void SDMVirtualDisplayFactory::Destroy(SDMDisplay *sdm_display) {
  DLOGI("Destroying virtual display instance");
  if (sdm_display->Deinit() != 0) {
    DLOGE("Failed to destroy instance");
  }

  delete sdm_display;
}

} // namespace sdm
