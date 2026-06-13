/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __SDM_COMP_DISPLAY_BUILTIN_H__
#define __SDM_COMP_DISPLAY_BUILTIN_H__

#include <string>
#include <vector>

#include "utils/constants.h"
#include "core/display_interface.h"
#include "core/core_interface.h"
#include "core/sdm_types.h"
#include "sdm_comp_interface.h"
#include "private/color_params.h"

namespace sdm {

class SDMCompDisplayBuiltIn : public DisplayEventHandler {
 public:
  explicit SDMCompDisplayBuiltIn(CoreInterface *core_intf, CallbackInterface *callback,
                                          SDMCompDisplayType disp_type, int32_t disp_id);
  virtual ~SDMCompDisplayBuiltIn() { }

  virtual DisplayError VSync(const DisplayEventVSync &vsync) { return kErrorNone; }
  virtual DisplayError Refresh() { return kErrorNone; }
  virtual DisplayError CECMessage(char *message) { return kErrorNone; }
  virtual DisplayError HistogramEvent(int source_fd, uint32_t blob_id) { return kErrorNone; }
  virtual DisplayError HandleEvent(DisplayEvent event);
  virtual void MMRMEvent(bool restricted) { }

  int Init();
  int Deinit();
  int GetDisplayAttributes(SDMCompDisplayAttributes *display_attributes);
  int ShowBuffer(BufferHandle *buf_handle, int32_t *out_release_fence);
  int SetColorModeWithRenderIntent(struct ColorMode mode);
  int GetColorModes(uint32_t *out_num_modes, struct ColorMode *out_modes);
  SDMCompDisplayType GetDisplayType() { return display_type_; }
  int SetPanelBrightness(float brightness_level);
  int SetMinPanelBrightness(float min_brightness);
  int GetNumVariableInfoConfigs(uint32_t *count);
  int GetDisplayConfig(int config_idx, DisplayConfigVariableInfo *variable_info);
  int SetDisplayConfig(int config_idx);
  void SetFrameBufferConfig();
  void SetMixerConfig(uint32_t width, uint32_t height);

 private:
  void CreateLayerSet();
  void DestroyLayerSet();
  int PrepareLayerStack(BufferHandle *buf_handle);

  void PopulateColorModes();
  int GetStcColorModeFromMap(const ColorMode &mode, snapdragoncolor::ColorMode *out_mode);
  int ApplyCurrentColorModeWithRenderIntent();

  CoreInterface *core_intf_ = NULL;
  DisplayInterface *display_intf_ = NULL;
  DisplayConfigVariableInfo variable_info_ = {};
  SDMCompMixerConfig mixer_config_ = {};
  CallbackInterface *callback_ = NULL;
  SDMCompDisplayType display_type_;
  int display_id_ = -1;
  LayerStack layer_stack_ = {};
  uint32_t active_config_ = 0xFFFFFFFF;

  bool apply_mode_ = false;
  ColorMode current_mode_ = {};
  snapdragoncolor::ColorModeList stc_mode_list_ = {};
  typedef std::map<RenderIntent, snapdragoncolor::ColorMode> RenderIntentMap;
  typedef std::map<GammaTransfer, RenderIntentMap> TransferMap;
  std::map<ColorPrimaries, TransferMap> color_mode_map_ = {};
  float min_panel_brightness_ = 0.0f;
  bool validated_ = false;
  bool first_commit_ = true;
  std::vector<Layer *> layer_set_ = {};
  BufferHandle cached_buf_handle_;
  Layer *demura_layer_ = nullptr;
};

}  // namespace sdm
#endif  // __SDM_COMP_DISPLAY_BUILTIN_H__
