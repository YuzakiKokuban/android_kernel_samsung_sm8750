/*
* Copyright (c) 2014 - 2020, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted
* provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright notice, this list of
*      conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright notice, this list of
*      conditions and the following disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 *
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DISPLAY_VIRTUAL_H__
#define __DISPLAY_VIRTUAL_H__

#include <private/hw_info_types.h>
#include <utils/multi_core_instantiator.h>
#include <vector>
#include "display_base.h"

namespace sdm {

class HWVirtualInterface;

class DisplayVirtual : public DisplayBase {
 public:
  DisplayVirtual(DisplayEventHandler *event_handler,
                 sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                 BufferAllocator *buffer_allocator, CompManager *comp_manager);
  DisplayVirtual(DisplayId display_id, DisplayEventHandler *event_handler,
                 sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                 BufferAllocator *buffer_allocator, CompManager *comp_manager);
  virtual DisplayError Init();
  virtual DisplayError Deinit();
  virtual DisplayError Prepare(LayerStack *layer_stack);
  virtual DisplayError GetNumVariableInfoConfigs(uint32_t *count);
  virtual DisplayError GetConfig(uint32_t index, DisplayConfigVariableInfo *variable_info);
  virtual DisplayError GetActiveConfig(uint32_t *index);
  virtual DisplayError SetActiveConfig(uint32_t index) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetActiveConfig(DisplayConfigVariableInfo *variable_info);
  virtual DisplayError SetMixerResolution(uint32_t width, uint32_t height) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetPanelLuminanceAttributes(float min_lum, float max_lum);
  virtual DisplayError SetVSyncState(bool enable) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetRefreshRate(uint32_t refresh_rate, bool final_rate, bool idle_screen) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetDetailEnhancerData(const DisplayDetailEnhancerData &de_data) {
    return kErrorNotSupported;
  }
  virtual DisplayError ValidateGPUTargetParams() {
    // TODO(user): Validate GPU target for virtual display when query display attributes
    // on virtual display is functional.
    return kErrorNone;
  }
  virtual DisplayError SetColorTransform(const uint32_t length, const double *color_transform) {
    return kErrorNone;
  }
  virtual DisplayError CaptureCwb(const LayerBuffer &output_buffer, const CwbConfig &config) {
    return kErrorNotSupported;
  }

  virtual DisplayError GetColorModeCount(uint32_t *mode_count);
  virtual DisplayError colorSamplingOn();
  virtual DisplayError colorSamplingOff();

 protected:
  float set_max_lum_ = -1.0;
  float set_min_lum_ = -1.0;
};

}  // namespace sdm

#endif  // __DISPLAY_VIRTUAL_H__

