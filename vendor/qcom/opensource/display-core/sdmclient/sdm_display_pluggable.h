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
#ifndef __SDM_DISPLAY_PLUGGABLE_H__
#define __SDM_DISPLAY_PLUGGABLE_H__

#include "display_event_handler.h"
#include "sdm_display.h"

namespace sdm {

class SDMDisplayPluggable : public SDMDisplay {
public:
 static DisplayError Create(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                            SDMCompositorCallbacks *callbacks,
                            SDMDisplayEventHandler *event_handler, Display id, int32_t sdm_id,
                            uint32_t primary_width, uint32_t primary_height, bool use_primary_res,
                            SDMDisplay **sdm_display);
 static void Destroy(SDMDisplay *sdm_display);
 virtual DisplayError Init();
 virtual DisplayError Validate(uint32_t *out_num_types, uint32_t *out_num_requests);
 virtual DisplayError Present(shared_ptr<Fence> *out_retire_fence);
 virtual DisplayError Flush();
 virtual DisplayError GetColorModes(uint32_t *out_num_modes, SDMColorMode *out_modes);
 virtual DisplayError GetRenderIntents(SDMColorMode mode, uint32_t *out_num_intents,
                                       SDMRenderIntent *out_intents);
 virtual DisplayError SetColorMode(SDMColorMode mode);
 virtual DisplayError SetColorModeWithRenderIntent(SDMColorMode mode, SDMRenderIntent intent);
 virtual DisplayError SetColorTransform(const float *matrix, SDMColorTransform hint);
 virtual DisplayError PreValidateDisplay(bool *exit_validate);
 virtual DisplayError PostCommitLayerStack(shared_ptr<Fence> *out_retire_fence);

private:
 SDMDisplayPluggable(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                     SDMCompositorCallbacks *callbacks, SDMDisplayEventHandler *event_handler,
                     Display id, int32_t sdm_id);
 void ApplyScanAdjustment(SDMRect *display_frame);
 void GetUnderScanConfig();
 static void GetDownscaleResolution(uint32_t primary_width, uint32_t primary_height,
                                    uint32_t *virtual_width, uint32_t *virtual_height);

 int underscan_width_ = 0;
 int underscan_height_ = 0;
 bool has_color_tranform_ = false;
};

} // namespace sdm

#endif // __SDM_DISPLAY_PLUGGABLE_H__
