/*
Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* ?딠hanges from Qualcomm Innovation Center, Inc. are provided under the following license:
*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <fcntl.h>
#include <display/drm/sde_drm.h>
#include <utils/debug.h>
#include <utils/sys.h>
#include <utils/rect.h>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "hw_peripheral_drm.h"

#define __CLASS__ "HWPeripheralDRM"

using sde_drm::DRMDisplayType;
using sde_drm::DRMOps;
using sde_drm::DRMPowerMode;
using sde_drm::DppsFeaturePayload;
using sde_drm::DRMDppsFeatureInfo;
using sde_drm::DRMPanelFeatureID;
using sde_drm::DRMPanelFeatureInfo;
using sde_drm::DRMSecureMode;

namespace sdm {

HWPeripheralDRM::HWPeripheralDRM(int32_t display_id, BufferAllocator *buffer_allocator,
                                 HWInfoInterface *hw_info_intf)
  : HWDeviceDRM(buffer_allocator, hw_info_intf) {
  disp_type_ = DRMDisplayType::PERIPHERAL;
  device_name_ = "Peripheral";
  display_id_ = display_id;
  core_id_ = hw_info_intf->GetCoreId();
}

DisplayError HWPeripheralDRM::Init() {
  DisplayError ret = HWDeviceDRM::Init();
  if (ret != kErrorNone) {
    DLOGE("Init failed for %s", device_name_);
    return ret;
  }

  UpdateLoopBackConnector();
  InitDestScaler();
  InitAIScaler();

  PopulateBitClkRates();
  CreatePanelFeaturePropertyMap();

  return kErrorNone;
}

void HWPeripheralDRM::InitDestScaler() {
  if (hw_resource_.hw_dest_scalar_info.count) {
    // Do all destination scaler block resource allocations here.
    dest_scaler_blocks_used_ = 1;
    if (kQuadSplit == mixer_attributes_.split_type) {
      dest_scaler_blocks_used_ = 4;
    } else if (kDualSplit == mixer_attributes_.split_type) {
      dest_scaler_blocks_used_ = 2;
    }
    if (hw_resource_.hw_dest_scalar_info.count >=
        (hw_dest_scaler_blocks_used_[core_id_] + dest_scaler_blocks_used_)) {
      // Enough destination scaler blocks available so update the static counter.
      hw_dest_scaler_blocks_used_[core_id_] += dest_scaler_blocks_used_;
    } else {
      dest_scaler_blocks_used_ = 0;
    }
    scalar_data_.resize(dest_scaler_blocks_used_);
    dest_scalar_cache_.resize(dest_scaler_blocks_used_);
    // Update crtc (layer-mixer) configuration info.
    mixer_attributes_.dest_scaler_blocks_used = dest_scaler_blocks_used_;
  }

  topology_control_ = UINT32(sde_drm::DRMTopologyControl::DSPP);
  if (dest_scaler_blocks_used_) {
    topology_control_ |= UINT32(sde_drm::DRMTopologyControl::DEST_SCALER);
  }
}

void HWPeripheralDRM::InitAIScaler() {
  if (hw_resource_.hw_ai_scaler_count) {
    // Do all ai scaler block resource allocations here.
    // ai_scaler_blocks_used_ will be only one irrespective of single or dual DSI.
    if (kQuadSplit == mixer_attributes_.split_type) {
      ai_scaler_blocks_used_ = 0;  // Not Supported
      return;
    }

    ai_scaler_blocks_used_ = 1;
    if (hw_resource_.hw_ai_scaler_count >= (hw_ai_scaler_blocks_used_ + ai_scaler_blocks_used_)) {
      // Enough ai scaler blocks available so update the static counter.
      hw_ai_scaler_blocks_used_ += ai_scaler_blocks_used_;
    } else {
      ai_scaler_blocks_used_ = 0;
    }
    ai_scaler_cache_.resize(ai_scaler_blocks_used_);
    // Update crtc (layer-mixer) configuration info.
    mixer_attributes_.ai_scaler_blocks_used = ai_scaler_blocks_used_;
  }
}

void HWPeripheralDRM::PopulateBitClkRates() {
  if (!hw_panel_info_.dyn_bitclk_support) {
    return;
  }

  // Group all bit_clk_rates corresponding to DRM_PREFERRED mode.
  uint32_t width = connector_info_.modes[current_mode_index_].mode.hdisplay;
  uint32_t height = connector_info_.modes[current_mode_index_].mode.vdisplay;

  for (auto &mode_info : connector_info_.modes) {
    auto &mode = mode_info.mode;
    if (mode.hdisplay == width && mode.vdisplay == height) {
      for (auto &sub_mode_info : mode_info.sub_modes) {
        for (uint32_t index = 0; index < sub_mode_info.dyn_bitclk_list.size(); index++) {
          if (std::find(bitclk_rates_.begin(), bitclk_rates_.end(),
                sub_mode_info.dyn_bitclk_list[index]) == bitclk_rates_.end()) {
            bitclk_rates_.push_back(sub_mode_info.dyn_bitclk_list[index]);
            DLOGI("Possible bit_clk_rates %" PRIu64, sub_mode_info.dyn_bitclk_list[index]);
          }
        }
      }
    }
  }

  hw_panel_info_.bitclk_rates = bitclk_rates_;
  DLOGI("bit_clk_rates Size %zu", bitclk_rates_.size());
}

DisplayError HWPeripheralDRM::SetJitterConfig(uint32_t jitter_type, float value, uint32_t time) {
  return HWDeviceDRM::SetJitterConfig(jitter_type, value, time);
}

DisplayError HWPeripheralDRM::SetDynamicDSIClock(uint64_t bit_clk_rate) {
  if (last_power_mode_ == DRMPowerMode::DOZE_SUSPEND || last_power_mode_ == DRMPowerMode::OFF) {
    return kErrorNotSupported;
  }

  if (doze_poms_switch_done_ || pending_poms_switch_) {
    return kErrorNotSupported;
  }

  if (vrefresh_) {
    // vrefresh change pending.
    // Defer bit rate clock change.
    return kErrorNotSupported;
  }

  if (GetSupportedBitClkRate(current_mode_index_, bit_clk_rate) ==
      connector_info_.modes[current_mode_index_].curr_bit_clk_rate) {
    return kErrorNone;
  }

  bit_clk_rate_ = bit_clk_rate;
  return kErrorNone;
}

DisplayError HWPeripheralDRM::GetDynamicDSIClock(uint64_t *bit_clk_rate) {
  // Update bit_rate corresponding to current refresh rate.
  *bit_clk_rate = (uint32_t)connector_info_.modes[current_mode_index_].curr_bit_clk_rate;
  return kErrorNone;
}


DisplayError HWPeripheralDRM::SetRefreshRate(uint32_t refresh_rate) {
  if (doze_poms_switch_done_ || pending_poms_switch_) {
    // poms switch in progress
    // Defer any refresh rate setting.
    return kErrorNotSupported;
  }

  DisplayError error = HWDeviceDRM::SetRefreshRate(refresh_rate);
  if (error != kErrorNone) {
    return error;
  }

  return kErrorNone;
}

DisplayError HWPeripheralDRM::SetDisplayMode(const HWDisplayMode hw_display_mode) {
  if (doze_poms_switch_done_ || pending_poms_switch_) {
    return kErrorNotSupported;
  }

  DisplayError error = HWDeviceDRM::SetDisplayMode(hw_display_mode);
  if (error != kErrorNone) {
    return error;
  }

  // update bit clk rates.
  hw_panel_info_.bitclk_rates = bitclk_rates_;

  return kErrorNone;
}

DisplayError HWPeripheralDRM::SetBppMode(uint32_t bpp) {

  if (bpp != static_cast<uint32_t>(kBppMode24) && bpp != static_cast<uint32_t>(kBppMode30)) {
    DLOGE("Invalid bpp mode parameter");
    return kErrorParameters;
  }

  if (bpp == connector_info_.modes[current_mode_index_].curr_bpp_mode) {
    DLOGE("Same as current bpp mode");
    return kErrorParameters;
  }

  //Check whether the sub_modes in current mode support the bpp mode
  sde_drm::DRMModeInfo current_mode = connector_info_.modes[current_mode_index_];
  for (uint32_t submode_idx = 0; submode_idx < current_mode.sub_modes.size(); submode_idx++) {
    if (bpp == current_mode.sub_modes[submode_idx].bpp_mode) {
      bpp_mode_changed_ = bpp;
      return kErrorNone;
    }
  }

  DLOGW("current display mode dont't support switch to bpp: %d", bpp);
  return kErrorNotSupported;
}

DisplayError HWPeripheralDRM::UpdateTransferTime(uint32_t transfer_time) {
  DisplayError error = HWDeviceDRM::UpdateTransferTime(transfer_time);

  return error;
}

DisplayError HWPeripheralDRM::Validate(HWLayersInfo *hw_layers_info) {
  SetDestScalarData(*hw_layers_info);
  SetIdlePCState();
  SetSelfRefreshState();
  SetVMReqState();

  return HWDeviceDRM::Validate(hw_layers_info);
}

bool HWPeripheralDRM::IsCACEnabled(const HWLayersInfo *hw_layers_info) {
  if (hw_layers_info->hw_layers.size() == 0) {
    return false;
  }

  for (uint32_t i = 0; i < hw_layers_info->hw_layers.size(); i++) {
    // if any layer has tunnel pipes then CAC is enabled
    if (hw_layers_info->config[i].tunnel_pipes.size() > 0) {
      return true;
    }
  }

  return false;
}

DisplayError HWPeripheralDRM::UpdateLoopBackConnector() {
  if (hw_resource_.cac_version != kCacVersionLoopback) {
    return kErrorNone;
  }

  // Fake register to get the loopback connector
  sde_drm::DRMDisplayToken token = {};
  int ret = drm_mgr_intf_->RegisterDisplay(sde_drm::DRMDisplayType::VIRTUAL, &token,
                                           true /* loopback connector */);
  if (ret) {
    if (ret != -ENODEV) {
      DLOGE("Failed registering display %d. Error: %d.", sde_drm::DRMDisplayType::VIRTUAL, ret);
    }
    return kErrorResources;
  }

  loopback_conn_id_ = token.conn_id;
  drm_mgr_intf_->UnregisterDisplay(&token);

  return kErrorNone;
}

DisplayError HWPeripheralDRM::ConfigureLoopbackCAC(bool cac_enabled) {
  if (hw_resource_.cac_version != kCacVersionLoopback) {
    return kErrorNone;
  }

  if (loopback_conn_id_ == -1) {
    DLOGE("Invalid virtual connector Id!!");
    return kErrorParameters;
  }

  if (!cac_enabled && !loopback_cac_configured_) {
    return kErrorNone;
  }

  bool register_loopback = cac_enabled && !loopback_cac_configured_;
  if (register_loopback) {
    int ret = drm_mgr_intf_->RegisterDisplay(loopback_conn_id_, &loopback_token_);
    if (ret) {
      if (ret != -ENODEV) {
        DLOGE("Failed registering display %d. Error: %d.", sde_drm::DRMDisplayType::VIRTUAL, ret);
      }
      return kErrorResources;
    }
    loopback_cac_configured_ = true;
  }

  if (loopback_cac_configured_) {
    if (cac_enabled) {
      DLOGV_IF(kTagDriverConfig, "Configuring CAC loopback");
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, loopback_token_.conn_id,
                                token_.crtc_id);
    } else {
      DLOGV_IF(kTagDriverConfig, "Teardown CAC loopback");
      drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_CRTC, loopback_token_.conn_id, 0);
      drm_mgr_intf_->UnregisterDisplay(&loopback_token_);
      loopback_token_ = {};
      loopback_cac_configured_ = false;
    }
  }

  return kErrorNone;
}

DisplayError HWPeripheralDRM::Commit(HWLayersInfo *hw_layers_info) {
  SetDestScalarData(*hw_layers_info);

  int64_t cwb_fence_fd = -1;
  bool has_fence = SetupConcurrentWriteback(*hw_layers_info, false, &cwb_fence_fd);
  bool cac_enabled = IsCACEnabled(hw_layers_info);
  auto error = ConfigureLoopbackCAC(cac_enabled);
  if (error != kErrorNone) {
    DLOGE("Failed to configure CacLoopback!");
    return error;
  }

  SetIdlePCState();
  SetSelfRefreshState();
  SetVMReqState();

  if (first_cycle_) {
    SetDisplayMode(
        static_cast<HWDisplayMode>(connector_info_.modes[current_mode_index_].cur_panel_mode));
  }

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_EPT, token_.conn_id,
                            hw_layers_info->common_info->expected_present_time);

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FRAME_INTERVAL, token_.conn_id,
                            hw_layers_info->common_info->frame_interval);

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_USECASE_IDX, token_.conn_id,
                            hw_layers_info->flags.only_video_updating);

  error = HWDeviceDRM::Commit(hw_layers_info);
  shared_ptr<Fence> cwb_fence = Fence::Create(INT(cwb_fence_fd), "cwb_fence");
  if (error != kErrorNone) {
    return error;
  }

  if (has_fence) {
    hw_layers_info->output_buffer->release_fence = cwb_fence;
  }

  CacheDestScalarData();
  PostCommitConcurrentWriteback(hw_layers_info->output_buffer);

  // Initialize to default after successful commit
  synchronous_commit_ = false;
  active_ = true;

  if (pending_poms_switch_) {
    HWDeviceDRM::SetDisplayMode(kModeCommand);
    hw_panel_info_.bitclk_rates = bitclk_rates_;
    doze_poms_switch_done_ = true;
    pending_poms_switch_ = false;
  }

  idle_pc_state_ = sde_drm::DRMIdlePCState::NONE;
  // After commit, update the Self Refresh state
  if (self_refresh_state_ != kSelfRefreshNone) {
    if (self_refresh_state_ == kSelfRefreshReadAlloc) {
      self_refresh_state_ = kSelfRefreshDisableReadAlloc;
    } else if (self_refresh_state_ == kSelfRefreshDisableReadAlloc ||
               self_refresh_state_ == kSelfRefreshWriteAlloc) {
      self_refresh_state_ = kSelfRefreshNone;
    }
  }
  return error;
}

void HWPeripheralDRM::ResetDestScalarCache() {
  if (dest_scaler_blocks_used_ > 0) {
    for (uint32_t j = 0; j < scalar_data_.size(); j++) {
      dest_scalar_cache_[j] = {};
    }
  }

  if (ai_scaler_blocks_used_ > 0) {
    for (uint32_t j = 0; j < ai_scaler_cache_.size(); j++) {
      ai_scaler_cache_[j] = {};
    }
  }
}

void HWPeripheralDRM::SetDestScalarData(const HWLayersInfo &hw_layer_info) {
  if (dest_scaler_blocks_used_ > 0) {
    SetDestScalarData(hw_layer_info.dest_scale_info_map);
  }

  if (ai_scaler_blocks_used_ > 0) {
    SetAIScalerData(hw_layer_info.ai_scale_info_map);
  }
}

void HWPeripheralDRM::SetDestScalarData(const DestScaleInfoMap dest_scale_info_map) {
  if (!hw_scale_ || !dest_scaler_blocks_used_) {
    return;
  }

  for (uint32_t i = 0; i < dest_scaler_blocks_used_; i++) {
    auto it = dest_scale_info_map.find(i);

    if (it == dest_scale_info_map.end()) {
      continue;
    }

    HWDestScaleInfo *dest_scale_info = it->second;
    SDEScaler *scale = &scalar_data_[i];
    hw_scale_->SetScaler(dest_scale_info->scale_data, scale);

    sde_drm_dest_scaler_cfg *dest_scalar_data = &sde_dest_scalar_data_.ds_cfg[i];
    dest_scalar_data->flags = 0;
    if (scale->scaler_v2.enable) {
      dest_scalar_data->flags |= SDE_DRM_DESTSCALER_ENABLE;
    }
    if (scale->scaler_v2.de.enable) {
      dest_scalar_data->flags |= SDE_DRM_DESTSCALER_ENHANCER_UPDATE;
    }
    if (dest_scale_info->scale_update) {
      dest_scalar_data->flags |= SDE_DRM_DESTSCALER_SCALE_UPDATE;
    }
    if (hw_panel_info_.partial_update) {
      dest_scalar_data->flags |= SDE_DRM_DESTSCALER_PU_ENABLE;
    }
    dest_scalar_data->index = i;
    dest_scalar_data->lm_width = dest_scale_info->mixer_width;
    dest_scalar_data->lm_height = dest_scale_info->mixer_height;
    dest_scalar_data->scaler_cfg = reinterpret_cast<uint64_t>(&scale->scaler_v2);

    if (std::memcmp(&dest_scalar_cache_[i].scalar_data, scale, sizeof(SDEScaler)) ||
        dest_scalar_cache_[i].flags != dest_scalar_data->flags) {
      needs_ds_update_ = true;
    }
  }

  if (needs_ds_update_) {
    sde_dest_scalar_data_.num_dest_scaler = UINT32(dest_scale_info_map.size());
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_DEST_SCALER_CONFIG, token_.crtc_id,
                              reinterpret_cast<uint64_t>(&sde_dest_scalar_data_));
  }
}

void HWPeripheralDRM::SetAIScalerData(const AIScalerInfoMap ai_scale_info_map) {
  if (!ai_scaler_blocks_used_) {
    return;
  }

  for (uint32_t i = 0; i < ai_scaler_blocks_used_; i++) {
    auto it = ai_scale_info_map.find(i);

    if (it == ai_scale_info_map.end()) {
      return;
    }

    HWAIScalerInfo *ai_scale_info = it->second;
    struct drm_msm_ai_scaler *ai_scaler_cfg = &sde_ai_scaler_cfg_;

    // Update UAPI structure for AI Scaler config
    ai_scaler_cfg->config = ai_scale_info->ai_scale_data.config;
    ai_scaler_cfg->src_w = ai_scale_info->ai_scale_data.src_w;
    ai_scaler_cfg->src_h = ai_scale_info->ai_scale_data.src_h;
    ai_scaler_cfg->dst_w = ai_scale_info->ai_scale_data.dst_w;
    ai_scaler_cfg->dst_h = ai_scale_info->ai_scale_data.dst_h;
    if (ai_scale_info->ai_scale_data.is_param_valid) {
      memcpy(ai_scaler_cfg->param, ai_scale_info->ai_scale_data.param,
             AIQE_AI_SCALER_PARAM_LEN * sizeof(ai_scaler_cfg->param[0]));
    }

    if (ai_scaler_cache_[i].scaler_data.config != sde_ai_scaler_cfg_.config) {
      needs_ai_scaler_update_ = true;
    }
  }

  // Set Panel Feature for AI Scaler Config
  if (needs_ai_scaler_update_) {
    PanelFeaturePropertyInfo payload{};
    int rc;
    payload.prop_id = kPanelFeatureAIScalerCfg;

    if (sde_ai_scaler_cfg_.config) {
      payload.prop_ptr = reinterpret_cast<uint64_t>(&sde_ai_scaler_cfg_);
    } else {
      // Disable AI Scaler case
      payload.prop_ptr = reinterpret_cast<uint64_t>(nullptr);
    }
    payload.prop_size = sizeof(sde_ai_scaler_cfg_);
    payload.version = 1;

    rc = SetPanelFeature(payload);
    if (rc) {
      DLOGE("failed to set kPanelFeatureAIScalerCfg rc %d", rc);
    }
  }
}

void HWPeripheralDRM::CacheDestScalarData() {
  if ((dest_scaler_blocks_used_ > 0) && needs_ds_update_) {
    // Cache the destination scalar data during commit
    for (uint32_t i = 0; i < sde_dest_scalar_data_.num_dest_scaler; i++) {
      dest_scalar_cache_[i].flags = sde_dest_scalar_data_.ds_cfg[i].flags;
      dest_scalar_cache_[i].scalar_data = scalar_data_[i];
    }
    needs_ds_update_ = false;
  }

  if ((ai_scaler_blocks_used_ > 0) && needs_ai_scaler_update_) {
    // Cache the AI Scaler data during commit
    for (uint32_t i = 0; i < ai_scaler_cache_.size(); i++) {
      ai_scaler_cache_[i].scaler_data = sde_ai_scaler_cfg_;
    }
    needs_ai_scaler_update_ = false;
  }
}

void HWPeripheralDRM::SetSelfRefreshState() {
  if (self_refresh_state_ != kSelfRefreshNone) {
    if (self_refresh_state_ == kSelfRefreshReadAlloc) {
      drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_CACHE_STATE, token_.crtc_id,
                                sde_drm::DRMCacheState::ENABLED);
    } else if (self_refresh_state_ == kSelfRefreshWriteAlloc) {
      drm_atomic_intf_->Perform(sde_drm::DRMOps::CONNECTOR_SET_CACHE_STATE,
                                cwb_config_[core_id_].token.conn_id, sde_drm::DRMCacheWBState::ENABLED);
    } else if (self_refresh_state_ == kSelfRefreshDisableReadAlloc) {
      drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_CACHE_STATE, token_.crtc_id,
                                sde_drm::DRMCacheState::DISABLED);
    }
  }
}

DisplayError HWPeripheralDRM::Flush(HWLayersInfo *hw_layers_info) {
  DisplayError err = HWDeviceDRM::Flush(hw_layers_info);
  if (err != kErrorNone) {
    return err;
  }

  ResetDestScalarCache();
  return kErrorNone;
}

DisplayError HWPeripheralDRM::SetDppsFeature(void *payload, size_t size) {
  uint32_t obj_id = 0, object_type = 0, feature_id = 0;
  uint64_t value = 0;

  if (size != sizeof(DppsFeaturePayload)) {
    DLOGE("invalid payload size %zu, expected %zu", size, sizeof(DppsFeaturePayload));
    return kErrorParameters;
  }

  DppsFeaturePayload *feature_payload = reinterpret_cast<DppsFeaturePayload *>(payload);
  object_type = feature_payload->object_type;
  feature_id = feature_payload->feature_id;
  value = feature_payload->value;

  if (feature_id == sde_drm::kFeatureAd4Roi) {
    if (feature_payload->value) {
      DisplayDppsAd4RoiCfg *params = reinterpret_cast<DisplayDppsAd4RoiCfg *>
                                                      (feature_payload->value);
      if (!params) {
        DLOGE("invalid playload value %" PRIu64, feature_payload->value);
        return kErrorNotSupported;
      }

      ad4_roi_cfg_.h_x = params->h_start;
      ad4_roi_cfg_.h_y = params->h_end;
      ad4_roi_cfg_.v_x = params->v_start;
      ad4_roi_cfg_.v_y = params->v_end;
      ad4_roi_cfg_.factor_in = params->factor_in;
      ad4_roi_cfg_.factor_out = params->factor_out;

      value = (uint64_t)&ad4_roi_cfg_;
    }
  }

  if (feature_id == sde_drm::kFeatureLtmHistCtrl)
    ltm_hist_en_ = value;

  if (feature_id == sde_drm::kFeatureAbaHistCtrl)
    aba_hist_en_ = value;

  if (object_type == DRM_MODE_OBJECT_CRTC) {
    obj_id = token_.crtc_id;
  } else if (object_type == DRM_MODE_OBJECT_CONNECTOR) {
    obj_id = token_.conn_id;
  } else {
    DLOGE("invalid object type 0x%x", object_type);
    return kErrorUndefined;
  }

  drm_atomic_intf_->Perform(DRMOps::DPPS_CACHE_FEATURE, obj_id, feature_id, value);
  return kErrorNone;
}

DisplayError HWPeripheralDRM::GetDppsFeatureInfo(void *payload, size_t size) {
  if (size != sizeof(DRMDppsFeatureInfo)) {
    DLOGE("invalid payload size %zu, expected %zu", size, sizeof(DRMDppsFeatureInfo));
    return kErrorParameters;
  }
  DRMDppsFeatureInfo *feature_info = reinterpret_cast<DRMDppsFeatureInfo *>(payload);
  feature_info->obj_id = token_.crtc_id;
  drm_mgr_intf_->GetDppsFeatureInfo(feature_info);
  return kErrorNone;
}

DisplayError HWPeripheralDRM::HandleSecureEvent(SecureEvent secure_event,
                                                const HWQosData &qos_data) {
  switch (secure_event) {
    case kTUITransitionPrepare:
    case kTUITransitionUnPrepare:
      if (tui_state_ == kTUIStateNone) {
        tui_state_ = kTUIStateInProgress;
      }
      break;
    case kTUITransitionStart: {
      if (tui_state_ == kTUIStateNone) {
        tui_state_ = kTUIStateStart;
      }
      ControlIdlePowerCollapse(false /* enable */, false /* synchronous */);
      if (hw_panel_info_.mode != kModeCommand) {
        SetQOSData(qos_data);
        SetVMReqState();
        SetIdlePCState();
        DisplayError err = Flush(NULL);
        if (err != kErrorNone) {
          return err;
        }
        SetTUIState();
      }
    }
    break;

    case kTUITransitionEnd: {
      if (tui_state_ == kTUIStateInProgress) {
        tui_state_ = kTUIStateEnd;
      } else {
        tui_state_ = kTUIStateNone;
      }
      ResetPropertyCache();
      ControlIdlePowerCollapse(true /* enable */, false /* synchronous */);
      if (hw_panel_info_.mode != kModeCommand || pending_power_state_ == kPowerStateOff) {
        SetQOSData(qos_data);
        SetVMReqState();
        SetIdlePCState();
        DisplayError err = Flush(NULL);
        if (err != kErrorNone) {
          return err;
        }
        SetTUIState();
      }
    }
    break;

    case kSecureDisplayStart: {
      secure_display_active_ = true;
      if (hw_panel_info_.mode != kModeCommand) {
        DisplayError err = Flush(NULL);
        if (err != kErrorNone) {
          return err;
        }
      }
    }
    break;

    case kSecureDisplayEnd: {
      if (hw_panel_info_.mode != kModeCommand) {
        DisplayError err = Flush(NULL);
        if (err != kErrorNone) {
          return err;
        }
      }
      secure_display_active_ = false;
      synchronous_commit_ = true;
    }
    break;

    default:
      DLOGE("Invalid secure event %d", secure_event);
      return kErrorNotSupported;
  }

  return kErrorNone;
}

DisplayError HWPeripheralDRM::ControlIdlePowerCollapse(bool enable, bool synchronous) {
  if (enable == idle_pc_enabled_) {
    return kErrorNone;
  }
  idle_pc_state_ = enable ? sde_drm::DRMIdlePCState::ENABLE : sde_drm::DRMIdlePCState::DISABLE;
  idle_pc_enabled_ = enable;
  return kErrorNone;
}

DisplayError HWPeripheralDRM::PowerOn(const HWQosData &qos_data, SyncPoints *sync_points) {
  DTRACE_SCOPED();
  if (tui_state_ != kTUIStateNone || pending_cwb_teardown_) {
    DLOGI("Request deferred TUI state %d pending cwb teardown %d", tui_state_,
          pending_cwb_teardown_);
    pending_power_state_ = kPowerStateOn;
    return kErrorDeferred;
  }

  if (!drm_atomic_intf_) {
    DLOGE("DRM Atomic Interface is null!");
    return kErrorUndefined;
  }

  if (first_cycle_ || tui_state_ != kTUIStateNone) {
    DLOGI("Request deferred TUI state %d", tui_state_);
    pending_power_state_ = kPowerStateOn;
    return kErrorDeferred;
  }
  SetVMReqState();

  if (switch_mode_valid_ && doze_poms_switch_done_ && (current_mode_index_ == cmd_mode_index_)) {
    HWDeviceDRM::SetDisplayMode(kModeVideo);
    hw_panel_info_.bitclk_rates = bitclk_rates_;
    doze_poms_switch_done_ = false;
  }

  if (!idle_pc_enabled_) {
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_IDLE_PC_STATE, token_.crtc_id,
                              sde_drm::DRMIdlePCState::ENABLE);
  }

  if (sde_dest_scalar_data_.num_dest_scaler) {
    for (uint32_t i = 0; i < dest_scaler_blocks_used_; i++) {
      sde_drm_dest_scaler_cfg *dest_scalar_data = &sde_dest_scalar_data_.ds_cfg[i];
      if (dest_scalar_data->flags & SDE_DRM_DESTSCALER_ENABLE) {
        dest_scalar_data->flags |= SDE_DRM_DESTSCALER_SCALE_UPDATE;
      }
    }
    drm_atomic_intf_->Perform(DRMOps::CRTC_SET_DEST_SCALER_CONFIG, token_.crtc_id,
                              reinterpret_cast<uint64_t>(&sde_dest_scalar_data_));
    needs_ds_update_ = true;
  }

  if (ai_scaler_blocks_used_ && sde_ai_scaler_cfg_.config) {
    PanelFeaturePropertyInfo payload{};
    int rc;
    payload.prop_id = kPanelFeatureAIScalerCfg;
    payload.prop_ptr = reinterpret_cast<uint64_t>(&sde_ai_scaler_cfg_);
    payload.prop_size = sizeof(sde_ai_scaler_cfg_);

    rc = SetPanelFeature(payload);
    if (rc) {
      DLOGE("failed to set kPanelFeatureAIScalerCfg rc %d", rc);
    }
    needs_ai_scaler_update_ = true;
  }

  DisplayError err = HWDeviceDRM::PowerOn(qos_data, sync_points);
  if (err != kErrorNone) {
    return err;
  }
  idle_pc_state_ = sde_drm::DRMIdlePCState::NONE;
  idle_pc_enabled_ = true;
  pending_poms_switch_ = false;
  active_ = true;
  SetTUIState();

  CacheDestScalarData();

  return kErrorNone;
}

DisplayError HWPeripheralDRM::PowerOff(bool teardown, SyncPoints *sync_points) {
  DTRACE_SCOPED();
  if ((tui_state_ != kTUIStateNone && tui_state_ != kTUIStateEnd) || pending_cwb_teardown_) {
    DLOGI("Request deferred TUI state %d pending cwb teardown %d", tui_state_,
          pending_cwb_teardown_);
    pending_power_state_ = kPowerStateOff;
    return kErrorDeferred;
  }
  if (!first_cycle_) {
    drm_mgr_intf_->MarkPanelFeatureForNullCommit(token_,
                                           panel_feature_property_map_[kPanelFeatureDemuraInitCfg]);
    drm_mgr_intf_->MarkPanelFeatureForNullCommit(token_,
                                                 panel_feature_property_map_[kPanelFeatureABCCfg]);
  }
  SetVMReqState();
  DisplayError err = kErrorNone;
  if (secure_display_active_) {
    err = Flush(NULL);
    if (err != kErrorNone) {
      return err;
    }
  }

  // QSync mode needs to be reset on device suspend and re-enabled on resume.
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_QSYNC_MODE, token_.conn_id,
                            sde_drm::DRMQsyncMode::NONE);
  ConfigureLoopbackCAC(false /* cac enabled */);

  err = HWDeviceDRM::PowerOff(teardown, sync_points);
  if (err != kErrorNone) {
    return err;
  }

  pending_poms_switch_ = false;
  active_ = false;
  SetTUIState();

  return kErrorNone;
}

DisplayError HWPeripheralDRM::Doze(const HWQosData &qos_data, SyncPoints *sync_points) {
  DTRACE_SCOPED();
  SetVMReqState();

  if (!first_cycle_ && switch_mode_valid_ && !doze_poms_switch_done_ &&
    (current_mode_index_ == video_mode_index_)) {
    if (active_) {
      HWDeviceDRM::SetDisplayMode(kModeCommand);
      hw_panel_info_.bitclk_rates = bitclk_rates_;
      doze_poms_switch_done_ = true;
    } else {
      pending_poms_switch_ = true;
    }
  }
  DisplayError err = HWDeviceDRM::Doze(qos_data, sync_points);
  if (err != kErrorNone) {
    return err;
  }

  active_ = true;

  SetTUIState();
  return kErrorNone;
}

DisplayError HWPeripheralDRM::DozeSuspend(const HWQosData &qos_data, SyncPoints *sync_points) {
  SetVMReqState();

  if (switch_mode_valid_ && !doze_poms_switch_done_ &&
    (current_mode_index_ == video_mode_index_)) {
    HWDeviceDRM::SetDisplayMode(kModeCommand);
    hw_panel_info_.bitclk_rates = bitclk_rates_;
    doze_poms_switch_done_ = true;
  }

  DisplayError err = HWDeviceDRM::DozeSuspend(qos_data, sync_points);
  if (err != kErrorNone) {
    return err;
  }

  pending_poms_switch_ = false;
  active_ = true;

  SetTUIState();
  return kErrorNone;
}

DisplayError HWPeripheralDRM::SetDisplayAttributes(uint32_t index) {
  if (doze_poms_switch_done_ || pending_poms_switch_ || bit_clk_rate_) {
    DLOGW("Bailing. Pending operations: doze_poms_switch_done_=%d, pending_poms_switch_=%d,"
     "bit_clk_rate_=%d", doze_poms_switch_done_, pending_poms_switch_, bit_clk_rate_);
    return kErrorDeferred;
  }

  HWDeviceDRM::SetDisplayAttributes(index);
  // update bit clk rates.
  hw_panel_info_.bitclk_rates = bitclk_rates_;

  return kErrorNone;
}

DisplayError HWPeripheralDRM::SetDisplayDppsAdROI(void *payload) {
  DisplayError err = kErrorNone;
  struct sde_drm::DppsFeaturePayload feature_payload = {};

  if (!payload) {
    DLOGE("Invalid payload parameter");
    return kErrorParameters;
  }

  feature_payload.object_type = DRM_MODE_OBJECT_CRTC;
  feature_payload.feature_id = sde_drm::kFeatureAd4Roi;
  feature_payload.value = (uint64_t)(payload);

  err = SetDppsFeature(&feature_payload, sizeof(feature_payload));
  if (err != kErrorNone) {
    DLOGE("Faid to SetDppsFeature feature_id = %d, err = %d",
           sde_drm::kFeatureAd4Roi, err);
  }

  return err;
}

DisplayError HWPeripheralDRM::SetFrameTrigger(FrameTriggerMode mode) {
  sde_drm::DRMFrameTriggerMode drm_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_DEFAULT;
  switch (mode) {
  case kFrameTriggerDefault:
    drm_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_DEFAULT;
    break;
  case kFrameTriggerSerialize:
    drm_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_SERIALIZE;
    break;
  case kFrameTriggerPostedStart:
    drm_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_POSTED_START;
    break;
  default:
    DLOGE("Invalid frame trigger mode %d", (int32_t)mode);
    return kErrorParameters;
  }

  int ret = drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FRAME_TRIGGER,
                                      token_.conn_id, drm_mode);
  if (ret) {
    DLOGE("Failed to perform CONNECTOR_SET_FRAME_TRIGGER, drm_mode %d, ret %d", drm_mode, ret);
    return kErrorUndefined;
  }
  return kErrorNone;
}

DisplayError HWPeripheralDRM::SetPanelBrightness(int level) {
  DTRACE_SCOPED();
  if (pending_power_state_ != kPowerStateNone) {
    DLOGI("Power state %d pending!! Skip for now", pending_power_state_);
    return kErrorDeferred;
  }

#ifdef TRUSTED_VM
  if (first_cycle_) {
    DLOGI("First cycle is not done yet!! Skip for now");
    return kErrorDeferred;
  }
#endif

  if (!active_) {
#ifdef SEC_GC_QC_DEBUG
    DLOGI("active_ is false");
#endif
    return kErrorNone;
  }

#ifdef SEC_GC_QC_SUPERHDR
  bool isSmoothDimOn = GetSmoothDimOn();

  if (max_brightness_expanded_ && isSmoothDimOn) {
    if (level != 0 && brightness_set_ != 0 && (level / 10 == brightness_set_ / 10)) {
      DLOGI("brightness level is same as brightness_set_ in SDR(%d, %d), (%d, %d)",
             level, brightness_set_, level / 10, brightness_set_ / 10);
      return kErrorNone;
    }
  }

  if (enable_brightness_drm_prop_) {
      if (!isSmoothDimOn || cached_brightness_level_ != -1) {
      // set brightness through drm property
      DLOGI("Setting brightness to level %d through drm property. primary(%d), type_id(%d), cached_brightness_level_(%d)",
            level, connector_info_.is_primary, connector_info_.type_id, cached_brightness_level_);

      brightness_set_ = level;
      cached_brightness_level_ = level;

      return kErrorNone;
    }
  }
#else
  if (enable_brightness_drm_prop_) {
    // set brightness through drm property
    cached_brightness_level_ = level;
    return kErrorNone;
  }
  #endif

  // set brightness through sysfs node
  char buffer[kMaxSysfsCommandLength] = {0};

  if (brightness_base_path_.empty()) {
#ifdef SEC_GC_QC_DEBUG
    DLOGW("brightness_base_path_ is empty");
#endif
    return kErrorHardware;
  }

  std::string brightness_node(brightness_base_path_ + "brightness");
  int fd = Sys::open_(brightness_node.c_str(), O_RDWR);
  if (fd < 0) {
    if (connector_info_.backlight_type != "dcs") {
      DLOGW("Failed to open node = %s, error = %s ", brightness_node.c_str(),
          strerror(errno));
      return kErrorFileDescriptor;
    } else {
    DLOGE("Failed to open node = %s, error = %s ", brightness_node.c_str(),
          strerror(errno));
    return kErrorFileDescriptor;
    }
  }

  int32_t bytes = snprintf(buffer, kMaxSysfsCommandLength, "%d\n", level);
  ssize_t ret = Sys::pwrite_(fd, buffer, static_cast<size_t>(bytes), 0);
  if (ret <= 0) {
    DLOGE("Failed to write to node = %s, error = %s ", brightness_node.c_str(),
          strerror(errno));
    Sys::close_(fd);
    return kErrorHardware;
  }

#ifdef SEC_GC_QC_DEBUG
    DLOGI("Setting brightness to level %d through sysfs node. primary(%d), type_id(%d)",
          level, connector_info_.is_primary, connector_info_.type_id);
#endif

#ifdef SEC_GC_QC_SUPERHDR
  brightness_set_ = level;
#endif

  Sys::close_(fd);

  return kErrorNone;
}

DisplayError HWPeripheralDRM::GetPanelBrightness(int *level) {
  DTRACE_SCOPED();
  char value[kMaxStringLength] = {0};

  if (!level) {
    DLOGE("Invalid input, null pointer.");
    return kErrorParameters;
  }

#ifdef SEC_GC_QC_SUPERHDR
  bool isSmoothDimOn = GetSmoothDimOn();

  if (enable_brightness_drm_prop_ && !isSmoothDimOn)
#else
  if (enable_brightness_drm_prop_)
#endif
  {
    *level = current_brightness_;
    return kErrorNone;
  }

  if (brightness_base_path_.empty()) {
    return kErrorHardware;
  }

  std::string brightness_node(brightness_base_path_ + "brightness");
  int fd = Sys::open_(brightness_node.c_str(), O_RDWR);
  if (fd < 0) {
    if (connector_info_.backlight_type != "dcs") {
      DLOGW("Failed to open brightness node = %s, error = %s", brightness_node.c_str(),
             strerror(errno));
      return kErrorFileDescriptor;
    } else {
    DLOGE("Failed to open brightness node = %s, error = %s", brightness_node.c_str(),
           strerror(errno));
    return kErrorFileDescriptor;
    }
  }

  if (Sys::pread_(fd, value, sizeof(value), 0) > 0) {
    *level = atoi(value);
  } else {
    DLOGE("Failed to read panel brightness");
    Sys::close_(fd);
    return kErrorHardware;
  }

  Sys::close_(fd);

  return kErrorNone;
}

void HWPeripheralDRM::GetHWPanelMaxBrightness() {
  DTRACE_SCOPED();
  char value[kMaxStringLength] = {0};
  hw_panel_info_.panel_max_brightness = 255.0f;

  // Panel nodes, driver connector creation, and DSI probing all occur in sync, for each DSI. This
  // means that the connector_type_id - 1 will reflect the same # as the panel # for panel node.
  char s[kMaxStringLength] = {};
  snprintf(s, sizeof(s), "/sys/class/backlight/panel%d-backlight/",
           static_cast<int>(connector_info_.type_id - 1));
  brightness_base_path_.assign(s);

  std::string brightness_node(brightness_base_path_ + "max_brightness");
  int fd = Sys::open_(brightness_node.c_str(), O_RDONLY);
  if (fd < 0) {
    if (connector_info_.backlight_type != "dcs") {
    DLOGW("Failed to open max brightness node = %s, error = %s", brightness_node.c_str(),
          strerror(errno));
    return;
  } else {
    DLOGE("Failed to open max brightness node = %s, error = %s", brightness_node.c_str(),
          strerror(errno));
    return;
    }
  }

  if (Sys::pread_(fd, value, sizeof(value), 0) > 0) {
    hw_panel_info_.panel_max_brightness = static_cast<float>(atof(value));
    DLOGI_IF(kTagDriverConfig, "Max brightness = %f", hw_panel_info_.panel_max_brightness);
#ifdef SEC_GC_QC_SUPERHDR
    if (hw_panel_info_.panel_max_brightness >= 10000.0f) {
      max_brightness_expanded_ = true;
    }
#endif
  } else {
    DLOGE("Failed to read max brightness. error = %s", strerror(errno));
  }

  Sys::close_(fd);
  return;
}

DisplayError HWPeripheralDRM::SetBLScale(uint32_t level) {
  int ret = drm_atomic_intf_->Perform(DRMOps::DPPS_CACHE_FEATURE,
              token_.conn_id, sde_drm::kFeatureSvBlScale, level);
  if (ret) {
    DLOGE("Failed to set backlight scale level %d, ret %d", level, ret);
    return kErrorUndefined;
  }
  return kErrorNone;
}

DisplayError HWPeripheralDRM::GetPanelBrightnessBasePath(std::string *base_path) const {
  if (!base_path) {
    DLOGE("Invalid base_path is null pointer");
    return kErrorParameters;
  }

  if (brightness_base_path_.empty()) {
    DLOGE("brightness_base_path_ is empty");
    return kErrorHardware;
  }

  *base_path = brightness_base_path_;
  return kErrorNone;
}

DisplayError HWPeripheralDRM::EnableSelfRefresh(SelfRefreshState self_refresh_state) {
  if (self_refresh_state != kSelfRefreshNone) {
    self_refresh_state_ = self_refresh_state;
  }
  return kErrorNone;
}

void HWPeripheralDRM::ResetPropertyCache() {
  drm_atomic_intf_->Perform(sde_drm::DRMOps::PLANES_RESET_CACHE, token_.crtc_id);
  drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_RESET_CACHE, token_.crtc_id);
}

void HWPeripheralDRM::CreatePanelFeaturePropertyMap() {
  panel_feature_property_map_.clear();
  panel_feature_property_map_[kPanelFeatureSPRInitCfg] = sde_drm::kDRMPanelFeatureSPRInit;
  panel_feature_property_map_[kPanelFeatureSPRPackType] = sde_drm::kDRMPanelFeatureSPRPackType;
  panel_feature_property_map_[kPanelFeatureSPRPackTypeMode] =
      sde_drm::kDRMPanelFeatureSPRPackTypeMode;
  panel_feature_property_map_[kPanelFeatureDemuraInitCfg] = sde_drm::kDRMPanelFeatureDemuraInit;
  panel_feature_property_map_[kPanelFeatureDsppIndex] = sde_drm::kDRMPanelFeatureDsppIndex;
  panel_feature_property_map_[kPanelFeatureDsppSPRInfo] = sde_drm::kDRMPanelFeatureDsppSPRInfo;
  panel_feature_property_map_[kPanelFeatureDsppRCInfo] = sde_drm::kDRMPanelFeatureDsppRCInfo;
  panel_feature_property_map_[kPanelFeatureDsppDemuraInfo] =
    sde_drm::kDRMPanelFeatureDsppDemuraInfo;
  panel_feature_property_map_[kPanelFeatureRCInitCfg] = sde_drm::kDRMPanelFeatureRCInit;
  panel_feature_property_map_[kPanelFeatureDemuraPanelId] = sde_drm::kDRMPanelFeaturePanelId;
  panel_feature_property_map_[kPanelFeatureSPRUDCCfg] = sde_drm::kDRMPanelFeatureSPRUDC;
  panel_feature_property_map_[kPanelFeatureDemuraCfg0Param2] =
      sde_drm::kDRMPanelFeatureDemuraCfg0Param2;
  panel_feature_property_map_[kPanelFeatureAiqeSsrcConfig] =
      sde_drm::kDRMPanelFeatureAiqeSSRCConfig;
  panel_feature_property_map_[kPanelFeatureAiqeSsrcData] = sde_drm::kDRMPanelFeatureAiqeSSRCData;
  panel_feature_property_map_[kPanelFeatureAIScalerCfg] = sde_drm::kDRMPanelFeatureAIScalerCfg;
  panel_feature_property_map_[kPanelFeatureAiqeMdnie] = sde_drm::kDRMPanelFeatureAiqeMdnie;
  panel_feature_property_map_[kPanelFeatureAiqeMdnieArt] = sde_drm::kDRMPanelFeatureAiqeMdnieArt;
  panel_feature_property_map_[kPanelFeatureAiqeMdnieIPC] = sde_drm::kDRMPanelFeatureAiqeMdnieIPC;
  panel_feature_property_map_[kPanelFeatureAiqeCopr] = sde_drm::kDRMPanelFeatureAiqeCopr;
  panel_feature_property_map_[kPanelFeatureABCCfg] = sde_drm::kDRMPanelFeatureABC;
  panel_feature_property_map_[kPanelFeatureDemuraBacklight] =
      sde_drm::kDRMPanelFeatureDemuraBacklight;
}

int HWPeripheralDRM::GetPanelFeature(PanelFeaturePropertyInfo *feature_info) {
  int ret = 0;
  DRMPanelFeatureInfo drm_feature = {};

  if (!feature_info) {
    DLOGE("Invalid object pointer of PanelFeaturePropertyInfo");
    return -EINVAL;
  }

  auto it = panel_feature_property_map_.find(feature_info->prop_id);
  if (it ==  panel_feature_property_map_.end()) {
    DLOGE("Failed to find prop-map entry for id %d", feature_info->prop_id);
    return -EINVAL;
  }

  drm_feature.prop_id = panel_feature_property_map_[feature_info->prop_id];
  drm_feature.prop_ptr = feature_info->prop_ptr;
  drm_feature.prop_size = feature_info->prop_size;

  switch (feature_info->prop_id) {
    case kPanelFeatureSPRInitCfg:
    case kPanelFeatureDemuraInitCfg:
    case kPanelFeatureDsppIndex:
    case kPanelFeatureDsppSPRInfo:
    case kPanelFeatureDsppDemuraInfo:
    case kPanelFeatureDsppRCInfo:
    case kPanelFeatureRCInitCfg:
    case kPanelFeatureSPRUDCCfg:
    case kPanelFeatureDemuraCfg0Param2:
    case kPanelFeatureAiqeSsrcConfig:
    case kPanelFeatureAiqeSsrcData:
    case kPanelFeatureAiqeMdnie:
    case kPanelFeatureAiqeMdnieArt:
    case kPanelFeatureAiqeMdnieIPC:
    case kPanelFeatureAiqeCopr:
    case kPanelFeatureABCCfg:
    case kPanelFeatureDemuraBacklight:
      drm_feature.obj_type = DRM_MODE_OBJECT_CRTC;
      drm_feature.obj_id = token_.crtc_id;
      break;
    case kPanelFeatureSPRPackType:
    case kPanelFeatureSPRPackTypeMode:
    case kPanelFeatureDemuraPanelId:
      drm_feature.obj_type = DRM_MODE_OBJECT_CONNECTOR;
      drm_feature.obj_id =  token_.conn_id;
      break;
    default:
      DLOGE("obj id population for property %d not implemented", feature_info->prop_id);
      return -EINVAL;
  }

  drm_mgr_intf_->GetPanelFeature(&drm_feature);

  feature_info->version = drm_feature.version;
  feature_info->prop_size = drm_feature.prop_size;

  return ret;
}

int HWPeripheralDRM::SetPanelFeature(const PanelFeaturePropertyInfo &feature_info) {
  int ret = 0;
  DRMPanelFeatureInfo drm_feature = {};
  drm_feature.prop_id = panel_feature_property_map_[feature_info.prop_id];
  drm_feature.prop_ptr = feature_info.prop_ptr;
  drm_feature.version = feature_info.version;
  drm_feature.prop_size = feature_info.prop_size;

  switch (feature_info.prop_id) {
    case kPanelFeatureSPRInitCfg:
    case kPanelFeatureRCInitCfg:
    case kPanelFeatureDemuraInitCfg:
    case kPanelFeatureSPRUDCCfg:
    case kPanelFeatureDemuraCfg0Param2:
    case kPanelFeatureAiqeSsrcConfig:
    case kPanelFeatureAiqeSsrcData:
    case kPanelFeatureAIScalerCfg:
    case kPanelFeatureAiqeMdnie:
    case kPanelFeatureAiqeMdnieArt:
    case kPanelFeatureAiqeMdnieIPC:
    case kPanelFeatureAiqeCopr:
    case kPanelFeatureABCCfg:
    case kPanelFeatureDemuraBacklight:
      drm_feature.obj_type = DRM_MODE_OBJECT_CRTC;
      drm_feature.obj_id = token_.crtc_id;
      break;
    case kPanelFeatureSPRPackType:
    case kPanelFeatureSPRPackTypeMode:
      drm_feature.obj_type = DRM_MODE_OBJECT_CONNECTOR;
      drm_feature.obj_id =  token_.conn_id;
      break;
    default:
      DLOGE("Set Panel feature property %d not implemented", feature_info.prop_id);
      return -EINVAL;
  }

  DLOGI("Set Panel feature property %d", feature_info.prop_id);
  drm_mgr_intf_->SetPanelFeature(drm_feature);

  return ret;
}

void HWPeripheralDRM::SetVMReqState() {
  if (tui_state_ == kTUIStateStart) {
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_VM_REQ_STATE, token_.crtc_id,
                              sde_drm::DRMVMRequestState::RELEASE);
    DLOGI("Release resources to SVM");
    if (ltm_hist_en_)
      drm_atomic_intf_->Perform(sde_drm::DRMOps::DPPS_CACHE_FEATURE, token_.crtc_id,
                                sde_drm::kFeatureLtmHistCtrl, 0);
    if (aba_hist_en_)
      drm_atomic_intf_->Perform(sde_drm::DRMOps::DPPS_CACHE_FEATURE, token_.crtc_id,
                                sde_drm::kFeatureAbaHistCtrl, 0);
  } else if (tui_state_ == kTUIStateEnd) {
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_VM_REQ_STATE, token_.crtc_id,
                              sde_drm::DRMVMRequestState::ACQUIRE);
    DLOGI("Acquire resources from SVM");
    if (ltm_hist_en_)
      drm_atomic_intf_->Perform(sde_drm::DRMOps::DPPS_CACHE_FEATURE, token_.crtc_id,
                                sde_drm::kFeatureLtmHistCtrl, 1);
    if (aba_hist_en_)
      drm_atomic_intf_->Perform(sde_drm::DRMOps::DPPS_CACHE_FEATURE, token_.crtc_id,
                                sde_drm::kFeatureAbaHistCtrl, 1);
  } else if (tui_state_ == kTUIStateNone) {
    drm_atomic_intf_->Perform(sde_drm::DRMOps::CRTC_SET_VM_REQ_STATE, token_.crtc_id,
                              sde_drm::DRMVMRequestState::NONE);
  }
}

DisplayError HWPeripheralDRM::SetAlternateDisplayConfig(uint32_t *alt_config) {
  uint32_t curr_mode_flag = 0;
  sde_drm::DRMModeInfo current_mode = connector_info_.modes[current_mode_index_];
  sde_drm::DRMSubModeInfo sub_mode = current_mode.sub_modes[current_mode.curr_submode_index];
  uint32_t curr_compression = current_mode.curr_compression_mode;

  if (current_mode.cur_panel_mode & DRM_MODE_FLAG_CMD_MODE_PANEL) {
    curr_mode_flag = DRM_MODE_FLAG_CMD_MODE_PANEL;
  } else if (current_mode.cur_panel_mode & DRM_MODE_FLAG_VID_MODE_PANEL) {
    curr_mode_flag = DRM_MODE_FLAG_VID_MODE_PANEL;
  }

  // First try to perform compression mode switch within same mode
  for (uint32_t submode_idx = 0; submode_idx < current_mode.sub_modes.size(); submode_idx++) {
    if ((curr_compression != current_mode.sub_modes[submode_idx].panel_compression_mode)) {
      connector_info_.modes[current_mode_index_].curr_submode_index = submode_idx;
      connector_info_.modes[current_mode_index_].curr_compression_mode =
              current_mode.sub_modes[submode_idx].panel_compression_mode;
      SetTopology(connector_info_.modes[current_mode_index_].sub_modes[submode_idx].topology,
                  &display_attributes_[current_mode_index_].topology);
      SetDisplaySwitchMode(current_mode_index_);
      panel_compression_changed_ = current_mode.sub_modes[submode_idx].panel_compression_mode;
      *alt_config = current_mode_index_;
      return kErrorNone;
    }
  }

  // If there is no compression switch possible within current mode, try with other modes
  for (uint32_t mode_index = 0; mode_index < connector_info_.modes.size(); mode_index++) {
    if ((current_mode.mode.vrefresh == connector_info_.modes[mode_index].mode.vrefresh) &&
        (curr_mode_flag & connector_info_.modes[mode_index].cur_panel_mode)) {
      for (uint32_t submode_idx = 0; submode_idx <
           connector_info_.modes[mode_index].sub_modes.size(); submode_idx++) {
        if ((curr_compression !=
             connector_info_.modes[mode_index].sub_modes[submode_idx].panel_compression_mode)) {
          connector_info_.modes[mode_index].curr_submode_index = submode_idx;
          SetTopology(connector_info_.modes[mode_index].sub_modes[submode_idx].topology,
                      &display_attributes_[mode_index].topology);
          connector_info_.modes[mode_index].curr_compression_mode =
                connector_info_.modes[mode_index].sub_modes[submode_idx].panel_compression_mode;
          SetDisplayAttributes(mode_index);
          panel_compression_changed_ = connector_info_.modes[mode_index].curr_compression_mode;
          *alt_config = mode_index;
          return kErrorNone;
        }
      }
    }
  }

  return kErrorNotSupported;
}

DisplayError HWPeripheralDRM::GetQsyncFps(uint32_t *qsync_fps) {
  uint32_t qsync_min_fps = connector_info_.modes[current_mode_index_].qsync_min_fps;
  if (qsync_min_fps > 0) {
    *qsync_fps = qsync_min_fps;
    return kErrorNone;
  }

  return kErrorNotSupported;
}

bool HWPeripheralDRM::IsAVRStepSupported(uint32_t config_index) {
  uint32_t avr_step = connector_info_.modes[config_index].avr_step_fps;
  return (avr_step > 0);
}

bool HWPeripheralDRM::IsVRRSupported() {
  for (uint32_t i = 0; i < connector_info_.modes.size(); i++) {
    if (connector_info_.modes[i].avr_step_fps > 0) {
      return true;
    }
  }

  return false;
}

#ifdef SEC_GC_QC_SUPERHDR
#define SMOOTH_DIM_PATH_PANEL "/sys/class/lcd/panel/smooth_dim"
#define SMOOTH_DIM_PATH_PANEL1 "/sys/class/lcd/panel1/smooth_dim"
#define SMOOTH_DIM_LEN 4

bool HWPeripheralDRM::GetSmoothDimOn() {
  DTRACE_SCOPED();

  char data[SMOOTH_DIM_LEN]{};
  std::string path;
  int smoothDim_fd = 0;

  if (connector_info_.is_primary)
    path = SMOOTH_DIM_PATH_PANEL;
  else
    path = SMOOTH_DIM_PATH_PANEL1;

  smoothDim_fd = Sys::open_(path.c_str(), O_RDONLY);

  if (smoothDim_fd < 0) {
    DLOGW("Failed to open node = %s, error = %s ", path.c_str(), strerror(errno));
    return true;
  }

  if (Sys::read_(smoothDim_fd, data, SMOOTH_DIM_LEN) > 0) {
    if (atoi(data) == 1) {
      Sys::close_(smoothDim_fd);
      return true;
    }
  }

  Sys::close_(smoothDim_fd);

  return false;
}
#endif

}  // namespace sdm
