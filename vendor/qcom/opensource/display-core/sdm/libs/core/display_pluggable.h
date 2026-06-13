/*
* Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DISPLAY_PLUGGABLE_H__
#define __DISPLAY_PLUGGABLE_H__

#include <private/hw_events_interface.h>
#include <utils/multi_core_instantiator.h>

#include <map>
#include <string>
#include <vector>

#include "display_base.h"

namespace sdm {

class DisplayPluggable : public DisplayBase, HWEventHandler {
 public:
  DisplayPluggable(DisplayEventHandler *event_handler,
                   sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                   BufferAllocator *buffer_allocator, CompManager *comp_manager);
  DisplayPluggable(DisplayId display_id, DisplayEventHandler *event_handler,
                   sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                   BufferAllocator *buffer_allocator, CompManager *comp_manager);
  DisplayError Init() override;
  DisplayError Prepare(LayerStack *layer_stack) override;
  DisplayError GetRefreshRateRange(uint32_t *min_refresh_rate,
                                   uint32_t *max_refresh_rate) override;
  DisplayError SetRefreshRate(uint32_t refresh_rate, bool final_rate, bool idle_screen) override;
  bool IsUnderscanSupported() override;
  DisplayError InitializeColorModes() override;
  DisplayError SetColorMode(const std::string &color_mode) override;
  DisplayError GetColorModeCount(uint32_t *mode_count) override;
  DisplayError GetColorModes(uint32_t *mode_count,
                             std::vector<std::string> *color_modes) override;
  DisplayError GetColorModeAttr(const std::string &color_mode, AttrVal *attr) override;
  DisplayError SetColorTransform(const uint32_t length,
                                 const double *color_transform) override {
    return kErrorNone;
  }
  DisplayError colorSamplingOn() override;
  DisplayError colorSamplingOff() override;

  // Implement the HWEventHandlers
  DisplayError VSync(int64_t timestamp) override;
  DisplayError Blank(bool blank) override { return kErrorNone; }
  void CECMessage(char *message) override;
  void IdlePowerCollapse() override {}
  void PingPongTimeout() override {}
  void PanelDead() override {}
  void HwRecovery(const HWRecoveryEvent sdm_event_code) override;
  void HandleBacklightEvent(float brightness_level) override;
  void Histogram(int histogram_fd, uint32_t blob_id) override;
  void MMRMEvent(uint32_t clk) override;
  void HandlePowerEvent() override;
  void HandleVmReleaseEvent() override;
  void GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) override;
  bool IsPrimaryDisplay() override;
  DisplayError GetPanelBrightnessBasePath(std::string *base_path) override;
#ifdef SEC_GC_QC_DYN_CLK
  DisplayError SetDynamicDSIClockCustom(uint64_t bit_clk_rate) override { return kErrorNone; };
#endif
  void UpdateColorModes();
  void InitializeColorModesFromColorspace();

 private:
  DisplayError GetOverrideConfig(uint32_t *mode_index);
  void GetScanSupport();

  static const int kPropertyMax = 256;

  bool underscan_supported_ = false;
  HWScanSupport scan_support_;
  std::vector<HWEvent> event_list_;
  uint32_t current_refresh_rate_ = 0;
};

}  // namespace sdm

#endif  // __DISPLAY_PLUGGABLE_H__
