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

/* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <utils/constants.h>
#include <utils/debug.h>
#include <private/hw_interface.h>
#include <private/hw_info_interface.h>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "display_pluggable.h"

#define __CLASS__ "DisplayPluggable"

namespace sdm {

DisplayPluggable::DisplayPluggable(DisplayEventHandler *event_handler,
                                   sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                                   BufferAllocator *buffer_allocator, CompManager *comp_manager)
    : DisplayBase(kPluggable, event_handler, kDevicePluggable, buffer_allocator, comp_manager,
                  hw_info_intf) {}

DisplayPluggable::DisplayPluggable(DisplayId display_id, DisplayEventHandler *event_handler,
                                   sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                                   BufferAllocator *buffer_allocator, CompManager *comp_manager)
    : DisplayBase(display_id, kPluggable, event_handler, kDevicePluggable, buffer_allocator,
                  comp_manager, hw_info_intf) {}

DisplayError DisplayPluggable::Init() {
  ClientLock lock(disp_mutex_);

  DisplayError error = DPUCoreFactory::Create(display_id_info_, kPluggable, hw_info_intf_,
                                              buffer_allocator_, &dpu_core_mux_);
  if (error != kErrorNone) {
    if (kErrorDeviceRemoved == error) {
      DLOGW("Aborted creating hardware interface. Device removed.");
    } else {
      DLOGE("Failed to create hardware interface. Error = %d", error);
    }
    return error;
  }

  dpu_core_mux_->GetHWInterface(&hw_intf_);
  if (!hw_intf_) {
    DLOGW("Invalid value for hw_intf_.");
    return kErrorParameters;
  }

  if (-1 == display_id_info_.GetDisplayId()) {
    dpu_core_mux_->GetDisplayId(&display_id_);
    display_id_info_ = DisplayId(display_id_);
    core_id_ = display_id_info_.GetCoreIdMap();
    std::bitset<32> core_id_bitset = std::bitset<32>(core_id_);
    core_count_ = core_id_bitset.count();
  }

  uint32_t active_mode_index = 0;
  error = dpu_core_mux_->GetActiveConfig(&active_mode_index);
  if (error != kErrorNone) {
    dpu_core_mux_->Destroy();
    return error;
  }

  uint32_t override_mode_index = active_mode_index;
  error = GetOverrideConfig(&override_mode_index);
  if (error == kErrorNone && override_mode_index != active_mode_index) {
    DLOGI("Overriding display mode %d with mode %d.", active_mode_index, override_mode_index);
    error = dpu_core_mux_->SetDisplayAttributes(override_mode_index);
    if (error != kErrorNone) {
      DLOGI("Failed overriding display mode %d with mode %d. Continuing with display mode %d.",
            active_mode_index, override_mode_index, active_mode_index);
    }
  }

  error = DisplayBase::Init();
  if (error == kErrorResources) {
    DLOGI("Reattempting display creation for Pluggable display %d-%d", display_id_, display_type_);
    uint32_t default_mode_index = 0;
    error = dpu_core_mux_->GetDefaultConfig(&default_mode_index);
    if (error == kErrorNone) {
      dpu_core_mux_->SetDisplayAttributes(default_mode_index);
      error = DisplayBase::Init();
    } else {
      DLOGE("640x480 default mode not found, failing creation!");
    }
  }
  if (error != kErrorNone) {
    dpu_core_mux_->Destroy();
    return error;
  }

  GetScanSupport();
  underscan_supported_ = (scan_support_ == kScanAlwaysUnderscanned) || (scan_support_ == kScanBoth);

  event_list_ = {HWEvent::VSYNC, HWEvent::EXIT, HWEvent::CEC_READ_MESSAGE,
                 HWEvent::HW_RECOVERY, HWEvent::POWER_EVENT};

  error = HWEventsInterface::Create(display_id_info_, kPluggable, this, event_list_,
                                    &hw_events_intf_);
  if (error != kErrorNone) {
    DisplayBase::Deinit();
    dpu_core_mux_->Destroy();
    DLOGE("Failed to create hardware events interface. Error = %d for display %d-%d", error,
          display_id_, display_type_);
  }

  InitializeColorModes();

  current_refresh_rate_ = client_ctx_.hw_panel_info.max_fps;

  return error;
}

DisplayError DisplayPluggable::Prepare(LayerStack *layer_stack) {
  DTRACE_SCOPED();
  ClientLock lock(disp_mutex_);
  DisplayError error = kErrorNone;
  uint32_t new_mixer_width = 0;
  uint32_t new_mixer_height = 0;
  uint32_t display_width = client_ctx_.display_attributes.x_pixels;
  uint32_t display_height = client_ctx_.display_attributes.y_pixels;

  error = PrePrepare(layer_stack);
  if (error == kErrorNone) {
    return error;
  }

  if (error == kErrorNeedsLutRegen && (ForceToneMapUpdate(layer_stack) == kErrorNone)) {
    return kErrorNone;
  }

  if (NeedsMixerReconfiguration(layer_stack, &new_mixer_width, &new_mixer_height)) {
    error = ReconfigureMixer(new_mixer_width, new_mixer_height);
    if (error != kErrorNone) {
      ReconfigureMixer(display_width, display_height);
    }
  }

  return DisplayBase::Prepare(layer_stack);
}

DisplayError DisplayPluggable::GetRefreshRateRange(uint32_t *min_refresh_rate,
                                                   uint32_t *max_refresh_rate) {
  ClientLock lock(disp_mutex_);
  DisplayError error = kErrorNone;

  if (client_ctx_.hw_panel_info.min_fps && client_ctx_.hw_panel_info.max_fps) {
    *min_refresh_rate = client_ctx_.hw_panel_info.min_fps;
    *max_refresh_rate = client_ctx_.hw_panel_info.max_fps;
  } else {
    error = DisplayBase::GetRefreshRateRange(min_refresh_rate, max_refresh_rate);
  }

  return error;
}

DisplayError DisplayPluggable::SetRefreshRate(uint32_t refresh_rate, bool final_rate,
                                              bool idle_screen) {
  ClientLock lock(disp_mutex_);

  if (!active_) {
    return kErrorPermission;
  }

  if (current_refresh_rate_ != refresh_rate) {
    DisplayError error = dpu_core_mux_->SetRefreshRate(refresh_rate);
    if (error != kErrorNone) {
      return error;
    }
  }

  current_refresh_rate_ = refresh_rate;
  return DisplayBase::ReconfigureDisplay();
}

bool DisplayPluggable::IsUnderscanSupported() {
  ClientLock lock(disp_mutex_);
  return underscan_supported_;
}

DisplayError DisplayPluggable::GetOverrideConfig(uint32_t *mode_index) {
  DisplayError error = kErrorNone;

  if (!mode_index) {
    DLOGE("Invalid mode index parameter.");
    return kErrorParameters;
  }

  char val[kPropertyMax] = {};
  // Used for changing HDMI Resolution - Override the preferred mode with user set config.
  bool user_config = Debug::GetExternalResolution(val);
  if (user_config) {
    uint32_t config_index = 0;
    // For the config, get the corresponding index
    error = dpu_core_mux_->GetConfigIndex(val, &config_index);
    if (error == kErrorNone) {
      *mode_index = config_index;
    }
  }

  return error;
}

void DisplayPluggable::GetScanSupport() {
  DisplayError error = kErrorNone;
  uint32_t video_format = 0;
  uint32_t max_cea_format = 0;
  HWScanInfo scan_info = HWScanInfo();
  dpu_core_mux_->GetHWScanInfo(&scan_info);

  uint32_t active_mode_index = 0;
  dpu_core_mux_->GetActiveConfig(&active_mode_index);

  error = dpu_core_mux_->GetVideoFormat(active_mode_index, &video_format);
  if (error != kErrorNone) {
    return;
  }

  error = dpu_core_mux_->GetMaxCEAFormat(&max_cea_format);
  if (error != kErrorNone) {
    return;
  }

  // The scan support for a given HDMI TV must be read from scan info corresponding to
  // Preferred Timing if the preferred timing of the display is currently active, and if it is
  // valid. In all other cases, we must read the scan support from CEA scan info if
  // the resolution is a CEA resolution, or from IT scan info for all other resolutions.
  if (active_mode_index == 0 && scan_info.pt_scan_support != kScanNotSupported) {
    scan_support_ = scan_info.pt_scan_support;
  } else if (video_format < max_cea_format) {
    scan_support_ = scan_info.cea_scan_support;
  } else {
    scan_support_ = scan_info.it_scan_support;
  }
}

void DisplayPluggable::CECMessage(char *message) {
  event_handler_->CECMessage(message);
}

// HWEventHandler overload, not DisplayBase
void DisplayPluggable::HwRecovery(const HWRecoveryEvent sdm_event_code) {
  DisplayBase::HwRecovery(sdm_event_code);
}

void DisplayPluggable::Histogram(int /* histogram_fd */, uint32_t /* blob_id */) {}

void DisplayPluggable::HandleBacklightEvent(float /* brightness_level */) {}

DisplayError DisplayPluggable::VSync(int64_t timestamp) {
  if (vsync_enable_) {
    DisplayEventVSync vsync;
    vsync.timestamp = timestamp;
    event_handler_->VSync(vsync);
  }

  return kErrorNone;
}

DisplayError DisplayPluggable::InitializeColorModes() {
  PrimariesTransfer pt = {};
  AttrVal var = {};
  bool hdr_supported = true;

  for (auto& res_info : hw_resource_info_) {
    hdr_supported &= res_info.has_hdr;
  }

  if ((!client_ctx_.hw_panel_info.hdr_enabled &&
       !client_ctx_.hw_panel_info.supported_colorspaces) ||
      !hdr_supported) {
    return kErrorNone;
  } else {
    if (client_ctx_.hw_panel_info.supported_colorspaces) {
      InitializeColorModesFromColorspace();
    }
    color_modes_cs_.push_back(pt);
    var.push_back(std::make_pair(kColorGamutAttribute, kSrgb));
    var.push_back(std::make_pair(kDynamicRangeAttribute, kSdr));
    var.push_back(std::make_pair(kPictureQualityAttribute, kStandard));
    var.push_back(std::make_pair(kRenderIntentAttribute, "0"));
    color_mode_attr_map_.insert(std::make_pair(kSrgb, var));

    // native mode
    pt.primaries = QtiColorPrimaries_Max;
    pt.transfer = QtiTransfer_Max;
    var.clear();
    var.push_back(std::make_pair(kColorGamutAttribute, kNative));
    var.push_back(std::make_pair(kGammaTransferAttribute, kNative));
    var.push_back(std::make_pair(kRenderIntentAttribute, "0"));
    color_modes_cs_.push_back(pt);
    color_mode_attr_map_.insert(std::make_pair("hal_native", var));
  }

  var.clear();
  var.push_back(std::make_pair(kColorGamutAttribute, kBt2020));
  var.push_back(std::make_pair(kPictureQualityAttribute, kStandard));
  var.push_back(std::make_pair(kRenderIntentAttribute, "0"));
  if (client_ctx_.hw_panel_info.hdr_eotf & kHdrEOTFHDR10) {
    pt.transfer = QtiTransfer_SMPTE_ST2084;
    var.push_back(std::make_pair(kGammaTransferAttribute, kSt2084));
    color_modes_cs_.push_back(pt);
    color_mode_attr_map_.insert(std::make_pair(kBt2020Pq, var));
  }
  if (client_ctx_.hw_panel_info.hdr_eotf & kHdrEOTFHLG) {
    pt.transfer = QtiTransfer_HLG;
    var.pop_back();
    var.push_back(std::make_pair(kGammaTransferAttribute, kHlg));
    color_modes_cs_.push_back(pt);
    color_mode_attr_map_.insert(std::make_pair(kBt2020Hlg, var));
  }
  current_color_mode_ = kSrgb;
  UpdateColorModes();

  return kErrorNone;
}

void DisplayPluggable::InitializeColorModesFromColorspace() {
  PrimariesTransfer pt = {};
  AttrVal var = {};
  if (client_ctx_.hw_panel_info.supported_colorspaces & kColorspaceDcip3) {
    pt.primaries = QtiColorPrimaries_DCIP3;
    pt.transfer = QtiTransfer_sRGB;
    var.clear();
    var.push_back(std::make_pair(kColorGamutAttribute, kDcip3));
    var.push_back(std::make_pair(kGammaTransferAttribute, kSrgb));
    var.push_back(std::make_pair(kPictureQualityAttribute, kStandard));
    var.push_back(std::make_pair(kRenderIntentAttribute, "0"));
    color_modes_cs_.push_back(pt);
    color_mode_attr_map_.insert(std::make_pair(kDisplayP3, var));
  }
  if (client_ctx_.hw_panel_info.supported_colorspaces & kColorspaceBt2020rgb) {
    pt.primaries = QtiColorPrimaries_BT2020;
    pt.transfer = QtiTransfer_sRGB;
    var.clear();
    var.push_back(std::make_pair(kColorGamutAttribute, kBt2020));
    var.push_back(std::make_pair(kGammaTransferAttribute, kSrgb));
    var.push_back(std::make_pair(kPictureQualityAttribute, kStandard));
    var.push_back(std::make_pair(kRenderIntentAttribute, "0"));
    color_modes_cs_.push_back(pt);
    color_mode_attr_map_.insert(std::make_pair(kDisplayBt2020, var));
  }
}

static PrimariesTransfer GetBlendSpaceFromAttributes(const std::string &color_gamut,
                                                     const std::string &transfer) {
  PrimariesTransfer blend_space_ = {};
  if (color_gamut == kNative) {  // Native mode is identified by Max
    blend_space_.primaries = QtiColorPrimaries_Max;
    blend_space_.transfer = QtiTransfer_Max;
  } else if (color_gamut == kBt2020) {
    blend_space_.primaries = QtiColorPrimaries_BT2020;
    if (transfer == kHlg) {
      blend_space_.transfer = QtiTransfer_HLG;
    } else if (transfer == kSt2084) {
      blend_space_.transfer = QtiTransfer_SMPTE_ST2084;
    } else if (transfer == kGamma2_2) {
      blend_space_.transfer = QtiTransfer_Gamma2_2;
    }
  } else if (color_gamut == kDcip3) {
    blend_space_.primaries = QtiColorPrimaries_DCIP3;
    blend_space_.transfer = QtiTransfer_sRGB;
  } else if (color_gamut == kSrgb) {
    blend_space_.primaries = QtiColorPrimaries_BT709_5;
    blend_space_.transfer = QtiTransfer_sRGB;
  } else {
    DLOGW("Failed to Get blend space color_gamut = %s transfer = %s",
          color_gamut.c_str(), transfer.c_str());
  }
  DLOGI("Blend Space Primaries = %d Transfer = %d", blend_space_.primaries, blend_space_.transfer);

  return blend_space_;
}

DisplayError DisplayPluggable::SetColorMode(const std::string &color_mode) {
  auto current_color_attr_ = color_mode_attr_map_.find(color_mode);
  if (current_color_attr_ == color_mode_attr_map_.end()) {
    DLOGE("Failed to get the color mode for display %d-%d = %s", display_id_,
          display_type_, color_mode.c_str());
    return kErrorNone;
  }
  AttrVal attr = current_color_attr_->second;
  std::string color_gamut = kNative, transfer = {};

  if (attr.begin() != attr.end()) {
    for (auto &it : attr) {
      if (it.first.find(kColorGamutAttribute) != std::string::npos) {
        color_gamut = it.second;
      } else if (it.first.find(kGammaTransferAttribute) != std::string::npos) {
        transfer = it.second;
      }
    }
  }

  DisplayError error = kErrorNone;
  PrimariesTransfer blend_space = GetBlendSpaceFromAttributes(color_gamut, transfer);
  error = comp_manager_->SetBlendSpace(display_comp_ctx_, blend_space);
  if (error != kErrorNone) {
    DLOGE("Failed Set blend space, error = %d for display %d-%d", error,
          display_id_, display_type_);
  }

  error = dpu_core_mux_->SetBlendSpace(blend_space);
  if (error != kErrorNone) {
    DLOGE("Failed to pass blend space, error = %d for display %d-%d", error,
    display_id_, display_type_);
  }

  current_color_mode_ = color_mode;

  return kErrorNone;
}

DisplayError DisplayPluggable::GetColorModeCount(uint32_t *mode_count) {
  ClientLock lock(disp_mutex_);
  if (!mode_count) {
    return kErrorParameters;
  }

  DLOGI("Display = %d Number of modes = %d", display_type_, num_color_modes_);
  *mode_count = num_color_modes_;

  return kErrorNone;
}

DisplayError DisplayPluggable::GetColorModes(uint32_t *mode_count,
                                             std::vector<std::string> *color_modes) {
  ClientLock lock(disp_mutex_);
  if (!mode_count || !color_modes) {
    return kErrorParameters;
  }

  for (uint32_t i = 0; i < num_color_modes_; i++) {
    DLOGI_IF(kTagDisplay, "ColorMode[%d] = %s", i, color_modes_[i].name);
    color_modes->at(i) = color_modes_[i].name;
  }

  return kErrorNone;
}

DisplayError DisplayPluggable::GetColorModeAttr(const std::string &color_mode, AttrVal *attr) {
  ClientLock lock(disp_mutex_);
  if (!attr) {
    return kErrorParameters;
  }

  auto it = color_mode_attr_map_.find(color_mode);
  if (it == color_mode_attr_map_.end()) {
    DLOGI("Mode %s has no attribute for display %d-%d", color_mode.c_str(), display_id_,
          display_type_);
    return kErrorNotSupported;
  }
  *attr = it->second;

  return kErrorNone;
}

void DisplayPluggable::UpdateColorModes() {
  uint32_t i = 0;
  num_color_modes_ = UINT32(color_mode_attr_map_.size());
  color_modes_.resize(num_color_modes_);
  for (ColorModeAttrMap::iterator it = color_mode_attr_map_.begin();
       ((i < num_color_modes_) && (it != color_mode_attr_map_.end())); i++, it++) {
    color_modes_[i].id = INT32(i);
    std::size_t length = (it->first).copy(color_modes_[i].name, sizeof(SDEDisplayMode::name) - 1);
    color_modes_[i].name[length] = '\0';
    color_mode_map_.insert(std::make_pair(color_modes_[i].name, &color_modes_[i]));
    DLOGI("Color mode = %s", color_modes_[i].name);
  }
  return;
}

DisplayError DisplayPluggable::colorSamplingOn() {
  return kErrorNone;
}

DisplayError DisplayPluggable::colorSamplingOff() {
  return kErrorNone;
}

void DisplayPluggable::MMRMEvent(uint32_t clk) {
  // Stub for future support
  return;
}

void DisplayPluggable::HandlePowerEvent() {
  return ProcessPowerEvent();
}

void DisplayPluggable::HandleVmReleaseEvent() {
}

void DisplayPluggable::GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) {
  dpu_core_mux_->GetDRMDisplayToken(token);
}

bool DisplayPluggable::IsPrimaryDisplay() {
  return DisplayBase::IsPrimaryDisplay();
}

DisplayError DisplayPluggable::GetPanelBrightnessBasePath(std::string *base_path) {
  return dpu_core_mux_->GetPanelBrightnessBasePath(base_path);
}

}  // namespace sdm
