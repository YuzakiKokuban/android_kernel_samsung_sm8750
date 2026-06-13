/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "dpu_single_core.h"

#define __CLASS__ "DPUSingleCore"

namespace sdm {

DPUSingleCore::DPUSingleCore(DisplayId display_id, SDMDisplayType type,
                             MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                             BufferAllocator *buffer_allocator)
    : display_id_(display_id),
      type_(type),
      hw_info_intf_(hw_info_intf),
      buffer_allocator_(buffer_allocator) {}

DisplayError DPUSingleCore::Init() {
  auto it = hw_info_intf_.Begin();
  core_id_ = it->first;
  DisplayError error = HWInterface::Create(display_id_.GetConnId(core_id_), type_, it->second,
                                           buffer_allocator_, &hw_intf_);
  if (error != kErrorNone) {
    DLOGE("HW interface create failed");
  }

  return error;
}

DisplayError DPUSingleCore::Destroy() {
  HWInterface::Destroy(hw_intf_);
  return kErrorNone;
}

DisplayError DPUSingleCore::GetDisplayId(int32_t *display_id) {
  *display_id = 0;
  DisplayError error = kErrorNone;
  int32_t disp_id;
  error = hw_intf_->GetDisplayId(&disp_id);
  if (error != kErrorNone) {
    return error;
  }
  *display_id = *display_id | DisplayId(core_id_, disp_id).GetDisplayId();

  return error;
}

DisplayError DPUSingleCore::GetActiveConfig(uint32_t *active_config) {
  return hw_intf_->GetActiveConfig(active_config);
}

DisplayError DPUSingleCore::GetDefaultConfig(uint32_t *default_config) {
  return hw_intf_->GetDefaultConfig(default_config);
}

DisplayError DPUSingleCore::GetNumDisplayAttributes(uint32_t *count) {
  return hw_intf_->GetNumDisplayAttributes(count);
}

DisplayError DPUSingleCore::GetDisplayAttributes(uint32_t index, DisplayDeviceContext *device_ctx,
                                                 DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  Resize(device_ctx);

  error = hw_intf_->GetDisplayAttributes(index, &device_ctx->at(core_id_).display_attributes);
  if (error != kErrorNone) {
    return error;
  }

  client_ctx->display_attributes = device_ctx->at(core_id_).display_attributes;

  return kErrorNone;
}

DisplayError DPUSingleCore::GetHWPanelInfo(DisplayDeviceContext *device_ctx,
                                           DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  Resize(device_ctx);

  error = hw_intf_->GetHWPanelInfo(&device_ctx->at(core_id_).hw_panel_info);
  if (error != kErrorNone) {
    return error;
  }

  client_ctx->hw_panel_info = device_ctx->at(core_id_).hw_panel_info;

  return kErrorNone;
}

DisplayError DPUSingleCore::SetDisplayAttributes(uint32_t index) {
  return hw_intf_->SetDisplayAttributes(index);
}

DisplayError DPUSingleCore::SetDisplayAttributes(const HWDisplayAttributes &display_attributes) {
  return hw_intf_->SetDisplayAttributes(display_attributes);
}

DisplayError DPUSingleCore::GetConfigIndex(char *mode, uint32_t *index) {
  return hw_intf_->GetConfigIndex(mode, index);
}

DisplayError DPUSingleCore::PowerOn(std::map<uint32_t, HWQosData> &qos_data,
                                    SyncPoints *sync_points) {
  return hw_intf_->PowerOn(qos_data.at(core_id_), sync_points);
}

DisplayError DPUSingleCore::PowerOff(bool teardown, SyncPoints *sync_points) {
  return hw_intf_->PowerOff(teardown, sync_points);
}

DisplayError DPUSingleCore::Doze(std::map<uint32_t, HWQosData> &qos_data, SyncPoints *sync_points) {
  return hw_intf_->Doze(qos_data.at(core_id_), sync_points);
}

DisplayError DPUSingleCore::DozeSuspend(std::map<uint32_t, HWQosData> &qos_data,
                                        SyncPoints *sync_points) {
  return hw_intf_->DozeSuspend(qos_data.at(core_id_), sync_points);
}

DisplayError DPUSingleCore::Standby(SyncPoints *sync_points) {
  return hw_intf_->Standby(sync_points);
}

DisplayError DPUSingleCore::Validate(std::map<uint32_t, HWLayersInfo> &hw_layers_info) {
  return hw_intf_->Validate(&hw_layers_info.at(core_id_));
}

DisplayError DPUSingleCore::Commit(std::map<uint32_t, HWLayersInfo> &hw_layers_info) {
  DisplayError error = hw_intf_->Commit(&hw_layers_info.at(core_id_));
  if (error != kErrorNone) {
    return error;
  }

  hw_layers_info.at(core_id_).common_info->retire_fence = hw_layers_info.at(core_id_).retire_fence;
  hw_layers_info.at(core_id_).common_info->sync_handle = hw_layers_info.at(core_id_).sync_handle;

  for (auto &layers_info : hw_layers_info) {
    for (auto &layer : layers_info.second.hw_layers) {
      layer.input_buffer.release_fence = hw_layers_info[core_id_].sync_handle;
    }
  }

  // To-Do: Revisit update_mask
  hw_layers_info.at(core_id_).common_info->updates_mask = 0;
  return kErrorNone;
}

DisplayError DPUSingleCore::Flush(std::map<uint32_t, HWLayersInfo> &hw_layers_info) {
  return hw_intf_->Flush(&hw_layers_info.at(core_id_));
}

DisplayError DPUSingleCore::GetPPFeaturesVersion(PPFeatureVersion *vers, uint32_t core_id) {
  return hw_intf_->GetPPFeaturesVersion(vers);
}

DisplayError DPUSingleCore::SetPPFeature(PPFeatureInfo *feature, uint32_t &core_id) {
  DisplayError error = kErrorNone;

  if (!feature) {
    DLOGE("Invalid null feature!");
    return kErrorParameters;
  }

  if (core_id_ != core_id) {
    DLOGE("hw_intf is not present for core_id=%u", core_id);
    return kErrorNotSupported;
  }

  error = hw_intf_->SetPPFeature(feature);
  if (error != kErrorNone) {
    DLOGE("Failed to set pp feature for core_id=%u", core_id);
    return error;
  }

  return error;
}

DisplayError DPUSingleCore::SetVSyncState(bool enable) {
  return hw_intf_->SetVSyncState(enable);
}

void DPUSingleCore::SetIdleTimeoutMs(uint32_t timeout_ms) {
  hw_intf_->SetIdleTimeoutMs(timeout_ms);
}

DisplayError DPUSingleCore::SetDisplayMode(const HWDisplayMode hw_display_mode) {
  return hw_intf_->SetDisplayMode(hw_display_mode);
}

DisplayError DPUSingleCore::SetRefreshRate(uint32_t refresh_rate) {
  return hw_intf_->SetRefreshRate(refresh_rate);
}

DisplayError DPUSingleCore::SetPanelBrightness(int level) {
  return hw_intf_->SetPanelBrightness(level);
}

DisplayError DPUSingleCore::GetHWScanInfo(HWScanInfo *scan_info) {
  return hw_intf_->GetHWScanInfo(scan_info);
}

DisplayError DPUSingleCore::GetVideoFormat(uint32_t config_index, uint32_t *video_format) {
  return hw_intf_->GetVideoFormat(config_index, video_format);
}

DisplayError DPUSingleCore::GetMaxCEAFormat(uint32_t *max_cea_format) {
  return hw_intf_->GetMaxCEAFormat(max_cea_format);
}

DisplayError DPUSingleCore::SetCursorPosition(std::map<uint32_t, HWLayersInfo> &hw_layers_info,
                                              int x, int y) {
  return hw_intf_->SetCursorPosition(&hw_layers_info.at(core_id_), x, y);
}

DisplayError DPUSingleCore::OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level) {
  return hw_intf_->OnMinHdcpEncryptionLevelChange(min_enc_level);
}

DisplayError DPUSingleCore::GetPanelBrightness(int *level) {
  return hw_intf_->GetPanelBrightness(level);
}

DisplayError DPUSingleCore::SetAutoRefresh(bool enable) {
  return hw_intf_->SetAutoRefresh(enable);
}

DisplayError DPUSingleCore::SetScaleLutConfig(HWScaleLutInfo *lut_info) {
  return hw_intf_->SetScaleLutConfig(lut_info);
}

DisplayError DPUSingleCore::UnsetScaleLutConfig() {
  return hw_intf_->UnsetScaleLutConfig();
}

DisplayError DPUSingleCore::SetMixerAttributes(const HWMixerAttributes &mixer_attributes) {
  return hw_intf_->SetMixerAttributes(mixer_attributes);
}

DisplayError DPUSingleCore::GetMixerAttributes(DisplayDeviceContext *device_ctx,
                                               DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  Resize(device_ctx);

  error = hw_intf_->GetMixerAttributes(&device_ctx->at(core_id_).mixer_attributes);
  if (error != kErrorNone) {
    return error;
  }

  client_ctx->mixer_attributes = device_ctx->at(core_id_).mixer_attributes;

  return kErrorNone;
}

DisplayError DPUSingleCore::DumpDebugData() {
  return hw_intf_->DumpDebugData();
}

DisplayError DPUSingleCore::SetDppsFeature(void *payload, size_t size) {
  return hw_intf_->SetDppsFeature(payload, size);
}

DisplayError DPUSingleCore::SetPPConfig(void *payload, size_t size) {
  return hw_intf_->SetPPConfig(payload, size);
}

DisplayError DPUSingleCore::GetDppsFeatureInfo(void *payload, size_t size) {
  return hw_intf_->GetDppsFeatureInfo(payload, size);
}

DisplayError DPUSingleCore::HandleSecureEvent(SecureEvent secure_event,
                                              std::map<uint32_t, HWQosData> &qos_data) {
  return hw_intf_->HandleSecureEvent(secure_event, qos_data.at(core_id_));
  ;
}

DisplayError DPUSingleCore::ControlIdlePowerCollapse(bool enable, bool synchronous) {
  return hw_intf_->ControlIdlePowerCollapse(enable, synchronous);
}

DisplayError DPUSingleCore::SetDisplayDppsAdROI(void *payload) {
  return hw_intf_->SetDisplayDppsAdROI(payload);
}

DisplayError DPUSingleCore::SetDynamicDSIClock(uint64_t bit_clk_rate) {
  return hw_intf_->SetDynamicDSIClock(bit_clk_rate);
}

DisplayError DPUSingleCore::GetDynamicDSIClock(uint64_t *bit_clk_rate) {
  return hw_intf_->GetDynamicDSIClock(bit_clk_rate);
}

DisplayError DPUSingleCore::GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                         uint8_t *out_data) {
  return hw_intf_->GetDisplayIdentificationData(out_port, out_data_size, out_data);
}

DisplayError DPUSingleCore::SetFrameTrigger(FrameTriggerMode mode, uint32_t core_id) {
  DisplayError error = hw_intf_->SetFrameTrigger(mode);
  return error;
}

DisplayError DPUSingleCore::SetFrameTrigger(FrameTriggerMode mode) {
  return hw_intf_->SetFrameTrigger(mode);
}

DisplayError DPUSingleCore::SetBLScale(uint32_t level) {
  return hw_intf_->SetBLScale(level);
}

DisplayError DPUSingleCore::GetPanelBlMaxLvl(uint32_t *max_bl) {
  return hw_intf_->GetPanelBlMaxLvl(max_bl);
}

DisplayError DPUSingleCore::GetPanelBrightnessBasePath(std::string *base_path) const {
  return hw_intf_->GetPanelBrightnessBasePath(base_path);
}

DisplayError DPUSingleCore::SetBlendSpace(const PrimariesTransfer &blend_space) {
  return hw_intf_->SetBlendSpace(blend_space);
}

DisplayError DPUSingleCore::EnableSelfRefresh(SelfRefreshState self_refresh_state) {
  return hw_intf_->EnableSelfRefresh(self_refresh_state);
}

DisplayError DPUSingleCore::GetFeatureSupportStatus(const HWFeature feature, uint32_t *status) {
  return hw_intf_->GetFeatureSupportStatus(feature, status);
}

void DPUSingleCore::FlushConcurrentWriteback() {
  hw_intf_->FlushConcurrentWriteback();
}

DisplayError DPUSingleCore::SetAlternateDisplayConfig(uint32_t *alt_config) {
  return hw_intf_->SetAlternateDisplayConfig(alt_config);
}

DisplayError DPUSingleCore::GetQsyncFps(uint32_t *qsync_fps) {
  return hw_intf_->GetQsyncFps(qsync_fps);
}

DisplayError DPUSingleCore::CancelDeferredPowerMode() {
  return hw_intf_->CancelDeferredPowerMode();
}

PanelFeaturePropertyIntf *DPUSingleCore::GetPanelFeaturePropertyIntf() {
  return hw_intf_->GetPanelFeaturePropertyIntf();
}

void DPUSingleCore::GetHWInterface(HWInterface **intf) {
  *intf = hw_intf_;
}

void DPUSingleCore::GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const {
  hw_intf_->GetDRMDisplayToken(token);
}

DisplayError DPUSingleCore::GetFbConfig(uint32_t width, uint32_t height,
                                        DisplayDeviceContext *device_ctx,
                                        DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  client_ctx->fb_config.x_pixels = width;
  client_ctx->fb_config.y_pixels = height;

  device_ctx->at(core_id_).fb_config = client_ctx->fb_config;

  uint32_t dpu_fb_width =
      client_ctx->fb_config.x_pixels * (device_ctx->at(core_id_).display_attributes.x_pixels /
                                        client_ctx->display_attributes.x_pixels);
  device_ctx->at(core_id_).fb_config.x_pixels = dpu_fb_width;

  return error;
}

}  // namespace sdm
