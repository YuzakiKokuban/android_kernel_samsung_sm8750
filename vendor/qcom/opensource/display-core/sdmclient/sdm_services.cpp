/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
#include <utils/constants.h>

#include <algorithm>
#include <vector>

#include "libsync.h"
#include "sdm_debugger.h"
#include "sdm_display_intf_parcel.h"
#include "sdm_services.h"

#define __CLASS__ "SDMServices"

namespace sdm {

static const uint32_t kBrightnessScaleMax = 100;
static const uint32_t kSvBlScaleMax = 65535;
static const int kSolidFillDelay = 100 * 1000;

void SDMServices::Init(SDMDisplayBuilder *disp,
                       BufferAllocator *buffer_allocator, Locker *locker,
                       SDMTrustedUI *tui) {
  disp_ = disp;
  locker_ = locker;
  tui_ = tui;

  color_mgr_ = SDMColorManager::CreateColorManager(buffer_allocator, socket_handler_);
  if (!color_mgr_) {
    DLOGW("Failed to load SDMColorManager.");
  }

  // Default is int. Support extensions, such as uint64_t and float
  for (int i = 0; i < PanelFeatureVendorServiceTypeMax; i++) {
    panel_feature_data_type_map_[static_cast<PanelFeatureVendorServiceType>(i)] = "int";
  }

  panel_feature_data_type_map_[kTypeDeleteDemuraConfig] = "uint64_t";
  panel_feature_data_type_map_[kTypeDeleteDemuraTnConfig] = "uint64_t";
}

void SDMServices::Deinit() {
  bw_mode_release_fd_ = -1;

  if (color_mgr_) {
    color_mgr_->DestroyColorManager();
  }
}

DisplayError SDMServices::notifyCallback(uint32_t command, SDMParcel *input_parcel,
                                         SDMParcel *output_parcel) {
  DLOGI("sdmclient qservice command %d", command);

  if (!input_parcel) {
    DLOGW("Service command %d invalid input parcel!", command);
    return kErrorNone;
  }

  if (vnd_handlers_set_.find(command) != vnd_handlers_set_.end()) {
    VndCmdSetHandler vnd_set_fn = vnd_handlers_set_[command];
    return ((this->*vnd_set_fn)(input_parcel));
  }

  if (!output_parcel) {
    DLOGW("Service command %d invalid output parcel!", command);
    return kErrorNone;
  }

  if (vnd_handlers_get_.find(command) != vnd_handlers_get_.end()) {
    VndCmdGetHandler vnd_get_fn_ = vnd_handlers_get_[command];
    return ((this->*vnd_get_fn_)(input_parcel, output_parcel));
  }

  DLOGW("Service command %d is not supported.", command);
  return kErrorNone;
}

DisplayError SDMServices::DynamicDebug(int type, bool enable,
                                       int verbose_level) {
  DLOGI("type = %d enable = %d", type, enable);

  switch (type) {
  case SDM_SERVICE_DEBUG_ALL:
    SDMDebugHandler::DebugAll(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_MDPCOMP:
    SDMDebugHandler::DebugStrategy(enable, verbose_level);
    SDMDebugHandler::DebugCompManager(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_PIPE_LIFECYCLE:
    SDMDebugHandler::DebugResources(enable, verbose_level);
    SDMDebugHandler::DebugQos(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_DRIVER_CONFIG:
    SDMDebugHandler::DebugDriverConfig(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_ROTATOR:
    SDMDebugHandler::DebugResources(enable, verbose_level);
    SDMDebugHandler::DebugDriverConfig(enable, verbose_level);
    SDMDebugHandler::DebugRotator(enable, verbose_level);
    SDMDebugHandler::DebugQos(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_QDCM:
    SDMDebugHandler::DebugQdcm(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_SCALAR:
    SDMDebugHandler::DebugScalar(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_CLIENT:
    SDMDebugHandler::DebugClient(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_DISPLAY:
    SDMDebugHandler::DebugDisplay(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_IWE:
    SDMDebugHandler::DebugIWE(enable, verbose_level);
    break;

  case SDM_SERVICE_DEBUG_WB_USAGE:
    SDMDebugHandler::DebugWbUsage(enable, verbose_level);
    break;

  default:
    DLOGW("type = %d is not supported", type);
  }

  return kErrorNone;
}

// This will make more sense once it is fully merged - right now there is still
// some functionality in sdm_session which will be moved over to sdm package
// with this class
DisplayError SDMServices::RefreshScreen(int idx) {
  cb_->Refresh(UINT64(idx));
  return kErrorNone;
}

DisplayError SDMServices::SetIdleTimeout(int value) {
  SDMDisplay *display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);
  if (!display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  int inactive_ms = IDLE_TIMEOUT_INACTIVE_MS;
  Debug::Get()->GetProperty(IDLE_TIME_INACTIVE_PROP, &inactive_ms);
  display->SetIdleTimeoutMs(value, inactive_ms);
  tui_->SetIdleTimeoutMs(value, inactive_ms);

  return kErrorNone;
}

DisplayError SDMServices::SetFrameDumpConfig(
    uint32_t frame_dump_count, std::bitset<32> bit_mask_display_type,
    uint32_t bit_mask_layer_type, int32_t processable_cwb_requests,
    int32_t output_format, CwbConfig cwb_config) {
  DisplayError status = kErrorNone;
  bool input_buffer_dump = bit_mask_layer_type & (1 << INPUT_LAYER_DUMP);
  for (uint32_t i = 0; i < bit_mask_display_type.size(); i++) {
    if (!bit_mask_display_type[i]) {
      continue;
    }
    int disp_idx = disp_->GetDisplayIndex(INT(i));
    if (disp_idx == -1) {
      continue;
    }

    if (i != UINT32(qdutilsDisplayType::DISPLAY_VIRTUAL) &&
        i != UINT32(qdutilsDisplayType::DISPLAY_VIRTUAL_2)) {
      if (processable_cwb_requests <= 0 && !input_buffer_dump) {
        continue;
      } else if (processable_cwb_requests > 0) {
        processable_cwb_requests--;
      }
    }

    SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_idx]);
    auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
    if (!sdm_display) {
      DLOGW("Display = %d is not connected.", disp_idx);
      status = (status) ? status
                        : kErrorNotSupported; // Return higher priority error.
      continue;
    }

    status = sdm_display->SetFrameDumpConfig(
        frame_dump_count, bit_mask_layer_type, output_format, cwb_config);
  }

  return status;
}

DisplayError
SDMServices::SetMaxMixerStages(std::bitset<32> bit_mask_display_type,
                               int max_mixer_stages) {
  DisplayError error = kErrorNone;
  DisplayError status = kErrorNone;
  for (uint32_t i = 0; i < 32 && bit_mask_display_type[i]; i++) {
    int disp_idx = disp_->GetDisplayIndex(INT(i));
    if (disp_idx == -1) {
      continue;
    }

    SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_idx]);
    auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
    if (!sdm_display) {
      DLOGW("Display = %d is not connected.", disp_idx);
      status = (status) ? status
                        : kErrorNotSupported; // Return higher priority error.
      continue;
    }

    error = sdm_display->SetMaxMixerStages(max_mixer_stages);
    if (error != kErrorNone) {
      status = kErrorNotSupported;
    }
  }

  return status;
}

DisplayError SDMServices::SetDisplayMode(int mode) {
  SEQUENCE_WAIT_SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);
  auto display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);
  if (!display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  return display->Perform(SET_DISPLAY_MODE, mode);
}

DisplayError SDMServices::SetDisplayStatus(int disp_id,
                                           SDMDisplayStatus disp_status) {
  auto display = cb_->GetDisplayFromClientId(disp_id);
  return display->SetDisplayStatus(disp_status);
}

DisplayError SDMServices::ConfigureRefreshRate(uint32_t operation,
                                               uint32_t refresh_rate) {
  SEQUENCE_WAIT_SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);

  SDMDisplay *sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);

  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  switch (operation) {
    case MetadataOps::DISABLE_METADATA_DYN_REFRESH_RATE:
      return sdm_display->Perform(SET_METADATA_DYN_REFRESH_RATE, false);

    case MetadataOps::ENABLE_METADATA_DYN_REFRESH_RATE:
      return sdm_display->Perform(SET_METADATA_DYN_REFRESH_RATE, true);

    case MetadataOps::SET_BINDER_DYNAMIC_REFRESH_RATE: {
      if (refresh_rate == 0) {
        DLOGE("Invalid refresh rate requested: %d", refresh_rate);
        return kErrorNotSupported;
      }
      return sdm_display->Perform(MetadataOps::SET_BINDER_DYNAMIC_REFRESH_RATE, refresh_rate);
    }

  default:
    DLOGW("Invalid operation %d", operation);
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::ToggleScreenUpdate(int disp_id, bool on) {
  auto sdm_display = cb_->GetDisplayFromClientId(disp_id);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_id);
    return kErrorNotSupported;
  }

  DisplayError error = sdm_display->ToggleScreenUpdates(on);
  if (error) {
    DLOGE("Failed to toggle screen updates = %d. Display = %" PRIu64
          ", Error = %d",
          on, disp_id, error);
  }

  return error;
}

DisplayError
SDMServices::MinHdcpEncryptionLevelChanged(int disp_id,
                                           uint32_t min_enc_level) {
  // SSG team hardcoded disp_id as external because it applies to external only
  // but SSG team sends this level irrespective of external connected or not. So
  // to honor the call, make disp_id to primary & set level.
  disp_id = SDM_DISPLAY_PRIMARY;
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  SDMDisplay *sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGE("Display = %d is not connected.", disp_idx);
    return kErrorNotSupported;
  }

  return sdm_display->OnMinHdcpEncryptionLevelChange(min_enc_level);
}

DisplayError SDMServices::ControlPartialUpdate(int disp_id, bool enable) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  if (disp_id != qdutilsDisplayType::DISPLAY_PRIMARY &&
      disp_id != qdutilsDisplayType::DISPLAY_BUILTIN_2) {
    DLOGW("CONTROL_PARTIAL_UPDATE is not applicable for display = %d", disp_id);
    return kErrorNotSupported;
  }
  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_idx]);
    SDMDisplay *sdm_display = cb_->GetDisplayFromClientId(disp_idx);
    if (!sdm_display) {
      DLOGE("Display = %d object is not instantiated", disp_idx);
      return kErrorNotSupported;
    }

    uint32_t pending = 0;
    DisplayError sdm_error =
        sdm_display->ControlPartialUpdate(enable, &pending);
    if (sdm_error == kErrorNone) {
      if (!pending) {
        return kErrorNone;
      }
    } else if (sdm_error == kErrorNotSupported) {
      return kErrorNone;
    } else {
      return kErrorNotSupported;
    }
  }

  return kErrorNone;
}

DisplayError SDMServices::SetNoisePlugInOverride(int32_t disp_id,
                                                 bool override_en, int32_t attn,
                                                 int32_t noise_zpos) {
  int32_t disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  return sdm_display->SetNoisePlugInOverride(override_en, attn, noise_zpos);
}

DisplayError SDMServices::SetActiveConfigIndex(int disp_id, uint32_t config) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);

  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorNotSupported;
  }

  auto error = sdm_display->SetActiveDisplayConfig(config);
  if (error == kErrorNone) {
    cb_->Refresh(disp_idx);
  }

  return error;
}

DisplayError SDMServices::GetActiveConfigIndex(int disp_id, uint32_t *config) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);

  if (sdm_display) {
    return sdm_display->GetActiveDisplayConfig(false, config);
  }

  return kErrorNotSupported;
}

DisplayError SDMServices::GetConfigCount(int disp_id, uint32_t *count) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);

  if (sdm_display) {
    return sdm_display->GetDisplayConfigCount(count);
  }

  return kErrorNotSupported;
}

DisplayError SDMServices::GetDisplayAttributesForConfig(
    int disp_id, int config, DisplayConfigVariableInfo *var_info) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1 || config < 0) {
    DLOGE("Invalid display = %d, or config = %d", disp_id, config);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (sdm_display) {
    return sdm_display->GetDisplayAttributesForConfig(config, var_info);
  }

  return kErrorNotSupported;
}

DisplayError
SDMServices::GetDisplayMaxBrightness(uint32_t display,
                                     uint32_t *max_brightness_level) {
  if (!max_brightness_level) {
    return kErrorParameters;
  }

  auto error = kErrorNotSupported;
  auto sdm_display = cb_->GetDisplayFromClientId(display);

  if (sdm_display && sdm_display->GetDisplayClass() == DISPLAY_CLASS_BUILTIN) {
    error = sdm_display->GetPanelMaxBrightness(max_brightness_level);
    if (error) {
      DLOGE("Failed to get the panel max brightness, display %u error %d",
            display, error);
    }
  }

  return error;
}

DisplayError SDMServices::GetDisplayBrightnessPercent(uint32_t display,
                                                      float *brightness) {
  if (!brightness) {
    return kErrorParameters;
  }

  auto error = kErrorNotSupported;
  *brightness = -1.0f;

  auto sdm_display = cb_->GetDisplayFromClientId(display);
  if (sdm_display && sdm_display->GetDisplayClass() == DISPLAY_CLASS_BUILTIN) {
    error = sdm_display->GetPanelBrightness(brightness);
    if (error) {
      DLOGE("Failed to get the panel brightness. Error = %d", error);
    }
  }

  return error;
}

DisplayError SDMServices::GetVisibleDisplayRect(int disp_id,
                                                SDMRect *visible_rect) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_idx);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    return kErrorHardware;
  }

  return sdm_display->GetVisibleDisplayRect(visible_rect);
}

DisplayError SDMServices::SetCameraLaunchStatus(int camera_status) {
  auto core_intf = cb_->GetCoreIntf();

  if (!core_intf) {
    DLOGW("core_intf_ not initialized.");
    return kErrorNotSupported;
  }

  HWBwModes mode = camera_status > 0 ? kBwVFEOn : kBwVFEOff;
  if (core_intf->SetMaxBandwidthMode(mode) != kErrorNone) {
    return kErrorNotSupported;
  }

  cb_->Refresh(0);

  return kErrorNone;
}

DisplayError SDMServices::DisplayBWTransactionPending(bool *state) {
  auto sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  if (sync_wait(bw_mode_release_fd_, 0) < 0) {
    DLOGI("bw_transaction_release_fd is not yet signaled: err= %s",
          strerror(errno));
    *state = false;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetMixerResolution(int dpy, uint32_t width,
                                             uint32_t height) {
  DisplayError error = kErrorNone;

  if (dpy != SDM_DISPLAY_PRIMARY) {
    DLOGW("Resolution change not supported for this display = %d", dpy);
    return kErrorNotSupported;
  }

  SEQUENCE_WAIT_SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);
  auto sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);
  if (!sdm_display) {
    DLOGW("Primary display is not initialized");
    return kErrorHardware;
  }

  error = sdm_display->SetMixerResolution(width, height);
  if (error != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetColorModeOverride(int dpy, SDMColorMode mode) {
  int disp_idx = disp_->GetDisplayIndex(dpy);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", dpy);
    return kErrorNotSupported;
  }

  if (mode < SDMColorMode::COLOR_MODE_NATIVE ||
      mode > SDMColorMode::COLOR_MODE_DISPLAY_BT2020) {
    DLOGE("Invalid SDMColorMode: %d", mode);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  auto err = sdm_display->SetColorMode(mode);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetColorModeWithRenderIntentOverride(
    int disp_idx, SDMColorMode mode, int int_intent) {
  if (mode < SDMColorMode::COLOR_MODE_NATIVE ||
      mode > SDMColorMode::COLOR_MODE_DISPLAY_BT2020) {
    DLOGE("Invalid SDMColorMode: %d", mode);
    return kErrorNotSupported;
  }

  if ((int_intent < 0) || (int_intent > MAX_EXTENDED_RENDER_INTENT)) {
    DLOGE("Invalid SDMRenderIntent: %d", int_intent);
    return kErrorNotSupported;
  }

  auto intent = static_cast<SDMRenderIntent>(int_intent);
  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  auto err = sdm_display->SetColorModeWithRenderIntent(mode, intent);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetColorModeById(int dpy, int mode) {
  int disp_idx = disp_->GetDisplayIndex(dpy);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", dpy);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  auto err = sdm_display->SetColorModeById(mode);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetQSyncMode(QSyncMode qsync_mode) {
  SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);
  auto sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  auto err = sdm_display->SetQSyncMode(qsync_mode);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  tui_->SetBackendQsyncMode(qsync_mode);
  return kErrorNone;
}

DisplayError SDMServices::SetColorSamplingEnabled(int dpy, int enabled_cmd) {
  if (dpy < SDM_DISPLAY_PRIMARY || dpy >= SDM_NUM_DISPLAY_TYPES ||
      enabled_cmd < 0 || enabled_cmd > 1) {
    return kErrorNotSupported;
  }

  SEQUENCE_WAIT_SCOPE_LOCK(locker_[dpy]);
  auto sdm_display = cb_->GetDisplayFromClientId(dpy);
  if (!sdm_display) {
    DLOGW("No display id %i active to enable histogram event", dpy);
    return kErrorHardware;
  }

  auto error =
      sdm_display->SetDisplayedContentSamplingEnabledVndService(enabled_cmd);
  return error;
}

DisplayError SDMServices::ControlIdlePowerCollapse(int disp_id, bool enable,
                                                   bool synchronous) {
  auto sdm_display = cb_->GetDisplayFromClientId(disp_id);
  auto idle_pc_ref_cnt = cb_->GetIdlePcRefCnt();

  if (disp_id >= kNumDisplays) {
    DLOGE("No active displays");
    return kErrorNotSupported;
  }
  bool needs_refresh = false;
  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[disp_id]);
    if (sdm_display) {
      if (!enable) {
        if (!idle_pc_ref_cnt) {
          auto err = sdm_display->ControlIdlePowerCollapse(enable, synchronous);
          if (err != kErrorNone) {
            return err;
          }
          needs_refresh = true;
        }
        idle_pc_ref_cnt++;
      } else if (idle_pc_ref_cnt > 0) {
        if (!(idle_pc_ref_cnt - 1)) {
          auto err = sdm_display->ControlIdlePowerCollapse(enable, synchronous);
          if (err != kErrorNone) {
            return err;
          }
        }
        idle_pc_ref_cnt--;
      }
    } else {
      DLOGW("Display = %d is not connected.", UINT32(disp_id));
      return kErrorHardware;
    }
  }

  if (needs_refresh) {
    auto ret = cb_->WaitForCommitDone(disp_id, kClientIdlepowerCollapse);
    if (ret != kErrorNone) {
      DLOGW("%s Idle PC failed with error %d", enable ? "Enable" : "Disable",
            ret);
      return ret;
    }
  }

  DLOGI("Idle PC %s!!", enable ? "enabled" : "disabled");
  return kErrorNone;
}

DisplayError SDMServices::SetDisplayDppsAdROI(uint32_t display_id,
                                              uint32_t h_start, uint32_t h_end,
                                              uint32_t v_start, uint32_t v_end,
                                              uint32_t factor_in,
                                              uint32_t factor_out) {
  auto sdm_display = cb_->GetDisplayFromClientId(display_id);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", display_id);
    return kErrorHardware;
  }

  auto err = sdm_display->SetDisplayDppsAdROI(h_start, h_end, v_start, v_end,
                                              factor_in, factor_out);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetDsiClk(int disp_id, uint64_t clk) {
  auto sdm_display = cb_->GetDisplayFromClientId(disp_id);
  if (!sdm_display) {
    return kErrorNotSupported;
  }

  return sdm_display->ScheduleDynamicDSIClock(clk);
}

DisplayError SDMServices::SetJitterConfig(uint32_t jitter_type,
                                          float jitter_val,
                                          uint32_t jitter_time) {
  SEQUENCE_WAIT_SCOPE_LOCK(locker_[SDM_DISPLAY_PRIMARY]);
  auto sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  return sdm_display->SetJitterConfig(jitter_type, jitter_val, jitter_time);
}

DisplayError SDMServices::GetDsiClk(int disp_id, uint64_t *clk) {
  if (disp_id != SDM_DISPLAY_PRIMARY) {
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_id);
  if (!sdm_display) {
    return kErrorNotSupported;
  }

  auto err = sdm_display->GetDynamicDSIClock(clk);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::GetSupportedDsiClk(int disp_id,
                                             std::vector<uint64_t> *bitrates) {
  if (disp_id != SDM_DISPLAY_PRIMARY) {
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_id);
  if (!sdm_display) {
    return kErrorNotSupported;
  }

  auto err = sdm_display->GetSupportedDSIClock(bitrates);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetPanelLuminanceAttributes(int disp_id,
                                                      float min_lum,
                                                      float max_lum) {
  // currently doing only for virtual display
  if (disp_id != qdutilsDisplayType::DISPLAY_VIRTUAL) {
    return kErrorNotSupported;
  }

  // check for out of range luminance values
  if (min_lum <= 0.0f || min_lum >= 1.0f || max_lum <= 100.0f ||
      max_lum >= 1000.0f) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetColorModeFromClient(int disp_id, int mode) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  auto err = sdm_display->SetColorModeFromClientApi(mode);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  cb_->Refresh(disp_idx);

  return kErrorNone;
}

DisplayError SDMServices::SetFrameTriggerMode(int disp_id, int mode) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  auto err = sdm_display->SetFrameTriggerMode(UINT32(mode));
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::SetDisplayBrightnessScale(int disp_id, int level,
                                                    int dre_case) {
  if (level < 0) {
    DLOGE("Invalid backlight scale level %d", level);
    return kErrorNotSupported;
  }

  // Non-Dre case to check max backlight scale
  if (!dre_case && level > kBrightnessScaleMax) {
    DLOGE("Invalid backlight scale level %d, max scale %d, dre_case %d", level,
          kBrightnessScaleMax, dre_case);
    return kErrorNotSupported;
  }

  auto bl_scale = level * kSvBlScaleMax / kBrightnessScaleMax;

  auto sdm_display = cb_->GetDisplayFromClientId(disp_id);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_id);
    return kErrorHardware;
  }

  auto err = sdm_display->SetBLScale(UINT32(bl_scale));
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  cb_->Refresh(disp_id);
  return kErrorNone;
}

DisplayError SDMServices::SetBppMode(uint32_t bpp) {
  auto sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);

  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  auto err = sdm_display->SetBppMode(bpp);
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::UpdateTransferTime(uint32_t transfer_time) {
  auto sdm_display = cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY);

  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", SDM_DISPLAY_PRIMARY);
    return kErrorHardware;
  }

  return sdm_display->Perform(UPDATE_TRANSFER_TIME, transfer_time);
}

DisplayError SDMServices::RetrieveDemuraTnFiles(int disp_id) {
  int disp_idx = disp_->GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  auto sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorHardware;
  }

  auto err = sdm_display->RetrieveDemuraTnFiles();
  if (err != kErrorNone) {
    return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError SDMServices::UpdateTransferTime(SDMParcel *input_parcel) {
  uint32_t transfer_time = UINT32(input_parcel->readInt32());
  return UpdateTransferTime(transfer_time);
}

DisplayError SDMServices::RetrieveDemuraTnFiles(SDMParcel *input_parcel,
                                                SDMParcel *output_parcel) {
  auto display_id = static_cast<int>(input_parcel->readInt32());

  DisplayError status = RetrieveDemuraTnFiles(display_id);
  if (status != kErrorNone) {
    return status;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::SetDemuraState(SDMParcel *input_parcel,
                                         SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int state = input_parcel->readInt32();
  auto ret = cb_->SetDemuraState(disp_id, state);
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);

  return kErrorNone;
}

DisplayError SDMServices::SetDemuraConfig(SDMParcel *input_parcel,
                                          SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int config = input_parcel->readInt32();
  auto ret = cb_->SetDemuraConfig(disp_id, config);
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::GetDisplayPortId(SDMParcel *input_parcel, SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int port_id = 0;
  auto err = cb_->GetDisplayPortId(UINT32(disp_id), &port_id);
  output_parcel->writeInt32(port_id);
  return err;
}

DisplayError SDMServices::PerformCacConfig(SDMParcel *input_parcel) {
  int display = INT(input_parcel->readInt32());
  int disp_idx = disp_->GetDisplayIndex(display);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", display);
    return kErrorParameters;
  }

  CacConfig config = {};
  bool cac_enable = UINT32(input_parcel->readInt32());

  if (cac_enable) {
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.k0r = input_parcel->readDouble();
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.k1r = input_parcel->readDouble();
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.k0b = input_parcel->readDouble();
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.k1b = input_parcel->readDouble();
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.pixel_pitch = input_parcel->readDouble();
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.normalization = input_parcel->readDouble();
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.mid_le_y_offset = UINT32(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.mid_le_x_offset = UINT32(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.mid_re_y_offset = UINT32(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.mid_re_x_offset = UINT32(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      config.skip_inc = UINT32(input_parcel->readInt32());
    }
  }

  const auto &sdm_display = cb_->GetDisplayFromClientId(disp_idx);
  if (!sdm_display) {
    DLOGW("Display = %d is not connected.", disp_idx);
    return kErrorResources;
  }

  return sdm_display->PerformCacConfig(config, cac_enable);
}

DisplayError SDMServices::GetComposerStatus(SDMParcel *input_parcel,
                                            SDMParcel *output_parcel) {
  bool is_composer_up = cb_->GetComposerStatus();

  output_parcel->writeInt32(is_composer_up);

  return kErrorNone;
}

DisplayError SDMServices::SetVSyncState(SDMParcel *input_parcel,
                                        SDMParcel *output_parcel) {
  auto display = input_parcel->readInt32();
  int32_t enable = input_parcel->readInt32();
  bool vsync_state = false;
  if (enable == 1) {
    vsync_state = true;
  }

  auto ret = cb_->SetVsyncEnabled(display, enable);
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::HandleTUITransition(SDMParcel *input_parcel,
                                              SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int event = input_parcel->readInt32();

  DisplayError ret = tui_->HandleTUITransition(disp_id, event);
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::SetDimmingEnable(SDMParcel *input_parcel,
                                           SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int enable = input_parcel->readInt32();

  auto ret = cb_->SetDimmingEnable(disp_id, enable);
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::SetDimmingMinBl(SDMParcel *input_parcel,
                                          SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int min_bl = input_parcel->readInt32();

  auto ret = cb_->SetDimmingMinBl(disp_id, min_bl);
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError
SDMServices::GetDisplayAttributesForConfig(SDMParcel *input_parcel,
                                           SDMParcel *output_parcel) {
  int config = input_parcel->readInt32();
  int dpy = input_parcel->readInt32();
  DisplayConfigVariableInfo display_attributes;

  auto error = GetDisplayAttributesForConfig(dpy, config, &display_attributes);
  if (error == kErrorNone) {
    output_parcel->writeInt32(INT(display_attributes.vsync_period_ns));
    output_parcel->writeInt32(INT(display_attributes.x_pixels));
    output_parcel->writeInt32(INT(display_attributes.y_pixels));
    output_parcel->writeFloat(display_attributes.x_dpi);
    output_parcel->writeFloat(display_attributes.y_dpi);
    output_parcel->writeInt32(0); // Panel type, unsupported.
  }

  return error;
}

DisplayError SDMServices::setColorSamplingEnabled(SDMParcel *input_parcel) {
  int dpy = input_parcel->readInt32();
  int enabled_cmd = input_parcel->readInt32();

  return SetColorSamplingEnabled(dpy, enabled_cmd);
}

DisplayError SDMServices::ConfigureRefreshRate(SDMParcel *input_parcel) {
  uint32_t operation = UINT32(input_parcel->readInt32());
  uint32_t refresh_rate = 0;
  if (operation == MetadataOps::SET_BINDER_DYNAMIC_REFRESH_RATE) {
    refresh_rate = UINT32(input_parcel->readInt32());
  }
  return ConfigureRefreshRate(operation, refresh_rate);
}

DisplayError SDMServices::SetDisplayMode(SDMParcel *input_parcel) {
  int mode = INT32(input_parcel->readInt32());
  return SetDisplayMode(mode);
}

DisplayError SDMServices::SetMaxMixerStages(SDMParcel *input_parcel) {
  std::bitset<32> bit_mask_display_type = UINT32(input_parcel->readInt32());
  int32_t max_mixer_stages = INT32(input_parcel->readInt32());

  return SetMaxMixerStages(bit_mask_display_type, max_mixer_stages);
}

DisplayError SDMServices::ValidateFrameDumpConfig(
    uint32_t frame_dump_count, uint32_t bit_mask_disp_type,
    uint32_t bit_mask_layer_type, int32_t *processable_cwb_requests) {
  std::bitset<32> bit_mask_display_type = bit_mask_disp_type;

  // Checking for frame count, display type and layer type bitmask as 0, which
  // is unsupported input.
  if (!frame_dump_count || bit_mask_display_type.none() ||
      !bit_mask_layer_type) {
    DLOGW("Invalid request with unsupported input(%s=0) for frame dump!",
          (!frame_dump_count)              ? "frame_dump_count"
          : (bit_mask_display_type.none()) ? "bit_mask_display_type"
                                           : "bit_mask_layer_type");
    return kErrorNotSupported;
  }

  bool output_buffer_dump = bit_mask_layer_type & (1 << OUTPUT_LAYER_DUMP);
  if (output_buffer_dump) {
    // Get running virtual display count which are using H/W WB block.
    uint32_t virtual_dpy_index = disp_->GetDisplayIndex(qdutilsDisplayType::DISPLAY_VIRTUAL);
    uint32_t running_vds = (virtual_dpy_index != -1 &&
                            cb_->GetDisplayFromClientId(virtual_dpy_index))
                               ? 1
                               : 0;
    virtual_dpy_index = disp_->GetDisplayIndex(qdutilsDisplayType::DISPLAY_VIRTUAL_2);
    running_vds += ((virtual_dpy_index != -1) &&
                    cb_->GetDisplayFromClientId(virtual_dpy_index))
                       ? 1
                       : 0;

    // Get requested virtual display count.
    uint32_t requested_vds =
        (bit_mask_display_type.test(qdutilsDisplayType::DISPLAY_VIRTUAL)) ? 1 : 0;
    requested_vds += (bit_mask_display_type.test(qdutilsDisplayType::DISPLAY_VIRTUAL_2)) ? 1 : 0;

    // Get requested physical display count.
    uint32_t requested_pds = bit_mask_display_type.count() - requested_vds;

    // Get available writeback block count.
    uint32_t available_wbs = disp_->GetVirtualDisplayCount() - running_vds;

    // if no any virtual display is running, but requested only virtual display
    // output dump, then can't process it.
    if (!running_vds && requested_vds && !requested_pds) {
      DLOGW("No any virtual display is running for virtual output frame dump.");
      return kErrorNotSupported;
    }

    // if any virtual displays is running and all WBs are occupied, but
    // requested only physical display output dump, then can't process it.
    if (requested_pds && !available_wbs && !requested_vds) {
      DLOGW("No any writeback block is available for CWB output frame dump.");
      return kErrorNotSupported;
    }

    // Get processable count of physical display output buffer request.
    *processable_cwb_requests = std::min(requested_pds, available_wbs);
  }

  return kErrorNone;
}

DisplayError SDMServices::SetFrameDumpConfig(SDMParcel *input_parcel) {
  uint32_t frame_dump_count = UINT32(input_parcel->readInt32());
  std::bitset<32> bit_mask_display_type = UINT32(input_parcel->readInt32());
  uint32_t bit_mask_layer_type = UINT32(input_parcel->readInt32());

  int32_t processable_cwb_requests = 0;
  auto err = ValidateFrameDumpConfig(
      frame_dump_count, bit_mask_display_type.to_ulong(), bit_mask_layer_type,
      &processable_cwb_requests);
  // if validation error occurs, just discard the frame dump request.
  if (err != kErrorNone) {
    return err;
  }

  // Read optional user preferences: output_format, tap_point, pu_in_cwb_roi,
  // cwb_roi.
  int32_t output_format =
      static_cast<int>(SDMPixelFormat::PIXEL_FORMAT_RGB_888);
  CwbConfig cwb_config = {};

  if (input_parcel->dataPosition() != input_parcel->dataSize()) {
    // HAL Pixel Format for output buffer
    output_format = input_parcel->readInt32();
  }

  LayerBufferFormat sdm_format = buffer_allocator_->GetSDMFormat(output_format, 0, 0);
  if (sdm_format == kFormatInvalid) {
    DLOGW("Format %d is not supported by SDM", output_format);
    return kErrorNotSupported;
  }

  if (processable_cwb_requests > 0) {
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      // Option to dump Layer Mixer output (0) or DSPP output (1) or Demura
      // output (2)
      cwb_config.tap_point =
          static_cast<CwbTapPoint>(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      std::bitset<32> bit_mask_cwb_flag = UINT32(input_parcel->readInt32());
      // Option to include PU ROI in CWB ROI, and retrieve it from corresponding
      // bit of CWB flag.
      cwb_config.pu_as_cwb_roi =
          static_cast<bool>(bit_mask_cwb_flag[kCwbFlagPuAsCwbROI]);
      // Option to avoid additional refresh to process pending CWB requests, and
      // retrieve it from corresponding bit of CWB flag.
      cwb_config.avoid_refresh =
          static_cast<bool>(bit_mask_cwb_flag[kCwbFlagAvoidRefresh]);
    }

    LayerRect &cwb_roi = cwb_config.cwb_roi;
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      cwb_roi.left = static_cast<float>(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      cwb_roi.top = static_cast<float>(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      cwb_roi.right = static_cast<float>(input_parcel->readInt32());
    }
    if (input_parcel->dataPosition() != input_parcel->dataSize()) {
      cwb_roi.bottom = static_cast<float>(input_parcel->readInt32());
    }
  }

  return SetFrameDumpConfig(frame_dump_count, bit_mask_display_type,
                            bit_mask_layer_type, processable_cwb_requests,
                            output_format, cwb_config);
}

DisplayError SDMServices::SetMixerResolution(SDMParcel *input_parcel) {
  int dpy = INT(input_parcel->readInt32());
  uint32_t width = UINT32(input_parcel->readInt32());
  uint32_t height = UINT32(input_parcel->readInt32());

  return SetMixerResolution(dpy, width, height);
}

DisplayError SDMServices::SetColorModeOverride(SDMParcel *input_parcel) {
  int display = static_cast<int>(input_parcel->readInt32());
  auto mode = static_cast<SDMColorMode>(input_parcel->readInt32());

  return SetColorModeOverride(display, mode);
}

DisplayError SDMServices::SetAd4RoiConfig(SDMParcel *input_parcel) {
  auto display_id = static_cast<uint32_t>(input_parcel->readInt32());
  auto h_s = static_cast<uint32_t>(input_parcel->readInt32());
  auto h_e = static_cast<uint32_t>(input_parcel->readInt32());
  auto v_s = static_cast<uint32_t>(input_parcel->readInt32());
  auto v_e = static_cast<uint32_t>(input_parcel->readInt32());
  auto f_in = static_cast<uint32_t>(input_parcel->readInt32());
  auto f_out = static_cast<uint32_t>(input_parcel->readInt32());

  return SetDisplayDppsAdROI(display_id, h_s, h_e, v_s, v_e, f_in, f_out);
}

DisplayError SDMServices::SetFrameTriggerMode(SDMParcel *input_parcel) {
  auto display_id = static_cast<int>(input_parcel->readInt32());
  auto mode = static_cast<int>(input_parcel->readInt32());

  return SetFrameTriggerMode(display_id, mode);
}

DisplayError
SDMServices::SetColorModeWithRenderIntentOverride(SDMParcel *input_parcel) {
  auto display = INT(input_parcel->readInt32());
  auto mode = static_cast<SDMColorMode>(input_parcel->readInt32());
  auto int_intent = static_cast<int>(input_parcel->readInt32());

  return SetColorModeWithRenderIntentOverride(display, mode, int_intent);
}
DisplayError SDMServices::SetColorModeById(SDMParcel *input_parcel) {
  int display = input_parcel->readInt32();
  auto mode = input_parcel->readInt32();

  return SetColorModeById(display, mode);
}

DisplayError SDMServices::SetColorModeFromClient(SDMParcel *input_parcel) {
  int display = input_parcel->readInt32();
  auto mode = input_parcel->readInt32();

  return SetColorModeFromClient(display, mode);
}

DisplayError SDMServices::RefreshScreen(SDMParcel *input_parcel) {
  int display = input_parcel->readInt32();
  int disp_idx = disp_->GetDisplayIndex(display);

  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", display);
    return kErrorNotSupported;
  }

  RefreshScreen(disp_idx);

  return kErrorNone;
}

DisplayError SDMServices::DynamicDebug(SDMParcel *input_parcel) {
  int type = input_parcel->readInt32();
  bool enable = (input_parcel->readInt32() > 0);
  int verbose_level = input_parcel->readInt32();

  return DynamicDebug(type, enable, verbose_level);
}

DisplayError SDMServices::SetIdleTimeout(SDMParcel *input_parcel) {
  int active_ms = input_parcel->readInt32();

  return SetIdleTimeout(active_ms);
}

DisplayError SDMServices::SetDisplayStatus(SDMParcel *input_parcel,
                                           SDMParcel *output_parcel) {
  int disp_id = INT(input_parcel->readInt32());
  SDMDisplayStatus disp_status =
      static_cast<SDMDisplayStatus>(input_parcel->readInt32());
  DisplayError status = SetDisplayStatus(disp_id, disp_status);
  if (status != kErrorNone) {
    return status;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::ToggleScreenUpdate(SDMParcel *input_parcel,
                                             SDMParcel *output_parcel) {
  int32_t input = input_parcel->readInt32();
  Display active_disp = disp_->GetActiveBuiltinDisplay();
  DisplayError status = ToggleScreenUpdate(active_disp, (input == 1));
  if (status != kErrorNone) {
    return status;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError
SDMServices::MinHdcpEncryptionLevelChanged(SDMParcel *input_parcel,
                                           SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  uint32_t min_enc_level = input_parcel->readInt32();
  DisplayError status = MinHdcpEncryptionLevelChanged(disp_id, min_enc_level);
  if (status != kErrorNone) {
    return status;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::ControlPartialUpdate(SDMParcel *input_parcel,
                                               SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  uint32_t enable = input_parcel->readInt32();
  DisplayError status = ControlPartialUpdate(disp_id, (enable == 1));
  if (status != kErrorNone) {
    return status;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::GetActiveConfigIndex(SDMParcel *input_parcel,
                                               SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  uint32_t config = 0;
  DisplayError status = GetActiveConfigIndex(disp_id, &config);
  if (!status) {
    output_parcel->writeInt32(INT(config));
    return kErrorNone;
  }

  return status;
}

DisplayError SDMServices::GetConfigCount(SDMParcel *input_parcel,
                                         SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  uint32_t count = 0;
  DisplayError status = GetConfigCount(disp_id, &count);
  if (status != kErrorNone) {
    return status;
  }

  output_parcel->writeInt32(INT(count));
  return kErrorNone;
}

DisplayError SDMServices::GetDisplayBrightness(SDMParcel *input_parcel,
                                               SDMParcel *output_parcel) {
  uint32_t display = input_parcel->readInt32();
  uint32_t max_brightness_level = 0;
  DisplayError status = GetDisplayMaxBrightness(display, &max_brightness_level);
  if (status || !max_brightness_level) {
    output_parcel->writeInt32(max_brightness_level);
    DLOGE("Failed to get max brightness %u, status %d", max_brightness_level,
          status);
    return kErrorNone;
  }
  DLOGV("Panel max brightness is %u", max_brightness_level);

  float brightness_percent = -1.0f;
  status = GetDisplayBrightnessPercent(display, &brightness_percent);
  if (brightness_percent == -1.0f) {
    output_parcel->writeInt32(0);
  } else {
    output_parcel->writeInt32(
        INT32(brightness_percent * (max_brightness_level - 1) + 1));
  }

  return status;
}

DisplayError SDMServices::SetDisplayBrightness(SDMParcel *input_parcel,
                                               SDMParcel *output_parcel) {
  uint32_t max_brightness_level = 0;
  uint32_t display = SDM_DISPLAY_PRIMARY;
  DisplayError status = GetDisplayMaxBrightness(display, &max_brightness_level);
  if (status || max_brightness_level < 1) {
    output_parcel->writeInt32(max_brightness_level);
    DLOGE("Failed to get max brightness %u, status %d", max_brightness_level,
          status);
    return kErrorNone;
  }

  int level = input_parcel->readInt32();
  DisplayError ret = kErrorNone;
  if (level == 0) {
    ret = cb_->SetDisplayBrightness(display, -1.0f);
  } else {
    ret = cb_->SetDisplayBrightness(
        display, (level - 1) / (static_cast<float>(max_brightness_level - 1)));
  }
  if (ret != kErrorNone) {
    return ret;
  }

  output_parcel->writeInt32(kErrorNone);
  return kErrorNone;
}

DisplayError SDMServices::SetNoisePlugInOverride(SDMParcel *input_parcel) {
  int32_t disp_id = input_parcel->readInt32();
  bool override_en = ((input_parcel->readInt32()) == 1);

  int32_t attn = -1;
  if (input_parcel->dataPosition() != input_parcel->dataSize()) {
    attn = input_parcel->readInt32();
  }

  int32_t noise_zpos = -1;
  if (input_parcel->dataPosition() != input_parcel->dataSize()) {
    noise_zpos = input_parcel->readInt32();
  }

  return SetNoisePlugInOverride(disp_id, override_en, attn, noise_zpos);
}

DisplayError SDMServices::SetActiveConfigIndex(SDMParcel *input_parcel) {
  uint32_t config = UINT32(input_parcel->readInt32());
  int disp_id = input_parcel->readInt32();
  return SetActiveConfigIndex(disp_id, config);
}

DisplayError SDMServices::SetCameraLaunchStatus(SDMParcel *input_parcel) {
  int camera_status = INT(input_parcel->readInt32());
  return SetCameraLaunchStatus(camera_status);
}

DisplayError
SDMServices::DisplayBWTransactionPending(SDMParcel *input_parcel,
                                         SDMParcel *output_parcel) {
  bool pending = true;
  auto status = DisplayBWTransactionPending(&pending);
  output_parcel->writeInt32(pending);
  return status;
}

DisplayError SDMServices::QdcmCMDDispatch(
    uint32_t display_id, const PPDisplayAPIPayload &req_payload,
    PPDisplayAPIPayload *resp_payload, PPPendingParams *pending_action) {
  DisplayError ret = kErrorNone;
  bool is_physical_display = false;

  if (display_id >= kNumDisplays || !cb_->GetDisplayFromClientId(display_id)) {
    DLOGW("Invalid display id or display = %d is not connected.", display_id);
    return kErrorHardware;
  }

  if (display_id == disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_PRIMARY)[0].client_id) {
    is_physical_display = true;
  } else {
    for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2)) {
      if (map_info.client_id == display_id) {
        is_physical_display = true;
        break;
      }
    }
  }

  if (!is_physical_display) {
    DLOGW("Skipping QDCM command dispatch on display = %d", display_id);
    return ret;
  }

  ret = cb_->GetDisplayFromClientId(display_id)
            ->ColorSVCRequestRoute(req_payload, resp_payload, pending_action);

  return ret;
}

DisplayError SDMServices::QdcmCMDHandler(SDMParcel *input_parcel,
                                         SDMParcel *output_parcel) {
  DisplayError ret = kErrorNone;
  float *brightness = NULL;
  uint32_t display_id(0);
  PPPendingParams pending_action;
  PPDisplayAPIPayload resp_payload, req_payload;
  uint8_t *disp_id = NULL;
  int32_t *mode_id = NULL;

  if (!color_mgr_) {
    DLOGW("color_mgr_ not initialized.");
    return kErrorNotSupported;
  }

  pending_action.action = kNoAction;
  pending_action.params = NULL;

  // Read display_id, payload_size and payload from in_parcel.
  ret = SDMColorManager::CreatePayloadFromParcel(input_parcel, &display_id,
                                                 &req_payload);
  if (!ret) {
    ret = QdcmCMDDispatch(display_id, req_payload, &resp_payload,
                          &pending_action);
  }

  if (ret) {
    output_parcel->writeInt32(
        ret); // first field in out parcel indicates return code.
    req_payload.DestroyPayload();
    resp_payload.DestroyPayload();
    return ret;
  }

  if (kNoAction != pending_action.action) {
    int32_t action = pending_action.action;
    int count = -1;
    while (action > 0) {
      count++;
      int32_t bit = (action & 1);
      action = action >> 1;

      if (!bit)
        continue;

      DLOGV_IF(kTagQDCM, "pending action = %d, display_id = %d", BITMAP(count),
               display_id);
      switch (BITMAP(count)) {
      case kInvalidating:
        cb_->Refresh(display_id);
        break;
      case kEnterQDCMMode:
        ret = color_mgr_->EnableQDCMMode(
            true, cb_->GetDisplayFromClientId(display_id));
        cb_->GetDisplayFromClientId(display_id)
            ->NotifyDisplayCalibrationMode(true);
        break;
      case kExitQDCMMode:
        ret = color_mgr_->EnableQDCMMode(
            false, cb_->GetDisplayFromClientId(display_id));
        cb_->GetDisplayFromClientId(display_id)
            ->NotifyDisplayCalibrationMode(false);
        break;
      case kApplySolidFill: {
        SCOPE_LOCK(locker_[display_id]);
        ret = color_mgr_->SetSolidFill(pending_action.params, true,
                                       cb_->GetDisplayFromClientId(display_id));
      }
        cb_->Refresh(display_id);
        usleep(kSolidFillDelay);
        break;
      case kDisableSolidFill: {
        SCOPE_LOCK(locker_[display_id]);
        ret = color_mgr_->SetSolidFill(pending_action.params, false,
                                       cb_->GetDisplayFromClientId(display_id));
      }
        cb_->Refresh(display_id);
        usleep(kSolidFillDelay);
        break;
      case kSetPanelBrightness:
        brightness = reinterpret_cast<float *>(resp_payload.payload);
        if (brightness == NULL) {
          DLOGE("Brightness payload is Null");
          ret = kErrorParameters;
        } else {
          auto err = cb_->SetDisplayBrightness(static_cast<Display>(display_id),
                                               *brightness);
          if (err != kErrorNone) {
            ret = kErrorNotSupported;
          }
        }
        break;
      case kEnableFrameCapture: {
        int external_dpy_index = disp_->GetDisplayIndex(qdutilsDisplayType::DISPLAY_EXTERNAL);
        int virtual_dpy_index = disp_->GetDisplayIndex(qdutilsDisplayType::DISPLAY_VIRTUAL);
        if (((external_dpy_index != -1) &&
             cb_->GetDisplayFromClientId(external_dpy_index)) ||
            ((virtual_dpy_index != -1) &&
             cb_->GetDisplayFromClientId(virtual_dpy_index))) {
          return kErrorHardware;
        }
        ret = color_mgr_->SetFrameCapture(
            pending_action.params, true,
            cb_->GetDisplayFromClientId(display_id));
        cb_->Refresh(display_id);
      } break;
      case kDisableFrameCapture:
        ret = color_mgr_->SetFrameCapture(
            pending_action.params, false,
            cb_->GetDisplayFromClientId(display_id));
        break;
      case kConfigureDetailedEnhancer:
        ret = color_mgr_->SetDetailedEnhancer(
            pending_action.params, cb_->GetDisplayFromClientId(display_id));
        cb_->Refresh(display_id);
        break;
      case kModeSet:
        ret = cb_->GetDisplayFromClientId(display_id)->RestoreColorTransform();
        cb_->Refresh(display_id);
        break;
      case kNoAction:
        break;
      case kMultiDispProc:
        for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2)) {
          uint32_t id = UINT32(map_info.client_id);
          if (id < kNumDisplays && cb_->GetDisplayFromClientId(id)) {
            auto result = kErrorNone;
            resp_payload.DestroyPayload();
            result = cb_->GetDisplayFromClientId(id)->ColorSVCRequestRoute(
                req_payload, &resp_payload, &pending_action);
            if (result) {
              DLOGW("Failed to dispatch action to disp %d ret %d", id, result);
              ret = result;
            }
          }
        }
        break;
      case kMultiDispGetId:
        ret = resp_payload.CreatePayloadBytes(kNumDisplays, &disp_id);
        if (ret) {
          DLOGW("Unable to create response payload!");
        } else {
          for (int i = 0; i < kNumDisplays; i++) {
            disp_id[i] = kNumDisplays;
          }
          if (cb_->GetDisplayFromClientId(SDM_DISPLAY_PRIMARY)) {
            disp_id[SDM_DISPLAY_PRIMARY] = SDM_DISPLAY_PRIMARY;
          }
          for (auto &map_info : disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2)) {
            uint64_t id = map_info.client_id;
            if (id < kNumDisplays && cb_->GetDisplayFromClientId(id)) {
              disp_id[id] = (uint8_t)id;
            }
          }
        }
        break;
      case kSetModeFromClient: {
        SCOPE_LOCK(locker_[display_id]);
        mode_id = reinterpret_cast<int32_t *>(resp_payload.payload);
        if (mode_id) {
          ret = cb_->GetDisplayFromClientId(display_id)
                    ->SetColorModeFromClientApi(*mode_id);
        } else {
          DLOGE("mode_id is Null");
          ret = kErrorNotSupported;
        }
      }
        if (!ret) {
          cb_->Refresh(display_id);
        }
        break;
      default:
        DLOGW("Invalid pending action = %d!", pending_action.action);
        break;
      }
    }
  }
  // for display API getter case, marshall returned params into out_parcel.
  output_parcel->writeInt32(ret);
  SDMColorManager::MarshallStructIntoParcel(resp_payload, output_parcel);
  req_payload.DestroyPayload();
  resp_payload.DestroyPayload();

  return ret;
}

DisplayError SDMServices::SetJitterConfig(SDMParcel *input_parcel) {
  uint32_t jitter_type = UINT32(input_parcel->readInt32());
  float jitter_val = input_parcel->readFloat();
  uint32_t jitter_time = UINT32(input_parcel->readInt32());

  return SetJitterConfig(jitter_type, jitter_val, jitter_time);
}

DisplayError SDMServices::SetDsiClk(SDMParcel *input_parcel) {
  uint32_t disp_id = UINT32(input_parcel->readInt32());
  uint64_t clk = UINT64(input_parcel->readInt64());
  if (disp_id != SDM_DISPLAY_PRIMARY) {
    auto &map_info_builtin = disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2);
    if (!std::any_of(map_info_builtin.begin(), map_info_builtin.end(),
                     [&disp_id](auto &i) { return disp_id == i.client_id; })) {
      return kErrorNotSupported;
    }
  }

  return SetDsiClk(INT(disp_id), clk);
}

DisplayError SDMServices::GetDsiClk(SDMParcel *input_parcel,
                                    SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();

  uint64_t bitrate = 0;
  auto status = GetDsiClk(disp_id, &bitrate);
  output_parcel->writeUint64(bitrate);

  return status;
}

DisplayError SDMServices::GetSupportedDsiClk(SDMParcel *input_parcel,
                                             SDMParcel *output_parcel) {
  int disp_id = INT(input_parcel->readInt32());
  std::vector<uint64_t> bit_rates;
  auto status = GetSupportedDsiClk(disp_id, &bit_rates);
  output_parcel->writeInt32(INT32(bit_rates.size()));
  for (auto &bit_rate : bit_rates) {
    output_parcel->writeUint64(bit_rate);
  }

  return status;
}

DisplayError SDMServices::SetPanelLuminanceAttributes(SDMParcel *input_parcel) {
  int disp_id = input_parcel->readInt32();
  float min_lum = input_parcel->readFloat();
  float max_lum = input_parcel->readFloat();

  auto status = SetPanelLuminanceAttributes(disp_id, min_lum, max_lum);
  if (!status) {
    std::lock_guard<std::mutex> obj(*cb_->GetLumMutex());
    disp_->SetLuminance(min_lum, max_lum);
    DLOGI("set max_lum %f, min_lum %f", max_lum, min_lum);
  }

  return status;
}

DisplayError SDMServices::GetVisibleDisplayRect(SDMParcel *input_parcel,
                                                SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  SDMRect visible_rect = {0, 0, 0, 0};

  auto status = GetVisibleDisplayRect(disp_id, &visible_rect);

  if (status == kErrorNone) {
    output_parcel->writeInt32(visible_rect.left);
    output_parcel->writeInt32(visible_rect.top);
    output_parcel->writeInt32(visible_rect.right);
    output_parcel->writeInt32(visible_rect.bottom);
  }

  return status;
}

DisplayError SDMServices::SetBppMode(SDMParcel *input_parcel) {
  uint32_t bpp = UINT32(input_parcel->readInt32());
  return SetBppMode(bpp);
}

DisplayError SDMServices::SetQSyncMode(SDMParcel *input_parcel) {
  auto mode = input_parcel->readInt32();

  QSyncMode qsync_mode = kQSyncModeNone;
  switch (mode) {
  case SDM_SERVICE_QSYNC_MODE_NONE:
    qsync_mode = kQSyncModeNone;
    break;
  case SDM_SERVICE_QSYNC_MODE_CONTINUOUS:
    qsync_mode = kQSyncModeContinuous;
    break;
  case SDM_SERVICE_QSYNC_MODE_ONESHOT:
    qsync_mode = kQsyncModeOneShot;
    break;
  default:
    DLOGE("Qsync mode not supported %d", mode);
    return kErrorNotSupported;
  }

  return SetQSyncMode(qsync_mode);
}

DisplayError SDMServices::SetIdlePC(SDMParcel *input_parcel) {
  auto enable = input_parcel->readInt32();
  auto synchronous = input_parcel->readInt32();
  Display active_builtin_disp_id = disp_->GetActiveBuiltinDisplay();

  return ControlIdlePowerCollapse(active_builtin_disp_id, enable, synchronous);
}

DisplayError
SDMServices::ProcessDisplayBrightnessScale(SDMParcel *input_parcel) {
  auto display = input_parcel->readInt32();
  auto level = input_parcel->readInt32();

  // DPPS DRE case
  int32_t dre_case = 0;
  if (input_parcel->dataPosition() != input_parcel->dataSize()) {
    dre_case = input_parcel->readInt32();
  }

  return SetDisplayBrightnessScale(display, level, dre_case);
}

#ifdef PROFILE_COVERAGE_DATA
DisplayError SDMServices::DumpCodeCoverage(SDMParcel *input_parcel) {
  auto enable = input_parcel->readInt32();
  DLOGD("SDMServices: Flushing llvm profile data");
  __llvm_profile_try_write_file();

  return static_cast<DisplayError>(core_intf_->DumpCodeCoverage());
}
#endif

DisplayError SDMServices::SetPanelFeatureConfig(SDMParcel *input_parcel, SDMParcel *output_parcel) {
  int disp_id = input_parcel->readInt32();
  int type = input_parcel->readInt32();
  void *data_ptr = nullptr;
  int data_int = 0;
  float data_float = 0;
  uint64_t data_uint64 = 0;

  if (type >= PanelFeatureVendorServiceTypeMax) {
    DLOGE("Invalid type %d", type);
    return kErrorNotSupported;
  }

  // Query data type
  auto it = panel_feature_data_type_map_.find(static_cast<PanelFeatureVendorServiceType>(type));
  if (it == panel_feature_data_type_map_.end()) {
    DLOGE("Type %d not found in map", type);
    return kErrorNotSupported;
  }

  // Compare with corresponding data type in order to parse the data
  if (it->second.compare("int") == 0) {
    data_int = input_parcel->readInt32();
    data_ptr = &data_int;
  } else if (it->second.compare("float") == 0) {
    data_float = input_parcel->readFloat();
    data_ptr = &data_float;
  } else if (it->second.compare("uint64_t") == 0) {
    data_uint64 = input_parcel->readInt64();
    data_ptr = &data_uint64;
  } else {
    DLOGE("Invalue data type %s", it->second.c_str());
    return kErrorNotSupported;
  }

  auto ret = cb_->SetPanelFeatureConfig(disp_id, type, data_ptr);
  if (ret != kErrorNone) {
    output_parcel->write("FAILED", strlen("FAILED"));
  } else {
    output_parcel->writeInt32(ret);
  }
  return ret;
}
} // namespace sdm
