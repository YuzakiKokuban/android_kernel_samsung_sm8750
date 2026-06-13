/*
* Copyright (c) 2014 - 2021, The Linux Foundation. All rights reserved.
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
* Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "display_builtin.h"

#include <core/buffer_allocator.h>
#include <display_properties.h>
#include <private/aiqe_ssrc_feature_factory.h>
#include <private/hw_info_interface.h>
#include <private/hw_interface.h>
#include <sys/mman.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/formats.h>
#include <utils/rect.h>
#include <utils/utils.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "drm_interface.h"
#include "drm_master.h"

#define __CLASS__ "DisplayBuiltIn"

#ifdef SEC_GC_QC_VSYNC
#define kMaxVSyncTimestampLength 32
#endif

namespace sdm {

#ifdef SEC_GC_QC_VSYNC
  int32_t vsync_count_ = 0;
  int64_t vsync_timestamp_[kMaxVSyncTimestampLength] = { 0 };
  int64_t vsync_disabled_timestamp_ = 0;
  int64_t vsync_current_timestamp_ = 0;
#endif

DisplayBuiltIn::DisplayBuiltIn(DisplayEventHandler *event_handler,
                               sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                               BufferAllocator *buffer_allocator, CompManager *comp_manager,
                               std::shared_ptr<IPCIntf> ipc_intf)
    : DisplayBase(kBuiltIn, event_handler, kDeviceBuiltIn, buffer_allocator, comp_manager,
                  hw_info_intf),
      ipc_intf_(ipc_intf) {}

DisplayBuiltIn::DisplayBuiltIn(DisplayId display_id, DisplayEventHandler *event_handler,
                               sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                               BufferAllocator *buffer_allocator, CompManager *comp_manager,
                               std::shared_ptr<IPCIntf> ipc_intf)
    : DisplayBase(display_id, kBuiltIn, event_handler, kDeviceBuiltIn, buffer_allocator,
                  comp_manager, hw_info_intf),
      ipc_intf_(ipc_intf) {}

DisplayBuiltIn::~DisplayBuiltIn() {
}

static uint64_t GetTimeInMs(struct timespec ts) {
  return (ts.tv_sec * 1000 + (ts.tv_nsec + 500000) / 1000000);
}

DisplayError DisplayBuiltIn::SetupAiqe() {
  int value = 0;
  char value_str[200] = {0};
  aiqe::GetSsrcFeatureFactoryFp get_ssrc_feature_factory = nullptr;

  if (!prop_intf_) {
    DLOGI("Skipping AIQE setup. No panel feature property interface");
    return kErrorNone;
  }
  
  DebugHandler::Get()->GetProperty(AIQE_COPR_ENABLE, &value);
  if (value == 1) {
    EnableCopr(true);
  } else {
    DLOGI("AIQE COPR Enable property not set. Skipping COPR enablement");
  }
  value = 0;
  
  DebugHandler::Get()->GetProperty(AIQE_SSRC_ENABLE, &value);
  if (value == 1) {
    aiqe::SsrcFeatureFactory *ssrc_feature_factory;
    aiqe::SsrcFeatureDisplayDetails *display_details;
    std::string *default_mode;
    bool *force_commit;
    GenericPayload payload;

    if (!ssrc_lib_.Open(SSRC_LIBRARY_NAME)) {
      DLOGE("Unable to open library %s", SSRC_LIBRARY_NAME);
      return kErrorNotSupported;
    } else if (!ssrc_lib_.Sym(GET_SSRC_FF_INTF_NAME,
                              reinterpret_cast<void **>(&get_ssrc_feature_factory))) {
      DLOGE("Unable to get function pointer to retrieve AIQE feature factory");
      return kErrorNotSupported;
    }

    ssrc_feature_factory = get_ssrc_feature_factory();
    if (!ssrc_feature_factory) {
      DLOGE("Unable to retrieve SSRC feature factory");
      return kErrorNotSupported;
    }

    ssrc_feature_interface_ = ssrc_feature_factory->GetSsrcFeatureInterface(prop_intf_);
    if (!ssrc_feature_interface_) {
      DLOGE("Unable to retrieve SSRC feature interface");
      return kErrorNotSupported;
    }

    if (ssrc_feature_interface_->Init() != 0) {
      DLOGE("Unable to initalize SSRC feature interface");
      return kErrorNotSupported;
    }

    if (payload.CreatePayload(display_details) != 0) {
      DLOGE("Unable to create display details payload");
      return kErrorMemory;
    }

    display_details->panel_name = client_ctx_.hw_panel_info.panel_name;
    display_details->primary_panel = client_ctx_.hw_panel_info.is_primary_panel;
    switch (client_ctx_.mixer_attributes.split_type) {
      case kQuadSplit:
        display_details->ppc = 4;
        break;
      case kDualSplit:
        display_details->ppc = 2;
        break;
      case kNoSplit:
        display_details->ppc = 1;
        break;
      default:
        DLOGW("Unsupported mixer split - %d. Skipping AIQE Feature enablement",
              client_ctx_.mixer_attributes.split_type);
        return kErrorNotSupported;
    }

    if (ssrc_feature_interface_->SetParameter(aiqe::kSsrcFeatureDisplayDetails, payload) != 0) {
      DLOGE("Unable to set display details on SSRC feature interface");
      return kErrorNotSupported;
    }

    payload.DeletePayload();
    if (payload.CreatePayload(default_mode) != 0) {
      DLOGE("Unable to create mode payload");
      return kErrorMemory;
    }

    DebugHandler::Get()->GetProperty(AIQE_SSRC_DEFAULT_MODE, value_str);
    if (strcmp(value_str, "") == 0) {
      DLOGI("Using default SSRC mode");
      *default_mode = "NORMAL/ON";
    } else {
      DLOGI("Using SSRC mode from vendor property - %s", value_str);
      *default_mode = value_str;
    }

    if (ssrc_feature_interface_->SetParameter(aiqe::kSsrcFeatureModeId, payload) != 0) {
      DLOGE("Unable to set mode on AIQE SSRC feature interface");
      return kErrorNotSupported;
    }

    payload.DeletePayload();
    if (payload.CreatePayload(force_commit) != 0) {
      DLOGE("Unable to create commit payload");
      return kErrorMemory;
    }

    *force_commit = true;
    if (ssrc_feature_interface_->SetParameter(aiqe::kSsrcFeatureCommitFeature, payload) != 0) {
      DLOGE("Unable to set commit on AIQE SSRC feature interface");
      return kErrorNotSupported;
    }

    ssrc_feature_enabled_ = true;
  } else {
    DLOGI("AIQE SSRC Enable property not set. Skipping SSRC enablement");
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::Init() {
  ClientLock lock(disp_mutex_);

  DisplayError error = DPUCoreFactory::Create(display_id_info_, kBuiltIn, hw_info_intf_,
                                              buffer_allocator_, &dpu_core_mux_);
  if (error != kErrorNone) {
    DLOGE("Failed to create hardware interface on. Error = %d", error);
    return error;
  }

  dpu_core_mux_->GetHWInterface(&hw_intf_);

  if (-1 == display_id_info_.GetDisplayId()) {
    dpu_core_mux_->GetDisplayId(&display_id_);
    display_id_info_ = DisplayId(display_id_);
    core_id_ = display_id_info_.GetCoreIdMap();
    std::bitset<32> core_id_bitset = std::bitset<32>(core_id_);
    core_count_ = core_id_bitset.count();
  }

  error = DisplayBase::Init();
  if (error != kErrorNone) {
    dpu_core_mux_->Destroy();
    return error;
  }

  if (color_mgr_) {
    color_mgr_->ColorMgrGetStcModes(&stc_color_modes_);
  }

  if (client_ctx_.hw_panel_info.mode == kModeCommand && Debug::IsVideoModeEnabled()) {
    error = dpu_core_mux_->SetDisplayMode(kModeVideo);
    if (error != kErrorNone) {
      DLOGW("Retaining current display mode. Current = %d, Requested = %d",
            client_ctx_.hw_panel_info.mode, kModeVideo);
    }
  }

#ifdef TRUSTED_VM
  event_list_ = {HWEvent::VSYNC, HWEvent::EXIT, HWEvent::PINGPONG_TIMEOUT, HWEvent::PANEL_DEAD,
                 HWEvent::HW_RECOVERY};
#else
  event_list_ = {HWEvent::VSYNC,            HWEvent::EXIT,
                 HWEvent::SHOW_BLANK_EVENT, HWEvent::THERMAL_LEVEL,
                 HWEvent::PINGPONG_TIMEOUT, HWEvent::PANEL_DEAD,
                 HWEvent::HW_RECOVERY,      HWEvent::HISTOGRAM,
                 HWEvent::BACKLIGHT_EVENT,  HWEvent::POWER_EVENT,
                 HWEvent::MMRM,             HWEvent::VM_RELEASE_EVENT,
#ifdef SEC_GC_QC_DYN_CLK
                 HWEvent::DYN_CLK_EVENT
#endif
                 };
  if (client_ctx_.hw_panel_info.mode == kModeCommand) {
    event_list_.push_back(HWEvent::IDLE_POWER_COLLAPSE);
  }
#endif
  event_list_.push_back(HWEvent::POWER_EVENT);
  avr_prop_disabled_ = Debug::IsAVRDisabled();

  error = HWEventsInterface::Create(display_id_info_, kBuiltIn, this, event_list_,
                                    &hw_events_intf_);
  if (error != kErrorNone) {
    DisplayBase::Deinit();
    dpu_core_mux_->Destroy();
    DLOGE("Failed to create hardware events interface on. Error = %d", error);
  }

  // For CAC loopback case where CAC pipes are after DS blocks, These pipes take input w.r.t.
  // full panel resolution. In case of DS / Anamorphic compression usecase with cac loopback,
  // src_crop condition will not qualify for CAC pipes as buffer is smallar then panel size.
  // During CreateFbId for CAC pipes will fake it with dummy full screen buffer.
  error = AllocateDummyLoopbackCACBuffer();
  if (error != kErrorNone) {
    DLOGE("Failed to Get dummu loopback CAC buffer info");
    return error;
  }

  current_refresh_rate_ = client_ctx_.hw_panel_info.max_fps;

  int value = 0;
  Debug::Get()->GetProperty(ENABLE_HISTOGRAM_INTR, &value);
  if (value == 1) {
    initColorSamplingState();
  }

  value = 0;
  Debug::Get()->GetProperty(DEFER_FPS_FRAME_COUNT, &value);
  deferred_config_.frame_count = (value > 0) ? UINT32(value) : 0;

  error = event_proxy_info_.Init(client_ctx_.hw_panel_info.panel_name, this, extension_lib_,
                                 prop_intf_);
  if (error != kErrorNone) {
    DLOGW("Failed to initialize event proxy info");
    event_proxy_info_.Deinit();
  }

  if (pf_factory_ && prop_intf_) {
    // Get status of RC enablement property. Default RC is disabled.
    int rc_prop_value = 0;
    Debug::GetProperty(ENABLE_ROUNDED_CORNER, &rc_prop_value);
    if (rc_prop_value && EnableRC()) {
      rc_enable_prop_ = true;
    }
    DLOGI("RC feature %s on %s for display %d-%d",
          rc_enable_prop_ ? "enabled" : "disabled",
          client_ctx_.hw_panel_info.is_primary_panel ? "primary" : "secondary",
          display_id_, display_type_);

    if ((error = SetupSPR()) != kErrorNone) {
      DLOGE("SPR Failed to initialize. Error = %d", error);
      DisplayBase::Deinit();
      dpu_core_mux_->Destroy();
      HWEventsInterface::Destroy(hw_events_intf_);
      return error;
    }

    if ((error = HandleSPR()) != kErrorNone) {
      DLOGE("Failed to get SPR status. Error = %d", error);
      DisplayBase::Deinit();
      HWInterface::Destroy(hw_intf_);
      HWEventsInterface::Destroy(hw_events_intf_);
      return error;
    }

    int enable_abc = 0;
    Debug::Get()->GetProperty(ENABLE_ABC, &enable_abc);
    abc_prop_ = enable_abc;

#ifndef TRUSTED_VM
    std::thread([=] { DisplayBuiltIn::StartTvmServices(); }).detach();
#endif

    if (abc_prop_) {
      SetupABC();
    } else {
      SetupDemuraT0AndTn();
    }
  } else {
    DLOGW("Skipping Panel Feature Setups!");
  }
  value = 0;
  DebugHandler::Get()->GetProperty(DISABLE_DYNAMIC_FPS, &value);
  disable_dyn_fps_ = (value == 1);

  value = 0;
  DebugHandler::Get()->GetProperty(ENABLE_QSYNC_IDLE, &value);
  enable_qsync_idle_ = client_ctx_.hw_panel_info.qsync_support && (value == 1);
  if (enable_qsync_idle_) {
    DLOGI("Enabling qsync on idling");

    if (client_ctx_.hw_panel_info.transfer_time_us_min) {
      DLOGI("Setting transfer time to min: %d", client_ctx_.hw_panel_info.transfer_time_us_min);
      UpdateTransferTime(client_ctx_.hw_panel_info.transfer_time_us_min);
    }
  }

  value = 0;
  DebugHandler::Get()->GetProperty(ENHANCE_IDLE_TIME, &value);
  enhance_idle_time_ = (value == 1);

  value = 0;
  DebugHandler::Get()->GetProperty(ENABLE_DPPS_DYNAMIC_FPS, &value);
  enable_dpps_dyn_fps_ = (value == 1);

  value = 0;
  Debug::Get()->GetProperty(DISABLE_NOISE_LAYER, &value);
  noise_disable_prop_ = (value == 1);
  DLOGI("Noise Layer Feature is %s for display = %d-%d", noise_disable_prop_ ? "Disabled" :
        "Enabled", display_id_, display_type_);

  value = 0;
  DebugHandler::Get()->GetProperty(DISABLE_CWB_IDLE_FALLBACK, &value);
  disable_cwb_idle_fallback_ = (value == 1);

  value = 0;
  DebugHandler::Get()->GetProperty(ENABLE_BRIGHTNESS_DRM_PROP, &value);
  enable_brightness_drm_prop_ = (value == 1);

#ifdef SEC_GC_QC_SUPERHDR
  value = 0;
  Debug::GetProperty("vendor.display.enable_brightness_drm_prop_in_super_hdr", &value);
  enable_brightness_drm_prop_ |= (value == 1);

  DLOGI("vendor.display.enable_brightness_drm_prop_in_super_hdr:%d, enable_brightness_drm_prop_:%d", 
         value, enable_brightness_drm_prop_);
#endif

#ifdef TRUSTED_VM
  disable_cwb_idle_fallback_ = 1;
#endif

  if (!disable_cwb_idle_fallback_) {
    value = 0;
    Debug::Get()->GetProperty(IDLE_FALLBACK_ON_DSPP, &value);
    idle_fallback_on_dspp_ = (value == 1);
  }

  value = 0;
  DebugHandler::Get()->GetProperty(FORCE_LM_TO_FB_CONFIG, &value);
  force_lm_to_fb_config_ = (value == 1);

  NoiseInit();
  InitCWBBuffer();
  SetupAiqe();

  left_frame_roi_.resize(core_count_);
  right_frame_roi_.resize(core_count_);

  return error;
}

DisplayError DisplayBuiltIn::Deinit() {
  {
    ClientLock lock(disp_mutex_);

    if (demura_) {
      SetDemuraIntfStatus(false);

      if (demura_->Deinit() != 0) {
        DLOGE("Unable to DeInit Demura on Display %d-%d", display_id_, display_type_);
      }
    }

    if (demuratn_) {
      EnableDemuraTn(false);
      if (demuratn_->Deinit() != 0) {
        DLOGE("Unable to DeInit DemuraTn on Display %d", display_id_);
      }
    }
    if (demuratn_cleanup_intf_) {
      if (demuratn_cleanup_intf_->Deinit() != 0) {
        DLOGE("Unable to DeInit demuratn_cleanup_intf_ on Display %d", display_id_);
      }
    }
    demura_dynamic_enabled_ = true;

    DeinitCWBBuffer();
    hw_rc_blocks_in_use_ -= rc_blocks_reserved_;

    if (service_manager_intf_) {
      service_manager_intf_->Deinit();
      service_manager_intf_.reset();
      service_manager_intf_ = nullptr;
    }
  }

  dpps_info_.Deinit();
  event_proxy_info_.Deinit();
  return DisplayBase::Deinit();
}

DisplayError DisplayBuiltIn::PrePrepare(LayerStack *layer_stack) {
  DTRACE_SCOPED();
  uint32_t new_mixer_width = 0;
  uint32_t new_mixer_height = 0;
  uint32_t display_width = client_ctx_.display_attributes.x_pixels;
  uint32_t display_height = client_ctx_.display_attributes.y_pixels;
  GenericPayload bool_payload;
  bool *force_update;

  DisplayError error = HandleDemuraLayer(layer_stack);
  if (error != kErrorNone) {
    return error;
  }

  error = HandleSPR();
  if (error != kErrorNone) {
    return error;
  }
  disp_layer_stack_->stack_info.spr_enable = spr_enable_;

  AppendCWBLayer(layer_stack);
  // Do not skip validate if needs update PP features.
  if (color_mgr_) {
    needs_validate_ |= color_mgr_->IsValidateNeeded();
  }

  error = DisplayBase::PrePrepare(layer_stack);
  if (error == kErrorNone || error == kErrorNeedsLutRegen) {
    return error;
  }

  if (NeedsMixerReconfiguration(layer_stack, &new_mixer_width, &new_mixer_height)) {
    error = ReconfigureMixer(new_mixer_width, new_mixer_height);
    if (error != kErrorNone) {
      ReconfigureMixer(display_width, display_height);
    }
  } else {
    if (CanSkipDisplayPrepare(layer_stack)) {
      UpdateQsyncConfig();
      return kErrorNone;
    }
  }
  error = ChangeFps();
  lower_fps_ = disp_layer_stack_->stack_info.lower_fps;

  if (color_mgr_ && client_ctx_.hw_panel_info.mode == kModeVideo && idle_fallback_on_dspp_) {
    CwbTapPoint tap_point = CwbTapPoint::kDsppTapPoint;
    bool destination_scaler =
        (client_ctx_.display_attributes.x_pixels != client_ctx_.mixer_attributes.width ||
         client_ctx_.display_attributes.y_pixels != client_ctx_.mixer_attributes.height);
    tap_point = destination_scaler ? CwbTapPoint::kLmTapPoint : CwbTapPoint::kDsppTapPoint;
    if (tap_point == CwbTapPoint::kDsppTapPoint) {
      color_mgr_->ColorMgrIdleFallback(lower_fps_);
      needs_validate_ |= color_mgr_->IsValidateNeeded();
    }
  }

  if (ssrc_feature_enabled_) {
    if (bool_payload.CreatePayload(force_update) != 0) {
      DLOGE("Unable to create force update payload");
      return kErrorMemory;
    }

    *force_update = false;
    if (ssrc_feature_interface_->SetParameter(aiqe::kSsrcFeatureCommitFeature, bool_payload) != 0) {
      DLOGE("Unable to set commit on SSRC feature interface");
      return kErrorNotSupported;
    }
  }

  return kErrorNotValidated;
}

DisplayError DisplayBuiltIn::HandleSPR() {
  if (spr_) {
    GenericPayload out;
    uint32_t *enable = nullptr;
    int ret = out.CreatePayload<uint32_t>(enable);
    if (ret) {
      DLOGE("Failed to create the payload. Error:%d", ret);
      validated_ = false;
      return kErrorUndefined;
    }
    ret = spr_->GetParameter(kSPRFeatureEnable, &out);
    if (ret) {
      DLOGE("Failed to get the spr status. Error:%d", ret);
      validated_ = false;
      return kErrorUndefined;
    }
    spr_enable_ = *enable;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::Prepare(LayerStack *layer_stack) {
  DTRACE_SCOPED();
  ClientLock lock(disp_mutex_);

  DisplayError error = PrePrepare(layer_stack);
  if (error == kErrorNone) {
    return kErrorNone;
  }

  if (error == kErrorNeedsLutRegen && (ForceToneMapUpdate(layer_stack) == kErrorNone)) {
    return kErrorNone;
  }

  error = DisplayBase::Prepare(layer_stack);
  if (error != kErrorNone) {
    return error;
  }

  UpdateQsyncConfig();

  CacheFrameROI();

  NotifyDppsHdrPresent(layer_stack);

  pending_commit_ = true;

  return kErrorNone;
}

void DisplayBuiltIn::NotifyDppsHdrPresent(LayerStack *layer_stack) {
  if (hdr_present_ != layer_stack->flags.hdr_present) {
    hdr_present_ = layer_stack->flags.hdr_present;
    DLOGV_IF(kTagDisplay, "Notify DPPS hdr_present %d on display %d-%d", hdr_present_,
             display_id_, display_type_);
    DppsNotifyPayload info = {};
    info.is_primary = IsPrimaryDisplay();
    info.payload = &hdr_present_;
    info.payload_size = sizeof(hdr_present_);
    dpps_info_.DppsNotifyOps(kDppsHdrPresentEvent, &info, sizeof(info));
  }
}

void DisplayBuiltIn::CacheFrameROI() {
  for (int i = 0; i < core_count_; i++) {
    uint32_t core_id = hw_resource_info_[i].core_id;
    left_frame_roi_[i] = {};
    right_frame_roi_[i] = {};
    if (disp_layer_stack_->info.at(core_id).left_frame_roi.size() &&
      disp_layer_stack_->info.at(core_id).right_frame_roi.size()) {
      left_frame_roi_[i] = disp_layer_stack_->info.at(core_id).left_frame_roi.at(0);
      right_frame_roi_[i] = disp_layer_stack_->info.at(core_id).right_frame_roi.at(0);
    }
  }
}

void DisplayBuiltIn::UpdateQsyncConfig() {
  // QSync and AVR Step features are de-coupled on CMD Mode panel.
  if ((client_ctx_.hw_panel_info.mode == kModeVideo) && !client_ctx_.hw_panel_info.qsync_support) {
    return;
  }

  // Get qsync min fps for the current mode
  uint32_t qsync_mode_min_fps = 0;
  dpu_core_mux_->GetQsyncFps(&qsync_mode_min_fps);
  QSyncMode mode = kQSyncModeNone;
  if (!qsync_mode_min_fps) {
    // Set qsync mode to 0 when the current mode doesn't support it.
    mode = kQSyncModeNone;
    DLOGV_IF(kTagDisplay, "Qsync disabled as current mode doesn't support it");
  } else if (lower_fps_ && enable_qsync_idle_) {
    // Override to continuous mode upon idling.
    mode = kQSyncModeContinuous;
    DLOGV_IF(kTagDisplay, "Qsync entering continuous mode");
  } else {
    // Set Qsync mode requested by client.
    mode = qsync_mode_;
    DLOGV_IF(kTagDisplay, "Restoring display %d-%d client's qsync mode: %d", display_id_,
             display_type_, mode);
  }

  disp_layer_stack_->stack_info.common_info.hw_avr_info.update = needs_avr_update_;
  if (mode != active_qsync_mode_) {
    disp_layer_stack_->stack_info.common_info.hw_avr_info.update.set(kUpdateAVRModeFlag);
  }
  disp_layer_stack_->stack_info.common_info.hw_avr_info.mode = GetAvrMode(mode);
  disp_layer_stack_->stack_info.common_info.hw_avr_info.step_enabled = avr_step_enabled_;

  DLOGV_IF(kTagDisplay, "display %d-%d update: %d mode: %d AVR Step state: %d", display_id_,
           display_type_, disp_layer_stack_->stack_info.common_info.hw_avr_info.update, mode,
           avr_step_enabled_);

  // Store active mode.
  active_qsync_mode_ = mode;
}

void DisplayBuiltIn::HandleUpdateTransferTime(QSyncMode mode) {
  if (mode == kQSyncModeNone) {
    DLOGI("Qsync mode set to %d successfully, setting transfer time to min: %d", mode,
          client_ctx_.hw_panel_info.transfer_time_us_min);
    UpdateTransferTime(client_ctx_.hw_panel_info.transfer_time_us_min);
  } else {
    DLOGI("Qsync mode set to %d successfully, setting transfer time to max: %d", mode,
          client_ctx_.hw_panel_info.transfer_time_us_max);
    UpdateTransferTime(client_ctx_.hw_panel_info.transfer_time_us_max);
  }
}

HWAVRModes DisplayBuiltIn::GetAvrMode(QSyncMode mode) {
  switch (mode) {
     case kQSyncModeNone:
       return kQsyncNone;
     case kQSyncModeContinuous:
       return kContinuousMode;
     case kQsyncModeOneShot:
     case kQsyncModeOneShotContinuous:
       return kOneShotMode;
     default:
       return kQsyncNone;
  }
}

void DisplayBuiltIn::initColorSamplingState() {
  samplingState = SamplingState::Off;
  histogramCtrl.object_type = DRM_MODE_OBJECT_CRTC;
  histogramCtrl.feature_id = sde_drm::DRMDPPSFeatureID::kFeatureAbaHistCtrl;
  histogramCtrl.value = sde_drm::HistModes::kHistDisabled;

  histogramIRQ.object_type = DRM_MODE_OBJECT_CRTC;
  histogramIRQ.feature_id = sde_drm::DRMDPPSFeatureID::kFeatureAbaHistIRQ;
  histogramIRQ.value = sde_drm::HistModes::kHistDisabled;
  histogramSetup = true;
}

DisplayError DisplayBuiltIn::setColorSamplingState(SamplingState state) {
  samplingState = state;
  if (samplingState == SamplingState::On) {
    histogramCtrl.value = sde_drm::HistModes::kHistEnabled;
    histogramIRQ.value = sde_drm::HistModes::kHistEnabled;
    if (client_ctx_.hw_panel_info.mode == kModeCommand) {
      uint32_t pending;
      ControlPartialUpdate(false /* enable */, &pending);
    }
  } else {
    histogramCtrl.value = sde_drm::HistModes::kHistDisabled;
    histogramIRQ.value = sde_drm::HistModes::kHistDisabled;
    if (client_ctx_.hw_panel_info.mode == kModeCommand) {
      uint32_t pending;
      ControlPartialUpdate(true /* enable */, &pending);
    }
  }

  // effectively drmModeAtomicAddProperty for the SDE_DSPP_HIST_CTRL_V1
  return DppsProcessOps(kDppsSetFeature, &histogramCtrl, sizeof(histogramCtrl));
}

DisplayError DisplayBuiltIn::colorSamplingOn() {
  if (!histogramSetup) {
    return kErrorParameters;
  }
  return setColorSamplingState(SamplingState::On);
}

DisplayError DisplayBuiltIn::colorSamplingOff() {
  if (!histogramSetup) {
    return kErrorParameters;
  }
  return setColorSamplingState(SamplingState::Off);
}

DisplayError DisplayBuiltIn::SetupSPR() {
  int spr_prop_value = 0;
  int spr_bypass_prop_value = 0;
  int spr_disable_value = 0;
  Debug::GetProperty(ENABLE_SPR, &spr_prop_value);
  Debug::GetProperty(ENABLE_SPR_BYPASS, &spr_bypass_prop_value);

  if (IsPrimaryDisplay()) {
    Debug::Get()->GetProperty(DISABLE_SPR_PRIMARY, &spr_disable_value);
  } else {
    Debug::Get()->GetProperty(DISABLE_SPR_SECONDARY, &spr_disable_value);
  }

  if (spr_prop_value && !spr_disable_value) {
    SPRInputConfig spr_cfg;
    spr_cfg.panel_name = std::string(client_ctx_.hw_panel_info.panel_name);
    spr_cfg.spr_bypassed = (spr_bypass_prop_value) ? true : false;
    spr_ = pf_factory_->CreateSPRIntf(spr_cfg, prop_intf_);

    if (spr_ == nullptr) {
      DLOGE("Failed to create SPR interface");
      return kErrorResources;
    }

    if (spr_->Init() != 0) {
      DLOGE("Failed to initialize SPR");
      return kErrorResources;
    }

    spr_bypassed_ = spr_cfg.spr_bypassed;
    if (color_mgr_) {
      color_mgr_->ColorMgrSetSprIntf(spr_);
    }
    comp_manager_->SetSprIntf(display_comp_ctx_, spr_);
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetupDemura() {
  DemuraInputConfig input_cfg;
  input_cfg.secure_session = false;  // TODO(user): Integrate with secure solution
  std::string brightness_base;
  dpu_core_mux_->GetPanelBrightnessBasePath(&brightness_base);
  input_cfg.brightness_path = brightness_base+"brightness";

  std::vector<FetchResourceList> frlv;
  frlv.resize(core_count_);
  comp_manager_->GetDemuraFetchResources(display_comp_ctx_, &frlv);
  // ToDo(devanshi) : handle for all DPU instead of DPU 0
  auto frl = frlv[0];
  for (auto &fr : frl) {
    int i = std::get<1>(fr);  // fetch resource index
    input_cfg.resources.set(i);
  }

#ifdef TRUSTED_VM
  input_cfg.secure_session = true;
#endif
  input_cfg.panel_id = panel_id_;
  DLOGI("panel id %lx\n", input_cfg.panel_id);
  input_cfg.panel_name = client_ctx_.hw_panel_info.panel_name;
  input_cfg.display_intf = this;
  std::unique_ptr<DemuraIntf> demura =
      pf_factory_->CreateDemuraIntf(input_cfg, prop_intf_, buffer_allocator_, spr_);
  if (!demura) {
    DLOGE("Unable to create Demura on Display %d-%d", display_id_, display_type_);
    return kErrorMemory;
  }

  demura_ = std::move(demura);
  if (demura_->Init() != 0) {
    DLOGE("Unable to initialize Demura on Display %d-%d", display_id_, display_type_);
    return kErrorUndefined;
  }

  if (SetupCorrectionLayer() != kErrorNone) {
    DLOGE("Unable to setup Demura layer on Display %d-%d", display_id_, display_type_);
    return kErrorUndefined;
  }

  if (SetDemuraIntfStatus(true)) {
    return kErrorUndefined;
  }

  demura_current_idx_ = kDemuraDefaultIdx;

  comp_manager_->SetDemuraStatusForDisplay(display_id_, true);
  demura_intended_ = true;
  DLOGI("Enabled Demura Core!");

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetupCorrectionLayer() {
  if (abc_prop_) {
    return SetupABCLayer();
  } else {
    return SetupDemuraLayer();
  }
}

DisplayError DisplayBuiltIn::SetupDemuraLayer() {
  int ret = 0;
  GenericPayload pl;

  DemuraCorrectionSurfaces *corrdata = nullptr;
  if ((ret = pl.CreatePayload<DemuraCorrectionSurfaces>(corrdata))) {
    DLOGE("Failed to create payload for BufferInfo, error = %d", ret);
    return kErrorResources;
  }

  if ((ret = demura_->GetParameter(kDemuraFeatureParamCorrectionBuffer, &pl))) {
    DLOGE("Failed to get BufferInfo, error = %d", ret);
    return kErrorResources;
  }
  demura_layer_.clear();  // This will clear the old demura layers

  for (int buf_idx = 0; buf_idx < corrdata->surfaces.size(); buf_idx++) {
    if (!corrdata->valid[buf_idx])
      continue;
    Layer demura_layer = {};
#ifndef TRUSTED_VM
    demura_layer.input_buffer.buffer_id = corrdata->surfaces[buf_idx].alloc_buffer_info.id;
    demura_layer.input_buffer.handle_id = corrdata->surfaces[buf_idx].alloc_buffer_info.id;
#endif
    demura_layer.input_buffer.size = corrdata->surfaces[buf_idx].alloc_buffer_info.size;
    demura_layer.input_buffer.format = corrdata->surfaces[buf_idx].alloc_buffer_info.format;
    demura_layer.input_buffer.width = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_width;
    demura_layer.input_buffer.unaligned_width =
        corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_width;
    demura_layer.input_buffer.height = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_height;
    demura_layer.input_buffer.unaligned_height =
        corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_height;
    demura_layer.input_buffer.planes[0].fd = corrdata->surfaces[buf_idx].alloc_buffer_info.fd;
    demura_layer.input_buffer.planes[0].stride =
        corrdata->surfaces[buf_idx].alloc_buffer_info.stride;
    hfc_buffer_width_ = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_width;
    hfc_buffer_height_ = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_height;
    demura_layer.input_buffer.planes[0].offset = 0;
    demura_layer.input_buffer.flags.demura = 1;
    demura_layer.composition = kCompositionDemura;
    demura_layer.blending = kBlendingSkip;
    demura_layer.flags.is_demura = 1;
    // ROI must match input dimensions
    demura_layer.src_rect.top = 0;
    demura_layer.src_rect.left = 0;
    demura_layer.src_rect.right = corrdata->surfaces[buf_idx].buffer_config.width;
    demura_layer.src_rect.bottom = corrdata->surfaces[buf_idx].buffer_config.height;
    LogI(kTagNone, "Demura src: ", demura_layer.src_rect);
    demura_layer.dst_rect.top = 0;
    demura_layer.dst_rect.left = 0;
    demura_layer.dst_rect.right = corrdata->surfaces[buf_idx].buffer_config.width;
    demura_layer.dst_rect.bottom = corrdata->surfaces[buf_idx].buffer_config.height;
    LogI(kTagNone, "Demura dst: ", demura_layer.dst_rect);
    demura_layer.buffer_map = std::make_shared<LayerBufferMap>();
    demura_layer_.push_back(demura_layer);
  }
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetupABCLayer() {
  int ret = 0;
  GenericPayload pl;

  DemuraCorrectionSurfaces *corrdata = nullptr;
  if ((ret = pl.CreatePayload<DemuraCorrectionSurfaces>(corrdata))) {
    DLOGE("Failed to create payload for BufferInfo, error = %d", ret);
    return kErrorResources;
  }

  if ((ret = demura_->GetParameter(kDemuraFeatureParamCorrectionBuffer, &pl))) {
    DLOGE("Failed to get BufferInfo, error = %d", ret);
    return kErrorResources;
  }
  demura_layer_.clear();  // This will clear the old abc layers

  for (int buf_idx = 0; buf_idx < corrdata->surfaces.size(); buf_idx++) {
    if (!corrdata->valid[buf_idx])
      continue;
    Layer demura_layer = {};
    demura_layer.input_buffer.size = corrdata->surfaces[buf_idx].alloc_buffer_info.size;
    demura_layer.input_buffer.buffer_id = corrdata->surfaces[buf_idx].alloc_buffer_info.id;
    demura_layer.input_buffer.handle_id = corrdata->surfaces[buf_idx].alloc_buffer_info.id;
    demura_layer.input_buffer.format = corrdata->surfaces[buf_idx].alloc_buffer_info.format;
    demura_layer.input_buffer.width = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_width;
    demura_layer.input_buffer.unaligned_width =
        corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_width;
    demura_layer.input_buffer.height = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_height;
    demura_layer.input_buffer.unaligned_height =
        corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_height;
    demura_layer.input_buffer.planes[0].fd = corrdata->surfaces[buf_idx].alloc_buffer_info.fd;
    demura_layer.input_buffer.planes[0].stride =
        corrdata->surfaces[buf_idx].alloc_buffer_info.stride;
    hfc_buffer_width_ = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_width;
    hfc_buffer_height_ = corrdata->surfaces[buf_idx].alloc_buffer_info.aligned_height;
    demura_layer.input_buffer.planes[0].offset = 0;
    demura_layer.input_buffer.flags.demura = 1;
    demura_layer.composition = kCompositionDemura;
    demura_layer.blending = kBlendingSkip;
    demura_layer.flags.is_abc = 1;
    // ROI must match input dimensions
    demura_layer.src_rect.top = 0;
    demura_layer.src_rect.left = 0;
    demura_layer.src_rect.right = corrdata->surfaces[buf_idx].buffer_config.width;
    demura_layer.src_rect.bottom = corrdata->surfaces[buf_idx].buffer_config.height;
    LogI(kTagNone, "Demura src: ", demura_layer.src_rect);
    demura_layer.dst_rect.top = 0;
    demura_layer.dst_rect.left = 0;
    demura_layer.dst_rect.right = corrdata->surfaces[buf_idx].buffer_config.width;
    demura_layer.dst_rect.bottom = corrdata->surfaces[buf_idx].buffer_config.height;
    LogI(kTagNone, "Demura dst: ", demura_layer.dst_rect);
    demura_layer.buffer_map = std::make_shared<LayerBufferMap>();
    demura_layer_.push_back(demura_layer);
  }
  return kErrorNone;
}

void DisplayBuiltIn::PreCommit(LayerStack *layer_stack) {
  uint32_t app_layer_count = disp_layer_stack_->stack_info.app_layer_count;

  // Enabling auto refresh is async and needs to happen before commit ioctl
  if (client_ctx_.hw_panel_info.mode == kModeCommand) {
    bool enable = (app_layer_count == 1) && layer_stack->flags.single_buffered_layer_present;
    bool need_refresh = layer_stack->flags.single_buffered_layer_present && (app_layer_count > 1);

    dpu_core_mux_->SetAutoRefresh(enable);
    if (need_refresh) {
      event_handler_->Refresh();
    }
  }

  if (trigger_mode_debug_ != kFrameTriggerMax) {
    DisplayError error = dpu_core_mux_->SetFrameTrigger(trigger_mode_debug_);
    if (error != kErrorNone) {
      DLOGE("Failed to set frame trigger mode %d, err %d", (int)trigger_mode_debug_, error);
    } else {
      DLOGV_IF(kTagDisplay, "Set frame trigger mode %d on display %d-%d", trigger_mode_debug_,
               display_id_, display_type_);
      trigger_mode_debug_ = kFrameTriggerMax;
    }
  }

  // effectively drmModeAtomicAddProperty for SDE_DSPP_HIST_IRQ_V1
  if (histogramSetup) {
    SetDppsFeatureLocked(&histogramIRQ, sizeof(histogramIRQ));
  }
}

DisplayError DisplayBuiltIn::SetupABCFeature() {
  DemuraInputConfig input_cfg;
  input_cfg.secure_session = false;  // TODO(user): Integrate with secure solution
  std::string brightness_base;
  hw_intf_->GetPanelBrightnessBasePath(&brightness_base);
  input_cfg.brightness_path = brightness_base + "brightness";

  std::vector<FetchResourceList> frlv;
  frlv.resize(core_count_);
  comp_manager_->GetDemuraFetchResources(display_comp_ctx_, &frlv);
  // handled for DPU 0
  auto frl = frlv[0];
  for (auto &fr : frl) {
    int i = std::get<1>(fr);  // fetch resource index
    input_cfg.resources.set(i);
  }

#ifdef TRUSTED_VM
  // TBD: TUI path
#endif
  input_cfg.panel_id = panel_id_;
  input_cfg.panel_width = client_ctx_.display_attributes.x_pixels;
  input_cfg.panel_height = client_ctx_.display_attributes.y_pixels;
  input_cfg.panel_name = std::string(client_ctx_.hw_panel_info.panel_name);
  for (auto it = input_cfg.panel_name.begin(); it != input_cfg.panel_name.end(); ++it) {
    if (*it == ' ') {
      *it = '_';
    }
  }
  DLOGI("ABC panel id %lx actual panel-name %s\n", input_cfg.panel_id,
        input_cfg.panel_name.c_str());
  if (!abc_factory_) {
    DLOGE("Failed to get ABC feature Factory");
    return kErrorResources;
  }

  std::unique_ptr<DemuraIntf> abc_intf =
      abc_factory_->CreateABCIntf(input_cfg, prop_intf_, buffer_allocator_, this);
  if (!abc_intf) {
    DLOGE("Unable to create abc_intf on Display %d-%d", display_id_, display_type_);
    return kErrorMemory;
  }

  demura_ = std::move(abc_intf);
  if (demura_->Init() != 0) {
    DLOGE("Unable to initialize abc_intf on Display %d-%d", display_id_, display_type_);
    return kErrorUndefined;
  }

  if (SetDemuraIntfStatus(true)) {
    DLOGE("Failed to set ABC Status on Display %d", display_id_);
    return kErrorUndefined;
  }

  comp_manager_->SetDemuraStatusForDisplay(display_id_, true);
  abc_enabled_ = true;
  DLOGI("Enabled ABC Core!");
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetupABC() {
  DisplayError error = kErrorNone;
  uint64_t ret = 0, panel_id = 0;
  int value = 0;

  // if ABC is not set during resourse reservation
  if (!comp_manager_->GetDemuraStatus()) {
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    return kErrorNone;
  }

  if (IsPrimaryDisplay()) {
    Debug::Get()->GetProperty(DISABLE_ABC_PRIMARY, &value);
    DLOGI("primary panel id value %lx\n", panel_id);
  } else {
    Debug::Get()->GetProperty(DISABLE_ABC_SECONDARY, &value);
    DLOGI("secondary panel id value %lx\n", panel_id);
  }

  if (value > 0) {
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    return kErrorNone;
  } else if (value < 0) {
    return kErrorUndefined;
  }

  PanelFeaturePropertyInfo info;
  if (!panel_id) {
    info.prop_ptr = reinterpret_cast<uint64_t>(&panel_id);
    info.prop_id = kPanelFeatureDemuraPanelId;
    ret = prop_intf_->GetPanelFeature(&info);
    if (ret) {
      DLOGE("Failed to get panel id, error = %d", ret);
      return kErrorUndefined;
    }
  }
  panel_id_ = panel_id;

  error = SetupABCFeature();
  if (error != kErrorNone) {
    // Non-fatal but not expected, log error
    DLOGE("Mdnie B failed to initialize on display %d-%d, Error = %d", display_id_, display_type_,
          error);
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    if (demura_) {
      SetDemuraIntfStatus(false);
    }
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetupDemuraT0AndTn() {
  DisplayError error = kErrorNone;
  int ret = 0, value = 0, panel_id_w = 0;
  uint64_t panel_id = 0;
  bool demura_allowed = false, demuratn_allowed = false;

  if (!comp_manager_->GetDemuraStatus()) {
    comp_manager_->FreeDemuraFetchResources(display_id_);
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    return kErrorNone;
  }

  if (IsPrimaryDisplay()) {
    Debug::Get()->GetProperty(DEMURA_PRIMARY_PANEL_OVERRIDE_LOW, &panel_id_w);
    panel_id = static_cast<uint32_t>(panel_id_w);
    Debug::Get()->GetProperty(DEMURA_PRIMARY_PANEL_OVERRIDE_HIGH, &panel_id_w);
    panel_id |= ((static_cast<uint64_t>(panel_id_w)) << 32);
    Debug::Get()->GetProperty(DISABLE_DEMURA_PRIMARY, &value);
    DLOGI("panel overide total value %lx\n", panel_id);
  } else {
    Debug::Get()->GetProperty(DEMURA_SECONDARY_PANEL_OVERRIDE_LOW, &panel_id_w);
    panel_id = static_cast<uint32_t>(panel_id_w);
    Debug::Get()->GetProperty(DEMURA_SECONDARY_PANEL_OVERRIDE_HIGH, &panel_id_w);
    panel_id |= ((static_cast<uint64_t>(panel_id_w)) << 32);
    Debug::Get()->GetProperty(DISABLE_DEMURA_SECONDARY, &value);
    DLOGI("panel overide total value %lx\n", panel_id);
  }

  if (value > 0) {
    comp_manager_->FreeDemuraFetchResources(display_id_);
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    return kErrorNone;
  } else if (value < 0) {
    return kErrorUndefined;
  }

  PanelFeaturePropertyInfo info;
  if (!panel_id) {
    info.prop_ptr = reinterpret_cast<uint64_t>(&panel_id);
    info.prop_id = kPanelFeatureDemuraPanelId;
    ret = prop_intf_->GetPanelFeature(&info);
    if (ret) {
      DLOGE("Failed to get panel id, error = %d", ret);
      return kErrorUndefined;
    }
  }
  panel_id_ = panel_id;
  DLOGI("panel_id 0x%lx", panel_id_);

#if defined SDM_UNIT_TESTING || defined TRUSTED_VM
  demura_allowed = true;
  demuratn_allowed = true;
#else
  if (!feature_license_factory_) {
    DLOGI("Feature license factory is not available");
    return kErrorNone;
  }

  std::shared_ptr<FeatureLicenseIntf> feat_license_intf =
      feature_license_factory_->CreateFeatureLicenseIntf();
  if (!feat_license_intf) {
    feature_license_factory_ = nullptr;
    DLOGE("Failed to create FeatureLicenseIntf");
    return kErrorUndefined;
  }
  ret = feat_license_intf->Init();
  if (ret) {
    DLOGE("Failed to init FeatureLicenseIntf");
    return kErrorUndefined;
  }

  GenericPayload demura_pl, aa_pl, out_pl;
  DemuraValidatePermissionInput *demura_input = nullptr;
  ret = demura_pl.CreatePayload<DemuraValidatePermissionInput>(demura_input);
  if (ret) {
    DLOGE("Failed to create the payload. Error:%d", ret);
    return kErrorUndefined;
  }

  bool *allowed = nullptr;
  ret = out_pl.CreatePayload<bool>(allowed);
  if (ret) {
    DLOGE("Failed to create the payload. Error:%d", ret);
    return kErrorUndefined;
  }

  demura_input->id = kDemura;
  demura_input->panel_id = panel_id_;
  ret = feat_license_intf->ProcessOps(kValidatePermission, demura_pl, &out_pl);
  if (ret) {
    DLOGE("Failed to get the license permission for Demura. Error:%d", ret);
    return kErrorUndefined;
  }
  demura_allowed = *allowed;

  AntiAgingValidatePermissionInput *aa_input = nullptr;
  ret = aa_pl.CreatePayload<AntiAgingValidatePermissionInput>(aa_input);
  if (ret) {
    DLOGE("Failed to create the payload. Error:%d", ret);
    return kErrorUndefined;
  }

  aa_input->id = kAntiAging;
  ret = feat_license_intf->ProcessOps(kValidatePermission, aa_pl, &out_pl);
  if (ret) {
    DLOGE("Failed to get the license permission for Anti-aging. Error:%d", ret);
    return kErrorUndefined;
  }
  demuratn_allowed = *allowed;
#endif

  DLOGI("Demura enable allowed %d, Anti-aging enable allowed %d", demura_allowed, demuratn_allowed);
  if (demura_allowed) {
    demuratn_permanent_disabled_ = GetDemuraTnUserCtrl();
    error = SetupDemura();
    if (error != kErrorNone) {
      // Non-fatal but not expected, log error
      DLOGE("Demura failed to initialize on display %d-%d, Error = %d", display_id_, display_type_,
            error);
      comp_manager_->FreeDemuraFetchResources(display_id_);
      comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
      if (demura_) {
        SetDemuraIntfStatus(false);
      }
    } else if (demuratn_allowed && demuratn_factory_ && !demuratn_permanent_disabled_) {
      error = SetupDemuraTn();
      if (error != kErrorNone) {
        DLOGW("Failed to setup DemuraTn, Error = %d", error);
      }
    }
  }
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetupDemuraTn() {
  int ret = 0;

  if (!demura_) {
    DLOGI("Demura is not enabled, cannot setup DemuraTn");
    return kErrorNone;
  }

  demuratn_ = demuratn_factory_->CreateDemuraTnCoreUvmIntf(demura_, buffer_allocator_, this);
  if (!demuratn_) {
    DLOGE("Failed to create demuraTnCoreUvmIntf");
    return kErrorUndefined;
  }

  ret = demuratn_->Init();
  if (ret) {
    DLOGE("Failed to init demuraTnCoreUvmIntf, ret %d", ret);
    demuratn_.reset();
    demuratn_ = nullptr;
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::EnableDemuraTn(bool enable) {
  int ret = 0;
  bool *en = nullptr;
  GenericPayload payload;

  if (!demuratn_) {
    DLOGE("demuratn_ is nullptr");
    return kErrorUndefined;
  }

  if (demuratn_enabled_ == enable)
    return kErrorNone;

  ret = payload.CreatePayload(en);
  if (ret) {
    DLOGE("Failed to create enable payload");
    return kErrorUndefined;
  }
  *en = enable;

  if (enable) {  // make sure init is ready before enabling
    DemuraTnCoreState *init_ready = nullptr;
    GenericPayload ready_pl;
    ret = ready_pl.CreatePayload<DemuraTnCoreState>(init_ready);
    if (ret) {
      DLOGE("failed to create the payload. Error:%d", ret);
      return kErrorUndefined;
    }

    ret = demuratn_->GetParameter(kDemuraTnCoreUvmParamInitReady, &ready_pl);
    if (ret) {
      DLOGE("GetParameter for InitReady failed ret %d", ret);
      return kErrorUndefined;
    }
    if (*init_ready == kDemuraTnCoreNotReady) {
      return kErrorNone;
    } else if (*init_ready == kDemuraTnCoreError) {
      DLOGE("DemuraTn init ready state returns error");
      int rc = demuratn_->Deinit();
      if (rc)
        DLOGE("Failed to deinit DemuraTn ret %d", rc);
      demuratn_.reset();
      demuratn_ = nullptr;
      return kErrorUndefined;
    } else if (*init_ready == kDemuraTnCoreReady) {
      ret = demuratn_->SetParameter(kDemuraTnCoreUvmParamEnable, payload);
      if (ret) {
        DLOGE("SetParameter for enable failed ret %d", ret);
        return kErrorUndefined;
      }
      demuratn_enabled_ = true;
    }
  } else {
    ret = demuratn_->SetParameter(kDemuraTnCoreUvmParamEnable, payload);
    if (ret) {
      DLOGE("SetParameter for enable failed ret %d", ret);
      return kErrorUndefined;
    }
    demuratn_enabled_ = false;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::RetrieveDemuraTnFiles() {
  int ret = 0;
  GenericPayload payload;

  if (!demuratn_ || !demuratn_enabled_) {
    DLOGE("demuratn_ %pK demuratn_enabled_ %d", demuratn_.get(), demuratn_enabled_);
    return kErrorUndefined;
  }

  ret = demuratn_->SetParameter(kDemuraTnCoreUvmParamRetrieveFiles, payload);
  if (ret) {
    DLOGE("SetParameter for RetrieveFiles failed ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetDisplayStateForDemuraTn(DisplayState state) {
  int ret = 0;
  DisplayState *disp_state = nullptr;
  GenericPayload pl;

  ret = pl.CreatePayload<DisplayState>(disp_state);
  if (ret) {
    DLOGE("failed to create the payload. Error:%d", ret);
    return kErrorUndefined;
  }
  *disp_state = state;

  ret = demuratn_->SetParameter(kDemuraTnCoreUvmParamDisplayState, pl);
  if (ret) {
    DLOGE("SetParameter for DisplayState failed ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetUpCommit(LayerStack *layer_stack) {
  DTRACE_SCOPED();
  last_panel_mode_ = client_ctx_.hw_panel_info.mode;
  PreCommit(layer_stack);

  return DisplayBase::SetUpCommit(layer_stack);
}

DisplayError DisplayBuiltIn::CommitLocked(LayerStack *layer_stack) {
  DTRACE_SCOPED();
  last_panel_mode_ = client_ctx_.hw_panel_info.mode;
  PreCommit(layer_stack);

  return DisplayBase::CommitLocked(layer_stack);
}

DisplayError DisplayBuiltIn::PostCommit() {
  DisplayBase::PostCommit();
  // Mutex scope
  {
    lock_guard<recursive_mutex> obj(brightness_lock_);
    if (pending_brightness_) {
      Fence::Wait(retire_fence_);
      SetPanelBrightness(cached_brightness_);
      pending_brightness_ = false;
    }
  }

  if (commit_event_enabled_) {
    dpps_info_.DppsNotifyOps(kDppsCommitEvent, &display_type_, sizeof(display_type_));
  }

  deferred_config_.UpdateDeferCount();

  ReconfigureDisplay();

  if (deferred_config_.CanApplyDeferredState()) {
    validated_ = false;
    deferred_config_.Clear();
  }

  clock_gettime(CLOCK_MONOTONIC, &idle_timer_start_);
  int idle_time_ms = disp_layer_stack_->stack_info.common_info.set_idle_time_ms;
  if (idle_time_ms >= 0) {
    dpu_core_mux_->SetIdleTimeoutMs(UINT32(idle_time_ms));
    idle_time_ms_ = idle_time_ms;
  }

  if (switch_to_cmd_) {
    uint32_t pending;
    switch_to_cmd_ = false;
    ControlPartialUpdateLocked(true /* enable */, &pending);
  }

  if (last_panel_mode_ != client_ctx_.hw_panel_info.mode) {
    UpdateDisplayModeParams();
  }

  if (dpps_pu_nofiy_pending_) {
    dpps_pu_nofiy_pending_ = false;
    dpps_pu_lock_.Broadcast();
  }
  dpps_info_.Init(this, client_ctx_.hw_panel_info.panel_name, this, prop_intf_);

  if (demuratn_ && !demuratn_permanent_disabled_)
    EnableDemuraTn(true);

  HandleQsyncPostCommit();

  handle_idle_timeout_ = false;

  pending_commit_ = false;
  lower_fps_ = false;

  return kErrorNone;
}

void DisplayBuiltIn::HandleQsyncPostCommit() {
  if (qsync_mode_ == kQsyncModeOneShot) {
    // Reset qsync mode.
    SetQSyncMode(kQSyncModeNone);
  } else if (qsync_mode_ == kQsyncModeOneShotContinuous) {
    // No action needed.
  } else if (qsync_mode_ == kQSyncModeContinuous) {
    if (!avoid_qsync_mode_change_) {
      needs_avr_update_.reset();
    } else if (needs_avr_update_.any()) {
      validated_ = false;
      event_handler_->Refresh();
    }
  } else if (qsync_mode_ == kQSyncModeNone) {
    needs_avr_update_.reset();
  }

  avoid_qsync_mode_change_ = false;
  SetVsyncStatus(true /*Re-enable vsync.*/);

  bool notify_idle = enable_qsync_idle_ && (active_qsync_mode_ != kQSyncModeNone) &&
                     handle_idle_timeout_;
  if (notify_idle) {
    event_handler_->HandleEvent(kPostIdleTimeout);
  }

  bool qsync_enabled = (active_qsync_mode_ != kQSyncModeNone);
  if (qsync_enabled == qsync_enabled_) {
    return;
  }

  QsyncEventData event_data;
  event_data.enabled = qsync_enabled;
  event_data.refresh_rate = client_ctx_.display_attributes.fps;
  dpu_core_mux_->GetQsyncFps(&event_data.qsync_refresh_rate);
  event_handler_->HandleQsyncState(event_data);

  qsync_enabled_ = qsync_enabled;
}

void DisplayBuiltIn::UpdateDisplayModeParams() {
  if (client_ctx_.hw_panel_info.mode == kModeVideo) {
    uint32_t pending = 0;
    ControlPartialUpdateLocked(false /* enable */, &pending);
  } else if (client_ctx_.hw_panel_info.mode == kModeCommand) {
    // Flush idle timeout value currently set.
    comp_manager_->SetIdleTimeoutMs(display_comp_ctx_, 0, 0);
    switch_to_cmd_ = true;
  }
}

DisplayError DisplayBuiltIn::SetDisplayState(DisplayState state, bool teardown,
                                             shared_ptr<Fence> *release_fence) {
  ClientLock lock(disp_mutex_);
  DisplayError error = kErrorNone;
  HWDisplayMode panel_mode = client_ctx_.hw_panel_info.mode;

  if ((state == kStateOn) && deferred_config_.IsDeferredState()) {
    SetDeferredFpsConfig();
  }

  // Must go in NullCommit
  if (((demura_intended_ && demura_dynamic_enabled_) || abc_enabled_) &&
      comp_manager_->GetDemuraStatusForDisplay(display_id_) && (state == kStateOff)) {
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    SetDemuraIntfStatus(false);
  }

  error = DisplayBase::SetDisplayState(state, teardown, release_fence);
  if (error != kErrorNone) {
    return error;
  }

  if (secure_event_ == kTUITransitionEnd && state == kStateOff) {
    SetPanelBrightness(cached_brightness_);
    pending_brightness_ = false;
  }

  if (client_ctx_.hw_panel_info.mode != panel_mode) {
    UpdateDisplayModeParams();
  }

  // Set vsync enable state to false, as driver disables vsync during display power off.
  if (state == kStateOff) {
    vsync_enable_ = false;
    if (qsync_mode_ != kQSyncModeNone) {
      needs_avr_update_.set(kUpdateAVRModeFlag);
    }
  }

  if (pending_power_state_ != kPowerStateNone) {
    event_handler_->Refresh();
  }

  // Must only happen after NullCommit and get applied in next frame
  if (((demura_intended_ && demura_dynamic_enabled_) || abc_enabled_) &&
      !comp_manager_->GetDemuraStatusForDisplay(display_id_) &&
      (state == kStateOn || state == kStateDoze)) {
    comp_manager_->SetDemuraStatusForDisplay(display_id_, true);
    SetDemuraIntfStatus(true, demura_current_idx_);
  }

  if (demuratn_ && demuratn_enabled_) {
    SetDisplayStateForDemuraTn(state);
  }

  return kErrorNone;
}

void DisplayBuiltIn::SetIdleTimeoutMs(uint32_t active_ms, uint32_t inactive_ms) {
  ClientLock lock(disp_mutex_);
  comp_manager_->SetIdleTimeoutMs(display_comp_ctx_, active_ms, inactive_ms);
  validated_ = false;
  handle_idle_timeout_ = false;
}

DisplayError DisplayBuiltIn::SetDisplayMode(uint32_t mode) {
  DisplayError error = kErrorNone;

  // Limit scope of mutex to this block
  {
    ClientLock lock(disp_mutex_);
    HWDisplayMode hw_display_mode = static_cast<HWDisplayMode>(mode);
    uint32_t pending = 0;

    if (!active_) {
      DLOGW("Invalid display state = %d. Panel must be on.", state_);
      return kErrorNotSupported;
    }

    if (hw_display_mode != kModeCommand && hw_display_mode != kModeVideo) {
      DLOGW("Invalid panel mode parameters on display %d-%d. Requested = %d",
            display_id_, display_type_, hw_display_mode);
      return kErrorParameters;
    }

    if (hw_display_mode == client_ctx_.hw_panel_info.mode) {
      DLOGW("Same display mode requested on display %d-%d. Current = %d, Requested = %d",
            display_id_, display_type_, client_ctx_.hw_panel_info.mode, hw_display_mode);
      return kErrorNone;
    }

    error = dpu_core_mux_->SetDisplayMode(hw_display_mode);
    if (error != kErrorNone) {
      DLOGW("Retaining current display mode on display %d-%d. Current = %d, Requested = %d",
            display_id_, display_type_, client_ctx_.hw_panel_info.mode, hw_display_mode);
      return error;
    }

    avoid_qsync_mode_change_ = true;
    DisplayBase::ReconfigureDisplay();

    if (mode == kModeVideo) {
      ControlPartialUpdateLocked(false /* enable */, &pending);
      uint32_t active_ms = 0;
      uint32_t inactive_ms = 0;
      Debug::GetIdleTimeoutMs(&active_ms, &inactive_ms);
      comp_manager_->SetIdleTimeoutMs(display_comp_ctx_, active_ms, inactive_ms);
    } else if (mode == kModeCommand) {
      // Flush idle timeout value currently set.
      comp_manager_->SetIdleTimeoutMs(display_comp_ctx_, 0, 0);
      switch_to_cmd_ = true;
    }
  }

  // Request for a new draw cycle. New display mode will get applied on next draw cycle.
  // New idle time will get configured as part of this.
  event_handler_->Refresh();

  return error;
}

DisplayError DisplayBuiltIn::SetPanelBrightness(float brightness) {
  lock_guard<recursive_mutex> obj(brightness_lock_);

  if (brightness != -1.0f && !(0.0f <= brightness && brightness <= 1.0f)) {
    DLOGE("Bad brightness value = %f", brightness);
    return kErrorParameters;
  }

  // -1.0f = off, 0.0f = min, 1.0f = max
  float level_remainder = 0.0f;
  int level = 0;
  if (brightness == -1.0f) {
    level = 0;
  } else {
    // Node only supports int level, so store the float remainder for accurate GetPanelBrightness
    float max = client_ctx_.hw_panel_info.panel_max_brightness;
    float min = client_ctx_.hw_panel_info.panel_min_brightness;
    if (min >= max) {
      DLOGE("Minimum brightness is greater than or equal to maximum brightness");
      return kErrorDriverData;
    }
    float t = (brightness * (max - min)) + min;
    level = static_cast<int>(t);
    level_remainder = t - level;
  }

  DisplayError err = dpu_core_mux_->SetPanelBrightness(level);
  if (enable_brightness_drm_prop_) {
    event_handler_->Refresh();
  }
  if (err == kErrorNone) {
    level_remainder_ = level_remainder;
    pending_brightness_ = false;
    comp_manager_->SetBacklightLevel(display_comp_ctx_, level);
#ifdef DSEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d (%f percent) primary(%d) display(%d)", 
          level, brightness * 100, hw_panel_info_.is_primary_panel, display_id_);
#endif
    DLOGI_IF(kTagDisplay, "Setting brightness to level %d (%f percent)", level,
             brightness * 100);

    if (demura_intended_ && comp_manager_->GetDemuraStatusForDisplay(display_id_)) {
      if (!demura_) {
        DLOGE("demura_ is nullptr");
        return kErrorParameters;
      }

      GenericPayload pl;
      int32_t *need_screen_refresh = nullptr;
      int rc = 0;
      if ((rc = pl.CreatePayload<int32_t>(need_screen_refresh))) {
        DLOGE("Failed to create payload for need_screen_refresh, error = %d", rc);
        return kErrorParameters;
      }

      rc = demura_->GetParameter(kDemuraFeatureParamNeedScreenRefresh, &pl);
      if (rc) {
        DLOGE("Failed to get need screen refresh, error %d", rc);
        return kErrorParameters;
      }

      if (*need_screen_refresh) {
        event_handler_->Refresh();
      }
    }
  } else if (err == kErrorDeferred) {
    // TODO(user): I8508d64a55c3b30239c6ed2886df391407d22f25 causes mismatch between perceived
    // power state and actual panel power state. Requires a rework. Below check will set up
    // deferment of brightness operation if DAL reports defer use case.
    cached_brightness_ = brightness;
    pending_brightness_ = true;
#ifdef DSEC_GC_QC_DEBUG
    DLOGI("Deferred : active(%d) state_(%d), pending_power_state_(%d) level(%d) primary(%d) display(%d)",
          active_, state_, pending_power_state_, level, hw_panel_info_.is_primary_panel, display_id_);
#endif
    return kErrorNone;
  }

  return err;
}

DisplayError DisplayBuiltIn::SetBppMode(uint32_t bpp) {
  {
    ClientLock lock(disp_mutex_);

    DisplayError error = hw_intf_->SetBppMode(bpp);
    if (error != kErrorNone) {
      DLOGW("Retaining current panel bpp mode on display %d-%d. Requested = 0x%x",
            display_id_, display_type_, bpp);
      return error;
    }
    DisplayBase::ReconfigureDisplay();
    shared_ptr<Fence> release_fence = nullptr;
    SetDisplayState(kStateOff, 0, &release_fence);
    sleep(1);
    SetDisplayState(kStateOn, 0, &release_fence);
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::GetRefreshRateRange(uint32_t *min_refresh_rate,
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

DisplayError DisplayBuiltIn::SetRefreshRate(uint32_t refresh_rate, bool final_rate,
                                            bool idle_screen) {
  ClientLock lock(disp_mutex_);

  if (!active_ || !client_ctx_.hw_panel_info.dynamic_fps || qsync_mode_ != kQSyncModeNone ||
      disable_dyn_fps_) {
    return kErrorNotSupported;
  }

  if (refresh_rate < client_ctx_.hw_panel_info.min_fps ||
      refresh_rate > client_ctx_.hw_panel_info.max_fps) {
    DLOGE("Invalid Fps = %d request", refresh_rate);
    return kErrorParameters;
  }

  if (CanLowerFps(idle_screen) && !final_rate && !enable_qsync_idle_) {
    refresh_rate = client_ctx_.hw_panel_info.min_fps;
  }

  if (current_refresh_rate_ != refresh_rate) {
    DisplayError error = dpu_core_mux_->SetRefreshRate(refresh_rate);
    if (error != kErrorNone) {
      // Attempt to update refresh rate can fail if rf interfenence is detected.
      // Just drop min fps settting for now.
      handle_idle_timeout_ = false;
      return error;
    }

    error = comp_manager_->CheckEnforceSplit(display_comp_ctx_, refresh_rate);
    if (error != kErrorNone) {
      return error;
    }
  }

  // Set safe mode upon success.
  if (enhance_idle_time_ && handle_idle_timeout_ &&
      (refresh_rate == client_ctx_.hw_panel_info.min_fps)) {
    comp_manager_->ProcessIdleTimeout(display_comp_ctx_);
  }

  // On success, set current refresh rate to new refresh rate
  current_refresh_rate_ = refresh_rate;
  deferred_config_.MarkDirty();

  return ReconfigureDisplay();
}

bool DisplayBuiltIn::CanLowerFps(bool idle_screen) {
  if (!enhance_idle_time_) {
    return handle_idle_timeout_;
  }

  if (!handle_idle_timeout_ || !idle_screen) {
    return false;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  uint64_t elapsed_time_ms = GetTimeInMs(now) - GetTimeInMs(idle_timer_start_);
  bool can_lower = elapsed_time_ms >= UINT32(idle_time_ms_);
  DLOGV_IF(kTagDisplay, "lower fps: %d", can_lower);

  return can_lower;
}

DisplayError DisplayBuiltIn::VSync(int64_t timestamp) {
  DTRACE_SCOPED();
#ifdef SEC_GC_QC_VSYNC
  struct timespec ts;
  int64_t current = 0;
  if(clock_gettime(CLOCK_MONOTONIC, &ts) != EINVAL) {
    current = (int64_t)(ts.tv_sec)*1000000000 + (int64_t)(ts.tv_nsec);
  }
  vsync_current_timestamp_ = current;
#endif
  bool qsync_enabled = enable_qsync_idle_ && (active_qsync_mode_ != kQSyncModeNone);
  // Client isn't aware of underlying qsync mode.
  // Disable vsync propagation as long as qsync is enabled.
  bool propagate_vsync = vsync_enable_ && !drop_hw_vsync_ && !qsync_enabled;
  if (!propagate_vsync) {
#ifdef SEC_GC_QC_VSYNC
    vsync_disabled_timestamp_ = timestamp;
#endif
    // Re enable when display updates.
    SetVsyncStatus(false /*Disable vsync events.*/);
    return kErrorNone;
  }

#ifdef SEC_GC_QC_VSYNC
  if (vsync_count_ != 0 && timestamp == vsync_timestamp_[vsync_count_ - 1]) {
    return kErrorNone;
  }
#endif
  DisplayEventVSync vsync;
  vsync.timestamp = timestamp;
  event_handler_->VSync(vsync);

#ifdef SEC_GC_QC_VSYNC
  if(vsync_count_ == kMaxVSyncTimestampLength) {
    vsync_count_ = 0;
  }
  vsync_timestamp_[vsync_count_] = timestamp;
  vsync_count_++;
#endif
  return kErrorNone;
}

void DisplayBuiltIn::SetVsyncStatus(bool enable) {
  string trace_name = enable ? "enable" : "disable";
  DTRACE_BEGIN(trace_name.c_str());
  if (enable) {
    // Enable if vsync is still enabled.
    hw_events_intf_->SetEventState(HWEvent::VSYNC, vsync_enable_);
    pending_vsync_enable_ = false;
  } else {
    hw_events_intf_->SetEventState(HWEvent::VSYNC, false);
    pending_vsync_enable_ = true;
  }
  DTRACE_END();
}

void DisplayBuiltIn::IdleTimeout() {
  DTRACE_SCOPED();
  if ((state_ == kStateOff) || avr_step_enabled_) {
    return;
  }

  if (pending_commit_) {
    return;
  }

  handle_idle_timeout_ = true;
  if (!enhance_idle_time_) {
    comp_manager_->ProcessIdleTimeout(display_comp_ctx_);
  }

  validated_ = false;
  event_handler_->Refresh();
}

void DisplayBuiltIn::PingPongTimeout() {
  ClientLock lock(disp_mutex_);
  dpu_core_mux_->DumpDebugData();
}

void DisplayBuiltIn::IdlePowerCollapse() {
  if ((client_ctx_.hw_panel_info.mode == kModeCommand) || client_ctx_.hw_panel_info.vhm_support) {
    ClientLock lock(disp_mutex_);
    validated_ = false;
    comp_manager_->ProcessIdlePowerCollapse(display_comp_ctx_);
    event_handler_->HandleEvent(kIdleTimeout);
  }
}

DisplayError DisplayBuiltIn::ClearLUTs() {
  validated_ = false;
  comp_manager_->ProcessIdlePowerCollapse(display_comp_ctx_);
  return kErrorNone;
}

void DisplayBuiltIn::MMRMEvent(uint32_t clk) {
  DTRACE_SCOPED();
  DisplayBase::MMRMEvent(clk);
}

void DisplayBuiltIn::PanelDead() {
  {
    ClientLock lock(disp_mutex_);
    reset_panel_ = true;
    validated_ = false;
  }
  event_handler_->HandleEvent(kPanelDeadEvent);
  event_handler_->Refresh();
}

// HWEventHandler overload, not DisplayBase
void DisplayBuiltIn::HwRecovery(const HWRecoveryEvent sdm_event_code) {
  DisplayBase::HwRecovery(sdm_event_code);
}

void DisplayBuiltIn::Histogram(int histogram_fd, uint32_t blob_id) {
  event_handler_->HistogramEvent(histogram_fd, blob_id);
}

void DisplayBuiltIn::HandleBacklightEvent(float brightness_level) {
  DLOGI("backlight event occurred %f ipc_intf %p", brightness_level, ipc_intf_.get());
  if (ipc_intf_) {
    GenericPayload in;
    IPCBacklightParams *backlight_params = nullptr;
    int ret = in.CreatePayload<IPCBacklightParams>(backlight_params);
    if (ret) {
      DLOGW("failed to create the payload. Error:%d", ret);
      return;
    }
    float brightness = 0.0f;
    if (GetPanelBrightnessFromLevel(brightness_level, &brightness) != kErrorNone) {
      return;
    }
    backlight_params->brightness = brightness;
    backlight_params->is_primary = IsPrimaryDisplayLocked();
    if ((ret = ipc_intf_->SetParameter(kIpcParamBacklight, in))) {
      DLOGW("Failed to set backlight, error = %d", ret);
    }
    lock_guard<recursive_mutex> obj(brightness_lock_);
    cached_brightness_ = brightness;
    pending_brightness_ = true;
  }
}

DisplayError DisplayBuiltIn::GetPanelBrightness(float *brightness) {
  lock_guard<recursive_mutex> obj(brightness_lock_);

  DisplayError err = kErrorNone;
  int level = 0;
  if ((err = dpu_core_mux_->GetPanelBrightness(&level)) != kErrorNone) {
    return err;
  }
  return GetPanelBrightnessFromLevel(level, brightness);
}

DisplayError DisplayBuiltIn::GetPanelBrightnessFromLevel(float level, float *brightness) {
  // -1.0f = off, 0.0f = min, 1.0f = max
  float max = client_ctx_.hw_panel_info.panel_max_brightness;
  float min = client_ctx_.hw_panel_info.panel_min_brightness;
  if (level == 0) {
    *brightness = -1.0f;
  } else if ((max > min) && (min <= level && level <= max)) {
    *brightness = (static_cast<float>(level) + level_remainder_ - min) / (max - min);
  } else {
    min >= max ? DLOGE("Minimum brightness is greater than or equal to maximum brightness") :
                 DLOGE("Invalid brightness level %f", level);
    return kErrorDriverData;
  }

  DLOGI_IF(kTagDisplay, "Received level %f (%f percent)", level, *brightness * 100);

  return kErrorNone;
}

DisplayError DisplayBuiltIn::GetPanelBrightnessLevel(int *level) {
  lock_guard<recursive_mutex> obj(brightness_lock_);

  if (!level) {
    DLOGE("Invalid input pointer is null");
    return kErrorParameters;
  }

  DisplayError err = dpu_core_mux_->GetPanelBrightness(level);
  if (err != kErrorNone) {
    DLOGE("Failed to get panel brightness, err %d", err);
    return err;
  }

  DLOGI_IF(kTagDisplay, "Current panel level %d", *level);
  return err;
}

DisplayError DisplayBuiltIn::GetPanelMaxBrightness(uint32_t *max_brightness_level) {
  lock_guard<recursive_mutex> obj(brightness_lock_);

  if (!max_brightness_level) {
    DLOGE("Invalid input pointer is null");
    return kErrorParameters;
  }

  *max_brightness_level = static_cast<uint32_t>(client_ctx_.hw_panel_info.panel_max_brightness);

  DLOGI_IF(kTagDisplay, "Get panel max_brightness_level %u", *max_brightness_level);
  return kErrorNone;
}

DisplayError DisplayBuiltIn::ControlPartialUpdate(bool enable, uint32_t *pending) {
  ClientLock lock(disp_mutex_);
  return ControlPartialUpdateLocked(enable, pending);
}

DisplayError DisplayBuiltIn::ControlPartialUpdateLocked(bool enable, uint32_t *pending) {
  if (!pending) {
    return kErrorParameters;
  }

  if (dpps_info_.disable_pu_ && enable) {
    // Nothing to be done.
    DLOGI("partial update is disabled by DPPS for display %d-%d", display_id_, display_type_);
    return kErrorNotSupported;
  }

  *pending = 0;
  if (enable == partial_update_control_) {
    DLOGI("Same state transition is requested.");
    return kErrorNone;
  }
  validated_ = false;
  partial_update_control_ = enable;

  if (!enable) {
    // If the request is to turn off feature, new draw call is required to have
    // the new setting into effect.
    *pending = 1;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::DisablePartialUpdateOneFrame() {
  ClientLock lock(disp_mutex_);
  disable_pu_one_frame_ = true;
  validated_ = false;

  return kErrorNone;
}

DisplayError DisplayBuiltIn::DisablePartialUpdateOneFrameInternal() {
  disable_pu_one_frame_ = true;
  validated_ = false;

  return kErrorNone;
}

DisplayError DisplayBuiltIn::DppsProcessOps(enum DppsOps op, void *payload, size_t size) {
  DisplayError error = kErrorNone;
  uint32_t pending;
  bool enable = false;
  DppsDisplayInfo *info;

  switch (op) {
    case kDppsSetFeature:
      if (!payload) {
        DLOGE("Invalid payload parameter for op %d", op);
        error = kErrorParameters;
        break;
      }
      {
        ClientLock lock(disp_mutex_);
        error = SetDppsFeatureLocked(payload, size);
      }
      break;
    case kDppsGetFeatureInfo:
      if (!payload) {
        DLOGE("Invalid payload parameter for op %d", op);
        error = kErrorParameters;
        break;
      }
      error = dpu_core_mux_->GetDppsFeatureInfo(payload, size);
      break;
    case kDppsScreenRefresh:
      event_handler_->Refresh();
      break;
    case kDppsPartialUpdate: {
      int ret;
      if (!payload) {
        DLOGE("Invalid payload parameter for op %d", op);
        error = kErrorParameters;
        break;
      }
      enable = *(reinterpret_cast<bool *>(payload));
      dpps_info_.disable_pu_ = !enable;
      ControlPartialUpdate(enable, &pending);
      event_handler_->Refresh();
      {
        ClientLock lock(disp_mutex_);
        validated_ = false;
        dpps_pu_nofiy_pending_ = true;
      }
      ret = dpps_pu_lock_.WaitFinite(kPuTimeOutMs);
      if (ret) {
        DLOGW("failed to %s partial update ret %d", ((enable) ? "enable" : "disable"), ret);
        error = kErrorTimeOut;
      }
      break;
    }
    case kDppsRequestCommit:
      if (!payload) {
        DLOGE("Invalid payload parameter for op %d", op);
        error = kErrorParameters;
        break;
      }
      {
        ClientLock lock(disp_mutex_);
        commit_event_enabled_ = *(reinterpret_cast<bool *>(payload));
      }
      break;
    case kDppsGetDisplayInfo:
      if (!payload) {
        DLOGE("Invalid payload parameter for op %d", op);
        error = kErrorParameters;
        break;
      }
      info = reinterpret_cast<DppsDisplayInfo *>(payload);
      info->width = client_ctx_.display_attributes.x_pixels;
      info->height = client_ctx_.display_attributes.y_pixels;
      info->is_primary = IsPrimaryDisplayLocked();
      info->display_id = display_id_;
      info->display_type = display_type_;
      info->fps = enable_dpps_dyn_fps_ ? client_ctx_.display_attributes.fps : 0;

      error = dpu_core_mux_->GetPanelBrightnessBasePath(&(info->brightness_base_path));
      if (error != kErrorNone) {
        DLOGE("Failed to get brightness base path %d", error);
      }
      break;
    case kDppsSetPccConfig:
      error = color_mgr_->ColorMgrSetLtmPccConfig(payload, size);
      if (error != kErrorNone) {
        DLOGE("Failed to set PCC config to ColorManagerProxy, error %d", error);
      } else {
        ClientLock lock(disp_mutex_);
        validated_ = false;
        DisablePartialUpdateOneFrameInternal();
      }
      break;
    default:
      DLOGE("Invalid input op %d", op);
      error = kErrorParameters;
      break;
  }
  return error;
}

DisplayError DisplayBuiltIn::SetDisplayDppsAdROI(void *payload) {
  ClientLock lock(disp_mutex_);
  DisplayError err = kErrorNone;

  err = dpu_core_mux_->SetDisplayDppsAdROI(payload);
  if (err != kErrorNone)
    DLOGE("Failed to set ad roi config, err %d", err);

  return err;
}

DisplayError DisplayBuiltIn::SetFrameTriggerMode(FrameTriggerMode mode) {
  ClientLock lock(disp_mutex_);
  validated_ = false;
  trigger_mode_debug_ = mode;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::GetStcColorModes(snapdragoncolor::ColorModeList *mode_list) {
  ClientLock lock(disp_mutex_);
  if (!mode_list) {
    return kErrorParameters;
  }

  if (!color_mgr_) {
    return kErrorNotSupported;
  }

  mode_list->list = stc_color_modes_.list;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetStcColorMode(const snapdragoncolor::ColorMode &color_mode) {
  ClientLock lock(disp_mutex_);
  if (!color_mgr_) {
    return kErrorNotSupported;
  }
  DisplayError ret = kErrorNone;
  PrimariesTransfer blend_space = {};
  blend_space = GetBlendSpaceFromStcColorMode(color_mode);
  ret = comp_manager_->SetBlendSpace(display_comp_ctx_, blend_space);
  if (ret != kErrorNone) {
    DLOGE("SetBlendSpace failed, ret = %d on display %d-%d", ret, display_id_, display_type_);
  }

  ret = dpu_core_mux_->SetBlendSpace(blend_space);
  if (ret != kErrorNone) {
    DLOGE("Failed to pass blend space, ret = %d on display %d-%d", ret, display_id_,
          display_type_);
  }

  ret = color_mgr_->ColorMgrSetStcMode(color_mode);
  if (ret != kErrorNone) {
    DLOGE("Failed to set stc color mode, ret = %d on display %d-%d", ret,
          display_id_, display_type_);
    return ret;
  }

  current_color_mode_ = color_mode;

  DynamicRangeType dynamic_range = kSdrType;
  if (std::find(color_mode.hw_assets.begin(), color_mode.hw_assets.end(),
                snapdragoncolor::kPbHdrBlob) != color_mode.hw_assets.end()) {
    dynamic_range = kHdrType;
  }
  if ((color_mode.gamut == ColorPrimaries_BT2020 && color_mode.gamma == Transfer_SMPTE_ST2084) ||
      (color_mode.gamut == ColorPrimaries_BT2020 && color_mode.gamma == Transfer_HLG)) {
    dynamic_range = kHdrType;
  }
  comp_manager_->ControlDpps(dynamic_range != kHdrType);

  return ret;
}

DisplayError DisplayBuiltIn::NotifyDisplayCalibrationMode(bool in_calibration) {
  ClientLock lock(disp_mutex_);
  if (!color_mgr_) {
    return kErrorNotSupported;
  }
  DisplayError ret = kErrorNone;
  ret = color_mgr_->NotifyDisplayCalibrationMode(in_calibration);
  if (ret != kErrorNone) {
    DLOGE("Failed to notify QDCM Mode status, ret = %d state = %d", ret, in_calibration);
  }

  return ret;
}

std::string DisplayBuiltIn::Dump() {
  ClientLock lock(disp_mutex_);
  uint32_t active_index = 0;
  uint32_t num_modes = 0;
  std::ostringstream os;
  char capabilities[16];
  HWPanelInfo hw_panel_info = client_ctx_.hw_panel_info;
  HWDisplayAttributes display_attributes = client_ctx_.display_attributes;
  HWMixerAttributes mixer_attributes = client_ctx_.mixer_attributes;

  dpu_core_mux_->GetNumDisplayAttributes(&num_modes);
  dpu_core_mux_->GetActiveConfig(&active_index);

  os << "device type:" << display_type_;
  os << " DrawMethod: " << draw_method_;
  os << "\nstate: " << state_ << " vsync on: " << vsync_enable_
     << " max. mixer stages: " << max_mixer_stages_;
  if (disp_layer_stack_->stack_info.noise_layer_info.enable) {
    os << "\nNoise z-orders: [" << disp_layer_stack_->stack_info.noise_layer_info.zpos_noise
       << "," << disp_layer_stack_->stack_info.noise_layer_info.zpos_attn << "]";
  }
  os << "\nnum configs: " << num_modes << " active config index: " << active_index;
  os << "\nDisplay Attributes:";
  os << "\n Mode:" << (hw_panel_info.mode == kModeVideo ? "Video" : "Command");
  os << std::boolalpha;
  os << " Primary:" << hw_panel_info.is_primary_panel;
  os << " DynFPS:" << hw_panel_info.dynamic_fps;
  os << "\n HDR Panel:" << hw_panel_info.hdr_enabled;
  os << " QSync:" << hw_panel_info.qsync_support;
  os << " DynBitclk:" << hw_panel_info.dyn_bitclk_support;
  os << "\n Left Split:" << hw_panel_info.split_info.left_split
     << " Right Split:" << hw_panel_info.split_info.right_split;
  os << "\n PartialUpdate:" << hw_panel_info.partial_update;
  if (hw_panel_info.partial_update) {
    os << "\n ROI Min w:" << hw_panel_info.min_roi_width;
    os << " Min h:" << hw_panel_info.min_roi_height;
    os << " NeedsMerge: " << hw_panel_info.needs_roi_merge;
    os << " Alignment: l:" << hw_panel_info.left_align << " w:" << hw_panel_info.width_align;
    os << " t:" << hw_panel_info.top_align << " b:" << hw_panel_info.height_align;
  }
  os << "\n FPS min:" << hw_panel_info.min_fps << " max:" << hw_panel_info.max_fps
     << " cur:" << display_attributes.fps;
  os << " TransferTime: " << hw_panel_info.transfer_time_us << "us";
  os << " Min TransferTime: " << hw_panel_info.transfer_time_us_min << "us";
  os << " Max TransferTime: " << hw_panel_info.transfer_time_us_max << "us";
  os << " AllowedModeSwitch: " << hw_panel_info.allowed_mode_switch;
  os << " PanelModeCaps: ";
  snprintf(capabilities, sizeof(capabilities), "0x%x", hw_panel_info.panel_mode_caps);
  os << capabilities;
  os << " MaxBrightness:" << hw_panel_info.panel_max_brightness;
  os << "\n Display WxH: " << display_attributes.x_pixels << "x" << display_attributes.y_pixels;
  os << " MixerWxH: " << mixer_attributes.width << "x" << mixer_attributes.height;
  os << " DPI: " << display_attributes.x_dpi << "x" << display_attributes.y_dpi;
  os << " LM_Split: " << display_attributes.is_device_split;
  os << "\n vsync_period " << display_attributes.vsync_period_ns;
  os << " v_back_porch: " << display_attributes.v_back_porch;
  os << " v_front_porch: " << display_attributes.v_front_porch;
  os << " v_pulse_width: " << display_attributes.v_pulse_width;
  os << "\n v_total: " << display_attributes.v_total;
  os << " h_total: " << display_attributes.h_total;
  os << " clk: " << display_attributes.clock_khz;
  os << " Topology: " << display_attributes.topology;
  os << " Qsync mode: " << active_qsync_mode_;
  os << " CAC enabled: " << disp_layer_stack_->stack_info.enable_cac;
  os << std::noboolalpha;

  DynamicRangeType curr_dynamic_range = kSdrType;
  if (std::find(current_color_mode_.hw_assets.begin(), current_color_mode_.hw_assets.end(),
                snapdragoncolor::kPbHdrBlob) != current_color_mode_.hw_assets.end()) {
    curr_dynamic_range = kHdrType;
  }
  os << "\nCurrent Color Mode: gamut " << current_color_mode_.gamut << " gamma "
     << current_color_mode_.gamma << " intent " << current_color_mode_.intent << " Dynamice_range"
     << (curr_dynamic_range == kSdrType ? " SDR" : " HDR");

  for (int j = 0; j < core_count_; j++) {
    uint32_t core_id = hw_resource_info_[j].core_id;
    uint32_t num_hw_layers = UINT32(disp_layer_stack_->info.at(core_id).hw_layers.size());

    if (num_hw_layers == 0) {
      os << "\nNo hardware layers programmed";
      return os.str();
    }

    os << "\n\n Table for DPU - " << j << "\n";
    if (cwb_active_) {
      os << "\n Output buffer res: " << cwb_output_buf_.width << "x" << cwb_output_buf_.height
         << " format: " << GetFormatString(cwb_output_buf_.format);
    }

    HWLayersInfo &layer_info = disp_layer_stack_->info.at(core_id);
    for (uint32_t i = 0; i < layer_info.left_frame_roi.size(); i++) {
      LayerRect &l_roi = layer_info.left_frame_roi.at(i);
      LayerRect &r_roi = layer_info.right_frame_roi.at(i);

      os << "\nROI(LTRB)#" << i << " LEFT(" << INT(l_roi.left) << " " << INT(l_roi.top) << " " <<
        INT(l_roi.right) << " " << INT(l_roi.bottom) << ")";
      if (IsValid(r_roi)) {
      os << " RIGHT(" << INT(r_roi.left) << " " << INT(r_roi.top) << " " << INT(r_roi.right) << " "
        << INT(r_roi.bottom) << ")";
      }
    }

    LayerRect &fb_roi = disp_layer_stack_->stack_info.partial_fb_roi;
    if (IsValid(fb_roi)) {
      os << "\nPartial FB ROI(LTRB):(" << INT(fb_roi.left) << " " << INT(fb_roi.top) << " " <<
        INT(fb_roi.right) << " " << INT(fb_roi.bottom) << ")";
    }

    AppendRCMaskData(os);

    const char *header  = "\n| Idx |   Comp Type   |   Split   | Pipe |    W x H    |          Format          |  Src Rect (L T R B) |  Dst Rect (L T R B) |  Z | Pipe Flags | Deci(HxV) | CS | Rng | Tr |";  //NOLINT
    const char *newline = "\n|-----|---------------|-----------|------|-------------|--------------------------|---------------------|---------------------|----|------------|-----------|----|-----|----|";  //NOLINT
    const char *format  = "\n| %3s | %13s | %9s | %4d | %4d x %4d | %24s | %4d %4d %4d %4d | %4d %4d %4d %4d | %2s | %10s | %9s | %2s | %3s | %2s |";  //NOLINT

    os << "\n";
    os << newline;
    os << header;
    os << newline;

    for (uint32_t i = 0; i < num_hw_layers; i++) {
      uint32_t layer_index = disp_layer_stack_->info.at(core_id).index.at(i);
      // hw-layer from hw layers info
      Layer &hw_layer = disp_layer_stack_->info.at(core_id).hw_layers.at(i);
      LayerBuffer *input_buffer = &hw_layer.input_buffer;
      HWLayerConfig &layer_config = disp_layer_stack_->info.at(core_id).config[i];
      HWRotatorSession &hw_rotator_session = layer_config.hw_rotator_session;

      const char *comp_type = GetCompositionName(hw_layer.composition);
      const char *buffer_format = GetFormatString(input_buffer->format);
      const char *pipe_split[2] = { "Pipe-1", "Pipe-2" };
      const char *rot_pipe[2] = { "Rot-inl-1", "Rot-inl-2" };
      char idx[8];

      snprintf(idx, sizeof(idx), "%d", layer_index);

      for (uint32_t count = 0; count < hw_rotator_session.hw_block_count; count++) {
        char row[1024];
        HWRotateInfo &rotate = hw_rotator_session.hw_rotate_info[count];
        LayerRect &src_roi = rotate.src_roi;
        LayerRect &dst_roi = rotate.dst_roi;
        char rot[12] = { 0 };

        snprintf(rot, sizeof(rot), "Rot-%s-%d", layer_config.use_inline_rot ?
                 "inl" : "off", count + 1);

        snprintf(row, sizeof(row), format, idx, comp_type, rot,
                 0, input_buffer->width, input_buffer->height, buffer_format,
                 INT(src_roi.left), INT(src_roi.top), INT(src_roi.right), INT(src_roi.bottom),
                 INT(dst_roi.left), INT(dst_roi.top), INT(dst_roi.right), INT(dst_roi.bottom),
                 "-", "-    ", "-    ", "-", "-", "-");
        os << row;
        // print the below only once per layer block, fill with spaces for rest.
        idx[0] = 0;
        comp_type = "";
      }

      if (hw_rotator_session.hw_block_count > 0) {
        input_buffer = &hw_rotator_session.output_buffer;
        buffer_format = GetFormatString(input_buffer->format);
      }

      if (layer_config.use_solidfill_stage) {
        LayerRect src_roi = layer_config.hw_solidfill_stage.roi;
        const char *decimation = "";
        char flags[16] = { 0 };
        char z_order[8] = { 0 };
        const char *color_primary = "";
        const char *range = "";
        const char *transfer = "";
        char row[1024] = { 0 };

        snprintf(z_order, sizeof(z_order), "%d", layer_config.hw_solidfill_stage.z_order);
        snprintf(flags, sizeof(flags), "0x%08x", hw_layer.flags.flags);
        snprintf(row, sizeof(row), format, idx, comp_type, pipe_split[0],
                 0, INT(src_roi.right), INT(src_roi.bottom),
                 buffer_format, INT(src_roi.left), INT(src_roi.top),
                 INT(src_roi.right), INT(src_roi.bottom), INT(src_roi.left),
                 INT(src_roi.top), INT(src_roi.right), INT(src_roi.bottom),
                 z_order, flags, decimation, color_primary, range, transfer);
        os << row;
        continue;
      }

      for (uint32_t count = 0; count < 2; count++) {
        char decimation[16] = { 0 };
        char flags[16] = { 0 };
        char z_order[8] = { 0 };
        char color_primary[8] = { 0 };
        char range[8] = { 0 };
        char transfer[8] = { 0 };
        bool rot = layer_config.use_inline_rot;

        HWPipeInfo &pipe = (count == 0) ? layer_config.left_pipe : layer_config.right_pipe;

        if (!pipe.valid) {
          continue;
        }

        LayerRect src_roi = pipe.src_roi;
        LayerRect &dst_roi = pipe.dst_roi;

        snprintf(z_order, sizeof(z_order), "%d", pipe.z_order);
        snprintf(flags, sizeof(flags), "0x%08x", pipe.flags);
        snprintf(decimation, sizeof(decimation), "%3d x %3d", pipe.horizontal_decimation,
                 pipe.vertical_decimation);
        Dataspace &color_metadata = hw_layer.input_buffer.dataspace;
        snprintf(color_primary, sizeof(color_primary), "%d", color_metadata.colorPrimaries);
        snprintf(range, sizeof(range), "%d", color_metadata.range);
        snprintf(transfer, sizeof(transfer), "%d", color_metadata.transfer);

        char row[1024];
        snprintf(row, sizeof(row), format, idx, comp_type, rot ? rot_pipe[count] :
                 pipe_split[count], pipe.pipe_id, input_buffer->width, input_buffer->height,
                 buffer_format, INT(src_roi.left), INT(src_roi.top),
                 INT(src_roi.right), INT(src_roi.bottom), INT(dst_roi.left),
                 INT(dst_roi.top), INT(dst_roi.right), INT(dst_roi.bottom),
                 z_order, flags, decimation, color_primary, range, transfer);

        os << row;
        // print the below only once per layer block, fill with spaces for rest.
        idx[0] = 0;
        comp_type = "";
      }
    }
    os << comp_manager_->Dump(display_comp_ctx_);
    os << newline << "\n";

#ifdef SEC_GC_QC_VSYNC
    os << DumpVsync();
#endif
  }
  return os.str();
}

#ifdef SEC_GC_QC_VSYNC
std::string DisplayBuiltIn::DumpVsync() {
  std::stringstream os;

  bool event_enabled = false;
  bool event_registered = false;
  int64_t event_registered_timestamp = 0;
  int64_t failed_timestamp = 0;
  int err_code = 0;
  if(display_type_ == kBuiltIn && hw_events_intf_ != NULL) {
    hw_events_intf_->GetEventState(&event_enabled, &event_registered, &event_registered_timestamp, &failed_timestamp, &err_code);
    os << "\nCurrent Vsync State: " << vsync_enable_;
    os << "\nCurrent Event State: " << event_enabled << " Registered: " << event_registered;
    os << "\nVsync timestamp: index " << vsync_count_;
    for (uint32_t i = 0; i < kMaxVSyncTimestampLength; i++) {
      os << "\n  " << vsync_timestamp_[i];
      if(i == 0) {
        os << "(+" << (vsync_timestamp_[i]-vsync_timestamp_[kMaxVSyncTimestampLength-1]) << ")";
      } else if(i != vsync_count_) {
        os << "(+" << (vsync_timestamp_[i]-vsync_timestamp_[i-1]) << ")";
      }
    }

    int32_t vsync_last_index = vsync_count_ > 0 ? vsync_count_-1 : kMaxVSyncTimestampLength-1;
    if (!vsync_enable_) {
      os << "\n";
      os << "\nVsync timestamp: Disabled";
      os << "\n  " << vsync_disabled_timestamp_;
      os << "(+" << (vsync_disabled_timestamp_ - vsync_timestamp_[vsync_last_index]) << ")";
    }

    struct timespec ts;
    int64_t current = 0;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) != EINVAL) {
      current = (int64_t)(ts.tv_sec)*1000000000 + (int64_t)(ts.tv_nsec);
    }
    os << "\n";
    os << "\nCurrent	Time: " << current;
    os << "\nRegister Time: " << event_registered_timestamp;
    os << "(" << (current-event_registered_timestamp)/1000000 << "ms, ";
    os << (event_registered_timestamp - vsync_timestamp_[vsync_last_index])/1000	<< "us)";
    os << "\nVsync	Time: " << vsync_current_timestamp_;
    os << "(" << (vsync_current_timestamp_-event_registered_timestamp)/1000 << "us, ";
    int64_t last_timestamp = (vsync_disabled_timestamp_ > vsync_timestamp_[vsync_last_index]) ?
                                    vsync_disabled_timestamp_ : vsync_timestamp_[vsync_last_index];
    os << (vsync_current_timestamp_-last_timestamp)/1000 << "us)";
    os << "\nReg Fail Time: " << failed_timestamp <<"(error code "<< err_code <<")";
    os << "\n";
  }

  return os.str();
}
#endif

DppsInterface* DppsInfo::dpps_intf_ = NULL;
std::vector<int32_t> DppsInfo::display_id_ = {};

void DppsInfo::Init(DppsPropIntf *intf, const std::string &panel_name,
                    DisplayInterface *display_intf, PanelFeaturePropertyIntf *prop_intf) {
  std::lock_guard<std::mutex> guard(lock_);
  int error = 0;
  int disable_dpps_features = 0;

  if (!intf || !display_intf || !prop_intf) {
    DLOGE("Invalid intf %pK display_intf %pK prop_intf %pK", intf, display_intf, prop_intf);
    return;
  }

  DppsDisplayInfo info_payload = {};
  DisplayError ret = intf->DppsProcessOps(kDppsGetDisplayInfo, &info_payload, sizeof(info_payload));
  if (ret != kErrorNone) {
    DLOGE("Get display information failed, ret %d", ret);
    return;
  }

  if (std::find(display_id_.begin(), display_id_.end(), info_payload.display_id)
    != display_id_.end()) {
    return;
  }
  DLOGI("Ready to register display %d-%d ", info_payload.display_id,
        info_payload.display_type);

  Debug::Get()->GetProperty(DISABLE_DPPS_FEATURES, &disable_dpps_features);
  if (disable_dpps_features) {
    if (!dpps_intf_) {
      dpps_intf_ = new DppsDummyImpl();
    }
  }

  if (!dpps_intf_) {
    if (!dpps_impl_lib_.Open(kDppsLib_)) {
      DLOGW("Failed to load Dpps lib %s", kDppsLib_);
      goto exit;
    }

    if (!dpps_impl_lib_.Sym("GetDppsInterface", reinterpret_cast<void **>(&GetDppsInterface))) {
      DLOGE("GetDppsInterface not found!, err %s", dlerror());
      goto exit;
    }

    dpps_intf_ = GetDppsInterface();
    if (!dpps_intf_) {
      DLOGE("Failed to get Dpps Interface!");
      goto exit;
    }
  }
  error = dpps_intf_->Init(intf, panel_name, display_intf, prop_intf);
  if (error) {
    DLOGE("DPPS Interface init failure with err %d", error);
    goto exit;
  }

  display_id_.push_back(info_payload.display_id);
  DLOGI("Registered display %d-%d successfully", info_payload.display_id,
        info_payload.display_type);
  return;

exit:
  Deinit_nolock();
  if (!dpps_intf_) {
    dpps_intf_ = new DppsDummyImpl();
    display_id_.push_back(info_payload.display_id);
  }
}

void DppsInfo::Deinit_nolock() {
  if (dpps_intf_) {
    dpps_intf_->Deinit();
    dpps_intf_ = NULL;
  }
  dpps_impl_lib_.~DynLib();
  DLOGI("Dpps info deinit done");
}

void DppsInfo::Deinit() {
  std::lock_guard<std::mutex> guard(lock_);
  Deinit_nolock();
}

void DppsInfo::DppsNotifyOps(enum DppsNotifyOps op, void *payload, size_t size) {
  int ret = 0;
  if (!dpps_intf_) {
    DLOGW("Dpps intf nullptr");
    return;
  }
  ret = dpps_intf_->DppsNotifyOps(op, payload, size);
  if (ret)
    DLOGE("DppsNotifyOps op %d error %d", op, ret);
}

DisplayError DisplayBuiltIn::GetQSyncMode(QSyncMode *qsync_mode) {
  *qsync_mode = active_qsync_mode_;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetQSyncMode(QSyncMode qsync_mode) {
  ClientLock lock(disp_mutex_);

  if (!client_ctx_.hw_panel_info.qsync_support) {
    DLOGW("Failed: qsync_support: %d", client_ctx_.hw_panel_info.qsync_support);
    return kErrorNotSupported;
  }

  // force clear qsync mode if set by idle timeout.
  if (qsync_mode_ ==  active_qsync_mode_ && qsync_mode_ == qsync_mode) {
    DLOGW("Qsync mode already set as requested mode: qsync_mode_=%d", qsync_mode_);
    return kErrorNone;
  }

  qsync_mode_ = qsync_mode;
  needs_avr_update_.set(kUpdateAVRModeFlag);
  validated_ = false;
  event_handler_->Refresh();
  return kErrorNone;
}

DisplayError DisplayBuiltIn::ControlIdlePowerCollapse(bool enable, bool synchronous) {
  ClientLock lock(disp_mutex_);
  if (!active_) {
    DLOGW("Invalid display state = %d on display %d-%d. Panel must be on.", state_, display_id_,
          display_type_);
    return kErrorPermission;
  }

  if ((client_ctx_.hw_panel_info.mode == kModeCommand) || client_ctx_.hw_panel_info.vhm_support) {
    validated_ = false;
    return dpu_core_mux_->ControlIdlePowerCollapse(enable, synchronous);
  }

  return kErrorNotSupported;
}

DisplayError DisplayBuiltIn::GetSupportedDSIClock(std::vector<uint64_t> *bitclk_rates) {
  ClientLock lock(disp_mutex_);
  if (!client_ctx_.hw_panel_info.dyn_bitclk_support) {
    return kErrorNotSupported;
  }

  *bitclk_rates = client_ctx_.hw_panel_info.bitclk_rates;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetJitterConfig(uint32_t jitter_type, float value, uint32_t time) {
  ClientLock lock(disp_mutex_);
  if (!active_) {
    DLOGW("Invalid display state = %d. Panel must be on.", state_);
    return kErrorNone;
  }

  if (jitter_type > 2 || (value > 10.0f || value < 0.0f)) {
    return kErrorNotSupported;
  }

  DLOGV("Setting jitter configuration; jitter_type: %d, jitter_val: %lf, jitter_time: %d",
        jitter_type, value, time);
  return hw_intf_->SetJitterConfig(jitter_type, value, time);
}

DisplayError DisplayBuiltIn::SetDynamicDSIClock(uint64_t bit_clk_rate) {
  ClientLock lock(disp_mutex_);
  if (!active_) {
    DLOGW("Invalid display state = %d on display %d-%d. Panel must be on.", state_, display_id_,
          display_type_);
    return kErrorNone;
  }

  if (!client_ctx_.hw_panel_info.dyn_bitclk_support) {
    return kErrorNotSupported;
  }

  uint64_t current_clk = 0;
  std::vector<uint64_t> &clk_rates = client_ctx_.hw_panel_info.bitclk_rates;
  GetDynamicDSIClock(&current_clk);
  bool valid = std::find(clk_rates.begin(), clk_rates.end(), bit_clk_rate) != clk_rates.end();
  if (current_clk == bit_clk_rate || !valid) {
    DLOGI("Invalid setting %d, Clk. already set %d", !valid, (current_clk == bit_clk_rate));
    return kErrorNone;
  }

  validated_ = false;
  avoid_qsync_mode_change_ = true;
  DLOGV("Setting new dynamic bit clk value: %" PRIu64, bit_clk_rate);
  return dpu_core_mux_->SetDynamicDSIClock(bit_clk_rate);
}

#ifdef SEC_GC_QC_DYN_CLK
DisplayError DisplayBuiltIn::SetDynamicDSIClockCustom(uint64_t bit_clk_rate) {
	DLOGI("[DynamicClock] Setting new dynamic bit clk value: %" PRIu64, bit_clk_rate);
 	event_handler_->NotifyDynamicDSIClock(bit_clk_rate);	
	return kErrorNone;
}
#endif

DisplayError DisplayBuiltIn::GetDynamicDSIClock(uint64_t *bit_clk_rate) {
  ClientLock lock(disp_mutex_);
  if (!client_ctx_.hw_panel_info.dyn_bitclk_support) {
    return kErrorNotSupported;
  }

  return dpu_core_mux_->GetDynamicDSIClock(bit_clk_rate);
}

DisplayError DisplayBuiltIn::GetRefreshRate(uint32_t *refresh_rate) {
  *refresh_rate = current_refresh_rate_;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetBLScale(uint32_t level) {
  ClientLock lock(disp_mutex_);

  DisplayError err = dpu_core_mux_->SetBLScale(level);
  if (err) {
    DLOGE("Failed to set backlight scale to level %d", level);
  } else {
    DLOGI_IF(kTagDisplay, "Setting backlight scale on display %d-%d to level %d", display_id_,
             display_type_, level);
  }
  return err;
}

bool DisplayBuiltIn::CanCompareFrameROI(LayerStack *layer_stack) {
  // Check Display validation and safe-mode states.
  if (needs_validate_ || comp_manager_->IsSafeMode() || layer_stack->needs_validate) {
    return false;
  }

  // Check Panel and Layer Stack attributes.
  int8_t stack_fudge_factor = 1;  // GPU Target Layer always present in input
  if (layer_stack->flags.stitch_present)
    stack_fudge_factor++;
  if (layer_stack->flags.demura_present)
    stack_fudge_factor++;

  if (!client_ctx_.hw_panel_info.partial_update || (client_ctx_.hw_panel_info.left_roi_count != 1)
      || layer_stack->flags.geometry_changed || layer_stack->flags.skip_present ||
      (layer_stack->layers.size() !=
       (disp_layer_stack_->stack_info.app_layer_count + stack_fudge_factor))) {
    return false;
  }

  // Check for Partial Update disable requests/scenarios.
  if (color_mgr_ && color_mgr_->NeedsPartialUpdateDisable()) {
    DisablePartialUpdateOneFrameInternal();
  }

  if (!partial_update_control_ || disable_pu_one_frame_) {
    return false;
  }

  bool surface_damage = false;
  uint32_t surface_damage_mask_value = (1 << kSurfaceDamage);
  for (uint32_t i = 0; i < layer_stack->layers.size(); i++) {
    Layer *layer = layer_stack->layers.at(i);
    if (layer->update_mask.none()) {
      continue;
    }
    // Only kSurfaceDamage bit should be set in layer's update-mask.
    if (layer->update_mask.to_ulong() == surface_damage_mask_value) {
      surface_damage = true;
    } else {
      return false;
    }
  }

  return surface_damage;
}

bool DisplayBuiltIn::CanSkipDisplayPrepare(LayerStack *layer_stack) {
  if (!CanCompareFrameROI(layer_stack)) {
    return false;
  }

  if (disp_layer_stack_->stack_info.iwe_target_index != -1) {
    return false;
  }

  for (auto& info : disp_layer_stack_->info) {
    info.second.left_frame_roi.clear();
    info.second.right_frame_roi.clear();
    info.second.dest_scale_info_map.clear();
  }
  comp_manager_->GenerateROI(display_comp_ctx_, disp_layer_stack_);

  for (int i = 0; i < core_count_; i++) {
    uint32_t core_id = hw_resource_info_[i].core_id;
    if (!disp_layer_stack_->info.at(core_id).left_frame_roi.size() ||
        !disp_layer_stack_->info.at(core_id).right_frame_roi.size()) {
      return false;
    }

    // Compare the cached and calculated Frame ROIs.
    bool same_roi = IsCongruent(left_frame_roi_[i],
                                disp_layer_stack_->info.at(core_id).left_frame_roi.at(0)) &&
                    IsCongruent(right_frame_roi_[i],
                                disp_layer_stack_->info.at(core_id).right_frame_roi.at(0));

    if (!same_roi) {
      return same_roi;
    }
  }

  for (auto& info : disp_layer_stack_->info) {
    // Update Surface Damage rectangle(s) in HW layers.
    uint32_t hw_layer_count = UINT32(info.second.hw_layers.size());
    for (uint32_t j = 0; j < hw_layer_count; j++) {
      Layer &hw_layer = info.second.hw_layers.at(j);
      Layer *sdm_layer = layer_stack->layers.at(info.second.index.at(j));
      if (hw_layer.dirty_regions.size() != sdm_layer->dirty_regions.size()) {
        return false;
      }
      for (uint32_t k = 0; k < hw_layer.dirty_regions.size(); k++) {
        hw_layer.dirty_regions.at(k) = sdm_layer->dirty_regions.at(k);
      }
    }

    // Set the composition type for SDM layers.
    size_t size_ff = 1;  // GPU Target Layer always present in input
    if (layer_stack->flags.stitch_present)
      size_ff++;
    if (layer_stack->flags.demura_present)
      size_ff++;
    if (disp_layer_stack_->stack_info.common_info.flags.noise_present)
      size_ff++;

    for (uint32_t j = 0; j < (layer_stack->layers.size() - size_ff); j++) {
      layer_stack->layers.at(j)->composition = kCompositionSDE;
    }
  }

  return true;
}

DisplayError DisplayBuiltIn::HandleDemuraLayer(LayerStack *layer_stack) {
  if (!layer_stack) {
    DLOGE("layer_stack is null");
    return kErrorParameters;
  }
  std::vector<Layer *> &layers = layer_stack->layers;
  if (comp_manager_->GetDemuraStatus() && comp_manager_->GetDemuraStatusForDisplay(display_id_) &&
      demura_layer_[0].input_buffer.planes[0].fd > 0) {
    if (disp_layer_stack_->stack_info.demura_target_index == -1) {
      // If demura layer added for first time, do not skip validate
      needs_validate_ = true;
    }

    for (int buf_idx = 0; buf_idx < demura_layer_.size(); buf_idx++) {
      layers.push_back(&demura_layer_.at(buf_idx));
    }

    DLOGI_IF(kTagDisplay, "Demura layer added to layer stack on display %d-%d", display_id_,
             display_type_);
  } else if (disp_layer_stack_->stack_info.demura_target_index != -1) {
    // Demura was present last frame but is now disabled
    needs_validate_ = true;
    disp_layer_stack_->stack_info.demura_present = false;
    DLOGD_IF(kTagDisplay, "Demura layer to be removed on display %d-%d in this frame",
             display_id_, display_type_);
  }
  return kErrorNone;
}

DisplayError DisplayBuiltIn::UpdateTransferTime(uint32_t transfer_time) {
  DisplayError error = kErrorNone;
  {
    ClientLock lock(disp_mutex_);

    if (!active_) {
      DLOGW("Invalid display state = %d. Panel must be on.", state_);
      return kErrorNotSupported;
    }

    if (transfer_time == client_ctx_.hw_panel_info.transfer_time_us) {
      DLOGW("Same transfer time requested. Current = %d, Requested = %d",
            client_ctx_.hw_panel_info.transfer_time_us, transfer_time);
      return kErrorNone;
    } else if (transfer_time > client_ctx_.hw_panel_info.transfer_time_us_max ||
               transfer_time < client_ctx_.hw_panel_info.transfer_time_us_min) {
      DLOGW(
          "Invalid transfer time requested or panel info missing valid range. Min = %d, Max = %d, "
          "Requested = %d, Current = %d",
          client_ctx_.hw_panel_info.transfer_time_us_min,
          client_ctx_.hw_panel_info.transfer_time_us_max, transfer_time,
          client_ctx_.hw_panel_info.transfer_time_us);
      return kErrorParameters;
    }

    error = hw_intf_->UpdateTransferTime(transfer_time);
    if (error != kErrorNone) {
      DLOGW("Retaining the older transfer time.");
      return error;
    }

    DLOGV_IF(kTagDisplay, "Updated transfer time to %d", transfer_time);

    DisplayBase::ReconfigureDisplay();
  }

  event_handler_->Refresh();

  return error;
}

DisplayError DisplayBuiltIn::BuildLayerStackStats(LayerStack *layer_stack) {
  std::vector<Layer *> &layers = layer_stack->layers;
  LayerStackInfo &stack_info = disp_layer_stack_->stack_info;
  stack_info.app_layer_count = 0;
  stack_info.gpu_target_index = -1;
  stack_info.stitch_target_index = -1;
  stack_info.demura_target_index = -1;
  stack_info.noise_layer_index = -1;
  stack_info.cwb_target_index = -1;

  disp_layer_stack_->stack = layer_stack;
  stack_info.common_info.flags = layer_stack->flags;
  stack_info.common_info.blend_cs = layer_stack->blend_cs;
  stack_info.wide_color_primaries.clear();
  stack_info.enable_cac = enable_cac_;
  stack_info.cac_config = cac_config_;

  int index = 0;
  for (auto &layer : layers) {
    if (layer->buffer_map == nullptr) {
      layer->buffer_map = std::make_shared<LayerBufferMap>();
    }
    if (layer->composition == kCompositionGPUTarget) {
      stack_info.gpu_target_index = index;
    } else if (layer->composition == kCompositionStitchTarget) {
      stack_info.stitch_target_index = index;
      disp_layer_stack_->stack->flags.stitch_present = true;
      stack_info.stitch_present = true;
    } else if (layer->composition == kCompositionDemura && stack_info.demura_target_index == -1) {
      stack_info.demura_target_index = index;
      disp_layer_stack_->stack->flags.demura_present = true;
      stack_info.demura_present = true;
      DLOGD_IF(kTagDisplay, "Display %d-%d shall request Demura in this frame", display_id_,
               display_type_);
    } else if (layer->composition == kCompositionDemura) {
      DLOGV_IF(kTagDisplay, "Adding Aiqe ABC feature - UDC layer");
    } else if (layer->flags.is_noise) {
      stack_info.common_info.flags.noise_present = true;
      stack_info.noise_layer_index = index;
      stack_info.noise_layer_info = noise_layer_info_;
      DLOGV_IF(kTagDisplay, "Display %d-%d requested Noise at index = %d with zpos_n = %d",
               display_id_, display_type_, index, noise_layer_info_.zpos_noise);
    } else if (layer->composition == kCompositionCWBTarget) {
      stack_info.cwb_target_index = index;
      stack_info.cwb_present = true;
    } else {
      stack_info.app_layer_count++;
    }
    if (IsWideColor(layer->input_buffer.dataspace.colorPrimaries)) {
      stack_info.wide_color_primaries.push_back(
          layer->input_buffer.dataspace.colorPrimaries);
    }
    if (layer->flags.is_game) {
      stack_info.game_present = true;
    }
#ifdef SEC_GC_CMN_FINGERPRINT_INDISPLAY
    if (layer->input_buffer.flags.fingerprint_indisplay_layer) {
      stack_info.fingerprint_present = true;
    }
#endif
    index++;
  }

  DLOGI_IF(kTagDisplay, "LayerStack layer_count: %zu, app_layer_count: %d "
            "gpu_target_index: %d, stitch_index: %d demura_index: %d cwb_target_index: %d "
            "game_present: %d noise_present: %d display: %d-%d", layers.size(),
            stack_info.app_layer_count, stack_info.gpu_target_index,
            stack_info.stitch_target_index, stack_info.demura_target_index,
            stack_info.cwb_target_index, stack_info.game_present,
            stack_info.common_info.flags.noise_present, display_id_, display_type_);

  if (!stack_info.app_layer_count) {
    DLOGW("Layer count is zero");
    return kErrorNoAppLayers;
  }

  if (stack_info.gpu_target_index > 0) {
    return ValidateGPUTargetParams();
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetActiveConfig(uint32_t index) {
  deferred_config_.MarkDirty();

  if (vrr_enabled_) {
    // Set VRR State
    bool enable = hw_intf_->IsAVRStepSupported(index);
    SetQSyncMode(enable ? kQSyncModeContinuous : kQSyncModeNone);
    SetAVRStepState(enable);
  }

  auto error = DisplayBase::SetActiveConfig(index);
#ifdef SEC_GC_QC_DEBUG
  DLOGI("setting activeConfig(%d) on Display %d-%d. error(%d)", index, display_id_, display_type_, error);
#endif
  shared_ptr<Fence> release_fence = nullptr;
  HWDMSType dms_type = client_ctx_.hw_panel_info.dms_type;
  if (dms_type == sdm::HWDMSType::kDMSVIDNonSeamless) {
    SetDisplayState(kStateOff, 0, &release_fence);
    sleep(1);
    SetDisplayState(kStateOn, 0, &release_fence);
  }
  return error;
}

DisplayError DisplayBuiltIn::ReconfigureDisplay() {
  DisplayError error = kErrorNone;
  HWDisplayAttributes display_attributes;
  HWMixerAttributes mixer_attributes;
  HWPanelInfo hw_panel_info;
  uint32_t active_index = 0;

  DTRACE_SCOPED();

  DisplayClientContext client_ctx = {};
  DisplayDeviceContext device_ctx;

  client_ctx = client_ctx_;
  device_ctx = device_ctx_;

  error = dpu_core_mux_->GetActiveConfig(&active_index);
  if (error != kErrorNone) {
    return error;
  }

  error = dpu_core_mux_->GetDisplayAttributes(active_index, &device_ctx, &client_ctx);
  if (error != kErrorNone) {
    return error;
  }

  error = dpu_core_mux_->GetMixerAttributes(&device_ctx, &client_ctx);
  if (error != kErrorNone) {
    return error;
  }

  error = dpu_core_mux_->GetHWPanelInfo(&device_ctx, &client_ctx);
  if (error != kErrorNone) {
    return error;
  }

  display_attributes = client_ctx.display_attributes;
  mixer_attributes = client_ctx.mixer_attributes;
  hw_panel_info = client_ctx.hw_panel_info;

  const bool dirty = deferred_config_.IsDirty();
  if (deferred_config_.IsDeferredState()) {
    if (dirty) {
      SetDeferredFpsConfig();
    } else {
      // In Deferred state, use current config for comparison.
      GetFpsConfig(&display_attributes, &hw_panel_info);
    }
  }

  const bool display_unchanged = (display_attributes == client_ctx_.display_attributes);
  const bool mixer_unchanged = (mixer_attributes == client_ctx_.mixer_attributes);
  const bool panel_unchanged = (hw_panel_info == client_ctx_.hw_panel_info);
  if (!dirty && display_unchanged && mixer_unchanged && panel_unchanged) {
    return kErrorNone;
  }

  if (CanDeferFpsConfig(display_attributes.fps)) {
    deferred_config_.Init(display_attributes.fps, display_attributes.vsync_period_ns,
                          hw_panel_info.transfer_time_us);

    // Apply current config until new Fps is deferred.
    GetFpsConfig(&display_attributes, &hw_panel_info);
  }

  error = comp_manager_->ReconfigureDisplay(display_comp_ctx_, device_ctx, client_ctx,
                                            &cached_qos_data_);
  if (error != kErrorNone) {
    return error;
  }
  for (auto& qos_data : cached_qos_data_) {
    default_clock_hz_.at(qos_data.first) = qos_data.second.clock_hz;
  }

  // Disable Partial Update for one frame as PU not supported during modeset.
  DisablePartialUpdateOneFrameInternal();

  client_ctx_.display_attributes = display_attributes;
  client_ctx_.mixer_attributes = mixer_attributes;
  client_ctx_.hw_panel_info = hw_panel_info;
  device_ctx_ = device_ctx;

  if (client_ctx_.hw_panel_info.partial_update) {
    // If current panel supports Partial Update, then add a pending PU request
    // to be served in the first PU enable frame after the modeset frame.
    // Because if first PU enable frame, after transition, has a partial Frame-ROI and
    // is followed by Skip Validate frames, then it can benefit those frames.
    pu_pending_ = true;
  }

  if (enable_dpps_dyn_fps_) {
    uint32_t dpps_fps = client_ctx_.display_attributes.fps;
    DppsNotifyPayload dpps_payload = {};
    dpps_payload.is_primary = IsPrimaryDisplayLocked();
    dpps_payload.payload = &dpps_fps;
    dpps_payload.payload_size = sizeof(dpps_fps);
    dpps_info_.DppsNotifyOps(kDppsUpdateFpsEvent, &dpps_payload, sizeof(dpps_payload));
  }

  // Notify Demura when refresh rate changes
  if (demura_) {
    GenericPayload demura_fps_pl = {};
    uint32_t *demura_fps_ptr = nullptr;
    int ret = demura_fps_pl.CreatePayload<uint32_t>(demura_fps_ptr);
    if (ret) {
      DLOGE("Failed to create payload for demura fps, error = %d", ret);
    } else {
      *demura_fps_ptr = client_ctx_.display_attributes.fps;
      ret = demura_->SetParameter(kDemuraFeatureParamRefreshRate, demura_fps_pl);
      if (ret) {
        DLOGE("Failed to set refresh rate for demura, error = %d", ret);
      }
    }
  }
  return kErrorNone;
}

bool DisplayBuiltIn::CanDeferFpsConfig(uint32_t fps) {
  if (deferred_config_.CanApplyDeferredState()) {
    // Deferred Fps Config needs to be applied.
    return false;
  }

  // In case of higher to lower Fps transition on a Builtin display, defer the Fps
  // (Transfer time) configuration, for the number of frames based on frame_count.
  return ((deferred_config_.frame_count != 0) && (client_ctx_.display_attributes.fps > fps));
}

void DisplayBuiltIn::SetDeferredFpsConfig() {
  // Update with the deferred Fps Config.
  client_ctx_.display_attributes.fps = deferred_config_.fps;
  client_ctx_.display_attributes.vsync_period_ns = deferred_config_.vsync_period_ns;
  client_ctx_.hw_panel_info.transfer_time_us = deferred_config_.transfer_time_us;
  for (uint32_t i = 0; i < device_ctx_.size(); i++) {
    device_ctx_[i].display_attributes.fps = deferred_config_.fps;
    device_ctx_[i].display_attributes.vsync_period_ns = deferred_config_.vsync_period_ns;
    device_ctx_[i].hw_panel_info.transfer_time_us = deferred_config_.transfer_time_us;
  }
  deferred_config_.Clear();
}

void DisplayBuiltIn::GetFpsConfig(HWDisplayAttributes *display_attr, HWPanelInfo *panel_info) {
  display_attr->fps = client_ctx_.display_attributes.fps;
  display_attr->vsync_period_ns = client_ctx_.display_attributes.vsync_period_ns;
  panel_info->transfer_time_us = client_ctx_.hw_panel_info.transfer_time_us;
}

PrimariesTransfer DisplayBuiltIn::GetBlendSpaceFromStcColorMode(
    const snapdragoncolor::ColorMode &color_mode) {
  PrimariesTransfer blend_space = {};
  if (!color_mgr_) {
    return blend_space;
  }

  // Set sRGB as default blend space.
  bool native_mode = (color_mode.intent == snapdragoncolor::kNative) ||
                     (color_mode.gamut == ColorPrimaries_Max && color_mode.gamma == Transfer_Max);
  if (stc_color_modes_.list.empty() || (native_mode && allow_tonemap_native_)) {
    return blend_space;
  }

  blend_space.primaries = qti_primaries_map[color_mode.gamut];
  blend_space.transfer = qti_transfer_map[color_mode.gamma];

  return blend_space;
}

DisplayError DisplayBuiltIn::GetConfig(DisplayConfigFixedInfo *fixed_info) {
  ClientLock lock(disp_mutex_);
  fixed_info->is_cmdmode = (client_ctx_.hw_panel_info.mode == kModeCommand);
  bool hdr_supported = true;
  bool has_concurrent_writeback = true;

  for (auto info_intf = hw_info_intf_.Begin(); info_intf != hw_info_intf_.End(); info_intf++) {
    HWResourceInfo hw_resource_info = HWResourceInfo();
    info_intf->second->GetHWResourceInfo(&hw_resource_info);
    hdr_supported &= hw_resource_info.has_hdr;
    uint32_t core_id = hw_resource_info.core_id;
    has_concurrent_writeback &= hw_resource_info.has_concurrent_writeback;
    has_concurrent_writeback &= device_ctx_[core_id].hw_panel_info.hdr_enabled;
  }

  bool hdr_plus_supported = false;
  bool dolby_vision_supported = false;

  // Checking library support for HDR10+
  comp_manager_->GetHDRCapability(&hdr_plus_supported, &dolby_vision_supported);

  fixed_info->hdr_supported = hdr_supported;
  // Built-in displays always support HDR10+ when the target supports HDR
  fixed_info->hdr_plus_supported = fixed_info->hdr_supported && hdr_plus_supported;
  fixed_info->dolby_vision_supported = fixed_info->hdr_supported && dolby_vision_supported;
  // Populate luminance values only if hdr will be supported on that display
  fixed_info->max_luminance = fixed_info->hdr_supported ?
                              client_ctx_.hw_panel_info.peak_luminance: 0;
  fixed_info->average_luminance = fixed_info->hdr_supported ?
                                  client_ctx_.hw_panel_info.average_luminance : 0;
  fixed_info->min_luminance = fixed_info->hdr_supported ?
                              client_ctx_.hw_panel_info.blackness_level: 0;
  fixed_info->hdr_eotf = client_ctx_.hw_panel_info.hdr_eotf;
  fixed_info->hdr_metadata_type_one = client_ctx_.hw_panel_info.hdr_metadata_type_one;
  fixed_info->partial_update = client_ctx_.hw_panel_info.partial_update;
  fixed_info->readback_supported = has_concurrent_writeback;
  fixed_info->supports_unified_draw = unified_draw_supported_;

  return kErrorNone;
}

void DisplayBuiltIn::SendBacklight() {
  DisplayError err = kErrorNone;
  int level = 0;
  if ((err = dpu_core_mux_->GetPanelBrightness(&level)) != kErrorNone) {
    return;
  }
  HandleBacklightEvent(level);
}

void DisplayBuiltIn::SendDisplayConfigs() {
  if (ipc_intf_) {
    GenericPayload in;
    uint32_t active_index = 0;
    IPCDisplayConfigParams *disp_configs = nullptr;
    int ret = in.CreatePayload<IPCDisplayConfigParams>(disp_configs);
    if (ret) {
      DLOGW("failed to create the payload. Error:%d", ret);
      return;
    }
    DisplayError error = dpu_core_mux_->GetActiveConfig(&active_index);
    if (error != kErrorNone) {
      return;
    }
    disp_configs->h_total = client_ctx_.display_attributes.h_total;
    disp_configs->v_total = client_ctx_.display_attributes.v_total;
    disp_configs->fps = client_ctx_.display_attributes.fps;
    disp_configs->smart_panel = client_ctx_.display_attributes.smart_panel;
    disp_configs->is_primary = IsPrimaryDisplayLocked();
    disp_configs->mixer_width = client_ctx_.mixer_attributes.width;
    disp_configs->mixer_height = client_ctx_.mixer_attributes.height;
    if ((ret = ipc_intf_->SetParameter(kIpcParamDisplayConfigs, in))) {
      DLOGW("Failed to send display config, error = %d", ret);
    }
  }
}

int DisplayBuiltIn::SetDemuraIntfStatus(bool enable, int current_idx) {
  int ret = 0;
  bool *reconfig = nullptr;
  GenericPayload reconfig_pl;
  if (!demura_) {
    DLOGE("demura_ is nullptr");
    return -EINVAL;
  }

  if (enable) {
    if ((ret = reconfig_pl.CreatePayload<bool>(reconfig))) {
      DLOGE("Failed to create payload for reconfig, error = %d", ret);
      return ret;
    }

    ret = demura_->GetParameter(kDemuraFeatureParamPendingReconfig, &reconfig_pl);
    if (ret) {
      DLOGE("Failed to get reconfig, error %d", ret);
      return ret;
    }

    if (*reconfig) {
      DLOGI("SetDemuraLayer for Anti-Aging reconfig");
      // TBD: handle for ABC during reconfig verification
      ret = SetupCorrectionLayer();
      if (ret) {
        DLOGE("Failed to setup Demura layer, error %d", ret);
        return ret;
      }
    }
  }

  GenericPayload config_pl;
  if (abc_prop_) {
    DemuraFeatureParamConfigIdx<std::string> *config_mode_name = nullptr;
    if ((ret = config_pl.CreatePayload(config_mode_name))) {
      DLOGE("Failed to create payload for config_mode_name, error = %d", ret);
      return ret;
    }

    config_mode_name->modeinfo = "";
    if ((ret = demura_->SetParameter(kDemuraFeatureParamConfigIdx, config_pl))) {
      DLOGE("Failed to set Config Idx, error = %d", ret);
      return ret;
    }

    if (SetupCorrectionLayer() != kErrorNone) {
      DLOGE("Unable to setup abc_intf layer on Display %d-%d", display_id_, display_type_);
      return kErrorUndefined;
    }

  } else {
    uConfigIdx *config_idx = nullptr;
    if ((ret = config_pl.CreatePayload<uConfigIdx>(config_idx))) {
      DLOGE("Failed to create payload for config_idx, error = %d", ret);
      return ret;
    }

    config_idx->modeinfo = current_idx;
    if ((ret = demura_->SetParameter(kDemuraFeatureParamConfigIdx, config_pl))) {
      DLOGE("Failed to set Config Idx, error = %d", ret);
      return ret;
    }
  }

  GenericPayload pl;
  bool* enable_ptr = nullptr;
  if ((ret = pl.CreatePayload<bool>(enable_ptr))) {
    DLOGE("Failed to create payload for enable, error = %d", ret);
    return ret;
  } else {
    *enable_ptr = enable;
    if ((ret = demura_->SetParameter(kDemuraFeatureParamActive, pl))) {
      DLOGE("Failed to set Active, error = %d", ret);
      return ret;
    }
  }

  if (enable && reconfig && (*reconfig)) {
    *reconfig = false;
    ret = demura_->SetParameter(kDemuraFeatureParamPendingReconfig, reconfig_pl);
    if (ret) {
      DLOGE("Failed to set reconfig, error %d", ret);
      return ret;
    }
  }
  DLOGI("Demura is now %s and current index is %d ", enable ? "Enabled" : "Disabled", current_idx);
  return ret;
}

DisplayError DisplayBuiltIn::SetDppsFeatureLocked(void *payload, size_t size) {
  return dpu_core_mux_->SetDppsFeature(payload, size);
}

void DisplayBuiltIn::HandlePowerEvent() {
  return ProcessPowerEvent();
}

void DisplayBuiltIn::HandleVmReleaseEvent() {
  if (event_handler_)
    event_handler_->HandleEvent(kVmReleaseDone);
}

DisplayError DisplayBuiltIn::GetQsyncFps(uint32_t *qsync_fps) {
  ClientLock lock(disp_mutex_);
  return dpu_core_mux_->GetQsyncFps(qsync_fps);
}

DisplayError DisplayBuiltIn::SetAlternateDisplayConfig(uint32_t *alt_config) {
  ClientLock lock(disp_mutex_);
  if (!alt_config) {
    return kErrorResources;
  }
  DisplayError error = dpu_core_mux_->SetAlternateDisplayConfig(alt_config);

  if (error == kErrorNone) {
    ReconfigureDisplay();
    validated_ = false;
  }

  return error;
}

// LCOV_EXCL_START
DisplayError DisplayBuiltIn::HandleSecureEvent(SecureEvent secure_event, bool *needs_refresh) {
  DisplayError error = kErrorNone;

  error = DisplayBase::HandleSecureEvent(secure_event, needs_refresh);
  if (error) {
    if (error == kErrorPermission) {
      DLOGW("Failed to handle secure event %d", secure_event);
    } else {
      DLOGE("Failed to handle secure event %d", secure_event);
    }
    return error;
  }

  if (secure_event == kTUITransitionEnd) {
    comp_manager_->SetDemuraStatusForDisplay(display_id_, true);
    // enable demura after TUI transition end
    if (demura_) {
      SetDemuraIntfStatus(true, demura_current_idx_);
    }
  }

  return error;
}

DisplayError DisplayBuiltIn::PostHandleSecureEvent(SecureEvent secure_event) {
  ClientLock lock(disp_mutex_);
  if (secure_event == kTUITransitionStart) {
    if (!pending_brightness_) {
      if (secure_event == kTUITransitionStart) {
        // Send the panel brightness event to secondary VM on TUI session start
        SendBacklight();
      }
    }
    if (secure_event == kTUITransitionStart) {
      // Send display config information to secondary VM on TUI session start
      SendDisplayConfigs();
    }

    if (secure_event == kTUITransitionStart) {
      comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
      //  disable demura before TUI transition start
      if (demura_) {
        SetDemuraIntfStatus(false);
      }
    }
  }
  if (secure_event == kTUITransitionEnd) {
    comp_manager_->PostHandleSecureEvent(display_comp_ctx_, secure_event);
  }
  return kErrorNone;
}

DisplayIPCVmCallbackImpl::DisplayIPCVmCallbackImpl(BufferAllocator *buffer_allocator,
                                                       std::shared_ptr<IPCIntf> ipc_intf,
                                                       uint64_t panel_id, uint32_t width,
                                                       uint32_t height)
  : buffer_allocator_(buffer_allocator),  ipc_intf_(ipc_intf), panel_id_(panel_id),
    hfc_buffer_width_(width), hfc_buffer_height_(height) {}

void DisplayIPCVmCallbackImpl::Init() {
  if (!ipc_intf_) {
    DLOGW("IPC interface is NULL");
    return;
  }
  GenericPayload in_reg;
  DisplayIPCVmCallbackImpl **cb_intf = nullptr;
  int ret = in_reg.CreatePayload<DisplayIPCVmCallbackImpl *>(cb_intf);
  if (ret) {
    DLOGE("failed to create the payload for in_reg. Error:%d", ret);
    return;
  }
  *cb_intf = this;
  GenericPayload out_reg;
  ret = out_reg.CreatePayload<int>(cb_hnd_out_);
  if (ret) {
    DLOGE("failed to create the payload for out_reg. Error:%d", ret);
    return;
  }
  if ((ret = ipc_intf_->ProcessOps(kIpcOpsRegisterVmCallback, in_reg, &out_reg))) {
    DLOGE("Failed to register vm callback, error = %d", ret);
    return;
  }
}
void DisplayIPCVmCallbackImpl::Deinit() {
  if (!ipc_intf_) {
    DLOGW("IPC interface is NULL");
    return;
  }
  GenericPayload in_unreg;
  int *cb_hnd_in = nullptr;
  int ret = in_unreg.CreatePayload<int>(cb_hnd_in);
  if (ret) {
    DLOGE("failed to create the payload for in_unreg. Error:%d", ret);
    return;
  }
  *cb_hnd_in = *cb_hnd_out_;
  if ((ret = ipc_intf_->ProcessOps(kIpcOpsUnRegisterVmCallback, in_unreg, nullptr))) {
    DLOGE("Failed to unregister vm callback, error = %d", ret);
    return;
  }
}
void DisplayIPCVmCallbackImpl::OnServerReady() {
  lock_guard<recursive_mutex> obj(cb_mutex_);
  server_ready_ = true;
}

void DisplayIPCVmCallbackImpl::ExportHFCBuffer() {
  lock_guard<recursive_mutex> obj(cb_mutex_);
  if (!server_ready_) {
    DLOGW("Server not ready, Failed to export HFC buffers");
    return;
  }

  if (!ipc_intf_ || !buffer_allocator_) {
    DLOGE("Invalid parameters ipc_intf_ %p, buffer_allocator_ %p", ipc_intf_.get(),
          buffer_allocator_);
    return;
  }

  buffer_info_hfc_.buffer_config.width = hfc_buffer_width_;
  buffer_info_hfc_.buffer_config.height = hfc_buffer_height_;
  buffer_info_hfc_.buffer_config.format = kFormatBGRA8888;
  buffer_info_hfc_.buffer_config.buffer_count = 1;
  std::bitset<kBufferPermMax> buf_perm;
  buf_perm.set(kBufferPermRead);
  buf_perm.set(kBufferPermWrite);
  buffer_info_hfc_.buffer_config.access_control.insert(
      std::make_pair(kBufferClientUnTrustedVM, buf_perm));
  buffer_info_hfc_.buffer_config.access_control.insert(
      std::make_pair(kBufferClientTrustedVM, buf_perm));

  int ret = buffer_allocator_->AllocateBuffer(&buffer_info_hfc_);
  if (ret != 0) {
    DLOGE("Fail to allocate hfc buffer");
    return;
  }

  GenericPayload in;
  IPCBufferInfo *export_buf_in_params = nullptr;
  ret = in.CreatePayload<IPCBufferInfo>(export_buf_in_params);
  if (ret) {
    DLOGE("failed to create IPCExportBufInParams payload. Error:%d", ret);
    buffer_allocator_->FreeBuffer(&buffer_info_hfc_);
    return;
  }

  export_buf_in_params->size = buffer_info_hfc_.alloc_buffer_info.size;
  export_buf_in_params->panel_id = panel_id_;
  export_buf_in_params->mem_handle = buffer_info_hfc_.alloc_buffer_info.mem_handle;

  DLOGI("Allocated hfc buffer mem_handle %d size %d panel id :%x", export_buf_in_params->mem_handle,
        export_buf_in_params->size, export_buf_in_params->panel_id);
  if ((ret = ipc_intf_->SetParameter(kIpcParamSetHFCBuffer, in))) {
    DLOGE("Failed to export demura buffers, error = %d", ret);
    buffer_allocator_->FreeBuffer(&buffer_info_hfc_);
    return;
  }
}

void DisplayIPCVmCallbackImpl::FreeExportBuffer() {
  lock_guard<recursive_mutex> obj(cb_mutex_);
  buffer_allocator_->FreeBuffer(&buffer_info_hfc_);
  DLOGI("Free hfc export buffer and fd");
}

void DisplayIPCVmCallbackImpl::OnServerExit() {
  lock_guard<recursive_mutex> obj(cb_mutex_);
  server_ready_ = false;
}
// LCOV_EXCL_STOP

CacVersion DisplayBuiltIn::GetCacVerion() {
  int cac_version = 0;
  for (int i = 0; i < core_count_; i++) {
    cac_version |= hw_resource_info_[i].cac_version;
  }

  return static_cast<CacVersion>(cac_version);
}

DisplayError DisplayBuiltIn::AllocateDummyLoopbackCACBuffer() {
  if (GetCacVerion() != kCacVersionLoopback) {
    return kErrorNone;
  }

  dummy_loopback_cac_info_.buffer_config.width = client_ctx_.display_attributes.x_pixels;
  dummy_loopback_cac_info_.buffer_config.height = client_ctx_.display_attributes.y_pixels;

  dummy_loopback_cac_info_.buffer_config.format = kFormatRGBA8888Ubwc;
  dummy_loopback_cac_info_.buffer_config.buffer_count = 1;
  if (buffer_allocator_->AllocateBuffer(&dummy_loopback_cac_info_) != 0) {
    DLOGE("Loopback CAC Buffer allocation failed");
    return kErrorMemory;
  }

  buffer_allocator_->FreeBuffer(&dummy_loopback_cac_info_);
  dummy_loopback_cac_info_.alloc_buffer_info.fd = -1;

  return kErrorNone;
}

void DisplayBuiltIn::InitCWBBuffer() {
  if (client_ctx_.hw_panel_info.mode != kModeVideo || !HasConcurrentWriteback()
      || !client_ctx_.hw_panel_info.is_primary_panel) {
    return;
  }

  if (disable_cwb_idle_fallback_ || cwb_buffer_initialized_) {
    return;
  }

  // Initialize CWB buffer with display resolution to get full size buffer
  // as mixer or fb can init with custom values based on property
  output_buffer_info_.buffer_config.width = client_ctx_.display_attributes.x_pixels;
  output_buffer_info_.buffer_config.height = client_ctx_.display_attributes.y_pixels;

  output_buffer_info_.buffer_config.format = kFormatRGBX8888Ubwc;
  output_buffer_info_.buffer_config.buffer_count = 1;
  if (buffer_allocator_->AllocateBuffer(&output_buffer_info_) != 0) {
    DLOGE("Buffer allocation failed");
    return;
  }

  LayerBuffer buffer = {};
  buffer.planes[0].fd = output_buffer_info_.alloc_buffer_info.fd;
  buffer.planes[0].offset = 0;
  buffer.planes[0].stride = output_buffer_info_.alloc_buffer_info.stride;
  buffer.size = output_buffer_info_.alloc_buffer_info.size;
  buffer.handle_id = output_buffer_info_.alloc_buffer_info.id;
  buffer.width = output_buffer_info_.alloc_buffer_info.aligned_width;
  buffer.height = output_buffer_info_.alloc_buffer_info.aligned_height;
  buffer.format = output_buffer_info_.alloc_buffer_info.format;
  buffer.unaligned_width = output_buffer_info_.buffer_config.width;
  buffer.unaligned_height = output_buffer_info_.buffer_config.height;

  cwb_layer_.composition = kCompositionCWBTarget;
  cwb_layer_.input_buffer = buffer;
  cwb_layer_.input_buffer.buffer_id = reinterpret_cast<uint64_t>(output_buffer_info_.private_data);
  cwb_layer_.src_rect = {0, 0, FLOAT(cwb_layer_.input_buffer.unaligned_width),
                         FLOAT(cwb_layer_.input_buffer.unaligned_height)};
  cwb_layer_.dst_rect = {0, 0, FLOAT(cwb_layer_.input_buffer.unaligned_width),
                         FLOAT(cwb_layer_.input_buffer.unaligned_height)};

  cwb_layer_.flags.is_cwb = 1;
  cwb_buffer_initialized_ = true;
  return;
}

void DisplayBuiltIn::DeinitCWBBuffer() {
  if (!cwb_buffer_initialized_) {
    return;
  }

  buffer_allocator_->FreeBuffer(&output_buffer_info_);
  cwb_layer_ = {};
  cwb_buffer_initialized_ = false;
}

void DisplayBuiltIn::AppendCWBLayer(LayerStack *layer_stack) {
  if (cwb_buffer_initialized_ &&
      (cwb_layer_.input_buffer.unaligned_width < client_ctx_.display_attributes.x_pixels ||
       cwb_layer_.input_buffer.unaligned_height < client_ctx_.display_attributes.y_pixels)) {
    DLOGI("Resetting CWB layer due to insufficient buffer size(%dx%d) compare to output(%dx%d).",
          cwb_layer_.input_buffer.unaligned_width, cwb_layer_.input_buffer.unaligned_height,
          client_ctx_.display_attributes.x_pixels, client_ctx_.display_attributes.y_pixels);
    DeinitCWBBuffer();
  }

  if (!cwb_buffer_initialized_) {
    // If CWB buffer is not initialized, then it must be initialized for video mode
    InitCWBBuffer();
  }

  if (!client_ctx_.hw_panel_info.is_primary_panel || disable_cwb_idle_fallback_ ||
      !cwb_buffer_initialized_) {
    return;
  }

  uint32_t new_mixer_width = client_ctx_.fb_config.x_pixels;
  uint32_t new_mixer_height = client_ctx_.fb_config.y_pixels;
  NeedsMixerReconfiguration(layer_stack, &new_mixer_width, &new_mixer_height);
  // Set cwb src_rect same as mixer resolution since LM tappoint
  // and dest_rect equal to fb resolution as strategy scales HWLayer dest rect based on fb
  cwb_layer_.src_rect = {0, 0, FLOAT(new_mixer_width), FLOAT(new_mixer_height)};
  cwb_layer_.dst_rect = {0, 0, FLOAT(client_ctx_.fb_config.x_pixels),
                         FLOAT(client_ctx_.fb_config.y_pixels)};
  cwb_layer_.composition = kCompositionCWBTarget;
  layer_stack->layers.push_back(&cwb_layer_);
}

uint32_t DisplayBuiltIn::GetUpdatingAppLayersCount(LayerStack *layer_stack) {
  uint32_t updating_count = 0;

  for (uint i = 0; i < layer_stack->layers.size(); i++) {
    auto layer = layer_stack->layers.at(i);
    if (layer->composition == kCompositionGPUTarget) {
      break;
    }
    if (layer->flags.updating) {
      updating_count++;
    }
  }

  return updating_count;
}

DisplayError DisplayBuiltIn::ChangeFps() {
  ClientLock lock(disp_mutex_);

  if (!active_ || !client_ctx_.hw_panel_info.dynamic_fps || qsync_mode_ != kQSyncModeNone ||
      disable_dyn_fps_) {
    return kErrorNotSupported;
  }

  uint32_t num_updating_layers = GetUpdatingLayersCount();
  bool one_updating_layer = (num_updating_layers == 1);
  uint32_t refresh_rate = GetOptimalRefreshRate(one_updating_layer);

  if (refresh_rate < client_ctx_.hw_panel_info.min_fps ||
      refresh_rate > client_ctx_.hw_panel_info.max_fps) {
    DLOGE("Invalid Fps = %d request", refresh_rate);
    return kErrorParameters;
  }

  bool idle_screen = GetUpdatingAppLayersCount(disp_layer_stack_->stack) == 0;
  if (!disp_layer_stack_->stack->force_refresh_rate && IdleFallbackLowerFps(idle_screen) &&
      !enable_qsync_idle_) {
    refresh_rate = client_ctx_.hw_panel_info.min_fps;
  }

  if (current_refresh_rate_ != refresh_rate) {
    DisplayError error = dpu_core_mux_->SetRefreshRate(refresh_rate);
    if (error != kErrorNone) {
      // Attempt to update refresh rate can fail if rf interference settings is detected.
      // Just drop min fps settting for now.
      if (disp_layer_stack_->stack_info.lower_fps) {
        disp_layer_stack_->stack_info.lower_fps = false;
      }
      return error;
    }

    error = comp_manager_->CheckEnforceSplit(display_comp_ctx_, refresh_rate);
    if (error != kErrorNone) {
      return error;
    }
  }

  // Set safe mode upon success.
  if (enhance_idle_time_ && (refresh_rate == client_ctx_.hw_panel_info.min_fps) &&
      (disp_layer_stack_->stack_info.lower_fps)) {
    comp_manager_->ProcessIdleTimeout(display_comp_ctx_);
  }

  // On success, set current refresh rate to new refresh rate
  current_refresh_rate_ = refresh_rate;
  deferred_config_.MarkDirty();

  return ReconfigureDisplay();
}

bool DisplayBuiltIn::IdleFallbackLowerFps(bool idle_screen) {
  if (!enhance_idle_time_) {
    return (disp_layer_stack_->stack_info.lower_fps);
  }
  if (!idle_screen || !disp_layer_stack_->stack_info.lower_fps) {
    return false;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  uint64_t elapsed_time_ms = GetTimeInMs(now) - GetTimeInMs(idle_timer_start_);
  bool can_lower = elapsed_time_ms >= UINT32(idle_time_ms_);
  DLOGV_IF(kTagDisplay, "display %d-%d , lower fps: %d", display_id_, display_type_, can_lower);

  return can_lower;
}

uint32_t DisplayBuiltIn::GetUpdatingLayersCount() {
  uint32_t updating_count = 0;

  for (uint i = 0; i < disp_layer_stack_->stack->layers.size(); i++) {
    auto layer = disp_layer_stack_->stack->layers.at(i);
    if (layer->flags.updating) {
      updating_count++;
    }
  }
  return updating_count;
}

uint32_t DisplayBuiltIn::GetOptimalRefreshRate(bool one_updating_layer) {
  LayerStack *layer_stack = disp_layer_stack_->stack;
  if (layer_stack->force_refresh_rate) {
    return layer_stack->force_refresh_rate;
  }

  uint32_t metadata_refresh_rate = CalculateMetaDataRefreshRate();
  if (layer_stack->flags.use_metadata_refresh_rate && one_updating_layer &&
      metadata_refresh_rate) {
    return metadata_refresh_rate;
  }

  return active_refresh_rate_;
}

uint32_t DisplayBuiltIn::CalculateMetaDataRefreshRate() {
  LayerStack *layer_stack = disp_layer_stack_->stack;
  uint32_t metadata_refresh_rate = 0;
  if (!layer_stack->flags.use_metadata_refresh_rate) {
    return 0;
  }

  uint32_t max_refresh_rate = 0;
  uint32_t min_refresh_rate = 0;
  GetRefreshRateRange(&min_refresh_rate, &max_refresh_rate);

  for (uint i = 0; i < layer_stack->layers.size(); i++) {
    auto layer = layer_stack->layers.at(i);
    if (layer->flags.has_metadata_refresh_rate && layer->frame_rate > metadata_refresh_rate) {
      metadata_refresh_rate = SanitizeRefreshRate(layer->frame_rate, max_refresh_rate,
                                                  min_refresh_rate);
    }
  }
  return metadata_refresh_rate;
}

uint32_t DisplayBuiltIn::SanitizeRefreshRate(uint32_t req_refresh_rate, uint32_t max_refresh_rate,
                                             uint32_t min_refresh_rate) {
  uint32_t refresh_rate = req_refresh_rate;

  if (refresh_rate < min_refresh_rate) {
    // Pick the next multiple of request which is within the range
    refresh_rate = (((min_refresh_rate / refresh_rate) +
                     ((min_refresh_rate % refresh_rate) ? 1 : 0)) * refresh_rate);
  }

  if (refresh_rate > max_refresh_rate) {
    refresh_rate = max_refresh_rate;
  }

  return refresh_rate;
}

DisplayError DisplayBuiltIn::SetDemuraState(int state) {
  int ret = 0;

  if (!demura_intended_) {
    DLOGW("Demura has not enabled");
    return kErrorNone;
  }

  if (state && !comp_manager_->GetDemuraStatusForDisplay(display_id_)) {
    if (SetupCorrectionLayer() != kErrorNone) {
      DLOGE("Unable to setup Demura layer on Display %d", display_id_);
      return kErrorUndefined;
    }

    ret = SetDemuraIntfStatus(true);
    if (ret) {
      DLOGE("Failed to set demura status to true, ret = %d", ret);
      return kErrorUndefined;
    }
    comp_manager_->SetDemuraStatusForDisplay(display_id_, true);
    demura_dynamic_enabled_ = true;
    demura_current_idx_ = kDemuraDefaultIdx;
  } else if (!state && comp_manager_->GetDemuraStatusForDisplay(display_id_)) {
    ret = SetDemuraIntfStatus(false);
    if (ret) {
      DLOGE("Failed to set demura status to false, ret = %d", ret);
      return kErrorUndefined;
    }
    comp_manager_->SetDemuraStatusForDisplay(display_id_, false);
    demura_dynamic_enabled_ = false;
    demura_layer_.clear();
  }

  // Disable Partial Update for one frame.
  DisablePartialUpdateOneFrameInternal();

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetDemuraConfig(int demura_idx) {
  int ret = 0;
  GenericPayload pl;
  uConfigIdx *idx = nullptr;

  if (!demura_intended_ || !demura_dynamic_enabled_) {
    DLOGW("Demura is not enabled");
    return kErrorNone;
  }

  DLOGI("Setting the Demura Config, config = %d", demura_idx);

  if (demura_idx < kDemuraDefaultIdx || demura_idx >= kMaxPanelConfigSupported) {
    DLOGE("Invalid demura config index");
    return kErrorParameters;
  }

  if (demura_idx == demura_current_idx_) {
    return kErrorNone;
  }

  // Update demura config
  if ((ret = pl.CreatePayload<uConfigIdx>(idx))) {
    DLOGE("Failed to create payload for enable, error = %d", ret);
    return kErrorUndefined;
  }

  idx->modeinfo = demura_idx;
  if ((ret = demura_->SetParameter(kDemuraFeatureParamConfigIdx, pl))) {
    DLOGE("Failed to update demura config, error = %d", ret);
    return kErrorUndefined;
  }

  if (SetupCorrectionLayer() != kErrorNone) {
    DLOGE("Unable to setup Demura layer on Display %d", display_id_);
    return kErrorUndefined;
  }

  if (SetDemuraIntfStatus(true, demura_idx)) {
    DLOGE("Failed to set Demura Status on Display %d", display_id_);
    return kErrorUndefined;
  }

  demura_current_idx_ = demura_idx;
  DLOGV("Demura config updated to config index %d", demura_idx);
  event_handler_->Refresh();

  // disable partial update for one frame
  DisablePartialUpdateOneFrame();

  return kErrorNone;
}

void DisplayBuiltIn::GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) {
  dpu_core_mux_->GetDRMDisplayToken(token);
}

bool DisplayBuiltIn::IsPrimaryDisplay() {
  return DisplayBase::IsPrimaryDisplay();
}

DisplayError DisplayBuiltIn::GetPanelBrightnessBasePath(std::string *base_path) {
  return dpu_core_mux_->GetPanelBrightnessBasePath(base_path);
}

bool DisplayBuiltIn::IsCacV2Supported() {
  for (auto &res_info : hw_resource_info_) {
    if ((res_info.cac_version != kCacVersion2) && (res_info.cac_version != kCacVersionLoopback)) {
      return false;
    }
  }

  return true;
}

DisplayError DisplayBuiltIn::PerformCacConfig(CacConfig config, bool enable) {
  ClientLock lock(disp_mutex_);

  if (!IsCacV2Supported()) {
    return kErrorNotSupported;
  }

  DLOGV_IF(kTagDisplay, "CAC enable: %d Config:: k0r: %f k1r: %f k0b: %f k1b: %f pixel_pitch: %f"
           "normalization: %f mid_le_y_offset: %d mid_le_x_offset: %d mid_re_y_offset: %d"
           " mid_re_x_offset: %d skip_inc: %d", enable, config.k0r, config.k1r, config.k0b,
           config.k1b, config.pixel_pitch, config.normalization, config.mid_le_y_offset,
           config.mid_le_x_offset, config.mid_re_y_offset, config.mid_re_x_offset, config.skip_inc);

  enable_cac_ = enable;
  cac_config_ = config;
  validated_ = false;
  event_handler_->Refresh();

  return kErrorNone;
}

DisplayError
DisplayBuiltIn::PanelOprInfo(const std::string &client_name, bool enable,
                             SdmDisplayCbInterface<PanelOprPayload> *cb_intf) {
  return event_proxy_info_.PanelOprInfo(client_name, enable, cb_intf);
}

DisplayError DisplayBuiltIn::SetPaHistCollection(
    const std::string &client_name, bool enable,
    SdmDisplayCbInterface<PaHistCollectionPayload> *cb_intf) {
  return event_proxy_info_.SetPaHistCollection(client_name, enable, cb_intf);
}

DisplayError DisplayBuiltIn::GetPaHistBins(std::array<uint32_t, HIST_BIN_SIZE> *buf) {
  return event_proxy_info_.GetPaHistBins(buf);
}

DisplayError DisplayBuiltIn::PanelBacklightInfo(
    const std::string &client_name, bool enable,
    SdmDisplayCbInterface<PanelBacklightPayload> *cb_intf) {
  return event_proxy_info_.PanelBacklightInfo(client_name, enable, cb_intf);
}

DisplayError DisplayBuiltIn::EnableCopr(bool en) {
  DisplayError ret = kErrorNone;

  DLOGI("%s COPR", en ? "Enable" : "Disable");
  ret = event_proxy_info_.EnableCopr("copr_test", en, &copr_info_);
  if (ret) {
    DLOGW("Failed to enable COPR ret %d", ret);
  } else {
    event_handler_->Refresh();
    copr_enabled_ = en;
  }

  return ret;
}

DisplayError DisplayBuiltIn::GetCoprStats(std::vector<int> *stats) {
  DisplayError ret = kErrorNone;

  if (!copr_enabled_) {
    DLOGW("COPR is not enabled");
    return kErrorResources;
  }

  ret = copr_info_.GetStats(stats);
  if (ret)
    DLOGE("Failed to get COPR stats ret %d", ret);
  return ret;
}

DisplayError EventProxyInfo::Init(const std::string &panel_name, DisplayInterface *intf,
                                  DynLib &extension_lib, PanelFeaturePropertyIntf *prop_intf) {
  std::lock_guard<std::mutex> guard(lock_);

  if (!intf || !prop_intf) {
    DLOGE("Invalid display_intf %pK prop_intf %pK", intf, prop_intf);
    return kErrorParameters;
  }

  if (event_proxy_intf_.get()) {
    DLOGV("Event proxy interface is already created");
    return kErrorNone;
  }

  typedef DispEventProxyFactIntf *(*GetDispEventProxyFactFunc)();
  GetDispEventProxyFactFunc get_disp_event_proxy_fact_func;

  if (!extension_lib.Sym(
          "GetDispEventProxyFactIntf",
          reinterpret_cast<void **>(&get_disp_event_proxy_fact_func))) {
    DLOGW("Fail to retrieve GetDispEventProxyFactIntf from %s",
          EXTENSION_LIBRARY_NAME);
    return kErrorUndefined;
  }

  DispEventProxyFactIntf *factory_intf = get_disp_event_proxy_fact_func();
  if (!factory_intf) {
    DLOGW("Failed to get display event proxy factory interface");
    return kErrorUndefined;
  }

  std::shared_ptr<DisplayEventProxyIntf> proxy_intf =
      factory_intf->CreateDispEventProxyIntf(panel_name, intf, prop_intf);
  if (!proxy_intf) {
    DLOGW("Failed to create display event proxy interface");
    return kErrorMemory;
  }

  int ret = proxy_intf->Init();
  if (ret) {
    DLOGW("Failed to initialize event proxy interface, ret %d", ret);
    return kErrorUndefined;
  }

  event_proxy_intf_ = proxy_intf;
  return kErrorNone;
}

DisplayError EventProxyInfo::Deinit() {
  std::lock_guard<std::mutex> guard(lock_);
  if (event_proxy_intf_) {
    event_proxy_intf_->Deinit();
    event_proxy_intf_.reset();
    event_proxy_intf_ = nullptr;
  }
  return kErrorNone;
}

DisplayError
EventProxyInfo::PanelOprInfo(const std::string &client_name, bool enable,
                             SdmDisplayCbInterface<PanelOprPayload> *cb_intf) {
  if (!event_proxy_intf_.get()) {
    DLOGW("Event proxy intf is not available");
    return kErrorParameters;
  }

  PanelOprInfoParam *opr_info = nullptr;
  GenericPayload payload;
  int ret = payload.CreatePayload(opr_info);
  if (ret || !opr_info) {
    DLOGE("Failed to create payload for OPR info, ret %d", ret);
    return kErrorParameters;
  }

  opr_info->name = client_name;
  opr_info->enable = enable;
  opr_info->cb_intf = cb_intf;

  ret = event_proxy_intf_->SetParameter(kSetPanelOprInfoEnable, payload);
  if (ret) {
    DLOGE("Failed to set panel Opr info enablement, ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError EventProxyInfo::EnableCopr(const std::string &client_name, bool enable,
                                        SdmDisplayCbInterface<CoprEventPayload> *cb_intf) {
  if (!event_proxy_intf_.get()) {
    DLOGW("Event proxy intf is not available");
    return kErrorParameters;
  }

  CoprParam *copr_info = nullptr;
  GenericPayload payload;
  int ret = payload.CreatePayload(copr_info);
  if (ret || !copr_info) {
    DLOGE("Failed to create payload for COPR info, ret %d", ret);
    return kErrorParameters;
  }

  copr_info->name = client_name;
  copr_info->enable = enable;
  copr_info->cb_intf = cb_intf;

  ret = event_proxy_intf_->SetParameter(kSetCoprEnable, payload);
  if (ret) {
    DLOGE("Failed to set Copr info enablement, ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError CoprInfo::GetStats(std::vector<int32_t> *stats) {
  std::lock_guard<std::mutex> guard(lock_);

  if (!stats) {
    DLOGE("Invalid input parameter stats %pK", stats);
    return kErrorUndefined;
  }

  *stats = copr_stats_;
  return kErrorNone;
}

int CoprInfo::Notify(const CoprEventPayload &payload) {
  std::lock_guard<std::mutex> guard(lock_);
  struct drm_msm_copr_status *copr_info =
      reinterpret_cast<struct drm_msm_copr_status *>(payload.payload);

  copr_stats_.clear();
  for (auto i = 0; i < AIQE_COPR_STATUS_LEN; i++) {
    copr_stats_.push_back(copr_info->status[i]);
  }

  return 0;
}

DisplayError EventProxyInfo::SetPaHistCollection(
    const std::string &client_name, bool enable,
    SdmDisplayCbInterface<PaHistCollectionPayload> *cb_intf) {
  if (!event_proxy_intf_.get()) {
    DLOGW("Event proxy intf is not available");
    return kErrorParameters;
  }

  PaHistCollectionParam *param = nullptr;
  GenericPayload payload;
  int ret = payload.CreatePayload(param);
  if (ret || !param) {
    DLOGE("Failed to create payload for pa hist, ret %d", ret);
    return kErrorParameters;
  }

  param->name = client_name;
  param->enable = enable;
  param->cb_intf = cb_intf;

  ret = event_proxy_intf_->SetParameter(kSetPaHistCollection, payload);
  if (ret) {
    DLOGE("Failed to set pa hist enablement, ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError EventProxyInfo::GetPaHistBins(std::array<uint32_t, HIST_BIN_SIZE> *buf) {
  PaHistBinsParam *param = nullptr;
  GenericPayload payload;

  if (!event_proxy_intf_.get()) {
    DLOGW("Event proxy intf is not available");
    return kErrorParameters;
  }

  if (!buf) {
    DLOGE("Invalid pa hist bins buf");
    return kErrorParameters;
  }

  int ret = payload.CreatePayload(param);
  if (ret || !param) {
    DLOGE("Failed to create payload for pa hist bins, ret %d", ret);
    return kErrorParameters;
  }

  param->buf = buf;
  ret = event_proxy_intf_->GetParameter(kGetPaHistBins, &payload);
  if (ret) {
    DLOGE("Failed to get pa hist bins, ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError EventProxyInfo::PanelBacklightInfo(
    const std::string &client_name, bool enable,
    SdmDisplayCbInterface<PanelBacklightPayload> *cb_intf) {
  if (!event_proxy_intf_.get()) {
    DLOGW("Event proxy intf is not available");
    return kErrorParameters;
  }

  PanelBacklightInfoParam *backlight_info = nullptr;
  GenericPayload payload;
  int ret = payload.CreatePayload(backlight_info);
  if (ret || !backlight_info) {
    DLOGE("Failed to create payload for backlight info, ret %d", ret);
    return kErrorParameters;
  }

  backlight_info->name = client_name;
  backlight_info->enable = enable;
  backlight_info->cb_intf = cb_intf;

  ret = event_proxy_intf_->SetParameter(kSetPanelBLInfoEnable, payload);
  if (ret) {
    DLOGE("Failed to set panel backlight info enablement, ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetSsrcMode(const std::string &mode) {
  DisplayError ret = kErrorNotSupported;

  if (ssrc_feature_enabled_ && ssrc_feature_interface_) {
    std::string *mode_str;
    GenericPayload payload;
    int rc = payload.CreatePayload(mode_str);
    if (rc) {
      DLOGE("Unable to create mode string payload! RC - %d", rc);
    } else {
      *mode_str = mode;
      rc = ssrc_feature_interface_->SetParameter(aiqe::kSsrcFeatureModeId, payload);
      if (rc) {
        DLOGE("Mode rejected by SSRC feature interface. RC - %d", rc);
      } else {
        ret = kErrorNone;
      }
    }
  }
  needs_validate_ = true;
  return ret;
}

DisplayError DisplayBuiltIn::SetAVRStepState(bool enable) {
  ClientLock lock(disp_mutex_);

  if (enable && (client_ctx_.hw_panel_info.mode == kModeVideo)) {
    if (!client_ctx_.hw_panel_info.qsync_support || (qsync_mode_ == kQSyncModeNone)) {
      DLOGW("AVR Step feature is not supported without QSync on VID mode");
      return kErrorNotSupported;
    }
  }

  if (avr_step_enabled_ == enable) {
    DLOGI("AVR Step already set in requested state %d", enable);
    return kErrorNone;
  }

  avr_step_enabled_ = enable;
  needs_avr_update_.set(kUpdateAVRStepFlag);
  validated_ = false;
  event_handler_->Refresh();
  DLOGI("AVR Step state set to %d successfully", avr_step_enabled_);

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetVRRState(bool state) {
  if (!hw_intf_->IsVRRSupported()) {
    return kErrorNotSupported;
  }

  uint32_t active_index = 0;
  dpu_core_mux_->GetActiveConfig(&active_index);
  if (hw_intf_->IsAVRStepSupported(active_index)) {
    DLOGI("Set VRR state %d in config %d", state, active_index);
    SetQSyncMode(state ? kQSyncModeContinuous : kQSyncModeNone);
    DisplayError error = SetAVRStepState(state);
    if (error != kErrorNone) {
      return error;
    }
  }

  vrr_enabled_ = state;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetABCState(bool state) {
  DLOGV("Setting the ABC State to %d", state);

  int ret = 0;
  bool current_abc_state = comp_manager_->GetDemuraStatusForDisplay(display_id_);

  // Check if current ABC state and coming ABC state is same
  if (state == current_abc_state) {
    DLOGI("ABC Feature state already set to %s", state ? "true" : "false");
    return kErrorNone;
  }

  // If current ABC state is disable and coming ABC state is enable
  if (state && !current_abc_state) {
    if (demura_->Init() != 0) {
      DLOGE("Unable to initialize ABC on Display %d-%d", display_id_, display_type_);
      return kErrorUndefined;
    }
  }

  // Enable or Disable ABC
  if (SetDemuraIntfStatus(state)) {
    DLOGE("Failed to set demura status to %s on Display %d, ret = %d", ret,
          state ? "true" : "false", display_id_);
    return kErrorUndefined;
  }

  // Update dispay abc state for current display
  comp_manager_->SetDemuraStatusForDisplay(display_id_, state);
  abc_enabled_ = state;
  if (abc_enabled_ && (SetABCMode("normal_on_udc_off") != kErrorNone)) {
    DLOGE("Failed to set mode to normal_on_udc_off");
    return kErrorUndefined;
  }

  needs_validate_ = true;
  // Disable Partial Update for one frame.
  DisablePartialUpdateOneFrameInternal();
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetABCReconfig() {
  if (!comp_manager_->GetDemuraStatusForDisplay(display_id_)) {
    return kErrorUndefined;
  }

  int ret = 0;
  GenericPayload pl;
  bool *b = nullptr;
  ret = pl.CreatePayload<bool>(b);
  if (ret) {
    DLOGE("Failed to create kDemuraFeatureParamPendingReconfig payload");
    return kErrorUndefined;
  }

  // Setting reconfig as true
  *b = true;
  ret = demura_->SetParameter(kDemuraFeatureParamPendingReconfig, pl);
  if (ret) {
    DLOGE("Failed to set reconfig parameter for ABC %d", ret);
    return kErrorUndefined;
  }

  if (SetDemuraIntfStatus(true)) {
    DLOGE("Failed to set ABC Status on Display %d", display_id_);
    return kErrorUndefined;
  }
  needs_validate_ = true;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetABCMode(const string &mode_name) {
  if (mode_name.empty()) {
    DLOGI("mode name is empty");
    return kErrorUndefined;
  }

  int ret = 0;
  GenericPayload config_pl;

  DemuraFeatureParamConfigIdx<std::string> *config_mode_name = nullptr;
  if ((ret = config_pl.CreatePayload(config_mode_name))) {
    DLOGE("Failed to create payload for config_mode_name, error = %d", ret);
    return kErrorUndefined;
  }

  // Setting the mode name
  config_mode_name->modeinfo = mode_name;
  if ((ret = demura_->SetParameter(kDemuraFeatureParamConfigIdx, config_pl))) {
    DLOGE("Failed to set Config Idx, error = %d", ret);
    return kErrorUndefined;
  }

  // Set up ABC correction layer for updated mode name
  if (SetupCorrectionLayer() != kErrorNone) {
    DLOGE("Unable to setup ABC layer on Display %d", display_id_);
    return kErrorUndefined;
  }

  // Set the ABC feature with updated mode name
  GenericPayload pl;
  bool *enable_ptr = nullptr;
  if ((ret = pl.CreatePayload<bool>(enable_ptr))) {
    DLOGE("Failed to create payload for enable, error = %d", ret);
    return kErrorUndefined;
  } else {
    *enable_ptr = true;
    if ((ret = demura_->SetParameter(kDemuraFeatureParamActive, pl))) {
      DLOGE("Failed to set Active, error = %d", ret);
      return kErrorUndefined;
    }
  }
  needs_validate_ = true;
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetPanelFeatureConfig(int32_t type, void *data) {
  DisplayError ret = kErrorNone;

  switch (type) {
    case kTypeDemuraTnCWBSamplingPeriod:
      ret = SetDemuraTnCWBSamplingPeriod(data);
      break;
    case kTypeDemuraTnEventsCtrl:
      ret = SetDemuraTnEventsCtrl(data);
      break;
    case kTypeDemuraTnUserCtrl:
      ret = SetDemuraTnUserCtrl(data);
      break;
    case kTypeDeleteDemuraConfig:
      ret = CleanupDemuraConfig(data, kDeleteDemuraConfig);
      break;
    case kTypeDeleteDemuraTnConfig:
      ret = CleanupDemuraConfig(data, kDeleteDemuraTnConfig);
      break;
    case kTypeTriggerDemuraOemPlugIn:
      ret = TriggerDemuraOemPlugIn(data);
      break;
    default:
      DLOGE("Invalid type %d", type);
      ret = kErrorParameters;
      break;
  }
  return ret;
}

DisplayError DisplayBuiltIn::SetDemuraTnCWBSamplingPeriod(void *data) {
  int ret = 0;
  int *period_ptr = nullptr;
  GenericPayload payload = {};

  if (!data || !demuratn_ || !demuratn_enabled_) {
    DLOGE("Data %pK demuratn_ %pK demuratn_enabled_ %d", data, demuratn_.get(), demuratn_enabled_);
    return kErrorUndefined;
  }

  ret = payload.CreatePayload<int>(period_ptr);
  if (ret) {
    DLOGE("Failed to create the payload, ret %d", ret);
    return kErrorUndefined;
  }
  *period_ptr = *(reinterpret_cast<int *>(data));

  ret = demuratn_->SetParameter(kDemuraTnCoreUvmParamCWBSamplingPeriod, payload);
  if (ret) {
    DLOGE("Set CWB sampling period failed ret %d", ret);
    return kErrorUndefined;
  }

  DLOGI("Set CWB sampling period %d success", *period_ptr);

  return kErrorNone;
}

DisplayError DisplayBuiltIn::ExportDemuraFiles() {
  if (!pf_factory_) {
    DLOGW("Invalid panel feature factory");
    return kErrorUndefined;
  }

  std::shared_ptr<DemuraParserManagerIntf> pm_intf =
      pf_factory_->CreateDemuraParserManager(ipc_intf_, buffer_allocator_);
  if (!pm_intf) {
    DLOGE("Failed to get Parser Manager intf");
    return kErrorResources;
  }

  if (pm_intf->Init() != 0) {
    DLOGE("Failed to init Parser Manager intf");
    return kErrorResources;
  }

  GenericPayload in;
  int ret = pm_intf->SetParameter(kDemuraParserManagerExportDemuraFiles, in);
  if (ret) {
    DLOGE("Failed to export demura files, ret %d", ret);
    return kErrorUndefined;
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::StartTvmServices() {
  int enable_demura = 0, enable_anti_aging = 0;

  Debug::Get()->GetProperty(ENABLE_DEMURA, &enable_demura);
  Debug::Get()->GetProperty(ENABLE_ANTI_AGING, &enable_anti_aging);

  if (!abc_prop_ && !enable_demura && !enable_anti_aging) {
    return kErrorNone;
  }

#ifndef SDM_UNIT_TESTING
  sleep(5);  // sleep 5 seconds to make sure persist is mounted on TVM
#endif

  DisplayError error = StartService(kStartVmFileTransferService);
  if (error) {
    DLOGE("Failed to start file transfer service, error %d", error);
    return error;
  } else {
    DLOGI("VmFiletransfer service is started successfully");
  }

  if (demuratn_factory_) {
    demuratn_cleanup_intf_ = demuratn_factory_->CreateDemuraTnCleanupIntf(buffer_allocator_);
    if (!demuratn_cleanup_intf_) {
      DLOGW("Failed to create DemuraTnCleanupIntf");
    } else {
      int ret = demuratn_cleanup_intf_->Init();
      if (ret) {
        DLOGE("Failed to init DemuraTnCleanupIntf, ret %d", ret);
        demuratn_cleanup_intf_.reset();
      }
    }
  }

  if (enable_demura) {
    error = ExportDemuraFiles();
    if (error) {
      DLOGE("Failed to export demura files, error %d", error);
      return error;
    }
  }

  if (enable_anti_aging) {
    error = StartService(kStartDemuraTnService);
    if (error) {
      DLOGE("Failed to start DemuraTn service, error %d", error);
    } else {
      DLOGI("DemuraTn service is started successfully");
    }
  }
  return error;
}

DisplayError DisplayBuiltIn::StartService(TvmDispServiceManagerParams service) {
  if (service_manager_intf_ == nullptr) {
    if (pf_factory_ == nullptr) {
      DLOGE("Invalid panel feature factory");
      return kErrorUndefined;
    }

    service_manager_intf_ = pf_factory_->CreateTvmServiceManager();
    if (!service_manager_intf_) {
      DLOGE("Failed to get Tvm Service Manager intf");
      return kErrorResources;
    }

    if (service_manager_intf_->Init() != 0) {
      DLOGE("Failed to init Tvm Service Manager intf");
      service_manager_intf_.reset();
      service_manager_intf_ = nullptr;
      return kErrorResources;
    }
  }

  GenericPayload in;
  int ret = service_manager_intf_->SetParameter(service, in);
  if (ret) {
    DLOGE("Failed to set parameter %d, ret %d", service, ret);
    return kErrorUndefined;
  } else {
    DLOGI("Start service %d", service);
  }

  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetDemuraTnEventsCtrl(void *data) {
  int ret = 0;
  bool *ctrl_ptr = nullptr;
  GenericPayload payload = {};

  if (!data || !demuratn_ || !demuratn_enabled_) {
    DLOGE("Data %pK demuratn_ %pK demuratn_enabled_ %d", data, demuratn_.get(), demuratn_enabled_);
    return kErrorUndefined;
  }

  ret = payload.CreatePayload<bool>(ctrl_ptr);
  if (ret) {
    DLOGE("Failed to create the payload, ret %d", ret);
    return kErrorUndefined;
  }
  *ctrl_ptr = *(reinterpret_cast<bool *>(data));

  ret = demuratn_->SetParameter(kDemuraTnCoreUvmParamEventsCtrl, payload);
  if (ret) {
    DLOGE("Set events ctrl failed ret %d", ret);
    return kErrorUndefined;
  }

  DLOGI("Set events ctrl %d success", *ctrl_ptr);
  return kErrorNone;
}

DisplayError DisplayBuiltIn::SetDemuraTnUserCtrl(void *data) {
  DisplayError ret = kErrorNone;
  if (!data) {
    DLOGE("Invalid parameters");
    return kErrorParameters;
  }

  bool user_ctrl = *(reinterpret_cast<uint32_t *>(data)) ? true : false;
  if (!user_ctrl) {
    ret = EnableDemuraTn(user_ctrl);
    if (ret != kErrorNone) {
      return ret;
    }
  }
  demuratn_permanent_disabled_ = user_ctrl;

  int error = UpdateDemuraTnUserCtrl(user_ctrl);
  if (error) {
    DLOGE("Failed to update demura user ctrl, error = %d", error);
    return kErrorUndefined;
  }

  return ret;
}

DisplayError DisplayBuiltIn::CleanupDemuraConfig(void *data, DemuraTnCleanupType type) {
  int ret = 0;
  if (!data) {
    DLOGE("Invalid parameters");
    return kErrorParameters;
  }
  if (!demuratn_cleanup_intf_) {
    DLOGW("Demuratn cleanup intf is not created");
    return kErrorNotSupported;
  }

  uint64_t panel_id = *(reinterpret_cast<uint64_t *>(data));

  DemuraTnCleanupDeleteConfigInput *input = nullptr;
  GenericPayload payload;
  ret = payload.CreatePayload(input);
  if (ret || !input) {
    DLOGE("Failed to create payload for DeleteConfigInput, ret %d", ret);
    return kErrorParameters;
  }

  input->panel_id = panel_id;
  if (type == kDeleteDemuraConfig) {
    input->delete_t0 = true;
  }
  if (type == kDeleteDemuraTnConfig) {
    input->delete_tn = true;
  }

  ret = demuratn_cleanup_intf_->SetParameter(kDemuraTnCleanupDeleteConfig, payload);
  if (ret) {
    DLOGE("Failed to SetParameter, param %d, ret %d", kDemuraTnCleanupDeleteConfig, ret);
    return kErrorParameters;
  }

  return kErrorNone;
}

bool DisplayBuiltIn::GetDemuraTnUserCtrl() {
  std::ifstream in(kDemuraTnUserCtrlFile, std::ios::binary);
  if (!in.is_open()) {
    return false;
  }

  in.seekg(0, in.end);
  int length = in.tellg();
  in.seekg(0, in.beg);
  std::string file_data(length, '\0');
  in.read(&file_data[0], length);

  if (static_cast<int>(file_data.size()) != length) {
    DLOGE("Couldn't read the whole file");
    return false;
  }

  auto pos = file_data.find("demuratn_permanent_disabled=true");
  if (pos != std::string::npos) {
    return true;
  }

  return false;
}

int DisplayBuiltIn::UpdateDemuraTnUserCtrl(bool user_ctrl) {
  int ret = 0;
  std::ofstream out(kDemuraTnUserCtrlFile, std::ios::binary | std::ios::trunc);

  if (out.fail()) {
    DLOGW("Failed to open the file %s %s", kDemuraTnUserCtrlFile.c_str(), strerror(errno));
    return -ENOENT;
  }

  std::string value = user_ctrl ? "true" : "false";
  std::string prefix = "demuratn_permanent_disabled=";
  out << prefix << value << '\n';
  return ret;
}

DisplayError DisplayBuiltIn::TriggerDemuraOemPlugIn(void *data) {
  (void)data;
  int ret = 0;
  int current_brightness_level = 0;
  struct DemuraBacklightInfo *demura_bl_info = nullptr;
  GenericPayload payload = {};

  if (!demura_intended_ || !demura_dynamic_enabled_) {
    DLOGW("Demura is not enabled");
    return kErrorNone;
  }

  ret = payload.CreatePayload<struct DemuraBacklightInfo>(demura_bl_info);
  if (ret) {
    DLOGE("Failed to create payload");
    return kErrorUndefined;
  }

  // Get current brightness level
  DisplayError error = dpu_core_mux_->GetPanelBrightness(&current_brightness_level);
  if (error != kErrorNone) {
    DLOGE("Failed to get current brightness level, error %d", error);
    return error;
  }

  // Fill demura backlight info
  demura_bl_info->os_brightness = current_brightness_level;
  demura_bl_info->os_brightness_max = client_ctx_.hw_panel_info.panel_max_brightness;

  // Call demura backlight event for trigger oem plugin
  ret = demura_->SetParameter(kDemuraFeatureParamBacklightEvent, payload);
  if (ret) {
    DLOGE("Failed to set backlight event");
    return kErrorUndefined;
  }

  // Trigger refresh, new config take effect
  event_handler_->Refresh();

  DLOGI("Trigger demura oem plugin success");
  return kErrorNone;
}

}  // namespace sdm
